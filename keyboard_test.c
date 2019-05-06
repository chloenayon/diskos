#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <sys/ioctl.h>		/* ioctl */

#define IOCTL_TEST _IOR(0,6,char*)

int main()
{
   int file_desc;
   char message[10] = "\0";
   file_desc = open("/proc/keyboard_driver", O_RDONLY);
   while(1)
   {
   sleep(1);
   ioctl(file_desc,IOCTL_TEST,message);
   if(message[1]!='$')
   {
   printf("%c",message[0]);
   }
   }
}
