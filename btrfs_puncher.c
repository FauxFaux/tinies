#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef FALLOC_FL_PUNCH_HOLE
#	define FALLOC_FL_PUNCH_HOLE 0x02
#endif

#ifndef FALLOC_FL_KEEP_SIZE
#	define FALLOC_FL_KEEP_SIZE 0x01
#endif

int main(void) {
	int fh, size, result;
	char buff[4096], ebuff[4096];
	off_t pos;
	int freed=0;

	memset(ebuff, 0, 4096); // prepare a block of 0's

	fh = open("testfile", O_RDWR, O_NOATIME);
	if (fh == -1) {
		printf("open() error: %i\n",errno);
		return 0;
	}

	while ((size = read(fh, buff, 4096)) > 0) {;
		result = memcmp(buff, ebuff,size);
		if (result == 0) {
			result = fallocate(fh, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, pos, size);
			if (result == -1) {
				printf("fallocate() error: %i\n", errno);
				return 0;
			}
			freed += size;
		}
		pos += size;
	}

	if (size == 0) {
		printf("EOF, freed %i\n", freed);
		return 0;
	}
	if (size == -1) {
		printf("read() error: %i\n", errno);
		return 0;
	}

	return 0;
}

