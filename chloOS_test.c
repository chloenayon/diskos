#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)
#define IOCTL_RD_CREAT _IOW(1, 6, struct rd_creat)

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

  int fd = open ("/proc/help_os", O_RDONLY);

  rd_creat.pathname = "/file.txt";
  rd_creat.mode = 0b11;

  short x = 2;
  char c[14] = "hello world";

  printf("size of string is %d\n", sizeof(c));

  ioctl (fd, IOCTL_RD_CREAT, &rd_creat);

  return 0;
}