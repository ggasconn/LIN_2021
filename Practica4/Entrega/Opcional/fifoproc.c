#include <linux/string.h>
#include <asm-generic/errno.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/cdev.h> /* Char device related methods */
 

#define MAX_KBUF 64
#define MAX_CBUFFER_LEN 64

#define DEVICE_NAME "fifopipe"

struct semaphore sem_mtx;
struct semaphore sem_prod;
struct semaphore sem_cons;
struct kfifo cbuffer;
int prod_count = 0, cons_count = 0;
int nr_prod_waiting = 0, nr_cons_waiting = 0;

/* Declare a new range of numbers and a new char device */
dev_t deviceRegister;
struct cdev *device = NULL;

static int fifoproc_open(struct inode *inode, struct file *file) {
    
    if (down_interruptible(&sem_mtx))
        return -EINTR;

    if (file->f_mode & FMODE_READ) {
        cons_count++;

        /* Por si hay algún productor bloqueado */
		if (nr_prod_waiting > 0) {	
			up(&sem_prod);
			nr_prod_waiting--;
		}

        while (prod_count < 1) {
            nr_cons_waiting++;
            up(&sem_mtx); // "Libera" el mutex

            // Se bloquea en la cola
            if (down_interruptible(&sem_cons)) {
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
    }else {
        prod_count++;

        if (nr_cons_waiting > 0) {
			up(&sem_cons);
			nr_cons_waiting--;
		}

        while (cons_count<1) {
            nr_prod_waiting++;
            up(&sem_mtx); // "Libera" el mutex

            // Se bloquea en la cola
            if (down_interruptible(&sem_prod)) {
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

    if (down_interruptible(&sem_mtx))
        return -EINTR;
    
    /* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
    while (kfifo_avail(&cbuffer)<len && cons_count>0) {
        nr_prod_waiting++;
        up(&sem_mtx); // "Libera" el mutex

        // Se bloquea en la cola
        if (down_interruptible(&sem_prod)) {
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

    /* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
    if (cons_count==0) { up(&sem_mtx); return -EPIPE; }

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
    int bytesRead;
    
    if (down_interruptible(&sem_mtx))
        return -EINTR;
    
    while (kfifo_len(&cbuffer) < len && prod_count > 0) {
        nr_cons_waiting++;
        up(&sem_mtx); // "Libera" el mutex

        // Se bloquea en la cola
        if (down_interruptible(&sem_cons)) {
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

    bytesRead = kfifo_out(&cbuffer, kbuffer, len);

    if (nr_prod_waiting > 0) {
        up(&sem_prod);
        nr_prod_waiting--;
    }
    up(&sem_mtx);

    if (copy_to_user(buff,kbuffer,len)) { return -EFAULT;}

    return len;
}

static int fifoproc_release(struct inode *inode, struct file *file) {
   
    if (down_interruptible(&sem_mtx))
        return -EINTR;

    if (file->f_mode & FMODE_READ) {
        cons_count--;
        
        if (nr_prod_waiting > 0) {
            nr_prod_waiting--;
            up(&sem_prod);
        }
    }else {
        prod_count--;
        
        if (nr_cons_waiting > 0) {
            nr_cons_waiting--;
            up(&sem_cons);
        }
    }

    if (cons_count == 0 && prod_count == 0)
        kfifo_reset(&cbuffer);
    
    up(&sem_mtx);

    return 0;
}

/* Removed proc_ from method names as it's no longer a /proc entry */
static const struct file_operations fops =
{
    .read = fifoproc_read,
    .write = fifoproc_write,
    .open = fifoproc_open,
    .release = fifoproc_release
};

int fifo_init_module( void ) {
   
    int ret = 0;

    /* Try to register a range of char device numbers */
    if (alloc_chrdev_region(&deviceRegister, 0, 1, DEVICE_NAME) < 0) return -ENOMEM;
    if ((device = cdev_alloc()) == NULL) return -ENOMEM;

    cdev_init(device, &fops); /* Init device with associated ops */

    if (cdev_add(device, deviceRegister, 1)) return -ENOMEM; /* Register new device */

    printk(KERN_INFO "Char device created with major %d and minor %d, can be used with name %s", MAJOR(deviceRegister), MINOR(deviceRegister), DEVICE_NAME);

    ret = kfifo_alloc(&cbuffer, MAX_CBUFFER_LEN, GFP_KERNEL);
    if (ret) return -ENOMEM;

    sema_init(&sem_prod,0);
	sema_init(&sem_cons,0);

	/* Inicializacion a 1 del semáforo que permite acceso en exclusión mutua a la SC */
	sema_init(&sem_mtx,1);

    printk(KERN_INFO ">>> FIFOPROC: Module loaded\n");

	return ret;
}


void fifo_cleanup_module( void ) {
    
    /* Unregister device and numbers if device was created */
    if (device) {
        cdev_del(device);
        unregister_chrdev_region(deviceRegister, 1);
    }

	kfifo_free(&cbuffer);
	printk(KERN_INFO "fifoproc: Modulo descargado.\n");
}


module_init( fifo_init_module );
module_exit( fifo_cleanup_module );