#include "syscall.h"
void forktest();
int main()
{
   
  int that;
  Fork(forktest);
  that = Join(1);
  Exit(0);
    
}
void forktest()
{
  Exit(1);
}


