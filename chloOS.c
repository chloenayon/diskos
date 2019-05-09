/*
 *  DISCOS - Linux Filesystem Kernel Module
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
    char *fname[14];
    short inode;
};

struct rd_mkdir {
    char *pathname;
} rd_mkdir;

struct rd_read {
    int fd;
    char *address;
    int num_bytes;
} rd_read;

/* protocols, global vars, etc */

#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)
#define IOCTL_RD_CREAT _IOW(1, 6, struct rd_creat)
#define IOCTL_RD_MKDIR _IOW(2, 6, struct rd_mkdir)
#define IOCTL_RD_READ _IOR(3, 6, struct rd_read)

#define MEM_SIZE 2000000 //2mb
#define BLOCK_SIZE 256

int** baseAddress;

static int pseudo_device_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg);

static struct file_operations pseudo_dev_proc_operations;

static struct proc_dir_entry *proc_entry;


/** STRING & HELPER FUNCTIONS **/

/* print information about the inodes, block contents, and files */
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

    struct dir_entry *entries = baseAddress + (inode.location * 64);

    int n;
    int index = 0;
    for (n = 0; n < inode.size; n += 16){
      struct dir_entry e = entries[index];
      printk("filename is: %s\n", e.fname);
      index++;
    }
  }
 return;
}

void set_bitmap(int block){
  char *bitmap = baseAddress + (257 * 64);
  bitmap[block] = 0xA | bitmap[block];
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

/* modified strtok */
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

/* modified strcpy */

void my_strcpy(char *src, char *dest){

  int i;
  for (i = 0; src[i] != '\0'; i++){
    dest[i] = src[i];
  }
  dest[i] = '\0';

  return;
}

// 0 if they are the same, -1 if not

int my_strcmp(char *str1, char *str2){

    int i;
    for (i = 0; (str1[i] != '\0') && (str2[i] != '\0'); i++){
      if (str1[i] != str2[i]){
        return -1;
      }
    }

    if (str1[i] != str2[i]){
      return -1;
    }
  return 0;
}

/** INIT FUNCTION **/

static int __init initialization_routine(void) {
  printk("<1> Loading module\n");

  baseAddress = vmalloc(MEM_SIZE);
  baseAddress[0]=7811; //free blocks
  baseAddress[1]= 0;//inode index

  // 256 bytes ahead of baseAddress, a.k.a. inode list
  struct myInode *inodeList = (baseAddress+64);  
  struct myInode rootAddress = {0,0,BLOCK_SIZE + 5,0,0b11};
  set_bitmap(0);
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

  return 0;
}

static void __exit cleanup_routine(void) {

  printk("<1> Dumping module\n");
  remove_proc_entry("help_os", NULL);

  return;
}

/* FILE OPERATION FUNCTIONS */

// -1: doesn't exist ; otherwise: corresponding i-node
int find_inode(struct myInode currNode, char *filename){
  struct dir_entry *entries = baseAddress + (currNode.location * 64);
  int n;
  int index = 0;
  for (n = 0; n < currNode.size; n += 16){
    struct dir_entry e = entries[index];
    printk("COMPARE: filename is: %s and out is: %s\n", e.fname, filename);
    if (my_strcmp(e.fname, filename) == 0){
      return e.inode;
    }
    index++;
  }
  return -1;
}

int do_ioctl_rd_creat(char *fname, short mode){

  printk("Attempting to create file %s\n", fname);

  // get root node 
  struct myInode *inodeList = (baseAddress+64);
  struct myInode currNode = inodeList[0];
  // inode number of destination inode
  int dest = 0;

  // iterate over filesystem
  char *pname = fname;
  char *d = "/";
  // NO vmalloc
  char *out = vmalloc(sizeof(pname));
  int index = 1;

  while (index != -1){
    index = my_strtok(pname, d, out, index);
    printk("INSERTING FNAME %s: ON %s with index %d\n", fname, out, index);
    if (index == -1){
      if (find_inode(currNode, out) != -1){
        my_printk("Error: File already exists!\n");
        return -1;
      }
      break;
    } else {
      dest = find_inode(currNode, out);
      if (dest == -1){
        my_printk("Error: Directory does not exist!\n");
        return -1;
      } else {
        currNode = inodeList[dest];
      }
    }
    printk("dir is: %s\n", out);
  }
  
  printk("final file is %s\n", out);

  // math not great here
  struct dir_entry *entries = baseAddress + (currNode.location * 64);

  int blocknum = BLOCK_SIZE + 5 + (int)(baseAddress[1]);
  struct myInode newNode = {1, 0, blocknum, 0, mode};
  set_bitmap(blocknum);
  inodeList[(int)(baseAddress[1])] = newNode;

  struct dir_entry e;
  my_strcpy(out, e.fname);
  e.inode = (int)(baseAddress[1]);
  entries[(currNode.size/16)] = e;

  inodeList[dest].size = inodeList[dest].size + 16;
  baseAddress[1] = (int)(baseAddress[1]) + 1; 
  baseAddress[0] = (int)(baseAddress[0]) - 1;

  printk("Successfully added %s\n", fname);

  filesystem();

  return 0;
}

int do_ioctl_rd_mkdir(char *fname){

   printk("Attempting to make directory %s\n", fname);

  // get root node 
  struct myInode *inodeList = (baseAddress+64);
  struct myInode currNode = inodeList[0];
  // inode number of destination inode
  int dest = 0;

  // iterate over filesystem
  char *pname = fname;
  char *d = "/";
  // NO vmalloc
  char *out = vmalloc(sizeof(pname));
  int index = 1;

  while (index != -1){
    index = my_strtok(pname, d, out, index);
    if (index == -1){
      if (find_inode(currNode, out) != -1){
        my_printk("Error: Directory already exists!\n");
        return -1;
      }
      break;
    } else {
      dest = find_inode(currNode, out);
      if (dest == -1){
        my_printk("Error: Directory does not exist!\n");
        return -1;
      } else {
        currNode = inodeList[dest];
      }
    }
    //printk("dir is: %s\n", out);
  }
  
  //printk("final file is %s\n", out);

  struct dir_entry *entries = baseAddress + (currNode.location * 64);
  int blocknum = BLOCK_SIZE + 5 + (int)(baseAddress[1]);
  struct myInode newNode = {0, 0, blocknum, 0, 0b11};
  set_bitmap(0);
  inodeList[(int)(baseAddress[1])] = newNode;

  struct dir_entry e;
  my_strcpy(out, e.fname);
  e.inode = (int)(baseAddress[1]);
  entries[(currNode.size/16)] = e;

  inodeList[dest].size = inodeList[dest].size + 16;
  baseAddress[1] = (int)(baseAddress[1]) + 1; 
  baseAddress[0] = (int)(baseAddress[0]) - 1;

  printk("Successfully created directory %s\n", fname);

  filesystem();

  return 0;
}

/* IOCTL ENTRY POINT */
static int pseudo_device_ioctl(struct inode *inode, struct file *file,
			       unsigned int cmd, unsigned long arg)
{
  struct ioctl_test_t ioc;
  struct rd_creat rc;
  struct rd_mkdir rmk;
  
  switch (cmd){

  case IOCTL_TEST:
  
    copy_from_user(&ioc, (struct ioctl_test_t *)arg, 
		   sizeof(struct ioctl_test_t));
    printk("<1> ioctl: call to IOCTL_TEST (%d,%c)!\n", 
	   ioc.field1, ioc.field2);
     
    my_printk ("Got msg in kernel\n");

    return 0;

  case IOCTL_RD_CREAT:
  
    copy_from_user(&rc, (struct rd_creat *)arg, 
        sizeof(struct rd_creat));
    printk("<1> CALL TO IOCTL!! Pathname is %s\n", rc.pathname);

    return do_ioctl_rd_creat(rc.pathname, rc.mode);

  case IOCTL_RD_MKDIR:

    copy_from_user(&rmk, (struct rd_mkdir *)arg, 
        sizeof(struct rd_mkdir));
    printk("<1> CALL TO IOCTL!! Pathname is %s\n", rmk.pathname);

    return do_ioctl_rd_mkdir(rmk.pathname);
    return 0;
  default:
    return -EINVAL;
    break;
  }
  
  return 0;
}

module_init(initialization_routine); 
module_exit(cleanup_routine); 