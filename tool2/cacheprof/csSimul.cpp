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
 

/*****************************************************************************
 * $Id: csSimul.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include <csSimul.h>

#define CacheMem(set, way)  (*(mCacheMem + (mCountOfLineEachWay * (way)) + (sSet)))

ThreadMutex csSimul::mMutex = ThreadNoId;

csSimul::csSimul()
{
}

int csSimul::initialize(Address aCapacity, Address aLineSize, Address aWay)
{
    // Statatistics

    mStat.initialize(1024);
    
    // Cache Memory Initialize
    mCapacity = aCapacity;
    mLineSize = aLineSize;
    mWay      = aWay;
    mCountOfLineEachWay = (mCapacity / mLineSize / mWay);

    mCacheMem = (Address *)malloc(sizeof(Address) * mCapacity / mLineSize);
    assert(mCacheMem != NULL);
    
    memset(mCacheMem, 0xFF, sizeof(Address) * mCapacity / mLineSize);

    // Profiling Action Initialize
    mStarted  = ID_FALSE;
    mTargetId = (ThreadId)0;

    return 0;
}

int csSimul::destroy()
{
    free(mCacheMem);
    return 0;
}

int csSimul::doRefer(Address aAddr)
{
    // Cache Miss를 판단한는 핵심 함수

    int i;
    int j;

    Address sTag = aAddr / (mCapacity / mWay);
    Address sSet = (aAddr / mLineSize) & (mCountOfLineEachWay - 1);

    for (i = 0; i < mWay; i++)
    {
        Address sCurTag;

        sCurTag = CacheMem(sSet, i);

        /* ------------------------------------------------
         * cache hit이 났을 경우, LRU 알고리즘에 의해
         * 자신을 0번으로 이동하고, 나머지를 뒤로 1칸씩 민다.
         * ----------------------------------------------*/
        if (sCurTag == sTag) // cache hit
        {
            for (j = i; j > 0; j--) // LRU 알고리즘에 의해 0번으로 이동
            {
                CacheMem(sSet, j) = CacheMem(sSet, j - 1);
            }
            CacheMem(sSet, 0) = sTag;
            
            return 0; // hit return
        }
    }
    // cache miss
    /* ------------------------------------------------
     *  cache miss가 났을 경우 LRU 알고리즘에 의해
     *  마지막 라인을 비우고, 0번에 Tag를 등록.
     * ----------------------------------------------*/
    for (j = mWay - 1; j > 0; j--)
    {
        CacheMem(sSet, j) = CacheMem(sSet, j - 1);
    }
    CacheMem(sSet, 0) = sTag;
    
    return 1; // miss return 
}

void csSimul::refer(ThreadId      aThreadId,
                    Address       aAddr,
                    int           aWriteFlag,
                    char         *aFileName,
                    char         *aProcName,
                    long          aLine)
{
    if (mStarted == ID_TRUE)
    {
        if (mTargetId == 0 || mTargetId == aThreadId)
        {
            assert(ThreadMutexLock(&mMutex, aThreadId) == THREAD_SUCCESS);
            if (doRefer(aAddr) == 1) // cache miss
            {
                mStat.update(aWriteFlag, aFileName, aProcName, aLine);
            }
            ThreadMutexUnlock(&mMutex);
        }
    }
}

void csSimul::Clear()
{
    mStat.clearInfo();
}

void csSimul::Dump(FILE *aOutput)
{
    mStat.Dump(aOutput);
}

void csSimul::DumpTest(Address aAddr)
{
    if (doRefer(aAddr) == 0)
    {
        fprintf(stderr, "Cache Hit  %lx\n", aAddr);
    }
    else
    {
        fprintf(stderr, "Cache Miss %lx\n", aAddr);
    }
}

void csSimul::writeHeader(FILE *aFp)
{
    time_t timet;
    struct tm  now;

    time(&timet);
    localtime_r(&timet, &now);
    fprintf(aFp, "[%4d/%02d/%02d %02d:%02d:%02d] ",
	    now.tm_year + 1900,
	    now.tm_mon + 1,
	    now.tm_mday,
	    now.tm_hour,
	    now.tm_min,
	    now.tm_sec);

    fprintf(aFp, "Cache Mem Info : Capacity (%ldk), LineSize (%ld), Ways (%ld)\n",
	      mCapacity, mLineSize, mWay);
    
}

void csSimul::Save(char *aFileName)
{
    FILE *fp;
    char sBuffer[256];
    timeval sNow;


    if (aFileName == NULL)
    {
        gettimeofday(&sNow, NULL);

        sprintf(sBuffer, "ProfData_%ld_%ld.cv", sNow.tv_sec, sNow.tv_usec);

        aFileName = sBuffer;
    }
    
    fp = fopen(aFileName, "a");
    if (fp != NULL)
    {
        writeHeader(fp);
        mStat.Dump(fp);
        fclose(fp);
    }
    else
    {
        fprintf(stderr, "error in open %s file (errno=%d) \n", aFileName, errno);
    }
}
