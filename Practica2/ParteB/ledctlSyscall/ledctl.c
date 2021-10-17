#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

struct tty_driver* kbd_driver= NULL;

/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void){
   printk(KERN_INFO ">>> modleds: loading\n");
   printk(KERN_INFO ">>> modleds: fgconsole is %x\n", fg_console);
   return vc_cons[fg_console].d->port.tty->driver;
}

static inline int set_leds(struct tty_driver* handler, unsigned int mask){
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
}


SYSCALL_DEFINE1(ledctl, unsigned int, mask) {
    int ledRet;
    unsigned int bit1;
    unsigned int bit2;
    unsigned int x;

    kbd_driver = get_kbd_driver_handler();

    if (mask < 0 || mask > 7)
        printk(KERN_INFO ">>> modleds: ERROR, mask is out of range. Should be within 0x0 - 0x7");
    else {
        // Swap bits 
        bit1 = (mask >> 1) & 1;
        bit2 = (mask >> 2) & 1;
        x = (bit1 ^ bit2);
        x = (x << 1) | (x << 2);
        mask = mask ^ x;

        if ((ledRet = set_leds(kbd_driver, (mask) + 0x0)) != 0)
            return ledRet;
    }

    return 0;
}
