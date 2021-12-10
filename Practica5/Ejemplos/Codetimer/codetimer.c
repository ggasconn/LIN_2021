#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>

struct timer_list my_timer; /* Structure that describes the kernel timer */
static struct workqueue_struct workQueue;
static struct kfifo bufferCircle;
int emergency_threshold;

static struct proc_dir_entry *modlist_entry;
static const struct proc_ops proc_entry_fops=
{
    .proc_read = codetimer_read,
    .proc_open = codetimer_open,
    .proc_release = codetimer_release
};

struct list_head list;

typedef struct {
  struct work_struct transfer_task;
  struct list_head entry;
  char    code[9];
} my_work_t;

my_work_t *work;

char code_format[9]; //= "000aA";

DEFINE_SPINLOCK(spinLock);

struct semaphore sem_mtx;
struct semaphore sem_prod;
struct semaphore sem_cons;
int prod_count =0, cons_count = 0;
int nr_prod_waiting=0, nr_cons_waiting =0;

void noinline trace_timer(char c){
    asm(" ");
}

void noinline trace_timer_msg(char* c){
    asm(" ");
}


/* Work's handler function */
static void my_wq_function( struct work_struct *work)
{
    //my_work_t *my_work = (my_work_t *)work;


    //kfree( (void *)work );
}




static void consumer_wq_function(struct work_struct *work){

}

static void copy_items_into_list(){

}

/* Function invoked when timer expires (fires) */
static void fire_timer(struct timer_list *timer)
{
    unsigned int rd = get_random_int();
    unsigned int rx;
    char chx;
    int i;
    int threshold=0;
    char code[9];

    while(threshold < emergency_threshold){
        for (i = 0; i < strlen(code_format); i++){
            char c = code_format[i];
            rx = rd % 1000;

            if (c == '0'){
                rx = rx %10;
                char st[sizeof(rx)];
                iota(rx, st, 10);
                trace_timer_msg(st);
                strcat(work->code,st);
            }
            else if (c == 'a'){
                chx = (rx % 26) + 'a';
                trace_timer_msg(chx);
                strcat(work->code,chx);
            }
            else{
                chx = (rx % 26) + 'A';
                trace_timer_msg(chx);
                strcat(work->code,chx);
            }
            rd = (rd - rx)/10;
        }
        threshold++;
    }

    trace_timer(work->code);
    
    /* Re-activate the timer one second from now */
    mod_timer(timer, jiffies + HZ); 
}



int init_timer_module( void )
{

    int ret=0;
    
    modlist_entry = proc_create("codetimer", 0666, NULL, &proc_entry_fops);

    ret = kfifo_alloc(&bufferCircle, emergency_threshold, GFP_KERNEL);
    if(ret) return -ENOMEM;

    /* Create a private workqueue*/
    workQueue = create_workqueue("workQueue");

    if(workQueue){
        work = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
        if (work) {
            INIT_WORK( (struct work_struct *)work, my_wq_function );
            work->x = 1;
            ret = queue_work( my_wq, (struct work_struct *)work );
            
            int newCpu;
            int cpu_actual=smp_processor_id();

            if((cpu_actual % 2) == 0) newCpu=cpu_actual+1;
            else newCpu=cpu_actual+2;

            ret = schedule_work_on(newCpu, my_wq, (struct work_struct *)work);
        }
        // work2 = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
        // if(work2){
        // }
    }

    spin_lock_init(&spinLock);
    sema_init(&sem_prod,0);
    sema_init(&sem_cons,0);
    sema_init(&sem_mtx,1);

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
  flush_workqueue(workQueue);
  destroy_workqueue(workQueue);
  remove_proc_entry("codetimer", NULL);
  kfifo_free(&bufferCircle);

}

module_init( init_timer_module );
module_exit( cleanup_timer_module );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("timermod Module");
MODULE_AUTHOR("Juan Carlos Saez");
