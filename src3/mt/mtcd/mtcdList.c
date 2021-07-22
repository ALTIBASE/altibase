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
 * $Id: mtcdList.cpp 35486 2009-09-10 00:26:55Z elcarim $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

#define MTD_LIST_ALIGN (sizeof(acp_uint64_t))

extern mtdModule mtcdList;

static ACI_RC mtdInitializeList( acp_uint32_t aNo );

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

static void mtdEndian( void* aValue );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);

static mtcName mtdTypeName[1] = {
    { NULL, 4, (void*)"LIST" },
};

static mtcColumn mtdColumn;

mtdModule mtcdList = {
    mtdTypeName,
    &mtdColumn,
    MTD_LIST_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_LIST_ALIGN,
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
    NULL, // staticNull : List type cannot have static null.
    mtdInitializeList,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtdGetPrecisionNA,
    mtdNull,
    mtdHash,
    mtdIsNull,
    mtdIsTrueNA,
    NULL,           // Logical Comparison
    {                         // Key Comparison
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
    NULL,
    NULL,
    NULL,
    mtdValueFromOracleDefault,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValueNA, 
    mtdNullValueSizeNA,
    mtdHeaderSizeNA
};

ACI_RC mtdInitializeList( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdList, aNo ) != ACI_SUCCESS );

    // mtdColumn의 초기화
    ACI_TEST( mtcInitializeColumn( & mtdColumn,
                                   & mtcdList,
                                   1,   // arguments
                                   1,   // precision
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
    ACI_TEST_RAISE( *aArguments != 1, ERR_INVALID_LENGTH );
    ACP_UNUSED(aScale);
    
    *aColumnSize = sizeof(mtcStack) * (*aPrecision);
    
    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_INVALID_LENGTH );
    aciSetErrorCode(mtERR_ABORT_INVALID_LENGTH);
    
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
                 ACI_RC*       aTemp8)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
    ACP_UNUSED(aTemp4);
    ACP_UNUSED(aTemp5);
    ACP_UNUSED(aTemp6);
    ACP_UNUSED(aTemp7);
    ACP_UNUSED(aTemp8);

    aciSetErrorCode(mtERR_ABORT_NOT_APPLICABLE);

    return ACI_FAILURE;
}

acp_uint32_t mtdActualSize( const mtcColumn* aColumn,
                            const void*      aTemp,
                            acp_uint32_t     aTemp2)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);

    return aColumn->column.size;
}

void mtdNull( const mtcColumn* aTemp,
              void* aTemp2,
              acp_uint32_t aTemp3 )
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);
}

acp_uint32_t mtdHash( acp_uint32_t     aHash,
                      const mtcColumn* aTemp,
                      const void*      aTemp2,
                      acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);

    return aHash;
}

acp_bool_t mtdIsNull( const mtcColumn* aTemp,
                      const void*      aTemp2,
                      acp_uint32_t     aTemp3)
{
    ACP_UNUSED(aTemp);
    ACP_UNUSED(aTemp2);
    ACP_UNUSED(aTemp3);

    return ACP_FALSE;
}

void mtdEndian( void* aTemp)
{
    ACP_UNUSED(aTemp);
}


ACI_RC mtdValidate( mtcColumn*           aColumn,
                    void*                aValue,
                    acp_uint32_t         aValueSize )
{
    ACP_UNUSED(aColumn);
    ACP_UNUSED(aValue);
    ACP_UNUSED(aValueSize);
    
    aciSetErrorCode(mtERR_ABORT_NOT_APPLICABLE);
    
    return ACI_FAILURE;
}

