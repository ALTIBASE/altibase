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
 * $Id: sdptbSpaceDDL.cpp 27228 2008-07-23 17:36:52Z newdaily $
 *
 * DDL에 관련된 함수들이다.
 **********************************************************************/

#include <smErrorCode.h>
#include <sdp.h>
#include <sdptb.h>
#include <sdd.h>
#include <sdpModule.h>
#include <sdpPhyPage.h>
#include <sdpReq.h>         //gSdpReqFuncList
#include <sctTableSpaceMgr.h>
#include <sdptbDef.h>
#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h>

/***********************************************************************
 * Description:
 *  모든 create TBS에서 호출되어지는 공통적인 핵심루틴
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::createTBS( idvSQL             * aStatistics,
                                 sdrMtxStartInfo    * aStartInfo,
                                 smiTableSpaceAttr  * aTableSpaceAttr,
                                 smiDataFileAttr   ** aFileAttr,
                                 UInt                 aFileAttrCount )
{
    scSpaceID         sSpaceID;
    sdptbSpaceCache * sCache;
    UInt              sPagesPerExt;
    UInt              sValidSmallSize;
    UInt              i;

    IDE_ASSERT( aTableSpaceAttr != NULL );
    IDE_ASSERT( aFileAttr       != NULL );
    IDE_ASSERT( aStartInfo      != NULL );
    IDE_ASSERT( aStartInfo->mTrans  != NULL );
    IDE_ASSERT( aTableSpaceAttr->mDiskAttr.mExtPageCount > 0 );

    /* FIT/ART/sm/Design/Resource/Bugs/BUG-14900/BUG-14900.tc */
    IDU_FIT_POINT( "1.TASK-1842@sdptbSpaceDDL::createTBS" );

    //1025개 이상의 파일은 생성할수 없다.
    IDE_TEST_RAISE( aFileAttrCount > SD_MAX_FID_COUNT,
                    error_data_file_is_too_many );

    sPagesPerExt = aTableSpaceAttr->mDiskAttr.mExtPageCount;

    sValidSmallSize = sPagesPerExt + 
                      SDPTB_GG_HDR_PAGE_CNT + SDPTB_LG_HDR_PAGE_CNT;

    IDE_TEST( sdpTableSpace::checkPureFileSize( aFileAttr,
                                                aFileAttrCount,
                                                sValidSmallSize )
              != IDE_SUCCESS );

    //auto extend mode 세팅 및 next 사이즈 등을 체크한다.
    checkDataFileSize( aFileAttr,
                       aFileAttrCount,
                       sPagesPerExt );

    /* ------------------------------------------------
     * disk 관리자를 통한 tablespace 생성
     * ----------------------------------------------*/
    IDE_TEST(sddDiskMgr::createTableSpace(aStatistics,
                                          aStartInfo->mTrans,
                                          aTableSpaceAttr,
                                          aFileAttr,
                                          aFileAttrCount,
                                          SMI_EACH_BYMODE) != IDE_SUCCESS);
    sSpaceID = aTableSpaceAttr->mID;

    /* Space 모듈을 위한 Space Cache를 할당 및 초기화한다. */
    IDE_TEST( sdptbGroup::allocAndInitSpaceCache(
                          sSpaceID,
                          aTableSpaceAttr->mDiskAttr.mExtMgmtType,
                          aTableSpaceAttr->mDiskAttr.mSegMgmtType,
                          aTableSpaceAttr->mDiskAttr.mExtPageCount )
              != IDE_SUCCESS );

    sCache = sddDiskMgr::getSpaceCache( sSpaceID );
    IDE_ERROR_MSG( sCache != NULL , 
                   "Unable to create tablespace. "
                   "(tablespace ID :%"ID_UINT32_FMT")\n",
                    sSpaceID );

    /* BUG-27368 [SM] 테이블스페이스의 Data File ID가 순차적이지 않은 
     *           경우에 대한 고려가 필요합니다. */
    for( i = 0 ; i < aFileAttrCount ; i++ )
    {
        aFileAttr[i]->mID = i ;
    }

    IDE_TEST( sdptbGroup::makeMetaHeaders( aStatistics,
                                           aStartInfo,
                                           sSpaceID,
                                           sCache,
                                           aFileAttr,
                                           aFileAttrCount )   
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_data_file_is_too_many )
    {
          IDE_SET( ideSetErrorCode( smERR_ABORT_TOO_MANY_DATA_FILE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 * redo_SCT_UPDATE_DRDB_CREATE_TBS 에서 호출하는 redo 하위 루틴
 * sdptbSpaceDDL::createTBS()와 유사한 redo 함수
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::createTBS4Redo( void                * aTrans,
                                      smiTableSpaceAttr   * aTableSpaceAttr )
{
    scSpaceID         sSpaceID;
    sdptbSpaceCache * sCache;

    /* ------------------------------------------------
     * disk 관리자를 통한 tablespace 생성
     * ----------------------------------------------*/
    IDE_TEST( sddDiskMgr::createTableSpace4Redo( aTrans,
                                                 aTableSpaceAttr )
              != IDE_SUCCESS );

    sSpaceID = aTableSpaceAttr->mID;

    /* Space 모듈을 위한 Space Cache를 할당 및 초기화한다. */
    IDE_TEST( sdptbGroup::allocAndInitSpaceCache(
                  sSpaceID,
                  aTableSpaceAttr->mDiskAttr.mExtMgmtType,
                  aTableSpaceAttr->mDiskAttr.mSegMgmtType,
                  aTableSpaceAttr->mDiskAttr.mExtPageCount )
              != IDE_SUCCESS );

    sCache = sddDiskMgr::getSpaceCache( sSpaceID );
    IDE_ASSERT( sCache != NULL );

    // sdptbGroup::makeMetaHeaders() 를 생략하지만, 아래는 해야 함.
    sCache->mGGIDHint   = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * PROJ-1923 ALTIBASE HDB Disaster Recovery
 * redo_SCT_UPDATE_DRDB_CREATE_DBF 에서 호출하는 redo 하위 루틴
 * sdptbSpaceDDl::createDataFilesFEBT()와 유사
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::createDBF4Redo( void            * aTrans,
                                      smLSN             aCurLSN,
                                      scSpaceID         aSpaceID,
                                      smiDataFileAttr * aDataFileAttr )
{
    sdptbSpaceCache   * sCache          = NULL;
    UInt                sValidSmallSize = 0;
    UInt                sNewFileID      = 0;

    sddTableSpaceNode * sSpaceNode;

    IDE_ASSERT( aTrans          != NULL );
    IDE_ASSERT( aDataFileAttr   != NULL );

    IDU_FIT_POINT( "1.PROJ-1548@sdpTableSpace::createDataFiles" );

    sCache = sddDiskMgr::getSpaceCache( aSpaceID );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void **)&sSpaceNode)
              != IDE_SUCCESS );

    IDE_ASSERT( sSpaceNode  != NULL );
    IDE_ASSERT( sCache      != NULL );

    /* 아래 sddDiskMgr::createDataFiles()에서 mNewFileID가 변경될 수 있다. */
    sNewFileID  = sSpaceNode->mNewFileID;

    /* 로그 앵커의 newFileID 와 redo 로그의 mID가 같지 않다면,
     * 로그 앵커와 redo 로그가 짝이 맞지 않다는 것이므로,
     * redo하면 안된다 */
    IDE_TEST( sNewFileID != aDataFileAttr->mID );

    /* 1025개 이상의 파일은 생성할수 없다. */
    IDE_TEST_RAISE( (sSpaceNode->mNewFileID + (UInt)1) > SD_MAX_FID_COUNT,
                    error_data_file_is_too_many );

    IDE_TEST( sdpTableSpace::checkPureFileSize( &aDataFileAttr,
                                                1,
                                                sValidSmallSize )
              != IDE_SUCCESS );

    /* redo 이므로 TBS lock / unlock은 무시한다. */

    /* auto extend mode 세팅 및 next 사이즈 등을 체크한다. */
    checkDataFileSize( &aDataFileAttr,
                       1,
                       sCache->mCommon.mPagesPerExt );

    /* 아래 함수에서 데이타파일 노드에 대한 (X) 잠금을 획득한다. */
    /* ------------------------------------------------
     * disk 관리자를 통한 data file 생성
     * ----------------------------------------------*/
    /* redo log 1개에 대응 하도록 한다. */
    IDE_TEST( sddDiskMgr::createDataFile4Redo( aTrans,
                                               aCurLSN,
                                               aSpaceID,
                                               aDataFileAttr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_data_file_is_too_many )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_TOO_MANY_DATA_FILE ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  reset루틴의 핵심루틴임.
 *  reset undoTBS, reset tempTBS에서 내부적으로 사용되어진다.
 *
 *  이 함수안에서는 IDE_TEST 대신  IDE_ASSERT를 사용한다. 왜냐하면, 
 *  start up시만 콜되므로 에러처리가 필요없기 때문이다.
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::resetTBSCore( idvSQL             *aStatistics,
                                    void               *aTransForMtx,
                                    scSpaceID           aSpaceID )
{
    smiTableSpaceAttr      sSpaceAttr;
    smiDataFileAttr      * sArrDataFileAttr;
    smiDataFileAttr     ** sArrDataFileAttrPtr;
    UInt                   sDataFileAttrCount = 0;

    sddTableSpaceNode    * sSpaceNode = NULL;
    sddDataFileNode      * sFileNode = NULL;

    sdrMtxStartInfo        sStartInfo;
    sdptbSpaceCache      * sCache;
    UInt                   i;

    IDE_ASSERT( aTransForMtx != NULL );

    sCache = sddDiskMgr::getSpaceCache( aSpaceID );


    IDE_ASSERT(sctTableSpaceMgr::getTBSAttrByID( aStatistics,
                                                 aSpaceID,
                                                 &sSpaceAttr )
             == IDE_SUCCESS);

    IDE_ASSERT( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sSpaceNode)
                       == IDE_SUCCESS );
    IDE_ASSERT( sSpaceNode != NULL );
    IDE_ASSERT( sCache != NULL );

    IDE_ASSERT(iduMemMgr::malloc(
                    IDU_MEM_SM_SDP,
                    sSpaceNode->mDataFileCount * ID_SIZEOF(smiDataFileAttr*),
                    (void**)&sArrDataFileAttrPtr,
                    IDU_MEM_FORCE )
             == IDE_SUCCESS);

    IDE_ASSERT(iduMemMgr::malloc(
                    IDU_MEM_SM_SDP,
                    sSpaceNode->mDataFileCount * ID_SIZEOF(smiDataFileAttr),
                    (void**)&sArrDataFileAttr,
                    IDU_MEM_FORCE )
             == IDE_SUCCESS);

    for (i=0; i < ((sddTableSpaceNode*)sSpaceNode)->mNewFileID ;i++)
    {
        sFileNode=((sddTableSpaceNode*)sSpaceNode)->mFileNodeArr[i];

        if(sFileNode != NULL )
        {
            if( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) )
            {
                continue;
            }

            /* read from disk manager */
            sddDataFile::getDataFileAttr(sFileNode,
                                         &sArrDataFileAttr[sDataFileAttrCount]);

            sArrDataFileAttrPtr[sDataFileAttrCount] =
                     &sArrDataFileAttr[sDataFileAttrCount];


            sDataFileAttrCount++;
        }
    }

    sStartInfo.mTrans   = aTransForMtx;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    //reset시에는 이미 존재하는 file node8&  getDataFileAttr로 읽어오게됨.
    IDE_ASSERT( sdptbGroup::makeMetaHeaders(
                            aStatistics,
                            &sStartInfo,
                            aSpaceID,
                            sCache,
                            sArrDataFileAttrPtr,
                            sDataFileAttrCount )  
              == IDE_SUCCESS );

    IDE_ASSERT(iduMemMgr::free(sArrDataFileAttr) == IDE_SUCCESS);
    IDE_ASSERT(iduMemMgr::free(sArrDataFileAttrPtr) == IDE_SUCCESS);

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description:
 *  RID를 세트하는 핵심함수 
 * 
 *  여기서는 대부분의 인자에대한 assert처리 안함에 유의.
 *  앞에서 다했으므로 필요없음
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::setPIDCore( idvSQL        * aStatistics,
                                  sdrMtx        * aMtx,
                                  scSpaceID       aSpaceID,
                                  UInt            aIndex,
                                  scPageID        aTSSPID,
                                  sdptbRIDType    aRIDType)
{
    UChar           * sPagePtr;
    idBool            sDummy;
    scPageID        * sArrSegPID;
    sdptbGGHdr      * sGGHdr;

    IDE_ERROR_MSG( aRIDType < SDPTB_RID_TYPE_MAX,
                   "The RID TYPE is not valid. "
                   "(RID TYPE : %"ID_UINT32_FMT")\n", 
                   aRIDType );

    //첫번째 파일의 GG header에 TSS관련정보가 저장되어 있다.
    IDE_TEST(sdbBufferMgr::getPageByPID( aStatistics,
                                         aSpaceID,
                                         SDPTB_GET_GGHDR_PID_BY_FID( 0 ),
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         &sPagePtr,
                                         &sDummy,
                                         NULL /*IsCorruptPage*/ ) != IDE_SUCCESS);

    sGGHdr = sdptbGroup::getGGHdr(sPagePtr);

    if( aRIDType == SDPTB_RID_TYPE_TSS )
    {
        sArrSegPID = sGGHdr->mArrTSSegPID;
    }
    else
    {
        IDE_ERROR_MSG( aRIDType == SDPTB_RID_TYPE_UDS,
                       "The RID TYPE is not valid. "
                       "(RID TYPE : %"ID_UINT32_FMT")\n",
                       aRIDType );
        sArrSegPID = sGGHdr->mArrUDSegPID;
    }

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sArrSegPID[ aIndex ],
                                         (void*)&aTSSPID,
                                         ID_SIZEOF(aTSSPID) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  RID를 얻어내는 핵심함수 
 * 
 *  여기서는 대부분의 인자에대한 assert처리 안함에 유의.
 *  앞에서 다했으므로 필요없음
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::getPIDCore( idvSQL        * aStatistics,
                                  scSpaceID       aSpaceID,
                                  UInt            aIndex,
                                  sdptbRIDType    aRIDType,
                                  scPageID      * aTSSPID )
{
    UChar           * sPagePtr;
    scPageID        * sArrSegPID;
    UInt              sState=0;
    sdptbGGHdr      * sGGHdr;

    IDE_ERROR_MSG( aRIDType < SDPTB_RID_TYPE_MAX, 
                   "The RID TYPE is not valid. "
                   "(RID TYPE : %"ID_UINT32_FMT")\n", 
                   aRIDType );

    //첫번째 파일의 GG header에 TSS관련정보가 저장되어 있다.
    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          SDPTB_GET_GGHDR_PID_BY_FID( 0 ),
                                          &sPagePtr )
              != IDE_SUCCESS );
    sState = 1;

    sGGHdr = sdptbGroup::getGGHdr(sPagePtr);

    if( aRIDType == SDPTB_RID_TYPE_TSS )
    {
        sArrSegPID = sGGHdr->mArrTSSegPID;
    }
    else
    {
        IDE_ERROR_MSG( aRIDType == SDPTB_RID_TYPE_UDS,
                       "The RID TYPE is not valid. "
                       "(RID TYPE : %"ID_UINT32_FMT")\n",
                       aRIDType );
        sArrSegPID = sGGHdr->mArrUDSegPID;
    }

    *aTSSPID = sArrSegPID[ aIndex ];

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                       sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  하나의 데이타 화일을 공간을 늘리거나 줄인다.
 *
 * aFileName              - [IN] 크기를 변경할 파일의 이름
 * aSizeWanted            - [IN] QP에서 요청한 변경한 파일크기
 * aSizeChanged           - [OUT] 실제로 변경된 파일크기 
 * aValidDataFileName     - [OUT] 호환성을 위해서만 사용됨.
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterDataFileReSizeFEBT( idvSQL     *aStatistics,
                                               void       *aTrans,
                                               scSpaceID   aSpaceID,
                                               SChar      *aFileName,
                                               ULong       aSizeWanted,
                                               ULong      *aSizeChanged,
                                               SChar      *aValidDataFileName )
{
    sdrMtx              sMtx;
    UChar             * sPagePtr;
    idBool              sDummy;
    UInt                sState=0;
    sdptbGGHdr        * sGGHdr;
    sddDataFileNode   * sFileNode;
    sddTableSpaceNode * sSpaceNode;
    smLSN               sOpNTA;
    ULong               sData[2]; //이전크기를 페이지단위로 저장한다.
    UInt                sPageCntOld;
    sdptbSpaceCache  *  sCache;
    ULong               sMinSize; //최소한의 크기
    ULong               sUsedPageCount; //사용중인 크기

    IDE_DASSERT( aTrans       != NULL );
    IDE_DASSERT( aFileName    != NULL );
    IDE_DASSERT( aSizeChanged != NULL );
    IDE_DASSERT( aValidDataFileName != NULL );

    IDU_FIT_POINT( "1.TASK-1842@dpTableSpace::alterDataFileReSize" );
    /*
     * 만약 현재 수정할려는 파일크기로 하나의  extent 조차 만들지 못한다면
     * 에러메시지를 출력
     */
    sCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ERROR_MSG( sCache != NULL,
                   "The data file cannot be resized "
                   "because the tablespace cache does not exist or is not valid. "
                   "(spaceID : %"ID_UINT32_FMT")\n",
                   aSpaceID );

    // PRJ-1548 User Memory Tablespace
    // 트랜잭션이 완료될때(commit or abort) DataFile 잠금을 해제한다.
    // -------- TBS List (IX) -> TBS Node(IX) -> DBF Node (X) -----------
    // 생성중인 DBF Node에 대해서 잠금을 대기하는 경우
    //
    // A. 트랜잭션 COMMIT으로 인해 DBF Node가 ONLINE이다 .
    //    -> 잠금을 획득하고 resize를 수행한다.
    // B. 트랜잭션 ROLLBACK으로 DBF Node가 DROPPED이다.
    //    -> 잠금을 획득하지만 DBF Node 상태가 DROPPED임을 확인하고
    //       exception발생
    //
    // # alter/drop/create dbf 연산
    // 1. 이미 TBS Node (X) 잠금을 획득한 상태
    // 2. TBS META PAGE (S) Latch 획득
    // 4. 파일연산
    // 5. TBS META PAGE (S) Latch 해제
    // 6. 트랜잭션 완료(commit or abort)이후 모든 잠금 해제

    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                 aTrans,
                                 aSpaceID,
                                 ID_FALSE, /* non-intent lock */
                                 ID_TRUE,   /* exclusive lock */
                                 SCT_VAL_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState=1;

    IDE_TEST( getSpaceNodeAndFileNode( aSpaceID,
                                       aFileName,
                                       &sSpaceNode,
                                       &sFileNode,
                                       aValidDataFileName )
               != IDE_SUCCESS );

    IDE_ERROR_RAISE_MSG( sSpaceNode != NULL,  /* must be exist */
                         error_spaceNode_was_not_found, 
                         "The data file cannot be resized "
                         "because the tablespace node does not exist or is not valid. "
                         "(SpaceID : %"ID_UINT32_FMT")\n",
                           aSpaceID ); 
    IDE_ERROR_RAISE_MSG( sFileNode != NULL,  /* must be exist */
                         error_fileNode_was_not_found, 
                         "The data file cannot be resized "
                         "because the file node does not exist or is not valid. "
                         "(SpaceID : %"ID_UINT32_FMT", "
                         "FileName : %s)\n",
                         aSpaceID, 
                         aFileName ); 
    IDE_ERROR_MSG( SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ),
                   "The data file cannot be resized "
                   "because the file was dropped. "
                   "(SpaceID : %"ID_UINT32_FMT", "
                   "FileName : %s)\n",
                   aSpaceID, 
                   aFileName );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          SDPTB_GET_GGHDR_PID_BY_FID(sFileNode->mID),
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

    sGGHdr = sdptbGroup::getGGHdr(sPagePtr);

    /*
     *   HWM 보다 작게 파일을 줄일수는 없다.
     */
    // HWM의 PageID가 곧 사용된 Page의 수이다.
    sUsedPageCount = SD_MAKE_FPID( sGGHdr->mHWM );

    // 최소 1개의 Extent Size보다는 커야 한다.
    sMinSize = SDPTB_GG_HDR_PAGE_CNT +
               SDPTB_LG_HDR_PAGE_CNT +
               sCache->mCommon.mPagesPerExt;

    // BUG-29566 데이터 파일의 크기를 32G 를 초과하여 지정해도 에러를
    //           출력하지 않습니다.
    // 사용자가 대응하기 편하게 하기 위해 큰 값과 비교해서 오류를 반환합니다.
    if( sUsedPageCount > sMinSize )
    {
        if( sUsedPageCount > aSizeWanted )
        {
            sState=0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
            IDE_RAISE( error_cant_shrink_below_HWM );
        }
        else
        {
            /* nothing to do ... */
        }
    }
    else
    {
        if( sMinSize > aSizeWanted )
        {
            sState=0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
            IDE_RAISE( error_file_size_is_too_small );
        }
        else
        {
            /* nothing to do ... */
        }
    }

    sPageCntOld = sGGHdr->mTotalPages;

    // 아래 함수에서 데이타파일 노드에 대한 (X) 잠금을 획득한다.
    if( sddDiskMgr::alterResizeFEBT( aStatistics,
                                     aTrans,
                                     aSpaceID,
                                     aFileName , //valid한 파일명임
                                     sGGHdr->mHWM,
                                     aSizeWanted,
                                     sFileNode) != IDE_SUCCESS )
    {
        sdrMiniTrans::setIgnoreMtxUndo( &sMtx );
        IDE_TEST( 1 );
    }
    else
    {
        /* nothing to do ... */
    }

    /* To Fix BUG-23868 [AT-F5 ART] Disk TableSpace의 Datafile 
     * Resize에 대한 복원이 되지 않음. 
     * Tablspace의 Resize연산을 포함하도록 NTA구간을 설정하면, 
     * 이후 Rollback 발생시 GG만 복원되고 DataFile 상태와 파일크기가 
     * 복원되지 않는다. */
    if( sdrMiniTrans::getTrans(&sMtx) != NULL )
    {
       sOpNTA = smLayerCallback::getLstUndoNxtLSN( sdrMiniTrans::getTrans( &sMtx ) );
    }
    else
    {
        /* Temporary Table 생성시에는 트랜잭션이 NULL이
         * 내려올 수 있다. */
    }

    if( sPageCntOld < aSizeWanted ) //확장
    {
        IDE_ERROR_MSG( sFileNode->mCurrSize == aSizeWanted,
                       "The data file cannot be resized "
                       "because the requested size of the data file is wrong. "
                       "(SpaceID : %"ID_UINT32_FMT", "
                       "Request Size : %"ID_UINT32_FMT", "
                       "Current Size : %"ID_UINT32_FMT")\n",
                       aSpaceID,
                       aSizeWanted,
                       sFileNode->mCurrSize );

        IDE_TEST( sdptbGroup::resizeGGCore( aStatistics,
                                            &sMtx,
                                            aSpaceID,
                                            sGGHdr,
                                            sFileNode->mCurrSize )
                    != IDE_SUCCESS );
    }
    else   //축소
    {

        IDE_TEST( sdptbGroup::resizeGGCore( aStatistics,
                                            &sMtx,
                                            aSpaceID,
                                            sGGHdr,
                                            aSizeWanted )
                    != IDE_SUCCESS );
    }

    /*
     * 만약 확장을 했다면 cache의 FID비트를 켜준다.
     */
    if( sFileNode->mCurrSize > sPageCntOld  )
    {
        /* BUG-47666 mFreenessOfGGs에 동시성 제어가 필요합니다. */
        sdptbBit::atomicSetBit32( (UInt*)sCache->mFreenessOfGGs, sFileNode->mID);
    }

    *aSizeChanged = aSizeWanted;

    sData[0] = sGGHdr->mGGID;
    sData[1] = sPageCntOld;

    sdrMiniTrans::setNTA( &sMtx,
                          aSpaceID,
                          SDR_OP_SDPTB_RESIZE_GG,
                          &sOpNTA,
                          sData,
                          2 /* Data Count */ );

    sState=0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;


    IDE_EXCEPTION( error_file_size_is_too_small );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_SHRINK_SIZE_IS_TOO_SMALL,
                                  aSizeWanted,
                                  sMinSize ));
    }
    IDE_EXCEPTION( error_cant_shrink_below_HWM );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CANT_SHRINK_BELOW_HWM,
                                  aSizeWanted,
                                  sUsedPageCount ));
    }
    IDE_EXCEPTION(error_spaceNode_was_not_found)
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                  aSpaceID ));
    }
    IDE_EXCEPTION(error_fileNode_was_not_found)
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileNode,
                                  aSpaceID ));
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}


/***********************************************************************
 * Description:
 *  File 명에 해당하는 FileNode를 반환하고 해당 SpaceNode도 함께 반환한다. 
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::getSpaceNodeAndFileNode(
                         scSpaceID               aSpaceID,
                         SChar                 * aFileName,
                         sddTableSpaceNode    ** aSpaceNode,
                         sddDataFileNode      ** aFileNode,
                         SChar                 * aValidDataFileName )
{
    UInt                  sNameLength;
    sddTableSpaceNode   * sSpaceNode;

    IDE_ASSERT( aFileName != NULL);
    IDE_ASSERT( aSpaceNode != NULL);
    IDE_ASSERT( aFileNode != NULL);
    IDE_ASSERT( aValidDataFileName != NULL);

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                                    aSpaceID,
                                    (void**)&sSpaceNode)
                       != IDE_SUCCESS );

    IDE_ERROR_RAISE_MSG( sSpaceNode->mHeader.mID == aSpaceID,
                         error_spaceNode_was_not_found,
                         "The tablespace was not found "
                         "because the tablespace node does not exist or is not valid. "
                         "(SpaceID : %"ID_UINT32_FMT", "
                         "TapleSpace Node : %"ID_UINT32_FMT")",
                         aSpaceID,
                         sSpaceNode->mHeader.mID );

    idlOS::strcpy( aValidDataFileName, aFileName ); //useless

    sNameLength = idlOS::strlen(aFileName);

    IDE_TEST( sctTableSpaceMgr::makeValidABSPath(
                                ID_TRUE,
                                aFileName,
                                &sNameLength,
                                SMI_TBS_DISK)
              != IDE_SUCCESS );

    IDE_TEST( sddTableSpace::getDataFileNodeByName( sSpaceNode,
                                                    aFileName,
                                                    aFileNode ) != IDE_SUCCESS);

    *aSpaceNode = sSpaceNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_spaceNode_was_not_found)
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                  aSpaceID ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  하나의 데이타 화일을 삭제한다.
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::removeDataFile( idvSQL      * aStatistics,
                                      void        * aTrans,
                                      scSpaceID     aSpaceID,
                                      SChar       * aFileName,
                                      SChar       * aValidDataFileName )
{
    sdrMtx                  sMtx;
    UInt                    sState  = 0;
    UChar                 * sPagePtr;
    idBool                  sDummy;
    sdptbGGHdr            * sGGHdr;
    sddDataFileNode       * sFileNode;
    sddTableSpaceNode     * sSpaceNode;
    sdptbSpaceCache       * sCache;
    sctPendingOp          * sPendingOp;

    IDE_ASSERT( aTrans != NULL );

    sCache = sddDiskMgr::getSpaceCache( aSpaceID );

    IDE_ERROR_MSG( sCache != NULL,
                   "The data file cannot be resized "
                   "because the tablespace cache does not exist or is not valid. "
                   "(spaceID : %"ID_UINT32_FMT")\n",
                   aSpaceID );

    // PRJ-1548 User Memory Tablespace
    // 트랜잭션이 완료될때(commit or abort) DataFile 잠금을 해제한다.
    // 운영중에는 제거한 DBF Node에 대해서 PENDING 연산으로도 DBF Node를
    // free하지 않기 때문에 잠금을 commit 후에 잠금을 해제해도 문제가 되지 않는다

    // # alter/drop/create dbf 연산
    // 1. 이미 TBS Node (X) 잠금을 획득한 상태
    // 2. TBS META PAGE (S) Latch 획득
    // 4. 파일연산
    // 5. TBS META PAGE (S) Latch 해제
    // 6. 트랜잭션 완료(commit or abort)이후 모든 잠금 해제

    // --------- TBS NODE (IX) --------------- //
    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                 aTrans,
                                 aSpaceID,
                                 ID_FALSE,   /* non-intent lock */
                                 ID_TRUE,    /* exclusive lock */
                                 SCT_VAL_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState=1;


    IDE_TEST( getSpaceNodeAndFileNode( aSpaceID,
                                       aFileName,
                                       &sSpaceNode,
                                       &sFileNode,
                                       aValidDataFileName )
               != IDE_SUCCESS );

    IDE_ERROR_RAISE_MSG( sSpaceNode != NULL,
                         error_spaceNode_was_not_found,
                         "The data file cannot be removed "             
                         "because the tablespace node does not exist or is not valid. " 
                         "(SpaceID : %"ID_UINT32_FMT")\n",
                          aSpaceID );  /* must be exist */
    IDE_ERROR_RAISE_MSG( sFileNode != NULL,
                          error_fileNode_was_not_found,
                         "The data file cannot be removed "             
                         "because the file node does not exist or is not valid. " 
                         "(SpaceID : %"ID_UINT32_FMT", "
                         "FileName : %s)\n", 
                         aSpaceID, aFileName );  /* must be exist */

    IDE_ERROR_MSG( SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ),
                   "The data file cannot be removed "
                   "because the file was dropped. " 
                   "(SpaceID : %"ID_UINT32_FMT", "
                   "FileName : %s)\n",
                   aSpaceID, aFileName );

    /*
     *  파일이 삭제 된다고해서 삭제되는 파일의 GG가 변경될 필요는 없다.
     *  space cache만 수정되면 된다. 그러므로, S로 잡으면 된다.
     */
    IDE_TEST( sdbBufferMgr::getPageByPID( 
                                   aStatistics,
                                   aSpaceID,
                                   SDPTB_GET_GGHDR_PID_BY_FID(sFileNode->mID),
                                   SDB_S_LATCH,
                                   SDB_WAIT_NORMAL,
                                   SDB_SINGLE_PAGE_READ,
                                   &sMtx,
                                   &sPagePtr,
                                   &sDummy,
                                   NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

    sGGHdr = sdptbGroup::getGGHdr(sPagePtr);


    /* 첫번째 파일은 삭제하지 못하도록 해야한다.*/
    IDE_TEST_RAISE( sFileNode->mID == SDPTB_FIRST_FID,
                    error_can_not_remove_data_file);

    /*
     * 파일의 삭제는 HWM가 0일때,즉 해당파일에대해 어떠한 할당도 이뤄진 적이
     * 없을 경우에만 가능하다.
     */
    IDE_TEST_RAISE( sGGHdr->mHWM != SD_CREATE_PID( sFileNode->mID, 0),
                    error_can_not_remove_data_file );

    // TBS META PAGE (S) Latch 해제
    sState=0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    /* BUGBUG-6878 */
    IDE_TEST( sddDiskMgr::removeDataFileFEBT( aStatistics,
                                              aTrans,
                                              sSpaceNode,
                                              sFileNode,
                                              SMI_ALL_NOTOUCH) != IDE_SUCCESS );

    /*
     * To Fix BUG-23874 [AT-F5 ART] alter tablespace add datafile 에
     * 대한 복원이 안되는 것 같음.
     *
     * 제거된 파일에 대한 가용도를 SpaceNode에 반영할때는 트랜잭션 Commit Pending으로
     * 처리해야 한다.
     */

    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                  aTrans,
                  sSpaceNode->mHeader.mID,
                  ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                  SCT_POP_UPDATE_SPACECACHE,
                  & sPendingOp )
              != IDE_SUCCESS );

    sPendingOp->mPendingOpFunc  = sdptbSpaceDDL::alterDropFileCommitPending;
    sPendingOp->mFileID         = sFileNode->mID;
    sPendingOp->mPendingOpParam = (void*)sCache;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_can_not_remove_data_file)
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_CannotRemoveDataFileNode  ) );
    }
    IDE_EXCEPTION(error_spaceNode_was_not_found)
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                  aSpaceID ));
    }
    IDE_EXCEPTION(error_fileNode_was_not_found)
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundDataFileNode,
                                  aSpaceID ));
    }
    IDE_EXCEPTION_END;

    if(sState == 1)
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  파일의 크기를 체크한다.
 ***********************************************************************/
void sdptbSpaceDDL::checkDataFileSize( smiDataFileAttr   ** aDataFileAttr,
                                       UInt                 aDataFileAttrCount,
                                       UInt                 aPagesPerExt )
{
    UInt                sLoop;
    smiDataFileAttr   * sDataFileAttrPtr;
    UInt                sTemp;
    UInt                sFileHdrSizeInBytes;
    UInt                sFileHdrPageCnt;

    IDE_DASSERT( aDataFileAttr != NULL );

    sFileHdrSizeInBytes = 
                     idlOS::align( SM_DBFILE_METAHDR_PAGE_SIZE, SD_PAGE_SIZE );

    sFileHdrPageCnt = sFileHdrSizeInBytes / SD_PAGE_SIZE;

    for( sLoop = 0; sLoop != aDataFileAttrCount; sLoop++ )
    {
        sDataFileAttrPtr = aDataFileAttr[ sLoop ];

        IDE_ASSERT( sDataFileAttrPtr->mCurrSize != 0 );
        IDE_ASSERT( sDataFileAttrPtr->mInitSize != 0 );

        IDE_ASSERT( sDataFileAttrPtr->mCurrSize ==
                    sDataFileAttrPtr->mInitSize );

        IDE_ASSERT( ((sDataFileAttrPtr->mIsAutoExtend == ID_TRUE) &&
                     (sDataFileAttrPtr->mNextSize != 0)) ||
                     (sDataFileAttrPtr->mIsAutoExtend == ID_FALSE) );

        /*
         * BUG-22351 TableSpace 의 MaxSize 가 이상합니다.
         */
        alignSizeWithOSFileLimit( &sDataFileAttrPtr->mInitSize,
                                  sFileHdrPageCnt );
        alignSizeWithOSFileLimit( &sDataFileAttrPtr->mCurrSize,
                                  sFileHdrPageCnt );

        //next사이즈를 extent사이즈로  align
        if( sDataFileAttrPtr->mNextSize != 0)
        {
            if( sDataFileAttrPtr->mNextSize % aPagesPerExt )
            {
                sTemp = sDataFileAttrPtr->mNextSize / aPagesPerExt;
                sDataFileAttrPtr->mNextSize = (sTemp + 1) * aPagesPerExt;
            }
        }

        /*
         * BUG-22351 TableSpace 의 MaxSize 가 이상합니다.
         */
        if( sDataFileAttrPtr->mMaxSize == 0 )
        {
            // 사용자가 maxsize를 명시하지 않은 경우
            // 또는 unlimited인 경우 OS file limit을 고려하여 설정한다.

            // BUG-17415 autoextend off일 경우 maxsize는
            // 의미가 없기 때문에 마찬가지로 OS file limit로 세팅한다.
            sDataFileAttrPtr->mMaxSize = sddDiskMgr::getMaxDataFileSize()
                                         - sFileHdrPageCnt;
        }
        else
        {

            alignSizeWithOSFileLimit( &sDataFileAttrPtr->mMaxSize,
                                      sFileHdrPageCnt );
        }

    }

}

/***********************************************************************
 * Description:
 *  space cache를 수정해줘야한다.
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::createDataFilesFEBT( idvSQL             * aStatistics,
                                           void               * aTrans,
                                           scSpaceID            aSpaceID,
                                           smiDataFileAttr   ** aDataFileAttr,
                                           UInt                 aDataFileAttrCount)
{
    sdptbSpaceCache     * sCache;
    UInt                  sPagesPerExt;
    UInt                  sStartNewFileID;
    sddTableSpaceNode   * sSpaceNode;
    sdrMtxStartInfo       sStartInfo;
    UInt                  sValidSmallSize;
    UInt                  sState    = 0;
    UInt                  i;

    IDE_ASSERT( aTrans          != NULL );
    IDE_ASSERT( aDataFileAttr   != NULL );

    sCache = sddDiskMgr::getSpaceCache( aSpaceID );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void **)&sSpaceNode)
                       != IDE_SUCCESS );

    IDE_ASSERT( sSpaceNode  != NULL );
    IDE_ASSERT( sCache      != NULL );


    /* 1025개 이상의 파일은 생성할수 없다. */
    IDE_TEST_RAISE( (sSpaceNode->mNewFileID + aDataFileAttrCount) > SD_MAX_FID_COUNT,
                    error_data_file_is_too_many );

    sPagesPerExt = sCache->mCommon.mPagesPerExt;

    sValidSmallSize = sPagesPerExt + 
                      SDPTB_GG_HDR_PAGE_CNT + SDPTB_LG_HDR_PAGE_CNT;

    IDE_TEST( sdpTableSpace::checkPureFileSize( aDataFileAttr,
                                                aDataFileAttrCount,
                                                sValidSmallSize )
              != IDE_SUCCESS );

    /* 트랜잭션이 완료될때(commit or abort) TableSpace 잠금을 해제한다. */
    if( aTrans != NULL )
    {
       /* # alter/create/drop dbf 연산
        * 1. 이미 TBS Node (X) 잠금을 획득한 상태
        * 2. TBS META PAGE (S) Latch 획득
        * 4. 파일연산
        * 5. TBS META PAGE (S) Latch 해제
        * 6. 트랜잭션 완료(commit or abort)이후 모든 잠금 해제 */
       /* PRJ-1548 : --------- TBS NODE (IX) --------------- */

        /* BUG-31608 [sm-disk-page] add datafile during DML
         * Intensive Lock으로 변경하여 AddDataFile동안 DML이 가능하도록
         * 수정한다. */
        IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                     aTrans,
                                     aSpaceID,
                                     ID_TRUE,   /* intention lock */
                                     ID_TRUE,   /* exclusive lock */
                                     SCT_VAL_DDL_DML )
                  != IDE_SUCCESS );
    }

    sdptbGroup::prepareAddDataFile( aStatistics, sCache );
    sState = 1;

    /* 아래 sddDiskMgr::createDataFiles()에서 mNewFileID가 변경될 수 있다. */
    sStartNewFileID = sSpaceNode->mNewFileID;

    /* auto extend mode 세팅 및 next 사이즈 등을 체크한다. */
    checkDataFileSize( aDataFileAttr,
                       aDataFileAttrCount,
                       sCache->mCommon.mPagesPerExt );

    /* 아래 함수에서 데이타파일 노드에 대한 (X) 잠금을 획득한다. */
    IDE_TEST( sddDiskMgr::createDataFiles( aStatistics,
                                           aTrans,
                                           aSpaceID,
                                           aDataFileAttr,
                                           aDataFileAttrCount,
                                           SMI_EACH_BYMODE )
              != IDE_SUCCESS );



    /* sdptb를 위한 메타 헤더들을 만들어준다. */
    sStartInfo.mTrans = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    for( i=0 ; i < aDataFileAttrCount ; i++ )
    {
        aDataFileAttr[i]->mID =  sStartNewFileID + i;
    }

    IDE_TEST( sdptbGroup::makeMetaHeaders( aStatistics,
                                           &sStartInfo,
                                           aSpaceID,
                                           sCache,
                                           aDataFileAttr,
                                           aDataFileAttrCount ) 
              != IDE_SUCCESS );

    sState = 0;
    sdptbGroup::completeAddDataFile( sCache );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_data_file_is_too_many )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_TOO_MANY_DATA_FILE ));
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        sdptbGroup::completeAddDataFile( sCache );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  autoextend mode를 set한다.
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterDataFileAutoExtendFEBT( idvSQL   *aStatistics,
                                                   void     *aTrans,
                                                   scSpaceID aSpaceID,
                                                   SChar    *aFileName,
                                                   idBool    aAutoExtend,
                                                   ULong     aNextSize,
                                                   ULong     aMaxSize,
                                                   SChar    *aValidDataFileName)
{
    sdrMtx                 sMtx;
    UChar                * sPagePtr;
    idBool                 sDummy;
    UInt                   sState=0;
    sdptbSpaceCache      * sCache;
    sddDataFileNode      * sFileNode;
    sddTableSpaceNode    * sSpaceNode;

    IDE_ASSERT( aTrans              != NULL );
    IDE_ASSERT( aFileName           != NULL );
    IDE_ASSERT( aValidDataFileName  != NULL );

    IDU_FIT_POINT( "1.TASK-1842@sdpTableSpace::alterDataFileAutoExtend" );

    // PRJ-1548 User Memory Tablespace
    // 트랜잭션이 완료될때(commit or abort) DataFile 잠금을 해제한다.
    //
    // A. 트랜잭션 COMMIT으로 인해 DBF Node가 ONLINE이다 .
    //    -> 잠금을 획득하고 resize를 수행한다.
    //
    // B. 트랜잭션 ROLLBACK으로 DBF Node가 DROPPED이다.
    //    -> 잠금을 획득하지만 DBF Node 상태가 DROPPED임을 확인하고
    //       exception발생
    //
    // DBF 파일의 자동확장연산과의 동시성문제로 다음과 같은 순서로
    // 잠금을 획득하고 연산을 수행한다.
    //
    // # alter/create/drop dbf 연산
    // 1. 이미 TBS Node (X) 잠금을 획득한 상태
    // 2. TBS META PAGE (S) Latch 획득
    // 4. 파일확장
    // 5. TBS META PAGE (S) Latch 해제
    // 6. 트랜잭션 완료(commit or abort)이후 모든 잠금 해제

    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                              aTrans,
                                              aSpaceID,
                                              ID_FALSE,    /* non-intent lock */
                                              ID_TRUE,    /* exclusive lock */
                                              SCT_VAL_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState=1;

    IDE_TEST( getSpaceNodeAndFileNode( aSpaceID,
                                       aFileName,
                                       &sSpaceNode,
                                       &sFileNode,
                                       aValidDataFileName )
               != IDE_SUCCESS );

    sCache = sddDiskMgr::getSpaceCache( sSpaceNode );

    IDE_ASSERT( sSpaceNode != NULL );  /* must be exist */
    IDE_ASSERT( sFileNode != NULL );  /* must be exist */
    IDE_ASSERT( sCache != NULL );

    IDE_ASSERT( SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ) );

    IDE_TEST( sdbBufferMgr::getPageByPID(
                               aStatistics,
                               aSpaceID,
                               SDPTB_GET_GGHDR_PID_BY_FID(sFileNode->mID),
                               SDB_S_LATCH,
                               SDB_WAIT_NORMAL,
                               SDB_SINGLE_PAGE_READ,
                               &sMtx,
                               &sPagePtr,
                               &sDummy,
                               NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

    if( aMaxSize == 0 )
    {
        aMaxSize = sddDiskMgr::getMaxDataFileSize();
    }

    // 아래 함수에서 데이타파일 노드에 대한 (X) 잠금을 획득한다.
    IDE_TEST( sddDiskMgr::alterAutoExtendFEBT( aStatistics,
                                               aTrans,
                                               sSpaceNode,
                                               aFileName,
                                               sFileNode,
                                               aAutoExtend,
                                               aNextSize,
                                               aMaxSize ) != IDE_SUCCESS );

    // TBS META PAGE (S) Latch 해제
    sState=0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 
 *  만약 파일헤더를 고려한 크기가 OS limit보다 크다면 OS limit에 맞춘다.
 *  (aFileHdrPageCnt는 일반적으로 1이다)
 *
 *  aAlignDest          - [IN][OUT] 정렬할 대상
 *  aFileHdrPageCnt     - [IN]      파일헤더의 페이지갯수(일반적으로 1임)
 * 
 * BUG-22351 TableSpace 의 MaxSize 가 이상합니다.
 **********************************************************************/
void sdptbSpaceDDL::alignSizeWithOSFileLimit( ULong *aAlignDest,
                                              UInt   aFileHdrPageCnt )
{
    IDE_ASSERT( aAlignDest != NULL);

    if( (*aAlignDest + aFileHdrPageCnt) > sddDiskMgr::getMaxDataFileSize() )
    {
        *aAlignDest = sddDiskMgr::getMaxDataFileSize() - aFileHdrPageCnt;
    }
}



/***********************************************************************
 * Description : TableSpace를 Drop한다.
 *
 * Implementation :
 *     sddDiskMgr::removeTableSpace( aSpace )
 *
 * aStatistics        - [IN] 통계정보
 * aTrans             - [IN] Transaction Pointer
 * aSpaceID           - [IN] TableSpace ID
 * aTouchMode         - [IN] TableSpace 에 속한 파일을 삭제할 지 결정한다.
 *
 **********************************************************************/
IDE_RC sdptbSpaceDDL::dropTBS( idvSQL      * aStatistics,
                               void        * aTrans,
                               scSpaceID     aSpaceID,
                               smiTouchMode  aTouchMode )
{
    IDE_DASSERT( aTrans != NULL );
    
    /* FIT/ART/sm/Design/Resource/TASK-1842/DROP_TABLESPACE.tc */
    IDU_FIT_POINT( "1.PROJ-1548@sdpTableSpace::dropTBS" );

    // PRJ-1548 User Memory Tablespace
    // 트랜잭션이 완료될때(commit or abort) TableSpace 잠금을 해제한다.
    // 잠금이 획득된 이후에 해당 테이블 스페이스를 요구하는 어떠한
    // 트랜잭션도 버퍼에 접근되어서는 안된다.
    // -------------- TBS Node (X) ------------------ //

    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                    aTrans,
                                    aSpaceID,
                                    ID_FALSE,   /* non-intent */
                                    ID_TRUE,    /* exclusive */
                                    SCT_VAL_DROP_TBS) /* aValidation */
              != IDE_SUCCESS );

    IDE_TEST( sddDiskMgr::removeTableSpace( aStatistics,
                                            aTrans,
                                            aSpaceID,
                                            aTouchMode )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "1.PROJ-1548@sdptbSpaceDDL::dropTBS" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  Disk Tablespace에 대해 Alter Tablespace Online/Offline을 수행
 *
 * aTrans        - [IN] 상태를 변경하려는 Transaction
 * aTableSpaceID - [IN] 상태를 변경하려는 Tablespace의 ID
 * aState        - [IN] 새로 전이할 상태 ( Online or Offline )
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterTBSStatus( idvSQL*             aStatistics,
                                      void              * aTrans,
                                      sddTableSpaceNode * aSpaceNode,
                                      UInt                aState )
{
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aSpaceNode != NULL );

    switch ( aState )
    {
        case SMI_TBS_ONLINE:
            IDE_TEST( alterTBSonline( aStatistics,
                                      aTrans,
                                      aSpaceNode )
                      != IDE_SUCCESS );
            break;

        case SMI_TBS_OFFLINE:
            IDE_TEST( alterTBSoffline( aStatistics,
                                       aTrans,
                                       aSpaceNode )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_ASSERT( 0 );
            break;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *   Tablespace를 DISCARDED상태로 바꾸고, Loganchor에 Flush한다.
 *
 *   *Discard의 정의 :
 *      - 더 이상 사용할 수 없는 Tablespace
 *      - 오직 Drop만이 가능
 *
 *   *사용예 :
 *      - Disk가 완전히 맛가서 더이상 사용불가할 때
 *        해당 Tablespace만 discard하고 나머지 Tablespace만이라도
 *        운영하고 싶을때, CONTROL단계에서 Tablespace를 DISCARD한다.
 *
 *    *동시성제어 :
 *      - CONTROL단계에서만 호출되기 때문에, sctTableSpaceMgr에
 *        Mutex를 잡을 필요가 없다.
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterTBSdiscard( sddTableSpaceNode  * aTBSNode )
{
    IDE_DASSERT( aTBSNode != NULL );

    IDE_TEST_RAISE ( SMI_TBS_IS_DISCARDED( aTBSNode->mHeader.mState),
                     error_already_discarded );

    aTBSNode->mHeader.mState = SMI_TBS_DISCARDED;

    IDE_TEST( smrRecoveryMgr::updateTBSNodeToAnchor( & aTBSNode->mHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_already_discarded );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TBS_ALREADY_DISCARDED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  화일의 이름을 바꾼다.
 *  sddDiskMgr::alterDataFileName 에 구현되어져 있다.
 **********************************************************************/
IDE_RC sdptbSpaceDDL::alterDataFileName( idvSQL      *aStatistics,
                                         scSpaceID    aSpaceID,
                                         SChar       *aOldName,
                                         SChar       *aNewName )
{

    IDE_TEST( sddDiskMgr::alterDataFileName( aStatistics,
                                             aSpaceID,
                                             aOldName,
                                             aNewName )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description:
 *   META/SERVICE단계에서 Tablespace를 Online상태로 변경한다.
 *
 *   [ 알고리즘 ]
 *     (010) TBSNode에 X락 획득
 *     (020) Tablespace를 Backup중이라면 Backup종료시까지 대기
 *     (030) "TBSNode.Status := ONLINE"에 대한 로깅
 *     (040) TBS Commit Pending등록
 *     (050) "DBFNode.Status := ONLINE"에 대한 로깅
 *     (060) DBF Commit Pending등록
 *     (note-1) Log anchor에 TBSNode를 flush하지 않는다.
 *              (commit pending으로 처리)
 *     (note-2) Log anchor에 DBFNode를 flush하지 않는다.
 *              (commit pending으로 처리)
 *
 *   [ Commit시 ]
 *     * TBS pending
 *     (c-010) TBSNode.Status := ONLINE    (주1)
 *     (c-020) Table의 Runtime정보 초기화
 *     (c-030) 해당 TBS에 속한 모든 Table에 대해 Index Header Rebuilding 실시
 *     (c-040) Flush TBSNode To LogAnchor
 *
 *     * DBF pending
 *     (c-030) DBFNode.Status := ONLINE    (주2)
 *     (c-040) Flush DBFNode To LogAnchor
 *
 *   [ Abort시 ]
 *     [ UNDO ] 수행
 *
 *   [ TBS REDO ]
 *     (r-010) (030)에 대한 REDO로 Commit Pending 등록
 *     (note-1) TBSNode를 loganchor에 flush하지 않음
 *              -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
 *     (note-2) Restart Recovery완료후 (070), (080)의 작업이 수행되므로
 *              Redo중에 이를 처리하지 않는다.
 *
 *   [ TBS UNDO ]
 *     (u-040) (030)에 대한 UNDO로 TBSNode.Status := Before Image(OFFLINE)
 *             -> TBSNode.Status가 Commit Pending에서 변경되기 때문에
 *                굳이 undo중에 Before Image로 덮어칠 필요는 없다.
 *                그러나 일관성을 유지하기 위해 TBSNode.Status를
 *                Before Image로 원복하도록 한다.
 *
 *     (note-1) TBSNode를 loganchor에 flush하지 않음
 *              -> ALTER TBS ONLINEE의 Commit Pending을 통해
 *                 COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문
 *
 *   [ 전제조건 ]
 *      이 함수는 META, SERVICE단계에서 ONLINE으로 올릴 경우에만 호출된다.
 *
 *   aTrans         - [IN] 상태를 변경하려는 Transaction
 *   aSpaceNode     - [IN] 상태를 변경할 Tablespace의 Node
 ************************************************************************/
IDE_RC sdptbSpaceDDL::alterTBSonline(idvSQL*              aStatistics,
                                     void               * aTrans,
                                     sddTableSpaceNode  * aSpaceNode )
{
    sctPendingOp    * sPendingOp;
    UInt              sBeforeState;
    UInt              sAfterState;
    sddDataFileNode * sFileNode;
    smLSN             sOnlineLSN;
    UInt              i;

    SM_LSN_INIT( sOnlineLSN );

    ///////////////////////////////////////////////////////////////////////////
    //  (010) TBSNode에 X락 획득
    //
    // Tablespace가 Offline상태여도 에러를 내지 않는다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                                   aTrans,
                                   & aSpaceNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   SCT_VAL_ALTER_TBS_ONOFF) /* validation */
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (e-010) Tablespace상태에 따른 에러처리
    IDE_TEST( sctTableSpaceMgr::checkError4AlterStatus(
                                      (sctTableSpaceNode*)aSpaceNode,
                                      SMI_TBS_ONLINE /* New State */ )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (020) Tablespace를 Backup중이라면 Backup종료시까지 대기
    IDE_TEST( sctTableSpaceMgr::wait4BackupAndBlockBackup( aStatistics,
                                                           (sctTableSpaceNode*)aSpaceNode,
                                                           SMI_TBS_SWITCHING_TO_ONLINE )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (030) "TBSNode.Status := ONLINE"에 대한 로깅

    sBeforeState = aSpaceNode->mHeader.mState ;

    // 로깅하기 전에 Backup관리자와의 동시성 제어를 위한 임시 Flag를 제거
    sBeforeState &= ~SMI_TBS_SWITCHING_TO_OFFLINE;
    sBeforeState &= ~SMI_TBS_SWITCHING_TO_ONLINE;

    sAfterState  = sBeforeState;
    sAfterState &= ~SMI_TBS_OFFLINE;
    sAfterState |=  SMI_TBS_ONLINE;

    IDE_TEST( smrUpdate::writeTBSAlterOnOff( aStatistics,
                                             aTrans,
                                             aSpaceNode->mHeader.mID,
                                             SCT_UPDATE_DRDB_ALTER_TBS_ONLINE,
                                             sBeforeState,
                                             sAfterState,
                                             &sOnlineLSN )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (040) TBS Commit Pending등록
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                  aTrans,
                  aSpaceNode->mHeader.mID,
                  ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                  SCT_POP_ALTER_TBS_ONLINE,
                  & sPendingOp ) != IDE_SUCCESS );

    sPendingOp->mPendingOpFunc = sdptbSpaceDDL::alterOnlineCommitPending;
    sPendingOp->mNewTBSState   = sAfterState;

    // fix BUG-17456 Disk Tablespace online이후 update 발생시 index 무한루프
    SM_GET_LSN( sPendingOp->mOnlineTBSLSN, sOnlineLSN );

    ///////////////////////////////////////////////////////////////////////////
    //  (050) DBF Online 로깅
    //  (060) DBF Commit Pending등록

    // Transaction Commit시에 수행할 DBFNode의 상태를
    // Offline으로 변경하는 Pending Operation 등록
    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL)
        {
            continue;
        }

        sBeforeState = sFileNode->mState ;

        sAfterState  = sBeforeState;
        sAfterState &= ~SMI_FILE_OFFLINE;
        sAfterState |=  SMI_FILE_ONLINE;

        IDE_TEST( smrUpdate::writeDiskDBFAlterOnOff(
                      aStatistics,
                      aTrans,
                      aSpaceNode->mHeader.mID,
                      sFileNode->mID,
                      SCT_UPDATE_DRDB_ALTER_DBF_ONLINE,
                      sBeforeState,
                      sAfterState )
                != IDE_SUCCESS );

        IDE_TEST( sddDataFile::addPendingOperation(
                  aTrans,
                  sFileNode,
                  ID_TRUE,        /* Pending 연산 수행 시점 : Commit 시 */
                  SCT_POP_ALTER_DBF_ONLINE,
                  &sPendingOp ) != IDE_SUCCESS );

        sPendingOp->mNewDBFState   = sAfterState;
        sPendingOp->mPendingOpFunc = NULL; // pending 시 처리할 함수가 없다.

        // PRJ-1548 User Memory Tablespace
        // TBS Node에 X 잠금을 획득하기 때문에 DBF Node에 X 잠금을
        // 획득할 필요가 없다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  META, SERVICE단계에서 Tablespace를 Offline상태로 변경한다.
 *
 *  [ 알고리즘 ]
 *    (010) TBSNode에 X락 획득
 *    (020) Tablespace를 Backup중이라면 Backup종료시까지 대기
 *    (030) "TBSNode.Status := Offline" 에 대한 로깅
 *    (040) TBSNode.OfflineSCN := Current System SCN
 *    (050) Instant Disk Aging 실시 - Aging 수행중에만 잠시 Ager 래치획득
 *    (060) Dirty Page Flush 실시
 *    (070) Commit Pending등록
 *
 *  [ Commit시 : (Pending) ]
 *    (c-010) TBSNode.Status := Offline
 *    (c-050) Free All Index Header of TBS
 *    (c-060) Free Runtime Info At Table Header
 *    (c-070) Free Runtime Info At TBSNode ( Expcet Lock )
 *    (c-080) flush TBSNode to loganchor
 *
 *  [ Abort시 ]
 *    [ UNDO ] 수행
 *
 *  [ REDO ]
 *    (u-010) (020)에 대한 REDO로 TBSNode.Status := After Image(OFFLINE)
 *    (note-1) TBSNode를 loganchor에 flush하지 않음
 *             -> Restart Recovery완료후 모든 TBS를 loganchor에 flush하기 때문
 *    (note-2) Commit Pending을 수행하지 않음
 *             -> Restart Recovery완료후 OFFLINE TBS에 대한 Resource해제를 한다
 *
 *  [ UNDO ]
 *    (u-010) (020)에 대한 UNDO로 TBSNode.Status := Before Image(ONLINE)
 *            TBSNode.Status가 Commit Pending에서 변경되기 때문에
 *            굳이 undo중에 Before Image로 덮어칠 필요는 없다.
 *            그러나 일관성을 유지하기 위해 TBSNode.Status를
 *            Before Image로 원복하도록 한다.
 *    (note-1) TBSNode를 loganchor에 flush하지 않음
 *             -> ALTER TBS OFFLINE의 Commit Pending을 통해
 *                COMMIT이후에야 변경된 TBS상태가 log anchor에 flush되기 때문
 *
 *  aTrans   - [IN] 상태를 변경하려는 Transaction
 *  aTBSNode - [IN] 상태를 변경할 Tablespace의 Node
 **********************************************************************/
IDE_RC sdptbSpaceDDL::alterTBSoffline( idvSQL*              aStatistics,
                                       void               * aTrans,
                                       sddTableSpaceNode  * aSpaceNode )
{
    UInt              sBeforeState;
    UInt              sAfterState;
    sctPendingOp    * sPendingOp;
    sddDataFileNode * sFileNode;
    UInt              i;

    ///////////////////////////////////////////////////////////////////////////
    //  (010) TBSNode에 X락 획득
    //
    // Tablespace가 Offline상태여도 에러를 내지 않는다.
    IDE_TEST( sctTableSpaceMgr::lockTBSNode(
                                   aTrans,
                                   & aSpaceNode->mHeader,
                                   ID_FALSE,   /* intent */
                                   ID_TRUE,    /* exclusive */
                                   SCT_VAL_ALTER_TBS_ONOFF) /* validation */
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (e-010) Tablespace상태에 따른 에러처리
    IDE_TEST( sctTableSpaceMgr::checkError4AlterStatus(
                                     (sctTableSpaceNode*)aSpaceNode,
                                     SMI_TBS_OFFLINE  /* New State */ )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (020) Tablespace를 Backup중이라면 Backup종료시까지 대기
    IDE_TEST( sctTableSpaceMgr::wait4BackupAndBlockBackup( aStatistics,
                                                           (sctTableSpaceNode*)aSpaceNode,
                                                           SMI_TBS_SWITCHING_TO_OFFLINE )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (030) "TBSNode.Status := Offline" 에 대한 로깅
    sBeforeState = aSpaceNode->mHeader.mState ;

    // 로깅하기 전에 Backup관리자와의 동시성 제어를 위한 임시 Flag를 제거
    sBeforeState &= ~SMI_TBS_SWITCHING_TO_OFFLINE;
    sBeforeState &= ~SMI_TBS_SWITCHING_TO_ONLINE;

    sAfterState  = sBeforeState;
    sAfterState &= ~SMI_TBS_ONLINE;
    sAfterState |=  SMI_TBS_OFFLINE;

    IDE_TEST( smrUpdate::writeTBSAlterOnOff( aStatistics,
                                             aTrans,
                                             aSpaceNode->mHeader.mID,
                                             SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE,
                                             sBeforeState,
                                             sAfterState,
                                             NULL )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (050) Instant Disk Aging 실시
    //        - Aging 수행중에만 잠시 Ager 래치획득
    /* xxxxxxxxxxxx
    IDE_TEST( smLayerCallback::doInstantAgingWithDiskTBS(
                  aStatistics,
                  aTrans,
                  aSpaceNode->mHeader.mID )
              != IDE_SUCCESS );
              */

    ///////////////////////////////////////////////////////////////////////////
    //  (060) all dirty pages flush in tablespace

    // invalidatePages 연산은 동시성이 고려되지 않기 때문에
    // 수행하는 동안에는 해당 테이블스페이스에 대한 DML도
    // 허용하지 않아야하고(X-LOCK) DISKGC도 Block되어 있어야
    // 한다. 위조건이 보장되지 않으면 Flush-List가 깨지는
    // 현상이 발생한다.
    // 트랜잭션이 완료될때 (commit or abort) disk GC를 unblock 한다.

    /* xxxxxxxxxxxxx
    smLayerCallback::blockDiskGC( aStatistics, aTrans );
    */

    //BUG-21392 table spabe offline 이후에 해당 table space에 속하는 BCB들이 buffer
    //에 남아 있습니다.

    /* 1.replacement flush를 통해 secondary buffer에 pageout대상 페이지가
       추가 발생할수 있어 flushPage호출 3.에서 pageOut 수행 */
    IDE_TEST( sdsBufferMgr::flushPagesInRange( aStatistics,
                                               aSpaceNode->mHeader.mID,/*aSpaceID*/
                                               0,                      /*StartPID*/
                                               SD_MAX_PAGE_COUNT - 1 )
              != IDE_SUCCESS );
    /* 2.buffer pool의 dirtypage가 최신 이므로 2nd->bufferpool의 순서로 수행 */
    IDE_TEST( sdbBufferMgr::pageOutInRange( aStatistics,
                                            aSpaceNode->mHeader.mID,
                                            0,
                                            SD_MAX_PAGE_COUNT - 1 )
              != IDE_SUCCESS );
    /* 3.replacement flush를 통해 secondary buffer에 pageout대상 페이지가 존재할수도 있습니다.*/
    IDE_TEST( sdsBufferMgr::pageOutInRange( aStatistics,
                                            aSpaceNode->mHeader.mID,/*aSpaceID*/
                                            0,                      /*StartPID*/
                                            SD_MAX_PAGE_COUNT - 1 )
              != IDE_SUCCESS );

    ///////////////////////////////////////////////////////////////////////////
    //  (070) Commit Pending등록
    //
    // Transaction Commit시에 수행할 Pending Operation등록
    IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                              aTrans,
                              aSpaceNode->mHeader.mID,
                              ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                              SCT_POP_ALTER_TBS_OFFLINE,
                              & sPendingOp )
              != IDE_SUCCESS );

    // Commit시 sctTableSpaceMgr::executePendingOperation에서
    // 수행할 Pending함수 설정
    sPendingOp->mPendingOpFunc = sdptbSpaceDDL::alterOfflineCommitPending;
    sPendingOp->mNewTBSState   = sAfterState;

    ///////////////////////////////////////////////////////////////////////////
    //  (080) DBF Online 로깅
    //  (090) DBF Commit Pending등록

    // Transaction Commit시에 수행할 DBFNode의 상태를
    // Offline으로 변경하는 Pending Operation 등록
    for (i=0; i < aSpaceNode->mNewFileID ; i++ )
    {
        sFileNode = aSpaceNode->mFileNodeArr[i] ;

        if( sFileNode == NULL)
        {
            continue;
        }

        sBeforeState = sFileNode->mState ;

        sAfterState  = sBeforeState;
        sAfterState &= ~SMI_FILE_ONLINE;
        sAfterState |=  SMI_FILE_OFFLINE;

        IDE_TEST( smrUpdate::writeDiskDBFAlterOnOff(
                                          aStatistics,
                                          aTrans,
                                          aSpaceNode->mHeader.mID,
                                          sFileNode->mID,
                                          SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE,
                                          sBeforeState,
                                          sAfterState ) != IDE_SUCCESS );

        IDE_TEST( sddDataFile::addPendingOperation(
                          aTrans,
                          sFileNode,
                          ID_TRUE,        /* Pending 연산 수행 시점 : Commit 시 */
                          SCT_POP_ALTER_DBF_OFFLINE,
                          &sPendingOp ) != IDE_SUCCESS );

        sPendingOp->mNewDBFState   = sAfterState;
        sPendingOp->mPendingOpFunc = NULL; // pending 시 처리할 함수가 없다.

        // PRJ-1548 User Memory Tablespace
        // TBS Node에 X 잠금을 획득하기 때문에 DBF Node에 X 잠금을
        // 획득할 필요가 없다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterOnlineCommitPending(
                                          idvSQL            * aStatistics,
                                          sctTableSpaceNode * aSpaceNode,
                                          sctPendingOp      * aPendingOp )
{
    sdpActOnlineArgs  sActionArgs;

    // 여기 들어오는 Tablespace는 항상 Disk Tablespace여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode->mID )
                == ID_TRUE );

    // SMI_TBS_SWITCHING_TO_OFFLINE 이 설정되어 있으면 안된다.
    IDE_ASSERT( ( aSpaceNode->mState & SMI_TBS_SWITCHING_TO_ONLINE )
                  == SMI_TBS_SWITCHING_TO_ONLINE );

    /////////////////////////////////////////////////////////////////////
    // (010) aSpaceNode.Status := ONLINE
    aSpaceNode->mState = aPendingOp->mNewTBSState;

    // SMI_TBS_SWITCHING_TO_OFFLINE 이 설정되어 있으면 안된다.
    IDE_ASSERT( ( aSpaceNode->mState & SMI_TBS_SWITCHING_TO_ONLINE )
                != SMI_TBS_SWITCHING_TO_ONLINE );

    if ( smrRecoveryMgr::isRestart() == ID_FALSE )
    {
        ///////////////////////////////////////////////////////////////////////////
        //  (020) Table의 Runtime정보 초기화
        //  (030) 해당 TBS에 속한 모든 Table에 대해 Index Header Rebuilding 실시
        //        TBS 상태를 ONLINE으로 변경한 후에 Index Header Rebuilding을
        //        수행하여야 데이타파일에 Read를 수행할 수 있다.

        sActionArgs.mTrans = NULL;

        IDE_TEST( smLayerCallback::alterTBSOnline4Tables( aStatistics,
                                                          (void*)&sActionArgs,
                                                          aSpaceNode->mID )
                  != IDE_SUCCESS );

        /////////////////////////////////////////////////////////////////////
        //  (040) flush aSpaceNode to loganchor
        IDE_TEST( smrRecoveryMgr::updateTBSNodeToAnchor( aSpaceNode )
                != IDE_SUCCESS );
    }
    else
    {
        // restart recovery시에는 상태만 변경한다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  PROJ-1548 User Memory Tablespace
 *
 *  Tablespace를 OFFLINE시킨 Tx가 Commit되었을 때 불리는 Pending함수
 *
 *  Tablespace와 관련된 모든 리소스를 반납한다.
 *  - 예외 : Tablespace의 Lock정보는 다른 Tx들이 대기하면서
 *            참조할 수 있기 때문에 해제해서는 안된다.
 *
 *   [참고] sctTableSpaceMgr::executePendingOperation 에서 호출된다.
 *
 *  [ 알고리즘 ] ======================================================
 *     (c-010) TBSNode.Status := OFFLINE
 *     (c-020) Free All Index Header of TBS
 *     (c-030) Destroy/Free Runtime Info At Table Header
 *     (c-040) flush TBSNode to loganchor
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterOfflineCommitPending(
                          idvSQL            * aStatistics,
                          sctTableSpaceNode * aSpaceNode,
                          sctPendingOp      * aPendingOp )
{
    IDE_DASSERT( aSpaceNode != NULL );

    // 여기 들어오는 Tablespace는 항상 Disk Tablespace여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode->mID )
                == ID_TRUE );

    // SMI_TBS_SWITCHING_TO_OFFLINE 이 설정되어 있어야 한다.
    IDE_ASSERT( ( aSpaceNode->mState & SMI_TBS_SWITCHING_TO_OFFLINE )
                  == SMI_TBS_SWITCHING_TO_OFFLINE );

    /////////////////////////////////////////////////////////////////////
    // (c-010) TBSNode.Status := OFFLINE
    aSpaceNode->mState = aPendingOp->mNewTBSState;

    // SMI_TBS_SWITCHING_TO_OFFLINE 이 설정되어 있으면 안된다.
    IDE_ASSERT( ( aSpaceNode->mState & SMI_TBS_SWITCHING_TO_OFFLINE )
                != SMI_TBS_SWITCHING_TO_OFFLINE );

    if ( smrRecoveryMgr::isRestart() == ID_FALSE )
    {
        /////////////////////////////////////////////////////////////////////
        //  (c-020) Free All Index Header of TBS
        //  (c-030) Destroy/Free Runtime Info At Table Header
        IDE_TEST( smLayerCallback::alterTBSOffline4Tables( aStatistics,
                                                           aSpaceNode->mID )
                  != IDE_SUCCESS );

        /////////////////////////////////////////////////////////////////////
        //  (c-050) flush TBSNode to loganchor
        IDE_TEST( smrRecoveryMgr::updateTBSNodeToAnchor( aSpaceNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // restart recovery 시에는 상태만 변경한다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description:
 *
 *    ALTER TABLESPACE ADD DATAFILE에 대한 Bitmap TBS의 Commit Pending연산
 *    을 수행한다.
 *
 *    추가된 데이타파일에 대해서 SpaceCache의 MaxGGID와 Freeness Bit를 반영한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceNode  - [IN] 테이블스페이스 노드 포인터
 * aPendingOp  - [IN] Pending연산 자료구조에 대한 포인터
 *
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterAddFileCommitPending(
                          idvSQL            * /* aStatistics */,
                          sctTableSpaceNode * aSpaceNode,
                          sctPendingOp      * aPendingOp )
{
    UInt              sGGID;
    sdptbSpaceCache * sCache;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aPendingOp != NULL );

    /* Restart Recovery시에는 호출되지 않는 Pending 연산이다.
     * 왜냐하면, Recovery 이후에 다시 SpaceCache는 refine 과정을 통해서
     * 다시 구하기 때문이다. */
    IDE_ASSERT( smrRecoveryMgr::isRestart() == ID_FALSE );

    sCache = (sdptbSpaceCache*)aPendingOp->mPendingOpParam;
    IDE_ASSERT( sCache != NULL );

    sGGID  = aPendingOp->mFileID;
    IDE_ASSERT( sGGID < SD_MAX_FID_COUNT );

    /* BUG-47666 mFreenessOfGGs에 동시성 제어가 필요합니다. */
    sdptbBit::atomicSetBit32( (UInt*)sCache->mFreenessOfGGs, sGGID );

    if ( sCache->mMaxGGID < sGGID )
    {
        sCache->mMaxGGID = sGGID;
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description:
 *
 *    ALTER TABLESPACE DROP DATAFILE에 대한 Bitmap TBS의 Commit Pending연산
 *    을 수행한다.
 *
 *    제거된 데이타파일에 대해서 SpaceCache의 MaxGGID와 Freeness Bit를 반영한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceNode  - [IN] 테이블스페이스 노드 포인터
 * aPendingOp  - [IN] Pending연산 자료구조에 대한 포인터
 *
 ***********************************************************************/
IDE_RC sdptbSpaceDDL::alterDropFileCommitPending(
                          idvSQL            * /* aStatistics */,
                          sctTableSpaceNode * aSpaceNode,
                          sctPendingOp      * aPendingOp )
{
    UInt              sGGID;
    sdptbSpaceCache * sCache;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aPendingOp != NULL );

    /* Restart Recovery시에는 호출되지 않는 Pending 연산이다.
     * 왜냐하면, Recovery 이후에 다시 SpaceCache는 refine 과정을 통해서
     * 다시 구하기 때문이다. */
    IDE_ASSERT( smrRecoveryMgr::isRestart() == ID_FALSE );

    // 여기 들어오는 Tablespace는 항상 Disk Tablespace여야 한다.
    IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace( aSpaceNode->mID )
                == ID_TRUE );

    sCache = (sdptbSpaceCache*)aPendingOp->mPendingOpParam;
    IDE_ASSERT( sCache != NULL );

    sGGID = aPendingOp->mFileID;
    IDE_ASSERT( sGGID < SD_MAX_FID_COUNT );

    //가장 마지막에 있는 파일을 삭제하는것이라면 max ggid를 수정해줄필요가 있다.
    if(  sCache->mMaxGGID == sGGID )
    {
        sCache->mMaxGGID--;
    }

    /* BUG-47666 mFreenessOfGGs에 동시성 제어가 필요합니다. */
    sdptbBit::atomicClearBit32( (UInt*)sCache->mFreenessOfGGs, sGGID );

    //만약 지금삭제하는파일이 힌트로 설정되있다면 힌트를 무조건 0으로
    //(이것은 꼭해주어야하는것은 아니다. 어차피 비트열검색의 시작지점으로만
    //사용되어지므로..)
    if(  sCache->mGGIDHint == sGGID )
    {
        sCache->mGGIDHint = 0;
    }

    return IDE_SUCCESS;
}
