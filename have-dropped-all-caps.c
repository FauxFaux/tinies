#include <stdio.h>

#include <errno.h>

#include <sys/prctl.h>

#include "all-caps.h"

int main() {
    if (SECURE_BITS != prctl(PR_GET_SECUREBITS, 0, 0, 0, 0)) {
        return 1;
    }

    if (1 != prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0)) {
        return 2;
    }

    const int max_cap = find_max_cap();
    for (int cap = 0; cap <= max_cap; ++cap) {
        int bounded = prctl(PR_CAPBSET_READ, cap, 0, 0, 0);
        if (1 == bounded) {
            return 3;
        }

        if (0 != bounded) {
            if (EINVAL == errno) {
                continue;
            }
            perror("reading bounding set failed");
            return 4;
        }
    }

    return 0;
}