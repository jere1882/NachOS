#include "syscall.h"


int
main(void)
{
    char *arg = "hola";
    char **args[2] = {arg,0};

    Write("main is forking filetest2, which should return 0 \n",50,1);
    const SpaceId newProc = Exec("../test/testJoinExistStatusAux",args);
    int ans = Join(newProc);
    if (ans==-1){
        Write("Joined testJoinExistStatusAux, TEST PASSED\n",43,1);
    }
    else {
        Write("Joined testJoinExistStatusAux, TEST FAILED\n",43,1);
    }

    Halt();
}
