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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnCursor.h>
#include <ulnCache.h>
#include <ulnConv.h>
#include <ulsdnBindData.h>

/*
 * ===================================================================================
 * ulnCache 의 구조
 *
 *                           +----+
 *                           |    |
 *                           |    V
 *       +-------------------|----+----------------------------------------------+
 *       | struct ulnCache   |    | ulnRow     | ulnRow     |       | ulnRow     |
 *       |                   |    | ColumnInfo | ColumnInfo | . . . | ColumnInfo |
 *       | . . . mColumnInfoArray | [0]        | [1]        |       | [n]        |
 *       +------------------------+----------------------------------------------+
 *                                    where n = mParentStmt->mColumnCount
 *
 * ===================================================================================
 */

ACI_RC ulnCachePrepareColumnInfoArray(ulnCache *aCache, acp_uint16_t aColumnCount)
{
    acp_uint32_t sSizeofColumnInfo;

    if (aColumnCount >= aCache->mColumnInfoArraySize)
    {
        sSizeofColumnInfo = ACP_ALIGN8(ACI_SIZEOF(ulnColumnInfo));

        ACI_TEST(acpMemRealloc((void**)&aCache->mColumnInfoArray,
                               sSizeofColumnInfo * (aColumnCount + 1))
                 != ACP_RC_SUCCESS);

        ACI_TEST(acpMemRealloc((void**)&aCache->mColumnBuffer,
                               ULN_CACHE_MAX_SIZE_FOR_FIXED_TYPE *
                               (aColumnCount + 1)) != ACP_RC_SUCCESS);

        aCache->mColumnInfoArraySize = aColumnCount + 1;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCacheCreate(ulnCache **aCache)
{
    acp_uint32_t    sStage;
    ulnCache       *sCache;

    sStage = 0;

    ACI_TEST(acpMemAlloc((void**)&sCache, ACI_SIZEOF(ulnCache)) != ACP_RC_SUCCESS);
    sStage = 1;

    sCache->mSingleRowSize         = 0;

    sCache->mRPAB.mUsedCount = 0;
    sCache->mRPAB.mCount     = 0;
    sCache->mRPAB.mBlock     = NULL;

    sCache->mParentStmt            = NULL;

    sCache->mColumnInfoArray       = NULL;
    sCache->mColumnInfoArraySize   = 0;
    sCache->mColumnBuffer          = NULL;

    sCache->mServerError           = NULL;

    ACI_TEST(acpMemAlloc((void**)&sCache->mChunk, ACI_SIZEOF(acl_mem_area_t))
             != ACP_RC_SUCCESS);
    sStage = 2;

    /* BUG-32474
     * ulnCache를 만들 때, 사용자가 가져간 locator를 기억하기 위한 Hash 생성 */
    ACI_TEST(aclHashCreate(&sCache->mReadLobLocatorHash,
                           19,
                           ACI_SIZEOF(acp_uint64_t),
                           aclHashHashInt64,
                           aclHashCompInt64,
                           ACP_FALSE) != ACP_RC_SUCCESS);
    sStage = 3;

    /*
     * BUG-37642 Improve performance to fetch.
     *
     * 8K씩 메모리를 덩어리로 할당해 할당비용과 재사용률을 높인다.
     */
    aclMemAreaCreate(sCache->mChunk, 8192);

    ACI_TEST( ulnCacheMarkChunk( sCache ) != ACI_SUCCESS );

    sCache->mHasLob = ACP_FALSE;

    sCache->mIsInvalidated = ACP_TRUE;

    *aCache = sCache;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    /* >> BUG-40316 */
    switch( sStage )
    {
        case 3:
            aclHashDestroy( &sCache->mReadLobLocatorHash );
        case 2:
            acpMemFree( sCache->mChunk );
        case 1:
            acpMemFree( sCache );
        default:
            break;
    }
    /* << BUG-40316 */

    return ACI_FAILURE;
}

static void ulnCacheFreeAllBlocks(ulnCache *aCache)
{
    /*
     * RowBlock 을 모조리 free 하므로 캐쉬된 row 도 모조리 사라짐.
     */
    aCache->mRowCount         = 0;
    aCache->mRowStartPosition = 1;
    aCache->mRPAB.mUsedCount  = 0;
}

void ulnCacheDestroy(ulnCache *aCache)
{
    /*
     * Cache Chunk 해제
     */
    if( aCache->mChunk != NULL )
    {
        aclMemAreaFreeAll(aCache->mChunk);
        aclMemAreaDestroy(aCache->mChunk);
        acpMemFree( aCache->mChunk );
    }

    /*
     * ColumnInfoArray 해제
     */
    if (aCache->mColumnInfoArray != NULL)
    {
        acpMemFree(aCache->mColumnInfoArray);
    }

    /*
     * ColumnBuffer 해제
     */
    if (aCache->mColumnBuffer != NULL)
    {
        acpMemFree(aCache->mColumnBuffer);
    }

    /*
     * RowBlock 해제
     */
    ulnCacheFreeAllBlocks(aCache);

    /*
     * RowBlockArray 해제
     */
    if (aCache->mRPAB.mBlock != NULL)
    {
        acpMemFree(aCache->mRPAB.mBlock);
    }

    /* BUG-32474: ReadLobLocatorHash 해제 */
    aclHashDestroy(&aCache->mReadLobLocatorHash);

    /*
     * Cache 해제
     */
    acpMemFree(aCache);
}

void ulnCacheInitRowCount(ulnCache *aCache)
{
    aCache->mRowCount = 0;
}

acp_sint32_t ulnCacheGetRowCount(ulnCache *aCache)
{
    return aCache->mRowCount;
}

void ulnCacheAdjustStartPosition( ulnCache * aCache )
{
    aCache->mRowStartPosition += aCache->mRowCount;
}

/*
 * =======================================================
 *
 * 1. 한번의 fetch 의 단위가되는 row 의 갯수 계산
 *
 * 2. 서버로부터 가지고 올 필요가 있는 row 의 갯수 계산
 *
 * 3. prefetch 옵션 사용시 더 이상 ulnRow 를 할당하지 않고
 *    다시 cache 의 첫번째 row 로 돌아갈 시점 결정
 *
 * =======================================================
 */

acp_uint32_t ulnCacheCalcBlockSizeOfOneFetch(ulnCache *aCache, ulnCursor *aCursor)
{
    acp_uint32_t  sCursorSize;
    acp_uint32_t  sBlockSize;

    acp_uint32_t  sPrefetchRows;
    acp_uint32_t  sPrefetchBlocks;
    acp_uint32_t  sPrefetchMemory;

    /*
     * prefetch 옵션들을 모두 고려해서 한번의 fetch op 에 가져오게 될
     * 이상적인 row 의 갯수를 계산한다.
     *
     * non scrollable 커서에서는 이 값이 일종의 row limit 으로 작용한다.
     */

    sPrefetchRows   = ulnStmtGetAttrPrefetchRows(aCursor->mParentStmt);
    sPrefetchBlocks = ulnStmtGetAttrPrefetchBlocks(aCursor->mParentStmt);
    sPrefetchMemory = ulnStmtGetAttrPrefetchMemory(aCursor->mParentStmt);

    sCursorSize     = ulnCursorGetSize(aCursor);

    if (sPrefetchRows != 0)
    {
        sBlockSize = sPrefetchRows;
    }
    else if (sPrefetchBlocks != 0)
    {
        sBlockSize = sCursorSize * sPrefetchBlocks;
    }
    else if (sPrefetchMemory != 0)
    {
        sBlockSize = sPrefetchMemory / ulnCacheGetSingleRowSize(aCache);

        if (sBlockSize == 0)
        {
            sBlockSize = ULN_CACHE_DEFAULT_PREFETCH_ROWS;
        }
    }
    else
    {
        sBlockSize = ULN_CACHE_DEFAULT_PREFETCH_ROWS;
    }

    sBlockSize = ACP_MAX(sCursorSize, sBlockSize);

    return sBlockSize;
}

/*
 * =======================================================
 *
 * 사용자가 지정한 PREFETCH ROW SIZE를 리턴한다.
 *
 * =======================================================
 */

acp_uint32_t ulnCacheCalcPrefetchRowSize(ulnCache *aCache, ulnCursor *aCursor)
{
    acp_uint32_t  sPrefetchRowSize = 0;

    acp_uint32_t  sPrefetchRows;
    acp_uint32_t  sPrefetchBlocks;
    acp_uint32_t  sPrefetchMemory;
    acp_uint32_t  sCacheRowSize;

    sPrefetchRows   = ulnStmtGetAttrPrefetchRows(aCursor->mParentStmt);
    sPrefetchBlocks = ulnStmtGetAttrPrefetchBlocks(aCursor->mParentStmt);
    sPrefetchMemory = ulnStmtGetAttrPrefetchMemory(aCursor->mParentStmt);

    if (sPrefetchRows != 0)
    {
        sPrefetchRowSize = sPrefetchRows;
    }
    else if (sPrefetchBlocks != 0)
    {
        sPrefetchRowSize = ulnCursorGetSize(aCursor) * sPrefetchBlocks;
    }
    else if (sPrefetchMemory != 0)
    {
        //BUG-28184 [CodeSonar] Division by Zero
        sCacheRowSize = ulnCacheGetSingleRowSize(aCache);
        if ( sCacheRowSize != 0 )
        {
            sPrefetchRowSize = sPrefetchMemory / sCacheRowSize;
        }

        if (sPrefetchRowSize == 0)
        {
            sPrefetchRowSize = ULN_CACHE_DEFAULT_PREFETCH_ROWS;
        }
    }
    else
    {
        sPrefetchRowSize = ULN_CACHE_OPTIMAL_PREFETCH_ROWS;
    }

    return sPrefetchRowSize;
}

acp_sint32_t ulnCacheCalcNumberOfRowsToGet(ulnCache *aCache, ulnCursor *aCursor, acp_uint32_t aBlockSizeOfOneFetch)
{
    acp_sint64_t sNumberOfRowsToGet;

    /*
     * Note : Cache 의 row 들의 number 는 1 베이스이다.
     *
     *        0 NULL                  0 NULL                       0 NULL
     *        1 #########             1 #########                  1 #########
     *        2 #########         +-- 2 #########              +-- 2 #########
     *        3 #########         |   3 #########              |   3 #########
     *        4 ---------      cursor 4 ---------           cursor 4 #########
     *        5 ---------         |   5 ---------              |   5 #########
     *    +-- 6 ---------         +-- 6 ---------              +-- 6 #########
     *    |   7 ---------             7 ---------                  7 #########
     * cursor 8 ---------             8 ---------                  8 ---------
     *    |   9 ---------             9 ---------                  9 ---------
     *    +- 10 ---------            10 ---------                 10 ---------
     *
     * 더 가져와야 하는 row 갯수 = CursorPosition - NumberOfCachedRows + CursorSize - 1
     *
     * 사용자가 커서를 아래로    정상적으로 캐쉬에서 가지고      에러인 경우
     * 많이 스크롤 시켜서 페치   오다가 4번 row 에서 캐쉬
     * 하는 경우                 miss 발생한 경우
     * 6 - 3 + 5 - 1= 7          2 - 3 + 5 - 1 = 3               2 - 7 + 5 - 1 = -1
     *
     * 정확히 더 필요한 row 의 갯수. 음수가 될 경우는 cursor 가 cache 된 row 안에
     * 들어가서 위치해 있는 경우 밖에 없다. 따라서, sRow 가 NULL 이어서는 안되는데,
     * 이곳으로 왔으므로 심각한 에러이다.
     */

    // To Fix BUG-20409
    // Cursor Position은 Logical 함.
    sNumberOfRowsToGet = aCursor->mPosition - ulnCacheGetTotalRowCnt(aCache)
                       + aBlockSizeOfOneFetch - 1;

    return (acp_sint32_t)sNumberOfRowsToGet;
}

ulnColumn *ulnCacheGetColumn(ulnCache *aCache, acp_uint16_t aColumnNumber)
{
    ulnColumnInfo *sColumnInfo;

    sColumnInfo = ulnCacheGetColumnInfo(aCache, aColumnNumber);

    return &(sColumnInfo->mColumn);
}

/*
 * ===========================================
 * Get ulnRow
 * ===========================================
 */

ulnRow *ulnCacheGetRow(ulnCache     *aCache,
                       acp_uint32_t  aRowNumber)
{
    ulnRow *sRPA;
    ulnRow *sRP;

    sRPA = aCache->mRPAB.mBlock[aRowNumber / ULN_CACHE_MAX_ROW_IN_RPA];
    sRP  = &sRPA[aRowNumber % ULN_CACHE_MAX_ROW_IN_RPA];

    return sRP;
}

ulnRow *ulnCacheGetCachedRow(ulnCache *aCache, acp_sint64_t aLogicalCurPos)
{
    acp_sint64_t sRowNumber;

    /*
     * Note : cache 바깥에서의 row number 는 모두 1 베이스이나
     *        cache 안에서는 0 베이스이다.
     */

    // To Fix BUG-20480
    // 논리적 Cursor Position을 물리적 위치로 변경한다.
    sRowNumber = aLogicalCurPos - aCache->mRowStartPosition;

    /*
     * ULN_CURSOR_POS_AFTER_END or ULN_CURSOR_POS_BEFORE_START
     */
    if (sRowNumber < 0)
    {
        return NULL;
    }

    /*
     * row number 는 1번부터 시작한다.
     */
    if (sRowNumber >= aCache->mRowCount)
    {
        return NULL;
    }

    return ulnCacheGetRow(aCache, (acp_uint32_t)sRowNumber);
}

/*
 * ================================================================
 *
 * CACHE BUILD FUNCTIONS
 *
 * ================================================================
 */

// BUG-21746
ACI_RC ulnCacheReBuildRPA( ulnCache *aCache )
{
    ulnRow       *sNewRPA   = NULL;
    acp_uint32_t  sRPAIndex = 0;

    // 시작 위치부터 마지막 위치까지 RPA를 재구성한다.
    for( sRPAIndex = 0; sRPAIndex < aCache->mRPAB.mUsedCount; ++sRPAIndex)
    {
        ACI_TEST( ulnCacheAllocChunk( aCache,
                                      ULN_CACHE_MAX_ROW_IN_RPA * ACI_SIZEOF(ulnRow),
                                      (acp_uint8_t**)&sNewRPA )
                  != ACI_SUCCESS );

        aCache->mRPAB.mBlock[sRPAIndex] = sNewRPA;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
// BUG-21746 이미 생성되어있을 경우는 생성하지 않는다.
#define NEED_MORE_RPA(x) \
        ((((x)->mRowCount) / ULN_CACHE_MAX_ROW_IN_RPA) >= ((x)->mRPAB.mUsedCount))

ACI_RC ulnCacheAddNewRP(ulnCache    *aCache,
                        acp_uint8_t *aRow)
{
    ulnRow   *sCurRPA;
    ulnRow   *sCurRP;

    if( NEED_MORE_RPA( aCache ) )
    {
        ACI_TEST( ulnCacheAddNewRPA( aCache ) != ACI_SUCCESS );
    }

    sCurRPA = aCache->mRPAB.mBlock[aCache->mRowCount / ULN_CACHE_MAX_ROW_IN_RPA];
    sCurRP  = &sCurRPA[aCache->mRowCount % ULN_CACHE_MAX_ROW_IN_RPA];

    sCurRP->mRowNumber = aCache->mRowCount + aCache->mRowStartPosition;
    sCurRP->mRow       = aRow;

    aCache->mRowCount++;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

#define NEED_MORE_RPAB(x) \
        ((x)->mRPAB.mUsedCount + 1 > aCache->mRPAB.mCount)

ACI_RC ulnCacheAddNewRPA(ulnCache *aCache)
{
    ulnRow  *sNewRPA;

    if( NEED_MORE_RPAB( aCache ) )
    {
        ACI_TEST( ulnCacheExtendRPAB( aCache ) != ACI_SUCCESS );
    }

    ACI_TEST( ulnCacheAllocChunk( aCache,
                                  ULN_CACHE_MAX_ROW_IN_RPA * ACI_SIZEOF(ulnRow),
                                  (acp_uint8_t**)&sNewRPA )
              != ACI_SUCCESS );

    aCache->mRPAB.mBlock[aCache->mRPAB.mUsedCount] = sNewRPA;
    aCache->mRPAB.mUsedCount++;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCacheExtendRPAB(ulnCache *aCache)
{
    acp_uint32_t     sNewBlockSize;
    ulnRPAB *sRPAB = &aCache->mRPAB;

    sNewBlockSize = (sRPAB->mCount * ACI_SIZEOF(acp_uint8_t *)) +
                    (ULN_CACHE_RPAB_UNIT_COUNT * ACI_SIZEOF(acp_uint8_t *));

    ACI_TEST(acpMemRealloc((void**)&sRPAB->mBlock, sNewBlockSize) != ACP_RC_SUCCESS);

    sRPAB->mCount += ULN_CACHE_RPAB_UNIT_COUNT;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ================================================================
 *
 * Cache 되어 있는 row 의 내용을 convert 해서 사용자 버퍼에 복사
 *
 *    : SQLFetch() 의 핵심부.
 *
 * ================================================================
 */

ACI_RC ulnCacheRowCopyToUserBuffer(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   ulnRow       *aRow,
                                   ulnCache     *aCache,
                                   ulnStmt      *aStmt,
                                   acp_uint32_t  aUserRowNumber)
{
    /*
     * aUserRowNumber 는 1 베이스 인덱스
     */

    acp_uint32_t      i;
    acp_uint32_t      sColumnCount;
    acp_uint32_t      sRowStatus    = ULN_ROW_SUCCESS;
    ulnStmt          *sKeysetStmt;
    ulnColumn        *sColumn;
    ulnDescRec       *sDescRecArd;
    ulnAppBuffer      sAppBuffer;
    ulnIndLenPtrPair  sUserIndLenPair;
    acp_uint8_t      *sSrc    = aRow->mRow;

    void             *sBackupArg;

    /* PROJ-1789 Updatable Scrollable Cursor
     * - 정보를 얻고 설정하는건 KeysetStmt를 써야한다.
     * - Sensitive일때는 _PROWID가 맨 앞에 붙으므로 Column 0부터 시작한다. */
    if (aStmt->mParentStmt != NULL)
    {
        sKeysetStmt = aStmt->mParentStmt;
    }
    else
    {
        sKeysetStmt = aStmt;
    }

    i = (ulnCursorGetScrollable(&sKeysetStmt->mCursor) == SQL_SCROLLABLE) ? 0 : 1;
    sColumnCount = ulnStmtGetColumnCount(sKeysetStmt);
    for (; i <= sColumnCount; i++)
    {
        sDescRecArd = ulnStmtGetArdRec(sKeysetStmt, i);
        sColumn     = ulnCacheGetColumn(aCache, i);

        sColumn->mGDPosition    = 0;
        sColumn->mRemainTextLen = 0;
        sColumn->mGDState       = ULN_GD_ST_INITIAL;
        sColumn->mBuffer        = (acp_uint8_t*)aCache->mColumnBuffer
                                + (i * ULN_CACHE_MAX_SIZE_FOR_FIXED_TYPE);

        if( sColumn->mColumnNumber ==0 )
        {
            ulnDataBuildColumnZero( aFnContext, aRow, sColumn);
        }
        else
        {
            /* PROJ-2638 shard native linker */
            if ( ( aStmt->mShardStmtCxt.mIsMtDataFetch == ACP_TRUE ) &&
                 ( aStmt->mShardStmtCxt.mRowDataBuffer != NULL ) )
            {
                ACI_TEST( ulsdCacheRowCopyToUserBufferShardCore( aFnContext,
                                                                 aStmt,
                                                                 sSrc,
                                                                 sColumn,
                                                                 i)
                          != ACI_SUCCESS );

            }
            else
            {
                ACI_TEST(ulnDataBuildColumnFromMT(aFnContext,
                                                  sSrc,
                                                  sColumn)
                         != ACI_SUCCESS );
            }

            sSrc += sColumn->mMTLength;
        }

        /* bind 하지 않은 컬럼이면 데이타 복사는 건너 뛴다. */
        if (sDescRecArd == NULL)
        {
            continue;
        }

        // fix BUG-24380
        // SetDescField를 사용해 새로운 DescRec를 생성하였지만
        // 데이터 공간 할당이 안되어 있을 수 있다.
        if (sDescRecArd->mDataPtr == NULL)
        {
            continue;
        }

        sAppBuffer.mColumnStatus  = ULN_ROW_SUCCESS;
        sAppBuffer.mCTYPE         = sDescRecArd->mMeta.mCTYPE;
        sAppBuffer.mBuffer        = (acp_uint8_t *)ulnBindCalcUserDataAddr(sDescRecArd,
                                                                           aUserRowNumber - 1);
        sAppBuffer.mBufferSize    = (ulvULen)(sDescRecArd->mMeta.mOctetLength);
        sAppBuffer.mFileOptionPtr = ulnBindCalcUserFileOptionAddr(sDescRecArd,
                                                                  aUserRowNumber - 1);

        ulnBindCalcUserIndLenPair(sDescRecArd, aUserRowNumber - 1, &sUserIndLenPair);

        /*
         * 캐쉬에서 사용자 버퍼로 데이터 변환해서 복사.
         */

        sBackupArg        = aFnContext->mArgs;
        aFnContext->mArgs = aPtContext;

        if ( aStmt->mShardStmtCxt.mIsMtDataFetch == ACP_FALSE )
        {
            ACI_TEST( ulnConvert( aFnContext,
                                  &sAppBuffer,
                                  sColumn,
                                  aUserRowNumber,
                                  &sUserIndLenPair ) != ACI_SUCCESS );
        }
        else
        {
            /* Do Nothing. */
            /*************************************************
             * PROJ-2638 shard native linker
             * MtDataFetch일 경우에는 ulnConvert를 호출하지 않고,
             * MT 타입의 데이터를 반환한다.
             *************************************************/
        }

        /* BUG-32474 */
        if ((sAppBuffer.mCTYPE == ULN_CTYPE_BLOB_LOCATOR)
         || (sAppBuffer.mCTYPE == ULN_CTYPE_CLOB_LOCATOR))
        {
            ACI_TEST_RAISE(ulnCacheAddReadLobLocator(aCache, (acp_uint64_t*)sAppBuffer.mBuffer)
                           != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR);

#ifdef COMPILE_SHARDCLI
            /* PROJ-2739 Client-side Sharding LOB */
            ACI_TEST_RAISE( ulsdLobAddFetchLocator(
                                aStmt,
                                (acp_uint64_t *)sAppBuffer.mBuffer)
                            != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR );
#endif
        }

        aFnContext->mArgs = sBackupArg;

        sRowStatus |= sAppBuffer.mColumnStatus;
    }

    /*
     * 여태까지 리턴된 각 컬럼의 status 들을 종합해서 row status 결정
     */

    if (sRowStatus & ULN_ROW_ERROR)
    {
        ulnStmtSetAttrRowStatusValue(sKeysetStmt, aUserRowNumber - 1, SQL_ROW_ERROR);
    }
    else
    {
        if (sRowStatus & ULN_ROW_SUCCESS_WITH_INFO)
        {
            ulnStmtSetAttrRowStatusValue(sKeysetStmt, aUserRowNumber - 1, SQL_ROW_SUCCESS_WITH_INFO);
        }
        else
        {
            /* PROJ-1789 SetPos(SQL_REFRESH)로 갱신에 성공했을때는 UPDATED */
            if (aFnContext->mFuncID == ULN_FID_SETPOS)
            {
                ulnStmtSetAttrRowStatusValue(sKeysetStmt, aUserRowNumber - 1, SQL_ROW_UPDATED);
            }
            else
            {
                ulnStmtSetAttrRowStatusValue(sKeysetStmt, aUserRowNumber - 1, SQL_ROW_SUCCESS);
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnCacheRowCopyToUserBuffer.");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ================================================================
 *
 * CACHE CHUNK FUNCTIONS
 *
 * ================================================================
 */


ACI_RC ulnCacheMarkChunk( ulnCache  *aCache )
{
    aclMemAreaGetSnapshot(aCache->mChunk, &aCache->mChunkStatus);

    return ACI_SUCCESS;
}

ACI_RC ulnCacheBackChunkToMark( ulnCache  *aCache )
{
    aclMemAreaFreeToSnapshot(aCache->mChunk, &aCache->mChunkStatus);

    return ACI_SUCCESS;
}

/*
 * ======================================================
 * LOB
 * ======================================================
 */

ACI_RC ulnCacheCloseLobInCurrentContents(ulnFnContext *aFnContext,
                                         ulnPtContext *aPtContext,
                                         ulnCache     *aCache)
{
    ulnDbc       *sDbc;
    ulnRow       *sRow;
    ulnColumn    *sColumn;
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    acp_uint8_t*  sSrc;
    acp_uint32_t  sOffset;
    acp_uint64_t  sLobLocatorID;
    acp_uint64_t  sLocatorCount = 0;
    acp_uint32_t  sColumnCount;
    acp_uint32_t  i, j;
    acp_rc_t      sRC;

    cmiProtocolContext *sCtx = &(aPtContext->mCmiPtContext);
    // opcode(1) + LocatorCount(8) = 9
    acp_uint32_t  sLimitCount = (sCtx->mWriteBlock->mBlockSize - sCtx->mWriteBlock->mCursor - 9 ) / 8;
    acp_uint64_t  sBuffer[ULN_STMT_MAX_MEMORY_CHUNK_SIZE / 8];

    // BUG-21241
    // ulnFlushAndReadProtocol을 호출하기 위해 sDbc를 얻음
    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE( ulnCacheHasLob( aCache ) == ACP_FALSE, LABEL_SKIP_CLOSE );

    if (sStmt->mParentStmt != NULL)
    {
        sColumnCount = ulnStmtGetColumnCount(sStmt->mParentStmt);
    }
    else
    {
        sColumnCount = ulnStmtGetColumnCount(sStmt);
    }

    for( i = 0; i < (acp_uint32_t)ulnCacheGetRowCount( aCache ); i++ )
    {
        sRow = ulnCacheGetRow( aCache, i );
        sOffset = 0;

        for( j = 1; j <= sColumnCount; j++ )
        {
            sColumn = ulnCacheGetColumn(aCache, j);
            if( (sColumn->mMtype == ULN_MTYPE_BLOB) ||
                (sColumn->mMtype == ULN_MTYPE_CLOB) )
            {
                sSrc = sRow->mRow + sOffset;
                CM_ENDIAN_ASSIGN8(&sLobLocatorID, sSrc);

                /* BUG-32474
                 * fetch나 GetData로 lob locator를 가져갔는지 확인
                 * 가져갔으면 locator 해지는 사용자의 몫이므로 넘어간다. */
                sRC = ulnCacheRemoveReadLobLocator(aCache, &sLobLocatorID);
                if (ACP_RC_IS_SUCCESS(sRC))
                {
                    /* PROJ-2047 Strengthening LOB - LOBCACHE */
                    ulnDataGetNextColumnOffset(sColumn,
                                               sRow->mRow+sOffset,
                                               &sOffset);
                    continue;
                }
                ACI_TEST(ACP_RC_NOT_ENOENT(sRC));

                sBuffer[sLocatorCount] = sLobLocatorID;

                sLocatorCount += 1;

                if( sLocatorCount + 1 > sLimitCount )
                {
                    ACI_TEST(ulnWriteLobFreeAllREQ(aFnContext,
                                                   aPtContext,
                                                   sLocatorCount,
                                                   sBuffer)
                             != ACI_SUCCESS);

                    // BUG-21241
                    // ulnFlushProtocol()을 호출하는 대신 ulnFlushAndReadProtocol()을 호출
                    ACI_TEST(ulnFlushAndReadProtocol(aFnContext,
                                                     aPtContext,
                                                     sDbc->mConnTimeoutValue)
                             != ACI_SUCCESS);

                    sLocatorCount = 0;
                    sLimitCount = ULN_STMT_MAX_MEMORY_CHUNK_SIZE / 8;
                }
            }
            ulnDataGetNextColumnOffset(sColumn, sRow->mRow+sOffset, &sOffset);
        }
    }

    if( sLocatorCount > 0 )
    {
        ACI_TEST(ulnWriteLobFreeAllREQ(aFnContext,
                                       aPtContext,
                                       sLocatorCount,
                                       sBuffer)
                 != ACI_SUCCESS);

        // BUG-21241
        // ulnFlushProtocol()을 호출하는 대신 ulnFlushAndReadProtocol()을 호출
        ACI_TEST(ulnFlushAndReadProtocol(aFnContext,
                                         aPtContext,
                                         sDbc->mConnTimeoutValue)
                 != ACI_SUCCESS);
    }

    ACI_EXCEPTION_CONT( LABEL_SKIP_CLOSE );

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    /* BUG-38818 Prevent to fail a assertion in ulnCacheInitialize() */
    if (aCache != NULL)
    {
        if (aclHashGetTotalRecordCount(&aCache->mReadLobLocatorHash) > 0)
        {
            ulnCacheEmptyHashTable(&aCache->mReadLobLocatorHash);
        }
        else
        {
            /* Nothing */
        }
    }
    else
    {
        /* Nothing */
    }

    return ACI_FAILURE;
}

/**
 * lob locator를 Hash에 추가한다.
 * 이미 Hash에 있다면 조용히 넘어간다.
 *
 * @param[in] aCache          cache
 * @param[in] aLobLocatorID   lob locator
 *
 * @return lob locator를 잘 추가했으면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
acp_rc_t ulnCacheAddReadLobLocator(ulnCache     *aCache,
                                   acp_uint64_t *aLobLocatorID)
{
    acp_rc_t sRC;

    // 방어 코드
    ACE_ASSERT(aCache != NULL);
    ACE_ASSERT(aLobLocatorID != NULL);

    sRC = aclHashAdd(&aCache->mReadLobLocatorHash, aLobLocatorID, aLobLocatorID);
    if (ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_EEXIST(sRC))
    {
        sRC = ACI_SUCCESS;
    }
    else
    {
        sRC = ACI_FAILURE;
    }
    return sRC;
}

/**
 * Hash에서 lob locator를 제거한다.
 *
 * @param[in] aCache          cache
 * @param[in] aLobLocatorID   lob locator
 *
 * @return lob locator를 Hash에서 제거했으면 ACI_SUCCESS,
 *         Hash에 없으면 ACI_ENOENT,
 *         에러가 발생했으면 에러 코드
 */
acp_rc_t ulnCacheRemoveReadLobLocator(ulnCache     *aCache,
                                      acp_uint64_t *aLobLocatorID)
{
    acp_uint64_t    sValue;

    // 방어 코드
    ACE_ASSERT(aCache != NULL);
    ACE_ASSERT(aLobLocatorID != NULL);

    return aclHashRemove(&aCache->mReadLobLocatorHash, aLobLocatorID, (void **)&sValue);
}

/*
 * BUG-38818 Prevent to fail a assertion in ulnCacheInitialize()
 *
 * Cursor close, Fetch 과정에서 네트워크가 단절된 실패가 나는 경우
 * Lob locator hash가 제대로 안 비워지는 경우가 있기에 함수를 추가한다.
 */

/**
 * ulnCacheEmptyHashTable - Hash Table을 비운다.
 *
 * @aHashTable : 유효한 Hash Table
 */
ACI_RC ulnCacheEmptyHashTable(acl_hash_table_t *aHashTable)
{
    acp_uint64_t        *sValue;
    acl_hash_traverse_t  sHashTraverse;
    acp_rc_t             sRC;

    ACE_ASSERT(aHashTable != NULL);

    sRC = aclHashTraverseOpen(&sHashTraverse,
                              aHashTable,
                              ACP_TRUE);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRC));

    do
    {
        /* aclHashTraverseNext()는 ACP_RC_EOF, ACP_RC_SUCCESS만 리턴한다. */
        sRC = aclHashTraverseNext(&sHashTraverse, (void **)&sValue);
    } while (ACP_RC_NOT_EOF(sRC));

    aclHashTraverseClose(&sHashTraverse);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-1789 Updatable Scrollable Cursor */

/**
 * Cache에 쌓일 첫 Row의 Position을 설정한다.
 *
 * @param[in] aCache           cache object
 * @param[in] aStartPosition   start position
 */
void ulnCacheSetStartPosition(ulnCache *aCache, acp_sint64_t aStartPosition)
{
    aCache->mRowStartPosition = aStartPosition;
}

/**
 * 특정 범위의 Row가 모두 Cache되어 있는지 확인한다.
 *
 * @param[in] aCache           cache
 * @param[in] aStartPosition   start position (inclusive)
 * @param[in] aEndPosition     end position (inclusive)
 *
 * @return 지정 범위의 Row가 모두 있으면 ACP_TRUE, 아니면 ACP_FALSE
 */
acp_bool_t ulnCacheCheckRowsCached(ulnCache     *aCache,
                                   acp_sint64_t  aStartPosition,
                                   acp_sint64_t  aEndPosition)
{
    acp_bool_t sIsCached;

    if ( (aCache == NULL)
     ||  (aStartPosition < aCache->mRowStartPosition)
     ||  (aEndPosition >= (aCache->mRowStartPosition + aCache->mRowCount)) )
    {
        sIsCached = ACP_FALSE;
    }
    else
    {
        sIsCached = ACP_TRUE;
    }

    return sIsCached;
}

/**
 * Cache를 초기화하고 RPA를 다시 생성한다.
 *
 * 이 때, RP에 설정된 값이 없음을 나타내기 위해서
 * RP의 RowNumber는 NULL, Row는 0으로 초기화한다.
 *
 * @param[in] aCache cache
 *
 * @return ACP_SUCCESS if successful, or ACP_FAILURE otherwise
 */
ACI_RC ulnCacheRebuildRPAForSensitive(ulnStmt *aRowsetStmt, ulnCache *aCache)
{
    ulnRow       *sNewRPA = NULL;
    acp_uint32_t  sRPACount;
    acp_uint32_t  sRPAIndex = 0;
    acp_uint32_t  sPreBuildSize;

    if (ulnStmtGetFetchMode(aRowsetStmt) == ULN_STMT_FETCHMODE_BOOKMARK)
    {
        /* BOOKMARK로 Fetch하면, 연속되지 않은 Row를 얻는다.
         * 그러므로, Position 기반으로 접근하려면
         * RPA를 Cache된 Keyset 개수만큼 미리 만들어 두어야 한다. */
        ACE_DASSERT(aRowsetStmt->mParentStmt != NULL);
        sPreBuildSize = ulnKeysetGetKeyCount(ulnStmtGetKeyset(aRowsetStmt->mParentStmt));
    }
    else
    {
        sPreBuildSize = ulnCacheCalcBlockSizeOfOneFetch(aCache,
                                                        ulnStmtGetCursor(aRowsetStmt));
    }

    /* Hole 감지를 위해 PrefetchSize 만큼 초기화를 해둬야 한다. */
    sRPACount = (sPreBuildSize / ULN_CACHE_MAX_ROW_IN_RPA);
    if ((sPreBuildSize % ULN_CACHE_MAX_ROW_IN_RPA) > 0)
    {
        sRPACount++;
    }

    for (sRPAIndex = 0; sRPAIndex < sRPACount; sRPAIndex++)
    {
        if (NEED_MORE_RPAB(aCache))
        {
            ACI_TEST(ulnCacheExtendRPAB( aCache ) != ACI_SUCCESS);
        }

        ACI_TEST(ulnCacheAllocChunk(aCache,
                                    ULN_CACHE_MAX_ROW_IN_RPA * ACI_SIZEOF(ulnRow),
                                    (acp_uint8_t**)&sNewRPA)
                 != ACI_SUCCESS);

        aCache->mRPAB.mBlock[sRPAIndex] = sNewRPA;
        aCache->mRPAB.mUsedCount++;

        acpMemSet((void *)sNewRPA, 0,
                  ULN_CACHE_MAX_ROW_IN_RPA * ACI_SIZEOF(ulnRow));
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Position에 해당하는 RP를 설정한다.
 *
 * @param[in] aCache      cache
 * @param[in] aRow        row data
 * @param[in] aPosition   cursor position of row
 *
 * @return ACP_SUCCESS if successful, or ACP_FAILURE otherwise
 */
ACI_RC ulnCacheSetRPByPosition( ulnCache     *aCache,
                                acp_uint8_t  *aRow,
                                acp_sint32_t  aPosition )
{
    ulnRow      *sCurRPA;
    ulnRow      *sCurRP;
    acp_uint32_t sCacheIdx;

    ACE_ASSERT(aPosition >= aCache->mRowStartPosition);

    sCacheIdx = aPosition - aCache->mRowStartPosition;
    ACE_ASSERT( (sCacheIdx / ULN_CACHE_MAX_ROW_IN_RPA)
                < aCache->mRPAB.mUsedCount );

    sCurRPA = aCache->mRPAB.mBlock[sCacheIdx / ULN_CACHE_MAX_ROW_IN_RPA];
    sCurRP  = &sCurRPA[sCacheIdx % ULN_CACHE_MAX_ROW_IN_RPA];

    sCurRP->mRowNumber = aPosition;
    sCurRP->mRow       = aRow;

    aCache->mRowCount = ACP_MAX(aCache->mRowCount, sCacheIdx + 1);

    return ACI_SUCCESS;
}


/* PROJ-2616 ulnCacheCreateIPCDA
 * 설명 : IPCDA의 Connection에 1개 이상의 Statement가 생성될경우
 *        Statement별로 Memory를 생성하고 ShardMemory의 정보를 copy한다.
 *
 * @aFnContext[in] - ulnFnContext
 * @aDbc[in]       - ulnDbc
 *
 * @return : success -> ACI_SUCCESS, fail -> ACI_FAILURE
 */
ACI_RC ulnCacheCreateIPCDA(ulnFnContext *aFnContext, ulnDbc *aDbc)
{
    /*PROJ-2616*/
    acp_list_node_t    *sIterator = NULL;
    ulnStmt            *sStmtTmp  = NULL;
    cmiProtocolContext *sCtx      = NULL;

    /* PROJ-2616 FAC 자동 설정하기 */
    if ((ulnDbcGetCmiLinkImpl(aDbc) == CMI_LINK_IMPL_IPCDA) &&
        (ulnDbcGetStmtCount(aDbc) > 1))
    {
        sCtx = &aDbc->mPtContext.mCmiPtContext;
        ACP_LIST_ITERATE(&aDbc->mStmtList, sIterator)
        {
            sStmtTmp = (ulnStmt*) sIterator;

            if (sStmtTmp->mCacheIPCDA.mIsAllocFetchBuffer == ACP_FALSE)
            {
                /* mFetchBuffer에 ShardMemory가 할당되어 있을수 있으므로 NULL로 초기화 해야 한다. */
                sStmtTmp->mCacheIPCDA.mFetchBuffer = NULL;
                ACI_TEST_RAISE(acpMemAlloc((void**)&sStmtTmp->mCacheIPCDA.mFetchBuffer, cmbBlockGetIPCDASimpleQueryDataBlockSize()) != ACI_SUCCESS,
                               LABEL_NOT_ENOUGH_MEM);

                ACI_TEST((sStmtTmp->mCacheIPCDA.mReadLength + sStmtTmp->mCacheIPCDA.mRemainingLength) > cmbBlockGetIPCDASimpleQueryDataBlockSize());
                acpMemCpy(sStmtTmp->mCacheIPCDA.mFetchBuffer,
                          sCtx->mSimpleQueryFetchIPCDAReadBlock.mData,
                          (sStmtTmp->mCacheIPCDA.mReadLength + sStmtTmp->mCacheIPCDA.mRemainingLength));

                sStmtTmp->mCacheIPCDA.mIsAllocFetchBuffer = ACP_TRUE;
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnCacheCreateIPCDA");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*PROJ-2616 ulnCacheDestoryIPCDA
 *
 * 설명 : IPCDA의 Statement에 할당된 Cache 버퍼를 해제 한다.
 *
 * @aStmt[in] - ulnStmt
 *
 * @return : void
 */
void ulnCacheDestoryIPCDA(ulnStmt *aStmt)
{
    if (aStmt->mCacheIPCDA.mIsAllocFetchBuffer == ACP_TRUE &&
        aStmt->mCacheIPCDA.mFetchBuffer != NULL)
    {
        acpMemFree(aStmt->mCacheIPCDA.mFetchBuffer);
        aStmt->mCacheIPCDA.mFetchBuffer          = NULL;
        aStmt->mCacheIPCDA.mIsAllocFetchBuffer   = ACP_FALSE;
    }
}

