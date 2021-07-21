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
 * $Id: svpFT.cpp 19860 2007-02-07 02:09:39Z kimmkeun $
 *
 * Description
 *
 * Vol DB에서 페이지 관련 정보를 Dump하기 위한 FixedTable을 생성함.
 * D$VOL_DB_PAGE : Volatile Page를 Hexa형식의 Binary값으로 출력해줌.
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>
#include <smpManager.h>
#include <smcDef.h>
#include <svpFT.h>
#include <smpFT.h>
#include <smiFixedTable.h>
#include <svpReq.h>

static iduFixedTableColDesc gDumpVolDBPageColDesc[] =
{
    {
        (SChar*)"TBSID",
        IDU_FT_OFFSETOF(svpVolDBPageDump, mTBSID),
        IDU_FT_SIZEOF(svpVolDBPageDump, mTBSID),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PID",
        IDU_FT_OFFSETOF(svpVolDBPageDump, mPID),
        IDU_FT_SIZEOF(svpVolDBPageDump, mPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP1",
        IDU_FT_OFFSETOF(svpVolDBPageDump, mPageDump1),
        IDU_FT_SIZEOF(svpVolDBPageDump, mPageDump1),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP2",
        IDU_FT_OFFSETOF(svpVolDBPageDump, mPageDump2),
        IDU_FT_SIZEOF(svpVolDBPageDump, mPageDump2),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP3",
        IDU_FT_OFFSETOF(svpVolDBPageDump, mPageDump3),
        IDU_FT_SIZEOF(svpVolDBPageDump, mPageDump3),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_DUMP4",
        IDU_FT_OFFSETOF(svpVolDBPageDump, mPageDump4),
        IDU_FT_SIZEOF(svpVolDBPageDump, mPageDump4),
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

// D$VOL_DB_PAGE
// VOL PCH로부터 페이지를 Dump한다.
IDE_RC svpFT::buildRecordVolDBPageDump(idvSQL              * /*aStatistics*/,
                                       void                *aHeader,
                                       void                *aDumpObj,
                                       iduFixedTableMemory *aMemory)
{
    scGRID                * sGRID = NULL;
    scSpaceID               sSpaceID;
    scPageID                sPageID;
    UChar                 * sPagePtr;
    UInt                    sLocked = ID_FALSE;
    svpVolDBPageDump        sVolDBPageDump;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    sGRID      = (scGRID*) aDumpObj;
    sSpaceID   = SC_MAKE_SPACE(*sGRID);
    sPageID    = SC_MAKE_PID(*sGRID);

    // VOL_TABLESPACE가 맞는지 검사한다.
    IDE_ASSERT( sctTableSpaceMgr::isVolatileTableSpace( sSpaceID ) == ID_TRUE );

    IDE_TEST_RAISE( smmManager::isPageExist( sSpaceID, sPageID ) != ID_TRUE,
                    ERR_EMPTY_OBJECT );

    IDE_TEST( svmManager::holdPageSLatch( sSpaceID, sPageID )
              != IDE_SUCCESS );
    sLocked = ID_TRUE;

    IDE_ASSERT( smmManager::getPersPagePtr( sSpaceID, 
                                            sPageID,
                                            (void**)&sPagePtr )
                == IDE_SUCCESS );

    sVolDBPageDump.mTBSID = sSpaceID;
    sVolDBPageDump.mPID   = sPageID;

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*0 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sVolDBPageDump.mPageDump1,
                            SMP_DUMP_COLUMN_SIZE );

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*1 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sVolDBPageDump.mPageDump2,
                            SMP_DUMP_COLUMN_SIZE );

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*2 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sVolDBPageDump.mPageDump3,
                            SMP_DUMP_COLUMN_SIZE );

    ideLog::ideMemToHexStr( &sPagePtr[ SMP_PAGE_PART_SIZE*3 ],
                            SMP_PAGE_PART_SIZE, 
                            IDE_DUMP_FORMAT_BINARY,
                            sVolDBPageDump.mPageDump4,
                            SMP_DUMP_COLUMN_SIZE );

    sLocked = ID_FALSE;
    IDE_TEST( svmManager::releasePageLatch( sSpaceID, sPageID )
              != IDE_SUCCESS );

    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                         aMemory,
                                         (void *)&sVolDBPageDump)
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( svmManager::releasePageLatch( sSpaceID, sPageID )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gDumpVolDBPageTableDesc =
{
    (SChar *)"D$VOL_DB_PAGE",
    svpFT::buildRecordVolDBPageDump,
    gDumpVolDBPageColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


