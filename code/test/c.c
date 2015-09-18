#include "syscall.h"

void a()
{
    int i = 0;
    while (i < 5)
        i++;
    Exit(i);
}
int
main()
{
    int j;
    j = 2;
    Fork(a);
    Fork(a);
    while (j < 10)
        j++;
    Yield();
    Exit(j);
}