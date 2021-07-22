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
 * $Id: smiLogRec.h 90444 2021-04-02 10:15:58Z minku.kang $
 **********************************************************************/

#ifndef _O_SMI_LOG_REC_H_
#define _O_SMI_LOG_REC_H_ 1

#include <smDef.h>
#include <smiDef.h>
#include <smr.h>
#include <smiTable.h>


// PROJ-2047
#define SMI_LOB_DUMMY_HEADER_LEN    (1)

// PROJ-1705
#define SMI_NON_CHAINED_VALUE       (0)
#define SMI_FIRST_CHAINED_VALUE     (1)
#define SMI_MIDDLE_CHAINED_VALUE    (2)
#define SMI_LAST_CHAINED_VALUE      (3)

// PROJ-1705
#define SMI_NON_ALLOCED             (0x00)
#define SMI_MEMPOOL_ALLOC           (0x01)
#define SMI_NORMAL_ALLOC            (0x02)

// PROJ-1705
#define SMI_NULL_VALUE              (0xFF)
#define SMI_LONG_VALUE              (0xFE)
#define SMI_SHORT_VALUE             (0xFA)
#define SMI_OUT_MODE_LOB            (0xFD)

// PROJ-1705
#define SMI_UNDO_LOG                (-1)
#define SMI_REDO_LOG                (0)

typedef struct smiExtVar
{
    smiValue * pVal;
    sdRID      RID;
} smiExtVar;


#define smiLogHdr smrLogHead

typedef smrDummyLog smiDummyLog;

typedef struct smiLogUnion
{
    /* smiReadLogByOrder에서 읽은 Log Header를 가리킨다.*/
    smiLogHdr    *mCommon;

    /* 아래 Member는 smiLogHdr를 제외한 나머지 부분만
       logfile에서 읽어서 복사한다. smiLogRec::readFrom 참조바람..*/
    union
    {
        smrUpdateLog    mMemUpdate;
        smrDiskLog      mDisk;
        smrLobLog       mLob;
        smrTableMetaLog mTableMetaLog;  // Tabe Meta Log Record
        smrDDLStmtMeta  mDDLStmtMeta;
        smrTransCommitLog mCommitLog;
        smrXaLog          mXaLog;
    };
} smiLogUnion;

// PROJ-1705
typedef struct smiChainedValue
{
    smiValue          mColumn;      // column의 value와 length를 저장
    SChar             mAllocMethod; // memory 할당받은 방식
    smiChainedValue * mLink;        // pool element보다 컬럼의 value가 큰 경우,
                                    // 여러 element를 list로 연결하여 사용한다.
} smiChainedValue;

typedef enum
{
    SMI_LT_NULL = 0,
    SMI_LT_TRANS_COMMIT,         // transaction commit
    SMI_LT_TRANS_ABORT,          // transaction abort
    SMI_LT_TRANS_PREABORT,       // transaction abort하기 직전의 log
    SMI_LT_SAVEPOINT_SET,        // savepoint 설정
    SMI_LT_SAVEPOINT_ABORT,      // savepoint 철회
    SMI_LT_FILE_END,
    SMI_LT_MEMORY_CHANGE,        // memory의 update 로그
    SMI_LT_DISK_CHANGE,          // disk의 update 로그
    SMI_LT_LOB_FOR_REPL,         // LOB operation 로그
    SMI_LT_DDL = 10,             // DDL Transaction
    SMI_LT_TABLE_META,           // Table Meta
    SMI_LT_DDL_QUERY_STRING,     // DDL Statement
    SMI_LT_TRANS_GROUPCOMMIT,
    SMI_LT_XA_START_REQ,         /* PROJ-2747 Global Tx Consistency */
    SMI_LT_XA_PREPARE_REQ,
    SMI_LT_XA_PREPARE,
    SMI_LT_XA_END 
} smiLogType;

// PROJ-1723
typedef smrDDLStmtMeta smiDDLStmtMeta;

typedef enum
{
    SMI_CHANGE_NULL = 0,
    SMI_CHANGE_MRDB_INSERT,
    SMI_CHANGE_MRDB_UPDATE,
    SMI_CHANGE_MRDB_DELETE,

    SMI_PK_DRDB,                               // PK info for delete or update DML
    SMI_REDO_DRDB_INSERT,                      // REDO : INSERT DML
    SMI_REDO_DRDB_DELETE,                      // REDO : DELETE DML
    SMI_REDO_DRDB_UPDATE,                      // REDO : UPDATE DML
    SMI_REDO_DRDB_UPDATE_INSERT_ROW_PIECE,     // REDO : UPDATE DML
    SMI_REDO_DRDB_UPDATE_DELETE_ROW_PIECE,     // REDO : UPDATE DML
    SMI_REDO_DRDB_UPDATE_OVERWRITE,            // REDO : UPDATE DML
    SMI_REDO_DRDB_UPDATE_DELETE_FIRST_COLUMN,  // REDO : UPDATE DML
    SMI_UNDO_DRDB_INSERT,                      // UNDO : INSERT DML
    SMI_UNDO_DRDB_DELETE,                      // UNDO : DELETE DML
    SMI_UNDO_DRDB_UPDATE,                      // UNDO : UPDATE DML
    SMI_UNDO_DRDB_UPDATE_INSERT_ROW_PIECE,     // UNDO : UPDATE DML
    SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE,     // UNDO : UPDATE DML
    SMI_UNDO_DRDB_UPDATE_OVERWRITE,            // UNDO : UPDATE DML
    SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN,  // UNDO : UPDATE DML
    SMI_CHANGE_DRDB_LOB_PIECE_WRITE,
    SMI_CHANGE_MRDB_LOB_CURSOR_OPEN,
    SMI_CHANGE_DRDB_LOB_CURSOR_OPEN,
    SMI_CHANGE_LOB_CURSOR_CLOSE,
    SMI_CHANGE_LOB_PREPARE4WRITE,
    SMI_CHANGE_LOB_FINISH2WRITE,
    SMI_CHANGE_MRDB_LOB_PARTIAL_WRITE,
    SMI_CHANGE_DRDB_LOB_PARTIAL_WRITE,
    SMI_CHANGE_LOB_TRIM,
    SMI_CHANGE_DRDB_LOCK_ROW,
    SMI_CHANGE_MAXMAX
} smiChangeLogType;

typedef enum
{
    SMI_LOG_REC_USE_MODE_NORMAL = 1,
    // used only to get the smrLogFile*
    SMI_LOG_REC_USE_MODE_DUMMY
} smiLogRecUseMode;

/* For Pirmary Key
 *
 *     Fixed Column : SIZE | Column ID | DATA
 *     Var   Column : SIZE | Column ID | DATA
 *
 */
#define SMI_LOGREC_PK_COLUMN_SIZE_OFFSET   ( 0 )
#define SMI_LOGREC_PK_COLUMN_CID_OFFSET   ( SMI_LOGREC_PK_COLUMN_SIZE_OFFSET  + ID_SIZEOF(UInt)/*Column ID*/ )
#define SMI_LOGREC_PK_COLUMN_DATA_OFFSET  ( SMI_LOGREC_PK_COLUMN_CID_OFFSET + ID_SIZEOF(UInt)/*Size*/ )

/* For MVCC
 *
 *     Before Image : Sender가 읽어서 보내는 로그에 대해서만 기록된다.
 *                    각각의 Update되는 Column에 대해서
 *        Fixed Column : Column ID | SIZE | DATA
 *        Var   Column :
 *            1. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SMP_VCDESC_MODE_OUT
 *               - Column ID(UInt) | Length(UInt) | Value
 *
 *            2. SMC_VC_LOG_WRITE_TYPE_BEFORIMG & SMP_VCDESC_MODE_IN
 *               - Column ID(UInt) | Length(UInt) | Value
 *
 *     After  Image: Header를 제외한 Fixed Row 전체와 Variable Column에
 *                   대한 Log를 기록.
 *        Fixed Column :
 *                   Fixed Row Size(UShort) + Fixed Row Data
 *
 *        Var/LOB Column :
 *            1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_OUT
 *               - Column ID(UInt) | Length(UInt) | Value | OID count(UInt) | OID ... 들
 *
 *            2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_IN
 *               - Fixed Row 로그에 데이타가 저장되어 있기때문에
 *                 로깅할 필요가 없다.
 */
/* Before Image, After Image VC Column */
#define SMI_LOGREC_MV_COLUMN_CID_OFFSET    ( 0 )
#define SMI_LOGREC_MV_COLUMN_SIZE_OFFSET   ( SMI_LOGREC_MV_COLUMN_CID_OFFSET + ID_SIZEOF(UInt)/*Column ID*/ )
#define SMI_LOGREC_MV_COLUMN_DATA_OFFSET   ( SMI_LOGREC_MV_COLUMN_SIZE_OFFSET + ID_SIZEOF(UInt)/*Size*/ )
#define SMI_LOGREC_MV_COLUMN_OIDLST_SIZE( aMaxSize, aValueSize ) \
             ( ((aValueSize + aMaxSize - 1) / aMaxSize) * ID_SIZEOF(smOID) )

/* After Image, Fixed Row */
#define SMI_LOGREC_MV_FIXED_ROW_SIZE_OFFSET  ( 0 )
#define SMI_LOGREC_MV_FIXED_ROW_DATA_OFFSET  ( ID_SIZEOF(UShort)/*Size*/ )


/* For Update Inplace Log
 *      Befor  Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt)  |ColumnID(UInt) | SIZE(UInt)
 *                        | Value
 *
 *         Var   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SMP_VCDESC_MODE_OUT:
 *                        | Value | OID 들...
 *               SMP_VCDESC_MODE_IN:
 *                        | Value
 *
 *      After  Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *                        | Value
 *
 *         Var   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SMP_VCDESC_MODE_OUT:
 *                        | Value | OID 들...
 *               SMP_VCDESC_MODE_IN:
 *                        | Value
 */
#if 0 //not used
#define SMI_LOGREC_UI_COLUMN_FLAG_OFFSET  ( 0 )
#define SMI_LOGREC_UI_COLUMN_CID_OFFSET   ( ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ )
#define SMI_LOGREC_UI_COLUMN_SIZE_OFFSET  ( SMI_LOGREC_UI_COLUMN_CID_OFFSET + ID_SIZEOF(UInt)/*Column ID*/ )
#define SMI_LOGREC_UI_COLUMN_DATA_OFFSET  ( SMI_LOGREC_UI_COLUMN_SIZE_OFFSET + ID_SIZEOF(UInt)/*Size*/ )
#endif 

#if 0  //not used
#define SMI_LOGREC_READ_VALUE(aPtr, aRet, aSize)                \
    IDE_DASSERT((aPtr) != NULL);                                \
    IDE_DASSERT((aRet) != NULL);                                \
    IDE_DASSERT((aSize) > 0);                                   \
    IDE_DASSERT( ID_SIZEOF(*(aRet)) == (aSize) );               \
    idlOS::memcpy( (void*)(aRet), (void*)(aPtr), (aSize) )
#endif

#define SMI_LOGREC_READ_AND_MOVE_PTR(aPtr, aRet, aSize)         \
    IDE_DASSERT( ID_SIZEOF(*(aPtr)) == 1 );                     \
    ID_READ_VALUE(aPtr, aRet, aSize);                           \
    (aPtr) += (aSize)

class smiLogRec;

// log analyze function
typedef IDE_RC (*smiAnalyzeLogFunc)(smiLogRec * aLogRec,
                                    UInt      * aPKColCnt,
                                    UInt      * aPKCIDArray,
                                    smiValue  * aPKColValueArray,
                                    UInt      * aColCnt,
                                    UInt      * aCIDArray,
                                    smiValue  * aBColValueArray,
                                    smiValue  * aAColValueArray,
                                    UInt      * aColIdx,
                                    UInt      * aAnalyzedValueLen,
                                    idBool    * aDoWait);


class smiLogRec
{

public :

    inline void initialize(const void * aMeta,
                           iduMemPool * aChainedValuePool,
                           UInt         aChainedValuePoolSize)
    {
        reset();

        /* DML Log Record 분석 정보 초기화 */
        mMeta                 = aMeta;
        mGetTable             = NULL;
        mGetColumnCount       = NULL;
        mGetColumn            = NULL;
        mChainedValuePool     = aChainedValuePool;
        mChainedValuePoolSize = aChainedValuePoolSize;
    };

    inline void reset()
    {
        SM_LSN_MAX( mRecordLSN );
        mLogType         = SMI_LT_NULL;
        mChangeType      = SMI_CHANGE_NULL;
        SC_MAKE_NULL_GRID( mRecordGRID );
        mUpdateColCntInRowPiece = 0;
        mLobColumnCnt    = 0;
        mPrimaryKeySize  = 0;
        mLogPtr          = NULL;
        mPrimaryKeyPtr   = NULL;
        mAnalyzeStartPtr = NULL;
        mBytesRead       = 0;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * DML Log Record 분석을 위한 Callback 함수 설정
     * 특정 시점의 DML은 RP에서 보관하고 있는 해당 Meta로 분석해야 한다.
     */
    inline void setCallbackFunction(smGetTbl4ReplFunc       aGetTable,
                                    smGetColumnCnt4ReplFunc aGetColumnCount,
                                    smGetColumn4ReplFunc    aGetColumn)
    {
        mGetTable       = aGetTable;
        mGetColumnCount = aGetColumnCount;
        mGetColumn      = aGetColumn;
    }

    inline idBool isBeginLog();

    inline void  incLogReadSize(UInt aOffset) { mBytesRead += aOffset; }
    inline UInt  getLogReadSize() { return mBytesRead; }


    inline SChar* getLogPtr()     { return mLogPtr; };
    inline SChar  getSCharValue (SChar*  aLogPtr, UInt aOffset = 0)
    {
        SChar sValue;
        idlOS::memcpy(&sValue, aLogPtr + aOffset, ID_SIZEOF(SChar));
        return sValue;
    };

    inline UInt   getUIntValue (UInt aOffset)
    {
        UInt sValue;
        idlOS::memcpy(&sValue, mLogPtr + aOffset, ID_SIZEOF(UInt));
        return sValue;
    };

    inline UShort getUShortValue(SChar*  aLogPtr, UInt aOffset = 0)
    {
        UShort sValue;
        idlOS::memcpy(&sValue, aLogPtr + aOffset, ID_SIZEOF(UShort));
        return sValue;
    };

    inline UInt  getUIntValue(SChar*  aLogPtr, UInt aOffset = 0)
    {
        UInt sValue;
        idlOS::memcpy(&sValue, aLogPtr + aOffset, ID_SIZEOF(UInt));
        return sValue;
    };

    inline vULong  getvULongValue(UInt aOffset)
    {
        vULong  sValue;
        idlOS::memcpy(&sValue, mLogPtr + aOffset, ID_SIZEOF(vULong));
        return sValue;
    };

    inline vULong  getvULongValue(SChar* aLogPtr, UInt aOffset = 0)
    {
        vULong  sValue;
        idlOS::memcpy(&sValue, aLogPtr + aOffset, ID_SIZEOF(vULong));
        return sValue;
    };

    inline ULong  getULongValue(SChar* aLogPtr, UInt aOffset = 0)
    {
        ULong  sValue;
        idlOS::memcpy(&sValue, aLogPtr + aOffset, ID_SIZEOF(ULong));
        return sValue;
    };

    /* BUG-47525 Group Commit */
    static inline UInt getGroupCommitCnt( SChar* aLogPtr )
    {
        UInt sGCCnt;
        UInt sOffset = SMR_LOG_GROUP_COMMIT_GROUPCNT_OFFSET;
        idlOS::memcpy(&sGCCnt, aLogPtr + sOffset, ID_SIZEOF(UInt));
        return sGCCnt;
    };

    static inline smTID getNthTIDFromGroupCommit( SChar* aLogPtr , UInt aNumber )
    {
        smTID sTID;
        UInt sOffset;
        sOffset = SMR_LOGREC_SIZE( smrTransGroupCommitLog ) + ( ID_SIZEOF( smTID ) * aNumber * 2 );
        idlOS::memcpy(&sTID, aLogPtr + sOffset, ID_SIZEOF(smTID));
        return sTID;
    };

#if 0
    static inline smTID getNthGlobalTIDFromGroupCommit( SChar* aLogPtr , UInt aNumber )
    {
        smTID sGlobalTID;
        UInt sOffset;
        sOffset = SMR_LOGREC_SIZE( smrTransGroupCommitLog ) + 
                  ( ID_SIZEOF( smTID ) * aNumber * 2 ) + 
                  ID_SIZEOF( smTID );
        idlOS::memcpy(&sGlobalTID, aLogPtr + sOffset, ID_SIZEOF(smTID));
        return sGlobalTID;
    };
#endif

    inline smiLogType getType()
    {
        return mLogType;
    };

    inline smiChangeLogType getChangeType()
    {
        return mChangeType;
    };

    inline void setChangeType(smiChangeLogType aChangeType)
    {
        mChangeType = aChangeType;
    };

    inline smTID getTransID()
    {
        return smrLogHeadI::getTransID(mLogUnion.mCommon);
    };

    /*BUG-17033 replication implicit svp statement depth */
    inline UInt getReplStmtDepth()
    {
        return smrLogHeadI::getReplStmtDepth(mLogUnion.mCommon);
    };

    inline UInt getContType()
    {
        return mLogUnion.mDisk.mContType;
    };

    inline UInt getRefOffset()
    {
        return mLogUnion.mDisk.mRefOffset;
    };

    inline smLSN& getRecordLSN()
    {
        return mRecordLSN;
    };

    inline smSN getRecordSN()
    {
        return SM_MAKE_SN( mRecordLSN );
    };

    inline UInt getLogSize()
    {
        return smrLogHeadI::getSize( mLogUnion.mCommon );
    };

    inline SInt getLogUpdateType()
    {
        return mLogUnion.mMemUpdate.mType;
    };

    // Table Meta Log Record 분석을 위한 기본 정보를 제공
    UInt           getTblMetaLogBodySize();
    void         * getTblMetaLogBodyPtr();
    smiTableMeta * getTblMeta();
    
    UInt             getDDLStmtMetaLogBodySize();
    void           * getDDLStmtMetaLogBodyPtr();
    smiDDLStmtMeta * getDDLStmtMeta();

    inline ID_XID  * getXID()
    {
        return &(mLogUnion.mXaLog.mXID);
    }

    inline smSCN * getGlobalCommitSCN()
    {
        return &(mLogUnion.mCommitLog.mCommitSCN);
    }

    inline iduMemPool * getChainedValuePool()
    {
        return mChainedValuePool;
    }
    // PROJ-1705
    IDE_RC chainedValueAlloc( iduMemAllocator  * aAllocator,
                              smiLogRec        * aLogRec,
                              smiChainedValue ** aValue );

    inline vULong getTableOID()
    {
        if ( mLogType == SMI_LT_TABLE_META )
        {
            return (vULong)mLogUnion.mTableMetaLog.mTableMeta.mOldTableOID;
        }
        else
        {
            if ( mLogType == SMI_LT_MEMORY_CHANGE )
            {
                return mLogUnion.mMemUpdate.mData;
            }
            else // SMI_LT_DISK_CHANGE
            {
                return mLogUnion.mDisk.mTableOID;
            }
        }
    };

    inline UInt getPrimaryKeySize()
    {
        return mPrimaryKeySize;
    };

    // PROJ-1705
    inline UInt getUpdateColCntInRowPiece()
    {
        return (UInt)mUpdateColCntInRowPiece;
    };
    inline void setUpdateColCntInRowPiece(UShort aUpdateColCntInRowPiece)
    {
        mUpdateColCntInRowPiece = aUpdateColCntInRowPiece;
    };
    // PROJ-1705
    inline UShort getColumnSequence(UShort i)
    {
        return mColumnSequence[i];
    };
    inline void setColumnSequence(UShort aColSeq, UShort i)
    {
        mColumnSequence[i] = aColSeq;
    };
    // PROJ-1705
    inline UInt getColumnId(UShort i)
    {
        return mColumnId[i];
    };
    inline void setColumnId(UInt aColId, UShort i)
    {
        mColumnId[i] = aColId;
    };
    // PROJ-1705
    inline SInt getColumnTotalLength(UShort i)
    {
        return (SInt)mColumnTotalLen[i];
    };
    inline void setColumnTotalLength(SInt aColTotalLen, UShort i)
    {
        mColumnTotalLen[i] = aColTotalLen;
    };

    // PROJ-1705
    inline UInt getChainedValuePoolSize()
    {
        return mChainedValuePoolSize;
    };

    inline UInt getLobColumnCnt()
    {
        return mLobColumnCnt;
    };
    inline void setLobColumnCnt(UInt aLobColumnCnt)
    {
        mLobColumnCnt = aLobColumnCnt;
    };

    inline SChar* getPrimaryKeyPtr()
    {
        return mPrimaryKeyPtr;
    };

    inline SChar* getAnalyzeStartPtr()
    {
        return mAnalyzeStartPtr;
    };

    inline void setAnalyzeStartPtr(SChar* aAnalyzeStartPtr)
    {
        mAnalyzeStartPtr = aAnalyzeStartPtr;
    };

    inline SInt getBfrImgSize() { return mLogUnion.mMemUpdate.mBImgSize; };
    inline SInt getAftImgSize() { return mLogUnion.mMemUpdate.mAImgSize; };

    inline void  getRecordGRID( scGRID * aRecordGRID)
    {
        SC_COPY_GRID(mRecordGRID, *aRecordGRID);
    };

    inline void  getDictGRID( scGRID * aGRID )
    {
        SC_COPY_GRID( mLogUnion.mMemUpdate.mGRID, *aGRID );
    };

    inline ULong getLobLocator() { return mLogUnion.mLob.mLocator; };

    IDE_RC readFrom ( void  * aLogHeadPtr,
                      void  * aLogPtr,
                      smLSN*  aLSN );
    /*proj-1670*/
    idBool needReplicationByType(void  * aLogHeadPtr,
                                 void  * aLogPtr,
                                 smLSN * aLSN );

    inline idBool needNormalReplicate();
    inline idBool needReplRecovery();
    inline idBool needReplicate();

    /* TASK-5030 */
    inline idBool needSupplementalLog();
    inline idBool checkSavePointFlag();

    void setChangeLogType(void* aLogHdr);

    void getPrimaryKeyInfo( SChar*   aPKStartFence );

    /* analyze DML log */
    static IDE_RC analyzeInsertLogMemory( smiLogRec  *aLogRec,
                                          UShort     *aColCnt,
                                          UInt       *aCIDArray,
                                          smiValue   *aAColValueArray,
                                          idBool     *aDoWait );

    static IDE_RC analyzeUpdateLogMemory( smiLogRec  *aLogRec,
                                          UInt       *aPKColCnt,
                                          UInt       *aPKCIDArray,
                                          smiValue   *aPKColValueArray,
                                          UShort     *aColCnt,
                                          UInt       *aCIDArray,
                                          smiValue   *aBColValueArray,
                                          smiValue   *aAColValueArray,
                                          idBool     *aDoWait );

    static IDE_RC analyzeDeleteLogMemory( smiLogRec  *aLogRec,
                                          UInt       *aPKColCnt,
                                          UInt       *aPKCIDArray,
                                          smiValue   *aPKColValueArray,
                                          idBool     *aDoWait,
                                          UShort     *aColCnt,
                                          UInt       *aCIDs,
                                          smiValue   *aBColValueArray );

    /* TASK-5030 */
    static IDE_RC analyzeFullXLogMemory( smiLogRec  * aLogRec,
                                         SChar      * aXLogPtr,
                                         UShort     * aColCnt,
                                         UInt       * aCIDs,
                                         smiValue   * aBColValueArray,
                                         const void * aTable );

    static IDE_RC analyzeWriteLobPieceLogDisk( iduMemAllocator * aAllocator,
                                               smiLogRec * aLogRec,
                                               smiValue  * aAColValueArray,
                                               UInt      * aCIDArray,
                                               UInt      * aAnalyzedValueLen,
                                               UShort    * aAnalyzedColCnt,
                                               idBool    * aDoWait,
                                               UInt      * aLobCID,
                                               idBool      aIsAfterInsert );

    static IDE_RC analyzeDeleteLogDisk( smiLogRec  *aLogRec,
                                        UInt       *aPKColCnt,
                                        UInt       *aPKCIDArray,
                                        smiValue   *aPKColValueArray );


    static IDE_RC analyzeLobCursorOpenMem(smiLogRec  *aLogRec,
                                          UInt       *aPKColCnt,
                                          UInt       *aPKCIDArray,
                                          smiValue   *aPKColValueArray,
                                          ULong      *aTableOID,
                                          UInt       *aCID);

    static IDE_RC analyzeLobCursorOpenDisk(smiLogRec  *aLogRec,
                                           UInt       *aPKColCnt,
                                           UInt       *aPKCIDArray,
                                           smiValue   *aPKColValueArray,
                                           ULong      *aTableOID,
                                           UInt       *aCID);

    static IDE_RC analyzeLobPrepare4Write( smiLogRec  *aLogRec,
                                           UInt       *aLobOffset,
                                           UInt       *aOldSize,
                                           UInt       *aNewSize );

    static IDE_RC analyzeLobTrim( smiLogRec  *aLogRec,
                                  UInt       *aLobOffset );

    static IDE_RC analyzeLobPartialWriteMemory( iduMemAllocator * aAllocator,
                                                smiLogRec  *aLogRec,
                                                ULong      *aLobLocator,
                                                UInt       *aLobOffset,
                                                UInt       *aLobPieceLen,
                                                SChar     **aLobPiece );

    static IDE_RC analyzeLobPartialWriteDisk( iduMemAllocator * aAllocator,
                                              smiLogRec  *aLogRec,
                                              ULong      *aLobLocator,
                                              UInt       *aLobOffset,
                                              UInt       *aAmount,
                                              UInt       *aLobPieceLen,
                                              SChar     **aLobPiece );

    /* Analyze Sub-function */
    static IDE_RC analyzeInsertLogAfterImageMemory( smiLogRec  * aLogRec,
                                                    SChar      * aAfterImagePtr,
                                                    SChar      * aAfterImagePtrFence,
                                                    UShort     * aColCount,
                                                    UInt       * aCidArray,
                                                    smiValue   * aAfterColsArray,
                                                    const void * aTable );

    static IDE_RC analyzeColumnUIImageMemory( smiLogRec  * aLogRec,
                                              SChar      * aColImagePtr,
                                              SChar      * aColImagePtrFence,
                                              UShort     * aColCount,
                                              UInt       * aCidArray,
                                              smiValue   * aColsArray,
                                              const void * aTable,
                                              idBool       aIsBefore );

    static IDE_RC analyzeColumnMVImageMemory( smiLogRec  * aLogRec,
                                              SChar      * aColImagePtr,
                                              SChar      * aColImagePtrFence,
                                              UShort     * aColCount,
                                              UInt       * aCidArray,
                                              smiValue   * aColsArray,
                                              const void * aTable );

    /*
    static IDE_RC analyzePrimaryKeyImageMemory( smiLogRec *aLog,
                                                SChar     * aColImagePtr,
                                                SChar     * aColImagePtrFence,
                                                UInt      * aColCount,
                                                UInt      * aCidArray,
                                                smiValue  * aColsArray );
    */

    static void analyzeUpdateAfterMultipleColumn( smiLogRec *aLogRec,
                                                  SChar     *aStartPtr,
                                                  UInt      *nReadSize,
                                                  smiValue  *aAfterColumnValueArray,
                                                  UInt      *aColumnIdx,
                                                  UInt      *aColumnValueLength );

    static IDE_RC analyzeDiskBeforeColInfo( smiLogRec *aLogRec,
                                            SChar     *aStartPtr,
                                            UInt      *aBytesRead,
                                            UInt      *aFlag,
                                            UInt      *aCID,
                                            smiValue  *aBeforeColumn);

    static IDE_RC analyzePKMem(smiLogRec  * aLogRec,
                               SChar      * aPKAreaPtr,
                               UInt       * aPKColCnt,
                               UInt       * aPKColSize,
                               UInt       * aPKCIDArray,
                               smiValue   * aPKColValueArray,
                               const void * aTable );

    static IDE_RC analyzePrimaryKey( smiLogRec  * aLogRec,
                                     SChar      * aPKColImagePtr,
                                     UInt       * aPKColCnt,
                                     UInt       * aPKCIDArray,
                                     smiValue   * aPKColsArray,
                                     const void * aTable );

    static IDE_RC analyzePrimaryKeyColumn(smiLogRec  * aLogRec,
                                          SChar      * aPKColPtr,
                                          UInt       * aPKCid,
                                          smiValue   * aPKCol,
                                          const void * aTable,
                                          UInt       * aAnalyzedColSize );

    /* PROJ-1705 */
    static IDE_RC analyzePKDisk(smiLogRec  * aLogRec,
                                UInt       * aPKColCnt,
                                UInt       * aPKCIDArray,
                                smiValue   * aPKColValueArray);

    static void analyzeColumnAndMovePtr(SChar ** aAnalyzePtr,
                                        UShort * aColumnLen,
                                        idBool * aIsOutModeLob,
                                        idBool * aIsNull);

    static IDE_RC analyzeHeader(smiLogRec  * aLogRec,
                                idBool     * aIsContinue);

    static IDE_RC analyzeRPInfo(smiLogRec * aLogRec, SInt aLogType);

    static IDE_RC analyzeUpdateInfo(smiLogRec * aLogRec);

    static IDE_RC analyzeUndoInfo(smiLogRec * aLogRec);

    static IDE_RC analyzeRowImage( iduMemAllocator * aAllocator,
                                   smiLogRec       * aLogRec,              // smiLogRec
                                   UInt            * aCIDArray,            // column id Array
                                   smiValue        * aColValueArray,       // column value Array
                                   smiChainedValue * aChainedColValueArray,// chained column value Array
                                   UInt            * aChainedValueTotalLen,// chained column value 의 전체 길이
                                   UInt            * aAnalyzedValueLen,    // 현재까지 chained value의 분석된 길이
                                   UShort          * aAnalyzedColCnt );    // 지금까지 분석된 컬럼수

    static IDE_RC analyzeColumnValue( iduMemAllocator  * aAllocator,
                                      smiLogRec        * aLogRec,                // smiLogRec
                                      SChar           ** aAnalyzePtr,            // 현재 읽고 있는 지점
                                      UInt             * aCIDArray,              // column id Array
                                      smiValue         * aColValueArray,         // column value Array
                                      smiChainedValue  * aChainedColValueArray,  // chained column value Array
                                      UInt             * aChainedValueTotalLen,  // chained column value 의 전체 길이
                                      UInt             * aAnalyzedValueLen,      // 현재까지 chained value의 분석된 길이
                                      UShort             aColumnCountInRowPiece, // row piece내 컬럼 수
                                      SChar              aRowHdrFlag,            // row header flag
                                      UShort           * aAnalyzedColCnt );      // 지금까지 분석된 컬럼수

    static IDE_RC copyAfterImage( iduMemAllocator  * aAllocator,
                                  const smiColumn  * aColumn,
                                  SChar           ** aAnalyzePtr,
                                  smiValue         * aAColValueArray,
                                  UInt             * aAnalyzedValueLen,
                                  UShort             aColumnLen,
                                  SInt               aColumnTotalLen,
                                  UInt               aColStatus );

    static IDE_RC copyBeforeImage( iduMemAllocator  * aAllocator,
                                   const smiColumn  * aColumn,
                                   smiLogRec        * aLogRec,
                                   SChar            * aAnalyzePtr,
                                   smiChainedValue  * aChainedValue,
                                   UInt             * aChainedValueTotalLen,
                                   UInt             * aAnalyzedValueLen,
                                   UShort             aColumnLen,
                                   UInt               aColStatus );

    static IDE_RC skipOptionalInfo(smiLogRec * aLogRec,
                                   void     ** aAnalyzePtr,
                                   SChar       aRowHdrFlag);

    static UInt checkChainedValue(SChar  aRowHdrFlag,
                                  UInt   aPosition,
                                  UShort aColumnCount,
                                  SInt   aLogType);

    /* Analyze Other log */
    static IDE_RC analyzeTxSavePointSetLog( smiLogRec *aLog,
                                            UInt      *aSPNameLen,
                                            SChar     *aSPName);
    static IDE_RC analyzeTxSavePointAbortLog( smiLogRec *aLog,
                                              UInt      *aSPNameLen,
                                              SChar     *aSPName);

    /* PROJ-1705 */
    static IDE_RC analyzeDeleteRedoLogDisk();
    static IDE_RC analyzeRPInfoDisk();
    static IDE_RC analyzeUndoInfoDisk();
    static IDE_RC analyzeUpdateInfoDisk();
    static IDE_RC analyzeRowImageDisk();
    static IDE_RC analyzePKDisk();
    static void * getRPLogStartPtr4Undo(void             * aLogPtr,
                                        smiChangeLogType   aUndoRecType);
    static void * getRPLogStartPtr4Redo(void             * aLogPtr,
                                        smiChangeLogType   aUndoRecType);
    /* BUG-30118 */
    static idBool isCIDInArray(UInt * aCIDArray,
                               UInt   aCID,
                               UInt   aArraySize);

    static inline void getVCDescInAftImg(const smiColumn *aColumn,
                                         SChar           *aAfterImg,
                                         smVCDesc        *aVCDesc);

    static inline SInt getMMDBUpdHdrSize() { return ID_SIZEOF(smrUpdateLog); };
    static inline SInt getLogTailSize() { return ID_SIZEOF(smrLogTail); };
    static inline UInt getPartLogSizeOfUIAfterImg(SChar aFlag,
                                                  UInt  aSize);

    static inline UInt getPartLogSizeOfUIBeForImg(SChar aFlag,
                                                  UInt  aSize);

    static inline UInt getVCPieceCnt(UInt aSize);

    /*PROJ-1670*/
    static inline void initializeDummyLog(smiDummyLog * aDummyLog);
    static inline void getLogHdr(void* aLogPtr, smiLogHdr *aLogHdrPtr);
    static inline UInt getLogSizeFromLogHdr(smiLogHdr* aLogHdrPtr);
    static inline void setLogSizeOfDummyLog(smiDummyLog * aLogPtr, UInt aSize);
    static inline void setLSN( smiLogHdr *aLogHead, smLSN aLSN );
    static inline void setSNOfDummyLog(smiDummyLog * aLogPtr, smSN aSN);
    static inline smSN getSNFromLogHdr(smiLogHdr *aLogHead);

    /* PROJ-2453 */
    static inline idBool isBeginLogFromHdr( smiLogHdr *aLogHead );
    static inline smTID  getTransIDFromLogHdr( smiLogHdr   * aLogHead );
/* not used
    static smiLogType    getLogTypeFromLogHdr( smiLogHdr   * aLogHead );
*/    

    static inline idBool isDummyLog(smiLogHdr* aLogHdr);

    static void dumpLogHead( smiLogHdr * aLogHead, 
                             UInt aChkFlag, 
                             ideLogModule aModule, 
                             UInt aLevel );

    static     idBool isNeedDecompressFromHdr( smiLogHdr * aLogHead );

    /* PROJ-2397 */
    static     IDE_RC analyzeInsertLogAfterImageDictionary( smiLogRec *aLogRec,
                                                            SChar     *aAfterImagePtr,
                                                            SChar     *aAfterImagePtrFence,
                                                            UShort    *aColCount,
                                                            UInt      *aCidArray,
                                                            smiValue  *aAfterColsArray );
    static     IDE_RC analyzeInsertLogDictionary( smiLogRec  *aLogRec,
                                                  UShort     *aColCnt,
                                                  UInt       *aCIDArray,
                                                  smiValue   *aAColValueArray,
                                                  idBool     *aDoWait );
private:

    // 실제 로그가 위치한 LSN값
    smLSN               mRecordLSN;

    // 로그 레코드의 종류
    smiLogType          mLogType;
    // 로그 레코드의 세부 타입
    smiChangeLogType    mChangeType;
    // 로그가 가리키는 레코드의 GRID
    scGRID              mRecordGRID;

    // [PROJ-1705] row piece 내에서 실제 Update가 발생한 column 수
    UShort              mUpdateColCntInRowPiece;

    // QCI_MAX_COLUMN_COUNT = SMI_COLUMN_ID_MAXIMUM

    // [PROJ-1705] row piece 내에서 column의 위치. 배열에 순차적으로 들어간다.
    UShort              mColumnSequence[SMI_COLUMN_ID_MAXIMUM]; // 초기화 없이 사용
    // [PROJ-1705] row piece 내에서 column id정보. 배열의 seq위치에 들어간다.
    UInt                mColumnId[SMI_COLUMN_ID_MAXIMUM];      // 초기화 없이 사용
    // [PROJ-1705] row 에서 해당 column의 총 길이 정보. 배열의 seq위치에 들어간다.
    SInt                mColumnTotalLen[SMI_COLUMN_ID_MAXIMUM]; // 초기화 없이 사용

    // Disk Update 로그에서 update가 발생한 LOB column 수
    UInt                mLobColumnCnt;       // PROJ-1705
    // Primary Key 영역의 크기 : Update/Delete에서 사용
    UInt                mPrimaryKeySize;

    // 바이너리 형태의 로그 레코드 포인터
    SChar*              mLogPtr;
    // 로그 레코드 내에서 Primary Key의 시작 포인터
    SChar*              mPrimaryKeyPtr;
    // 분석을 시작할 로그 레코드 시작 포인터
    SChar*              mAnalyzeStartPtr;
    // 현재 로그 레코드를 읽어들인 길이
    UInt                mBytesRead;

    // common header
    smiLogUnion         mLogUnion;

    //static smiAnalyzeLogFunc  mAnalyzeLogFunc[SMI_CHANGE_MAXMAX+1];

    // DML Log Record 분석을 위해 필요한 정보
    const void              * mMeta;
    smGetTbl4ReplFunc         mGetTable;
    smGetColumnCnt4ReplFunc   mGetColumnCount;
    smGetColumn4ReplFunc      mGetColumn;

    // PROJ-1705
    iduMemPool              * mChainedValuePool;
    UInt                      mChainedValuePoolSize;

};

inline idBool smiLogRec::isBeginLog()
{
    return smrRecoveryMgr::isBeginLog((smrLogHead*)mLogUnion.mCommon);
}

inline idBool smiLogRec::needNormalReplicate()
{
    idBool sIsNeedNormalReplicate = ID_FALSE;
    UInt   sLogHdrFlag            = smrLogHeadI::getFlag(mLogUnion.mCommon);

    /* BUG-37931 dummy log 가 아닐 때만 rep 대상이 된다. */
    if( (sLogHdrFlag & SMR_LOG_DUMMY_LOG_MASK) != SMR_LOG_DUMMY_LOG_OK )
    {
        if( (sLogHdrFlag & SMR_LOG_TYPE_MASK) == SMR_LOG_TYPE_NORMAL )
        {
            sIsNeedNormalReplicate = ID_TRUE;
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

    return sIsNeedNormalReplicate;
}

inline idBool smiLogRec::needReplRecovery()
{
    idBool sIsNeedReplRecovery  = ID_FALSE;
    UInt   sLogHdrFlag          = smrLogHeadI::getFlag(mLogUnion.mCommon);

    /* BUG-37931 dummy log 가 아닐 때만 rep 대상이 된다. */
    if( (sLogHdrFlag & SMR_LOG_DUMMY_LOG_MASK) != SMR_LOG_DUMMY_LOG_OK )
    {
        if( (sLogHdrFlag & SMR_LOG_TYPE_MASK) == SMR_LOG_TYPE_REPL_RECOVERY )
        {
            sIsNeedReplRecovery =  ID_TRUE;
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

    return sIsNeedReplRecovery;
}

inline idBool smiLogRec::needReplicate()
{
    idBool sIsNeedReplicate = ID_FALSE;
    UInt   sLogHdrFlag      = smrLogHeadI::getFlag(mLogUnion.mCommon);

    if(((sLogHdrFlag & SMR_LOG_TYPE_MASK) == SMR_LOG_TYPE_NORMAL ) ||
       ((sLogHdrFlag & SMR_LOG_TYPE_MASK) == SMR_LOG_TYPE_REPL_RECOVERY ))
    {
        sIsNeedReplicate = ID_TRUE;
    }

    return sIsNeedReplicate;
}


/* TASK-5030 */
inline idBool smiLogRec::needSupplementalLog()
{
    if((smrLogHeadI::getFlag(mLogUnion.mCommon) & SMR_LOG_FULL_XLOG_MASK) 
        == SMR_LOG_FULL_XLOG_OK)
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

inline idBool smiLogRec::checkSavePointFlag()
{
    if((smrLogHeadI::getFlag(mLogUnion.mCommon) & SMR_LOG_SAVEPOINT_MASK)
       == SMR_LOG_SAVEPOINT_OK)
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/***********************************************************************
 * Description : Insert, Update(MVCC)의 After Image로그로 부터
 *               aColumn의 smVCDesc를 구한다.
 *
 * aColumn   - [IN]: Column Dsscriptor
 * aAfterImg - [IN]: Insert, Update(MVCC)의 After Image Log Ptr
 * aVCDesc   - [OUT] : Variable Column Descriptor가 리턴됨.
  ***********************************************************************/
inline void smiLogRec::getVCDescInAftImg(const smiColumn *aColumn,
                                         SChar           *aAfterImg,
                                         smVCDesc        *aVCDesc)
{
    IDE_ASSERT(SMI_IS_FIXED_COLUMN(aColumn->flag) != ID_TRUE);

    idlOS::memcpy(aVCDesc,
                  aAfterImg - ID_SIZEOF(smpSlotHeader)
                  + aColumn->offset + ID_SIZEOF(UShort),
                  ID_SIZEOF(smVCDesc));
}

/***********************************************************************
 * Description : Replication이 보낼 Update Inplace Log의 After Image에
 *               는 각각 Update 된 column단위로 Log가 기록되어 있다.
 *               여기서는 로그에 기록된 각각의 Column의 flag, Size로
 *               Column을 위해 기록된 log의 크기를 구한다.
 *
 * aFlag  - [IN]: SM_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 * aSize  - [IN]: Column Size
 *
 *      After  Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt)  |
 *                        ColumnID(UInt) | SIZE(UInt) | Value
 *
 *         Var   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SM_VCDESC_MODE_OUT:
 *                        | Value | OID Count(UInt) | OID
 *               SM_VCDESC_MODE_IN:
 *                        | Value
 *
  ***********************************************************************/
inline UInt smiLogRec::getPartLogSizeOfUIAfterImg(SChar aFlag,
                                           UInt  aSize)
{
    UInt sLogSize;

    sLogSize = ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ +
        ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*SIZE*/ + aSize;

    if ( (SMI_IS_VARIABLE_COLUMN(aFlag) == ID_TRUE) ||
         (SMI_IS_VARIABLE_LARGE_COLUMN(aFlag) == ID_TRUE) )
    {
        if( ( aFlag >> 1 ) == SM_VCDESC_MODE_OUT )
        {
            sLogSize += getVCPieceCnt(aSize) * ID_SIZEOF(smOID);
        }
    }

    return sLogSize;
}

/***********************************************************************
 * Description : Replication이 보낼 Update Inplace Log의 Before Image에
 *               는 각각 Update 된 column단위로 Log가 기록되어 있다.
 *               여기서는 로그에 기록된 각각의 Column의 flag, Size로
 *               Column을 위해 기록된 log의 크기를 구한다.
 *
 * aFlag  - [IN]: SM_VCDESC_MODE(2st bit) | SMI_COLUMN_TYPE (1st bit)
 * aSize  - [IN]: Column Size
 *
 *      Before Image: 각각의 Update되는 Column에 대해서
 *         Fixed Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *                        | Value
 *
 *         Var   Column : Flag(SChar) | Offset(UInt) | ColumnID(UInt) | SIZE(UInt)
 *               SM_VCDESC_MODE_OUT:
 *                        | Value | OID
 *               SM_VCDESC_MODE_IN:
 *                        | Value
 *
 ***********************************************************************/
inline UInt smiLogRec::getPartLogSizeOfUIBeForImg(SChar aFlag,
                                           UInt  aSize)
{
    UInt sLogSize;

    sLogSize = ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ +
        ID_SIZEOF(UInt)/*ColumnID*/ + ID_SIZEOF(UInt)/*SIZE*/ + aSize;

    if ( (SMI_IS_VARIABLE_COLUMN(aFlag) == ID_TRUE) ||
         (SMI_IS_VARIABLE_LARGE_COLUMN(aFlag) == ID_TRUE) )
    {
        if( ( aFlag >> 1 ) == SM_VCDESC_MODE_OUT )
        {
            sLogSize += ID_SIZEOF(smOID);
        }
    }

    return sLogSize;
}

/***********************************************************************
 * Description : Variable Column의 길이가 aSize일 경우 이 Variable Column을
 *               저장하기 위해 필요한 Variable Column Piece의 갯수를 구한다.
 *
 * aSize : Variable Column의 길이
 *
 ***********************************************************************/
inline UInt smiLogRec::getVCPieceCnt(UInt aSize)
{
    return smcRecord::getVCPieceCount(aSize);
}

inline idBool smiLogRec::isDummyLog(smiLogHdr* aLogHdr)
{
    if(smrLogHeadI::getType(aLogHdr) == SMR_LT_DUMMY)
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

inline void smiLogRec::initializeDummyLog(smiDummyLog * aDummyLog)
{
    IDE_DASSERT( aDummyLog != NULL );
    smrLogHeadI::setType(&aDummyLog->mHead, SMR_LT_DUMMY);
    smrLogHeadI::setTransID(&aDummyLog->mHead, SM_NULL_TID);
    smrLogHeadI::setSize(&aDummyLog->mHead, SMR_LOGREC_SIZE(smrDummyLog));
    smrLogHeadI::setFlag(&aDummyLog->mHead, SMR_LOG_TYPE_NORMAL);
    smrLogHeadI::setPrevLSN(&aDummyLog->mHead,
                            ID_UINT_MAX,  // FILENO
                            ID_UINT_MAX); // OFFSET
    smrLogHeadI::setReplStmtDepth( &aDummyLog->mHead,
                                   SMI_STATEMENT_DEPTH_NULL );
    aDummyLog->mTail = SMR_LT_DUMMY;
}
inline void smiLogRec::getLogHdr(void* aLogPtr, smiLogHdr *aLogHdrPtr)
{
    IDE_ASSERT(aLogHdrPtr != NULL);
    IDE_ASSERT(aLogPtr != NULL);
    idlOS::memcpy(aLogHdrPtr, aLogPtr, ID_SIZEOF(smiLogHdr));
}
inline UInt smiLogRec::getLogSizeFromLogHdr(smiLogHdr* aLogHdrPtr)
{
    return smrLogHeadI::getSize(aLogHdrPtr);
}
inline void smiLogRec::setLSN( smiLogHdr *aLogHead, smLSN aLSN )
{
    smrLogHeadI::setLSN( aLogHead, aLSN );
}
inline void smiLogRec::setSNOfDummyLog(smiDummyLog * aLogPtr, smSN aSN)
{
    smLSN sLSN;
    SM_MAKE_LSN( sLSN, aSN );
    smrLogHeadI::setLSN( &aLogPtr->mHead, sLSN );
}
inline void smiLogRec::setLogSizeOfDummyLog(smiDummyLog * aLogPtr, UInt aSize)
{
    smrLogHeadI::setSize(&aLogPtr->mHead, aSize);
}
inline smSN smiLogRec::getSNFromLogHdr( smiLogHdr *aLogHead )
{
    return SM_MAKE_SN( smrLogHeadI::getLSN( aLogHead ) );
}

/* PROJ-2453 Eager Replication Performance enhancement */
inline smTID smiLogRec::getTransIDFromLogHdr( smiLogHdr   * aLogHead )
{
    return smrLogHeadI::getTransID( aLogHead );
}

inline idBool smiLogRec::isBeginLogFromHdr( smiLogHdr * aLogHead )
{
    return smrRecoveryMgr::isBeginLog( (smrLogHead*)aLogHead );
}

inline idBool smiLogRec::isNeedDecompressFromHdr( smiLogHdr * aLogHead )
{
    idBool sIsNeedDecompress = ID_FALSE;
    UInt   sLogHdrFlag       = smrLogHeadI::getFlag( (smrLogHead*)aLogHead );

    if ( ( ( sLogHdrFlag & SMR_LOG_BEGINTRANS_MASK ) == SMR_LOG_BEGINTRANS_OK )  ||
         ( ( sLogHdrFlag & SMR_LOG_SAVEPOINT_MASK ) == SMR_LOG_SAVEPOINT_OK ) )
    {
        sIsNeedDecompress = ID_TRUE;
    }

    return sIsNeedDecompress;
}

#endif /* _O_SMI_LOG_REC_H_ */
