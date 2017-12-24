/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"


int
main(void)
{
    Create("testJoin.txt");
    OpenFileId o = Open("testJoin.txt");
    Write("Probando si anda join!\n",23,o);
    char *arg = "hola";
    char **args[2] = {arg,0};
    //Write(args[0],4,o);
    const SpaceId newProc = Exec("../test/filetest2",args);
    Write("Ya hice exec...\n",16,o);
    Join(newProc);
    Write("Ya volvio \n",12,o);
    Close(o);
    Halt();
    // Not reached.
}
