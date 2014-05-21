// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include "time.h"
#include "memory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    //currentPath = "/";
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *targetPath, char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen) &&  !strcmp(targetPath, table[i].path))
	    return i;
    return -1;		// name not in directory
    // for(int i=0; i<tableSize; i++)
    // {
    //     if(table[i].inUse && strcmp(table[i].path, targetPath) && 
    //         strcmp(table[i].name, name))
    //     return i;
    // }
    // return -1;
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *targetPath, char *name)
{
    int i = FindIndex(targetPath, name);

    if (i != -1)
    {
        //updateTime(table[i].lastVisited);
        return table[i].sector;
    }
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector, char type, char *targetPath)
{ 
    
    for(int i=0; i<tableSize; i++)
    {
        if(table[i].inUse && !strcmp(table[i].path, targetPath) && 
             !strcmp(table[i].name, name))
            return FALSE;
    }

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen); 
            //table[i].name = name;
            //table[i].path = targetPath;
            strncpy(table[i].path, targetPath, 100);
            table[i].type = type;
            table[i].sector = newSector;
            table[i].threadNumber = 0;
            updateTime(table[i].createTime);
            updateTime(table[i].lastVisited);
            updateTime(table[i].lastModified);
            printf("file named %s has been successfully added to table[%d]\n", name, i);

            return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *targetPath, char *name)
{ 
    // int index = FindIndex(targetPath, name);
    // if (index == -1)
    //     return FALSE;       // name not in directory
    int index = -1;
    for(int i=0; i<tableSize; i++)
    {
        if(table[i].inUse && !strcmp(table[i].path, targetPath) && 
             !strcmp(table[i].name, name))
        {
            index = i;
            break;
        }
    }
    if(index == -1)
        return FALSE;
    if(table[index].type == 'f')            //delete a file
    {
        table[index].inUse = FALSE;
        return TRUE; 
    }
    else                                    //delete a directory
    {
        char *fullPath = new char[100];
        memset(fullPath, 0, sizeof fullPath);
        strcat(fullPath, targetPath);
        strcat(fullPath, name);
        strcat(fullPath, "/");
        int len = strlen(fullPath);
        table[index].inUse = FALSE;
        for(int i=0; i<tableSize; i++)
        {
            if(table[i].inUse && !strncmp(table[i].path, fullPath, len))
            {
               table[i].inUse = FALSE;
            }
        }
        delete fullPath;
    }
    

    
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse)
	    printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
    {
        //printf("now is is %d\n", i);
        if (table[i].inUse) 
        {
            //printf("and now table[%d] is inUse\n", i);
            printf("Name: %s\n", table[i].name);
            printf("Path: %s\n", table[i].path);
            printf("Sector: %d\n", table[i].sector);
            //printf("Name: %s, Path: %s, Sector: %d\n", table[i].name, table[i].path, table[i].sector);
            printf("createTime: %s, lastVisited: %s, lastModified %s\n", 
                table[i].createTime, table[i].lastVisited, table[i].lastModified);
            hdr->FetchFrom(table[i].sector);
            printf("size is %d\n", hdr->getNumBytes());
            //hdr->Print();
        }
    }
	
    printf("\n");
    delete hdr;
}


 void 
 Directory::addThread(char *targetPath, char *name)
 {
    int i = FindIndex(targetPath, name);
    table[i].threadNumber++;
 }

void 
Directory::subThread(char *targetPath, char *name)
{
    int i = FindIndex(targetPath, name);
    table[i].threadNumber--;
}

int 
Directory::getThread(char *targetPath, char *name)
{
    int i = FindIndex(targetPath, name);
    return table[i].threadNumber;
}

void 
Directory::updateTime(char needUpdate[])
{
    time_t rawTime;
    tm *timeInfo;
    time(&rawTime);
    timeInfo = localtime(&rawTime);
    char *temp = asctime(timeInfo);
    strncpy(needUpdate, temp, 24);
}

void 
Directory::updateVisitedTime(char *targetPath, char *name)
{
    int i = FindIndex(targetPath, name);
    updateTime(table[i].lastVisited);
}

void 
Directory::updateModifiedTime(int sector)
{
    int index;
    for (int i = 0; i < tableSize; i++)
    {
        if (table[i].inUse && table[i].sector == sector)
        {
            index = i;
            break;
        }
    }
    updateTime(table[index].lastModified);  
}

void 
Directory::updateModified(char *targetPath, char *name)
{
    int i = FindIndex(targetPath, name);
    updateTime(table[i].lastModified);
}

bool 
Directory::existDirectory(char *fullPath)
{
    if(!strcmp("/", fullPath))
    {
        return TRUE;
        printf("path is / and already existed\n");
    }

    char *newPath = new char[100];
    for(int i=0; i<tableSize; i++)
    {
        if(table[i].inUse && table[i].type == 'd')
        {
            
            memset(newPath, 0, sizeof newPath);
            strcat(newPath, table[i].path);
            strcat(newPath, table[i].name);
            strcat(newPath, "/");
            printf("new path is %s now\n", newPath);
            if(!strcmp(newPath, fullPath))
            {
                printf("new path is %s and fullPath is %s, they equal\n",newPath, fullPath);
                delete newPath;
                return TRUE;
            }
        }
    }
    printf("no such directory!\n");
    delete newPath;
    return FALSE;
}


/*
bool 
Directory::changeDirectory(char *newPath)
{
    if(!strcmp("/", newPath))
    {
        currentPath = "/";
        return TRUE;
    }
    char *fullPath = new char[100];
    for(int i=0; i<tableSize; i++)
    {
        if(table[i].inUse && table[i].type == 'd')
        {
            memset(fullPath, 0, sizeof fullPath);
            strcat(fullPath, table[i].path);
            strcat(fullPath, "/");
            strcat(fullPath, table[i].name);
            if(!strcmp(fullPath, newPath))
            {
                currentPath = newPath;
                delete fullPath;
                return TRUE;
            }
        }
    }
    printf("no such directory!\n");
    delete fullPath;
    return FALSE;
}

*/


