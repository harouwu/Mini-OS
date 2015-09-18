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
#include <time.h>
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
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    int leftSectors;
    int leftSecondBlock;
    int totalSectors;
    printf("numSectors: %d\n", numSectors);
    if (numSectors <= FirstIndex)
        totalSectors = numSectors;
    else {
        leftSectors = numSectors - FirstIndex;
        leftSecondBlock = leftSectors / SecondDirect;
        if (leftSectors % SecondDirect != 0)
            leftSecondBlock += 1;
        if (leftSecondBlock > SecondIndex)
            return FALSE;
        totalSectors = numSectors + leftSecondBlock;
        printf("leftSecondBlock: %d, totalSectors: %d\n", leftSecondBlock, totalSectors);
    }
    if (freeMap->NumClear() < totalSectors)
	return FALSE;		// not enough space
    if (numSectors <= FirstIndex) {
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
    }
    else {
         for (int i = 0; i < FirstIndex; i++)
            dataSectors[i] = freeMap->Find();
        for (int j = 0; j < leftSecondBlock; ++j) {
            dataSectors[FirstIndex + j] = freeMap->Find();
            int cnt;
            if (leftSectors <= 0)
                break;
            if (leftSectors > SecondDirect)
                cnt = SecondDirect;
            else
                cnt = leftSectors;
            int *sector = new int[cnt];
            for (int k = 0; k < cnt; ++k) {
                sector[k] = freeMap->Find();
            }
            synchDisk->WriteSector(dataSectors[FirstIndex + j], (char *)sector);
            delete sector;
            leftSectors -= SecondDirect;
        }
    }
    freeMap->Print();
    setCreateTime();
    printf("File created! CreateTime: %s\n", createTime);
    setLastAccessTime();
    setLastModifiedTime();
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
    if (numSectors <= FirstIndex) {
        for (int i = 0; i < numSectors; i++) {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    }
    else {
        int leftSectors = numSectors - FirstIndex;
        int leftSecondBlock = leftSectors / SecondDirect;
        if (leftSectors % SecondDirect != 0)
            leftSecondBlock += 1;
         for (int i = 0; i < FirstIndex; i++) {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
         }
        for (int j = 0; j < leftSecondBlock; ++j) {
            char *sectorTmp = new char[SectorSize];
            synchDisk->ReadSector(dataSectors[FirstIndex + j], sectorTmp);
            int *sector = (int *) sectorTmp;
            int cnt;
            if (leftSectors <= 0)
                break;
            if (leftSectors > SecondDirect)
                cnt = SecondDirect;
            else
                cnt = leftSectors;
            for (int k = 0; k < cnt; ++k) {
                ASSERT(freeMap->Test((int)sector[k]));  // ought to be marked!
                freeMap->Clear((int)sector[k]);
            }
            delete sectorTmp;
            ASSERT(freeMap->Test((int)dataSectors[FirstIndex + j]));  // ought to be marked!
            freeMap->Clear((int)dataSectors[FirstIndex + j]);
            leftSectors -= SecondDirect;
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
    // printf("FileHeader WriteBack! lastModifiedTime: %s\n", lastModifiedTime);
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
    int sectorOffset = offset / SectorSize;
    if (sectorOffset < FirstIndex)
        return dataSectors[sectorOffset];
    else {
        int leftSectors = sectorOffset - FirstIndex;
        int index = leftSectors / SecondDirect;
        int indexOffset = leftSectors % SecondDirect;
        char *sectorTmp = new char[SectorSize];
        synchDisk->ReadSector(dataSectors[FirstIndex + index], sectorTmp);
        int *sector = (int *) sectorTmp;
        int ans = sector[indexOffset];
        ASSERT(ans > 0);
        delete sectorTmp;
        return ans;
    }
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
    printf("createTime:%s\n", createTime);
    printf("lastAccessTime:%s\n", lastAccessTime);
    printf("lastModifiedTime:%s\n", lastModifiedTime);

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

void
FileHeader::setCreateTime()
{
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char *tmp = asctime(timeinfo);
    strncpy(createTime, tmp, TimeLen);
}

void
FileHeader::setLastAccessTime()
{
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char *tmp = asctime(timeinfo);
    strncpy(lastAccessTime, tmp, TimeLen);
}

void
FileHeader::setLastModifiedTime()
{
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char *tmp = asctime(timeinfo);
    strncpy(lastModifiedTime, tmp, TimeLen);
}

bool
FileHeader::externLength(BitMap *freeMap, int size)
{
    int needSectors  = divRoundUp(size, SectorSize);
    int totalSectors = numSectors + needSectors;
    if (totalSectors > TotalDirect)
        return FALSE;
    if (freeMap->NumClear() < needSectors)
        return FALSE;

    printf("numSectors: %d, needSectors:%d\n", numSectors, needSectors);
    // calculate lastSecondBlock and lastIndex
    int lastSecondBlock;
    int lastIndex;
    if (numSectors <= FirstIndex) {
        lastSecondBlock = 0;
        lastIndex = 0;
    }
    else {
        lastSecondBlock = (numSectors - FirstIndex) / SecondDirect;
        lastIndex = (numSectors - FirstIndex) % SecondDirect;
    }
  
    int leftSectors = needSectors;
    for (int i = 0; i < needSectors; ++i) {
        if (leftSectors <= 0)
            break;
        // append in the first index
        if (numSectors + i < FirstIndex) {
            printf("appending in the first index %d\n", numSectors + i);
            dataSectors[numSectors + i] = freeMap->Find();
            leftSectors--;
        }
        else {
            if (lastIndex == 0) { // prepare for new second index block
                dataSectors[FirstIndex + lastSecondBlock] = freeMap->Find();
                printf("New Second Index Block! Point to %d\n",  
                    dataSectors[FirstIndex + lastSecondBlock]);
                int *sector = new int[SecondDirect];
                synchDisk->WriteSector(dataSectors[FirstIndex + lastSecondBlock], (char *)sector);
                delete sector;
            }
            // append in the second index block
            for (; leftSectors > 0; lastSecondBlock++) {
                char *sectorTmp = new char[SectorSize];
                synchDisk->ReadSector(dataSectors[FirstIndex + lastSecondBlock], sectorTmp);
                int *tmp = (int *)sectorTmp;
                for (int k = lastIndex; k < SecondDirect; ++k) {
                    tmp[k] = freeMap->Find();
                    leftSectors -= 1;
                    if (leftSectors <= 0)
                        break;
                }
                synchDisk->WriteSector(dataSectors[FirstIndex + lastSecondBlock], sectorTmp);
                if (leftSectors > 0) {
                    lastIndex = 0;
                    dataSectors[FirstIndex + lastSecondBlock + 1] = freeMap->Find();
                    printf("New Second Index Block! Point to %d\n",  
                        dataSectors[FirstIndex + lastSecondBlock + 1]);
                    int *sector = new int[SecondDirect];
                    synchDisk->WriteSector(dataSectors[FirstIndex + lastSecondBlock + 1], (char *)sector);
                    delete sector;
                }
            }
        }
    }
    numSectors += needSectors;
    numBytes += SectorSize * needSectors;
    OpenFile *freeMapFile = new  OpenFile(0);
    freeMap->WriteBack(freeMapFile);
}