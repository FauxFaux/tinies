#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sched.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <wait.h>
#include <sys/prctl.h>

#include "cap-helper.h"

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s --reading path command [command args...]\n", argv0);
}

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "--reading")) {
        usage(argv[0]);
        return 1;
    }

    char buffer[512] = "/tmp/root.XXXXXX";
    if (NULL == mkdtemp(buffer)) {
        perror("mkdtemp");
        return 2;
    }

    const char *src = argv[2];

    // 20 ~= min(strlen("/dev/null"), strlen("/dev/urandom"), ..
    const size_t max_sub_path_length = 20;
    if (sizeof(buffer) - strlen(buffer) < max_sub_path_length) {
        fprintf(stderr, "generated temporary file path is too long: %s\n", buffer);
        return 3;
    }

    char *const chroot_end = buffer + strlen(buffer);

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
        pid_t pid = vfork();
        switch (pid) {
            case 0: {
                char *args[] = {"/home/faux/code/tinies/mkchroot", buffer, NULL};
                execvp(args[0], args);
                perror("execvp mkchroot");
                return 31;
            }
            case -1:
                perror("fork");
                return 32;
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

    strcpy(chroot_end, "/foo");
    if (mkdir(buffer, 0700)) {
        perror("mkdir");
        return 40;
    }

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
