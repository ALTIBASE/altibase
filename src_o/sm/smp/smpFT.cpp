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
 * $Id: smpFT.cpp 19860 2007-02-07 02:09:39Z kimmkeun $
 *
 * Description
 *
 * MEM DB에서 페이지 관련 정보를 Dump하기 위한 FixedTable을 생성함.
 * D$MEM_DB_PAGE : Mem Page를 Hexa형식의 Binary값으로 출력해줌.
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>
#include <smpManager.h>
#include <smcDef.h>
#include <smpFT.h>
#include <smpReq.h>

static iduFixedTableColDesc gDumpMemDBPageColDesc[] =
{
    {
        (SChar*)"TBSID",
        IDU_FT_OFFSETOF(smpMemDBPageDump, mTBSID),
        IDU_FT_SIZEOF(smpMemDBPageDump, mTBSID),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PID",
        IDU_FT_OFFSETOF(smpMemDBPageDump, mPID),
        IDU_FT_SIZEOF(smpMemDBPageDump, mPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP1",
        IDU_FT_OFFSETOF(smpMemDBPageDump, mPageDump1),
        IDU_FT_SIZEOF(smpMemDBPageDump, mPageDump1),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP2",
        IDU_FT_OFFSETOF(smpMemDBPageDump, mPageDump2),
        IDU_FT_SIZEOF(smpMemDBPageDump, mPageDump2),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP3",
        IDU_FT_OFFSETOF(smpMemDBPageDump, mPageDump3),
        IDU_FT_SIZEOF(smpMemDBPageDump, mPageDump3),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP4",
        IDU_FT_OFFSETOF(smpMemDBPageDump, mPageDump4),
        IDU_FT_SIZEOF(smpMemDBPageDump, mPageDump4),
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

// D$MEM_DB_PAGE
// MEM PCH로부터 페이지를 Dump한다.
IDE_RC smpFT::buildRecordMemDBPageDump( idvSQL              * /*aStatistics*/,
                                        void                *aHeader,
                                        void                *aDumpObj,
                                        iduFixedTableMemory *aMemory )
{
    scGRID                * sGRID = NULL;
    scSpaceID               sSpaceID;
    scPageID                sPageID;
    UChar                 * sPagePtr;
    UInt                    sLocked = ID_FALSE;
    smpMemDBPageDump        sMemDBPageDump;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sGRID     = (scGRID*) aDumpObj;
    sSpaceID  = SC_MAKE_SPACE(*sGRID);
    sPageID   = SC_MAKE_PID(*sGRID);

    // MEM_TABLESPACE가 맞는지 검사한다.
    IDE_ASSERT( sctTableSpaceMgr::isMemTableSpace( sSpaceID ) == ID_TRUE );

    IDE_TEST_RAISE( smmManager::isPageExist( sSpaceID, sPageID ) == ID_TRUE,
                    ERR_EMPTY_OBJECT );

    IDE_TEST( smmManager::holdPageSLatch( sSpaceID, sPageID )
              != IDE_SUCCESS );
    sLocked = ID_TRUE;

    IDE_ASSERT( smmManager::getPersPagePtr( sSpaceID, 
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );

    sMemDBPageDump.mTBSID = sSpaceID;
    sMemDBPageDump.mPID   = sPageID;

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*0 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sMemDBPageDump.mPageDump1,
                            SMP_DUMP_COLUMN_SIZE );

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*1 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sMemDBPageDump.mPageDump2,
                            SMP_DUMP_COLUMN_SIZE );

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*2 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sMemDBPageDump.mPageDump3,
                            SMP_DUMP_COLUMN_SIZE );

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*3 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sMemDBPageDump.mPageDump4,
                            SMP_DUMP_COLUMN_SIZE );

    sLocked = ID_FALSE;
    IDE_TEST( smmManager::releasePageLatch( sSpaceID, sPageID )
              != IDE_SUCCESS );

    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                         aMemory,
                                         (void *)&sMemDBPageDump )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EMPTY_OBJECT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch( sSpaceID, sPageID )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpMemDBPageTableDesc =
{
    (SChar *)"D$MEM_DB_PAGE",
    smpFT::buildRecordMemDBPageDump,
    gDumpMemDBPageColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

