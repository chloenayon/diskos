#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)
#define IOCTL_RD_CREAT _IOW(1, 6, struct rd_creat)
#define IOCTL_RD_MKDIR _IOW(2, 6, struct rd_mkdir)
#define IOCTL_RD_READ _IOR(3, 6, struct rd_read)

int main () {

  /* attribute structures */
  struct ioctl_test_t {
    int field1;
    char field2;
  } ioctl_test_t;

  struct rd_creat {
    char *pathname;
    short mode;
  } rd_creat;

  struct rd_mkdir {
    char *pathname;
  } rd_mkdir;

  struct rd_read {
    int fd;
    char *address;
    int num_bytes;
  } rd_read;

  int fd = open ("/proc/help_os", O_RDONLY);

  rd_creat.pathname = "/file.txt";
  rd_creat.mode = 0b11;

  ioctl (fd, IOCTL_RD_CREAT, &rd_creat);

  struct rd_creat newfile;
  newfile.pathname = "/helpOS.txt";
  newfile.mode = 0b11;

  ioctl(fd, IOCTL_RD_CREAT, &newfile);

  rd_mkdir.pathname= "/usr";
  ioctl(fd, IOCTL_RD_MKDIR, &rd_mkdir);

  struct rd_creat nfile;
  nfile.pathname = "/usr/foo.txt";
  nfile.mode = 0b11;

  ioctl(fd, IOCTL_RD_CREAT, &nfile);

  struct rd_creat err;
  err.pathname = "/usr/src/foo.txt";
  err.mode = 0b11;

  ioctl(fd, IOCTL_RD_CREAT, &err);

  return 0;
}