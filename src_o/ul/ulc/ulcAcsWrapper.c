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
 *
 *    Spatio-Temporal API Wrapper Date ÇÔ¼ö
 *
 ***********************************************************************/

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <sqlcli.h>
#include <ulsEnvHandle.h>
#include <ulsCreateObject.h>
#include <ulsSearchObject.h>
#include <ulsByteOrder.h>
#include <ulsAPI.h>

/*----------------------------------------------------------------*
 *  Handle Management
 *----------------------------------------------------------------*/

// Alloc Environment
ACSRETURN ACSAllocEnv(ACSHENV * aHandle)
{
    return ulsAllocEnv( (ulsHandle**) aHandle );
}

// Free Environment
ACSRETURN
ACSFreeEnv(ACSHENV aHandle)
{
    return ulsFreeEnv( (ulsHandle*) aHandle );
}

// Get Error Information
ACSRETURN
ACSError( ACSHENV       aHandle,
          SQLUINTEGER * aErrorCode,
          SQLSCHAR   ** aErrorMessage,
          SQLSMALLINT * aErrorMessageLength )
{
    return ulsGetError( (ulsHandle*) aHandle,
                        (acp_uint32_t*) aErrorCode,
                        (acp_char_t**) aErrorMessage,
                        aErrorMessageLength );
}
                        
/*----------------------------------------------------------------*
 *  Create Objects
 *----------------------------------------------------------------*/

// Create 2D Point Ojbect
ACSRETURN
ACSCreatePoint2D( ACSHENV             aHandle,
                  stdGeometryType   * aBuffer,
                  SQLLEN              aBufferSize,
                  stdPoint2D        * aPoint,
                  SQLINTEGER          aSRID,
                  SQLLEN            * aObjLength )
{
    return ulsCreatePoint2D( (ulsHandle*) aHandle,
                             aBuffer,
                             aBufferSize,
                             aPoint,
                             aSRID,
                             (ulvSLen*) aObjLength );
}


// Create 2D LineString Object
ACSRETURN
ACSCreateLineString2D( ACSHENV             aHandle,
                       stdGeometryType   * aBuffer,
                       SQLLEN              aBufferSize,
                       SQLUINTEGER         aNumPoints,
                       stdPoint2D        * aPoints,
                       SQLINTEGER          aSRID,
                       SQLLEN            * aObjLength )
{
    return ulsCreateLineString2D( (ulsHandle*) aHandle,
                                  aBuffer,
                                  aBufferSize,
                                  aNumPoints,
                                  aPoints,
                                  aSRID,
                                  (ulvSLen*) aObjLength );
}


// Create 2D LinearRing Object
ACSRETURN
ACSCreateLinearRing2D( ACSHENV               aHandle,
                       stdLinearRing2D     * aBuffer,
                       SQLLEN                aBufferSize,
                       SQLUINTEGER           aNumPoints,
                       stdPoint2D          * aPoints,
                       SQLLEN              * aObjLength )
{
    return ulsCreateLinearRing2D( (ulsHandle*) aHandle,
                                  aBuffer,
                                  aBufferSize,
                                  aNumPoints,
                                  aPoints,
                                  (ulvSLen*) aObjLength );
}


// Create 2D Polygon Object
ACSRETURN
ACSCreatePolygon2D( ACSHENV                  aHandle,
                    stdGeometryType        * aBuffer,
                    SQLLEN                   aBufferSize,
                    SQLUINTEGER              aNumRings,
                    stdLinearRing2D       ** aRings,
                    SQLINTEGER               aSRID,
                    SQLLEN                 * aObjLength )
{
    return ulsCreatePolygon2D( (ulsHandle*) aHandle,
                               aBuffer,
                               aBufferSize,
                               aNumRings,
                               aRings,
                               aSRID,
                               (ulvSLen*) aObjLength );
}



// Create 2D MultiPoint Object
ACSRETURN
ACSCreateMultiPoint2D( ACSHENV                   aHandle,
                       stdGeometryType         * aBuffer,
                       SQLLEN                    aBufferSize,
                       SQLUINTEGER               aNumPoints,
                       stdPoint2DType         ** aPoints,
                       SQLLEN                  * aObjLength )
{
    return ulsCreateMultiPoint2D( (ulsHandle*) aHandle,
                                  aBuffer,
                                  aBufferSize,
                                  aNumPoints,
                                  aPoints,
                                  (ulvSLen*) aObjLength );
    
}


// Create 2D MultiLineString Object
ACSRETURN
ACSCreateMultiLineString2D( ACSHENV                   aHandle,
                            stdGeometryType         * aBuffer,
                            SQLLEN                    aBufferSize,
                            SQLUINTEGER               aNumLineStrings,
                            stdLineString2DType    ** aLineStrings,
                            SQLLEN                  * aObjLength )
{
    return ulsCreateMultiLineString2D( (ulsHandle*) aHandle,
                                       aBuffer,
                                       aBufferSize,
                                       aNumLineStrings,
                                       aLineStrings,
                                       (ulvSLen*) aObjLength );
}


// Create 2D MultiPolygon Object
ACSRETURN
ACSCreateMultiPolygon2D( ACSHENV                   aHandle,
                         stdGeometryType         * aBuffer,
                         SQLLEN                    aBufferSize,
                         SQLUINTEGER               aNumPolygons,
                         stdPolygon2DType       ** aPolygons,
                         SQLLEN                  * aObjLength )
{
    return ulsCreateMultiPolygon2D( (ulsHandle*) aHandle,
                                    aBuffer,
                                    aBufferSize,
                                    aNumPolygons,
                                    aPolygons,
                                    (ulvSLen*) aObjLength );
}


// Create 2D GeometryCollection Object
ACSRETURN
ACSCreateGeomCollection2D( ACSHENV                   aHandle,
                           stdGeometryType         * aBuffer,
                           SQLLEN                    aBufferSize,
                           SQLUINTEGER               aNumGeometries,
                           stdGeometryType        ** aGeometries,
                           SQLLEN                  * aObjLength )
{
    return ulsCreateGeomCollection2D( (ulsHandle*) aHandle,
                                      aBuffer,
                                      aBufferSize,
                                      aNumGeometries,
                                      aGeometries,
                                      (ulvSLen*) aObjLength );
}


/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  : Geometry
 *----------------------------------------------------------------*/
// Get GeometryType
ACSRETURN
ACSGetGeometryType( ACSHENV                   aHandle,
                    stdGeometryType         * aGeometry,
                    stdGeoTypes             * aGeoType )
{
    return ulsGetGeometryType( (ulsHandle*) aHandle,
                               aGeometry,
                               aGeoType );
}

// Get Geometry Size
ACSRETURN
ACSGetGeometrySize( ACSHENV                  aHandle,
                    stdGeometryType        * aGeometry,
                    SQLLEN                 * aSize  )
{
    return ulsGetGeometrySize( (ulsHandle*) aHandle,
                               aGeometry,
                               (ulvSLen*) aSize );
}

// Get Geometry Size From WKB
// BUG - 28091

ACSRETURN ACSGetGeometrySizeFromWKB( ACSHENV       aHandle,
                                     SQLCHAR     * aWKB,
                                     SQLUINTEGER   aWKBLength,
                                     SQLLEN      * aSize )
{
    return ulsGetGeometrySizeFromWKB( (ulsHandle*) aHandle,
                                      aWKB,
                                      aWKBLength,
                                      (ulvSLen*) aSize);
}

ACSRETURN ACSGetGeometrySRID( ACSHENV            aHandle,
                              stdGeometryType  * aGeometry,
                              SQLINTEGER       * aSRID )
{
    return ulsGetSRID( aHandle,
                       aGeometry,
                       aSRID );
}

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D & 3D MultiPoint, MultiLineString, MultiGeometryCollection
 *----------------------------------------------------------------*/
// Get Geometries Num 
ACSRETURN
ACSGetNumGeometries( ACSHENV                   aHandle,
                     stdGeometryType         * aGeometry,
                     SQLUINTEGER             * aNumGeometries )
{
    return ulsGetNumGeometries( (ulsHandle*) aHandle,
                                aGeometry,
                                (acp_uint32_t*) aNumGeometries );
    

}


// Get Nth Geometry 
ACSRETURN
ACSGetGeometryN( ACSHENV                   aHandle,
                 stdGeometryType         * aGeometry,
                 SQLUINTEGER               aNth,
                 stdGeometryType        ** aSubGeometry )
{
    return ulsGetGeometryN( (ulsHandle*) aHandle,
                            aGeometry,
                            aNth,
                            aSubGeometry );
}


/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D Polygon 
 *----------------------------------------------------------------*/
// Get Exterior Ring : 2D Polygon 
ACSRETURN
ACSGetExteriorRing2D( ACSHENV                    aHandle,
                      stdPolygon2DType         * aPolygon,
                      stdLinearRing2D         ** aLinearRing )
{
    return ulsGetExteriorRing2D( (ulsHandle*) aHandle,
                                 aPolygon,
                                 aLinearRing );
}


// Get InteriorRing Num : 2D Polygon 
ACSRETURN
ACSGetNumInteriorRing2D( ACSHENV             aHandle,
                         stdPolygon2DType  * aPolygon,
                         SQLUINTEGER       * aNumInterinor )
{
    return ulsGetNumInteriorRing2D( (ulsHandle*) aHandle,
                                    aPolygon,
                                    (acp_uint32_t*) aNumInterinor );
}


// Get Nth InteriorRing : 2D Polygon 
ACSRETURN
ACSGetInteriorRingNPolygon2D( ACSHENV                 aHandle,
                              stdPolygon2DType      * aPolygon,
                              SQLUINTEGER             aNth,
                              stdLinearRing2D      ** aLinearRing )
{
    return ulsGetInteriorRingNPolygon2D( (ulsHandle*) aHandle,
                                         aPolygon,
                                         (acp_uint32_t) aNth,
                                         aLinearRing );
}


/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D LineString
 *----------------------------------------------------------------*/
// Get Points num : 2D LineString 
ACSRETURN
ACSGetNumPointsLineString2D( ACSHENV                 aHandle,
                             stdLineString2DType   * aLineString,
                             SQLUINTEGER           * aNumPoints )
{
    return ulsGetNumPointsLineString2D( (ulsHandle*) aHandle,
                                        aLineString,
                                        (acp_uint32_t*) aNumPoints );
}


// Get Nth Point : 2D LineString 
ACSRETURN
ACSGetPointNLineString2D( ACSHENV                 aHandle,
                          stdLineString2DType   * aLineString,
                          SQLUINTEGER             aNth, 
                          stdPoint2D            * aPoint )
{
    return ulsGetPointNLineString2D( (ulsHandle*) aHandle,
                                     aLineString,
                                     aNth, 
                                     aPoint );
}


// Get Points pointer : 2D LineString 
ACSRETURN
ACSGetPointsLineString2D( ACSHENV                 aHandle,
                          stdLineString2DType   * aLineString,
                          stdPoint2D           ** aPoints )
{
    return ulsGetPointsLineString2D( (ulsHandle*) aHandle,
                                     aLineString,
                                     aPoints );
}



/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D LinearRing
 *----------------------------------------------------------------*/
// Get Points num : 2D LinearRing 
ACSRETURN ACSGetNumPointsLinearRing2D( ACSHENV                 aHandle,
                                       stdLinearRing2D       * aLinearRing,
                                       SQLUINTEGER           * aNumPoints )
{
    return ulsGetNumPointsLinearRing2D( (ulsHandle*) aHandle,
                                        aLinearRing,
                                        (acp_uint32_t*) aNumPoints );
}


// Get Nth Point : 2D LinearRing 
ACSRETURN ACSGetPointNLinearRing2D( ACSHENV                 aHandle,
                                    stdLinearRing2D       * aLinearRing,
                                    SQLUINTEGER             aNth, 
                                    stdPoint2D            * aPoint )
{
    return ulsGetPointNLinearRing2D( (ulsHandle*) aHandle,
                                     aLinearRing,
                                     aNth, 
                                     aPoint );
}


// Get Points pointer : 2D LinearRing 
ACSRETURN ACSGetPointsLinearRing2D( ACSHENV                 aHandle,
                                    stdLinearRing2D       * aLinearRing,
                                    stdPoint2D           ** aPoints )
{
    return ulsGetPointsLinearRing2D( (ulsHandle*) aHandle,
                                     aLinearRing,
                                     aPoints );
}


/*----------------------------------------------------------------*
 *  Byte Order Interface
 *----------------------------------------------------------------*/

// Adjust Byte Order
ACSRETURN ACSAdjustByteOrder( ACSHENV           aHandle,
                              stdGeometryType * aObject )
{
    return ulsAdjustByteOrder( (ulsHandle*) aHandle,
                               aObject );
}


// Change Byte Order 
ACSRETURN ACSEndian( ACSHENV             aHandle,
                     stdGeometryType   * aObject )
    
{
    return ulsEndian( (ulsHandle*) aHandle,
                      aObject );
}

