#include "paginador.hh"


/* NOTE: One and only of of the following definitions must be uncommented */
//#define FIFO
//#define RANDOM
#define CLOCK

Paginador::Paginador() {

    for (unsigned i=0; i<NUM_PHYS_PAGES; i++){
        coremap[i].space = NULL;
        coremap[i].vpn = -1;
    }

    usedFrames = 0;

#ifdef CLOCK
    fcq_index = -1;
#endif

}


Paginador::~Paginador() {
}

void Paginador::ReleaseFrame(int frame) {

    ASSERT(frame >= 0 && frame < (int)NUM_PHYS_PAGES);
 
#ifdef FIFO
    ASSERT( (std::find(frame_queue.begin(), frame_queue.end(), frame)) != frame_queue.end())
    frame_queue.remove(frame);
    DEBUG('f', "Removing frame %d from the queue (released frame) \n", frame);
#endif

#ifdef CLOCK
    remove_element_from_circular_queue(frame);
    DEBUG('c', "Removing frame %d from the queue (released frame) \n", frame);
#endif

    if (coremap[frame].space!=NULL) {
        usedFrames--;
        ASSERT(usedFrames>=0);
    }

    coremap[frame].space = NULL;
    coremap[frame].vpn = -1;

}

int Paginador::FindFreeFrame(AddressSpace *new_space, int new_vpn) {

    DEBUG('v', "space %d is looking for a free frame to save vpn %d \n", new_space->get_pid(),new_vpn);

    // Caso en que la memoria está llena:
    if (usedFrames == NUM_PHYS_PAGES) {

        // Elegir un marco a liberar
        int victim = ChooseVictimFrame();
        int victim_vpn = coremap[victim].vpn;

        DEBUG('v', "Memory is full, sending frame %d (adds %d vpn %d)  to swap \n",victim,coremap[victim].space->get_pid(),victim_vpn);

        // Mandarlo a swap
        coremap[victim].space->MemoryToSwap(victim_vpn);

        // Reasignar el frame
        coremap[victim].space = new_space;
        coremap[victim].vpn   = new_vpn;

        #ifdef FIFO
        frame_queue.push_back(victim);
        DEBUG('f', "Pushing frame %d into the queue \n", victim);
        #endif

        #ifdef CLOCK
        insert_element_in_the_back(victim);
        DEBUG('c', "Inserting frame %d at the end of the circular queue \n", victim);
        #endif

        return victim;
    }

    // Caso en que hay un marco libre:
    for (unsigned i=0; i<NUM_PHYS_PAGES; i++){
        if (coremap[i].space==NULL) {
            coremap[i].space = new_space;
            coremap[i].vpn   = new_vpn;
            usedFrames++;
            DEBUG('v', "it was given free frame %d \n", i);

            #ifdef FIFO
            frame_queue.push_back(i);
            DEBUG('f', "Pushing frame %d into the queue \n", i);
            #endif

            #ifdef CLOCK
            insert_element_in_the_back(i);
            DEBUG('c', "Inserting frame %d at the end of the circular queue \n", i);
            #endif

            return i;
        }
    }

    ASSERT(false); //nunca se alcanza
    return -1;
}

int Paginador::ChooseVictimFrame(){

    ASSERT(usedFrames == NUM_PHYS_PAGES);

    #ifdef FIFO
        return ChooseVictimFrame_FIFO();
    #endif
    
    #ifdef CLOCK
        return ChooseVictimFrame_Clock();
    #endif
    
    #ifdef RANDOM
        return ChooseVictimFrame_Random();
    #endif

    ASSERT(false);
    return -1;
}

unsigned Paginador::ChooseVictimFrame_Random(){

    ASSERT(usedFrames == NUM_PHYS_PAGES);
    unsigned r = rand() % NUM_PHYS_PAGES;
    return(r);

}

// The frame that has been in memory the longest, is replaced 
unsigned Paginador::ChooseVictimFrame_FIFO(){

    ASSERT(usedFrames == NUM_PHYS_PAGES);
    ASSERT(frame_queue.size()>0);

    unsigned page = frame_queue.front();
    frame_queue.pop_front();
    DEBUG('f', "Popping frame %d from the queue (victim frame) \n", page);

    return page;

}

unsigned Paginador::ChooseVictimFrame_Clock(){

    DEBUG('c', "ChooseVictimFrame_Clock \n");

    ASSERT(usedFrames == NUM_PHYS_PAGES);
    ASSERT(frame_circular_queue.size()>0)

    while(true){
        int candidate_frame = circular_list_pop_front_element();
        CoreMapEntry entry = coremap[candidate_frame];
        DEBUG('c', "popped frame %d \n", candidate_frame);        
        if ( (entry.space->pageTable[entry.vpn]).use==false ) {  // TODO: What if that entry is in the TLB? 
            DEBUG('c', "use bit is false, victim chosen \n");        
            return candidate_frame;
        }
        else {
            (entry.space->pageTable[entry.vpn]).use=false;
            insert_element_in_the_back(candidate_frame);
            DEBUG('c', "use bit is on, setting it off \n");        
        }
    }

    ASSERT(false);
    return 0;

}




void Paginador::remove_element_from_circular_queue(int frame){
    print_circular_list();
    DEBUG('c', "remove_element_from_circular_queue %d \n",frame);        

    int size = frame_circular_queue.size();
    ASSERT(size>0);
    ASSERT(fcq_index!=-1);

    std::vector<int>::iterator it = std::find(frame_circular_queue.begin(), frame_circular_queue.end(), frame);

    ASSERT(it != frame_circular_queue.end()); // The element is in the queue

    // Find element index
    int index = std::distance(frame_circular_queue.begin(), it);
    ASSERT(0 <= index && index < size);

    // Remove the element
    frame_circular_queue.erase(frame_circular_queue.begin()+index);

    // If there's only one element in the list, then we're emptying it
    if (size==1){
        fcq_index = -1;
    }
    else if (index == fcq_index){
        if (index==size-1){
            fcq_index = 0;
        }
    }
    else if (index < fcq_index) {
        fcq_index--;
    }
    
    ASSERT ( fcq_index>=0 && fcq_index < (int)frame_circular_queue.size());

}

void Paginador::insert_element_in_the_back(int frame){
    print_circular_list();
    DEBUG('c', "inserting element %d in the back \n",frame);        

    if (fcq_index==-1){
        frame_circular_queue.push_back(frame);
        fcq_index=0;
        return;
    }

    std::vector<int>::iterator it = frame_circular_queue.begin();

    for (int i = 0 ; i< fcq_index ; i++)
        it++;

    frame_circular_queue.insert(it,frame);
    fcq_index++;
}

void Paginador::print_circular_list() {
    DEBUG('c', "CL: ");

    for (unsigned i = 0 ; i< frame_circular_queue.size() ; i++)
    DEBUG('c', " %d ",frame_circular_queue[i]);

    DEBUG('c', "||| Pointing at position %d \n", fcq_index);

}

int Paginador::circular_list_pop_front_element() {
    print_circular_list();
    DEBUG('c', "circular_list_pop_front_element \n");        

    int size = frame_circular_queue.size();
    ASSERT(size>0);
    ASSERT(fcq_index!=-1);
    int elem = frame_circular_queue[fcq_index];

    frame_circular_queue.erase(frame_circular_queue.begin()+fcq_index);

    if(size==1){
        fcq_index=-1;
    }
    else if (fcq_index==size-1){
        fcq_index=0;
    }

    return elem;
}


