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
 * $Id: sdpSegment.cpp 87185 2020-04-08 04:26:15Z jiwon.kim $
 **********************************************************************/
# include <smErrorCode.h>
# include <sdr.h>
# include <sdb.h>
# include <smr.h>
# include <sdpDef.h>
# include <sdpReq.h>
# include <sdpDef.h>
# include <sdpSegDescMgr.h>
# include <sdpSegment.h>
# include <sdpPhyPage.h>

IDE_RC sdpSegment::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC sdpSegment::destroy()
{
    return IDE_SUCCESS;
}

IDE_RC sdpSegment::initCommonCache( sdpSegCCache * aCommonCache,
                                    sdpSegType     aSegType,
                                    UInt           aPageCntInExt,
                                    smOID          aTableOID,
                                    UInt           aIndexID )
{
    aCommonCache->mTmpSegHead     = SD_NULL_PID;
    aCommonCache->mTmpSegTail     = SD_NULL_PID;

    aCommonCache->mSegType        = aSegType;
    aCommonCache->mPageCntInExt   = aPageCntInExt ;
    aCommonCache->mSegSizeByBytes = 0;

    aCommonCache->mMetaPID        = SD_NULL_PID;

    /* BUG-31372: It is needed that the amount of space used in
     * Segment is refered. */
    aCommonCache->mFreeSegSizeByBytes = 0;

    aCommonCache->mTableOID = aTableOID;
    aCommonCache->mIndexID  = aIndexID;

    return IDE_SUCCESS;
}

/* PROJ-1671 LOB Segment에 대한 Segment Handle을 생성하고, 초기화한다.*/
IDE_RC sdpSegment::allocLOBSegDesc( smiColumn * aColumn,
                                    smOID       aTableOID )
{
    UInt               sState = 0;
    sdpSegmentDesc   * sSegDesc;
    scPageID           sSegPID;

    IDE_ASSERT( aColumn != NULL );

    /* sdpSegment_allocLOBSegDesc_malloc_SegDesc.tc */
    IDU_FIT_POINT("sdpSegment::allocLOBSegDesc::malloc::SegDesc");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SDP,
                                 ID_SIZEOF(sdpSegmentDesc),
                                 (void **)&(sSegDesc),
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );
    sState = 1;

    sdpSegDescMgr::setDefaultSegAttr(
        &(sSegDesc->mSegHandle.mSegAttr),
        SDP_SEG_TYPE_LOB);
    sdpSegDescMgr::setDefaultSegStoAttr(
        &(sSegDesc->mSegHandle.mSegStoAttr) );

    sSegPID = SC_MAKE_PID( aColumn->colSeg );

    IDE_TEST( sdpSegDescMgr::initSegDesc(
                             sSegDesc,
                             aColumn->colSpace,
                             sSegPID,
                             SDP_SEG_TYPE_LOB,
                             aTableOID,
                             SM_NULL_INDEX_ID )
              != IDE_SUCCESS );

    aColumn->descSeg = (void*)sSegDesc;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sSegDesc ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* PROJ-1671 LOB Segment에 대한 Segment Desc을 생성하고, 초기화한다.*/
IDE_RC sdpSegment::freeLOBSegDesc( smiColumn * aColumn )
{
    sdpSegmentDesc * sSegmentDesc;

    IDE_ASSERT( aColumn != NULL );
    sSegmentDesc = (sdpSegmentDesc*)aColumn->descSeg;

    //XXX if가 아니라 명확하게 처리해야 한다.
    if( sSegmentDesc != NULL )
    {
        IDE_ASSERT( sdpSegDescMgr::destSegDesc( sSegmentDesc )
                    == IDE_SUCCESS );
     
        IDE_ASSERT( iduMemMgr::free( sSegmentDesc ) == IDE_SUCCESS );
        aColumn->descSeg = NULL;
    }

    return IDE_SUCCESS;
}

IDE_RC sdpSegment::createSegment( idvSQL            * aStatistics,
                                  void              * aTrans,
                                  scSpaceID           aTableSpaceID,
                                  sdrMtxLogMode       aLogMode,
                                  sdpSegHandle      * aSegHandle,
                                  scPageID          * aSegPID )
{
    sdrMtx         sMtx;
    smLSN          sNTA;
    SInt           sState = 0;
    sdpSegMgmtOp * sSegMgmtOp;
    ULong          sNTAData;

    *aSegPID = SD_NULL_PID;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aTableSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    if( aTrans != NULL )
    {
        IDE_ASSERT( aLogMode == SDR_MTX_LOGGING );
        sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );
    }

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   aLogMode,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSegMgmtOp->mCreateSegment(
                  aStatistics,
                  &sMtx,
                  aTableSpaceID,
                  SDP_SEG_TYPE_TABLE,
                  aSegHandle ) /* Table Segment는 specific data가 없다 */
              != IDE_SUCCESS );


    if( aTrans != NULL )
    {
        IDE_ASSERT( aLogMode == SDR_MTX_LOGGING );

        sNTAData = aSegHandle->mSegPID;

        sdrMiniTrans::setNTA( &sMtx,
                              aTableSpaceID,
                              SDR_OP_SDP_CREATE_TABLE_SEGMENT,
                              &sNTA,
                              (ULong*)&sNTAData,
                              1 /* Data Count */);
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );


    *aSegPID = aSegHandle->mSegPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    *aSegPID = SD_NULL_PID;

    return IDE_FAILURE;
}

IDE_RC sdpSegment::dropSegment( idvSQL        * aStatistics,
                                void          * aTrans,
                                scSpaceID       aSpaceID,
                                sdpSegHandle  * aSegHandle )
{
    sdrMtx           sMtx;
    sdpSegMgmtOp   * sSegMgmtOp;
    UInt             sState = 0;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST( sdrMiniTrans::begin( NULL,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
                != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSegMgmtOp->mDropSegment( aStatistics,
                                        &sMtx,
                                        aSpaceID,
                                        aSegHandle->mSegPID )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page list entry에 table segment desc 할당
 *
 * disk page list entry의 초기화시에 segment 할당을 위해 호출되어 수행된다.
 * smc layer에서 mtx begin하고, 이 함수를 빠져나가면 SDR_OP_SDP_TABLE_CREATE_SEGMENT의
 * NTA 설정 후 mtx commit하게 된다.
 *
 * + 2nd. code design
 *   - NTA를 위한 LSN 저장
 *   - segment를 할당할 tablespace id를 인자로 넘겨 해당 tablespace에 속한
 *     segment desc의 RID를 얻는다.
 *   - table segment 생성이후 table segment를 위해 예약된 page를 생성한다.
 *   - table segment RID와 table segment hdr page ID를 로깅한다.(redo-undo function 존재)
 *   - table hdr page를 dirty page로 등록한다.
 *   - 첫번째 page를 fix하여 table meta page hdr를 초기화 한다.
 ***********************************************************************/
IDE_RC sdpSegment::allocTableSeg4Entry( idvSQL               * aStatistics,
                                        void                 * aTrans,
                                        scSpaceID              aTableSpaceID,
                                        smOID                  aTableOID,
                                        sdpPageListEntry     * aPageEntry,
                                        sdrMtxLogMode          aLogMode )
{
    sdpSegHandle *sSegHandle;

    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID )  == ID_TRUE );
    IDE_DASSERT( aPageEntry != NULL );
    IDE_DASSERT( aTableOID  != SM_NULL_OID );

    if( aLogMode == SDR_MTX_LOGGING )
    {
        IDE_ASSERT( aTrans != NULL );
    }

    /* Temp Table생성시 No Logging으로 온다. */
    if( aLogMode == SDR_MTX_NOLOGGING )
    {
        IDE_ASSERT( aTrans == NULL );
    }

    sSegHandle = &aPageEntry->mSegDesc.mSegHandle;

    IDE_TEST( createSegment( aStatistics,
                             aTrans,
                             aTableSpaceID,
                             aLogMode,
                             sSegHandle,
                             &( aPageEntry->mSegDesc.mSegHandle.mSegPID ) )
              != IDE_SUCCESS );

    if( aTrans != NULL )
    {
        IDE_ASSERT( aLogMode == SDR_MTX_LOGGING );

        IDE_TEST( smrUpdate::updateTableSegPIDAtTableHead(
                      aStatistics,
                      aTrans,
                      SM_MAKE_PID(aTableOID),
                      SM_MAKE_OFFSET(aTableOID)
                      + smLayerCallback::getSlotSize(),
                      aPageEntry,
                      sSegHandle->mSegPID )
                  != IDE_SUCCESS );

        /* FIT/ART/sm/Bugs/BUG-19151/BUG-19151.tc */
        IDU_FIT_POINT( "1.BUG-19151@sdpSegment::allocTableSeg4Entry" );

        IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                SM_MAKE_PID(aTableOID))
                  != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : static index header에 index segment desc 할당
 *
 * smc layer에서 mtx begin하고, 이 함수를 빠져나가면 SDR_OP_SDP_CREATE_SEGMENT의
 * NTA 설정 후 mtx commit하게 된다.
 *
 * + 2nd. code design
 *   - NTA를 위한 LSN 저장
 *   - segment를 할당할 tablespace id를 인자로 넘겨 해당 tablespace에 속한
 *     segment desc의 RID를 얻는다.
 *   - index  segment 생성이후 index segment를 위해 예약된 page를 생성한다.
 *   - index segment RID와  segment hdr page ID를 로깅한다.(redo-undo function 존재)
 *   - index hdr page를 dirty page로 등록한다.
 *   - 첫번째 page를 fix하여 index meta page hdr를 초기화 한다.
 ***********************************************************************/
IDE_RC sdpSegment::allocIndexSeg4Entry( idvSQL             * aStatistics,
                                        void*               aTrans,
                                        scSpaceID           aTableSpaceID,
                                        smOID               aTableOID,
                                        smOID               aIndexOID,
                                        UInt                aIndexID,
                                        UInt                aType,
                                        UInt                aBuildFlag,
                                        sdrMtxLogMode       aLogMode,
                                        smiSegAttr        * aSegAttr,
                                        smiSegStorageAttr * aSegStoAttr )
{
    sdrMtx               sMtx;
    scGRID*              sIndexSegGRID;
    scPageID             sSegPID;
    scGRID               sBeforeSegGRID;
    scOffset             sOffset;
    scPageID             sIndexPID;
    SChar*               sIndexPagePtr;
    void*                sIndexHeader;
    UInt                 sState = 0;
    UChar              * sMetaPtr;
    smLSN                sNTA;
    sdpSegMgmtOp       * sSegMgmtOP;
    sdpSegmentDesc       sSegDesc;
    scPageID             sMetaPageID;
    ULong                sNTAData;
    sdpPageType          sMetaPageType;

    IDE_DASSERT( ((aLogMode == SDR_MTX_LOGGING) && aTrans != NULL) ||
                 ((aLogMode == SDR_MTX_NOLOGGING) && aTrans == NULL) );
    IDE_DASSERT( sctTableSpaceMgr::isDiskTableSpace( aTableSpaceID )
                 == ID_TRUE );

    sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

    // Index Segment Handle에 Segment Cache를 초기화한다.
    IDE_TEST( sdpSegDescMgr::initSegDesc(
                                  &sSegDesc,
                                  aTableSpaceID,
                                  SD_NULL_PID, // Segment 생성전
                                  SDP_SEG_TYPE_INDEX,
                                  aTableOID,
                                  aIndexID) != IDE_SUCCESS );

    /*
     * INITRANS와 MAXTRANS를 설정한다.
     * PCTFREE과 PCTUSED를 사용하지 않지만 설정한다.
     */
    sdpSegDescMgr::setDefaultSegAttr( &(sSegDesc.mSegHandle.mSegAttr),
                                      SDP_SEG_TYPE_INDEX );

    // STORAGE 속성을 설정한다. 
    sdpSegDescMgr::setSegAttr( &sSegDesc, aSegAttr );
    // STORAGE 속성을 설정한다. 
    sdpSegDescMgr::setSegStoAttr( &sSegDesc, aSegStoAttr );

    IDE_TEST( sdrMiniTrans::begin(aStatistics,
                                  &sMtx,
                                  aTrans,
                                  aLogMode,
                                  ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                  SM_DLOG_ATTR_DEFAULT)
              != IDE_SUCCESS );
    sState = 1;

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aTableSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mCreateSegment( aStatistics,
                                          &sMtx,
                                          aTableSpaceID,
                                          SDP_SEG_TYPE_INDEX,
                                          &sSegDesc.mSegHandle )
              != IDE_SUCCESS );

    if( aType == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
    {
        sMetaPageType = SDP_PAGE_INDEX_META_RTREE;
    }
    else // if( aType == SMI_BUILTIN_B_TREE_INDEXTYPE_ID )
    {
        sMetaPageType = SDP_PAGE_INDEX_META_BTREE;
    }
    
    IDE_TEST( sSegMgmtOP->mAllocNewPage( aStatistics,
                                         &sMtx,
                                         aTableSpaceID,
                                         &sSegDesc.mSegHandle,
                                         // SDP_PAGE_INDEX_META,
                                         sMetaPageType,
                                         &sMetaPtr )
              != IDE_SUCCESS );

    sMetaPageID = sdpPhyPage::getPageIDFromPtr( sMetaPtr );

    IDE_TEST( sSegMgmtOP->mSetMetaPID( aStatistics,
                                       &sMtx,
                                       aTableSpaceID,
                                       sSegDesc.mSegHandle.mSegPID,
                                       0, /* Meta PID Array Index */
                                       sMetaPageID )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::initIndexMetaPage( sMetaPtr,
                                                  aType,
                                                  aBuildFlag,
                                                  &sMtx )
              != IDE_SUCCESS );

    sNTAData = sSegDesc.mSegHandle.mSegPID;

    sdrMiniTrans::setNTA( &sMtx,
                          aTableSpaceID,
                          SDR_OP_SDP_CREATE_INDEX_SEGMENT,
                          &sNTA,
                          &sNTAData,
                          1 /* Data Count */);

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    // 이후에 initIndex에서 다시 초기화한다.
    IDE_TEST( sdpSegDescMgr::destSegDesc( &sSegDesc )
              != IDE_SUCCESS );

    sSegPID    = sSegDesc.mSegHandle.mSegPID;
    sIndexPID  = SM_MAKE_PID(aIndexOID);

    // index page 포인터 계산
    IDE_ASSERT( smmManager::getPersPagePtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            sIndexPID,
                                            (void**)&sIndexPagePtr )
                == IDE_SUCCESS );

    // index header 구함
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aIndexOID,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );
    // index header의 seg rID 위치 계산
    sIndexSegGRID  =  smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );

    SC_MAKE_GRID( *sIndexSegGRID,
                  aTableSpaceID,
                  sSegPID,
                  0 );

    if ( aLogMode == SDR_MTX_LOGGING )
    {
        SC_MAKE_NULL_GRID( sBeforeSegGRID );
        sOffset = ((SChar*)sIndexSegGRID - sIndexPagePtr);

        /* Update type:  SMR_PHYSICAL    */
        IDE_TEST( smrUpdate::physicalUpdate(
                      aStatistics,
                      aTrans,
                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                      sIndexPID,
                      sOffset,
                      (SChar*)&sBeforeSegGRID,
                      ID_SIZEOF(scGRID),
                      (SChar*)sIndexSegGRID,
                      ID_SIZEOF(scGRID),
                      NULL,
                      0)
                  != IDE_SUCCESS );


    }

    if (aLogMode == SDR_MTX_LOGGING)
    {
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sIndexPID )
                  != IDE_SUCCESS );
    }

    /* ------------------------------------------------
     * !! CHECK RECOVERY POINT
     * case) index segment를 할당하고 crash가 발생 경우이다.
     * restart시 undoAll 과정에서 segment는 nta logical undo를 통해
     * free가 되며, table header의 mIndexOID에 해당 index header의
     * 기록된 index segment rid는 NULL로 undo 되도록 처리되고,
     * 새롭게 할당받은 index varcolumn은 free 되도록 한다.
     * ----------------------------------------------*/

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

/*
 * Column(smiColumn)에 LOB Segment 할당
 */
IDE_RC sdpSegment::allocLobSeg4Entry( idvSQL            *aStatistics,
                                      void*             aTrans,
                                      smiColumn*        aLobColumn,
                                      smOID             aLobColOID,
                                      smOID             aTableOID,
                                      sdrMtxLogMode     aLogMode )
{
    sdrMtx               sMtx;
    scPageID             sSegPID;
    scGRID               sSegGRID;
    scGRID               sBeforeSegGRID;
    scOffset             sOffset;
    scPageID             sLobColPID;
    SChar*               sLobColPagePtr;
    UInt                 sState = 0;
    sdpSegHandle       * sSegHandle;
    smLSN                sNTA;
    ULong                sNTAData;
    sdpSegMgmtOp       * sSegMgmtOP;
    UChar              * sMetaPtr;
    scPageID             sMetaPageID;
    

    IDE_DASSERT( (aLogMode == SDR_MTX_LOGGING && aTrans != NULL) ||
                 (aLogMode == SDR_MTX_NOLOGGING && aTrans == NULL) );

    sNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

    /* PROJ-1671 LOB Segment에 대한 Segment Desc을 생성하고, 초기화한다.*/
    IDE_TEST( allocLOBSegDesc( aLobColumn,
                               aTableOID )
              != IDE_SUCCESS );
    
    sSegHandle = sdpSegDescMgr::getSegHandle( aLobColumn );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   aLogMode,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aLobColumn->colSpace );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );
 
    IDE_TEST( sSegMgmtOP->mCreateSegment( aStatistics,
                                          &sMtx,
                                          aLobColumn->colSpace,
                                          SDP_SEG_TYPE_LOB,
                                          sSegHandle ) 
              != IDE_SUCCESS );

    IDE_TEST( sSegMgmtOP->mAllocNewPage( aStatistics,
                                         &sMtx,
                                         aLobColumn->colSpace,
                                         sSegHandle,
                                         SDP_PAGE_LOB_META,
                                         &sMetaPtr )
              != IDE_SUCCESS );

    sMetaPageID = sdpPhyPage::getPageIDFromPtr( sMetaPtr );

    IDE_TEST( sSegMgmtOP->mSetMetaPID( aStatistics,
                                       &sMtx,
                                       aLobColumn->colSpace,
                                       sSegHandle->mSegPID,
                                       0, /* Meta PID Array Index */
                                       sMetaPageID )
              != IDE_SUCCESS );

    ((sdpSegCCache*)(sSegHandle->mCache))->mMetaPID = sMetaPageID;

    IDE_TEST( smLayerCallback::initLobMetaPage( sMetaPtr,
                                                aLobColumn,
                                                &sMtx )
              != IDE_SUCCESS );

    sNTAData = sSegHandle->mSegPID;
    
    sdrMiniTrans::setNTA( &sMtx,
                          aLobColumn->colSpace,
                          SDR_OP_SDP_CREATE_LOB_SEGMENT,
                          &sNTA,
                          &sNTAData,
                          1 /* Data Count */);

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    sLobColPID = SM_MAKE_PID(aLobColOID);

    sSegPID = sSegHandle->mSegPID;

    SC_MAKE_GRID( sSegGRID,
                  aLobColumn->colSpace,
                  sSegPID,
                  0 );

    // column이 저장된  page 포인터 계산
    IDE_ASSERT( smmManager::getPersPagePtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            sLobColPID,
                                            (void**)&sLobColPagePtr )
                == IDE_SUCCESS );

    if (aLogMode == SDR_MTX_LOGGING)
    {
        SC_MAKE_NULL_GRID( sBeforeSegGRID );

        sOffset       = ( (SChar*) &(aLobColumn->colSeg) - sLobColPagePtr );

        /* Update type:  SMR_PHYSICAL    */
        IDE_TEST( smrUpdate::physicalUpdate(
                      aStatistics,
                      aTrans,
                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                      sLobColPID,
                      sOffset,
                      (SChar*)&sBeforeSegGRID,
                      ID_SIZEOF(scGRID),
                      (SChar*)&sSegGRID,
                      ID_SIZEOF(scGRID),
                      NULL,
                      0)
                  != IDE_SUCCESS );

    }

    SC_COPY_GRID( sSegGRID, aLobColumn->colSeg );

    if (aLogMode == SDR_MTX_LOGGING)
    {
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                      sLobColPID)
                  != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

/*
 * Lob Segment 초기화
 */
IDE_RC sdpSegment::initLobSegDesc( smiColumn * aLobColumn )
{
    sdpSegHandle       * sSegHandle;
    sdpSegMgmtOp       * sSegMgmtOP;
    scPageID             sMetaPageID;
    
    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aLobColumn->colSpace );
    sSegHandle = sdpSegDescMgr::getSegHandle( aLobColumn );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mGetMetaPID( NULL, /* aStatistics */
                                       aLobColumn->colSpace,
                                       sSegHandle->mSegPID,
                                       0, /* Meta PID Array Index */
                                       &sMetaPageID )
              != IDE_SUCCESS );
    
    ((sdpSegCCache*)(sSegHandle->mCache))->mMetaPID = sMetaPageID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * page list entry의 table segment 해제
 */
IDE_RC sdpSegment::freeTableSeg4Entry(  idvSQL           * aStatistics,
                                        scSpaceID          aSpaceID,
                                        void             * aTrans,
                                        smOID              aTableOID,
                                        sdpPageListEntry * aPageEntry,
                                        sdrMtxLogMode      aLogMode )
{

    sdrMtx   sMtx;
    UInt     sState = 0;

    IDE_DASSERT( aPageEntry != NULL );
    IDE_DASSERT( aTableOID != SM_NULL_OID );
    IDE_DASSERT( (aLogMode == SDR_MTX_LOGGING && aTrans != NULL) ||
                 (aLogMode == SDR_MTX_NOLOGGING && aTrans == NULL) );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   aLogMode,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    // 실패할 경우 mtx rollback 처리한다.
    IDE_TEST( freeTableSeg4Entry( aStatistics,
                                  aSpaceID,
                                  aTableOID,
                                  aPageEntry,
                                  &sMtx) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    /* ------------------------------------------------
     * !! CHECK RECOVERY POINT
     * case) drop table pending시 table의 segment를 free하고 crash가 발생 경우이다.
     * restart시 disk GC의 tx는 abort이 되긴해도 undo에 대한 아무런 일도 하지 않는다.
     * 즉, table segment는 지워진채로 다시 restart가 되고, refine catalog 단계에서
     * drop table pending이 다시 호출되게 된다.
     * ----------------------------------------------*/

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}


/*
 * Table 혹은 Index Segment에 Extent 확장
 */
IDE_RC sdpSegment::allocExts(  idvSQL           * aStatistics,
                               scSpaceID          aSpaceID,
                               void             * aTrans,
                               sdpSegmentDesc   * aSegDesc,
                               ULong              aExtendSize )
{
    UInt                   sExtCnt;
    UInt                   sPageCnt;
    sdrMtxStartInfo        sStartInfo;
    sdpSpaceCacheCommon  * sCache;
    sdpSegMgmtOp         * sSegMgmtOP;

    IDE_ASSERT( aTrans   != NULL );
    IDE_ASSERT( aSegDesc != NULL );

    sStartInfo.mTrans   = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    sCache  = (sdpSpaceCacheCommon*)sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ASSERT( sCache != NULL );

    if( aExtendSize > 0 )
    {
        sPageCnt = ( aExtendSize/ SD_PAGE_SIZE );
        sPageCnt = ( sPageCnt > 0  ? sPageCnt : 1 );
        sPageCnt = idlOS::align( sPageCnt, sCache->mPagesPerExt );
        sExtCnt  = sPageCnt / sCache->mPagesPerExt;

        sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aSegDesc );
        IDE_ERROR( sSegMgmtOP != NULL );  

        IDE_TEST( sSegMgmtOP->mExtendSegment( aStatistics,
                                              &sStartInfo,
                                              aSpaceID,
                                              &aSegDesc->mSegHandle,
                                              sExtCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Description : page list entry의 table segment 해제
 *
 * disk page list entry의 table segment desc RID가 SD_NULL_RID가 아닐경우
 * 호출되어 수행된다. smc layer에서 mtx begin하고, 이 함수를 빠져나가면
 * SMR_OP_NULL의 NTA 설정 후 mtx commit하게 된다.
 *
 * + 2nd. code design
 *   - table segment RID와 table segment hdr page ID를 로깅한다.(redo-undo function 존재)
 *   - table hdr page를 dirty page로 등록한다.
 *   - segment를 할당할 tablespace id를 인자로 넘겨 해당 tablespace에
 *     속한 segment desc를 free함
 * BUGBUG : NULL RID일때 들어와서는 안되나 들어오고 있다.
 ***********************************************************************/
IDE_RC sdpSegment::freeTableSeg4Entry( idvSQL            * aStatistics,
                                        scSpaceID          aSpaceID,
                                        smOID              aTableOID,
                                        sdpPageListEntry * aPageEntry,
                                        sdrMtx           * aMtx )
{

    void*              sTrans;
    smLSN              sNTA;
    idBool             sIsExist;
    idBool             sIsOffline;
    sdrMtxLogMode      sLoggingMode;
    sdpSegMgmtOp     * sSegMgmtOP;

    IDE_DASSERT( aPageEntry != NULL );
    IDE_DASSERT( aTableOID != SM_NULL_OID );
    IDE_DASSERT( aMtx != NULL );

    sTrans       = sdrMiniTrans::getTrans(aMtx);
    sLoggingMode = sdrMiniTrans::getLogMode(aMtx);

    if ( sLoggingMode == SDR_MTX_LOGGING )
    {
        sNTA = smLayerCallback::getLstUndoNxtLSN( sTrans );

        /* Update type:  SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT */
        IDE_TEST( smrUpdate::updateTableSegPIDAtTableHead(
                                              aStatistics,
                                              sTrans,
                                              SM_MAKE_PID(aTableOID),
                                              SM_MAKE_OFFSET(aTableOID)
                                              + smLayerCallback::getSlotSize(),
                                              aPageEntry,
                                              SD_NULL_PID )
                  != IDE_SUCCESS );


        sdrMiniTrans::setNTA( aMtx,
                              aSpaceID,
                              SDR_OP_NULL,
                              &sNTA,
                              NULL,
                              0 /* Data Count */);
    }

    sIsExist = sddDiskMgr::isValidPageID(
                            aStatistics,
                            aSpaceID,
                            aPageEntry->mSegDesc.mSegHandle.mSegPID );

    if ( sIsExist == ID_TRUE )
    {
        // fix BUG-18132
        sIsOffline = sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_FREE_SEG_DISK_TBS );

        if ( sIsOffline == ID_FALSE )
        {
            sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aPageEntry );
            IDE_ERROR( sSegMgmtOP != NULL );

            IDE_TEST( sSegMgmtOP->mDropSegment( aStatistics,
                                                aMtx,
                                                aSpaceID,
                                                aPageEntry->mSegDesc.mSegHandle.mSegPID ) 
                      != IDE_SUCCESS );
        }
        else
        {
            // drop tablespace시 offline 관련된 TBS의
            // Table/Index/Lob 세그먼트는 free하지 않는다.
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_DPAGE_WARNNING4,
                        aPageEntry->mSegDesc.mSegHandle.mSegPID,
                        aSpaceID);
        }
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_DPAGE_WARNNING2,
                    aPageEntry->mSegDesc.mSegHandle.mSegPID,
                    aSpaceID);
    }

    aPageEntry->mSegDesc.mSegHandle.mSegPID  = SD_NULL_RID;

    if (sLoggingMode == SDR_MTX_LOGGING)
    {
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                SM_MAKE_PID(aTableOID))
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * Table Segment 리셋 ( for Temporary )
 *
 * disk page list entry의 table segment desc의 모든 page를 free 하고,
 * entry를 초기화 한다.
 */
IDE_RC sdpSegment::resetTableSeg4Entry( idvSQL           *aStatistics,
                                        scSpaceID         aSpaceID,
                                        sdpPageListEntry *aPageEntry )
{
    sdrMtx          sMtx;
    UInt            sState = 0;
    sdpSegMgmtOp  * sSegMgmtOP;

    IDE_DASSERT( aPageEntry != NULL );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_NOLOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aPageEntry );
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mResetSegment( aStatistics,
                                         &sMtx,
                                         aSpaceID,
                                         &(aPageEntry->mSegDesc.mSegHandle),
                                         SDP_SEG_TYPE_TABLE )
              != IDE_SUCCESS );

    aPageEntry->mRecCnt = 0;

    /*
     * Segment 속성과 Segment Storage 속성과 Segment 공간 인터페이스 모듈은
     * 재설정하지 않는다.
     */
    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : static index header의 index segment 해제 : wrapper
 ***********************************************************************/
IDE_RC sdpSegment::freeIndexSeg4Entry(  idvSQL           *aStatistics,
                                         scSpaceID         aSpaceID,
                                         void*             aTrans,
                                         smOID             aIndexOID,
                                         sdrMtxLogMode     aLogMode )
{

    sdrMtx   sMtx;
    UInt     sState = 0;

    IDE_DASSERT( aIndexOID != SM_NULL_OID );
    IDE_DASSERT( (aLogMode == SDR_MTX_LOGGING && aTrans != NULL) ||
                 (aLogMode == SDR_MTX_NOLOGGING && aTrans == NULL) );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   aLogMode,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    // 실패할 경우 mtx rollback 처리한다.
    IDE_TEST( freeIndexSeg4Entry( aStatistics,
                                  aSpaceID,
                                  aIndexOID,
                                  &sMtx) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    /* ------------------------------------------------
     * !! CHECK RECOVERY POINT
     * case) drop disk index pending시 index의 segment를 free하고 crash가 발생 경우이다.
     * restart시 disk GC의 tx는 abort이 되긴해도 undo에 대한 아무런 일도 하지 않는다.
     * 즉, index segment는 지워진채로 다시 restart가 되고, refine catalog 단계에서
     * drop table pending이 다시 호출되어 이미 free된 index segment는 skip하게 된다.
     * ----------------------------------------------*/

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}


/***********************************************************************
 * Description : static index header의 index segment 해제
 ***********************************************************************/
IDE_RC sdpSegment::freeIndexSeg4Entry( idvSQL           *aStatistics,
                                        scSpaceID         aSpaceID,
                                        smOID             aIndexOID,
                                        sdrMtx*           aMtx )
{
    void             * sTrans;
    smLSN              sNTA;
    sdrMtxLogMode      sLoggingMode;
    scGRID           * sIndexSegGRID;
    scGRID             sNullGRID;
    scOffset           sOffset;
    scPageID           sIndexPID;
    SChar            * sIndexPagePtr;
    void             * sIndexHeader;
    idBool             sIsExist;
    idBool             sIsOffline;
    scPageID           sTmpPID;
    sdpSegMgmtOp     * sSegMgmtOP = NULL;
    ULong              sNTAData;

    IDE_DASSERT( aIndexOID != SM_NULL_OID );
    IDE_DASSERT( aMtx != NULL );

    sTrans       = sdrMiniTrans::getTrans(aMtx);
    sLoggingMode = sdrMiniTrans::getLogMode(aMtx);

    sIndexPID     = SM_MAKE_PID(aIndexOID);

    IDE_ASSERT( smmManager::getPersPagePtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            sIndexPID,
                                            (void**)&sIndexPagePtr )
                == IDE_SUCCESS );
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       aIndexOID,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );
    // index header의 seg rID 위치 계산
    sIndexSegGRID  =  smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );

    IDE_DASSERT( SC_GRID_IS_NOT_NULL(*sIndexSegGRID) );
    IDE_ASSERT( aSpaceID == SC_MAKE_SPACE(*sIndexSegGRID) );
    IDE_DASSERT( (sctTableSpaceMgr::isSystemMemTableSpace( aSpaceID ) == ID_FALSE) &&
                 (sctTableSpaceMgr::isUndoTableSpace( aSpaceID ) == ID_FALSE) );   

    sTmpPID = SC_MAKE_PID(*sIndexSegGRID);

    if (sLoggingMode == SDR_MTX_LOGGING)
    {
        sNTA = smLayerCallback::getLstUndoNxtLSN( sTrans );

        sOffset       = ((SChar*)sIndexSegGRID - sIndexPagePtr);
        SC_MAKE_NULL_GRID( sNullGRID );

        /* Update type:  SMR_PHYSICAL    */
        IDE_TEST( smrUpdate::physicalUpdate(
                                  aStatistics,
                                  sTrans,
                                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                  sIndexPID,
                                  sOffset,
                                  (SChar*)sIndexSegGRID,
                                  ID_SIZEOF(scGRID),
                                  (SChar*)&sNullGRID,
                                  ID_SIZEOF(scGRID),
                                  NULL,
                                  0)
                  != IDE_SUCCESS );


        sNTAData = sTmpPID;
        sdrMiniTrans::setNTA(aMtx,
                             aSpaceID,
                             SDR_OP_NULL,
                             &sNTA,
                             (ULong*)&sNTAData,
                             1 /* Data Count */);
    }

    sIsExist = sddDiskMgr::isValidPageID( aStatistics,
                                          aSpaceID,
                                          sTmpPID );

    if ( sIsExist == ID_TRUE )
    {
        // fix BUG-18132
        sIsOffline = sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_FREE_SEG_DISK_TBS );

        if ( sIsOffline == ID_FALSE )
        {
            sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aSpaceID );
            // codesonar::Null Pointer Dereference
            IDE_ERROR( sSegMgmtOP != NULL );

            IDE_TEST( sSegMgmtOP->mDropSegment( aStatistics,
                                                aMtx,
                                                aSpaceID,
                                                sTmpPID )
                      != IDE_SUCCESS );
        }
        else
        {
            // drop tablespace시 offline 관련된 TBS의
            // Table/Index/Lob 세그먼트는 free하지 않는다.
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_DPAGE_WARNNING5,
                        sTmpPID,
                        aSpaceID);
        }
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_DPAGE_WARNNING3,
                    sTmpPID,
                    aSpaceID);
    }

    SC_MAKE_NULL_GRID( *sIndexSegGRID );

    if ( sLoggingMode == SDR_MTX_LOGGING )
    {
        IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                                 sIndexPID )
                  != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : lob column의 lob  segment 해제
 ***********************************************************************/
IDE_RC sdpSegment::freeLobSeg(idvSQL           *aStatistics,
                               void*             aTrans,
                               smOID             aLobColOID,
                               smiColumn*        aLobCol,
                               sdrMtxLogMode     aLogMode )
{

    sdrMtx   sMtx;
    UInt     sState = 0;

    IDE_DASSERT( aLobColOID != SM_NULL_OID );
    IDE_DASSERT( ( (aLogMode == SDR_MTX_LOGGING) && (aTrans != NULL) ) ||
                 ( (aLogMode == SDR_MTX_NOLOGGING) && (aTrans == NULL) ) );
    IDE_TEST_CONT( SC_GRID_IS_NULL( aLobCol->colSeg), skipFreeSeg);

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   aLogMode,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    // 실패할 경우 mtx rollback 처리한다.
    IDE_TEST( freeLobSeg( aStatistics,
                          aLobColOID,
                          aLobCol,
                          &sMtx ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    /* PROJ-1671 LOB Segment에 대한 Segment Desc를 해제한다. */
    IDE_TEST( freeLOBSegDesc( aLobCol ) != IDE_SUCCESS );

    /* ------------------------------------------------
     * !! CHECK RECOVERY POINT
     * ----------------------------------------------*/

    IDE_EXCEPTION_CONT(skipFreeSeg);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)sdrMiniTrans::rollback(&sMtx);
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : lob column의 lob  segment 해제
 ***********************************************************************/
IDE_RC sdpSegment::freeLobSeg( idvSQL          * aStatistics,
                               smOID             aLobColOID,
                               smiColumn       * aLobCol,
                               sdrMtx          * aMtx )
{
    void             * sTrans;
    smLSN              sNTA;
    sdrMtxLogMode      sLoggingMode;
    scGRID             sSegGRID;
    scGRID             sBeforeSegGRID;
    scOffset           sOffset;
    scPageID           sLobColPID;
    SChar            * sLobColPagePtr;
    idBool             sIsExist;
    idBool             sIsOffline;
    scSpaceID          sSpaceID;
    sdpSegMgmtOp     * sSegMgmtOP;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( aLobColOID != SM_NULL_OID);

    sTrans = sdrMiniTrans::getTrans(aMtx);
    sLoggingMode = sdrMiniTrans::getLogMode(aMtx);
    sLobColPID  = SM_MAKE_PID(aLobColOID);
    IDE_ASSERT( smmManager::getPersPagePtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            sLobColPID,
                                            (void**)&sLobColPagePtr )
                == IDE_SUCCESS );


    sSpaceID = SC_MAKE_SPACE(aLobCol->colSeg);

    IDE_DASSERT( (sctTableSpaceMgr::isSystemMemTableSpace( sSpaceID ) == ID_FALSE) &&
                 (sctTableSpaceMgr::isUndoTableSpace( sSpaceID ) == ID_FALSE) );   

    if ( sLoggingMode == SDR_MTX_LOGGING )
    {
        sNTA    = smLayerCallback::getLstUndoNxtLSN( sTrans );
        SC_COPY_GRID( aLobCol->colSeg, sBeforeSegGRID );
        sOffset = (SChar*)(&(aLobCol->colSeg)) - sLobColPagePtr;

        SC_MAKE_NULL_GRID( sSegGRID );

        /* Update type:  SMR_PHYSICAL    */
        IDE_TEST( smrUpdate::physicalUpdate( aStatistics,
                                             sTrans,
                                             SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                             sLobColPID,
                                             sOffset,
                                             (SChar*)&sBeforeSegGRID,
                                             ID_SIZEOF(scGRID),
                                             (SChar*)&sSegGRID,
                                             ID_SIZEOF(scGRID),
                                             NULL,
                                             0)
                  != IDE_SUCCESS );

        sdrMiniTrans::setNTA( aMtx,
                              sSpaceID,
                              SDR_OP_NULL,
                              &sNTA,
                              NULL,
                              0 /* Data Count */);
    }

    sIsExist = sddDiskMgr::isValidPageID( aStatistics,
                                          SC_MAKE_SPACE(aLobCol->colSeg),
                                          SC_MAKE_PID(aLobCol->colSeg));

    if (sIsExist == ID_TRUE)
    {
        // fix BUG-18132
        sIsOffline = sctTableSpaceMgr::hasState( SC_MAKE_SPACE(aLobCol->colSeg),
                                                 SCT_SS_SKIP_FREE_SEG_DISK_TBS );

        if ( sIsOffline == ID_FALSE )
        {
            sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( sSpaceID );
            IDE_ERROR( sSegMgmtOP != NULL );

            /* BUG-21545: Drop Table Pending연산에서 비정상 종료시 Table의 Drop Flag는
             *            설정되어 있어 Restart시 Disk Refine에서 Skip되어 Lob Column의
             *            Segment Desc가 초기화 되지 않고 NULL설정됩니다. 하여 여기서
             *            참조하면 않됩니다. */
            IDE_TEST( sSegMgmtOP->mDropSegment( aStatistics,
                                                aMtx,
                                                SC_MAKE_SPACE(aLobCol->colSeg),
                                                SC_MAKE_PID(aLobCol->colSeg) )
                      != IDE_SUCCESS );
        }
        else
        {
            // drop tablespace시 offline 관련된 TBS의
            // Table/Index/Lob 세그먼트는 free하지 않는다.
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_DPAGE_WARNNING4,
                        SC_MAKE_PID(aLobCol->colSeg),
                        SC_MAKE_SPACE(aLobCol->colSeg));
        }
    }
    else
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    SM_TRC_DPAGE_WARNNING2,
                    SC_MAKE_PID(aLobCol->colSeg),
                    SC_MAKE_SPACE(aLobCol->colSeg));
    }

    SC_MAKE_NULL_GRID( aLobCol->colSeg );

    if ( sLoggingMode == SDR_MTX_LOGGING )
    {
        IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                 sLobColPID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSegment::getSegInfo( idvSQL          *aStatistics,
                               scSpaceID        aSpaceID,
                               scPageID         aSegPID,
                               sdpSegInfo      *aSegInfo )
{
    sdpSegMgmtOp  *sSegMgmtOP;

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mGetSegInfo( aStatistics,
                                       aSpaceID,
                                       aSegPID,
                                       NULL, /* aTableHeader */
                                       aSegInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSegment::freeSeg4OperUndo( idvSQL      *aStatistics,
                                     scSpaceID    aSpaceID,
                                     scPageID     aSegPID,
                                     sdrOPType    aOpType,
                                     sdrMtx      *aMtx )
{
    sdpSegMgmtOp  *sSegMgmtOP;

    IDE_ASSERT( (aOpType == SDR_OP_SDP_CREATE_TABLE_SEGMENT) ||
                (aOpType == SDR_OP_SDP_CREATE_INDEX_SEGMENT) ||
                (aOpType == SDR_OP_SDP_CREATE_LOB_SEGMENT ) );

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mDropSegment( aStatistics,
                                        aMtx,
                                        aSpaceID,
                                        aSegPID )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
