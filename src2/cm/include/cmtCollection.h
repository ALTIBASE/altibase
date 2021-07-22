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

#ifndef _O_CMT_COLLECTION_H_
#define _O_CMT_COLLECTION_H_ 1

typedef struct cmtCollection
{
    UChar mType;

    union
    {
        cmtVariable   mVariable;
        cmtInVariable mInVariable;
    } mValue;
    
} cmtCollection;


IDE_RC cmtCollectionInitialize(cmtCollection *aCollection);
IDE_RC cmtCollectionFinalize(cmtCollection *aCollection);
IDE_RC cmtCollectionInitializeFromBlock(cmtCollection *aCollection, UChar aType);

UChar  cmtCollectionGetType(cmtCollection *aCollection);
UInt   cmtCollectionGetSize(cmtCollection *aCollection);
UChar *cmtCollectionGetData(cmtCollection *aCollection);
IDE_RC cmtCollectionCopyData(cmtCollection *aCollection,
                             UChar         *aBuffer);

UInt   cmtCollectionAnySize(cmtAny *aAny);
IDE_RC cmtCollectionReadAnyNext(UChar     *aCollection, /*   IN   */
                                UInt      *aCursor,     /* IN/OUT */
                                cmtAny    *aAny);       /*   OUT  */
IDE_RC cmtCollectionReadDataNext(UChar     *aCollectionData, /*   [IN]   */
                                 UInt      *aCursor,         /* [IN/OUT] */
                                 UChar     *aType,           /*   [OUT]   */
                                 UChar    **aBuffer,         /*   [OUT]  */
                                 UInt      *aDataLength,     /*   [OUT]  */
                                 UInt      *aPrecision,      /*   [OUT]  */
                                 ULong     *aLocationID);    /*   [OUT]  */
void   cmtCollectionReadParamInfoSetNext(UChar *aCollectionData,
                                         UInt  *aCursor,
                                         void  *aCollectionParamInfoSet);
void   cmtCollectionWriteParamInfoSet(UChar *aCollectionData,
                                      UInt  *aCursor,
                                      void  *aCollectionParamInfoSet);
void   cmtCollectionWriteColumnInfoGetResult(UChar  *aCollectionData,
                                             UInt   *aCursor,
                                             void   *aCollectionColumnInfoGetResult);
void   cmtCollectionWriteAny(UChar     *aCollectionData, /*   IN   */
                              UInt      *aCursor,          /* IN/OUT */
                              cmtAny    *aAny);            /*   OUT  */

IDE_RC cmtCollectionWriteVariable(cmtCollection *aCollection,
                                  UChar         *aBuffer,
                                  UInt           aBufferSize);

IDE_RC cmtCollectionWriteInVariable(cmtCollection *aCollection,
                                    UChar         *aBuffer,
                                    UInt           aBufferSize);

#ifdef ENDIAN_IS_BIG_ENDIAN

#define CMT_COLLECTION_READ_BYTE1(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        *(UChar *)(aValue) = (aData)[sCursor];                              \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE2(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aData)[sCursor + 0];                      \
        ((UChar *)(aValue))[1] = (aData)[sCursor + 1];                      \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE4(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aData)[sCursor + 0];                      \
        ((UChar *)(aValue))[1] = (aData)[sCursor + 1];                      \
        ((UChar *)(aValue))[2] = (aData)[sCursor + 2];                      \
        ((UChar *)(aValue))[3] = (aData)[sCursor + 3];                      \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE8(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aData)[sCursor + 0];                      \
        ((UChar *)(aValue))[1] = (aData)[sCursor + 1];                      \
        ((UChar *)(aValue))[2] = (aData)[sCursor + 2];                      \
        ((UChar *)(aValue))[3] = (aData)[sCursor + 3];                      \
        ((UChar *)(aValue))[4] = (aData)[sCursor + 4];                      \
        ((UChar *)(aValue))[5] = (aData)[sCursor + 5];                      \
        ((UChar *)(aValue))[6] = (aData)[sCursor + 6];                      \
        ((UChar *)(aValue))[7] = (aData)[sCursor + 7];                      \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE1(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor] = (UChar)aValue;                                   \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE2(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((UChar *)&(aValue))[0];                     \
        (aData)[sCursor + 1] = ((UChar *)&(aValue))[1];                     \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE4(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((UChar *)&(aValue))[0];                     \
        (aData)[sCursor + 1] = ((UChar *)&(aValue))[1];                     \
        (aData)[sCursor + 2] = ((UChar *)&(aValue))[2];                     \
        (aData)[sCursor + 3] = ((UChar *)&(aValue))[3];                     \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE8(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((UChar *)&(aValue))[0];                     \
        (aData)[sCursor + 1] = ((UChar *)&(aValue))[1];                     \
        (aData)[sCursor + 2] = ((UChar *)&(aValue))[2];                     \
        (aData)[sCursor + 3] = ((UChar *)&(aValue))[3];                     \
        (aData)[sCursor + 4] = ((UChar *)&(aValue))[4];                     \
        (aData)[sCursor + 5] = ((UChar *)&(aValue))[5];                     \
        (aData)[sCursor + 6] = ((UChar *)&(aValue))[6];                     \
        (aData)[sCursor + 7] = ((UChar *)&(aValue))[7];                     \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#else /* ENDIAN_IS_BIG_ENDIAN */

#define CMT_COLLECTION_READ_BYTE1(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        *(UChar *)(aValue) = (aData)[sCursor];                              \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE2(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aData)[sCursor + 1];                      \
        ((UChar *)(aValue))[1] = (aData)[sCursor + 0];                      \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE4(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aData)[sCursor + 3];                      \
        ((UChar *)(aValue))[1] = (aData)[sCursor + 2];                      \
        ((UChar *)(aValue))[2] = (aData)[sCursor + 1];                      \
        ((UChar *)(aValue))[3] = (aData)[sCursor + 0];                      \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE8(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((UChar *)(aValue))[0] = (aData)[sCursor + 7];                      \
        ((UChar *)(aValue))[1] = (aData)[sCursor + 6];                      \
        ((UChar *)(aValue))[2] = (aData)[sCursor + 5];                      \
        ((UChar *)(aValue))[3] = (aData)[sCursor + 4];                      \
        ((UChar *)(aValue))[4] = (aData)[sCursor + 3];                      \
        ((UChar *)(aValue))[5] = (aData)[sCursor + 2];                      \
        ((UChar *)(aValue))[6] = (aData)[sCursor + 1];                      \
        ((UChar *)(aValue))[7] = (aData)[sCursor + 0];                      \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE1(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor] = (UChar)aValue;                                   \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE2(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((UChar *)&(aValue))[1];                     \
        (aData)[sCursor + 1] = ((UChar *)&(aValue))[0];                     \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE4(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((UChar *)&(aValue))[3];                     \
        (aData)[sCursor + 1] = ((UChar *)&(aValue))[2];                     \
        (aData)[sCursor + 2] = ((UChar *)&(aValue))[1];                     \
        (aData)[sCursor + 3] = ((UChar *)&(aValue))[0];                     \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE8(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((UChar *)&(aValue))[7];                     \
        (aData)[sCursor + 1] = ((UChar *)&(aValue))[6];                     \
        (aData)[sCursor + 2] = ((UChar *)&(aValue))[5];                     \
        (aData)[sCursor + 3] = ((UChar *)&(aValue))[4];                     \
        (aData)[sCursor + 4] = ((UChar *)&(aValue))[3];                     \
        (aData)[sCursor + 5] = ((UChar *)&(aValue))[2];                     \
        (aData)[sCursor + 6] = ((UChar *)&(aValue))[1];                     \
        (aData)[sCursor + 7] = ((UChar *)&(aValue))[0];                     \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#endif /* ENDIAN_IS_BIG_ENDIAN */

/* PROJ-1920 */
#define CMT_COLLECTION_WRITE_PTR(aData, sCursor, aValue) \
    idlOS::memcpy(&aData[sCursor], &aValue, ID_SIZEOF(vULong)); \
    sCursor += ID_SIZEOF(vULong);

/* PROJ-1920 */
#define CMT_COLLECTION_READ_PTR(aData, sCursor, aValue) \
    idlOS::memcpy(aValue, &aData[sCursor], ID_SIZEOF(vULong)); \
    sCursor += ID_SIZEOF(vULong);

#endif
