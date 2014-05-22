// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize, int thisSector)
{ 
    //hdrSector = thisSector;
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	   return FALSE;		// not enough space
    if(numSectors > 29+32) // exceed the max file limitation
        return FALSE;
    if(numSectors <= 29)
    {
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
        dataSectors[29] = -1;
        //printf("numSectors is small and dataSectors[29] is set to %d, hdrSector is %d\n", 
          //  dataSectors[29], hdrSector);
    }
    else
    {
        for (int i = 0; i < 30; i++)
            dataSectors[i] = freeMap->Find();
        int secondSector = dataSectors[29];
        int secondNeedSec = numSectors - 29;
        int *secondIndex = new int[secondNeedSec];
        for(int i=0; i<secondNeedSec; i++)
        {
            secondIndex[i] = freeMap->Find();
        }
        synchDisk->WriteSector(secondSector, (char *)secondIndex); 
    }

    
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if(dataSectors[29] == -1)       //2-level index not used
    {
       for (int i = 0; i < numSectors; i++) 
       {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        } 
    }
    else                                            //2-level index used
    {
        int *secondIndex;
        char *temp = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[29], temp);
        secondIndex = (int *)temp;
        for(int i=0; i<numSectors-29; i++)          //clear 2-level index first
        {
            ASSERT(freeMap->Test((int) secondIndex[i]));
            freeMap->Clear((int) secondIndex[i]);
        }
        for(int i=0; i<30; i++)                     //then clear 1-level index
        {
            ASSERT(freeMap->Test((int) dataSectors[i]));
            freeMap->Clear((int) dataSectors[i]);
        }

    }
    
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int firstSector = offset / SectorSize;
    if(firstSector < 29)                //2-level index not used
        return(dataSectors[offset / SectorSize]);
    else
    {
        ASSERT(dataSectors[29] != -1);
        int *secondIndex;
        char *temp = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[29], temp);
        secondIndex = (int *)temp;
        //printf("in 2-level index offset %d is in phy sector number %d\n", offset, secondIndex[firstSector - 29]);
        return(secondIndex[firstSector - 29]);
    }

    //return(dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	   printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

//----------------------------------------------------------------------
// FileHeader::EnlargeFile
//  Enlarge the file with bytesNeeded more bytes.
//----------------------------------------------------------------------

bool 
FileHeader::EnlargeFile(BitMap *freeMap, int bytesNeeded)
{
    int newSectors = divRoundUp(bytesNeeded, SectorSize);
    printf("in FileHeader::EnlargeFile, newSectors is %d and numSectors is %d\n",
    newSectors, numSectors);
    //printf("the sector num is %d and situation is \n", getSector());
    //for(int i=0; i<30; i++)
    //   printf("%d\n", dataSectors[i]);
    //printf("the dataSectors[29] is %d\n", dataSectors[29]);
    if(freeMap->NumClear() < newSectors)
        return FALSE;
    if(newSectors + numSectors >29+32)
        return FALSE;
    if(newSectors + numSectors > 29)            //need to allocate new space in 2-level index
    {
        if(numSectors >29)                  //already uses a 2-level index
        {
            ASSERT(dataSectors[29] != -1);
            int *secondIndex;
            char *temp = new char[SectorSize];
            synchDisk->ReadSector(dataSectors[29], temp);
            secondIndex = (int *)temp;
            for(int i=numSectors-29; i<newSectors+numSectors-29; i++)          //clear 2-level index first
            {
                secondIndex[i] = freeMap->Find();
                printf("secondIndex[%d] is %d\n", i, secondIndex[i]);
            }
            synchDisk->WriteSector(dataSectors[29], (char *)secondIndex); 
            numSectors += newSectors;
            numBytes += bytesNeeded;
            printf("in situation 1 and now numSectors is %d, newSectors is %d, numBytes is %d\n", 
                numSectors, newSectors, numBytes);
        }
        else
        {
            ASSERT(dataSectors[29] == -1);
            for(int i=numSectors; i<30; i++)
            {
                dataSectors[i] = freeMap->Find();
            }
            int secondSector = dataSectors[29];
            printf("in situation 2 new secondSector found is sector %d\n",secondSector);
            int secondNeedSec = numSectors + newSectors - 29;
            int *secondIndex = new int[secondNeedSec];
            for(int i=0; i<secondNeedSec; i++)
            {
                secondIndex[i] = freeMap->Find();
                printf("secondIndex[%d] is %d\n", i, secondIndex[i]);
            }
            synchDisk->WriteSector(secondSector, (char *)secondIndex); 
            numSectors += newSectors;
            numBytes += bytesNeeded;
            printf("in situation 2 and now numSectors is %d, newSectors is %d, numBytes is %d\n", 
                numSectors, newSectors, numBytes);


        }
        return TRUE;
    }
    else                                    //no need to use 2-level index
    {
        ASSERT(dataSectors[29] == -1);
        for(int i=numSectors; i<numSectors+newSectors; i++)
        {
            dataSectors[i] = freeMap->Find();
            printf("in situation 3 new sector found is %d\n", dataSectors[i]);
        }
        numSectors += newSectors;
        numBytes += bytesNeeded;
        printf("in situation 3 and now numSectors is %d, newSectors is %d, numBytes is %d\n", 
            numSectors, newSectors, numBytes);

        return TRUE;
    }
    return FALSE;


}