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
 * Copyight 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: smiLogRec.cpp 90444 2021-04-02 10:15:58Z minku.kang $
 **********************************************************************/

#include <smErrorCode.h>
#include <smr.h>
#include <sdr.h>
#include <sdc.h>
#include <sdp.h>
#include <smiMisc.h>
#include <smiLogRec.h>
#include <smrLogHeadI.h>

extern smiGlobalCallBackList gSmiGlobalCallBackList;
/*
  smiAnalyzeLogFunc smiLogRec::mAnalyzeLogFunc[] =
  {
  NULL,                             // null
  analyzeInsertLogMemory,           // MTBL insert
  analyzeUpdateLogMemory,           // MTBL update
  analyzeDeleteLogMemory,           // MTBL delete
  analyzePKDisk,
  analyzeRedoInsertDisk,
  analyzeRedoDeleteDisk,
  analyzeRedoUpdateDisk,
  analyzeRedoUpdateInsertRowPieceDisk,
  NULL,                             //analyzeRedoUpdateDeleteRowPieceDisk
  analyzeRedoUpdateOverwriteDisk,
  NULL,                             //analyzeRedoUpdateDeleteFirstColumnDisk
  NULL,                             // anlzUndoInsertDisk
  NULL,                             // anlzUndoDeleteDisk
  analyzeUndoUpdateDisk,
  NULL,                             //analyzeUndoUpdateInsertRowPieceDisk,
  analyzeUndoUpdateDeleteRowPieceDisk,
  analyzeUndoUpdateOverwriteDisk,
  analyzeUndoUpdateDeleteFirstColumnDisk,
  analyzeWriteLobPieceLogDisk,      // DTBL Write LOB Piece
  analyzeLobCursorOpenMem,          // MTBL LOB Cursor Open
  analyzeLobCursorOpenDisk,         // DTBL LOB Cursor Close
  analyzeLobCursorClose,            // LOB Cursor Close
  analyzeLobPrepare4Write,          // LOB Prepare for Write
  analyzeLobFinish2Write,           // LOB Finish to Write
  analyzeLobPartialWriteMemory,     // MTBL LOB Partial Write
  analyzeLobPartialWriteDisk,       // DTBL LOB Partial Write
  NULL
  };
*/


/***************************************************************
 * Full XLog : Primary Key 영역 뒤에 저장
 * -----------------------------------------
 * | FXLog  | FXLog  |     Column DATA     |
 * | SIZE   | COUNT  |---------------------|
 * |        |        | ID | Length | Value |
 * -----------------------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzeFullXLogMemory( smiLogRec  * aLogRec,
                                         SChar      * aXLogPtr,
                                         UShort     * aColCount,
                                         UInt       * aCIDs,
                                         smiValue   * aBColValueArray,
                                         const void * aTable )
{
    UInt        i           = 0;
    UInt        sOffset     = 0;
    UInt        sDataSize   = 0;
    UInt        sFXLogCnt   = 0;
    UInt        sFXLogSize  = 0;
    SChar     * sFXLogPtr;

    const smiColumn * spCol  = NULL;

    /* Full XLog Size */
    sFXLogSize  = aLogRec->getUIntValue( aXLogPtr, sOffset );
    sOffset    += ID_SIZEOF(UInt);

    /* Full XLog Count */
    sFXLogCnt   = aLogRec->getUIntValue( aXLogPtr, sOffset );
    *aColCount  = sFXLogCnt;
    sOffset    += ID_SIZEOF(UInt);

    sFXLogPtr = aXLogPtr;

    for( i = 0 ; i < sFXLogCnt ; i++ )
    {
        /* Get Column ID */
        aCIDs[i] = aLogRec->getUIntValue(sFXLogPtr, sOffset) & SMI_COLUMN_ID_MASK ;
        sOffset += ID_SIZEOF(UInt);

        /* column value length */
        sDataSize = aLogRec->getUIntValue(sFXLogPtr, sOffset);
        sOffset += ID_SIZEOF(UInt);

        spCol = aLogRec->mGetColumn( aTable, aCIDs[i] );

        /* column value */

        if ( sDataSize <= spCol->size )
        {
            (aBColValueArray[i]).length  = sDataSize; //length 
        }
        else
        {
            (aBColValueArray[i]).length = spCol->size;
        }
        (aBColValueArray[i]).value   = sFXLogPtr + sOffset; // value

        sOffset += sDataSize;

        IDE_ASSERT( sOffset <= sFXLogSize );
   }

    return IDE_SUCCESS;
}


/***************************************************************
 * Primary Key 영역 로그
 * ----------------------------------
 * |  PK   |  PK    |               |
 * | Size  | Column |   Column Area |
 * |       | Count  |               |
 * ----------------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzePKMem(smiLogRec  *aLogRec,
                               SChar      *aPKAreaPtr,
                               UInt       *aPKColCnt,
                               UInt       *aPKColSize,
                               UInt       *aPKCIDArray,
                               smiValue   *aPKColValueArray,
                               const void *aTable )
{
    UInt     sOffset;
    SChar   *sPKColPtr;

    sOffset = 0;

    /* Get Primary Key Area Size */
    *aPKColSize = aLogRec->getUIntValue(aPKAreaPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    /* Get Primary Key Column Count */
    *aPKColCnt = aLogRec->getUIntValue(aPKAreaPtr, sOffset);
    sOffset  += ID_SIZEOF(UInt);

    sPKColPtr = aPKAreaPtr + sOffset;

    /* Analyze Primary Key Column Area */
    IDE_TEST( analyzePrimaryKey( aLogRec,
                                 sPKColPtr,
                                 aPKColCnt,
                                 aPKCIDArray,
                                 aPKColValueArray,
                                 aTable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiLogRec::analyzePrimaryKey( smiLogRec  * aLogRec,
                                     SChar      * aPKColImagePtr,
                                     UInt       * aPKColCnt,
                                     UInt       * aPKCIDArray,
                                     smiValue   * aPKColsArray,
                                     const void * aTable )
{
    UInt   i;
    UInt   sAnalyzedColSize = 0;   /* BUG-43565 */ 
    SChar *sPKColPtr;

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aPKColImagePtr != NULL );
    IDE_DASSERT( aPKColCnt != NULL );
    IDE_DASSERT( aPKCIDArray != NULL );
    IDE_DASSERT( aPKColsArray != NULL );

    IDE_DASSERT(*aPKColCnt > 0 );
    IDE_DASSERT((aLogRec->getChangeType() == SMI_CHANGE_MRDB_UPDATE) ||
                (aLogRec->getChangeType() == SMI_CHANGE_MRDB_DELETE) ||
                (aLogRec->getChangeType() == SMI_CHANGE_MRDB_LOB_CURSOR_OPEN));

    sPKColPtr = aPKColImagePtr;
    for(i = 0; i < *aPKColCnt; i++)
    {
        IDE_TEST(analyzePrimaryKeyColumn(aLogRec,
                                         sPKColPtr,
                                         &aPKCIDArray[i],
                                         &aPKColsArray[i],
                                         aTable,
                                         &sAnalyzedColSize )
                 != IDE_SUCCESS );

        /* BUG-43565 : 분석한 컬럼의 size가 보정 될 수 있으므로 보정되기 전 크기를 
         * 이용하여 포인터를 PK의 다음 컬럼으로 이동한다. */
        sPKColPtr += ( ID_SIZEOF(UInt)      /*Length*/ 
                       + ID_SIZEOF(UInt)    /*CID*/ 
                       + sAnalyzedColSize   /*보정 전 Col. size*/ );
}

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiLogRec::analyzePrimaryKeyColumn(smiLogRec  *aLogRec,
                                          SChar      *aPKColPtr,
                                          UInt       *aPKCid,
                                          smiValue   *aPKCol,
                                          const void *aTable,
                                          UInt       *aAnalyzedColSize )
{
    UInt    sDataSize;
    UInt    sCID;
    UInt    sOffset = 0;
    SChar   sErrorBuffer[256];
    
    const smiColumn * spCol  = NULL;

    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aPKColPtr != NULL );
    IDE_DASSERT( aPKCid != NULL );
    IDE_DASSERT( aPKCol != NULL );

    /* column value length */
    sDataSize = aLogRec->getUIntValue(aPKColPtr, 0);
    sOffset += ID_SIZEOF(UInt);

    /* Get Column ID */
    sCID = aLogRec->getUIntValue(aPKColPtr, sOffset) & SMI_COLUMN_ID_MASK ;
    sOffset += ID_SIZEOF(UInt);

    IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

    *aPKCid = sCID;
    aPKCol->value = aPKColPtr + sOffset;

    if ( aTable != NULL )
    {
        spCol = aLogRec->mGetColumn( aTable, sCID );

        if ( sDataSize <= spCol->size )
        {
            aPKCol->length = sDataSize;
        }
        else
        {
            aPKCol->length = spCol->size;
        }
    }
    else
    {
        aPKCol->length = sDataSize;
    }

    /* BUG-43565 : 보정 전 Col. size 전달 */
    *aAnalyzedColSize = sDataSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LARGE_CID );
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzePrimaryKeyColumn] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN : %"ID_UINT32_FMT",%"ID_UINT32_FMT" ]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < PK 로그 구조 >
 *
 *       |<-- sAnalyzePtr는 여기부터 시작
 *               |<------------- repeat ------------>|
 * ---------------------------------------------------
 * | sdr | PK    | PK     |        |        |        |
 * | Log | total | column | column | column | column |
 * | Hdr | size  | count  | id     | length | data   |
 * ---------------------------------------------------
 *
 * PK size는 4K 이하로 제한 되어 있으며, 하나의 로그에 모두 쓰여진다.
 **************************************************************/
IDE_RC smiLogRec::analyzePKDisk(smiLogRec  *aLogRec,
                                UInt       *aPKColCnt,
                                UInt       *aPKCIDArray,
                                smiValue   *aPKColValueArray)
{
    UInt     i;
    SChar  * sAnalyzePtr;
    UShort   sPKSize;
    UShort   sLen;
    SChar    sErrorBuffer[256];

    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aPKColCnt != NULL );
    IDE_DASSERT( aPKCIDArray != NULL );
    IDE_DASSERT( aPKColValueArray != NULL );
    IDE_DASSERT( (aLogRec->getChangeType() == SMI_PK_DRDB) ||
                 (aLogRec->getChangeType() == SMI_CHANGE_DRDB_LOB_CURSOR_OPEN) );

    sAnalyzePtr = aLogRec->getAnalyzeStartPtr();

    // 1. Get PK total size
    sPKSize = aLogRec->getUShortValue(sAnalyzePtr);
    sAnalyzePtr += ID_SIZEOF(UShort);

    IDE_ASSERT(sPKSize > 0);

    // 2. Get PK count
    *aPKColCnt = aLogRec->getUShortValue(sAnalyzePtr);
    sAnalyzePtr += ID_SIZEOF(UShort);

    IDE_ASSERT(*aPKColCnt > 0 );

    for(i = 0; i < *aPKColCnt; i++)
    {
        // 3. Get PK Column ID
        aPKCIDArray[i] = aLogRec->getUIntValue(sAnalyzePtr) & SMI_COLUMN_ID_MASK;
        sAnalyzePtr += ID_SIZEOF(UInt);
        IDE_TEST_RAISE( aPKCIDArray[i] > SMI_COLUMN_ID_MAXIMUM, ERR_INVALID_CID );

        // 4. Get PK Column length
        analyzeColumnAndMovePtr((SChar **)&sAnalyzePtr,
                                &sLen,
                                NULL,
                                NULL);

        // 5. Get PK Column data
        aPKColValueArray[i].length = sLen;
        aPKColValueArray[i].value = (SChar *)sAnalyzePtr;

        sAnalyzePtr += sLen;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CID);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzePrimaryKeyColumn] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT"]",
                         aPKCIDArray[i], 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 *  PROJ-1705
 *
 *  column length 분석 :
 *
 *  column length 정보는 1~3byte로 저장되어 있다.
 *  우선 1byte를 읽는다. 그 값이,
 *  1. <0xFE>이면, 이 컬럼의 길이는 250을 초과한
 *     것으로 다음의 2byte를 읽어 분석한다.
 *  2. <0xFF>이면, 컬럼의 value가 NULL인 것으로 판단한다.
 *  3. <0xFD>이면, Out Mode LOB Descriptor이다.
 *  4. 그 외의 값이면, 250 이하의 컬럼 길이이다.
 *
 **************************************************************/
void smiLogRec::analyzeColumnAndMovePtr( SChar  ** aAnalyzePtr,
                                         UShort  * aColumnLen,
                                         idBool  * aIsOutModeLob,
                                         idBool  * aIsNull )
{
    SChar  * sLen;
    UShort   sLongLen;

    if ( aIsOutModeLob != NULL )
    {
        *aIsOutModeLob = ID_FALSE;
    }

    if ( aIsNull != NULL )
    {
        *aIsNull = ID_FALSE;
    }

    sLen = (SChar *)*aAnalyzePtr;
    *aAnalyzePtr += ID_SIZEOF(SChar);

    if ( (*sLen & 0xFF) == SMI_LONG_VALUE )
    {
        idlOS::memcpy( &sLongLen, *aAnalyzePtr, ID_SIZEOF(UShort) );
        IDE_DASSERT( sLongLen > SMI_SHORT_VALUE );
        *aAnalyzePtr += ID_SIZEOF(UShort);

        *aColumnLen = sLongLen;
    }
    else
    {
        if ( (*sLen & 0xFF) == SMI_OUT_MODE_LOB )
        {
            // PROJ-1862 In Mode Lob
            // Column Prefix가 Out Mode LOB인 경우
            if ( aIsOutModeLob != NULL )
            {
                *aIsOutModeLob = ID_TRUE;
            }

            *aColumnLen = ID_SIZEOF(sdcLobDesc);
        }
        else
        {
            if ( ((UShort)*sLen & 0xFF) == SMI_NULL_VALUE )
            {
                if ( aIsNull != NULL )
                {
                    *aIsNull = ID_TRUE;
                }

                *aColumnLen = 0;
            }
            else
            {
                IDE_DASSERT( ((UShort)*sLen & 0xFF) <= SMI_SHORT_VALUE );
                *aColumnLen = ((UShort)*sLen & 0xFF);
            }
        }
    }

    return;
}

IDE_RC smiLogRec::readFrom( void  * aLogHeadPtr,
                            void  * aLogPtr,
                            smLSN * aLSN )
{
    smrLogHead    *sCommonHdr;

    static SInt sMMDBLogSize = ID_SIZEOF(smrUpdateLog) - ID_SIZEOF(smiLogHdr);
    static SInt sDRDBLogSize = ID_SIZEOF(smrDiskLog) - ID_SIZEOF(smiLogHdr);
    static SInt sLobLogSize = ID_SIZEOF(smrLobLog) - ID_SIZEOF(smiLogHdr);
    // Table Meta Log Record
    static SInt sTableMetaLogSize = ID_SIZEOF(smrTableMetaLog) - ID_SIZEOF(smiLogHdr);

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aLSN != NULL );
    IDE_DASSERT( aLogHeadPtr != NULL );

    ACP_UNUSED( aLSN );

    mLogPtr           = (SChar *)aLogPtr;
    sCommonHdr        = (smrLogHead *)aLogHeadPtr;
    mLogUnion.mCommon = sCommonHdr;
    mRecordLSN        = smrLogHeadI::getLSN( sCommonHdr );

#ifdef DEBUG
    if ( !SM_IS_LSN_INIT ( *aLSN ) )
    {
        IDE_DASSERT( smrCompareLSN::isEQ( &mRecordLSN, aLSN ) ); 
    }
#endif

    /* BUG-35392 */
    if ( smrLogHeadI::isDummyLog( sCommonHdr ) == ID_FALSE )
    {   /* Dummy Log 가 아닌 일반 로그 */

        switch(smrLogHeadI::getType(sCommonHdr))
        {
            case SMR_LT_MEMTRANS_COMMIT:
            case SMR_LT_DSKTRANS_COMMIT:
                mLogType = SMI_LT_TRANS_COMMIT;

                idlOS::memcpy( (SChar*)&( mLogUnion.mCommitLog ) + ID_SIZEOF(smiLogHdr),
                               mLogPtr + ID_SIZEOF(smiLogHdr), 
                               ID_SIZEOF(smrTransCommitLog) );
                break;

            case SMR_LT_MEMTRANS_GROUPCOMMIT:
                mLogType = SMI_LT_TRANS_GROUPCOMMIT;
                break;

            case SMR_LT_MEMTRANS_ABORT:
            case SMR_LT_DSKTRANS_ABORT:
                mLogType = SMI_LT_TRANS_ABORT;
                break;

            case SMR_LT_TRANS_PREABORT:
                mLogType = SMI_LT_TRANS_PREABORT;
                break;

            case SMR_LT_SAVEPOINT_SET:
                mLogType = SMI_LT_SAVEPOINT_SET;
                break;

            case SMR_LT_SAVEPOINT_ABORT:
                mLogType = SMI_LT_SAVEPOINT_ABORT;
                break;

            case SMR_LT_FILE_END:
                mLogType = SMI_LT_FILE_END;
                break;

            case SMR_LT_UPDATE:
                idlOS::memcpy( (SChar*)&(mLogUnion.mMemUpdate) + ID_SIZEOF(smiLogHdr),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               sMMDBLogSize );

                mLogType = SMI_LT_MEMORY_CHANGE;

                switch(mLogUnion.mMemUpdate.mType)
                {
                    case SMR_SMC_PERS_INSERT_ROW :
                        mChangeType = SMI_CHANGE_MRDB_INSERT;
                        mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrUpdateLog);
                        break;

                    case SMR_SMC_PERS_UPDATE_INPLACE_ROW :
                    case SMR_SMC_PERS_UPDATE_VERSION_ROW :
                        mChangeType = SMI_CHANGE_MRDB_UPDATE;
                        mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrUpdateLog);
                        break;

                    case SMR_SMC_PERS_DELETE_VERSION_ROW :
                        mChangeType = SMI_CHANGE_MRDB_DELETE;
                        mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrUpdateLog);
                        break;

                    case SMR_SMC_PERS_WRITE_LOB_PIECE :
                        mChangeType = SMI_CHANGE_MRDB_LOB_PARTIAL_WRITE;
                        mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrUpdateLog);
                        break;

                    default:
                        mLogType = SMI_LT_NULL;
                        break;
                }

                break;
                /*
                 * PROJ-1705 : 로그 구조 (Outline)
                 *------------------------------------------------------------
                 * |<-- mLogPtr
                 *              |<--->| mRefOffset
                 *              (page alloc 등의 RP에서 안보는 로그들을
                 *               skip하기 위함)
                 *                    |<--RP에서 필요로하는 DML로그의 시작
                 * -------------------------------------------------
                 * | smrDiskLog |      Log Body      |   LogTail   |
                 * -------------------------------------------------
                 */
            case SMR_DLT_REDOONLY:
            case SMR_DLT_UNDOABLE:
                idlOS::memcpy((SChar*)&(mLogUnion.mDisk) + ID_SIZEOF(smiLogHdr),
                              mLogPtr + ID_SIZEOF(smiLogHdr),
                              sDRDBLogSize);
                mLogType = SMI_LT_DISK_CHANGE;

                setChangeLogType(mLogPtr +
                                 SMR_LOGREC_SIZE(smrDiskLog) +
                                 mLogUnion.mDisk.mRefOffset);
                break;

            case SMR_LT_LOB_FOR_REPL:
                idlOS::memcpy( (SChar*)&(mLogUnion.mLob) + ID_SIZEOF(smiLogHdr),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               sLobLogSize );
                mLogType = SMI_LT_LOB_FOR_REPL;

                switch(mLogUnion.mLob.mOpType)
                {
                    case SMR_MEM_LOB_CURSOR_OPEN:
                        {
                            mChangeType = SMI_CHANGE_MRDB_LOB_CURSOR_OPEN;
                            mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrLobLog);
                            break;
                        }
                    case SMR_DISK_LOB_CURSOR_OPEN:
                        {
                            mChangeType = SMI_CHANGE_DRDB_LOB_CURSOR_OPEN;
                            mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrLobLog);
                            break;
                        }
                    case SMR_LOB_CURSOR_CLOSE:
                        {
                            mChangeType = SMI_CHANGE_LOB_CURSOR_CLOSE;
                            mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrLobLog);
                            break;
                        }
                    case SMR_PREPARE4WRITE:
                        {
                            mChangeType = SMI_CHANGE_LOB_PREPARE4WRITE;
                            mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrLobLog);
                            break;
                        }
                    case SMR_FINISH2WRITE:
                        {
                            mChangeType = SMI_CHANGE_LOB_FINISH2WRITE;
                            mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrLobLog);
                            break;
                        }
                    case SMR_LOB_TRIM:
                        {
                            mChangeType = SMI_CHANGE_LOB_TRIM;
                            mAnalyzeStartPtr = mLogPtr + SMR_LOGREC_SIZE(smrLobLog);
                            break;
                        }
                    default:
                        {
                            mLogType = SMI_LT_NULL;
                            break;
                        }
                }
                break;

            case SMR_LT_DDL :           // DDL Transaction임을 표시하는 Log Record
                mLogType = SMI_LT_DDL;
                break;

            case SMR_LT_TABLE_META :    // Table Meta Log Record
                idlOS::memcpy( (SChar*)&(mLogUnion.mTableMetaLog) + ID_SIZEOF(smiLogHdr),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               sTableMetaLogSize );
                mLogType = SMI_LT_TABLE_META;
                break;

            case SMR_LT_DDL_QUERY_STRING :
                idlOS::memcpy( (SChar*)&( mLogUnion.mDDLStmtMeta ),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               ID_SIZEOF(smrDDLStmtMeta) );
                mLogType = SMI_LT_DDL_QUERY_STRING;
                break;

            /* PROJ-2747 Global Tx Consistency */
            case SMR_LT_XA_START_REQ :
                idlOS::memcpy( (SChar*)&( mLogUnion.mXaLog ),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               ID_SIZEOF(smrXaLog) );
                mLogType = SMI_LT_XA_START_REQ;
                break;

            case SMR_LT_XA_PREPARE_REQ :
                idlOS::memcpy( (SChar*)&( mLogUnion.mXaLog ),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               ID_SIZEOF(smrXaLog) );
                mLogType = SMI_LT_XA_PREPARE_REQ;
                break;

            case SMR_LT_XA_PREPARE :
                idlOS::memcpy( (SChar*)&( mLogUnion.mXaLog ),
                               mLogPtr + ID_SIZEOF(smiLogHdr),
                               ID_SIZEOF(smrXaLog) );
                mLogType = SMI_LT_XA_PREPARE;
                break;

            case SMR_LT_XA_END :
                mLogType = SMI_LT_XA_END;
                break;

            default:
                mLogType = SMI_LT_NULL;
                break;
        }
    }
    else
    {   /* Dummy Log 일 때 */
        mLogType = SMI_LT_NULL;
    }

    return IDE_SUCCESS;
}

/***************************************************************************
 * PROJ-1705 [레코드 구조 최적화]
 *  :  Disk Table의 DML 로그 타입을 RP모듈에서 사용 할 수 있도록
 *     smi 로그 타입으로 변환시킨다.
 *
 *----------------------------------------------------------------------
 * [DML]    [sdrLogType]                         [smiChangeLogType]
 *----------------------------------------------------------------------
 * INSERT   SDR_SDC_INSERT_ROW_PIECE             SMI_REDO_DRDB_INSERT
 *----------------------------------------------------------------------
 * DELETE   SDR_SDC_DELETE_ROW_PIECE             SMI_REDO_DRDB_DELETE
 *----------------------------------------------------------------------
 * UPDATE   SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE  SMI_REDO_DRDB_UPDATE_INSERT_ROW_PIECE
 *          SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE  SMI_REDO_DRDB_UPDATE_DELETE_ROW_PIECE
 *          SDR_SDC_UPDATE_ROW_PIECE             SMI_REDO_DRDB_UPDATE
 *          SDR_SDC_OVERWRITE_ROW_PIECE          SMI_REDO_DRDB_UPDATE_OVERWRITE
 *          SDR_SDC_DELETE_FIRST_COLUMN_PIECE    SMI_REDO_DRDB_UPDATE_DELETE_FIRST_COLUMN
 *
 *----------------------------------------------------------------------
 * [DML]    [sdcUndoRecHdr.mRecType]             [smiChangeLogType]
 *----------------------------------------------------------------------
 * INSERT   SDC_UNDO_INSERT_ROW_PIECE            SMI_UNDO_DRDB_INSERT
 *----------------------------------------------------------------------
 * DELETE   SDC_UNDO_DELETE_ROW_PIECE            SMI_UNDO_DRDB_DELETE
 *----------------------------------------------------------------------
 * UPDATE   SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE SMI_UNDO_DRDB_UPDATE_INSERT_ROW_PIECE
 *          SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE
 *          SDC_UNDO_UPDATE_ROW_PIECE            SMI_UNDO_DRDB_UPDATE
 *          SDC_UNDO_OVERWRITE_ROW_PIECE         SMI_UNDO_DRDB_UPDATE_OVERWRITE
 *          SDC_UNDO_DELETE_FIRST_COLUMN_PIECE   SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN
 *
 ****************************************************************************
 *
 * REDO와 UNDO로그의 log type 위치가 다르다.
 * + REDO의 경우 : sdrLogHdr.mType에 RP에서 분류하는 로그 타입이 존재한다.
 * + UNDO의 경우 : sdrLogHdr.mType에 SDR_SDC_INSERT_UNDO_REC 로그 타입으로 남으며,
 *                  이때에는 바로 뒤에 이어지는 sdcUndoRecHdr의 type을 읽어 이로써
 *                  구별해야한다. 처음설계에서는 sdrLogHdr뒤에 바로 RP info 가
 *                  왔지만, 로그타입을 연속적으로 읽어야 하므로, RP info 를
 *                  맨 뒤로 보내버렸다.
 ****************************************************************************/

void smiLogRec::setChangeLogType(void* aLogHdr)
{
    sdrLogHdr      sLogHdr;
    sdcUndoRecType sUndoRecType;

    IDE_DASSERT( aLogHdr != NULL );

    idlOS::memcpy(&sLogHdr, aLogHdr, (UInt)ID_SIZEOF(sdrLogHdr));
    mAnalyzeStartPtr = (SChar*)aLogHdr;

    if ( sLogHdr.mType == SDR_SDC_INSERT_UNDO_REC )
    {
        // undo log type을 읽기 위해서는 sdrLogHdr를 건너뛴 후, undo log 의 size(UShort)를
        // skip하고 읽는다.
        idlOS::memcpy( &sUndoRecType,
                       (UChar*)aLogHdr + ID_SIZEOF(sdrLogHdr) + ID_SIZEOF(UShort),
                       ID_SIZEOF(UChar) );

        switch(sUndoRecType)
        {
            case SDC_UNDO_INSERT_ROW_PIECE :
                mChangeType = SMI_UNDO_DRDB_INSERT;
                break;

            case SDC_UNDO_DELETE_ROW_PIECE :
                mChangeType = SMI_UNDO_DRDB_DELETE;
                break;

            case SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE :
                mChangeType = SMI_UNDO_DRDB_UPDATE_INSERT_ROW_PIECE;
                break;

            case SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE :
                mChangeType = SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE;
                break;

            case SDC_UNDO_UPDATE_ROW_PIECE :
                mChangeType = SMI_UNDO_DRDB_UPDATE;
                break;

            case SDC_UNDO_OVERWRITE_ROW_PIECE :
                mChangeType = SMI_UNDO_DRDB_UPDATE_OVERWRITE;
                break;

            case SDC_UNDO_DELETE_FIRST_COLUMN_PIECE :
                mChangeType = SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN;
                break;

            default :
                mLogType = SMI_LT_NULL;
                mChangeType = SMI_CHANGE_NULL;
                break;
        }
    }
    else
    {
        switch(sLogHdr.mType)
        {
            case SDR_SDC_PK_LOG :
                mChangeType = SMI_PK_DRDB;
                break;

            case SDR_SDC_INSERT_ROW_PIECE :
            case SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO :
                mChangeType = SMI_REDO_DRDB_INSERT;
                break;

            case SDR_SDC_DELETE_ROW_PIECE :
                mChangeType = SMI_REDO_DRDB_DELETE;
                break;

            case SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE :
                mChangeType = SMI_REDO_DRDB_UPDATE_INSERT_ROW_PIECE;
                break;

            case SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE :
                mChangeType = SMI_REDO_DRDB_UPDATE_DELETE_ROW_PIECE;
                break;

            case SDR_SDC_UPDATE_ROW_PIECE :
                mChangeType = SMI_REDO_DRDB_UPDATE;
                break;

            case SDR_SDC_OVERWRITE_ROW_PIECE :
                mChangeType = SMI_REDO_DRDB_UPDATE_OVERWRITE;
                break;

            case SDR_SDC_DELETE_FIRST_COLUMN_PIECE :
                mChangeType = SMI_REDO_DRDB_UPDATE_DELETE_FIRST_COLUMN;
                break;

            case SDR_SDC_LOB_WRITE_PIECE4DML :  // LOB Write for DML
                mChangeType = SMI_CHANGE_DRDB_LOB_PIECE_WRITE;
                SC_COPY_GRID(sLogHdr.mGRID, mRecordGRID );
                break;
                
            case SDR_SDC_LOB_WRITE_PIECE :      // LOB Piece Write
                mChangeType = SMI_CHANGE_DRDB_LOB_PARTIAL_WRITE;
                SC_COPY_GRID(sLogHdr.mGRID, mRecordGRID );
                break;
                
            case SDR_SDC_LOCK_ROW : // row lock , bug-34581
                mChangeType = SMI_CHANGE_DRDB_LOCK_ROW;
                break;

            default :
                mLogType = SMI_LT_NULL;
                mChangeType = SMI_CHANGE_NULL;
                break;
        }
    }
    return;
}

/* _________________________________________________________________________________________
 * | Fixed area | Fixed | United       | Large            |   LOB     | OldRowOID | NextOID |
 * |____size____|_area__|_var columns__|_variable columns_|___________|___________|_________|
 */
IDE_RC smiLogRec::analyzeInsertLogMemory( smiLogRec  *aLogRec,
                                          UShort     *aColCnt,
                                          UInt       *aCIDArray,
                                          smiValue   *aAColValueArray,
                                          idBool     *aDoWait )
{
    const void * sTable = NULL;

    SChar *sAfterImagePtr;
    SChar *sAfterImagePtrFence;

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aColCnt != NULL );
    IDE_DASSERT( aCIDArray != NULL );
    IDE_DASSERT( aAColValueArray != NULL );
    IDE_DASSERT( aDoWait != NULL );

    IDE_ASSERT( aLogRec->mGetTable( aLogRec->mMeta,
                                    (ULong)aLogRec->getTableOID(),
                                    &sTable)
                == IDE_SUCCESS );

    // 메모리 로그는 언제나 하나의 로그로 쓰이므로, *aDoWait이 ID_FALSE가 된다.
    *aDoWait = ID_FALSE;

    sAfterImagePtr = aLogRec->getLogPtr() + SMR_LOGREC_SIZE(smrUpdateLog);

    /* TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log
     *                      can be reduced to 1.
     * 로그 마지막에 OldVersion RowOID가 있기 때문에 Fence에 이를 제외해준다. */
    sAfterImagePtrFence = aLogRec->getLogPtr() +
                          + aLogRec->getLogSize()
                          - ID_SIZEOF(ULong)
                          - smiLogRec::getLogTailSize();
    
    IDE_TEST( analyzeInsertLogAfterImageMemory( aLogRec,
                                                sAfterImagePtr,
                                                sAfterImagePtrFence,
                                                aColCnt,
                                                aCIDArray,
                                                aAColValueArray,
                                                sTable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiLogRec::analyzeInsertLogAfterImageMemory( smiLogRec  *aLogRec,
                                                    SChar      *aAfterImagePtr,
                                                    SChar      *aAfterImagePtrFence,
                                                    UShort     *aColCount,
                                                    UInt       *aCidArray,
                                                    smiValue   *aAfterColsArray,
                                                    const void *aTable )
{
    const smiColumn     * spCol             = NULL;

    SChar               * sFixedAreaPtr     = NULL;
    SChar               * sVarAreaPtr       = NULL;
    SChar               * sVarColPtr        = NULL;
    SChar               * sVarCIDPtr        = NULL;
    UShort                sFixedAreaSize;
    UInt                  sCID;
    UInt                  sAfterColSize;
    UInt                  i;
    UInt                  sOIDCnt;
    smVCDesc              sVCDesc;
    SChar                 sErrorBuffer[256];
    smOID                 sVCPieceOID           = SM_NULL_OID;
    UShort                sVarColCount          = 0;
    UShort                sVarColCountInPiece   = 0;
    UShort                sCurrVarOffset        = 0;
    UShort                sNextVarOffset        = 0;

    /*
      After Image : Fixed Row와 Variable Column에 대한 Log를 기록.
      Fixed Row Size(UShort) + Fixed Row Data
      + VCLOG(1) + VCLOG(2) ... + VCLOG (n)

      VCLOG : Variable Column당 하나씩 생긴다.
      1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SM_VCDESC_MODE_OUT
      - Column ID(UInt) | Length(UInt) | Value | OID Cnt(UInt) | OID List |

      2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SM_VCDESC_MODE_IN
      - None (After Image인 경우는 Fixed Row에 In Mode로 저장되고
      또한 Fixed Row에 대한 Logging을 별도로 수행하기 때문에
      VC에 대한 Logging이 불필요.
    */

    sFixedAreaPtr = aAfterImagePtr + SMI_LOGREC_MV_FIXED_ROW_DATA_OFFSET;

    /* Fixed Row의 길이:UShort */
    sFixedAreaSize = aLogRec->getUShortValue( aAfterImagePtr, SMI_LOGREC_MV_FIXED_ROW_SIZE_OFFSET );
    IDE_TEST_RAISE( sFixedAreaSize > SM_PAGE_SIZE,
                    err_too_big_fixed_area_size );

    sVarAreaPtr   = aAfterImagePtr + SMI_LOGREC_MV_FIXED_ROW_DATA_OFFSET + sFixedAreaSize;

    *aColCount = (UShort) aLogRec->mGetColumnCount(aTable);

    /* extract fixed fields */
    for (i=0; i < *aColCount; i++ )
    {
        spCol = aLogRec->mGetColumn(aTable, i);

        sCID = spCol->id & SMI_COLUMN_ID_MASK;
        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

        if ( (spCol->flag & SMI_COLUMN_COMPRESSION_MASK ) == SMI_COLUMN_COMPRESSION_FALSE )
        {
            if ( (spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED )
            {
                aCidArray[sCID] = sCID;
                aAfterColsArray[sCID].length = spCol->size;
                aAfterColsArray[sCID].value  =
                    sFixedAreaPtr + (spCol->offset - smiGetRowHeaderSize(SMI_TABLE_MEMORY));
            }
            else /* large var + lob */
            {
                if ( (spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE )
                {
                    /* Nothing to do
                     * united var 는 fixed 영역에서 아무것도 기록하지 않았다 */
                }
                else
                {
                    smiLogRec::getVCDescInAftImg(spCol, aAfterImagePtr, &sVCDesc);
                    if ( (sVCDesc.flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_IN )
                    {
                        aCidArray[sCID] = sCID;
                        
                        if ( ( SMI_IS_LOB_COLUMN(spCol->flag) == ID_TRUE ) && ( sVCDesc.length != 0 ) )
                        {
                            aAfterColsArray[sCID].length = sVCDesc.length + SMI_LOB_DUMMY_HEADER_LEN;
                            aAfterColsArray[sCID].value  =
                                sFixedAreaPtr + (spCol->offset - smiGetRowHeaderSize(SMI_TABLE_MEMORY))
                                + ID_SIZEOF( smVCDescInMode ) - SMI_LOB_DUMMY_HEADER_LEN;
                        }
                        else
                        {
                            aAfterColsArray[sCID].length = sVCDesc.length;
                            aAfterColsArray[sCID].value  =
                                sFixedAreaPtr + (spCol->offset - smiGetRowHeaderSize(SMI_TABLE_MEMORY))
                                + ID_SIZEOF( smVCDescInMode );
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
        }
        else /* PROJ-2397 Compression Column Replication SMI_COLUMN_TYPE_FIXED */
        {
            aCidArray[sCID] = sCID;
            aAfterColsArray[sCID].length = ID_SIZEOF(smOID);
            aAfterColsArray[sCID].value =
                    (smOID *)( sFixedAreaPtr + ( spCol->offset - smiGetRowHeaderSize( SMI_TABLE_MEMORY ) ) );
        }
    }

    /* extract variabel fields */
    sVCPieceOID = aLogRec->getvULongValue( sVarAreaPtr );
    sVarAreaPtr += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        sVarColCount = aLogRec->getUShortValue (sVarAreaPtr );
        sVarAreaPtr += ID_SIZEOF(UShort);

        sVarCIDPtr   = sVarAreaPtr;
        sVarAreaPtr += ID_SIZEOF(UInt) * sVarColCount;

        if ( sVarColCount > 0 )
        {
            /* next united var piece 를 찾아서 반복해야한다. */
            while ( (sVCPieceOID != SM_NULL_OID) )
            {
                /* united var piece 당 한번만 한다 */
                sVCPieceOID  = aLogRec->getvULongValue( sVarAreaPtr );
                sVarAreaPtr += ID_SIZEOF(smOID);

                sVarColCountInPiece  = aLogRec->getUShortValue ( sVarAreaPtr );
                sVarAreaPtr         += ID_SIZEOF(UShort);

                IDE_DASSERT( sVarColCountInPiece < SMI_COLUMN_ID_MAXIMUM );

                /* var piece 내의 column을 추출한다 */
                for ( i = 0 ; i < sVarColCountInPiece ; i++ )
                {
                    /* Column ID 는 앞에 따로있으므로 별도의 offset 을 적용한다 */
                    sCID = aLogRec->getUIntValue( sVarCIDPtr, SMI_LOGREC_MV_COLUMN_CID_OFFSET );

                    sVarCIDPtr += ID_SIZEOF(UInt);

                    if ( sCID == ID_UINT_MAX )
                    {
                        /* United var 중 rp 에서는 필요없는 컬럼 */
                        sNextVarOffset = aLogRec->getUShortValue( sVarAreaPtr, 
                                                                  (UInt)( ID_SIZEOF(UShort) * ( i + 1 ) ) );
                    }
                    else
                    {
                        sCID &= SMI_COLUMN_ID_MASK;
                        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

                        spCol = aLogRec->mGetColumn( aTable, sCID );

                        IDE_TEST_RAISE((spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED,
                                err_fixed_col_in_united_var_area);

                        aCidArray[sCID] = sCID;

                        /* value 의 앞부분에 있는 offset array에서 offset을 읽어온다 */
                        sCurrVarOffset = aLogRec->getUShortValue( sVarAreaPtr,
                                                                  (UInt)( ID_SIZEOF(UShort) * i ) );
                        sNextVarOffset = aLogRec->getUShortValue( sVarAreaPtr, 
                                                                  (UInt)( ID_SIZEOF(UShort) * ( i + 1 )));

                        if ( (UInt)( sNextVarOffset - sCurrVarOffset ) <= spCol->size )
                        {
                            aAfterColsArray[sCID].length = sNextVarOffset - sCurrVarOffset;
                        }
                        else
                        {
                            aAfterColsArray[sCID].length = spCol->size;
                        }

                        if ( sNextVarOffset == sCurrVarOffset )
                        {
                            aAfterColsArray[sCID].value = NULL;
                        }
                        else
                        {
                            aAfterColsArray[sCID].value = sVarAreaPtr + sCurrVarOffset - ID_SIZEOF(smVCPieceHeader);
                        }
                        
                    }
                }
                /* next piece 로 offset 이동한다 */
                sVarAreaPtr += sNextVarOffset - ID_SIZEOF(smVCPieceHeader);
            }
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

    /* extract Large variable fields & LOB fields */
    for (sVarColPtr = sVarAreaPtr; sVarColPtr < aAfterImagePtrFence; )
    {
        sCID = aLogRec->getUIntValue( sVarColPtr, SMI_LOGREC_MV_COLUMN_CID_OFFSET )
            & SMI_COLUMN_ID_MASK ;
        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

        spCol = aLogRec->mGetColumn(aTable, sCID);
        IDE_TEST_RAISE((spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED,
                       err_fixed_col_in_var_area);

        aCidArray[sCID] = sCID;

        sAfterColSize = aLogRec->getUIntValue(sVarColPtr, SMI_LOGREC_MV_COLUMN_SIZE_OFFSET);
        if ( ( SMI_IS_LOB_COLUMN(spCol->flag) == ID_TRUE ) && ( sAfterColSize != 0 ) )
        {
            aAfterColsArray[sCID].length = sAfterColSize + SMI_LOB_DUMMY_HEADER_LEN;
            aAfterColsArray[sCID].value  = sVarColPtr + SMI_LOGREC_MV_COLUMN_DATA_OFFSET - SMI_LOB_DUMMY_HEADER_LEN;
        }
        else
        {
            aAfterColsArray[sCID].length = sAfterColSize;
            aAfterColsArray[sCID].value  = sVarColPtr + SMI_LOGREC_MV_COLUMN_DATA_OFFSET;
        }
        sVarColPtr += SMI_LOGREC_MV_COLUMN_DATA_OFFSET + sAfterColSize;

        /* Variable/LOB Column Value 인 경우에는 OID List를 건너뛰어야 함 */
        /* 여기에는 OID count가 저장되어 있으므로, Count를 읽은 후 그 수 만큼 */
        /* 건너뛰도록 한다. */
        sOIDCnt = aLogRec->getUIntValue(sVarColPtr, 0);
        sVarColPtr += ID_SIZEOF(UInt) + (sOIDCnt * ID_SIZEOF(smOID));
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_fixed_col_in_united_var_area);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageMemory] "
                         "Fixed Column [CID:%"ID_UINT32_FMT"] is in United Variable Area at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT"]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG, sErrorBuffer ) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(err_fixed_col_in_var_area);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageMemory] "
                         "Fixed Column [CID:%"ID_UINT32_FMT"] is in Variable Area at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sCID,
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(ERR_TOO_LARGE_CID);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageMemory] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(err_too_big_fixed_area_size);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageMemory] "
                         "Fixed Area Size [%"ID_UINT32_FMT"] is over page size [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT"]",
                         sFixedAreaSize, 
                         SM_PAGE_SIZE,
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiLogRec::analyzeUpdateLogMemory( smiLogRec  *aLogRec,
                                          UInt       *aPKColCnt,
                                          UInt       *aPKCIDArray,
                                          smiValue   *aPKColValueArray,
                                          UShort     *aColCnt,
                                          UInt       *aCIDArray,
                                          smiValue   *aBColValueArray,
                                          smiValue   *aAColValueArray,
                                          idBool     *aDoWait )
{
    SChar    *sBeforeImagePtr;
    SChar    *sBeforeImagePtrFence;
    SChar    *sAfterImagePtr;
    SChar    *sAfterImagePtrFence;
    SChar    *sPkImagePtr;

    UInt     sPKColSize;
    UShort   sAfterColCount;
    UInt     sAfterCids[ SMI_COLUMN_ID_MAXIMUM ];
    SChar    sErrorBuffer[256];

    UInt     i;

    const void * sTable = NULL;

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aPKColCnt != NULL );
    IDE_DASSERT( aPKCIDArray != NULL );
    IDE_DASSERT( aPKColValueArray != NULL );
    IDE_DASSERT( aColCnt != NULL );
    IDE_DASSERT( aCIDArray != NULL );
    IDE_DASSERT(aBColValueArray != NULL );
    IDE_DASSERT( aAColValueArray != NULL );
    IDE_DASSERT( aDoWait != NULL );

    IDE_ASSERT( aLogRec->mGetTable( aLogRec->mMeta,
                                        (ULong)aLogRec->getTableOID(),
                                        &sTable)
                   == IDE_SUCCESS );

    // 메모리 로그는 언제나 하나의 로그로 쓰이므로, *aDoWait이 ID_FALSE가 된다.
    *aDoWait = ID_FALSE;

    // fix PR-3409 PR-3556
    for(i=0 ; i < SMI_COLUMN_ID_MAXIMUM ; i++)
    {
        // 0으로 초기화하면 안 될듯? 0도 CID로 사용되는 값임
        // MAX로 추기화하는 것에 대해서 생각해보자 - by mycomman, review
        sAfterCids[i] = 0;
    }

    sBeforeImagePtr = aLogRec->getLogPtr() + SMR_LOGREC_SIZE(smrUpdateLog);
    sBeforeImagePtrFence = sBeforeImagePtr + aLogRec->getBfrImgSize();

    sAfterImagePtr = sBeforeImagePtrFence;
    sAfterImagePtrFence  = sAfterImagePtr + aLogRec->getAftImgSize();

    sPkImagePtr = sAfterImagePtrFence;

    if ( aLogRec->getLogUpdateType() == SMR_SMC_PERS_UPDATE_INPLACE_ROW )
    {
        /* BUG-47366 UpdateInplace Log의 Before Image 끝에 
         * BeforeImage의 Size를 나타내는 UInt의 Log가 추가되었다. */
        sBeforeImagePtrFence -= ID_SIZEOF(UInt);

        /* sModifyIdxBit BUG-47632,47615 */
        sBeforeImagePtr += ID_SIZEOF(ULong);

        IDE_TEST(analyzeColumnUIImageMemory(aLogRec,
                                            sBeforeImagePtr,
                                            sBeforeImagePtrFence,
                                            aColCnt,
                                            aCIDArray,
                                            aBColValueArray,
                                            sTable,
                                            ID_TRUE/* Before Image */)
                 != IDE_SUCCESS );

        IDE_TEST(analyzeColumnUIImageMemory(aLogRec,
                                            sAfterImagePtr,
                                            sAfterImagePtrFence,
                                            &sAfterColCount,
                                            aCIDArray,
                                            aAColValueArray,
                                            sTable,
                                            ID_FALSE/* After Image */)
                 != IDE_SUCCESS );

        /* Before image와 After image에 있는 Update column count가 항상
           같아야만 한다. */
        //BUG-43744 : trailing NULL 을/으로 업데이트 하면 다를 수 있다.
    }
    else
    {
        if ( aLogRec->getLogUpdateType() == SMR_SMC_PERS_UPDATE_VERSION_ROW )
        {
            /* TASK-4690, BUG-32319
             * [sm-mem-collection] The number of MMDB update log can be
             * reduced to 1.
             *
             * Before 이미지 마지막에 OldVersion RowOID와 lock row 여부가 있고
             * After 이미지 마지막에 OldVersion RowOID가 있으므로
             * Fence에 이를 제외해준다. */
            sBeforeImagePtrFence = sBeforeImagePtrFence - ID_SIZEOF(ULong) - ID_SIZEOF(idBool);
            sAfterImagePtrFence  = sAfterImagePtrFence - ID_SIZEOF(ULong);

            IDE_TEST( analyzeColumnMVImageMemory( aLogRec,
                                                  sBeforeImagePtr,
                                                  sBeforeImagePtrFence,
                                                  aColCnt,
                                                  aCIDArray,
                                                  aBColValueArray,
                                                  sTable )
                      != IDE_SUCCESS );

            IDE_TEST( analyzeInsertLogAfterImageMemory( aLogRec,
                                                        sAfterImagePtr,
                                                        sAfterImagePtrFence,
                                                        &sAfterColCount,
                                                        sAfterCids,
                                                        aAColValueArray,
                                                        sTable )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE(err_no_matching_update_type);
        }
    }

    IDE_TEST( analyzePKMem( aLogRec,
                            sPkImagePtr,
                            aPKColCnt,
                            &sPKColSize,
                            aPKCIDArray,
                            aPKColValueArray,
                            sTable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_no_matching_update_type);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeUpdateLogMemory] No matching update type [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT"]",
                         aLogRec->getLogUpdateType(), 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *      Befor  Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt)  |
 *                        ColumnID(UInt) | SIZE(UInt) | Value
 *
 *         Var   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SM_VCDESC_MODE_OUT:
 *                        | Value | OID
 *               SM_VCDESC_MODE_IN:
 *                        | Value
 *         LOB   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *                        | Piece Count(UInt) | firstLPCH * | Value | OID
 *
 *      After  Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *                        | Value
 *
 *         Var/LOB Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SM_VCDESC_MODE_OUT, LOB:
 *                        | Value | OID Count | OID 들...
 *               SM_VCDESC_MODE_IN:
 *                        | Value
 *
 */
IDE_RC smiLogRec::analyzeColumnUIImageMemory( smiLogRec  * aLogRec,
                                              SChar      * aColImagePtr,
                                              SChar      * aColImagePtrFence,
                                              UShort     * aColCount,
                                              UInt       * aCidArray,
                                              smiValue   * aColsArray,
                                              const void * aTable,
                                              idBool       aIsBefore)
{
    SChar  sFlag;
    UInt   i        = 0;

    UInt   sDataSize;
    UInt   sCID;
    UShort sCidCount = 0;
    SChar  sErrorBuffer[256];

    SChar *sColPtr;
    UInt   sOIDCnt;

    SChar *sVarAreaPtr          = aColImagePtr;
    SChar *sVarCIDPtr           = NULL;
    SChar *sVarCIDPence         = NULL;     //BUG-43744
    smOID  sVCPieceOID          = SM_NULL_OID;
    UShort sVarColCount         = 0;
    UShort sVarColCountInPiece  = 0;
    UShort sCurrVarOffset       = 0;
    UShort sNextVarOffset       = 0;

    const smiColumn    * spCol  = NULL;

    /* extract variabel fields */
    sVCPieceOID = aLogRec->getvULongValue( sVarAreaPtr );
    sVarAreaPtr += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        sVarColCount = aLogRec->getUShortValue (sVarAreaPtr );
        sVarAreaPtr += ID_SIZEOF(UShort);

        sVarCIDPtr   = sVarAreaPtr;
        sVarAreaPtr += ID_SIZEOF(UInt) * sVarColCount;
        sVarCIDPence = sVarAreaPtr;

        if ( sVarColCount > 0 )
        {
            /* next united var piece 를 찾아서 반복해야한다. */
            while ( (sVCPieceOID != SM_NULL_OID) )
            {
                /* united var piece 당 한번만 한다 */
                sVCPieceOID = aLogRec->getvULongValue( sVarAreaPtr );
                sVarAreaPtr += ID_SIZEOF(smOID);

                sVarColCountInPiece = aLogRec->getUShortValue (sVarAreaPtr );
                sVarAreaPtr         += ID_SIZEOF(UShort);

                /* var piece 내의 column을 추출한다 */
                for ( i = 0 ; i < sVarColCountInPiece ; i++ )
                {
                    /* Column ID 는 앞에 따로있으므로 별도의 offset 을 적용한다 */
                    sCID = aLogRec->getUIntValue( sVarCIDPtr, SMI_LOGREC_MV_COLUMN_CID_OFFSET );

                    sVarCIDPtr += ID_SIZEOF(UInt);

                    if ( sCID == ID_UINT_MAX )
                    {
                        /* United var 중 rp 에서는 필요없는 컬럼 */
                        sNextVarOffset = aLogRec->getUShortValue( sVarAreaPtr, 
                                                                  (UInt)( ID_SIZEOF(UShort) * (i+1) ) );
                    }
                    else
                    {
                        sCID &= SMI_COLUMN_ID_MASK;

                        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

                        spCol = aLogRec->mGetColumn( aTable, sCID );

                        IDE_TEST_RAISE((spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED,
                                       err_fixed_col_in_united_var_area);

                        aCidArray[sCidCount] = sCID;

                        /* value 의 앞부분에 있는 offset array에서 offset을 읽어온다 */
                        sCurrVarOffset = aLogRec->getUShortValue( sVarAreaPtr,
                                                                  (UInt)( ID_SIZEOF(UShort) * i ) );
                        sNextVarOffset = aLogRec->getUShortValue( sVarAreaPtr, 
                                                                  (UInt)( ID_SIZEOF(UShort) * ( i + 1 )));


                        if ( (UInt)( sNextVarOffset - sCurrVarOffset ) <= spCol->size )
                        {
                            aColsArray[sCID].length = sNextVarOffset - sCurrVarOffset;
                        }
                        else
                        {
                            aColsArray[sCID].length = spCol->size;
                        }

                        if ( sNextVarOffset == sCurrVarOffset ) /* length 0 = null value */
                        {
                            aColsArray[sCID].value = NULL;
                        }
                        else
                        {
                            aColsArray[sCID].value  = sVarAreaPtr + sCurrVarOffset - ID_SIZEOF(smVCPieceHeader);
                        }
                        
                        /* BUG-43441 */
                        sCidCount++;
                    }
                }
                /* next piece 로 offset 이동한다 */
                sVarAreaPtr += sNextVarOffset - ID_SIZEOF(smVCPieceHeader);
            }
            
            /* BUG-43744 : before img.인 경우, 트레일링 널이 존재할 때, 업데이트
             * 할 컬럼이 트레일링 널에 속한 컬럼이면 이 컬럼은 위의 while 루프에서
             * 처리하지 못할 수 있다. 각 piece의 컬럼 개수(sVarColCountInPiece)는
             * 트레일링 널을 포함하지 않기 때문이다.
             */
            while ( sVarCIDPtr < sVarCIDPence )
            {
                sCID = aLogRec->getUIntValue( sVarCIDPtr, SMI_LOGREC_MV_COLUMN_CID_OFFSET );

                sCID &= SMI_COLUMN_ID_MASK;

                if ( sCID != ID_UINT_MAX )
                {
                    aCidArray[sCidCount] = sCID;
                    aColsArray[sCID].length = 0;
                    aColsArray[sCID].value = NULL;

                    sVarCIDPtr += ID_SIZEOF(UInt);
                    sCidCount++;
                }
                else
                {
                    sVarCIDPtr += ID_SIZEOF(UInt);
                }
            }
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

    for ( sColPtr = sVarAreaPtr; sColPtr < aColImagePtrFence; )
    {
        /* Get Flag & Skip Flag, Offset*/
        sFlag = aLogRec->getSCharValue(sColPtr, 0);
        sColPtr += (ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/);

        /* Get Column ID */
        sCID = aLogRec->getUIntValue(sColPtr, 0) & SMI_COLUMN_ID_MASK ;
        sColPtr += ID_SIZEOF(UInt);

        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

        /* Get Size */
        sDataSize = aLogRec->getUIntValue(sColPtr, 0);
        sColPtr += ID_SIZEOF(UInt);

        /* LOB Column의 경우, Before image에 Piece count와 firstLPCH ptr이 있음 */
        if ( aIsBefore == ID_TRUE )
        {
            if ( ( SMI_IS_LOB_COLUMN(sFlag) == ID_TRUE ) &&
                 ( sDataSize != 0 ) )
            {
                sColPtr += ( ( ID_SIZEOF(UInt)     /*Piece Count*/
                               + ID_SIZEOF(void *) /*firstLPCH ptr */));
            }
            aCidArray[sCidCount] = sCID;
        }
        else
        {
            /* BUG-29234 before image 분석시에 aCidArray의 값을 설정했으므로,
             * after image분석 시에는 또 넣을 필요가 없다.
             * 단, before image와 after image의 CID가 같은지는 검증한다.
             */
            IDE_DASSERT( aCidArray[sCidCount] == sCID );
        }

        /* Get Value */
        switch ( sFlag & SMI_COLUMN_TYPE_MASK )
        {
            case SMI_COLUMN_TYPE_LOB:
                if ( sDataSize != 0 )
                {
                    aColsArray[sCID].length = sDataSize + SMI_LOB_DUMMY_HEADER_LEN;
                    aColsArray[sCID].value  = sColPtr   - SMI_LOB_DUMMY_HEADER_LEN;
                }
                else
                {
                    aColsArray[sCID].length = 0;
                    aColsArray[sCID].value  = NULL;
                }
                break;

            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                /* fall through */
            case SMI_COLUMN_TYPE_FIXED:
                aColsArray[sCID].length = sDataSize;
                aColsArray[sCID].value  = sColPtr;
                break;

            default:
                break;
        }

        // BUG-37433, BUG-40282
        // before면서 OutMode이면서 LOB 일 때는 Data를 Log에 남기지 않음.
        // smcRecordUpdate::undo_SMC_PERS_UPDATE_INPLACE_ROW() 함수 참조
        if ( ( aIsBefore                == ID_TRUE ) &&
             ( SMI_IS_OUT_MODE(sFlag)   == ID_TRUE ) &&
             ( SMI_IS_LOB_COLUMN(sFlag) == ID_TRUE ))
        {
            // do nothing
        }
        else
        {
            sColPtr += sDataSize;
        }

        if ( aIsBefore == ID_TRUE )
        {
            /* Before image에서 OUT mode value일때는, OID가 있음 */
            if ( (SMI_IS_VARIABLE_LARGE_COLUMN(sFlag) == ID_TRUE ) ||
               ( SMI_IS_LOB_COLUMN(sFlag) == ID_TRUE ) )
            {
                if ( SMI_IS_OUT_MODE(sFlag) == ID_TRUE )
                {
                    sColPtr += ID_SIZEOF(smOID);
                }
            }
        }
        else
        {
            /* After image에서 OUT mode value일때는, OID Count와 OID list가 있고,
             * In mode일때는 OID count, OID list가 없음 */
            if ( ( SMI_IS_VARIABLE_LARGE_COLUMN(sFlag) == ID_TRUE ) ||
                 ( SMI_IS_LOB_COLUMN(sFlag) == ID_TRUE ) )
            {
                if ( SMI_IS_OUT_MODE(sFlag) == ID_TRUE )
                {
                    sOIDCnt = aLogRec->getUIntValue(sColPtr, 0);
                    sColPtr += (sOIDCnt * ID_SIZEOF(smOID));    // OID List
                    sColPtr += ID_SIZEOF(UInt);                 // OID Count
                }
            }
        }

        sCidCount++;
    }

    IDE_TEST_RAISE( sCidCount == 0, err_cid_count_eq_zero );

    *aColCount = (UShort)sCidCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cid_count_eq_zero);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeColumnUIImageMemory] Column Count is 0(zero) at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG, sErrorBuffer ) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(ERR_TOO_LARGE_CID);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeColumnUIImageMemory] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(err_fixed_col_in_united_var_area);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageMemory] "
                         "Fixed Column [CID:%"ID_UINT32_FMT"] is in United Variable Area at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* For MVCC
 *
 *     Before Image : Sender가 읽어서 보내는 로그에 대해서만 기록된다.
 *                    각각의 Update되는 Column에 대해서
 *        Fixed Column : Column ID | SIZE | DATA
 *        Var   Column :
 *            1. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SM_VCDESC_MODE_OUT
 *               - Column ID(UInt) | Length(UInt) | Value
 *
 *            2. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SM_VCDESC_MODE_IN
 *               - Column ID(UInt) | Length(UInt) | Value
 *
 */
IDE_RC smiLogRec::analyzeColumnMVImageMemory( smiLogRec  * aLogRec,
                                              SChar      * aColImagePtr,
                                              SChar      * aColImagePtrFence,
                                              UShort     * aColCount,
                                              UInt       * aCidArray,
                                              smiValue   * aColsArray,
                                              const void * aTable )
{
    UInt  sDataSize;
    UInt  sCID;
    UShort sCidCount = 0;
    SChar sErrorBuffer[256];

    SChar * sColPtr;

    const smiColumn * spCol  = NULL;

    /* BUG-47366 */
    SChar * sBfrImgFence      = NULL;
    smOID   sVCPieceOID       = SM_NULL_OID;
    UShort  sVarColumnCnt     = 0;

    /* BUG-47366 UpdateVersionLog의 BeforeImage의 끝에
     * OldVersion의 UnitedVar OID Array가 추가되었다. */
    sBfrImgFence = aColImagePtrFence;

    sBfrImgFence -= ID_SIZEOF(smOID);
    idlOS::memcpy(&sVCPieceOID, sBfrImgFence, ID_SIZEOF(smOID));

    /* OID가 NULL이면 볼 필요 없다. */
    if ( sVCPieceOID != SM_NULL_OID )
    {
        sBfrImgFence -= ID_SIZEOF(UShort);
        idlOS::memcpy(&sVarColumnCnt, sBfrImgFence, ID_SIZEOF(UShort));

        /* 수정된 VarColumn Cnt가 0이면 Old Version을 그냥 사용하는 것이라 변경된 것이 없다. */
        if ( sVarColumnCnt > 0 )
        {
            while( sVCPieceOID != SM_NULL_OID )
            {
                /* get next piece oid */
                sBfrImgFence -= ID_SIZEOF(smOID);
                idlOS::memcpy( &sVCPieceOID, sBfrImgFence, ID_SIZEOF(smOID) );
            }
        }
    }

    for ( sColPtr = aColImagePtr; sColPtr < sBfrImgFence; )
    {
        /* Get Column ID */
        sCID = aLogRec->getUIntValue(sColPtr, 0) & SMI_COLUMN_ID_MASK ;
        sColPtr += ID_SIZEOF(UInt);

        IDE_TEST_RAISE( sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

        /* Get Size */
        sDataSize = aLogRec->getUIntValue(sColPtr, 0);
        sColPtr += ID_SIZEOF(UInt);

        spCol = aLogRec->mGetColumn( aTable, sCID );

        aCidArray[sCidCount] = sCID;
        aColsArray[sCID].value  = sColPtr;

        if ( sDataSize <= spCol->size )
        {
            aColsArray[sCID].length = sDataSize;
        }
        else
        {
            aColsArray[sCID].length = spCol->size;
        }
        
        sColPtr += sDataSize;
        sCidCount++;
    }

    IDE_TEST_RAISE(sCidCount == 0, err_cid_count_eq_zero);

    *aColCount = sCidCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cid_count_eq_zero);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeColumnMVImageMemory] Column Count is 0(zero) at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL_ARG, sErrorBuffer ) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(ERR_TOO_LARGE_CID);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeColumnMVImageMemory] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiLogRec::analyzeDeleteLogMemory( smiLogRec  *aLogRec,
                                          UInt       *aPKColCnt,
                                          UInt       *aPKCIDArray,
                                          smiValue   *aPKColValueArray,
                                          idBool     *aDoWait,
                                          UShort     *aColCnt,
                                          UInt       *aCIDs,
                                          smiValue   *aBColValueArray )
{
    const void * sTable = NULL;

    SChar     * sPkImagePtr;
    UInt        sPKColSize;
    SChar     * sFXLogPtr;
    SChar       sErrorBuffer[256];

    // Simple argument check code
    IDE_DASSERT( aLogRec          != NULL );
    IDE_DASSERT( aPKColCnt        != NULL );
    IDE_DASSERT( aPKCIDArray      != NULL );
    IDE_DASSERT( aPKColValueArray != NULL );
    IDE_DASSERT( aDoWait          != NULL );
    IDE_DASSERT( aColCnt          != NULL );
    IDE_DASSERT(aBColValueArray  != NULL );

    IDE_ASSERT( aLogRec->mGetTable( aLogRec->mMeta,
                                        (ULong)aLogRec->getTableOID(),
                                        &sTable)
                   == IDE_SUCCESS );

    // 메모리 로그는 언제나 하나의 로그로 쓰이므로, *aDoWait이 ID_FALSE가 된다.
    *aDoWait = ID_FALSE;

    IDE_TEST_RAISE( aLogRec->getLogUpdateType()
                    != SMR_SMC_PERS_DELETE_VERSION_ROW, err_no_matching_delete_type );

    sPkImagePtr = aLogRec->getLogPtr() + SMR_LOGREC_SIZE(smrUpdateLog);

    /* 현재 Main에는 Before Next 와 After Next,총 2개가 포함되어있다.*/
    sPkImagePtr += (ID_SIZEOF(ULong) * 2);

    IDE_TEST( analyzePKMem( aLogRec,
                            sPkImagePtr,
                            aPKColCnt,
                            &sPKColSize,
                            aPKCIDArray,
                            aPKColValueArray,
                            sTable )
              != IDE_SUCCESS );

    /* TASK-5030 */
    if ( aLogRec->needSupplementalLog() == ID_TRUE )
    {
        sFXLogPtr = sPkImagePtr + sPKColSize;

        IDE_TEST( analyzeFullXLogMemory( aLogRec,
                                         sFXLogPtr,
                                         aColCnt,
                                         aCIDs,
                                         aBColValueArray,
                                         sTable )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_no_matching_delete_type);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeDeleteLogMemory] No matching delete type [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         aLogRec->getLogUpdateType(), 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * < sdrLogHdr 구조 >
 *
 * typedef struct sdrLogHdr
 * {
 *     scGRID         mGRID;
 *     sdrLogType     mType;
 *     UInt           mLength;
 * } sdrLogHdr;
 *
 **************************************************************/
IDE_RC smiLogRec::analyzeHeader( smiLogRec * aLogRec,
                                 idBool    * aIsContinue )
{
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aIsContinue != NULL );

    // mContType을 보고 다음에 로그가 이어지를 판단한다.
    *aIsContinue = ( (smrContType)aLogRec->getContType() == SMR_CT_END) ?
                   ID_FALSE : ID_TRUE;

    // Skip sdrLogHdr
    aLogRec->setAnalyzeStartPtr( (SChar *)(aLogRec->getAnalyzeStartPtr() +
                                 ID_SIZEOF(sdrLogHdr)));

    return IDE_SUCCESS;
}

/***************************************************************
 * < RP info  구조 >
 *
 *          |<------- repeat --------->|
 * -------------------------------------
 * |        |        |        | column |
 * | column | column | column | total  |
 * | count  | seq    | id     | length |
 * -------------------------------------
 *
 * UNDO LOG : total length 정보가 없다.
 **************************************************************/
IDE_RC smiLogRec::analyzeRPInfo( smiLogRec * aLogRec, SInt aLogType )
{
    UShort             i;
    SChar            * sAnalyzePtr;
    UShort             sColCntInRPInfo;
    UShort             sColSeq = 0;
    UShort             sTrailingNullColSeq;
    UInt               sColId;
    UInt               sColTotalLen;
    smiChangeLogType   sChangeLogType;

    sAnalyzePtr = aLogRec->getAnalyzeStartPtr();
    sChangeLogType = aLogRec->getChangeType();

    // RP info 가 맨 뒤에 있으므로, 그 위치까지 이동.
    if ( aLogType == SMI_UNDO_LOG )
    {
        sAnalyzePtr = (SChar *) aLogRec->getRPLogStartPtr4Undo(
                                (void *)sAnalyzePtr, sChangeLogType);
    }
    else if ( aLogType == SMI_REDO_LOG )
    {
        sAnalyzePtr = (SChar *) aLogRec->getRPLogStartPtr4Redo(
                                (void *)sAnalyzePtr, sChangeLogType);
    }
    else
    {
        IDE_ASSERT(ID_FALSE);
    }

    // 1. Get Column Count
    sColCntInRPInfo = aLogRec->getUShortValue(sAnalyzePtr);
    aLogRec->setUpdateColCntInRowPiece(sColCntInRPInfo);
    sAnalyzePtr += ID_SIZEOF(UShort);

    for(i=0; i < sColCntInRPInfo; i++)
    {
        sTrailingNullColSeq = sColSeq;
        // 2. Get column sequence
        sColSeq = aLogRec->getUShortValue(sAnalyzePtr);
        if ( sColSeq == ID_USHORT_MAX )
        {
            // trailing null을 업데이트 하는 경우의 before image이며,
            // overwrite update의 경우에만 발생하며, 겹치는 일이 없도록 순차적인 seq를 주도록한다.
            // IDE_DASSERT(aLogRec->getChangeType() == SMI_UNDO_DRDB_UPDATE_OVERWRITE);
            // BUGBUG - delete row piece for update 여기로 오는 경우가 있다. 확인필요. 동작에 이상은 없음.
            sColSeq = sTrailingNullColSeq + 1;
        }
        aLogRec->setColumnSequence(sColSeq, i);  // seq는 배열에 순차적으로 들어간다.
        sAnalyzePtr += ID_SIZEOF(UShort);

        // 3. Get column id
        sColId = aLogRec->getUIntValue(sAnalyzePtr);
        aLogRec->setColumnId(sColId, sColSeq);   // id는 배열의 seq위치에 들어간다.
        sAnalyzePtr += ID_SIZEOF(UInt);

        // 4. Get column total length
        if ( aLogType == SMI_REDO_LOG )
        {
            SMI_LOGREC_READ_AND_MOVE_PTR( sAnalyzePtr, &sColTotalLen, ID_SIZEOF(UInt) );
            aLogRec->setColumnTotalLength((SInt)sColTotalLen, sColSeq); // totalLen은 배열의 seq위치에 들어간다.
        }
        else
        {
            // undo log의 경우, total length는 SMI_UNDO_LOG(-1)의 값을 갖는다.
            aLogRec->setColumnTotalLength(SMI_UNDO_LOG, sColSeq);
        }
    }

    return IDE_SUCCESS;
}

/***************************************************************
 * < Undo info  구조 >
 *
 * --------------------------------------
 * |                 |                  |
 * |  sdcUndoRecHdr  |      scGRID      |
 * |                 |                  |
 * --------------------------------------
 *
 **************************************************************/
IDE_RC smiLogRec::analyzeUndoInfo(smiLogRec *  aLogRec)
{
    SChar          * sAnalyzePtr;

    IDE_DASSERT( aLogRec != NULL );

    // Skip sdcUndoRecHdr, scGRID
    sAnalyzePtr = aLogRec->getAnalyzeStartPtr() +
                    ID_SIZEOF(UShort) +           // size(2)
                    SDC_UNDOREC_HDR_SIZE  +
                    ID_SIZEOF(scGRID);

    aLogRec->setAnalyzeStartPtr(sAnalyzePtr);

    return IDE_SUCCESS;
}

/***************************************************************
 * < Update info  구조 >
 *
 * column desc set size : 뒤에 나오는 column desc set의 크기
 * column desc set : 1~128 byte까지 길이가 가변적이다. RP에서는
 *                   필요없는 정보이므로 모두 skip한다.
 *
 * ----------------------------------------------
 * |        |      |        | column   | column |
 * | opcode | size | column | desc     | desc   |
 * |        |      | count  | set size | set    |
 * ----------------------------------------------
 *
 **************************************************************/
IDE_RC smiLogRec::analyzeUpdateInfo(smiLogRec *  aLogRec)
{
    SChar          * sAnalyzePtr;
    SChar            sSize;

    IDE_DASSERT( aLogRec != NULL );

    sAnalyzePtr = aLogRec->getAnalyzeStartPtr();

    switch(aLogRec->getChangeType())
    {
        case SMI_UNDO_DRDB_UPDATE :
        case SMI_REDO_DRDB_UPDATE :
            // Skip flag(1), size(2), columnCount(2)
            sAnalyzePtr += (ID_SIZEOF(SChar) + ID_SIZEOF(UShort) + ID_SIZEOF(UShort));
            /* Skip column desc */
            sSize = (SChar) *sAnalyzePtr;
            sAnalyzePtr += (ID_SIZEOF(SChar) + sSize);
            break;
        case SMI_UNDO_DRDB_UPDATE_OVERWRITE :
        case SMI_REDO_DRDB_UPDATE_OVERWRITE :
        case SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN :
            // Skip flag(1), size(2)
            sAnalyzePtr += (ID_SIZEOF(SChar) + ID_SIZEOF(UShort));
            break;
        case SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE :
            // Skip size(2)
            sAnalyzePtr += ID_SIZEOF(UShort);
            break;
        /* TASK-5030 */
        case SMI_UNDO_DRDB_DELETE :
            // Skip size(2)
            sAnalyzePtr += ID_SIZEOF(UShort);
            break;
        default :
            IDE_DASSERT(ID_FALSE);
            break;
    }
    aLogRec->setAnalyzeStartPtr(sAnalyzePtr);

    return IDE_SUCCESS;
}

/***************************************************************
 * < Row image info  구조 >
 *
 *          |<- optional->|<--- repeat ---->|
 * ------------------------------------------
 * |        |      | next |        |        |
 * | row    | next | slot | column | column |
 * | header | PID  | num  | length | data   |
 * ------------------------------------------
 *
 **************************************************************/
IDE_RC smiLogRec::analyzeRowImage( iduMemAllocator * aAllocator,
                                   smiLogRec       * aLogRec,
                                   UInt            * aCIDArray,
                                   smiValue        * aColValueArray,
                                   smiChainedValue * aChainedColValueArray,
                                   UInt            * aChainedValueTotalLen,
                                   UInt            * aAnalyzedValueLen,
                                   UShort          * aAnalyzedColCnt )
{
    SChar           * sAnalyzePtr;
    SChar             sRowHdrFlag;
    UShort            sColCntInRowPiece;
    UInt              sTableColCnt = 0;
    const void      * sTable;
    const smiColumn * sColumn;
    UInt              sCID;
    SChar             sErrorBuffer[256];
    UInt              i = 0;

    IDE_DASSERT( aLogRec           != NULL );
    IDE_DASSERT( aAnalyzedColCnt   != NULL );
    IDE_DASSERT( aCIDArray         != NULL );
    IDE_DASSERT( aColValueArray    != NULL );
    IDE_DASSERT( aAnalyzedValueLen != NULL );

    sAnalyzePtr = aLogRec->getAnalyzeStartPtr();

    // 1. Analyze RowHeader

    // row piece내의 컬럼의 개수
    sAnalyzePtr += SDC_ROWHDR_COLCOUNT_OFFSET;
    sColCntInRowPiece = aLogRec->getUShortValue(sAnalyzePtr);
    sAnalyzePtr += SDC_ROWHDR_COLCOUNT_SIZE;

    // row header의 mFlag
    idlOS::memcpy( &sRowHdrFlag,
                   sAnalyzePtr,
                   ID_SIZEOF(SChar) );

    sAnalyzePtr = aLogRec->getAnalyzeStartPtr();
    sAnalyzePtr += SDC_ROWHDR_SIZE;

    // 2. Skip NextPID, NextSlotNum
    IDE_TEST( skipOptionalInfo(aLogRec, (void **)&sAnalyzePtr, sRowHdrFlag)
              != IDE_SUCCESS );

    // 3. analyze Column value
    IDE_TEST( analyzeColumnValue( aAllocator,
                                  aLogRec,
                                  (SChar **)&sAnalyzePtr,
                                  aCIDArray,
                                  aColValueArray,
                                  aChainedColValueArray,
                                  aChainedValueTotalLen,
                                  aAnalyzedValueLen,
                                  sColCntInRowPiece,
                                  sRowHdrFlag,
                                  aAnalyzedColCnt )
              != IDE_SUCCESS );

    // 5. Check trailing NULL
    if ( aLogRec->getChangeType() == SMI_REDO_DRDB_INSERT )
    {
        if ( (sRowHdrFlag & SDC_ROWHDR_H_FLAG) == SDC_ROWHDR_H_FLAG ) //insert의 마지막 log piece
        {
            // Get Meta
            IDE_TEST_RAISE( aLogRec->mGetTable( aLogRec->mMeta,
                                                (ULong)aLogRec->getTableOID(),
                                                &sTable )
                            != IDE_SUCCESS, ERR_TABLE_NOT_FOUND );
            sTableColCnt = aLogRec->mGetColumnCount(sTable);
            IDE_DASSERT( sTableColCnt != 0 );

            if ( *aAnalyzedColCnt != sTableColCnt )
            {
                IDE_DASSERT( *aAnalyzedColCnt < sTableColCnt );

                for(i = 0; i < sTableColCnt; i++)
                {
                    sColumn = aLogRec->mGetColumn(sTable, i);
                    sCID = sColumn->id & SMI_COLUMN_ID_MASK;
                    IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_INVALID_CID);

                    if ( i < *aAnalyzedColCnt )
                    {
                        if ( (sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
                        {
                            /* BUG-30118 : Lob이 중간 컬럼이면서 trailing null이 있는 경우 처리.
                             *
                             * inmode lob의 경우, lob로그가 따로 오지 않고 INSERT 로그로 모두 처리합니다.
                             * outmode lob의 경우, INSERT로그 이후에 LOB_PIECE_WRITE로그가 옵니다.
                             * 처리방법 : inmode lob은 이미 분석이 된 상태로 CID가 CIDArray에 들어가
                             * 있습니다. outmode lob은 아직 로그도 안온 상태이므로 CID가 CIDArray에 없습니다.
                             * 컬럼의 CID가 CIDArray에 들어가 있으면 분석된 inmode lob컬럼으로 간주합니다.
                             */
                            if ( isCIDInArray(aCIDArray, sCID, *aAnalyzedColCnt)
                                 == ID_TRUE )
                            {
                                // IN-MODE LOB. Nothing todo.
                            }
                            else
                            {
                                // OUT-MODE LOB
                                aCIDArray[*aAnalyzedColCnt] = sCID;
                                *aAnalyzedColCnt += 1;
                            }
                        }
                    }
                    else
                    {
                        aCIDArray[sCID] = sCID;
                        aColValueArray[sCID].length = 0;
                        aColValueArray[sCID].value  = NULL;
                        *aAnalyzedColCnt += 1;
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    aLogRec->setAnalyzeStartPtr(sAnalyzePtr);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TABLE_NOT_FOUND);
    {
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(ERR_INVALID_CID);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageMemory] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT"]",
                         sCID,
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 * + Skip non-update column image 에 대한 설명
 *
 * update dml의 경우, 실제 업데이트 한 컬럼이 아니어도,
 * 그 컬럼의 변경으로 인해 다른 컬럼들이 영향을 받을 수 있다.
 * RP에서 필요한 정보는 업데이트 한 컬럼 만이므로, 그 외의
 * 컬럼 정보는 skip한다.
 *
 * EX) RP  info   =>    C5의 seq : 3,  C8의 seq : 6
 *     Row image  =>    C3, C4, C5, C6, C7, C8 의 value
 *
 * 위의 경우, 해당 row piece내에 C3~C8의 컬럼이 들어 있다.
 * 실제 업데이트는 C5와 C8에 발생하였지만, overwrite_row_piece
 * 가 발생하여, row image 에는 C3~C8의 image가 모두 쓰여졌다.
 * RP에서 필요한 것은 C5, C8의 image이므로, row image 에서
 * C5의 seq에 해당하는 세번째 image와 C8의 seq에 해당하는 6번째
 * image를 읽어간다.
 *
 * log type이 update_row_piece인 경우는 좀 다르다.
 * 실제로 업데이트 된 컬럼의 image만이 row image 에 남기
 * 때문에 seq정보로 찾지 말고, RP Info에 기록된 순서와 동일한
 * row image 에서 읽어간다.
 *
 * EX) RP  info   =>    C5의 seq : 3,  C8의 seq : 6
 *     Row image  =>    C5, C8 의 value
 *
 **************************************************************/
IDE_RC smiLogRec::analyzeColumnValue( iduMemAllocator * aAllocator,
                                      smiLogRec       * aLogRec,
                                      SChar          ** aAnalyzePtr,
                                      UInt            * aCIDArray,
                                      smiValue        * aColValueArray,
                                      smiChainedValue * aChainedColValueArray,
                                      UInt            * aChainedValueTotalLen,
                                      UInt            * aAnalyzedValueLen,
                                      UShort            aColumnCountInRowPiece,
                                      SChar             aRowHdrFlag,
                                      UShort          * aAnalyzedColCnt )
{
    const void      * sTable;
    const smiColumn * sColumn; 
    SChar             sErrorBuffer[256];
    UInt              sCID;
    UShort            sColumnLen;
    SInt              sColumnTotalLen;
    SInt              sLogType;
    UShort            i;
    UInt              sCheckChainedValue;
    UShort            sColSeq;
    UInt              sAnalyzedValueLen = 0;
    UInt              sSaveAnalyzeLen = 0;
    UInt              sAnalyzeColCntInRowPiece = 0;
    idBool            sIsOutModeLob;
    idBool            sIsNull;

    IDE_TEST_RAISE( aLogRec->mGetTable( aLogRec->mMeta,
                                        (ULong)aLogRec->getTableOID(),
                                        &sTable )
                    != IDE_SUCCESS, ERR_TABLE_NOT_FOUND );

    for(i=0; i<aColumnCountInRowPiece; i++)
    {
        // row piece내에서 분석한 컬럼의 수와 rp info 에서의 컬럼수가
        // 일치하면 column value분석을 종료한다.
        if ( aLogRec->getUpdateColCntInRowPiece() <= sAnalyzeColCntInRowPiece )
        {
            break;
        }

        sColSeq = aLogRec->getColumnSequence( sAnalyzeColCntInRowPiece );
        sCID = (UInt)aLogRec->getColumnId(sColSeq) & SMI_COLUMN_ID_MASK;

        sColumn = aLogRec->mGetColumn(sTable, sCID);

        /*
         * <TRAILING NULL을 업데이트 하는 경우>
         * UPDATE_OVERWRITE 로그가 남는 경우는 split이 발생한 경우와,
         * trailing null인 경우 두 가지이다.
         * trailing null의 경우, sm에서는 데이터를 저장하지 않으므로
         * rowImage에도 정보가 남지 않는다.
         * RP info 에 seq와 id값을 각각 ID_USHORT_MAX와 ID_UINT_MAX로
         * 저장하여, 이를 읽으면 rowImage의 컬럼이미지 분석단계를
         * skip하고, 바로 null로 채우도록 한다.
         */
        if ( (UInt)aLogRec->getColumnId(sColSeq) == ID_UINT_MAX )
        {
            IDE_DASSERT( aLogRec->getChangeType() == SMI_UNDO_DRDB_UPDATE_OVERWRITE );

            aChainedColValueArray[sCID].mColumn.value  = NULL;
            aChainedColValueArray[sCID].mColumn.length = 0;
            *aAnalyzedColCnt += 1;
            sAnalyzeColCntInRowPiece += 1;
            aChainedValueTotalLen[sCID] = 0;
            continue;
        }

        // Get column length
        analyzeColumnAndMovePtr( (SChar **)aAnalyzePtr,
                                 &sColumnLen,
                                 &sIsOutModeLob,
                                 &sIsNull );

        // PROJ-1862 In Mode Lob
        // 만약 Out Mode LOB Descriptor이면 제외한다.
        if ( sIsOutModeLob == ID_TRUE )
        {
            IDE_DASSERT( sColumnLen == ID_SIZEOF(sdcLobDesc) );
            IDE_CONT(SKIP_COLUMN_ANLZ);
        }

        /*
         * Skip non-update column image.
         * 아래의 두 타입의 경우는 업데이트 대상 컬럼만이 row image 에 남는다.
         * 따라서 sColSeq와 i가 일치하지 않을 수 있다.
         * 하지만 그 외의 타입은 sColSeq에 해당하는 위치에 존재하는 row image 의
         * value만을 보아야한다.
         * DELETE ROW PIECE FOR UPDATE의 경우, 맨 첫 컬럼에 대해서만 image를 보기때문에
         * 항상 sColSeq가 i와 같다.
         */
        if ( ( aLogRec->getChangeType() != SMI_UNDO_DRDB_UPDATE ) &&
             ( aLogRec->getChangeType() != SMI_REDO_DRDB_UPDATE ) )
        {
            if ( sColSeq != i )
            {
                /* INSERT시에 중간에 lob데이터가 껴있는 경우, sColSeq != i일 수 있다.
                   log descriptor만큼 건너 뛴다.
                   if (aLogRec->getChangeType() == SMI_REDO_DRDB_INSERT)
                   {
                   IDE_DASSERT(sColumnLen == sdcLobDescriptorSize);
                   }
                */

                IDE_CONT(SKIP_COLUMN_ANLZ);
            }
        }

        // Get column total length
        sColumnTotalLen = aLogRec->getColumnTotalLength(sColSeq);

        // Set log type
        if ( sColumnTotalLen == SMI_UNDO_LOG )
        {
            sLogType = SMI_UNDO_LOG;
        }
        else
        {
            sLogType = SMI_REDO_LOG;
        }

        // Set CID
        if ( sLogType == SMI_REDO_LOG )
        {
            aCIDArray[*aAnalyzedColCnt] = sCID;
        }
        else
        {
            /* TASK-5030 */
            if ( aLogRec->needSupplementalLog() == ID_TRUE )
            {
                aCIDArray[*aAnalyzedColCnt] = sCID;
            }
        }

        // NULL Value
        if ( sIsNull == ID_TRUE )
        {
            if ( sLogType == SMI_UNDO_LOG )
            {
                aChainedColValueArray[sCID].mColumn.value  = NULL;
                aChainedColValueArray[sCID].mColumn.length = 0;
                aChainedValueTotalLen[sCID] = 0;
            }
            else if ( sLogType == SMI_REDO_LOG )
            {
                aColValueArray[sCID].value  = NULL;
                aColValueArray[sCID].length = 0;
            }
            else
            {
                IDE_DASSERT(ID_FALSE);
            }

            *aAnalyzedColCnt += 1;
            sAnalyzeColCntInRowPiece += 1;

            continue;
        }

        sCheckChainedValue = checkChainedValue(aRowHdrFlag,
                                               sColSeq,
                                               aColumnCountInRowPiece,
                                               sLogType);

        /*
         * Set analyzedValueLen
         *
         * 할당된 공간에서 value복사 할 위치를 계산하기 위해 현재까지
         * 분석된 길이를 설정한다.
         * First chained value의 경우, 분석의 시작이므로,
         * 현재까지 분석한 길이는 0이다.
         */
        if ( ( sCheckChainedValue == SMI_FIRST_CHAINED_VALUE ) ||
             ( sCheckChainedValue == SMI_NON_CHAINED_VALUE ) )
        {
            sAnalyzedValueLen = 0;
        }
        else
        {
            sAnalyzedValueLen = *aAnalyzedValueLen;
        }

        // undo log
        if ( sLogType == SMI_UNDO_LOG )
        {
            IDE_TEST(copyBeforeImage(aAllocator,
                                     sColumn,
                                     aLogRec,
                                     *aAnalyzePtr,
                                     &(aChainedColValueArray[sCID]),
                                     &(aChainedValueTotalLen[sCID]),
                                     &sAnalyzedValueLen,
                                     sColumnLen,
                                     sCheckChainedValue)
                     != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST(copyAfterImage(aAllocator,
                                    sColumn,
                                    aAnalyzePtr,
                                    &(aColValueArray[sCID]),
                                    &sAnalyzedValueLen,
                                    sColumnLen,
                                    sColumnTotalLen,
                                    sCheckChainedValue)
                     != IDE_SUCCESS );
        }

        /*
         * 현재까지 분석한 chained value의 길이를 다음 log piece분석에 전달해야한다.
         * 분석길이 누적을 통해, 미리할당 받은 공간에서 다음에 복사할 위치를 알 수 있다.
         * 한 row piece 내에서 양 쪽에 chained value가 존재할 경우, 먼저 나온 chained value의 값을
         * 저장해 두는 용도로 사용한다.
         * FIRST_CHAINE_VALUE 또는 MIDDLE_CHAINED_VALUE 일때, 다음 row piece의 나머지 value의 복사를
         * 위해, 현재까지 분석된 길이를 넘긴다.
         */
        if ( ( sCheckChainedValue != SMI_LAST_CHAINED_VALUE ) &&
             ( sCheckChainedValue != SMI_NON_CHAINED_VALUE ) )
        {
            sSaveAnalyzeLen = sAnalyzedValueLen;
        }

        // 중복카운팅을 막기 위한 조건. 한 value에 대해 First, Middle, Last piece에 대해 모두 카운팅
        // 해서는 안되므로, Last piece하나에 대해서만 함.
        if ( (sCheckChainedValue == SMI_LAST_CHAINED_VALUE ) ||
             (sCheckChainedValue == SMI_NON_CHAINED_VALUE ))
        {
            // 현재까지 분석한 컬럼 수 - 이 정보는 trailing null 체크에도 사용한다.
            *aAnalyzedColCnt += 1;
        }

        sAnalyzeColCntInRowPiece += 1;

        IDE_EXCEPTION_CONT(SKIP_COLUMN_ANLZ);

        *aAnalyzePtr += sColumnLen;
    }

    if ( sSaveAnalyzeLen > 0 )
    {
        *aAnalyzedValueLen = sSaveAnalyzeLen;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_TABLE_NOT_FOUND);
    {
        idlOS::snprintf( sErrorBuffer,
                         ID_SIZEOF(sErrorBuffer),
                         "[smiLogRec::analyzeColumnValue] Table Not Found (OID: %"ID_UINT64_FMT")",
                         (ULong)aLogRec->getTableOID() );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************
 *
 * Disk DML undo log의 value분석 함수.
 *
 * 특징 :
 *  1. 분석하는 column value의 tatal length를 알지 못한다.
 *  2. column value의 로깅 순서가 저장된 순서와 동일하다.
 *     chained value의 경우, 처음에 복사하는 value가 전체 value의 첫부분이며,
 *     마지막에 복사하는 value가 전체 value의 마지막 부분이다.
 *
 * undo log value는 다음의 두 가지로 복사 방식이 나뉜다.
 *
 * 1. non-chained value
 *    이 경우, log piece내의 column value의 length가 tatal length와 동일하다.
 *    굳이 pool에서 할당받은 공간에 copy를 할 필요가 없으므로,
 *    redo log의 value와 마찬가지로, normal alloc을 받는다.
 *
 * 2. chained value
 *    column value의 총 길이를 모르므로, 매번 분석 시마다의 malloc을 줄이기
 *    위해 pool로 부터 pool_element_size만큼 할당받는다.
 *
 *    2-1. 남은 pool_element의 공간이 복사하려는 value의 길이보다 클때.
 *         공간에 value를 복사하고, pool element의 사용공간 상황을 갱신한다.
 *
 *    2-2. 남은 pool_element의 공간이 복사하려는 value의 길이보다 작을때.
 *         우선 남은 공간에 value를 복사한다. pool element의 사용공간 상황은 full이다.
 *         남은 value의 복사를 위해, 새로운 공간을 할당 받는다.
 *         value의 복사가 모두 끝날 때까지 반복한다.
 *
 **************************************************************/
IDE_RC smiLogRec::copyBeforeImage( iduMemAllocator * aAllocator,
                                   const smiColumn * aColumn,
                                   smiLogRec       * aLogRec,
                                   SChar           * aAnalyzePtr,
                                   smiChainedValue * aChainedValue,
                                   UInt            * aChainedValueTotalLen,
                                   UInt            * aAnalyzedValueLen,
                                   UShort            aColumnLen,
                                   UInt              aColStatus )
{
    smiChainedValue   * sChainedValue;
    SChar             * sAnalyzePtr = NULL;
    UInt                sColumnLen;
    UInt                sState          = 0;
    UInt                sRemainLen      = 0;
    UInt                sRemainSpace    = 0;
    UInt                sCopyLen        = 0;
    UInt                sChainedValuePoolSize   = 0;

    sChainedValue = aChainedValue;
    
    /*
     * PROJ-2047 Strengthening LOB
     * In LOB, the empty value and null value should be distinguished.
     * But the empty value and null value's length is 0.
     * Therefore the dummy header was added to LOB value.
     * In case of empty value, empty value length is 1.
     * In case of null value, null value is null and null value length is 0.
     * The receiver calculates the real value length.(value length - dummy header length)
     */
    sAnalyzePtr = aAnalyzePtr;
    sColumnLen  = aColumnLen;
    
    if ( SMI_IS_LOB_COLUMN(aColumn->flag) == ID_TRUE )
    {
        if ( (aColStatus == SMI_FIRST_CHAINED_VALUE) ||
             (aColStatus == SMI_NON_CHAINED_VALUE ) )
        {
            sAnalyzePtr -= SMI_LOB_DUMMY_HEADER_LEN;
            sColumnLen  += SMI_LOB_DUMMY_HEADER_LEN;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    // non-chained value
    // value의 전체 길이를 알고 있으므로 size만큼 normal alloc하여 공간할당 후, 복사한다.
    if ( aColStatus == SMI_NON_CHAINED_VALUE )
    {

        IDU_FIT_POINT( "smiLogRec::copyBeforeImage::SMI_NON_CHAINED_VALUE::malloc" );

        IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPS,
                                     sColumnLen,
                                     (void **)&sChainedValue->mColumn.value,
                                     IDU_MEM_IMMEDIATE,
                                     aAllocator )
                  != IDE_SUCCESS );
        sState = 1;

        sChainedValue->mColumn.length  = sColumnLen;
        sChainedValue->mAllocMethod    = SMI_NORMAL_ALLOC;
        sChainedValue->mLink           = NULL;

        idlOS::memcpy( (void *)sChainedValue->mColumn.value,
                       sAnalyzePtr,
                       sColumnLen );

        *aChainedValueTotalLen = sColumnLen;

        IDE_CONT(SKIP_COPY_VALUE);
    }
    // chained value
    else
    {
        // 앞 log piece를 분석하며, 이미 linkedlist가 어느정도 차 있을 수 있다.
        // Linkedlist의 마지막 노드까지 이동한다.
        if ( aChainedValue->mAllocMethod != SMI_NON_ALLOCED )
        {
            while( sChainedValue->mLink != NULL )
            {
                sChainedValue = sChainedValue->mLink;
            }
        }

        /*
         * undo log는 저장된 컬럼 순서대로 정방향 로깅되어 있다.
         * 로그의 처음인 first_chained 상태는, 분석의 처음이므로
         * 공간 할당을 한다. 이 상태에서의 value는 chained value의
         * 맨 앞부분 value이다.
         */
        if ( aColStatus == SMI_FIRST_CHAINED_VALUE )
        {
            IDE_DASSERT( sChainedValue->mAllocMethod == SMI_NON_ALLOCED );

            IDE_TEST( aLogRec->chainedValueAlloc( aAllocator, aLogRec, &sChainedValue )
                      != IDE_SUCCESS );
        }

        // 현재까지 복사된 컬럼 밸류의 길이를 누적시킨다.
        *aAnalyzedValueLen     += sColumnLen;
        *aChainedValueTotalLen  = *aAnalyzedValueLen;
    }

    // 공간 계산을 위해, pool element size를 받아온다.
    sChainedValuePoolSize = aLogRec->getChainedValuePoolSize();

    /*
     * pool element에 value를 copy한 이후, 남은 공간을 계산한다.
     * 각각의 smiChainedValue 노드는 smiValue를 멤버로 가지고 있다.
     * 이 smiVale의 value와 length는 이 노드 안에서의 value와 length를 의미한다.
     * pool element size에서 지금까지 공간에 복사된 value의 길이만큼의 차로
     * 남은 공간을 구한다.
     */
    sRemainSpace = sChainedValuePoolSize - sChainedValue->mColumn.length;

    // value size가 남은 공간 크기보다 크므로, memory pool에서 추가 할당을 받아야한다.
    if ( sRemainSpace < sColumnLen )
    {
        if ( sRemainSpace > 0 )
        {
            // 남아있는 공간에 우선 복사를 한다.
            idlOS::memcpy( (SChar *)sChainedValue->mColumn.value + sChainedValue->mColumn.length,
                           sAnalyzePtr, 
                           sRemainSpace );
        }
        // copyLen는 현재까지 복사한 value의 크기이다.
        sCopyLen = sRemainSpace;
        // pool element 내에 현재까지 복사한 Value의 크기이다.
        sChainedValue->mColumn.length += sRemainSpace;
        sAnalyzePtr += sRemainSpace;

        // 복사한 value의 크기가 log piece내 column value의 크기와 같아질 때까지 Loop.
        while( sCopyLen < sColumnLen )
        {
            // 공간을 새로 할당받는다. old node를 인자로 넘겨주면, new node의 주소를
            // 넘져 받는다
            IDE_TEST( aLogRec->chainedValueAlloc( aAllocator, aLogRec, &sChainedValue )
                      != IDE_SUCCESS );

            // log piece내 column value의 크기에서 지금까지 복사한 크기의 차로
            // 앞으로 남은 복사할 분량을 계산한다.
            sRemainLen = sColumnLen - sCopyLen;

            // 새로 할당받은 pool element size보다, 남아있는 복사크기가 작다면,
            // log piece내 value 복사의 끝이다.
            if ( sRemainLen < sChainedValuePoolSize )
            {
                idlOS::memcpy( (void *)sChainedValue->mColumn.value,
                               sAnalyzePtr, 
                               sRemainLen);
                sCopyLen += sRemainLen;
                sChainedValue->mColumn.length += sRemainLen;
                sAnalyzePtr += sRemainLen;
            }
            else
            {
                idlOS::memcpy( (void *)sChainedValue->mColumn.value,
                               sAnalyzePtr,
                               sChainedValuePoolSize );
                sCopyLen += sChainedValuePoolSize;
                sChainedValue->mColumn.length += sChainedValuePoolSize;
                sAnalyzePtr += sChainedValuePoolSize;
            }
        }
    }
    else
    {
        idlOS::memcpy( (SChar *)sChainedValue->mColumn.value + sChainedValue->mColumn.length,
                       sAnalyzePtr,
                       sColumnLen );
        sChainedValue->mColumn.length += sColumnLen;
    }

    IDE_EXCEPTION_CONT(SKIP_COPY_VALUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( (void *)sChainedValue->mColumn.value )
                        == IDE_SUCCESS );
            sChainedValue->mColumn.value = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***************************************************************
 *
 * Disk DML redo log의 value분석 함수.
 *
 * 특징 :
 *  1. 분석하는 column value의 tatal length를 알고 있다.
 *  2. chained value piece의 로깅 순서가 거꾸로 되어있다.
 *     chained value의 경우, 처음에 복사하는 value가 전체 value의 첫부분이며,
 *     마지막에 복사하는 value가 전체 value의 첫 부분이다.
 *
 ***************************************************************/
IDE_RC smiLogRec::copyAfterImage(iduMemAllocator  * aAllocator,
                                 const smiColumn  * aColumn,
                                 SChar           ** aAnalyzePtr,
                                 smiValue         * aColValue,
                                 UInt             * aAnalyzedValueLen,
                                 UShort             aColumnLen,
                                 SInt               aColumnTotalLen,
                                 UInt               aColStatus)
{
    UInt    sOffset = 0;
    SInt    sColumnTotalLen;

    IDE_DASSERT(aColumnTotalLen >= 0); // data가 null인 경우 0.
    IDE_DASSERT(aColumnTotalLen >= (SInt)aColumnLen);

    sColumnTotalLen = aColumnTotalLen;
 
    /*
     * PROJ-2047 Strengthening LOB
     * In LOB, the empty value and null value should be distinguished.
     * But the empty value and null value's length is 0.
     * Therefore the dummy header was added to LOB value.
     * In case of empty value, empty value length is 1.
     * In case of null value, null value is null and null value length is 0.
     * The receiver calculates the real value length.(value length - dummy header length)
     */
    if ( SMI_IS_LOB_COLUMN(aColumn->flag) == ID_TRUE )
    {
        sColumnTotalLen += SMI_LOB_DUMMY_HEADER_LEN;
    }
    
    // non-chained value
    if ( aColStatus == SMI_NON_CHAINED_VALUE )
    {
        sOffset = sColumnTotalLen - (SInt)aColumnLen;
        
        if ( SMI_IS_LOB_COLUMN(aColumn->flag) == ID_TRUE )
        {
            IDE_DASSERT( sOffset == SMI_LOB_DUMMY_HEADER_LEN );
        }
        else
        {
            IDE_DASSERT( sOffset == 0 );
        }
        
        IDU_FIT_POINT( "smiLogRec::copyAfterImage::SMI_NON_CHAINED_VALUE::malloc" );

        IDE_TEST(iduMemMgr::malloc(IDU_MEM_RP_RPS,
                                   sColumnTotalLen,
                                   (void **)&aColValue->value,
                                   IDU_MEM_IMMEDIATE,
                                   aAllocator)
                 != IDE_SUCCESS );

        aColValue->length = sColumnTotalLen;
    }
    else
    {
        // chained value 분석의 시작이므로, total length만큼의 공간을 할당한다.
        if ( aColStatus == SMI_FIRST_CHAINED_VALUE )
        {
            IDU_FIT_POINT( "smiLogRec::copyAfterImage::SMI_CHAINED_VALUE::malloc" );
            
            IDE_TEST(iduMemMgr::malloc(IDU_MEM_RP_RPS,
                                       sColumnTotalLen,
                                       (void **)&aColValue->value,
                                       IDU_MEM_IMMEDIATE,
                                       aAllocator)
                     != IDE_SUCCESS );

            aColValue->length = sColumnTotalLen;

            // redo log에서는 chained value의 뒤 쪽 value부터 로깅되므로,
            // 복사는 공간의 뒤부터 한다.
            sOffset = sColumnTotalLen - (SInt)aColumnLen;
        }
        // chained value의 중간 또는 마지막 piece인 경우, 현재까지 분석된 길이만큼
        // 뒤 쪽 공간을 남겨두고 복사한다.
        else
        {
            IDE_DASSERT( aColValue->length == (UInt)sColumnTotalLen );
            
            IDE_DASSERT((aColStatus == SMI_MIDDLE_CHAINED_VALUE ) ||
                        (aColStatus == SMI_LAST_CHAINED_VALUE ));

            IDE_DASSERT((sColumnTotalLen - *aAnalyzedValueLen) >= aColumnLen);

            sOffset = sColumnTotalLen - (*aAnalyzedValueLen + (UInt)aColumnLen);
        }

        *aAnalyzedValueLen += (UInt)aColumnLen;
    }

    idlOS::memcpy((SChar *)aColValue->value + sOffset,
                  *aAnalyzePtr,
                  aColumnLen);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***************************************************************
 *
 * Row image info 에서, nextPID 와 nextSlotNum을
 * skip해야할지 판단하여  처리한다.
 *
 * flag에 <L>이 포함 되어 있는 경우와,
 * log type이  SDC_UNDO_UPDATE_ROW_PIECE
 *             SDR_SDC_UPDATE_ROW_PIECE
 *             SDC_UNDO_DELETE_FIRST_COLUMN_PIECE
 * 인 경우, 존재하지 않는다.
 *
 **************************************************************/
IDE_RC smiLogRec::skipOptionalInfo(smiLogRec  * aLogRec,
                                   void      ** aAnalyzePtr,
                                   SChar        aRowHdrFlag)
{
    UInt    sLogType;
    SChar * sAnalyzePtr = (SChar *)*aAnalyzePtr;

    // Flag가 <L>인 경우, 로깅되지 않는 정보이다.
    if ( (aRowHdrFlag & SDC_ROWHDR_L_FLAG) == SDC_ROWHDR_L_FLAG )
    {
        // Nothing to do.
    }
    else
    {
        sLogType = aLogRec->getChangeType();

        if ( (sLogType == SMI_UNDO_DRDB_UPDATE) ||
             (sLogType == SMI_REDO_DRDB_UPDATE) ||
             (sLogType == SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN) )
        {
            // Nothing to do.
        }
        else
        {
            sAnalyzePtr += ID_SIZEOF(scPageID)+ ID_SIZEOF(scSlotNum);
            *aAnalyzePtr = sAnalyzePtr;
        }
    }

    return IDE_SUCCESS;
}

/***************************************************************
 *
 * chainedValue의 상태를 확인한다.
 * 상태는 chained value 또는 non-chained value이며,
 * chained value의 경우, 로그 분석 순서를 기준으로
 * value의 시작, 중간, 끝으로 설정한다.
 *
 * 상태값 :
 * A. chained value       1. SMI_FIRST_CHAINED_VALUE
 *                        2. SMI_MIDDLE_CHAINED_VALUE
 *                        3. SMI_LAST_CHAINED_VALUE
 * B. non-chained value   4. SMI_NON_CHAINED_VALUE
 *
 * 위의 상태값에서 주의할 사항은,
 * FIRST_CHAINED_VALUE 라는 말이,
 * log에 나오는 순서상의 chained value의 처음이라는 말이지
 * 실제 value의 첫 부분이 아니라는 점이다.
 * 분석시에 공간 계산을 용이하게 하기 위하여 붙인 이름으로,
 * REDO LOG의 경우에는 value의 맨 끝부분이 FIRST_CHAINED_VALUE가 된다.
 * 반대로 UNDO LOG의 경우에는 상태값과 value의 상태가 동일하다.
 *
 **************************************************************/
UInt smiLogRec::checkChainedValue(SChar    aRowHdrFlag,
                                  UInt     aPosition,
                                  UShort   aColCntInRowPiece,
                                  SInt     aLogType)
{
    UInt sCheckChainedValue;

    // row piece내에 반드시 하나 이상의 컬럼이 존재해야하므로
    // -1을 하여도 음수가 될 일은 없다.
    aColCntInRowPiece -= 1;

    // P와 N 플래그가 동시에 존재하며, row piece내의 컬럼이 1개 뿐이면,
    // chained value의 중간 piece이다.
    if ( ( (aRowHdrFlag & SDC_ROWHDR_N_FLAG) == SDC_ROWHDR_N_FLAG ) &&
         ( (aRowHdrFlag & SDC_ROWHDR_P_FLAG) == SDC_ROWHDR_P_FLAG ) &&
         ( aColCntInRowPiece == 0 ) )
    {
        sCheckChainedValue = SMI_MIDDLE_CHAINED_VALUE;
    }
    else
    {
        // REDO LOG는 로깅 순서가 저장되는 컬럼 순서의 역방향이다.
        // 따라서, 밸류의 뒷 쪽이 먼저 로깅된다.
        if ( aLogType == SMI_REDO_LOG )
        {
            // row piece내에서 마지막에 위치하는 chained value
            if ( ( (aRowHdrFlag & SDC_ROWHDR_N_FLAG) == SDC_ROWHDR_N_FLAG ) &&
                 ( aPosition == aColCntInRowPiece))
            {
                sCheckChainedValue = SMI_LAST_CHAINED_VALUE;
            }
            // row piece내에서 처음에 위치하는 chained value
            else if ( ( (aRowHdrFlag & SDC_ROWHDR_P_FLAG) == SDC_ROWHDR_P_FLAG ) &&
                      ( aPosition == 0) )
            {
                sCheckChainedValue = SMI_FIRST_CHAINED_VALUE;
            }
            else
            {
                sCheckChainedValue = SMI_NON_CHAINED_VALUE;
            }
        }
        // UNDO LOG는 로깅 순서가 정방향이다.
        else
        {
            // row piece내에서 처음에 위치하는 chained value
            if ( ( (aRowHdrFlag & SDC_ROWHDR_P_FLAG) == SDC_ROWHDR_P_FLAG ) &&
                 ( aPosition == 0 ) )
            {
                sCheckChainedValue = SMI_LAST_CHAINED_VALUE;
            }
            // row piece내에서 마지막에 위치하는 chained value
            else if ( ( (aRowHdrFlag & SDC_ROWHDR_N_FLAG) == SDC_ROWHDR_N_FLAG ) &&
                      ( aPosition == aColCntInRowPiece ) )
            {
                sCheckChainedValue = SMI_FIRST_CHAINED_VALUE;
            }
            else
            {
                sCheckChainedValue = SMI_NON_CHAINED_VALUE;
            }
        }
    }

    return sCheckChainedValue;
}

/***************************************************************
 * LOB Piece write for DML
 * -------------------------------------------------
 * |           |  Piece  |  LOB  | Column | Total  |
 * | sdrLogHdr |   Len   | Piece |   ID   |  Len   |
 * |           | (UInt)  | Value | (UInt) | (UInt) |
 * -------------------------------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzeWriteLobPieceLogDisk( iduMemAllocator * aAllocator,
                                               smiLogRec * aLogRec,
                                               smiValue  * aAColValueArray,
                                               UInt      * aCIDArray,
                                               UInt      * aAnalyzedValueLen,
                                               UShort    * aAnalyzedColCnt,
                                               idBool    * aDoWait,
                                               UInt      * aLobCID,
                                               idBool      aIsAfterInsert )
{
    UInt        sPieceLen;
    SChar     * sValue;
    UInt        sCID;
    UInt        sTotalLen;
    SChar     * sOffsetPtr;

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aAColValueArray != NULL );
    IDE_DASSERT( aAnalyzedValueLen != NULL );
    IDE_DASSERT( aDoWait != NULL );

    // 디스크 로그의 mContType을 보고 다음에 로그가 이어지는 판단한다.
    *aDoWait = ((smrContType)aLogRec->getContType() == SMR_CT_END) ?
               ID_FALSE : ID_TRUE;

    sOffsetPtr = aLogRec->getAnalyzeStartPtr();

    /* skip sdrLogHdr */
    sOffsetPtr += ID_SIZEOF(sdrLogHdr);

    /* Get Piece Length */
    sPieceLen = aLogRec->getUIntValue(sOffsetPtr);
    sOffsetPtr += ID_SIZEOF(UInt);

    /* Set Value Pointer */
    sValue = sOffsetPtr;
    sOffsetPtr += sPieceLen;

    /* Get Column ID */
    sCID = aLogRec->getUIntValue(sOffsetPtr) & SMI_COLUMN_ID_MASK;
    *aLobCID = sCID;
    sOffsetPtr += ID_SIZEOF(UInt);

    /* Get LOB Column Total length */
    sTotalLen = aLogRec->getUIntValue(sOffsetPtr) + SMI_LOB_DUMMY_HEADER_LEN;
    sOffsetPtr += ID_SIZEOF(UInt);

    /* 해당 LOB column value의 처음 시작 */
    if ( aAColValueArray[sCID].value == NULL )
    {
        aAColValueArray[sCID].length = sTotalLen;

        if ( aIsAfterInsert == ID_FALSE )
        {
            /* PROJ-1705
             * CID를 우선 분석된 컬럼들 순서 뒤에다 넣어둔다.
             * 한 레코드에 대한 모든 분석이 끝난 후, CID의 정렬작업을 하기때문에 괜찮다.
             * 단, insert의 경우, lob의 null 데이터가 입력되는 경우를 위해 이미 cid를 모두
             * 넣었으므로 제외한다.
             */
            aCIDArray[*aAnalyzedColCnt] = sCID;

            /* PROJ-1705
             * insert의 경우, non-Lob 컬럼들의 insert작업을 마친 후, 아직 분석되지 않은
             * lob컬럼까지 카운팅하여 anlyzedColCnt를 증가시킨다.
             * 이유는 lob에 null데이터가 insert될 경우, 로그가 따로 오지 않기 때문에
             * 현재까지 분석된 상태만으로도 (null밸류로 이미 초기화 되어 있으므로)
             * 이중화가 가능하게끔 하기 위함이다. 이미 증가되어 있기때문에 제외한다.
             */
            *aAnalyzedColCnt += 1;
        }

        /* -> NULL, EMPTY 로 Update 되는 경우에는 별다르게 할일이 없으므로
           종료 조건으로 간다. */
        IDE_TEST_CONT(sTotalLen == SMI_LOB_DUMMY_HEADER_LEN, SKIP_UPDATE_TO_NULL);

        *aAnalyzedValueLen = SMI_LOB_DUMMY_HEADER_LEN;
        IDE_TEST(iduMemMgr::malloc( IDU_MEM_RP_RPS,
                                    sTotalLen,
                                    (void **)&aAColValueArray[sCID].value,
                                    IDU_MEM_IMMEDIATE,
                                    aAllocator)
                 != IDE_SUCCESS );
    }

    idlOS::memcpy((SChar *)aAColValueArray[sCID].value + *aAnalyzedValueLen, sValue, sPieceLen);
    *aAnalyzedValueLen += sPieceLen;

    IDE_EXCEPTION_CONT(SKIP_UPDATE_TO_NULL);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***************************************************************
 * LOB Prepare for Write log
 * --------------------------------
 * |  Table  | Column |  Primary  |
 * |   OID   |   ID   |    Key    |
 * | (smOID) | (UInt) |    Area   |
 * |(vULong) |        |           |
 * --------------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzeLobCursorOpenMem(smiLogRec  *aLogRec,
                                          UInt       *aPKColCnt,
                                          UInt       *aPKCIDArray,
                                          smiValue   *aPKColValueArray,
                                          ULong      *aTableOID,
                                          UInt       *aCID)
{
    SChar    *sAnlzPtr = aLogRec->getAnalyzeStartPtr();
    UInt      sOffset = 0;
    UInt      sPKSize;
    smOID     sTableOID;

    /* Argument 주석
       aLogRec : 현재 Log Record
       aPKColCnt : Primary Key column count
       aPKCIDArray : Primary Key Column ID array
       aPKColValueArray : Primary Key Column Value array
       aTableOID : 현재 LOB cursor를 open하는 table OID
       aCID : open할 LOB Column ID
    */

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aPKColCnt != NULL );
    IDE_DASSERT( aPKCIDArray != NULL );
    IDE_DASSERT( aPKColValueArray != NULL );
    IDE_DASSERT( aTableOID != NULL );
    IDE_DASSERT( aCID != NULL );

    sTableOID = aLogRec->getvULongValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(vULong);
    *aTableOID = (ULong)sTableOID;

    *aCID = aLogRec->getUIntValue(sAnlzPtr, sOffset) & SMI_COLUMN_ID_MASK;

    sOffset += ID_SIZEOF(UInt);

    // PROJ-1705로 인해 PKLog분석 함수명이 PKMem과 PKDisk로 나뉘었다.
    // Lob로그는 PROJ-1705이전의 PK Log구조를 가지므로,
    // 예전의 로그 구조 분석 함수인 PKMem으로 분석하도록 한다.
    IDE_TEST( analyzePKMem( aLogRec,
                            sAnlzPtr + sOffset,
                            aPKColCnt,
                            &sPKSize,
                            aPKCIDArray,
                            aPKColValueArray,
                            NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***************************************************************
 * PROJ-1705
 * analyzeLobCursorOpenMem과는
 * PK정보의 구조와 value의 저장형태가 다르다.
 **************************************************************/
IDE_RC smiLogRec::analyzeLobCursorOpenDisk(smiLogRec * aLogRec,
                                           UInt       * aPKColCnt,
                                           UInt       * aPKCIDArray,
                                           smiValue   * aPKColValueArray,
                                           ULong      * aTableOID,
                                           UInt       * aCID)
{
    SChar * sAnlzPtr;
    smOID   sTableOID;

    /* Argument 주석
       aLogRec : 현재 Log Record
       aPKColCnt : Primary Key column count
       aPKCIDArray : Primary Key Column ID array
       aPKColValueArray : Primary Key Column Value array
       aTableOID : 현재 LOB cursor를 open하는 table OID
       aCID : open할 LOB Column ID
    */

    // Simple argument check code
    IDE_ASSERT( aLogRec          != NULL );
    IDE_ASSERT( aPKColCnt        != NULL );
    IDE_ASSERT( aPKCIDArray      != NULL );
    IDE_ASSERT( aPKColValueArray != NULL );
    IDE_ASSERT( aTableOID        != NULL );
    IDE_ASSERT( aCID             != NULL );

    sAnlzPtr = aLogRec->getAnalyzeStartPtr();
    sTableOID = aLogRec->getvULongValue(sAnlzPtr);
    sAnlzPtr += ID_SIZEOF(vULong);
    *aTableOID = (ULong)sTableOID;

    *aCID = aLogRec->getUIntValue(sAnlzPtr) & SMI_COLUMN_ID_MASK;

    sAnlzPtr += ID_SIZEOF(UInt);

    aLogRec->setAnalyzeStartPtr(sAnlzPtr);

    return IDE_SUCCESS;
}

/***************************************************************
 * LOB Prepare for Write log
 * -----------------------------
 * | Offset |  Old    |  New   |
 * |        |  Size   |  Size  |
 * | (UInt) | (UInt)  | (UInt) |
 * -----------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzeLobPrepare4Write( smiLogRec  *aLogRec,
                                           UInt       *aLobOffset,
                                           UInt       *aOldSize,
                                           UInt       *aNewSize )
{
    SChar    *sAnlzPtr = aLogRec->getAnalyzeStartPtr();
    UInt      sOffset = 0;

    /* Argument 주석
       aLogRec : 현재 Log Record
       aLobOffset : LOB Piece의 시작 offset
       aOldSize : 변경할 LOB Piece의 Old Size
       aNewSize : 변경할 LOB Piece의 New Size
    */

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aLobOffset != NULL );
    IDE_DASSERT( aOldSize != NULL );
    IDE_DASSERT( aNewSize != NULL );

    *aLobOffset = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    *aOldSize = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    *aNewSize = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    return IDE_SUCCESS;
}

/***************************************************************
 * LOB Trim for Write log
 * ----------
 * | Offset |
 * |        |
 * | (UInt) |
 * ----------
 **************************************************************/
IDE_RC smiLogRec::analyzeLobTrim( smiLogRec   * aLogRec,
                                  UInt        * aLobOffset )
{
    SChar * sAnlzPtr    = aLogRec->getAnalyzeStartPtr();
    UInt    sOffset     = 0;
    ULong   sTemp       = 0;

    /* Argument 주석
       aLogRec : 현재 Log Record
       aLobOffset : Trim offset
    */

    // Simple argument check code
    IDE_DASSERT( aLogRec     != NULL );
    IDE_DASSERT( aLobOffset  != NULL );

    /* BUG-39648 로그에 8바이트로 남은 offset을 4바이트로 캐스팅 한다. */
    sTemp = aLogRec->getULongValue(sAnlzPtr, sOffset);
    *aLobOffset = (UInt)sTemp;

    return IDE_SUCCESS;
}

/***************************************************************
 * Memory Partial Write Log
 * --------------------------------------
 * |        |        |   LOB Desc Area  |
 * | Before | After  |------------------|
 * | Image  | Image  |   LOB   |  LOB   |
 * |  Area  | Area   | Locator | Offset |
 * |        |        | (ULong) | (UInt) |
 * --------------------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzeLobPartialWriteMemory( iduMemAllocator * aAllocator,
                                                smiLogRec  *aLogRec,
                                                ULong      *aLobLocator,
                                                UInt       *aLobOffset,
                                                UInt       *aLobPieceLen,
                                                SChar     **aLobPiece )
{
    SChar      *sAnlzPtr = aLogRec->getAnalyzeStartPtr();
    UInt        sOffset = 0;
    SChar      *sAImgPtr;

    sOffset += aLogRec->getBfrImgSize();

    sAImgPtr = sAnlzPtr + sOffset;
    sOffset += aLogRec->getAftImgSize();

    *aLobPieceLen = aLogRec->getAftImgSize();
    *aLobLocator = aLogRec->getULongValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(ULong);

    *aLobOffset = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);


    IDU_FIT_POINT( "smiLogRec::analyzeLobPartialWriteMemory::malloc" );

    /* LOB Piece를 위한 메모리 할당. 호출한 쪽에서 사용후에 이 메모리를
       반드시 free 시켜주어야 한다.
    */
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_RP_RPS,
                                *aLobPieceLen,
                                (void **)aLobPiece,
                                IDU_MEM_IMMEDIATE,
                                aAllocator )
             != IDE_SUCCESS );
    (void)idlOS::memcpy(*aLobPiece, sAImgPtr, *aLobPieceLen);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***************************************************************
 * Disk Partial Write Log
 * ----------------------------------------------
 * |  LOB  |  LOB   |   LOB   |        |        |
 * | Piece | Piece  | Locator | Offset | Amount |
 * |  Len  | Value  |         |        |        |
 * ----------------------------------------------
 **************************************************************/
IDE_RC smiLogRec::analyzeLobPartialWriteDisk( iduMemAllocator * aAllocator,
                                              smiLogRec  *aLogRec,
                                              ULong      *aLobLocator,
                                              UInt       *aLobOffset,
                                              UInt       *aAmount,
                                              UInt       *aLobPieceLen,
                                              SChar     **aLobPiece )
{
    SChar      *sAnlzPtr = aLogRec->getAnalyzeStartPtr();
    UInt        sOffset = 0;
    SChar      *sValue;

    sOffset += ID_SIZEOF(sdrLogHdr);

    *aLobPieceLen = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    sValue = sAnlzPtr + sOffset;
    sOffset += *aLobPieceLen;

    *aLobLocator = aLogRec->getULongValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(ULong);

    *aLobOffset = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    *aAmount = aLogRec->getUIntValue(sAnlzPtr, sOffset);
    sOffset += ID_SIZEOF(UInt);

    IDU_FIT_POINT( "smiLogRec::analyzeLobPartialWriteDisk::malloc" );

    /* LOB Piece를 위한 메모리 할당. 호출한 쪽에서 사용후에 이 메모리를
       반드시 free 시켜주어야 한다.
    */
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_RP_RPS,
                                *aLobPieceLen,
                                (void **)aLobPiece,
                                IDU_MEM_IMMEDIATE,
                                aAllocator)
             != IDE_SUCCESS );
    (void)idlOS::memcpy(*aLobPiece, sValue, *aLobPieceLen);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smiLogRec::analyzeTxSavePointSetLog( smiLogRec *aLogRec,
                                            UInt      *aSPNameLen,
                                            SChar     *aSPName)
{
    SChar * sLogPtr;

    sLogPtr = aLogRec->getLogPtr();

    sLogPtr += ID_SIZEOF(smiLogHdr);
    idlOS::memcpy( aSPNameLen, sLogPtr, ID_SIZEOF(UInt) );
    sLogPtr += ID_SIZEOF(UInt);

    idlOS::memcpy( aSPName,
                   sLogPtr,
                   *aSPNameLen );
    aSPName[*aSPNameLen] = '\0';

    return IDE_SUCCESS;
}


IDE_RC smiLogRec::analyzeTxSavePointAbortLog( smiLogRec *aLogRec,
                                              UInt      *aSPNameLen,
                                              SChar     *aSPName)
{
    SChar * sLogPtr;

    sLogPtr = aLogRec->getLogPtr();

    sLogPtr += ID_SIZEOF(smiLogHdr);
    idlOS::memcpy( aSPNameLen, sLogPtr, ID_SIZEOF(UInt) );
    sLogPtr += ID_SIZEOF(UInt);

    idlOS::memcpy( aSPName,
                   sLogPtr,
                   *aSPNameLen );
    aSPName[*aSPNameLen] = '\0';

    return IDE_SUCCESS;
}

idBool smiLogRec::needReplicationByType( void  * aLogHeadPtr,
                                         void  * aLogPtr,
                                         smLSN * aLSN )
{
    smrLogHead    *sCommonHdr;

    static SInt sMMDBLogSize = ID_SIZEOF(smrUpdateLog) - ID_SIZEOF(smiLogHdr);
    static SInt sLobLogSize  = ID_SIZEOF(smrLobLog) - ID_SIZEOF(smiLogHdr);
    // Table Meta Log Record 
    static SInt sTableMetaLogSize  = ID_SIZEOF(smrTableMetaLog) - ID_SIZEOF(smiLogHdr);

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aLSN != NULL );
    
    ACP_UNUSED( aLSN );

    mLogPtr           = (SChar*)aLogPtr;
    sCommonHdr        = (smrLogHead*)aLogHeadPtr;
    mLogUnion.mCommon = sCommonHdr;
    mRecordLSN        = smrLogHeadI::getLSN( sCommonHdr );

#ifdef DEBUG
    if ( !SM_IS_LSN_INIT ( *aLSN ) )
    {
        IDE_DASSERT( smrCompareLSN::isEQ( &mRecordLSN, aLSN ) ); 
    }
#endif

    if ( needNormalReplicate() != ID_TRUE )
    {
        return ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }
    switch(smrLogHeadI::getType(sCommonHdr))
    {
        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_DSKTRANS_COMMIT:
            mLogType = SMI_LT_TRANS_COMMIT;
            break;

        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            mLogType = SMI_LT_TRANS_GROUPCOMMIT;
            break;

        case SMR_LT_MEMTRANS_ABORT:
        case SMR_LT_DSKTRANS_ABORT:
            mLogType = SMI_LT_TRANS_ABORT;
            break;

        case SMR_LT_TRANS_PREABORT:
            mLogType = SMI_LT_TRANS_PREABORT;
            break;

        case SMR_LT_SAVEPOINT_SET:
            mLogType = SMI_LT_SAVEPOINT_SET;
            break;

        case SMR_LT_SAVEPOINT_ABORT:
            mLogType = SMI_LT_SAVEPOINT_ABORT;
            break;

        case SMR_LT_FILE_END:
            mLogType = SMI_LT_FILE_END;          // BUG-29837
            break;

        case SMR_LT_UPDATE:
            idlOS::memcpy( (SChar*)&(mLogUnion.mMemUpdate) + ID_SIZEOF(smiLogHdr),
                            mLogPtr + ID_SIZEOF(smiLogHdr),
                            sMMDBLogSize );

            mLogType = SMI_LT_MEMORY_CHANGE;

            switch(mLogUnion.mMemUpdate.mType)
            {
                case SMR_SMC_PERS_INSERT_ROW :
                    mChangeType = SMI_CHANGE_MRDB_INSERT;
                    break;

                case SMR_SMC_PERS_UPDATE_INPLACE_ROW :
                case SMR_SMC_PERS_UPDATE_VERSION_ROW :
                    mChangeType = SMI_CHANGE_MRDB_UPDATE;
                    break;

                case SMR_SMC_PERS_DELETE_VERSION_ROW :
                    mChangeType = SMI_CHANGE_MRDB_DELETE;
                    break;

                case SMR_SMC_PERS_WRITE_LOB_PIECE :
                    mChangeType = SMI_CHANGE_MRDB_LOB_PARTIAL_WRITE;
                    break;
                default:
                    mLogType = SMI_LT_NULL;
                    break;
            }

            break;

        case SMR_DLT_REDOONLY:
        case SMR_DLT_UNDOABLE:
            mLogType = SMI_LT_DISK_CHANGE;
            break;

        case SMR_LT_LOB_FOR_REPL:
            idlOS::memcpy( (SChar*)&(mLogUnion.mLob) + ID_SIZEOF(smiLogHdr),
                           mLogPtr + ID_SIZEOF(smiLogHdr),
                           sLobLogSize );
            mLogType = SMI_LT_LOB_FOR_REPL;

            switch(mLogUnion.mLob.mOpType)
            {
                case SMR_MEM_LOB_CURSOR_OPEN:
                {
                    mChangeType = SMI_CHANGE_MRDB_LOB_CURSOR_OPEN;
                    break;
                }
                case SMR_DISK_LOB_CURSOR_OPEN:
                {
                    mChangeType = SMI_CHANGE_DRDB_LOB_CURSOR_OPEN;
                    break;
                }
                case SMR_LOB_CURSOR_CLOSE:
                {
                    mChangeType = SMI_CHANGE_LOB_CURSOR_CLOSE;
                    break;
                }
                case SMR_PREPARE4WRITE:
                {
                    mChangeType = SMI_CHANGE_LOB_PREPARE4WRITE;
                    break;
                }
                case SMR_FINISH2WRITE:
                {
                    mChangeType = SMI_CHANGE_LOB_FINISH2WRITE;
                    break;
                }
                case SMR_LOB_TRIM:
                {
                    mChangeType = SMI_CHANGE_LOB_TRIM;
                    break;
                }
                default:
                {
                    mLogType = SMI_LT_NULL;
                    break;
                }
            }
            break;

        case SMR_LT_DDL :           // DDL Transaction임을 표시하는 Log Record
            mLogType = SMI_LT_DDL;
            break;

        case SMR_LT_TABLE_META :    // Table Meta Log Record
            idlOS::memcpy( (SChar*)&(mLogUnion.mTableMetaLog) + ID_SIZEOF(smiLogHdr),
                           mLogPtr + ID_SIZEOF(smiLogHdr),
                           sTableMetaLogSize );
            mLogType = SMI_LT_TABLE_META;
            break;

        case SMR_LT_DDL_QUERY_STRING :
            idlOS::memcpy( (SChar*)&(mLogUnion.mDDLStmtMeta),
                           mLogPtr + ID_SIZEOF(smiLogHdr),
                           ID_SIZEOF(smrDDLStmtMeta) );
            mLogType = SMI_LT_DDL_QUERY_STRING;
            break;

        default:
            mLogType = SMI_LT_NULL;
            break;
    }

    if ( (mLogType == SMI_LT_NULL) && (isBeginLog() != ID_TRUE ))
    {
        return ID_FALSE;
    }

    return ID_TRUE;
}

/*******************************************************************************
 * Description : Table Meta Log Record의 Body 크기를 얻는다.
 ******************************************************************************/
UInt smiLogRec::getTblMetaLogBodySize()
{
    UInt sSize = getLogSize() - (SMR_LOGREC_SIZE(smrTableMetaLog)
                                 + ID_SIZEOF(smrLogTail));

    IDE_DASSERT( getLogSize() >= ( SMR_LOGREC_SIZE( smrTableMetaLog )
                                   + ID_SIZEOF( smrLogTail ) ) );

    return sSize;
};

/*******************************************************************************
 * Description : Table Meta Log Record의 Body를 얻는다.
 ******************************************************************************/
void * smiLogRec::getTblMetaLogBodyPtr()
{
    return (void *)(mLogPtr + SMR_LOGREC_SIZE(smrTableMetaLog));
}

/*******************************************************************************
 * Description : Table Meta Log Record의 Header를 얻는다.
 ******************************************************************************/
smiTableMeta * smiLogRec::getTblMeta()
{
    return (smiTableMeta *)&mLogUnion.mTableMetaLog.mTableMeta;
}

UInt smiLogRec::getDDLStmtMetaLogBodySize()
{
    UInt sSize = getLogSize() - ( ID_SIZEOF(smrLogHead) + ID_SIZEOF( smrDDLStmtMeta )
                                 + ID_SIZEOF(smrLogTail));

    IDE_DASSERT( getLogSize() > ( ID_SIZEOF(smrLogHead) + ID_SIZEOF( smrDDLStmtMeta )
                                  + ID_SIZEOF( smrLogTail ) ) );

    return sSize;
}

void * smiLogRec::getDDLStmtMetaLogBodyPtr()
{
    return (void *)( mLogPtr + ID_SIZEOF(smrLogHead) + ID_SIZEOF(smrDDLStmtMeta) );
}

smiDDLStmtMeta * smiLogRec::getDDLStmtMeta()
{
    return (smiDDLStmtMeta *)&mLogUnion.mDDLStmtMeta;
}

/*******************************************************************************
 * Description : memory pool로 부터 공간을 할당받고 list에 연결시킨다.
 *           new node를 생성하여 new node의 value에 pool로 부터의 공간을 할당하고,
 *           인자로 받은 노드의 주소는 old node로써 old node의 link에 new node를 연결한다.
 *           new node의 주소를 보낸다.
 ******************************************************************************/
IDE_RC smiLogRec::chainedValueAlloc( iduMemAllocator  * aAllocator,
                                     smiLogRec        * aLogRec,
                                     smiChainedValue ** aChainedValue )
{
    smiChainedValue * sChainedValue ;
    smiChainedValue * sOldChainedValue = NULL;
    idBool            sChanedValueAllocFlag = ID_FALSE;

    IDE_ASSERT( *aChainedValue != NULL );

    // 첫 smiChainedValue노드는 이 자체가 new node이다.
    // 배열이므로 공간할당이 필요없다.
    if ( (*aChainedValue)->mAllocMethod == SMI_NON_ALLOCED )
    {
        sChainedValue = *aChainedValue;
    }
    else
    {
        IDU_FIT_POINT( "smiLogRec::chainedValueAlloc::valueFull:malloc" );

        // 이 경우는, 인자로 받은 smiChainedValue노드의 value가 full상태이다.
        // smiChainedValue노드를 새로 생성한다.
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_RP_RPS,
                                   ID_SIZEOF(smiChainedValue),
                                   (void **)&sChainedValue,
                                   IDU_MEM_IMMEDIATE,
                                   aAllocator)
                 != IDE_SUCCESS );
        sChanedValueAllocFlag = ID_TRUE;

        // BUG-27329 CodeSonar::Uninitialized Variable (2)
        IDE_TEST( sChainedValue == NULL );

        // old node를 link에 연결 시키고, new node을 초기화한다.
        sOldChainedValue = (*aChainedValue)->mLink;
        (*aChainedValue)->mLink = sChainedValue;
    }

    sChainedValue->mLink = NULL;
    sChainedValue->mColumn.length = 0;

    // smiChainedValue노드의 value에 pool로부터 공간할당을 받는다.
    IDE_TEST(aLogRec->mChainedValuePool->alloc((void **)&(sChainedValue->mColumn.value))
             != IDE_SUCCESS );
    sChainedValue->mAllocMethod = SMI_MEMPOOL_ALLOC;

    // new node의 주소로 교체한다.
    *aChainedValue = sChainedValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sChanedValueAllocFlag == ID_TRUE )
    {
        (*aChainedValue)->mLink = sOldChainedValue;
        (void)iduMemMgr::free(sChainedValue, aAllocator);
    }

    IDE_POP();

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
void * smiLogRec::getRPLogStartPtr4Undo(void             * aLogPtr,
                                        smiChangeLogType   aLogType )
{
    UChar  * sCurrLogPtr;
    UShort   sUptColCount;
    UChar    sColDescSetSize;
    UShort   sLoop;

    IDE_DASSERT(aLogPtr != NULL );
    /* TASK-5030 */
    IDE_DASSERT( ( aLogType == SMI_UNDO_DRDB_UPDATE )                     ||
                 ( aLogType == SMI_UNDO_DRDB_UPDATE_OVERWRITE )           ||
                 ( aLogType == SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN ) ||
                 ( aLogType == SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE )    ||
                 ( aLogType == SMI_UNDO_DRDB_DELETE ) );

    sCurrLogPtr = (UChar *)aLogPtr;

    sCurrLogPtr += ID_SIZEOF(UShort); // size(2) - undo info 의 맨 앞에 위치
    sCurrLogPtr += SDC_UNDOREC_HDR_SIZE;

    sCurrLogPtr += ID_SIZEOF(scGRID);

    switch (aLogType)
    {
        case SMI_UNDO_DRDB_UPDATE :
            sCurrLogPtr += (1);     // flag(1)
            sCurrLogPtr += (2);     // size(2)

            // read update column count(2)
            SMI_LOGREC_READ_AND_MOVE_PTR( sCurrLogPtr,
                                          &sUptColCount,
                                          ID_SIZEOF(sUptColCount) );

            // read column desc set size(1)
            SMI_LOGREC_READ_AND_MOVE_PTR( sCurrLogPtr,
                                          &sColDescSetSize,
                                          ID_SIZEOF(sColDescSetSize) );

            // skip column desc set(1~128)
            sCurrLogPtr += sColDescSetSize;

            // read row header
            sCurrLogPtr += SDC_ROWHDR_SIZE;

            for( sLoop = 0; sLoop < sUptColCount; sLoop++ )
            {
                sCurrLogPtr = sdcRow::getNxtColPiece(sCurrLogPtr);
            }
            break;

        case SMI_UNDO_DRDB_UPDATE_OVERWRITE :

            sCurrLogPtr += (1);     // flag(1)
            sCurrLogPtr += (2);     // size(2)
            sCurrLogPtr += sdcRow::getRowPieceSize(sCurrLogPtr);
            break;

        case SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN :
            sCurrLogPtr += (1);     // flag(1)
            sCurrLogPtr += (2);     // size(2)
            sCurrLogPtr += SDC_ROWHDR_SIZE;
            sCurrLogPtr = sdcRow::getNxtColPiece(sCurrLogPtr);
            break;

        case SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE :
            sCurrLogPtr += (2);     // size(2)
            sCurrLogPtr += sdcRow::getRowPieceSize(sCurrLogPtr);
            break;

        /* TASK-5030 */
        case SMI_UNDO_DRDB_DELETE :
            sCurrLogPtr += (2);     // size(2)
            sCurrLogPtr += sdcRow::getRowPieceSize(sCurrLogPtr);
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return (void *)sCurrLogPtr;
}


/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
void * smiLogRec::getRPLogStartPtr4Redo(void             * aLogPtr,
                                        smiChangeLogType   aLogType)
{
    UChar  * sCurrLogPtr;
    UShort   sUptColCount;
    UChar    sColDescSetSize;
    UShort   sLoop;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( ( aLogType == SMI_REDO_DRDB_INSERT ) ||
                 ( aLogType == SMI_REDO_DRDB_UPDATE_INSERT_ROW_PIECE) ||
                 ( aLogType == SMI_REDO_DRDB_UPDATE) ||
                 ( aLogType == SMI_REDO_DRDB_UPDATE_OVERWRITE));

    sCurrLogPtr = (UChar *)aLogPtr;

    switch (aLogType)
    {

        case SMI_REDO_DRDB_INSERT :
        case SMI_REDO_DRDB_UPDATE_INSERT_ROW_PIECE :

            sCurrLogPtr += sdcRow::getRowPieceSize((UChar *)sCurrLogPtr);
            break;

        case SMI_REDO_DRDB_UPDATE :

            sCurrLogPtr += (1);     // flag(1)
            sCurrLogPtr += (2);     // size(2)

            // read update column count(2)
            SMI_LOGREC_READ_AND_MOVE_PTR(sCurrLogPtr,
                                         &sUptColCount,
                                         ID_SIZEOF(sUptColCount));

            // read column desc set size(1)
            SMI_LOGREC_READ_AND_MOVE_PTR(sCurrLogPtr,
                                         &sColDescSetSize,
                                         ID_SIZEOF(sColDescSetSize));

            // skip column desc set(1~128)
            sCurrLogPtr += sColDescSetSize;

            // read row header
            sCurrLogPtr += SDC_ROWHDR_SIZE;

            for( sLoop = 0; sLoop < sUptColCount; sLoop++ )
            {
                sCurrLogPtr = sdcRow::getNxtColPiece(sCurrLogPtr);
            }

            break;

        case SMI_REDO_DRDB_UPDATE_OVERWRITE :

            sCurrLogPtr += (1);     // flag(1)
            sCurrLogPtr += (2);     // size(2)
            sCurrLogPtr += sdcRow::getRowPieceSize(sCurrLogPtr);
            break;

        default:

            IDE_ASSERT(0);
            break;
    }

    return (void *)sCurrLogPtr;
}

/*******************************************************************************
 * Description : 해당 CID가 CIDArray에 존재하는 지 확인한다.
 ******************************************************************************/
idBool smiLogRec::isCIDInArray( UInt * aCIDArray, 
                                UInt   aCID, 
                                UInt aArraySize )
{
    idBool sIsCIDInArray = ID_FALSE;

    for ( ; aArraySize > 0 ; aArraySize-- )
    {
        if ( aCIDArray[aArraySize - 1] == aCID )
        {
            sIsCIDInArray = ID_TRUE;
            break;
        }
    }
    return sIsCIDInArray;
}

/**********************************************
 *     Log의 Head를 출력한다.
 *
 *     [IN] aLogHead - 출력할 Log의 Head
 *     [IN] aChkFlag  
 *     [IN] aModule - log module
 *     [IN] aLevel  - log level
 *     [USAGE]
 *     dumpLogHead(sLogHead, IDE_RP_0);
 **********************************************/
void smiLogRec::dumpLogHead( smiLogHdr * aLogHead, 
                             UInt aChkFlag, 
                             ideLogModule aModule, 
                             UInt aLevel )
{
    idBool sIsBeginLog = ID_FALSE;
    idBool sIsReplLog  = ID_FALSE;
    idBool sIsSvpLog   = ID_FALSE;
    smrLogHead * sLogHead = (smrLogHead*)aLogHead;

    if ( ( smrLogHeadI::getFlag( sLogHead ) & SMR_LOG_BEGINTRANS_MASK )
         == SMR_LOG_BEGINTRANS_OK )
    {
        sIsBeginLog = ID_TRUE;
    }
    else
    {
        sIsBeginLog = ID_FALSE;
    }
    
    if ( ( smrLogHeadI::getFlag( sLogHead ) & SMR_LOG_TYPE_MASK )
          == SMR_LOG_TYPE_NORMAL )
    {
        sIsReplLog = ID_TRUE;
    }
    else
    {
        sIsReplLog = ID_FALSE;
    }

    if ( ( smrLogHeadI::getFlag( sLogHead ) & SMR_LOG_SAVEPOINT_MASK )
          == SMR_LOG_SAVEPOINT_OK )
    {
        sIsSvpLog = ID_TRUE;
    }
    else
    {
        sIsSvpLog = ID_FALSE;
    }
    
    ideLog::log( aChkFlag, 
                 aModule, 
                 aLevel, 
                 "MAGIC: %"ID_UINT32_FMT", TID: %"ID_UINT32_FMT", "
                 "BE: %s, REP: %s, ISVP: %s, ISVP_DEPTH: %"ID_UINT32_FMT", "
                 "PLSN=<%"ID_UINT32_FMT", %"ID_UINT32_FMT">, "
                 "LT: < %"ID_UINT32_FMT" >, SZ: %"ID_UINT32_FMT" ",
                 smrLogHeadI::getMagic(sLogHead),
                 smrLogHeadI::getTransID(sLogHead),
                 (sIsBeginLog == ID_TRUE)?"Y":"N",
                 (sIsReplLog == ID_TRUE)?"Y":"N",
                 (sIsSvpLog == ID_TRUE)?"Y":"N",
                 smrLogHeadI::getReplStmtDepth(sLogHead),
                 smrLogHeadI::getPrevLSNFileNo(sLogHead),
                 smrLogHeadI::getPrevLSNOffset(sLogHead),
                 smrLogHeadI::getType(sLogHead),
                 smrLogHeadI::getSize(sLogHead));
}

IDE_RC smiLogRec::analyzeInsertLogDictionary( smiLogRec  *aLogRec,
                                              UShort     *aColCnt,
                                              UInt       *aCIDArray,
                                              smiValue   *aAColValueArray,
                                              idBool     *aDoWait )
{
    SChar *sAfterImagePtr;
    SChar *sAfterImagePtrFence;

    // Simple argument check code
    IDE_DASSERT( aLogRec != NULL );
    IDE_DASSERT( aColCnt != NULL );
    IDE_DASSERT( aCIDArray != NULL );
    IDE_DASSERT( aAColValueArray != NULL );
    IDE_DASSERT( aDoWait != NULL );

    // 메모리 로그는 언제나 하나의 로그로 쓰이므로, *aDoWait이 ID_FALSE가 된다.
    *aDoWait = ID_FALSE;

    sAfterImagePtr = aLogRec->getLogPtr() + SMR_LOGREC_SIZE(smrUpdateLog);

    /* TASK-4690, BUG-32319 [sm-mem-collection] The number of MMDB update log
     *                      can be reduced to 1.
     * 로그 마지막에 OldVersion RowOID가 있기 때문에 Fence에 이를 제외해준다. */
    sAfterImagePtrFence = aLogRec->getLogPtr()
                          + aLogRec->getLogSize()
                          - ID_SIZEOF(ULong)
                          - smiLogRec::getLogTailSize();

    IDE_TEST( analyzeInsertLogAfterImageDictionary( aLogRec,
                                                    sAfterImagePtr,
                                                    sAfterImagePtrFence,
                                                    aColCnt,
                                                    aCIDArray,
                                                    aAColValueArray )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiLogRec::analyzeInsertLogAfterImageDictionary( smiLogRec *aLogRec,
                                                        SChar     *aAfterImagePtr,
                                                        SChar     *aAfterImagePtrFence,
                                                        UShort    *aColCount,
                                                        UInt      *aCidArray,
                                                        smiValue  *aAfterColsArray )
{
    void                * sTableHandle;
    const smiColumn     * spCol                 = NULL;
    SChar               * sFixedAreaPtr;
    SChar               * sVarAreaPtr;
    SChar               * sVarColPtr;
    UShort                sFixedAreaSize;
    UInt                  sCID;
    UInt                  sAfterColSize;
    UInt                  i;
    UInt                  sOIDCnt;
    smVCDesc              sVCDesc;
    SChar                 sErrorBuffer[256];
    smOID                 sTableOID;
    smOID                 sVCPieceOID           = SM_NULL_OID;
    UShort                sCurrVarOffset        = 0;
    UShort                sNextVarOffset        = 0;

    sFixedAreaPtr = aAfterImagePtr + SMI_LOGREC_MV_FIXED_ROW_DATA_OFFSET;

    /* Fixed Row의 길이:UShort */
    sFixedAreaSize = aLogRec->getUShortValue( aAfterImagePtr, SMI_LOGREC_MV_FIXED_ROW_SIZE_OFFSET );
    IDE_TEST_RAISE( sFixedAreaSize > SM_PAGE_SIZE,
                    err_too_big_fixed_area_size );

    sVarAreaPtr   = aAfterImagePtr + SMI_LOGREC_MV_FIXED_ROW_DATA_OFFSET + sFixedAreaSize;

    sTableOID = aLogRec->getTableOID();
    sTableHandle = (void *)smiGetTable( sTableOID );

    *aColCount = 1;
    /* extract fixed fields */
    for ( i = 0 ; i < *aColCount ; i++ )
    {
        IDE_TEST( smiGetTableColumns( sTableHandle,
                                      i,
                                      &spCol )
                  != IDE_SUCCESS );

        sCID = spCol->id & SMI_COLUMN_ID_MASK;
        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);
        if ( (spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED )
        {
            aCidArray[sCID] = sCID;
            aAfterColsArray[sCID].length = spCol->size;
            aAfterColsArray[sCID].value  =
                    sFixedAreaPtr + (spCol->offset - smiGetRowHeaderSize(SMI_TABLE_MEMORY));
        }
        else /* large var + lob */
        {
            if ( (spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE ) 
            {
                /* United var 는 fixed 영역에 가진 값이 없다 */
                continue;
            }
            else
            {
                smiLogRec::getVCDescInAftImg(spCol, aAfterImagePtr, &sVCDesc);
                if ((sVCDesc.flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_IN)
                {
                    aCidArray[sCID] = sCID;
                    if ( ( SMI_IS_LOB_COLUMN(spCol->flag) == ID_TRUE ) && ( sVCDesc.length != 0 ) )
                    {
                        aAfterColsArray[sCID].length = sVCDesc.length + SMI_LOB_DUMMY_HEADER_LEN;
                        aAfterColsArray[sCID].value  =
                            sFixedAreaPtr + (spCol->offset - smiGetRowHeaderSize(SMI_TABLE_MEMORY))
                            + ID_SIZEOF( smVCDescInMode ) - SMI_LOB_DUMMY_HEADER_LEN;
                    }
                    else
                    {
                        aAfterColsArray[sCID].length = sVCDesc.length;
                        aAfterColsArray[sCID].value  =
                            sFixedAreaPtr + (spCol->offset - smiGetRowHeaderSize(SMI_TABLE_MEMORY))
                            + ID_SIZEOF( smVCDescInMode );
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }

    /* extract variabel fields */
    sVCPieceOID = aLogRec->getvULongValue( sVarAreaPtr );
    sVarAreaPtr += ID_SIZEOF(smOID);

    if ( sVCPieceOID != SM_NULL_OID )
    {
        /* sVarColCount = aLogRec->getUShortValue (sVarAreaPtr );           항상 1 */
        sVarAreaPtr += ID_SIZEOF(UShort);

        sCID = aLogRec->getUIntValue( sVarAreaPtr, SMI_LOGREC_MV_COLUMN_CID_OFFSET ) & SMI_COLUMN_ID_MASK ;
        sVarAreaPtr += ID_SIZEOF(UInt);

        /* sVCPieceOID  = aLogRec->getvULongValue( sVarAreaPtr );            항상 null oid */
        sVarAreaPtr += ID_SIZEOF(smOID);

        /* sVarColCountInPiece  = aLogRec->getUShortValue ( sVarAreaPtr );  항상 1 */
        sVarAreaPtr += ID_SIZEOF(UShort);

        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

        aCidArray[sCID] = sCID;

        /* value 의 앞부분에 있는 offset array에서 offset을 읽어온다 */
        sCurrVarOffset = aLogRec->getUShortValue( sVarAreaPtr, ID_SIZEOF(UShort) * 0 ); 
        sNextVarOffset = aLogRec->getUShortValue( sVarAreaPtr, ID_SIZEOF(UShort) * 1 ); 

        aAfterColsArray[sCID].length = sNextVarOffset - sCurrVarOffset;

        if ( sNextVarOffset == sCurrVarOffset )
        {
            aAfterColsArray[sCID].value = NULL;
        }
        else
        {
            aAfterColsArray[sCID].value  = sVarAreaPtr + sCurrVarOffset - ID_SIZEOF(smVCPieceHeader);
        }

        sVarAreaPtr += sNextVarOffset - ID_SIZEOF(smVCPieceHeader);
    }
    else
    {
        /* Nothing to do */
    }

    /* extract Large variable fields & LOB fields */
    for (sVarColPtr = sVarAreaPtr; sVarColPtr < aAfterImagePtrFence; )
    {
        sCID = aLogRec->getUIntValue( sVarColPtr, SMI_LOGREC_MV_COLUMN_CID_OFFSET )
               & SMI_COLUMN_ID_MASK;

        IDE_TEST_RAISE(sCID > SMI_COLUMN_ID_MAXIMUM, ERR_TOO_LARGE_CID);

        IDE_TEST( smiGetTableColumns( sTableHandle,
                                      sCID,
                                      &spCol )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (spCol->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_FIXED,
                        err_fixed_col_in_var_area );

        aCidArray[sCID] = sCID;

        sAfterColSize = aLogRec->getUIntValue(sVarColPtr, SMI_LOGREC_MV_COLUMN_SIZE_OFFSET);
        if ( ( SMI_IS_LOB_COLUMN(spCol->flag) == ID_TRUE ) && ( sAfterColSize != 0 ) )
        {
            aAfterColsArray[sCID].length = sAfterColSize + SMI_LOB_DUMMY_HEADER_LEN;
            aAfterColsArray[sCID].value  = sVarColPtr + SMI_LOGREC_MV_COLUMN_DATA_OFFSET - SMI_LOB_DUMMY_HEADER_LEN;
        }
        else
        {
            aAfterColsArray[sCID].length = sAfterColSize;
            aAfterColsArray[sCID].value  = sVarColPtr + SMI_LOGREC_MV_COLUMN_DATA_OFFSET;
        }
        sVarColPtr += SMI_LOGREC_MV_COLUMN_DATA_OFFSET + sAfterColSize;

        /* Variable/LOB Column Value 인 경우에는 OID List를 건너뛰어야 함 */
        /* 여기에는 OID count가 저장되어 있으므로, Count를 읽은 후 그 수 만큼 */
        /* 건너뛰도록 한다. */
        sOIDCnt = aLogRec->getUIntValue(sVarColPtr, 0);
        sVarColPtr += ID_SIZEOF(UInt) + (sOIDCnt * ID_SIZEOF(smOID));
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_fixed_col_in_var_area);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageDictionary] "
                         "Fixed Column [CID:%"ID_UINT32_FMT"] is in Variable Area at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(ERR_TOO_LARGE_CID);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageDictionary] Too Large Column ID [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sCID, 
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION(err_too_big_fixed_area_size);
    {
        idlOS::snprintf( sErrorBuffer, 
                         256,
                         "[smiLogRec::analyzeInsertLogAfterImageDictionary] "
                         "Fixed Area Size [%"ID_UINT32_FMT"] is over page size [%"ID_UINT32_FMT"] at "
                         "[LSN: %"ID_UINT32_FMT",%"ID_UINT32_FMT")]",
                         sFixedAreaSize, 
                         SM_PAGE_SIZE,
                         aLogRec->getRecordLSN().mFileNo, 
                         aLogRec->getRecordLSN().mOffset );
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, sErrorBuffer) );
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Check Error.");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*  not used
smiLogType smiLogRec::getLogTypeFromLogHdr( smiLogHdr    * aLogHead )
{
    smiLogType      sLogType = SMI_LT_NULL;

    switch ( smrLogHeadI::getType( aLogHead ) )
    {
        case SMR_LT_MEMTRANS_COMMIT:
        case SMR_LT_DSKTRANS_COMMIT:
            sLogType = SMI_LT_TRANS_COMMIT;
            break;

        case SMR_LT_MEMTRANS_GROUPCOMMIT:
            sLogType = SMI_LT_TRANS_GROUPCOMMIT;
            break;

        case SMR_LT_MEMTRANS_ABORT:
        case SMR_LT_DSKTRANS_ABORT:
            sLogType = SMI_LT_TRANS_ABORT;
            break;

        case SMR_LT_TRANS_PREABORT:
            sLogType = SMI_LT_TRANS_PREABORT;
            break;

        case SMR_LT_SAVEPOINT_SET:
            sLogType = SMI_LT_SAVEPOINT_SET;
            break;

        case SMR_LT_SAVEPOINT_ABORT:
            sLogType = SMI_LT_SAVEPOINT_ABORT;
            break;

        case SMR_LT_FILE_END:
            sLogType = SMI_LT_FILE_END;
            break;

        case SMR_LT_UPDATE:
            sLogType = SMI_LT_MEMORY_CHANGE;
            break;

        case SMR_DLT_REDOONLY:
        case SMR_DLT_UNDOABLE:
            sLogType = SMI_LT_DISK_CHANGE;
            break;

        case SMR_LT_LOB_FOR_REPL:
            sLogType = SMI_LT_LOB_FOR_REPL;
            break;

        case SMR_LT_DDL :           // DDL Transaction임을 표시하는 Log Record
            sLogType = SMI_LT_DDL;
            break;

        case SMR_LT_TABLE_META :    // Table Meta Log Record
            sLogType = SMI_LT_TABLE_META;
            break;

        case SMR_LT_DDL_QUERY_STRING :
            sLogType = SMI_LT_DDL_QUERY_STRING;
            break;

        default:
            sLogType = SMI_LT_NULL;
            break;
    }

    return sLogType;
}
*/

