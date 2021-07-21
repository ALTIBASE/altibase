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
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 *
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id:
 **********************************************************************/

#include <smxLegacyTransMgr.h>

iduMutex smxLegacyTransMgr::mLTLMutex;
iduMutex smxLegacyTransMgr::mLSLMutex;
smuList  smxLegacyTransMgr::mHeadLegacyTrans;
smuList  smxLegacyTransMgr::mHeadLegacyStmt;
UInt     smxLegacyTransMgr::mLegacyTransCnt;
UInt     smxLegacyTransMgr::mLegacyStmtCnt;

IDE_RC smxLegacyTransMgr::initializeStatic()
{
    IDE_TEST(mLTLMutex.initialize( (SChar*)"LEGACY_TRANS_LIST_MUTEX",
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);
    IDE_TEST(mLSLMutex.initialize( (SChar*)"LEGACY_STMT_LIST_MUTEX",
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    SMU_LIST_INIT_BASE( &mHeadLegacyTrans );

    SMU_LIST_INIT_BASE( &mHeadLegacyStmt );

    mLegacyTransCnt = 0;
    mLegacyStmtCnt  = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxLegacyTransMgr::destroyStatic()
{
    smuList * sCurNode;
    smuList * sNxtNode;

    SMU_LIST_ITERATE_SAFE( &mHeadLegacyTrans, sCurNode, sNxtNode )
    {
        IDE_TEST( removeLegacyTrans( sCurNode, SM_NULL_TID  ) != IDE_SUCCESS );
    }

    SMU_LIST_ITERATE_SAFE( &mHeadLegacyStmt, sCurNode, sNxtNode )
    {
        IDE_TEST( removeLegacyStmt( sCurNode, NULL ) != IDE_SUCCESS );
    }

    IDE_TEST( mLTLMutex.destroy() != IDE_SUCCESS );
    IDE_TEST( mLSLMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxLegacyTransMgr::allocAndSetLegacyStmt( void * aLegacyTrans,
                                                 void * aStmtListHead )
{
    smxLegacyStmt * sLegacyStmt;
    smuList       * sNewNode;
    UInt            sState  = 0;

    /* smxLegacyTransMgr_allocAndSetLegacyStmt_malloc_LegacyStmt.tc */
    IDU_FIT_POINT_RAISE( "smxLegacyTransMgr::allocAndSetLegacyStmt::malloc::LegacyStmt", 
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_TRANSACTION_LEGACY_TRANS,
                               ID_SIZEOF(smxLegacyStmt),
                               (void**)&sLegacyStmt) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    /* smxLegacyTransMgr_allocAndSetLegacyStmt_calloc_NewNode.tc */
    IDU_FIT_POINT_RAISE( "smxLegacyTransMgr::allocAndSetLegacyStmt::calloc::NewNode", 
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_SM_TRANSACTION_LEGACY_TRANS,
                               1,
                               ID_SIZEOF(smuList),
                               (void**)&(sNewNode)) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 2;
    sNewNode->mData = sLegacyStmt;

    sLegacyStmt->mLegacyTrans  = (smxLegacyTrans *)aLegacyTrans;
    sLegacyStmt->mStmtListHead = aStmtListHead;

    lockLSL();
    SMU_LIST_ADD_LAST( &mHeadLegacyStmt, sNewNode );
    mLegacyStmtCnt++;
    unlockLSL();

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( iduMemMgr::free( sNewNode ) == IDE_SUCCESS );
            sNewNode = NULL;
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLegacyStmt ) == IDE_SUCCESS );
            sLegacyStmt = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smxLegacyTransMgr::removeLegacyStmt( smuList * aLegacyStmtNode,
                                            void    * aStmtListHead )
{
    smxLegacyStmt * sLegacyStmt;
    smuList       * sCurNode;
    smuList       * sTargetNode = NULL;

    lockLSL();
    if( aLegacyStmtNode == NULL )
    {
        IDE_DASSERT( aStmtListHead != NULL );

        SMU_LIST_ITERATE( &mHeadLegacyStmt, sCurNode )
        {
            sLegacyStmt = (smxLegacyStmt*)sCurNode->mData;

            if( sLegacyStmt->mStmtListHead == aStmtListHead )
            {
                sTargetNode = sCurNode;
                break;
            }
        }
    }
    else
    {
        IDE_DASSERT( aStmtListHead == NULL );
        sTargetNode = aLegacyStmtNode;
    }

    IDE_DASSERT( sTargetNode != NULL );

    if( sTargetNode != NULL )
    {
        SMU_LIST_DELETE( sTargetNode );
        mLegacyStmtCnt--;
    }
    unlockLSL();

    if( sTargetNode != NULL )
    {
        sLegacyStmt = (smxLegacyStmt *)sTargetNode->mData;

        if( sLegacyStmt->mLegacyTrans != NULL )
        {
            IDE_TEST( iduMemMgr::free(sLegacyStmt->mStmtListHead)
                      != IDE_SUCCESS );
            sLegacyStmt->mStmtListHead = NULL;
        }

        IDE_TEST( iduMemMgr::free(sLegacyStmt) != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free(sTargetNode) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxLegacyTransMgr::addLegacyTrans( smxTrans * aSmxTrans,
                                          smLSN      aCommitEndLSN,
                                          void    ** aLegacyTrans,
                                          idBool     aIsCommit )
                                          
{
    smxLegacyTrans    * sLegacyTrans;
    smuList           * sNewNode;
    UInt                sState = 0;
    SChar               sMutexName[128];

    /* smxLegacyTransMgr_addLegacyTrans_malloc_LegacyStmt.tc */
    IDU_FIT_POINT_RAISE("smxLegacyTransMgr::addLegacyTrans::malloc::LegacyStmt", insufficient_memory);
    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_TRANSACTION_LEGACY_TRANS,
                                ID_SIZEOF(smxLegacyTrans),
                                (void**)&sLegacyTrans) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    /* smxLegacyTransMgr_addLegacyTrans_calloc_NewNode.tc */
    IDU_FIT_POINT_RAISE("smxLegacyTransMgr::addLegacyTrans::calloc::NewNode", insufficient_memory);
    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_SM_TRANSACTION_LEGACY_TRANS,
                                     1,
                                     ID_SIZEOF(smuList),
                                     (void**)&(sNewNode))
                   != IDE_SUCCESS, insufficient_memory );
    sState = 2;
    sNewNode->mData = sLegacyTrans;

    sLegacyTrans->mTransID       = aSmxTrans->mTransID;
    sLegacyTrans->mCommitEndLSN  = aCommitEndLSN;
    sLegacyTrans->mCommitSCN     = aSmxTrans->mCommitSCN;
    sLegacyTrans->mMinMemViewSCN = aSmxTrans->mMinMemViewSCN;
    sLegacyTrans->mMinDskViewSCN = aSmxTrans->mMinDskViewSCN;
    sLegacyTrans->mFstDskViewSCN = aSmxTrans->mFstDskViewSCN;

    /* PROJ-2694 Fetch Across Rollback 
     * rollback에서 호출될 경우에는 OID list를 이미 처리 한 다음이므로 OID list를 연결할 필요가 없다. */
    if( aIsCommit == ID_TRUE )
    {
        sLegacyTrans->mMadeType  = 'C';
        sLegacyTrans->mOIDList   = aSmxTrans->mOIDList;
        ((smxOIDList*)sLegacyTrans->mOIDList)->mCacheOIDNode4Insert = NULL;
    }
    else
    {
        sLegacyTrans->mMadeType  = 'R';
        sLegacyTrans->mOIDList = NULL;
    }


    idlOS::snprintf( sMutexName,
                     128,
                     "LEGACY_TRANS_CHECK_MUTEX_%"ID_UINT32_FMT,
                     (UInt)aSmxTrans->mTransID );

    IDE_TEST( sLegacyTrans->mWaitForNoAccessAftDropTbl.initialize( sMutexName,
                                                                   IDU_MUTEX_KIND_NATIVE,
                                                                   IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    if( smuProperty::isTrcLogLegacyTxInfo() == ID_TRUE )
    {
        /* --------------------------------------------------------------------
         * BUG-38515 __SM_SKIP_CHECKSCN_IN_STARTUP 히든 프로퍼티를 사용시 분석을 돕기 위해
         * Legacy Tx에 관련하여 addLegacyTrans와 removeLegacyTrans 함수에서 legacy Tx가 
         * 추가/제거 될 때마다 trc 로그를 남긴다. 
         * ----------------------------------------------------------------- */
        ideLog::log( IDE_SM_0, 
                     "AddLegacyTrans TID=%"ID_UINT32_FMT", CommitSCN=%"ID_UINT64_FMT
                     ", CommitEndLSN=[%"ID_UINT32_FMT",%"ID_UINT32_FMT"]",
                     sLegacyTrans->mTransID,
                     SM_SCN_TO_LONG( sLegacyTrans->mCommitSCN ),
                     sLegacyTrans->mCommitEndLSN.mFileNo,
                     sLegacyTrans->mCommitEndLSN.mOffset );
    }
    else
    {
        /* do nothing... */
    }

    lockLTL();
    SMU_LIST_ADD_LAST( &mHeadLegacyTrans, sNewNode );
    mLegacyTransCnt++;
    unlockLTL();

    *aLegacyTrans = (void*)sLegacyTrans;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( iduMemMgr::free( sNewNode ) == IDE_SUCCESS );
            sNewNode = NULL;
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLegacyTrans ) == IDE_SUCCESS );
            sLegacyTrans = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smxLegacyTransMgr::removeLegacyTrans( smuList * aLegacyTransNode,
                                             smTID     aTID )
{
    smuList           * sCurNode;
    smuList           * sTargetNode     = NULL;
    smxOIDList        * sOIDList;
    smxLegacyTrans    * sLegacyTrans    = NULL;
    UInt                sState          = 0;

    IDU_FIT_POINT( "smxLegacyTransMgr::removeLegacyTrans::begin::sleep" );

    IDE_DASSERT( ( aLegacyTransNode != NULL ) ||
                 ( aTID             != SM_NULL_TID ) );

    /* 1.1 Lock Legacy Transaction List
     * 1.2 Remove Legacy Transaction from Legacy Transaction List.
     * 1.3 Unlock Legacy Transaction List
     * 2.1 Process OIDList pending jobs.
     * 2.2 Change TSSlot State to COMMIT.
     * 3. Destroy Legacy Trans Node */

    /* BUG-38622 Remove legacy trans node after processOIDList.
     *
     * 1. Lock
     * 2. Search target node
     * 3. Unlock    -- 추후 튜닝
     * 4. Process OIDList pending jobs.
     * 5. Lock      -- 추후 튜닝
     * 6. Remove Legacy Transaction From Legacy Transaction List.
     * 7. Unlock */

    lockLTL();
    sState = 1;

    if( aLegacyTransNode == NULL )
    {
        SMU_LIST_ITERATE( &mHeadLegacyTrans, sCurNode )
        {
            sLegacyTrans = (smxLegacyTrans *)sCurNode->mData;

            if( sLegacyTrans->mTransID == aTID )
            {
                sTargetNode = sCurNode;
                break;
            }
            else
            {
                /* do nothing */
            }
        } // end of loop
    }
    else
    {
        sTargetNode = aLegacyTransNode;
    }

    /* sState = 0;
     * unlockLTL(); -- 추후 튜닝 */

    if( sTargetNode != NULL )
    {
        sLegacyTrans = (smxLegacyTrans*)sTargetNode->mData;
        sOIDList     = (smxOIDList*)sLegacyTrans->mOIDList;

        if( sOIDList != NULL )
        {
            if ( sLegacyTrans->mMadeType == 'C' )
            {
                /* BUG-42760 */
                IDE_ASSERT( lockWaitForNoAccessAftDropTbl( sLegacyTrans )
                            == IDE_SUCCESS );

                IDE_TEST( sOIDList->processOIDList4LegacyTx( &(sLegacyTrans->mCommitEndLSN),
                                                             sLegacyTrans->mCommitSCN )
                          != IDE_SUCCESS );

                IDE_ASSERT( unlockWaitForNoAccessAftDropTbl( sLegacyTrans )
                            == IDE_SUCCESS );

                IDE_TEST( sOIDList->destroy() != IDE_SUCCESS );
                IDE_TEST( iduMemMgr::free(sOIDList) != IDE_SUCCESS );
            }
            else
            {
                /* PROJ-2694 Fetch Across Rollback
                 * legacy Tx가 rollback일 경우는 OID list가 모두 정리된 상태로 연결되므로 
                 * OID list를 정리할 필요 없다. */
            }
        }
        else
        {
            /* do nothing */
        }

        if( smuProperty::isTrcLogLegacyTxInfo() == ID_TRUE )
        {
            /* --------------------------------------------------------------------
             * BUG-38515 __SM_SKIP_CHECKSCN_IN_STARTUP 히든 프로퍼티를 사용시 분석을 돕기 위해
             * Legacy Tx에 관련하여 addLegacyTrans와 removeLegacyTrans 함수에서 legacy Tx>가 
             * 추가/제거 될 때마다 trc 로그를 남긴다. 
             * ----------------------------------------------------------------- */
            ideLog::log( IDE_SM_0, "RemoveLegacyTrans TID=%"ID_UINT32_FMT, sLegacyTrans->mTransID );
        }
        else
        {
            /* do nothing... */
        }
    }
    else
    {
        /* do nothing */
    }

    /* lockLTL();   -- 추후 튜닝
     * sState = 1; */

    if( sTargetNode != NULL )
    {
        SMU_LIST_DELETE( sTargetNode );
        mLegacyTransCnt--;

        IDE_TEST( iduMemMgr::free( sTargetNode  ) != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    if( sLegacyTrans != NULL )
    {
        IDE_TEST( sLegacyTrans->mWaitForNoAccessAftDropTbl.destroy() != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free( sLegacyTrans ) != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    sState = 0;
    unlockLTL();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        unlockLTL();
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

smSCN smxLegacyTransMgr::getCommitSCN( smTID aTID )
{
    smuList        * sCurNode;
    smxLegacyTrans * sLegacyTrans;
    smSCN            sSCN;

    SM_MAX_SCN( &sSCN );

    lockLTL();
    SMU_LIST_ITERATE( &mHeadLegacyTrans, sCurNode )
    {
        sLegacyTrans = (smxLegacyTrans*)sCurNode->mData;

        if( sLegacyTrans->mTransID == aTID )
        {
            SM_SET_SCN( &sSCN, &(sLegacyTrans->mCommitSCN) );
            break;
        }
    }
    unlockLTL();

    return sSCN;
}

/* BUG-42760 */
void smxLegacyTransMgr::waitForNoAccessAftDropTbl()
{
    smuList        * sCurNode = NULL;
    smxLegacyTrans * sLegacyTrans = NULL;

    lockLTL();

    SMU_LIST_ITERATE( &mHeadLegacyTrans, sCurNode )
    {
        sLegacyTrans = (smxLegacyTrans*)sCurNode->mData;

        IDE_ASSERT( lockWaitForNoAccessAftDropTbl( sLegacyTrans )
                    == IDE_SUCCESS );

        IDE_ASSERT( unlockWaitForNoAccessAftDropTbl( sLegacyTrans )
                    == IDE_SUCCESS );
    }

    unlockLTL();
}
