#include "syscall.h"
int
main()
{
    int i;
    for (i = 0; i < 30; i++) {
        int fd = Open("yoyo");
        char tmp[13];
        Read(tmp, 13, fd);
    }
    Halt();
}