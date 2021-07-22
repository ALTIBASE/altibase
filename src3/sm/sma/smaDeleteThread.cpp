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
 * $Id: smaDeleteThread.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smx.h>
#include <smcLob.h>
#include <sma.h>
#include <svcRecord.h>
#include <smi.h>
#include <sgmManager.h>
#include <smr.h>
#include <smlLockMgr.h>
#include <svm.h>

iduMutex           smaDeleteThread::mMutex;
ULong              smaDeleteThread::mHandledCnt; // atomicInc 대상
smaDeleteThread*   smaDeleteThread::mDeleteThreadList;
UInt               smaDeleteThread::mThreadCnt;
smxTrans**         smaDeleteThread::mTrans;
smxTrans*          smaDeleteThread::mTrans4Self;
ULong              smaDeleteThread::mAgingProcessedOIDCnt; // AtomicInc 대상
ULong              smaDeleteThread::mSleepCountOnAgingCondition; // lock 획득후에만 증가함
iduMutex *         smaDeleteThread::mListLock;
UInt               smaDeleteThread::mIsParallelMode;
iduMutex           smaDeleteThread::mCheckMutex4Self;

smaDeleteThread::smaDeleteThread() : idtBaseThread()
{

}


IDE_RC smaDeleteThread::initializeStatic()
{
    UInt i;

    mHandledCnt = 0;
    mAgingProcessedOIDCnt = 0;
    mDeleteThreadList = NULL;

    mIsParallelMode = smuProperty::getParallelDeleteThread();

    IDE_TEST(mMutex.initialize( (SChar*)"DELETE_THREAD_MUTEX",
                                IDU_MUTEX_KIND_POSIX,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    IDE_TEST(mCheckMutex4Self.initialize((SChar*)"DELETE_THREAD_CHECK_MUTEX",
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    mThreadCnt = smuProperty::getDeleteAgerCount();

    /* TC/FIT/Limit/sm/sma/smaDeleteThread_alloc_malloc.sql */
    IDU_FIT_POINT_RAISE( "smaDeleteThread::alloc::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMA,
                               (ULong)ID_SIZEOF(smaDeleteThread) * mThreadCnt,
                               (void**)&mDeleteThreadList) != IDE_SUCCESS,
                   insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                      (ULong)ID_SIZEOF( smxTrans* ) * mThreadCnt,
                                      (void**)&mTrans ) != IDE_SUCCESS,
                   insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                       ID_SIZEOF(iduMutex) * smaLogicalAger::mListCnt ,
                                       (void**)&(mListLock)) != IDE_SUCCESS,
                    insufficient_memory );

    for ( i = 0; i < smaLogicalAger::mListCnt ; i++ )
    {
        IDE_TEST( mListLock[i].initialize( (SChar*)"DELETE_THREAD_MUTEX",
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    }

    for(i = 0; i < mThreadCnt; i++)
    {
        new (mDeleteThreadList + i) smaDeleteThread;
        IDE_TEST(mDeleteThreadList[i].initialize( i ) != IDE_SUCCESS);
    }

    IDE_TEST(smxTransMgr::alloc(&mTrans4Self) != IDE_SUCCESS);

    mTrans4Self->mConnectDeleteThread = &mCheckMutex4Self;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaDeleteThread::destroyStatic()
{
    UInt i;

    mTrans4Self->mConnectDeleteThread = NULL;
    IDE_TEST(smxTransMgr::freeTrans( mTrans4Self ) != IDE_SUCCESS);

    for( i = 0; i < smaLogicalAger::mListCnt; i++ )
    {
        IDE_TEST( mListLock[i].destroy() != IDE_SUCCESS );
    }

    IDE_TEST( iduMemMgr::free( mListLock )
              != IDE_SUCCESS );
    mListLock = NULL;

    for(i = 0; i < mThreadCnt; i++)
    {
        IDE_TEST(mDeleteThreadList[i].destroy() != IDE_SUCCESS);
    }

    IDE_TEST( mCheckMutex4Self.destroy() != IDE_SUCCESS );

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    if(mDeleteThreadList != NULL)
    {
        IDE_TEST(iduMemMgr::free(mDeleteThreadList)
                 != IDE_SUCCESS);
        mDeleteThreadList = NULL;
    }

    if ( mTrans != NULL )
    {
        IDE_TEST( iduMemMgr::free( mTrans )
                  != IDE_SUCCESS );
        mTrans = NULL;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaDeleteThread::shutdownAll()
{
    UInt i;

    for(i = 0; i < mThreadCnt; i++)
    {
        IDE_TEST(mDeleteThreadList[i].shutdown() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaDeleteThread::initialize( UInt aThreadID )
{

    mFinished  = ID_FALSE;
    mHandled   = ID_FALSE;
    mSleepCountOnAgingCondition = 0;
    mAgingCntAfterATransBegin   = 0;

    mTimeOut   = smuProperty::getAgerWaitMax();
    mThreadID  = aThreadID;

    mTrans[mThreadID] = NULL;

    IDE_TEST(mCheckMutex.initialize((SChar*)"DELETE_THREAD_CHECK_MUTEX",
                                    IDU_MUTEX_KIND_POSIX,
                                    IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    IDE_TEST(smxTransMgr::alloc( &mTrans[mThreadID]) != IDE_SUCCESS);

    mTrans[mThreadID]->mConnectDeleteThread = &mCheckMutex;

    IDE_TEST( start() != IDE_SUCCESS );

    IDE_TEST(waitToStart(0) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smaDeleteThread::destroy()
{
    mTrans[mThreadID]->mConnectDeleteThread = NULL;
    IDE_TEST(smxTransMgr::freeTrans(mTrans[mThreadID]) != IDE_SUCCESS);

    IDE_TEST( mCheckMutex.destroy() != IDE_SUCCESS );

    mTrans[mThreadID] = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

/***********************************************************************
 * Description : Table Drop을 Commit후에 Transaction이 직접 수행하기때문에
 *               Ager가 Drop Table에 대해서 Aging작업을 하면 안된다. 이를 위해
 *               Ager는 해당 Table에 대해 Aging작업을 하기전에 Table이 Drop되
 *               었는지 조사한다. 그런데 이 Table이 Drop되었는지 Check하고 간사이
 *               에 Table이 Transaction에 의해서 Drop될 수 있기 때문에 문제가된다.
 *               이때문에 Ager는 연산 수행시 mCheckMutex를 걸고 작업을 수행한다.
 *               따라서 Table Drop을 수행한 Transaction이 mCheckMutex를 잡았다는
 *               이야기는 아무도 Ager가 Table에 접근하지 않고 있다는 것을 보장하고
 *               또한 그 이후에 Ager가 접근할때는 테이블에 Set된 Drop Flag를
 *               보게 되어 Table에 대해 Aging작업을 수행하지 않는다.
 *
 * 관련 BUG: 15047
 **********************************************************************/
void smaDeleteThread::waitForNoAccessAftDropTbl()
{
    UInt i;

    for ( i = 0; i < mThreadCnt; i++ )
    {
        IDE_ASSERT( mDeleteThreadList[i].lockCheckMtx()
                    == IDE_SUCCESS);
        IDE_ASSERT( mDeleteThreadList[i].unlockCheckMtx() 
                    == IDE_SUCCESS);
    }

    IDE_ASSERT( mCheckMutex4Self.lock( NULL ) == IDE_SUCCESS );
    IDE_ASSERT( mCheckMutex4Self.unlock()     == IDE_SUCCESS );
   
}

void smaDeleteThread::run()
{

    PDL_Time_Value      sTimeOut;
    UInt                sState = 0;
    UInt  sAgerWaitMin;
    UInt  sAgerWaitMax;
    UInt   sListN   = mThreadID % smaLogicalAger::mListCnt ;
    idBool sHandled = ID_FALSE;

  startPos :

    sState = 0;

    sAgerWaitMax = smuProperty::getAgerWaitMax();
    sAgerWaitMin = smuProperty::getAgerWaitMin();

    while(mFinished != ID_TRUE)
    {
        /* BUG-47601: 전부 순회 하고 나면 Sleep한번.. */
        if ( sListN == (mThreadID % smaLogicalAger::mListCnt) )
        {
            if ( sHandled == ID_TRUE )
            {
                mTimeOut >>= 1;
                mTimeOut = ( mTimeOut < sAgerWaitMin ) ? sAgerWaitMin : mTimeOut;
                sHandled = ID_FALSE;
            }
            else
            {
                mTimeOut <<= 1;
                mTimeOut = ( mTimeOut > sAgerWaitMax ) ? sAgerWaitMax : mTimeOut;
            }

            if ( mTimeOut > sAgerWaitMin )
            {
                sTimeOut.set(0, mTimeOut);
                idlOS::sleep(sTimeOut);
            }
        }

        if ( smuProperty::isRunMemDeleteThread() == SMU_THREAD_OFF )
        {
            // To Fix PR-14783
            // System Thread의 작업을 수행하지 않도록 한다.
            mTimeOut = sAgerWaitMax;
            continue;
        }
        else
        {
            // Go Go
        }

        IDU_FIT_POINT("1.smaDeleteThread::run");

        /* realDelete 내부에서 DeleteThread간의 동시성 제어를 한다. */
        //Delete All Record and Drop Table
        IDE_TEST(realDelete(ID_FALSE, sListN) != IDE_SUCCESS);

        if( mHandled == ID_TRUE )
        {
            sHandled = ID_TRUE; 
        }

        /* 다음 list를 탐색하러 간다 */
        sListN++;

        if (sListN >= smaLogicalAger::mListCnt )
        {
            sListN = 0;
        }
    }

    IDE_TEST(lock() != IDE_SUCCESS);
    sState = 1;

    /* aging thread가 정지되니 일단 다 돌아본다. */
    for ( sListN = 0; sListN < smaLogicalAger::mListCnt ; sListN++ )
    {
        IDE_TEST(realDelete(ID_TRUE, sListN) != IDE_SUCCESS);
    }

    sState = 0;
    IDE_TEST(unlock() != IDE_SUCCESS);

    return;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)unlock();
    }

    IDE_WARNING(SM_TRC_LOG_LEVEL_WARNNING,
                SM_TRC_MAGER_WARNNING);

    goto startPos;

}

/*
 * Aging OID List에 대해서 Aging을 수행한다. 이 함수는 크게
 * 두가지 상황에서 Aging 대상에 대해서 Aging을 수행한다.
 *
 * 1. Server가 종료시에 aDeleteAll이 ID_TRUE로 넘어와서
 *    Aging OID List에 대해서 모두 Aging연산을 수행한다.
 *
 * 2. OID List Header의 mKeyFreeSCN이
 *
 *    MinViewSCN = MIN( system scn + 1, MIN( Transaction들의 View SCN )
 *
 *   2.1 MinViewSCN보다 작을때
 *
 *   2.2 MinViewSCN과 같고, Transaction의 View SCN이 없을 경우
 *     - system scn + 1과 같고 , Transaction들이 모두 View를 보고 있지
 *       않을 경우 이 List는 어떤 Transaction도 보지 않는다는 것을 보장하기
 *       때문에 Aging할 수 있다.
 *
 *  Data Structure
 *   - <smaOidList ( mNext ) >  -> < smaOidList ( mNext ) >  ->  ...
 *      -  smxOIDNode* mHead         - smxOIDNode* mHead
 *      -  smxOIDNode* mTail         - smxOIDNode* mTail
 *
 *   - smaOIDList가 Header이고 Header들은 mNext를 이용해서 Linked List를 유지
 *     한다.
 *   - smaOIDList의 mHead, mTail이 Node리스트의 Head와 Tail을 가리킨다.
 */
IDE_RC smaDeleteThread::realDelete( idBool aDeleteAll, SInt aListN )
{
    smaOidList         * sCurOIDHead        = NULL;
    smSCN                sMinViewSCN;
    smSCN                sSystemSCN;
    smTID                sOldestTID;
    smaOidList         * sLogicalAgerHead   = NULL;
    SInt                 sListN = aListN;
    idBool               sState             = ID_FALSE;
    idBool               sIncreaseSystemSCN = ID_FALSE;
    
    mHandled = ID_FALSE;

//    IDE_TEST( mListLock[sListN].lock(NULL) !=IDE_SUCCESS );
//    sState = ID_TRUE;
    mListLock[sListN].trylock( sState );

    if( sState != ID_TRUE )
    {
        IDE_CONT( skip_list );
    }

    if( smaLogicalAger::mTailDeleteThread[sListN] != NULL )
    {
        /* BUG-19308: Server Stop시 Delete Thread가 Aging작업을 완료하지 않고
         *             종료합니다.
         *
         * aDeleteAll = ID_TRUE이면 무조건 Aging을 하도록 합니다. */

        if( ( smaLogicalAger::mTailDeleteThread[sListN]->mErasable == ID_TRUE ) ||
            ( aDeleteAll == ID_TRUE ) )
        {
            /* Transaction의 Minimum View SCN을 구한다. */
            ID_SERIAL_BEGIN(
                    sLogicalAgerHead = smaLogicalAger::mTailLogicalAger[sListN] );
            ID_SERIAL_END(
                    SMX_GET_MIN_MEM_VIEW( &sMinViewSCN, &sOldestTID ) );

            beginATrans();

            while( 1 )
            {
                sCurOIDHead = smaLogicalAger::mTailDeleteThread[sListN];

                /* 해당 Node를 Aging을 수행해도 되는지 조사한다. */
                if( isAgingTarget( sCurOIDHead,
                                   aDeleteAll,
                                   &sMinViewSCN )
                    == ID_FALSE )
                {
                    break;
                }

                smaLogicalAger::mTailDeleteThread[sListN] = sCurOIDHead->mNext;

                if ( mIsParallelMode == SMA_PARALLEL_DELETE_THREAD_ON )
                {
                    /* 처리할 OID를 가져왔으면 lock을 풀어준다. */
                    sState = ID_FALSE;
                    IDE_TEST( mListLock[sListN].unlock() != IDE_SUCCESS );
                }

                /* sCurOIDHead에 메달려 있는 OID Node List에 대해서
                   Aging을 수행한다. */
                IDE_ASSERT( processAgingOIDNodeList( sCurOIDHead, aDeleteAll )
                            == IDE_SUCCESS );

                if ( mIsParallelMode == SMA_PARALLEL_DELETE_THREAD_ON )
                {
                    /* 다음 OID 탐색을 위해 lock을 획득한다. */
                    IDE_TEST( mListLock[sListN].lock(NULL) !=IDE_SUCCESS );
                    sState = ID_TRUE;
                }

                if( sCurOIDHead == sLogicalAgerHead )
                {
                    break;
                }

            } /* While */

            commitATrans();

            /* OIDHead의 mKeyFreeSCN이 Min SCN보다 작을때 Aging하는데 Key Free SCN은
             * System SCN으로 설정되기 때문에 mKeyFreeSCN이 설정된 이후에 Commit한
             * Transaction이 없을 경우 System SCN은 증가하지 않기때문에 Delete Thread가
             * 계속 대기하는 문제가 발생한다. 하여 Aging List에 Aging대상은 있지만
             * System SCN이 KeyFreeSCN과 동일하고 ViewSCN을 설정한 Transaction이 없다면
             * System SCN을 증가시킨다. 이때 자주 증가 되지 않도록 mTimeOut이 최대치 일때
             * 만 증가시킨다. */

            sIncreaseSystemSCN = ID_FALSE;

            if ( ( mHandled   == ID_FALSE ) &&
                 ( mTimeOut   == smuProperty::getAgerWaitMax() ) &&
                 ( sOldestTID == SM_NULL_TID ) )
            {
                if ( smxTransMgr::isActiveVersioningMinTime() == ID_TRUE )
                {
                    /* PROJ-2733
                       AgingViewSCN을 이용해 Aging 중이라면,
                       AgingViewSCN이 system SCN 까지 도달하지 못한 경우가 있을수있다.
                       AgingViewsCN이 system SCN 을 넘어선 이후에만 system SCN을 증가시키도록 한다. */

                    sSystemSCN = smmDatabase::getLstSystemSCN();

                    if ( SM_SCN_IS_GT( &sMinViewSCN, &sSystemSCN ) )
                    {
                        sIncreaseSystemSCN = ID_TRUE;
                    }
                }
                else
                {
                    sIncreaseSystemSCN = ID_TRUE;
                }
            }

            if ( sIncreaseSystemSCN == ID_TRUE )
            {
                IDE_TEST( smmDatabase::getCommitSCN( NULL,     /* aTrans* */
                                                     ID_FALSE, /* aIsLegacytrans */
                                                     NULL )    /* aStatus */
                          != IDE_SUCCESS );
            }
        }
    }

    /* 해당 List에서 더 이상 처리할 OID가 없다. lock을 풀어준다. */
    sState = ID_FALSE;
    IDE_TEST( mListLock[sListN].unlock() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_list );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == ID_TRUE )
    {
        sState = ID_FALSE;
        IDE_ASSERT( mListLock[sListN].unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC smaDeleteThread::processAgingOIDNodeList( smaOidList *aOIDHead,
                                                 idBool      aDeleteAll )
{
    smxOIDNode         *sCurOIDNode;
    smxOIDNode         *sNxtOIDNode;
    smxOIDInfo         *sOIDInfo;
    UInt                i;
    UInt                sCondition;
    idBool              sIsProcessed;

    //delete record and drop table
    sCurOIDNode = aOIDHead->mHead;
    sCondition  = aOIDHead->mCondition;

    while( sCurOIDNode != NULL )
    {
        for( i = 0; i < sCurOIDNode->mOIDCnt; i++ )
        {
            sOIDInfo = sCurOIDNode->mArrOIDInfo + i;

            if ( (( sOIDInfo->mFlag & sCondition ) == sCondition) && 
                 (( sOIDInfo->mFlag & SM_OID_ACT_COMPRESSION ) == 0) )
            {
                /* BUG-15047 */
                IDE_ASSERT( lockCheckMtx() == IDE_SUCCESS );

                /* Ager Thread에서 연산실패는 Fatal이므로 에러처리하지
                 * 않는다.*/
                IDE_ASSERT( processJob( mTrans[mThreadID] ,
                                        sOIDInfo,
                                        aDeleteAll,
                                        &sIsProcessed,
                                        aOIDHead->mSCN )
                            == IDE_SUCCESS );

                IDE_ASSERT( unlockCheckMtx() == IDE_SUCCESS );

                if( sIsProcessed == ID_TRUE )
                {
                    mAgingCntAfterATransBegin++;
                }//if sIsProcessd
            }//if

            /* BUG-17417 V$Ager정보의 Add OID갯수는 실제 Ager가
             *           해야할 작업의 갯수가 아니다.
             *
             * 하나의 OID에 대해서 Aging을 수행하면
             * mAgingProcessedOIDCnt를 1증가 시킨다. */
            if( smxOIDList::checkIsAgingTarget( aOIDHead->mCondition,
                                                sOIDInfo ) == ID_TRUE )
            {
                acpAtomicInc64( &mAgingProcessedOIDCnt );
            }

        }//for

        sNxtOIDNode = sCurOIDNode->mNxtNode;

        mHandled = ID_TRUE;

        IDE_ASSERT( smxOIDList::freeMem( sCurOIDNode )
                    == IDE_SUCCESS );

        sCurOIDNode = sNxtOIDNode;

        commitAndBeginATransIfNeed();
    }//while

    // BUG-15306
    // DummyOID인 경우엔 처리한 count에 포함하지 않는다.
    if( smaLogicalAger::isDummyOID( aOIDHead ) != ID_TRUE )
    {
        acpAtomicInc64( &mHandledCnt );
    }

    IDE_ASSERT( freeOIDNodeListHead( aOIDHead ) == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/***********************************************************

  특정 Table안의 OID나 특정 Tablespace안의 OID에 대해서만
  즉시 Aging을 실시한다.

  Argument:
      1. smOID aTableOID: Aging을 즉각적으로 수행할 Table의 OID
      2. smaOidList *aEndPtr: aEndPtr까지 Aging을 수행.

  Return Value:
      1. SUCCESS : IDE_SUCCESS
      2. FAIL    : IDE_FAILURE

  Description:
    Aging OID List에 매달려 있는 것중에서
    특정 Table안의 OID나 특정 Tablespace안의 OID에
    대해서만 Aging을 수행한다.

    Aging하려는 OID의 SCN이 Active Transaction들의 Minimum ViewSCN 보다
    크더라도 Aging을 실시한다.

  Caution :
    이 Function을 사용시 다른 쪽에서 Aging Filter에 지정한 Table이나
    Tablespace에 대해서 다른 Transaction이 접근하지 않는다는 것을 보장
    해야 한다.

    Table이나 Tablespace에  X-Lock을 잡는 것으로 이를 보장할 수 있다.

**********************************************************/
IDE_RC smaDeleteThread::deleteInstantly( smaInstantAgingFilter * aAgingFilter )
{
    smxOIDNode         * sCurOIDNodePtr;
    smxOIDNode         * sNxtOIDNodePtr;
    smaOidList         * sCurOIDHeadPtr;
    UInt                 i;
    smxOIDInfo         * sOIDInfoPtr;
    UInt                 sCondition;
    idBool               sIsProcessed;
    UInt                 sState = 0;
    /* BUG-41026 */
    PDL_Time_Value       sTimeOut;
    UInt                 sVarTime = 0;
    UInt                 sAgerWaitMin = 0;
    UInt                 sAgerWaitMax = 0;

    UInt                 sListN = 0;
    idBool               sIsLock = ID_FALSE;
    smcTableHeader     * sCurTable;

    sAgerWaitMax = smuProperty::getAgerWaitMax();
    sAgerWaitMin = smuProperty::getAgerWaitMin();
    sVarTime = sAgerWaitMin;
    sTimeOut.set(0, sVarTime);

    /* BUG-47367 DeleteThread와의 동시성과는 별도로
     * Instant Aging을 수행하는 Tx끼리의 동시성 제어가 필요하다.
     * Instant Aging 용으로 할당된 mTrans4Self는 1개이기 때문에
     * 동시에 Instant Aging을 수행하면 안된다. */
    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    /*
      별도의 Transaction으로 처리한다. 왜냐하면 Aging을 수행한
      Transaction은 항상 Commit해야하는데 Rollback이 발생할
      수 있기 때문에 별도의 Transaction으로 처리한다.
    */
    IDE_ASSERT( mTrans4Self->begin( NULL,
                                    ( SMI_TRANSACTION_REPL_NONE |
                                      SMI_COMMIT_WRITE_NOWAIT ),
                                    SMX_NOT_REPL_TX_ID )
               == IDE_SUCCESS);
    sState = 2;

    /* Transaction의 RSGroupID를 0으로 설정한다.
     * 0번 PageList에 페이지를 반납하게 된다. */
    smxTrans::setRSGroupID((void*)mTrans4Self, 0);

    /* instant aging은 특정 Table,TableSpace를 대상으로 하기 때문에
     * 모든 list를 다 뒤져야 하는 부담이 있다. */
    for ( sListN = 0; sListN < smaLogicalAger::mListCnt ; sListN++ )
    {
        mListLock[sListN].lock( NULL );
        sIsLock = ID_TRUE;

        /* BUG-32780  [sm-mem-index] The delete thread can access removed page
         * because the range of instant aging is abnormal.
         * Lock을 잡고 mTailDeleteThread 값을 가져와야 한다. */
        sCurOIDHeadPtr = smaLogicalAger::mTailDeleteThread[sListN];

        while(sCurOIDHeadPtr != NULL)
        {
            sCondition = sCurOIDHeadPtr->mCondition;

            sCurOIDNodePtr = sCurOIDHeadPtr->mHead;

            while(sCurOIDNodePtr != NULL)
            {
                for(i = 0; i < sCurOIDNodePtr->mOIDCnt; i++)
                {
                    sOIDInfoPtr = sCurOIDNodePtr->mArrOIDInfo + i;

                    if ( ((sOIDInfoPtr->mFlag & sCondition) == sCondition) &&
                         ((sOIDInfoPtr->mFlag & SM_OID_ACT_COMPRESSION) == 0) )
                    {
                        /* BUG-47367 LogicalAger에서 Drop된 OID 객체에 대해서는 
                         * Filter 조건에 맞아도 Aging 처리를 해주지 않는다.(Flag 확인에서 HANG)
                         * LogicalAger와 동일한 순서로 Drop Check를 하도록 해준다. */
                        /* Instant Ager의 동작은 TABLE or TBS lock을 잡고 하기 때문에
                         * 중간에 Drop 되거나 하지는 않으나 같은 OID를 가진 
                         * 이전에 Drop된 Table의 OID를 가지고 달려 있는 OIDList가 존재할수 있다. */
                        IDE_ASSERT( smcTable::getTableHeaderFromOID( sOIDInfoPtr->mTableOID,
                                                                     (void**)&sCurTable)
                                    == IDE_SUCCESS );

                        if ( smcTable::isDropedTable4Ager( sCurTable, sCurOIDHeadPtr->mSCN ) == ID_TRUE )
                        {
                            continue;
                        }

                        // Filter조건을 만족?
                        if ( smaLogicalAger::isAgingFilterTrue(
                                                 aAgingFilter,
                                                 sOIDInfoPtr->mSpaceID,
                                                 sOIDInfoPtr->mTableOID ) == ID_TRUE )
                        {
                            /* BUG-41026 : parallel로 진행중인 normal aging이 진행중
                             * 이면 끝날 때 까지 대기한다. */
                            while ( 1 )
                            {
                                /* BUG-41026 */
                                IDU_FIT_POINT( "4.BUG-41026@smaDeleteThread::deleteInstantly::wakeup_2");

                                if ( ( sOIDInfoPtr->mFlag & SM_OID_ACT_AGING_INDEX ) == 0 )
                                {
                                    break;
                                }
                                else
                                {
                                    idlOS::sleep(sTimeOut);
                                    
                                    sVarTime <<= 1;
                                    sVarTime = (sVarTime > sAgerWaitMax) ? sAgerWaitMax : sVarTime;
                                    sTimeOut.set(0, sVarTime);
                                }
                            }

                            IDE_TEST( processJob( mTrans4Self,
                                                  sOIDInfoPtr,
                                                  ID_FALSE,   /* aDeleteAll */
                                                  &sIsProcessed,
                                                  sCurOIDHeadPtr->mSCN )
                                      != IDE_SUCCESS );
                            acpAtomicInc64( &mAgingProcessedOIDCnt );

                            /*
                              다시 작업을 반복하지 않기 위해서 flag을 clear시킨다.
                            */
                            sOIDInfoPtr->mFlag &= ~sCondition;
                        }
                    }
                }

                sNxtOIDNodePtr = sCurOIDNodePtr->mNxtNode;
                sCurOIDNodePtr = sNxtOIDNodePtr;
                
            }

            sCurOIDHeadPtr = sCurOIDHeadPtr->mNext;

        }
        sIsLock = ID_FALSE;
        mListLock[sListN].unlock();
    }

    sState = 1;
    IDE_TEST( mTrans4Self->commit() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 2:
        IDE_ASSERT(mTrans4Self->commit() == IDE_SUCCESS);

        case 1:
        IDE_ASSERT( unlock() == IDE_SUCCESS );
        break;
    }

    if( sIsLock == ID_TRUE )
    {
        mListLock[sListN].unlock();
    }

    return IDE_FAILURE;
}
/********************************************************

  Function Name: processJob(smxTrans    *aTransPtr,
                            smxOIDNode  *aCurOIDNodePtr,
                            smxOIDInfo  *aOIDInfoPtr,
                            idBool      *aIsProcessed)
  Argument:
      1. smxTrans    *aTransPtr : Begin된 Transaction Pointer.
      2. smxOIDNode  *aCurOIDNodePtr: Aging을 수행할 OID Header
                                      Pointer
      3. smxOIDInfo  *aOIDInfoPtr: Aging을 수행할 OID Info Pointer
      4. idBool      *aIsProcessed : Out argument로서 delete작업이
                                     있었으면 ID_TRUE, 아니면 ID_FALSE
      5. smSCN        aSCN : Aging을 수행할 OID 의 CommitSCN 값

  Return Value:
      1. SUCCESS : IDE_SUCCESS
      2. FAIL    : IDE_FAILURE

  Description:
      aTransPtr이 가리키는 Transaction으로 aOIDInfoPtr을 참조하여 Aging을
      수행함.

  Caution:
      이 Function은 무조건 지우는 작업을 하기때문에 Aging대상이 더 이상 참조되지
      않을 경우에만 호출되어야만 한다.

*********************************************************/
IDE_RC smaDeleteThread::processJob( smxTrans    * aTransPtr,
                                    smxOIDInfo  * aOIDInfoPtr,
                                    idBool        aDeleteAll,
                                    idBool      * aIsProcessed,
                                    smSCN         aSCN )
{
    smcTableHeader *sCatalogTable = (smcTableHeader*)SMC_CAT_TABLE;
    smcTableHeader *sTableHeader = NULL;
    SChar          *sRowPtr = NULL;
    smxSavepoint   *sISavepoint = NULL;
    UInt            sDummy = 0;

    ACP_UNUSED( aDeleteAll );

    *aIsProcessed  = ID_TRUE;

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aOIDInfoPtr->mTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    /*
      Table can be droped by ager before ager free fix/var record.
      because table lock of transaction is freed, when transaction
      inserting record is partially rolled back. and then other transaction can drop table.
      so aging sequence could be drop table pending, free slot record.
      so we must check table is droped before free fix/var record.
      - BUG-13968
    */
    switch(aOIDInfoPtr->mFlag & SM_OID_TYPE_MASK)
    {
        case SM_OID_TYPE_INSERT_FIXED_SLOT:
        case SM_OID_TYPE_UPDATE_FIXED_SLOT:
        case SM_OID_TYPE_DELETE_FIXED_SLOT:
            if(smcTable::isDropedTable4Ager(sTableHeader, aSCN) == ID_FALSE)
            {
#if defined(DEBUG)
                /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the 
                 * failure of index aging. 
                 * Index Key가 먼저 지워졌는지 확인한다.
                 * DeleteAll이 설정되면 logical ager는 무시하고 모든
                 * OIDList를 aging 하기 때문에 index key가 지워지지 않고
                 * 존재할 수 있다. */
                /* BUG-47526 1. Delete All 인 경우에만 확인하므로,
                 * Delete All이 아닌 경우에 Index key가 지워지지 않은 node가
                 * 오면 안됩니다. (smaLogicalAger::run()에서 수정)
                 * 2. Assert 위치를 옮깁니다.
                 * 상위 단 보다는 Index의 key를 확인한 시점에 Assert되어야
                 * 비교 검사가 가능한듯 하여 아래로 내려보냅니다. */
                if( aDeleteAll == ID_FALSE )
                {
                    IDE_TEST( smaLogicalAger::checkOldKeyFromIndexes(
                                                    sTableHeader,
                                                    aOIDInfoPtr->mTargetOID )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* do nothing */
                }
# endif
                IDE_ASSERT( smmManager::getOIDPtr( aOIDInfoPtr->mSpaceID,
                                                   aOIDInfoPtr->mTargetOID,
                                                   (void**)&sRowPtr )
                            == IDE_SUCCESS );

                if( SMI_TABLE_TYPE_IS_VOLATILE( sTableHeader ) == ID_TRUE )
                {
                    /* PROJ-1594 Volatile TBS */
                    /* volatile table일 경우 svcRecord를 호출해야 한다. */
                    IDE_TEST(svcRecord::setFreeFixRowPending(
                                                         aTransPtr,
                                                         sTableHeader,
                                                         sRowPtr,
                                                         aSCN )
                             != IDE_SUCCESS);
                }
                else
                {
                    IDE_TEST(smcRecord::setFreeFixRowPending(
                                                         aTransPtr,
                                                         sTableHeader,
                                                         sRowPtr,
                                                         aSCN )
                             != IDE_SUCCESS);
                }
            }
            break;

        case SM_OID_TYPE_FREE_LPCH:
            IDE_ASSERT( iduMemMgr::free((void*)(aOIDInfoPtr->mTargetOID))
                        == IDE_SUCCESS );
            break;

        case SM_OID_TYPE_VARIABLE_SLOT:
            if(smcTable::isDropedTable4Ager(sTableHeader,aSCN) == ID_FALSE)
            {
                /* PROJ-1594 Volatile TBS */
                /* volatile table일 경우 svcRecord를 호출해야 한다. */
                    IDE_ASSERT( smmManager::getOIDPtr( aOIDInfoPtr->mSpaceID,
                                                       aOIDInfoPtr->mTargetOID,
                                                       (void**)&sRowPtr )
                                == IDE_SUCCESS );
                if( SMI_TABLE_TYPE_IS_VOLATILE( sTableHeader ) == ID_TRUE )
                {
                    IDE_TEST( svcRecord::setFreeVarRowPending(
                                                     aTransPtr,
                                                     sTableHeader,
                                                     aOIDInfoPtr->mTargetOID,
                                                     sRowPtr,
                                                     aSCN )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( smcRecord::setFreeVarRowPending(
                                                     aTransPtr,
                                                     sTableHeader,
                                                     aOIDInfoPtr->mTargetOID,
                                                     sRowPtr,
                                                     aSCN )
                              != IDE_SUCCESS );
                }
            }

            break;

        case SM_OID_TYPE_DROP_TABLE:
            /* BUG-32237 [sm_transaction] Free lock node when dropping table.
             * DropTablePending 에서 연기해둔 freeLockNode를 수행합니다. */
            if( ( SMI_TABLE_TYPE_IS_DISK( sTableHeader )     == ID_TRUE ) ||
                ( SMI_TABLE_TYPE_IS_MEMORY( sTableHeader )   == ID_TRUE ) ||
                ( SMI_TABLE_TYPE_IS_VOLATILE( sTableHeader ) == ID_TRUE ) )
            {
                IDE_ASSERT( smcTable::isDropedTable4Ager(sTableHeader, aSCN)
                            == ID_TRUE );

                /* Transaction이 단 하나 존재하고, 해당 Transaction이 Drop을
                 * 수행했을때, Aging List를 전달하고 Transaction의 Lock을 풀기
                 * 때문에, Ager가 LockItem을 먼저 제거한 후 Lock이 풀리는
                 * 경우가 있을 수 있음. 따라서 Table에 XLock을 잡아 동시성
                 * 문제를 제거한 후 (어차피 Drop된 Table이라 성능저하 없음)
                 * Lock을 해제해야 함. */
                /* 다만 TABLE_LOCK_ENABLE이 꺼져있을 경우에는 CreateTable이
                 * 실패한 경우, Property 끄기 전에 매달린 Job들이 오는 것이지
                 * 이후에는 DDL이 없기 때문에 Lock체크할 필요가 없음 */
                if( smuProperty::getTableLockEnable() == 1 )
                {
                    IDE_TEST( aTransPtr->setImpSavepoint( &sISavepoint, sDummy )
                              != IDE_SUCCESS );
                    /* BUG-42928 No Partition Lock
                     *  이미 DROP한 Table/Partition이므로, mLock을 사용한다.
                     */
                    IDE_TEST( smlLockMgr::lockTableModeX( aTransPtr,
                                                          sTableHeader->mLock )
                              != IDE_SUCCESS );
                    IDE_TEST( aTransPtr->abortToImpSavepoint(
                                  sISavepoint )
                              != IDE_SUCCESS );
                    IDE_TEST( aTransPtr->unsetImpSavepoint( sISavepoint )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* nothing to do... */
                }

                IDE_TEST( smcTable::finLockItem( sTableHeader )
                          != IDE_SUCCESS );

                /* 어차피 free 될 slot이므로 값이 잘못되어도 상관없다.
                    * DASSERT로만 확인한다. */
                IDE_DASSERT( sTableHeader->mSelfOID == aOIDInfoPtr->mTableOID );

                if(( sTableHeader->mFlag & SMI_TABLE_PRIVATE_VOLATILE_MASK )
                   == SMI_TABLE_PRIVATE_VOLATILE_TRUE )
                {
                    /* PROJ-1407 Temporary table
                     * User temp table은 한 세션만 참조하고 create/drop이
                     * 빈번하므로, table header를 바로 Free한다.*/

                    IDE_DASSERT(( sTableHeader->mFlag & SMI_TABLE_TYPE_MASK )
                                == SMI_TABLE_VOLATILE );

                    IDE_TEST( smmManager::getOIDPtr(
                                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                              aOIDInfoPtr->mTableOID,
                                              (void**)&sRowPtr )
                              != IDE_SUCCESS );

                    IDE_TEST( smpFixedPageList::freeSlot(
                                              NULL, //aStatistics,
                                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                              &(SMC_CAT_TEMPTABLE->mFixed.mMRDB),
                                              sRowPtr,
                                              SMP_TABLE_TEMP,
                                              aSCN )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* PROJ-2268 Reuse Catalog Table Slot
                     * Property를 통해 재활용 여부를 선택할 수 있어야 한다. */
                    if ( smuProperty::getCatalogSlotReusable() == 1 )
                    {
                        IDE_TEST( smmManager::getOIDPtr(
                                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                              aOIDInfoPtr->mTableOID,
                                              (void**)&sRowPtr )
                                  != IDE_SUCCESS );

                        IDE_TEST( smcRecord::setFreeFixRowPending( aTransPtr,
                                                                   sCatalogTable,
                                                                   sRowPtr,
                                                                   aSCN )
                                  != IDE_SUCCESS);
                    }
                    else
                    {
                        /* Dictionary Table Slot을 재활용하지 않는다. */
                    }
                }
            }
            else
            {
                /* PROJ-2268 Reuse Catalog Table Slot
                 * Sequence, Procedure, Pakage 등에서 사용한 Catalog Slot의 경우
                 * Table Type이 Meta로 적용되기 때문에 따로 처리해 주어야 한다. */
                if ( smuProperty::getCatalogSlotReusable()  == 1 )
                {
                    if ( SMI_TABLE_TYPE_IS_META( sTableHeader ) == ID_TRUE )
                    {
                        IDE_ASSERT( smcTable::isDropedTable4Ager(sTableHeader, aSCN) == ID_TRUE );

                        /* Transaction이 단 하나 존재하고, 해당 Transaction이 Drop을
                         * 수행했을때, Aging List를 전달하고 Transaction의 Lock을 풀기
                         * 때문에, Ager가 LockItem을 먼저 제거한 후 Lock이 풀리는
                         * 경우가 있을 수 있음. 따라서 Table에 XLock을 잡아 동시성
                         * 문제를 제거한 후 (어차피 Drop된 Table이라 성능저하 없음)
                         * Lock을 해제해야 함. */
                        /* 다만 TABLE_LOCK_ENABLE이 꺼져있을 경우에는 CreateTable이
                         * 실패한 경우, Property 끄기 전에 매달린 Job들이 오는 것이지
                         * 이후에는 DDL이 없기 때문에 Lock체크할 필요가 없음 */
                        if ( smuProperty::getTableLockEnable() == 1 )
                        {
                            IDE_TEST( aTransPtr->setImpSavepoint( &sISavepoint, sDummy )
                                      != IDE_SUCCESS );
                            /* BUG-42928 No Partition Lock
                             *  이미 DROP한 Table/Partition이므로, mLock을 사용한다.
                             */
                            IDE_TEST( smlLockMgr::lockTableModeX( aTransPtr,
                                                                  sTableHeader->mLock )
                                      != IDE_SUCCESS );
                            IDE_TEST( aTransPtr->abortToImpSavepoint( sISavepoint )
                                      != IDE_SUCCESS );
                            IDE_TEST( aTransPtr->unsetImpSavepoint( sISavepoint )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            /* nothing to do... */
                        }

                        IDE_TEST( smcTable::finLockItem( sTableHeader )
                                  != IDE_SUCCESS );

                        /* 어차피 free 될 slot이므로 값이 잘못되어도 상관없다.
                         * DASSERT로만 확인한다. */
                        IDE_DASSERT( sTableHeader->mSelfOID == aOIDInfoPtr->mTableOID );

                        /* PROJ-2268 Reuse Catalog Table Slot
                         * Property를 통해 재활용 여부를 선택할 수 있어야 한다. */
                        if ( smuProperty::getCatalogSlotReusable() == 1 )
                        {
                            IDE_TEST( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                             aOIDInfoPtr->mTableOID,
                                                             (void**)&sRowPtr )
                                      != IDE_SUCCESS );

                            IDE_TEST( smcRecord::setFreeFixRowPending( aTransPtr,
                                                                       sCatalogTable,
                                                                       sRowPtr,
                                                                       aSCN )
                                      != IDE_SUCCESS);
                        }
                        else
                        {
                            /* Dictionary Table Slot을 재활용하지 않는다. */
                        }
                    }
                    else
                    {
                        /* Meta 외의 나머지는 대상으로 하지 않는다. */
                    }
                }
                else
                {
                    /* 재활용하지 않는다면 기존과 동일하게 동작하여야 한다. */
                }
            }
            break;

        case SM_OID_TYPE_DELETE_TABLE_BACKUP:
            /* BUG-16161: Add Column이 실패한후 다시 Add Column을 수행하면
               Session이 Hang상태로 빠집니다.: Transaction이 Commit이나 Abort시에
               Backup File삭제를 수행함.*/
            break;

        default:

            *aIsProcessed  = ID_FALSE;
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsProcessed  = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC smaDeleteThread::shutdown()
{

    mFinished = ID_TRUE;

    IDE_TEST_RAISE(join() != 0, err_thr_join);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Commit이나 Abort후에 FreeSlot을 실제 FreeSlotList에 매단다.
 *
 * BUG-14093
 * freeSlot()에서 slot에 대한 Free작업만 수행하고
 * ager Tx가 commit한 이후에 FreeSlotList에 등록하는 작업을 수행한다.
 */
IDE_RC smaDeleteThread::processFreeSlotPending(smxTrans   *aTrans,
                                               smxOIDList *aOIDList)
{
    smxOIDNode         *sCurOIDNode;
    smxOIDNode         *sNxtOIDNode;
    smxOIDInfo         *sOIDInfo;
    smcTableHeader     *sTableHeader;
    SChar              *sRowPtr;
    UInt                i;
    UInt                sState = 0;
    iduMutex           *sCheckMutex = (iduMutex *)aTrans->mConnectDeleteThread;

    if( aOIDList->isEmpty() == ID_FALSE )
    {
        /* BUG-47367 일반적으로 FreeSlotList에는 Ager Tx만이 add 하나
         * DropPending 등을 수행하기 위해 임시로 할당된 Tx 같은 경우 FreeSlotList를 사용할 수 있다.
         * Ager Tx는 각자 전용 checkMutex를 가지고 작업하고
         * DropPending 등의 Tx의 경우 mCheckMutex4Self를 사용하도록 한다.
         */
        if ( sCheckMutex == NULL )
        {
            sCheckMutex = &mCheckMutex4Self; 
        }

        /* BUG-15047 */
        IDE_ASSERT( sCheckMutex->lock( NULL ) == IDE_SUCCESS );
        sState = 1;

        // OIDList의 Head는 NULL Node부터 시작하므로 Head의 Next Node부터 시작
        sCurOIDNode = aOIDList->mOIDNodeListHead.mNxtNode;

        // CurOIDNode가 순회해서 다시 OIDList의 Head로 돌아오면 끝낸다.
        while(sCurOIDNode != &(aOIDList->mOIDNodeListHead))
        {
            for(i = 0; i < sCurOIDNode->mOIDCnt; i++)
            {
                sOIDInfo     = sCurOIDNode->mArrOIDInfo + i;
                IDE_ASSERT( smcTable::getTableHeaderFromOID( sOIDInfo->mTableOID,
                                                             (void**)&sTableHeader )
                            == IDE_SUCCESS );
                /* BUG-15969: 메모리 Delete Thread에서 비정상 종료 */
                if(smcTable::isDropedTable4Ager( sTableHeader, sOIDInfo->mSCN ) == ID_FALSE)
                {
                    /* PROJ-1594 Volatile TBS */
                    IDE_ASSERT( smmManager::getOIDPtr( sOIDInfo->mSpaceID,
                                                       sOIDInfo->mTargetOID, 
                                                       (void**)&sRowPtr)
                                == IDE_SUCCESS );

                    IDE_ASSERT( sTableHeader->mSpaceID == sOIDInfo->mSpaceID );

                    switch(sOIDInfo->mFlag)
                    {
                        case SM_OID_TYPE_FREE_FIXED_SLOT :
                            if( SMI_TABLE_TYPE_IS_VOLATILE( sTableHeader ) == ID_TRUE )
                            {
                                IDE_TEST(svcRecord::freeFixRowPending(
                                                             aTrans,
                                                             sTableHeader,
                                                             sRowPtr)
                                         != IDE_SUCCESS);
                            }
                            else
                            {
                                IDE_TEST(smcRecord::freeFixRowPending(
                                                             aTrans,
                                                             sTableHeader,
                                                             sRowPtr)
                                         != IDE_SUCCESS);
                            }
                            break;

                        case SM_OID_TYPE_FREE_VAR_SLOT :
                            if( SMI_TABLE_TYPE_IS_VOLATILE( sTableHeader ) == ID_TRUE )
                            {
                                IDE_TEST(svcRecord::freeVarRowPending(
                                                             aTrans,
                                                             sTableHeader,
                                                             sOIDInfo->mTargetOID,
                                                             sRowPtr)
                                     != IDE_SUCCESS);
                            }
                            else
                            {
                                IDE_TEST(smcRecord::freeVarRowPending(
                                                             aTrans,
                                                             sTableHeader,
                                                             sOIDInfo->mTargetOID,
                                                             sRowPtr)
                                         != IDE_SUCCESS);
                            }
                            break;
                        case SM_OID_TYPE_UNLOCK_FIXED_SLOT :
                            if( SMI_TABLE_TYPE_IS_VOLATILE( sTableHeader ) == ID_TRUE )
                            {
                                IDE_TEST(svcRecord::unlockRow( aTrans,
                                                               sRowPtr )
                                         != IDE_SUCCESS);
                            }
                            else
                            {
                                IDE_TEST(smcRecord::unlockRow( aTrans,
                                                               sOIDInfo->mSpaceID,
                                                               sRowPtr )
                                         != IDE_SUCCESS);
                            }
                            break;
                        default:
                            // OIDFreeSlotList에는 다른 작업이 없다.
                            IDE_ASSERT(0);
                            break;
                    }
                }//for
            }

            sNxtOIDNode = sCurOIDNode->mNxtNode;

            sCurOIDNode->mNxtNode->mPrvNode = sCurOIDNode->mPrvNode;
            sCurOIDNode->mPrvNode->mNxtNode = sCurOIDNode->mNxtNode;

            IDE_TEST(smxOIDList::freeMem(sCurOIDNode) != IDE_SUCCESS);

            sCurOIDNode = sNxtOIDNode;
        }//while

        /* BUG-15047 */
        sState = 0;
        IDE_ASSERT( sCheckMutex->unlock() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( sCheckMutex->unlock() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

IDE_RC  smaDeleteThread::lockAll()
{
    UInt i;
    for ( i = 0; i < smaLogicalAger::mListCnt ; i++ )
    {
        mListLock[i].lock( NULL );
    }

    return IDE_SUCCESS;
}

IDE_RC  smaDeleteThread::unlockAll()
{
    UInt i;
    for ( i = 0; i < smaLogicalAger::mListCnt ; i++ )
    {
        mListLock[i].unlock();
    }
   
    return IDE_SUCCESS; 
}
