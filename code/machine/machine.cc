/// Routines for simulating the execution of user programs.
///
/// DO NOT CHANGE -- part of the machine emulation
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "machine.hh"
#include "threads/system.hh"


/// Textual names of the exceptions that can be generated by user program
/// execution, for debugging.
static const char *EXCEPTION_NAMES[] = {
    "no exception", "syscall", "page fault/no TLB entry", "page read only",
    "bus error", "address error", "overflow", "illegal instruction"
};

/// Check to be sure that the host really uses the format it says it does,
/// for storing the bytes of an integer.  Stop on error.
static void
CheckEndian()
{
    union checkIt {
        char     charWord[4];
        unsigned intWord;
    } check;

    check.charWord[0] = 1;
    check.charWord[1] = 2;
    check.charWord[2] = 3;
    check.charWord[3] = 4;

#ifdef HOST_IS_BIG_ENDIAN
    ASSERT(check.intWord == 0x01020304);
#else
    ASSERT(check.intWord == 0x04030201);
#endif
}

/// Initialize the simulation of user program execution.
///
/// * `debug` -- if true, drop into the debugger after each user instruction
///   is executed.
Machine::Machine(bool debug)
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        registers[i] = 0;

    mainMemory = new char[MEMORY_SIZE];
    for (unsigned i = 0; i < MEMORY_SIZE; i++)
          mainMemory[i] = 0;

#ifdef USE_TLB
    tlb = new TranslationEntry[TLB_SIZE];
    for (unsigned i = 0; i < TLB_SIZE; i++)
        tlb[i].valid = false;
    pageTable = NULL;
#else  // Use linear page table.
    tlb = NULL;
    pageTable = NULL;
#endif

    singleStep = debug;
    CheckEndian();
}

/// De-allocate the data structures used to simulate user program execution.
Machine::~Machine()
{
    delete [] mainMemory;
    if (tlb != NULL)
        delete [] tlb;
}

/// Transfer control to the Nachos kernel from user mode, because the user
/// program either invoked a system call, or some exception occured (such as
/// the address translation failed).
///
/// * `which` is the cause of the kernel trap
/// * `badVaddr` is the virtual address causing the trap, if appropriate.
void
Machine::RaiseException(ExceptionType which, unsigned badVAddr)
{
    DEBUG('m', "Exception: %s\n", EXCEPTION_NAMES[which]);

    //ASSERT(interrupt->getStatus() == USER_MODE);
    registers[BAD_VADDR_REG] = badVAddr;
    DelayedLoad(0, 0);  // Finish anything in progress.
    interrupt->setStatus(SYSTEM_MODE);
    ExceptionHandler(which);  // Interrupts are enabled at this point.
    interrupt->setStatus(USER_MODE);
}

const int *
Machine::GetRegisters() const
{
    return registers;
}

/// Fetch or write the contents of a user program register.
int
Machine::ReadRegister(unsigned num) const
{
    ASSERT(num < NUM_TOTAL_REGS);
    return registers[num];
}

void
Machine::WriteRegister(unsigned num, int value)
{
    ASSERT(num < NUM_TOTAL_REGS);
    //DEBUG('m', "WriteRegister %u, value %d\n", num, value);
    registers[num] = value;
}


void Machine::printtlb() {
    for (unsigned i = 0 ; i < TLB_SIZE ; i++){
        TranslationEntry entry =  tlb[i];
        printf( "TLB entry: vp %d, ph %d, valid %d, ro %d, dirty %d, use %d \n", entry.virtualPage, entry.physicalPage, entry.valid, entry.readOnly, entry.dirty, entry.use);
    }
}
