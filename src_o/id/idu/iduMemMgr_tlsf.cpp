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
#include <idp.h>
#include <iduMemMgr.h>
#include <idtContainer.h>

/***********************************************************************
 * iduMemMgr_tlsf.cpp : IDU_SERVER_TYPE에서 사용
 * iduMemMgr을 서버 모드로 초기화한 이후
 * 메모리 관리자 타입이 TLSF(=1)일 때 사용한다.
 * 메모리 통계정보를 기록한다.
 **********************************************************************/

#define IDU_PAGESIZE (4096)

/***********************************************************************
 * HTML Codes to output memory dump
 **********************************************************************/
#define MAKECOLOR(aCol)             \
    (((aCol << 16 | 0xF00000) |     \
      (aCol <<  8 | 0x00F000) |     \
      (0xFF)) & 0xFFFFFF)

const SChar gDumpHeader[]   = "<html><body>\n";
const SChar gAllocHeader[]  = "<table border=1><tr><td colspan=3 align=center>\n";
const SChar gAllocBody[]    = "%s<br>(%p)<br> %lluKiB Occupied <br>%lluKiB Used</td></tr>";
const SChar gAllocTailer[]  = "</td></tr></table>\n";
const SChar gChunkHeader[]  = "<tr><td colspan=3 color=#FFFFFF align=center>\n";
const SChar gChunkBody[]    = "Chunk %p %lluKiB\n</td></tr>"
                              "<tr><td>ALLOC_NAME</td><td>Start Address</td>"
                              "<td>Size</td></tr>";
const SChar gChunkTailer[]  = "";
const SChar gBlockHeader[]  = "<tr>\n";
const SChar gBlockBody[]    = "<td bgcolor=#%06X>%s</td><td bgcolor=%06X>%p</td><td bgcolor=%06X>%llu</td>\n";
const SChar gBlockTailer[]  = "</tr>\n";
const SChar gDumpTailer[]   = "</body></html>\n";

/***********************************************************************
 * Small implementations
 * Maximum available allocation size :
 * 2^11=2kBytes
 **********************************************************************/
IDE_RC iduMemSmall::initialize(SChar* aName, UInt aChunkSize)
{
    IDE_TEST(iduMemAlloc::initialize(aName) != IDE_SUCCESS);
    mOwner = ID_ULONG(0xFFFFFFFFFFFFFFFF);

    mChunkSize = aChunkSize;
    mSpinCount = iduMemMgr::getSpinCount();
    mSpinCount = (mSpinCount == -1)? 0 : mSpinCount;
    idlOS::strcpy(mType, "THREAD_LOCAL_SMALL");
#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_TEST(idlOS::thread_mutex_init(&mLock) != 0);
#endif
    idlOS::strcpy(mMemInfo[IDU_MEM_RESERVED].mName, "RESERVED");

    idlOS::memset(mFrees, 0, sizeof(iduMemSmallHeader*) * IDU_SMALL_SIZES);
    mChunks = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemSmall::destroy()
{
    /*
     * TODO :: Freeing chunks here
     */
    iduMemSmallChunk*   sChunk;
    iduMemSmallChunk*   sNext;

    sChunk = mChunks;

    while(sChunk != NULL)
    {
        sNext = sChunk->mNext;
        IDE_TEST(iduMemMgr::free(sChunk) != IDE_SUCCESS);
        sChunk = sNext;
    }

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_TEST(idlOS::thread_mutex_destroy(&mLock) != 0);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemSmall::malloc(iduMemoryClientIndex     aIndex,
                           ULong                    aSize,
                           void**                   aPtr)
{
    SInt                sFL;
    UInt                sSize;
    iduMemSmallHeader*  sHeader;

    IDE_DASSERT(aSize <= IDU_SMALL_MAXSIZE);
    sSize = calcAllocSize(aSize, &sFL);
    IDE_DASSERT(sFL < IDU_SMALL_SIZES);

    IDE_ASSERT(lock() == IDE_SUCCESS);
    
    sHeader = mFrees[sFL];
    while(sHeader == NULL)
    {
        IDE_TEST_RAISE(allocChunk(sSize) != IDE_SUCCESS, ECANNOTEXPAND);
        sHeader = mFrees[sFL];
    }

    mFrees[sFL] = sHeader->h.mNext;
    statupdate(aIndex, sSize, 1);

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    sHeader->mAllocSize    |= IDU_TLSF_USED;
    sHeader->h.a.mClientIndex   = aIndex;

    *aPtr = (void*)(sHeader + 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ECANNOTEXPAND)
    {
        IDE_ASSERT(unlock() == IDE_SUCCESS);
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemSmall::free(void* aPtr)
{
    const ULong sF0 = ID_ULONG(0xFFFFFFF0);

    iduMemSmallHeader*  sHeader;
    ULong               sAllocSize;
    SInt                sFL;
    SInt                sIndex;

    sHeader = (iduMemSmallHeader*)aPtr;
    sHeader--;

    sAllocSize  = sHeader->mAllocSize & sF0;
    sIndex      = sHeader->h.a.mClientIndex;
    sFL         = calcFL(sAllocSize);

    IDE_ASSERT(lock() == IDE_SUCCESS);
    sHeader->h.mNext = mFrees[sFL];
    mFrees[sFL] = sHeader;
    statupdate((iduMemoryClientIndex)sIndex, -(SInt)sAllocSize, -1);
    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

IDE_RC iduMemSmall::freeStatic(void* aPtr)
{
    const ULong sFF = ID_ULONG(0xFFFFFFFF);
    const ULong sF0 = ID_ULONG(0xFFFFFFF0);

    iduMemSmallHeader*  sHeader;
    iduMemSmallChunk*   sChunk;
    iduMemSmall*        sSmall;

    ULong               sAllocSize;
    ULong               sOffset;

    sHeader = (iduMemSmallHeader*)aPtr;
    sHeader--;

    IDE_DASSERT((sHeader->mAllocSize & IDU_TLSF_USED) != 0);
    IDE_DASSERT((sHeader->mAllocSize & IDU_TLSF_SMALL) != 0);
    sHeader->mAllocSize    &= ~IDU_TLSF_USED;

    sAllocSize = sHeader->mAllocSize & sF0;
    sOffset    = (sHeader->mAllocSize >> 32) & sFF;

    sChunk  = (iduMemSmallChunk*)((SChar*)sHeader - sOffset);
    IDE_DASSERT(sChunk->mBlockSize == sAllocSize);
    sSmall  = sChunk->mParent;
    
    return sSmall->free(aPtr);
}

IDE_RC iduMemSmall::lock(void)
{
    ULong sOwner = (ULong)idlOS::thr_self();

    if(mOwner == sOwner)
    {
        mLocked++;
    }
    else
    {
#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
        IDE_TEST(idlOS::thread_mutex_lock(&mLock) != 0);
        mOwner = sOwner;
#else

        while((ULong)acpAtomicCas64(&mOwner, sOwner, ID_ULONG(0xFFFFFFFFFFFFFFFF))
              != ID_ULONG(0xFFFFFFFFFFFFFFFF))
        {
            idlOS::thr_yield();
        }
#endif
        mLocked = 1;
    }

    return IDE_SUCCESS;

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

IDE_RC iduMemSmall::unlock(void)
{
    IDL_MEM_BARRIER;
    IDE_DASSERT(mOwner == (ULong)idlOS::thr_self());
    mLocked--;

    if(mLocked == 0)
    {
        mOwner = ID_ULONG(0xFFFFFFFFFFFFFFFF);
#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
        IDE_TEST(idlOS::thread_mutex_unlock(&mLock) != 0);
#endif
    }
    else
    {
        /* Still lock got */
    }

    return IDE_SUCCESS;

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

UInt   iduMemSmall::calcAllocSize(ULong aSize, SInt* aFL)
{
    UInt sRet;
    SInt sMSB;
    SInt sLSB;
    IDE_DASSERT(aSize <= IDU_SMALL_MAXSIZE);

    sRet = IDL_MAX((UInt)aSize, IDU_SMALL_MINSIZE);
    sMSB = acpBitFls32(sRet);
    sLSB = acpBitFfs32(sRet);

    if(sMSB == sLSB)
    {
        /* aSize is a power of two */
        *aFL = sMSB - 4;
    }
    else
    {
        /* Adjust size to the power of two */
        sRet = 1 << (sMSB + 1);
        *aFL = sMSB - 3;
    }

    /* Add header size */
    sRet += sizeof(iduMemSmallHeader);
    return sRet;
}


SInt   iduMemSmall::calcFL(ULong aSize)
{
    SInt sMSB;
    SInt sLSB;

    aSize -= sizeof(iduMemSmallHeader);
    IDE_DASSERT(aSize >= IDU_SMALL_MINSIZE);
    IDE_DASSERT(aSize <= IDU_SMALL_MAXSIZE);

    sMSB = acpBitFls32(aSize);
    sLSB = acpBitFfs32(aSize);

    IDE_DASSERT(sMSB == sLSB);

    return sMSB - 4;
}

IDE_RC iduMemSmall::allocChunk(UInt aSize)
{
    iduMemSmallChunk*   sChunk;
    iduMemSmallHeader*  sHeader;
    iduMemSmallHeader*  sHeaderPrev;

    SInt                sFL;
    SInt                sNoBlocks;
    SInt                i;
    ULong               sOffset;
    ULong               sAllocSize;

    IDE_DASSERT(mChunkSize > IDU_SMALL_MAXSIZE);
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_ID_CHUNK, mChunkSize, (void**)&sChunk)
             != IDE_SUCCESS);

    sFL = calcFL(aSize);
    sChunk->mParent = this;
    sChunk->mChunkSize = mChunkSize;
    sChunk->mBlockSize = aSize;

    sNoBlocks = (mChunkSize - sizeof(iduMemSmallChunk)
                            + sizeof(iduMemSmallHeader))
                / aSize;

    /* Initialize blocks */
    sOffset = offsetof(iduMemSmallChunk, mFirst);
    sAllocSize = aSize | IDU_TLSF_SMALL;

    sHeader = &(sChunk->mFirst);
    sHeader->mAllocSize = (sOffset << 32) | sAllocSize;
    sHeader->h.mNext = mFrees[sFL];

    for(i = 1; i < sNoBlocks; i++)
    {
        sHeaderPrev = sHeader;

        sOffset += aSize;
        sHeader = (iduMemSmallHeader*)((SChar*)sChunk + sOffset);
        sHeader->h.mNext = sHeaderPrev;
        sHeader->mAllocSize = (sOffset << 32) | sAllocSize;
    }

    /* Last one will be new top */
    mFrees[sFL] = sHeader;

    sChunk->mNext = mChunks;
    mChunks = sChunk;

    mPoolSize += mChunkSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemSmall::dumpMemory(PDL_HANDLE aFile, UInt aDumpLevel)
{
    const ULong         sF0 = ID_ULONG(0x00000000FFFFFFF0);
    size_t              sLen;
    iduMemSmallChunk*   sChunk;
    iduMemSmallHeader*  sBlock;
    SInt                i;
    SInt                sNoBlocks;
    ULong               sOffset;
    ULong               sSize;

    SChar               sLine[256];

    PDL_UNUSED_ARG(aDumpLevel);

    IDE_ASSERT(lock() == IDE_SUCCESS);

    idlOS::snprintf(sLine, sizeof(sLine), "=== Dump %s begin ===\n", mName);
    IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

    /* dump myself */
    idlOS::write(aFile, gDumpHeader, sizeof(gDumpHeader));
    idlOS::write(aFile, gAllocHeader, sizeof(gAllocHeader));
    sLen = idlOS::snprintf(sLine, sizeof(sLine), gAllocBody,
                           mName, this,
                           (ULong)mPoolSize / 1024,
                           (ULong)mUsedSize / 1024);
    idlOS::write(aFile, sLine, sLen);

    sChunk = mChunks;

    while(sChunk != NULL)
    {
        idlOS::snprintf(sLine, sizeof(sLine), "Dump chunk %p of %s.\n", sChunk, mName);
        IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

        idlOS::write(aFile, gChunkHeader, sizeof(gChunkHeader));
        sLen = idlOS::snprintf(sLine, sizeof(sLine), gChunkBody,
                               sChunk, (ULong)mChunkSize / 1024);
        idlOS::write(aFile, sLine, sLen);

        sNoBlocks = (mChunkSize - sizeof(iduMemSmallChunk)
                     + sizeof(iduMemSmallHeader))
            / sChunk->mBlockSize;

        sOffset = offsetof(iduMemSmallChunk, mFirst);

        for(i = 0; i < sNoBlocks; i++)
        {
            sBlock   = (iduMemSmallHeader*)((SChar*)sChunk + sOffset +
                                            (sChunk->mBlockSize * i));
            sSize    = sBlock->mAllocSize & sF0;

            if((sBlock->mAllocSize & IDU_TLSF_USED) == 0)
            {
                /* fall through */
            }
            else
            {
                idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                       MAKECOLOR(sBlock->h.a.mClientIndex),
                                       mMemInfo[sBlock->h.a.mClientIndex].mName,
                                       MAKECOLOR(sBlock->h.a.mClientIndex),
                                       sBlock,
                                       MAKECOLOR(sBlock->h.a.mClientIndex),
                                       sSize);
                idlOS::write(aFile, sLine, sLen);
                idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));
            }
        }

        idlOS::write(aFile, gChunkTailer, sizeof(gChunkTailer));

        sChunk = sChunk->mNext;
    }

    idlOS::write(aFile, gAllocTailer, sizeof(gAllocTailer));
    idlOS::write(aFile, gDumpTailer, sizeof(gDumpTailer));

    idlOS::snprintf(sLine, sizeof(sLine), "=== Dump %s done ===\n", mName);
    IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

    IDE_ASSERT(unlock() == IDE_SUCCESS);
    return IDE_SUCCESS;
}

/***********************************************************************
 * TLSF implementations
 * Maximum available allocation size :
 * 2^52=4Petabytes
 **********************************************************************/

IDE_RC iduMemTlsf::initialize(SChar*            aName,
                              idBool            aAutoShrink,
                              ULong             aChunkSize)
{
    SInt i;
    SInt j;

    IDE_TEST(iduMemAlloc::initialize(aName) != IDE_SUCCESS);

    mOwner  = ID_ULONG(0xFFFFFFFFFFFFFFFF);
    mLocked = 0;

    mAutoShrink     = aAutoShrink;
    mChunkSize      = aChunkSize;

    mSpinCount = iduMemMgr::getSpinCount();
    mSpinCount = (mSpinCount == -1)? 0 : mSpinCount;

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_TEST(idlOS::thread_mutex_init(&mLock) != 0);
#endif

    mChunk          = NULL;
    mLargeBlocks    = NULL;
    mFLBitmap       = 0;
    idlOS::memset(mSLBitmap, 0, sizeof(ULong) * IDU_TLSF_FL);

    for(i = 0; i < IDU_TLSF_FL; i++)
    {
        for(j = 0; j < IDU_TLSF_SL; j++)
        {
            mFreeList[i][j].h.f.mPrev = &(mFreeList[i][j]);
            mFreeList[i][j].h.f.mNext = &(mFreeList[i][j]);
        }
    }

    idlOS::strcpy(mMemInfo[IDU_MEM_RESERVED].mName, "RESERVED");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsf::destroy()
{
    /* Code to free all chunks due to the flag */
    iduMemTlsfChunk* sChunk;
    iduMemTlsfChunk* sNextChunk;

    if(mChunk != NULL)
    {
        sChunk = mChunk->mNext;

        while(sChunk != mChunk)
        {
            sNextChunk = sChunk->mNext;
            IDE_TEST(freeChunk(sChunk) != IDE_SUCCESS);
            sChunk = sNextChunk;
        }

        IDE_TEST(freeChunk(mChunk) != IDE_SUCCESS);
    }
    else
    {
        /* This allocator does not have any memory chunk */
    }

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_TEST(idlOS::thread_mutex_destroy(&mLock) != 0);
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsf::malloc(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                          void**                aPtr)
{
    iduMemTlsfHeader*   sHeader;
    iduMemTlsfHeader*   sRemainder;
    ULong               sSize;

    IDE_DASSERT(aPtr != NULL);
    
    /* make size suitable */
    sSize = calcAllocSize(aSize);
        
    IDE_ASSERT(lock() == IDE_SUCCESS);
    if(sSize >= mChunkSize / 2)
    {
        IDE_TEST(mallocLarge(sSize, (void**)&sHeader) != IDE_SUCCESS);

        sHeader->mAllocSize = sSize;
        sHeader->h.a.mParent    = this;

        sRemainder = (iduMemTlsfHeader*)((SChar*)sHeader + sSize);
        sRemainder->mPrevBlock = sHeader;
    }
    else
    {
        sHeader = popBlock(sSize);
        while(sHeader == NULL)
        {
            IDE_TEST_RAISE(expand() != IDE_SUCCESS, ECANNOTEXPAND);
            sHeader = popBlock(sSize);
        }

        split(sHeader, sSize, &sRemainder);

        if(sRemainder != NULL)
        {
            IDE_DASSERT((ULong)sRemainder - (ULong)sHeader == sSize);
            (void)mergeRight(sRemainder);

            /* Clear used bit of remainder block */
            sRemainder->mAllocSize &= ~IDU_TLSF_USED;
            /* Set prevused bit of remainder block */
            sRemainder->mAllocSize |=  IDU_TLSF_PREVUSED;
            insertBlock(sRemainder, ID_TRUE);

            /* Clear prevused bit of right block */
            sRemainder = getRightBlock(sRemainder);
            sRemainder->mAllocSize &= ~IDU_TLSF_PREVUSED;
        }
        else
        {
            /* No split occured */
            /* Set prevused bit of right block */
            sRemainder = getRightBlock(sHeader);
            sRemainder->mAllocSize |=  IDU_TLSF_PREVUSED;
        }
    }

    /* Set used bit of current header */
    sHeader->mAllocSize     |= IDU_TLSF_USED;
    sHeader->h.a.mClientIndex    = aIndex;
    sHeader->h.a.mParent         = this;

    statupdate(aIndex, sSize, 1);

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    *aPtr = (void*)(sHeader + 1);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ECANNOTEXPAND)
    {
    }

    IDE_EXCEPTION_END;
        
    IDE_ASSERT(unlock() == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduMemTlsf::malign(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                          ULong                 aAlign,
                          void**                aPtr)
{
    iduMemTlsfHeader*   sHeader;
    iduMemTlsfHeader*   sRight;
    iduMemTlsfHeader*   sRemainder;

    ULong               sAllocSize;
    ULong               sBlockSize;
    ULong               sLeftSplitSize;
    ULong               sBlockSplitSize;
    ULong               sAddr;

    IDE_DASSERT(aPtr != NULL);
    /* The FFS and FLS of a power of 2 should be the same */
    IDE_DASSERT((aAlign >= sizeof(iduMemLibcHeader)) && (aAlign % sizeof(void*) == 0));
    IDE_DASSERT(acpBitFfs64(aAlign) == acpBitFls64(aAlign));
    
    /* make size suitable */
    sBlockSize = calcAllocSize(aSize);
    sAllocSize = idlOS::alignLong(aSize + aAlign
                                        + sizeof(iduMemTlsfHeader) * 2,
                                  IDU_TLSF_SIZEGAP);
        
    IDE_ASSERT(lock() == IDE_SUCCESS);

    if(sAllocSize >= mChunkSize / 2)
    {
        IDE_TEST(mallocLarge(sAllocSize, (void**)&sHeader) != IDE_SUCCESS);

        sHeader->h.a.mClientIndex = (UInt)aIndex;
        sHeader->h.a.mParent      = this;
        sHeader->mAllocSize       = sAllocSize | IDU_TLSF_USED;

        sRight = (iduMemTlsfHeader*)((SChar*)sHeader + sAllocSize);
        sRight->mPrevBlock = sHeader;

        sLeftSplitSize = aAlign - (((ULong)(sHeader + 1)) % aAlign);

        if(sLeftSplitSize == 0)
        {
            /* Align matches. Do nothing*/
        }
        else
        {
            /* align mismatches - match align by hand */
            sAddr = (ULong)(sHeader + 1) + aAlign;
            sAddr -= (sAddr % aAlign);

            /* Reserve space for second block header */
            sHeader = (iduMemTlsfHeader*)sAddr;
            sHeader--;

            /* Set tailer as the last part of block */
            sHeader->h.a.mClientIndex = aIndex;
            sHeader->mAllocSize   = sAllocSize - sLeftSplitSize;
        }
            
        *aPtr = (void*)(sHeader + 1);
        statupdate(aIndex, sAllocSize, 1);
    }
    else
    {
        sHeader = popBlock(sAllocSize);
        while(sHeader == NULL)
        {
            IDE_TEST_RAISE(expand() != IDE_SUCCESS, ECANNOTEXPAND);
            sHeader = popBlock(sAllocSize);
        }

        IDE_TEST(sHeader == NULL);
        sLeftSplitSize = aAlign - (((ULong)(sHeader + 1)) % aAlign);

        if(sLeftSplitSize == 0)
        {
            /* Align match - Split right */
            split(sHeader, sBlockSize, &sRemainder);
        }
        else
        {
            /* Align mismatch - Split left first */
            split(sHeader, sLeftSplitSize, &sRemainder);
            /* Clear used bit */
            sHeader->mAllocSize &= ~(IDU_TLSF_USED);

            /* split occured. push back sHeader */
            (void)mergeLeft(sHeader, &sHeader);
            insertBlock(sHeader, ID_TRUE);

            sHeader = sRemainder;

            sBlockSplitSize = sBlockSize;

            /* Split right */
            split(sHeader, sBlockSplitSize, &sRemainder);
        }
    
        /* Set used bit */
        sHeader->mAllocSize    |= IDU_TLSF_USED;
        sHeader->h.a.mClientIndex   = aIndex;
        sHeader->h.a.mParent        = this;

        statupdate(aIndex, IDU_TLSF_ALLOCSIZE(sHeader), 1);

        *aPtr = (void*)(sHeader + 1);

        if(sRemainder != NULL)
        {
            /* Clear used, set prev used */
            sRemainder->mAllocSize &= ~(IDU_TLSF_USED);
            sRemainder->mAllocSize |= IDU_TLSF_PREVUSED;

            (void)mergeRight(sRemainder);
            insertBlock(sRemainder, ID_TRUE);

            sRemainder = getRightBlock(sRemainder);
            sRemainder->mAllocSize &= ~IDU_TLSF_PREVUSED;
        }
        else
        {
            /* no split */
            sRemainder = getRightBlock(sHeader);
            sRemainder->mAllocSize |=  IDU_TLSF_PREVUSED;
        }
    }
        
    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ECANNOTEXPAND)
    {
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(unlock() == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduMemTlsf::free(void* aPtr)
{
    iduMemTlsfHeader*       sHeader;
    iduMemTlsfHeader*       sRight;
    idBool                  sIsLeftMerged;
    idBool                  sIsRightMerged;
    idBool                  sFreeChunk;
    ULong                   sSize;
    iduMemoryClientIndex    sIndex;

    sHeader = getHeader(aPtr);

    if(sHeader->h.a.mParent != this)
    {
        return sHeader->h.a.mParent->free(aPtr);
    }
    IDE_TEST_RAISE(IDU_TLSF_IS_BLOCK_USED(sHeader) == ID_FALSE, EDOUBLEFREE);

    sIndex  = (iduMemoryClientIndex)sHeader->h.a.mClientIndex;
    sSize   = IDU_TLSF_ALLOCSIZE(sHeader);

    IDE_ASSERT(lock() == IDE_SUCCESS);

    if(sSize >= mChunkSize / 2)
    {
        IDE_TEST(freeLarge(sHeader, sSize) != IDE_SUCCESS);
    }
    else
    {
        sIsLeftMerged  = mergeLeft(sHeader, &sHeader);
        sIsRightMerged = mergeRight(sHeader);

        sRight = getRightBlock(sHeader);
            
        /* Clear used bit */
        sHeader->mAllocSize &= ~IDU_TLSF_USED;
        sRight->mAllocSize  &= ~IDU_TLSF_PREVUSED;

        /* Check whether this chunk is blank */
        if((mAutoShrink == ID_TRUE) &&
           ((sHeader->mPrevBlock->mAllocSize & IDU_TLSF_SENTINEL) != 0) &&
           ((sRight->mAllocSize & IDU_TLSF_SENTINEL) != 0))
        { 
            iduMemTlsfChunk* sChunk;
            sChunk = (iduMemTlsfChunk*)((SChar*)sHeader - sizeof(iduMemTlsfChunk)
                                        + sizeof(iduMemTlsfHeader));

            IDE_DASSERT(sChunk->mChunkSize == mChunkSize);

            if(mChunk == sChunk)
            {
                if(mChunk->mNext == mChunk)
                {
                    /*
                     * This is the only chunk
                     * Do not free
                     */
                    sFreeChunk = ID_FALSE;
                }
                else
                {
                    mChunk = mChunk->mNext;
                    sFreeChunk = ID_TRUE;
                }
            }
            else
            {
                    sFreeChunk = ID_TRUE;
            }

            if(sFreeChunk == ID_TRUE)
            {
                sChunk->mNext->mPrev = sChunk->mPrev;
                sChunk->mPrev->mNext = sChunk->mNext;
                freeChunk(sChunk);
            }
            else
            {
                /* Only chunk should remain */
            }
        }
        else
        {
            insertBlock(sHeader, 
                        ((sIsLeftMerged == ID_TRUE) ||
                         (sIsRightMerged == ID_TRUE))?
                        ID_TRUE : ID_FALSE);
        }
        
    }
    statupdate(sIndex, -sSize, -1);

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(EDOUBLEFREE);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_DOUBLEFREE));
        IDE_ASSERT(0);
    }

    IDE_EXCEPTION_END;
    IDE_ASSERT(unlock() == IDE_SUCCESS);
    return IDE_FAILURE;
}

IDE_RC iduMemTlsf::freeStatic(void* aPtr)
{
    iduMemTlsfHeader*       sHeader;
    sHeader = getHeader(aPtr);

    return sHeader->h.a.mParent->free(aPtr);
}

IDE_RC iduMemTlsf::lock(void)
{
    ULong sOwner = (ULong)idlOS::thr_self();
    SInt  i;

    if(mOwner == sOwner)
    {
        mLocked++;
        return IDE_SUCCESS;
    }
    else
    {
#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
        IDE_TEST(idlOS::thread_mutex_lock(&mLock) != 0);
        mOwner = sOwner;
        mLocked = 1;
#else
        while(1)
        {
            for(i = 0; i < mSpinCount; i++)
            {
                if((ULong)acpAtomicCas64(&mOwner, sOwner, ID_ULONG(0xFFFFFFFFFFFFFFFF))
                   == ID_ULONG(0xFFFFFFFFFFFFFFFF))
                {
                    mLocked = 1;
                    return IDE_SUCCESS;
                }
                else
                {
                    /* continue */
                }
            }
            idlOS::thr_yield();
        }
#endif
    }

    return IDE_SUCCESS;

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

IDE_RC iduMemTlsf::unlock(void)
{
    IDE_DASSERT(mOwner == (ULong)idlOS::thr_self());
    IDE_DASSERT(mLocked > 0);
    mLocked--;

    if(mLocked == 0)
    {
        mOwner = ID_ULONG(0xFFFFFFFFFFFFFFFF);
#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
        IDE_TEST(idlOS::thread_mutex_unlock(&mLock) != 0);
#endif
    }
    else
    {
        /* Still lock got */
    }

    return IDE_SUCCESS;

#if defined(ALTI_CFG_OS_AIX) || defined(ALTI_CFG_OS_SOLARIS) || defined(POWERPC64_LINUX)
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

iduMemTlsfHeader* iduMemTlsf::popBlock(ULong aSize)
{
    const ULong sFF = ID_ULONG(0xFFFFFFFFFFFFFFFF);

    ULong               sTemp;
    SInt                sFL;
    SInt                sSL;
    iduMemTlsfHeader*   sRet;

    calcFLSL(&sFL, &sSL, aSize);

    sTemp = mSLBitmap[sFL] & (sFF << sSL);

    if(sTemp != 0)
    {
        /* Gotcha! */
        sSL = acpBitFfs64(sTemp);
        sRet = mFreeList[sFL][sSL].h.f.mNext;
        
        if((sRet->mAllocSize < aSize) || (sRet == &(mFreeList[sFL][sSL])))
        {
            sRet = NULL;
        }
        else
        {
            /* Gotcha! */
        }
    }
    else
    {
        sRet = NULL;
    }

    if(sRet == NULL)
    {
        /* Find larger block */
        sFL = acpBitFls64(mFLBitmap & (sFF << (sFL + 1)));

        if(sFL == -1)
        {
            /* No larger block */
            sRet = NULL;
        }
        else
        {
            /* Smallest larger block */
            sSL  = acpBitFfs64(mSLBitmap[sFL]);
            sRet = mFreeList[sFL][sSL].h.f.mNext;
        }
    }

    if(sRet != NULL)
    {
        /* Remove the block from free list */
        IDE_DASSERT(sRet != &(mFreeList[sFL][sSL]));
        removeBlock(sRet, sFL, sSL);
    }
    else
    {
        /* Do nothing */
    }

    return sRet;
}

void iduMemTlsf::insertBlock(iduMemTlsfHeader* aHeader,
                             idBool            aBefore)
{
    SInt    sFL;
    SInt    sSL;
    ULong   sSize;

    IDE_DASSERT(IDU_TLSF_IS_BLOCK_FREE(aHeader) == ID_TRUE);

    if(aHeader != NULL)
    {
        sSize = IDU_TLSF_ALLOCSIZE(aHeader);
        calcFLSL(&sFL, &sSL, sSize);

        if(aBefore == ID_TRUE)
        {
            aHeader->h.f.mNext = &(mFreeList[sFL][sSL]);
            aHeader->h.f.mPrev = mFreeList[sFL][sSL].h.f.mPrev;
            aHeader->h.f.mPrev->h.f.mNext = aHeader;
            mFreeList[sFL][sSL].h.f.mPrev = aHeader;

            IDE_DASSERT(mFreeList[sFL][sSL].h.f.mNext != &(mFreeList[sFL][sSL]));
            IDE_DASSERT(mFreeList[sFL][sSL].h.f.mPrev != &(mFreeList[sFL][sSL]));
        }
        else
        {
            aHeader->h.f.mPrev = &(mFreeList[sFL][sSL]);
            aHeader->h.f.mNext = mFreeList[sFL][sSL].h.f.mNext;
            aHeader->h.f.mNext->h.f.mPrev = aHeader;
            mFreeList[sFL][sSL].h.f.mNext = aHeader;

            IDE_DASSERT(mFreeList[sFL][sSL].h.f.mNext != &(mFreeList[sFL][sSL]));
            IDE_DASSERT(mFreeList[sFL][sSL].h.f.mPrev != &(mFreeList[sFL][sSL]));
        }

        mSLBitmap[sFL]  |= ID_ULONG(1) << sSL;
        mFLBitmap       |= ID_ULONG(1) << sFL;
    }
    else
    {
        /* Trying to insert NULL. Just return */
    }
}

void iduMemTlsf::removeBlock(iduMemTlsfHeader* aHeader,
                             SInt aFL, SInt aSL)
{
    SInt sFL;
    SInt sSL;

    aHeader->h.f.mNext->h.f.mPrev = aHeader->h.f.mPrev;
    aHeader->h.f.mPrev->h.f.mNext = aHeader->h.f.mNext;

    if(aFL == -1 || aSL == -1)
    {
        calcFLSL(&sFL, &sSL, aHeader->mAllocSize);
    }
    else
    {
        sFL = aFL;
        sSL = aSL;
    }

    if(mFreeList[sFL][sSL].h.f.mNext == &(mFreeList[sFL][sSL]))
    {
        /* This matrix entry is empty */
        mSLBitmap[sFL] &= ~(ID_ULONG(1) << sSL);

        if(mSLBitmap[sFL] == ID_ULONG(0))
        {
            /* Second matrix entry is empty */
            mFLBitmap &= ~(ID_ULONG(1) << sFL);
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        /* Do nothing */
    }
}

void iduMemTlsf::calcFLSL(SInt* aFL, SInt* aSL, ULong aSize)
{
    SInt sMSB;
    SInt sTemp;

    /* Smallest block is 32bytes */
    IDE_DASSERT(aSize >= 32);
    /* Cannot allocate larger than 2EB block */
    IDE_DASSERT(aSize < (ID_ULONG(1)<<61));

    if(aSize <= IDU_TLSF_MAXSMALL)
    {
        sTemp  = aSize / IDU_TLSF_SIZEGAP;
        *aFL = sTemp / IDU_TLSF_SL;
        *aSL = sTemp % IDU_TLSF_SL;
    }
    else
    {
        if(aSize < IDU_TLSF_MAXNEXT)
        {
            /*
             * Smaller than first large block = 8192 + 512
             * Consider them as all 8192
             */
            sTemp  = IDU_TLSF_MAXSMALL / IDU_TLSF_SIZEGAP;
            *aFL = sTemp / IDU_TLSF_SL;
            *aSL = sTemp % IDU_TLSF_SL;
        }
        else
        {
            sMSB = acpBitFls64(aSize);
            sTemp = (aSize >> (sMSB - 6)) & 0x3F;

            if(sTemp == 0)
            {
                *aFL = sMSB + 2;
                *aSL = IDU_TLSF_SL - 1;
            }
            else
            {
                *aFL = sMSB + 3;
                *aSL = sTemp;
            }
        }
    }
}

ULong iduMemTlsf::calcAllocSize(ULong aSize)
{
    ULong   sSize;
        
    /* match size with 32bytes */
    sSize = idlOS::alignLong(aSize + sizeof(iduMemTlsfHeader)
                                   + sizeof(iduMemTlsfHeader*),
                                   32);
    sSize = IDL_MAX(sSize, 64);
    return sSize;
}


iduMemTlsfHeader* iduMemTlsf::getHeader(void* aPtr)
{
    iduMemTlsfHeader*  sHeader;
    iduMemTlsfHeader*  sRight;

    sHeader = (iduMemTlsfHeader*)aPtr;
    sHeader--;

    sRight = (iduMemTlsfHeader*)((SChar*)sHeader + IDU_TLSF_ALLOCSIZE(sHeader));
    if(sHeader == sRight->mPrevBlock)
    {
        /*
         * Allocated by malloc.
         * Just do nothing
         */
    }
    else
    {
        /*
         * Large block allocated by malign
         */
        sHeader = sRight->mPrevBlock;

        IDE_DASSERT((ULong)sHeader + IDU_TLSF_ALLOCSIZE(sHeader) == (ULong)sRight);
    }

    return sHeader;
}

iduMemTlsfHeader* iduMemTlsf::getLeftBlock(iduMemTlsfHeader* aHeader)
{
    /* sHeader containes the address of left block */
    IDE_DASSERT((aHeader->mAllocSize & IDU_TLSF_SENTINEL) == 0);
    return aHeader->mPrevBlock;
}

iduMemTlsfHeader* iduMemTlsf::getRightBlock(iduMemTlsfHeader* aHeader)
{
    /* The position just after current tailer contains the
     * header of the right block */
    iduMemTlsfHeader* sHeader;

    IDE_DASSERT((aHeader->mAllocSize & IDU_TLSF_SENTINEL) == 0);
    sHeader = (iduMemTlsfHeader*)((SChar*)aHeader + 
                                  IDU_TLSF_ALLOCSIZE(aHeader));

    return sHeader;
}

void iduMemTlsf::split(iduMemTlsfHeader*    aBlock,
                       ULong                aSize,
                       iduMemTlsfHeader**   aRemainder)
{
    iduMemTlsfHeader*   sRemainder;
    iduMemTlsfHeader*   sRight;
    ULong               sExtra;
    ULong               sOrig;

    IDE_DASSERT(aBlock != NULL);
    IDE_DASSERT(aRemainder != NULL);
    IDE_DASSERT((ULong)aBlock % 32 == 0);
    IDE_DASSERT(aSize >= IDU_TLSF_SIZEGAP);
    IDE_DASSERT(aSize % IDU_TLSF_SIZEGAP == 0);

    /*
     * split block
     * [HEADER][BLOCK                  ][RIGHT->HEADER] ->
     * [HEADER][BLOCK][REMAINDER][BLOCK][RIGHT->REMAINDER]
     */
    sOrig  = IDU_TLSF_ALLOCSIZE(aBlock);
    sExtra = sOrig - aSize;

    if(sExtra != 0)
    {
        sRight = getRightBlock(aBlock);

        aBlock->mAllocSize -= sExtra;

        sRemainder  = (iduMemTlsfHeader*)((SChar*)aBlock + aSize);
        sRemainder->mAllocSize = sExtra;
        sRemainder->mPrevBlock = aBlock;

        sRight->mPrevBlock = sRemainder;

        IDE_DASSERT((ULong)sRemainder % 32 == 0);

        *aRemainder = sRemainder;
    }
    else
    {
        *aRemainder = NULL;
    }
}

idBool iduMemTlsf::mergeLeft(iduMemTlsfHeader*     aHeader,
                             iduMemTlsfHeader**    aNewHeader)
{
    idBool              sRet;
    iduMemTlsfHeader*   sLeft;
    iduMemTlsfHeader*   sRight;

    ULong               sCurSize;

    if((aHeader->mAllocSize & IDU_TLSF_PREVUSED) != 0)
    {
        /* Previous block is used. */
        *aNewHeader = aHeader;
        sRet = ID_FALSE;
    }
    else
    {
        sCurSize    = IDU_TLSF_ALLOCSIZE(aHeader);
        sLeft       = getLeftBlock(aHeader);

        /*
         * merge
         * [LEFT(size1)      ][BLOCK][HDR(size2)->LEFT][BLK][RIGHT->HDR] ->
         * [LEFT(size1+size2)][BLOCK                       ][RIGHT->LEFT]
         */
        sRight      = getRightBlock(aHeader);

        removeBlock(sLeft);
        sLeft->mAllocSize += sCurSize;
        sRight->mPrevBlock = sLeft;

        *aNewHeader = sLeft;
        sRet = ID_TRUE;
    }

    return sRet;
}

idBool iduMemTlsf::mergeRight(iduMemTlsfHeader* aHeader)
{
    idBool              sRet;

    iduMemTlsfHeader*   sRight;
    ULong               sRightSize;

    iduMemTlsfHeader*   sRightRight;

    sRight      = getRightBlock(aHeader);

    if(IDU_TLSF_IS_BLOCK_FREE(sRight))
    {
        /*
         * merge
         * [HDR(size1)      ][BLOCK][RGH(size2)->HDR][BLOCK][RRH->RGH] ->
         * [HDR(size1+size2)][BLOCK                        ][RRH->HDR]
         */

        sRightSize  = IDU_TLSF_ALLOCSIZE(sRight);
        sRightRight = getRightBlock(sRight);
        removeBlock(sRight);

        /* merge */
        aHeader->mAllocSize += sRightSize;
        sRightRight->mPrevBlock = aHeader;
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

IDE_RC iduMemTlsf::expand(void)
{
    iduMemTlsfChunk*    sChunk;
    iduMemTlsfHeader*   sTailer;

    IDE_TEST(allocChunk(&sChunk) != IDE_SUCCESS);

    sChunk->mChunkSize = mChunkSize;
    sTailer = (iduMemTlsfHeader*)((SChar*)sChunk
                                  - sizeof(iduMemTlsfHeader)
                                  + mChunkSize);

    if(mChunk == NULL)
    {
        sChunk->mNext = sChunk;
        sChunk->mPrev = sChunk;

        mChunk = sChunk;
    }
    else
    {
        sChunk->mNext = mChunk;
        sChunk->mPrev = mChunk->mPrev;

        mChunk->mPrev = sChunk;
        sChunk->mPrev->mNext = sChunk;
    }

    /*
     * Setup chunk as
     * [HEADER][SENTINEL][FIRST][F..R..E..E..B..L..O..C..K][SENTINEL]
     * Chunk is mmap'ed. So, the starting address is always aligned by
     * 4096 or 8192, with regard to the OS.
     * Size of header and sentinel, first are 32.
     * So, the allocated memory always will be aligned by 32bytes.
     */

    /* Setup leftmost sentinel */
    sChunk->mSentinel.mAllocSize = sizeof(iduMemTlsfHeader) |
        IDU_TLSF_SENTINEL | IDU_TLSF_USED;
    sChunk->mSentinel.h.a.mClientIndex = IDU_MEM_SENTINEL;
    sChunk->mSentinel.mPrevBlock = NULL;
    sChunk->mSentinel.h.f.mPrev = NULL;
    sChunk->mSentinel.h.f.mNext = NULL;

    /* Plus Minus Zero */
    sChunk->mFirst.mAllocSize = (mChunkSize - sizeof(iduMemTlsfChunk))
                                | IDU_TLSF_PREVUSED;
    sChunk->mFirst.mPrevBlock = &(sChunk->mSentinel);

    /* Setup rightmost sentinel */
    sTailer->mAllocSize = sizeof(iduMemTlsfHeader) |
        IDU_TLSF_SENTINEL | IDU_TLSF_USED;
    sTailer->h.a.mClientIndex = IDU_MEM_SENTINEL;
    sTailer->mPrevBlock = &(sChunk->mFirst);
    sTailer->h.f.mPrev = sTailer->h.f.mNext = NULL;

    insertBlock(&(sChunk->mFirst), ID_TRUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsf::dumpMemory(PDL_HANDLE aFile, UInt aDumpLevel)
{
    const ULong         sF0 = ID_ULONG(0xFFFFFFFFFFFFFFF0);

    size_t              sLen;
    iduMemTlsfChunk*    sChunk;
    iduMemTlsfHeader*   sBlock;
    ULong               sSize;
    ULong               sOffset;
    ULong               sAccSize;
    UInt                sClientIndex;
    void*               sBeginAddr;
    
    SChar               sLine[256];

    IDE_ASSERT(lock() == IDE_SUCCESS);

    idlOS::snprintf(sLine, sizeof(sLine), "=== Dump %s begin ===\n", mName);
    IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

    /* dump myself */
    idlOS::write(aFile, gAllocHeader, sizeof(gAllocHeader));
    sLen = idlOS::snprintf(sLine, sizeof(sLine), gAllocBody,
                           mName, this,
                           mPoolSize / 1024,
                           mUsedSize / 1024);
    idlOS::write(aFile, sLine, sLen);

    sChunk = mChunk;

    if(sChunk != NULL)
    {
        do
        {
            idlOS::snprintf(sLine, sizeof(sLine), "Dump chunk %p of %s.\n",
                            sChunk, mName);
            IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

            idlOS::write(aFile, gChunkHeader, sizeof(gChunkHeader));
            sLen = idlOS::snprintf(sLine, sizeof(sLine), gChunkBody,
                                   sChunk, (ULong)mChunkSize / 1024);
            idlOS::write(aFile, sLine, sLen);

            sOffset         = offsetof(iduMemTlsfChunk, mFirst);
            sBlock          = (iduMemTlsfHeader*)((SChar*)sChunk + sOffset);
            sAccSize        = ID_ULONG(0);
            sBeginAddr      = NULL;
            sClientIndex    = (UInt)IDU_MAX_CLIENT_COUNT;

            do
            {
                sSize    = sBlock->mAllocSize & sF0;

                switch(aDumpLevel)
                {
                case 1:
                    if((sBlock->mAllocSize & IDU_TLSF_USED) == 0)
                    {
                        if(sBeginAddr != NULL)
                        {
                            idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                            sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                                   0xF0F0F0, "USED",
                                                   0xF0F0F0, sBeginAddr,
                                                   0xF0F0F0, sAccSize);
                            idlOS::write(aFile, sLine, sLen);
                            idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));
                        }
                        else
                        {
                            /* Do not print */
                        }

                        idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                        sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                               0xD0D0D0, "FREE",
                                               0xD0D0D0, sBlock,
                                               0xD0D0D0, sSize);
                        idlOS::write(aFile, sLine, sLen);
                        idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));

                        sAccSize = ID_ULONG(0);
                        sBeginAddr = NULL;
                    }
                    else
                    {
                        if((sBlock->mAllocSize & IDU_TLSF_SENTINEL) == 0)
                        {
                            sBeginAddr = (sBeginAddr==NULL)? sBlock:sBeginAddr;
                            sAccSize += sSize;
                        }
                        else if(sBeginAddr != NULL)
                        {
                            idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                            sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                                   0xF0F0F0, "USED",
                                                   0xF0F0F0, sBeginAddr,
                                                   0xF0F0F0, sAccSize);
                            idlOS::write(aFile, sLine, sLen);
                            idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));
                        }
                        else
                        {
                            /* No block to log */
                        }
                    }
                    break;

                case 2:
                    if((sBlock->mAllocSize & IDU_TLSF_USED) == 0)
                    {
                        if(sClientIndex != IDU_MAX_CLIENT_COUNT)
                        {
                            idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                            sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                                   MAKECOLOR(sClientIndex),
                                                   mMemInfo[sClientIndex].mName,
                                                   MAKECOLOR(sClientIndex),
                                                   sBeginAddr,
                                                   MAKECOLOR(sClientIndex),
                                                   sAccSize);
                            idlOS::write(aFile, sLine, sLen);
                            idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));
                        }
                        else
                        {
                            /* fall through */
                        }

                        idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                        sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                               0xD0D0D0, "FREE",
                                               0xD0D0D0, sBlock,
                                               0xD0D0D0, sSize);
                        idlOS::write(aFile, sLine, sLen);
                        idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));

                        sClientIndex    = (UInt)IDU_MAX_CLIENT_COUNT;
                    }
                    else
                    {
                        if(sClientIndex == IDU_MAX_CLIENT_COUNT)
                        {
                            sBeginAddr      = sBlock;
                            sAccSize        = sSize;
                            sClientIndex    = sBlock->h.a.mClientIndex;
                        }
                        else if(sClientIndex != sBlock->h.a.mClientIndex)
                        {
                            idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                            sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                                   MAKECOLOR(sClientIndex),
                                                   mMemInfo[sClientIndex].mName,
                                                   MAKECOLOR(sClientIndex),
                                                   sBeginAddr,
                                                   MAKECOLOR(sClientIndex),
                                                   sAccSize);
                            idlOS::write(aFile, sLine, sLen);
                            idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));

                            sBeginAddr      = sBlock;
                            sAccSize        = sSize;
                            sClientIndex    = sBlock->h.a.mClientIndex;
                        }
                        else
                        {
                            sAccSize       += sSize;
                        }
                    }
                    break;

                case 3:
                    idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
                    if((sBlock->mAllocSize & IDU_TLSF_USED) == 0)
                    {
                        sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                               0xD0D0D0, "FREE",
                                               0xD0D0D0, sBlock,
                                               0xD0D0D0, sSize);
                    }
                    else
                    {
                        sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                               MAKECOLOR(sBlock->h.a.mClientIndex),
                                               mMemInfo[sBlock->h.a.mClientIndex].mName,
                                               MAKECOLOR(sBlock->h.a.mClientIndex),
                                               sBlock,
                                               MAKECOLOR(sBlock->h.a.mClientIndex),
                                               sSize);
                    }
                    idlOS::write(aFile, sLine, sLen);
                    idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));
                    break;

                default:
                    IDE_DASSERT(0);
                    break;
                }

                sOffset += sSize;
                sBlock   = (iduMemTlsfHeader*)((SChar*)sChunk + sOffset);
            } while((sBlock->mAllocSize & IDU_TLSF_SENTINEL) == 0);

            idlOS::write(aFile, gChunkTailer, sizeof(gChunkTailer));

            sChunk = sChunk->mNext;
        } while(sChunk != mChunk);
    }
    else
    {
        /* fall through */
    }

    sChunk = mLargeBlocks;
            
    idlOS::snprintf(sLine, sizeof(sLine), "Dump large blocks of %s.\n", mName);
    IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

    if(sChunk != NULL)
    {
        do
        {
            sBlock = &(sChunk->mSentinel);
            sSize  = sBlock->mAllocSize & sF0;

            idlOS::write(aFile, gChunkHeader, sizeof(gChunkHeader));
            sLen = idlOS::snprintf(sLine, sizeof(sLine), gChunkBody,
                                   sChunk, sSize);
            idlOS::write(aFile, sLine, sLen);

            idlOS::write(aFile, gBlockHeader, sizeof(gBlockHeader));
            if((sBlock->mAllocSize & IDU_TLSF_USED) == 0)
            {
                sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                       0xD0D0D0, "FREE",
                                       0xD0D0D0, sBlock,
                                       0xD0D0D0, sSize);
            }
            else
            {
                switch(aDumpLevel)
                {
                case 1:
                    sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                           0xF0F0F0, "USED",
                                           0xF0F0F0, sBlock,
                                           0xF0F0F0, sSize);
                    break;
                case 2: case 3:
                    sLen = idlOS::snprintf(sLine, sizeof(sLine), gBlockBody,
                                           MAKECOLOR(sBlock->h.a.mClientIndex),
                                           mMemInfo[sBlock->h.a.mClientIndex].mName,
                                           MAKECOLOR(sBlock->h.a.mClientIndex),
                                           sBlock,
                                           MAKECOLOR(sBlock->h.a.mClientIndex),
                                           sSize);
                    break;
                default:
                    IDE_DASSERT(0);
                }
            }
            idlOS::write(aFile, sLine, sLen);
            idlOS::write(aFile, gBlockTailer, sizeof(gBlockTailer));

            idlOS::write(aFile, gChunkTailer, sizeof(gChunkTailer));

            sChunk = sChunk->mNext;
        } while(sChunk != mLargeBlocks);
    }
    else
    {
        /* fall through */
    }

    idlOS::write(aFile, gAllocTailer, sizeof(gAllocTailer));

    idlOS::snprintf(sLine, sizeof(sLine), "=== Dump %s done ===\n", mName);
    IDE_CALLBACK_SEND_SYM_NOLOG(sLine);

    IDE_ASSERT(unlock() == IDE_SUCCESS);
    return IDE_SUCCESS;
}

/***********************************************************************
 * iduMemTlsfMmap implementation
 **********************************************************************/

IDE_RC iduMemTlsfMmap::initialize(SChar*               aName,
                                  idBool               aAutoShrink,
                                  ULong                aChunkSize)
{
    IDE_TEST(iduMemTlsf::initialize(aName,
                                    aAutoShrink,
                                    aChunkSize)
             != IDE_SUCCESS);
    idlOS::strcpy(mType, "TLSF_MMAP");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfMmap::allocChunk(iduMemTlsfChunk** aChunk)
{
    void* sMem;

#if defined(VC_WIN32)
    sMem = iduMemMgr::mallocRaw(mChunkSize);
    IDE_TEST(sMem == NULL);
#else
    sMem = idlOS::mmap(NULL,
                       mChunkSize,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS,
                       PDL_INVALID_HANDLE);
    IDE_TEST(sMem == MAP_FAILED);
#endif
    *aChunk = (iduMemTlsfChunk*)sMem;
    mPoolSize += mChunkSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfMmap::freeChunk(iduMemTlsfChunk* aChunk)
{
#if defined(VC_WIN32)
    iduMemMgr::freeRaw(aChunk);
#else
    IDE_TEST(idlOS::munmap(aChunk, mChunkSize) != 0);
#endif
    mPoolSize -= mChunkSize;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC iduMemTlsfMmap::mallocLarge(ULong aSize, void** aPtr)
{
    ULong               sSize;
    iduMemTlsfChunk*    sMem;

    sSize = idlOS::alignLong(aSize + sizeof(iduMemTlsfChunk), IDU_PAGESIZE);

#if defined(VC_WIN32)
    sMem = (iduMemTlsfChunk*)iduMemMgr::mallocRaw(sSize);
    IDE_TEST(sMem == NULL);
#else
    sMem = (iduMemTlsfChunk*)
        idlOS::mmap(NULL,
                    sSize,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    PDL_INVALID_HANDLE);
    IDE_TEST(sMem == (iduMemTlsfChunk*)MAP_FAILED);
#endif
    sMem->mChunkSize = sSize;

    /* Link to large blocks list */
    if(mLargeBlocks == NULL)
    {
        sMem->mPrev = sMem;
        sMem->mNext = sMem;
        mLargeBlocks = sMem;
    }
    else
    {
        sMem->mPrev = mLargeBlocks;
        sMem->mNext = mLargeBlocks->mNext;

        sMem->mNext->mPrev  = sMem;
        mLargeBlocks->mNext         = sMem;
    }

    mPoolSize += sSize;
    *aPtr = (void*)(&(sMem->mSentinel));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfMmap::freeLarge(void* aPtr, ULong aSize)
{
    ULong               sSize;
    iduMemTlsfChunk*    sMem;

    PDL_UNUSED_ARG(aSize);
    /* Move back with the size of chunk header */
    sMem = (iduMemTlsfChunk*)((SChar*)aPtr - sizeof(iduMemTlsfChunk) +
                                             sizeof(iduMemTlsfHeader) * 2);
    sSize = sMem->mChunkSize;

    /* Remove from large blocks list */
    if(mLargeBlocks == sMem)
    {
        if(mLargeBlocks->mNext == mLargeBlocks)
        {
            /* only one large block */
            mLargeBlocks = NULL;
        }
        else
        {
            mLargeBlocks = mLargeBlocks->mNext;
            sMem->mPrev->mNext = sMem->mNext;
            sMem->mNext->mPrev = sMem->mPrev;
        }
    }
    else
    {
        sMem->mPrev->mNext = sMem->mNext;
        sMem->mNext->mPrev = sMem->mPrev;
    }

#if defined(VC_WIN32)
    iduMemMgr::freeRaw(sMem);
#else
    IDE_TEST(idlOS::munmap(sMem, sSize) != 0);
#endif
    mPoolSize -= sSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_MEMORY_DEALLOCATION));
    return IDE_FAILURE;
}

/***********************************************************************
 * iduMemTlsfLocal implementation
 **********************************************************************/

IDE_RC iduMemTlsfLocal::initialize(SChar*               aName,
                                   idBool               aAutoShrink,
                                   ULong                aChunkSize)
{
    IDE_TEST(iduMemTlsf::initialize(aName,
                                    aAutoShrink,
                                    aChunkSize)
             != IDE_SUCCESS);
    idlOS::strcpy(mType, "TLSF_LOCAL");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfLocal::malloc(iduMemoryClientIndex  aIndex,
                               ULong                 aSize,
                               void**                aPtr)
{
    ULong               sSize;

    IDE_DASSERT(aPtr != NULL);
    
    /* make size suitable */
    sSize = calcAllocSize(aSize);
        
    if(sSize >= mChunkSize / 2)
    {
        IDE_TEST(
            iduMemMgr::getGlobalAlloc()->malloc(aIndex, aSize, aPtr)
            != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(
            iduMemTlsf::malloc(aIndex, aSize, aPtr)
            != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfLocal::malign(iduMemoryClientIndex  aIndex,
                          ULong                 aSize,
                          ULong                 aAlign,
                          void**                aPtr)
{
    ULong               sAllocSize;

    /* The FFS and FLS of a power of 2 should be the same */
    /* make size suitable */
    sAllocSize = idlOS::alignLong(aSize + aAlign
                                        + sizeof(iduMemTlsfHeader) * 2,
                                  IDU_TLSF_SIZEGAP);
        
    if(sAllocSize >= mChunkSize / 2)
    {
        IDE_TEST(iduMemMgr::getGlobalAlloc()->malign(aIndex,
                                                     aSize,
                                                     aAlign,
                                                     aPtr)
                 != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemTlsf::malign(aIndex, aSize, aAlign, aPtr)
                 != IDE_SUCCESS);
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfLocal::allocChunk(iduMemTlsfChunk** aChunk)
{
    /* Allocate chunk from global instance */
    IDE_TEST(iduMemMgr::getGlobalAlloc()->malign(IDU_MEM_ID_CHUNK,
                                                 mChunkSize,
                                                 IDU_PAGESIZE,
                                                 (void**)aChunk)
             != IDE_SUCCESS);

    mPoolSize += mChunkSize;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfLocal::freeChunk(iduMemTlsfChunk* aChunk)
{
    /* Return chunk to global instance */
    IDE_TEST(iduMemMgr::getGlobalAlloc()->free(aChunk) != IDE_SUCCESS);

    mPoolSize -= mChunkSize;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * iduMemTlsfGlobal implementation
 **********************************************************************/

IDE_RC iduMemTlsfGlobal::initialize(SChar* aName, ULong aChunkSize)
{
    iduMemTlsfHeader*   sTailer;
    void* sMem;

    IDE_TEST(iduMemTlsf::initialize(aName,
                                    ID_FALSE,
                                    aChunkSize)
             != IDE_SUCCESS);
    idlOS::strcpy(mType, "TLSF_GLOBAL");

#if defined(VC_WIN32)
    sMem = iduMemMgr::mallocRaw(mChunkSize);
    IDE_TEST(sMem == NULL);
#else
    sMem = idlOS::mmap(NULL,
                       mChunkSize,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS,
                       PDL_INVALID_HANDLE);
    IDE_TEST(sMem == MAP_FAILED);
#endif
    mChunk = (iduMemTlsfChunk*)sMem;

    mChunk->mChunkSize = mChunkSize;
    sTailer = (iduMemTlsfHeader*)((SChar*)mChunk
                                  - sizeof(iduMemTlsfHeader)
                                  + mChunkSize);
    mChunk->mNext = mChunk;
    mChunk->mPrev = mChunk;

    /*
     * Setup chunk as
     * [HEADER][SENTINEL][FIRST][F..R..E..E..B..L..O..C..K][SENTINEL]
     * Chunk is mmap'ed. So, the starting address is always aligned by
     * 4096 or 8192, with regard to the OS.
     * Size of header and sentinel, first are 32.
     * So, the allocated memory always will be aligned by 32bytes.
     */

    /* Setup leftmost sentinel */
    mChunk->mSentinel.mAllocSize = sizeof(iduMemTlsfHeader) |
        IDU_TLSF_SENTINEL | IDU_TLSF_USED;
    mChunk->mSentinel.mPrevBlock = NULL;
    mChunk->mSentinel.h.f.mPrev = NULL;
    mChunk->mSentinel.h.f.mNext = NULL;

    /* Plus Minus Zero */
    mChunk->mFirst.mAllocSize = 
        (mChunkSize - sizeof(iduMemTlsfChunk)) | IDU_TLSF_PREVUSED;
    mChunk->mFirst.mPrevBlock = &(mChunk->mSentinel);

    /* Setup rightmost sentinel */
    sTailer->mAllocSize = sizeof(iduMemTlsfHeader) |
        IDU_TLSF_SENTINEL | IDU_TLSF_USED;
    sTailer->mPrevBlock = &(mChunk->mFirst);
    sTailer->h.f.mPrev = sTailer->h.f.mNext = NULL;

    mPoolSize += mChunkSize;
    insertBlock(&(mChunk->mFirst), ID_TRUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemTlsfGlobal::destroy(void)
{
#if defined(VC_WIN32)
    iduMemMgr::freeRaw(mChunk);
#else
    IDE_TEST(idlOS::munmap(mChunk, mChunkSize) != 0);
#endif
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* TLSF Allocators */
iduMemTlsfGlobal*    iduMemMgr::mTLSFGlobal;
ULong                iduMemMgr::mTLSFGlobalChunkSize;

/***********************************************************************
 * iduMemMgr functions
 **********************************************************************/
IDE_RC iduMemMgr::tlsf_initializeStatic(void)
{
    iduMemTlsf* sTlsf;
    SInt sNoChunks;
    SInt i;

    SChar sName[128];

    IDE_TEST(idp::read("MEMORY_ALLOCATOR_POOLSIZE_GLOBAL",
                       &mTLSFGlobalChunkSize)
             != IDE_SUCCESS);
    IDE_TEST(idp::read("MEMORY_ALLOCATOR_POOLSIZE",
                       &mTLSFLocalChunkSize)
             != IDE_SUCCESS);

    if(mTLSFGlobalChunkSize == 0)
    {
        /* No global limitation */
    }
    else
    {
        mTLSFGlobal = (iduMemTlsfGlobal*)iduMemMgr::mallocRaw(sizeof(iduMemTlsfGlobal));
        IDE_TEST(mTLSFGlobal == NULL);

        mTLSFGlobal = new(mTLSFGlobal) iduMemTlsfGlobal;
        IDE_TEST(mTLSFGlobal->initialize((SChar*)"GLOBAL_ALLOCATOR",
                                         mTLSFGlobalChunkSize)
                 != IDE_SUCCESS);
    }

    sNoChunks = idlVA::getProcessorCount();
    mNoAllocators = sNoChunks;
    mIsCPU = ID_TRUE;

    mTLSFLocal = (iduMemTlsf**)iduMemMgr::mallocRaw(sizeof(iduMemTlsf*) * sNoChunks);
    IDE_TEST(mTLSFLocal == NULL);

    for(i = 0; i < sNoChunks; i++)
    {
        if(mTLSFGlobalChunkSize == 0)
        {
            idlOS::snprintf(sName, sizeof(sName), "MMAP_%03d_ALLOCATOR", i);
            sTlsf = (iduMemTlsfMmap*)iduMemMgr::mallocRaw(sizeof(iduMemTlsfMmap));
            IDE_TEST(sTlsf == NULL);
            sTlsf = new(sTlsf) iduMemTlsfMmap;

        }
        else
        {
            idlOS::snprintf(sName, sizeof(sName), "LOCAL_%03d_ALLOCATOR", i);
            sTlsf = (iduMemTlsfLocal*)iduMemMgr::mallocRaw(sizeof(iduMemTlsfLocal));
            IDE_TEST(sTlsf == NULL);
            sTlsf = new(sTlsf) iduMemTlsfLocal;
        }

        IDE_TEST(sTlsf->initialize(sName, mAutoShrink,
                                   mTLSFLocalChunkSize)
                 != IDE_SUCCESS);

        mTLSFLocal[i] = sTlsf;
    }

    /*
     * Get thread allocator for main thread after memory manager
     * is initialized
     */
    IDE_ASSERT(iduMemMgr::getSmallAlloc(&(idtContainer::mMainThread.mSmallAlloc))
               == IDE_SUCCESS);
    IDE_ASSERT(iduMemMgr::getTlsfAlloc(&(idtContainer::mMainThread.mTlsfAlloc))
               == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_destroyStatic(void)
{
    UInt i;

    for(i = 0; i < mNoAllocators; i++)
    {
        IDE_TEST(mTLSFLocal[i]->destroy() != IDE_SUCCESS);
        iduMemMgr::freeRaw(mTLSFLocal[i]);
    }

    if(mTLSFGlobalChunkSize != 0)
    {
        mTLSFGlobal->destroy();
        iduMemMgr::freeRaw(mTLSFGlobal);
    }
    else
    {
        /* Do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_malloc(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              void                 **aMemPtr)
{
    IDE_DASSERT(aMemPtr != NULL);

    if((mUsePrivateAlloc != 0) && (aSize <= IDU_SMALL_MAXSIZE))
    {
        idtContainer*   sContainer;
        sContainer = idtContainer::getThreadContainer();
        IDE_TEST(
            sContainer->mSmallAlloc->malloc(aIndex, aSize, aMemPtr)
            != IDE_SUCCESS);
    }
    else
    {
# if(__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 9)
        IDE_TEST(
            mTLSFLocal[getAllocIndex()]->malloc(aIndex, aSize, aMemPtr)
            != IDE_SUCCESS);
# else
        idtContainer*   sContainer;
        sContainer = idtContainer::getThreadContainer();
        IDE_TEST(
            sContainer->mTlsfAlloc->malloc(aIndex, aSize, aMemPtr)
            != IDE_SUCCESS);
# endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_malign(iduMemoryClientIndex   aIndex,
                              ULong                  aSize,
                              ULong                  aAlign,
                              void                 **aMemPtr)
{
    idtContainer*   sContainer;
    IDE_DASSERT(aMemPtr != NULL);

    sContainer = idtContainer::getThreadContainer();
    IDE_TEST(
        sContainer->mTlsfAlloc->malign(aIndex, aSize, aAlign, aMemPtr)
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_calloc(iduMemoryClientIndex   aIndex,
                              vSLong                 aCount,
                              ULong                  aSize,
                              void                 **aMemPtr)
{
    IDE_TEST(tlsf_malloc(aIndex, aCount * aSize, aMemPtr) != IDE_SUCCESS);
    idlOS::memset(*aMemPtr, 0, aCount * aSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_realloc(iduMemoryClientIndex  aIndex,
                               ULong                 aSize,
                               void                **aMemPtr)
{
    const ULong sF0  = ID_ULONG(0xFFFFFFF0);
    const ULong sFF0 = ID_ULONG(0xFFFFFFFFFFFFFFF0);

    void*   sOldPtr;
    void*   sNewPtr;
    ULong*  sSizePtr;
    ULong   sOldSize;

    sOldPtr     = *aMemPtr;
    sSizePtr    = (ULong*)(*aMemPtr);
    sSizePtr--;

    sOldSize = *sSizePtr;
    if((sOldSize & IDU_TLSF_SMALL) != 0)
    {
        sOldSize &= sF0;
    }
    else
    {
        sOldSize &= sFF0;
    }

    if(sOldSize < aSize)
    {
        IDE_TEST(tlsf_malloc(aIndex, aSize, &sNewPtr) != IDE_SUCCESS);
        idlOS::memcpy(sNewPtr, sOldPtr, sOldSize);
        IDE_TEST_RAISE(tlsf_free(*aMemPtr) != IDE_SUCCESS, EFREE);
        *aMemPtr = sNewPtr;
    }
    else
    {
        /* return as is */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(EFREE)
    {
        IDE_ASSERT(tlsf_free(sNewPtr) == IDE_SUCCESS);
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_free(void                *aMemPtr)
{
    ULong*          sAllocSize;

    sAllocSize = (ULong*)aMemPtr;
    sAllocSize--;

    if((*sAllocSize & IDU_TLSF_SMALL) != 0)
    {
        IDE_TEST(iduMemSmall::freeStatic(aMemPtr) != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST(iduMemTlsf::freeStatic(aMemPtr) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::tlsf_free4malign(void                *aMemPtr,
                                   iduMemoryClientIndex aIndex,
                                   ULong                aSize)
{
    PDL_UNUSED_ARG(aIndex);
    PDL_UNUSED_ARG(aSize);

    return tlsf_free(aMemPtr);
}

IDE_RC iduMemMgr::tlsf_shrink(void)
{
    return IDE_SUCCESS;
}

