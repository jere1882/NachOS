#include "syscall.h"


int
main(void)
{
    char *arg = "hola";
    char **args[2] = {arg,0};
    const SpaceId newProc = Exec("../test/filetest2",args);
     Write("About to fork filetest2\n",24,1);
    Join(newProc);
    Write("Joined filetest2, now I can rest in peace\n",42,1);
    Halt();
}
