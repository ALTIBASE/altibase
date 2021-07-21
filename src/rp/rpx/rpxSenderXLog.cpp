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
 * $Id: rpxSenderXLog.cpp 91226 2021-07-14 08:01:24Z lswhh $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>
#include <smErrorCode.h>
#include <rpDef.h>
#include <rpcManager.h>
#include <rpxSender.h>

#include <rpxReplicator.h>

smSN rpxSender::getNextRestartSN()
{
    smSN sRestartSN = SM_SN_NULL;
    UInt i;
    UInt sChildCnt = RPU_REPLICATION_EAGER_PARALLEL_FACTOR - 1;
    smSN sTmpSN = SM_SN_NULL;

    mReplicator.getMinTransFirstSN( &sRestartSN );
    if(sRestartSN == SM_SN_NULL)    // Transaction Table이 비었을 경우
    {
        // mXSN을 사용하면, Active Transaction이 없을 때 Restart SN이 매우 빈번하게 갱신된다.
        // (mXSN 증가 -> mXSN 전송 -> ACK 수신 -> Restart SN 갱신 -> mXSN 증가 -> ...)
        sRestartSN = mCommitXSN;
    }
    if(isParallelParent() == ID_TRUE)
    {
        if(mChildArray != NULL)
        {
            for(i=0; i < sChildCnt; i++)
            {
                sTmpSN = mChildArray[i].getNextRestartSN();
                if(sTmpSN < sRestartSN)
                {
                    sRestartSN = sTmpSN;
                }
            }
        }
    }
    return sRestartSN;
}

void rpxSender::setCommitXSNForFileEndLog(smSN aFileEndSN)
{
    UInt i;
    UInt sChildCnt = RPU_REPLICATION_EAGER_PARALLEL_FACTOR - 1;

    if(mReplicator.isExistActiveTrans() != ID_TRUE)    // Transaction Table이 비었을 경우
    {
        mCommitXSN = aFileEndSN;
    }
    if(isParallelParent() == ID_TRUE)
    {
        if(mChildArray != NULL)
        {
            for(i=0; i < sChildCnt; i++)
            {
                mChildArray[i].setCommitXSNForFileEndLog(aFileEndSN);
            }
        }
    }

    return ;
}

//===================================================================
//
// Name:          addXLogKeepAlive
//
// Return Value:
//
// Argument:
//
// Called By:     senderSleep()
//
// Description:
//
//===================================================================

IDE_RC rpxSender::addXLogKeepAlive()
{
    IDE_RC sRC        = IDE_SUCCESS;
    smSN   sRestartSN = SM_SN_NULL;
    idBool sNeedMessengerLock = ID_FALSE;
    
    // BUG-17748
    sRestartSN = getNextRestartSN();

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    /* PROJ-2453 */
    if ( mMeta.getReplMode() == RP_EAGER_MODE )
    {
        sNeedMessengerLock = ID_TRUE;
    }
    else
    {
        sNeedMessengerLock = ID_FALSE;
    }
    
    sRC = mMessenger.sendXLogKeepAlive( mXSN,
                                        mReplicator.getFlushSN(),
                                        sRestartSN,
                                        sNeedMessengerLock );
    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Network를 종료하지 않고 Handshake를 다시 수행함을
 *              Receiver에 알린다.
 *
 ***********************************************************************/
IDE_RC rpxSender::addXLogHandshake( smTID aTID )
{
    IDE_RC sRC;
    idBool sNeedMessengerLock = ID_FALSE;
    
    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    /* PROJ-2453 */
    if ( mMeta.getReplMode() == RP_EAGER_MODE )
    {
        sNeedMessengerLock = ID_TRUE;
    }
    else
    {
        sNeedMessengerLock = ID_FALSE;
    }
    
    sRC = mMessenger.sendXLogHandshake( aTID,
                                        mXSN,
                                        mReplicator.getFlushSN(),
                                        sNeedMessengerLock );
    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Incremental Sync Primary Key Begin를 전송하여,
 *               Incremental Sync를 위한 Primary Key를 전송할 것임을 알린다.
 *               (Failback Slave -> Failback Master)
 *
 ******************************************************************************/
IDE_RC rpxSender::addXLogSyncPKBegin()
{
    IDE_RC sRC = IDE_SUCCESS;

    IDE_DASSERT( ( mStatus == RP_SENDER_FAILBACK_SLAVE ) || 
                 ( mCurrentType == RP_XLOGFILE_FAILBACK_SLAVE ) );

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    sRC = mMessenger.sendXLogSyncPKBegin();

    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Incremental Sync Primary Key End를 전송하여,
 *               Incremental Sync를 위한 Primary Key가 더 이상 없음을 알린다.
 *               (Failback Slave -> Failback Master)
 *
 ******************************************************************************/
IDE_RC rpxSender::addXLogSyncPKEnd()
{
    IDE_RC sRC = IDE_SUCCESS;

    IDE_DASSERT( ( mStatus == RP_SENDER_FAILBACK_SLAVE ) || 
                 ( mCurrentType == RP_XLOGFILE_FAILBACK_SLAVE ) );

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    sRC = mMessenger.sendXLogSyncPKEnd();

    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Failback End를 전송하여, Failback이 완료되었음을 알린다.
 *               (Failback Master -> Failback Slave)
 *
 ******************************************************************************/
IDE_RC rpxSender::addXLogFailbackEnd()
{
    IDE_RC sRC = IDE_SUCCESS;

    IDE_DASSERT( ( mStatus == RP_SENDER_FAILBACK_MASTER ) || 
                 ( mCurrentType == RP_XLOGFILE_FAILBACK_MASTER ) );

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    sRC = mMessenger.sendXLogFailbackEnd();

    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxSender::addXLogAckOnDML()
{
    return mMessenger.sendAckOnDML();
}

/*******************************************************************************
 * Description : Incremental Sync Row을 전송한다.
 *               (Failback Master -> Failback Slave)
 *
 ******************************************************************************/
IDE_RC rpxSender::addXLogSyncRow(rpdSyncPKEntry *aSyncPKEntry)
{
    IDE_RC       sRC       = IDE_SUCCESS;
    rpdMetaItem *sMetaItem = NULL;

    IDE_DASSERT( ( mStatus == RP_SENDER_FAILBACK_MASTER ) || 
                 ( mCurrentType == RP_XLOGFILE_FAILBACK_MASTER ) );
    
    IDE_DASSERT(aSyncPKEntry->mType == RP_SYNC_PK);

    // check existence of TABLE
    IDE_TEST(mMeta.searchTable(&sMetaItem, aSyncPKEntry->mTableOID)
             != IDE_SUCCESS);

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    // Delete를 전송한다.
    sRC = mMessenger.sendXLogDelete( aSyncPKEntry->mTableOID,
                                     aSyncPKEntry->mPKColCnt,
                                     aSyncPKEntry->mPKCols,
                                     mSyncPKMtdValueLen );
    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }
    // Row가 있으면, Insert(LOB 포함)를 전송한다.
    sRC = syncRow(sMetaItem, aSyncPKEntry->mPKCols);

    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Incremental Sync Commit을 전송한다.
 *               (Failback Master -> Failback Slave)
 *
 ******************************************************************************/
IDE_RC rpxSender::addXLogSyncCommit()
{
    IDE_RC sRC = IDE_SUCCESS;

    IDE_DASSERT( ( mStatus == RP_SENDER_FAILBACK_MASTER ) || 
                 ( mCurrentType == RP_XLOGFILE_FAILBACK_MASTER ) );
 
    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    sRC = mMessenger.sendXLogTrCommit();

    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Incremental Sync Abort를 전송한다.
 *               (Failback Master -> Failback Slave)
 *
 ******************************************************************************/
IDE_RC rpxSender::addXLogSyncAbort()
{
    IDE_RC sRC = IDE_SUCCESS;

    IDE_DASSERT( ( mStatus == RP_SENDER_FAILBACK_MASTER ) || 
                 ( mCurrentType == RP_XLOGFILE_FAILBACK_MASTER ) );

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    sRC = mMessenger.sendXLogTrAbort();

    if(sRC != IDE_SUCCESS)
    {
        IDE_TEST_RAISE((ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED) ||
                       (ideGetErrorCode() == cmERR_ABORT_SEND_ERROR) ||
                       ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                       ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK);
        IDE_RAISE(ERR_ETC);
    }

    IDE_TEST_RAISE( checkHBTFault() != IDE_SUCCESS, NORMAL_EXIT);

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_ETC);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Row를 Select하고, 있으면 Insert(LOB 포함)로 전송한다.
 *
 **********************************************************************/
IDE_RC rpxSender::syncRow(rpdMetaItem *aMetaItem,
                          smiValue    *aPKCols)
{
    smiTrans             sTrans;
    SInt                 sTxStage            = 0;
    idBool               sIsTxBegin          = ID_FALSE;
    smiStatement       * spRootStmt          = NULL;
    UInt                 sFlag               = 0;
    smiStatement         sSmiStmt;
    smiRange             sKeyRange;
    smiFetchColumnList * sFetchColumnList    = NULL;

    void               * sRow                = NULL;
    SChar              * sRealRow            = NULL;
    scGRID               sRid;
    void               * sTable              = NULL;
    const void         * sIndex;
    smSCN                sSCN;
    smiTableCursor       sCursor;
    smiCursorProperties  sProperty;
    UInt                 sRowSize            = 0;
    UInt                 sColCount;
    mtcColumn          * sMtcCol             = NULL;

    // PROJ-1583 large geometry
    UInt                 i                   = 0;
    UInt                 sVariableRecordSize = 0;
    UInt                 sVarColumnOffset    = 0;
    mtcColumn          * sColumn             = NULL;
    SChar              * sColumnBuffer       = NULL;
    idBool               sIsExistLobColumn   = ID_FALSE;

    IDE_ASSERT(aMetaItem != NULL);
    IDE_ASSERT(aPKCols != NULL);

    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
    sTxStage = 1;

    sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = (sFlag & ~SMI_TRANSACTION_MASK) | SMI_TRANSACTION_NORMAL;
    sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
    sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

    if ( mCurrentType == RP_XLOGFILE_FAILBACK_MASTER )
    {
        sFlag = (sFlag & ~SMI_TRANS_GCTX_MASK) | SMI_TRANS_GCTX_ON;

        IDE_TEST(sTrans.begin(&spRootStmt, NULL, sFlag, SMX_NOT_REPL_TX_ID)
                 != IDE_SUCCESS);

        sTrans.setIndoubtFetchTimeout(1);
        sTrans.setIndoubtFetchMethod(1);
    }
    else
    {
        IDE_TEST(sTrans.begin(&spRootStmt, NULL, sFlag, SMX_NOT_REPL_TX_ID )
                 != IDE_SUCCESS);
    }
        
    sIsTxBegin = ID_TRUE;
    sTxStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              spRootStmt,
                              SMI_STATEMENT_NORMAL |
                              SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sTxStage = 3;


    // Table을 접근하기 전에 IS LOCK을 잡는다.
    IDE_TEST( aMetaItem->lockReplItem( &sTrans,
                                       &sSmiStmt,
                                       SMI_TBSLV_DDL_DML,
                                       SMI_TABLE_LOCK_IS,
                                       (ULong)RPU_REPLICATION_SYNC_LOCK_TIMEOUT * 1000000 )
              != IDE_SUCCESS );

    sTable = (void *)smiGetTable( (smOID)aMetaItem->mItem.mTableOID );
    sSCN   = smiGetRowSCN(sTable);

    // Table OID와 PK로 Key Range를 얻는다.
    IDE_TEST( getKeyRange( (smOID)aMetaItem->mItem.mTableOID,
                           aPKCols,
                           &sKeyRange,
                           mRangeColumn )
              != IDE_SUCCESS);
    sIndex = smiGetTablePrimaryKeyIndex( sTable );

    SMI_CURSOR_PROP_INIT(&sProperty, NULL, sIndex);

    sColCount = smiGetTableColumnCount(sTable);
    if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
       == SMI_TABLE_DISK)
    {
        /* PROJ-1705 Fetch Column List를 위한 메모리 할당 */
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPX_SENDER,
                                         ID_SIZEOF(smiFetchColumnList) * sColCount,
                                         (void **)&sFetchColumnList,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_MEMORY_ALLOC_FETCH_COLUMN_LIST);

        // PROJ-1705 Fetch Column List 구성
        IDE_TEST(makeFetchColumnList((smOID)aMetaItem->mItem.mTableOID,
                                     sFetchColumnList)
                 != IDE_SUCCESS);
    }

    // smiCursorProperties에 fetch Column List 정보 설정
    sProperty.mFetchColumnList = sFetchColumnList;

    sCursor.initialize();

    IDE_TEST(sCursor.open(&sSmiStmt,
                          sTable,
                          sIndex, //fix BUG-9946
                          sSCN,
                          NULL,
                          &sKeyRange,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          SMI_LOCK_READ|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                          SMI_SELECT_CURSOR,
                          &sProperty)
             != IDE_SUCCESS);
    sTxStage = 4;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
       == SMI_TABLE_DISK)
    {
        /* 메모리 할당을 위해서, 현재 읽어야 할 Row의 size를 먼저 얻어와야 함 */
        IDE_TEST(qciMisc::getDiskRowSize(sTable,
                                         &sRowSize)
                 != IDE_SUCCESS);

        /* Row를 저장할 Memory를 할당해야 함 */
        IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPX_SENDER,
                                         1,
                                         sRowSize,
                                         (void **)&sRealRow,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_MEMORY_ALLOC_REAL_ROW);

        sRow = (void *)sRealRow;
    }

    /* mtcColumn을 위한 Memory를 할당해야 함 */
    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPX_SENDER,
                                     sColCount,
                                     ID_SIZEOF(mtcColumn),
                                     (void **)&sMtcCol,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_MTC_COL);

    /* mtcColumn 정보의 copy본을 생성 */
    IDE_TEST(qciMisc::copyMtcColumns( sTable, sMtcCol ) != IDE_SUCCESS);

    if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
       == SMI_TABLE_MEMORY)
    {
        //PROJ-1583 large geometry
        for(i = 0; i < sColCount; i++)
        {
            sColumn = sMtcCol + i;
            // To fix BUG-24356
            // geometry에 대해서만 buffer할당
            if( ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                   == SMI_COLUMN_TYPE_VARIABLE_LARGE ) &&
                (sColumn->module->id == MTD_GEOMETRY_ID) )
            {
                sVariableRecordSize = sVariableRecordSize +
                    smiGetVarColumnBufHeadSize(&sColumn->column) +       // PROJ-1705
                    sColumn->column.size;
                sVariableRecordSize = idlOS::align(sVariableRecordSize, 8);
            }
        }

        if(sVariableRecordSize > 0)
        {
            IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPX_SENDER,
                                             1,
                                             sVariableRecordSize,
                                             (void **)&sColumnBuffer,
                                             IDU_MEM_IMMEDIATE)
                           != IDE_SUCCESS, ERR_MEMORY_ALLOC_COLUMN_BUFFER);

            sVarColumnOffset = 0;

            for(i = 0; i < sColCount; i++)
            {
                sColumn = sMtcCol + i;

                // To fix BUG-24356
                // geometry에 대해서만 value buffer할당
                if ( ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                       == SMI_COLUMN_TYPE_VARIABLE_LARGE ) &&
                     (sColumn->module->id == MTD_GEOMETRY_ID) )
                {
                    sColumn->column.value =
                        (void *)(sColumnBuffer + sVarColumnOffset);
                    sVarColumnOffset = sVarColumnOffset +
                        smiGetVarColumnBufHeadSize(&sColumn->column) +   // PROJ-1705
                        sColumn->column.size;
                    sVarColumnOffset = idlOS::align(sVarColumnOffset, 8);
                }
            }
        }
    }

    for ( i = 0; i < sColCount; i++ )
    {
        sColumn = sMtcCol + i;

        if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_LOB )
        {
            sIsExistLobColumn = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid, SMI_FIND_NEXT)
             != IDE_SUCCESS);
    if(sRow != NULL)
    {
        IDE_TEST( mMessenger.sendXLogInsert( RP_TID_NULL,
                                             (SChar *)sRow,
                                             sMtcCol,
                                             sColCount,
                                             (smOID)aMetaItem->mItem.mTableOID )
                  != IDE_SUCCESS );

        if ( sIsExistLobColumn == ID_TRUE )
        {
            IDE_TEST( mMessenger.sendXLogLob( RP_TID_NULL,
                                              (SChar *)sRow,
                                              sRid,
                                              sMtcCol,
                                              (smOID)aMetaItem->mItem.mTableOID,
                                              &sCursor )
                      != IDE_SUCCESS );
        }
    }

    IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid, SMI_FIND_NEXT)
             != IDE_SUCCESS);
    IDE_TEST_RAISE(sRow != NULL, ERR_READ_MORE_THAN_ONE_ROW);

    if(sMtcCol != NULL)
    {
        (void)iduMemMgr::free(sMtcCol);
        sMtcCol = NULL;
    }

    if(sRealRow != NULL)
    {
        (void)iduMemMgr::free(sRealRow);
        sRealRow = NULL;
    }

    //PROJ-1586 large geometry
    if(sColumnBuffer != NULL)
    {
        (void)iduMemMgr::free(sColumnBuffer);
        sColumnBuffer = NULL;
    }

    sTxStage = 3;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sTxStage = 2;

    if(sFetchColumnList != NULL)
    {
        (void)iduMemMgr::free(sFetchColumnList);
        sFetchColumnList = NULL;
    }

    sTxStage = 1;
    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sIsTxBegin = ID_FALSE;

    sTxStage = 0;
    IDE_TEST(sTrans.destroy( NULL ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_READ_MORE_THAN_ONE_ROW);
    {
        IDE_SET(ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                 "[rpxSender::syncRow] err_read_more_than_one_row"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_FETCH_COLUMN_LIST);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxSender::syncRow",
                                "sFetchColumnList"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_REAL_ROW);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxSender::syncRow",
                                "sRealRow"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_MTC_COL);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxSender::syncRow",
                                "sMtcCol"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_COLUMN_BUFFER);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxSender::syncRow",
                                "sColumnBuffer"));
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if(sMtcCol != NULL)
    {
        (void)iduMemMgr::free(sMtcCol);
    }

    if(sRealRow != NULL)
    {
        (void)iduMemMgr::free(sRealRow);
    }

    //PROJ-1586 large geometry
    if(sColumnBuffer != NULL)
    {
        (void)iduMemMgr::free(sColumnBuffer);
    }

    if(sFetchColumnList != NULL)
    {
        (void)iduMemMgr::free(sFetchColumnList);
    }

    switch(sTxStage)
    {
        case 4 :
            (void)sCursor.close();
        case 3 :
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
            sIsTxBegin = ID_FALSE;
        case 1:
            if(sIsTxBegin == ID_TRUE)
            {
                IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();
    return IDE_FAILURE;
}

/*
 *
 */
void rpxSender::setNetworkErrorAndDeactivate( void )
{
    IDE_ERRLOG( IDE_RP_0 );
    IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    
    mRetryError = ID_TRUE;
    mSenderInfo->checkAndRunDeactivate(); //isDisconnect()
}

void rpxSender::setRestartErrorAndDeactivate( void )
{
    mRetryError = ID_TRUE;
    mSenderInfo->checkAndRunDeactivate(); 
}
/*
 *
 */
IDE_RC rpxSender::checkHBTFault( void )
{
    IDE_TEST( mRetryError == ID_TRUE );

    // PROJ-1537
    if ( ( mMeta.mReplication.mRole != RP_ROLE_ANALYSIS ) &&
         ( mMeta.mReplication.mRole != RP_ROLE_ANALYSIS_PROPAGATION ) &&
         ( mCurrentType != RP_OFFLINE ) &&  //PROJ-1915
         ( mSocketType != RP_SOCKET_TYPE_IB ) )
    {
        IDE_TEST_RAISE( rpcHBT::checkFault( mRsc ) == ID_TRUE, ERR_NETWORK );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) );

        mRetryError = ID_TRUE;
        mSenderInfo->checkAndRunDeactivate(); //isDisconnect()
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
