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
 * $Id: qtcSpArrType.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 *  Description :
 *      PROJ-1075 Associative Array Type
 *      array type은 파라미터로 사용시, 또는 변수로 사용시
 *      맨 처음에 초기화를 일단 한번 시켜주어야 한다.
 *      절대 null로 초기화는 불가능. null자체가 없다.
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxArray.h>

#define QTC_ARRTYPE_ALIGN   (ID_SIZEOF(SDouble))

// PROJ-1904 Extend UDT
#define QTC_ARRTYPE_SIZE    (idlOS::align8((UInt)ID_SIZEOF(qsxArrayInfo*)))

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

// PROJ-1904 Extend UDT
static void mtdSetNull( const mtcColumn * aColumn,
                        void            * aRow );

static mtcName mtdTypeName[1] = {
    { NULL, 17, (void*)"ASSOCIATIVE_ARRAY" },
};

static mtcColumn mtdColumn;

mtdModule qtc::spArrTypeModule = {
    mtdTypeName,
    &mtdColumn,
    MTD_ASSOCIATIVE_ARRAY_ID,
    MTD_UDT_MODULE_NO,
    {0,0,0,0,0,0,0,0},
    QTC_ARRTYPE_ALIGN,
    MTD_GROUP_MISC|MTD_CANON_NEEDLESS|MTD_CREATE_DISABLE|
    MTD_COLUMN_TYPE_FIXED|MTD_SELECTIVITY_DISABLE|MTD_PSM_TYPE_ENABLE,
    0,              // max precision
    0,              // min scale
    0,              // max scale
    NULL,
    NULL,
    mtdEstimate,
    NULL,
    mtdActualSize,
    NULL,
    mtdSetNull,
    NULL,
    mtd::isNullDefault,
    mtd::isTrueNA,
    {
        mtd::compareNA,           // Logical Comparison
        mtd::compareNA
    },
    {                         // Key Comparison
        {
            mtd::compareNA,         // Ascending Key Comparison
            mtd::compareNA          // Descending Key Comparison
        },
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
        }
    },
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // BUG-28934
    NULL,
    NULL,
    
    { 
        // PROJ-1705
        NULL,
        // PROJ-2429
        NULL
    },        
    NULL,           
    NULL,    

    // PROJ-2399
    NULL
};

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * /* aArguments */,
                    SInt * /* aPrecision */,
                    SInt * /* aScale */ )
{
    *aColumnSize = QTC_ARRTYPE_SIZE;
    
    return IDE_SUCCESS;
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    return QTC_ARRTYPE_SIZE;
}

// PROJ-1904 Extend UDT
// Array type의 null 함수는 truncate로 동작한다.
void mtdSetNull( const mtcColumn * /*aColumn*/,
                 void            * aRow )
{
    qsxArrayInfo * sArrayInfo = *((qsxArrayInfo**)aRow);
    IDE_RC         sRc;

    if ( sArrayInfo != NULL )
    {
        sRc = qsxArray::truncateArray( sArrayInfo );

        IDE_DASSERT( sRc == IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( 0 );
    }
}

