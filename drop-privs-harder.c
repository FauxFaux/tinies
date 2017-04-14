#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <errno.h>
#include <seccomp.h>

#include <sched.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s other-command [other args...]\n", argv[0]);
        return 1;
    }

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl");
        return 2;
    }

#define FIRST_ARG_CONTAINS_CLONE_NEWUSER \
    SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, CLONE_NEWUSER)

    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);

    if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(unshare), 1,
                FIRST_ARG_CONTAINS_CLONE_NEWUSER)) {
        perror("rule add: unshare");
        return 3;
    }

    // the glibc wrapper doesn't have flags as the first argument, but the syscall does
    if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(clone), 1,
                FIRST_ARG_CONTAINS_CLONE_NEWUSER)) {
        perror("rule add: clone");
        return 3;
    }

    if (seccomp_load(ctx)) {
        perror("load");
        return 4;
    }

    execvp(argv[1], &argv[1]);
    perror("execv");
    return 5;
}
