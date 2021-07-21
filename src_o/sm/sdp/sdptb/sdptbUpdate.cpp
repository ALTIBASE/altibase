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
 * $Id:
 **********************************************************************/
#include <smr.h>
#include <sdr.h>
#include <sdpDef.h>
#include <sdpReq.h>
#include <sdpPhyPage.h>
#include <sdptb.h>
#include <sdptbUpdate.h>
#include <sdptbGroup.h>

/***********************************************************************
 * Description:
 *  redo type:  SDR_SDPTB_INIT_LGHDR_PAGE
 ***********************************************************************/
IDE_RC sdptbUpdate::redo_SDPTB_INIT_LGHDR_PAGE( SChar       * aData,
                                                UInt          aLength,
                                                UChar       * aPagePtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx      * /*aMtx*/ )
{
    sdptbData4InitLGHdr     sData4Init;
    sdptbLGHdr          *   sLGHdr;

    IDE_DASSERT( aData != NULL );
    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aLength == ID_SIZEOF( sdptbData4InitLGHdr ));

    idlOS::memcpy( &sData4Init, aData, aLength );

    sLGHdr = sdptbGroup::getLGHdr( aPagePtr );

    sdptbGroup::initBitmapOfLG( (UChar*)sLGHdr,   //LG header
                                sData4Init.mBitVal,
                                sData4Init.mStartIdx,
                                sData4Init.mCount );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description:
 *  redo type:  SDR_SDPTB_ALLOC_IN_LG
 ***********************************************************************/
IDE_RC sdptbUpdate::redo_SDPTB_ALLOC_IN_LG( SChar       * aData,
                                            UInt          aLength,
                                            UChar       * aPagePtr,
                                            sdrRedoInfo * /*aRedoInfo*/,
                                            sdrMtx      * /*aMtx*/ )
{
    sdpPageType sIdx;

    IDE_DASSERT( aData != NULL );
    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aLength == ID_SIZEOF( sdpPageType ) );

    idlOS::memcpy(&sIdx, aData, aLength);

    sdptbExtent::allocByBitmapIndex( sdptbGroup::getLGHdr(aPagePtr), sIdx );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description:
 *  redo type:  SDR_SDPTB_FREE_IN_LG
 ***********************************************************************/
IDE_RC sdptbUpdate::redo_SDPTB_FREE_IN_LG( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * /*aMtx*/ )
{
    sdpPageType sIdx;

    IDE_DASSERT( aData != NULL );
    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aLength == ID_SIZEOF( sdpPageType ) );

    idlOS::memcpy(&sIdx, aData, aLength);

    sdptbExtent::freeByBitmapIndex( sdptbGroup::getLGHdr(aPagePtr), sIdx );

    return IDE_SUCCESS;
}


/***********************************************************************
 *
 * Description : 할당했던 ExtDir 페이지를 다시 Free List에 추가한다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] SpaceID
 * aFreeListIdx - [IN] Seg의 PID
 * aExtDirPID   - [IN] 할당했던 PageID
 *
 ***********************************************************************/
IDE_RC sdptbUpdate::undo_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST(
                                           idvSQL          * aStatistics,
                                           sdrMtx          * aMtx,
                                           scSpaceID         aSpaceID,
                                           sdpFreeExtDirType aFreeListIdx,
                                           scPageID          aExtDirPID )
{
    sdptbGGHdr      * sGGHdrPtr;
    UChar           * sPagePtr;
    idBool            sTrySuccess;
    sdptbSpaceCache * sSpaceCache;

    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aExtDirPID    != SD_NULL_PID );
    IDE_ASSERT( (aFreeListIdx == SDP_TSS_FREE_EXTDIR_LIST) ||
                (aFreeListIdx == SDP_UDS_FREE_EXTDIR_LIST) );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          SDPTB_GET_GGHDR_PID_BY_FID(0),
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sTrySuccess,
                                          NULL ) != IDE_SUCCESS);

    sGGHdrPtr = sdptbGroup::getGGHdr( sPagePtr );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aExtDirPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          NULL,
                                          NULL ) != IDE_SUCCESS );

    IDE_TEST( sdpSglPIDList::addNode2Head(
                                &(sGGHdrPtr->mArrFreeExtDirList[aFreeListIdx]),
                                sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*)sPagePtr ),
                                aMtx )
              != IDE_SUCCESS );

    /*
     * 일반적으로는 Restart Recovery이후에 Undo Tablespace가 Reset되지만,
     * Prepare Tx가 존재할 경우에는 Reset하지 않으므로 Recovery에서는 SpaceCache까지
     * 복원해놓아야 한다
     */
    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ASSERT( sSpaceCache != NULL );
    sSpaceCache->mArrIsFreeExtDir[ aFreeListIdx ] = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
