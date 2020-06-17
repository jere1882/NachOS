#ifndef _PAGINADOR_HH_
#define _PAGINADOR_HH_

#include "address_space.hh"
#include "machine.hh"
#include <list>
#include <vector>
#include <algorithm>

typedef struct CoreMapEntry {
    AddressSpace      *space;
    int               vpn;
} CoreMapEntry;

/* Esta clase se encarga de la asignación de marcos físicos para colocar las distintas
   páginas virtuales de memoria. 
   
   Tres diferentes estategias son implementadas:

   Random 
   FIFO   -> usa una cola
   Reloj  -> usa una lista circular 

*/

class Paginador {
  public:
    Paginador();
    ~Paginador();
    
    // Retorna un marco libre
    // Si todos están ocupados, selecciona uno y lo manda a swap
    // Luego, lo ocupa con los parametros provistos
    int FindFreeFrame(AddressSpace *new_space, int new_vpn);

    // Libera el frame fn. 
    // Cuando un thread finaliza, se invoca Clear en cada uno de sus marcos.
    // Esto ocasiona que todos los marcos que estaba usando pasen a estar disponibles 
    // para otros procesos.
    void ReleaseFrame(int fn);

  private:

    // Elegir un marco víctima para mandar a swap
    int ChooseVictimFrame();

    unsigned ChooseVictimFrame_Random();
    unsigned ChooseVictimFrame_FIFO();
    unsigned ChooseVictimFrame_Clock();

    CoreMapEntry coremap[NUM_PHYS_PAGES];
    int usedFrames;

    /// FIFO
    std::list<int> frame_queue;

    // RELOJ MEJORADO
    std::vector<int> frame_circular_queue;
    int fcq_index;

    void remove_element_from_circular_queue(int frame);
    void insert_element_in_the_back(int frame);
    void print_circular_list();
    int circular_list_pop_front_element();

};

#endif // _PAGINADOR_HH_