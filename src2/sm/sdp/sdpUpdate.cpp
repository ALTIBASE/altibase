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
 * $Id: sdpUpdate.cpp 85791 2019-07-08 05:44:14Z et16 $
 **********************************************************************/

#include <smr.h>
#include <sdr.h>
#include <sdpDef.h>
#include <sdpReq.h>
#include <sdpPhyPage.h>
#include <sdpUpdate.h>
#include <sdptbGroup.h>
#include <sdptbSpaceDDL.h>
#include <sdpDPathInfoMgr.h>

/***********************************************************************
 * redo type:  SDR_SDP_1BYTE, SDR_SDP_2BYTE, SDR_SDP_4BYTE, SDR_SDP_8BYTE
 *             SDR_SDP_BINARY
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_NBYTE( SChar       * aData,
                                      UInt          aLength,
                                      UChar       * aPagePtr,
                                      sdrRedoInfo * /*aRedoInfo*/,
                                      sdrMtx      * aMtx )
{

    IDE_ERROR( aData != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx != NULL );
    IDE_ERROR( (aLength == SDR_SDP_1BYTE) ||
               (aLength == SDR_SDP_2BYTE) ||
               (aLength == SDR_SDP_4BYTE) ||
               (aLength == SDR_SDP_8BYTE) );

    idlOS::memcpy(aPagePtr, aData, aLength);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * redo type: SDR_SDP_BINARY
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_BINARY( SChar       * aData,
                                       UInt          aLength,
                                       UChar       * aPagePtr,
                                       sdrRedoInfo * /*aRedoInfo*/,
                                       sdrMtx      * aMtx )
{

    IDE_ERROR( aData != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx != NULL );

    if( aLength == SD_PAGE_SIZE)
    {
        IDE_ERROR( aPagePtr == sdpPhyPage::getPageStartPtr( aPagePtr));
        /*
         * sdbFrameHdr에 보면 mBCBPtr이라는 member가 있다.  이것은 현 frame에 대한
         * 정보를 가지고 있는 sdbBCB에 대한 포인터이다.  이것을 통해서 frame에
         * 대한 BCB를 얻을 수가 있다.
         * 어떠한 페이지의 전체 image를 로깅하고 이것을 redo한다고 해보자.
         * 로깅할 당시의 frame에 대한 BCB와 redo하는 시점에서 frame에대한
         * BCB는 다른 값이다. 즉, mBCBPtr은 로깅되어선 안되는 값이고,
         * 그렇기 때문에 당연히 redo가 되어서도 안된다.
         * */
        idlOS::memcpy(aPagePtr + ID_SIZEOF(sdbFrameHdr),
                      aData + ID_SIZEOF(sdbFrameHdr),
                      aLength - ID_SIZEOF(sdbFrameHdr));
    }
    else
    {
        idlOS::memcpy(aPagePtr, aData, aLength);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * redo type: SDR_SDP_WRITE_PAGEIMG, SDR_SDP_DPATH_INS_PAGE
 *            Backup 중이나, Direct-Path INSERT로 수행된
 *            Page 전체에 대한 Image log redo
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_WRITE_PAGEIMG( SChar       * aData,
                                              UInt          aLength,
                                              UChar       * aPagePtr,
                                              sdrRedoInfo * /* aRedoInfo */,
                                              sdrMtx      * aMtx )
{
    IDE_ERROR( aData     != NULL );
    IDE_ERROR( aPagePtr  != NULL );
    IDE_ERROR( aMtx      != NULL );
    IDE_ERROR( aLength   == SD_PAGE_SIZE );
    IDE_ERROR( aPagePtr  == sdpPhyPage::getPageStartPtr(aPagePtr));

    /*
     * sdbFrameHdr에 보면 mBCBPtr이라는 member가 있다.  이것은 현 frame에 대한
     * 정보를 가지고 있는 sdbBCB에 대한 포인터이다.  이것을 통해서 frame에
     * 대한 BCB를 얻을 수가 있다.
     * 어떠한 페이지의 전체 image를 로깅하고 이것을 redo한다고 해보자.
     * 로깅할 당시의 frame에 대한 BCB와 redo하는 시점에서 frame에대한
     * BCB는 다른 값이다. 즉, mBCBPtr은 로깅되어선 안되는 값이고,
     * 그렇기 때문에 당연히 redo가 되어서도 안된다.
     * */
    idlOS::memcpy( aPagePtr + ID_SIZEOF(sdbFrameHdr),
                   aData + ID_SIZEOF(sdbFrameHdr),
                   aLength - ID_SIZEOF(sdbFrameHdr));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDP_PAGE_CONSISTENT
 *            Page의 Consistent 상태에 대한 log
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_PAGE_CONSISTENT( SChar       * aData,
                                                UInt          aLength,
                                                UChar       * aPagePtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx*       /*aMtx*/ )
{
    sdpPhyPageHdr* sPagePhyHdr = (sdpPhyPageHdr*)aPagePtr;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aLength  == 1 );
    IDE_ERROR( ( *aData == SDP_PAGE_INCONSISTENT ) ||
               ( *aData == SDP_PAGE_CONSISTENT ) );

    sPagePhyHdr->mIsConsistent = *aData ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 * redo type:  SDR_SDP_INIT_PHYSICAL_PAGE
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_INIT_PHYSICAL_PAGE( SChar       * aData,
                                                   UInt          aLength,
                                                   UChar       * aPagePtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx      * /*aMtx*/ )
{
    sdrInitPageInfo  sInitPageInfo;
    sdpParentInfo    sParentInfo;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aLength  == ID_SIZEOF( sdrInitPageInfo ) );

    idlOS::memcpy( &sInitPageInfo, aData, ID_SIZEOF(sInitPageInfo) );

    sParentInfo.mParentPID   = sInitPageInfo.mParentPID;
    sParentInfo.mIdxInParent = sInitPageInfo.mIdxInParent;

    IDE_TEST( sdpPhyPage::initialize( sdpPhyPage::getHdr(aPagePtr),
                                      sInitPageInfo.mPageID,
                                      &sParentInfo,
                                      sInitPageInfo.mPageState,
                                      (sdpPageType)sInitPageInfo.mPageType,
                                      sInitPageInfo.mTableOID,
                                      sInitPageInfo.mIndexID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  SDR_SDP_INIT_LOGICAL_HDR
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_INIT_LOGICAL_HDR( SChar       * aData,
                                                 UInt          aLength,
                                                 UChar       * aPagePtr,
                                                 sdrRedoInfo * /*aRedoInfo*/,
                                                 sdrMtx      * /*aMtx*/ )
{
    UInt sLogicalHdrSize;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aLength  == ID_SIZEOF(UInt) );

    idlOS::memcpy(&sLogicalHdrSize, aData, aLength);

    sdpPhyPage::initLogicalHdr( sdpPhyPage::getHdr(aPagePtr),
                                sLogicalHdrSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  SDR_SDP_INIT_SLOT_DIRECTORY
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_INIT_SLOT_DIRECTORY( SChar       * /*aData*/,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx      * /*aMtx*/ )
{
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aLength  == 0 );

    sdpSlotDirectory::init( sdpPhyPage::getHdr(aPagePtr) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  SDR_SDP_FREE_SLOT
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_FREE_SLOT( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx*         /*aMtx*/ )
{

    sdpPhyPageHdr*  sPageHdr;
    UChar*          sFreeSlotPtr;
    UShort          sSlotLen;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aLength   > 0 );
    IDE_ERROR( aPagePtr != NULL );

    sFreeSlotPtr = aPagePtr;

    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sFreeSlotPtr);
    IDE_ERROR( sdpPhyPage::getPageType(sPageHdr) != SDP_PAGE_DATA );

    IDE_ERROR( aLength == ID_SIZEOF(UShort) );
    idlOS::memcpy(&sSlotLen, aData, ID_SIZEOF(UShort));

    IDE_TEST( sdpPhyPage::freeSlot(sPageHdr,
                                   sFreeSlotPtr,
                                   sSlotLen,
                                   ID_TRUE,
                                   SDP_8BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  SDR_SDP_FREE_SLOT_FOR_SID
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_FREE_SLOT_FOR_SID( SChar       * aData,
                                                  UInt          aLength,
                                                  UChar       * aPagePtr,
                                                  sdrRedoInfo * aRedoInfo,
                                                  sdrMtx      * /* aMtx */ )
{

    sdpPhyPageHdr*  sPageHdr;
    UChar*          sFreeSlotPtr;
    scSlotNum       sSlotNum;
    UShort          sSlotLen;

    IDE_ERROR( aData     != NULL );
    IDE_ERROR( aLength    > 0 );
    IDE_ERROR( aPagePtr  != NULL );
    IDE_ERROR( aRedoInfo != NULL );

    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr);
    IDE_ERROR( sdpPhyPage::getPageType(sPageHdr) == SDP_PAGE_DATA );

    IDE_ERROR( aLength == ID_SIZEOF(UShort) );
    idlOS::memcpy(&sSlotLen, aData, ID_SIZEOF(UShort));

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                                sSlotNum,
                                &sFreeSlotPtr)
              != IDE_SUCCESS );

    IDE_DASSERT( sSlotLen == smLayerCallback::getRowPieceSize( sFreeSlotPtr ) );

    IDE_TEST( sdpPhyPage::freeSlot4SID( sPageHdr,
                                        sFreeSlotPtr,
                                        sSlotNum,
                                        sSlotLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  SDR_SDP_RESTORE_FREESPACE_CREDIT
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_RESTORE_FREESPACE_CREDIT( SChar       *   aData,
                                                         UInt            aLength,
                                                         UChar       *   aPagePtr,
                                                         sdrRedoInfo * /*aRedoInfo*/,
                                                         sdrMtx      * /*aMtx*/ )
{

    sdpPhyPageHdr*  sPageHdr;
    UShort          sRestoreSize;
    UShort          sAvailableFreeSize;

    IDE_ERROR( aPagePtr != NULL );

    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr);

    IDE_ERROR( aLength == ID_SIZEOF(UShort) );
    idlOS::memcpy(&sRestoreSize, aData, ID_SIZEOF(UShort));

    sAvailableFreeSize =
        sdpPhyPage::getAvailableFreeSize(sPageHdr);

    sAvailableFreeSize += sRestoreSize;

    sdpPhyPage::setAvailableFreeSize( sPageHdr,
                                      sAvailableFreeSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_SDP_RESET_PAGE
 ***********************************************************************/
IDE_RC sdpUpdate::redo_SDR_SDP_RESET_PAGE( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * /* aRedoInfo */,
                                           sdrMtx      * /* aMtx */ )
{
    UInt      sLogicalHdrSize;

    IDE_ERROR( aData != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aLength == ID_SIZEOF(UInt) );

    idlOS::memcpy(&sLogicalHdrSize, aData, aLength);

    IDE_TEST( sdpPhyPage::reset(sdpPhyPage::getHdr(aPagePtr),
                                sLogicalHdrSize,
                                NULL) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * undo type: SDR_OP_SDP_DPATH_ADD_SEGINFO
 ******************************************************************************/
IDE_RC sdpUpdate::undo_SDR_OP_SDP_DPATH_ADD_SEGINFO(
                                        idvSQL          * /*aStatistics*/,
                                        sdrMtx          * aMtx,
                                        scSpaceID         /*aSpaceID*/,
                                        UInt              aSegInfoSeqNo )
{
    void              * sTrans;
    smTID               sTID;
    sdpDPathInfo      * sDPathInfo;
    sdpDPathSegInfo   * sDPathSegInfo;
    smuList           * sBaseNode;
    smuList           * sCurNode;
    
    sDPathSegInfo = NULL;

    //-----------------------------------------------------------------------
    // 본 undo 함수는 Rollback 시에만 사용되는 함수로, Restart Recovery와는
    // 관련이 없다. 따라서 Restart Recovery가 아닐 때만 다음 코드를 수행한다.
    //-----------------------------------------------------------------------
    if ( smLayerCallback::isRestartRecoveryPhase() == ID_FALSE )
    {
        sTrans      = sdrMiniTrans::getTrans( aMtx );
        sTID        = smLayerCallback::getTransID( sTrans );
        sDPathInfo  = (sdpDPathInfo*)smLayerCallback::getDPathInfo( sTrans );

        if( sDPathInfo == NULL )
        {
            ideLog::log( IDE_DUMP_0,
                         "Undo Trans ID : %u",
                         sTID );
            (void)smLayerCallback::dumpDPathEntry( sTrans );

            IDE_ERROR( 0 );
        }

        sBaseNode = &sDPathInfo->mSegInfoList;
        sCurNode = SMU_LIST_GET_LAST(sBaseNode);
        sDPathSegInfo = (sdpDPathSegInfo*)sCurNode;

        //-----------------------------------------------------------------
        // 로깅된 DPathSegInfo의 SeqNo와 현재 메모리에 존재하는 마지막
        // DPathSegInfo의 SeqNo가 일치하는지 확인하여 Rollback을 수행한다.
        //-----------------------------------------------------------------
        if( (sDPathSegInfo != NULL) && (sDPathSegInfo->mSeqNo == aSegInfoSeqNo) )
        {
            IDE_ERROR( sdpDPathInfoMgr::destDPathSegInfo(
                                            sDPathInfo,
                                            sDPathSegInfo,
                                            ID_TRUE ) // Move LastFlag
                        == IDE_SUCCESS );
        }
        else
        {
            //----------------------------------------------------------------
            // 로깅된 DPathSegInfo의 SeqNo가 메모리에 존재하는 마지막
            // DPathSegInfo의 SeqNo와 일치하지 않는 경우.
            //
            // 로깅이 완료 되었다면, 로깅된 SeqNo의 DPathSegInfo이 메모리에
            // 존재해야 하는데, 존재하지 않는다는 것은 메모리에 정상적이지 않은
            // 변경이 발생했다는 의미이므로 ASSERT 처리한다.
            //----------------------------------------------------------------

            ideLog::log( IDE_DUMP_0,
                         "Requested DPath INSERT Undo Seq. No : %u",
                         aSegInfoSeqNo );
            (void)sdpDPathInfoMgr::dumpDPathInfo( sDPathInfo );
            IDE_ERROR( 0 );
        }
    }
    else
    {
        // Restart Recovery인 경우, 무시한다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

