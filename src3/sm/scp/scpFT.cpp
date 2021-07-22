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
 * $Id: scpFT.cpp 39271 2010-05-06 07:08:00Z eerien $
 *
 * Description :
 *
 * 본 파일은 Common-DataPort Layer의 Fixed Table 구현파일 입니다.
 *
 **********************************************************************/

#include <smDef.h>
#include <scpDef.h>
#include <scpFT.h>
#include <scpManager.h>

typedef struct scpHandleRow
{
    SChar              mJobName[ SMI_DATAPORT_JOBNAME_SIZE ];
    UInt               mTypeIdx;
    UInt               mVersion;
    idBool             mIsBigEndian;
    UInt               mCompileBit;
    SChar            * mDBCharSet;
    SChar            * mNationalCharSet;
    UInt               mPartitionCount;
    UInt               mColumnCount;
    UInt               mBasicColumnCount;
    UInt               mLobColumnCount;
} scpHandleRow;



/***********************************************************************
 * Description : X$DATAPORT의 레코드를 구성한다.
 ***********************************************************************/

IDE_RC scpFT::buildRecord4DataPort( idvSQL              * /*aStatistics*/,
                                    void                *aHeader,
                                    void                * /*aDumpObj*/,
                                    iduFixedTableMemory *aMemory )
{
    smuList               * sOpNode;
    smuList               * sBaseNode;
    idBool                  sMutexLocked = ID_FALSE;
    scpHandle             * sHandle;
    smiDataPortHeader     * sHeader;
    scpHandleRow            sRow;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST( scpManager::lock() != IDE_SUCCESS );
    sMutexLocked = ID_TRUE;

    sBaseNode = scpManager::getListHead();

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
          sOpNode != sBaseNode;
          sOpNode = SMU_LIST_GET_NEXT(sOpNode) )
    {
        sHandle = (scpHandle*) sOpNode->mData;
        sHeader = sHandle->mHeader;

        idlOS::strncpy( (SChar*)sRow.mJobName,
                        (SChar*)sHandle->mJobName,
                        SMI_DATAPORT_JOBNAME_SIZE );
        sRow.mTypeIdx           = sHandle->mType;
        sRow.mVersion           = sHeader->mVersion;           
        sRow.mIsBigEndian       = sHeader->mIsBigEndian;
        sRow.mCompileBit        = sHeader->mCompileBit; 
        sRow.mDBCharSet         = sHeader->mDBCharSet;
        sRow.mNationalCharSet   = sHeader->mNationalCharSet;
        sRow.mPartitionCount    = sHeader->mPartitionCount;   
        sRow.mColumnCount       = sHeader->mColumnCount; 
        sRow.mBasicColumnCount  = sHeader->mBasicColumnCount;
        sRow.mLobColumnCount    = sHeader->mLobColumnCount;

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

static iduFixedTableColDesc gDataPortDesc[]=
{
    {
        (SChar*)"JOBNAME",
        offsetof(scpHandleRow, mJobName),
        IDU_FT_SIZEOF(scpHandleRow, mJobName),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"TYPE",
        offsetof(scpHandleRow, mTypeIdx),
        IDU_FT_SIZEOF(scpHandleRow, mTypeIdx),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"VERSION",
        offsetof(scpHandleRow, mVersion),
        IDU_FT_SIZEOF(scpHandleRow, mVersion),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BIG_ENDIAN",
        offsetof(scpHandleRow, mIsBigEndian),
        IDU_FT_SIZEOF(scpHandleRow, mIsBigEndian),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"COMPILE_BIT",
        offsetof(scpHandleRow, mCompileBit),
        IDU_FT_SIZEOF(scpHandleRow, mCompileBit),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"DB_CHARSET",
        offsetof(scpHandleRow, mDBCharSet),
        IDN_MAX_CHAR_SET_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NATIONAL_CHARSET",
        offsetof(scpHandleRow, mNationalCharSet),
        IDN_MAX_CHAR_SET_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PARTITION_COUNT",
        offsetof(scpHandleRow, mPartitionCount),
        IDU_FT_SIZEOF(scpHandleRow, mPartitionCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"COLUMN_COUNT",
        offsetof(scpHandleRow, mColumnCount),
        IDU_FT_SIZEOF(scpHandleRow, mColumnCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"BASIC_COLUMN_COUNT",
        offsetof(scpHandleRow, mBasicColumnCount),
        IDU_FT_SIZEOF(scpHandleRow, mBasicColumnCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"LOB_COLUMN_COUNT",
        offsetof(scpHandleRow, mLobColumnCount),
        IDU_FT_SIZEOF(scpHandleRow, mLobColumnCount),
        IDU_FT_TYPE_UINTEGER,
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

// X$TableExctractor fixed table def
iduFixedTableDesc gDataPort =
{
    (SChar *)"X$DATAPORT",
    scpFT::buildRecord4DataPort,
    gDataPortDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

