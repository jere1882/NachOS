
#ifndef PROCTABLE_H
#define PROCTABLE_H

#include "../threads/thread.hh"
#include "syscall.h"

#define NUM_MAX_PROC 777

class ProcTable {
	
  public:
	ProcTable();  //constructor	
    ~ProcTable(); //destructor
		
    Thread* Fetch(int i);  
    SpaceId Add(Thread *t);
    void Remove(int i);
    
  private:
	  Thread *procTable[NUM_MAX_PROC];
};
   
#endif


