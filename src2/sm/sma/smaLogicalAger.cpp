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
 * $Id: smaLogicalAger.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smm.h>
#include <sma.h>
#include <smx.h>
#include <smi.h>
#include <sgmManager.h>

smmSlotList*    smaLogicalAger::mSlotList;
iduMutex        smaLogicalAger::mBlock;
iduMutex        smaLogicalAger::mCheckMutex;
smaOidList**    smaLogicalAger::mHead;
smaOidList**    smaLogicalAger::mTailLogicalAger;
smaOidList**    smaLogicalAger::mTailDeleteThread;
UInt            smaLogicalAger::mListCnt;
smaLogicalAger* smaLogicalAger::mLogicalAgerList;
ULong           smaLogicalAger::mAddCount;
ULong           smaLogicalAger::mHandledCount;
UInt            smaLogicalAger::mCreatedThreadCount;
UInt            smaLogicalAger::mRunningThreadCount;
idBool          smaLogicalAger::mIsInitialized = ID_FALSE;
ULong           smaLogicalAger::mAgingRequestOIDCnt;
ULong           smaLogicalAger::mAgingProcessedOIDCnt;
ULong           smaLogicalAger::mSleepCountOnAgingCondition;
iduMutex        smaLogicalAger::mAgerCountChangeMutex;
iduMutex        smaLogicalAger::mFreeNodeListMutex;
SInt            smaLogicalAger::mBlockFreeNodeCount;
UInt            smaLogicalAger::mIsParallelMode;
iduMutex*       smaLogicalAger::mListLock;


smaLogicalAger::smaLogicalAger() : idtBaseThread()
{

}


IDE_RC smaLogicalAger::initializeStatic()
{

    UInt i;

    mLogicalAgerList = NULL;

    mSlotList = NULL;
    mListCnt  = smuProperty::getAgerListCount(); 

    /* TC/FIT/Limit/sm/sma/smaLogicalAger_alloc_malloc1.sql */
    IDU_FIT_POINT_RAISE( "smaLogicalAger::alloc::malloc1",
                          insufficient_memory );
    /* TC/FIT/Limit/sm/sma/smaLogicalAger_alloc_malloc2.sql */
    IDU_FIT_POINT_RAISE( "smaLogicalAger::alloc::malloc2",
                          insufficient_memory );
    /* TC/FIT/Limit/sm/sma/smaLogicalAger_alloc_malloc3.sql */
    IDU_FIT_POINT_RAISE( "smaLogicalAger::alloc::malloc3",
                          insufficient_memory );

    //fix bug-23007
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                       ID_SIZEOF(smmSlotList) * mListCnt ,
                                       (void**)&(mSlotList)) != IDE_SUCCESS,
                    insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                       ID_SIZEOF(smaOidList*) * mListCnt ,
                                       (void**)&(mHead)) != IDE_SUCCESS,
                    insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                       ID_SIZEOF(smaOidList*) * mListCnt ,
                                       (void**)&(mTailLogicalAger)) != IDE_SUCCESS,
                    insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                       ID_SIZEOF(smaOidList*) * mListCnt ,
                                       (void**)&(mTailDeleteThread)) != IDE_SUCCESS,
                    insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMA,
                                       ID_SIZEOF(iduMutex) * mListCnt ,
                                       (void**)&(mListLock)) != IDE_SUCCESS,
                    insufficient_memory );

    for( i = 0; i < mListCnt; i++)
    {
        /* BUG-47367 기존에는 SlotList를 3개 만들어서 할당용 반납용이 따로 있었지만
         * 이제는 각 List별로 따로 List를 두어 addList시 병목을 분산시킨다.
         * List당 1개 이기 때문에 할당/반납시에 Ager와 Tx간의 간섭은 발생한다. */
        IDE_TEST( mSlotList[i].initialize( IDU_MEM_SM_SMA_LOGICAL_AGER,
                                           "SMA_AGER_OID_LIST",
                                           ID_SIZEOF( smaOidList ),
                                           SMA_NODE_POOL_MAXIMUM,
                                           SMA_NODE_POOL_CACHE )
                  != IDE_SUCCESS );

        mHead[i]             = NULL;
        mTailLogicalAger[i]  = NULL;
        mTailDeleteThread[i] = NULL;

        IDE_TEST( mListLock[i].initialize( (SChar*)"MEMORY_LOGICAL_GC_MUTEX",
                                           IDU_MUTEX_KIND_NATIVE,
                                           IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );
    }

    IDE_TEST( mBlock.initialize((SChar*)"MEMORY_LOGICAL_GC_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( mCheckMutex.initialize((SChar*)"MEMORY_LOGICAL_GC_CHECK_MUTEX",
                                     IDU_MUTEX_KIND_NATIVE,
                                     IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );

    IDE_TEST( mAgerCountChangeMutex.initialize(
                                  (SChar*)"MEMORY_LOGICAL_GC_COUNT_CHANGE_MUTEX",
                                  IDU_MUTEX_KIND_NATIVE,
                                  IDV_WAIT_INDEX_NULL) 
              != IDE_SUCCESS );

    IDE_TEST( mFreeNodeListMutex.initialize(
                                (SChar*)"MEMORY_LOGICAL_GC_FREE_NODE_LIST_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL) 
              != IDE_SUCCESS );


    mBlockFreeNodeCount = 0;

    mAddCount = 0;
    mHandledCount = 0;
    mSleepCountOnAgingCondition = 0;

    mAgingRequestOIDCnt = 0;
    mAgingProcessedOIDCnt = 0;


    /* TC/FIT/Limit/sm/sma/smaLogicalAger_alloc_malloc4.sql */
    IDU_FIT_POINT_RAISE( "smaLogicalAger::alloc::malloc4",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SMA,
                                 ID_SIZEOF(smaLogicalAger) *
                                 smuProperty::getMaxLogicalAgerCount(),
                                 (void**)&(mLogicalAgerList)) != IDE_SUCCESS,
                    insufficient_memory );

    mCreatedThreadCount = smuProperty::getLogicalAgerCount();
    mRunningThreadCount = mCreatedThreadCount;


    IDE_ASSERT( mCreatedThreadCount <= smuProperty::getMaxLogicalAgerCount() );

    /* BUG-35179 Add property for parallel logical ager */
    mIsParallelMode = smuProperty::getParallelLogicalAger();
        
    for(i = 0; i < mCreatedThreadCount; i++)
    {
        new (mLogicalAgerList + i) smaLogicalAger;
        IDE_TEST(mLogicalAgerList[i].initialize(i) != IDE_SUCCESS);
    }

    mIsInitialized = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smaLogicalAger::destroyStatic()
{

    UInt i;

    for(i = 0; i <  mCreatedThreadCount; i++)
    {
        IDE_TEST( mLogicalAgerList[i].destroy() != IDE_SUCCESS );
    }

    IDE_TEST( mBlock.destroy() != IDE_SUCCESS );
    IDE_TEST( mCheckMutex.destroy() != IDE_SUCCESS );
    IDE_TEST( mAgerCountChangeMutex.destroy() != IDE_SUCCESS );
    IDE_TEST( mFreeNodeListMutex.destroy() != IDE_SUCCESS );

    for ( i = 0; i < mListCnt; i++ )
    {    
        IDE_TEST( mSlotList[i].release( ) != IDE_SUCCESS );
        IDE_TEST( mSlotList[i].destroy( ) != IDE_SUCCESS );

        IDE_TEST( mListLock[i].destroy() != IDE_SUCCESS );
    }

    IDE_TEST( iduMemMgr::free( mListLock ) != IDE_SUCCESS );
    mListLock = NULL;

    IDE_TEST( iduMemMgr::free( mTailDeleteThread ) != IDE_SUCCESS );
    mTailDeleteThread = NULL;

    IDE_TEST( iduMemMgr::free( mTailLogicalAger ) != IDE_SUCCESS );
    mTailLogicalAger = NULL;

    IDE_TEST( iduMemMgr::free( mHead ) != IDE_SUCCESS );
    mHead = NULL;

    IDE_TEST( iduMemMgr::free(mSlotList) != IDE_SUCCESS );
    mSlotList = NULL;

    IDE_TEST( iduMemMgr::free(mLogicalAgerList) != IDE_SUCCESS );
    mLogicalAgerList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smaLogicalAger::shutdownAll()
{
    UInt i;

    for(i = 0; i < mCreatedThreadCount; i++)
    {
        IDE_TEST(mLogicalAgerList[i].shutdown() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaLogicalAger::initialize( UInt aThreadID )
{
    SChar           sMutexName[128];

    mContinue         = ID_TRUE;

    mTimeOut = smuProperty::getAgerWaitMax();
    mThreadID = aThreadID;
    
    idlOS::snprintf( sMutexName,
                     128,
                     "MEMORY_LOGICAL_GC_THREAD_MUTEX_%"ID_UINT32_FMT,
                     aThreadID );

    /* BUG-35179 Add property for parallel logical ager
     * drop table, drop tablespace와의 동시성 문제를 해결하기 위해 각 thread별로
     * lock을 추가한다. */
    IDE_TEST( mWaitForNoAccessAftDropTblLock.initialize(
                                        sMutexName,
                                        IDU_MUTEX_KIND_NATIVE,
                                        IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    SM_MAX_SCN( &mViewSCN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smaLogicalAger::destroy( void )
{
    /* BUG-35179 Add property for parallel logical ager
     * drop table, drop tablespace와의 동시성 문제를 해결하기 위해 각 thread별로
     * lock을 추가한다. */
    IDE_TEST( mWaitForNoAccessAftDropTblLock.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smaLogicalAger::shutdown( void )
{


    // BUGBUG smi interface layer구현되면 그때 하자.
    //IDE_RC _smiSetAger( idBool aValue );

    mContinue = ID_FALSE;
    // BUGBUG smi interface layer구현되면 그때 하자.
    //IDE_TEST( _smiSetAger( ID_TRUE ) != IDE_SUCCESS );

    IDE_TEST( setAger( ID_TRUE ) != IDE_SUCCESS );

    IDE_TEST_RAISE( join() != IDE_SUCCESS, ERR_THREAD_JOIN );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_THREAD_JOIN );
    IDE_SET( ideSetErrorCode( smERR_FATAL_Systhrjoin ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smaLogicalAger::setAger( idBool aValue )
{
    static idBool sValue = ID_TRUE;

    if( aValue != sValue )
    {
        if( aValue == ID_TRUE )
        {
            IDE_TEST( smaLogicalAger::unblock() != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( smaLogicalAger::block() != IDE_SUCCESS );
        }
        sValue = aValue;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
    Ager가 초기화되었는지 여부를 리턴한다.

    BLOCK/UNBLOCK을 호출하기 전에 이 함수를 호출하여
    AGER가 초기화되었는지를 검사할 수 있다.

    Ager가 초기화되지 않은 META이전 단계에서 BLOCK/UNBLOCK을
    호출하지 않도록 하기 위해 사용된다.
*/
idBool smaLogicalAger::isInitialized( void )
{
    return mIsInitialized;
}


IDE_RC smaLogicalAger::block( void )
{
    UInt sStage = 0;
    UInt i = 0;

    // Ager가 초기화 된 META상태에서만 Block/Unblock이 호출되어야 한다.
    IDE_ASSERT( mIsInitialized == ID_TRUE );

    /* 모든 Ager를 멈춰야 한다. 모든 List의 lock을 획득한다. */
    for ( i = 0; i< mListCnt; i++ )
    {
        mListLock[i].lock(NULL);
    }

    sStage = 1;

    IDE_TEST( smaDeleteThread::lockAll() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sStage )
    {
        case 1:
        for ( i = 0; i < mListCnt; i++ )
        {
            mListLock[i].unlock();
        }
        break;
    }
    IDE_POP();

    return IDE_FAILURE;

}

IDE_RC smaLogicalAger::unblock( void )
{

    UInt sStage = 1;
    UInt i = 0;

    // Ager가 초기화 된 META상태에서만 Block/Unblock이 호출되어야 한다.
    IDE_ASSERT( mIsInitialized == ID_TRUE );

    IDE_TEST( smaDeleteThread::unlockAll() != IDE_SUCCESS );

    sStage = 0;
    for ( i = 0; i< mListCnt; i++ )
    {
        mListLock[i].unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sStage )
    {
        case 1:
        for ( i = 0; i < mListCnt; i++ )
        {
            mListLock[i].unlock();
        }
        break;
    }
    IDE_POP();

    return IDE_FAILURE;

}

IDE_RC smaLogicalAger::addList( smTID         aTID,
                                idBool        aIsDDL,
                                smSCN       * aSCN,
                                UInt          aCondition,
                                smxOIDList  * aList,    
                                void       ** aAgerListPtr ) 
{

    smaOidList* sOidList;
    SInt        sListN;

#ifdef DEBUG
        if( aCondition == 0 ) // for dummy
        {
            IDE_DASSERT( SM_SCN_IS_INIT( *aSCN ) );
        }
        else
        {
            IDE_DASSERT( (aCondition == SM_OID_ACT_AGING_COMMIT)   ||
                         (aCondition == SM_OID_ACT_AGING_ROLLBACK) ); 
            IDE_DASSERT( SM_SCN_IS_NOT_INIT( *aSCN ) );
        }
#endif

    sListN = aTID % mListCnt;

    if( aAgerListPtr != NULL )
    {
        *(smaOidList**)aAgerListPtr = NULL;
    }

    /* SlotList가 할당되었는지 확인 */
    if( mSlotList != NULL )
    {
        /* BUG-47367 Slot lock을 획득한다.
         * 해당 lock은 할당/반납/AgerList에 등록을 제어 한다. */
        mSlotList[sListN].lock();

        IDE_TEST( mSlotList[sListN].allocateSlots( 1,
                                                   (smmSlot**)&sOidList,
                                                   SMM_SLOT_LIST_MUTEX_NEEDLESS )
                  != IDE_SUCCESS );

        sOidList->mSCN       = *aSCN;
        sOidList->mErasable  = ID_FALSE;
        sOidList->mCondition = aCondition;
        sOidList->mNext      = NULL;
        sOidList->mTransID   = aTID;
        sOidList->mListN     = sListN; /* BUG-47367 반납할때 List번호를 알아야 한다. */
        SM_SET_SCN_INFINITE_AND_TID(&(sOidList->mKeyFreeSCN), aTID);


        if(aAgerListPtr != NULL)
        {
            sOidList->mFinished  = ID_FALSE;
        }
        else
        {
            sOidList->mFinished  = ID_TRUE;
        }

        // BUG-15306
        // DummyOID일 경우엔 add count를 증가시키지 않는다.
        if( isDummyOID( sOidList ) != ID_TRUE )
        {
            acpAtomicInc64( &mAddCount );
        }

        if(aList != NULL)
        {
            IDE_ASSERT(aList->mOIDNodeListHead.mNxtNode != &(aList->mOIDNodeListHead));
            sOidList->mHead = aList->mOIDNodeListHead.mNxtNode;
            sOidList->mTail = aList->mOIDNodeListHead.mPrvNode;
        }
        else
        {
            sOidList->mHead = NULL;
            sOidList->mTail = NULL;
        }

        IDL_MEM_BARRIER;

        if( mHead[sListN] != NULL )
        {
            mHead[sListN]->mNext = sOidList;
            mHead[sListN]        = sOidList;
        }
        else
        {
            mHead[sListN]             = sOidList;
            mTailLogicalAger[sListN]  = mHead[sListN];
            mTailDeleteThread[sListN] = mHead[sListN];
        }

        mSlotList[sListN].unlock();

        if(aIsDDL == ID_TRUE)
        {
            IDE_TEST( addList( aTID,   
                               ID_FALSE,  // aIsDDL
                               aSCN, 
                               aCondition, 
                               NULL,      // aList,
                               NULL )     // aAgerListPtr
                      != IDE_SUCCESS );
        }
        
        if(aAgerListPtr != NULL)
        {
            *(smaOidList**)aAgerListPtr = sOidList;

        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/************************************************************************
 * Description :
 *    OID list에 OID가 하나만 달려 있으면 이 함수를 통해서
 *    dummy OID를 하나 추가한다. 이렇게 함으로써
 *    OID list에 처리 안되고 남아 있는 하나의 OID를 처리하도록 한다.
 *
 *    BUG-15306을 위해 추가된 함수
 ************************************************************************/
IDE_RC smaLogicalAger::addDummyOID( SInt aListN )
{
    SInt  sState = 0;
    smSCN sNullSCN;

    // OID list가 하나만 있으면 이 함수가 불려진다.
    // 하지만 OID list는 수시로 add되고 있기 때문에
    // mutex를 잡고 다시한번 검사해야 한다.

    // addList와 동시성 제어기 때문에 mSlotList의 lock을 획득해야 한다.
    mSlotList[aListN].lock();
    sState = 1;

    if( ( mHead[aListN] != NULL )                     &&
        ( mTailLogicalAger[aListN] == mHead[aListN] ) &&
        ( mTailLogicalAger[aListN]->mNext == NULL )   &&
        ( isDummyOID( mHead[aListN] ) == ID_FALSE ) )
    {
        SM_INIT_SCN( &sNullSCN );

        /* BUG-47367 addList에서 동일한 lock을 획득하려 하기 때문에
         * 호출전에 풀어주어야한다.
         * 인자 추가로 내부에서 판단해도 되지만
         * dummy가 중간에 있다고 큰 문제는 안되기 때문에 lock 구간을 줄였다. */
        sState = 0;

        mSlotList[aListN].unlock();

        IDE_TEST( addList( aListN,         // 원래 TID 이지만 내부에서 TID를 이용 ListN로 바뀌기 때문에 상관없다
                           ID_FALSE,       // aIsDDL
                           &sNullSCN,      // aSCN
                           0,              // aCondition
                           NULL,           // aList,
                           NULL )          // aAgerListPtr 
                  != IDE_SUCCESS );
    }
    else
    {
        sState = 0;
        mSlotList[aListN].unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        mSlotList[aListN].unlock();
    }

    return IDE_FAILURE;
}

void smaLogicalAger::setOIDListFinished( void* aOIDList, idBool aFlag)
{
    ((smaOidList*)aOIDList)->mFinished = aFlag;
}

/*
    Aging할 OID를 가리키는 Index Slot을 제거한다.
    logical ager에서 호출한다.

    [IN] aOIDList          - Aging할 OID List
    [OUT] aDidDelOldKey -
 */
IDE_RC smaLogicalAger::deleteIndex(smaOidList            * aOIDList,
                                   UInt                  * aDidDelOldKey )
{

    smxOIDNode*     sNode;
    smxOIDInfo*     sCursor = NULL;
    smxOIDInfo*     sFence;
    smcTableHeader* sLastTable;
    smcTableHeader* sTable;
    smcTableHeader* sNextTable;
    smcTableHeader* sCurTable = NULL;
    UInt            sStage = 0;

    sLastTable  = NULL;
    sTable      = NULL;
    sNode       = aOIDList->mTail;

    while( sNode != NULL )
    {
        sCursor = sNode->mArrOIDInfo + sNode->mOIDCnt - 1;
        sFence  = sNode->mArrOIDInfo - 1 ;
        for( ; sCursor != sFence; sCursor-- )
        {
            if( smxOIDList::checkIsAgingTarget( aOIDList->mCondition,
                                                sCursor ) == ID_FALSE )
            {
                continue;
            }

            idCore::acpAtomicInc64( &mAgingProcessedOIDCnt );
            IDE_DASSERT( checkAgingProcessedCnt() == ID_TRUE );

            if ( (( sCursor->mFlag & SM_OID_ACT_AGING_INDEX ) == SM_OID_ACT_AGING_INDEX ) &&
                 (( sCursor->mFlag & SM_OID_ACT_COMPRESSION ) == 0) )
            {
                IDE_ASSERT( sCursor->mTableOID != SM_NULL_OID );

                if (( sCurTable == NULL ) ||
                    ( sCursor->mTableOID != sCurTable->mSelfOID ))
                {
                    IDE_ASSERT( smcTable::getTableHeaderFromOID( sCursor->mTableOID,
                                                                 (void**)&sCurTable )
                                == IDE_SUCCESS );
                }

                if( sCurTable != sLastTable )
                {
                    sNextTable = sCurTable;

                    if(smcTable::isDropedTable4Ager( sNextTable , aOIDList->mSCN ) == ID_TRUE)
                    {
                        continue;
                    }

                    if( sStage == 1)
                    {
                        sStage = 0;
                        IDE_TEST( smcTable::unlatch( sTable )
                                  != IDE_SUCCESS );
                    }

                    sLastTable  = sNextTable;
                    sTable      = sNextTable;

                    IDE_TEST( smcTable::latchShared( sTable )
                              != IDE_SUCCESS );
                    sStage = 1;
                }

                /* Fit Wait */
                IDU_FIT_POINT( "2.BUG-41026@smaLogicalAger::run::wakeup_1" );
                IDU_FIT_POINT( "3.BUG-41026@smaLogicalAger::run::sleep_2" );

                IDE_ASSERT( sCursor->mTargetOID != SM_NULL_OID );
                IDE_ASSERT( sTable != NULL );

                /* Fit  Wait */
                IDU_FIT_POINT( "1.PROJ-1407@smaLogicalAger::deleteIndex" );

                //old version row를 가리켰던 index들의 key slot을 해제한다.
                IDE_TEST_RAISE( freeOldKeyFromIndexes( sTable,
                                                       sCursor->mTargetOID,
                                                       aDidDelOldKey )
                                != IDE_SUCCESS, ERR_FREE_OIDKEY_FROM_IDX );
                sCursor->mFlag &= ~SM_OID_ACT_AGING_INDEX;
            }
        }
        sNode = sNode->mPrvNode;
    }

    if( sStage == 1 )
    {
        sStage = 0;
        IDE_TEST( smcTable::unlatch( sTable ) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FREE_OIDKEY_FROM_IDX );
    {
        sCursor->mFlag &= ~SM_OID_ACT_AGING_INDEX;
    }
    IDE_EXCEPTION_END;

    /* BUG-47619 디버깅 정보 보강, free slot에 필요한 ager 정보 출력*/
    ideLog::log( IDE_ERR_0,
                 "smaLogicalAger::deleteIndex\n"
                 "__PARALLEL_LOGICAL_AGER : %"ID_UINT32_FMT"\n"
                 "LOGICAL_AGER_COUNT_     : %"ID_UINT32_FMT"\n"
                 "MIN_LOGICAL_AGER_COUNT  : %"ID_UINT32_FMT"\n"
                 "MAX_LOGICAL_AGER_COUNT  : %"ID_UINT32_FMT"\n",
                 smuProperty::getParallelLogicalAger(),
                 smuProperty::getLogicalAgerCount(),
                 smuProperty::getMinLogicalAgerCount(),
                 smuProperty::getMaxLogicalAgerCount() );

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
     * index aging. */
    if( sCursor != NULL )
    {
        ideLog::log( IDE_ERR_0, 
                     "smaLogicalAger::deleteIndex\n"
                     "sCursor->mTableOID  : %"ID_UINT64_FMT"\n"
                     "sCursor->mTargetOID : %"ID_UINT64_FMT"\n"
                     "sCursor->mSpaceID   : %"ID_UINT32_FMT"\n"
                     "sCursor->mFlag      : %"ID_UINT32_FMT"\n",
                     sCursor->mTableOID,
                     sCursor->mTargetOID,
                     sCursor->mSpaceID,
                     sCursor->mFlag );
    }
  
    if ( sStage == 1 )
    {
        sStage = 0;
        IDE_ASSERT( smcTable::unlatch( sTable ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*
    Aging할 OID를 가리키는 Index Slot을 제거한다.
    DDL 등에서 임시로 해당 테이블에 대한 것을 모두 정리 할 때 호출 된다.
    BUG-47637 에서 deleteIndex에서 분리되었다.

    [IN] aOIDList          - Aging할 OID List
    [IN] aAgingFilter      - Instant Aging시 적용할 Filter
                             ( smaDef.h주석참고 )
    [OUT] aDidDelOldKey -
 */
IDE_RC smaLogicalAger::deleteIndexInstantly( smaOidList            * aOIDList,
                                             smaInstantAgingFilter * aAgingFilter,
                                             UInt                  * aDidDelOldKey )
{

    smxOIDNode*     sNode;
    smxOIDInfo*     sCursor = NULL;
    smxOIDInfo*     sFence;
    smcTableHeader* sLastTable;
    smcTableHeader* sTable;
    smcTableHeader* sNextTable;
    smcTableHeader* sCurTable = NULL;
    UInt            sStage    = 0;

    sLastTable  = NULL;
    sTable      = NULL;
    sNode       = aOIDList->mTail;

    while( sNode != NULL )
    {
        sCursor = sNode->mArrOIDInfo + sNode->mOIDCnt - 1;
        sFence  = sNode->mArrOIDInfo - 1 ;
        for( ; sCursor != sFence; sCursor-- )
        {
            /* BUG-47637: SM_OID_DROP_INDEX처럼 Aging 대상이지만 
             * old version 해제가 필요하지 않은 경우 */
            if( smxOIDList::checkIsAgingTarget( aOIDList->mCondition,
                                                sCursor ) == ID_FALSE )
            {
                continue;
            }

            if ( isAgingFilterTrue( aAgingFilter,
                                    sCursor->mSpaceID,
                                    sCursor->mTableOID ) == ID_FALSE )
            {
                continue;
            }

            IDE_ASSERT( sCursor->mTableOID != SM_NULL_OID );

            if (( sCurTable == NULL ) ||
                ( sCursor->mTableOID != sCurTable->mSelfOID ))
            {
                IDE_ASSERT( smcTable::getTableHeaderFromOID( sCursor->mTableOID,
                                                             (void**)&sCurTable )
                            == IDE_SUCCESS );
            }

            if( sCurTable != sLastTable )
            {
                sNextTable = sCurTable;

                if(smcTable::isDropedTable4Ager( sNextTable , aOIDList->mSCN ) == ID_TRUE)
                {
                    continue;
                }

                if( sStage == 1)
                {
                    sStage = 0;
                    IDE_TEST( smcTable::unlatch( sTable )
                              != IDE_SUCCESS );
                }

                sLastTable  = sNextTable;
                sTable      = sNextTable;

                IDE_TEST( smcTable::latchShared( sTable )
                          != IDE_SUCCESS );
                sStage = 1;
            }

            /* BUG-47637 여기까지 통과하면 delete thread aging 대상이다.
             * index가 아니더라도 여기에서 count 한다. */
            idCore::acpAtomicInc64( &mAgingProcessedOIDCnt );
            IDE_DASSERT( checkAgingProcessedCnt() == ID_TRUE );

            if ( (( sCursor->mFlag & SM_OID_ACT_AGING_INDEX ) == SM_OID_ACT_AGING_INDEX  ) &&
                 (( sCursor->mFlag & SM_OID_ACT_COMPRESSION ) == 0) )
            {
                IDE_ASSERT( sCursor->mTargetOID != SM_NULL_OID );
                IDE_ASSERT( sTable != NULL );

                /* old version row를 가리켰던 index들의 key slot을 해제한다.*/
                IDE_TEST( freeOldKeyFromIndexes( sTable,
                                                 sCursor->mTargetOID,
                                                 aDidDelOldKey ));

                /* 다음번에 Aging될 때 Index Slot에 대해
                 * Aging을 한번 더 하는일이 없도록 Flag를 끈다. */
                sCursor->mFlag &= ~SM_OID_ACT_AGING_INDEX;
            }
        }
        sNode = sNode->mPrvNode;
    }

    if( sStage == 1 )
    {
        sStage = 0;
        IDE_TEST( smcTable::unlatch( sTable ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-47619 디버깅 정보 보강, free slot에 필요한 ager 정보 출력*/
    ideLog::log( IDE_ERR_0,
                 "smaLogicalAger::deleteIndexInstantly\n"
                 "__PARALLEL_LOGICAL_AGER : %"ID_UINT32_FMT"\n"
                 "LOGICAL_AGER_COUNT_     : %"ID_UINT32_FMT"\n"
                 "MIN_LOGICAL_AGER_COUNT  : %"ID_UINT32_FMT"\n"
                 "MAX_LOGICAL_AGER_COUNT  : %"ID_UINT32_FMT"\n",
                 smuProperty::getParallelLogicalAger(),
                 smuProperty::getLogicalAgerCount(),
                 smuProperty::getMinLogicalAgerCount(),
                 smuProperty::getMaxLogicalAgerCount() );

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
     * index aging. */
    if( sCursor != NULL )
    {
        ideLog::log( IDE_ERR_0, 
                     "smaLogicalAger::deleteIndexInstantly\n"
                     "sCursor->mTableOID  : %"ID_UINT64_FMT"\n"
                     "sCursor->mTargetOID : %"ID_UINT64_FMT"\n"
                     "sCursor->mSpaceID   : %"ID_UINT32_FMT"\n"
                     "sCursor->mFlag      : %"ID_UINT32_FMT"\n",
                     sCursor->mTableOID,
                     sCursor->mTargetOID,
                     sCursor->mSpaceID,
                     sCursor->mFlag );
    }

    if ( sStage == 1 )
    {
        sStage = 0;
        IDE_ASSERT( smcTable::unlatch( sTable ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/*********************************************************************
  Name: sdaGC::freeOldKeyFromIndexes
  Description:
  old verion  row를 가리켰던 old index key slot을
  table 의 인덱스(들)으로 부터 free한다.
 *********************************************************************/
IDE_RC smaLogicalAger::freeOldKeyFromIndexes(smcTableHeader*  aTableHeader,
                                             smOID            aOldVersionRowOID,
                                             UInt*            aDidDelOldKey)
{
    UInt             i ;
    UInt             sIndexCnt;
    smnIndexHeader * sIndexHeader;
    SChar          * sRowPtr;
    idBool           sIsExistFreeKey = ID_FALSE;
   
    IDE_ASSERT( smmManager::getOIDPtr( aTableHeader->mSpaceID,
                                       aOldVersionRowOID, 
                                       (void**)&sRowPtr)
                == IDE_SUCCESS );

    sIndexCnt =  smcTable::getIndexCount(aTableHeader);

    for(i = 0; i < sIndexCnt ; i++)
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex(aTableHeader,i);

        if( smnManager::isIndexEnabled( sIndexHeader ) == ID_TRUE )
        {
            IDE_ASSERT( sIndexHeader->mHeader != NULL );
            IDE_ASSERT( sIndexHeader->mModule != NULL );
            IDE_ASSERT( sIndexHeader->mModule->mFreeSlot != NULL );

            IDE_TEST( sIndexHeader->mModule->mFreeSlot( sIndexHeader, // aIndex
                                                        sRowPtr,
                                                        ID_FALSE, /*aIgnoreNotFoundKey*/
                                                        &sIsExistFreeKey )
                      != IDE_SUCCESS );

            IDE_ASSERT( sIsExistFreeKey == ID_TRUE );
            
            *aDidDelOldKey |= 1;
        }//if
    }//for

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
  Name: sdaGC::checkOldKeyFromIndexes
  Description:
  old verion  row를 가리켰던 old index key slot이 존재하는지 검사한다.
 *********************************************************************/
IDE_RC smaLogicalAger::checkOldKeyFromIndexes(smcTableHeader*  aTableHeader,
                                              smOID            aOldVersionRowOID )
{
    UInt             i ;
    UInt             sIndexCnt;
    smnIndexHeader * sIndexHeader;
    SChar          * sRowPtr;
    UInt             sState = 0;

    IDE_TEST( smcTable::latchShared( aTableHeader )
              != IDE_SUCCESS );
    sState = 1;

    sIndexCnt    = smcTable::getIndexCount(aTableHeader);

    IDE_ASSERT( smmManager::getOIDPtr( aTableHeader->mSpaceID,
                                       aOldVersionRowOID, 
                                       (void**)&sRowPtr)
                == IDE_SUCCESS );

    for(i = 0; i < sIndexCnt ; i++)
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex(aTableHeader,i);

        if( sIndexHeader->mType == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
        {
            /* MRDB R-Tree는 mExistKey 함수가 구현되어 있지 않다. */
            continue;
        }

        if ( smnManager::isIndexEnabled( sIndexHeader ) == ID_FALSE )
        {
            continue;
        }

        IDE_ASSERT( sIndexHeader->mHeader != NULL );
        IDE_ASSERT( sIndexHeader->mModule != NULL );

        // BUG-47526 Free Slot이 Null일 경우가 Exist Check도 안된다.
        IDE_ASSERT( sIndexHeader->mModule->mFreeSlot != NULL );

        IDE_TEST( smnbBTree::checkExistKey( sIndexHeader, // aIndex
                                            sRowPtr )
                  != IDE_SUCCESS );
    }//for

    sState = 0;
    IDE_TEST( smcTable::unlatch( aTableHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) smcTable::unlatch( aTableHeader );
    }

    return IDE_FAILURE;
}



// added for A4
// smaPhysical ager가 제거됨에 따라
// 추가된 함수이며, free node list를 관리한다.
// 실제 데이터는 smnManager에 있는 List를 사용한다.
// 인덱스 유형(memory b+ tree,R tree)에 따라
// free node list를 각각 가지게 된다.
void smaLogicalAger::addFreeNodes(void*     aFreeNodeList,
                                  smnNode*   aNodes)
{
    smnFreeNodeList * sFreeNodeList;

    IDE_DASSERT( aFreeNodeList != NULL);
    IDE_DASSERT( aNodes != NULL);

    IDE_ASSERT( mFreeNodeListMutex.lock( NULL ) == IDE_SUCCESS );
     
    sFreeNodeList = (smnFreeNodeList*) aFreeNodeList;

    if( aNodes != NULL )
    {
        sFreeNodeList->mAddCnt++;
        if(sFreeNodeList->mAddCnt == 0)
        {
            sFreeNodeList->mHandledCnt = 0;
        }
        // 현재 system view scn을 가져온다.
        // node를 index에서 제거한 후에 view SCN을 설정해야 한다.
        SMX_GET_SYSTEM_VIEW_SCN( &(aNodes->mSCN) );

        if( sFreeNodeList->mHead != NULL )
        {
            // empty node list에 추가.
            aNodes->mNext        = NULL;
            sFreeNodeList->mHead->mNext = aNodes;
            sFreeNodeList->mHead       = aNodes;
        }
        else
        {
            aNodes->mNext  = NULL;
            IDL_MEM_BARRIER;
            sFreeNodeList->mHead = aNodes;
            sFreeNodeList->mTail  = aNodes;
        }
    }

    IDE_ASSERT( mFreeNodeListMutex.unlock() == IDE_SUCCESS );
}

//added for A4.
//index유형별로 있는 free node list들을 traverse하면서
//지울 노드가 있으면 노드를 지운다.
void smaLogicalAger::freeNodesIfPossible()
{

    smnNode*         sTail;
    smnNode*         sHead = NULL;
    smnFreeNodeList* sCurFreeNodeList;
    smSCN            sMinSCN;
    smTID            sDummyTID;
    UInt             i;

    IDE_ASSERT( mFreeNodeListMutex.lock( NULL ) == IDE_SUCCESS );

    if( mBlockFreeNodeCount > 0 )
    {
        /* TASK-4990 changing the method of collecting index statistics
         * 통계정보 재구축 등으로 Index를 조회하는 중.
         * FreeNode를 막음으로써 Node의 재활용을 막고,
         * 이를 통해 통계정보 재구축을 위해 Index Node
         * 순회시 TreeLock등 없이도 진행할 수 있도록 한다.*/
    }
    else
    {

        for(sCurFreeNodeList =  smnManager::mBaseFreeNodeList.mNext;
            sCurFreeNodeList != &smnManager::mBaseFreeNodeList;
            sCurFreeNodeList = sCurFreeNodeList->mNext)
        {
            if(sCurFreeNodeList->mTail == NULL)
            {
                continue;
            }

            // fix BUG-9620.
            ID_SERIAL_BEGIN(
                    sHead = sCurFreeNodeList->mHead );
            ID_SERIAL_END(
                    SMX_GET_MIN_MEM_VIEW( &sMinSCN, &sDummyTID ) );

            // BUG-39122 parallel logical aging에서는 logical ager의
            // view SCN도 고려해야 합니다.
            if( mIsParallelMode == SMA_PARALLEL_LOGICAL_AGER_ON )
            {
                for( i = 0; i < mCreatedThreadCount; i++ )
                {
                    if( SM_SCN_IS_LT( &(mLogicalAgerList[i].mViewSCN) ,&sMinSCN ))
                    {
                        SM_SET_SCN( &sMinSCN, &(mLogicalAgerList[i].mViewSCN) );
                    }
                }
            }

            while( sCurFreeNodeList->mTail->mNext != NULL  )
            {
                if(SM_SCN_IS_LT(&(sCurFreeNodeList->mTail->mSCN),&sMinSCN))
                {
                    sTail = sCurFreeNodeList->mTail;
                    sCurFreeNodeList->mTail = sCurFreeNodeList->mTail->mNext;
                    sCurFreeNodeList->mHandledCnt++;
                    IDE_TEST( sCurFreeNodeList->mFreeNodeFunc( sTail )
                              != IDE_SUCCESS );
                    if(sTail == sHead)
                    {
                        break;
                    }

                }//if
                else
                {
                    break;
                }
            }//while
        }//for
    } // if( mBlockFreeNodeCount > 0 )

    IDE_ASSERT( mFreeNodeListMutex.unlock() == IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL( "smaLogicalAger" );

}

void smaLogicalAger::run()
{
    PDL_Time_Value   sTimeOut;
    idBool           sHandled = ID_FALSE;
    smSCN            sMinSCN;
    smaOidList*      sTail = NULL;
    smTID            sDummyTID;
    idBool           sState = ID_FALSE;
    idBool           sState2 = ID_FALSE;
    idBool           sDropTableLockState = ID_FALSE;
    UInt             sAgerWaitMin;
    UInt             sAgerWaitMax;
    UInt             sDidDelOldKey = 0;
    UInt             sListN = mThreadID % smaLogicalAger::mListCnt  ;

  startPos:
    sState              = ID_FALSE;
    sDropTableLockState = ID_FALSE;

    sAgerWaitMin = smuProperty::getAgerWaitMin();
    sAgerWaitMax = smuProperty::getAgerWaitMax();

    while( mContinue == ID_TRUE )
    {
        /* BUG-47601: 전부 순회 하고 나면 Sleep한번.. */
        if ( sListN == (mThreadID % smaLogicalAger::mListCnt) )
        {
            if( sHandled == ID_TRUE )
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

            if( mTimeOut > sAgerWaitMin )
            {
                sTimeOut.set( 0, mTimeOut );
                idlOS::sleep( sTimeOut );
            }
        }

        if ( smuProperty::isRunMemGCThread() == SMU_THREAD_OFF )
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

        /* BUG-35179 Add property for parallel logical ager */
        IDE_TEST( lockWaitForNoAccessAftDropTbl() != IDE_SUCCESS );
        sDropTableLockState = ID_TRUE;

        IDU_FIT_POINT("1.smaLogicalAger::run");

        /* BUG-47367 이번에 처리할 List의 Lock을 획득한다.
         * 각 List별로 Lock을 잡기 때문에 Parallel Ager가 켜져 있지 않아도
         * 서로 다른 Ager가 다른 List의 접근 하는 경우 Parallel 처럼 동작한다.
         * Parallel을 막으려면 List lock대신 전체 lock을 잡도록 해주면 된다.
         */
//        IDE_TEST( mListLock[sListN].lock(NULL) !=IDE_SUCCESS );
//        sState = ID_TRUE;

        mListLock[sListN].trylock( sState );

        if ( sState != ID_TRUE )
        {
            IDE_CONT( skip_list );
        }

        if( mTailLogicalAger[sListN] != NULL )
        {
            // BUG-15306
            // mTailLogicalAger가 하나 밖에 없으면 처리하지 못하므로
            // DummyOID를 하나 달아서 두개가 되게 한다.
            if( ( isDummyOID( mHead[sListN] ) != ID_TRUE ) && ( mTailLogicalAger[sListN]->mNext == NULL ) )
            {
                IDE_TEST( addDummyOID( sListN ) != IDE_SUCCESS );
            }

            // fix BUG-9620.
            SMX_GET_MIN_MEM_VIEW( &sMinSCN, &sDummyTID );

            /* 통계치가 정확한지 검사해봄 */
#if defined(DEBUG)

            /* BUG-47367 List가 여러개 일 경우 동시성 문제가 생길 수 있기 때문에
             * 해당 작업은 List 1개 일때만 동작하도록 한다.*/
            /* 
             * BUG-35179 Add property for parallel logical ager
             * 다음 통계치 검사는 LogicalAger가 parallel로 동작하지
             * 않을 때만 유효하다. parallel로 동작할경우 동시성문제로 잠깐동안
             * 통계치가 일치하지 않을 수 있기 때문이다.
             */
            if( ( mTailLogicalAger[sListN]->mNext == NULL ) &&
                ( isDummyOID( mTailLogicalAger[sListN] ) == ID_TRUE ) &&
                ( mIsParallelMode == SMA_PARALLEL_LOGICAL_AGER_OFF ) &&
                ( mListCnt == 1) )
            {
                /* Transaction과의 동시성을 위해 Lock */
                /* BUG-47367 addList와의 동시성만 제어하면 됨. List가 1개 뿐이라 항상 0임. */
                mSlotList[0].lock();
                /* Lock 후 다시 확인해봄 */
                if( ( mTailLogicalAger[sListN]->mNext == NULL ) &&
                    ( isDummyOID( mTailLogicalAger[sListN] ) == ID_TRUE ) )
                {
                    IDE_ASSERT( mAddCount == mHandledCount );
                    IDE_ASSERT( checkAgingProcessedCnt() == ID_TRUE );
                    IDE_ASSERT( mAgingRequestOIDCnt == mAgingProcessedOIDCnt );
                }
                mSlotList[0].unlock();
            }
#endif

            while( ( mTailLogicalAger[sListN]->mNext != NULL )  &&
                   ( mTailLogicalAger[sListN]->mFinished == ID_TRUE ) )
            {
                if( SM_SCN_IS_LT( &(mTailLogicalAger[sListN]->mSCN), &sMinSCN ) )
                {
                    sTail                    = mTailLogicalAger[sListN];
                    mTailLogicalAger[sListN] = mTailLogicalAger[sListN]->mNext;

                    sDidDelOldKey = 0;

                    /* BUG-35179 Add property for parallel logical ager */
                    if( mIsParallelMode == SMA_PARALLEL_LOGICAL_AGER_ON )
                    {
                        // 다른 ager가 list를 처리할 수 있도록 list lock을 풀어준다.
                        sState = ID_FALSE;
                        IDE_TEST( mListLock[sListN].unlock() != IDE_SUCCESS );

                        /* BUG-39122 parallel logical aging에서는
                         * logical ager의 view SCN도 고려해야 합니다.
                         * 현재 system view scn을 가져온다.*/
                        SMX_GET_SYSTEM_VIEW_SCN( &mViewSCN );
                    }

                    IDE_TEST( deleteIndex( sTail,
                                           &sDidDelOldKey )
                              != IDE_SUCCESS );

                    // delete key slot을 완료한 시점에서
                    // system view scn을 딴다.
                    if( sDidDelOldKey != 0 )
                    {
                        SMX_GET_SYSTEM_VIEW_SCN( &(sTail->mKeyFreeSCN) );
                    }
                    else
                    {
                        // old key를 삭제 안한 경우는 등록시점이
                        // physical aging까지 완료한 시점이다.
                        SM_SET_SCN( &( sTail->mKeyFreeSCN ), &( sTail->mSCN ) );
                    }

                    // BUG-15306
                    // dummy oid를 처리할 땐 mHandledCount를 증가시키지 않는다.
                    if( isDummyOID( sTail ) != ID_TRUE )
                    {
                        /* BUG-41026 */
                        acpAtomicInc( &mHandledCount );
                    }

                    sTail->mErasable = ID_TRUE;
                    sHandled         = ID_TRUE;
                    
                    /* BUG-35179 Add property for parallel logical ager */
                    /* BUG-41026 lock 위치 변경 */
                    if ( mIsParallelMode == SMA_PARALLEL_LOGICAL_AGER_ON )
                    {
                        SM_MAX_SCN( &mViewSCN );

                        IDE_TEST( mListLock[sListN].lock(NULL) != IDE_SUCCESS );
                        sState = ID_TRUE;
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }//if
                else
                {
                    //BUG-17371 [MMDB] Aging이 밀릴경우 System에 과부화 및
                    //                 Aging이 밀리는 현산이 더 심화됨.
                    mSleepCountOnAgingCondition++;
                    break;
                }//else
            }//while

            //free nodes들에 대한  해제시도.
            freeNodesIfPossible();
        }//if

        sState = ID_FALSE;
        IDE_TEST( mListLock[sListN].unlock() != IDE_SUCCESS );

        IDE_EXCEPTION_CONT( skip_list );

        /* BUG-35179 Add property for parallel logical ager */
        sDropTableLockState = ID_FALSE;
        IDE_TEST( unlockWaitForNoAccessAftDropTbl() != IDE_SUCCESS );
        
        sListN++;
        if ( sListN >= mListCnt )
        {
            sListN = 0;
        }
    }

    IDE_TEST( lock() != IDE_SUCCESS );
    sState2 = ID_TRUE;

    mRunningThreadCount--;

    // BUG-47526 Logical Ager를 정리 할 때 node를 모두 정리하는 코드를 제거합니다.
    // Delete Thread 에서 Thread종료시에 모든 Delete를 모두 지울 때
    // SCN을 확인하지 않으므로 필요없는 작업입니다.

    sState2 = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
     * index aging. */
    if( sTail != NULL )
    {
        ideLog::log( IDE_ERR_0, 
                     "smaLogicalAger::run\n"
                     "sTail->mSCN         : %"ID_UINT64_FMT"\n"
                     "sTail->mKeyFreeSCN  : %"ID_UINT64_FMT"\n"
                     "sTail->mErasable    : %"ID_UINT32_FMT"\n"
                     "sTail->mFinished    : %"ID_UINT32_FMT"\n"
                     "sTail->mCondition   : %"ID_UINT32_FMT"\n"
                     "sTail->mTransID     : %"ID_UINT32_FMT"\n",
                     SM_SCN_TO_LONG( sTail->mSCN ),
                     SM_SCN_TO_LONG( sTail->mKeyFreeSCN ),
                     sTail->mErasable,
                     sTail->mFinished,
                     sTail->mCondition,
                     sTail->mTransID );
    }

    IDE_ASSERT( 0 );

    IDE_PUSH();
    
    if( sState == ID_TRUE )
    {
        IDE_ASSERT( mListLock[sListN].unlock() == IDE_SUCCESS );
    }

    if( sState2 == ID_TRUE )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    if( sDropTableLockState == ID_TRUE )
    {
        IDE_ASSERT( unlockWaitForNoAccessAftDropTbl() == IDE_SUCCESS );
    }

    IDE_POP();

    IDE_WARNING(SM_TRC_LOG_LEVEL_WARNNING,
                SM_TRC_MAGER_WARNNING2);

    goto startPos;

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
void smaLogicalAger::waitForNoAccessAftDropTbl()
{
    UInt i;

    /* BUG-35179 Add property for parallel logical ager 
     * drop table, drop tablespace와의 동시성 문제를 해결하기 위해 각 thread별로
     * lock을 추가한다.
     *
     * parallel logical ager코드 추가로인해 기존의 lock(), unlock()함수는
     * OidList와 통계정보를 갱신을 위한 lock역할만 한다. 따라서
     * 각 logical ager thread 별로 존재하는 mWaitForNoAccessAftDropTblLock을
     * 잡아야지만 drop table, drop tablespace와의 동시성 제어가 된다.
     */ 

    for( i = 0; i < mRunningThreadCount; i++ )
    {
        IDE_ASSERT( mLogicalAgerList[i].lockWaitForNoAccessAftDropTbl()
                    == IDE_SUCCESS );

        IDE_ASSERT( mLogicalAgerList[i].unlockWaitForNoAccessAftDropTbl()
                    == IDE_SUCCESS );
    }

    /* BUG-15969: 메모리 Delete Thread에서 비정상 종료
     * Delete Thread도 접근하지 않는다는 것을 보장해야 한다.*/
    smaDeleteThread::waitForNoAccessAftDropTbl();

    /*
       BUG-42760
       LEGACY Transaction의 OID LIst는 AGER 에서 처리하지 않고,
       Transaction이 종료할때 Transactino에서 직접 처리한다.
       따라서, AGER와 마찬가지로 LEGACY Transaction에서도 Drop된 Table의 OID를
       처리하지 않도록 lock을 추가한다.
     */
    smxLegacyTransMgr::waitForNoAccessAftDropTbl();
}



/*
    Aging Filter조건에 부합하는지 체크

    [IN] aAgingFilter - 검사할 Filter 조건
    [IN] aTBSID       - Aging하려는 OID가 속한 Tablespace의 ID
    [IN] aTableOID    - Aging하려는 OID가 속한 Table의 OID

    <RETURN> - 조건에 부합할 경우 ID_TRUE, 아니면 ID_FALSE
 */
idBool smaLogicalAger::isAgingFilterTrue( smaInstantAgingFilter * aAgingFilter,
                                          scSpaceID               aTBSID,
                                          smOID                   aTableOID )
{
    IDE_DASSERT( aAgingFilter != NULL );

    idBool sIsFilterTrue = ID_FALSE;

    // 특정 Tablespace안의 OID에 대해서만 Aging실시?
    if ( aAgingFilter->mTBSID != SC_NULL_SPACEID )
    {
        if(aTBSID == aAgingFilter->mTBSID)
        {
            sIsFilterTrue = ID_TRUE;
        }
    }
    else
    {
        // 특정 Table안의 OID에 대해서만 Aging실시?
        if ( aAgingFilter->mTableOID != SM_NULL_OID )
        {
            if(aTableOID == aAgingFilter->mTableOID)
            {
                sIsFilterTrue = ID_TRUE;
            }
        }
        else
        {
            // Aging Filter에 mTableOID나 mSpaceID중 하나가
            // 설정되어 있어야 한다.

            // 이 경우는 둘다 설정되지 않은 경우이므로
            // ASSERT로 죽인다.
            IDE_ASSERT(0);
        }
    }

    return sIsFilterTrue;
}



/*
    Instant Aging Filter를 초기화한다 ( smaDef.h 참고 )
 */
void smaLogicalAger::initInstantAgingFilter( smaInstantAgingFilter * sAgingFilter )
{
    IDE_DASSERT( sAgingFilter != NULL );

    sAgingFilter->mTBSID    = SC_NULL_SPACEID;
    sAgingFilter->mTableOID = SM_NULL_OID;
}



/*
    특정 Table안의 OID나 특정 Tablespace안의 OID에 대해 즉시 Aging을 실시한다.

    [특이사항]
       Aging하려는 OID의 SCN이 Active Transaction들의 Minimum ViewSCN 보다
       크더라도 Aging을 실시한다.

    [주의사항]
       이 Function을 사용시 다른 쪽에서 Aging Filter에 지정한 Table이나
       Tablespace에 대해서 다른 Transaction이 접근하지 않는다는 것을 보장
       해야 한다.

       Table이나 Tablespace에  X-Lock을 잡는 것으로 이를 보장할 수 있다.

    [IN] aAgingFilter - Aging할 조건 ( smaDef.h 참고 )
 */
IDE_RC smaLogicalAger::agingInstantly( smaInstantAgingFilter * aAgingFilter )
{
    smaOidList*      sCurOIDList;
    UInt             sDidDelOldKey = 0;
    UInt             sListN = 0;
    UInt             sLocked = 0;

    IDE_DASSERT( aAgingFilter != NULL );

    /* BUG-41026 */
    IDU_FIT_POINT( "1.BUG-41026@smaLogicalAger::agingInstantly::sleep_1" );

    /* BUG-47637 Instantly Aging 에서 delete Thread에서 처리하는 것 까지 모두 Lock을 잡아야 한다.
     * Logical Ager의 Processed Count를 counting하는 것은 smaLogicalAger::deleteIndexInstantly() 이고,
     * smaLogicalAger::deleteIndex() 에서 Processed Count를 counting 할 때 참조하는 Flag는
     * smaDeleteThread::deleteInstantly() 에서 제거된다.
     * smaLogicalAger::deleteIndexInstantly() 와 smaDeleteThread::deleteInstantly() 사이에
     * smaLogicalAger::deleteIndex() 가 들어오면 Processed Count가 중복 계산 된다.*/
    for ( sLocked = 0; sLocked < mListCnt; sLocked++ )
    {
        mListLock[ sLocked ].lock(NULL);
    }

    for ( sListN = 0 ; sListN < mListCnt; sListN ++ )
    {
        /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
         * index aging.
         *
         * Ager의 AgingJob은 Head에 달립니다. 그래서 Tail부터 쫓아가야 합니다.
         * 그림으로 표현하자면....
         *
         *      Next Next Next
         * mTail -> A -> B -> C -> mHead
         * 따라서 mHead에서 mNext를 쫓아가면 아무것도 없습니다. */
        sCurOIDList = mTailLogicalAger[sListN];

        while(sCurOIDList != NULL)
        {
            IDE_TEST( deleteIndexInstantly( sCurOIDList,
                                            aAgingFilter,
                                            &sDidDelOldKey )
                      != IDE_SUCCESS );

            sCurOIDList = sCurOIDList->mNext;
        }
    }

    /* delete Thread 내부에서도 모든 List를 탐색하도록 되어 있다. */
    /* BUG-32780 */
    IDE_TEST(smaDeleteThread::deleteInstantly( aAgingFilter )
             != IDE_SUCCESS);

    for ( sListN = 0 ; sListN < sLocked ; sListN++ )
    {
        mListLock[ sListN ].unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for ( sListN = 0 ; sListN < sLocked ; sListN++ )
    {
        mListLock[ sListN ].unlock();
    }

    return IDE_FAILURE;
}


/*
    특정 Tablespace에 속한 OID에 대해서 즉시 Aging실시

    [IN] aTBSID - Aging할 OID가 속한 Tablespace의 ID
 */
IDE_RC smaLogicalAger::doInstantAgingWithTBS( scSpaceID aTBSID )
{
    smaInstantAgingFilter sAgingFilter;

    initInstantAgingFilter( & sAgingFilter );

    sAgingFilter.mTBSID = aTBSID;

    IDE_TEST( agingInstantly( & sAgingFilter ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    특정 Table에 속한 OID에 대해서 즉시 Aging실시

    [IN] aTBSID - Aging할 OID가 속한 Tablespace의 ID
 */
IDE_RC smaLogicalAger::doInstantAgingWithTable( smOID aTableOID )
{
    smaInstantAgingFilter sAgingFilter;

    initInstantAgingFilter( & sAgingFilter );

    sAgingFilter.mTableOID = aTableOID;

    IDE_TEST( agingInstantly( & sAgingFilter ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Ager Thread를 하나 추가한다.
 */
IDE_RC smaLogicalAger::addOneAger()
{
    smaLogicalAger * sEmptyAgerObj = NULL;

    // Shutdown중에는 이 함수가 호출될 수 없다.
    // mRunningThreadCount는 Shutdown시에만 감소되며
    // 시스템 운영중에는 mCreatedThreadCount와 항상 같은 값이어야 한다.
    IDE_ASSERT( mCreatedThreadCount == mRunningThreadCount );
    
    // 하나 증가 직전이므로 smuProperty::getMaxAgerCount() 보다는 작아야함
    IDE_ASSERT( mCreatedThreadCount < smuProperty::getMaxLogicalAgerCount() );
    IDE_ASSERT( mCreatedThreadCount >= smuProperty::getMinLogicalAgerCount() );

    sEmptyAgerObj = & mLogicalAgerList[ mCreatedThreadCount ];

    new (sEmptyAgerObj) smaLogicalAger;
    
    IDE_TEST( sEmptyAgerObj->initialize( mCreatedThreadCount ) != IDE_SUCCESS );

    mCreatedThreadCount++;
    mRunningThreadCount++;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
    
/*
    Ager Thread를 하나 제거한다.
 */
IDE_RC smaLogicalAger::removeOneAger()
{
    smaLogicalAger * sLastAger = NULL;

    // Shutdown중에는 이 함수가 호출될 수 없다.
    // mRunningThreadCount는 Shutdown시에만 감소되며
    // 시스템 운영중에는 mCreatedThreadCount와 항상 같은 값이어야 한다.
    IDE_ASSERT( mCreatedThreadCount == mRunningThreadCount );

    IDE_ASSERT( mCreatedThreadCount <= smuProperty::getMaxLogicalAgerCount() );
    // 하나 감소 직전이므로 smuProperty::getMinAgerCount() 보다는 커야함
    IDE_ASSERT( mCreatedThreadCount > smuProperty::getMinLogicalAgerCount() );

    mCreatedThreadCount--;
    sLastAger = & mLogicalAgerList[ mCreatedThreadCount ];

    // run함수 종료시키고 thread join까지 실시
    IDE_TEST( sLastAger->shutdown() != IDE_SUCCESS );

    // run함수를 빠져나오면서 mRunningThreadCount가 감소된다
    // 그러므로 join후에 Created와 Running갯수가 같아야 한다.
    IDE_ASSERT( mCreatedThreadCount == mRunningThreadCount );
    
    IDE_TEST( sLastAger->destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
    Ager Thread의 갯수를 특정 갯수가 되도록 생성,제거한다.
    
    [IN] aAgerCount - Ager thread 생성,제거후의 Ager갯수

    [참고] 본 함수는 다음과 같은 사용자의
            LOGICAL AGER Thread 수 변경 명령에 의해 수행된다.

            ALTER SYSTEM SET LOGICA_AGER_COUNT_ = 10;
 */
IDE_RC smaLogicalAger::changeAgerCount( UInt aAgerCount )
{

    UInt sStage = 0;

    IDE_ASSERT( mCreatedThreadCount == mRunningThreadCount );

    // 이 함수 호출전에 aAgerCount의 범위 체크 완료한 상태.
    // AgerCount는 Min,Max범위 안쪽에 있어야 한다.
    IDE_ASSERT( aAgerCount <= smuProperty::getMaxLogicalAgerCount() );
    IDE_ASSERT( aAgerCount >= smuProperty::getMinLogicalAgerCount() );

    // mAgerCountChangeMutex 의 역할
    // - ALTER SYSTEM SET LOGICA_AGER_COUNT_ 구문이 동시에 수행되지 않도록
    IDE_TEST( lockChangeMtx() != IDE_SUCCESS );
    sStage = 1;

    while ( mRunningThreadCount < aAgerCount )
    {
        IDE_TEST( addOneAger() != IDE_SUCCESS );
    }

    while ( mRunningThreadCount > aAgerCount )
    {
        IDE_TEST( removeOneAger() != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( unlockChangeMtx() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sStage != 0 )
    {
        IDE_ASSERT( unlockChangeMtx() == IDE_SUCCESS );
    }
    
    IDE_POP();
    
    return IDE_FAILURE;
}
