// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

#define MAX_BUFFER 1
#define MAX_PRODUCT 10
int buffer[20];
Semaphore *full = new Semaphore("full", 0);
Semaphore *empty = new Semaphore("empty", MAX_BUFFER);
Lock *mutex = new Lock("mutex");
Condition *condp = new Condition("condp");
Condition *condc = new Condition("condc");
Barrier *barrierTest = new Barrier("barrier", 3);
int change = 0;
RWLock *rwlock = new RWLock("rwlock");
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    //for (num = 0; num < 5; num++) {
	printf("*** thread %d with priority %d level is running\n", currentThread->getTid(), 
        currentThread->getPriority());
        currentThread->Yield();
    //}
}

void
ThreadStatus()
{
    printf("TID\t\tThreadName\t\tThreadStatus\n");
          for(int i=0; i<MAX_THREAD_NUM; i++)
            {
                if(threads[i] != NULL)
                {
                    printf("%d\t\t%s\t\t", threads[i]->getTid(), threads[i]->getName());
              //printf("Thread %s with tid %d is in status ", threads[i]->getName(), threads[i]->getTid());
                    //enum ThreadStatus { JUST_CREATED, RUNNING, READY, BLOCKED };

                    switch(threads[i]->getStatus())
                    {
                        case JUST_CREATED:
                            printf("JUST_CREATED\n");
                            break;
                        case RUNNING:
                            printf("RUNNING\n");
                            break;
                        case READY:
                            printf("READY\n");
                            break;
                        default:
                            printf("BLOCKED\n");
                            break; 
                    }
                }
            }
          return;
}


//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1(int n)
{
    DEBUG('t', "Entering ThreadTest1");
    Thread *temp = NULL;
    //printf("Thread with tid %d is running with priority %d now\n", currentThread->getTid(), currentThread->getPriority());
    for (int i =0 ;i < 10; i++)
    {
        temp = new Thread("forked thread!");
        //printf("loop %d after new and the tidUse[1] is %d\n", i, tidUse[1]);

        temp->setPriority(i%5+1);
        //printf("loop %d after setPriority and the tidUse[1] is %d\n", i, tidUse[1]);

        temp->Fork(SimpleThread,1);

        //printf("loop %d finished and the tidUse[1] is %d\n", i, tidUse[1]);
    }
    //Thread *t = new Thread("forked thread");

    //t->Fork(SimpleThread, 1);
    //SimpleThread(0);
    if(n==1)
        ThreadStatus();

}



void
ProducerWithSem(int temp)
{
    for(int i=0; i<MAX_PRODUCT; i++)
    {
        //printf("pro i %d\n", i);
        empty->P();
        mutex->Acquire();
        buffer[0] = i;
        printf("Producer produce product %d\n", buffer[0]);
        mutex->Release();
        full->V();
    }
}


void 
ConsumerWithSem(int temp)
{
    for(int i=0; i<MAX_PRODUCT; i++)
    {
        //printf("con i %d\n", i);
        full->P();
        mutex->Acquire();
        printf("Consumer consume product %d\n", buffer[0]);
        

        mutex->Release();
        empty->V();

    }
}

void 
ProCom1()
{
    memset(buffer, 0, sizeof(buffer));
    Thread *Producer = new Thread("Producer");
    Producer->Fork(ProducerWithSem, 0);
    Thread *Consumer = new Thread("Consumer");
    Consumer->Fork(ConsumerWithSem, 0);
    return;
}

void 
ProducerWithCond(int temp)
{
    for(int i=0; i<MAX_PRODUCT; i++)
    {
        mutex->Acquire();
        while(buffer[0] != 0)
        {
            condp->Wait(mutex);
            printf("producer waitint\n");
        }
        printf("Producer produce product %d\n", i);
        buffer[0] = 1;
        condc->Signal(mutex);
        mutex->Release();
    }
}

void 
ConsumerWithCond(int temp)
{
    for(int i=0; i<MAX_PRODUCT; i++)
    {
        mutex->Acquire();
        while(buffer[0] == 0)
        {
            condc->Wait(mutex);
            printf("consumer waiting\n");
        }
        printf("Consumer consume product %d\n", i);
        buffer[0] = 0;
        condp->Signal(mutex);
        mutex->Release();
    }
}

void 
Procom2()
{
    Thread *Producer = new Thread("Producer");
    Producer->Fork(ProducerWithCond, 0);
    Thread *Consumer = new Thread("Consumer");
    Consumer->Fork(ConsumerWithCond, 0);
    return;
}

void 
Barrier1(int temp)
{
    for (int i=0; i<2; i++)
    {
        printf("barrier1 loop to %d\n", i);
    }
    barrierTest->Wait();
    printf("barrier1 ended\n");
}


void 
Barrier2(int temp)
{
    for (int i=0; i<3; i++)
    {
        printf("barrier2 loop to %d\n", i);
    }
    barrierTest->Wait();
    printf("barrier2 ended\n");
}

void
Barrier3(int temp)
{
    for (int i=0; i<4; i++)
    {
        printf("barrier3 loop to %d\n", i);
    }
    barrierTest->Wait();
    printf("barrier3 ended\n");
}

void
testBarrier()
{
    Thread *barrier1 = new Thread("barrier1");
    barrier1->Fork(Barrier1, 0);
    Thread *barrier2 = new Thread("barrier2");
    barrier2->Fork(Barrier2, 0);
    Thread *barrier3 = new Thread("barrier3");
    barrier3->Fork(Barrier3, 0);

}


void
writerTest(int temp)
{
    //printf("writer reach here\n");
    
    rwlock->AcquireWLock();
    change =  1;
    printf("writer here, and change is %d now\n", change);
    rwlock->ReleaseLock();
}

void 
readerTest(int temp)
{
    rwlock->AcquireRLock();
    printf("reader %d here, and change is %d\n", temp, change);
    rwlock->ReleaseLock();


}


void 
rwlockTest()
{
    Thread *reader1 = new Thread("reader1");
    Thread *reader2 = new Thread("reader2");
    Thread *writer = new Thread("writer");
    Thread *reader3 = new Thread("reader3");
    reader1->Fork(readerTest, 1);
    reader2->Fork(readerTest, 2);
    writer->Fork(writerTest, 0);
    reader3->Fork(readerTest, 3);
    //printf("read here\n");

}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest(int n)
{
    //printf("ThreadTest started!\n");
    switch (testnum) {
    case 1:
	   ThreadTest1(n);
	   break;
    case 2:
        ProCom1();
        break;
    case 3:
        Procom2();
        break;
    case 4:
        testBarrier();
        break;
    case 5:
        rwlockTest();
        break;
    default:
	   printf("No test specified.\n");
	   break;
    }
    //printf("ThreadTest ended!\n");
}

