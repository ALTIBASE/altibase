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

#ifndef _O_ULN_CAHCE_H_
#define _O_ULN_CAHCE_H_ 1

#include <uluArray.h>
#include <ulnPrivate.h>
#include <ulnGetData.h>

#define ULN_CACHE_DEFAULT_PREFETCH_ROWS         1
#define ULN_CACHE_OPTIMAL_PREFETCH_ROWS         0

#define ULN_ROWCOUNT_UNKNOWN                    (ACP_SINT64_MAX)
#define ULN_CACHE_ROW_BLOCK_SIZE_THRESHOLD      (64 * 1024)

#define ULN_CACHE_RPAB_UNIT_COUNT               (256)
#define ULN_CACHE_MAX_ROW_IN_RPA                (256)
#define ULN_CACHE_MAX_SIZE_FOR_FIXED_TYPE       (1024)

typedef struct ulnRow        ulnRow;
typedef struct ulnColumnInfo ulnColumnInfo;
typedef struct ulnColumn     ulnColumn;

struct ulnRow
{
    acp_sint64_t  mRowNumber;                    /**< Position. ResultSet에서의 순번 */
    acp_uint8_t  *mRow;                          /**< Raw 데이타를 담은 버퍼 포인터 */
};

struct ulnColumn
{
    acp_uint16_t  mColumnNumber;
    acp_uint16_t  mMtype;                        /* 데이터 타입. ulnMTypeID 인데, enum 이라서 사이즈 결정하기가
                                                    거시기하다. 그래서 그냥 acp_uint16_t 로 잡음 */
    ulvSLen       mDataLength;                   /* mBuffer 에 들어있는 데이터의 길이. mBuffer 의 크기가 아님! */
    ulvSLen       mMTLength;                     /* MT data size */
    acp_uint32_t  mPrecision;
    acp_uint8_t  *mBuffer;

    acp_uint32_t  mGDState;                      /* GetData 시 컬럼의 상태
                                                    ULN_GD_ST_INITIAL    1
                                                    ULN_GD_ST_RETRIEVING 2 */

    ulvULen       mGDPosition;                   /* 다음 GetData() 호출시 읽게 될 문자 위치 */

    ulvULen       mRemainTextLen;                /* 사용자 버퍼에 다 못주고 남은 문자열 길이 */
    acp_uint8_t   mRemainText[ULN_MAX_CHARSIZE]; /* 사용자 버퍼에 다 못주고 남은 문자열 */
};

struct ulnColumnInfo
{
    ulnColumn    mColumn;
    acp_uint32_t mOffset;
    ulnMTypeID   mMTYPE;
};

/*
 * RowPointerArrayBlock(RPAB)
 * +---+---+---+---+-------+---+
 * | 0 | 1 | 2 | 3 | . . . | n | n = ulnRPAB::mCount
 * +-|-+---+---+---+-------+---+
 *   |
 *   |
 *   |
 *   |
 *   +---> +------------------+     +-------------------------------------------------------+
 *         | ulnRow 1         | --> | column data 1 | column data 2 |  ...  | column data p |
 *         +------------------+     +-------------------------------------------------------+
 *         | ulnRow 2         | --> | column data 1 | column data 2 |  ...  | column data p |
 *         +------------------+     +-------------------------------------------------------+
 *         | ulnRow 3         | --> | column data 1 | column data 2 |  ...  | column data p |
 *         +------------------+     +-------------------------------------------------------+
 *         | . . . .          |     | . . . . . .                                           |
 *         +------------------+     +-------------------------------------------------------+
 *         | ulnRow m         |     RowBlock
 *         +------------------+
 *         RowPointerArray(RPA)
 *         m = ULN_CACHE_MAX_ROW_IN_RPA
 */

typedef struct ulnRPAB
{
    acp_uint32_t   mCount;       /* mArray 에 현재 할당되어 있는 element 의 갯수 */
    acp_uint32_t   mUsedCount;   /* 사용하고 있는 element 의 갯수 */
    ulnRow       **mBlock;       /* RPA 의 배열 */
} ulnRPAB;

struct ulnCache
{
    ulnStmt                 *mParentStmt;
    acp_sint64_t             mResultSetSize;   /* 서버에서 생성된 result set 의 row 의 전체 갯수 */

    acp_sint64_t             mRowStartPosition; /**< Cache에 쌓인 첫 Row의 Position */
    acp_uint32_t             mRowCount;        /* 서버로부터 수신해서 캐쉬가 이루어진 row 의 갯수 */

    acp_uint32_t             mSingleRowSize;

    ulnRPAB                  mRPAB;            /* ROW POINTER ARRAY BLOCK */

    acp_uint16_t             mColumnInfoArraySize;
    acp_uint8_t             *mColumnInfoArray;
    acp_uint8_t             *mColumnBuffer;    /* FIXED COLUMN DATA를 저장할 공간 */
    ulnDiagRec              *mServerError;

    // PROJ-1752
    acl_mem_area_t          *mChunk;           /* COLLECTION DATA가 CACHING될 메모리 공간 */
    acl_mem_area_snapshot_t  mChunkStatus;
    acp_bool_t               mHasLob;

    /* BUG-32474
     * BUGBUG: HashSet 없어서 HashTable 사용. 나중에 core에 추가되면 바꿀것. */
    acl_hash_table_t         mReadLobLocatorHash;

    acp_bool_t               mIsInvalidated;
};

/* PROJ-2616 */
/* SimpleQuery-Fetch시에 사용되는 버퍼에 대한 정보.*/
typedef struct ulnCacheIPCDA
{
    acp_uint32_t  mRemainingLength;
    acp_uint32_t  mReadLength;
    acp_bool_t    mIsParamDataInList;
    acp_bool_t    mIsAllocFetchBuffer; /* mFetchBuffer의 메모리 생성 유무 */
    acp_uint8_t  *mFetchBuffer;        /* 다수의 Statement 생성시 Fetch Data를 저장하는 Buffer
                                        * statement가 1개일때는 shared memory 주소를 가지고 있으며,
                                        * 다수의 statement 생성시 memory가 생성되어 shared memory에 저장되어 있던 데이터를 가져온다.*/
} ulnCacheIPCDA;

/*
 * 캐쉬에 있는 하나의 row 를 사용자에게 건네 주면서 각각의 컬럼들을 컨버젼을 하게 된다.
 * 이때, 에러가 발생하거나, 경고가 발생하면 컨버젼 함수는 이를 리턴한다.
 * 하나의 row 가 끝나면, 여태 컨버젼 함수가 건네준 에러 / 경고 코드들을 ORing 해 둔 변수를
 * 읽어서 플래그를 취한 후 row status 를 무엇으로 할지 결정한다.
 */
#define ULN_ROW_SUCCESS             0x00
#define ULN_ROW_SUCCESS_WITH_INFO   0x01
#define ULN_ROW_ERROR               0x02
#define ULN_ROW_COMPOUND            0x03

/*
 * 생성과 소멸, 그리고 초기화
 */
ACI_RC ulnCacheCreate(ulnCache **aCache);
void   ulnCacheDestroy(ulnCache *aCache);

/* BUG-38818 Prevent to fail a assertion in ulnCacheInitialize() */
ACI_RC ulnCacheEmptyHashTable(acl_hash_table_t *aHashTable);

ACP_INLINE ACI_RC ulnCacheInitialize(ulnCache *aCache)
{
    aCache->mResultSetSize    = ULN_ROWCOUNT_UNKNOWN;
    aCache->mRowCount         = 0;
    aCache->mRowStartPosition = 1;
    aCache->mSingleRowSize    = 0;
    aCache->mHasLob           = ACP_FALSE;
    aCache->mRPAB.mUsedCount  = 0;

    /* BUG-38818 Prevent to fail a assertion in ulnCacheInitialize() */
    if (aclHashGetTotalRecordCount(&aCache->mReadLobLocatorHash) > 0)
    {
        ulnCacheEmptyHashTable(&aCache->mReadLobLocatorHash);
    }
    else
    {
        /* Nothing */
    }

    /* BUG-32474 (방어코드)
     * 중간에 ulnCacheInitialize()가 호출 될 때는 hash가 비어있어야 한다. */
    ACE_ASSERT(aclHashGetTotalRecordCount(&aCache->mReadLobLocatorHash) == 0);

    // (void)ulnCacheBackChunkToMark( aCache );
    aclMemAreaFreeToSnapshot(aCache->mChunk, &aCache->mChunkStatus);

    /* BUG-48269 */
    aCache->mServerError  = NULL;

    return ACI_SUCCESS;
}

ACI_RC ulnCachePrepareColumnInfoArray(ulnCache *aCache, acp_uint16_t aColumnCount);

/*
 * cached row 와 관련된 연산들
 */
ulnRow *ulnCacheGetRow(ulnCache *aCache, acp_uint32_t aPhysicalPos);
ulnRow *ulnCacheGetCachedRow(ulnCache *aCache, acp_sint64_t aLogicalCurPos);

ACP_INLINE void ulnCacheSetSingleRowSize(ulnCache *aCache, acp_uint32_t aSizeOfSingleRow)
{
    aCache->mSingleRowSize = aSizeOfSingleRow;
}

ACP_INLINE acp_uint32_t ulnCacheGetSingleRowSize(ulnCache *aCache)
{
    return aCache->mSingleRowSize;
}

acp_sint32_t ulnCacheGetRowCount(ulnCache *aCache);
void         ulnCacheInitRowCount(ulnCache *aCache);

// To Fix BUG-20409
void         ulnCacheAdjustStartPosition(ulnCache * aCache);

ACP_INLINE acp_sint64_t ulnCacheGetTotalRowCnt( ulnCache * aCache )
{
    return aCache->mRowStartPosition + aCache->mRowCount - 1;
}

acp_uint32_t ulnCacheCalcBlockSizeOfOneFetch(ulnCache *aCache, ulnCursor *aCursor);
acp_uint32_t ulnCacheCalcPrefetchRowSize(ulnCache *aCache, ulnCursor *aCursor);
acp_sint32_t ulnCacheCalcNumberOfRowsToGet(ulnCache     *aCache,
                                           ulnCursor    *aCursor,
                                           acp_uint32_t  aBlockSizeOfOneFetch);

ACI_RC     ulnCacheCloseLobInCurrentContents(ulnFnContext *aFnContext,
                                             ulnPtContext *aPtContext,
                                             ulnCache     *aCache);

ACP_INLINE acp_bool_t ulnCacheHasLob(ulnCache *aCache)
{
    acp_bool_t sHasLob = ACP_FALSE;

    /* Cache가 아직 만들어지지 않았다면 core가 날 수 있다. */
    if ((aCache != NULL)
     && (aCache->mHasLob == ACP_TRUE)
     && (aCache->mRowCount > 0))
    {
        sHasLob = ACP_TRUE;
    }

    return sHasLob;
}

/*
 * Column Info 를 만드는 함수
 */
ACP_INLINE ulnColumnInfo *ulnCacheGetColumnInfo(ulnCache *aCache, acp_uint16_t aColumnNumber)
{
    return (ulnColumnInfo *)(aCache->mColumnInfoArray +
                             (ACP_ALIGN8(ACI_SIZEOF(ulnColumnInfo)) * aColumnNumber));
}

ACP_INLINE void ulnCacheSetColumnInfo(ulnCache     *aCache,
                                         acp_uint16_t  aColumnNumber,
                                         ulnMTypeID    aType,
                                         acp_uint32_t  aPosition)
{
    ulnColumnInfo *sColumnInfo = ulnCacheGetColumnInfo(aCache,
                                                       aColumnNumber);

    sColumnInfo->mOffset = aPosition;
    sColumnInfo->mMTYPE  = aType;
    sColumnInfo->mColumn.mColumnNumber  = aColumnNumber;
    sColumnInfo->mColumn.mMtype         = aType;
    sColumnInfo->mColumn.mGDState       = ULN_GD_ST_INITIAL;
    sColumnInfo->mColumn.mGDPosition    = 0;
    sColumnInfo->mColumn.mRemainTextLen = 0;
    sColumnInfo->mColumn.mDataLength    = 0;
    sColumnInfo->mColumn.mPrecision     = 0;
    sColumnInfo->mColumn.mBuffer        = NULL;
    sColumnInfo->mColumn.mMTLength      = 0;

    if( aType == ULN_MTYPE_BLOB || aType == ULN_MTYPE_CLOB )
    {
        aCache->mHasLob = ACP_TRUE;
    }
}

/*
 * Row 에 데이터를 채워넣고, 혹은 row 에서 사용자 버퍼로 복사하는 함수들
 */

// BUG-21746
ACI_RC   ulnCacheReBuildRPA(ulnCache *aCache);

ACI_RC   ulnCacheAddNewRP(ulnCache    *aCache,
                          acp_uint8_t *aRow);
acp_bool_t ulnCacheNeedMoreRPA(ulnCache *aCache);
ACI_RC     ulnCacheAddNewRPA(ulnCache *aCache);
acp_bool_t ulnCacheNeedExtendRPAB(ulnCache *aCache);
ACI_RC     ulnCacheExtendRPAB(ulnCache *aCache);

ulnColumn *ulnCacheGetColumn(ulnCache     *aCache,
                             acp_uint16_t  aColumnNumber);

ACI_RC ulnCacheRowCopyToUserBuffer(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   ulnRow       *aRow,
                                   ulnCache     *aCache,
                                   ulnStmt      *aStmt,
                                   acp_uint32_t  aUserRowNumber);

/*
 * =======================================================
 * Result Set
 *
 * 일단 끝까지 페치해 가서 확실한 갯수를 알기 전에는
 * ULN_ROWCOUNT_UNKNOWN = ID_SLONG_MAX 로 세팅되어 있다.
 * =======================================================
 */

ACP_INLINE acp_sint64_t ulnCacheGetResultSetSize(ulnCache *aCache)
{
    return aCache->mResultSetSize;
}

ACP_INLINE void ulnCacheSetResultSetSize(ulnCache *aCache, acp_sint64_t aNumberOfRows)
{
    aCache->mResultSetSize = aNumberOfRows;
}

ACP_INLINE ACI_RC ulnCacheAllocChunk( ulnCache      *aCache,
                                      acp_uint32_t   aSize,
                                      acp_uint8_t  **aData )
{
    ACI_TEST(ACP_RC_NOT_SUCCESS(
                 aclMemAreaAlloc(aCache->mChunk,
                                 (void**)aData,
                                 aSize)));

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCacheMarkChunk(ulnCache  *aCache);
ACI_RC ulnCacheBackChunkToMark(ulnCache  *aCache);

/* BUG-32474 */
acp_rc_t ulnCacheAddReadLobLocator(ulnCache     *aCache,
                                   acp_uint64_t *aLobLocatorID);
acp_rc_t ulnCacheRemoveReadLobLocator(ulnCache     *aCache,
                                      acp_uint64_t *aLobLocatorID);

/* PROJ-1789 Updatable Scrollable Cursor */

acp_bool_t ulnCacheCheckRowsCached(ulnCache     *aCache,
                                   acp_sint64_t  aStartPosition,
                                   acp_sint64_t  aEndPosition);

ACI_RC ulnCacheRebuildRPAForSensitive(ulnStmt *aStmt, ulnCache *aCache);

ACI_RC ulnCacheSetRPByPosition(ulnCache     *aCache,
                               acp_uint8_t  *aRow,
                               acp_sint32_t  aPosition);

void ulnCacheSetStartPosition(ulnCache *aCache, acp_sint64_t aStartPosition);

#define ulnCacheIsInvalidated(aCache) \
    ( (aCache)->mIsInvalidated )

#define ulnCacheSetInvalidated(aCache, aIsInvalidated) do\
{\
    (aCache)->mIsInvalidated = (aIsInvalidated);\
} while (0)

#define ulnCacheInitInvalidated(aCache) \
    ulnCacheSetInvalidated(aCache, ACP_FALSE)

#define ulnCacheInvalidate(aCache) \
    ulnCacheSetInvalidated(aCache, ACP_TRUE)

/* PROJ-2616 IPCDA */
ACI_RC ulnCacheCreateIPCDA(ulnFnContext *aFnContext, ulnDbc *aDbc);
void   ulnCacheDestoryIPCDA(ulnStmt *aStmt);

#endif /* _O_ULN_CAHCE_H_ */
