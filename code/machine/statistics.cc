/// Routines for managing statistics about Nachos performance.
///
/// DO NOT CHANGE -- these stats are maintained by the machine emulation.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2017 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "statistics.hh"
#include "threads/utility.hh"


/// Initialize performance metrics to zero, at system startup.
Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    numTLBHits = numTLBMisses = 0;
    swaps_in = swaps_out = 0;
    numFrames = -1;
#ifdef DFS_TICKS_FIX
    tickResets = 0;
#endif

}

/// Print performance metrics, when we have finished everything at system
/// shutdown.
void
Statistics::Print()
{
#ifdef DFS_TICKS_FIX
    if (tickResets != 0)
        printf("WARNING: the tick counter was reset %lu times; the following"
               " statistics may be invalid.\n\n", tickResets);
#endif
    printf("Ticks: total %u, idle %u, system %u, user %u\n",
           totalTicks, idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %u, writes %u\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %u, writes %u\n",
           numConsoleCharsRead, numConsoleCharsWritten);
    printf("Paging: faults %u\n", numPageFaults);
    printf("Paging: TLB hits %u\n", numTLBHits);
    printf("Paging: TLB misses %u\n", numTLBMisses);
    if (numTLBHits+numTLBMisses!=0)
    {
        float ratio = (float)numTLBHits/(float)(numTLBHits+numTLBMisses);
        printf("Paging: TLB Ratio %f\n",ratio);
    }

    printf("Paging: swaps_in %u\n", swaps_in);
    printf("Paging: swaps_out %u\n", swaps_out);
    //calculateStatsOptimalPGA();

    printf("Network I/O: packets received %u, sent %u\n",
           numPacketsRecvd, numPacketsSent);
}



/* Brute force approach */
void 
Statistics::calculateStatsOptimalPGA(){

    if (numFrames==-1){
        return;
    }

    bool ever_loaded[numPages];
    int coremap[numFrames]; // Simulated memory

    // We start with an empty trace
    for (int i = 0 ; i < numFrames ; i++)
        coremap[i] = -1;

    // No page has been loaded at the beginning
    for (int i = 0 ; i < numPages ; i++)
        ever_loaded[i] = false;

    unsigned op_swaps_in = 0;    /// Swap to memory
    unsigned op_swaps_out = 0;   /// Memory to swap
    unsigned op_hits = 0;
    // Let's iterate over the trace

    for (int i = 0 ; i < referenced_pags.size() ; i++){

        int vpn = referenced_pags[i]; 

        //////////  1- If vpn is already in memory, continue processing the trace :)  //////////
        bool found = false;

        for (int j = 0 ; j < numFrames ; j++){
        if (coremap[j]==vpn){
            found = true;
            break;
        }
        }

        if (found) {
        op_hits++;
        continue;
        }
        /////////////////////////////////////////////////////////////////////////////////////////

        // SO: WE WANT TO LOAD A vpn THAT IS NOT IN MEMORY!

        // If there is an emtpy frame, put the vpn there and continue processing the trace
        found = false;

        for (int j = 0 ; j < numFrames ; j++){
        if (coremap[j]==-1){
            coremap[j] = vpn;
            found = true;
            break;
        }
        }

        if (found) {
        if (ever_loaded[vpn]){
            swaps_in++;  // We move the contents from swap to memory
        }
        else {
            ever_loaded[vpn] = true;
            // We load the fage for the 1st time, this is not a swap in or out.
        }
        continue;
        }

        //////////////////////////////////////////////////////////////////////////////////////////

        // The vpn is not in memory
        // we have no empty frames... 
        // Thus, we have to choose a victim frame and send it to memory

        // We must choose the frame that will be used most lately

        int next_use_distance[numFrames]; // Para cada vpn del coremap, en cuantos pasos se vuelve a usar

        for (int j = 0 ; j < numFrames ; j++)
        next_use_distance[j] = -1;  // Default: Never used again

        // Now we process the rest of the trace
        for (int j = i+1 ; j <  referenced_pags.size() ; j++){
        int referenced_page = referenced_pags[j];
        int distance = j-i;
        
        for (int k = 0 ; k < numFrames ; k++) {
            if (coremap[k]==referenced_page && next_use_distance[k]==-1)
            next_use_distance[k]=distance;
        }

        }

        int victim_frame = -1;
        int max_d = -1;

        for (int j = 0 ; j < numFrames ; j++){
        if (next_use_distance[j]==-1){ // If there is a frame that will never be used again, we choose that one
            victim_frame = j;
            break;
        }
        else if (next_use_distance[j] > max_d){
            max_d = next_use_distance[j];
            victim_frame = j; 
        }
        }

        if (victim_frame==-1) {
            printf("[statistics] Optimal page replacement algorithm simulation error \n");
        return;
        }

        // We send the victim frame to swap
        op_swaps_in++;
        // We bring the desired vpn to memory
        coremap[victim_frame] = vpn;
        op_swaps_out++;

    }

    printf("Optimal paging: swaps_in %u\n", op_swaps_in);
    printf("Optimal paging: swaps_out %u\n", op_swaps_out);
    printf("Optimal paging: hits %u\n", op_hits);

}
