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
 * $Id: svcFT.cpp 90083 2021-02-26 00:58:48Z et16 $
 *
 * Description
 *
 *   PROJ-1407
 *   Volatile Table Records를 DUMP하기 위한 함수
 *
 **********************************************************************/

#include <svmManager.h>
#include <svpManager.h>
#include <svpVarPageList.h>
#include <smcFT.h>
#include <svcFT.h>

/***********************************************************************
 * Description
 *
 *   D$VOL_TABLE_RECORD
 *   : VOLATILE TABLE의 Record 내용을 출력
 *
 *
 **********************************************************************/

extern smiGlobalCallBackList gSmiGlobalCallBackList;

//------------------------------------------------------
// D$VOL_TABLE_RECORD Dump Table의 Column Description
//------------------------------------------------------

static iduFixedTableColDesc gDumpVolTableRecordColDesc[]=
{
    {
        (SChar*)"PAGE_ID",
        offsetof(smcDumpMemTableRow, mPageID ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mPageID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"OFFSET",
        offsetof(smcDumpMemTableRow, mOffset ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mOffset ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_TID",
        offsetof(smcDumpMemTableRow, mCreateTID ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mCreateTID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_SCN",
        offsetof(smcDumpMemTableRow, mCreateSCN ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_TID",
        offsetof(smcDumpMemTableRow, mLimitTID ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mLimitTID ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LIMIT_SCN",
        offsetof(smcDumpMemTableRow, mLimitSCN ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NEXT_OID",
        offsetof(smcDumpMemTableRow, mNext ),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FLAG",
        offsetof(smcDumpMemTableRow, mFlag ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mFlag ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_FLAG",
        offsetof(smcDumpMemTableRow, mUsedFlag ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mUsedFlag ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DROP_FLAG",
        offsetof(smcDumpMemTableRow, mDropFlag ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mDropFlag ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SKIP_REFINE_FLAG",
        offsetof(smcDumpMemTableRow, mSkipRefineFlag ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mSkipRefineFlag ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_SLOT",
        offsetof(smcDumpMemTableRow, mNthSlot ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mNthSlot ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NTH_COLUMN",
        offsetof(smcDumpMemTableRow, mNthColumn ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mNthColumn ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"VALUE24B",
        offsetof(smcDumpMemTableRow, mValue ),
        IDU_FT_SIZEOF(smcDumpMemTableRow, mValue ),
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
// D$VOL_TABLE_RECORD Dump Table의 Table Description
//------------------------------------------------------

iduFixedTableDesc  gDumpVolTableRecordTableDesc =
{
    (SChar *)"D$VOL_TABLE_RECORD",
    svcFT::buildRecordVolTableRecord,
    gDumpVolTableRecordColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$VOL_TABLE_RECORD Dump Table의 레코드 Build
//------------------------------------------------------

IDE_RC svcFT::buildRecordVolTableRecord( idvSQL              * /*aStatistics*/,
                                         void                * aHeader,
                                         void                * aDumpObj,
                                         iduFixedTableMemory * aMemory )
{
    smcTableHeader      * sTblHdr = NULL;
    scPageID              sCurPageID;
    scPageID              sLstPageID;
    SChar               * sPagePtr;
    smpSlotHeader       * sSlotHeader;
    smcDumpMemTableRow    sDumpRecord;
    smpPageListEntry    * sFixedPageList;
    idBool                sLocked = ID_FALSE;
    SChar               * sRowPtr;
    SChar               * sFence;
    UInt                  i;
    UInt                  sSlotSeq;
    SChar                 sStrCreateSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar                 sStrLimitSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar                 sNext[ SM_SCN_STRING_LENGTH + 1 ];

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sTblHdr = (smcTableHeader *)( (smpSlotHeader*)aDumpObj + 1);

    IDE_TEST_RAISE( sTblHdr->mType != SMC_TABLE_NORMAL,
                    ERR_INVALID_DUMP_OBJECT );

    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_VOLATILE( sTblHdr ) == ID_FALSE,
                    ERR_INVALID_DUMP_OBJECT );

    //------------------------------------------
    // Get Table Info
    //------------------------------------------
    sFixedPageList = (smpPageListEntry*)&(sTblHdr->mFixed);
    sCurPageID     = svpManager::getFirstAllocPageID(sFixedPageList);
    sLstPageID     = svpManager::getLastAllocPageID(sFixedPageList);

    //------------------------------------------
    // Get Table Records
    //------------------------------------------
    while( 1 )
    {
        sSlotSeq = 0;
        IDE_TEST( svmManager::holdPageSLatch(sTblHdr->mSpaceID,
                                             sCurPageID)
                  != IDE_SUCCESS );
        sLocked = ID_TRUE;

        IDE_ASSERT( smmManager::getPersPagePtr( sTblHdr->mSpaceID,
                                                sCurPageID,
                                                (void**)&sPagePtr )
                    == IDE_SUCCESS );

        sRowPtr  = sPagePtr + SMP_PERS_PAGE_BODY_OFFSET;
        sFence   = sRowPtr + sFixedPageList->mSlotCount * sFixedPageList->mSlotSize;

        for( ; sRowPtr < sFence; sRowPtr += sFixedPageList->mSlotSize )
        {
            sSlotHeader = (smpSlotHeader *)sRowPtr;

            sDumpRecord.mPageID = sCurPageID;
            sDumpRecord.mOffset    = SMP_SLOT_GET_OFFSET( sSlotHeader );
            sDumpRecord.mCreateTID = SMP_GET_TID( sSlotHeader->mCreateSCN );
            sDumpRecord.mLimitTID  = SMP_GET_TID( sSlotHeader->mLimitSCN );

            idlOS::memset( sStrCreateSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
            idlOS::sprintf( (SChar*)sStrCreateSCN, 
                            "%"ID_XINT64_FMT, 
                            SM_SCN_TO_LONG( sSlotHeader->mCreateSCN ) );
            sDumpRecord.mCreateSCN = sStrCreateSCN;

            idlOS::memset( sStrLimitSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
            idlOS::sprintf( (SChar*)sStrLimitSCN, 
                            "%"ID_XINT64_FMT, 
                            SM_SCN_TO_LONG( sSlotHeader->mLimitSCN ) );
            sDumpRecord.mLimitSCN = sStrLimitSCN;

            idlOS::memset( sNext, 0x00, SM_SCN_STRING_LENGTH + 1 );
            idlOS::snprintf( (SChar*)sNext, SM_SCN_STRING_LENGTH,
                             "%"ID_XINT64_FMT, SMP_SLOT_GET_NEXT_OID(sSlotHeader) );
            sDumpRecord.mNext = sNext;

            /* BUG-31062로 Slot Header Flag 처리 방식 변경*/
            sDumpRecord.mFlag = SMP_SLOT_GET_FLAGS( sSlotHeader );

            sDumpRecord.mUsedFlag       = SMP_SLOT_IS_USED( sSlotHeader ) ? 'U':'F';
            sDumpRecord.mDropFlag       = SMP_SLOT_IS_DROP( sSlotHeader ) ? 'D':'F';
            sDumpRecord.mSkipRefineFlag = SMP_SLOT_IS_SKIP_REFINE( sSlotHeader ) ? 'S':'F';

            sDumpRecord.mNthSlot = sSlotSeq++;

            for( i = 0; i < sTblHdr->mColumnCount; i++ )
            {
                idlOS::memset( sDumpRecord.mValue,
                               0x00,
                               SM_DUMP_VALUE_LENGTH );

                if( ( sDumpRecord.mFlag & SMP_SLOT_USED_MASK )
                    == SMP_SLOT_USED_TRUE )
                {
                    IDE_TEST( smcFT::makeMemColValue24B(
                                  smcTable::getColumn(sTblHdr, i),
                                  sRowPtr,
                                  sDumpRecord.mValue ) != IDE_SUCCESS );
                }

                sDumpRecord.mNthColumn = i;

                IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                    aMemory,
                                                    (void *)&sDumpRecord )
                         != IDE_SUCCESS);
            }
        }

        sLocked = ID_FALSE;
        IDE_TEST( svmManager::releasePageLatch(sTblHdr->mSpaceID,
                                               sCurPageID)
                  != IDE_SUCCESS );

        if( sCurPageID == sLstPageID )
        {
            break;
        }

        sCurPageID = svpManager::getNextAllocPageID(sTblHdr->mSpaceID,
                                                    sFixedPageList,
                                                    sCurPageID);
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------

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

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch( sTblHdr->mSpaceID,
                                                  sCurPageID )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

