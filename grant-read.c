#define _GNU_SOURCE

void test();

#include <stdio.h>
#include <stdint.h>

#include <errno.h>
#include <unistd.h>
#include <sched.h>

#include <sys/capability.h>

static int drop_setgroups() {
    FILE *fd = fopen("/proc/self/setgroups", "w");

    if (NULL == fd) {
        if (errno == ENOENT) {
            return 0;
        }
        perror("couldn't deny setgroups, no file");
        return 2;
    }

    if (!fputs("deny", fd)) {
        perror("couldn't deny setgroups, write error");
        fclose(fd);
        return 3;
    }

    fclose(fd);

    return 0;
}

static int map_id(const char *file, uint32_t from, uint32_t to) {

    FILE *fd = fopen(file, "w");
    if (NULL == fd) {
        perror("opening map_id file");
        return 4;
    }

    char buf[10 + 10 + 1 + 2 + 1];
    sprintf(buf, "%u %u 1", from, to);
    if (!fputs(buf, fd)) {
        perror("couldn't write map_id file");
        fclose(fd);
        return 3;
    }

    fclose(fd);
    return 0;
}


int main(int argc, char *argv[]) {
    uid_t real_euid = geteuid();
    gid_t real_egid = getegid();

    cap_value_t caps[] = { CAP_DAC_READ_SEARCH };
    cap_t all = cap_get_proc();
    cap_t orig = cap_dup(all);


    cap_set_flag(all, CAP_INHERITABLE, 0, caps, CAP_SET);
    cap_set_flag(all, CAP_EFFECTIVE, 0, caps, CAP_SET);
    cap_set_flag(all, CAP_PERMITTED, 0, caps, CAP_SET);
    cap_set_proc(all);

    test();

    if (unshare(CLONE_NEWUSER | CLONE_NEWNS)) {
        perror("unshare");
        return 2;
    }

    test();

    if (drop_setgroups()) {
        return 3;
    }

    test();

    if (map_id("/proc/self/uid_map", 0, real_euid)) {
        return 3;
    }

    test();

    if (map_id("/proc/self/gid_map", 0, real_egid)) {
        return 3;
    }

    test();

//    execvp(argv[1], &argv[1]);
//    perror("execvp");
//    return 3;
    return 0;
}

void test() {
    FILE *file = fopen("/etc/shadow", "r");
    if (NULL == file) {
        perror("fopen");
    } else {
        printf("fopen: success\n");
        fclose(file);
    }
}