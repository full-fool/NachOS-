#include "syscall.h"

int main()
{
    //printf("haha\n");
    //Create("haha");
    //return 0;
    //Halt();
    int fileId;
    char buffer[5];
    Create("hongheila");
    //int fileId = 0;
    
    fileId = Open("hongheila");
	Write("abcd", 5, fileId);
	Read(buffer, 4, fileId);
	
    Exit(0);
}
