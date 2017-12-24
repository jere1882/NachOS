#include "syscall.h"


int main (int argc, char **argv)
{
	
	const OpenFileId OUTPUT = ConsoleOutput;
	
	
	    
    if(argc <= 1){	
	   static const char usage[] =  "NAME       cat - concatenate files and print on the standard output \n"
                                    "SYNOPSIS   cat [FILE]...\n";
       Write(usage, sizeof(usage), OUTPUT);
       return 0;
    }
    
    
    int i;
    
    for(i=1; i<argc; i++){   // Open each file and print it, char by char!
		
        OpenFileId f = Open(argv[i]);
        
        if (f >=0){
            char c;
            
            while(Read(&c,1,f)){  
				Write(&c,1,OUTPUT);
			}
			
            Close(f);
            Write('\n',1,OUTPUT);
        }
        
    }
    
	return 0;
	
}




