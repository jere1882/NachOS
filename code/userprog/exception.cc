/// Entry point into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "syscall.h"
#include "threads/system.hh"

#define MAXNAMELENGTH 256   // Length limit for file names

#define SYSC_OK      0      // Succesful signal execution return vale 
#define SYSC_ERROR  -1      // Error signal execution return value

/// Entry point into the Nachos kernel.  Called when a user program is
/// executing, and either does a syscall, or generates an addressing or
/// arithmetic exception.
///
/// For system calls, the following is the calling convention:
///
/// * system call code in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the pc before returning. (Or else you will
/// loop making the same system call forever!)
///
/// * `which` is the kind of exception.  The list of possible exceptions is
///   in `machine.hh`.



void ReadStringFromUser (int userAddress, char *outString, unsigned maxByteCount) {
    int buf;
    unsigned i;
    for (i = 0; i<maxByteCount ; i++) { 
        if (machine->ReadMem(userAddress+i, 1, &buf)) 
            break;
        else if (buf=='\0') {
            outString[i] = buf;
            break;
        } 
        else
            outString[i] = buf;
    }
}

void ReadBufferFromUser (int userAddress, char *outBuffer, unsigned byteCount) {
    unsigned i;
    int buf;
    for (i = 0; i<byteCount ; i++) {
        if (machine->ReadMem(userAddress+i, 1, &buf)) 
            break;
        else
            outBuffer[i] = buf;
    }
}

void WriteStringToUser (const char *string, int userAddress){
    int i;    
    for (i=0 ; string[i]!='\0' ; i++){
       if (machine->WriteMem(userAddress+i, 1, string[i]))
           break;
    }   
 
    machine->WriteMem(userAddress+i, 1, '\0');
}


void WriteBufferToUser (const char *buffer, int userAddress, unsigned byteCount){
    unsigned i;    
    for (i=0 ; i< byteCount ; i++){
       if (machine->WriteMem(userAddress+i, 1, buffer[i]))
           break;
    }    
}


void incPC() {
    int programCounter = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, programCounter); 
    programCounter = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, programCounter);
    machine->WriteRegister(NEXT_PC_REG, programCounter+4);    
}


void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SYSCALL_EXCEPTION ) {
        switch(type) {
            case SC_Halt: {
                DEBUG('a', "Shutdown, initiated by user program.\n");
                interrupt->Halt();
                break;
            }
            case SC_Create: {
                DEBUG('a', "Syscall Create\n");
                char buf[MAXNAMELENGTH];
                int dname = machine->ReadRegister(4);
                ReadStringFromUser(dname, buf, MAXNAMELENGTH);
                if (fileSystem->Create(buf, MAXNAMELENGTH)) 
                    machine->WriteRegister(2, SYSC_OK); // archivo creado exitosamente
                else 
                    machine->WriteRegister(2, SYSC_ERROR);
                incPC();
                break;
            }
        }
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(false);
    }
}