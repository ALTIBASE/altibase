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


IDE_RC cmtCollectionInitialize(cmtCollection *aCollection)
{
    aCollection->mType = CMT_ID_NONE;

    return IDE_SUCCESS;
}

IDE_RC cmtCollectionFinalize(cmtCollection *aCollection)
{
    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        IDE_TEST(cmtVariableFinalize(&aCollection->mValue.mVariable) != IDE_SUCCESS);
    }
    else
    {
        IDE_DASSERT( aCollection->mType == CMT_ID_IN_VARIABLE );
    }
    
    aCollection->mType = CMT_ID_NONE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmtCollectionInitializeFromBlock(cmtCollection *aCollection, UChar aType)
{
    switch (aType)
    {
        case CMT_ID_VARIABLE:
            IDE_TEST(cmtVariableInitialize(&aCollection->mValue.mVariable) != IDE_SUCCESS);
            break;
            
        case CMT_ID_IN_VARIABLE:
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    aCollection->mType = aType;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UChar cmtCollectionGetType(cmtCollection *aCollection)
{
    return aCollection->mType;
}

UInt cmtCollectionGetSize(cmtCollection *aCollection)
{
    UInt aSize;
    
    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        aSize = aCollection->mValue.mVariable.mTotalSize;
    }
    else
    {
        IDE_DASSERT( aCollection->mType == CMT_ID_IN_VARIABLE );
        aSize = aCollection->mValue.mInVariable.mSize;
    }
    return aSize;
}

UChar *cmtCollectionGetData(cmtCollection *aCollection)
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

IDE_RC cmtCollectionCopyData(cmtCollection *aCollection,
                             UChar         *aBuffer)
{
    iduListNode      *sIterator;
    cmtVariablePiece *sPiece;

    if( aCollection->mType == CMT_ID_VARIABLE )
    {
        IDU_LIST_ITERATE(&aCollection->mValue.mVariable.mPieceList, sIterator)
        {
            sPiece = (cmtVariablePiece *)sIterator->mObj;

            idlOS::memcpy(aBuffer + sPiece->mOffset,
                          sPiece->mData,
                          sPiece->mSize);
        }
    }
    else
    {
        idlOS::memcpy(aBuffer,
                      aCollection->mValue.mInVariable.mData,
                      aCollection->mValue.mInVariable.mSize);
    }

    return IDE_SUCCESS;
}

IDE_RC cmtCollectionReadAnyNext(UChar     *aCollectionData, /*   [IN]   */
                                UInt      *aCursor,         /* [IN/OUT] */
                                cmtAny    *aAny)            /*   [OUT]  */
{
    UInt   sCursor;
    UChar *sData;
    UChar  sInVariableDelimeter;

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
            idlOS::memcpy( aAny->mValue.mNumeric.mData,
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
        case CMT_ID_PTR:
            /* PROJ-1920 */
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, &aAny->mValue.mInVariable.mSize );
            CMT_COLLECTION_READ_PTR( sData, sCursor, &aAny->mValue.mInVariable.mData );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    *aCursor = sCursor;

    return IDE_SUCCESS;
}

IDE_RC cmtCollectionReadDataNext(UChar     *aCollectionData, /*   [IN]   */
                                 UInt      *aCursor,         /* [IN/OUT] */
                                 UChar     *aType,           /*   [OUT]   */
                                 UChar    **aBuffer,         /*   [OUT]  */
                                 UInt      *aParam1,         /*   [OUT]  */
                                 UInt      *aParam2,         /*   [OUT]  */
                                 ULong     *aParam3)         /*   [OUT]  */
{
    UInt         sCursor;
    UChar       *sData;
    UChar        sInVariableDelimeter;
    UChar        sType;
    UInt         sDataLength = 0;
    UInt         sPrecision = 0;
    cmtDateTime *sDateTime;
    cmtInterval *sInterval;
    cmtNumeric  *sNumeric;
    ULong        sLocatorID;

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
            sDataLength = ID_SIZEOF(SChar);
            break;
        case CMT_ID_UINT8:
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(UChar);
            break;
        case CMT_ID_SINT16:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(SShort);
            break;
        case CMT_ID_UINT16:
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(UShort);
            break;
        case CMT_ID_SINT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(SInt);
            break;
        case CMT_ID_UINT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(UInt);
            break;
        case CMT_ID_SINT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(SLong);
            break;
        case CMT_ID_UINT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(ULong);
            break;
        case CMT_ID_FLOAT32:
            CMT_COLLECTION_READ_BYTE4( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(SFloat);
            break;
        case CMT_ID_FLOAT64:
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, *aBuffer );
            sDataLength = ID_SIZEOF(SDouble);
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
            sDataLength = ID_SIZEOF(cmtDateTime);
            break;
        case CMT_ID_INTERVAL:
            sInterval = (cmtInterval*)*aBuffer;
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &sInterval->mSecond);
            CMT_COLLECTION_READ_BYTE8( sData, sCursor, &sInterval->mMicroSecond);
            sDataLength = ID_SIZEOF(cmtInterval);
            break;
        case CMT_ID_NUMERIC:
            sNumeric = (cmtNumeric*)*aBuffer;
            idlOS::memset(sNumeric, 0, ID_SIZEOF(cmtNumeric));
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sNumeric->mSize );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sNumeric->mPrecision );
            CMT_COLLECTION_READ_BYTE2( sData, sCursor, &sNumeric->mScale );
            CMT_COLLECTION_READ_BYTE1( sData, sCursor, &sNumeric->mSign );
            idlOS::memcpy( sNumeric->mData,
                           sData + sCursor,
                           sNumeric->mSize );
            sCursor += sNumeric->mSize;
            sDataLength = ID_SIZEOF(cmtNumeric);
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
            *aParam3 = sLocatorID;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    *aType   = sType;
    *aCursor = sCursor;
    *aParam1 = sDataLength;
    *aParam2 = sPrecision;


    return IDE_SUCCESS;
}

void cmtCollectionReadParamInfoSetNext(UChar  *aCollectionData,
                                       UInt   *aCursor,
                                       void   *aCollectionParamInfoSet)
{
    UInt                             sCursor;
    UChar                           *sData;  
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

void cmtCollectionWriteParamInfoSet(UChar  *aCollectionData,
                                    UInt   *aCursor,
                                    void   *aCollectionParamInfoSet)
{
    UInt                             sCursor;
    UChar                           *sData;
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

void cmtCollectionWriteColumnInfoGetResult(UChar  *aCollectionData,
                                           UInt   *aCursor,
                                           void   *aCollectionColumnInfoGetResult)
{
    UInt                                    sCursor;
    UChar                                  *sData;  
    cmpCollectionDBColumnInfoGetResultA5 *sCollectionColumnInfoGetResult;

    sCursor = *aCursor;
    sData   = aCollectionData;
    sCollectionColumnInfoGetResult = (cmpCollectionDBColumnInfoGetResultA5*)aCollectionColumnInfoGetResult;

    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mDataType);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mLanguage);
    CMT_COLLECTION_WRITE_BYTE1(sData, sCursor, sCollectionColumnInfoGetResult->mArguments);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mPrecision);
    CMT_COLLECTION_WRITE_BYTE4(sData, sCursor, sCollectionColumnInfoGetResult->mScale);
    CMT_COLLECTION_WRITE_BYTE1(sData, sCursor, sCollectionColumnInfoGetResult->mNullableFlag);

    cmtCollectionWriteAny( sData,
                           &sCursor,
                           &sCollectionColumnInfoGetResult->mDisplayName );

    *aCursor = sCursor;
}

UInt cmtCollectionAnySize(cmtAny *aAny)
{
    UInt sSize;

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
        case CMT_ID_PTR:
            /* PROJ-1920 */
            sSize += ID_SIZEOF(UInt) + ID_SIZEOF(vULong);
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return sSize;
}

void cmtCollectionWriteAny(UChar     *aCollectionData, /*   [IN]   */
                           UInt      *aCursor,         /* [IN/OUT] */
                           cmtAny    *aAny)
{
    UInt   sCursor;
    UChar *sData;
    UChar  sInVariableDelimeter = 0;

    /*
     * Cursor 위치 복사
     */
    sCursor = *aCursor;
    sData   = aCollectionData;

    /*
     * Type ID 범위 검사
     */
    IDE_DASSERT( aAny->mType != CMT_ID_VARIABLE );

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
        case CMT_ID_REDUNDANCY: // PROJ-2256
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
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mNumeric.mData,
                           aAny->mValue.mNumeric.mSize );
            sCursor += aAny->mValue.mNumeric.mSize;
            break;
        case CMT_ID_IN_VARIABLE:
        case CMT_ID_IN_BINARY:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInVariable.mSize );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mInVariable.mData,
                           aAny->mValue.mInVariable.mSize );
            sCursor += aAny->mValue.mInVariable.mSize;
            /* Make in-variable data null terminated string */
            CMT_COLLECTION_WRITE_BYTE1( sData, sCursor, sInVariableDelimeter );
            break;
        case CMT_ID_IN_BIT:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInBit.mData.mSize );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mInBit.mData.mData,
                           aAny->mValue.mInBit.mData.mSize );
            sCursor += aAny->mValue.mInBit.mData.mSize;
            break;
        case CMT_ID_IN_NIBBLE:
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mPrecision );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInNibble.mData.mSize );
            idlOS::memcpy( sData + sCursor,
                           aAny->mValue.mInNibble.mData.mData,
                           aAny->mValue.mInNibble.mData.mSize );
            sCursor += aAny->mValue.mInNibble.mData.mSize;
            break;
        case CMT_ID_LOBLOCATOR:
            CMT_COLLECTION_WRITE_BYTE8( sData, sCursor, aAny->mValue.mLobLocator.mLocator );
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mLobLocator.mSize );
            break;
        case CMT_ID_PTR:
            /* PROJ-1920 */
            CMT_COLLECTION_WRITE_BYTE4( sData, sCursor, aAny->mValue.mInVariable.mSize );
            CMT_COLLECTION_WRITE_PTR( sData, sCursor, aAny->mValue.mInVariable.mData );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    *aCursor = sCursor;
}

IDE_RC cmtCollectionWriteVariable(cmtCollection *aCollection,
                                  UChar         *aBuffer,
                                  UInt           aBufferSize)
{
    aCollection->mType = CMT_ID_VARIABLE;

    IDE_TEST(cmtVariableInitialize(&aCollection->mValue.mVariable) != IDE_SUCCESS);

    IDE_TEST(cmtVariableSetData(&aCollection->mValue.mVariable,
                                aBuffer,
                                aBufferSize) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmtCollectionWriteInVariable(cmtCollection *aCollection,
                                    UChar         *aBuffer,
                                    UInt           aBufferSize)
{
    aCollection->mType = CMT_ID_IN_VARIABLE;

    aCollection->mValue.mInVariable.mSize = aBufferSize;
    aCollection->mValue.mInVariable.mData = aBuffer;

    return IDE_SUCCESS;
}

