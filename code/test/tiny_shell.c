#include "syscall.h"

/* Example: Run from userprog, ./nachos -x ../test/tiny_shell
   then enter the path to a nachos program such as  ../test/halt
   it'll execute it, but cannot pass parameters  */

int
main(void)
{
    SpaceId    newProc;
    OpenFileId input  = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char       prompt[2], ch, buffer[60];
    int        i;

    prompt[0] = '-';
    prompt[1] = '-';

    while (1)
    {
        Write(prompt, 2, output);
        i = 0;
        do
            Read(&buffer[i], 1, input);
        while (buffer[i++] != '\n');

        buffer[--i] = '\0';

        if (i > 0) {
            char *arg = "tiny_shell";
            char **args[2] = {arg,0};
            newProc = Exec(buffer,args);
            Join(newProc);
        }
    }
}




