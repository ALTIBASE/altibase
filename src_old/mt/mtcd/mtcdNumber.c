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
 * $Id: mtcdNumber.cpp 36231 2009-10-22 04:07:06Z kumdory $
 **********************************************************************/

#include <mtce.h>
#include <mtcc.h>
#include <mtcd.h>
#include <mtcl.h>

#include <mtcdTypes.h>

extern mtdModule mtcdFloat;
extern mtdModule mtcdNumeric;

static ACI_RC mtdInitializeNumber( acp_uint32_t aNo );

static ACI_RC mtdValidate( mtcColumn*   aColumn,
                           void*        aValue,
                           acp_uint32_t aValueSize);


extern ACI_RC mtdFloatValue( mtcTemplate*  aTemplate,
                             mtcColumn*    aColumn,
                             void*         aValue,
                             acp_uint32_t* aValueOffset,
                             acp_uint32_t  aValueSize,
                             const void*   aToken,
                             acp_uint32_t  aTokenLength,
                             ACI_RC*       aResult );

extern ACI_RC mtdFloatValueFromOracle( mtcColumn*    aColumn,
                                       void*         aValue,
                                       acp_uint32_t* aValueOffset,
                                       acp_uint32_t  aValueSize,
                                       const void*   aOracleValue,
                                       acp_sint32_t  aOracleLength,
                                       ACI_RC*       aResult );


static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"NUMBER" }
};

mtdModule mtcdNumber = {
    mtdTypeName,
    NULL,
    MTD_NUMBER_ID,
    0,
    { 0,
      0,
      0, 0, 0, 0, 0 },
    0,
    0|  // MTD_SELECTIVITY_DISABLE(Float 또는 Numeric으로 처리)
    MTD_CREATE_ENABLE|
    MTD_CREATE_PARAM_PRECISIONSCALE|
    MTD_SEARCHABLE_PRED_BASIC|
    MTD_UNSIGNED_ATTR_TRUE|
    MTD_NUM_PREC_RADIX_TRUE|
    MTD_VARIABLE_LENGTH_TYPE_TRUE|   // PROJ-1705
    MTD_DATA_STORE_DIVISIBLE_FALSE|  // PROJ-1705
    MTD_DATA_STORE_MTDVALUE_FALSE,   // PROJ-1705
    MTD_NUMERIC_PRECISION_MAXIMUM,
    MTD_NUMERIC_SCALE_MINIMUM,
    MTD_NUMERIC_SCALE_MAXIMUM,
    NULL, // staticNull
    mtdInitializeNumber,
    NULL, // mtdEstimate : 칼럼 생성 시, Float/Numeric으로 초기화되므로 사용 X
    mtdFloatValue,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
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
    NULL,
    NULL,
    mtdValidate,
    mtdSelectivityNA, // NUMBER Module은 Float/Numeric Module로 변환됨.
    mtdEncodeNumericDefault,
    mtdDecodeDefault,
    mtdCompileFmtDefault,
    mtdFloatValueFromOracle,
    mtdMakeColumnInfoDefault,

    // PROJ-1705
    mtdStoredValue2MtdValueNA,
    mtdNullValueSizeNA,
    mtdHeaderSizeNA
};

ACI_RC mtdInitializeNumber( acp_uint32_t aNo )
{
    ACI_TEST( mtdInitializeModule( &mtcdNumber, aNo ) != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC mtdValidate( mtcColumn*   aColumn,
                    void*        aValue,
                    acp_uint32_t aValueSize)
{
    ACI_TEST( mtcdNumeric.validate( aColumn, aValue, aValueSize )
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
