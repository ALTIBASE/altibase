/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <idl.h>
#include <act.h>
#include <idu.h>
#include <idp.h>
#include <ideErrorMgr.h>
#include <idtBaseThread.h>
#include <idtContainer.h>


#define TEST_ALLOC_COUNT   10240
// allocation size 2K~4K
#define TEST_ALLOC_SIZE_RANGE 1023
#define TEST_ALLOC_SIZE_MIN 1
// maximum thread count
#define TEST_THREAD_COUNT  1024

SInt gAllocSize[TEST_ALLOC_COUNT];

class testMultiThread : public idtBaseThread
{
public:
    UChar*                  mBufPtrMalloc[TEST_ALLOC_COUNT];

testMultiThread(SInt Flags = IDT_JOINABLE)
    : idtBaseThread(Flags)
{
}

virtual void run(void)
{
    ULong                   sAllocSize;
    SInt                    i;
    iduMemoryClientIndex    sTestIndex = (iduMemoryClientIndex)0;
    
    for (i = 0; i < TEST_ALLOC_COUNT / 2; i++)
    {
        sAllocSize = gAllocSize[i];

        ACT_CHECK_DESC(iduMemMgr::malloc(sTestIndex,
                                         sAllocSize,
                                         (void **)&mBufPtrMalloc[i])
                       == IDE_SUCCESS,
                       ("malloc() failed at %p", (void*)getTid()));
        ACT_CHECK(mBufPtrMalloc[i] != NULL);
    }

    for (i = 0; i < TEST_ALLOC_COUNT / 2; i++)
    {
        sAllocSize = gAllocSize[i];

        ACT_CHECK_DESC(iduMemMgr::malloc(sTestIndex,
                                         sAllocSize,
                                         (void **)&mBufPtrMalloc[i + TEST_ALLOC_COUNT / 2])
                       == IDE_SUCCESS,
                       ("malloc() failed at %p", (void*)getTid()));
        ACT_CHECK(mBufPtrMalloc[i] != NULL);
        ACT_CHECK(iduMemMgr::free(mBufPtrMalloc[i]) == IDE_SUCCESS);
    }
    
    for (i = 0; i < TEST_ALLOC_COUNT / 2; i++)
    {
        ACT_CHECK(iduMemMgr::free(mBufPtrMalloc[i + TEST_ALLOC_COUNT / 2]) == IDE_SUCCESS);
    }
}
};

testMultiThread gThreads[TEST_THREAD_COUNT];

void testMultiThread(int aThreadCount)
{
    acp_time_t      sBegin;
    acp_time_t      sEnd;
    acp_time_t      sElapsed[4];
    acp_time_t      sMax;
    acp_time_t      sMin;
    acp_time_t      sMean;
    SInt            i;
    SInt            j;


    ACT_ASSERT(aThreadCount < TEST_THREAD_COUNT);

    printf("%10d %10d ", aThreadCount, TEST_ALLOC_COUNT);
    for(j = 0; j < 4; j++)
    {
        sBegin = acpTimeNow();    
        for (i = 0; i < aThreadCount; i++)
        {
            gThreads[i].start();
        }
        for (i = 0; i < aThreadCount; i++)
        {
            gThreads[i].join();
        }
        sEnd = acpTimeNow();

        sElapsed[j] = sEnd - sBegin;
        printf("%6ld ", acpTimeToMsec(sElapsed[j]));
    }

    sMax = sElapsed[0];
    sMin = sElapsed[0];
    sMean = sElapsed[0];

    for(i = 1; i < 4; i++)
    {
        if(sMax < sElapsed[i]) sMax = sElapsed[i];
        if(sMin > sElapsed[i]) sMin = sElapsed[i];
        sMean += sElapsed[i];
    }
    sMean /= 4;

    printf("%6ld %6ld %6ld\n",
           acpTimeToMsec(sMin),
           acpTimeToMsec(sMean),
           acpTimeToMsec(sMax));

    ACT_ASSERT(aThreadCount < TEST_THREAD_COUNT);
}

void testPerfMemMgr(void)
{
    SInt sCPUCount = idlVA::getProcessorCount();
    SInt i;
    SInt j;
    UInt sSeed;

    /* create sizes */
    sSeed = acpRandSeedAuto();
    
    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        gAllocSize[i] =
            ((i * 1024) % TEST_ALLOC_SIZE_RANGE) + TEST_ALLOC_SIZE_MIN;
    }

    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStaticContainer() == IDE_SUCCESS);

    printf("%10s %10s\n", "Threads", "Allocs");
    for(i = 1; i <= sCPUCount && i <= TEST_THREAD_COUNT; i *= 2)
    {
        testMultiThread(i);
    }

    ACT_ASSERT(idtContainer::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);
}

SInt main( void )
{
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);
    ACT_ASSERT(iduProperty::load() == IDE_SUCCESS);

    idlOS::printf("Testing LIBC Memory Allocator\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)0) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing with no private allocator.\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_USE_PRIVATE",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);

    idlOS::printf("Testing TLSF Memory Allocator with mmap, auto shrink.\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(1)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing TLSF Memory Allocator with mmap, no shrink.\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing TLSF Memory Allocator with global allocator, auto shrink\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(16)*1024*1024*1024)
               == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(1)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing TLSF Memory Allocator with global allocator, no shrink\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(16)*1024*1024*1024)
               == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing with private allocators.\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_USE_PRIVATE",
                                (ULong)ID_ULONG(1)) == IDE_SUCCESS);

    idlOS::printf("Testing TLSF Memory Allocator with mmap, auto shrink.\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(1)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing TLSF Memory Allocator with mmap, no shrink.\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing TLSF Memory Allocator with global allocator, auto shrink\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(16)*1024*1024*1024)
               == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(1)) == IDE_SUCCESS);
    testPerfMemMgr();

    idlOS::printf("Testing TLSF Memory Allocator with global allocator, no shrink\n");
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)1) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                                (ULong)ID_ULONG(16)*1024*1024*1024)
               == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_AUTO_SHRINK",
                                (ULong)ID_ULONG(0)) == IDE_SUCCESS);
    testPerfMemMgr();

    return IDE_SUCCESS;
}
