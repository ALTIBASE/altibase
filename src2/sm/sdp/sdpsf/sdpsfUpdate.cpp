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
 * $Id:$
 **********************************************************************/

# include <sdr.h>
# include <sdpReq.h>
# include <sdpsfSH.h>
# include <sdpSglRIDList.h>
# include <sdpsfSH.h>
# include <sdpsfExtMgr.h>
# include <sdpsfUpdate.h>

/***********************************************************************
 * Description : Segment Merge연산에 대한 Undo함수이다. 두 Segment의
 *               Extent List와 Target Segment의 마지막 Extent의 NxtRID를
 *               SD_NULL_RID로 설정한다.
 *
 * aStatistics          - [IN] 통계정보
 * aMtx                 - [IN] Mini Transaction Pointer
 * aSpaceID             - [IN] SpaceID
 * aToSegRID            - [IN] ToSeg의 PID
 * aLstExtRIDOfToSeg    - [IN] ToSeg의 마지막 Alloc ExtRID
 * aFstPIDOfLstAllocExt - [IN] ToSeg의 마지막 Alloc Extent의 First PID
 * aFmtPageCntOfToSeg   - [IN] ToSeg의 Format Page Count
 * aHWMOfToSeg          - [IN] ToSeg의 HWM PID
 ***********************************************************************/
IDE_RC sdpsfUpdate::undo_SDPSF_MERGE_SEG_4DPATH( idvSQL     * aStatistics,
                                                 sdrMtx     * aMtx,
                                                 scSpaceID    aSpaceID,
                                                 scPageID     aToSegPID,
                                                 sdRID        aLstAllocExtRID,
                                                 scPageID     aFstPIDOfLstAllocExt,
                                                 ULong        aFmtPageCntOfToSeg,
                                                 scPageID     aHWMOfToSeg )
{
    sdpsfSegHdr *sToSegHdr;

    IDE_ERROR( aMtx                 != NULL );
    IDE_ERROR( aSpaceID             != 0 );
    IDE_ERROR( aToSegPID            != SD_NULL_PID );
    IDE_ERROR( aLstAllocExtRID      != SD_NULL_RID );
    IDE_ERROR( aFstPIDOfLstAllocExt != SD_NULL_PID );
    IDE_ERROR( aFmtPageCntOfToSeg   != 0 );
    IDE_ERROR( aHWMOfToSeg          != SD_NULL_PID );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aToSegPID,
                                               &sToSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setAllocExtRID( aMtx, sToSegHdr, aLstAllocExtRID )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setFstPIDOfAllocExt( aMtx,
                                            sToSegHdr,
                                            aFstPIDOfLstAllocExt )
                  != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setFmtPageCnt( aMtx,
                                      sToSegHdr,
                                      aFmtPageCntOfToSeg )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setHWM( aMtx, sToSegHdr, aHWMOfToSeg )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 할당했던 페이지를 Free하여 Segment에 UFmtPageList에
 *               추가한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] SpaceID
 * aSegPID      - [IN] Seg의 PID
 * aPageID      - [IN] 할당했던 PageID
 ***********************************************************************/
IDE_RC sdpsfUpdate::undo_SDPSF_ALLOC_PAGE( idvSQL    * aStatistics,
                                           sdrMtx    * aMtx,
                                           scSpaceID   aSpaceID,
                                           scPageID    aSegPID,
                                           scPageID    aPageID )
{
    sdpsfSegHdr  *sSegHdr;
    UChar        *sPagePtr;

    IDE_ERROR( aMtx                 != NULL );
    IDE_ERROR( aSpaceID             != 0 );
    IDE_ERROR( aSegPID              != SD_NULL_PID );
    IDE_ERROR( aPageID              != SD_NULL_RID );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aPageID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL /*aTrySuccess*/,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfUFmtPIDList::add2Head( aMtx,
                                          sSegHdr,
                                          sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SegHdr의 AllocExtRID, HWM, FmtPageCnt 을 원복한다.
 *
 * LogType: SDR_OP_SDPSF_UPDATE_HWMINFO_4DPATH
 *
 * aStatistics       - [IN] 통계정보
 * aMtx              - [IN] Mini Transaction Pointer
 * aSpaceID          - [IN] SpaceID
 * aSegPID           - [IN] Seg의 PID
 * aAllocExtRID      - [IN] Alloc Extent RID
 * aFstPIDOfAllocExt - [IN] Alloc Extent의 First PID
 * aHWM              - [IN] HWM
 * aFmtPageCnt       - [IN] Format Page Count
 ***********************************************************************/
IDE_RC sdpsfUpdate::undo_UPDATE_HWM_4DPATH( idvSQL    * aStatistics,
                                            sdrMtx    * aMtx,
                                            scSpaceID   aSpaceID,
                                            scPageID    aSegPID,
                                            sdRID       aAllocExtRID,
                                            scPageID    aFstPIDOfAllocExt,
                                            scPageID    aHWM,
                                            ULong       aFmtPageCnt )
{
    sdpsfSegHdr  *sSegHdr;

    IDE_ERROR( aMtx              != NULL );
    IDE_ERROR( aSpaceID          != 0 );
    IDE_ERROR( aSegPID           != SD_NULL_PID );
    IDE_ERROR( aAllocExtRID      != SD_NULL_RID );
    IDE_ERROR( aFstPIDOfAllocExt != SD_NULL_PID );
    IDE_ERROR( aFmtPageCnt       != 0 );
    IDE_ERROR( aHWM                != SD_NULL_PID );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setAllocExtRID( aMtx,
                                       sSegHdr,
                                       aAllocExtRID )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setFstPIDOfAllocExt( aMtx, sSegHdr, aFstPIDOfAllocExt )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setHWM( aMtx, sSegHdr, aHWM )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setFmtPageCnt( aMtx, sSegHdr, aFmtPageCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Private PID List에 대한 Add연산을 취소한다.
 *
 * LogType : SDR_OP_SDPSF_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] SpaceID
 * aSegPID      - [IN] Seg의 PID
 * aFstPID      - [IN] 첫번째 PageID
 * aLstPID      - [IN] 마지막 PageID
 * aPageCnt     - [IN] Page갯수
 ***********************************************************************/
IDE_RC sdpsfUpdate::undo_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH( idvSQL    * aStatistics,
                                                            sdrMtx    * aMtx,
                                                            scSpaceID   aSpaceID,
                                                            scPageID    aSegPID,
                                                            scPageID    aFstPID,
                                                            scPageID    aLstPID,
                                                            ULong       aPageCnt )
{
    sdpsfSegHdr       *sSegHdr;
    sdpSglPIDListBase *sPvtFreePIDList;

    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR( aSpaceID != 0 );
    IDE_ERROR( aSegPID  != SD_NULL_PID );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                               aMtx,
                                               aSpaceID,
                                               aSegPID,
                                               &sSegHdr )
              != IDE_SUCCESS );

    sPvtFreePIDList = sdpsfSH::getPvtFreePIDList( sSegHdr );

    /* Private Page List의 Head 페이지를 원복한다. */
    IDE_TEST( sdpSglPIDList::setHeadOfList( sPvtFreePIDList,
                                            aFstPID,
                                            aMtx )
              != IDE_SUCCESS );

    IDE_TEST( sdpSglPIDList::setTailOfList( sPvtFreePIDList,
                                            aLstPID,
                                            aMtx )
              != IDE_SUCCESS );

    /* Private Page List의 페이지 갯수를 원복한다. */
    IDE_TEST( sdpSglPIDList::setNodeCnt( sPvtFreePIDList,
                                         aPageCnt,
                                         aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

