/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemList.h 56007 2012-10-19 08:04:09Z kclee $
 **********************************************************************/

#ifndef _O_IDU_MEMLIST_H_
#define _O_IDU_MEMLIST_H_ 1

#include <idl.h>
#include <iduMutex.h>
#include <iduMemMgr.h>

class iduMemList;

typedef struct    iduMemSlot 
{
    struct    iduMemSlot *mNext; 
}   iduMemSlot ;

struct iduMemChunk
{
    iduMemList  *mParent;
    iduMemChunk *mNext;
    iduMemChunk *mPrev;
    iduMemSlot  *mTop;
    UInt         mMaxSlotCnt;
    UInt         mFreeSlotCnt;
};


/* ------------------------------------------------
 *  Runtime시의 속도를 위해 alloc & free를 각각
 *  2벌씩 준비함. ( 1 alloc & bulk alloc)
 * ----------------------------------------------*/
class iduMemList
{
    friend class iduMemPool;
    friend void iduCheckMemConsistency(iduMemList *aMemList);
public:
    iduMemList();
    ~iduMemList();

    IDE_RC          initialize(UInt        aSeqNumber,
                               iduMemPool *parent);
    
    IDE_RC          destroy(idBool aCheck = ID_TRUE);

    IDE_RC          alloc(void **aMem);
    IDE_RC          memfree(void *aMem);
    UInt            getUsedMemory();
    void            status();

    IDE_RC          cralloc(void **aMem);

    /* 
     *  PROJ-2065 한계상황 테스트
     */
    IDE_RC          shrink(UInt * aSize);
    void            fillMemPoolInfo( struct iduMemPoolInfo * aInfo );

private:
    
    IDE_RC          grow(void);

    inline void     unlink(iduMemChunk *aChunk);
    inline void     link(iduMemChunk *aBefore, iduMemChunk *aChunk);


    iduMemPool      * mP;


    iduMutex         mMutex;
    iduMemChunk      mFreeChunk;
    iduMemChunk      mPartialChunk;
    iduMemChunk      mFullChunk;
                        
    UInt             mFreeChunkCnt;
    UInt             mPartialChunkCnt;
    UInt             mFullChunkCnt;
                        

};

#ifdef DEBUG
#define MEMORY_ASSERT
#endif

#endif  // _O_MEMLIST_H_

