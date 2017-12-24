#include "syscall.h"


int
main(int argc, char** argv)
{
    Create("testShell2.txt");
    OpenFileId o = Open("testShell2.txt");
    Write(argv[1],4,o);
    Close(o);
    //Halt();
    // Not reached.
}
