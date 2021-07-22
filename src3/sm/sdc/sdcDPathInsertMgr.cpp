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
 

/*******************************************************************************
 * $Id: sdcDPathInsertMgr.cpp 83870 2018-09-03 04:32:39Z kclee $
 ******************************************************************************/

#include <sdcReq.h>
#include <sdcDPathInsertMgr.h>
#include <sdbDPathBufferMgr.h>
#include <sdpDef.h>
#include <sdpDPathInfoMgr.h>

iduMemPool      sdcDPathInsertMgr::mDPathEntryPool;

sdcDPathStat    sdcDPathInsertMgr::mDPathStat;
iduMutex        sdcDPathInsertMgr::mStatMtx;

/*******************************************************************************
 * Description : static 변수들을 초기화 한다.
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::initializeStatic()
{
    SInt    sState = 0;

    IDE_TEST( sdbDPathBufferMgr::initializeStatic() != IDE_SUCCESS );
    sState = 1;
    IDE_TEST( sdpDPathInfoMgr::initializeStatic() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mDPathEntryPool.initialize(
                            IDU_MEM_SM_SDC,
                            (SChar*)"DIRECT_PATH_ENTRY_MEMPOOL",
                            1, /* List Count */
                            ID_SIZEOF( sdcDPathEntry ),
                            16, /* 생성시 가지고 있는 Item갯수 */
                            IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                            ID_TRUE,							/* UseMutex */
                            IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignSize */
                            ID_FALSE,							/* ForcePooling */
                            ID_TRUE,							/* GarbageCollection */
                            ID_TRUE,                            /* HWCacheLine */
                            IDU_MEMPOOL_TYPE_LEGACY             /* mempool type */) 
              != IDE_SUCCESS);			
    sState = 3;

    // 통계 값에 접근 시, 동시성 제어를 위한 Mutex 초기화
    IDE_TEST( mStatMtx.initialize( (SChar*)"DIRECT_PATH_INSERT_STAT_MUTEX",
                                    IDU_MUTEX_KIND_NATIVE,
                                    IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 4;

    // 통계 정보 자료구조 mDPathStat 초기화
    mDPathStat.mCommitTXCnt    = 0;
    mDPathStat.mAbortTXCnt     = 0;
    mDPathStat.mInsRowCnt      = 0;
    mDPathStat.mAllocBuffPageTryCnt    = 0;
    mDPathStat.mAllocBuffPageFailCnt   = 0;
    mDPathStat.mBulkIOCnt      = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 4:
            IDE_ASSERT( mStatMtx.destroy() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( mDPathEntryPool.destroy() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sdpDPathInfoMgr::destroyStatic() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdbDPathBufferMgr::destroyStatic() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : static 변수들을 파괴한다.
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::destroyStatic()
{
    IDE_TEST( sdbDPathBufferMgr::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( sdpDPathInfoMgr::destroyStatic() != IDE_SUCCESS );
    IDE_TEST( mDPathEntryPool.destroy( ID_TRUE ) != IDE_SUCCESS );
    IDE_TEST( mStatMtx.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Transaction당 Direct-Path INSERT 관리용 자료구조인 DPathEntry를
 *          생성한다.
 *
 * Implementation :
 *          1. sdcDPathEntry 하나 메모리 할당
 *          2. 멤버 변수 초기화
 *          3. DPathBuffInfo 생성
 *          4. DPathInfo 생성
 *          5. aDPathEntry 매개변수에 할당하여 OUT
 *
 * Parameters :
 *      aDPathEntry - [OUT] 생성한 DPathEntry를 할당하여 반환할 OUT 매개변수
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::allocDPathEntry( void **aDPathEntry )
{
    SInt            sState = 0;
    sdcDPathEntry * sDPathEntry;

    IDE_DASSERT( aDPathEntry != NULL );

    // DPathEntry 하나를 메모리 할당하고 초기화 한다.
    /* sdcDPathInsertMgr_allocDPathEntry_alloc_DPathEntry.tc */
    IDU_FIT_POINT("sdcDPathInsertMgr::allocDPathEntry::alloc::DPathEntry");
    IDE_TEST( mDPathEntryPool.alloc( (void**)&sDPathEntry ) != IDE_SUCCESS );
    sState = 1;

    sDPathEntry = new ( sDPathEntry ) sdcDPathEntry();

    // DPathBuffInfo를 초기화한다.
    IDE_TEST( sdbDPathBufferMgr::initDPathBuffInfo(&sDPathEntry->mDPathBuffInfo)
              != IDE_SUCCESS );
    sState = 2;

    // DPathInfo를 초기화한다.
    IDE_TEST( sdpDPathInfoMgr::initDPathInfo(&sDPathEntry->mDPathInfo)
              != IDE_SUCCESS );
    sState = 3;

    /* FIT/ART/sm/Projects/PROJ-2068/PROJ-2068.ts  */
    IDU_FIT_POINT( "1.PROJ-2068@sdcDPathInsertMgr::allocDPathEntry" );

    // OUT 매개변수에 달아준다.
    *aDPathEntry = (void*)sDPathEntry;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 3:
            IDE_ASSERT( sdpDPathInfoMgr::destDPathInfo(&sDPathEntry->mDPathInfo)
                        == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sdbDPathBufferMgr::destDPathBuffInfo(
                                                &sDPathEntry->mDPathBuffInfo)
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mDPathEntryPool.memfree( sDPathEntry ) == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Transaction당 Direct-Path INSERT 관리용 자료구조인 DPathEntry를
 *          파괴한다.
 *
 * Implementation :
 *          1. mDPathBuffInfo 파괴
 *          2. mDPathInfo 파괴
 *          3. DPathEntry 메모리 할당 해제
 *
 * Parameters :
 *      aDPathEntry - [IN] 파괴할 DPathEntry
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::destDPathEntry( void *aDPathEntry )
{
    sdcDPathEntry  * sDPathEntry;

    IDE_DASSERT( aDPathEntry != NULL );

    sDPathEntry = (sdcDPathEntry*)aDPathEntry;

    // mDPathBuffInfo 파괴
    IDE_TEST( sdbDPathBufferMgr::destDPathBuffInfo(
                                                &sDPathEntry->mDPathBuffInfo)
              != IDE_SUCCESS );

    // mDPathInfo 파괴
    IDE_TEST( sdpDPathInfoMgr::destDPathInfo(&sDPathEntry->mDPathInfo)
              != IDE_SUCCESS );

    IDE_TEST( mDPathEntryPool.memfree( sDPathEntry ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : DPathSegInfo를 얻어온다.
 *          여기서 할당 및 생성한 DPathSegInfo는 Commit시에 merge이후, 그 때
 *          까지 생성한 모든 DPathSegInfo를 한번에 파괴해 준다.
 *
 * Implementation :
 *      1. aTrans에 달려있는 DPathEntry를 얻어온다.
 *      2. aTableOID에 대응하는 DPathSegInfo를 만들어서 DPathEntry->DPathInfo에
 *         매달고, 반환해 준다.
 *
 * Parameters :
 *      aStatistics     - [IN] 통계
 *      aTrans          - [IN] DPath INSERT를 수행하는 Transaction의 포인터
 *      aTableOID       - [IN] DPath INSERT를 수행할 대상 Table의 OID
 *                             대응하는 DPathSegInfo를 얻어오기 위한 값
 *      aDPathSegInfo   - [OUT] 얻어온 DPathSegInfo를 할당하여 반환
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::allocDPathSegInfo(
                                        idvSQL             * aStatistics,
                                        void               * aTrans,
                                        smOID                aTableOID,
                                        void              ** aDPathSegInfo )
{
    smTID               sTID;
    sdcDPathEntry     * sDPathEntry;
    sdpDPathInfo      * sDPathInfo;
    sdpDPathSegInfo   * sDPathSegInfo;

    IDE_DASSERT( aStatistics != NULL );
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTableOID != SM_NULL_OID );
    IDE_DASSERT( aDPathSegInfo != NULL );

    sDPathSegInfo = (sdpDPathSegInfo*)*aDPathSegInfo;
    
    // DPathEntry를 얻어온다.
    sDPathEntry = (sdcDPathEntry*)smLayerCallback::getDPathEntry( aTrans );
    if( sDPathEntry == NULL )
    {
        // aTrans에 대하여 allocDPathEntry를 먼저 수행하지 않은 상태에서
        // allocDPathSegInfo가 호출된 상황.
        //
        // allocDPathEntry를 먼저 호출해 주어야 한다.
        sTID = smLayerCallback::getTransID( aTrans );
        ideLog::log( IDE_SERVER_0,
                     "Trans ID  : %u",
                     sTID );
        IDE_ASSERT( 0 );
    }

    // DPathSegInfo를 얻어온다.
    sDPathInfo = &sDPathEntry->mDPathInfo;

    IDE_TEST( sdpDPathInfoMgr::createDPathSegInfo(aStatistics,
                                                  aTrans,
                                                  aTableOID,
                                                  sDPathInfo,
                                                  &sDPathSegInfo)
              != IDE_SUCCESS );

    *aDPathSegInfo = sDPathSegInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : BUG-30109 Direct-Path Buffer에 APPEND PAGE LIST에 달려있는
 *          페이지 들은 새로운 페이지 할당 시도가 있을 때, 이전에 할당된 페이지
 *          가 Update Only 상태인지 확인하여 setDirtyPage를 수행한다.
 *           각 커서 단위에서 마지막으로 할당된 페이지는, 그 후 추가적인 페이지
 *          할당이 이루어 지지 않기 때문에 setDirtyPage될 기회를 얻을 수 없다.
 *          따라서 Cursor를 close 할 때, aDPathSegInfo에서 마지막으로 할당된
 *          페이지는 별도로 setDirtyPage()를 수행해 주어야 한다.
 * 
 * Parameters :
 *      aDPathSegInfo   - [IN] sdpDPathSegInfo의 포인터
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::setDirtyLastAllocPage( void *aDPathSegInfo )
{
    sdpDPathSegInfo   * sDPathSegInfo;

    IDE_DASSERT( aDPathSegInfo != NULL );

    sDPathSegInfo = (sdpDPathSegInfo*)aDPathSegInfo;

    if( sDPathSegInfo->mLstAllocPagePtr != NULL )
    {
        IDE_TEST( sdbDPathBufferMgr::setDirtyPage(
                                        sDPathSegInfo->mLstAllocPagePtr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTID에 해당하는 Transaction의 DPath INSERT의 commit 작업을
 *          수행해 준다.
 *
 * Implementation :
 *          1. aTID에 해당하는 DPathEntry 얻기
 *          2. Flush 수행
 *          3. Merge 수행
 *
 * Parameters :
 *      aStatistics     - [IN] 통계
 *      aDathEntry      - [IN] Commit 작업을 수행할 DPathEntry
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::commit( idvSQL  * aStatistics,
                                  void    * aTrans,
                                  void    * aDPathEntry )
{
    SInt            sState = 0;
    sdcDPathEntry * sDPathEntry;

    IDE_DASSERT( aStatistics != NULL );
    IDE_DASSERT( aDPathEntry != NULL );

    // DPathEntry를 얻어온다.
    sDPathEntry = (sdcDPathEntry*)aDPathEntry;

    // 모든 Page를 Flush 한다.
    IDE_TEST_RAISE( sdbDPathBufferMgr::flushAllPage(
                                        aStatistics,
                                        &sDPathEntry->mDPathBuffInfo)
                    != IDE_SUCCESS, commit_failed );

    // DPath INSERT를 수행한 모든 Segment의 변화를 merge해 준다.
    IDE_TEST_RAISE( sdpDPathInfoMgr::mergeAllSegOfDPathInfo(
                                                 aStatistics,
                                                 aTrans,
                                                 &sDPathEntry->mDPathInfo)
                    != IDE_SUCCESS, commit_failed );

    // X$DIRECT_PATH_INSERT - COMMIT_TX_COUNT
    IDE_TEST( mStatMtx.lock( NULL ) != IDE_SUCCESS );
    sState = 1;

    mDPathStat.mCommitTXCnt++;
    mDPathStat.mInsRowCnt += sDPathEntry->mDPathInfo.mInsRowCnt;
    mDPathStat.mAllocBuffPageTryCnt +=
                            sDPathEntry->mDPathBuffInfo.mAllocBuffPageTryCnt;
    mDPathStat.mAllocBuffPageFailCnt +=
                            sDPathEntry->mDPathBuffInfo.mAllocBuffPageFailCnt;
    mDPathStat.mBulkIOCnt += sDPathEntry->mDPathBuffInfo.mBulkIOCnt;

    sState = 0;
    IDE_TEST( mStatMtx.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( commit_failed );
    {
        (void)dumpDPathEntry( sDPathEntry );
        IDE_ASSERT( 0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( mStatMtx.unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTID에 해당하는 Transaction의 DPath INSERT의 abort 작업을
 *          수행해 준다.
 *
 * Implementation :
 *          1. DPathBuffInfo에 페이지 정보를 모두 날림
 *
 * Parameters :
 *      aStatistics     - [IN] 통계
 *      aDPathEntry     - [IN] Abort 작업을 수행할 대상 DPathEntry
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::abort( void * aDPathEntry )
{
    SInt                sState = 0;
    sdcDPathEntry     * sDPathEntry;

    IDE_DASSERT( aDPathEntry != NULL );

    // DPathEntry를 얻어온다.
    sDPathEntry = (sdcDPathEntry*)aDPathEntry;

    IDE_TEST( sdbDPathBufferMgr::cancelAll(&sDPathEntry->mDPathBuffInfo)
              != IDE_SUCCESS );

    // X$DIRECT_PATH_INSERT - ABORT_TX_COUNT
    IDE_TEST( mStatMtx.lock( NULL ) != IDE_SUCCESS );
    sState = 1;

    mDPathStat.mAbortTXCnt++;
    mDPathStat.mAllocBuffPageTryCnt +=
                            sDPathEntry->mDPathBuffInfo.mAllocBuffPageTryCnt;
    mDPathStat.mAllocBuffPageFailCnt +=
                            sDPathEntry->mDPathBuffInfo.mAllocBuffPageFailCnt;
    mDPathStat.mBulkIOCnt += sDPathEntry->mDPathBuffInfo.mBulkIOCnt;

    sState = 0;
    IDE_TEST( mStatMtx.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( mStatMtx.unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTrans에 해당하는 Transaction의 DPathBuffInfo를 반환한다.
 * 
 * Implementation :
 *          1. aTrans에 해당하는 DPathEntry 얻기
 *          2. DPathBuffInfo 확보 후 반환
 *
 * Parameters :
 *      aTrans  - [IN] 찾을 대상 DPathEntry의 Transaction
 *
 * Return : 존재하면 DPathBuffInfo의 포인터, 없으면 NULL
 ******************************************************************************/
void* sdcDPathInsertMgr::getDPathBuffInfo( void *aTrans )
{
    sdcDPathEntry     * sDPathEntry;
    sdbDPathBuffInfo  * sDPathBuffInfo = NULL;

    IDE_DASSERT( aTrans != NULL );

    sDPathEntry = (sdcDPathEntry*)smLayerCallback::getDPathEntry( aTrans );

    if( sDPathEntry != NULL )
    {
        sDPathBuffInfo = &sDPathEntry->mDPathBuffInfo;
    }

    return sDPathBuffInfo;
}

/*******************************************************************************
 * Description : aTrans에 해당하는 DPathInfo를 찾아준다.
 *
 * Parameters :
 *      aTrans    - [IN] 찾을 DPathInfo의 Transaction
 *
 * Return : 존재하면 DPathInfo의 포인터, 없으면 NULL
 ******************************************************************************/
void* sdcDPathInsertMgr::getDPathInfo( void *aTrans )
{
    sdcDPathEntry  * sDPathEntry;
    sdpDPathInfo   * sDPathInfo = NULL;

    IDE_DASSERT( aTrans != NULL );

    sDPathEntry = (sdcDPathEntry*)smLayerCallback::getDPathEntry( aTrans );

    if( sDPathEntry != NULL )
    {
        sDPathInfo = &sDPathEntry->mDPathInfo;
    }

    return sDPathInfo;
}

/*******************************************************************************
 * Description : aTID에 해당하는 DPathInfo를 찾아준다.
 *
 * Parameters :
 *      aTrans      - [IN] 찾을 DPathInfo의 Transaction
 *      aTableOID   - [IN] 찾을 DPathSegInfo의 대상 Table의 OID
 *
 * Return : 존재하면 DPathSegInfo의 포인터, 없으면 NULL
 ******************************************************************************/
void* sdcDPathInsertMgr::getDPathSegInfo( void    * aTrans,
                                          smOID     aTableOID )
{
    sdcDPathEntry    * sDPathEntry;
    sdpDPathInfo     * sDPathInfo;
    sdpDPathSegInfo  * sDPathSegInfo = NULL;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTableOID != SM_NULL_OID );

    sDPathEntry = (sdcDPathEntry*)smLayerCallback::getDPathEntry( aTrans );

    if( sDPathEntry != NULL )
    {
        sDPathInfo = &sDPathEntry->mDPathInfo;
        sDPathSegInfo = sdpDPathInfoMgr::findLastDPathSegInfo( sDPathInfo,
                                                               aTableOID );
    }

    return sDPathSegInfo;
}

/*******************************************************************************
 * Description : X$DIRECT_PATH_INSERT를 위한 통계 값을 반환한다.
 *
 * Parameters :
 *      aDPathStat  - [OUT] DPath 통계 값을 할당하여 반환해줄 OUT 파라미터
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::getDPathStat( sdcDPathStat *aDPathStat )
{
    SInt    sState = 0;

    IDE_DASSERT( aDPathStat != NULL );

    IDE_TEST( mStatMtx.lock( NULL ) != IDE_SUCCESS );
    sState = 1;
    *aDPathStat = mDPathStat;
    sState = 0;
    IDE_TEST( mStatMtx.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( mStatMtx.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : DPathEntry 값을 dump 하기 위한 함수
 *
 * Parameters :
 *      aDPathEntry - [IN] dump할 DPathEntry
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::dumpDPathEntry( sdcDPathEntry *aDPathEntry )
{
    if( aDPathEntry == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "========================================\n"
                     " DPathEntry: NULL\n"
                     "========================================" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "========================================\n"
                     " DPath Entry Dump Begin...\n"
                     "========================================" );

        (void)sdbDPathBufferMgr::dumpDPathBuffInfo(
                                            &aDPathEntry->mDPathBuffInfo );
        (void)sdpDPathInfoMgr::dumpDPathInfo( &aDPathEntry->mDPathInfo );
    }

    ideLog::log( IDE_SERVER_0,
                 "========================================\n"
                 " DPath Entry Dump End...\n"
                 "========================================" );

    return IDE_SUCCESS;
}

/*******************************************************************************
 * Description : aTrans에 달린 DPathEntry 값을 dump 하기 위한 함수
 *
 * Parameters :
 *      aTrans  - [IN] 매달린 DPathEntry를 dump할 Transaction
 ******************************************************************************/
IDE_RC sdcDPathInsertMgr::dumpDPathEntry( void *aTrans )
{
    sdcDPathEntry     * sDPathEntry;

    if( aTrans == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "========================================\n"
                     " Transaction  : NULL\n"
                     "========================================" );
    }
    else
    {
        sDPathEntry = (sdcDPathEntry*)smLayerCallback::getDPathEntry( aTrans );

        (void)dumpDPathEntry( sDPathEntry );
    }

    return IDE_SUCCESS;
}

