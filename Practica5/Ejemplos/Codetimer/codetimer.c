#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Codetimer Module");
MODULE_AUTHOR("Guillermo Gascón");

#define BUFFER_SIZE 32

struct timer_list my_timer; /* Structure that describes the kernel timer */
static struct kfifo cbuf; /* Circular buffer */

unsigned int emergency_threshold;
unsigned int timer_period_ms = 1000; /* Default 1 sec */
char code_format[9] = "0000Aa";

DEFINE_SPINLOCK(bufferSpinLock);

void noinline trace_timer(char c){
    asm(" ");
}

void noinline trace_timer_msg(char* c){
    asm(" ");
}

/* Function invoked when timer expires (fires) */
static void fire_timer(struct timer_list *timer) {
    unsigned int randomNumber = get_random_int();
    unsigned int i;
    unsigned long bufferFlags;
    char randomCode[9];

    for (i = 0; i < strlen(code_format); i++) {
        if (code_format[i] == '0') {
            if (randomNumber <= 0)
                randomNumber = get_random_int();
            
            sprintf(&randomCode[i], "%d", randomNumber % 10);
            randomNumber /= 10;
        }else {
            if (randomNumber <= 1000)
                randomNumber = get_random_int();

            if (code_format[i] == 'a')
                randomCode[i] = ((randomNumber % 1000) % 26) + 'a';
            else
                randomCode[i] = ((randomNumber % 1000) % 26) + 'A';

            randomNumber /= 1000;
        }
    }

    randomCode[strlen(code_format)] = '\0';
    
    spin_lock_irqsave(&bufferSpinLock, bufferFlags);

    printk(KERN_INFO "Adding code %s with length %ld", randomCode, strlen(randomCode));
    kfifo_in(&cbuf, &randomCode, strlen(randomCode));

    spin_unlock_irqrestore(&bufferSpinLock, bufferFlags);
    
    /* Re-activate the timer one second from now */
    mod_timer(timer, jiffies + (timer_period_ms / 1000) * HZ); 
}

int init_timer_module( void ) {
    int retval = 0;

    retval = kfifo_alloc(&cbuf, BUFFER_SIZE, GFP_KERNEL);

    if (retval)
        return -ENOMEM;
    
    spin_lock_init(&bufferSpinLock);

    /* Create timer */
    timer_setup(&my_timer, fire_timer, 0);
    
    my_timer.expires = jiffies + (timer_period_ms / 1000) * HZ;  /* Activate it one second from now */

    /* Activate the timer for the first time */
    add_timer(&my_timer); 
  
    return retval;
}


void cleanup_timer_module( void ) {
    kfifo_free(&cbuf);

    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );