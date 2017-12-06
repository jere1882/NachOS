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
        if (!machine->ReadMem(userAddress+i, 1, &buf)) 
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
        if (!machine->ReadMem(userAddress+i, 1, &buf)) 
            break;
        else
            outBuffer[i] = buf;
    }
}

void WriteStringToUser (const char *string, int userAddress){
    int i;    
    for (i=0 ; string[i]!='\0' ; i++){
       if (!machine->WriteMem(userAddress+i, 1, string[i]))
           break;
    }   
 
    machine->WriteMem(userAddress+i, 1, '\0');
}


void WriteBufferToUser (const char *buffer, int userAddress, unsigned byteCount){
    unsigned i;    
    for (i=0 ; i< byteCount ; i++){
       if (!machine->WriteMem(userAddress+i, 1, buffer[i]))
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
                DEBUG('a', "Creating File..\n");
                ReadStringFromUser(dname, buf, MAXNAMELENGTH);
                DEBUG('a', "Creating File: %s\n",buf);                
                if (fileSystem->Create(buf, MAXNAMELENGTH)) 
                    machine->WriteRegister(2, SYSC_OK); // archivo creado exitosamente
                else 
                    machine->WriteRegister(2, SYSC_ERROR);
                incPC();
                break;
           }
           case SC_Read: { //int Read(char *buffer, int size, OpenFileId id);
                DEBUG('a', "Syscall Read\n");
                int dbuf = machine->ReadRegister(4);
                int size = machine->ReadRegister(5);
                OpenFileId fd   = machine->ReadRegister(6); 
                char *buf = new char[size];
           
                int read=size; //cantidad efectivamente leída
            
                if (size < 0){
                    machine->WriteRegister(2, SYSC_ERROR);
                    DEBUG('a', "Syscall Read: Error. Negative size of buffer.\n");
                }
                else if (fd == ConsoleOutput){ 
                    machine->WriteRegister(2, SYSC_ERROR);
                    DEBUG('a', "Syscall Read: Error. Attempting to read from screen\n");
                }
                else if (fd < 0) {
                    machine->WriteRegister(2, SYSC_ERROR);
                    DEBUG('a', "Syscall Read: Error. Illegal file descriptor.\n");
                }
                else {        
                    if (fd == ConsoleInput) {
                       DEBUG('a', "Syscall Read: Reading from keyboard.\n");
                        int i;
                        for(i=0; i < size; i++)
                            buf[i] = synchconsole->GetChar();
                         machine->WriteRegister(2, size);
                    }
                    else {
                        OpenFile *f = currentThread->GetFile(fd);
                        if (f != NULL){
                            DEBUG('a', "Syscall Read: Reading from open file.\n");
                            read = f->Read(buf, size);    
                            machine->WriteRegister(2, read);
                        }
                        else {
                            machine->WriteRegister(2, SYSC_ERROR);
                            DEBUG('a', "Syscall Read: Error. File couldn't be opened.\n");
                        }                                
                    }
                    WriteBufferToUser(buf, dbuf, read);
                }
                incPC(); 
                delete [] buf;
                break;
           }     
            case SC_Write: { //void Write(char *buffer, int size, OpenFileId id);
                DEBUG('a', "Syscall Write\n");
                int dbuf = machine->ReadRegister(4);
                int size = machine->ReadRegister(5);
                OpenFileId fd   = machine->ReadRegister(6); 
                char *buf = new char[size];
            
                if (size < 0){
                    machine->WriteRegister(2, SYSC_ERROR);
                    DEBUG('a', "Syscall Write: Error. Attempting to write a negative amount of chars.\n");
                }
                else if (fd == ConsoleInput) { // Evitamos que se escriba en "teclado"
                    machine->WriteRegister(2, SYSC_ERROR);
                    DEBUG('a', "Syscall Write: Error. Attempting to write in keyboard.\n");
                }
                else if (fd < 0){
                    machine->WriteRegister(2, SYSC_ERROR);
                    DEBUG('a', "Syscall Write: Error. Invalid file descriptor.\n");
                }     
                else {
                    ReadBufferFromUser(dbuf, buf, size);   
                    if(fd == ConsoleOutput) {
                       DEBUG('a', "Syscall Write: Writing to screen.\n");
                        int i;
                       for(i=0; i < size; i++)
                            synchconsole->PutChar(buf[i]);
                       machine->WriteRegister(2, size);
                    }
                    else {
                        OpenFile *f = currentThread->GetFile(fd);
                        if (f!=NULL) {
                            int sizew = f->Write(buf, size);
                            machine->WriteRegister(2, sizew);
                            DEBUG('a', "Syscall Write: %d chars written to file.\n",sizew);
                        }
                        else { 
                            machine->WriteRegister(2, SYSC_ERROR);
                            DEBUG('a', "Syscall Write: Error. File couldn't be opened.\n");                       
                        }
                    }
                }
            delete buf;
            incPC();
            break;    
        }
        case SC_Open: {  //OpenFileId Open(char *name);
            DEBUG('a', "Syscall Open\n");
            char name[MAXNAMELENGTH];
            int dname = machine->ReadRegister(4);
            ReadStringFromUser(dname, name, MAXNAMELENGTH);
            DEBUG('a', "Syscall Open. File name is %s\n",name);
           
            OpenFile *f = fileSystem->Open(name);
            if (f == NULL){
                DEBUG('a', "Syscall Open: Error. Couldn't open file %s\n",name);
                machine->WriteRegister(2, SYSC_ERROR);               
            } 
            else {
                OpenFileId fd = currentThread -> AddFile(f);
                if (fd >= 0) {
                    DEBUG('a', "Syscall Open: File opened. File descriptor assigned is %d \n",fd);
                    machine->WriteRegister(2,fd);
                }
                else {
                    DEBUG('a', "Syscall Open: Error. Problem getting the file descriptor\n",name);
                    machine->WriteRegister(2,SYSC_ERROR);
                } 
            }
               
            incPC();
            break;   
        }
        case SC_Close: { //void Close(OpenFileId id);
            DEBUG('a', "Syscall Close\n");
            OpenFileId fd = machine->ReadRegister(4);
            OpenFile *f = currentThread->GetFile(fd);
            if (f) {
                DEBUG('a', "Syscall Close: Closing file\n");
                delete f;
                currentThread->CloseFile(fd);
                machine->WriteRegister(2, SYSC_OK);                
            }
            else{
                machine->WriteRegister(2, SYSC_ERROR);            
                DEBUG('a', "Syscall Close: Error getting file.\n");
            } 
            incPC();
            break;                  
        }
        case SC_Exit:{ // void Exit(int status)
            DEBUG('a',"Syscall Exit");
            int status = machine->ReadRegister(4);
            currentThread->Finish(status);
            //incPC();
            break;
        }
      }           
    }    
    else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(false);
    }
}
