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
#include <iduVLogShmPersPool.h>
#include <iduShmPersPool.h>

idrUndoFunc iduVLogShmPersPool::mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSPOOL_MAX];
SChar       iduVLogShmPersPool::mStrVLogType[IDU_VLOG_TYPE_SHM_PERSPOOL_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_PERSPOOL_NONE",
    "IDU_VLOG_TYPE_SHM_PERSPOOL_INITIALIZE",
    "IDU_VLOG_TYPE_SHM_PERSPOOL_SET_LIST_COUNT",
    "IDU_VLOG_TYPE_SHM_PERSPOOL_SET_ADDR_4_ARRMEMLIST",
    "IDU_VLOG_TYPE_SHM_PERSPOOL_MAX"
};

IDE_RC iduVLogShmPersPool::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_PERSPOOL,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSPOOL_INITIALIZE] =
        undo_SHM_PERSPOOL_INITIALIZE;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSPOOL_SET_LIST_COUNT] =
        undo_SHM_PERSPOOL_SET_LIST_COUNT;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSPOOL_SET_ADDR_4_ARRMEMLIST] =
        undo_SHM_PERSPOOL_SET_ADDR_4_ARRMEMLIST;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSPOOL_NONE] = NULL;


    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_PERSPOOL,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::writeInitialize( iduShmTxInfo    * aShmTxInfo,
                                           idrLSN            aNTALsn,
                                           iduStShmMemPool * aStMemPool )
{
    idrUImgInfo  sArrUImgInfo[1];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aStMemPool->mAddrSelf,
                       ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeNTALog( aShmTxInfo,
                            aNTALsn,
                            IDR_MODULE_TYPE_ID_IDU_SHM_PERSPOOL,
                            IDU_VLOG_TYPE_SHM_PERSPOOL_INITIALIZE,
                            1,
                            sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::undo_SHM_PERSPOOL_INITIALIZE( idvSQL       * aStatistics,
                                                       iduShmTxInfo * aShmTxInfo,
                                                       idrLSN         aNTALsn,
                                                       UShort       /*aSize*/,
                                                       SChar        * aImage )
{
    iduStShmMemPool * sMemPoolInfo;
    idShmAddr         sAddr4MemPool;
    idrSVP            sSavepoint;

    idrLogMgr::crtSavepoint( aShmTxInfo,
                             &sSavepoint,
                             aNTALsn );

    idlOS::memcpy( &sAddr4MemPool, aImage, ID_SIZEOF(idShmAddr) );

    sMemPoolInfo = (iduStShmMemPool*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4MemPool );

    IDE_ASSERT( iduShmPersPool::destroy( aStatistics,
                                        aShmTxInfo,
                                        &sSavepoint,
                                        sMemPoolInfo,
                                        ID_FALSE /* aCheck */ )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::writeSetListCnt( iduShmTxInfo    * aShmTxInfo,
                                           iduStShmMemPool * aStMemPool )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aStMemPool->mAddrSelf,
                       ID_SIZEOF(idShmAddr) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[1],
                       &aStMemPool->mListCount,
                       ID_SIZEOF(UInt) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSPOOL,
                         IDU_VLOG_TYPE_SHM_PERSPOOL_SET_LIST_COUNT,
                         2,
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::undo_SHM_PERSPOOL_SET_LIST_COUNT( idvSQL       * /*aStatistics*/,
                                                           iduShmTxInfo * /*aShmTxInfo*/,
                                                           idrLSN         /*aNTALsn*/,
                                                           UShort         /*aSize*/,
                                                           SChar        * aImage )
{
    iduStShmMemPool * sMemPoolInfo;
    idShmAddr         sAddr4MemPool;
    UInt              sListCnt;

    idlOS::memcpy( &sAddr4MemPool, aImage, ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);
    idlOS::memcpy( &sListCnt, aImage, ID_SIZEOF(UInt) );

    sMemPoolInfo = (iduStShmMemPool*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4MemPool );
    sMemPoolInfo->mListCount = sListCnt;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::writeSetArrMemList( iduShmTxInfo    * aShmTxInfo,
                                              iduStShmMemPool * aMemPoolInfo )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aMemPoolInfo->mAddrSelf,
                       ID_SIZEOF(idShmAddr) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[1],
                       &aMemPoolInfo->mAddrArrMemList,
                       ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSPOOL,
                         IDU_VLOG_TYPE_SHM_PERSPOOL_SET_ADDR_4_ARRMEMLIST,
                         2,
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersPool::undo_SHM_PERSPOOL_SET_ADDR_4_ARRMEMLIST( idvSQL        * /*aStatistics*/,
                                                                  iduShmTxInfo  * /*aShmTxInfo*/,
                                                                  idrLSN          /*aNTALsn*/,
                                                                  UShort          /*aSize*/,
                                                                  SChar         * aImage )
{
    SChar           * sCurImgPtr = aImage;
    idShmAddr         sAddr4MemPool;
    idShmAddr         sAddr4ArrMemList;
    iduStShmMemPool * sMemPoolInfo;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurImgPtr,
                              &sAddr4MemPool,
                              ID_SIZEOF( idShmAddr ) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurImgPtr,
                              &sAddr4ArrMemList,
                              ID_SIZEOF( idShmAddr ) );


    sMemPoolInfo = (iduStShmMemPool*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4MemPool );
    sMemPoolInfo->mAddrArrMemList = sAddr4ArrMemList;

    return IDE_SUCCESS;
}


