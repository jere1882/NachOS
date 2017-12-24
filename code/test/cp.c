#include "syscall.h"


int main (int argc, char **argv)
{
	
	const OpenFileId OUTPUT = ConsoleOutput;
	    
    if(argc != 3){	
	   static const char usage[] =  "NAME       cp - copy files \n"
                                    "SYNOPSIS   cp SOURCE DEST  \n";
       Write(usage, sizeof(usage), OUTPUT);
       return 0;
    }

    OpenFileId s = Open(argv[1]);
    Create(argv[2]);
    OpenFileId d = Open(argv[2]);
      
     if (s < 0 || d < 0) return 0;
     
     char c;
      
	while(Read(&c,1,s)){  
		Write(&c,1,d);
	}
				
	Close(s);
	Close(d);

    
	return 0;
	
}

