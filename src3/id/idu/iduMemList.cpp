/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemList.cpp 84564 2018-12-10 05:40:03Z kclee $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <iduRunTimeInfo.h>
#include <iduMemList.h>
#include <iduMemPoolMgr.h>

#ifndef X86_64_DARWIN
#include <malloc.h>
#endif

extern const vULong gFence =
#ifdef COMPILE_64BIT
ID_ULONG(0xDEADBEEFDEADBEEF);
#else
0xDEADBEEF;
#endif


#undef IDU_ALIGN
#define IDU_ALIGN(x)          idlOS::align(x,mP->mAlignByte)

//get chunk address in the type TIGHT.
#define IDU_GET_CHUNK_FOR_TIGHT(addr) (iduMemChunk *)( (ULong)addr - mP->mChunkSize )

void iduCheckMemConsistency(iduMemList * aMemList)
{
    return;

    vULong sCnt = 0;
    iduMemSlot *sSlot;

    //check free chunks
    iduMemChunk *sCur = aMemList->mFreeChunk.mNext;
    while(sCur != NULL)
    {
        sSlot = (iduMemSlot *)sCur->mTop;
        while(sSlot != NULL)
        {
            sSlot = sSlot->mNext;
        }

        IDE_ASSERT( sCur->mMaxSlotCnt  == sCur->mFreeSlotCnt );

        sCur = sCur->mNext;
        sCnt++;
        IDE_ASSERT(sCnt <= aMemList->mFreeChunkCnt);
    }

    IDE_ASSERT(sCnt == aMemList->mFreeChunkCnt);

    //check full chunks
    sCnt = 0;
    sCur = aMemList->mFullChunk.mNext;
    while(sCur != NULL)
    {
        IDE_ASSERT( sCur->mFreeSlotCnt == 0 );
        sCur = sCur->mNext;
        sCnt++;
    }

    IDE_ASSERT(sCnt == aMemList->mFullChunkCnt);

    //check partial chunks
    sCnt = 0;
    sCur = aMemList->mPartialChunk.mNext;
    while(sCur != NULL)
    {
        /*
         *현재 알고리즘에서 slot이 한개인 chunk는 partial chunk list
         *에 존재할수가 없음. free와 full chunk list에만 존재해야함.
         */
        IDE_ASSERT( sCur->mMaxSlotCnt > 1 );

        //sCur->mMaxSlotCnt > 1
        IDE_ASSERT( (0  < sCur->mFreeSlotCnt) &&  
                    (sCur->mFreeSlotCnt < sCur->mMaxSlotCnt) );

        sCur = sCur->mNext;
        sCnt++;
    }

    IDE_ASSERT(sCnt == aMemList->mPartialChunkCnt);
}


#ifndef MEMORY_ASSERT
#define iduCheckMemConsistency( xx )
#endif

iduMemList::iduMemList()
{
}

iduMemList::~iduMemList(void)
{
}

/*---------------------------------------------------------
  iduMemList

   ___________ mFullChunk
  |iduMemList |    _____      _____
  |___________|-> |chunk| -> |chunk| ->NULL
        |  NULL<- |_____| <- |_____|
        |
        |
        |     mPartialChunk
        |          _____      _____
        |`------->|chunk| -> |chunk| ->NULL
        |   NULL<- |_____| <- |_____|
        |
        | 
        |     mFreeChunk
        |          _____      _____
         `------->|chunk| -> |chunk| ->NULL
           NULL<- |_____| <- |_____|


  mFreeChunk    : 모든 slot이 free인 chunk
  mPartialChunk : 일부의 slot만 사용중인 chunk
  mFullChunk    : 모든 slot이 사용중인 chunk

  *처음 chunk를 할당받으면 mFreeChunk에 매달리게되고
   slot할당이 이뤄지면 mPartialChunk를 거쳐서 mFullchunk로 이동함.
  *mFullChunk에 있는 chunk는 slot반납시 mPartialchunk를 거쳐서 mFreeChunk로 이동.
  *mFreeChunk에 있는 chunk들은 메모리가 부족한 한계상황에서는 OS에 반납되어질수 있음.
  
  chunk:
   _________________________________
  |iduMemChunk |Slot|Slot|... | Slot|
  |____________|____|____|____|_____|

  iduMemChunk        : chunk 헤더(현재 chunk에 대한 내용을 담고 있다.)
  
  
  
  Slot:
   ___________________________________
  |momory element |iduMemChunk pointer|
  |_______________|___________________|

  memory element      : 사용자가 실제 메모리를 사용하는 영역
             이 영역은 사용자에게 할당되어 있지 않을때는, 다음
             (free)slot영역을 가리키는 포인터 역활을 한다.
  iduMemChunk pointer : slot이 속한 chunk의 헤더(chunk의 iduMemChunk영역)을
                        포인팅 하고 있다.
  -----------------------------------------------------------*/
/*-----------------------------------------------------------
 * task-2440 iduMemPool에서 메모리를 얻어올때 align된 메모리 주소를 얻어
 *          올 수 있게 하자.
 *          
 * chunk내에서 align이 일어나는 곳을 설명하겠다.
 * ___________________________________________
  |iduMemChunk |   |  Slot|  Slot|... |   Slot|
  |____________|___|______|______|____|_______|
  ^            ^   ^      ^
  1            2   3      4
  
  일단 chunk를 할당하면 1번은 임의의 주소값으로 설정된다.  이 주소값은
  사용자가 align되기를 원하는 주소가 아니기 때문에 그냥 놔둔다.
  2번주소 = 1번 주소 + SIZEOF(iduMemChunk)
  2번 주소는 위와 같이 구해지고, 2번주소를 align up시키면 3번처럼 뒷 공간을
  가리키게 된다. 그러면 3번은 align이 되어있게 된다.
  실제로 사용자에게 할당 되는 공간은 3번 공간 부터이다.
  4번이 align되어 있기 위해서는 slotSize가 align 되어 있어야 한다.
  그렇기 때문에 SlotSize도 실제 size보다 align up된 값을 사용한다.
  그러면 모든 slot의 주소는 align되어있음을 알 수있다.

  위의 2번과 3번사이 공간의 최대 크기는 alignByte이다.
  그렇기 때문에
  chunkSize = SIZEOF(iduMemChunk)+ alignByte +
              ('alignByte로 align up된 SIZEOF(slot)' * 'slot의 개수')
  이다.
 * ---------------------------------------------------------*/

/****************************************************** 
 BUG-46165 

 S:Slot  
 H:chunk Header    
 A:chunk header Address:8byte , the rest: waste 

 When slot size is 8k ,alignment size is 8k,and count is 8,

 in IDU_MEMPOOL_TYPE_TIGHT, 

  8k 8k 8k 8k 8k 8k 8k 8k
 +--+--+--+--+--+--+--+--+-+
 |S |S |S |S |S |S |S |S |H|
 +--+--+--+--+--+--+--+--+-+

 in IDU_MEMPOOL_TYPE_LEGACY,

    8k 8k 8k 8k 8k 8k 8k 8k 8k 8k 8k 8k 8k 8k 8k 8k     
 +-+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |H|S |A |S |A |S |A |S |A |S |A |S |A |S |A |S |A |
 +-+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

******************************************************/
/*-----------------------------------------------------------
 * Description: iduMemList를 초기화 한다.
 *
 * aSeqNumber        - [IN] iduMemList 식별 번호
 * aParent           - [IN] 자신이 속한 iduMemPool
 * ---------------------------------------------------------*/
/*-----------------------------------------------------------
 * task-2440 iduMemPool에서 메모리를 얻어올때 align된 메모리 주소를 얻어
 *          올 수 있게 하자.
 *          
 * chunkSize = SIZEOF(iduMemChunk)+ alignByte +
 *             ('alignByte로 align up된 SIZEOF(slot)' * 'slot의 개수')
 */

IDE_RC iduMemList::initialize( UInt         aSeqNumber,
                               iduMemPool * aParent)
{

    SChar sBuffer[IDU_MUTEX_NAME_LEN*2];//128 bytes

    mP = aParent;

    mFreeChunk.mParent         = this;
    mFreeChunk.mNext           = NULL;
    mFreeChunk.mPrev           = NULL;
    mFreeChunk.mTop            = NULL;
    mFreeChunk.mMaxSlotCnt     = 0;
    mFreeChunk.mFreeSlotCnt    = 0;
                               
    mPartialChunk.mParent      = this;
    mPartialChunk.mNext        = NULL;
    mPartialChunk.mPrev        = NULL;
    mPartialChunk.mTop         = NULL;
    mPartialChunk.mMaxSlotCnt  = 0;
    mPartialChunk.mFreeSlotCnt = 0;

    mFullChunk.mParent         = this;
    mFullChunk.mNext           = NULL;
    mFullChunk.mPrev           = NULL;
    mFullChunk.mTop            = NULL;
    mFullChunk.mMaxSlotCnt     = 0;
    mFullChunk.mFreeSlotCnt    = 0;

    mFreeChunkCnt              = 0;
    mPartialChunkCnt           = 0;
    mFullChunkCnt              = 0;

    IDE_TEST(grow() != IDE_SUCCESS);

    if (mP->mFlag & IDU_MEMPOOL_USE_MUTEX)
    {

        idlOS::memset(sBuffer, 0, ID_SIZEOF(sBuffer));
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "%s"IDU_MEM_POOL_MUTEX_POSTFIX"%"ID_UINT32_FMT, mP->mName, aSeqNumber);


        IDE_TEST(mMutex.initialize(sBuffer,
                                   IDU_MUTEX_KIND_NATIVE,
                                   IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* ---------------------------------------------------------------------------
 * BUG-19253
 * iduMemList는 mFreeChunk와 mFullChunk의 두개의 chunk list를 유지한다.
 * iduMemPool을 사용하는 모듈에서 iduMemPool을 destroy하기 전에
 * 모든 iduMemPool의 slot을 memfree했을 때만 정상적인 destroy가 수행된다.
 * 만약, 하나의 slot이라도 free되지 않은 채 destroy가 요청되면 debug mode에서는
 * DASSERT()로 죽고, release mode에서는 mFullChunk에 연결된 chunk만큼의
 * memory leak이 발생하게 된다.
 * debug mode에서는 memory leak을 감지하기 위해 DASSERT문을 유지하고,
 * release mode에서는 mFullChunk에 연결된 chunk들도 free 시키도록 한다.
 *
 * index build와 같이 사용한 iduMemPool을 완료 후 한꺼번에 free 시키는 경우
 * mFullChunk를 추가적으로 free 시키도록 한다. 한꺼번에 free 시키는 경우는
 * aBCheck가 ID_FALSE로 설정되어야 한다.
 * ---------------------------------------------------------------------------*/
IDE_RC iduMemList::destroy(idBool aBCheck)
{
    vULong i;
    iduMemChunk *sCur = NULL;

    for (i = 0; i < mFreeChunkCnt; i++)
    {
        sCur = mFreeChunk.mNext;
        IDE_ASSERT(sCur != NULL);

        if(aBCheck == ID_TRUE)
        {
            if (sCur->mFreeSlotCnt != sCur->mMaxSlotCnt )
            {
                ideLog::log(IDE_SERVER_0,ID_TRC_MEMLIST_MISMATCH_FREE_SLOT_COUNT,
                            (UInt)sCur->mFreeSlotCnt,
                            (UInt)sCur->mMaxSlotCnt);

                IDE_DASSERT(0);
            }
        }

        unlink(sCur);

        if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
        {
            sCur  = IDU_GET_CHUNK_FOR_TIGHT(sCur);
            IDE_TEST( iduMemMgr::free4malign(sCur, 
                                             mP->mIndex, 
                                             mP->mChunkSize + ID_SIZEOF(iduMemChunk))
                                     != IDE_SUCCESS);
        }
        else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
        {
            IDE_TEST( iduMemMgr::free(sCur) != IDE_SUCCESS);
        }
    }

/* BUG-45749 */
#ifdef DEBUG
    if(aBCheck == ID_TRUE)
    {
        if( mFullChunkCnt != 0 )
        {
            ideLog::log(IDE_SERVER_0,ID_TRC_MEMLIST_WRONG_FULL_CHUNK_COUNT,
                        (UInt)mFullChunkCnt);
        }
        else
        {
            /* do nothing */
        }

        if( mPartialChunkCnt != 0 )
        {
            ideLog::log(IDE_SERVER_0,ID_TRC_MEMLIST_WRONG_PARTIAL_CHUNK_COUNT,
                        (UInt)mPartialChunkCnt);
        }
        else
        {
            /* do nothing */
        }
    }
#endif

    for (i = 0; i < mPartialChunkCnt; i++)
    {
        sCur = mPartialChunk.mNext;
        IDE_ASSERT(sCur != NULL);

        if(aBCheck == ID_TRUE)
        {
            if ( (sCur->mFreeSlotCnt == 0) || 
                 (sCur->mFreeSlotCnt == sCur->mMaxSlotCnt) )
            {
                ideLog::log(IDE_SERVER_0, ID_TRC_MEMLIST_WRONG_FREE_SLOT_COUNT,
                            (UInt)sCur->mFreeSlotCnt,
                            (UInt)sCur->mMaxSlotCnt);
                IDE_DASSERT(0);
            }
        }

        unlink(sCur);

        if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
        {
            sCur  = IDU_GET_CHUNK_FOR_TIGHT(sCur);
            IDE_TEST( iduMemMgr::free4malign(sCur, 
                                             mP->mIndex, 
                                             mP->mChunkSize + ID_SIZEOF(iduMemChunk) )
                                     != IDE_SUCCESS);
        }
        else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
        {
            IDE_TEST( iduMemMgr::free(sCur) != IDE_SUCCESS);
        }
    }

    // BUG-19253
    // mFullChunk list에 연결된 chunk가 있으면 free시킨다.
    for (i = 0; i < mFullChunkCnt; i++)
    {
        sCur = mFullChunk.mNext;
        IDE_ASSERT(sCur != NULL);

        unlink(sCur);

        if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
        {
            sCur  = IDU_GET_CHUNK_FOR_TIGHT(sCur);
            IDE_TEST( iduMemMgr::free4malign(sCur, 
                                             mP->mIndex, 
                                             mP->mChunkSize + ID_SIZEOF(iduMemChunk))
                                     != IDE_SUCCESS);
        }
        else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
        {
            IDE_TEST( iduMemMgr::free(sCur) != IDE_SUCCESS);
        }
    }

    if (mP->mFlag & IDU_MEMPOOL_USE_MUTEX)
    {
        IDE_TEST(mMutex.destroy() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

inline void iduMemList::unlink(iduMemChunk *aChunk)
{
    iduMemChunk *sBefore = aChunk->mPrev;
    iduMemChunk *sAfter  = aChunk->mNext;

    sBefore->mNext = sAfter;
    if (sAfter != NULL)
    {
        sAfter->mPrev = sBefore;
    }
}

inline void iduMemList::link(iduMemChunk *aBefore, iduMemChunk *aChunk)
{
    iduMemChunk *sAfter   = aBefore->mNext;

    aBefore->mNext        = aChunk;
    aChunk->mPrev         = aBefore;
    aChunk->mNext         = sAfter;

    if (sAfter != NULL)
    {
        sAfter->mPrev = aChunk;
    }
}

IDE_RC iduMemList::grow(void)
{

    vULong       i;
    iduMemChunk *sChunk=NULL;
    iduMemSlot  *sSlot;
    iduMemSlot  *sFirstSlot;



#ifdef MEMORY_ASSERT
    IDE_TEST(iduMemMgr::malloc(mP->mIndex,
                               mP->mChunkSize,
                               (void**)&sChunk,
                               IDU_MEM_FORCE)
             != IDE_SUCCESS);

#else
    if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
    {
        IDE_TEST(iduMemMgr::malign(mP->mIndex,
                                   mP->mChunkSize + ID_SIZEOF(iduMemChunk),
                                   mP->mChunkSize,
                                   (void**)&sChunk,
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);

        IDE_DASSERT( ((ULong)sChunk & (ULong)(mP->mChunkSize - 1) ) == 0 );

        sChunk         = (iduMemChunk *)( (ULong)sChunk + mP->mChunkSize );
    }
    else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
    {
        IDE_TEST(iduMemMgr::malloc(mP->mIndex,
                                   mP->mChunkSize,
                                   (void**)&sChunk,
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);
    }
#endif


    IDE_ASSERT( sChunk != NULL ); 


    *(iduMemList**)&sChunk->mParent         = this;
    sChunk->mMaxSlotCnt  = mP->mElemCnt;
    sChunk->mFreeSlotCnt = mP->mElemCnt;
    sChunk->mTop       = NULL;

    /*chunk내의 각 slot을 mNext로 연결한다. 가장 마지막에 있는 slot이 sChunk->mTop에 연결된다.
     * mNext는 사용자에게 할당될때는 실제 사용하는 영역으로 쓰인다.*/
    if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
    {
        sFirstSlot = (iduMemSlot*)( (ULong)sChunk - mP->mChunkSize );
    }
    else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
    {
        sFirstSlot = (iduMemSlot*)( (UChar*)sChunk + CHUNK_HDR_SIZE );

    }

    sFirstSlot = (iduMemSlot*) IDU_ALIGN(sFirstSlot);
    for (i = 0; i < mP->mElemCnt; i++)
    {
        sSlot        = (iduMemSlot *)((UChar *)sFirstSlot + (i * mP->mSlotSize));
        sSlot->mNext = sChunk->mTop;
        sChunk->mTop = sSlot;
#ifdef MEMORY_ASSERT
        *((vULong *)((UChar *)sSlot + mP->mElemSize)) = gFence;
        *((iduMemChunk **)((UChar *)sSlot +
                           mP->mElemSize +
                           ID_SIZEOF(gFence))) = sChunk;
        *((vULong *)((UChar *)sSlot +
                     mP->mElemSize +
                     ID_SIZEOF(gFence)  +
                     ID_SIZEOF(iduMemChunk *))) = gFence;
#else
        if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
        {
                *((iduMemChunk **)((UChar *)sSlot + mP->mElemSize)) = sChunk;
        }
        else if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
        {
            /*we don't need to do anything here */
        }
#endif
    }

    iduCheckMemConsistency(this);

    /* ------------------------------------------------
     *  mFreeChunk에 연결
     * ----------------------------------------------*/
    link(&mFreeChunk, sChunk);
    mFreeChunkCnt++;

    iduCheckMemConsistency(this);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC iduMemList::alloc(void **aMem)
{

    iduMemChunk *sMyChunk=NULL;



#if defined(ALTIBASE_MEMORY_CHECK)
    IDE_TEST( iduMemMgr::malign(mP->mIndex,mP->mElemSize,mP->mAlignByte, aMem)
              != IDE_SUCCESS);

    IDE_ASSERT(*aMem != NULL);
#else /* normal case : not memory check */
    
    iduCheckMemConsistency(this);

    //victim chunk를 선택.
    if (mPartialChunk.mNext != NULL) 
    {
        sMyChunk = mPartialChunk.mNext;
    }
    else
    {
        if (mFreeChunk.mNext == NULL) 
        {
            IDE_TEST(grow() != IDE_SUCCESS);
        }
        sMyChunk = mFreeChunk.mNext;

        IDE_ASSERT( sMyChunk != NULL );
    }

    iduCheckMemConsistency(this);

    IDE_ASSERT( sMyChunk->mTop != NULL );

    *aMem           = sMyChunk->mTop;

    sMyChunk->mTop = ((iduMemSlot *)(*aMem))->mNext;

    if ((--sMyChunk->mFreeSlotCnt) == 0) // * Partial list ->Full list
    {
        unlink(sMyChunk);
        link(&mFullChunk, sMyChunk);
        
        IDE_ASSERT( mPartialChunkCnt > 0 );
        mPartialChunkCnt--;
        mFullChunkCnt++;
    }
    else if ( (sMyChunk->mMaxSlotCnt - sMyChunk->mFreeSlotCnt) == 1)//Free list ->Partial list
    {
            unlink(sMyChunk);
            link(&mPartialChunk, sMyChunk);

            IDE_ASSERT( mFreeChunkCnt > 0 );
            mFreeChunkCnt--;
            mPartialChunkCnt++;
    }

    iduCheckMemConsistency(this);

#endif

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC iduMemList::memfree(void *aFreeElem)
{
    iduMemSlot*  sFreeElem;
    iduMemChunk *sCur;

    IDE_ASSERT(aFreeElem != NULL);

#if defined(ALTIBASE_MEMORY_CHECK)
    IDE_TEST( iduMemMgr::free4malign( aFreeElem, mP->mIndex,mP->mElemSize ) 
              != IDE_SUCCESS);
#else /* normal case : not memory check */


#  ifdef MEMORY_ASSERT

    IDE_ASSERT(*((vULong *)((UChar *)aFreeElem + mP->mElemSize)) == gFence);
    IDE_ASSERT(*((vULong *)((UChar *)aFreeElem +
                        mP->mElemSize +
                        ID_SIZEOF(gFence)  +
                        ID_SIZEOF(iduMemChunk *))) == gFence);
    sCur         = *((iduMemChunk **)((UChar *)aFreeElem + mP->mElemSize +
                                           ID_SIZEOF(gFence)));
#  else
    if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
    {
        sCur   = (iduMemChunk *)( ( (ULong)aFreeElem & (ULong)~(mP->mChunkSize - 1 )) 
                                    + mP->mChunkSize ); 
    }
    else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
    {
        sCur   = *((iduMemChunk **)((UChar *)aFreeElem + mP->mElemSize));
    }
#endif

    sFreeElem        = (iduMemSlot*)aFreeElem;
    sFreeElem->mNext = sCur->mTop;
    sCur->mTop  = sFreeElem;

    sCur->mFreeSlotCnt++;
    IDE_ASSERT(sCur->mFreeSlotCnt <= sCur->mMaxSlotCnt);


    if (sCur->mFreeSlotCnt == sCur->mMaxSlotCnt )  //Partial List -> Free List
    {
        unlink(sCur);
        
        mPartialChunkCnt--;

        //Limit을 넘어섰을 경우 autofree
        if (mFreeChunkCnt <= mP->mAutoFreeChunkLimit)
        {
            link(&mFreeChunk, sCur);
            mFreeChunkCnt++;
        }
        else
        {
            if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
            {
                sCur  = IDU_GET_CHUNK_FOR_TIGHT(sCur);
                IDE_TEST( iduMemMgr::free4malign(sCur, 
                                                 mP->mIndex, 
                                                 mP->mChunkSize + ID_SIZEOF(iduMemChunk))
                                         != IDE_SUCCESS);
            }
            else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
            {
                IDE_TEST( iduMemMgr::free(sCur) != IDE_SUCCESS);
            }
        }
    }
    else // Full list -> Partial list
    {
        if (sCur->mFreeSlotCnt == 1)
        {
            unlink(sCur);
            link(&mPartialChunk, sCur);

            IDE_ASSERT( mFullChunkCnt > 0 );
            mFullChunkCnt--;
            mPartialChunkCnt++;
        }
    }

    iduCheckMemConsistency(this);

#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC iduMemList::cralloc(void **aMem)
{
    IDE_TEST(alloc(aMem) != IDE_SUCCESS);
    idlOS::memset(*aMem, 0, mP->mElemSize);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC iduMemList::shrink( UInt *aSize)
{
    iduMemChunk *sCur = NULL;
    iduMemChunk *sNxt = NULL;
    UInt         sFreeSizeDone = mFreeChunkCnt*mP->mChunkSize;

    IDE_ASSERT( aSize != NULL );

    *aSize = 0;

    sCur = mFreeChunk.mNext;
    while( sCur != NULL )
    {
        sNxt = sCur->mNext;
        unlink(sCur);
        mFreeChunkCnt--;
        if( mP->mType == IDU_MEMPOOL_TYPE_TIGHT )
        {
            sCur  = IDU_GET_CHUNK_FOR_TIGHT(sCur);
            IDE_TEST( iduMemMgr::free4malign(sCur, 
                                             mP->mIndex, 
                                             mP->mChunkSize + ID_SIZEOF(iduMemChunk))
                                     != IDE_SUCCESS);
        }
        else if( mP->mType == IDU_MEMPOOL_TYPE_LEGACY )
        {
            IDE_TEST( iduMemMgr::free(sCur) != IDE_SUCCESS);
        }

        sCur = sNxt;
    }

    IDE_ASSERT( mFreeChunkCnt == 0 ); //당연.

    *aSize = sFreeSizeDone;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


UInt iduMemList::getUsedMemory()
{
    UInt sSize;

    sSize = mP->mElemSize * mFreeChunkCnt * mP->mElemCnt;
    sSize += (mP->mElemSize * mFullChunkCnt * mP->mElemCnt);

    return sSize;
}

void iduMemList::status()
{
    SChar sBuffer[IDE_BUFFER_SIZE];

    IDE_CALLBACK_SEND_SYM_NOLOG("    Mutex Internal State\n");
    mMutex.status();

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "    Memory Usage: %"ID_UINT32_FMT" KB\n", (UInt)(getUsedMemory() / 1024));
    IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);
}

/*
 * X$MEMPOOL를 위해서 필요한 정보를 채움.
 */
void iduMemList::fillMemPoolInfo( struct iduMemPoolInfo * aInfo )
{
    IDE_ASSERT( aInfo != NULL );

    aInfo->mFreeCnt    = mFreeChunkCnt;
    aInfo->mFullCnt    = mFullChunkCnt;
    aInfo->mPartialCnt = mPartialChunkCnt;
    aInfo->mChunkLimit = mP->mAutoFreeChunkLimit;
    aInfo->mChunkSize  = mP->mChunkSize;
    aInfo->mElemCnt    = mP->mElemCnt;
    aInfo->mElemSize   = mP->mElemSize;
}

