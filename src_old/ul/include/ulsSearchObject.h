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
 * Spatio-Temporal Object 검색 함수 
 *
 ***********************************************************************/

#ifndef _O_ULS_SEARCH_OBJECT_H_
#define _O_ULS_SEARCH_OBJECT_H_    (1)

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>

#include <ulsEnvHandle.h>
#include <ulsTypes.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/
/* Get GeometryType*/
ACSRETURN
ulsGetGeometryType( ulsHandle               * aHandle,
                    stdGeometryType         * aGeometry,
                    stdGeoTypes             * aGeoType );

ACSRETURN
ulsGetGeometrySize( ulsHandle              * aHandle,
                    stdGeometryType        * aGeometry,
                    ulvSLen                * aSize  );

ACSRETURN
ulsGetGeometrySizeFromWKB( ulsHandle        * aHandle,
                           acp_uint8_t      * aWKB,
                           acp_uint32_t       aWKBLength,
                           ulvSLen          * aGeomtrySize);

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D & 3D MultiPoint, MultiLineString, MultiGeometryCollection
 *----------------------------------------------------------------*/
/* Get Geometries Num */
ACSRETURN
ulsGetNumGeometries( ulsHandle               * aHandle,
                     stdGeometryType         * aGeometry,
                     acp_uint32_t            * aNumGeometries );

/* Get Nth Geometry */
ACSRETURN
ulsGetGeometryN( ulsHandle               * aHandle,
                 stdGeometryType         * aGeometry,
                 acp_uint32_t              aNth,
                 stdGeometryType        ** aSubGeometry );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D Polygon 
 *----------------------------------------------------------------*/
/* Get Exterior Ring : 2D Polygon */
ACSRETURN
ulsGetExteriorRing2D( ulsHandle                * aHandle,
                      stdPolygon2DType         * aPolygon,
                      stdLinearRing2D         ** aLinearRing );

/* Get InteriorRing Num : 2D Polygon */
ACSRETURN
ulsGetNumInteriorRing2D( ulsHandle               * aHandle,
                         stdPolygon2DType        * aPolygon,
                         acp_uint32_t            * aNumInterinor );

/* Get Nth InteriorRing : 2D Polygon */
ACSRETURN
ulsGetInteriorRingNPolygon2D( ulsHandle                * aHandle,
                              stdPolygon2DType         * aPolygon,
                              acp_uint32_t               aNth,
                              stdLinearRing2D         ** aLinearRing );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D LineString
 *----------------------------------------------------------------*/
/* Get Points num : 2D LineString */
ACSRETURN
ulsGetNumPointsLineString2D( ulsHandle                   * aHandle,
                             stdLineString2DType         * aLineString,
                             acp_uint32_t                * aNumPoints );

/* Get Nth Point : 2D LineString */
ACSRETURN
ulsGetPointNLineString2D( ulsHandle                   * aHandle,
                          stdLineString2DType         * aLineString,
                          acp_uint32_t                  aNth,
                          stdPoint2D                  * aPoint );

/* Get Points pointer : 2D LineString */
ACSRETURN
ulsGetPointsLineString2D( ulsHandle                   * aHandle,
                          stdLineString2DType         * aLineString,
                          stdPoint2D                 ** aPoints );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D LinearRing
 *----------------------------------------------------------------*/
/* Get Points num : 2D LinearRing */
ACSRETURN
ulsGetNumPointsLinearRing2D( ulsHandle        * aHandle,
                             stdLinearRing2D  * aLinearRing,
                             acp_uint32_t     * aNumPoints );

/* Get Nth Point : 2D LinearRing */
ACSRETURN
ulsGetPointNLinearRing2D( ulsHandle        * aHandle,
                          stdLinearRing2D  * aLinearRing,
                          acp_uint32_t       aNth,
                          stdPoint2D       * aPoint );

/* Get Points pointer : 2D LinearRing */
ACSRETURN
ulsGetPointsLinearRing2D( ulsHandle       * aHandle,
                          stdLinearRing2D * aLinearRing,
                          stdPoint2D     ** aPoints );


/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/
ACI_RC
ulsCheckGeometry( ulsHandle       * aHandle,
                  stdGeometryType * aGeometry );

#endif /* _O_ULS_SEARCH_OBJECT_H_ */



