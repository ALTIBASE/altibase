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
 * Abstraction : 버퍼풀에서 사용하는 prepare list를 구현한 파일이다.
 *               prepare list란 victim을 빠르게 찾기위해 flush list에서
 *               flush되어서 clean이거나 free인 BCB들을 저장하는 list이다.
 *               때문에 buffer miss시 victim 대상을 찾을 때
 *               prepare list를 가장먼저 뒤진다.
 *               prepare list로 들어올 땐 clean상태이지만 이는 엄격히
 *               지켜지진 않는다. 버퍼풀 정책상 BCB가 어느리스트에 있건
 *               hit된 후 dirty 또는 inIOB 상태로 될 수 있기 때문이다.
 *               그런데, 이들을 상태가 변하는 즉시 제거하지 않고,
 *               victim을 찾기위해 prepare List를 탐색하는 트랜잭션들이
 *               옮기는 작업을 수행한다.
 *
 ***********************************************************************/
#include <sdbPrepareList.h>
#include <smErrorCode.h>

/***********************************************************************
 * Description :
 *  aListID     - [IN]  prepare list 식별자
 ***********************************************************************/
IDE_RC sdbPrepareList::initialize(UInt aListID)
{
    SChar sMutexName[128];

    mBase               = &mBaseObj;
    mID                 = aListID;
    mListLength		= 0;
    mWaitingClientCount = 0;
    SMU_LIST_INIT_BASE(mBase);

    idlOS::memset(sMutexName, 0, 128);
    idlOS::snprintf(sMutexName, 128, "PREPARE_LIST_MUTEX_%"ID_UINT32_FMT, aListID);

    IDE_TEST(mMutex.initialize(sMutexName,
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_LATCH_FREE_DRDB_PREPARE_LIST)
             != IDE_SUCCESS);

    idlOS::memset(sMutexName, 0, 128);
    idlOS::snprintf(sMutexName, 128, "PREPARE_LIST_MUTEX_FOR_WAIT_%"ID_UINT32_FMT, aListID);

    IDE_TEST(mMutexForWait.initialize(sMutexName,
                                      IDU_MUTEX_KIND_POSIX,
                                      IDV_WAIT_INDEX_LATCH_FREE_DRDB_PREPARE_LIST_WAIT)
             != IDE_SUCCESS);

    // condition variable 초기화
    idlOS::snprintf(sMutexName, 
                    ID_SIZEOF(sMutexName),
                    "PREPARE_LIST_COND_%"ID_UINT32_FMT, 
                    aListID);

    IDE_TEST_RAISE(mCondVar.initialize(sMutexName) != IDE_SUCCESS,
                   err_cond_var_init);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  삭제 함수
 ***********************************************************************/
IDE_RC sdbPrepareList::destroy()
{
    IDE_ASSERT(mMutex.destroy() == IDE_SUCCESS);

    IDE_ASSERT(mMutexForWait.destroy() == IDE_SUCCESS);

    IDE_TEST_RAISE(mCondVar.destroy() != 0, err_cond_destroy);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Abstraction :
 *    prepare list에서 last를 하나 떼온다.
 *    반환되는 BCB의 next, prev는 모두 NULL이다.
 *    동시성은 내부적으로 제어된다.
 *
 * Implementation :
 *  처음에 mutex를 잡지않고 empty인지 확인해 본다.
 *  prepare list는 empty인 경우가 많다. 그리고, 트랜잭션들은 가장 먼저 prepare
 *  list에 접근을 하기 때문에, 경합이 많이 일어날 수 있다. 그러므로 empty판단
 *  까지는 mutex를 잡지 않고 해본다.
 *
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
sdbBCB* sdbPrepareList::removeLast( idvSQL *aStatistics )
{
    sdbBCB  *sRet               = NULL;
    smuList *sTarget;

    if ( mListLength == 0 )
    {
        sRet = NULL;
    }
    else
    {
        IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

        if (mListLength == 0)
        {
            // list에 BCB가 하나도 없다.
            IDE_ASSERT( SMU_LIST_IS_EMPTY(mBase) == ID_TRUE );
            sRet = NULL;
        }
        else
        {
            // list에 최소한 하나 이상의 BCB가 있다.
            sTarget = SMU_LIST_GET_LAST(mBase);


	    SMU_LIST_DELETE(sTarget);


            sRet = (sdbBCB*)sTarget->mData;
            SDB_INIT_BCB_LIST(sRet);

            IDE_ASSERT( mListLength != 0 );
            mListLength--;
        }

        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);

    return sRet;
}

/***********************************************************************
 * Description:
 *    flusher가 prepare list에 BCB를 추가할 때까지 조건대기를 한다.
 *    이 함수는 prepare list, LRU list에서 victim을 찾다가 실패해
 *    prepare list에 대해 대기를 하고자할 때 사용된다.
 *    timeover로 깨어날 경우엔 aBCBAdded가 false이고 BCB가 추가되어
 *    깨어난 경우엔 이 파라메터가 true가 된다.
 *
 *  aStatistics - [IN]  통계정보
 *  aTimeSec    - [IN]  조건 대기를 몇초동안 할 것인가를 지정
 *  aBCBAdded   - [OUT] BCB가 추가된 경우 true, timeover로깨어난 경우
 *                      false가 리턴된다.
 ***********************************************************************/
IDE_RC sdbPrepareList::timedWaitUntilBCBAdded(idvSQL *aStatistics,
                                              UInt    aTimeMilliSec,
                                              idBool *aBCBAdded)
{
    PDL_Time_Value sTimeValue;

    *aBCBAdded = ID_FALSE;
    sTimeValue.set(idlOS::time(NULL) + (SDouble)aTimeMilliSec / 1000);

    IDE_ASSERT(mMutexForWait.lock(aStatistics) == IDE_SUCCESS);
    mWaitingClientCount++;

    IDE_TEST_RAISE(mCondVar.timedwait(&mMutexForWait, &sTimeValue,
                                      IDU_IGNORE_TIMEDOUT)
                   != IDE_SUCCESS, err_cond_wait);

    // cond_broadcast를 받고 깨어난 경우
    *aBCBAdded = ID_TRUE;
    mWaitingClientCount--;

    IDE_ASSERT(mMutexForWait.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Abstraction :
 *    prepare list의 first에 BCB 리스트를 추가한다.
 *    그리고 이 prepare list에 대기중인 쓰레드가 있으면 깨운다.
 *    
 *  aStatistics - [IN]  통계정보
 *  aFirstBCB   - [IN]  추가할 BCB list의 첫번째 BCB. 이 BCB의 prev는 NULL이어야
 *                      한다.
 *  aLastBCB    - [IN]  추가할 BCB list의 마지막 BCB. 이 BCB의 next는
 *                      NULL이어야 한다.
 *  aLength     - [IN]  추가되는 BCB list의 길이.
 ***********************************************************************/
IDE_RC sdbPrepareList::addBCBList( idvSQL *aStatistics,
                                   sdbBCB *aFirstBCB,
                                   sdbBCB *aLastBCB,
                                   UInt    aLength )
{
    UInt     i;
    smuList *sNode;

    IDE_DASSERT(aFirstBCB != NULL);
    IDE_DASSERT(aLastBCB != NULL);

    // 입력되는 BCB의 리스트 정보를 세팅한다.
    for (i = 0, sNode = &aFirstBCB->mBCBListItem; i < aLength; i++)
    {
        ((sdbBCB*)sNode->mData)->mBCBListType = SDB_BCB_PREPARE_LIST;
        ((sdbBCB*)sNode->mData)->mBCBListNo   = mID;
        sNode = SMU_LIST_GET_NEXT(sNode);
    }
    IDE_DASSERT(sNode == NULL);

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    mListLength += aLength;

    SMU_LIST_ADD_LIST_FIRST( mBase,
                             &aFirstBCB->mBCBListItem,
                             &aLastBCB->mBCBListItem );

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    // prepare list에 대기하고 있는 쓰레드가 있으면 깨운다.
    if (mWaitingClientCount > 0)
    {
        // mWaitingClientCount만 올리고 미처 cond_wait까지 못한 쓰레드를 위해
        // mMutexForWait를 획득한 후 broadcast한다.
        IDE_ASSERT(mMutexForWait.lock(aStatistics) == IDE_SUCCESS);

        IDE_TEST_RAISE(mCondVar.broadcast() != IDE_SUCCESS,
                       err_cond_signal);

        IDE_ASSERT(mMutexForWait.unlock() == IDE_SUCCESS);
    }

    IDE_DASSERT(checkValidation(aStatistics) == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  sdbPrepareList의 모든 BCB들이 제대로 연결되어 있는지 확인한다.
 *  
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
IDE_RC sdbPrepareList::checkValidation(idvSQL *aStatistics)
{
    smuList *sPrevNode;
    smuList *sListNode;
    UInt     i;

    IDE_ASSERT(mMutex.lock(aStatistics) == IDE_SUCCESS);

    sListNode = SMU_LIST_GET_FIRST(mBase);
    sPrevNode = SMU_LIST_GET_PREV( sListNode );
    for( i = 0; i < mListLength; i++)
    {
        IDE_ASSERT( sPrevNode == SMU_LIST_GET_PREV( sListNode ));
        sPrevNode = sListNode;
        sListNode = SMU_LIST_GET_NEXT( sListNode);
    }
    
    IDE_ASSERT(sListNode == mBase);

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;
}
