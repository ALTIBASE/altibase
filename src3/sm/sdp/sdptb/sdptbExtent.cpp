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
 * $Id: sdptbExtent.cpp 27228 2008-07-23 17:36:52Z newdaily $
 *
 * TBS에서 extent를 할당하고 해제하는 루틴에 관련된 함수들이다.
 **********************************************************************/
#include <sdp.h>
#include <sdptb.h>
#include <sctTableSpaceMgr.h>
#include <smErrorCode.h>
#include <sdpst.h>
#include <sdpReq.h>

/***********************************************************************
 * Description:
 *   [INTERFACE] tablespace로부터 extent를 할당받는다.
 *
 *
 * aStatistics - [IN] 통계정보
 * aStartInfo  - [IN] Mini Transaction Start Info
 * aSpaceID    - [IN] TableSpace ID
 * aOrgNrExts  - [IN] 할당을 요청할 Extent 갯수
 *
 * aExtSlot    - [OUT] 할당된 Extent Desc Array Ptr
 ***********************************************************************/
IDE_RC sdptbExtent::allocExts( idvSQL          * aStatistics,
                               sdrMtxStartInfo * aStartInfo,
                               scSpaceID         aSpaceID,
                               UInt              aOrgNrExts,
                               sdpExtDesc      * aExtSlot )
{
    UInt                sNrExts; //할당해야하는 ext의 갯수를 관리한다.
    sdptbSpaceCache   * sSpaceCache;
    sdFileID            sFID = SD_MAKE_FID(SD_NULL_PID);
    UInt                sNrDone;
    sddTableSpaceNode * sTBSNode;
    UInt                sNeededPageCnt; //파일확장시 요청할 페이지 갯수
    UInt                i;
    scPageID            sExtFstPID[4]; // 최대 4개까지 한번에 할당받을수 있다.
    scPageID          * sCurExtFstPIDPtr;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aOrgNrExts == 1 ); // segment에서 호출될때 1만 넘겨준다.

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );

    IDE_ERROR_MSG( sTBSNode != NULL,
                   "Tablespace node not found (ID : %"ID_UINT32_FMT")",
                   aSpaceID );

    IDE_DASSERT_MSG( sctTableSpaceMgr::isDiskTableSpace( sTBSNode ) == ID_TRUE,
                     "Fatal error during alloc extent (Tablespace ID : %"ID_UINT32_FMT") ",
                      aSpaceID );
                    
    sSpaceCache = sddDiskMgr::getSpaceCache( sTBSNode );
    IDE_ERROR_MSG( sSpaceCache != NULL,
                   "Tablespace cache not found (ID : %"ID_UINT32_FMT")",
                    aSpaceID );

    sNrExts          = aOrgNrExts;
    sCurExtFstPIDPtr = sExtFstPID;

    while( sNrExts > 0 )
    {
        IDE_TEST( getAvailFID( sSpaceCache, &sFID ) != IDE_SUCCESS );

        if( sFID != SDPTB_NOT_FOUND )
        {
            IDE_TEST( tryAllocExtsInGG( aStatistics,
                                        aStartInfo,
                                        sSpaceCache,
                                        sFID,
                                        sNrExts,
                                        sCurExtFstPIDPtr,
                                        &sNrDone)
                       != IDE_SUCCESS );

            sCurExtFstPIDPtr += sNrDone;
            sNrExts          -= sNrDone;
        }
        else
        {
            sNeededPageCnt = sNrExts * sSpaceCache->mCommon.mPagesPerExt;

            IDE_TEST( autoExtDatafileOnDemand( aStatistics,
                                               aSpaceID,
                                               sSpaceCache,
                                               aStartInfo->mTrans,
                                               sNeededPageCnt )
                       != IDE_SUCCESS  );

            //파일확장에 성공했다면,  다시 루프를 돌면서 할당을 한다.

            /* [참고]
             * 여기까지 왔다는것은 파일확장에 성공했다는 뜻이다.
             * 만약 확장가능한 파일이 없는 이유등으로 확장에 실패했다면
             * autoExtDatafileonDemand안에서 모두 처리된다.
             */

            continue;
        }

        // 주의!
        // cache의 정보를 보고 파일에 들어가는것이므로 sNrDone이 1일수도 있다.
        // cache는 dirty read 하므로.....
        // IDE_ASSERT( sNrDone == 1 );

    }//while

    //에러가 발생하지 않았다면  sNrExts가 0이 될것이다.(0이 정상상황)
    IDE_ERROR_MSG( sNrExts == 0,
                   "Error occurred while new extents alloc"
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "ExtCnt : %"ID_UINT32_FMT")",
                   aSpaceID,
                   sNrExts );

    //할당받은 extent의 첫번째 PID를 인자로받은 aExtSlot에 저장한다.
    for( i=0 ; i < aOrgNrExts ; i++ )
    {
        aExtSlot[i].mExtFstPID = sExtFstPID[i];
        aExtSlot[i].mLength    = sSpaceCache->mCommon.mPagesPerExt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description:
 *   GG 안에서 extent를 할당받는다.
 *
 * aStatistics - [IN] 통계정보
 * aStartInfo  - [IN] Mini Transaction Start Info
 * aNrExts     - [IN] 할당을 요청할 Extent 갯수
 * aExtFstPID  - [OUT] 할당한 extent의 첫번째 pid array
 * aNrDone     - [OUT] 할당된 Extent 수
 ***********************************************************************/
IDE_RC sdptbExtent::tryAllocExtsInGG( idvSQL             * aStatistics,
                                      sdrMtxStartInfo    * aStartInfo,
                                      sdptbSpaceCache    * aCache,
                                      sdFileID             aFID,
                                      UInt                 aNrExts,
                                      scPageID           * aExtFstPID,
                                      UInt               * aNrDone )
{
    UInt        i;
    UInt        sSpaceID;
    scPageID    sGGPID;
    scPageID    sLGHdrPID;
    idBool      sDummy;
    UInt        sLGID;  //할당을 고려할  LGID
    sdptbGGHdr* sGGHdrPtr;
    UInt        sAllocLGIdx;  //사용중인 LG index
    UChar     * sPagePtr;
    idBool      sSwitching;//임시변수
    UInt        sFreeInLG;
    sdrMtx      sMtx;
    smLSN       sOpNTA;
    ULong       sData[5]; //extent갯수 ,4개의 PID까지 저장할 공간을 만든다.
    UInt        sBitIdx;
    UInt        sState      = 0;
    UInt        sNrDoneInLG = 0;

    IDE_ASSERT( aStartInfo  != NULL );
    IDE_ASSERT( aCache      != NULL );
    IDE_ASSERT( aExtFstPID  != NULL );
    IDE_ASSERT( aNrDone     != NULL );
    IDE_ASSERT( aNrExts   == 1 ); // segment에서 호출될때 1만 넘겨준다.

    sSpaceID = aCache->mCommon.mSpaceID;
    sGGPID   = SDPTB_GET_GGHDR_PID_BY_FID( aFID );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    if( sdrMiniTrans::getTrans(&sMtx) != NULL )
    {
       sOpNTA = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );
    }
    else
    {
        /* Temporary Table 생성시에는 트랜잭션이 NULL이 내려올 수 있다. */
    }

    IDE_TEST(sdbBufferMgr::getPageByPID( aStatistics,
                                         sSpaceID,
                                         sGGPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (void*)&sMtx,
                                         (UChar**)&sPagePtr,
                                         &sDummy,
                                         NULL /*IsCorruptPage*/ )
             != IDE_SUCCESS);

    sGGHdrPtr   = sdptbGroup::getGGHdr( sPagePtr );
    sAllocLGIdx = sdptbGroup::getAllocLGIdx(sGGHdrPtr);
    /*
     * 할당을 시도할  LG를 찾는다
     *
     * 현재 LG갯수만큼을 대상으로 검색해야한다.
     */
    sdptbBit::findBit( sGGHdrPtr->mLGFreeness[sAllocLGIdx].mBits,
                       sGGHdrPtr->mLGCnt,
                       &sLGID );
    //아래 두가지 조건중 하나가  아니라면 에러임.
    IDE_ERROR_MSG( (sLGID < sGGHdrPtr->mLGCnt) ||
                   (sLGID ==  SDPTB_BIT_NOT_FOUND),
                   "Error occurred while new extents find" 
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "Page ID : %"ID_UINT32_FMT", "
                   "Local Group ID : %"ID_UINT32_FMT", "
                   "Local Group Cnt : %"ID_UINT32_FMT")",
                   sSpaceID,
                   sGGPID,
                   sLGID,
                   sGGHdrPtr->mLGCnt );

    //일단 해당 LG에서 하나의 ext를 할당한다.
    if( sLGID < sGGHdrPtr->mLGCnt ) //free가 있는 LG를 찾았다.
    {
        sLGHdrPID = SDPTB_LG_HDR_PID_FROM_LGID( aFID,
                                                sLGID,
                                                sAllocLGIdx,
                                                sGGHdrPtr->mPagesPerExt );

        IDE_TEST( allocExtsInLG( aStatistics,
                                 &sMtx,
                                 aCache,
                                 sGGHdrPtr,
                                 sLGHdrPID,
                                 aNrExts, //할당요청갯수
                                 aExtFstPID,   //할당한 extent의 첫번째 pid array
                                 &sNrDoneInLG, //실제 할당한 갯수
                                 &sFreeInLG )  //free extent 수
          != IDE_SUCCESS );

       /*
        * LG에 free가 있다는 정보를 mLGFreeness에서 읽고 들어왔으므로
        * 하나라도 할당해야만한다. 아니라면 치명적버그
        */
        IDE_ERROR_MSG( sNrDoneInLG != 0,
                       "Error occurred while new extents find"
                       "(Tablespace ID : %"ID_UINT32_FMT", "
                       "Page ID : %"ID_UINT32_FMT", "
                       "ExtCnt : %"ID_UINT32_FMT")",
                       sSpaceID,
                       sGGPID,
                       sNrDoneInLG );


       if( sFreeInLG == 0 )
       {
           /*
            * extent를 지금 할당한다음 요청한 LG에 free가 없다면
            *  LGFreeness비트를 꺼야만 한다.!
            */

           sBitIdx = SDPTB_GET_LGID_BY_PID( sLGHdrPID,
                                            sGGHdrPtr->mPagesPerExt);

           IDE_TEST( sdptbGroup::logAndSetLGFNBitsOfGG( &sMtx,
                                                        sGGHdrPtr,
                                                        sBitIdx,
                                                        0 ) //비트를0으로 세트
                     != IDE_SUCCESS );
       }
    }
    else
    {
        /*  !( sLGID <  sGGHdrPtr->mLGCnt)인경우는
         *  sLGID가 SDPTB_BIT_NOT_FOUND 인 경우밖에 없어야한다.
         *  아니면 치명적 에러.
         */
        IDE_ERROR_MSG( sLGID == SDPTB_BIT_NOT_FOUND,
                       "Error occurred while new extents find"
                       "(Tablespace ID : %"ID_UINT32_FMT", "
                       "Page ID : %"ID_UINT32_FMT", "
                       "Local Group ID : %"ID_UINT32_FMT")",
                       sSpaceID,
                       sGGPID,
                       sLGID );

        /*
         * space cache는 락을 잡지 않으므로 space cache 정보를 보고
         * GG에 들어왔을때 다른 Tx가 나머지 exts를 할당한 후일경우
         * 이곳으로 들어올수가 있다.
         */


        //이 GG에 free가 있는 LG가 없어서 여기들어왔으므로 NTA필요없다.
        IDE_CONT( return_anyway );
    }

    //4개이하여야함(sdpst와 관련되어있다)
    //setNTA하기전에 체크.
    IDE_ERROR_MSG( (0 < sNrDoneInLG) && (sNrDoneInLG<= 4), 
                   "Error occurred while new extents find"
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "Page ID : %"ID_UINT32_FMT", "
                   "ExtCnt : %"ID_UINT32_FMT")",
                   sSpaceID,
                   sGGPID,
                   sNrDoneInLG );

    /*
     * sData[0] ~  sData[3] : 할당한 extent의 첫번째 pid값들
     */
    for(i=0; i < sNrDoneInLG ; i++)
    {
        sData[i] = (ULong)aExtFstPID[i];
    }

    // Undo 될수 있도록 OP NTA 처리한다.
    // TBS에서 extent를 해제하는데 필요한 정보를 전달한다.
    sdrMiniTrans::setNTA( &sMtx,
                          sSpaceID,
                          SDR_OP_SDPTB_ALLOCATE_AN_EXTENT_FROM_TBS,
                          &sOpNTA,
                          sData,
                          sNrDoneInLG /* Data Count */ );

    IDE_EXCEPTION_CONT( return_anyway );

    //switching을 시도해본다.
    IDE_TEST( trySwitch( &sMtx,
                         sGGHdrPtr,
                         &sSwitching,
                         aCache ) != IDE_SUCCESS );

    sState=0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    *aNrDone = sNrDoneInLG;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState==1)
    {
       IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) ==  IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *   LG 안에서 extent를 할당받는다.
 *
 * aStatistics - [IN] 통계정보
 * aStartInfo  - [IN] Mini Transaction Start Info
 * aOrgNrExts  - [IN] 할당을 요청할 Extent 갯수
 * aExtFstPID  - [OUT] 할당한 extent의 첫번째 pid array
 * aNrDone     - [OUT] 할당된 Extent 수
 * aFreeInLG   - [OUT] free Extent 수
 ***********************************************************************/
IDE_RC sdptbExtent::allocExtsInLG( idvSQL                  * aStatistics,
                                   sdrMtx                  * aMtx,
                                   sdptbSpaceCache         * aSpaceCache,
                                   sdptbGGHdr              * aGGPtr,
                                   scPageID                  aLGHdrPID,
                                   UInt                      aOrgNrExts,
                                   scPageID                * aExtFstPID,
                                   UInt                    * aNrDone,
                                   UInt                    * aFreeInLG )
{
    idBool       sDummy;
    sdptbLGHdr * sLGHdrPtr;
    UInt         sBitIdx;
    UInt         sNrExts = aOrgNrExts; //요청한갯수
    UChar      * sPagePtr;
    scPageID     sLastPID;

    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aSpaceCache != NULL );
    IDE_ASSERT( aGGPtr      != NULL );
    IDE_ASSERT( aExtFstPID  != NULL );
    IDE_ASSERT( aNrDone     != NULL );
    IDE_ASSERT( aFreeInLG   != NULL );
    IDE_ASSERT( aOrgNrExts  == 1 ); // segment에서 호출될때 1만 넘겨준다.

    IDE_TEST(sdbBufferMgr::getPageByPID( aStatistics,
                                         aSpaceCache->mCommon.mSpaceID,
                                         aLGHdrPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (void*)aMtx,
                                         (UChar**)&sPagePtr,
                                         &sDummy,
                                         NULL /*IsCorruptPage*/ )
             != IDE_SUCCESS );

    sLGHdrPtr = sdptbGroup::getLGHdr( sPagePtr );

    // 심각한 에러상황!
    // 이미 GG에서 free인 LG를 찾아서 들어왔으므로.. 정상적인상황에서는
    // mFree가 0일수가 없다.
    IDE_ERROR_MSG( sLGHdrPtr->mFree > 0,
                   "Error occurred while new extents alloc"
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "Page ID : %"ID_UINT32_FMT", "
                   "ExtCnt : %"ID_UINT32_FMT")",
                   aSpaceCache->mCommon.mSpaceID,
                   aLGHdrPID,
                   sLGHdrPtr->mFree );

    IDE_ERROR_MSG( sLGHdrPtr->mValidBits > sLGHdrPtr->mHint,
                   "Error occurred while new extents alloc"
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "Page ID : %"ID_UINT32_FMT", "
                   "Hint Cnt : %"ID_UINT32_FMT", "
                   "ValidBits : %"ID_UINT32_FMT")",
                   aSpaceCache->mCommon.mSpaceID,
                   aLGHdrPID,
                   sLGHdrPtr->mHint,
                   sLGHdrPtr->mValidBits );

    while( ( sNrExts > 0 ) && ( sLGHdrPtr->mFree > 0 ) )
    {
        //LG에서 현재 사용중인 mValidBits갯수 만큼을 검색해야한다.
        sdptbBit::findZeroBitFromHint( sLGHdrPtr->mBitmap,
                                       sLGHdrPtr->mValidBits,
                                       sLGHdrPtr->mHint,
                                       &sBitIdx);

        //해당 LG에 free가 있는것을 보고서 들어왔으므로
        //이게 거짓이된다면 심각한 에러상황이다.
        IDE_ERROR_MSG( sBitIdx < sLGHdrPtr->mValidBits,
                       "Error occurred while new extents alloc"
                       "(Tablespace ID : %"ID_UINT32_FMT", "
                       "Page ID : %"ID_UINT32_FMT", "
                       "Bit Index : %"ID_UINT32_FMT", "
                       "Valid Bits : %"ID_UINT32_FMT")",
                       aSpaceCache->mCommon.mSpaceID,
                       aLGHdrPID,
                       sBitIdx,
                       sLGHdrPtr->mValidBits );

        if( sBitIdx < sLGHdrPtr->mValidBits ) //인덱스값이 유효
        {
            allocByBitmapIndex( sLGHdrPtr,
                                sBitIdx );

            sNrExts--;

            //mFree, mBitmap을 모두 처리함
            IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                                 (UChar*)sLGHdrPtr,
                                                 &sBitIdx,
                                                 SDR_SDP_4BYTE,
                                                 SDR_SDPTB_ALLOC_IN_LG)
                      != IDE_SUCCESS );

            IDE_TEST( sdptbGroup::logAndModifyFreeExtsOfGG( aMtx,
                                                            aGGPtr,
                                                            -1 )
                      != IDE_SUCCESS );

            //extent의 첫번째 PID값을 얻는다.
            *aExtFstPID = sLGHdrPtr->mStartPID +
                         aSpaceCache->mCommon.mPagesPerExt*sBitIdx;

            //현재 extent의 마지막 pid를 구한다.
            sLastPID = SDPTB_LAST_PID_OF_EXTENT(
                                         *aExtFstPID ,
                                         aSpaceCache->mCommon.mPagesPerExt );

            //현재 할당된 extent의 마지막 page id 가 지금 설정된 값보다 크다면
            //HWM을 변경한다.
            if( aGGPtr->mHWM < sLastPID )
            {
                IDE_TEST( sdptbGroup::logAndSetHWMOfGG( aMtx,
                                                        aGGPtr,
                                                        sLastPID )
                          != IDE_SUCCESS );
            }

            aExtFstPID++;
        }// if
    }//while

    *aNrDone = aOrgNrExts - sNrExts;

    *aFreeInLG = sLGHdrPtr->mFree;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description:
 * 할당을 시도할수있는 유용한 FID를 얻는다.
 ***********************************************************************/
IDE_RC sdptbExtent::getAvailFID( sdptbSpaceCache   * aCache,
                                 sdFileID          * aFID)
{
    UInt sIdx;

    sdptbBit::findBitFromHintRotate( (void *)aCache->mFreenessOfGGs,
                                     aCache->mMaxGGID+1,   //검색대상비트수
                                     aCache->mGGIDHint,
                                     &sIdx );

    if( sIdx != SDPTB_BIT_NOT_FOUND )
    {
        IDE_ERROR_MSG( sIdx <= aCache->mMaxGGID,
                       "Error occurred while new extents alloc " 
                       "(Tablespace ID : %"ID_UINT32_FMT", "
                       "index : %"ID_UINT32_FMT", "
                       "MaxID : %"ID_UINT32_FMT")",
                       aCache->mCommon.mSpaceID,
                       sIdx,
                       aCache->mMaxGGID );

        *aFID = sIdx;

        /*
         * MaxGGID의 GG에 항상 free extent가 있는것은 아니다
         *
         * 예를들어,
         *   MaxGGID가 5일때
         *   다음과 같은 비트열이 mFreenessOfGGs에 저장되었을 수 있다.
         *
         *   11000
         *
         * 그러므로 if( sIdx >=  aCache->mMaxGGID )  로해야한다.
         */
        if( sIdx >=  aCache->mMaxGGID )
        {
            aCache->mGGIDHint = 0;
        }
        else
        {
            aCache->mGGIDHint = sIdx + 1;
        }

    }
    else
    {
        *aFID = SDPTB_NOT_FOUND;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE; 
}

/***********************************************************************
 * Description:
 *   deallcation LG hdr에 free가 있다면 switching을 한다.
 *   switching 을 했다면 aSwitching = ID_TRUE 가 된다.
 ***********************************************************************/
IDE_RC sdptbExtent::trySwitch( sdrMtx                 * aMtx,
                               sdptbGGHdr             * aGGHdrPtr,
                               idBool                 * aSwitching,
                               sdptbSpaceCache        * aCache )
{
    UInt                 sOldLGType; //alloc lg
    UInt                 sNewLGType; //dealloc lg
    sdptbLGFreeInfo     *sOldFNPtr;
    sdptbLGFreeInfo     *sNewFNPtr;

    IDE_ASSERT( aGGHdrPtr != NULL);
    IDE_ASSERT( aSwitching != NULL);
    IDE_ASSERT( aCache != NULL );
    IDE_ASSERT( aGGHdrPtr->mAllocLGIdx < SDPTB_ALLOC_LG_IDX_CNT );

    sOldLGType =  sdptbGroup::getAllocLGIdx(aGGHdrPtr);
    sNewLGType =  sdptbGroup::getDeallocLGIdx(aGGHdrPtr);

    sOldFNPtr = &aGGHdrPtr->mLGFreeness[sOldLGType];
    sNewFNPtr = &aGGHdrPtr->mLGFreeness[sNewLGType];

    //switching조건.
    if( (sOldFNPtr->mFreeExts == 0) && (sNewFNPtr->mFreeExts > 0) )
    {
        *aSwitching = ID_TRUE;

        IDE_TEST( sdptbGroup::logAndSetLGTypeOfGG( aMtx,
                                                   aGGHdrPtr,
                                                   sNewLGType )
                  != IDE_SUCCESS );

        /* BUG-47666 mFreenessOfGGs에 동시성 제어가 필요합니다. */
        sdptbBit::atomicSetBit32( (UInt*)aCache->mFreenessOfGGs,
                                  aGGHdrPtr->mGGID);
    }
    else
    {
        *aSwitching = ID_FALSE;

        //switching에 실패했다면,
        //기존의 free extents를 보고 0일때 cache의 freeness비트를 꺼야한다.
        if( sOldFNPtr->mFreeExts == 0 )
        {
            /* BUG-47666 mFreenessOfGGs에 동시성 제어가 필요합니다. */
            sdptbBit::atomicClearBit32( (UInt*)aCache->mFreenessOfGGs,
                                        aGGHdrPtr->mGGID);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description:
 *   on demand 로 datafile을 auto extend 한다.
 ***********************************************************************/
IDE_RC sdptbExtent::autoExtDatafileOnDemand( idvSQL           *  aStatistics,
                                             UInt                aSpaceID,
                                             sdptbSpaceCache  *  aCache,
                                             void*               aTransForMtx,
                                             UInt                aNeededPageCnt)
{
    sdrMtxStartInfo sStartInfo;
    idBool          sDoExtend;
    UInt            sState=0;

    /*
     * allocExt에서 어차피 extent의 크기의 배수로 이 값을 넘겨주기 때문에
     * 이럴경우는 없을것이다. 확실히 하기위해 assert
     */
    IDE_ASSERT( aCache         != NULL );
    IDE_ASSERT( aNeededPageCnt >= aCache->mCommon.mPagesPerExt );

    if ( aTransForMtx != NULL)
    {
        sStartInfo.mLogMode = SDR_MTX_LOGGING;
    }
    else
    {
        // Temproary Tablespace인 경우
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;
    }
    sStartInfo.mTrans = aTransForMtx;
    IDE_ERROR( sdptbGroup::prepareExtendFileOrWait( aStatistics,
                                                    aCache,
                                                    &sDoExtend)
                == IDE_SUCCESS);
    sState=1;

    IDE_TEST_CONT( sDoExtend == ID_FALSE, already_extended);

    IDE_TEST( sdptbGroup::makeMetaHeadersForAutoExtend( aStatistics,
                                                        &sStartInfo,
                                                        aSpaceID,
                                                        aCache,
                                                        aNeededPageCnt)
              != IDE_SUCCESS );

    sState=0;
    IDE_ERROR( sdptbGroup::completeExtendFileAndWakeUp( aStatistics,
                                                        aCache ) 
               == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( already_extended );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdptbGroup::completeExtendFileAndWakeUp( aStatistics,
                                                             aCache )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description: Temp Tablespace에서 extent를 미리 할당해서 cache한다.
 *              추가 확장하지는 않고, 확장된 영역에서만 할당한다.
 *
 * aStatistics    - [IN] 통계정보
 * aSpaceID       - [IN] Space ID
 **********************************************************************/
IDE_RC sdptbExtent::prepareCachedFreeExts( idvSQL           * aStatistics,
                                           sctTableSpaceNode* aSpaceNode )
{
    sdptbSpaceCache  *  sSpaceCache;
    sdrMtxStartInfo     sStartInfo;
    sdFileID            sFID = SD_MAKE_FID(SD_NULL_PID);
    UInt                sNrDone;
    scPageID            sExtFstPID;

    sStartInfo.mTrans   = NULL;
    sStartInfo.mLogMode = SDR_MTX_NOLOGGING;

    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceNode );
    IDE_ASSERT( sSpaceCache != NULL );

    IDE_ASSERT( sctTableSpaceMgr::isTempTableSpace( aSpaceNode ) == ID_TRUE );

    IDE_TEST( getAvailFID( sSpaceCache, &sFID ) != IDE_SUCCESS );

    while( sFID != SDPTB_NOT_FOUND )
    {
        IDE_TEST( tryAllocExtsInGG( aStatistics,
                                    &sStartInfo,
                                    sSpaceCache,
                                    sFID,
                                    1,
                                    &sExtFstPID,
                                    &sNrDone)
                  != IDE_SUCCESS );

        if( sNrDone == 0 )
        {
            break;
        }
        else
        {
            IDE_TEST( sSpaceCache->mFreeExtPool.push( ID_TRUE, /* Lock */
                                                      (void*)&sExtFstPID )
                      != IDE_SUCCESS );
        }

        IDE_TEST( getAvailFID( sSpaceCache, &sFID ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: [INTERFACE] temp tablespace로부터 extent를 할당받는다.
 *
 * BUG-24730: Drop된 Temp Segment의 Extent는 빠르게 재사용 되어야
 *            합니다.
 *
 * Temp Segment는 자주 Drop, Create되고 Bitmap Tablespace특성상
 * Extent를 Free하고 다시 할당시 Free된 Extent가 바로 할당되는 것이
 * 아니다. 예를 들면 TBS에 Free Extent가 < 1, 2, 3 >이 있다고 하자
 *
 *  1. alloc Extent = alloc:1, free extent< 2, 3 >
 *  2. free  Extent = free extent< 1, 2, 3 >
 *  3. alloc Extent = alloc:2, free extent< 1, 3 >
 *  4. free  Extent = free extent< 1, 2, 3 >
 *  5. alloc Extent = alloc:3, free extent< 1, 2 >
 *
 * Bitmap TBS는 Extent할당시 이전에 할당된 Extent가 Free되었다하더
 * 라도 할당시에는 File의 끝 부분으로 free extent를 찾는다. File
 * 끝까지 Free Extent를 모두 찾았다면 다시 처음부터 free extent를
 * 찾는다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] TableSpace ID
 * aExtSlot    - [OUT] 할당된 Extent Desc Array Ptr
 ***********************************************************************/
IDE_RC sdptbExtent::allocTmpExt( idvSQL          * aStatistics,
                                 scSpaceID         aSpaceID,
                                 sdpExtDesc      * aExtSlot )
{
    sdptbSpaceCache * sSpaceCache;
    sdrMtxStartInfo   sStartInfo;
    idBool            sIsEmpty;

    IDE_DASSERT( sctTableSpaceMgr::isTempTableSpace( aSpaceID ) == ID_TRUE );

    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ASSERT( sSpaceCache != NULL );

    IDE_TEST( sSpaceCache->mFreeExtPool.pop( ID_TRUE, /* Lock */
                                             (void*)&(aExtSlot->mExtFstPID),
                                             &sIsEmpty )
              != IDE_SUCCESS );

    if( sIsEmpty == ID_FALSE )
    {
        aExtSlot->mLength = sSpaceCache->mCommon.mPagesPerExt;
    }
    else
    {
        sStartInfo.mTrans   = NULL;
        sStartInfo.mLogMode = SDR_MTX_NOLOGGING;

        IDE_TEST( allocExts( aStatistics,
                             &sStartInfo,
                             aSpaceID,
                             1, /*need extent count */
                             aExtSlot ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description: 하나의 exts를 TBS에 반납한다.
 *   NTA처리시 사용됨.
 *
 * aStatistics - [IN] 통계정보
 * aMtx        - [IN] Mini transaction
 * aSpaceID    - [IN] 반환할 tablespace id
 * aExtFstPID  - [IN] 반환할 Extent의 첫번째 PageID
 * aNrDone     - [OUT] free에 성공한 ext의 갯수
 ***********************************************************************/
IDE_RC sdptbExtent::freeExt( idvSQL           *  aStatistics,
                             sdrMtx           *  aMtx,
                             scSpaceID           aSpaceID,
                             scPageID            aExtFstPID,
                             UInt            *   aNrDone )
{
    UInt                 sLGID;
    UInt                 sEndIdx;
    sdptbSpaceCache    * sSpaceCache;
    sdptbSortExtSlot     sExtSlot;

    IDE_ASSERT( aNrDone != NULL);

    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ERROR_MSG( sSpaceCache != NULL,
                   "Tablespace cache not found (ID : %"ID_UINT32_FMT")",
                    aSpaceID );

    sLGID = SDPTB_GET_LGID_BY_PID( aExtFstPID,
                                   sSpaceCache->mCommon.mPagesPerExt);

    sExtSlot.mExtFstPID    = aExtFstPID;
    sExtSlot.mLength       = 1;
    sExtSlot.mLocalGroupID = sLGID  ;

    IDE_ERROR( sctTableSpaceMgr::isDiskTableSpace(aSpaceID ) == ID_TRUE );

    IDE_TEST( freeExtsInLG( aStatistics,
                            aMtx,
                            aSpaceID,
                            &sExtSlot,
                            1,   // aNrSortedExt
                            0,   // aBeginIdx
                            &sEndIdx,
                            aNrDone)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: 하나의 extent를 Temp TBS에 반납한다.
 *
 * aSpaceID    - [IN] 반환할 tablespace id
 * aExtFstPID  - [IN] 반환할 Extent의 첫번째 PageID
 ***********************************************************************/
IDE_RC sdptbExtent::freeTmpExt( scSpaceID   aSpaceID,
                                scPageID    aExtFstPID )
{
    sdptbSpaceCache    * sSpaceCache;

    // Temp Segment만 재활용된다.
    IDE_ASSERT_MSG( sctTableSpaceMgr::isTempTableSpace( aSpaceID ) == ID_TRUE,
                   "Error occurred during drop Temp Table "
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "PID : %"ID_UINT32_FMT")",
                   aSpaceID,
                   aExtFstPID );

    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ERROR_MSG( sSpaceCache != NULL,
                   "Tablespace cache not found (ID : %"ID_UINT32_FMT")",
                    aSpaceID );

    IDE_ERROR_MSG( aExtFstPID != SC_NULL_PID,
                   "Error occurred during drop Temp Table "
                   "(Tablespace ID : %"ID_UINT32_FMT", "
                   "PID : %"ID_UINT32_FMT")",
                   aSpaceID,
                   aExtFstPID );

    IDE_TEST( sSpaceCache->mFreeExtPool.push( ID_TRUE, /* Lock */
                                              (void*)&(aExtFstPID) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  extent들을 TBS에 반납한다.  NTA처리시 사용된다.
 *  free할 모든 extent는 같은 LG에 있다.(setNTA를 그렇게 찍음)
 *
 *  aNrDone          - [OUT] free에 성공한 ext의 갯수
 ***********************************************************************/
IDE_RC sdptbExtent::freeExts( idvSQL           *  aStatistics,
                              sdrMtx           *  aMtx,
                              scSpaceID           aSpaceID,
                              ULong            *  aExtFstPIDs,
                              UInt                aArrElements)
{
    UInt                   sLGID;
    sdptbSortExtSlot       sExtSlot[4]; //allocExt에서 최대 extent 4개할당하므로.
    UInt                   sDummy;
    UInt                   sNrDone;
    UInt                   i;
    sdptbSpaceCache      * sSpaceCache;

    IDE_ASSERT( aMtx != NULL);
    IDE_ASSERT( aExtFstPIDs != NULL );
    IDE_ASSERT( aArrElements > 0 );

    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ASSERT( sSpaceCache != NULL );

    /*
     * 하나의 LG에 있는 extent들에 대해서만 NTA가 찍힌다.
     * 그러므로, free할 모든 extent들은 같은 LG에 있다.
     */
    sLGID = SDPTB_GET_LGID_BY_PID( *aExtFstPIDs,
                                   sSpaceCache->mCommon.mPagesPerExt );

    //본 함수는 NTA Undo시에만 호출되며, TempTablespace는 Undo돼지 않는다.
    IDE_ASSERT( sctTableSpaceMgr::isTempTableSpace( aSpaceID ) != ID_TRUE );

    for( i=0 ; i < aArrElements ; i++ )
    {
        sExtSlot[i].mExtFstPID    = *aExtFstPIDs;
        sExtSlot[i].mLength       = sSpaceCache->mCommon.mPagesPerExt;
        sExtSlot[i].mLocalGroupID = sLGID  ;
        aExtFstPIDs++;
    }

    IDE_TEST( freeExtsInLG( aStatistics,
                            aMtx,
                            aSpaceID,
                            sExtSlot,
                            aArrElements,
                            0,       // aBeginIdx
                            &sDummy, //aEndIdx
                            &sNrDone)
              != IDE_SUCCESS );

    // BUG-27329 CodeSonar::Uninitialized Variable (2)
    //요청한 모든 extent를 해제해야한다.
    IDE_ASSERT( sNrDone == aArrElements );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *   [INTERFACE] free extents를 LG에 반납한다.
 *
 * aSortedExts  - [IN] sdptbSortExtSlot를 요소로 갖는 배열의 시작주소
 * sNrElement   - [IN] 윗 배열의 요소의 갯수
 *                     (segment에서 넘겨주는 aSortedExts와
 *                     sNrElement의 값은 이함수 안에서 변하지 않는다.)
 * aEndIdx      - [OUT] free에 성공한 마지막 index
 * aNrDone      - [OUT] free에 성공한 ext의 갯수
 ***********************************************************************/
IDE_RC sdptbExtent::freeExtsInLG( idvSQL           *  aStatistics,
                                  sdrMtx           *  aMtx,
                                  scSpaceID           aSpaceID,
                                  sdptbSortExtSlot *  aSortedExts,
                                  UInt                aNrElement,
                                  UInt                aBeginIndex,
                                  UInt             *  aEndIndex,
                                  UInt             *  aNrDone)
{
    UInt                  sGGHdrPID;
    sdptbGGHdr        *   sGGHdrPtr;
    sdptbLGHdr        *   sLGHdrPtr;
    idBool                sDummy;
    sdptbSpaceCache   *   sCache;
    UInt                  i;
    sdFileID              sFID;
    UInt                  sLGID;
    UInt                  sStartFPID;
    UInt                  sExtentIDInLG; //LG의 mBitmap에 세트할때 사용
    UChar             *   sPagePtr;
    idBool                sSwitching;
    ULong                 sTemp;
    UInt                  sDeallocLGType;

    IDE_ASSERT( aSortedExts != NULL );
    IDE_ASSERT( aEndIndex != NULL );
    IDE_ASSERT( aNrDone != NULL );

    sCache  = sddDiskMgr::getSpaceCache( aSpaceID );
    /* writeCommitLog 이후 / undo 작업이므로 예외처리 하지 않는다. */
    IDE_ASSERT_MSG( sCache != NULL,
                   "Tablespace cache not found (ID : %"ID_UINT32_FMT")",
                   aSpaceID );

    i       = aBeginIndex;
    sFID    = SD_MAKE_FID( aSortedExts[i].mExtFstPID );
    sLGID   = aSortedExts[i].mLocalGroupID;

    IDE_ASSERT_MSG( sLGID < SDPTB_LGID_MAX ,
                   "Tablespace cache not found "
                   "(TableSpace ID : %"ID_UINT32_FMT", "
                   "Page ID : %"ID_UINT32_FMT", "
                   "ID : %"ID_UINT32_FMT")",
                   aSpaceID,
                   sFID,
                   sLGID );

    sGGHdrPID =SDPTB_GET_GGHDR_PID_BY_FID( sFID );
    IDE_TEST(sdbBufferMgr::getPageByPID( aStatistics,
                                         aSpaceID,
                                         sGGHdrPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (void*)aMtx,
                                         (UChar**)&sPagePtr,
                                         &sDummy,
                                         NULL /*IsCorruptPage*/ )
             != IDE_SUCCESS);

    sGGHdrPtr = sdptbGroup::getGGHdr( sPagePtr);

    sStartFPID = SDPTB_EXTENT_START_FPID_FROM_LGID(sLGID, sGGHdrPtr->mPagesPerExt);

    //해당 LG의 dealloc page에서 ext들을 해제한다.
    IDE_TEST(sdbBufferMgr::getPageByPID(
                                 aStatistics,
                                 aSpaceID,
                                 sdptbGroup::getDeallocLGHdrPID( sGGHdrPtr, sLGID ),
                                 SDB_X_LATCH,
                                 SDB_WAIT_NORMAL,
                                 SDB_SINGLE_PAGE_READ,
                                 (void*)aMtx,
                                 (UChar**)&sPagePtr,
                                 &sDummy,
                                 NULL /*IsCorruptPage*/ )
             != IDE_SUCCESS);

    sLGHdrPtr = sdptbGroup::getLGHdr( sPagePtr);

    ////////////////////////////////////////////////////////
    // 동일한 LG에 있는 모든 extent를 한꺼번에 해제한다.
    ////////////////////////////////////////////////////////
    while( (sFID == SD_MAKE_FID( aSortedExts[i].mExtFstPID) )
           && ( sLGID == aSortedExts[i].mLocalGroupID ) )
    {
        sExtentIDInLG = (SD_MAKE_FPID(
                        aSortedExts[i].mExtFstPID )
                        - sStartFPID ) /sGGHdrPtr->mPagesPerExt;

        freeByBitmapIndex( sLGHdrPtr,
                           sExtentIDInLG );

        //LG hdr의 mFree, mBitmap 에대한 로깅남김
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sLGHdrPtr,
                                             &sExtentIDInLG,
                                             ID_SIZEOF( sExtentIDInLG ),
                                             SDR_SDPTB_FREE_IN_LG)
                  != IDE_SUCCESS );

        sDeallocLGType = sdptbGroup::getDeallocLGIdx(sGGHdrPtr);

        /*
         * GG hdr의 mFreeExts , mBits 에대한 로깅을 남긴다.
         */
        IDE_TEST( sdptbGroup::logAndModifyFreeExtsOfGGByLGType( aMtx,
                                                                sGGHdrPtr,
                                                                sDeallocLGType,
                                                                +1 )
                  != IDE_SUCCESS );


        //dealloc LG에 해당비트가 꺼져있을때만 로깅
        if( sdptbBit::getBit( sGGHdrPtr->mLGFreeness[sDeallocLGType].mBits,
                              sLGID) == SDPTB_BIT_OFF)
        {
            sdptbBit::setBit(sGGHdrPtr->mLGFreeness[sDeallocLGType].mBits , sLGID );

            if( sLGID < SDPTB_BITS_PER_ULONG )
            {
                sTemp = sGGHdrPtr->mLGFreeness[sDeallocLGType].mBits[0];

                IDE_TEST( sdrMiniTrans::writeNBytes(
                            aMtx,
                            (UChar*)
                            &(sGGHdrPtr->mLGFreeness[sDeallocLGType].mBits[0]),
                            &sTemp,
                            ID_SIZEOF(sTemp) ) != IDE_SUCCESS );
            }
            else
            {
                sTemp = sGGHdrPtr->mLGFreeness[sDeallocLGType].mBits[1];

                IDE_TEST( sdrMiniTrans::writeNBytes(
                            aMtx,
                            (UChar*)
                            &(sGGHdrPtr->mLGFreeness[sDeallocLGType].mBits[1]),
                            &sTemp,
                            ID_SIZEOF(sTemp) ) != IDE_SUCCESS );
            }
        }

        i++;

        if( i ==  aNrElement )
        {
            break;
        }
    }
    
    IDE_ASSERT_MSG( i > aBeginIndex,
                    "Error occurred while extent free "
                    "(Tablespace ID : %"ID_UINT32_FMT", "
                    "PageID : %"ID_UINT32_FMT", "
                    "BeginIndex : %"ID_UINT32_FMT", "
                    "Index : %"ID_UINT32_FMT")",
                    aSpaceID,
                    sdptbGroup::getDeallocLGHdrPID( sGGHdrPtr, sLGID ),
                    aBeginIndex,
                    i );

    /*
     * switching을 고려한다.
     */
    IDE_TEST( trySwitch( aMtx,
                         sGGHdrPtr,
                         &sSwitching,
                         sCache )
              != IDE_SUCCESS );

    *aNrDone = i - aBeginIndex;

    /*
     * end index는 해제가 완.료.된 마지막 인덱스 번호이다
     * 그러므로 i값을 감소시킨후 대입해야한다.
     */
    *aEndIndex = --i;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description:
 ***********************************************************************/
void sdptbExtent::allocByBitmapIndex( sdptbLGHdr * aLGHdr,
                                      UInt         aIndex )
{
    IDE_ASSERT( aLGHdr != NULL );

    sdptbBit::setBit( aLGHdr->mBitmap, aIndex);

    IDE_ASSERT( aLGHdr->mFree > 0 ); // free extent가 남아 있어야 할당 가능
    aLGHdr->mFree-- ;

    // Hint 정보를 세트한다.
    aLGHdr->mHint = aIndex + 1;
}

/***********************************************************************
 * Description:
 ***********************************************************************/
void sdptbExtent::freeByBitmapIndex( sdptbLGHdr * aLGHdr,
                                     UInt         aIndex)
{
    IDE_ASSERT( aLGHdr != NULL );

    sdptbBit::clearBit( aLGHdr->mBitmap, aIndex);
    aLGHdr->mFree++ ;

    IDE_ASSERT( aLGHdr->mValidBits >= aLGHdr->mFree );

    if( aLGHdr->mHint > aIndex )
    {
        aLGHdr->mHint = aIndex;
    }
}

/***********************************************************************
 *
 * Description : Undo TBS의 Free ExtDir 페이지를 할당한다.
 *
 ***********************************************************************/
IDE_RC sdptbExtent::tryAllocExtDir( idvSQL            * aStatistics,
                                    sdrMtxStartInfo   * aStartInfo,
                                    scSpaceID           aSpaceID,
                                    sdpFreeExtDirType   aFreeListIdx,
                                    scPageID          * aExtDirPID )
{
    UInt                 sState = 0;
    smLSN                sOpLSN;
    sdrMtx               sMtx;
    UChar              * sPagePtr;
    ULong                sNodeCnt;
    ULong                sData[2];
    sdptbGGHdr         * sGGHdrPtr;
    sdpSglPIDListNode  * sNode;
    sdptbSpaceCache    * sSpaceCache;
    idBool               sCurPageLatched;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aExtDirPID != NULL );
    IDE_ASSERT( sctTableSpaceMgr::isUndoTableSpace(aSpaceID) == ID_TRUE );

    *aExtDirPID     = SD_NULL_PID;

    sPagePtr        = NULL;
    sCurPageLatched = ID_FALSE;
    sSpaceCache     = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ERROR_MSG( sSpaceCache != NULL,
                   "Tablespace cache not found (ID : %"ID_UINT32_FMT")",
                   aSpaceID );

    IDE_TEST_CONT( sSpaceCache->mArrIsFreeExtDir[ aFreeListIdx ] == ID_FALSE,
                    CONT_NOT_FOUND_FREE_EXTDIR );

    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          SDPTB_GET_GGHDR_PID_BY_FID(0),
                                          &sPagePtr ) 
              != IDE_SUCCESS );
    sState = 1;

    sGGHdrPtr = sdptbGroup::getGGHdr( sPagePtr );

    sNodeCnt  = sdpSglPIDList::getNodeCnt(
                &(sGGHdrPtr->mArrFreeExtDirList[aFreeListIdx]));

    IDE_TEST_CONT( sNodeCnt == 0,
                    CONT_NOT_FOUND_FREE_EXTDIR_ONLYFIX );

    /*
     * BUG-25708 [5.3.1] UndoTBS에 Free ExtDirPage List가 가용해도 Race
     *           발생하면 조금씩 HWM가 증가될 수 있음.
     */
     sdbBufferMgr::latchPage( aStatistics,
                              sPagePtr,
                              SDB_X_LATCH,
                              SDB_WAIT_NORMAL,
                              &sCurPageLatched  );

    IDE_TEST_CONT( sCurPageLatched == ID_FALSE,
                    CONT_NOT_FOUND_FREE_EXTDIR_ONLYFIX );
    sState = 2;
    sNodeCnt  = sdpSglPIDList::getNodeCnt(
                           &(sGGHdrPtr->mArrFreeExtDirList[aFreeListIdx]));

    if ( sNodeCnt > 0 )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 3;

        IDE_ASSERT( sdrMiniTrans::getTrans(&sMtx) != NULL );

        sOpLSN = smLayerCallback::getLstUndoNxtLSN( aStartInfo->mTrans );

        IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx, sPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSglPIDList::removeNodeAtHead(
                              aStatistics,
                              &(sGGHdrPtr->mArrFreeExtDirList[aFreeListIdx]),
                              &sMtx,
                              aExtDirPID,
                              &sNode ) 
                   != IDE_SUCCESS );

        IDE_ASSERT( *aExtDirPID != SC_NULL_PID );

        sData[0] = aFreeListIdx;
        sData[1] = *aExtDirPID;

        sdrMiniTrans::setNTA( &sMtx,
                              aSpaceID,
                              SDR_OP_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST,
                              &sOpLSN,
                              sData,
                              2 /* Data Count */);

        if ( sNodeCnt == 1 )
        {
            sSpaceCache->mArrIsFreeExtDir[ aFreeListIdx ] = ID_FALSE;
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( CONT_NOT_FOUND_FREE_EXTDIR_ONLYFIX );

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( CONT_NOT_FOUND_FREE_EXTDIR );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            sSpaceCache->mArrIsFreeExtDir[ aFreeListIdx ] = ID_TRUE;
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
            break;

        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                        == IDE_SUCCESS );
            break;

        case 1:
            IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, sPagePtr )
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : ExtDir 페이지를 해제한다.
 *
 ***********************************************************************/
IDE_RC sdptbExtent::freeExtDir( idvSQL            * aStatistics,
                                sdrMtx            * aMtx,
                                scSpaceID           aSpaceID,
                                sdpFreeExtDirType   aFreeListIdx,
                                scPageID            aExtDirPID )
{

    idBool               sDummy;
    UChar              * sPagePtr;
    UChar              * sExtDirPagePtr;
    ULong                sNodeCnt;
    sdptbGGHdr         * sGGHdrPtr;
    sdptbSpaceCache    * sSpaceCache;

    IDE_ASSERT( aExtDirPID != SD_NULL_PID );
    IDE_ASSERT( aMtx       != NULL );

    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceID );
    IDE_ASSERT( sSpaceCache != NULL );

    IDE_TEST(sdbBufferMgr::getPageByPID( aStatistics,
                                         aSpaceID,
                                         SDPTB_GET_GGHDR_PID_BY_FID(0),
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         &sPagePtr,
                                         &sDummy,
                                         NULL ) != IDE_SUCCESS);

    sGGHdrPtr = sdptbGroup::getGGHdr( sPagePtr );

    sNodeCnt  = sdpSglPIDList::getNodeCnt(
                &(sGGHdrPtr->mArrFreeExtDirList[aFreeListIdx]));

    IDE_TEST(sdbBufferMgr::getPageByPID( aStatistics,
                                         aSpaceID,
                                         aExtDirPID,
                                         SDB_X_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         aMtx,
                                         &sExtDirPagePtr,
                                         &sDummy,
                                         NULL ) != IDE_SUCCESS);

    IDE_TEST( sdpSglPIDList::addNode2Head(
              &(sGGHdrPtr->mArrFreeExtDirList[aFreeListIdx]),
              sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*)sExtDirPagePtr ),
              aMtx) != IDE_SUCCESS );

    if ( sNodeCnt == 0 )
    {
        sSpaceCache->mArrIsFreeExtDir[ aFreeListIdx ] = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  space id와 page id를 입력받아서
 *  page 가 속한 extent가 free 상태인지 검사 한다.
 *  필요하다면 해당 extent의 fst pid와 lst pid를 반환할 수 있다.
 *
 * aStatistics - [IN]  통계정보
 * aSpaceID    - [IN]  확인하고자 하는 page의 space id
 * aPageID     - [IN]  확인하고자 하는 page의 page id
 * aIsFreeExt  - [OUT] page가 속한 extent의 free 여부를 반환한다.
 ******************************************************************************/
IDE_RC sdptbExtent::isFreeExtPage( idvSQL      * aStatistics,
                                   scSpaceID     aSpaceID,
                                   scPageID      aPageID,
                                   idBool      * aIsFreeExt )
{
    UChar           * sGGHdrPagePtr ;
    UChar           * sLGHdrPagePtr ;
    UInt              sPageState = 0;
    scPageID          sGGPID;
    scPageID          sLGPID;
    scPageID          sExtFstPID = 0;
    UInt              sLGID;
    UInt              sPagesPerExt;
    UInt              sExtentIdx;
    sdptbGGHdr      * sGGHdr;
    sdptbLGHdr      * sLGHdr;
    idBool            sIsAllocExt;
    idBool            sTry;
    smiTableSpaceAttr sTBSAttr;

    IDE_ASSERT( sdpTableSpace::getExtMgmtType( aSpaceID ) == SMI_EXTENT_MGMT_BITMAP_TYPE );
    IDE_ASSERT( aIsFreeExt != NULL );

    /* BUG-27608 CodeSonar::Division By Zero (3)
     * Tablespace가 Drop된 경우 Page 사용하지 않으므로,
     * Corrupt Page어도 무시 할 수 있음
     */
    if( sctTableSpaceMgr::hasState( aSpaceID, SCT_SS_INVALID_DISK_TBS ) == ID_TRUE )
    {
        *aIsFreeExt = ID_TRUE;

        IDE_CONT( skip_check_free_page );
    }

    *aIsFreeExt = ID_FALSE;

    sPagesPerExt = sdpTableSpace::getPagesPerExt( aSpaceID );

    // BUG-27608 CodeSonar::Division By Zero (3)
    IDE_ASSERT( 0 < sPagesPerExt );

    //------------------------------------------
    // GG Hdr PID와 LG Hdr PID를 구해서
    // 받은 aPageID가 GG,LG Hdr인지 확인한다.
    //------------------------------------------

    sGGPID = SDPTB_GET_GGHDR_PID_BY_FID( SD_MAKE_FID( aPageID ) );

    IDE_TEST_RAISE( aPageID == sGGPID , fail_read_gg );

    IDE_TEST_RAISE( sdbBufferMgr::getPageByPID( aStatistics,
                                                aSpaceID,
                                                sGGPID,
                                                SDB_S_LATCH,
                                                SDB_WAIT_NORMAL,
                                                SDB_SINGLE_PAGE_READ,
                                                NULL, /* aMtx */
                                                &sGGHdrPagePtr,
                                                &sTry,
                                                NULL /* isCorruptPage*/)
                    != IDE_SUCCESS , fail_read_gg );

    sPageState = 1 ;

    sGGHdr = sdptbGroup::getGGHdr( sGGHdrPagePtr );

    sLGID = SDPTB_GET_LGID_BY_PID( aPageID, sPagesPerExt );

    sLGPID = SDPTB_LG_HDR_PID_FROM_LGID( SD_MAKE_FID( aPageID ),
                                         sLGID,
                                         sdptbGroup::getAllocLGIdx( sGGHdr ),
                                         sPagesPerExt );

    IDE_TEST_RAISE( aPageID == sLGPID , fail_read_lg );

    // alloc Group Header pid를 이용해서
    // dealloc Group Header의 pid를 계산한다.
    sLGPID = sLGPID + ( sdptbGroup::getAllocLGIdx( sGGHdr ) * (-2) + 1 ) ;

    IDE_TEST_RAISE( aPageID == sLGPID , fail_read_lg );

    sExtFstPID = SDPTB_GET_EXTENT_PID_BY_PID( aPageID, sPagesPerExt );

    //------------------------------------------
    // Hdr를 읽어서 page가 속한 extent가 free인지 확인한다.
    //------------------------------------------

    // extent는 HWM이내에 있어야 한다. LGID는 LGCnt보다 클수 없다.
    // 조건을 만족하지 않으면 아직 할당된 적이 없는 extent이다.
    if( ( sGGHdr->mHWM >= sExtFstPID ) && ( sGGHdr->mLGCnt > sLGID ) )
    {
        // 할당 된 적이 있는 extent이다.
        // dealloc LG Hdr의 bitmap을 확인해서 free가 되었는지 확인한다.

        IDE_TEST_RAISE( sdbBufferMgr::getPageByPID( aStatistics,
                                                    aSpaceID,
                                                    sLGPID,
                                                    SDB_S_LATCH,
                                                    SDB_WAIT_NORMAL,
                                                    SDB_SINGLE_PAGE_READ,
                                                    NULL, /* aMtx */
                                                    &sLGHdrPagePtr,
                                                    &sTry,
                                                    NULL /* isCorruptPage*/ )
                        != IDE_SUCCESS , fail_read_lg );

        sPageState = 2 ;

        sLGHdr = sdptbGroup::getLGHdr( sLGHdrPagePtr );

        // bit를 확인하여 extent의 free여부를 알아낸다.

        sExtentIdx  = SDPTB_EXTENT_IDX_AT_LG_BY_PID( sExtFstPID, sPagesPerExt );

        sIsAllocExt = sdptbBit::getBit( sLGHdr->mBitmap, sExtentIdx );

        *aIsFreeExt = ( sIsAllocExt == ID_FALSE ) ? ID_TRUE : ID_FALSE ;

        sPageState = 1 ;

        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             sLGHdrPagePtr )
                  != IDE_SUCCESS );
    }
    else
    {
        // 할당된 적이 없는 extent이므로 Free 이다.
        *aIsFreeExt = ID_TRUE;
    }

    sPageState = 0 ;

    IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                         sGGHdrPagePtr )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_check_free_page );

    return IDE_SUCCESS;

    // GG,LG Hdr가 corrupted page인 경우
    IDE_EXCEPTION( fail_read_gg );
    {
        sctTableSpaceMgr::getTBSAttrByID( aStatistics,
                                          aSpaceID,
                                          &sTBSAttr );
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     SM_TRC_DRECOVER_GROUPHDR_IS_CORRUPTED,
                     sTBSAttr.mName,
                     sGGPID,
                     aPageID );

        IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                  aSpaceID,
                                  sGGPID ));
    }
    IDE_EXCEPTION( fail_read_lg );
    {
        sctTableSpaceMgr::getTBSAttrByID( aStatistics,
                                          aSpaceID,
                                          &sTBSAttr );
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     SM_TRC_DRECOVER_GROUPHDR_IS_CORRUPTED,
                     sTBSAttr.mName,
                     sLGPID,
                     aPageID );

        IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                  aSpaceID,
                                  sLGPID ));
    }
    IDE_EXCEPTION_END;

    switch( sPageState )
    {
        case 2 :
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sLGHdrPagePtr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sGGHdrPagePtr )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}
