// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    //printf("in %s thread, we need %d pages\n", currentThread->getName(), numPages);
    //printf("this thread needs %d pages \n", numPages);
    size = numPages * PageSize;
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
        // first, set up the translation 

    int intraFileAddr, virPageNum, offSet, phyPageNum;
    int bytesToWrite;
    int clearMem = memoryBitmap->NumClear();
    int lessPageNum;
    if(clearMem < numPages)   //on this condition, we should write info on swap zone since there is no enough memory
    {
        lessPageNum = clearMem;
        char tempBuffer[size + 10];
        char *fileName = currentThread->getName();
#ifdef FILESYS_STUB 
        fileSystem->Create(fileName, 0);
        swapFile = fileSystem->Open(fileName);
#else
        fileSystem->Create(fileName, 0, 'f', "/");
        swapFile = fileSystem->Open("/", fileName);
#endif
       
        if(swapFile == NULL)
        {
            printf("unable to open %s swap zone\n", fileName);
            interrupt->Halt();
            return;
        }
        else
            printf("%s swap zone opend successfully\n",fileName);
        
        if (noffH.code.size > 0) 
        {
            printf("noffh code size is %d and virtualAddr is %d\n", noffH.code.size, noffH.code.virtualAddr);
            DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
                noffH.code.virtualAddr, noffH.code.size);
            executable->ReadAt(&(tempBuffer[noffH.code.virtualAddr]),
               noffH.code.size, noffH.code.inFileAddr);
            /*
            int intraFileAddr = noffH.code.inFileAddr;
            int offset = noffH.code.virtualAddr;
            for(int i=0; i<noffH.code.size; i++)
            {
                executable->ReadAt(&(tempBuffer[offset]), 1, intraFileAddr);
                intraFileAddr++;
                offset++;
            }
            */
            //swapFile->Write((char *)&noffH, sizeof(noffH));
            //printf("len of tempBuffer is %d\n", strlen(tempBuffer));
            //printf("Tempbuffer is %s\n", tempBuffer);

        }
        if (noffH.initData.size > 0) 
        {
            printf("noffh initdata size is %d\n", noffH.initData.size);
            
            DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
                noffH.initData.virtualAddr, noffH.initData.size);
            executable->ReadAt(&(tempBuffer[noffH.initData.virtualAddr]),
                noffH.initData.size, noffH.initData.inFileAddr);
        }
        //printf("tempbuffer is %s\n", tempBuffer);
        //swapFile->Write(tempBuffer, strlen(tempBuffer));
    }
    else
    {
        lessPageNum = numPages;
    }
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < lessPageNum; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = memoryBitmap->Find();
	//pageTable[i].physicalPage = i;
    pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    pageTable[i].hitTimes = 0;
    }
    for(;i<numPages;i++)
    {
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].hitTimes = 0;
    }



    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    //bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
   
    if(noffH.code.size < lessPageNum * PageSize)
    {
        bytesToWrite = noffH.code.size;

    }
    else                    
    {
        bytesToWrite = lessPageNum * PageSize;


    }

    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
            noffH.code.virtualAddr, noffH.code.size);
        intraFileAddr = noffH.code.inFileAddr;
        //printf("infileaddr is %d and virtualAddr is %d\n", intraFileAddr, noffH.code.virtualAddr);
        for(i=0; i<bytesToWrite; i++)
        {
            virPageNum = (noffH.code.virtualAddr + i) / PageSize;
            offSet = (noffH.code.virtualAddr + i) % PageSize;
            phyPageNum = pageTable[virPageNum].physicalPage;
            //printf("intraFileAddr is %d, virPageNum is %d, offSet is %d, phyPageNum is %d, place to read is %d\n", 
              //  intraFileAddr, virPageNum, offSet, phyPageNum, phyPageNum * PageSize + offSet);
            executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 1, intraFileAddr);
            intraFileAddr++;
            //printf("main memory size is now %d\n", strlen(machine->mainMemory));
        }
        //printf("noffH.code.size is %d\n", noffH.code.size);
       
    }
    bytesToWrite = lessPageNum * PageSize - noffH.code.size;
    if(bytesToWrite > 0)
    {
        if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
            noffH.initData.virtualAddr, noffH.initData.size);
        intraFileAddr = noffH.code.inFileAddr;
        for(i=0; i<bytesToWrite; i++)
        {
            virPageNum = (noffH.initData.virtualAddr + i) / PageSize;
            offSet = (noffH.initData.virtualAddr + i) % PageSize;
            phyPageNum = pageTable[virPageNum].physicalPage;
            executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 1, intraFileAddr);
            intraFileAddr++;
        }
        }
    }



    
    /*
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        intraFileAddr = noffH.code.inFileAddr;
        for(i=0; i<noffH.code.size; i++)
        {
            virPageNum = (noffH.code.virtualAddr + i) / PageSize;
            offSet = (noffH.code.virtualAddr + i) % PageSize;
            phyPageNum = pageTable[virPageNum].physicalPage;
            //printf("intraFileAddr is %d, virPageNum is %d, offSet is %d, phyPageNum is %d, place to read is %d\n", 
              //  intraFileAddr, virPageNum, offSet, phyPageNum, phyPageNum * PageSize + offSet);
            executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 1, intraFileAddr);
            intraFileAddr++;
        }
        //printf("noffH.code.size is %d\n", noffH.code.size);
       
    }


    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        intraFileAddr = noffH.code.inFileAddr;
        for(i=0; i<noffH.initData.size; i++)
        {
            virPageNum = (noffH.initData.virtualAddr + i) / PageSize;
            offSet = (noffH.initData.virtualAddr + i) % PageSize;
            phyPageNum = pageTable[virPageNum].physicalPage;
            executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 1, intraFileAddr);
            intraFileAddr++;
        }
    }
    */

    

   
    //memoryBitmap->Print();
    

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   
   for(int i=0; i<numPages; i++)
   {
        memoryBitmap->Clear(pageTable[i].physicalPage);
   }
   
   delete pageTable;

}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    //printf("address space restored!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

int 
AddrSpace::getStackReg()
{
    return machine->ReadRegister(StackReg);
}

void 
AddrSpace::Suspend()
{
    currentThread->Suspend();
    for(int i=0; i<numPages; i++)
    {
        if(pageTable[i].valid)
        {
            memoryBitmap->Clear(pageTable[i].physicalPage);
        }
    }
}
