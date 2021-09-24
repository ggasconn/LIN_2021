#include <linux/module.h>	/* Requerido por todos los módulos */
#include <linux/kernel.h>	/* Definición de KERN_INFO */
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/uaccess.h>


MODULE_LICENSE("GPL"); 	/*  Licencia del modulo */

#define PARSRE_BUFFER_LENGTH 50

static struct proc_dir_entry *proc_entry;
static char *parserBuffer;

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
  int num;

  if (copy_from_user( parserBuffer, buf, len ))  
    return -EFAULT;
  
  parserBuffer[len] = '\n';

  if (sscanf(parserBuffer, "add %d", &num) == 1) {
    printk("Im adding a new node");
  }else if (sscanf(parserBuffer, "remove %d", &num) == 1) {
      printk("Im removing a new node");
  }

  
  *off+=len;

  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  return 0;
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = modlist_read,
    .proc_write = modlist_write,    
};

/* Función que se invoca cuando se carga el módulo en el kernel */
int modlist_init(void) {
	int ret = 0;

  parserBuffer = (char *)vmalloc(PARSRE_BUFFER_LENGTH);

  if(!parserBuffer) {
    ret = -ENOMEM;
  }else {
    memset(parserBuffer, 0, PARSRE_BUFFER_LENGTH);
    proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);

    if (proc_entry == NULL) {
      ret = -ENOMEM;
      vfree(parserBuffer);
      printk(KERN_INFO "Modlist: Can't create /proc entry\n");
    } else {
      printk(KERN_INFO "Modlist: Module loaded\n");
    }
  }

	return ret;
}

/* Función que se invoca cuando se descarga el módulo del kernel */
void modlist_exit(void) {
	remove_proc_entry("modlist", NULL);
  vfree(parserBuffer);
	printk(KERN_INFO "Modulo modlist descargado. Adios kernel.\n");
}

/* Declaración de funciones init y exit */
module_init(modlist_init);
module_exit(modlist_exit);

