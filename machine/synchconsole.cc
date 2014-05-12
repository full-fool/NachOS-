// synchconsole.cc 
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "console.h"
#include "system.h"
#include "synch.h"

static void ReadAvail(int arg)
{
	readAvail->V();
}

static void WriteDone(int arg)
{
	writeDone->V();
}

SynchConsole::SynchConsole(char *in, char *out)
{
	concole = new Console(in, out, ReadAvail, WriteDone, 0);
	readLock = new Lock("readLock");
	writeLock = new Lock("writeLock");
	readAvail = new Semaphore("readAvail", 0);
	writeDone = new Semaphore("writeDone", 0);
}

SynchConsole::~SynchConsole()
{

}

void
SynchConsole::putChar(char ch)
{
	writeLock->Acquire();
	console->putChar(ch);
	writeDone->P();
	writeLock->Release();
}

char 
SynchConsole::getChar()
{
	readLock->Acquire();
	readAvail->P();
	char c = console->getChar();
	readLock->Release();
	return c;


}

