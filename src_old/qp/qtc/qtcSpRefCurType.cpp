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
 * $Id$
 *
 *  Description :
 *      PROJ-1386 REF CURSOR Type
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxRefCursor.h>

#define QTC_REFCURTYPE_ALIGN   (ID_SIZEOF(SDouble))
#define QTC_REFCURTYPE_SIZE    (idlOS::align8((UInt)ID_SIZEOF(qsxRefCursorInfo)))

qsxRefCursorInfo qtcRefCurNull = { 0, 0, ID_TRUE, ID_FALSE, ID_FALSE, ID_FALSE, ID_FALSE, NULL };

static IDE_RC mtdEstimate( UInt     * aColumnSize,
                           UInt     * aArguments,
                           SInt     * aPrecision,
                           SInt     * aScale );

static UInt mtdActualSize( const mtcColumn* aColumn,
                           const void*      aRow );

static void mtdSetNull( const mtcColumn* aColumn,
                        void*            aRow );

static idBool mtdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static mtcName mtdTypeName[1] = {
    { NULL, 10, (void*)"REF_CURSOR" },
};

static mtcColumn mtdColumn;

mtdModule qtc::spRefCurTypeModule = {
    mtdTypeName,
    &mtdColumn,
    MTD_REF_CURSOR_ID,
    MTD_UDT_MODULE_NO,
    {0,0,0,0,0,0,0,0},
    QTC_REFCURTYPE_ALIGN,
    MTD_GROUP_MISC|MTD_CANON_NEEDLESS|MTD_CREATE_DISABLE|
    MTD_COLUMN_TYPE_FIXED|MTD_SELECTIVITY_DISABLE|MTD_PSM_TYPE_DISABLE,
    0,              // max precision
    0,              // min scale
    0,              // max scale
    (void*)&qtcRefCurNull,
    NULL,
    mtdEstimate,
    NULL,
    mtdActualSize,
    NULL,
    mtdSetNull,
    NULL,
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
                    UInt     * /* aArguments */,
                    SInt     * /* aPrecision */,
                    SInt     * /* aScale */ )
{
    // size는 structure의 크기와 관계가 있다.
    
    *aColumnSize = QTC_REFCURTYPE_SIZE;
    
    return IDE_SUCCESS;
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    // actual size는 structure의 크기와 관계가 있다.
    return QTC_REFCURTYPE_SIZE;
}

void mtdSetNull( const mtcColumn* ,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        *(qsxRefCursorInfo*)aRow = qtcRefCurNull;
    }
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    SInt                     sResult;

    if( ( ((qsxRefCursorInfo*)aRow)->id    == qtcRefCurNull.id ) &&
        ( ((qsxRefCursorInfo*)aRow)->hStmt == qtcRefCurNull.hStmt ) )
    {
        sResult = 1;
    }
    else
    {
        sResult = 0;
    }

    return ( sResult == 1 ) ? ID_TRUE : ID_FALSE;
}
