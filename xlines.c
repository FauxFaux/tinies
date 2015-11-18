#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <getopt.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

void usage() {
    fprintf(stderr, "Usage: [-P threads] -- command to run\n");
    fprintf(stderr, "Usage: [-P threads] -c command-to-pass-to-shell\n");
}

int main(int argc, char *argv[]) {
    int c;
    int parallelism = sysconf(_SC_NPROCESSORS_ONLN);
    char *cmd = NULL;

    while ((c = getopt(argc, argv, "P:c:")) != -1) {
        switch (c) {
            case 'P':
                parallelism = atoi(optarg);
                break;
            case 'c':
                cmd = strdup(optarg);
                assert(cmd);
                break;
            case '?':
                usage();
                return 2;
        }
    }

    if ((optind != argc) ^ (NULL == cmd) || parallelism < 1) {
        usage();
        return 3;
    }

    char *tmpdir = strdup("/tmp/xlines-XXXXXX");
    assert(mkdtemp(tmpdir));

    pid_t children[parallelism];
    char *fifo_paths[parallelism];
    const size_t path_size = 64;

    for (int t = 0; t < parallelism; ++t) {
        char *fifo_path = fifo_paths[t] = malloc(path_size);
        fifo_path[0] = 0;
        snprintf(fifo_path, path_size, "%s/fifo-%d", tmpdir, t);
        assert(fifo_path[0]);
        if (mkfifo(fifo_path, 0600)) {
            perror("mkfifo");
            return 6;
        }


        int pid = fork();
        if (0 == pid) { // child
            freopen(fifo_path, "r", stdin);
            unlink(fifo_path);
            if (NULL == cmd) {
                execvp(argv[optind], argv+optind);
            } else {
                char *shell = getenv("SHELL");
                assert(shell);
                execl(shell, shell, "-c", strdup(cmd), NULL);
            }
            perror("exec failed");
            return 5;
        } else if (pid < 0) {
            perror("couldn't fork");
            return 4;
        } else {
            children[t] = pid;
        }
    }

    FILE *fifos[parallelism];

    for (int t = 0; t < parallelism; ++t) {
        fifos[t] = fopen(fifo_paths[t], "w");
        free(fifo_paths[t]);
    }

    int r;
    int output = 0;
    while (EOF != (r = getchar())) {
        fputc(r, fifos[output]);
        if ('\n' == r) {
            output++;
            output %= parallelism;
        }
    }

    for (int t = 0; t < parallelism; ++t) {
        fclose(fifos[t]);
    }

    for (int t = 0; t < parallelism; ++t) {
        int status;
        waitpid(children[t], &status, 0);
    }

    if (rmdir(tmpdir)) {
        perror("couldn't remove temporary directory");
    }
}
