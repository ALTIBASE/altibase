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
 * $Id: sdpDPathInfoMgr.cpp 83870 2018-09-03 04:32:39Z kclee $
 *
 * Description : Direct-Path INSERT를 수행할 때 실제 Table에 반영할 Segment의
 *          정보를 관리하는 자료구조가 DPathInfo이다. sdpDPathInfoMgr 클래스는
 *          이 DPathInfo를 조작하기 위한 함수들을 제공한다.
 ******************************************************************************/

#include <sdb.h>
#include <sdr.h>
#include <sdpReq.h>
#include <sdpPageList.h>
#include <sdpSegDescMgr.h>
#include <sdpDPathInfoMgr.h>
#include <smrUpdate.h>

iduMemPool sdpDPathInfoMgr::mSegInfoPool;

/*******************************************************************************
 * Description : Static 변수들의 초기화를 수행한다.
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::initializeStatic()
{
    IDE_TEST( mSegInfoPool.initialize(
                 IDU_MEM_SM_SDP,
                 (SChar*)"DIRECT_PATH_SEGINFO_MEMPOOL",
                 1, /* List Count */
                 ID_SIZEOF( sdpDPathSegInfo ),
                 128, /* 생성시 가지고 있는 Item갯수 */
                 IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                 ID_TRUE,							/* UseMutex */
                 IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                 ID_FALSE,							/* ForcePooling */
                 ID_TRUE,							/* GarbageCollection */
                 ID_TRUE,                           /* HWCacheLine */
                 IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
              != IDE_SUCCESS);			

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Static 변수들을 파괴한다.
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::destroyStatic()
{
    IDE_TEST( mSegInfoPool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Direct Path Info를 생성하여 반환한다.
 *
 * Parameters :
 *      aDPathInfo - [OUT] 새로 생성한 Direct Path Info
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::initDPathInfo( sdpDPathInfo *aDPathInfo )
{
    IDE_DASSERT( aDPathInfo != NULL );

    /* FIT/ART/sm/Projects/PROJ-1665/createAlterTableTest.tc */
    IDU_FIT_POINT( "1.PROJ-1665@sdpDPathInfoMgr::initDPathInfo" );

    SMU_LIST_INIT_BASE( &aDPathInfo->mSegInfoList );

    aDPathInfo->mNxtSeqNo = 0;
    aDPathInfo->mInsRowCnt = 0;

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*******************************************************************************
 * Description : Direct Path Info에 달려 있는 모든 DPathSegInfo을 파괴한다.
 *
 * Parameters :
 *      aDPathInfo - [IN] 파괴할 Direct Path Info
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::destDPathInfo( sdpDPathInfo *aDPathInfo )
{
    IDE_DASSERT( aDPathInfo != NULL );

    IDE_TEST( sdpDPathInfoMgr::destAllDPathSegInfo( aDPathInfo )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : DPathSegInfo를 생성하고 초기화 한다.
 *
 * Parameters :
 *      aStatistics     - [IN] 통계
 *      aTrans          - [IN] DPath INSERT를 수행하는 Transaction
 *      aTableOID       - [IN] 생성할 DPathSegInfo에 해당하는 Table의 OID
 *      aDPathInfo      - [IN] 생성한 DPathSegInfo를 연결할 DPathInfo
 *      aDPathSegInfo   - [OUT] 생성한 DPathSegInfo를 반환
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::createDPathSegInfo( idvSQL           * aStatistics,
                                            void*              aTrans,
                                            smOID              aTableOID,
                                            sdpDPathInfo     * aDPathInfo, 
                                            sdpDPathSegInfo ** aDPathSegInfo )
{
    SInt                sState = 0;
    sdpPageListEntry  * sPageEntry;
    sdpSegmentDesc    * sSegDesc;
    scSpaceID           sSpaceID;
    scPageID            sSegPID;
    sdpDPathSegInfo   * sDPathSegInfo;

    sdpDPathSegInfo   * sLstSegInfo;
    sdpSegMgmtOp      * sSegMgmtOp;
    sdpSegInfo          sSegInfo;

    sdrMtx              sMtx;
    sdrMtxStartInfo     sStartInfo;
    smLSN               sNTA;
    ULong               sArrData[1];

    IDE_DASSERT( aStatistics != NULL );
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTableOID != SM_NULL_OID );
    IDE_DASSERT( aDPathInfo != NULL );
    IDE_DASSERT( *aDPathSegInfo == NULL );

    *aDPathSegInfo  = NULL;

    //--------------------------------------------------------------------
    // 지역 변수 초기화
    //--------------------------------------------------------------------
    sPageEntry  = (sdpPageListEntry*)smLayerCallback::getPageListEntry( aTableOID );
    sSegDesc    = &sPageEntry->mSegDesc;
    sSpaceID    = sSegDesc->mSegHandle.mSpaceID;
    sSegPID     = sdpPageList::getTableSegDescPID( sPageEntry );

    //--------------------------------------------------------------------
    // DPathSegInfo 하나를 할당한다.
    //--------------------------------------------------------------------
    /* sdpDPathInfoMgr_createDPathSegInfo_alloc_DPathSegInfo.tc */
    IDU_FIT_POINT("sdpDPathInfoMgr::createDPathSegInfo::alloc::DPathSegInfo");
    IDE_TEST( mSegInfoPool.alloc( (void**)&sDPathSegInfo ) != IDE_SUCCESS );
    sState = 1;
    
    //--------------------------------------------------------------------
    // DPathSegInfo에 항상 동일하게 초기화 되는 부분들을 먼저 초기화
    //--------------------------------------------------------------------
    SMU_LIST_INIT_NODE( &sDPathSegInfo->mNode );
    (&sDPathSegInfo->mNode)->mData = sDPathSegInfo;

    sDPathSegInfo->mSpaceID         = sSpaceID;
    sDPathSegInfo->mTableOID        = aTableOID;
    sDPathSegInfo->mFstAllocPID     = SD_NULL_PID;
    sDPathSegInfo->mLstAllocPagePtr = NULL;
    sDPathSegInfo->mRecCount        = 0;
    sDPathSegInfo->mSegDesc         = sSegDesc;
    sDPathSegInfo->mIsLastSeg       = ID_TRUE;

    sLstSegInfo = findLastDPathSegInfo( aDPathInfo, aTableOID );

    if( sLstSegInfo != NULL )
    {
        //------------------------------------------------------------
        //  이전 Statement에서 지금 Insert를 수행하려는 Segment에
        // Direct Path Insert를 수행한 적이 있는 경우, 기존에 수행했던
        // Direct Path Insert에 이어 붙이도록 DPathSegInfo를 구성한다.
        //------------------------------------------------------------
        sDPathSegInfo->mLstAllocExtRID      = sLstSegInfo->mLstAllocExtRID;
        sDPathSegInfo->mLstAllocPID         = sLstSegInfo->mLstAllocPID;
        sDPathSegInfo->mFstPIDOfLstAllocExt = sLstSegInfo->mFstPIDOfLstAllocExt;
        sDPathSegInfo->mTotalPageCount      = sLstSegInfo->mTotalPageCount;

        // 기존 SegInfo의 mIsLastSeg 플래그는 FALSE로 변경해준다.
        sLstSegInfo->mIsLastSeg           = ID_FALSE;
    }
    else
    {
        //-------------------------------------------------------------
        //  현재 Transaction 내에서 처음 Direct Path Insert를 수행하는
        // Segment일 경우, Insert 대상 테이블의 Segment 정보를 받아와서
        // DPathSegInfo를 구성한다.
        //-------------------------------------------------------------
        sSegMgmtOp  = sdpSegDescMgr::getSegMgmtOp( sSpaceID );

        IDE_ERROR( sSegMgmtOp != NULL );

        IDE_TEST( sSegMgmtOp->mGetSegInfo( aStatistics,
                                           sSpaceID,
                                           sSegPID,
                                           NULL, /* aTableHeader */
                                           &sSegInfo )
                  != IDE_SUCCESS );

        if( (sSegInfo.mLstAllocExtRID == SD_NULL_RID) ||
            (sSegInfo.mHWMPID == SD_NULL_PID) )
        {
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&sSegInfo,
                            ID_SIZEOF(sdpSegInfo),
                            "Create DPath Segment Info Dump..\n"
                            " Space ID     : %u\n"
                            " Segment PID  : %u",
                            sSpaceID,
                            sSegPID );

            IDE_ASSERT( 0 );
        }

        //--------------------------------------------------------------------
        //  할당할 Page를 찾는 시작점인 LstAllocPID를 HWMPID로 설정함으로써
        // HWM 이후의 페이지부터 할당하도록 지정한다.
        //
        //  TMS에서 HWM는 Extent 단위로 설정된다. 따라서 최초로 Page를 할당할
        // 때는 새로운 Extent를 할당받아서 사용한다.
        //--------------------------------------------------------------------
        sDPathSegInfo->mLstAllocExtRID      = sSegInfo.mLstAllocExtRID;
        sDPathSegInfo->mLstAllocPID         = sSegInfo.mHWMPID;
        sDPathSegInfo->mFstPIDOfLstAllocExt = sSegInfo.mFstPIDOfLstAllocExt;
        sDPathSegInfo->mTotalPageCount      = sSegInfo.mFmtPageCnt;
    }

    //-------------------------------------------------------------------
    // 새로 생성한 DPathSegInfo를 DPathInfo에 추가한다.
    // Rollback을 지원하기 DPathSegInfo가 추가될 때 마다 로깅을 해 둔다.
    //-------------------------------------------------------------------
    sStartInfo.mTrans   = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
                != IDE_SUCCESS );
    sState = 2;

    sNTA = smLayerCallback::getLstUndoNxtLSN( sStartInfo.mTrans );

    sDPathSegInfo->mSeqNo = aDPathInfo->mNxtSeqNo;
    SMU_LIST_ADD_LAST( &aDPathInfo->mSegInfoList,
                       &sDPathSegInfo->mNode );
    aDPathInfo->mNxtSeqNo++;
    sState = 3;

    /* FIT/ART/sm/Projects/PROJ-2068/PROJ-2068.tc */
    IDU_FIT_POINT( "1.PROJ-2068@sdpDPathInfoMgr::createDPathSegInfo" );

    sArrData[0] = sDPathSegInfo->mSeqNo;

    sdrMiniTrans::setNTA( &sMtx,
                          sSpaceID,
                          SDR_OP_SDP_DPATH_ADD_SEGINFO,
                          &sNTA,
                          sArrData,
                          1 ); // sArrData Size

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    *aDPathSegInfo = sDPathSegInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 3:
            IDE_ASSERT( destDPathSegInfo( aDPathInfo,
                                          sDPathSegInfo,
                                          ID_TRUE ) // Move LastFlag
                        == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
            break;
        case 1:
            IDE_ASSERT( mSegInfoPool.memfree( sDPathSegInfo ) == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    *aDPathSegInfo = NULL;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : DPathSegInfo를 파괴한다.
 *
 * Parameters :
 *      aDPathInfo      - [IN] 파괴할 DPathSegInfo가 달린 DPathInfo
 *      aDPathSegInfo   - [IN] 파괴할 DPathSegInfo
 *      aMoveLastFlag   - [IN] Last Flag를 옮겨줄지 말지 여부
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::destDPathSegInfo( sdpDPathInfo    * aDPathInfo,
                                          sdpDPathSegInfo * aDPathSegInfo,
                                          idBool            aMoveLastFlag )
{
    sdpDPathSegInfo   * sPrvDPathSegInfo;

    IDE_DASSERT( aDPathInfo != NULL );
    IDE_DASSERT( aDPathSegInfo != NULL );

    // BUG-30262 파괴할 DPathSegInfo를 먼저 리스트에서 제거해 주어야 합니다.
    SMU_LIST_DELETE( &aDPathSegInfo->mNode );

    // rollback을 수행할 때와 같이 LastFlag를 옮겨줘야 할 경우
    if( aMoveLastFlag == ID_TRUE )
    {
        // 가장 최근의 DPathSegInfo를 찾아서,
        sPrvDPathSegInfo = findLastDPathSegInfo( aDPathInfo,
                                                 aDPathSegInfo->mTableOID );

        if( sPrvDPathSegInfo != NULL )
        {
            sPrvDPathSegInfo->mIsLastSeg = ID_TRUE;
        }
    }

    IDE_TEST( mSegInfoPool.memfree( aDPathSegInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : DPathInfo에 달려있는 모든 DPathSegInfo을 파괴한다.
 *
 * Parameters :
 *      aDPathInfo  - 달려있는 모든 DPathSegInfo을 파괴할 DPathInfo
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::destAllDPathSegInfo( sdpDPathInfo  * aDPathInfo )
{
    sdpDPathSegInfo * sDPathSegInfo;
    smuList         * sBaseNode;
    smuList         * sCurNode;

    IDE_DASSERT( aDPathInfo != NULL );

    //----------------------------------------------------------------------
    // DPathInfo에 SegInfoList를 돌면서 모두 파괴해준다.
    //----------------------------------------------------------------------
    sBaseNode = &aDPathInfo->mSegInfoList;

    for( sCurNode = SMU_LIST_GET_FIRST(sBaseNode);
         !SMU_LIST_IS_EMPTY(sBaseNode);
         sCurNode = SMU_LIST_GET_FIRST(sBaseNode) )
    {
        sDPathSegInfo = (sdpDPathSegInfo*)sCurNode;
        IDE_TEST( destDPathSegInfo( aDPathInfo,
                                    sDPathSegInfo,
                                    ID_FALSE ) // Don't move LastFlag
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aDPathSegInfo에 달린 모든 DPathSegInfo를 Table Segment에
 *          merge 한다.
 *
 * Implementation :
 *    (1) DPath Insert 된 record 개수를 table info에 반영
 *    (3) Table Segment의 HWM 갱신
 *
 * Parameters :
 *      aStatistics     - [IN] 통계
 *      aTrans          - [IN] merge를 수행하는 TX
 *      aDPathInfo      - [IN] merge를 수행할 대상 DPathInfo
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::mergeAllSegOfDPathInfo(
                                        idvSQL*         aStatistics,
                                        void*           aTrans,
                                        sdpDPathInfo*   aDPathInfo )
{
    sdrMtxStartInfo     sStartInfo;
    sdpDPathSegInfo   * sDPathSegInfo;
    sdpSegmentDesc    * sSegDesc;
    sdpSegHandle      * sBaseSegHandle;
    smuList           * sBaseNode;
    smuList           * sCurNode;

    IDE_DASSERT( aStatistics != NULL );
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aDPathInfo != NULL );

    sStartInfo.mTrans   = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    sBaseNode = &aDPathInfo->mSegInfoList;
    for( sCurNode = SMU_LIST_GET_FIRST(sBaseNode);
         sCurNode != sBaseNode;
         sCurNode = SMU_LIST_GET_NEXT(sCurNode) )
    {
        sDPathSegInfo = (sdpDPathSegInfo*)sCurNode;

        /* FIT/ART/sm/Projects/PROJ-1665/recoveryTest.tc */
        IDU_FIT_POINT( "3.PROJ-1665@sdpDPathInfoMgr::mergeAllSegOfDPathInfo" );

        //------------------------------------------------------
        // INSERT 하러 들어왔는데, 할당한 Page가 없으면 ASSERT
        //------------------------------------------------------
        if( sDPathSegInfo->mFstAllocPID == SD_NULL_PID )
        {
            (void)dumpDPathInfo( aDPathInfo );
            IDE_ASSERT( 0 );
        }

        IDE_TEST( smLayerCallback::incRecordCountOfTableInfo( aTrans,
                                                              sDPathSegInfo->mTableOID,
                                                              sDPathSegInfo->mRecCount )
                  != IDE_SUCCESS );

        // X$DIRECT_PATH_INSERT - INSERT_ROW_COUNT
        aDPathInfo->mInsRowCnt += sDPathSegInfo->mRecCount;

        sSegDesc        = sDPathSegInfo->mSegDesc;
        sBaseSegHandle  = &(sSegDesc->mSegHandle);

        //-----------------------------------------------------------------
        // 마지막 SegInfo인 경우 해당 DPath Insert가 수행한 마지막 페이지가 
        // 포함된 Extent의 나머지 페이지들을 모두 재포맷한다.
        // rollback시 해당 페이지에 쓰레기 값이 존재할 수 있기 때문. 
        //-----------------------------------------------------------------
        if( sDPathSegInfo->mIsLastSeg == ID_TRUE )
        {
            IDE_TEST( sSegDesc->mSegMgmtOp->mReformatPage4DPath(
                                        aStatistics,
                                        &sStartInfo,
                                        sDPathSegInfo->mSpaceID,
                                        sBaseSegHandle,
                                        sDPathSegInfo->mLstAllocExtRID,
                                        sDPathSegInfo->mLstAllocPID )
                      != IDE_SUCCESS );
        }

        if( sDPathSegInfo->mLstAllocExtRID == SD_NULL_RID )
        {
            (void)dumpDPathInfo( aDPathInfo );
            IDE_ASSERT( 0 );
        }

        IDE_TEST( sSegDesc->mSegMgmtOp->mUpdateHWMInfo4DPath(
                                    aStatistics,
                                    &sStartInfo,
                                    sDPathSegInfo->mSpaceID,
                                    sBaseSegHandle,
                                    sDPathSegInfo->mFstAllocPID,
                                    sDPathSegInfo->mLstAllocExtRID,
                                    sDPathSegInfo->mFstPIDOfLstAllocExt,
                                    sDPathSegInfo->mLstAllocPID,
                                    sDPathSegInfo->mTotalPageCount,
                                    ID_FALSE )
                  != IDE_SUCCESS );

        /* FIT/ART/sm/Projects/PROJ-1665/recoveryTest.tc */
        IDU_FIT_POINT( "5.PROJ-1665@sdpDPathInfoMgr::mergeAllSegOfDPathInfo" );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTableOID에 해당하는 마지막 DPathSegInfo를 찾아준다.
 *
 *           동일 트랜잭션 내에, DPath INSERT를 수행한 테이블에 대해 DML 수행을
 *          금지하는데, 커서를 Open할 때 본 함수를 통해 커서를 오픈한 테이블이 
 *          동일 트랜잭션 내에서 DPath Insert를 수행했던 테이블인지 검사한다.
 *
 * Parameters :
 *      aDPathInfo  - [IN] Transaction에 달려있는 sdpDPathInfo
 *      aTableOID   - [IN] 찾을 DPathSegInfo에 해당하는 TableOID
 *
 * Return :
 *       aTableOID에 해당하는 sdpDPathSegInfo가 aDPathInfo에 존재한다면 찾아낸
 *      sdpDPathSegInfo의 포인터, 존재하지 않는다면 NULL을 반환한다.
 ******************************************************************************/
sdpDPathSegInfo* sdpDPathInfoMgr::findLastDPathSegInfo(
                                            sdpDPathInfo  * aDPathInfo,
                                            smOID           aTableOID )
{
    sdpDPathSegInfo       * sDPathSegInfo;
    sdpDPathSegInfo       * sLstSegInfo = NULL;   /* BUG-29031 */
    smuList               * sBaseNode;
    smuList               * sCurNode;

    IDE_DASSERT( aDPathInfo != NULL );
    IDE_DASSERT( aTableOID != SM_NULL_OID );

    sBaseNode = &aDPathInfo->mSegInfoList;
    for( sCurNode = SMU_LIST_GET_LAST(sBaseNode);
         sCurNode != sBaseNode;
         sCurNode = SMU_LIST_GET_PREV(sCurNode) )
    {
        sDPathSegInfo = (sdpDPathSegInfo*)sCurNode;

        if( sDPathSegInfo->mTableOID == aTableOID )
        {
            sLstSegInfo = sDPathSegInfo;
            break;
        }
    }

    return sLstSegInfo;
}

/*******************************************************************************
 * Description : DPathInfo를 dump 한다.
 *
 * Parameters :
 *      aDPathInfo  - [IN] dump할 DPathInfo
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::dumpDPathInfo( sdpDPathInfo *aDPathInfo )
{
    smuList   * sBaseNode;
    smuList   * sCurNode;

    if( aDPathInfo == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     " DPath Info: NULL\n"
                     "-----------------------" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "-----------------------\n"
                     "  DPath Info Dump...\n"
                     "Next Seq No       : %u\n"
                     "Insert Row Count  : %llu\n"
                     "-----------------------",
                     aDPathInfo->mNxtSeqNo,
                     aDPathInfo->mInsRowCnt );

        sBaseNode = &aDPathInfo->mSegInfoList;
        for( sCurNode = SMU_LIST_GET_FIRST(sBaseNode);
             sCurNode != sBaseNode;
             sCurNode = SMU_LIST_GET_NEXT(sCurNode) )
        {
            (void)dumpDPathSegInfo( (sdpDPathSegInfo*)sCurNode );
        }
    }

    return IDE_SUCCESS;
}

/*******************************************************************************
 * Description : DPathSegInfo를 dump 한다.
 *
 * Parameters :
 *      aDPathSegInfo   - [IN] dump할 DPathSegInfo
 ******************************************************************************/
IDE_RC sdpDPathInfoMgr::dumpDPathSegInfo( sdpDPathSegInfo *aDPathSegInfo )
{
    if( aDPathSegInfo == NULL )
    {
        ideLog::log( IDE_SERVER_0,
                     "-------------------\n"
                     " DPath Segment Info: NULL\n"
                     "-------------------" );
    }
    else
    {
        ideLog::log( IDE_SERVER_0,
                     "-------------------\n"
                     "  DPath Segment Info Dump...\n"
                     "Seq No                : %u\n"
                     "Space ID              : %u\n"
                     "Table OID             : %lu\n"
                     "First Alloc PID       : %u\n"
                     "Last Alloc PID        : %u\n"
                     "Last Alloc Extent RID : %llu\n"
                     "First PID Of Last Alloc Extent    : %u\n"
                     "Last Alloc PagePtr    : %"ID_XPOINTER_FMT"\n"
                     "Total Page Count      : %u\n"
                     "Record Count          : %u\n"
                     "Last Segment Flag    : %u\n"
                     "-------------------",
                     aDPathSegInfo->mSeqNo,
                     aDPathSegInfo->mSpaceID,
                     aDPathSegInfo->mTableOID,
                     aDPathSegInfo->mFstAllocPID,
                     aDPathSegInfo->mLstAllocPID,
                     aDPathSegInfo->mLstAllocExtRID,
                     aDPathSegInfo->mFstPIDOfLstAllocExt,
                     aDPathSegInfo->mLstAllocPagePtr,
                     aDPathSegInfo->mTotalPageCount,
                     aDPathSegInfo->mRecCount,
                     aDPathSegInfo->mIsLastSeg );

        (void)sdpSegDescMgr::dumpSegDesc( aDPathSegInfo->mSegDesc );
    }

    return IDE_SUCCESS;
}

