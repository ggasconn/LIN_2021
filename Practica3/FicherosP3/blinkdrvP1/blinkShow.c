#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define SLEEP_TIME_POLICE 8
#define SLEEP_TIME_BOUNCE 2
#define SLEEP_TIME_GUESS 2
#define NR_LEDS 8

unsigned int COLOR_POOL[] = { 0xFC0303, 0xFC8403, 0x7FFC03, 0x03FCC2, 0x2803FC, 0xFC03EC, 0xFC036B, 0xFC0303 };
unsigned int BLUE = 0x001EFF;
unsigned int RED = 0xFF030F;
unsigned int WHITE = 0xEDEDED;
unsigned int GREEN = 0x00FF3C;

char buf[200];

void guess(int fd) {
	int randomNumber = rand() % 7;
	int guess = -1;
	int finalColor;
	int i, j;
	
	write(fd, 0, 0); // Write empty string to reset all leds	

	printf(">>> Guess the number! Insert your guess [0-7]: ");
	scanf("%d", &guess);

	for (i = 0; i < 5; i++) {
		for (j = 0; j < NR_LEDS; j++) {
			sprintf(buf, "%d:0x%X", j, WHITE);
			write(fd, &buf[0], strlen(buf));
			usleep(SLEEP_TIME_GUESS * 10000);	
		}
	}

	finalColor = (guess == randomNumber) ? GREEN : RED;
	
	sprintf(buf, "0:0x%X,1:0x%X,2:0x%X:,3:0x%X,4:0x%X,5:0x%X,6:0x%X,7:0x%X", finalColor, finalColor, finalColor, finalColor, 
                                                                                finalColor, finalColor, finalColor, finalColor);
	write(fd, &buf[0], strlen(buf));
}

void police(int fd) {
    while (1) {
        sprintf(buf, "0:0x%X,1:0x%X,6:0x%X,7:0x%X", BLUE, BLUE, RED, RED);
        write(fd, &buf[0], strlen(buf));
        usleep(SLEEP_TIME_POLICE * 10000);

        sprintf(buf, "2:0x%X,3:0x%X,4:0x%X,5:0x%X", BLUE, BLUE, RED, RED);
        write(fd, &buf[0], strlen(buf));
        usleep(SLEEP_TIME_POLICE* 10000);
    }
}

void bounce(int fd) {
    int color;
    int i;

    while (1) {
        color = COLOR_POOL[rand() % 7];   
        for (i = 0; i < NR_LEDS; i++) {
            sprintf(buf, "%d:0x%X", i, color);
            write(fd, &buf[0], strlen(buf));
            usleep(SLEEP_TIME_BOUNCE * 10000);
        }

        for (i = NR_LEDS - 1; i != 0; i--) {
            sprintf(buf, "%d:0x%X", i, color);
            write(fd, &buf[0], strlen(buf));
            usleep(SLEEP_TIME_BOUNCE * 10000);
        }
    }
}

int main(int argc, char** argv) {
    int fd;

    if (argc != 2) {
        printf(">>> ERROR! Invalid arguments! Usage: %s [police/bounce/guess]\n", argv[0]);
        return -1;
    }

    srand(time(NULL)); /* Initialize seed */

    if ((fd = open("/dev/usb/blinkstick0", O_RDWR)) < 0) {
        printf("ERROR! Couldn't open device /dev/usb/blinkstick0\n");
        return -1;
    }

    if (strcmp("police", argv[1]) == 0)
        police(fd);
    else if (strcmp("bounce", argv[1]) == 0)
        bounce(fd);
    else if (strcmp("guess", argv[1]) == 0)
        guess(fd);
    else
        printf(">>> Show %s not available!\n", argv[1]);

    close(fd);

    return 0;
}
