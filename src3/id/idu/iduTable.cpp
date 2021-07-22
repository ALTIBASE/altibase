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
 
#include <ide.h>
#include <iduTable.h>
#include <iduMemMgr.h>

void iduTable::freeColumns( iduTableColumn* aColumns )
{
    UInt sIterator;
    
    if( aColumns != NULL )
    {
        freeColumns( aColumns->next );
        for( sIterator = 0; sIterator < aColumns->valueCount; sIterator++ )
        {
            if( aColumns->values[sIterator] != NULL )
            {
                iduMemMgr::freeRaw( aColumns->values[sIterator] );
            }
        }
        iduMemMgr::freeRaw( aColumns->column );
        iduMemMgr::freeRaw( aColumns->values );
        iduMemMgr::freeRaw( aColumns );
    }
}

void iduTable::freeFilters( iduTableFilter* aFilters )
{
    if( aFilters != NULL )
    {
        freeFilters( aFilters->next );
        iduMemMgr::freeRaw( aFilters->column );
        iduMemMgr::freeRaw( aFilters->value );
        iduMemMgr::freeRaw( aFilters );
    }
}

UInt iduTable::columns( void )
{
    UInt            sColumns;
    iduTableColumn* sColumn;
    
    for( sColumns = 0, sColumn = mColumns.next;
         sColumn != NULL;
         sColumns++, sColumn = sColumn->next ) ;
    
    return sColumns;
}

UInt iduTable::rows( void )
{
    UInt            sRows;
    iduTableColumn* sColumn;
    
    for( sColumn = mColumns.next, sRows = 0;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        if( sColumn->valueCount > sRows )
        {
            sRows = sColumn->valueCount;
        }
    }
    
    return sRows;
}

idBool iduTable::filter( UInt aRow )
{
    iduTableFilter* sFilter;
    iduTableColumn* sColumn;
    
    for( sFilter = mFilters.next;
         sFilter != NULL;
         sFilter = sFilter->next )
    {
        for( sColumn = mColumns.next;
             sColumn != NULL;
             sColumn = sColumn->next )
        {
            if( idlOS::strcmp( (char*)sColumn->column,
                               (char*)sFilter->column ) == 0 )
            {
                if( aRow < sColumn->valueCount )
                {
                    if( sColumn->values[aRow] != NULL )
                    {
                        if( idlOS::strcmp( (char*)sColumn->values[aRow],
                                           (char*)sFilter->value ) == 0 )
                        {
                            break;
                        }
                    }
                    else
                    {
                        if( idlOS::strcmp( "",
                                           (char*)sFilter->value ) == 0 )
                        {
                            break;
                        }
                    }
                }
            }
        }
        if( sColumn == NULL )
        {
            return ID_FALSE;
        }
    }
    
    return ID_TRUE;
}


UInt iduTable::textLength( const UChar* aString )
{
    UInt sLength;
    
    for( sLength = 0; *aString != '\0'; aString++ )
    {
        if( *aString >= 0x01 && *aString < 0x1F )
        {
            sLength++;
        }
        else
        {
            sLength++;
        }
    }
    
    return sLength;
}

void iduTable::textPrint( FILE*        aFile,
                          const UChar* aString,
                          UInt         aWidth )
{
    const UChar* sString;
    UInt         sLength;
    UInt         sIterator;
    
    sIterator = 0;
    
    for( sString = aString, sLength = 0;
         *sString != '\0';
         sString++, sLength++ )
    {
        if( *sString < '0' || *sString > '9' )
        {
            break;
        }
    }
    if( *sString == '\0' )
    {
        for( ; sIterator < aWidth - sLength; sIterator++ )
        {
            idlOS::fprintf( aFile, "%c", ' ' );
        }
    }
    
    for( sString = aString, sLength = 0; *sString != '\0'; sString++ )
    {
        if( *sString >= 0x01 && *sString < 0x1F )
        {
            sIterator += 2;
            idlOS::fprintf( aFile, "%c", '^' );
            idlOS::fprintf( aFile, "%c", (*sString) + 0x40 );
        }
        else
        {
            sIterator++;
            idlOS::fprintf( aFile, "%c", *sString );
        }
    }
    
    for( ; sIterator < aWidth; sIterator++ )
    {
        idlOS::fprintf( aFile, "%c", ' ' );
    }
}

void iduTable::printText( FILE* aFile )
{
    iduTableColumn* sColumn;
    UInt            sRowIterator;
    UInt            sRowFence;
    UInt            sColumnIterator;
    UInt            sColumnFence;
    UInt            sWidth;
    UInt            sColumnWidth[16384];
    
    sRowFence    = rows();
    sColumnFence = columns();
    
    for( sColumnIterator = 0, sColumn = mColumns.next;
         sColumnIterator < sColumnFence;
         sColumnIterator++, sColumn = sColumn->next )
    {
        if( mTitle == ID_TRUE )
        {
            sColumnWidth[sColumnIterator] = textLength( sColumn->column );
        }
        else
        {
            //fix for UMR
            sColumnWidth[sColumnIterator] = 0;
        }

        for( sRowIterator = 0; sRowIterator < sRowFence; sRowIterator++ )
        {
            sWidth = textLength( value( sColumnIterator, sRowIterator ) );
            if( sColumnWidth[sColumnIterator] < sWidth )
            {
                sColumnWidth[sColumnIterator] = sWidth;
            }
        }
    }
    
    if( mTitle == ID_TRUE )
    {
        sColumnIterator = 0;
        if( sColumnIterator < sColumnFence )
        {
            textPrint( aFile,
                       column( sColumnIterator ),
                       sColumnWidth[sColumnIterator] );
            for( sColumnIterator++;
                 sColumnIterator < sColumnFence;
                 sColumnIterator++ )
            {
                idlOS::fprintf( aFile, "%c", ' ' );
                textPrint( aFile,
                           column( sColumnIterator ),
                           sColumnWidth[sColumnIterator] );
            }
        }
        idlOS::fprintf( aFile, "%c", '\n' );
    }
    
    for( sRowIterator = 0;
         sRowIterator < sRowFence;
         sRowIterator++ )
    {
        if( filter( sRowIterator ) == ID_TRUE )
        {
            sColumnIterator = 0;
            if( sColumnIterator < sColumnFence )
            {
                textPrint( aFile,
                           value( sColumnIterator, sRowIterator ),
                           sColumnWidth[sColumnIterator] ); 
                for( sColumnIterator++;
                     sColumnIterator < sColumnFence;
                     sColumnIterator++ )
                {
                    idlOS::fprintf( aFile, "%c", ' ' );
                    textPrint( aFile,
                               value( sColumnIterator, sRowIterator ),
                               sColumnWidth[sColumnIterator] );
                }
            }
            idlOS::fprintf( aFile, "%c", '\n' );
        }
    }
}

void iduTable::initialize( void )
{
    mFormat                 = iduTableCsvFormat;
    mTitle                  = ID_TRUE;
    mColumns.next           = NULL;
    mColumns.column         = NULL;
    mColumns.valueAllocated = 0;
    mColumns.valueCount     = 0;
    mColumns.values         = NULL;
    mFilters.next           = NULL;
    mFilters.column         = NULL;
    mFilters.value          = NULL;
}

void iduTable::finalize( void )
{
    clearColumns();
    clearFilters();
}

void iduTable::clearColumns( void )
{
    freeColumns( mColumns.next );
    mColumns.next           = NULL;
    mColumns.column         = NULL;
    mColumns.valueAllocated = 0;
    mColumns.valueCount     = 0;
    mColumns.values         = NULL;
}

const UChar* iduTable::column( UInt aColumn )
{
    iduTableColumn* sColumn;
    
    for( sColumn = mColumns.next;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        if( aColumn == 0 )
        {
            return sColumn->column;
        }
        aColumn--;
    }
    
    return (const UChar*)"";
}

IDE_RC iduTable::addColumn( UInt*         aColumnOrder,
                            const UChar*  aColumn )
{
    iduTableColumn* sColumn;
    
    for( sColumn = &mColumns, *aColumnOrder = 0;
         sColumn->next != NULL;
         sColumn = sColumn->next, (*aColumnOrder)++ ) ;


    sColumn->next = (iduTableColumn*)iduMemMgr::mallocRaw( sizeof(iduTableColumn) );
    
    IDE_TEST_RAISE( sColumn->next == NULL, ERR_MEMORY_ALLOCATION );
    
    sColumn = sColumn->next;
    
    sColumn->next           = NULL;
    sColumn->column         =
            (UChar*)iduMemMgr::mallocRaw( idlOS::strlen( (const char*)aColumn ) + 1 );
    sColumn->valueAllocated = 0;
    sColumn->valueCount     = 0;
    sColumn->values         = NULL;
    
    IDE_TEST_RAISE( sColumn->column == NULL, ERR_MEMORY_ALLOCATION );
    idlOS::strcpy( (char*)sColumn->column, (char*)aColumn );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION );
    {
        ideLogEntry sLog(IDE_SERVER_0);
        sLog.append( "Memory allocation failure.\n" );
        sLog.write();
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

const UChar* iduTable::value( UInt aColumn,
                              UInt aRow )
{
    iduTableColumn* sColumn;
    
    for( sColumn = mColumns.next;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        if( aColumn == 0 )
        {
            if( aRow < sColumn->valueCount )
            {
                if( sColumn->values[aRow] != NULL )
                {
                    return sColumn->values[aRow];
                }
            }
            return (const UChar*)"";
        }
        aColumn--;
    }
    
    return (const UChar*)"";
}

IDE_RC iduTable::addValue( UInt         aColumn,
                           const UChar* aValue )
{
    iduTableColumn* sColumn;
    
    for( sColumn = mColumns.next;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        if( aColumn == 0 )
        {
            if( sColumn->valueAllocated <= sColumn->valueCount )
            {
                sColumn->valueAllocated = sColumn->valueCount + 10;
                sColumn->values = (UChar**)idlOS::realloc( sColumn->values,
                                    sColumn->valueAllocated * sizeof(UChar*) );
                IDE_TEST_RAISE( sColumn->values == NULL,
                                ERR_MEMORY_ALLOCATION );
            }
            sColumn->values[sColumn->valueCount] =
                   (UChar*)iduMemMgr::mallocRaw( idlOS::strlen( (char*)aValue ) + 1 );
            IDE_TEST_RAISE( sColumn->values[sColumn->valueCount] == NULL,
                            ERR_MEMORY_ALLOCATION );
            idlOS::strcpy( (char*)sColumn->values[sColumn->valueCount],
                           (char*)aValue );
            sColumn->valueCount++;
            break;
        }
        aColumn--;
    }
    IDE_TEST_RAISE( sColumn == NULL, ERR_COLUMN_NOT_FOUND );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND );
    {
        ideLogEntry sLog(IDE_SERVER_0);
        sLog.append( "Column not found.\n" );
        sLog.write();
    }
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION );
    {
        ideLogEntry sLog(IDE_SERVER_0);
        sLog.append( "Memory allocation failure.\n" );
        sLog.write();
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

iduTableFormat iduTable::format( void )
{
    return mFormat;
}

void iduTable::setFormat( iduTableFormat aFormat )
{
    mFormat = aFormat;
}

idBool iduTable::title( void )
{
    return mTitle;
}

void iduTable::setTitle( idBool aTitle )
{
    mTitle = aTitle;
}

void iduTable::clearFilters( void )
{
    freeFilters( mFilters.next );
    mFilters.next   = NULL;
    mFilters.column = NULL;
    mFilters.value  = NULL;
}

IDE_RC iduTable::addFilter( const UChar* aColumn,
                            const UChar* aValue )
{
    iduTableFilter* sFilter;
    
    for( sFilter = &mFilters; sFilter->next != NULL; sFilter = sFilter->next ) ;
    
    sFilter->next = (iduTableFilter*)iduMemMgr::mallocRaw( sizeof(iduTableFilter) );
    
    IDE_TEST_RAISE( sFilter->next == NULL, ERR_MEMORY_ALLOCATION );
    
    sFilter = sFilter->next;
    
    sFilter->next   = NULL;
    sFilter->column =
                  (UChar*)iduMemMgr::mallocRaw( idlOS::strlen( (char*)aColumn ) + 1 );
    IDE_TEST_RAISE( sFilter->column == NULL, ERR_MEMORY_ALLOCATION );
    idlOS::strcpy( (char*)sFilter->column, (char*)aColumn );
    sFilter->value  =
                   (UChar*)iduMemMgr::mallocRaw( idlOS::strlen( (char*)aValue ) + 1 );
    IDE_TEST_RAISE( sFilter->value == NULL, ERR_MEMORY_ALLOCATION );
    idlOS::strcpy( (char*)sFilter->value, (char*)aValue );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION );
    {
        ideLogEntry sLog(IDE_SERVER_0);
        sLog.append( "Memory allocation failure.\n" );
        sLog.write();
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void iduTable::print( FILE* aFile )
{
    switch( mFormat )
    {
     case iduTableTextFormat:
        printText( aFile );
        break;
     case iduTableCsvFormat:
        break;
     case iduTableHtmlFormat:
        break;
     default:
        break;
    }
}
