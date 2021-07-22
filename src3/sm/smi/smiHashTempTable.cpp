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
 * $Id $
 **********************************************************************/

#include <sdt.h>
#include <smiMain.h>
#include <smiTrans.h>
#include <smiMisc.h>
#include <smiStatement.h>
#include <smuProperty.h>
#include <smiTempTable.h>
#include <smiHashTempTable.h>

iduMemPool smiHashTempTable::mTempCursorPool;

#define SDT_CHECK_HASHSTATS( aHeader, aOpr )            \
    if( ( ++((aHeader)->mCheckCnt) % SMI_TT_STATS_INTERVAL ) == 0 ) \
    {                                                               \
        generateHashStats( (aHeader), (aOpr) );                     \
    }                                                               \
    else                                                            \
    {                                                               \
        (aHeader)->mStatsPtr->mTTLastOpr = (aOpr);                  \
    };

/*********************************************************
 * Description :
 *   서버 구동시 TempTable을 위해 주요 메모리를 초기화함
 ***********************************************************/
IDE_RC smiHashTempTable::initializeStatic()
{
    return mTempCursorPool.initialize(
                  IDU_MEM_SM_SMI,
                  (SChar*)"SMI_HASH_CURSOR_POOL",
                  smuProperty::getIteratorMemoryParallelFactor(),
                  ID_SIZEOF( smiHashTempCursor ),
                  64, /* ElemCount */
                  IDU_AUTOFREE_CHUNK_LIMIT,        /* ChunkLimit */
                  ID_TRUE,                         /* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE, /* AlignByte */
                  ID_FALSE,                        /* ForcePooling */
                  ID_TRUE,                         /* GarbageCollection */
                  ID_TRUE,                         /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY );
}

/*********************************************************
 * Description :
 *   서버 종료시 TempTable을 위해 주요 메니저를 닫음.
 ***********************************************************/
IDE_RC smiHashTempTable::destroyStatic()
{
    return mTempCursorPool.destroy();
}

/**************************************************************************
 * Description : Column의 갯수를 확인합니다.
 *
 * aTempHeader - [IN] 대상 Temp Table Header
 * aColumnList - [IN] Column의 List
 ***************************************************************************/
IDE_RC smiHashTempTable::getColumnCount( smiTempTableHeader  * aTempHeader,
                                         const smiColumnList * aColumnList )
{
    smiColumnList * sColumns = (smiColumnList *)aColumnList;
    smiColumn     * sColumn;
    UInt            sColumnCount = 0;

    aTempHeader->mRowSize  = 0;

    while( sColumns != NULL)
    {
        sColumn  = (smiColumn*)sColumns->column;
        IDE_ERROR_MSG( (sColumn->flag & SMI_COLUMN_TYPE_MASK)
                       == SMI_COLUMN_TYPE_FIXED,
                       "Error occurred while temp table create "
                       "(Tablespace ID : %"ID_UINT32_FMT")",
                       aTempHeader->mSpaceID );

        if ( (sColumn->offset + sColumn->size) > aTempHeader->mRowSize )
        {
            aTempHeader->mRowSize = sColumn->offset + sColumn->size;
        }

        sColumnCount++;
        sColumns = sColumns->next;

        /* Key Column List와는 다르게 중복 skip 하지 않으므로,
         * Column Max Count를 넘지는 않는지 정확하게 확인해야 한다.
         * List가 잘못되었을 경우에 Hang이 발생하는 것을 막기 위해
         * BUG-40079에 추가된 검증을 BUG-46265에서 while 안으로 옮긴다. */
        IDE_TEST_RAISE( sColumnCount > SMI_COLUMN_ID_MAXIMUM,
                        maximum_column_count_error );
    }

    aTempHeader->mColumnCount = sColumnCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( maximum_column_count_error );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_Maximum_Column_count_in_temptable ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description : TempTable의 Column Info를 재구축 합니다.
 *
 * aTempHeader - [IN] 대상 Temp Table Header
 * aColumnList - [IN] Column의 List
 * aKeyColumns - [IN] Key Column의 List
 ***************************************************************************/
IDE_RC smiHashTempTable::buildColumnInfo( smiTempTableHeader  * aTempHeader,
                                          const smiColumnList * aColumnList,
                                          const smiColumnList * aKeyColumns )
{
    smiColumnList * sColumns = (smiColumnList *)aColumnList;
    smiColumn     * sColumn;
    smiTempColumn * sTempColumn;
    smiColumnList * sKeyColumns;
    UInt            sColumnIdx = 0;

    while( sColumns != NULL )
    {
        IDE_ERROR( sColumnIdx < aTempHeader->mColumnCount );

        sColumn     = (smiColumn*)sColumns->column;
        sTempColumn = &aTempHeader->mColumns[ sColumnIdx ];

        /*********************** KEY COLUMN ***************************/
        /* 키 칼럼인지 찾은 후, 키 칼럼이면 그에 해당하는 Flag를 설정 */
        sKeyColumns = (smiColumnList*)aKeyColumns;
        while( sKeyColumns != NULL )
        {
            if ( sKeyColumns->column->id == sColumn->id )
            {
                /* KeyColumn의 Flag를 설정해줌 */
                sColumn->flag = sKeyColumns->column->flag;
                sColumn->flag &= ~SMI_COLUMN_USAGE_MASK;
                sColumn->flag |= SMI_COLUMN_USAGE_INDEX;
                break;
            }
            else
            {
                sKeyColumns = sKeyColumns->next;
            }
        }

        /*********************** STORE COLUMN *********************/
        idlOS::memcpy( &sTempColumn->mColumn,
                       sColumn,
                       ID_SIZEOF( smiColumn ) );

        IDE_TEST( gSmiGlobalCallBackList.findCopyDiskColumnValue(
                      sColumn,
                      &sTempColumn->mConvertToCalcForm )
                  != IDE_SUCCESS);

        IDE_TEST( gSmiGlobalCallBackList.findNull( sColumn,
                                                   sColumn->flag,
                                                   &sTempColumn->mNull )
                  != IDE_SUCCESS );

        /* PROJ-2435 order by nulls first/last */
        IDE_TEST( gSmiGlobalCallBackList.findIsNull( sColumn,
                                                     sColumn->flag,
                                                     &sTempColumn->mIsNull )
                  != IDE_SUCCESS );

        IDE_TEST( gSmiGlobalCallBackList.getNonStoringSize(
                      sColumn,
                      &sTempColumn->mStoringSize )
                  != IDE_SUCCESS);

        IDE_TEST( gSmiGlobalCallBackList.findCompare(
                      sColumn,
                      (sColumn->flag & SMI_COLUMN_ORDER_MASK) | SMI_COLUMN_COMPARE_NORMAL,
                      &sTempColumn->mCompare )
                  != IDE_SUCCESS );

        sTempColumn->mIdx = sColumnIdx;
        sTempColumn->mNextKeyColumn = NULL;
        sColumnIdx++;
        sColumns = sColumns->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : TempTable의 Key Column Info를 재구축 합니다.
 *
 * aTempHeader - [IN] 대상 Temp Table Header
 * aKeyColumns - [IN] Key Column의 List
 ***************************************************************************/
IDE_RC smiHashTempTable::buildKeyColumns( smiTempTableHeader  * aTempHeader,
                                          const smiColumnList * aKeyColumns )
{
    smiTempColumn * sTempColumn;
    smiColumnList * sKeyColumns;
    smiTempColumn * sPrevKeyColumn;
    UInt            sColumnIdx;
    UInt            sColumnCount;

    /* KeyColumnList를 구성함 */
    sKeyColumns    = (smiColumnList*)aKeyColumns;
    sColumnIdx     = sKeyColumns->column->id & SMI_COLUMN_ID_MASK;
    sPrevKeyColumn = &aTempHeader->mColumns[ sColumnIdx ];
    sColumnCount   = 0;
    aTempHeader->mKeyColumnList = sPrevKeyColumn;
    aTempHeader->mKeyEndOffset  = sKeyColumns->column->offset + sKeyColumns->column->size;

    while( sKeyColumns->next != NULL )
    {
        // 중복되어도 skip하면 되므로 정확하게 Column Max Count 이하인지
        // 확인 할 필요는 없다. Hang이 발생하지 않을 정도로 적당히 확인한다.
        IDE_ERROR( ++sColumnCount <= ( SMI_COLUMN_ID_MAXIMUM * 2 ) );

        sKeyColumns = sKeyColumns->next;
        sColumnIdx  = sKeyColumns->column->id & SMI_COLUMN_ID_MASK;

        IDE_ERROR( sColumnIdx < aTempHeader->mColumnCount );

        sTempColumn = &aTempHeader->mColumns[ sColumnIdx ];

        IDE_ERROR( sKeyColumns->column->id == sTempColumn->mColumn.id );

        if ( sTempColumn->mNextKeyColumn == NULL )
        {
            sPrevKeyColumn->mNextKeyColumn = sTempColumn;
            sPrevKeyColumn = sTempColumn;
        }
        else
        {
            // BUG-46265 Create Disk Temp Table 에서 Key column list가
            // 순환 할 수도 있는지 검증합니다.
            // NULL이 아닌 경우는 이미 앞서 Key Column list 에 포함 한 것이다.
            // 이를 skip하지 않으면 hang이나 잘못된 order로 출력되므로 skip한다.
            // Order by I0, I1, I0 예를 들면 첫번째 I0에서 이미 I0로 정렬 되므로
            // 두번째 I0는 의미가 없다, skip 해도 된다.
            // QX에서 중복 제거 하므로 중복해서 내려오는 경우는 없지만,
            // 만약의 경우를 위해서 검증한다.

            IDE_DASSERT( 0 );
        }

        if ( aTempHeader->mKeyEndOffset < ( sKeyColumns->column->offset +
                                            sKeyColumns->column->size ))
        {
            aTempHeader->mKeyEndOffset = ( sKeyColumns->column->offset +
                                           sKeyColumns->column->size );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***************************************************************************
 * Description : TempTable을 생성합니다.
 *
 * aStatistics   - [IN] 통계정보
 * aSpaceID      - [IN] TablespaceID
 * aWorkAreaSize - [IN] 생성할 TempTable의 WA 크기. (0이면 자동 설정)
 * aStatement    - [IN] Statement
 * aFlag         - [IN] TempTable의 타입을 설정함
 * aColumnList   - [IN] Column 목록
 * aKeyColumns   - [IN] KeyColumn(정렬할) Column 목록
 * aTable        - [OUT] 생성된 TempTable
 ***************************************************************************/
IDE_RC smiHashTempTable::create( idvSQL              * aStatistics,
                                 scSpaceID             aSpaceID,
                                 ULong                 aInitWorkAreaSize,
                                 smiStatement        * aStatement,
                                 UInt                  aFlag,
                                 const smiColumnList * aColumnList,
                                 const smiColumnList * aKeyColumns,
                                 const void         ** aTable )
{
    smiTempTableHeader   * sHeader;
    UInt                   sState = 0;

    IDE_TEST_RAISE( sctTableSpaceMgr::isTempTableSpace( aSpaceID ) != ID_TRUE,
                    tablespaceID_error );

    /* TempTablespace에  Lock을 잡는다. */
    IDE_TEST( sctTableSpaceMgr::lockAndValidateTBS(
                                        aStatement->getTrans()->getTrans(),
                                        aSpaceID,
                                        SCT_VAL_DDL_DML,
                                        ID_TRUE,   /* intent lock  여부 */
                                        ID_TRUE,  /* exclusive lock */
                                        sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aStatement->getTrans()->getTrans()) )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "smiHashTempTable::create::alloc" );
    IDE_TEST( smiTempTable::allocTempTableHdr( &sHeader ) != IDE_SUCCESS );
    sState = 1;
    idlOS::memset( sHeader, 0, ID_SIZEOF( smiTempTableHeader ) );

    /************ TempTable 통계정보 초기화 *************/
    smiTempTable::initStatsPtr( sHeader,
                                aStatistics,
                                aStatement );
    
    generateHashStats( sHeader,
                       SMI_TTOPR_CREATE );
    
    sHeader->mRowCount       = 0;
    sHeader->mWASegment      = NULL;
    sHeader->mTTState        = SMI_TTSTATE_INIT;
    sHeader->mTTFlag         = aFlag | SMI_TTFLAG_TYPE_HASH ;
    sHeader->mSpaceID        = aSpaceID;
    sHeader->mHitSequence    = 1;
    sHeader->mTempCursorList = NULL;
    sHeader->mStatistics     = aStatistics;
    sHeader->mCheckCnt       = 0;

    sHeader->mSortGroupID  = 0;
    sHeader->mRowBuffer4Compare    = NULL;
    sHeader->mRowBuffer4CompareSub = NULL;

    IDE_TEST( getColumnCount( sHeader,
                              aColumnList ) != IDE_SUCCESS );

    /* smiTempTable_create_malloc_Columns.tc */
    IDU_FIT_POINT("smiHashTempTable::create::malloc::Columns");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMI,
                                 (ULong)ID_SIZEOF(smiTempColumn) * sHeader->mColumnCount,
                                 (void**)&sHeader->mColumns )
        != IDE_SUCCESS );
    sState = 2;
    sHeader->mStatsPtr->mRuntimeMemSize += ID_SIZEOF(smiTempColumn) * sHeader->mColumnCount;

    /***************************************************************
     * Column정보 설정
     ****************************************************************/
    IDE_TEST( buildColumnInfo( sHeader,
                               aColumnList,
                               aKeyColumns ) != IDE_SUCCESS );

    if ( aKeyColumns != NULL )
    {
        IDE_TEST( buildKeyColumns( sHeader,
                                   aKeyColumns ) != IDE_SUCCESS );
    }
    else
    {
        sHeader->mKeyEndOffset = sHeader->mRowSize;
    }

    /* smiTempTable_create_malloc_RowBuffer4Fetch.tc */
    IDU_FIT_POINT("smiHashTempTable::create::malloc::RowBuffer4Fetch");
    if ( SM_IS_FLAG_ON( sHeader->mTTFlag,  SMI_TTFLAG_UNIQUE ) )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMI,
                                     sHeader->mRowSize,
                                     (void**)&sHeader->mRowBuffer4Fetch )
                  != IDE_SUCCESS );
        sHeader->mStatsPtr->mRuntimeMemSize += sHeader->mRowSize;
    }
    sState = 3;

    IDE_TEST( sdtHashModule::createHashSegment( sHeader,
                                                aInitWorkAreaSize )
              != IDE_SUCCESS );
    sState = 4;

    sdtHashModule::initHashTempHeader( sHeader );

    *aTable = (const void*)sHeader;

    return IDE_SUCCESS;

    IDE_EXCEPTION( tablespaceID_error );
    {
        ideLog::log( IDE_SM_0, "[FAILURE] Fatal error during create disk temp table "
                               "(Tablespace ID : %"ID_UINT32_FMT")",
                               aSpaceID );
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );
    }

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 4:
        (void)sdtHashModule::dropHashSegment( (sdtHashSegHdr*)sHeader->mWASegment );
    case 3:
        if ( sHeader->mRowBuffer4Fetch != NULL )
        {
            (void)iduMemMgr::free( sHeader->mRowBuffer4Fetch );
            sHeader->mRowBuffer4Fetch = NULL;
        }
    case 2:
        (void)iduMemMgr::free( sHeader->mColumns );
        sHeader->mColumns = NULL;
    case 1:
        (void)smiTempTable::freeTempTableHdr( sHeader );
        sHeader = NULL;
        break;
    default:
        break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : TempTable을 삭제합니다.
 *
 * aTable   - [IN] 대상 Table
 ***************************************************************************/
IDE_RC smiHashTempTable::drop( void    * aTable )
{
    UInt                 sState  = 5;
    smiTempTableHeader * sHeader = (smiTempTableHeader*)aTable;

    /* Drop은 반드시 동작 하여야 하기 때문에 Session check를 하지 않는다.*/
    sHeader->mStatsPtr->mDropTV = smiGetCurrTime();
    generateHashStats( sHeader,
                       SMI_TTOPR_DROP );

    sdtHashModule::estimateHashSize( sHeader );

    IDE_TEST( closeAllCursor( sHeader ) != IDE_SUCCESS );

    sState = 4;
    IDE_TEST( sdtHashModule::dropHashSegment( (sdtHashSegHdr*)sHeader->mWASegment ) != IDE_SUCCESS );

    /* 통계정보 누적시킴 */
    smiTempTable::accumulateStats( sHeader );

    sState = 3;
    if ( sHeader->mNullRow != NULL )
    {
        IDE_TEST( iduMemMgr::free( sHeader->mNullRow )
                  != IDE_SUCCESS );
        sHeader->mNullRow = NULL;
    }

    sState = 2;
    if ( sHeader->mRowBuffer4Fetch != NULL )
    {
        IDE_TEST( iduMemMgr::free( sHeader->mRowBuffer4Fetch )
                  != IDE_SUCCESS );
        sHeader->mRowBuffer4Fetch = NULL;
    }
    sState = 1;
    IDE_TEST( iduMemMgr::free( sHeader->mColumns )
              != IDE_SUCCESS );
    sHeader->mColumns = NULL;

    sState = 0;
    IDE_TEST( smiTempTable::freeTempTableHdr( sHeader ) != IDE_SUCCESS );
    sHeader = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 5:
        (void)sdtHashModule::dropHashSegment( (sdtHashSegHdr*)sHeader->mWASegment );
    case 4:
        if ( sHeader->mNullRow != NULL )
        {
            (void)iduMemMgr::free( sHeader->mNullRow );
        }
    case 3:
        if ( sHeader->mRowBuffer4Fetch != NULL )
        {
            (void)iduMemMgr::free( sHeader->mRowBuffer4Fetch );
        }
    case 2:
        (void)iduMemMgr::free( sHeader->mColumns );
    case 1:
        (void)smiTempTable::freeTempTableHdr( sHeader );
        break;
    default:
        break;
    }
    return IDE_FAILURE;
}

/**************************************************************************
 * Description : truncateTable하고 커서를 닫습니다.
 *
 * aTable   - [IN] 대상 Table
 ***************************************************************************/
IDE_RC smiHashTempTable::clear( void   * aTable )
{
    smiTempTableHeader * sHeader = (smiTempTableHeader*)aTable;
    sdtHashSegHdr      * sWASeg  = (sdtHashSegHdr*)sHeader->mWASegment;
    
    //BUG-48477: 
    static UInt sTRCChkCnt = 0; 

    if ( ( sHeader->mTTState != SMI_TTSTATE_INIT ) ||
         ( sHeader->mRowCount != 0 ) ||
         ( sWASeg->mUsedWCBPtr != SDT_USED_PAGE_PTR_TERMINATED ) ||
         ( sWASeg->mOpenCursorType > SMI_HASH_CURSOR_INIT ) ||
         ( sWASeg->mHashSlotAllocPageCount != 0 ) ||
         ( sWASeg->mAllocWAPageCount != 0 ) ) 
    {
        if ( sHeader->mTTState == SMI_TTSTATE_INIT )
        {
            if ( sTRCChkCnt < 10 )
            {   
                ideLog::log(IDE_ERR_0,  "RowCount               = %"ID_INT64_FMT"\n"
                                        "mUsedWCBPtr            = %"ID_xPOINTER_FMT"\n"
                                        "OpenCursorType         = %"ID_UINT32_FMT"\n"
                                        "HashSlotAllocPageCount = %"ID_UINT64_FMT"\n"
                                        "AllocWAPageCount       = %"ID_UINT64_FMT"\n", 
                                        sHeader->mRowCount, 
                                        sWASeg->mUsedWCBPtr, 
                                        sWASeg->mOpenCursorType, 
                                        sWASeg->mHashSlotAllocPageCount,
                                        sWASeg->mAllocWAPageCount );
                sTRCChkCnt++;
            }
        }

        generateHashStats( sHeader,
                           SMI_TTOPR_CLEAR );

        sdtHashModule::estimateHashSize( sHeader );

        IDE_TEST( closeAllCursor( sHeader ) != IDE_SUCCESS );

        sHeader->mRowCount       = 0;
        sHeader->mTTState        = SMI_TTSTATE_INIT;
        sHeader->mHitSequence    = 1;

        IDE_TEST( sdtHashModule::truncateHashSegment( sWASeg )
                  != IDE_SUCCESS );

        sdtHashModule::initHashTempHeader( sHeader );
    } 
    else
    {
        if ( sHeader->mHitSequence != 1 ) 
        {
            sHeader->mHitSequence = 1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( sHeader );

    return IDE_FAILURE;
}


/**************************************************************************
 * Description : HitFlag를 초기화합니다.
 * 실제로는 Sequence값만 초기화 하여, Row의 HitSequence와 값을 다르게 하고,
 * 이 값이 다르면 Hit돼지 않았다고 판단합니다.
 *
 * aTable    - [IN] 대상 Table
 ***************************************************************************/
void smiHashTempTable::clearHitFlag( void   * aTable )
{
    IDE_DASSERT( ((smiTempTableHeader*)aTable)->mHitSequence < ID_UINT_MAX );
    ((smiTempTableHeader*)aTable)->mHitSequence++;
}

/**************************************************************************
 * Description : Hash에 데이터를 삽입합니다.
 *
 * aTable       - [IN] 대상 Table
 * aValue       - [IN] 삽입할 Value
 * aHashValue   - [IN] 삽입할 HashValue ( HashTemp만 유효 )
 * aGRID        - [OUT] 삽입한 위치
 * aResult      - [OUT] 삽입이 성공하였는가? ( UniqueViolation Check용 )
 ***************************************************************************/
IDE_RC smiHashTempTable::insert( void     * aTable,
                                 smiValue * aValue,
                                 UInt       aHashValue,
                                 scGRID   * aGRID,
                                 idBool   * aResult )
{
    SDT_CHECK_HASHSTATS( (smiTempTableHeader*)aTable,
                         SMI_TTOPR_INSERT );

    return sdtHashModule::insert( (smiTempTableHeader*)aTable,
                                  aValue,
                                  aHashValue,
                                  aGRID,
                                  aResult );
}

/**************************************************************************
 * Description : 특정 row를 갱신합니다.
 *
 * aCursor  - [IN] 갱신할 Row를 가리키는 커서
 * aValue   - [IN] 갱신할 Value
 ***************************************************************************/
IDE_RC smiHashTempTable::update(smiHashTempCursor * aCursor,
                                smiValue          * aValue )
{
    SDT_CHECK_HASHSTATS( (smiTempTableHeader*)aCursor->mTTHeader,
                         SMI_TTOPR_UPDATE );

    return sdtHashModule::update( aCursor, aValue ) ;
}

/**************************************************************************
 * Description : HitFlag를 설정합니다.
 *
 * aCursor   - [IN] 갱신할 Row를 가리키는 커서
 ***************************************************************************/
void smiHashTempTable::setHitFlag( smiHashTempCursor * aCursor )
{
    SDT_CHECK_HASHSTATS( (smiTempTableHeader*)aCursor->mTTHeader,
                         SMI_TTOPR_SETHITFLAG );

    sdtHashModule::setHitFlag( aCursor );

    return ;
}

/**************************************************************************
 * Description : HitFlag 여부를 결정합니다.
 *
 * aCursor   - [IN] 현재 Row를 가리키는 커서
 ***************************************************************************/
idBool smiHashTempTable::isHitFlagged( smiHashTempCursor * aCursor )
{
    /* PROJ-2339
     * Hit여부는 아래와 같이 확인한다.
     * ( (Temp Table의 HitSequence) == (현재 Row의 HitSequence) ) ? T : F;
     *
     * setHitFlag() :
     * 현재 Row의 HitSequence를 1 증가시킨다.
     *
     * clearHitFlag() :
     * Temp Table의 HitSequence를 1 증가시킨다.
     * 이러면 모든 Row가 Non-Hit되므로 HitFlag를 모두 지우는 것과 같은 효과를
     * I/O 비용 없이 처리할 수 있다.
     *
     * (HitFlag 개념은 PROJ-2201에서 구현되었다.)
     */

    // TTHeader의 HitSequence를 입력으로, HitFlag가 있는지 반환한다.
    return sdtHashModule::isHitFlagged( aCursor );
}


/**************************************************************************
 * Description : GRID로 Row를 가져옵니다.
 *
 * aTable       - [IN] 대상 Table
 * aGRID        - [IN] 대상 Row
 * aDestRowBuf  - [OUT] 리턴할 Row를 저장할 버퍼
 ***************************************************************************/
IDE_RC smiHashTempTable::fetchFromGRID( void     * aTable,
                                        scGRID     aGRID,
                                        void     * aDestRowBuf )
{
    smiTempTableHeader * sHeader = (smiTempTableHeader*)aTable;

    SDT_CHECK_HASHSTATS( sHeader,
                         SMI_TTOPR_FETCH_FROMGRID );

    IDE_DASSERT( ( SC_MAKE_SPACE( aGRID ) != SDT_SPACEID_WAMAP ) &&
                 ( SC_MAKE_SPACE( aGRID ) != SDT_SPACEID_WORKAREA ) );

    return sdtHashModule::fetchFromGRID( sHeader,
                                         aGRID,
                                         aDestRowBuf );
}


/***************************************************************************
 * Description : 커서를 할당한다.
 *
 * aTable      - [IN] 대상 Table
 * aColumns    - [IN] 가져올 Column 정보
 * aCursorType - [IN] Cursor의 Type을 지정한다.
 * aCursor     - [OUT] 할당한 Cursor를 반환
 ***************************************************************************/
IDE_RC smiHashTempTable::allocHashCursor( void                * aTable,
                                          const smiColumnList * aColumns,
                                          UInt                  aCursorType,
                                          smiHashTempCursor  ** aCursor )
{
    smiTempTableHeader * sHeader = (smiTempTableHeader*)aTable;
    smiHashTempCursor  * sCursor;
    const smiColumn    * sColumn;
    UInt                 sState = 0;

    IDU_FIT_POINT_RAISE( "smiHashTempTable::openCursor::alloc", memery_allocate_failed );

    /* 바로 앞에 할당한 cursor 기준으로 확인한다.
     * hash cursor는 update등도 사용된다.
     * mWCBPtr이 변경 될 때 마다 writePage 하는데
     * 만약 mWcb가 남아있고, dirty라면
     * 새 cursor로 옮기기 전에 flush 해준다.*/
    sCursor = (smiHashTempCursor*)sHeader->mTempCursorList;

    IDE_TEST( mTempCursorPool.alloc( (void**)&sCursor ) != IDE_SUCCESS );
    sState = 1;

    sCursor->mTTHeader      = sHeader;
    sCursor->mUpdateColumns = (smiColumnList*)aColumns;
    sCursor->mUpdateEndOffset = 0;
    sCursor->mWASegment     = sHeader->mWASegment;
    sCursor->mWCBPtr        = NULL;

    while ( aColumns != NULL )
    {
        sColumn = aColumns->column;
        if (( sColumn->offset + sColumn->size ) > sCursor->mUpdateEndOffset )
        {
            sCursor->mUpdateEndOffset = sColumn->offset + sColumn->size;
        }
        aColumns = aColumns->next;
    }

    IDE_TEST( sdtHashModule::initHashCursor( sCursor,
                                             aCursorType ) != IDE_SUCCESS );
    sState = 2;

    *aCursor = sCursor;

    /* 예외처리가 일어나지 않는 경우가 되어서야 Link에 연결함 */
    sCursor->mNext = (smiHashTempCursor*)sHeader->mTempCursorList;
    sHeader->mTempCursorList = (void*)sCursor;

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( memery_allocate_failed );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
#endif
    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 2:
        // nothing
    case 1:
        (void) mTempCursorPool.memfree( sCursor );
        break;
    default:
        break;
    }

    smiTempTable::checkAndDump( sHeader );

    return IDE_FAILURE;
}


/***************************************************************************
 * Description : Hash Scan용 커서를 연다. HashValue가 변경 될 때 마다 다시 연다.
 * 해당 Hash Row가 있는지 Hash Value를 기반으로 해서,Hash Slot을 보고 1차 판단하여
 * 없으면 FALSE를, 있을수도 있다고 판단되면 TRUE를 리턴한다.
 * (True - 있을수도 있고 없을수도 있음, False - 확실하게 없음)
 * True return시에 Hash value와 Hash slot의 pointer를 Cursor에 세팅한다.
 * Hash Temp Table에서 가장 많이 호출되는 함수 중 하나로 성능이 상당히 중요하다.
 *
 * aCursor    - [IN/OUT] 대상 Cursor
 * aHashValue - [IN] 탐색 할 Row의 Hash Value
 ***************************************************************************/
idBool smiHashTempTable::checkHashSlot( smiHashTempCursor* aHashCursor,
                                        UInt               aHashValue )
{
    IDU_FIT_POINT( "1.BUG-48313@smiHashTempTable::checkHashSlot" );
    
    return sdtHashModule::checkHashSlot( aHashCursor,
                                         aHashValue );
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;
    return ID_FALSE;
#endif
}

/***************************************************************************
 * Description : Update용 Scan용 커서를 다시 연다.
 *
 * aCursor    - [IN/OUT] 대상 Cursor
 * aHashValue - [IN] 탐색 할 Row의 Hash Value
 ***************************************************************************/
idBool smiHashTempTable::checkUpdateHashSlot( smiHashTempCursor* aHashCursor,
                                              UInt               aHashValue )
{
    return sdtHashModule::checkUpdateHashSlot( aHashCursor,
                                               aHashValue );
}

/***************************************************************************
 * Description : 조건(HashValue, KeyFilter)에 맞는 첫번째 Row를 반환한다.
 *               WorkArea를 탐에 유리하도록 재구성 하는 역할도 한다.
 *               openHashCursor를 최대한 빠르게 동작하도록 하기 위하여
 *               일부 cursor open의 기능을 옮겨왔다.
 *
 * aCursor     - [IN] 대상 Cursor
 * aFlag       - [IN] TempCursor의 타입/용도/조건 등을 나타냄
 * aRowFilter  - [IN] RowFilter
 * aRow        - [OUT] 찾은 Row
 * aRowGRID    - [OUT] 찾은 row의 GRID
 ***************************************************************************/
IDE_RC smiHashTempTable::openHashCursor( smiHashTempCursor   * aCursor,
                                         UInt                  aFlag,
                                         const smiCallBack   * aRowFilter,
                                         UChar              ** aRow,
                                         scGRID              * aRowGRID )
{
    /* Insert된 것이 없다면 cursor가 open 되지 않는다. */
    IDE_DASSERT( aCursor->mTTHeader->mRowCount != 0 );

    SDT_CHECK_HASHSTATS( aCursor->mTTHeader,
                         SMI_TTOPR_OPENCURSOR_HASH );

    SM_SET_FLAG_OFF( aFlag, SMI_TCFLAG_FILTER_MASK );

    if ( aRowFilter != smiGetDefaultFilter() )
    {
        SM_SET_FLAG_ON( aFlag, SMI_TCFLAG_FILTER_ROW );
    }

    aCursor->mRowFilter = aRowFilter;
    aCursor->mTCFlag    = aFlag;

    return sdtHashModule::openHashCursor( aCursor,
                                          aRow,
                                          aRowGRID );
}


/***************************************************************************
 * Description : Update용 Cursor를 통해서 첫번 째 row를 받아온다.
 *               Update Cursor는 Unique Hash이므로 FetchNext는 없다.
 *
 * aCursor     - [IN] 대상 Cursor
 * aFlag       - [IN] TempCursor의 타입/용도/조건 등을 나타냄
 * aRowFilter  - [IN] RowFilter
 * aRow        - [OUT] 찾은 Row
 * aRowGRID    - [OUT] 찾은 row의 GRID
 ***************************************************************************/
IDE_RC smiHashTempTable::openUpdateHashCursor( smiHashTempCursor   * aCursor,
                                               UInt                  aFlag,
                                               const smiCallBack   * aRowFilter,
                                               UChar              ** aRow,
                                               scGRID              * aRowGRID )
{
    /* Insert된 것이 없다면 open이 호출되지 않는다. */
    IDE_DASSERT( aCursor->mTTHeader->mRowCount != 0 );

    SDT_CHECK_HASHSTATS( aCursor->mTTHeader,
                         SMI_TTOPR_OPENCURSOR_UPDATE );

    SM_SET_FLAG_OFF( aFlag, SMI_TCFLAG_FILTER_MASK );

    if ( aRowFilter != smiGetDefaultFilter() )
    {
        SM_SET_FLAG_ON( aFlag, SMI_TCFLAG_FILTER_ROW );
    }

    aCursor->mRowFilter = aRowFilter;
    aCursor->mTCFlag    = aFlag;

    return sdtHashModule::openUpdateHashCursor( aCursor,
                                                aRow,
                                                aRowGRID );
}


/***************************************************************************
 * Description : Full Scan에서 첫번째 Row를 가져온다.
 *
 * aCursor     - [IN] 대상 Cursor
 * aFlag       - [IN] TempCursor의 타입/용도/조건 등을 나타냄
 * aRow        - [OUT] 찾은 Row
 * aRowGRID    - [OUT] 찾은 row의 GRID
 ***************************************************************************/
IDE_RC smiHashTempTable::openFullScanCursor( smiHashTempCursor   * aCursor,
                                             UInt                  aFlag,
                                             UChar              ** aRow,
                                             scGRID              * aRowGRID )
{
    if ( aCursor->mTTHeader->mRowCount != 0 )
    {
        SDT_CHECK_HASHSTATS( aCursor->mTTHeader,
                             SMI_TTOPR_OPENCURSOR_FULL );

        SM_SET_FLAG_OFF( aFlag, SMI_TCFLAG_FILTER_MASK );

        aCursor->mTCFlag = aFlag;

        IDE_DASSERT( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) );
        IDE_DASSERT( SM_IS_FLAG_OFF( aCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) );

        return sdtHashModule::openFullScanCursor( aCursor,
                                                  aRow,
                                                  aRowGRID );
    }
    else
    {
        SC_MAKE_NULL_GRID(*aRowGRID);
        *aRow = NULL;
        aCursor->mWAPagePtr = NULL; // fetch full next를 호출 할 때를 대비함
        return IDE_SUCCESS;
    }
}

/**************************************************************************
 * Description : Cursor로 부터 다음 Row를 가져옵니다(FullScan)
 *
 * aCursor   - [IN] 대상 Cursor
 * aRow      - [OUT] 대상 Row
 * aGRID     - [OUT] 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC smiHashTempTable::fetchFullNext( smiHashTempCursor  * aCursor,
                                        UChar             ** aRow,
                                        scGRID             * aRowGRID )
{
    if ( aCursor->mTTHeader->mRowCount != 0 )
    {
        SDT_CHECK_HASHSTATS( aCursor->mTTHeader,
                             SMI_TTOPR_FETCH_FULL );

        return sdtHashModule::fetchFullNext( aCursor,
                                             aRow,
                                             aRowGRID);
    }
    else
    {
        SC_MAKE_NULL_GRID(*aRowGRID);
        *aRow = NULL;
        aCursor->mWAPagePtr = NULL; // fetch full next를 호출 할 때를 대비함

        return IDE_SUCCESS;
    }
}

/**************************************************************************
 * Description : Cursor로 부터 다음 Row를 가져옵니다( Hash Scan )
 *
 * aCursor   - [IN] 대상 Cursor
 * aRow      - [OUT] 대상 Row
 * aGRID     - [OUT] 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC smiHashTempTable::fetchHashNextInternal( smiHashTempCursor  * aCursor,
                                                UChar             ** aRow,
                                                scGRID             * aRowGRID )
{
    SDT_CHECK_HASHSTATS( (smiTempTableHeader*)aCursor->mTTHeader,
                         SMI_TTOPR_FETCH_HASH );

    return sdtHashModule::fetchHashNext( aCursor,
                                         aRow,
                                         aRowGRID );
}

/**************************************************************************
 * Description : 모든 커서를 닫습니다.
 *
 * aHeader  - [IN] 대상 Temp Table Header
 ***************************************************************************/
IDE_RC smiHashTempTable::closeAllCursor( smiTempTableHeader * aHeader )
{
    smiHashTempCursor  * sCursor;
    smiHashTempCursor  * sNxtCursor;

    sCursor = (smiHashTempCursor*)aHeader->mTempCursorList;
    while( sCursor != NULL )
    {
        sNxtCursor = sCursor->mNext;

        IDE_TEST( mTempCursorPool.memfree( sCursor ) != IDE_SUCCESS );

        sCursor = sNxtCursor;
    }

    aHeader->mTempCursorList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Page개수, Record개수를 반환함. PlanGraph 찍기 위함
 *
 * aTable       - [IN] 대상 Table
 * aPageCount   - [OUT] 총 Page 개수
 * aRecordCount - [OUT] 총 Row 개수
 ***************************************************************************/
void smiHashTempTable::getDisplayInfo( void  * aTable,
                                       ULong * aPageCount,
                                       SLong * aRecordCount )
{
    smiTempTableHeader * sHeader    = (smiTempTableHeader*)aTable;
    sdtHashSegHdr      * sWASegment = (sdtHashSegHdr*)sHeader->mWASegment;

    *aRecordCount = sHeader->mRowCount;

    // BUG-39728
    // parallel aggregation 을 사용할때 temp table 가 부족해서 에러가 발생한 경우
    // getDisplayInfo 를 호출하면 FATAL 이 발생한다.
    // 이때 sHeader 는 정상이지만 sWASegment가 NULL 이다.
    // 이 상황에서 getDisplayInfo 의 return 값은 중요치 않으므로 FATAL 이 발생하지 않도록 한다.
    if ( sWASegment != NULL )
    {
        *aPageCount = sdtHashModule::getNExtentCount( sWASegment ) * SDT_WAEXTENT_PAGECOUNT ;
    }
    else
    {
        *aPageCount = 0;
    }

    return ;
}

/**************************************************************************
 * Description : HashSlot의 최대 갯수를 가져온다.
 *
 * aTable       - [IN] 대상 Table (NULL일 경우 설정값)
 ***************************************************************************/
ULong smiHashTempTable::getMaxHashBucketCount( void * aTable )
{
    smiTempTableHeader * sHeader;
    sdtHashSegHdr      * sWASegment;
    ULong                sMaxBucketCount;
    ULong                sWAExtentCount;

    if ( aTable != NULL )
    {
        sHeader       = (smiTempTableHeader*)aTable;
        sWASegment    = (sdtHashSegHdr*)sHeader->mWASegment;
        sWAExtentCount = sWASegment->mMaxWAExtentCount;
    }
    else
    {
        sWAExtentCount = sdtHashModule::getMaxWAExtentCount();
    }

    sMaxBucketCount = ( sWAExtentCount - 1 ) * SDT_WAEXTENT_SIZE / ID_SIZEOF( sdtHashSlot );

    return sMaxBucketCount;
}

/**************************************************************************
 * Description : WorkArea 상의 Row의 최대 적재 공간을 가져온다.
 *
 * aTable       - [IN] 대상 Table
 ***************************************************************************/
ULong smiHashTempTable::getRowAreaSize( void * aTable )
{
    smiTempTableHeader * sHeader;
    sdtHashSegHdr      * sWASegment;
    ULong                sRowPageCount;

    if ( aTable != NULL )
    {
        sHeader       = (smiTempTableHeader*)aTable;
        sWASegment    = (sdtHashSegHdr*)sHeader->mWASegment;
        sRowPageCount = ( sWASegment->mMaxWAExtentCount * SDT_WAEXTENT_PAGECOUNT )
                        - sWASegment->mHashSlotPageCount - sWASegment->mSubHashPageCount ;
    }
    else
    {
        sRowPageCount = ( sdtHashModule::getMaxWAExtentCount() * SDT_WAEXTENT_PAGECOUNT )
                        - sdtHashModule::getHashSlotPageCount()
                        - sdtHashModule::getSubHashPageCount() ;
    }

    return sRowPageCount * SD_PAGE_SIZE;
}

/**************************************************************************
 * Description : 변경되는 통계정보를 다시 생성함
 *
 * aHeader  - [IN] 대상 Table
 * aOPr     - [IN] 현재의 Operation
 ***************************************************************************/
void smiHashTempTable::generateHashStats( smiTempTableHeader * aHeader,
                                          smiTempTableOpr      aOpr )
{
    smiTempTableStats * sStats ;
    sdtHashSegHdr     * sWASegment = (sdtHashSegHdr*)aHeader->mWASegment;

    if ( ( smiGetCurrTime() - aHeader->mStatsPtr->mCreateTV >=
           smuProperty::getTempStatsWatchTime() ) &&
         ( aHeader->mStatsPtr == &aHeader->mStatsBuffer ) )
    {
        /* 유효시간 이상 동작했으면서, 아직 WatchArray에 등록 안돼어있으면
         * 등록한다. */
        aHeader->mStatsPtr = smiTempTable::registWatchArray( aHeader->mStatsPtr );

        if ( sWASegment != NULL )
        {
            sWASegment->mStatsPtr = aHeader->mStatsPtr;
        }
    }
    sStats = aHeader->mStatsPtr;

    sStats->mTTState   = aHeader->mTTState;
    sStats->mTTLastOpr = aOpr;

    if ( sWASegment != NULL )
    {
        sStats->mMaxWorkAreaSize =
            sdtHashModule::getMaxWAPageCount( sWASegment ) * SD_PAGE_SIZE;
        sStats->mUsedWorkAreaSize =
            sdtHashModule::getWASegmentUsedPageCount( sWASegment ) * SD_PAGE_SIZE;
        sStats->mNormalAreaSize = sdtHashModule::getNExtentCount( sWASegment )
            * SDT_WAEXTENT_PAGECOUNT
            * SD_PAGE_SIZE;
    }
    sStats->mRecordLength  = aHeader->mRowSize;
    sStats->mRecordCount   = aHeader->mRowCount;
    sStats->mMergeRunCount = aHeader->mMergeRunCount;
    sStats->mHeight        = aHeader->mHeight;
}

/**************************************************************************
 * Description : Cursor 정보를 출력
 *
 * aTempCursor  - [IN] 대상 Cursor
 * aOutBuf      - [OUT] 정보를 기록할 Buffer
 * aOutSize     - [OUT] Buffer의 크기
 ***************************************************************************/
void smiHashTempTable::dumpTempCursor( smiHashTempCursor * aTempCursor,
                                       SChar             * aOutBuf,
                                       UInt                aOutSize )
{
    while ( aTempCursor != NULL )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "DUMP HASH TEMP CURSOR:\n"
                                   "Flag        : %4"ID_UINT32_FMT"\n"
                                   "Seq         : %"ID_UINT32_FMT"\n"
                                   "InMemory    : %4"ID_UINT32_FMT"\n"
                                   "HashValue   : %"ID_XINT32_FMT"\n"
                                   "ChildPageID : %"ID_UINT32_FMT"\n"
                                   "ChildOffset : %"ID_UINT32_FMT"\n"
                                   "SubHashIdx  : %"ID_UINT32_FMT"\n"
                                   "UpdateEnd   : %"ID_UINT32_FMT"\n"
                                   "LastNPageID : %"ID_UINT32_FMT"\n",
                                   aTempCursor->mTCFlag,
                                   aTempCursor->mSeq,
                                   aTempCursor->mIsInMemory == SDT_WORKAREA_OUT_MEMORY ? "OutMemory" : "InMemory",
                                   aTempCursor->mHashValue,
                                   aTempCursor->mChildPageID,
                                   aTempCursor->mChildOffset,
                                   aTempCursor->mSubHashIdx,
                                   aTempCursor->mUpdateEndOffset,
                                   aTempCursor->mNExtLstPID );

        aTempCursor = aTempCursor->mNext;
    }

    return;
}
