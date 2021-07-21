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
 * $Id: qtcSpRowType.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 *  Description :
 *      PROJ-1075 rowtype
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsParseTree.h>
#include <qsxArray.h>
#include <qsvEnv.h>

#define QTC_ROWTYPE_ALIGN   (ID_SIZEOF(SDouble))


static IDE_RC mtdEstimate( UInt     * aColumnSize,
                           UInt     *  aArguments,
                           SInt     *  aPrecision,
                           SInt     *  aScale );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static mtcName mtdTypeName1[1] = {
    { NULL, 8, (void*)"ROW_TYPE" },
};

static mtcColumn mtdColumn1;

static mtcName mtdTypeName2[1] = {
    { NULL, 11, (void*)"RECORD_TYPE" },
};

static mtcColumn mtdColumn2;

mtdModule qtc::spRowTypeModule = {
    mtdTypeName1,
    &mtdColumn1,
    MTD_ROWTYPE_ID,
    MTD_UDT_MODULE_NO,
    {0,0,0,0,0,0,0,0},
    QTC_ROWTYPE_ALIGN,
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
    NULL,
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

mtdModule qtc::spRecordTypeModule = {
    mtdTypeName2,
    &mtdColumn2,
    MTD_RECORDTYPE_ID,
    MTD_UDT_MODULE_NO,
    {0,0,0,0,0,0,0,0},
    QTC_ROWTYPE_ALIGN,
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
    NULL,
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


IDE_RC mtdEstimate( UInt     * aColumnSize,
                    UInt     * aArguments,
                    SInt     * aPrecision,
                    SInt *   /*aScale*/ )
{
#define IDE_FN "IDE_RC mtdEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    IDE_TEST_RAISE( *aArguments != 1, ERR_INVALID_LENGTH );

    IDE_TEST_RAISE( *aPrecision < 0,
                    ERR_INVALID_LENGTH );
    
    *aColumnSize = *aPrecision;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

UInt mtdActualSize( const mtcColumn* aColumn,
                    const void* )
{
    return aColumn->precision;
}

// PROJ-1904 Extend UDT
// Row/record type이 array type을 column type으로 갖고 있으면,
// 각 column 마다 null 함수를 호출한다.
void mtdSetNull( const mtcColumn* aColumn,
                 void*            aRow )
{
    mtcColumn   * sColumn = NULL;
    qcmColumn   * sQcmColumn = NULL;
    void        * sRow = NULL;
    UInt          sSize = 0;
    qsTypes     * sType = NULL;

    if ( aRow != NULL )
    {
        sType = (qsTypes*)(((qtcModule*)aColumn->module)->typeInfo);

        if ( (sType->flag & QTC_UD_TYPE_HAS_ARRAY_MASK) ==
             QTC_UD_TYPE_HAS_ARRAY_FALSE )
        {
            idlOS::memcpy( aRow, aColumn->module->staticNull, aColumn->precision );
        }
        else
        {
            sQcmColumn = sType->columns;

            while ( sQcmColumn != NULL )
            {
                sColumn = sQcmColumn->basicInfo;

                sSize = idlOS::align( sSize, sColumn->module->align ); 
                sRow = (void*)((UChar*)aRow + sSize);

                sColumn->module->null( sColumn, sRow );

                sSize += sColumn->column.size;

                sQcmColumn = sQcmColumn->next;
            }
        }
    }
    else
    {
        // Nothing to do.
    }
}
