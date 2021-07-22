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


#define TEST_LOOP_COUNT 100

SInt main( void )
{
    iduMemoryClientIndex sTestIndex = (iduMemoryClientIndex)IDU_MEM_OTHER;
    ULong sAllocSize;
    UChar *sBufPtr[TEST_LOOP_COUNT];
    SInt i;
#if defined(BUILD_MODE_DEBUG)
    SInt sAccIndex;
#endif
    
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);    

    sAllocSize = 1024;

    ACT_CHECK(acpCStrCmp(iduMemMgr::mClientInfo[sTestIndex].mName,
                         "IDU_MEM_OTHER",
                         sizeof("IDU_MEM_OTHER")) == 0);
    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        ACT_CHECK_DESC(iduMemMgr::malloc(sTestIndex,
                                         sAllocSize,
                                         (void **)&sBufPtr[i]) == IDE_SUCCESS,
                       ("malloc() failed"));
    }

/*
 * Statistics information update according to allocation size works only for debug mode.
 */
#if defined(BUILD_MODE_DEBUG)
    sAccIndex = acpBitFls64(sAllocSize);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == 0);
#endif
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocCount == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocSize > TEST_LOOP_COUNT * sAllocSize &&
              iduMemMgr::mClientInfo[sTestIndex].mAllocSize < TEST_LOOP_COUNT * sAllocSize * 2);

    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        ACT_CHECK_DESC(iduMemMgr::free(sBufPtr[i]) == IDE_SUCCESS,
                       ("free() failed"));
    }
#if defined(BUILD_MODE_DEBUG)
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == TEST_LOOP_COUNT);
#endif
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocCount == 0);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocSize == 0);


    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        ACT_CHECK_DESC(iduMemMgr::calloc(sTestIndex,
                                         1,
                                         sAllocSize,
                                         (void **)&sBufPtr[i]) == IDE_SUCCESS,
                       ("malloc() failed"));
    }

    // count of malloc + count of calloc == 2 * TEST_LOOP_COUNT
#if defined(BUILD_MODE_DEBUG)
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT * 2);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == TEST_LOOP_COUNT);
#endif
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocCount == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocSize > TEST_LOOP_COUNT * sAllocSize &&
              iduMemMgr::mClientInfo[sTestIndex].mAllocSize < TEST_LOOP_COUNT * sAllocSize * 2);


    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        ACT_CHECK_DESC(iduMemMgr::realloc(sTestIndex,
                                         sAllocSize * 2,
                                         (void **)&sBufPtr[i]) == IDE_SUCCESS,
                       ("malloc() failed"));
    }
#if defined(BUILD_MODE_DEBUG)
    // count of realloc is not added into alloc-count
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT * 2);
    // realloc means free of old memory size so that free-count is increased
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == TEST_LOOP_COUNT * 2);
#endif
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocCount == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocSize > TEST_LOOP_COUNT * sAllocSize * 2&&
              iduMemMgr::mClientInfo[sTestIndex].mAllocSize < TEST_LOOP_COUNT * sAllocSize * 3);

#if defined(BUILD_MODE_DEBUG)
    // info of new size
    sAccIndex = acpBitFls64(sAllocSize * 2);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == 0);
#endif
    
    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        ACT_CHECK_DESC(iduMemMgr::free(sBufPtr[i]) == IDE_SUCCESS,
                       ("free() failed"));
    }

#if defined(BUILD_MODE_DEBUG)
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == TEST_LOOP_COUNT);
#endif
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocCount == 0);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocSize == 0);

#if defined(BUILD_MODE_DEBUG)
    // info of old size
    sAccIndex = acpBitFls64(sAllocSize);
    
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccAllocCount[sAccIndex] == TEST_LOOP_COUNT * 2);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAccFreedCount[sAccIndex] == TEST_LOOP_COUNT * 2);
#endif
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocCount == 0);
    ACT_CHECK(iduMemMgr::mClientInfo[sTestIndex].mAllocSize == 0);

    return IDE_SUCCESS;
}


