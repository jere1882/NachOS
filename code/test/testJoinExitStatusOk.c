#include "syscall.h"


int
main(void)
{
    char *arg = "hola";
    char **args[2] = {arg,0};

    Write("main is forking filetest2, which should return 0 \n",50,1);
    const SpaceId newProc = Exec("../test/filetest2",args);
    int ans = Join(newProc);
    if (ans==0){
        Write("Joined filetest2, exit status zero PASSED\n",43,1);
    }
    else {
        Write("Joined filetest2, exit status non-zero FAILED\n",47,1);
    }

    Halt();
}
