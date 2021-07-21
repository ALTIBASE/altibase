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
/***********************************************************************
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ***********************************************************************/

/***********************************************************************
 * Description : 버퍼풀에서 사용하는 LRU list를 구현한 파일이다.
 *               LRU list는 객체로 사용되며 동기화 제어는 내부적으로
 *               수행되기 때문에 따로 lock, unlock 인터페이스를
 *               제공하지 않는다.
 *               알고리즘은 HOT-COLD LRU 알고리즘을 사용한다.
 *
 *   + 제약사항
 *     - 초기화할 때 aHotMax는 최소한 1보다 커야한다.
 *       즉 hot 영역의 최대 크기는 최소한 1이거나 그보다 커야한다.
 *       기존 버퍼 알고리즘(단순 LRU)과 동일한 알고리즘을
 *       사용하기위해서는 aHotMax를 1로하면된다.
 *
 *   + 데이터 구조 및 알고리즘
 *      . mHotLength와 mColdLength는 뮤텍스를 잡은 동안엔 항상 정확한 정보를
 *          유지하고 있다.
 *      . hotBCB가 전혀 없는 경우엔 mMid는 mBase를 가리키고 있다.
 *      . hotBCB가 하나라도 있다면  mMid는 hotLast를 가리키고 있다.
 *      . 즉, mMid와 mBase가 같다는 것은 hot영역에 bcb가 전혀 없음을 의미한다.
 *      
 *      . cold로 삽입은 항상 mMid->mNext 로이루어 진다. 이때, mMid포인트의
 *          이동이 전혀 없음을 주목. 왜냐면 mMid포인트는 hotLast를 가리키기 때문.
 *      . Hot으로 삽입때에 만약 mHotMax까지 BCB가 차지 않았다면, 역시 mMid포인트
 *          이동이 전혀 없음을 주목. 왜냐면 mMid포인트는 hotLast를 가리키기 때문.
 *      . Hot으로 삽입때에 만약 mHotMax까지 BCB가 찼다면, mMid->mPrev로
 *          mMid포인터 이동
 *
 *      . cold 삽입 => mMid->mNext로 삽입
 *      . cold 삭제 => (if cold가 존재할때만) mBase->mPrev제거
 *      . hot 삽입  => (if hot이 없다면) mBase->mNext로 삽입; mMid포인터 변경
 *                    (if hot이 있다면) mBase->mNext로 삽입
 *
 *      . 리스트 중간에서 삭제 => 그냥 삭제하면 되지만, 만약 삭제 대상이 mMid라면
 *                              mMid포인터를 mMid->mPrev로 변경..
 *
 ***********************************************************************/

#include <sdbLRUList.h>

/* Hot영역에 있던 BCB를 Cold영역으로 옮겨올때 설정하는 정보 */
#define SDB_MAKE_BCB_COLD(node) {                                       \
        ((sdbBCB*)(node)->mData)->mTouchCnt    = 1;                     \
        ((sdbBCB*)(node)->mData)->mBCBListType = SDB_BCB_LRU_COLD;      \
    }


/***********************************************************************
 * Description :
 *  LRUList를 초기화 한다.
 * 
 *  aListID     - [IN]  리스트 ID
 *  aHotMax     - [IN]  리스트에서 hot이 차지할 수 있는 최대 갯수.
 *  aStat       - [IN]  버퍼풀 통계정보
 ***********************************************************************/
IDE_RC sdbLRUList::initialize(UInt               aListID,
                              UInt               aHotMax,
                              sdbBufferPoolStat *aStat)
{
    SChar sMutexName[128];
    SInt  sState = 0;

    // aHotMax는 1 이상이어야 한다.
    if (aHotMax == 0)
    {
        aHotMax = 1;
    }

    mID              = aListID;
    mBase            = &mBaseObj;
    mMid             = mBase;
    mHotLength       = 0;
    mHotMax          = aHotMax;
    mColdLength	     = 0;
    mStat            = aStat;

    SMU_LIST_INIT_BASE( mBase );

    idlOS::memset(sMutexName, 0, 128);
    idlOS::snprintf(sMutexName, 128, "LRU_LIST_MUTEX_%"ID_UINT32_FMT, aListID);

    IDE_TEST(mMutex.initialize(sMutexName,
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_LATCH_FREE_DRDB_LRU_LIST)
             != IDE_SUCCESS);
    sState = 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 1:
            IDE_ASSERT(mMutex.destroy() == IDE_SUCCESS);
        default:
            break;
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  제거함수
 ***********************************************************************/
IDE_RC sdbLRUList::destroy()
{
    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 *  ColdLast에서 BCB를 하나 리턴한다. cold영역에 BCB가 전혀 없다면 NULL을 리턴.
 *  
 *  aStatistics - [IN]  통계정보
 ****************************************************************/
sdbBCB* sdbLRUList::removeColdLast(idvSQL *aStatistics)
{
    sdbBCB  *sRet    = NULL;
    smuList *sTarget = NULL;

    if( mColdLength == 0 )
    {
        // 뮤텍스를 잡지 않았기 때문에 정확하지 않을 수 있다.
        sRet = NULL;
    }
    else
    {
        IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

        if( mColdLength != 0 )
        {
            sTarget = SMU_LIST_GET_PREV(mBase);
            
            SMU_LIST_DELETE(sTarget);

            sRet = (sdbBCB*)sTarget->mData;
            SDB_INIT_BCB_LIST(sRet);

            IDE_ASSERT( mColdLength != 0 );
            mColdLength--;
        }
        else
        {
            IDE_ASSERT( mBase->mPrev == mMid );
            sRet = NULL;
        }

        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);

    return sRet;
}




/****************************************************************
 * Description:
 *  Mid포인터를 Hot 방향으로 aMoveCnt만큼 이동시킨다.
 *  만약 aMoveCnt가 mHotLength보다 큰경우에는 값을 보정하여,
 *  mHotLength만큼만 옮긴다.
 *  
 *  aStatistics - [IN]  통계정보
 *  aMoveCnt    - [IN]  hot방향으로 이동하는 횟수
 ****************************************************************/
UInt sdbLRUList::moveMidPtrForward(idvSQL *aStatistics, UInt aMoveCnt)
{
    UInt sMoveCnt = aMoveCnt;
    UInt i;
    
    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);
    
    if( mHotLength < sMoveCnt )
    {
        sMoveCnt = mHotLength;
    }

    for( i = 0 ; i < sMoveCnt; i++)
    {
        SDB_MAKE_BCB_COLD(mMid);
        mMid = SMU_LIST_GET_PREV(mMid);
    }

    mColdLength += sMoveCnt;
    
    IDE_ASSERT(mHotLength >= sMoveCnt );
    mHotLength  -= sMoveCnt;
    
    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    
    return sMoveCnt;
}

/****************************************************************
 * Description:
 *  LRU의 Hot영역에 BCB를 한개 삽입한다.
 *  이때, 기존에 Hot영역에 BCB가 한개도 없다면, mBase와 mMid는 같고,
 *  삽입 후에 mMid를 삽입한 BCB로 설정한다. 왜냐면, mMid는 hotLast를 가리
 *  키기 때문이다.
 *  그렇지 않고 Hot영역에 BCB가 있다면, 그냥 mBase->mNext에 삽입하면 된다.
 *  만약 mHotMax를 넘을 경우엔 mMid가 가리키는 BCB를 cold로 만들고,
 *  mMid를 mMid->mPrev로 변경한다. 
 *  
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  해당 BCB
 ****************************************************************/
void sdbLRUList::addToHot(idvSQL *aStatistics, sdbBCB *aBCB)
{
    IDE_DASSERT(aBCB != NULL);

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    /* BUG-22550: mHotLength값을 Lock을 잡지않고 IDE_DASSERT로
     * 확인하고 있습니다.
     *
     * Lock을 잡은 상태에서 값을 확인해야 합니다.
     * */
    IDE_DASSERT(mHotLength <= mHotMax);

    aBCB->mBCBListType = SDB_BCB_LRU_HOT;
    aBCB->mBCBListNo   = mID;

    SMU_LIST_ADD_FIRST(mBase, &aBCB->mBCBListItem);
    mHotLength++;
    mStat->applyHotInsertions();

    /* mHotLength가 1인경우엔 mMid가 mBase를 가리키지 않고,
     * HotLast를 가리켜야 한다.
     */
    if( mHotLength == 1 )
    {
        IDE_DASSERT( mMid == mBase);
        mMid = &(aBCB->mBCBListItem);
    }

    if ( mHotLength > mHotMax )
    {
        // hot 영역이 가득찬 상태이다.
        // 이 경우 mMid를 당겨야 한다.
        SDB_MAKE_BCB_COLD(mMid);
        mMid = SMU_LIST_GET_PREV(mMid);
        mColdLength ++;
        mHotLength  --;
    }
    
    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS );
}

/****************************************************************
 * Description:
 *  mMid 뒤에 BCB를 한개 삽입한다. 
 *  
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  해당 BCB
 ****************************************************************/
void sdbLRUList::insertBCB2BehindMid(idvSQL *aStatistics, sdbBCB *aBCB)
{
    IDE_DASSERT(aBCB != NULL);
    
    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);
    
    aBCB->mTouchCnt    = 1;
    aBCB->mBCBListType = SDB_BCB_LRU_COLD;
    aBCB->mBCBListNo   = mID;

    mColdLength++;

    SMU_LIST_ADD_AFTER(mMid, &aBCB->mBCBListItem);
    
    mStat->applyColdInsertions();

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);
}

/****************************************************************
 * Description:
 *  LRU 리스트에 존재하는 aBCB를 리스트에서 삭제한다.
 *  만약 aBCB가 리스트에 존재하지 않는다면, ID_FALSE를 리턴한다.
 *  그외엔 ID_TRUE를 리턴.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  삭제 하고자 하는 BCB
 ****************************************************************/
idBool sdbLRUList::removeBCB(idvSQL *aStatistics, sdbBCB *aBCB )
{
    UInt    sListType;
    idBool  sRet;
    
    IDE_DASSERT(aBCB != NULL);
    
    sListType   = aBCB->mBCBListType;
    sRet        = ID_FALSE;    
    
    /* 기본적으로 뮤텍스 잡지 않고, 현재 list와 bcb의 정보가 동일한지 확인한다.
     * 그 이후에 다시 뮤텍스 잡고선 정확히 확인한다.*/
    if(((sListType == SDB_BCB_LRU_HOT) ||
        (sListType == SDB_BCB_LRU_COLD )) &&
       (aBCB->mBCBListNo == mID ))
    {
        IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

        if(((aBCB->mBCBListType == SDB_BCB_LRU_HOT) ||
            (aBCB->mBCBListType == SDB_BCB_LRU_COLD )) &&
           (aBCB->mBCBListNo == mID ))
        {
            /* aBCB가 mMid인 경우엔 mMid를 mPrev로 한칸 이동하고,
             * 그외엔 그냥 삭제만 하면 된다.*/
            if( aBCB->mBCBListType == SDB_BCB_LRU_HOT )
            {
                if( mMid == &(aBCB->mBCBListItem ))
                {
                    mMid = SMU_LIST_GET_PREV(mMid);
                }
                mHotLength--;
            }
            else
            {
                mColdLength--;
            }
            SMU_LIST_DELETE(&(aBCB->mBCBListItem));
            SDB_INIT_BCB_LIST(aBCB);

            sRet = ID_TRUE;
        }
        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
        IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);
    }

    return sRet;
}

/****************************************************************
 * Description:
 *  mHotMax값을 새로 설정한다.
 *  만약 새로 설정하는  mHotMax값이 기존의 mHotMax보다 크다면
 *  그냥 설정하면 되지만, 작다면 기존에 hot영역에 존재하는 BCB를
 *  cold영역으로 이동을 시켜야 한다.
 *  
 *  aStatistics - [IN]  통계정보
 *  aNewHotMax  - [IN]  새로 설정할 hotMax값. BCB 개수를 나타냄
 ****************************************************************/
void sdbLRUList::setHotMax(idvSQL *aStatistics, UInt aNewHotMax)
{
    if (aNewHotMax < mHotLength)
    {
        moveMidPtrForward( aStatistics, mHotLength - aNewHotMax);
    }
    mHotMax = aNewHotMax;
}

/* LRUList를 dump한다.*/
void sdbLRUList::dump()
{
    smuList *sListNode;

    UInt     sHotFixCnt[10];
    UInt     sHotTouchCnt[10];

    UInt     sColdFixCnt[10];
    UInt     sColdTouchCnt[10];

    UInt    i;
    sdbBCB   *sBCB;

    idlOS::memset( sHotFixCnt, 0x00, ID_SIZEOF(UInt) * 10 );
    idlOS::memset( sHotTouchCnt, 0x00, ID_SIZEOF(UInt) * 10 );
    idlOS::memset( sColdFixCnt, 0x00, ID_SIZEOF(UInt) * 10 );
    idlOS::memset( sColdTouchCnt, 0x00, ID_SIZEOF(UInt) * 10 );

    IDE_ASSERT(mMutex.lock(NULL) == IDE_SUCCESS);

    sListNode = SMU_LIST_GET_NEXT(mBase);
    for( i = 1 ; i < getHotLength(); i++)
    {
        sBCB = (sdbBCB*)sListNode->mData;

        if( sBCB->mFixCnt >= 10 )
        {
            sHotFixCnt[9]++;
        }
        else
        {
            sHotFixCnt[ sBCB->mFixCnt ]++;
        }
        if( sBCB->mTouchCnt >= 10)
        {
            sHotTouchCnt[9]++;
        }
        else
        {
            sHotTouchCnt[ sBCB->mTouchCnt]++;
        }
        sListNode = SMU_LIST_GET_NEXT(sListNode);
    }

    for( i = 0 ; i < getColdLength(); i++)
    {
        sBCB = (sdbBCB*)sListNode->mData;

        if( sBCB->mFixCnt >= 10 )
        {
            sColdFixCnt[9]++;
        }
        else
        {
            sColdFixCnt[ sBCB->mFixCnt ]++;
        }
        if( sBCB->mTouchCnt >= 10)
        {
            sColdTouchCnt[9]++;
        }
        else
        {
            sColdTouchCnt[ sBCB->mTouchCnt]++;
        }
        sListNode = SMU_LIST_GET_NEXT(sListNode);
    }

    idlOS::printf("   hotfix\t hottouch\t coldfix\t coldtouch\n");
    for( i = 0 ; i < 10 ; i++ )
    {
        idlOS::printf("[%d] ",i);
        idlOS::printf( "%d\t\t", sHotFixCnt[i]);
        idlOS::printf( "%d\t\t", sHotTouchCnt[i]);
        idlOS::printf( "%d\t\t", sColdFixCnt[i]);
        idlOS::printf( "%d\n", sColdTouchCnt[i]);
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

}


/***********************************************************************
 * Description :
 *  sdbLRUList의 모든 BCB들이 제대로 연결되어 있는지 확인한다.
 *  
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
IDE_RC sdbLRUList::checkValidation(idvSQL *aStatistics)
{
    smuList *sPrevNode;
    smuList *sListNode;
    sdbBCB  *sBCB;
    UInt     i;

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    /* hot 갯수 확인*/
    sListNode = mBase;
    sPrevNode = SMU_LIST_GET_PREV( sListNode );
    for( i = 0 ; i < mHotLength; i++)
    {
        IDE_ASSERT( sPrevNode == SMU_LIST_GET_PREV( sListNode ));
        sPrevNode = sListNode;
        sListNode = SMU_LIST_GET_NEXT( sListNode);
        sBCB = (sdbBCB*)(sListNode->mData);
        IDE_ASSERT( sBCB->mBCBListType == SDB_BCB_LRU_HOT);
    }

    //hotLast는 mMid 와 같다.
    IDE_ASSERT(sListNode == mMid);

    if( mHotLength != 0 )
    {
        sBCB = (sdbBCB*)(sListNode->mData);
        IDE_ASSERT( sBCB->mBCBListType == SDB_BCB_LRU_HOT);
    }
    else
    {
        IDE_ASSERT( mMid == mBase);
    }

    /* cold 갯수 확인*/
    sListNode = mMid;
    sPrevNode = SMU_LIST_GET_PREV( sListNode );
    for( i = 0 ; i < mColdLength; i++)
    {
        IDE_ASSERT( sPrevNode == SMU_LIST_GET_PREV( sListNode ));
        sPrevNode = sListNode;
        sListNode = SMU_LIST_GET_NEXT( sListNode);
        sBCB = (sdbBCB*)(sListNode->mData);
        IDE_ASSERT( sBCB->mBCBListType == SDB_BCB_LRU_COLD);
    }
    IDE_ASSERT(sListNode == SMU_LIST_GET_PREV( mBase));

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;
}
