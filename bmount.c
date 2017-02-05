#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <pwd.h>
#include <sys/capability.h>
#include <sys/prctl.h>

static void raise() {
    cap_t init = cap_get_proc();
    if (!init) {
        perror("warning: cap_get_proc");
        return;
    }

    cap_value_t caps[] = { CAP_SYS_ADMIN };

    if (cap_set_flag(init, CAP_EFFECTIVE, 1, caps, CAP_SET)) {
        perror("warning: cap_set_flag");
        cap_free(init);
        return;
    }

    if (cap_set_proc(init)) {
        perror("warning: cap_set_proc");
    }
    cap_free(init);
}

static int do_mount(const char *src, const char *dest) {
    // this whole sanity checking thing is inherently racy, but afaics mount(2)
    // doesn't have any way around this, so let's just roll with it.

    {
        struct stat src_stat;
        if (stat(src, &src_stat)) {
            perror("src stat");
            return 12;
        }

        if (!S_ISDIR(src_stat.st_mode)) {
            fprintf(stderr, "src must be a directory\n");
            return 13;
        }

        if (!access(src, X_OK)) {
            fprintf(stderr, "src must not be traversable\n");
            return 14;
        }

        // is it better to do this, or to chdir/generate paths with ../../.. .. all the way up?
        char *path = strdup(src);
        char *orig = path;
        do {
            if (!access(path, W_OK)) {
                fprintf(stderr, "whole directory tree must not be writable: '%s' is\n", path);
                free(path);
                return 15;
            }
            path = dirname(path);
        } while (strlen(path) > 1);
        free(orig);
    }

    {
        struct stat dest_stat;
        if (stat(src, &dest_stat)) {
            perror("dest stat");
            return 22;
        }

        if (!S_ISDIR(dest_stat.st_mode)) {
            fprintf(stderr, "dest must be a directory\n");
            return 23;
        }

        if (access(dest, W_OK | X_OK)) {
            fprintf(stderr, "dest must be writable\n");
            return 24;
        }

        DIR *dir = opendir(dest);
        if (!dir) {
            perror("opendir(dest)");
            return 25;
        }

        while (1) {
            errno = 0;
            struct dirent *ent = readdir(dir);
            if (NULL == ent) {
                if (!errno) {
                    break;
                }
                perror("readdir");
                return 26;
            }
            if (0 == strcmp(".", ent->d_name) || 0 == strcmp("..", ent->d_name)) {
                continue;
            }
            fprintf(stderr, "dest directory must be empty; contained '%s'\n", ent->d_name);
            closedir(dir);
            return 27;
        }

        if (closedir(dir)) {
            perror("closedir(dest)");
            return 28;
        }
    }

    raise();

    if (mount(src, dest, "", MS_BIND | MS_RDONLY, NULL)) {
        perror("mount");
        return 30;
    }

    return 0;
}

static int clean_path(const char *path) {
    char *resolved = realpath(path, NULL);
    if (!resolved) {
        fprintf(stderr, "error: must be an existing, findable directory: %s\n", path);
        perror("realpath");
        return 2;
    }

    if (strcmp(path, resolved)) {
        fprintf(stderr, "must be a fully resolved path; '%s' != '%s'\n", path, resolved);
        free(resolved);
        return 3;
    }
    free(resolved);

    return 0;
}

static int check_in_mounts(const char *root, const char *path) {
    if (strncmp(root, path, strlen(root))) {
        fprintf(stderr, "dest must be within your mounts directory: '%s'\n", root);
        return 4;
    }

    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s --bind from to\n", argv0);
    fprintf(stderr, "usage: %s --unmount what\n", argv0);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    // paranoia: should be set by setcap'dness
    if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0)) {
        perror("pr_set_dumpable");
        return 41;
    }

    char root[512] = {0};
    {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) {
            perror("getpwuid");
            return 1;
        }

        const int end = snprintf(root, sizeof(root), "%s/.bmounts/", pw->pw_dir);

        if (end > (int) (sizeof(root) - 2)) {
            fprintf(stderr, "your home directory is too long: '%s'\n", root);
            return 2;
        }

        struct stat base;
        if (stat(root, &base)) {
            if (ENOENT == errno) {
                if (mkdir(root, 0700)) {
                    perror("couldn't create ~/.bmounts/");
                    return 3;
                }
            } else {
                perror("stat root");
                return 4;
            }
        }

        if (!S_ISDIR(base.st_mode) || getuid() != base.st_uid) {
            fprintf(stderr, "'%s' must be a directory owned by you\n", root);
            return 5;
        }
    }

    const char *src = argv[2];
    if (clean_path(src)) {
        return 2;
    }

    if (!strcmp("--bind", argv[1])) {
        if (4 != argc) {
            usage(argv[0]);
            return 1;
        }

        const char *dest = argv[3];

        if (clean_path(dest)) {
            return 3;
        }

        if (check_in_mounts(root, dest)) {
            return 4;
        }

        return do_mount(src, dest);
    }

    if (!strcmp("--unmount", argv[1])) {
        if (3 != argc) {
            usage(argv[0]);
            return 1;
        }

        if (check_in_mounts(root, src)) {
            return 5;
        }

        raise();
        if (umount2(src, UMOUNT_NOFOLLOW)) {
            perror("umount2");
            return 6;
        }

        return 0;
    }

    usage(argv[0]);
    return 1;
}

