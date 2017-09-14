#include "synchconsole.hh"

// interrupt handlers: Para crear la consola 
// Serán usados por la consola (variable privada de nuestra SynchConsole) 
// para avisar cuando un char llegue del teclado/haya sido mostrado.
// arg será la misma synchConsole_

static void handlerReadAvail(void* arg) { 
	((SynchConsole *)arg)->ReadAvail();
}
static void handlerWriteDone(void* arg) { 
	((SynchConsole *)arg)->WriteDone(); 
}

SynchConsole::SynchConsole(const char *readFile, const char*writeFile) {
    
    console = new Console(readFile, writeFile, handlerReadAvail, handlerWriteDone, this);	
	
    rSem  = new Semaphore ("SynchConsole Read Semaphore",0);
    rLock = new Lock("SynchConsole Read Lock");
    
    wSem  = new Semaphore ("SynchConsole Write Semaphore",0);
    wLock = new Lock ("SynchConsole Write Lock");
    
    /* Los locks permitirán leer/escribir exclusivamente*/ 
    
}

SynchConsole::~SynchConsole(){
    delete rSem;
    delete rLock;
    delete wLock;
    delete wSem;
    delete console;
}


char SynchConsole::GetChar() {
    rLock->Acquire();            // Leemos exclusivamente
    rSem->P();                   // Esperamos a que haya un caracter 
    char c = console->GetChar(); // Leemos el caracter
    rLock->Release();            // Terminamos de leer 
    return c;                    // Retornamos el caracter leído
}


void SynchConsole::PutChar(char ch) {
    wLock->Acquire();            // Escribimos exclusivamente
    console->PutChar(ch);        // Escribimos el caracter
    wSem->P();                   // Esperamos a que el caracter haya sido mostrado
    wLock->Release();            // Terminamos de escribir
}

void SynchConsole::WriteDone() { // Será llamada por handler
	wSem->V();
}

void SynchConsole::ReadAvail() { // Será llamada por handler
	rSem->V();
}


