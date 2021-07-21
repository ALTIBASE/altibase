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
 * $Id: mtcdVarbit.cpp 17938 2006-09-05 00:54:03Z leekmo $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdVarbit;
extern mtdModule mtcdBit;

// To Remove Warning
const mtdBitType mtdVarbitNull = { 0, {'\0',} };

static ACI_RC mtdInitializeVarbit( acp_uint32_t aNo );

static ACI_RC mtdEstimate( acp_uint32_t * aColumnSize,
                           acp_uint32_t * aArguments,
                           acp_sint32_t * aPrecision,
                           acp_sint32_t * aScale );

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

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale );

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

static acp_sint32_t mtdVarbitLogicalComp( mtdValueInfo * aValueInfo1,
                                          mtdValueInfo * aValueInfo2 );

static acp_sint32_t mtdVarbitMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                               mtdValueInfo * aValueInfo2 );

static acp_sint32_t mtdVarbitMtdMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                mtdValueInfo * aValueInfo2 );

static acp_sint32_t mtdVarbitStoredMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                                                  mtdValueInfo * aValueInfo2 );

static acp_sint32_t mtdVarbitStoredMtdKeyDescComp( mtdValueInfo * aValueInfo1,
                                                   mtdValueInfo * aValueInfo2 );

static acp_sint32_t mtdVarbitStoredStoredKeyAscComp( mtdValueInfo * aValueInfo1,
                                                     mtdValueInfo * aValueInfo2 );

static acp_sint32_t mtdVarbitStoredStoredKeyDescComp( mtdValueInfo * aValueInfo1,
                                                      mtdValueInfo * aValueInfo2 );

static ACI_RC mtdCanonize( const mtcColumn * aCanon,
                           void**            aCanonizedValue,
                           mtcEncryptInfo  * aCanonInfo,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * aColumnInfo,
                           mtcTemplate     * aTemplate );

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue );

static acp_uint32_t mtdNullValueSize();

static acp_uint32_t mtdHeaderSize();

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"VARBIT" }
};

static mtcColumn mtdColumn;

mtdModule mtcdVarbit = {
    mtdTypeName,
    &mtdColumn,
    MTD_VARBIT_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    MTD_VARBIT_ALIGN,
    MTD_GROUP_TEXT|
    MTD_CANON_NEED|
    MTD_CREATE_ENABLE|
    MTD_COLUMN_TYPE_VARIABLE|
    MTD_SELECTIVITY_DISABLE|
    MTD_CREATE_PARAM_PRECISION|
    MTD_SEARCHABLE_SEARCHABLE|    // BUG-17020
    MTD_LITERAL_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|  // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_TRUE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_TRUE,  // PROJ-1705
    128000,
    0,
    0,
    (void*)&mtdVarbitNull,
    mtdInitializeVarbit,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecision,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    mtdVarbitLogicalComp,     // Logical Comparison
    {
        // Key Comparison
        {
            // mt value들 간의 compare 
            mtdVarbitMtdMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbitMtdMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // mt value와 stored value간의 compare 
            mtdVarbitStoredMtdKeyAscComp, // Ascending Key Comparison
            mtdVarbitStoredMtdKeyDescComp // Descending Key Comparison
        }
        ,
        {
            // stored value들 간의 compare 
            mtdVarbitStoredStoredKeyAscComp, // Ascending Key Comparison
            mtdVarbitStoredStoredKeyDescComp // Descending Key Comparison
        }
    },
    mtdCanonize,
    mtdEndian,
    mtdValidate,
    mtdSelectivityNA,
    mtdEncodeNA,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValue,
    mtdNullValueSize,
    mtdHeaderSize
};

ACI_RC mtdInitializeVarbit( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdVarbit, aNo ) != ACI_SUCCESS );
    
    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdVarbit,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdEstimate( acp_uint32_t* aColumnSize,
                    acp_uint32_t* aArguments,
                    acp_sint32_t* aPrecision,
                    acp_sint32_t* aScale )
{
    ACP_UNUSED(aScale);
    
    if( *aArguments == 0 )
    {
        *aArguments = 1;
        *aPrecision = MTD_VARBIT_PRECISION_DEFAULT;
    }

    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_SCALE );

    ACI_TEST_RAISE( *aPrecision < MTD_VARBIT_PRECISION_MINIMUM ||
                    *aPrecision > MTD_VARBIT_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    *aColumnSize = sizeof(acp_uint32_t) + BIT_TO_BYTE(*aPrecision);
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);

    ACI_EXCEPTION( ERR_INVALID_SCALE );
    aciSetErrorCode(mtERR_ABORT_INVALID_SCALE);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*  aTemplate ,
                 mtcColumn*    aColumn,
                 void*         aValue,
                 acp_uint32_t* aValueOffset,
                 acp_uint32_t  aValueSize,
                 const void*   aToken,
                 acp_uint32_t  aTokenLength,
                 ACI_RC*       aResult )
{
    acp_uint32_t       sValueOffset;
    mtdBitType*        sValue;
    const acp_uint8_t* sToken;
    const acp_uint8_t* sTokenFence;
    acp_uint8_t*       sIterator;
    acp_uint32_t       sSubIndex;

    static const acp_uint8_t sBinary[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        0,  1, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

    ACP_UNUSED(aTemplate);
    
    sValueOffset = ACP_ALIGN_ALL( *aValueOffset, MTD_VARBIT_ALIGN );
    
    sValue = (mtdBitType*)( (acp_uint8_t*)aValue + sValueOffset );
    
    *aResult = ACI_SUCCESS;
    
    sIterator = sValue->value;
    
    if( BIT_TO_BYTE(aTokenLength) <= (acp_uint8_t*)aValue - sIterator + aValueSize )
    {
        if( aTokenLength == 0 )
        {
            sValue->length = 0;
        }
        else
        {
            sValue->length = 0;
            
            for( sToken      = (const acp_uint8_t*)aToken,
                     sTokenFence = sToken + aTokenLength;
                 sToken      < sTokenFence;
                 sIterator++, sToken += 8 )
            {
                ACI_TEST_RAISE( sBinary[sToken[0]] > 1, ERR_INVALID_LITERAL );

                // 값 넣기 전에 0으로 초기화
                acpMemSet( sIterator,
                           0x00,
                           1 );

                *sIterator = sBinary[sToken[0]] << 7;

                sValue->length++;
                ACI_TEST_RAISE( sValue->length == 0,
                                ERR_INVALID_LITERAL );

                sSubIndex = 1;
                while( sToken + sSubIndex < sTokenFence && sSubIndex < 8 )
                {
                    ACI_TEST_RAISE( sBinary[sToken[sSubIndex]] > 1, ERR_INVALID_LITERAL );
                    *sIterator |= ( sBinary[sToken[sSubIndex]] << ( 7 - sSubIndex ) );
                    sValue->length++;
                    sSubIndex++;
                }
            }
        }

        // precision, scale 재 설정 후, estimate로 semantic 검사
        aColumn->flag            = 1;
        aColumn->precision       = sValue->length != 0 ? sValue->length : 1;
        aColumn->scale           = 0;
        
        ACI_TEST( mtdEstimate( & aColumn->column.size,
                               & aColumn->flag,
                               & aColumn->precision,
                               & aColumn->scale )
                  != ACI_SUCCESS );
        
        aColumn->column.offset   = sValueOffset;
        *aValueOffset            = sValueOffset +
            sizeof(acp_uint32_t) + BIT_TO_BYTE(sValue->length);
    }
    else
    {
        *aResult = ACI_FAILURE;
    }
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LITERAL );
    aciSetErrorCode(mtERR_ABORT_INVALID_LITERAL);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aRow,
                            acp_uint32_t     aFlag )
{
    const mtdBitType* sValue;
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdVarbit.staticNull );

    if( mtcdVarbit.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        return sizeof(acp_uint32_t);
    }
    else
    {
        return sizeof(acp_uint32_t) + BIT_TO_BYTE(sValue->length);
    }
}

static ACI_RC mtdGetPrecision( const mtcColumn* aColumn,
                               const void*      aRow,
                               acp_uint32_t     aFlag,
                               acp_sint32_t*    aPrecision,
                               acp_sint32_t*    aScale )
{
    const mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdBit.staticNull );

    *aPrecision = sValue->length;
    *aScale = 0;

    return ACI_SUCCESS;
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           NULL );

    if( sValue != NULL )
    {
        sValue->length = 0;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdVarbit.staticNull );

    if( mtcdVarbit.isNull( aColumn, sValue, MTD_OFFSET_USELESS ) == ACP_TRUE )
    {
        return aHash;
    }
    
    return mtcHashSkip( aHash, sValue->value, BIT_TO_BYTE(sValue->length) );
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBitType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdVarbit.staticNull );

    return ( sValue->length == 0 ) ? ACP_TRUE : ACP_FALSE;
}

acp_sint32_t
mtdVarbitLogicalComp( mtdValueInfo * aValueInfo1,
                      mtdValueInfo * aValueInfo2 )
{
    return mtdVarbitMtdMtdKeyAscComp( aValueInfo1,
                                      aValueInfo2 );
}

acp_sint32_t
mtdVarbitMtdMtdKeyAscComp( mtdValueInfo * aValueInfo1,
                           mtdValueInfo * aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*  sVarbitValue1;
    const mtdBitType*  sVarbitValue2;
    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    //---------
    // value1
    //---------
    sVarbitValue1 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdVarbit.staticNull );
    
    sLength1 = sVarbitValue1->length;

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;
    
    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return acpMemCmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return acpMemCmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue1, sValue2,
                          BIT_TO_BYTE(sLength1) );
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;

}

acp_sint32_t
mtdVarbitMtdMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                            mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*  sVarbitValue1;
    const mtdBitType*  sVarbitValue2;
    acp_uint32_t       sLength1;
    acp_uint32_t       sLength2;
    const acp_uint8_t* sValue1;
    const acp_uint8_t* sValue2;

    //---------
    // value1
    //---------        
    sVarbitValue1 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo1->value,
                           aValueInfo1->flag,
                           mtcdVarbit.staticNull );
    
    sLength1 = sVarbitValue1->length;

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;
    
    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        sValue1  = sVarbitValue1->value;
        sValue2  = sVarbitValue2->value;
        
        if( sLength2 > sLength1 )
        {
            return acpMemCmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return acpMemCmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue2, sValue1,
                          BIT_TO_BYTE(sLength2) );
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdVarbitStoredMtdKeyAscComp( mtdValueInfo* aValueInfo1,
                              mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*          sVarbitValue2;
    acp_uint32_t               sLength1;
    acp_uint32_t               sLength2;
    const acp_uint8_t*         sValue1;
    const acp_uint8_t*         sValue2;

    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------
    sVarbitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;
    
    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        // length가 0이 아닌 경우(즉, NULL value가 아닌 경우)에 
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();
        
        sValue2  = sVarbitValue2->value;
        
        if( sLength1 > sLength2 )
        {
            return acpMemCmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return acpMemCmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue1, sValue2,
                          BIT_TO_BYTE(sLength1) );
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;

}

acp_sint32_t
mtdVarbitStoredMtdKeyDescComp( mtdValueInfo* aValueInfo1,
                               mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Mtd 타입의 Key와 Stored Key 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    const mtdBitType*      sVarbitValue2;
    acp_uint32_t           sLength1;
    acp_uint32_t           sLength2;
    const acp_uint8_t*     sValue1;
    const acp_uint8_t*     sValue2;
    
    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------        
    sVarbitValue2 = (const mtdBitType*)
        mtdValueForModule( NULL,
                           aValueInfo2->value,
                           aValueInfo2->flag,
                           mtcdVarbit.staticNull );
    
    sLength2 = sVarbitValue2->length;
    
    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        // length가 0이 아닌 경우(즉, NULL value가 아닌 경우)에 
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();
        
        sValue2  = sVarbitValue2->value;
        
        if( sLength2 > sLength1 )
        {
            return acpMemCmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return acpMemCmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue2, sValue1,
                          BIT_TO_BYTE(sLength2) );
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

acp_sint32_t
mtdVarbitStoredStoredKeyAscComp( mtdValueInfo* aValueInfo1,
                                 mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 ascending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint32_t               sLength1;
    acp_uint32_t               sLength2;
    const acp_uint8_t*         sValue1;
    const acp_uint8_t*         sValue2;

    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------

    sLength2 = aValueInfo2->length;

    //---------
    // compare
    //---------

    if( sLength1 != 0 && sLength2 != 0 )
    {
        // length가 0이 아닌 경우(즉, NULL value가 아닌 경우)에 
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 )
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();

        MTC_INT_BYTE_ASSIGN( &sLength2, aValueInfo2->value );
        sValue2  = ((acp_uint8_t*)aValueInfo2->value) + mtdHeaderSize();

        if( sLength1 > sLength2 )
        {
            return acpMemCmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength2) ) >= 0 ? 1 : -1 ;
        }
        if( sLength1 < sLength2 )
        {
            return acpMemCmp( sValue1, sValue2,
                              BIT_TO_BYTE(sLength1) ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue1, sValue2,
                          BIT_TO_BYTE(sLength1) );
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;

}

acp_sint32_t
mtdVarbitStoredStoredKeyDescComp( mtdValueInfo* aValueInfo1,
                                  mtdValueInfo* aValueInfo2 )
{
/***********************************************************************
 *
 * Description : Stored Key들 간의 descending compare
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_uint32_t             sLength1;
    acp_uint32_t             sLength2;
    const acp_uint8_t*       sValue1;
    const acp_uint8_t*       sValue2;

    //---------
    // value1
    //---------

    sLength1 = aValueInfo1->length;

    //---------
    // value2
    //---------

    sLength2 = aValueInfo2->length;

    //---------
    // compare
    //---------
    
    if( sLength1 != 0 && sLength2 != 0 )
    {
        // length가 0이 아닌 경우(즉, NULL value가 아닌 경우)에 
        // aValueInfo1->value에는 varbit value가 저장되어 있음
        // ( varbit value는 (length + value) 이고,
        //   NULL value가 아닌 경우에 length 정보를 읽어올 수 있음 ) 
        MTC_INT_BYTE_ASSIGN( &sLength1, aValueInfo1->value );
        sValue1  = ((acp_uint8_t*)aValueInfo1->value) + mtdHeaderSize();

        MTC_INT_BYTE_ASSIGN( &sLength2, aValueInfo2->value );
        sValue2  = ((acp_uint8_t*)aValueInfo2->value) + mtdHeaderSize();
        
        if( sLength2 > sLength1 )
        {
            return acpMemCmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength1) ) >= 0 ? 1 : -1 ;
        }
        if( sLength2 < sLength1 )
        {
            return acpMemCmp( sValue2, sValue1,
                              BIT_TO_BYTE(sLength2) ) > 0 ? 1 : -1 ;
        }
        return acpMemCmp( sValue2, sValue1,
                          BIT_TO_BYTE(sLength2) );
    }

    if( sLength1 < sLength2 )
    {
        return 1;
    }
    if( sLength1 > sLength2 )
    {
        return -1;
    }
    return 0;
}

static ACI_RC mtdCanonize( const mtcColumn* aCanon,
                           void**           aCanonizedValue,
                           mtcEncryptInfo*  aCanonInfo ,
                           const mtcColumn* aColumn,
                           void*            aValue,
                           mtcEncryptInfo*  aColumnInfo ,
                           mtcTemplate*     aTemplate )
{
    ACP_UNUSED(aCanonInfo);
    ACP_UNUSED(aColumnInfo);
    ACP_UNUSED(aTemplate);
    
    if( mtcdVarbit.isNull( aColumn, aValue, MTD_OFFSET_USELESS ) != ACP_TRUE )
    {
        ACI_TEST_RAISE( ((mtdBitType*)aValue)->length > (acp_uint32_t)aCanon->precision,
                        ERR_INVALID_LENGTH );
    }
    
    *aCanonizedValue = aValue;
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;

    sLength       = (acp_uint8_t*)&(((mtdBitType*)aValue)->length);
    sIntermediate = sLength[0];
    sLength[0]    = sLength[3];
    sLength[3]    = sIntermediate;
    sIntermediate = sLength[1];
    sLength[1]    = sLength[2];
    sLength[2]    = sIntermediate;
}

ACI_RC mtdValidate( mtcColumn*   aColumn,
                    void*        aValue,
                    acp_uint32_t aValueSize)
{
/***********************************************************************
 *
 * Description : value의 semantic 검사 및 mtcColum 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    mtdBitType * sVal = (mtdBitType *)aValue;
    
    ACI_TEST_RAISE( sVal == NULL, ERR_INVALID_NULL);

    ACI_TEST_RAISE( aValueSize == 0, ERR_INVALID_LENGTH );
    ACI_TEST_RAISE( BIT_TO_BYTE(sVal->length) + sizeof(acp_uint32_t) != aValueSize,
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음 
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdVarbit,
                                   1,              // arguments
                                   sVal->length,   // precision
                                   0 )             // scale
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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t aColumnSize,
                                       void*        aDestValue,
                                       acp_uint32_t aDestValueOffset,
                                       acp_uint32_t aLength,
                                       const void*  aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdBitType  * sVarbitValue;

    sVarbitValue = (mtdBitType*)aDestValue;
    
    if( ( aDestValueOffset == 0 ) && ( aLength == 0 ) )
    {
        // NULL 데이타
        sVarbitValue->length = 0;
    }
    else
    {
        // bit type은 mtdDataType형태로 저장되므로
        // length 에 header size가 포함되어 있어 
        // 여기서 별도의 계산을 하지 않아도 된다. 
        ACI_TEST_RAISE( (aDestValueOffset + aLength) > aColumnSize, ERR_INVALID_STORED_VALUE );

        acpMemCpy( (acp_uint8_t*)sVarbitValue + aDestValueOffset, aValue, aLength );
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
 * 예 ) mtdBitType( acp_uint32_t length; acp_uint8_t value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *******************************************************************/

    return mtdActualSize( NULL,
                          & mtdVarbitNull,
                          MTD_OFFSET_USELESS );
}

static acp_uint32_t mtdHeaderSize()
{
/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예 ) mtdBitType( acp_uint32_t length; acp_uint8_t value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/

    return sizeof(acp_uint32_t);
}

