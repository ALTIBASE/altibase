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
 * $Id: iduMemListOld.h 40953 2010-08-09 02:09:41Z orc $
 **********************************************************************/

#ifndef _O_IDU_MEMLISTOld_H_
#define _O_IDU_MEMLISTOld_H_ 1

#include <idl.h>
#include <iduMutex.h>
#include <iduMemMgr.h>

class iduMemListOld;

struct iduMemSlotQP
{
    iduMemSlotQP*    mNext;
};

struct iduMemChunkQP
{
    iduMemListOld *mParent;
    iduMemChunkQP *mNext;
    iduMemChunkQP *mPrev;
    iduMemSlotQP  *mTop;
    UInt         mMaxSlotCnt;
    UInt         mFreeSlotCnt;
};


/* ------------------------------------------------
 *  Runtime시의 속도를 위해 alloc & free를 각각
 *  2벌씩 준비함. ( 1 alloc & bulk alloc)
 * ----------------------------------------------*/
class iduMemListOld
{
    friend class iduMemPool;
    friend void iduCheckMemConsistency(iduMemListOld *aMemList);
public:
    iduMemListOld();
    ~iduMemListOld();

    IDE_RC          initialize(iduMemoryClientIndex aIndex,
                               UInt   aSeqNumber,
                               SChar *aStrName,
                               vULong aElemSize,
                               vULong aElemCnt,
                               vULong aAutofreeChunkLimit,
                               idBool aUseMutex  = ID_TRUE,
                               UInt   aAlignByte = 8);
    
    IDE_RC          destroy(idBool aCheck = ID_TRUE);

    /* ------------------------------------------------
     *  Special case for only 1 allocation & free
     * ----------------------------------------------*/
    IDE_RC          cralloc(void **);
    IDE_RC          alloc(void **aMem);
    IDE_RC          memfree(void *aMem);
    UInt            getUsedMemory();
    void            status();

    /* 
     *  PROJ-2065 한계상황 테스트
     */
    IDE_RC          shrink(UInt * aSize);
    void            fillMemPoolInfo( struct iduMemPoolInfo * aInfo );

private:
    
    IDE_RC          grow(void);

    inline void     unlink(iduMemChunkQP *aChunk);
    inline void     link(iduMemChunkQP *aBefore, iduMemChunkQP *aChunk);

    iduMutex             mMutex;

    iduMemChunkQP          mFreeChunk;
    iduMemChunkQP          mPartialChunk;
    iduMemChunkQP          mFullChunk;
                        
    vULong               mFreeChunkCnt;
    vULong               mPartialChunkCnt;
    vULong               mFullChunkCnt;
                        
    vULong               mAutoFreeChunkLimit;
                        
    vULong               mChunkSize;
    vULong               mElemCnt;
    vULong               mElemSize;
    vULong               mSlotSize;
    iduMemoryClientIndex mIndex;
    idBool               mUseMutex;
    UInt                 mAlignByte;

};

#ifdef DEBUG
#define MEMORY_ASSERT
#endif

#endif  // _O_MEMLISTOld_H_

