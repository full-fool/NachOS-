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
    Exec("halt");
    Exit(0);
    
}


/*
#include "syscall.h"

int A[100];  
int
main()
{
    int i, j, tmp;

   
    for (i = 0; i < 100; i++)        
        A[i] = 100 - i;


    for (i = 0; i < 99; i++)
        for (j = i; j < (99 - i); j++)
       if (A[j] > A[j + 1]) {   
          tmp = A[j];
          A[j] = A[j + 1];
          A[j + 1] = tmp;
           }
    Exit(A[0]);    
}

*/
