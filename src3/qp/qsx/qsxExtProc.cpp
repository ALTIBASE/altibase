/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: qsxExtProc.cpp 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#include <qsxExtProc.h>
#include <qsxUtil.h>

extern mtdModule mtdDouble;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;
// PROJ-2717 Internal Procedure
extern mtdModule mtdByte;
extern mtdModule mtdVarbyte;

void   
qsxExtProc::returnCharLength( void       * aValue,
                              SInt         aMaxLength,
                              UShort     * aOutLength )
{
    UChar              * sValueIndex;
    UChar              * sValueFence;

    // we cannot call strlen() because aValue may not have NULL character.
    // Then, strlen() may read more bytes of the string beyond its boundary.

    sValueIndex = (UChar*)aValue;
    sValueFence = sValueIndex + aMaxLength;

    while( sValueIndex < sValueFence )
    {
        if( *sValueIndex == '\0' )
        {
            break;
        }
        else
        {
            sValueIndex++;
        }
    }

    // return length
    *aOutLength = sValueIndex - (UChar*)aValue;
}
void
qsxExtProc::returnNcharLength( mtcColumn * aColumn,
                               void      * aValue,
                               SInt        aMaxLength,
                               UShort    * aOutLength )
{
    const mtlModule    * sLanguage;
    UChar              * sNextIndex;
    UChar              * sCurrIndex;
    UChar              * sValueFence;

    UChar                sCharSize      = 0;
    UShort               sStringSize    = 0;
    SInt                 i              = 0;
    idBool               sIsNull        = ID_FALSE;

    sLanguage = aColumn->language;

    sNextIndex = (UChar*)aValue;
    sValueFence = sNextIndex + aMaxLength;

    while ( sNextIndex < sValueFence )
    {
        sCurrIndex = sNextIndex;

        if( sLanguage->nextCharPtr( & sNextIndex, sValueFence ) == NC_VALID )
        {
            // calculate current character size, because it may be variable
            sCharSize = sNextIndex - sCurrIndex;

            /* check whether this charachter is null or not */
            for( i = 0; i < sCharSize; i++ )
            {
                if( *( sCurrIndex + i ) == '\0' )
                {
                    sIsNull = ID_TRUE;
                }
                else
                {
                    sIsNull = ID_FALSE;
                    break;
                }
            }


            if( sIsNull == ID_TRUE )
            {
                /* Here is the end of input string */
                break;
            }
            else
            {
                /* Add size of current character to the string size. */
                sStringSize += sCharSize;
            }
        }
        else
        {
            /* NC_INCOMPLETE, NC_MB_INVALID */
            break;
        }
    }

    /* return calculate size */
    *aOutLength = sStringSize;
}

void
qsxExtProc::convertDate2Timestamp( mtdDateType    * aDate,
                                   idxTimestamp   * aTimestamp )
{
    idxTimestamp      sTimestamp;

    sTimestamp.mYear       = mtdDateInterface::year(aDate);
    sTimestamp.mMonth      = mtdDateInterface::month(aDate);
    sTimestamp.mDay        = mtdDateInterface::day(aDate);
    sTimestamp.mHour       = mtdDateInterface::hour(aDate);
    sTimestamp.mMinute     = mtdDateInterface::minute(aDate);
    sTimestamp.mSecond     = mtdDateInterface::second(aDate);
    sTimestamp.mFraction   = mtdDateInterface::microSecond(aDate);

    *aTimestamp = sTimestamp;
}

IDE_RC
qsxExtProc::convertTimestamp2Date( idxTimestamp   * aTimestamp,
                                   mtdDateType    * aDate )
{
    IDE_TEST( mtdDateInterface::setYear( aDate, aTimestamp->mYear ) != IDE_SUCCESS );
    IDE_TEST( mtdDateInterface::setMonth( aDate, aTimestamp->mMonth ) != IDE_SUCCESS );
    IDE_TEST( mtdDateInterface::setDay( aDate, aTimestamp->mDay ) != IDE_SUCCESS );
    IDE_TEST( mtdDateInterface::setHour( aDate, aTimestamp->mHour ) != IDE_SUCCESS );
    IDE_TEST( mtdDateInterface::setMinute( aDate, aTimestamp->mMinute ) != IDE_SUCCESS );
    IDE_TEST( mtdDateInterface::setSecond( aDate, aTimestamp->mSecond ) != IDE_SUCCESS );
    IDE_TEST( mtdDateInterface::setMicroSecond( aDate, aTimestamp->mFraction ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxExtProc::fillTimestampParamValue( UInt             aDataTypeId,
                                     SChar          * aValue,
                                     idxTimestamp   * aTimestamp )
{
    mtdDateType       sDate;
    mtdIntervalType   sInterval;

    switch( aDataTypeId )
    {
        case MTD_DATE_ID:
            sDate = *(mtdDateType *)aValue;
            convertDate2Timestamp( &sDate, aTimestamp );
            break;
        case MTD_INTERVAL_ID:
            sInterval = *(mtdIntervalType *)aValue;
            IDE_TEST( mtdDateInterface::convertInterval2Date( &sInterval, &sDate )
                      != IDE_SUCCESS );
            convertDate2Timestamp( &sDate, aTimestamp );
            break;
        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idxParamType
qsxExtProc::getParamType( UInt aTypeId )
{
    idxParamType    sType = IDX_TYPE_NONE;

    switch( aTypeId )
    {
        case MTD_BOOLEAN_ID:
            sType = IDX_TYPE_BOOL;
            break;
        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
        case MTD_NCHAR_ID:
        case MTD_NVARCHAR_ID:
            sType = IDX_TYPE_CHAR;
            break;
        case MTD_SMALLINT_ID:
            sType = IDX_TYPE_SHORT;
            break;
        case MTD_INTEGER_ID:
            sType = IDX_TYPE_INT;
            break;
        case MTD_BIGINT_ID:
            sType = IDX_TYPE_INT64;
            break;
        case MTD_REAL_ID:
            sType = IDX_TYPE_FLOAT;
            break;
        case MTD_DOUBLE_ID:
            sType = IDX_TYPE_DOUBLE;
            break;
        case MTD_NUMBER_ID:
        case MTD_NUMERIC_ID:
        case MTD_FLOAT_ID:
            sType = IDX_TYPE_NUMERIC;
            break;
        case MTD_DATE_ID:
        case MTD_INTERVAL_ID:
            sType = IDX_TYPE_TIMESTAMP;
            break;
        case MTD_BLOB_ID:
        case MTD_CLOB_ID:
            // BUG-39814 IN mode LOB Parameter in Extproc
            sType = IDX_TYPE_LOB;
            break;
        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:
            // PROJ-2717 Internal Procedure
            sType = IDX_TYPE_BYTE;
            break;
        default:
            break;
    }

    return sType;
}

IDE_RC
qsxExtProc::fillParamAndPropValue( iduMemory    * aQxeMem,
                                   mtcColumn    * aColumn,
                                   SChar        * aRow,
                                   qcTemplate   * aTmplate,
                                   idxParamInfo * aParamInfo,
                                   qsProcType     aProcType )
{
    UInt      sPtrDataSize = 0;
    mtcColumn sColumn;          /* for IDX_TYPE_NUMERIC */
    SDouble   sDouble = 0.0;    /* for IDX_TYPE_NUMERIC */

    /*** 1. Property Value (ordinary parameter also has) */
    switch ( aParamInfo->mType )
    {
        case IDX_TYPE_CHAR:
        case IDX_TYPE_BYTE:  // PROJ-2717 Internal procedure
            aParamInfo->mIndicator  = aColumn->module->isNull( aColumn, aRow );
            aParamInfo->mLength     = aColumn->module->actualSize( aColumn, aRow ) - aColumn->module->headerSize();
            aParamInfo->mMaxLength  = aColumn->column.size - aColumn->module->headerSize();
            break;
        case IDX_TYPE_LOB:
            // BUG-39814 IN mode LOB Parameter in Extproc
            aParamInfo->mIndicator  = aColumn->module->isNull( aColumn, aRow );
            aParamInfo->mLength     = aColumn->module->actualSize( aColumn, aRow ) - aColumn->module->headerSize();
            aParamInfo->mMaxLength  = aParamInfo->mLength; // IN mode (Otherwise, mMaxLength is the same as CHAR's mMaxLength)
            break;
        case IDX_TYPE_TIMESTAMP:
            aParamInfo->mIndicator  = aColumn->module->isNull( aColumn, aRow );
            aParamInfo->mLength     = ID_SIZEOF(idxTimestamp);
            aParamInfo->mMaxLength  = aParamInfo->mLength;
            break;
        case IDX_TYPE_NUMERIC:
            IDE_TEST( mtc::initializeColumn( &sColumn, &mtdDouble, 0, 0, 0 ) != IDE_SUCCESS );

            aParamInfo->mIndicator  = aColumn->module->isNull( aColumn, aRow );
            aParamInfo->mLength     = sColumn.column.size;  // conversion to double
            aParamInfo->mMaxLength  = aParamInfo->mLength;
            break;
        default:
            aParamInfo->mIndicator  = aColumn->module->isNull( aColumn, aRow );
            aParamInfo->mLength     = aColumn->module->actualSize( aColumn, aRow ) - aColumn->module->headerSize();
            aParamInfo->mMaxLength  = aParamInfo->mLength;
            break;
    }

    /*** 2. Real Value */
    if( aParamInfo->mPropType == IDX_TYPE_PROP_NONE )
    {
        switch( aParamInfo->mType )
        {
            case IDX_TYPE_CHAR:
            {
                sPtrDataSize = aParamInfo->mMaxLength;

                switch ( aParamInfo->mMode )
                {
                    case IDX_MODE_IN:
                        // use the original value, or set NULL
                        if( aParamInfo->mIndicator == ID_FALSE )
                        {
                            aParamInfo->mD.mPointer = ((mtdCharType *)aRow)->value;
                        }
                        else
                        {
                            // 1 byte for padding
                            sPtrDataSize = 1;

                            if ( aProcType == QS_EXTERNAL_C )
                            {
                                aParamInfo->mD.mPointer = NULL;
                            }
                            else if ( aProcType == QS_INTERNAL_C )
                            {
                                aParamInfo->mD.mPointer = &(aParamInfo->mD.mPointer);
                            }
                            else
                            {
                                IDE_ERROR(0);
                            }
                        }

                        break;
                    case IDX_MODE_INOUT:
                        // Paste if the input value exists, or use alloc area
                        IDE_TEST( aQxeMem->cralloc( idlOS::align8( aParamInfo->mMaxLength + 1 ),
                                                    (void **)&aParamInfo->mD.mPointer )
                                  != IDE_SUCCESS );

                        if( aParamInfo->mIndicator == ID_FALSE )
                        {
                            idlOS::memcpy( aParamInfo->mD.mPointer,
                                           ((mtdCharType *)aRow)->value,
                                           aParamInfo->mMaxLength );

                            ((SChar*)aParamInfo->mD.mPointer)[aParamInfo->mMaxLength] = '\0';
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        break;
                    case IDX_MODE_OUT:
                        // use alloc area
                        IDE_TEST( aQxeMem->cralloc( idlOS::align8( aParamInfo->mMaxLength + 1 ),
                                                    (void **)&aParamInfo->mD.mPointer )
                                  != IDE_SUCCESS );

                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }

                break;
            }
            case IDX_TYPE_BYTE:
            {
                // PROJ-2717 Internal Procedure
                sPtrDataSize = aParamInfo->mMaxLength;

                switch ( aParamInfo->mMode )
                {
                    case IDX_MODE_IN:
                        // use the original value, or set NULL
                        if( aParamInfo->mIndicator == ID_FALSE )
                        {
                            aParamInfo->mD.mPointer = ((mtdByteType*)aRow)->value;
                        }
                        else
                        {
                            // 1 byte for padding
                            sPtrDataSize = 1;
                        
                            if ( aProcType == QS_EXTERNAL_C )
                            {
                                aParamInfo->mD.mPointer = NULL;
                            }
                            else if ( aProcType == QS_INTERNAL_C )
                            {
                                aParamInfo->mD.mPointer = &(aParamInfo->mD.mPointer);
                            }
                            else
                            {
                                IDE_ERROR(0);
                            }
                        }

                        break;
                    case IDX_MODE_INOUT:
                    case IDX_MODE_OUT:
                        // If you want to add INOUT/OUT Mode BYTE Parameter,
                        // please consider IDX_TYPE_CHAR code above
                        IDE_DASSERT(0);
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }

                break;
            }
            case IDX_TYPE_LOB:
            {
                // BUG-39814 IN mode LOB Parameter in Extproc
                sPtrDataSize = aParamInfo->mMaxLength;

                switch ( aParamInfo->mMode )
                {
                    case IDX_MODE_IN:
                        // BUG-40195
                        // use the original value, or set NULL
                        if( aParamInfo->mIndicator == ID_FALSE )
                        {
                            aParamInfo->mD.mPointer = ((mtdLobType *)aRow)->value;
                        }
                        else
                        {
                            // 1 byte for padding
                            sPtrDataSize = 1;

                            if ( aProcType == QS_EXTERNAL_C )
                            {
                                aParamInfo->mD.mPointer = NULL;
                            }
                            else if ( aProcType == QS_INTERNAL_C )
                            {
                                aParamInfo->mD.mPointer = &(aParamInfo->mD.mPointer);
                            }
                            else
                            {
                                IDE_ERROR(0);
                            }
                        }

                        break;
                    case IDX_MODE_INOUT:
                    case IDX_MODE_OUT:
                        // If you want to add INOUT/OUT Mode LOB Parameter,
                        // please consider IDX_TYPE_CHAR code above
                        IDE_DASSERT(0);
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }

                break;
            }
            case IDX_TYPE_BOOL:
            {
                /* BUG 36962 TRUE/FALSE reverse */
                if( *(mtdBooleanType *)aRow == MTD_BOOLEAN_TRUE )
                {
                    aParamInfo->mD.mBool = IDX_BOOLEAN_TRUE;
                }
                else if( *(mtdBooleanType *)aRow == MTD_BOOLEAN_FALSE )
                {
                    aParamInfo->mD.mBool = IDX_BOOLEAN_FALSE;
                }
                else
                {
                    /* aParamInfo->mD.mBool == MTD_BOOLEAN_NULL */
                    // Nothing to do.
                }

                break;
            }
            case IDX_TYPE_SHORT:
                aParamInfo->mD.mShort  = (SShort)*(mtdSmallintType *)aRow;
                break;
            case IDX_TYPE_INT:
                aParamInfo->mD.mInt    = (SInt)*(mtdIntegerType *)aRow;
                break;
            case IDX_TYPE_INT64:
                aParamInfo->mD.mLong   = (SLong)*(mtdBigintType *)aRow;
                break;
            case IDX_TYPE_FLOAT:
                aParamInfo->mD.mFloat  = (SFloat)*(mtdRealType *)aRow;
                break;
            case IDX_TYPE_DOUBLE:
                aParamInfo->mD.mDouble = (SDouble)*(mtdDoubleType *)aRow;
                break;
            case IDX_TYPE_NUMERIC:
            {
                IDE_TEST( qsxUtil::assignValue( aQxeMem,
                                                aColumn,
                                                aRow,
                                                &sColumn,
                                                &sDouble,
                                                &aTmplate->tmplate,
                                                ID_FALSE ) != IDE_SUCCESS );
                aParamInfo->mD.mDouble = sDouble;
                break;
            }
            case IDX_TYPE_TIMESTAMP:
                IDE_TEST( fillTimestampParamValue( aColumn->type.dataTypeId,
                                                   aRow,
                                                   &aParamInfo->mD.mTimestamp )
                          != IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    else
    {
        /* Nothing to do. */
    }

    aParamInfo->mSize = idlOS::align8( ID_SIZEOF(idxParamInfo) ) + sPtrDataSize + 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxExtProc::fillParamInfo( iduMemory        * aQxeMem,
                           qsCallSpecParam  * aParam,
                           qcTemplate       * aTmplate,
                           idxParamInfo     * aParamInfo,
                           UInt               aOrder,
                           qsProcType         aProcType )
{
    SChar               sPropertyName[IDX_PROPERTY_NAME_MAXLEN]; 
    mtcTuple          * sTuple;
    mtcColumn         * sColumn;
    SChar             * sRow;

    /* 0. Initialize */
    QSX_EXTPROC_INIT_PARAM_INFO( aParamInfo );

    /*** 1. Column + Table index */
    aParamInfo->mColumn = aParam->column;
    aParamInfo->mTable  = aParam->table;

    sTuple  = &aTmplate->tmplate.rows[aParamInfo->mTable];
    sColumn = &sTuple->columns[aParamInfo->mColumn];
    sRow    = (SChar*)sTuple->row + sColumn->column.offset;

    /*** 2. Order + Mode */
    aParamInfo->mOrder = aOrder;

    switch( aParam->inOutType )
    {
        case QS_IN:
            aParamInfo->mMode = IDX_MODE_IN;
            break;
        case QS_OUT:
            aParamInfo->mMode = IDX_MODE_OUT;
            break;
        case QS_INOUT:
            aParamInfo->mMode = IDX_MODE_INOUT;
            break;
        default:
            break;
    }

    /*** 3. Type ID (Property Parameter 도 마찬가지로 소유) */
    aParamInfo->mType = getParamType( sColumn->type.dataTypeId );

    /*** 4. Property Type */
    aParamInfo->mPropType = IDX_TYPE_PROP_NONE;

    if( QC_IS_NULL_NAME( aParam->paramPropertyPos ) != ID_TRUE )
    {
        QC_STR_COPY( sPropertyName, aParam->paramPropertyPos );

        if ( idlOS::strMatch( sPropertyName,
                              idlOS::strlen( sPropertyName ),
                              "INDICATOR",
                              9 ) == 0 )
        {
            aParamInfo->mPropType = IDX_TYPE_PROP_IND;
        }
        else if ( idlOS::strMatch( sPropertyName,
                                   idlOS::strlen( sPropertyName ),
                                   "LENGTH",
                                   6 ) == 0 )
        {
            aParamInfo->mPropType = IDX_TYPE_PROP_LEN;
        }
        else if ( idlOS::strMatch( sPropertyName,
                                   idlOS::strlen( sPropertyName ),
                                   "MAXLEN",
                                   6 ) == 0 )
        {
            aParamInfo->mPropType = IDX_TYPE_PROP_MAX;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    /*** 5. 포인터 여부 */
    if ( ( aParamInfo->mMode == IDX_MODE_OUT )   ||
         ( aParamInfo->mMode == IDX_MODE_INOUT ) ||
         ( aParamInfo->mType == IDX_TYPE_CHAR )  ||
         ( aParamInfo->mType == IDX_TYPE_BYTE )  || // PROJ-2717 Internal Procedure
         ( aParamInfo->mType == IDX_TYPE_LOB ) )    // BUG-39814 IN mode LOB Parameter in EXTPROC
    {
        aParamInfo->mIsPtr = ID_TRUE;
    }
    else
    {
        aParamInfo->mIsPtr = ID_FALSE;
    }

    /*** 6. Property Value (일반 Parameter 도 마찬가지로 소유) */
    IDE_TEST( fillParamAndPropValue( aQxeMem,
                                     sColumn,
                                     sRow,
                                     aTmplate,
                                     aParamInfo,
                                     aProcType ) != IDE_SUCCESS );

    /*** 7. Align its size ***/
    aParamInfo->mSize = idlOS::align8( aParamInfo->mSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxExtProc::fillReturnInfo( iduMemory        * aQxeMem,
                            qsVariableItems    aRetItem,
                            qcTemplate       * aTmplate,
                            idxParamInfo     * aParamInfo,
                            qsProcType         aProcType )
{
    /* NOTE : RETURN argument doesn't have its property value. */

    mtcTuple          * sTuple;
    mtcColumn         * sColumn;
    SChar             * sRow;

    /*** 1. Column + Table index */
    aParamInfo->mColumn = aRetItem.column;
    aParamInfo->mTable  = aRetItem.table;

    sTuple  = &aTmplate->tmplate.rows[aRetItem.table];
    sColumn = &sTuple->columns[aRetItem.column];
    sRow    = (SChar*)sTuple->row + sColumn->column.offset;

    /*** 2. Order + Mode */
    aParamInfo->mOrder = IDX_RETURN_ORDER;
    aParamInfo->mMode  = IDX_MODE_OUT;

    /*** 3. Type ID (Property Parameter also has) */
    aParamInfo->mType     = getParamType( sColumn->type.dataTypeId );
    aParamInfo->mPropType = IDX_TYPE_PROP_NONE;

    /*** 5. Pointer? */
    aParamInfo->mIsPtr = ID_TRUE;

    /*** 6. Properties & Value */
    IDE_TEST( fillParamAndPropValue( aQxeMem,
                                     sColumn,
                                     sRow,
                                     aTmplate,
                                     aParamInfo,
                                     aProcType ) != IDE_SUCCESS );

    /*** 7. Shorten its size. We only need actual size. */
    aParamInfo->mSize -= idlOS::align8( ID_SIZEOF(idxParamInfo) );
    aParamInfo->mSize = idlOS::align8( aParamInfo->mSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC
qsxExtProc::returnParamValue( iduMemory        * aQxeMem,
                              idxParamInfo     * aParamInfo,
                              qcTemplate       * aTmplate )
{
    mtcTuple          * sTuple;
    mtcColumn         * sColumn;
    mtcColumn           sSrcColumn;
    SChar             * sRow;
    SChar             * sSrcRow;
    UShort              sByteLength;

    idxTimestamp        sTime;
    mtdDateType         sDate;
    mtdIntervalType     sInterval;

    mtdCharType       * sCharData;

    sTuple  = &aTmplate->tmplate.rows[aParamInfo->mTable];
    sColumn = &sTuple->columns[aParamInfo->mColumn];
    sRow    = (SChar*)sTuple->row + sColumn->column.offset;
    /* BUG-41818 변수 초기화 ( valgrind ) */
    IDX_INIT_TIMESTAMP( sTime );
    sDate = mtdDateNull;
    sInterval = mtdIntervalNull;

    if( aParamInfo->mPropType == IDX_TYPE_PROP_NONE )
    {
        switch( sColumn->type.dataTypeId )
        {
            case MTD_BOOLEAN_ID:
            {
                /* BUG 36962 TRUE/FALSE reverse */
                if( aParamInfo->mD.mBool == IDX_BOOLEAN_TRUE )
                {
                    *(mtdBooleanType *)sRow = MTD_BOOLEAN_TRUE;
                }
                else if( aParamInfo->mD.mBool == IDX_BOOLEAN_FALSE )
                {
                    *(mtdBooleanType *)sRow = MTD_BOOLEAN_FALSE;
                }
                else
                {
                    /* aParamInfo->mD.mBool == MTD_BOOLEAN_NULL */
                    // Nothing to do even if user inserted abnormal value.
                    // it's his or her fault!
                }
                break;
            }
            case MTD_SMALLINT_ID:
                *(mtdSmallintType *)sRow = (mtdSmallintType)aParamInfo->mD.mShort;
                break;
            case MTD_INTEGER_ID:
                *(mtdIntegerType *)sRow = (mtdIntegerType)aParamInfo->mD.mInt;
                break;
            case MTD_BIGINT_ID:
                *(mtdBigintType *)sRow = (mtdBigintType)aParamInfo->mD.mLong;
                break;
            case MTD_REAL_ID:
                *(mtdRealType *)sRow = (mtdRealType)aParamInfo->mD.mFloat;
                break;
            case MTD_DOUBLE_ID:
                *(mtdDoubleType *)sRow = (mtdDoubleType)aParamInfo->mD.mDouble;
                break;
            case MTD_NUMBER_ID:
            case MTD_NUMERIC_ID:
            case MTD_FLOAT_ID:
            {
                IDE_TEST( mtc::initializeColumn( &sSrcColumn, &mtdDouble, 0, 0, 0 ) != IDE_SUCCESS );
                IDE_TEST( qsxUtil::assignValue( aQxeMem,
                                                &sSrcColumn,
                                                &aParamInfo->mD.mDouble,
                                                sColumn,
                                                sRow,
                                                &aTmplate->tmplate,
                                                ID_FALSE ) != IDE_SUCCESS );
                break;
            }
            case MTD_CHAR_ID:
            case MTD_VARCHAR_ID:
            {
                returnCharLength( aParamInfo->mD.mPointer,
                                  aParamInfo->mMaxLength,
                                  &sByteLength );

                sCharData = (mtdCharType*)sRow;
                sCharData->length = sByteLength;

                idlOS::memcpy( sCharData->value,
                               aParamInfo->mD.mPointer,
                               aParamInfo->mMaxLength );

                /* assign temporary 'value' buffer */
                IDE_TEST( aQxeMem->alloc( aParamInfo->mMaxLength + ID_SIZEOF(sCharData->length),
                                          (void **)&sSrcRow )
                          != IDE_SUCCESS );

                idlOS::memcpy( sSrcRow, 
                               sCharData,
                               aParamInfo->mMaxLength + ID_SIZEOF(sCharData->length) );

                if( sColumn->type.dataTypeId == MTD_CHAR_ID )
                {
                    IDE_TEST( mtdChar.canonize( sColumn,
                                                (void**)&sRow,
                                                NULL,
                                                NULL,
                                                (void*)sSrcRow,
                                                NULL,
                                                NULL ) != IDE_SUCCESS );
                }
                else /* MTD_VARCHAR_ID */ 
                {
                    IDE_TEST( mtdVarchar.canonize( sColumn,
                                                   (void**)&sRow,
                                                   NULL,
                                                   NULL,
                                                   (void*)sSrcRow,
                                                   NULL,
                                                   NULL ) != IDE_SUCCESS );
                }

                break;
            }
            case MTD_NCHAR_ID:
            case MTD_NVARCHAR_ID:
            {
                returnNcharLength( sColumn,
                                   aParamInfo->mD.mPointer,
                                   aParamInfo->mMaxLength,
                                   &sByteLength );

                sCharData = (mtdCharType*)sRow;
                sCharData->length = sByteLength;

                idlOS::memcpy( sCharData->value,
                               aParamInfo->mD.mPointer,
                               aParamInfo->mMaxLength );

                if( sColumn->type.dataTypeId == MTD_NCHAR_ID )
                {
                    /* assign temporary 'value' buffer */
                    IDE_TEST( aQxeMem->alloc( aParamInfo->mMaxLength + ID_SIZEOF(sCharData->length),
                                              (void **)&sSrcRow )
                              != IDE_SUCCESS );

                    idlOS::memcpy( sSrcRow, 
                                   sCharData,
                                   aParamInfo->mMaxLength + ID_SIZEOF(sCharData->length) );

                    IDE_TEST( mtdNchar.canonize( sColumn,
                                                 (void**)&sRow,
                                                 NULL,
                                                 NULL,
                                                 (void*)sSrcRow,
                                                 NULL,
                                                 NULL ) != IDE_SUCCESS );
                }
                else /* MTD_NVARCHAR_ID */
                {
                    idlOS::memset( sCharData->value + sByteLength,
                                   0,
                                   aParamInfo->mMaxLength - sByteLength ); 
                }

                break;
            }
            case MTD_DATE_ID:
                sTime = aParamInfo->mD.mTimestamp;
                IDE_TEST( convertTimestamp2Date( &sTime, &sDate ) != IDE_SUCCESS );
                *(mtdDateType *)sRow = sDate;
                break;
            case MTD_INTERVAL_ID:
                sTime = aParamInfo->mD.mTimestamp;
                IDE_TEST( convertTimestamp2Date( &sTime, &sDate ) != IDE_SUCCESS );
                IDE_TEST( mtdDateInterface::convertDate2Interval( &sDate, &sInterval )
                                                  != IDE_SUCCESS );
                *(mtdIntervalType *)sRow = sInterval;
                break;
            default:
                break;
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qsxExtProc::returnParamProperty( idxParamInfo     * aParamInfo,
                                 qcTemplate       * aTmplate )
{
    mtcTuple          * sTuple;
    mtcColumn         * sColumn;
    SChar             * sRow;

    mtdCharType       * sCharData;

    sTuple  = &aTmplate->tmplate.rows[aParamInfo->mTable];
    sColumn = &sTuple->columns[aParamInfo->mColumn];
    sRow    = (SChar*)sTuple->row + sColumn->column.offset;

    /* Property를 나중에 적용한다. */
    switch( aParamInfo->mPropType )
    {
        case IDX_TYPE_PROP_IND:
        {
            if( aParamInfo->mIndicator == ID_TRUE )
            {
                /* 아까 넣었건 넣지 않았건 간에 null 로 만든다. */
                sColumn->module->null( sColumn, sRow );
            }
            else
            {
                /* Nothing to do.
                 * IN 모드에서 indicator 조작으로 인한 예외처리는 agent에서 이미 했다. */
            }
            break;
        }
        case IDX_TYPE_PROP_LEN:
        {
            /* CHAR type에서만 변동 */
            if(  aParamInfo->mType == IDX_TYPE_CHAR )
            {
                sCharData = (mtdCharType*)sRow;
                sCharData->length = aParamInfo->mLength;
            }
            else
            {
                /* Nothing to do.
                 * CHAR Type이 아닌 경우 length 비정상 조작으로 인한 예외처리는 agent에서 이미 했다. */
            }
            break;
        }
        case IDX_TYPE_PROP_MAX:
            /* assertion. OUT 모드인 MAXLEN parameter는 존재하지 않는다. */
            IDE_DASSERT( ID_FALSE );
            break;
        default:
            /* property parameter가 아니다. */
            break;
    }
}

IDE_RC
qsxExtProc::returnAllParams( iduMemory     * aQxeMem,
                             idxExtProcMsg * aMsg,
                             qcTemplate    * aTmplate )
{
    UInt i = 0;

    /* Value */
    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        if( aMsg->mParamInfos[i].mMode != IDX_MODE_IN )
        {
            IDE_TEST( returnParamValue( aQxeMem,
                                        &aMsg->mParamInfos[i],
                                        aTmplate ) != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    /* Return Value */
    if( aMsg->mReturnInfo.mSize > 0 )
    {
        IDE_TEST( returnParamValue( aQxeMem,
                                    &aMsg->mReturnInfo,
                                    aTmplate ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    /* Property (including return's property) */
    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        if( aMsg->mParamInfos[i].mMode != IDX_MODE_IN 
            && aMsg->mParamInfos[i].mPropType != 0 )
        {
            returnParamProperty( &aMsg->mParamInfos[i], aTmplate );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC
qsxExtProc::returnAllParams4IntProc( iduMemory     * aQxeMem,
                                     idxIntProcMsg * aMsg,
                                     qcTemplate    * aTmplate )
{
    UInt i = 0;

    /* Value */
    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        if( aMsg->mParamInfos[i].mMode != IDX_MODE_IN )
        {
            IDE_TEST( returnParamValue( aQxeMem,
                                        &aMsg->mParamInfos[i],
                                        aTmplate ) != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    /* Return Value */
    if( aMsg->mReturnInfo.mSize > 0 )
    {
        IDE_TEST( returnParamValue( aQxeMem,
                                    &aMsg->mReturnInfo,
                                    aTmplate ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    /* Property (including return's property) */
    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        if( ((aMsg->mParamInfos[i].mMode) != IDX_MODE_IN) &&
            ((aMsg->mParamInfos[i].mPropType) != 0) )
        {
            returnParamProperty( &aMsg->mParamInfos[i], aTmplate );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
