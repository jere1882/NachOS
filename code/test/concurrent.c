#include "syscall.h"


int main (int argc, char **argv)
{
	
    int i,j,k;
    
    int pasaje = 0;

    for(i=1; i<1000; i++){  
        for(j=1; j<1000; j++){  
                pasaje++;
                pasaje--;
        }
    }
    
    if (pasaje==0){
       Write(argv[1],1,1);
       Write("\n",1,1);
    }
    else{
       Write("Failed\n",7,1);
    }
    return 0;
	
}