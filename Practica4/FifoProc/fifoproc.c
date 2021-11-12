
#include <linux/string.h>
#include <asm-generic/errno.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
 
#define MAX_KBUF 64
#define MAX_CBUFFER_LEN 64


struct semaphore sem_mtx;
struct semaphore sem_prod;
struct semaphore sem_cons;
int prod_count=0,cons_count=0;
int nr_prod_waiting=0,nr_cons_waiting=0;
struct kfifo cbuffer;

static struct proc_dir_entry *proc_entry;

static int fifoproc_open(struct inode *inode, struct file *file) {

    if(down_interruptible(&sem_mtx))
        return -EINTR;

    if(file->f_mode & FMODE_READ){
		cons_count++;

        /* Por si hay algún productor bloqueado */
		if (nr_prod_waiting > 0){	
			up(&sem_prod);
			nr_prod_waiting--;
		}

        while(prod_count<1) {
            nr_cons_waiting++;
            up(&sem_mtx); // "Libera" el mutex

            // Se bloquea en la cola
            if (down_interruptible(&sem_cons)){
                down(&sem_mtx);
                nr_cons_waiting--;
                up(&sem_mtx);
                return -EINTR;
            }

            // "Adquiere" el mutex 
            if (down_interruptible(&sem_mtx)) {
                nr_cons_waiting--;
                return -EINTR;
            }
        }      
    }else{
        prod_count++;

        if (nr_cons_waiting > 0){
			up(&sem_cons);
			nr_cons_waiting--;
		}

        while(cons_count<1) {
            nr_prod_waiting++;
            up(&sem_mtx); // "Libera" el mutex

            // Se bloquea en la cola
            if (down_interruptible(&sem_prod)){
                down(&sem_mtx);
                nr_prod_waiting--;
                up(&sem_mtx);
                return -EINTR;
            }

            // "Adquiere" el mutex 
            if (down_interruptible(&sem_mtx)) {
                nr_prod_waiting--;
                return -EINTR;
            }
        }
    }
    up(&sem_mtx);
    return 0;
}

static ssize_t fifoproc_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {

	char kbuffer[MAX_KBUF];

    if (len> MAX_CBUFFER_LEN || len> MAX_KBUF) { return -ENOSPC;}
    if (copy_from_user(kbuffer,buff,len)) { return -EFAULT;}

    kbuffer[len] = '\0';
    *off+=len;

     if(down_interruptible(&sem_mtx))
        return -EINTR;
    /* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
    while(kfifo_avail(&cbuffer)<len && cons_count>0) {
        nr_prod_waiting++;
        up(&sem_mtx); // "Libera" el mutex

        // Se bloquea en la cola
        if (down_interruptible(&sem_prod)){
            down(&sem_mtx);
            nr_prod_waiting--;
            up(&sem_mtx);
            return -EINTR;
        }

        // "Adquiere" el mutex 
        if (down_interruptible(&sem_mtx))
            return -EINTR;
    }

    /* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
    if (cons_count==0) {up(&sem_mtx); return -EPIPE;}

    kfifo_in(&cbuffer,kbuffer,len);
    /* Despertar a posible consumidor bloqueado */
    if (nr_cons_waiting>0) {
        up(&sem_cons);
        nr_cons_waiting--;
    }
    up(&sem_mtx);
    return len;
}

static ssize_t fifoproc_read(struct file *filp, char __user *buff, size_t len, loff_t *off) {
    char kbuffer[MAX_KBUF];
    int nr_bytes=0;
    int bytes_extracted;
    int val;

    if ((*off)>0)
        return 0;
    

    if (len> MAX_CBUFFER_LEN || len> MAX_KBUF) { return -ENOSPC;}

     if(down_interruptible(&sem_mtx))
        return -EINTR;
    
    while(kfifo_len(&cbuffer)<len && prod_count>0) {
        nr_cons_waiting++;
        up(&sem_mtx); // "Libera" el mutex

        // Se bloquea en la cola
        if (down_interruptible(&sem_cons)){
            down(&sem_mtx);
            nr_cons_waiting--;
            up(&sem_mtx);
            return -EINTR;
        }

        // "Adquiere" el mutex 
        if (down_interruptible(&sem_mtx))
            return -EINTR;
    }
    
    if (prod_count==0 && kfifo_is_empty(&cbuffer)) {up(&sem_mtx); return 0;}

    bytes_extracted = kfifo_out(&cbuffer,kbuffer,len);

    if (nr_prod_waiting>0) {
        up(&sem_prod);
        nr_prod_waiting--;
    }
    up(&sem_mtx);

    /* 
    if (bytes_extracted!=sizeof(int))
        return -EINVAL;
    */
   
    nr_bytes = sprintf(kbuffer, "%i\n", val);

    if(len < nr_bytes)
        return -ENOSPC;

    if (copy_to_user(buff,kbuffer,nr_bytes)) { return -EFAULT;}

    ((*off))+=nr_bytes;

    return nr_bytes;
}

static int fifoproc_release(struct inode *inode, struct file *file) {
     if(down_interruptible(&sem_mtx))
        return -EINTR;

    if(file->f_mode & FMODE_READ){
        cons_count--;
        
        if (nr_prod_waiting>0) {
            up(&sem_prod);
            nr_prod_waiting--;
        }
    }else{
        prod_count--;
        if (nr_cons_waiting>0) {
            up(&sem_cons);
            nr_cons_waiting--;
        }
    }
    kfifo_reset(&cbuffer);
    up(&sem_mtx);

    return 0;
}

static const struct proc_ops proc_entry_fops =
{
    .proc_read = fifoproc_read,
    .proc_write = fifoproc_write,
    .proc_open = fifoproc_open,
    .proc_release = fifoproc_release
};

int fifo_init_module(void){
    int ret = 0;

    proc_entry = proc_create( "fifoproc", 0666, NULL, &proc_entry_fops);

    if (proc_entry == NULL) {
        ret = -ENOMEM;
        printk(KERN_INFO ">>> FIFOPROC: ERROR! Can't create /proc entry\n");
    } else {
        printk(KERN_INFO ">>> FIFOPROC: Module loaded\n");
    }

    ret = kfifo_alloc(&cbuffer, MAX_CBUFFER_LEN, GFP_KERNEL);
    if(ret)
        return -ENOMEM;

    sema_init(&sem_prod,0);
	sema_init(&sem_cons,0);

	/* Inicializacion a 1 del semáforo que permite acceso en exclusión mutua a la SC */
	sema_init(&sem_mtx,1);

	printk(KERN_INFO "fifoproc: Cargado el Modulo.\n");

	return ret;
}


void fifo_cleanup_module( void )
{
	remove_proc_entry("fifoproc", NULL);
	kfifo_free(&cbuffer);
	printk(KERN_INFO "fifoproc: Modulo descargado.\n");
}


module_init( fifo_init_module );
module_exit( fifo_cleanup_module );



