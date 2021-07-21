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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduProperty.h>
#include <idrLogMgr.h>
#include <idwVLogPMMgr.h>

idrUndoFunc idwVLogPMMgr::mArrUndoFunction[IDW_VLOG_TYPE_PM_MGR_MAX];

IDE_RC idwVLogPMMgr::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_PM_MGR,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_NONE] = NULL;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_REGISTER_NEW_SEG] = undo_PMMGR_UPDATE_THREAD_COUNT;

    return IDE_SUCCESS;
}

IDE_RC idwVLogPMMgr::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_PM_MGR,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

IDE_RC idwVLogPMMgr::writeThreadCount4Proc( iduShmTxInfo * aShmTxInfo,
                                            idShmAddr      aAddr4Proc,
                                            UInt           aThrCnt )
{
    idrUImgInfo sArrUImgInfo[2];

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData = (SChar*)&aAddr4Proc;

    sArrUImgInfo[1].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[1].mData = (SChar*)&aThrCnt;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_PM_MGR,
                         IDW_VLOG_TYPE_PM_MGR_UPDATE_THREAD_COUNT,
                         2, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC idwVLogPMMgr::undo_PMMGR_UPDATE_THREAD_COUNT( idvSQL       * /*aStatistics*/,
                                                     iduShmTxInfo * /*aShmTxInfo*/,
                                                     idrLSN         /*aNTALsn*/,
                                                     UShort         /*aSize*/,
                                                     SChar        * aImage )
{
    SChar          * sLogPtr;
    idShmAddr        sAddr4ProcInfo;
    UInt             sThrCount;
    iduShmProcInfo * sProcInfo;

    sLogPtr  = aImage;

    idlOS::memcpy( &sAddr4ProcInfo, sLogPtr, ID_SIZEOF(idShmAddr) );
    sLogPtr += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sThrCount, sLogPtr, ID_SIZEOF(UInt) );

    sProcInfo = (iduShmProcInfo*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4ProcInfo );

    sProcInfo->mThreadCnt = sThrCount;

    return IDE_SUCCESS;
}

