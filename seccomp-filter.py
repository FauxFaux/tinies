#!/usr/bin/env python3

import collections
import platform
import re
import sys

machine = platform.uname().machine


def usage():
    print('usage: {} script-args -- program [program args..]'.format(sys.argv[0]))


def main():
    if 'x86_64' != machine or 'x86_64' != platform.uname().processor:
        print("non-amd64 machine; you're probably going to have a bad time")

    GROUPS['@critical'] = LOADER_CRITICAL

    args = sys.argv
    try:
        splitter = args.index('--')
    except ValueError:
        usage()
        sys.exit(1)

    args = args[splitter+1:]
    if not args:
        usage()
        sys.exit(2)

    import argparse
    parser = argparse.ArgumentParser(description='Limit syscalls for a command')
    parser.add_argument('syscalls', metavar='N', type=str, nargs='*')
    parser.add_argument('--blacklist', dest='blacklist', action='store_const', const=True, default=False)
    parser.add_argument('--exit-code', type=str)
    parser.add_argument('-v', dest='verbose', action='store_const', const=True, default=False)
    parsed = parser.parse_args(sys.argv[1:splitter])
    seen = set()
    for syscall in parsed.syscalls:
        seen.update(expand_group(syscall))

    for name, reason in {
            'write': 'may impact printing errors (it may just hang)',
            'writev': 'may impact printing errors (it may just hang)',
            'execve': 'we won\'t be able to actually run the child',
            }.items():
        if parsed.blacklist ^ (not SYSCALLS[machine][name] in seen):
            print('warning: {} not present (consider including @critical), {}'.format(name, reason))

    error_code = ERROR_CODES['ENOSYS'][0]
    if parsed.exit_code:
        if parsed.exit_code.isdigit():
            error_code = int(parsed.exit_code)
        else:
            error_code = ERROR_CODES[parsed.exit_code][0]


    cmd = (['seccomp-tool', '--error', str(error_code), '--deny' if parsed.blacklist else '--allow'] +
            [str(syscall) for syscall in sorted(seen)] +
            ['--'] + args)

    if parsed.verbose:
        print('$ ' + ' '.join("'{}'".format(x.replace("'", r"'\''")) for x in cmd))

    import subprocess
    sys.exit(subprocess.call(cmd))


def expand_group(syscall):
    if syscall.isdigit():
        yield int(syscall)
        return

    if syscall[0] != '@':
        try:
            yield SYSCALLS[machine][syscall]
        except KeyError:
            print("warning: unrecognised syscall: " + syscall)

        return

    for item in GROUPS[syscall]:
        yield from expand_group(item)

# all of these seem to be needed to start "echo hello world" on my machine
LOADER_CRITICAL = {
'access',
'arch_prctl',
'brk',
'close',
'execve',
'fstat',
'mmap',
'mprotect',
'open',
'read',
'read',
'readv',
'stat',
'write',
'writev',
}

# cat /usr/include/asm-generic/errno*.h | sed -En 's|^#define[ \t]+(E[A-Z]+)[ \t]+([0-9]+)\s+/\* (.*) \*/|"\1": (\2, "\3"),|p'
ERROR_CODES = {
"EPERM": (1, "Operation not permitted"),
"ENOENT": (2, "No such file or directory"),
"ESRCH": (3, "No such process"),
"EINTR": (4, "Interrupted system call"),
"EIO": (5, "I/O error"),
"ENXIO": (6, "No such device or address"),
"ENOEXEC": (8, "Exec format error"),
"EBADF": (9, "Bad file number"),
"ECHILD": (10, "No child processes"),
"EAGAIN": (11, "Try again"),
"ENOMEM": (12, "Out of memory"),
"EACCES": (13, "Permission denied"),
"EFAULT": (14, "Bad address"),
"ENOTBLK": (15, "Block device required"),
"EBUSY": (16, "Device or resource busy"),
"EEXIST": (17, "File exists"),
"EXDEV": (18, "Cross-device link"),
"ENODEV": (19, "No such device"),
"ENOTDIR": (20, "Not a directory"),
"EISDIR": (21, "Is a directory"),
"EINVAL": (22, "Invalid argument"),
"ENFILE": (23, "File table overflow"),
"EMFILE": (24, "Too many open files"),
"ENOTTY": (25, "Not a typewriter"),
"ETXTBSY": (26, "Text file busy"),
"EFBIG": (27, "File too large"),
"ENOSPC": (28, "No space left on device"),
"ESPIPE": (29, "Illegal seek"),
"EROFS": (30, "Read-only file system"),
"EMLINK": (31, "Too many links"),
"EPIPE": (32, "Broken pipe"),
"EDOM": (33, "Math argument out of domain of func"),
"ERANGE": (34, "Math result not representable"),
"EDEADLK": (35, "Resource deadlock would occur"),
"ENAMETOOLONG": (36, "File name too long"),
"ENOLCK": (37, "No record locks available"),
"ENOSYS": (38, "Invalid system call number"),
"ENOTEMPTY": (39, "Directory not empty"),
"ELOOP": (40, "Too many symbolic links encountered"),
"ENOMSG": (42, "No message of desired type"),
"EIDRM": (43, "Identifier removed"),
"ECHRNG": (44, "Channel number out of range"),
"ELNRNG": (48, "Link number out of range"),
"EUNATCH": (49, "Protocol driver not attached"),
"ENOCSI": (50, "No CSI structure available"),
"EBADE": (52, "Invalid exchange"),
"EBADR": (53, "Invalid request descriptor"),
"EXFULL": (54, "Exchange full"),
"ENOANO": (55, "No anode"),
"EBADRQC": (56, "Invalid request code"),
"EBADSLT": (57, "Invalid slot"),
"EBFONT": (59, "Bad font file format"),
"ENOSTR": (60, "Device not a stream"),
"ENODATA": (61, "No data available"),
"ETIME": (62, "Timer expired"),
"ENOSR": (63, "Out of streams resources"),
"ENONET": (64, "Machine is not on the network"),
"ENOPKG": (65, "Package not installed"),
"EREMOTE": (66, "Object is remote"),
"ENOLINK": (67, "Link has been severed"),
"EADV": (68, "Advertise error"),
"ESRMNT": (69, "Srmount error"),
"ECOMM": (70, "Communication error on send"),
"EPROTO": (71, "Protocol error"),
"EMULTIHOP": (72, "Multihop attempted"),
"EDOTDOT": (73, "RFS specific error"),
"EBADMSG": (74, "Not a data message"),
"EOVERFLOW": (75, "Value too large for defined data type"),
"ENOTUNIQ": (76, "Name not unique on network"),
"EBADFD": (77, "File descriptor in bad state"),
"EREMCHG": (78, "Remote address changed"),
"ELIBACC": (79, "Can not access a needed shared library"),
"ELIBBAD": (80, "Accessing a corrupted shared library"),
"ELIBSCN": (81, ".lib section in a.out corrupted"),
"ELIBMAX": (82, "Attempting to link in too many shared libraries"),
"ELIBEXEC": (83, "Cannot exec a shared library directly"),
"EILSEQ": (84, "Illegal byte sequence"),
"ERESTART": (85, "Interrupted system call should be restarted"),
"ESTRPIPE": (86, "Streams pipe error"),
"EUSERS": (87, "Too many users"),
"ENOTSOCK": (88, "Socket operation on non-socket"),
"EDESTADDRREQ": (89, "Destination address required"),
"EMSGSIZE": (90, "Message too long"),
"EPROTOTYPE": (91, "Protocol wrong type for socket"),
"ENOPROTOOPT": (92, "Protocol not available"),
"EPROTONOSUPPORT": (93, "Protocol not supported"),
"ESOCKTNOSUPPORT": (94, "Socket type not supported"),
"EOPNOTSUPP": (95, "Operation not supported on transport endpoint"),
"EPFNOSUPPORT": (96, "Protocol family not supported"),
"EAFNOSUPPORT": (97, "Address family not supported by protocol"),
"EADDRINUSE": (98, "Address already in use"),
"EADDRNOTAVAIL": (99, "Cannot assign requested address"),
"ENETDOWN": (100, "Network is down"),
"ENETUNREACH": (101, "Network is unreachable"),
"ENETRESET": (102, "Network dropped connection because of reset"),
"ECONNABORTED": (103, "Software caused connection abort"),
"ECONNRESET": (104, "Connection reset by peer"),
"ENOBUFS": (105, "No buffer space available"),
"EISCONN": (106, "Transport endpoint is already connected"),
"ENOTCONN": (107, "Transport endpoint is not connected"),
"ESHUTDOWN": (108, "Cannot send after transport endpoint shutdown"),
"ETOOMANYREFS": (109, "Too many references: cannot splice"),
"ETIMEDOUT": (110, "Connection timed out"),
"ECONNREFUSED": (111, "Connection refused"),
"EHOSTDOWN": (112, "Host is down"),
"EHOSTUNREACH": (113, "No route to host"),
"EALREADY": (114, "Operation already in progress"),
"EINPROGRESS": (115, "Operation now in progress"),
"ESTALE": (116, "Stale file handle"),
"EUCLEAN": (117, "Structure needs cleaning"),
"ENOTNAM": (118, "Not a XENIX named type file"),
"ENAVAIL": (119, "No XENIX semaphores available"),
"EISNAM": (120, "Is a named type file"),
"EREMOTEIO": (121, "Remote I/O error"),
"EDQUOT": (122, "Quota exceeded"),
"ENOMEDIUM": (123, "No medium found"),
"EMEDIUMTYPE": (124, "Wrong medium type"),
"ECANCELED": (125, "Operation Canceled"),
"ENOKEY": (126, "Required key not available"),
"EKEYEXPIRED": (127, "Key has expired"),
"EKEYREVOKED": (128, "Key has been revoked"),
"EKEYREJECTED": (129, "Key was rejected by service"),
"EOWNERDEAD": (130, "Owner died"),
"ENOTRECOVERABLE": (131, "State not recoverable"),
"ERFKILL": (132, "Operation not possible due to RF-kill"),
"EHWPOISON": (133, "Memory page has hardware error"),
}

GROUPS_FILE = 'seccomp-util.c'

# generated by parse_groups()
GROUPS = \
{'@basic-io': {'close', 'dup', 'dup2', 'dup3', 'lseek', 'pread64', 'preadv',
               'pwrite64', 'pwritev', 'read', 'readv', 'write', 'writev'},
 '@clock': {'adjtimex', 'clock_adjtime', 'clock_settime', 'settimeofday',
            'stime'},
 '@cpu-emulation': {'modify_ldt', 'subpage_prot', 'switch_endian', 'vm86',
                    'vm86old'},
 '@debug': {'lookup_dcookie', 'perf_event_open', 'process_vm_readv',
            'process_vm_writev', 'ptrace', 'rtas', 's390_runtime_instr',
            'sys_debug_setcontext'},
 '@default': {'clock_getres', 'clock_gettime', 'clock_nanosleep', 'execve',
              'exit', 'exit_group', 'getrlimit', 'gettimeofday', 'nanosleep',
              'pause', 'rt_sigreturn', 'sigreturn', 'time'},
 '@file-system': {'access', 'chdir', 'chmod', 'close', 'creat', 'faccessat',
                  'fallocate', 'fchdir', 'fchmod', 'fchmodat', 'fcntl',
                  'fcntl64', 'fgetxattr', 'flistxattr', 'fsetxattr', 'fstat',
                  'fstat64', 'fstatat64', 'fstatfs', 'fstatfs64', 'ftruncate',
                  'ftruncate64', 'futimesat', 'getcwd', 'getdents',
                  'getdents64', 'getxattr', 'inotify_add_watch',
                  'inotify_init1', 'inotify_rm_watch', 'lgetxattr', 'link',
                  'linkat', 'listxattr', 'llistxattr', 'lremovexattr',
                  'lsetxattr', 'lstat', 'lstat64', 'mkdir', 'mkdirat', 'mknod',
                  'mknodat', 'mmap', 'mmap2', 'munmap', 'newfstatat', 'open',
                  'openat', 'readlink', 'readlinkat', 'removexattr', 'rename',
                  'renameat', 'renameat2', 'rmdir', 'setxattr', 'stat',
                  'stat64', 'statfs', 'symlink', 'symlinkat', 'truncate',
                  'truncate64', 'unlink', 'unlinkat', 'utimensat', 'utimes'},
 '@io-event': {'_newselect', 'epoll_create', 'epoll_create1', 'epoll_ctl',
               'epoll_ctl_old', 'epoll_pwait', 'epoll_wait', 'epoll_wait_old',
               'eventfd', 'eventfd2', 'poll', 'ppoll', 'pselect6', 'select'},
 '@ipc': {'ipc', 'memfd_create', 'mq_getsetattr', 'mq_notify', 'mq_open',
          'mq_timedreceive', 'mq_timedsend', 'mq_unlink', 'msgctl', 'msgget',
          'msgrcv', 'msgsnd', 'pipe', 'pipe2', 'process_vm_readv',
          'process_vm_writev', 'semctl', 'semget', 'semop', 'semtimedop',
          'shmat', 'shmctl', 'shmdt', 'shmget'},
 '@keyring': {'keyctl', 'request_key', 'add_key'},
 '@module': {'delete_module', 'init_module', 'finit_module'},
 '@mount': {'chroot', 'umount', 'umount2', 'mount', 'pivot_root'},
 '@network-io': {'accept', 'accept4', 'bind', 'connect', 'getpeername',
                 'getsockname', 'getsockopt', 'listen', 'recv', 'recvfrom',
                 'recvmmsg', 'recvmsg', 'send', 'sendmmsg', 'sendmsg', 'sendto',
                 'setsockopt', 'shutdown', 'socket', 'socketcall',
                 'socketpair'},
 '@obsolete': {'_sysctl', 'afs_syscall', 'bdflush', 'break', 'create_module',
               'ftime', 'get_kernel_syms', 'getpmsg', 'gtty', 'lock', 'mpx',
               'prof', 'profil', 'putpmsg', 'query_module', 'security',
               'sgetmask', 'ssetmask', 'stty', 'sysfs', 'tuxcall', 'ulimit',
               'uselib', 'ustat', 'vserver'},
 '@privileged': {'@clock', '@module', '@raw-io', '_sysctl', 'acct', 'bpf',
                 'capset', 'chown', 'chown32', 'chroot', 'fchown', 'fchown32',
                 'fchownat', 'kexec_file_load', 'kexec_load', 'lchown',
                 'lchown32', 'nfsservctl', 'pivot_root', 'quotactl', 'reboot',
                 'setdomainname', 'setfsuid', 'setfsuid32', 'setgroups',
                 'setgroups32', 'sethostname', 'setresuid', 'setresuid32',
                 'setreuid', 'setreuid32', 'setuid', 'setuid32', 'swapoff',
                 'swapon', 'vhangup'},
 '@process': {'arch_prctl', 'clone', 'execveat', 'fork', 'kill', 'prctl',
              'setns', 'tgkill', 'tkill', 'unshare', 'vfork'},
 '@raw-io': {'ioperm', 'iopl', 'pciconfig_iobase', 'pciconfig_read',
             'pciconfig_write', 's390_pci_mmio_read', 's390_pci_mmio_write'},
 '@reboot': {'kexec', 'reboot', 'kexec_file_load'},
 '@resources': {'mbind', 'migrate_pages', 'move_pages', 'prlimit64',
                'sched_setaffinity', 'sched_setattr', 'sched_setparam',
                'sched_setscheduler', 'set_mempolicy', 'setpriority',
                'setrlimit'},
 '@swap': {'swapon', 'swapoff'}}

# </usr/include/x86_64-linux-gnu/asm/unistd_64.h | sed -En "s/#define __NR_([a-z_]+) ([0-9]+)/'\1': \2,/p"
SYSCALLS = {
'x86_64': {
'read': 0,
'write': 1,
'open': 2,
'close': 3,
'stat': 4,
'fstat': 5,
'lstat': 6,
'poll': 7,
'lseek': 8,
'mmap': 9,
'mprotect': 10,
'munmap': 11,
'brk': 12,
'rt_sigaction': 13,
'rt_sigprocmask': 14,
'rt_sigreturn': 15,
'ioctl': 16,
'readv': 19,
'writev': 20,
'access': 21,
'pipe': 22,
'select': 23,
'sched_yield': 24,
'mremap': 25,
'msync': 26,
'mincore': 27,
'madvise': 28,
'shmget': 29,
'shmat': 30,
'shmctl': 31,
'dup': 32,
'pause': 34,
'nanosleep': 35,
'getitimer': 36,
'alarm': 37,
'setitimer': 38,
'getpid': 39,
'sendfile': 40,
'socket': 41,
'connect': 42,
'accept': 43,
'sendto': 44,
'recvfrom': 45,
'sendmsg': 46,
'recvmsg': 47,
'shutdown': 48,
'bind': 49,
'listen': 50,
'getsockname': 51,
'getpeername': 52,
'socketpair': 53,
'setsockopt': 54,
'getsockopt': 55,
'clone': 56,
'fork': 57,
'vfork': 58,
'execve': 59,
'exit': 60,
'kill': 62,
'uname': 63,
'semget': 64,
'semop': 65,
'semctl': 66,
'shmdt': 67,
'msgget': 68,
'msgsnd': 69,
'msgrcv': 70,
'msgctl': 71,
'fcntl': 72,
'flock': 73,
'fsync': 74,
'fdatasync': 75,
'truncate': 76,
'ftruncate': 77,
'getdents': 78,
'getcwd': 79,
'chdir': 80,
'fchdir': 81,
'rename': 82,
'mkdir': 83,
'rmdir': 84,
'creat': 85,
'link': 86,
'unlink': 87,
'symlink': 88,
'readlink': 89,
'chmod': 90,
'fchmod': 91,
'chown': 92,
'fchown': 93,
'lchown': 94,
'umask': 95,
'gettimeofday': 96,
'getrlimit': 97,
'getrusage': 98,
'sysinfo': 99,
'times': 100,
'ptrace': 101,
'getuid': 102,
'syslog': 103,
'getgid': 104,
'setuid': 105,
'setgid': 106,
'geteuid': 107,
'getegid': 108,
'setpgid': 109,
'getppid': 110,
'getpgrp': 111,
'setsid': 112,
'setreuid': 113,
'setregid': 114,
'getgroups': 115,
'setgroups': 116,
'setresuid': 117,
'getresuid': 118,
'setresgid': 119,
'getresgid': 120,
'getpgid': 121,
'setfsuid': 122,
'setfsgid': 123,
'getsid': 124,
'capget': 125,
'capset': 126,
'rt_sigpending': 127,
'rt_sigtimedwait': 128,
'rt_sigqueueinfo': 129,
'rt_sigsuspend': 130,
'sigaltstack': 131,
'utime': 132,
'mknod': 133,
'uselib': 134,
'personality': 135,
'ustat': 136,
'statfs': 137,
'fstatfs': 138,
'sysfs': 139,
'getpriority': 140,
'setpriority': 141,
'sched_setparam': 142,
'sched_getparam': 143,
'sched_setscheduler': 144,
'sched_getscheduler': 145,
'sched_get_priority_max': 146,
'sched_get_priority_min': 147,
'sched_rr_get_interval': 148,
'mlock': 149,
'munlock': 150,
'mlockall': 151,
'munlockall': 152,
'vhangup': 153,
'modify_ldt': 154,
'pivot_root': 155,
'_sysctl': 156,
'prctl': 157,
'arch_prctl': 158,
'adjtimex': 159,
'setrlimit': 160,
'chroot': 161,
'sync': 162,
'acct': 163,
'settimeofday': 164,
'mount': 165,
'swapon': 167,
'swapoff': 168,
'reboot': 169,
'sethostname': 170,
'setdomainname': 171,
'iopl': 172,
'ioperm': 173,
'create_module': 174,
'init_module': 175,
'delete_module': 176,
'get_kernel_syms': 177,
'query_module': 178,
'quotactl': 179,
'nfsservctl': 180,
'getpmsg': 181,
'putpmsg': 182,
'afs_syscall': 183,
'tuxcall': 184,
'security': 185,
'gettid': 186,
'readahead': 187,
'setxattr': 188,
'lsetxattr': 189,
'fsetxattr': 190,
'getxattr': 191,
'lgetxattr': 192,
'fgetxattr': 193,
'listxattr': 194,
'llistxattr': 195,
'flistxattr': 196,
'removexattr': 197,
'lremovexattr': 198,
'fremovexattr': 199,
'tkill': 200,
'time': 201,
'futex': 202,
'sched_setaffinity': 203,
'sched_getaffinity': 204,
'set_thread_area': 205,
'io_setup': 206,
'io_destroy': 207,
'io_getevents': 208,
'io_submit': 209,
'io_cancel': 210,
'get_thread_area': 211,
'lookup_dcookie': 212,
'epoll_create': 213,
'epoll_ctl_old': 214,
'epoll_wait_old': 215,
'remap_file_pages': 216,
'set_tid_address': 218,
'restart_syscall': 219,
'semtimedop': 220,
'timer_create': 222,
'timer_settime': 223,
'timer_gettime': 224,
'timer_getoverrun': 225,
'timer_delete': 226,
'clock_settime': 227,
'clock_gettime': 228,
'clock_getres': 229,
'clock_nanosleep': 230,
'exit_group': 231,
'epoll_wait': 232,
'epoll_ctl': 233,
'tgkill': 234,
'utimes': 235,
'vserver': 236,
'mbind': 237,
'set_mempolicy': 238,
'get_mempolicy': 239,
'mq_open': 240,
'mq_unlink': 241,
'mq_timedsend': 242,
'mq_timedreceive': 243,
'mq_notify': 244,
'mq_getsetattr': 245,
'kexec_load': 246,
'waitid': 247,
'add_key': 248,
'request_key': 249,
'keyctl': 250,
'ioprio_set': 251,
'ioprio_get': 252,
'inotify_init': 253,
'inotify_add_watch': 254,
'inotify_rm_watch': 255,
'migrate_pages': 256,
'openat': 257,
'mkdirat': 258,
'mknodat': 259,
'fchownat': 260,
'futimesat': 261,
'newfstatat': 262,
'unlinkat': 263,
'renameat': 264,
'linkat': 265,
'symlinkat': 266,
'readlinkat': 267,
'fchmodat': 268,
'faccessat': 269,
'ppoll': 271,
'unshare': 272,
'set_robust_list': 273,
'get_robust_list': 274,
'splice': 275,
'tee': 276,
'sync_file_range': 277,
'vmsplice': 278,
'move_pages': 279,
'utimensat': 280,
'epoll_pwait': 281,
'signalfd': 282,
'timerfd_create': 283,
'eventfd': 284,
'fallocate': 285,
'timerfd_settime': 286,
'timerfd_gettime': 287,
'preadv': 295,
'pwritev': 296,
'rt_tgsigqueueinfo': 297,
'perf_event_open': 298,
'recvmmsg': 299,
'fanotify_init': 300,
'fanotify_mark': 301,
'name_to_handle_at': 303,
'open_by_handle_at': 304,
'clock_adjtime': 305,
'syncfs': 306,
'sendmmsg': 307,
'setns': 308,
'getcpu': 309,
'process_vm_readv': 310,
'process_vm_writev': 311,
'kcmp': 312,
'finit_module': 313,
'sched_setattr': 314,
'sched_getattr': 315,
'seccomp': 317,
'getrandom': 318,
'memfd_create': 319,
'kexec_file_load': 320,
'bpf': 321,
'execveat': 322,
'userfaultfd': 323,
'membarrier': 324,
'copy_file_range': 326,
'pkey_mprotect': 329,
'pkey_alloc': 330,
'pkey_free': 331,
}
}


def parse_groups():
    HEADER = re.compile(r'\.name = "(@[a-z-]+)"')
    SYSCALL = re.compile(r'"(@?[a-z0-9_-]+)\\0"')

    results = collections.defaultdict(set)
    with open(GROUPS_FILE, 'r') as f:
        for l in f.readlines():
            if not re.search('[ \t]\.name|\\\\0"', l):
                continue
            l = l.strip()
            if l.startswith('.name'):
                ma = HEADER.search(l)
                if not ma:
                    raise Exception('invalid line: ' + l)
                block = ma.group(1)
            else:
                ma = SYSCALL.search(l)
                if not ma:
                    raise Exception('invalid syscall: ' + l)
                results[block].add(ma.group(1))

    import pprint
    pp = pprint.PrettyPrinter(compact=True)
    pp.pprint(dict(results))


if '__main__' == __name__:
    main()
