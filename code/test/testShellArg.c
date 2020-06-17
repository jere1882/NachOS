#include "syscall.h"

int
main(int argc, char** argv)
{
    Create("testShell2.txt");
    OpenFileId o = Open("testShell2.txt");
    Write(argv[1],5,o);
    Close(o);
    //Halt();
}