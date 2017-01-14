#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <libgen.h>
#include <sys/wait.h>

const int ESPONGER = 241;
const int ESPONGER_EXEC = 242;

void usage(const char *argv0) {
    printf("usage: %s [--] output-file program [program args..]\n", argv0);
}

int main(int argc, char *argv[]) {
    int arg = 1;
    for (; arg < argc; ++arg) {

        if (argv[arg][0] != '-') {
            break;
        }

        if (2 != strlen(argv[arg])) {
            fprintf(stderr, "invalid argument flag: %s\n", argv[arg]);
            usage(argv[0]);
            return ESPONGER;
        }

        if ('-' == argv[arg][1]) {
            ++arg;
            // handling --
            break;
        }

        switch(argv[arg][1]) {
        case 'v':
            break;
        default:
            fprintf(stderr, "unrecognised argument flag: %s\n", argv[arg]);
            usage(argv[0]);
            return ESPONGER;
        }
    }

    if (arg + 1 >= argc) {
        fprintf(stderr, "not enough arguments\n");
        usage(argv[0]);
        return ESPONGER;
    }

    const char *dest_path = argv[arg++];
    char *dest_path_copy = strdup(dest_path);
    if (!dest_path_copy) {
        perror("strdup dest_path failed");
        return ESPONGER;
    }
    char *dir = dirname(dest_path_copy);
    const char *tmp_template = ".sponger.XXXXXX";
    const int tmp_path_len = strlen(dir) + 1 + strlen(tmp_template) + 1;
    char *tmp_path = calloc(tmp_path_len, sizeof(char));
    snprintf(tmp_path, tmp_path_len, "%s/%s", dir, tmp_template);
    int tmp_file = mkstemp(tmp_path);
    free(dest_path_copy);

    if (-1 == tmp_file) {
        perror("couldn't create temp file");
        return ESPONGER;
    }

    const int copied_args = argc - arg;

    // +1: terminating NULL, automatically set by calloc
    char **new_args = calloc(copied_args + 1, sizeof(char*));
    if (!new_args) {
        perror("calloc new_args failed");
        return ESPONGER;
    }

    for (int i = 0; i < copied_args; ++i) {
        char *copy = strdup(argv[arg + i]);
        if (!copy) {
            perror("calloc copy failed");
            return ESPONGER;
        }
        new_args[i] = copy;
    }

//    for (int i = 0; i < copied_args + 1; ++i) {
//        printf("%d: %s\n", i, new_args[i]);
//    }

    int pipefd[2];
    if (-1 == pipe(pipefd)) {
        perror("couldn't open communication pipe");
        return ESPONGER;
    }
    int pipe_read = pipefd[0];
    int pipe_write = pipefd[1];

    const pid_t forked = fork();
    switch (forked) {
        case 0: // child
            if (-1 == close(pipe_read)) {
                perror("warning: couldn't close read pipe in child");
            }
            if (!freopen(tmp_path, "w", stdout)) {
                perror("rewriting stdout");
                return ESPONGER;
            }

            if (-1 == close(tmp_file)) {
                perror("couldn't close tmp_file in child");
                return ESPONGER;
            }

            if (!freopen(tmp_path, "w", stderr)) {
                perror("rewriting stderr");
                return ESPONGER;
            }

            // after this point, perror() doesn't do anything useful,
            // so we try to write the results to the prepared pipe

            execvp(new_args[0], new_args);
            const int failure = errno;

            FILE *pipe = fdopen(pipe_write, "a");
            if (!pipe) {
                perror("we failed to fdopen, but we can't tell anyone why");
                return ESPONGER_EXEC;
            }
            const char *msg = strerror(failure);
            const int len = strlen(msg);
            int written = fwrite(msg, sizeof(char), len, pipe);
            if (written != len) {
                perror("we failed to fwrite, but can't tell anyone why");
            }
            fclose(pipe);
            return ESPONGER_EXEC;
        case -1: // failed
            perror("fork failed");
            return ESPONGER;
    }

    for (int i = 0; i < copied_args; ++i) {
        free(new_args[i]);
    }

    free(new_args);

    if (-1 == close(pipe_write)) {
        perror("warning: couldn't close write pipe in parent");
    }

    if (-1 == close(tmp_file)) {
        perror("warning: couldn't close temp file in parent");
    }

    int wstatus = 0;
    int waitpid_failed = waitpid(forked, &wstatus, 0);
    if (-1 == waitpid_failed) {
        perror("waitpid failed");
        return ESPONGER;
    }
    if (!WIFEXITED(wstatus)) {
        fprintf(stderr, "child didn't exit normally\n");
        return ESPONGER;
    }

    int status = WEXITSTATUS(wstatus);

    if (status) {
        if (-1 == unlink(tmp_path)) {
            perror("warning: couldn't clean up temp file");
        }
        if (ESPONGER_EXEC != status) {
            if (-1 == close(pipe_read)) {
                perror("warning: couldn't close read pipe");
            }
            goto cleanup;
        }
        FILE *pipe = fdopen(pipe_read, "r");
        if (!pipe) {
            perror("warning: fdopen pipe_read failed");
            goto cleanup;
        }
        const size_t buf_max = 256;
        char buf[buf_max];
        int buf_read = fread(buf, sizeof(char), buf_max, pipe);
        if (!feof(pipe)) {
            perror("warning: incomplete message from child");
        }
        if (0 != buf_read) {
            buf[buf_read] = 0;
            fprintf(stderr, "execvp failed: %s\n", buf);
        }
        if (0 != fclose(pipe)) {
            perror("warning: parent couldn't close read pipe");
        }
        goto cleanup;
    }

    if (-1 == rename(tmp_path, dest_path)) {
        perror("couldn't rename temp file into place");
        status = ESPONGER;
        goto cleanup;
    }

cleanup:
    free(tmp_path);
    return status;
}
