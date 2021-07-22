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

#ifndef _O_CMB_BLOCK_MARSHAL_H_
#define _O_CMB_BLOCK_MARSHAL_H_ 1


#define CMB_BLOCK_TEST_READ_CURSOR(aBlock, aSize)                               \
    ACI_TEST(((aBlock)->mCursor + (aSize)) > (aBlock)->mDataSize)

#define CMB_BLOCK_TEST_WRITE_CURSOR(aBlock, aSize)                              \
    ACI_TEST(((aBlock)->mCursor + (aSize)) > (aBlock)->mBlockSize)


#define CMB_BLOCK_CHECK_READ_CURSOR(aBlock, aSize)                              \
    do                                                                          \
    {                                                                           \
        if (((aBlock)->mCursor + (aSize)) > (aBlock)->mDataSize)                \
        {                                                                       \
            *aSuccess = ACP_FALSE;                                              \
            ACI_RAISE(EndOfBlock);                                              \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            *aSuccess = ACP_TRUE;                                               \
        }                                                                       \
    } while (0)

#define CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, aSize)                             \
    do                                                                          \
    {                                                                           \
        if (((aBlock)->mCursor + (aSize)) > (aBlock)->mBlockSize)               \
        {                                                                       \
            *aSuccess = ACP_FALSE;                                              \
            ACI_RAISE(EndOfBlock);                                              \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            *aSuccess = ACP_TRUE;                                               \
        }                                                                       \
    } while (0)


#define CMB_BLOCK_MOVE_READ_CURSOR(aBlock, aDelta)                              \
    do                                                                          \
    {                                                                           \
        (aBlock)->mCursor += (aDelta);                                          \
    }                                                                           \
    while (0);

#define CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, aDelta)                             \
    do                                                                          \
    {                                                                           \
        (aBlock)->mCursor   += aDelta;                                          \
        (aBlock)->mDataSize  = (aBlock)->mCursor;                               \
    }                                                                           \
    while (0);


#ifdef ACP_CFG_BIG_ENDIAN

#define CMB_BLOCK_READ_BYTE1(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        *(acp_uint8_t *)(aValue) = (aBlock)->mData[(aBlock)->mCursor];          \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 1);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_READ_BYTE2(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        ((acp_uint8_t *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 0];  \
        ((acp_uint8_t *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 1];  \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 2);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_READ_BYTE4(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        ((acp_uint8_t *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 0];  \
        ((acp_uint8_t *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 1];  \
        ((acp_uint8_t *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 2];  \
        ((acp_uint8_t *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 3];  \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 4);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_READ_BYTE8(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        ((acp_uint8_t *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 0];  \
        ((acp_uint8_t *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 1];  \
        ((acp_uint8_t *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 2];  \
        ((acp_uint8_t *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 3];  \
        ((acp_uint8_t *)(aValue))[4] = (aBlock)->mData[(aBlock)->mCursor + 4];  \
        ((acp_uint8_t *)(aValue))[5] = (aBlock)->mData[(aBlock)->mCursor + 5];  \
        ((acp_uint8_t *)(aValue))[6] = (aBlock)->mData[(aBlock)->mCursor + 6];  \
        ((acp_uint8_t *)(aValue))[7] = (aBlock)->mData[(aBlock)->mCursor + 7];  \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 8);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE1(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor] = (acp_uint8_t)aValue;               \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 1);                                 \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE2(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((acp_uint8_t *)&(aValue))[0]; \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((acp_uint8_t *)&(aValue))[1]; \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 2);                                 \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE4(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((acp_uint8_t *)&(aValue))[0]; \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((acp_uint8_t *)&(aValue))[1]; \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((acp_uint8_t *)&(aValue))[2]; \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((acp_uint8_t *)&(aValue))[3]; \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 4);                                 \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE8(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((acp_uint8_t *)&(aValue))[0]; \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((acp_uint8_t *)&(aValue))[1]; \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((acp_uint8_t *)&(aValue))[2]; \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((acp_uint8_t *)&(aValue))[3]; \
        (aBlock)->mData[(aBlock)->mCursor + 4] = ((acp_uint8_t *)&(aValue))[4]; \
        (aBlock)->mData[(aBlock)->mCursor + 5] = ((acp_uint8_t *)&(aValue))[5]; \
        (aBlock)->mData[(aBlock)->mCursor + 6] = ((acp_uint8_t *)&(aValue))[6]; \
        (aBlock)->mData[(aBlock)->mCursor + 7] = ((acp_uint8_t *)&(aValue))[7]; \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 8);                                 \
    }                                                                           \
    while (0);

#else /* ACP_CFG_BIG_ENDIAN */

#define CMB_BLOCK_READ_BYTE1(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        *(acp_uint8_t *)(aValue) = (aBlock)->mData[(aBlock)->mCursor];          \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 1);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_READ_BYTE2(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        ((acp_uint8_t *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 1];  \
        ((acp_uint8_t *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 0];  \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 2);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_READ_BYTE4(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        ((acp_uint8_t *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 3];  \
        ((acp_uint8_t *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 2];  \
        ((acp_uint8_t *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 1];  \
        ((acp_uint8_t *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 0];  \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 4);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_READ_BYTE8(aBlock, aValue)                                    \
    do                                                                          \
    {                                                                           \
        ((acp_uint8_t *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 7];  \
        ((acp_uint8_t *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 6];  \
        ((acp_uint8_t *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 5];  \
        ((acp_uint8_t *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 4];  \
        ((acp_uint8_t *)(aValue))[4] = (aBlock)->mData[(aBlock)->mCursor + 3];  \
        ((acp_uint8_t *)(aValue))[5] = (aBlock)->mData[(aBlock)->mCursor + 2];  \
        ((acp_uint8_t *)(aValue))[6] = (aBlock)->mData[(aBlock)->mCursor + 1];  \
        ((acp_uint8_t *)(aValue))[7] = (aBlock)->mData[(aBlock)->mCursor + 0];  \
                                                                                \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 8);                                  \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE1(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor] = (acp_uint8_t)aValue;               \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 1);                                 \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE2(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((acp_uint8_t *)&(aValue))[1]; \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((acp_uint8_t *)&(aValue))[0]; \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 2);                                 \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE4(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((acp_uint8_t *)&(aValue))[3]; \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((acp_uint8_t *)&(aValue))[2]; \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((acp_uint8_t *)&(aValue))[1]; \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((acp_uint8_t *)&(aValue))[0]; \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 4);                                 \
    }                                                                           \
    while (0);

#define CMB_BLOCK_WRITE_BYTE8(aBlock, aValue)                                   \
    do                                                                          \
    {                                                                           \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((acp_uint8_t *)&(aValue))[7]; \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((acp_uint8_t *)&(aValue))[6]; \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((acp_uint8_t *)&(aValue))[5]; \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((acp_uint8_t *)&(aValue))[4]; \
        (aBlock)->mData[(aBlock)->mCursor + 4] = ((acp_uint8_t *)&(aValue))[3]; \
        (aBlock)->mData[(aBlock)->mCursor + 5] = ((acp_uint8_t *)&(aValue))[2]; \
        (aBlock)->mData[(aBlock)->mCursor + 6] = ((acp_uint8_t *)&(aValue))[1]; \
        (aBlock)->mData[(aBlock)->mCursor + 7] = ((acp_uint8_t *)&(aValue))[0]; \
                                                                                \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 8);                                 \
    }                                                                           \
    while (0);

#endif /* ACP_CFG_BIG_ENDIAN */


#endif
