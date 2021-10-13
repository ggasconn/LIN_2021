#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define LOOP_COUNT 100
#define SLEEP_TIME 2
#define NR_LEDS 8

unsigned int COLOR_POOL[] = { 0xFC0303, 0xFC8403, 0x7FFC03, 0x03FCC2, 0x2803FC, 0xFC03EC, 0xFC036B, 0xFC0303 };

int main(void) {
    int fd;
    int i, j;

    srand(time(NULL)); /* Initialize seed */

    if ((fd = open("/dev/usb/blinkstick0", O_RDWR)) < 0) {
        printf("ERROR! Couldn't open device /dev/usb/blinkstick0");
        return -1;
    }

    /*
    char buf[200];

    for (i = 0; i < LOOP_COUNT; i++) {
        sprintf(buf, "0:0x%X,1:0x%X,2:0x%X,3:0x%X,4:0x%X,5:0x%X,6:0x%X,7:0x%X", COLOR_POOL[rand() % 7], COLOR_POOL[rand() % 7], 
                                                                                COLOR_POOL[rand() % 7], COLOR_POOL[rand() % 7], 
                                                                                COLOR_POOL[rand() % 7], COLOR_POOL[rand() % 7], 
                                                                                COLOR_POOL[rand() % 7], COLOR_POOL[rand() % 7]);

        write(fd, &buf[0], strlen(buf));
        sleep(SLEEP_TIME);
    }
    */

    for (i = 0; i < LOOP_COUNT; i++) {
        int color = COLOR_POOL[rand() % 7];

        for (j = 0; j < NR_LEDS; j++) {
            char tempBuf[50];
            sprintf(tempBuf, "%d:0x%X", j, color);
            write(fd, &tempBuf[0], strlen(tempBuf));
	    usleep(SLEEP_TIME * 10000);
        }
        
	for (j = NR_LEDS - 1; j != 0; j--) {
            char tempBuf[50];
            sprintf(tempBuf, "%d:0x%X", j, color);
            write(fd, &tempBuf[0], strlen(tempBuf));
	    usleep(SLEEP_TIME * 10000);
        }
    }

    close(fd);

    return 0;
}
