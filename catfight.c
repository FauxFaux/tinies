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
    if (found < 1 || found >= (ssize_t) buf_size) {
        perror("warning: readlink");
        return 0;
    }
    buf[found] = '\0';
    return atoll(buf);
}

static void copy_fallback(int from, int to, size_t len) {
    size_t remaining = len;
    do {
        ssize_t sent = sendfile(to, from, NULL, remaining);
        if (-1 == sent) {
            perror("sendfile");
            exit(3);
        }
        remaining -= sent;
    } while (remaining > 0);
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

        size_t remaining = src_len;
        do {
            ssize_t copy = copy_file_range(src_fd, NULL, fd, NULL, remaining, 0);
            if (-1 == copy) {
                if (src_len == remaining // it's the first loop
                    && (ENOSYS == errno // the kernel doesn't support it
                        || EXDEV == errno // the files are incompatible due to devices
                        || EINVAL == errno) // the filesystem doesn't like us, e.g. block alignment
                        ) {
                    copy_fallback(src_fd, fd, src_len);
                    break;
                }

                perror("copy_file_range");
                exit(3);
            }
            remaining -= copy;
        } while (remaining > 0);

        int src_close = close(src_fd);
        check("close src", -1 != src_close);

        int dest_close = close(fd);
        check("close dest", -1 != dest_close);

        printf("%" PRIu64 "\n", seek);

        return 0;
    }
}
