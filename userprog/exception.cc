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
        printf("PageFaultException happended here on virtual address %d\n", addr);
        bool swapResult = machine->TlbSwap(addr, 1);
        if(!swapResult)         //means tlb swap failed, and we should load right physical pages from file
        {
          printf("must load pages from disk\n");
          machine->LoadPage(addr);
          printf("after load page, swap tlb again\n");
          swapResult =  machine->TlbSwap(addr, 1);
          ASSERT(swapResult);

        }
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
        /*
        do{
          machine->ReadMem(baseAddr++, 1, &readValue);
          count++;
        }while(readValue != 0);
        //printf("filename length is %d\n", count);

        baseAddr -= count;
        char *fileName = new char[count];
        for(int i=0; i<count; i++)
        {
          machine->ReadMem(baseAddr + i, 1, &readValue);
          fileName[i] = (char)readValue;
        }
        */
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
      int retVal = fileSystem->SysCallOpen(fileName);
      machine->WriteRegister(2, retVal);
      //printf("!!!!!!!!!!!!!!!1in exception.cc SC_open, %d is written to reg2\n", retVal);
//#endif
      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Close)
    {
      printf("syscall close called\n");
      int fileId = machine->ReadRegister(4);
      Close(fileId);
      machine->AddPC();
    }
    else if(which == SyscallException && type == SC_Write)
    {
      printf("syscall write called\n");
      int baseAddr = machine->ReadRegister(4);
      int size = machine->ReadRegister(5);
      int fileId = machine->ReadRegister(6);
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
      fileSystem->SysCallWrite(content, size, fileId);
      //OpenFile *openfile = new OpenFile(fileId);
      //openfile->Write(content, size);
      //delete openfile;
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
      count = fileSystem->SysCallRead(temp, size, fileId);
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
      space = new AddrSpace(executable);
      currentThread->space = space;
      space->InitRegisters();
      space->RestoreState();
      delete space;
      machine->Run();
      ASSERT(FALSE);


    }

    else if(which == SyscallException && type == SC_Join)
    {
      
    }
    else if(which == SyscallException && type == SC_Exit)
    {
      printf("the procedure exited\n");
      currentThread->Finish();
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
