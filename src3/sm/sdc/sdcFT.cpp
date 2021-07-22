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
 * $Id: sdcFT.cpp 85993 2019-08-07 04:59:34Z et16 $
 *
 * Description :
 *
 * 본 파일은 Collection Layer의 Fixed Table 구현파일 입니다.
 *
 **********************************************************************/

# include <smErrorCode.h>
# include <sdcDef.h>
# include <sdcFT.h>
# include <sdc.h>
# include <smxTransMgr.h>
# include <sdcTXSegMgr.h>
# include <smiFixedTable.h>
# include <sdcReq.h>
# include <sdbMPRMgr.h>
# include <smcFT.h>

static iduFixedTableColDesc gTSSEGSDesc[]=
{
    {
        (SChar*)"SPACE_ID",
        offsetof(sdcTSSegInfo, mSpaceID),
        IDU_FT_SIZEOF(sdcTSSegInfo, mSpaceID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"SEG_PID",
        offsetof(sdcTSSegInfo, mSegPID),
        IDU_FT_SIZEOF(sdcTSSegInfo, mSegPID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TYPE",
        offsetof(sdcTSSegInfo,mType),
        IDU_FT_SIZEOF(sdcTSSegInfo,mType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"STATE",
        offsetof(sdcTSSegInfo,mState),
        IDU_FT_SIZEOF(sdcTSSegInfo,mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TXSEG_ENTRY_ID",
        offsetof(sdcTSSegInfo, mTXSegID),
        IDU_FT_SIZEOF(sdcTSSegInfo, mTXSegID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"CUR_ALLOC_EXTENT_RID",
        offsetof(sdcTSSegInfo, mCurAllocExtRID),
        IDU_FT_SIZEOF(sdcTSSegInfo, mCurAllocExtRID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"CUR_ALLOC_PAGE_ID",
        offsetof(sdcTSSegInfo, mCurAllocPID),
        IDU_FT_SIZEOF(sdcTSSegInfo, mCurAllocPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TOTAL_EXTENT_COUNT",
        offsetof(sdcTSSegInfo, mTotExtCnt),
        IDU_FT_SIZEOF(sdcTSSegInfo, mTotExtCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TOTAL_EXTDIR_COUNT",
        offsetof(sdcTSSegInfo, mTotExtDirCnt),
        IDU_FT_SIZEOF(sdcTSSegInfo, mTotExtDirCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"PAGE_COUNT_IN_EXTENT",
        offsetof(sdcTSSegInfo, mPageCntInExt),
        IDU_FT_SIZEOF(sdcTSSegInfo, mPageCntInExt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0,
        0,
        NULL // for internal use
    }
};

/***********************************************************************
 * Description : X$TSSEGS의 레코드를 구성한다.
 ***********************************************************************/
IDE_RC sdcFT::buildRecord4TSSEGS( idvSQL              * /*aStatistics*/,
                                  void                *aHeader,
                                  void                * /* aDumpObj */,
                                  iduFixedTableMemory *aMemory )
{
    UInt               i;
    UInt              sTotEntryCnt;
    sdcTXSegEntry   * sEntry;
    UInt              sSpaceID;        // TBSID
    scPageID          sSegPID;         // 세그먼트 PID
    void            * sIndexValues[2];

    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();

    for ( i = 0; i < sTotEntryCnt; i++ )
    {
        sEntry = sdcTXSegMgr::getEntryByIdx( i );

        sSpaceID = SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO;
        sSegPID = sdcTXSegMgr::getTSSegPtr(sEntry)->getSegPID();

        /* BUG-43006 FixedTable Indexing Filter
         * Column Index 를 사용해서 전체 Record를 생성하지않고
         * 부분만 생성해 Filtering 한다.
         * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
         * 해당하는 값을 순서대로 넣어주어야 한다.
         * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
         * 어 주어야한다.
         */
        sIndexValues[0] = &sSpaceID;
        sIndexValues[1] = &sSegPID;
        if ( iduFixedTable::checkKeyRange( aMemory,
                                           gTSSEGSDesc,
                                           sIndexValues )
             == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( sdcTXSegMgr::getTSSegPtr(sEntry)->build4SegmentPerfV(
                      aHeader,
                      aMemory ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// X$TSSEGS fixed table def
iduFixedTableDesc gTSSEGS =
{
    (SChar *)"X$TSSEGS",
    sdcFT::buildRecord4TSSEGS,
    gTSSEGSDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// X$DISK_TSS_RECORDS Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDiskTSSRecordsColDesc[]=
{
    {
        (SChar*)"SEG_SEQ",
        offsetof(sdcTSS4FT, mSegSeq ),
        IDU_FT_SIZEOF(sdcTSS4FT, mSegSeq ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEG_PID",
        offsetof(sdcTSS4FT, mSegPID ),
        IDU_FT_SIZEOF(sdcTSS4FT, mSegPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof(sdcTSS4FT, mPageID ),
        IDU_FT_SIZEOF(sdcTSS4FT, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"OFFSET",
        offsetof(sdcTSS4FT, mOffset ),
        IDU_FT_SIZEOF(sdcTSS4FT, mOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(sdcTSS4FT, mNthSlot ),
        IDU_FT_SIZEOF(sdcTSS4FT, mNthSlot ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TID",
        offsetof(sdcTSS4FT, mTransID),
        IDU_FT_SIZEOF(sdcTSS4FT, mTransID),
        IDU_FT_TYPE_UBIGINT, // BUG-47379 unsigned int -> big int
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CSCN",
        offsetof(sdcTSS4FT, mCSCN ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof(sdcTSS4FT, mState),
        IDU_FT_SIZEOF(sdcTSS4FT, mState),
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
// X$DISK_TSS_RECORDS Table Description
//------------------------------------------------------

iduFixedTableDesc gDiskTSSRecordsTableDesc =
{
    (SChar *)"X$DISK_TSS_RECORDS",
    sdcFT::buildRecordForDiskTSSRecords,
    gDiskTSSRecordsColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// X$DISK_TSS_RECORDS Build Function
//------------------------------------------------------
IDE_RC sdcFT::buildRecordForDiskTSSRecords( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * /* aDumpObj */,
                                            iduFixedTableMemory * aMemory )
{
    UInt               i;
    UInt              sTotEntryCnt;
    sdcTXSegEntry   * sEntry;

    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();

    for( i = 0; i < sTotEntryCnt; i++ )
    {
        sEntry = sdcTXSegMgr::getEntryByIdx( i );

        IDE_TEST( sdcTXSegMgr::getTSSegPtr(sEntry)->build4RecordPerfV(
                      i, 
                      aHeader,
                      aMemory ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc gUDSEGSDesc[]=
{
    {
        (SChar*)"SPACE_ID",
        offsetof(sdcUDSegInfo, mSpaceID),
        IDU_FT_SIZEOF(sdcUDSegInfo, mSpaceID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"SEG_PID",
        offsetof(sdcUDSegInfo, mSegPID),
        IDU_FT_SIZEOF(sdcUDSegInfo, mSegPID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TYPE",
        offsetof(sdcUDSegInfo,mType),
        IDU_FT_SIZEOF(sdcUDSegInfo,mType),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"STATE",
        offsetof(sdcUDSegInfo,mState),
        IDU_FT_SIZEOF(sdcUDSegInfo,mState),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TXSEG_ENTRY_ID",
        offsetof(sdcUDSegInfo, mTXSegID),
        IDU_FT_SIZEOF(sdcUDSegInfo, mTXSegID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"CUR_ALLOC_EXTENT_RID",
        offsetof(sdcUDSegInfo, mCurAllocExtRID),
        IDU_FT_SIZEOF(sdcUDSegInfo, mCurAllocExtRID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"CUR_ALLOC_PAGE_ID",
        offsetof(sdcUDSegInfo, mCurAllocPID),
        IDU_FT_SIZEOF(sdcUDSegInfo, mCurAllocPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TOTAL_EXTENT_COUNT",
        offsetof(sdcUDSegInfo, mTotExtCnt),
        IDU_FT_SIZEOF(sdcUDSegInfo, mTotExtCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"TOTAL_EXTDIR_COUNT",
        offsetof(sdcUDSegInfo, mTotExtDirCnt),
        IDU_FT_SIZEOF(sdcUDSegInfo, mTotExtDirCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0,
        0,
        NULL
    },
    {
        (SChar*)"PAGE_COUNT_IN_EXTENT",
        offsetof(sdcUDSegInfo, mPageCntInExt),
        IDU_FT_SIZEOF(sdcUDSegInfo, mPageCntInExt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0,
        0,
        NULL
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

/***********************************************************************
 * Description : X$UDSEGS의 Record들을 생성한다.
 ***********************************************************************/
IDE_RC sdcFT::buildRecord4UDSEGS( idvSQL              * /*aStatistics*/,
                                  void                *aHeader,
                                  void                * /* aDumpObj */,
                                  iduFixedTableMemory *aMemory )
{
    UInt               i;
    UInt              sTotEntryCnt;
    sdcTXSegEntry   * sEntry;
    UInt              sSpaceID;        // TBSID
    scPageID          sSegPID;         // 세그먼트 PID
    void            * sIndexValues[2];

    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();

    for ( i = 0; i < sTotEntryCnt; i++ )
    {
        sEntry = sdcTXSegMgr::getEntryByIdx( i );

        sSpaceID = SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO;
        sSegPID = sdcTXSegMgr::getUDSegPtr(sEntry)->getSegPID();

        /* BUG-43006 FixedTable Indexing Filter
         * Column Index 를 사용해서 전체 Record를 생성하지않고
         * 부분만 생성해 Filtering 한다.
         * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
         * 해당하는 값을 순서대로 넣어주어야 한다.
         * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
         * 어 주어야한다.
         */
        sIndexValues[0] = &sSpaceID;
        sIndexValues[1] = &sSegPID;
        if ( iduFixedTable::checkKeyRange( aMemory,
                                           gUDSEGSDesc,
                                           sIndexValues )
             == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( sdcTXSegMgr::getUDSegPtr(sEntry)->build4SegmentPerfV(
                      aHeader,
                      aMemory ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// X$UDSEGS fixed table def
iduFixedTableDesc gUDSEGS =
{
    (SChar *)"X$UDSEGS",
    sdcFT::buildRecord4UDSEGS,
    gUDSEGSDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// X$DISK_UNDO_RECORDS Build Function
//------------------------------------------------------
IDE_RC sdcFT::buildRecordForDiskUndoRecords(idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * /* aDumpObj */,
                                            iduFixedTableMemory * aMemory)
{
    UInt               i;
    UInt              sTotEntryCnt;
    sdcTXSegEntry   * sEntry;

    sTotEntryCnt = sdcTXSegMgr::getTotEntryCnt();

    for( i = 0; i < sTotEntryCnt; i++ )
    {
        sEntry = sdcTXSegMgr::getEntryByIdx( i );

        IDE_TEST( sdcTXSegMgr::getUDSegPtr(sEntry)->build4RecordPerfV(
                      i,
                      aHeader,
                      aMemory ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//------------------------------------------------------
// X$DISK_UNDO_RECORDS Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDiskUndoRecordsColDesc[]=
{
    {
        (SChar*)"SEG_SEQ",
        offsetof(sdcUndoRec4FT, mSegSeq ),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mSegSeq ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEG_PID",
        offsetof(sdcUndoRec4FT, mSegPID ),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mSegPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof(sdcUndoRec4FT, mPageID ),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"OFFSET",
        offsetof(sdcUndoRec4FT, mOffset ),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(sdcUndoRec4FT, mNthSlot ),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mNthSlot ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SIZE",
        offsetof(sdcUndoRec4FT, mSize),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TYPE",
        offsetof(sdcUndoRec4FT, mType),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mType),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FLAG",
        offsetof(sdcUndoRec4FT, mFlag),
        IDU_FT_SIZEOF(sdcUndoRec4FT, mFlag),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_OID",
        offsetof(sdcUndoRec4FT, mTableOID),
        IDU_FT_SIZEOF_UBIGINT,
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
// X$DISK_UNDO_RECORDS Table Description
//------------------------------------------------------
iduFixedTableDesc  gDiskUndoRecordsTableDesc =
{
    (SChar *)"X$DISK_UNDO_RECORDS",
    sdcFT::buildRecordForDiskUndoRecords,
    gDiskUndoRecordsColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_TABLE_RECORD Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskTableRecordColDesc[]=
{
    {
        (SChar*)"PAGE_ID",
        offsetof(sdcDumpDiskTableRow, mPageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mPageID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_NUM",
        offsetof(sdcDumpDiskTableRow, mSlotNum ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mSlotNum ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_OFFSET",
        offsetof(sdcDumpDiskTableRow, mSlotOffset ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mSlotOffset ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_CTS",
        offsetof(sdcDumpDiskTableRow, mNthCTS),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mNthCTS),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INFINITE_SCN",
        offsetof(sdcDumpDiskTableRow, mInfiniteSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNDO_PAGEID",
        offsetof(sdcDumpDiskTableRow, mUndoPID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mUndoPID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNDO_SLOTNUM",
        offsetof(sdcDumpDiskTableRow, mUndoSlotNum),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mUndoSlotNum),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_COUNT",
        offsetof(sdcDumpDiskTableRow, mColumnCount ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mColumnCount ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROW_FLAG",
        offsetof(sdcDumpDiskTableRow, mRowFlag ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mRowFlag ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PIECE_PAGE_ID",
        offsetof(sdcDumpDiskTableRow, mNextPiecePageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mNextPiecePageID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PIECE_SLOT_NUM",
        offsetof(sdcDumpDiskTableRow, mNextPieceSlotNum ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mNextPieceSlotNum ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_SEQ_IN_ROWPIECE",
        offsetof(sdcDumpDiskTableRow, mColumnSeqInRowPiece ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mColumnSeqInRowPiece ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_LEN_IN_ROWPIECE",
        offsetof(sdcDumpDiskTableRow, mColumnLenInRowPiece ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mColumnLenInRowPiece ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_VAL_IN_ROWPIECE",
        offsetof(sdcDumpDiskTableRow, mColumnValInRowPiece ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mColumnValInRowPiece ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TSS_PAGEID",
        offsetof(sdcDumpDiskTableRow, mTSSPageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mTSSPageID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TSS_SLOTNUM",
        offsetof(sdcDumpDiskTableRow, mTSSlotNum ),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mTSSlotNum ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FSCREDIT",
        offsetof(sdcDumpDiskTableRow, mFSCredit),
        IDU_FT_SIZEOF(sdcDumpDiskTableRow, mFSCredit),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FSCN_OR_CSCN",
        offsetof(sdcDumpDiskTableRow, mFSCNOrCSCN ),
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
// D$DISK_TABLE_RECORD Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskTableRecordTableDesc =
{
    (SChar *)"D$DISK_TABLE_RECORD",
    sdcFT::buildRecordDiskTableRecord,
    gDumpDiskTableRecordColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*******************************************************
 * D$DISK_TABLE_RECORD Dump Table의 레코드 Build
 *******************************************************/

IDE_RC sdcFT::buildRecordDiskTableRecord( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * aDumpObj,
                                          iduFixedTableMemory * aMemory )
{
    sdRID                         sCurExtRID;
    UInt                          sState = 0;
    sdpSegMgmtOp                * sSegMgmtOp;
    sdpSegInfo                    sSegInfo;
    sdpExtInfo                    sExtInfo;
    scPageID                      sCurPageID;
    scPageID                      sSegPID;
    UChar                       * sCurPagePtr;
    UChar                         sTempRowPieceBuf[ SM_DUMP_VALUE_LENGTH*2 ];
    UChar                         sTempStrBuf[ SM_DUMP_VALUE_LENGTH*2 ];
    smcTableHeader              * sTblHdr = NULL;
    sdcDumpDiskTableRow           sDumpRow;
    idBool                        sIsSuccess;
    sdpPhyPageHdr               * sPhyPageHdr;
    sdcRowHdrInfo                 sRowHdrInfo;
    sdSID                         sNextRowPieceSID;
    UChar                       * sSlotDirPtr;
    UChar                       * sSlot;
    UShort                        sSlotNum;
    UShort                        sSlotCount;
    UShort                        sColumnSeq;
    idBool                        sIsLastLimitResult;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sTblHdr = (smcTableHeader *)( (smpSlotHeader*)aDumpObj + 1);

    IDE_TEST_RAISE( (sTblHdr->mType != SMC_TABLE_NORMAL) &&
                    (sTblHdr->mType != SMC_TABLE_CATALOG),
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( sTblHdr ) == ID_FALSE,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Table Info
    //------------------------------------------
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( &(sTblHdr->mFixed.mDRDB) );
    sSegPID    = sdpSegDescMgr::getSegPID( &(sTblHdr->mFixed.mDRDB) );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sTblHdr->mSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       sTblHdr->mSpaceID,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            sTblHdr->mSpaceID,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID )
              != IDE_SUCCESS );

    while( sCurPageID != SD_NULL_PID )
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

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sTblHdr->mSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess) != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        if ( sPhyPageHdr->mPageType == SDP_PAGE_DATA )
        {
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPhyPageHdr );
            sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

            for( sSlotNum=0; sSlotNum < sSlotCount; sSlotNum++ )
            {
                if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotNum)
                        == ID_TRUE )
                {
                    continue;
                }

                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(sSlotDirPtr, 
                                                                  sSlotNum,
                                                                  &sSlot)
                          != IDE_SUCCESS );

                sdcRow::getRowHdrInfo( sSlot, &sRowHdrInfo );

                sDumpRow.mPageID        = sCurPageID;
                sDumpRow.mSlotNum       = sSlotNum;
                IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr, 
                                                      sSlotNum, 
                                                      &sDumpRow.mSlotOffset )
                          != IDE_SUCCESS);
                sDumpRow.mNthCTS        = (SShort)sRowHdrInfo.mCTSlotIdx;
                sDumpRow.mInfiniteSCN   = sRowHdrInfo.mInfiniteSCN;

                sDumpRow.mUndoPID       = SD_MAKE_PID( sRowHdrInfo.mUndoSID );
                sDumpRow.mUndoSlotNum   = SD_MAKE_SLOTNUM( sRowHdrInfo.mUndoSID );
                                        
                sDumpRow.mColumnCount   = sRowHdrInfo.mColCount;
                sDumpRow.mRowFlag       = (UShort)sRowHdrInfo.mRowFlag;
                                        
                sDumpRow.mTSSPageID     = sRowHdrInfo.mExInfo.mTSSPageID;
                sDumpRow.mTSSlotNum     = sRowHdrInfo.mExInfo.mTSSlotNum;
                sDumpRow.mFSCredit      = sRowHdrInfo.mExInfo.mFSCredit;
                sDumpRow.mFSCNOrCSCN    = sRowHdrInfo.mExInfo.mFSCNOrCSCN;
                                        
                sDumpRow.mUndoPID       = SD_MAKE_PID(sRowHdrInfo.mUndoSID);
                sDumpRow.mUndoSlotNum   = SD_MAKE_OFFSET(sRowHdrInfo.mUndoSID);
                                        
                sNextRowPieceSID        = sdcRow::getNextRowPieceSID(sSlot);
                sDumpRow.mNextPiecePageID   = SD_MAKE_PID(sNextRowPieceSID);
                sDumpRow.mNextPieceSlotNum  = SD_MAKE_SLOTNUM(sNextRowPieceSID);

                if( sRowHdrInfo.mColCount == 0 )
                {
                    sDumpRow.mColumnSeqInRowPiece = 0;
                    sDumpRow.mColumnLenInRowPiece = 0;

                    idlOS::memset( sDumpRow.mColumnValInRowPiece,
                            0x00,
                            SM_DUMP_VALUE_LENGTH );

                    IDE_TEST( iduFixedTable::buildRecord(
                                aHeader,
                                aMemory,
                                (void *) &sDumpRow )
                            != IDE_SUCCESS );
                }
                else
                {
                    for( sColumnSeq = 0;
                            sColumnSeq < sRowHdrInfo.mColCount;
                            sColumnSeq++ )
                    {
                        sDumpRow.mColumnSeqInRowPiece = sColumnSeq;

                        idlOS::memset( sTempRowPieceBuf,
                                       0x00,
                                       SM_DUMP_VALUE_LENGTH+1 );

                        sdcRow::getColumnPiece( sSlot,
                                                sColumnSeq,
                                                (UChar*)sTempRowPieceBuf,
                                                SM_DUMP_VALUE_LENGTH,
                                                &sDumpRow.mColumnLenInRowPiece );

                        // ideMemToHexStr 함수는 Null termanted String을 고려하여
                        // 마지막 바이트를 \0으로 채우지만, mtdChar의 경우 마지막에
                        // \0이 들어가지 않기에 이점을 주의한다.
                        ideLog::ideMemToHexStr( sTempRowPieceBuf,
                                                SM_DUMP_VALUE_LENGTH, 
                                                IDE_DUMP_FORMAT_BINARY,
                                                (SChar*)sTempStrBuf,
                                                SM_DUMP_VALUE_LENGTH*2 ); 

                        idlOS::memcpy( sDumpRow.mColumnValInRowPiece, sTempStrBuf,SM_DUMP_VALUE_LENGTH );

                        IDE_TEST( iduFixedTable::buildRecord(
                                    aHeader,
                                    aMemory,
                                    (void *) &sDumpRow )
                                != IDE_SUCCESS );
                    }
                }
            }
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                sTblHdr->mSpaceID,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

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

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                               sCurPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description
 *
 *   D$DISK_TABLE_CTS
 *   : Disk Table의 CTS을 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_TABLE_CTS Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskTableCTSlotColDesc[]=
{
    {
        (SChar*)"PID",
        offsetof(sdcDumpCTS, mPID ),
        IDU_FT_SIZEOF(sdcDumpCTS, mPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(sdcDumpCTS, mNthSlot ),
        IDU_FT_SIZEOF(sdcDumpCTS, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TSS_PAGEID",
        offsetof(sdcDumpCTS, mTSSPageID ),
        IDU_FT_SIZEOF(sdcDumpCTS, mTSSPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TSS_SLOTNUM",
        offsetof(sdcDumpCTS, mTSSlotNum ),
        IDU_FT_SIZEOF(sdcDumpCTS, mTSSlotNum ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FSCN_OR_CSCN",
        offsetof(sdcDumpCTS, mFSCNOrCSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL
    },
    {
        (SChar*)"STATE",
        offsetof(sdcDumpCTS, mState ),
        IDU_FT_SIZEOF(sdcDumpCTS, mState ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"FSCREDIT",
        offsetof(sdcDumpCTS, mFSCredit),
        IDU_FT_SIZEOF(sdcDumpCTS, mFSCredit),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_CNT",
        offsetof(sdcDumpCTS, mRefCnt ),
        IDU_FT_SIZEOF(sdcDumpCTS, mRefCnt ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_SLOTNUM1",
        offsetof(sdcDumpCTS, mRefSlotNum1 ),
        IDU_FT_SIZEOF(sdcDumpCTS, mRefSlotNum1 ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"REF_SLOTNUM2",
        offsetof(sdcDumpCTS, mRefSlotNum2 ),
        IDU_FT_SIZEOF(sdcDumpCTS, mRefSlotNum2 ),
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
// D$DISK_TABLE_CTS Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskTableCTSlotTableDesc =
{
    (SChar *)"D$DISK_TABLE_CTS",
    sdcFT::buildRecordDiskTableCTS,
    gDumpDiskTableCTSlotColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*******************************************************
 * D$DISK_TABLE_CTS Dump Table의 레코드 Build
 *******************************************************/

IDE_RC sdcFT::buildRecordDiskTableCTS( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * aDumpObj,
                                       iduFixedTableMemory * aMemory )
{
    sdRID                 sCurExtRID;
    UInt                  sState = 0;
    sdpSegMgmtOp        * sSegMgmtOp;
    sdpSegInfo            sSegInfo;
    sdpExtInfo            sExtInfo;
    scPageID              sCurPageID;
    scPageID              sSegPID;
    UChar               * sCurPagePtr;
    smcTableHeader      * sTblHdr = NULL;
    sdcDumpCTS            sDumpCTS;
    idBool                sIsSuccess;
    sdpPhyPageHdr       * sPhyPageHdr;
    UChar                 sIdx;
    sdpCTL              * sCTL;
    sdpCTS              * sCTS;
    idBool                sIsLastLimitResult;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sTblHdr = (smcTableHeader *)( (smpSlotHeader*)aDumpObj + 1);

    IDE_TEST_RAISE( (sTblHdr->mType != SMC_TABLE_NORMAL) &&
                    (sTblHdr->mType != SMC_TABLE_CATALOG),
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( sTblHdr ) == ID_FALSE,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Table Info
    //------------------------------------------
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sTblHdr->mSpaceID );
    sSegPID    = sdpSegDescMgr::getSegPID( &(sTblHdr->mFixed.mDRDB) );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sTblHdr->mSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       sTblHdr->mSpaceID,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            sTblHdr->mSpaceID,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID )
              != IDE_SUCCESS );

    while( sCurPageID != SD_NULL_PID )
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

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sTblHdr->mSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess) != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        if ( sPhyPageHdr->mPageType == SDP_PAGE_DATA )
        {
            sCTL = sdcTableCTL::getCTL( sPhyPageHdr );

            for( sIdx=0; sIdx < sCTL->mTotCTSCnt; sIdx++ )
            {
                sCTS = sdcTableCTL::getCTS( sCTL, sIdx );

                sDumpCTS.mPID         = sCurPageID;
                sDumpCTS.mNthSlot     = sIdx;
                sDumpCTS.mFSCNOrCSCN  = sCTS->mFSCNOrCSCN;

                switch( sCTS->mStat )
                {
                    case SDP_CTS_STAT_NUL :
                        sDumpCTS.mState = 'N';
                        break;
                    case SDP_CTS_STAT_ACT :
                        sDumpCTS.mState = 'A';
                        break;
                    case SDP_CTS_STAT_CTS :
                        sDumpCTS.mState = 'T';
                        break;
                    case SDP_CTS_STAT_RTS :
                        sDumpCTS.mState = 'R';
                        break;
                    case SDP_CTS_STAT_ROL :
                        sDumpCTS.mState = 'O';
                        break;
                    default:
                        sDumpCTS.mState = '?';
                        break;
                }

                sDumpCTS.mTSSPageID   = sCTS->mTSSPageID;
                sDumpCTS.mTSSlotNum   = sCTS->mTSSlotNum;
                sDumpCTS.mFSCredit    = sCTS->mFSCredit;
                sDumpCTS.mRefCnt      = sCTS->mRefCnt;
                sDumpCTS.mRefSlotNum1 = sCTS->mRefRowSlotNum[0];
                sDumpCTS.mRefSlotNum2 = sCTS->mRefRowSlotNum[1];

                IDE_TEST( iduFixedTable::buildRecord(
                            aHeader,
                            aMemory,
                            (void *) &sDumpCTS )
                        != IDE_SUCCESS );
            }
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                sTblHdr->mSpaceID,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

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

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                               sCurPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description
 *
 *   D$DISK_TABLE_CTL
 *   : Disk Table의 CTL을 출력
 *
 *
 **********************************************************************/

//------------------------------------------------------
// D$DISK_TABLE_CTL Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskTableCTLColDesc[]=
{
    {
        (SChar*)"PID",
        offsetof(sdcDumpCTL, mPID ),
        IDU_FT_SIZEOF(sdcDumpCTL, mPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TOTAL_CTS_COUNT",
        offsetof(sdcDumpCTL, mTotCTSCnt),
        IDU_FT_SIZEOF(sdcDumpCTL, mTotCTSCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"BIND_CTS_COUNT",
        offsetof(sdcDumpCTL, mBindCTSCnt),
        IDU_FT_SIZEOF(sdcDumpCTL, mBindCTSCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"DELETE_ROW_COUNT",
        offsetof(sdcDumpCTL, mDelRowCnt),
        IDU_FT_SIZEOF(sdcDumpCTL, mDelRowCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"SCN_FOR_AGING",
        offsetof(sdcDumpCTL, mSCN4Aging),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0, NULL
    },
    {
        (SChar*)"CANDIDATE_AGING_COUNT",
        offsetof(sdcDumpCTL, mCandAgingCnt),
        IDU_FT_SIZEOF(sdcDumpCTL, mCandAgingCnt),
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
        0, 0,NULL // for internal use
    }
};

//------------------------------------------------------
// D$DISK_TABLE_CTL Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskTableCTLTableDesc =
{
    (SChar *)"D$DISK_TABLE_CTL",
    sdcFT::buildRecordDiskTableCTL,
    gDumpDiskTableCTLColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*******************************************************
 * D$DISK_TABLE_CTL Dump Table의 레코드 Build
 *******************************************************/

IDE_RC sdcFT::buildRecordDiskTableCTL( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * aDumpObj,
                                       iduFixedTableMemory * aMemory )
{
    sdRID                 sCurExtRID;
    UInt                  sState = 0;
    sdpSegMgmtOp        * sSegMgmtOp;
    sdpSegInfo            sSegInfo;
    sdpExtInfo            sExtInfo;
    scPageID              sCurPageID;
    scPageID              sSegPID;
    UChar               * sCurPagePtr;
    smcTableHeader      * sTblHdr = NULL;
    sdcDumpCTL            sDumpCTL;
    idBool                sIsSuccess;
    sdpPhyPageHdr       * sPhyPageHdr;
    sdpCTL              * sCTL;
    idBool                sIsLastLimitResult;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sTblHdr = (smcTableHeader *)( (smpSlotHeader*)aDumpObj + 1);

    IDE_TEST_RAISE( (sTblHdr->mType != SMC_TABLE_NORMAL) &&
                    (sTblHdr->mType != SMC_TABLE_CATALOG),
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( sTblHdr ) == ID_FALSE,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Table Info
    //------------------------------------------
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sTblHdr->mSpaceID );
    sSegPID    = sdpSegDescMgr::getSegPID( &(sTblHdr->mFixed.mDRDB) );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sTblHdr->mSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       sTblHdr->mSpaceID,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            sTblHdr->mSpaceID,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID )
              != IDE_SUCCESS );

    while( sCurPageID != SD_NULL_PID )
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

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sTblHdr->mSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess) != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        if ( sPhyPageHdr->mPageType == SDP_PAGE_DATA )
        {
            sCTL = sdcTableCTL::getCTL( sPhyPageHdr );

            sDumpCTL.mPID          = sCurPageID;
            sDumpCTL.mTotCTSCnt    = sCTL->mTotCTSCnt;
            sDumpCTL.mBindCTSCnt   = sCTL->mBindCTSCnt;
            sDumpCTL.mDelRowCnt    = sCTL->mDelRowCnt;
            sDumpCTL.mCandAgingCnt = sCTL->mCandAgingCnt;
            sDumpCTL.mSCN4Aging    = sCTL->mSCN4Aging;

            IDE_TEST( iduFixedTable::buildRecord( aHeader,
                        aMemory,
                        (void *) &sDumpCTL )
                    != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                sTblHdr->mSpaceID,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

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

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                               sCurPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


//------------------------------------------------------
// D$DISK_TABLE_SLOT Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskTableSlotDirColDesc[]=
{
    {
        (SChar*)"PAGE_ID",
        offsetof(sdcDumpDiskTableSlotDir, mPageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableSlotDir, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_ENTRY_CNT",
        offsetof(sdcDumpDiskTableSlotDir, mSlotEntryCnt),
        IDU_FT_SIZEOF(sdcDumpDiskTableSlotDir, mSlotEntryCnt),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UNUSED_LIST_HEAD",
        offsetof(sdcDumpDiskTableSlotDir, mUnusedListHead),
        IDU_FT_SIZEOF(sdcDumpDiskTableSlotDir, mUnusedListHead),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_NUM",
        offsetof(sdcDumpDiskTableSlotDir, mSlotNum),
        IDU_FT_SIZEOF(sdcDumpDiskTableSlotDir, mSlotNum),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_UNUSED",
        offsetof(sdcDumpDiskTableSlotDir, mIsUnused ),
        IDU_FT_SIZEOF(sdcDumpDiskTableSlotDir, mIsUnused ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"OFFSET",
        offsetof(sdcDumpDiskTableSlotDir, mOffset),
        IDU_FT_SIZEOF(sdcDumpDiskTableSlotDir, mOffset),
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
// D$DISK_TABLE_SLOT Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpDiskTableSlotDirTableDesc =
{
    (SChar *)"D$DISK_TABLE_SLOT",
    sdcFT::buildRecordDiskTableSlotDir,
    gDumpDiskTableSlotDirColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*******************************************************
 * D$DISK_TABLE_SLOT Dump Table의 레코드 Build
 *******************************************************/

IDE_RC sdcFT::buildRecordDiskTableSlotDir( idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * aDumpObj,
                                           iduFixedTableMemory * aMemory )
{
    sdRID                   sCurExtRID;
    UInt                    sState = 0;
    sdpSegMgmtOp          * sSegMgmtOp;
    sdpSegInfo              sSegInfo;
    sdpExtInfo              sExtInfo;
    scPageID                sCurPageID;
    scPageID                sSegPID;
    UChar                 * sCurPagePtr;
    smcTableHeader        * sTblHdr = NULL;
    sdcDumpDiskTableSlotDir sDumpRow;
    idBool                  sIsSuccess;
    sdpPhyPageHdr         * sPhyPageHdr;
    UChar                 * sSlotDirPtr;
    sdpSlotDirHdr         * sSlotDirHdr;
    UShort                  sSlotNum;
    idBool                  sIsLastLimitResult;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sTblHdr = (smcTableHeader *)( (smpSlotHeader*)aDumpObj + 1);

    IDE_TEST_RAISE( (sTblHdr->mType != SMC_TABLE_NORMAL) &&
                    (sTblHdr->mType != SMC_TABLE_CATALOG),
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( sTblHdr ) == ID_FALSE,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Table Info
    //------------------------------------------
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sTblHdr->mSpaceID );
    sSegPID    = sdpSegDescMgr::getSegPID( &(sTblHdr->mFixed.mDRDB) );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sTblHdr->mSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       sTblHdr->mSpaceID,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            sTblHdr->mSpaceID,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID )
              != IDE_SUCCESS );

    while( sCurPageID != SD_NULL_PID )
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

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sTblHdr->mSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess) != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        if ( sPhyPageHdr->mPageType == SDP_PAGE_DATA )
        {
            sSlotDirPtr   = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPhyPageHdr );
            sSlotDirHdr   = (sdpSlotDirHdr*)sSlotDirPtr;

            sDumpRow.mPageID         = sCurPageID;
            sDumpRow.mSlotEntryCnt   = sSlotDirHdr->mSlotEntryCount;
            sDumpRow.mUnusedListHead = sSlotDirHdr->mHead;

            for( sSlotNum=0; sSlotNum < sSlotDirHdr->mSlotEntryCount; sSlotNum++ )
            {
                sDumpRow.mSlotNum = sSlotNum;

                if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotNum)
                        == ID_TRUE )
                {
                    sDumpRow.mIsUnused = 'Y';
                    /* BUG-31534 [sm-util] dump utility and fixed table 
                     * do not consider unused slot. */
                    IDE_TEST( sdpSlotDirectory::getNextUnusedSlot( 
                                                              sSlotDirPtr, 
                                                              sSlotNum,
                                                              &sDumpRow.mOffset)
                              != IDE_SUCCESS );
                }
                else
                {
                    sDumpRow.mIsUnused = 'N';
                    /* BUG-31534 [sm-util] dump utility and fixed table 
                     * do not consider unused slot. */
                    IDE_TEST( sdpSlotDirectory::getValue(sSlotDirPtr, 
                                                         sSlotNum,
                                                         &sDumpRow.mOffset )
                              != IDE_SUCCESS);
                }


                IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                      aMemory,
                                                      (void *) &sDumpRow )
                          != IDE_SUCCESS );
            }
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                sTblHdr->mSpaceID,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

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

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                               sCurPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* TASK-4007 [SM]PBT를 위한 기능 추가
 * SegType을 영어 단문자로 표현
 * T = Table Segment
 * I = Index Segment
 * L = Lob Segment
 * */
SChar sdcFT::convertSegTypeToChar( sdpSegType aSegType )
{
    SChar sRet = '?';

    switch( aSegType )
    {
    case SDP_SEG_TYPE_TABLE:
        sRet = 'T';
        break;
    case SDP_SEG_TYPE_INDEX:
        sRet = 'I';
        break;
    case SDP_SEG_TYPE_LOB:
        sRet = 'L';
        break;
    default:
        break;
    }

    return sRet;
}


/* TASK-4007 [SM]PBT를 위한 기능 추가
 * Table로부터 TblSeg, IdxSeg, LobSeg의 SegHdrPID를 가져온다*/
IDE_RC sdcFT::doAction4EachSeg( void                * aTable,
                                sdcSegDumpCallback    aSegDumpFunc,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory )

{
    smcTableHeader  * sTblHdr = (smcTableHeader*)((smpSlotHeader*)aTable + 1);
    UInt              sIdxCnt;
    UInt              i;
    const void      * sIndex;
    const scGRID    * sIndexGRID;
    const smiColumn * sColumn;
    UInt              sColumnCnt;

    // Table SegHdr
    IDE_TEST( aSegDumpFunc( sTblHdr->mSpaceID,
                            smcTable::getSegPID( aTable ),
                            SDP_SEG_TYPE_TABLE,
                            aHeader,
                            aMemory )
              != IDE_SUCCESS );

    // Index SegHdr
    sIdxCnt = smcTable::getIndexCount( sTblHdr );

    for( i = 0; i < sIdxCnt ; i++ )
    {
        sIndex = smcTable::getTableIndex( sTblHdr, i );
        sIndexGRID = smLayerCallback::getIndexSegGRIDPtr( (void*)sIndex );

        IDE_TEST( aSegDumpFunc( SC_MAKE_SPACE(*sIndexGRID),
                                SC_MAKE_PID(*sIndexGRID), 
                                SDP_SEG_TYPE_INDEX,
                                aHeader,
                                aMemory )
              != IDE_SUCCESS );
    }

    // Lob SegHdr
    sColumnCnt = smcTable::getColumnCount( sTblHdr );

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn( sTblHdr, i );

        if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
        {
            IDE_TEST( aSegDumpFunc( sColumn->colSeg.mSpaceID,
                                    sColumn->colSeg.mPageID,
                                    SDP_SEG_TYPE_LOB,
                                    aHeader,
                                    aMemory )
              != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*------------------------------------------------------
 * D$DISK_TABLE_EXTLIST Dump Table의 레코드 Build
 *------------------------------------------------------*/
IDE_RC sdcFT::buildRecord4ExtList(
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

    IDE_TEST( doAction4EachSeg( sTable,
                                dumpExtList,
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

IDE_RC sdcFT::dumpExtList( scSpaceID             aSpaceID,
                           scPageID              aPageID,
                           sdpSegType            /*aSegType*/,
                           void                * aHeader,
                           iduFixedTableMemory * aMemory )
{

    sdcDumpDiskTableExtList sDumpExtInfo;
    scSpaceID               sSpaceID;
    sdpSegInfo              sSegInfo;
    sdRID                   sCurExtRID;
    sdpExtInfo              sExtInfo;
    scPageID                sSegPID;
    sdpSegMgmtOp           *sSegMgmtOp;
    idBool                  sIsLastLimitResult;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sSpaceID = aSpaceID;
    sSegPID  = aPageID;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sCurExtRID = sSegInfo.mFstExtRID;

    sDumpExtInfo.mSegType =
        convertSegTypeToChar( sSegInfo.mType );

    while( sCurExtRID != SD_NULL_RID )
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

        IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                           sSpaceID,
                                           sCurExtRID,
                                           &sExtInfo )
                  != IDE_SUCCESS );

        sDumpExtInfo.mExtRID       = sCurExtRID;
        sDumpExtInfo.mPID          = SD_MAKE_PID( sCurExtRID );
        sDumpExtInfo.mOffset       = SD_MAKE_OFFSET( sCurExtRID );
        sDumpExtInfo.mFstPID       = sExtInfo.mFstPID;
        sDumpExtInfo.mFstDataPID   = sExtInfo.mFstDataPID;
        sDumpExtInfo.mPageCntInExt = sSegInfo.mPageCntInExt;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *) & sDumpExtInfo )
                  != IDE_SUCCESS );

        IDE_TEST( sSegMgmtOp->mGetNxtExtInfo( NULL,
                                              sSpaceID,
                                              sSegPID,
                                              sCurExtRID,
                                              &sCurExtRID )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_EXTLIST Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTableEXTListColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdcDumpDiskTableExtList, mSegType ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"EXTRID",
        offsetof( sdcDumpDiskTableExtList, mExtRID ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mExtRID ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PID",
        offsetof( sdcDumpDiskTableExtList, mPID ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"Offset",
        offsetof( sdcDumpDiskTableExtList, mOffset ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mOffset ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FstPID",
        offsetof( sdcDumpDiskTableExtList, mFstPID ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mFstPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FstDataPID",
        offsetof( sdcDumpDiskTableExtList, mFstDataPID ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mFstDataPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ExtSize",
        offsetof( sdcDumpDiskTableExtList, mPageCntInExt ),
        IDU_FT_SIZEOF( sdcDumpDiskTableExtList, mPageCntInExt ),
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
// D$DISK_TABLE_EXTLIST Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTableEXTListTableDesc =
{
    (SChar *)"D$DISK_TABLE_EXTLIST",
    sdcFT::buildRecord4ExtList,
    gDumpDiskTableEXTListColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*------------------------------------------------------
 * D$DISK_TABLE_PIDLIST Dump Table의 레코드 Build
 *------------------------------------------------------*/
IDE_RC sdcFT::buildRecord4PIDList(
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

    IDE_TEST( doAction4EachSeg( sTable,
                                dumpPidList,
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

IDE_RC sdcFT::dumpPidList( scSpaceID             aSpaceID,
                           scPageID              aPageID,
                           sdpSegType            /*aSegType*/,
                           void                * aHeader,
                           iduFixedTableMemory * aMemory )
{
    sdcDumpDiskTablePIDList sDumpRow;
    scSpaceID               sSpaceID;
    sdpSegInfo              sSegInfo;
    sdRID                   sCurExtRID;
    sdpExtInfo              sExtInfo;
    scPageID                sSegPID;
    sdpSegMgmtOp           *sSegMgmtOp;
    idBool                  sIsLastLimitResult;
    scPageID                sCurPageID;
    idBool                  sIsSuccess;
    sdpPhyPageHdr          *sPhyPageHdr;
    SInt                    sState = 0;
    UChar                 * sCurPagePtr;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sSpaceID = aSpaceID;
    sSegPID  = aPageID;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       sSpaceID,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            sSpaceID,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID )
              != IDE_SUCCESS );

    sDumpRow.mSegType =
        convertSegTypeToChar( sSegInfo.mType );

    while( sCurPageID != SD_NULL_PID )
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

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess) != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        IDE_DASSERT( sPhyPageHdr->mPageID == sCurPageID );

        sDumpRow.mPageID    = sPhyPageHdr->mPageID;
        sDumpRow.mPageType  = sPhyPageHdr->mPageType;
        sDumpRow.mPageState = sPhyPageHdr->mPageState;

        sDumpRow.mLogicalHdrSize = sPhyPageHdr->mLogicalHdrSize;
        sDumpRow.mSizeOfCTL      = sPhyPageHdr->mSizeOfCTL;

        sDumpRow.mTotalFreeSize        = sPhyPageHdr->mTotalFreeSize;
        sDumpRow.mAvailableFreeSize    = sPhyPageHdr->mAvailableFreeSize;
        sDumpRow.mFreeSpaceBeginOffset = sPhyPageHdr->mFreeSpaceBeginOffset;
        sDumpRow.mFreeSpaceEndOffset   = sPhyPageHdr->mFreeSpaceEndOffset;

        sDumpRow.mSeqNo                = sPhyPageHdr->mSeqNo;

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( iduFixedTable::buildRecord(
                    aHeader,
                    aMemory,
                    (void *) &sDumpRow ) != IDE_SUCCESS );

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                sSpaceID,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    //------------------------------------------
    // Finalize
    //------------------------------------------

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                               sCurPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

//------------------------------------------------------
// D$DISK_TABLE_PIDLIST Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpDiskTablePIDListColDesc[]=
{
    {
        (SChar*)"SEGTYPE",
        offsetof( sdcDumpDiskTablePIDList, mSegType ),
        IDU_FT_SIZEOF( sdcDumpDiskTablePIDList, mSegType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof(sdcDumpDiskTablePIDList, mPageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mPageID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_TYPE",
        offsetof(sdcDumpDiskTablePIDList, mPageType ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mPageType ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_STATE",
        offsetof(sdcDumpDiskTablePIDList, mPageState ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mPageState ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOGICAL_HDR_SIZE",
        offsetof(sdcDumpDiskTablePIDList, mLogicalHdrSize ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mLogicalHdrSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CTL_SIZE",
        offsetof(sdcDumpDiskTablePIDList, mSizeOfCTL),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mSizeOfCTL),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOFS",
        offsetof(sdcDumpDiskTablePIDList, mTotalFreeSize ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mTotalFreeSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"AVSP",
        offsetof(sdcDumpDiskTablePIDList, mAvailableFreeSize ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mAvailableFreeSize ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FSBO",
        offsetof(sdcDumpDiskTablePIDList, mFreeSpaceBeginOffset ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mFreeSpaceBeginOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FSEO",
        offsetof(sdcDumpDiskTablePIDList, mFreeSpaceEndOffset ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mFreeSpaceEndOffset ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEQNO",
        offsetof(sdcDumpDiskTablePIDList, mSeqNo ),
        IDU_FT_SIZEOF(sdcDumpDiskTablePIDList, mSeqNo ),
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
// D$DISK_TABLE_PIDLIST Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTablePIDListTableDesc =
{
    (SChar *)"D$DISK_TABLE_PIDLIST",
    sdcFT::buildRecord4PIDList,
    gDumpDiskTablePIDListColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


//------------------------------------------------------
// X$DIRECT_PATH_INSERT
//------------------------------------------------------
static iduFixedTableColDesc gDPathInsertColDesc[]=
{
    {
        (SChar*)"COMMIT_TX_COUNT",
        offsetof(sdcDPathStat, mCommitTXCnt),
        IDU_FT_SIZEOF(sdcDPathStat, mCommitTXCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ABORT_TX_COUNT",
        offsetof(sdcDPathStat, mAbortTXCnt),
        IDU_FT_SIZEOF(sdcDPathStat, mAbortTXCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INSERT_ROW_COUNT",
        offsetof(sdcDPathStat, mInsRowCnt),
        IDU_FT_SIZEOF(sdcDPathStat, mInsRowCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ALLOC_BUFFER_PAGE_TRY_COUNT",
        offsetof(sdcDPathStat, mAllocBuffPageTryCnt),
        IDU_FT_SIZEOF(sdcDPathStat, mAllocBuffPageTryCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ALLOC_BUFFER_PAGE_FAIL_COUNT",
        offsetof(sdcDPathStat, mAllocBuffPageFailCnt),
        IDU_FT_SIZEOF(sdcDPathStat, mAllocBuffPageFailCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"BULK_IO_COUNT",
        offsetof(sdcDPathStat, mBulkIOCnt),
        IDU_FT_SIZEOF(sdcDPathStat, mBulkIOCnt),
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

IDE_RC sdcFT::buildRecordForDPathInsert( idvSQL              * /*aStatistics*/,
                                         void                 * aHeader,
                                         void                 * /*aDumpObj*/,
                                         iduFixedTableMemory  * aMemory )
{
    sdcDPathStat    sDPathStat;

    IDE_TEST( sdcDPathInsertMgr::getDPathStat( &sDPathStat ) != IDE_SUCCESS );
    
    IDE_TEST( iduFixedTable::buildRecord(aHeader,
                                         aMemory,
                                         (void *)&sDPathStat)
              != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc  gDPathInsertDesc=
{
    (SChar *)"X$DIRECT_PATH_INSERT",
    sdcFT::buildRecordForDPathInsert,
    gDPathInsertColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*
 * PROJ-2047 Strengthening LOB
 */

/*------------------------------------------------------
 * 
 * D$DISK_TABLE_LOB_AGINGLIST
 * 
 *------------------------------------------------------*/

static iduFixedTableColDesc gDumpDiskTableLobAgingListColDesc[]=
{
    {
        (SChar*)"COLUMN_IDX",
        offsetof(sdcDumpDiskTableLobAgingList, mColumnIdx ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mColumnIdx ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_ID",
        offsetof(sdcDumpDiskTableLobAgingList, mPageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mPageID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_TYPE",
        offsetof(sdcDumpDiskTableLobAgingList, mPageType ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mPageType ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HEIGHT",
        offsetof(sdcDumpDiskTableLobAgingList, mHeight ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mHeight ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"KEY_CNT",
        offsetof(sdcDumpDiskTableLobAgingList, mKeyCnt ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mKeyCnt ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STORE_SIZE",
        offsetof(sdcDumpDiskTableLobAgingList, mStoreSize ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mStoreSize ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NODE_SEQ",
        offsetof(sdcDumpDiskTableLobAgingList, mNodeSeq ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mNodeSeq ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOB_PAGE_STATE",
        offsetof(sdcDumpDiskTableLobAgingList, mLobPageState ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mLobPageState ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOB_VERSION",
        offsetof(sdcDumpDiskTableLobAgingList, mLobVersion ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mLobVersion ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TSS_SID",
        offsetof(sdcDumpDiskTableLobAgingList, mTSSlotSID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mTSSlotSID ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_DSK_VIEWSCN",
        offsetof(sdcDumpDiskTableLobAgingList, mFstDskViewSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use

    },
    {
        (SChar*)"PREV_PID",
        offsetof(sdcDumpDiskTableLobAgingList, mPrevPID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mPrevPID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PID",
        offsetof(sdcDumpDiskTableLobAgingList, mNextPID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobAgingList, mNextPID ),
        IDU_FT_TYPE_INTEGER,
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

iduFixedTableDesc  gDumpDiskTableLobAgingListTableDesc =
{
    (SChar *)"D$DISK_TABLE_LOB_AGINGLIST",
    sdcFT::buildRecord4LobAgingList,
    gDumpDiskTableLobAgingListColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC sdcFT::buildRecord4LobAgingList(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * aDumpObj,
    iduFixedTableMemory * aMemory )
{
    smcTableHeader  * sTblHdr;
    const smiColumn * sColumn;
    UInt              sColumnCnt;
    UInt              sColumnIdx;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );

    sTblHdr = (smcTableHeader*)((smpSlotHeader*)aDumpObj + 1);

    sColumnCnt = smcTable::getColumnCount( sTblHdr );

    for( sColumnIdx = 0; sColumnIdx < sColumnCnt; sColumnIdx++ )
    {
        sColumn = smcTable::getColumn( sTblHdr, sColumnIdx );

        if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
        {
            IDE_TEST( dumpLobAgingList(sColumn->colSeg.mSpaceID,
                                       sColumn->colSeg.mPageID,
                                       sColumnIdx,
                                       aHeader,
                                       aMemory)
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcFT::dumpLobAgingList( scSpaceID             aSpaceID,
                                scPageID              aPageID,
                                UInt                  aColumnIdx,
                                void                * aHeader,
                                iduFixedTableMemory * aMemory )
{
    sdcDumpDiskTableLobAgingList      sDumpRow;
    sdcLobNodeHdr                   * sLobNodeHdr;
    sdcLobMeta                      * sLobMeta;
    sdpSegInfo                        sSegInfo;
    sdpSegMgmtOp                    * sSegMgmtOp;
    sdpPhyPageHdr                   * sPhyPageHdr;
    sdpDblPIDListNode               * sNode;
    scSpaceID                         sSpaceID;
    scPageID                          sSegPID;
    scPageID                          sMetaPID;
    scPageID                          sCurPageID;
    idBool                            sIsLastLimitResult;
    idBool                            sIsSuccess;
    UChar                           * sCurPagePtr;
    UChar                           * sMetaPagePtr;
    SInt                              sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sSpaceID = aSpaceID;
    sSegPID  = aPageID;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST( sSegMgmtOp->mGetMetaPID( NULL, /* idvSQL */
                                       sSpaceID,
                                       sSegPID,
                                       0, /* Seg Meta PID Array Index */
                                       &sMetaPID )
              != IDE_SUCCESS );

    IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                     sSpaceID,
                                     sMetaPID,
                                     SDB_S_LATCH,
                                     SDB_WAIT_NORMAL,
                                     SDB_SINGLE_PAGE_READ,
                                     (UChar**)&sMetaPagePtr,
                                     &sIsSuccess )
              != IDE_SUCCESS );
    sState = 1;

    sPhyPageHdr = sdpPhyPage::getHdr( sMetaPagePtr );
    sLobMeta    = (sdcLobMeta*)sdpPhyPage::getLogicalHdrStartPtr( sMetaPagePtr );
    sCurPageID  = sdpDblPIDList::getListHeadNode( &(sLobMeta->mAgingListBase) );

    while( sCurPageID != sMetaPID )
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
        IDE_TEST_CONT( sIsLastLimitResult == ID_TRUE, CONT_SKIP_BUILD_RECORDS );

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess)
                  != IDE_SUCCESS );
        sState = 2;

        sPhyPageHdr = sdpPhyPage::getHdr(sCurPagePtr);
        sLobNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sCurPagePtr);
        sNode       = sdpPhyPage::getDblPIDListNode( sPhyPageHdr );

        sDumpRow.mColumnIdx     = aColumnIdx;
        sDumpRow.mPageID        = sCurPageID;
        sDumpRow.mPageType      = sPhyPageHdr->mPageType;
        sDumpRow.mHeight        = sLobNodeHdr->mHeight;
        sDumpRow.mKeyCnt        = sLobNodeHdr->mKeyCnt;
        sDumpRow.mNodeSeq       = sLobNodeHdr->mNodeSeq;
        sDumpRow.mStoreSize     = sLobNodeHdr->mStoreSize;
        sDumpRow.mLobPageState  = sLobNodeHdr->mLobPageState;
        sDumpRow.mLobVersion    = sLobNodeHdr->mLobVersion;
        sDumpRow.mTSSlotSID     = sLobNodeHdr->mTSSlotSID;
        sDumpRow.mFstDskViewSCN = sLobNodeHdr->mFstDskViewSCN;
        sDumpRow.mPrevPID       = sdpDblPIDList::getPrvOfNode( sNode );
        sDumpRow.mNextPID       = sdpDblPIDList::getNxtOfNode( sNode );

        sCurPageID = sdpDblPIDList::getNxtOfNode( sNode );

        sState = 1;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( iduFixedTable::buildRecord(
                    aHeader,
                    aMemory,
                    (void *)&sDumpRow )
                  != IDE_SUCCESS );
    }

    // Meta Page 의 S_Latch를 해제한다.
    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                         sMetaPagePtr )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( CONT_SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   sCurPagePtr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   sMetaPagePtr )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/*------------------------------------------------------
 * 
 * D$DISK_TABLE_LOBINFO
 * 
 *------------------------------------------------------*/

static iduFixedTableColDesc gDumpDiskTableLobDescColDesc[]=
{
    {
        (SChar*)"HEAD_ROW_PIECE_PAGE_ID",
        offsetof(sdcDumpDiskTableLobInfo, mPageID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mPageID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"HEAD_ROW_PIECE_SLOT_NUM",
        offsetof(sdcDumpDiskTableLobInfo, mHdrSlotNum ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mHdrSlotNum ),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_ID",
        offsetof(sdcDumpDiskTableLobInfo, mColumnID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mColumnID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_PIECE_SIZE",
        offsetof(sdcDumpDiskTableLobInfo, mColumnPieceSize),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mColumnPieceSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LENGTH",
        offsetof(sdcDumpDiskTableLobInfo, mLength ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mLength ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_NULL",
        offsetof(sdcDumpDiskTableLobInfo, mIsNull),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mIsNull),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IN_OUT_MODE",
        offsetof(sdcDumpDiskTableLobInfo, mInOutMode ),
        SM_DUMP_VALUE_LENGTH,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOB_DESC_FLAG",
        offsetof(sdcDumpDiskTableLobInfo, mLobDescFlag ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mLobDescFlag ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_PAGE_SIZE",
        offsetof(sdcDumpDiskTableLobInfo, mLastPageSize ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mLastPageSize ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_PAGE_SEQ",
        offsetof(sdcDumpDiskTableLobInfo, mLastPageSeq ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mLastPageSeq ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOB_VERSION",
        offsetof(sdcDumpDiskTableLobInfo, mLobVersion ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mLobVersion ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ROOT_NODE_PID",
        offsetof(sdcDumpDiskTableLobInfo, mRootNodePID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mRootNodePID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DIRECT_CNT",
        offsetof(sdcDumpDiskTableLobInfo, mDirectCnt ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mDirectCnt ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DIRECT_00",
        offsetof(sdcDumpDiskTableLobInfo, mDirect[0]),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mDirect[0]),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
        {
        (SChar*)"DIRECT_01",
        offsetof(sdcDumpDiskTableLobInfo, mDirect[1]),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mDirect[1]),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DIRECT_02",
        offsetof(sdcDumpDiskTableLobInfo, mDirect[2]),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mDirect[2]),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DIRECT_03",
        offsetof(sdcDumpDiskTableLobInfo, mDirect[3]),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobInfo, mDirect[3]),
        IDU_FT_TYPE_INTEGER,
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

iduFixedTableDesc  gDumpDiskTableLobInfoTableDesc =
{
    (SChar *)"D$DISK_TABLE_LOBINFO",
    sdcFT::buildRecordDiskTableLobInfo,
    gDumpDiskTableLobDescColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

/*******************************************************
 * D$DISK_TABLE_LOBINFO Dump Table의 레코드 Build
 *******************************************************/
IDE_RC sdcFT::buildRecordDiskTableLobInfo( idvSQL              * /*aStatistics*/,
                                           void                * aHeader,
                                           void                * aDumpObj,
                                           iduFixedTableMemory * aMemory )
{
    UChar               * sSlotPtr;
    UChar               * sSlotDirPtr;
    UChar               * sCurPagePtr;
    scSpaceID             sSpaceID;
    scPageID              sCurPageID;
    smcTableHeader      * sTblHdr = NULL;
    UShort                sSlotNum;
    UInt                  sColumnCnt;
    UInt                  sLobColCnt;
    smiColumn             sColumns[SMI_COLUMN_ID_MAXIMUM];
    void                * sTrans;
    idBool                sIsSuccess;
    idBool                sIsPageLatchReleased = ID_TRUE;
    UShort                sSlotCount;
    sdcRowHdrInfo         sRowHdrInfo;
    sdpPhyPageHdr       * sPhyPageHdr;
    sdbMPRMgr             sMPRMgr;
    UInt                  sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sTblHdr = (smcTableHeader *)( (smpSlotHeader*)aDumpObj + 1);

    IDE_TEST_RAISE( (sTblHdr->mType != SMC_TABLE_NORMAL) &&
                    (sTblHdr->mType != SMC_TABLE_CATALOG),
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST_RAISE( (sTblHdr->mFlag & SMI_TABLE_TYPE_MASK)
                    != SMI_TABLE_DISK, ERR_INVALID_DUMP_OBJECT );

    sSpaceID = sTblHdr->mSpaceID;

    if ( aMemory->useExternalMemory() == ID_TRUE )
    {
        /* BUG-43006 FixedTable Indexing Filter */
        sTrans = ((smiFixedTableProperties *)aMemory->getContext())->mTrans;
    }
    else
    {
        sTrans  = ((smiIterator*)aMemory->getContext())->trans;
    }


    // LOB Column에 대한 Column정보를 미리 준비해 둔다.
    (void)getLobColInfoLst( sTblHdr, sColumns, &sColumnCnt, &sLobColCnt );

    // LOB Column이 최소한 하나는 있어야 한다.
    IDE_TEST_RAISE( sLobColCnt == 0, ERR_INVALID_DUMP_OBJECT );

    IDE_TEST( sMPRMgr.initialize( NULL, // idvSQL
                                  sSpaceID,
                                  sdpSegDescMgr::getSegHandle( 
                                      &(sTblHdr->mFixed.mDRDB) ),
                                  NULL ) /* mFilter */
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    // [ Build Lob Column Info ]
    // Table의 Page를 순서대로 읽어서 Head Row Piece를 탐색한다.
    // Head Row Piece를 발견하면 해당 Record의 Lob Column을
    // 하나씩 Fetch하여 얻은 정보를 Dump Tabe Record에 Build한다.

    while( 1 )
    {
        IDE_TEST( sMPRMgr.getNxtPageID( NULL, /*FilterData*/
                                        &sCurPageID )
                  != IDE_SUCCESS );

        if( sCurPageID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         sSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_MULTI_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess )
                  != IDE_SUCCESS );
        sIsPageLatchReleased = ID_FALSE;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        if( sPhyPageHdr->mPageType != SDP_PAGE_DATA )
        {
            sIsPageLatchReleased = ID_TRUE;
            IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                 sCurPagePtr )
                      != IDE_SUCCESS );

            continue;
        }

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPhyPageHdr );
        sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

        // [ Head Row Piece 탐색 ]
        for( sSlotNum = 0 ; sSlotNum < sSlotCount ; sSlotNum++ )
        {
            if( sdpSlotDirectory::isUnusedSlotEntry( sSlotDirPtr,
                                                     sSlotNum ) == ID_TRUE )
            {
                continue;
            }

            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                               sSlotNum,
                                                               &sSlotPtr )
                      != IDE_SUCCESS );

            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );

            if( SDC_IS_HEAD_ROWPIECE( sRowHdrInfo.mRowFlag ) != ID_TRUE )
            {
                continue;
            }

            // [ find Lob Column Info ]
            // Head Row Piece인 경우이다.
            // Lob Column을 Fetch해 온다.
            IDE_TEST( buildLobInfoDumpRec( aHeader,
                                           aMemory,
                                           sTrans,
                                           sSpaceID,
                                           sCurPageID,
                                           sSlotNum,
                                           sColumns,
                                           sLobColCnt,
                                           sTblHdr,
                                           &sCurPagePtr,
                                           &sIsPageLatchReleased )
                      != IDE_SUCCESS );
        }

        // buildLobInfoDumpRec 에서 Latch가 풀릴 경우 다시 잡습니다.
        // 본 위치에서는 Latch가 반드시 잡혀 있어야 합니다.
        IDE_ASSERT( sIsPageLatchReleased != ID_TRUE );

        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );

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

    IDE_PUSH();
    {
        if( sIsPageLatchReleased == ID_FALSE )
        {
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   sCurPagePtr )
                        == IDE_SUCCESS );
        }

        if( sState != 0 )
        {
            IDE_ASSERT( sMPRMgr.destroy() == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

/****************************************************************
 * Description : Lob Column의 Column Info를 구하는 함수
 *               D$DISK_TABLE_LOBINFO Dump Table에서만 사용된다.
 *
 *    aHeader     - [IN]
 *    aMemory     - [IN] Fixed Table Memory
 *    aTrans      - [IN] Transaction
 *    aSpaceID    - [IN] TableSpace ID
 *    aCurPageID  - [IN] 현재 Page ID
 *    aSlotNum    - [IN] Slot Number
 *    aColumns    - [IN] Lob Column 정보
 *    aLobColCnt  - [IN] Lob Column의 수
 *    aCurPagePtr - [OUT] 현재 Page의 Ptr
 *    aIsPageLatchReleased - [OUT] Page에 Latch를 잡혀있는지 여부
 ****************************************************************/
IDE_RC sdcFT::buildLobInfoDumpRec( void                * aHeader,
                                   iduFixedTableMemory * aMemory ,
                                   void                * aTrans,
                                   scSpaceID             aSpaceID,
                                   scPageID              aCurPageID,
                                   UShort                aSlotNum,
                                   smiColumn           * aColumns,
                                   UInt                  aLobColCnt,
                                   smcTableHeader      * aTblHdr,
                                   UChar              ** aCurPagePtr,
                                   idBool              * aIsPageLatchReleased )
{
    UInt                    sLobColIdx;
    UInt                    i;
    UChar                 * sSlotPtr;
    UChar                 * sSlotDirPtr;
    UChar                 * sCurPagePtr;
    idBool                  sIsSuccess;
    idBool                  sIsRowDeleted;
    idBool                  sIsPageLatchReleased = ID_FALSE ;
    idBool                  sIsLastLimitResult;
    UChar                   sLobColData[SDC_LOB_MAX_IN_MODE_SIZE];
    smiValue                sValue;
    sdcLobDesc              sLobDesc;
    sdcLobInfo4Fetch        sLobInfo4Fetch;
    smiFetchColumnList      sFetchColumn;
    sdcDumpDiskTableLobInfo sDumpLobInfo;

    IDE_ASSERT( aHeader      != NULL );
    IDE_ASSERT( aMemory      != NULL );
    IDE_ASSERT( aTrans       != NULL );
    IDE_ASSERT( aColumns     != NULL );
    IDE_ASSERT( aCurPagePtr  != NULL );
    IDE_ASSERT( *aCurPagePtr != NULL );
    IDE_ASSERT( aIsPageLatchReleased != NULL );

    sCurPagePtr = *aCurPagePtr;

    sFetchColumn.copyDiskColumn = (void*)sdcLob::copyLobColData;
    sFetchColumn.next           = NULL;

    sLobInfo4Fetch.mOpenMode    = SMI_LOB_READ_MODE;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sCurPagePtr );
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                       aSlotNum,
                                                       &sSlotPtr )
              != IDE_SUCCESS );

    // 해당 Record의 Lob Column들을 찾는다.
    sLobColIdx = 0;
    
    while( sLobColIdx < aLobColCnt )
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
        IDE_TEST_CONT( sIsLastLimitResult == ID_TRUE, CONT_SKIP_ROW );

        sFetchColumn.column    = &aColumns[ sLobColIdx ];
        sFetchColumn.columnSeq = SDC_GET_COLUMN_SEQ( &aColumns[ sLobColIdx ] );
        
        sValue.value  = sLobColData;
        sValue.length = 0;

        IDE_TEST( sdcRow::fetch( NULL, // Statistics,
                                 NULL, // aMtx
                                 NULL, // SP
                                 aTrans,
                                 aSpaceID,
                                 (UChar*)sSlotPtr,
                                 ID_TRUE,     // aIsPersSlot
                                 SDB_SINGLE_PAGE_READ,
                                 &sFetchColumn,
                                 SMI_FETCH_VERSION_LAST,
                                 SD_NULL_RID, // TssRID,
                                 NULL,        // SCN,
                                 NULL,        // InfiniteSCN,
                                 NULL,        // IndexInfo4Fetch,
                                 &sLobInfo4Fetch,
                                 aTblHdr->mRowTemplate,
                                 (UChar*)&sValue,
                                 &sIsRowDeleted,
                                 &sIsPageLatchReleased )
                  != IDE_SUCCESS );

        if( sIsPageLatchReleased == ID_TRUE )
        {
            // fetch시 Latch가 잡힌 Head Row Piece의 Pointer가
            // 필요하므로 Latch가 풀렸으면 다시 잡는다.
            IDE_TEST( sdbBufferMgr::getPage( NULL, // Statistics,
                                             aSpaceID,
                                             aCurPageID,
                                             SDB_S_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_MULTI_PAGE_READ,
                                             &sCurPagePtr,
                                             &sIsSuccess )
                      != IDE_SUCCESS );
            sIsPageLatchReleased = ID_FALSE;

            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( sCurPagePtr );
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                        sSlotDirPtr,
                                                        aSlotNum,
                                                        &sSlotPtr )
                      != IDE_SUCCESS );

            // page latch가 풀린 사이에 다른 트랜잭션이
            // 동일 페이지에 접근하여 변경 연산을 수행할 수 있다.
            // 다시 가져온다.
            if( sdpSlotDirectory::isUnusedSlotEntry( sSlotDirPtr,
                                                     aSlotNum )
                == ID_TRUE )
            {
                continue;
            }
        }

        // 삭제된 경우에는 다음 Row로 넘어간다.
        IDE_TEST_CONT( sIsRowDeleted == ID_TRUE , CONT_SKIP_ROW);

        // 읽어온 Column Data 로 Dump Table의 Record를 Build한다.

        idlOS::memset( &sDumpLobInfo, 0, ID_SIZEOF(sDumpLobInfo) );

        sDumpLobInfo.mPageID          = aCurPageID;
        sDumpLobInfo.mHdrSlotNum      = aSlotNum;
        sDumpLobInfo.mColumnID        = SDC_GET_COLUMN_SEQ( sFetchColumn.column ) ;
        sDumpLobInfo.mColumnPieceSize = sValue.length;

        if( SDC_IS_NULL(&sValue) == ID_TRUE )
        {
            sDumpLobInfo.mIsNull = 'T';
        }
        else
        {
            sDumpLobInfo.mIsNull = 'F';
        }

        if( sLobInfo4Fetch.mInOutMode == SDC_COLUMN_IN_MODE )
        {
            sDumpLobInfo.mInOutMode = (UChar*)"IN MODE";

            sDumpLobInfo.mLength       = sValue.length;
            sDumpLobInfo.mLobDescFlag  = 0;
            sDumpLobInfo.mLastPageSize = 0;
            sDumpLobInfo.mLastPageSeq  = 0;
            sDumpLobInfo.mLobVersion   = 0;
            sDumpLobInfo.mRootNodePID  = SD_NULL_PID;
            sDumpLobInfo.mDirectCnt    = 0;

            for( i = 0; i < SDC_LOB_MAX_DIRECT_PAGE_CNT; i++ )
            {
                sDumpLobInfo.mDirect[i] = SD_NULL_PID;
            }
        }
        else
        {
            IDE_ASSERT( sValue.length == ID_SIZEOF(sdcLobDesc) );

            idlOS::memcpy( &sLobDesc, sLobColData, ID_SIZEOF(sdcLobDesc) );

            sDumpLobInfo.mInOutMode = (UChar*)"OUT MODE";

            sDumpLobInfo.mLength       = sdcLob::getLobLengthFromLobDesc(&sLobDesc);
            sDumpLobInfo.mLobDescFlag  = sLobDesc.mLobDescFlag;
            sDumpLobInfo.mLastPageSize = sLobDesc.mLastPageSize;
            sDumpLobInfo.mLastPageSeq  = sLobDesc.mLastPageSeq;
            sDumpLobInfo.mLobVersion   = sLobDesc.mLobVersion;
            sDumpLobInfo.mRootNodePID  = sLobDesc.mRootNodePID;
            sDumpLobInfo.mDirectCnt    = sLobDesc.mDirectCnt;

            for( i = 0; i < SDC_LOB_MAX_DIRECT_PAGE_CNT; i++ )
            {
                sDumpLobInfo.mDirect[i] = sLobDesc.mDirect[i];
            }
        }

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sDumpLobInfo )
                  != IDE_SUCCESS );

        sLobColIdx++;
    }

    IDE_EXCEPTION_CONT( CONT_SKIP_ROW );

    *aIsPageLatchReleased = sIsPageLatchReleased;
    *aCurPagePtr = sCurPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsPageLatchReleased = sIsPageLatchReleased;
    *aCurPagePtr = sCurPagePtr;

    return IDE_FAILURE;
}

/*******************************************************
 * Description : Lob Column의 Column Info를 구하는 함수
 *               D$DISK_TABLE_LOBINFO Dump Table에서만 사용된다.
 *
 *   aTblHdr    - [IN]  Table Header
 *   aColumns   - [OUT] Lob Column Info의 Array
 *   aColumnCnt - [OUT] Table의 Column의 수
 *   aLobColCnt - [OUT] Table의 Lob Column의 수
 *******************************************************/
void sdcFT::getLobColInfoLst( smcTableHeader      * aTblHdr,
                              smiColumn           * aColumns,
                              UInt                * aColumnCnt,
                              UInt                * aLobColCnt )
{
    UInt              sLoop;
    UInt              sLobColIdx;
    UInt              sLobColCnt;
    UInt              sColumnCnt;
    const smiColumn * sSrcColumn;
    smiColumn       * sDstColumn;

    IDE_ASSERT( aTblHdr    != NULL ) ;
    IDE_ASSERT( aColumns   != NULL );
    IDE_ASSERT( aColumnCnt != NULL );
    IDE_ASSERT( aLobColCnt != NULL );

    sLobColCnt = aTblHdr->mLobColumnCount;
    sColumnCnt = aTblHdr->mColumnCount;

    // BUG-27328 CodeSonar::Uninitialized Variable
    IDE_ASSERT( sLobColCnt <= sColumnCnt ); 

    if( sLobColCnt > 0 )
    {
        sLobColIdx = 0;

        for( sLoop = 0 ; sLoop < sColumnCnt ; sLoop++ )
        {
            sSrcColumn = smcTable::getColumn( aTblHdr, sLoop );

            IDE_ASSERT( sSrcColumn != NULL );

            if( SDC_IS_LOB_COLUMN( sSrcColumn ) == ID_FALSE )
            {
                continue;
            }

            sDstColumn = &aColumns[ sLobColIdx ];

            idlOS::memcpy( (void*)sDstColumn,
                           (void*)sSrcColumn,
                           ID_SIZEOF(smiColumn));

            sDstColumn->offset = 0;

            sLobColIdx++;

            if( sLobColIdx >= sLobColCnt )
            {
                break;
            }
        }
    }

    *aColumnCnt = sColumnCnt;
    *aLobColCnt = sLobColCnt;

    return;
}


/*------------------------------------------------------
 * 
 * D$DISK_TABLE_LOB_META
 * 
 *------------------------------------------------------*/

static iduFixedTableColDesc gDumpDiskTableLobMetaColDesc[]=
{
    {
        (SChar*)"COLUMN_IDX",
        offsetof(sdcDumpDiskTableLobMeta, mColumnIdx ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mColumnIdx ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SEG_PID",
        offsetof(sdcDumpDiskTableLobMeta, mSegPID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mSegPID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOGGING",
        offsetof(sdcDumpDiskTableLobMeta, mLogging ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mLogging ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"BUFFER",
        offsetof(sdcDumpDiskTableLobMeta, mBuffer ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mBuffer ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COLUMN_ID",
        offsetof(sdcDumpDiskTableLobMeta, mColumnID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mColumnID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NODE_CNT",
        offsetof(sdcDumpDiskTableLobMeta, mNodeCnt ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mNodeCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREV_PID",
        offsetof(sdcDumpDiskTableLobMeta, mPrevPID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mPrevPID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_PID",
        offsetof(sdcDumpDiskTableLobMeta, mNextPID ),
        IDU_FT_SIZEOF(sdcDumpDiskTableLobMeta, mNextPID ),
        IDU_FT_TYPE_INTEGER,
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

iduFixedTableDesc  gDumpDiskTableLobMetaTableDesc =
{
    (SChar *)"D$DISK_TABLE_LOB_META",
    sdcFT::buildRecord4LobMeta,
    gDumpDiskTableLobMetaColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC sdcFT::buildRecord4LobMeta( idvSQL              * /*aStatistics*/,
                                   void                * aHeader,
                                   void                * aDumpObj,
                                   iduFixedTableMemory * aMemory )
{
    smcTableHeader  * sTblHdr;
    const smiColumn * sColumn;
    UInt              sColumnCnt;
    UInt              sColumnIdx;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_INVALID_DUMP_OBJECT );

    sTblHdr = (smcTableHeader*)((smpSlotHeader*)aDumpObj + 1);

    sColumnCnt = smcTable::getColumnCount( sTblHdr );

    for( sColumnIdx = 0; sColumnIdx < sColumnCnt; sColumnIdx++ )
    {
        sColumn = smcTable::getColumn( sTblHdr, sColumnIdx );

        if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
        {
            IDE_TEST( dumpLobMeta( sColumn->colSeg.mSpaceID,
                                   sColumn->colSeg.mPageID,
                                   sColumnIdx,
                                   aHeader,
                                   aMemory )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcFT::dumpLobMeta( scSpaceID             aSpaceID,
                           scPageID              aPageID,
                           UInt                  aColumnIdx,
                           void                * aHeader,
                           iduFixedTableMemory * aMemory )
{
    sdcDumpDiskTableLobMeta   sDumpRow;
    sdcLobMeta              * sLobMeta;
    sdpSegInfo                sSegInfo;
    sdpSegMgmtOp            * sSegMgmtOp;
    scSpaceID                 sSpaceID;
    scPageID                  sSegPID;
    scPageID                  sMetaPID;
    idBool                    sIsLastLimitResult;
    idBool                    sIsSuccess;
    UChar                   * sMetaPagePtr;
    SInt                      sState = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

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

    sSpaceID = aSpaceID;
    sSegPID  = aPageID;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( sSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo( NULL,
                                             sSpaceID,
                                             sSegPID,
                                             NULL, /* aTableHeader */
                                             &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST( sSegMgmtOp->mGetMetaPID( NULL, /* idvSQL */
                                       sSpaceID,
                                       sSegPID,
                                       0, /* Seg Meta PID Array Index */
                                       &sMetaPID )
              != IDE_SUCCESS );

    IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                     sSpaceID,
                                     sMetaPID,
                                     SDB_S_LATCH,
                                     SDB_WAIT_NORMAL,
                                     SDB_SINGLE_PAGE_READ,
                                     (UChar**)&sMetaPagePtr,
                                     &sIsSuccess )
              != IDE_SUCCESS );
    sState = 1;

    sLobMeta    = (sdcLobMeta*)sdpPhyPage::getLogicalHdrStartPtr( sMetaPagePtr );

    sDumpRow.mColumnIdx = aColumnIdx;
    sDumpRow.mSegPID    = sSegPID;
    sDumpRow.mLogging   = sLobMeta->mLogging == ID_TRUE ? 'Y' : 'N';
    sDumpRow.mBuffer    = sLobMeta->mBuffer == ID_TRUE ? 'Y' : 'N';
    sDumpRow.mColumnID  = sLobMeta->mColumnID;
    sDumpRow.mNodeCnt   = sdpDblPIDList::getNodeCnt( &(sLobMeta->mAgingListBase) );
    sDumpRow.mPrevPID   = sdpDblPIDList::getListTailNode( &(sLobMeta->mAgingListBase) );
    sDumpRow.mNextPID   = sdpDblPIDList::getListHeadNode( &(sLobMeta->mAgingListBase) );
    
    IDE_TEST( iduFixedTable::buildRecord(
                  aHeader,
                  aMemory,
                  (void *)&sDumpRow )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                         sMetaPagePtr )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_BUILD_RECORDS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DUMP_OBJECT);
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_INVALID_DUMP_OBJECT ));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   sMetaPagePtr )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}
