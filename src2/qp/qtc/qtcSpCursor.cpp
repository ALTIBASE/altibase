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
 * $Id: qtcSpCursor.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxCursor.h>

#define MTD_SP_CURSOR_ALIGN   (ID_SIZEOF(SDouble))
#define MTD_SP_CURSOR_SIZE    (idlOS::align8((UInt)ID_SIZEOF(qsxCursorInfo)))

static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdEstimate( UInt * aColumnSize,
                           UInt * aArguments,
                           SInt * aPrecision,
                           SInt * aScale );

static IDE_RC mtdValue( mtcTemplate* aTemplate,
                        mtcColumn*   aColumn,
                        void*        aValue,
                        UInt*        aValueOffset,
                        UInt         aValueSize,
                        const void*  aToken,
                        UInt         aTokenLength,
                        IDE_RC*      aResult );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static UInt mtdHash( UInt             aHash,
                     const mtcColumn* aColumn,
                     const void*      aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static void mtdEndian( void* aValue );

static mtcName mtdTypeName[1] = {
    { NULL, 8, (void*)"SPCURSOR" },
};

static mtcColumn mtdColumn;

mtdModule qtc::spCursorModule = {
    mtdTypeName,
    &mtdColumn,
    MTD_NULL_ID, // 아직 정해지지 않은 Data Type으로 추후 결정됨.
    MTD_UDT_MODULE_NO,
    {0,0,0,0,0,0,0,0},
    MTD_SP_CURSOR_ALIGN,
    MTD_GROUP_MISC|MTD_CANON_NEEDLESS|MTD_CREATE_DISABLE|MTD_COLUMN_TYPE_FIXED|MTD_PSM_TYPE_ENABLE,
    0,              // max precision
    0,              // min scale
    0,              // max scale
    NULL, // staticNull : not used
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    NULL,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
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
    mtd::canonizeDefault,
    mtdEndian,
    NULL,
    NULL, // selectivity 추출 함수
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

IDE_RC mtdInitialize( UInt aNo )
{
#define IDE_FN "IDE_RC mtdInitialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    IDE_TEST( mtd::initializeModule( &qtc::spCursorModule, aNo )
              != IDE_SUCCESS );
    
    IDE_TEST( mtc::initializeColumn( & mtdColumn,
                                     & qtc::spCursorModule,
                                     0,
                                     0,
                                     0 ) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtdEstimate( UInt * aColumnSize,
                    UInt * aArguments,
                    SInt * /*aPrecision*/,
                    SInt * /*aScale*/ )
{
#define IDE_FN "IDE_RC mtdEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );

    /*
    aColumn->column.flag     = SMI_COLUMN_TYPE_FIXED;
    aColumn->column.size     = MTD_SP_CURSOR_SIZE;
    aColumn->type.dataTypeId = qtc::spCursorModule.id;
    aColumn->flag            = aArguments;
    aColumn->precision       = 0;
    aColumn->scale           = 0;
    aColumn->module          = & qtc::spCursorModule;
    */

    *aColumnSize = MTD_SP_CURSOR_SIZE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtdValue( mtcTemplate* /* aTemplate */,
                 mtcColumn*   /* aColumn      */,
                 void*        /* aValue       */,
                 UInt*        /* aValueOffset */,
                 UInt         /* aValueSize   */,
                 const void*  /* aToken       */,
                 UInt         /* aTokenLength */,
                 IDE_RC*      /*   aResult    */   )
{
#define IDE_FN "IDE_RC mtdValue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_APPLICABLE));
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
#define IDE_FN "UInt mtdActualSize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    return MTD_SP_CURSOR_SIZE;
    
#undef IDE_FN
}

void mtdSetNull( const mtcColumn* /* aColumn */,
                 void*            /* aRow  */)
{
#define IDE_FN "IDE_RC mtdNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // This function is not called.
    
#undef IDE_FN
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hash( aHash, (UChar*)aRow, MTD_SP_CURSOR_SIZE );
}

idBool mtdIsNull( const mtcColumn* /* aColumn */,
                  const void*      /* aRow */ )
{
    // This function is not called.
    return ID_FALSE ; // not null
}

void mtdEndian( void* )
{
#define IDE_FN "void mtdEndian"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
#undef IDE_FN
}
 
