#include <linux/module.h>	/* Requerido por todos los módulos */
#include <linux/kernel.h>	/* Definición de KERN_INFO */
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/list.h>

MODULE_LICENSE("GPL"); 	/*  Licencia del modulo */

#define BUFFER_LENGTH 100

static struct proc_dir_entry *proc_entry;

struct list_head myList;

struct list_item {
  int data;
  struct list_head links;
};

void addNewItem(int num) {
  struct list_item *item = NULL;
  struct list_head *node = NULL;

  // Allocate memory for the new item
  item = (struct list_item *)vmalloc(sizeof(struct list_item));
  // Store the data on the returned memory position
  item->data = num;

  // Add node to the end of the list
  list_add_tail(&item->links, &myList);
}

void removeItem(int num) {

}

void cleanup() {
  struct list_head *pos = NULL;
  struct list_head *aux = NULL;
  struct list_item *item = NULL;

  list_for_each_safe(pos, aux, &myList) {
    // Retrieve node from list to later delete it and free its memory
    item = list_entry(pos, struct list_item, links);
    list_del(pos); // Delete node
    vfree(item); // Free memory allocated to store data
  }
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
  int num;
  int availableSpace = BUFFER_LENGTH - 1;
  char myBuffer[BUFFER_LENGTH];

  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;
  
  if (len > available_space) {
    printk(KERN_INFO ">>> MODLIST: not enough space!!\n");
    return -ENOSPC;
  }

  if (copy_from_user( myBuffer, buf, len ))  
    return -EFAULT;
  
  myBuffer[len] = '\0'; // Built a proper ending string

  // Parse buffer content looking for commands
  if (sscanf(myBuffer, "add %d", &num) == 1) {
    addNewItem(num);
    printk(KERN_INFO ">>> MODLIST: Im adding a new node\n");
  } else if (sscanf(myBuffer, "remove %d", &num) == 1) {
    printk(KERN_INFO ">>> MODLIST: Im removing a new node\n");
  } else if (strcmp(myBuffer, "cleanup") == 1) {
    printk(KERN_INFO ">>> MODLIST: Im cleaning up the list...\n");
  } else {
    cleanup();
    printk(KERN_INFO ">>> MODLIST: ERROR! Command %s is not valid\n", myBuffer);
  }
  
  *off += len;

  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  int nr_bytes;
  struct list_item *item;
  struct list_head *pos;
  char myBuffer[BUFFER_LENGTH];
  
  if ((*off) > 0) /* Tell the application that there is nothing left to read */
      return 0;

  list_for_each(pos, &myList) {
    item = list_entry(pos, struct list_item, links);
    
  }
    
  nr_bytes = strlen(clipboard);
    
  if (len < nr_bytes)
    return -ENOSPC;
  
    /* Transfer data from the kernel to userspace */  
  if (copy_to_user(buf, clipboard,nr_bytes))
    return -EINVAL;
    
  (*off)+=len;  /* Update the file pointer */

  return nr_bytes; 
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = modlist_read,
    .proc_write = modlist_write,    
};

/* Función que se invoca cuando se carga el módulo en el kernel */
int modlist_init(void) {
	int ret = 0;

  proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);

  if (proc_entry == NULL) {
    ret = -ENOMEM;
    printk(KERN_INFO ">>> MODLIST: ERROR! Can't create /proc entry\n");
  } else {
    printk(KERN_INFO ">>> MODLIST: Module loaded\n");
  }

  INIT_LIST_HEAD(&myList); // Initialize linked list

	return ret;
}

/* Función que se invoca cuando se descarga el módulo del kernel */
void modlist_exit(void) {
  cleanup(); // Delete linked list and free all the allocated memory
	remove_proc_entry("modlist", NULL);
	printk(KERN_INFO ">>> MODLIST: Correctly unloaded.\n");
}

/* Declaración de funciones init y exit */
module_init(modlist_init);
module_exit(modlist_exit);

