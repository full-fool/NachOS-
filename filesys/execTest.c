#include "syscall.h"
int main()
{
   
  	int fileID;
  	char tempBuffer[31];
  	Create("heihaha");
  	fileID = Open("heihaha");
	Write("test string:hello world\n", 30, fileID);
	Read(tempBuffer, 30, fileID);
	Close(fileID);
	Exec("halt");
	Exit(0);
    
}


