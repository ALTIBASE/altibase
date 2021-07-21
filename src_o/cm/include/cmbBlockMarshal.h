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


#define CMB_BLOCK_TEST_READ_CURSOR(aBlock, aSize)                           \
    IDE_TEST(((aBlock)->mCursor + (aSize)) > (aBlock)->mDataSize)

#define CMB_BLOCK_TEST_WRITE_CURSOR(aBlock, aSize)                          \
    IDE_TEST(((aBlock)->mCursor + (aSize)) > (aBlock)->mBlockSize)


#define CMB_BLOCK_CHECK_READ_CURSOR(aBlock, aSize)                          \
    do                                                                      \
    {                                                                       \
        if (((aBlock)->mCursor + (aSize)) > (aBlock)->mDataSize)            \
        {                                                                   \
            *aSuccess = ID_FALSE;                                           \
            IDE_RAISE(EndOfBlock);                                          \
        }                                                                   \
        else                                                                \
        {                                                                   \
            *aSuccess = ID_TRUE;                                            \
        }                                                                   \
    } while (0)

#define CMB_BLOCK_CHECK_WRITE_CURSOR(aBlock, aSize)                         \
    do                                                                      \
    {                                                                       \
        if (((aBlock)->mCursor + (aSize)) > (aBlock)->mBlockSize)           \
        {                                                                   \
            *aSuccess = ID_FALSE;                                           \
            IDE_RAISE(EndOfBlock);                                          \
        }                                                                   \
        else                                                                \
        {                                                                   \
            *aSuccess = ID_TRUE;                                            \
        }                                                                   \
    } while (0)


#define CMB_BLOCK_MOVE_READ_CURSOR(aBlock, aDelta)                          \
    do                                                                      \
    {                                                                       \
        (aBlock)->mCursor += (aDelta);                                      \
    }                                                                       \
    while (0);

#define CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, aDelta)                         \
    do                                                                      \
    {                                                                       \
        (aBlock)->mCursor   += aDelta;                                      \
        (aBlock)->mDataSize  = (aBlock)->mCursor;                           \
    }                                                                       \
    while (0);


#ifdef ENDIAN_IS_BIG_ENDIAN

#define CMB_BLOCK_READ_BYTE1(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        *(UChar *)(aValue) = (aBlock)->mData[(aBlock)->mCursor];            \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 1);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_READ_BYTE2(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 0];    \
        ((UChar *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 1];    \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 2);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_READ_BYTE4(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 0];    \
        ((UChar *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 1];    \
        ((UChar *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 2];    \
        ((UChar *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 3];    \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 4);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_READ_BYTE8(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 0];    \
        ((UChar *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 1];    \
        ((UChar *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 2];    \
        ((UChar *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 3];    \
        ((UChar *)(aValue))[4] = (aBlock)->mData[(aBlock)->mCursor + 4];    \
        ((UChar *)(aValue))[5] = (aBlock)->mData[(aBlock)->mCursor + 5];    \
        ((UChar *)(aValue))[6] = (aBlock)->mData[(aBlock)->mCursor + 6];    \
        ((UChar *)(aValue))[7] = (aBlock)->mData[(aBlock)->mCursor + 7];    \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 8);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE1(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor] = (UChar)aValue;                 \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 1);                             \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE2(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((UChar *)&(aValue))[0];   \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((UChar *)&(aValue))[1];   \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 2);                             \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE4(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((UChar *)&(aValue))[0];   \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((UChar *)&(aValue))[1];   \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((UChar *)&(aValue))[2];   \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((UChar *)&(aValue))[3];   \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 4);                             \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE8(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((UChar *)&(aValue))[0];   \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((UChar *)&(aValue))[1];   \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((UChar *)&(aValue))[2];   \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((UChar *)&(aValue))[3];   \
        (aBlock)->mData[(aBlock)->mCursor + 4] = ((UChar *)&(aValue))[4];   \
        (aBlock)->mData[(aBlock)->mCursor + 5] = ((UChar *)&(aValue))[5];   \
        (aBlock)->mData[(aBlock)->mCursor + 6] = ((UChar *)&(aValue))[6];   \
        (aBlock)->mData[(aBlock)->mCursor + 7] = ((UChar *)&(aValue))[7];   \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 8);                             \
    }                                                                       \
    while (0);

#else /* ENDIAN_IS_BIG_ENDIAN */

#define CMB_BLOCK_READ_BYTE1(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        *(UChar *)(aValue) = (aBlock)->mData[(aBlock)->mCursor];            \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 1);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_READ_BYTE2(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 1];    \
        ((UChar *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 0];    \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 2);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_READ_BYTE4(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 3];    \
        ((UChar *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 2];    \
        ((UChar *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 1];    \
        ((UChar *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 0];    \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 4);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_READ_BYTE8(aBlock, aValue)                                \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aBlock)->mData[(aBlock)->mCursor + 7];    \
        ((UChar *)(aValue))[1] = (aBlock)->mData[(aBlock)->mCursor + 6];    \
        ((UChar *)(aValue))[2] = (aBlock)->mData[(aBlock)->mCursor + 5];    \
        ((UChar *)(aValue))[3] = (aBlock)->mData[(aBlock)->mCursor + 4];    \
        ((UChar *)(aValue))[4] = (aBlock)->mData[(aBlock)->mCursor + 3];    \
        ((UChar *)(aValue))[5] = (aBlock)->mData[(aBlock)->mCursor + 2];    \
        ((UChar *)(aValue))[6] = (aBlock)->mData[(aBlock)->mCursor + 1];    \
        ((UChar *)(aValue))[7] = (aBlock)->mData[(aBlock)->mCursor + 0];    \
                                                                            \
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, 8);                              \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE1(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor] = (UChar)aValue;                 \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 1);                             \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE2(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((UChar *)&(aValue))[1];   \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((UChar *)&(aValue))[0];   \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 2);                             \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE4(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((UChar *)&(aValue))[3];   \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((UChar *)&(aValue))[2];   \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((UChar *)&(aValue))[1];   \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((UChar *)&(aValue))[0];   \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 4);                             \
    }                                                                       \
    while (0);

#define CMB_BLOCK_WRITE_BYTE8(aBlock, aValue)                               \
    do                                                                      \
    {                                                                       \
        (aBlock)->mData[(aBlock)->mCursor + 0] = ((UChar *)&(aValue))[7];   \
        (aBlock)->mData[(aBlock)->mCursor + 1] = ((UChar *)&(aValue))[6];   \
        (aBlock)->mData[(aBlock)->mCursor + 2] = ((UChar *)&(aValue))[5];   \
        (aBlock)->mData[(aBlock)->mCursor + 3] = ((UChar *)&(aValue))[4];   \
        (aBlock)->mData[(aBlock)->mCursor + 4] = ((UChar *)&(aValue))[3];   \
        (aBlock)->mData[(aBlock)->mCursor + 5] = ((UChar *)&(aValue))[2];   \
        (aBlock)->mData[(aBlock)->mCursor + 6] = ((UChar *)&(aValue))[1];   \
        (aBlock)->mData[(aBlock)->mCursor + 7] = ((UChar *)&(aValue))[0];   \
                                                                            \
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, 8);                             \
    }                                                                       \
    while (0);

#endif /* ENDIAN_IS_BIG_ENDIAN */

/* PROJ-1920 */
#define CMB_BLOCK_WRITE_PTR(aBlock, aValue, sPtrSize)                       \
        idlOS::memcpy(&aBlock->mData[(aBlock)->mCursor], &aValue, sPtrSize);\
        CMB_BLOCK_MOVE_WRITE_CURSOR(aBlock, sPtrSize);

/* PROJ-1920 */
#define CMB_BLOCK_READ_PTR(aBlock, aValue, sPtrSize)                        \
        idlOS::memcpy(&aValue, &aBlock->mData[(aBlock)->mCursor], sPtrSize);\
        CMB_BLOCK_MOVE_READ_CURSOR(aBlock, sPtrSize);

#endif
