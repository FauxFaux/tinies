#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <ftw.h>
#include <unistd.h>
#include <sched.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>

#include "cap-helper.h"
#include "copy.h"

static char buffer[8096];
static char *chroot_end;
static char *chroot_src;

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s --reading path --chroot-template $(mkchroot) -- command [command args...]\n", argv0);
}

static bool ends_with(const char *haystack, const char *needle) {
    if (strlen(haystack) < strlen(needle)) {
        return false;
    }

    return !strcmp(haystack + strlen(haystack) - strlen(needle), needle);
}

static void print_hint(const char *src) {
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

int create_callback(const char *src,
                    const struct stat *st,
                    int types,
                    struct FTW *ftw) {
    (void)(ftw);

    if (strncmp(chroot_src, src, strlen(chroot_src))) {
        fprintf(stderr, "illegal template: '%s' must be under '%s'\n", src, chroot_src);
        return 1;
    }

    if (strlen(src) - strlen(chroot_src) + (chroot_end - buffer) >= sizeof(buffer)) {
        fprintf(stderr, "chroot part too long: '%s'\n", src);
        return 2;
    }

    strcpy(chroot_end, src + strlen(chroot_src));

    if ((types & FTW_D) == FTW_D) {
        if (mkdir(buffer, 0700) && EEXIST != errno) {
            perror("mkdir");
            return 3;
        }
        return 0;
    }

    if (st->st_size < 0) {
        return 6;
    }

    int from = open(src, O_RDONLY);
    if (-1 == from) {
        perror("open");
        return 4;
    }

    const int to = open(buffer, O_CREAT | O_EXCL | O_WRONLY, !access(src, X_OK) ? 0700 : 0600);
    if (-1 == to) {
        if (close(from)) {
            perror("warning: close");
        }
        return 5;
    }

    int result = copy_file(from, to, (const size_t) st->st_size);
    if (close(from)) {
        perror("warning: close");
    }

    if (close(to)) {
        perror("close");
        return 9;
    }

    return result;
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
            {"reading",          required_argument, 0, 'r'},
            {"chroot-template",  required_argument, 0, 't'},
            {"disable-security", no_argument,       0, 'D'},
            {0, 0,                                  0, 0}
    };

    char *src = NULL;
    chroot_src = NULL;
    bool security = true;

    while (true) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "r:t:", long_options, &option_index);
        if (-1 == c) {
            break;
        }
        switch (c) {
            case 'r':
                free(src);
                src = strdup(optarg);
                if (!src) {
                    perror("strdup");
                    return 7;
                }
                break;
            case 't':
                free(chroot_src);
                chroot_src = strdup(optarg);
                if (!chroot_src) {
                    perror("strdup");
                    return 7;
                }
                break;
            case 'D':
                security = false;
                break;
            case '?':
                usage(argv[0]);
                return 1;
            default:
                fprintf(stderr, "error: unexpected error from getopt\n");
                return 1;
        }
    }

    if (!src || !chroot_src) {
        fprintf(stderr, "both --reading and --chroot-template are required\n");
        usage(argv[0]);
        return 1;
    }

    if (optind == argc) {
        usage(argv[0]);
        return 1;
    }

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
    if (strlen(template) + strlen(src) > sizeof(buffer)) {
        fprintf(stderr, "src path too long: '%s'\n", src);
        return 5;
    }

    strcpy(buffer, template);

    if (NULL == mkdtemp(buffer)) {
        perror("mkdtemp");
        return 2;
    }

    assert(strlen(buffer) == strlen(template));

    chroot_end = buffer + strlen(template);

    if (change_cap_state(CAP_SYS_ADMIN, CAP_EFFECTIVE, CAP_SET)) {
        fprintf(stderr, "warning: couldn't effective sys_admin\n");
    }

    if (security && unshare(CLONE_NEWNS)) {
        perror("unshare");
        return 10;
    }

    if (security && mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
        perror("mount private");
        return 11;
    }

    if (security && mount("tmpfs", buffer, "tmpfs", 0, "")) {
        perror("mount tmpfs");
        return 14;
    }

    if (change_cap_state(CAP_SYS_ADMIN, CAP_EFFECTIVE, CAP_CLEAR)) {
        fprintf(stderr, "couldn't drop sys_admin\n");
        return 19;
    }

    strcpy(chroot_end, "/dev");
    if (mkdir(buffer, 0700)) {
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

    if (nftw(chroot_src, &create_callback, 32, FTW_PHYS)) {
        fprintf(stderr, "failed to populate chroot\n");
        return 49;
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

    execvp(argv[optind], &argv[optind]);
    perror("execvp");
    return 3;
}
