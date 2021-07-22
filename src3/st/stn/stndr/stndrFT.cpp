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
 * $Id: stndrFT.cpp 88717 2020-09-24 08:12:20Z et16 $
 *
 * Description
 *
 *   PROJ-1618
 *   Disk RTree Index 의 FT를 위한 함수
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>

#include <sdbBufferMgr.h>
#include <sdpPhyPage.h>
#include <stndrModule.h>
#include <stndrStackMgr.h>
#include <sdnFT.h>
#include <stndrFT.h>
#include <sdnIndexCTL.h>
#include <sdnReq.h>
#include <smnManager.h>
#include <smiFixedTable.h>


/***********************************************************************
 * Description
 *
 *   D$DISK_INDEX_RTREE_CTS
 *   : Disk RTREE INDEX의 CTS을 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_INDEX_RTREE_CTS Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskIndexRTreeCTSlotColDesc[]=
{
    {
        (SChar*)"MY_PAGEID",
        offsetof(sdnDumpCTS, mMyPID ),
        IDU_FT_SIZEOF(sdnDumpCTS, mMyPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"PAGE_SEQ",
        offsetof(sdnDumpCTS, mPageSeq ),
        IDU_FT_SIZEOF(sdnDumpCTS, mPageSeq ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(sdnDumpCTS, mNthSlot ),
        IDU_FT_SIZEOF(sdnDumpCTS, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"COMMIT_SCN",
        offsetof(sdnDumpCTS, mCommitSCN ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NEXT_COMMIT_SCN",
        offsetof(sdnDumpCTS, mNxtCommitSCN ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"STATE",
        offsetof(sdnDumpCTS, mState ),
        IDU_FT_SIZEOF(sdnDumpCTS, mState ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"CHAINED",
        offsetof(sdnDumpCTS, mChained ),
        IDU_FT_SIZEOF(sdnDumpCTS, mChained ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TSS_PAGEID",
        offsetof(sdnDumpCTS, mTSSlotPID ),
        IDU_FT_SIZEOF(sdnDumpCTS, mTSSlotPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TSS_SLOTNUM",
        offsetof(sdnDumpCTS, mTSSlotNum),
        IDU_FT_SIZEOF(sdnDumpCTS, mTSSlotNum),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"UNDO_PAGEID",
        offsetof(sdnDumpCTS, mUndoPID ),
        IDU_FT_SIZEOF(sdnDumpCTS, mUndoPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"UNDO_SLOTNUM",
        offsetof(sdnDumpCTS, mUndoSlotNum),
        IDU_FT_SIZEOF(sdnDumpCTS, mUndoSlotNum),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_CNT",
        offsetof(sdnDumpCTS, mRefCnt ),
        IDU_FT_SIZEOF(sdnDumpCTS, mRefCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_KEY1",
        offsetof(sdnDumpCTS, mRefKey1 ),
        IDU_FT_SIZEOF(sdnDumpCTS, mRefKey1 ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_KEY2",
        offsetof(sdnDumpCTS, mRefKey2 ),
        IDU_FT_SIZEOF(sdnDumpCTS, mRefKey2 ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_KEY3",
        offsetof(sdnDumpCTS, mRefKey3 ),
        IDU_FT_SIZEOF(sdnDumpCTS, mRefKey3 ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
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
// D$DISK_INDEX_RTREE_CTS Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskIndexRTreeCTSlotTableDesc =
{
    (SChar *)"D$DISK_INDEX_RTREE_CTS",
    stndrFT::buildRecordCTS,
    gDumpDiskIndexRTreeCTSlotColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_INDEX_RTREE_CTS Dump Table의 레코드 Build
//------------------------------------------------------
IDE_RC stndrFT::buildRecordCTS( idvSQL              * /*aStatistics*/,
                                void                * aHeader,
                                void                * aDumpObj,
                                iduFixedTableMemory * aMemory )
{
    stndrHeader      * sIdxHdr = NULL;
    idBool             sIsLatched;
    idBool             sStackInit = ID_FALSE;
    stndrStack         sTraverseStack;
    ULong              sPageSeq = 0;

    
    sIsLatched = ID_FALSE;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );
    
    //------------------------------------------
    // Get Disk RTree Index Header
    //------------------------------------------
    
    sIdxHdr = (stndrHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr != NULL )
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mSdnHeader.mLatch.lockRead( NULL, /* idvSQL* */
                                                       NULL /* idvWeArgs* */)
                  != IDE_SUCCESS );
        sIsLatched = ID_TRUE;


        //------------------------------------------
        // Initialize
        //------------------------------------------

        IDE_TEST( stndrStackMgr::initialize( &sTraverseStack )
                  != IDE_SUCCESS );
        sStackInit = ID_TRUE;

        //------------------------------------------
        // Traverse
        //------------------------------------------
        
        IDE_TEST( traverseBuildCTS( NULL, /* idvSQL* */
                                    aHeader,
                                    aMemory,
                                    sIdxHdr,
                                    sIdxHdr->mSdnHeader.mIndexTSID,
                                    &sTraverseStack,
                                    &sPageSeq )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Finalize
        //------------------------------------------

        sStackInit = ID_FALSE;
        IDE_TEST( stndrStackMgr::destroy( &sTraverseStack )
                  != IDE_SUCCESS );

        // unlatch tree latch
        sIsLatched = ID_FALSE;
        IDE_TEST( sIdxHdr->mSdnHeader.mLatch.unlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ));
    }
    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ));
    }
    
    IDE_EXCEPTION_END;

    if ( sIsLatched == ID_TRUE )
    {
        sIsLatched = ID_FALSE;
        (void) sIdxHdr->mSdnHeader.mLatch.unlock();
    }

    if( sStackInit == ID_TRUE )
    {
        sStackInit = ID_FALSE;
        (void)stndrStackMgr::destroy( &sTraverseStack );
    }
    
    return IDE_FAILURE;    
}
    
IDE_RC stndrFT::traverseBuildCTS( idvSQL              * aStatistics,
                                  void                * aHeader,
                                  iduFixedTableMemory * aMemory,
                                  stndrHeader         * aIdxHdr,
                                  scSpaceID             aSpaceID,
                                  stndrStack          * aTraverseStack,
                                  ULong               * aPageSeq )
{
    SInt               i;
    SInt               sSlotCnt;
    idBool             sIsLeaf;
    idBool             sIsFixed = ID_FALSE;
    sdnDumpCTS         sDumpCTS; 
    sdnCTL           * sCTL;
    sdnCTS           * sCTS;
#ifndef COMPILE_64BIT
    ULong              sSCN;
#endif
    SChar              sStrCommitSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar              sStrNxtCommitSCN[ SM_SCN_STRING_LENGTH + 1];
    idBool             sTrySuccess;
    sdpPhyPageHdr    * sPageHdr;
    stndrNodeHdr     * sIdxNodeHdr;
    stndrIKey        * sIKey;
    UChar            * sSlotDirPtr;
    scPageID           sRootPID;    
    scPageID           sChildPID;
    idBool             sIsLastLimitResult;
    stndrStackSlot     sStackSlot;
    ULong              sIndexSmoNo;
    

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );
    IDE_DASSERT( aIdxHdr != NULL );
    IDE_DASSERT( aSpaceID != 0 );
    IDE_DASSERT( aTraverseStack != NULL );

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
    IDE_TEST_RAISE( sIsLastLimitResult == ID_TRUE, SKIP_BUILD_RECORDS );

    //------------------------------------------
    // Initialize
    //------------------------------------------

  retry:

    stndrStackMgr::clear( aTraverseStack );
    
    // push Root
    stndrRTree::getSmoNo( aIdxHdr, &sIndexSmoNo );

    sRootPID = aIdxHdr->mRootNode;

    IDE_TEST_RAISE( sRootPID == SD_NULL_PID, SKIP_BUILD_RECORDS );

    IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                   sRootPID,
                                   sIndexSmoNo,
                                   (SShort)STNDR_INVALID_KEY_SEQ )
              != IDE_SUCCESS );

    while( stndrStackMgr::getDepth( aTraverseStack ) >= 0 )
    {
        sStackSlot = stndrStackMgr::pop( aTraverseStack );

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sStackSlot.mNodePID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL,
                                              (UChar**)&sPageHdr,
                                              &sTrySuccess,
                                              NULL )   
                  != IDE_SUCCESS );
        sIsFixed = ID_TRUE;

        if( sdpPhyPage::getIndexSMONo(sPageHdr) > sStackSlot.mSmoNo )
        {
            // Root가 변경되었을 경우 재시도
            if( sStackSlot.mNodePID == sRootPID )
            {
                sIsFixed = ID_FALSE;
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                     (UChar*)sPageHdr )
                          != IDE_SUCCESS );
                goto retry;
            }

            IDE_ASSERT( sdpPhyPage::getNxtPIDOfDblList(sPageHdr) != SD_NULL_PID );
            
            IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                           sdpPhyPage::getNxtPIDOfDblList(sPageHdr),
                                           sStackSlot.mSmoNo,
                                           (SShort)STNDR_INVALID_KEY_SEQ )
                      != IDE_SUCCESS );
        }

        sIdxNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*) sPageHdr );
        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
        sSlotCnt    = sdpSlotDirectory::getCount(sSlotDirPtr);

        sIsLeaf = ( sIdxNodeHdr->mHeight == 0 ) ? ID_TRUE : ID_FALSE;
    
        //------------------------------------------
        // Build My Record
        //------------------------------------------
        *aPageSeq += 1;
        
        if( sIsLeaf == ID_TRUE )
        {
            sCTL = sdnIndexCTL::getCTL( sPageHdr );

            idlOS::memset( &sDumpCTS, 0x00, ID_SIZEOF( sdnDumpCTS ) );
        
            for( i = 0; i < sdnIndexCTL::getCount(sCTL); i++ )
            {
                sCTS = sdnIndexCTL::getCTS( sCTL, i );
            
                sDumpCTS.mMyPID   = sStackSlot.mNodePID;
                sDumpCTS.mPageSeq = *aPageSeq;
                sDumpCTS.mNthSlot = i;
                idlOS::memset( sStrCommitSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
#ifdef COMPILE_64BIT
                idlOS::sprintf( (SChar*)sStrCommitSCN, "%"ID_XINT64_FMT, sCTS->mCommitSCN );
#else
                sSCN  = (ULong)sCTS->mCommitSCN.mHigh << 32;
                sSCN |= (ULong)sCTS->mCommitSCN.mLow;
                idlOS::snprintf( (SChar*)sStrCommitSCN, SM_SCN_STRING_LENGTH,
                                 "%"ID_XINT64_FMT, sSCN );
#endif
                sDumpCTS.mCommitSCN = sStrCommitSCN;
                idlOS::memset( sStrNxtCommitSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                idlOS::sprintf( (SChar*)sStrNxtCommitSCN, "%"ID_XINT64_FMT, SM_SCN_INIT );
                sDumpCTS.mNxtCommitSCN = sStrNxtCommitSCN;
                sDumpCTS.mChained = 'N';

                switch( sCTS->mState )
                {
                    case SDN_CTS_NONE:
                        sDumpCTS.mState = 'N';
                        break;
                    case SDN_CTS_UNCOMMITTED:
                        sDumpCTS.mState = 'U';
                        break;
                    case SDN_CTS_STAMPED:
                        sDumpCTS.mState = 'S';
                        break;
                    case SDN_CTS_DEAD:
                        sDumpCTS.mState = 'D';
                        break;
                    default:
                        sDumpCTS.mState = '-';
                        break;
                }

                sDumpCTS.mTSSlotPID   = sCTS->mTSSlotPID;
                sDumpCTS.mTSSlotNum   = sCTS->mTSSlotNum;
                sDumpCTS.mRefCnt      = sCTS->mRefCnt;
                sDumpCTS.mRefKey1     = sCTS->mRefKey[0];
                sDumpCTS.mRefKey2     = sCTS->mRefKey[1];
                sDumpCTS.mRefKey3     = sCTS->mRefKey[2];

                // KEY CACHE의 크기가 커지면 위의 코드도 수정되어야 함.
                IDE_ASSERT( SDN_CTS_MAX_KEY_CACHE == 3 );
            
                IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                      aMemory,
                                                      (void *) & sDumpCTS )
                          != IDE_SUCCESS);
            }
        }
        else
        {
            //------------------------------------------
            // Child
            //------------------------------------------
            stndrRTree::getSmoNo( aIdxHdr, &sIndexSmoNo );
            for( i = sSlotCnt - 1; i >= 0; i-- )
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                            sSlotDirPtr, 
                                                            i,
                                                            (UChar**)&sIKey)
                          != IDE_SUCCESS );
            
                STNDR_GET_CHILD_PID( &sChildPID, sIKey );

                IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                               sChildPID,
                                               sIndexSmoNo,
                                               (SShort)STNDR_INVALID_KEY_SEQ )
                          != IDE_SUCCESS );
            }
        }

        sIsFixed = ID_FALSE;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             (UChar*)sPageHdr )
                  != IDE_SUCCESS );
    }
    
    //------------------------------------------
    // Finalize
    //------------------------------------------
    
    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sIsFixed == ID_TRUE )
    {
        (void) sdbBufferMgr::releasePage( aStatistics,
                                          (UChar*)sPageHdr );
    }
    
    return IDE_FAILURE;
}


/***********************************************************************
 * Description
 *
 *   D$DISK_INDEX_RTREE_STRUCTURE
 *   : Disk RTREE INDEX의 Page Tree 구조 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_INDEX_RTREE_STRUCTURE Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskRTreeStructureColDesc[]=
{
    {
        (SChar*)"HEIGHT",
        offsetof(stndrDumpTreePage, mHeight ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mHeight ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(stndrDumpTreePage, mIsLeaf ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGEID",
        offsetof(stndrDumpTreePage, mMyPID ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mMyPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PAGEID",
        offsetof(stndrDumpTreePage, mNextPID ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mNextPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_SEQ",
        offsetof(stndrDumpTreePage, mPageSeq ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mPageSeq ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof(stndrDumpTreePage, mSlotCount ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SMO_NO",
        offsetof(stndrDumpTreePage, mSmoNo ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mSmoNo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NODE_MBR",
        offsetof(stndrDumpTreePage, mNodeMBRString ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mNodeMBRString ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_X",
        offsetof(stndrDumpTreePage, mNodeMBR ) + offsetof(stdMBR, mMinX),
        IDU_FT_SIZEOF(stdMBR, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MIN_Y",
        offsetof(stndrDumpTreePage, mNodeMBR ) + offsetof(stdMBR, mMinY),
        IDU_FT_SIZEOF(stdMBR, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_X",
        offsetof(stndrDumpTreePage, mNodeMBR ) + offsetof(stdMBR, mMaxX),
        IDU_FT_SIZEOF(stdMBR, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_Y",
        offsetof(stndrDumpTreePage, mNodeMBR ) + offsetof(stdMBR, mMaxY),
        IDU_FT_SIZEOF(stdMBR, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TOTAL_FREE_SIZE",
        offsetof(stndrDumpTreePage, mTotalFreeSize ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mTotalFreeSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNLIMITED_KEY_COUNT",
        offsetof(stndrDumpTreePage, mUnlimitedKeyCount ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mUnlimitedKeyCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_DEAD_KEY_SIZE",
        offsetof(stndrDumpTreePage, mTotalDeadKeySize ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mTotalDeadKeySize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TBK_COUNT",
        offsetof(stndrDumpTreePage, mTBKCount ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mTBKCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_STATE",
        offsetof(stndrDumpTreePage, mPageState ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mPageState ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CTL_SIZE",
        offsetof(stndrDumpTreePage, mCTLayerSize ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mCTLayerSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CTS_USED_COUNT",
        offsetof(stndrDumpTreePage, mCTSlotUsedCount ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mCTSlotUsedCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(stndrDumpTreePage, mIsConsistent ),
        IDU_FT_SIZEOF(stndrDumpTreePage, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
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
// D$DISK_INDEX_RTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskRTreeStructureTableDesc =
{
    (SChar *)"D$DISK_INDEX_RTREE_STRUCTURE",
    stndrFT::buildRecordTreeStructure,
    gDumpDiskRTreeStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_INDEX_RTREE_HEADER Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC stndrFT::buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * aDumpObj,
                                          iduFixedTableMemory * aMemory )
{
    stndrHeader       * sIdxHdr = NULL;
    idBool              sIsLatched;
    ULong               sPageSeq = 0;
    stndrStack          sTraverseStack;
    idBool              sStackInit = ID_FALSE;

    sIsLatched = ID_FALSE;
    
    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Disk RTree Index Header
    //------------------------------------------
    
    sIdxHdr = (stndrHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr != NULL )
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mSdnHeader.mLatch.lockRead( NULL, /* idvSQL * */
                                                       NULL /* idvWeArgs */ )
                  != IDE_SUCCESS );
        sIsLatched = ID_TRUE;

        //------------------------------------------
        // Initialize
        //------------------------------------------
        
        IDE_TEST( stndrStackMgr::initialize( &sTraverseStack )
                  != IDE_SUCCESS );
        sStackInit = ID_TRUE;

        //------------------------------------------
        // Traverse
        //------------------------------------------
        
        IDE_TEST( traverseBuildTreePage( NULL, /* idvSQL* */
                                         aHeader,
                                         aMemory,
                                         sIdxHdr,
                                         sIdxHdr->mSdnHeader.mIndexTSID,
                                         &sTraverseStack,
                                         &sPageSeq )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Finalize
        //------------------------------------------
            
        sStackInit = ID_FALSE;
        IDE_TEST( stndrStackMgr::destroy( &sTraverseStack )
                  != IDE_SUCCESS );

        // unlatch tree latch
        sIsLatched = ID_FALSE;
        IDE_TEST( sIdxHdr->mSdnHeader.mLatch.unlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_INVALID_DUMP_OBJECT));
    }
    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DUMP_EMPTY_OBJECT));
    }
    
    IDE_EXCEPTION_END;

    if ( sIsLatched == ID_TRUE )
    {
        sIsLatched = ID_FALSE;
        (void)sIdxHdr->mSdnHeader.mLatch.unlock();
    }
    
    if( sStackInit == ID_TRUE )
    {
        sStackInit = ID_FALSE;
        (void)stndrStackMgr::destroy( &sTraverseStack );
    }

    return IDE_FAILURE;    
}
    
IDE_RC stndrFT::traverseBuildTreePage( idvSQL              * aStatistics,
                                       void                * aHeader,
                                       iduFixedTableMemory * aMemory,
                                       stndrHeader         * aIdxHdr,
                                       scSpaceID             aSpaceID,
                                       stndrStack          * aTraverseStack,
                                       ULong               * aPageSeq )
{
    SInt                i;
    stndrDumpTreePage   sDumpTreePage; // BUG-16648
    idBool              sIsFixed = ID_FALSE;
    sdpPhyPageHdr     * sPageHdr;
    UChar             * sSlotDirPtr;
    stndrNodeHdr      * sIdxNodeHdr;
    sdnCTL            * sCTL;
    stndrIKey         * sIKey;
    scPageID            sRootPID;
    scPageID            sChildPID;
    idBool              sIsLastLimitResult;
    idBool              sTrySuccess;
    stndrStackSlot      sStackSlot;
    ULong               sIndexSmoNo;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );
    IDE_DASSERT( aSpaceID != 0 );

  retry:

    stndrStackMgr::clear( aTraverseStack );
    
    // push Root
    stndrRTree::getSmoNo( aIdxHdr, &sIndexSmoNo );

    sRootPID = aIdxHdr->mRootNode;

    IDE_TEST_RAISE( sRootPID == SD_NULL_PID, SKIP_BUILD_RECORDS );

    IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                   sRootPID,
                                   sIndexSmoNo,
                                   (SShort)STNDR_INVALID_KEY_SEQ )
              != IDE_SUCCESS );

    while( stndrStackMgr::getDepth( aTraverseStack ) >= 0 )
    {
        sStackSlot = stndrStackMgr::pop( aTraverseStack );

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
        IDE_TEST_RAISE( sIsLastLimitResult == ID_TRUE, SKIP_BUILD_RECORDS );

        idlOS::memset( & sDumpTreePage, 0x00, ID_SIZEOF( stndrDumpTreePage ) );
        
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sStackSlot.mNodePID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL,
                                              (UChar**)&sPageHdr,
                                              &sTrySuccess,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );
        sIsFixed = ID_TRUE;

        if( sdpPhyPage::getIndexSMONo(sPageHdr) > sStackSlot.mSmoNo )
        {
            // Root가 변경되었을 경우 재시도
            if( sStackSlot.mNodePID == sRootPID )
            {
                sIsFixed = ID_FALSE;
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                     (UChar*)sPageHdr )
                          != IDE_SUCCESS );
                goto retry;
            }
            
            IDE_ASSERT( sdpPhyPage::getNxtPIDOfDblList(sPageHdr) != SD_NULL_PID );
            
            IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                           sdpPhyPage::getNxtPIDOfDblList(sPageHdr),
                                           sStackSlot.mSmoNo,
                                           (SShort)STNDR_INVALID_KEY_SEQ )
                      != IDE_SUCCESS );
        }

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);

        sIdxNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*) sPageHdr );
    
        sCTL = sdnIndexCTL::getCTL( sPageHdr );

        *aPageSeq += 1;
    
        //------------------------------------------
        // Build My Record
        //------------------------------------------
        sDumpTreePage.mHeight            = sIdxNodeHdr->mHeight;
        sDumpTreePage.mIsLeaf            =
            ( sDumpTreePage.mHeight > 0 ) ? 'F' : 'T';
        sDumpTreePage.mMyPID             = sStackSlot.mNodePID;
        sDumpTreePage.mNextPID           = sPageHdr->mListNode.mNext;
        sDumpTreePage.mPageSeq           = *aPageSeq;
        sDumpTreePage.mSlotCount         = sdpSlotDirectory::getCount(sSlotDirPtr);
        sDumpTreePage.mSmoNo             = sdpPhyPage::getIndexSMONo( sPageHdr );
        sDumpTreePage.mTotalFreeSize     = sdpPhyPage::getTotalFreeSize( sPageHdr );
        sDumpTreePage.mTBKCount          = sIdxNodeHdr->mTBKCount;
    
        if( sDumpTreePage.mHeight == 0 )
        {
            sDumpTreePage.mUnlimitedKeyCount = sIdxNodeHdr->mUnlimitedKeyCount;
            sDumpTreePage.mTotalDeadKeySize  = sIdxNodeHdr->mTotalDeadKeySize;
            switch( sIdxNodeHdr->mState )
            {
                case STNDR_IN_USED:
                    sDumpTreePage.mPageState = 'U';
                    break;
                case STNDR_IN_EMPTY_LIST:
                    sDumpTreePage.mPageState = 'E';
                    break;
                case STNDR_IN_FREE_LIST:
                    sDumpTreePage.mPageState = 'F';
                    break;
                default:
                    sDumpTreePage.mPageState = '-';
                    break;
            }
            sDumpTreePage.mCTLayerSize = sCTL->mCount;
            sDumpTreePage.mCTSlotUsedCount = sCTL->mUsedCount;
        }
        else
        {
            sDumpTreePage.mUnlimitedKeyCount = 0;
            sDumpTreePage.mTotalDeadKeySize  = 0;
            sDumpTreePage.mPageState = '-';
            sDumpTreePage.mCTLayerSize = 0;
            sDumpTreePage.mCTSlotUsedCount = 0;
        }

        sDumpTreePage.mNodeMBR = sIdxNodeHdr->mMBR;
        IDE_TEST( convertMBR2String( (UChar*)&sIdxNodeHdr->mMBR,
                                     (UChar*)sDumpTreePage.mNodeMBRString )
                  != IDE_SUCCESS );
    
        sDumpTreePage.mIsConsistent =
            (sPageHdr->mIsConsistent == SDP_PAGE_CONSISTENT)? 'T' : 'F';
    
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) & sDumpTreePage )
                 != IDE_SUCCESS);

        //------------------------------------------
        // Child
        //------------------------------------------
        if( sDumpTreePage.mHeight > 0 )
        {
            stndrRTree::getSmoNo( aIdxHdr, &sIndexSmoNo );
            for ( i = sDumpTreePage.mSlotCount - 1; i >= 0; i-- )
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                        sSlotDirPtr, 
                                                        i,
                                                        (UChar**)&sIKey)
                          != IDE_SUCCESS );
            
                STNDR_GET_CHILD_PID( &sChildPID, sIKey );

                IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                               sChildPID,
                                               sIndexSmoNo,
                                               (SShort)STNDR_INVALID_KEY_SEQ )
                          != IDE_SUCCESS );
            }
        }
        
        sIsFixed = ID_FALSE;
        IDE_TEST( sdbBufferMgr::releasePage(aStatistics,
                                            (UChar*)sPageHdr )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------
    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sIsFixed == ID_TRUE )
    {
        (void) sdbBufferMgr::releasePage(aStatistics,
                                         (UChar*)sPageHdr );
    }
    
    return IDE_FAILURE;
}


/***********************************************************************
 * Description
 *
 *   D$DISK_INDEX_RTREE_KEY
 *   : Disk RTREE INDEX의 KEY 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_INDEX_RTREE_KEY Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskRTreeKeyColDesc[]=
{
    {
        (SChar*)"MY_PAGEID",
        offsetof(stndrDumpKey, mMyPID ),
        IDU_FT_SIZEOF(stndrDumpKey, mMyPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_SEQ",
        offsetof(stndrDumpKey, mPageSeq ),
        IDU_FT_SIZEOF(stndrDumpKey, mPageSeq ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HEIGHT",
        offsetof(stndrDumpKey, mHeight ),
        IDU_FT_SIZEOF(stndrDumpKey, mHeight ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(stndrDumpKey, mIsLeaf ),
        IDU_FT_SIZEOF(stndrDumpKey, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(stndrDumpKey, mNthSlot ),
        IDU_FT_SIZEOF(stndrDumpKey, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_LENGTH",
        offsetof(stndrDumpKey, mColumnLength ),
        IDU_FT_SIZEOF(stndrDumpKey, mColumnLength ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"VALUE24B",
        offsetof(stndrDumpKey, mValue ),
        IDU_FT_SIZEOF(stndrDumpKey, mValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_X",
        offsetof(stndrDumpKey, mMBR ) + offsetof(stdMBR, mMinX),
        IDU_FT_SIZEOF(stdMBR, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MIN_Y",
        offsetof(stndrDumpKey, mMBR ) + offsetof(stdMBR, mMinY),
        IDU_FT_SIZEOF(stdMBR, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_X",
        offsetof(stndrDumpKey, mMBR ) + offsetof(stdMBR, mMaxX),
        IDU_FT_SIZEOF(stdMBR, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_Y",
        offsetof(stndrDumpKey, mMBR ) + offsetof(stdMBR, mMaxY),
        IDU_FT_SIZEOF(stdMBR, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"CHILD_PAGEID",
        offsetof(stndrDumpKey, mChildPID ),
        IDU_FT_SIZEOF(stndrDumpKey, mChildPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_PAGEID",
        offsetof(stndrDumpKey, mRowPID ),
        IDU_FT_SIZEOF(stndrDumpKey, mRowPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_SLOTNUM",
        offsetof(stndrDumpKey, mRowSlotNum ),
        IDU_FT_SIZEOF(stndrDumpKey, mRowSlotNum ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof(stndrDumpKey, mState ),
        IDU_FT_SIZEOF(stndrDumpKey, mState ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_CTS_NO",
        offsetof(stndrDumpKey, mCreateCTS ),
        IDU_FT_SIZEOF(stndrDumpKey, mCreateCTS ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_CHAINED",
        offsetof(stndrDumpKey, mCreateChained ),
        IDU_FT_SIZEOF(stndrDumpKey, mCreateChained ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_CTS_NO",
        offsetof(stndrDumpKey, mLimitCTS ),
        IDU_FT_SIZEOF(stndrDumpKey, mLimitCTS ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_CHAINED",
        offsetof(stndrDumpKey, mLimitChained ),
        IDU_FT_SIZEOF(stndrDumpKey, mLimitChained ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TB_TYPE",
        offsetof(stndrDumpKey, mTxBoundType ),
        IDU_FT_SIZEOF(stndrDumpKey, mTxBoundType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_TSS_SID",
        offsetof(stndrDumpKey, mCreateTSS ),
        IDU_FT_SIZEOF(stndrDumpKey, mCreateTSS ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_TSS_SID",
        offsetof(stndrDumpKey, mLimitTSS ),
        IDU_FT_SIZEOF(stndrDumpKey, mLimitTSS ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_COMMIT_SCN",
        offsetof(stndrDumpKey, mCreateCSCN ),
        SM_SCN_STRING_LENGTH,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"LIMIT_COMMIT_SCN",
        offsetof(stndrDumpKey, mLimitCSCN ),
        SM_SCN_STRING_LENGTH,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"CREATE_SCN",
        offsetof(stndrDumpKey, mCreateSCN ),
        SM_SCN_STRING_LENGTH,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"LIMIT_SCN",
        offsetof(stndrDumpKey, mLimitSCN ),
        SM_SCN_STRING_LENGTH,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
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
// D$DISK_INDEX_RTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskRTreeKeyTableDesc =
{
    (SChar *)"D$DISK_INDEX_RTREE_KEY",
    stndrFT::buildRecordKey,
    gDumpDiskRTreeKeyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_INDEX_RTREE_HEADER Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC stndrFT::buildRecordKey( idvSQL              * /*aStatistics*/,
                                void                * aHeader,
                                void                * aDumpObj,
                                iduFixedTableMemory * aMemory )
{
    stndrHeader       * sIdxHdr = NULL;
    idBool              sIsLatched;
    ULong               sPageSeq = 0;
    stndrStack          sTraverseStack;
    idBool              sStackInit;

    sIsLatched = ID_FALSE;
    sStackInit = ID_FALSE;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );
    
    //------------------------------------------
    // Get Disk RTree Index Header
    //------------------------------------------
    
    sIdxHdr = (stndrHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr != NULL )
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mSdnHeader.mLatch.lockRead( NULL, /* idvSQL* */
                                                       NULL /* idvWeArgs* */)
                  != IDE_SUCCESS );
        sIsLatched = ID_TRUE;

        //------------------------------------------
        // Initialize
        //------------------------------------------
        IDE_TEST( stndrStackMgr::initialize( &sTraverseStack )
                  != IDE_SUCCESS );
        sStackInit = ID_TRUE;

        //------------------------------------------
        // Traverse
        //------------------------------------------

        IDE_TEST( traverseBuildKey( NULL, /* idvSQL* */
                                    aHeader,
                                    aMemory,
                                    sIdxHdr,
                                    sIdxHdr->mSdnHeader.mIndexTSID,
                                    &sTraverseStack,
                                    &sPageSeq ) // Parent PID
                  != IDE_SUCCESS );

        //------------------------------------------
        // Finalize
        //------------------------------------------
                
        sStackInit = ID_FALSE;
        IDE_TEST( stndrStackMgr::destroy( &sTraverseStack )
                  != IDE_SUCCESS );
        
        // unlatch tree latch
        sIsLatched = ID_FALSE;
        IDE_TEST( sIdxHdr->mSdnHeader.mLatch.unlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ) );
    }
    IDE_EXCEPTION( ERR_EMPTY_OBJECT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    
    IDE_EXCEPTION_END;

    if ( sIsLatched == ID_TRUE )
    {
        sIsLatched = ID_FALSE;
        (void)sIdxHdr->mSdnHeader.mLatch.unlock();
    }
    
    if( sStackInit == ID_TRUE )
    {
        sStackInit = ID_FALSE;
        (void)stndrStackMgr::destroy( &sTraverseStack );
    }

    return IDE_FAILURE;    
}
    
IDE_RC stndrFT::traverseBuildKey( idvSQL              * aStatistics,
                                  void                * aHeader,
                                  iduFixedTableMemory * aMemory,
                                  stndrHeader         * aIdxHdr,
                                  scSpaceID             aSpaceID,
                                  stndrStack          * aTraverseStack,
                                  ULong               * aPageSeq )
{
    SInt                i;
    SInt                sSlotCnt;
    idBool              sIsLeaf;
    // BUG-29039 codesonar ( Uninitialized Variable )
    idBool              sIsFixed = ID_FALSE;
    stndrDumpKey        sDumpKey; 
    
    idBool              sTrySuccess;
    
    sdpPhyPageHdr     * sPageHdr;
    UChar             * sSlotDirPtr;
    stndrNodeHdr      * sIdxNodeHdr;
    
    stndrIKey         * sIKey;
    stndrLKey         * sLKey;
    stndrKeyInfo        sKeyInfo;

    UChar             * sKeyPtr;
    idBool              sIsLastLimitResult;

#ifndef COMPILE_64BIT
    ULong               sSCN;
#endif
    smSCN               sCommitSCN;
    smSCN               sCISCN;
    SChar               sStrCreateCSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar               sStrLimitCSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar               sStrCreateSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar               sStrLimitSCN[ SM_SCN_STRING_LENGTH + 1];

    scPageID            sRootPID;
    scPageID            sRightChildPID;
    UInt                sColumnLength;
    UInt                sColumnHeaderLength;
    UInt                sColumnValueLength;
    stndrStackSlot      sStackSlot;
    ULong               sIndexSmoNo;
    
    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aMemory != NULL );
    IDE_DASSERT( aIdxHdr != NULL );
    IDE_DASSERT( aSpaceID != 0 );

  retry:

    stndrStackMgr::clear( aTraverseStack );
    
    // push Root
    stndrRTree::getSmoNo( aIdxHdr, &sIndexSmoNo );

    sRootPID = aIdxHdr->mRootNode;

    IDE_TEST_RAISE( sRootPID == SD_NULL_PID, SKIP_BUILD_RECORDS );

    IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                   sRootPID,
                                   sIndexSmoNo,
                                   (SShort)STNDR_INVALID_KEY_SEQ )
              != IDE_SUCCESS );

    while( stndrStackMgr::getDepth(aTraverseStack) >= 0 )
    {
        sStackSlot = stndrStackMgr::pop( aTraverseStack );

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
        IDE_TEST_RAISE( sIsLastLimitResult == ID_TRUE, SKIP_BUILD_RECORDS );

        idlOS::memset( & sDumpKey, 0x00, ID_SIZEOF( stndrDumpKey ) );

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sStackSlot.mNodePID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL,
                                              (UChar**)&sPageHdr,
                                              & sTrySuccess,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );
        sIsFixed = ID_TRUE;

        if( sdpPhyPage::getIndexSMONo(sPageHdr) > sStackSlot.mSmoNo )
        {
            // Root가 변경되었을 경우 재시도
            if( sStackSlot.mNodePID == sRootPID )
            {
                sIsFixed = ID_FALSE;
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                     (UChar*)sPageHdr )
                          != IDE_SUCCESS );
                goto retry;
            }
            
            IDE_ASSERT( sdpPhyPage::getNxtPIDOfDblList(sPageHdr) != SD_NULL_PID );
            
            IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                           sdpPhyPage::getNxtPIDOfDblList(sPageHdr),
                                           sStackSlot.mSmoNo,
                                           (SShort)STNDR_INVALID_KEY_SEQ )
                      != IDE_SUCCESS );
        }

        sIdxNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*) sPageHdr );

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
        sSlotCnt    = sdpSlotDirectory::getCount(sSlotDirPtr);
        sIsLeaf = ( sIdxNodeHdr->mHeight == 0 ) ? ID_TRUE : ID_FALSE;
    
        *aPageSeq += 1;
    
        //------------------------------------------
        // Build My Record
        //------------------------------------------
        sDumpKey.mMyPID         = sStackSlot.mNodePID;
        sDumpKey.mPageSeq       = *aPageSeq;
        sDumpKey.mHeight        = sIdxNodeHdr->mHeight;
        sDumpKey.mIsLeaf        = ( sIdxNodeHdr->mHeight == 0 ) ? 'T' : 'F';
    
        for ( i = 0; i < sSlotCnt; i++ )
        {
            if ( sIsLeaf == ID_TRUE )
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                        sSlotDirPtr, 
                                                        i,
                                                        (UChar**)&sLKey )
                          != IDE_SUCCESS );
            
                sDumpKey.mNthSlot    = i;
                sDumpKey.mChildPID   = SD_NULL_PID;
            
                STNDR_LKEY_TO_KEYINFO( sLKey, sKeyInfo );
            
                sDumpKey.mRowPID     = sKeyInfo.mRowPID;
                sDumpKey.mRowSlotNum = sKeyInfo.mRowSlotNum;

                switch( STNDR_GET_STATE(sLKey) )
                {
                    case STNDR_KEY_UNSTABLE:
                        sDumpKey.mState = 'U';
                        break;
                    case STNDR_KEY_STABLE:
                        sDumpKey.mState = 'S';
                        break;
                    case STNDR_KEY_DELETED:
                        sDumpKey.mState = 'd';
                        break;
                    case STNDR_KEY_DEAD:
                        sDumpKey.mState = 'D';
                        break;
                    default:
                        sDumpKey.mState = '-';
                        break;
                }

                sDumpKey.mCreateCTS  = STNDR_GET_CCTS_NO( sLKey );
                sDumpKey.mCreateChained = 'N';
                sDumpKey.mLimitCTS  = STNDR_GET_LCTS_NO( sLKey );
                sDumpKey.mLimitChained = 'N';
            
                sDumpKey.mTxBoundType =
                    (STNDR_GET_TB_TYPE( sLKey ) == STNDR_KEY_TB_CTS) ? 'T' : 'K';

                if( STNDR_GET_TB_TYPE( sLKey ) == STNDR_KEY_TB_KEY )
                {
                    STNDR_GET_TBK_CTSS( ((stndrLKeyEx*)sLKey), &sDumpKey.mCreateTSS );
                    STNDR_GET_TBK_LTSS( ((stndrLKeyEx*)sLKey), &sDumpKey.mLimitTSS );

                    STNDR_GET_TBK_CSCN( ((stndrLKeyEx*)sLKey), &sCommitSCN );
                    idlOS::memset( sStrCreateCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                
#ifdef COMPILE_64BIT
                    idlOS::sprintf( (SChar*)sStrCreateCSCN, "%"ID_XINT64_FMT, sCommitSCN );
#else
                    sSCN  = (ULong)sCommitSCN.mHigh << 32;
                    sSCN |= (ULong)sCommitSCN.mLow;
                    idlOS::snprintf( (SChar*)sStrCreateCSCN, SM_SCN_STRING_LENGTH,
                                     "%"ID_XINT64_FMT, sSCN );
#endif
                
                    STNDR_GET_TBK_LSCN( ((stndrLKeyEx*)sLKey), &sCommitSCN );
                    idlOS::memset( sStrLimitCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );

#ifdef COMPILE_64BIT
                    idlOS::sprintf( (SChar*)sStrLimitCSCN, "%"ID_XINT64_FMT, sCommitSCN );
#else
                    sSCN  = (ULong)sCommitSCN.mHigh << 32;
                    sSCN |= (ULong)sCommitSCN.mLow;
                    idlOS::snprintf( (SChar*)sStrLimitCSCN, SM_SCN_STRING_LENGTH,
                                     "%"ID_XINT64_FMT, sSCN );
#endif
                }
                else
                {
                    sDumpKey.mCreateTSS = 0;
                    sDumpKey.mLimitTSS = 0;
                    idlOS::memset( sStrCreateCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                    idlOS::strcpy( sStrCreateCSCN, "-" );
                    idlOS::memset( sStrLimitCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                    idlOS::strcpy( sStrLimitCSCN, "-" );
                }

                STNDR_GET_CSCN( ((stndrLKey*)sLKey), &sCISCN );
                idlOS::memset( sStrCreateSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
#ifdef COMPILE_64BIT
                idlOS::sprintf( (SChar*)sStrCreateSCN, "%"ID_XINT64_FMT, sCISCN );
#else

                sSCN  = (ULong)sCISCN.mHigh << 32;
                sSCN |= (ULong)sCISCN.mLow;
                idlOS::snprintf( (SChar*)sStrCreateSCN, SM_SCN_STRING_LENGTH,
                                 "%"ID_XINT64_FMT, sSCN );
#endif

                STNDR_GET_LSCN( ((stndrLKey*)sLKey), &sCISCN );
                idlOS::memset( sStrLimitSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
#ifdef COMPILE_64BIT
                idlOS::sprintf( (SChar*)sStrLimitSCN, "%"ID_XINT64_FMT, sCISCN );
#else

                sSCN  = (ULong)sCISCN.mHigh << 32;
                sSCN |= (ULong)sCISCN.mLow;
                idlOS::snprintf( (SChar*)sStrLimitSCN, SM_SCN_STRING_LENGTH,
                                 "%"ID_XINT64_FMT, sSCN );
#endif
            
                sDumpKey.mCreateCSCN = sStrCreateCSCN;
                sDumpKey.mLimitCSCN = sStrLimitCSCN;

                sDumpKey.mCreateSCN = sStrCreateSCN;
                sDumpKey.mLimitSCN = sStrLimitSCN;

                sKeyPtr = STNDR_LKEY_KEYVALUE_PTR(sLKey);
            }
            else
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                            sSlotDirPtr, 
                                                            i,
                                                            (UChar**)&sIKey )
                          != IDE_SUCCESS );

                sDumpKey.mNthSlot = i;
                STNDR_GET_CHILD_PID( &sDumpKey.mChildPID, sIKey );
                STNDR_IKEY_TO_KEYINFO( sIKey, sKeyInfo );
                sDumpKey.mRowPID = sKeyInfo.mRowPID;
                sDumpKey.mRowSlotNum = sKeyInfo.mRowSlotNum;
                sDumpKey.mState = '-';
                sDumpKey.mCreateCTS = 0;
                sDumpKey.mCreateChained = '-';
                sDumpKey.mLimitCTS = 0;
                sDumpKey.mLimitChained = '-';
                sDumpKey.mTxBoundType = '-';
                sDumpKey.mCreateTSS = 0;
                sDumpKey.mLimitTSS = 0;
                idlOS::memset( sStrCreateCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                idlOS::strcpy( sStrCreateCSCN, "-" );
                idlOS::memset( sStrLimitCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                idlOS::strcpy( sStrLimitCSCN, "-" );
                sDumpKey.mCreateCSCN = sStrCreateCSCN;
                sDumpKey.mLimitCSCN = sStrLimitCSCN;
                idlOS::memset( sStrCreateSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                idlOS::strcpy( sStrCreateSCN, "-" );
                idlOS::memset( sStrLimitSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
                idlOS::strcpy( sStrLimitSCN, "-" );
                sDumpKey.mCreateSCN = sStrCreateSCN;
                sDumpKey.mLimitSCN = sStrLimitSCN;

                sKeyPtr = STNDR_IKEY_KEYVALUE_PTR(sIKey);
            }
        

            //------------------------------
            // BUG-16805 Column별 Value String 생성
            //------------------------------

            idlOS::memcpy( (UChar*)&sDumpKey.mMBR, (UChar*)sKeyPtr, ID_SIZEOF(stdMBR) );
            IDE_TEST( convertMBR2String( (UChar*)sKeyPtr,
                                         (UChar*)sDumpKey.mValue )
                      != IDE_SUCCESS );
                               
            sColumnLength  = stndrRTree::getColumnLength( 
                ((UChar*)sKeyPtr),
                &sColumnHeaderLength,
                &sColumnValueLength,
                NULL );
            
            sDumpKey.mColumnLength = sColumnLength;

            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *) & sDumpKey )
                     != IDE_SUCCESS);
        }

        //------------------------------------------
        // Build Child Record
        //------------------------------------------
        if ( sIsLeaf == ID_FALSE )
        {
            stndrRTree::getSmoNo( aIdxHdr, &sIndexSmoNo );
            for ( i = sSlotCnt - 1; i >= 0; i-- )
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                            sSlotDirPtr, 
                                                            i,
                                                            (UChar**)&sIKey )
                          != IDE_SUCCESS );
            
                STNDR_GET_CHILD_PID( &sRightChildPID, sIKey );
                IDE_TEST( stndrStackMgr::push( aTraverseStack,
                                               sRightChildPID,
                                               sIndexSmoNo,
                                               (SShort)STNDR_INVALID_KEY_SEQ )
                          != IDE_SUCCESS );
            }
        }
        
        sIsFixed = ID_FALSE;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             (UChar*)sPageHdr )
                  != IDE_SUCCESS );
    }
    
    //------------------------------------------
    // Finalize
    //------------------------------------------
    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sIsFixed == ID_TRUE )
    {
        (void)sdbBufferMgr::releasePage( aStatistics,
                                         (UChar*)sPageHdr );
    }
    
    return IDE_FAILURE;
}

IDE_RC stndrFT::convertMBR2String( UChar * sSrcKeyPtr,
                                   UChar * aDestString )
{
    UChar         sValueBuffer[SM_DUMP_VALUE_BUFFER_SIZE];
    UInt          sValueLength;
    
    idlOS::memset( aDestString, 0x00, SM_DUMP_VALUE_LENGTH );
    sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
            
    IDE_TEST( stndrRTree::columnValue2String( (UChar*)sSrcKeyPtr,
                                              (UChar*)sValueBuffer,
                                              &sValueLength )
              != IDE_SUCCESS );
            
    idlOS::memcpy( aDestString,
                   sValueBuffer,
                   ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                   SM_DUMP_VALUE_LENGTH : sValueLength );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
                                   
//======================================================================
//  X$DISK_RTREE_HEADER
//  disk index의 run-time header를 보여주는 peformance view
//======================================================================

IDE_RC stndrFT::buildRecordForDiskRTreeHeader(idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * /* aDumpObj */,
                                              iduFixedTableMemory * aMemory)
{
    smcTableHeader     * sCatTblHdr;
    smcTableHeader     * sTableHeader;
    smpSlotHeader      * sPtr;
    SChar              * sCurPtr;
    SChar              * sNxtPtr;
    UInt                 sTableType;
    smnIndexHeader     * sIndexCursor;
    stndrHeader        * sIndexHeader;
    stndrHeader4PerfV    sIndexHeader4PerfV;
    void               * sTrans;
    UInt                 sIndexCnt;
    UInt                 i;
    smSCN                sFreeNodeSCN;
    SChar                sStrFreeNodeSCN[ SM_SCN_STRING_LENGTH + 1];
#ifndef COMPILE_64BIT
    ULong               sSCN;
#endif
    void              * sISavepoint = NULL;
    UInt                sDummy = 0;
    UInt                sState = 0;
    
    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr = NULL;

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

        sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

        // disk table only
        if( sTableType != SMI_TABLE_DISK )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        if( smcTable::isDropedTable(sTableHeader) == ID_TRUE )
        {
            sCurPtr = sNxtPtr;
            continue;
        }//if

        sIndexCnt =  smcTable::getIndexCount(sTableHeader);

        if( sIndexCnt != 0  )
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
                                                              SMC_TABLE_LOCK( sTableHeader ))
                     == IDE_SUCCESS )
            {
                sState = 2;
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                if( smcTable::isDropedTable(sTableHeader) == ID_TRUE )
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

                // lock을 대기하는 동안 index가 drop되었거나, 새로운 index가
                // 생성되었을 수 있으므로 정확한 index 수를 다시 구한다.
                // 뿐만 아니라, index cnt를 증가시킨 후 index를 생성하므로
                // index가 완료되지 못하면 index cnt가 감소하므로 다시 구해야 함.
                sIndexCnt = smcTable::getIndexCount(sTableHeader);

                for( i = 0; i < sIndexCnt; i++ )
                {
                    sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader, i );
                    IDE_TEST( sIndexCursor == NULL );

                    if( sIndexCursor->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    // BUG-30867 Discard 된 Tablespace에 속한 Index는 Skip되어야 함
                    if( sctTableSpaceMgr::hasState( sIndexCursor->mIndexSegDesc.mSpaceID,
                                                    SCT_SS_INVALID_DISK_TBS ) == ID_TRUE )
                    {
                        continue;
                    }

                    sIndexHeader = (stndrHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        idlOS::memset( &sIndexHeader4PerfV,
                                       0x00,
                                       ID_SIZEOF(stndrHeader4PerfV) );

                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;

                        sIndexHeader4PerfV.mIsConsistent = 'F';
                    }
                    else
                    {
                        idlOS::memset( &sIndexHeader4PerfV,
                                       0x00,
                                       ID_SIZEOF(stndrHeader4PerfV) );

                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8 );
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;

                        sIndexHeader4PerfV.mTreeMBR    = sIndexHeader->mTreeMBR;
                        sIndexHeader4PerfV.mIndexTSID  =
                            sIndexHeader->mSdnHeader.mIndexTSID;
                        sIndexHeader4PerfV.mTableTSID  =
                            sIndexHeader->mSdnHeader.mTableTSID;
                        sIndexHeader4PerfV.mSegHdrPID  = sdpSegDescMgr::getSegPID(
                            &sIndexHeader->mSdnHeader.mSegmentDesc);
                        sIndexHeader4PerfV.mRootNode      = sIndexHeader->mRootNode;
                        sIndexHeader4PerfV.mEmptyNodeHead =
                            sIndexHeader->mEmptyNodeHead;
                        sIndexHeader4PerfV.mEmptyNodeTail =
                            sIndexHeader->mEmptyNodeTail;
                        stndrRTree::getSmoNo( (void*)sIndexHeader,
                                              &sIndexHeader4PerfV.mSmoNo );

                        sIndexHeader4PerfV.mFreeNodeHead =
                            sIndexHeader->mFreeNodeHead;
                        sIndexHeader4PerfV.mFreeNodeCnt =
                            sIndexHeader->mFreeNodeCnt;

                        SM_SET_SCN( &sFreeNodeSCN, &sIndexHeader->mFreeNodeSCN );
                        idlOS::memset( sStrFreeNodeSCN,
                                       0x00,
                                       sizeof(sStrFreeNodeSCN) );
                    
#ifdef COMPILE_64BIT
                        idlOS::sprintf( (SChar*)sStrFreeNodeSCN,
                                        "%"ID_XINT64_FMT,
                                        sFreeNodeSCN );
#else
                        sSCN  = (ULong)sFreeNodeSCN.mHigh << 32;
                        sSCN |= (ULong)sFreeNodeSCN.mLow;
                        idlOS::snprintf( (SChar*)sStrFreeNodeSCN,
                                         SM_SCN_STRING_LENGTH,
                                         "%"ID_XINT64_FMT, sSCN );
#endif
                        /* BUG-32764 [st-disk-index] The RTree module writes 
                         * the invalid log of FreeNodeSCN */
                        sIndexHeader4PerfV.mFreeNodeSCN = sStrFreeNodeSCN;
                    
                        sIndexHeader4PerfV.mCompletionLSN =
                            sIndexHeader->mSdnHeader.mCompletionLSN;

                        sIndexHeader4PerfV.mIsConsistent =
                            ( sIndexHeader->mSdnHeader.mIsConsistent
                              == ID_TRUE ) ? 'T' : 'F';

                        // BUG-17957
                        // X$DISK_RTREE_HEADER에 creation option(logging, force) 추가
                        sIndexHeader4PerfV.mIsCreatedWithLogging =
                            ( sIndexHeader->mSdnHeader.mIsCreatedWithLogging
                              == ID_TRUE ) ? 'T' : 'F';
                    
                        sIndexHeader4PerfV.mIsCreatedWithForce =
                            ( sIndexHeader->mSdnHeader.mIsCreatedWithForce
                              == ID_TRUE ) ? 'T' : 'F';

                        // PROJ-1704
                        sIndexHeader4PerfV.mSegAttr =
                            sIndexHeader->mSdnHeader.mSegmentDesc.mSegHandle.mSegAttr;
                    
                        // PROJ-1671
                        sIndexHeader4PerfV.mSegStoAttr =
                            sIndexHeader->mSdnHeader.mSegmentDesc.mSegHandle.mSegStoAttr;
                    }

                    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                          aMemory,
                                                          (void *)&sIndexHeader4PerfV )
                              != IDE_SUCCESS);
                }//for
                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                sISavepoint )
                          != IDE_SUCCESS );
            }
            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                          sISavepoint )
                      != IDE_SUCCESS );
        }// if 인덱스가 있으면
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

static iduFixedTableColDesc  gDiskRTreeHeaderColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(stndrHeader4PerfV, mName ),
        SMN_MAX_INDEX_NAME_SIZE,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(stndrHeader4PerfV, mIndexID ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INDEX_TBS_ID",
        offsetof(stndrHeader4PerfV, mIndexTSID ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mIndexTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TABLE_TBS_ID",
        offsetof(stndrHeader4PerfV, mTableTSID ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mTableTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"SEG_HDR_PAGEID",
        offsetof(stndrHeader4PerfV, mSegHdrPID ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mSegHdrPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"ROOT_PAGEID",
        offsetof(stndrHeader4PerfV, mRootNode ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mRootNode ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"EMPTY_HEAD_PAGEID",
        offsetof(stndrHeader4PerfV, mEmptyNodeHead ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mEmptyNodeHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"EMPTY_TAIL_PAGEID",
        offsetof(stndrHeader4PerfV, mEmptyNodeTail ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mEmptyNodeTail ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"SMO_NO",
        offsetof(stndrHeader4PerfV, mSmoNo ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mSmoNo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FREE_NODE_HEAD",
        offsetof(stndrHeader4PerfV, mFreeNodeHead ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mFreeNodeHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FREE_NODE_CNT",
        offsetof(stndrHeader4PerfV, mFreeNodeCnt ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mFreeNodeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    // TEST CASE: FreeNodeSCN 확인하기
    {
        (SChar*)"FREE_NODE_SCN",
        offsetof(stndrHeader4PerfV, mFreeNodeSCN ),
        SM_SCN_STRING_LENGTH,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(stndrHeader4PerfV, mIsConsistent ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CREATED_WITH_LOGGING",
        offsetof(stndrHeader4PerfV, mIsCreatedWithLogging ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mIsCreatedWithLogging ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CREATED_WITH_FORCE",
        offsetof(stndrHeader4PerfV, mIsCreatedWithForce ),
        IDU_FT_SIZEOF(stndrHeader4PerfV, mIsCreatedWithForce ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"COMPLETION_LSN_FILE_NO",
        offsetof(stndrHeader4PerfV, mCompletionLSN ) + offsetof(smLSN, mFileNo ),
        IDU_FT_SIZEOF(smLSN, mFileNo ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"COMPLETION_LSN_FILE_OFFSET",
        offsetof(stndrHeader4PerfV, mCompletionLSN ) + offsetof(smLSN, mOffset ),
        IDU_FT_SIZEOF(smLSN, mOffset ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MIN_X",
        offsetof(stndrHeader4PerfV, mTreeMBR ) + offsetof(stdMBR, mMinX),
        IDU_FT_SIZEOF(stdMBR, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MIN_Y",
        offsetof(stndrHeader4PerfV, mTreeMBR ) + offsetof(stdMBR, mMinY),
        IDU_FT_SIZEOF(stdMBR, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_X",
        offsetof(stndrHeader4PerfV, mTreeMBR ) + offsetof(stdMBR, mMaxX),
        IDU_FT_SIZEOF(stdMBR, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_Y",
        offsetof(stndrHeader4PerfV, mTreeMBR ) + offsetof(stdMBR, mMaxY),
        IDU_FT_SIZEOF(stdMBR, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INIT_TRANS",
        offsetof(stndrHeader4PerfV, mSegAttr ) +
            offsetof(smiSegAttr, mInitTrans),
        IDU_FT_SIZEOF(smiSegAttr, mInitTrans),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_TRANS",
        offsetof(stndrHeader4PerfV, mSegAttr ) +
            offsetof(smiSegAttr, mMaxTrans),
        IDU_FT_SIZEOF(smiSegAttr, mMaxTrans),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INITEXTENTS",
        offsetof(stndrHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mInitExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mInitExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NEXTEXTENTS",
        offsetof(stndrHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mNextExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mNextExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MINEXTENTS",
        offsetof(stndrHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mMinExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mMinExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAXEXTENTS",
        offsetof(stndrHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mMaxExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mMaxExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL  // for internal use
    }
};

iduFixedTableDesc  gDiskRTreeHeaderDesc=
{
    (SChar *)"X$DISK_RTREE_HEADER",
    stndrFT::buildRecordForDiskRTreeHeader,
    gDiskRTreeHeaderColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

//======================================================================
//  X$DISK_RTREE_STAT
//  disk index의 run-time statistic information을 위한 peformance view
//======================================================================

IDE_RC stndrFT::buildRecordForDiskRTreeStat(idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * /* aDumpObj */,
                                            iduFixedTableMemory * aMemory)
{
    smcTableHeader   * sCatTblHdr;
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sPtr;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    UInt               sTableType;
    smnIndexHeader   * sIndexCursor;
    stndrHeader      * sIndexHeader;
    stndrStat4PerfV    sIndexStat4PerfV;
    void             * sTrans;
    UInt               sIndexCnt;
    UInt               i;
    void             * sISavepoint = NULL;
    UInt               sDummy = 0;
    UInt               sState = 0;

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr = NULL;

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

        sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

        // disk table only
        if( sTableType != SMI_TABLE_DISK )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        if( smcTable::isDropedTable(sTableHeader) == ID_TRUE )
        {
            sCurPtr = sNxtPtr;
            continue;
        }//if

        sIndexCnt =  smcTable::getIndexCount(sTableHeader);

        if( sIndexCnt != 0  )
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
                                                              SMC_TABLE_LOCK( sTableHeader ))
                == IDE_SUCCESS )
            {
                sState = 2;
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                if( smcTable::isDropedTable(sTableHeader) == ID_TRUE )
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

                // lock을 대기하는 동안 index가 drop되었거나, 새로운 index가
                // 생성되었을 수 있으므로 정확한 index 수를 다시 구한다.
                // 뿐만 아니라, index cnt를 증가시킨 후 index를 생성하므로
                // index가 완료되지 못하면 index cnt가 감소하므로 다시 구해야 함.
                sIndexCnt = smcTable::getIndexCount(sTableHeader);

                for( i = 0; i < sIndexCnt; i++ )
                {
                    sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader, i );
                    IDE_TEST( sIndexCursor == NULL );

                    if( sIndexCursor->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    // BUG-30867 Discard 된 Tablespace에 속한 Index는 Skip되어야 함
                    if( sctTableSpaceMgr::hasState( sIndexCursor->mIndexSegDesc.mSpaceID,
                                                    SCT_SS_INVALID_DISK_TBS ) == ID_TRUE )
                    {
                        continue;
                    }

                    sIndexHeader = (stndrHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        continue;
                    }

                    idlOS::memset( &sIndexStat4PerfV, 0x00, ID_SIZEOF(stndrStat4PerfV) );

                    idlOS::memcpy( &sIndexStat4PerfV.mName,
                                   &sIndexCursor->mName,
                                   SMN_MAX_INDEX_NAME_SIZE+8);
                    sIndexStat4PerfV.mIndexID = sIndexCursor->mId;
                
                    sIndexStat4PerfV.mTreeLatchReadCount =
                        sIndexHeader->mSdnHeader.mLatch.getReadCount();
                    sIndexStat4PerfV.mTreeLatchWriteCount =
                        sIndexHeader->mSdnHeader.mLatch.getWriteCount();
                    sIndexStat4PerfV.mTreeLatchReadMissCount =
                        sIndexHeader->mSdnHeader.mLatch.getReadMisses();
                    sIndexStat4PerfV.mTreeLatchWriteMissCount =
                        sIndexHeader->mSdnHeader.mLatch.getWriteMisses();
                    sIndexStat4PerfV.mKeyCount    = sIndexHeader->mKeyCount;
                    sIndexStat4PerfV.mDMLStat     = sIndexHeader->mDMLStat;
                    sIndexStat4PerfV.mQueryStat   = sIndexHeader->mQueryStat;

                    sIndexStat4PerfV.mTreeMBR     = sIndexHeader->mTreeMBR;

                    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                          aMemory,
                                                          (void *)&sIndexStat4PerfV )
                              != IDE_SUCCESS);
                }//for
                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                sISavepoint )
                          != IDE_SUCCESS );
            }
            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                          sISavepoint )
                      != IDE_SUCCESS );
        }// if 인덱스가 있으면
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

static iduFixedTableColDesc  gDiskRTreeStatColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(stndrStat4PerfV, mName ),
        SMN_MAX_INDEX_NAME_SIZE,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(stndrStat4PerfV, mIndexID ),
        IDU_FT_SIZEOF(stndrStat4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_READ_COUNT",
        offsetof(stndrStat4PerfV, mTreeLatchReadCount),
        IDU_FT_SIZEOF(stndrStat4PerfV, mTreeLatchReadCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_WRITE_COUNT",
        offsetof(stndrStat4PerfV, mTreeLatchWriteCount),
        IDU_FT_SIZEOF(stndrStat4PerfV, mTreeLatchWriteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_READ_MISS_COUNT",
        offsetof(stndrStat4PerfV, mTreeLatchReadMissCount),
        IDU_FT_SIZEOF(stndrStat4PerfV, mTreeLatchReadMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_WRITE_MISS_COUNT",
        offsetof(stndrStat4PerfV, mTreeLatchWriteMissCount),
        IDU_FT_SIZEOF(stndrStat4PerfV, mTreeLatchWriteMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COUNT",
        offsetof(stndrStat4PerfV, mKeyCount),
        IDU_FT_SIZEOF(stndrStat4PerfV, mKeyCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) + offsetof(stndrStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(stndrStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_PROPAGATE_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) + offsetof(stndrStatistic, mKeyPropagateCount),
        IDU_FT_SIZEOF(stndrStatistic, mKeyPropagateCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FOLLOW_RIGHT_LINK_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) + offsetof(stndrStatistic, mFollowRightLinkCount),
        IDU_FT_SIZEOF(stndrStatistic, mFollowRightLinkCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"OP_RETRY_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) + offsetof(stndrStatistic, mOpRetryCount),
        IDU_FT_SIZEOF(stndrStatistic, mOpRetryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) + offsetof(stndrStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(stndrStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_INDEX_PAGE_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) +
        offsetof(stndrStatistic, mIndexPage) +
        offsetof(stndrPageStat, mGetPageCount),
        IDU_FT_SIZEOF(stndrPageStat, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_INDEX_PAGE_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) +
        offsetof(stndrStatistic, mIndexPage) +
        offsetof(stndrPageStat, mReadPageCount),
        IDU_FT_SIZEOF(stndrPageStat, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_META_PAGE_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) +
        offsetof(stndrStatistic, mMetaPage) +
        offsetof(stndrPageStat, mGetPageCount),
        IDU_FT_SIZEOF(stndrPageStat, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_META_PAGE_COUNT_BY_DML",
        offsetof(stndrStat4PerfV, mDMLStat) +
        offsetof(stndrStatistic, mMetaPage) +
        offsetof(stndrPageStat, mReadPageCount),
        IDU_FT_SIZEOF(stndrPageStat, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_RANGE_COUNT_BY_QUERY",
        offsetof(stndrStat4PerfV, mQueryStat) + offsetof(stndrStatistic, mKeyRangeCount),
        IDU_FT_SIZEOF(stndrStatistic, mKeyRangeCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"ROW_FILTER_COUNT_BY_QUERY",
        offsetof(stndrStat4PerfV, mQueryStat) + offsetof(stndrStatistic, mRowFilterCount),
        IDU_FT_SIZEOF(stndrStatistic, mRowFilterCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_INDEX_PAGE_COUNT_BY_QUERY",
        offsetof(stndrStat4PerfV, mQueryStat) +
        offsetof(stndrStatistic, mIndexPage) +
        offsetof(stndrPageStat, mGetPageCount),
        IDU_FT_SIZEOF(stndrPageStat, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_INDEX_PAGE_COUNT_BY_QUERY",
        offsetof(stndrStat4PerfV, mQueryStat) +
        offsetof(stndrStatistic, mIndexPage) +
        offsetof(stndrPageStat, mReadPageCount),
        IDU_FT_SIZEOF(stndrPageStat, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_MBR_MIN_X",
        offsetof(stndrStat4PerfV, mTreeMBR ) + offsetof(stdMBR, mMinX),
        IDU_FT_SIZEOF(stdMBR, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_MBR_MIN_Y",
        offsetof(stndrStat4PerfV, mTreeMBR ) + offsetof(stdMBR, mMinY),
        IDU_FT_SIZEOF(stdMBR, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_MBR_MAX_X",
        offsetof(stndrStat4PerfV, mTreeMBR ) + offsetof(stdMBR, mMaxX),
        IDU_FT_SIZEOF(stdMBR, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_MBR_MAX_Y",
        offsetof(stndrStat4PerfV, mTreeMBR ) + offsetof(stdMBR, mMaxY),
        IDU_FT_SIZEOF(stdMBR, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL
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

iduFixedTableDesc  gDiskRTreeStatDesc=
{
    (SChar *)"X$DISK_RTREE_STAT",
    stndrFT::buildRecordForDiskRTreeStat,
    gDiskRTreeStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};
