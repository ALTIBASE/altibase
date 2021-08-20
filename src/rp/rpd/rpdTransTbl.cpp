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
 * $Id: rpdTransTbl.cpp 91485 2021-08-17 23:46:35Z seulki $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smDef.h>
#include <smiMisc.h>
#include <smErrorCode.h>

#include <qci.h>
#include <rp.h>
#include <rpDef.h>
#include <rpdTransTbl.h>
#include <rpError.h>

#include <rpcManager.h>

rpdTransTbl::rpdTransTbl()
{
}

rpdTransTbl::~rpdTransTbl()
{
}

/***********************************************************************
 * Description : Replication Transaction Table초기화
 *
 * aFlag - [IN] 이 Table을 Replication Receiver가 사용하면
 *              RPD_TRANSTBL_USING_TRANS_POOL,
 *              아니면
 *              RPD_TRANSTBL_USING_TRANS_POOL_SENDER
 *
 **********************************************************************/
IDE_RC rpdTransTbl::initialize( rpdTransPoolType aType, UInt aTransactionPoolSize )
{
    UInt   i;
    idBool sSvpPoolInit = ID_FALSE;
    idBool sInitTransPool = ID_FALSE;
    UInt   sTransactionPoolSize = 0;

    mTransTbl    = NULL;
    mTblSize     = smiGetTransTblSize();
    (void)idCore::acpAtomicSet32( &mATransCnt, 0);
    mLFGCount    = 1; //[TASK-6757]LFG,SN 제거
    mEndSN       = SM_SN_NULL;
    mTransPoolType = aType;

    mOverAllocTransCount = 0;

    if ( aTransactionPoolSize > mTblSize )
    {
        sTransactionPoolSize = mTblSize;
    }
    else
    {
        sTransactionPoolSize = aTransactionPoolSize;
    }
    // BUG-28206 불필요한 Transaction Begin을 방지
    IDE_TEST(mSvpPool.initialize(IDU_MEM_RP_RPD,
                                 (SChar *)"RP_SAVEPOINT_POOL",
                                 1,
                                 ID_SIZEOF(rpdSavepointEntry),
                                 mTblSize * RP_INIT_SAVEPOINT_COUNT_PER_TX,
                                 IDU_AUTOFREE_CHUNK_LIMIT, //chunk max(default)
                                 ID_FALSE,                 //use mutex(no use)
                                 8,                        //align byte(default)
                                 ID_FALSE,				   //ForcePooling
                                 ID_TRUE,				   //GarbageCollection
                                 ID_TRUE,                  /* HWCacheLine */
                                 IDU_MEMPOOL_TYPE_LEGACY   /* mempool type*/) 
              != IDE_SUCCESS);			
    sSvpPoolInit = ID_TRUE;

    IDU_FIT_POINT_RAISE( "rpdTransTbl::initialize::malloc::TransTbl",
                          ERR_MEMORY_ALLOC_TRANS_TABLE );
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPD,
                                     mTblSize * ID_SIZEOF(rpdTransTblNode),
                                     (void**)&mTransTbl,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_TRANS_TABLE);

    /* BUG-18045 [Eager Mode] 같은 Tx Slot을 사용하는 이전 Tx의 영향을 배제한다. */
    IDE_TEST_RAISE(mAbortTxMutex.initialize((SChar *)"Repl_Tx_Table_Mtx",
                                            IDU_MUTEX_KIND_NATIVE,
                                            IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    switch ( transPoolType() )
    {
        case RPD_TRANSTBL_USING_TRANS_POOL :
            IDE_TEST( initTransPool() != IDE_SUCCESS );
            sInitTransPool = ID_TRUE;

            for( i = 0; i < mTblSize; i++ )
            {
                mTransTbl[i].mLogAnlz = NULL;
                initTransNode(mTransTbl + i);

                if ( i < sTransactionPoolSize )
                {
                    IDE_TEST( allocTransNode() != IDE_SUCCESS );
                }
            }
            break;

        case RPD_TRANSTBL_USING_LOG_ANALYZER :
            for( i = 0; i < mTblSize; i++ )
            {
                mTransTbl[i].mLogAnlz = NULL;
                initTransNode( mTransTbl + i );
            }
            break;

        case RPD_TRANSTBL_USING_MEMORY_ALLOC :
            initFreeTransList();
            for( i = 0; i < mTblSize; i++ )
            {
                mTransTbl[i].mLogAnlz = NULL;
                initTransNode(mTransTbl + i);
            }
            break;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_TRANS_TABLE);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::initialize",
                                "mTransTbl"));
    }
    IDE_EXCEPTION(ERR_MUTEX_INIT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrMutexInit));
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Mutex initialization error");
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);
    IDE_PUSH();

    if ( sInitTransPool == ID_TRUE ) 
    {
        switch ( transPoolType() )
        {
            case RPD_TRANSTBL_USING_TRANS_POOL :
                destroyTransPool();
                break;

            case RPD_TRANSTBL_USING_LOG_ANALYZER :
                break;

            case RPD_TRANSTBL_USING_MEMORY_ALLOC :
                removeFreeTransList();
                break;
        }
    }
    
    if(mTransTbl != NULL)
    {
        (void)iduMemMgr::free(mTransTbl);
        mTransTbl = NULL;
    }

    if(sSvpPoolInit == ID_TRUE)
    {
        (void)mSvpPool.destroy(ID_FALSE);
    }

    IDE_POP();
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Replication Transaction Table소멸자
 *
 **********************************************************************/
void rpdTransTbl::destroy()
{
    UInt i;

    // BUG-28206 불필요한 Transaction Begin을 방지
    if(mSvpPool.destroy(ID_FALSE) != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    switch ( transPoolType() )
    {
        case RPD_TRANSTBL_USING_TRANS_POOL :
            destroyTransPool();
            break;

        case RPD_TRANSTBL_USING_LOG_ANALYZER :
            break;

        case RPD_TRANSTBL_USING_MEMORY_ALLOC :
            removeFreeTransList();
            break;
    }

    for(i = 0; i < mTblSize; i++)
    {
        if(mTransTbl[i].mLogAnlz != NULL)
        {
            (void)iduMemMgr::free(mTransTbl[i].mLogAnlz);
            mTransTbl[i].mLogAnlz = NULL;
        }
    }

    /* Transaction Table에 할당된 모든 Memory를 해제한다.*/
    (void)iduMemMgr::free(mTransTbl);
    mTransTbl = NULL;

    if(mAbortTxMutex.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    switch ( transPoolType() )
    {
        case RPD_TRANSTBL_USING_TRANS_POOL :
        case RPD_TRANSTBL_USING_LOG_ANALYZER :
            IDE_ASSERT( idCore::acpAtomicGet32( &mATransCnt ) == 0);
            break;

        case RPD_TRANSTBL_USING_MEMORY_ALLOC :
            /* Use the ATrans other thread.
             * Therefore, leave ATrans without deleting it. */
            break;
    }

    return;
}

/***********************************************************************
 * Description : rpdTransTblNode 초기화
 *
 * aTransNode  - [IN] 초기화를 수행할 rpdTransTblNode
 *
 **********************************************************************/
void rpdTransTbl::initTransNode(rpdTransTblNode *aTransNode)
{
    aTransNode->mRepName = NULL;
    aTransNode->mRemoteTID = ID_UINT_MAX;
    aTransNode->mMyTID = RP_TID_NULL;
    aTransNode->mBeginFlag = ID_FALSE;
    aTransNode->mAbortSN = SM_SN_NULL;
    aTransNode->mTxListIdx = ID_UINT_MAX;
    aTransNode->mIsConflict = ID_FALSE;
    aTransNode->mSendBeginFlag = ID_FALSE;

    /* BUG-21858 DML + LOB 처리 */
    aTransNode->mSkipLobLogFlag = ID_FALSE;

    //BUG-24398
    aTransNode->mSendLobLogFlag = ID_FALSE;

    /* BUG-18045 [Eager Mode] 같은 Tx Slot을 사용하는 이전 Tx의 영향을 배제한다. */
    IDE_ASSERT(mAbortTxMutex.lock( NULL /* idvSQL* */) == IDE_SUCCESS);
    aTransNode->mBeginSN = SM_SN_NULL;
    aTransNode->mAbortFlag = ID_FALSE;
    IDE_ASSERT(mAbortTxMutex.unlock() == IDE_SUCCESS);

    aTransNode->mLocHead.mNext = NULL;
    aTransNode->mSNMapEntry    = NULL;

    /* PROJ-1442 Replication Online 중 DDL 허용
     * DDL Transaction 정보 초기화
     */
    aTransNode->mIsDDLTrans = ID_FALSE;
    IDU_LIST_INIT(&aTransNode->mItemMetaList);

    aTransNode->mDDLStmtMetaLog = NULL;

    // BUG-28206 불필요한 Transaction Begin을 방지
    IDU_LIST_INIT(&aTransNode->mSvpList);
    aTransNode->mPSMSvp        = NULL;
    aTransNode->mIsSvpListSent = ID_FALSE;
    aTransNode->mTrans.mTransForConflictResolution = NULL;
    aTransNode->mTrans.mSetPSMSavepoint = ID_FALSE;

    aTransNode->mGlobalTxID         = SM_NULL_TID;
    aTransNode->mGlobalTxAckSN      = SM_SN_NULL;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 할당하고 초기화한다.
 *
 * aRemoteTID - [IN]  Transaction ID
 * aBeginSN   - [IN]  Transaction 의 Begin SN
 *
 **********************************************************************/
IDE_RC rpdTransTbl::insertTrans(iduMemAllocator * aAllocator,
                                smTID       aRemoteTID,
                                smSN        aBeginSN,
                                iduMemPool *aChainedValuePool)
{
    UInt           sIndex;
    smiTransNode * sTrans = NULL;

    // BUG-17548
    IDE_ASSERT(aBeginSN != SM_SN_NULL);

    /* aRemoteTID가 사용할 Transaction Slot을 찾는다.*/
    sIndex = aRemoteTID % mTblSize;

    /* 동시에 같은 Transaction Table Slot을 사용하는지 검사한다. */
    IDE_TEST_RAISE(isATransNode(&(mTransTbl[sIndex])) == ID_TRUE,
                   ERR_ALREADY_BEGIN);

    /* TID를 Setting한다 */
    mTransTbl[sIndex].mRemoteTID = aRemoteTID;
    mTransTbl[sIndex].mSendBeginFlag = ID_FALSE;

    /* BUG-18045 [Eager Mode] 같은 Tx Slot을 사용하는 이전 Tx의 영향을 배제한다. */
    IDE_ASSERT(mAbortTxMutex.lock( NULL /* idvSQL* */) == IDE_SUCCESS);
    mTransTbl[sIndex].mBeginSN = aBeginSN;
    IDE_ASSERT(mAbortTxMutex.unlock() == IDE_SUCCESS);

    switch ( transPoolType() )
    {
        case RPD_TRANSTBL_USING_TRANS_POOL :
        case RPD_TRANSTBL_USING_MEMORY_ALLOC :
            IDE_TEST_RAISE( getTransNode( &sTrans ) != IDE_SUCCESS, ERR_GET_TRANS );
            mTransTbl[sIndex].mTrans.mRpdTrans = sTrans;
            break;

        case RPD_TRANSTBL_USING_LOG_ANALYZER :
            if(mTransTbl[sIndex].mLogAnlz == NULL)
            {
                IDU_FIT_POINT_RAISE( "rpdTransTbl::insertTrans::malloc::LogAnalyzer",
                                     ERR_MEMORY_ALLOC_LOG_ANALYZER,
                                     rpERR_ABORT_MEMORY_ALLOC,
                                     "rpdTransTbl::insertTrans",
                                     "mTransTbl[sIndex].mLogAnlz" );
                IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPD,
                                                 ID_SIZEOF(rpdLogAnalyzer),
                                                 (void**)&mTransTbl[sIndex].mLogAnlz,
                                                 IDU_MEM_IMMEDIATE)
                               != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOG_ANALYZER);
                new (mTransTbl[sIndex].mLogAnlz) rpdLogAnalyzer;

                IDE_TEST(mTransTbl[sIndex].mLogAnlz->initialize(aAllocator, aChainedValuePool)
                         != IDE_SUCCESS);
            }
            else
            {
                //nothing to do 
            }
            break;
    }

    /* 새로운 Transaction이 Begin되었으므로 Active Trans Count를
       늘려준다.*/
    (void)idCore::acpAtomicInc32( &mATransCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ALREADY_BEGIN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_ENTRY_EXIST));

        ideLog::log(IDE_RP_0, RP_TRC_TT_DUPLICATED_BEGIN,
                    mTransTbl[sIndex].mBeginSN,
                    aBeginSN,
                    aRemoteTID,
                    mTblSize);
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_LOG_ANALYZER);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::insertTrans",
                                "mTransTbl[sIndex].mLogAnlz"));

        mTransTbl[sIndex].mLogAnlz = NULL;
        initTransNode( &(mTransTbl[sIndex]) );
    }
    IDE_EXCEPTION(ERR_GET_TRANS);
    {
        initTransNode( &(mTransTbl[sIndex]) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 반환하고 각각의
 *               member에 대해서 소멸자를 호출한다.
 *
 * aRemoteTID - [IN]  end시킬 Transaction ID
 *
 **********************************************************************/
void rpdTransTbl::removeTrans(smTID  aRemoteTID)
{
    iduListNode       * sNode;
    iduListNode       * sDummy;
    rpdItemMetaEntry  * sItemMetaEntry;
    rpdSavepointEntry * sSavepointEntry;
    UInt                sIndex;
    rpdTransTblNode   * sTransTblNode = NULL;

    sIndex = aRemoteTID % mTblSize;

    sTransTblNode = &(mTransTbl[sIndex]);

    if ( isATransNode( sTransTblNode ) == ID_TRUE )
    {
        switch (transPoolType() )
        {
            case RPD_TRANSTBL_USING_TRANS_POOL :
                /*return transaction to transaction pool*/
                if ( sTransTblNode->mTrans.mTransForConflictResolution != NULL )
                {
                    removeTransNode( sTransTblNode->mTrans.mTransForConflictResolution );
                }
                else
                {
                    /* Nothing to do */
                }
                removeTransNode( sTransTblNode->mTrans.mRpdTrans );
                break;

            case RPD_TRANSTBL_USING_MEMORY_ALLOC :
                removeFreeTransNode( sTransTblNode->mTrans.mRpdTrans );
                sTransTblNode->mTrans.mRpdTrans = NULL;
                break;

            case RPD_TRANSTBL_USING_LOG_ANALYZER :
                if ( sTransTblNode->mLogAnlz != NULL )
                {
                    sTransTblNode->mLogAnlz->destroy();
                }
                else
                {
                    /* Nothing to do */
                }

                if ( sTransTblNode->mDDLStmtMetaLog != NULL )
                {
                    if ( sTransTblNode->mDDLStmtMetaLog->mLogBody != NULL )
                    {
                        (void)iduMemMgr::free( sTransTblNode->mDDLStmtMetaLog->mLogBody );
                        sTransTblNode->mDDLStmtMetaLog->mLogBody = NULL;
                    }

                    (void)iduMemMgr::free( sTransTblNode->mDDLStmtMetaLog );
                    sTransTblNode->mDDLStmtMetaLog = NULL;
                }

                /* PROJ-1442 Replication Online 중 DDL 허용
                 * DDL Transaction 정보 제거
                 */
                IDU_LIST_ITERATE_SAFE( &sTransTblNode->mItemMetaList, sNode, sDummy )
                {
                    sItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
                    IDU_LIST_REMOVE(sNode);

                    if ( sItemMetaEntry->mLogBody != NULL )
                    {
                        (void)iduMemMgr::free(sItemMetaEntry->mLogBody);
                    }

                    (void)iduMemMgr::free(sItemMetaEntry);
                }
                break;
        }

        /* BUG-28206 불필요한 Transaction Begin을 방지 (Sender) */
        /* BUG-18028 Eager Mode에서 Partial Rollback 지원 (Receiver) */
        IDU_LIST_ITERATE_SAFE( &sTransTblNode->mSvpList, sNode, sDummy )
        {
            sSavepointEntry = (rpdSavepointEntry *)sNode->mObj;
            IDU_LIST_REMOVE(sNode);

            (void)mSvpPool.memfree(sSavepointEntry);
        }

        initTransNode( sTransTblNode );

        /* Transaction이 end되었으므로 Active Trans Count를 줄여준다. */
        (void)idCore::acpAtomicDec32( &mATransCnt );
    }

    return;
}

idBool rpdTransTbl::isExistActiveTrans()
{
    if ( idCore::acpAtomicGet32( &mATransCnt ) > 0 )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}
/***********************************************************************
 * Description : Active Transaction중에서 First Write Log의 SN중에서 가장
 *               작은 SN값을 가져온다.
 *
 * aSN - [OUT]  가장 작은 SN값을 Return할 변수
 *
 **********************************************************************/
void rpdTransTbl::getMinTransFirstSN( smSN * aSN )
{
    UInt    i;
    smSN    sMinFirstSN = SM_SN_NULL;

    IDE_ASSERT( aSN != NULL );

    if ( idCore::acpAtomicGet32( &mATransCnt ) > 0 )
    {
        for(i = 0; i < mTblSize; i++)
        {
            if(isATransNode(mTransTbl + i) == ID_FALSE)
            {
                continue;
            }
            else
            {
                /* Active Transaction은 mBeginSN이 SM_SN_NULL이 아니다.
                 * sender가 mBeginSN을 set하고 sender가 읽으므로
                 * mBeginSN을 읽을 때 lock을 안 잡아도 됨
                 */
                if(sMinFirstSN > mTransTbl[i].mBeginSN)
                {
                    sMinFirstSN = mTransTbl[i].mBeginSN;
                }
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aSN = sMinFirstSN;
}

/***********************************************************************
 * Description : 모든 Active Transaction을 Rollback시키고 할당된 Slot을 반환
 *               한다.
 *
 *
 **********************************************************************/
void rpdTransTbl::rollbackAllATrans()
{
    UInt i;
    smLobLocator     sRemoteLL = SMI_NULL_LOCATOR;
    smLobLocator     sLocalLL = SMI_NULL_LOCATOR;
    idBool           sIsExist;

    switch( transPoolType() )
    {
        case RPD_TRANSTBL_USING_TRANS_POOL :
        case RPD_TRANSTBL_USING_MEMORY_ALLOC :
            for(i = 0; i < mTblSize; i++)
            {
                if(isATransNode(mTransTbl + i) == ID_TRUE)
                {
                    /* getFirstLocator,removeLocator는 TID의 트랜잭션이
                     * Active가 아닐 때만 에러를 반환하기 때문에 아래의
                     *  if 블럭안에서는 에러가 반환되지 않는다.
                     */
                    IDE_ASSERT(getFirstLocator(mTransTbl[i].mRemoteTID,
                                               &sRemoteLL,
                                               &sLocalLL,
                                               &sIsExist)
                               == IDE_SUCCESS);

                    while(sIsExist == ID_TRUE)
                    {
                        removeLocator(mTransTbl[i].mRemoteTID, sRemoteLL);

                        if ( smiLob::closeLobCursor(NULL, sLocalLL) != IDE_SUCCESS )
                        {
                            IDE_WARNING( IDE_RP_0, RP_TRC_RA_ERR_LOB_CLOSE );
                        }
                        else
                        {
                            /* do noting */
                        }

                        IDE_ASSERT(getFirstLocator(mTransTbl[i].mRemoteTID,
                                                   &sRemoteLL,
                                                   &sLocalLL,
                                                   &sIsExist)
                                   == IDE_SUCCESS);
                    }
                    if ( mTransTbl[i].mTrans.rollback( mTransTbl[i].mRemoteTID, ID_TRUE ) != IDE_SUCCESS )
                    {
                        IDE_ERRLOG(IDE_RP_0);
                        IDE_CALLBACK_FATAL("[Repl] Transaction Rollback Failure");
                    }
                    removeTrans(mTransTbl[i].mRemoteTID);
                }
            }
            break;

    case RPD_TRANSTBL_USING_LOG_ANALYZER :
        /* Sender일 경우는 Transaction을 begin하지 않고
           정보만 유지하고 있기 때문에 Rollback할 필요가
           없다.*/
        for( i = 0; i < mTblSize; i++ )
        {
            if( isATransNode( mTransTbl + i ) == ID_TRUE )
            {
                removeTrans(mTransTbl[i].mRemoteTID);
            }

            if( mTransTbl[i].mLogAnlz != NULL )
            {
                (void)iduMemMgr::free( mTransTbl[i].mLogAnlz );
                mTransTbl[i].mLogAnlz = NULL;
            }
            else
            {
                /* nothing to do */
            }
        }
        break;
    }

    return;
}

/***********************************************************************
 * Description : aTID 트랜잭션 Entry에 Remote LOB Locator와 Local LOB Locator
 *               를 pair로 삽입한다.
 *
 * aTID      - [IN]  트랜잭션 ID
 * aRemoteLL - [IN]  삽입할 Remote LOB Locator(Sender쪽의 Locator)
 * aLocalLL  - [IN]  삽입할 Local LOB Locator(Reciever에서 open한 Locator)
 *
 * Return
 *   IDE_SUCCESS - 성공
 *   IDE_FAILURE
 *        - NOT Active : 현재 트랜잭션이 비활성 상태임
 *
 **********************************************************************/
IDE_RC rpdTransTbl::insertLocator(smTID aTID, smLobLocator aRemoteLL, smLobLocator aLocalLL)
{
    rpdTransTblNode *sTrNode   = NULL;
    rpdLOBLocEntry  *sLocEntry = NULL;

    IDE_TEST_RAISE(isATrans(aTID) != ID_TRUE, ERR_NOT_ACTIVE);
    sTrNode = getTrNode(aTID);

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPD,
                                     ID_SIZEOF(rpdLOBLocEntry),
                                     (void **)&sLocEntry,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOC_ENTRY);

    sLocEntry->mRemoteLL    = aRemoteLL;
    sLocEntry->mLocalLL     = aLocalLL;
    sLocEntry->mNext        = sTrNode->mLocHead.mNext;
    sTrNode->mLocHead.mNext = sLocEntry;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ACTIVE);
    {
        // 트랜잭션이 현재 비활성 상태임
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TRANSACTION_NOT_ACTIVE ) );
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_LOC_ENTRY);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::insertLocator",
                                "sLocEntry"));
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if(sLocEntry != NULL)
    {
        (void)iduMemMgr::free(sLocEntry);
    }

    IDE_POP();
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Remote LOB Locator를 Key로 트랜잭션 Entry에서
 *               Locator entry를 삭제한다.
 *
 * aTID      - [IN]  트랜잭션 ID
 * aRemoteLL - [IN]  삭제할 Remote LOB Locator(Sender쪽의 Locator)
 *
 **********************************************************************/
void rpdTransTbl::removeLocator(smTID aTID, smLobLocator aRemoteLL)
{
    rpdTransTblNode *sTrNode;
    rpdLOBLocEntry  *sPrevEntry;
    rpdLOBLocEntry  *sCurEntry;

    if(isATrans(aTID) == ID_TRUE)
    {
        sTrNode = getTrNode(aTID);

        sPrevEntry = &sTrNode->mLocHead;
        sCurEntry = sPrevEntry->mNext;

        while(sCurEntry != NULL)
        {
            if(sCurEntry->mRemoteLL == aRemoteLL)
            {
                sPrevEntry->mNext = sCurEntry->mNext;
                (void)iduMemMgr::free(sCurEntry);
                break;
            }
            sPrevEntry = sCurEntry;
            sCurEntry = sCurEntry->mNext;
        }
    }

    return;
}

/***********************************************************************
 * Description : aTID 트랜잭션 Entry에서 Remote LOB Locator를 Key로
 *               Local LOB Locator를 반환한다.
 *
 * aTID      - [IN]  트랜잭션 ID
 * aRemoteLL - [IN]  찾을 Remote LOB Locator(Sender쪽의 Locator)
 * aLocalLL  - [OUT] 반환할 Local LOB Locator 저장 변수(Reciever에서 open한 Locator)
 *
 * Return
 *   IDE_SUCCESS - 성공
 *   IDE_FAILURE
 *        - NOT Active : 현재 트랜잭션이 비활성 상태임
 *
 **********************************************************************/
IDE_RC rpdTransTbl::searchLocator( smTID          aTID,
                                   smLobLocator   aRemoteLL,
                                   smLobLocator  *aLocalLL,
                                   idBool        *aIsFound )
{
    rpdTransTblNode *sTrNode;
    rpdLOBLocEntry  *sCurEntry;

    *aIsFound = ID_FALSE;

    IDE_TEST_RAISE(isATrans(aTID) != ID_TRUE, ERR_NOT_ACTIVE);
    sTrNode = getTrNode(aTID);

    sCurEntry = sTrNode->mLocHead.mNext;

    while(sCurEntry != NULL)
    {
        if(sCurEntry->mRemoteLL == aRemoteLL)
        {
            *aLocalLL = sCurEntry->mLocalLL;
            *aIsFound = ID_TRUE;
            break;
        }
        sCurEntry = sCurEntry->mNext;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ACTIVE);
    {
        // 트랜잭션이 현재 비활성 상태임
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TRANSACTION_NOT_ACTIVE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aTID 트랜잭션 Entry의 Locator List에서 첫번째 entry의
 *               Local LOB Locator를 반환한다.
 *
 * aTID      - [IN]  트랜잭션 ID
 * aRemoteLL - [OUT] 반환할 Remote LOB Locator 저장 변수(Sender쪽 Locator)
 * aLocalLL  - [OUT] 반환할 Local LOB Locator 저장 변수(Reciever에서 open한 Locator)
 *
 * Return
 *   IDE_SUCCESS - 성공
 *   IDE_FAILURE
 *        - NOT Active : 현재 트랜잭션이 비활성 상태임
 *
 **********************************************************************/
IDE_RC rpdTransTbl::getFirstLocator( smTID          aTID,
                                     smLobLocator  *aRemoteLL,
                                     smLobLocator  *aLocalLL,
                                     idBool        *aIsExist )
{
    rpdTransTblNode *sTrNode;
    rpdLOBLocEntry  *sCurEntry;

    *aIsExist = ID_FALSE;

    IDE_TEST_RAISE(isATrans(aTID) != ID_TRUE, ERR_NOT_ACTIVE);
    sTrNode = getTrNode(aTID);

    sCurEntry = sTrNode->mLocHead.mNext;

    if(sCurEntry != NULL)
    {
        *aRemoteLL = sCurEntry->mRemoteLL;
        *aLocalLL = sCurEntry->mLocalLL;
        *aIsExist = ID_TRUE;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ACTIVE);
    {
        // 트랜잭션이 현재 비활성 상태임
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TRANSACTION_NOT_ACTIVE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTID 트랜잭션 Entry의 Item Meta List에 Entry를 추가한다.
 *
 * aTID                 - [IN]  트랜잭션 ID
 * aItemMeta            - [IN]  추가할 Item Meta
 * aItemMetaLogBody     - [IN]  추가할 Item Meta Body
 * aItemMetaLogBodySize - [IN]  추가할 Item Meta Body의 크기
 ******************************************************************************/
IDE_RC rpdTransTbl::addItemMetaEntry(smTID          aTID,
                                     smiTableMeta * aItemMeta,
                                     const void   * aItemMetaLogBody,
                                     UInt           aItemMetaLogBodySize,
                                     smSN           aLogSN)
{
    rpdItemMetaEntry * sItemMetaEntry = NULL;
    UInt               sIndex         = aTID % mTblSize;

    IDU_FIT_POINT_RAISE( "rpdTransTbl::addItemMetaEntry::malloc::ItemMetaEntry",
                         ERR_MEMORY_ALLOC_ITEM_META_ENTRY );
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPD,
                                     ID_SIZEOF(rpdItemMetaEntry),
                                     (void **)&sItemMetaEntry,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_ITEM_META_ENTRY);
    idlOS::memset((void *)sItemMetaEntry, 0, ID_SIZEOF(rpdItemMetaEntry));

    idlOS::memcpy((void *)&sItemMetaEntry->mItemMeta,
                  (const void  *)aItemMeta,
                  ID_SIZEOF(smiTableMeta));

    if ( aItemMetaLogBodySize > 0 )
    {
        IDU_FIT_POINT_RAISE( "rpdTransTbl::addItemMetaEntry::malloc::LogBody",
                             ERR_MEMORY_ALLOC_LOG_BODY );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                           (ULong)aItemMetaLogBodySize,
                                           (void **)&sItemMetaEntry->mLogBody,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOG_BODY);
        idlOS::memcpy( (void *)sItemMetaEntry->mLogBody,
                       aItemMetaLogBody,
                       aItemMetaLogBodySize );
    }
    else
    {
        sItemMetaEntry->mLogBody = NULL;
    }
    sItemMetaEntry->mLogSN = aLogSN;

    // 이후에는 실패하지 않는다
    IDU_LIST_INIT_OBJ(&(sItemMetaEntry->mNode), sItemMetaEntry);

    IDU_LIST_ADD_LAST(&mTransTbl[sIndex].mItemMetaList,
                      &(sItemMetaEntry->mNode));


    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MEMORY_ALLOC_ITEM_META_ENTRY);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::addItemMetaEntry",
                                "sItemMetaEntry"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_LOG_BODY);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::addItemMetaEntry",
                                "sItemMetaEntry->mLogBody"));
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if(sItemMetaEntry != NULL)
    {
        if(sItemMetaEntry->mLogBody != NULL)
        {
            (void)iduMemMgr::free(sItemMetaEntry->mLogBody);
        }

        (void)iduMemMgr::free(sItemMetaEntry);
    }

    IDE_POP();
    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTID 트랜잭션 Entry의 Item Meta List에서 첫 번째 Entry를 반환한다.
 *
 * aTID           - [IN]  트랜잭션 ID
 * aItemMetaEntry - [OUT] 반환될 첫 번째 Item Meta Entry
 ******************************************************************************/
void rpdTransTbl::getFirstItemMetaEntry(smTID               aTID,
                                        rpdItemMetaEntry ** aItemMetaEntry)
{
    iduListNode * sNode;
    iduListNode * sDummy;
    UInt          sIndex = aTID % mTblSize;

    *aItemMetaEntry = NULL;

    if(IDU_LIST_IS_EMPTY(&mTransTbl[sIndex].mItemMetaList) != ID_TRUE)
    {
        IDU_LIST_ITERATE_SAFE(&mTransTbl[sIndex].mItemMetaList, sNode, sDummy)
        {
            *aItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
            break;
        }
    }

    IDE_ASSERT(*aItemMetaEntry != NULL);

    return;
}

/*******************************************************************************
 * Description : aTID 트랜잭션 Entry의 Item Meta List에서 첫 번째 Entry를 제거한다.
 *
 * aTID           - [IN]  트랜잭션 ID
 ******************************************************************************/
void rpdTransTbl::removeFirstItemMetaEntry(smTID aTID)
{
    iduListNode      * sNode;
    iduListNode      * sDummy;
    UInt               sIndex = aTID % mTblSize;
    rpdItemMetaEntry * sItemMetaEntry = NULL;

    if(IDU_LIST_IS_EMPTY(&mTransTbl[sIndex].mItemMetaList) != ID_TRUE)
    {
        IDU_LIST_ITERATE_SAFE(&mTransTbl[sIndex].mItemMetaList, sNode, sDummy)
        {
            sItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
            IDU_LIST_REMOVE(sNode);

            if ( sItemMetaEntry->mLogBody != NULL )
            {
                (void)iduMemMgr::free( (void *)sItemMetaEntry->mLogBody );
            }

            (void)iduMemMgr::free((void *)sItemMetaEntry);
            break;
        }
    }

    return;
}

/*******************************************************************************
 * Description : aTID 트랜잭션 Entry의 Item Meta List에 Entry가 있는지 확인한다.
 *
 * aTID         - [IN]  트랜잭션 ID
 ******************************************************************************/
idBool rpdTransTbl::existItemMeta(smTID aTID)
{
    UInt sIndex = aTID % mTblSize;

    return (IDU_LIST_IS_EMPTY(&mTransTbl[sIndex].mItemMetaList) != ID_TRUE)
           ? ID_TRUE : ID_FALSE;
}

iduList * rpdTransTbl::getItemMetaList( smTID aTID )
{
    UInt sIndex = aTID % mTblSize;

    return &mTransTbl[sIndex].mItemMetaList;
}

idBool rpdTransTbl::existDDLStmtMetaLog( smTID aTID )
{
    UInt sIndex = aTID % mTblSize;

    return ( mTransTbl[sIndex].mDDLStmtMetaLog != NULL )
        ? ID_TRUE : ID_FALSE;
}

IDE_RC rpdTransTbl::setDDLStmtMetaLog( smTID            aTID,
                                       smiDDLStmtMeta * aDDLStmtMeta,
                                       const void     * aDDLStmtMetaLogBody,
                                       UInt             aDDLStmtMetaLogBodySize )
{
    UInt sIndex = aTID % mTblSize;
    rpdDDLStmtMetaLog * sDDLStmtMetaLog = NULL;

    IDE_ASSERT( aDDLStmtMetaLogBodySize > 0 );
    IDE_TEST_RAISE( mTransTbl[sIndex].mDDLStmtMetaLog != NULL, 
                    ERR_ALREADY_EXIST_DDL_STMT_LOG );

    IDU_FIT_POINT_RAISE( "rpdTransTbl::setDDLStmtMetaLog::malloc::sDDLStmtMetaLog",
                         ERR_MEMORY_ALLOC_DDL_STMT_META_ENTRY );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       ID_SIZEOF(rpdDDLStmtMetaLog),
                                       (void **)&sDDLStmtMetaLog,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DDL_STMT_META_ENTRY );
    idlOS::memset( (void *)sDDLStmtMetaLog, 0, ID_SIZEOF(rpdDDLStmtMetaLog) );

    idlOS::memcpy( (void *)&( sDDLStmtMetaLog->mDDLStmtMeta ),
                   (const void  *)aDDLStmtMeta,
                   ID_SIZEOF(smiDDLStmtMeta) );

    IDU_FIT_POINT_RAISE( "rpdTransTbl::setDDLStmtMetaLog::malloc::LogBody",
                         ERR_MEMORY_ALLOC_LOG_BODY );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       (ULong)aDDLStmtMetaLogBodySize,
                                       (void **)&sDDLStmtMetaLog->mLogBody,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOG_BODY);
    idlOS::memcpy( (void *)sDDLStmtMetaLog->mLogBody,
                   aDDLStmtMetaLogBody,
                   aDDLStmtMetaLogBodySize );

    mTransTbl[sIndex].mDDLStmtMetaLog = sDDLStmtMetaLog;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALREADY_EXIST_DDL_STMT_LOG );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "Already exist DDL statement log in trans table." ) )
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_DDL_STMT_META_ENTRY);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::setDDLStmtMetaLog",
                                "sDDLStmtMetaLog"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_LOG_BODY);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::setDDLStmtMetaLog",
                                "sDDLStmtMeta->mLogBody"));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sDDLStmtMetaLog != NULL )
    {
        if ( sDDLStmtMetaLog->mLogBody != NULL )
        {
            (void)iduMemMgr::free( sDDLStmtMetaLog->mLogBody );
            sDDLStmtMetaLog->mLogBody = NULL;
        }

        (void)iduMemMgr::free( sDDLStmtMetaLog );
        sDDLStmtMetaLog = NULL;
    }

    IDE_POP();
    return IDE_FAILURE;

}

void rpdTransTbl::getDDLStmtMetaLog( smTID aTID, rpdDDLStmtMetaLog ** aDDLStmtMetaLog )
{
    UInt sIndex = aTID % mTblSize;

    *aDDLStmtMetaLog = mTransTbl[sIndex].mDDLStmtMetaLog;
}

void rpdTransTbl::removeDDLStmtMetaLog( smTID aTID )
{
    UInt sIndex = aTID % mTblSize;
    rpdDDLStmtMetaLog * sDDLStmtMetaLog = NULL;

    sDDLStmtMetaLog = mTransTbl[sIndex].mDDLStmtMetaLog;
    IDE_DASSERT( sDDLStmtMetaLog != NULL );

    if ( sDDLStmtMetaLog->mLogBody != NULL )
    {
        (void)iduMemMgr::free( sDDLStmtMetaLog->mLogBody );
        sDDLStmtMetaLog->mLogBody = NULL;
    }

    (void)iduMemMgr::free( sDDLStmtMetaLog );
    sDDLStmtMetaLog = NULL;

    mTransTbl[sIndex].mDDLStmtMetaLog = NULL;
}

/*******************************************************************************
 * Description : Savepoint Entry List의 마지막에 Entry를 추가한다.
 *
 * aTID              - [IN]  트랜잭션 ID
 * aSN               - [IN]  추가할 Savepoint SN
 * aType             - [IN]  추가할 Savepoint Type
 * aSvpName          - [IN]  추가할 Savepoint Name
 * aImplicitSvpDepth - [IN]  Implicit Savepoint일 경우의 Depth
 ******************************************************************************/
IDE_RC rpdTransTbl::addLastSvpEntry(smTID            aTID,
                                    smSN             aSN,
                                    rpSavepointType  aType,
                                    SChar           *aSvpName,
                                    UInt             aImplicitSvpDepth)
{
    rpdTransTblNode   *sTransNode        = &mTransTbl[aTID % mTblSize];
    rpdSavepointEntry *sSvpEntry         = NULL;
    idBool             sNeedFreeSvpEntry = ID_FALSE;
    UInt               sSvpNameLen       = 0;

    IDE_ASSERT(aSvpName != NULL);

    IDU_FIT_POINT( "rpdTransTbl::addLastSvpEntry::alloc::SvpEntry" );
    IDE_TEST(mSvpPool.alloc((void**)&sSvpEntry) != IDE_SUCCESS);
    sNeedFreeSvpEntry = ID_TRUE;

    sSvpEntry->mSN    = aSN;
    sSvpEntry->mType  = aType;

    sSvpNameLen = idlOS::strlen( aSvpName );

    IDE_DASSERT( sSvpNameLen <= RP_SAVEPOINT_NAME_LEN );

    idlOS::memcpy( (void *)sSvpEntry->mName,
                   (const void  *)aSvpName,
                   sSvpNameLen + 1);

    // 최적화...
    switch(aType)
    {
        case RP_SAVEPOINT_IMPLICIT :
            IDE_ASSERT(aImplicitSvpDepth != SMI_STATEMENT_DEPTH_NULL);

            // Depth 1인 경우, 이전 Implicit Savepoint를 모두 제거
            if(aImplicitSvpDepth == 1)
            {
                removeLastImplicitSvpEntries(&sTransNode->mSvpList);
            }
            break;

        case RP_SAVEPOINT_EXPLICIT :
            // 이전 Implicit Savepoint를 모두 제거
            removeLastImplicitSvpEntries(&sTransNode->mSvpList);
            break;

        case RP_SAVEPOINT_PSM :
            // 이전 Implicit Savepoint를 모두 제거
            removeLastImplicitSvpEntries(&sTransNode->mSvpList);

            // PSM Savepoint는 마지막 1개만 유효하므로, 이전의 PSM Savepoint를 제거
            if(sTransNode->mPSMSvp != NULL)
            {
                IDU_LIST_REMOVE(&sTransNode->mPSMSvp->mNode);
                (void)mSvpPool.memfree(sTransNode->mPSMSvp);
            }
            sTransNode->mPSMSvp = sSvpEntry;
            break;

        default :
            IDE_ASSERT(0);
    }

    IDU_LIST_INIT_OBJ(&(sSvpEntry->mNode), sSvpEntry);
    IDU_LIST_ADD_LAST(&sTransNode->mSvpList, &(sSvpEntry->mNode));
    sNeedFreeSvpEntry = ID_FALSE;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if(sNeedFreeSvpEntry == ID_TRUE)
    {
        (void)mSvpPool.memfree(sSvpEntry);
    }

    IDE_POP();
    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Savepoint Entry List에서 첫 번째 Entry를 반환한다.
 *
 * aTID            - [IN]  트랜잭션 ID
 * aSavepointEntry - [OUT] 반환될 첫 번째 Savepoint Entry
 ******************************************************************************/
void rpdTransTbl::getFirstSvpEntry(smTID                aTID,
                                   rpdSavepointEntry ** aSavepointEntry)
{
    iduListNode     *sNode;
    rpdTransTblNode *sTransNode = &mTransTbl[aTID % mTblSize];

    *aSavepointEntry = NULL;

    if(IDU_LIST_IS_EMPTY(&sTransNode->mSvpList) != ID_TRUE)
    {
        sNode = IDU_LIST_GET_FIRST(&sTransNode->mSvpList);
        *aSavepointEntry = (rpdSavepointEntry *)sNode->mObj;
    }

    return;
}

/*******************************************************************************
 * Description : Savepoint Entry List에서 Savepoint Entry를 제거한다.
 *
 * aSavepointEntry - [IN]  제거할 Savepoint Entry
 ******************************************************************************/
void rpdTransTbl::removeSvpEntry(rpdSavepointEntry * aSavepointEntry)
{
    IDU_LIST_REMOVE(&aSavepointEntry->mNode);
    (void)mSvpPool.memfree(aSavepointEntry);

    return;
}

/*******************************************************************************
 * Description : Savepoint Entry List의 마지막 Implicit Savepoint들을 제거한다.
 *
 * aSvpList   - [IN]  Savepoint Entry List
 ******************************************************************************/
void rpdTransTbl::removeLastImplicitSvpEntries(iduList *aSvpList)
{
    iduListNode       * sNode;
    iduListNode       * sDummy;
    rpdSavepointEntry * sSavepointEntry;

    IDU_LIST_ITERATE_BACK_SAFE(aSvpList, sNode, sDummy)
    {
        sSavepointEntry = (rpdSavepointEntry *)sNode->mObj;

        if(sSavepointEntry->mType != RP_SAVEPOINT_IMPLICIT)
        {
            break;
        }

        IDU_LIST_REMOVE(sNode);
        (void)mSvpPool.memfree(sSavepointEntry);
    }

    return;
}

/*******************************************************************************
 * Description : Savepoint Entry List에 Savepoint Abort를 적용한다.
 *               추가적으로, Partial Rollback된 위치를 반환한다.
 *
 * aTID       - [IN]  트랜잭션 ID
 * aSvpName   - [IN]  Savepoint Abort Name
 * aSN        - [OUT] Partial Rollback된 위치
 ******************************************************************************/
void rpdTransTbl::applySvpAbort(smTID  aTID,
                                SChar *aSvpName,
                                smSN  *aSN)
{
    rpdTransTblNode   *sTransNode = &mTransTbl[aTID % mTblSize];
    iduListNode       *sNode;
    iduListNode       *sDummy;
    rpdSavepointEntry *sSavepointEntry;
    SChar             *sPsmSvpName = NULL;

    sPsmSvpName = smiGetPsmSvpName();
    *aSN        = SM_SN_NULL;

    /* process PSM Savepoint Abort */
    if(idlOS::strcmp(aSvpName, sPsmSvpName) == 0)
    {
        /* PSM Savepoint는 0~1개만 있으므로, List를 수정하지 않는다. */
        *aSN = sTransNode->mPSMSvp->mSN;
        IDE_CONT(SKIP_APPLY);
    }

    /* process Implicit/Explicit Savepoint Abort */
    IDU_LIST_ITERATE_BACK_SAFE(&sTransNode->mSvpList, sNode, sDummy)
    {
        sSavepointEntry = (rpdSavepointEntry *)sNode->mObj;

        if(idlOS::strcmp(sSavepointEntry->mName, aSvpName) == 0)
        {
            /* Current Savepoint's Name is Savepoint Abort's Name */
            *aSN = sSavepointEntry->mSN;
            break;
        }

        if(sSavepointEntry->mType == RP_SAVEPOINT_PSM)
        {
            /* Current Savepoint is PSM Savepoint Set
             *
             *  PSM()
             *  {
             *      SAVEPOINT SP1;
             *      INSERT 1;
             *      ROLLBACK TO SAVEPOINT SP1;
             *      INSERT 1;
             *  }
             *  을 수행하면,
             *      SAVEPOINT SP1;
             *      PSM SAVEPOINT;
             *      INSERT 1; (with Implicit Savepoint depth 1)
             *      ROLLBACK TO SAVEPOINT SP1;  <- POINT 1
             *      INSERT 1; (with Implicit Savepoint depth 1)
             *      ROLLBACK TO IMPLICIT SAVEPOINT DEPTH 1;
             *      ROLLBACK TO PSM SAVEPOINT;  <- POINT 2
             *  와 같이 로그 레코드가 기록된다.
             *  그런데, POINT 1에서 PSM Savepoint를 제거했다면, POINT 2에서 실패한다.
             *  따라서, PSM Savepoint는 제거하지 않는다.
             */
        }
        else
        {
            /* remove Current Savepoint from Savepoint List */
            IDU_LIST_REMOVE(sNode);
            (void)mSvpPool.memfree(sSavepointEntry);
        }
    }

    RP_LABEL(SKIP_APPLY);

    return;
}

IDE_RC rpdTransTbl::initTransPool()
{
    idBool sTransPoolInit = ID_FALSE;
    IDE_TEST(mTransPool.initialize(IDU_MEM_RP_RPD,
                                   (SChar *)"RP_TRANSACTION_POOL",
                                   1,
                                   ID_SIZEOF( smiTransNode ),
                                   mTblSize,
                                   IDU_AUTOFREE_CHUNK_LIMIT,//chunk max(default)
                                   ID_FALSE,                //use mutex(no use)
                                   8,                       //align byte(default)
                                   ID_FALSE,				//ForcePooling 
                                   ID_TRUE,					//GarbageCollection
                                   ID_TRUE,                 /* HWCacheLine */
                                   IDU_MEMPOOL_TYPE_LEGACY  /* mempool type*/) 
              != IDE_SUCCESS);		
    sTransPoolInit = ID_TRUE;
    
    IDU_LIST_INIT( &mFreeTransList );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if(sTransPoolInit == ID_TRUE)
    {
        (void)mTransPool.destroy(ID_FALSE);
    }
    else
    {
        /*nothing to do*/
    }
    return IDE_FAILURE;
}

IDE_RC rpdTransTbl::allocTransNode()
{
    smiTransNode *sTrans = NULL;

    IDU_FIT_POINT( "rpdTransTbl::allocTransNode::alloc::TransPool",
                   rpERR_ABORT_MEMORY_ALLOC,
                   "rpdTransTbl::allocTransNode",
                   "TransPool" );
    IDE_TEST( mTransPool.alloc( (void**)&sTrans ) != IDE_SUCCESS );
    IDE_TEST( sTrans->mSmiTrans.initialize() != IDE_SUCCESS );
    
    IDU_LIST_INIT_OBJ( &( sTrans->mNode ), sTrans );
    IDU_LIST_ADD_LAST( &mFreeTransList, &( sTrans->mNode ) );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( sTrans != NULL )
    {
        IDE_ERRLOG(IDE_RP_0);
        (void)mTransPool.memfree( sTrans );
        sTrans = NULL;
    }

    return IDE_FAILURE;
}

void rpdTransTbl::initFreeTransList()
{
    IDU_LIST_INIT( &mFreeTransList );
}

void rpdTransTbl::removeFreeTransNode( smiTransNode * aTrans )
{
    IDU_LIST_REMOVE( &(aTrans->mNode) );

    if ( aTrans->mSmiTrans.isPrepared() != ID_TRUE )
    {
        /* prepared Trans is Free By another thread */
        if ( aTrans->mSmiTrans.destroy( NULL ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        (void)iduMemMgr::free( aTrans );
        aTrans = NULL;
    }
}

void rpdTransTbl::removeFreeTransList()
{
    smiTransNode * sTrans = NULL;
    iduListNode  * sNode  = NULL;
    iduListNode  * sDummy = NULL;

    IDU_LIST_ITERATE_SAFE( &mFreeTransList, sNode, sDummy )
    {
        sTrans = (smiTransNode *) sNode->mObj;
        removeFreeTransNode( sTrans );
        sTrans = NULL;
    }
}

void rpdTransTbl::destroyTransPool()
{
    smiTransNode * sTrans = NULL;
    iduListNode  * sNode = NULL;
    iduListNode  * sDummy = NULL;
    
    IDU_LIST_ITERATE_SAFE( &mFreeTransList, sNode, sDummy )
    {
        sTrans = (smiTransNode *) sNode->mObj;
        if ( sTrans->mSmiTrans.destroy( NULL ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        IDU_LIST_REMOVE( sNode );
        (void)mTransPool.memfree( sTrans );
        sTrans = NULL;
    }
    
    if( mTransPool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /*nothing to do*/
    }

    return;
}

IDE_RC rpdTransTbl::getTransNode( smiTransNode** aRpdTrans )
{
    smiTransNode * sTrans = NULL;
    iduListNode  * sNode;
    idBool         sIsPoolEmpty = ID_FALSE;

    /*get transaction from transaction pool*/
    if( IDU_LIST_IS_EMPTY( &mFreeTransList ) != ID_TRUE )
    {
        sNode = IDU_LIST_GET_FIRST( &mFreeTransList );
        sTrans = (smiTransNode *)sNode->mObj;
        IDU_LIST_REMOVE( sNode );
    }
    else /*transaction pool is empty*/
    {
        IDU_FIT_POINT( "rpdTransTbl::getTransNode::alloc::TransPool",
                       rpERR_ABORT_MEMORY_ALLOC,
                       "rpdTransTbl::getTransNode",
                       "TransPool" );
        sIsPoolEmpty = ID_TRUE;

        if ( transPoolType() == RPD_TRANSTBL_USING_TRANS_POOL )
        {
            IDE_TEST( mTransPool.alloc( (void**) &sTrans ) != IDE_SUCCESS );
            mOverAllocTransCount++;
        }
        else
        {
            IDE_TEST_RAISE(iduMemMgr::calloc( IDU_MEM_RP_RPD,
                                              1,
                                              ID_SIZEOF(smiTransNode),
                                              (void**)&sTrans,
                                              IDU_MEM_IMMEDIATE )
                           != IDE_SUCCESS, ERR_MEMORY_ALLOC );
        }

        IDE_TEST( sTrans->mSmiTrans.initialize() != IDE_SUCCESS );
        IDU_LIST_INIT_OBJ( &( sTrans->mNode ), sTrans );
    }
    
    *aRpdTrans = sTrans;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdTransTbl::getTransNode",
                                "sTrans"));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsPoolEmpty == ID_TRUE )
    {
        if ( transPoolType() == RPD_TRANSTBL_USING_TRANS_POOL )
        {
            if ( sTrans != NULL )
            {
                (void)mTransPool.memfree( sTrans );
            }
            mOverAllocTransCount--;
        }
        else
        {
            if ( sTrans != NULL )
            {
                (void)iduMemMgr::free( sTrans );
            }
        }
            
        sTrans = NULL;
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdTransTbl::removeTransNode(smiTransNode* aRpdTrans)
{
    if ( mOverAllocTransCount > 0 )
    {
        if ( aRpdTrans->mSmiTrans.destroy( NULL ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        (void)mTransPool.memfree( aRpdTrans );
        mOverAllocTransCount--;
    }
    else
    {
        /*return transaction to transaction pool*/
        IDU_LIST_ADD_LAST( &mFreeTransList, &( aRpdTrans->mNode ) );
    }
    aRpdTrans = NULL;
}

void rpdTransTbl::setMyTID(smTID       aRemoteTID)
{
    UInt          sIndex;

    sIndex = aRemoteTID % mTblSize;
    mTransTbl[sIndex].mMyTID = mTransTbl[sIndex].mTrans.mRpdTrans->mSmiTrans.getTransID();
}

IDE_RC rpdTransTbl::buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                                        void                    * /* aDumpObj */,
                                                        iduFixedTableMemory     * aMemory,
                                                        SChar                   * aRepName,
                                                        UInt                      aParallelID,
                                                        SInt                      aParallelApplyIndex )
{
    UInt                      i = 0;
    UInt                      sTransTblSize = 0;
    rpdTransTblNode         * sTransTblNodeArray = NULL;
    rpdTransTblNodeInfo       sTransTblNodeInfo;

    sTransTblNodeArray = getTransTblNodeArray();

    IDE_TEST_CONT( sTransTblNodeArray == NULL, NORMAL_EXIT );

    sTransTblSize = getTransTblSize();
    for ( i = 0; i < sTransTblSize; i++ )
    {
        if ( isATransNode( &(sTransTblNodeArray[i]) ) == ID_TRUE )
        {
            //sTransTblNodeInfo 세팅
            sTransTblNodeInfo.mRepName    = aRepName;
            sTransTblNodeInfo.mParallelID = aParallelID;
            sTransTblNodeInfo.mParallelApplyIndex = aParallelApplyIndex;

            sTransTblNodeInfo.mMyTID      = sTransTblNodeArray[i].mMyTID;
            sTransTblNodeInfo.mRemoteTID  = sTransTblNodeArray[i].mRemoteTID;
            sTransTblNodeInfo.mBeginFlag  = sTransTblNodeArray[i].mBeginFlag;
            sTransTblNodeInfo.mBeginSN    = sTransTblNodeArray[i].mBeginSN;

            IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                  aMemory,
                                                  (void *) &(sTransTblNodeInfo) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
    }

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdTransTbl::allocConflictResolutionTransNode( smTID aTID )
{
    smiTransNode * sRpdTrans = NULL;
    UInt sIndex = getTransSlotID( aTID );

    IDE_TEST( getTransNode( &sRpdTrans ) != IDE_SUCCESS );

    mTransTbl[sIndex].mTrans.mTransForConflictResolution = sRpdTrans;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

