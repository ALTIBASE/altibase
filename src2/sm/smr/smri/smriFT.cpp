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
 * $Id$
 *
 * Incremental backup관련 fixed table 구현
 **********************************************************************/

#include <idl.h>
#include <smriDef.h>
#include <smriBackupInfoMgr.h>
#include <smrRecoveryMgr.h>


typedef struct smriBackupInfo
{
    SChar               mBeginBackupTime[ SMRI_BI_TIME_LEN ];
    SChar               mEndBackupTime[ SMRI_BI_TIME_LEN ];
    UInt                mIBChunkCNT;
    smriBIBackupTarget  mBackupTarget;
    smiBackupLevel      mBackupLevel;
    UShort              mBackupType;
    scSpaceID           mSpaceID;
    UInt                mFileID;
    SChar               mBackupTag[SMI_MAX_BACKUP_TAG_NAME_LEN];
    SChar               mBackupFileName[SMRI_BI_MAX_BACKUP_FILE_NAME_LEN];
} smriBackupInfo;

static iduFixedTableColDesc gBackupInfoColDesc[] =
{
    {  
        (SChar*)"BEGIN_BACKUP_TIME",
        IDU_FT_OFFSETOF(smriBackupInfo, mBeginBackupTime),
        IDU_FT_SIZEOF(smriBackupInfo, mBeginBackupTime),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"END_BACKUP_TIME",
        IDU_FT_OFFSETOF(smriBackupInfo, mEndBackupTime),
        IDU_FT_SIZEOF(smriBackupInfo, mEndBackupTime),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"INCREMENTAL_BACKUP_CHUNK_COUNT",
        IDU_FT_OFFSETOF(smriBackupInfo, mIBChunkCNT),
        IDU_FT_SIZEOF(smriBackupInfo, mIBChunkCNT),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"BACKUP_TARGET",
        IDU_FT_OFFSETOF(smriBackupInfo, mBackupTarget),
        IDU_FT_SIZEOF(smriBackupInfo, mBackupTarget),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"BACKUP_LEVEL",
        IDU_FT_OFFSETOF(smriBackupInfo, mBackupLevel),
        IDU_FT_SIZEOF(smriBackupInfo, mBackupLevel),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"BACKUP_TYPE",
        IDU_FT_OFFSETOF(smriBackupInfo, mBackupType),
        IDU_FT_SIZEOF(smriBackupInfo, mBackupType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"TABLESPACE_ID",
        IDU_FT_OFFSETOF(smriBackupInfo, mSpaceID),
        IDU_FT_SIZEOF(smriBackupInfo, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"FILE_ID",
        IDU_FT_OFFSETOF(smriBackupInfo, mFileID),
        IDU_FT_SIZEOF(smriBackupInfo, mFileID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"BACKUP_TAG",
        IDU_FT_OFFSETOF(smriBackupInfo, mBackupTag),
        IDU_FT_SIZEOF(smriBackupInfo, mBackupTag),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    }, 
    {  
        (SChar*)"BACKUP_FILE",
        IDU_FT_OFFSETOF(smriBackupInfo, mBackupFileName),
        IDU_FT_SIZEOF(smriBackupInfo, mBackupFileName),
        IDU_FT_TYPE_CHAR,
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

static IDE_RC makeBackupInfoRecord( void                * aHeader, 
                                    iduFixedTableMemory * aMemory, 
                                    UInt                  aEndBISlotIdx )
{
    smriBISlot    * sBISlot;
    UInt            sBISlotIdx;
    smriBackupInfo  sBackupInfo;
    idvSQL        * sStatistics;
    time_t          sTime;
    struct tm       sBackupTime;

    idlOS::memset( &sBackupTime, 0, ID_SIZEOF(struct tm) );

    if ( aMemory->useExternalMemory() == ID_FALSE )
    {
        sStatistics = ((smiIterator *)aMemory->getContext())->properties->mStatistics;
    }
    else
    {
        sStatistics = ((smiFixedTableProperties *)aMemory->getContext())->mStatistics;
    }

    for( sBISlotIdx = 0; sBISlotIdx < aEndBISlotIdx; sBISlotIdx++ )
    {
        IDE_TEST(iduCheckSessionEvent(sStatistics) != IDE_SUCCESS );

        IDE_TEST( smriBackupInfoMgr::getBISlot( sBISlotIdx, &sBISlot ) != IDE_SUCCESS );

        sTime = (time_t)sBISlot->mBeginBackupTime;
        idlOS::localtime_r( (time_t *)&sTime, &sBackupTime );
        idlOS::snprintf( sBackupInfo.mBeginBackupTime, 
                         SMRI_BI_TIME_LEN,
                         "%04"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         " %02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT,
                         sBackupTime.tm_year + 1900,
                         sBackupTime.tm_mon + 1,
                         sBackupTime.tm_mday,
                         sBackupTime.tm_hour,
                         sBackupTime.tm_min,
                         sBackupTime.tm_sec );

        sTime = (time_t)sBISlot->mEndBackupTime;
        idlOS::localtime_r( (time_t *)&sTime, &sBackupTime );
        idlOS::snprintf( sBackupInfo.mEndBackupTime,
                         SMRI_BI_TIME_LEN,
                         "%04"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         "-%02"ID_UINT32_FMT
                         " %02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT
                         ":%02"ID_UINT32_FMT,
                         sBackupTime.tm_year + 1900,
                         sBackupTime.tm_mon + 1,
                         sBackupTime.tm_mday,
                         sBackupTime.tm_hour,
                         sBackupTime.tm_min,
                         sBackupTime.tm_sec );         

        sBackupInfo.mIBChunkCNT         = sBISlot->mIBChunkCNT;
        sBackupInfo.mBackupTarget       = sBISlot->mBackupTarget;
        sBackupInfo.mBackupLevel        = sBISlot->mBackupLevel;
        sBackupInfo.mBackupType         = sBISlot->mBackupType;
        sBackupInfo.mSpaceID            = sBISlot->mSpaceID;
        sBackupInfo.mFileID             = sBISlot->mFileID;

        idlOS::strncpy( sBackupInfo.mBackupTag, 
                        sBISlot->mBackupTag, 
                        SMI_MAX_BACKUP_TAG_NAME_LEN );

        idlOS::strncpy( sBackupInfo.mBackupFileName, 
                        sBISlot->mBackupFileName,
                        SMRI_BI_MAX_BACKUP_FILE_NAME_LEN );

        IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                             aMemory,
                                             (void *)&sBackupInfo )
                 != IDE_SUCCESS);
    } 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC buildRecordForBackupInfo(idvSQL              * /*aStatistics*/,
                                       void                *aHeader,
                                       void                */*aDumpObj*/,
                                       iduFixedTableMemory *aMemory)
{
    smriBIFileHdr * sBIFileHdr;
    UInt            sState = 0;

    IDE_TEST_CONT( smrRecoveryMgr::getBIMgrState() != SMRI_BI_MGR_INITIALIZED,
                    skip_build_record );

    IDE_TEST( smriBackupInfoMgr::lock() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smriBackupInfoMgr::loadBISlotArea() != IDE_SUCCESS );
    sState = 2;
    
    (void)smriBackupInfoMgr::getBIFileHdr( &sBIFileHdr );

    IDE_TEST( makeBackupInfoRecord( aHeader, 
                                    aMemory, 
                                    sBIFileHdr->mBISlotCnt ) 
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( smriBackupInfoMgr::unloadBISlotArea() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smriBackupInfoMgr::unlock() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_build_record );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( smriBackupInfoMgr::unloadBISlotArea() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smriBackupInfoMgr::unlock() == IDE_SUCCESS );
        case 0:
        default:
            break;
    }
    
    return IDE_FAILURE;
}

iduFixedTableDesc gBackupInfoDesc =
{
    (SChar *)"X$BACKUP_INFO",
    buildRecordForBackupInfo,
    gBackupInfoColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static IDE_RC buildRecordForObsoleteBackupInfo(idvSQL              * /*aStatistics*/,
                                               void                *aHeader,
                                               void                */*aDumpObj*/,
                                               iduFixedTableMemory *aMemory)
{
    UInt            sValidBISlotIdx;
    UInt            sState = 0;

    IDE_TEST_CONT( smrRecoveryMgr::getBIMgrState() != SMRI_BI_MGR_INITIALIZED,
                    skip_build_record );

    IDE_TEST( smriBackupInfoMgr::lock() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smriBackupInfoMgr::loadBISlotArea() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( smriBackupInfoMgr::getValidBISlotIdx( &sValidBISlotIdx ) 
              != IDE_SUCCESS );
    IDE_TEST_CONT( sValidBISlotIdx == SMRI_BI_INVALID_SLOT_IDX, 
                    there_is_no_obsolete_backup_info);
    
    IDE_TEST( makeBackupInfoRecord( aHeader, 
                                    aMemory, 
                                    sValidBISlotIdx ) 
              != IDE_SUCCESS );
    
    IDE_EXCEPTION_CONT( there_is_no_obsolete_backup_info );

    sState = 1;
    IDE_TEST( smriBackupInfoMgr::unloadBISlotArea() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smriBackupInfoMgr::unlock() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_build_record );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2: 
            IDE_ASSERT( smriBackupInfoMgr::unloadBISlotArea() == IDE_SUCCESS );
        case 1: 
            IDE_ASSERT( smriBackupInfoMgr::unlock() == IDE_SUCCESS );
        case 0: 
        default:
            break;
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gObsoleteBackupInfoDesc =
{
    (SChar *)"X$OBSOLETE_BACKUP_INFO",
    buildRecordForObsoleteBackupInfo,
    gBackupInfoColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
