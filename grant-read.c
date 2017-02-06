#define _GNU_SOURCE

void test();

#include <stdio.h>
#include <stdint.h>

#include <errno.h>
#include <unistd.h>
#include <sched.h>

#include <sys/capability.h>
#include <sys/stat.h>
#include <sys/prctl.h>

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

    test();

//    cap_set_flag(all, CAP_INHERITABLE, 1, caps, CAP_CLEAR);
//    cap_set_flag(all, CAP_PERMITTED, 1, caps, CAP_CLEAR);
    cap_set_flag(all, CAP_EFFECTIVE, 1, caps, CAP_SET);
    cap_set_proc(all);


    // when we have capabilities, or change anything, /proc/sys/fs/suid_dumpable kicks in and makes us not dumpable
    // (tested 4.8) this means that /proc/self/setgroups is chown'd to root
    // see fs/proc/base.c's use of "inode->i_uid = GLOBAL_ROOT_UID;" where task_dumpable is false
    // it seems that we don't require any privileges to regain this.

    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0)) {
        perror("dumpable");
    }

    test();

//    execvp(argv[1], &argv[1]);
//    perror("execvp");
//    return 3;


    if (unshare(CLONE_NEWUSER | CLONE_NEWNS)) {
        perror("unshare");
        return 2;
    }

    test();

    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0)) {
        perror("dumpable");
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

    cap_set_proc(all);

    test();

    execvp(argv[1], &argv[1]);
    perror("execvp");
    return 3;
    return 0;
}

void test() {
    FILE *file = fopen("/etc/shadow", "r");
    if (NULL == file) {
        perror("fopen shadow");
    } else {
        printf("fopen shadow: success\n");
        fclose(file);
    }

    printf("I am %u (%u) / %u (%u); pid: %u\n", getuid(), getgid(), geteuid(), getegid(), getpid());
    char dat[255];
    snprintf(dat, sizeof(dat), "/proc/%u/setgroups", getpid());
    struct stat buf;
    if (stat(dat, &buf)) {
        perror("stat");
    } else {
        printf("file is %u (%u), inode: %lu\n", buf.st_uid, buf.st_gid, buf.st_ino);
    }
    printf("\n");
}
