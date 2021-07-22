/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemPool.h 83870 2018-09-03 04:32:39Z kclee $
 **********************************************************************/

#ifndef _O_IDU_MEM_POOL_H_
#define _O_IDU_MEM_POOL_H_ 1

#include <idl.h>
#include <iduMemMgr.h>

#include <iduMemList.h>

/* BUG-46165 */
typedef enum iduMemPoolType{
    IDU_MEMPOOL_TYPE_LEGACY,
    IDU_MEMPOOL_TYPE_TIGHT 
}iduMemPoolType;

#define IDU_AUTOFREE_CHUNK_LIMIT         5

#define IDU_MEM_POOL_DEFAULT_ALIGN_SIZE  8

#define IDU_MEM_POOL_MUTEX_POSTFIX       "_MUTEX_"

//10: UINT32_FMT의 10진수표기최대자리수(iduMemList.cpp에서 사용함.)
//1 : 문자열끝 null
//ex)64 - 7 -10 - 1
#define IDU_MEM_POOL_NAME_LEN  ( IDU_MUTEX_NAME_LEN - \
                                 ID_SIZEOF(IDU_MEM_POOL_MUTEX_POSTFIX)-10 -1)


//PROJ-2182
#define IDU_MEMPOOL_USE_MUTEX          (0x00000001U)  
#define IDU_MEMPOOL_GARBAGE_COLLECT    (0x00000002U)  
#define IDU_MEMPOOL_USE_POOLING        (0x00000004U)  
#define IDU_MEMPOOL_CACHE_LINE_IDU_ALIGN   (0x00000008U)  


/*
// use mutex, garbage collect, H/W cache line
#define IDU_MEMPOOL_DEFAULT_FLAG     (IDU_MEMPOOL_USE_MUTEX       |  \
                                      IDU_MEMPOOL_GARBAGE_COLLECT |  \
                                      IDU_MEMPOOL_CACHE_LINE_IDU_ALIGN) 
*/

#define IDU_ALIGN(x)          idlOS::align(x,IDL_CACHE_LINE)
#define IDU_ALIGN8(x)         idlOS::align(x,8)
#define CHUNK_HDR_SIZE    IDU_ALIGN( ID_SIZEOF(iduMemChunk) )

class iduMemList;
class iduMemPool
{
public:
  /*************************************************************
   * BUG-46165 
   *   [MUST]
   *   when IDU_MEMPOOL_TYPE_TIGHT,the following specifications "MUST" be met.
   *   otherwise, you will encounter assert error.
   *    -  aElemSize must be 2^n, 
   *    -  aElemCount must be 2^n,
   *    -  aElemSize == aAlignByte
   *    -  aElemSize >=8
   *   
   *   [INFO] 
   *   (following things are just for your information.)
   *   In IDU_MEMPOOL_TYPE_TIGHT,
   *   - aHWCacheLine is ignored  
   *   - aElemCount is set to 2 if it is 1.
   *   - __MEMPOOL_MINIMUM_SLOT_COUNT is ignored.
   *************************************************************/
    IDE_RC  initialize( iduMemoryClientIndex aIndex,
                        SChar*         aStrName,
                        UInt           aListCount,
                        vULong         aObjSize,
                        vULong         aElemCount,
                        vULong         aChunkLimit,        /* IDU_AUTOFREE_CHUNK_LIMIT,*/
                        idBool         aUseMutex,          /* ID_TRUE, */
                        UInt           aAlignByte,         /* IDU_MEM_POOL_DEFAULT_ALIGN_SIZE, */
                        idBool         aForcePooling,      /* ID_FALSE, */
                        idBool         aGarbageCollection, /* ID_TRUE, */
                        idBool         aHWCacheLine,       /* ID_TRUE  */
                        iduMemPoolType aType );            /*IDU_MEMPOOL_TYPE_LEGACY*/
    IDE_RC  destroy(idBool aCheck = ID_TRUE);

    IDE_RC  cralloc(void **);
    IDE_RC  alloc(void **aMem);
    IDE_RC  memfree(void *aMem);
    IDE_RC  shrink(UInt * aSize);

    void    dumpState(SChar *aBuffer, UInt aBufferSize);
    void    status();

    /* added by mskim for check memory statement */
    ULong   getMemorySize();

    //check if the size is 2^n.
    idBool  checkSize4Tight(UInt  aSize);


public: /* POD class type should make non-static data members as public */
    UInt                 mFlag;  //PROJ-2182
    ULong                mChunkSize;
    ULong                mElemCnt;
    ULong                mElemSize;
    ULong                mSlotSize;
    iduMemoryClientIndex mIndex;
    UInt                 mAlignByte;
    UInt                 mAutoFreeChunkLimit;
    UInt                 mListCount;
    iduMemPool         * mPrev;
    iduMemPool         * mNext;
    iduMemList        ** mMemList;
    SChar                mName[IDU_MEM_POOL_NAME_LEN];
    iduMemPoolType       mType;
};

#endif  // _O_MEM_POOL_H_

