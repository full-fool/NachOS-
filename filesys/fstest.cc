// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {	 
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);		
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength, 'f')) {	 // Create Nachos file
	printf("Copy: couldn't create output file %s\n", to);
	fclose(fp);
	return;
    }
    
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    printf("file %s opened successfully\n", to);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
	openFile->Write(buffer, amountRead);	
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *targetPath, char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(targetPath, name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}


void ChangeDirectory(char *newPath)
{
    fileSystem->changeDirectory(newPath);
    printf("now currentPaht is changed to %s\n", newPath);
}


void PrintDirectory()
{
    fileSystem->getCurrentPath();
}
//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 500))

RWLock *Rwlock = new RWLock("rwlock");

static void 
FileWrite()
{
    OpenFile *openFile;    
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
    
    if (!fileSystem->Create("testDirectory", 0, 'd', "/")) {
      printf("Perf test: can't create directory");
      //return;
    }

    
    if (!fileSystem->Create("fileUnderDirectory", 0, 'f', "/testDirectory/")) {
      printf("Perf test: can't create directory file");
      //return;
    }
    

    if (!fileSystem->Create(FileName, 0, 'f', "/")) {
      printf("Perf test: can't create %s\n", FileName);
      //return;
    }
    //printf("reach here\n");
    openFile = fileSystem->Open("/", FileName);
    if (openFile == NULL) {
	printf("Perf test: unable to open %s\n", FileName);
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
        //printf("i is %d and \n", );
	   if (numBytes < 10) {
	       printf("Perf test: unable to write %s\n", FileName);
	       delete openFile;
	       return;
	   }
    }
    //printf("write succeeded!!!!!!\n");
    delete openFile;	// close file
}

static void 
FileRead()
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);



    if ((openFile = fileSystem->Open(FileName)) == NULL) {
	printf("Perf test: unable to open file %s\n", FileName);
	delete [] buffer;
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) 
    {
        numBytes = openFile->Read(buffer, ContentSize);
	   if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) 
       {
    	    printf("Perf test: unable to read %s\n", FileName);
    	    delete openFile;
    	    delete [] buffer;
    	    return;
	   }
    }
    delete [] buffer;
    delete openFile;	// close file
}


void testFunction()
{
    OpenFile *openFile;    
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
    FileSize, ContentSize);
    fileSystem->getCurrentPath();
    
    if (!fileSystem->Create("testDirectory", 0, 'd')) {
      printf("Perf test: can't create directory\n");
      //return;
    }
    //fileSystem->Print();
    
    if (!fileSystem->Create("fileUnderDirectory", 0, 'f', "/testDirectory/")) {
      printf("Perf test: can't create directory file\n");
      //return;
    }
    //fileSystem->Print();

    
    

    if (!fileSystem->Create(FileName, 0, 'f')) {
      printf("Perf test: can't create %s\n", FileName);
      //return;
    }
    //printf("reach here\n");
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
    printf("Perf test: unable to open %s\n", FileName);
    return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
        //printf("i is %d and \n", );
       if (numBytes < 10) {
           printf("Perf test: unable to write %s\n", FileName);
           delete openFile;
           return;
       }
    }

    delete openFile;    
}





void 
fileThread(int arg)
{
    if(arg == 1)
    {
        OpenFile *openFile;    
        int i, numBytes;
        char *buffer = new char[5];
        printf("thread 1 is running\n");
        openFile = fileSystem->Open("exclusiveFile");
        printf("thread 1 successfully open file\n");
        Rwlock->AcquireRLock();
        numBytes = openFile->Read(buffer, 4);
        buffer[4] = '\0';
        int filethreadsnum = fileSystem->getFileThreadsNum("/", "exclusiveFile");
        //printf("the filethreadsnum is %d\n", filethreadsnum);
        Rwlock->ReleaseLock();
        //fileSystem->Remove("/", "exclusiveFile");
        printf("thread 1 read content \"%s\"\n", buffer);
        currentThread->Yield();
        fileSystem->Close("/", "exclusiveFile");

    }
    
    else if(arg == 2)
    {
        OpenFile *openFile;    
        int i, numBytes;
        char *buffer = new char[6];
        printf("thread 2 is running\n");
        openFile = fileSystem->Open("exclusiveFile");
        Rwlock->AcquireRLock();
        numBytes = openFile->Read(buffer, 5);
        buffer[5] = '\0';
        Rwlock->ReleaseLock();
        printf("thread 2 read content \"%s\"\n", buffer);
        fileSystem->Close("/", "exclusiveFile");

    }
    else if(arg == 3)
    {
        fileSystem->Remove("/", "exclusiveFile");
    }
    

}

void FileExclusive()
{
    bool success = fileSystem->Create("exclusiveFile", 128, 'f', "/");
    if(!success)
    {
        printf("the file exclusiveFile already exist\n");
        fileSystem->Remove("/", "exclusiveFile");
        success = fileSystem->Create("exclusiveFile", 128, 'f', "/");
    }
    if(!success)
    {
        printf("cannot create file exclusiveFile\n");
        return;
    }
    OpenFile *openFile = fileSystem->Open("exclusiveFile");
    int numBytes = openFile->Write("9876543210", 10);
    if(numBytes < 10)
        printf("Perf test: unable to write exclusiveFile\n");
    fileSystem->Close("/", "exclusiveFile");
    Thread *Thread1 = new Thread("Thread1");
    Thread *Thread2 = new Thread("Thread2");
    //Thread *Thread3 = new Thread("Thread3");
    Thread1->Fork(fileThread, 1);
    Thread2->Fork(fileThread, 2);
    //Thread3->Fork(fileThread, 3);


}


void
PerformanceTest()
{
    //printf("Starting file system performance test:\n");
    //stats->Print();
    //FileWrite();
    FileExclusive();
    //testFunction();
    /*
    FileRead();
    fileSystem->Print();
    
    if (!fileSystem->Remove(FileName)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }
    stats->Print();
    */
    
    
}

