#pragma once

#include <stdbool.h>

#include <sys/capability.h>

#ifndef WARN_UNUSED
#define WARN_UNUSED __attribute__((warn_unused_result))
#endif

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