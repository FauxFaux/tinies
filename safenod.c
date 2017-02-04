#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    char *tmp = getenv("TMPDIR");
    if (NULL == tmp) {
        tmp = "/tmp";
    }

    char template[512];
    const int end = snprintf(template, sizeof(template), "%s/root.XXXXXX", tmp);
    if (NULL == mkdtemp(template)) {
        perror("mkdtemp");
        return 2;
    }

    // 20 ~= min(strlen("/dev/null"), strlen("/dev/urandom"), ..
    const size_t max_sub_path_length = 20;
    if (sizeof(template) - end < max_sub_path_length) {
        fprintf(stderr, "generated temporary file path is too long: %s\n", template);
    }

    strcpy(template + end, "/dev");
    if (0 != mkdir(template, 0700)) {
        perror("mkdir /dev");
        return 3;
    }

#define MAKE_DEV(path, type, maj, min) \
    strcpy(template + end, path); \
    if (0 != mknod(template, type | 0666, makedev(maj, min))) { \
        perror("mknod " path); \
        return 4; \
    }

    MAKE_DEV("/dev/null", S_IFCHR, 1, 3);
    MAKE_DEV("/dev/zero", S_IFCHR, 1, 5);
    MAKE_DEV("/dev/full", S_IFCHR, 1, 7);
    MAKE_DEV("/dev/random", S_IFCHR, 1, 8);
    MAKE_DEV("/dev/urandom", S_IFCHR, 1, 9);

    template[end] = '\0';
    printf("%s\n", template);
}
