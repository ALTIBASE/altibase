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
 * $Id: smnbValuebaseBuild.cpp 15589 2006-04-18 01:47:21Z leekmo $
 **********************************************************************/

#include <ide.h>
#include <smErrorCode.h>

#include <smc.h>
#include <smm.h>
#include <smnDef.h>
#include <smnManager.h>
#include <smnbValuebaseBuild.h>
#include <sgmManager.h>

extern smiStartupPhase smiGetStartupPhase();

smnbValuebaseBuild::smnbValuebaseBuild() : idtBaseThread()
{

}

smnbValuebaseBuild::~smnbValuebaseBuild()
{

}

/* ------------------------------------------------
 * Description:
 *
 * Row에서 각 key value를 만듦
 * ------------------------------------------------*/
void smnbValuebaseBuild::makeKeyFromRow( smnbHeader   * aIndex,
                                         SChar        * aRow,
                                         SChar        * aKey )
{
    smnbColumn   * sColumn4Build;
    smiColumn    * sColumn;
    SChar        * sKeyPtr;
    SChar        * sVarValuePtr = NULL;

    for( sColumn4Build = &aIndex->columns4Build[0]; // aIndex의 컬럼 개수만큼
         sColumn4Build != aIndex->fence4Build;
         sColumn4Build++ )
    {
        sColumn = &(sColumn4Build->column);

        sKeyPtr = aKey + sColumn4Build->keyColumn.offset;

        // BUG-38573
        // Compressed Column인 경우, FIXED/VARIABLE type에 상관없이
        // FIXED type 처리와 같이 memcpy만 수행한다.
        if( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
              == SMI_COLUMN_COMPRESSION_TRUE )
        {
                idlOS::memcpy( sKeyPtr,
                               aRow + sColumn4Build->column.offset,
                               sColumn4Build->column.size );
        }
        else
        {
            switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_FIXED:
                    idlOS::memcpy( sKeyPtr,
                                   aRow + sColumn4Build->column.offset,
                                   sColumn4Build->column.size );
                    break;
                    
                case SMI_COLUMN_TYPE_VARIABLE:
                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    sVarValuePtr = sgmManager::getVarColumn( aRow, sColumn, sKeyPtr );

                    // variable column의 value가 NULL일 경우, NULL 값을 채움
                    if ( sVarValuePtr == NULL )
                    {
                        sColumn4Build->null( &(sColumn4Build->keyColumn), sKeyPtr );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;
                default:
                    break;
            }
        }

    }
}

/* ------------------------------------------------
 * Description:
 *
 * Run(sort/merge) 할당
 *  - 사용 후 free된 run이 있으면 재사용
 *  - 없으면 새 memory 할당
 *  +-------------+         +-------------+
 *  | mFstFreeRun |-> ... ->| mFstFreeRun |->NULL
 *  +-------------+         +-------------+
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::allocRun( smnbBuildRun ** aRun )
{
    smnbBuildRun   * sCurRun;

    /* BUG-47082 : 메모리 절약과 성능향상을 위해 free run list를 삭제한다. */

    /* smnbValuebaseBuild_allocRun_alloc_CurRun.tc */
    IDU_FIT_POINT("smnbValuebaseBuild::allocRun::alloc::CurRun");
    IDE_TEST( mRunPool.alloc( (void**)&sCurRun )
              != IDE_SUCCESS );

    // run initialize
    sCurRun->mSlotCount = 0;
    sCurRun->mNext = NULL;

    *aRun = sCurRun;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description:
 *
 * Run(sort/merge) free
 *  - freeRun list에 free될 run을 연결한다.
 * ------------------------------------------------*/
void smnbValuebaseBuild::freeRun( smnbBuildRun    * aRun )
{
    mRunPool.memfree( aRun );
}

/* ------------------------------------------------
 * Description:
 *
 * Run에서 key value의 offset swap
 * ----------------------------------------------*/
void smnbValuebaseBuild::swapOffset( UShort  * aOffset1,
                                     UShort  * aOffset2 )
{
    UShort sTmpOffset;

    sTmpOffset = *aOffset1;
    *aOffset1  = *aOffset2;
    *aOffset2  = sTmpOffset;
}

/* ------------------------------------------------
 * Description:
 *
 * thread 작업 시작 루틴
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::threadRun( UInt                  aPhase,
                                      UInt                  aThreadCnt,
                                      smnbValuebaseBuild  * aThreads )
{
    UInt             i;

    // Working Thread 실행
    for( i = 0; i < aThreadCnt; i++ )
    {
        aThreads[i].mPhase = aPhase;

        IDE_TEST( aThreads[i].start( ) != IDE_SUCCESS );
    }

    // Working Thread 종료 대기
    for( i = 0; i < aThreadCnt; i++ )
    {
        IDE_TEST( aThreads[i].join() != IDE_SUCCESS );
    }

    // Working Thread 수행 결과 확인
    for( i = 0; i < aThreadCnt; i++ )
    {
        if( aThreads[i].mIsSuccess == ID_FALSE )
        {
            IDE_TEST_RAISE( aThreads[i].mErrorCode != 0,
                            err_thread_join );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_thread_join );
    {
        ideCopyErrorInfo( ideGetErrorMgr(),
                          aThreads[i].getErrorMgr() );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Memory Index Build Main Routine
 *
 * 1. Parallel Key Extraction & In-Memory Sort
 * 2. Parallel Merge & Union Merge Runs
 * 3. Make Index Tree
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::buildIndex( void              * aTrans,
                                       idvSQL            * aStatistics,
                                       smcTableHeader    * aTable,
                                       smnIndexHeader    * aIndex,
                                       UInt                aThreadCnt,
                                       idBool              aIsNeedValidation,
                                       UInt                aKeySize,
                                       UInt                aRunSize,
                                       smnGetPageFunc      aGetPageFunc,
                                       smnGetRowFunc       aGetRowFunc )
{
    smnbValuebaseBuild  * sThreads = NULL;
    smnbStatistic         sIndexStat;
    UInt                  i         = 0;
    UInt                  sState    = 0;
    UInt                  sThrState = 0;
    UInt                  sMaxUnionRunCnt;
    UInt                  sIsPers;
    iduMemPool            sRunPool;
    smnIndexModule      * sIndexModules;
    smnbIntStack          sStack;
    idBool                sIsIndexBuilt = ID_FALSE;
    idBool                sIsPrivateVol;

    sStack.mDepth = -1;

    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aThreadCnt > 0 );

    sIndexModules = aIndex->mModule;

    sIsPers = aIndex->mFlag & SMI_INDEX_PERSISTENT_MASK;

    // persistent index는 항상 pointer base build로 수행됨.
    IDE_ERROR( sIsPers != SMI_INDEX_PERSISTENT_ENABLE );

    /* BUG-47082
       THREAD 갯수의 제곱근 값으로 Union Run 갯수를 설정한다.
       ex) THREAD 16개 => Union Run 4개, THREAD 25개 => Union Run 5개 */
    sMaxUnionRunCnt = (UInt)idlOS::sqrt( (double)aThreadCnt );

    /* PROJ-1407 Temporary Table
     * Temp table index일 경우 trace log를 기록하지 않는다. */
    sIsPrivateVol = (( aTable->mFlag & SMI_TABLE_PRIVATE_VOLATILE_MASK )
                     == SMI_TABLE_PRIVATE_VOLATILE_TRUE ) ? ID_TRUE : ID_FALSE ;

    IDE_TEST( sRunPool.initialize( IDU_MEM_SM_SMN,
                                   (SChar*)"SMNB_BUILD_RUN_POOL",
                                   2,
                                   aRunSize,
                                   128,
                                   IDU_AUTOFREE_CHUNK_LIMIT,//chunk limit
                                   ID_TRUE,                 //use mutex
                                   IDU_MEM_POOL_DEFAULT_ALIGN_SIZE, //align
                                   ID_FALSE,               // force pooling
                                   ID_TRUE,                /* garbage collection */
                                   ID_TRUE,                /* HWCacheLine */
                                   IDU_MEMPOOL_TYPE_LEGACY /* mempool type */ ) 
              != IDE_SUCCESS);			
    sState = 1;

    //TC/FIT/Limit/sm/smn/smnb/smnbValuebaseBuild_buildIndex_malloc.sql
    IDU_FIT_POINT_RAISE( "smnbValuebaseBuild::buildIndex::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                (ULong)ID_SIZEOF(smnbValuebaseBuild) * aThreadCnt,
                                (void**)&sThreads,
                                IDU_MEM_IMMEDIATE ) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 2;

    if( sIsPrivateVol == ID_FALSE )
    {
        ideLog::log( IDE_SM_0, "\
=========================================\n\
 [MEM_IDX_CRE] 0. BUILD INDEX (Value)    \n\
                  NAME : %s              \n\
                  ID   : %u              \n\
=========================================\n\
    1. Extract & In-Memory Sort          \n\
    2. Merge & Union Merge               \n\
    3. Build Tree                        \n\
=========================================\n\
          BUILD_THREAD_COUNT     : %u    \n\
          SORT_RUN_SIZE          : %u    \n\
          UNION_MERGE_RUN_CNT    : %u    \n\
          KEY_SIZE               : %u    \n\
=========================================\n",
                     aIndex->mName,
                     aIndex->mId,
                     aThreadCnt,
                     aRunSize,
                     sMaxUnionRunCnt,
                     aKeySize );
    }

    for(i = 0; i < aThreadCnt; i++)
    {
        new (sThreads + i) smnbValuebaseBuild;

        IDE_TEST(sThreads[i].initialize( aTrans,
                                         aTable,
                                         aIndex,
                                         aThreadCnt,
                                         i,
                                         aIsNeedValidation,
                                         aRunSize,
                                         sRunPool,
                                         sMaxUnionRunCnt,
                                         aKeySize,
                                         aStatistics,
                                         aGetPageFunc,
                                         aGetRowFunc )
                 != IDE_SUCCESS);

        // BUG-30426 [SM] Memory Index 생성 실패시 thread를 destroy 해야 합니다.
        sThrState++;
    }

// ----------------------------------------
// Phase 1. Extract & Sort & Merge Run
// ----------------------------------------
    if( sIsPrivateVol == ID_FALSE )
    {
        ideLog::log( IDE_SM_0, "\
============================================\n\
 [MEM_IDX_CRE] 1. Extract & In-Memory Sort & Merge\n\
============================================\n");
    }

    IDE_TEST( threadRun( SMN_EXTRACT_AND_SORT,
                         aThreadCnt,
                         sThreads )
              != IDE_SUCCESS );

// ----------------------------------------
// Phase 2. Union Merge Run
// ----------------------------------------
    if( (aThreadCnt / sMaxUnionRunCnt) >= 2 )
    {
        if( sIsPrivateVol == ID_FALSE )
        {
            ideLog::log( IDE_SM_0, "\
============================================\n\
 [MEM_IDX_CRE] 2. Union Merge Run         \n\
============================================\n" );
        }

        for( i = 0; i < aThreadCnt; i++ )
        {
            sThreads[i].mThreads = sThreads;
        }
        IDE_TEST( threadRun( SMN_UNION_MERGE,
                             aThreadCnt,
                             sThreads )
                  != IDE_SUCCESS );
    }


// ------------------------------------------------
// Phase 3. Make Tree
// ------------------------------------------------
    if( sIsPrivateVol == ID_FALSE )
    {
        ideLog::log( IDE_SM_0, "\
============================================\n\
 [MEM_IDX_CRE] 3. Make Tree                 \n\
============================================\n" );
    }

    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF( smnbStatistic ) );

    IDE_TEST( sThreads[0].makeTree( sThreads,
                                    &sIndexStat,
                                    aThreadCnt,
                                    &sStack ) != IDE_SUCCESS );
    sIsIndexBuilt = ID_TRUE;


    /* BUG-39681 Do not need statistic information when server start */
    if( smiGetStartupPhase() == SMI_STARTUP_SERVICE )
    {
        // index build에 의한 통계치 누적
        SMNB_ADD_STATISTIC( &(((smnbHeader*)aIndex->mHeader)->mStmtStat),
                            &sIndexStat );
    }


    // BUG-30426 [SM] Memory Index 생성 실패시 thread를 destroy 해야 합니다.
    while( sThrState > 0 )
    {
        IDE_TEST( sThreads[--sThrState].destroy() != IDE_SUCCESS);
    }
    sState = 1;
    IDE_TEST( iduMemMgr::free( sThreads ) != IDE_SUCCESS);
    sThreads = NULL;

    sState = 0;
    IDE_TEST( sRunPool.destroy( ID_FALSE ) != IDE_SUCCESS );

    if( ((smnbHeader*)aIndex->mHeader)->root != NULL )
    {
        ideLog::log( IDE_SM_0, "\
========================================\n\
[MEM_IDX_CRE] 4. Gather Stat           \n\
========================================\n" );

        /* BUG-39681 Do not need statistic information when server start */
        if( smiGetStartupPhase() == SMI_STARTUP_SERVICE )
        {
            IDE_TEST( smnbBTree::gatherStat( aStatistics,
                                             NULL, /* aTrans - nologging */
                                             1.0f, /* 100% */
                                             aThreadCnt,
                                             aTable,
                                             NULL, /*TotalTableArg*/
                                             aIndex,
                                             NULL,
                                             ID_FALSE )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* 데이터가 전혀 없는 상태면, 통계를 Invalid상태로 둠. */
        ideLog::log( IDE_SM_0, "\
========================================\n\
[MEM_IDX_CRE] 4. Gather Stat(SKIP)     \n\
========================================\n" );
    }

    if( sIsPrivateVol == ID_FALSE )
    {
        ideLog::log( IDE_SM_0, "\
============================================\n\
 [MEM_IDX_CRE] Build Completed              \n\
============================================\n" );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    
    IDE_PUSH();

    // BUG-30426 [SM] Memory Index 생성 실패시 thread를 destroy 해야 합니다.
    for(i = 0; i < sThrState; i++)
    {
        (void)sThreads[i].destroy();
    }

    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free(sThreads);
            sThreads = NULL;
        case 1:
            (void)sRunPool.destroy( ID_FALSE );
            break;
    }
    if( sIsIndexBuilt == ID_TRUE )
    {
        (void)(sIndexModules)->mFreeAllNodeList( aStatistics,
                                                 aIndex,
                                                 aTrans );
    }
        
    IDE_POP();
    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * Index build 쓰레드 초기화
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::initialize( void              * aTrans,
                                       smcTableHeader    * aTable,
                                       smnIndexHeader    * aIndex,
                                       UInt                aThreadCnt,
                                       UInt                aMyTid,
                                       idBool              aIsNeedValidation,
                                       UInt                aRunSize,
                                       iduMemPool          aRunPool,
                                       UInt                aMaxUnionRunCnt,
                                       UInt                aKeySize,
                                       idvSQL            * aStatistics,
                                       smnGetPageFunc      aGetPageFunc,
                                       smnGetRowFunc       aGetRowFunc )
{
    UInt       sAvailableRunSize;
    UInt       sKeyNOffsetSize;

    mErrorCode         = 0;
    mTable             = aTable;
    mIndex             = aIndex;
    mThreadCnt         = aThreadCnt;
    mMyTID             = aMyTid;
    mIsNeedValidation  = aIsNeedValidation;
    mStatistics        = aStatistics;
    mTrans             = aTrans;
    
    mSortedRunCount    = 0;
    mMergedRunCount    = 0;
    mMaxUnionRunCnt    = aMaxUnionRunCnt;

    mFstRun            = NULL;
    mLstRun            = NULL;

    mIsSuccess         = ID_TRUE;

    mRunPool           = aRunPool;

    mKeySize           = aKeySize;

    sAvailableRunSize  = aRunSize - ID_SIZEOF(smnbBuildRunHdr);
    sKeyNOffsetSize    = aKeySize + ID_SIZEOF(UShort);

    // sort run에 채워질 최대 key 수
    // sort run의 key contents : key value, row pointer, offset
    mMaxSortRunKeyCnt  = sAvailableRunSize / sKeyNOffsetSize;

    // merge run에 채워질 최대 key 수
    // merge run의 key contents : key value, row pointer
    mMaxMergeRunKeyCnt = sAvailableRunSize / aKeySize;

    /* BUG-42152 when adding column with default value on memory partitioned table,
     * local unique constraint is not validated.
     */
    if ( ( ( aIndex->mFlag & SMI_INDEX_UNIQUE_MASK )
           == SMI_INDEX_UNIQUE_ENABLE ) ||
         ( ( aIndex->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK )
           == SMI_INDEX_LOCAL_UNIQUE_ENABLE ) )
    {
        mIsUnique = ID_TRUE;
    }
    else
    {
        mIsUnique = ID_FALSE;
    }

    mGetPageFunc     = aGetPageFunc;
    mGetRowFunc      = aGetRowFunc;

    IDE_TEST(mSortStack.initialize(IDU_MEM_SM_SMN, ID_SIZEOF(smnSortStack))
         != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void)mSortStack.destroy();

    return IDE_FAILURE;
}

/* BUG-27403 쓰레드 정리 */
IDE_RC smnbValuebaseBuild::destroy()
{
    IDE_TEST(mSortStack.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * 쓰레드 메인 실행 루틴
 * ----------------------------------------------*/
void smnbValuebaseBuild::run()
{
    smnbStatistic      sIndexStat;

    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF( smnbStatistic ) );

    switch( mPhase )
    {
        case SMN_EXTRACT_AND_SORT:

            /* BUG-47082
               아래와 같이 변경하였습니다.

               기존에는
               1. 쓰레드 각각 "record extract"와 "run 내부 quick sort" 까지 수행
               2. 모든 쓰레드가 끝날때까지 join 해서 기다림.
               3. 쓰레드 각각 자신이 갖고 있는 run을 "일차적으로 merge" 를 수행
               2. 모든 쓰레드가 끝날때까지 join 해서 기다림.
               
               수정이후는
               1. 쓰레드 각각 "record extract"와 "run 내부 quick sort"와
                  자신이 갖고 있는 run을 "일차적으로 merge" 를 수행
               2. 모든 쓰레드가 끝날때까지 join 해서 기다림.
            */

            /* Phase 1. Parallel Key Extraction & Sort */
            IDE_TEST( extractNSort( &sIndexStat ) != IDE_SUCCESS );
            /* Phase 2. Parallel Merge */
            IDE_TEST( mergeRun( &sIndexStat ) != IDE_SUCCESS );
            break;

        case SMN_UNION_MERGE:
            // Phase 2-1. Parallel Union Merge Run
            IDE_TEST( unionMergeRun( &sIndexStat ) != IDE_SUCCESS );

            break;

        default:
            IDE_ASSERT(0);
    }
    
    /* BUG-39681 Do not need statistic information when server start */
    if( smiGetStartupPhase() == SMI_STARTUP_SERVICE )
    {
        SMNB_ADD_STATISTIC( &(((smnbHeader*)mIndex->mHeader)->mStmtStat),
                            &sIndexStat );
    }

    return;

    IDE_EXCEPTION_END;

    mErrorCode = ideGetErrorCode();
    ideCopyErrorInfo( &mErrorMgr, ideGetErrorMgr() );

    /* BUG-47581 build memory index 에서 예외 발생 상황에 대한 정보 보강.
     * 1. Null, Unique 등은 유저 실수로 발생 할 수 있다.
     *    Rebuild indexes 에서만 확인한다.
     * 2. Error Code 가 0일 경우 상위에서 인지하지 못하므로 여기에서 출력한다.*/
    if (( smiGetStartupPhase() != SMI_STARTUP_SERVICE ) &&
        ( mErrorCode == 0 ))
    {
        ideLog::log( IDE_ERR_0,
                     "Index creation failed (ErrorCode=0)\n"
                     "Phase: %"ID_UINT32_FMT", "
                     "SpaceID: %"ID_UINT32_FMT", "
                     "TableOID: %"ID_vULONG_FMT", "
                     "IndexID: %"ID_UINT32_FMT", NAME: %s\n",
                     mPhase,
                     mTable->mSpaceID,
                     mTable->mSelfOID,
                     mIndex->mId,
                     mIndex->mName );
    }
    IDL_MEM_BARRIER;
}

IDE_RC smnbValuebaseBuild::isRowUnique( smnbStatistic       * aIndexStat,
                                        smnbHeader          * aIndexHeader,
                                        smnbKeyInfo         * aKeyInfo1,
                                        smnbKeyInfo         * aKeyInfo2,
                                        SInt                * aResult )
{
    SInt              sResult;
    idBool            sIsEqual = ID_FALSE;
    smpSlotHeader   * sRow1;
    smpSlotHeader   * sRow2;

    IDE_ASSERT( aIndexHeader != NULL );
    IDE_ASSERT( aKeyInfo1    != NULL );
    IDE_ASSERT( aKeyInfo2    != NULL );
    IDE_ASSERT( aResult      != NULL );

    *aResult = 0;

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)aKeyInfo1->keyValue,
                           (SChar*)aKeyInfo2->keyValue,
                           aKeyInfo1->rowPtr,
                           aKeyInfo2->rowPtr,
                           &sIsEqual);

    // uniqueness check
    if( (mIsUnique == ID_TRUE) && (sIsEqual == ID_TRUE) )
    {
        sRow1 = (smpSlotHeader*)aKeyInfo1->rowPtr;
        sRow2 = (smpSlotHeader*)aKeyInfo2->rowPtr;

        /* BUG-32926 [SM] when server restart, Prepared row
         * in the rebuilding memory index process should
         * be read.
         *
         * restart index rebuild시 prepare된 row가 존재하는 경우
         * old version과 new version의 key value가 같은 경우가
         * 존재할 수 있다.
         * 이 경우 동일 value라도 문제없다. */
        if ( mIsNeedValidation == ID_FALSE )
        {
            IDE_TEST_RAISE(
                ( SMP_SLOT_HAS_NULL_NEXT_OID(sRow1) &&
                  SMP_SLOT_HAS_NULL_NEXT_OID(sRow2) ) &&
                ( (aIndexHeader->mIsNotNull == ID_TRUE) ||
                  (isNull(aIndexHeader, aKeyInfo1->keyValue) != ID_TRUE) ),
                ERR_UNIQUENESS_VIOLATION );
        }
        else
        {
            IDE_TEST_RAISE( (aIndexHeader->mIsNotNull == ID_TRUE) ||
                            (isNull(aIndexHeader,
                                    aKeyInfo1->keyValue)
                             != ID_TRUE),
                            ERR_UNIQUENESS_VIOLATION );
        }
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNIQUENESS_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smnUniqueViolation ) );

        /* BUG-42169 서버 재시작으로 인한 index rebuild 과정에서 uniqueness가 깨질경우
         * 디버그를 위한 추가 정보를 출력해야 한다. */
        if ( smiGetStartupPhase() != SMI_STARTUP_SERVICE )
        {
            /* sm trc log에는 header 및 문제가 발생한 slot의 정보를 남긴다. */
            ideLog::log( IDE_SM_0, "Unique Violation Error Occurs In Restart Rebuild Index" );
            smnManager::logCommonHeader( mIndex );
            (void)smpFixedPageList::dumpSlotHeader( sRow1 );
            (void)smpFixedPageList::dumpSlotHeader( sRow2 );

            /* error trc log에는 문제가 발생한 인덱스의 id, name을 남긴다. */
            ideLog::log( IDE_ERR_0, "Unique Violation Error Occurs In Restart Rebuild Index\n"
                         "IndexName   : %s\n"
                         "IndexOID    : %"ID_UINT32_FMT,
                         mIndex->mName,
                         mIndex->mId ); 
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
                                      
/* ---------------------------------------------------------
 * Description:
 *
 * - thread에 할당된 page들로부터 row pointer를 추출하여
 *   key value를 생성해서 sort run에 저장.
 * - sort run이 가득 차면 quick sort로 정렬한 후 정렬된
 *   순서대로 merge run에 <row ptr, key value>쌍으로 저장.
 * ---------------------------------------------------------*/
IDE_RC smnbValuebaseBuild::extractNSort( smnbStatistic    * aIndexStat )
{
    smnbHeader        * sHeader;
    smnbBuildRun      * sSortRun   = NULL;
    smnbKeyInfo       * sKeyInfo;
    smnbColumn        * sColumn;
    UShort            * sKeyOffset;
    scPageID            sPageID    = SM_NULL_PID;
    UInt                sPageSeq   = 0;
    SChar             * sFence;
    SChar             * sRow       = NULL;
    idBool              sLocked    = ID_FALSE;
    UInt                sSlotCount = 0;
    UInt                sState     = 0;

    sHeader = (smnbHeader*)mIndex->mHeader;

    // sort run을 할당 받음
    IDE_TEST( allocRun( &sSortRun ) != IDE_SUCCESS );
    sState = 1;

    // sort run에서 key offset 할당
    sKeyOffset = (UShort*)(&sSortRun->mBody[mMaxSortRunKeyCnt * mKeySize]);

    while( 1 )
    {
        /* 함수 내부에서 Lock 풀고 다시 잡아줌 */
        IDE_TEST( mGetPageFunc( mTable, &sPageID, &sLocked ) != IDE_SUCCESS );
        sState = 2;

        if( sPageID == SM_NULL_PID )
        {
            break;
        }

        if( ((sPageSeq++) % mThreadCnt) != mMyTID )
        {
            continue;
        }

        sRow = NULL;

        while( 1 )
        {
            IDE_TEST( mGetRowFunc( mTable,
                                   sPageID,
                                   &sFence,
                                   &sRow,
                                   mIsNeedValidation ) != IDE_SUCCESS );
            sState = 3;

            if( sRow == NULL )
            {
                break;
            }

            // mMaxSortRunKeyCnt : sort run에 저장할 수 있는 key의 수
            if( sSlotCount == mMaxSortRunKeyCnt )
            {
                IDE_TEST( quickSort( aIndexStat,
                                     sSortRun->mBody,
                                     0,
                                     sSlotCount - 1 )
                          != IDE_SUCCESS );
                sState = 4;

                IDE_TEST( moveSortedRun( sSortRun,
                                         sSlotCount,
                                         sKeyOffset )
                          != IDE_SUCCESS );
                sState = 5;

                sSlotCount = 0;

                IDE_TEST( iduCheckSessionEvent( mStatistics )
                          != IDE_SUCCESS );
            }
            sState = 6;

            // sort run에서 key(row pointer + key value) 공간 할당
            sKeyInfo = (smnbKeyInfo*)(&sSortRun->mBody[sSlotCount * mKeySize]);

            sKeyInfo->rowPtr = sRow;               // row pointer
            sKeyOffset[sSlotCount] = sSlotCount;   // offset

            // row pointer로부터 key value 생성 -> sKeyInfo->keyValue
            makeKeyFromRow( sHeader,
                            sRow,
                            (SChar*)sKeyInfo->keyValue );

            IDU_FIT_POINT_RAISE( "BUG-47581@smnbValuebaseBuild::extractNSort",
                                 ERR_NOTNULL_VIOLATION );

            // check null violation
            if( sHeader->mIsNotNull == ID_TRUE )
            {
                // 하나라도 Null이면 안되기 때문에
                // 모두 Null인 경우만 체크하는 isNullKey를 사용하면 안된다
                for( sColumn = &sHeader->columns4Build[0];
                     sColumn < sHeader->fence4Build;
                     sColumn++)
                {
                    IDE_TEST_RAISE(( smnbBTree::isNullColumn( sColumn,
                                                              &(sColumn->keyColumn),
                                                              sKeyInfo->keyValue ) == ID_TRUE ),
                                   ERR_NOTNULL_VIOLATION );
                }
            }

            sState = 7;

            sSlotCount++;
        }
    }

    // 마지막 row를 extract하고 그 sort run을 정렬한 후 merge run으로 copy
    if( sSlotCount != 0 )
    {
        IDE_TEST( quickSort( aIndexStat,
                             sSortRun->mBody,
                             0,
                             sSlotCount - 1 )
                  != IDE_SUCCESS );
        sState = 8;

        IDE_TEST( moveSortedRun( sSortRun,
                                 sSlotCount,
                                 sKeyOffset )
                  != IDE_SUCCESS );
        sState = 9;
    }

    // sort run free시킴
    freeRun( sSortRun );

    return IDE_SUCCESS;

    // PRIMARY KEY는 NULL값을 색인할수 없다
    IDE_EXCEPTION( ERR_NOTNULL_VIOLATION );
    {
        if ( smiGetStartupPhase() != SMI_STARTUP_SERVICE )
        {
            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)sKeyInfo->keyValue,
                            mKeySize,
                            "[ Hex Dump Key Value In smnbValuebaseBuild::extractNSort() ]\n"
                            "Not Null Column Idx: %"ID_UINT32_FMT"\n",
                            ((UChar*)sColumn - (UChar*)&sHeader->columns4Build[0])/
                            ID_SIZEOF(smnbColumn) + 1 );

            for( sColumn = &sHeader->columns4Build[0];
                 sColumn < sHeader->fence4Build;
                 sColumn++)
            {
                ideLog::logMem( IDE_DUMP_0,
                                (UChar*)sColumn,
                                ID_SIZEOF(smnbColumn),
                                "[ Hex Dump Mem Column Info ]\n" );
            }
        }

        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_NULL_VIOLATION ) );
    }
    IDE_EXCEPTION_END;

    if ( smiGetStartupPhase() != SMI_STARTUP_SERVICE )
    {
        // BUG-47581 build memory index 에서 예외 발생 상황에 대한 정보 보강.
        ideLog::log( IDE_ERR_0,
                     "Index creation failed (smnbValuebaseBuild::extractNSort())\n"
                     "State: %"ID_UINT32_FMT", "
                     "SpaceID: %"ID_UINT32_FMT", "
                     "TableOID: %"ID_vULONG_FMT", "
                     "IndexID: %"ID_UINT32_FMT", "
                     "PageID: %"ID_UINT32_FMT", "
                     "SlotCount: %"ID_UINT32_FMT", "
                     "RowPtr: %"ID_xPOINTER_FMT", "
                     "RowSize: %"ID_UINT32_FMT"\n",
                     sState,
                     mTable->mSpaceID,
                     mTable->mSelfOID,
                     mIndex->mId,
                     sPageID,
                     sSlotCount,
                     sRow,
                     mTable->mFixed.mMRDB.mSlotSize );

        if ( sRow != NULL )
        {
            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)sRow,
                            mTable->mFixed.mMRDB.mSlotSize,
                            "[ Hex Dump Mem Row In smnbValuebaseBuild::extractNSort() ]\n" );
        }

        if ( sPageID != SM_NULL_PID )
        {
            SChar * sPagePtr;
            IDE_ASSERT( smmManager::getPersPagePtr( mTable->mSpaceID,
                                                    sPageID,
                                                    (void**)&sPagePtr )
                        == IDE_SUCCESS );

            ideLog::logMem( IDE_DUMP_0,
                            (UChar*)sPagePtr,
                            SM_PAGE_SIZE,
                            "[ Hex Dump Mem Page In smnbValuebaseBuild::extractNSort() ]\n" );
        }
    }

    if( sLocked == ID_TRUE )
    {
        // null일 경우 latch가 잡힌 page의 latch를 해제
        (void)smnManager::releasePageLatch( mTable, sPageID );
        sLocked = ID_FALSE;
    }

    mIsSuccess = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC smnbValuebaseBuild::moveSortedRun( smnbBuildRun   * aSortRun,
                                          UInt             aSlotCount,
                                          UShort         * aKeyOffset )
{
    smnbBuildRun   * sMergeRun = NULL;
    UInt             i;

    IDE_TEST( allocRun( &sMergeRun ) != IDE_SUCCESS );

    // sort run의 key들을 merge run으로 copy
    for( i = 0; i < aSlotCount; i++ )
    {
        idlOS::memcpy( &sMergeRun->mBody[i * mKeySize],
                       &aSortRun->mBody[aKeyOffset[i] * mKeySize],
                       mKeySize );
    }

    if( mFstRun == NULL )
    {
        mFstRun = mLstRun = sMergeRun;
    }
    else
    {
        mLstRun->mNext = sMergeRun;
        mLstRun        = sMergeRun;
    }

    sMergeRun->mSlotCount = aSlotCount;
    mSortedRunCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbValuebaseBuild::getPivot             *
 * ------------------------------------------------------------------*
 * BUG-47082
 *
 * QUICK SORT 에서 사용할 PIVOT 을 구해서 리턴한다.
 * 최악을 피하기 위해, 3개의 값(lo, mid, hi) 중 중간값을 선택한다.
 *
 *********************************************************************/
SInt smnbValuebaseBuild::getPivot( smnbStatistic * aIndexStat,
                                   smnbHeader    * aIndexHeader,
                                   SChar         * aBuffer,
                                   UShort        * aKeyOffset,
                                   SInt            aLeftPos,
                                   SInt            aRightPos )
{
    SInt   sLo;
    SInt   sMid;
    SInt   sHi;
    SInt   sTmp;
    idBool sDummy;

    smnbKeyInfo  * sKeyInfo1;
    smnbKeyInfo  * sKeyInfo2;

    SInt sResult;

    if ( aRightPos - aLeftPos < 2 )
    {
        /* 2개 이하면 */
        sHi = aLeftPos;
        IDE_CONT( PIVOT_RETURN );
    }
    else if ( aRightPos - aLeftPos >= 999 )
    {
        /* 1000개 이상이면 */
        sHi = getPivot4Large( aIndexStat,
                              aIndexHeader,
                              aBuffer,
                              aKeyOffset,
                              aLeftPos,
                              aRightPos );
        IDE_CONT( PIVOT_RETURN );
    }

    sLo  = aLeftPos;
    sMid = (aLeftPos + aRightPos ) / 2;
    sHi  = aRightPos;

    /* sLo, sMid, sHi 중에 중간값을 찾는다. */

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[sMid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[sLo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( sMid < sLo ) */
    if ( sResult < 0 )
    {
        sTmp = sMid;
        sMid = sLo;
        sLo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[sHi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[sLo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( sHi < sLo ) */
    if ( sResult < 0 )
    {
        sTmp = sHi;
        sHi  = sLo;
        sLo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[sMid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[sHi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( sMid < sHi ) */
    if ( sResult < 0 )
    {
        sTmp = sMid;
        sMid = sHi;
        sHi  = sTmp;
    }

    IDE_EXCEPTION_CONT( PIVOT_RETURN );

    return sHi;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbValuebaseBuild::getPivot4Large       *
 * ------------------------------------------------------------------*
 * BUG-47082
 *
 * 인덱스키 갯수가 1000개이상인 경우
 * QUICK SORT 에서 사용할 PIVOT 을 구해서 리턴한다.
 *
 * 9개의 값 중 중간값을 선택한다.
 *   
 * pivot = median( median(s1Lo, s1Mid, s1Hi),
 *                 median(s2Lo, s2Mid, s2Hi),
 *                 median(s3Lo, s3Mid, s3Hi) )
 *
 *********************************************************************/
SInt smnbValuebaseBuild::getPivot4Large( smnbStatistic * aIndexStat,
                                         smnbHeader    * aIndexHeader,
                                         SChar         * aBuffer,
                                         UShort        * aKeyOffset,
                                         SInt            aLeftPos,
                                         SInt            aRightPos )
{
    SInt   s1Lo;
    SInt   s1Mid;
    SInt   s1Hi;
    SInt   s2Lo;
    SInt   s2Mid;
    SInt   s2Hi;
    SInt   s3Lo;
    SInt   s3Mid;
    SInt   s3Hi;

    SInt   sTmp;
    SInt   sInc;
    idBool sDummy;

    smnbKeyInfo  * sKeyInfo1;
    smnbKeyInfo  * sKeyInfo2;

    SInt sResult;

    sInc = ( aRightPos - aLeftPos ) / 8;

    s1Lo  = aLeftPos;
    s1Mid = aLeftPos + ( 1 * sInc );
    s1Hi  = aLeftPos + ( 2 * sInc );

    s2Lo  = aLeftPos + ( 3 * sInc );
    s2Mid = aLeftPos + ( 4 * sInc );
    s2Hi  = aLeftPos + ( 5 * sInc );

    s3Lo  = aLeftPos + ( 6 * sInc );
    s3Mid = aLeftPos + ( 7 * sInc );
    s3Hi  = aRightPos;

    /************************************
     * s1Hi = median(s1Lo, s1Mid, s1Hi)
     ************************************/

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Mid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Lo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s1Mid < s1Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s1Mid;
        s1Mid = s1Lo;
        s1Lo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Hi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Lo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s1Hi < s1Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s1Hi;
        s1Hi  = s1Lo;
        s1Lo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Mid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Hi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s1Mid < s1Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s1Mid;
        s1Mid = s1Hi;
        s1Hi  = sTmp;
    }

    /************************************
     * s2Hi = median(s2Lo, s2Mid, s2Hi)
     ************************************/

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Mid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Lo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s2Mid < s2Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s2Mid;
        s2Mid = s2Lo;
        s2Lo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Hi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Lo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s2Hi < s2Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s2Hi;
        s2Hi  = s2Lo;
        s2Lo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Mid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Hi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s2Mid < s2Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s2Mid;
        s2Mid = s2Hi;
        s2Hi  = sTmp;
    }

    /************************************
     * s3Hi = median(s3Lo, s3Mid, s3Hi)
     ************************************/

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Mid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Lo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue, 
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s3Mid < s3Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s3Mid;
        s3Mid = s3Lo;
        s3Lo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Hi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Lo]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s3Hi < s3Lo ) */
    if ( sResult < 0 )
    {
        sTmp = s3Hi;
        s3Hi  = s3Lo;
        s3Lo  = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Mid]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Hi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s3Mid < s3Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s3Mid;
        s3Mid = s3Hi;
        s3Hi  = sTmp;
    }

    /************************************
     * s3Hi = median(s1Hi, s2Hi, s3Hi)
     ************************************/

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Hi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Hi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s2Hi < s1Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s2Hi;
        s2Hi = s1Hi;
        s1Hi = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Hi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s1Hi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s3Hi < s1Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s3Hi;
        s3Hi = s1Hi;
        s1Hi = sTmp;
    }

    sKeyInfo1 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s2Hi]];
    sKeyInfo2 = (smnbKeyInfo *)&aBuffer[mKeySize * aKeyOffset[s3Hi]];

    sResult = compareKeys( aIndexStat,
                           aIndexHeader->columns4Build,
                           aIndexHeader->fence4Build,
                           (SChar*)sKeyInfo1->keyValue,
                           (SChar*)sKeyInfo2->keyValue,
                           sKeyInfo1->rowPtr,
                           sKeyInfo2->rowPtr,
                           &sDummy );
    /* ( s2Hi < s3Hi ) */
    if ( sResult < 0 )
    {
        sTmp = s2Hi;
        s2Hi = s3Hi;
        s3Hi = sTmp;
    }

    return s3Hi;
}

/* ------------------------------------------------
 * quick sort
 *  - sort run의 offset을 이용하여 해당 위치의
 *    key value를 비교하여 offset을 swap
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::quickSort( smnbStatistic  * aIndexStat,
                                      SChar          * aBuffer,
                                      SInt             aHead,
                                      SInt             aTail )
{
    smnbHeader*    sIndexHeader;
    SInt           sResult;
    SInt           i = 0;
    SInt           j = 0;
    SInt           k = 0;
    UShort       * sKeyOffset;
    smnbKeyInfo  * sKeyInfo1;
    smnbKeyInfo  * sKeyInfo2;
    smnSortStack   sCurStack; // fix BUG-27403 -for Stack overflow
    smnSortStack   sNewStack; 
    idBool         sEmpty;

    sIndexHeader = (smnbHeader*)(mIndex->mHeader);
    sKeyOffset   = (UShort*)&(aBuffer[mKeySize * mMaxSortRunKeyCnt]);

    //fix BUG-27403 최초 해야할 일 입력
    sCurStack.mLeftPos   = aHead;
    sCurStack.mRightPos  = aTail;
    IDE_TEST( mSortStack.push(ID_FALSE, &sCurStack) != IDE_SUCCESS );

    sEmpty = ID_FALSE;

    while( 1 )
    {
        IDE_TEST(mSortStack.pop(ID_FALSE, &sCurStack, &sEmpty)
                 != IDE_SUCCESS);

        // Bug-27403
        // QuickSort의 알고리즘상, CallStack은 ItemCount보다 많아질 수 없다.
        // 따라서 이보다 많으면, 무한루프에 빠졌을 가능성이 높다.
        IDE_ERROR( (aTail - aHead + 1) >= 0 );
        IDE_ERROR( (UInt)(aTail - aHead + 1) > mSortStack.getTotItemCnt() );

        if( sEmpty == ID_TRUE)
        {
            break;
        }

        i = sCurStack.mLeftPos;
        j = sCurStack.mRightPos + 1;
        /* BUG-47082 */
        k = getPivot( aIndexStat,
                      sIndexHeader,
                      aBuffer,
                      sKeyOffset,
                      sCurStack.mLeftPos,
                      sCurStack.mRightPos );

        if( sCurStack.mLeftPos < sCurStack.mRightPos )
        {
            swapOffset( &sKeyOffset[k], &sKeyOffset[ sCurStack.mLeftPos ] );
    
            while( 1 )
            {
                while( (++i) <= sCurStack.mRightPos )
                {
                    sKeyInfo1 = 
                        (smnbKeyInfo*)&aBuffer[mKeySize * sKeyOffset[i]];
                    sKeyInfo2 = 
                        (smnbKeyInfo*)&aBuffer[mKeySize * sKeyOffset[sCurStack.mLeftPos]];

                    IDE_TEST( isRowUnique( aIndexStat,
                                           sIndexHeader,
                                           sKeyInfo1,
                                           sKeyInfo2,
                                           &sResult )
                              != IDE_SUCCESS );

                    if( sResult > 0)
                    {
                        break;
                    }
                }
    
                while( (--j) > sCurStack.mLeftPos )
                {
                    sKeyInfo1 = (smnbKeyInfo*)&aBuffer[mKeySize * sKeyOffset[j]];
                    sKeyInfo2 = (smnbKeyInfo*)&aBuffer[mKeySize * sKeyOffset[sCurStack.mLeftPos]];
    
                    IDE_TEST( isRowUnique( aIndexStat,
                                           sIndexHeader,
                                           sKeyInfo1,
                                           sKeyInfo2,
                                           &sResult )
                              != IDE_SUCCESS );

                    if( sResult < 0)
                    {
                        break;
                    }
                }
    
                if( i < j )
                {
                    swapOffset( &sKeyOffset[i], &sKeyOffset[j] );
                }
                else
                {
                    break;
                }
            }
    
            swapOffset( &sKeyOffset[i-1], &sKeyOffset[sCurStack.mLeftPos] );

            if( i > (sCurStack.mLeftPos + 2) )
            {
                sNewStack.mLeftPos  = sCurStack.mLeftPos;
                sNewStack.mRightPos = i-2;

                IDE_TEST(mSortStack.push(ID_FALSE, &sNewStack)
                         != IDE_SUCCESS);
            }
            if( i < sCurStack.mRightPos )
            {
                sNewStack.mLeftPos  = i;
                sNewStack.mRightPos = sCurStack.mRightPos;

                IDE_TEST(mSortStack.push(ID_FALSE, &sNewStack)
                         != IDE_SUCCESS);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description:
 *
 * - mergeRun은 각 thread에서 만들어진 merge run들을
 *   정렬된 하나의 list로 만든다.
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::mergeRun( smnbStatistic    * aIndexStat )
{
    UInt                 sMergeRunInfoCnt = 0;
    smnbMergeRunInfo   * sMergeRunInfo;
    smnbBuildRun       * sCurRun = NULL;
    UInt                 sState  = 0;
    
    // merge할 run의 수가 한개 이하일 경우 skip
    mMergedRunCount = mSortedRunCount;

    IDE_TEST_CONT( mMergedRunCount <= 1, SKIP_MERGE );

    /* TC/FIT/Limit/sm/smn/smnb/smnbValuebaseBuild_mergeRun_malloc.sql */
    IDU_FIT_POINT_RAISE( "smnbValuebaseBuild::mergeRun::malloc",
                          insufficient_memory );
    
    // initialize heap
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 (ULong)mSortedRunCount * ID_SIZEOF(smnbMergeRunInfo),
                                 (void**)&sMergeRunInfo,
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    sCurRun = mFstRun;

    // merge할 run들을 mergeRun에 채움
    while( sCurRun != NULL )
    {
        sMergeRunInfo[sMergeRunInfoCnt].mRun     = sCurRun;
        sMergeRunInfo[sMergeRunInfoCnt].mSlotCnt = sCurRun->mSlotCount;
        sMergeRunInfo[sMergeRunInfoCnt].mSlotSeq = 0;

        sCurRun = sCurRun->mNext;
        sMergeRunInfo[sMergeRunInfoCnt].mRun->mNext = NULL;
        sMergeRunInfoCnt++;
    }

    // do merge
    IDE_TEST( merge( aIndexStat, sMergeRunInfoCnt, sMergeRunInfo )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sMergeRunInfo ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_MERGE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    if( sState == 1 )
    {
        (void)iduMemMgr::free( sMergeRunInfo );
    }

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description:
 *
 * - union merge run은 각 thread에서 merge된 list들을
 *   특정 갯수만큼의 list로 merge한다.
 * - make tree 단계에서 한꺼번에 merge하는 것보다
 *   병렬성을 극대화함으로써 index build 시간을 단축.
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::unionMergeRun( smnbStatistic  * aIndexStat )
{
    smnbMergeRunInfo  * sMergeRunInfo;
    UInt                sMergeRunInfoCnt   = 0;
    UInt                sTotalRunCount = 0;
    UInt                sBeginTID;
    UInt                sLastTID;
    UInt                sState = 0;
    UInt                i;

    // union merge를 실행할 thread ID
    sBeginTID = mMyTID;  

    // union merge를 실행할 thread에 할당된 마지막 thread의 ID
    sLastTID  = IDL_MIN( mMyTID + mMaxUnionRunCnt, mThreadCnt );

    // union merge는 모든 thread가 다 실행되지는 않는다.
    // 특정 갯수만큼의 thread가 다른 thread의 merge 결과를 가져와서 merge.
    // union merge를 수행할 TID가 아니면 종료.
    IDE_TEST_CONT( mMyTID % mMaxUnionRunCnt != 0, SKIP_MERGE );

    // sTotalRunCount : merge할 전체 run 수
    for( i = sBeginTID; i < sLastTID; i++ )
    {
        sTotalRunCount += mThreads[i].mMergedRunCount;
    }

    // merge할 run의 수가 0이면(no key) 종료
    IDE_TEST_CONT( sTotalRunCount <= 1, SKIP_MERGE );

    // initialize heap
    /* smnbValuebaseBuild_unionMergeRun_malloc_MergeRunInfo.tc */
    IDU_FIT_POINT("smnbValuebaseBuild::unionMergeRun::malloc::MergeRunInfo");
    IDE_TEST( iduMemMgr::malloc(
                  IDU_MEM_SM_SMN,
                  (ULong)sTotalRunCount * ID_SIZEOF(smnbMergeRunInfo),
                  (void**)&sMergeRunInfo,
                  IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sState = 1;

    // union merge를 위해 각 thread의 merge list를 가져옴
    for( i = sBeginTID; i < sLastTID; i++ )
    {
        if( mThreads[i].mMergedRunCount > 0 )
        {
            sMergeRunInfo[sMergeRunInfoCnt].mRun     =
                mThreads[i].mFstRun;
            sMergeRunInfo[sMergeRunInfoCnt].mSlotCnt =
                mThreads[i].mFstRun->mSlotCount;
            sMergeRunInfo[sMergeRunInfoCnt].mSlotSeq = 0;
            sMergeRunInfoCnt++;
        }
        mThreads[i].mMergedRunCount = 0;
    }

    // do merge
    IDE_TEST( merge( aIndexStat, sMergeRunInfoCnt, sMergeRunInfo )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sMergeRunInfo ) != IDE_SUCCESS );
    
    IDE_EXCEPTION_CONT( SKIP_MERGE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    if( sState == 1 )
    {
        (void)iduMemMgr::free( sMergeRunInfo );
    }

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Phase 3. Make Tree
 * ----------------------------------------------*/
IDE_RC smnbValuebaseBuild::makeTree( smnbValuebaseBuild  * aThreads,
                                     smnbStatistic       * aIndexStat,
                                     UInt                  aThreadCnt,
                                     smnbIntStack        * aStack )
{
    smnbHeader           * sIndexHeader;
    UInt                   sHeapMapCount = 1;
    SInt                 * sHeapMap;
    smnbMergeRunInfo     * sMergeRunInfo;
    UInt                   i, j;
    UInt                   sClosedRun = 0;
    UInt                   sMinIdx;
    idBool                 sIsClosedRun;
    UInt                   sMergeRunInfoCnt = 0;
    smnbBuildRun         * sCurRun = NULL;
    smnbBuildRun         * sTmpRun = NULL;

    UInt                   sTotalRunCount = 0;
    smnbKeyInfo          * sKeyInfo;
    smnbINode            * sParentPtr;
    SInt                   sDepth;
    UInt                   sSequence = 0;
    smnbLNode            * sLstNode;
    UInt                   sState = 0;
    smnIndexModule       * sIndexModules;
    SChar                * sRow = NULL;
    void                 * sKey = NULL;

    sIndexHeader = (smnbHeader*)(mIndex->mHeader);

    sIndexModules = mIndex->mModule;

    sIndexHeader->pFstNode = NULL;
    sIndexHeader->pLstNode = NULL;

    for(i = 0; i < aThreadCnt; i++)
    {
        sTotalRunCount = sTotalRunCount + aThreads[i].mMergedRunCount;
    }

    if( sTotalRunCount == 0 )
    {
        if( mFstRun != NULL )
        {
            freeRun( mFstRun );
        }
        
        IDE_CONT( RETURN_SUCCESS );
    }

    if( sTotalRunCount == 1 )
    {
        for(i = 0; i < aThreadCnt; i++ )
        {
            if( aThreads[i].mMergedRunCount != 0 )
            {
                break;
            }
        }
        sCurRun = aThreads[i].mFstRun;

        while( sCurRun != NULL )
        {
            for( i = 0; i < sCurRun->mSlotCount; i++ )
            {
                sKeyInfo = (smnbKeyInfo*)&(sCurRun->mBody[i * mKeySize]);
                
                IDE_TEST( write2LeafNode( aIndexStat,
                                          sIndexHeader,
                                          aStack,
                                          sKeyInfo->rowPtr )
                          != IDE_SUCCESS );
            }
            sTmpRun = sCurRun;
            sCurRun = sCurRun->mNext;
            freeRun( sTmpRun );
        }
        IDE_CONT( SKIP_MERGE_PHASE );
    }

    //TC/FIT/Limit/sm/smn/smnb/smnbValuebaseBuild_makeTree_malloc1.sql
    IDU_FIT_POINT_RAISE( "smnbValuebaseBuild::makeTree::malloc1",
                          insufficient_memory );

    // initialize heap
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMN, 
                                 (ULong)sTotalRunCount * ID_SIZEOF(smnbMergeRunInfo),
                                 (void**)&sMergeRunInfo,
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    for( i = 0; i < aThreadCnt; i++ )
    {
        sCurRun = aThreads[i].mFstRun;
        for(j = 0; j < aThreads[i].mMergedRunCount; j++)
        {
            sMergeRunInfo[sMergeRunInfoCnt].mRun     = sCurRun;
            sMergeRunInfo[sMergeRunInfoCnt].mSlotCnt = sCurRun->mSlotCount;
            sMergeRunInfo[sMergeRunInfoCnt].mSlotSeq = 0;

            sCurRun = sCurRun->mNext;
            sMergeRunInfoCnt++;
        }
    }

    while( sHeapMapCount < ( sMergeRunInfoCnt * 2) )
    {
        sHeapMapCount = sHeapMapCount * 2;
    }
    
    //TC/FIT/Limit/sm/smn/smnb/smnbValuebaseBuild_makeTree_malloc2.sql
    IDU_FIT_POINT_RAISE( "smnbValuebaseBuild::makeTree::malloc2",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SMN,
                                (ULong)sHeapMapCount * ID_SIZEOF(SInt),
                                (void**)&sHeapMap,
                                IDU_MEM_IMMEDIATE ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 2;

    for( i = 0; i < sHeapMapCount; i++ )
    {
        sHeapMap[i] = -1;
    }
    
    IDE_TEST( heapInit( aIndexStat,
                        sMergeRunInfoCnt,
                        sHeapMapCount,
                        sHeapMap,
                        sMergeRunInfo )
              != IDE_SUCCESS );    

    // the last merge & build tree
    while( 1 )
    {
        // selection tree의 모든 run들에서 key를 꺼냈을 때 종료
        if( sClosedRun == sMergeRunInfoCnt )
        {
            break;
        }
        
        sMinIdx = sHeapMap[1];   // the root of selection tree

        // run에서 가져올 key의 offset을 계산하여 key를 가져옴
        sKeyInfo =
            ((smnbKeyInfo*)
             &(sMergeRunInfo[sMinIdx].mRun->mBody[sMergeRunInfo[sMinIdx].mSlotSeq * mKeySize]));

        // row pointer를 leaf node에 쓴다
        IDE_TEST( write2LeafNode( aIndexStat,
                                  sIndexHeader,
                                  aStack,
                                  sKeyInfo->rowPtr )
                  != IDE_SUCCESS );

        // 매번 Session Event를 검사하는 것은 성능상 부하이기 때문에,
        // 새로운 LeafNode가 생성될때마다 Session Event를 검사한다.
        if( sSequence != sIndexHeader->pLstNode->sequence )
        {
            sSequence = sIndexHeader->pLstNode->sequence;
            IDE_TEST(iduCheckSessionEvent( mStatistics )
                     != IDE_SUCCESS);
         }

        // heap에서 key를 가져온다
        IDE_TEST( heapPop( aIndexStat,
                           sMinIdx,
                           &sIsClosedRun,
                           sHeapMapCount,
                           sHeapMap,
                           sMergeRunInfo )
                  != IDE_SUCCESS );

        if( sIsClosedRun == ID_TRUE )
        {
            sClosedRun++;
        }            
    }

    sState = 1;
    IDE_TEST( iduMemMgr::free( sHeapMap ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sMergeRunInfo ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_MERGE_PHASE );

    if( aStack->mDepth >= 0 )
    {
        IDE_ERROR( sIndexHeader->pLstNode != NULL );
        sLstNode = (smnbLNode *)(sIndexHeader->pLstNode);
        sRow     = sLstNode->mRowPtrs[sLstNode->mSlotCount - 1];
        sKey     = ( ( SMNB_IS_DIRECTKEY_INDEX( sIndexHeader ) == ID_TRUE ) ?
                     SMNB_GET_KEY_PTR( sLstNode, sLstNode->mSlotCount - 1 ) : NULL );

        IDE_TEST( write2ParentNode( aIndexStat,
                                    sIndexHeader,
                                    aStack,
                                    0,
                                    (smnbNode*)sLstNode,
                                    sRow,
                                    sKey )
                  != IDE_SUCCESS );

        for( sDepth = 0; sDepth < aStack->mDepth; sDepth++ )
        {
            sParentPtr = aStack->mStack[sDepth].node;
            sRow       = sParentPtr->mRowPtrs[sParentPtr->mSlotCount - 1];
            sKey       = ( ( SMNB_IS_DIRECTKEY_INDEX( sIndexHeader ) == ID_TRUE ) ?
                           SMNB_GET_KEY_PTR( sParentPtr, sParentPtr->mSlotCount - 1 ) : NULL );

            IDE_TEST( write2ParentNode(
                          aIndexStat,
                          sIndexHeader,
                          aStack,
                          sDepth + 1,
                          (smnbNode*)sParentPtr,
                          sRow,
                          sKey )
                      != IDE_SUCCESS );
        }

        sParentPtr = (smnbINode*)(aStack->mStack[aStack->mDepth].node);

        while ( SMNB_IS_INTERNAL_NODE( sParentPtr ) )
        {
            /* PROJ-2433
             * child pointer는 유지 */
            smnbBTree::setInternalSlot( sParentPtr,
                                        (SShort)( sParentPtr->mSlotCount - 1 ),
                                        sParentPtr->mChildPtrs[sParentPtr->mSlotCount - 1],
                                        NULL,
                                        NULL );

            sParentPtr = (smnbINode*)(sParentPtr->mChildPtrs[sParentPtr->mSlotCount - 1]);
        }

        sIndexHeader->root = (void *)(aStack->mStack[aStack->mDepth].node);
    }
    else
    {
        sIndexHeader->root = (void *)(sIndexHeader->pLstNode);
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    if( sTotalRunCount == 0 )
    {
        sIndexHeader->root = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    IDE_PUSH();
    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( sHeapMap );
        case 1:
            (void)iduMemMgr::free( sMergeRunInfo );
            break;
    }

    // 생성된 index node들을 free시킨다.
    if( aStack->mDepth == -1 )
    {
        ((smnbHeader*)(mIndex->mHeader))->root =
            (void *)(((smnbHeader*)mIndex->mHeader)->pLstNode);
    }
    else
    {
        ((smnbHeader*)(mIndex->mHeader))->root =
            aStack->mStack[aStack->mDepth].node;
    }

    // BUG-19247
    (void)(sIndexModules)->mFreeAllNodeList( mStatistics,
                                             mIndex,
                                             mTrans );
    IDE_POP();
    
    return IDE_FAILURE;
}


IDE_RC smnbValuebaseBuild::write2LeafNode( smnbStatistic    * aIndexStat,
                                           smnbHeader       * aHeader,
                                           smnbIntStack     * aStack,
                                           SChar            * aRowPtr )
{
    smnbLNode * sLNode;
    SInt        sDepth = 0;
    SChar     * sRow   = NULL;
    void      * sKey   = NULL;

    sLNode = aHeader->pLstNode;
    
    if ( ( sLNode == NULL ) ||
         ( sLNode->mSlotCount >= SMNB_LEAF_SLOT_MAX_COUNT( aHeader ) ) )
    {
        if( sLNode != NULL )
        {
            sRow = sLNode->mRowPtrs[sLNode->mSlotCount - 1];
            sKey = ( ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE ) ?
                     SMNB_GET_KEY_PTR( sLNode, sLNode->mSlotCount - 1 ) : NULL );

            IDE_TEST( write2ParentNode(
                          aIndexStat,
                          aHeader,
                          aStack,
                          sDepth,
                          (smnbNode*)sLNode,
                          sRow,
                          sKey )
                      != IDE_SUCCESS );
        }
        
        IDE_TEST(aHeader->mNodePool.allocateSlots(1,
                                               (smmSlot**)&(sLNode))
                 != IDE_SUCCESS);
        
        smnbBTree::initLeafNode( sLNode,
                                 aHeader,
                                 IDU_LATCH_UNLOCKED );

        aHeader->nodeCount++;
        
        if( aHeader->pFstNode == NULL )
        {
            sLNode->sequence  = 0;
            aHeader->pFstNode = sLNode;
            aHeader->pLstNode = sLNode;
        }
        else
        {
            IDE_ERROR(aHeader->pLstNode != NULL);

            sLNode->sequence            = aHeader->pLstNode->sequence + 1;
            aHeader->pLstNode->nextSPtr = sLNode;
            sLNode->prevSPtr            = aHeader->pLstNode;
            aHeader->pLstNode           = sLNode;
        }
    }
    else
    {
        /* nothing to do */
    }

    smnbBTree::setLeafSlot( sLNode,
                            sLNode->mSlotCount,
                            aRowPtr,
                            NULL ); /* PROJ-2433 : key는 아래에서 세팅 */

    /* PROJ-2433
     * direct key 세팅 */
    if ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE )
    {
        IDE_TEST( smnbBTree::makeKeyFromRow( aHeader,
                                             aRowPtr,
                                             SMNB_GET_KEY_PTR( sLNode, sLNode->mSlotCount ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    (sLNode->mSlotCount)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbValuebaseBuild::write2ParentNode( smnbStatistic    * aIndexStat,
                                             smnbHeader       * aHeader,
                                             smnbIntStack     * aStack,
                                             SInt               aDepth,
                                             smnbNode         * aChildPtr,
                                             SChar            * aRowPtr,
                                             void             * aKey )
{
    smnbINode * sINode;
    SChar     * sRow = NULL;
    void      * sKey = NULL;

    if( aStack->mDepth < aDepth )
    {
        IDE_TEST(aHeader->mNodePool.allocateSlots( 1,
                                                   (smmSlot**)&(sINode) )
                 != IDE_SUCCESS);

        smnbBTree::initInternalNode( sINode,
                                     aHeader,
                                     IDU_LATCH_UNLOCKED );

        aHeader->nodeCount++;
        
        aStack->mDepth = aDepth;
        aStack->mStack[aDepth].node = sINode;
    }
    else
    {
        sINode = aStack->mStack[aDepth].node;
    }

    if ( sINode->mSlotCount >= SMNB_INTERNAL_SLOT_MAX_COUNT( aHeader ) )
    {
        sRow = sINode->mRowPtrs[sINode->mSlotCount - 1];
        sKey = ( ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE ) ?
                 SMNB_GET_KEY_PTR( sINode, sINode->mSlotCount - 1 ) : NULL );

        IDE_TEST( write2ParentNode( aIndexStat,
                                    aHeader,
                                    aStack,
                                    aDepth + 1,
                                    (smnbNode*)sINode,
                                    sRow,
                                    sKey )
                  != IDE_SUCCESS );

        IDE_TEST(aHeader->mNodePool.allocateSlots( 1,
                                                   (smmSlot**)&(sINode) )
                 != IDE_SUCCESS);

        smnbBTree::initInternalNode( sINode,
                                     aHeader,
                                     IDU_LATCH_UNLOCKED );

        aHeader->nodeCount++;
        
        aStack->mStack[aDepth].node->nextSPtr = sINode;
        sINode->prevSPtr = aStack->mStack[aDepth].node;
        aStack->mStack[aDepth].node = sINode;
    }
    else
    {
        /* nothing to do */
    }
     
    /* PROJ-2433
     * internal node에 row를 추가한다 */
    smnbBTree::setInternalSlot( sINode,
                                sINode->mSlotCount,
                                (smnbNode *)aChildPtr,
                                aRowPtr,
                                aKey );

    (sINode->mSlotCount)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbValuebaseBuild::heapInit( smnbStatistic     * aIndexStat,
                                     UInt                aRunCount,
                                     UInt                aHeapMapCount,
                                     SInt              * aHeapMap,
                                     smnbMergeRunInfo  * aMergeRunInfo )
{
    SInt                sRet;
    UInt                i;
    UInt                sLChild;
    UInt                sRChild;
    UInt                sPos;
    UInt                sHeapLevelCnt;
    smnbHeader        * sIndexHeader;
    smnbMergeRunInfo  * sMergeRunInfo1;
    smnbMergeRunInfo  * sMergeRunInfo2;
    smnbKeyInfo       * sKeyInfo1;
    smnbKeyInfo       * sKeyInfo2;

    sPos = aHeapMapCount / 2;
    sHeapLevelCnt = aRunCount;
    sIndexHeader = (smnbHeader*)(mIndex->mHeader);

    // heapMap의 leaf level 초기화 : mergeRun 배열의 index
    for( i = 0; i < sHeapLevelCnt; i++ )
    {
        aHeapMap[i + sPos] = i;
    }

    // heapMap의 leaf level 초기화
    //  : run이 할당되지 않은 heapMap의 나머지 부분은 -1로 설정
    for(i = i + sPos ; i < aHeapMapCount; i++)
    {
        aHeapMap[i] = -1;
    }

    while( 1 )
    {
        // bottom-up으로 heapMap 초기화
        sPos = sPos / 2;
        sHeapLevelCnt  = (sHeapLevelCnt + 1) / 2;
        
        for( i = 0; i < sHeapLevelCnt; i++ )
        {
            sLChild = (i + sPos) * 2;
            sRChild = sLChild + 1;
            
            if (aHeapMap[sLChild] == -1 && aHeapMap[sRChild] == -1)
            {
                aHeapMap[i + sPos]= -1;
            }
            else if (aHeapMap[sLChild] == -1)                     
            {
                aHeapMap[i + sPos] = aHeapMap[sRChild];
            }
            else if (aHeapMap[sRChild] == -1)
            {
                aHeapMap[i + sPos] = aHeapMap[sLChild];
            }
            else
            {
                sMergeRunInfo1 = &aMergeRunInfo[aHeapMap[sLChild]];
                sMergeRunInfo2 = &aMergeRunInfo[aHeapMap[sRChild]];
                
                sKeyInfo1  = (smnbKeyInfo*)(&(sMergeRunInfo1->mRun->mBody[sMergeRunInfo1->mSlotSeq*mKeySize]));
                sKeyInfo2  = (smnbKeyInfo*)(&(sMergeRunInfo2->mRun->mBody[sMergeRunInfo2->mSlotSeq*mKeySize]));

                IDE_TEST( isRowUnique( aIndexStat,
                                       sIndexHeader,
                                       sKeyInfo1,
                                       sKeyInfo2,
                                       &sRet )
                          != IDE_SUCCESS );

                if(sRet > 0)
                {
                    aHeapMap[i + sPos] = aHeapMap[sRChild];
                }
                else
                {
                    aHeapMap[i + sPos] = aHeapMap[sLChild];
                }
            }                
        }
        if(sPos == 1)    // the root of selection tree
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbValuebaseBuild::heapPop( smnbStatistic    * aIndexStat,
                                    UInt               aMinIdx,
                                    idBool           * aIsClosedRun,
                                    UInt               aHeapMapCount,
                                    SInt             * aHeapMap,
                                    smnbMergeRunInfo * aMergeRunInfo )
{
    SInt                sRet;
    UInt                sLChild;
    UInt                sRChild;
    smnbHeader        * sIndexHeader;
    smnbMergeRunInfo  * sMergeRunInfo1;
    smnbMergeRunInfo  * sMergeRunInfo2;
    UInt                sPos;
    smnbBuildRun      * sTmpRun;
    smnbKeyInfo       * sKeyInfo1;
    smnbKeyInfo       * sKeyInfo2;

    sIndexHeader = (smnbHeader*)(mIndex->mHeader);
    sPos = aHeapMapCount / 2;

    aMergeRunInfo[aMinIdx].mSlotSeq++;
    *aIsClosedRun = ID_FALSE;

    // run의 마지막 slot일 경우
    if ( aMergeRunInfo[aMinIdx].mSlotSeq == aMergeRunInfo[aMinIdx].mSlotCnt )
    {
        sTmpRun = aMergeRunInfo[aMinIdx].mRun;

        // next가 NULL이면 heapMap에 -1을 설정하여 key가 없음을 표시
        // 아니면 list의 next를 서핑
        if( sTmpRun->mNext == NULL )
        {
            aMergeRunInfo[aMinIdx].mRun     = NULL;
            aMergeRunInfo[aMinIdx].mSlotSeq = 0; 
            *aIsClosedRun               = ID_TRUE;
            aHeapMap[sPos + aMinIdx]    = -1;
        }
        else
        {
            aMergeRunInfo[aMinIdx].mRun     = sTmpRun->mNext;
            aMergeRunInfo[aMinIdx].mSlotSeq = 0;
            aMergeRunInfo[aMinIdx].mSlotCnt = aMergeRunInfo[aMinIdx].mRun->mSlotCount;
        }

        freeRun( sTmpRun );
    }
    else
    {
        IDE_DASSERT( aMergeRunInfo[aMinIdx].mSlotSeq < aMergeRunInfo[aMinIdx].mSlotCnt );
    }

    // heap에서 pop된 run의 parent 위치
    sPos = (sPos + aMinIdx) / 2;
    
    while( 1 )
    {
        sLChild = sPos * 2;      // Left child position
        sRChild = sLChild + 1;   // Right child Position
        
        if (aHeapMap[sLChild] == -1 && aHeapMap[sRChild] == -1)
        {
            aHeapMap[sPos]= -1;
        }
        else if (aHeapMap[sLChild] == -1)
        {
            aHeapMap[sPos] = aHeapMap[sRChild];
        }
        else if (aHeapMap[sRChild] == -1)
        {
            aHeapMap[sPos] = aHeapMap[sLChild];
        }
        else
        {
            sMergeRunInfo1 = &aMergeRunInfo[aHeapMap[sLChild]];
            sMergeRunInfo2 = &aMergeRunInfo[aHeapMap[sRChild]];
            
            sKeyInfo1  = (smnbKeyInfo*)(&(sMergeRunInfo1->mRun->mBody[sMergeRunInfo1->mSlotSeq*mKeySize]));
            sKeyInfo2  = (smnbKeyInfo*)(&(sMergeRunInfo2->mRun->mBody[sMergeRunInfo2->mSlotSeq*mKeySize]));

            IDE_TEST( isRowUnique( aIndexStat,
                                   sIndexHeader,
                                   sKeyInfo1,
                                   sKeyInfo2,
                                   &sRet )
                      != IDE_SUCCESS );

            if(sRet > 0)
            {
                aHeapMap[sPos] = aHeapMap[sRChild];
            }
            else
            {
                aHeapMap[sPos] = aHeapMap[sLChild];
            }
        }
        if(sPos == 1)    // the root of selection tree
        {
            break;
        }
        sPos = sPos / 2;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;    
    
    return IDE_FAILURE;
}

SInt smnbValuebaseBuild::compareKeys( smnbStatistic    * aIndexStat,
                                      const smnbColumn * aColumns,
                                      const smnbColumn * aFence,
                                      const void       * aKey1,
                                      const void       * aKey2 )
{
    SInt         sResult;
    smiValueInfo sValueInfo1;
    smiValueInfo sValueInfo2;    

    if( aKey1 == NULL )
    {
        if( aKey2 == NULL )
        {
            return 0;
        }

        return 1;
    }
    else
    {
        if( aKey2 == NULL )
        {
            return -1;
        }
    }

    SMI_SET_VALUEINFO(
        &sValueInfo1, NULL, aKey1, 0, SMI_OFFSET_USE,
        &sValueInfo2, NULL, aKey2, 0, SMI_OFFSET_USE );

    for( ; aColumns < aFence; aColumns++ )
    {
        aIndexStat->mKeyCompareCount++;

        sValueInfo1.column = &(aColumns->keyColumn);
        sValueInfo2.column = &(aColumns->keyColumn);

        sResult = aColumns->compare( &sValueInfo1,
                                     &sValueInfo2 );
        if( sResult != 0 )
        {
            return sResult;
        }
    }
    
    return 0;
}

SInt smnbValuebaseBuild::compareKeys( smnbStatistic      * aIndexStat,
                                      const smnbColumn   * aColumns,
                                      const smnbColumn   * aFence,
                                      SChar              * aKey1,
                                      SChar              * aKey2,
                                      SChar              * aRowPtr1,
                                      SChar              * aRowPtr2,
                                      idBool             * aIsEqual )
{
    SInt         sResult;
    smiValueInfo sValueInfo1;
    smiValueInfo sValueInfo2;

    *aIsEqual = ID_FALSE; 

    if( aKey1 == NULL )
    {
        if( aKey2 == NULL )
        {
            *aIsEqual = ID_TRUE;
            
            return 0;
        }

        return 1;
    }
    else
    {
        if( aKey2 == NULL )
        {
            return -1;
        }
    }

    SMI_SET_VALUEINFO(
        &sValueInfo1, NULL, aKey1, 0, SMI_OFFSET_USE,
        &sValueInfo2, NULL, aKey2, 0, SMI_OFFSET_USE );

    for( ; aColumns < aFence; aColumns++ )
    {
        aIndexStat->mKeyCompareCount++;

        sValueInfo1.column = &(aColumns->keyColumn);
        sValueInfo2.column = &(aColumns->keyColumn);
        
        sResult = aColumns->compare( &sValueInfo1,
                                     &sValueInfo2 );
        if( sResult != 0 )
        {
            return sResult;
        }
    }

    *aIsEqual = ID_TRUE;

    /* BUG-39043 키 값이 동일 할 경우 pointer 위치로 순서를 정하도록 할 경우
     * 서버를 새로 올린 경우 위치가 변경되기 때문에 persistent index를 사용하는 경우
     * 저장된 index와 실제 순서가 맞지 않아 문제가 생길 수 있다.
     * 이를 해결하기 위해 키 값이 동일 한 경우 OID로 순서를 정하도록 한다. */
    if( SMP_SLOT_GET_OID( aRowPtr1 ) > SMP_SLOT_GET_OID( aRowPtr2 ) )
    {
        return 1;
    }
    if( SMP_SLOT_GET_OID( aRowPtr1 ) < SMP_SLOT_GET_OID( aRowPtr2 ) )
    {
        return -1;
    }

    return 0;
}

/* ------------------------------------------------
 * 모든 컬럼이 NULL을 가지고 있는지 검사
 * Unique Index가 아닐 경우 무조건 Null이라고 인식하기
 * 때문에 주의해야 한다.
 * ----------------------------------------------*/
idBool smnbValuebaseBuild::isNull( smnbHeader  * aHeader,
                                   SChar       * aKeyValue )
{
    smnbColumn * sColumn;
    idBool       sIsNull = ID_TRUE;
    
    // Build하기 위한 Column 구조를 써야 하기 때문에
    // smnbModule::isNullKey를 사용할 수 없다
    sColumn = &aHeader->columns4Build[0];
    for( ; sColumn != aHeader->fence4Build; sColumn++ )
    {
        if( smnbBTree::isNullColumn( sColumn,
                                     &(sColumn->keyColumn),
                                     aKeyValue ) != ID_TRUE )
        {
            sIsNull = ID_FALSE;
            break;
        }
    }

    return sIsNull;
}

IDE_RC smnbValuebaseBuild::merge( smnbStatistic     * aIndexStat,
                                  UInt                aMergeRunInfoCnt,
                                  smnbMergeRunInfo  * aMergeRunInfo )
{
    UInt           sHeapMapCount = 1;
    SInt         * sHeapMap;
    UInt           i;
    UInt           sClosedRun = 0;
    UInt           sMinIdx;
    idBool         sIsClosedRun;
    smnbBuildRun * sMergeRun = NULL;
    UInt           sState    = 0;
    
    while( sHeapMapCount < (aMergeRunInfoCnt * 2) )
    {
        sHeapMapCount = sHeapMapCount * 2;
    }
    
    /* TC/FIT/Limit/sm/smn/smnb/smnbValuebaseBuild_merge_malloc.sql */
    IDU_FIT_POINT_RAISE( "smnbValuebaseBuild::merge::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 (ULong)sHeapMapCount * ID_SIZEOF(SInt),
                                 (void**)&sHeapMap,
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    for( i = 0; i < sHeapMapCount; i++ )
    {
        sHeapMap[i] = -1;
    }
    
    IDE_TEST( heapInit( aIndexStat,
                        aMergeRunInfoCnt,
                        sHeapMapCount,
                        sHeapMap,
                        aMergeRunInfo )
              != IDE_SUCCESS );    

    IDE_TEST( allocRun( &sMergeRun ) != IDE_SUCCESS );

    mFstRun = mLstRun = sMergeRun;

    while( 1 )
    {
        if( sClosedRun == aMergeRunInfoCnt )
        {
            break;
        }

        sMinIdx = (UInt)sHeapMap[1];   // the root of selection tree
        
        if( sMergeRun->mSlotCount == mMaxMergeRunKeyCnt )
        {
            IDE_TEST( allocRun( &sMergeRun ) != IDE_SUCCESS );
            mLstRun->mNext  = sMergeRun;
            mLstRun         = sMergeRun;
        }

        idlOS::memcpy( &(sMergeRun->mBody[sMergeRun->mSlotCount * mKeySize]),
                       &(aMergeRunInfo[sMinIdx].mRun->mBody[aMergeRunInfo[sMinIdx].mSlotSeq * mKeySize]),
                       mKeySize );

        sMergeRun->mSlotCount++;

        IDE_TEST( heapPop( aIndexStat,
                           sMinIdx,
                           &sIsClosedRun,
                           sHeapMapCount,
                           sHeapMap,
                           aMergeRunInfo )
                  != IDE_SUCCESS );

        if( sIsClosedRun == ID_TRUE )
        {
            sClosedRun++;
            
            IDE_TEST( iduCheckSessionEvent( mStatistics )
                      != IDE_SUCCESS);
        }
    }

    mMergedRunCount = 1;

    sState = 0;
    IDE_TEST( iduMemMgr::free( sHeapMap ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    IDE_PUSH();
    if( sState == 1 )
    {
        (void)iduMemMgr::free( sHeapMap );
    }
    IDE_POP();

    return IDE_FAILURE;
}
