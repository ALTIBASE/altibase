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
 * $Id: rpdLogAnalyzer.h 90444 2021-04-02 10:15:58Z minku.kang $
 **********************************************************************/

#ifndef _O_RPD_LOG_ANALYZER_H_
#define _O_RPD_LOG_ANALYZER_H_ 1

#include <smiLogRec.h>
#include <qci.h>

class rpdLogAnalyzer;
typedef IDE_RC (*rpdAnalyzeLogFunc)(rpdLogAnalyzer *aAnlz,
                                    smiLogRec      *aLogRec,
                                    smTID           aTID,
                                    smSN            aSN);

typedef struct rpValueLen
{
    UShort lengthSize; // ID_SIZEOF(UChar), ID_SIZEOF(UShort), ID_SIZEOF(UInt)에 해당하는 값.
    UShort lengthValue;// mtdValue의 앞에 오는 mtdValueLengh값
} rpValueLen;

/*
 *   rpdLogAnalyzer analyzes log record,
 *   to fill the protocol data members of rpdXLog.
 */
typedef struct rpdDictionaryValue
{
    smOID       mDictValueOID;
    smiValue    mValue;
    iduListNode mNode;
} rpdDictionaryValue;

class rpdLogAnalyzer
{
public :
    /* 현재 분석중인 XLog의 타입 */
    rpXLogType  mType;
    /* 현재 분석중인 Log의 트랜잭션 ID */
    smTID       mTID;

    ID_XID      mXID;
    smSCN       mGlobalCommitSCN;

    smTID       mSendTransID;

    /* 현재 로그의 SN */
    smSN        mSN;
    /* 현재 분석중인 Log의 테이블 OID */
    ULong       mTableOID;

    /* Column ID Array */
    UInt            mCIDs[QCI_MAX_COLUMN_COUNT]; // 초기값 UINT_MAX

    /* 현재 분석중인 로그의 Before Image Column Value Array */
    smiValue        mBCols[QCI_MAX_COLUMN_COUNT];
    /* [PROJ-1705] 디스크 테이블의 before image */
    smiChainedValue mBChainedCols[QCI_MAX_COLUMN_COUNT];
    /* [PROJ-1705] chained value의 total length - 초기화하지 않는다 */
    UInt            mChainedValueTotalLen[QCI_MAX_COLUMN_COUNT];

    /* 현재 분석중인 로그의 After Image Column Value Array */
    smiValue        mACols[QCI_MAX_COLUMN_COUNT];

    /* 현재 분석중인 로그의 Priamry Key Column Value의 Array */
    smiValue        mPKCols[QCI_MAX_KEY_COLUMN_COUNT];
    /* 현재 분석중인 로그의 Primary Key Column Count */
    UInt            mPKColCnt;
    /* 현재 분석중인 로그의 Primary Key Column의 Column ID Array */
    UInt            mPKCIDs[QCI_MAX_KEY_COLUMN_COUNT];
    ULong           mPKArea[SM_PAGE_SIZE/ID_SIZEOF(ULong)];

    /* [PROJ-1705] PK value 앞에 붙는 mtdValueLen 정보 */
    rpValueLen      mPKMtdValueLen[QCI_MAX_KEY_COLUMN_COUNT];
    /* [PROJ-1705] After value 앞에 붙는 mtdValueLen 정보 */
    rpValueLen      mAMtdValueLen[QCI_MAX_COLUMN_COUNT];
    /* [PROJ-1705] Before value 앞에 붙는 mtdValueLen 정보 */
    rpValueLen      mBMtdValueLen[QCI_MAX_COLUMN_COUNT];

    /* [PROJ-1705] Analyzed Redo Column의 개수 : Insert에서 사용 */
    UShort          mRedoAnalyzedColCnt;
    /* [PROJ-1705] Analyzed Undo Column의 개수 : before image array 순서대로 넣기 위함 */
    UShort          mUndoAnalyzedColCnt;

    /* Savepoint 이름의 길이 */
    UInt        mSPNameLen;
    SChar       mSPName[RP_SAVEPOINT_NAME_LEN + 1];

    UInt        mImplSPDepth; //implicit svp stmt depth
    /* Flush Option */
    UInt        mFlushOption;

    /* LOB Operation Argument */
    ULong       mLobLocator;
    UInt        mLobColumnID;
    UInt        mLobOffset;
    UInt        mLobOldSize;
    UInt        mLobNewSize;
    UInt        mLobPieceLen;
    SChar      *mLobPiece;

    /* PROJ-1705 현재 분석중인 Redo Column Value에서 진행된 길이 */
    UInt        mRedoAnalyzedLen;
    /* PROj-1705 현재 분석중인 Undo Column Value에서 진행된 길이 */
    UInt        mUndoAnalyzedLen;
    /* PROJ-1705 현재 분석중인 Lob Column Value에서 진행된 길이 */
    UInt        mLobAnalyzedLen;

    /* 다음 로그로 이어지는 지 아닌지에 대한 플래그 */
    idBool      mIsCont;

    /* BCols, ACols 메모리를 free 시켜주어야 하는 경우 표시 플래그 */
    idBool      mNeedFree;

    /* mtdValue로의 변환여부 */
    idBool      mNeedConvertToMtdValue;

    iduMemAllocator * mAllocator;
    iduList           mDictionaryValueList;
private :
    /* smiLogRec의 것과 같은 Memory Pool */
    iduMemPool *mChainedValuePool;

    // PROJ-1705
    idBool      mSkipXLog;

public :
    IDE_RC initialize( iduMemAllocator * aAllocator, iduMemPool * aChainedValuePool );
    void   destroy();
    IDE_RC analyze( smiLogRec *aLog,
                    idBool    *aIsDML,
                    smTID      aTID );
    void   freeColumnValue(idBool aIsAborted);
    void   resetVariables(idBool aNeedInitMtdValueLen,  // BUG-28564
                          UInt   aTableColCount);       // BUG-31103

    /* PROJ-2397 Compressed Table Replication */
    void                 insertDictValue( rpdDictionaryValue *aValue );
    void                 destroyDictValueList();
    rpdDictionaryValue*  getDictValueFromList( smOID aOID );

    inline idBool   isCont() { return mIsCont; };
    inline void     setNeedFree(idBool aFlag) { mNeedFree = aFlag; }
    inline idBool   getNeedFree() { return mNeedFree; }

    inline void     setXLogHdr( rpXLogType aType,
                                smTID      aTID,
                                smSN       aSN)
    {
        mType   = aType;
        mTID    = aTID;
        mSN     = aSN;
        mSendTransID = aTID;
    }

    inline void     convertUpdateToDelete(smiLogType /*aLogType*/)
    {
        mType = RP_X_DELETE;
    }

    // PROJ-1705
    inline idBool skipXLog()
    {
        return mSkipXLog;
    };
    inline void setSkipXLog(idBool aSkipXLog)
    {
        mSkipXLog = aSkipXLog;
    };

    idBool isInCIDArray( UInt aCID );

    void    setSendTransID( smTID     aTransID );
    smTID   getSendTransID( void );
    smSCN   getGlobalCommitSCN( void );

    inline void cancelAnalyzedLog()
    {
        if ( mIsCont == ID_TRUE )
        {
            mIsCont = ID_FALSE;
            freeColumnValue( ID_TRUE );
            setNeedFree( ID_FALSE );
        }
        else
        {
            /*do nothing*/
        }
    }
public:

    // Ananlyze Log List
    static rpdAnalyzeLogFunc  mAnalyzeFunc[SMI_CHANGE_MAXMAX+1];

    static IDE_RC anlzInsMem(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzUdtMem(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzDelMem(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);

    static IDE_RC anlzWriteDskLobPiece(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobCurOpenMem(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobCurOpenDisk(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobCurClose(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobPrepare4Write(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobFinish2Write(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobPartialWriteMem(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzLobPartialWriteDsk(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);
    static IDE_RC anlzNA(rpdLogAnalyzer *, smiLogRec*, smTID, smSN);

    // PROJ-1705
    static IDE_RC anlzPKDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzRedoInsertDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzRedoUpdateDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzRedoDeleteDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzRedoUpdateInsertRowPieceDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzRedoUpdateOverwriteDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    // TASK-5030 
    static IDE_RC anlzUndoDeleteDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzUndoUpdateDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzUndoUpdateDeleteRowPieceDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzUndoUpdateOverwriteDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);
    static IDE_RC anlzUndoUpdateDeleteFirstColumnDisk(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);

    // PROJ-2047 Strengthening LOB
    static IDE_RC anlzLobTrim(rpdLogAnalyzer *, smiLogRec *, smTID, smSN);

    // PROJ-2397 Compressed Table Replication
    static IDE_RC anlzInsDictionary( rpdLogAnalyzer *aLA,
                                     smiLogRec      *aLog,
                                     smTID           aTID,
                                     smSN            aSN );
};

/* For Pirmary Key
 *
 *     Fixed Column : Column ID | SIZE | DATA
 *     Var   Column : Column ID | SIZE | DATA
 *
 */
#define RP_PK_COLUMN_CID_OFFSET   ( 0 )
#define RP_PK_COLUMN_SIZE_OFFSET  ( RP_PK_COLUMN_CID_OFFSET  + ID_SIZEOF(UInt)/*Column ID*/ )
#define RP_PK_COLUMN_DATA_OFFSET  ( RP_PK_COLUMN_SIZE_OFFSET + ID_SIZEOF(UInt)/*Size*/ )

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
 *        Var   Column :
 *            1. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_OUT
 *               - Column ID(UInt) | Length(UInt) | Value | OID ... 들
 *
 *            2. SMC_VC_LOG_WRITE_TYPE_AFTERIMG & SMP_VCDESC_MODE_IN
 *               - Fixed Row 로그에 데이타가 저장되어 있기때문에
 *                 로깅할 필요가 없다.
 */
/* Before Image, After Image VC Column */
#define RP_MV_COLUMN_CID_OFFSET    ( 0 )
#define RP_MV_COLUMN_SIZE_OFFSET   ( RP_MV_COLUMN_CID_OFFSET + ID_SIZEOF(UInt)/*Column ID*/ )
#define RP_MV_COLUMN_DATA_OFFSET   ( RP_MV_COLUMN_SIZE_OFFSET + ID_SIZEOF(UInt)/*Size*/ )
#define RP_MV_COLUMN_OIDLST_SIZE( aMaxSize, aValueSize ) \
             ( ((aValueSize + aMaxSize - 1) / aMaxSize) * ID_SIZEOF(smOID) )

/* After Image, Fixed Row */
#define RP_MV_FIXED_ROW_SIZE_OFFSET  ( 0 )
#define RP_MV_FIXED_ROW_DATA_OFFSET  ( ID_SIZEOF(UShort)/*Size*/ )


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
#define RP_UI_COLUMN_FLAG_OFFSET  ( 0 )
#define RP_UI_COLUMN_CID_OFFSET   ( ID_SIZEOF(SChar)/*Flag*/ + ID_SIZEOF(UInt)/*Offset*/ )
#define RP_UI_COLUMN_SIZE_OFFSET  ( RP_UI_COLUMN_CID_OFFSET + ID_SIZEOF(UInt)/*Column ID*/ )
#define RP_UI_COLUMN_DATA_OFFSET  ( RP_UI_COLUMN_SIZE_OFFSET + ID_SIZEOF(UInt)/*Size*/ )


#endif /* _O_RPD_LOG_ANALYZER_H_ */
