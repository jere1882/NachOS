#include "proctable.hh"

ProcTable::ProcTable(){  // Inicializa la tabla vacía.
	int i;
	for(i = 0; i < NUM_MAX_PROC; i++) {
		procTable[i]=NULL;
	}
}

ProcTable::~ProcTable(){
}

SpaceId ProcTable::Add(Thread *t){
    int j;
    for (j = 0; j < NUM_MAX_PROC; j++)
		if (procTable[j]==NULL){
			procTable[j] = t;
			return j;
		};
	return -1;
}
  
Thread*  ProcTable::Fetch(int i){
	if(i < 0 || i >= NUM_MAX_PROC) return NULL;
	return procTable[i];
}

void  ProcTable::Remove(int i){
	if (i < 0 || i >= NUM_MAX_PROC) return;
	procTable[i] = NULL;
	return;
}

