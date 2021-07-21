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
 * $Id: sdnbFT.cpp 19860 2007-02-07 02:09:39Z leekmo $
 *
 * Description
 *
 *   PROJ-1618
 *   Disk BTree Index 의 FT를 위한 함수
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>

#include <sdnFT.h>
#include <sdnbFT.h>
#include <sdn.h>
#include <sdnReq.h>
#include <sdp.h>
#include <sdb.h>
#include <smiFixedTable.h>


/***********************************************************************
 * Description
 *
 *   D$DISK_INDEX_BTREE_CTS
 *   : Disk BTREE INDEX의 CTS을 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_INDEX_BTREE_CTS Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskIndexBTreeCTSlotColDesc[]=
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
// D$DISK_INDEX_BTREE_CTS Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskIndexBTreeCTSlotTableDesc =
{
    (SChar *)"D$DISK_INDEX_BTREE_CTS",
    sdnFT::buildRecordCTS,
    gDumpDiskIndexBTreeCTSlotColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_INDEX_BTREE_CTS Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC sdnFT::buildRecordCTS( idvSQL              * /*aStatistics*/,
                              void                * aHeader,
                              void                * aDumpObj,
                              iduFixedTableMemory * aMemory )
{
    sdnbHeader       * sIdxHdr = NULL;
    idBool             sIsLatched;
    scPageID           sRootPID;
    ULong              sPageSeq = 0;
    sdrMtx             sMiniTx;
    idBool             sIsTxBegin;
    UInt               sMiniTxLogType;

    sIsLatched = ID_FALSE;
    sIsTxBegin = ID_FALSE;

    // Temp BTree에서 복사
    sMiniTxLogType = SM_DLOG_ATTR_DEFAULT;
    
    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );
    
    //------------------------------------------
    // Get Disk BTree Index Header
    //------------------------------------------
    
    sIdxHdr = (sdnbHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr != NULL )
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mLatch.lockRead( NULL, /* idvSQL* */
                                      NULL /* idvWeArgs* */) != IDE_SUCCESS );
        sIsLatched = ID_TRUE;

        // Get Root Page ID
        sRootPID = sIdxHdr->mRootNode;

        //------------------------------------------
        // Set Root Page Record
        //------------------------------------------
        
        if( sRootPID == SD_NULL_PID )
        {
            // 구축할 Data가 없음
            // Nothing To Do
        }
        else
        {
            // Mini Transaction Begin
            IDE_TEST( sdrMiniTrans::begin( NULL, /* idvSQL* */
                                           &sMiniTx,
                                           NULL,
                                           SDR_MTX_NOLOGGING,
                                           ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                           sMiniTxLogType )
                      != IDE_SUCCESS);
            sIsTxBegin = ID_TRUE;
            
            IDE_TEST( traverseBuildCTS( NULL, /* idvSQL* */
                                        aHeader,
                                        aMemory,
                                        sIdxHdr,
                                        & sMiniTx,
                                        0, // Depth
                                        -1, // Nth Sibling
                                        sIdxHdr->mIndexTSID,
                                        sRootPID,
                                        &sPageSeq ) // My PID
                      != IDE_SUCCESS );

            // Mini Transaction Commit
            sIsTxBegin = ID_FALSE;
            IDE_TEST(sdrMiniTrans::commit(&sMiniTx) != IDE_SUCCESS);
            
        }

        //------------------------------------------
        // Finalize
        //------------------------------------------
        
        // unlatch tree latch
        sIsLatched = ID_FALSE;
        IDE_TEST( sIdxHdr->mLatch.unlock( ) != IDE_SUCCESS );
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
        (void) sIdxHdr->mLatch.unlock( );
    }
    if ( sIsTxBegin == ID_TRUE )
    {
        sIsTxBegin = ID_FALSE;
        (void) sdrMiniTrans::rollback(&sMiniTx);
    }
    
    return IDE_FAILURE;    
}
    
IDE_RC sdnFT::traverseBuildCTS( idvSQL*               aStatistics,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory,
                                sdnbHeader          * aIdxHdr,
                                sdrMtx              * aMiniTx,
                                UShort                aDepth,
                                SShort                aNthSibling,
                                scSpaceID             aSpaceID,
                                scPageID              aMyPID,
                                ULong               * aPageSeq )
{
    SInt               i;
    SInt               sSlotCnt;
    idBool             sIsLeaf;
    sdnDumpCTS         sDumpCTS; 
    sdnCTL           * sCTL;
    sdnCTS           * sCTS;
#ifndef COMPILE_64BIT
    ULong              sSCN;
#endif
    SChar              sStrCommitSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar              sStrNxtCommitSCN[ SM_SCN_STRING_LENGTH + 1];
    sdrSavePoint       sSP;
    idBool             sTrySuccess;
    sdpPhyPageHdr    * sPageHdr;
    sdnbNodeHdr      * sIdxNodeHdr;
    sdnbIKey         * sIKey;
    UChar            * sSlotDirPtr;
    scPageID           sRightChildPID;
    idBool             sIsLastLimitResult;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aIdxHdr != NULL );
    IDE_ERROR( aMiniTx != NULL );
    IDE_ERROR( aNthSibling >= -1 );
    IDE_ERROR( aSpaceID != 0 );
    IDE_ERROR( aMyPID != SD_NULL_PID );

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

    //------------------------------------------
    // Initialize
    //------------------------------------------

    idlOS::memset( & sDumpCTS, 0x00, ID_SIZEOF( sdnDumpCTS ) );

    sdrMiniTrans::setSavePoint( aMiniTx, &sSP );
    
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aMyPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMiniTx,
                                          (UChar**)&sPageHdr,
                                          &sTrySuccess,
                                          NULL )   
              != IDE_SUCCESS );

    sIdxNodeHdr = (sdnbNodeHdr*)
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
        
        for( i = 0; i < sdnIndexCTL::getCount(sCTL); i++ )
        {
            sCTS = sdnIndexCTL::getCTS( sCTL, i );
            
            sDumpCTS.mMyPID = aMyPID;
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

            sDumpCTS.mTSSlotPID = sCTS->mTSSlotPID;
            sDumpCTS.mTSSlotNum   = sCTS->mTSSlotNum;
            sDumpCTS.mRefCnt = sCTS->mRefCnt;
            sDumpCTS.mRefKey1 = sCTS->mRefKey[0];
            sDumpCTS.mRefKey2 = sCTS->mRefKey[1];
            sDumpCTS.mRefKey3 = sCTS->mRefKey[2];

            // KEY CACHE의 크기가 커지면 위의 코드도 수정되어야 함.
            IDE_ASSERT( SDN_CTS_MAX_KEY_CACHE == 3 );
            
            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *) & sDumpCTS )
                     != IDE_SUCCESS);
        }
    }
    else
    {
        //------------------------------------------
        // Left Most Child
        //------------------------------------------
        IDE_TEST( traverseBuildCTS( aStatistics,
                                    aHeader,
                                    aMemory,
                                    aIdxHdr,
                                    aMiniTx,
                                    aDepth + 1, // Depth
                                    -1, // Nth Sibling
                                    aSpaceID,
                                    sIdxNodeHdr->mLeftmostChild,
                                    aPageSeq )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Other Child
        //------------------------------------------
        for ( i = 0; i < sSlotCnt; i++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               (UChar**)&sIKey )
                      != IDE_SUCCESS );
            
            SDNB_GET_RIGHT_CHILD_PID( &sRightChildPID, sIKey );
            IDE_TEST( traverseBuildCTS( aStatistics,
                                        aHeader,
                                        aMemory,
                                        aIdxHdr,
                                        aMiniTx,
                                        aDepth + 1, 
                                        i, 
                                        aSpaceID,
                                        sRightChildPID,
                                        aPageSeq ) 
                      != IDE_SUCCESS );
        }
    }
    
    //------------------------------------------
    // Finalize
    //------------------------------------------
    
    sdrMiniTrans::releaseLatchToSP( aMiniTx, &sSP );

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description
 *
 *   D$DISK_INDEX_BTREE_STRUCTURE
 *   : Disk BTREE INDEX의 Page Tree 구조 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_INDEX_BTREE_STRUCTURE Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskBTreeStructureColDesc[]=
{
    {
        (SChar*)"DEPTH",
        offsetof(sdnbDumpTreePage, mDepth ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HEIGHT",
        offsetof(sdnbDumpTreePage, mHeight ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mHeight ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SIBLING",
        offsetof(sdnbDumpTreePage, mNthSibling ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mNthSibling ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(sdnbDumpTreePage, mIsLeaf ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MY_PAGEID",
        offsetof(sdnbDumpTreePage, mMyPID ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mMyPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PAGEID",
        offsetof(sdnbDumpTreePage, mNextPID ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mNextPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREV_PAGEID",
        offsetof(sdnbDumpTreePage, mPrevPID ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mPrevPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_SEQ",
        offsetof(sdnbDumpTreePage, mPageSeq ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mPageSeq ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_CNT",
        offsetof(sdnbDumpTreePage, mSlotCount ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mSlotCount ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SMO_NO",
        offsetof(sdnbDumpTreePage, mSmoNo ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mSmoNo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_FREE_SIZE",
        offsetof(sdnbDumpTreePage, mTotalFreeSize ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mTotalFreeSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGEID",
        offsetof(sdnbDumpTreePage, mParentPID ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mParentPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LMOST_CHILD_PAGEID",
        offsetof(sdnbDumpTreePage, mLeftmostChildPID ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mLeftmostChildPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNLIMITED_KEY_COUNT",
        offsetof(sdnbDumpTreePage, mUnlimitedKeyCount ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mUnlimitedKeyCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_DEAD_KEY_SIZE",
        offsetof(sdnbDumpTreePage, mTotalDeadKeySize ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mTotalDeadKeySize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TBK_COUNT",
        offsetof(sdnbDumpTreePage, mTBKCount ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mTBKCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_STATE",
        offsetof(sdnbDumpTreePage, mPageState ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mPageState ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CTL_SIZE",
        offsetof(sdnbDumpTreePage, mCTLayerSize ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mCTLayerSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CTS_USED_COUNT",
        offsetof(sdnbDumpTreePage, mCTSlotUsedCount ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mCTSlotUsedCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(sdnbDumpTreePage, mIsConsistent ),
        IDU_FT_SIZEOF(sdnbDumpTreePage, mIsConsistent ),
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
// D$DISK_INDEX_BTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskBTreeStructureTableDesc =
{
    (SChar *)"D$DISK_INDEX_BTREE_STRUCTURE",
    sdnbFT::buildRecordTreeStructure,
    gDumpDiskBTreeStructureColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_INDEX_BTREE_HEADER Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC sdnbFT::buildRecordTreeStructure( idvSQL              * /*aStatistics*/,
                                         void                * aHeader,
                                         void                * aDumpObj,
                                         iduFixedTableMemory * aMemory )
{
    sdnbHeader       * sIdxHdr = NULL;
    idBool             sIsLatched;
    scPageID           sRootPID;
    ULong              sPageSeq = 0;

    sIsLatched = ID_FALSE;
    
    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Disk BTree Index Header
    //------------------------------------------
    
    sIdxHdr = (sdnbHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr != NULL )
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mLatch.lockRead( NULL, /* idvSQL * */
                                      NULL /* idvWeArgs */ ) != IDE_SUCCESS );
        sIsLatched = ID_TRUE;

        // Get Root Page ID
        sRootPID = sIdxHdr->mRootNode;

        //------------------------------------------
        // Set Root Page Record
        //------------------------------------------
        
        if( sRootPID == SD_NULL_PID )
        {
            // 구축할 Data가 없음
            // Nothing To Do
        }
        else
        {
            IDE_TEST( traverseBuildTreePage( NULL, /* idvSQL* */
                                             aHeader,
                                             aMemory,
                                             0,            // Depth
                                             -1,           // Nth Sibling
                                             sIdxHdr->mIndexTSID,
                                             sRootPID,     // My PID
                                             &sPageSeq,    // Page Sequence
                                             SD_NULL_PID ) // Parent PID
                      != IDE_SUCCESS );
        }

        //------------------------------------------
        // Finalize
        //------------------------------------------
        
        // unlatch tree latch
        sIsLatched = ID_FALSE;
        IDE_TEST( sIdxHdr->mLatch.unlock( ) != IDE_SUCCESS );
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
        (void) sIdxHdr->mLatch.unlock( );
    }
    
    return IDE_FAILURE;    
}
    
IDE_RC sdnbFT::traverseBuildTreePage( idvSQL*               aStatistics,
                                      void                * aHeader,
                                      iduFixedTableMemory * aMemory,
                                      UShort                aDepth,
                                      SShort                aNthSibling,
                                      scSpaceID             aSpaceID,
                                      scPageID              aMyPID,
                                      ULong               * aPageSeq,                 
                                      scPageID              aParentPID )
{
    SInt               i;
    sdnbDumpTreePage   sDumpTreePage; // BUG-16648
    idBool             sIsFixed = ID_FALSE;
    sdpPhyPageHdr    * sPageHdr;
    UChar            * sSlotDirPtr;
    sdnbNodeHdr      * sIdxNodeHdr;
    sdnCTL           * sCTL;
    sdnbIKey*          sIKey;
    scPageID           sRightChildPID;
    idBool             sIsLastLimitResult;
    idBool             sTrySuccess;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aNthSibling >= -1 );
    IDE_ERROR( aSpaceID != 0 );
    IDE_ERROR( aMyPID != SD_NULL_PID );

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

    //------------------------------------------
    // Initialize
    //------------------------------------------
    
    idlOS::memset( & sDumpTreePage, 0x00, ID_SIZEOF( sdnbDumpTreePage ) );
        
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aMyPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL,
                                          (UChar**)&sPageHdr,
                                          &sTrySuccess,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );

    sIsFixed = ID_TRUE;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);

    sIdxNodeHdr = (sdnbNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*) sPageHdr );
    
    sCTL = sdnIndexCTL::getCTL( sPageHdr );

    *aPageSeq += 1;
    
    //------------------------------------------
    // Build My Record
    //------------------------------------------

    sDumpTreePage.mDepth             = aDepth;
    sDumpTreePage.mHeight            = sIdxNodeHdr->mHeight;
    sDumpTreePage.mNthSibling        = aNthSibling;
    sDumpTreePage.mIsLeaf            =
        ( sDumpTreePage.mHeight > 0 ) ? 'F' : 'T';
    sDumpTreePage.mMyPID             = aMyPID;
    sDumpTreePage.mNextPID           = sPageHdr->mListNode.mNext;
    sDumpTreePage.mPrevPID           = sPageHdr->mListNode.mPrev;
    sDumpTreePage.mPageSeq           = *aPageSeq;
    sDumpTreePage.mSlotCount         = sdpSlotDirectory::getCount(sSlotDirPtr);
    sDumpTreePage.mSmoNo             = sdpPhyPage::getIndexSMONo( sPageHdr );
    sDumpTreePage.mTotalFreeSize     = sdpPhyPage::getTotalFreeSize( sPageHdr );
    sDumpTreePage.mParentPID         = aParentPID; 
    sDumpTreePage.mLeftmostChildPID  = sIdxNodeHdr->mLeftmostChild;
    sDumpTreePage.mTBKCount = sIdxNodeHdr->mTBKCount;
    
    if( sDumpTreePage.mHeight == 0 )
    {
        sDumpTreePage.mUnlimitedKeyCount = sIdxNodeHdr->mUnlimitedKeyCount;
        sDumpTreePage.mTotalDeadKeySize  = sIdxNodeHdr->mTotalDeadKeySize;
        switch( sIdxNodeHdr->mState )
        {
            case SDNB_IN_INIT:
                sDumpTreePage.mPageState = 'I';
                break;
            case SDNB_IN_USED:
                sDumpTreePage.mPageState = 'U';
                break;
            case SDNB_IN_EMPTY_LIST:
                sDumpTreePage.mPageState = 'E';
                break;
            case SDNB_IN_FREE_LIST:
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
    
    sDumpTreePage.mIsConsistent =
        (sPageHdr->mIsConsistent == SDP_PAGE_CONSISTENT)? 'T' : 'F';
    
    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *) & sDumpTreePage )
             != IDE_SUCCESS);

    //------------------------------------------
    // Build Child Record
    //------------------------------------------

    if ( sDumpTreePage.mHeight > 0 )
    {
        //------------------------------------------
        // Left Most Child
        //------------------------------------------
        
        IDE_TEST( traverseBuildTreePage( aStatistics,
                                         aHeader,
                                         aMemory,
                                         aDepth + 1, // Depth
                                         -1, // Nth Sibling
                                         aSpaceID,
                                         sDumpTreePage.mLeftmostChildPID, // My
                                         aPageSeq,
                                         sDumpTreePage.mMyPID ) // Parent PID
                  != IDE_SUCCESS );

        //------------------------------------------
        // Other Child
        //------------------------------------------

        for ( i = 0; i < sDumpTreePage.mSlotCount; i++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               (UChar**)&sIKey )
                      != IDE_SUCCESS );
            
            SDNB_GET_RIGHT_CHILD_PID( &sRightChildPID, sIKey );
            IDE_TEST( traverseBuildTreePage( aStatistics,
                                             aHeader,
                                             aMemory,
                                             aDepth + 1, 
                                             i, 
                                             aSpaceID,
                                             sRightChildPID,
                                             aPageSeq,
                                             sDumpTreePage.mMyPID ) 
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Leaf Node
        // Nothing To Do
        IDE_DASSERT( sDumpTreePage.mHeight == 0 );
    }
    
    
    //------------------------------------------
    // Finalize
    //------------------------------------------
    
    sIsFixed = ID_FALSE;
    IDE_TEST( sdbBufferMgr::releasePage(aStatistics,
                                        (UChar*)sPageHdr )
              != IDE_SUCCESS );

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
 *   D$DISK_INDEX_BTREE_KEY
 *   : Disk BTREE INDEX의 KEY 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_INDEX_BTREE_KEY Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskBTreeKeyColDesc[]=
{
    {
        (SChar*)"MY_PAGEID",
        offsetof(sdnbDumpKey, mMyPID ),
        IDU_FT_SIZEOF(sdnbDumpKey, mMyPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_SEQ",
        offsetof(sdnbDumpKey, mPageSeq ),
        IDU_FT_SIZEOF(sdnbDumpKey, mPageSeq ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DEPTH",
        offsetof(sdnbDumpKey, mDepth ),
        IDU_FT_SIZEOF(sdnbDumpKey, mDepth ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_LEAF",
        offsetof(sdnbDumpKey, mIsLeaf ),
        IDU_FT_SIZEOF(sdnbDumpKey, mIsLeaf ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARENT_PAGEID",
        offsetof(sdnbDumpKey, mParentPID ),
        IDU_FT_SIZEOF(sdnbDumpKey, mParentPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SIBLING",
        offsetof(sdnbDumpKey, mNthSibling ),
        IDU_FT_SIZEOF(sdnbDumpKey, mNthSibling ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(sdnbDumpKey, mNthSlot ),
        IDU_FT_SIZEOF(sdnbDumpKey, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_COLUMN",
        offsetof(sdnbDumpKey, mNthColumn ),
        IDU_FT_SIZEOF(sdnbDumpKey, mNthColumn ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_LENGTH",
        offsetof(sdnbDumpKey, mColumnLength ),
        IDU_FT_SIZEOF(sdnbDumpKey, mColumnLength ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"VALUE24B",
        offsetof(sdnbDumpKey, mValue ),
        IDU_FT_SIZEOF(sdnbDumpKey, mValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CHILD_PAGEID",
        offsetof(sdnbDumpKey, mChildPID ),
        IDU_FT_SIZEOF(sdnbDumpKey, mChildPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_PAGEID",
        offsetof(sdnbDumpKey, mRowPID ),
        IDU_FT_SIZEOF(sdnbDumpKey, mRowPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_SLOTNUM",
        offsetof(sdnbDumpKey, mRowSlotNum ),
        IDU_FT_SIZEOF(sdnbDumpKey, mRowSlotNum ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof(sdnbDumpKey, mState ),
        IDU_FT_SIZEOF(sdnbDumpKey, mState ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DUPLICATED",
        offsetof(sdnbDumpKey, mDuplicated ),
        IDU_FT_SIZEOF(sdnbDumpKey, mDuplicated ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_CTS_NO",
        offsetof(sdnbDumpKey, mCreateCTS ),
        IDU_FT_SIZEOF(sdnbDumpKey, mCreateCTS ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_CHAINED",
        offsetof(sdnbDumpKey, mCreateChained ),
        IDU_FT_SIZEOF(sdnbDumpKey, mCreateChained ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_CTS_NO",
        offsetof(sdnbDumpKey, mLimitCTS ),
        IDU_FT_SIZEOF(sdnbDumpKey, mLimitCTS ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_CHAINED",
        offsetof(sdnbDumpKey, mLimitChained ),
        IDU_FT_SIZEOF(sdnbDumpKey, mLimitChained ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TB_TYPE",
        offsetof(sdnbDumpKey, mTxBoundType ),
        IDU_FT_SIZEOF(sdnbDumpKey, mTxBoundType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_TSS_SID",
        offsetof(sdnbDumpKey, mCreateTSS ),
        IDU_FT_SIZEOF(sdnbDumpKey, mCreateTSS ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_TSS_SID",
        offsetof(sdnbDumpKey, mLimitTSS ),
        IDU_FT_SIZEOF(sdnbDumpKey, mLimitTSS ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_COMMIT_SCN",
        offsetof(sdnbDumpKey, mCreateCSCN ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"LIMIT_COMMIT_SCN",
        offsetof(sdnbDumpKey, mLimitCSCN ),
        16,
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
// D$DISK_INDEX_BTREE_KEY Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskBTreeKeyTableDesc =
{
    (SChar *)"D$DISK_INDEX_BTREE_KEY",
    sdnbFT::buildRecordKey,
    gDumpDiskBTreeKeyColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_INDEX_BTREE_HEADER Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC sdnbFT::buildRecordKey( idvSQL              * /*aStatistics*/,
                               void                * aHeader,
                               void                * aDumpObj,
                               iduFixedTableMemory * aMemory )
{
    sdnbHeader       * sIdxHdr = NULL;
    idBool             sIsLatched;
    scPageID           sRootPID;
    ULong              sPageSeq = 0;
    sdrMtx             sMiniTx;
    idBool             sIsTxBegin;
    UInt               sMiniTxLogType;

    sIsLatched = ID_FALSE;
    sIsTxBegin = ID_FALSE;

    // Temp BTree에서 복사
    sMiniTxLogType = SM_DLOG_ATTR_DEFAULT;
    
    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );
    
    IDE_TEST_RAISE( ((smnIndexHeader*)aDumpObj)->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
                    ERR_INVALID_DUMP_OBJECT );
    
    //------------------------------------------
    // Get Disk BTree Index Header
    //------------------------------------------
    
    sIdxHdr = (sdnbHeader*) (((smnIndexHeader*) aDumpObj)->mHeader);

    if( sIdxHdr != NULL )
    {
        // Set Tree Latch
        IDE_TEST( sIdxHdr->mLatch.lockRead( NULL, /* idvSQL* */
                                      NULL /* idvWeArgs* */) != IDE_SUCCESS );
        sIsLatched = ID_TRUE;

        // Get Root Page ID
        sRootPID = sIdxHdr->mRootNode;

        //------------------------------------------
        // Set Root Page Record
        //------------------------------------------
        
        if( sRootPID == SD_NULL_PID )
        {
            // 구축할 Data가 없음
            // Nothing To Do
        }
        else
        {
            // Mini Transaction Begin
            IDE_TEST( sdrMiniTrans::begin( NULL, /* idvSQL* */
                                           &sMiniTx,
                                           NULL,
                                           SDR_MTX_NOLOGGING,
                                           ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                           sMiniTxLogType )
                      != IDE_SUCCESS);
            sIsTxBegin = ID_TRUE;
            
            IDE_TEST( traverseBuildKey( NULL, /* idvSQL* */
                                        aHeader,
                                        aMemory,
                                        sIdxHdr,
                                        & sMiniTx,
                                        0, // Depth
                                        -1, // Nth Sibling
                                        sIdxHdr->mIndexTSID,
                                        sRootPID, // My PID
                                        &sPageSeq,
                                        SD_NULL_PID ) // Parent PID
                      != IDE_SUCCESS );

            // Mini Transaction Commit
            sIsTxBegin = ID_FALSE;
            IDE_TEST(sdrMiniTrans::commit(&sMiniTx) != IDE_SUCCESS);
            
        }

        //------------------------------------------
        // Finalize
        //------------------------------------------
        
        // unlatch tree latch
        sIsLatched = ID_FALSE;
        IDE_TEST( sIdxHdr->mLatch.unlock() != IDE_SUCCESS );
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
        (void)sIdxHdr->mLatch.unlock( );
    }
    if ( sIsTxBegin == ID_TRUE )
    {
        sIsTxBegin = ID_FALSE;
        (void) sdrMiniTrans::rollback(&sMiniTx);
    }
    
    return IDE_FAILURE;    
}
    
IDE_RC sdnbFT::traverseBuildKey( idvSQL*               aStatistics,
                                 void                * aHeader,
                                 iduFixedTableMemory * aMemory,
                                 sdnbHeader          * aIdxHdr,
                                 sdrMtx              * aMiniTx,
                                 UShort                aDepth,
                                 SShort                aNthSibling,
                                 scSpaceID             aSpaceID,
                                 scPageID              aMyPID,
                                 ULong               * aPageSeq,
                                 scPageID              aParentPID )
{
    SInt   i, j;
    SInt   sSlotCnt;
    idBool sIsLeaf;
    
    sdnbDumpKey         sDumpKey; 
    
    sdrSavePoint        sSP;
    idBool              sTrySuccess;
    
    sdpPhyPageHdr     * sPageHdr;
    UChar             * sSlotDirPtr;
    sdnbNodeHdr       * sIdxNodeHdr;
    
    sdnbIKey          * sIKey;
    sdnbLKey          * sLKey;
    sdnbKeyInfo         sKeyInfo;

    UChar             * sKeyPtr;
    UInt                sOffset;
    sdnbColumn*         sIndexColumn;
    UChar               sValueBuffer[SM_DUMP_VALUE_BUFFER_SIZE];
    UInt                sValueLength;
    IDE_RC              sReturn;
    idBool              sIsLastLimitResult;

#ifndef COMPILE_64BIT
    ULong               sSCN;
#endif
    smSCN               sCommitSCN;
    SChar               sStrCreateCSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar               sStrLimitCSCN[ SM_SCN_STRING_LENGTH + 1];
    scPageID            sRightChildPID;
    sdnbColLenInfoList *sColLenInfoList;
    UInt                sColumnLength;
    UInt                sColumnHeaderLength;
    UInt                sColumnValueLength;
    
    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );
    IDE_ERROR( aIdxHdr != NULL );
    IDE_ERROR( aMiniTx != NULL );
    IDE_ERROR( aNthSibling >= -1 );
    IDE_ERROR( aSpaceID != 0 );
    IDE_ERROR( aMyPID != SD_NULL_PID );

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

    //------------------------------------------
    // Initialize
    //------------------------------------------

    idlOS::memset( & sDumpKey, 0x00, ID_SIZEOF( sdnbDumpKey ) );

    sdrMiniTrans::setSavePoint( aMiniTx, &sSP );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aMyPID,
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMiniTx,
                                          (UChar**)&sPageHdr,
                                          & sTrySuccess,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );

    sIdxNodeHdr = (sdnbNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*) sPageHdr );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
    sSlotCnt    = sdpSlotDirectory::getCount(sSlotDirPtr);
    sIsLeaf = ( sIdxNodeHdr->mHeight == 0 ) ? ID_TRUE : ID_FALSE;
    sColLenInfoList = &aIdxHdr->mColLenInfoList;
    
    *aPageSeq += 1;
    
    //------------------------------------------
    // Build My Record
    //------------------------------------------

    sDumpKey.mMyPID = aMyPID;
    sDumpKey.mPageSeq = *aPageSeq;
    sDumpKey.mDepth = aDepth;
    sDumpKey.mParentPID = aParentPID; 
    sDumpKey.mNthSibling = aNthSibling;
    sDumpKey.mIsLeaf = ( sIdxNodeHdr->mHeight == 0 ) ? 'T' : 'F';
    
    for ( i = 0; i < sSlotCnt; i++ )
    {
        if ( sIsLeaf == ID_TRUE )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               (UChar**)&sLKey )
                      != IDE_SUCCESS );
            
            sDumpKey.mNthSlot = i;
            sDumpKey.mChildPID = SD_NULL_PID;
            SDNB_LKEY_TO_KEYINFO( sLKey, sKeyInfo );
            sDumpKey.mRowPID = sKeyInfo.mRowPID;
            sDumpKey.mRowSlotNum = sKeyInfo.mRowSlotNum;

            switch( SDNB_GET_STATE(sLKey) )
            {
                case SDNB_KEY_UNSTABLE:
                    sDumpKey.mState = 'U';
                    break;
                case SDNB_KEY_STABLE:
                    sDumpKey.mState = 'S';
                    break;
                case SDNB_KEY_DELETED:
                    sDumpKey.mState = 'd';
                    break;
                case SDNB_KEY_DEAD:
                    sDumpKey.mState = 'D';
                    break;
                default:
                    sDumpKey.mState = '-';
                    break;
            }

            sDumpKey.mDuplicated =
                (SDNB_GET_DUPLICATED( sLKey ) == SDNB_DUPKEY_YES) ? 'Y' : 'N';
            sDumpKey.mCreateCTS  = SDNB_GET_CCTS_NO( sLKey );
            sDumpKey.mCreateChained = 'N';
            sDumpKey.mLimitCTS  = SDNB_GET_LCTS_NO( sLKey );
            sDumpKey.mLimitChained = 'N';
            
            sDumpKey.mTxBoundType =
                (SDNB_GET_TB_TYPE( sLKey ) == SDNB_KEY_TB_CTS) ? 'T' : 'K';

            if( SDNB_GET_TB_TYPE( sLKey ) == SDNB_KEY_TB_KEY )
            {
                SDNB_GET_TBK_CTSS( ((sdnbLKeyEx*)sLKey), &sDumpKey.mCreateTSS );
                SDNB_GET_TBK_LTSS( ((sdnbLKeyEx*)sLKey), &sDumpKey.mLimitTSS );

                SDNB_GET_TBK_CSCN( ((sdnbLKeyEx*)sLKey), &sCommitSCN );
                idlOS::memset( sStrCreateCSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
#ifdef COMPILE_64BIT
                idlOS::sprintf( (SChar*)sStrCreateCSCN, "%"ID_XINT64_FMT, sCommitSCN );
#else
                sSCN  = (ULong)sCommitSCN.mHigh << 32;
                sSCN |= (ULong)sCommitSCN.mLow;
                idlOS::snprintf( (SChar*)sStrCreateCSCN, SM_SCN_STRING_LENGTH,
                                 "%"ID_XINT64_FMT, sSCN );
#endif
                SDNB_GET_TBK_LSCN( ((sdnbLKeyEx*)sLKey), &sCommitSCN );
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
            
            sDumpKey.mCreateCSCN = sStrCreateCSCN;
            sDumpKey.mLimitCSCN = sStrLimitCSCN;
            
            sKeyPtr = SDNB_LKEY_KEY_PTR(sLKey);
        }
        else
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               (UChar**)&sIKey )
                      != IDE_SUCCESS );

            sDumpKey.mNthSlot = i;
            SDNB_GET_RIGHT_CHILD_PID( &sDumpKey.mChildPID, sIKey );
            SDNB_IKEY_TO_KEYINFO( sIKey, sKeyInfo );
            sDumpKey.mRowPID = sKeyInfo.mRowPID;
            sDumpKey.mRowSlotNum = sKeyInfo.mRowSlotNum;
            sDumpKey.mState = '-';
            sDumpKey.mDuplicated = '-';
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

            sKeyPtr = SDNB_IKEY_KEY_PTR(sIKey);
        }
        

        //------------------------------
        // BUG-16805 Column별 Value String 생성
        //------------------------------
       
        //PROJ-1872 Disk index 저장 구조 최적화 
        //Column읽는 메커니즘 수정
        sOffset = 0;

        for ( sIndexColumn = aIdxHdr->mColumns, j = 0;
              sIndexColumn < aIdxHdr->mColumnFence;
              sIndexColumn++, j++ )
        {
            sDumpKey.mNthColumn = j;
            idlOS::memset( sDumpKey.mValue, 0x00, SM_DUMP_VALUE_LENGTH );
            sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
            
            IDE_TEST(
                sdnbBTree::columnValue2String( sIndexColumn,
                                               sColLenInfoList->mColLenInfo[ j ],
                                               (UChar*)sKeyPtr + sOffset,
                                               (UChar*)sValueBuffer,
                                               &sValueLength,
                                               &sReturn )
                != IDE_SUCCESS );
            idlOS::memcpy( sDumpKey.mValue,
                           sValueBuffer,
                           ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                           SM_DUMP_VALUE_LENGTH : sValueLength );

            sColumnLength  = sdnbBTree::getColumnLength( 
                                    sColLenInfoList->mColLenInfo[ j ],
                                    ((UChar*)sKeyPtr) + sOffset,
                                    &sColumnHeaderLength,
                                    &sColumnValueLength,
                                    NULL );
            sDumpKey.mColumnLength = sColumnLength ;
            sOffset += sColumnLength ;

            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *) & sDumpKey )
                     != IDE_SUCCESS);
        }
    }

    //------------------------------------------
    // Build Child Record
    //------------------------------------------

    if ( sIsLeaf == ID_FALSE )
    {
        //------------------------------------------
        // Left Most Child
        //------------------------------------------
        
        IDE_TEST( traverseBuildKey( aStatistics,
                                    aHeader,
                                    aMemory,
                                    aIdxHdr,
                                    aMiniTx,
                                    aDepth + 1, // Depth
                                    -1, // Nth Sibling
                                    aSpaceID,
                                    sIdxNodeHdr->mLeftmostChild, // My
                                    aPageSeq,
                                    aMyPID ) // Parent PID
                  != IDE_SUCCESS );

        //------------------------------------------
        // Other Child
        //------------------------------------------

        for ( i = 0; i < sSlotCnt; i++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               (UChar**)&sIKey )
                      != IDE_SUCCESS );
            
            SDNB_GET_RIGHT_CHILD_PID( &sRightChildPID, sIKey );
            IDE_TEST( traverseBuildKey( aStatistics,
                                        aHeader,
                                        aMemory,
                                        aIdxHdr,
                                        aMiniTx,
                                        aDepth + 1, 
                                        i, 
                                        aSpaceID,
                                        sRightChildPID,
                                        aPageSeq,
                                        aMyPID ) 
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Leaf Node
        // Nothing To Do
    }
    
    
    //------------------------------------------
    // Finalize
    //------------------------------------------
    
    sdrMiniTrans::releaseLatchToSP( aMiniTx, &sSP );

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


//======================================================================
//  X$DISK_BTREE_HEADER
//  disk index의 run-time header를 보여주는 peformance view
//======================================================================

IDE_RC sdnbFT::buildRecordForDiskBTreeHeader(idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * /* aDumpObj */,
                                             iduFixedTableMemory * aMemory)
{
    smcTableHeader     * sCatTblHdr;
    smcTableHeader     * sTableHeader;
    smpSlotHeader      * sPtr;
    SChar              * sCurPtr;
    SChar              * sNxtPtr;
    smnIndexHeader     * sIndexCursor;
    sdnbHeader         * sIndexHeader;
    sdnbHeader4PerfV     sIndexHeader4PerfV;
    void               * sTrans;
    UInt                 sIndexCnt;
    UInt                 i;
    sdnbColLenInfoList * sColLenInfoList;
    UInt                 sColumnIdx ;
    void               * sISavepoint = NULL;
    UInt                 sDummy = 0;
    UInt                 sState = 0;

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

        // disk table only
        if( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_FALSE )
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
                                                              SMC_TABLE_LOCK( sTableHeader ) )
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

                    if( sIndexCursor->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    // BUG-30867 Discard 된 Tablespace에 속한 Index는 Skip되어야 함
                    if( sctTableSpaceMgr::hasState( sIndexCursor->mIndexSegDesc.mSpaceID,
                                                    SCT_SS_INVALID_DISK_TBS ) == ID_TRUE )
                    {
                        continue;
                    }

                    sIndexHeader = (sdnbHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        idlOS::memset( &sIndexHeader4PerfV, 0x00, ID_SIZEOF(sdnbHeader4PerfV) );

                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;

                        sIndexHeader4PerfV.mIsConsistent = 'F';
                    }
                    else
                    {
                        idlOS::memset( &sIndexHeader4PerfV, 0x00, ID_SIZEOF(sdnbHeader4PerfV) );

                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;

                        sIndexHeader4PerfV.mIndexTSID  = sIndexHeader->mIndexTSID;
                        sIndexHeader4PerfV.mTableTSID  = sIndexHeader->mTableTSID;
                        sIndexHeader4PerfV.mSegHdrPID  = sdpSegDescMgr::getSegPID(&sIndexHeader->mSegmentDesc);
                        sIndexHeader4PerfV.mRootNode      = sIndexHeader->mRootNode;
                        sIndexHeader4PerfV.mEmptyNodeHead = sIndexHeader->mEmptyNodeHead;
                        sIndexHeader4PerfV.mEmptyNodeTail = sIndexHeader->mEmptyNodeTail;
                        sdnbBTree::getSmoNo( (void*)sIndexHeader, &sIndexHeader4PerfV.mSmoNo );

                        sIndexHeader4PerfV.mFreeNodeHead = sIndexHeader->mFreeNodeHead;
                        sIndexHeader4PerfV.mFreeNodeCnt = sIndexHeader->mFreeNodeCnt;

                        sIndexHeader4PerfV.mCompletionLSN = sIndexHeader->mCompletionLSN;
                        sIndexHeader4PerfV.mIsUnique =
                            ( sIndexHeader->mIsUnique == ID_TRUE ) ? 'T' : 'F';


                        sColLenInfoList = &(sIndexHeader->mColLenInfoList);
                        sIndexHeader4PerfV.mColLenInfoStr[0] = 0;

                        if( SMI_MAX_IDX_COLUMNS < sColLenInfoList->mColumnCount )
                        {
                            idlOS::snprintf( (SChar*)sIndexHeader4PerfV.mColLenInfoStr, 
                                             SDNB_MAX_COLLENINFOLIST_STR_SIZE+8, 
                                             "INVALIDE COLUMN COUNT_%"ID_UINT32_FMT, 
                                             sColLenInfoList->mColumnCount  );
                        }
                        else
                        {
                            for( sColumnIdx = 0 ; 
                                 sColumnIdx < sColLenInfoList->mColumnCount ; 
                                 sColumnIdx++ )
                            {
                                if( sColLenInfoList->mColLenInfo[ sColumnIdx ] ==  
                                    SDNB_COLLENINFO_LENGTH_UNKNOWN )
                                {
                                    sIndexHeader4PerfV.mColLenInfoStr[ sColumnIdx*2 ] = '?';
                                }
                                else
                                {
                                    sIndexHeader4PerfV.mColLenInfoStr[ sColumnIdx*2 ] = 
                                        sColLenInfoList->mColLenInfo[ sColumnIdx ] +'0';
                                }

                                if( sColumnIdx == (UInt)( sColLenInfoList->mColumnCount -1 ) )
                                {
                                    sIndexHeader4PerfV.mColLenInfoStr[ sColumnIdx*2 + 1] = '\0';
                                }
                                else
                                {
                                    sIndexHeader4PerfV.mColLenInfoStr[ sColumnIdx*2 + 1] = ',';
                                }
                            }
                        }

                        sIndexHeader4PerfV.mMinNode = sIndexHeader->mMinNode;
                        sIndexHeader4PerfV.mMaxNode = sIndexHeader->mMaxNode;

                        sIndexHeader4PerfV.mIsConsistent =
                            ( sIndexHeader->mIsConsistent 
                              == ID_TRUE ) ? 'T' : 'F';

                        // BUG-17957
                        // X$DISK_BTREE_HEADER에 creation option(logging, force) 추가
                        sIndexHeader4PerfV.mIsCreatedWithLogging =
                            ( sIndexHeader->mIsCreatedWithLogging == ID_TRUE ) ? 'T' : 'F';
                        sIndexHeader4PerfV.mIsCreatedWithForce =
                            ( sIndexHeader->mIsCreatedWithForce == ID_TRUE ) ? 'T' : 'F';

                        // PROJ-1704
                        sIndexHeader4PerfV.mSegAttr =
                            sIndexHeader->mSegmentDesc.mSegHandle.mSegAttr;
                        // PROJ-1671
                        sIndexHeader4PerfV.mSegStoAttr =
                            sIndexHeader->mSegmentDesc.mSegHandle.mSegStoAttr;
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

static iduFixedTableColDesc  gDiskBTreeHeaderColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(sdnbHeader4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(sdnbHeader4PerfV, mIndexID ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INDEX_TBS_ID",
        offsetof(sdnbHeader4PerfV, mIndexTSID ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mIndexTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TABLE_TBS_ID",
        offsetof(sdnbHeader4PerfV, mTableTSID ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mTableTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"SEG_HDR_PAGEID",
        offsetof(sdnbHeader4PerfV, mSegHdrPID ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mSegHdrPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"ROOT_PAGEID",
        offsetof(sdnbHeader4PerfV, mRootNode ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mRootNode ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"EMPTY_HEAD_PAGEID",
        offsetof(sdnbHeader4PerfV, mEmptyNodeHead ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mEmptyNodeHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"EMPTY_TAIL_PAGEID",
        offsetof(sdnbHeader4PerfV, mEmptyNodeTail ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mEmptyNodeTail ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"SMO_NO",
        offsetof(sdnbHeader4PerfV, mSmoNo ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mSmoNo ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_UNIQUE",
        offsetof(sdnbHeader4PerfV, mIsUnique ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mIsUnique ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"COLLENINFO_LIST",
        offsetof(sdnbHeader4PerfV, mColLenInfoStr ),
        SDNB_MAX_COLLENINFOLIST_STR_SIZE,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MIN_PAGEID",
        offsetof(sdnbHeader4PerfV, mMinNode ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mMinNode ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_PAGEID",
        offsetof(sdnbHeader4PerfV, mMaxNode ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mMaxNode ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FREE_NODE_HEAD",
        offsetof(sdnbHeader4PerfV, mFreeNodeHead ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mFreeNodeHead ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FREE_NODE_CNT",
        offsetof(sdnbHeader4PerfV, mFreeNodeCnt ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mFreeNodeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CONSISTENT",
        offsetof(sdnbHeader4PerfV, mIsConsistent ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mIsConsistent ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CREATED_WITH_LOGGING",
        offsetof(sdnbHeader4PerfV, mIsCreatedWithLogging ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mIsCreatedWithLogging ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"IS_CREATED_WITH_FORCE",
        offsetof(sdnbHeader4PerfV, mIsCreatedWithForce ),
        IDU_FT_SIZEOF(sdnbHeader4PerfV, mIsCreatedWithForce ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"COMPLETION_LSN_FILE_NO",
        offsetof(sdnbHeader4PerfV, mCompletionLSN ) + offsetof(smLSN, mFileNo ),
        IDU_FT_SIZEOF(smLSN, mFileNo ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"COMPLETION_LSN_FILE_OFFSET",
        offsetof(sdnbHeader4PerfV, mCompletionLSN ) + offsetof(smLSN, mOffset ),
        IDU_FT_SIZEOF(smLSN, mOffset ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INIT_TRANS",
        offsetof(sdnbHeader4PerfV, mSegAttr ) +
            offsetof(smiSegAttr, mInitTrans),
        IDU_FT_SIZEOF(smiSegAttr, mInitTrans),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAX_TRANS",
        offsetof(sdnbHeader4PerfV, mSegAttr ) +
            offsetof(smiSegAttr, mMaxTrans),
        IDU_FT_SIZEOF(smiSegAttr, mMaxTrans),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INITEXTENTS",
        offsetof(sdnbHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mInitExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mInitExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NEXTEXTENTS",
        offsetof(sdnbHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mNextExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mNextExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MINEXTENTS",
        offsetof(sdnbHeader4PerfV, mSegStoAttr ) +
            offsetof(smiSegStorageAttr, mMinExtCnt),
        IDU_FT_SIZEOF(smiSegStorageAttr, mMinExtCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MAXEXTENTS",
        offsetof(sdnbHeader4PerfV, mSegStoAttr ) +
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

iduFixedTableDesc  gDiskBTreeHeaderDesc=
{
    (SChar *)"X$DISK_BTREE_HEADER",
    sdnbFT::buildRecordForDiskBTreeHeader,
    gDiskBTreeHeaderColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

//======================================================================
//  X$DISK_BTREE_STAT
//  disk index의 run-time statistic information을 위한 peformance view
//======================================================================

IDE_RC sdnbFT::buildRecordForDiskBTreeStat(idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * /* aDumpObj */,
                                           iduFixedTableMemory * aMemory)
{
    smcTableHeader   * sCatTblHdr;
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sPtr;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    smnIndexHeader   * sIndexCursor;
    sdnbHeader       * sIndexHeader;
    sdnbColumn       * sIndexColumn;
    sdnbStat4PerfV     sIndexStat4PerfV;
    void             * sTrans;
    UInt               sIndexCnt;
    UInt               i;
    UChar              sValueBuffer[SM_DUMP_VALUE_BUFFER_SIZE];
    UInt               sValueLength;
    IDE_RC             sReturn;
    void             * sISavepoint = NULL;
    UInt               sDummy = 0;
    UChar            * sColumnValuePtr;
    UInt               sLength;
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

        // disk table only
        if( SMI_TABLE_TYPE_IS_DISK( sTableHeader ) == ID_FALSE )
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
                                                              SMC_TABLE_LOCK( sTableHeader ) )
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

                    if( sIndexCursor->mType != SMI_BUILTIN_B_TREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    // BUG-30867 Discard 된 Tablespace에 속한 Index는 Skip되어야 함
                    if( sctTableSpaceMgr::hasState( sIndexCursor->mIndexSegDesc.mSpaceID,
                                                    SCT_SS_INVALID_DISK_TBS ) == ID_TRUE )
                    {
                        continue;
                    }

                    sIndexHeader = (sdnbHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        continue;
                    }

                    idlOS::memset( &sIndexStat4PerfV, 0x00, ID_SIZEOF(sdnbStat4PerfV) );

                    idlOS::memcpy( &sIndexStat4PerfV.mName,
                                   &sIndexCursor->mName,
                                   SMN_MAX_INDEX_NAME_SIZE+8);
                    sIndexStat4PerfV.mIndexID = sIndexCursor->mId;

                    sIndexStat4PerfV.mTreeLatchReadCount = 
                        sIndexHeader->mLatch.getReadCount();
                    sIndexStat4PerfV.mTreeLatchWriteCount = 
                        sIndexHeader->mLatch.getWriteCount();
                    sIndexStat4PerfV.mTreeLatchReadMissCount = 
                        sIndexHeader->mLatch.getReadMisses();
                    sIndexStat4PerfV.mTreeLatchWriteMissCount = 
                        sIndexHeader->mLatch.getWriteMisses();

                    sIndexStat4PerfV.mKeyCount = sIndexCursor->mStat.mKeyCount;

                    sIndexStat4PerfV.mDMLStat   = sIndexHeader->mDMLStat;
                    sIndexStat4PerfV.mQueryStat = sIndexHeader->mQueryStat;
                    sIndexStat4PerfV.mNumDist   = sIndexCursor->mStat.mNumDist;

                    sIndexColumn = &(sIndexHeader->mColumns[0]);

                    /* PROJ-2429 Dictionary based data compress for on-disk DB */
                    if ( ((sIndexColumn->mKeyColumn.flag & SMI_COLUMN_COMPRESSION_MASK) 
                          == SMI_COLUMN_COMPRESSION_TRUE ) &&
                         (*(smOID*)(sIndexCursor->mStat.mMinValue) != SM_NULL_OID) )
                    {
                        sColumnValuePtr = (UChar*)smiGetCompressionColumn( 
                            (const void *)sIndexCursor->mStat.mMinValue,
                            &sIndexColumn->mKeyColumn,
                            ID_FALSE, // aUseColumnOffset
                            &sLength );
                    }
                    else
                    {
                        sColumnValuePtr = (UChar*)sIndexCursor->mStat.mMinValue;
                    }

                    // BUG-18188 : MIN_VALUE
                    sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
                    IDE_TEST( sIndexColumn->mKey2String(
                                  & (sIndexColumn->mKeyColumn),
                                  sColumnValuePtr,
                                  sIndexColumn->mKeyColumn.size,
                                  (UChar*) SM_DUMP_VALUE_DATE_FMT,
                                  idlOS::strlen( SM_DUMP_VALUE_DATE_FMT ),
                                  sValueBuffer,
                                  & sValueLength,
                                  & sReturn )
                              != IDE_SUCCESS );

                    idlOS::memcpy( sIndexStat4PerfV.mMinValue,
                                   sValueBuffer,
                                   ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                                   SM_DUMP_VALUE_LENGTH : sValueLength );

                    /* PROJ-2429 Dictionary based data compress for on-disk DB */
                    if ( ((sIndexColumn->mKeyColumn.flag & SMI_COLUMN_COMPRESSION_MASK) 
                          == SMI_COLUMN_COMPRESSION_TRUE ) &&
                         (*(smOID*)(sIndexCursor->mStat.mMinValue) != SM_NULL_OID) )
                    {
                        sColumnValuePtr = (UChar*)smiGetCompressionColumn( 
                            (const void *)sIndexCursor->mStat.mMaxValue,
                            &sIndexColumn->mKeyColumn,
                            ID_FALSE, // aUseColumnOffset
                            &sLength );
                    }
                    else
                    {
                        sColumnValuePtr = (UChar*)sIndexCursor->mStat.mMaxValue;
                    }

                    // BUG-18188 : MAX_VALUE
                    sValueLength = SM_DUMP_VALUE_BUFFER_SIZE;
                    IDE_TEST( sIndexColumn->mKey2String(
                                  & (sIndexColumn->mKeyColumn),
                                  sColumnValuePtr,
                                  sIndexColumn->mKeyColumn.size,
                                  (UChar*) SM_DUMP_VALUE_DATE_FMT,
                                  idlOS::strlen( SM_DUMP_VALUE_DATE_FMT ),
                                  sValueBuffer,
                                  & sValueLength,
                                  & sReturn )
                              != IDE_SUCCESS );

                    idlOS::memcpy( sIndexStat4PerfV.mMaxValue,
                                   sValueBuffer,
                                   ( sValueLength > SM_DUMP_VALUE_LENGTH ) ?
                                   SM_DUMP_VALUE_LENGTH : sValueLength );

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

static iduFixedTableColDesc  gDiskBTreeStatColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(sdnbStat4PerfV, mName ),
        SM_MAX_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(sdnbStat4PerfV, mIndexID ),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_READ_COUNT",
        offsetof(sdnbStat4PerfV, mTreeLatchReadCount),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mTreeLatchReadCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_WRITE_COUNT",
        offsetof(sdnbStat4PerfV, mTreeLatchWriteCount),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mTreeLatchWriteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_READ_MISS_COUNT",
        offsetof(sdnbStat4PerfV, mTreeLatchReadMissCount),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mTreeLatchReadMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_LATCH_WRITE_MISS_COUNT",
        offsetof(sdnbStat4PerfV, mTreeLatchWriteMissCount),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mTreeLatchWriteMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COUNT",
        offsetof(sdnbStat4PerfV, mKeyCount),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mKeyCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(sdnbStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(sdnbStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_RETRY_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mNodeRetryCount),
        IDU_FT_SIZEOF(sdnbStatistic, mNodeRetryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"RETRAVERSE_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mRetraverseCount),
        IDU_FT_SIZEOF(sdnbStatistic, mRetraverseCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"OP_RETRY_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mOpRetryCount),
        IDU_FT_SIZEOF(sdnbStatistic, mOpRetryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"PESSIMISTIC_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mPessimisticCount),
        IDU_FT_SIZEOF(sdnbStatistic, mPessimisticCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(sdnbStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"BACKWARD_SCAN_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) + offsetof(sdnbStatistic, mBackwardScanCount),
        IDU_FT_SIZEOF(sdnbStatistic, mBackwardScanCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_INDEX_PAGE_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) +
        offsetof(sdnbStatistic, mIndexPage) +
        offsetof(sdnbPageStat, mGetPageCount),
        IDU_FT_SIZEOF(sdnbPageStat, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_INDEX_PAGE_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) +
        offsetof(sdnbStatistic, mIndexPage) +
        offsetof(sdnbPageStat, mReadPageCount),
        IDU_FT_SIZEOF(sdnbPageStat, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_META_PAGE_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) +
        offsetof(sdnbStatistic, mMetaPage) +
        offsetof(sdnbPageStat, mGetPageCount),
        IDU_FT_SIZEOF(sdnbPageStat, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_META_PAGE_COUNT_BY_DML",
        offsetof(sdnbStat4PerfV, mDMLStat) +
        offsetof(sdnbStatistic, mMetaPage) +
        offsetof(sdnbPageStat, mReadPageCount),
        IDU_FT_SIZEOF(sdnbPageStat, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"GET_INDEX_PAGE_COUNT_BY_QUERY",
        offsetof(sdnbStat4PerfV, mQueryStat) +
        offsetof(sdnbStatistic, mIndexPage) +
        offsetof(sdnbPageStat, mGetPageCount),
        IDU_FT_SIZEOF(sdnbPageStat, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"READ_INDEX_PAGE_COUNT_BY_QUERY",
        offsetof(sdnbStat4PerfV, mQueryStat) +
        offsetof(sdnbStatistic, mIndexPage) +
        offsetof(sdnbPageStat, mReadPageCount),
        IDU_FT_SIZEOF(sdnbPageStat, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NUMDIST",
        offsetof(sdnbStat4PerfV, mNumDist ),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mNumDist ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"MIN_VALUE",
        offsetof(sdnbStat4PerfV, mMinValue ),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mMinValue ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MAX_VALUE",
        offsetof(sdnbStat4PerfV, mMaxValue ),
        IDU_FT_SIZEOF(sdnbStat4PerfV, mMaxValue ),
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

iduFixedTableDesc  gDiskBTreeStatDesc=
{
    (SChar *)"X$DISK_BTREE_STAT",
    sdnbFT::buildRecordForDiskBTreeStat,
    gDiskBTreeStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};
