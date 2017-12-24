#include "syscall.h"


int
main(void)
{
   
    char *arg = "hola";
    char **args[2] = {arg,0};
    const SpaceId newProc = Exec("../test/filetest2",args);
    Join(newProc);
    Halt();
}
