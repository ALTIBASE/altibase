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
 ***********************************************************************/

#include <smxTrans.h>

#include <sdr.h>
#include <sdp.h>
#include <sdpPhyPage.h>
#include <sdpSglRIDList.h>

#include <sdpsfDef.h>
#include <sdpsfSH.h>
#include <sdpReq.h>
#include <sdpsfAllocPage.h>
#include <sdpsfFindPage.h>

/***********************************************************************
 * Description : aRecordSize가 삽일될 Free Page를 찾는다.
 *
 * 1. SegHdr에 SLatch를 잡고 Free PID List를 찾는다.
 * 2. 만약 Free PID List에 없다면 새로운 페이지를 할당한다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aSegHandle   - [IN] Segment Handle
 * aTableInfo   - [IN] Insert, update, delete시 Transaction에 추가되는
 *                     TableInfo정보로서 InsertPID Hint를 참조하기 위해
 *                     사용
 * aPageType    - [IN] Page Type
 * aRecordSize  - [IN] Record Size
 * aNeedKeySlot - [IN] 페이지 free공간 계산시 KeySlot을 포함해서 계산해야
 *                     되면 ID_TRUE, else ID_FALSE
 *
 * aPagePtr     - [OUT] Free공간을 가진 Page에 대한 Pointer가 설정된다.
 *                      return 시 해당 페이지에 XLatch가 걸려있다.
 *
 ***********************************************************************/
IDE_RC sdpsfFindPage::findFreePage( idvSQL           *aStatistics,
                                    sdrMtx           *aMtx,
                                    scSpaceID         aSpaceID,
                                    sdpSegHandle     *aSegHandle,
                                    void             *aTableInfo,
                                    sdpPageType       aPageType,
                                    UInt              aRecordSize,
                                    idBool            aNeedKeySlot,
                                    UChar           **aPagePtr,
                                    UChar            *aCTSlotIdx )
{
    UChar         * sPagePtr;
    sdpsfSegHdr   * sSegHdr;
    scPageID        sSegPID;
    scPageID        sHintDataPID;
    SInt            sState = 0;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSpaceID     != 0 );
    IDE_ASSERT( aSegHandle   != NULL );
    IDE_ASSERT( aPageType    < SDP_PAGE_TYPE_MAX );
    IDE_ASSERT( aRecordSize  < SD_PAGE_SIZE );
    IDE_ASSERT( aRecordSize  > 0 );
    IDE_ASSERT( aPagePtr     != NULL );

    sPagePtr  = NULL;
    sSegPID   = aSegHandle->mSegPID;

    IDE_TEST( checkHintPID4Insert( aStatistics,
                                   aMtx,
                                   aSpaceID,
                                   aTableInfo,
                                   aRecordSize,
                                   aNeedKeySlot,
                                   &sPagePtr,
                                   aCTSlotIdx )
              != IDE_SUCCESS );

    if( sPagePtr == NULL )
    {
        /* SegHdr에 XLatch를 건다. */
        IDE_TEST( sdpsfSH::fixAndGetSegHdr4Update( aStatistics,
                                                   aSpaceID,
                                                   sSegPID,
                                                   &sSegHdr )
                  != IDE_SUCCESS );
        sState = 1;

        /* Free PID List에서 Free Page를 찾는다. */
        IDE_TEST( sdpsfFreePIDList::walkAndUnlink( aStatistics,
                                                   aMtx,
                                                   aSpaceID,
                                                   sSegHdr,
                                                   aRecordSize,
                                                   aNeedKeySlot,
                                                   &sPagePtr,
                                                   aCTSlotIdx )
                  != IDE_SUCCESS );

        if( sPagePtr == NULL )
        {
            /* Free PID List에 없다면 Pvt, UFmt, Ext List에서 페이지를
             * 할당받는다. */
            IDE_TEST( sdpsfAllocPage::allocNewPage( aStatistics,
                                                    aMtx,
                                                    aSpaceID,
                                                    aSegHandle,
                                                    sSegHdr,
                                                    aPageType,
                                                    &sPagePtr )
                      != IDE_SUCCESS );

            IDE_ASSERT( sPagePtr != NULL );
        }

        sState = 0;
        IDE_TEST( sdpsfSH::releaseSegHdr( aStatistics,
                                          sSegHdr )
                  != IDE_SUCCESS );
    }

    sHintDataPID = sdpPhyPage::getPageID( sPagePtr );

    smLayerCallback::setHintDataPIDofTableInfo( aTableInfo,
                                                sHintDataPID );

    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdpsfSH::releaseSegHdr( aStatistics,
                                            sSegHdr )
                    == IDE_SUCCESS );
    }

    smLayerCallback::setHintDataPIDofTableInfo( aTableInfo,
                                                SD_NULL_PID );

    *aPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aTableInfo에서 가리키는 Hint Page에 빈 공간이 있는지 Check
 *               한다.
 *
 * aStatistics  - [IN] 통계 정보
 * aMtx         - [IN] Mini Transaction Pointer
 * aSpaceID     - [IN] TableSpace ID
 * aTableInfo   - [IN] TableInfo
 * aRecordSize  - [IN] Record Size
 * aNeedKeySlot - [IN] Insert시 KeySlot이 필요한가?
 *
 * aPagePtr     - [OUT] Hint PID가 가리키는 페이지에 Insert가 가능하면 Hint
 *                      PID가 가리키는 Page Pointer가 리턴되고, 아니면 Null
 *                      이 Return
 ***********************************************************************/
IDE_RC sdpsfFindPage::checkHintPID4Insert( idvSQL           *aStatistics,
                                           sdrMtx           *aMtx,
                                           scSpaceID         aSpaceID,
                                           void             *aTableInfo,
                                           UInt              aRecordSize,
                                           idBool            aNeedKeySlot,
                                           UChar           **aPagePtr,
                                           UChar            *aCTSlotIdx )
{
    scPageID         sHintDataPID;
    UChar           *sPagePtr      = NULL;;
    SInt             sState        = 0;
    idBool           sCanAllocSlot = ID_TRUE;
    idBool           sRemFrmList   = ID_FALSE;
    sdrMtxStartInfo  sStartInfo;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSpaceID     != 0 );
    IDE_ASSERT( aRecordSize  < SD_PAGE_SIZE );
    IDE_ASSERT( aRecordSize  > 0 );
    IDE_ASSERT( aPagePtr     != NULL );

    smLayerCallback::getHintDataPIDofTableInfo( aTableInfo,
                                                &sHintDataPID );

    if( sHintDataPID != SD_NULL_PID )
    {
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sHintDataPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* aMtx */
                                              &sPagePtr,
                                              NULL /*aTrySuccess*/,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );
        sState = 1;

        sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

        IDE_TEST( checkSizeAndAllocCTS( aStatistics,
                                        &sStartInfo,
                                        sPagePtr,
                                        aRecordSize,
                                        aNeedKeySlot,
                                        &sCanAllocSlot,
                                        &sRemFrmList,
                                        aCTSlotIdx )
                  != IDE_SUCCESS );

        if( sCanAllocSlot == ID_TRUE )
        {
            IDE_TEST( sdrMiniTrans::pushPage( aMtx,
                                              sPagePtr,
                                              SDB_X_LATCH )
                      != IDE_SUCCESS );
        }
        else
        {
            sState = 0;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sPagePtr )
                      != IDE_SUCCESS );

            sPagePtr = NULL;
        }
    }

    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                               sPagePtr )
                    == IDE_SUCCESS );
    }

    *aPagePtr = NULL;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 페이지의 가용공간을 확인하고 Row을 저장할 수 있는지 검사
 *
 * aPagePtr      - [IN] Page Pointer
 * aRowSize      - [IN] 저장할 Row 크기
 * aCanAllocSlot - [OUT] 가용공간 할당 여부
 * aCanAllocSlot - [OUT] List로부터 제거여부
 *
 ***********************************************************************/
IDE_RC sdpsfFindPage::checkSizeAndAllocCTS(
                              idvSQL          * aStatistics,
                              sdrMtxStartInfo * aStartInfo,
                              UChar           * aPagePtr,
                              UInt              aRowSize,
                              idBool            aNeedKeySlot,
                              idBool          * aCanAllocSlot,
                              idBool          * aRemFrmList,
                              UChar           * aCTSlotIdx )
{
    UChar            sCTSlotIdx;
    idBool           sCanAllocSlot;
    idBool           sRemFrmList;
    sdpPhyPageHdr  * sPageHdr;
    idBool           sAfterSelfAging;
    sdpSelfAgingFlag sCheckFlag = SDP_SA_FLAG_NOTHING_TO_AGING;

    IDE_ASSERT( aStartInfo    != NULL );
    IDE_ASSERT( aPagePtr      != NULL );
    IDE_ASSERT( aRowSize      > 0 );
    IDE_ASSERT( aCanAllocSlot != NULL );
    IDE_ASSERT( aRemFrmList   != NULL );
    IDE_ASSERT( aCTSlotIdx    != NULL );

    sCanAllocSlot    = ID_FALSE;
    sRemFrmList      = ID_TRUE;
    sCTSlotIdx       = SDP_CTS_IDX_NULL;
    sPageHdr         = (sdpPhyPageHdr*)aPagePtr;
    sAfterSelfAging  = ID_FALSE;

    IDE_TEST_CONT( sdpPhyPage::getState( sPageHdr )
                                != SDPSF_PAGE_USED_INSERTABLE,
                    remove_page_from_list );

    while ( 1 )
    {
        IDE_TEST( smLayerCallback::allocCTSAndSetDirty( aStatistics,
                                                        NULL,        /* aFixMtx */
                                                        aStartInfo,  /* for Logging */
                                                        sPageHdr,    /* in-out param */
                                                        &sCTSlotIdx )
                  != IDE_SUCCESS );

        sCanAllocSlot = sdpPhyPage::canAllocSlot( sPageHdr,
                                                  aRowSize,
                                                  aNeedKeySlot,
                                                  SDP_1BYTE_ALIGN_SIZE );

        if ( ( smLayerCallback::getCountOfCTS( sPageHdr ) > 0 ) &&
             ( sCTSlotIdx == SDP_CTS_IDX_NULL ) )
        {
            sCanAllocSlot = ID_FALSE;
        }

        if ( sCanAllocSlot == ID_TRUE )
        {
            sRemFrmList = ID_FALSE;
            break;
        }
        else
        {
            sCTSlotIdx = SDP_CTS_IDX_NULL;
        }

        if ( sAfterSelfAging == ID_TRUE )
        {
            if ( sCheckFlag != SDP_SA_FLAG_NOTHING_TO_AGING )
            {
                sRemFrmList = ID_FALSE;
            }
            break;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( smLayerCallback::checkAndRunSelfAging( aStatistics,
                                                         aStartInfo,
                                                         sPageHdr,
                                                         &sCheckFlag )
                  != IDE_SUCCESS );

        sAfterSelfAging = ID_TRUE;
    }

    IDE_EXCEPTION_CONT( remove_page_from_list );

    *aCanAllocSlot = sCanAllocSlot;
    *aRemFrmList   = sRemFrmList;
    *aCTSlotIdx    = sCTSlotIdx;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page에 사용량에 따라서 페이지의 상태를 UPDATE_ONLY나
 *               INSERTABLE로 변경한다.
 *
 * aStatistics   - [IN] 통계 정보
 * aMtx          - [IN] Mini Transaction Pointer
 * aSpaceID      - [IN] TableSpace ID
 * aSegHandle    - [IN] Segment Handle
 * aDataPagePtr  - [IN] insert, update, delete가 발생한 Data Page
 *
 ***********************************************************************/
IDE_RC sdpsfFindPage::updatePageState( idvSQL           * aStatistics,
                                       sdrMtx           * aMtx,
                                       scSpaceID          aSpaceID,
                                       sdpSegHandle     * aSegHandle,
                                       UChar            * aDataPagePtr )
{
    sdpsfPageState    sPageState;
    SInt              sState   = 0;
    sdpsfSegHdr      *sSegHdr  = NULL;
    sdpPhyPageHdr    *sPageHdr = sdpPhyPage::getHdr( aDataPagePtr );

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSpaceID     != 0 );
    IDE_ASSERT( aSegHandle   != NULL );
    IDE_ASSERT( aDataPagePtr != NULL );

    if( sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA )
    {
        sPageState = (sdpsfPageState)sdpPhyPage::getState( sPageHdr  );

        /* PCTFREE와 PCTUSED를 조사해야 한다. */
        switch( sPageState )
        {
            case SDPSF_PAGE_USED_INSERTABLE:
                /* Insertable이다가 insertRow나 updateRow에 의해서
                 * 페이지의 상태가 Update Only로 바꿜수 있기때문에
                 * Check한다. */
                if( isPageUpdateOnly( sPageHdr,
                                      aSegHandle->mSegAttr.mPctFree )
                    == ID_TRUE )
                {
                    /* Page상태를 Update Only로 바꾼다. */
                    IDE_TEST( sdpPhyPage::setState(
                                    sPageHdr,
                                    (UShort)SDPSF_PAGE_USED_UPDATE_ONLY,
                                    aMtx ) != IDE_SUCCESS );
                }
                break;

            case SDPSF_PAGE_USED_UPDATE_ONLY:
                /* Update Only였다가 FreeRow에 의해서 상태가 변경될 수 있기 때문에 이것을
                 * Check한다. */
                if( isPageInsertable( sPageHdr,
                                      aSegHandle->mSegAttr.mPctUsed )
                    == ID_TRUE )
                {
                    IDE_TEST( sdpPhyPage::setState(
                                    sPageHdr,
                                    (UShort)SDPSF_PAGE_USED_INSERTABLE,
                                    aMtx ) != IDE_SUCCESS );

                    if( sPageHdr->mLinkState == SDP_PAGE_LIST_UNLINK )
                    {
                        /* 만약 Free PID List에 등록이 되어 있지 않다면 Add시킨다. */
                        IDE_TEST( sdpsfAllocPage::addPageToFreeList( aStatistics,
                                                                     aMtx,
                                                                     aSpaceID,
                                                                     aSegHandle->mSegPID,
                                                                     aDataPagePtr )
                                  != IDE_SUCCESS );
                    }
                }
                break;

                /* 이상태로 들어올수 없다. */
            case SDPSF_PAGE_FREE:
            default:
                IDE_ASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdpsfSH::releaseSegHdr( aStatistics,
                                            sSegHdr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}
