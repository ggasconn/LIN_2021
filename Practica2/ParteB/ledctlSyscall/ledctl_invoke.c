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
		printf(">>> ERROR! Invalid parameter number!\n");
		return -1;
	}

	if (sscanf(argv[1], "0x%d", &mask) == 1)
		if ((retCode = lin_ledctl(mask)) != 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			return retCode;
		}

	return 0;
}
