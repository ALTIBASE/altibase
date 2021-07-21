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
 
#ifndef _O_IDU_TABLE_H_
# define _O_IDU_TABLE_H_ 1

# include <idl.h>

enum iduTableFormat {
    iduTableTextFormat,
    iduTableCsvFormat,
    iduTableHtmlFormat
};

typedef struct iduTableColumn {
    iduTableColumn* next;
    UChar*          column;
    UInt            valueAllocated;
    UInt            valueCount;
    UChar**         values;
} iduTableColumn;

typedef struct iduTableFilter {
    iduTableFilter* next;
    UChar*          column;
    UChar*          value;
} iduTableFilter;

class iduTable{
 private:
    iduTableFormat mFormat;
    
    idBool mTitle;
    
    iduTableColumn mColumns;
    
    iduTableFilter mFilters;
    
    void freeColumns( iduTableColumn* aColumns );
    
    void freeFilters( iduTableFilter* aFilters );
    
    UInt columns( void );
    
    UInt rows( void );
    
    idBool filter( UInt aRow );
    
    UInt textLength( const UChar* aString );
    
    void textPrint( FILE*        aFile,
                    const UChar* aString,
                    UInt         aWidth );
    
    void printText( FILE* aFile );
    
 public:
    void initialize( void );
    
    void finalize( void );
    
    void clearColumns( void );
    
    const UChar* column( UInt aColumn );
                   
    IDE_RC addColumn( UInt*         aColumnOrder,
                      const UChar*  aColumn );
    
    const UChar* value( UInt aColumn,
                        UInt aRow );
    
    IDE_RC addValue( UInt         aColumn,
                     const UChar* aValue );
    
    iduTableFormat format( void );
    
    void setFormat( iduTableFormat aFormat );
    
    idBool title( void );
    
    void setTitle( idBool aTitle );
    
    void clearFilters( void );
    
    IDE_RC addFilter( const UChar* aColumn,
                      const UChar* aValue );
    
    void print( FILE* aFile );
};

#endif /* _O_IDU_TABLE_H_ */
