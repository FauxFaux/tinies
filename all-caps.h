#include <stdlib.h>

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

const int SECURE_BITS = /* SECBIT_KEEP_CAPS off */ SECBIT_KEEP_CAPS_LOCKED |
                       SECBIT_NO_SETUID_FIXUP | SECBIT_NO_SETUID_FIXUP_LOCKED |
                       SECBIT_NOROOT | SECBIT_NOROOT_LOCKED;
