#ifndef SYNCHCONSOLE_HH
#define SYNCHCONSOLE_HH

#include "threads/synch.hh"
#include "machine/console.hh"

class SynchConsole {
public:
    // Inicializar la consola
    SynchConsole(const char *readFile, const char *writeFile);
/// * `readFile` is a UNIX file simulating the keyboard (when `NULL`, use
///   stdin).
/// * `writeFile` is a UNIX file simulating the display (when `NULL`, use
///   stdout).
/// * `readAvail` is the interrupt handler called when a character arrives
///   from the keyboard.
/// * `writeDone` is the interrupt handler called when a character has been
///   output, so that it is ok to request the next char be output.

    // Eliminar la consola
    ~SynchConsole();

    // Read/write a consola
    char GetChar();
    void PutChar(char);


    void ReadAvail();
    void WriteDone();

private:
    Console *console;
    
    Semaphore *rSem;
    Semaphore *wSem;
    Lock *rLock;
    Lock *wLock;
};

#endif 
