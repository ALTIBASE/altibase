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
 * $Id: scpfFT.cpp 39271 2010-05-06 07:08:00Z eerien $
 *
 * Description :
 *
 * 본 파일은 Common-DataPort File Layer의 Fixed Table 구현파일 입니다.
 *
 **********************************************************************/

#include <smDef.h>
#include <scpDef.h>
#include <scpfDef.h>
#include <scpfFT.h>
#include <scpManager.h>

/***********************************************************************
 * Description : X$DATAPORT_FILE_HEADER의 레코드를 구성한다.
 ***********************************************************************/

IDE_RC scpfFT::buildRecord4DataPortFileHeader( 
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                * /*aDumpObj*/,
    iduFixedTableMemory *aMemory )
{
    smuList               * sOpNode;
    smuList               * sBaseNode;
    idBool                  sMutexLocked = ID_FALSE;
    scpfHandle            * sHandle;
    scpfHeader            * sHeader;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( scpManager::lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    sBaseNode = scpManager::getListHead();

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = SMU_LIST_GET_NEXT(sOpNode) )
    {
        sHandle = (scpfHandle*) sOpNode->mData;
        sHeader = &sHandle->mFileHeader;

        // FileType만 취급한다.
        if( sHandle->mCommonHandle.mType != SMI_DATAPORT_TYPE_FILE )
        {
            continue;
        }

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)sHeader )
                  != IDE_SUCCESS);
    }

    sMutexLocked = ID_FALSE;
    IDE_TEST( scpManager::unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMutexLocked == ID_TRUE )
    {
        IDE_ASSERT( scpManager::unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc gDataPortFileHeaderDesc[]=
{
    {
        (SChar*)"FILE_HEADERSIZE",
        offsetof(scpfHeader, mFileHeaderSize),
        IDU_FT_SIZEOF(scpfHeader, mFileHeaderSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_HEADERSIZE",
        offsetof(scpfHeader, mBlockHeaderSize),
        IDU_FT_SIZEOF(scpfHeader, mBlockHeaderSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_SIZE",
        offsetof(scpfHeader, mBlockSize),
        IDU_FT_SIZEOF(scpfHeader, mBlockSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_INFO_COUNT",
        offsetof(scpfHeader, mBlockInfoCount),
        IDU_FT_SIZEOF(scpfHeader, mBlockInfoCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"MAX_ROW_COUNT_IN_BLOCK",
        offsetof(scpfHeader, mMaxRowCountInBlock),
        IDU_FT_SIZEOF(scpfHeader, mMaxRowCountInBlock),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_COUNT",
        offsetof(scpfHeader, mBlockCount),
        IDU_FT_SIZEOF(scpfHeader, mBlockCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"ROW_COUNT",
        offsetof(scpfHeader, mRowCount),
        IDU_FT_SIZEOF(scpfHeader, mRowCount),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"FIRST_ROW_SEQ_IN_FILE",
        offsetof(scpfHeader, mFirstRowSeqInFile),
        IDU_FT_SIZEOF(scpfHeader, mFirstRowSeqInFile),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"LAST_ROW_SEQ_IN_FILE",
        offsetof(scpfHeader, mLastRowSeqInFile),
        IDU_FT_SIZEOF(scpfHeader, mLastRowSeqInFile),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL // for internal use
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

// X$DATAPORT_FILE_HEADER fixed table def
iduFixedTableDesc gDataPortFileHeader =
{
    (SChar *)"X$DATAPORT_FILE_HEADER",
    scpfFT::buildRecord4DataPortFileHeader,
    gDataPortFileHeaderDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/***********************************************************************
 * Description : X$DATAPORT_FILE_CURSOR의 레코드를 구성한다.
 ***********************************************************************/

typedef struct scpfCursorRow4Dump
{
    UInt       mBlockSeq;       // 읽고있는 Block Info내 Block 번호.
    UInt       mBlockID;        // 현재 읽고있는 Block의 File내 번호.
    UInt       mBlockOffset;    // Block내 Offset 
    UInt       mBlockSlotSeq;   // Block내 Slot번호
    UInt       mFileIdx;        // 현재 다루고 있는 파일의 번호
    ULong      mFileOffset;     // 현재까지 읽거나 쓴 FileOffset
} scpfCursorRow4Dump;


IDE_RC scpfFT::buildRecord4DataPortFileCursor( 
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * /*aDumpObj*/,
    iduFixedTableMemory * aMemory )
{
    smuList               * sOpNode;
    smuList               * sBaseNode;
    idBool                  sMutexLocked = ID_FALSE;
    scpfHandle            * sHandle;
    scpfCursorRow4Dump      sRow;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( scpManager::lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    sBaseNode = scpManager::getListHead();

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = SMU_LIST_GET_NEXT(sOpNode) )
    {
        sHandle = (scpfHandle*) sOpNode->mData;

        // FileType만 취급한다.
        if( sHandle->mCommonHandle.mType != SMI_DATAPORT_TYPE_FILE )
        {
            continue;
        }

        sRow.mBlockSeq      = sHandle->mBlockBufferPosition.mSeq;
        sRow.mBlockID       = sHandle->mBlockBufferPosition.mID;
        sRow.mBlockOffset   = sHandle->mBlockBufferPosition.mOffset;
        sRow.mBlockSlotSeq  = sHandle->mBlockBufferPosition.mSlotSeq;
        sRow.mFileIdx       = sHandle->mFilePosition.mIdx;
        sRow.mFileOffset    = sHandle->mFilePosition.mOffset;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sRow )
                  != IDE_SUCCESS);
    }

    sMutexLocked = ID_FALSE;
    IDE_TEST( scpManager::unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMutexLocked == ID_TRUE )
    {
        IDE_ASSERT( scpManager::unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc gDataPortFileCursorDesc[]=
{
    {
        (SChar*)"BLOCK_SEQ_IN_MEMORY",
        offsetof(scpfCursorRow4Dump, mBlockSeq),
        IDU_FT_SIZEOF(scpfCursorRow4Dump, mBlockSeq),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_ID",
        offsetof(scpfCursorRow4Dump, mBlockID),
        IDU_FT_SIZEOF(scpfCursorRow4Dump, mBlockID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_OFFSET",
        offsetof(scpfCursorRow4Dump, mBlockOffset),
        IDU_FT_SIZEOF(scpfCursorRow4Dump, mBlockOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BLOCK_SLOT_SEQ",
        offsetof(scpfCursorRow4Dump, mBlockSlotSeq),
        IDU_FT_SIZEOF(scpfCursorRow4Dump, mBlockSlotSeq),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"FILE_IDX",
        offsetof(scpfCursorRow4Dump, mFileIdx),
        IDU_FT_SIZEOF(scpfCursorRow4Dump, mFileIdx),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"FILE_OFFSET",
        offsetof(scpfCursorRow4Dump, mFileOffset),
        IDU_FT_SIZEOF(scpfCursorRow4Dump, mFileOffset),
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
        0, 0, NULL // for internal use
    }
};

// X$DATAPORT_FILE_CURSOR fixed table def
iduFixedTableDesc gDataPortFileCursor =
{
    (SChar *)"X$DATAPORT_FILE_CURSOR",
    scpfFT::buildRecord4DataPortFileCursor,
    gDataPortFileCursorDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

