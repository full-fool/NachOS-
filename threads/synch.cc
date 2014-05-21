// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
    //printf("in semaphore, thread starts to sleep\n");
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	   scheduler->ReadyToRun(thread);
    
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) 
{ 
    name = debugName;
    sem = new Semaphore(name, 1);         
}

Lock::~Lock() 
{
    delete sem;
    delete lockHolder;
}

void 
Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    sem->P();
    lockHolder = currentThread;
    (void) interrupt->SetLevel(oldLevel);
}

void 
Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    ASSERT(isHeldByCurrentThread());
    sem->V();
    lockHolder = NULL;  
    (void) interrupt->SetLevel(oldLevel);
}


bool
Lock::isHeldByCurrentThread()
{
    return (lockHolder == currentThread);
}


Condition::Condition(char* debugName) 
{
    name = debugName;
    //queue = new List;
    conLock = new Semaphore(name, 0);
}

Condition::~Condition() 
{
    //delete queue;
    delete conLock;
}

void 
Condition::Wait(Lock* conditionLock) 
{ 
    //ASSERT(FALSE); 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    ASSERT(conditionLock->isHeldByCurrentThread());
    //queue->Append((void *)currentThread); 
    conditionLock->Release();
    conLock->P();
    conditionLock->Acquire();
    (void) interrupt->SetLevel(oldLevel);

}

void 
Condition::Signal(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    ASSERT(conditionLock->isHeldByCurrentThread());
    //Thread *tempThread = (Thread *)queue->Remove();
    //if(tempThread != NULL)
        //scheduler->ReadyToRun(tempThread);
    conLock->V();
    (void) interrupt->SetLevel(oldLevel);
}

void 
Condition::Broadcast(Lock* conditionLock) 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    while(!conLock->noWaiting())
    {
        conLock->V();
    }
    (void) interrupt->SetLevel(oldLevel);

}

Barrier::Barrier(char *debugName, int _threadNum)
{
    name = debugName;
    threadNum = _threadNum;
    waitingNum = 0;
    innerLock = new Semaphore(name, 0);   
}

Barrier::~Barrier()
{
    delete innerLock;
}

void 
Barrier::Wait()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    if(waitingNum == threadNum - 1)
    {
        while(!innerLock->noWaiting())
        {
            innerLock->V();
        }
        return;
    }
    waitingNum++;
    innerLock->P();
    (void) interrupt->SetLevel(oldLevel);
}


RWLock::RWLock(char *debugName)
{
    name = debugName;
    readerNum = 0;
    isHeldByWriter = false; 
    mutex = new Lock("mutex");
    conLock = new Condition("conLock");
}

RWLock::~RWLock()
{
    delete mutex;
    delete conLock;
}

void
RWLock::AcquireRLock()
{
    
    mutex->Acquire();
    //printf("reader %s acquire the lock\n", currentThread->getName());

    while(isHeldByWriter)
    {
        conLock->Wait(mutex);
    }
    readerNum++;
    mutex->Release();
}


void
RWLock::ReleaseLock()
{
    mutex->Acquire();
    if(isHeldByWriter)
    {
        conLock->Broadcast(mutex);
        mutex->Release();
        isHeldByWriter = false;
        //printf("%s release the lock\n", currentThread->getName());
        return;
    }
    readerNum--;
    //printf("readerNum is %d now\n", readerNum);
    if(readerNum == 0)
    {
        conLock->Broadcast(mutex);
        //printf("%s release the lock\n", currentThread->getName());
    }
    mutex->Release();
}

void 
RWLock::AcquireWLock()
{
    mutex->Acquire();
    while(readerNum != 0 || isHeldByWriter)
    {
        conLock->Wait(mutex);
    }
    isHeldByWriter = true;
    //printf("%s acquire the lock\n", currentThread->getName());
    mutex->Release();
}

int 
RWLock::TryRLock()
{
    if(isHeldByWriter)
        return -1;
    return 0;
}

int 
RWLock::TryWLock()
{
    if(readerNum == 0 && !isHeldByWriter)
        return 0;
    return -1;
}

