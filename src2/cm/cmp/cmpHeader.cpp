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

#include <cmAll.h>

/* initialize writeBlock's header */
IDE_RC cmpHeaderInitialize(cmpHeader *aHeader)
{
    idlOS::memset(aHeader, 0x00, ID_SIZEOF(cmpHeader));
    aHeader->mA7.mHeaderSign     = CMP_HEADER_SIGN_A7;
    aHeader->mA7.mReserved1      = 0; // fixed value
    aHeader->mA7.mCmSeqNo        = 0; // started from 0
    return IDE_SUCCESS;
}

/* initialize header to A5 type */
IDE_RC cmpHeaderInitializeForA5( cmpHeader *aHeader )
{
    idlOS::memset(aHeader, 0x00, ID_SIZEOF(cmpHeader));

    aHeader->mA5.mHeaderSign     = CMP_HEADER_SIGN_A5;
    aHeader->mA5.mNextHeaderType = 0;
    aHeader->mA5.mPayloadLength  = 0;

    aHeader->mA5.mCmSeqNo        = 0;
    aHeader->mA5.mSerialNo       = 0;

    return IDE_SUCCESS;
}

IDE_RC cmpHeaderRead(cmnLinkPeer *aLink, cmpHeader *aHeader, cmbBlock *aBlock)
{
    SChar  sPeerInfo[IDL_IP_ADDR_MAX_LEN];

    /*
     * Cursor 위치 세팅
     */
    aBlock->mCursor = 0;

    CMB_BLOCK_TEST_READ_CURSOR(aBlock, CMP_HEADER_SIZE);

    /* proj_2160 cm_type removal */
    CMB_BLOCK_READ_BYTE1(aBlock, &aHeader->mA7.mHeaderSign);

    /* proj_2160 cm_type removal
     * This block of code is really important.
     * if server receive the first packet from client,
     * Link's flag is set to A5 or A7 right here. */
    if (aLink->mLink.mPacketType == CMP_PACKET_TYPE_UNKNOWN)
    {
        if (CMP_HEADER_IS_A7(aHeader))
        {
            aLink->mLink.mPacketType = CMP_PACKET_TYPE_A7;
        }
        else
        {
            aLink->mLink.mPacketType = CMP_PACKET_TYPE_A5;
        }
    }

    if (aLink->mLink.mPacketType != CMP_PACKET_TYPE_A5)
    {
        CMB_BLOCK_READ_BYTE1(aBlock, &aHeader->mA7.mReserved1);
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA7.mPayloadLength);
        CMB_BLOCK_READ_BYTE4(aBlock, &aHeader->mA7.mCmSeqNo);
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA7.mFlag);        
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA7.mSessionID);
        aBlock->mCursor += 4; // mReserved3
    }
    else
    {
        CMB_BLOCK_READ_BYTE1(aBlock, &aHeader->mA5.mNextHeaderType);
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA5.mPayloadLength);
        CMB_BLOCK_READ_BYTE4(aBlock, &aHeader->mA5.mCmSeqNo);
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA5.mSerialNo);
        CMB_BLOCK_READ_BYTE1(aBlock, &aHeader->mA5.mModuleID);
        CMB_BLOCK_READ_BYTE1(aBlock, &aHeader->mA5.mModuleVersion);
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA5.mSourceSessionID);
        CMB_BLOCK_READ_BYTE2(aBlock, &aHeader->mA5.mTargetSessionID);
    }

    // check altibase header version: 0x07 or 0x06
    // both A5 and A7 header have the same fields until 8 bytes
    IDE_TEST((aHeader->mA7.mHeaderSign != CMP_HEADER_SIGN_A7) &&
             (aHeader->mA7.mHeaderSign != CMP_HEADER_SIGN_A5));
    IDE_TEST((aHeader->mA7.mPayloadLength +  CMP_HEADER_SIZE) > aBlock->mBlockSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        // BUG-24993 프로토콜 헤더에러 메시지 상세화
        aLink->mPeerOp->mGetInfo(aLink, sPeerInfo, ID_SIZEOF(sPeerInfo),
                                 CMI_LINK_INFO_ALL);
        IDE_SET(ideSetErrorCode(cmERR_ABORT_PROTOCOL_HEADER_ERROR, sPeerInfo));
    }

    return IDE_FAILURE;
}

IDE_RC cmpHeaderWrite(cmpHeader *aHeader, cmbBlock *aBlock)
{
    UShort sCursor;

    /*
     * Cursor 위치와 DataSize 일치여부 검사
     */
    IDE_ASSERT(aBlock->mCursor == aBlock->mDataSize);

    /*
     * Cursor 위치 저장
     */
    sCursor = aBlock->mCursor;
    aBlock->mCursor = 0;

    // proj_2160 cm_type removal
    // both A5 and A7 header have the same fields until 8 bytes
    aHeader->mA7.mPayloadLength = aBlock->mDataSize - CMP_HEADER_SIZE;
    CMB_BLOCK_WRITE_BYTE1(aBlock, aHeader->mA7.mHeaderSign);
    CMB_BLOCK_WRITE_BYTE1(aBlock, aHeader->mA7.mReserved1);
    CMB_BLOCK_WRITE_BYTE2(aBlock, aHeader->mA7.mPayloadLength);
    CMB_BLOCK_WRITE_BYTE4(aBlock, aHeader->mA7.mCmSeqNo);

    if (CMP_HEADER_IS_A7(aHeader))
    {
        CMB_BLOCK_WRITE_BYTE2( aBlock, aHeader->mA7.mFlag );
        
        CMB_BLOCK_WRITE_BYTE2(aBlock, aHeader->mA7.mSessionID);
        CMB_BLOCK_WRITE_BYTE4(aBlock, aHeader->mA7.mReserved2);
    }
    else
    {
        CMB_BLOCK_WRITE_BYTE2(aBlock, aHeader->mA5.mSerialNo);
        CMB_BLOCK_WRITE_BYTE1(aBlock, aHeader->mA5.mModuleID);
        CMB_BLOCK_WRITE_BYTE1(aBlock, aHeader->mA5.mModuleVersion);
        CMB_BLOCK_WRITE_BYTE2(aBlock, aHeader->mA5.mSourceSessionID);
        CMB_BLOCK_WRITE_BYTE2(aBlock, aHeader->mA5.mTargetSessionID);
    }

    /*
     * Cursor 위치 및 Date Size 복구
     */
    aBlock->mCursor   = sCursor;
    aBlock->mDataSize = sCursor;

    return IDE_SUCCESS;
}
