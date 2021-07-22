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
 * $Id: mtdNumber.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

extern mtdModule mtdFloat;
extern mtdModule mtdNumeric;

static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdValidate( mtcColumn * aColumn,
                           void      * aValue,
                           UInt        aValueSize);

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcName mtdTypeName[1] = {
    { NULL, 6, (void*)"NUMBER" }
};

mtdModule mtdNumber = {
    mtdTypeName,
    NULL,
    MTD_NUMBER_ID,
    0,
    { SMI_BUILTIN_B_TREE_INDEXTYPE_ID,
      SMI_BUILTIN_B_TREE2_INDEXTYPE_ID,
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
    MTD_DATA_STORE_MTDVALUE_FALSE|   // PROJ-1705
    MTD_PSM_TYPE_ENABLE, // PROJ-1904
    MTD_NUMERIC_PRECISION_MAXIMUM,
    MTD_NUMERIC_SCALE_MINIMUM,
    MTD_NUMERIC_SCALE_MAXIMUM,
    NULL, // staticNull
    mtdInitialize,
    NULL, // mtdEstimate : 칼럼 생성 시, Float/Numeric으로 초기화되므로 사용 X
    mtdFloat.value,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    {
        mtd::compareNA,           // Logical Comparison
        mtd::compareNA
    },
    {
        // Key Comparison
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        }
        ,
        {
            /* PROJ-2433 */
            mtd::compareNA,
            mtd::compareNA
        }
        ,
        {
            /* PROJ-2433 */
            mtd::compareNA,
            mtd::compareNA
        }
    },
    NULL,
    NULL,
    mtdValidate,
    mtd::selectivityNA, // NUMBER Module은 Float/Numeric Module로 변환됨.
    mtd::encodeNumericDefault,
    mtd::decodeDefault,
    mtd::compileFmtDefault,
    mtdFloat.valueFromOracle,
    mtd::makeColumnInfoDefault,

    // BUG-28934
    mtk::mergeAndRangeDefault,
    mtk::mergeOrRangeListDefault,

    {
        // PROJ-1705
        mtd::mtdStoredValue2MtdValueNA, 
        // PROJ-2429
        mtd::mtdStoredValue2MtdValue4CompressColumn
    },
    mtd::mtdNullValueSizeNA,
    mtd::mtdHeaderSizeNA,

    // PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdNumber, aNo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdValidate( mtcColumn * aColumn,
                    void      * aValue,
                    UInt        aValueSize)
{
    IDE_TEST( mtdNumeric.validate( aColumn, aValue, aValueSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static UInt mtdStoreSize( const smiColumn * /*aColumn*/ )
{
/***********************************************************************
 * PROJ-2399 row tmaplate 
 * sm에 저장되는 데이터의 크기를 반환한다. 
 * varchar와 같은 variable 타입의 데이터 타입은 ID_UINT_MAX를 반환
 * mtheader가 sm에 저장된경우가 아니면 mtheader크기를 빼서 반환 
 **********************************************************************/

    return ID_UINT_MAX;
}
