/// Routines to choose the next thread to run, and to dispatch to that
/// thread.
///
/// These routines assume that interrupts are already disabled.  If
/// interrupts are disabled, we can assume mutual exclusion (since we are on
/// a uniprocessor).
///
/// NOTE: we cannot use `Lock`s to provide mutual exclusion here, since if we
/// needed to wait for a lock, and the lock was busy, we would end up calling
/// `FindNextToRun`, and that would put us in an infinite loop.
///
/// Very simple implementation -- no priorities, straight FIFO.  Might need
/// to be improved in later assignments.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "scheduler.hh"
#include "system.hh"


/// Initialize the list of ready but not running threads to empty.
Scheduler::Scheduler()
{
	int i;
	for (i=0; i<=MAX_PRIO ; i++)
		rdyLists[i]= new List<Thread*>;
}

/// De-allocate the list of ready threads.
Scheduler::~Scheduler()
{
	int i;
	for (i=0; i<=MAX_PRIO ; i++)
		delete rdyLists[i];
		
}


/// Mark a thread as ready, but not running.
/// Put it on the ready list, for later scheduling onto the CPU.
///
/// * `thread` is the thread to be put on the ready list.
void
Scheduler::ReadyToRun(Thread *thread)
{

	// int prio= thread->GetPriority();
	int prio = thread->GetEffectivePriority();
    thread->setStatus(READY);
    if (prio >= MAX_PRIO)
		prio = MAX_PRIO;
	if (prio < 0)
		prio = 0;
	DEBUG('t', "Putting thread %s on ready list %d.\n", thread->getName(),prio);
    rdyLists[prio]->Append(thread);
}

/// Return the next thread to be scheduled onto the CPU.
///
/// If there are no ready threads, return `NULL`.
///
/// Side effect: thread is removed from the ready list.
Thread *
Scheduler::FindNextToRun()
{
	int i;

	for (i=MAX_PRIO; i>=0 ; i--)
		if (!rdyLists[i]->IsEmpty())
		   return rdyLists[i]->Remove();

	return NULL;
	
}

/// Dispatch the CPU to `nextThread`.
///
/// Save the state of the old thread, and load the state of the new thread,
/// by calling the machine dependent context switch routine, `SWITCH`.
///
/// Note: we assume the state of the previously running thread has already
/// been changed from running to blocked or ready (depending).
///
/// Side effect: the global variable `currentThread` becomes `nextThread`.
///
/// * `nextThread` is the thread to be put into the CPU.

void
Scheduler::Run(Thread *nextThread){
    Thread *oldThread = currentThread;

#ifdef USER_PROGRAM  // Ignore until running user programs.
    if (currentThread->space != NULL) {
        // If this thread is a user program, save the user's CPU registers.
        currentThread->SaveUserState();
        currentThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow();  // Check if the old thread had an undetected
                                 // stack overflow.

    currentThread = nextThread;  // Switch to the next thread.
    currentThread->setStatus(RUNNING);  // `nextThread` is now running.

    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
          oldThread->getName(), nextThread->getName());

    // This is a machine-dependent assembly language routine defined in
    // `switch.s`.  You may have to think a bit to figure out what happens
    // after this, both from the point of view of the thread and from the
    // perspective of the “outside world”.

    SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing, we
    // need to delete its carcass.  Note we cannot delete the thread before
    // now (for example, in `Thread::Finish`), because up to this point, we
    // were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        DEBUG('t', "Deleting thread \"%s\"\n", threadToBeDestroyed->getName());
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {
        // If there is an address space to restore, do it.
        currentThread->RestoreUserState();
        currentThread->space->RestoreState();
        DEBUG('t', "Restoring registers and memory of thread \"%s\"\n", currentThread->getName());

    }
#endif
}

/// Busca en cola de prioridad orig, lo elimina si está
/// Lo agrega en la cola de prioridad targ

void //pensar bien
Scheduler::ChangePriorityList(Thread* thread, int orig, int targ){
	ASSERT( orig>=0 && targ >=0 && orig<=MAX_PRIO && targ <=MAX_PRIO);
	List<Thread*> * new_orig = new List<Thread*>;
	Thread *temp;
	while (!rdyLists[orig]->IsEmpty()) {
		temp = rdyLists[orig]->Remove();
		if (temp != thread) //¿Cómo comparar threads?
			new_orig->Append(temp);
	}
	DEBUG('t', "Sacamos el hilo %s de la lista %d si estaba.\n", thread->getName(),orig);
	if (thread->getStatus()==READY){
		rdyLists[targ]->Append(thread);
	DEBUG('t', "Putting thread %s on ready list %d.\n", thread->getName(),targ);
	}
	delete rdyLists[orig];
	rdyLists[orig] = new_orig;
}



/// Print the scheduler state -- in other words, the contents of the ready
/// list.
///
/// For debugging.
static void
ThreadPrint(Thread* t)
{
    t->Print();
}

void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    int i;
    for (i=MAX_PRIO;i>=0; i--)
		rdyLists[i]->Apply(ThreadPrint);
}
