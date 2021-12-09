#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

struct timer_list my_timer; /* Structure that describes the kernel timer */
static struct workqueue_struct *bufferCircle;

typedef struct {
  struct work_struct my_work;
  char    code_format[9];
} my_work_t;

my_work_t *work, *work2;

//char code_format[9] = "000aA";

void noinline trace_timer(char c){
    asm(" ");
}

void noinline trace_timer_msg(char* c){
    asm(" ");
}


/* Work's handler function */
static void my_wq_function( struct work_struct *work )
{
  my_work_t *my_work = (my_work_t *)work;

  kfree( (void *)work );
}


static void consumer_wq_function(struct work_struct *work){

}

/* Function invoked when timer expires (fires) */
static void fire_timer(struct timer_list *timer)
{
    unsigned int rd = get_random_int();
    unsigned int rx;
    char chx;
    int i;
    char code[9];

    for (i = 0; i < strlen(work->code_format); i++){
        char c = work->code_format[i];
        rx = rd % 1000;

        if (c == '0'){
            rx = rx %10;
            char st[sizeof(rx)];
            iota(rx, st, 10);
            trace_timer_msg(st);
            strcat(code,st);
        }
        else if (c == 'a'){
            chx = (rx % 26) + 'a';
            trace_timer_msg(chx);
            strcat(code,chx);
        }
        else{
            chx = (rx % 26) + 'A';
            trace_timer_msg(chx);
            strcat(code,chx);
        }
        rd = (rd - rx)/10;
    }

    trace_timer(code);
    
    /* Re-activate the timer one second from now */
    mod_timer(timer, jiffies + HZ); 
}




int init_timer_module( void )
{

    int ret=0;
    
    /* Create a private workqueue*/
    bufferCircle = create_workqueue("bufferCircle");

    if(bufferCircle){
        work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
        if (work) {
            INIT_WORK( (struct work_struct *)work, my_wq_function );
            work->x = 1;
            ret = queue_work( my_wq, (struct work_struct *)work );
        }

        work2 = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
        if(work2){
            int newCpu;
            int cpu_actual=smp_processor_id();
            if((cpu_actual % 2) == 0){
                newCpu=cpu_actual+1;
            }else{
                newCpu=cpu_actual+2;
            }
            ret = schedule_work_on(newCpu, consumer_wq_function);
        }
    }

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
  flush_workqueue(bufferCircle);
  destroy_workqueue(bufferCircle);
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("timermod Module");
MODULE_AUTHOR("Juan Carlos Saez");
