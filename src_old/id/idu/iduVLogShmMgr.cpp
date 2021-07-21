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
#include <iduVLogShmMgr.h>
#include <iduShmMgr.h>
#include <idwPMMgr.h>

idrUndoFunc iduVLogShmMgr::mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_MAX];
SChar       iduVLogShmMgr::mStrVLogType[IDU_VLOG_TYPE_SHM_MGR_MAX+1][100] =
{
    "IDU_VLOG_TYPE_SHM_MGR_NONE",
    "IDU_VLOG_TYPE_SHM_MGR_REGISTER_NEW_SEG",
    "IDU_VLOG_TYPE_SHM_MGR_REGISTER_OLD_SEG",
    "IDU_VLOG_TYPE_SHM_MGR_ALLOC_SEG",
    "IDU_VLOG_TYPE_SHM_MGR_INSERT_FREE_BLOCK",
    "IDU_VLOG_TYPE_SHM_MGR_REMOVE_FREE_BLOCK",
    "IDU_VLOG_TYPE_SHM_MGR_SPLIT_BLOCK",
    "IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_PREV",
    "IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_SIZE",
    "IDU_VLOG_TYPE_SHM_MGR_ALLOC_MEM",
    "IDU_VLOG_TYPE_SHM_MGR_MAX"
};

IDE_RC iduVLogShmMgr::initialize()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                                 mArrUndoFunction );

    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_NONE]              = NULL;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_REGISTER_NEW_SEG]  = undo_SHM_MGR_REGISTER_NEW_SEG;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_REGISTER_OLD_SEG]  = undo_SHM_MGR_REGISTER_OLD_SEG;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_ALLOC_SEG]         = undo_SHM_MGR_REGISTER_ALLOC_SEG;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_INSERT_FREE_BLOCK] = undo_SHM_MGR_INSERT_FREE_BLOCK;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_REMOVE_FREE_BLOCK] = undo_SHM_MGR_REMOVE_FREE_BLOCK;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_SPLIT_BLOCK]       = undo_SHM_MGR_SPLIT_BLOCK;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_PREV]    = undo_SHM_MGR_SET_BLOCK_PREV;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_SIZE]    = undo_SHM_MGR_SET_BLOCK_SIZE;
    mArrUndoFunction[IDU_VLOG_TYPE_SHM_MGR_ALLOC_MEM]         = undo_SHM_MGR_ALLOC_MEM;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::destroy()
{
    idrLogMgr::registerUndoFunc( IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                                 NULL /* aArrUndoFunc */ );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeRegisterNewSeg( iduShmTxInfo * aShmTxInfo,
                                           ULong          aSegCnt,
                                           ULong          aFstFreeSegInfoIdx )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aSegCnt,
                       ID_SIZEOF(ULong) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[1],
                       &aFstFreeSegInfoIdx,
                       ID_SIZEOF(ULong) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_REGISTER_NEW_SEG,
                         2, /* Undo Image Size */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_REGISTER_NEW_SEG( idvSQL       * /*aStatistics*/,
                                                     iduShmTxInfo * /*aShmTxInfo*/,
                                                     idrLSN         /*aNTALsn */,
                                                     UShort           aSize,
                                                     SChar          * aImage )
{
    SChar          * sLogPtr;
    iduShmSSegment * sSSegment =
        (iduShmSSegment*)iduShmMgr::getSegShmHeader( IDU_SHM_SYS_SEGMENT_ID );

    IDE_ASSERT( aSize == ID_SIZEOF(ULong) * 2 );

    sLogPtr  = aImage;
    idlOS::memcpy( &sSSegment->mSegCount, sLogPtr, ID_SIZEOF(ULong) );
    sLogPtr += ID_SIZEOF(ULong);

    iduShmMgr::initSegInfo( sSSegment->mSegCount );

    idlOS::memcpy( &sSSegment->mFstFreeSegInfoIdx, sLogPtr, ID_SIZEOF(ULong) );

    if( iduShmMgr::mArrSegment[sSSegment->mFstFreeSegInfoIdx] != NULL )
    {
        iduShmMgr::mArrSegment[sSSegment->mFstFreeSegInfoIdx] = NULL;
        iduShmMgr::mAttachSegCnt--;
    }

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeRegisterOldSeg( iduShmTxInfo * aShmTxInfo,
                                           ULong          aSegID,
                                           ULong          aAttachSegCnt )
{
    idrUImgInfo  sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0],
                       &aSegID,
                       ID_SIZEOF(ULong) );

    IDR_VLOG_SET_UIMG( sArrUImgInfo[1],
                       &aAttachSegCnt,
                       ID_SIZEOF(ULong) );

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_REGISTER_OLD_SEG,
                         2, /* Undo Image Size */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_REGISTER_OLD_SEG( idvSQL       * aStatistics,
                                                     iduShmTxInfo * /*aShmTxInfo*/,
                                                     idrLSN         /*aNTALsn*/,
                                                     UShort         aSize,
                                                     SChar        * aImage )
{
    SChar           * sLogPtr;
    ULong             sSegID;

    IDE_ASSERT( aSize == ID_SIZEOF(ULong) * 2 );

    sLogPtr  = aImage;
    idlOS::memcpy( &sSegID, sLogPtr, ID_SIZEOF(ULong) );
    sLogPtr += ID_SIZEOF(ULong);

    if( idwPMMgr::getProcState( aStatistics ) != IDU_SHM_PROC_STATE_RECOVERY )
    {
        iduShmMgr::mArrSegment[sSegID] = NULL;
        idlOS::memcpy( &iduShmMgr::mAttachSegCnt, sLogPtr, ID_SIZEOF(ULong) );
    }

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeAllocSeg( iduShmTxInfo * aShmTxInfo,
                                     iduSCH      *  aSCH )
{
    idrUImgInfo sArrUImgInfo[1];

    sArrUImgInfo[0].mSize = ID_SIZEOF(iduSCH);
    sArrUImgInfo[0].mData = (SChar*)aSCH;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_ALLOC_SEG,
                         1, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_REGISTER_ALLOC_SEG( idvSQL       * aStatistics,
                                                       iduShmTxInfo * /*aShmTxInfo*/,
                                                       idrLSN         /*aNTALsn*/,
                                                       UShort         aSize,
                                                       SChar        * aImage )
{
    iduSCH           sSCH;
    iduShmHeader   * sShmChunkHdr;
    iduShmProcState  sProcState;
    IDE_RC           sRC = IDE_SUCCESS;

    IDE_ASSERT( aSize == ID_SIZEOF(iduSCH) );

    idlOS::memcpy( &sSCH, aImage, ID_SIZEOF( iduSCH) );

    sProcState = idwPMMgr::getProcState( aStatistics );

    /* Shared Memory Chunk를 할당하다가 실패할 경우, WatchDog에 의해서
     * Undo가 되는 경우에는 해당 Shared Memory영역은 아직 Daemon Process
     * 영역에 Attach되어 있지 않기때문에 Attach를 하고 삭제연산을
     * 수행한다. */
    if( sProcState == IDU_SHM_PROC_STATE_RECOVERY )
    {
        sRC = iduShmChunkMgr::attachChunk( sSCH.mShmKey,
                                           sSCH.mSize,
                                           &sSCH );
    }
    else
    {
        IDE_ASSERT( sProcState == IDU_SHM_PROC_STATE_RUN );
    }

    if( sRC == IDE_SUCCESS )
    {
        sShmChunkHdr = (iduShmHeader*)sSCH.mChunkPtr;

        if( idlOS::strncmp( sShmChunkHdr->mSigniture,
                            IDU_SHM_SIGNATURE,
                            idlOS::strlen(IDU_SHM_SIGNATURE) ) == 0 )
        {
            (void)iduShmChunkMgr::removeChunck( (SChar*)sSCH.mChunkPtr,
                                                sSCH.mShmID );
        }
        else
        {
            (void)iduShmChunkMgr::detachChunk( (SChar*)sSCH.mChunkPtr );
        }
    }

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeInsertFreeBlock( iduShmTxInfo * aShmTxInfo,
                                            UInt           aFstLvlIdx,
                                            UInt           aSndLvlIdx,
                                            iduShmBlockHeader * aBlockHdr )
{
    idrUImgInfo sArrUImgInfo[6];

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData =
        (SChar*)&iduShmMgr::mSSegment->mFBMatrix[aFstLvlIdx][aSndLvlIdx];

    sArrUImgInfo[1].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[1].mData = (SChar*)&aFstLvlIdx;
    sArrUImgInfo[2].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[2].mData = (SChar*)&aSndLvlIdx;

    sArrUImgInfo[3].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[3].mData = (SChar*)&aBlockHdr->mAddrSelf;
    sArrUImgInfo[4].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[4].mData = (SChar*)&aBlockHdr->mFreeList.mAddrPrev;
    sArrUImgInfo[5].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[5].mData = (SChar*)&aBlockHdr->mFreeList.mAddrNext;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_INSERT_FREE_BLOCK,
                         6, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_INSERT_FREE_BLOCK( idvSQL       * /*aStatistics*/,
                                                      iduShmTxInfo * /*aShmTxInfo*/,
                                                      idrLSN         /*aNTALsn*/,
                                                      UShort         /*aSize*/,
                                                      SChar        * aImage )
{
    idShmAddr           sAddr4FstFreeBlock;
    UInt                sFstLvlIdx;
    UInt                sSndLvlIdx;
    iduShmBlockHeader * sShmBlockHdr;

    iduShmBlockHeader * sFreeBlockHdr;
    idShmAddr           sAddr4FreeBlock;
    idShmAddr           sAddr4FreeBlockPrev;
    idShmAddr           sAddr4FreeBlockNext;

    idlOS::memcpy( &sAddr4FstFreeBlock,
                   aImage,
                   ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sFstLvlIdx, aImage, ID_SIZEOF(UInt) );
    aImage += ID_SIZEOF(UInt);

    idlOS::memcpy( &sSndLvlIdx, aImage, ID_SIZEOF(UInt) );
    aImage += ID_SIZEOF(UInt);

    idlOS::memcpy( &sAddr4FreeBlock,
                   aImage,
                   ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sAddr4FreeBlockPrev,
                   aImage,
                   ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sAddr4FreeBlockNext,
                   aImage,
                   ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    sFreeBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4FreeBlock );
    sFreeBlockHdr->mFreeList.mAddrPrev = sAddr4FreeBlockPrev;
    sFreeBlockHdr->mFreeList.mAddrNext = sAddr4FreeBlockNext;

    if( sAddr4FstFreeBlock != IDU_SHM_NULL_ADDR )
    {
        sShmBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4FstFreeBlock );
        sShmBlockHdr->mFreeList.mAddrPrev = IDU_SHM_NULL_ADDR;
    }

    iduShmMgr::mSSegment->mFBMatrix[sFstLvlIdx][sSndLvlIdx] = sAddr4FstFreeBlock;

    if( sAddr4FstFreeBlock == IDU_SHM_NULL_ADDR )
    {
        iduShmMgr::mSSegment->mSndLvlBitmap[sFstLvlIdx] &= ~( 1 << sSndLvlIdx );

        /* If the second bitmap is now empty, clear the fl bitmap. */
        if( iduShmMgr::mSSegment->mSndLvlBitmap[sFstLvlIdx] == 0 )
        {
            iduShmMgr::mSSegment->mFstLvlBitmap &= ~(1 << sFstLvlIdx);
        }
    }

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeRemoveFreeBlock( iduShmTxInfo      * aShmTxInfo,
                                            iduShmBlockHeader * aBlockHdr,
                                            UInt                aFstLvlIdx,
                                            UInt                aSndLvlIdx )
{
    idrUImgInfo sArrUImgInfo[6];

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData =
        (SChar*)&iduShmMgr::mSSegment->mFBMatrix[aFstLvlIdx][aSndLvlIdx];

    sArrUImgInfo[1].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[1].mData = (SChar*)&aFstLvlIdx;
    sArrUImgInfo[2].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[2].mData = (SChar*)&aSndLvlIdx;

    sArrUImgInfo[3].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[3].mData = (SChar*)&aBlockHdr->mAddrSelf;
    sArrUImgInfo[4].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[4].mData = (SChar*)&aBlockHdr->mFreeList.mAddrPrev;
    sArrUImgInfo[5].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[5].mData = (SChar*)&aBlockHdr->mFreeList.mAddrNext;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_REMOVE_FREE_BLOCK,
                         6, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_REMOVE_FREE_BLOCK( idvSQL       * /*aStatistics*/,
                                                      iduShmTxInfo * /*aShmTxInfo*/,
                                                      idrLSN         /*aNTALsn*/,
                                                      UShort         /*aSize*/,
                                                      SChar        * aImage )
{
    idShmAddr           sAddr4FstFreeBlock;
    UInt                sFstLvlIdx;
    UInt                sSndLvlIdx;
    idShmAddr           sAddr4FreeBlock;
    idShmAddr           sAddr4PrvFreeBlock;
    idShmAddr           sAddr4NxtFreeBlock;
    iduShmBlockHeader * sPrvBlockHdr;
    iduShmBlockHeader * sNxtBlockHdr;

    idlOS::memcpy( &sAddr4FstFreeBlock,
                   aImage,
                   ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sFstLvlIdx, aImage, ID_SIZEOF(UInt) );
    aImage += ID_SIZEOF(UInt);

    idlOS::memcpy( &sSndLvlIdx, aImage, ID_SIZEOF(UInt) );
    aImage += ID_SIZEOF(UInt);

    idlOS::memcpy( &sAddr4FreeBlock, aImage, ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sAddr4PrvFreeBlock, aImage, ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sAddr4NxtFreeBlock, aImage, ID_SIZEOF(idShmAddr) );
    /* aImage += ID_SIZEOF(idShmAddr); */

    iduShmMgr::mSSegment->mFBMatrix[sFstLvlIdx][sSndLvlIdx] = sAddr4FstFreeBlock;

    iduShmMgr::mSSegment->mFstLvlBitmap |= (1 << sFstLvlIdx);
    iduShmMgr::mSSegment->mSndLvlBitmap[sFstLvlIdx] |= (1 << sSndLvlIdx);

    if( sAddr4PrvFreeBlock != IDU_SHM_NULL_ADDR )
    {
        sPrvBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4PrvFreeBlock );

        sPrvBlockHdr->mFreeList.mAddrNext = sAddr4FreeBlock;
    }

    if( sAddr4NxtFreeBlock != IDU_SHM_NULL_ADDR )
    {
        sNxtBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4NxtFreeBlock );

        sNxtBlockHdr->mFreeList.mAddrPrev = sAddr4FreeBlock;
    }

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeSplitBlock( iduShmTxInfo      * aShmTxInfo,
                                       iduShmBlockHeader * aBlockHdr )
{
    idrUImgInfo sArrUImgInfo[2];

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData = (SChar*)&aBlockHdr->mAddrSelf;
    sArrUImgInfo[1].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[1].mData = (SChar*)&aBlockHdr->mSize;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_SPLIT_BLOCK,
                         2, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_SPLIT_BLOCK( idvSQL       * /*aStatistics*/,
                                                iduShmTxInfo * /*aShmTxInfo*/,
                                                idrLSN         /*aNTALsn*/,
                                                UShort         /*aSize*/,
                                                SChar        * aImage )
{
    idShmAddr           sAddr4FreeBlock;
    iduShmBlockHeader * sPrvBlockHdr;
    UInt                sSize;

    idlOS::memcpy( &sAddr4FreeBlock, aImage, ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sSize, aImage, ID_SIZEOF(UInt) );
    /* aImage += ID_SIZEOF(UInt); */

    sPrvBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4FreeBlock );

    sPrvBlockHdr->mSize = sSize;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeSetBlockPrev( iduShmTxInfo      * aShmTxInfo,
                                         iduShmBlockHeader * aBlockHdr )
{
    idrUImgInfo sArrUImgInfo[2];
    iduShmBlockFooter *sPrevFooter = (iduShmBlockFooter*)aBlockHdr - 1;

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData = (SChar*)&aBlockHdr->mAddrSelf;
    sArrUImgInfo[1].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[1].mData = (SChar*)&sPrevFooter->mHeaderAddr;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_PREV,
                         2, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_SET_BLOCK_PREV( idvSQL       * /*aStatistics*/,
                                                   iduShmTxInfo * /*aShmTxInfo*/,
                                                   idrLSN         /*aNTALsn*/,
                                                   UShort         /*aSize*/,
                                                   SChar        * aImage )
{
    idShmAddr           sAddr4Block;
    idShmAddr           sAddr4PrvBlock;
    iduShmBlockFooter * sPrevFooter;
    iduShmBlockHeader * sBlockHdr;

    idlOS::memcpy( &sAddr4Block, aImage, ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sAddr4PrvBlock, aImage, ID_SIZEOF(idShmAddr) );

    sBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4Block );

    sPrevFooter = (iduShmBlockFooter*)sBlockHdr - 1;
    sPrevFooter->mHeaderAddr = sAddr4PrvBlock;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeSetBlockSize( iduShmTxInfo      * aShmTxInfo,
                                         iduShmBlockHeader * aBlockHdr )
{
    idrUImgInfo sArrUImgInfo[2];

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData = (SChar*)&aBlockHdr->mAddrSelf;
    sArrUImgInfo[1].mSize = ID_SIZEOF(UInt);
    sArrUImgInfo[1].mData = (SChar*)&aBlockHdr->mSize;

    idrLogMgr::writeLog( aShmTxInfo,
                         IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                         IDU_VLOG_TYPE_SHM_MGR_SET_BLOCK_SIZE,
                         2, /* Undo Info Count */
                         sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_SET_BLOCK_SIZE( idvSQL       * /*aStatistics*/,
                                                   iduShmTxInfo * /*aShmTxInfo*/,
                                                   idrLSN         /*aNTALsn*/,
                                                   UShort         /*aSize*/,
                                                   SChar        * aImage )
{
    idShmAddr           sAddr4Block;
    UInt                sSize;
    iduShmBlockHeader * sBlockHdr;

    idlOS::memcpy( &sAddr4Block, aImage, ID_SIZEOF(idShmAddr) );
    aImage += ID_SIZEOF(idShmAddr);

    idlOS::memcpy( &sSize, aImage, ID_SIZEOF(UInt) );

    sBlockHdr = iduShmMgr::getBlockPtrByAddr( sAddr4Block );

    sBlockHdr->mSize = sSize;

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::writeAllocMem( iduShmTxInfo      * aShmTxInfo,
                                     idrLSN              aPrvLSN,
                                     iduShmBlockHeader * aBlockHdr )
{
    idrUImgInfo sArrUImgInfo[2];

    sArrUImgInfo[0].mSize = ID_SIZEOF(idShmAddr);
    sArrUImgInfo[0].mData = (SChar*)&aBlockHdr->mAddrSelf;

    idrLogMgr::writeNTALog( aShmTxInfo,
                            aPrvLSN,
                            IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
                            IDU_VLOG_TYPE_SHM_MGR_ALLOC_MEM,
                            1,
                            sArrUImgInfo );

    return IDE_SUCCESS;
}

IDE_RC iduVLogShmMgr::undo_SHM_MGR_ALLOC_MEM( idvSQL       * aStatistics,
                                              iduShmTxInfo * aShmTxInfo,
                                              idrLSN         aNTALsn,
                                              UShort         /*aSize*/,
                                              SChar        * aImage )
{
    idShmAddr  sAddr4Obj;
    SChar    * sDataPtr;
    idrSVP     sSavepoint;

    idrLogMgr::crtSavepoint( aShmTxInfo,
                             &sSavepoint,
                             aNTALsn );

    idlOS::memcpy( &sAddr4Obj, aImage, ID_SIZEOF(idShmAddr) );

    sDataPtr = IDU_SHM_GET_ADDR_PTR( sAddr4Obj );

    return iduShmMgr::freeMem( aStatistics, aShmTxInfo, &sSavepoint, sDataPtr );
}
