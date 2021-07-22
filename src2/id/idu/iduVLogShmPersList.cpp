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
#include <iduVLogShmPersList.h>
#include <iduShmPersList.h>

idrUndoFunc iduVLogShmPersList::mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_MAX];
SChar       iduVLogShmPersList::mStrVLogType[IDU_VLOG_TYPE_SHM_PERSLIST_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_PERSLIST_NONE",
    "IDU_VLOG_TYPE_SHM_PERSLIST_INITIALIZE",
    "IDU_VLOG_TYPE_SHM_PERSLIST_LINK_WITH_AFTER",
    "IDU_VLOG_TYPE_SHM_PERSLIST_LINK_ONLY_BEFORE",
    "IDU_VLOG_TYPE_SHM_PERSLIST_UNLINK_WITH_AFTER",
    "IDU_VLOG_TYPE_SHM_PERSLIST_UNLINK_ONLY_BEFORE",
    "IDU_VLOG_TYPE_SHM_PERSLIST_ALLOC",
    "IDU_VLOG_TYPE_SHM_PERSLIST_ALLOC_MYCHUNK",
    "IDU_VLOG_TYPE_SHM_PERSLIST_MEMFREE",
    "IDU_VLOG_TYPE_SHM_PERSLIST_FREECHUNKCNT",
    "IDU_VLOG_TYPE_SHM_PERSLIST_PARTCHUNKCNT",
    "IDU_VLOG_TYPE_SHM_PERSLIST_FULLCHUNKCNT",
    "IDU_VLOG_TYPE_SHM_PERSLIST_FREESLOTCNT",
    "IDU_VLOG_TYPE_SHM_PERSLIST_MAX"
};

IDE_RC iduVLogShmPersList::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_NONE] = NULL;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_INITIALIZE] =
        undo_SHM_PERSLIST_INITIALIZE;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_LINK_WITH_AFTER] =
        undo_SHM_PERSLIST_LINK_WITH_AFTER;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_LINK_ONLY_BEFORE] =
        undo_SHM_PERSLIST_LINK_ONLY_BEFORE;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_UNLINK_WITH_AFTER] =
        undo_SHM_PERSLIST_UNLINK_WITH_AFTER;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_UNLINK_ONLY_BEFORE] =
        undo_SHM_PERSLIST_UNLINK_ONLY_BEFORE;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_ALLOC] =
        undo_SHM_PERSLIST_ALLOC;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_ALLOC_MYCHUNK] =
        undo_SHM_PERSLIST_ALLOC_MYCHUNK;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_MEMFREE] =
        undo_SHM_PERSLIST_MEMFREE;

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_FREECHUNKCNT] =
        undo_SHM_PERSLIST_FREECHUNKCNT;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_PARTCHUNKCNT] =
        undo_SHM_PERSLIST_PARTCHUNKCNT;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_FULLCHUNKCNT] =
        undo_SHM_PERSLIST_FULLCHUNKCNT;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_PERSLIST_FREESLOTCNT]  =
        undo_SHM_PERSLIST_FREESLOTCNT;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeInitialize( iduShmTxInfo     * aShmTxInfo,
                                           idrLSN             aNTALsn,
                                           iduStShmMemList  * aPersListInfo )
{
    idrUImgInfo  sArrUImgInfo[1];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aPersListInfo->mAddrSelf, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeNTALog( aShmTxInfo,
                            aNTALsn,
                            IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                            IDU_VLOG_TYPE_SHM_PERSLIST_INITIALIZE,
                            1,
                            sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_INITIALIZE( idvSQL       * aStatistics,
                                                       iduShmTxInfo * aShmTxInfo,
                                                       idrLSN         aNTALsn,
                                                       UShort         /*aSize*/,
                                                       SChar        * aImage )
{
    iduStShmMemList    * sPersListInfo;
    idShmAddr            sAddr4MemLst;
    idrSVP               sSavepoint;

    idrLogMgr::crtSavepoint( aShmTxInfo,
                             &sSavepoint,
                             aNTALsn );

    idlOS::memcpy( &sAddr4MemLst, aImage, ID_SIZEOF(idShmAddr) );

    sPersListInfo = (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4MemLst );

    IDE_ASSERT( iduShmPersList::destroy( aStatistics,
                                        aShmTxInfo,
                                        &sSavepoint,
                                        sPersListInfo,
                                        ID_FALSE /* aCheck */ )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeLinkWithAfter( iduShmTxInfo       * aShmTxInfo,
                                              iduStShmMemChunk   * aBefore,
                                              iduStShmMemChunk   * aChunk,
                                              iduStShmMemChunk   * aAfter )
{
    idrUImgInfo  sArrUImgInfo[7];

    // sBefore->mAddrSelf
    // aBefore->mAddrNext

    // sChunk->mAddrSelf
    // sChunk->mAddrPrev
    // aChunk->mAddrNext

    // sAfter->mAddrSelf
    // sAfter->mAddrPrev

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aBefore->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aBefore->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aChunk->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aChunk->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[4], &aChunk->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[5], &aAfter->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[6], &aAfter->mAddrPrev, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_LINK_WITH_AFTER,
                         7,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_LINK_WITH_AFTER( idvSQL       * /*aStatistics*/,
                                                            iduShmTxInfo * /*aShmTxInfo*/,
                                                            idrLSN         /*aNTALsn*/,
                                                            UShort         /*aSize*/,
                                                            SChar        * aImage )
{
    idShmAddr       sAddObj4Before;
    idShmAddr       sAddObj4Chunk;
    idShmAddr       sAddObj4After;
    SChar         * sCurLogPtr;

    iduStShmMemChunk * sBefore;
    iduStShmMemChunk * sChunk;
    iduStShmMemChunk * sAfter;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Before, ID_SIZEOF(idShmAddr) );
    sBefore = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4Before );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sBefore->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Chunk, ID_SIZEOF(idShmAddr) );
    sChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4Chunk );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sChunk->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sChunk->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4After, ID_SIZEOF(idShmAddr) );
    sAfter = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4After );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sAfter->mAddrPrev, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeLinkOnlyBefore( iduShmTxInfo     * aShmTxInfo,
                                               iduStShmMemChunk * aBefore,
                                               iduStShmMemChunk * aChunk )
{
    idrUImgInfo  sArrUImgInfo[5];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aBefore->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aBefore->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aChunk->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aChunk->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[4], &aChunk->mAddrNext, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_LINK_ONLY_BEFORE,
                         5,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_LINK_ONLY_BEFORE( idvSQL       * /*aStatistics*/,
                                                             iduShmTxInfo * /*aShmTxInfo*/,
                                                             idrLSN         /*aNTALsn*/,
                                                             UShort         /*aSize*/,
                                                             SChar        * aImage )
{
    idShmAddr       sAddObj4Before;
    idShmAddr       sAddObj4Chunk;
    SChar         * sCurLogPtr;

    iduStShmMemChunk * sBefore;
    iduStShmMemChunk * sChunk;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Before, ID_SIZEOF(idShmAddr) );
    sBefore = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4Before );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sBefore->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Chunk, ID_SIZEOF(idShmAddr) );
    sChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4Chunk );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sChunk->mAddrPrev, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sChunk->mAddrNext, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeUnlinkWithAfter( iduShmTxInfo     * aShmTxInfo,
                                                iduStShmMemChunk * aBefore,
                                                iduStShmMemChunk * aAfter )
{
    idrUImgInfo  sArrUImgInfo[4];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aBefore->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aBefore->mAddrNext, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aAfter->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aAfter->mAddrPrev, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_UNLINK_WITH_AFTER,
                         4,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_UNLINK_WITH_AFTER( idvSQL       * /*aStatistics*/,
                                                              iduShmTxInfo * /*aShmTxInfo*/,
                                                              idrLSN         /*aNTALsn*/,
                                                              UShort         /*aSize*/,
                                                              SChar        * aImage )
{
    idShmAddr       sAddObj4Before;
    idShmAddr       sAddObj4After;
    SChar         * sCurLogPtr;

    iduStShmMemChunk * sBefore;
    iduStShmMemChunk * sAfter;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4Before, ID_SIZEOF(idShmAddr) );
    sBefore = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4Before );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sBefore->mAddrNext, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4After, ID_SIZEOF(idShmAddr) );
    sAfter = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4After );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sAfter->mAddrPrev, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeUnlinkOnlyBefore( iduShmTxInfo     * aShmTxInfo,
                                                 iduStShmMemChunk * aBefore )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aBefore->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aBefore->mAddrNext, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_UNLINK_ONLY_BEFORE,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_UNLINK_ONLY_BEFORE( idvSQL       * /*aStatistics*/,
                                                               iduShmTxInfo * /*aShmTxInfo*/,
                                                               idrLSN         /*aNTALsn*/,
                                                               UShort         /*aSize*/,
                                                               SChar        * aImage )
{
    idShmAddr          sAddObj;
    SChar            * sCurLogPtr;
    iduStShmMemChunk * sBefore;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sBefore = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sBefore->mAddrNext, ID_SIZEOF(idShmAddr) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeFreeChunkCnt( iduShmTxInfo    * aShmTxInfo,
                                             iduStShmMemList * aPersListInfo )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aPersListInfo->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aPersListInfo->mFreeChunkCnt, ID_SIZEOF(vULong) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_FREECHUNKCNT,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_FREECHUNKCNT( idvSQL       * /*aStatistics*/,
                                                         iduShmTxInfo * /*aShmTxInfo*/,
                                                         idrLSN         /*aNTALsn*/,
                                                         UShort         /*aSize*/,
                                                         SChar        * aImage )
{
    idShmAddr         sAddObj;
    SChar           * sCurLogPtr;
    iduStShmMemList * sPersListInfo;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sPersListInfo = (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sPersListInfo->mFreeChunkCnt, ID_SIZEOF(vULong) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writePartChunkCnt( iduShmTxInfo    * aShmTxInfo,
                                             iduStShmMemList * aPersListInfo )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aPersListInfo->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aPersListInfo->mPartialChunkCnt, ID_SIZEOF(vULong) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_PARTCHUNKCNT,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_PARTCHUNKCNT( idvSQL        * /*aStatistics*/,
                                                         iduShmTxInfo  * /*aShmTxInfo*/,
                                                         idrLSN          /*aNTALsn*/,
                                                         UShort          /*aSize*/,
                                                         SChar         * aImage )
{
    idShmAddr         sAddObj;
    SChar           * sCurLogPtr;
    iduStShmMemList * sPersListInfo;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sPersListInfo = (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sPersListInfo->mPartialChunkCnt, ID_SIZEOF(vULong) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeFullChunkCnt( iduShmTxInfo    * aShmTxInfo,
                                             iduStShmMemList * aPersListInfo )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aPersListInfo->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aPersListInfo->mFullChunkCnt, ID_SIZEOF(vULong) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_FULLCHUNKCNT,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_FULLCHUNKCNT( idvSQL        * /*aStatistics*/,
                                                         iduShmTxInfo  * /*aShmTxInfo*/,
                                                         idrLSN          /*aNTALsn*/,
                                                         UShort          /*aSize*/,
                                                         SChar         * aImage )
{
    idShmAddr         sAddObj;
    SChar           * sCurLogPtr;
    iduStShmMemList * sPersListInfo;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sPersListInfo = (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sPersListInfo->mFullChunkCnt, ID_SIZEOF(vULong) );

    return IDE_SUCCESS;
}


IDE_RC iduVLogShmPersList::writeFreeSlotCnt( iduShmTxInfo     * aShmTxInfo,
                                            iduStShmMemChunk * aMyChunk )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aMyChunk->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aMyChunk->mFreeSlotCnt, ID_SIZEOF(UInt) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_FREESLOTCNT,
                         2,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_FREESLOTCNT( idvSQL        * /*aStatistics*/,
                                                        iduShmTxInfo  * /*aShmTxInfo*/,
                                                        idrLSN          /*aNTALsn*/,
                                                        UShort          /*aSize*/,
                                                        SChar         * aImage )
{
    idShmAddr          sAddObj;
    SChar            * sCurLogPtr;
    iduStShmMemChunk * sMyChunk;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sMyChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sMyChunk->mFreeSlotCnt, ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeAllocMyChunk( iduShmTxInfo     * aShmTxInfo,
                                             iduStShmMemChunk * aMyChunk )
{
    idrUImgInfo  sArrUImgInfo[3];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aMyChunk->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aMyChunk->mAddrTop, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aMyChunk->mFreeSlotCnt, ID_SIZEOF(UInt) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_ALLOC_MYCHUNK,
                         3,
                         sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_ALLOC_MYCHUNK( idvSQL        * /*aStatistics*/,
                                                          iduShmTxInfo  * /*aShmTxInfo*/,
                                                          idrLSN          /*aNTALsn*/,
                                                          UShort          /*aSize*/,
                                                          SChar         * aImage )
{
    idShmAddr          sAddObj;
    SChar            * sCurLogPtr;
    iduStShmMemChunk * sMyChunk;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj, ID_SIZEOF(idShmAddr) );
    sMyChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sMyChunk->mAddrTop, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sMyChunk->mFreeSlotCnt, ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeNTAAlloc( iduShmTxInfo    * aShmTxInfo,
                                         idrLSN            aNTALsn,
                                         iduStShmMemList * aPersListInfo,
                                         idShmAddr         aAddr4Mem )
{

    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aPersListInfo->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aAddr4Mem, ID_SIZEOF(idShmAddr) );

    idrLogMgr::writeNTALog( aShmTxInfo,
                            aNTALsn,
                            IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                            IDU_VLOG_TYPE_SHM_PERSLIST_ALLOC,
                            2,
                            sArrUImgInfo);

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_ALLOC( idvSQL          * aStatistics,
                                                  iduShmTxInfo    * aShmTxInfo,
                                                  idrLSN            aNTALsn,
                                                  UShort            /*aSize*/,
                                                  SChar           * aImage )
{
    SChar           * sCurLogPtr;
    iduStShmMemList * sPersListInfo;
    idShmAddr         sAddr4PersList;
    idShmAddr         sAddr4Mem;
    void            * sFreeElem;
    idrSVP            sVSavepoint;

    sCurLogPtr  = aImage;

    idrLogMgr::crtSavepoint( aShmTxInfo,
                             &sVSavepoint,
                             aNTALsn );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr,
                              &sAddr4PersList,
                              ID_SIZEOF(idShmAddr) );

    sPersListInfo = (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR_CHECK( sAddr4PersList );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddr4Mem, ID_SIZEOF(idShmAddr) );

    sFreeElem = IDU_SHM_GET_ADDR_PTR( sAddr4Mem );

    if( sPersListInfo->mUseLatch == ID_TRUE ) {

        IDE_ASSERT( iduShmLatchAcquire( aStatistics, aShmTxInfo, &sPersListInfo->mLatch ) == IDE_SUCCESS );

        IDE_ASSERT( iduShmPersList::memfree( aStatistics,
                                            aShmTxInfo,
                                            &sVSavepoint,
                                            sPersListInfo,
                                            sAddr4Mem,
                                            sFreeElem )
                    == IDE_SUCCESS );

        IDE_ASSERT( iduShmLatchRelease( aShmTxInfo, &sPersListInfo->mLatch ) == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( iduShmPersList::memfree( aStatistics,
                                            aShmTxInfo,
                                            &sVSavepoint,
                                            sPersListInfo,
                                            sAddr4Mem,
                                            sFreeElem )
                    == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::writeMemFree( iduShmTxInfo     * aShmTxInfo,
                                        iduStShmMemChunk * aCurChunk,
                                        iduStShmMemSlot  * aFreeElem )
{
    idrUImgInfo  sArrUImgInfo[4];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aFreeElem->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], &aCurChunk->mAddrSelf, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[2], &aCurChunk->mAddrTop, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[3], &aCurChunk->mFreeSlotCnt, ID_SIZEOF(UInt) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
                         IDU_VLOG_TYPE_SHM_PERSLIST_MEMFREE,
                         4,
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmPersList::undo_SHM_PERSLIST_MEMFREE( idvSQL        * /*aStatistics*/,
                                                    iduShmTxInfo  * /*aShmTxInfo*/,
                                                    idrLSN          /*aNTALsn*/,
                                                    UShort          /*aSize*/,
                                                    SChar         * aImage )
{
    idShmAddr          sAddObj4FreeElem;
    idShmAddr          sAddObj4CurChunk;
    SChar            * sCurLogPtr;

    iduStShmMemChunk * sCurChunk;

    sCurLogPtr  = aImage;

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4FreeElem, ID_SIZEOF(idShmAddr) );

    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sAddObj4CurChunk, ID_SIZEOF(idShmAddr) );
    sCurChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR( sAddObj4CurChunk );
    IDR_VLOG_GET_UIMG_N_NEXT( sCurLogPtr, &sCurChunk->mAddrTop, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_GET_UIMG( sCurLogPtr, &sCurChunk->mFreeSlotCnt, ID_SIZEOF(UInt) );

    return IDE_SUCCESS;
}

