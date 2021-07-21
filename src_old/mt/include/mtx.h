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
 * $Id: mtx.h 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#ifndef _O_MTX_H_
# define _O_MTX_H_ 1

#include <mte.h>
#include <mtd.h>
#include <mtl.h>
#include <mtv.h>
#include <mtcDef.h>
#include <mtdTypes.h>

#define MTX_IS_NOT_CALCULATED( _arg_ ) ( ( ( _arg_ ) & MTX_ENTRY_FLAG_CALCULATE_MASK ) \
                                          != MTX_ENTRY_FLAG_CALCULATE )
#define MTX_IS_NULL_REAL( _arg_ )      ( ( *(UInt *)( _arg_ ) & MTD_REAL_EXPONENT_MASK ) \
                                          == MTD_REAL_EXPONENT_MASK )
#define MTX_IS_NULL_DOUBLE( _arg_ )    ( ( *(ULong *)( _arg_ ) & MTD_DOUBLE_EXPONENT_MASK ) \
                                          == MTD_DOUBLE_EXPONENT_MASK )
#define MTX_IS_NULL_BOOLEAN( _arg_ )   ( ( _arg_ ) == MTD_BOOLEAN_NULL )
#define MTX_IS_NULL_SMALLINT( _arg_ )  ( ( _arg_ ) == MTD_SMALLINT_NULL )
#define MTX_IS_NULL_INTEGER( _arg_ )   ( ( _arg_ ) == MTD_INTEGER_NULL )
#define MTX_IS_NULL_BIGINT( _arg_ )    ( ( _arg_ ) == MTD_BIGINT_NULL )
#define MTX_IS_NULL_FLOAT( _arg_ )     ( ( _arg_ )->length == 0 )
#define MTX_IS_NULL_NUMERIC( _arg_ )   MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_CHAR( _arg_ )      MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_VARCHAR( _arg_ )   MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_NCHAR( _arg_ )     MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_NVARCHAR( _arg_ )  MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_BIT( _arg_ )       MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_VARBIT( _arg_ )    MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_BYTE( _arg_ )      MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_VARBYTE( _arg_ )   MTX_IS_NULL_FLOAT( _arg_ )
#define MTX_IS_NULL_NIBBLE( _arg_ )    ( ( _arg_ )->length == MTD_NIBBLE_NULL_LENGTH )

#define MTX_SET_NULL_BOOLEAN( _ret_ )   \
{                                       \
    *( _ret_ ) = MTD_BOOLEAN_NULL;      \
}
#define MTX_SET_NULL_SMALLINT( _ret_ )  \
{                                       \
    *( _ret_ ) = MTD_SMALLINT_NULL;     \
}
#define MTX_SET_NULL_INTEGER( _ret_ )   \
{                                       \
    *( _ret_ ) = MTD_INTEGER_NULL;      \
}
#define MTX_SET_NULL_BIGINT( _ret_ )    \
{                                       \
    *( _ret_ ) = MTD_BIGINT_NULL;       \
}
#define MTX_SET_NULL_REAL( _ret_ )      \
{                                       \
    *(UInt *)( _ret_ ) = mtdRealNull;   \
}
#define MTX_SET_NULL_DOUBLE( _ret_ )      \
{                                         \
    *(ULong *)( _ret_ ) = mtdDoubleNull;  \
}
#define MTX_SET_NULL_FLOAT( _ret_ )     \
{                                       \
    *( _ret_ ) = mtdNumericNull;        \
}
#define MTX_SET_NULL_NUMERIC( _ret_ )   \
{                                       \
    *( _ret_ ) = mtdNumericNull;        \
}
#define MTX_SET_NULL_CHAR( _ret_ )      \
{                                       \
    *( _ret_ ) = mtdCharNull;           \
}
#define MTX_SET_NULL_VARCHAR( _ret_ )   \
{                                       \
    *( _ret_ ) = mtdCharNull;           \
}
#define MTX_SET_NULL_NCHAR( _ret_ )     \
{                                       \
    *( _ret_ ) = mtdNcharNull;          \
}
#define MTX_SET_NULL_NVARCHAR( _ret_ )  \
{                                       \
    *( _ret_ ) = mtdNcharNull;          \
}
#define MTX_SET_NULL_BIT( _ret_ )       \
{                                       \
    *( _ret_ ) = mtdBitNull;            \
}
#define MTX_SET_NULL_VARBIT( _ret_ )    \
{                                       \
    *( _ret_ ) = mtdBitNull;            \
}
#define MTX_SET_NULL_BYTE( _ret_ )      \
{                                       \
    *( _ret_ ) = mtdByteNull;           \
}
#define MTX_SET_NULL_VARBYTE( _ret_ )   \
{                                       \
    *( _ret_ ) = mtdByteNull;           \
}
#define MTX_SET_NULL_NIBBLE( _ret_ )    \
{                                       \
    *( _ret_ ) = mtdNibbleNull;         \
}
#define MTX_SET_NULL_DATE( _ret_ )      \
{                                       \
    *( _ret_ ) = mtdDateNull;           \
}
#define MTX_SET_NULL_INTERVAL( _ret_ )  \
{                                       \
    *( _ret_ ) = mtdIntervalNull;       \
}

class mtx
{
public:
    static IDE_RC calculateEmpty( mtxEntry ** aEntry );

    static IDE_RC calculateNA( mtxEntry ** aEntry );

    static IDE_RC generateSerialExecute( mtcTemplate          * aTemplate,
                                         mtxSerialFilterInfo  * aInfo,
                                         mtxSerialExecuteData * aData,
                                         UInt                   aTable );

    static IDE_RC doSerialFilterExecute( idBool       * aResult,
                                         const void   * aRow,
                                         void         *,
                                         UInt          ,
                                         const scGRID   aRid,
                                         void         * aData );
};

#endif /* _O_MTX_H_ */
