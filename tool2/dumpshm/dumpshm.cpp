/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <idl.h>
#include <act.h>
#include <idu.h>
#include <idp.h>
#include <iduMemMgr.h>
#include <idtContainer.h>
#include <idtBaseThread.h>
#include <ideErrorMgr.h>
#include <iduMemMgr.h>
#include <iduShmPersMgr.h>

iduShmPersSys*          gSys = NULL;
iduShmPersData*         gData[IDU_SHM_MAX_CHUNKS];
SInt                    gIDs[IDU_SHM_MAX_CHUNKS];
SInt                    gSysKey;
iduMemClientInfo        gClientInfo[IDU_MEM_UPPERLIMIT];

void dumpSys(SInt aDumpLevel)
{
    iduShmPersChunk* sChunk = (iduShmPersChunk*)gSys;
    iduShmPtr*       sPtr = &(gSys->mSentinel);

    if(aDumpLevel == 0)
    {
        idlOS::printf("Chunk %d(0x%06X/%d) "
                      "%llu(%dM + %dk)\n",
                      sChunk->mID,
                      sChunk->mKey, gIDs[sChunk->mID],
                      sChunk->mSize,
                      sChunk->mSize / (1024 * 1024),
                      (sChunk->mSize / 1024) % 1024);
    }
    else
    {
        idlOS::printf("Chunk Size  : %d(%dM + %dk)\n", sChunk->mSize,
                      sChunk->mSize / (1024 * 1024), (sChunk->mSize / 1024) % 1024);
        idlOS::printf("Key/ID      : %d(%06X)/%d\n",
                      sChunk->mKey, sChunk->mKey, gIDs[sChunk->mID]);
        idlOS::printf("Signature   : %s\n", sChunk->mSignature);

        if(aDumpLevel >= 2)
        {
            idlOS::printf("%-30s\tOffset  \tBlock Size\tUsed\tPrevUsed\n", "Name");
            idlOS::printf("%-30s\t%08X\t%10u\t%4s\t%8s\n",
                          "HEADER", 0, sPtr->mLeftSize, "-", "-");
            idlOS::printf("%-30s\t%08X\t%10u\t%4s\t%8s\n",
                          "SENTINEL",
                          sPtr->mOffset,
                          sPtr->mSize,
                          "-", "-");
        }
    }
}

void dumpChunk(iduShmPtr* aPtr, SInt aDumpLevel)
{
    iduShmPersChunk* sChunk;

    idBool  sUsed;
    ULong   sSize;
    SInt    sIndex;
    ULong   sOffset;
    ULong   sUsedSize;
    ULong   sFreeSize;

    sChunk = (iduShmPersChunk*)aPtr->getLeft(); /* Chunk Header */

    if(aDumpLevel == 0)
    {
        idlOS::printf("Chunk %d(0x%06X/%d) "
                      "%llu(%dM + %dk) ",
                      sChunk->mID,
                      sChunk->mKey, gIDs[sChunk->mID],
                      sChunk->mSize,
                      sChunk->mSize / (1024 * 1024),
                      (sChunk->mSize / 1024) % 1024);
    }
    else
    {
        idlOS::printf("Chunk Size  : %d(%dM + %dk)\n", sChunk->mSize,
                      sChunk->mSize / (1024 * 1024), (sChunk->mSize / 1024) % 1024);
        idlOS::printf("Key/ID      : %d(%06X)/%d\n",
                      sChunk->mKey, sChunk->mKey, gIDs[sChunk->mID]);
        idlOS::printf("Signature   : %s\n", sChunk->mSignature);

        if(aDumpLevel >= 2)
        {
            idlOS::printf("%-30s\tOffset  \tBlock Size\tUsed\tPrevUsed\n", "Name");
            idlOS::printf("%-30s\t%08X\t%10u\t%4s\t%8s\n",
                          "HEADER", 0, aPtr->mLeftSize, "-", "-");
        }
    }

    sFreeSize = 0;
    sUsedSize = aPtr->mLeftSize;

    while(1)
    {
        switch(aDumpLevel)
        {
        case 0:
        case 1:
            if(aPtr->getUsed() == ID_TRUE)
            {
                sUsedSize += aPtr->mSize;
            }
            else
            {
                sFreeSize += aPtr->mSize;
            }
            aPtr = (iduShmPtr*)((ULong)aPtr + aPtr->mSize);
            break;

        case 2:
            sUsed   = aPtr->getUsed();
            sSize   = 0;
            sOffset = aPtr->mOffset;

            while((sUsed == aPtr->getUsed()) && (aPtr->mIndex != IDU_MEM_SENTINEL))
            {
                sSize += aPtr->mSize;
                aPtr = (iduShmPtr*)((ULong)aPtr + aPtr->mSize);
            }

            idlOS::printf("%-30s\t%08llX\t%10llu\t%4s\t%8s\n",
                          (sUsed == ID_TRUE)? "USED":"FREE",
                          sOffset, sSize, "-", "-");
            break;

        case 3:
            sIndex  = aPtr->mIndex;
            sSize   = 0;
            sOffset = aPtr->mOffset;

            while((sIndex == aPtr->mIndex) && (aPtr->mIndex != IDU_MEM_SENTINEL))
            {
                sSize += aPtr->mSize;
                aPtr = (iduShmPtr*)((ULong)aPtr + aPtr->mSize);
            }

            idlOS::printf("%-30s\t%08llX\t%10llu\t%4s\t%8s\n",
                          iduMemMgr::mClientInfo[sIndex].mName,
                          sOffset, sSize,
                          sIndex != IDU_MEM_RESERVED? "YES":"NO", "-");

            break;

        case 4:
            idlOS::printf("%-30s\t%08llX\t%10llu\t%4s\t%8s\n",
                          iduMemMgr::mClientInfo[aPtr->mIndex].mName,
                          aPtr->mOffset,
                          aPtr->mSize,
                          aPtr->getUsed()==ID_TRUE? "YES":"NO",
                          aPtr->getPrevUsed()==ID_TRUE? "YES":"NO");
            aPtr = (iduShmPtr*)((ULong)aPtr + aPtr->mSize);
            break;
        }

        if(aPtr->mIndex == IDU_MEM_SENTINEL)
        {
            sUsedSize += aPtr->mSize;

            if(aDumpLevel >= 2)
            {
                idlOS::printf("%-30s\t%08X\t%10u\t%4s\t%8s\n",
                              "SENTINEL",
                              aPtr->mOffset,
                              aPtr->mSize,
                              "-", "-");
            }
            break;
        }
    }

    switch(aDumpLevel)
    {
    case 0:
        idlOS::printf("%llu %llu\n", sUsedSize, sFreeSize);
        break;
    case 1:
        idlOS::printf("Used blocks : %llu\n", sUsedSize);
        idlOS::printf("Free blocks : %llu\n", sFreeSize);
        idlOS::printf("Total       : %llu\n", sUsedSize + sFreeSize);
        break;
    default:
        break;
    }
}

void dump(SInt aDumpLevel, SInt aDumpNumber)
{
    SInt    i;
    if(aDumpNumber == -1)
    {
        dumpSys(aDumpLevel);

        for(i = 1; i < IDU_SHM_MAX_CHUNKS && gSys->mKeys[i] != 0; i++)
        {
            if(aDumpLevel >= 1)
            {   
                idlOS::printf("Data chunk[%d] [0x%06X/%d]\n",
                              i, gData[i]->mKey, gIDs[i]);
            }
            dumpChunk(&(gData[i]->mFirstBlock), aDumpLevel);
        }
    }
    else
    {
        if(aDumpNumber < gSys->mChunkCount)
        {
            i = aDumpNumber;
            if(i == 0)
            {
                dumpSys(aDumpLevel);
            }
            else
            {
                if(aDumpLevel >= 1)
                {   
                    idlOS::printf("Data chunk[%d] [0x%06X/%d]\n",
                                  i, gData[i]->mKey, gIDs[i]);
                }
                dumpChunk(&(gData[i]->mFirstBlock), aDumpLevel);
            }
        }
        else
        {
            idlOS::printf("Chunk %d does not exist.\n", aDumpNumber);
            idlOS::printf("Count of chunks is %d (0 ~ %d).\n",
                          gSys->mChunkCount,
                          gSys->mChunkCount - 1);
        }
    }
}

void attach(void)
{
    SInt    i;
    SInt    sKey;
    ULong   sChunkSize;
    SInt    sFlag = 0666;

    if(gSys != NULL) return;

    sKey        = iduProperty::getShmPersKey();
    sChunkSize  = iduProperty::getShmChunkSize();

    gIDs[0] = idlOS::shmget(sKey, sizeof(iduShmPersSys), sFlag);
    IDE_TEST_RAISE(gIDs[0] == -1, ENOSYSCHUNK);
    gSys = (iduShmPersSys*)idlOS::shmat(gIDs[0], NULL, sFlag);
    IDE_TEST_RAISE(gSys == MAP_FAILED, EMAPFAILED);

    gSysKey = sKey;
    gData[0] = (iduShmPersData*)gSys;

    for(i = 1; i < IDU_SHM_MAX_CHUNKS && gSys->mKeys[i] != 0; i++)
    {
        sKey        = gSys->mKeys[i];
        sChunkSize  = gSys->mSizes[i];

        gIDs[i] = idlOS::shmget(sKey, sChunkSize, sFlag);
        IDE_TEST_RAISE(gIDs[i] == -1, ENODATACHUNK);
        gData[i] = (iduShmPersData*)idlOS::shmat(gIDs[i], NULL, sFlag);
        IDE_TEST_RAISE(gData[i] == MAP_FAILED, EMAPFAILED);
    }

    idlOS::printf("System chunk [0x%06X]", gSysKey);
    switch(gSys->mStatus)
    {
    case IDU_SHM_STATUS_INIT:
        idlOS::printf(" Initializing status...\n");
        break;
    case IDU_SHM_STATUS_RUNNING:
        idlOS::printf(" Running status...\n");
        break;
    case IDU_SHM_STATUS_DETACHED:
        idlOS::printf(" Detached status...\n");
        break;
    default:
        idlOS::printf(" Not a valid status...\n");
        break;
    }

    return;

    IDE_EXCEPTION(ENOSYSCHUNK)
    {
        idlOS::fprintf(stderr, "Error : System chunk does not exist : %d\n", errno);
    }
    IDE_EXCEPTION(ENODATACHUNK)
    {
        idlOS::fprintf(stderr, "Error : Data chunk %d does not exist\n", i);
    }

    IDE_EXCEPTION(EMAPFAILED)
    {
        idlOS::fprintf(stderr, "Could not map chunk : %d\n", errno);
    }
    
    IDE_EXCEPTION_END;
    idlOS::fprintf(stderr, "Cannot proceed. Exiting...\n");
    idlOS::exit(-1);
}

void dumpFree(void)
{
    SInt        i;
    SInt        sIndex;
    ULong       sOffset;
    idShmAddr   sFree;
    idShmAddr   sFirstFree;
    iduShmPtr*  sFreePtr;

    idlOS::printf("Free List\n");

    idlOS::printf("%5s\t%13s\t%10s\t%13s\t%13s\n",
                  "Index",
                  "Current",
                  "Block Size",
                  "Prev",
                  "Next");

    for(i = 0; i < 25; i++)
    {
        sFree = gSys->mFrees[i];
        if(sFree == IDU_SHM_NULL_ADDR)
        {
            /* continue */
        }
        else
        {
            sFirstFree = sFree;

            do
            {
                sIndex = (sFree >> 32) & 0x7FFFFFFF;
                sOffset = sFree & 0xFFFFFFFF;
                sFreePtr = (iduShmPtr*)((SChar*)gData[sIndex] + sOffset);
                idlOS::printf("%5d\t%4d/%08X\t%10d\t"
                              "%4d/%08X\t%4d/%08X\n",
                              i,
                              sFreePtr->mID,
                              sFreePtr->mOffset,
                              sFreePtr->mSize,
                              sFreePtr->mPrev >> 32 & 0x7FFFFFFF,
                              sFreePtr->mPrev       & 0xFFFFFFFF,
                              sFreePtr->mNext >> 32 & 0x7FFFFFFF,
                              sFreePtr->mNext       & 0xFFFFFFFF);
                sFree = sFreePtr->mNext;
            } while(sFree != sFirstFree);
        }
    }

}

void dumpReserved(void)
{
    SInt        i;
    SInt        sCount;
    SInt        sIndex;
    SInt        sOffset;
    idShmAddr   sReserved;

    idlOS::printf("Reserved\n");

    sCount = 0;
    for(i = 0; i < IDU_SHM_META_MAX_COUNT; i++)
    {
        sReserved = gSys->mReserved[i];
        if(sReserved == IDU_SHM_NULL_ADDR)
        {
            /* continue */
        }
        else
        {
            sIndex = (sReserved >> 32) & 0x7FFFFFFF;
            sOffset = sReserved & 0xFFFFFFFF;
            idlOS::printf("%d\t%04d/%08X\n", i, sIndex, sOffset);
            sCount++;
        }
    }

    idlOS::printf("%d memory %s reserved\n", sCount,
                  (sCount < 2)? "block":"blocks");
}

void cleanup(idBool aCleanupForce)
{
    SInt    i;
    SInt    sRet;
    SInt    sChunkSize;

    SInt    sFlag = 0666;

    if(gSys->mStatus != IDU_SHM_STATUS_DETACHED)
    {
        idlOS::printf("Warning : Removing non-detached chunks\n");
        idlOS::printf("          DBMS server may be still running.\n");

        if(aCleanupForce == ID_TRUE)
        {
            idlOS::printf("          Removing them immediately.\n");
        }
        else
        {
            idlOS::printf("          To cancel removing, press ^C.\n");
            for(i = 0; i < 10; i++)
            {
                idlOS::printf("%d seconds until removal...\n", 10 - i);
                idlOS::sleep(1);
            }
        }
    }

    for(i = 1; i < IDU_SHM_MAX_CHUNKS && gSys->mKeys[i] != 0; i++)
    {
        sChunkSize = gSys->mSizes[i];
        gIDs[i] = idlOS::shmget(gSys->mKeys[i], sChunkSize, sFlag);
        sRet = idlOS::shmctl(gIDs[i], IPC_RMID, NULL);
        if(sRet == 0)
        {
            idlOS::printf("Data chunk %d(%d/%d) removed successfully\n", i, gSys->mKeys[i], gIDs[i]);
        }
        else
        {
            idlOS::printf("Data chunk %d(%d/%d) removal failed : %d\n", i, gSys->mKeys[i], gIDs[i], errno);
        }
    }

    (void)idlOS::shmdt(gSys);
    sRet = idlOS::shmctl(gIDs[0], IPC_RMID, NULL);
    if(sRet == 0)
    {
        idlOS::printf("System chunk (%d/%d) removed successfully\n", gSysKey, gIDs[0]);
    }
    else
    {
        idlOS::printf("System chunk (%d/%d) removal failed : %d\n", gSysKey, gIDs[0], errno);
    }

    return;
}

void dumpChunkStat(iduShmPtr*   aPtr,
                   ULong&       aUsed,
                   ULong&       aFree,
                   ULong&       aOverhead)
{
    aOverhead   = aPtr->mLeftSize;
    aUsed       = 0;
    aFree       = 0; 

    while(aPtr->mIndex != IDU_MEM_SENTINEL)
    {
        if(aPtr->getUsed() == ID_TRUE)
        {
            aUsed += aPtr->mSize;
            gClientInfo[aPtr->mIndex].mAllocSize += (ULong)aPtr->mSize;
            gClientInfo[aPtr->mIndex].mAllocCount++;
        }
        else
        {
            aFree += aPtr->mSize;
        }

        aPtr = aPtr->getRight();
    }

    aOverhead += aPtr->mSize;
}

void dumpStat(void)
{
    SInt    i;
    SInt    j;
    SInt    sChunkCount;
    ULong   sTotalUsed = 0;
    ULong   sTotalFree = 0;
    ULong   sTotalOverhead = sizeof(iduShmPersSys);
    ULong   sUsed;
    ULong   sFree;
    ULong   sOverhead;

    idlOS::memset(gClientInfo, 0, sizeof(iduMemClientInfo) * IDU_MEM_UPPERLIMIT);
    idlOS::printf("Dump memory statistics\n");

    for(i = 1; i < IDU_SHM_MAX_CHUNKS && gSys->mKeys[i] != 0; i++)
    {
        dumpChunkStat(&(gData[i]->mFirstBlock), sUsed, sFree, sOverhead);

        sTotalUsed      += sUsed;
        sTotalFree      += sFree;
        sTotalOverhead  += sOverhead;
    }
    sChunkCount = i;

    idlOS::printf("%-40s\t%12s\t%12s\n", "Name", "Alloc Size", "Block Count");
    for(j = 0; j < IDU_MAX_CLIENT_COUNT; j++)
    {
        if(gClientInfo[j].mAllocSize == 0)
        {
            /* continue */
        }
        else
        {
            idlOS::printf("%-40s\t%12llu\t%12llu\n",
                          iduMemMgr::mClientInfo[j].mName,
                          gClientInfo[j].mAllocSize,
                          gClientInfo[j].mAllocCount);
        }
    }

    idlOS::printf("Total %d chunk%s\n", sChunkCount, (sChunkCount > 1)? "s":"");
    idlOS::printf("Total used size             : %llu\n", sTotalUsed);
    idlOS::printf("Total free size             : %llu\n", sTotalFree);
    idlOS::printf("Total overhead              : %llu\n", sTotalOverhead);
    idlOS::printf("Total persistent chunk size : %llu\n",
                  sTotalUsed + sTotalFree + sTotalOverhead);
}

void printusage(void)
{
    idlOS::printf("Usage : dumpshm [-c] [-l 0..4 [-n number]] [-r] [-f] [-s] [-h]\n");
    idlOS::printf("\t -c : cleanup persistent shared memory chunks.\n");
    idlOS::printf("\t\t -f : cleanup immediately even in running status.\n");
    idlOS::printf("\t -l : dump persistent shared memory chunks.\n");
    idlOS::printf("\t\t  0 : only information of chunks.\n");
    idlOS::printf("\t\t  1 : only information of chunks.\n");
    idlOS::printf("\t\t  2 : only used or unused.\n");
    idlOS::printf("\t\t  3 : grouped by memory index.\n");
    idlOS::printf("\t\t  4 : every block.\n");
    idlOS::printf("\t\t -n : Dump only [number] chunk.\n");
    idlOS::printf("\t -r : dump free and reserved memory blocks.\n");
    idlOS::printf("\t -s : dump X$MEMSTAT like memory statistics.\n");
    idlOS::printf("\t -h : print this help screen.\n");
}

SInt main(SInt aArgc, SChar** aArgv)
{
    SInt    sOptVal;
    SInt    sOptCount = 0;
    SInt    sDumpNumber = -1;
    idBool  sCleanup        = ID_FALSE;
    idBool  sDumpChunks     = ID_FALSE;
    idBool  sCleanupForce   = ID_FALSE;
    SInt    sDumpLevel = 0;
    idBool  sDumpReserved   = ID_FALSE;
    idBool  sDumpFree       = ID_FALSE;
    idBool  sDumpStat       = ID_FALSE;

    while((sOptVal = idlOS::getopt(aArgc, aArgv, "cl:n:hfrs")) != EOF)
    {
        sOptCount++;

        switch(sOptVal)
        {
        case 'c':
            sCleanup = ID_TRUE;
            break;
        case 'l':
            if(optarg == NULL)
            {
                idlOS::printf("-l needs level 0 ~ 4 to dump shm chunks\n");
            }
            else
            {
                sDumpChunks = ID_TRUE;
                sDumpLevel = (SInt)idlOS::strtol(optarg, NULL, 10);
                if(sDumpLevel >= 0 && sDumpLevel <= 4)
                {
                    /* break */
                }
                else
                {
                    idlOS::printf("-l needs level 0 ~ 4 to dump shm chunks.\n");
                    idlOS::printf("Defaulting the level to 3.\n");
                    sDumpLevel = 3;
                }
            }
            break;

        case 'n':
            if(optarg == NULL)
            {
                idlOS::printf("-n needs chunk number to dump\n");
            }
            else
            {
                sDumpChunks = ID_TRUE;
                sDumpNumber = (SInt)idlOS::strtol(optarg, NULL, 10);
            }
            break;

        case 'h':
            printusage();
            break;

        case 'f':
            sCleanupForce = ID_TRUE;
            break;

        case 'r':
            sDumpFree = ID_TRUE;
            sDumpReserved = ID_TRUE;
            break;

        case 's':
            sDumpStat = ID_TRUE;
            break;

        default:
            printusage();
            break;
        }
    }

    if(sOptCount == 0)
    {
        printusage();
    }
    else
    {
        /* idp must be initlaized at the first of program */
        IDE_TEST_RAISE(idp::initialize(NULL, NULL) != IDE_SUCCESS, EPROPERTY);
        IDE_TEST_RAISE(iduProperty::load() != IDE_SUCCESS,         EPROPERTY);

        attach();

        if(sCleanup == ID_TRUE)
        {
            idlOS::printf("Cleanup chunks...\n");
            cleanup(sCleanupForce);
        }
        else
        {
            if(sDumpChunks == ID_TRUE)
            {
                dump(sDumpLevel, sDumpNumber);
            }

            if(sDumpFree == ID_TRUE)
            {
                dumpFree();
            }

            if(sDumpReserved == ID_TRUE)
            {
                dumpReserved();
            }

            if(sDumpStat == ID_TRUE)
            {
                dumpStat();
            }
        }
    }

    return 0;

    IDE_EXCEPTION(EPROPERTY)
    {
        idlOS::fprintf(stderr, "Error : Loading properties failed.\n");
    }

    IDE_EXCEPTION_END;
    idlOS::fprintf(stderr, "Cannot proceed. exiting...\n");
    return -1;
}

