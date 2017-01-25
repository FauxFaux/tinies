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
#include <sys/sendfile.h>
#include <sys/syscall.h>
#include <byteswap.h>

#define check(func, cond) \
    if (!(cond)) { \
        perror(func); \
        exit(4); \
    }

// 22 ~= log(2^64)/log(10) + 1
const size_t max_number_rendering = 22;


static loff_t copy_file_range(int fd_in, loff_t *off_in, int fd_out,
                              loff_t *off_out, size_t len, unsigned int flags) {
    return syscall(__NR_copy_file_range, fd_in, off_in, fd_out,
                   off_out, len, flags);
}

static size_t align16(size_t val) {
    return (val + 16) - (val % 16);
}

static char *append(const char *base, const char *extra) {
    char *created = strndup(base, strlen(base) + strlen(extra));
    check("strndup append", created);
    strcat(created, extra);
    return created;
}

static uint64_t read_hint(const char *hint_path) {
    const size_t buf_size = 22;
    char buf[buf_size];
    ssize_t found = readlink(hint_path, buf, buf_size);
    if (-1 == found) {
        if (ENOENT != errno) {
            perror("warning: readlink");
        }
        return 0;
    }

    if (found >= (ssize_t) buf_size) {
        fprintf(stderr, "warning: invalid link, ignoring\n");
        return 0;
    }
    buf[found] = '\0';
    long long int val = atoll(buf);
    if (val < 0) {
        fprintf(stderr, "warning: invalid link value, ignoring\n");
        return 0;
    }
    return (uint64_t) val;
}

inline static size_t smallest(size_t left, size_t right) {
    return left < right ? left : right;
}

const int COPY_FILE_TRY_ANOTHER = 2;

static int try_copy_file_range(int src_fd, int dest_fd, const size_t len) {
    size_t remaining = len;
    do {
        ssize_t copy = copy_file_range(src_fd, NULL, dest_fd, NULL, remaining, 0);
        if (-1 == copy) {
            if (len == remaining // it's the first loop
                && (ENOSYS == errno // the kernel doesn't support it
                    || EXDEV == errno // the files are incompatible due to devices
                    || EINVAL == errno) // the filesystem doesn't like us, e.g. block alignment
                    ) {
                return COPY_FILE_TRY_ANOTHER;
            }
            return -1;
        }
        remaining -= copy;
    } while (remaining > 0);

    return 0;
}

static int try_sendfile(int from, int to, size_t len) {
    size_t remaining = len;
    do {
        ssize_t sent = sendfile(to, from, NULL, remaining);
        if (-1 == sent) {
            if (EAGAIN == errno) {
                continue;
            }
            if (len == remaining
                && (EINVAL == errno
                    || ENOSYS == errno)) {
                return COPY_FILE_TRY_ANOTHER;
            }

            return -1;
        }
        remaining -= sent;
    } while (remaining > 0);

    return 0;
}

static int try_fread_fwrite(int src_fd, int dest_fd, const size_t len) {
    FILE *src = fdopen(src_fd, "rb");
    if (!src) {
        return 3;
    }

    FILE *dest = fdopen(dest_fd, "wb");
    if (!dest) {
        return 3;
    }

    size_t remaining = len;
    do {
        char buf[8096];
        size_t to_read = smallest(sizeof(buf), remaining);
        size_t found = fread(buf, 1, to_read, src);
        if (found != to_read) {
            return ferror(src);
        }

        size_t written = fwrite(buf, 1, to_read, dest);
        if (written != to_read) {
            return ferror(dest);
        }

        remaining -= to_read;
    } while (remaining > 0);

    if (0 != fclose(src)) {
        return 4;
    }

    if (0 != fclose(dest)) {
        return 5;
    }

    return 0;
}

static int copy_file(int src_fd, int dest_fd, const size_t len) {
    int by_range = try_copy_file_range(src_fd, dest_fd, len);
    if (COPY_FILE_TRY_ANOTHER != by_range) {
        return by_range;
    }

    int by_sendfile = try_sendfile(src_fd, dest_fd, len);
    if (COPY_FILE_TRY_ANOTHER != by_sendfile) {
        return by_sendfile;
    }

    return try_fread_fwrite(src_fd, dest_fd, len);
}

int main(int argc, char *argv[]) {
    if (3 != argc) {
        fprintf(stderr, "usage: %s to from\n", argv[0]);
        return 2;
    }

    const char *dest_root = argv[1];
    const char *src = argv[2];

    char *target_path = strndup(dest_root, strlen(dest_root) + max_number_rendering);
    check("strndup target", target_path);

    int src_fd = open(src, O_RDONLY);
    check("open src", -1 != src_fd);

    size_t src_len;
    {
        struct stat st;
        int stat_err = fstat(src_fd, &st);
        check("stat src", -1 != stat_err);
        src_len = st.st_size;
    }

    char *hint_path = append(dest_root, ".hint");
    const uint64_t hint = read_hint(hint_path);

    for (uint64_t target_num = hint; target_num < UINT64_MAX; ++target_num) {
        sprintf(target_path, "%s.%022" PRIu64, dest_root, target_num);
        int fd = open(target_path, O_CREAT | O_WRONLY, 0744);
        check("open", -1 != fd);
        int lock = flock(fd, LOCK_EX | LOCK_NB);
        if (lock) {
            const int lock_err = errno;
            if (EWOULDBLOCK == lock_err) {
                continue;
            }
            check("flock", lock);
        }

        off_t seek = lseek(fd, 0, SEEK_END);
        check("lseek", -1 != seek);
        seek = lseek(fd, 16 - (seek % 16), SEEK_CUR);
        check("lseek 2", -1 != seek);

        ssize_t written = write(fd, &src_len, sizeof(src_len));
        check("write len", written == sizeof(src_len));

        int copy = copy_file(src_fd, fd, src_len);
        check("copy_file", 0 == copy);

        int src_close = close(src_fd);
        check("close src", -1 != src_close);

        int dest_close = close(fd);
        check("close dest", -1 != dest_close);

        printf("%" PRIu64 "\n", seek);

        return 0;
    }
}
