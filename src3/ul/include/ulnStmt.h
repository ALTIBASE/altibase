/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_ULN_STMT_H_
#define _O_ULN_STMT_H_ 1

#include <ulnPrivate.h>

#include <ulnCursor.h>
#include <ulnCache.h>
#include <ulnDesc.h>

#include <ulsdDef.h>

#define ULN_HAS_DATA_AT_EXEC_PARAM          0x01
#define ULN_HAS_LOB_DATA_AT_EXEC_PARAM      0X02

#define ULN_STMT_MAX_MEMORY_CHUNK_SIZE      (30*1024)

// qp의 QC_MAX_NAME_LEN(40) 대신 사용
#define ULN_MAX_NAME_LEN          (40 + 1)


// PROJ-2727
#define ULN_MAX_VALUE_LEN     (256) //  IDP_MAX_VALUE_LEN

#define ULN_GD_COLUMN_NUMBER_INIT_VALUE     -1
#define ULN_GD_TARGET_POSITION_INIT_VALUE   -1

/* PROJ-2160 CM 타입제거
   파라미터 바인드시에 임시 버퍼에 데이타를 복사하는 메크로 */
#define ULN_CHUNK_WR1(aStmt, aSrc)                                                     \
    do                                                                                 \
    {                                                                                  \
        CM_ENDIAN_ASSIGN1(*((aStmt)->mChunk.mData + (aStmt)->mChunk.mCursor), (aSrc)); \
        (aStmt)->mChunk.mCursor += 1;                                                  \
    } while (0)

#define ULN_CHUNK_WR2(aStmt, aSrc)                                                  \
    do                                                                              \
    {                                                                               \
        CM_ENDIAN_ASSIGN2((aStmt)->mChunk.mData + (aStmt)->mChunk.mCursor, (aSrc)); \
        (aStmt)->mChunk.mCursor += 2;                                               \
    } while (0)

#define ULN_CHUNK_WR4(aStmt, aSrc)                                                  \
    do                                                                              \
    {                                                                               \
        CM_ENDIAN_ASSIGN4((aStmt)->mChunk.mData + (aStmt)->mChunk.mCursor, (aSrc)); \
        (aStmt)->mChunk.mCursor += 4;                                               \
    } while (0)

#define ULN_CHUNK_WR8(aStmt, aSrc)                                                  \
    do                                                                              \
    {                                                                               \
        CM_ENDIAN_ASSIGN8((aStmt)->mChunk.mData + (aStmt)->mChunk.mCursor, (aSrc)); \
        (aStmt)->mChunk.mCursor += 8;                                               \
    } while (0)

#define ULN_CHUNK_WCP(aStmt, aSrc, aSize)                                            \
    do                                                                               \
    {                                                                                \
        acpMemCpy((aStmt)->mChunk.mData + (aStmt)->mChunk.mCursor, (aSrc), (aSize)); \
        (aStmt)->mChunk.mCursor += (aSize);                                          \
    } while (0)


// BUG-38649

typedef acp_uint32_t ulnResultType;

#define ULN_RESULT_TYPE_UNKNOWN   0
#define ULN_RESULT_TYPE_ROWCOUNT  1
#define ULN_RESULT_TYPE_RESULTSET 2

#define ULN_RESULT_TYPE_HAS_ROWCOUNT(aType)  (((aType) & ULN_RESULT_TYPE_ROWCOUNT) == ULN_RESULT_TYPE_ROWCOUNT)
#define ULN_RESULT_TYPE_HAS_RESULTSET(aType) (((aType) & ULN_RESULT_TYPE_RESULTSET) == ULN_RESULT_TYPE_RESULTSET)

#define ULN_DEFAULT_ROWCOUNT      ACP_SINT64_LITERAL(-1)



typedef enum
{
    ULN_STMT_FETCHMODE_BOOKMARK,
    ULN_STMT_FETCHMODE_ROWSET
} ulnStmtFetchMode;

typedef struct ulnResult
{
    acp_list_node_t  mList;

    ulnResultType    mType;

    acp_uint32_t     mFromRowNumber;
    acp_uint32_t     mToRowNumber;
    acp_sint64_t     mAffectedRowCount;
    acp_sint64_t     mFetchedRowCount;
} ulnResult;

typedef struct ulnStmtChunk
{
    acp_uint8_t  *mData;
    acp_uint32_t  mCursor;
    acp_uint32_t  mRowCount;
    acp_uint32_t  mSize;
} ulnStmtChunk;

typedef struct ulnStmtChunkSavePoint
{
    acp_uint32_t  mCursor;
    acp_uint32_t  mRowCount;
} ulnStmtChunkSavePoint;

/* PROJ-1789 Updatable Scrollable Cursor */
typedef enum
{
    ULN_COLUMN_PROCEED,
    ULN_COLUMN_IGNORE
} ulnColumnIgnoreFlag;

struct ulnStmt
{
    ulnObject     mObj;

    ulnDbc       *mParentDbc;

    ulnDesc      *mAttrApd;       /* SQL_ATTR_APP_PARAM_DESC */
    ulnDesc      *mAttrArd;       /* SQL_ATTR_APP_ROW_DESC */
    ulnDesc      *mAttrIpd;       /* SQL_ATTR_IMP_PARAM_DESC */
    ulnDesc      *mAttrIrd;       /* SQL_ATTR_IMP_ROW_DESC */

    /*
     * 사용자가 explicit 하게 할당한 디스크립터를
     * STMT 에다가 바인딩 시켰다가 그것을 해제하면
     * 원래 implicit 하게 할당된 디스크립터로 돌아와야 한다고
     * ODBC 3.0에 정의되어 있음.
     */
    ulnDesc      *mOrigApd;
    ulnDesc      *mOrigArd;

    ulnCursor     mCursor;
    ulnCache     *mCache;

    /* PROJ-1697 2가지 통신 프로토콜 개선 */
    ulnStmtChunk  mChunk;
    acp_uint32_t  mMaxBindDataSize;
    acp_uint32_t  mInOutType;
    acp_uint32_t  mInOutTypeWithoutBLOB;
    acp_bool_t    mBuildBindInfo;

    ulnResult    *mCurrentResult;
    acp_uint32_t  mCurrentResultRowNumber;
    acp_list_t    mResultList;
    acp_list_t    mResultFreeList;

    acp_uint64_t  mTotalAffectedRowCount;
    acp_uint32_t  mAtomicArray;   // PROJ-1518

    acp_uint32_t  mStatementID;   /* ID (Server Side. Server allocates this id)     */
    acp_bool_t    mIsPrepared;    /* Prepare 여부 */

    acp_uint32_t  mStatementType; /* Type (DML, DDL, QUERY 등. prepare result 에 서버가 알려줌 */

    // BUG-17592
    ulnResultType mResultType;    /* SQL 수행 후 결과가 ResultSet인지 AffectedRow인지 구분 */

    acp_uint16_t  mParamCount;            /* PREPARE RES 에 의해 설정되는 파라미터 갯수 */
    acp_uint16_t  mResultSetCount;        /* PREPARE RES 및 EXECUTE RES 에 의해 설정되는 ResultSet 갯수 */
    acp_uint16_t  mCurrentResultSetID;    /* 현재 사용하고 있는 ResultSet의 ID. 1 BASE */

    /*
     * SQLGetData() 를 위해서 필요한 멤버
     */
    acp_sint32_t  mGDColumnNumber;

    /*
     * SQLPutData() 와 SQLParamData() 함수를 위해 필요한 멤버들.
     */
    acp_uint16_t  mProcessingParamNumber;
    acp_uint32_t  mProcessingRowNumber;
    acp_uint32_t  mProcessingLobRowNumber;

    acp_uint8_t   mHasDataAtExecParam;

    acp_bool_t    mHasMemBoundLobParam;
    acp_bool_t    mHasMemBoundLobColumn;

    acp_uint32_t  mFetchedRowCountFromServer; /* 한번의 FETCH REQ 에 의해 수신된 row 의 갯수 */
    acp_uint64_t  mFetchedBytesFromServer;    /* 한번의 FETCH REQ 에 의해 수신된 bytes (PROJ-2625) */
    //fix BUG-17820
    acp_uint16_t  mFetchedColCnt;             /* FetchResult의 컬럼마다 붙는  row number를 제거하기 위하여,
                                                 하나의 row가 fetch가 완료되었는지 여부를 위하여 사용된다.  */

    acp_uint8_t  *mAttrFetchBookmarkPtr;  /* SQL_ATTR_FETCH_BOOKMARK_PTR */
    acp_uint32_t  mAttrUseBookMarks;      /* SQL_ATTR_USE_BOOKMARKS */

    acp_uint32_t  mAttrEnableAutoIPD;     /* SQL_ATTR_ENABLE_AUTO_IPD */

    acp_uint32_t  mAttrNoscan;            /* SQL_ATTR_NOSCAN */

    acp_uint32_t  mAttrQueryTimeout;      /* SQL_ATTR_QUERY_TIMEOUT */
    acp_uint32_t  mAttrRetrieveData;      /* SQL_ATTR_RETRIEVE_DATA */

    acp_uint8_t   mAttrAsyncEnable;       /* SQL_ATTR_ASYNC_ENABLE */

    acp_uint32_t  mAttrRowsetSize;        /* SQL_ROWSET_SIZE. ExtendedFetch 에서만 사용하는 속성 */

    acp_bool_t    mAttrInputNTS;          /* SQL_ATTR_INPUT_NTS : SES -n 옵션. BUG-13704 */

    acp_uint32_t  mAttrPrefetchRows;      /* SQL_ATTR_PREFETCH_ROWS 비표준 속성. 알티베이스 전용 */
    acp_uint32_t  mAttrPrefetchBlocks;    /* SQL_ATTR_PREFETCH_BLOCKS 비표준 속성. 알티베이스 전용 */
    acp_uint32_t  mAttrPrefetchMemory;    /* SQL_ATTR_PREFETCH_MEMORY 비표준 속성. 알티베이스 전용 */
    acp_uint32_t  mAttrCacheBlockSizeThreshold;
    /* SQL_ATTR_CACHE_BLOCK_THRESHOLD 비표준속성 */

    acp_char_t   *mPlanTree;              /* 플랜트리 스트링 */

#if 0
    /*
     * 지원하지 않는 속성들
     */
    acp_uint32_t  mAttrMaxRows;           /* SQL_ATTR_MAX_ROWS : 알티베이스 미지원 */
    acp_uint32_t  mAttrRowNumber;         /* SQL_ATTR_ROW_NUMBER
                                             전체 result set 에서 현재 row 의 번호.
                                             현재의 ul 에서는 안쓰임 */

    acp_uint32_t  mAttrKeysetSize;        /* SQL_ATTR_KEYSET_SIZE */
    acp_uint32_t  mAttrMaxLength;         /* SQL_ATTR_MAX_LENGTH */

    acp_uint32_t  mAttrMetadataID;        /* SQL_ATTR_METADATA_ID : BUG-17016 참조 */
#endif

    /* PROJ-2177 User Interface - Cancel */

    acp_uint32_t  mStmtCID;               /**< StmtID를 아직 모를 때를 위한 Client side Statement ID */
    ulnFunctionId mLastFetchFuncID;       /**< 마지막 Fetch에 사용한 함수 ID. Cancle 후 상태전이를 위해 필요. */
    ulnFunctionId mNeedDataFuncID;        /**< NEED DATA가 발생한 함수 ID. Cancle 후 상태전이를 위해 필요. */

    /* PROJ-1789 Updatable Scrollable Cursor */

    ulnStmt             *mRowsetStmt;
    ulnStmt             *mParentStmt;
    ulnKeyset           *mKeyset;

    acp_char_t          *mPrepareTextBuf;
    acp_sint32_t         mPrepareTextBufMaxLen;

    acp_char_t          *mQstrForRowset;
    acp_sint32_t         mQstrForRowsetMaxLen;
    acp_sint32_t         mQstrForRowsetLen;
    acp_size_t           mQstrForRowsetParamPos;
    acp_sint32_t         mQstrForRowsetParamCnt;

    ulnStmtFetchMode     mFetchMode;
    ulnDesc             *mIrd4KeysetDriven;
    acp_sint64_t         mGDTargetPosition;
    acp_sint64_t         mRowsetCacheStartPosition;

    acp_char_t          *mQstrForInsUpd;
    acp_sint32_t         mQstrForInsUpdMaxLen;
    acp_sint32_t         mQstrForInsUpdLen;

    acp_char_t          *mQstrForDelete;
    acp_sint32_t         mQstrForDeleteMaxLen;
    acp_sint32_t         mQstrForDeleteLen;

    acp_uint8_t         *mRowsetParamBuf;
    acp_sint32_t         mRowsetParamBufMaxLen;
    acp_uint16_t        *mRowsetParamStatusBuf;
    acp_sint32_t         mRowsetParamStatusBufMaxCnt;
    ulnColumnIgnoreFlag *mColumnIgnoreFlagsBuf;
    acp_sint32_t         mColumnIgnoreFlagsBufMaxCnt;

    acp_char_t           mTableNameForUpdate[(ULN_MAX_NAME_LEN + 2) * 2 + 1];
    acp_sint32_t         mTableNameForUpdateLen;

    /* PROJ-1721 Name-based Binding */
    ulnAnalyzeStmt      *mAnalyzeStmt;

    /* PROJ-1891 SQL_ATTR_DEFERRED_PREPARE */
    ulnStateFunc        *mDeferredPrepareStateFunc;
    acp_char_t           mAttrDeferredPrepare; 

    /* BUG-35008 */
    acp_bool_t           mIsSelect;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    ulnLobCache         *mLobCache;
    acp_uint32_t         mPrevRowSetSize; // bug-35198

    /* BUG-42096 Stmt에서 수행된 ParamSetSize의 MAX Size */
    acp_uint32_t         mExecutedParamSetMaxSize;  
    
    /* PROJ-2616 */
    acp_bool_t           mIsSimpleQuery;
    acp_bool_t           mIsSimpleQuerySelectExecuted;
    ulnCacheIPCDA        mCacheIPCDA;

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    acp_uint32_t          mAttrPrefetchAsync;          /* ALTIBASE_PREFETCH_ASYNC */
    acp_bool_t            mAttrPrefetchAutoTuning;     /* ALTIBASE_PREFETCH_AUTO_TUNIING */
    acp_uint32_t          mAttrPrefetchAutoTuningInit; /* ALTIBASE_PREFETCH_AUTO_TUNIING_INIT */
    acp_uint32_t          mAttrPrefetchAutoTuningMin;  /* ALTIBASE_PREFETCH_AUTO_TUNIING_MIN */
    acp_uint32_t          mAttrPrefetchAutoTuningMax;  /* ALTIBASE_PREFETCH_AUTO_TUNIING_MAX */

    ulnSemiAsyncPrefetch *mSemiAsyncPrefetch;

    /* BUG-44858 통신비용을 줄이기 위해 Prepare시 ParamInfoGet 전송을 결정 */
    acp_bool_t           mAttrPrepareWithDescribeParam;

    /* BUG-44957 */
    acp_sint32_t        *mAttrParamsRowCountsPtr;
    acp_sint32_t         mAttrParamsSetRowCounts;

    /* BUG-46011 */
    acp_char_t          *mDeferredQstr;
    acp_sint32_t         mDeferredQstrMaxLen;
    acp_sint32_t         mDeferredQstrLen;
    acp_uint8_t          mDeferredPrepareMode;

    // PROJ-2727
    acp_uint16_t         mAttributeID;
    acp_uint32_t         mAttributeLen;
    acp_char_t          *mAttributeStr;

    /* Sharding Context 관련은 구조체 마지막에 유지. */
    ulsdStmtContext      mShardStmtCxt;
    ulsdModule          *mShardModule;
};

/*
 * Function Declarations
 */

ACI_RC ulnStmtCreate(ulnDbc *aParentDbc, ulnStmt **aOutputStmt);
ACI_RC ulnStmtDestroy(ulnStmt *aStmt);

ACI_RC ulnStmtInitialize(ulnStmt *aStmt);

ulnDesc *ulnStmtGetApd(ulnStmt *aStmt);
ACI_RC   ulnStmtSetApd(ulnStmt *aStmt, ulnDesc *aDesc);
ulnDesc *ulnStmtGetArd(ulnStmt *aStmt);
ACI_RC ulnStmtSetArd(ulnStmt *aStmt, ulnDesc *aDesc);

ulnDesc *ulnStmtGetIrd(ulnStmt *aStmt);
ACI_RC   ulnStmtSetIrd(ulnStmt *aStmt, ulnDesc *aDesc);
ulnDesc *ulnStmtGetIpd(ulnStmt *aStmt);
ACI_RC   ulnStmtSetIpd(ulnStmt *aStmt, ulnDesc *aDesc);

/*
 * =======================================================
 * Getting APD, ARD, IPD, IRD Record from STMT
 * =======================================================
 */
ulnDescRec *ulnStmtGetIrdRec(ulnStmt *aStmt, acp_uint16_t aColumnNumber);

ACP_INLINE ulnDescRec *ulnStmtGetArdRec(ulnStmt *aStmt, acp_uint16_t aColumnNumber)
{
    return ulnDescGetDescRec(aStmt->mAttrArd, aColumnNumber);
}

ACP_INLINE ulnDescRec *ulnStmtGetIpdRec(ulnStmt *aStmt, acp_uint16_t aParamNumber)
{
    return ulnDescGetDescRec(aStmt->mAttrIpd, aParamNumber);
}

ACP_INLINE ulnDescRec *ulnStmtGetApdRec(ulnStmt *aStmt, acp_uint16_t aParamNumber)
{
    return ulnDescGetDescRec(aStmt->mAttrApd, aParamNumber);
}


ACI_RC ulnRecoverOriginalApd(ulnStmt *aStmt);
ACI_RC ulnRecoverOriginalArd(ulnStmt *aStmt);

/*
 * Stmt 의 필드들을 Set/Get 하는 함수들
 */

ACP_INLINE void ulnStmtSetStatementID(ulnStmt *aStmt, acp_uint32_t aStatementID)
{
    aStmt->mStatementID = aStatementID;
}

ACP_INLINE acp_uint32_t ulnStmtGetStatementID(ulnStmt *aStmt)
{
    return aStmt->mStatementID;
}

ACP_INLINE ACI_RC ulnStmtSetStatementType(ulnStmt *aStmt, acp_uint32_t aStatementType)
{
    aStmt->mStatementType = aStatementType;
    return ACI_SUCCESS;
}

ACP_INLINE acp_uint32_t ulnStmtGetStatementType(ulnStmt *aStmt)
{
    return aStmt->mStatementType;
}

ACP_INLINE acp_bool_t ulnStmtIsPrepared(ulnStmt *aStmt)
{
    return aStmt->mIsPrepared;
}

acp_bool_t ulnStmtIsCursorOpen(ulnStmt *aStmt);

#define ulnStmtSetBuildBindInfo(aStmt, aBuild) (aStmt)->mBuildBindInfo = aBuild
#define ulnStmtGetBuildBindInfo(aStmt)         (aStmt)->mBuildBindInfo

// BUG-17592
ACP_INLINE ulnResultType ulnStmtGetResultType(ulnStmt *aStmt)
{
    return aStmt->mResultType;
}

// BUG-38649
ACP_INLINE acp_bool_t ulnStmtHasResultSet(ulnStmt *aStmt)
{
    return ULN_RESULT_TYPE_HAS_RESULTSET(aStmt->mResultType) ? ACP_TRUE : ACP_FALSE;
}

ACP_INLINE acp_bool_t ulnStmtHasRowCount(ulnStmt *aStmt)
{
    return ULN_RESULT_TYPE_HAS_ROWCOUNT(aStmt->mResultType) ? ACP_TRUE : ACP_FALSE;
}

/*
 * =======================================================
 * Param Count / Column Count
 * =======================================================
 */

ACP_INLINE ACI_RC ulnStmtSetParamCount(ulnStmt *aStmt, acp_uint16_t aNewParamCount)
{
    /*
     * PREPARE RESULT 에 있는 param count 이다.
     */
    aStmt->mParamCount = aNewParamCount;
    return ACI_SUCCESS;
}

ACP_INLINE acp_uint16_t ulnStmtGetParamCount(ulnStmt *aStmt)
{
    /*
     * PREPARE RESULT 에 있는 param count 이다.
     */
    return aStmt->mParamCount;
}

acp_uint16_t ulnStmtGetColumnCount(ulnStmt *aStmt);

ACP_INLINE ACI_RC ulnStmtSetResultSetCount(ulnStmt *aStmt, acp_uint16_t aResultSetCount)
{
    aStmt->mResultSetCount = aResultSetCount;

    return ACI_SUCCESS;
}

ACP_INLINE acp_uint16_t ulnStmtGetResultSetCount(ulnStmt *aStmt)
{
    return aStmt->mResultSetCount;
}

ACP_INLINE void ulnStmtSetCurrentResultRowNumber(ulnStmt *aStmt, acp_uint32_t aRowNumber)
{
    aStmt->mCurrentResultRowNumber = aRowNumber;
}

ACP_INLINE ACI_RC ulnStmtSetCurrentResultSetID(ulnStmt *aStmt, acp_uint16_t aResultSetID)
{
    aStmt->mCurrentResultSetID = aResultSetID;

    return ACI_SUCCESS;
}

ACP_INLINE acp_uint16_t ulnStmtGetCurrentResultSetID(ulnStmt *aStmt)
{
    return aStmt->mCurrentResultSetID;
}

void         ulnStmtSetAttrConcurrency(ulnStmt *aStmt, acp_uint32_t aConcurrency);
acp_uint32_t ulnStmtGetAttrConcurrency(ulnStmt *aStmt);

acp_uint32_t ulnStmtGetAttrCursorSensitivity(ulnStmt *aStmt);
ACI_RC       ulnStmtSetAttrCursorSensitivity(ulnStmt *aStmt, acp_uint32_t aSensitivity);

acp_uint32_t ulnStmtGetAttrCursorScrollable(ulnStmt *aStmt);
ACI_RC       ulnStmtSetAttrCursorScrollable(ulnStmt *aStmt, acp_uint32_t aScrollable);

acp_uint32_t ulnStmtGetAttrCursorType(ulnStmt *aStmt);
void         ulnStmtSetAttrCursorType(ulnStmt *aStmt, acp_uint32_t aType);

void         ulnStmtSetAttrUseBookMarks(ulnStmt *aStmt, acp_uint32_t aUseBookMarks);
acp_uint32_t ulnStmtGetAttrUseBookMarks(ulnStmt *aStmt);

ACP_INLINE void ulnStmtSetAttrInputNTS(ulnStmt *aStmt, acp_bool_t aNTS)
{
    aStmt->mAttrInputNTS = aNTS;
}

ACP_INLINE acp_bool_t ulnStmtGetAttrInputNTS(ulnStmt *aStmt)
{
    return aStmt->mAttrInputNTS;
}

// PROJ-1518
ACP_INLINE void ulnStmtSetAtomicArray(ulnStmt *aStmt, acp_uint32_t aAtomicArray)
{
    aStmt->mAtomicArray = aAtomicArray;
}

ACP_INLINE acp_uint32_t ulnStmtGetAtomicArray(ulnStmt *aStmt)
{
    return aStmt->mAtomicArray;
}

/*
 * ============================================
 * Prefetch 속성
 * ============================================
 */

ACP_INLINE void ulnStmtSetAttrPrefetchRows(ulnStmt *aStmt, acp_uint32_t aNumberOfRows)
{
    /*
     * BUG-37642 Improve performance to fetch.
     */
    if (aNumberOfRows > ACP_SINT32_MAX)
    {
        aStmt->mAttrPrefetchRows = ACP_SINT32_MAX;
    }
    else
    {
        aStmt->mAttrPrefetchRows = aNumberOfRows;
    }
}

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchRows(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchRows;
}

ACP_INLINE void ulnStmtSetAttrPrefetchBlocks(ulnStmt *aStmt, acp_uint32_t aMultiplier)
{
    aStmt->mAttrPrefetchBlocks = aMultiplier;
}

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchBlocks(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchBlocks;
}

ACP_INLINE void ulnStmtSetAttrPrefetchMemory(ulnStmt *aStmt, acp_uint32_t aMemorySize)
{
    aStmt->mAttrPrefetchMemory = aMemorySize;
}

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchMemory(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchMemory;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC ulnStmtSetAttrPrefetchAsync(ulnFnContext *aFnContext,
                                   acp_uint32_t  aAsyncMethod);

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchAsync(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchAsync;
}

void ulnStmtSetAttrPrefetchAutoTuning(ulnFnContext *aFnContext,
                                      acp_bool_t    aIsPrefetchAutoTuning);

ACP_INLINE acp_bool_t ulnStmtGetAttrPrefetchAutoTuning(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchAutoTuning;
}

ACP_INLINE void ulnStmtSetAttrPrefetchAutoTuningInit(ulnStmt      *aStmt,
                                                     acp_uint32_t  aPrefetchAutoTuningInit)
{
    aStmt->mAttrPrefetchAutoTuningInit = aPrefetchAutoTuningInit;
}

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchAutoTuningInit(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchAutoTuningInit;
}

void ulnStmtSetAttrPrefetchAutoTuningMin(ulnFnContext *aFnContext,
                                         acp_uint32_t  aPrefetchAutoTuningMin);

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchAutoTuningMin(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchAutoTuningMin;
}

void ulnStmtSetAttrPrefetchAutoTuningMax(ulnFnContext *aFnContext,
                                         acp_uint32_t  aPrefetchAutoTuningMax);

ACP_INLINE acp_uint32_t ulnStmtGetAttrPrefetchAutoTuningMax(ulnStmt *aStmt)
{
    return aStmt->mAttrPrefetchAutoTuningMax;
}

/* BUG-42096 A user's app with CLI dies when Array insert is executing */
#define ulnStmtSetExecutedParamSetMaxSize(aStmt, aSize) (aStmt)->mExecutedParamSetMaxSize = aSize
#define ulnStmtGetExecutedParamSetMaxSize(aStmt)        (aStmt)->mExecutedParamSetMaxSize

/*
 * Parameter 속성들 중 Descriptor 속성으로 매핑되는 함수들
 */
acp_uint32_t ulnStmtGetAttrParamBindType(ulnStmt *aStmt);
void         ulnStmtSetAttrParamBindType(ulnStmt *aStmt, acp_uint32_t aType);

acp_uint32_t ulnStmtGetAttrParamsetSize(ulnStmt *aStmt);
void         ulnStmtSetAttrParamsetSize(ulnStmt *aStmt, acp_uint32_t aSize);

// fix BUG-20745 BIND_OFFSET_PTR 지원
void     ulnStmtSetAttrParamBindOffsetPtr(ulnStmt *aStmt, ulvULen *aOffsetPtr);
ulvULen *ulnStmtGetAttrParamBindOffsetPtr(ulnStmt *aStmt);
ulvULen  ulnStmtGetParamBindOffsetValue(ulnStmt *aStmt);

void     ulnStmtSetAttrParamsProcessedPtr(ulnStmt *aStmt, ulvULen *aProcessedPtr);
ulvULen *ulnStmtGetAttrParamsProcessedPtr(ulnStmt *aStmt);
void     ulnStmtSetParamsProcessedValue(ulnStmt *aStmt, ulvULen aValue);
ACI_RC   ulnStmtIncreaseParamsProcessedValue(ulnStmt *aStmt);

void          ulnStmtSetAttrParamOperationPtr(ulnStmt *aStmt, acp_uint16_t *aOperationPtr);
acp_uint16_t *ulnStmtGetAttrParamOperationPtr(ulnStmt *aStmt);
acp_uint16_t  ulnStmtGetAttrParamOperationValue(ulnStmt *aStmt, acp_uint32_t aRow);

acp_uint16_t *ulnStmtGetAttrParamStatusPtr(ulnStmt *aStmt);
void          ulnStmtSetAttrParamStatusPtr(ulnStmt *aStmt, acp_uint16_t *aStatusPtr);
void          ulnStmtSetAttrParamStatusValue(ulnStmt *aStmt, acp_uint32_t aRow, acp_uint16_t aValue);

/*
 * Row 속성들 중 Descriptor 속성으로 매핑되는 함수들
 */

acp_uint32_t ulnStmtGetAttrRowBindType(ulnStmt *aStmt);
void         ulnStmtSetAttrRowBindType(ulnStmt *aStmt, acp_uint32_t aType);

/*
 * -------------------------------------
 *  SQL_ATTR_ROW_ARRAY_SIZE
 *  SQL_ROWSET_SIZE
 * -------------------------------------
 */
ACP_INLINE void ulnStmtSetRowSetSize(ulnStmt *aStmt, acp_uint32_t aRowSetSize)
{
    aStmt->mAttrRowsetSize = aRowSetSize;
}

ACP_INLINE acp_uint32_t ulnStmtGetRowSetSize(ulnStmt *aStmt)
{
    return aStmt->mAttrRowsetSize;
}

ACP_INLINE ACI_RC ulnStmtSetAttrRowArraySize(ulnStmt *aStmt, acp_uint32_t aSize)
{
    /*
     * 기본적으로, SQLFetch() 나 SQLFetchScroll() 함수에 의해서 리턴될
     * row 의 갯수를 가지고 있는 stmt attribute 이다.
     *
     * 디폴트 값은 1.
     *
     * SQL_ATTR_ROW_ARRAY_SIZE.
     *
     * ARD 의 SQL_DESC_ARRAY_SIZE 로 매핑
     */

    ACI_TEST(aSize == 0);

    ulnDescSetArraySize(aStmt->mAttrArd, aSize);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACP_INLINE acp_uint32_t ulnStmtGetAttrRowArraySize(ulnStmt *aStmt)
{
    /*
     * Note : 64비트 odbc 에 따르면 필드가 64비트를 지원해야 하는데,
     *        어떤 -_-;; 사람이 20억개 이상의 array 를 써서 바인딩 하겠누 -_-;;
     *
     *        그냥 내부에서는 acp_uint32_t 로 하고, SQLSet/GetStmtAttr 함수들에서 캐스팅만 하자.
     */
    if (aStmt->mObj.mExecFuncID == ULN_FID_EXTENDEDFETCH)
    {
        return ulnStmtGetRowSetSize(aStmt);
    }
    else
    {
        return ulnDescGetArraySize(aStmt->mAttrArd);
    }
}

// fix BUG-20745 BIND_OFFSET_PTR 지원
void     ulnStmtSetAttrRowBindOffsetPtr(ulnStmt *aStmt, ulvULen *aOffsetPtr);
ulvULen *ulnStmtGetAttrRowBindOffsetPtr(ulnStmt *aStmt);
ulvULen  ulnStmtGetRowBindOffsetValue(ulnStmt *aStmt);

void     ulnStmtSetAttrRowsFetchedPtr(ulnStmt *aStmt, ulvULen *aFetchedPtr);
ulvULen *ulnStmtGetAttrRowsFetchedPtr(ulnStmt *aStmt);
void     ulnStmtSetRowsFetchedValue(ulnStmt *aStmt, ulvULen aValue);

void          ulnStmtSetAttrRowOperationPtr(ulnStmt *aStmt, acp_uint16_t *aOperationPtr);
acp_uint16_t *ulnStmtGetAttrRowOperationPtr(ulnStmt *aStmt);

acp_uint16_t *ulnStmtGetAttrRowStatusPtr(ulnStmt *aStmt);
void          ulnStmtSetAttrRowStatusPtr(ulnStmt *aStmt, acp_uint16_t *aStatusPtr);
acp_uint16_t  ulnStmtGetAttrRowStatusValue(ulnStmt *aStmt, acp_uint32_t aRow);
void          ulnStmtSetAttrRowStatusValue(ulnStmt *aStmt, acp_uint32_t aRow, acp_uint16_t aValue);

/*
 * Fetched Row Count 를 읽고 쓰는 함수들
 */
ACP_INLINE void ulnStmtSetFetchedRowCountFromServer(ulnStmt *aStmt, acp_uint32_t aNewCount)
{
    aStmt->mFetchedRowCountFromServer = aNewCount;
}

ACP_INLINE acp_uint32_t ulnStmtGetFetchedRowCountFromServer(ulnStmt *aStmt)
{
    return aStmt->mFetchedRowCountFromServer;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACP_INLINE void ulnStmtSetFetchedBytesFromServer(ulnStmt *aStmt, acp_uint64_t aNewBytes)
{
    aStmt->mFetchedBytesFromServer = aNewBytes;
}

ACP_INLINE acp_uint64_t ulnStmtGetFetchedBytesFromServer(ulnStmt *aStmt)
{
    return aStmt->mFetchedBytesFromServer;
}

void ulnStmtInitRowStatusArrayValue(ulnStmt      *aStmt,
                                    acp_uint32_t  aStartIndex,
                                    acp_uint32_t  aArraySize,
                                    acp_uint16_t  aValue);

/*
 * 그 외 함수들
 */

ACI_RC ulnStmtSetAttrQueryTimeout(ulnStmt *aStmt, acp_uint32_t aTimeout);
ACI_RC ulnStmtGetAttrQueryTimeout(ulnStmt *aStmt, acp_uint32_t *aTimeout);

ACI_RC ulnStmtGetAttrRetrieveData(ulnStmt *aStmt, acp_uint32_t *aRetrieve);
ACI_RC ulnStmtSetAttrRetrieveData(ulnStmt *aStmt, acp_uint32_t aRetrieve);

void   ulnStmtClearBindInfoSentFlagAll(ulnStmt *aStmt);

ACI_RC   ulnStmtCreateCache(ulnStmt *aStmt);

ACP_INLINE ulnCursor *ulnStmtGetCursor(ulnStmt *aStmt)
{
    return &aStmt->mCursor;
}

ACP_INLINE ulnCache *ulnStmtGetCache(ulnStmt *aStmt)
{
    return aStmt->mCache;
}

void       ulnStmtSetCache(ulnStmt *aStmt, ulnCache *aCache);

ACI_RC ulnStmtFreePlanTree(ulnStmt *aStmt);
ACI_RC ulnStmtAllocPlanTree(ulnStmt *aStmt, acp_uint32_t aSize);

acp_uint32_t ulnStmtGetAttrRowBlockSizeThreshold(ulnStmt *aStmt);

/*
 * result 관련 함수들
 */

ulnResult *ulnResultCreate(ulnStmt *aParentStmt);
ulnResult *ulnStmtGetNextResult(ulnStmt *aStmt);

ACP_INLINE void ulnResultSetType(ulnResult *aResult, ulnResultType aType)
{
    if (aResult != NULL)
    {
        aResult->mType = aType;
    }
}

ACP_INLINE ulnResultType ulnResultGetType(ulnResult *aResult)
{
    ulnResultType sResultType;

    if (aResult == NULL)
    {
        sResultType = ULN_RESULT_TYPE_UNKNOWN;
    }
    else
    {
        sResultType = aResult->mType;
    }

    return sResultType;
}

ACP_INLINE acp_sint64_t ulnResultGetAffectedRowCount(ulnResult *aResult)
{
    return (aResult == NULL) ? ULN_DEFAULT_ROWCOUNT : aResult->mAffectedRowCount;
}

ACP_INLINE acp_sint64_t ulnResultGetFetchedRowCount(ulnResult *aResult)
{
    return (aResult == NULL) ? ULN_DEFAULT_ROWCOUNT : aResult->mFetchedRowCount;
}

ACP_INLINE ulnResult *ulnStmtGetNewResult(ulnStmt *aStmt)
{
    ulnResult *sResult = NULL;

    if (acpListIsEmpty(&aStmt->mResultFreeList))
    {
        sResult = ulnResultCreate(aStmt);
    }
    else
    {
        sResult = (ulnResult *)(aStmt->mResultFreeList.mNext);
        /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
        if ( sResult != NULL )
        {

            acpListDeleteNode((acp_list_t *)sResult);

            sResult->mType              = ULN_RESULT_TYPE_UNKNOWN;
            sResult->mAffectedRowCount  = ULN_DEFAULT_ROWCOUNT;
            sResult->mFromRowNumber     = 0;
            sResult->mToRowNumber       = 0;
        }
        else
        {
            /* do nothing */
        }
    }

    return sResult;
}

ACP_INLINE void ulnStmtAddResult(ulnStmt *aStmt, ulnResult *aResult)
{
    acpListAppendNode(&aStmt->mResultList, (acp_list_t *)aResult);
}

ACP_INLINE ulnResult *ulnStmtGetCurrentResult(ulnStmt *aStmt)
{
    return aStmt->mCurrentResult;
}

ACP_INLINE void ulnStmtSetCurrentResult(ulnStmt *aStmt, ulnResult *aResult)
{
    aStmt->mCurrentResult = aResult;
}

ACP_INLINE void ulnStmtFreeAllResult(ulnStmt *aStmt)
{
    acpListAppendList(&aStmt->mResultFreeList, &aStmt->mResultList);

    aStmt->mResultSetCount         = 0;
    aStmt->mCurrentResult          = NULL;
    aStmt->mCurrentResultRowNumber = 0;   /* 0-BASE */
    aStmt->mCurrentResultSetID     = 0;
}

acp_bool_t ulnStmtIsLastResult(ulnStmt *aStmt);

/*
 * Chunk Handling Routines ( PROJ-1697 )
 */

ACP_INLINE void ulnStmtChunkAppendParamData( ulnStmt       *aStmt,
                                  acp_uint8_t   *aData,
                                  acp_uint32_t   aSize )
{
    acpMemCpy(aStmt->mChunk.mData + aStmt->mChunk.mCursor,
              aData,
              aSize);

    aStmt->mChunk.mCursor += aSize;
}

ACP_INLINE acp_uint8_t *ulnStmtChunkGetData( ulnStmt  *aStmt )
{
    return aStmt->mChunk.mData;
}

ACP_INLINE acp_uint32_t ulnStmtChunkGetSize( ulnStmt  *aStmt )
{
    return aStmt->mChunk.mCursor;
}

ACP_INLINE acp_uint32_t ulnStmtChunkGetCursor( ulnStmt *aStmt )
{
    return aStmt->mChunk.mCursor;
}

ACP_INLINE void ulnStmtChunkReset( ulnStmt *aStmt )
{
    aStmt->mChunk.mCursor   = 0;
    aStmt->mChunk.mRowCount = 0;
}

ACP_INLINE void ulnStmtChunkIncRowCount( ulnStmt *aStmt )
{
    aStmt->mChunk.mRowCount++;
}

ACP_INLINE acp_uint32_t ulnStmtChunkGetRowCount( ulnStmt *aStmt )
{
    return aStmt->mChunk.mRowCount;
}

ACP_INLINE void ulnStmtChunkSetSavePoint( ulnStmt               *aStmt,
                                             ulnStmtChunkSavePoint *aSavePoint )
{
    aSavePoint->mCursor   = aStmt->mChunk.mCursor;
    aSavePoint->mRowCount = aStmt->mChunk.mRowCount;
}

ACP_INLINE void ulnStmtChunkRollbackToSavePoint( ulnStmt               *aStmt,
                                                    ulnStmtChunkSavePoint *aSavePoint )
{
    aStmt->mChunk.mCursor   = aSavePoint->mCursor;
    aStmt->mChunk.mRowCount = aSavePoint->mRowCount;
}

ACI_RC       ulnStmtAllocChunk( ulnStmt *aStmt,
                                acp_uint32_t     aSize,
                                acp_uint8_t  **aData );

//PROJ-1645 UL-FailOver.
void ulnStmtSetPrepared(ulnStmt  *aStmt,
                        acp_bool_t    aIsPrepared);

/* PROJ-2177 User Interface - Cancel */

#define ulnStmtGetCID(aStmtPtr)                 ((aStmtPtr)->mStmtCID)

#define ulnStmtSetCID(aStmtPtr, aStmtCID) do\
{\
    (aStmtPtr)->mStmtCID = (aStmtCID);\
} while(0)

#define ulnStmtResetCID(aStmtPtr)               ulnStmtSetCID((aStmtPtr), 0)

#define ulnStmtGetNeedDataFuncID(aStmtPtr)      ((aStmtPtr)->mNeedDataFuncID)

#define ulnStmtSetNeedDataFuncID(aStmtPtr, aFuncID) do\
{\
    (aStmtPtr)->mNeedDataFuncID = (aFuncID);\
} while(0)

#define ulnStmtResetNeedDataFuncID(aStmtPtr)    ulnStmtSetNeedDataFuncID((aStmtPtr), ULN_FID_NONE)

#define ulnStmtGetLastFetchFuncID(aStmtPtr)     ((aStmtPtr)->mLastFetchFuncID)

#define ulnStmtSetLastFetchFuncID(aStmtPtr, aFuncID) do\
{\
    (aStmtPtr)->mLastFetchFuncID = (aFuncID);\
} while(0)

#define ulnStmtResetLastFetchFuncID(aStmtPtr)   ulnStmtSetLastFetchFuncID((aStmtPtr), ULN_FID_NONE)

/* PROJ-1381 Fetch Across Commit */

void         ulnStmtSetAttrCursorHold(ulnStmt *aStmt, acp_uint32_t aCursorHold);
acp_uint32_t ulnStmtGetAttrCursorHold(ulnStmt *aStmt);

acp_uint8_t  ulnStmtGetPrepareMode(ulnStmt *aStmt, acp_uint8_t aExecMode);

/* PROJ-1789 Updatable Scrollable Cursor */

#define ulnStmtGetCursorPosition(aStmtPtr) \
    ulnCursorGetPosition(ulnStmtGetCursor(aStmtPtr))

ACI_RC           ulnStmtCreateKeyset(ulnStmt *aStmt);

acp_sint64_t     ulnStmtGetAttrFetchBookmarkVal(ulnStmt *aStmt);
acp_uint8_t*     ulnStmtGetAttrFetchBookmarkPtr(ulnStmt *aStmt);
void             ulnStmtSetAttrFetchBookmarkPtr(ulnStmt     *aStmt,
                                                acp_uint8_t *aFetchBookmarkPtr);

/**
 * Keyset 객체를 얻는다.
 *
 * @param[in] aStmt statement handle
 *
 * @return Keyset 객체
 */
ACP_INLINE ulnKeyset* ulnStmtGetKeyset(ulnStmt *aStmt)
{
    return aStmt->mKeyset;
}

ulnStmtFetchMode ulnStmtGetFetchMode(ulnStmt *aStmt);
void             ulnStmtSetFetchMode(ulnStmt          *aStmt,
                                     ulnStmtFetchMode  aFetchMode);

ACI_RC           ulnStmtEnsureAllocColNames(ulnStmt      *aStmt,
                                            acp_sint32_t  aColumnCount);
ACI_RC           ulnStmtEnsureAllocPrepareTextBuf(ulnStmt      *aStmt,
                                                  acp_sint32_t  aBufLen);
ACI_RC           ulnStmtEnsureReallocQstrForRowset(ulnStmt      *aStmt,
                                                   acp_sint32_t  aBufLen);
ACI_RC           ulnStmtEnsureAllocQstrForUpdate(ulnStmt      *aStmt,
                                                 acp_sint32_t  aBufLen);
ACI_RC           ulnStmtEnsureAllocQstrForDelete(ulnStmt      *aStmt,
                                                 acp_sint32_t  aBufLen);
ACI_RC           ulnStmtEnsureAllocRowsetParamBuf(ulnStmt      *aStmt,
                                                  acp_sint32_t  aBufLen);
ACI_RC           ulnStmtEnsureAllocRowsetParamStatusBuf(ulnStmt      *aStmt,
                                                        acp_sint32_t  aBufLen);
ACI_RC           ulnStmtEnsureAllocColumnIgnoreFlagsBuf(ulnStmt      *aStmt,
                                                        acp_sint32_t  aBufLen);

#define ulnStmtResetQstrForDelete(aStmt) \
    ( (aStmt)->mQstrForDeleteLen = 0 )

#define ulnStmtIsQstrForDeleteBuilt(aStmt) \
    ( ((aStmt)->mQstrForDeleteLen == 0) ? ACP_FALSE : ACP_TRUE )

#define ulnStmtGetRowsetParamBuf(aStmtPtr) \
    ( (aStmtPtr)->mRowsetParamBuf )

#define ulnStmtGetRowsetParamStatusBuf(aStmtPtr) \
    ( (aStmtPtr)->mRowsetParamStatusBuf )

#define ulnStmtGetColumnIgnoreFlagsBuf(aStmtPtr) \
    ( (aStmtPtr)->mColumnIgnoreFlagsBuf )

ACI_RC ulnStmtBuildTableNameForUpdate(ulnStmt *aStmt);

#define ulnStmtIsTableNameForUpdateBuilt(aStmt) \
    (((aStmt)->mTableNameForUpdateLen > 0) ? ACP_TRUE : ACP_FALSE)

#define ulnStmtGetTableNameForUpdate(aStmt) \
    ((aStmt)->mTableNameForUpdate)

#define ulnStmtGetTableNameForUpdateLen(aStmt) \
    ((aStmt)->mTableNameForUpdateLen)

#define ulnStmtResetTableNameForUpdate(aStmt) do\
{\
    (aStmt)->mTableNameForUpdate[0] = '\0';\
    (aStmt)->mTableNameForUpdateLen = 0;\
} while (0)

#define ulnStmtGetAttrDeferredPrepare(aStmt) \
    ((aStmt)->mAttrDeferredPrepare)

#define ulnStmtSetAttrDeferredPrepare(aStmt, aValue) \
{ \
    (aStmt)->mAttrDeferredPrepare = aValue; \
} while (0)


/* BUG-44858 */
#define ulnStmtGetAttrPrepareWithDescribeParam(aStmt) \
    ((aStmt)->mAttrPrepareWithDescribeParam)

#define ulnStmtSetAttrPrepareWithDescribeParam(aStmt, aValue) \
{ \
    (aStmt)->mAttrPrepareWithDescribeParam = aValue; \
} while (0)


ulnStmt* ulnStmtGetSubStmtBySELECT( ulnStmt *aBaseStmt, ulnStmt *aSubStmt );
ACI_RC   ulnStmtMakeSubStmt( ulnStmt *aBaseStmt, ulnStmt **aNewSubStmt );
ACI_RC   ulnStmtCleanUpSubStmt( ulnStmt *aBaseStmt, ulnStmt *aSubStmt );
void     ulnStmtAdjustParamNumber( ulnStmt **aStmt, acp_uint16_t *aParamNumber );

ACP_INLINE
void ulnStmtResetPD( ulnStmt *aStmt )
{
    aStmt->mProcessingRowNumber    = 0;
    aStmt->mProcessingLobRowNumber = 0;
    aStmt->mProcessingParamNumber  = 1;
    aStmt->mHasDataAtExecParam     = 0;
}

/* BUG-44957 */

#define ulnStmtGetAttrParamsRowCountsPtr(aStmtPtr) \
    ((aStmtPtr)->mAttrParamsRowCountsPtr)

#define ulnStmtSetAttrParamsRowCountsPtr(aStmtPtr, aValuePtr) do\
{\
    (aStmtPtr)->mAttrParamsRowCountsPtr = aValuePtr;\
} while (0)

#define ulnStmtInitAttrParamsRowCounts(aStmtPtr) do\
{\
    if (ulnStmtGetAttrParamsRowCountsPtr(aStmtPtr) != NULL)\
    {\
        *(aStmtPtr)->mAttrParamsRowCountsPtr = SQL_SUCCESS;\
    }\
} while (0)

#define ulnStmtUpdateAttrParamsRowCountsValue(aStmtPtr, aValue) do\
{\
    if (ulnStmtGetAttrParamsRowCountsPtr(aStmtPtr) != NULL)\
    {\
        if ( (aValue == SQL_ERROR) ||\
             (*(aStmtPtr)->mAttrParamsRowCountsPtr != SQL_ERROR) )\
        {\
            *(aStmtPtr)->mAttrParamsRowCountsPtr = aValue;\
        }\
    }\
} while (0)

#define ulnStmtGetAttrParamsSetRowCounts(aStmtPtr) \
    ((aStmtPtr)->mAttrParamsSetRowCounts)

#define ulnStmtSetAttrParamsSetRowCounts(aStmtPtr, aValue) do\
{\
    (aStmtPtr)->mAttrParamsSetRowCounts = aValue;\
} while (0)

/* BUG-46011 */

ACP_INLINE acp_bool_t ulnStmtIsSetDeferredQstr(ulnStmt *aStmt)
{
    return (aStmt->mDeferredQstrLen > 0) ? ACP_TRUE : ACP_FALSE;
}

ACP_INLINE void ulnStmtClearDeferredQstr(ulnStmt *aStmt)
{
    aStmt->mDeferredQstrLen = 0;

    if (aStmt->mDeferredQstrMaxLen > 0)
    {
        aStmt->mDeferredQstr[0] = '\0';
    }
}

ACI_RC ulnStmtEnsureAllocDeferredQstr(ulnStmt *aStmt, acp_sint32_t aBufLen);

#endif /* _O_ULN_STMT_H_ */
