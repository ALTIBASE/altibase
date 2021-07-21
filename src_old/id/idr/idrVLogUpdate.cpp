/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#include <idu.h>
#include <idw.h>
#include <idrDef.h>
#include <idrLogMgr.h>
#include <idrVLogUpdate.h>

idrUndoFunc idrVLogUpdate::mArrUndoFunction[IDR_VLOG_TYPE_MAX];

SChar idrVLogUpdate::mStrVLogType[IDR_VLOG_TYPE_MAX+1][100] =
{
    "IDR_VLOG_TYPE_NONE",
    "IDR_VLOG_TYPE_PHYSICAL",
    "IDR_VLOG_TYPE_DUMMY_NTA",
    "IDR_VLOG_TYPE_MAX"
};

IDE_RC idrVLogUpdate::initialize()
{
    idlOS::memset( mArrUndoFunction,
                   0,
                   ID_SIZEOF(idrUndoFunc) * IDR_VLOG_TYPE_MAX );

    mArrUndoFunction[IDR_VLOG_TYPE_PHYSICAL]  = undo_VLOG_TYPE_PHYSICAL;
    mArrUndoFunction[IDR_VLOG_TYPE_DUMMY_NTA] = undo_VLOG_TYPE_DUMMY_NTA;

    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID, mArrUndoFunction );

    return IDE_SUCCESS;
}

IDE_RC idrVLogUpdate::destroy()
{
    return IDE_SUCCESS;
}

IDE_RC idrVLogUpdate::undo_VLOG_TYPE_PHYSICAL( idvSQL       * /*aStatistics*/,
                                               iduShmTxInfo * /*aShmTxInfo*/,
                                               idrLSN         /*aNTALsn*/,
                                               UShort         aSize,
                                               SChar        * aImage )
{
    idShmAddr   sAddObj;
    SChar     * sDataPtr;
    SChar     * sCurLogPtr;
    UShort      sCurLogSize;

    sCurLogPtr  = aImage;
    sCurLogSize = aSize;

    idlOS::memcpy( &sAddObj, sCurLogPtr, ID_SIZEOF( idShmAddr ) );
    sCurLogPtr  += ID_SIZEOF( idShmAddr );
    sCurLogSize -= ID_SIZEOF( idShmAddr );

    sDataPtr = IDU_SHM_GET_ADDR_PTR( sAddObj );

    idlOS::memcpy( sDataPtr, sCurLogPtr, sCurLogSize );
    return IDE_SUCCESS;
}

IDE_RC idrVLogUpdate::undo_VLOG_TYPE_DUMMY_NTA( idvSQL       * /*aStatistics*/,
                                                iduShmTxInfo * aShmTxInfo,
                                                idrLSN         aNTALsn,
                                                UShort         /*aSize*/,
                                                SChar        * /*aImage*/ )
{
    idrLogMgr::setLstLSN( aShmTxInfo, aNTALsn );
    return IDE_SUCCESS;
}

