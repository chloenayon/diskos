#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
#define IOCTL_TEST _IOR(0, 6,char*)

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);



static inline unsigned char inb(unsigned short usPort)
{
    unsigned char uch;

    asm volatile("inb %1,%0" : "=a" (uch) : "Nd" (usPort) );
    return uch;
}


/* 'printk' version that prints to active tty. */
void my_printk(char *string)
{
  struct tty_struct *my_tty;

  my_tty = current->signal->tty;

  if (my_tty != NULL) {
    (*my_tty->driver->ops->write)(my_tty, string, strlen(string));
    (*my_tty->driver->ops->write)(my_tty, "\015\012", 2);
  }
} 

static int __init initialization_routine(void) {
  printk("<1> Loading module\n");
  return 0;
}

static inline void outb(unsigned char uch,unsigned short usPort)
{
    asm volatile("outb %0,%1"::"a"(uch),"Nd"(usPort)); 
}

static void __exit cleanup_routine(void) {
  printk("<1> Dumping module\n");
  return;
}

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
    printk("in ioctl\n");
    switch(cmd)
    {
	    case IOCTL_TEST:
	    	printk("IOCTL_TEST");
		break;
	}
    return 0;
}

module_init(initialization_routine);
module_exit(cleanup_routine);

