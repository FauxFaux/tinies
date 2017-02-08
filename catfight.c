#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <byteswap.h>
#include <stdbool.h>
#include <time.h>

#include "copy.h"

#define check(func, cond) \
    if (!(cond)) { \
        perror(func); \
        exit(4); \
    }

// 22 ~= log(2^64)/log(10) + 1
const size_t max_number_rendering = 22;


static size_t align16(size_t val) {
    return (val + 16) - (val % 16);
}

static char *append(const char *base, const char *extra) {
    char *created = calloc(sizeof(char), strlen(base) + strlen(extra));
    check("calloc append", created);
    strcat(created, base);
    strcat(created, extra);
    return created;
}

static void write_hint(const char *hint_path, uint64_t new_val) {
    srand((31 * (31 * (31 * (unsigned int) clock()) + (unsigned int) time(NULL)) + getpid()));
    char dest_val[max_number_rendering];
    sprintf(dest_val, "/%" PRIu64, new_val);

    const size_t avail = strlen(hint_path) + 16;
    char *temp_path = calloc(sizeof(char), avail);
    assert(temp_path);

    try_another:
    snprintf(temp_path, avail, "%s.%d~", hint_path, rand());
    if (-1 == symlink(dest_val, temp_path)) {
        if (EEXIST == errno) {
            goto try_another;
        }

        perror("warning: couldn't create hint temporary file");
        goto fail;
    }

    if (-1 == rename(temp_path, hint_path)) {
        perror("warning: couldn't update hint");
        if (-1 == unlink(temp_path)) {
            perror("warning: ...and couldn't remove hint temporary file");
        }
    }

    fail:
    free(temp_path);
}

static uint64_t read_hint(const char *hint_path) {
    const size_t buf_size = max_number_rendering;
    char buf[buf_size];
    ssize_t found = readlink(hint_path, buf, buf_size);
    if (-1 == found) {
        if (ENOENT == errno) {
            write_hint(hint_path, 0);
        } else {
            perror("warning: readlink");
        }
        return 0;
    }

    if (found >= (ssize_t) buf_size) {
        fprintf(stderr, "warning: invalid link, ignoring\n");
        return 0;
    }
    buf[found] = '\0';
    long long int val = atoll(buf + 1);
    if (val < 0) {
        fprintf(stderr, "warning: invalid link value, ignoring\n");
        return 0;
    }
    return (uint64_t) val;
}

static void usage(const char *name) {
    fprintf(stderr, "usage: %s [-b block-size-in-bytes] [-e extra-data] to from\n", name);
    fprintf(stderr, "usage: %s [-b ...] -u offset-to-extract from\n", name);
}

static void write_fully(int fd, const char *data, const size_t len) {
    if (0 == len) {
        return;
    }

    size_t remaining = len;
    do {
        const ssize_t written = write(fd, data, remaining);
        if (-1 == written) {
            if (EAGAIN == errno) {
                continue;
            }
            perror("write");
            exit(4);
        }
        remaining -= written;
    } while (remaining > 0);
}

enum operation_t {
    MODE_ARCHIVE,
    MODE_UNARCHIVE,
};

int unarchive(const char *dest_root, uint64_t block_size, uint64_t pos) {
    const uint64_t target_file_id = pos / block_size;
    const uint64_t target_file_offset = pos % block_size;

    char *target_path = calloc(sizeof(char), strlen(dest_root) + max_number_rendering + 16);
    check("calloc target", target_path);

    sprintf(target_path, "%s.%022" PRIu64, dest_root, target_file_id);
    int fd = open(target_path, O_RDONLY);
    check("open", -1 != fd);

    off_t seek = lseek(fd, target_file_offset, SEEK_SET);
    check("seek", -1 != seek);

    uint64_t buf[2];
    ssize_t header_read = read(fd, buf, sizeof(buf));
    check("header_read", sizeof(buf) == header_read);

    uint64_t end = buf[0];
    uint64_t extra = buf[1];

    seek = lseek(fd, extra, SEEK_CUR);
    check("seek2", -1 != seek);

    int copy = copy_file(fd, STDOUT_FILENO, end - extra - sizeof(buf));
    check("copy_file", -1 != copy);

    if (-1 == close(fd)) {
        perror("warning: close");
    }

    return 0;
}

int main(int argc, char *argv[]) {

    char *extra = "";
    uint64_t block_size = 1 * 1024ULL * 1024 * 1024;
    int opt;
    enum operation_t operation = MODE_ARCHIVE;
    uint64_t read_pos = 0;

    while ((opt = getopt(argc, argv, "b:e:u:")) != -1) {
        switch (opt) {
            case 'b': {
                int64_t val = atoll(optarg);
                if (val <= 16 || val >= INT64_MAX) {
                    fprintf(stderr, "-b %" PRId64 " must be >16 and sane\n", val);
                    return 2;
                }
                block_size = (uint64_t) val;
                break;
            }
            case 'e':
                extra = optarg;
                break;
            case 'u': {
                operation = MODE_UNARCHIVE;
                int64_t val = atoll(optarg);
                if (val < 16 || val >= INT64_MAX) {
                    fprintf(stderr, "-u %" PRId64 " must be >= 32 and sane\n", val);
                    return 2;
                }
                read_pos = (uint64_t) val;
                break;
            }
            case '?':
                usage(argv[0]);
                return 2;
            default:
                abort();
        }
    }

    if (MODE_UNARCHIVE == operation) {
        if (optind + 1 != argc) {
            usage(argv[0]);
            return 2;
        }

        return unarchive(argv[optind], block_size, read_pos);
    }

    if (optind + 2 != argc) {
        usage(argv[0]);
        return 2;
    }

    const char *dest_root = argv[optind];
    const char *src = argv[optind + 1];

    char *target_path = calloc(sizeof(char), strlen(dest_root) + max_number_rendering);
    check("calloc target", target_path);
    memcpy(target_path, dest_root, strlen(dest_root));

    int src_fd = open(src, O_RDONLY);
    check("open src", -1 != src_fd);

    size_t src_len;
    {
        struct stat st;
        int stat_err = fstat(src_fd, &st);
        check("stat src", -1 != stat_err);
        assert(st.st_size >= 0);
        src_len = (size_t) st.st_size;
    }

    char *hint_path = append(dest_root, ".hint");
    const uint64_t hint = read_hint(hint_path);
    bool skipped_due_to_locking = false;

    for (uint64_t target_num = hint; target_num < UINT64_MAX; ++target_num) {
        sprintf(target_path, "%s.%022" PRIu64, dest_root, target_num);
        int fd = open(target_path, O_CREAT | O_WRONLY, 0644);
        check("open", -1 != fd);
        int lock = flock(fd, LOCK_EX | LOCK_NB);
        if (lock) {
            const int lock_err = errno;
            if (EWOULDBLOCK == lock_err) {
                skipped_due_to_locking = true;
                continue;
            }
            check("flock", lock);
        }

        off_t seek = lseek(fd, 0, SEEK_END);
        check("lseek", -1 != seek);

        if (0 == seek) {
            write_fully(fd, "cf1\0\0\0\0\0", 8);
            write_fully(fd, (const char *) &target_num, 8);
            seek = 16;
        }

        if (seek % 16 != 0) {
            seek = lseek(fd, 16 - (seek % 16), SEEK_CUR);
            check("lseek 2", -1 != seek);
        }

        if (seek >= (off_t) block_size) {
            if (-1 == close(fd)) {
                perror("warning: close unwanted file");
            }
            continue;
        }

        {
            size_t extra_len = strlen(extra);
            size_t record_end = 8 + 8 + src_len + extra_len;
            write_fully(fd, (const char *) &record_end, sizeof(record_end));
            write_fully(fd, (const char *) &extra_len, sizeof(extra_len));
            write_fully(fd, extra, extra_len);
        }

        int copy = copy_file(src_fd, fd, src_len);
        check("copy_file", 0 == copy);

        int src_close = close(src_fd);
        check("close src", -1 != src_close);

        int dest_close = close(fd);
        check("close dest", -1 != dest_close);

        printf("%" PRIu64 "\n", target_num * block_size + seek);

        if (!skipped_due_to_locking && target_num > hint) {
            write_hint(hint_path, target_num);
        }

        return 0;
    }
}
