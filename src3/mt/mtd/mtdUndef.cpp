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
 * $Id: mtdUndef.cpp 47933 2011-11-09 02:01:37Z junokun $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtl.h>
#include <mtk.h>
#include <mtdTypes.h>

#define MTD_UNDEF_ALIGN (ID_SIZEOF(UChar))
#define MTD_UNDEF_SIZE  (ID_SIZEOF(UChar))

extern mtdModule mtdUndef;

static mtdUndefType mtdUndefNull = 0;

static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

static IDE_RC mtdValue( mtcTemplate* /* aTemplate */,
                        mtcColumn*   /* aColumn */,
                        void*        /* aValue */,
                        UInt*        /* aValueOffset */,
                        UInt         /* aValueSize */,
                        const void*  /* aToken */,
                        UInt         /* aTokenLength */,
                        IDE_RC*      /* aResult */ );

static mtcName mtdTypeName[1] = {
    { NULL, 5, (void*)"UNDEF" },
};

static UInt mtdStoreSize( const smiColumn * aColumn );

static mtcColumn mtdColumn;

mtdModule mtdUndef = {
    mtdTypeName,
    &mtdColumn,
    MTD_UNDEF_ID,
    0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    MTD_UNDEF_ALIGN,                // align
    MTD_GROUP_TEXT|MTD_CANON_NEEDLESS|MTD_CREATE_DISABLE|
    MTD_COLUMN_TYPE_FIXED|MTD_SELECTIVITY_DISABLE|MTD_INTERNAL_TYPE_TRUE|
    MTD_PSM_TYPE_DISABLE, // PROJ-1904
    0,                              // max precision
    0,                              // min scale
    0,                              // max scale
    &mtdUndefNull,                  // null
    mtdInitialize,
    mtdEstimate,
    mtdValue,                       // mtdValue
    NULL,                           // mtdActualSize
    mtd::getPrecisionNA,
    NULL,                           // mtdNull
    NULL,                           // mtdHash
    NULL,                           // mtdIsNull
    mtd::isTrueNA,
    {
        mtd::compareNA,             // Logical Comparison
        mtd::compareNA              // Logical Comparison
    },
    {                               // Key Comparison
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        },
        {
            mtd::compareNA,
            mtd::compareNA
        },
        {
            mtd::compareNA,
            mtd::compareNA
        },
        {
            mtd::compareNA,
            mtd::compareNA
        },
        {
            /* PROJ-2433 */
            mtd::compareNA,
            mtd::compareNA
        },
        {
            /* PROJ-2433 */
            mtd::compareNA,
            mtd::compareNA
        }
    },
    NULL,                           // mtd::canonizeDefault
    NULL,                           // mtdEndian
    NULL,                           // mtdValidate
    mtd::selectivityNA,
    NULL,                           // encode
    NULL,                           // mtd::decodeDefault
    NULL,                           // mtd::compileFmtDefault
    NULL,                           // mtd::valueFromOracleDefault
    NULL,                           // mtd::makeColumnInfoDefault

    // BUG-28934
    mtk::mergeAndRangeNA,
    mtk::mergeOrRangeListNA,

    {    
        // PROJ-1705
        mtd::mtdStoredValue2MtdValueNA,
        // PROJ-2429
        NULL 
    }, 
    mtd::mtdNullValueSizeNA,
    mtd::mtdHeaderSizeNA,

    // PROJ-2399
    mtdStoreSize
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtdUndef, aNo ) != IDE_SUCCESS );    

    // mtdColumn 의 초기화
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & mtdUndef,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/  )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    *aColumnSize = MTD_UNDEF_SIZE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtdValue( mtcTemplate* /* aTemplate */,
                 mtcColumn*   /* aColumn */,
                 void*        /* aValue */,
                 UInt*        /* aValueOffset */,
                 UInt         /* aValueSize */,
                 const void*  /* aToken */,
                 UInt         /* aTokenLength */,
                 IDE_RC*      /* aResult */ )
{
    IDE_ASSERT(0);

    return IDE_FAILURE;
}

static UInt mtdStoreSize( const smiColumn * /*aColumn*/ )
{
/***********************************************************************
 * PROJ-2399 row tmaplate 
 * sm에 저장되는 데이터의 크기를 반환한다. 
 * variable 타입의 데이터 타입은 ID_UINT_MAX를 반환
 * mtheader가 sm에 저장된경우가 아니면 mtheader크기를 빼서 반환 
 **********************************************************************/

    return ID_UINT_MAX;
}
