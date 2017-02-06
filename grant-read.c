#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <sys/capability.h>
#include <sys/prctl.h>

#define WARN_UNUSED __attribute__((warn_unused_result))

// @return false on success; true otherwise
static bool WARN_UNUSED change_cap_state(const cap_value_t cap, cap_flag_t flag, cap_flag_value_t value) {
    const cap_value_t as_array[] = {cap};

    cap_t all = cap_get_proc();
    if (!all) {
        perror("cap_get_proc");
        return true;
    }

    bool ret = true;

    if (cap_set_flag(all, flag, 1, as_array, value)) {
        perror("cap_set_flag");
        goto done;
    }

    if (cap_set_proc(all)) {
        perror("cap_set_proc");
        goto done;
    }

    ret = false;

    done:
    if (cap_free(all)) {
        perror("cap_free");
        return true;
    }

    return ret;
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s --chroot chroot-dir command [command-arg...]\n", argv0);
}

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "--chroot")) {
        usage(argv[0]);
        return 1;
    }

    if (change_cap_state(CAP_SYS_CHROOT, CAP_EFFECTIVE, CAP_SET)) {
        fprintf(stderr, "warning: couldn't get permission to chroot, trying anyway...\n");
    }

    if (chroot(argv[2])) {
        perror("chroot");
        return 10;
    }

    if (change_cap_state(CAP_SYS_CHROOT, CAP_EFFECTIVE, CAP_CLEAR)) {
        fprintf(stderr, "error: couldn't remove effectiveness of cap_sys_chroot\n");
        return 20;
    }

    if (change_cap_state(CAP_SYS_CHROOT, CAP_PERMITTED, CAP_CLEAR)) {
        fprintf(stderr, "error: couldn't un-permit cap_sys_chroot\n");
        return 21;
    }

    if (change_cap_state(CAP_DAC_READ_SEARCH, CAP_INHERITABLE, CAP_SET)) {
        fprintf(stderr, "error: couldn't inheritable cap_dac_read_search\n");
        return 22;
    }

    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, CAP_DAC_READ_SEARCH, 0, 0)) {
        perror("prctl ambient");
        fprintf(stderr, "error: couldn't ambient cap_dac_read_search\n");
        return 23;
    }

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl no_new_privs");
        return 24;
    }

    execvp(argv[3], &argv[3]);
    perror("execvp");
    return 3;
}
