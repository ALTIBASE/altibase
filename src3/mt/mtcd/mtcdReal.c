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
 

/***********************************************************************
 * $Id: mtcdReal.cpp 36509 2009-11-04 03:26:25Z sungminee $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

#include <math.h>

#define MTD_REAL_ALIGN   (sizeof(mtdRealType))
#define MTD_REAL_SIZE    (sizeof(mtdRealType))

extern mtdModule mtcdReal;

const acp_uint32_t mtcdRealNull = ( MTD_REAL_EXPONENT_MASK|
                                   MTD_REAL_FRACTION_MASK );

static ACI_RC mtdInitializeReal( acp_uint32_t aNo );

static ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                           acp_uint32_t* aArguments,
                           acp_sint32_t* aPrecision,
                           acp_sint32_t* aScale );

static ACI_RC mtdValue( mtcTemplate*  aTemplate,
                        mtcColumn*    aColumn,
                        void*         aValue,
                        acp_uint32_t* aValueOffset,
                        acp_uint32_t  aValueSize,
                        const void*   aToken,
                        acp_uint32_t  aTokenLength,
                        ACI_RC*       aResult );

static acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                                   const void*      aRow,
                                   acp_uint32_t     aFlag );

static void mtdNull( const mtcColumn* aColumn,
                     void*            aRow,
                     acp_uint32_t     aFlag );

static acp_uint32_t mtdHash( acp_uint32_t     aHash,
                             const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                             const void*      aRow,
                             acp_uint32_t     aFlag );

static acp_sint32_t mtdRealLogicalComp( mtdValueInfo* aValueInfo1,
                                        mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdRealMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                             mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdRealMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                              mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdRealStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                                                mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdRealStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                                                 mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdRealStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                                   mtdValueInfo* aValueInfo2 );

static acp_sint32_t mtdRealStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                                    mtdValueInfo* aValueInfo2 );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*     aColumn,
                           void*          aValue,
                           acp_uint32_t   aValueSize);

static acp_double_t mtdSelectivityReal( void* aColumnMax, 
                                        void* aColumnMin, 
                                        void* aValueMax,  
                                        void* aValueMin );

static ACI_RC mtdEncode( mtcColumn*    aColumn,
                         void*         aValue,
                         acp_uint32_t  aValueSize,
                         acp_uint8_t*  aCompileFmt,
                         acp_uint32_t  aCompileFmtLen,
                         acp_uint8_t*  aText,
                         acp_uint32_t* aTextLen,
                         ACI_RC*       aRet );

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestVale,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdNullValueSize();

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"REAL" },
};

static mtcColumn mtdColumn;

mtdModule mtcdReal = {
    mtdTypeName,
    &mtdColumn,
    MTD_REAL_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_REAL_ALIGN,
    MTD_GROUP_NUMBER|
    MTD_CANON_NEEDLESS|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_ENABLE|
    MTD_NUMBER_GROUP_TYPE_DOUBLE|
    MTD_SEARCHABLE_PRED_BASIC|
    MTD_UNSIGNED_ATTR_TRUE|
    MTD_NUM_PREC_RADIX_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    7,
    0,
    0,
    (void*)&mtcdRealNull,
    mtdInitializeReal,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdRealLogicalComp,    // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdRealMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdRealMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdRealStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdRealStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdRealStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdRealStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonizeDefault,
    mtdEndian,
    mtdValidate,
    mtdSelectivityReal,
    mtdEncode,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSizeDefault
};

ACI_RC mtdInitializeReal( acp_uint32_t aNo )
{
    ACI_TEST_RAISE( sizeof(acp_float_t) != 4, ERR_INCOMPATIBLE_TYPE );
    
    ACI_TEST( mtdInitializeModule( &mtcdReal, aNo ) != ACI_SUCCESS );    

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdReal,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INCOMPATIBLE_TYPE );
    aciSetErrorCode(mtERR_FATAL_INCOMPATIBLE_TYPE);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                    acp_uint32_t* aArguments,
                    acp_sint32_t* aPrecision,
                    acp_sint32_t* aScale )
{
    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);

    ACI_TEST_RAISE( *aArguments != 0, ERR_INVALID_PRECISION );

    *aColumnSize = MTD_REAL_SIZE;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_PRECISION );
    aciSetErrorCode(mtERR_ABORT_INVALID_PRECISION);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*   aTemplate ,
                 mtcColumn*     aColumn ,
                 void*          aValue,
                 acp_uint32_t*  aValueOffset,
                 acp_uint32_t   aValueSize,
                 const void*    aToken,
                 acp_uint32_t   aTokenLength,
                 ACI_RC*        aResult)
{
    acp_uint32_t   sValueOffset;
    mtdRealType*   sValue;
    acp_double_t   sTemp;

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_REAL_ALIGN );
    
    if( sValueOffset + MTD_REAL_SIZE <= aValueSize )
    {
        sValue = (mtdRealType*)( (acp_uint8_t*)aValue + sValueOffset );
        if( aTokenLength == 0 )
        {
            mtcdReal.null( mtcdReal.column, sValue, MTD_OFFSET_USELESS );
        }
        else
        {
            char  sBuffer[1024];
            ACI_TEST_RAISE( aTokenLength >= sizeof(sBuffer),
                            ERR_INVALID_LITERAL );
            acpMemCpy( sBuffer, aToken, aTokenLength );
            sBuffer[aTokenLength] = 0;
//BUGBUG            *sValue = idlOS::strtod( sBuffer, (char**)NULL );

            acpCStrToDouble ( sBuffer, acpCStrLen(sBuffer, ACP_UINT32_MAX), &sTemp, (char**)NULL);
            *sValue = sTemp;
            
            /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            if( *sValue == 0 )
            {
                *sValue = 0;
            }
            /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */
            ACI_TEST_RAISE( mtdIsNull( mtcdReal.column,
                                       sValue,
                                       MTD_OFFSET_USELESS ) == ACP_TRUE,
                            ERR_VALUE_OVERFLOW );

            // To fix BUG-12281
            // underflow 검사
            if( ( (*sValue) < MTD_REAL_MINIMUM && (*sValue) > -MTD_REAL_MINIMUM ) &&
                ( *sValue != 0 ) )
            {
                *sValue = 0;
            }
            else
            {
                // Nothing to do
            }
        }
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset + MTD_REAL_SIZE;
        
        *aResult = ACI_SUCCESS;
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    
    ACI_EXCEPTION( ERR_VALUE_OVERFLOW );
    aciSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aTemp,
                            const void     * aTemp2,
                            acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    
    return (acp_uint32_t)MTD_REAL_SIZE;
}

void mtdNull( const mtcColumn* aColumn,
              void           * aRow,
              acp_uint32_t     aFlag )
{
    static acp_uint32_t sNull = ( MTD_REAL_EXPONENT_MASK | MTD_REAL_FRACTION_MASK );
    
    mtdRealType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdRealType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        acpMemCpy( sValue, &sNull, MTD_REAL_SIZE );
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void     * aRow,
                      acp_uint32_t     aFlag )
{
    const mtdRealType* sValue;
  
    sValue = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdReal.staticNull );

    if( mtcdReal.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) != ACP_TRUE )
    {
        aHash = mtcHash( aHash, (const acp_uint8_t*)sValue, MTD_REAL_SIZE );
    }
    
    return aHash;
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void     * aRow,
                      acp_uint32_t     aFlag )
{
    const mtdRealType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdReal.staticNull );

    return ( ( *(acp_uint32_t*)sValue & MTD_REAL_EXPONENT_MASK )
             == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;
}

acp_sint32_t
mtdRealLogicalComp( mtdValueInfo* aValueInfo1,
                    mtdValueInfo* aValueInfo2 )
{
    return mtdRealMtdMtdKeyAscComp( aValueInfo1,
                                    aValueInfo2 );
}

acp_sint32_t
mtdRealMtdMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                         mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdRealType* sValue1;
    const mtdRealType* sValue2;
    acp_bool_t         sNull1;
    acp_bool_t         sNull2;

    //---------
    // value1
    //---------    
    sValue1 = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdReal.staticNull );
    
    sNull1 = ( ( *(acp_uint32_t*)sValue1 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // value2
    //---------    
    sValue2 = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdReal.staticNull );
    
    sNull2 = ( ( *(acp_uint32_t*)sValue2 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        if( *sValue1 > *sValue2 )
        {
            return 1;
        }
        if( *sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( (sNull1 == ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ACP_TRUE) /*&& (sNull2 == ACP_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdRealMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                          mtdValueInfo* aValueInfo2 )
{
    /***********************************************************************
     *
     * Description : Mtd 타입의 Key들 간의 descending compare
     *
     * Implementation :
     *
     ***********************************************************************/

    const mtdRealType* sValue1;
    const mtdRealType* sValue2;
    acp_bool_t         sNull1;
    acp_bool_t         sNull2;

    //---------
    // value1
    //---------    
    sValue1 = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdReal.staticNull );

    sNull1 = ( ( *(acp_uint32_t*)sValue1 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;


    //---------
    // value2
    //---------    
    sValue2 = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdReal.staticNull );
    
    sNull2 = ( ( *(acp_uint32_t*)sValue2 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // compare
    //---------    

    if( (sNull1 != ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( (sNull1 == ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ACP_TRUE) /*&& (sNull2 == ACP_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdRealStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
    /***********************************************************************
     *
     * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
     *
     * Implementation :
     *
     ***********************************************************************/
    
    mtdRealType        sRealValue1;
    mtdRealType*       sValue1;
    const mtdRealType* sValue2;
    acp_bool_t         sNull1;
    acp_bool_t         sNull2;
    
    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    MTC_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );
    
    sNull1 = ( ( *(acp_uint32_t*)sValue1 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;
    
    //---------
    // value1
    //---------
    sValue2 = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdReal.staticNull );

    sNull2 = ( ( *(acp_uint32_t*)sValue2 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // compare
    //---------    
    
    if( (sNull1 != ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        if( *sValue1 > *sValue2 )
        {
            return 1;
        }
        if( *sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( (sNull1 == ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ACP_TRUE) /*&& (sNull2 == ACP_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdRealStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                             mtdValueInfo* aValueInfo2 )
{
    /***********************************************************************
     *
     * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
     *
     * Implementation :
     *
     ***********************************************************************/
    
    mtdRealType        sRealValue1;
    mtdRealType*       sValue1;
    const mtdRealType* sValue2;
    acp_bool_t         sNull1;
    acp_bool_t         sNull2;
    
    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    MTC_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( ( *(acp_uint32_t*)sValue1 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // value1
    //---------
    sValue2 = (const mtdRealType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdReal.staticNull );

    sNull2 = ( ( *(acp_uint32_t*)sValue2 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // compare
    //---------

    if( (sNull1 != ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( (sNull1 == ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ACP_TRUE) /*&& (sNull2 == ACP_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdRealStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
    /***********************************************************************
     *
     * Description : Stored Key들 간의 ascending compare
     *
     * Implementation :
     *
     ***********************************************************************/

    mtdRealType  sRealValue1;
    mtdRealType* sValue1;
    mtdRealType  sRealValue2;
    mtdRealType* sValue2;
    acp_bool_t   sNull1;
    acp_bool_t   sNull2;

    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    MTC_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );

    sNull1 = ( ( *(acp_uint32_t*)sValue1 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // value2
    //---------

    sValue2 = &sRealValue2;
    
    MTC_FLOAT_BYTE_ASSIGN( sValue2, aValueInfo2->value );
    
    sNull2 = ( ( *(acp_uint32_t*)sValue2 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        if( *sValue1 > *sValue2 )
        {
            return 1;
        }
        if( *sValue1 < *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( (sNull1 == ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ACP_TRUE) /*&& (sNull2 == ACP_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdRealStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                mtdValueInfo* aValueInfo2 )
{
    /***********************************************************************
     *
     * Description : Stored Key들 간의 descending compare
     *
     * Implementation :
     *
     ***********************************************************************/

    mtdRealType  sRealValue1;
    mtdRealType* sValue1;
    mtdRealType  sRealValue2;
    mtdRealType* sValue2;
    acp_bool_t   sNull1;
    acp_bool_t   sNull2;

    //---------
    // value1
    //---------

    sValue1 = &sRealValue1;
    
    MTC_FLOAT_BYTE_ASSIGN( sValue1, aValueInfo1->value );
    
    sNull1 = ( ( *(acp_uint32_t*)sValue1 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // value2
    //---------

    sValue2 = &sRealValue2;
    
    MTC_FLOAT_BYTE_ASSIGN( sValue2, aValueInfo2->value );
    
    sNull2 = ( ( *(acp_uint32_t*)sValue2 & MTD_REAL_EXPONENT_MASK )
               == MTD_REAL_EXPONENT_MASK ) ? ACP_TRUE : ACP_FALSE ;

    //---------
    // compare
    //---------
    
    if( (sNull1 != ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        if( *sValue1 < *sValue2 )
        {
            return 1;
        }
        if( *sValue1 > *sValue2 )
        {
            return -1;
        }
        return 0;
    }
    
    if( (sNull1 == ACP_TRUE) && (sNull2 != ACP_TRUE) )
    {
        return 1;
    }
    if( (sNull1 != ACP_TRUE) /*&& (sNull2 == ACP_TRUE)*/ )
    {
        return -1;
    }
    return 0;
}

void mtdEndian( void* aValue )
{
    acp_uint32_t sCount;
    acp_uint8_t* sValue;
    acp_uint8_t  sBuffer[MTD_REAL_SIZE];
    
    sValue = (acp_uint8_t*)aValue;
    for( sCount = 0; sCount < MTD_REAL_SIZE; sCount++ )
    {
        sBuffer[MTD_REAL_SIZE-sCount-1] = sValue[sCount];
    }
    for( sCount = 0; sCount < MTD_REAL_SIZE; sCount++ )
    {
        sValue[sCount] = sBuffer[sCount];
    }
}


ACI_RC mtdValidate( mtcColumn*     aColumn,
                    void*          aValue,
                    acp_uint32_t   aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
        
    ACI_TEST_RAISE( aValue == NULL, ERR_INVALID_NULL );
    
    ACI_TEST_RAISE( aValueSize != sizeof(mtdRealType), ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdReal,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_INVALID_NULL);
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_VALUE);
    }
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    {
        aciSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_double_t mtdSelectivityReal( void* aColumnMax, 
                                 void* aColumnMin, 
                                 void* aValueMax,  
                                 void* aValueMin )
{
/***********************************************************************
 *
 * Description :
 *    REAL 의 Selectivity 추출 함수
 *
 * Implementation :
 *
 *    Selectivity = (aValueMax - aValueMin) / (aColumnMax - aColumnMin)
 *    0 < Selectivity <= 1 의 값을 리턴함
 *
 ***********************************************************************/
    
    mtdRealType  * sColumnMax;
    mtdRealType  * sColumnMin;
    mtdRealType  * sValueMax;
    mtdRealType  * sValueMin;
    mtdRealType  * sUIntPtr;
    acp_double_t   sSelectivity;
    acp_float_t    sSelectivityReal;
    acp_double_t   sDenominator;  // 분모값
    acp_double_t   sNumerator;    // 분자값

    mtdValueInfo   sValueInfo1;
    mtdValueInfo   sValueInfo2;
    mtdValueInfo   sValueInfo3;
    mtdValueInfo   sValueInfo4;

    sColumnMax = (mtdRealType*) aColumnMax;
    sColumnMin = (mtdRealType*) aColumnMin;
    sValueMax  = (mtdRealType*) aValueMax;
    sValueMin  = (mtdRealType*) aValueMin;

    //------------------------------------------------------
    // Data의 유효성 검사
    //     NULL 검사 : 계산할 수 없음
    //------------------------------------------------------
    
    if ( ( mtdIsNull( NULL, aColumnMax, MTD_OFFSET_USELESS ) == ACP_TRUE  ) ||
         ( mtdIsNull( NULL, aColumnMin, MTD_OFFSET_USELESS ) == ACP_TRUE ) ||
         ( mtdIsNull( NULL, aValueMax, MTD_OFFSET_USELESS ) == ACP_TRUE )  ||
         ( mtdIsNull( NULL, aValueMin, MTD_OFFSET_USELESS ) == ACP_TRUE ) )
    {
        // Data중 NULL 이 있을 경우
        // 부등호의 Default Selectivity인 1/3을 Setting함
        sSelectivity = MTD_DEFAULT_SELECTIVITY;
    }
    else
    {
        //------------------------------------------------------
        // 유효성 검사
        // 다음의 경우는 조건을 잘못된 통계 정보이거나 입력 정보임.
        // Column의 Min값보다 Value의 Max값이 작은 경우
        // Column의 Max값보다 Value의 Min값이 큰 경우
        //------------------------------------------------------

        sValueInfo1.column = NULL;
        sValueInfo1.value  = sColumnMin;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = sValueMax;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sValueInfo3.column = NULL;
        sValueInfo3.value  = sColumnMax;
        sValueInfo3.flag   = MTD_OFFSET_USELESS;

        sValueInfo4.column = NULL;
        sValueInfo4.value  = sValueMin;
        sValueInfo4.flag   = MTD_OFFSET_USELESS;

        if ( ( mtdRealLogicalComp( &sValueInfo1,
                                   &sValueInfo2 ) > 0 )
             ||
             ( mtdRealLogicalComp( &sValueInfo3,
                                   &sValueInfo4 ) < 0 ) )
        {
            // To Fix PR-11858
            sSelectivity = 0;
        }
        else
        {
            //------------------------------------------------------
            // Value값 보정
            // Value의 Min값이 Column의 Min값보다 작다면 보정
            // Value의 Max값이 Column의 Max값보다 크다면 보정
            //------------------------------------------------------

            sValueInfo1.column = NULL;
            sValueInfo1.value  = sValueMin;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = sColumnMin;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            
            if ( mtdRealLogicalComp( &sValueInfo1,
                                     &sValueInfo2 ) < 0 )
            {
                sValueMin = sColumnMin;
            }
            else
            {
                // Nothing To Do
            }

            sValueInfo1.column = NULL;
            sValueInfo1.value  = sValueMax;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = sColumnMax;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( mtdRealLogicalComp( &sValueInfo1,
                                     &sValueInfo2 ) > 0 )
            {
                sValueMax = sColumnMax;
            }
            else
            {
                // Nothing To Do
            }
            
            //------------------------------------------------------
            // 분모값 (aColumnMax - aColumnMin) 값 획득
            //------------------------------------------------------
        
            sDenominator = (acp_double_t)(*sColumnMax - *sColumnMin);
    
            if ( sDenominator <= 0.0 )
            {
                // 잘못된 통계 정보의 사용한 경우
                sSelectivity = MTD_DEFAULT_SELECTIVITY;
            }
            else
            {
                //------------------------------------------------------
                // 분자값 (aValueMax - aValueMin) 값 획득
                //------------------------------------------------------
            
                sNumerator = (acp_double_t) (*sValueMax - *sValueMin);
                sSelectivity = sNumerator / sDenominator;
                sSelectivityReal = (acp_float_t)sSelectivity;
                sUIntPtr = &sSelectivityReal;
                
                // jhseong, NaN check, PR-9195
                if ( sNumerator <= 0.0 ||
                     (((*(acp_uint32_t*)sUIntPtr)&MTD_REAL_EXPONENT_MASK)==(MTD_REAL_EXPONENT_MASK)) )
                {
                    // 잘못된 입력 정보인 경우
                    // To Fix PR-11858
                    sSelectivity = 0;
                }
                else
                {
                    //------------------------------------------------------
                    // Selectivity 계산
                    //------------------------------------------------------
                
                    if ( sSelectivity > 1.0 )
                    {
                        sSelectivity = 1.0;
                    }
                }
            }
        }
    }
    
    return sSelectivity;
}

ACI_RC mtdEncode( mtcColumn*    aColumn,
                  void*         aValue,
                  acp_uint32_t  aValueSize,
                  acp_uint8_t*  aCompileFmt,
                  acp_uint32_t  aCompileFmtLen,
                  acp_uint8_t*  aText,
                  acp_uint32_t* aTextLen,
                  ACI_RC*       aRet )
{
    mtdRealType  sReal;
    acp_uint32_t sStringLen;

    //----------------------------------
    // Parameter Validation
    //----------------------------------

    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValueSize);
    ACP_UNUSED(aCompileFmt);
    ACP_UNUSED(aCompileFmtLen);
    
    ACE_ASSERT( aValue != NULL );
    ACE_ASSERT( aText != NULL );
    ACE_ASSERT( *aTextLen > 0 );
    ACE_ASSERT( aRet != NULL );
    
    //----------------------------------
    // Initialization
    //----------------------------------

    aText[0] = '\0';
    sStringLen = 0;

    //----------------------------------
    // Set String
    //----------------------------------
    
    // To Fix BUG-16801
    if ( mtdIsNull( NULL, aValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sReal = *(mtdRealType*)aValue;
    
        // BUG-17025
        if ( sReal == (mtdRealType)0 )
        {
            sReal = (mtdRealType)0;
        }
        else
        {
            // Nothing to do.
        }

        sStringLen = aciVaAppendFormat( (acp_char_t*) aText,
                                        (acp_size_t) aTextLen,
                                        "%g",
                                        sReal);

    }

    //----------------------------------
    // Finalization
    //----------------------------------

    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;

    *aRet = ACI_SUCCESS;

    return ACI_SUCCESS;
}

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t   aColumnSize,
                                       void*          aDestVale,
                                       acp_uint32_t   aDestValueOffset ,
                                       acp_uint32_t   aLength,
                                       const void*    aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdRealType  * sRealValue;

    ACP_UNUSED(aDestValueOffset);
    
    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다. 

    sRealValue = (mtdRealType*) aDestVale;    
    
    if( aLength == 0 )
    {
        // NULL 데이타
        acpMemCpy( sRealValue, &mtcdRealNull, MTD_REAL_SIZE );        
    }
    else
    {
        ACI_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );        

        acpMemCpy( sRealValue, aValue, aLength );        
    }
    
    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


acp_uint32_t mtdNullValueSize()
{
/*******************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환    
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtcdRealNull,
                          MTD_OFFSET_USELESS );   
}


