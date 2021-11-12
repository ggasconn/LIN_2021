#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <err.h>
#include <errno.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/semaphore.h>


struct semaphore sem_mtx;
struct semaphore sem_prod;
struct semaphore sem_cons;
int prod_count=0,cons_count=0;
int nr_prod_waiting=0,nr_cons_waiting=0;
struct kfifo cbuffer;

void fifoproc_open(bool openRead) {
/*
while(condición==false) {
    nr_waiting++;
    up(&sem_mtx); // "Libera" el mutex

    // Se bloquea en la cola
    if (down_interruptible(&sem_queue)){
        down(&sem_mtx);
        nr_waiting--;
        up(&sem_mtx);
        return -EINTR;
    }

    // "Adquiere" el mutex 
    if (down_interruptible(&sem_mtx)
    return -EINTR;
}
*/

    if(down_interruptible(&sem_mtx))
        return -EINTR;

    if(openRead==1){
        cons_count++;
        
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
            if (down_interruptible(&sem_mtx)
                return -EINTR;
        }      
    }else{
        prod_count++;
            
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
            if (down_interruptible(&sem_mtx)
                return -EINTR;
        }
    }

    up(&sem_mtx);
}

int fifoproc_write(char* buff, int len) {
/*
 if (nr_waiting>0) {
/* Despierta a uno de los hilos bloqueados
*/
up(&sem_queue);
nr_waiting--;
}
 * */
    

	char kbuffer[MAX_KBUF];

    if (len> MAX_CBUFFER_LEN || len> MAX_KBUF) { return Error;}
    if (copy_from_user(kbuffer,buff,len)) { return Error;}

     if(down_interruptible(&sem_mtx))
        return -EINTR;
    /* Esperar hasta que haya hueco para insertar (debe haber consumidores) */
    
    while (kfifo_avail(&cbuffer)<len && cons_count>0){
        cond_wait(prod,mtx);
    }

    /* Detectar fin de comunicación por error (consumidor cierra FIFO antes) */
    if (cons_count==0) {up(&sem_mtx); return -EPIPE;}

    kfifo_in(&cbuffer,kbuffer,len);
    /* Despertar a posible consumidor bloqueado */
    cond_signal(cons);
    up(&sem_mtx);
    return len;
}

int fifoproc_read(const char* buff, int len) {
    char kbuffer[MAX_KBUF];

    if (len> MAX_CBUFFER_LEN || len> MAX_KBUF) { return Error;}

     if(down_interruptible(&sem_mtx))
        return -EINTR;
    
    while (kfifo_len(&cbuffer)<len && prod_count>0){
        cond_wait(cons,mtx);
    }
    
    if (prod_count==0 && kfifo_is_empty(&cbuffer)) {up(&sem_mtx); return 0;}

    kfifo_out(&cbuffer,kbuffer,len);

    cond_signal(prod);
    up(&sem_mtx);

    if (copy_to_user(buff,kbuffer,len)) { return Error;}
    return len;
}

void fifoproc_release(bool lectura) {
     if(down_interruptible(&sem_mtx))
        return -EINTR;
    if(lectura==1){
        cons_count--;
        cond_signal(prod);
    }else{
        prod_count--;
        cond_signal(cons);
    }
    kfifo_reset(&cbuffer);
    up(&sem_mtx);
}


