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




//old version of AddrSpace is stored in lab syscall.

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
    printf("code: size is %d, virtualAddr is %d, inFileAddr is %d\n", 
        noffH.code.size, noffH.code.virtualAddr, noffH.code.inFileAddr);
    printf("initData: size is %d, virtualAddr is %d, inFileAddr is %d\n", 
        noffH.initData.size, noffH.initData.virtualAddr, noffH.initData.inFileAddr);
    printf("uninitData: size is %d, virtualAddr is %d, inFileAddr is %d\n",
        noffH.uninitData.size, noffH.uninitData.virtualAddr, noffH.uninitData.inFileAddr);
    numPages = divRoundUp(size, PageSize);
    printf("file size is %d, numPages is %d\n", size, numPages);    
    size = numPages * PageSize;
    printf("new size is %d\n", size);
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
                    numPages, size);
        // first, set up the translation 

    int intraFileAddr, virPageNum, offSet, phyPageNum;
    int clearMem = memoryBitmap->NumClear();

    if(numPages <= clearMem)
    {
        printf("in Addrspace, numPages is %d, clearMem is %d, so no need to create swapfile\n", 
            numPages, clearMem);
        pageTable = new TranslationEntry[numPages];
        for (i = 0; i < numPages; i++) 
        {
            pageTable[i].virtualPage = i;   // for now, virtual page # = phys page #
            pageTable[i].physicalPage = memoryBitmap->Find();
            pageTable[i].valid = TRUE;
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE; 
            pageTable[i].hitTimes = 0;

        }
        if (noffH.code.size > 0) 
        {
            intraFileAddr = noffH.code.inFileAddr;
            for(i=0; i<noffH.code.size; i++)
            {
                virPageNum = (noffH.code.virtualAddr + i) / PageSize;
                offSet = (noffH.code.virtualAddr + i) % PageSize;
                phyPageNum = pageTable[virPageNum].physicalPage;
                executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 1, intraFileAddr);
                intraFileAddr++;
            }
        }


        if (noffH.initData.size > 0) 
        {
            intraFileAddr = noffH.initData.inFileAddr;
            for(i=0; i<noffH.initData.size; i++)
            {
                virPageNum = (noffH.initData.virtualAddr + i) / PageSize;
                offSet = (noffH.initData.virtualAddr + i) % PageSize;
                phyPageNum = pageTable[virPageNum].physicalPage;
                executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 1, intraFileAddr);
                intraFileAddr++;
            }
        }
        return;

    }
    //on this condition, we should write info on swap zone since there is no enough memory
    else 
    {
        printf("in Addrspace, clearMem is %d, numPages is %d, so not enough!\n", clearMem, numPages);
        pageTable = new TranslationEntry[numPages];
        for (i = 0; i < numPages; i++) 
        {
            pageTable[i].virtualPage = i;   // for now, virtual page # = phys page #
            if(i < clearMem)
            {
                pageTable[i].physicalPage = memoryBitmap->Find();
                pageTable[i].valid = TRUE;
            }
            else
            {
                pageTable[i].physicalPage = -1;
                pageTable[i].valid = FALSE;
            }
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE; 
            pageTable[i].hitTimes = 0;
           
        }

        char *fileName = currentThread->getName();
        bool createSuccess = FALSE;
    #ifdef FILESYS_STUB 
        createSuccess = fileSystem->Create(fileName, size);
        swapFile = fileSystem->Open(fileName);
    #else
        createSuccess = fileSystem->Create(fileName, size, 'f', "/");
        swapFile = fileSystem->Open("/", fileName);
    #endif
       
        if(swapFile == NULL)
        {
            printf("unable to open %s swap zone\n", fileName);
            interrupt->Halt();
            return;
        }
        
        if(createSuccess)
        {
            char *tempBuffer = new char[1]; 
            tempBuffer[0] = 0;
            int amountRead = 0;
            for(i=0; i<size; i++)
                swapFile->Write(&(tempBuffer[0]), 1);
            if (noffH.code.size > 0) 
            {
                intraFileAddr = noffH.code.inFileAddr;
                for(i=0; i<noffH.code.size; i++)
                {
                    virPageNum = (noffH.code.virtualAddr + i) / PageSize;
                    offSet = (noffH.code.virtualAddr + i) % PageSize;
                    if(pageTable[virPageNum].valid)
                    {
                        phyPageNum = pageTable[virPageNum].physicalPage;
                        executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 
                            1, intraFileAddr);
                    }
                    executable->ReadAt(&(tempBuffer[0]), 1, intraFileAddr);
                    swapFile->WriteAt(&(tempBuffer[0]), 1, noffH.code.virtualAddr+i);
                    intraFileAddr++;
                }
            }
            if (noffH.initData.size > 0) 
            {
                intraFileAddr = noffH.initData.inFileAddr;
                for(i=0; i<noffH.initData.size; i++)
                {
                    virPageNum = (noffH.initData.virtualAddr + i) / PageSize;
                    offSet = (noffH.initData.virtualAddr + i) % PageSize;
                    if(pageTable[virPageNum].valid)
                    {
                        printf("pageTable[%d] is valid, so initData write in\n", virPageNum);
                        phyPageNum = pageTable[virPageNum].physicalPage;
                        executable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize + offSet]), 
                            1, intraFileAddr);
                    }
                    executable->ReadAt(&(tempBuffer[0]), 1, intraFileAddr);
                    swapFile->WriteAt(&(tempBuffer[0]), 1, noffH.initData.virtualAddr+i);
                    intraFileAddr++;
                }
            }
            

           
            delete tempBuffer;  
        }
    
    }

}




//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   
   for(int i=0; i<numPages; i++)
   {
        if(pageTable[i].valid)
        {
            memoryBitmap->Clear(pageTable[i].physicalPage);
        }
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
    //printf("in addrspace initregisters, successfully returned\n");
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

    // printf("in restorestate, the new pageTable situation is :\n");
    // printf("there are %d table entries in all\n", numPages);
    // for(int i=0; i< machine->pageTableSize; i++)
    //     printf("virtualPage num is %d, physicalPage num is %d\n", i, machine->pageTable[i].physicalPage);
    // //printf("address space restored!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
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

void 
AddrSpace::Print()
{
    printf("in this AddrSpace, the numPages is %d\n", numPages);
    for(int i=0; i<numPages; i++)
        printf("pageTable[%d] virtualPage is %d, physicalPage is %d\n", 
            i, pageTable[i].virtualPage, pageTable[i].physicalPage);
}

