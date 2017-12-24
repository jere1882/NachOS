#include "syscall.h"


int
main(void)
{
    Create("testShell.txt");
    OpenFileId o = Open("testShell.txt");
    Write("=)\n",3,o);
    Halt();
    // Not reached.
}
