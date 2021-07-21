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

#ifndef _O_CMT_COLLECTION_CLIENT_H_
#define _O_CMT_COLLECTION_CLIENT_H_ 1

typedef struct cmtCollection
{
    acp_uint8_t mType;

    union
    {
        cmtVariable   mVariable;
        cmtInVariable mInVariable;
    } mValue;

} cmtCollection;


ACI_RC cmtCollectionInitialize(cmtCollection *aCollection);
ACI_RC cmtCollectionFinalize(cmtCollection *aCollection);
ACI_RC cmtCollectionInitializeFromBlock(cmtCollection *aCollection, acp_uint8_t aType);

acp_uint8_t   cmtCollectionGetType(cmtCollection  *aCollection);
acp_uint32_t  cmtCollectionGetSize(cmtCollection  *aCollection);
acp_uint8_t  *cmtCollectionGetData(cmtCollection  *aCollection);
ACI_RC        cmtCollectionCopyData(cmtCollection *aCollection,
                                    acp_uint8_t   *aBuffer);

acp_uint32_t  cmtCollectionAnySize(cmtAny *aAny);
ACI_RC        cmtCollectionReadAnyNext(acp_uint8_t     *aCollection,     /*   IN      */
                                       acp_uint32_t    *aCursor,         /* IN/OUT    */
                                       cmtAny          *aAny);           /*   OUT     */
ACI_RC        cmtCollectionReadDataNext(acp_uint8_t    *aCollectionData, /*   [IN]    */
                                        acp_uint32_t   *aCursor,         /* [IN/OUT]  */
                                        acp_uint8_t    *aType,           /*   [OUT]   */
                                        acp_uint8_t   **aBuffer,         /*   [OUT]   */
                                        acp_uint32_t   *aParam1,         /*   [OUT]   */
                                        acp_uint32_t   *aParam2,         /*   [OUT]   */
                                        acp_uint64_t   *aParam3);        /*   [OUT]   */
void          cmtCollectionReadParamInfoSetNext(acp_uint8_t  *aCollectionData,
                                                acp_uint32_t *aCursor,
                                                void         *aCollectionParamInfoSet);
void          cmtCollectionWriteParamInfoSet(acp_uint8_t  *aCollectionData,
                                             acp_uint32_t *aCursor,
                                             void         *aCollectionParamInfoSet);
void          cmtCollectionReadColumnInfoGetResultNext(acp_uint8_t  *aCollectionData,
                                                       acp_uint32_t *aCursor,
                                                       void         *aCollectionColumnInfoGetResult);
void          cmtCollectionWriteColumnInfoGetResult(acp_uint8_t  *aCollectionData,
                                                    acp_uint32_t *aCursor,
                                                    void         *aCollectionColumnInfoGetResult);
void          cmtCollectionWriteAny(acp_uint8_t  *aCollectionData, /*   IN   */
                                    acp_uint32_t *aCursor,         /* IN/OUT */
                                    cmtAny       *aAny);           /*   OUT  */

ACI_RC        cmtCollectionWriteVariable(cmtCollection *aCollection,
                                         acp_uint8_t   *aBuffer,
                                         acp_uint32_t   aBufferSize);

ACI_RC        cmtCollectionWriteInVariable(cmtCollection *aCollection,
                                           acp_uint8_t   *aBuffer,
                                           acp_uint32_t   aBufferSize);

#ifdef ACP_CFG_BIG_ENDIAN

#define CMT_COLLECTION_READ_BYTE1(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        *(acp_uint8_t *)(aValue) = (aData)[sCursor];                        \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE2(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((acp_uint8_t *)(aValue))[0] = (aData)[sCursor + 0];                \
        ((acp_uint8_t *)(aValue))[1] = (aData)[sCursor + 1];                \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE4(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((acp_uint8_t *)(aValue))[0] = (aData)[sCursor + 0];                \
        ((acp_uint8_t *)(aValue))[1] = (aData)[sCursor + 1];                \
        ((acp_uint8_t *)(aValue))[2] = (aData)[sCursor + 2];                \
        ((acp_uint8_t *)(aValue))[3] = (aData)[sCursor + 3];                \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE8(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((acp_uint8_t *)(aValue))[0] = (aData)[sCursor + 0];                \
        ((acp_uint8_t *)(aValue))[1] = (aData)[sCursor + 1];                \
        ((acp_uint8_t *)(aValue))[2] = (aData)[sCursor + 2];                \
        ((acp_uint8_t *)(aValue))[3] = (aData)[sCursor + 3];                \
        ((acp_uint8_t *)(aValue))[4] = (aData)[sCursor + 4];                \
        ((acp_uint8_t *)(aValue))[5] = (aData)[sCursor + 5];                \
        ((acp_uint8_t *)(aValue))[6] = (aData)[sCursor + 6];                \
        ((acp_uint8_t *)(aValue))[7] = (aData)[sCursor + 7];                \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE1(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor] = (acp_uint8_t)aValue;                             \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE2(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((acp_uint8_t *)&(aValue))[0];               \
        (aData)[sCursor + 1] = ((acp_uint8_t *)&(aValue))[1];               \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE4(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((acp_uint8_t *)&(aValue))[0];               \
        (aData)[sCursor + 1] = ((acp_uint8_t *)&(aValue))[1];               \
        (aData)[sCursor + 2] = ((acp_uint8_t *)&(aValue))[2];               \
        (aData)[sCursor + 3] = ((acp_uint8_t *)&(aValue))[3];               \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE8(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((acp_uint8_t *)&(aValue))[0];               \
        (aData)[sCursor + 1] = ((acp_uint8_t *)&(aValue))[1];               \
        (aData)[sCursor + 2] = ((acp_uint8_t *)&(aValue))[2];               \
        (aData)[sCursor + 3] = ((acp_uint8_t *)&(aValue))[3];               \
        (aData)[sCursor + 4] = ((acp_uint8_t *)&(aValue))[4];               \
        (aData)[sCursor + 5] = ((acp_uint8_t *)&(aValue))[5];               \
        (aData)[sCursor + 6] = ((acp_uint8_t *)&(aValue))[6];               \
        (aData)[sCursor + 7] = ((acp_uint8_t *)&(aValue))[7];               \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#else /* ACP_CFG_BIG_ENDIAN */

#define CMT_COLLECTION_READ_BYTE1(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        *(acp_uint8_t *)(aValue) = (aData)[sCursor];                        \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE2(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((acp_uint8_t *)(aValue))[0] = (aData)[sCursor + 1];                \
        ((acp_uint8_t *)(aValue))[1] = (aData)[sCursor + 0];                \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE4(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((acp_uint8_t *)(aValue))[0] = (aData)[sCursor + 3];                \
        ((acp_uint8_t *)(aValue))[1] = (aData)[sCursor + 2];                \
        ((acp_uint8_t *)(aValue))[2] = (aData)[sCursor + 1];                \
        ((acp_uint8_t *)(aValue))[3] = (aData)[sCursor + 0];                \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_READ_BYTE8(aData, sCursor, aValue)                   \
    do                                                                      \
    {                                                                       \
        ((acp_uint8_t *)(aValue))[0] = (aData)[sCursor + 7];                \
        ((acp_uint8_t *)(aValue))[1] = (aData)[sCursor + 6];                \
        ((acp_uint8_t *)(aValue))[2] = (aData)[sCursor + 5];                \
        ((acp_uint8_t *)(aValue))[3] = (aData)[sCursor + 4];                \
        ((acp_uint8_t *)(aValue))[4] = (aData)[sCursor + 3];                \
        ((acp_uint8_t *)(aValue))[5] = (aData)[sCursor + 2];                \
        ((acp_uint8_t *)(aValue))[6] = (aData)[sCursor + 1];                \
        ((acp_uint8_t *)(aValue))[7] = (aData)[sCursor + 0];                \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE1(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor] = (acp_uint8_t)aValue;                             \
        sCursor += 1;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE2(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((acp_uint8_t *)&(aValue))[1];               \
        (aData)[sCursor + 1] = ((acp_uint8_t *)&(aValue))[0];               \
        sCursor += 2;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE4(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((acp_uint8_t *)&(aValue))[3];               \
        (aData)[sCursor + 1] = ((acp_uint8_t *)&(aValue))[2];               \
        (aData)[sCursor + 2] = ((acp_uint8_t *)&(aValue))[1];               \
        (aData)[sCursor + 3] = ((acp_uint8_t *)&(aValue))[0];               \
        sCursor += 4;                                                       \
    }                                                                       \
    while (0);

#define CMT_COLLECTION_WRITE_BYTE8(aData, sCursor, aValue)                  \
    do                                                                      \
    {                                                                       \
        (aData)[sCursor + 0] = ((acp_uint8_t *)&(aValue))[7];               \
        (aData)[sCursor + 1] = ((acp_uint8_t *)&(aValue))[6];               \
        (aData)[sCursor + 2] = ((acp_uint8_t *)&(aValue))[5];               \
        (aData)[sCursor + 3] = ((acp_uint8_t *)&(aValue))[4];               \
        (aData)[sCursor + 4] = ((acp_uint8_t *)&(aValue))[3];               \
        (aData)[sCursor + 5] = ((acp_uint8_t *)&(aValue))[2];               \
        (aData)[sCursor + 6] = ((acp_uint8_t *)&(aValue))[1];               \
        (aData)[sCursor + 7] = ((acp_uint8_t *)&(aValue))[0];               \
        sCursor += 8;                                                       \
    }                                                                       \
    while (0);

#endif /* ACP_CFG_BIG_ENDIAN */


#endif
