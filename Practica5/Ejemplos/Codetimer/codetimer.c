#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>


struct timer_list my_timer; /* Structure that describes the kernel timer */

char code_format[9] = "000aA";

void noinline trace_timer(char c){
    asm(" ");
}

void noinline trace_timer_msg(char* c){
    asm(" ");
}

/* Function invoked when timer expires (fires) */
static void fire_timer(struct timer_list *timer)
{
    unsigned int rd = get_random_int();
    unsigned int rx = rd % 1000;
    char chx = (rx % 26) + 'a';
    int i;

    for (i = 0; i < strlen(code_format); i++){
        char c = code_format[i];

        if (c == '0')
            trace_timer_msg("Im a 0");
        else if (c == 'a')
            trace_timer_msg("Im a a");
        else
            trace_timer_msg("Im a A");
    }

    trace_timer(chx);
    
    /* Re-activate the timer one second from now */
    mod_timer(timer, jiffies + HZ); 
}

int init_timer_module( void )
{
    /* Create timer */
    timer_setup(&my_timer, fire_timer, 0);
    my_timer.expires=jiffies + HZ;  /* Activate it one second from now */
    /* Activate the timer for the first time */
    add_timer(&my_timer); 
    return 0;
}


void cleanup_timer_module( void ){
  /* Wait until completion of the timer function (if it's currently running) and delete timer */
  del_timer_sync(&my_timer);
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("timermod Module");
MODULE_AUTHOR("Juan Carlos Saez");
