#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Codetimer Module");
MODULE_AUTHOR("Guillermo Gascón");

/* Circular buffer data */
#define BUFFER_SIZE 32
static struct kfifo cbuf; /* Circular buffer */

/* Timer data */
struct timer_list my_timer; /* Structure that describes the kernel timer */

/* /proc entry data */
static struct proc_dir_entry *conf_entry;

/* Workqueue data */
static struct workqueue_struct *emptyBufferWQ;
struct work_struct copyValuesWork;

DEFINE_SPINLOCK(bufferSpinLock);

/* Double linked list data */
struct list_head myList;

struct list_item {
    char *data;
    struct list_head links;
};

/* Global variables to store config */
unsigned int emergency_threshold = 75; /* Default to 75% */
unsigned int timer_period_ms = 1000; /* Default 1 sec */
char code_format[9] = "0000Aa";


/* BPFTRACE trace functions */
void noinline trace_timer(char c){
    asm(" ");
}

void noinline trace_timer_msg(char* c){
    asm(" ");
}
/* BPFTRACE trace functions */


/* START OF THE ACTUAL CODE */
void cleanList(void) {
    struct list_head *pos = NULL;
    struct list_head *aux = NULL;
    struct list_item *item = NULL;

    list_for_each_safe(pos, aux, &myList) {
        // Retrieve node from list to later delete it and free its memory
        item = list_entry(pos, struct list_item, links);
        list_del(pos); // Delete node
        
        vfree(item->data);
        vfree(item); // Free memory allocated to store data
    } 
}

static void copyValues(struct work_struct *work) { /* TODO */ }

/* Function invoked when timer expires (fires) */
static void fire_timer(struct timer_list *timer) {
    unsigned int randomNumber = get_random_int();
    unsigned int i;
    unsigned long bufferFlags;
    int currentCPU;
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

    printk(KERN_INFO ">>> CODETIMER: Adding code %s with length %ld", randomCode, strlen(randomCode));
    kfifo_in(&cbuf, &randomCode, strlen(randomCode));

    /* Check if the emergency_threshold has been passed and if so if the last queued work has ended */
    if (((kfifo_len(&cbuf) * 100) / BUFFER_SIZE) > emergency_threshold 
                                    && work_pending(&copyValuesWork) == 0) {
        printk(">>> CODETIMER: Scheduling work, emergency threshold triggered");
        currentCPU = smp_processor_id();
        queue_work_on((currentCPU % 2 == 0) ? 1 : 0, emptyBufferWQ, &copyValuesWork);
    }

    spin_unlock_irqrestore(&bufferSpinLock, bufferFlags);
    
    /* Re-activate the timer one second from now */
    mod_timer(timer, jiffies + (timer_period_ms / 1000) * HZ); 
}


static ssize_t config_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    unsigned int value;
    char newCode[9];
    char myBuffer[128];

    if (copy_from_user( myBuffer, buf, len ))  
        return -EFAULT;

    myBuffer[len] = '\0'; // Built a proper ending string

    // Parse buffer content looking for commands
    if (sscanf(myBuffer, "emergency_threshold %d", &value) == 1) {
        emergency_threshold = value;
        printk(KERN_INFO ">>> CODETIMER: Changed emergency_threshold to %dd", emergency_threshold);
    } else if (sscanf(myBuffer, "timer_period_ms %d", &value) == 1) {
        timer_period_ms = value;
        printk(KERN_INFO ">>> CODETIMER: Changed timer_period_ms to %d", timer_period_ms);
    } else if (sscanf(myBuffer, "code_format %s", newCode) == 1) {
        strncpy(code_format, newCode, 9);
        printk(KERN_INFO ">>> CODETIMER: Changed code_format to %s", code_format);
    } else 
        printk(KERN_INFO ">>> [ERROR] CODETIMER: Invalid input parameters!");

    (*off)+=len;  /* Update the file pointer */

    return len;
}

static ssize_t config_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int lastByte = 0;
    int nrBytes = 0;
    char myBuffer[512] = "";
    char *myBufferPtr = myBuffer;

    if ((*off) > 0) return 0; /* Nothing left to read */

    lastByte = sprintf(myBufferPtr, "timer_period_ms=%d\n", timer_period_ms);
    myBufferPtr += lastByte;
    nrBytes += lastByte;

    lastByte = sprintf(myBufferPtr, "emergency_threshold=%d\n", emergency_threshold);
    myBufferPtr += lastByte;
    nrBytes += lastByte;

    lastByte = sprintf(myBufferPtr, "codeFormat=%s\n", code_format);
    myBufferPtr += lastByte;
    nrBytes += lastByte;
  
    /* Transfer data from the kernel to userspace */  
    if (copy_to_user(buf, myBuffer, nrBytes))
        return -EINVAL;

    (*off) += nrBytes;

    return nrBytes; 
}

static const struct proc_ops conf_entry_fops = {
    .proc_read = config_read,
    .proc_write = config_write,
};

int init_timer_module( void ) {
    int retval = 0;

    retval = kfifo_alloc(&cbuf, BUFFER_SIZE, GFP_KERNEL);

    if (retval)
        return -ENOMEM;
    
    conf_entry = proc_create("codeconfig", 0666, NULL, &conf_entry_fops);
    if (conf_entry == NULL)
        return -ENOMEM;
    
    printk(">>> CODETIMER: /proc/codeconfig entry exported successfully");

    spin_lock_init(&bufferSpinLock);

    /* Create timer */
    timer_setup(&my_timer, fire_timer, 0);
    
    my_timer.expires = jiffies + (timer_period_ms / 1000) * HZ;  /* Activate it one second from now */

    /* Activate the timer for the first time */
    add_timer(&my_timer); 
  
    emptyBufferWQ = create_workqueue("emptyBufferWQ");

    if (!emptyBufferWQ)
        return -ENOMEM;
    
    INIT_WORK(&copyValuesWork, copyValues);

    INIT_LIST_HEAD(&myList);

    return retval;
}


void cleanup_timer_module( void ) {
    /* Delete circular buffer */
    kfifo_free(&cbuf);

    /* Delete and free all the memory linked list related */
    cleanList();

    /* Delete /proc entry */
    remove_proc_entry("codeconfig", NULL);

    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);
}

module_init( init_timer_module );
module_exit( cleanup_timer_module );
