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
 

#include <idl.h>
#include <ideLog.h>

#include <rpdConvertSQL.h>
#include <rpdCatalog.h>

#define UTF16_CHARSET_STR   "UTF16"

void rpdConvertSQL::getColumnValueCondition( UInt        aTargetDataTypeID,
                                             UInt        aSourceDataTypeID,
                                             idBool    * aNeedSingleQuotation,
                                             idBool    * aNeedRPad,
                                             idBool    * aNeedRTrim,
                                             idBool    * aNeedColumnType,
                                             idBool    * aIsUTF16 )
{
    *aNeedSingleQuotation = ID_FALSE;
    *aNeedRPad = ID_FALSE;
    *aNeedRTrim = ID_FALSE;
    *aNeedColumnType = ID_FALSE;
    *aIsUTF16 = ID_FALSE;

    switch ( aTargetDataTypeID )
    {
        case MTD_CHAR_ID:
        case MTD_NCHAR_ID:

            *aNeedSingleQuotation = ID_TRUE;
            *aNeedRPad = ID_TRUE;
            break;

        case MTD_VARCHAR_ID:
        case MTD_NVARCHAR_ID:

            *aNeedSingleQuotation = ID_TRUE;
            break;

        case MTD_BIT_ID:
        case MTD_VARBIT_ID:
        case MTD_NIBBLE_ID:
        case MTD_BYTE_ID:
        case MTD_VARBYTE_ID:

            *aNeedColumnType = ID_TRUE;
            *aNeedSingleQuotation = ID_TRUE;
            break;

        default:
            break;
    }

    switch ( aSourceDataTypeID )
    {
        case MTD_CHAR_ID:

            *aNeedRTrim = ID_TRUE;
            break;

        case MTD_NCHAR_ID:

            *aIsUTF16 = ID_TRUE;
            *aNeedRTrim = ID_TRUE;
            *aNeedSingleQuotation = ID_TRUE;
            break;

        case MTD_NVARCHAR_ID:

            *aIsUTF16 = ID_TRUE;
            *aNeedSingleQuotation = ID_TRUE;
            break;

        default:
            break;
    }
}

void rpdConvertSQL::addSingleQuotation( SChar    * aValue,
                                        UInt       aValueLength )
{
    idlOS::snprintf( aValue,
                     aValueLength,
                     "\'" );
}

void rpdConvertSQL::rTrim( idBool       aIsUTF16,
                           SChar      * aString )
{
    SInt        sOffset = 0;
    SInt        sValueLength = 0;

    sValueLength = idlOS::strlen( aString );

    if ( aIsUTF16 == ID_FALSE )
    {
        sOffset = sValueLength - 1;

        while ( ( aString[sOffset] == ' ' ) &&
                ( sOffset >= 0 ) )
        {
            aString[sOffset] = '\0';
            sOffset--;
        }
    }
    else
    {
        sOffset = sValueLength - 1;

        while ( sOffset >= 4 )
        {
            if ( ( aString[sOffset - 4] == '\\' ) &&
                 ( aString[sOffset - 3] == '0' ) &&  
                 ( aString[sOffset - 2] == '0' ) && 
                 ( aString[sOffset - 1] == '2' ) && 
                 ( aString[sOffset] == '0' ) )
            {
                aString[sOffset - 4] = '\0';
                aString[sOffset - 3] = '\0';
                aString[sOffset - 2] = '\0';
                aString[sOffset - 1] = '\0';
                aString[sOffset] = '\0';
            }
            else
            {
                break;
            }

            sOffset -= 5;
        }
    }
}

IDE_RC rpdConvertSQL::rPadForUTF16String( UInt       aColumnLength,
                                          SChar    * aString,
                                          UInt       aStringLength )
{
    UInt        sOffset = 0;
    UInt        sFence = 0;

    sOffset = idlOS::strlen( aString );
    sFence = aColumnLength * 5;

    IDE_TEST_RAISE( sFence > aStringLength - 1, ERR_NOT_ENOUGH_SPACE );

    while ( sOffset < sFence )
    {
        aString[sOffset] = '\\';
        aString[sOffset + 1] = '0';
        aString[sOffset + 2] = '0';
        aString[sOffset + 3] = '2';
        aString[sOffset + 4] = '0';

        sOffset += 5;
    }

    aString[sOffset] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
                        
IDE_RC rpdConvertSQL::convertNumericType( mtcColumn    * aColumn,
                                          smiValue     * aSmiValue,
                                          SChar        * aValue,
                                          UInt           aValueLength )
{
    const mtdModule     * sModule = NULL;
    IDE_RC                sReturnCode = IDE_FAILURE;
    UInt                  sLength = aValueLength;

    IDE_TEST_RAISE( aValueLength < 49, ERR_NOT_ENOUGH_SPACE );

    IDE_TEST( mtd::moduleById( &sModule, 
                               aColumn->type.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( sModule->encode( aColumn,
                               (void *)aSmiValue->value,
                               aSmiValue->length,
                               NULL,
                               0,
                               (UChar *)aValue,
                               &sLength,   
                               &sReturnCode )
              != IDE_SUCCESS );
    IDU_FIT_POINT_RAISE( "rpdConvertSQL::convertNumericType::sRetrunCode",
                         ERR_ENCODE_RESULT_FAIL );
    IDE_TEST_RAISE( sReturnCode != IDE_SUCCESS, ERR_ENCODE_RESULT_FAIL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION( ERR_ENCODE_RESULT_FAIL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "NumericType encode fail" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::convertCharType( mtcColumn   * aColumn,
                                       smiValue    * aSmiValue,
                                       SChar       * aValue,
                                       UInt          aValueLength )
{
    const mtdModule     * sModule = NULL;
    IDE_RC                sReturnCode = IDE_FAILURE;
    UInt                  sLength = aValueLength;

    IDE_TEST_RAISE( aValueLength < aSmiValue->length + 1, ERR_NOT_ENOUGH_SPACE );

    IDE_TEST( mtd::moduleById( &sModule, 
                               aColumn->type.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( sModule->encode( aColumn,
                               (void *)aSmiValue->value,
                               aSmiValue->length,
                               NULL,
                               0,
                               (UChar *)aValue,
                               &sLength,
                               &sReturnCode )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sReturnCode != IDE_SUCCESS, ERR_ENCODE_RESULT_FAIL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION( ERR_ENCODE_RESULT_FAIL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "CharType encode fail" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::convertDateType( mtcColumn    * aColumn,
                                       smiValue     * aSmiValue,
                                       SChar        * aValue,
                                       UInt           aValueLength )
{
    const mtdModule   * sModule = NULL;
    SChar               sTempValue[IDA_VARCHAR_MAX] = { 0, };
    UInt                sTempValueLength = IDA_VARCHAR_MAX;
    SChar             * sDateValue = NULL;
    SChar             * sMicrosecondValue = NULL;
    SChar             * sEndPtr = NULL;
    IDE_RC              sReturnCode = IDE_FAILURE;

    IDE_TEST( mtd::moduleById( &sModule, 
                               aColumn->type.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( sModule->encode( aColumn,
                               (void *)aSmiValue->value,
                               aSmiValue->length,
                               (UChar *)RP_DEFAULT_DATE_FORMAT,
                               RP_DEFAULT_DATE_FORMAT_LEN,   // BUG-18936
                               (UChar *)sTempValue,
                               &sTempValueLength,
                               &sReturnCode )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sReturnCode != IDE_SUCCESS, ERR_ENCODE_RESULT_FAIL );

    sDateValue = idlOS::strtok_r( sTempValue, ".", &sEndPtr );
    sMicrosecondValue = idlOS::strtok_r( sEndPtr, ".", &sEndPtr );
    idlOS::snprintf( aValue,
                     aValueLength,
                     "TO_DATE(\'%s%s\', \'YYYY-MM-DD HH:MI:SSSSSSSS\')",
                     sDateValue,
                     sMicrosecondValue );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ENCODE_RESULT_FAIL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DateType encode fail" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::convertBitType( smiValue     * aSmiValue,
                                      UChar        * aValue,
                                      UInt           aValueLength ) 
{
    mtdBitType    * sBit = NULL;
    UInt            sLength = 0;
    SChar           sTargetChar = ' ';
    UInt            sBitIndex = 0;
    UInt            sBitPosition = 0;
    UInt            sFence = 0;
    UInt            i = 0;

    sBit = (mtdBitType *)aSmiValue->value;
    sLength = (UInt)sBit->length;

    IDE_TEST_RAISE( aValueLength < (sLength + 1), ERR_NOT_ENOUGH_SPACE ); 

    sFence = ( sLength + 7 ) / 8;

    for ( i = 0; i < sFence; i++ )
    {
        for ( sBitIndex = 0; sBitIndex < 8; sBitIndex++ )
        {
            sBitPosition = (i * 8) + sBitIndex;
            if ( sBitPosition == sLength )
            {
                break;
            }
            else
            {
                sTargetChar = (SChar)( ( sBit->value[i] << sBitIndex >> 7 ) & 0x01 );
                aValue[sBitPosition] = sTargetChar + '0';
            }
        }
    }

    aValue[sLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::convertByteType( smiValue    * aSmiValue,
                                       SChar       * aValue,
                                       UInt          aValueLength )
{
    mtdByteType     * sByte = NULL;
    UInt              sLength = 0;
    UInt              sFence = 0;
    UInt              i = 0;
    UInt              sOffset = 0;
    SChar             sTargetChar = ' ';

    sByte = (mtdByteType *)aSmiValue->value;
    sLength = (UInt)(sByte->length * 2);

    IDE_TEST_RAISE( aValueLength < (sLength + 1), ERR_NOT_ENOUGH_SPACE );

    sFence = (UInt)sByte->length;

    for ( i = 0; i < sFence; i++ )
    {
        sOffset = i * 2;

        sTargetChar = (SChar)( ( sByte->value[i] & 0xF0 ) >> 4 );
        aValue[sOffset] = rpdConvertSQL::hexToString( sTargetChar );

        sTargetChar = (SChar)( sByte->value[i] & 0x0F );
        aValue[sOffset + 1] = rpdConvertSQL::hexToString( sTargetChar );
    }
    aValue[sLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::convertNibbleType( smiValue      * aSmiValue,
                                         SChar         * aValue,
                                         UInt            aValueLength )
{
    mtdNibbleType       * sNibble = NULL;
    UInt                  sLength = 0;
    UInt                  i = 0;
    UInt                  sFence = 0;
    UInt                  sOffset = 0;
    SChar                 sTargetChar = ' ';

    sNibble = (mtdNibbleType *)aSmiValue->value;

    sLength = sNibble->length;

    IDE_TEST_RAISE( aValueLength < (sLength + 1), ERR_NOT_ENOUGH_SPACE );

    sFence = ( sLength + 1 ) / 2;

    for ( i = 0 ; i < sFence; i++ )
    {
        sOffset = i * 2;

        sTargetChar = (SChar)((sNibble->value[i] & 0xF0 ) >> 4 );
        aValue[sOffset] = rpdConvertSQL::hexToString( sTargetChar );

        sTargetChar = (SChar)(sNibble->value[i] & 0x0F );
        aValue[sOffset + 1] = rpdConvertSQL::hexToString( sTargetChar );
    }
    aValue[sLength] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::charSetConvert( const mtlModule  * aSrcCharSet,
                                      const mtlModule  * aDestCharSet,
                                      mtdNcharType     * aSource,
                                      UChar            * aResult,
                                      UInt             * aResultLen )
{
    idnCharSetList   sIdnDestCharSet;
    idnCharSetList   sIdnSrcCharSet;
    UChar          * sSourceIndex = NULL;
    UChar          * sTempIndex = NULL;
    UChar          * sSourceFence = NULL;
    UChar          * sResultValue = NULL;
    UChar          * sResultFence = NULL;
    SInt             sSrcRemain = 0;
    SInt             sDestRemain = 0;
    SInt             sTempRemain = 0;

    sDestRemain = aDestCharSet->maxPrecision( aSource->length );

    IDE_TEST_RAISE( *aResultLen < (UInt)sDestRemain, ERR_NOT_ENOUGH_SPACE );

    sSourceIndex = aSource->value;
    sSrcRemain   = aSource->length;
    sSourceFence = sSourceIndex + aSource->length;

    sResultValue = aResult;
    sResultFence = sResultValue + sDestRemain;

    if ( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        while ( sSrcRemain > 0 )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence, ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;

            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      &sDestRemain,
                                      MTU_NLS_NCHAR_CONV_EXCP )
                      != IDE_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( &sSourceIndex, sSourceFence );

            sResultValue += ( sTempRemain - sDestRemain );
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        *aResultLen = sResultValue - aResult;
    }
    else
    {
        idlOS::memcpy( aResult, aSource->value, aSource->length );
        *aResultLen = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH )
    {
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::convertNCharType( smiValue    * aSmiValue,
                                        UChar       * aResult,
                                        UInt          aResultLength )
{
    mtdNcharType   * sSource = NULL;
    SChar          * sNationalCharSetStr = NULL;
    mtlModule      * sSrcCharSet = NULL;
    mtlModule      * sDestCharSet = NULL;
    UChar            sTempBuffer[IDA_VARCHAR_MAX + 1] = { 0, };
    UInt             sTempBufferLength = aResultLength;
    SChar            sTargetChar = ' ';
    UInt             sOffset = 0;
    UInt             i = 0;

    sSource = (mtdNcharType*)aSmiValue->value;

    sNationalCharSetStr = mtc::getNationalCharSet();

    IDE_TEST( mtl::moduleByName( (const mtlModule **)&sSrcCharSet,
                                 sNationalCharSetStr,
                                 (UInt)idlOS::strlen( sNationalCharSetStr ) )
              != IDE_SUCCESS );

    IDE_TEST( mtl::moduleByName( (const mtlModule **)&sDestCharSet,
                                 UTF16_CHARSET_STR,
                                 (UInt)idlOS::strlen( UTF16_CHARSET_STR ) )
              != IDE_SUCCESS );

    IDE_TEST( charSetConvert( sSrcCharSet,
                              sDestCharSet,
                              sSource,
                              sTempBuffer,
                              &sTempBufferLength )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aResultLength < ( ( sTempBufferLength / 2 ) +  /* '\'  EXAMPLE '\'   */
                                      ( sTempBufferLength * 2 ) ), /* CODE         '0020' */
                    ERR_NOT_ENOUGH_SPACE );

    for ( i = 0; i < sTempBufferLength; i++ )
    {
        if ( ( i % 2 ) == 0 )
        {
            aResult[sOffset] = '\\';
            sOffset++;
        }
        else
        {
            /* do nothing */
        }

        sTargetChar = ( sTempBuffer[i] & 0xF0 ) >> 4;
        aResult[sOffset] = rpdConvertSQL::hexToString( sTargetChar );
        sOffset++;

        sTargetChar = ( sTempBuffer[i] & 0x0F );
        aResult[sOffset] = rpdConvertSQL::hexToString( sTargetChar );
        sOffset++;
    }

    aResult[sOffset] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ENOUGH_SPACE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Not enough space for convert buffer" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool rpdConvertSQL::isSupportDataType( UInt   aDataTypeId )
{
    idBool sIsSupport = ID_FALSE;

    switch ( aDataTypeId )
    {
        case MTD_FLOAT_ID:
        case MTD_NUMERIC_ID:
        case MTD_DOUBLE_ID:
        case MTD_REAL_ID:
        case MTD_BIGINT_ID:
        case MTD_INTEGER_ID:
        case MTD_SMALLINT_ID:
        case MTD_CHAR_ID :
        case MTD_VARCHAR_ID :
        case MTD_DATE_ID:
        case MTD_NVARCHAR_ID:
        case MTD_NCHAR_ID:
        case MTD_BIT_ID :
        case MTD_VARBIT_ID :
        case MTD_BYTE_ID :
        case MTD_VARBYTE_ID :
        case MTD_NIBBLE_ID:

           sIsSupport = ID_TRUE;
           break;

        default:
           
           sIsSupport = ID_FALSE;
           break;
    }

    return sIsSupport;
}

IDE_RC rpdConvertSQL::convertValue( mtcColumn   * aColumn,
                                    smiValue    * aSmiValue,
                                    SChar       * aValue,
                                    UInt          aValueLength )
{
    SChar       sErrorMessage[128] = { 0, };

    switch ( aColumn->type.dataTypeId )
    {
        case MTD_FLOAT_ID:
        case MTD_NUMERIC_ID:
        case MTD_DOUBLE_ID:
        case MTD_REAL_ID:
        case MTD_BIGINT_ID:
        case MTD_INTEGER_ID:
        case MTD_SMALLINT_ID:

            IDE_TEST( convertNumericType( aColumn,
                                          aSmiValue,
                                          aValue,
                                          aValueLength )
                      != IDE_SUCCESS );
            break;

        case MTD_CHAR_ID :
        case MTD_VARCHAR_ID :

            IDE_TEST( convertCharType( aColumn,
                                       aSmiValue,
                                       aValue,
                                       aValueLength )
                      != IDE_SUCCESS );
            break;

        case MTD_DATE_ID:

            IDE_TEST( convertDateType( aColumn,
                                       aSmiValue,
                                       aValue,
                                       aValueLength )
                      != IDE_SUCCESS );
            break;

        case MTD_NVARCHAR_ID:
        case MTD_NCHAR_ID:

            IDE_TEST( convertNCharType( aSmiValue,
                                        (UChar *)aValue,
                                        aValueLength )
                      != IDE_SUCCESS );
            break;

        case MTD_BIT_ID :
        case MTD_VARBIT_ID :

            IDE_TEST( convertBitType( aSmiValue,
                                      (UChar *)aValue,
                                      aValueLength )
                      != IDE_SUCCESS );

            break;

        case MTD_BYTE_ID :
        case MTD_VARBYTE_ID :

            IDE_TEST( convertByteType( aSmiValue,
                                       aValue,
                                       aValueLength )
                      != IDE_SUCCESS );
            break;

        case MTD_NIBBLE_ID:

            IDE_TEST( convertNibbleType( aSmiValue,
                                         aValue,
                                         aValueLength )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_NOT_SUPPORT_DATA_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_DATA_TYPE )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "Not Support Data Type( id : %"ID_UINT32_FMT" )", aColumn->type.dataTypeId );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpdConvertSQL::getColumnListClause( rpdMetaItem  * aMetaItemForSource,
                                         rpdMetaItem  * aMetaItemForTarget,
                                         UInt           aCIDCount,
                                         UInt         * aCIDArrayForSource,
                                         UInt         * aCIDArrayForTarget,
                                         smiValue     * aSmiValueArray,
                                         idBool         aNeedColumnName,
                                         idBool         aIsWhereClause,
                                         idBool         aNeedRPad,
                                         SChar        * aDelimeter,
                                         SChar        * aValueClause,
                                         UInt           aValueClauseLength,
                                         idBool       * aIsValid )
{
    UInt          i = 0;
    UInt          sColumnCountForSource = 0;
    rpdColumn   * sColumnArrayForSource = NULL;
    UInt          sColumnCountForTarget = 0;
    rpdColumn   * sColumnArrayForTarget = NULL;
    UInt          sColumnIndexForSource = 0;
    UInt          sColumnIndexForTarget = 0;

    SChar       * sValueClause = aValueClause;
    UInt          sValueClauseLength = aValueClauseLength;
    idBool        sNeedDelimeter = ID_FALSE;

    sColumnCountForSource = aMetaItemForSource->mColCount;
    sColumnArrayForSource = aMetaItemForSource->mColumns;
    sColumnCountForTarget = aMetaItemForTarget->mColCount;
    sColumnArrayForTarget = aMetaItemForTarget->mColumns;

    *aIsValid = ID_TRUE;

    for ( i = 0; i < aCIDCount; i++ )
    {
        if ( aMetaItemForTarget->mIsReplCol[i] == ID_TRUE )
        {
            sColumnIndexForSource = getColumnIndex( aCIDArrayForSource[i] );
            sColumnIndexForTarget = getColumnIndex( aCIDArrayForTarget[i] );

            if ( ( sColumnArrayForTarget[sColumnIndexForTarget].mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                 != QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                if ( sNeedDelimeter == ID_TRUE )
                {
                    idlOS::snprintf( sValueClause,
                                     sValueClauseLength,
                                     " %s ",
                                     aDelimeter );
                    sValueClauseLength = sValueClauseLength - idlOS::strlen( sValueClause );
                    sValueClause = sValueClause + idlOS::strlen( sValueClause );
                }
                else
                {
                    /* do nothing */
                }

                if ( ( sColumnIndexForSource < sColumnCountForSource ) &&
                     ( sColumnIndexForTarget < sColumnCountForTarget ) )
                {
                    if ( getColumnClause( &sColumnArrayForSource[sColumnIndexForSource],
                                          &sColumnArrayForTarget[sColumnIndexForTarget],
                                          &aSmiValueArray[i],
                                          aNeedColumnName,
                                          aIsWhereClause,
                                          aNeedRPad,
                                          sValueClause,
                                          sValueClauseLength )
                         != IDE_SUCCESS )
                    {
                        IDE_ERRLOG( IDE_RP_0 );

                        idlOS::snprintf( sValueClause,
                                         sValueClauseLength,
                                         "<None printable data>" );

                        *aIsValid = ID_FALSE;
                    }
                    else
                    {
                        /* do nothing */
                    }

                    sValueClauseLength = sValueClauseLength - idlOS::strlen( sValueClause );
                    sValueClause = sValueClause + idlOS::strlen( sValueClause );
                }
                else
                {
                    idlOS::snprintf( sValueClause,
                                     sValueClauseLength,
                                     "<None printable data>" );
                    sValueClauseLength = sValueClauseLength - idlOS::strlen( sValueClause );
                    sValueClause = sValueClause + idlOS::strlen( sValueClause );

                    *aIsValid = ID_FALSE;

                    IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
                }

                sNeedDelimeter = ID_TRUE;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }
}

IDE_RC rpdConvertSQL::getColumnClause( rpdColumn    * aColumnForSource,
                                       rpdColumn    * aColumnForTarget,
                                       smiValue     * aSmiValue,
                                       idBool         aNeedColumnName,
                                       idBool         aIsWhereClause,
                                       idBool         aNeedRPad,
                                       SChar        * aColumnValue,
                                       UInt           aColumnValueLength )
{
    const mtdModule * sModule = NULL;
    idBool            sNeedSingleQuotation = ID_FALSE;
    idBool            sNeedRPad = ID_FALSE;
    idBool            sNeedRTrim = ID_FALSE;
    idBool            sNeedColumnType = ID_FALSE;
    idBool            sIsUTF16 = ID_FALSE;
    SChar           * sColumnValue = aColumnValue;
    UInt              sColumnValueLength = aColumnValueLength;
    idBool            sIsNullValue = ID_FALSE;
    SChar             sErrorMessage[128] = { 0, };

    IDE_TEST_RAISE( isSupportDataType( aColumnForSource->mColumn.type.dataTypeId ) == ID_FALSE,
                    ERR_NOT_SUPPORT_DATA_TYPE );

    IDE_TEST( isNullValue( &(aColumnForSource->mColumn),
                           aSmiValue,
                           &sIsNullValue )
              != IDE_SUCCESS );

    if ( sIsNullValue == ID_FALSE )
    {
        if ( aNeedColumnName == ID_TRUE )
        {
            idlOS::snprintf( sColumnValue,
                             sColumnValueLength,
                             "%s = ",
                             aColumnForTarget->mColumnName );
            sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
            sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
        }
        else
        {
            /* do nothing */
        }

        getColumnValueCondition( aColumnForTarget->mColumn.type.dataTypeId,
                                 aColumnForSource->mColumn.type.dataTypeId,
                                 &sNeedSingleQuotation,
                                 &sNeedRPad,
                                 &sNeedRTrim,
                                 &sNeedColumnType,
                                 &sIsUTF16 );

        if ( sNeedColumnType == ID_TRUE ) 
        {
            IDE_TEST( mtd::moduleById( &sModule, 
                                       aColumnForTarget->mColumn.type.dataTypeId )
                      != IDE_SUCCESS );

            idlOS::snprintf( sColumnValue,
                             sColumnValueLength,
                             "%s",
                             sModule->names->string );

            sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
            sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
        }
        else
        {
            /* do nothing */
        }

        if ( sIsUTF16 == ID_TRUE )
        {
            idlOS::snprintf( sColumnValue,
                             sColumnValueLength,
                             "UNISTR(" );

            sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
            sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
        }
        else
        {
            /* do nothing */
        }

        if ( sNeedSingleQuotation == ID_TRUE )
        {
            addSingleQuotation( sColumnValue,
                                sColumnValueLength );
            sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
            sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST ( convertValue( &(aColumnForSource->mColumn),
                                 aSmiValue,
                                 sColumnValue,
                                 sColumnValueLength ) != IDE_SUCCESS );

        if ( sNeedRTrim == ID_TRUE )
        {
            rTrim( sIsUTF16,
                   sColumnValue );
        }
        else
        {
            /* do nothing */
        }

        if ( ( aNeedRPad == ID_TRUE ) &&
             ( sIsUTF16 == ID_TRUE ) &&
             ( sNeedRPad == ID_TRUE ) )
        {
            IDE_TEST( rPadForUTF16String( aColumnForTarget->mColumn.precision,
                                          sColumnValue,
                                          sColumnValueLength )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        /* trim 하고 pad 이후에 위치를 변경 하여야 한다 */
        sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
        sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );

        if ( sNeedSingleQuotation == ID_TRUE )
        {
            addSingleQuotation( sColumnValue,
                                sColumnValueLength );
            sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
            sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
        }
        else
        {
            /* do nothing */
        }

        if ( sIsUTF16 == ID_TRUE )
        {
            idlOS::snprintf( sColumnValue,
                             sColumnValueLength,
                             ")" );

            sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
            sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        if ( aNeedColumnName == ID_TRUE )
        {
            /* UPDATE T1 SET C2 = NULL WHERE C1 IS NULL */
            if ( aIsWhereClause == ID_TRUE )
            {
                idlOS::snprintf( sColumnValue,
                                 sColumnValueLength,
                                 "%s IS NULL",
                                 aColumnForTarget->mColumnName );
            }
            else
            {
                idlOS::snprintf( sColumnValue,
                                 sColumnValueLength,
                                 "%s = NULL",
                                 aColumnForTarget->mColumnName );
            }
        }
        else
        {
            idlOS::snprintf( sColumnValue,
                             sColumnValueLength,
                             "NULL" );
        }
        sColumnValueLength = sColumnValueLength - idlOS::strlen( sColumnValue );
        sColumnValue = sColumnValue + idlOS::strlen( sColumnValue );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_DATA_TYPE )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "Not Support Data Type( id : %"ID_UINT32_FMT" )", aColumnForSource->mColumn.type.dataTypeId );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::getColumnNameClause( rpdMetaItem  * aMetaItemForSource,
                                           rpdMetaItem  * aMetaItemForTarget,
                                           rpdXLog      * aXLog,
                                           SChar        * aColumnName,
                                           UInt           aColumnNameLength )
{
    UInt            i = 0;
    UInt            sColumnCount = 0;
    rpdColumn     * sColumnForTarget = NULL;
    rpdColumn     * sColumnForSource = NULL;
    SChar         * sColumnName = NULL;
    UInt            sColumnNameLength = 0;
    UInt            sCID = 0;
    UInt            sCIDIndex = 0;
    idBool          sIsFirst = ID_FALSE;
    idBool          sIsNullValue = ID_FALSE;

    sColumnName = aColumnName;
    sColumnNameLength = aColumnNameLength;

    sColumnCount = aXLog->mColCnt;

    sIsFirst = ID_TRUE;
    for ( i = 0; i < sColumnCount; i++ )
    {
        if ( aMetaItemForTarget->mIsReplCol[i] == ID_TRUE )
        {
            sColumnForTarget = aMetaItemForTarget->getRpdColumn( aXLog->mCIDs[i] );
            IDE_TEST_RAISE( sColumnForTarget == NULL, ERR_META_NO_SUCH_DATA );

            sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
            sCID = aMetaItemForSource->mMapColID[sCIDIndex];
            sColumnForSource = aMetaItemForSource->getRpdColumn( sCID );
            IDE_TEST_RAISE( sColumnForSource == NULL, ERR_META_NO_SUCH_DATA );

            IDE_TEST( isNullValue( &(sColumnForSource->mColumn),
                                   &(aXLog->mACols[i]),
                                   &sIsNullValue )
                      != IDE_SUCCESS );

            if ( ( ( sColumnForTarget->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK ) 
                   != QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
                 ( sIsNullValue == ID_FALSE ) )
            {
                if ( sIsFirst == ID_TRUE )
                {
                    idlOS::snprintf( sColumnName,
                                     sColumnNameLength,
                                     "%s",
                                     sColumnForTarget->mColumnName );
                    sIsFirst = ID_FALSE;
                }
                else
                {
                    idlOS::snprintf( sColumnName,
                                     sColumnNameLength,
                                     ", %s",
                                     sColumnForTarget->mColumnName );
                }

                sColumnNameLength = sColumnNameLength - idlOS::strlen( sColumnName );
                sColumnName = sColumnName + idlOS::strlen( sColumnName );
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_NO_SUCH_DATA )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::getInsertSQL( rpdMetaItem * aMetaItemForSource,
                                    rpdMetaItem * aMetaItemForTarget,
                                    rpdXLog     * aXLog,
                                    SChar       * aQueryString,
                                    UInt          aQueryStringLength )
{
    SChar       * sTempString = NULL;
    UInt          sTempStringLength = 0;
    UInt          i = 0;
    rpdColumn   * sColumnForTarget = NULL;
    rpdColumn   * sColumnForSource = NULL;
    UInt          sColumnID = 0;
    UInt          sCIDIndex = 0;
    idBool        sIsFirst = ID_FALSE;
    idBool        sIsNullValue = ID_FALSE;

    idlOS::snprintf( aQueryString,
                     aQueryStringLength,
                     "INSERT /*+ NO_EXEC_FAST */ INTO %s.%s ( ",
                     aMetaItemForTarget->mItem.mLocalUsername,
                     aMetaItemForTarget->mItem.mLocalTablename );
    sTempString = aQueryString + idlOS::strlen( aQueryString );
    sTempStringLength = aQueryStringLength - idlOS::strlen( aQueryString );

    /* COLUMN NAME */
    IDE_TEST( getColumnNameClause( aMetaItemForSource,
                                   aMetaItemForTarget,
                                   aXLog,
                                   sTempString,
                                   sTempStringLength )
              != IDE_SUCCESS );
    sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
    sTempString = sTempString + idlOS::strlen( sTempString );

    idlOS::snprintf( sTempString,
                     sTempStringLength,
                     " ) VALUES ( " );
    sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
    sTempString = sTempString + idlOS::strlen( sTempString );

    sIsFirst = ID_TRUE;
    for ( i = 0; i < aXLog->mColCnt; i++ )
    {
        if ( aMetaItemForTarget->mIsReplCol[i] == ID_TRUE )
        {
            sColumnForTarget = aMetaItemForTarget->getRpdColumn( aXLog->mCIDs[i] );
            IDE_TEST_RAISE( sColumnForTarget == NULL, ERR_META_NO_SUCH_DATA );

            sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
            sColumnID = aMetaItemForTarget->mMapColID[sCIDIndex];
            sColumnForSource = aMetaItemForSource->getRpdColumn( sColumnID );
            IDE_TEST_RAISE( sColumnForSource == NULL, ERR_META_NO_SUCH_DATA );

            IDE_TEST( isNullValue( &(sColumnForSource->mColumn),
                                   &(aXLog->mACols[i]),
                                   &sIsNullValue )
                      != IDE_SUCCESS );

            if ( ( ( sColumnForTarget->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                   != QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
                 ( sIsNullValue == ID_FALSE ) )
            {
                if ( sIsFirst == ID_TRUE )
                {
                    idlOS::snprintf( sTempString,
                                     sTempStringLength,
                                     "?" );
                    sIsFirst = ID_FALSE;
                }
                else
                {
                    idlOS::snprintf( sTempString,
                                     sTempStringLength,
                                     ", ?" );
                }

                sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
                sTempString = sTempString + idlOS::strlen( sTempString );
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    idlOS::snprintf( sTempString,
                     sTempStringLength,
                     " );" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_NO_SUCH_DATA )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdConvertSQL::getUpdateSQL( rpdMetaItem * aMetaItemForSource,
                                    rpdMetaItem * aMetaItemForTarget,
                                    rpdXLog     * aXLog,
                                    idBool        aNeedBeforeImage,
                                    SChar       * aQueryString,
                                    UInt          aQueryStringLength )
{
    UInt          i = 0;
    SChar       * sTempString = NULL;
    UInt          sTempStringLength = 0;
    UInt          sColumnID = 0;
    UInt          sCIDIndex = 0;
    rpdColumn   * sColumnForTarget = NULL;
    rpdColumn   * sColumnForSource = NULL;
    const SChar * sValue = NULL;
    idBool        sIsFirst = ID_FALSE;
    idBool        sIsNullValue = ID_FALSE;

    IDE_DASSERT( aXLog->mPKColCnt == aMetaItemForTarget->mPKIndex.keyColCount );

    idlOS::snprintf( aQueryString,
                     aQueryStringLength,
                     "UPDATE /*+ NO_EXEC_FAST NO_SERIAL_FILTER */%s.%s SET ",
                     aMetaItemForTarget->mItem.mLocalUsername,
                     aMetaItemForTarget->mItem.mLocalTablename );
    sTempString = aQueryString + idlOS::strlen( aQueryString );
    sTempStringLength = aQueryStringLength - idlOS::strlen( aQueryString );

    /* UPDATE COLUMN */
    sIsFirst = ID_TRUE;
    for ( i = 0; i <aXLog->mColCnt; i++ )
    {
        sColumnID = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
        sColumnForTarget = aMetaItemForTarget->getRpdColumn( sColumnID );
        IDE_TEST_RAISE( sColumnForTarget == NULL, ERR_META_NO_SUCH_DATA );

        if ( ( aMetaItemForTarget->mIsReplCol[ ( sColumnID & SMI_COLUMN_ID_MASK ) ] == ID_TRUE ) &&
             ( ( sColumnForTarget->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
               != QCM_COLUMN_HIDDEN_COLUMN_TRUE ) ) 
        {
            sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
            sColumnID = aMetaItemForTarget->mMapColID[sCIDIndex];
            sColumnForSource = aMetaItemForSource->getRpdColumn( sColumnID );
            IDE_TEST_RAISE( sColumnForSource == NULL, ERR_META_NO_SUCH_DATA );

            IDE_TEST( isNullValue( &(sColumnForSource->mColumn),
                                   &(aXLog->mACols[i]),
                                   &sIsNullValue )
                      != IDE_SUCCESS );

            if ( sIsNullValue == ID_FALSE )
            {
                sValue = "?";
            }
            else
            {
                sValue = "NULL";
            }

            if ( sIsFirst == ID_TRUE )
            {
                idlOS::snprintf( sTempString,
                                 sTempStringLength,
                                 "%s = %s",
                                 sColumnForTarget->mColumnName,
                                 sValue );
                sIsFirst = ID_FALSE;
            }
            else
            {
                idlOS::snprintf( sTempString,
                                 sTempStringLength,
                                 ", %s = %s",
                                 sColumnForTarget->mColumnName,
                                 sValue );
            }

            sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
            sTempString = sTempString + idlOS::strlen( sTempString );
        }
        else
        {
            /* do nothing */
        }
    }

    idlOS::snprintf( sTempString,
                     sTempStringLength,
                     " WHERE " );
    sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
    sTempString = sTempString + idlOS::strlen( sTempString );

    /* PK */
    sIsFirst = ID_TRUE;
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        sColumnID = aMetaItemForTarget->mPKIndex.keyColumns[i].column.id;
        sColumnForTarget = aMetaItemForTarget->getRpdColumn( sColumnID );
        IDE_TEST_RAISE( sColumnForTarget == NULL, ERR_META_NO_SUCH_DATA );

        if ( ( aMetaItemForTarget->mIsReplCol[ ( sColumnID & SMI_COLUMN_ID_MASK ) ] == ID_TRUE ) &&
             ( sColumnForTarget->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
             != QCM_COLUMN_HIDDEN_COLUMN_TRUE )
        {
            if ( sIsFirst == ID_TRUE )
            {
                idlOS::snprintf( sTempString,
                                 sTempStringLength,
                                 "%s = ?",
                                 sColumnForTarget->mColumnName );
                sIsFirst = ID_FALSE;
            }
            else
            {
                idlOS::snprintf( sTempString,
                                 sTempStringLength,
                                 "AND %s = ?",
                                 sColumnForTarget->mColumnName );
            }
            sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
            sTempString = sTempString + idlOS::strlen( sTempString );
        }
        else
        {
            /* do nothing */
        }
    }

    if ( aMetaItemForTarget->mTsFlag != NULL )
    {
        rpdConvertSQL::getTimestampCondition( aMetaItemForTarget->mTsFlag->column.id,
                                              aXLog,
                                              aMetaItemForTarget->mColumns,
                                              sTempString,
                                              sTempStringLength );
        sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
        sTempString = sTempString + idlOS::strlen( sTempString );
    }
    else
    {
        if ( aNeedBeforeImage == ID_TRUE )
        {
            for ( i = 0; i < aXLog->mColCnt; i++ )
            {
                sColumnID = aXLog->mCIDs[i];
                sColumnForTarget = aMetaItemForTarget->getRpdColumn( sColumnID );
                IDE_TEST_RAISE( sColumnForTarget == NULL, ERR_META_NO_SUCH_DATA );

                if ( ( aMetaItemForTarget->mIsReplCol[ (sColumnID & SMI_COLUMN_ID_MASK) ] == ID_TRUE ) &&
                     ( sColumnForTarget->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                     != QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                {
                    sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
                    sColumnID = aMetaItemForTarget->mMapColID[sCIDIndex];
                    sColumnForSource = aMetaItemForSource->getRpdColumn( sColumnID );
                    IDE_TEST_RAISE( sColumnForSource == NULL, ERR_META_NO_SUCH_DATA );

                    if ( ( sColumnForTarget->mColumn.column.flag & SMI_COLUMN_TYPE_MASK ) != SMI_COLUMN_TYPE_LOB )
                    {
                        IDE_TEST( isNullValue( &(sColumnForSource->mColumn),
                                               &(aXLog->mBCols[i]),
                                               &sIsNullValue )
                                  != IDE_SUCCESS );

                        if ( sIsNullValue == ID_FALSE )
                        {
                            idlOS::snprintf( sTempString,
                                             sTempStringLength,
                                             " AND %s = ?",
                                             sColumnForTarget->mColumnName );
                        }
                        else
                        {
                            idlOS::snprintf( sTempString,
                                             sTempStringLength,
                                             " AND %s IS NULL",
                                             sColumnForTarget->mColumnName );
                        }
                        sTempStringLength = sTempStringLength - idlOS::strlen( sTempString );
                        sTempString = sTempString + idlOS::strlen( sTempString );
                    }
                    else
                    {
                        /* do nothing */
                    }

                }
                else
                {
                    /* do nothing */
                }
            }
        }
        else
        {
            /* do nothing */
        }
    }

    idlOS::snprintf( sTempString,
                     sTempStringLength,
                     " ;" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_NO_SUCH_DATA )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpdConvertSQL::getTimestampCondition( UInt        aColumnID,
                                           rpdXLog   * aXLog,
                                           rpdColumn * aColumnArray,
                                           SChar     * aCondition,
                                           UInt        aConditionLength )

{
    UInt      i = 0;
    UInt      sTimestampIndex = 0;
    UInt      sCID = 0;
    SChar   * sCondition = aCondition;
    UInt      sConditionLength = aConditionLength;

    sTimestampIndex = getColumnIndex( aColumnID );

    for ( i = 0; i < aXLog->mColCnt; i++ )
    {
        sCID = aXLog->mCIDs[i];

        if ( sCID == sTimestampIndex )
        {
            idlOS::snprintf( sCondition,
                             sConditionLength,
                             " AND %s <= ?",
                             aColumnArray[sTimestampIndex].mColumnName );
            //sConditionLength = sConditionLength - idlOS::strlen( sCondition );
            //sCondition = sCondition + idlOS::strlen( sCondition );
            break;
        }
        else
        {
            /* do nothing */
        }
    }
}

IDE_RC rpdConvertSQL::isNullValue( mtcColumn           * aColumn,
                                   smiValue            * aSmiValue,
                                   idBool              * aIsNullValue )
{
    const mtdModule * sModule = NULL;
    idBool            sIsNullValue = ID_FALSE;

    IDE_TEST( mtd::moduleById( &sModule,
                               aColumn->type.dataTypeId )
              != IDE_SUCCESS );

    if ( ( aSmiValue->value != NULL ) && ( aSmiValue->length != 0 ) )
    {
        if ( sModule->isNull( NULL, (const void*)(aSmiValue->value) )
             == ID_TRUE )
        {
            sIsNullValue = ID_TRUE;
        }
        else
        {
            sIsNullValue = ID_FALSE;
        }
    }
    else
    {
        sIsNullValue = ID_TRUE;
    }

    *aIsNullValue = sIsNullValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SChar rpdConvertSQL::hexToString( UChar    aHex )
{
    return (aHex < 10) ? (aHex + '0') : (aHex + 'A' - 10 );
}
