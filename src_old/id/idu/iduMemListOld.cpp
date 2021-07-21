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
 * $Id: iduMemListOld.cpp 40979 2010-08-10 04:02:04Z orc $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <iduRunTimeInfo.h>
#include <iduMemListOld.h>
#include <iduMemPoolMgr.h>

extern const vULong gFence;



void iduCheckMemConsistency(iduMemListOld * aMemList)
{
    vULong sCnt = 0;
    iduMemSlotQP *sSlot;

    //check free chunks
    iduMemChunkQP *sCur = aMemList->mFreeChunk.mNext;
    while(sCur != NULL)
    {
        sSlot = (iduMemSlotQP *)sCur->mTop;
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
        IDE_ASSERT( sCur->mMaxSlotCnt > 1 );

        //sCur->mMaxSlotCnt > 1
        IDE_ASSERT( (0  < sCur->mFreeSlotCnt) &&  
                    (sCur->mFreeSlotCnt < sCur->mMaxSlotCnt) );

        sCur = sCur->mNext;
        sCnt++;
    }

    IDE_ASSERT(sCnt == aMemList->mPartialChunkCnt);
}


//#ifndef MEMORY_ASSERT
#define iduCheckMemConsistency( xx )
//#endif

iduMemListOld::iduMemListOld()
{
}

iduMemListOld::~iduMemListOld(void)
{
}

/*---------------------------------------------------------
  iduMemListOld

   ___________ mFullChunk
  |iduMemListOld |    _____      _____
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
  |iduMemChunkQP |Slot|Slot|... | Slot|
  |____________|____|____|____|_____|

  iduMemChunkQP        : chunk 헤더(현재 chunk에 대한 내용을 담고 있다.)
  
  
  
  Slot:
   ___________________________________
  |momory element |iduMemChunkQP pointer|
  |_______________|___________________|

  memory element      : 사용자가 실제 메모리를 사용하는 영역
             이 영역은 사용자에게 할당되어 있지 않을때는, 다음
             (free)slot영역을 가리키는 포인터 역활을 한다.
  iduMemChunkQP pointer : slot이 속한 chunk의 헤더(chunk의 iduMemChunkQP영역)을
                        포인팅 하고 있다.
  -----------------------------------------------------------*/
/*-----------------------------------------------------------
 * task-2440 iduMemPool에서 메모리를 얻어올때 align된 메모리 주소를 얻어
 *          올 수 있게 하자.
 *          
 * chunk내에서 align이 일어나는 곳을 설명하겠다.
 * ___________________________________________
  |iduMemChunkQP |   |  Slot|  Slot|... |   Slot|
  |____________|___|______|______|____|_______|
  ^            ^   ^      ^
  1            2   3      4
  
  일단 chunk를 할당하면 1번은 임의의 주소값으로 설정된다.  이 주소값은
  사용자가 align되기를 원하는 주소가 아니기 때문에 그냥 놔둔다.
  2번주소 = 1번 주소 + SIZEOF(iduMemChunkQP)
  2번 주소는 위와 같이 구해지고, 2번주소를 align up시키면 3번처럼 뒷 공간을
  가리키게 된다. 그러면 3번은 align이 되어있게 된다.
  실제로 사용자에게 할당 되는 공간은 3번 공간 부터이다.
  4번이 align되어 있기 위해서는 slotSize가 align 되어 있어야 한다.
  그렇기 때문에 SlotSize도 실제 size보다 align up된 값을 사용한다.
  그러면 모든 slot의 주소는 align되어있음을 알 수있다.

  위의 2번과 3번사이 공간의 최대 크기는 alignByte이다.
  그렇기 때문에
  chunkSize = SIZEOF(iduMemChunkQP)+ alignByte +
              ('alignByte로 align up된 SIZEOF(slot)' * 'slot의 개수')
  이다.
 * ---------------------------------------------------------*/
/*-----------------------------------------------------------
 * Description: iduMemListOld를 초기화 한다.
 *
 * aIndex            - [IN] 이 함수를 호출한 모듈
 *                        (어디서 호출하는지 정보를 유지하기 위해 필요)
 * aSeqNumber        - [IN] iduMemListOld 식별 번호
 * aName             - [IN] iduMemListOld를 호출한 iduMemPool의 이름.
 * aElemSize         - [IN] 필요로 하는 메모리 크기( 할당받을 메모리의 단위크기)
 * aElemCnt        - [IN] 한 chunk내의 element의 개수, 즉, chunk내의
 *                        slot의 개수
 * aAutofreeChunkLimit-[IN] mFreeChunk의 개수가 이 숫자보다 큰 경우에
 *                          mFreeChunk중에서 전혀 다른 곳에서 쓰이지 않는
 *                          chunk를 메모리에서 해제한다.
 * aUseMutex         - [IN] mutex사용 여부                          
 * aAlignByte        - [IN] aline되어 있는 메모리를 할당받고 싶을때, 원하는 align
 *                          값을 넣어서 함수를 호출한다. 
 * ---------------------------------------------------------*/
IDE_RC iduMemListOld::initialize(iduMemoryClientIndex aIndex,
                              UInt   aSeqNumber,
                              SChar* aName,
                              vULong aElemSize,
                              vULong aElemCnt,
                              vULong aAutofreeChunkLimit,
                              idBool aUseMutex,
                              UInt   aAlignByte)
{
    SChar sBuffer[IDU_MUTEX_NAME_LEN*2];//128 bytes

    IDE_ASSERT( aName != NULL );
    //IDE_ASSERT( aElemCnt > 1 ); // then... why do you make THIS mem pool??

    mIndex     = aIndex;
    mElemCnt   = aElemCnt;
    mUseMutex  = aUseMutex;

    IDE_ASSERT(aElemSize <= ID_UINT_MAX);

    /*BUG-19455 mElemSize도 align 되어야 함 */
#if !defined(COMPILE_64BIT)
    mElemSize   = idlOS::align((UInt)aElemSize, sizeof(SDouble));
#else /* !defined(COMPILE_64BIT) */
    mElemSize   = idlOS::align((UInt)aElemSize);
#endif /* !defined(COMPILE_64BIT) */    
    
#ifdef MEMORY_ASSERT
    mSlotSize = (vULong)idlOS::align( (UInt)( mElemSize +
                                              ID_SIZEOF(gFence)       +
                                              ID_SIZEOF(iduMemSlotQP *) +
                                              ID_SIZEOF(gFence)), aAlignByte );
#else
    mSlotSize   = idlOS::align(mElemSize + ID_SIZEOF(iduMemSlotQP *), aAlignByte);
#endif
    
    mChunkSize            = ID_SIZEOF(iduMemChunkQP) + (mSlotSize * mElemCnt) + aAlignByte;
    mAlignByte            = aAlignByte;
    
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

    mAutoFreeChunkLimit    = aAutofreeChunkLimit;

    IDE_TEST(grow() != IDE_SUCCESS);

    if (mUseMutex == ID_TRUE)
    {
        if (aName == NULL)
        {
            aName = (SChar *)"noname";
        }

        idlOS::memset(sBuffer, 0, ID_SIZEOF(sBuffer));
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer), 
                         "%s"IDU_MEM_POOL_MUTEX_POSTFIX"%"ID_UINT32_FMT, aName, aSeqNumber);

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
 * iduMemListOld는 mFreeChunk와 mFullChunk의 두개의 chunk list를 유지한다.
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
IDE_RC iduMemListOld::destroy(idBool aBCheck)
{
    vULong i;
    iduMemChunkQP *sCur = NULL;

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

        IDE_TEST(iduMemMgr::free(sCur) != IDE_SUCCESS);
    }

    if(aBCheck == ID_TRUE)
    {
        if( mFullChunkCnt != 0)
        {
            ideLog::log(IDE_SERVER_0,ID_TRC_MEMLIST_WRONG_FULL_CHUNK_COUNT,
                        (UInt)mFullChunkCnt);

            IDE_DASSERT(0);
        }
    }

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
        IDE_TEST(iduMemMgr::free(sCur) != IDE_SUCCESS);
    }

    // BUG-19253
    // mFullChunk list에 연결된 chunk가 있으면 free시킨다.
    for (i = 0; i < mFullChunkCnt; i++)
    {
        sCur = mFullChunk.mNext;
        IDE_ASSERT(sCur != NULL);

        unlink(sCur);
        IDE_TEST(iduMemMgr::free(sCur) != IDE_SUCCESS);
    }

    if (mUseMutex == ID_TRUE)
    {
        IDE_TEST(mMutex.destroy() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

inline void iduMemListOld::unlink(iduMemChunkQP *aChunk)
{
    iduMemChunkQP *sBefore = aChunk->mPrev;
    iduMemChunkQP *sAfter  = aChunk->mNext;

    sBefore->mNext = sAfter;
    if (sAfter != NULL)
    {
        sAfter->mPrev = sBefore;
    }
}

inline void iduMemListOld::link(iduMemChunkQP *aBefore, iduMemChunkQP *aChunk)
{
    iduMemChunkQP *sAfter   = aBefore->mNext;

    aBefore->mNext        = aChunk;
    aChunk->mPrev         = aBefore;
    aChunk->mNext         = sAfter;

    if (sAfter != NULL)
    {
        sAfter->mPrev = aChunk;
    }
}

/*
 *iduMemChunkQP를 하나 할당하여, 이것을 iduMemListOld->mFreeChunk에 연결한다.
 */
IDE_RC iduMemListOld::grow(void)
{

    vULong       i;
    iduMemChunkQP *sChunk;
    iduMemSlotQP  *sSlot;
    iduMemSlotQP  *sFirstSlot;

    IDE_TEST(iduMemMgr::malloc(mIndex,
                               mChunkSize,
                               (void**)&sChunk,
                               IDU_MEM_FORCE)
             != IDE_SUCCESS);

    sChunk->mParent    = this;
    sChunk->mMaxSlotCnt  = mElemCnt;
    sChunk->mFreeSlotCnt = mElemCnt;
    sChunk->mTop       = NULL;

    /*chunk내의 각 slot을 mNext로 연결한다. 가장 마지막에 있는 slot이 sChunk->mTop에 연결된다.
     * mNext는 사용자에게 할당될때는 실제 사용하는 영역으로 쓰인다.*/
    sFirstSlot = (iduMemSlotQP*)((UChar*)sChunk + ID_SIZEOF( iduMemChunkQP));
    sFirstSlot = (iduMemSlotQP*)idlOS::align(sFirstSlot, mAlignByte);
    for (i = 0; i < mElemCnt; i++)
    {
        sSlot        = (iduMemSlotQP *)((UChar *)sFirstSlot + (i * mSlotSize));
        sSlot->mNext = sChunk->mTop;
        sChunk->mTop = sSlot;
#ifdef MEMORY_ASSERT
        *((vULong *)((UChar *)sSlot + mElemSize)) = gFence;
        *((iduMemChunkQP **)((UChar *)sSlot +
                           mElemSize +
                           ID_SIZEOF(gFence))) = sChunk;
        *((vULong *)((UChar *)sSlot +
                     mElemSize +
                     ID_SIZEOF(gFence)  +
                     ID_SIZEOF(iduMemChunkQP *))) = gFence;
#else
        *((iduMemChunkQP **)((UChar *)sSlot + mElemSize)) = sChunk;
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

/* ------------------------------------------------
 *  Allocation
 * ----------------------------------------------*/
IDE_RC iduMemListOld::alloc(void **aMem)
{

    iduMemChunkQP *sMyChunk=NULL;
    idBool       sIsFreeChunk=ID_TRUE; //어느 chunk에서 할당하기로 결정했는가?
                                       // 1: free  chunk   0: partial chunk


#if defined(ALTIBASE_MEMORY_CHECK)
    void  *  sStartPtr;
    UChar ** sTmpPtr;
    
    /*valgrind로 테스트할 때, 에러가 나타나는 것을 방지하기 위해서
     * valgrind사용시에는 memory를 할당해서 사용함    */
    /* BUG-20590 : align도 고려해야 함 - free때 사용함 */

//    -------------------------------------------------------
//    |     | Start Pointer | aligned returned memory       |
//    -------------------------------------------------------
//   / \           |
//    |            |
//    -------------+
    
    if( mAlignByte > ID_SIZEOF(void*) )
    {
        *aMem = (void *)idlOS::malloc((mElemSize * 2) + ID_SIZEOF(void*));
        sStartPtr = *aMem;

        // 메모리 시작 위치를 저장할 최소한의 공간 마련
        *aMem = (UChar**)*aMem + 1;
        *aMem = (void *)idlOS::align(*aMem, mAlignByte);
        sTmpPtr = (UChar**)*aMem - 1;
        *sTmpPtr = (UChar*)sStartPtr;
    }
    else
    {
        *aMem = (void *)idlOS::malloc(mElemSize + ID_SIZEOF(void*));
        IDE_ASSERT(*aMem != NULL);
        *((void**)*aMem) = *aMem;
        *aMem = (UChar*)((UChar**)*aMem + 1);
    }
    
    IDE_ASSERT(*aMem != NULL);

#else /* normal case : not memory check */

    if (mUseMutex == ID_TRUE)
    {
        IDE_ASSERT(mMutex.lock( NULL ) == IDE_SUCCESS);
    }
    else
    {
        /* No synchronisation */
    }

    iduCheckMemConsistency(this);

    //victim chunk를 선택.
    if (mPartialChunk.mNext != NULL) 
    {
        sMyChunk = mPartialChunk.mNext;
        sIsFreeChunk=ID_FALSE;
    }
    else
    {
        if (mFreeChunk.mNext == NULL) 
        {
            IDE_TEST(grow() != IDE_SUCCESS);
        }
        sMyChunk = mFreeChunk.mNext;
        sIsFreeChunk=ID_TRUE;

        IDE_ASSERT( sMyChunk != NULL );
    }

    iduCheckMemConsistency(this);

    IDE_ASSERT( sMyChunk->mTop != NULL );

    *aMem           = sMyChunk->mTop;

    sMyChunk->mTop = ((iduMemSlotQP *)(*aMem))->mNext;

    /* 
     * Partial list ->Full list, Free list ->Partial list:
                                chunk당  슬롯이 1개를 초과할때 (일반적인경우)
     * Free list -> Full list  :chunk당  슬롯이 1개 있는 경우.
     */
    if ((--sMyChunk->mFreeSlotCnt) == 0)
    {
        unlink(sMyChunk);
        link(&mFullChunk, sMyChunk);
        if( sIsFreeChunk == ID_TRUE ) //Free list -> Full list
        {
            IDE_ASSERT( mFreeChunkCnt > 0 );
            mFreeChunkCnt--;
        }
        else  //Partial list -> Full list
        {
            IDE_ASSERT( mPartialChunkCnt > 0 );
            mPartialChunkCnt--;
        }
        mFullChunkCnt++;
    }
    else 
    {
        // Free list ->Partial list
        if ( (sMyChunk->mMaxSlotCnt - sMyChunk->mFreeSlotCnt) == 1)
        {
            unlink(sMyChunk);
            link(&mPartialChunk, sMyChunk);
            IDE_ASSERT( mFreeChunkCnt > 0 );
            mFreeChunkCnt--;
            mPartialChunkCnt++;
        }
    }

    iduCheckMemConsistency(this);

    if (mUseMutex == ID_TRUE)
    {
        IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* No synchronisation */
    }

#endif

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

#if !defined(ALTIBASE_MEMORY_CHECK)
    if (mUseMutex == ID_TRUE)
    {
        IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* No synchronisation */
    }
#endif

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Free for one
 * ----------------------------------------------*/

IDE_RC iduMemListOld::memfree(void *aFreeElem)
{
    iduMemSlotQP*  sFreeElem;
    iduMemChunkQP *sCurChunk;

    IDE_ASSERT(aFreeElem != NULL);

#if defined(ALTIBASE_MEMORY_CHECK)
    /*valgrind로 테스트할 때, 에러가 나타나는 것을 방지하기 위해서
     * valgrind사용시에는 memory를 할당해서 사용함*/
    /* BUG-20590 : align을 위해 할당해주는 주소 바로 앞에 실제
       할당받은 메모리의 시작 주소가 적혀있음 */
    void * sStartPtr = (void*)*((UChar**)aFreeElem - 1);
    idlOS::free(sStartPtr);

#else /* normal case : not memory check */

    if (mUseMutex == ID_TRUE)
    {
        IDE_ASSERT(mMutex.lock( NULL ) == IDE_SUCCESS);
    }
    else
    {
        /* No synchronisation */
    }

#  ifdef MEMORY_ASSERT

    IDE_ASSERT(*((vULong *)((UChar *)aFreeElem + mElemSize)) == gFence);
    IDE_ASSERT(*((vULong *)((UChar *)aFreeElem +
                        mElemSize +
                        ID_SIZEOF(gFence)  +
                        ID_SIZEOF(iduMemChunkQP *))) == gFence);
    sCurChunk         = *((iduMemChunkQP **)((UChar *)aFreeElem + mElemSize +
                                           ID_SIZEOF(gFence)));
#  else
    sCurChunk         = *((iduMemChunkQP **)((UChar *)aFreeElem + mElemSize));
#  endif

    IDE_ASSERT((iduMemListOld*)this == sCurChunk->mParent);

    sFreeElem        = (iduMemSlotQP*)aFreeElem;
    sFreeElem->mNext = sCurChunk->mTop;
    sCurChunk->mTop  = sFreeElem;

    sCurChunk->mFreeSlotCnt++;
    IDE_ASSERT(sCurChunk->mFreeSlotCnt <= sCurChunk->mMaxSlotCnt);

    //slot cnt는 바뀌었지만 아직 리스트이동이 일어나지 않았으므로 check하면 안됨!
    //iduCheckMemConsistency(this);

    /*
     * Partial List -> Free List, Full list -> Partial list :일반적인 경우.
     * Full list -> Free list : chunk당 slot이 한개일때
     */
    if (sCurChunk->mFreeSlotCnt == sCurChunk->mMaxSlotCnt )
    {
        unlink(sCurChunk);

        if( sCurChunk->mMaxSlotCnt  == 1 ) //Full list -> Free list
        {
            IDE_ASSERT( mFullChunkCnt > 0 );
            mFullChunkCnt--;
        }
        else   //Partial List -> Free List
        {
            mPartialChunkCnt--;
        }

        //Limit을 넘어섰을 경우 autofree
        if (mFreeChunkCnt <= mAutoFreeChunkLimit)
        {
            link(&mFreeChunk, sCurChunk);
            mFreeChunkCnt++;
        }
        else
        {
            IDE_TEST(iduMemMgr::free(sCurChunk) != IDE_SUCCESS);
        }
    }
    else
    {
        // Full list -> Partial list
        if (sCurChunk->mFreeSlotCnt == 1)
        {
            unlink(sCurChunk);
            link(&mPartialChunk, sCurChunk);

            IDE_ASSERT( mFullChunkCnt > 0 );
            mFullChunkCnt--;
            mPartialChunkCnt++;
        }
    }
#endif

    iduCheckMemConsistency(this);

#if !defined(ALTIBASE_MEMORY_CHECK)
    if ( mUseMutex == ID_TRUE )
    {
        IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* No synchronisation */
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

#if !defined(ALTIBASE_MEMORY_CHECK)
    if ( mUseMutex == ID_TRUE )
    {
        IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* No synchronisation */
    }
#endif

    return IDE_FAILURE;

}

IDE_RC iduMemListOld::cralloc(void **aMem)
{
    IDE_TEST(alloc(aMem) != IDE_SUCCESS);
    idlOS::memset(*aMem, 0, mElemSize);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemListOld::shrink( UInt *aSize)
{
    iduMemChunkQP *sCur = NULL;
    iduMemChunkQP *sNxt = NULL;
    UInt         sFreeSizeDone = mFreeChunkCnt*mChunkSize;

    IDE_ASSERT( aSize != NULL );

    *aSize = 0;

    sCur = mFreeChunk.mNext;
    while( sCur != NULL )
    {
        sNxt = sCur->mNext;
        unlink(sCur);
        mFreeChunkCnt--;

        IDE_TEST(iduMemMgr::free(sCur) != IDE_SUCCESS);
        sCur = sNxt;
    }

    IDE_ASSERT( mFreeChunkCnt == 0 ); //당연.

    *aSize = sFreeSizeDone;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


#ifdef NOTDEF // for testing
SInt tmp()
{
    iduMemListOld *p;
    SChar *k;

    p->alloc((void **)&k);
    p->memfree(k);

    return 0;
}
#endif

UInt iduMemListOld::getUsedMemory()
{
    UInt sSize;

    sSize = mElemSize * mFreeChunkCnt * mElemCnt;
    sSize += (mElemSize * mFullChunkCnt * mElemCnt);

    return sSize;
}

void iduMemListOld::status()
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
void iduMemListOld::fillMemPoolInfo( struct iduMemPoolInfo * aInfo )
{
    IDE_ASSERT( aInfo != NULL );

    aInfo->mFreeCnt    = mFreeChunkCnt;
    aInfo->mFullCnt    = mFullChunkCnt;
    aInfo->mPartialCnt = mPartialChunkCnt;
    aInfo->mChunkLimit = mAutoFreeChunkLimit;
    aInfo->mChunkSize  = mChunkSize;
    aInfo->mElemCnt    = mElemCnt;
    aInfo->mElemSize   = mElemSize;
}

