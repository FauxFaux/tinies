#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include <sys/prctl.h>

#include "all-caps.h"

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("usage: %s other-program [other program args..]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (prctl(PR_SET_SECUREBITS, SECURE_BITS, 0, 0, 0)) {
        perror("setting securebits");
        fprintf(stderr, "hint: this command requires:\n   sudo setcap cap_setpcap+ep %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int max_cap = find_max_cap();
    for (int cap = 0; cap <= max_cap; ++cap) {
        if (!prctl(PR_CAPBSET_DROP, cap, 0, 0, 0)) {
            continue;
        }

        if (EINVAL == errno) {
            // not supported on this kernel
            continue;
        }

        perror("couldn't drop a valid capability");
        return EXIT_FAILURE;
    }

    if (execvp(argv[1], argv + 1)) {
        perror("execvp");
        return EXIT_FAILURE;
    }

    // unreachable: execvp lied to us
    return EXIT_FAILURE;
}

