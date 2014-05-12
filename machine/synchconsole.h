// synchconsole.h 
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYNCHCONSOLE_H
#define SYNCHCONSOLE_H

#include "copyright.h"
#include "utility.h"
#include "console.h"
#include "synch.h"


class SynchConsole {
public:
	SynchConsole(char *in, char *out);
	~SynchConsole();
	void putChar(char ch);
	char getChar();

private:
	Console *console;
	Lock *readLock;
	Lock *writeLock;
	Semaphore *readAvail;
	Semaphore *writeDone
  
};

#endif // SYNCHCONSOLE_H
