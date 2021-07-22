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
 * $Id: rpdLogAnalyzer.cpp 90444 2021-04-02 10:15:58Z minku.kang $
 **********************************************************************/


#include <idl.h>
#include <idErrorCode.h>
#include <ideErrorMgr.h>

#include <smi.h>

#include <qcuError.h>

#include <qci.h>
#include <rp.h>
#include <rpDef.h>
#include <rpdLogAnalyzer.h>

rpdAnalyzeLogFunc rpdLogAnalyzer::mAnalyzeFunc[] =
{
    NULL,                             // null
    anlzInsMem,                       // MTBL insert
    anlzUdtMem,                       // MTBL update
    anlzDelMem,                       // MTBL delete
    /* PROJ-1705 [RP] */
    anlzPKDisk,                       // analyze PK info
    anlzRedoInsertDisk,
    anlzRedoDeleteDisk,
    anlzRedoUpdateDisk,
    anlzRedoUpdateInsertRowPieceDisk,
    anlzNA,                           //anlzRedoUpdateDeleteRowPieceDisk
    anlzRedoUpdateOverwriteDisk,
    anlzNA,                           //anlzRedoUpdateDeleteFirstColumnDisk
    anlzNA,                           //anlzUndoInsertDisk
    /* TASK-5030 */
    anlzUndoDeleteDisk,                //anlzUndoDeleteDisk
    anlzUndoUpdateDisk,
    anlzNA,                           //anlzUndoUpdateInsertRowPieceDisk
    anlzUndoUpdateDeleteRowPieceDisk,
    anlzUndoUpdateOverwriteDisk,
    anlzUndoUpdateDeleteFirstColumnDisk,
    anlzWriteDskLobPiece,             // DTBL Write LOB Piece
    anlzLobCurOpenMem,                // LOB Cursor Open
    anlzLobCurOpenDisk,               // LOB Cursor Open
    anlzLobCurClose,                  // LOB Cursor Close
    anlzLobPrepare4Write,             // LOB Prepare for Write
    anlzLobFinish2Write,              // LOB Finish to Write
    anlzLobPartialWriteMem,           // MTBL LOB Partial Write
    anlzLobPartialWriteDsk,           // DTBL LOB Partial Write
    anlzLobTrim,                      // LOB Trim

    NULL
};

IDE_RC rpdLogAnalyzer::initialize(iduMemAllocator * aAllocator, iduMemPool *aChainedValuePool)
{
    IDE_ASSERT(aChainedValuePool != NULL);
    mChainedValuePool = aChainedValuePool;

    /* 최초 메모리를 할당했을 때에 초기화하는 변수 */
    idlOS::memset(mPKCIDs, 0, ID_SIZEOF(UInt) * QCI_MAX_KEY_COLUMN_COUNT);
    idlOS::memset(mPKCols, 0, ID_SIZEOF(smiValue) * QCI_MAX_KEY_COLUMN_COUNT);

    /*
     * PROJ-1705
     * 로그 분석 시, 컬럼이 CID순서로 분석되지 않으므로,
     * 분석이 끝난 후, 한꺼번에 정렬하는데, 초기값을 0으로 주면, CID가 0인 것과
     * 섞이게 되므로, 초기값을 UINT_MAX로 한다.
     */
    idlOS::memset(mCIDs, ID_UINT_MAX, ID_SIZEOF(UInt) * QCI_MAX_COLUMN_COUNT);

    idlOS::memset(mBCols, 0, ID_SIZEOF(smiValue) * QCI_MAX_COLUMN_COUNT);
    idlOS::memset(mACols, 0, ID_SIZEOF(smiValue) * QCI_MAX_COLUMN_COUNT);

    // PROJ-1705
    // smiChainedValue의 멤버변수인 AllocMethod와 Link값을 모두 0으로
    // 초기화 하여도 무방하다.
    idlOS::memset(mBChainedCols, 0, ID_SIZEOF(smiChainedValue) * QCI_MAX_COLUMN_COUNT);

    // PROJ-1705
    idlOS::memset(mPKMtdValueLen, 0, ID_SIZEOF(rpValueLen) * QCI_MAX_KEY_COLUMN_COUNT);
    idlOS::memset(mBMtdValueLen, 0, ID_SIZEOF(rpValueLen) * QCI_MAX_COLUMN_COUNT);
    idlOS::memset(mAMtdValueLen, 0, ID_SIZEOF(rpValueLen) * QCI_MAX_COLUMN_COUNT);

    /* Transaction 내에서 XLog를 분석하기 전에 초기화하는 변수 */
    mType               = RP_X_NONE;
    mPKColCnt           = 0;
    mUndoAnalyzedColCnt = 0;
    mRedoAnalyzedColCnt = 0;
    mAllocator = aAllocator;
    IDU_LIST_INIT( &mDictionaryValueList );

    resetVariables(ID_FALSE, 0);

    return IDE_SUCCESS;
}

void rpdLogAnalyzer::destroy()
{
    /* PROJ-2397 free List */
    destroyDictValueList();

    /* XLog 분석이 실패한 경우, 메모리를 해제한다. */
    if(getNeedFree() == ID_TRUE)
    {
        freeColumnValue(ID_TRUE);
        setNeedFree(ID_FALSE);
    }
}

void rpdLogAnalyzer::freeColumnValue(idBool aIsAborted)
{
    UInt              i;
    idBool            sFreeNode;
    smiChainedValue * sChainedCols = NULL;
    smiChainedValue * sNextChainedCols = NULL;
    UShort            sColumnCount = 0;

    /* 분석을 완료하지 않은 경우, 전체 Before/After Image를 확인한다. */
    if(aIsAborted == ID_TRUE)
    {
        for(i = 0; i < QCI_MAX_COLUMN_COUNT; i++)
        {
            mCIDs[i] = i;
        }

        mRedoAnalyzedColCnt = QCI_MAX_COLUMN_COUNT;
    }

    /* Out-Mode LOB Update 시 해당 LOB의 Before는 분석하지 않기 때문에,
     * mUndoAnalyzedColCnt < mRedoAnalyzedColCnt 상황이 발생할 수 있다.
     * 그런데, mCIDs를 참고하여 메모리를 해제한다.
     * 따라서, Before Image 메모리를 해제할 때도 mRedoAnalyzedColCnt를 사용한다.
     */
    if ( mRedoAnalyzedColCnt > mUndoAnalyzedColCnt )
    {
        sColumnCount = mRedoAnalyzedColCnt;
    }
    else
    {
        sColumnCount = mUndoAnalyzedColCnt;
    }

    for(i = 0; i < sColumnCount; i++)
    {
        /* Disk인 경우, mBCols에 메모리를 할당하지 않는다. */

        // PROJ-1705
        sFreeNode = ID_FALSE;
        sChainedCols = &mBChainedCols[mCIDs[i]];
        if(sChainedCols->mAllocMethod != SMI_NON_ALLOCED)
        {
            // 맨 첫 smiChainedValue는 배열이므로, free해선 안된다.
            do
            {
                if(sChainedCols->mAllocMethod == SMI_NORMAL_ALLOC)
                {
                    (void)iduMemMgr::free((void *)sChainedCols->mColumn.value, mAllocator);
                }
                if(sChainedCols->mAllocMethod == SMI_MEMPOOL_ALLOC)
                {
                    (void)mChainedValuePool->memfree((void *)sChainedCols->mColumn.value);
                }

                sNextChainedCols = sChainedCols->mLink;
                if(sFreeNode == ID_TRUE)
                {
                    (void)iduMemMgr::free((void *)sChainedCols, mAllocator);
                }
                else
                {
                    sFreeNode = ID_TRUE;
                }
                sChainedCols = sNextChainedCols;
            }
            while(sChainedCols != NULL);

            // free 수행 여부를 allocMethod로 판단하므로, 여러 레코드를 업데이트 하는 경우,
            // 한 레코드 분석 시 마다 mAllocMethod를 초기화하여야한다.
            sChainedCols = &mBChainedCols[mCIDs[i]];
            sChainedCols->mColumn.value  = NULL;
            sChainedCols->mAllocMethod   = SMI_NON_ALLOCED;
            sChainedCols->mColumn.length = 0;
            sChainedCols->mLink          = NULL;
        }

        if(mACols[mCIDs[i]].value != NULL)
        {
            (void)iduMemMgr::free((void *)mACols[mCIDs[i]].value, mAllocator);
            mACols[mCIDs[i]].value = NULL;
            mACols[mCIDs[i]].length = 0;
        }
    }

#ifdef DEBUG
    for(i = 0; i < QCI_MAX_COLUMN_COUNT; i++)
    {
        IDE_ASSERT(mBCols[i].value == NULL);
        IDE_ASSERT(mACols[i].value == NULL);
        /*
         * PROJ-1705
         * linkedlist내의 모든 노드가 null임을 확인은 못하겠고,
         * 첫 노드의 value와 link상태만 확인한다.
         */
        IDE_ASSERT((mBChainedCols[i].mColumn.value == NULL) &&
                   (mBChainedCols[i].mLink         == NULL));
    }
#endif

    if(mLobPiece != NULL)
    {
        (void)iduMemMgr::free(mLobPiece, mAllocator);
        mLobPiece = NULL;
    }

    if( aIsAborted == ID_TRUE )
    {
        idlOS::memset( mCIDs, ID_UINT_MAX, ID_SIZEOF(UInt) * QCI_MAX_COLUMN_COUNT );
    }
    else
    {
        /*do nothing*/
    }

    return;
}

void rpdLogAnalyzer::resetVariables(idBool aNeedInitMtdValueLen,
                                    UInt   aTableColCount)
{
#ifdef DEBUG
    UInt i = 0;
#endif

    if(aNeedInitMtdValueLen == ID_TRUE)
    {
        /* Disk Table에만 관련된 초기화한다.
         * Selection 기능을 사용할 경우, UPDATE가 INSERT/DELETE로 변경될 수 있으므로,
         * INSERT, UPDATE, DELETE를 구분하지 않는다.
         */
        switch(mType)
        {
            case RP_X_INSERT :
            case RP_X_UPDATE :
            case RP_X_DELETE :
                if(mPKColCnt > 0)
                {
                    idlOS::memset(mPKMtdValueLen,
                                  0x00,
                                  ID_SIZEOF(rpValueLen) * mPKColCnt);
                }
                if(aTableColCount > 0)
                {
                    idlOS::memset(mBMtdValueLen,
                                  0x00,
                                  ID_SIZEOF(rpValueLen) * aTableColCount);
                    idlOS::memset(mAMtdValueLen,
                                  0x00,
                                  ID_SIZEOF(rpValueLen) * aTableColCount);
                }
                break;

            case RP_X_LOB_CURSOR_OPEN :
                if(mPKColCnt > 0)
                {
                    idlOS::memset(mPKMtdValueLen,
                                  0x00,
                                  ID_SIZEOF(rpValueLen) * mPKColCnt);
                }
                break;

            default :
                break;
        }
    }

#ifdef DEBUG
    for(i = 0; i < QCI_MAX_KEY_COLUMN_COUNT; i++)
    {
        IDE_ASSERT(mPKMtdValueLen[i].lengthSize == 0);
        IDE_ASSERT(mPKMtdValueLen[i].lengthValue == 0);
    }
    for(i = 0; i < QCI_MAX_COLUMN_COUNT; i++)
    {
        IDE_ASSERT(mBMtdValueLen[i].lengthSize == 0);
        IDE_ASSERT(mBMtdValueLen[i].lengthValue == 0);
        IDE_ASSERT(mAMtdValueLen[i].lengthSize == 0);
        IDE_ASSERT(mAMtdValueLen[i].lengthValue == 0);
    }
#endif

    if(mPKColCnt > 0)
    {
        idlOS::memset(mPKCIDs, 0, ID_SIZEOF(UInt) * mPKColCnt);
        idlOS::memset(mPKCols, 0, ID_SIZEOF(smiValue) * mPKColCnt);
    }

#ifdef DEBUG
    for(i = 0; i < QCI_MAX_KEY_COLUMN_COUNT; i++)
    {
        IDE_ASSERT(mPKCIDs[i] == 0);
        IDE_ASSERT(mPKCols[i].length == 0);
        IDE_ASSERT(mPKCols[i].value  == NULL);
    }
#endif

    if ( mRedoAnalyzedColCnt > 0 || mUndoAnalyzedColCnt > 0 )
    {
        if(aTableColCount > 0)
        {
            /*
             * PROJ-1705
             * 로그 분석 시, 컬럼이 CID순서로 분석되지 않으므로,
             * 분석이 끝난 후, 한꺼번에 정렬하는데, 초기값을 0으로 주면, CID가 0인 것과
             * 섞이게 되므로, 초기값을 UINT_MAX로 한다.
             */
            idlOS::memset(mCIDs, ID_UINT_MAX, ID_SIZEOF(UInt) * aTableColCount);
            idlOS::memset(mBCols, 0, ID_SIZEOF(smiValue) * aTableColCount);
            idlOS::memset(mACols, 0, ID_SIZEOF(smiValue) * aTableColCount);
        }
    }

#ifdef DEBUG
    for(i = 0; i < QCI_MAX_COLUMN_COUNT; i++)
    {
        IDE_ASSERT(mCIDs[i] == ID_UINT_MAX);
        IDE_ASSERT(mBCols[i].length == 0);
        IDE_ASSERT(mBCols[i].value == NULL);
        IDE_ASSERT(mACols[i].length == 0);
        IDE_ASSERT(mACols[i].value == NULL);
    }
#endif

    /* 배열이 아닌 변수들을 초기화한다. */
    mType               = RP_X_NONE;
    mTID                = 0;
    mSendTransID        = 0;
    mSN                 = 0;
    mTableOID           = 0;

    mPKColCnt           = 0;
    mUndoAnalyzedColCnt = 0;
    mRedoAnalyzedColCnt = 0;

    mUndoAnalyzedLen    = 0;
    mRedoAnalyzedLen    = 0;
    mLobAnalyzedLen     = 0;

    mSPNameLen          = 0;

    mFlushOption        = 0;

    mLobLocator         = 0;
    mLobColumnID        = 0;
    mLobOffset          = 0;
    mLobOldSize         = 0;
    mLobNewSize         = 0;
    mLobPieceLen        = 0;
    mLobPiece           = NULL;

    mIsCont             = ID_FALSE;
    mNeedFree           = ID_FALSE;
    mNeedConvertToMtdValue = ID_FALSE;

    mImplSPDepth        = SMI_STATEMENT_DEPTH_NULL;
    idlOS::memset(mSPName, 0, RP_SAVEPOINT_NAME_LEN + 1);

    return;
}

IDE_RC rpdLogAnalyzer::analyze( smiLogRec *aLog,
                                idBool    *aIsDML,
                                smTID      aTID )
{
    smTID      sTID = aTID;
    smSN       sLogRecordSN = SM_SN_NULL;
    smiLogType sType = aLog->getType();    // BUG-22613

    IDE_DASSERT(aLog != NULL);

    *aIsDML = ID_FALSE;
    sLogRecordSN = aLog->getRecordSN();

    switch(sType)
    {
        case SMI_LT_TRANS_COMMIT :
            idlOS::memcpy( &mGlobalCommitSCN, aLog->getGlobalCommitSCN(), ID_SIZEOF(smSCN) );

        case SMI_LT_TRANS_GROUPCOMMIT :
            IDE_TEST_RAISE(mIsCont == ID_TRUE, ERR_INVALID_CONT_FLAG);
            setXLogHdr(RP_X_COMMIT, sTID, sLogRecordSN);
            break;

        case SMI_LT_TRANS_ABORT :
        case SMI_LT_TRANS_PREABORT :
            cancelAnalyzedLog();
            setXLogHdr(RP_X_ABORT, sTID, sLogRecordSN);
            break;

        case SMI_LT_SAVEPOINT_SET :
            setXLogHdr(RP_X_SP_SET, sTID, sLogRecordSN);
            IDE_TEST( smiLogRec::analyzeTxSavePointSetLog( aLog,
                                                           &mSPNameLen,
                                                           mSPName )
                      != IDE_SUCCESS );
            break;

        case SMI_LT_SAVEPOINT_ABORT :
            cancelAnalyzedLog();
            setXLogHdr(RP_X_SP_ABORT, sTID, sLogRecordSN);
            IDE_TEST( smiLogRec::analyzeTxSavePointAbortLog( aLog,
                                                             &mSPNameLen,
                                                             mSPName )
                      != IDE_SUCCESS );
            break;

        case SMI_LT_MEMORY_CHANGE:
        case SMI_LT_DISK_CHANGE:
            if(aLog->checkSavePointFlag() == ID_TRUE)
            {
                mImplSPDepth = aLog->getReplStmtDepth();
            }

        case SMI_LT_LOB_FOR_REPL:
            *aIsDML = ID_TRUE;
            IDE_TEST(mAnalyzeFunc[aLog->getChangeType()](this,
                                                         aLog,
                                                         sTID,
                                                         sLogRecordSN)
                     != IDE_SUCCESS);
            break;

        case SMI_LT_TABLE_META :
            setXLogHdr(RP_X_NONE, sTID, sLogRecordSN);
            if(aLog->checkSavePointFlag() == ID_TRUE)
            {
                mImplSPDepth = aLog->getReplStmtDepth();
            }
            break;

        case SMI_LT_XA_START_REQ :
            setXLogHdr( RP_X_XA_START_REQ, sTID, sLogRecordSN );
            idlOS::memcpy( &mXID, aLog->getXID(), ID_SIZEOF(ID_XID) );

            break;
        case SMI_LT_XA_PREPARE_REQ :
            setXLogHdr( RP_X_XA_PREPARE_REQ, sTID, sLogRecordSN );
            idlOS::memcpy( &mXID, aLog->getXID(), ID_SIZEOF(ID_XID) );
            break;

        case SMI_LT_XA_PREPARE :
            setXLogHdr( RP_X_XA_PREPARE, sTID, sLogRecordSN );
            idlOS::memcpy( &mXID, aLog->getXID(), ID_SIZEOF(ID_XID) );
            break;

        case SMI_LT_XA_END :
            setXLogHdr( RP_X_XA_END, sTID, sLogRecordSN );
            idlOS::memcpy( &mXID, aLog->getXID(), ID_SIZEOF(ID_XID) );
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CONT_FLAG);
    {
        /* BUG-28939 중복 begin을 발생시킬 가능성이 있는 대상을 찾는다.
         * 잘못된 continue flag로그로 인해 뒤에 오는 commit이 skip되어
         * transaction table이 비워지지 않을 수 있다. remove transaction table하는
         * 데이터타입을 대상으로 continue flag를 확인한다. */
        IDE_SET(ideSetErrorCode(rpERR_ABORT_INVALID_CONT_LOG,
                                sLogRecordSN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool rpdLogAnalyzer::isInCIDArray( UInt aCID )
{
    UInt    sLow;
    UInt    sHigh;
    UInt    sMid;
    idBool  sRet = ID_FALSE;

    sLow = 0;
    sHigh = (UInt)mRedoAnalyzedColCnt - 1;

    while ( sLow <= sHigh )
    {
        sMid = ( sHigh + sLow ) >> 1;
        if ( mCIDs[sMid] > aCID )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
            sHigh = sMid - 1;
        }
        else if ( mCIDs[sMid] < aCID )
        {
            sLow = sMid + 1;
        }
        else
        {
            sRet = ID_TRUE;
            break;
        }
    }

    return sRet;
}

IDE_RC rpdLogAnalyzer::anlzInsMem(rpdLogAnalyzer *aLA,
                                  smiLogRec      *aLog,
                                  smTID           aTID,
                                  smSN            aSN)
{
    aLA->setXLogHdr(RP_X_INSERT, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    IDE_TEST( smiLogRec::analyzeInsertLogMemory( aLog,
                                                 &aLA->mRedoAnalyzedColCnt,
                                                 aLA->mCIDs,
                                                 aLA->mACols,
                                                 &aLA->mIsCont )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzUdtMem(rpdLogAnalyzer *aLA,
                                  smiLogRec      *aLog,
                                  smTID           aTID,
                                  smSN            aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();
    //PROJ-1705 메모리 테이블에서는 Redo / Undo AnalyzedColCnt의 구분이 의미가 없다.
    //의미없이 둘 중의 하나 mRedoAnalyzedColCnt 변수를 사용 한다.
    IDE_TEST( smiLogRec::analyzeUpdateLogMemory( aLog,
                                                 &aLA->mPKColCnt,
                                                 aLA->mPKCIDs,
                                                 aLA->mPKCols,
                                                 &aLA->mRedoAnalyzedColCnt,
                                                 aLA->mCIDs,
                                                 aLA->mBCols,
                                                 aLA->mACols,
                                                 &aLA->mIsCont )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzDelMem(rpdLogAnalyzer *aLA,
                                  smiLogRec      *aLog,
                                  smTID           aTID,
                                  smSN            aSN)
{
    aLA->setXLogHdr(RP_X_DELETE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    IDE_TEST( smiLogRec::analyzeDeleteLogMemory( aLog,
                                                 &aLA->mPKColCnt,
                                                 aLA->mPKCIDs,
                                                 aLA->mPKCols,
                                                 &aLA->mIsCont,
                                                 &aLA->mUndoAnalyzedColCnt,
                                                 aLA->mCIDs,
                                                 aLA->mBCols )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < REDO INSERT DML 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |   ==> NULL
 * ----------------
 * | Update info  |   ==> NULL
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzRedoInsertDisk(rpdLogAnalyzer *aLA,
                                          smiLogRec      *aLog,
                                          smTID           aTID,
                                          smSN            aSN)
{
    aLA->setXLogHdr(RP_X_INSERT, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_REDO_LOG)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mACols,
                                        NULL,
                                        NULL,
                                        &aLA->mRedoAnalyzedLen,
                                        &aLA->mRedoAnalyzedColCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < REDO UPDATE DML - UPDATE ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |   ==> NULL
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzRedoUpdateDisk(rpdLogAnalyzer * aLA,
                                          smiLogRec      * aLog,
                                          smTID            aTID,
                                          smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_REDO_LOG)
             != IDE_SUCCESS);

    if(aLog->getUpdateColCntInRowPiece() == 0)
    {
        aLA->setSkipXLog(ID_TRUE);
        return IDE_SUCCESS;
    }

    IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mACols,
                                        NULL,
                                        NULL,
                                        &aLA->mRedoAnalyzedLen,
                                        &aLA->mRedoAnalyzedColCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < REDO UPDATE DML - INSERT ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |   ==> NULL
 * ----------------
 * | Update info  |   ==> NULL
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzRedoUpdateInsertRowPieceDisk(rpdLogAnalyzer * aLA,
                                                        smiLogRec      * aLog,
                                                        smTID            aTID,
                                                        smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_REDO_LOG)
             != IDE_SUCCESS);

    if(aLog->getUpdateColCntInRowPiece() > 0)
    {
        IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                            aLog,
                                            aLA->mCIDs,
                                            aLA->mACols,
                                            NULL,
                                            NULL,
                                            &aLA->mRedoAnalyzedLen,
                                            &aLA->mRedoAnalyzedColCnt)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < REDO UPDATE DML - OVERWRITE ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |   ==> NULL
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzRedoUpdateOverwriteDisk(rpdLogAnalyzer * aLA,
                                                   smiLogRec      * aLog,
                                                   smTID            aTID,
                                                   smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_REDO_LOG)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mACols,
                                        NULL,
                                        NULL,
                                        &aLA->mRedoAnalyzedLen,
                                        &aLA->mRedoAnalyzedColCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 로그 헤더를 XLog에 입력하기 위해 존재한다.
 *               XLog에 필요한 delete 정보는 로그의 맨 뒤에 나오는
 *               PK log에 존재한다.
 *               undo delete log는 그냥 skip하고,
 *                  : TASK-5030에 의해 undo도 분석한다.
 *               redo delete log에서 헤더정보만 XLog에 채워둔다.
 *
 ***********************************************************************/
IDE_RC rpdLogAnalyzer::anlzRedoDeleteDisk(rpdLogAnalyzer * aLA,
                                          smiLogRec      * aLog,
                                          smTID            aTID,
                                          smSN             aSN)
{
    aLA->setXLogHdr(RP_X_DELETE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();
    aLA->setSkipXLog(ID_TRUE);

    return IDE_SUCCESS;
}


/***************************************************************
 * < UNDO UPDATE DML - DELETE ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzUndoDeleteDisk(rpdLogAnalyzer * aLA,
                                          smiLogRec      * aLog,
                                          smTID            aTID,
                                          smSN             aSN)
{
    /* TASK-5030 
     * Full XLog 일때만 분석 */
    if( aLog->needSupplementalLog() == ID_TRUE )
    {
        aLA->setXLogHdr(RP_X_DELETE, aTID, aSN);
        aLA->mTableOID = aLog->getTableOID();

        aLA->setNeedFree(ID_TRUE);

        aLA->setSkipXLog(ID_TRUE);

        IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                          &aLA->mIsCont)
                 != IDE_SUCCESS);

        IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_UNDO_LOG)
                 != IDE_SUCCESS);

        IDE_TEST(smiLogRec::analyzeUndoInfo(aLog)
                 != IDE_SUCCESS);

        IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
                 != IDE_SUCCESS);

        IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                            aLog,
                                            aLA->mCIDs,
                                            aLA->mBCols,
                                            aLA->mBChainedCols,
                                            aLA->mChainedValueTotalLen,
                                            &aLA->mUndoAnalyzedLen,
                                            &aLA->mUndoAnalyzedColCnt)
                 != IDE_SUCCESS);
    }
    else
    {
        aLA->setSkipXLog(ID_TRUE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***************************************************************
 * < UNDO UPDATE DML - UPDATE ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzUndoUpdateDisk(rpdLogAnalyzer * aLA,
                                          smiLogRec      * aLog,
                                          smTID            aTID,
                                          smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_UNDO_LOG)
             != IDE_SUCCESS);

    if(aLog->getUpdateColCntInRowPiece() == 0)
    {
        return IDE_SUCCESS;
    }

    IDE_TEST(smiLogRec::analyzeUndoInfo(aLog)
             != IDE_SUCCESS);


    IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mBCols,
                                        aLA->mBChainedCols,
                                        aLA->mChainedValueTotalLen,
                                        &aLA->mUndoAnalyzedLen,
                                        &aLA->mUndoAnalyzedColCnt)
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < UNDO UPDATE DML - DELETE ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzUndoUpdateDeleteRowPieceDisk(rpdLogAnalyzer * aLA,
                                                        smiLogRec      * aLog,
                                                        smTID            aTID,
                                                        smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_UNDO_LOG)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUndoInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mBCols,
                                        aLA->mBChainedCols,
                                        aLA->mChainedValueTotalLen,
                                        &aLA->mUndoAnalyzedLen,
                                        &aLA->mUndoAnalyzedColCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < UNDO UPDATE DML - OVERWRITE ROW PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzUndoUpdateOverwriteDisk(rpdLogAnalyzer * aLA,
                                                   smiLogRec      * aLog,
                                                   smTID            aTID,
                                                   smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_UNDO_LOG)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUndoInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mBCols,
                                        aLA->mBChainedCols,
                                        aLA->mChainedValueTotalLen,
                                        &aLA->mUndoAnalyzedLen,
                                        &aLA->mUndoAnalyzedColCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < UNDO UPDATE DML - DELETE FIRST COLUMN PIECE 로그 구조 >
 *
 * ----------------
 * |  sdrLogHdr   |
 * ----------------
 * |  Undo info   |
 * ----------------
 * | Update info  |
 * ----------------
 * |  Row image   |
 * ----------------
 * |   RP info    |
 * ----------------
 *
 **************************************************************/
IDE_RC rpdLogAnalyzer::anlzUndoUpdateDeleteFirstColumnDisk(rpdLogAnalyzer * aLA,
                                                           smiLogRec      * aLog,
                                                           smTID            aTID,
                                                           smSN             aSN)
{
    aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    aLA->setNeedFree(ID_TRUE);

    aLA->setSkipXLog(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRPInfo(aLog, SMI_UNDO_LOG)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUndoInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeUpdateInfo(aLog)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzeRowImage(aLA->mAllocator,
                                        aLog,
                                        aLA->mCIDs,
                                        aLA->mBCols,
                                        aLA->mBChainedCols,
                                        aLA->mChainedValueTotalLen,
                                        &aLA->mUndoAnalyzedLen,
                                        &aLA->mUndoAnalyzedColCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : update/delete DML을 위한 정보이다.
 *               각 로그의 맨 뒤에 나온다.
 *
 ***********************************************************************/
IDE_RC rpdLogAnalyzer::anlzPKDisk(rpdLogAnalyzer * aLA,
                                  smiLogRec      * aLog,
                                  smTID          /* aTID */,
                                  smSN           /* aSN */)
{
    /*
     * addXLog에서 처음에 needFree를 False로 초기화 한다.
     * PK log는 malloc하지 않으므로, Free가 필요하지 않으나,
     * 이전에 분석한 undo log와 redo log를 Free하기 위해 needFree를
     * TRUE로 셋팅한다.
     * 만약 FALSE로 되어 있으면,
     * 여러개의 레코드를 동시에 업데이트 시에 rpdLogAnalyzer 구조체가
     * 초기화 되어있지않아, 앞 레코드의 데이터에 이어서 저장되게 된다.
     */
    aLA->setNeedFree(ID_TRUE);

    IDE_TEST(smiLogRec::analyzeHeader(aLog,
                                      &aLA->mIsCont)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzePKDisk(aLog,
                                      &aLA->mPKColCnt,
                                      aLA->mPKCIDs,
                                      aLA->mPKCols)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzWriteDskLobPiece(rpdLogAnalyzer *aLA,
                                            smiLogRec      *aLog,
                                            smTID           aTID,
                                            smSN            aSN)
{
    UInt   sCID           = UINT_MAX;
    idBool sIsAfterInsert = ID_FALSE;

    if(aLA->mType == RP_X_NONE)
    {
        aLA->setXLogHdr(RP_X_UPDATE, aTID, aSN);
        aLA->mTableOID = aLog->getTableOID();
    }
    if(aLA->mType == RP_X_INSERT)
    {
        sIsAfterInsert = ID_TRUE;
    }

    aLA->setNeedFree(ID_TRUE);
    aLA->mTableOID = aLog->getTableOID();
    IDE_TEST(smiLogRec::analyzeWriteLobPieceLogDisk(aLA->mAllocator,
                                                    aLog,
                                                    aLA->mACols,
                                                    aLA->mCIDs,
                                                    &aLA->mLobAnalyzedLen,
                                                    &aLA->mRedoAnalyzedColCnt,
                                                    &aLA->mIsCont,
                                                    &sCID,
                                                    sIsAfterInsert)
             != IDE_SUCCESS);

    /* 한 LOB column value의 copy를 완료했으면, 초기화 */
    if(aLA->mLobAnalyzedLen == aLA->mACols[sCID].length)
    {
        aLA->mLobAnalyzedLen = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzLobCurOpenMem(rpdLogAnalyzer *aLA,
                                         smiLogRec      *aLog,
                                         smTID           aTID,
                                         smSN            aSN)
{
    aLA->mLobLocator = aLog->getLobLocator();
    aLA->setXLogHdr(RP_X_LOB_CURSOR_OPEN, aTID, aSN);
    IDE_TEST(smiLogRec::analyzeLobCursorOpenMem(aLog,
                                                &aLA->mPKColCnt,
                                                aLA->mPKCIDs,
                                                aLA->mPKCols,
                                                &aLA->mTableOID,
                                                &aLA->mLobColumnID)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzLobCurOpenDisk(rpdLogAnalyzer *aLA,
                                          smiLogRec      *aLog,
                                          smTID           aTID,
                                          smSN            aSN)
{
    aLA->mLobLocator = aLog->getLobLocator();
    aLA->setXLogHdr(RP_X_LOB_CURSOR_OPEN, aTID, aSN);
    IDE_TEST(smiLogRec::analyzeLobCursorOpenDisk(aLog,
                                                 &aLA->mPKColCnt,
                                                 aLA->mPKCIDs,
                                                 aLA->mPKCols,
                                                 &aLA->mTableOID,
                                                 &aLA->mLobColumnID)
             != IDE_SUCCESS);

    IDE_TEST(smiLogRec::analyzePKDisk(aLog,
                                      &aLA->mPKColCnt,
                                      aLA->mPKCIDs,
                                      aLA->mPKCols)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzLobCurClose(rpdLogAnalyzer *aLA,
                                       smiLogRec      *aLog,
                                       smTID           aTID,
                                       smSN            aSN)
{
    aLA->mLobLocator = aLog->getLobLocator();
    aLA->setXLogHdr(RP_X_LOB_CURSOR_CLOSE, aTID, aSN);

    return IDE_SUCCESS;
}

IDE_RC rpdLogAnalyzer::anlzLobPrepare4Write(rpdLogAnalyzer *aLA,
                                            smiLogRec      *aLog,
                                            smTID           aTID,
                                            smSN            aSN)
{
    aLA->mLobLocator = aLog->getLobLocator();
    aLA->setXLogHdr(RP_X_LOB_PREPARE4WRITE, aTID, aSN);
    IDE_TEST( smiLogRec::analyzeLobPrepare4Write( aLog,
                                                  &aLA->mLobOffset,
                                                  &aLA->mLobOldSize,
                                                  &aLA->mLobNewSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzLobFinish2Write(rpdLogAnalyzer *aLA,
                                           smiLogRec      *aLog,
                                           smTID           aTID,
                                           smSN            aSN)
{
    aLA->mLobLocator = aLog->getLobLocator();
    aLA->setXLogHdr(RP_X_LOB_FINISH2WRITE, aTID, aSN);

    return IDE_SUCCESS;
}

IDE_RC rpdLogAnalyzer::anlzLobPartialWriteMem(rpdLogAnalyzer *aLA,
                                              smiLogRec      *aLog,
                                              smTID           aTID,
                                              smSN            aSN)
{
    aLA->setNeedFree(ID_TRUE);
    aLA->setXLogHdr(RP_X_LOB_PARTIAL_WRITE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();
    IDE_TEST( smiLogRec::analyzeLobPartialWriteMemory( aLA->mAllocator,
                                                       aLog,
                                                       &aLA->mLobLocator,
                                                       &aLA->mLobOffset,
                                                       &aLA->mLobPieceLen,
                                                       &aLA->mLobPiece )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzLobPartialWriteDsk(rpdLogAnalyzer *aLA,
                                              smiLogRec      *aLog,
                                              smTID           aTID,
                                              smSN            aSN)
{
    UInt    sAmount;

    aLA->setNeedFree(ID_TRUE);
    aLA->setXLogHdr(RP_X_LOB_PARTIAL_WRITE, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();
    IDE_TEST( smiLogRec::analyzeLobPartialWriteDisk( aLA->mAllocator,
                                                     aLog,
                                                     &aLA->mLobLocator,
                                                     &aLA->mLobOffset,
                                                     &sAmount,
                                                     &aLA->mLobPieceLen,
                                                     &aLA->mLobPiece )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdLogAnalyzer::anlzLobTrim(rpdLogAnalyzer *aLA,
                                   smiLogRec      *aLog,
                                   smTID           aTID,
                                   smSN            aSN)
{
    aLA->mLobLocator = aLog->getLobLocator();
    aLA->setXLogHdr(RP_X_LOB_TRIM, aTID, aSN);
    IDE_TEST( smiLogRec::analyzeLobTrim( aLog,
                                         &aLA->mLobOffset )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// 분석할 필요가 없는 로그타입
IDE_RC rpdLogAnalyzer::anlzNA(rpdLogAnalyzer * aLA,
                              smiLogRec      * /* aLog */,
                              smTID           /* aTID */,
                              smSN            /* aSN */)
{
    aLA->setSkipXLog(ID_TRUE);
    return IDE_SUCCESS;
}
void   rpdLogAnalyzer::insertDictValue( rpdDictionaryValue *aValue )
{

    IDU_LIST_INIT_OBJ( &(aValue->mNode), aValue );
    IDU_LIST_ADD_FIRST( &mDictionaryValueList, &(aValue->mNode) );
}

rpdDictionaryValue* rpdLogAnalyzer::getDictValueFromList( smOID aOID )
{
    iduListNode * sIterator = NULL;
    iduListNode * sNextNode = NULL;
    rpdDictionaryValue * sCurrentNode = NULL;
    rpdDictionaryValue * sResultNode = NULL;


    IDU_LIST_ITERATE_SAFE( &mDictionaryValueList, sIterator, sNextNode )
    {
        sCurrentNode = (rpdDictionaryValue *)sIterator->mObj;
        if ( sCurrentNode->mDictValueOID == aOID )
        {
            sResultNode = sCurrentNode;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
    return sResultNode;
}


IDE_RC rpdLogAnalyzer::anlzInsDictionary( rpdLogAnalyzer *aLA,
                                          smiLogRec      *aLog,
                                          smTID           aTID,
                                          smSN            aSN)
{
    aLA->setXLogHdr(RP_X_INSERT, aTID, aSN);
    aLA->mTableOID = aLog->getTableOID();

    IDE_TEST( smiLogRec::analyzeInsertLogDictionary( aLog,
                                                     &aLA->mRedoAnalyzedColCnt,
                                                     aLA->mCIDs,
                                                     aLA->mACols,
                                                     &aLA->mIsCont )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void   rpdLogAnalyzer::destroyDictValueList()
{
    iduListNode * sIterator = NULL;
    iduListNode * sNextNode = NULL;
    rpdDictionaryValue * sCurrentNode = NULL;

    IDU_LIST_ITERATE_SAFE( &mDictionaryValueList, sIterator, sNextNode )
    {
        sCurrentNode = (rpdDictionaryValue *)sIterator->mObj;

        IDE_DASSERT(sCurrentNode != NULL);

        IDU_LIST_REMOVE( &(sCurrentNode->mNode) );

        (void)iduMemMgr::free( (void*)(sCurrentNode->mValue.value), mAllocator );
        (void)iduMemMgr::free( sCurrentNode, mAllocator );

    }

}

void rpdLogAnalyzer::setSendTransID( smTID     aTransID )
{
    mSendTransID = aTransID;
}

smTID  rpdLogAnalyzer::getSendTransID( void )
{
    return mSendTransID;
}

smSCN  rpdLogAnalyzer::getGlobalCommitSCN( void )
{
    return mGlobalCommitSCN;
}
