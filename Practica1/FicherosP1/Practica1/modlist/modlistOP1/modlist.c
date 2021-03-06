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

#ifdef PARTE_OPCIONAL
struct list_item {
  char* data;
  struct list_head links;
};
#else
struct list_item {
  int data;
  struct list_head links;
};
#endif

#ifdef PARTE_OPCIONAL
void addNewItem(char str[]) {
  struct list_item *item = NULL;
  char* s;

  item = (struct list_item *)vmalloc(sizeof(struct list_item)); // Allocate memory for the new item  

  s = (char *)vmalloc(strlen(str));
  memcpy(s, str, strlen(str));
  item->data = s;

  list_add_tail(&item->links, &myList); // Add node to the end of the list
}
#else
void addNewItem(int num) {
  struct list_item *item = NULL;

  item = (struct list_item *)vmalloc(sizeof(struct list_item)); // Allocate memory for the new item  
  item->data = num; // Store the data on the returned memory position

  list_add_tail(&item->links, &myList); // Add node to the end of the list
}
#endif

#ifdef PARTE_OPCIONAL
void removeItem(char str[]) {
  struct list_head *pos = NULL;
  struct list_head *aux = NULL;
  struct list_item *item = NULL;

  list_for_each_safe(pos, aux, &myList) {
    
    item = list_entry(pos, struct list_item, links); // Retrieve node from list to later free its memory

    if (strcmp((char *)item->data, str) == 0) {
      list_del(pos); // Delete node
      vfree(item->data); // Free string memory
      vfree(item);
    }
  }
}
#else
void removeItem(int num) {
  struct list_head *pos = NULL;
  struct list_head *aux = NULL;
  struct list_item *item = NULL;

  list_for_each_safe(pos, aux, &myList) {
    
    item = list_entry(pos, struct list_item, links); // Retrieve node from list to later free its memory

    if (item->data == num) {
      list_del(pos); // Delete node
      vfree(item); 
    }
  }
}
#endif

void cleanup(void) {
  struct list_head *pos = NULL;
  struct list_head *aux = NULL;
  struct list_item *item = NULL;

  list_for_each_safe(pos, aux, &myList) {
    // Retrieve node from list to later delete it and free its memory
    item = list_entry(pos, struct list_item, links);
    list_del(pos); // Delete node

    #ifdef PARTE_OPCIONAL
    vfree(item->data);
    #endif

    vfree(item); // Free memory allocated to store data
  }
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
  int availableSpace = BUFFER_LENGTH - 1;
  char myBuffer[BUFFER_LENGTH];
  #ifdef PARTE_OPCIONAL
  char str[BUFFER_LENGTH];
  #else
  int num;
  #endif

  if ((*off) > 0) /* The application can write in this entry just once !! */
    return 0;
  
  if (len > availableSpace) {
    printk(KERN_INFO ">>> MODLIST: not enough space!!\n");
    return -ENOSPC;
  }

  if (copy_from_user( myBuffer, buf, len ))  
    return -EFAULT;
  
  myBuffer[len] = '\0'; // Built a proper ending string

  // Parse buffer content looking for commands
  #ifdef PARTE_OPCIONAL
  if (sscanf(myBuffer, "add %s", str) == 1) {
    addNewItem(str);
    printk(KERN_INFO ">>> MODLIST: Im adding a new node\n");
  } else if (sscanf(myBuffer, "remove %s", str) == 1) {
    removeItem(str);
    printk(KERN_INFO ">>> MODLIST: Im removing a new node\n");
  #else
  if (sscanf(myBuffer, "add %d", &num) == 1) {
    addNewItem(num);
    printk(KERN_INFO ">>> MODLIST: Im adding a new node\n");
  } else if (sscanf(myBuffer, "remove %d", &num) == 1) {
    removeItem(num);
    printk(KERN_INFO ">>> MODLIST: Im removing a new node\n");
  #endif
  } else if (strcmp((char *)myBuffer, "cleanup\n") == 0) {
    cleanup();
    printk(KERN_INFO ">>> MODLIST: Im cleaning up the list...\n");
  } else {
    printk(KERN_INFO ">>> MODLIST: ERROR! Command %s is not valid\n", myBuffer);
  }
  
  (*off)+=len;  /* Update the file pointer */

  return len;
}

static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
  int nrBytes = 0;
  int lastBytes = 0;
  char myBuffer[BUFFER_LENGTH] = "";
  /* 
    Stores a pointer to the buffer. Each time the buffer is written the pointer 
    moves n bytes forward, being n the amount of bytes written to the buffer.
  */
  char *bufferPtr = myBuffer;

  char tempBuf[BUFFER_LENGTH / 2] = "";  


  struct list_item *item;
  struct list_head *pos;
  
  if ((*off) > 0) /* Tell the application that there is nothing left to read */
      return 0;

  list_for_each(pos, &myList) {
    item = list_entry(pos, struct list_item, links);

    #ifdef PARTE_OPCIONAL
    sprintf(tempBuf, "%s\n", item->data); // sprintf return value to myBuffer's address
    #else
    sprintf(tempBuf, "%d\n", item->data); // sprintf return value to myBuffer's address
    #endif

    if ((nrBytes + strlen(tempBuf)) > BUFFER_LENGTH - 1)
      return -ENOSPC;

    #ifdef PARTE_OPCIONAL
    lastBytes = sprintf(bufferPtr, "%s\n", item->data); // sprintf return value to myBuffer's address
    #else
    lastBytes = sprintf(bufferPtr, "%d\n", item->data); // sprintf return value to myBuffer's address
    #endif

    bufferPtr += lastBytes;
    nrBytes += lastBytes; // item size + newline char
  }
        
  if (len < nrBytes)
    return -ENOSPC;
  
  /* Transfer data from the kernel to userspace */  
  if (copy_to_user(buf, myBuffer, nrBytes))
    return -EINVAL;
    
  (*off)+=len;  /* Update the file pointer */

  return nrBytes; 
}

static const struct proc_ops proc_entry_fops = {
    .proc_read = modlist_read,
    .proc_write = modlist_write,    
};

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

void modlist_exit(void) {
  cleanup(); // Delete linked list and free all the allocated memory
	remove_proc_entry("modlist", NULL);
	printk(KERN_INFO ">>> MODLIST: Correctly unloaded.\n");
}

/* init and exit rewritten declarations */
module_init(modlist_init);
module_exit(modlist_exit);