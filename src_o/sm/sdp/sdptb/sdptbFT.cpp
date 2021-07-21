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
 * $Id: sdptbFT.cpp 27228 2008-07-23 17:36:52Z newdaily $
 *
 * TBS관려 Dump Table 함수들이 모여있다.
 **********************************************************************/
#include <sdp.h>
#include <sdptb.h>
#include <sctTableSpaceMgr.h>
#include <smErrorCode.h>
#include <sdpst.h>
#include <sdpReq.h>


/******************************************************************************
 * Description : Free Ext List를 보여주는 D$DISK_TBS_FREEEXTLIST의 Record를
 *               Build한다.
 *
 *  aHeader   - [IN]
 *  aDumpObj  - [IN]
 *  aMemory   - [IN]
 ******************************************************************************/
IDE_RC sdptbFT::buildRecord4FreeExtOfTBS(
    void                * aHeader,
    void                * aDumpObj,
    iduFixedTableMemory * aMemory )
{
    sdptbSpaceCache   * sSpaceCache;
    sddTableSpaceNode * sTBSNode;
    UInt                i;
    UInt                sLstGGID;
    UInt                sFileID;
    scSpaceID           sSpaceID;
    UInt                sIdx;

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    /* TBS가 존재하는지 검사하고 Dump중에 Drop되지 않도록 Lock을 잡는다. */
    /* BUG-28678  [SM] qmsDumpObjList::mObjInfo에 설정될 메모리 주소는 
     * 반드시 공간을 할당해서 설정해야합니다. 
     * 
     * aDumpObj는 Pointer로 데이터가 오기 때문에 값을 가져와야 합니다. */
    sSpaceID = *( (scSpaceID*)aDumpObj );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sSpaceID,
                                                        (void**)&sTBSNode)
              != IDE_SUCCESS );

    sSpaceCache = sddDiskMgr::getSpaceCache( sTBSNode );

    IDE_ASSERT( sTBSNode != NULL );
    IDE_ASSERT( sSpaceCache != NULL );

    sLstGGID = sSpaceCache->mMaxGGID;

    for( i = 0, sFileID = 0; i <= sLstGGID; i++ )
    {
        sdptbBit::findBitFromHint( (void *)sSpaceCache->mFreenessOfGGs,
                                   sLstGGID + 1,   //검색대상비트수
                                   sFileID,
                                   &sIdx );

        if( sIdx == SDPTB_BIT_NOT_FOUND )
        {
            break;
        }

        /* 각 File의 GG에 대해서 FreeExt 정보를 생성하도록 한다. */
        sFileID = sIdx;

        IDE_TEST( buildRecord4FreeExtOfGG( aHeader,
                                           aMemory,
                                           sSpaceCache,
                                           sFileID )
                  != IDE_SUCCESS );

        /*
         * BUG-28059 [SD] drop 된 파일이 있는경우 D$disk_tbs_free_extlist 
         *           조회시 죽는 경우가 있음. 
         */
        if( sIdx == sLstGGID )
        {
            break;
        }

        sFileID++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/******************************************************************************
 * Description : TBS에서 D$DISK_TBS_FREE_EXTLIST의 레코드중 aFID에 해당하는 파
 *               일의 Record를 Build한다.
 *
 *  aHeader   - [IN]
 *  aMemory   - [IN]
 *  aCache    - [IN] TableSpace Cache
 *  sdFileID  - [IN] Dump하려는 File의 ID
 ******************************************************************************/
IDE_RC sdptbFT::buildRecord4FreeExtOfGG( void                * aHeader,
                                         iduFixedTableMemory * aMemory,
                                         sdptbSpaceCache     * aCache,
                                         sdFileID              aFID )
{
    UInt               i;
    UInt               sSpaceID;
    scPageID           sGGPID;
    sdptbGGHdr       * sGGHdrPtr;
    UChar            * sPagePtr;
    sdptbLGFreeInfo  * sLFFreeInfo;
    UInt               sFreeLGID;
    scPageID           sLGHdrPID;
    UInt               sTotLGCnt;
    UInt               sAllocIdx;
    UInt               sDeAllocIdx;
    UInt               sIdx;
    UInt               sState = 0;

    IDE_ASSERT( aCache != NULL );

    sSpaceID = aCache->mCommon.mSpaceID;
    sGGPID   = SDPTB_GET_GGHDR_PID_BY_FID( aFID );

    IDE_TEST( sdbBufferMgr::getPageByPID( NULL, /* Statistics */
                                          sSpaceID,
                                          sGGPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* Mini Trans */
                                          (UChar**)&sPagePtr,
                                          NULL, /*TrySuccess*/
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );
    sState = 1;

    sGGHdrPtr   = sdptbGroup::getGGHdr( sPagePtr );
    sAllocIdx   = sdptbGroup::getAllocLGIdx( sGGHdrPtr );
    sDeAllocIdx = sdptbGroup::getDeallocLGIdx( sGGHdrPtr );
    sLFFreeInfo = sGGHdrPtr->mLGFreeness + sAllocIdx;
    sTotLGCnt   = sGGHdrPtr->mLGCnt;

    for( i = 0, sFreeLGID = 0; i < sTotLGCnt; i++ )
    {
        sdptbBit::findBitFromHint( &sLFFreeInfo->mBits,
                                   sTotLGCnt, //검색대상비트수
                                   sFreeLGID,
                                   &sIdx );
        if( sIdx == SDPTB_BIT_NOT_FOUND )
        {
            break;
        }

        /* 각 File의 GG에 대해서 FreeExt 정보를 생성하도록 한다. */
        sFreeLGID = sIdx;

        sLGHdrPID = SDPTB_LG_HDR_PID_FROM_LGID( aFID,
                                                sFreeLGID,
                                                sDeAllocIdx,
                                                sGGHdrPtr->mPagesPerExt );

        IDE_TEST( buildRecord4FreeExtOfLG( aHeader,
                                           aMemory,
                                           aCache,
                                           sGGHdrPtr,
                                           sLGHdrPID )
                  != IDE_SUCCESS );

        sLGHdrPID = SDPTB_LG_HDR_PID_FROM_LGID( aFID,
                                                sFreeLGID,
                                                sAllocIdx,
                                                sGGHdrPtr->mPagesPerExt );

        IDE_TEST( buildRecord4FreeExtOfLG( aHeader,
                                           aMemory,
                                           aCache,
                                           sGGHdrPtr,
                                           sLGHdrPID )
                  != IDE_SUCCESS );

        sFreeLGID++;
    }

    sState  = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL, /* Statistics */
                                         sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* Statistics */
                                               sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : TBS에서 D$DISK_TBS_FREEEXTLIST의 레코드중 aGGPtr의 aLGHdrPID
 *               가 가리키는 LG의 Record들을 Build한다.
 *
 *  aHeader   - [IN]
 *  aMemory   - [IN]
 *  aCache    - [IN] TableSpace Cache
 *  aGGPtr    - [IN] GGHeader Pointer
 *  aLGHdrPID - [IN] LGHeader PID
 ******************************************************************************/
IDE_RC sdptbFT::buildRecord4FreeExtOfLG( void                * aHeader,
                                         iduFixedTableMemory * aMemory,
                                         sdptbSpaceCache     * aSpaceCache,
                                         sdptbGGHdr          * aGGPtr,
                                         scPageID              aLGHdrPID )
{
    UInt            i;
    sdptbLGHdr    * sLGHdrPtr;
    UInt            sBitIdx;
    UChar         * sPagePtr;
    UInt            sFreeExtIdx;
    sdpDumpTBSInfo  sDumpFreeExtInfo;
    ULong           sBitmap[ SD_PAGE_SIZE / ID_SIZEOF( ULong ) ];
    UInt            sState = 0;

    IDE_TEST(sdbBufferMgr::getPageByPID( NULL, /* Statistics */
                                         aSpaceCache->mCommon.mSpaceID,
                                         aLGHdrPID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         NULL, /* Mini Mtx */
                                         (UChar**)&sPagePtr,
                                         NULL, /*TrySuccess*/
                                         NULL /*IsCorruptPage*/ )
             != IDE_SUCCESS );
    sState = 1;

    sLGHdrPtr = sdptbGroup::getLGHdr( sPagePtr);

    idlOS::memcpy( sBitmap,
                   sLGHdrPtr->mBitmap,
                   sdptbGroup::getLenBitmapOfLG() );

    for( i = 0, sFreeExtIdx = 0; i < sLGHdrPtr->mFree; i++ )
    {
        //LG에서 현재 사용중인 mValidBits갯수 만큼을 검색해야한다.
        sdptbBit::findZeroBitFromHint( sBitmap,
                                       sLGHdrPtr->mValidBits,
                                       sFreeExtIdx,
                                       &sBitIdx );

        sdptbBit::setBit( sBitmap, sBitIdx );

        sFreeExtIdx = sBitIdx;

        //해당 LG에 free가 있는것을 보고서 들어왔으므로 이게 거짓이된다
        //면 심각한 에러상황이다.
        IDE_ASSERT( sBitIdx <  sLGHdrPtr->mValidBits );

        sDumpFreeExtInfo.mExtRID       = SD_MAKE_RID( aLGHdrPID, sFreeExtIdx );
        sDumpFreeExtInfo.mPID          = aLGHdrPID;
        sDumpFreeExtInfo.mOffset       = sFreeExtIdx;
        sDumpFreeExtInfo.mFstPID       = sLGHdrPtr->mStartPID + aSpaceCache->mCommon.mPagesPerExt*sBitIdx;
        sDumpFreeExtInfo.mPageCntInExt = aGGPtr->mPagesPerExt;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sDumpFreeExtInfo )
                  != IDE_SUCCESS );

        sFreeExtIdx++;
    }

    sState  = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL, /* Statistics */
                                         sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* Statistics */
                                               sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

