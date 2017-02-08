#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sched.h>
#include <wait.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/prctl.h>

#include "cap-helper.h"

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s --reading path command [command args...]\n", argv0);
}

static bool ends_with(const char *haystack, const char *needle) {
    if (strlen(haystack) < strlen(needle)) {
        return false;
    }

    return !strcmp(haystack + strlen(haystack) - strlen(needle), needle);
}

void print_hint(const char *src) {
    char *resolved = realpath(src, NULL);
    if (!resolved) {
        perror("warning: realpath");
        resolved = strdup("<unable to resolve>");
        if (!resolved) {
            perror("strdup");
            return;
        }
    }

    char *cwd = get_current_dir_name();
    if (!cwd) {
        perror("warning: get_current_dir_name");
        cwd = strdup("<unable to resolve>");
        if (!cwd) {
            perror("strdup");
            return;
        }
    }

    fprintf(stderr, "path must be fully absolute; here's some generated suggestions:\n"
                    " - $(pwd)/%s\n"
                    " - %s\n"
                    " - %s/%s\n",
            src,
            resolved,
            cwd, src);

    free(cwd);
    free(resolved);
}

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "--reading")) {
        usage(argv[0]);
        return 1;
    }

    const char *src = argv[2];

    // non-absolute paths are just plain confusing here, as one expects
    // to be consistent inside and outside the chrot.
    // . and .. paths are also confusing (but not dangerous),
    // and mount --bind hates them anyway.
    if ('/' != *src
        || strstr(src, "/../") || ends_with(src, "/..")
        || strstr(src, "/./") || ends_with(src, "/.")) {

        print_hint(src);
        return 7;
    }

    const char *template = "/tmp/root.XXXXXX";

    // 20 ~= min(strlen("/dev/null"), strlen("/dev/urandom"), ..
    char *buffer = calloc(strlen(template) + 20 + strlen(src), sizeof(char));
    strcpy(buffer, template);

    if (!buffer) {
        perror("calloc");
        return 4;
    }

    if (NULL == mkdtemp(buffer)) {
        perror("mkdtemp");
        return 2;
    }

    assert(strlen(buffer) == strlen(template));

    char *const chroot_end = buffer + strlen(template);

    if (change_cap_state(CAP_SYS_ADMIN, CAP_EFFECTIVE, CAP_SET)) {
        fprintf(stderr, "warning: couldn't effective sys_admin\n");
    }

    if (unshare(CLONE_NEWNS)) {
        perror("unshare");
        return 10;
    }

    if (mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
        perror("mount private");
        return 11;
    }

    if (mount("tmpfs", buffer, "tmpfs", 0, "")) {
        perror("mount tmpfs");
        return 14;
    }

    if (change_cap_state(CAP_SYS_ADMIN, CAP_EFFECTIVE, CAP_CLEAR)) {
        fprintf(stderr, "couldn't drop sys_admin\n");
        return 19;
    }

    strcpy(chroot_end, "/dev");
    if (0 != mkdir(buffer, 0700)) {
        perror("mkdir /dev");
        return 21;
    }

    if (change_cap_state(CAP_MKNOD, CAP_EFFECTIVE, CAP_SET)) {
        fprintf(stderr, "warning: couldn't effective mknod\n");
    }

#define MAKE_DEV(path, type, maj, min) \
    strcpy(chroot_end, path); \
    if (0 != mknod(buffer, type | 0666, makedev(maj, min))) { \
        perror("mknod " path); \
        return 22; \
    }

    MAKE_DEV("/dev/null", S_IFCHR, 1, 3);
    MAKE_DEV("/dev/zero", S_IFCHR, 1, 5);
    MAKE_DEV("/dev/full", S_IFCHR, 1, 7);
    MAKE_DEV("/dev/random", S_IFCHR, 1, 8);
    MAKE_DEV("/dev/urandom", S_IFCHR, 1, 9);

#undef MAKE_DEV

    if (change_cap_state(CAP_MKNOD, CAP_EFFECTIVE, CAP_CLEAR)) {
        fprintf(stderr, "couldn't half-drop mknod\n");
        return 23;
    }

    if (change_cap_state(CAP_MKNOD, CAP_PERMITTED, CAP_CLEAR)) {
        fprintf(stderr, "couldn't drop mknod\n");
        return 24;
    }

    *chroot_end = '\0';

    {
        int pipes[2];
        if (pipe(pipes)) {
            perror("pipe");
            return 34;
        }

        pid_t pid = vfork();
        if (-1 == pid) {
            perror("fork");
            return 32;
        }

        if (0 == pid) {
            while (dup2(pipes[1], STDOUT_FILENO)) {
                if (EINTR == errno) {
                    continue;
                }
                perror("dup2");
                return 35;
            }
            if (close(pipes[0]) || close(pipes[1])) {
                perror("close");
                return 35;
            }

            char *args[] = {"mkchroot", NULL};
            execvp(args[0], args);
            perror("execvp mkchroot");
            return 31;
        }

        if (close(pipes[1])) {
            perror("close");
            return 36;
        }

        int status = 0;
        if (-1 == waitpid(pid, &status, 0)) {
            perror("waitpid");
            return 33;
        }

        if (status) {
            fprintf(stderr, "setting up chroot failed\n");
            return status;
        }
    }

    size_t pos = 1;
    while (pos < strlen(src)) {
        // starting at the start, find the next folder component
        // e.g. "/foo/bar" -> "/foo" the first time, "/foo/bar" the second time
        char *const slash_in_src = strchrnul(src + pos, '/');
        pos = slash_in_src - src + 1;

        // set our path to the src string up to this point
        strncpy(chroot_end, src, pos);
        chroot_end[pos] = '\0';

        // create that directory, but don't panic if it's already there
        if (mkdir(buffer, 0700) && EEXIST != errno) {
            perror("mkdir");
            return 40;
        }
    }

    assert(strlen(buffer) == strlen(src) + strlen(template));

    if (change_cap_state(CAP_SYS_ADMIN, CAP_EFFECTIVE, CAP_SET)) {
        fprintf(stderr, "warning: couldn't effective sys_admin\n");
    }

    if (mount(src, buffer, "", MS_RDONLY | MS_BIND | MS_PRIVATE, NULL)) {
        perror("mount --bind");
        return 41;
    }

    if (change_cap_state(CAP_SYS_ADMIN, CAP_EFFECTIVE, CAP_CLEAR)) {
        fprintf(stderr, "couldn't half-drop sys_admin\n");
        return 43;
    }

    if (change_cap_state(CAP_SYS_ADMIN, CAP_PERMITTED, CAP_CLEAR)) {
        fprintf(stderr, "couldn't drop sys_admin\n");
        return 44;
    }

    if (change_cap_state(CAP_SYS_CHROOT, CAP_EFFECTIVE, CAP_SET)) {
        fprintf(stderr, "warning: couldn't effective sys_admin\n");
    }

    *chroot_end = '\0';
    if (chroot(buffer)) {
        perror("chroot");
        return 45;
    }

    if (change_cap_state(CAP_SYS_CHROOT, CAP_EFFECTIVE, CAP_CLEAR)) {
        fprintf(stderr, "couldn't half-drop sys_chroot\n");
        return 44;
    }

    if (change_cap_state(CAP_SYS_CHROOT, CAP_PERMITTED, CAP_CLEAR)) {
        fprintf(stderr, "couldn't drop sys_chroot\n");
        return 45;
    }

    free(buffer);

    if (chdir("/")) {
        perror("chdir");
        return 47;
    }

    if (change_cap_state(CAP_DAC_READ_SEARCH, CAP_INHERITABLE, CAP_SET)) {
        fprintf(stderr, "error: couldn't inheritable cap_dac_read_search\n");
        return 51;
    }

    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, CAP_DAC_READ_SEARCH, 0, 0)) {
        perror("prctl ambient");
        fprintf(stderr, "error: couldn't ambient cap_dac_read_search\n");
        return 52;
    }

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl no_new_privs");
        return 53;
    }

    execvp(argv[3], &argv[3]);
    perror("execvp");
    return 3;
}
