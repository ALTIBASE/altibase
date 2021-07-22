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
 *
 * $Id: sdpstFT.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에 대한 Fixed Table의 구현파일이다.
 *
 **********************************************************************/

# include <idl.h>

# include <smErrorCode.h>
# include <sdb.h>
# include <sdpPhyPage.h>
# include <sdpDef.h>

# include <sdpstSH.h>
# include <sdpstDef.h>
# include <sdpstFT.h>
# include <sdpstStackMgr.h>
# include <sdpstBMP.h>
# include <sdpstRtBMP.h>
# include <sdpstItBMP.h>
# include <sdpstLfBMP.h>
# include <sdcFT.h>
# include <smiFixedTable.h>

IDE_RC sdpstFT::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC sdpstFT::destroy()
{
    return IDE_SUCCESS;
}

/*************************************************************************
 * DESCRIPTION: Rt-BMP, It-BMP, Lf-BMP 구조를 출력하기 위한 함수
 * aBMPHdr는 반드시 It-BMP이어야 한다.
 *************************************************************************/
void sdpstFT::makeDumpBMPStructureInfo(
                                sdpstBMPHdr                 * aItBMPHdr,
                                SShort                        aSlotNo,
                                scPageID                      aParentPID,
                                sdpSegType                    aSegType,
                                sdpstDumpBMPStructureInfo   * aBMPInfo )
{
    sdpstBMPSlot    * sSlotPtr;

    IDE_ASSERT( aItBMPHdr  != NULL );
    IDE_ASSERT( aBMPInfo != NULL );

    sSlotPtr         = sdpstBMP::getSlot( aItBMPHdr, aSlotNo );

    aBMPInfo->mSegType = sdcFT::convertSegTypeToChar( aSegType );
    aBMPInfo->mRtBMP   = aParentPID;
    aBMPInfo->mItBMP   = sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aItBMPHdr) );
    aBMPInfo->mLfBMP   = sSlotPtr->mBMP;

    toStrMFNL( sSlotPtr->mMFNL, aBMPInfo->mLfBMPMFNLStr );
}

/*************************************************************************
 * DESCRIPTION: Rt-BMP, It-BMP, Lf-BMP 공통으로 사용되는 정보를 가져온다.
 *************************************************************************/
void sdpstFT::makeDumpBMPHdrInfo( sdpstBMPHdr            * aBMPHdr,
                                  sdpSegType               aSegType,
                                  sdpstDumpBMPHdrInfo    * aBMPHdrInfo )
{
    SInt    sLoop;

    IDE_ASSERT( aBMPHdr     != NULL );
    IDE_ASSERT( aBMPHdrInfo != NULL );

    aBMPHdrInfo->mSegType       = sdcFT::convertSegTypeToChar( aSegType );
    aBMPHdrInfo->mPageID        = 
        sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aBMPHdr) );

    aBMPHdrInfo->mParentInfo    = aBMPHdr->mParentInfo;
    aBMPHdrInfo->mSlotCnt       = aBMPHdr->mSlotCnt;
    aBMPHdrInfo->mFreeSlotCnt   = aBMPHdr->mFreeSlotCnt;
    aBMPHdrInfo->mMaxSlotCnt    = aBMPHdr->mMaxSlotCnt;
    aBMPHdrInfo->mFstFreeSlotNo = aBMPHdr->mMaxSlotCnt;
    aBMPHdrInfo->mNxtRtBMP      = aBMPHdr->mNxtRtBMP;

    toStrMFNL( aBMPHdr->mMFNL, aBMPHdrInfo->mMFNLStr );
    toStrBMPType( aBMPHdr->mType, aBMPHdrInfo->mTypeStr );

    for ( sLoop = 0; sLoop < SDPST_MFNL_MAX; sLoop++ )
    {
        aBMPHdrInfo->mMFNLTbl[sLoop] = aBMPHdr->mMFNLTbl[sLoop];
    }
}

/*************************************************************************
 * DESCRIPTION: Rt-BMP, It-BMP, Lf-BMP 공통으로 사용되는 정보를 가져온다.
 *************************************************************************/
void sdpstFT::makeDumpBMPBodyInfo( sdpstBMPHdr            * aBMPHdr,
                                   SShort                   aSlotNo,
                                   sdpSegType               aSegType,
                                   sdpstDumpBMPBodyInfo   * aBMPBodyInfo )
{
    sdpstBMPSlot    * sSlotPtr;

    IDE_ASSERT( aBMPHdr      != NULL );
    IDE_ASSERT( aBMPBodyInfo != NULL );


    aBMPBodyInfo->mSegType = sdcFT::convertSegTypeToChar( aSegType );
    aBMPBodyInfo->mPageID =
        sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aBMPHdr) );

    sSlotPtr           = sdpstBMP::getSlot( aBMPHdr, aSlotNo );
    aBMPBodyInfo->mBMP = sSlotPtr->mBMP;

    toStrMFNL( sSlotPtr->mMFNL, aBMPBodyInfo->mMFNLStr );
}

/*************************************************************************
 * DESCRIPTION: Lf-BMP 사용되는 정보를 가져온다.
 *************************************************************************/
void sdpstFT::makeDumpLfBMPHdrInfo( sdpstLfBMPHdr          * aLfBMPHdr,
                                    sdpSegType               aSegType,
                                    sdpstDumpLfBMPHdrInfo  * aLfBMPHdrInfo )
{
    IDE_ASSERT( aLfBMPHdr     != NULL );
    IDE_ASSERT( aLfBMPHdrInfo != NULL );

    makeDumpBMPHdrInfo( &aLfBMPHdr->mBMPHdr,
                        aSegType,
                        &aLfBMPHdrInfo->mCommon );

    aLfBMPHdrInfo->mPageRange        = aLfBMPHdr->mPageRange;
    aLfBMPHdrInfo->mTotPageCnt       = aLfBMPHdr->mTotPageCnt;
    aLfBMPHdrInfo->mFstDataPagePBSNo = aLfBMPHdr->mFstDataPagePBSNo;
}

/*************************************************************************
 * DESCRIPTION: Lf-BMP 사용되는 정보를 가져온다.
 *************************************************************************/
void sdpstFT::makeDumpLfBMPRangeSlotInfo(
                         sdpstLfBMPHdr               * aLfBMPHdr,
                         SShort                        aSlotNo,
                         sdpSegType                    aSegType,
                         sdpstDumpLfBMPRangeSlotInfo * aLfBMPBodyInfo )
{
    sdpstRangeMap       * sRangeMap;
    sdpstRangeSlot      * sRangeSlot;

    IDE_ASSERT( aLfBMPHdr      != NULL );
    IDE_ASSERT( aLfBMPBodyInfo != NULL );

    idlOS::memset( aLfBMPBodyInfo, 0x00, ID_SIZEOF(*aLfBMPBodyInfo) );

    sRangeMap  = sdpstLfBMP::getMapPtr( aLfBMPHdr );
    sRangeSlot = &sRangeMap->mRangeSlot[aSlotNo];

    /* make RangeSlot */

    aLfBMPBodyInfo->mSegType        = sdcFT::convertSegTypeToChar( aSegType );
    aLfBMPBodyInfo->mPageID         = 
            sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aLfBMPHdr) );
    aLfBMPBodyInfo->mRangeSlotNo    = aSlotNo;
    aLfBMPBodyInfo->mFstPID         = sRangeSlot->mFstPID;
    aLfBMPBodyInfo->mLength         = sRangeSlot->mLength;
    aLfBMPBodyInfo->mFstPBSNo       = sRangeSlot->mFstPBSNo;
    aLfBMPBodyInfo->mExtDirPID      = sRangeSlot->mExtDirPID;
    aLfBMPBodyInfo->mSlotNoInExtDir = sRangeSlot->mSlotNoInExtDir;
}

/*************************************************************************
 * DESCRIPTION: Lf-BMP 사용되는 정보를 가져온다.
 *************************************************************************/
void sdpstFT::makeDumpLfBMPPBSTblInfo(
                                    sdpstLfBMPHdr            * aLfBMPHdr,
                                    sdpSegType                 aSegType,
                                    sdpstDumpLfBMPPBSTblInfo * aLfBMPBodyInfo )
{
    sdpstRangeMap       * sRangeMap;
    sdpstPBS            * sPBSTbl;
    sdpstPBS              sPBS;
    SShort                sCurPBSNo;
    UInt                  sBitIdx = 0;

    IDE_ASSERT( aLfBMPHdr      != NULL );
    IDE_ASSERT( aLfBMPBodyInfo != NULL );

    idlOS::memset( aLfBMPBodyInfo, 0x00, ID_SIZEOF(*aLfBMPBodyInfo) );

    sRangeMap = sdpstLfBMP::getMapPtr( aLfBMPHdr );
    sPBSTbl   = sRangeMap->mPBSTbl;

    aLfBMPBodyInfo->mSegType = sdcFT::convertSegTypeToChar( aSegType );
    aLfBMPBodyInfo->mPageID  =
            sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aLfBMPHdr) );

    /* make RangeSlot */
    for ( sCurPBSNo = 0;
          sCurPBSNo < aLfBMPHdr->mTotPageCnt;
          sCurPBSNo++ )
    {
        sPBS = sPBSTbl[sCurPBSNo];
        aLfBMPBodyInfo->mPBSStr[sBitIdx++] = toCharPageType( sPBS );
        aLfBMPBodyInfo->mPBSStr[sBitIdx++] = toCharPageFN( sPBS );
        aLfBMPBodyInfo->mPBSStr[sBitIdx++] = ' ';
    }
}

/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_SEGHDR의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4SegHdr( idvSQL              * /*aStatistics*/,
                                    void                * aHeader,
                                    void                * aDumpObj,
                                    iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );

    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpSegHdr,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpstFT::dumpSegHdr( scSpaceID             aSpaceID,
                            scPageID              aPageID,
                            sdpSegType            /*aSegType*/,
                            void                * aHeader,
                            iduFixedTableMemory * aMemory )
{
    sdpstSegHdr         * sSegHdr;
    sdpstDumpSegHdrInfo   sDumpRow;
    UChar               * sPagePtr;
    UInt                  sState = 0 ;
    sdpstStack            sHWMStack;
    sdpstPosItem          sPosItem;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( sdbBufferMgr::fixPageByPID( NULL /*aStatistics*/,
                                          aSpaceID,
                                          aPageID,
                                          &sPagePtr ) != IDE_SUCCESS );
    sState = 1;

    sSegHdr   = sdpstSH::getHdrPtr( sPagePtr );
    sHWMStack = sSegHdr->mHWM.mStack;

    sDumpRow.mSegPID           = sSegHdr->mSegHdrPID;
    sDumpRow.mSegType          =
        sdcFT::convertSegTypeToChar( sSegHdr->mSegType );
    sDumpRow.mSegState         = sSegHdr->mSegState;

    sDumpRow.mTotExtCnt        = sSegHdr->mTotExtCnt;
    sDumpRow.mTotPageCnt       = sSegHdr->mTotPageCnt;
    sDumpRow.mFmtPageCnt       = sSegHdr->mTotPageCnt;
    sDumpRow.mFreeIndexPageCnt = sSegHdr->mFreeIndexPageCnt;

    sDumpRow.mTotExtDirCnt     = sdpstSH::getTotExtDirCnt( sSegHdr );
    sDumpRow.mTotRtBMPCnt      = sSegHdr->mTotRtBMPCnt;

    /* Lst PID */
    sDumpRow.mLstExtDir        = sdpstSH::getLstExtDir( sSegHdr );
    sDumpRow.mLstRtBMP         = sSegHdr->mLstRtBMP;
    sDumpRow.mLstItBMP         = sSegHdr->mLstItBMP;
    sDumpRow.mLstLfBMP         = sSegHdr->mLstLfBMP;
    sDumpRow.mLstSeqNo         = sSegHdr->mLstSeqNo;

    /* HWM */
    sDumpRow.mHPID             = sSegHdr->mHWM.mWMPID;

    sDumpRow.mHExtDirPID       = sSegHdr->mHWM.mExtDirPID;
    sDumpRow.mHSlotNoInExtDir  = sSegHdr->mHWM.mSlotNoInExtDir;

    sPosItem = sdpstStackMgr::getSeekPos( &sHWMStack, SDPST_VIRTBMP );
    sDumpRow.mHVtBMP           = sPosItem.mNodePID;
    sDumpRow.mHSlotNoInVtBMP   = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sHWMStack, SDPST_RTBMP );
    sDumpRow.mHRtBMP           = sPosItem.mNodePID;
    sDumpRow.mHSlotNoInRtBMP   = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sHWMStack, SDPST_ITBMP );
    sDumpRow.mHItBMP           = sPosItem.mNodePID;
    sDumpRow.mHSlotNoInItBMP   = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sHWMStack, SDPST_LFBMP );
    sDumpRow.mHLfBMP           = sPosItem.mNodePID;
    sDumpRow.mHPBSNoInLfBMP    = sPosItem.mIndex;

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( NULL /* aStatistics */, sPagePtr )
              != IDE_SUCCESS );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *) & sDumpRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage(NULL /*aStatistics*/, sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


//------------------------------------------------------
// D$DISK_TABLE_TMS_SEG_HDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsSegHdrColDesc[]=
{
    {
        (SChar*)"SEGPID",
        offsetof( sdpstDumpSegHdrInfo, mSegPID ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mSegPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpSegHdrInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof( sdpstDumpSegHdrInfo, mSegState ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mSegState ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOT_EXT_CNT",
        offsetof( sdpstDumpSegHdrInfo, mTotExtCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mTotExtCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOT_PAGE_CNT",
        offsetof( sdpstDumpSegHdrInfo, mTotPageCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mTotPageCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FMT_PAGE_CNT",
        offsetof( sdpstDumpSegHdrInfo, mFmtPageCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mFmtPageCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_INDEX_PAGE_CNT",
        offsetof( sdpstDumpSegHdrInfo, mFreeIndexPageCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mFreeIndexPageCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOT_EXTDIR_CNT",
        offsetof( sdpstDumpSegHdrInfo, mTotExtDirCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mTotExtDirCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOT_RTBMP_CNT",
        offsetof( sdpstDumpSegHdrInfo, mTotRtBMPCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mTotRtBMPCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_EXTDIR_PID",
        offsetof( sdpstDumpSegHdrInfo, mLstExtDir ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mLstExtDir ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_RTBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mLstRtBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mLstRtBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_ITBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mLstItBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mLstItBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_LFBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mLstLfBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mLstLfBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_SEQNO",
        offsetof( sdpstDumpSegHdrInfo, mLstSeqNo ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mLstSeqNo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_PID",
        offsetof( sdpstDumpSegHdrInfo, mHPID ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_EXTDIR_PID",
        offsetof( sdpstDumpSegHdrInfo, mHExtDirPID ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHExtDirPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_EXTDESC_SLOTNO",
        offsetof( sdpstDumpSegHdrInfo, mHSlotNoInExtDir ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHSlotNoInExtDir ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_VTBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mHVtBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHVtBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_VTBMP_SLOTNO",
        offsetof( sdpstDumpSegHdrInfo, mHSlotNoInVtBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHSlotNoInVtBMP ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_RTBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mHRtBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHRtBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_RTBMP_SLOTNO",
        offsetof( sdpstDumpSegHdrInfo, mHSlotNoInRtBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHSlotNoInRtBMP ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_ITBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mHItBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHItBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_ITBMP_SLOTNO",
        offsetof( sdpstDumpSegHdrInfo, mHSlotNoInItBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHSlotNoInItBMP ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_LFBMP_PID",
        offsetof( sdpstDumpSegHdrInfo, mHLfBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHLfBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_LFBMP_PBSNO",
        offsetof( sdpstDumpSegHdrInfo, mHPBSNoInLfBMP ),
        IDU_FT_SIZEOF( sdpstDumpSegHdrInfo, mHPBSNoInLfBMP ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_TMS_SEG_HDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsSegHdrTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_SEGHDR",
    sdpstFT::buildRecord4SegHdr,
    gDumpDiskTmsSegHdrColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_BMPSTRUCTURE의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4BMPStructure( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * aDumpObj,
                                          iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpBMPStructure,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpstFT::dumpBMPStructure( scSpaceID             aSpaceID,
                                  scPageID              aPageID,
                                  sdpSegType            aSegType,
                                  void                * aHeader,
                                  iduFixedTableMemory * aMemory )
{
    scPageID                     sRtCurPID;
    scPageID                     sItCurPID;
    scPageID                     sNxtRtBMP;
    UInt                         sState = 0;
    UChar                      * sRtPagePtr;
    UChar                      * sItPagePtr;
    sdpstBMPHdr                * sRtBMPHdr;
    sdpstBMPHdr                * sItBMPHdr;
    sdpstBMPSlot               * sRtBMPSlot;
    SShort                       sRtCurSlotNo;
    SShort                       sItCurSlotNo;
    sdpstDumpBMPStructureInfo    sDumpRow;
    idBool                       sIsLastLimitResult;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sRtCurPID = aPageID;

    while ( sRtCurPID != SD_NULL_PID )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL /*aStatistics*/,
                                              aSpaceID,
                                              sRtCurPID,
                                              &sRtPagePtr ) != IDE_SUCCESS );
        sState = 1;

        sRtBMPHdr = sdpstBMP::getHdrPtr( sRtPagePtr );

        for ( sRtCurSlotNo = 0;
              sRtCurSlotNo < sRtBMPHdr->mSlotCnt;
              sRtCurSlotNo++ )
        {
            sRtBMPSlot = sdpstBMP::getSlot( sRtBMPHdr, sRtCurSlotNo );

            sItCurPID = sRtBMPSlot->mBMP;

            IDE_TEST( sdbBufferMgr::fixPageByPID( NULL /*aStatistics*/,
                                                  aSpaceID,
                                                  sItCurPID,
                                                  &sItPagePtr )
                      != IDE_SUCCESS );
            sState = 2;

            sItBMPHdr = sdpstBMP::getHdrPtr( sItPagePtr );

            for ( sItCurSlotNo = 0;
                  sItCurSlotNo < sItBMPHdr->mSlotCnt;
                  sItCurSlotNo++ )
            {
                /* BUG-42639 Monitoring query */
                if ( aMemory->useExternalMemory() == ID_FALSE )
                {
                    // BUG-26201 : LimitCheck
                    IDE_TEST( smnfCheckLastLimit( aMemory->getContext(),
                                                  &sIsLastLimitResult )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( smiFixedTable::checkLastLimit( aMemory->getContext(),
                                                             &sIsLastLimitResult )
                              != IDE_SUCCESS );
                }
                IDE_TEST_CONT( sIsLastLimitResult == ID_TRUE,
                                SKIP_BUILD_RECORDS );


                makeDumpBMPStructureInfo( sItBMPHdr,
                                          sItCurSlotNo,
                                          sRtCurPID,
                                          aSegType,
                                          &sDumpRow );

                IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                      aMemory,
                                                      (void *)&sDumpRow )
                          != IDE_SUCCESS );
            }

            sState = 1;
            IDE_TEST( sdbBufferMgr::unfixPage( NULL, sItPagePtr )
                      != IDE_SUCCESS );

        }

        sNxtRtBMP = sdpstRtBMP::getNxtRtBMP( sRtBMPHdr );

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( NULL, sRtPagePtr ) != IDE_SUCCESS );

        sRtCurPID = sNxtRtBMP;
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    switch ( sState )
    {
        case 2:
            sState = 1;
            IDE_TEST( sdbBufferMgr::unfixPage( NULL, sItPagePtr )
                      != IDE_SUCCESS );
        case 1:
            sState = 0;
            IDE_TEST( sdbBufferMgr::unfixPage( NULL, sRtPagePtr )
                      != IDE_SUCCESS );
        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, sItPagePtr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, sRtPagePtr )
                        == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_BMPSTRUCTURE Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsBMPStructureColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpBMPStructureInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpBMPStructureInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RTBMP_PID",
        offsetof( sdpstDumpBMPStructureInfo, mRtBMP ),
        IDU_FT_SIZEOF( sdpstDumpBMPStructureInfo, mRtBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ITBMP_PID",
        offsetof( sdpstDumpBMPStructureInfo, mItBMP ),
        IDU_FT_SIZEOF( sdpstDumpBMPStructureInfo, mItBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LFBMP_PID",
        offsetof( sdpstDumpBMPStructureInfo,  mLfBMP ),
        IDU_FT_SIZEOF( sdpstDumpBMPStructureInfo, mLfBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LFBMP_MFNL",
        offsetof( sdpstDumpBMPStructureInfo, mLfBMPMFNLStr ),
        IDU_FT_SIZEOF( sdpstDumpBMPStructureInfo, mLfBMPMFNLStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_TMS_ITBMPBODYSTRUCTURE Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsBMPStructureTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_BMPSTRUCTURE",
    sdpstFT::buildRecord4BMPStructure,
    gDumpDiskTmsBMPStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/**************************************************************************
 * Description: 주어진 page type인 페이지를 스캔하여 주어진 함수를 수행한다.
 **************************************************************************/
IDE_RC sdpstFT::doAction4EachPage( void                    * aHeader,
                                   iduFixedTableMemory     * aMemory,
                                   scSpaceID                 aSpaceID,
                                   scPageID                  aSegPID,
                                   sdpSegType                aSegType,
                                   sdpstFTDumpCallbackFunc   aDumpFunc,
                                   sdpPageType               aPageType )
{
    sdpPageType     sPageTypeArray[1];

    sPageTypeArray[0] = aPageType;

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aSegPID,
                                 aSegType,
                                 aDumpFunc,
                                 sPageTypeArray,
                                 1 ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description: 주어진 page type인 페이지를 스캔하여 주어진 함수를 수행한다.
 **************************************************************************/
IDE_RC sdpstFT::doAction4EachPage( void                    * aHeader,
                                   iduFixedTableMemory     * aMemory,
                                   scSpaceID                 aSpaceID,
                                   scPageID                  aSegPID,
                                   sdpSegType                aSegType,
                                   sdpstFTDumpCallbackFunc   aDumpFunc,
                                   sdpPageType             * aPageTypeArray,
                                   UInt                      aPageTypeArrayCnt )
{
    sdpSegInfo        sSegInfo;
    sdpExtInfo        sExtInfo;
    sdRID             sExtRID;
    scPageID          sPageID = SD_NULL_PID;
    UChar           * sPagePtr;
    sdpPhyPageHdr   * sPhyHdr;
    idBool            sIsLastLimitResult;
    UInt              sLoop;
    UInt              sState = 0;

    IDE_TEST( sdpstSH::getSegInfo( NULL,
                                   aSpaceID,
                                   aSegPID,
                                   NULL, /* aTableHeader */
                                   &sSegInfo ) != IDE_SUCCESS );

    sExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sdpstExtDir::getExtInfo( NULL,
                                       aSpaceID,
                                       sExtRID,
                                       &sExtInfo ) != IDE_SUCCESS );

    IDE_TEST( sdpstExtDir::getNxtPage( NULL,
                                       aSpaceID,
                                       &sSegInfo,
                                       NULL,
                                       &sExtRID,
                                       &sExtInfo,
                                       &sPageID ) != IDE_SUCCESS );

    while ( sPageID != SD_NULL_PID )
    {
        /* BUG-42639 Monitoring query */
        if ( aMemory->useExternalMemory() == ID_FALSE )
        {
            // BUG-26201 : LimitCheck
            IDE_TEST( smnfCheckLastLimit( aMemory->getContext(),
                                          &sIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( smiFixedTable::checkLastLimit( aMemory->getContext(),
                                                     &sIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        IDE_TEST_CONT( sIsLastLimitResult == ID_TRUE, SKIP_BUILD_RECORDS );

        sState = 0;
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL /*aStatistics*/,
                                              aSpaceID,
                                              sPageID,
                                              &sPagePtr ) != IDE_SUCCESS );
        sState = 1;

        sPhyHdr = sdpPhyPage::getHdr( sPagePtr );

        for ( sLoop = 0; sLoop < aPageTypeArrayCnt; sLoop++ )
        {
            if ( sdpPhyPage::getPageType( sPhyHdr ) == aPageTypeArray[sLoop] )
            {
                IDE_TEST( aDumpFunc( aHeader,
                                     aMemory,
                                     aSegType,
                                     sPagePtr,
                                     &sIsLastLimitResult ) != IDE_SUCCESS );
            }
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( NULL, sPagePtr ) != IDE_SUCCESS );

        IDE_TEST( sdpstExtDir::getNxtPage( NULL, /* aStatistics */
                                           aSpaceID,
                                           &sSegInfo,
                                           NULL,
                                           &sExtRID,
                                           &sExtInfo,
                                           &sPageID ) != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, sPagePtr ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_RTBMPHDR의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4RtBMPHdr( idvSQL              * /*aStatistics*/,
                                      void                * aHeader,
                                      void                * aDumpObj,
                                      iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpRtBMPHdr,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpRtBMPHdr( scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdpSegType            aSegType,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory )
{
    sdpPageType     sPageTypeArray[2];

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sPageTypeArray[0] = SDP_PAGE_TMS_RTBMP;
    sPageTypeArray[1] = SDP_PAGE_TMS_SEGHDR;

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpRtBMPHdrPage,
                                 sPageTypeArray,
                                 2 ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpRtBMPHdrPage( void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    sdpSegType            aSegType,
                                    UChar               * aPagePtr,
                                    idBool              * aIsLastLimitResult )
{
    sdpstBMPHdr          * sBMPHdr;
    sdpstDumpBMPHdrInfo    sDumpRow;

    IDE_ASSERT( aPagePtr != NULL );

    *aIsLastLimitResult = ID_FALSE;

    sBMPHdr = sdpstBMP::getHdrPtr( aPagePtr );

    makeDumpBMPHdrInfo( sBMPHdr, aSegType, &sDumpRow );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDumpRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_RTBMPHDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsRtBMPHdrColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpBMPHdrInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof( sdpstDumpBMPHdrInfo, mPageID),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TYPE",
        offsetof( sdpstDumpBMPHdrInfo, mTypeStr),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mTypeStr),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_SLOT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mFreeSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mFreeSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_SLOT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMaxSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMaxSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_FREE_SLOTNO",
        offsetof( sdpstDumpBMPHdrInfo, mFstFreeSlotNo ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mFstFreeSlotNo ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MFNL",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLStr ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_MFNL_FUL_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FUL]  ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FUL] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_MFNL_INS_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_INS]  ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_INS] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"SLOT_MFNL_FMT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FMT]  ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FMT] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_RTBMP_PID",
        offsetof( sdpstDumpBMPHdrInfo, mNxtRtBMP ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mNxtRtBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_TMS_RTBMPHDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsRtBMPHdrTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_RTBMPHDR",
    sdpstFT::buildRecord4RtBMPHdr,
    gDumpDiskTmsRtBMPHdrColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_RTBMPBODY의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4RtBMPBody( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * aDumpObj,
                                       iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpRtBMPBody,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpRtBMPBody( scSpaceID             aSpaceID,
                               scPageID              aPageID,
                               sdpSegType            aSegType,
                               void                * aHeader,
                               iduFixedTableMemory * aMemory )
{
    sdpPageType     sPageTypeArray[2];

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sPageTypeArray[0] = SDP_PAGE_TMS_RTBMP;
    sPageTypeArray[1] = SDP_PAGE_TMS_SEGHDR;

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpRtBMPBodyPage,
                                 sPageTypeArray,
                                 2 ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpRtBMPBodyPage( void                * aHeader,
                                     iduFixedTableMemory * aMemory,
                                     sdpSegType            aSegType,
                                     UChar               * aPagePtr,
                                     idBool              * aIsLastLimitResult )
{
    sdpstBMPHdr           * sBMPHdr;
    SShort                  sSlotNo;
    sdpstDumpBMPBodyInfo    sDumpRow;

    IDE_ASSERT( aPagePtr != NULL );

    sBMPHdr = sdpstBMP::getHdrPtr( aPagePtr );

    for ( sSlotNo = 0; sSlotNo < sBMPHdr->mSlotCnt; sSlotNo++ )
    {
        /* BUG-42639 Monitoring query */
        if ( aMemory->useExternalMemory() == ID_FALSE )
        {
            // BUG-26201 : LimitCheck
            IDE_TEST( smnfCheckLastLimit( aMemory->getContext(),
                                          aIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( smiFixedTable::checkLastLimit( aMemory->getContext(),
                                                     aIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        if ( *aIsLastLimitResult == ID_TRUE )
        {
            break;
        }

        makeDumpBMPBodyInfo( sBMPHdr, sSlotNo, aSegType, &sDumpRow );

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sDumpRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_RTBMPBODY Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsRtBMPBodyColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpBMPBodyInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RTBMP_PID",
        offsetof( sdpstDumpBMPBodyInfo, mPageID ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ITBMP_PID",
        offsetof( sdpstDumpBMPBodyInfo,  mBMP ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MFNL",
        offsetof( sdpstDumpBMPBodyInfo, mMFNLStr ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mMFNLStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


//------------------------------------------------------
// D$DISK_TABLE_TMS_RTBMPBODY Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsRtBMPBodyTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_RTBMPBODY",
    sdpstFT::buildRecord4RtBMPBody,
    gDumpDiskTmsRtBMPBodyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_ITBMPHDR의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4ItBMPHdr( idvSQL              * /*aStatistics*/,
                                      void                * aHeader,
                                      void                * aDumpObj,
                                      iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpItBMPHdr,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpItBMPHdr( scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdpSegType            aSegType,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory )
{
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpItBMPHdrPage,
                                 SDP_PAGE_TMS_ITBMP ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpItBMPHdrPage( void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    sdpSegType            aSegType,
                                    UChar               * aPagePtr,
                                    idBool              * aIsLastLimitResult )
{
    sdpstBMPHdr          * sBMPHdr;
    sdpstDumpBMPHdrInfo    sDumpRow;

    IDE_ASSERT( aPagePtr != NULL );

    *aIsLastLimitResult = ID_FALSE;

    sBMPHdr = sdpstBMP::getHdrPtr( aPagePtr );

    makeDumpBMPHdrInfo( sBMPHdr, aSegType, &sDumpRow );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDumpRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_ITBMPHDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsItBMPHdrColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpBMPHdrInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof( sdpstDumpBMPHdrInfo, mPageID),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TYPE",
        offsetof( sdpstDumpBMPHdrInfo, mTypeStr),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mTypeStr),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_SLOT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mFreeSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mFreeSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_SLOT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMaxSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMaxSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_FREE_SLOTNO",
        offsetof( sdpstDumpBMPHdrInfo, mFstFreeSlotNo ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mFstFreeSlotNo ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MFNL",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLStr ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_MFNL_FUL_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FUL]  ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FUL] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_MFNL_INS_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_INS]  ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_INS] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"SLOT_MFNL_FMT_CNT",
        offsetof( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FMT]  ),
        IDU_FT_SIZEOF( sdpstDumpBMPHdrInfo, mMFNLTbl[SDPST_MFNL_FMT] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


//------------------------------------------------------
// D$DISK_TABLE_TMS_ITBMPHDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsItBMPHdrTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_ITBMPHDR",
    sdpstFT::buildRecord4ItBMPHdr,
    gDumpDiskTmsItBMPHdrColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_ITBMPBODY의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4ItBMPBody( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * aDumpObj,
                                       iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpItBMPBody,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpItBMPBody( scSpaceID             aSpaceID,
                               scPageID              aPageID,
                               sdpSegType            aSegType,
                               void                * aHeader,
                               iduFixedTableMemory * aMemory )
{
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpItBMPBodyPage,
                                 SDP_PAGE_TMS_ITBMP ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpItBMPBodyPage( void                * aHeader,
                                     iduFixedTableMemory * aMemory,
                                     sdpSegType            aSegType,
                                     UChar               * aPagePtr,
                                     idBool              * aIsLastLimitResult )
{
    sdpstBMPHdr           * sBMPHdr;
    SShort                  sSlotNo;
    sdpstDumpBMPBodyInfo    sDumpRow;

    IDE_ASSERT( aPagePtr != NULL );

    sBMPHdr = sdpstBMP::getHdrPtr( aPagePtr );

    for ( sSlotNo = 0; sSlotNo < sBMPHdr->mSlotCnt; sSlotNo++ )
    {
        /* BUG-42639 Monitoring query */
        if ( aMemory->useExternalMemory() == ID_FALSE )
        {
            // BUG-26201 : LimitCheck
            IDE_TEST( smnfCheckLastLimit( aMemory->getContext(),
                                          aIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( smiFixedTable::checkLastLimit( aMemory->getContext(),
                                                     aIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        if ( *aIsLastLimitResult == ID_TRUE )
        {
            break;
        }

        makeDumpBMPBodyInfo( sBMPHdr, sSlotNo, aSegType, &sDumpRow );

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sDumpRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_ITBMPBODY Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsItBMPBodyColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpBMPBodyInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ITBMP_PID",
        offsetof( sdpstDumpBMPBodyInfo, mPageID ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LFBMP_PID",
        offsetof( sdpstDumpBMPBodyInfo,  mBMP ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mBMP ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MFNL",
        offsetof( sdpstDumpBMPBodyInfo, mMFNLStr ),
        IDU_FT_SIZEOF( sdpstDumpBMPBodyInfo, mMFNLStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};



//------------------------------------------------------
// D$DISK_TABLE_TMS_ITBMPBODY Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsItBMPBodyTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_ITBMPBODY",
    sdpstFT::buildRecord4ItBMPBody,
    gDumpDiskTmsItBMPBodyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_LFBMPHDR의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4LfBMPHdr( idvSQL              * /*aStatistics*/,
                                      void                * aHeader,
                                      void                * aDumpObj,
                                      iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpLfBMPHdr,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpLfBMPHdr( scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdpSegType            aSegType,
                              void                * aHeader,
                              iduFixedTableMemory * aMemory )
{
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpLfBMPHdrPage,
                                 SDP_PAGE_TMS_LFBMP ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpLfBMPHdrPage( void                * aHeader,
                                    iduFixedTableMemory * aMemory,
                                    sdpSegType            aSegType,
                                    UChar               * aPagePtr,
                                    idBool              * aIsLastLimitResult )
{
    sdpstLfBMPHdr        * sLfBMPHdr;
    sdpstDumpLfBMPHdrInfo  sDumpRow;

    IDE_ASSERT( aPagePtr != NULL );

    *aIsLastLimitResult = ID_FALSE;

    sLfBMPHdr = sdpstLfBMP::getHdrPtr( aPagePtr );

    makeDumpLfBMPHdrInfo( sLfBMPHdr, aSegType, &sDumpRow );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDumpRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


//------------------------------------------------------
// D$DISK_TABLE_TMS_LFBMPHDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsLfBMPHdrColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mSegType ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mPageID),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TYPE",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mTypeStr),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mTypeStr),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_SLOT_CNT",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mFreeSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mFreeSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_SLOT_CNT",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mMaxSlotCnt ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mMaxSlotCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_FREE_SLOTNO",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mFstFreeSlotNo ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mFstFreeSlotNo ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MFNL",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLStr ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_MFNL_FUL_CNT",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLTbl[SDPST_MFNL_FUL]  ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLTbl[SDPST_MFNL_FUL] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_MFNL_INS_CNT",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLTbl[SDPST_MFNL_INS]  ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLTbl[SDPST_MFNL_INS] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"SLOT_MFNL_FMT_CNT",
        offsetof( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLTbl[SDPST_MFNL_FMT]  ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPHdrInfo, mCommon.mMFNLTbl[SDPST_MFNL_FMT] ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};



//------------------------------------------------------
// D$DISK_TABLE_TMS_LFBMPHDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsLfBMPHdrTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_LFBMPHDR",
    sdpstFT::buildRecord4LfBMPHdr,
    gDumpDiskTmsLfBMPHdrColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_LFBMPRANGESLOT의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4LfBMPRangeSlot( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpLfBMPRangeSlot,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpLfBMPRangeSlot( scSpaceID             aSpaceID,
                                    scPageID              aPageID,
                                    sdpSegType            aSegType,
                                    void                * aHeader,
                                    iduFixedTableMemory * aMemory )
{
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpLfBMPPage4RangeSlot,
                                 SDP_PAGE_TMS_LFBMP ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpLfBMPPage4RangeSlot(
                                     void                * aHeader,
                                     iduFixedTableMemory * aMemory,
                                     sdpSegType            aSegType,
                                     UChar               * aPagePtr,
                                     idBool              * aIsLastLimitResult )
{
    sdpstLfBMPHdr              * sLfBMPHdr;
    sdpstDumpLfBMPRangeSlotInfo  sDumpRow;
    SShort                       sCurSlotNo;

    IDE_ASSERT( aPagePtr != NULL );

    sLfBMPHdr = sdpstLfBMP::getHdrPtr( aPagePtr );

    for ( sCurSlotNo = 0;
          sCurSlotNo < sLfBMPHdr->mBMPHdr.mSlotCnt;
          sCurSlotNo++ )
    {
        /* BUG-42639 Monitoring query */
        if ( aMemory->useExternalMemory() == ID_FALSE )
        {
            // BUG-26201 : LimitCheck
            IDE_TEST( smnfCheckLastLimit( aMemory->getContext(),
                                          aIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( smiFixedTable::checkLastLimit( aMemory->getContext(),
                                                     aIsLastLimitResult )
                      != IDE_SUCCESS );
        }
        if ( *aIsLastLimitResult == ID_TRUE )
        {
            break;
        }

        makeDumpLfBMPRangeSlotInfo( sLfBMPHdr,
                                    sCurSlotNo,
                                    aSegType,
                                    &sDumpRow );

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sDumpRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_LFBMPRANGESLOT Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsLfBMPRangeSlotColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LFBMP_PID",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mPageID ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RANGESLOT_NO",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mRangeSlotNo),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mRangeSlotNo ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_PID",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mFstPID),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mFstPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LENGTH",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mLength),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mLength ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_PBSNO",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mFstPBSNo),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mFstPBSNo ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXTDIR_PID",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mExtDirPID),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mExtDirPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXTDIR_SLOTNO",
        offsetof( sdpstDumpLfBMPRangeSlotInfo, mSlotNoInExtDir),
        IDU_FT_SIZEOF( sdpstDumpLfBMPRangeSlotInfo, mSlotNoInExtDir ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_TMS_LFBMPRANGESLOT Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsLfBMPRangeSlotTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_LFBMPRANGESLOT",
    sdpstFT::buildRecord4LfBMPRangeSlot,
    gDumpDiskTmsLfBMPRangeSlotColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_LFBMPPBSTBL의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4LfBMPPBSTbl( idvSQL              * /*aStatistics*/,
                                         void                * aHeader,
                                         void                * aDumpObj,
                                         iduFixedTableMemory * aMemory )
{
    void        * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpLfBMPPBSTbl,
                                       aHeader,
                                       aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdpstFT::dumpLfBMPPBSTbl( scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 sdpSegType            aSegType,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory )
{
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( doAction4EachPage( aHeader,
                                 aMemory,
                                 aSpaceID,
                                 aPageID,
                                 aSegType,
                                 doDumpLfBMPPage4PBSTbl,
                                 SDP_PAGE_TMS_LFBMP ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC sdpstFT::doDumpLfBMPPage4PBSTbl(
                                     void                * aHeader,
                                     iduFixedTableMemory * aMemory,
                                     sdpSegType            aSegType,
                                     UChar               * aPagePtr,
                                     idBool              * aIsLastLimitResult )
{
    sdpstLfBMPHdr         * sLfBMPHdr;
    sdpstDumpLfBMPPBSTblInfo  sDumpRow;

    IDE_ASSERT( aPagePtr != NULL );
    *aIsLastLimitResult = ID_FALSE;

    sLfBMPHdr = sdpstLfBMP::getHdrPtr( aPagePtr );

    makeDumpLfBMPPBSTblInfo( sLfBMPHdr, aSegType, &sDumpRow );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDumpRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_TMS_LFBMPBODY Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsLfBMPPBSTblColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpLfBMPPBSTblInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPPBSTblInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LFBMP_PID",
        offsetof( sdpstDumpLfBMPPBSTblInfo, mPageID ),
        IDU_FT_SIZEOF( sdpstDumpLfBMPPBSTblInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PBS_TABLE",
        offsetof( sdpstDumpLfBMPPBSTblInfo, mPBSStr ),
        SDPST_MAX_BITSET_BUFF_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_TMS_LFBMPBODY Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsLfBMPPBSTblTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_LFBMPPBSTBL",
    sdpstFT::buildRecord4LfBMPPBSTbl,
    gDumpDiskTmsLfBMPPBSTblColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*****************************************************************************
 * Description: D$DISK_TABLE_TMS_SEGCACHE의 Record를 만드는 함수이다.
 *
 * aHeader  - [IN] FixedTable의 헤더
 * aDumpObj - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory  - [IN] FixedTable의 레코드를 저장할 메모리
 *****************************************************************************/
IDE_RC sdpstFT::buildRecord4SegCache( idvSQL              * /*aStatistics*/,
                                      void                * aHeader,
                                      void                * aDumpObj,
                                      iduFixedTableMemory * aMemory )
{
    smcTableHeader          * sTable;
    sdpSegHandle            * sSegHandle;
    sdpstSegCache           * sSegCache;
    sdpstDumpSegCacheInfo     sDumpRow;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );

    /* Table헤더를 가져와 Segment Cache을 얻어온다. */
    sTable = (smcTableHeader*)((smpSlotHeader*)aDumpObj + 1);

    sSegHandle = sdpSegDescMgr::getSegHandle( &sTable->mFixed.mDRDB );
    IDE_TEST_RAISE( sSegHandle == NULL, ERR_INVALID_DUMP_OBJECT );

    sSegCache  = (sdpstSegCache*)sSegHandle->mCache;
    IDE_TEST_RAISE( sSegCache == NULL, ERR_INVALID_DUMP_OBJECT );

    sDumpRow.mSegType        = sdcFT::convertSegTypeToChar(sSegCache->mSegType);
    sDumpRow.mSegPID         = sSegHandle->mSegPID;
    sDumpRow.mTmpSegHeadPID  = sSegCache->mCommon.mTmpSegHead;
    sDumpRow.mTmpSegTailPID  = sSegCache->mCommon.mTmpSegTail;
    sDumpRow.mSize           = sSegCache->mCommon.mSegSizeByBytes;
    sDumpRow.mPageCntInExt   = sSegCache->mCommon.mPageCntInExt;
    sDumpRow.mMetaPID        = sSegCache->mCommon.mMetaPID;
    sDumpRow.mTableOID       = sSegCache->mCommon.mTableOID;
    sDumpRow.mFmtPageCnt     = sSegCache->mFmtPageCnt;

    sDumpRow.mUseLstAllocPageHint = sSegCache->mUseLstAllocPageHint == ID_TRUE ?
                                    'T' : 'F';
    sDumpRow.mLstAllocPID         = sSegCache->mLstAllocPID;
    sDumpRow.mLstAllocSeqNo       = sSegCache->mLstAllocSeqNo;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDumpRow )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



//------------------------------------------------------
// D$DISK_TABLE_TMS_SEG_HDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTmsSegCacheColDesc[]=
{
    {
        (SChar*)"SEGPID",
        offsetof( sdpstDumpSegCacheInfo, mSegPID ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mSegPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpstDumpSegCacheInfo, mSegType ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TEMP_SEG_HEAD_PID",
        offsetof( sdpstDumpSegCacheInfo, mTmpSegHeadPID ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mTmpSegHeadPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TEMP_SEG_TAIL_PID",
        offsetof( sdpstDumpSegCacheInfo, mTmpSegTailPID ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mTmpSegTailPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SIZE",
        offsetof( sdpstDumpSegCacheInfo, mSize ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mSize ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_OID",
        offsetof( sdpstDumpSegCacheInfo, mTableOID ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mTableOID ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FORMAT_PAGE_COUNT",
        offsetof( sdpstDumpSegCacheInfo, mFmtPageCnt ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mFmtPageCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USE_LAST_ALLOC_PAGE_HINT",
        offsetof( sdpstDumpSegCacheInfo, mUseLstAllocPageHint),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mUseLstAllocPageHint),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_ALLOC_PID",
        offsetof( sdpstDumpSegCacheInfo, mLstAllocPID),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mLstAllocPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_ALLOC_SEQNO",
        offsetof( sdpstDumpSegCacheInfo, mLstAllocSeqNo ),
        IDU_FT_SIZEOF( sdpstDumpSegCacheInfo, mLstAllocSeqNo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_TMS_SEG_HDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableTmsSegCacheTblDesc =
{
    (SChar *)"D$DISK_TABLE_TMS_SEGCACHE",
    sdpstFT::buildRecord4SegCache,
    gDumpDiskTmsSegCacheColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

