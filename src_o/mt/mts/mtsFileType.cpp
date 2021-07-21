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
 * $Id: mtsFileType.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 *  Description :
 *      PROJ-1371 PSM File handling¿ª ¿ß«— FILE_TYPE
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtdTypes.h>

#define MTS_FILETYPE_ALIGN   (ID_SIZEOF(SDouble))
#define MTS_FILETYPE_SIZE    (idlOS::align8((UInt)ID_SIZEOF(mtsFileType)))

extern mtdModule mtsFile;

static const mtsFileType mtsFileTypeNull = { NULL, "N" };


static IDE_RC mtdInitialize( UInt aNo );

static IDE_RC mtdEstimate( UInt     * aColumnSize,
                           UInt     *  aArguments,
                           SInt     *  aPrecision,
                           SInt     *  aScale );

static IDE_RC mtdValue( mtcTemplate* aTemplate,
                        mtcColumn*  aColumn,
                        void*       aValue,
                        UInt*       aValueOffset,
                        UInt        aValueSize,
                        const void* aToken,
                        UInt        aTokenLength,
                        IDE_RC*     aResult );

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
    { NULL, 9, (void*)"FILE_TYPE" },
};

static mtcColumn mtdColumn;

mtdModule mtsFile = {
    mtdTypeName,
    &mtdColumn,
    MTS_FILETYPE_ID,
    0,
    {0,0,0,0,0,0,0,0},
    MTS_FILETYPE_ALIGN,
    MTD_GROUP_MISC|
      MTD_CANON_NEEDLESS|
      MTD_CREATE_DISABLE|
      MTD_COLUMN_TYPE_FIXED|
      MTD_SELECTIVITY_DISABLE|
      MTD_PSM_TYPE_DISABLE, // PROJ-1904
    0,
    0,
    0,
    (void*)&mtsFileTypeNull,
    mtdInitialize,
    mtdEstimate,
    mtdValue,
    mtdActualSize,
    mtd::getPrecisionNA,
    mtdSetNull,
    mtdHash,
    mtdIsNull,
    mtd::isTrueNA,
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
    },
    mtd::canonizeDefault,
    mtdEndian,
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
        mtd::mtdStoredValue2MtdValueNA,
        // PROJ-2429
        NULL
    },
    mtd::mtdNullValueSizeNA,
    mtd::mtdHeaderSizeNA,

    // PROJ-2399
    mtd::mtdStoreSizeDefault
};

IDE_RC mtdInitialize( UInt aNo )
{
    IDE_TEST( mtd::initializeModule( &mtsFile, aNo ) != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( & mtdColumn, 
                                     & mtsFile, 
                                     0,   // arguments 
                                     0,   // precision 
                                     0 )  // scale 
              != IDE_SUCCESS ); 

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtdEstimate( UInt     * aColumnSize,
                    UInt     * aArguments,
                    SInt *   /*aPrecision*/,
                    SInt *   /*aScale*/ )
{
    IDE_TEST_RAISE( *aArguments != 0, ERR_INVALID_LENGTH );
    
    *aColumnSize = MTS_FILETYPE_SIZE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    
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
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_APPLICABLE));
    
    return IDE_FAILURE;
}

UInt mtdActualSize( const mtcColumn*,
                    const void* )
{
    return MTS_FILETYPE_SIZE;
}

void mtdSetNull( const mtcColumn*,
                 void*            aRow )
{
    if( aRow != NULL )
    {
        *(mtsFileType*)aRow = mtsFileTypeNull;
    }
}

UInt mtdHash( UInt             aHash,
              const mtcColumn* ,
              const void*      aRow )
{
    return mtc::hash( aHash, (const UChar*)aRow, MTS_FILETYPE_SIZE );
}

idBool mtdIsNull( const mtcColumn* ,
                  const void*      aRow )
{
    if( ( ((mtsFileType*)aRow)->fp == mtsFileTypeNull.fp ) &&
        ( idlOS::strcmp( ((mtsFileType*)aRow)->mode, mtsFileTypeNull.mode ) == 0 ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

void mtdEndian( void* )
{
    return ;
} 
