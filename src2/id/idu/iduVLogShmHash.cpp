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
#include <iduVLogShmHash.h>

idrUndoFunc iduVLogShmHash::mArrUndoFunction[IDU_VLOG_TYPE_SHM_HASH_MAX];
SChar       iduVLogShmHash::mStrVLogType[IDU_VLOG_TYPE_SHM_HASH_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_HASH_NONE",
    "IDU_VLOG_TYPE_SHM_HASH_CUTNODE",
    "IDU_VLOG_TYPE_SHM_HASH_INSERT_NOLATCH",
    "IDU_VLOG_TYPE_SHM_HASH_UPDATE_ADDR_BUCKET_LIST",
    "IDU_VLOG_TYPE_SHM_HASH_MAX"
};

IDE_RC iduVLogShmHash::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_HASH,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_HASH_NONE] =
        NULL;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_HASH_CUTNODE] =
        undo_SHM_HASH_CUTNODE;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_HASH_INSERT_NOLATCH] =
        undo_SHM_HASH_INSERT_NOLATCH;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_HASH_UPDATE_ADDR_BUCKET_LIST] =
        undo_SHM_HASH_UPDATE_ADDR_BUCKET_LIST;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmHash::writeNTAInsertNodeNoLatch( iduShmTxInfo       * aShmTxInfo,
                                                  idrLSN               aNTALsn,
                                                  iduLtShmHashBase   * aBase,
                                                  iduStShmHashBucket * aBucket,
                                                  void               * aKeyPtr,
                                                  UInt                 aKeyLength )
{
    idrUImgInfo  sArrUImgInfo[3];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aBase->mStShmHashBase->mAddrSelf,
                       ID_SIZEOF(idShmAddr) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aBucket->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], aKeyPtr, aKeyLength );

    idrLogMgr::writeNTALog( aShmTxInfo,
                            aNTALsn,
                            IDR_MODULE_TYPE_ID_IDU_SHM_HASH,
                            IDU_VLOG_TYPE_SHM_HASH_INSERT_NOLATCH,
                            3,
                            sArrUImgInfo);

    return IDE_SUCCESS;

}

IDE_RC iduVLogShmHash::undo_SHM_HASH_INSERT_NOLATCH( idvSQL       * aStatistics,
                                                     iduShmTxInfo * aShmTxInfo,
                                                     idrLSN         aNTALsn,
                                                     UShort       /*aSize*/,
                                                     SChar        * aImage )
{
    idShmAddr            sAddObj4Base;
    idShmAddr            sAddObj4Bucket;
    ULong                sKey;
    SChar              * sCurLogPtr;
    iduLtShmHashBase     sLtHashBase;
    iduStShmHashBase   * sStShmHashBase;
    iduStShmHashBucket * sBucket;
    void               * sTempNode;
    idrSVP               sVSavepoint;

    idrLogMgr::crtSavepoint( aShmTxInfo, &sVSavepoint, aNTALsn );

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Base, ID_SIZEOF(idShmAddr) );
    sStShmHashBase = (iduStShmHashBase*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddObj4Base );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Bucket, ID_SIZEOF(idShmAddr) );
    sBucket = (iduStShmHashBucket*)IDU_SHM_GET_ADDR_PTR( sAddObj4Bucket );

    iduShmHash::attach( sStShmHashBase->mUseLatch,
                        sStShmHashBase,
                        &sLtHashBase );

    IDR_VLOG_GET_UIMG( sCurLogPtr, &sKey, sStShmHashBase->mKeyLength );

    IDE_ASSERT( iduShmHash::deleteNodeNoLatch( aStatistics,
                                               aShmTxInfo,
                                               &sVSavepoint,
                                               &sLtHashBase,
                                               sBucket,
                                               &sKey,
                                               &sTempNode )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmHash::writeCutNode( iduShmTxInfo     * aShmTxInfo,
                                     iduStShmHashBase * aStHashBase )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aStHashBase->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aStHashBase->mAddrCurChain, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_HASH,
                         IDU_VLOG_TYPE_SHM_HASH_CUTNODE,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmHash::undo_SHM_HASH_CUTNODE( idvSQL       * /*aStatistics*/,
                                              iduShmTxInfo * /*aShmTxInfo*/,
                                              idrLSN         /*aNTALsn*/,
                                              UShort         /*aSize*/,
                                              SChar        * aImage )
{
    idShmAddr       sAddObj;
    SChar         * sCurLogPtr;

    iduStShmHashBase * sStHashBase;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sStHashBase = (iduStShmHashBase*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sStHashBase->mAddrCurChain, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmHash::writeUpdateAddrBucketList( iduShmTxInfo     * aShmTxInfo,
                                                  iduStShmHashBase * aStHashBase )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aStHashBase->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aStHashBase->mAddrBucketLst, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_HASH,
                         IDU_VLOG_TYPE_SHM_HASH_UPDATE_ADDR_BUCKET_LIST,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmHash::undo_SHM_HASH_UPDATE_ADDR_BUCKET_LIST( idvSQL       * /*aStatistics*/,
                                                              iduShmTxInfo * /*aShmTxInfo*/,
                                                              idrLSN         /*aNTALsn*/,
                                                              UShort         /*aSize*/,
                                                              SChar        * aImage )
{
    idShmAddr          sAddObj;
    SChar            * sCurLogPtr;
    iduStShmHashBase * sStHashBase;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sStHashBase = (iduStShmHashBase*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sStHashBase->mAddrBucketLst, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmHash::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_HASH,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

