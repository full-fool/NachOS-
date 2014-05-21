// directory.h 
//	Data structures to manage a UNIX-like directory of file names.
// 
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and 
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 		127	// for simplicity, we assume 
					// file names are <= 9 characters long

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.

class DirectoryEntry {
  public:
    bool inUse;				// Is this directory entry in use?
    int sector;				// Location on disk to find the 
					//   FileHeader for this file 
    char name[FileNameMaxLen + 1];	// Text name for file, with +1 for 
					// the trailing '\0'
    //char *name; 
    char type;          //'f' denotes file, 'd' denotes directory
    char createTime[25];
    char lastVisited[25];
    char lastModified[25];
    char path[100];
    int threadNumber;

};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk. 

class Directory {
  public:
    Directory(int size); 		// Initialize an empty directory
					// with space for "size" files
    ~Directory();			// De-allocate the directory

    void FetchFrom(OpenFile *file);  	// Init directory contents from disk
    void WriteBack(OpenFile *file);	// Write modifications to 
					// directory contents back to disk

    int Find(char *targetPath, char *name);		// Find the sector number of the 
					// FileHeader for file: "name"

    bool Add(char *name, int newSector, char type, char *targetPath);  // Add a file name into the directory

    bool Remove(char *targetPath, char *name);		// Remove a file from the directory

    void List();			// Print the names of all the files
					//  in the directory
    void Print();			// Verbose print of the contents
					//  of the directory -- all the file
					//  names and their contents.
    //char *getCurrentPath(){return currentPath;}
    void PrintTableEntry(){
        for(int i=0; i<tableSize; i++)
            printf("entry name is %s and sector is %d\n", table[i].name, table[i].sector);

    }
    void addThread(char *targetPath, char *name);
    void subThread(char *targetPath, char *name);
    int getThread(char *targetPath, char *name);
    void updateTime(char needUpdate[]);
    void updateVisitedTime(char *targetPath, char *name);
    void updateModifiedTime(int sector);
    void updateModified(char *targetPath, char *name);
    bool existDirectory(char *fullPath);
    int getTableSize(){return tableSize;}
    //bool changeDirectory(char *newPath);

  private:
    int tableSize;			// Number of directory entries
    DirectoryEntry *table;		// Table of pairs: 
					// <file name, file header location> 
    //char *currentPath;

    int FindIndex(char *targetPath, char *name);		// Find the index into the directory 
					//  table corresponding to "name"
};

#endif // DIRECTORY_H
