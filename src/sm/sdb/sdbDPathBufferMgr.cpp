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
 * $Id: sdbDPathBufferMgr.cpp 91197 2021-07-12 01:15:29Z emlee $
 *
 * Description : DPath Insert를 수행할 때 사용하는 전용 버퍼를 관리한다.
 ******************************************************************************/

#include <smErrorCode.h>
#include <sdbReq.h>
#include <sdbBufferMgr.h>
#include <sdbDPathBFThread.h>
#include <sdbDPathBufferMgr.h>
#include <sddDiskMgr.h>
#include <smriChangeTrackingMgr.h>

iduMemPool sdbDPathBufferMgr::mBCBPool;
iduMemPool sdbDPathBufferMgr::mPageBuffPool;
iduMemPool sdbDPathBufferMgr::mFThreadPool;

iduMutex   sdbDPathBufferMgr::mBuffMtx;
UInt       sdbDPathBufferMgr::mMaxBuffPageCnt;
UInt       sdbDPathBufferMgr::mAllocBuffPageCnt;

/***********************************************************************
 * Description : 초기화 Function으로 세개의 Memory Pool을 초기화한다.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::initializeStatic()
{
    SInt    sState = 0;

    IDE_TEST( mBCBPool.initialize(
                 IDU_MEM_SM_SDB,
                 (SChar*)"DIRECT_PATH_BUFFER_BCB_MEMPOOL",
                 1, /* List Count */
                 ID_SIZEOF( sdbDPathBCB ),
                 1024, /* 생성시 가지고 있는 Item갯수 */
                 IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                 ID_TRUE,							/* UseMutex */
                 IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                 ID_FALSE,							/* ForcePooling */
                 ID_TRUE,							/* GarbageCollection */
                 ID_TRUE, 							/* HwCacheLine */
                 IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/)
              != IDE_SUCCESS );
    sState = 1;

    mAllocBuffPageCnt = 0;
    mMaxBuffPageCnt   = smuProperty::getDPathBuffPageCnt();

    IDE_TEST( mPageBuffPool.initialize(
                 IDU_MEM_SM_SDB,
                 (SChar*)"DIRECT_PATH_BUFFER_PAGE_MEMPOOL",
                 1, /* List Count */
                 SD_PAGE_SIZE,
                 1024,
                 1,
                 ID_TRUE,       /* Use Mutex */
                 SD_PAGE_SIZE, 	/* AlignByte */
                 ID_FALSE,		/* ForcePooling */
                 ID_TRUE,		/* GarbageCollection */
                 ID_TRUE,       /* HWCacheLine */
                 IDU_MEMPOOL_TYPE_TIGHT /* mempool type*/) 
              != IDE_SUCCESS);			
    sState = 2;

    IDE_TEST( mFThreadPool.initialize(
                  IDU_MEM_SM_SDB,
                  (SChar*)"DIRECT_PATH_BUFFER_FLUSH_THREAD_MEMPOOL",
                  1, /* List Count */
                  ID_SIZEOF( sdbDPathBFThread ),
                  32, /* 생성시 가지고 있는 Item갯수 */
                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                  ID_TRUE,							/* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                  ID_FALSE,							/* ForcePooling */
                  ID_TRUE,							/* GarbageCollection */
                  ID_TRUE,                          /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS );			
    sState = 3;

    IDE_TEST( mBuffMtx.initialize( (SChar*)"DIRECT_PATH_BUFFER_POOL_MUTEX",
                                   IDU_MUTEX_KIND_POSIX,
                                   IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 4;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 4:
            IDE_ASSERT( mBuffMtx.destroy() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( mFThreadPool.destroy() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( mPageBuffPool.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mBCBPool.destroy() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 세개의 메모리 Pool을 정리한다.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::destroyStatic()
{
    IDE_TEST( mBCBPool.destroy() != IDE_SUCCESS );
    IDE_TEST( mPageBuffPool.destroy() != IDE_SUCCESS );
    IDE_TEST( mFThreadPool.destroy() != IDE_SUCCESS );

    IDE_TEST( mBuffMtx.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : <aSpaceID, aPageID>에 해당하는 Direct Path Buffer를 만든다.
 *
 * aStatistics  - [IN] 통계정보
 * aSpaceID     - [IN] Space ID
 * aPageID      - [IN] Page ID
 * aDPathInfo   - [IN] DPathBuffInfo
 *
 * aIsLogging   - [IN] Logging Mode 
 * aPagePtr     - [OUT] Page Pointer
 *
 * 알고리즘:
 * 1. <aSpaceID, aPageID>에 해당하는 페이지가 Normal Buffer에 존재할 경우
 *    이 페이지는 Direct Path Buffer로 사용할 수가 없다. 왜냐하면 Normal
 *    Buffer와 Direct Path Buffer에서 동시에 이 페이지에 대해서 접근하여
 *    Consistency가 깨지기 때문이다. *aPagePtr = NULL로 하고 Return한다.
 *    아니면 계속 진행.
 * 2. sDPathBCB = DPathBCB Pool에서 BCB를 할당한다.
 * 3. sDPathBCB를 aSpaceID, aPageID로 초기화수행한다.
 * 4. sDPathBCB.mPage = Direct Buffer Pool로 새로운 Page 를 할당한다.
 * 5. sDPathBCB의 상태를 SDB_DPB_APPEND로 만든다.
 * 6. sDPathBCB를 Transaction의 DirectPathBuffer Info의 Append Page List에
 *    추가한다.
 * 7. aPagePtr = sNewFrameBuf를 해준다.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::createPage( idvSQL           * aStatistics,
                                      void             * aTrans,
                                      scSpaceID          aSpaceID,
                                      scPageID           aPageID,
                                      idBool             aIsLogging,
                                      UChar**            aPagePtr )
{
    SInt                sState = 0;
    smTID               sTID;
    idBool              sIsExist;
    sdbDPathBCB        *sNewBCB;
    sdbFrameHdr        *sFrameHdr;
    sdbDPathBuffInfo   *sDPathBuffInfo;

    IDE_DASSERT( aTrans   != NULL );
    IDE_DASSERT( aSpaceID <= SC_MAX_SPACE_COUNT - 1);
    IDE_DASSERT( aPageID  != SD_NULL_PID );
    IDE_DASSERT( aPagePtr != NULL );

    *aPagePtr = NULL;
    sIsExist  = ID_FALSE;

    sTID = smLayerCallback::getTransID( aTrans );
    sDPathBuffInfo = (sdbDPathBuffInfo*)smLayerCallback::getDPathBuffInfo(
                                                                    aTrans );
    if( sDPathBuffInfo == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "Trans ID  : %u\n"
                     "Space ID  : %u\n"
                     "Page ID   : %u",
                     sTID,
                     aSpaceID,
                     aPageID );
        (void)smLayerCallback::dumpDPathEntry( aTrans );

        IDE_ASSERT( 0 );
    }

    /* Step 1. <aSpaceID, aPageID>에 해당하는 페이지가 Normal Buffer에
     * 존재하면 제거한다.  */
    IDE_TEST( sdbBufferMgr::removePageInBuffer( aStatistics,
                                                aSpaceID,
                                                aPageID )
              != IDE_SUCCESS );

    /* Buffer에서 제거했으므로 제거되었는지 확인한다. 부하가 클것으로
     * 생각되어 Debug일때만 체크한다.
     * Normal Buffer와 Direct Path Buffer에서 동시에 같은
     * 페이지가 있는 경우 Consistency가 깨질수 있다.
     * 예) 동시에 Disk에 페이지가 기록되는 경우 */
    IDE_DASSERT( sdbBufferMgr::isPageExist( aStatistics,
                                            aSpaceID,
                                            aPageID ) == ID_FALSE );
    
    if ( sIsExist == ID_FALSE )
    {
        sDPathBuffInfo->mTotalPgCnt++;

        /* Step 2. DPathBCB Pool에서 BCB를 할당한다 */
        /* sdbDPathBufferMgr_createPage_alloc_NewBCB.tc */
        IDU_FIT_POINT("sdbDPathBufferMgr::createPage::alloc::NewBCB");
        IDE_TEST( mBCBPool.alloc( (void**)&sNewBCB ) != IDE_SUCCESS );
        sState = 1;

        /* Step 3. BCB를 초기화한다. */
        sNewBCB->mIsLogging     = aIsLogging;
        sNewBCB->mSpaceID       = aSpaceID;
        sNewBCB->mPageID        = aPageID;
        sNewBCB->mDPathBuffInfo = sDPathBuffInfo;
    
        SMU_LIST_INIT_NODE(&(sNewBCB->mNode));
        sNewBCB->mNode.mData = sNewBCB;

        /* Step 4. Page Buffer를 할당하여 BCB에 매단다. */
        IDE_TEST( allocBuffPage( aStatistics,
                                 sDPathBuffInfo,
                                 (void**)&(sNewBCB->mPage) )
                  != IDE_SUCCESS );
        sState = 2;

        sFrameHdr = (sdbFrameHdr*) ( sNewBCB->mPage );
        sFrameHdr->mBCBPtr = sNewBCB;
        sFrameHdr->mSpaceID = aSpaceID;

        /* Step 5. BCB를 Append상태로 바꾼다. */
        sNewBCB->mState   = SDB_DPB_APPEND;

        /* Step 6. Append Page List에 뒤에 추가한다. */
        SMU_LIST_ADD_LAST(&(sDPathBuffInfo->mAPgList),
                          &(sNewBCB->mNode));

        /* Step 7. */
        *aPagePtr = sNewBCB->mPage;
    }
    else
    {
        /* Normal Buffer와 Direct Path Buffer에서 동시에 같은
         * 페이지가 있는 경우 Consistency가 깨질수 있다.
         * 예) 동시에 Disk에 페이지가 기록되는 경우
         * */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 2:
            IDE_ASSERT( freeBuffPage( sNewBCB->mPage ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mBCBPool.memfree( sNewBCB ) == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPagePtr에 해당하는 BCB의 상태를 SDB_DPB_DIRTY로 만들고
 *               Flush Request List에 등록한다.
 *
 * aPagePtr - [IN] Page Pointer
 *
 * 알고리즘
 * 1. aPagePtr에 해당하는 BCB를 찾는다.
 * 2. APPEND List에서 제거한다.
 * 3. BCB의 상태를 SDB_DPB_DIRTY로 만든다.
 * 4. Flush Request List에 추가한다.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::setDirtyPage( void* aPagePtr )
{
    SInt              sState = 0;
    sdbDPathBCB*      sBCB;
    sdbDPathBuffInfo *sDPathBCBInfo;
    sdbFrameHdr      *sFrameHdr;

    IDE_DASSERT( aPagePtr != NULL );


    /* 1. 페이지의 시작위치 앞에 BCB의 Pointer저장되어 있다 .*/
    sFrameHdr = (sdbFrameHdr*) aPagePtr;
    sBCB = (sdbDPathBCB*)(sFrameHdr->mBCBPtr);

    sDPathBCBInfo = sBCB->mDPathBuffInfo;

    /* Transaction의 Flush Request List는 Direct Path Flush Thread와
     * 동시에 접근하는 리스트이기때문에 반드시 Mutex를 잡아야 한다.*/
    IDE_TEST( sDPathBCBInfo->mMutex.lock( NULL /* idvSQL* */ )
              != IDE_SUCCESS );
    sState = 1;

    /* 2. APPEND List에서 제거한다. */
    SMU_LIST_DELETE( &(sBCB->mNode) );

    /* 3. BCB의 상태를 SDB_DPB_DIRTY로 만든다. */
    sBCB->mState = SDB_DPB_DIRTY;

    /* 4. Flush Request List에 추가한다. */
    SMU_LIST_ADD_LAST(&(sDPathBCBInfo->mFReqPgList),
                      &(sBCB->mNode));

    sState = 0;
    IDE_TEST( sDPathBCBInfo->mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( sDPathBCBInfo->mMutex.unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aDPathBuffInfo가 생성한 모든 Direct Path Buffer를
 *               디스크에 내린다. 이는 Transaction의 Commit전에 호출한다.
 *
 * aTrans - [IN] Transaction Pointer
 *
 * 알고리즘
 * 1. aTrans로 부터 DPBI(Direct Path Buffer Info)를 구한다.
 * 2. APPEND List에는 페이지를 Disk에 내린다.
 * 3. Flush List에 있는 페이지중 BCB의 상태가 Dirty인것의 Flag를 FLUSH로
 *    바꾸고 직접내린다.
 * 4. Flush Request List가 완전히 빌때까지 기다린다. 즉 Flush Thread가
 *    IO작업을 완료할때까지 기다린다.
 *
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::flushAllPage( idvSQL            * aStatistics,
                                        sdbDPathBuffInfo  * aDPathBuffInfo )
{
    UInt                sState = 0;
    sdbDPathBulkIOInfo  sDPathBulkIOInfo;

    IDE_DASSERT( aDPathBuffInfo != NULL );

    IDE_TEST( sdbDPathBufferMgr::initBulkIOInfo( &sDPathBulkIOInfo )
              != IDE_SUCCESS );
    sState = 1;

    //---------------------------------------------------------------
    // Flush Thread를 먼저 종료시킨다.
    //---------------------------------------------------------------
    IDE_TEST( destFlushThread( aDPathBuffInfo ) != IDE_SUCCESS );

    //---------------------------------------------------------------------
    // DPathBuffInfo에 있는 모든 페이지들에 대하여 Flush를 수행한다.
    //---------------------------------------------------------------------
    IDE_TEST( flushBCBInDPathInfo( aStatistics,
                                   aDPathBuffInfo,
                                   &sDPathBulkIOInfo )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdbDPathBufferMgr::destBulkIOInfo( &sDPathBulkIOInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            // BulkIOInfo에 매달려 있던 Page들도 전부 free 해준다.
            IDE_ASSERT( freeBCBInList( &sDPathBulkIOInfo.mBaseNode )
                        == IDE_SUCCESS );
            IDE_ASSERT( sdbDPathBufferMgr::destBulkIOInfo( &sDPathBulkIOInfo )
                        == IDE_SUCCESS );
        default:
            break;
    }

    // 본 함수가 호출 되면 flush가 됐든, 실패하여 free하게 되든
    // DPathBuffInfo에 달려있는 BCB와 Page들을 free해주고 나가도록 한다.
    IDE_ASSERT( freeBCBInDPathInfo( aDPathBuffInfo ) == IDE_SUCCESS );
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aDPathBuffInfo의 모든 Direct Path Buffer 페이지의
 *               Flush 요청을 모두 최소한다. 이는 Rollback시에 호출한다.
 *
 * aDPathBuffInfo  - [IN] Direct Path Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::cancelAll( sdbDPathBuffInfo  * aDPathBuffInfo )
{
    //---------------------------------------------------------------
    // Flush Thread를 먼저 종료시킨다.
    //---------------------------------------------------------------
    IDE_TEST( destFlushThread( aDPathBuffInfo )
                != IDE_SUCCESS );

    IDE_TEST( freeBCBInDPathInfo( aDPathBuffInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Buffer Info에 있는 모든 BCB에 대해 Flush를
 *               수행하고 완료되면 BCB와 할당된 Buffer Page를 Free시킨다.
 *               이때 aNeedWritePage가 ID_TRUE면 Flush를 수행하고 ID_FALSE
 *               이면 Free만한다.
 *
 * aDPathBCBInfo   - [IN] Direct Path Buffer Info
 * aNeedWritePage  - [IN] if ID_TRUE, BCB의 페이지에 대해 WritePage수행,
 *                        아니면 writePage를 수행하지 않고 단지 Free만
 *                        시킨다.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::flushBCBInDPathInfo(
    idvSQL             * aStatistics,
    sdbDPathBuffInfo   * aDPathBCBInfo,
    sdbDPathBulkIOInfo * aDPathBulkIOInfo )
{
    IDE_DASSERT( aDPathBCBInfo != NULL );
    IDE_DASSERT( aDPathBulkIOInfo != NULL );

    scSpaceID   sNeedSyncSpaceID = 0;

    if( aDPathBCBInfo->mTotalPgCnt != 0 )
    {
        /* Append List에 있는 모든 Page를 write 한다. */
        IDE_TEST( flushBCBInList( aStatistics,
                                  aDPathBCBInfo,
                                  aDPathBulkIOInfo,
                                  &(aDPathBCBInfo->mAPgList),
                                  &sNeedSyncSpaceID )
                  != IDE_SUCCESS );
        /* Flush Request List에 있는 모든 페이지를 write한다 */
        IDE_TEST( flushBCBInList( aStatistics,
                                  aDPathBCBInfo,
                                  aDPathBulkIOInfo,
                                  &(aDPathBCBInfo->mFReqPgList),
                                  &sNeedSyncSpaceID )
                  != IDE_SUCCESS );

        /* Bulk IO이기때문에 아직 버퍼가 차지 않아서 Disk에 기록하지
         * 않은 페이지가 있을수 있다. 때문에 write를 요청한다.*/
        IDE_TEST( writePagesByBulkIO( aStatistics,
                                      aDPathBCBInfo,
                                      aDPathBulkIOInfo,
                                      &sNeedSyncSpaceID )
                  != IDE_SUCCESS );

        if( sNeedSyncSpaceID != 0 )
        {
            IDE_TEST( sddDiskMgr::syncTBSInNormal( aStatistics,
                                                   sNeedSyncSpaceID )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aBaseNode의 BCB에 대해 Flush작업을 한다.
 * 
 * 알고리즘
 * + aBaseNode의 각각의 Node에 다음과 같은 연산을 수행한다.
 *  1. 현재 Node의 BCB의 상태가 SDB_DPB_FLUSH이 아니면
 *   1.1 aFlushPage가 ID_TRUE면
 *    - BCB상태를 SDB_DPB_FLUSH로 바꾼다.
 *    - BCB가 가리키는 페이지를 Disk에 기록한다.
 *   1.2 aBaseNode의 리스트에서 현재 BCB를 제거한다.
 *   1.3 BCB와 BCB의 할당된 Buffer Page를 Free한다.

 *
 * aDPathBCBInfo    - [IN] Direct Path Buffer Info
 * aDPathBulkIOInfo - [IN] Direct Path Bulk IO Info
 * aBaseNode        - [IN] Flush작업 및 Free할 List의 BaseNode
 * aFlushPage       - [IN] if ID_TRUE, BCB의 페이지에 대해 WritePage수행, 아니면
 *                         WritePage를 수행하지 않고 단지 Free만 시킨다.
 * aNeedSyncSpaceID - [IN-OUT] Flush해야할 TableSpaceID를 가리킨다. 이 값이
 *                        현재 기록해야할 BCB의 TableSpaceID와 다를때
 *                        aNeedSyncSpaceID가 가리키는 TableSpace를 Flush한다.
 *                        그리고 aNeedSyncSpaceID를 BCB의 TableSpaceID로
 *                        갱신한다.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::flushBCBInList(
    idvSQL*              aStatistics,
    sdbDPathBuffInfo*    aDPathBCBInfo,
    sdbDPathBulkIOInfo*  aDPathBulkIOInfo,
    smuList*             aBaseNode,
    scSpaceID*           aNeedSyncSpaceID )
{
    SInt          sState = 0;
    smuList      *sCurNode;
    smuList      *sNxtNode;
    sdbDPathBCB  *sCurBCB;

    IDE_DASSERT( aDPathBCBInfo != NULL );
    IDE_DASSERT( aBaseNode != NULL );

    IDE_TEST( aDPathBCBInfo->mMutex.lock( NULL /* idvSQL* */ )
              != IDE_SUCCESS );
    sState = 1;

    sCurNode  = SMU_LIST_GET_FIRST(aBaseNode);
    while( sCurNode != aBaseNode )
    {
        sCurBCB = (sdbDPathBCB*)sCurNode->mData;
        sNxtNode = SMU_LIST_GET_NEXT( sCurNode );

        /* List에서 제거한다. */
        SMU_LIST_DELETE( sCurNode );

        /* 현재 BCB에 대해 Flush를 수행중이라고 표시한다. */
        sCurBCB->mState = SDB_DPB_FLUSH;

        if( isNeedBulkIO( aDPathBulkIOInfo, sCurBCB ) == ID_TRUE )
        {
            sState = 0;
            IDE_TEST( aDPathBCBInfo->mMutex.unlock() != IDE_SUCCESS );

            /* Bulk IO를 수행한다. 동시에 리스트에서 제거하고 free 한다. */
            IDE_TEST( writePagesByBulkIO( aStatistics,
                                          aDPathBCBInfo,
                                          aDPathBulkIOInfo,
                                          aNeedSyncSpaceID )
                      != IDE_SUCCESS );

            IDE_TEST( aDPathBCBInfo->mMutex.lock( NULL /* idvSQL* */ )
                      != IDE_SUCCESS );
            sState = 1;
            /* Mutex풀기전에 구한 sNxtNode이 Valid하다는 것을 보장할 수 없다.
             * 왜냐하면 이 리스트는 Transactoin과 Flush Thread가 동시에
             * 접근해서 IO를 수행하여 Mutex를 푼사이에 다른 Thread가
             * 그 nextnode에 대해서 작업을 완료하여 free시켜버릴수도 있기때문이다.*/
            sNxtNode  = SMU_LIST_GET_FIRST(aBaseNode);
        }

        IDE_TEST( addNode4BulkIO( aDPathBulkIOInfo,
                                  sCurNode )
                    != IDE_SUCCESS );

        sCurNode = sNxtNode;
    }

    sState = 0;
    IDE_TEST( aDPathBCBInfo->mMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( aDPathBCBInfo->mMutex.unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Buffer Info에 있는 모든 BCB에 대해 Free를
 *               수행 한다.
 *
 * aDPathBCBInfo   - [IN] Direct Path Buffer Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::freeBCBInDPathInfo(
    sdbDPathBuffInfo   * aDPathBCBInfo )
{
    SInt          sState = 0;

    IDE_DASSERT( aDPathBCBInfo != NULL );

    if( aDPathBCBInfo->mTotalPgCnt != 0 )
    {
        IDE_TEST( aDPathBCBInfo->mMutex.lock( NULL /* idvSQL* */ )
                != IDE_SUCCESS );
        sState = 1;

        /* Append List에 있는 모든 Page를 free 한다. */
        IDE_TEST( freeBCBInList( &(aDPathBCBInfo->mAPgList) )
                  != IDE_SUCCESS );
        /* Flush Request List에 있는 모든 페이지를 free 한다 */
        IDE_TEST( freeBCBInList( &(aDPathBCBInfo->mFReqPgList) )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( aDPathBCBInfo->mMutex.unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( aDPathBCBInfo->mMutex.unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aBaseNode의 BCB에 대해 Free작업을 한다.
 *
 * aBaseNode        - [IN] Flush작업 및 Free할 List의 BaseNode
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::freeBCBInList( smuList* aBaseNode )
{
    smuList      *sCurNode;
    sdbDPathBCB  *sCurBCB;

    IDE_DASSERT( aBaseNode != NULL );

    for( sCurNode = SMU_LIST_GET_FIRST(aBaseNode);
         sCurNode != aBaseNode;
         sCurNode = SMU_LIST_GET_FIRST(aBaseNode) ) /* BUG-30076 */
    {
        sCurBCB = (sdbDPathBCB*)sCurNode->mData;

        /* List에서 제거한다. */
        SMU_LIST_DELETE( sCurNode );

        /* BCB와 할당된 버퍼를 Free한다 */
        IDE_TEST( freeBuffPage( sCurBCB->mPage )
                  != IDE_SUCCESS );
        IDE_TEST( mBCBPool.memfree( sCurBCB ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Info에 대해서 백그라운드로 Flush할 Flush Thread
 *               생성 및 시작시킨다.
 *
 * aDPathBCBInfo - [IN] Direct Path Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::createFlushThread( sdbDPathBuffInfo*  aDPathBCBInfo )
{
    SInt    sState = 0;
    IDE_DASSERT( aDPathBCBInfo != NULL );

    /* sdbDPathBufferMgr_createFlushThread_alloc_FlushThread.tc */
    IDU_FIT_POINT("sdbDPathBufferMgr::createFlushThread::alloc::FlushThread");
    IDE_TEST( mFThreadPool.alloc((void**)&(aDPathBCBInfo->mFlushThread))
              != IDE_SUCCESS );
    sState = 1;

    new (aDPathBCBInfo->mFlushThread) sdbDPathBFThread();

    IDE_TEST( aDPathBCBInfo->mFlushThread->initialize( aDPathBCBInfo )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( aDPathBCBInfo->mFlushThread->startThread()
              != IDE_SUCCESS );
    sState = 3;

    IDU_FIT_POINT_RAISE( "1.PROJ-2068@sdbDPathBufferMgr::createFlushThread", err_ART );

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION(err_ART);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ART));
    }
#endif
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 3:
            IDE_ASSERT( aDPathBCBInfo->mFlushThread->shutdown()
                        == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( aDPathBCBInfo->mFlushThread->destroy()
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mFThreadPool.memfree( aDPathBCBInfo->mFlushThread )
                        == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Info의 Flush Thread를 종료시킨다.
 *
 * aDPathBCBInfo - [IN] Direct Path Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::destFlushThread( sdbDPathBuffInfo*  aDPathBCBInfo )
{
    IDE_DASSERT( aDPathBCBInfo != NULL );

    // BUG-30216 destFlushThread 함수가 동일 TX 내에서 여러번 불릴 수 있기
    // 때문에 Flush Thread가 존재 할 때만 파괴해 준다.
    if( aDPathBCBInfo->mFlushThread != NULL )
    {
        IDE_TEST( aDPathBCBInfo->mFlushThread->shutdown()
                != IDE_SUCCESS );

        IDE_TEST( aDPathBCBInfo->mFlushThread->destroy()
                != IDE_SUCCESS );

        IDE_TEST( mFThreadPool.memfree( aDPathBCBInfo->mFlushThread )
                != IDE_SUCCESS );

        aDPathBCBInfo->mFlushThread = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Info를 초기화한다.
 *
 * aDPathBCBInfo - [IN-OUT] Direct Path Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::initDPathBuffInfo( sdbDPathBuffInfo *aDPathBuffInfo )
{
    SInt    sState = 0;

    IDE_ASSERT( aDPathBuffInfo != NULL );

    aDPathBuffInfo->mTotalPgCnt = 0;

    SMU_LIST_INIT_BASE(&(aDPathBuffInfo->mAPgList));
    SMU_LIST_INIT_BASE(&(aDPathBuffInfo->mFReqPgList));

    IDE_TEST( aDPathBuffInfo->mMutex.initialize(
                    (SChar*)"DIRECT_PATH_BUFFER_INFO_MUTEX",
                    IDU_MUTEX_KIND_NATIVE,
                    IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( createFlushThread( aDPathBuffInfo )
              != IDE_SUCCESS );
    sState = 2;

    aDPathBuffInfo->mAllocBuffPageTryCnt = 0;
    aDPathBuffInfo->mAllocBuffPageFailCnt = 0;
    aDPathBuffInfo->mBulkIOCnt = 0;

    IDU_FIT_POINT_RAISE( "1.PROJ-2068@sdbDPathBufferMgr::initDPathBuffInfo", err_ART );

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION(err_ART);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ART));
    }
#endif
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 2:
            IDE_ASSERT( destFlushThread( aDPathBuffInfo ) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( aDPathBuffInfo->mMutex.destroy() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Info를 소멸시킨다.
 *
 * aDPathBCBInfo - [IN] Direct Path Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::destDPathBuffInfo( sdbDPathBuffInfo *aDPathBuffInfo )
{
    IDE_DASSERT( aDPathBuffInfo != NULL );

    IDE_TEST( destFlushThread( aDPathBuffInfo )
              != IDE_SUCCESS );

    if( !SMU_LIST_IS_EMPTY(&(aDPathBuffInfo->mAPgList)) ||
        !SMU_LIST_IS_EMPTY(&(aDPathBuffInfo->mFReqPgList)) )
    {
        (void)dumpDPathBuffInfo( aDPathBuffInfo );
        IDE_ASSERT( 0 );
    }

    IDE_TEST( aDPathBuffInfo->mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Bulk IO에 대한 structure를 초기화한다.
 *
 * aDPathBulkIOInfo - [IN] Direct Path Bulk IO Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::initBulkIOInfo( sdbDPathBulkIOInfo *aDPathBulkIOInfo )
{
    /* Bulk IO를 수행할때 한번에 기록될 페이지의 갯수를 가져온다. */
    aDPathBulkIOInfo->mDBufferSize =
        smuProperty::getBulkIOPageCnt4DPInsert();

    aDPathBulkIOInfo->mIORequestCnt = 0;

    aDPathBulkIOInfo->mLstSpaceID   = SC_NULL_SPACEID;
    aDPathBulkIOInfo->mLstPageID    = SC_NULL_PID;

    /* Bulk IO를 수행할때 한번에 기록하기 위해 필요한 버퍼공간을 할당한다 . */
    IDE_TEST( iduFile::allocBuff4DirectIO(
                  IDU_MEM_SM_SDB,
                  aDPathBulkIOInfo->mDBufferSize * SD_PAGE_SIZE,
                  (void**)&( aDPathBulkIOInfo->mRDIOBuffPtr ),
                  (void**)&( aDPathBulkIOInfo->mADIOBuffPtr ) )
              != IDE_SUCCESS );

    SMU_LIST_INIT_BASE(&(aDPathBulkIOInfo->mBaseNode));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Bulk IO에 대한 structure를 정리한다.
 *
 * aDPathBulkIOInfo - [IN] Direct Path Bulk IO Info
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::destBulkIOInfo( sdbDPathBulkIOInfo *aDPathBulkIOInfo )
{
    if( aDPathBulkIOInfo->mRDIOBuffPtr == NULL ||
        aDPathBulkIOInfo->mADIOBuffPtr == NULL ||
        !SMU_LIST_IS_EMPTY(&aDPathBulkIOInfo->mBaseNode) )
    {
        (void)dumpDPathBulkIOInfo( aDPathBulkIOInfo );
        IDE_ASSERT( 0 );
    }

    IDE_TEST( iduMemMgr::free( aDPathBulkIOInfo->mRDIOBuffPtr )
              != IDE_SUCCESS );
    aDPathBulkIOInfo->mRDIOBuffPtr = NULL;
    aDPathBulkIOInfo->mADIOBuffPtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aBCB를 Bulk IO Buffer에 추가가능한지 조사한다. 불가하면
 *               Buffer에 있는 것을 내려야 한다. 때문에 ID_TRUE, 아니면
 *               ID_FALSE.
 *
 * aDPathBulkIOInfo - [IN] Direct Path Bulk IO Info
 * aBCB             - [IN] aDPathBulkIOInfo의 mBaseNode에 추가될 BCB
 **********************************************************************/
idBool sdbDPathBufferMgr::isNeedBulkIO( sdbDPathBulkIOInfo *aDPathBulkIOInfo,
                                        sdbDPathBCB        *aBCB )
{
    idBool  sIsNeed = ID_TRUE;

    /* 다음과 같은 경우 Bulk IO를 수행한다.
     * 1. Bulk IO Buffer가 Full
     * 2. 연속된 페이지가 아닌경우
     *   - SpaceID가 틀린경우
     *   - PageID가 이전에 Add한 페이지와 연속되지 않을 때 */
    if( ( aDPathBulkIOInfo->mIORequestCnt >= aDPathBulkIOInfo->mDBufferSize ) ||
        ( (aBCB->mSpaceID != aDPathBulkIOInfo->mLstSpaceID) &&
          (aDPathBulkIOInfo->mLstSpaceID != 0 )) ||
        ( (aBCB->mPageID  != aDPathBulkIOInfo->mLstPageID + 1) &&
          (aDPathBulkIOInfo->mLstPageID != 0) ) )
    {
        sIsNeed = ID_TRUE;
    }
    else
    {
        sIsNeed = ID_FALSE;
    }

    return sIsNeed;
}

/***********************************************************************
 * Description : Bulk IO Info에 IO Request를 추가한다.
 *
 * aDPathBCBInfo - [IN] Direct Path Info
 * aDPathBCB     - [IN] Direct Path BCB Pointer
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::addNode4BulkIO( sdbDPathBulkIOInfo *aDPathBulkIOInfo,
                                          smuList            *aFlushNode )
{
    sdbDPathBCB  *sCurBCB;

    IDE_DASSERT( aDPathBulkIOInfo != NULL );
    IDE_DASSERT( aFlushNode != NULL );

    sCurBCB  = (sdbDPathBCB*)aFlushNode->mData;

    SMU_LIST_ADD_LAST( &( aDPathBulkIOInfo->mBaseNode ),
                       aFlushNode );

    aDPathBulkIOInfo->mLstSpaceID = sCurBCB->mSpaceID;
    aDPathBulkIOInfo->mLstPageID = sCurBCB->mPageID;
    aDPathBulkIOInfo->mIORequestCnt++;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Bulk IO를 수행한다.
 *
 * aDPathBulkIOInfo - [IN] Direct Path Bulk IO Info
 * aPrevSpaceID     - 이전에 writePage를 수행한 BCB의 TableSpace의 ID
 *                    return시 현재 BCB의 SpaceID로 변경.
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::writePagesByBulkIO(
                                        idvSQL             *aStatistics,
                                        sdbDPathBuffInfo   *aDPathBuffInfo,
                                        sdbDPathBulkIOInfo *aDPathBulkIOInfo,
                                        scSpaceID          *aPrevSpaceID )
{
    smLSN               sLstLSN;
    UChar               *sBuffFrame;
    scSpaceID           sSpaceID;
    scPageID            sPageID;
    smuList             *sCurNode;
    smuList             *sNxtNode;
    sdbDPathBCB         *sCurBCB;
    scGRID              sPageGRID;
    UInt                sIBChunkID;
    UInt                i;
    sddTableSpaceNode   *sSpaceNode;  
    sddDataFileNode     *sFileNode;

    if( aDPathBulkIOInfo->mIORequestCnt > 0 )
    {
        sCurNode  = SMU_LIST_GET_FIRST( &(aDPathBulkIOInfo->mBaseNode) );
        sCurBCB   = (sdbDPathBCB*)sCurNode->mData;

        /* sddDiskMgr::syncTableSpaceInNormal의 호출횟수를 줄이기 위해서
         * writePage때마다 하지 않지 않고 writePage시 이전에 writePage를
         * 요청한 BCB의 SpaceID와 현재 writePage를 요청한 BCB의 SpaceID가
         * 다를때만 syncTableSpaceInNormal를 수행한다. 그리고 마지막
         * writePage후 aPrevSpaceID가 가리키는 TableSpace에 대해서
         * syncTableSpaceInNormal를 수행한다. */
        if( *aPrevSpaceID != 0 )
        {
            if( *aPrevSpaceID != sCurBCB->mSpaceID )
            {
                IDE_TEST( sddDiskMgr::syncTBSInNormal( aStatistics, *aPrevSpaceID )
                          != IDE_SUCCESS );
            }
        }

        *aPrevSpaceID = sCurBCB->mSpaceID;

        IDU_FIT_POINT( "3.PROJ-1665@sdbDPathBufferMgr::writePagesByBulkIO" );

        sSpaceID = sCurBCB->mSpaceID;
        sPageID  = sCurBCB->mPageID;

        /* 이 페이지에 대해서 로깅을 하지 않을 경우 redo시 LSN이
         * Page가 write되는 시점의 System LSN보다 LSN이 작게 설정되면
         * 이전에 이 Page에 update 사용된 Log가 Redo가 될 수 있다.
         * 때문에 LSN은 현재 write시점의 system last lsn으로 설정한다. */
        smLayerCallback::getLstLSN( &sLstLSN );

        for( i = 0; i < aDPathBulkIOInfo->mIORequestCnt; i++ )
        {
            if ( sCurBCB->mIsLogging == ID_TRUE ) 
            {
                /* PROJ-1665 : Page의 Physical Image를 Logging한다 */
                SC_MAKE_GRID(sPageGRID,sCurBCB->mSpaceID,sCurBCB->mPageID,0);

                IDE_TEST( smLayerCallback::writeDPathPageLogRec( aStatistics,
                                                                 sCurBCB->mPage,
                                                                 sPageGRID,
                                                                 &sLstLSN )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }

            sBuffFrame = sCurBCB->mPage;

            smLayerCallback::setPageLSN( sBuffFrame, &sLstLSN );
            smLayerCallback::calcAndSetCheckSum( sBuffFrame );

            idlOS::memcpy( aDPathBulkIOInfo->mADIOBuffPtr + (SD_PAGE_SIZE * i),
                           sBuffFrame,
                           SD_PAGE_SIZE );

            sNxtNode = SMU_LIST_GET_NEXT( sCurNode );
            SMU_LIST_DELETE( sCurNode );

            /* BCB와 할당된 버퍼를 Free한다 */
            IDE_TEST( freeBuffPage( sCurBCB->mPage )
                      != IDE_SUCCESS );

            IDE_TEST( mBCBPool.memfree( sCurBCB ) != IDE_SUCCESS );

            sCurNode = sNxtNode;
            sCurBCB  = (sdbDPathBCB*)sCurNode->mData;
        }

        /* Bulk IO를 수행한다. */

        IDE_TEST( sddDiskMgr::writeMultiPage( aStatistics,
                                              sSpaceID,
                                              sPageID,
                                              aDPathBulkIOInfo->mIORequestCnt,
                                              aDPathBulkIOInfo->mADIOBuffPtr )
                  != IDE_SUCCESS );
        
        if ( smLayerCallback::isCTMgrEnabled() == ID_TRUE )
        {
            
            IDE_ASSERT( sctTableSpaceMgr::isTempTableSpace( sSpaceID ) 
                        != ID_TRUE );

            IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sSpaceID,
                                                                (void**)&sSpaceNode)
                      != IDE_SUCCESS );

            IDE_ASSERT( sSpaceNode->mHeader.mID == sSpaceID );

            IDE_TEST( sddTableSpace::getDataFileNodeByPageID(sSpaceNode,
                                                             sPageID,
                                                             &sFileNode,
                                                             ID_TRUE ) /* aFatal */
              != IDE_SUCCESS );

            for( i = 0; i < aDPathBulkIOInfo->mIORequestCnt; i++ )
            {
                sIBChunkID = 
                   smriChangeTrackingMgr::calcIBChunkID4DiskPage( sPageID + i );

                IDE_TEST( smriChangeTrackingMgr::changeTracking( sFileNode,
                                                                 NULL,
                                                                 sIBChunkID )
                          != IDE_SUCCESS );
            }
        }

        aDPathBulkIOInfo->mIORequestCnt = 0;

        // X$DIRECT_PATH_INSERT - BULK_IO_COUNT
        aDPathBuffInfo->mBulkIOCnt++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Buffer를 할당한다. mPageBuffPool은 모든
 *               Direct Path Insert를 수행하는 Transaction들이 공유한다. 때문에
 *               동시성 제어가 필요하고 무한정 공간이 늘어나면 되지 않기때문에
 *               DIRECT_PATH_BUFFER_PAGE_COUNT라는 속성을 두고 이 값 이상으로
 *               메모리가 할당되지 않도록 하였다. 이 속성은 Alter구문을 이용해서
 *               바꿀 수가 있다. 할당되는 메모리 크기는 SD_PAGE_SIZE이다.
 *
 * aPage - [OUT] 할당된 메모리를 받는다.
 *
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::allocBuffPage( idvSQL            * aStatistics,
                                         sdbDPathBuffInfo  * aDPathBuffInfo,
                                         void             ** aPage )
{
    SInt             sState = 0;
    SInt             sAllocState = 0;
    PDL_Time_Value   sTV;

    sTV.set(0, smuProperty::getDPathBuffPageAllocRetryUSec() );

    while(1)
    {
        IDE_TEST( mBuffMtx.lock( NULL /* idvSQL* */) != IDE_SUCCESS );
        sState = 1;

        // X$DIRECT_PATH_INSERT - TRY_ALLOC_BUFFER_PAGE_COUNT
        aDPathBuffInfo->mAllocBuffPageTryCnt++;

        if( mAllocBuffPageCnt < mMaxBuffPageCnt )
        {
            break;
        }

        sState = 0;
        IDE_TEST( mBuffMtx.unlock() != IDE_SUCCESS );

        // X$DIRECT_PATH_INSERT - FAIL_ALLOC_BUFFER_PAGE_COUNT
        aDPathBuffInfo->mAllocBuffPageFailCnt++;

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

        idlOS::sleep( sTV );
    }

    mAllocBuffPageCnt++;
    IDE_TEST( mPageBuffPool.alloc( aPage ) != IDE_SUCCESS );
    sAllocState = 1;

    sState = 0;
    IDE_TEST( mBuffMtx.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sState != 0 )
    {
        IDE_ASSERT( mBuffMtx.unlock() == IDE_SUCCESS );
    }
    if( sAllocState != 0 )
    {
        IDE_ASSERT( mPageBuffPool.memfree( aPage ) == IDE_SUCCESS );
        mAllocBuffPageCnt--;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Direct Path Buffer를 반환한다.
 *
 * aPage - [IN] 반환될 메모리 페이지 시작주소.
 *
 **********************************************************************/
IDE_RC sdbDPathBufferMgr::freeBuffPage( void* aPage )
{
    SInt    sState = 0;

    IDE_TEST( mBuffMtx.lock( NULL /* idvSQL* */) != IDE_SUCCESS );
    sState = 1;

    IDE_ASSERT( mAllocBuffPageCnt != 0 );

    mAllocBuffPageCnt--;
    IDE_TEST( mPageBuffPool.memfree( aPage ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( mBuffMtx.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( mBuffMtx.unlock() == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : DPathBuffInfo를 dump한다.
 *
 * Parameters :
 *      aDPathBuffInfo - [IN] dump할 DPathBuffInfo
 ******************************************************************************/
IDE_RC sdbDPathBufferMgr::dumpDPathBuffInfo( sdbDPathBuffInfo *aDPathBuffInfo )
{
    smuList   * sBaseNode;
    smuList   * sCurNode;

    if( aDPathBuffInfo == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     " DPath Buff Info: NULL\n"
                     "-----------------------" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     "  DPath Buffer Info Dump...\n"
                     "Total Page Count  : %llu\n"
                     "Last Space ID     : %u\n"
                     "-----------------------",
                     aDPathBuffInfo->mTotalPgCnt,
                     aDPathBuffInfo->mLstSpaceID );


        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     "  Append Page List Dump...\n"
                     "-----------------------" );

        sBaseNode = &aDPathBuffInfo->mAPgList;
        for( sCurNode = SMU_LIST_GET_FIRST(sBaseNode);
             sCurNode != sBaseNode;
             sCurNode = SMU_LIST_GET_NEXT(sCurNode) )
        {
            (void)dumpDPathBCB( (sdbDPathBCB*)sCurNode->mData );
        }

        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     "  Flush Request Page List Dump...\n"
                     "-----------------------" );

        sBaseNode = &aDPathBuffInfo->mFReqPgList;
        for( sCurNode = SMU_LIST_GET_FIRST(sBaseNode);
             sCurNode != sBaseNode;
             sCurNode = SMU_LIST_GET_NEXT(sCurNode) )
        {
            (void)dumpDPathBCB( (sdbDPathBCB*)sCurNode->mData );
        }
    }

    return IDE_SUCCESS;
}

/*******************************************************************************
 * Description : DPathBulkIOInfo를 dump한다.
 *
 * Parameters :
 *      aDPathBulkIOInfo - [IN] dump할 DPathBulkIOInfo
 ******************************************************************************/
IDE_RC sdbDPathBufferMgr::dumpDPathBulkIOInfo(
                                    sdbDPathBulkIOInfo *aDPathBulkIOInfo )
{
    smuList   * sBaseNode;
    smuList   * sCurNode;

    if( aDPathBulkIOInfo == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     " DPath Bulk IO Info: NULL\n"
                     "-----------------------" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     "  DPath Bulk IO Info Dump...\n"
                     "Real Direct IO Buffer Ptr     : %"ID_XPOINTER_FMT"\n"
                     "Aligned Direct IO Buffer Ptr  : %"ID_XPOINTER_FMT"\n"
                     "Buffer Size                   : %u\n"
                     "IO Request Count              : %u\n"
                     "Last Space ID                 : %u\n"
                     "Last Page ID                  : %u\n"
                     "-----------------------",
                     aDPathBulkIOInfo->mRDIOBuffPtr,
                     aDPathBulkIOInfo->mADIOBuffPtr,
                     aDPathBulkIOInfo->mDBufferSize,
                     aDPathBulkIOInfo->mIORequestCnt,
                     aDPathBulkIOInfo->mLstSpaceID,
                     aDPathBulkIOInfo->mLstPageID );

        sBaseNode = &aDPathBulkIOInfo->mBaseNode;
        for( sCurNode = SMU_LIST_GET_FIRST(sBaseNode);
             sCurNode != sBaseNode;
             sCurNode = SMU_LIST_GET_NEXT(sCurNode) )
        {
            (void)dumpDPathBCB( (sdbDPathBCB*)sCurNode->mData );
        }
    }

    return IDE_SUCCESS;
}

/*******************************************************************************
 * Description : DPathBCB를 모두 출력한다.
 *
 * Parameters :
 *      aDPathBCB   - [IN] dump할 DPathBCB
 ******************************************************************************/
IDE_RC sdbDPathBufferMgr::dumpDPathBCB( sdbDPathBCB *aDPathBCB )
{
    if( aDPathBCB == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "------------------\n"
                     " DPath BCB: NULL\n"
                     "------------------" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "---------------------\n"
                     "  DPath BCB Dump...\n"
                     "Space ID      : %u\n"
                     "Page ID       : %u\n"
                     "State         : %u\n"
                     "Logging       : %u\n"
                     "---------------------",
                     aDPathBCB->mSpaceID,
                     aDPathBCB->mPageID,
                     aDPathBCB->mState,
                     aDPathBCB->mIsLogging );
    }

    return IDE_SUCCESS;
}

