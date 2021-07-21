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
 
/***********************************************************************
 * $Id:
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduMemMgr.h>

/***********************************************************************
 * iduMemMgr_libc.cpp : IDU_SERVER_TYPE에서 사용
 * iduMemMgr을 서버 모드로 초기화한 이후
 * 메모리 관리자 타입이 LIBC(=0)일 때 사용한다.
 * 메모리 통계정보를 기록한다.
 **********************************************************************/

IDE_RC iduMemMgr::libc_initializeStatic(void)
{
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::libc_destroyStatic(void)
{
    return IDE_SUCCESS;
}

void iduMemMgr::libc_getHeader(void* aMemPtr,
                               void** aHeader1,
                               void** aHeader2)
{
    iduMemLibcHeader*  sHeader1;
    iduMemLibcHeader*  sHeader2;
    iduMemLibcHeader** sTailer2;

    SChar*              sMemPtr = (SChar*)aMemPtr;

    sHeader2 = (iduMemLibcHeader*)aMemPtr - 1;

    sTailer2 = (iduMemLibcHeader**)(sMemPtr + sHeader2->mAllocSize
                                     - sizeof(iduMemLibcHeader) - sizeof(void*));
    sHeader1 = *sTailer2;

    IDE_DASSERT( sHeader1 == sHeader2 );

    *aHeader1 = (void*)sHeader1;
    *aHeader2 = (void*)sHeader2;
}

IDE_RC iduMemMgr::libc_malloc(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              void                 **aMemPtr)
{
    ULong               sSize;
    SChar*              sMemPtr;
    iduMemLibcHeader*   sHeader;
    void**              sTailer;

    IDE_DASSERT(aMemPtr != NULL);
    sSize = idlOS::align8(aSize + ID_SIZEOF(iduMemLibcHeader)) + ID_SIZEOF(void*);

    sMemPtr = (SChar*)idlOS::malloc(sSize);
    IDE_TEST(sMemPtr == NULL);

    sHeader  = (iduMemLibcHeader*)sMemPtr;
    sTailer  = (void**)(sMemPtr + sSize - sizeof(void*));
    *sTailer = (void*)sHeader;

    sHeader->mClientIndex = (UInt)aIndex;
    sHeader->mAllocSize   = sSize;

    *aMemPtr = (void*)(sHeader + 1);

    server_statupdate(aIndex, (SLong)sSize, 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::libc_malign(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              ULong                  aAlign,
                              void                 **aMemPtr)
{
    IDE_DASSERT(aMemPtr != NULL);

#if defined(ALTI_CFG_OS_LINUX) || defined(ALTI_CFG_OS_HPUX) 
    *aMemPtr = (SChar*)memalign(aAlign, aSize);
#elif defined(ALTI_CFG_OS_AIX)
    ULong               sBlockSize;
    ULong               sAllocSize;
    ULong               sAddr;
    SChar*              sMemPtr;
    iduMemLibcHeader*   sHeader1;
    iduMemLibcHeader*   sHeader2;
    void**              sTailer;

    /* The FFS and FLS of a power of 2 should be the same */
    IDE_DASSERT(aMemPtr != NULL);
    IDE_DASSERT((aAlign >= sizeof(iduMemLibcHeader)) && (aAlign % sizeof(void*) == 0));
    IDE_DASSERT(acpBitFfs64(aAlign) == acpBitFls64(aAlign));

    sBlockSize = idlOS::align8(aSize + ID_SIZEOF(iduMemLibcHeader)) + ID_SIZEOF(void*);
    sAllocSize = idlOS::align8(aSize + aAlign + ID_SIZEOF(iduMemLibcHeader) * 2 + ID_SIZEOF(void*) * 2);

    sMemPtr = (SChar*)idlOS::malloc(sAllocSize);
    IDE_TEST(sMemPtr == NULL);

    sHeader1  = (iduMemLibcHeader*)sMemPtr;
    sHeader1->mClientIndex = (UInt)aIndex;
    sHeader1->mAllocSize   = sAllocSize;

    if(((ULong)(sHeader1 + 1) % aAlign) == 0)
    {
        /* align matches */
        sTailer  = (void**)(sMemPtr + sAllocSize - sizeof(void*));
        *sTailer = (void*)sHeader1;

        *aMemPtr = (void*)(sHeader1 + 1);
    }
    else
    {
        /* align mismatches - match align by hand */
        sAddr = (ULong)(sHeader1 + 1) + aAlign;
        sAddr -= (sAddr % aAlign);

        /* Reserve space for second block header */
        sHeader2 = (iduMemLibcHeader*)sAddr;
        sHeader2--;

        sHeader2->mClientIndex = aIndex;
        sHeader2->mAllocSize   = sBlockSize;

        /* Preserve first block header address */
        sTailer  = (void**)((SChar*)sHeader2 + sBlockSize - sizeof(void*));
        *sTailer = (void*)sHeader1;

        sTailer  = (void**)((SChar*)sHeader1 + sAllocSize - sizeof(void*));
        *sTailer = (void*)sHeader1;

        *aMemPtr = (void*)(sHeader2 + 1);
    }
#else
#error not_implemented
#endif
    IDE_TEST(*aMemPtr == NULL);

    server_statupdate(aIndex, (SLong)aSize, 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerError ));
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::libc_calloc(iduMemoryClientIndex   aIndex,
                              vSLong                 aCount,
                              ULong                  aSize,
                              void                 **aMemPtr)
{
    ULong               sSize;
    SChar*              sMemPtr;
    iduMemLibcHeader*   sHeader;
    void**              sTailer;

    IDE_DASSERT(aMemPtr != NULL);
    sSize = idlOS::align8(aSize * aCount + ID_SIZEOF(iduMemLibcHeader)) + ID_SIZEOF(void*);

#if defined(IBM_AIX)
    sMemPtr = (SChar*)idlOS::malloc(sSize);
    IDE_TEST(sMemPtr == NULL);
    idlOS::memset(sMemPtr, 0, sSize);
#else
    sMemPtr = (SChar*)idlOS::calloc(1, sSize);
    IDE_TEST(sMemPtr == NULL);
#endif

    sHeader  = (iduMemLibcHeader*)sMemPtr;
    sTailer  = (void**)(sMemPtr + sSize - sizeof(void*));
    *sTailer = (void*)sHeader;

    sHeader->mClientIndex = (UInt)aIndex;
    sHeader->mAllocSize   = sSize;

    *aMemPtr = (void*)(sHeader + 1);

    server_statupdate(aIndex, (SLong)sSize, 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::libc_realloc(iduMemoryClientIndex  aIndex,
                               ULong                 aSize,
                               void                **aMemPtr)
{
    iduMemLibcHeader*  sHeader1;
    iduMemLibcHeader*  sHeader2;
    iduMemLibcHeader** sTailer;
    ULong               sOldSize;
    ULong               sNewSize;
    SChar*              sMemPtr;

    IDE_DASSERT(aMemPtr != NULL);
    libc_getHeader(*aMemPtr, (void**)&sHeader1, (void**)&sHeader2);
    sNewSize = idlOS::align8(aSize + ID_SIZEOF(iduMemLibcHeader)) + ID_SIZEOF(void*);
    sOldSize = sHeader1->mAllocSize;

    if(sNewSize > sOldSize)
    {
        sMemPtr = (SChar*)sHeader1;
        sMemPtr = (SChar*)idlOS::realloc(sMemPtr, sNewSize);
        IDE_TEST(sMemPtr == NULL);

        if(sHeader1 == sHeader2)
        {
            /*
             * This block was allocated by malloc, calloc or realloc
             * No need of copy
             */
        }
        else
        {
            /*
             * This block was allocated by malign 
             * copy memory contents from real block position
             */
            idlOS::memmove((void*)(sHeader1 + 1),
                    (void*)(sHeader2 + 1),
                    sHeader2->mAllocSize);
        }
        sHeader1 = (iduMemLibcHeader*)sMemPtr;
        sHeader1->mClientIndex = (UInt)aIndex;
        sHeader1->mAllocSize   = sNewSize;

        sTailer  = (iduMemLibcHeader**)(sMemPtr + sNewSize - sizeof(void*));
        *sTailer = sHeader1;

        *aMemPtr = (void*)(sHeader1 + 1);

        server_statupdate(aIndex, -((SLong)sOldSize), -1);
        server_statupdate(aIndex,   (SLong)sNewSize ,  1);
    }
    else
    {
        /* no need of realloc */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::libc_free(void                   *aMemPtr)
{
    iduMemLibcHeader*  sHeader1;
    iduMemLibcHeader*  sHeader2;

    IDE_DASSERT(aMemPtr != NULL);

    libc_getHeader(aMemPtr, (void**)&sHeader1, (void**)&sHeader2);
    server_statupdate((iduMemoryClientIndex)sHeader1->mClientIndex,
                      -((SLong)sHeader1->mAllocSize), -1);

    /* sHeader1 is always the header block */
    idlOS::free(sHeader1);

    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::libc_free4malign(void                   *aMemPtr,
                                   iduMemoryClientIndex    aIndex,
                                   ULong                   aSize)
{
    idlOS::free(aMemPtr);
    server_statupdate(aIndex, 
                     -(SLong)aSize, -1);
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::libc_shrink(void)
{
    return IDE_SUCCESS;
}

