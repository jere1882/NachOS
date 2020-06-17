/// Simple test case for the threads assignment.
///
/// Create several threads, and have them context switch back and forth
/// between themselves by calling `Thread::Yield`, to illustrate the inner
/// workings of the thread system.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "system.hh"
#include "synch.hh"
#include <utility>
using namespace std;

void
LockThread(void* varg)
{
    pair<char*, Lock*> *arg= (pair<char*, Lock*>*)varg;
    
    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
    DEBUG('t', "Thread %s is locking\n", arg->first);
    arg->second->Acquire();

    for (int num = 0; num < 10; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	printf("*** thread %s looped %d times\n", arg->first, num);
	//interrupt->SetLevel(oldLevel);
      currentThread->Yield();
    }
    DEBUG('t', "Thread %s is unlocking\n", arg->first);
    arg->second-> Release ();
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> Thread %s has finished\n", arg->first);
    //interrupt->SetLevel(oldLevel);
}

void
LockTest()
{
    DEBUG('t', "Entering LockTest\n");
    Lock *l = new Lock("lockTest");

    for(int i=4; i>=0; i--){
        char *threadname = new char[128];
        pair<char*, Lock*> *arg = new pair<char*, Lock*>();
        arg->first = threadname;
        arg->second = l;
        sprintf(threadname, "Hilo %d", i);
        Thread* newThread = new Thread (threadname);
        if(i!=0) 
            newThread->Fork (LockThread, (void*)arg);
        else 
            LockThread( (void*)arg);
    }
}

////////////////////////////////////////////////////////////////////

Lock *lock;
Semaphore *s1,*s2;

void Low (void*)
{
	DEBUG ('t'," Running Low Priority thread\n");
	lock -> Acquire ();
	DEBUG ('t',"Low acquired the lock \n");
	s1-> V();
	s2-> V();
    DEBUG ('t',"Low Yields \n");
	currentThread -> Yield();
    lock -> Release ();
}
void High (void *)
{
	s1 ->P();
	DEBUG('t',"Running High Priority thread\n");
    DEBUG ('t',"High attempts to acquire the lock \n");
	lock -> Acquire();
	DEBUG ('t',"High acquired the lock \n");
	lock -> Release();

}
void Mid (void*)
{
	DEBUG('t',"Running Mid priority thread\n");
	s2 ->P();
	DEBUG ('t',"Running infinite loop in Mid, High should've finished by now \n");
	while (1);
}

// https://en.wikipedia.org/wiki/Priority_inheritance example
void InversionPrioridadesTest ()
{
	DEBUG('t',"InversionPrioridadesTest \n");
	lock = new Lock ("InversionPrioridadesTest_lock");
	s1 = new Semaphore ("s1",0);
    s2 = new Semaphore ("s2",0);
	Thread* alta = new Thread ("High",false,2);
	alta -> Fork(High,(void*)"High");
	Thread *media = new Thread ("Mid",false,1);
	media->Fork(Mid,(void*)"Mid");
	Low((void*)"Low");
	currentThread ->Yield();
}

void
ThreadTest()
{
    //LockTest();
    InversionPrioridadesTest();  

}

