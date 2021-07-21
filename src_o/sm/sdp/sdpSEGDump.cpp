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
#include <smErrorCode.h>
#include <sdpSEGDump.h>
#include <sdpSegDescMgr.h>
#include <sdcTXSegMgr.h>
#include <smcTable.h>
#include <sdpReq.h>

IDE_RC sdpSEGDump::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC sdpSEGDump::destroy()
{
    return IDE_SUCCESS;
}

IDE_RC sdpSEGDump::validateAndGetSegPID( scSpaceID   aSpaceID,
                                         void       *aTable,
                                         scPageID   *aSegPID,
                                         smiDumpType aType )
{
    /* FID, FFPID가 Valid한지 검증한다. */
    switch( aType )
    {
        case SMI_DUMP_TBL:
            *aSegPID = smcTable::getSegPID( aTable );

            IDE_TEST_RAISE( *aSegPID == SD_NULL_PID,
                            ERR_INVALID_DUMP_PID );
            break;

        case SMI_DUMP_IDX:
            IDE_TEST_RAISE( smcTable::isIdxSegPIDOfTbl( aTable, aSpaceID, *aSegPID )
                            != ID_TRUE, ERR_INVALID_DUMP_PID );
            break;

        case SMI_DUMP_LOB:
            IDE_TEST_RAISE( smcTable::isLOBSegPIDOfTbl( aTable, aSpaceID, *aSegPID )
                            != ID_TRUE, ERR_INVALID_DUMP_PID );
            break;

        case SMI_DUMP_TSS:
            IDE_TEST_RAISE( aSpaceID != SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                            ERR_INVALID_DUMP_PID );

            IDE_TEST_RAISE( sdcTXSegMgr::isTSSegPID( *aSegPID )
                            != ID_TRUE, ERR_INVALID_DUMP_PID );
            break;

        case SMI_DUMP_UDS:
            IDE_TEST_RAISE( aSpaceID != SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                            ERR_INVALID_DUMP_PID );

            IDE_TEST_RAISE( sdcTXSegMgr::isUDSegPID( *aSegPID )
                            != ID_TRUE, ERR_INVALID_DUMP_PID );
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_PID )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


