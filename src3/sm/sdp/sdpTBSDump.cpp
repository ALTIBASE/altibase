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
 * $Id:$
 **********************************************************************/
#include <idl.h>
#include <sdpTableSpace.h>
#include <sctTableSpaceMgr.h>
#include <sdpTBSDump.h>
#include <sdpReq.h>
#include <sdptbFT.h>

IDE_RC sdpTBSDump::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC sdpTBSDump::destroy()
{
    return IDE_SUCCESS;
}

//------------------------------------------------------
// D$DISK_TBS_FREEEXTLIST Dump Table의 Column Description
//------------------------------------------------------
static iduFixedTableColDesc gDumpDiskTBSFreeEXTListColDesc[]=
{
    {
        (SChar*)"EXTRID",
        offsetof( sdpDumpTBSInfo, mExtRID ),
        IDU_FT_SIZEOF( sdpDumpTBSInfo, mExtRID ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PID",
        offsetof( sdpDumpTBSInfo, mPID ),
        IDU_FT_SIZEOF( sdpDumpTBSInfo, mPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"Offset",
        offsetof( sdpDumpTBSInfo, mOffset ),
        IDU_FT_SIZEOF( sdpDumpTBSInfo, mOffset ),
        IDU_FT_TYPE_SMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FstPID",
        offsetof( sdpDumpTBSInfo, mFstPID ),
        IDU_FT_SIZEOF( sdpDumpTBSInfo, mFstPID ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ExtSize",
        offsetof( sdpDumpTBSInfo, mPageCntInExt ),
        IDU_FT_SIZEOF( sdpDumpTBSInfo, mPageCntInExt ),
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
// D$DISK_TBS_FREEEXTLIST Dump Table의 Table Description
//------------------------------------------------------
iduFixedTableDesc  gDumpDiskTBSFreeEXTListTableDesc =
{
    (SChar *)"D$DISK_TBS_FREEEXTLIST",
    sdpTBSDump::buildRecord4ExtFreeList,
    gDumpDiskTBSFreeEXTListColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

//------------------------------------------------------
// D$DISK_TBS_FREEEXTLIST Dump Table의 레코드 Build
//------------------------------------------------------
IDE_RC sdpTBSDump::buildRecord4ExtFreeList(
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * aDumpObj,
    iduFixedTableMemory * aMemory )
{
    scSpaceID       sSpaceID;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_RAISE( aDumpObj == NULL, ERR_EMPTY_OBJECT );

    /* TBS가 존재하는지 검사하고 Dump중에 Drop되지 않도록 Lock을 잡는다. */
    /* BUG-28678  [SM] qmsDumpObjList::mObjInfo에 설정될 메모리 주소는 
     * 반드시 공간을 할당해서 설정해야합니다. 
     * 
     * aDumpObj는 Pointer로 데이터가 오기 때문에 값을 가져와야 합니다. */
    sSpaceID  = *( (scSpaceID*)aDumpObj );
    IDE_ASSERT( sctTableSpaceMgr::isDiskTableSpace( sSpaceID ) == ID_TRUE );

    IDE_TEST( sdptbFT::buildRecord4FreeExtOfTBS( aHeader,
                                                 aDumpObj,
                                                 aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EMPTY_OBJECT);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_DUMP_EMPTY_OBJECT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
