#define _XOPEN_SOURCE 500

#include <stdio.h>

#include <ftw.h>
#include <sys/stat.h>

int create_callback(const char *seg,
                    const struct stat *s, int types, struct FTW *ftw) {
    if ((types & FTW_D) == FTW_D)
        printf(" D %s\n", seg);
    else
        printf(" - %s\n", seg);
    return 0;
}

int main() {
    return nftw(".", &create_callback, 32, FTW_PHYS);
}