#include "syscall.h"

int main()
{
   
    int that;
    char did[20];
    int fileId;
    Create("heihaha");
    fileId = Open("heihaha");
	Write("hello world!\0", 15, fileId);
    //char *readbuffer;
    //Read(readbuffer, 15, fileId);
    Close(fileId);
    Exit(0);
    
}
