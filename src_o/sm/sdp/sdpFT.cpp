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
 * $Id: sdpFT.cpp 19860 2007-02-07 02:09:39Z kimmkeun $
 *
 * Description
 *
 *   Disk DB의 Page를 binary로 Dump하거나 PhyPageHdr를 Dump한다.
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>
#include <smpManager.h>
#include <smcDef.h>
#include <sdpFT.h>
#include <smiFixedTable.h>
#include <sdp.h>
#include <sdc.h>
#include <sdn.h>
#include <sdbMPRMgr.h>
#include <sdpReq.h>

static iduFixedTableColDesc gDumpDiskDBPageColDesc[] =
{
    {
        (SChar*)"TBSID",
        IDU_FT_OFFSETOF(sdpDiskDBPageDump, mTBSID),
        IDU_FT_SIZEOF(sdpDiskDBPageDump, mTBSID),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PID",
        IDU_FT_OFFSETOF(sdpDiskDBPageDump, mPID),
        IDU_FT_SIZEOF(sdpDiskDBPageDump, mPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP",
        IDU_FT_OFFSETOF(sdpDiskDBPageDump, mPageDump),
        IDU_FT_SIZEOF(sdpDiskDBPageDump, mPageDump),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};


// D$DISK_DB_PAGE
// ReadPage( Fix + latch )한 후 ptrToDiskPageDump을 이용하여 Dump한다.
IDE_RC sdpFT::buildRecordDiskDBPageDump(idvSQL              * /*aStatistics*/,
                                        void                *aHeader,
                                        void                *aDumpObj,
                                        iduFixedTableMemory *aMemory)
{
    scGRID                * sGRID = NULL;
    scSpaceID               sSpaceID;
    scPageID                sPageID;
    UChar                 * sPage;
    sdpDiskDBPageDump       sDiskDBPageDump;
    idBool                  sSuccess;
    UInt                    sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sGRID      = (scGRID*) aDumpObj;
    sSpaceID   = SC_MAKE_SPACE(*sGRID);
    sPageID    = SC_MAKE_PID(*sGRID);

    // FixPage를 시도한다
    IDE_TEST( sdbBufferMgr::fixPageByPID( NULL, // idvSQL
                                          sSpaceID,
                                          sPageID,
                                          &sPage )
              != IDE_SUCCESS );
    IDE_TEST( sPage == NULL ); 
    sState = 1;

    sdbBufferMgr::latchPage( NULL, // idvSQL
                             (UChar*)sPage,
                             SDB_S_LATCH,
                             SDB_WAIT_NORMAL,
                             &sSuccess );
    IDE_ASSERT( sSuccess == ID_TRUE );
    sState = 2;

    sDiskDBPageDump.mTBSID = sSpaceID;
    sDiskDBPageDump.mPID   = sPageID;
    ideLog::ideMemToHexStr( sPage,
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY, 
                            sDiskDBPageDump.mPageDump,
                            SMP_DUMP_COLUMN_SIZE );

    sState = 1;
    sdbBufferMgr::unlatchPage( (UChar*)sPage );

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( NULL, // idvSQL
                                       (UChar*)sPage )
              != IDE_SUCCESS );

    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                         aMemory,
                                         (void *)&sDiskDBPageDump)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 2:
        (void) sdbBufferMgr::unlatchPage( (UChar*)sPage );
    case 1:
        IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, // idvSQL
                                             (UChar*)sPage )
                    == IDE_SUCCESS );
    default:
        break;
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpDiskDBPageTableDesc =
{
    (SChar *)"D$DISK_DB_PAGE",
    sdpFT::buildRecordDiskDBPageDump,
    gDumpDiskDBPageColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


// D$DISK_DB_PHYPAGEHDR
static iduFixedTableColDesc gDumpDiskDBPhyPageHdrColDesc[]=
{
    {
        (SChar*)"CHECK_SUM",
        offsetof(sdpDiskDBPhyPageHdrDump, mCheckSum ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mCheckSum ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_LSN_FILE_NO",
        offsetof(sdpDiskDBPhyPageHdrDump, mPageLSNFILENo ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mPageLSNFILENo ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_LSN_OFFSET",
        offsetof(sdpDiskDBPhyPageHdrDump, mPageLSNOffset ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mPageLSNOffset ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_SMO_NO",
        offsetof(sdpDiskDBPhyPageHdrDump, mIndexSMONo ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mIndexSMONo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SPACE_ID",
        offsetof(sdpDiskDBPhyPageHdrDump, mSpaceID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mSpaceID ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_FREE_SIZE",
        offsetof(sdpDiskDBPhyPageHdrDump, mTotalFreeSize ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mTotalFreeSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"AVAILABLE_FREE_SIZE",
        offsetof(sdpDiskDBPhyPageHdrDump, mAvailableFreeSize ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mAvailableFreeSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOGICAL_HEADER_SIZE",
        offsetof(sdpDiskDBPhyPageHdrDump, mLogicalHdrSize ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mLogicalHdrSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SIZE_OF_CTL",
        offsetof(sdpDiskDBPhyPageHdrDump, mSizeOfCTL ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mSizeOfCTL ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_SPACE_BEGIN_OFFSET",
        offsetof(sdpDiskDBPhyPageHdrDump, mFreeSpaceBeginOffset ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mFreeSpaceBeginOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_SPACE_END_OFFSET",
        offsetof(sdpDiskDBPhyPageHdrDump, mFreeSpaceEndOffset ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mFreeSpaceEndOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_TYPE",
        offsetof(sdpDiskDBPhyPageHdrDump, mPageType ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mPageType ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_STATE",
        offsetof(sdpDiskDBPhyPageHdrDump, mPageState ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mPageState ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGEID",
        offsetof(sdpDiskDBPhyPageHdrDump, mPageID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(sdpDiskDBPhyPageHdrDump, mIsConsistent ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LINK_STATE",
        offsetof(sdpDiskDBPhyPageHdrDump, mLinkState ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mLinkState ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PID",
        offsetof(sdpDiskDBPhyPageHdrDump, mParentPID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mParentPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IDX_IN_PARENT",
        offsetof(sdpDiskDBPhyPageHdrDump, mIdxInParent ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mIdxInParent ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREV_PID",
        offsetof(sdpDiskDBPhyPageHdrDump, mPrevPID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mPrevPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PID",
        offsetof(sdpDiskDBPhyPageHdrDump, mNextPID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mNextPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_OID",
        offsetof(sdpDiskDBPhyPageHdrDump, mTableOID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mTableOID ),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(sdpDiskDBPhyPageHdrDump, mIndexID ),
        IDU_FT_SIZEOF(sdpDiskDBPhyPageHdrDump, mIndexID ),
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

// D$DISK_DB_PHYPAGEHDR
// PhysiacalPageHdr를 Dump한다.
IDE_RC sdpFT::buildRecordDiskDBPhyPageHdrDump(idvSQL              * /*aStatistics*/,
                                              void                *aHeader,
                                              void                *aDumpObj,
                                              iduFixedTableMemory *aMemory)
{
    scGRID                   * sGRID = NULL;
    sdpDiskDBPhyPageHdrDump    sPhyPageHdrDump;
    UInt                       sState = 0;
    scSpaceID                  sSpaceID;
    scPageID                   sPageID;
    sdpPhyPageHdr            * sPhyPageHdr;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sGRID      = (scGRID*) aDumpObj;
    sSpaceID   = SC_MAKE_SPACE(*sGRID);
    sPageID    = SC_MAKE_PID(*sGRID);

    //DISK_TABLESPACE가 맞는지 검사한다.
    IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace( sSpaceID ) == ID_TRUE );

    IDE_TEST( sdbBufferMgr::getPageByPID( NULL, /* idvSQL */
                                          sSpaceID,
                                          sPageID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* aMtx */
                                          (UChar**)&sPhyPageHdr,
                                          NULL /*aTrySuccess*/,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );
    sState = 1;

    sPhyPageHdrDump.mCheckSum             = sPhyPageHdr->mFrameHdr.mCheckSum;
    sPhyPageHdrDump.mPageLSNFILENo        = sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo;
    sPhyPageHdrDump.mPageLSNOffset        = sPhyPageHdr->mFrameHdr.mPageLSN.mOffset;
    sPhyPageHdrDump.mIndexSMONo           = sPhyPageHdr->mFrameHdr.mIndexSMONo;
    sPhyPageHdrDump.mSpaceID              = sPhyPageHdr->mFrameHdr.mSpaceID;
    sPhyPageHdrDump.mTotalFreeSize        = sPhyPageHdr->mTotalFreeSize;
    sPhyPageHdrDump.mAvailableFreeSize    = sPhyPageHdr->mAvailableFreeSize;
    sPhyPageHdrDump.mLogicalHdrSize       = sPhyPageHdr->mLogicalHdrSize;
    sPhyPageHdrDump.mSizeOfCTL            = sPhyPageHdr->mSizeOfCTL;
    sPhyPageHdrDump.mFreeSpaceBeginOffset = sPhyPageHdr->mFreeSpaceBeginOffset;
    sPhyPageHdrDump.mFreeSpaceEndOffset   = sPhyPageHdr->mFreeSpaceEndOffset;
    sPhyPageHdrDump.mPageType             = sPhyPageHdr->mPageType;
    sPhyPageHdrDump.mPageState            = sPhyPageHdr->mPageState;
    sPhyPageHdrDump.mPageID               = sPhyPageHdr->mPageID;
    sPhyPageHdrDump.mIsConsistent         = 
        (sPhyPageHdr->mIsConsistent == ID_TRUE) ? 'T' : 'F';
    sPhyPageHdrDump.mLinkState            = sPhyPageHdr->mLinkState;
    sPhyPageHdrDump.mParentPID            = sPhyPageHdr->mParentInfo.mParentPID;
    sPhyPageHdrDump.mIdxInParent          = sPhyPageHdr->mParentInfo.mIdxInParent;
    sPhyPageHdrDump.mPrevPID = (UInt)sdpDblPIDList::getPrvOfNode( 
                              sdpPhyPage::getDblPIDListNode( sPhyPageHdr ) );
    sPhyPageHdrDump.mNextPID = (UInt)sdpDblPIDList::getNxtOfNode( 
                              sdpPhyPage::getDblPIDListNode( sPhyPageHdr ) );
    sPhyPageHdrDump.mTableOID = sPhyPageHdr->mTableOID;
    sPhyPageHdrDump.mIndexID  = sPhyPageHdr->mIndexID;

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                         (UChar*)sPhyPageHdr )
              != IDE_SUCCESS );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *) &sPhyPageHdrDump )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EMPTY_OBJECT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                         (UChar*)sPhyPageHdr ) ;
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpDiskDBPhyPageHdrTableDesc =
{
    (SChar *)"D$DISK_DB_PHYPAGEHDR",
    sdpFT::buildRecordDiskDBPhyPageHdrDump,
    gDumpDiskDBPhyPageHdrColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gSegmentColDesc[]=
{

    {
        (SChar*)"SPACE_ID",
        offsetof(sdpSegmentPerfV, mSpaceID),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mSpaceID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar*)"TABLE_OID",
        offsetof(sdpSegmentPerfV, mTableOID),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mTableOID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar*)"OBJECT_ID",
        offsetof(sdpSegmentPerfV, mObjectID),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mObjectID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar*)"SEGMENT_PID",
        offsetof(sdpSegmentPerfV, mSegPID),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mSegPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar*)"SEGMENT_TYPE",
        offsetof(sdpSegmentPerfV, mSegmentType),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mSegmentType),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar*)"SEGMENT_STATE",
        offsetof(sdpSegmentPerfV, mState),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mState),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },


    {
        (SChar*)"TOTAL_EXTENT_COUNT",
        offsetof(sdpSegmentPerfV, mExtentTotalCnt),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mExtentTotalCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },

    /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다.
     * CASE-28174 에서 요구된 기능으로, TMSC에서 사용할 컬럼이다.
     * 원래는 X$가 제공되면 안되지만, 긴급한 요청으로
     * TOTAL_USED_SIZE을 제공한다.
     * 해당 컬럼이 삭제되거나 변경된다면 TMSC에 해당 사항을 확인하도록 한다. */
    {
        (SChar*)"TOTAL_USED_SIZE",
        offsetof(sdpSegmentPerfV, mUsedTotalSize),
        IDU_FT_SIZEOF(sdpSegmentPerfV, mUsedTotalSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
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

//======================================================================
//  X$SEGEMNT
//  index, table segment 의 segment desc info를 보여주는 peformance view
//======================================================================

IDE_RC sdpFT::buildRecordForSegment(idvSQL              * /*aStatistics*/,
                                    void                * aHeader,
                                    void                * /* aDumpObj */,
                                    iduFixedTableMemory * aMemory)
{
    smcTableHeader   * sCatTblHdr;
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sPtr;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    UInt               i;
    UInt               sIndexCnt;
    smnIndexHeader   * sIndexHeaderCursor;
    sdpSegmentPerfV    sSegmentInfo;
    void             * sTrans;
    UInt               sLobCnt;
    sdpSegCCache     * sSegCache;
    sdpSegHandle     * sSegHandle;
    smiColumn        * sColumn;
    void             * sISavepoint = NULL;
    UInt               sDummy = 0;
    void             * sIndexValues[3];
    UInt               sState = 0;

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr    = NULL;

    if ( aMemory->useExternalMemory() == ID_TRUE )
    {
        /* BUG-43006 FixedTable Indexing Filter */
        sTrans = ((smiFixedTableProperties *)aMemory->getContext())->mTrans;
    }
    else
    {
        /* BUG-32292 [sm-util] Self deadlock occur since fixed-table building
         * operation uses another transaction. 
         * NestedTransaction을 사용하면 Self-deadlock 우려가 있다.
         * 따라서 id Memory 영역으로부터 Iterator를 얻어 Transaction을 얻어낸다. */
        sTrans = ((smiIterator*)aMemory->getContext())->trans;
    }

    while(1)
    {
        IDE_TEST(smcRecord::nextOIDall(sCatTblHdr,sCurPtr,&sNxtPtr)
                  != IDE_SUCCESS);
        if( sNxtPtr == NULL )
        {
            break;
        }//if sNxtPtr
        sPtr = (smpSlotHeader *)sNxtPtr;
        // To fix BUG-14681
        if( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) == ID_TRUE )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }
        sTableHeader = (smcTableHeader *)( sPtr + 1 );

        if( smcTable::isDropedTable(sTableHeader) == ID_TRUE )
        {
            sCurPtr = sNxtPtr;
            continue;
        }//if

        if( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_TRUE )
        {
            //DDL 을 방지.
            sState = 1;
            IDE_TEST( smLayerCallback::setImpSavepoint( sTrans, 
                                                        &sISavepoint,
                                                        sDummy )
                      != IDE_SUCCESS );

            /* BUG-48160 lock 잡힌 table 은 제외하고 출력하는 기능 추가.
             * Property : __SKIP_LOCKED_TABLE_AT_FIXED_TABLE */
            if ( smLayerCallback::lockTableModeIS4FixedTable( sTrans,
                                                              SMC_TABLE_LOCK( sTableHeader ) )
                 == IDE_SUCCESS )
            {
                sState = 2;
            
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                if(smcTable::isDropedTable(sTableHeader) == ID_TRUE)
                {
                    sState = 1;
                    IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                    sISavepoint )
                              != IDE_SUCCESS );
                    sState = 0;
                    IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                                  sISavepoint )
                              != IDE_SUCCESS );

                    sCurPtr = sNxtPtr;
                    continue;
                }//if

                // build table segment record

                // BUG-30867 Discard 된 Tablespace에 속한 Table은 Skip되어야 함.
                // 단,다른 Tablespace에 속한 Index는 출력 되어야 함
                if( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                                SCT_SS_SKIP_BUILD_FIXED_TABLE ) == ID_FALSE )
                {
                    sSegCache  = sdpSegDescMgr::getSegCCache(
                                                       &(sTableHeader->mFixed.mDRDB) );

                    /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
                    IDE_TEST( getSegmentInfo(
                                  sTableHeader->mSpaceID,
                                  sdpSegDescMgr::getSegPID( &(sTableHeader->mFixed.mDRDB) ),
                                  sTableHeader->mSelfOID,
                                  sSegCache,
                                  &sSegmentInfo )
                              != IDE_SUCCESS );

                    /* BUG-43006 FixedTable Indexing Filter
                     * Column Index 를 사용해서 전체 Record를 생성하지않고
                     * 부분만 생성해 Filtering 한다.
                     * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
                     * 해당하는 값을 순서대로 넣어주어야 한다.
                     * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
                     * 어 주어야한다.
                     */
                    sIndexValues[0] = &sSegmentInfo.mSpaceID;
                    sIndexValues[1] = &sSegmentInfo.mSegmentType;
                    sIndexValues[2] = &sSegmentInfo.mState;
                    if ( iduFixedTable::checkKeyRange( aMemory,
                                                       gSegmentColDesc,
                                                       sIndexValues )
                         == ID_FALSE )
                    {
                        IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                        sISavepoint )
                                  != IDE_SUCCESS );
                        IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                                      sISavepoint )
                                  != IDE_SUCCESS );

                        sCurPtr = sNxtPtr;
                        continue;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                        aMemory,
                                                        (void *)&sSegmentInfo)
                             != IDE_SUCCESS);
                }

                // build index segment record
                sIndexCnt =  smcTable::getIndexCount(sTableHeader);

                if(sIndexCnt != 0)
                {
                    for( i = 0 ; i < sIndexCnt; i++)
                    {
                        sIndexHeaderCursor = (smnIndexHeader*) smcTable::getTableIndex(sTableHeader,i);

                        // BUG-30867 Discard 된 Tablespace에 속한 Index는 볼 수 없음
                        if( sctTableSpaceMgr::hasState(SC_MAKE_SPACE(sIndexHeaderCursor->mIndexSegDesc),
                                                       SCT_SS_SKIP_BUILD_FIXED_TABLE ) == ID_TRUE )
                        {
                            continue;
                        }

                        sSegHandle = sdpSegDescMgr::getSegHandle(
                            &((sdnRuntimeHeader*)sIndexHeaderCursor->mHeader)->mSegmentDesc );
                        sSegCache  = (sdpSegCCache*)sSegHandle->mCache;

                        //----------------------------
                        // To Fix BUG-14155
                        // Extent 총개수 만을 관리하였으나,
                        // Full Extent 개수와 Used Extent 개수를 분리하여
                        // 관리하도록 변경
                        //----------------------------
                        // BUGBUG:위에거 해야 하나?

                        /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
                        IDE_TEST( getSegmentInfo(
                                      SC_MAKE_SPACE(sIndexHeaderCursor->mIndexSegDesc),
                                      SC_MAKE_PID(sIndexHeaderCursor->mIndexSegDesc),
                                      sTableHeader->mSelfOID,
                                      sSegCache,
                                      &sSegmentInfo )
                                  != IDE_SUCCESS );

                        /* BUG-43006 FixedTable Indexing Filter
                         * Column Index 를 사용해서 전체 Record를 생성하지않고
                         * 부분만 생성해 Filtering 한다.
                         * Column Desc 순서대로 IndexValues에 추가해야한다.
                         */
                        sIndexValues[0] = &sSegmentInfo.mSpaceID;
                        sIndexValues[1] = &sSegmentInfo.mSegmentType;
                        sIndexValues[2] = &sSegmentInfo.mState;
                        if ( iduFixedTable::checkKeyRange( aMemory,
                                                           gSegmentColDesc,
                                                           sIndexValues )
                             == ID_FALSE )
                        {
                            continue;
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                            aMemory,
                                                            (void *)&sSegmentInfo)
                                 != IDE_SUCCESS);
                    }//for
                }//sIndexCnt

                // build lob segment record
                sLobCnt = sTableHeader->mLobColumnCount;

                if( sLobCnt > 0 )
                {
                    for( i = 0; i < sTableHeader->mColumnCount; i++ )
                    {
                        sColumn = (smiColumn*)smcTable::getColumn( sTableHeader, i );
                        if( (sColumn->flag & SMI_COLUMN_TYPE_MASK)
                            != SMI_COLUMN_TYPE_LOB )
                        {
                            continue;
                        }

                        // BUG-30867 Discard 된 Tablespace에 속한 lob은 볼 수 없음
                        if( sctTableSpaceMgr::hasState( sColumn->colSpace,
                                                        SCT_SS_SKIP_BUILD_FIXED_TABLE ) == ID_TRUE )
                        {
                            continue;
                        }

                        sSegHandle = sdpSegDescMgr::getSegHandle( sColumn );
                        sSegCache  = (sdpSegCCache*)sSegHandle->mCache;

                        /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
                        IDE_TEST( getSegmentInfo(
                                      sColumn->colSpace,
                                      SC_MAKE_PID(sColumn->colSeg),
                                      sTableHeader->mSelfOID,
                                      sSegCache,
                                      &sSegmentInfo )
                                  != IDE_SUCCESS );

                        /* BUG-43006 FixedTable Indexing Filter
                         * Column Index 를 사용해서 전체 Record를 생성하지않고
                         * 부분만 생성해 Filtering 한다.
                         * Column Desc 순서대로 IndexValues에 추가해야한다.
                         */
                        sIndexValues[0] = &sSegmentInfo.mSpaceID;
                        sIndexValues[1] = &sSegmentInfo.mSegmentType;
                        sIndexValues[2] = &sSegmentInfo.mState;
                        if ( iduFixedTable::checkKeyRange( aMemory,
                                                           gSegmentColDesc,
                                                           sIndexValues )
                             == ID_FALSE )
                        {
                            continue;
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                            aMemory,
                                                            (void *)&sSegmentInfo)
                                 != IDE_SUCCESS);
                    } // for
                } // sLobCnt

                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, sISavepoint )
                          != IDE_SUCCESS );
            }

            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, sISavepoint )
                      != IDE_SUCCESS );
        }//if   sTableType == SMI_TABLE_DISK

        sCurPtr = sNxtPtr;
    }// while

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( smLayerCallback::abortToImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smLayerCallback::unsetImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        default:
            break;
    }
    return IDE_FAILURE;
}

/* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
IDE_RC sdpFT::getSegmentInfo( UInt               aSpaceID,
                              scPageID           aSegPID,
                              ULong              aTableOID,
                              sdpSegCCache     * aSegCache,
                              sdpSegmentPerfV  * aSegmentInfo )
{
    sdpSegInfo        sSegInfo;
    sdpSegMgmtOp    * sSegMgmtOP;

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( aSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );
    
    aSegmentInfo->mSpaceID  = aSpaceID;
    aSegmentInfo->mTableOID = aTableOID;
    aSegmentInfo->mObjectID = aSegCache->mTableOID;
    aSegmentInfo->mSegPID   = aSegPID;

    IDE_TEST( sSegMgmtOP->mGetSegInfo( NULL, /* Statistics */
                                       aSegmentInfo->mSpaceID,
                                       aSegmentInfo->mSegPID,
                                       NULL, /* aTableHeader */
                                       &sSegInfo )
              != IDE_SUCCESS );

    aSegmentInfo->mSegmentType    = sSegInfo.mType;
    aSegmentInfo->mState          = sSegInfo.mState;
    aSegmentInfo->mExtentTotalCnt = sSegInfo.mExtCnt;
                
    // 세그먼트 실사용양을 구한다.
    aSegmentInfo->mUsedTotalSize  = 
                (sSegInfo.mFmtPageCnt * SD_PAGE_SIZE) -
                aSegCache->mFreeSegSizeByBytes;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc  gSegmentDesc=
{
    (SChar *)"X$SEGMENT",
    sdpFT::buildRecordForSegment,
    gSegmentColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

