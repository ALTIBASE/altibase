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
#include <iduVLogShmLatch.h>

idrUndoFunc iduVLogShmLatch::mArrUndoFunction[IDU_VLOG_TYPE_SHM_LATCH_MAX];
SChar       iduVLogShmLatch::mStrVLogType[IDU_VLOG_TYPE_SHM_LATCH_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_LATCH_NONE",
    "IDU_VLOG_TYPE_SHM_LATCH_UPDATE_SLATCH",
    "IDU_VLOG_TYPE_SHM_LATCH_RELEASE_SHARED",
    "IDU_VLOG_TYPE_SHM_LATCH_MAX"
};

IDE_RC iduVLogShmLatch::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_LATCH,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LATCH_NONE] = NULL;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LATCH_UPDATE_SLATCH] =
        undo_SHM_LATCH_UPDATE_SLATCH;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_LATCH_RELEASE_SHARED] =
        undo_SHM_LATCH_RELEASE_SHARED;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmLatch::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_LATCH,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmLatch::undo_SHM_LATCH_UPDATE_SLATCH( idvSQL       * /*aStatistics*/,
                                                      iduShmTxInfo * /*aShmTxInfo*/,
                                                      idrLSN         /*aNTALsn*/,
                                                      UShort           aSize,
                                                      SChar        * aImage )
{
    idShmAddr       sAddObj;
    SChar         * sDataPtr;
    SChar         * sCurLogPtr;
    UShort          sCurLogSize;
    iduShmSXLatch * sSXLatch;

    sCurLogPtr  = aImage;
    sCurLogSize = aSize;

    idlOS::memcpy( &sAddObj, sCurLogPtr, ID_SIZEOF( idShmAddr ) );
    sCurLogPtr  += ID_SIZEOF( idShmAddr );
    sCurLogSize -= ID_SIZEOF( idShmAddr );

    sDataPtr = IDU_SHM_GET_ADDR_PTR( sAddObj );

    IDE_ASSERT( sCurLogSize == ID_SIZEOF(SInt) );

    sSXLatch = (iduShmSXLatch*)sDataPtr;
    idlOS::memcpy( &sSXLatch->mSharedLatchCnt, sCurLogPtr, sCurLogSize );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmLatch::writeReleaseSLatch( iduShmTxInfo      * aShmTxInfo,
                                            idrLSN              aNTALsn,
                                            iduShmSXLatch     * aSXLatch,
                                            UInt                aStackIdx4SXLatch )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aSXLatch->mLatch.mAddrSelf,
                       ID_SIZEOF(idShmAddr) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[1],
                       &aStackIdx4SXLatch,
                       ID_SIZEOF(UInt) );

    idrLogMgr::writeNTALog( aShmTxInfo,
                            aNTALsn,
                            IDR_MODULE_TYPE_ID_IDU_SHM_LATCH,
                            IDU_VLOG_TYPE_SHM_LATCH_RELEASE_SHARED,
                            2,
                            sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmLatch::undo_SHM_LATCH_RELEASE_SHARED( idvSQL       * aStatistics,
                                                       iduShmTxInfo * aShmTxInfo,
                                                       idrLSN         aNTALsn,
                                                       UShort         /*aSize*/,
                                                       SChar        * aImage )
{
    idShmAddr       sAddr4Latch;
    SChar         * sCurLogPtr;
    iduShmSXLatch * sSXLatch;
    UInt            sStackIdx4SXLatch;
    idrSVP          sVSavepoint;

    idrLogMgr::crtSavepoint( aShmTxInfo, &sVSavepoint, aNTALsn );

    sCurLogPtr = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr,
                              &sAddr4Latch,
                              ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr,
                              &sStackIdx4SXLatch,
                              ID_SIZEOF(UInt) );

    sSXLatch = (iduShmSXLatch*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4Latch );

    idrLogMgr::clearLatchStack( aShmTxInfo, sStackIdx4SXLatch, ID_TRUE );

    IDE_ASSERT( iduShmLatchRelease( aShmTxInfo, &sSXLatch->mLatch )
                == IDE_SUCCESS );

    IDE_ASSERT( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sVSavepoint )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}
