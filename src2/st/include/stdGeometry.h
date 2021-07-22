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
 * $Id: stdGeometry.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description: stdGeometry 
 **********************************************************************/

#ifndef _O_STD_GEOMETRY_H_
#define _O_STD_GEOMETRY_H_        1

#include <idTypes.h>
#include <mtcDef.h>
#include <mtc.h>
#include <stdTypes.h>


extern mtdModule            stdGeometry;

static IDE_RC stdInitialize( UInt aNo );

static IDE_RC stdEstimate(
                    UInt*            aColumnSize,
                    UInt*            aArguments,
                    SInt*            aPrecision,
                    SInt*            aScale );

static IDE_RC stdValue(
                    mtcTemplate*     aTemplate,
                    mtcColumn*       aColumn,
                    void*            aValue,
                    UInt*            aValueOffset,
                    UInt             aValueSize,
                    const void*      aToken,
                    UInt             aTokenLength,
                    IDE_RC*          aResult );

static UInt stdActualSize(
                    const mtcColumn* aColumn,
                    const void*      aRow );

static IDE_RC stdGetPrecision( const mtcColumn * aColumn,
                               const void      * aRow,
                               SInt            * aPrecision,
                               SInt            * aScale );

static void stdNull( const mtcColumn* aColumn,
                     void*            aRow );

static UInt stdHash(
                    UInt             aHash,
                    const mtcColumn* aColumn,
                    const void*      aRow );

static idBool stdIsNull( const mtcColumn* aColumn,
                         const void*      aRow );

static SInt stdGeometryKeyComp( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 );

static IDE_RC stdCanonize( const mtcColumn * aCanon,
                           void**            aCanonized,
                           mtcEncryptInfo  * aCanonInfo,
                           const mtcColumn * aColumn,
                           void*             aValue,
                           mtcEncryptInfo  * aColumnInfo,
                           mtcTemplate     * aTemplate );

static void stdEndian( void* aValue );

static IDE_RC stdStoredValue2StdValue( UInt              aColumnSize,
                                       void            * aDestValue,
                                       UInt              aDestValueOffset,
                                       UInt              aLength,
                                       const void      * aValue );

static UInt stdNullValueSize();

static UInt stdHeaderSize();

static mtcName stdTypeName[8] = {
    { stdTypeName+1, STD_GEOMETRY_NAME_LEN,        (void*)STD_GEOMETRY_NAME        },
    { stdTypeName+2, STD_POINT_NAME_LEN,           (void*)STD_POINT_NAME           },
    { stdTypeName+3, STD_LINESTRING_NAME_LEN,      (void*)STD_LINESTRING_NAME      },
    { stdTypeName+4, STD_POLYGON_NAME_LEN,         (void*)STD_POLYGON_NAME         },
    { stdTypeName+5, STD_MULTIPOINT_NAME_LEN,      (void*)STD_MULTIPOINT_NAME      },
    { stdTypeName+6, STD_MULTILINESTRING_NAME_LEN, (void*)STD_MULTILINESTRING_NAME },
    { stdTypeName+7, STD_MULTIPOLYGON_NAME_LEN,    (void*)STD_MULTIPOLYGON_NAME    },
    { NULL,          STD_GEOCOLLECTION_NAME_LEN,   (void*)STD_GEOCOLLECTION_NAME   }
};


#endif /* _O_STD_GEOMETRY_H_ */


