#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>


struct timer_list my_timer; /* Structure that describes the kernel timer */


void noinline trace_timer(char* message){
    asm(" ");
}


/* Function invoked when timer expires (fires) */
static void fire_timer(struct timer_list *timer)
{
    static char flag=0;
    char* message[]={"Tic","Tac"};
        
    if (flag==0)
        trace_timer(message[0]);
        //printk(KERN_INFO "%s\n",message[0]);
    else 
        trace_timer(message[1]);
        //printk(KERN_INFO "%s\n",message[1]);

    flag=~flag;
    
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
