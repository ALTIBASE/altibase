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
#include <iduShmDump.h>

/// String Table
const SChar iduShmDump::mErrTextBufInsufficient[] =
    "error: Text Buffer Insufficient ";

IDE_RC iduShmDump::initialize()
{
    IDE_TEST( iduShmMgr::initialize( IDU_PROC_TYPE_USER )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::destroy()
{
    IDE_TEST( iduShmMgr::destroy( ID_FALSE ) /* NormalShutdown */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/////////////////////////////////////////////////////////////////////////////
/// message getters (message maker)

IDE_RC iduShmDump::getMsgSysSeg( SChar * aMsgBuf, UInt aBufSize )
{
    UInt             sWrittenBytes = 0;
    SChar            sLineBuf[LINE_BUF_MAX];
    iduShmSSegment * sSSegment;

    sSSegment = iduShmMgr::mSSegment;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- System Segment View ---- \n" );

    sWrittenBytes += getMsgShmHeader( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes );
    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "\n" );

    sWrittenBytes += getMsgShmStatictics( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes );
    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "\n" );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "First Bitmap: %s \n",
                                      stringizeBinStyle( sSSegment->mFstLvlBitmap,
                                                         IDU_SHM_FST_LVL_INDEX_MAX,
                                                         sLineBuf,
                                                         LINE_BUF_MAX ) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mMaxSegCount: %"ID_vULONG_FMT" \t",
                                      sSSegment->mMaxSegCount );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mSegSize: %"ID_UINT32_FMT" \t",
                                      sSSegment->mSegSize );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mSegCount: %"ID_vULONG_FMT" \n",
                                      sSSegment->mSegCount );

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::getMsgDataSeg( SChar * aMsgBuf, UInt aBufSize )
{
    UInt             sWrittenBytes = 0;
    iduShmSSegment * sSSegment;

    sSSegment = iduShmMgr::mSSegment;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Data Segment View ---- \n" );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mMaxSegCount: %"ID_vULONG_FMT" \t",
                                      sSSegment->mMaxSegCount );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mSegSize: %"ID_UINT32_FMT" \t",
                                      sSSegment->mSegSize );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mSegCount: %"ID_vULONG_FMT" \n",
                                      sSSegment->mSegCount );

    for( UInt i = 0 ; i < sSSegment->mSegCount ; i++ )
    {
        sWrittenBytes += getMsgStShmSegInfo( aMsgBuf + sWrittenBytes,
                                             aBufSize - sWrittenBytes,
                                             i );
    }

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::getMsgMatrix( SChar * aMsgBuf, UInt aBufSize )
{
    UInt             sWrittenBytes = 0;
    iduShmSSegment * sSSegment;

    sSSegment = iduShmMgr::mSSegment;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////


    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Matrix View ---- \n" );
    UInt sFstMask = 1;
    UInt sSndMask = 1;

    for( UInt i = 0 ; i < IDU_SHM_FST_LVL_INDEX_COUNT ; i++ )
    {
        sWrittenBytes +=
            idlOS::snprintf( aMsgBuf + sWrittenBytes,
                             aBufSize - sWrittenBytes,
                             "%c | ",
                             sSSegment->mFstLvlBitmap & sFstMask ? '1' : '0' );

        sSndMask = 1;

        for( UInt j = 0 ; j < IDU_SHM_SND_LVL_INDEX_COUNT ; j++ )
        {
            sWrittenBytes +=
                idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                 aBufSize - sWrittenBytes,
                                 "%c ",
                                 sSSegment->mSndLvlBitmap[i] & sSndMask ? '1' : '0' );

            sSndMask = sSndMask << 1;
        }

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "\n" );

        sFstMask = sFstMask << 1;
    }

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::getMsgFreeBlocks( SChar * aMsgBuf, UInt aBufSize )
{
    UInt                sWrittenBytes = 0;
    iduShmSSegment    * sSSegment;
    iduShmBlockHeader * sShmBlkHdr;
    idShmAddr           sLoopAddr;

    sSSegment = iduShmMgr::mSSegment;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////


    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Free Blocks ---- \n" );

    for( UInt i = 0 ; i < IDU_SHM_FST_LVL_INDEX_COUNT ; i++ )
    {
        for( UInt j = 0 ; j < IDU_SHM_SND_LVL_INDEX_COUNT ; j++ )
        {
            sLoopAddr = sSSegment->mFBMatrix[i][j];
            while( sLoopAddr != IDU_SHM_NULL_ADDR )
            {
                sShmBlkHdr = (iduShmBlockHeader*)
                    iduShmMgr::getBlockPtrByAddr( sLoopAddr );
                sWrittenBytes += getMsgShmBlock( aMsgBuf + sWrittenBytes,
                                                 aBufSize - sWrittenBytes,
                                                 sShmBlkHdr );
                sLoopAddr = sShmBlkHdr->mFreeList.mAddrNext;
            }
        }
    }

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::getMsgProcessMgrInfo( SChar * aMsgBuf, UInt aBufSize )
{
    UInt                sWrittenBytes = 0;
    SChar               sLineBuf[LINE_BUF_MAX];
    iduShmProcMgrInfo * sShmProcMgrInfo;

    sShmProcMgrInfo = &iduShmMgr::mSSegment->mProcMgrInfo;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////


    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Process Mgr Info ---- \n" );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Size : %"ID_INT32_FMT"\n",
                                      sizeof(iduShmProcMgrInfo) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "AddrSelf: %s \n",
                                      stringizeShmAddr(sShmProcMgrInfo->mAddrSelf,
                                                       sLineBuf,
                                                       LINE_BUF_MAX ) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "CurProcCnt: %"ID_UINT32_FMT" \n",
                                      sShmProcMgrInfo->mCurProcCnt );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Processes ---- \n" );

    for( UInt i = 0 ; i < IDU_MAX_PROCESS_COUNT ; i++ )
    {
        sWrittenBytes += getMsgProcWithThr( aMsgBuf + sWrittenBytes,
                                            aBufSize - sWrittenBytes,
                                            i,
                                            (idBool)ID_FALSE );
    }

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::getMsgProcess( SChar * aMsgBuf,
                                  UInt    aBufSize,
                                  UInt    aLPID )
{
    UInt                sWrittenBytes = 0;
    iduShmProcMgrInfo * sShmProcMgrInfo;
    iduShmProcInfo    * sShmProcInfo;
    UInt                sProcIndex;

    sShmProcMgrInfo = &iduShmMgr::mSSegment->mProcMgrInfo;
    sShmProcInfo    = sShmProcMgrInfo->mPRTable;
    sProcIndex      = aLPID;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Process Info ---- \n" );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "ProcInfo: 0x%08"
                                      ID_XPOINTER_FMT" ( Length: %"
                                      ID_INT32_FMT") \n",
                                      sShmProcInfo,
                                      sizeof( iduShmProcInfo ) );

    sWrittenBytes += getMsgProcWithThr( aMsgBuf + sWrittenBytes,
                                        aBufSize - sWrittenBytes,
                                        sProcIndex,
                                        (idBool)ID_TRUE );

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmDump::getMsgBinDump( SChar   * aMsgBuf,
                                  UInt      aBufSize,
                                  idShmAddr aShmAddr,
                                  UInt      aDumpLength )
{
    UInt       sWrittenBytes = 0;
    SChar      sLineBuf[LINE_BUF_MAX];
    SChar    * sPtrStart;
    UInt       sOffset = 0;
    UInt       sLineBytes = 16;

    sPtrStart = IDU_SHM_GET_ADDR_PTR( aShmAddr );

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Dump From %s - %p (Size: %"ID_INT32_FMT")\n",
                                      stringizeShmAddr( aShmAddr,
                                                        sLineBuf,
                                                        LINE_BUF_MAX ),
                                      sPtrStart,
                                      aDumpLength );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Begin of Binary Dump ---- \n" );

    while ( sOffset < aDumpLength )
    {
        sLineBytes = (aDumpLength - sOffset < 16 ? aDumpLength - sOffset : 16 );
        sLineBytes -= ((ULong)(sPtrStart + sOffset) % 16);

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "%s ",
                                          stringizeDumpAdr( sPtrStart + sOffset,
                                                            (idBool)ID_TRUE,
                                                            sLineBuf,
                                                            LINE_BUF_MAX ) );
        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "%s ",
                                          stringizeDumpHex( sPtrStart + sOffset,
                                                            sLineBytes,
                                                            (idBool)ID_TRUE,
                                                            sLineBuf,
                                                            LINE_BUF_MAX ) );
        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "%s\n",
                                          stringizeDumpBin( sPtrStart + sOffset,
                                                            sLineBytes,
                                                            (idBool)ID_TRUE,
                                                            sLineBuf,
                                                            LINE_BUF_MAX ) );
        sOffset += sLineBytes;
    }

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- End of Binary Dump ---- \n" );

    IDE_TEST_RAISE( sWrittenBytes >= (aBufSize  - 1),
                    err_textbuf_insufficient );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_textbuf_insufficient );
    {
        idlOS::printf( "%s (Current Text Buffer Length : %"ID_UINT32_FMT")\n",
                       mErrTextBufInsufficient,
                       aBufSize );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt iduShmDump::getMsgShmHeader( SChar * aMsgBuf, UInt aBufSize )
{
    UInt           sWrittenBytes = 0;
    SChar          sLineBuf[LINE_BUF_MAX];
    iduShmHeader * sShmHeader;

    sShmHeader = &iduShmMgr::mSSegment->mHeader;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "(iduShmHeader)Header: %p (Size : %"
                                      ID_INT32_FMT") \n",
                                      sShmHeader,
                                      sizeof(iduShmHeader) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Signiture: \"%s\" \n",
                                      sShmHeader->mSigniture );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "SegID: %"ID_INT32_FMT" \t",
                                      sShmHeader->mSegID );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Size: %"ID_INT32_FMT" \t",
                                      sShmHeader->mSize );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "ShmKey: %"ID_INT32_FMT" \n",
                                      sShmHeader->mShmKey );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "State: %"ID_INT32_FMT" (%s) \t",
                                      sShmHeader->mState,
                                      stringizeShmState( sShmHeader->mState ) );
    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Type: %"ID_UINT32_FMT" \n",
                                      sShmHeader->mType );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "CreateTime: %s \n",
                                      stringizeTimeval( &sShmHeader->mCreateTime,
                                                        sLineBuf,
                                                        LINE_BUF_MAX ) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mVersion: %s \t",
                                      sShmHeader->mVersion );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "mVersionID: %"ID_UINT32_FMT" \n",
                                      sShmHeader->mVersionID );

    return sWrittenBytes;
}

UInt iduShmDump::getMsgShmStatictics( SChar * aMsgBuf, UInt aBufSize )
{
    UInt               sWrittenBytes = 0;
    iduShmStatistics * sShmStatistics;

    sShmStatistics = &iduShmMgr::mSSegment->mStatistics;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "(iduShmStatistics)mStatistics: %p (Size : %"
                                      ID_UINT32_FMT") \n",
                                      sShmStatistics,
                                      sizeof(iduShmStatistics) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "TotalSize: %"ID_UINT32_FMT" \t",
                                      sShmStatistics->mTotalSize );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "AllocSize: %"ID_UINT32_FMT" \t",
                                      sShmStatistics->mAllocSize );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "FreeSize: %"ID_UINT32_FMT" \n",
                                      sShmStatistics->mFreeSize );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "FreeBlkCnt: %"ID_UINT32_FMT" \t",
                                      sShmStatistics->mFreeBlkCnt );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "FreeReqCnt: %"ID_UINT32_FMT" \t",
                                      sShmStatistics->mFreeReqCnt );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "AllocBlkCnt: %"ID_UINT32_FMT" \t",
                                      sShmStatistics->mAllocBlkCnt );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "AllocReqCnt: %"ID_UINT32_FMT" \n",
                                      sShmStatistics->mAllocReqCnt );

    return sWrittenBytes;
}

UInt iduShmDump::getMsgStShmSegInfo( SChar * aMsgBuf, UInt aBufSize, UInt aIndex )
{
    UInt              sWrittenBytes = 0;
    iduStShmSegInfo * sStShmSegInfo;

    sStShmSegInfo = &iduShmMgr::mSSegment->mArrSegInfo[aIndex];

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "ArrSegInfo[%"ID_UINT32_FMT"] : \t",
                                      aIndex );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "ShmKey: %"ID_INT32_FMT" \t",
                                      sStShmSegInfo->mShmKey );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Size: %"ID_UINT32_FMT" \n",
                                      sStShmSegInfo->mSize );

    return sWrittenBytes;
}

UInt iduShmDump::getMsgShmBlock( SChar * aMsgBuf, UInt aBufSize,
                                 iduShmBlockHeader * aShmBlkHeader )
{
    UInt                sWrittenBytes = 0;
    SChar               sLineBuf[LINE_BUF_MAX];
    iduShmBlockHeader * sShmBlkHdr;
    iduShmBlockFooter * sPrevFooter = (iduShmBlockFooter*)aShmBlkHeader - 1;

    sShmBlkHdr = aShmBlkHeader;

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////


    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "---- Block ---- \n" );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "PrvBlkAddr: %s\n",
                                      stringizeShmAddr( sPrevFooter->mHeaderAddr,
                                                        sLineBuf,
                                                        LINE_BUF_MAX ) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "AddrSelf  : %s \n",
                                      stringizeShmAddr( sShmBlkHdr->mAddrSelf,
                                                        sLineBuf,
                                                        LINE_BUF_MAX ) );

    sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                      aBufSize - sWrittenBytes,
                                      "Size: %"ID_UINT32_FMT" \n",
                                      sShmBlkHdr->mSize );

    return sWrittenBytes;
}

UInt iduShmDump::getMsgProcWithThr( SChar * aMsgBuf,
                                    UInt    aBufSize,
                                    UInt    aIndex,
                                    idBool  aPrintWithThreads )
{
    UInt             sWrittenBytes = 0;
    SChar            sLineBuf[LINE_BUF_MAX];
    iduShmProcInfo * sShmProcInfo;

    sShmProcInfo = &iduShmMgr::mSSegment->mProcMgrInfo.mPRTable[aIndex];

    /// 이 코드 아래로는 iduShmMgr으로부터 정보를 가져오지 않는다.
    /////////////////////////////////////////////////////////////////////////////

    /// NULL/INIT state는 printout 하지 않음.
    if( !(sShmProcInfo->mState == IDU_SHM_PROC_STATE_NULL ||
          sShmProcInfo->mState == IDU_SHM_PROC_STATE_INIT) )
    {
        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "SelfAddr: %s \n",
                                          stringizeShmAddr( sShmProcInfo->mSelfAddr,
                                                            sLineBuf,
                                                            LINE_BUF_MAX ) );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "PID: %"ID_vULONG_FMT" \t",
                                          sShmProcInfo->mPID );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "State: %"ID_INT32_FMT" (%s) \n",
                                          sShmProcInfo->mState,
                                          stringizeProcState( sShmProcInfo->mState ) );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "LPID: %"ID_UINT32_FMT" \t",
                                          sShmProcInfo->mLPID );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "Type: %"ID_INT32_FMT" (%s) \n",
                                          sShmProcInfo->mType,
                                          stringizeProcType( sShmProcInfo->mType ) );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "ThreadCnt: %"ID_UINT32_FMT" \n",
                                          sShmProcInfo->mThreadCnt );

        if( aPrintWithThreads == ID_TRUE )
        {
            sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                              aBufSize - sWrittenBytes,
                                              "---- ThrInfo ---- \n" );
            sWrittenBytes += getMsgThreads( aMsgBuf + sWrittenBytes,
                                            aBufSize - sWrittenBytes,
                                            &sShmProcInfo->mMainTxInfo );
        }
    }

    return sWrittenBytes;
}

UInt iduShmDump::getMsgThreads( SChar         * aMsgBuf,
                                UInt            aBufSize,
                                iduShmTxInfo  * aThrInfo )
{
    UInt             sWrittenBytes = 0;
    SChar            sLineBuf[LINE_BUF_MAX];
    iduShmTxInfo   * sShmThrInfo  = aThrInfo;
    iduShmListNode * sShmListNode = &aThrInfo->mNode;
    iduShmListNode * sShmListHead = &aThrInfo->mNode;

    // mNode는 Circular double linked list이다.
    while( sShmListHead->mAddrSelf != sShmListNode->mAddrNext )
    {
        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "Thread Addr: %s \n",
                                          stringizeShmAddr( sShmThrInfo->mSelfAddr,
                                                            sLineBuf,
                                                            LINE_BUF_MAX ) );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "Flag: %"ID_UINT32_FMT" \t",
                                          sShmThrInfo->mFlag );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "LPID: %"ID_UINT32_FMT" \t",
                                          sShmThrInfo->mLPID );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "ThrID: %"ID_UINT32_FMT" \n",
                                          sShmThrInfo->mThrID );

        sWrittenBytes += idlOS::snprintf( aMsgBuf + sWrittenBytes,
                                          aBufSize - sWrittenBytes,
                                          "State: %"ID_INT32_FMT" (%s) \n",
                                          sShmThrInfo->mState,
                                          stringizeThrState( sShmThrInfo->mState ) );

        sShmListNode = (iduShmListNode *)
            IDU_SHM_GET_ADDR_PTR(sShmListNode->mAddrNext);
        sShmThrInfo = (iduShmTxInfo *)
            IDU_SHM_GET_ADDR_PTR(sShmListNode->mAddrData);
    }

    return sWrittenBytes;
}

// enum type에 대한 stringize를 아래와 같이 처리하면,
// enum type의 definition이 바뀌었을 경우 STRINGIZE_ERROR을 출력하거나
// syntax error이 발생하여 이후 update에 대응할 수 있다.
const SChar * iduShmDump::stringizeShmState( iduShmState aState )
{
    if( IDU_SHM_STATE_INVALID == aState )
    {
        return STRINGIZE( IDU_SHM_STATE_INVALID );
    }
    if( IDU_SHM_STATE_CREATE == aState )
    {
        return STRINGIZE( IDU_SHM_STATE_CREATE );
    }
    if( IDU_SHM_STATE_INITIALIZE == aState )
    {
        return STRINGIZE( IDU_SHM_STATE_INITIALIZE );
    }

    return STRINGIZE( STRINGIZE_ERROR );
}

const SChar * iduShmDump::stringizeProcState( iduShmProcState aState )
{
    if( IDU_SHM_PROC_STATE_NULL == aState )
    {
        return STRINGIZE( IDU_SHM_PROC_STATE_NULL );
    }
    if ( IDU_SHM_PROC_STATE_INIT == aState )
    {
        return STRINGIZE( IDU_SHM_PROC_STATE_INIT );
    }
    if ( IDU_SHM_PROC_STATE_RUN == aState )
    {
        return STRINGIZE( IDU_SHM_PROC_STATE_RUN );
    }
    if ( IDU_SHM_PROC_STATE_DEAD == aState )
    {
        return STRINGIZE( IDU_SHM_PROC_STATE_DEAD );
    }
    if ( IDU_SHM_PROC_STATE_RECOVERY == aState )
    {
        return STRINGIZE( IDU_SHM_PROC_STATE_RECOVERY );
    }

    return STRINGIZE( STRINGIZE_ERROR );
}

const SChar * iduShmDump::stringizeProcType( iduShmProcType aType )
{
    if( IDU_PROC_TYPE_DAEMON == aType )
    {
        return STRINGIZE( IDU_PROC_TYPE_DAEMON );
    }
    if( IDU_PROC_TYPE_WSERVER == aType )
    {
        return STRINGIZE( IDU_PROC_TYPE_WSERVER );
    }
    if( IDU_PROC_TYPE_RSERVER == aType )
    {
        return STRINGIZE( IDU_PROC_TYPE_REPLICATION_MANAGER );
    }
    if( IDU_PROC_TYPE_USER == aType )
    {
        return STRINGIZE( IDU_PROC_TYPE_USER );
    }
    if ( IDU_SHM_PROC_TYPE_NULL == aType )
    {
        return STRINGIZE( IDU_SHM_PROC_TYPE_NULL );
    }

    return STRINGIZE( STRINGIZE_ERROR );
}

const SChar * iduShmDump::stringizeThrState( iduShmThrState aState )
{
    if( IDU_SHM_THR_STATE_RUN == aState )
    {
        return STRINGIZE( IDU_SHM_THR_STATE_RUN );
    }
    if( IDU_SHM_THR_STATE_WAIT == aState )
    {
        return STRINGIZE( IDU_SHM_THR_STATE_WAIT );
    }

    return STRINGIZE( STRINGIZE_ERROR );
}


SChar * iduShmDump::stringizeBinStyle( UInt    aBitmap,
                                       UInt    aMaxBit,
                                       SChar * aBuf,
                                       UInt    aBufLength )
{
    UInt  sWrittenBytes = 0;
    UInt  sMask = 1;

    if( aMaxBit > sizeof(sMask)*8 )
    { // aMask 크기보다 큰 bitmap은 고려되지 않았다.
        sWrittenBytes +=
            idlOS::snprintf( aBuf + sWrittenBytes,
                             aBufLength - sWrittenBytes,
                             "stringizeBinStyle: Error. aMaxbit is too big." );
    }

    for( UInt i = 0 ; i < aMaxBit ; i++ )
    {
        sWrittenBytes +=
            idlOS::snprintf( aBuf + sWrittenBytes,
                             aBufLength - sWrittenBytes,
                             "%c ",
                             aBitmap & sMask ? '1' : '0' );

        sMask = sMask << 1;
    }

    return aBuf;
}

SChar * iduShmDump::stringizeShmAddr( idShmAddr aShmAddr,
                                      SChar   * aBuf,
                                      UInt      aBufLength )
{
    idlOS::snprintf( aBuf,
                     aBufLength,
                     "[Addr Shm: %"ID_xINT64_FMT","
                     " SegID: %"ID_UINT32_FMT","
                     " Offset: %"ID_UINT32_FMT"]",
                     aShmAddr,
                     (UInt)IDU_SHM_GET_ADDR_SEGID(aShmAddr),
                     (UInt)IDU_SHM_GET_ADDR_OFFSET(aShmAddr) );

    return aBuf;
}

SChar * iduShmDump::stringizeTimeval( struct timeval * aTimeval,
                                      SChar          * aBuf,
                                      UInt             aBufLength )
{
    struct tm   sTime;
    time_t      sSec;

    sSec = (time_t)aTimeval->tv_sec;
    (void)idlOS::localtime_r( &sSec, &sTime );

    idlOS::snprintf( aBuf,
                     aBufLength,
                     "%04"ID_INT32_FMT
                     "-%02"ID_INT32_FMT
                     "-%02"ID_INT32_FMT
                     " %02"ID_INT32_FMT
                     ":%02"ID_INT32_FMT
                     ":%02"ID_INT32_FMT
                     ".%06"ID_INT32_FMT,
                     sTime.tm_year+1900,
                     sTime.tm_mon+1,
                     sTime.tm_mday,
                     sTime.tm_hour,
                     sTime.tm_min,
                     sTime.tm_sec,
                     aTimeval->tv_usec );

    return aBuf;
}

SChar * iduShmDump::stringizeDumpAdr( SChar * aPtr,
                                      idBool  aEnableAlign,
                                      SChar * aBuf,
                                      UInt    aBufLength )
{
    SChar * sPtr = aPtr;

    if( aEnableAlign == ID_TRUE )
    {
        // 주소를 편의를 위해 10단위(16진수)로 표기하기 위함.
        sPtr = sPtr - ((ULong)sPtr % 16);
    }

    idlOS::snprintf( aBuf,
                     aBufLength,
                     "%p",
                     sPtr );

    return aBuf;
}

SChar * iduShmDump::stringizeDumpBin( SChar * aPtr,
                                      UInt    aByte,
                                      idBool  aEnableAlign,
                                      SChar * aBuf,
                                      UInt    aBufLength )
{
    UInt sWrittenBytes = 0;
    UInt sSpaceCnt = 0;
    UInt i;

    if( aEnableAlign == ID_TRUE )
    { // line당 16byte 기준으로 앞부분 공백을 생성한다.
        sSpaceCnt = (ULong)aPtr % 16;

        for( UInt i = 0 ; i < sSpaceCnt ; i++ )
        {
            sWrittenBytes += idlOS::snprintf( aBuf + sWrittenBytes,
                                              aBufLength - sWrittenBytes,
                                              " " );
        }
    }

    for( i = 0 ; i < aByte ; i++ )
    { //print bin
        sWrittenBytes += idlOS::snprintf( aBuf + sWrittenBytes,
                                          aBufLength - sWrittenBytes,
                                          "%c",
                                          filterBin2ASCII( aPtr[i] ) );
    }

    for( i = 0 ; i < 16-(sSpaceCnt+aByte) ; i++ )
    {
        sWrittenBytes += idlOS::snprintf( aBuf + sWrittenBytes,
                                          aBufLength - sWrittenBytes,
                                          " " );
    }

    return aBuf;
}

SChar * iduShmDump::stringizeDumpHex( SChar * aPtr,
                                      UInt    aByte,
                                      idBool  aEnableAlign,
                                      SChar * aBuf,
                                      UInt    aBufLength )
{
    UInt sWrittenBytes = 0;
    UInt sSpaceCnt = 0;
    SChar sHexWord[3];

    if( aEnableAlign == ID_TRUE )
    {   // line당 16byte 기준으로 앞부분 공백을 생성한다.
        // hex는 byte당 2칸이므로 두칸 + 공백(3칸) 찍는다.
        sSpaceCnt = (ULong)aPtr % 16;

        for( UInt i = 0 ; i < sSpaceCnt ; i++ )
        {
            sWrittenBytes += idlOS::snprintf( aBuf + sWrittenBytes,
                                              aBufLength - sWrittenBytes,
                                              "   " );
        }
    }

    for( UInt i = 0 ; i < aByte ; i++ )
    { //print Hex
        convBin2Hex( aPtr[i], sHexWord );
        sWrittenBytes += idlOS::snprintf( aBuf + sWrittenBytes,
                                          aBufLength - sWrittenBytes,
                                          "%c%c ",
                                          sHexWord[0],
                                          sHexWord[1] );
    }

    if( aEnableAlign == ID_TRUE )
    { // 16byte 기준으로 남은 공간에 공백을 찍어준다.
        for( UInt i = 0 ; i < 16-(sSpaceCnt+aByte) ; i++ )
        {
            sWrittenBytes += idlOS::snprintf( aBuf + sWrittenBytes,
                                              aBufLength - sWrittenBytes,
                                              "   " );
        }
    }

    return aBuf;
}
