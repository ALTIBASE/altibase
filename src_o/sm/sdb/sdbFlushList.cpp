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
 * Description :
 *    버퍼풀에서 사용하는 flush list를 구현한 파일이다.
 *    flush list는 LRU list에서 dirty된 BCB들이 victim을 찾는 과정에서
 *    옮겨지는 list이다.
 *
 ***********************************************************************/


#include <sdbFlushList.h>

/***********************************************************************
 * Description :
 *  flushList 초기화
 *
 *  aListID     - [IN]  flushList 식별자  
 ***********************************************************************/
IDE_RC sdbFlushList::initialize( UInt aListID, sdbBCBListType aListType )
{
    SChar sMutexName[128];

    mID            = aListID;
    mBase          = &mBaseObj;
    mExplorerCount = 0;
    mCurrent       = NULL;
    mListLength    = 0;

    /* PROJ-2669 */
    mListType       = aListType;
    mMaxListLength  = 0;

    SMU_LIST_INIT_BASE(mBase);

    idlOS::memset(sMutexName, 0, 128);
    idlOS::snprintf(sMutexName, 128, "FLUSH_LIST_MUTEX_%"ID_UINT32_FMT, aListID);

    IDE_TEST(mMutex.initialize(sMutexName,
			       IDU_MUTEX_KIND_NATIVE,
			       IDV_WAIT_INDEX_LATCH_FREE_DRDB_FLUSH_LIST)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbFlushList::destroy()
{

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *    flush list에 대해 mCurrent부터 탐색을 요청한다. 
 *    flush list의 BCB를 보기위해서는 이 함수의 호출이 선행되어야 한다.
 *    내부적으로 mExploringCount를 하나 증가시킨다.
 *    이 값이 1이상이라면 누군가 탐색하고 있다는 의미이다.
 *    탐색을 마쳤을 땐 endExploring()을 호출해야 한다.
 *    
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
void sdbFlushList::beginExploring(idvSQL *aStatistics)
{
    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    IDE_DASSERT(mExplorerCount >= 0);

    mExplorerCount++;
    if (mExplorerCount == 1)
    {
        // 이 flush list에 대해 beginExploring()을 아무도 호출안한
        // 상태이다. 이 경우 mExploring을 세팅한다.
        mCurrent = SMU_LIST_GET_LAST(mBase);
    }
    else
    {
        // 누군가 exploring을 하고 있다.
        // mExploring을 세팅하지 않는다.
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
}

/***********************************************************************
 * Description :
 *    flush list에 대한 탐색을 마쳤음을 알린다.
 *    beginExploring 호출했다면, 반드시 endExploring을 호출해야 한다.
 *    mExploringCount를 하나 감소시킨다.
 *
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
void sdbFlushList::endExploring(idvSQL *aStatistics)
{
    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    // endExploring()이 호출되기 전에 반드시 최소한 한번은
    // beginExploring()이 호출되어야 한다.
    IDE_DASSERT(mExplorerCount >= 1);

    mExplorerCount--;
    if (mExplorerCount == 0)
    {
        // 더이상 아무도 exploring하지 않는 상태이다.
        mCurrent = NULL;
    }
    else
    {
        // Nothing to do
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
}

/***********************************************************************
 * Description :
 *    현재 탐색 위치의 BCB를 얻어온다. BCB를 리스트에서 빼진 않는다.
 *    그리고 탐색 위치를 앞으로 하나 당겨 위치시킨다.
 *    현재 탐색 위치가 mFirst이면 NULL을 반환한다.
 *    리스트에 BCB가 하나도 없을 때도 NULL을 반환한다.
 *    이 함수를 호출하기 위해서는 먼저 beginExploring()을 호출해야 한다.
 *
 *    보통 getNext를 수행하여 얻어온 BCB에 대해서 remove를 적용해서 삭제하게
 *    되는데, 모든 BCB에 대해 remove를 적용하지 않고, 그냥 flushList에 남겨
 *    놓는 경우도 있다. 이때는 remove를 호출하지 않고, 다시 getNext를 호출한다.
 *    
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
sdbBCB* sdbFlushList::getNext(idvSQL *aStatistics)
{
    sdbBCB *sRet = NULL;

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    IDE_DASSERT(mExplorerCount > 0);

    if (mCurrent == mBase)
    {
        // sRet가 NULL이 라고 mListLength가 0을 뜻하진 않는다.
        sRet = NULL;
    }
    else
    {
        sRet = (sdbBCB*)mCurrent->mData;
        mCurrent = SMU_LIST_GET_PREV(mCurrent);
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return sRet;
}

/***********************************************************************
 * Description :
 *    리스트에서 해당 BCB를 제거한다.
 *    인자로 주어지는 BCB는 반드시 next()로 얻은
 *    BCB이어야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aTargetBCB  - [IN]  해당 BCB
 ***********************************************************************/
void sdbFlushList::remove(idvSQL *aStatistics, sdbBCB *aTargetBCB)
{
    smuList *sTarget;

    IDE_ASSERT(aTargetBCB != NULL);

    sTarget = &aTargetBCB->mBCBListItem;

    // mCurrent는 항상 getNext보다 prevBCB를 가리키고 있기때문에,
    // 제거 대상과 같지 않다.
    IDE_ASSERT(sTarget != mCurrent);

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    IDE_ASSERT(mExplorerCount > 0);

    SMU_LIST_DELETE(sTarget);
    SDB_INIT_BCB_LIST(aTargetBCB);

    IDE_ASSERT( mListLength != 0 );
    mListLength--;

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);
}

/***********************************************************************
 * Description :
 *    flush list의 맨 앞에 BCB를 하나 추가한다.
 *    
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  해당 BCB
 ***********************************************************************/
void sdbFlushList::add(idvSQL *aStatistics, sdbBCB *aBCB)
{
    IDE_ASSERT(aBCB != NULL);

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    mListLength++;

    SMU_LIST_ADD_FIRST(mBase, &aBCB->mBCBListItem);

    aBCB->mBCBListType = mListType;
    aBCB->mBCBListNo = mID;

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);
}

/***********************************************************************
 * Description :
 *  sdbFlushList의 모든 BCB들이 제대로 연결되어 있는지 확인한다.
 *  
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
IDE_RC sdbFlushList::checkValidation(idvSQL *aStatistics)
{
    smuList *sPrevNode;
    smuList *sListNode;
    UInt     i;

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);
    
    sListNode = SMU_LIST_GET_FIRST(mBase);
    sPrevNode = SMU_LIST_GET_PREV( sListNode );
    for( i = 0 ; i < mListLength; i++)
    {
        IDE_ASSERT( sPrevNode == SMU_LIST_GET_PREV( sListNode ));
        sPrevNode = sListNode;
        sListNode = SMU_LIST_GET_NEXT( sListNode);
    }
    
    IDE_ASSERT(sListNode == mBase);


    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

