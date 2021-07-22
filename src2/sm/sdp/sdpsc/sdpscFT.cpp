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
 * $Id: sdpscFT.cpp 89495 2020-12-14 05:19:22Z emlee $
 *
 * 본 파일은 Circular-List Managed Segment에 대한 Fixed Table의 구현파일이다.
 *
 **********************************************************************/

# include <idl.h>

# include <smErrorCode.h>
# include <smi.h>
# include <sdb.h>
# include <sdpPhyPage.h>

# include <sdpscDef.h>
# include <sdpscFT.h>
# include <sdpscSH.h>
# include <sdpscED.h>
# include <sdpReq.h>
# include <sdcTXSegMgr.h>
# include <smx.h>



/* TASK-4007 [SM]PBT를 위한 기능 추가
 * SegType을 영어 단문자로 표현
 * U = Undo Segment
 * T = TSS Segment
 * */
SChar sdpscFT::convertSegTypeToChar( sdpSegType aSegType )
{
    SChar sRet = '?';

    switch( aSegType )
    {
    case SDP_SEG_TYPE_TSS:
        sRet = 'T';
        break;
    case SDP_SEG_TYPE_UNDO:
        sRet = 'U';
        break;
    default:
        break;
    }

    return sRet;
}

SChar sdpscFT::convertToChar( idBool aIsTrue )
{
    SChar sRet = '?';

    switch ( aIsTrue )
    {
    case 1:
        sRet = 'Y';
        break;
    case 0:
        sRet = 'N';
        break;
    default:
        break;
    }

    return sRet;
}

IDE_RC sdpscFT::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC sdpscFT::destroy()
{
    return IDE_SUCCESS;
}

//------------------------------------------------------
// X$DISK_UNDO_SEGHDR Dump Table의 레코드 Build
//------------------------------------------------------
IDE_RC sdpscFT::buildRecord4SegHdr(
                            idvSQL              * /*aStatistics*/,
                            void                * aHeader,
                            void                * /*aDumpObj*/,
                            iduFixedTableMemory * aMemory )
{
    UInt              sSegEntryID;
    UInt              sTotEntryCnt;
    sdcTXSegEntry   * sEntry;
    scPageID          sSegPID;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    /* BUG-30567 Users need the function that check the amount of 
     * usable undo tablespace. 
     * UndoSegemnt ID를 입력해야 했던 D$DISK_UNDO_SEGHDR를
     * X$DISK_UNDO_SEGHDR로 변경하여, Undo tablespace에 대한 정보를
     * 보기 편하도록 수정합니다.*/
    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();
    for( sSegEntryID = 0; sSegEntryID < sTotEntryCnt; sSegEntryID++ )
    {
        sEntry = sdcTXSegMgr::getEntryByIdx( sSegEntryID );

        // TS Segment
        sSegPID = sdcTXSegMgr::getTSSegPtr(sEntry)->getSegPID();
        IDE_TEST( dumpSegHdr( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                              sSegPID,
                              SDP_SEG_TYPE_TSS,
                              sEntry,
                              NULL,
                              aHeader,
                              aMemory )
                  != IDE_SUCCESS );

        // Undo Segment
        sSegPID = sdcTXSegMgr::getUDSegPtr(sEntry)->getSegPID();
        IDE_TEST( dumpSegHdr( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                              sSegPID,
                              SDP_SEG_TYPE_UNDO,
                              sEntry,
                              NULL,
                              aHeader,
                              aMemory )
                  != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
        
// 세그먼트 헤더 Dump
IDE_RC sdpscFT::dumpSegHdr( scSpaceID             aSpaceID,
                            scPageID              aPageID,
                            sdpSegType            aSegType,
                            sdcTXSegEntry       * aEntry,
                            sdpscDumpSegHdrInfo * aDumpSegHdr,
                            void                * aHeader,
                            iduFixedTableMemory * aMemory )
{
    sdpscDumpSegHdrInfo   sDumpSegHdr;
    sdpscDumpSegHdrInfo * sDumpRow = NULL;
    UChar               * sPagePtr = NULL;
    idBool                sDummy;
    sdpscSegMetaHdr     * sSegHdr;
    smSCN                 sSysMinDskViewSCN;
    scPageID              sPageID;
    sdpscExtDirCntlHdr  * sExtDirCntlHdr = NULL;
    UInt                  sState = 0;
    UInt                  sExtState = 0;

    if ( aDumpSegHdr != NULL )
    {
        sDumpRow = aDumpSegHdr;
    }
    else
    {
        sDumpRow = &sDumpSegHdr;
    }
    IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                     aSpaceID,
                                     aPageID,
                                     SDB_S_LATCH,
                                     SDB_WAIT_NORMAL,
                                     SDB_SINGLE_PAGE_READ,
                                     (UChar**)&sPagePtr,
                                     &sDummy) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST_RAISE( sdpPhyPage::getPhyPageType( sPagePtr )
                    != SDP_PAGE_CMS_SEGHDR,
                    ERR_INVALID_DUMP_OBJECT );

    sSegHdr = sdpscSegHdr::getHdrPtr( sPagePtr );

    IDE_TEST_RAISE( sdpscSegHdr::getSegCntlHdr( sSegHdr )->mSegState
                    != SDP_SEG_USE,
                    ERR_INVALID_DUMP_OBJECT );

    sDumpRow->mSegType       =
        convertSegTypeToChar( (sdpSegType)sSegHdr->mSegCntlHdr.mSegType );
    sDumpRow->mSegPID        = aPageID;
    sDumpRow->mSegState      = sSegHdr->mSegCntlHdr.mSegState;
    sDumpRow->mTotExtCnt     = sSegHdr->mExtCntlHdr.mTotExtCnt;
    sDumpRow->mLstExtDir     = sSegHdr->mExtCntlHdr.mLstExtDir;
    sDumpRow->mTotExtDirCnt  = sSegHdr->mExtCntlHdr.mTotExtDirCnt;
    sDumpRow->mPageCntInExt  = sSegHdr->mExtCntlHdr.mPageCntInExt;
    sDumpRow->mExtCntInPage  = sSegHdr->mExtDirCntlHdr.mExtCnt;
    sDumpRow->mNxtExtDir     = sSegHdr->mExtDirCntlHdr.mNxtExtDir;
    sDumpRow->mMaxExtCnt     = sSegHdr->mExtDirCntlHdr.mMaxExtCnt;
    sDumpRow->mMapOffset     = sSegHdr->mExtDirCntlHdr.mMapOffset;
    sDumpRow->mLstCommitSCN  = sSegHdr->mExtDirCntlHdr.mLstCommitSCN;
    sDumpRow->mFstDskViewSCN = sSegHdr->mExtDirCntlHdr.mFstDskViewSCN;
    
    sDumpRow->mUnExpiredExtCnt = 0;
    sDumpRow->mExpiredExtCnt   = 0;
    sDumpRow->mTxExtCnt        = 0;
    sDumpRow->mUnStealExtCnt   = 0;

    if ( aSegType == SDP_SEG_TYPE_TSS )
    {
        /* TSS Seg 는 전부 TxExtCnt 로 계산 */
        sDumpRow->mTxExtCnt += sSegHdr->mExtDirCntlHdr.mExtCnt;  /* TSS SegHdr */
    }
    else 
    {
        IDE_ERROR( aSegType == SDP_SEG_TYPE_UNDO );
        /* Undo SegHdr 는 steal 할수 없음 */
        sDumpRow->mUnStealExtCnt += sSegHdr->mExtDirCntlHdr.mExtCnt; /* Undo SegHdr */
    }

    SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

    sDumpRow->mSysMinViewSCN = sSysMinDskViewSCN;

    sPageID = sSegHdr->mExtDirCntlHdr.mNxtExtDir;

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL /* aStatistics */,
                                         sPagePtr )
              != IDE_SUCCESS );
    sDumpRow->mIsOnline = convertToChar( (idBool)aEntry->mStatus );
   
    /* SegHdr 하나만 있는 경우 */
    IDE_TEST_CONT( sPageID == sDumpRow->mSegPID, SKIP_BUILD_RECORDS );

    /* next를 full scan 하자 */
    while ( sPageID != SD_NULL_PID )
    {
        IDE_DASSERT( sPageID != sDumpRow->mSegPID );

        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Read( NULL, /* aStatistics */
                                                      NULL, /* sdrMtx * */
                                                      aSpaceID,
                                                      sPageID,
                                                      &sExtDirCntlHdr ) 
                  != IDE_SUCCESS );
        sExtState = 1;

        if ( aSegType == SDP_SEG_TYPE_TSS )
        {
            /* TSS Seg 는 전부 TxExtCnt 로 계산 */
            sDumpRow->mTxExtCnt += sExtDirCntlHdr->mExtCnt; 
        }
        else
        {
            IDE_ERROR( aSegType == SDP_SEG_TYPE_UNDO );

            /* 1.지금 보고 있는 ExtDir 은 스틸 불가 */
            if ( sPageID == sdcTXSegMgr::getUDSegPtr(aEntry)->getCurAllocExtDir() )
            {
                sDumpRow->mUnStealExtCnt += sExtDirCntlHdr->mExtCnt;
            }
            else
            {
                if ( SM_SCN_IS_GE(&sExtDirCntlHdr->mLstCommitSCN, &sSysMinDskViewSCN) )
                {
                    sDumpRow->mUnExpiredExtCnt += sExtDirCntlHdr->mExtCnt;
                }
                else
                {
                    sDumpRow->mExpiredExtCnt += sExtDirCntlHdr->mExtCnt;
                }
            }
        }

        sPageID = sExtDirCntlHdr->mNxtExtDir;

        sExtState = 0;
        IDE_TEST( sdpscExtDir::releaseCntlHdr( NULL /* aStatistics */, 
                                               sExtDirCntlHdr )
                  != IDE_SUCCESS );

        if ( sPageID == sDumpRow->mSegPID )
        {
            break;
        }
        else
        {
            /* nothing to do */
        }

    } // end of while

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    if ( ( aHeader != NULL ) && ( aMemory != NULL ) )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)sDumpRow )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL /* aStatistics */,
                                               sPagePtr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    if ( sExtState != 0 )
    {
        IDE_ASSERT( sdpscExtDir::releaseCntlHdr( NULL /* aStatistics */,
                                                 sExtDirCntlHdr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// X$DISK_UNDO_SEGHDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskCmsSegHdrColDesc[]=
{
    {
        (SChar*)"PID",
        offsetof( sdpscDumpSegHdrInfo, mSegPID ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mSegPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpscDumpSegHdrInfo, mSegType ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mSegType ),
        IDU_FT_TYPE_CHAR, /* BUG-28729 D$DISK_UNDO_SEGHDR의 SEGTYPE의 데이터
                           * 타입이 잘못되어 있습니다. */
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEG_STATE",
        offsetof( sdpscDumpSegHdrInfo, mSegState ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mSegState ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TX_EXT_CNT",
        offsetof( sdpscDumpSegHdrInfo, mTxExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mTxExtCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_ONLINE",
        offsetof( sdpscDumpSegHdrInfo, mIsOnline ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mIsOnline ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNSTEAL_EXT_CNT",
        offsetof( sdpscDumpSegHdrInfo, mUnStealExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mUnStealExtCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXPIRED_EXT_CNT",
        offsetof( sdpscDumpSegHdrInfo, mExpiredExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mUnExpiredExtCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNEXPIRED_EXT_CNT",
        offsetof( sdpscDumpSegHdrInfo, mUnExpiredExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mUnExpiredExtCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOT_EXT_CNT",
        offsetof( sdpscDumpSegHdrInfo, mTotExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mTotExtCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LST_EXT_PID",
        offsetof( sdpscDumpSegHdrInfo, mLstExtDir ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mLstExtDir ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOT_EXTDIR_CNT",
        offsetof( sdpscDumpSegHdrInfo, mTotExtDirCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mTotExtDirCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_CNT_IN_EXT",
        offsetof( sdpscDumpSegHdrInfo, mPageCntInExt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mPageCntInExt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXT_CNT_IN_PAGE",
        offsetof( sdpscDumpSegHdrInfo, mExtCntInPage ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mExtCntInPage ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NXT_EXTDIR_PID",
        offsetof( sdpscDumpSegHdrInfo, mNxtExtDir ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mNxtExtDir ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_EXT_CNT",
        offsetof( sdpscDumpSegHdrInfo, mMaxExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mMaxExtCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAP_OFFSET",
        offsetof( sdpscDumpSegHdrInfo, mMapOffset ),
        IDU_FT_SIZEOF( sdpscDumpSegHdrInfo, mMapOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LST_COMMIT_SCN",
        offsetof( sdpscDumpSegHdrInfo, mLstCommitSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_COMMIT_SCN",
        offsetof( sdpscDumpSegHdrInfo, mFstDskViewSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SYS_MIN_DISK_VIEWSCN",
        offsetof( sdpscDumpSegHdrInfo, mSysMinViewSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
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
// X$DISK_UNDO_SEGHDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskCmsSegHdrTableDesc =
{
    (SChar *)"X$DISK_UNDO_SEGHDR",
    sdpscFT::buildRecord4SegHdr,
    gDumpDiskCmsSegHdrColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

// ExtentDirectory헤더 Dump
IDE_RC sdpscFT::dumpExtDirHdr( scSpaceID             aSpaceID,
                               scPageID              aPageID,
                               sdpSegType            /*aSegType*/,
                               void                * aHeader,
                               iduFixedTableMemory * aMemory )
{
    scSpaceID               sSpaceID;
    scPageID                sSegPID;
    scPageID                sPageID;
    sdpscExtDirCntlHdr     *sCntlHdr = NULL;
    sdpscDumpExtDirCntlHdr  sDumpRow;
    UInt                    sPageType;
    UInt                    sGetSegHdr  = 0;
    UInt                    sGetCntlHdr = 0;
    idBool                  sIsLastLimitResult;
    sdpscSegMetaHdr        *sSegHdr;
    UChar                  *sPagePtr = NULL;
    idBool                  sDummy;
    /* BUG-31055 Can not reuse undo pages immediately after it is used
     * to aborted transaction. */
    UInt                    sCheckCnt;

 
    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sSpaceID   = aSpaceID;
    sPageID    = aPageID;
    sSegPID    = sPageID;

    IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                     aSpaceID,
                                     aPageID,
                                     SDB_S_LATCH,
                                     SDB_WAIT_NORMAL,
                                     SDB_SINGLE_PAGE_READ,
                                     (UChar**)&sPagePtr,
                                     &sDummy) 
              != IDE_SUCCESS );
    sGetSegHdr = 1;

    IDE_TEST_RAISE( sdpPhyPage::getPhyPageType( sPagePtr )
                    != SDP_PAGE_CMS_SEGHDR,
                    ERR_INVALID_DUMP_OBJECT );

    sSegHdr = sdpscSegHdr::getHdrPtr( sPagePtr );

    sDumpRow.mSegType =
        convertSegTypeToChar( (sdpSegType)sSegHdr->mSegCntlHdr.mSegType );

    sGetSegHdr = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL /* aStatistics */,
                                         sPagePtr )
              != IDE_SUCCESS );


    sCheckCnt = 0;

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

        IDE_TEST( sdpscExtDir::fixAndGetCntlHdr4Read( 
                               NULL, /* aStatistics */
                               NULL, /* sdrMtx * */
                               sSpaceID,
                               sPageID,
                               &sCntlHdr ) != IDE_SUCCESS );
        sGetCntlHdr = 1;

        sPageType = sdpPhyPage::getPhyPageType(
                                         sdpPhyPage::getPageStartPtr(sCntlHdr));

        IDE_TEST_RAISE( !((sPageType == SDP_PAGE_CMS_SEGHDR) ||
                          (sPageType == SDP_PAGE_CMS_EXTDIR)),
                        ERR_INVALID_DUMP_OBJECT );
	
        sDumpRow.mSegSpaceID    = sSpaceID;
        sDumpRow.mSegPageID     = sSegPID;
        sDumpRow.mMyPageID      = sPageID;
        sDumpRow.mExtCnt        = sCntlHdr->mExtCnt;
        sDumpRow.mNxtExtDir     = sCntlHdr->mNxtExtDir;
        sDumpRow.mMaxExtCnt     = sCntlHdr->mMaxExtCnt;
        sDumpRow.mMapOffset     = sCntlHdr->mMapOffset;
        sDumpRow.mLstCommitSCN  = sCntlHdr->mLstCommitSCN;
        sDumpRow.mFstDskViewSCN = sCntlHdr->mFstDskViewSCN;

        // BUG-28567  [SM] D$DISK_UNDO_EXTDIRHDR에서 잘못된 DUMP 정보를 출력하고
        // 있습니다.
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sDumpRow )
                  != IDE_SUCCESS );

        sPageID = sCntlHdr->mNxtExtDir;

        sGetCntlHdr = 0;
        IDE_TEST( sdpscExtDir::releaseCntlHdr( NULL /* aStatistics */, 
                                               sCntlHdr )
                  != IDE_SUCCESS );

        if ( sPageID == sSegPID )
        {
            break;
        }

        sCheckCnt++;
        /* UndoED의 최소 크기는 Page 8개 이다.
         * (SYS_UNDO_TBS_EXTENT_SIZE의 실질적인 최소값이 65536이다)
         * 따라서 그 이상 루프를 돌고 있다는 것은 무한루프이다.
         * 이 경우 UndoTBS가 꼬여있을 것이기 때문에 사망시킨다. */
        if( sCheckCnt >= ( ID_UINT_MAX / 8 ) )
        {
            ideLog::log( IDE_SERVER_0,
                         "mSegType       =%u\n"
                         "mSegSpaceID    =%u\n"
                         "mSegPageID     =%u\n"
                         "mMyPageID      =%u\n"
                         "mExtCnt        =%u\n"
                         "mNxtExtDir     =%u\n"
                         "mMaxExtCnt     =%u\n"
                         "mMapOffset     =%u\n"
                         "sCheckCnt      =%u\n",
                         sDumpRow.mSegType,
                         sDumpRow.mSegSpaceID,
                         sDumpRow.mSegPageID,
                         sDumpRow.mMyPageID,
                         sDumpRow.mExtCnt,
                         sDumpRow.mNxtExtDir,
                         sDumpRow.mMaxExtCnt,
                         sDumpRow.mMapOffset,
                         sCheckCnt );

            IDE_ASSERT( 0 );
        }
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    if( sGetCntlHdr != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( sdpscExtDir::releaseCntlHdr( NULL, sCntlHdr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    if( sGetSegHdr != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL /* aStatistics */,
                                               sPagePtr )
                    == IDE_SUCCESS );
        IDE_POP();
    }


    return IDE_FAILURE;
}

//------------------------------------------------------
// X$DISK_UNDO_EXTDIRHDR Dump Table의 레코드 Build
//------------------------------------------------------
IDE_RC sdpscFT::buildRecord4ExtDirHdr(
                                idvSQL              * /*aStatistics*/,
                                void                * aHeader,
                                void                * /*aDumpObj*/,
                                iduFixedTableMemory * aMemory )
{
    UInt              sSegEntryID;
    UInt              sTotEntryCnt;
    sdcTXSegEntry   * sEntry;
    scPageID          sSegPID;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    /* BUG-30567 Users need the function that check the amount of 
     * usable undo tablespace. 
     * UndoSegemnt ID를 입력해야 했던 D$DISK_UNDO_DIRHDR를
     * X$DISK_UNDO_DIRHDR로 변경하여, Undo tablespace에 대한 정보를
     * 보기 편하도록 수정합니다.*/
    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();
    for( sSegEntryID = 0; sSegEntryID < sTotEntryCnt; sSegEntryID++ )
    {
        sEntry = sdcTXSegMgr::getEntryByIdx( sSegEntryID );

        // TS Segment
        sSegPID = sdcTXSegMgr::getTSSegPtr(sEntry)->getSegPID();
        IDE_TEST( dumpExtDirHdr( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                 sSegPID,
                                 SDP_SEG_TYPE_TSS,
                                 aHeader,
                                 aMemory )
                  != IDE_SUCCESS );

        // Undo Segment
        sSegPID = sdcTXSegMgr::getUDSegPtr(sEntry)->getSegPID();
        IDE_TEST( dumpExtDirHdr( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                 sSegPID,
                                 SDP_SEG_TYPE_UNDO,
                                 aHeader,
                                 aMemory )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//------------------------------------------------------
// X$DISK_UNDO_EXTDIRHDR Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskCmsExtDirHdrColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdpscDumpExtDirCntlHdr, mSegType ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEG_SPACEID",
        offsetof( sdpscDumpExtDirCntlHdr, mSegSpaceID ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mSegSpaceID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEG_PAGEID",
        offsetof( sdpscDumpExtDirCntlHdr, mSegPageID ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mSegPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGEID",
        offsetof( sdpscDumpExtDirCntlHdr, mMyPageID ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mMyPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXT_CNT",
        offsetof( sdpscDumpExtDirCntlHdr, mExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mExtCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_EXT_DIR_PAGEID",
        offsetof( sdpscDumpExtDirCntlHdr, mNxtExtDir ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mNxtExtDir ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_EXTENT_CNT",
        offsetof( sdpscDumpExtDirCntlHdr, mMaxExtCnt ),
        IDU_FT_SIZEOF( sdpscDumpExtDirCntlHdr, mMaxExtCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LATEST_COMMIT_SCN",
        offsetof( sdpscDumpExtDirCntlHdr, mLstCommitSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_DISK_VIEW_SCN",
        offsetof( sdpscDumpExtDirCntlHdr, mFstDskViewSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
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
// X$DISK_UNDO_EXTDIRHDR Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskCmsExtDirHdrTableDesc =
{
    (SChar *)"X$DISK_UNDO_EXTDIRHDR",
    sdpscFT::buildRecord4ExtDirHdr,
    gDumpDiskCmsExtDirHdrColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
