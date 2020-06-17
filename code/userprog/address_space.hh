/// Data structures to keep track of executing user programs (address
/// spaces).
///
/// For now, we do not keep any information about address spaces.  The user
/// level CPU state is saved and restored in the thread executing the user
/// program (see `thread.hh`).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_USERPROG_ADDRSPACE__HH
#define NACHOS_USERPROG_ADDRSPACE__HH


#include "filesys/file_system.hh"
#include "machine/translation_entry.hh"
#include "bin/noff.h"

const unsigned USER_STACK_SIZE = 1024;  ///< Increase this as necessary!


class AddressSpace {
public:

    /// Create an address space, initializing it with the program stored in
    /// the file `executable`.
    ///
    /// * `executable` is the open file that corresponds to the program.
    AddressSpace(OpenFile *executable, const char * name);

    /// De-allocate an address space.
    ~AddressSpace();

    /// Initialization used when demand loading is disabled 
    void init_demand_loading();

    /// Initialization used when deman loading is enabled
    void init_non_demand_loading();

    /// Initialize user-level CPU registers, before jumping to user code.
    void InitRegisters();

    /// Save/restore address space-specific info on a context switch.
    void SaveState();
    void RestoreState();

    /// Handle a TLB miss
    void handleTLBMiss(unsigned vaddr);

    const char *m_name;

    int get_pid() {return m_pid;}

    /// Used when demand loading is enabled to read a single vpn from the executable and 
    /// copy it into a fresh frame
    void loadPage(unsigned vaddr);

#ifdef VMEM
    // Retrieve page 'vpn' from Swap and store it in frame 'frn'
    void SwapToMemory(unsigned vpn, int frn);

    // Send page vpn to swap
    void MemoryToSwap(unsigned vpn);
#endif

    /// Assume linear page table translation for now!
    TranslationEntry *pageTable;

private:

    int m_pid;

    /// Number of pages in the virtual address space.
    unsigned numPages;
    
#ifdef VMEM
        OpenFile *swap_file;
#endif

    // Demand loading
    NoffHeader noffH;
    OpenFile *executable;
    unsigned nCodePages;  // Number of pages used by text segment
    unsigned nDataPages;  // Number of pages used by initialised data segment

};


#endif
