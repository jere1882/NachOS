/// Routines for synchronizing threads.
///
/// Three kinds of synchronization routines are defined here: semaphores,
/// locks and condition variables (the implementation of the last two are
/// left to the reader).
///
/// Any implementation of a synchronization routine needs some primitive
/// atomic operation.  We assume Nachos is running on a uniprocessor, and
/// thus atomicity can be provided by turning off interrupts.  While
/// interrupts are disabled, no context switch can occur, and thus the
/// current thread is guaranteed to hold the CPU throughout, until interrupts
/// are reenabled.
///
/// Because some of these routines might be called with interrupts already
/// disabled (`Semaphore::V` for one), instead of turning on interrupts at
/// the end of the atomic operation, we always simply re-set the interrupt
/// state back to its original value (whether that be disabled or enabled).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "synch.hh"
#include "system.hh"


/// Initialize a semaphore, so that it can be used for synchronization.
///
/// * `debugName` is an arbitrary name, useful for debugging.
/// * `initialValue` is the initial value of the semaphore.
Semaphore::Semaphore(const char *debugName, int initialValue)
{
    name  = debugName;
    value = initialValue;
    queue = new List<Thread *>;
    DEBUG('s',"Semaphore %s constructed with initial value %d\n",currentThread->getName(), name,initialValue);

}

/// De-allocate semaphore, when no longer needed.
///
/// Assume no one is still waiting on the semaphore!
Semaphore::~Semaphore()
{
    delete queue;
}

/// Wait until semaphore `value > 0`, then decrement.
///
/// Checking the value and decrementing must be done atomically, so we need
/// to disable interrupts before checking the value.
///
/// Note that `Thread::Sleep` assumes that interrupts are disabled when it is
/// called.
void
Semaphore::P()
{
    DEBUG('s',"%s does semaphore %s P\n", currentThread->getName(),name);
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);
      // Disable interrupts.

    while (value == 0) {  // Semaphore not available.
        queue->Append(currentThread);  // So go to sleep.
        currentThread->Sleep();
    }
    value--;  // Semaphore available, consume its value.

    interrupt->SetLevel(oldLevel);  // Re-enable interrupts.
}

/// Increment semaphore value, waking up a waiter if necessary.
///
/// As with `P`, this operation must be atomic, so we need to disable
/// interrupts.  `Scheduler::ReadyToRun` assumes that threads are disabled
/// when it is called.
void
Semaphore::V()
{
    DEBUG('s',"%s does Semaphore %s V\n",currentThread->getName(), name);

    Thread   *thread;
    IntStatus oldLevel = interrupt->SetLevel(INT_OFF);

    thread = queue->Remove();
    if (thread != NULL)  // Make thread ready, consuming the `V` immediately.
        scheduler->ReadyToRun(thread);
    value++;
    interrupt->SetLevel(oldLevel);
}

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Lock::Lock(const char *debugName) {
    sem = new Semaphore(debugName, 1);
    name = debugName;
    heldBy = NULL;    
}

Lock::~Lock() {
    ASSERT (heldBy == NULL);
    delete sem;
}

void

Lock::Acquire() {
    DEBUG('s',"Thread %s tries to acquire the lock %s\n",currentThread->getName(),name);
    ASSERT(!isHeldByCurrentThread());
    if (heldBy!=NULL) {
        DEBUG('s',"The lock has been already acquired by %s. Comparing priorities: ¿owner: %d < %d :current ? \n",heldBy->getName(), heldBy->GetEffectivePriority() , currentThread->GetEffectivePriority());
        if (heldBy->GetEffectivePriority() < currentThread->GetEffectivePriority() ) {
            DEBUG('s',"Upgrading owner's priority\n");
            heldBy->ChangePriority(currentThread->GetEffectivePriority());
            scheduler->ChangePriorityList(heldBy,heldBy->GetPriority(),heldBy->GetEffectivePriority());
            //scheduler->ReadyToRun(currentThread); 
        }
    }
    sem->P();
    heldBy = currentThread; 
    DEBUG('s',"Thread %s acquired the lock %s\n",currentThread->getName(),name);       
}

void
Lock::Release(){
	DEBUG('s',"Thread %s tries to release the lock %s\n",currentThread->getName(),name);
    ASSERT(isHeldByCurrentThread());

    DEBUG('s',"Thread %s released the lock %s\n",currentThread->getName(),name);
    if (currentThread->GetEffectivePriority() != currentThread->GetPriority()){
        scheduler->ChangePriorityList(heldBy,heldBy->GetEffectivePriority(),heldBy->GetPriority());
        currentThread -> ResetPriority();
    }

    heldBy = NULL;
    sem->V();    
}

bool Lock::isHeldByCurrentThread(){
    return heldBy == currentThread;
}


Condition::Condition(const char *debugName, Lock *conditionLock){
    DEBUG('s',"Constructing conditional variable %s\n",debugName);
    queue = new List<Semaphore*>;    
    name = debugName;
    lock = conditionLock;    
}

Condition::~Condition(){
    delete queue;
}

void Condition::Wait(){
    ASSERT(lock->isHeldByCurrentThread());
    Semaphore *sem = new Semaphore("CondSem", 0);
    queue->Append(sem);
    lock->Release();
    sem->P(); 
    lock->Acquire();   
    delete sem;
}

void Condition::Signal(){
    Semaphore *semTemp;
    
    ASSERT(lock->isHeldByCurrentThread());    
    if (!queue->IsEmpty()) {
        semTemp = queue->Remove();
        semTemp->V();
    }
}

void Condition::Broadcast(){
    Semaphore *sem;

    while (!queue->IsEmpty()) {
        sem = queue->Remove();
        sem->V();
    }    
}


Port::Port(const char* debugName){
    full = false;
    name = debugName; 
    lock = new Lock("PortLock");
    write_allowed = new Condition("write_allowed", lock);
    read_allowed = new Condition("read_allowed", lock);
    message_received = new Condition("message_received", lock);
}


Port::~Port(){
    delete lock;
    delete write_allowed;
	delete read_allowed;
	delete message_received;
}

void Port::Send(int data){

    lock->Acquire();

    // Wait until the receiver allows us to write and the buffer is empty
    while(full)
        write_allowed->Wait();
    
    // Write our message
    buffer = data;
    full = true;

    // Tell the receiver that it's allowed to start reading
    read_allowed->Signal();

    // Wait until the message is acknowledged
    message_received->Wait();

    lock->Release();
}


void Port::Receive(int *data){

    lock -> Acquire();

    // Wait until the buffer has contents AND the sender lets us read
    while(!full)
        read_allowed->Wait();

    // Read the message
    full = false;
    *data = buffer;

    // Acknowledge the message
    message_received->Signal();

    // Let any other sender start writing
    write_allowed->Signal();

    lock->Release();
}