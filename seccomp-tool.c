#define _GNU_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sched.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <seccomp.h>


static void usage(const char *argv0) {
    fprintf(stderr,
            "usage: %s --error errno --allow|deny syscallno [syscallno..] -- other-command [other args...]\n",
            argv0);
}

static int parse_positive(const char *str) {
    char *end;
    long read = strtol(str, &end, 10);
    if (str + strlen(str) != end || read < 0 || read >= INT_MAX) {
        fprintf(stderr, "error: failed to parse a small, positive integer: '%s'\n", str);
        return INT_MIN;
    }

    return (int) read;
}

int main(int argc, char *argv[]) {

    if (argc < 6 || strcmp(argv[1], "--error") || (strcmp(argv[3], "--allow") && strcmp(argv[3], "--deny"))) {
        usage(argv[0]);
        return 1;
    }

    const int err = parse_positive(argv[2]);
    if (err < 0) {
        return 2;
    }
    const bool default_block = !strcmp(argv[3], "--allow");

    if (-1 == prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl");
        return 10;
    }

    uint32_t rule_action = !default_block ? SCMP_ACT_ERRNO(err) : SCMP_ACT_ALLOW;
    scmp_filter_ctx ctx = seccomp_init(default_block ? SCMP_ACT_ERRNO(err) : SCMP_ACT_ALLOW);

    int arg = 4;
    for (; arg < argc; ++arg) {
        if (!strcmp(argv[arg], "--")) {
            ++arg;
            break;
        }

        const int syscall_no = parse_positive(argv[arg]);

        if (syscall_no < 0) {
            seccomp_release(ctx);
            return 5;
        }

        if (seccomp_rule_add(ctx, rule_action, syscall_no, 0)) {
            perror("rule add");
            fprintf(stderr, "not a valid syscall number for a rule: '%s'\n", argv[arg]);
            seccomp_release(ctx);
            return 20;
        }
    }

    if (arg == argc) {
        usage(argv[0]);
        return 3;
    }

    if (seccomp_load(ctx)) {
        perror("load");
        seccomp_release(ctx);
        return 4;
    }

    seccomp_release(ctx);
    execvp(argv[arg], &argv[arg]);
    perror("execve");
    return 5;
}
