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
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideMsgLog.h>
#include <iduRunTimeInfo.h>
#include <iduVLogShmMemList.h>

extern const vULong gFence;

void iduCheckMemConsistency( iduStShmMemList * aStShmMemList )
{
    return;

    vULong            sCnt = 0;
    iduStShmMemSlot * sSlot;

    //check free chunks
    iduStShmMemChunk *sCur =
        (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aStShmMemList->mFreeChunk.mAddrNext );

    while(sCur != NULL)
    {
        sSlot = (iduStShmMemSlot *)IDU_SHM_GET_ADDR_PTR( sCur->mAddrTop );
        while(sSlot != NULL)
        {
            sSlot = (iduStShmMemSlot*)IDU_SHM_GET_ADDR_PTR( sSlot->mAddrNext );
        }

        IDE_ASSERT( sCur->mMaxSlotCnt  == sCur->mFreeSlotCnt );

        sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sCur->mAddrNext );
        sCnt++;
        IDE_ASSERT(sCnt <= aStShmMemList->mFreeChunkCnt);
    }

    IDE_ASSERT(sCnt == aStShmMemList->mFreeChunkCnt);

    //check full chunks
    sCnt = 0;
    sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aStShmMemList->mFullChunk.mAddrNext );
    while(sCur != NULL)
    {
        IDE_ASSERT( sCur->mFreeSlotCnt == 0 );
        sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sCur->mAddrNext );
        sCnt++;
    }

    IDE_ASSERT(sCnt == aStShmMemList->mFullChunkCnt);

    //check partial chunks
    sCnt = 0;
    sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aStShmMemList->mPartialChunk.mAddrNext );
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

        sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sCur->mAddrNext );
        sCnt++;
    }

    IDE_ASSERT(sCnt == aStShmMemList->mPartialChunkCnt);
}


#ifndef MEMORY_ASSERT
#define iduCheckMemConsistency( xx )
#endif

iduShmMemList::iduShmMemList()
{
}

iduShmMemList::~iduShmMemList(void)
{
}

/*---------------------------------------------------------
  iduShmMemList

  ___________ mFullChunk
  |iduShmMemList |    _____      _____
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
  |iduStShmMemChunk |Slot|Slot|... | Slot|
  |____________|____|____|____|_____|

  iduStShmMemChunk        : chunk 헤더(현재 chunk에 대한 내용을 담고 있다.)



  Slot:
  ___________________________________
  |momory element |iduStShmMemChunk pointer|
  |_______________|___________________|

  memory element      : 사용자가 실제 메모리를 사용하는 영역
  이 영역은 사용자에게 할당되어 있지 않을때는, 다음
  (free)slot영역을 가리키는 포인터 역활을 한다.
  iduStShmMemChunk pointer : slot이 속한 chunk의 헤더(chunk의 iduStShmMemChunk영역)을
  포인팅 하고 있다.
  -----------------------------------------------------------*/
/*-----------------------------------------------------------
 * task-2440 scuMemPool에서 메모리를 얻어올때 align된 메모리 주소를 얻어
 *          올 수 있게 하자.
 *
 * chunk내에서 align이 일어나는 곳을 설명하겠다.
 * ___________________________________________
 |iduStShmMemChunk |   |  Slot|  Slot|... |   Slot|
 |____________|___|______|______|____|_______|
 ^            ^   ^      ^
 1            2   3      4

 일단 chunk를 할당하면 1번은 임의의 주소값으로 설정된다.  이 주소값은
 사용자가 align되기를 원하는 주소가 아니기 때문에 그냥 놔둔다.
 2번주소 = 1번 주소 + SIZEOF(iduStShmMemChunk)
 2번 주소는 위와 같이 구해지고, 2번주소를 align up시키면 3번처럼 뒷 공간을
 가리키게 된다. 그러면 3번은 align이 되어있게 된다.
 실제로 사용자에게 할당 되는 공간은 3번 공간 부터이다.
 4번이 align되어 있기 위해서는 slotSize가 align 되어 있어야 한다.
 그렇기 때문에 SlotSize도 실제 size보다 align up된 값을 사용한다.
 그러면 모든 slot의 주소는 align되어있음을 알 수있다.

 위의 2번과 3번사이 공간의 최대 크기는 alignByte이다.
 그렇기 때문에
 chunkSize = SIZEOF(iduStShmMemChunk)+ alignByte +
 ('alignByte로 align up된 SIZEOF(slot)' * 'slot의 개수')
 이다.
 * ---------------------------------------------------------*/
/*-----------------------------------------------------------
 * Description: iduShmMemList를 초기화 한다.
 *
 * aIndex              - [IN] 이 함수를 호출한 모듈
 *                            (어디서 호출하는지 정보를 유지하기 위해 필요)
 * aSeqNumber          - [IN] iduShmMemList 식별 번호
 * aName               - [IN] iduShmMemList를 호출한 scuMemPool의 이름.
 * aElemSize           - [IN] 필요로 하는 메모리 크기( 할당받을 메모리의 단위크기)
 * aElemCnt            - [IN] 한 chunk내의 element의 개수, 즉, chunk내의
 *                            slot의 개수
 * aAutofreeChunkLimit -[IN] mFreeChunk의 개수가 이 숫자보다 큰 경우에
 *                           mFreeChunk중에서 전혀 다른 곳에서 쓰이지 않는
 *                           chunk를 메모리에서 해제한다.
 * aUseMutex           - [IN] mutex사용 여부
 * aAlignByte          - [IN] aline되어 있는 메모리를 할당받고 싶을때, 원하는 align
 *                            값을 넣어서 함수를 호출한다.
 * ---------------------------------------------------------*/
IDE_RC iduShmMemList::initialize( idvSQL             * aStatistics,
                                  iduShmTxInfo       * aShmTxInfo,
                                  iduMemoryClientIndex aIndex,
                                  UInt                 aSeqNumber,
                                  SChar              * aName,
                                  vULong               aElemSize,
                                  vULong               aElemCnt,
                                  vULong               aAutofreeChunkLimit,
                                  idBool               aUseLatch,
                                  UInt                 aAlignByte,
                                  idShmAddr            aAddr4MemList,
                                  iduStShmMemList    * aMemListInfo )
{
    idrSVP sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    IDE_ASSERT( aName != NULL );
    //IDE_ASSERT( aElemCnt > 1 ); // then... why do you make THIS mem pool??

    aMemListInfo->mAddrSelf = aAddr4MemList;

    IDU_SHM_VALIDATE_ADDR_PTR( aAddr4MemList, aMemListInfo );

    aMemListInfo->mIndex    = aIndex;
    aMemListInfo->mSequence = aSeqNumber;
    aMemListInfo->mElemCnt  = aElemCnt;

    IDE_ASSERT( aElemSize <= ID_UINT_MAX );

    // 슬롯이 할당되지 않았을 때 Element영역은 iduStShmMemSlot으로 동작함.
    if( aElemSize < ID_SIZEOF(iduStShmMemSlot) )
    {
        aElemSize = ID_SIZEOF(iduStShmMemSlot);
    }

    /*BUG-19455 mElemSize도 align 되어야 함 */
#if !defined( COMPILE_64BIT )
    aMemListInfo->mElemSize   = idlOS::align( (UInt)aElemSize, sizeof(SDouble) );
#else /* !defined(COMPILE_64BIT) */
    aMemListInfo->mElemSize   = idlOS::align( (UInt)aElemSize );
#endif /* !defined(COMPILE_64BIT) */

    aMemListInfo->mSlotSize = idlOS::align(
        aMemListInfo->mElemSize + ID_SIZEOF(idShmAddr), aAlignByte );

    aMemListInfo->mChunkSize = ID_SIZEOF(iduStShmMemChunk) +
        ( aMemListInfo->mSlotSize * aMemListInfo->mElemCnt ) + aAlignByte;
    aMemListInfo->mAlignByte = aAlignByte;

    aMemListInfo->mFreeChunk.mAddrSelf =
        IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4MemList,
                                    iduStShmMemList,
                                    mFreeChunk );

    IDU_SHM_VALIDATE_ADDR_PTR( aMemListInfo->mFreeChunk.mAddrSelf,
                               &aMemListInfo->mFreeChunk );

    aMemListInfo->mFreeChunk.mAddrParent   = aAddr4MemList;
    aMemListInfo->mFreeChunk.mAddrNext     = IDU_SHM_NULL_ADDR;
    aMemListInfo->mFreeChunk.mAddrPrev     = IDU_SHM_NULL_ADDR;
    aMemListInfo->mFreeChunk.mAddrTop      = IDU_SHM_NULL_ADDR;
    aMemListInfo->mFreeChunk.mMaxSlotCnt   = 0;
    aMemListInfo->mFreeChunk.mFreeSlotCnt  = 0;

    aMemListInfo->mPartialChunk.mAddrSelf      =
        IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4MemList,
                                    iduStShmMemList,
                                    mPartialChunk );

    IDU_SHM_VALIDATE_ADDR_PTR( aMemListInfo->mPartialChunk.mAddrSelf,
                               &aMemListInfo->mPartialChunk );

    aMemListInfo->mPartialChunk.mAddrParent  = aAddr4MemList;
    aMemListInfo->mPartialChunk.mAddrNext    = IDU_SHM_NULL_ADDR;
    aMemListInfo->mPartialChunk.mAddrPrev    = IDU_SHM_NULL_ADDR;
    aMemListInfo->mPartialChunk.mAddrTop     = IDU_SHM_NULL_ADDR;
    aMemListInfo->mPartialChunk.mMaxSlotCnt  = 0;
    aMemListInfo->mPartialChunk.mFreeSlotCnt = 0;

    aMemListInfo->mFullChunk.mAddrSelf      =
        IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4MemList,
                                    iduStShmMemList,
                                    mFullChunk );

    IDU_SHM_VALIDATE_ADDR_PTR( aMemListInfo->mFullChunk.mAddrSelf,
                               &aMemListInfo->mFullChunk );

    aMemListInfo->mFullChunk.mAddrParent    = aAddr4MemList;
    aMemListInfo->mFullChunk.mAddrNext      = IDU_SHM_NULL_ADDR;
    aMemListInfo->mFullChunk.mAddrPrev      = IDU_SHM_NULL_ADDR;
    aMemListInfo->mFullChunk.mAddrTop       = IDU_SHM_NULL_ADDR;
    aMemListInfo->mFullChunk.mMaxSlotCnt    = 0;
    aMemListInfo->mFullChunk.mFreeSlotCnt   = 0;

    aMemListInfo->mFreeChunkCnt             = 0;
    aMemListInfo->mPartialChunkCnt          = 0;
    aMemListInfo->mFullChunkCnt             = 0;

    aMemListInfo->mAutoFreeChunkLimit       = aAutofreeChunkLimit;

    aMemListInfo->mUseLatch                 = aUseLatch;

    IDE_TEST( grow( aStatistics, aShmTxInfo, aMemListInfo ) != IDE_SUCCESS );

    if( aMemListInfo->mUseLatch == ID_TRUE )
    {
        iduShmLatchInit( IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4MemList,
                                                     iduStShmMemList,
                                                     mLatch ),
                         IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                         &aMemListInfo->mLatch );
    }

    IDE_TEST( iduVLogShmMemList::writeInitialize( aShmTxInfo,
                                                  sSavepoint.mLSN,
                                                  aMemListInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                      aShmTxInfo,
                                      &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

/* ---------------------------------------------------------------------------
 * BUG-19253
 * iduShmMemList는 mFreeChunk와 mFullChunk의 두개의 chunk list를 유지한다.
 * scuMemPool을 사용하는 모듈에서 scuMemPool을 destroy하기 전에
 * 모든 scuMemPool의 slot을 memfree했을 때만 정상적인 destroy가 수행된다.
 * 만약, 하나의 slot이라도 free되지 않은 채 destroy가 요청되면 debug mode에서는
 * DASSERT()로 죽고, release mode에서는 mFullChunk에 연결된 chunk만큼의
 * memory leak이 발생하게 된다.
 * debug mode에서는 memory leak을 감지하기 위해 DASSERT문을 유지하고,
 * release mode에서는 mFullChunk에 연결된 chunk들도 free 시키도록 한다.
 *
 * index build와 같이 사용한 scuMemPool을 완료 후 한꺼번에 free 시키는 경우
 * mFullChunk를 추가적으로 free 시키도록 한다. 한꺼번에 free 시키는 경우는
 * aBCheck가 ID_FALSE로 설정되어야 한다.
 * ---------------------------------------------------------------------------*/
IDE_RC iduShmMemList::destroy( idvSQL             * aStatistics,
                               iduShmTxInfo       * aShmTxInfo,
                               idrSVP             * aSavepoint,
                               iduStShmMemList    * aMemListInfo,
                               idBool               aBCheck )
{
    vULong             i;
    iduStShmMemChunk * sCur = NULL;

    idrSVP             sSVP;
    idrSVP             sInitSVP;
    vULong             sFreeChunkCnt;
    vULong             sPartialChunkCnt;
    vULong             sFullChunkCnt;

    idrLogMgr::setSavepoint( aShmTxInfo, &sInitSVP );

    if( aSavepoint == NULL )
    {
        aSavepoint = &sInitSVP;
    }

    sFreeChunkCnt = aMemListInfo->mFreeChunkCnt;

    for( i = 0; i < sFreeChunkCnt ; i++ )
    {
        sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aMemListInfo->mFreeChunk.mAddrNext );
        IDE_ASSERT(sCur != NULL);

        idrLogMgr::setSavepoint( aShmTxInfo, &sSVP );

        if( aBCheck == ID_TRUE )
        {
            if( sCur->mFreeSlotCnt != sCur->mMaxSlotCnt )
            {
                ideLog::log( IDE_SERVER_0,
                             ID_TRC_MEMLIST_MISMATCH_FREE_SLOT_COUNT,
                             (UInt)sCur->mFreeSlotCnt,
                             (UInt)sCur->mMaxSlotCnt );

                IDE_DASSERT(0);
            }
        }

        unlink( aShmTxInfo, sCur );

        IDE_ASSERT( aMemListInfo->mFreeChunkCnt > 0 );

        iduVLogShmMemList::writeFreeChunkCnt( aShmTxInfo, aMemListInfo );

        aMemListInfo->mFreeChunkCnt--;


        IDE_TEST( iduShmMgr::freeMem( aStatistics,
                                      aShmTxInfo,
                                      &sSVP,
                                      sCur ) != IDE_SUCCESS );

        IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSVP )
                  != IDE_SUCCESS );

    }

    if( aBCheck == ID_TRUE )
    {
        if( aMemListInfo->mFreeChunkCnt != 0)
        {
            ideLog::log(IDE_SERVER_0,ID_TRC_MEMLIST_WRONG_FULL_CHUNK_COUNT,
                        (UInt)aMemListInfo->mFullChunkCnt);

            IDE_DASSERT(0);
        }
    }

    sPartialChunkCnt = aMemListInfo->mPartialChunkCnt;
    for( i = 0; i < sPartialChunkCnt ; i++ )
    {
        idrLogMgr::setSavepoint( aShmTxInfo, &sSVP );

        sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR(
            aMemListInfo->mPartialChunk.mAddrNext );
        IDE_ASSERT( sCur != NULL );

        if( aBCheck == ID_TRUE )
        {
            if( ( sCur->mFreeSlotCnt == 0 ) ||
                ( sCur->mFreeSlotCnt == sCur->mMaxSlotCnt ) )
            {
                ideLog::log( IDE_SERVER_0,
                             ID_TRC_MEMLIST_WRONG_FREE_SLOT_COUNT,
                             (UInt)sCur->mFreeSlotCnt,
                             (UInt)sCur->mMaxSlotCnt );
                IDE_DASSERT(0);
            }
        }

        unlink( aShmTxInfo, sCur );

        IDE_ASSERT( aMemListInfo->mPartialChunkCnt > 0 );

        iduVLogShmMemList::writePartChunkCnt( aShmTxInfo, aMemListInfo );

        aMemListInfo->mPartialChunkCnt--;

        IDE_TEST( iduShmMgr::freeMem( aStatistics, aShmTxInfo, &sSVP, sCur )
                  != IDE_SUCCESS );

        IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSVP )
                  != IDE_SUCCESS );

    }

    // BUG-19253
    // mFullChunk list에 연결된 chunk가 있으면 free시킨다.
    sFullChunkCnt = aMemListInfo->mFullChunkCnt;
    for( i = 0; i < sFullChunkCnt ; i++ )
    {
        idrLogMgr::setSavepoint( aShmTxInfo, &sSVP );

        sCur =
            (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aMemListInfo->mFullChunk.mAddrNext );
        IDE_ASSERT( sCur != NULL );

        unlink( aShmTxInfo, sCur );

        IDE_ASSERT( aMemListInfo->mFullChunkCnt > 0 );

        iduVLogShmMemList::writeFullChunkCnt( aShmTxInfo, aMemListInfo );

        aMemListInfo->mFullChunkCnt--;

        IDE_TEST( iduShmMgr::freeMem( aStatistics, aShmTxInfo, &sSVP, sCur )
                  != IDE_SUCCESS );

        IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSVP )
                  != IDE_SUCCESS );

    }

    if( aMemListInfo->mUseLatch == ID_TRUE )
    {
        iduShmLatchDest( &aMemListInfo->mLatch );
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     aShmTxInfo,
                                     aSavepoint ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                      aShmTxInfo,
                                      &sInitSVP )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

/*
 * iduStShmMemChunk를 하나 할당하여, 이것을 iduShmMemList->mFreeChunk에 연결한다.
 */
IDE_RC iduShmMemList::grow( idvSQL           * aStatistics,
                            iduShmTxInfo     * aShmTxInfo,
                            iduStShmMemList  * aMemListInfo )
{
    vULong             i;
    iduStShmMemChunk * sNewChunk;
    iduStShmMemSlot  * sSlot;
    iduStShmMemSlot  * sFirstSlot;
    idShmAddr          sChunkAddr;
    UInt               sOffset;
    idrSVP             sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    IDE_TEST( iduShmMgr::allocMem( aStatistics,
                                   aShmTxInfo,
                                   aMemListInfo->mIndex,
                                   aMemListInfo->mChunkSize,
                                   (void**)&sNewChunk )
              != IDE_SUCCESS );

    sChunkAddr = iduShmMgr::getAddr( sNewChunk );

    sNewChunk->mAddrSelf    = sChunkAddr;
    sNewChunk->mAddrParent  = aMemListInfo->mAddrSelf;
    sNewChunk->mMaxSlotCnt  = aMemListInfo->mElemCnt;
    sNewChunk->mFreeSlotCnt = aMemListInfo->mElemCnt;
    sNewChunk->mAddrTop     = IDU_SHM_NULL_ADDR;

    /* chunk내의 각 slot을 mAddrNext로 연결한다. 가장 마지막에 있는 slot이
     * sNewChunk->mAddrTop에 연결된다. mAddrNext는 사용자에게 할당될때는 실제
     * 사용하는 영역으로 쓰인다.*/
    sOffset    = ID_SIZEOF( iduStShmMemChunk );
    sFirstSlot = (iduStShmMemSlot*)( (UChar*)sNewChunk + sOffset );
    sFirstSlot = (iduStShmMemSlot*)idlOS::align( sFirstSlot, aMemListInfo->mAlignByte );
    sOffset    = (SChar*)sFirstSlot - (SChar*)sNewChunk;

    for( i = 0; i < aMemListInfo->mElemCnt; i++ )
    {
        sSlot = (iduStShmMemSlot *)((UChar *)sFirstSlot + aMemListInfo->mSlotSize * i);
        sSlot->mAddrSelf = sChunkAddr + sOffset;

        IDE_DASSERT( sSlot == (iduStShmMemSlot*)IDU_SHM_GET_ADDR_PTR( sSlot->mAddrSelf ) );

        sSlot->mAddrNext = sNewChunk->mAddrTop;

        sNewChunk->mAddrTop = sSlot->mAddrSelf;

        *((idShmAddr*)((UChar *)sSlot + aMemListInfo->mElemSize)) = sNewChunk->mAddrSelf;

        sOffset += aMemListInfo->mSlotSize;
    }

    /* ------------------------------------------------
     *  mFreeChunk에 연결
     * ----------------------------------------------*/
    link( aShmTxInfo, &aMemListInfo->mFreeChunk, sNewChunk );

    iduVLogShmMemList::writeFreeChunkCnt( aShmTxInfo, aMemListInfo );

    aMemListInfo->mFreeChunkCnt++;


    // grow는 굳이 memfree하지 않아도 됨. 그냥 free에 달아둠
    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSavepoint )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Allocation
 * ----------------------------------------------*/
IDE_RC iduShmMemList::alloc( idvSQL          * aStatistics,
                             iduShmTxInfo    * aShmTxInfo,
                             iduStShmMemList * aMemListInfo,
                             idShmAddr       * aAddr4Mem,
                             void           ** aMem )
{
    iduStShmMemChunk * sMyChunk     = NULL;
    idrSVP             sSavepoint;
    idBool             sIsFreeChunk = ID_TRUE; //어느 chunk에서 할당하기로 결정했는가?
                                               // 1: free  chunk   0: partial chunk

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    iduCheckMemConsistency( aMemListInfo );

    //victim chunk를 선택.
    if( aMemListInfo->mPartialChunk.mAddrNext != IDU_SHM_NULL_ADDR )
    {
        sMyChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR(
            aMemListInfo->mPartialChunk.mAddrNext );
        sIsFreeChunk = ID_FALSE;
    }
    else
    {
        if( aMemListInfo->mFreeChunk.mAddrNext == IDU_SHM_NULL_ADDR )
        {
            IDE_TEST( grow( aStatistics, aShmTxInfo, aMemListInfo ) != IDE_SUCCESS );
        }

        sMyChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR(
            aMemListInfo->mFreeChunk.mAddrNext );
        sIsFreeChunk = ID_TRUE;
        IDE_ASSERT( sMyChunk != NULL );
    }

    iduCheckMemConsistency( aMemListInfo );

    IDE_ASSERT( sMyChunk->mAddrTop != IDU_SHM_NULL_ADDR );

    *aMem = IDU_SHM_GET_ADDR_PTR( sMyChunk->mAddrTop );

    // log mAddrTop , mFreeSlotCnt
    iduVLogShmMemList::writeAllocMyChunk( aShmTxInfo,
                                          sMyChunk );

    sMyChunk->mAddrTop = ((iduStShmMemSlot *)(*aMem))->mAddrNext;

    /*
     * Partial list ->Full list, Free list ->Partial list:
     chunk당  슬롯이 1개를 초과할때 (일반적인경우)
     * Free list -> Full list  :chunk당  슬롯이 1개 있는 경우.
     */
    if( ( --sMyChunk->mFreeSlotCnt ) == 0 )
    {
        unlink( aShmTxInfo, sMyChunk );

        if( sIsFreeChunk == ID_TRUE ) //Free list -> Full list
        {
            IDE_ASSERT( aMemListInfo->mFreeChunkCnt > 0 );

            iduVLogShmMemList::writeFreeChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mFreeChunkCnt--;
        }
        else  //Partial list -> Full list
        {
            IDE_ASSERT( aMemListInfo->mPartialChunkCnt > 0 );

            iduVLogShmMemList::writePartChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mPartialChunkCnt--;
        }

        link( aShmTxInfo, &aMemListInfo->mFullChunk, sMyChunk );

        iduVLogShmMemList::writeFullChunkCnt( aShmTxInfo, aMemListInfo );

        aMemListInfo->mFullChunkCnt++;
    }
    else
    {
        // Free list ->Partial list
        if( (sMyChunk->mMaxSlotCnt - sMyChunk->mFreeSlotCnt) == 1 )
        {
            unlink( aShmTxInfo, sMyChunk );

            IDE_ASSERT( aMemListInfo->mFreeChunkCnt > 0 );

            iduVLogShmMemList::writeFreeChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mFreeChunkCnt--;

            link( aShmTxInfo, &aMemListInfo->mPartialChunk, sMyChunk );

            iduVLogShmMemList::writePartChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mPartialChunkCnt++;
        }
    }

    iduCheckMemConsistency( aMemListInfo );

    IDU_SHM_VALIDATE_ADDR_PTR( ((iduStShmMemSlot *)(*aMem))->mAddrSelf, *aMem );

    *aAddr4Mem = ((iduStShmMemSlot *)(*aMem))->mAddrSelf;


    IDE_TEST( iduVLogShmMemList::writeNTAAlloc( aShmTxInfo,
                                                sSavepoint.mLSN,
                                                aMemListInfo,
                                                *aAddr4Mem )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Free for one
 * ----------------------------------------------*/
IDE_RC iduShmMemList::memfree( idvSQL          * aStatistics,
                               iduShmTxInfo    * aShmTxInfo,
                               idrSVP          * aSavepoint,
                               iduStShmMemList * aMemListInfo,
                               idShmAddr         aAddr4Mem,
                               void            * aFreeElem )
{
    iduStShmMemSlot  * sFreeElem;
    iduStShmMemChunk * sCurChunk;
    idShmAddr          sAddr4Shm;
    idrSVP             sSavepoint;

    IDE_ASSERT( aFreeElem != NULL );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    if( aSavepoint == NULL )
    {
        aSavepoint = &sSavepoint;
    }

    sAddr4Shm = *((idShmAddr*)((UChar *)aFreeElem + aMemListInfo->mElemSize));
    sCurChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4Shm );

    IDE_ASSERT( aMemListInfo->mAddrSelf == sCurChunk->mAddrParent );

    sFreeElem = (iduStShmMemSlot*)aFreeElem;

    iduVLogShmMemList::writeMemFree( aShmTxInfo,
                                     sCurChunk,
                                     sFreeElem);

    sFreeElem->mAddrSelf = aAddr4Mem;
    sFreeElem->mAddrNext = sCurChunk->mAddrTop;
    sCurChunk->mAddrTop  = sFreeElem->mAddrSelf;

    sCurChunk->mFreeSlotCnt++;
    IDE_ASSERT( sCurChunk->mFreeSlotCnt <= sCurChunk->mMaxSlotCnt );


    //slot cnt는 바뀌었지만 아직 리스트이동이 일어나지 않았으므로 check하면 안됨!
    //iduCheckMemConsistency(this);

    /*
     * Partial List -> Free List, Full list -> Partial list :일반적인 경우.
     * Full list -> Free list : chunk당 slot이 한개일때
     */
    if( sCurChunk->mFreeSlotCnt == sCurChunk->mMaxSlotCnt )
    {
        unlink( aShmTxInfo, sCurChunk );

        if( sCurChunk->mMaxSlotCnt == 1 ) //Full list -> Free list
        {
            IDE_ASSERT( aMemListInfo->mFullChunkCnt > 0 );

            iduVLogShmMemList::writeFullChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mFullChunkCnt--;

        }
        else   //Partial List -> Free List
        {
            IDE_ASSERT( aMemListInfo->mPartialChunkCnt > 0 );

            iduVLogShmMemList::writePartChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mPartialChunkCnt--;

        }

        //Limit을 넘어섰을 경우 autofree
        if( aMemListInfo->mFreeChunkCnt <= aMemListInfo->mAutoFreeChunkLimit )
        {
            link( aShmTxInfo, &aMemListInfo->mFreeChunk, sCurChunk );

            iduVLogShmMemList::writeFreeChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mFreeChunkCnt++;


            iduCheckMemConsistency( aMemListInfo );
        }
        else
        {
            iduCheckMemConsistency( aMemListInfo );

            IDE_TEST( iduShmMgr::freeMem( aStatistics,
                                          aShmTxInfo,
                                          aSavepoint,
                                          sCurChunk )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Full list -> Partial list
        if( sCurChunk->mFreeSlotCnt == 1 )
        {
            unlink( aShmTxInfo, sCurChunk );
            IDE_ASSERT( aMemListInfo->mFullChunkCnt > 0 );

            iduVLogShmMemList::writeFullChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mFullChunkCnt--;


            link( aShmTxInfo, &aMemListInfo->mPartialChunk, sCurChunk );

            iduVLogShmMemList::writePartChunkCnt( aShmTxInfo, aMemListInfo );

            aMemListInfo->mPartialChunkCnt++;

        }

        iduCheckMemConsistency( aMemListInfo );
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, aSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC iduShmMemList::cralloc( idvSQL          * aStatistics,
                               iduShmTxInfo    * aShmTxInfo,
                               iduStShmMemList * aMemListInfo,
                               idShmAddr       * aAddr4Mem,
                               void           ** aMem )
{
    IDE_TEST( alloc( aStatistics,
                     aShmTxInfo,
                     aMemListInfo,
                     aAddr4Mem,
                     aMem ) != IDE_SUCCESS );

    idlOS::memset( *aMem, 0, aMemListInfo->mElemSize );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduShmMemList::shrink( idvSQL          * aStatistics,
                              iduShmTxInfo    * aShmTxInfo,
                              iduStShmMemList * aMemListInfo,
                              UInt            * aSize )
{
    iduStShmMemChunk * sCur = NULL;
    iduStShmMemChunk * sNxt = NULL;
    UInt               sFreeSizeDone =
        aMemListInfo->mFreeChunkCnt * aMemListInfo->mChunkSize;

    idrSVP      sSVP;
    idrSVP      sInitSVP;

    IDE_ASSERT( aSize != NULL );

    *aSize = 0;

    idrLogMgr::setSavepoint( aShmTxInfo, &sInitSVP );

    sCur = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( aMemListInfo->mFreeChunk.mAddrNext );

    while( sCur != NULL )
    {
        idrLogMgr::setSavepoint( aShmTxInfo, &sSVP );

        sNxt = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sCur->mAddrNext );
        unlink( aShmTxInfo, sCur );

        iduVLogShmMemList::writeFreeChunkCnt( aShmTxInfo, aMemListInfo );

        aMemListInfo->mFreeChunkCnt--;

        IDE_TEST( iduShmMgr::freeMem( aStatistics, aShmTxInfo, &sSVP, sCur )
                  != IDE_SUCCESS );
        sCur = sNxt;
    }

    IDE_ASSERT( aMemListInfo->mFreeChunkCnt == 0 ); //당연.

    *aSize = sFreeSizeDone;

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sInitSVP )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sInitSVP )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

UInt iduShmMemList::getUsedMemory( iduStShmMemList * aMemListInfo )
{
    UInt sSize;

    sSize = aMemListInfo->mElemSize * aMemListInfo->mFreeChunkCnt * aMemListInfo->mElemCnt;
    sSize += (aMemListInfo->mElemSize * aMemListInfo->mFullChunkCnt * aMemListInfo->mElemCnt);

    return sSize;
}

void iduShmMemList::status( iduStShmMemList * aMemListInfo )
{
    SChar sBuffer[IDE_BUFFER_SIZE];

    idlOS::snprintf( sBuffer,
                     ID_SIZEOF(sBuffer),
                     "    Memory Usage: %"ID_UINT32_FMT" KB\n",
                     (UInt)(getUsedMemory( aMemListInfo ) / 1024));
    IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);
}

/*
 * X$MEMPOOL를 위해서 필요한 정보를 채움.
 */
void iduShmMemList::fillMemPoolInfo( iduStShmMemList     * aMemListInfo,
                                     iduShmMemPoolState  * aInfo )
{
    IDE_ASSERT( aInfo != NULL );

    aInfo->mFreeCnt    = aMemListInfo->mFreeChunkCnt;
    aInfo->mFullCnt    = aMemListInfo->mFullChunkCnt;
    aInfo->mPartialCnt = aMemListInfo->mPartialChunkCnt;
    aInfo->mChunkLimit = aMemListInfo->mAutoFreeChunkLimit;
    aInfo->mChunkSize  = aMemListInfo->mChunkSize;
    aInfo->mElemCnt    = aMemListInfo->mElemCnt;
    aInfo->mElemSize   = aMemListInfo->mElemSize;
}

void iduShmMemList::dumpPartialFreeChunk4TSM( iduStShmMemList * aMemListInfo )
{
    iduStShmMemChunk * sMyChunk = NULL;
    UInt               sID = 0;

    sMyChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR(
        aMemListInfo->mPartialChunk.mAddrNext );

    while(1)
    {
        if( sMyChunk == NULL )
        {
            break;
        }

        idlOS::printf( "##[%d] Free Count:%"ID_UINT32_FMT"\n",
                       sID,
                       sMyChunk->mFreeSlotCnt );

        sID++;

        sMyChunk =
            (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sMyChunk->mAddrNext );
    }
}

