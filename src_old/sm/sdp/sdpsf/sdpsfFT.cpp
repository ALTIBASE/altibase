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
# include <smErrorCode.h>
# include <sdr.h>
# include <sdp.h>
# include <sdpReq.h>
# include <sdpsfDef.h>
# include <sdpsfExtDirPageList.h>
# include <sdpsfExtDirPage.h>
# include <sdpsfExtMgr.h>
# include <sdcFT.h>
# include <sdpsfFT.h>
# include <sdpsfSH.h>
# include <smiFixedTable.h>

/* TASK-4007 [SM]PBT를 위한 기능 추가 
 * 편리한 dumpSegHdr를 위해, Table이름을 인자로 넣으면
 * 해당 테이블 관련 모든 Segment를 Dump함 */

/***********************************************************************
 * Description : D$DISK_TABLE_FMS_SEGHDR의 Record를 만드는 함수이다.
 *
 * aHeader   - [IN] FixedTable의 헤더
 * aDumpObj  - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory   - [IN] FixedTable의 레코드를 저장할 메모리
 *
 ***********************************************************************/
IDE_RC sdpsfFT::buildRecord4SegHdr( idvSQL              * /*aStatistics*/,
                                    void                * aHeader,
                                    void                * aDumpObj,
                                    iduFixedTableMemory * aMemory )
{
    void                     * sTable;

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

IDE_RC sdpsfFT::dumpSegHdr( scSpaceID             aSpaceID,
                            scPageID              aPageID,
                            sdpSegType            /*aSegType*/,
                            void                * aHeader,
                            iduFixedTableMemory * aMemory )
{
    sdpsfSegHdr     *sSegHdr;
    sdpsfDumpSegHdr  sDumpRow;
    UInt             sState = 0;
    
    IDE_TEST_RAISE( sdpTableSpace::getSegMgmtType( aSpaceID ) !=
                    SMI_SEGMENT_MGMT_FREELIST_TYPE, ERR_INVALIDE_SEG_TYPE );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Read( NULL, /* idvSQL */
                                             aSpaceID,
                                             aPageID,
                                             &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    sDumpRow.mSegHdrPID      = aPageID;
    sDumpRow.mSegType        = 
        sdcFT::convertSegTypeToChar( (sdpSegType)sSegHdr->mType );
    sDumpRow.mState          = sSegHdr->mState;
    sDumpRow.mPageCntInExt   = sSegHdr->mPageCntInExt;
    sDumpRow.mPvtFreePIDList = sSegHdr->mPvtFreePIDList;
    sDumpRow.mUFmtPIDList    = sSegHdr->mUFmtPIDList;
    sDumpRow.mFreePIDList    = sSegHdr->mFreePIDList;
    sDumpRow.mFmtPageCnt     = sSegHdr->mFmtPageCnt;
    sDumpRow.mHWMPID         = sSegHdr->mHWMPID;
    sDumpRow.mAllocExtRID    = sSegHdr->mAllocExtRID;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDumpRow )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                      sSegHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALIDE_SEG_TYPE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                                sSegHdr )
                        == IDE_SUCCESS );

        default:
            break;
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_FMS_SEGHDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpSegHdrColDescOfFMS[]=
{
    {
        (SChar*)"SEGHDRPID",
        offsetof( sdpsfDumpSegHdr, mSegHdrPID ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mSegHdrPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpsfDumpSegHdr, mSegType ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof( sdpsfDumpSegHdr, mState ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mState ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXTSIZE",
        offsetof( sdpsfDumpSegHdr, mPageCntInExt ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mPageCntInExt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PVTLIST_HEAD_PID",
        offsetof( sdpsfDumpSegHdr, mPvtFreePIDList ) + offsetof( sdpSglPIDListBase, mHead ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PVTLIST_TAIL_PID",
        offsetof( sdpsfDumpSegHdr, mPvtFreePIDList ) + offsetof( sdpSglPIDListBase, mTail ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mTail ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PVTLIST_PID_CNT",
        offsetof( sdpsfDumpSegHdr, mPvtFreePIDList ) + offsetof( sdpSglPIDListBase, mNodeCnt ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mNodeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UFMTLIST_HEAD_PID",
        offsetof( sdpsfDumpSegHdr, mUFmtPIDList ) + offsetof( sdpSglPIDListBase, mHead ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UFMTLIST_TAIL_PID",
        offsetof( sdpsfDumpSegHdr, mUFmtPIDList ) + offsetof( sdpSglPIDListBase, mTail ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mTail ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UFMTLIST_PID_CNT",
        offsetof( sdpsfDumpSegHdr, mUFmtPIDList ) + offsetof( sdpSglPIDListBase, mNodeCnt ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mNodeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREELIST_HEAD_PID",
        offsetof( sdpsfDumpSegHdr, mFreePIDList ) + offsetof( sdpSglPIDListBase, mHead ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREELIST_TAIL_PID",
        offsetof( sdpsfDumpSegHdr, mFreePIDList ) + offsetof( sdpSglPIDListBase, mTail ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mTail ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREELIST_PID_CNT",
        offsetof( sdpsfDumpSegHdr, mFreePIDList ) + offsetof( sdpSglPIDListBase, mNodeCnt ) ,
        IDU_FT_SIZEOF( sdpSglPIDListBase, mNodeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ALLOC_EXT_RID",
        offsetof( sdpsfDumpSegHdr, mAllocExtRID ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mAllocExtRID ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FMT_PAGE_CNT",
        offsetof( sdpsfDumpSegHdr, mFmtPageCnt ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mFmtPageCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HWM_PID",
        offsetof( sdpsfDumpSegHdr, mHWMPID ),
        IDU_FT_SIZEOF( sdpsfDumpSegHdr, mHWMPID ),
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
// D$DISK_TABLE_FMS_SEGHDR Dump Table의 Column Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableFmsSegHdrTblDesc =
{
    (SChar *)"D$DISK_TABLE_FMS_SEGHDR",
    sdpsfFT::buildRecord4SegHdr,
    gDumpSegHdrColDescOfFMS,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



/***********************************************************************
 * Description : D$DISK_TABLE_FMS_FREEPIDLIST의 Record를 만드는 함수이다.
 *
 * aHeader   - [IN] FixedTable의 헤더
 * aDumpObj  - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory   - [IN] FixedTable의 레코드를 저장할 메모리
 *
 ***********************************************************************/
IDE_RC sdpsfFT::buildRecord4FreePIDList(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * aDumpObj,
    iduFixedTableMemory * aMemory )
{
    void                     * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpFreePIDList,
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

IDE_RC sdpsfFT::dumpFreePIDList( scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 sdpSegType            /*aSegType*/,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory )
{
    sdpsfPageInfoOfFreeLst     sDumpPageInfo;
    scSpaceID                  sSpaceID;
    scPageID                   sPageID;
    sdpsfSegHdr               *sSegHdr;
    sdpSglPIDListBase         *sFreePIDList;
    ULong                      sTotalPageCnt;
    ULong                      sPageCnt;
    sdpPhyPageHdr             *sPhyPageHdr;
    UChar                     *sSlotDirPtr;
    sdpSglPIDListNode         *sPIDLstNode;
    SInt                       sState = 0;
    idBool                     sIsLastLimitResult;
    
    IDE_TEST_RAISE( sdpTableSpace::getSegMgmtType( aSpaceID ) !=
                    SMI_SEGMENT_MGMT_FREELIST_TYPE, ERR_INVALIDE_SEG_TYPE );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Read( NULL, /* idvSQL */
                                             aSpaceID,
                                             aPageID,
                                             &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    sSpaceID        = aSpaceID;
    sFreePIDList    = sdpsfSH::getFreePIDList( sSegHdr );
    sTotalPageCnt   = sdpSglPIDList::getNodeCnt( sFreePIDList );
    sPageID         = sdpSglPIDList::getHeadOfList( sFreePIDList );

    sDumpPageInfo.mSegType        = 
        sdcFT::convertSegTypeToChar( (sdpSegType)sSegHdr->mType );

    sPageCnt = 0;

    while( sPageID != SD_NULL_PID )
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

        sPageCnt++;

        sDumpPageInfo.mPID  = sPageID;

        IDE_TEST( sdbBufferMgr::getPageByPID( NULL, /* idvSQL */
                                              sSpaceID,
                                              sPageID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* aMtx */
                                              (UChar**)&sPhyPageHdr,
                                              NULL  /* aTrySuccess*/,
                                              NULL  /* IsCorruptPage*/ )
                  != IDE_SUCCESS );
        sState = 2;

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPhyPageHdr);

        sDumpPageInfo.mRecCnt   = sdpSlotDirectory::getCount(sSlotDirPtr);
        sDumpPageInfo.mPageType = sdpPhyPage::getPageType( sPhyPageHdr );
        sDumpPageInfo.mState    = sdpPhyPage::getState( sPhyPageHdr );
        sDumpPageInfo.mFreeSize = sdpPhyPage::getTotalFreeSize( sPhyPageHdr );

        sPIDLstNode = sdpPhyPage::getSglPIDListNode( sPhyPageHdr );
        sPageID     = sdpSglPIDList::getNxtOfNode( sPIDLstNode );


        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             (UChar*)sPhyPageHdr )
                  != IDE_SUCCESS );

        sState = 1;
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *) & sDumpPageInfo )
                  != IDE_SUCCESS );
    }

    /* BUG-31386  [sm-disk-page] [FMS] ASSERT statement in Fixed table code 
     *            did not consider the LIMIT operation. */
    if( sPageCnt != sTotalPageCnt )
    {
        ideLog::log( IDE_SM_0,
                     "ERROR! Invalid page count :"
                     "%u <-> %u",
                     sPageCnt,
                     sTotalPageCnt );
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    sState = 0;
    IDE_TEST( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                      sSegHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALIDE_SEG_TYPE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   (UChar*)sPhyPageHdr )
                        == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                                sSegHdr )
                        == IDE_SUCCESS );

        default:
            break;
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_FMS_FREELIST Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpFreeLstColDescOfFMS[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpsfPageInfoOfFreeLst, mSegType ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfFreeLst, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PID",
        offsetof( sdpsfPageInfoOfFreeLst, mPID ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfFreeLst, mPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RECORD_COUNT",
        offsetof( sdpsfPageInfoOfFreeLst, mRecCnt ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfFreeLst, mRecCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_SIZE",
        offsetof( sdpsfPageInfoOfFreeLst, mFreeSize ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfFreeLst, mFreeSize ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_TYPE",
        offsetof( sdpsfPageInfoOfFreeLst, mPageType ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfFreeLst, mPageType ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof( sdpsfPageInfoOfFreeLst, mState ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfFreeLst, mState ),
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
// D$DISK_TABLE_FMS_FREEPIDLIST Dump Table의 Column Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableFmsFreeLstTblDesc =
{
    (SChar *)"D$DISK_TABLE_FMS_FREEPIDLIST",
    sdpsfFT::buildRecord4FreePIDList,
    gDumpFreeLstColDescOfFMS,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/***********************************************************************
 * Description : D$DISK_TABLE_FMS_PVTPIDLIST의 Record를 만드는 함수이다.
 *
 * aHeader   - [IN] FixedTable의 헤더
 * aDumpObj  - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory   - [IN] FixedTable의 레코드를 저장할 메모리
 *
 ***********************************************************************/
IDE_RC sdpsfFT::buildRecord4PvtPIDList(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * aDumpObj,
    iduFixedTableMemory * aMemory )
{
    void                     * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpPvtPIDList,
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

IDE_RC sdpsfFT::dumpPvtPIDList( scSpaceID             aSpaceID,
                                scPageID              aPageID,
                                sdpSegType            /*aSegType*/,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory )
{
    sdpsfPageInfoOfPvtFreeLst  sDumpPageInfo;
    scSpaceID                  sSpaceID;
    scPageID                   sPageID;
    sdpsfSegHdr               *sSegHdr;
    sdpSglPIDListBase         *sPvtFreePIDList;
    ULong                      sTotalPageCnt;
    ULong                      sPageCnt;
    sdpPhyPageHdr             *sPhyPageHdr;
    sdpSglPIDListNode         *sPIDLstNode;
    SInt                       sState = 0;
    idBool                     sIsLastLimitResult;
    
    IDE_TEST_RAISE( sdpTableSpace::getSegMgmtType( aSpaceID ) !=
                    SMI_SEGMENT_MGMT_FREELIST_TYPE, ERR_INVALIDE_SEG_TYPE );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Read( NULL, /* idvSQL */
                                             aSpaceID,
                                             aPageID,
                                             &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    sSpaceID        = aSpaceID;
    sPvtFreePIDList = sdpsfSH::getPvtFreePIDList( sSegHdr );
    sTotalPageCnt   = sdpSglPIDList::getNodeCnt( sPvtFreePIDList );
    sPageID         = sdpSglPIDList::getHeadOfList( sPvtFreePIDList );

    sDumpPageInfo.mSegType = 
        sdcFT::convertSegTypeToChar( (sdpSegType)sSegHdr->mType );

    sPageCnt = 0;

    while( sPageID != SD_NULL_PID )
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

        sPageCnt++;

        sDumpPageInfo.mPID  = sPageID;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *) & sDumpPageInfo )
                  != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID( NULL, /* idvSQL */
                                              sSpaceID,
                                              sPageID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* aMtx */
                                              (UChar**)&sPhyPageHdr,
                                              NULL  /* aTrySuccess*/,
                                              NULL  /* IsCorruptPage*/ )
                  != IDE_SUCCESS );
        sState = 2;

        sPIDLstNode = sdpPhyPage::getSglPIDListNode( sPhyPageHdr );
        sPageID     = sdpSglPIDList::getNxtOfNode( sPIDLstNode );

        sState = 1;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             (UChar*)sPhyPageHdr )
                  != IDE_SUCCESS );
    }

    /* BUG-31386  [sm-disk-page] [FMS] ASSERT sentences in Fixed table code 
     *            did not consider the LIMIT operation. */
    if( sPageCnt != sTotalPageCnt )
    {
        ideLog::log( IDE_SM_0,
                     "ERROR! Invalid page count :"
                     "%u <-> %u",
                     sPageCnt,
                     sTotalPageCnt );
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    sState = 0;
    IDE_TEST( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                      sSegHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALIDE_SEG_TYPE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   (UChar*)sPhyPageHdr )
                        == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                                sSegHdr )
                        == IDE_SUCCESS );

        default:
            break;
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_FMS_PVTPIDLIST Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpPvtFreeLstColDescOfFMS[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpsfPageInfoOfPvtFreeLst, mSegType ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfPvtFreeLst, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PID",
        offsetof( sdpsfPageInfoOfPvtFreeLst, mPID ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfPvtFreeLst, mPID ),
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
// D$DISK_TABLE_FMS_PVTPIDLIST Dump Table의 Column Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableFmsPvtFreeLstTblDesc =
{
    (SChar *)"D$DISK_TABLE_FMS_PVTPIDLIST",
    sdpsfFT::buildRecord4PvtPIDList,
    gDumpPvtFreeLstColDescOfFMS,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



/***********************************************************************
 * Description : D$DISK_FMS_SEG_UFMT_PIDLIST의 Record를 만드는 함수이다.
 *
 * aHeader   - [IN] FixedTable의 헤더
 * aDumpObj  - [IN] Dump할 대상 객체, smcTableHeader.
 * aMemory   - [IN] FixedTable의 레코드를 저장할 메모리
 *
 ***********************************************************************/
IDE_RC sdpsfFT::buildRecord4UFmtPIDList(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * aDumpObj,
    iduFixedTableMemory * aMemory )
{
    void                     * sTable;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );
    
    sTable = aDumpObj;

    IDE_TEST( sdcFT::doAction4EachSeg( sTable,
                                       dumpUfmtPIDList,
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

IDE_RC sdpsfFT::dumpUfmtPIDList( scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 sdpSegType            /*aSegType*/,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory )
{
    sdpsfPageInfoOfPvtFreeLst  sDumpPageInfo;
    scSpaceID                  sSpaceID;
    scPageID                   sPageID;
    sdpsfSegHdr               *sSegHdr;
    sdpSglPIDListBase         *sUFmtPIDList;
    ULong                      sTotalPageCnt;
    ULong                      sPageCnt;
    sdpPhyPageHdr             *sPhyPageHdr;
    sdpSglPIDListNode         *sPIDLstNode;
    SInt                       sState = 0;
    idBool                     sIsLastLimitResult;

    IDE_TEST_RAISE( sdpTableSpace::getSegMgmtType( aSpaceID ) !=
                    SMI_SEGMENT_MGMT_FREELIST_TYPE, ERR_INVALIDE_SEG_TYPE );

    IDE_TEST( sdpsfSH::fixAndGetSegHdr4Read( NULL, /* idvSQL */
                                             aSpaceID,
                                             aPageID,
                                             &sSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    sSpaceID        = aSpaceID;
    sUFmtPIDList    = sdpsfSH::getUFmtPIDList( sSegHdr );
    sTotalPageCnt   = sdpSglPIDList::getNodeCnt( sUFmtPIDList );
    sPageID         = sdpSglPIDList::getHeadOfList( sUFmtPIDList );

    sDumpPageInfo.mSegType = 
        sdcFT::convertSegTypeToChar( (sdpSegType)sSegHdr->mType );

    sPageCnt = 0;

    while( sPageID != SD_NULL_PID )
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

        sPageCnt++;

        sDumpPageInfo.mPID  = sPageID;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *) & sDumpPageInfo )
                  != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID( NULL, /* idvSQL */
                                              sSpaceID,
                                              sPageID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* aMtx */
                                              (UChar**)&sPhyPageHdr,
                                              NULL  /* aTrySuccess*/,
                                              NULL  /* IsCorruptPage*/ )
                  != IDE_SUCCESS );
        sState = 2;

        sPIDLstNode = sdpPhyPage::getSglPIDListNode( sPhyPageHdr );
        sPageID     = sdpSglPIDList::getNxtOfNode( sPIDLstNode );

        sState = 1;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             (UChar*)sPhyPageHdr )
                  != IDE_SUCCESS );
    }

    /* BUG-31386  [sm-disk-page] [FMS] ASSERT sentences in Fixed table code 
     *            did not consider the LIMIT operation. */
    if( sPageCnt != sTotalPageCnt )
    {
        ideLog::log( IDE_SM_0,
                     "ERROR! Invalid page count :"
                     "%u <-> %u",
                     sPageCnt,
                     sTotalPageCnt );
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    sState = 0;
    IDE_TEST( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                      sSegHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALIDE_SEG_TYPE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   (UChar*)sPhyPageHdr )
                        == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( sdpsfSH::releaseSegHdr( NULL, /* idvSQL */
                                                sSegHdr )
                        == IDE_SUCCESS );

        default:
            break;
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_FMS_UFMTPIDLIST Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpUFmtFreeLstColDescOfFMS[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpsfPageInfoOfUFmtFreeLst, mSegType ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfUFmtFreeLst, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PID",
        offsetof( sdpsfPageInfoOfUFmtFreeLst, mPID ),
        IDU_FT_SIZEOF( sdpsfPageInfoOfPvtFreeLst, mPID ),
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
// D$DISK_TABLE_FMS_UFMTPIDLIST Dump Table의 Column Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableFmsUFmtFreeLstTblDesc =
{
    (SChar *)"D$DISK_TABLE_FMS_UFMTPIDLIST",
    sdpsfFT::buildRecord4UFmtPIDList,
    gDumpUFmtFreeLstColDescOfFMS,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

