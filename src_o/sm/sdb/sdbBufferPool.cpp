/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $$Id:$
 **********************************************************************/
/******************************************************************************
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ******************************************************************************/
#include <smDef.h>
#include <smcDef.h>
#include <smcTable.h>
#include <smErrorCode.h>
#include <sddDef.h>
#include <sddDiskMgr.h>
#include <sdbDef.h>
#include <sdbReq.h>
#include <sdbFlushList.h>
#include <sdbLRUList.h>
#include <sdbPrepareList.h>
#include <sdbCPListSet.h>
#include <sdbBufferPool.h>
#include <sdbBCB.h>
#include <sdbFlushMgr.h>
#include <sdp.h>
#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h> 

IDE_RC sdbBufferPool::initPrepareList()
{
    UInt    i = 0, j = 0;
    UInt    sState = 0;

    /* TC/FIT/Limit/sm/sdb/sdbBufferPool_initPrepareList_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbBufferPool::initPrepareList::malloc", 
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDB,
                                       (ULong)ID_SIZEOF( sdbPrepareList ) * mPrepareListCnt,
                                       (void**)&mPrepareList ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    for( i = 0; i < mPrepareListCnt; i++ )
    {
        IDE_TEST(mPrepareList[i].initialize(i)
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        for( j = 0 ; j < i; j++)
        {
            IDE_ASSERT( mPrepareList[i].destroy() == IDE_SUCCESS );
        }

        IDE_ASSERT( iduMemMgr::free( mPrepareList )
                    == IDE_SUCCESS );

        mPrepareList = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC sdbBufferPool::initLRUList()
{
    UInt i = 0, j = 0;
    UInt sState = 0;
    UInt sHotMax;

    /* TC/FIT/Limit/sm/sdb/sdbBufferPool_initLRUList_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbBufferPool::initLRUList::malloc", 
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDB,
                                       (ULong)ID_SIZEOF( sdbLRUList ) * mLRUListCnt,
                                       (void**)&mLRUList) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    sHotMax = getHotCnt4LstFromPCT(smuProperty::getHotListPct());

    for( i = 0 ; i < mLRUListCnt; i++)
    {
        IDE_TEST(mLRUList[i].initialize(i, sHotMax, &mStatistics)
                 != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        for( j = 0 ; j < i; j++)
        {
            IDE_ASSERT( mLRUList[i].destroy() == IDE_SUCCESS );
        }

        IDE_ASSERT( iduMemMgr::free(mLRUList )
                    == IDE_SUCCESS );
        mLRUList = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC sdbBufferPool::initFlushList()
{
    UInt i = 0, j = 0;
    UInt sState = 0;

    /* TC/FIT/Limit/sm/sdb/sdbBufferPool_initFlushList_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbBufferPool::initFlushList::malloc", 
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDB,
                                       (ULong)ID_SIZEOF( sdbFlushList ) * mFlushListCnt,
                                       (void**)&mFlushList ) != IDE_SUCCESS,
                    insufficient_memory );

    sState = 1;

    for( i = 0 ; i < mFlushListCnt; i++)
    {
        IDE_TEST( mFlushList[i].initialize( i, SDB_BCB_FLUSH_LIST )
                  != IDE_SUCCESS );
    }


    sState = 2;

    /* TC/FIT/Limit/sm/sdb/sdbBufferPool_initFlushList_delayedFlushList_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbBufferPool::initFlushList::delayedFlushList_malloc", 
                          insufficient_memory );
    /* PROJ-2669
     * Delayed flush list 생성
     * Normal Flush list 와 동일한 갯수로 생성한다 */
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDB,
                                       (ULong)ID_SIZEOF( sdbFlushList ) * mFlushListCnt,
                                       (void**)&mDelayedFlushList ) != IDE_SUCCESS,
                    insufficient_memory );

    sState = 3;

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        IDE_TEST( mDelayedFlushList[i].initialize( i, SDB_BCB_DELAYED_FLUSH_LIST )
                  != IDE_SUCCESS );
    }

    sState = 4;

    setMaxDelayedFlushListPct( smuProperty::getDelayedFlushListPct() );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
    case 4:
        i = mFlushListCnt;
    case 3:
        for ( j = 0 ; j < i; j++ )
        {
            IDE_ASSERT( mDelayedFlushList[j].destroy() == IDE_SUCCESS );
        }
        IDE_ASSERT( iduMemMgr::free( mDelayedFlushList ) == IDE_SUCCESS );
        mDelayedFlushList = NULL;

    case 2:
        i = mFlushListCnt;
    case 1:
        for ( j = 0 ; j < i; j++ )
        {
            IDE_ASSERT( mFlushList[j].destroy() == IDE_SUCCESS );
        }

        IDE_ASSERT( iduMemMgr::free( mFlushList )
                    == IDE_SUCCESS );
        mFlushList = NULL;

    default:
        break;
    }

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *
 * aBCBCnt        - [IN]  buffer pool에 속하게될 총 BCB갯수.
 * aBucketCnt     - [IN]  buffer pool에서 사용하는 hash table의 bucket
 *                      갯수.. sdbBCBHash.cpp 참조
 * aBucketCntPerLatch  - [IN]  hash latch하나가 커버하는 bucket의 개수..
 *                            sdbBCBHash.cpp참조
 * aLRUListCnt    - [IN]  내부에서 contention을 줄이기 위해 여러개의
 *                      리스트를 유지한다.  sdbLRUList의 개수
 * aPrepareListCnt- [IN]  sdbPrepareList의 개수.
 * aFlushListCnt  - [IN]  sdbFlushList의 개수.
 * aCPListCnt     - [IN]  checkpoint list의 개수.
 *
 ****************************************************************/
IDE_RC sdbBufferPool::initialize( UInt aBCBCnt,
                                  UInt aBucketCnt,
                                  UInt aBucketCntPerLatch,
                                  UInt aLRUListCnt,
                                  UInt aPrepareListCnt,
                                  UInt aFlushListCnt,
                                  UInt aCPListCnt )
{
    UInt sState = 0;

    IDE_ASSERT( aLRUListCnt > 0);
    IDE_ASSERT( aPrepareListCnt > 0);
    IDE_ASSERT( aFlushListCnt > 0);
    IDE_ASSERT( aCPListCnt > 0);
    IDE_ASSERT( aBCBCnt > 0 );

    mPageSize = SD_PAGE_SIZE;
    mBCBCnt   = aBCBCnt;
    mHotTouchCnt = smuProperty::getHotTouchCnt();
     
    /* Secondary Buffer의 identify 이전에 생성되므로 아직은 Unserviceable 상태로 설정 */
    setSBufferServiceState( ID_FALSE ); 

    mLRUListCnt = aLRUListCnt;
    IDE_TEST( initLRUList () != IDE_SUCCESS );
    sState = 1;

    mFlushListCnt = aFlushListCnt;
    IDE_TEST( initFlushList() != IDE_SUCCESS );
    sState = 2;

    mPrepareListCnt = aPrepareListCnt;
    IDE_TEST( initPrepareList() != IDE_SUCCESS );
    sState = 3;

    IDE_TEST( mCPListSet.initialize( aCPListCnt, SD_LAYER_BUFFER_POOL )
              != IDE_SUCCESS );
    sState = 4;

    IDE_TEST( mHashTable.initialize( aBucketCnt,
                                     aBucketCntPerLatch,
                                     SD_LAYER_BUFFER_POOL )
              != IDE_SUCCESS );
    sState = 5;

    setLRUSearchCnt( smuProperty::getBufferVictimSearchPct());

    setInvalidateWaitTime( SDB_BUFFER_POOL_INVALIDATE_WAIT_UTIME );

    IDE_TEST( mStatistics.initialize(this) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 5:
            IDE_ASSERT( mHashTable.destroy() == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( mCPListSet.destroy() == IDE_SUCCESS );
        case 3:
            destroyPrepareList();
        case 2:
            destroyFlushList();
        case 1:
            destroyLRUList();
        default:
            break;
    }

    return IDE_FAILURE;
}

void sdbBufferPool::destroyPrepareList()
{
    UInt i;

    for( i = 0 ; i < mPrepareListCnt; i++ )
    {
        IDE_ASSERT( mPrepareList[i].destroy() == IDE_SUCCESS );
    }

    IDE_ASSERT( iduMemMgr::free( mPrepareList ) == IDE_SUCCESS );
    mPrepareList = NULL;

    return;
}

void sdbBufferPool::destroyLRUList()
{
    UInt i;

    for( i = 0 ; i < mLRUListCnt; i++)
    {
        IDE_ASSERT( mLRUList[i].destroy() == IDE_SUCCESS );
    }

    IDE_ASSERT( iduMemMgr::free( mLRUList ) == IDE_SUCCESS );
    mLRUList = NULL;

    return;
}

void sdbBufferPool::destroyFlushList()
{
    UInt i;

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        IDE_ASSERT( mDelayedFlushList[i].destroy() == IDE_SUCCESS );
    }
    IDE_ASSERT( iduMemMgr::free( mDelayedFlushList ) == IDE_SUCCESS );
    mDelayedFlushList = NULL;

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        IDE_ASSERT( mFlushList[i].destroy() == IDE_SUCCESS );
    }

    IDE_ASSERT( iduMemMgr::free( mFlushList ) == IDE_SUCCESS );
    mFlushList = NULL;

    return;
}

IDE_RC sdbBufferPool::destroy()
{
    IDE_ASSERT( mHashTable.destroy() == IDE_SUCCESS );

    IDE_ASSERT( mCPListSet.destroy() == IDE_SUCCESS );

    destroyPrepareList();
    destroyLRUList();
    destroyFlushList();

    IDE_ASSERT( mStatistics.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  buffer pool이 가진 LRUList중 가장 길이가 작은 list를 리턴한다.
 ****************************************************************/
sdbLRUList *sdbBufferPool::getMinLengthLRUList()
{
    UInt         i;
    sdbLRUList  *sMinList = &mLRUList[0];

    for (i = 1; i < mLRUListCnt; i++)
    {
        if (sMinList->getColdLength() > mLRUList[i].getColdLength())
        {
            sMinList = &mLRUList[i];
        }
    }

    return sMinList;
}

/****************************************************************
 * Description :
 *  buffer pool이 가진 prepare List중 가장 길이가 작은 prepare list를
 *  반납한다.
 ****************************************************************/
sdbPrepareList  *sdbBufferPool::getMinLengthPrepareList(void)
{
    UInt            i;
    sdbPrepareList *sMinList = &mPrepareList[0];

    for( i = 1 ; i < mPrepareListCnt; i++)
    {
        if( sMinList->getLength() > mPrepareList[i].getLength())
        {
            sMinList = &mPrepareList[i];
        }
    }

    return sMinList;
}

/****************************************************************
 * Description :
 *  buffer pool이 가진 flush List중 가장 길이가 긴 flush list를
 *  리턴한다.
 ****************************************************************/
sdbFlushList  *sdbBufferPool::getMaxLengthFlushList(void)
{
    UInt          i;
    sdbFlushList *sMaxList = &mFlushList[0];

    for( i = 1 ; i < mFlushListCnt; i++ )
    {
        if ( getFlushListLength( sMaxList->getID() )
             < getFlushListLength( i ) )
        {
            sMaxList = &mFlushList[i];
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sMaxList;
}

/****************************************************************
 * Description :
 *   PROJ-2669
 *     모든 Normal/Delayed flush list 의 길이의 총합을 반환한다.
 ****************************************************************/
UInt sdbBufferPool::getFlushListTotalLength(void)
{
    UInt          i;
    UInt          sTotalLength = 0;

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        sTotalLength += mFlushList[i].getPartialLength();
        sTotalLength += mDelayedFlushList[i].getPartialLength();
    }

    return sTotalLength;
}

/****************************************************************
 * Description :
 *   PROJ-2669
 *     모든 Normal flush list 의 길이의 총합을 반환한다.
 ****************************************************************/
UInt sdbBufferPool::getFlushListNormalLength(void)
{
    UInt          i;
    UInt          sTotalLength = 0;

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        sTotalLength += mFlushList[i].getPartialLength();
    }

    return sTotalLength;
}

/****************************************************************
 * Description :
 *   PROJ-2669
 *     모든 Delayed flush list 의 길이를 반환한다.
 ****************************************************************/
UInt sdbBufferPool::getDelayedFlushListLength(void)
{
    UInt          sTotalLength = 0;
    UInt          i;

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        sTotalLength += mDelayedFlushList[i].getPartialLength();
    }

    return sTotalLength;
}

/****************************************************************
 * Description :
 *   PROJ-2669
 *     모든 Normal/Delayed flush list 의 길이의 합을 반환한다.
 ****************************************************************/
UInt sdbBufferPool::getFlushListLength( UInt aID )
{
    UInt          sTotalLength = 0;

    sTotalLength += mFlushList[ aID ].getPartialLength();
    sTotalLength += mDelayedFlushList[ aID ].getPartialLength();

    return sTotalLength;
}

/****************************************************************
 * Description :
 *   PROJ-2669
 *     Delayed flush list 의 최대 길이를 설정한다.
 ****************************************************************/
void sdbBufferPool::setMaxDelayedFlushListPct( UInt aMaxListPct )
{
    UInt sDelayedFlushMax;
    UInt i;

    sDelayedFlushMax = getDelayedFlushCnt4LstFromPCT( aMaxListPct );

    for ( i = 0 ; i < mFlushListCnt; i++ )
    {
        mDelayedFlushList[ i ].setMaxListLength( sDelayedFlushMax );
    }
}

/****************************************************************
 * Description :
 *   PROJ-2669
 *     Delayed flush list 를 반환한다.
 ****************************************************************/
sdbFlushList *sdbBufferPool::getDelayedFlushList( UInt aIndex )
{
    IDE_DASSERT( aIndex < mFlushListCnt );
    return &mDelayedFlushList[ aIndex ];
}

/****************************************************************
 * Description :
 *  buffer pool의 prepare list에 free상태의 bcb들을 삽입할 때 사용한다.
 *  가장 길이가 짧은 prepareList에 삽입한다.
 *  하나의 BCB를 삽입하고자 하는 경우엔 aFirstBCB와 aLastBCB를 같은
 *  BCB로 하고, aLength를 1로 한다.
 *
 * aStatistics  - [IN]  통계정보
 * aFirstBCB    - [IN]  삽입하려는 BCB리스트의 첫 BCB
 * aLastBCB     - [IN]  삽입하려는 BCB리스트의 마지막 BCB
 * aLength      - [IN]  삽입하고자 하는 BCB리스트의 길이
 ****************************************************************/
void sdbBufferPool::addBCBList2PrepareLst( idvSQL    *aStatistics,
                                           sdbBCB    *aFirstBCB,
                                           sdbBCB    *aLastBCB,
                                           UInt       aLength )
{
    sdbPrepareList  *sMinList;

    IDE_DASSERT( aLength > 0 );

    sMinList = getMinLengthPrepareList();

    sMinList->addBCBList( aStatistics,
                          aFirstBCB,
                          aLastBCB,
                          aLength);
    return;
}


/****************************************************************
 * Description :
 *  aFirstBCB 부터 연결된 모든 BCB 리스트를 버퍼풀의 prepare list에
 *  골고루 삽입한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFirstBCB   - [IN]  BCB리스트의 시작 BCB
 *  aLastBCB    - [IN]  BCB리스트의 마지막 BCB
 *  aLength     - [IN]  BCB리스트의 총 길이
 ****************************************************************/
void sdbBufferPool::distributeBCB2PrepareLsts( idvSQL    *aStatistics,
                                               sdbBCB    *aFirstBCB,
                                               sdbBCB    *aLastBCB,
                                               UInt       aLength )
{
    UInt        i;
    UInt        sBCBCnt;
    UInt        sQuota;
    smuList    *sFirst;
    smuList    *sCurrent;
    smuList    *sNextFirst;

    IDE_DASSERT(aFirstBCB != NULL);

    if( aLength < mPrepareListCnt )
    {
        // 원래 1개씩 prepareList에 삽입을 해야 하지만,
        // aLength가 무척 작은 경우엔 분산에 별 의미가 없다.
        // ( 게다가 aLength가 mPrepareListCnt보다 작은경우엔 본 함수가
        //   거의 호출되지도 않는다. 호출된다면, 설정을 잘못한 것이다.)
        // 오히려, 그것을 위해 BCB를 매번 끊고 삽입하는 연산이
        //  더비싸다고 생각되고, 코드를 simple하게 가기 위해서
        // 그냥 한 리스트에 삽입한다.
        mPrepareList[aLength].addBCBList( aStatistics,
                                          aFirstBCB,
                                          aLastBCB,
                                          aLength);
    }
    else
    {
        sQuota = aLength / mPrepareListCnt;

        sFirst   = &aFirstBCB->mBCBListItem;
        sCurrent = sFirst;
        IDE_ASSERT(SMU_LIST_GET_PREV(sFirst) == NULL);

        for( i = 0 ; i < mPrepareListCnt - 1; i++)
        {
            for(sBCBCnt = 1; sBCBCnt < sQuota; sBCBCnt++)
            {
                sCurrent = SMU_LIST_GET_NEXT(sCurrent);
            }

            sNextFirst = SMU_LIST_GET_NEXT( sCurrent );
            SMU_LIST_CUT_BETWEEN( sCurrent, sNextFirst );

            mPrepareList[i].addBCBList( aStatistics,
                                        (sdbBCB*)sFirst->mData,
                                        (sdbBCB*)sCurrent->mData,
                                        sBCBCnt );

            sFirst   = sNextFirst;
            sCurrent = sFirst;
        }

        sBCBCnt = 1;

        for(; SMU_LIST_GET_NEXT( sCurrent ) != NULL;
            sCurrent = SMU_LIST_GET_NEXT( sCurrent ) )
        {
            sBCBCnt++;
        }

        IDE_ASSERT( sBCBCnt == sQuota + ( aLength % mPrepareListCnt ) );
        IDE_ASSERT( (sdbBCB*)sCurrent->mData == aLastBCB );

        mPrepareList[i].addBCBList( aStatistics,
                                    (sdbBCB*)sFirst->mData,
                                    (sdbBCB*)sCurrent->mData,
                                    sBCBCnt );
    }
}

void sdbBufferPool::setHotMax( idvSQL *aStatistics,
                               UInt    aHotMaxPCT )
{
    UInt sHotMax;
    UInt i;

    sHotMax = getHotCnt4LstFromPCT( aHotMaxPCT );

    for( i = 0 ; i < mLRUListCnt; i++ )
    {
        mLRUList[i].setHotMax( aStatistics, sHotMax );
    }
}

/*****************************************************************
 * 쓰레드들이 BCB를 접근할 수있는 경로는 총 3가지 이다.
 * 1. list를 통해 = 주로 victim을 찾는다던가, flush를 할때
 * 2. hash를 통해 = 주로 getPage할때 hit한 경우
 * 3. BCB포인터를 통해 = 매우 다양한 경우가 존재.
 * 이들간의 동시성 제어에 대해 살펴보자.
 *
 * ****************** BufferPool의 동시성 제어 ********************
 * 1. list vs list =
 *      한 쓰레드가 BCB를 한 리스트에서 제거를 해버리면 다른 쓰레드 접근을
 *      할 수 없으므로, 동시성이 쉽게 제어된다.  이때 리스트에 뮤텍스 필요.
 *
 * 2. hash vs hash =
 *  2.1 hash 삽입 vs hash 삽입,
 *      hash 삭제 vs hash 삽입,
 *      hash 삭제 vs hash 삭제,
 *      hash 삽입 vs hash 탐색,
 *      hash 삭제 vs hash 탐색,
 *          이 경우에 hash 삽입은 hash chains x 래치를 잡고, hash 탐색 연산은
 *          hash chains s 래치를 요구한다.  동시성 보장됨
 *
 *  2.2 hash 탐색 vs hash 탐색
 *          hash 탐색을 하여 수행하는 연산간의 동시성을 보장해 주어야 한다.
 *          이때 하는 연산은 fix를 하는 연산인데, 이것의 동시성을 보장하기
 *          위해 별도로 BCB의 mMutex를 잡고 제어한다.
 *
 * 3. list vs hash =
 *      list를 통해 접근하여 하는 연산과 변경하는 값은
 *      hash를 통해 접근하여 하는 연산과 변경하는 값과 완전히 분리되어 있다.
 *      그렇기 때문에 둘간의 동시성은 제어를 하지 않아도 된다.
 *
 * 4. BCB포인터 vs( list or hash or BCB포인터)
 *      BCB 포인터를 통해 접근하여 어떤 연산을 수행하는지 여부에 따라
 *      잡는 mutex가 다르고, 변경하는 연산이 다르다.
 *      그렇기 때문에, 각 연산이 수행되는 조건과 변경하는 연산, 그리고 그들이
 *      잡는 뮤텍스를 잘 파악해서 동시성을 섬세하게 조정해야 한다.
 *
 *      아래에 기술한 것은 BCB의 멤버 변수에 해당하는 값이 어떤 연산을 통해
 *      어떻게 바뀌며, 그리고 어떤 래치를 잡는지를 나타낸다.
 *
 *  4.1 mState
 *      mState를 변경하는 모든 연산은 동시성 제어를 위해 기본적으로
 *      BCB의mMutex 잡고 변경한다.
 *
 *      invalidateBCB   = SDB_BCB_DIRTY -> SDB_BCB_CLEAN
 *      setDirty = SDB_BCB_CLEAN -> SDB_BCB_DIRTY
 *      setDirty = SDB_BCB_INIOB -> SDB_BCB_REDIRTY
 *              setDirty의 경우엔 페이지 x래치를 잡고 들어오며,
 *              항상 fix된 BCB에 대해서만 수행한다.
 *
 *      writeIOB = SDB_BCB_INIOB    -> SDB_BCB_CLEAN
 *      writeIOB = SDB_BCB_REDIRTY  -> SDB_BCB_DIRTY
 *                 플러셔에 의해 수행됨
 *
 *      makeFree = SDB_BCB_CLEAN -> SDB_BCB_FREE
 *      (이경우 hash x 래치 잡음=>hash에서 제거)
 *
 *  4.2 mPageID, mSpaceID
 *      SDB_BCB_FREE 상태의 BCB가 getVictim에 의해 SDB_BCB_CLEAN으로
 *      상태가 변경될때, mPageID와 mSpaceID가 변한다. 이때 BCB의 mMutex
 *      를 잡고 변경한다.
 *
 *  4.3 mFixCnt
 *      fix    =   hash chains (S or X) 래치  와 mMutex 잡고 변경, hash를 통해
 *                  접근하지 않고, fix를 할때는 mMutex만 잡아도 된다.
 *                  hash chains latch를 잡는 이유는 hash를 통해 접근하는 트랜잭션들
 *                  과의 동시성 제어 때문이다.
 *      unfix  =   mMutex만 잡고 변경( fix or unfix와의 동시성때문에)
 *
 ***********************************************************************/

/****************************************************************
 * Description :
 *  buffer pool의 prepare list로 부터 하나의 victim을 얻어온다.
 * 주의! 이함수 수행과정에서 hash x래치를 잡는다. 즉, 이 함수 수행
 * 전에 어떤 해시래치도 잡혀 있어선 안된다.
 *
 * aStatistics  - [IN]  통계정보
 * aListKey     - [IN]  prepare list는 여러개로 다중화 되어있다.
 *                     그중에서 하나를 선택해야 하는데, 이 파라미터
 *                     를 통해서 하나를 선택할 힌트를 얻는다.
 * aResidentBCB -[OUT] victim을 찾은 경우 BCB를 리턴, 못찾은 경우
 *                     NULL을 리턴, 리턴되는 BCB는 hash와 리스트에서
 *                     제거된 free상태의 BCB이다.
 ****************************************************************/
void sdbBufferPool::getVictimFromPrepareList( idvSQL  *aStatistics,
                                              UInt     aListKey,
                                              sdbBCB **aResidentBCB )
{
    sdbPrepareList           *sPrepareList;
    sdbBCB                   *sBCB = NULL;
    sdbHashChainsLatchHandle *sHashChainsHandle = NULL;
    sdsBCB                   *sSBCB;

    /* aListKey를 통해 하나의 prepareList를 선택.. 다른 prepare list에는
     * 접근을 하지 않는다.  */
    sPrepareList = &mPrepareList[getPrepareListIdx( aListKey )];

    while(1)
    {
        /* 일단 리스트에서 제거를 하면 getVictim을 수행하는 연산간의
         * 동시성은 보장된다.*/
        sBCB = sPrepareList->removeLast( aStatistics );

        if( sBCB == NULL )
        {
            break;
        }

        if( sBCB->mState == SDB_BCB_FREE )
        {
            /* BCB가 SDB_BCB_FREE 이고, 리스트에서도 제거되어 있다면,
             * 마음대로 victim으로 선정할 수 있다.*/
            break;
        }

        /* 아래의 조건 중 하나라도 만족하지 못한다면 victim으로 부적당 */
        /* fix이면서 isHot이 아닐 수 있다.  왜냐면, 실제 touchcount 를 증가시키기
         * 위해선 3초가 필요하다. */
        if( ( isFixedBCB( sBCB ) == ID_FALSE ) &&
            ( isHotBCB  ( sBCB ) == ID_FALSE ) &&
            ( sBCB->mState       == SDB_BCB_CLEAN))
        {
            /* prepareList에는 clean 이외의 여러 상태의 BCB가 있을수 있다.
             * clean상태이면서 hot이 아니고, fix되어있지 않은 BCB를 선택한다.
             *
             * 이때, 이조건 검사는 hash 래치를 잡고 해야 한다. 왜냐면,
             * 검사도중 다른 쓰레드에 의해 fix및 touch가 될 수 있고,
             * hash에서 제거 될 수도 있기 때문이다. hash 래치를 잡으면,
             * fix및 hash에서 제거가 되지 않음을 보장할 수 있다.
             *
             * 하지만 동시성을 높이기 위해서 hash 래치를 잡기전에
             * 먼저 검사를 해본다.
             * */

            /* 제대로된 검사를 위해 hash래치를 잡는다. */
            sHashChainsHandle = mHashTable.lockHashChainsXLatch(
                                                        aStatistics,
                                                        sBCB->mSpaceID,
                                                        sBCB->mPageID );


            if( sBCB->isFree() == ID_TRUE )
            {
                /* BCB 포인트를 통해서 make Free 접근하는 경우 밖에 없다.
                 * hash chains mutex를 잡기때문에, BCB의 mMutex를 잡을 필요 없다.*/
                mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                sHashChainsHandle = NULL;
                break;
            }
            else
            {
                /* Hot여부를  BCBMutex를 잡지 않고 보기 때문에 정확하지
                 * 않을 수 있어 hot을 victim으로 선정할 수 있으나,
                 * 문제를 일으키지는 않는다.*/
                if( ( isFixedBCB(sBCB) == ID_FALSE ) &&
                    ( isHotBCB(sBCB)   == ID_FALSE ) &&
                    ( sBCB->mState     == SDB_BCB_CLEAN ) )
                {
                    mHashTable.removeBCB( sBCB );

                    /* sBCB가 SDB_BCB_CLEAN 상태라면 반드시 hash에존재*/
                    /* 해시에서 제거되었기 때문에 상태를 free로 변경한다.*/
                    sBCB->lockBCBMutex( aStatistics );

                    if (( isFixedBCB(sBCB) == ID_TRUE ) ||
                        ( isHotBCB(sBCB)   == ID_TRUE ) ||
                        ( sBCB->mState     != SDB_BCB_CLEAN ) )
                    {
                        /* BUG-47945 cp list 디버깅 정보 보강 */
                        sBCB->dump();
                        IDE_ASSERT( 0 );
                    }

                    /* 연결된 SBCB가 있다면 delink */
                    sSBCB = sBCB->mSBCB;
                    if( sSBCB != NULL )
                    {
                        sSBCB->mBCB = NULL;
                        sBCB->mSBCB = NULL;    
                    }

                    sBCB->setToFree();
                    sBCB->unlockBCBMutex();

                    mStatistics.applyVictimPagesFromPrepare( aStatistics,
                                                             sBCB->mSpaceID,
                                                             sBCB->mPageID,
                                                             sBCB->mPageType );
                    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                    sHashChainsHandle = NULL;

                    break;
                }
                else
                {
                    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                    sHashChainsHandle = NULL;
                }
            }
        }
        mStatistics.applySkipPagesFromPrepare( aStatistics,
                                               sBCB->mSpaceID,
                                               sBCB->mPageID,
                                               sBCB->mPageType );

        /* 일단 prepare에 온 BCB들은 touchCnt개수와 상관없이 Hot이라고
         * 판단 하지 않는다.
         * 그렇기 때문에 무조건 LRU Mid에 삽입한다.
         * */
        mLRUList[ getLRUListIdx(aListKey)].insertBCB2BehindMid( aStatistics,
                                                                sBCB );

    }//while

    *aResidentBCB = sBCB;

    return;
}

/****************************************************************
 * Description :
 *  LRU List의 end부터 victim을 탐색하여, 조건에 맞는 BCB를 리턴한다.
 * 주의! 이 함수 수행과정에서 hash x래치를 잡는다. 즉, 이 함수 수행
 * 전에 어떤 해시 래치도 잡혀 있어선 안된다.
 *
 * aStatistics  - [IN]  통계정보
 * aListKey     - [IN]  LRU list는 여러개로 다중화 되어있다.
 *                      그중에서 하나를 선택해야 하는데, 이 파라미터
 *                      를 통해서 하나를 선택할 힌트를 얻는다.
 * aResidentBCB - [OUT] victim을 리턴. victim찾는것에 실패할 경우
 *                      NULL을 리턴할 수도 있다.
 ****************************************************************/
void sdbBufferPool::getVictimFromLRUList( idvSQL  *aStatistics,
                                          UInt     aListKey,
                                          sdbBCB **aResidentBCB )
{
    sdbLRUList               *sLRUList;
    sdbFlushList             *sFlushList;
    sdbBCB                   *sBCB = NULL;
    UInt                      sLoopCnt;
    sdbHashChainsLatchHandle *sHashChainsHandle = NULL;
    UInt                      sColdInsertedID   = ID_UINT_MAX;
    sdsBCB                   *sSBCB;

    /*여러 list들 중에서 aListKey에 따라 한개만 선택.. 여기에 대해서만
     *수행한다.*/
    sLRUList   = &mLRUList[ getLRUListIdx( aListKey ) ];
    sFlushList = &mFlushList[ getFlushListIdx( aListKey ) ];

    sLoopCnt = 0;
    while(1)
    {
        sLoopCnt++; //리스트 탐색횟수 증가
        if( sLoopCnt > mLRUSearchCnt )
        {
            sBCB = NULL;
            break;
        }

        /* 일단 리스트에서 제거를 하면 getVictim을 수행하는 연산간의
         * 동시성은 보장된다.*/
        sBCB = sLRUList->removeColdLast(aStatistics);

        if( sBCB == NULL )
        {
            break;
        }

        // 자신이 넣은 BCB가 다시 리턴되면 더이상 볼 필요없다.
        if (sBCB->mID == sColdInsertedID)
        {
            sLRUList->insertBCB2BehindMid(aStatistics, sBCB);
            sBCB = NULL;
            break;
        }

        mStatistics.applyVictimSearchs();


    retest:
        /* victim조건에 맞는지 여부를 검사 */
        // Hot인 경우 hot 영역에 삽입
        if( isHotBCB(sBCB) == ID_TRUE )
        {
            if( sHashChainsHandle != NULL )
            {
                mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                sHashChainsHandle = NULL;
            }
            sLRUList->addToHot(aStatistics, sBCB);
            mStatistics.applyVictimSearchsToHot();
            mStatistics.applySkipPagesFromLRU( aStatistics,
                                               sBCB->mSpaceID,
                                               sBCB->mPageID,
                                               sBCB->mPageType );
            continue;
        }
        // fix된 경우, 여전히 접근중이라고 판단하여 다시 cold first에 삽입
        if( isFixedBCB(sBCB) == ID_TRUE )
        {
            if( sHashChainsHandle != NULL)
            {
                mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                sHashChainsHandle = NULL;
            }
            sLRUList->insertBCB2BehindMid(aStatistics, sBCB);
            mStatistics.applyVictimSearchsToCold();
            sColdInsertedID = sBCB->mID;
            mStatistics.applySkipPagesFromLRU( aStatistics,
                                               sBCB->mSpaceID,
                                               sBCB->mPageID,
                                               sBCB->mPageType );
            continue;
        }

        if( sBCB->mState != SDB_BCB_CLEAN )
        {
            if( sHashChainsHandle != NULL )
            {
                mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                sHashChainsHandle = NULL;
            }

            if( sBCB->isFree() == ID_TRUE )
            {
                /* sBCB가 free상태인 경우엔 바로 사용이 가능하다.
                 * 왜냐면, free상태이고, 리스트를 통해 접근(리스트에서 삭제)
                 * 했으므로, 현재 BCB에 다른 연산이 없음을 보장할 수 있다. */
                break;
            }

            sFlushList->add( aStatistics, sBCB);
            mStatistics.applyVictimSearchsToFlush();
            mStatistics.applySkipPagesFromLRU( aStatistics,
                                               sBCB->mSpaceID,
                                               sBCB->mPageID,
                                               sBCB->mPageType );
            continue;
        }

        if( sHashChainsHandle == NULL)
        {
            /* sBCB의 상태는 hash 래치를 잡지 않는 이상 변할 수 있다.
             * 그렇기 때문에 제대로된 상태를 보고 제거하기 위해서 hash X 래치를 잡고
             * sBCB의 상태를 테스트 해야 한다.
             * 하지만 동시성을 위해 hash 래치를 잡지 않고 먼저 테스트를 해본다.
             */
            sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                                 sBCB->mSpaceID,
                                                                 sBCB->mPageID );
            if( sBCB->isFree() == ID_TRUE )
            {
                mHashTable.unlockHashChainsLatch( sHashChainsHandle );
                sHashChainsHandle = NULL;
                break;
            }

            goto retest;
        }

        mHashTable.removeBCB( sBCB );

        /* 해시에서 제거되었기 때문에 상태를 free로 변경한다.*/
        sBCB->lockBCBMutex( aStatistics );
        // sBCB를 hash에서 제거한 나만이 BCB의 상태를 free로 변경할 수 있다.
        IDE_DASSERT( sBCB->mState != SDB_BCB_FREE );

        if (( isFixedBCB(sBCB) == ID_TRUE ) ||
            ( isHotBCB(sBCB)   == ID_TRUE ) ||
            ( sBCB->mState     != SDB_BCB_CLEAN ))
        {
            /* BUG-47945 cp list 디버깅 정보 보강 */
            sBCB->dump();
            IDE_ASSERT( 0 );
        }

        /* 연결된 SBCB가 있다면 delink */
        sSBCB = sBCB->mSBCB;
        if( sSBCB != NULL )
        {
            sSBCB->mBCB = NULL;
            sBCB->mSBCB = NULL;
        }

        sBCB->setToFree();
        sBCB->unlockBCBMutex();

        mStatistics.applyVictimPagesFromLRU( aStatistics,
                                             sBCB->mSpaceID,
                                             sBCB->mPageID,
                                             sBCB->mPageType );
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;

        break;
    }// while

    *aResidentBCB = sBCB;

    return;
}

/****************************************************************
 * Description :
 * prepare list또는 LRU list에서 free상태의 BCB를 리턴한다.
 *
 *
 * 주의! 본 함수 수행중 hashChainsLatch 를 잡기 때문에 이러한 뮤텍스를
 * 해제한 다음 getVictim함수를 호출해야 한다.
 * hashChains x Latch를 잡는 경우는, SDB_BCB_CLEAN인 BCB를 Hash에서
 * 제거하기 위해서이다.
 * 이 함수 수행 중 victim을 찾지 못한경우 리턴하지 않고, 찾을때까지
 * 대기한다.
 * resource dead lock인경우에 에러를 리턴한다. (구현안되어 있음)
 *
 * aStatistics  - [IN]  통계정보
 * aListKey     - [IN]  LRU list는 여러개로 다중화 되어있다.
 *                      그중에서 하나를 선택해야 하는데, 이 파라미터
 *                      를 통해서 하나를 선택할 힌트를 얻는다.
 * aReturnedBCB -[OUT]  victim을 리턴. 절대 NULL을 리턴하지 않는다.
 ****************************************************************/
IDE_RC sdbBufferPool::getVictim( idvSQL  * aStatistics,
                                 UInt      aListKey,
                                 sdbBCB ** aReturnedBCB )
{
    UInt     sIdx;
    UInt     sTryCount = 0;
    idBool   sBCBAdded;
    sdbBCB  *sVictimBCB = NULL;

    IDE_DASSERT( aReturnedBCB != NULL );

    *aReturnedBCB = NULL;

    /* victim 선정 정책
     * 1. 먼저 prepareList를 하나 선정해 찾고, 없다면, LRUList를 하나 선정해서 찾는다.
     * 2. 1번 실패할 경우 prepareList를 1번과 다른 것으로 선정하고, LRUList역시 1번과
     *      다른것으로 설정한후 찾는다.
     * 3. 모든 prepareList에서 찾아 볼때가지 victim을 찾을 수 없다면,
     *      prepareList하나를 잡고 여기에 BCB가 생길때까지 대기한다.
     * 4. prepareList에 BCB가 생기던지, 아님 시간이 다되던지 깨어나면
     *      다시 그 prepareList에 victim탐색을 시도한다.
     * */
    while( sVictimBCB == NULL )
    {
        getVictimFromPrepareList( aStatistics, aListKey, &sVictimBCB );
        sTryCount++;

        if( sVictimBCB == NULL )
        {
            getVictimFromLRUList(aStatistics,
                                 aListKey,
                                 &sVictimBCB);

            if ( sVictimBCB == NULL )
            {
                if( ( sTryCount % mPrepareListCnt ) == 0 )
                {
                    // flusher들을 깨우고 자신은 prepare list에 대해 대기를 한다.
                    (void)sdbFlushMgr::wakeUpAllFlushers();

                    sIdx = getPrepareListIdx( aListKey );

                    (void)mPrepareList[ sIdx ].timedWaitUntilBCBAdded(
                                        aStatistics,
                                        smuProperty::getBufferVictimSearchInterval(),
                                        &sBCBAdded );

                    mStatistics.applyVictimWaits();

                    if( sBCBAdded == ID_TRUE )
                    {
                        // 해당 prepare list에 BCB가 추가되었기 때문에
                        // prepare list로부터 victim찾기를 한번 더 시도해본다.
                        getVictimFromPrepareList( aStatistics,
                                                  aListKey,
                                                  &sVictimBCB );
                    }

                    if( sVictimBCB != NULL )
                    {
                        mStatistics.applyPrepareAgainVictims();
                    }
                    else
                    {
                        // 주어진 시간동안 clean BCB를 받지 못했다.
                        // victim search warp가 발생한다.
                        // 즉, 다음 LRU-prepare list set으로 이동하여 wait한다.
                        mStatistics.applyVictimSearchWarps();
                    }
                }
            }
            else
            {
                mStatistics.applyLRUVictims();
            }
        }
        else
        {
            mStatistics.applyPrepareVictims();
        }

        aListKey++;
    }

    IDE_ASSERT( ( sVictimBCB != NULL) &&
                ( sVictimBCB->isFree() == ID_TRUE ) );

    *aReturnedBCB = sVictimBCB;

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *
 *  aSpaceID와 aPageID에 속하는 페이지를 버퍼상에 확보시킨다.
 *  이때, 내용은 중요하지 않다. 왜냐면 이 함수는 BCB의 mFrame를
 *  초기화 하기 전에 불리는 함수이기 때문이다. 즉, 어떤 내용이 있더라도
 *  이함수 수행후 초기화 된다. 이 때문에 이 함수는 무조건 페이지
 *  x래치를 잡은 상태에서 BCB를 리턴한다.
 *
 *  주의! 페이지 x래치를 잡은 상태에서 BCB를 리턴한다. 또한 fix시키기
 *  때문에 이함수 수행후 반드시 releasePage를 호출해야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  생성하길 원하는 space ID
 *  aPageID     - [IN]  생성하길 원하는 페이지
 *  aPageType   - [IN]  생성하길 원하는 페이지 type, sdpPageType
 *  aMtx        - [IN]  sdrMtx, createPage수행 중에 잡은 페이지 x 래치
 *                      정보 및 BCB정보를 기억하고 있는 미니트랜잭션.
 *  aPagePtr    - [OUT] create된 페이지를 리턴한다.
 ****************************************************************/
IDE_RC sdbBufferPool::createPage( idvSQL          *aStatistics,
                                  scSpaceID        aSpaceID,
                                  scPageID         aPageID,
                                  UInt             aPageType,
                                  void*            aMtx,
                                  UChar**          aPagePtr)
{
    UChar                    *sPagePtr;
    sdbBCB                   *sBCB = NULL;
    sdbBCB                   *sAlreadyExistBCB  = NULL;
    sdbHashChainsLatchHandle *sHashChainsHandle = NULL;
    sdrMtx                   *sMtx = NULL;
    smxTrans                 *sTrans = NULL;

    IDE_DASSERT( aSpaceID <= SC_MAX_SPACE_COUNT - 1);
    IDE_DASSERT( aMtx     != NULL );
    IDE_DASSERT( aPagePtr != NULL );

    sMtx = (sdrMtx*)aMtx;
    sTrans = (smxTrans*)sMtx->mTrans;

    /* 생성하기에 앞서 이미 hash에 존재하는지 알아본다. 만약
     * 존재한다면? 그것을 리턴하면 된다. */
    sHashChainsHandle = mHashTable.lockHashChainsSLatch( aStatistics,
                                                         aSpaceID,
                                                         aPageID );

    IDE_TEST( mHashTable.findBCB( aSpaceID,
                                  aPageID,
                                  (void**)&sAlreadyExistBCB )
              != IDE_SUCCESS );

    if ( sAlreadyExistBCB == NULL )
    {
        /* hash에 존재하지 않는다.
         * 이경우에는 victim을 얻어와야 한다.*/
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;

        // getVictim내에서 hash chains latch를 잡기 때문에,
        // 기존에 hash chains latch를 풀어준다.
        IDE_TEST( getVictim( aStatistics,
                             genListKey( (UInt)aPageID, aStatistics),
                             &sBCB )
                  != IDE_SUCCESS );

        /* hash에서 빠져 있기 때문에 fixCount를 갱신할 때,
         * hash chains latch를 잡지 않는다. */
        sBCB->lockBCBMutex( aStatistics );

        IDE_DASSERT( sBCB->mSBCB == NULL );

        /* BUG-45904 victim으로 받아오는 BCB는 fix count가 0이어야 한다. */
        if( sBCB->mFixCnt != 0 )
        {
            ideLog::log( IDE_ERR_0,"Invalid fixed BCB founded.");
            sdbBCB::dump( sBCB );

            if( sTrans != NULL )
            {
                sTrans->dumpTransInfo();
            }
            IDE_DASSERT( 0 );

            sBCB->mFixCnt = 0;
        }

        sBCB->incFixCnt();

        sBCB->mSpaceID     = aSpaceID;
        sBCB->mPageID      = aPageID;
        sBCB->mState       = SDB_BCB_CLEAN;
        sBCB->mPrevState   = SDB_BCB_CLEAN;

        sBCB->mReadyToRead = ID_TRUE;
        sBCB->updateTouchCnt();
        SM_LSN_INIT( sBCB->mRecoveryLSN );
        sBCB->unlockBCBMutex();


        /* 설정한 BCB를 hash에 삽입.. */
        sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                             aSpaceID,
                                                             aPageID);
        mHashTable.insertBCB( sBCB,
                              (void**)&sAlreadyExistBCB );

        if( sAlreadyExistBCB != NULL )
        {
            /* by proj-1704 MVCC Renewal ASSERT 제거
             *
             * 삽입실패.. 이미 누군가 먼저 getPage에서 fixPageInBuffer에서
             * victim을 얻어서 Hash에 삽입하였음. 즉, 동일한 페이지에
             * 대해서 fixPage와 createPage가 동시에 발생.
             * 서로 존재하는 것을 그대로 이용하면됨.
             * 하지만, 예외 이러한 예외상황은 TSS 페이지에 대해서만 허용함.
             */
            if ( aPageType == SDP_PAGE_TSS )
            {
                fixAndUnlockHashAndTouch( aStatistics,
                                          sAlreadyExistBCB,
                                          sHashChainsHandle );
                sHashChainsHandle = NULL;

                /*sBCB에 세팅한 정보 모두 해제 후 FREE상태로 만들고
                 * prepareList에 삽입 */
                sBCB->lockBCBMutex( aStatistics );
                sBCB->decFixCnt();

                sBCB->setToFree();
                sBCB->unlockBCBMutex();

                mPrepareList[getPrepareListIdx((UInt)aPageID)].addBCBList( aStatistics,
                                                                           sBCB,
                                                                           sBCB,
                                                                           1 );

                // 이 페이지에 접근하는 트랜잭션은 없지만
                // flusher가 이 페이지를 flush할 수는 있다.
                // 따라서 S-latch가 잡혀있는 상태일 수도 있기 때문에
                // try lock을 해서는 안되고 대기 모드로 latch를 획득해야 한다.
                sAlreadyExistBCB->lockPageXLatch( aStatistics );
                sBCB            = sAlreadyExistBCB;
                sBCB->mPageType = aPageType;

                IDE_ASSERT( smLayerCallback::isPageTSSType( sBCB->mFrame )
                            == ID_TRUE )
            }
            else
            {
                /* 누가 먼저 create 해 버렸다.  이런 경우는 있을 수 없다.
                 * 즉, 내가 create 하고 있는데, 동시에 같은 페이지에 대해
                 * create또는 getPage를 할 수 없다. */
                ideLog::log( SM_TRC_LOG_LEVEL_BUFFER,
                             SM_TRC_BUFFER_POOL_CONFLICT_CREATEPAGE,
                             aPageType );
                IDE_ASSERT(0);
            }
        }
        else
        {
            /* hash에 삽입 성공..
             * sBCB를 LRU List에 삽입한다. */
            sBCB->lockPageXLatch(aStatistics);
            /* 페이지 x래치를 잡은후에 mPageType변경 */
            sBCB->mPageType = aPageType;
            IDV_TIME_GET(&sBCB->mCreateOrReadTime);

            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;

            mLRUList[ getLRUListIdx((UInt)aPageID)].insertBCB2BehindMid( aStatistics, sBCB );

            //BCBPtr이 변화된것이 아니므로, 현재 BCB와 같아야 한다.
            sPagePtr = sBCB->mFrame;
            if( ((sdbFrameHdr*)sPagePtr)->mBCBPtr != sBCB )
            {
                /* BUG-32528 disk page header의 BCB Pointer 가 긁혔을 경우에 대한
                 * 디버깅 정보 추가. */
                ideLog::log( IDE_DUMP_0,
                             "mFrame->mBCBPtr( %"ID_xPOINTER_FMT" ) != sBCB( %"ID_xPOINTER_FMT" )\n",
                             ((sdbFrameHdr*)sPagePtr)->mBCBPtr,
                             sBCB );

                smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                                sPagePtr - SD_PAGE_SIZE,
                                                "[ Prev Page ]" );
                smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                                sPagePtr,
                                                "[ Curr Page ]" );
                smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                                sPagePtr + SD_PAGE_SIZE,
                                                "[ Next Page ]" );

                sdbBCB::dump( sBCB );

                //BUG-48422: BCB의 페이지 프레임이 가지고 있는 BCB 포인터 값을 바꿔준다.
                sdbBCB::setBCBPtrOnFrame( ((sdbFrameHdr*)sPagePtr), sBCB );
            }

            sdbBCB::setSpaceIDOnFrame( (sdbFrameHdr*)sBCB->mFrame, aSpaceID );
        }
    }
    else
    {
        if ( aPageType == SDP_PAGE_TSS )
        {
            /* BUG-37702 DelayedStamping fixes an TSS page that will reuse
             * 재사용 가능한 TSS page는 DelayedStamping(sdcTSSegment::getCommitSCN)에 의해
             * 이미 commit된 Tx의 TSSlot을 참조 할 수 있습니다.
             * 따라서 createPage중 버퍼에 이미 존재할 수 있고 fix되어있을 수 있습니다. */
        }
        else
        {
            // create 하려고 하는 BCB가 버퍼에 존재 할 수는 있으나,
            // 이것은 이미 다른 쓰레드로 부터 접근이 끝난 이후다.
            if ( isFixedBCB( sAlreadyExistBCB ) == ID_TRUE )
            {
                ideLog::log( IDE_ERR_0,"Invalid fixed BCB detected.");
                sdbBCB::dump(sAlreadyExistBCB);
                
                if( sTrans != NULL )
                {
                    sTrans->dumpTransInfo();
                }

                IDE_ERROR( 0 );
            }
        }

        /*이미 hash에 BCB가 존재한다.*/
        fixAndUnlockHashAndTouch( aStatistics, sAlreadyExistBCB, sHashChainsHandle );
        sHashChainsHandle = NULL;

        // 이 페이지에 접근하는 트랜잭션은 없지만
        // flusher가 이 페이지를 flush할 수는 있다.
        // 따라서 S-latch가 잡혀있는 상태일 수도 있기 때문에
        // try lock을 해서는 안되고 대기 모드로 latch를 획득해야 한다.
        sAlreadyExistBCB->lockPageXLatch( aStatistics );

        sBCB = sAlreadyExistBCB;
        sBCB->mPageType = aPageType;
    }

    mStatistics.applyCreatePages( aStatistics,
                                  aSpaceID,
                                  aPageID,
                                  aPageType );

    IDE_ASSERT( smLayerCallback::pushToMtx( aMtx,
                                            sBCB,
                                            SDB_X_LATCH )
                == IDE_SUCCESS );

    *aPagePtr = sBCB->mFrame;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sHashChainsHandle != NULL )
    {
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;
    }
    else 
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  aBCB가 가리키는 mSpaceID와 mPageID를 실제 디스크에서 mFrame으로
 *  읽어온다.
 *  주의 !
 *  이 함수 수행중 BCB의 mFrame에는 아무도 접근하지 못하게 해야한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  여기에 설정된 mPageID와 mSpaceID의 페이지를 mFrame
 *                      으로 읽어온다.
 *  aIsCorruptPage
 *              - [OUT] page의 corrupt유무를 반환한다.
 ****************************************************************/
IDE_RC sdbBufferPool::readPageFromDisk( idvSQL                 *aStatistics,
                                        sdbBCB                 *aBCB,
                                        idBool                 *aIsCorruptPage )
{
    UInt            sState = 0;
    idvTime         sBeginTime;
    idvTime         sEndTime;
    ULong           sReadTime;
    ULong           sCalcChecksumTime;

    // 반드시 fix되어 있어야 한다. 그래야 load 중간에 버퍼에서 삭제되지 않는다.
    IDE_DASSERT( isFixedBCB(aBCB) == ID_TRUE );
    IDE_DASSERT( aBCB->mReadyToRead == ID_FALSE );

    if( aIsCorruptPage != NULL )
    {
        *aIsCorruptPage = ID_FALSE;
    }

    IDV_TIME_GET(&sBeginTime);

    mStatistics.applyBeforeSingleReadPage( aStatistics );
    sState = 1;

    IDE_TEST( sddDiskMgr::read( aStatistics,
                                aBCB->mSpaceID,
                                aBCB->mPageID,
                                aBCB->mFrame )
              != IDE_SUCCESS );

    sState = 0;
    mStatistics.applyAfterSingleReadPage( aStatistics );

    IDV_TIME_GET(&sEndTime);
    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
    sBeginTime = sEndTime;

    if ( smLayerCallback::checkAndSetPageCorrupted( aBCB->mSpaceID,
                                                    aBCB->mFrame ) == ID_TRUE )
    {
        if( aIsCorruptPage != NULL )
        {
            *aIsCorruptPage = ID_TRUE;
        }

        if ( smLayerCallback::getCorruptPageReadPolicy()
              != SDB_CORRUPTED_PAGE_READ_PASS )
        {
            IDE_RAISE( page_corruption_error );
        }
    }

    setFrameInfoAfterReadPage( aBCB,
                               ID_TRUE );  // check to online tablespace

    mStatistics.applyReadPages( aStatistics ,
                                aBCB->mSpaceID,
                                aBCB->mPageID,
                                aBCB->mPageType );

    IDV_TIME_GET(&sEndTime);
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * 일반 ReadPage시 걸린 시간 저장함.
     * 단 Checksum은 실제 Checksum 외 여러가지가 포함되니 주의 */
    mStatistics.applyReadByNormal( sCalcChecksumTime,
                                   sReadTime,
                                   1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( page_corruption_error );
    {
        /* BUG-45598: BCB까지 셋팅 했지만 Corrupt 페이지 인 경우 false 리턴.
         * 만약 aIsCorruptPage가 NULL이 아닌 경우라면 이 함수가 redo 과정의
         * sdrRedoMgr::applyHashedLogRec에서 불린 경우다. 이 경우 BCB 정보를
         * 세팅해주어야 한다. Corrupt 페이지에 대한 처리가 상위 함수에서 
         * 수행되기 때문이다.
         */
        if ( aIsCorruptPage != NULL )
        {
            setFrameInfoAfterReadPage( aBCB,
                                       ID_TRUE );  // check to online tablespace
        }

        sdpPhyPage::tracePage( IDE_DUMP_0,
                               aBCB->mFrame,
                               "[ Corruption Page ]");

        switch ( smLayerCallback::getCorruptPageReadPolicy() )
        {
            case SDB_CORRUPTED_PAGE_READ_FATAL :
                IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                          aBCB->mSpaceID,
                                          aBCB->mPageID ));

                break;
            case SDB_CORRUPTED_PAGE_READ_ABORT :
                // PROJ-1665 : ABORT Error를 반환함
                //리뷰: 지워버리고, mFrame을 받는 쪽에서 이걸 호출해서 써라.
                IDE_SET( ideSetErrorCode( smERR_ABORT_PageCorrupted,
                                          aBCB->mSpaceID,
                                          aBCB->mPageID));
                break;
            default:
                break;
        }
    }

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        mStatistics.applyAfterSingleReadPage( aStatistics );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Descrition:
 *  디스크에서 페이지를 읽어온 후, BCB와 읽어온 frame에 여러가지 정보를 설정한다
 *
 *  aBCB            - [IN] BCB
 *  aChkOnlineTBS   - [IN] TBS Online 여부를 확인해서 SMO NO 갱신 (Yes/No)
 **********************************************************************/
void sdbBufferPool::setFrameInfoAfterReadPage( sdbBCB * aBCB,
                                               idBool   aChkOnlineTBS )
{
    aBCB->mPageType = smLayerCallback::getPhyPageType( aBCB->mFrame );
    IDV_TIME_GET(&aBCB->mCreateOrReadTime);

    /* disk에서 페이지를 읽은 후 반드시 세팅해줘야 하는 정보들 */
    sdbBCB::setBCBPtrOnFrame( (sdbFrameHdr*)aBCB->mFrame, aBCB );
    sdbBCB::setSpaceIDOnFrame( (sdbFrameHdr*)aBCB->mFrame, aBCB->mSpaceID );

    // BUG-47429 SMO NO가 0이 아닌 경우에만 SMO NO 초기화 필요 여부를 확인한다.
    if ( smLayerCallback::getIndexSMONo( aBCB->mFrame ) != 0 )
    {
        // SMO no를 초기화해야 한다.
        smLayerCallback::resetIndexSMONo( aBCB->mFrame,
                                          aBCB->mSpaceID,
                                          aChkOnlineTBS );
    }
}

/****************************************************************
 * Description :
 *  요청한 페이지를 디스크로 부터 읽어오는 함수. 읽어오는것 뿐만 아니라,
 *  fix 및 touch 하고, hash와 LRU리스트에 삽입한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 *  aReadMode   - [IN]  page read mode(SPR or MPR)
 *  aHit        - [OUT] Hit되었는지 여부 리턴
 *  aReplacedBCB- [OUT] replace된 BCB를 리턴한다.
 *  aIsCorruptPage
 *              - [OUT] page의 Corrupt 유무를 반환한다.
 *
 ****************************************************************/
IDE_RC sdbBufferPool::readPage( idvSQL                   *aStatistics,
                                scSpaceID                 aSpaceID,
                                scPageID                  aPageID,
                                sdbPageReadMode           aReadMode,
                                idBool                   *aHit,
                                sdbBCB                  **aReplacedBCB,
                                idBool                   *aIsCorruptPage )
{
    sdbBCB                     *sBCB              = NULL;
    sdbBCB                     *sAlreadyExistBCB  = NULL;
    sdbHashChainsLatchHandle   *sHashChainsHandle = NULL;
    /* PROJ-2102 Secondary Buffer Control Block  */
    sdsBCB  * sSBCB          = NULL;
    idBool    sIsCorruptRead = ID_FALSE;
         
    IDE_TEST( getVictim( aStatistics,
                         genListKey( (UInt)aPageID, aStatistics ),
                         &sBCB )
              != IDE_SUCCESS );

    IDE_DASSERT( ( sBCB != NULL ) && ( sBCB->isFree() == ID_TRUE ) );

    /* 현재 sBCB가 Free이기 때문에 FixCount 갱신시에 hash chains latch를 잡지
     * 않는다. */
    sBCB->lockBCBMutex( aStatistics );

    IDE_DASSERT( sBCB->mSBCB == NULL );
    IDE_DASSERT( sBCB->mFixCnt == 0 );

    sBCB->incFixCnt();
    sBCB->mSpaceID   = aSpaceID;
    sBCB->mPageID    = aPageID;
    sBCB->mState     = SDB_BCB_CLEAN;
    sBCB->mPrevState = SDB_BCB_CLEAN;

    SM_LSN_INIT( sBCB->mRecoveryLSN );

    /* BUG-24092: [SD] BufferMgr에서 BCB의 Latch Stat을 갱신시 Page Type이
     * Invalid하여 서버가 죽습니다.
     *
     * 타 Transaction이 mPageType을 읽을 수 있기때문에서 여기서 초기화합니다. */
    sBCB->mPageType = SDB_NULL_PAGE_TYPE;

    if ( aReadMode == SDB_SINGLE_PAGE_READ )
    {
        /* MPR의 경우 touch count를 증가시키지 않는다.
         * 왜나하면 sdbBufferPool::cleanUpKey()에서
         * touch count가 0보다 크면 BCB를 LRU list에
         * 넣어버기리 때문이다. */
        sBCB->updateTouchCnt();
    }
    sBCB->unlockBCBMutex();

    // free 상태의 BCB는 모두 mReadyToRead가 ID_FALSE 이다.
    // 왜냐면, free 상태의 Frame을 읽어서 뭐하겠는가?
    IDE_ASSERT( sBCB->mReadyToRead == ID_FALSE );
    // 해시에 달기 전까지는 페이지 x래치와 mReadIOMutex를 잡기를 시도 하지 못한다.
    /* 디스크에서 데이터를 읽어 올 것이므로 페이지 x 래치를 잡는다.
     * 보통 트랜잭션은 페이지 x 래치를 잡지 못할 것이므로 hash를 통해
     * fix를 하였다 하더라도 실제 읽지는 못한다. */
    /* BUG-21836 버퍼매니저에서 flusher와 readPage간의 동시성 문제로 assert
     * 에서 죽을 수 있습니다.
     * sdbFlusher::flushForCheckpoint와 sdbFlusher::flushDBObjectBCB
     * 에서 sBCB에 slatch를 잡을 수 있습니다. 이경우엔 그들이 래치를 풀기를
     * 여기서 대기합니다.*/
    sBCB->lockPageXLatch( aStatistics );

    /* no latch로 읽는 트랜잭션도 읽지 못하게 만든다.
     * 즉, no latch로 읽는 트랜잭션일지라도 mReadIOMutex가 잡혀있다면,
     * 여기서 대기를 하게 된다. */
    sBCB->mReadIOMutex.lock( aStatistics );

    /* 먼저 hash에 삽입부터 한다. 이후 트랜잭션들은 hit할 수 있으나, (fix count증가 가능)
     * 페이지 x 래치와 mReadIOMutex 때문에 실제 데이터를 읽지는 못한다.*/
    sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                         aSpaceID,
                                                         aPageID );

    /* 먼저 Hash Table에 삽입을 한 이후에 디스크에서 읽어온다.
     * 왜냐면, 먼저 디스크에서 읽어온 후에 삽입하게 되면, 자신이 읽어온 내용을
     * 다른 트랜잭션도 동시에 읽을 수 있기 때문에, IO 비용에서 손해를 본다.
     * 그렇다고, 자신이 읽는다고해서 다른 트랜잭션이 읽지 못하게 계속
     * Hash table 에 래치를 걸고 있을 수도 없는 노릇이다.*/
    mHashTable.insertBCB( sBCB,
                          (void**)&sAlreadyExistBCB );

    if( sAlreadyExistBCB != NULL )
    {
        /* 삽입실패.. 이미 누군가 먼저 삽입, 존재하는 것을 그대로 이용,
         * 디스크에서 읽어올 필요가 없다. */
        fixAndUnlockHashAndTouch( aStatistics, sAlreadyExistBCB, sHashChainsHandle );
        sHashChainsHandle = NULL;
        /*sBCB에 세팅한 정보 모두 해제 후 FREE상태로 만들고 prepareList에 삽입 */
        sBCB->mReadIOMutex.unlock();
        sBCB->unlockPageLatch();

        sBCB->lockBCBMutex( aStatistics );
        sBCB->decFixCnt();
        sBCB->setToFree();
        sBCB->unlockBCBMutex();

        mPrepareList[getPrepareListIdx((UInt)aPageID)].addBCBList( aStatistics,
                                                                   sBCB,
                                                                   sBCB,
                                                                   1 );

        *aReplacedBCB = sAlreadyExistBCB;
        *aHit = ID_TRUE;
    }
    else
    {
        /*삽입 성공.. 이제 디스크에서 실제 데이터를 버퍼로 읽어와야 한다.*/
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;

        if ( isSBufferServiceable() == ID_TRUE )
        {
            sdsBufferMgr::applyGetPages();

            IDE_TEST_RAISE( sdsBufferMgr::findBCB( aStatistics, 
                                                   aSpaceID,
                                                   aPageID,
                                                   &sSBCB )
                            != IDE_SUCCESS,
                            ERROR_READ_PAGE_FAULT );
        }

        if ( sSBCB != NULL )
        {
            IDE_TEST_RAISE( sdsBufferMgr::moveUpPage( aStatistics,
                                                      &sSBCB, 
                                                      sBCB,
                                                      &sIsCorruptRead,
                                                      aIsCorruptPage )
                             != IDE_SUCCESS,
                             ERROR_READ_PAGE_FAULT);
            
            if ( sIsCorruptRead == ID_FALSE )
            { 
                SM_GET_LSN( sBCB->mRecoveryLSN, sSBCB->mRecoveryLSN );
                /* 관련 sBCB link */
                sBCB->mSBCB = sSBCB;

                if( sSBCB->mState == SDS_SBCB_DIRTY )
                {
                    setDirtyBCB( aStatistics, sBCB );
                }
                else
                {
                    /* state 는 clean, iniob 일수 있다.
                       이경우는 clean으로 처리 */
                    /* nothing to do */
                }
                IDE_DASSERT( validate(sBCB) == IDE_SUCCESS );
            }
            else 
            {
                IDE_TEST_RAISE( readPageFromDisk( aStatistics,
                                                  sBCB,
                                                  aIsCorruptPage ) != IDE_SUCCESS,
                                ERROR_READ_PAGE_FAULT);
            }
        } 
        else       
        { 
            IDE_TEST_RAISE( readPageFromDisk( aStatistics,
                                              sBCB,
                                              aIsCorruptPage ) != IDE_SUCCESS,
                            ERROR_READ_PAGE_FAULT);
        }

        /* BUG-28423 - [SM] sdbBufferPool::latchPage에서 ASSERT로 비정상
         * 종료합니다. */
        IDU_FIT_POINT( "1.BUG-28423@sdbBufferPool::readPage" );

        /*I/O를 위해 걸어두었던 모든 래치를 푼다.*/
        sBCB->mReadyToRead = ID_TRUE;
        sBCB->mReadIOMutex.unlock();
        sBCB->unlockPageLatch();

        mLRUList[getLRUListIdx((UInt)aPageID)].insertBCB2BehindMid( aStatistics,
                                                                    sBCB);
        *aReplacedBCB = sBCB;
        *aHit = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERROR_READ_PAGE_FAULT)
    {
        IDE_ASSERT( sBCB != NULL );

    retry:
        sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                             aSpaceID,
                                                             aPageID );
        if ( sBCB->mFixCnt != 1 )
        {
            /*내가 fix하고 있으므로 반드시 1이상이다.*/
            IDE_ASSERT( sBCB->mFixCnt != 0);
            /* 나 이외의 누군가 fix하고 있다. 이것은 no latch로 접근하는,
             * 즉, fixPage연산에 의해 존재 할 수 있다. 이경우 에러임을 설정하고,
             * 그들이 unfix할때 까지 기다린다. */
            sBCB->mPageReadError = ID_TRUE;
            sBCB->mReadIOMutex.unlock();
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
            /* BUG-21576 page corruption에러 처리중, 버퍼매니저에서 deadlock
             * 가능성 있음.
             * */
            sBCB->unlockPageLatch();

            idlOS::thr_yield();
            sBCB->lockPageXLatch( aStatistics );
            sBCB->mReadIOMutex.lock( aStatistics );
            goto retry;
        }
        else
        {
            /* 해시에 x래치를 잡고 있기 때문에 mFixCnt가 변하지 않는다. */
            IDE_ASSERT( sBCB->mFixCnt == 1 );
            mHashTable.removeBCB( sBCB );
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
        }

        sBCB->mReadIOMutex.unlock();

        sBCB->unlockPageLatch();

        sBCB->lockBCBMutex( aStatistics );
        sBCB->decFixCnt();
        sBCB->setToFree();
        sBCB->unlockBCBMutex();

        mPrepareList[ getPrepareListIdx((UInt)aPageID)].addBCBList( aStatistics,
                                                                    sBCB, 
                                                                    sBCB, 
                                                                    1 );
    }
    //fix BUG-29682 The position of IDE_EXCEPTION_END is wrong.
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  aSpaceID, aPageID에 속하는 페이지를 버퍼에 fix시키고, 래치 모드에
 *  맞는 래치를 걸어서 리턴해준다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 *  aLatchMode  - [IN]  fix된 버퍼 페이지에 대해 걸을 페이지 래치 종류
 *  aWaitMode   - [IN]  페이지 래치를 걸때, try할지 말지 여부
 *  aReadMode   - [IN]  page read mode(SPR or MPR)
 *  aMtx        - [IN]  해당 미니트랜잭션
 *  aRetPage    - [OUT] fix되고 페이지 래치 걸린 페이지의 포인터
 *  aTrySuccess - [OUT] 래치를 성공적으로 걸었는지 여부. 래치를 걸지 못했다면
 *                      페이지를 fix시키지도 않는다.
 ****************************************************************/
IDE_RC sdbBufferPool::getPage( idvSQL               * aStatistics,
                               scSpaceID              aSpaceID,
                               scPageID               aPageID,
                               sdbLatchMode           aLatchMode,
                               sdbWaitMode            aWaitMode,
                               sdbPageReadMode        aReadMode,
                               void                 * aMtx,
                               UChar               ** aRetPage,
                               idBool               * aTrySuccess,
                               idBool               * aIsCorruptPage )
{
    sdbBCB *sBCB;
    idBool  sLatchSuccess = ID_FALSE;
    idBool  sHit          = ID_FALSE;

retry:
    /* 먼저 fixPageInBuffer를 호출하여 버퍼에 올려놓고 unFixPage하기 전까지
     * 버퍼에서 삭제되지 않도록 한다. fixPageInBuffer를 호출한 이후라면,
     * 페이지가 버퍼에 적재되어 있음을 보장할 수 있다.
     * 아래 함수 수행후 반드시 unfixPage를 해야 한다.*/
    IDE_TEST( fixPageInBuffer( aStatistics,
                               aSpaceID,
                               aPageID,
                               aReadMode,
                               aRetPage,
                               &sHit,
                               &sBCB,
                               aIsCorruptPage )
              != IDE_SUCCESS );

    latchPage( aStatistics,
               sBCB,
               aLatchMode,
               aWaitMode,
               &sLatchSuccess );

    // BUG-21576 page corruption 에러 처리중, 버퍼매니저에서 deadlock 가능성있음
    if( sBCB->mPageReadError == ID_TRUE )
    {
        /* BUG-45598: CorruptPageErrPolicy가 FATAL일 때만 서버가 Corrupt페이지
         * 만나서 사망하도록 재시도 함.
         */
        if ( smLayerCallback::getCorruptPageReadPolicy()
             == SDB_CORRUPTED_PAGE_READ_FATAL )
        {
            /* 페이지 에러가 난경우 */
            /* 자신이 디스크에서 읽어와서 실제 에러를 만나서 튕길때 까지
             * 계속 수행한다. 왜냐면, 무슨 에러가 발생했는지는 실제 에러를
             * 만날때 까지는 모르기 때문이다. */
            if( sLatchSuccess == ID_TRUE )
            {
                releasePage( aStatistics, sBCB, aLatchMode );
            }
            else
            {
                unfixPage(aStatistics, sBCB);
            }
            goto retry;
        }
        else
        {
            /* do nothing */
        }
    }

    if( sLatchSuccess == ID_TRUE )
    {
        mStatistics.applyGetPages(aStatistics,
                                  sBCB->mSpaceID,
                                  sBCB->mPageID,
                                  sBCB->mPageType);
        if(aMtx != NULL)
        {
            IDE_ASSERT( smLayerCallback::pushToMtx( aMtx,
                                                    sBCB,
                                                    aLatchMode )
                        == IDE_SUCCESS );
        }
        //BUG-22042 [valgrind]sdbBufferPool::fixPageInBuffer에 UMR이 있습니다.
        if( sHit == ID_TRUE )
        {
            mStatistics.applyHits(aStatistics,
                                  sBCB->mPageType,
                                  sBCB->mBCBListType );
        }
    }
    else
    {
        //래치가 실패하였기 때문에 fix도 해제한다.
        unfixPage(aStatistics, sBCB );
    }

    if( aTrySuccess != NULL )
    {
        *aTrySuccess = sLatchSuccess;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 * 단지 page fix만 한다.
 * mtx를 사용하지 않으며, 페이지에 대한 latch를 잡지도 않는다.
 * getPage의 no-latch 모드와 같다.
 * 이 페이지를 해제하는 것은 이 함수를 사용하는 쪽의 책임이며,
 * 이 함수 호출후 반드시 unfixPage가 호출되어야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 *  aRetPage    - [OUT] fix된 페이지를 리턴한다. 리턴되는 페이지에는
 *                      전혀 페이지 래치가 잡혀 있지 않다.
 *
 ****************************************************************/
IDE_RC sdbBufferPool::fixPage( idvSQL              *aStatistics,
                               scSpaceID            aSpaceID,
                               scPageID             aPageID,
                               UChar              **aRetPage)

{
    sdbBCB *sBCB       = NULL;
    idBool  sHit       = ID_FALSE;

 retry:
    /* 먼저 fixPageInBuffer를 호출하여 버퍼에 올려놓고 unFixPage하기 전까지
     * 버퍼에서 삭제되지 않도록 한다. fixPageInBuffer를 호출한 이후라면,
     * 페이지가 버퍼에 적재되어 있음을 보장할 수 있다.*/
    IDE_TEST( fixPageInBuffer( aStatistics,
                               aSpaceID,
                               aPageID,
                               SDB_SINGLE_PAGE_READ,
                               aRetPage,
                               &sHit,
                               &sBCB,
                               NULL/*IsCorruptPage*/ )
              != IDE_SUCCESS );

    // fix page는 nolatch로 접근하지만, 현재 디스크에서 읽는 중인
    // 페이지를 접근하지 않게 해야한다.
    latchPage(aStatistics,
              sBCB,
              SDB_NO_LATCH,
              SDB_WAIT_NORMAL,
              NULL);  // lock 성공적으로 잡았는지 여부

    if( sBCB->mPageReadError == ID_TRUE )
    {
        if ( smLayerCallback::getCorruptPageReadPolicy()
             == SDB_CORRUPTED_PAGE_READ_FATAL )
        {
            /* 페이지 에러가 난경우 */
            /* 자신이 디스크에서 읽어와서 실제 에러를 만나서 튕길때 까지
             * 계속 수행한다. 왜냐면, 무슨 에러가 발생했는지는 실제 에러를
             * 만날때 까지는 모르기 때문이다. */
            unfixPage(aStatistics, sBCB );
            goto retry;
        }
        else
        {
            /* do nothing */
        }   
    }

    if( sHit == ID_TRUE )
    {
        //BUG-22042 [valgrind]sdbBufferPool::fixPageInBuffer에 UMR이 있습니다.
        mStatistics.applyHits(aStatistics,
                              sBCB->mPageType,
                              sBCB->mBCBListType );
    }
    mStatistics.applyFixPages(aStatistics,
                              sBCB->mSpaceID,
                              sBCB->mPageID,
                              sBCB->mPageType);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 ****************************************************************/
IDE_RC sdbBufferPool::fixPageWithoutIO( idvSQL              *aStatistics,
                                        scSpaceID            aSpaceID,
                                        scPageID             aPageID,
                                        UChar              **aRetPage)
{
    sdbBCB                   * sBCB = NULL;
    sdbHashChainsLatchHandle * sHashChainsHandle = NULL;

    /* hash에 있다면 그냥 fix후에 리턴하고, 그렇지 않다면 replace한다.*/
    sHashChainsHandle = mHashTable.lockHashChainsSLatch( aStatistics,
                                                         aSpaceID,
                                                         aPageID );

    IDE_TEST( mHashTable.findBCB( aSpaceID, aPageID, (void**)&sBCB )
              != IDE_SUCCESS );

    /* To Fix BUG-23380 [SD] sdbBufferPool::fixPageWithoutIO에서
     * 디스크로부터 readpage가 완료되지 않은 BCB에 대해서는 fix 하지 않는다.
     * 왜냐하면, 본 함수의 본래 목적상 대기할 이유가 없다. */
    if ( ( sBCB != NULL ) && ( sBCB->mReadyToRead == ID_TRUE ) )
    {
        fixAndUnlockHashAndTouch( aStatistics,
                                  sBCB,
                                  sHashChainsHandle );
        sHashChainsHandle = NULL;

        IDE_DASSERT( sBCB->mReadyToRead == ID_TRUE );

        mStatistics.applyHits( aStatistics,
                               sBCB->mPageType,
                               sBCB->mBCBListType );

        mStatistics.applyFixPages( aStatistics,
                                   sBCB->mSpaceID,
                                   sBCB->mPageID,
                                   sBCB->mPageType );
        *aRetPage = sBCB->mFrame;

    }
    else
    {
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle =  NULL;
        *aRetPage = NULL;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sHashChainsHandle != NULL )
    {
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );  
        sHashChainsHandle = NULL;
    }
    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 ****************************************************************/
void sdbBufferPool::tryEscalateLatchPage( idvSQL            *aStatistics,
                                          sdbBCB            *aBCB,
                                          sdbPageReadMode    aReadMode,
                                          idBool            *aTrySuccess )
{
    iduLatchMode sLatchMode;

    IDE_DASSERT( isFixedBCB(aBCB) == ID_TRUE );

    sLatchMode = aBCB->mPageLatch.getLatchMode( );

    if( sLatchMode == IDU_LATCH_READ )
    {
        aBCB->lockBCBMutex(aStatistics);

        IDE_ASSERT( (aReadMode == SDB_SINGLE_PAGE_READ) ||
                    (aReadMode == SDB_MULTI_PAGE_READ) );

        if( aReadMode == SDB_SINGLE_PAGE_READ )
        {
            if( aBCB->mFixCnt > 1 )
            {
                *aTrySuccess = ID_FALSE;
                aBCB->unlockBCBMutex();
                IDE_CONT( skip_escalate_lock );
            }
            IDE_ASSERT( aBCB->mFixCnt == 1 );
        }

        if( aReadMode == SDB_MULTI_PAGE_READ )
        {
            /* MPR을 사용하는 경우, fixCount를 두번 증가시키고 두번 감소시킨다.
             * 1. sdbMPRMgr::getNxtPageID...()시에 fixcount 증가시킴
             * 2. sdbBufferMgr::getPage()시에 fixcount 증가시킴
             * 3. sdbBufferMgr::releasePage()시에 fixcount 감소시킴
             * 4. sdbMPRMgr::destroy()시에 fixcount 감소시킴 */
            /* 그래서 fixCnt가 2인 경우에도 lock escalation을 시도해본다. */
            if( aBCB->mFixCnt > 2 )
            {
                *aTrySuccess = ID_FALSE;
                aBCB->unlockBCBMutex();
                IDE_CONT( skip_escalate_lock );
            }
            IDE_ASSERT( aBCB->mFixCnt == 2 );
        }

        // To Fix BUG-23313
        // Fix Count가 1이라도 다른 Thread가 Page에 Latch를
        // 획득하는 경우가 있는데 Flusher가 Page를 Flush할때
        // 이다. 이를 고려하여 try로 X-Latch를 획득해보고
        // 실패하면, 다시 Page에 S-Latch를 획득한다.
        aBCB->unlockPageLatch();
        aBCB->tryLockPageXLatch( aTrySuccess );

        if ( *aTrySuccess == ID_FALSE )
        {
            aBCB->lockPageSLatch( aStatistics );
        }
        
        aBCB->unlockBCBMutex();
    }
    else
    {
        *aTrySuccess = ID_TRUE;
    }

    IDE_EXCEPTION_CONT( skip_escalate_lock );
}

/****************************************************************
 * Description :
 *  getPage한 BCB는 페이지 래치가 걸려있고 fix되어 있다.  이것을 해제
 *  시켜주는 함수
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  getPage를 수행했던 BCB
 *  aLatchMode  - [IN]  getPage를 수행했을때의 latch mode
 ****************************************************************/
void sdbBufferPool::releasePage( idvSQL          *aStatistics,
                                 void            *aBCB,
                                 UInt             aLatchMode )
{
    if((aLatchMode == (UInt)SDB_S_LATCH) ||
       (aLatchMode == (UInt)SDB_X_LATCH))
    {
        ((sdbBCB*)aBCB)->unlockPageLatch();
    }

    unfixPage(aStatistics, (sdbBCB*)aBCB);
}

void sdbBufferPool::releasePage( idvSQL          *aStatistics,
                                 void            *aBCB )
{
    ((sdbBCB*)aBCB)->unlockPageLatch();
    unfixPage(aStatistics, (sdbBCB*)aBCB);
}

/****************************************************************
 * Description :
 *  aBCB가 가리키는 frame에 PageLSN을 설정한다.
 *
 *  aBCB        - [IN]  BCB
 *  aMtx        - [IN]  Mini transaction
 ****************************************************************/
void sdbBufferPool::setPageLSN( sdbBCB   *aBCB,
                                void     *aMtx )
{
    smLSN        sEndLSN;
    smLSN        sPageLSN;
    idBool       sIsLogging;
    static UInt  sLogFileSize = smuProperty::getLogFileSize();

    IDE_DASSERT( aBCB != NULL );
    IDE_DASSERT( aMtx != NULL );

    sIsLogging = smLayerCallback::isMtxModeLogging( aMtx );

    sEndLSN = smLayerCallback::getMtxEndLSN( aMtx );

    IDE_ASSERT( sEndLSN.mOffset < sLogFileSize );

    if( sIsLogging == ID_TRUE )
    {
        /* logging mode */
        if ( smLayerCallback::isLogWritten( aMtx ) == ID_TRUE )
        {
            sdbBCB::setPageLSN((sdbFrameHdr*)aBCB->mFrame, sEndLSN);
        }
    }
    else
    {
        /* nologging mode 또는 restart recovery redo mode*/
        if ( smLayerCallback::isLSNZero( &sEndLSN ) != ID_TRUE )
        {
            /* Restart Recovery Redo시에만 이쪽으로 들어온다.
             * Read Only로 수행된다. Redo시 Mini Trans는 No-Logging으로
             * Begin되고
             * RedoLOG LSN이 Mini Trans가 Commit시 EndLSN으로 등록된다.
             * 따라서 sEndLSN은 현재 이 페이지에 반영된 Redo Log의 LSN이다.
             */
            sPageLSN = smLayerCallback::getPageLSN( aBCB->mFrame );

            if ( smLayerCallback::isLSNLT( &sPageLSN, &sEndLSN )
                 == ID_TRUE )
            {
                /* Redo시 Redo로그가 페이지에 반영이 되었기때문 */
                sdbBCB::setPageLSN( (sdbFrameHdr*)aBCB->mFrame, sEndLSN);
            }
        }
        else
        {
            /* no logging mode */
        }
    }
}

/****************************************************************
 * Description :
 *  버퍼 페이지를 해제한다.
 *  이함수는 다음의 경우에 호출된다.
 *  - mtx가 savepoint까지 롤백될때 호출
 *    이때 여기서 해제하는 어떤 페이지도 수정된 상태가 아니다.
 *  - mtx가 rollback되는 경우
 *
 * Implementation:
 *
 * mutex
 * 페이지의 fix를 해제한다.
 * 페이지의 latch를 해제한다.
 * mutex
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 *  aLatchMode  - [IN]  래치 모드, page latch를 잡았을때 래치 모드
 ****************************************************************/
void sdbBufferPool::releasePageForRollback( idvSQL * aStatistics,
                                            void   * aBCB,
                                            UInt     aLatchMode )
{
    sdbBCB *sBCB;

    IDE_DASSERT( aBCB != NULL);

    sBCB = (sdbBCB*)aBCB;

    if( (aLatchMode == (UInt)SDB_X_LATCH) ||
        (aLatchMode == (UInt)SDB_S_LATCH))
    {
        sBCB->unlockPageLatch();
    }

    unfixPage( aStatistics, sBCB );

    return;
}


/****************************************************************
 * Description :
 *      해당 page id를 버퍼에 존재함을 보장하고, BCB에 fix를 해놓아서
 *      버퍼에서 내려가지 않음을 보장시켜 준다.
 *
 * aStatistics  - [IN]  통계정보
 * aSpaceID     - [IN]  table space ID
 * aPageID      - [IN]  버퍼에 fix시키고자하는 pageID
 * aReadMode    - [IN]  page read mode(SPR or MPR)
 * aRetPage     - [IN]  버퍼에 fix된 페이지를 리턴
 * aHit         - [OUT] Hit되었는지 여부 리턴
 * aFixedBCB    - [OUT] fix된 BCB를 리턴한다.
 *
 ****************************************************************/
IDE_RC sdbBufferPool::fixPageInBuffer( idvSQL              *aStatistics,
                                       scSpaceID            aSpaceID,
                                       scPageID             aPageID,
                                       sdbPageReadMode      aReadMode,
                                       UChar              **aRetPage,
                                       idBool              *aHit,
                                       sdbBCB             **aFixedBCB,
                                       idBool             * aIsCorruptPage )
{
    sdbBCB *sBCB = NULL;
    sdbHashChainsLatchHandle *sHashChainsHandle=NULL;

    IDE_DASSERT( aSpaceID <= SC_MAX_SPACE_COUNT - 1);
    IDE_DASSERT( aRetPage != NULL );

    /* hash에 있다면 그냥 fix후에 리턴하고, 그렇지 않다면 readPage한다.*/
    sHashChainsHandle = mHashTable.lockHashChainsSLatch( aStatistics,
                                                         aSpaceID,
                                                         aPageID);

    IDE_TEST( mHashTable.findBCB( aSpaceID,
                                  aPageID,
                                  (void**)&sBCB )
              != IDE_SUCCESS );

    if ( sBCB == NULL )
    {
        /*find 실패 readPage해야 한다.*/
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;

        IDE_TEST( readPage( aStatistics,
                            aSpaceID,
                            aPageID,
                            aReadMode,
                            aHit,
                            &sBCB,
                            aIsCorruptPage )
                  != IDE_SUCCESS );
        /*readPage후에는 반드시 sBCB가 리턴됨을 보장할 수 있다.*/
        IDE_ASSERT( sBCB != NULL );
        IDE_ASSERT( isFixedBCB(sBCB) == ID_TRUE );
    }
    else
    {
        fixAndUnlockHashAndTouch( aStatistics, sBCB, sHashChainsHandle );
        sHashChainsHandle = NULL;
        *aHit = ID_TRUE;
    }

    *aRetPage = sBCB->mFrame;

    if ( aFixedBCB != NULL )
    {
        *aFixedBCB = sBCB;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sHashChainsHandle != NULL )
    {
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;
    }
    else
    {
        /* nothing to do */
    }
    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 *  MPR 기능을 위해 수행되는 함수.  연속되어있는 aSpaceID내의 aStartPID부터
 *  aPageCount만큼 읽어 들인다.
 *  MPR은 연속되어 있는 영역을 한번에 읽어 들이기 위해 사용한다.
 *  I/O 양보다 I/O 횟수를 줄이는 것이 매우 중요하기 때문이다.
 *  하지만, 만약 MPR의 target이 되는 페이지들이 이미 상당수 버퍼에 있다면,
 *  이들을 연속적으로 읽어 들여야 하는지 마는지를 결정해야 한다.
 *  (여기에 대한 자세한 설명은 PROJ-1568 문서를 참고.)
 *  간단히, 한번에 읽는것이 여러번 읽는 것보다 비용이 싸다면 한번에 읽는다.
 *  이것을 위해서 sdbMPRKey자료구조의 sdbReadUnit을 이용한다.
 *  그리고, 비용 계산은 analyzeCostToReadAtOnce을 통해서 한다.
 *
 * Implementation:
 *  sStartPID부터 버퍼에 미리 존재하는지 확인한다. 이때, 버퍼에 이미 존재 한다면
 *  굳이 읽어올 필요가 없으므로, 이것에 fix를 걸어놓는다.
 *  만약 버퍼에 존재하지 않는다면 페이지 x래치를 걸어서 해시에 삽입해 놓는다.
 *  이 페이지 x래치는 차후에 read가 끝난 후에 풀린다.
 *  이때, 연속된 페이지들에 대한 정보를 sdbMPRKey자료구조의 sdbReadUnit에
 *  설정해 놓는다.
 *  mReadUnit에는 어떤 pid부터 몇개를 연속으로 읽어 와야 하는지 정보가 쓰여져
 *  있다.
 *  이것을 이용해 비용을 계산하고, 한번에 읽을지 여러번에 걸쳐서 읽을지를
 *  결정한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPI    - [IN]  시작 page ID
 *  aPageCount  - [IN]  aStartPID부터 읽어들일 페이지 갯수
 *  aKey        - [IN]  MPR Key
 ****************************************************************/
IDE_RC sdbBufferPool::fetchPagesByMPR( idvSQL               *aStatistics,
                                       scSpaceID             aSpaceID,
                                       scPageID              aStartPID,
                                       UInt                  aPageCount,
                                       sdbMPRKey            *aKey )
{
    sdbBCB    * sBCB;
    sdbBCB    * sAlreadyExistBCB;
    sdbBCB    * sFixedBCB;
    SInt        sUnitIndex  = -1;
    idBool      sContReadIO = ID_FALSE;
    UInt        i = 0;
    UInt        j = 0;
    UInt        sMPRSetBCBCnt = 0;
    scPageID    sPID;
    idBool      sIsNeedIO = ID_FALSE;

    sdbHashChainsLatchHandle * sLatchHandle;

    /* PROJ-2102 */
    sdsBCB    * sSBCB = NULL;
    idBool      sIsCorruptRead = ID_FALSE;

    for( i = 0, sPID = aStartPID; i < aPageCount; i++, sPID++ )
    {
        /* 먼저 자신이 삽입을 원하는 pid가 이미 버퍼에 존재 하는지 확인한다.*/
        sLatchHandle = mHashTable.lockHashChainsSLatch(aStatistics,
                                                       aSpaceID,
                                                       sPID);

        IDE_TEST_RAISE( ( mHashTable.findBCB( aSpaceID, sPID, (void**)&sBCB )
                          != IDE_SUCCESS ), FAIL_MPR_BCB_SETTING );

        if ( sBCB != NULL )
        {
            /* 이미 존재한다면, fix값을 증가 시켜서 버퍼에 그대로 남아 있게 한다.
             * 그리고, 나중에 그것을 이용한다.*/
            sBCB->lockBCBMutex(aStatistics);
            sBCB->incFixCnt();
            /* BUG-22378 full scan(MPR)의 getPgae수행시 BCB의 touch count를 증가
             * 시켜야 합니다.
             * 기존에 존재하는 BCB에 대한 접근은 touch count를 증가 시켜야 합니다.
             * */
            sBCB->updateTouchCnt();
            sBCB->unlockBCBMutex();

            mHashTable.unlockHashChainsLatch( sLatchHandle );
            sLatchHandle = NULL;

            /* BUG-29643 [valgrind] sdbMPRMgr::fixPage() 에서 valgrind 오류가
             *           발생합니다.
             * IO mutex를 잡고 풀어 BCB에 DISK I/O 가 완료되기까지 기다린다. */
            if ( sBCB->mReadyToRead == ID_FALSE )
            {
                sBCB->mReadIOMutex.lock( aStatistics );
                sBCB->mReadIOMutex.unlock();
            }

            if ( sBCB->mReadyToRead == ID_FALSE )
            {
                ideLog::log( IDE_DUMP_0,
                             "SpaceID: %u, "
                             "PageID: %u\n",
                             aSpaceID,
                             sPID );

                ideLog::logMem( IDE_DUMP_0,
                                (UChar*)sBCB,
                                ID_SIZEOF( sdbBCB ),
                                "BCB Dump:\n" );

                ideLog::logMem( IDE_DUMP_0,
                                (UChar*)sBCB->mFrame,
                                SD_PAGE_SIZE,
                                "Frame Dump:\n" );

                sBCB->dump( sBCB );

                IDE_ASSERT( 0 );
            }

            /* 이것은 버퍼에 존재함을 표시해둔다.*/
            aKey->mBCBs[i].mType = SDB_MRPBCB_TYPE_BUFREG;
            aKey->mBCBs[i].mBCB  = sBCB;
            sContReadIO = ID_FALSE;
            IDE_ASSERT( aKey->mBCBs[i].mBCB->mFixCnt != 0 );
        }
        else
        {
            /* 존재하지 않는다면, 이것은 디스크로 부터 읽어와야 할 대상이다.
             * */
            mHashTable.unlockHashChainsLatch( sLatchHandle );
            sLatchHandle = NULL;

            /* key에도 (getVictim overhead를 줄이기 위해) freeBCB를 유지하는데,
             * 만약 없다면 getVictim을 호출하여 BCB를 얻어오고,
             * 있다면, 그걸 그대로 이용한다.
             */
            sBCB = aKey->removeLastFreeBCB();
            if ( sBCB == NULL )
            {
                IDE_TEST_RAISE( ( getVictim( aStatistics, (UInt)sPID, &sBCB )
                                  != IDE_SUCCESS ), FAIL_MPR_BCB_SETTING );
            }
            /* 디스크에서 data를 읽어 오기 위한 readPageFromDisk를 수행하기 위해
             * 해야 하는 작업들은 readPage에서의 그것과 매우 유사하다.
             * 단, MPR에서는 touchCount를 증가 시키지 않는다.*/
            sBCB->lockBCBMutex( aStatistics );

            IDE_DASSERT( sBCB->mSBCB == NULL );
            IDE_DASSERT( sBCB->mFixCnt == 0 );

            /* 기존에 존재하는 BCB에 대한 접근이 아닌 경우엔 touch count를 증가
             * 시키지 않습니다. (touch count는 0)
             * 그렇기 때문에 MPR이 가지고 있는 BCB를 누구라도 접근하기만 해도
             * touch count가 증가 하게 되어 있습니다.
             * cleanUpKey에서 touch count가 증가되지 않은 BCB는
             * 재사용하고, touch count가 증가된 BCB에 대해선 (동시에 접근되는
             * 쓰레드가 존재하므로) LRU에 삽입합니다.
             * */
            sBCB->incFixCnt();
            sBCB->mSpaceID  = aSpaceID;
            sBCB->mPageID   = sPID;

            /*  여기서 BCB의 상태를 clean으로 하지만,
             *  BCB의 mPageType을 설정하지는 않는다.
             *  그렇기 때문에, clean이지만 pageType이 설정되어 있지 않게 된다.*/
            sBCB->mState     = SDB_BCB_CLEAN;
            sBCB->mPrevState = SDB_BCB_CLEAN;

            SM_LSN_INIT( sBCB->mRecoveryLSN );
            sBCB->unlockBCBMutex();

            // free 상태의 BCB는 모두 mReadyToRead가 ID_FALSE 이다.
            IDE_ASSERT( sBCB->mReadyToRead == ID_FALSE );

            /* 실제 disk에서 읽어 올 것이므로 아래 두 래치를 잡는다.
             * 래치에 대한 자세한 설명은 sdbBufferPool::readPage를 참조. */
            sBCB->lockPageXLatch(aStatistics);
            (void)sBCB->mReadIOMutex.lock(aStatistics);

            IDU_FIT_POINT( "1.BUG-29643@sdbBufferPool::fetchPagesByMPR" );

            sLatchHandle = mHashTable.lockHashChainsXLatch( aStatistics, aSpaceID, sPID );
            mHashTable.insertBCB( sBCB,
                                  (void**)&sAlreadyExistBCB );

            if ( sAlreadyExistBCB != NULL )
            {
                sAlreadyExistBCB->lockBCBMutex(aStatistics);
                sAlreadyExistBCB->incFixCnt();
                /* BUG-22378 full scan(MPR)의 getPgae수행시 BCB의 touch count를 증가
                 * 시켜야 합니다.
                 * 기존에 존재하는 BCB에 대한 접근은 touch count를 증가 시켜야 합니다.
                 * */
                sAlreadyExistBCB->updateTouchCnt();
                sAlreadyExistBCB->unlockBCBMutex();

                mHashTable.unlockHashChainsLatch( sLatchHandle );
                sLatchHandle = NULL;

                sBCB->mReadIOMutex.unlock();
                sBCB->unlockPageLatch();

                sBCB->lockBCBMutex(aStatistics);
                sBCB->decFixCnt();
                sBCB->setToFree();
                sBCB->unlockBCBMutex();

                /* BUG-29643 [valgrind] sdbMPRMgr::fixPage() 에서 valgrind
                 *           오류가 발생합니다.
                 * IO mutex를 잡고 풀어 BCB에 DISK I/O 가 완료되기까지
                 * 기다린다. */
                if ( sAlreadyExistBCB->mReadyToRead == ID_FALSE )
                {
                    sAlreadyExistBCB->mReadIOMutex.lock( aStatistics );
                    sAlreadyExistBCB->mReadIOMutex.unlock();
                }

                if ( sAlreadyExistBCB->mReadyToRead == ID_FALSE )
                {
                    ideLog::log( IDE_DUMP_0,
                                 "SpaceID: %u, "
                                 "PageID: %u\n",
                                 aSpaceID,
                                 sPID );

                    ideLog::logMem( IDE_DUMP_0,
                                    (UChar*)sAlreadyExistBCB,
                                    ID_SIZEOF( sdbBCB ),
                                    "BCB Dump:\n" );

                    ideLog::logMem( IDE_DUMP_0,
                                    (UChar*)sAlreadyExistBCB->mFrame,
                                    SD_PAGE_SIZE,
                                    "Frame Dump:\n" );

                    sAlreadyExistBCB->dump( sAlreadyExistBCB );

                    IDE_ASSERT( 0 );
                }

                aKey->mBCBs[i].mType = SDB_MRPBCB_TYPE_BUFREG;
                aKey->mBCBs[i].mBCB  = sAlreadyExistBCB;
                aKey->addFreeBCB(sBCB);

                sContReadIO = ID_FALSE;
                IDE_ASSERT( aKey->mBCBs[i].mBCB->mFixCnt != 0 );

                continue;
            }

            mHashTable.unlockHashChainsLatch( sLatchHandle );
            sLatchHandle = NULL;

            /* 1. 먼저 Secondatry Buffer에서 검색한다. */
            if( isSBufferServiceable() == ID_TRUE )
            { 
                IDE_TEST_RAISE( ( sdsBufferMgr::findBCB( aStatistics, aSpaceID, sPID, &sSBCB )
                          != IDE_SUCCESS ), FAIL_MPR_BCB_SETTING );
            }   

            if ( sSBCB != NULL )
            {
                sContReadIO = ID_FALSE;

                /* Secondary Buffer는 MPR 지원 안함
                 * MPR을 하기 위해 ReadIOMutex를 잡은 SBCB가
                 * BCB가 내려 쓰려고 선정한 블럭에 있으면
                 * (Buffer, Secondary Buffer둘다 상당히 작은 상황에서 발생가능성 있음)
                 * victim이 되지 못해 deadlock 상황 발생
                 * 하지만.. single page로 읽는다고 하더라도 성능의 문제는 없음.
                 * SSD를 고려한 SB이므로.
                 */
                IDE_TEST_RAISE( ( sdsBufferMgr::moveUpbySinglePage( aStatistics, 
                                                                    &sSBCB, 
                                                                    sBCB, 
                                                                    &sIsCorruptRead )
                                  != IDE_SUCCESS ), FAIL_MPR_BCB_SETTING );

                if( sIsCorruptRead == ID_FALSE )
                {
                    SM_GET_LSN( sBCB->mRecoveryLSN, sSBCB->mRecoveryLSN );
                    /* link */
                    sBCB->mSBCB = sSBCB;
                    IDE_DASSERT( validate(sBCB) == IDE_SUCCESS );
                    if( sSBCB->mState == SDS_SBCB_DIRTY )
                    {
                        setDirtyBCB( aStatistics, sBCB );
                    }
                    /* MPR operation */  
                    aKey->mBCBs[i].mType = SDB_MRPBCB_TYPE_NEWFETCH;
                    aKey->mBCBs[i].mBCB  = sBCB;
                    IDE_ASSERT( aKey->mBCBs[i].mBCB->mFixCnt != 0 );
                    /* copyToFrame operation */
                    sBCB->mReadyToRead = ID_TRUE;
                    (void)sBCB->mReadIOMutex.unlock();
                    sBCB->unlockPageLatch();
                    
                    continue;
                }
            }

            sIsNeedIO = ID_TRUE;

            if( sContReadIO == ID_FALSE )
            {
                sContReadIO = ID_TRUE;
                sUnitIndex++;
                // 읽어야 하는 페이지들 분포가 OXOXOX 이런 분포를 이룰때
                // (O : 읽어야 하는 페이지,  X: 버퍼에 존재하는 페이지)
                // sUnitIndex의 값은 최대가 되며, 이것은
                // SDB_MAX_MPR_PAGE_COUNT / 2 이다.
                IDE_DASSERT(sUnitIndex <= SDB_MAX_MPR_PAGE_COUNT / 2);

                aKey->mReadUnit[sUnitIndex].mFirstPID = sBCB->mPageID;
                aKey->mReadUnit[sUnitIndex].mReadBCBCount = 0;
                aKey->mReadUnit[sUnitIndex].mReadBCB = &(aKey->mBCBs[i]);
            }

            aKey->mReadUnit[sUnitIndex].mReadBCBCount++;
            aKey->mBCBs[i].mType = SDB_MRPBCB_TYPE_NEWFETCH;
            aKey->mBCBs[i].mBCB  = sBCB;
            IDE_ASSERT( aKey->mBCBs[i].mBCB->mFixCnt != 0 );
        }
    }

    sMPRSetBCBCnt        = aPageCount; 

    /* 해당 FIT point는 BUG-45904에서도 사용 */
    IDU_FIT_POINT( "2.BUG-29643@sdbBufferPool::fetchPagesByMPR" );

    aKey->mReadUnitCount = (UInt)sUnitIndex + 1;
    aKey->mSpaceID       = aSpaceID;
    aKey->mIOBPageCount  = aPageCount;
    aKey->mCurrentIndex  = 0;

    if( sIsNeedIO == ID_TRUE )
    {
        if( aPageCount != 1 )
        {
            if( analyzeCostToReadAtOnce(aKey) == ID_TRUE )
            {
                IDE_TEST(fetchMultiPagesAtOnce(aStatistics,
                                               aStartPID,
                                               aPageCount,
                                               aKey )
                         != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(fetchMultiPagesNormal( aStatistics,
                                                aKey )
                         != IDE_SUCCESS);
            }
        }
        else
        {
            IDE_TEST( fetchSinglePage( aStatistics,
                                       aStartPID,
                                       aKey )
                      != IDE_SUCCESS );
        }
    }

    aKey->mState = SDB_MPR_KEY_FETCHED;

    return IDE_SUCCESS;

    /* BUG-45959: MPRKey에 BCB를 setting하던 중 예외처리가 발생하는 경우 
     * setting된 BCB 개수 만큼 unfix 해주어야 함.   
     */
    IDE_EXCEPTION( FAIL_MPR_BCB_SETTING )
    {
        if( i != 0 )
        {
            sMPRSetBCBCnt = i;
        }
        else
        {
            sMPRSetBCBCnt = 0;
        }
    }
 
    IDE_EXCEPTION_END;

    if ( sLatchHandle != NULL )
    {
        mHashTable.unlockHashChainsLatch( sLatchHandle );
        sLatchHandle =  NULL;
    }
    else
    {
        /* nothing to do */
    }

    /* BUG-45904 예외처리시 증가시킨 fix count를 감소시켜야 한다. */
    for( j = 0 ; j < sMPRSetBCBCnt ; j++ )
    {
        sFixedBCB = aKey->mBCBs[j].mBCB;

        sLatchHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                        aSpaceID,
                                                        sFixedBCB->mPageID);

        sFixedBCB->lockBCBMutex(aStatistics);

        if ( aKey->mBCBs[j].mType == SDB_MRPBCB_TYPE_NEWFETCH )
        {
            if( ( sFixedBCB->mFixCnt == 1 ) &&
                ( ( sFixedBCB->mState == SDB_BCB_CLEAN ) || ( sFixedBCB->mState == SDB_BCB_FREE ) ) )
            {
                /* 해당 BCB가 이 함수( fetchPagesByMPR )를 통해서 처음 버퍼에 올라온 
                 * 경우 해시에서 제거한다. */
                sFixedBCB->decFixCnt();
                sFixedBCB->setToFree();
                sFixedBCB->unlockBCBMutex();

                mHashTable.removeBCB( sFixedBCB );

                /* BCB를 재활용할 수 있도록 freeBCB에 삽입한다. */
                aKey->addFreeBCB( sFixedBCB );

            }
            else
            {
                /* 자신만 보고 있지 않거나 dirty일 경우 해시에서 제거하지 않고 LRU에 삽입한다. */
                sFixedBCB->decFixCnt();
                sFixedBCB->unlockBCBMutex();
                mLRUList[getLRUListIdx((UInt)sFixedBCB->mPageID)].insertBCB2BehindMid(
                    aStatistics,
                    sFixedBCB );
            }
        }
        else
        {
            /* 기존에 이미 버퍼에 올라와 있던 BCB라면 fix count만 감소시킨다. */
            sFixedBCB->decFixCnt();
            sFixedBCB->unlockBCBMutex();
        }

        mHashTable.unlockHashChainsLatch( sLatchHandle );
    }

    return IDE_FAILURE;
}


/****************************************************************
 * Description:
 *  sdbMPRKey의 readUnit 을 보고선 한번에 읽어야 할지 아니면
 *  여러번에 읽어야 할지를 결정한다.
 *
 *  aKey     - [IN]  MPR Key
 ****************************************************************/
idBool sdbBufferPool::analyzeCostToReadAtOnce(sdbMPRKey *aKey)
{
    if( aKey->mReadUnitCount == 1 )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/****************************************************************
 * Description:
 *  연속적인 메모리 공간이 읽어들인 내용을 실제 BCB가 가리키고 있는 mFrame공간에
 *  memcpy를 수행한다.
 *  복사를 수행했다면, BCB에 걸어놓은 페이지 x래치를 제거한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aReadUnit   - [IN]  copyToFrame의 단위, 하나의 readUnit에 속해있는
 *                      페이지 만큼 복사를 수행한다. BCB 포인터정보 등 유용한
 *                      정보가 들어있다.
 *  aIOBPtr     - [IN]  실제 데이터가 들어있는 연속적인 메모리 공간.
 *                      여기의 내용을 mFrame공간에 복사한다.
 ****************************************************************/
IDE_RC sdbBufferPool::copyToFrame(
    idvSQL                 *aStatistics,
    scSpaceID               aSpaceID,
    sdbReadUnit            *aReadUnit,
    UChar                  *aIOBPtr )
{
    sdbBCB  *sBCB = NULL;
    UInt     i    = 0;
    UInt            sCorruptPageReadCnt = 0;
    scSpaceID       sCurruptSpaceID     = SC_NULL_SPACEID;
    scPageID        sCurruptPageID      = SC_NULL_PID;

    for ( i = 0;
          i < aReadUnit->mReadBCBCount;
          i++ )
    {
        sBCB = aReadUnit->mReadBCB[i].mBCB;

        idlOS::memcpy(sBCB->mFrame,
                      aIOBPtr + i * mPageSize,
                      mPageSize);

        if ( smLayerCallback::checkAndSetPageCorrupted( aSpaceID,
                                                        sBCB->mFrame ) == ID_TRUE )
        {
            if( ( smuProperty::getCorruptPageErrPolicy()
              & SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
            != SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
            {
                // BUG-45598: copyToFrame @ page corrupt 상황 처리
                sBCB->mPageReadError = ID_TRUE;
                sCurruptSpaceID = sBCB->mSpaceID;
                sCurruptPageID  = sBCB->mPageID;
                sCorruptPageReadCnt++;
            }
            else
            {
                sCurruptSpaceID = sBCB->mSpaceID;
                sCurruptPageID  = sBCB->mPageID;
                IDE_RAISE( page_corruption_error );
            }

        }

        setFrameInfoAfterReadPage( sBCB,
                                   ID_FALSE );  // check to online tablespace
        
        /* BUG-22341: TC/Server/qp4/Project1/PROJ-1502-PDT/Design/DELETE/RangePartTable.sql
         * 수행시 서버 사망:
         *
         * sBCB->mPageType은 setFrameInfoAfterReadPage에서 초기화 됩니다. */
        mStatistics.applyReadPages( aStatistics ,
                                    sBCB->mSpaceID,
                                    sBCB->mPageID,
                                    sBCB->mPageType );

        sBCB->mReadyToRead = ID_TRUE;
        sBCB->mReadIOMutex.unlock();
        sBCB->unlockPageLatch();
    }

    // BUG-45598: corrupt 페이지가 있으면 에러
    IDE_TEST_RAISE( ( sCorruptPageReadCnt != 0 )
                    &&( smuProperty::getCrashTolerance() == 0 ),  page_corruption_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( page_corruption_error );
    {
        switch( smLayerCallback::getCorruptPageReadPolicy() )
        {
            case SDB_CORRUPTED_PAGE_READ_FATAL :
                IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                          sCurruptSpaceID,
                                          sCurruptPageID));
                break;
            case SDB_CORRUPTED_PAGE_READ_ABORT :
                IDE_SET( ideSetErrorCode( smERR_ABORT_PageCorrupted,
                                          sCurruptSpaceID,
                                          sCurruptPageID));              
                break;
            default:
                break;
        }
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 *  MPR의 대상이 되는 페이지를 디스크로부터 읽어 들이는 함수.
 *  이때, 한번에 읽는 것이 아니라 여러번에 걸쳐서 읽는다.
 *  읽는 단위는 sdbMPRKey의 mReadUnit이다.
 *  mReadUnit에는 어떤 pid부터 몇개를 연속으로 읽어 와야 하는지 정보가 쓰여져
 *  있다.
 *
 *  aStatistics - [IN]  통계정보
 *  aKey        - [IN]  MPR Key
 ****************************************************************/
IDE_RC sdbBufferPool::fetchMultiPagesNormal(
                                    idvSQL                 * aStatistics,
                                    sdbMPRKey              * aKey )
{
    SInt i;
    sdbReadUnit *sReadUnit;
    idvTime      sBeginTime;
    idvTime      sEndTime;
    ULong        sReadTime         = 0;
    ULong        sCalcChecksumTime = 0;
    UInt         sReadPageCount    = 0;

    for (i = 0; i < aKey->mReadUnitCount; i++)
    {
        sReadUnit = &(aKey->mReadUnit[i]);

        IDV_TIME_GET(&sBeginTime);

        if( sReadUnit->mReadBCBCount == 1 )
        {
            mStatistics.applyBeforeSingleReadPage( aStatistics );
        }
        else
        {
            mStatistics.applyBeforeMultiReadPages( aStatistics );
        }

        IDE_TEST( sddDiskMgr::read( aStatistics,
                                    aKey->mSpaceID,
                                    sReadUnit->mFirstPID,
                                    sReadUnit->mReadBCBCount,
                                    aKey->mReadIOB,
                                    ID_TRUE )  /* aFatal */
                 != IDE_SUCCESS);

        if( sReadUnit->mReadBCBCount == 1 )
        {
            mStatistics.applyAfterSingleReadPage( aStatistics );
        }
        else
        {
            mStatistics.applyAfterMultiReadPages( aStatistics );
        }

        IDV_TIME_GET(&sEndTime);
        sReadTime += IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

        sBeginTime = sEndTime;
        IDE_TEST( copyToFrame( aStatistics,
                               aKey->mSpaceID,
                               sReadUnit,
                               aKey->mReadIOB )
                  != IDE_SUCCESS );
        IDV_TIME_GET(&sEndTime);
        sCalcChecksumTime += IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

        sReadPageCount += sReadUnit->mReadBCBCount;
    }

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * MPR을 이용한 Fullscan, ReadPage시 걸린 시간 저장함.
     * Checksum은 실제 Checksum 외 Frame에 복사하는 시간도 걸림 */
    mStatistics.applyReadByMPR( sCalcChecksumTime,
                                sReadTime,
                                sReadPageCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/****************************************************************
 * Description:
 *  MPR의 대상이 되는 페이지를 디스크로부터 읽어 들이는 함수.
 *  이때, MPR의 대상이 되는 모든 페이지를 한번에 읽어들인다.
 *  한번에 읽어 들였기 때문에, 이미 버퍼에 존재하는 페이지 역시도 읽어 들인다.
 *  하지만, 이것의 내용은 그냥 무시해 버린다.
 *
 *  aStatistics - [IN]  통계정보
 *  aStartPID   - [IN]  읽어들일 페이지들 중  처음 페이지 id
 *  aPageCount  - [IN]  읽어들일 페이지 갯수
 *  aKey        - [IN]  MPR Key
 ****************************************************************/
IDE_RC sdbBufferPool::fetchMultiPagesAtOnce(
                                idvSQL                 *aStatistics,
                                scPageID                aStartPID,
                                UInt                    aPageCount,
                                sdbMPRKey              *aKey )
{
    UInt         sIndex;
    SInt         i;
    sdbReadUnit *sReadUnit;
    idvTime      sBeginTime;
    idvTime      sEndTime;
    ULong        sReadTime;
    ULong        sCalcChecksumTime;
    
    IDV_TIME_GET(&sBeginTime);

    if( aKey->mReadUnit[0].mReadBCBCount == 1 )
    {
        mStatistics.applyBeforeSingleReadPage( aStatistics );
    }
    else
    {
        mStatistics.applyBeforeMultiReadPages( aStatistics );
    }

    IDE_TEST( sddDiskMgr::read( aStatistics,
                                aKey->mSpaceID,
                                aStartPID,
                                aPageCount,
                                aKey->mReadIOB,
                                ID_TRUE ) /* aFatal */
             != IDE_SUCCESS);

    if( aKey->mReadUnit[0].mReadBCBCount == 1 )
    {
        mStatistics.applyAfterSingleReadPage( aStatistics );
    }
    else
    {
        mStatistics.applyAfterMultiReadPages( aStatistics );
    }

    IDV_TIME_GET(&sEndTime);
    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    /* 실제 버퍼에 존재하지 않는 페이지들에 대해서만 frame복사를 수행한다. */
    sBeginTime = sEndTime;
    for (i = 0; i < aKey->mReadUnitCount; i++)
    {
        sReadUnit = &(aKey->mReadUnit[i]);
        sIndex = sReadUnit->mFirstPID - aStartPID;
        IDE_TEST( copyToFrame( aStatistics,
                               aKey->mSpaceID,
                               sReadUnit,
                               aKey->mReadIOB + sIndex * mPageSize )
                  != IDE_SUCCESS );
    }
    IDV_TIME_GET(&sEndTime);
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * MPR을 이용한 Fullscan, ReadPage시 걸린 시간 저장함.
     * Checksum은 실제 Checksum 외 Frame에 복사하는 시간도 걸림 */
    mStatistics.applyReadByMPR( sCalcChecksumTime,
                                sReadTime,
                                aPageCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbBufferPool::fetchSinglePage(
                            idvSQL                 * aStatistics,
                            scPageID                 aPageID,
                            sdbMPRKey              * aKey )
{
    sdbBCB   *sBCB;
    SInt      sState = 0;
    idvTime   sBeginTime;
    idvTime   sEndTime;
    ULong     sReadTime;
    ULong     sCalcChecksumTime;
    UInt      sCorruptPageReadCnt = 0;

    sBCB = aKey->mReadUnit[0].mReadBCB[0].mBCB;

    IDV_TIME_GET(&sBeginTime);

    mStatistics.applyBeforeSingleReadPage( aStatistics );
    sState = 1;

    IDE_TEST( sddDiskMgr::read( aStatistics,
                                aKey->mSpaceID,
                                aPageID,
                                1,
                                sBCB->mFrame,
                                ID_TRUE ) /* aFatal */
             != IDE_SUCCESS);

    sState = 0;
    mStatistics.applyAfterSingleReadPage( aStatistics );

    IDV_TIME_GET(&sEndTime);
    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
    sBeginTime = sEndTime;

    if ( smLayerCallback::checkAndSetPageCorrupted( aKey->mSpaceID,
                                                    sBCB->mFrame ) == ID_TRUE )
    {
        // BUG-45598: mpr 이용하여 페이지 하나 씩 fetch 할 경우 고려
        if( ( smuProperty::getCorruptPageErrPolicy()
              & SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
            != SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL )
        {
            sBCB->mPageReadError = ID_TRUE;
            sCorruptPageReadCnt++;
        }
        else
        {
            IDE_RAISE( page_corruption_error );
        }
    }

    setFrameInfoAfterReadPage( sBCB,
                               ID_FALSE ); // check to online tablespace

    mStatistics.applyReadPages( aStatistics ,
                                sBCB->mSpaceID,
                                sBCB->mPageID,
                                sBCB->mPageType );

    sBCB->mReadyToRead = ID_TRUE;
    sBCB->mReadIOMutex.unlock();
    sBCB->unlockPageLatch();

    IDV_TIME_GET(&sEndTime);
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance.
     * MPR을 이용한 Fullscan, ReadPage시 걸린 시간 저장함.
     * MPR이지만, 읽을 페이지가 하나 있을 경우 호출되는데
     * 다른 통계정보에서는 '한페이지만 읽었으므로' single에 들어가지만
     * 여기서는 'MPR이냐'로 구분하기 때문에, MPR로 분류함 */
    mStatistics.applyReadByMPR( sCalcChecksumTime,
                                sReadTime,
                                1 );

    // BUG-45598: corrupt 페이지가 있으면 에러( MPR이용하여 Single 페이지 fetch 일 경우 )
    IDE_TEST_RAISE(   ( sCorruptPageReadCnt != 0 )
                    &&( smuProperty::getCrashTolerance() == 0 ),  page_corruption_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( page_corruption_error );
    {
        switch( smLayerCallback::getCorruptPageReadPolicy() )
        {
            case SDB_CORRUPTED_PAGE_READ_FATAL :
                IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                      sBCB->mSpaceID,
                                      sBCB->mPageID ));

                break;
            case SDB_CORRUPTED_PAGE_READ_ABORT :
                IDE_SET( ideSetErrorCode( smERR_ABORT_PageCorrupted,
                                          sBCB->mSpaceID,
                                          sBCB->mPageID));
                break;
            default:
                break;
        }
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        mStatistics.applyAfterSingleReadPage( aStatistics );
    }

    return IDE_FAILURE;
}


/****************************************************************
 * Description:
 *  MPR을 위해 읽어 왔던 여러 정보들을 초기화 한다.
 *  이것은 fetchMultiPages와 짝을 이루는 함수이다. 즉, fetchMultiPages가
 *  수행되고, 이때 읽어왔던 페이지들에 모든 접근이 이루어 지고 난 이후에
 *  cleanUpKey함수를 호출하고, 다시 fetchMultiPages를 호출한다.
 *
 * 이 함수에서 하는일:
 *  MPR은 기본적으로 full scan을 위해서 만들어 졌기 때문에, 자신이 한번 읽어온
 *  페이지는 한동안 접근이 없을 거라는 가정하에서 만들어 졌다.(MRU알고리즘)
 *  그렇기 때문에 cleanUpKey라는 함수가 필요한데, 이 함수에서는 MPR을 위해
 *  읽어온 BCB들을 다음번 fetchMultiPages를 위해 free상태로 만드는 역활을 한다.
 *  이때, 모든 BCB를 free로 만들지 않는다. 만약 동시에 다른 트랜잭션에 의해서
 *  접근된 BCB들이 존재한다면, 이것은 앞으로도 한동안 접근될 것으로 판단하고,
 *  hash Table에 그대로 남겨둔다.(버퍼매니저의 LRU리스트에 삽입)
 *  만약 그렇지 않다면, free로 만들고, 다음 fetchMultiPages를 위해 쓰여진다.
 *
 *  동시에 다른트랜잭션에 의해서 접근되었는지 여부는 현재 touch count로만 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aKey        - [IN]  MPR key
 ****************************************************************/
void sdbBufferPool::cleanUpKey( idvSQL    *aStatistics,
                                sdbMPRKey *aKey,
                                idBool     aCachePage )
{
    SInt                      i;
    sdbBCB                   *sBCB;
    sdbHashChainsLatchHandle *sLatchHandle;
    sdsBCB                   *sSBCB;

    IDE_ASSERT(aKey->mState == SDB_MPR_KEY_FETCHED);

    for ( i = 0; i < aKey->mIOBPageCount ; i++ )
    {
        sBCB = aKey->mBCBs[i].mBCB;

        if ( aKey->mBCBs[i].mType == SDB_MRPBCB_TYPE_NEWFETCH )
        {
            //  구지 여기서 미리 해시에서 제거할 필요는 없어 보인다.
            //  일단은 해시에 달려 있는 채로 MPRKey의 mFreeBCB에 삽입하고,
            //  나중에 removeLastFreeBCB때에 리턴하면서 hash에서 제거하면
            //  hit율에 더 좋은 영향을 미친다. 하지만, 그 효과가 매우 작을것으로
            //  생각된다.
            sLatchHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                            sBCB->mSpaceID,
                                                            sBCB->mPageID );
            sBCB->lockBCBMutex( aStatistics );
            sBCB->decFixCnt();

            if( ( sBCB->mTouchCnt <= 1 )             &&
                ( sBCB->mFixCnt   == 0 )             &&
                ( sBCB->mState    == SDB_BCB_CLEAN ) &&
                ( aCachePage      == ID_FALSE ) )
            {
                //BUGBUG: 위에서 hash x chains latch를 먼저 잡는데,
                // 이부분에서 잡았다 풀어도 충분합니다.
                mHashTable.removeBCB( sBCB );

                mHashTable.unlockHashChainsLatch( sLatchHandle );
                sLatchHandle = NULL;

                /* 연결된 SBCB가 있다면 delink */
                sSBCB = sBCB->mSBCB;
                if( sSBCB != NULL )
                {
                    sSBCB->mBCB = NULL;
                    sBCB->mSBCB = NULL;
                }

                sBCB->setToFree();
                sBCB->unlockBCBMutex();

                aKey->addFreeBCB( sBCB );
            }
            else
            {
                mHashTable.unlockHashChainsLatch( sLatchHandle );
                sLatchHandle = NULL;
                sBCB->unlockBCBMutex();
                mLRUList[getLRUListIdx((UInt)sBCB->mPageID)].insertBCB2BehindMid(
                                                                        aStatistics,
                                                                        sBCB );
                // BUGBUG: 여기서 통계정보 절실히 필요.
                // 만약 여기서 hash Table에 삽입하는 BCB가 많다면, 그리고,
                // 그들이 한동안 접근되지 않을 BCB들이라면, 현재와 같은
                // 알고리즘( mFixCnt, mTouchCnt)로 판단하지 말아야 한다.
                // 그럼 어떡해? TouchTimeInterval까지 적용할 수 있고, 아니면,
                // mFixCnt만으로 판단 할 수도 있다.
            }
        }
        else
        {
            // 원래 상주해있던 버퍼이다.
            sBCB->lockBCBMutex(aStatistics);
            sBCB->decFixCnt();
            sBCB->unlockBCBMutex();
        }
    }

    aKey->mState = SDB_MPR_KEY_CLEAN;
}

/****************************************************************
 * Description :
 *  page ptr로 부터 BCB 를 찾고 페이지를 unfix한다..
 *  fixPage - unfixPage는 쌍으로 호출된다.
 *
 *  aStatistics - [IN]  통계정보
 *  aPagePtr    - [IN]  fixPage한 페이지
 ****************************************************************/
void sdbBufferPool::unfixPage( idvSQL     *aStatistics,
                               UChar      *aPagePtr )
{
    sdbBCB *sBCB;
    UChar  *sPageBeginPtr = smLayerCallback::getPageStartPtr( aPagePtr );

    sBCB = (sdbBCB*)((sdbFrameHdr*)sPageBeginPtr)->mBCBPtr;
    unfixPage(aStatistics, sBCB);
}

/****************************************************************
 * Description :
 *  aBCB가 가리키는 페이지를 Dirty로 등록시킨다.
 *
 * 주의! 이 함수는
 *       반드시 페이지에 대한 첫번째 로그를 기록하기 전에
 *       호출되어야 한다. 그렇지 않으면 페이지의 로그가 디스크에
 *       내려간후 아직 Dirty로 등록되지 않은상태에서 flush
 *       가 발생한다면 페이지의 로그 이후로 Restart Redo Point가
 *       결정될 수 있다.
 *
 *       이 함수가 호출되는 시점에는 실제 페이지에 대한 write 연산을
 *       모두 수행하였고, 그것에 해당하는 로그를 모두 aMtx가 가지고
 *       있는 상태이다. 그 로그들은 이 함수 수행 후에 실제 LSN을
 *       따서 쓰여지고, 그리고 나서 releasePage가 호출된다.
 *
 * Related Issue:
 *       BUG-19122 Restart Recovery LSN이 잘못 설정될수 있습니다.
 *
 * aStatistics - [IN]  통계정보
 * aBCB        - [IN]  Dirty를 등록시키고자 하는 BCB Pointer
 * aMtx        - [IN]  Mini Transaction Pointer
 *
 ****************************************************************/
void sdbBufferPool::setDirtyPage( idvSQL *aStatistics,
                                  void   *aBCB,
                                  void   *aMtx )
{
    sdbBCB     *sBCB;
    smLSN       sLstLSN;
    smLSN       sEndLSN;
    idBool      sIsMtxNoLogging;
    sdbBCBState sBCBState;

    sBCB = (sdbBCB*) aBCB;
    IDE_ASSERT( isFixedBCB(sBCB) == ID_TRUE );
    IDE_ASSERT( sBCB->mState    != SDB_BCB_FREE );

    /* BCB의 mMutex를 잡지 않은 상태이기 때문에, sBCB의 State가
     * 변할 수 있다. 만약 변수를 쓰지 않는다면, 아래의 if문에서
     * 제대로된 검사를 할 수 없게 된다. */
    sBCBState = sBCB->mState;

    if( (sBCBState == SDB_BCB_DIRTY) ||
        (sBCBState == SDB_BCB_REDIRTY ))
    {
        /*
         * 이 함수는 반드시 페이지 x래치를 잡은 상태로 들어온다.
         * 즉, 현재 sBCB의 상태가 dirty또는 redirty라면,
         * 이들의 상태는 절대 변하지 않는다.
         * 왜냐면, dirty또는 redirty를 iniob또는 clean으로 변경하기
         * 위해선 플러셔가 현재 페이지를 접근해야 하는데,
         * 페이지 x래치가 잡혀 있으므로, 접근할 수 없다.
         */
        sBCBState = sBCB->mState;
        IDE_ASSERT( (sBCBState == SDB_BCB_DIRTY) ||
                    (sBCBState == SDB_BCB_REDIRTY));
    }
    else
    {
        IDU_FIT_POINT( "1.BUG-19122@sdbBufferPool::setDirtyPage" );

        /*BUG-27501 sdbBufferPool::setDirtyPage에서 TempPage를 Dirty시키려 할때,
         *aMtx가 Null인 경우가 고려되지 않음
         *
         *  일반 페이지의 경우 로깅이 있는지 없는지를 바탕으로 Dirty할지 안할지를
         * 결정합니다. 하지만 TempPage의 경우 어차피 로그를 기록하지 않기 때문에
         * 무조건 Dirty시켜 Flush할 수 있도록 해야 합니다. */
        if ( smLayerCallback::isTempTableSpace( sBCB->mSpaceID ) )
        {
            /* Temporal Page이기 때문에 기록된 로그가 없고 redo가
             * 발생하지 않는다.
             */
            SM_LSN_INIT( sBCB->mRecoveryLSN );

            setDirtyBCB(aStatistics, sBCB);

            IDE_CONT( RETURN_SUCCESS );
        }

        /*
         * PROJ-1704 MVCC Renewal
         * Mini Transaction을 사용하지 않는 경우는 End LSN을 얻을수가 없다.
         * 따라서, Disk Last LSN을 Page LSN으로 설정한다.
         */
        if( aMtx == NULL )
        {
            smLayerCallback::getLstLSN( &sLstLSN );
            IDE_ASSERT( !SM_IS_LSN_INIT( sLstLSN ));

            if( SM_IS_LSN_INIT( sBCB->mRecoveryLSN) )
            {
                sBCB->mRecoveryLSN = sLstLSN;
            }
            sdbBCB::setPageLSN( (sdbFrameHdr*)sBCB->mFrame, sLstLSN);

            setDirtyBCB(aStatistics, sBCB);
            IDE_CONT( RETURN_SUCCESS );
        }

        sIsMtxNoLogging = smLayerCallback::isMtxModeNoLogging( aMtx );

        if(sIsMtxNoLogging == ID_FALSE )
        {
            /* logging mode */
            /* aMtx는 자신이 생성한 로그를 실제 write하지 않고,
             * 자신의 버퍼에 가지고 있는다.
             * */
            if ( smLayerCallback::isLogWritten( aMtx ) == ID_TRUE )
            {
                if ( SM_IS_LSN_INIT( sBCB->mRecoveryLSN) )
                {
                    smLayerCallback::getLstLSN( &(sBCB->mRecoveryLSN) );
                }

                aStatistics = (idvSQL*)smLayerCallback::getStatSQL( aMtx );

                setDirtyBCB(aStatistics, sBCB);
            }
            else
            {
                /*
                 * 실제로 로그를 생성하지 않았다면, 즉, 실제 wirte가 되지
                 * 않았다면, 굳이 dirty로 만들지 않는다.
                 * */
            }
        }
        else
        {
            sEndLSN = smLayerCallback::getMtxEndLSN( aMtx );

            /* bugbug: loggging mode인 경우엔 아무리 setDirtyPage를 호출하였다
             * 하더라도, 실제 dirty가 되었는지 여부를 알 수 있다. 즉, 자신이
             * 기록한 로그가 존재하는지 여부로 이것을 알 수 있다.
             * 하지만, no logging mode인 페이지의 경우엔 그것을 알 수 없다.
             * 왜냐면 로그가 존재하지 않기 때문이다.
             * 즉, setDirtyPage를 호출한 temp page의 경우엔, 그것이 실제
             * write 가 되었는지 여부에 상관없이 무조건 flush대상이 된다.
             */
            if ( smLayerCallback::isLSNZero( &sEndLSN ) == ID_TRUE )
            {
                // restart redo가 아닌 경우
                /* NOLOGGING mode */
                /* NOLOGGING mode의 release page시에 temp page가 아니더라도
                 * checkpoint list에 연결해야 할 경우가 있음.
                 * NOLOGGING mode로 index를 build할 때, 생성된 index page들을
                 * checkpoint list에 연결한다.
                 *
                 * 주의! index build중에 disk로 쫓겨난 index page의 BCB들은
                 * First modified LSN과 last modified LSN이 '0'이 되기 때문에
                 * system의 최신 LSN을 이용해서 BCB의 LSN을 수정한 후
                 * buffer checkpoint list에 연결한다.*/
                if ( smLayerCallback::isMtxNologgingPersistent( aMtx )
                     == ID_TRUE )
                {
                    smLayerCallback::getLstLSN( &sLstLSN );
                    IDE_ASSERT( !SM_IS_LSN_INIT( sLstLSN ));

                    /* no logging이지만 현재 disk에 저장된 내용은 persistent해야
                     * 한다. 즉, recovery redo에 의해서 내용이 없어지지 않도록
                     * 보장해야 한다. 그렇기 때문에 pageLSN과 mRecoveryLSN을
                     * 설정한다. */
                    if( SM_IS_LSN_INIT( sBCB->mRecoveryLSN) )
                    {                    
                        sBCB->mRecoveryLSN = sLstLSN;
                    }

                    sdbBCB::setPageLSN( (sdbFrameHdr*)sBCB->mFrame, sLstLSN);

                    /* NOLOGGING mini-transaction이 update한 page의 persistency
                     * 에는 2가지 option이 있다.
                     * NOLOGGING index build의 경우 persistent해야 하고
                     * 그외 다른 transaction의 경우에 대해서는 persistency를
                     * 요구하지 않는다.*/
                    IDE_ASSERT( smLayerCallback::isPageIndexOrIndexMetaType( sBCB->mFrame )
                                == ID_TRUE );
                }
                else
                {
                    /* 기록된 로그가 없고 redo도 발생하지 않았다면 TempPage인데
                     * TempPage일 경우, 위쪽에서 걸러지기 때문에 이곳으로 올 수
                     * 없다. 따라서 이곳으로 오는 경우는 비정상적인 경우이다.*/

                    /* BUG-24703 인덱스 페이지를 NOLOGGING으로 X-LATCH를 획득하는
                     * 경우가 있습니다. */
                    (void)sdbBCB::dump(sBCB);
                    IDE_ASSERT( 0 );
                }
            }
            else
            {
                /* recovery redo시에 이곳으로 옴
                 * bug-19742관련하여..
                 * 이곳에서 sBCB->mRecoveryLSN을 설정한다면
                 * redo, undo후에 flush ALL을 하지 않아도된다.
                 */
                
                if( SM_IS_LSN_INIT( sBCB->mRecoveryLSN) )
                {                    
                    sBCB->mRecoveryLSN = sEndLSN;
                }
            }

            /* no logging mode에서 setDirtyPage를 수행하면 모두 flush를
             * 수행해야한다. */
            aStatistics = (idvSQL*)smLayerCallback::getStatSQL( aMtx );

            setDirtyBCB(aStatistics, sBCB);
        }
    }
    
    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;
#endif
}


/****************************************************************
 * Description :
 *  BCB를 버퍼내에서 제거한다.  BCB를 제거한다는 것은 것은 곧 해시
 *  테이블에서 제거한다는 것이다.  이것은 곧 상태가 free가 됨을
 *  뜻한다. 이때, 해당 BCB가 dirty가 되어있다면, flush를 시키지 않고
 *  free 상태로 만든다.
 *
 *  discardBCB는 보통, tableSpace drop또는 table drop와 같은 연산에 쓰인다.
 *  보통, 이런 경우에 discardBCB의 조건이 된 BCB는 다시 접근 되지 않아야 함을
 *  알 수 있다. 왜냐? drop된 table에 왜 접근을 하는가? 이미 버려진 정보인데..
 *  이것을 상위에서 보장해야 하는데, 이것에 대한 체크를 친절하게도
 *  버퍼 매니저에서 해준다.(assert문 참고)
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  discard하고자 하는 BCB,
 *  aFilter     - [IN]  discard조건을 검사하기 위한 함수.
 *  aObj        - [IN]  discard조건을 검사할때, aFilter함수에 건네주는
 *                      데이터.
 *  aIsWait     - [IN]  sBCB의 상태가 INIOB또는 REDIRTY인 경우에
 *                      플러시가 끝나길 기다릴지 여부.
 *  aIsSuccess  - [OUT] discardBCB 성공여부를 리턴.
 *  aMakeFree   - [OUT] discardBCB가 직접 aBCB를 SDB_FREE_BCB로 만들었는지
 *                      여부를 리턴한다.
 ****************************************************************/
void sdbBufferPool::discardBCB( idvSQL                 *aStatistics,
                                sdbBCB                 *aBCB,
                                sdbFiltFunc             aFilter,
                                void                   *aObj,
                                idBool                  aIsWait,
                                idBool                 *aIsSuccess,
                                idBool                 *aMakeFree)
{
    idBool      sFiltOK;
    sdsBCB   * sSBCB;

    *aMakeFree = ID_FALSE;
    /* 아래 함수를 수행해서 해당 BCB를 SDB_BCB_CLEAN으로 만든다.
     * invalidateDirty를 수행하고 난 이후에 다시 dirty가 될 수 없음을
     * 상위에서 보장해야 한다.*/
    invalidateDirtyOfBCB( aStatistics,
                          aBCB,
                          aFilter,
                          aObj,
                          aIsWait,
                          aIsSuccess,
                          &sFiltOK); //aOK

    if( *aIsSuccess == ID_TRUE )
    {
        if( sFiltOK == ID_FALSE )
        {
            // aFiltOK가 ID_TRUE라는 것은 Filter조건에 맞지 않는다는 것이다.
            // 만약 여기서 assert에 걸린다면, 상위에서 보장해 주지 못한것이다.
            // 즉, 처음에는 discardBCB의 조건이 아니었는데,
            // 이 함수가 수행된 이후 discardBCB의 조건을 만족하지 못했던 BCB가
            // discardBCB의 조건을 만족하게 된것이다.
            // 만약 상위에서 이러한 현상을 의도했거나, 이런 현상이문제가
            // 안될경우에는 아래 assert문은 제거해도 상관없다.
            IDE_DASSERT( aFilter( aBCB, aObj) == ID_FALSE);
        }
        else
        {
#ifdef DEBUG
            // 이 부분 또한, discardBCB의 조건을 만족하는 BCB가
            // discardBCB수행이 된 이후에 접근이 된 경우에 아래 부분에서
            // 죽을 수 있다.
            // 만약 상위에서 이러한 현상을 의도했거나, 이런 현상이문제가
            // 안될경우에는 아래 assert문은 제거해도 상관없다.
            // Secondary Buffer를 "ALL page"로 on 시켜 놓으면
            // discardBCB를 만족하는  CLEAN 상태의 BCB가
            // discardBCB 수행후에 IOB가 들어갈수 있기에 예외허용.
            aBCB->lockBCBMutex(aStatistics);
            IDE_DASSERT( (aBCB->mState == SDB_BCB_CLEAN) ||
                         (aBCB->mState == SDB_BCB_INIOB) ||
                         (aBCB->isFree() == ID_TRUE ));
            aBCB->unlockBCBMutex();
#endif
            if ( aBCB->mState == SDB_BCB_CLEAN )
            {
                //연결된 SBCB가 있다면 delink
                sSBCB = aBCB->mSBCB;
                if ( sSBCB != NULL )
                {
                    /* delink 작업이 lock없이 수행되므로
                       victim 발생으로 인한 상태 변경이 있을수 있다.
                       pageID 등이 다르다면 이미 free 된 상황일수 있다. */
                    if ( (aBCB->mSpaceID == sSBCB->mSpaceID ) &&
                         (aBCB->mPageID == sSBCB->mPageID ) )
                    {
                        (void)sdsBufferMgr::removeBCB( aStatistics,
                                                       sSBCB );
                    }
                    else 
                    {
                        /* 여기서 sSBCB 를 삭제하는 이유는
                           sSBCB에 있는 discard된 페이지를 free 하기 위함인데
                           pageID 등이 다르다면 (victim 등으로 인한) 다른 페이지이므로
                           삭제대상이 아니다. */
                    }
                    aBCB->mSBCB = NULL;
                }
                else
                {
                    /* nothig to do */
                }
                *aMakeFree = makeFreeBCB( aStatistics, aBCB, aFilter, aObj );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    return;
}


/****************************************************************
 * Description :
 *  CLEAN 상태이면서 fix되어 있지 않고, 트랜잭션들이 접근하지 않는 BCB를
 *  free 상태로 변경한다. free상태란 hash에 없는 BCB를 의미한다.
 *  즉, BCB를 버퍼에서 제거하는 함수이다.
 *  aBCB에 대해 makeFreeBCB를 수행했는지 여부를 리턴한다. 즉,
 *  makeFreeBCB를 수행하여 SDB_FREE_BCB로 만들었다면, return ID_TRUE,
 *  makeFreeBCB를 수행하지 않았다면, return ID_FALSE
 *
 *  aStatistics     - [IN]  통계정보
 *  aBCB            - [IN]  free로 만들 BCB
 *  aFilter, aObj   - [IN]  BCB를 free로 만들지 말지 결정하기 위한 함수와
 *                          필요한 argument
 ****************************************************************/
idBool sdbBufferPool::makeFreeBCB(idvSQL     *aStatistics,
                                  sdbBCB     *aBCB,
                                  sdbFiltFunc aFilter,
                                  void       *aObj)
{
    scSpaceID    sSpaceID = aBCB->mSpaceID;
    scPageID     sPageID  = aBCB->mPageID;
    sdbHashChainsLatchHandle *sHashChainsHandle=NULL;
    idBool       sRet     = ID_FALSE;

    // 이 함수는 tablespace offline, alter system flush buffer_pool 명령에
    // 의해 수행될 수 있다.
    // 이 함수의 대상이 되는 BCB가 동시에 접근이 가능하기 때문에
    // fix가 될 수도 있고, hot이 될 수도 있는 등 여러가지 상황을 고려해야 한다.

    // fix되어있는 BCB가 makeFree대상이 되어선 절대로 안된다.
    // 즉, 현재 버퍼에서 접근되지 않거나, 이미 접근이 끝난
    // 페이지에 대해서만 makeFree를 호출한다.
    if( isFixedBCB(aBCB) == ID_TRUE )
    {
        // nothing to do
    }
    else
    {
        /* BCB가 Free상태라는것은 곧 hash에서 제거되어 있는 상태임을 뜻한다.
         * BCB를 Free상태로 만들기 위해 hash에서 제거한다.
         */
        sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                             sSpaceID,
                                                             sPageID );

        if( aBCB->isFree() == ID_FALSE )
        {
            /* BCB상태 변경을 위해서 필수적으로 BCBMutex를 잡아야 한다.*/
            aBCB->lockBCBMutex( aStatistics );

            if( (aFilter( aBCB, aObj) == ID_TRUE) &&
                (aBCB->mState  == SDB_BCB_CLEAN) &&
                (isFixedBCB(aBCB) == ID_FALSE) )
            {
                mHashTable.removeBCB( aBCB );
                /* BCB를 free상태로 만들되, 이들을 리스트에서 제거하지는 않는다.
                 * 이들은 결국 Victim으로 선정되어 새삶을 살게 될 것이다.*/
                aBCB->makeFreeExceptListItem();
                sRet = ID_TRUE;
            }

            aBCB->unlockBCBMutex();
        }
        mHashTable.unlockHashChainsLatch( sHashChainsHandle );
        sHashChainsHandle = NULL;
    }
    return sRet;
}

/****************************************************************
 * Description :
 *  SDB_BCB_DIRTY 상태이고, aFilter조건에 맞는다면 aBCB를
 *  SDB_BCB_DIRTY 상태에서 SDB_BCB_CLEAN상태로 변경한다.
 *  만약 SDB_BCB_REDIRTY 또는 SDB_BCB_INIOB 상태라면,
 *  aIsWait에 따라 기다릴지 여부를 선택한다. 이때는 invalidateDirty를
 *  바로 적용할 수 없다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  invalidateDirty를 적용할 BCB
 *  aFilter,aObj- [IN]  조건 검사를 위한 파라미터
 *  aIsWait     - [IN]  aBCB의 상태가 SDB_BCB_REDIRTY 또는 SDB_BCB_INIOB
 *                      일때, 상태가 변경되기를 기다릴지 말지 여부
 *  aIsSuccess  - [OUT] aIsWait가 ID_FALSE일때, aBCB의 상태가 SDB_BCB_REDIRTY
 *                      또는 SDB_BCB_INIOB 인 경우 ID_FALSE를 리턴한다.
 *  aIsFilterOK - [OUT] aFilter에 만족하는지 여부 리턴
 *
 ****************************************************************/
void sdbBufferPool::invalidateDirtyOfBCB( idvSQL      *aStatistics,
                                          sdbBCB      *aBCB,
                                          sdbFiltFunc  aFilter,
                                          void        *aObj,
                                          idBool       aIsWait,
                                          idBool      *aIsSuccess,
                                          idBool      *aIsFilterOK )
{
    *aIsSuccess  = ID_FALSE;
    *aIsFilterOK = ID_FALSE;
retry:
    /* 여기서 BCBMutex를 잡는 이유는 상태를 변경시키지 않게 하기 위해서이다.
     * aBCB가 dirty이고 mutex를 잡힌 상태에서는 다른 pid로 설정되지도 못하고,
     * 상태가 변경되지도 못한다.
     */
    aBCB->lockBCBMutex( aStatistics );

    if ( isFixedBCB(aBCB) == ID_TRUE )
    {
        // BUG-20667 테이블 스페이스를 drop중인데, 그 테이블 스페이스에
        // 접근하는 쓰레드가 존재합니다.
        ideLog::log(SM_TRC_LOG_LEVEL_BUFFER,
                    SM_TRC_BUFFER_POOL_FIXEDBCB_DURING_INVALIDATE,
                    (aFilter( aBCB, aObj ) == ID_TRUE ));
        (void)sdbBCB::dump(aBCB);
    }
    else
    {
        /* nothing to do */
    }

    /* 해당 aBCB가 다른 것으로 변했는지 여부 검사.
     * 일단 aBCB Mutex를 잡으면 절대로 다른 PageID로 변경되지 않는다.*/
    if ( aFilter( aBCB, aObj) == ID_FALSE )
    {
        *aIsFilterOK = ID_FALSE;

        aBCB->unlockBCBMutex();
        *aIsSuccess = ID_TRUE;
    }
    else
    {
        *aIsFilterOK = ID_TRUE;

        if ( ( aBCB->mState == SDB_BCB_CLEAN ) ||
             ( aBCB->mState == SDB_BCB_FREE  ) )
        {
            aBCB->unlockBCBMutex();
            *aIsSuccess = ID_TRUE;
        }
        else
        {
            if ( (aBCB->mState == SDB_BCB_INIOB ) ||
                 (aBCB->mState == SDB_BCB_REDIRTY) ||
                 (isFixedBCB(aBCB) == ID_TRUE ))
            {
                aBCB->unlockBCBMutex();

                if ( aIsWait == ID_TRUE )
                {
                    idlOS::sleep( mInvalidateWaitTime );
                    goto retry;
                }
                else
                {
                    *aIsSuccess = ID_FALSE;
                }
            }
            else
            {
                IDE_ASSERT( aBCB->mState == SDB_BCB_DIRTY );
                IDE_ASSERT( aFilter( aBCB, aObj ) == ID_TRUE );

                // dirty상태이기 mMutex를 잡고 있기 때문에 절대로 상태가 변하지
                // 않음을 보장할 수 있다.
                /* dirty인것은 먼저 checkpoint list에서 제거를 하는데,
                 * 이때, BCB의 mMutex를 풀지 않는다.  왜냐면, mMutex를 푸는 순간,
                 * 다른 상태로 변경될 수 있기 때문이다.
                 * 현재 상태가 dirty이라면 이것은 아직 flusher가 보지 않았다는
                 * 뜻이 된다. flusher는 aBCB의 상태를 먼저 본후에 BCB Mutex를 잡고
                 * 다시 상태를 확인하기 때문이다.
                 *
                 * 그러므로 마음놓고 제거할 수 있다.
                 **/
                if (!SM_IS_LSN_INIT(aBCB->mRecoveryLSN))
                {
                    // recovery LSN이 0이란 말은 temp page이다.
                    // temp page는 checkpoint list에 달리지 않는다.
                    mCPListSet.remove( aStatistics, aBCB );
                }

                aBCB->clearDirty();

                aBCB->unlockBCBMutex();

                *aIsSuccess = ID_TRUE;
            }
        }
    }

    return;
}

/****************************************************************
 * Description :
 *  bufferPool에 BCB를 삽입한다.
 *  prepare List중 길이가 가장 짧은 리스트에 BCB삽입
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 ****************************************************************/
IDE_RC sdbBufferPool::addBCB2PrepareLst(idvSQL *aStatistics,
                                        sdbBCB *aBCB)
{
    UInt            i;
    UInt            sMinLength  = ID_UINT_MAX;
    UInt            sLength;
    sdbPrepareList *sList;
    sdbPrepareList *sTargetList = NULL;
    static UInt     sStartPos   = 0;
    UInt            sListIndex;

    if (sStartPos >= mPrepareListCnt)
    {
        sStartPos = 0;
    }
    else
    {
        sStartPos++;
    }

    // sStartPos는 static 변수이기 때문에 여러 스레드가 동시 접근, 증가시킬 수
    // 있기 때문에 mPrepareListCnt보다 큰 상황도 발생할 수 있어서
    // mod 연산으로 그럴 가능성을 배제한다.

    // BUG-21289
    // 또한 HP등 일부 장비에서 % 연산을 하는 도중 갱신된 sStartPos를 읽어
    // %의 연산결과가 피젯수인 mPrepareListCnt보다 큰 경우가 발생한 사례(case-13531)이 있다.
    // 이를 방지하기 위해 while문으로 원천 차단한다.

    sListIndex = sStartPos;
    while (sListIndex >= mPrepareListCnt)
    {
        sListIndex = sListIndex % mPrepareListCnt;
    }

    for (i = 0; i < mPrepareListCnt; i++)
    {
        sList = &mPrepareList[sListIndex];
        if (sListIndex == mPrepareListCnt - 1)
        {
            sListIndex = 0;
        }
        else
        {
            sListIndex++;
        }

        if (sList->getWaitingClientCount() > 0)
        {
            sTargetList = sList;
            break;
        }

        sLength = sList->getLength();

        if (sMinLength > sLength)
        {
            sMinLength = sLength;
            sTargetList = sList;
        }
    }

    IDE_ASSERT(sTargetList != NULL);
    // prepare List중 길이가 가장 짧은 리스트에 BCB삽입
    IDE_TEST(sTargetList->addBCBList(aStatistics,
                                     aBCB,
                                     aBCB,
                                     1)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  aBCB가 속해있는 리스트에서 BCB를 제거한다.
 *  현재는 LRUList에서만 제거가 가능하고, LRUList가 아닌 리스트에서 제거를
 *  시도하는 경우엔 제거가 실패하고, ID_FALSE를 리턴한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 ****************************************************************/
idBool sdbBufferPool::removeBCBFromList(idvSQL *aStatistics, sdbBCB *aBCB)
{
    idBool sRet;
    UInt   sLstIdx;

retry:
    sRet     = ID_FALSE;
    sLstIdx  = aBCB->mBCBListNo;

    switch( aBCB->mBCBListType )
    {
        case SDB_BCB_LRU_HOT:
        case SDB_BCB_LRU_COLD:
            if( sLstIdx >= mLRUListCnt )
            {
                /* BCB가 처음에 호출되었을때와 달라졌다.
                 * 다시시도한다. segv 에러방지*/
                goto retry;
            }

            if( mLRUList[sLstIdx].removeBCB( aStatistics, aBCB) == ID_FALSE )
            {
                /* BCB가 처음에 호출되었을때와 달라졌다.
                 * 다시시도한다.*/
                goto retry;
            }
            /* [BUG-20630] alter system flush buffer_pool을 수행하였을때,
             * hot영역은 freeBCB들로 가득차 있습니다.
             * 성공적으로 제거를 수행하였다.
             * */
            sRet = ID_TRUE;
            break;

        case SDB_BCB_PREPARE_LIST:
        case SDB_BCB_FLUSH_LIST:
        case SDB_BCB_DELAYED_FLUSH_LIST:
        default:
            break;
    }
    return sRet;
}
#ifdef DEBUG
IDE_RC sdbBufferPool::validate( sdbBCB *aBCB )
{
    IDE_DASSERT( aBCB != NULL );

    sdpPhyPageHdr   * sPhyPageHdr;
    smcTableHeader  * sTableHeader;
    smOID             sTableOID;

    sPhyPageHdr = sdpPhyPage::getHdr(aBCB->mFrame);

    if( sdpPhyPage::getPageType( sPhyPageHdr ) == SDP_PAGE_DATA )
    {
        if( sPhyPageHdr->mPageID != aBCB->mPageID )
        {
            ideLog::log( IDE_ERR_0,
                     "----------------- Page Dump -----------------\n"
                     "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                     "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                     "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                     "Total Free Size           : %"ID_UINT32_FMT"\n"
                     "Available Free Size       : %"ID_UINT32_FMT"\n"
                     "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                     "Size of CTL               : %"ID_UINT32_FMT"\n"
                     "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                     "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                     "Page Type                 : %"ID_UINT32_FMT"\n"
                     "Page State                : %"ID_UINT32_FMT"\n"
                     "Page ID                   : %"ID_UINT32_FMT"\n"
                     "is Consistent             : %"ID_UINT32_FMT"\n"
                     "LinkState                 : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                     "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "TableOID                  : %"ID_vULONG_FMT"\n"
                     "Seq No                    : %"ID_UINT64_FMT"\n"
                     "---------------------------------------------\n",
                     (UChar*)sPhyPageHdr,
                     sPhyPageHdr->mFrameHdr.mCheckSum,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                     sPhyPageHdr->mFrameHdr.mIndexSMONo,
                     sPhyPageHdr->mFrameHdr.mSpaceID,
                     sPhyPageHdr->mFrameHdr.mBCBPtr,
                     sPhyPageHdr->mTotalFreeSize,
                     sPhyPageHdr->mAvailableFreeSize,
                     sPhyPageHdr->mLogicalHdrSize,
                     sPhyPageHdr->mSizeOfCTL,
                     sPhyPageHdr->mFreeSpaceBeginOffset,
                     sPhyPageHdr->mFreeSpaceEndOffset,
                     sPhyPageHdr->mPageType,
                     sPhyPageHdr->mPageState,
                     sPhyPageHdr->mPageID,
                     sPhyPageHdr->mIsConsistent,
                     sPhyPageHdr->mLinkState,
                     sPhyPageHdr->mParentInfo.mParentPID,
                     sPhyPageHdr->mParentInfo.mIdxInParent,
                     sPhyPageHdr->mListNode.mPrev,
                     sPhyPageHdr->mListNode.mNext,
                     sPhyPageHdr->mTableOID,
                     sPhyPageHdr->mSeqNo );
            IDE_DASSERT( 0 );
        }

        sTableOID = sdpPhyPage::getTableOID( (UChar*)sdpPhyPage::getHdr( aBCB->mFrame ));

        (void)smcTable::getTableHeaderFromOID( sTableOID,
                                               (void**)&sTableHeader );
        if( sTableOID != sTableHeader->mSelfOID )
        {
            ideLog::log( IDE_ERR_0,
                     "----------------- Page Dump -----------------\n"
                     "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                     "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                     "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                     "Total Free Size           : %"ID_UINT32_FMT"\n"
                     "Available Free Size       : %"ID_UINT32_FMT"\n"
                     "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                     "Size of CTL               : %"ID_UINT32_FMT"\n"
                     "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                     "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                     "Page Type                 : %"ID_UINT32_FMT"\n"
                     "Page State                : %"ID_UINT32_FMT"\n"
                     "Page ID                   : %"ID_UINT32_FMT"\n"
                     "is Consistent             : %"ID_UINT32_FMT"\n"
                     "LinkState                 : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                     "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "TableOID                  : %"ID_vULONG_FMT"\n"
                     "Seq No                    : %"ID_UINT64_FMT"\n"
                     "---------------------------------------------\n",
                     (UChar*)sPhyPageHdr,
                     sPhyPageHdr->mFrameHdr.mCheckSum,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                     sPhyPageHdr->mFrameHdr.mIndexSMONo,
                     sPhyPageHdr->mFrameHdr.mSpaceID,
                     sPhyPageHdr->mFrameHdr.mBCBPtr,
                     sPhyPageHdr->mTotalFreeSize,
                     sPhyPageHdr->mAvailableFreeSize,
                     sPhyPageHdr->mLogicalHdrSize,
                     sPhyPageHdr->mSizeOfCTL,
                     sPhyPageHdr->mFreeSpaceBeginOffset,
                     sPhyPageHdr->mFreeSpaceEndOffset,
                     sPhyPageHdr->mPageType,
                     sPhyPageHdr->mPageState,
                     sPhyPageHdr->mPageID,
                     sPhyPageHdr->mIsConsistent,
                     sPhyPageHdr->mLinkState,
                     sPhyPageHdr->mParentInfo.mParentPID,
                     sPhyPageHdr->mParentInfo.mIdxInParent,
                     sPhyPageHdr->mListNode.mPrev,
                     sPhyPageHdr->mListNode.mNext,
                     sPhyPageHdr->mTableOID,
                     sPhyPageHdr->mSeqNo );
            IDE_DASSERT( 0 );
        }
    }
    return IDE_SUCCESS;
}
#endif
