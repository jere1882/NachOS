#include "syscall.h"


#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '
#define BACKGR_TOKEN   '&'

#define NULL  ((void *) 0)

static inline unsigned
strlen(const char *s)
{
    if (s){
		unsigned i;
		for (i = 0; s[i] != '\0'; i++);
		return i;
	}
	return 0;
}

static inline void
WritePrompt(OpenFileId output)
{
    static const char PROMPT[] = "--> ";
    Write(PROMPT, sizeof PROMPT - 1, output);
}

static inline void
WriteError(const char *description, OpenFileId output)
{
    if (description){
		static const char PREFIX[] = "Error: ";
		static const char SUFFIX[] = "\n";

		Write(PREFIX, sizeof PREFIX - 1, output);
		Write(description, strlen(description), output);
		Write(SUFFIX, sizeof SUFFIX - 1, output);
  }
	
}

static unsigned
ReadLine(char *buffer, unsigned size, OpenFileId input)
{
    if (buffer){
	
		unsigned i;

		for (i = 0; i < size; i++) {
			Read(&buffer[i], 1, input);
			if (buffer[i]=='\0')
				break;
			if (buffer[i] == '\n') {
				buffer[i] = '\0';
				break;
			}
		}
		return i;
		
	}
	return 0;
}

static int
PrepareArguments(char *line, char **argv, unsigned argvSize)
{
    if (!line | !argv)
   		return -1;
	
    unsigned argCount;


    unsigned i=0;
    while(line[i]==ARG_SEPARATOR) // Fixes the problem of spaces at the beginning
		i++;


    argv[0] = &(line[i]);
    argCount = 1;
    	
    
    for ( ; line[i] != '\0'; i++)
        if (line[i] == ARG_SEPARATOR) {
			line[i] = '\0';
			while(line[i+1]==ARG_SEPARATOR){  // Fixes the problem of consecutive spaces
				line[i+1]='\0';
				i++;
			}
			
            if (argCount == argvSize - 1)
                return 0;

            argv[argCount] = &line[i + 1];
            argCount++;
        }

    argv[argCount] = NULL;
    return 1;
}

int
main(void)
{
    const OpenFileId INPUT  = ConsoleInput;
    const OpenFileId OUTPUT = ConsoleOutput;
    char             line[MAX_LINE_SIZE];
    char            *argv[MAX_ARG_COUNT];
    char            *line2;
	unsigned rb;
	
    for (;;) {
        WritePrompt(OUTPUT);
        const unsigned lineSize = ReadLine(line, MAX_LINE_SIZE, INPUT);
        if (lineSize == 0)
            continue;

		if (line[0]==BACKGR_TOKEN){
			line2 = &(line[1]);
			rb=1;
		} else {
			line2=line;
			rb=0;
		}
		
        if (PrepareArguments(line2, argv, MAX_ARG_COUNT) == 0) {
            WriteError("too many arguments.", OUTPUT);
            continue;
        }


        const SpaceId newProc = Exec(argv[0], argv);
		
		if (newProc<0) {
				WriteError("Exec error. File may not exist", OUTPUT);
				continue;	
		}

		if (!rb){
			if (Join(newProc)<0){
				WriteError("Join error.", OUTPUT);
				continue;
			}
		}
    }

    return 0;  // Never reached.
}
