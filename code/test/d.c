#include "syscall.h"

int
main()
{
    int i;

    for (i = 0; i < 30; i++) {
    	int fd = Open("yoyo");
    	if (i % 4 == 0)
   		Write("000000000000", 13, fd);
   	else if (i % 4 == 1)
   		Write("111111111111", 13, fd);
   	else if (i % 4 == 2)
   		Write("222222222222", 13, fd);
   	else
   		Write("333333333333", 13, fd);
    }
    Halt();
}