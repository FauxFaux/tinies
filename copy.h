#include <sys/sendfile.h>
#include <sys/syscall.h>

const int COPY_FILE_TRY_ANOTHER = 2;

static loff_t copy_file_range(int fd_in, loff_t *off_in, int fd_out,
                              loff_t *off_out, size_t len, unsigned int flags) {
    return syscall(__NR_copy_file_range, fd_in, off_in, fd_out,
                   off_out, len, flags);
}

inline static size_t smallest(size_t left, size_t right) {
    return left < right ? left : right;
}

static int try_copy_file_range(int src_fd, int dest_fd, const size_t len) {
    size_t remaining = len;
    do {
        ssize_t copy = copy_file_range(src_fd, NULL, dest_fd, NULL, remaining, 0);
        if (-1 == copy) {
            if (len == remaining // it's the first loop
                && (ENOSYS == errno // the kernel doesn't support it
                    || EXDEV == errno // the files are incompatible due to devices
                    || EBADF == errno // the files are incompatible types
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

    if (0 != fflush(dest)) {
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
