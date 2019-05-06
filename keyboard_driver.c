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


static struct file_operations pseudo_dev_proc_operations;

static struct proc_dir_entry *proc_entry;

static char strbuf[10];
static int block;

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

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    int c;
	my_printk("here");
        static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        if(!(inb(0x64)& 0x1)||((c=inb(0x60)) & 0x80))
        {
            block = 1;//we have the key
        }
        else
        {
            block = 0;
            strbuf[0] = scancode[c];
        }
    //printk("irq %d \n",irq);
	return IRQ_RETVAL(1);
}

static int __init initialization_routine(void) {

  block  = 1;
  int (*removeCurrentDriver)(void);
  removeCurrentDriver = (void *) kallsyms_lookup_name("i8042_remove");
  removeCurrentDriver();
  
  printk("<1> Loading module\n");

  pseudo_dev_proc_operations.ioctl = pseudo_device_ioctl;

  /* Start create proc entry */
  proc_entry = create_proc_entry("keyboard_driver", 0444, NULL);
  if(!proc_entry)
  {
    printk("<1> Error creating /proc entry.\n");
    return 1;
  }

  //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
  proc_entry->proc_fops = &pseudo_dev_proc_operations;

  free_irq(1,NULL); 
  return request_irq(1,irq_handler,IRQF_SHARED,"my_keyboard_driver",(void *)(irq_handler)); 
}


static inline void outb(unsigned char uch,unsigned short usPort)
{
    asm volatile("outb %0,%1"::"a"(uch),"Nd"(usPort)); 
}

/*
char my_getchar(void){

    printk("my_getchar waiting");
    
    //request_irq(1,irq_handler,IRQF_SHARED,"my_keyboard_driver",(void *)(irq_handler));
    printk("calling my_getchar");
    char c;

    static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    //while(!(inb(0x64)& 0x1)||((c=inb(0x60)) & 0x80));
    //c = inb(0x60);
    return scancode[(int)c];
}

*/

static void __exit cleanup_routine(void) {

  printk("<1> Dumping module\n");
  remove_proc_entry("keyboard_driver", NULL);

  return;
}

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
    printk("in ioctl\n");
    switch(cmd)
    {
	    case IOCTL_TEST:
	    	((char*)arg)[0] = strbuf[0];
            if(block)
            {
	    	    ((char*)arg)[1] = '$';
            }
	    	printk("\nchar is : ");
		    //my_printk(strbuf);
	    	printk(strbuf);
		break;
	}
    return 0;
}

module_init(initialization_routine);
module_exit(cleanup_routine);

