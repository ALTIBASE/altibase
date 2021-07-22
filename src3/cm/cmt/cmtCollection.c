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

#include <cmAllClient.h>


ACI_RC cmtCollectionInitialize(cmtCollection *aCollection)
{
    aCollection->mType = CMT_ID_NONE;

    return ACI_SUCCESS;
}

ACI_RC cmtCollectionFinalize(cmtCollection *aCollection)
{
    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        ACI_TEST(cmtVariableFinalize(&aCollection->mValue.mVariable) != ACI_SUCCESS);
    }
    else
    {
        ACE_DASSERT( aCollection->mType == CMT_ID_IN_VARIABLE );
    }

    aCollection->mType = CMT_ID_NONE;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtCollectionInitializeFromBlock(cmtCollection *aCollection, acp_uint8_t aType)
{
    switch (aType)
    {
        case CMT_ID_VARIABLE:
            ACI_TEST(cmtVariableInitialize(&aCollection->mValue.mVariable) != ACI_SUCCESS);
            break;

        case CMT_ID_IN_VARIABLE:
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    aCollection->mType = aType;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

acp_uint8_t cmtCollectionGetType(cmtCollection *aCollection)
{
    return aCollection->mType;
}

acp_uint32_t cmtCollectionGetSize(cmtCollection *aCollection)
{
    acp_uint32_t aSize;

    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        aSize = aCollection->mValue.mVariable.mTotalSize;
    }
    else
    {
        ACE_DASSERT( aCollection->mType == CMT_ID_IN_VARIABLE );
        aSize = aCollection->mValue.mInVariable.mSize;
    }
    return aSize;
}

acp_uint8_t *cmtCollectionGetData(cmtCollection *aCollection)
{
    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        return NULL;
    }
    else
    {
        return aCollection->mValue.mInVariable.mData;
    }
}

ACI_RC cmtCollectionCopyData(cmtCollection *aCollection,
                             acp_uint8_t   *aBuffer)
{
    acp_list_node_t  *sIterator;
    cmtVariablePiece *sPiece;

    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        ACP_LIST_ITERATE(&aCollection->mValue.mVariable.mPieceList, sIterator)
        {
            sPiece = (cmtVariablePiece *)sIterator->mObj;

            acpMemCpy(aBuffer + sPiece->mOffset,
                      sPiece->mData,
                      sPiece->mSize);
        }
    }
    else
    {
        acpMemCpy(aBuffer,
                  aCollection->mValue.mInVariable.mData,
                  aCollection->mValue.mInVariable.mSize);
    }

    return ACI_SUCCESS;
}

ACI_RC cmtCollectionReadAnyNext(acp_uint8_t   *aCollectionData, /*   [IN]   */
                                acp_uint32_t  *aCursor,         /* [IN/OUT] */
                                cmtAny        *aAny)            /*   [OUT]  */
{
    acp_uint32_t  sCursor;
    acp_uint8_t  *sData;
    acp_uint8_t   sInVariableDelimeter;

    /*
     * Cursor 위치 복사
     */
    sCursor = *aCursor;
    sData   = aCollectionData;

    /*
     * Type ID 읽음
     */
    CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mType)

    /*
     * Type 별로 Data 읽음
     */
    switch (aAny->mType)
    {
        case CMT_ID_NULL:
            break;
        case CMT_ID_SINT8:
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mSInt8)
            break;
        case CMT_ID_UINT8:
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mUInt8)
            break;
        case CMT_ID_SINT16:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &aAny->mValue.mSInt16)
            break;
        case CMT_ID_UINT16:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &aAny->mValue.mUInt16)
            break;
        case CMT_ID_SINT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mSInt32)
            break;
        case CMT_ID_UINT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mUInt32)
            break;
        case CMT_ID_SINT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &aAny->mValue.mSInt64)
            break;
        case CMT_ID_UINT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &aAny->mValue.mUInt64)
            break;
        case CMT_ID_FLOAT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mFloat32)
            break;
        case CMT_ID_FLOAT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &aAny->mValue.mFloat64)
            break;
        case CMT_ID_DATETIME:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &aAny->mValue.mDateTime.mYear );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mDateTime.mMonth );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mDateTime.mDay );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mDateTime.mHour );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mDateTime.mMinute );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mDateTime.mSecond );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mDateTime.mMicroSecond );
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &aAny->mValue.mDateTime.mTimeZone );
            break;
        case CMT_ID_INTERVAL:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &aAny->mValue.mInterval.mSecond);
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &aAny->mValue.mInterval.mMicroSecond);
            break;
        case CMT_ID_NUMERIC:
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mNumeric.mSize );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mNumeric.mPrecision );
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &aAny->mValue.mNumeric.mScale );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &aAny->mValue.mNumeric.mSign );
            acpMemCpy( aAny->mValue.mNumeric.mData,
                       sData + sCursor,
                       aAny->mValue.mNumeric.mSize );
            sCursor += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mInVariable.mSize );
            aAny->mValue.mInVariable.mData = sData + sCursor;
            sCursor += aAny->mValue.mInVariable.mSize;
            /* skip in-variable data delimeter */
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sInVariableDelimeter );
            break;
        case CMT_ID_IN_BIT:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mInBit.mPrecision );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mInBit.mData.mSize );
            aAny->mValue.mInBit.mData.mData = sData + sCursor;
            sCursor += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_IN_NIBBLE:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mInNibble.mPrecision );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mInNibble.mData.mSize );
            aAny->mValue.mInNibble.mData.mData = sData + sCursor;
            sCursor += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &aAny->mValue.mLobLocator.mLocator );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mLobLocator.mSize );
            break;
        default:
            ACE_DASSERT(0);
            break;
    }

    *aCursor = sCursor;

    return ACI_SUCCESS;
}

ACI_RC cmtCollectionReadDataNext(acp_uint8_t     *aCollectionData, /*   [IN]   */
                                 acp_uint32_t    *aCursor,         /* [IN/OUT] */
                                 acp_uint8_t     *aType,           /*   [OUT]   */
                                 acp_uint8_t    **aBuffer,         /*   [OUT]  */
                                 acp_uint32_t    *aDataLength,     /*   [OUT]  */
                                 acp_uint32_t    *aPrecision,      /*   [OUT]  */
                                 acp_uint64_t    *aLocatorID)      /*   [OUT]  */
{
    acp_uint32_t    sCursor;
    acp_uint8_t    *sData;
    acp_uint8_t     sInVariableDelimeter;
    acp_uint8_t     sType;
    acp_uint32_t    sDataLength = 0;
    acp_uint32_t    sPrecision = 0;
    cmtDateTime     *sDateTime;
    cmtInterval     *sInterval;
    cmtNumeric      *sNumeric;
    acp_uint64_t     sLocatorID;

    /*
     * Cursor 위치 복사
     */
    sCursor = *aCursor;
    sData   = aCollectionData;

    /*
     * Type ID 읽음
     */
    CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sType );

    /*
     * Type 별로 Data 읽음
     */

    switch (sType)
    {
        case CMT_ID_NULL:
            break;
        case CMT_ID_SINT8:
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, *aBuffer);
            sDataLength = ACI_SIZEOF(acp_sint8_t);
            break;
        case CMT_ID_UINT8:
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_uint8_t);
            break;
        case CMT_ID_SINT16:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_sint16_t);
            break;
        case CMT_ID_UINT16:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_uint16_t);
            break;
        case CMT_ID_SINT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_sint32_t);
            break;
        case CMT_ID_UINT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_uint32_t);
            break;
        case CMT_ID_SINT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_sint64_t);
            break;
        case CMT_ID_UINT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_uint64_t);
            break;
        case CMT_ID_FLOAT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_float_t);
            break;
        case CMT_ID_FLOAT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, *aBuffer );
            sDataLength = ACI_SIZEOF(acp_double_t);
            break;
        case CMT_ID_DATETIME:
            sDateTime = (cmtDateTime*)*aBuffer;
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &sDateTime->mYear );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sDateTime->mMonth );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sDateTime->mDay );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sDateTime->mHour );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sDateTime->mMinute );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sDateTime->mSecond );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sDateTime->mMicroSecond );
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &sDateTime->mTimeZone );
            sDataLength = ACI_SIZEOF(cmtDateTime);
            break;
        case CMT_ID_INTERVAL:
            sInterval = (cmtInterval*)*aBuffer;
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &sInterval->mSecond);
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &sInterval->mMicroSecond);
            sDataLength = ACI_SIZEOF(cmtInterval);
            break;
        case CMT_ID_NUMERIC:
            sNumeric = (cmtNumeric*)*aBuffer;
            acpMemSet(sNumeric, 0, ACI_SIZEOF(cmtNumeric));
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sNumeric->mSize );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sNumeric->mPrecision );
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &sNumeric->mScale );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sNumeric->mSign );
            acpMemCpy( sNumeric->mData,
                       sData + sCursor,
                       sNumeric->mSize );
            sCursor += sNumeric->mSize;
            sDataLength = ACI_SIZEOF(cmtNumeric);
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sDataLength );
            *aBuffer = sData + sCursor;
            sCursor += sDataLength;
            /* skip in-variable data delimeter */
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sInVariableDelimeter );
            break;
        case CMT_ID_IN_BIT:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sPrecision );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sDataLength );
            *aBuffer = sData + sCursor;
            sCursor += sDataLength;
            break;
        case CMT_ID_IN_NIBBLE:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sPrecision );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sDataLength );
            *aBuffer = sData + sCursor;
            sCursor += sDataLength;
            break;
        case CMT_ID_LOBLOCATOR:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &sLocatorID );
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &sDataLength );
            /* fix BUG-27332, Code-Sonar UMR  */
            *aLocatorID = sLocatorID;
            break;
        default:
            ACE_DASSERT(0);
            break;
    }

    *aType   = sType;
    *aCursor = sCursor;
    *aDataLength = sDataLength;
    *aPrecision = sPrecision;


    return ACI_SUCCESS;
}

void cmtCollectionReadParamInfoSetNext(acp_uint8_t  *aCollectionData,
                                       acp_uint32_t *aCursor,
                                       void         *aCollectionParamInfoSet)
{
    acp_uint32_t                     sCursor;
    acp_uint8_t                     *sData;
    cmpCollectionDBParamInfoSetA5 *sCollectionParamInfoSet;

    /*
     * Cursor 위치 복사
     */
    sCursor = *aCursor;
    sData   = aCollectionData;
    sCollectionParamInfoSet = (cmpCollectionDBParamInfoSetA5*)aCollectionParamInfoSet;

    CMT_COLLECTION_READ_BYTE2(sData, sCursor, &sCollectionParamInfoSet->mParamNumber);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionParamInfoSet->mDataType);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionParamInfoSet->mLanguage);
    CMT_COLLECTION_READ_BYTE1(sData, sCursor, &sCollectionParamInfoSet->mArguments);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionParamInfoSet->mPrecision);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionParamInfoSet->mScale);
    CMT_COLLECTION_READ_BYTE1(sData, sCursor, &sCollectionParamInfoSet->mInOutType);

    *aCursor = sCursor;
}

void cmtCollectionWriteParamInfoSet(acp_uint8_t  *aCollectionData,
                                    acp_uint32_t *aCursor,
                                    void         *aCollectionParamInfoSet)
{
    acp_uint32_t                     sCursor;
    acp_uint8_t                     *sData;
    cmpCollectionDBParamInfoSetA5 *sCollectionParamInfoSet;

    /*
     * Cursor 위치 복사
     */
    sCursor = *aCursor;
    sData   = aCollectionData;
    sCollectionParamInfoSet = (cmpCollectionDBParamInfoSetA5*)aCollectionParamInfoSet;

    CMT_COLLECTION_WRITE_BYTE2(sData, sCursor, sCollectionParamInfoSet->mParamNumber);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionParamInfoSet->mDataType);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionParamInfoSet->mLanguage);
    CMT_COLLECTION_WRITE_BYTE1(sData, sCursor, sCollectionParamInfoSet->mArguments);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionParamInfoSet->mPrecision);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionParamInfoSet->mScale);
    CMT_COLLECTION_WRITE_BYTE1(sData, sCursor, sCollectionParamInfoSet->mInOutType);

    *aCursor = sCursor;
}

void cmtCollectionReadColumnInfoGetResultNext(acp_uint8_t  *aCollectionData,
                                              acp_uint32_t *aCursor,
                                              void         *aCollectionColumnInfoGetResult)
{
    acp_uint32_t                            sCursor;
    acp_uint8_t                            *sData;
    cmpCollectionDBColumnInfoGetResultA5 *sCollectionColumnInfoGetResult;

    sCursor = *aCursor;
    sData   = aCollectionData;
    sCollectionColumnInfoGetResult = (cmpCollectionDBColumnInfoGetResultA5*)aCollectionColumnInfoGetResult;

    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionColumnInfoGetResult->mDataType);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionColumnInfoGetResult->mLanguage);
    CMT_COLLECTION_READ_BYTE1(sData, sCursor, &sCollectionColumnInfoGetResult->mArguments);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionColumnInfoGetResult->mPrecision);
    CMT_COLLECTION_READ_BYTE4(sData, sCursor, &sCollectionColumnInfoGetResult->mScale);

    /* PROJ-1789 Updatable Scrollable Cursor */
    CMT_COLLECTION_READ_BYTE1(sData, sCursor, &sCollectionColumnInfoGetResult->mNullableFlag);
    ACE_ASSERT( cmtCollectionReadAnyNext(sData, &sCursor, &sCollectionColumnInfoGetResult->mDisplayName) == ACI_SUCCESS );

    *aCursor = sCursor;
}

void cmtCollectionWriteColumnInfoGetResult(acp_uint8_t  *aCollectionData,
                                           acp_uint32_t *aCursor,
                                           void         *aCollectionColumnInfoGetResult)
{
    acp_uint32_t                            sCursor;
    acp_uint8_t                            *sData;
    cmpCollectionDBColumnInfoGetResultA5 *sCollectionColumnInfoGetResult;

    sCursor = *aCursor;
    sData   = aCollectionData;
    sCollectionColumnInfoGetResult = (cmpCollectionDBColumnInfoGetResultA5*)aCollectionColumnInfoGetResult;

    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mDataType);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mLanguage);
    CMT_COLLECTION_WRITE_BYTE1(sData, sCursor, sCollectionColumnInfoGetResult->mArguments);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mPrecision);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mScale);

    /* PROJ-1789 Updatable Scrollable Cursor */
    CMT_COLLECTION_WRITE_BYTE1(sData, sCursor, sCollectionColumnInfoGetResult->mNullableFlag);
    cmtCollectionWriteAny(sData, &sCursor, &sCollectionColumnInfoGetResult->mDisplayName);

    *aCursor = sCursor;
}

acp_uint32_t cmtCollectionAnySize(cmtAny *aAny)
{
    acp_uint32_t sSize;

    /* TYPE(1) */
    sSize = 1;

    switch( aAny->mType )
    {
        case CMT_ID_SINT8:
        case CMT_ID_UINT8:
            sSize += 1;
            break;
        case CMT_ID_SINT16:
        case CMT_ID_UINT16:
            sSize += 2;
            break;
        case CMT_ID_SINT32:
        case CMT_ID_UINT32:
        case CMT_ID_FLOAT32:
            sSize += 4;
            break;
        case CMT_ID_SINT64:
        case CMT_ID_UINT64:
        case CMT_ID_FLOAT64:
            sSize += 8;
            break;
        case CMT_ID_DATETIME:
            sSize += 13;
            break;
        case CMT_ID_INTERVAL:
            sSize += 16;
            break;
        case CMT_ID_NUMERIC:
            /* NUMERIC_HEADER_SIZE(5) */
            sSize += 5;
            /* NUMERIC SIZE(x) */
            sSize += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            /* IN HEADER(4) */
            sSize += 4;
            /* IN SIZE(x) */
            sSize += aAny->mValue.mInVariable.mSize;
            /* IN delimeter */
            sSize += 1;
            break;
        case CMT_ID_IN_BIT:
            /* IN_BIT_HEADER(4) */
            sSize += 4;
            /* IN_VARIABLE_HEADER(4) */
            sSize += 4;
            /* BIT SIZE(x) */
            sSize += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_IN_NIBBLE:
            /* IN_NIBBLE_HEADER(4) */
            sSize += 4;
            /* IN_VARIABLE_HEADER(4) */
            sSize += 4;
            /* NIBBLE SIZE(x) */
            sSize += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            sSize += 12;
            break;
        default:
            ACE_DASSERT(0);
            break;
    }

    return sSize;
}

void cmtCollectionWriteAny(acp_uint8_t     *aCollectionData, /*   [IN]   */
                           acp_uint32_t    *aCursor,         /* [IN/OUT] */
                           cmtAny          *aAny)
{
    acp_uint32_t  sCursor;
    acp_uint8_t  *sData;
    acp_uint8_t   sInVariableDelimeter = 0;

    /*
     * Cursor 위치 복사
     */
    sCursor = *aCursor;
    sData   = aCollectionData;

    /*
     * Type ID 범위 검사
     */
    ACE_DASSERT( aAny->mType != CMT_ID_VARIABLE );

    /*
     * Type ID 씀
     */
    CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mType)

    /*
     * Type 별로 Data 씀
     */
    switch (aAny->mType)
    {
        case CMT_ID_NULL:
            break;
        case CMT_ID_SINT8:
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mSInt8)
            break;
        case CMT_ID_UINT8:
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mUInt8)
            break;
        case CMT_ID_SINT16:
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mSInt16)
            break;
        case CMT_ID_UINT16:
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mUInt16)
            break;
        case CMT_ID_SINT32:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mSInt32)
            break;
        case CMT_ID_UINT32:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mUInt32)
            break;
        case CMT_ID_SINT64:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mSInt64)
            break;
        case CMT_ID_UINT64:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mUInt64)
            break;
        case CMT_ID_FLOAT32:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mFloat32)
            break;
        case CMT_ID_FLOAT64:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mFloat64)
            break;
        case CMT_ID_DATETIME:
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mDateTime.mYear );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mMonth );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mDay );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mHour );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mMinute );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mDateTime.mSecond );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mDateTime.mMicroSecond );
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mDateTime.mTimeZone );
            break;
        case CMT_ID_INTERVAL:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mInterval.mSecond);
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mInterval.mMicroSecond);
            break;
        case CMT_ID_NUMERIC:
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mNumeric.mSize );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mNumeric.mPrecision );
            CMT_COLLECTION_WRITE_BYTE2( sData, sCursor, aAny->mValue.mNumeric.mScale );
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, aAny->mValue.mNumeric.mSign );
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mNumeric.mData,
                       aAny->mValue.mNumeric.mSize );
            sCursor += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInVariable.mSize );
            if (aAny->mValue.mInVariable.mSize > 0)
            {
                acpMemCpy( sData + sCursor,
                           aAny->mValue.mInVariable.mData,
                           aAny->mValue.mInVariable.mSize );
                sCursor += aAny->mValue.mInVariable.mSize;
            }
            /* Make in-variable data null terminated string */
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sInVariableDelimeter );
            break;
        case CMT_ID_IN_BIT:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mData.mSize );
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mInBit.mData.mData,
                       aAny->mValue.mInBit.mData.mSize );
            sCursor += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_IN_NIBBLE:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mData.mSize );
            acpMemCpy( sData + sCursor,
                       aAny->mValue.mInNibble.mData.mData,
                       aAny->mValue.mInNibble.mData.mSize );
            sCursor += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mLobLocator.mLocator );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mLobLocator.mSize );
            break;
        default:
            ACE_DASSERT(0);
            break;
    }

    *aCursor = sCursor;
}

ACI_RC cmtCollectionWriteVariable(cmtCollection *aCollection,
                                  acp_uint8_t   *aBuffer,
                                  acp_uint32_t   aBufferSize)
{
    aCollection->mType = CMT_ID_VARIABLE;

    ACI_TEST(cmtVariableInitialize(&aCollection->mValue.mVariable) != ACI_SUCCESS);

    ACI_TEST(cmtVariableSetData(&aCollection->mValue.mVariable,
                                aBuffer,
                                aBufferSize) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmtCollectionWriteInVariable(cmtCollection *aCollection,
                                    acp_uint8_t     *aBuffer,
                                    acp_uint32_t     aBufferSize)
{
    aCollection->mType = CMT_ID_IN_VARIABLE;

    aCollection->mValue.mInVariable.mSize = aBufferSize;
    aCollection->mValue.mInVariable.mData = aBuffer;

    return ACI_SUCCESS;
}

