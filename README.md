# NachOS

Not Another Completely Heuristic Operating System, or Nachos, is instructional software for operating systems courses. Originally written in C++ for MIPS, Nachos runs as a user-process on a host operating system. A MIPS simulator executes the code for any user programs running on top of the Nachos operating system. 

This repo contains my final working version of Nachos, after solving the following assignments:

Assignment 2: Multithread synchronisation
  - Implementation of mutexes and conditional variables 
  - Implementation of Thread::join
  - Scheduling: Implementation of Multilevel feedback queue (Round robin)

Assignment 3: Computer multitasking 
  - Implementation of syscalls: Exec, Join, Create, Read, Write, Open, Close, Exit
  - Scheduling: Implementation of time slicing (preemption)
  - Allow execution of user-written programs with custom parameters
  - Implementation of a shell

Assignment 4: Virtual memory
  - Implementation of TLB (Translation Lookaside Buffer)
  - Enable the use of virtual memory
  - Implementaton of demand loading
  - Calculate stats on the number of TLB hits, page faults, etc.
  - Implementation of different page replacement algorithms: FIFO, LRU, Clock.
  
Additional:
  - An extensive set of tests was written
  - See documentation folder for a brief report on Assignment 4 (in Spanish)
