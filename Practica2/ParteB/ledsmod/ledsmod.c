#include <linux/module.h>	/* Requerido por todos los módulos */
#include <linux/kernel.h>	/* Definición de KERN_INFO */
#include <linux/proc_fs.h>
//#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>


MODULE_LICENSE("GPL"); 	/*  Licencia del modulo */

#define ALL_LEDS_OFF 0
#define BUFFER_LENGTH 100

static struct proc_dir_entry *proc_entry;
struct tty_driver* kbd_driver= NULL;

/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void){
   printk(KERN_INFO "modleds: loading\n");
   printk(KERN_INFO "modleds: fgconsole is %x\n", fg_console);
   return vc_cons[fg_console].d->port.tty->driver;
}

static inline int set_leds(struct tty_driver* handler, unsigned int mask){
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}


static ssize_t ledsmod_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
   
   char kbuff[BUFFER_LENGTH];
   unsigned int mask;

   if((*off) > 0) return 0;

    if (len > BUFFER_LENGTH -1)
      {
        printk(KERN_INFO "NOT ENOUGH SPACE\n");
        return -ENOSPC;
      }
  
  if(copy_from_user(kbuff, buf, len)) return -EFAULT;

  kbuff[len]='\0';

  printk("%s", kbuff);

  if (sscanf(kbuff, "0x%d", &mask) == 1)
  {
    printk("Found: %u", mask);
    mask += 0x0;
    set_leds(kbd_driver, mask);
  }

  ((*off)) += len;
  return len;
  

}


static const struct proc_ops proc_entry_fops = {
    .proc_write = ledsmod_write,    
};


/* Función que se invoca cuando se carga el módulo en el kernel */
int ledsmod_init(void) {
  int ret =0;
  proc_entry = proc_create( "ledsmod", 0666, NULL, &proc_entry_fops);
	if (proc_entry == NULL)
  {
    ret = -ENOMEM;
  }
  
  kbd_driver= get_kbd_driver_handler();


  return ret;
}

/* Función que se invoca cuando se descarga el módulo del kernel */
void ledsmod_exit(void) {
  set_leds(kbd_driver,ALL_LEDS_OFF); 
	remove_proc_entry("ledsmod", NULL);
	printk(KERN_INFO "Modulo ledsmod descargado. Adios kernel.\n");
}

/* Declaración de funciones init y exit */
module_init(ledsmod_init);
module_exit(ledsmod_exit);

