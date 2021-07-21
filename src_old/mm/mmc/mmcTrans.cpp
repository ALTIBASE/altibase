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

#include <smErrorCode.h>
#include <mmErrorCode.h>
#include <smi.h>
#include <mmcSession.h>
#include <mmcTrans.h>
#include <mmcTransManager.h>
#include <dki.h>
#include <idtContainer.h>
#include <sdi.h>
#include <sdiZookeeper.h>

static inline void setTransactionPrepareSlot( mmcTransObj * aTransObj, smTID aTID )
{
    if ( aTID != MMC_TRANS_NULL_SLOT_NO )
    {
        IDE_DASSERT( aTransObj->mShareInfo->mTxInfo.mState == MMC_TRANS_STATE_BEGIN );

        aTransObj->mShareInfo->mTxInfo.mPrepareSlot = smxTransMgr::getSlotID( aTID );
    }
    else
    {
        aTransObj->mShareInfo->mTxInfo.mPrepareSlot = MMC_TRANS_NULL_SLOT_NO;
    }
}

static inline void setTransactionCommitSCN( mmcTransObj * aTransObj, smSCN * aSCN )
{
    if ( aSCN == NULL )
    {
        SMI_INIT_SCN( &aTransObj->mShareInfo->mTxInfo.mCommitSCN );
    }
    else
    {
        SM_SET_SCN( &aTransObj->mShareInfo->mTxInfo.mCommitSCN, aSCN );
    }
}

static inline void getTransactionCommitSCN( mmcTransObj * aTransObj, smSCN * aSCN )
{
    SM_SET_SCN( aSCN, &aTransObj->mShareInfo->mTxInfo.mCommitSCN );
}

static inline smSCN * getTransactionCommitSCNPtr( mmcTransObj * aTransObj )
{
    return &aTransObj->mShareInfo->mTxInfo.mCommitSCN;
}

/*
 * beginRaw: Session에 대한 후속 처리 없이 기본적인 sm의 begin 및 event block만 처리하는 경우 
 * 사용하는 함수로 internal 로직에서 주로 사용
 */
void mmcTrans::beginRaw(mmcTransObj *aTrans, 
                        idvSQL      *aStatistics, 
                        UInt         aFlag, 
                        ULong       *aSessionEventFlag)
{
    smiStatement *sDummySmiStmt = NULL;
    if ( aTrans->mSmiTrans.isBegin() == ID_FALSE )
    {
        /* session event에 의한 fail을 방지한다. */
        IDU_SESSION_SET_BLOCK(*aSessionEventFlag);
        IDE_ASSERT( aTrans->mSmiTrans.begin( &sDummySmiStmt, 
                                             aStatistics, /* PROJ-2446 */
                                             aFlag,
                                             SMX_NOT_REPL_TX_ID,
                                             ID_FALSE, /* tx alloc 실패시 재시도한다. */
                                             ID_TRUE   /* is service tx */ ) 
                    == IDE_SUCCESS );
        IDU_SESSION_CLR_BLOCK(*aSessionEventFlag);
    }
    else
    {
        /* Nothing to do */
    }
}

/*
 * clearAndSetSessionInfoAfterBegin: Transaction의 begin이후 mmcSession의 정보의 변경이 필요한 경우
 * 사용하는 함수로 internal 로직에서 주로 사용
 */
void mmcTrans::clearAndSetSessionInfoAfterBegin( mmcSession  * aSession,
                                                 mmcTransObj * aTrans )
{
    //PROJ-1677 DEQUEUE
    aSession->clearPartialRollbackFlag();

    /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
    if ( aSession->getLockTableUntilNextDDL() == ID_TRUE )
    {
        aSession->setLockTableUntilNextDDL( ID_FALSE );
        aSession->setTableIDOfLockTableUntilNextDDL( 0 );
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-44967 */
    aSession->getInfo()->mTransID = getTransID( aTrans );
}

/*
 * commitRaw: Session에 대한 후속 처리 없이 기본적인 sm의 commit 및 event block만 처리하는 경우 
 * 사용하는 함수로 internal 로직에서 주로 사용
 */
IDE_RC mmcTrans::commitRaw( mmcTransObj *aTrans,
                            mmcSession  *aSession,
                            ULong       *aEventFlag,
                            UInt         aTransReleasePolicy,
                            smSCN       *aCommitSCN )
{
    if ( aTrans->mSmiTrans.isBegin() == ID_TRUE )
    {
        IDU_SESSION_SET_BLOCK(*aEventFlag);

        setGlobalTxID4Trans( NULL, aSession ); /* BUG-48703 */
        IDE_TEST(aTrans->mSmiTrans.commit(aCommitSCN, aTransReleasePolicy) != IDE_SUCCESS);

        IDU_SESSION_CLR_BLOCK(*aEventFlag);
    }
    else
    {
        /*do nothing*/
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDU_SESSION_CLR_BLOCK(*aEventFlag);

    return IDE_FAILURE;
}

/*
 * rollbackRaw: Session에 대한 후속 처리 없이 기본적인 sm의 rollback 및 event block만 처리하는 경우
 * 사용하는 함수로 internal 로직에서 주로 사용
 */
IDE_RC mmcTrans::rollbackRaw( mmcTransObj *aTrans,
                              mmcSession  *aSession,
                              ULong       *aEventFlag,
                              UInt         aTransReleasePolicy )
{
    if ( aTrans->mSmiTrans.isBegin() == ID_TRUE )
    {
        IDU_SESSION_SET_BLOCK(*aEventFlag);

        setGlobalTxID4Trans( NULL, aSession ); /* BUG-48703 */
        IDE_TEST(aTrans->mSmiTrans.rollback(NULL, aTransReleasePolicy) != IDE_SUCCESS);

        IDU_SESSION_CLR_BLOCK(*aEventFlag);
    }
    else
    {
        /*nothing to do*/
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDU_SESSION_CLR_BLOCK(*aEventFlag);

    return IDE_FAILURE;
}

/*
 * PROJ-2701 share transaction commit
 */
IDE_RC mmcTrans::commitShareableTrans( mmcTransObj *aTrans,
                                       mmcSession  *aSession,
                                       UInt         aTransReleasePolicy,
                                       smSCN       *aCommitSCN,
                                       mmcTransEndAction aTransEndAction )
{
    idBool sIsLock         = ID_FALSE;

    fixSharedTrans( aTrans, aSession->getSessionID() );
    sIsLock = ID_TRUE;

    MMC_SHARED_TRANS_TRACE( aSession,
                            aTrans,
                            "commitShareableTrans: locked" );

    IDU_FIT_POINT_RAISE( "mmcTrans::commitShareableTrans::stateUnexpected",
                         ERR_STATE_UNEXPECTED );

    IDE_TEST_RAISE( ( getLocalTransactionBroken( aTrans ) == ID_TRUE ) &&
                    ( aSession->isGTx() == ID_TRUE ),
                    ERR_TRANS_BROKEN );

    IDE_TEST_CONT( aTransEndAction == MMC_TRANS_SESSION_ONLY_END, SESSION_ONLY_COMMIT );

    /* Shared Transaction FSM: 1PC-Commit */
    switch ( aTrans->mShareInfo->mTxInfo.mState )
    {
        case MMC_TRANS_STATE_PREPARE :
            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "commitShareableTrans: endPendingBySlotN" );
            IDE_TEST( endPendingBySlotN( aTrans,
                                         aSession,
                                         NULL,
                                         ID_TRUE,   /* Commit */
                                         ID_TRUE,   /* Self */
                                         aCommitSCN )
                      != IDE_SUCCESS );

            setTransactionPrepareSlot( aTrans, MMC_TRANS_NULL_SLOT_NO );
            setLocalTransactionBroken( aTrans,
                                       aSession->getSessionID(),
                                       ID_FALSE );
            setTransactionCommitSCN( aTrans, aCommitSCN );
            aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_END;
            break;

        case MMC_TRANS_STATE_BEGIN :
            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "commitShareableTrans: commitRaw" );

            getSmiTrans( aTrans )->setStatistics( aSession->getStatSQL() );

            #if defined(DEBUG)
            ideLog::log( IDE_SD_18, "= [%s] commitShareableTrans CommitSCN : %"ID_UINT64_FMT,
                         aSession->getSessionTypeString(),
                         *aCommitSCN );
            #endif
            if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
                 QC_SESSION_SHARD_META_TOUCH_TRUE )
            {
                /* shard ddl local transaction shard meta touch */
                aSession->setCallbackForReloadNewIncreasedDataSMN(mmcTrans::getSmiTrans(aTrans));
            }
            if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_INTERNAL_TABLE_SWAP_MASK ) ==
                 QC_SESSION_INTERNAL_TABLE_SWAP_TRUE )
            {
                aSession->setInternalTableSwap( mmcTrans::getSmiTrans(aTrans) );
            }

            IDE_TEST( mmcTrans::commitRaw( aTrans,
                                           aSession,
                                           aSession->getEventFlag(),
                                           aTransReleasePolicy,
                                           aCommitSCN )
                      != IDE_SUCCESS );

            setLocalTransactionBroken( aTrans,
                                       aSession->getSessionID(),
                                       ID_FALSE );
            setTransactionCommitSCN( aTrans, aCommitSCN );
            aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_END;
            break;

        case MMC_TRANS_STATE_END :
            getTransactionCommitSCN( aTrans, aCommitSCN );
            break;

        default :
            IDE_RAISE( ERR_STATE_UNEXPECTED );
            break;
    }

    IDE_EXCEPTION_CONT( SESSION_ONLY_COMMIT );

    if ( aSession->getSessionBegin() == ID_TRUE )
    {
        --aTrans->mShareInfo->mTxInfo.mTransRefCnt;

        if ( aSession->isShardLibrarySession() == ID_TRUE )
        {
            /* touch count 1, no global TX */
            /* no global TX */
            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "commitShareableTrans: remove delegate" );

            removeDelegatedSession( aTrans->mShareInfo, aSession );
        }
    }

    MMC_SHARED_TRANS_TRACE( aSession,
                            aTrans,
                            "commitShareableTrans: unlock" );

    sIsLock = ID_FALSE;
    unfixSharedTrans( aTrans, aSession->getSessionID() );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_BROKEN )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_COMMIT_FAILED ) );
    }
    IDE_EXCEPTION( ERR_STATE_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID,
                                  getSharedTransStateString( aTrans ),
                                  "commitShareableTrans" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "commitShareableTrans: exception: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }

    return IDE_FAILURE;
}

/*
 * after transaction commit, process related session
 */
IDE_RC mmcTrans::doAfterCommit( mmcSession *aSession,
                                UInt        aTransReleasePolicy,
                                idBool      aIsSqlPrepare,
                                smSCN      *aCommitSCN,
                                ULong       aNewSMN )
{
    if ( aSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
    {
        aSession->setSessionBegin( ID_FALSE );
    }

    if ( aSession->getTransPrepared() == ID_TRUE )
    {
        aSession->setTransPrepared( NULL );
    }

    if ( aIsSqlPrepare == ID_FALSE )
    {
        (void)aSession->clearLobLocator();

        (void)aSession->flushEnqueue( aCommitSCN );

        // PROJ-1677 DEQUEUE
        if ( aSession->getPartialRollbackFlag() == MMC_DID_PARTIAL_ROLLBACK )
        {
            (void)aSession->flushDequeue( aCommitSCN );
        }
        else
        {
            (void)aSession->clearDequeue();
        }

        // PROJ-1407 Temporary Table
        qci::endTransForSession( aSession->getQciSession(),
                                 ID_TRUE,                   /* Commit */
                                 aCommitSCN,
                                 aNewSMN,
                                 &aSession->mZKPendingFunc);
        if ( aSession->mZKPendingFunc == NULL )
        {
            aSession->clearToBeShardMetaNumber();
        }
        else
        {
            /* no clear, do nothing: use toBeShardMetaNumber after call pending function at distributed transaction complete */
        }

        // BUG-42464 dbms_alert package
        IDE_TEST( aSession->getInfo()->mEvent.commit() != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aTransReleasePolicy == SMI_RELEASE_TRANSACTION )
    {
        aSession->getInfo()->mTransID = 0;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::doAfterRollback( mmcSession  * aSession,
                                  UInt          aTransReleasePolicy,
                                  idBool        aIsSqlPrepare,
                                  const SChar * aSavePoint )
{
    if ( aSavePoint == NULL )
    {
        if ( aSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
        {
            aSession->setSessionBegin( ID_FALSE );
        }

        if ( aSession->getTransPrepared() == ID_TRUE )
        {
            aSession->setTransPrepared( NULL );
        }

        if ( aIsSqlPrepare == ID_FALSE )
        {
            (void)aSession->clearLobLocator();

            (void)aSession->clearEnqueue();

            //PROJ-1677 DEQUEUE 
            (void)aSession->flushDequeue();

            // PROJ-1407 Temporary Table
            qci::endTransForSession( aSession->getQciSession(),
                                     ID_FALSE,                  /* ROLLBACK */
                                     NULL,
                                     SDI_NULL_SMN,
                                     &aSession->mZKPendingFunc);
            aSession->clearToBeShardMetaNumber();
        }
        else
        {
            /* Nothing to do */
        }

        if (aTransReleasePolicy == SMI_RELEASE_TRANSACTION)
        {
            aSession->getInfo()->mTransID = 0;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aIsSqlPrepare == ID_FALSE )
    {
        IDE_TEST( aSession->getInfo()->mEvent.rollback( (SChar *)aSavePoint ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * follow function(removeDelegatedSession) must be located in critical section
 */
void mmcTrans::removeDelegatedSession(mmcTransShareInfo *aShareInfo, mmcSession *aSession)
{
    if ( aShareInfo->mTxInfo.mDelegatedSessions == aSession )
    {
        aShareInfo->mTxInfo.mDelegatedSessions = NULL;
    }
    else
    {
        /* BUG-47093 */
        IDE_DASSERT(aShareInfo->mTxInfo.mDelegatedSessions == NULL);
    }
}

/*
 * follow function(addSession) must be located in critical section
 */
void mmcTrans::addDelegatedSession(mmcTransShareInfo *aShareInfo, mmcSession *aSession)
{
    aShareInfo->mTxInfo.mDelegatedSessions = aSession;
}

IDE_RC mmcTrans::alloc(mmcSession *aSession, mmcTransObj **aTrans)
{
    //fix BUG-18117
    mmcTransObj * sTrans = NULL;

    IDU_FIT_POINT( "mmcTrans::alloc::alloc::Trans" );

    IDE_TEST( mmcTransManager::allocTrans( &sTrans, aSession )
              != IDE_SUCCESS );

    if ( ( aSession != NULL ) &&
         ( isShareableTrans( sTrans ) == ID_TRUE ) )
    {
        /* 공유TX는 smiTrans에 대한 처리를 
           mmcTransManager::allocTrans() -> mmcSharedTrans::allocTrans() 에서 수행함 */

        MMC_SHARED_TRANS_TRACE( aSession,
                                sTrans,
                                "mmcTrans::alloc");
    }
    else
    {
        IDE_ASSERT( sTrans->mSmiTrans.initialize()
                    == IDE_SUCCESS );
    }

    if ( aSession != NULL )
    {
        aSession->mTrans = sTrans;
    }

    *aTrans = sTrans;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aTrans = sTrans;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::free(mmcSession *aSession, mmcTransObj *aTrans)
{
    smiTrans * sSmiTx = NULL;

    if ( ( aSession != NULL ) && ( isShareableTrans(aTrans) == ID_TRUE ) )
    {
        IDE_DASSERT( aSession->mTrans == aTrans );

        /* 공유TX는 smiTrans에 대한 처리를 
           mmcTransManager::freeTrans() -> mmcSharedTrans::freeTrans() 에서 수행함 */

        IDE_TEST( mmcTransManager::freeTrans( &aTrans, aSession )
                  != IDE_SUCCESS );

        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "free");
    }
    else
    {
        IDE_DASSERT(isShareableTrans(aTrans) != ID_TRUE);

        sSmiTx = getSmiTrans( aTrans );
        IDE_TEST(sSmiTx->destroy( NULL ) != IDE_SUCCESS);

        IDE_TEST( mmcTransManager::freeTrans( &aTrans, aSession )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * beginXA: xa 프로토콜을 통해서 들어온 트랜잭션의 sm 모듈에 대한 트랜잭션 begin 처리로 
 * Session의 일반 트랜잭션에 영향을 주지 않음. 
 */
void mmcTrans::beginXA( mmcTransObj *aTrans,
                        idvSQL      *aStatistics,
                        UInt         aFlag )
{
    ULong sDummyEventFlag = 0;
    mmcTrans::beginRaw(aTrans, aStatistics, aFlag, &sDummyEventFlag);
}

/*
 * begin: Session과 연관하여 들어온 일반적인 트랜잭션 begin 상황에서 사용하는 함수
 */
void mmcTrans::begin( mmcTransObj *aTrans,
                      idvSQL      *aStatistics,
                      UInt         aFlag,
                      mmcSession  *aSession,
                      idBool      *aIsDummyBegin )
{
    idBool      sIsDummyBegin = ID_TRUE;

    if ( isShareableTrans(aTrans) == ID_TRUE )
    {
        fixSharedTrans( aTrans, aSession->getSessionID() );
        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "begin: locked");

        IDE_DASSERT( aSession->mTrans == aTrans );

        /* Shared Transaction FSM: Begin */
        switch ( aTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_INIT_DONE :
            case MMC_TRANS_STATE_END :
                mmcTrans::beginRaw(aTrans, aStatistics, aFlag, aSession->getEventFlag());
                sIsDummyBegin = ID_FALSE;
                setTransactionCommitSCN( aTrans, NULL );
                aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_BEGIN;
                break;

            case MMC_TRANS_STATE_PREPARE :
                IDE_DASSERT( aTrans->mShareInfo->mTxInfo.mPrepareSlot != MMC_TRANS_NULL_SLOT_NO );

                if ( isUserConnectedNode( aTrans ) == ID_TRUE )
                {
                    aTrans->mSmiTrans.attach( aTrans->mShareInfo->mTxInfo.mPrepareSlot );
                }
                else
                {
                    aTrans->mSmiTrans.realloc( aSession->getStatSQL(), ID_FALSE );
                    setLocalTransactionBroken( aTrans,
                                               aSession->getSessionID(),
                                               ID_FALSE );
                    mmcTrans::beginRaw(aTrans, aStatistics, aFlag, aSession->getEventFlag());
                    sIsDummyBegin = ID_FALSE;
                    aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_BEGIN;
                }
                break;

            case MMC_TRANS_STATE_BEGIN :
                /* Nothing to do */
                break;

            default :
                IDE_DASSERT( 0 );
                break;
        }

        if ( aSession->getSessionBegin() == ID_FALSE )
        {
            ++aTrans->mShareInfo->mTxInfo.mTransRefCnt;

            if ( aSession->isShardLibrarySession() == ID_TRUE )
            {
                addDelegatedSession(aTrans->mShareInfo,aSession);

                MMC_SHARED_TRANS_TRACE( aSession,
                                        aTrans,
                                        "begin: add delegate");
            }
        }

        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "begin: unlock");

        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }
    else
    {
        mmcTrans::beginRaw(aTrans, aStatistics, aFlag, aSession->getEventFlag());
        sIsDummyBegin = ID_FALSE;
    }

    if ( aSession->getSessionBegin() == ID_FALSE )
    {
        if ( aSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
        {
            aSession->setSessionBegin(ID_TRUE);
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-46913 */
        clearAndSetSessionInfoAfterBegin( aSession,
                                          aTrans );
    }

    *aIsDummyBegin = sIsDummyBegin;
}

idBool mmcTrans::isUsableNEqualXID( ID_XID * aTargetXID, ID_XID * aSourceXID )
{
    idBool sFind = ID_FALSE;

    if ( SDU_SHARD_ENABLE == 0 )
    {
        if ( mmdXid::compFunc( aTargetXID, aSourceXID ) == 0 ) 
        {
            sFind = ID_TRUE;
        }
    }
    else
    {
        if ( dkiIsUsableNEqualXID( aTargetXID, aSourceXID ) == ID_TRUE )
        {
            sFind = ID_TRUE;
        }
    }
    
    return sFind;
}

/**
 * 설명 :
 *   서버사이드 샤딩과 관련된 함수.
 *   사용 가능한 XA 트랜잭션을 찾는다.
 *
 * @param aXID[OUT]    XA Transaction ID
 * @param aFound[OUT]  성공/실패 플래그
 * @param aSlotID[OUT] Slot ID
 * @return 성공하면 IDE_SUCCESS, 아니면 IDE_FAILURE
 **/
IDE_RC mmcTrans::findPreparedTrans( ID_XID *aXID,
                                    idBool *aFound,
                                    iduList *aSlotIDList )
{
    SInt            sSlotID = -1;
    ID_XID          sXid;
    timeval         sTime;
    smiCommitState  sTxState;
    idBool          sFound = ID_FALSE;
    mmcPendingTx  * sPendingTx = NULL;
    iduList         sSlotIDList;
    iduListNode   * sNode  = NULL;
    iduListNode   * sDummy = NULL;

    IDU_LIST_INIT( &sSlotIDList );


    while ( 1 )
    {
        IDE_TEST( smiXaRecover( &sSlotID, &sXid, &sTime, &sTxState )
                  != IDE_SUCCESS );

        if ( sSlotID < 0 )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sTxState == SMX_XA_PREPARED ) &&
             ( isUsableNEqualXID( &sXid, aXID ) == ID_TRUE ) )
        {
            sFound = ID_TRUE;

            if ( aSlotIDList != NULL )
            {
                IDE_TEST( iduMemMgr::malloc( IDU_MEM_MMC,
                                             ID_SIZEOF(mmcPendingTx),
                                             (void**)&sPendingTx,
                                             IDU_MEM_IMMEDIATE )
                          != IDE_SUCCESS );
                sPendingTx->mSlotID = sSlotID;
                dkiCopyXID( &(sPendingTx->mXID), &sXid );

                IDU_LIST_INIT_OBJ( &( sPendingTx->mListNode ), (void*)sPendingTx );
                IDU_LIST_ADD_LAST( &sSlotIDList, &(sPendingTx->mListNode) );
            }
            
            if ( SDU_SHARD_ENABLE == 0 )
            {
                break;
            }
        }
        sPendingTx = NULL;
    }

    if ( aSlotIDList != NULL )
    {
        IDU_LIST_JOIN_LIST( aSlotIDList, &sSlotIDList );
    }

    *aFound = sFound;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( IDU_LIST_IS_EMPTY( &sSlotIDList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &sSlotIDList, sNode, sDummy )
        {
            sPendingTx = (mmcPendingTx*)sNode->mObj;
            IDU_LIST_REMOVE( &(sPendingTx->mListNode) );
            iduMemMgr::free( sPendingTx );
            sPendingTx = NULL;
        }
    }

    return IDE_FAILURE;
}

/**
 * 설명 :
 *   서버사이드 샤딩과 관련된 함수.
 *   트랜잭이 sIsReadOnly이 아닌 경우에, 사용할 수 있는 2PC용 XA 트랜잭션을
 *   찾아서 XID를 Transaction에 설정하고, 반환한다.
 *
 * @param aTrans[IN]     smiTrans
 * @param aSession[IN]   mmcSession
 * @param aXID[OUT]      XA Transaction ID
 * @param aReadOnly[OUT] 트랜잭션의 ReadOnly 속성
 * @return 성공하면 IDE_SUCCESS, 아니면 IDE_FAILURE
 **/
IDE_RC mmcTrans::prepareForShard( mmcTransObj * aTrans,
                                  mmcSession  * aSession,
                                  ID_XID      * aXID,
                                  idBool      * aReadOnly,
                                  smSCN       * aPrepareSCN )
{
    smiTrans   * sSmiTrans           = getSmiTrans(aTrans);
    idBool       sIsReadOnly         = ID_FALSE;
    idBool       sIsDelegateReadOnly = ID_TRUE;
    idBool       sIsLock             = ID_FALSE;
    idBool       sFound              = ID_FALSE;
    idBool       sPrepareWithLog     = ID_TRUE;
    smSCN        sParticipantPrepareSCN;

    *aReadOnly = ID_FALSE;

    SMI_INIT_SCN( aPrepareSCN );
    SMI_INIT_SCN( &sParticipantPrepareSCN );

    IDE_DASSERT( isShareableTrans(aTrans) == ID_TRUE );

    IDE_TEST( dkiCommitPrepare( aSession->getDatabaseLinkSession(), aXID )
              != IDE_SUCCESS );

    fixSharedTrans( aTrans, aSession->getSessionID() );
    sIsLock = ID_TRUE;

    MMC_SHARED_PREPARE_TRANS_TRACE( aSession,
                                    aTrans,
                                    "prepareForShard: locked",
                                    aXID );

    IDE_TEST_RAISE( getLocalTransactionBroken( aTrans ) == ID_TRUE,
                    ERR_TRANS_BROKEN );

    IDE_TEST_RAISE( aTrans->mShareInfo->mTxInfo.mState == MMC_TRANS_STATE_INIT_DONE,
                    ERR_PREPARE_NOT_BEGIN_TX );

    switch ( aTrans->mShareInfo->mTxInfo.mState )
    {
        case MMC_TRANS_STATE_BEGIN :
            IDE_TEST( aTrans->mSmiTrans.isReadOnly( &sIsReadOnly ) != IDE_SUCCESS );
            if ( sIsReadOnly == ID_TRUE )
            {
                /* dblink나 shard로 인해 commit이 필요할 수 있다. */
                sIsReadOnly = dkiIsReadOnly( aSession->getDatabaseLinkSession() );
            }
            break;
        default :
            sIsReadOnly = ID_FALSE;
            break;
    }

    if ( sIsReadOnly == ID_FALSE )
    {
        if ( isUserConnectedNode( aTrans ) == ID_TRUE )
        {
            sPrepareWithLog = ID_FALSE;
        }

        IDU_FIT_POINT_RAISE( "mmcTrans::prepareForShard::stateUnexpected",
                             ERR_STATE_UNEXPECTED );

        /* Shared Transaction FSM: 2PC-Commit-Prepare */
        switch ( aTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_BEGIN :
                IDU_FIT_POINT_RAISE( "mmcTrans::prepareForShard::findPreparedTrans",
                                     ERR_XID_INUSE );
                IDE_TEST( findPreparedTrans( aXID, &sFound ) != IDE_SUCCESS );
                IDE_TEST_RAISE( sFound == ID_TRUE, ERR_XID_INUSE );

                MMC_SHARED_PREPARE_TRANS_TRACE( aSession,
                                                aTrans,
                                                "prepareForShard: prepare TX and dettach",
                                                aXID );

                sSmiTrans->setStatistics( aSession->getStatSQL() );

                IDE_TEST( sSmiTrans->prepare( aXID,
                                              aPrepareSCN,
                                              sPrepareWithLog )
                          != IDE_SUCCESS );

                /* dktGlobalCoordinator::executeTwoPhaseCommitPrepareForShard 함수에서
                 * 2PC Prepare 전파 후 Preapre SCN 을 얻어온것이 아래 sParticipantPrepareSCN 이다. */
                aSession->getCoordPrepareSCN( aSession->getShardClientInfo(),
                                              &sParticipantPrepareSCN );

                SM_SET_MAX_SCN( aPrepareSCN, &sParticipantPrepareSCN );

                setTransactionPrepareSlot( aTrans, sSmiTrans->getTransID() );

                sSmiTrans->setStatistics( NULL );

                IDE_ASSERT( sSmiTrans->dettach() == IDE_SUCCESS );

                aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_PREPARE;

                IDU_FIT_POINT( "mmcTrans::prepareForShard::afterPrepare" );
                break;

            case MMC_TRANS_STATE_PREPARE :
            case MMC_TRANS_STATE_END :
                /* Nothing to do */
                break;

            case MMC_TRANS_STATE_INIT_DONE :
            default :
                IDE_RAISE( ERR_STATE_UNEXPECTED );
                break;
        }

        if ( aSession->getSessionBegin() == ID_TRUE )
        {
            --aTrans->mShareInfo->mTxInfo.mTransRefCnt;
            if ( aSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
            {
                aSession->setSessionBegin( ID_FALSE );
            }
        }

        aSession->setTransPrepared( aXID );
    }
    else
    {
        /* sIsReadOnly == ID_TRUE */
        /* Nothing to do */
    }

    if ( aSession->isShardLibrarySession() == ID_FALSE )
    {
        IDE_TEST( aSession->prepareForShardDelegateSession( aTrans,
                                                            aXID,
                                                            &sIsDelegateReadOnly,
                                                            &sParticipantPrepareSCN )
                  != IDE_SUCCESS );

        sIsReadOnly = ( ( sIsReadOnly == ID_TRUE ) && ( sIsDelegateReadOnly == ID_TRUE ) )
                      ? ID_TRUE : ID_FALSE;

        SM_SET_MAX_SCN( aPrepareSCN, &sParticipantPrepareSCN );
    }

    MMC_SHARED_TRANS_TRACE( aSession,
                            aTrans,
                            "prepareForShard: unlock" );

    sIsLock = ID_FALSE;
    unfixSharedTrans( aTrans, aSession->getSessionID() );

    *aReadOnly = sIsReadOnly;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_BROKEN )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_COMMIT_FAILED ) );
    }
    IDE_EXCEPTION( ERR_XID_INUSE )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_XID_INUSE ) );
    }
    IDE_EXCEPTION( ERR_PREPARE_NOT_BEGIN_TX )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_PREPARE_DID_NOT_BEGIN_TX ) );
    }
    IDE_EXCEPTION( ERR_STATE_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID,
                                  getSharedTransStateString( aTrans ),
                                  "prepareForShard" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "prepareForShard: exception: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }

    return IDE_FAILURE;
}

/*
 * prepareXA: xa 프로토콜을 통해서 들어온 트랜잭션의 sm/dk 모듈에 대한 트랜잭션 prepare 처리로
 * Session이 갖고 있는 트랜잭션에 영향을 주지 않음.
 */
IDE_RC mmcTrans::prepareXA( mmcTransObj *aTrans, 
                            ID_XID      *aXID, 
                            mmcSession  *aSession )
{
    smiTrans *sTrans  = mmcTrans::getSmiTrans(aTrans);

    IDE_DASSERT(aSession != NULL);
    IDE_DASSERT( isShareableTrans(aTrans) == ID_FALSE );

    sTrans->setStatistics(aSession->getStatSQL());

    if ( aSession->getTransPrepared() == ID_FALSE )
    {
        IDE_TEST(dkiCommitPrepare(aSession->getDatabaseLinkSession(), NULL) != IDE_SUCCESS);
    }

    IDE_TEST(mmcTrans::getSmiTrans(aTrans)->prepare(aXID) != IDE_SUCCESS);
    aSession->setTransPrepared(aXID);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mmcTrans::endPending( mmcSession * aSession,
                             ID_XID     * aXID,
                             idBool       aIsCommit,
                             smSCN      * aGlobalCommitSCN )
{
    smiTrans        sTrans;
    SInt            sSlotID;
    idBool          sFound = ID_FALSE;
    mmcPendingTx  * sPendingTx = NULL;
    iduList         sSlotIDList;
    iduListNode   * sNode  = NULL;
    iduListNode   * sDummy = NULL;

    (void)dkiEndPendingPassiveDtxInfo( aXID, aIsCommit );
    (void)dkiEndPendingFailoverDtxInfo( aXID, 
                                        aIsCommit,
                                        aGlobalCommitSCN );

    IDU_LIST_INIT( &sSlotIDList );

    IDE_TEST( findPreparedTrans( aXID, &sFound, &sSlotIDList ) != IDE_SUCCESS );

    if ( sFound == ID_TRUE )
    {
        MMC_END_PENDING_TRANS_TRACE( aSession,
                                     NULL,
                                     aXID,
                                     aIsCommit,
                                     "endPending: Find prepare transaction success" );

        IDU_LIST_ITERATE_SAFE( &(sSlotIDList), sNode, sDummy )
        {
            sPendingTx = (mmcPendingTx*)sNode->mObj;
            sSlotID = sPendingTx->mSlotID;

            IDE_ASSERT( sTrans.initialize() == IDE_SUCCESS );

            // R2HA BUG-48227 추가 처리 예정
            IDE_ASSERT( sTrans.initializeInternal() == IDE_SUCCESS );

            IDE_ASSERT( sTrans.attach(sSlotID) == IDE_SUCCESS );

            MMC_END_PENDING_TRANS_TRACE( aSession,
                                         &sTrans,
                                         aXID,
                                         aIsCommit,
                                         "endPending: Attach transaction" );

            IDE_TEST_RAISE( sTrans.getStatistics() != NULL,
                            ERR_XID_INUSE );

            if ( aIsCommit == ID_TRUE )
            {
                #if defined(DEBUG)
                ideLog::log( IDE_SD_18, "= [%s] endPending, GlobalCommitSCN : %"ID_UINT64_FMT,
                             aSession->getSessionTypeString(),
                             *aGlobalCommitSCN );
                #endif
                aSession->setCallbackForReloadNewIncreasedDataSMN(&sTrans);

                if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_INTERNAL_TABLE_SWAP_MASK ) ==
                     QC_SESSION_INTERNAL_TABLE_SWAP_TRUE )
                {
                    aSession->setInternalTableSwap( &sTrans );
                }

                if ( sTrans.commit( aGlobalCommitSCN ) != IDE_SUCCESS )
                {
                    (void)idaXaConvertXIDToString(NULL, aXID, (UChar*)&(sPendingTx->mXID), XID_DATA_MAX_LEN);

                    (void)ideLog::logLine( IDE_SERVER_0,
                                           "#### mmcTrans::commitPending (XID:%s) commit failed",
                                           &(sPendingTx->mXID) );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                if ( sTrans.rollback( NULL ) != IDE_SUCCESS )
                {
                    (void)idaXaConvertXIDToString(NULL, aXID, (UChar*)&(sPendingTx->mXID), XID_DATA_MAX_LEN);

                    (void)ideLog::logLine( IDE_SERVER_0,
                                           "#### mmcTrans::commitPending (XID:%s) rollback failed",
                                           &(sPendingTx->mXID) );
                }
                else
                {
                    /* Nothing to do */
                }

            }
            
            IDE_ASSERT( sTrans.destroy( aSession->getStatSQL() ) == IDE_SUCCESS );

            IDU_LIST_REMOVE( &(sPendingTx->mListNode) );
            iduMemMgr::free( sPendingTx );
            sPendingTx = NULL;
        }
    }
    else
    {
        /* Nothing to do */
        MMC_END_PENDING_TRANS_TRACE( aSession,
                                     NULL,
                                     aXID,
                                     aIsCommit,
                                     "endPending: Find prepare transaction fail" );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_XID_INUSE)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_XID_INUSE));
    }
    IDE_EXCEPTION_END;

    if ( IDU_LIST_IS_EMPTY( &sSlotIDList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &sSlotIDList, sNode, sDummy )
        {
            sPendingTx = (mmcPendingTx*)sNode->mObj;
            IDU_LIST_REMOVE( &(sPendingTx->mListNode) );
            iduMemMgr::free( sPendingTx );
            sPendingTx = NULL;
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmcTrans::endPendingBySlotN( mmcTransObj * aTrans,
                                    mmcSession  * aSession,
                                    ID_XID      * aXID,
                                    idBool        aIsCommit,
                                    idBool        aMySelf,
                                    smSCN       * aCommitSCN )
{
    smiTrans      * sTransPtr           = NULL;
    smiTrans        sSmiTrans;
    smxTrans      * sSmxTrans           = NULL;
    UInt            sTransReleasePolicy = SMI_DO_NOT_RELEASE_TRANSACTION;
    SInt            sSlotN              = MMC_TRANS_NULL_SLOT_NO;
    ID_XID          sSmxXID;
    UChar           sXidString[XID_DATA_MAX_LEN];
    idBool          sAttached           = ID_FALSE;

    sSlotN = aSession->mTrans->mShareInfo->mTxInfo.mPrepareSlot;

    if ( aMySelf == ID_TRUE )
    {
        sTransPtr = &aTrans->mSmiTrans;
    }
    else
    {
        IDE_ASSERT( sSmiTrans.initialize() == IDE_SUCCESS );

        sTransReleasePolicy = SMI_RELEASE_TRANSACTION;

        sTransPtr = &sSmiTrans;
    }

    IDE_ASSERT( sTransPtr->attach( sSlotN ) == IDE_SUCCESS );
    sAttached = ID_TRUE;

    sTransPtr->setStatistics( aSession->getStatSQL() );

    sSmxTrans = (smxTrans *)sTransPtr->getTrans();

    IDE_TEST_RAISE( sSmxTrans->isPrepared() == ID_FALSE,
                    ERR_NOT_PREPARED );

    if ( aXID != NULL )
    {
        IDE_TEST_RAISE( sSmxTrans->getXID( &sSmxXID ) != IDE_SUCCESS,
                        ERR_GET_XID );

        IDE_TEST_RAISE( mmdXid::compFunc( aXID, &sSmxXID ) != 0,
                        ERR_XID_IS_INVALID );
    }

    MMC_END_PENDING_TRANS_TRACE( aSession,
                                 sTransPtr,
                                 aXID,
                                 aIsCommit,
                                 "endPendingBySlotN: Find prepare transaction success" );

    if ( aIsCommit == ID_TRUE )
    {
        if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
             QC_SESSION_SHARD_META_TOUCH_TRUE )
        {
            aSession->setCallbackForReloadNewIncreasedDataSMN(sTransPtr);
        }

        if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_INTERNAL_TABLE_SWAP_MASK ) ==
             QC_SESSION_INTERNAL_TABLE_SWAP_TRUE )
        {
            aSession->setInternalTableSwap( sTransPtr );
        }

        setGlobalTxID4Trans( NULL, aSession ); /* BUG-48703 */
        IDE_TEST_RAISE( sTransPtr->commit( aCommitSCN, sTransReleasePolicy ) != IDE_SUCCESS,
                        ERR_COMMIT_FAIL );
    }
    else
    {
        setGlobalTxID4Trans( NULL, aSession ); /* BUG-48703 */
        IDE_TEST_RAISE( sTransPtr->rollback( NULL, sTransReleasePolicy ) != IDE_SUCCESS,
                        ERR_ROLLBACK_FAIL );
    }

    if ( aMySelf == ID_TRUE )
    {
        /* Nothing to do */
    }
    else
    {
        IDE_ASSERT( sTransPtr->destroy( NULL ) == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_PREPARED )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "Transaction to commit is not prepared." )  );
    }
    IDE_EXCEPTION( ERR_GET_XID )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "Getting transaction XID is failed." )  );
    }
    IDE_EXCEPTION( ERR_XID_IS_INVALID )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "Transaction XID is invalid." )  );

        if ( aXID != NULL )
        {
            (void)idaXaConvertXIDToString( NULL, aXID, sXidString, XID_DATA_MAX_LEN );
        }
        else
        {
            idlOS::snprintf( (SChar*)sXidString, ID_SIZEOF( sXidString ), "(NULL)" );
        }

        ideLog::log( IDE_SD_0, "[END PENDING N : MISS MATCH XIDs] (XID:%s)\n",
                               sXidString );
    }
    IDE_EXCEPTION( ERR_COMMIT_FAIL )
    {
        if ( aXID != NULL )
        {
            (void)idaXaConvertXIDToString( NULL, aXID, sXidString, XID_DATA_MAX_LEN );
        }
        else
        {
            idlOS::snprintf( (SChar*)sXidString, ID_SIZEOF( sXidString ), "(NULL)" );
        }


        (void)ideLog::logLine( IDE_SERVER_0,
                               "#### mmcTrans::commitPendingN (XID:%s) commit failed",
                               sXidString );
    }
    IDE_EXCEPTION( ERR_ROLLBACK_FAIL )
    {
        if ( aXID != NULL )
        {
            (void)idaXaConvertXIDToString( NULL, aXID, sXidString, XID_DATA_MAX_LEN );
        }
        else
        {
            idlOS::snprintf( (SChar*)sXidString, ID_SIZEOF( sXidString ), "(NULL)" );
        }


        (void)ideLog::logLine( IDE_SERVER_0,
                               "#### mmcTrans::commitPendingN (XID:%s) rollback failed",
                               sXidString );
    }
    IDE_EXCEPTION_END;

    if ( sAttached == ID_TRUE )
    {
        sTransPtr->setStatistics( NULL );
        IDE_ASSERT( sTransPtr->dettach() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC mmcTrans::endPendingSharedTx( mmcSession * aSession,
                                     ID_XID     * aXID,
                                     idBool       aIsCommit,
                                     smSCN      * aGlobalCommitSCN )
{
    mmcTransObj * sTrans    = aSession->mTrans;
    idBool        sIsLock   = ID_FALSE;
    idBool        sSetBlock = ID_FALSE;
    smSCN         sCommitSCN;

    IDU_FIT_POINT( "mmcTrans::endPendingSharedTx::beforeEndPending" );

    SMI_INIT_SCN( &sCommitSCN );

    if ( ( aGlobalCommitSCN != NULL ) &&
         ( SM_SCN_IS_NOT_INIT( *aGlobalCommitSCN ) ) )
    {
        SM_SET_SCN( &sCommitSCN, aGlobalCommitSCN );

        IDE_TEST( sdi::syncSystemSCN4GCTx( &sCommitSCN, NULL ) != IDE_SUCCESS );
    }

    if ( isShareableTrans( sTrans ) == ID_TRUE )
    {
        /* coordinator session */
        fixSharedTrans( sTrans, aSession->getSessionID() );
        sIsLock = ID_TRUE;

        MMC_SHARED_TRANS_TRACE( aSession,
                                sTrans,
                                "endPendingSharedTx: lock" );

        if ( aSession->isShardLibrarySession() == ID_FALSE )
        {
            (void)aSession->endPendingSharedTxDelegateSession( sTrans,
                                                               aXID,
                                                               aIsCommit,
                                                               &sCommitSCN );
        }

        if ( getLocalTransactionBroken( aSession->mTrans ) == ID_TRUE )
        {
            (void)dkiEndPendingPassiveDtxInfo( aXID, aIsCommit );
        }

        IDU_FIT_POINT_RAISE( "mmcTrans::endPendingSharedTx::stateUnexpected",
                             ERR_STATE_UNEXPECTED );

        IDU_SESSION_SET_BLOCK(*aSession->getEventFlag());
        sSetBlock = ID_TRUE;

        /* Shared Transaction FSM: 2PC-Commit-EndPending */
        switch ( sTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_PREPARE :
                IDE_DASSERT( SM_SCN_IS_INIT( sTrans->mShareInfo->mTxInfo.mCommitSCN ) );

                MMC_SHARED_TRANS_TRACE( aSession,
                                        sTrans,
                                        "endPendingSharedTx: endPendingBySlotN" );

                IDE_DASSERT( sTrans->mShareInfo->mTxInfo.mPrepareSlot != MMC_TRANS_NULL_SLOT_NO );

                #if defined(DEBUG)
                ideLog::log( IDE_SD_18, "= [%s] endPendingSharedTx, STATE_PREPARE CommitSCN : %"ID_UINT64_FMT,
                             aSession->getSessionTypeString(),
                             sCommitSCN );
                #endif

                IDE_TEST( endPendingBySlotN( sTrans,
                                             aSession,
                                             aXID,
                                             aIsCommit,
                                             ID_TRUE,   /* Self */
                                             &sCommitSCN )
                          != IDE_SUCCESS );

                setTransactionPrepareSlot( sTrans, MMC_TRANS_NULL_SLOT_NO );
                setLocalTransactionBroken( sTrans,
                                           aSession->getSessionID(),
                                           ID_FALSE );
                setTransactionCommitSCN( sTrans, &sCommitSCN );
                sTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_END;
                break;

            case MMC_TRANS_STATE_BEGIN :
                IDE_DASSERT( SM_SCN_IS_INIT( sTrans->mShareInfo->mTxInfo.mCommitSCN ) );

                getSmiTrans( sTrans )->setStatistics( aSession->getStatSQL() );

                MMC_SHARED_TRANS_TRACE( aSession,
                                        sTrans,
                                        "endPendingSharedTx: rollbackRaw");

                if ( aIsCommit == ID_TRUE )
                {
                    IDE_DASSERT ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) !=
                                  QC_SESSION_SHARD_META_TOUCH_TRUE );

                    if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
                         QC_SESSION_SHARD_META_TOUCH_TRUE )
                    {
                        aSession->setCallbackForReloadNewIncreasedDataSMN(mmcTrans::getSmiTrans(sTrans));
                    }

                    if ( ( aSession->mQciSession.mQPSpecific.mFlag & QC_SESSION_INTERNAL_TABLE_SWAP_MASK ) ==
                         QC_SESSION_INTERNAL_TABLE_SWAP_TRUE )
                    {
                        aSession->setInternalTableSwap( mmcTrans::getSmiTrans(sTrans) );
                    }

                    IDE_TEST( mmcTrans::commitRaw( sTrans,
                                                   aSession,
                                                   aSession->getEventFlag(),
                                                   SMI_DO_NOT_RELEASE_TRANSACTION,
                                                   &sCommitSCN )
                              != IDE_SUCCESS );

                    setTransactionCommitSCN( sTrans, &sCommitSCN );
                }
                else
                {
                    IDE_DASSERT( SM_SCN_IS_INIT( sCommitSCN ) );

                    IDE_TEST( mmcTrans::rollbackRaw( sTrans,
                                                     aSession,
                                                     aSession->getEventFlag(),
                                                     SMI_DO_NOT_RELEASE_TRANSACTION )
                              != IDE_SUCCESS );
                }

                IDE_DASSERT( sTrans->mShareInfo->mTxInfo.mPrepareSlot == MMC_TRANS_NULL_SLOT_NO );

                setTransactionPrepareSlot( sTrans, MMC_TRANS_NULL_SLOT_NO );
                setLocalTransactionBroken( sTrans,
                                           aSession->getSessionID(),
                                           ID_FALSE );
                sTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_END;
                break;

            case MMC_TRANS_STATE_END :
                /* User 노드와 Coordinating 노드가 동일한 경우
                 * User session 에 의해서 이미 Commit/Rollback 된 후
                 * Coordinator session 에서 Commit/Rollback 하려는 상황 */
                break;

            case MMC_TRANS_STATE_INIT_DONE :
                /* SHARD_ENABLE = 1; Notify session */
                IDE_TEST( endPending( aSession,
                                      aXID,
                                      aIsCommit,
                                      &sCommitSCN ) != IDE_SUCCESS );
                break;

            default :
                IDE_RAISE( ERR_STATE_UNEXPECTED );
                break;
        }

        sSetBlock = ID_FALSE;
        IDU_SESSION_CLR_BLOCK(*aSession->getEventFlag());

        if ( aSession->isShardLibrarySession() == ID_TRUE )
        {
            MMC_SHARED_TRANS_TRACE( aSession,
                                    sTrans,
                                    "endPendingSharedTx: remove delegate");

            removeDelegatedSession( sTrans->mShareInfo, aSession );
        }

        switch ( sTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_END :
                if ( aIsCommit == ID_TRUE )
                {
                    IDE_TEST( doAfterCommit( aSession,
                                             SMI_DO_NOT_RELEASE_TRANSACTION,
                                             ID_FALSE,
                                             getTransactionCommitSCNPtr( sTrans ),
                                             aSession->mInfo.mToBeShardMetaNumber )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( doAfterRollback( aSession,
                                               SMI_DO_NOT_RELEASE_TRANSACTION,
                                               ID_FALSE,
                                               NULL )
                              != IDE_SUCCESS );
                }
                break;

            default :
                /* Nothing to do */
                break;
        }

        MMC_SHARED_TRANS_TRACE( aSession,
                                sTrans,
                                "endPendingSharedTx: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( sTrans, aSession->getSessionID() );
    }
    else
    {
        /* SHARD_ENABLE = 0; Notify session */
        IDE_TEST( endPending( aSession,
                              aXID,
                              aIsCommit,
                              &sCommitSCN ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATE_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID,
                                  getSharedTransStateString( sTrans ),
                                  "endPendingSharedTx" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        MMC_SHARED_TRANS_TRACE( aSession,
                                sTrans,
                                "endPendingSharedTx: exception: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( sTrans, aSession->getSessionID() );
    }

    if ( sSetBlock == ID_TRUE )
    {
        IDU_SESSION_CLR_BLOCK( *aSession->getEventFlag() );
    }
    else
    {
        /* Nothing to do */
    }

    ideLog::log( IDE_SD_2,
                 "[endPendingSharedTx : FAILURE] ERR-<%"ID_xINT32_FMT"> : <%s>",
                 E_ERROR_CODE( ideGetErrorCode() ),
                 ideGetErrorMsg( ideGetErrorCode() ) );

    return IDE_FAILURE;
}

/*
 * commit: Session과 연관하여 들어온 일반적인 트랜잭션 commit 상황에서 사용하는 함수
 */
IDE_RC mmcTrans::commit( mmcTransObj *aTrans,
                         mmcSession  *aSession,
                         UInt         aTransReleasePolicy )
{
    idBool sIsBlocked = ID_FALSE;

    IDE_TEST( aSession->rebuildShardSessionBeforeEndTran( aTrans )
              != IDE_SUCCESS );

    IDE_TEST( aSession->blockForLibrarySession( aTrans,
                                                &sIsBlocked )
              != IDE_SUCCESS );

    if ( aSession->getTransPrepared() == ID_FALSE )
    {
        IDE_TEST( dkiCommitPrepare( aSession->getDatabaseLinkSession(), NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        // BUG-45411, BUG-45826
        IDU_FIT_POINT( "mmcTrans::commit::AlreadyPrepared" );
    }

    IDU_FIT_POINT( "mmcTrans::commit::BeforeCommitLocal" );

    // BUG-48697
    IDU_FIT_POINT_RAISE( "mmcTrans::commit::CommitError", ERR_COMMTI_FAIL );

    IDE_TEST( commitLocal( aTrans, aSession, aTransReleasePolicy, ID_FALSE )
              != IDE_SUCCESS );

    /* BUG-46092 */
    IDU_FIT_POINT( "mmcTrans::commit::AfterCommitLocal" );

    dkiCommit( aSession->getDatabaseLinkSession() );

    if ( sIsBlocked == ID_TRUE )
    {
        aSession->unblockForLibrarySession( aTrans );
    }

    aSession->executeZookeeperPendingJob();

    aSession->rebuildShardSessionAfterEndTran();

    /* apply after zookeeper connect function created
    if ( aSession->globalDDLUserSession() == ID_TRUE )
    {
       sdiZookeeper::releaseZookeeperMetaLock();
    }
    */

    /* PROJ-2733-DistTxInfo 분산정보 정리 */
    if ( aSession->getShardClientInfo() != NULL )  /* BUG-48109 */
    {
        sdi::endTranDistTx( aSession->getShardClientInfo(), aSession->isGCTx() );
    }

    /* TASK-7219 Non-shard DML execution sequence의 초기화 */
    aSession->initStmtExecSeqForShardTx();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COMMTI_FAIL )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_COMMIT_ERROR, "commitError" ) );
    }

    IDE_EXCEPTION_END;

    if ( sIsBlocked == ID_TRUE )
    {
        aSession->unblockForLibrarySession( aTrans );
    }

    return IDE_FAILURE;
}

/*
 * commitXA: xa 프로토콜을 통해서 들어온 트랜잭션의 sm/dk 모듈에 대한 트랜잭션 commit 처리로 
 * Session이 갖고 있는 트랜잭션에 영향을 주지 않음.
 */
IDE_RC mmcTrans::commitXA( mmcTransObj *aTrans,
                           mmcSession  *aSession,
                           UInt         aTransReleasePolicy )
{
    ULong sDummyEventFlag = 0;
    smSCN          sDummySCN = SM_SCN_INIT;

    if ( aSession != NULL )
    {
        if ( aSession->getTransPrepared() == ID_FALSE )
        {
            IDE_TEST( dkiCommitPrepare( aSession->getDatabaseLinkSession(), NULL )
                    != IDE_SUCCESS );
        }
        else
        {
            /*do nothing*/
        }

        IDE_TEST( mmcTrans::commitRaw( aTrans,
                                       aSession, 
                                       aSession->getEventFlag(), 
                                       aTransReleasePolicy, 
                                       &sDummySCN )
                != IDE_SUCCESS );

        dkiCommit( aSession->getDatabaseLinkSession() );
    }
    else
    {
        IDE_TEST( mmcTrans::commitRaw( aTrans, 
                                       aSession, 
                                       &sDummyEventFlag, 
                                       aTransReleasePolicy, 
                                       &sDummySCN ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::commit4Prepare( mmcTransObj                       * aTrans,
                                 mmcSession                        * aSession,
                                 mmcTransEndAction                   aTransEndAction )
{
    switch( aTransEndAction )
    {
        case MMC_TRANS_DO_NOTHING:
            break;

        case MMC_TRANS_SESSION_ONLY_END:
            IDE_TEST( commitLocal( aTrans, 
                                   aSession, 
                                   SMI_RELEASE_TRANSACTION,
                                   ID_TRUE,
                                   aTransEndAction )
                      != IDE_SUCCESS );
            break;

        case MMC_TRANS_END:
            IDE_TEST( commitLocal( aTrans, 
                                   aSession, 
                                   SMI_RELEASE_TRANSACTION,
                                   ID_TRUE,
                                   aTransEndAction )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::rollback4Prepare( mmcTransObj                 * aTrans,
                                   mmcSession                  * aSession,
                                   mmcTransEndAction             aTransEndAction )
{
    switch( aTransEndAction )
    {
        case MMC_TRANS_DO_NOTHING:
            break;

        case MMC_TRANS_SESSION_ONLY_END:
            IDE_TEST( rollbackLocal( aTrans,
                                     aSession,
                                     NULL,
                                     SMI_RELEASE_TRANSACTION,
                                     ID_TRUE,
                                     aTransEndAction )
                      != IDE_SUCCESS );
            break;

        case MMC_TRANS_END:
            IDE_TEST( rollbackLocal( aTrans,
                                     aSession,
                                     NULL,
                                     SMI_RELEASE_TRANSACTION,
                                     ID_TRUE,
                                     aTransEndAction )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_DASSERT( 0 );
            break;

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::commitForceDatabaseLink( mmcTransObj *aTrans,
                                          mmcSession  *aSession,
                                          UInt         aTransReleasePolicy )
{
    IDE_TEST( aSession->rebuildShardSessionBeforeEndTran( aTrans )
              != IDE_SUCCESS );

    IDE_TEST( dkiCommitPrepareForce( aSession->getDatabaseLinkSession() ) != IDE_SUCCESS );

    IDE_TEST( commitLocal( aTrans, aSession, aTransReleasePolicy, ID_FALSE )
              != IDE_SUCCESS );

    dkiCommit( aSession->getDatabaseLinkSession() );

    aSession->executeZookeeperPendingJob();

    aSession->rebuildShardSessionAfterEndTran();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * rollback; Session과 연관하여 들어온 일반적인 트랜잭션 rollback 상황에서 사용하는 함수
 */
IDE_RC mmcTrans::rollback( mmcTransObj *aTrans,
                           mmcSession  *aSession,
                           const SChar *aSavePoint,
                           UInt         aTransReleasePolicy )
{
    IDE_RC sRC = IDE_FAILURE;

    if ( aSession->rebuildShardSessionBeforeEndTran( aTrans ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( SDI_REUILD_ERROR_LOG_LEVEL );
    }

    IDU_FIT_POINT( "mmcTrans::rollback::beforeDkiRollbackPrepare" );

    IDE_TEST( dkiRollbackPrepare( aSession->getDatabaseLinkSession(), aSavePoint ) != IDE_SUCCESS );

    // BUG-45411, BUG-45826
    IDU_FIT_POINT( "mmcTrans::rollback::afterDkiRollbackPrepare" );

    // BUG-48697
    IDU_FIT_POINT_RAISE( "mmcTrans::rollback::RollbackError", ERR_ROLLBACK_FAIL );

    IDE_TEST( rollbackLocal( aTrans,
                             aSession,
                             aSavePoint,
                             aTransReleasePolicy,
                             ID_FALSE )
              != IDE_SUCCESS );

    /* BUG-48489 Check sRC while doing partial rollback */
    sRC = dkiRollback( aSession->getDatabaseLinkSession(), aSavePoint );
    IDE_TEST_RAISE( ( sRC != IDE_SUCCESS ) && ( aSavePoint != NULL ), ERR_PARTIAL_ROLLBACK );

    aSession->executeZookeeperPendingJob();

    if ( aSavePoint == NULL )
    {
        aSession->rebuildShardSessionAfterEndTran();
    }

    /* PROJ-2733-DistTxInfo 분산정보 정리 */
    if ( ( aSavePoint == NULL ) &&
         ( aSession->getShardClientInfo() != NULL ) )  /* BUG-48109 */
    {
        sdi::endTranDistTx( aSession->getShardClientInfo(), aSession->isGCTx() );
    }

    /* TASK-7219 Non-shard DML execution sequence의 초기화 */
    if ( aSavePoint == NULL )
    {
        aSession->initStmtExecSeqForShardTx();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ROLLBACK_FAIL )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_ROLLBACK_ERROR, "rollbackError" ) );
    }

    IDE_EXCEPTION( ERR_PARTIAL_ROLLBACK )
    {
        aSession->executeZookeeperPendingJob();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * rollbackXA: xa 프로토콜을 통해서 들어온 트랜잭션의 sm/dk 모듈에 대한 트랜잭션 rollback 처리로 
 * Session이 갖고 있는 트랜잭션에 영향을 주지 않음.
 */
IDE_RC mmcTrans::rollbackXA( mmcTransObj *aTrans,
                             mmcSession  *aSession,
                             UInt         aTransReleasePolicy )
{
    ULong sDummyEventFlag = 0;

    if ( aSession != NULL )
    {
        dkiRollbackPrepareForce( aSession->getDatabaseLinkSession() );

        IDE_TEST( rollbackRaw( aTrans, aSession, &sDummyEventFlag, aTransReleasePolicy) != IDE_SUCCESS );

        (void)dkiRollback( aSession->getDatabaseLinkSession(), NULL );  /* BUG-48489 */
    }
    else
    {
        IDE_TEST( rollbackRaw( aTrans, aSession, &sDummyEventFlag, aTransReleasePolicy) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::rollbackForceDatabaseLink( mmcTransObj *aTrans,
                                            mmcSession  *aSession,
                                            UInt         aTransReleasePolicy )
{
    if ( aSession->rebuildShardSessionBeforeEndTran( aTrans ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( SDI_REUILD_ERROR_LOG_LEVEL );
    }

    dkiRollbackPrepareForce( aSession->getDatabaseLinkSession() );

    IDE_TEST( rollbackLocal( aTrans,
                             aSession,
                             NULL,
                             aTransReleasePolicy )
              != IDE_SUCCESS );

    (void)dkiRollback( aSession->getDatabaseLinkSession(), NULL );  /* BUG-48489 */

    aSession->executeZookeeperPendingJob();

    aSession->rebuildShardSessionAfterEndTran();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::savepoint( mmcTransObj *aTrans,
                            mmcSession  *aSession,
                            const SChar *aSavePoint )
{
    idBool sIsLock = ID_FALSE;

    IDE_TEST( dkiSavepoint( aSession->getDatabaseLinkSession(), aSavePoint ) != IDE_SUCCESS );

    if ( isShareableTrans(aTrans) == ID_TRUE )
    {
        fixSharedTrans( aTrans, aSession->getSessionID() );
        sIsLock = ID_TRUE;

        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "mmcTrans::savepoint: locked" );

        switch ( aTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_BEGIN :
                MMC_SHARED_TRANS_TRACE( aSession,
                                        aTrans,
                                        "mmcTrans::savepoint: savepoint" );

                IDE_TEST( aTrans->mSmiTrans.savepoint(aSavePoint) != IDE_SUCCESS );
                break;

            default :
                IDE_DASSERT( 0 );
                break;
        }

        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "mmcTrans::savepoint: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }
    else
    {
        IDE_TEST( aTrans->mSmiTrans.savepoint(aSavePoint) != IDE_SUCCESS );
    }

    // BUG-42464 dbms_alert package
    IDE_TEST( aSession->getInfo()->mEvent.savepoint( (SChar *)aSavePoint ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "mmcTrans::savepoint: exception: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }

    return IDE_FAILURE;
}

/* BUG-46785 Shard Stmt Partial rollback */
IDE_RC mmcTrans::shardStmtPartialRollback( mmcTransObj * aTrans, mmcSession * aSession )
{
    idBool    sIsLock = ID_FALSE;

    if ( isShareableTrans(aTrans) == ID_TRUE )
    {
        fixSharedTrans( aTrans, aSession->getSessionID() );
        sIsLock = ID_TRUE;

        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "shardStmtPartialRollback: locked" );

        switch ( aTrans->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_BEGIN :
                IDU_FIT_POINT_RAISE( "mmcTrans::shardStmtPartialRollback::checkImpSVP4Shard", 
                                     ERR_NO_IMPSVP_SHARD );
                IDE_TEST_RAISE( smiTrans::checkImpSVP4Shard( &(aTrans->mSmiTrans) ) != ID_TRUE,
                                ERR_NO_IMPSVP_SHARD );

                IDE_TEST( smiTrans::abortToImpSVP4Shard( &(aTrans->mSmiTrans) ) != IDE_SUCCESS );
                break;

            default :
                IDE_DASSERT( 0 );
                break;
        }

        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "shardStmtPartialRollback: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }
    else
    {
        /* Nothing to do */
        IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_IMPSVP_SHARD )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "there is no implicit savepoint for shard" )  );
    }
    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "shardStmtPartialRollback: exception: unlock" );

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC mmcTrans::commitLocal( mmcTransObj *aTrans,
                              mmcSession  *aSession,
                              UInt         aTransReleasePolicy,
                              idBool       aIsSqlPrepare,
                              mmcTransEndAction aTransEndAction )
{
    smSCN  sCommitSCN;

    SM_INIT_SCN( &sCommitSCN );

    /* non-autocommit 모드의 begin된 tx이거나, autocommit 모드의 tx이거나,
       xa의 tx이거나, prepare tx인 경우 */
    if ( ( aSession->getSessionBegin() == ID_TRUE ) ||
         ( aSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) ||
         ( aIsSqlPrepare == ID_TRUE ) )
    {
        IDE_TEST( collectPrepareSCN( aSession, &sCommitSCN ) != IDE_SUCCESS );

        if ( isShareableTrans( aTrans ) == ID_TRUE )
        {
            IDE_TEST( mmcTrans::commitShareableTrans( aTrans,
                                                      aSession,
                                                      aTransReleasePolicy,
                                                      & sCommitSCN,
                                                      aTransEndAction )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( mmcTrans::commitRaw( aTrans,
                                           aSession, 
                                           aSession->getEventFlag(), 
                                           aTransReleasePolicy,
                                           &sCommitSCN  )
                        != IDE_SUCCESS );
        }

        IDV_SESS_ADD_DIRECT(aSession->getStatistics(), IDV_STAT_INDEX_COMMIT_COUNT, 1);

        deployGlobalCommitSCN( aSession, &sCommitSCN );

    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( mmcTrans::doAfterCommit( aSession,
                                       aTransReleasePolicy,
                                       aIsSqlPrepare,
                                       & sCommitSCN,
                                       aSession->mInfo.mToBeShardMetaNumber )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTrans::rollbackLocal( mmcTransObj *aTrans,
                                mmcSession  *aSession,
                                const SChar *aSavePoint,
                                UInt aTransReleasePolicy,
                                idBool aIsSqlPrepare,
                                mmcTransEndAction aEndAction )
{
    idBool       sSetBlock    = ID_FALSE;
    idBool       sIsLock      = ID_FALSE;
    idBool       sIsLastBegin = ID_FALSE;

    if (aSavePoint == NULL)
    {
        if ( isShareableTrans(aTrans) == ID_TRUE )
        {
            fixSharedTrans( aTrans, aSession->getSessionID() );
            sIsLock = ID_TRUE;

            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "rollbackLocal: locked");

            IDE_TEST_CONT( aEndAction == MMC_TRANS_SESSION_ONLY_END, SESSION_ONLY_ROLLBACK );

            sIsLastBegin = ( ( aTrans->mShareInfo->mTxInfo.mTransRefCnt == 1 ) &&
                             ( aSession->getSessionBegin() == ID_TRUE ) ) ?
                           ID_TRUE: ID_FALSE;

            if ( aSession->getSessionState() != MMC_SESSION_STATE_ROLLBACK )
            {
                setLocalTransactionBroken( aTrans,
                                           aSession->getSessionID(),
                                           ID_FALSE );
            }

            if ( isUserConnectedNode( aTrans ) == ID_TRUE )
            {
                /* Shared Transaction FSM: Rollback-User */
                switch ( aTrans->mShareInfo->mTxInfo.mState )
                {
                    case MMC_TRANS_STATE_PREPARE :
                        MMC_SHARED_TRANS_TRACE( aSession,
                                                aTrans,
                                                "rollbackLocal: endPendingBySlotN" );

                        IDE_TEST( endPendingBySlotN( aTrans,
                                                     aSession,
                                                     NULL,
                                                     ID_FALSE,  /* Rollback */
                                                     ID_TRUE,   /* Self */
                                                     NULL )
                                  != IDE_SUCCESS );

                        setTransactionPrepareSlot( aTrans, MMC_TRANS_NULL_SLOT_NO );
                        aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_END;
                        break;

                    default:
                        /* Nothing to do */
                        break;
                }
            }

            if ( ( sIsLastBegin == ID_TRUE ) ||
                 ( aSession->getSessionState() != MMC_SESSION_STATE_ROLLBACK ) )
            {
                IDU_FIT_POINT_RAISE( "mmcTrans::rollbackLocal-1::stateUnexpected",
                                     ERR_STATE_UNEXPECTED_1 );

                /* Shared Transaction FSM: Rollback-1 */
                switch ( aTrans->mShareInfo->mTxInfo.mState )
                {
                    case MMC_TRANS_STATE_PREPARE :
                        /* Nothing to do to keep pending tx */
                        break;

                    case MMC_TRANS_STATE_BEGIN :
                        MMC_SHARED_TRANS_TRACE( aSession,
                                                aTrans,
                                                "rollbackLocal: rollbackRaw");

                        IDE_TEST( mmcTrans::rollbackRaw( aTrans,
                                                         aSession,
                                                         aSession->getEventFlag(),
                                                         aTransReleasePolicy )
                                  != IDE_SUCCESS );

                        aTrans->mShareInfo->mTxInfo.mState = MMC_TRANS_STATE_END;

                        IDE_DASSERT( SM_SCN_IS_INIT( aTrans->mShareInfo->mTxInfo.mCommitSCN ) );
                        break;

                    case MMC_TRANS_STATE_INIT_DONE :
                    case MMC_TRANS_STATE_END :
                        /* Nothing to do */
                        break;

                    default:
                        IDE_RAISE( ERR_STATE_UNEXPECTED_1 );
                        break;

                }
            }

            IDE_EXCEPTION_CONT( SESSION_ONLY_ROLLBACK );

            aSession->setTransPrepared( NULL );

            if ( aSession->getSessionBegin() == ID_TRUE )
            {
                --aTrans->mShareInfo->mTxInfo.mTransRefCnt;
            }

            if ( aSession->isShardLibrarySession() == ID_TRUE )
            {
                MMC_SHARED_TRANS_TRACE( aSession,
                                        aTrans,
                                        "rollbackLocal: remove delegate");

                removeDelegatedSession( aTrans->mShareInfo, aSession );
            }

            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "rollbackLocal: unlock");

            sIsLock = ID_FALSE;
            unfixSharedTrans( aTrans, aSession->getSessionID() );
        }
        else
        {
            /* non-shared transaction */
            if ( ( aSession->getSessionBegin() == ID_TRUE ) ||
                 ( aSession->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) ||
                 ( aIsSqlPrepare == ID_TRUE ) )
            {
                IDE_TEST( mmcTrans::rollbackRaw( aTrans,
                                                 aSession,
                                                 aSession->getEventFlag(),
                                                 aTransReleasePolicy )
                          != IDE_SUCCESS );
            }
        }

        IDV_SESS_ADD_DIRECT(aSession->getStatistics(), IDV_STAT_INDEX_ROLLBACK_COUNT, 1);
    }
    else
    {
        /* aSavePoint != NULL */

        //PROJ-1677 DEQUEUE  
        aSession->setPartialRollbackFlag();

        if ( isShareableTrans(aTrans) == ID_TRUE )
        {
            fixSharedTrans( aTrans, aSession->getSessionID() );
            sIsLock = ID_TRUE;

            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "rollbackLocal(savepoint): locked");

            IDE_TEST_RAISE( ( getLocalTransactionBroken( aTrans ) == ID_TRUE ) &&
                            ( aSession->isGTx() == ID_TRUE ),
                            ERR_TRANS_BROKEN );

            IDU_FIT_POINT_RAISE( "mmcTrans::rollbackLocal-2::stateUnexpected",
                                 ERR_STATE_UNEXPECTED_2 );

            /* Shared Transaction FSM: Rollback-2 */
            switch ( aTrans->mShareInfo->mTxInfo.mState )
            {
                case MMC_TRANS_STATE_PREPARE :
                    /* TODO how can I rollback to savepoint */
                    IDE_RAISE( ERR_STATE_UNEXPECTED_3 );
                    break;

                case MMC_TRANS_STATE_BEGIN :
                    MMC_SHARED_TRANS_TRACE( aSession,
                                            aTrans,
                                            "rollbackLocal(savepoint): rollback to savepoint");

                    IDU_SESSION_SET_BLOCK(*aSession->getEventFlag());
                    sSetBlock = ID_TRUE;

                    setGlobalTxID4Trans( aSavePoint, aSession ); /* BUG-48703 */
                    IDE_TEST(aTrans->mSmiTrans.rollback(aSavePoint, aTransReleasePolicy) != IDE_SUCCESS);

                    sSetBlock = ID_FALSE;
                    IDU_SESSION_CLR_BLOCK(*aSession->getEventFlag());

                    IDE_DASSERT( SM_SCN_IS_INIT( aTrans->mShareInfo->mTxInfo.mCommitSCN ) );
                    break;

                case MMC_TRANS_STATE_INIT_DONE :
                case MMC_TRANS_STATE_END :
                    /* Nothing to do */
                    break;

                default:
                    IDE_RAISE( ERR_STATE_UNEXPECTED_2 );
                    break;

            }

            MMC_SHARED_TRANS_TRACE( aSession,
                                    aTrans,
                                    "rollbackLocal(savepoint): unlock");

            sIsLock = ID_FALSE;
            unfixSharedTrans( aTrans, aSession->getSessionID() );
        }
        else
        {
            /* non-shared transaction */
            IDU_SESSION_SET_BLOCK(*aSession->getEventFlag());
            sSetBlock = ID_TRUE;

            setGlobalTxID4Trans( aSavePoint, aSession ); /* BUG-48703 */
            IDE_TEST(aTrans->mSmiTrans.rollback(aSavePoint, aTransReleasePolicy) != IDE_SUCCESS);

            sSetBlock = ID_FALSE;
            IDU_SESSION_CLR_BLOCK(*aSession->getEventFlag());
        }

        IDV_SESS_ADD_DIRECT(aSession->getStatistics(), IDV_STAT_INDEX_ROLLBACK_COUNT, 1);
    }

    IDE_TEST( doAfterRollback( aSession, 
                               aTransReleasePolicy,
                               aIsSqlPrepare,
                               aSavePoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATE_UNEXPECTED_1 )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID,
                                  getSharedTransStateString( aTrans ),
                                  "rollbackLocal-1" ) );
    }
    IDE_EXCEPTION( ERR_STATE_UNEXPECTED_2 )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID,
                                  getSharedTransStateString( aTrans ),
                                  "rollbackLocal-2" ) );
    }
    IDE_EXCEPTION( ERR_STATE_UNEXPECTED_3 )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SHARED_TRANSACTION_STATE_INVALID,
                                  getSharedTransStateString( aTrans ),
                                  "rollbackLocal-3" ) );
    }
    IDE_EXCEPTION( ERR_TRANS_BROKEN )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_COMMIT_FAILED ) );
    }
    IDE_EXCEPTION_END;

    if (sSetBlock == ID_TRUE)
    {
        IDU_SESSION_CLR_BLOCK(*aSession->getEventFlag());
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsLock == ID_TRUE )
    {
        MMC_SHARED_TRANS_TRACE( aSession,
                                aTrans,
                                "rollbackLocal: exception: unlock");

        sIsLock = ID_FALSE;
        unfixSharedTrans( aTrans, aSession->getSessionID() );
    }

    return IDE_FAILURE;
}

const SChar * mmcTrans::getSharedTransStateString( mmcTransObj * aTrans )
{
    const SChar *sString = "NA";

    switch ( aTrans->mShareInfo->mTxInfo.mState )
    {
        case MMC_TRANS_STATE_NONE            : sString = "NONE"           ; break;
        case MMC_TRANS_STATE_INIT_DONE       : sString = "INIT_DONE"      ; break;
        case MMC_TRANS_STATE_BEGIN           : sString = "BEGIN"          ; break;
        case MMC_TRANS_STATE_PREPARE         : sString = "PREPARE"        ; break;
        case MMC_TRANS_STATE_END             : sString = "END"            ; break;
        default:                                                            break;
    }
    return sString;
}

void mmcTrans::shardedTransTrace( mmcSession  * aSession,
                                  mmcTransObj * aTrans,
                                  const SChar * aStr,
                                  ID_XID      * aXID )
{
    const SChar     * sSessionClientAppInfo         = "NULL";
    const SChar     * sSessionShardNodeName         = "NULL";
    sdiShardPin       sSessionShardPin              = SDI_SHARD_PIN_INVALID;
    mmcSessID         sSessionID                    = ID_UINT_MAX;
    mmcTransID        sSessionTransID               = ID_UINT_MAX;
    UInt              sSessionBegin                 = ID_UINT_MAX;
    mmcTransObj     * sSessionTransPtr              = NULL;

    mmcTransID        sTransID                      = ID_UINT_MAX;
    UInt              sTransBegin                   = ID_UINT_MAX;
    mmcTransObj     * sTransPtr                     = NULL;

    UInt              sSharedAllocCnt               = ID_UINT_MAX;
    UInt              sSharedRefCnt                 = ID_UINT_MAX;
    idBool            sSharedBroken                 = ID_FALSE;
    const SChar     * sSharedState                  = NULL;


    mmcSession      * sdelegateSessionPtr           = NULL;
    mmcSessID         sdelegateSessionID            = ID_UINT_MAX;
    const SChar     * sdelegateSessionClientAppInfo = "NULL";
    const SChar     * sdelegateSessionShardNodeName = "NULL";
    sdiShardPin       sdelegateSessionShardPin      = SDI_SHARD_PIN_INVALID;

    SInt              sLen;
    UChar             sXidString[XID_DATA_MAX_LEN + 6]  = { 0, };

    if ( aXID != NULL )
    {
        idlOS::strcpy( (SChar*)sXidString, "[XID:" );
        sLen = idaXaConvertXIDToString( NULL, aXID, &sXidString[5], XID_DATA_MAX_LEN );
        idlOS::strcpy( (SChar*)&sXidString[sLen + 5], "]" );
    }

    if ( aSession != NULL )
    {
        sSessionClientAppInfo = aSession->mInfo.mClientAppInfo;
        sSessionShardNodeName = aSession->mInfo.mShardNodeName;
        sSessionShardPin      = aSession->mInfo.mShardPin;
        sSessionID            = aSession->mInfo.mSessionID;
        sSessionTransID       = aSession->mInfo.mTransID;
        sSessionBegin         = aSession->mSessionBegin;
        sSessionTransPtr      = aSession->mTrans;
    }

    if ( aTrans != NULL )
    {
        sTransID              = mmcTrans::getTransID(aTrans);
        sTransBegin           = aTrans->mSmiTrans.isBegin();
        sTransPtr             = aTrans;

        if ( aTrans->mShareInfo != NULL )
        {
            sSharedAllocCnt = aTrans->mShareInfo->mTxInfo.mAllocRefCnt;
            sSharedRefCnt   = aTrans->mShareInfo->mTxInfo.mTransRefCnt;
            sSharedBroken   = getLocalTransactionBroken( aTrans );
            sSharedState    = getSharedTransStateString( aTrans );

            sdelegateSessionPtr = aTrans->mShareInfo->mTxInfo.mDelegatedSessions;
            if ( sdelegateSessionPtr != NULL )
            {
                sdelegateSessionID            = sdelegateSessionPtr->getSessionID();
                sdelegateSessionClientAppInfo = sdelegateSessionPtr->mInfo.mClientAppInfo;
                sdelegateSessionShardNodeName = sdelegateSessionPtr->mInfo.mShardNodeName;
                sdelegateSessionShardPin      = sdelegateSessionPtr->mInfo.mShardPin;
            }
        }
    }

    ideLog::log( IDE_SD_32, "[SHARED_TX] "
                            "[THREAD %"ID_UINT64_FMT"] "
                            "[CONNECTION CLI_APP_INFO:%s|NODE:%s|SHARD_PIN:0x%"ID_XINT64_FMT"] "
                            "[SESSION%s ID:%"ID_INT32_FMT"|TX_ID:%"ID_INT32_FMT"|BEGIN:%"ID_INT32_FMT"|TRANS:0x%"ID_vxULONG_FMT"] "
                            "[TRANS TX_ID:%"ID_INT32_FMT"|BEGIN:%"ID_INT32_FMT"|TRANS:0x%"ID_vxULONG_FMT"] "
                            "[SHARED ALLOC:%"ID_INT32_FMT"|REF:%"ID_INT32_FMT"|BROKEN:%"ID_INT32_FMT"|STATE:%s] "
                            "[DELEGATE SESSION_ID:%"ID_INT32_FMT"|CLI_APP_INFO:%s|NODE:%s|SHARD_PIN:0x%"ID_XINT64_FMT"] "
                            "[%s]"
                            " %s",
                            idtContainer::getSysThreadNumber(),
                            sSessionClientAppInfo, sSessionShardNodeName, sSessionShardPin,
                            (aSession != NULL ? "" : "(NULL)"), sSessionID, sSessionTransID, sSessionBegin, sSessionTransPtr,
                            sTransID, sTransBegin, sTransPtr,
                            sSharedAllocCnt, sSharedRefCnt, sSharedBroken, sSharedState,
                            sdelegateSessionID, sdelegateSessionClientAppInfo, sdelegateSessionShardNodeName, sdelegateSessionShardPin,
                            aStr,
                            sXidString );
}

void mmcTrans::endPendingTrace( mmcSession    * aSession,
                                smiTrans      * aSmiTrans,
                                ID_XID        * aXID,
                                idBool          aIsCommit,
                                const SChar   * aStr )
{
    const SChar     * sSessionClientAppInfo         = "NULL";
    const SChar     * sSessionShardNodeName         = "NULL";
    sdiShardPin       sSessionShardPin              = SDI_SHARD_PIN_INVALID;
    mmcSessID         sSessionID                    = ID_UINT_MAX;
    mmcTransID        sSessionTransID               = ID_UINT_MAX;
    UInt              sSessionBegin                 = ID_UINT_MAX;
    mmcTransObj     * sSessionTransPtr              = NULL;

    smTID             sTransID                      = ID_UINT_MAX;
    UInt              sTransBegin                   = ID_UINT_MAX;
    smxTrans        * sTransPtr                     = NULL;

    UChar             sXidString[XID_DATA_MAX_LEN]  = { 0, };

    if ( aSession != NULL )
    {
        sSessionClientAppInfo = aSession->mInfo.mClientAppInfo;
        sSessionShardNodeName = aSession->mInfo.mShardNodeName;
        sSessionShardPin      = aSession->mInfo.mShardPin;
        sSessionID            = aSession->mInfo.mSessionID;
        sSessionTransID       = aSession->mInfo.mTransID;
        sSessionBegin         = aSession->mSessionBegin;
        sSessionTransPtr      = aSession->mTrans;
    }

    if ( aSmiTrans != NULL )
    {
        sTransID              = aSmiTrans->getTransID();
        sTransBegin           = aSmiTrans->isBegin();
        sTransPtr             = (smxTrans *)aSmiTrans->getTrans();
    }

    if ( aXID != NULL )
    {
        (void)idaXaConvertXIDToString( NULL, aXID, sXidString, XID_DATA_MAX_LEN );
    }

    ideLog::log( IDE_SD_32, "[NOTIFY_RECV] "
                            "[THREAD %"ID_UINT64_FMT"] "
                            "[CONNECTION CLI_APP_INFO:%s|NODE:%s|SHARD_PIN:0x%"ID_XINT64_FMT"] "
                            "[SESSION%s ID:%"ID_INT32_FMT"|TX_ID:%"ID_INT32_FMT"|BEGIN:%"ID_INT32_FMT"|TRANS:0x%"ID_vxULONG_FMT"] "
                            "[TRANS TX_ID:%"ID_INT32_FMT"|BEGIN:%"ID_INT32_FMT"|SMX_TRANS:0x%"ID_vxULONG_FMT"] "
                            "[INFO XID:\"%s\"|COMMIT:%"ID_INT32_FMT"] "
                            "[%s]",
                            idtContainer::getSysThreadNumber(),
                            sSessionClientAppInfo, sSessionShardNodeName, sSessionShardPin,
                            (aSession != NULL ? "" : "(NULL)"), sSessionID, sSessionTransID, sSessionBegin, sSessionTransPtr,
                            sTransID, sTransBegin, sTransPtr,
                            sXidString, aIsCommit,
                            aStr );
}

IDE_RC mmcTrans::collectPrepareSCN( mmcSession * aSession, smSCN * aPrepareSCN )
{
    IDU_FIT_POINT( "mmcTrans::collectPrepareSCN" );

    aSession->getCoordPrepareSCN( aSession->getShardClientInfo(), aPrepareSCN );

    if ( SM_SCN_IS_NOT_INIT( *aPrepareSCN ) )
    {
        /* For X$SESSION */
        SM_SET_SCN( &aSession->mInfo.mGCTxCommitInfo.mPrepareSCN, aPrepareSCN );

        IDE_TEST( sdi::syncSystemSCN4GCTx( aPrepareSCN, aPrepareSCN ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmcTrans::deployGlobalCommitSCN( mmcSession * aSession, smSCN * aGlobalCommitSCN )
{
    if ( SM_SCN_IS_NOT_INIT( *aGlobalCommitSCN ) )
    {
        /* For X$SESSION */
        SM_SET_SCN( &aSession->mInfo.mGCTxCommitInfo.mGlobalCommitSCN, aGlobalCommitSCN );
    }

    aSession->setCoordGlobalCommitSCN( aSession->getShardClientInfo(), aGlobalCommitSCN );
}

void mmcTrans::fixSharedTrans( mmcTransObj *aTrans, mmcSessID aSessionID )
{
    mmcTxConcurrency * sConcurrency = NULL; 

    sConcurrency = &aTrans->mShareInfo->mConcurrency;

    IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );

    while ( sConcurrency->mFixCount > 0 )
    {
        IDE_DASSERT( sConcurrency->mFixOwner != (ULong)PDL_INVALID_HANDLE );

        if ( ( sConcurrency->mFixOwner == aSessionID ) &&
             ( sConcurrency->mAllowRecursive == ID_TRUE ) )
        {
            /* Recursive lock allowed */
            break;
        }

        ++sConcurrency->mWaiterCount;
        (void)sConcurrency->mCondVar.wait( &sConcurrency->mMutex );
        --sConcurrency->mWaiterCount;
    }

    if ( sConcurrency->mFixOwner == (ULong)PDL_INVALID_HANDLE )
    {
        sConcurrency->mFixOwner = aSessionID;
    }

    IDE_DASSERT( aSessionID == sConcurrency->mFixOwner );

    sConcurrency->mAllowRecursive = ID_FALSE;
    ++sConcurrency->mFixCount;

    IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
}

IDE_RC mmcTrans::fixSharedTrans4Statement( mmcTransObj     * aTrans,
                                           mmcSession      * aSession,
                                           mmcTransFixFlag   aFlag )
{
    mmcTxConcurrency * sConcurrency = NULL; 
    mmcSessID          sSessionID   = aSession->getSessionID();

    sConcurrency = &aTrans->mShareInfo->mConcurrency;

    IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );

    while ( sConcurrency->mFixCount > 0 )
    {
        IDE_DASSERT( sConcurrency->mFixOwner != (ULong)PDL_INVALID_HANDLE );

        if ( ( sConcurrency->mFixOwner == sSessionID ) &&
             ( sConcurrency->mAllowRecursive == ID_TRUE ) )
        {
            /* Recursive lock allowed */
            break;
        }
        else
        {
            PDL_Time_Value     sTVWait;
            PDL_Time_Value     sTVIntv;

            sTVIntv.set( 1, 0 );    /* 1 second */
            sTVWait = idlOS::gettimeofday();
            sTVWait += sTVIntv;

            ++sConcurrency->mWaiterCount;

            if ( sConcurrency->mCondVar.timedwait( &sConcurrency->mMutex,
                                                   &sTVWait,
                                                   IDU_IGNORE_TIMEDOUT)
                 != IDE_SUCCESS )
            {
                IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
                idlOS::sleep( sTVIntv );
                IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );
            }

            --sConcurrency->mWaiterCount;
        }

        IDE_TEST( iduCheckSessionEvent( aSession->getStatSQL() )
                  != IDE_SUCCESS );
    }

    if ( sConcurrency->mFixOwner == (ULong)PDL_INVALID_HANDLE )
    {
        sConcurrency->mFixOwner = sSessionID;
    }

    IDE_DASSERT( sSessionID == sConcurrency->mFixOwner );

    if ( ( aFlag & MMC_TRANS_FIX_RECURSIVE ) == MMC_TRANS_FIX_RECURSIVE )
    {
        sConcurrency->mAllowRecursive = ID_TRUE;
    }
    else
    {
        sConcurrency->mAllowRecursive = ID_FALSE;
    }

    ++sConcurrency->mFixCount;

    IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );

    return IDE_FAILURE;

}

void mmcTrans::pauseFix( mmcTransObj          * aTrans,
                         mmcTxConcurrencyDump * aDump,
                         mmcSessID              aSessionID )
{
    mmcTxConcurrency * sConcurrency = NULL; 

    sConcurrency = &aTrans->mShareInfo->mConcurrency;

    IDE_DASSERT( aDump->isStored() == ID_FALSE );

    IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );

    if ( sConcurrency->mFixCount > 0 )
    {
        if ( sConcurrency->mFixOwner == aSessionID )
        {
            aDump->store( sConcurrency );

            sConcurrency->mFixCount = 0;
            sConcurrency->mFixOwner = (ULong)PDL_INVALID_HANDLE;
            sConcurrency->mAllowRecursive = ID_FALSE;

            if ( sConcurrency->mWaiterCount > 0 )
            {
                (void)sConcurrency->mCondVar.broadcast();
            }
        }
        else
        {
            /* Fixed by another session. Nothing to do. */
        }
    }
    else
    {
        IDE_DASSERT( sConcurrency->mFixOwner == (ULong)PDL_INVALID_HANDLE );

        /* Not fixed. Nothing to do. */
    }

    IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
}

void mmcTrans::resumeFix( mmcTransObj          * aTrans,
                          mmcTxConcurrencyDump * aDump,
                          mmcSessID              aSessionID )
{
    mmcTxConcurrency * sConcurrency = NULL; 

    ACP_UNUSED( aSessionID );

    if ( aDump->isStored() == ID_TRUE )
    {
        sConcurrency = &aTrans->mShareInfo->mConcurrency;

        IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );

        while ( sConcurrency->mFixCount > 0 )
        {
            IDE_DASSERT( sConcurrency->mFixOwner != (ULong)PDL_INVALID_HANDLE );

            ++sConcurrency->mWaiterCount;
            (void)sConcurrency->mCondVar.wait( &sConcurrency->mMutex );
            --sConcurrency->mWaiterCount;
        }

        IDE_DASSERT( sConcurrency->mFixCount == 0 );
        IDE_DASSERT( aDump->mFixOwner == aSessionID );

        aDump->restore( sConcurrency );

        IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( aDump->mFixOwner == (ULong)PDL_INVALID_HANDLE );
    }
}

void mmcTrans::unfixSharedTrans( mmcTransObj *aTrans, mmcSessID aSessionID )
{
    mmcTxConcurrency * sConcurrency = NULL; 

    sConcurrency = &aTrans->mShareInfo->mConcurrency;

    IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );

    IDE_DASSERT( sConcurrency->mFixOwner == aSessionID );
    if ( sConcurrency->mFixOwner == aSessionID )
    {
        --sConcurrency->mFixCount;

        IDE_DASSERT( sConcurrency->mFixCount >= 0 );

        if ( sConcurrency->mFixCount == 0 )
        {
            sConcurrency->mFixOwner = (ULong)PDL_INVALID_HANDLE;
        }
        else
        {
            /* Recursive unlock 이다.
             * 따라서 Recursive 가 이전에는 허용되고 있었다고 할 수 있다. */
            sConcurrency->mAllowRecursive = ID_TRUE;
        }

        if ( sConcurrency->mWaiterCount > 0 )
        {
            (void)sConcurrency->mCondVar.broadcast();
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
}

idBool mmcTrans::isSharableTransBegin( mmcTransObj * aObj )
{
    idBool             isBegin      = ID_FALSE;
    mmcTxConcurrency * sConcurrency = NULL; 

    if ( aObj->mShareInfo != NULL )
    {
        sConcurrency = &aObj->mShareInfo->mConcurrency;

        IDE_ASSERT( sConcurrency->mMutex.lock( NULL ) == IDE_SUCCESS );
        switch ( aObj->mShareInfo->mTxInfo.mState )
        {
            case MMC_TRANS_STATE_BEGIN:
                isBegin = ID_TRUE;
                break;
            default:
                break;
        }
        IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
    }

    return isBegin;
}

/**
 *  decideTotalRollback
 *
 *  Partial rollback이면 aSavePoint를 반환하고 Total rollback이면 NULL을 반환한다.
 */
const SChar *mmcTrans::decideTotalRollback(mmcTransObj *aTrans, const SChar *aSavePoint)
{
    /* BUG-48489 */
    const SChar *sDecidedSavePoint = aSavePoint;

    IDE_DASSERT(aSavePoint != NULL);

    /* $$SHARD_CLONE_PROC_PARTIAL_ROLLBACK
       $$SHARD_PARTIAL_ROLLBACK
       $$DDL_BEGIN_SAVEPOINT$$
       $$DDL_INFO_SAVEPOINT$$ */
    if (aSavePoint[0] == '$')
    {
        /* Partial rollback */
    }
    else if (aTrans->mSmiTrans.isBegin() == ID_TRUE)
    {
        if (aTrans->mSmiTrans.isExistExpSavepoint(aSavePoint) != ID_TRUE)
        {
            sDecidedSavePoint = NULL;  /* Total rollback */
        }
        else
        {
            /* Partial rollback */
        }
    }
    else
    {
        /* Partial rollback */
    }

    return sDecidedSavePoint;
}

/* BUG-48703 
   Transaction(smxTrans)에 GlobalTxID를 설정한다.
   smiTrans::commit 또는 rollback 직전에 호출되어야한다. */
void mmcTrans::setGlobalTxID4Trans( const SChar * aSavepoint, mmcSession * aSession )
{
    UInt sLocalTxId;
    UInt sGlobalTxId;

    if ( ( aSavepoint == NULL ) && /* savepoint rollback 시 GlobalTxID를 설정해서는 안된다. */
         ( aSession != NULL ) &&
         ( aSession->isGTx() ) &&
         ( aSession->getDatabaseLinkSession() != NULL ) &&
         ( aSession->getDatabaseLinkSession()->mSession != NULL ) )
    {
        sLocalTxId  = aSession->getDatabaseLinkSession()->mSession->mLocalTxId;
        sGlobalTxId = aSession->getDatabaseLinkSession()->mSession->mGlobalTxId;

        if ( sGlobalTxId != DK_INIT_GTX_ID )
        {
            IDE_DASSERT( sLocalTxId != DK_INIT_LTX_ID );

            smiSetGlobalTxId( sLocalTxId, sGlobalTxId );
        }
    }
}

