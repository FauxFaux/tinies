#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>

#include <sys/prctl.h>

#include <linux/securebits.h>


static int find_max_cap() {

    FILE *f = fopen("/proc/sys/kernel/cap_last_cap", "r");

    if (!f) {
        perror("fopen /proc");
        exit(EXIT_FAILURE);
    }

    char buf[16] = {};

    size_t read = fread(buf, 1, sizeof(buf) - 1, f);

    if (0 == read || read >= sizeof(buf) - 1 || ferror(f) || !feof(f)) {
        perror("fread");
        if (fclose(f)) {
            perror("fclose");
        }
        exit(EXIT_FAILURE);
    }

    buf[read] = 0;

    int val = atoi(buf);

    if (val <= 0) {
        fprintf(stderr, "??? totally insane value in /proc: %d\n", val);
        exit(EXIT_FAILURE);
    }

    if (fclose(f)) {
        perror("couldn't close file");
        exit(EXIT_FAILURE);
    }

    return val;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("usage: %s other-program [other program args..]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (prctl(PR_SET_SECUREBITS,
            /* SECBIT_KEEP_CAPS off */ SECBIT_KEEP_CAPS_LOCKED |
            SECBIT_NO_SETUID_FIXUP | SECBIT_NO_SETUID_FIXUP_LOCKED |
            SECBIT_NOROOT | SECBIT_NOROOT_LOCKED)) {
        perror("setting securebits");
        fprintf(stderr, "hint: this command requires:\n   sudo setcap cap_setpcap+ep %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int max_cap = find_max_cap();
    for (int cap = 0; cap <= max_cap; ++cap) {
        if (!prctl(PR_CAPBSET_DROP, cap)) {
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
}

