/// Routines to manage address spaces (executing user programs).
///
/// In order to run a user program, you must:
///
/// 1. Link with the `-N -T 0` option.
/// 2. Run `coff2noff` to convert the object file to Nachos format (Nachos
///    object code format is essentially just a simpler version of the UNIX
///    executable object code format).
/// 3. Load the NOFF file into the Nachos file system (if you have not
///    implemented the file system yet, you do not need to do this last
///    step).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"

#define DEMAND_LOADING

static unsigned global_pids = 0;

/// Do little endian to big endian conversion on the bytes in the object file
/// header, in case the file was generated on a little endian machine, and we
/// are re now running on a big endian machine.
static void
SwapHeader(NoffHeader *noffH)
{
    noffH->noffMagic              = WordToHost(noffH->noffMagic);
    noffH->code.size              = WordToHost(noffH->code.size);
    noffH->code.virtualAddr       = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr        = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size          = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr   = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr    = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size        = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr =
      WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr  = WordToHost(noffH->uninitData.inFileAddr);
}

/// Create an address space to run a user program.
///
/// Load the program from a file `executable`, and set everything up so that
/// we can start executing user instructions.
///
/// Assumes that the object code file is in NOFF format.
///
/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
///
/// * `executable` is the file containing the object code to load into
///   memory.
AddressSpace::AddressSpace(OpenFile *exe, const char *name){
	
    m_name = name;
    m_pid = global_pids++;
    executable = exe;


    executable->ReadAt((char *) &noffH, sizeof noffH, 0);
    if (noffH.noffMagic != NOFFMAGIC && WordToHost(noffH.noffMagic) == NOFFMAGIC)
        SwapHeader(&noffH);

    ASSERT(noffH.noffMagic == NOFFMAGIC);     

    unsigned size;          // Size in bytes of the whole program
    unsigned initSizeBytes; // Data segment size in bytes
    unsigned codeSizeBytes; // Code segment size in bytes
    unsigned unitSizeBytes; // Uninitialized data size in bytes
    unsigned numPagesZero;  // Uninitialized data size in pages

	nCodePages = divRoundUp(noffH.code.size, PAGE_SIZE);
	codeSizeBytes = nCodePages * PAGE_SIZE;

    nDataPages = divRoundUp(noffH.initData.size, PAGE_SIZE);
    initSizeBytes = nDataPages * PAGE_SIZE;

    unitSizeBytes = noffH.uninitData.size + USER_STACK_SIZE; 

    numPagesZero = divRoundUp(unitSizeBytes, PAGE_SIZE);
    unitSizeBytes = numPagesZero * PAGE_SIZE;

    size = initSizeBytes + codeSizeBytes + unitSizeBytes;
    numPages = nCodePages + nDataPages + numPagesZero;

    DEBUG('a', "Initializing address space, num pages %u, size %u\n", numPages, size);
    stats->numPages = numPages;
    stats->numFrames = NUM_PHYS_PAGES;

#ifndef VMEM
    ASSERT(numPages <= NUM_PHYS_PAGES && "Program doesn't fit in physical memory.");    
#else
    DEBUG('v', "Total size in pages is %d \n",numPages);
#endif

    // CREAR EL ARCHIVO DE SWAP
#ifdef VMEM
    char swapFilename[32];
    sprintf(swapFilename, "SWAP.%d", m_pid);
    ASSERT(fileSystem->Create(swapFilename, MEMORY_SIZE));
    swap_file = fileSystem->Open(swapFilename);
    DEBUG('v', "Creating swap file %s total size is %d\n",swapFilename,numPages*PAGE_SIZE);

    ASSERT(swap_file != NULL);

    for (unsigned i = 0; i < numPages; i++)
        for (unsigned j = 0; j < PAGE_SIZE; j++){
            swap_file->Write("0", 1);
        }
#endif

#ifdef DEMAND_LOADING
    init_demand_loading();
#else
    init_non_demand_loading();
#endif

}


void AddressSpace::init_demand_loading(){
    
    pageTable = new TranslationEntry[numPages];

    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;  // false means 'never loaded'
        pageTable[i].use          = false;
        pageTable[i].dirty        = true;
        pageTable[i].readOnly     = false;
    }

}
void AddressSpace::init_non_demand_loading(){

    pageTable = new TranslationEntry[numPages];

    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;

#ifndef VMEM
        int newPage = bitmap->Find();
        DEBUG('a', "Not enough space in physical memory to load the program\n");
        ASSERT(newPage>=0 && "physical memory is full!");  // newPage==-1 => memory is full.
#else
        int newPage = paginador->FindFreeFrame(this,i); 
#endif // VMEM
        pageTable[i].physicalPage = newPage;
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;

        memset(&(machine -> mainMemory [pageTable[i].physicalPage*PAGE_SIZE]),0,PAGE_SIZE);

    }

    // copy in the code and data segments into memory.
	char temp;
	int frame, off;
	
    // copiar el segmento de código
    if (noffH.code.size > 0) {     
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n", noffH.code.virtualAddr, noffH.code.size);
        for (int j=0; j<noffH.code.size; j++){
			ASSERT(executable->ReadAt(&temp,1,noffH.code.inFileAddr+j)==1);                     //Leemos un byte del archivo

            int vpn = (noffH.code.virtualAddr + j) / PAGE_SIZE;
			frame = pageTable[vpn].physicalPage; // Calculamos el frame
			off = (noffH.code.virtualAddr + j) % PAGE_SIZE;

#ifndef VMEM
            ASSERT(frame>=0)
#else
            DEBUG('v', "Filling code virtual page %d whose registered frame is %d \n",vpn,frame);
            if (frame < 0) {  // This vpn no longer has a frame associated in memory
                frame = paginador->FindFreeFrame(this,vpn);
                DEBUG('v', "Paginador le consiguio un frame nuevo, %d \n",frame);
                SwapToMemory(vpn,frame);
            }
#endif // VMEM
            pageTable[vpn].dirty = true;
			machine->mainMemory[frame * PAGE_SIZE +off] = temp;
        }                             
    }

    // copiar el segmento de datos initializados
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n", noffH.initData.virtualAddr, noffH.initData.size);
        
        for (int j=0; j<noffH.initData.size; j++){
			ASSERT(executable->ReadAt(&temp,1,noffH.initData.inFileAddr+j)==1);  //Leemos un byte del archivo
            int vpn = (noffH.initData.virtualAddr + j) / PAGE_SIZE;
			frame = pageTable[vpn].physicalPage; // Calculamos el frame
			off = (noffH.initData.virtualAddr + j) % PAGE_SIZE;

#ifndef VMEM
            ASSERT(frame>=0)
#else
            DEBUG('v', "Filling data virtual page %d whose registered frame is %d \n",vpn,frame);
            if (frame < 0) {  // This vpn no longer has a frame associated in memory :
                frame = paginador->FindFreeFrame(this,vpn);
                DEBUG('v', "Paginador le consiguio un frame nuevo, %d \n",frame);
                SwapToMemory(vpn,frame); // Solo copia ceros, pero acomoda todas las tablas.
            }
#endif //VMEM
            pageTable[vpn].dirty = true; // Set to true because it has not been copied to swap yet!
			machine->mainMemory[frame * PAGE_SIZE +off] = temp; // Acá le pone los valores reales.
        }     
        
    }
}


// Loads the page of a particular address to the binary
void
AddressSpace::loadPage(unsigned vaddr)
{

    DEBUG('a', "Loading page associated to vaddr 0x%X \n",vaddr);
    int virtualPage = vaddr / PAGE_SIZE;

#ifndef VMEM
    int frame = bitmap->Find();
    ASSERT(frame>=0 && "physical memory is full!");  // newPage==-1 => memory is full.
#else
    int frame = paginador->FindFreeFrame(this,virtualPage); 
#endif // VMEM

    ASSERT(frame >= 0);

    pageTable[virtualPage].physicalPage = frame; 

    DEBUG('a', "[Demand loading] vpn %d about to be loaded to frame %d\n",virtualPage,frame);
    //DEBUG('a', "[Prior loading] memory[frame] has value = %8.8x\n", machine->mainMemory[frame]);


    if (virtualPage < nCodePages){

        DEBUG('a', "[Demand loading] page was found in code segment \n");

        executable->ReadAt(
            &(machine->mainMemory[pageTable[virtualPage].physicalPage*PAGE_SIZE]),
                PAGE_SIZE, noffH.code.inFileAddr + virtualPage*PAGE_SIZE);
        //DEBUG('a', "[Post loading] memory[frame] has value = %8.8x\n", machine->mainMemory[frame]);

    }
    else if (virtualPage < nDataPages + nCodePages){
        DEBUG('a', "[Demand loading] page was found in initialised data segment \n");
        executable->ReadAt(
            &(machine->mainMemory[pageTable[virtualPage].physicalPage*PAGE_SIZE]),
        PAGE_SIZE, noffH.initData.inFileAddr + (virtualPage-nCodePages)*PAGE_SIZE);
        //  DEBUG('a', "[Post loading] memory[frame] has value = %8.8x\n", machine->mainMemory[frame]);
    } 
    else 
    {
        DEBUG('a', "[Demand loading] page was found in uninitialised data segment \n");
        memset(machine->mainMemory + (pageTable[virtualPage].physicalPage)*PAGE_SIZE, 0, PAGE_SIZE);
      //  DEBUG('a', "[Post loading] memory[frame] has value = %8.8x\n", machine->mainMemory[frame]);

    }
  pageTable[virtualPage].valid = true;  

}


/// Deallocate an address space.
AddressSpace::~AddressSpace(){

    DEBUG('a', "Deleting AddressSpace");
	unsigned i;
	for(i=0; i<numPages; i++) {
#ifndef VMEM
        bitmap->Clear(pageTable[i].physicalPage);
#else
        if (pageTable[i].physicalPage>=0 && pageTable[i].valid)
        {
            paginador->ReleaseFrame(pageTable[i].physicalPage);
        }
#endif
    }

    delete [] pageTable;

#ifdef VMEM
    delete swap_file;
#endif

    delete executable;

}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters(){
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void AddressSpace::SaveState()
{
#ifdef USE_TLB
    DEBUG('a', "AddressSpace::SaveState \n");
    for (unsigned i = 0 ; i < TLB_SIZE ; i++){
        if(machine->tlb[i].valid) {
            pageTable[machine->tlb[i].virtualPage] = machine->tlb[i];
        }
    }
#endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void AddressSpace::RestoreState()
{
    DEBUG('a', "AddressSpace::RestoreState \n");
#ifdef USE_TLB
    for (unsigned i = 0 ; i < TLB_SIZE ; i++)
        machine->tlb[i].valid = false;
#else
    machine->pageTable     = pageTable;
    machine->pageTableSize = numPages;
#endif
}

void AddressSpace::handleTLBMiss(unsigned vaddr) 
{

    unsigned int vpn = vaddr/PAGE_SIZE;  // Virtual page number

    DEBUG('a', "Handling TLB miss, looking for VA 0x%X \n",vaddr);

    // Make sure vpn is within bounds
    if (vpn <0 || vpn > numPages) {
        DEBUG('a', "Error: Requested vpn is not within bounds, numpages= %d \n", numPages);
        currentThread->Finish(1);
        return;
    }

    TranslationEntry& entry = pageTable[vpn];
    DEBUG('a', "pagetable[%d] renders an entry whose vpn is %d \n",vpn,entry.virtualPage);
    ASSERT(pageTable[vpn].virtualPage == vpn)


    // If it has never been loaded, do it
#ifdef DEMAND_LOADING
    if (!entry.valid){
        DEBUG('a', "[Demand loading] vpn is not loaded in memory \n",vpn,entry.virtualPage);
        loadPage(vaddr);
    }
#endif
    ASSERT(pageTable[vpn].valid)
    ASSERT(pageTable[vpn].virtualPage == vpn)

#ifdef VMEM
    // Bring it from swap, if necessary
    if (entry.physicalPage==-1){
        entry.physicalPage = paginador->FindFreeFrame(this,vpn);
        SwapToMemory(vpn,entry.physicalPage);
        DEBUG('v', "virtual page %d was restored to frame %d, set to valid? %d \n",vpn,entry.physicalPage,entry.valid);
    }
#endif
    ASSERT(pageTable[vpn].virtualPage == vpn)

    // Try to find an empty spot in the tlb
    for(unsigned i = 0; i < TLB_SIZE; i++){
        if(!machine->tlb[i].valid){
            DEBUG('a', "Filling empty TLB entry %d with vpage %d \n",i,vpn);
            machine->tlb[i] = entry;
            return;
        }
    }

    // If there is no empty spot, randomly choose one
    unsigned r = rand() % TLB_SIZE;
    // Update the page table (so as to save any changes in bits
    // We know it is valid, it would've returned in the previous if not
    pageTable[machine->tlb[r].virtualPage] = machine->tlb[r];

    ASSERT(pageTable[machine->tlb[r].virtualPage].virtualPage == machine->tlb[r].virtualPage)

    // Update the TLB
    machine->tlb[r] = entry;
    DEBUG('a', "Overwriting TLB entry %d with vpage %d pointing at frame %d\n",r,entry.virtualPage,entry.physicalPage);

}

#ifdef VMEM

void AddressSpace::SwapToMemory(unsigned vpn, int physicalPage)
{
    ASSERT(0 <= physicalPage && physicalPage < (int)NUM_PHYS_PAGES);
    ASSERT( vpn < numPages);
    
    pageTable[vpn].physicalPage = physicalPage;

    int ret = swap_file->ReadAt(&(machine->mainMemory[physicalPage * PAGE_SIZE]), PAGE_SIZE, vpn*PAGE_SIZE);
    ASSERT(ret == PAGE_SIZE);
    DEBUG('v',"Retrieving vpn %d into frame %d, addrspaceid %d from swap\n", vpn, physicalPage,m_pid);

    //DEBUG('v', "Memory retrieved looks like: %d \n",machine->mainMemory[0]);


    pageTable[vpn].dirty = false;
    pageTable[vpn].valid = true;
    pageTable[vpn].use   = true;

    stats->swaps_in++;

    ASSERT(pageTable[vpn].virtualPage == vpn)

}

void AddressSpace::MemoryToSwap(unsigned vpn)
{
    ASSERT(vpn < numPages);

    int physicalPage = pageTable[vpn].physicalPage;
    ASSERT(0 <= physicalPage && physicalPage < (int)NUM_PHYS_PAGES);

    DEBUG('v',"[MemoryToSwap] About to send vpn %d, located in frame %d, addrspaceid %d to swap\n", vpn, physicalPage,m_pid);
    // Invalidar la entrada de la TLB si corresponde
#ifdef USE_TLB
    if(currentThread->space == this){
        for(unsigned i=0; i<TLB_SIZE; i++) {
            if((machine->tlb[i].virtualPage == vpn) && machine->tlb[i].valid) {
                DEBUG('v',"There is a TLB entry for vpn %d. Copy back to pagetable & invalidate\n",machine->tlb[i].virtualPage); 
                pageTable[vpn] = machine->tlb[i];
                machine->tlb[i].valid = false;
            }
        }
    }
#endif   

    if(pageTable[vpn].dirty){ 
        DEBUG('v',"[MemoryToSwap] vpn was dirty so we're actually copying it\n");
        int ret =  swap_file->WriteAt(&(machine->mainMemory[physicalPage * PAGE_SIZE]), PAGE_SIZE, vpn*PAGE_SIZE);
        ASSERT(ret == PAGE_SIZE);
    }
   
    pageTable[vpn].physicalPage = -1;
    stats->swaps_out++;

    ASSERT(pageTable[vpn].virtualPage == vpn)

}

#endif