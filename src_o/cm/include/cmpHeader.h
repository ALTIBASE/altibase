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

#ifndef _O_CMP_HEADER_H_
#define _O_CMP_HEADER_H_ 1


/*
 * BASE Protocol Header
 */

#define CMP_HEADER_SIZE              16

// proj_2160 cm_type removal
#define CMP_HEADER_SIGN_A5           0x06 // fixed value
#define CMP_HEADER_SIGN_A7           0x07 // fixed value

#define CMP_HEADER_IS_A7(aHeader)  \
    (((aHeader)->mA7.mHeaderSign) == CMP_HEADER_SIGN_A7)

// packetType is set according to the header sign
typedef enum
{
    CMP_PACKET_TYPE_A5 = 0,
    CMP_PACKET_TYPE_A7 = 1,
    CMP_PACKET_TYPE_UNKNOWN = 255  // 최초의 값
} cmpPacketType;

//=========================================================
// macros for cmSeqNo can be used for both A5 and A7
#define CMP_HEADER_MAX_SEQ_NO                ((UInt)0x7fffffff)
#define CMP_HEADER_SEQ_NO(aHeader)           (((aHeader)->mA7.mCmSeqNo) & 0x7fffffff)

#define CMP_HEADER_PROTO_END_IS_SET(aHeader) \
    ((((aHeader)->mA7.mCmSeqNo) & 0x80000000) ? ID_TRUE : ID_FALSE)

#define CMP_HEADER_SET_PROTO_END(aHeader)    (aHeader)->mA7.mCmSeqNo |= 0x80000000
#define CMP_HEADER_CLR_PROTO_END(aHeader)    (aHeader)->mA7.mCmSeqNo &= 0x7fffffff

/*
 * Header flag bit mask
 */ 
#define CMP_HEADER_FLAG_COMPRESS_MASK        (0x0001)

#define CMP_HEADER_FLAG_COMPRESS_IS_SET( aHeader )              \
    ( ( ( (aHeader)->mA7.mFlag ) & CMP_HEADER_FLAG_COMPRESS_MASK ) ? \
      ID_TRUE : ID_FALSE )
#define CMP_HEADER_FLAG_SET_COMPRESS( aHeader )                 \
    ( (aHeader)->mA7.mFlag |= CMP_HEADER_FLAG_COMPRESS_MASK )
#define CMP_HEADER_FLAG_CLR_COMPRESS( aHeader )                 \
    ( (aHeader)->mA7.mFlag &= ~CMP_HEADER_FLAG_COMPRESS_MASK )

/* BUG-38102 */
#define CMP_HEADER_FLAG_ENCRYPT_MASK        (0x0002)

#define CMP_HEADER_FLAG_ENCRYPT_IS_SET( aHeader )              \
    ( ( ( (aHeader)->mA7.mFlag ) & CMP_HEADER_FLAG_ENCRYPT_MASK ) ? \
      ID_TRUE : ID_FALSE )
#define CMP_HEADER_FLAG_SET_ENCRYPT( aHeader )                 \
    ( (aHeader)->mA7.mFlag |= CMP_HEADER_FLAG_ENCRYPT_MASK )
#define CMP_HEADER_FLAG_CLR_ENCRYPT( aHeader )                 \
    ( (aHeader)->mA7.mFlag &= ~CMP_HEADER_FLAG_ENCRYPT_MASK )


typedef struct cmpHeaderA5
{
    UChar  mHeaderSign;     // 0x06, cm minor version
    UChar  mNextHeaderType;
    UShort mPayloadLength;

    UInt   mCmSeqNo;

    UShort mSerialNo;
    UChar  mModuleID;
    UChar  mModuleVersion;

    UShort mSourceSessionID;
    UShort mTargetSessionID;
} cmpHeaderA5;

typedef struct cmpHeaderA7
{
    UChar  mHeaderSign;     // 0x06: A5, 0x07: A7 or higher
    UChar  mReserved1;      // 0x00
    UShort mPayloadLength;  // data length except header

    // msb(bit31): last protcol end flag (1: end, 0: continued)
    // bit0~bit30: cyclic packet seq number (0 ~ 0x7fffffff)
    UInt   mCmSeqNo;    

    UShort mFlag;           /* PROJ-2296 */
    UShort mSessionID;      // mmcSession's ID, 0x00 at first

    UInt   mReserved2;
} cmpHeaderA7;

typedef union cmpHeader
{
    cmpHeaderA5 mA5;
    cmpHeaderA7 mA7;
} cmpHeader;

#endif
