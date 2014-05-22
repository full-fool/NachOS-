// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "noff.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void forkProc(int funcAddr);

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) 
    {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        printf("it's going to halt\n");
   	    interrupt->Halt();
    }
    else if(which == PageFaultException)
    {

        int addr = machine->ReadRegister(BadVAddrReg);
        bool swapResult = machine->TlbSwap(addr, 1);
        ASSERT(swapResult);
        
    }
    else if(which == SyscallException && type == SC_Create)
    {
        printf("syscall create called\n");
        int baseAddr = machine->ReadRegister(4);
        int readValue;
        int count = 0;
        char fileName[30];
        memset(fileName, 0, sizeof fileName);
        for (int i = 0; i < 30; i++)
        {
          machine->ReadMem(baseAddr + i, 1, &readValue);
          fileName[i] = (char)readValue;
          if (readValue == 0)
              break;
        }
        printf("file %s needs to be created\n", fileName);
#ifdef FILESYS_STUB
        fileSystem->Create(fileName, 128);
#else 
        fileSystem->Create(fileName, 128, 'f', "/");
#endif

        machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Open)
    {
//#ifdef FILESYS_STUB
      printf("syscall open called\n");
      int baseAddr = machine->ReadRegister(4);
      int readValue;
      int count = 0;
      int retVal;
      do{
        machine->ReadMem(baseAddr++, 1, &readValue);
        count++;
      }while(readValue != 0);

      baseAddr -= count;
      char *fileName = new char[count];
      for(int i=0; i<count; i++)
      {
        machine->ReadMem(baseAddr + i, 1, &readValue);
        fileName[i] = (char)readValue;
      }

#ifdef FILESYS_STUB
        
#else 
      retVal = fileSystem->SysCallOpen(fileName);
#endif
      printf("in syscall open, fileId returned is %d\n", retVal);
      machine->WriteRegister(2, retVal);
      //printf("!!!!!!!!!!!!!!!1in exception.cc SC_open, %d is written to reg2\n", retVal);
//#endif
      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Close)
    {
      printf("syscall close called\n");
      int fileId = machine->ReadRegister(4);

#ifdef FILESYS_STUB

#else

      fileSystem->SysCallClose(fileId);
#endif
      //Close(fileId);
      printf("syscall close return\n");

      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Write)
    {
      printf("syscall write called\n");
      int baseAddr = machine->ReadRegister(4);
      int size = machine->ReadRegister(5);
      int fileId = machine->ReadRegister(6);
      printf("in write syscall, size is %d, fileId is %d\n", size, fileId);
      int readValue;
      int count = 0;
      do{
        machine->ReadMem(baseAddr++, 1, &readValue);
        count++;
      }while(readValue != 0);

      baseAddr -= count;
      char *content = new char[count];
      for(int i=0; i<count; i++)
      {
        machine->ReadMem(baseAddr + i, 1, &readValue);
        content[i] = (char)readValue;
      }
#ifdef FILESYS_STUB
 
#else
      fileSystem->SysCallWrite(content, size, fileId);
#endif

      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Read)
    {
      printf("syscall read called\n");
      int baseAddr = machine->ReadRegister(4);
      int size = machine->ReadRegister(5);
      int fileId = machine->ReadRegister(6);
      int count = 0;
      char *temp = new char[size];
      //OpenFile *openfile = new OpenFile(fileId);
      //count = openfile->Read(temp, size);
#ifdef FILESYS_STUB

#else
      count = fileSystem->SysCallRead(temp, size, fileId);
#endif
      for(int i=0; i<size; i++)
      {
        machine->WriteMem(baseAddr+i, 1, temp[i]);
      }
      machine->WriteRegister(2, count);
      //printf("!!!!!!!!!!!!!!!!!!in exception.cc SC_Read, %d is written to reg2\n", count);

      //delete openfile;
      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Exec)
    {
      printf("syscall exec called\n");
      int baseAddr = machine->ReadRegister(4);
      int readValue;
      int count = 0;
      AddrSpace *space;
      do{
        machine->ReadMem(baseAddr++, 1, &readValue);
        count++;
      }while(readValue != 0);
      baseAddr -= count;
      char *fileName = new char[count];
      for(int i=0; i<count; i++)
      {
        machine->ReadMem(baseAddr + i, 1, &readValue);
        fileName[i] = (char)readValue;
      }
#ifdef FILESYS_STUB

      OpenFile *executable = fileSystem->Open(fileName);
#else
      OpenFile *executable = fileSystem->Open("/", fileName);
#endif
      if(executable == NULL)
      {
        printf("cannot open %s\n", fileName);
        machine->AddPC();
        return;
      }
      //printf("in exec, %s opened successfully\n", fileName);
      delete fileName;
      space = new AddrSpace(executable);
      currentThread->space = space;
      delete executable; 
      space->InitRegisters(); 
      space->RestoreState();
      machine->cleanTlb();     //very very important!!!!!!!!!!!!!
      //printf("!!!!!!!!!!!!!!!!!exec here!!!!\n");
      int retVal = currentThread->getTid();
      machine->WriteRegister(2, retVal);



      machine->Run();
      ASSERT(FALSE);

    }

    else if(which == SyscallException && type == SC_Join)
    {
      int arg1 = machine->ReadRegister(4);
      printf("syscall join called, thread %d is joined\n", arg1);

      int retVal;
      if(tidUse[arg1] == 1)
        retVal = threads[arg1]->getUid();
      else
        retVal = -1;
      if(retVal > -1)
        currentThread->Yield();
      machine->WriteRegister(2, retVal);
      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Exit)
    {
      int retVal = machine->ReadRegister(4);
      printf("syscall exit called, the procedure exited with status %d\n", retVal);
      currentThread->Finish();
      machine->AddPC();
    }

    else if(which == SyscallException && type == SC_Fork)
    {
      printf("syscall fork called\n");
      int virtualSpace = machine->ReadRegister(4);
      //SCFork(arg1);

      AddrSpace * space;
      Thread * thread;
      space = new AddrSpace(*currentThread->space);
      thread = new Thread("forked");
      thread->space = space;
      space->SaveState();

      int* currentState = new int[NumTotalRegs];
      for(int i =0; i < NumTotalRegs;i++)
          currentState[i] = machine->ReadRegister(i);
      currentState[PCReg] = virtualSpace;
      currentState[NextPCReg] = virtualSpace+4;

      thread->Fork(forkProc, (int)currentState);


      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Yield)
    {
      printf("syscall yield called, the currentThread yield!\n");
      currentThread->Yield();
      machine->AddPC();
    }
    else 
    {
   	    if(which == IllegalInstrException && type == SC_Halt)
          printf("IllegalInstrException happened and type is SC_Halt\n");
        printf("exception is %d and type is %d\n", which, type);
        interrupt->Halt();
    	 
	       printf("Unexpected user mode exception %d %d\n", which, type);
	       //ASSERT(FALSE);
    }
}

void forkProc(int funcAddr) 
{
    int* state = (int*)funcAddr;
    for(int i =0; i < NumTotalRegs;i++)
         machine->WriteRegister(i,state[i]);
    delete[] state;

    currentThread->space->RestoreState(); // load page table register
    machine->Run(); // jump to the user progam
    ASSERT(FALSE); // machine->Run never returns;
}