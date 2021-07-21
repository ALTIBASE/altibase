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
 * $Id: mtcdBlobLocator.cpp 16552 2006-06-07 02:29:08Z mhjeong $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>
#include <mtcdTypes.h>

extern mtdModule mtcdBlobLocator;

#define MTD_BLOB_LOCATOR_ALIGN             ( (acp_sint32_t) sizeof(mtdBlobLocatorType) )
#define MTD_BLOB_LOCATOR_PRECISION_DEFAULT (1)


static mtdBlobLocatorType mtdBlobLocatorNull = MTD_BLOB_LOCATOR_NULL;

static ACI_RC mtdInitializeBlobLocator( acp_uint32_t aNo );

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

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn *  aColumn,
                           void      *  aValue,
                           acp_uint32_t aValueSize);


static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                        void            * aDestValue,
                                        acp_uint32_t      aDestValueOffset,
                                        acp_uint32_t      aLength,
                                        const void      * aValue );

static mtcName mtdTypeName[1] = {
    { NULL, 12, (void*)"BLOB_LOCATOR" }
};

static mtcColumn mtdColumn;

mtdModule mtcdBlobLocator = {
    mtdTypeName,
    &mtdColumn,
    MTD_BLOB_LOCATOR_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_BLOB_LOCATOR_ALIGN,
    MTD_GROUP_MISC|
    MTD_CANON_NEEDLESS|
    MTD_CREATE_DISABLE|
    MTD_COLUMN_TYPE_FIXED|
    MTD_SELECTIVITY_DISABLE|
    MTD_VARIABLE_LENGTH_TYPE_FALSE| // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE| // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,  // PROJ-1705
    0,
    0,
    0,
    &mtdBlobLocatorNull,
    mtdInitializeBlobLocator,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    NULL,           // Logical Comparison
    {
        // Key Comparison
        {
            NULL,         // Ascending Key Comparison
            NULL          // Descending Key Comparison
        }
        ,
        {
            NULL,         // Ascending Key Comparison
            NULL          // Descending Key Comparison
        }
        ,
        {
            NULL,         // Ascending Key Comparison
            NULL          // Descending Key Comparison
        }
    },
    mtdCanonizeDefault,
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
    mtdNullValueSizeNA,
    mtdHeaderSizeDefault
};

ACI_RC mtdInitializeBlobLocator( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdBlobLocator, aNo ) != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdBlobLocator,
                                   0,   // arguments
                                   0,   // precision
                                   0 )  // scale
              != ACI_SUCCESS );
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdEstimate( acp_uint32_t * aColumnSize,
                    acp_uint32_t * aArguments,
                    acp_sint32_t * aPrecision,
                    acp_sint32_t * aScale )
{
    ACP_UNUSED(aPrecision);
    ACP_UNUSED(aScale);
    
    ACI_TEST_RAISE( *aArguments != 0, ERR_INVALID_PRECISION );
    
    *aColumnSize = sizeof(mtdBlobLocatorType);
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_PRECISION );
    aciSetErrorCode(mtERR_ABORT_INVALID_PRECISION);
    
    ACI_EXCEPTION_END;
    
    return ACI_FAILURE;
}

ACI_RC mtdValue( mtcTemplate*  aTemp,
                 mtcColumn*    aTemp2,
                 void*         aTemp3,
                 acp_uint32_t* aTemp4,
                 acp_uint32_t  aTemp5,
                 const void*   aTemp6,
                 acp_uint32_t  aTemp7,
                 ACI_RC*       aResult )
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    ACP_UNUSED(aTemp4);
    ACP_UNUSED(aTemp5);
    ACP_UNUSED(aTemp6);
    ACP_UNUSED(aTemp7);

    *aResult = ACI_FAILURE;

    return ACI_SUCCESS;
}

acp_uint32_t mtdActualSize( const mtcColumn* aTemp,
                            const void*      aTemp2,
                            acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);

    return sizeof(mtdBlobLocatorType);
}

void mtdNull( const mtcColumn* aColumn,
              void*            aRow,
              acp_uint32_t     aFlag )
{
    mtdBlobLocatorType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (mtdBlobLocatorType*)
        mtdValueForModule( NULL,
                           aRow, 
                           aFlag,
                           NULL );

    if ( sValue != NULL )
    {
        *sValue = MTD_BLOB_LOCATOR_NULL;
    }
}

acp_uint32_t mtdHash( acp_uint32_t     aTemp,
                      const mtcColumn* aTemp2,
                      const void*      aTemp3,
                      acp_uint32_t     aTemp4)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    ACP_UNUSED(aTemp4);
    
    return 0;
}

acp_bool_t mtdIsNull( const mtcColumn* aColumn,
                      const void*      aRow,
                      acp_uint32_t     aFlag )
{
    const mtdBlobLocatorType* sValue;

    ACP_UNUSED( aColumn);
    
    sValue = (const mtdBlobLocatorType*)
        mtdValueForModule( NULL,
                           aRow,
                           aFlag,
                           mtcdBlobLocator.staticNull );
    
    return ( *sValue == MTD_BLOB_LOCATOR_NULL ) ? ACP_TRUE : ACP_FALSE;
}

void mtdEndian( void* aValue )
{
    acp_uint8_t* sLength;
    acp_uint8_t  sIntermediate;

    sLength = (acp_uint8_t*)(mtdBlobLocatorType*)aValue;  // smLobLocaotr is ULong
    sIntermediate = sLength[0];
    sLength[0]    = sLength[7];
    sLength[7]    = sIntermediate;
    sIntermediate = sLength[1];
    sLength[1]    = sLength[6];
    sLength[6]    = sIntermediate;
    sIntermediate = sLength[2];
    sLength[2]    = sLength[5];
    sLength[5]    = sIntermediate;
    sIntermediate = sLength[3];
    sLength[3]    = sLength[4];
    sLength[4]    = sIntermediate;
}

ACI_RC mtdValidate( mtcColumn    * aColumn,
                    void         * aValue,
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

    ACI_TEST_RAISE( aValueSize != sizeof(mtdBlobLocatorType),
                    ERR_INVALID_LENGTH );

    // 초기화된 aColumn은 cannonize() 시에 사용
    // 이때, data type module의 precision 정보만을 사용하므로,
    // language 정보 설정할 필요없음
    ACI_TEST( mtcInitializeColumn( aColumn,
                                   & mtcdBlobLocator,
                                   0,     // arguments
                                   0,     // precision
                                   0 )    // scale
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

static ACI_RC mtdStoredValue2MtdValue( acp_uint32_t      aColumnSize,
                                       void            * aDestValue,
                                       acp_uint32_t      aDestValueOffset ,
                                       acp_uint32_t      aLength,
                                       const void      * aValue )
{
/*******************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *******************************************************************/

    mtdBlobLocatorType* sValue;

    ACP_UNUSED(aDestValueOffset);
    
    // 고정길이 데이타 타입의 경우
    // 하나의 컬럼 데이타가 여러페이지에 나누어 저장되는 경우는 없다.

    sValue = (mtdBlobLocatorType*)aDestValue;

    if( aLength == 0 )
    {
        // NULL 데이타
        *sValue = MTD_LOCATOR_NULL;
    }
    else
    {
        ACI_TEST_RAISE( aLength != aColumnSize, ERR_INVALID_STORED_VALUE );

        acpMemCpy( sValue, aValue, aLength );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_INVALID_STORED_VALUE);
    {
        aciSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

