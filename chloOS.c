/*
 *  ioctl test module -- Rich West.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");

/* attribute structures */
struct ioctl_test_t {
  int field1;
  char field2;
};

// 64 bytes
struct myInode {
    short type;
    int size;//how much space is being used
    long location;
    long overflow;//location for next inode
    short permissions;
    char padding[44];
};

struct rd_creat {
    char *pathname;
    short mode;
};

// Entries in a database file; 14-byte filename and 2-byte inode number
// 16 bytes
struct dir_entry { 
    char fname[14];
    short inode;
};

/* protocols, global vars, etc */

#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)
#define IOCTL_RD_CREAT _IOW(1, 6, struct rd_creat)

#define MEM_SIZE 2000000 //2mb
#define BLOCK_SIZE 256

char** baseAddress;

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);

static struct file_operations pseudo_dev_proc_operations;

static struct proc_dir_entry *proc_entry;


/** STRING & HELPER FUNCTIONS **/

void filesystem(){
  struct myInode *inodeList = (baseAddress+64);
  struct myInode rootNode = inodeList[0];
  struct dir_entry *entries = baseAddress + (rootNode.location * 64);
  int inodes = (int)(baseAddress[1]);

  printk("num inodes is: %d\n", inodes);
  
  int x;
  for (x = 0; x < inodes; x++){
    struct myInode inode = inodeList[x];
    printk("Inode %d: Type is %d; Size is %d, Location is %d\n", x, inode.type, inode.size, inode.location);
  }

  int count = rootNode.size / 16;
  int i;

  for (i = 0; i < count; i++){
    struct dir_entry e = entries[i];
    printk("/%s\n", e.fname);
  }
  /*
  inodeList[0] 
  
  printk("A\n");
  // calculate beginning of filespace, add number of used blocks to get next free
  int blocknum = BLOCK_SIZE + 4 + (int)(baseAddress[1]);
  printk("B\n");
  struct myInode newNode = {0, 0, blocknum, 0, mode};
  inodeList[(int)(baseAddress[1])] = newNode;
  struct dir_entry entry = {fname, (int)(baseAddress[1])};
  printk("C\n");
  // increment inode count, decrease free # of inodes
  baseAddress[1] = (int)(baseAddress[1]) + 1; 
  printk("D\n");
  baseAddress[0] = (int)(baseAddress[0]) - 1;
  printk("E\n");
  */
 return;
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

int my_strtok(char *string, char *delim, char *dname, int index){

  if (string[0] != '/'){
    my_printk("Error! Invalid pathname, does not begin with '/'\n");
    return -1;
  }

  int i;
  int x = 0;
  char c;

  // find first split
  for (i = index; string[i] != '\0' && string[i] != *delim; i++){
    
      c = string[i];
      dname[x] = c;
      x++;
  }

  // terminate the return string
  dname[x] = '\0';

  if (string[i] == '\0'){
    return -1;
  }

  return i + 1;
}

/** INIT FUNCTION **/

static int __init initialization_routine(void) {
  printk("<1> Loading module\n");

  baseAddress = vmalloc(MEM_SIZE);
  baseAddress[0]=7811; //free blocks
  baseAddress[1]= 0;//inode index

  // 256 bytes ahead of baseAddress, a.k.a. inode list
  struct myInode *inodeList = (baseAddress+64);  
  //struct myInode *inodeList = (baseAddress+2);  
  //struct myInode rootAddress = {0,0,BLOCK_SIZE,0,0b11};
  struct myInode rootAddress = {0,0,BLOCK_SIZE + 5,0,0b11};
  inodeList[(int)(baseAddress[1])] = rootAddress;
  baseAddress[1] = 1;
  baseAddress[0] = (int)(baseAddress[1]) - 1;

  pseudo_dev_proc_operations.ioctl = pseudo_device_ioctl;

  /* Start create proc entry */
  proc_entry = create_proc_entry("help_os", 0444, NULL);
  if(!proc_entry)
  {
    printk("<1> Error creating /proc entry.\n");
    return 1;
  }

  //proc_entry->owner = THIS_MODULE; <-- This is now deprecated
  proc_entry->proc_fops = &pseudo_dev_proc_operations;

  filesystem();

  return 0;
}

static void __exit cleanup_routine(void) {

  printk("<1> Dumping module\n");
  remove_proc_entry("help_os", NULL);

  return;
}

/* FILE OPERATION FUNCTIONS */

void do_ioctl_rd_creat(char *fname, short mode){

  // get root node 
  struct myInode *currNode = baseAddress+64; 

  // iterate over filesystem
  char *pname = fname;
  char *d = "/";
  char *out = vmalloc(sizeof(pname));
  int index = 1;

  while (index != -1){
    index = my_strtok(pname, d, out, index);
    if (index == -1){
     // we are in the directory that the file needs to be inserted in
      break;
    } else {
      // we are in a superdirectory and must navigate down the filesystem
      /*
      for (){
        currNode = ;
      }*/
    }
    printk("dir is: %s\n", out);
  }
  
  printk("final file is %s\n", out);

  // create dir entry in the directory, insert inode in inode list, decrement available blocks, increase inode index, set block bitmap
  struct myInode *inodeList = (baseAddress+64);
  struct myInode rootNode = inodeList[0];

  // math not great here
  struct dir_entry *newEntry = baseAddress + (rootNode.location * 64) + rootNode.size;
  printk("A\n");
  // calculate beginning of filespace, add number of used blocks to get next free
  int blocknum = BLOCK_SIZE + 5 + (int)(baseAddress[1]);
  printk("B\n");
  struct myInode newNode = {0, 0, blocknum, 0, mode};
  inodeList[(int)(baseAddress[1])] = newNode;
  struct dir_entry e = {fname, (int)(baseAddress[1])};
  rootNode.size += 16;
  newEntry[0] = e;
  printk("C\n");
  // increment inode count, decrease free # of inodes
  baseAddress[1] = (int)(baseAddress[1]) + 1; 
  printk("D\n");
  baseAddress[0] = (int)(baseAddress[0]) - 1;
  printk("E\n");


  return;
}

/* IOCTL ENTRY POINT */
static int pseudo_device_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg)
{
  struct ioctl_test_t ioc;
  struct rd_creat rc;
  
  switch (cmd){

  case IOCTL_TEST:
  
    copy_from_user(&ioc, (struct ioctl_test_t *)arg, 
		   sizeof(struct ioctl_test_t));
    printk("<1> ioctl: call to IOCTL_TEST (%d,%c)!\n", 
	   ioc.field1, ioc.field2);
     
    my_printk ("Got msg in kernel\n");

    break;

  case IOCTL_RD_CREAT:

    my_printk("HERE\n");

    copy_from_user(&rc, (struct rd_creat *)arg, 
        sizeof(struct rd_creat));
    printk("<1> CALL TO IOCTL!! Pathname is %s\n", rc.pathname);

    do_ioctl_rd_creat(rc.pathname, rc.mode);

    /*
    printk("size of inode is: %d\n", sizeof(struct myInode));
    printk("size of dir_entry is: %d\n", sizeof(struct dir_entry));
    */

    my_printk ("Got msg in kernel\n");

    filesystem();

    break;
  
  default:
    return -EINVAL;
    break;
  }
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 