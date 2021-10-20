#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>

#define __NR_LEDCTL 441

long lin_ledctl(unsigned int mask) {
	return (long) syscall(__NR_LEDCTL, mask);
}

int main(int argc, char**argv) {
	unsigned int mask;
	int retCode;

	if (argc != 2) {
		printf(">>> ERROR! Invalid argument number! Usage: %s [0x0-0x7]\n", argv[0]);
		return -1;
	}

	if (sscanf(argv[1], "%X", &mask) == 1) {
		if ((retCode = lin_ledctl(mask)) != 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			return retCode;
		}
	} else
		printf(">>> Cannot find a valid hex number! Please use 0x0 - 0x7 range\n");

	return 0;
}
