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
#include <linux/semaphore.h>


/* Circular buffer data */
#define BUFFER_SIZE 32
static struct kfifo cbuf; /* Circular buffer */

/* Timer data */
struct timer_list my_timer; /* Structure that describes the kernel timer */

/* /proc entry data */
static struct proc_dir_entry *conf_entry;
static struct proc_dir_entry *consumer_entry;

/* Workqueue data */
static struct workqueue_struct *emptyBufferWQ;
struct work_struct copyValuesWork;

/* Syncro */
DEFINE_SPINLOCK(bufferSpinLock);
struct semaphore sem_mtx;
struct semaphore list_queue;
unsigned int nr_cons_waiting;
unsigned int entryInUse = 0;

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
void noinline trace_code_in_buffer(char* random_code, int cur_buf_size) { asm(" "); }

void noinline trace_code_in_list(char* random_code) { asm(" "); }

void noinline trace_code_read(char* random_code) { asm(" "); }
/* BPFTRACE trace functions */


/* START OF THE ACTUAL CODE */
int cleanList(void) {
    struct list_head *pos = NULL;
    struct list_head *aux = NULL;
    struct list_item *item = NULL;

    if (down_interruptible(&sem_mtx))
        return -EINTR;

    list_for_each_safe(pos, aux, &myList) {
        // Retrieve node from list to later delete it and free its memory
        item = list_entry(pos, struct list_item, links);
        list_del(pos); // Delete node
        
        vfree(item->data);
        vfree(item); // Free memory allocated to store data
    }

    up(&sem_mtx);

    return 0;
}

int storeIntoList(char *buffer, unsigned int codes) {
    char *str;
    unsigned int i;
    struct list_item *item = NULL;

    for (i = 0; i < codes; i++) {
        item = (struct list_item *)vmalloc(sizeof(struct list_item)); // Allocate memory for the new item  

        str = (char *)vmalloc(strlen(code_format));
        memcpy(str, buffer, strlen(code_format));
        buffer += strlen(code_format);
        item->data = str;

        list_add_tail(&item->links, &myList); // Add node to the end of the list
    
        trace_code_in_list(str); /* BPFTRACE FUNCTION */
    }

    return 0;
}

static void copyValues(struct work_struct *work) {
    unsigned long bufferFlags;
    unsigned int extractedBytes = 0;
    unsigned int codes = 0;
    char codeBuffer[9];
    char tempBuffer[BUFFER_SIZE];
    char *ptr = tempBuffer;
    char *ptr2 = tempBuffer;

    spin_lock_irqsave(&bufferSpinLock, bufferFlags);

    while (!kfifo_is_empty(&cbuf)) {
        extractedBytes += kfifo_out(&cbuf, &codeBuffer, strlen(code_format));
        codeBuffer[strlen(code_format)] = '\0';
        printk("kfifo out %d bytes and %s", extractedBytes, codeBuffer);
        extractedBytes = sprintf(ptr, "%s", codeBuffer);
        ptr += extractedBytes;
        codes++;
    }

    kfifo_reset(&cbuf);

    spin_unlock_irqrestore(&bufferSpinLock, bufferFlags);

    down(&sem_mtx);

    storeIntoList(ptr2, codes);

    /* Wake up consumer, if any waiting */
    if (nr_cons_waiting > 0) {
        nr_cons_waiting--;
        up(&list_queue);
    }

    up(&sem_mtx);
}

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
    trace_code_in_buffer(randomCode, kfifo_len(&cbuf)); /* BPFTRACE FUNCTION*/

    /* Check if the emergency_threshold has been passed and if so if the last queued work has ended */
    if (((kfifo_len(&cbuf) * 100) / BUFFER_SIZE) > emergency_threshold 
                                    && work_pending(&copyValuesWork) == 0) {
        printk(">>> CODETIMER: Scheduling work, emergency threshold triggered");
        currentCPU = smp_processor_id();
        queue_work_on((currentCPU % 2 == 0) ? 1 : 0, emptyBufferWQ, &copyValuesWork);
    }

    spin_unlock_irqrestore(&bufferSpinLock, bufferFlags);

    mod_timer(&my_timer, jiffies + (timer_period_ms / 1000) * HZ); 
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

    (*off) += len;  /* Update the file pointer */

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

    lastByte = sprintf(myBufferPtr, "code_format=%s\n", code_format);
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


static int consumer_open(struct inode *inode, struct file *file) {
    if (entryInUse) return -EBUSY;

    entryInUse = 1;

    try_module_get(THIS_MODULE);

    /* Re-activate the timer one second from now */
    add_timer(&my_timer); 

    return 0;
}

static int consumer_release(struct inode *inode, struct file *file) {
    entryInUse = 0;

    /* De-activate the timer */
    del_timer_sync(&my_timer);
    /* Wait until the job ends */
    flush_work(&copyValuesWork);
    /* Empty circulsr buffer */
    kfifo_reset(&cbuf);
    /* Empty linked list */
    cleanList();

    module_put(THIS_MODULE);

    return 0;
}

static ssize_t consumer_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int totalBytes = 0;
    char tempBuffer[512];
    char *ptr = tempBuffer;

    struct list_head *pos = NULL;
    struct list_head *aux = NULL;
    struct list_item *item = NULL;

    if (down_interruptible(&sem_mtx))
        return -EINTR;

    while(list_empty(&myList)) {
        nr_cons_waiting++;

        up(&sem_mtx);

        if (down_interruptible(&list_queue)){
            down(&sem_mtx);
            nr_cons_waiting--;
            up(&sem_mtx);
            return -EINTR;
        }

        if (down_interruptible(&sem_mtx)){
            nr_cons_waiting--;
            return -EINTR;
        }
    }

    list_for_each_safe(pos, aux, &myList) {
        item = list_entry(pos, struct list_item, links);
        totalBytes += sprintf(ptr, "%s\n", item->data);
        ptr += strlen(code_format) + 1;

        trace_code_read(item->data); /* BPFTRACE FUNCTION */

        list_del(pos); // Delete node
        
        vfree(item->data);
        vfree(item); // Free memory allocated to store data
    }

    if (copy_to_user(buf, tempBuffer, totalBytes))
        return -EINVAL;

    up(&sem_mtx);

    (*off) += totalBytes;  /* Update the file pointer */

    return totalBytes;
}


static const struct proc_ops consumer_entry_fops = {
    .proc_read = consumer_read,
    .proc_open = consumer_open,
    .proc_release = consumer_release,
};


int init_timer_module( void ) {
    int retval = 0;

    retval = kfifo_alloc(&cbuf, BUFFER_SIZE, GFP_KERNEL);

    if (retval)
        return -ENOMEM;
    
    conf_entry = proc_create("codeconfig", 0666, NULL, &conf_entry_fops);
    consumer_entry = proc_create("codetimer", 0666, NULL, &consumer_entry_fops);

    if (conf_entry == NULL || consumer_entry == NULL)
        return -ENOMEM;
    
    printk(KERN_INFO ">>> CODETIMER: /proc entries exported successfully");

    spin_lock_init(&bufferSpinLock);

    /* Create timer */
    timer_setup(&my_timer, fire_timer, 0);
    
    my_timer.expires = jiffies + (timer_period_ms / 1000) * HZ;  /* Activate it one second from now */
  
    emptyBufferWQ = create_workqueue("emptyBufferWQ");

    if (!emptyBufferWQ)
        return -ENOMEM;
    
    INIT_WORK(&copyValuesWork, copyValues);

    INIT_LIST_HEAD(&myList);

    sema_init(&sem_mtx, 1);
    sema_init(&list_queue, 0);

    return retval;
}


void cleanup_timer_module( void ) {
    /* Delete circular buffer */
    kfifo_free(&cbuf);

    /* Delete and free all the memory linked list related */
    cleanList();

    /* Delete /proc entry */
    remove_proc_entry("codeconfig", NULL);
    remove_proc_entry("codetimer", NULL);

    /* Wait until completion of the timer function (if it's currently running) and delete timer */
    del_timer_sync(&my_timer);
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Codetimer Module");
MODULE_AUTHOR("Andrés Eduardo Salazar Molina, Guillermo Gascón Celdrán");

module_init( init_timer_module );
module_exit( cleanup_timer_module );