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
 * Spatio-Temporal Object 생성 함수 
 *
 ***********************************************************************/

#ifndef _O_ULS_CREATE_OBJECT_H_
#define _O_ULS_CREATE_OBJECT_H_    (1)

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>

#include <ulsEnvHandle.h>
#include <ulsTypes.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *
 * To Fix BUG-17186 use SQLLEN 
 *----------------------------------------------------------------*/

/* Create 2D Point Object*/
ACSRETURN
ulsCreatePoint2D( ulsHandle               * aHandle,
                  stdGeometryType         * aBuffer,
                  ulvSLen                   aBufferSize,
                  stdPoint2D              * aPoint,
                  acp_sint32_t              aSRID,
                  ulvSLen                 * aObjLength );

/* Create 2D LineString Object*/
ACSRETURN
ulsCreateLineString2D( ulsHandle         * aHandle,
                       stdGeometryType   * aBuffer,
                       ulvSLen             aBufferSize,
                       acp_uint32_t        aNumPoints,
                       stdPoint2D        * aPoints,
                       acp_sint32_t        aSRID,
                       ulvSLen           * aObjLength );

/* Create 2D LinearRing Object*/
ACSRETURN
ulsCreateLinearRing2D( ulsHandle           * aHandle,
                       stdLinearRing2D     * aBuffer,
                       ulvSLen               aBufferSize,
                       acp_uint32_t          aNumPoints,
                       stdPoint2D          * aPoints,
                       ulvSLen             * aObjLength );

/* Create 2D Polygon Object*/
ACSRETURN
ulsCreatePolygon2D( ulsHandle              * aHandle,
                    stdGeometryType        * aBuffer,
                    ulvSLen                  aBufferSize,
                    acp_uint32_t             aNumRings,
                    stdLinearRing2D       ** aRings,
                    acp_sint32_t             aSRID,
                    ulvSLen                * aObjLength );


/* Create 2D MultiPoint Object*/
ACSRETURN
ulsCreateMultiPoint2D( ulsHandle               * aHandle,
                       stdGeometryType         * aBuffer,
                       ulvSLen                   aBufferSize,
                       acp_uint32_t              aNumPoints,
                       stdPoint2DType         ** aPoints,
                       ulvSLen                 * aObjLength );

/* Create 2D MultiLineString Object*/
ACSRETURN
ulsCreateMultiLineString2D( ulsHandle                   * aHandle,
                            stdGeometryType             * aBuffer,
                            ulvSLen                       aBufferSize,
                            acp_uint32_t                  aNumLineStrings,
                            stdLineString2DType        ** aLineStrings,
                            ulvSLen                     * aObjLength );


/* Create 2D MultiPolygon Object*/
ACSRETURN
ulsCreateMultiPolygon2D( ulsHandle               * aHandle,
                         stdGeometryType         * aBuffer,
                         ulvSLen                   aBufferSize,
                         acp_uint32_t              aNumPolygons,
                         stdPolygon2DType       ** aPolygons,
                         ulvSLen                 * aObjLength );

/* Create 2D GeometryCollection Object*/
ACSRETURN
ulsCreateGeomCollection2D( ulsHandle               * aHandle,
                           stdGeometryType         * aBuffer,
                           ulvSLen                   aBufferSize,
                           acp_uint32_t              aNumGeometries,
                           stdGeometryType        ** aGeometries,
                           ulvSLen                 * aObjLength );


/* TODO - 다양한 객체 생성을 위한 함수를 생성하여야 함.*/
/**/

/**/
/* Temporal 2D Geometry객체의 생성*/
/**/

/**/
/* Temporal 3D Geometry객체의 생성*/
/**/


/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/

/* Geometry Header의 초기화 */
ACI_RC
ulsInitHeader( ulsHandle          * aHandle,
               stdGeometryHeader  * aObjectHeader );

/* Geometry Type의 설정*/
ACI_RC
ulsSetGeoType( ulsHandle          * aHandle,
               stdGeometryHeader  * aObjHeader,
               stdGeoTypes          aType );

/* Geometry Type의 획득*/
ACI_RC
ulsGetGeoType( ulsHandle          * aHandle,
               stdGeometryHeader  * aObjHeader,
               stdGeoTypes        * aType );

/* 다목적 멀티객체 생성함수*/
ACSRETURN
ulsCreateMultiGeometry( ulsHandle               * aHandle,
                        stdGeometryType         * aBuffer,
                        ulvSLen                   aBufferSize,
                        stdGeoTypes               aGeoTypes,
                        stdGeoTypes               aSubGeoTypes,
                        acp_uint32_t              aIs2D,
                        acp_uint32_t              aNumGeometries,
                        stdGeometryType        ** aGeometries,
                        ulvSLen                 * aObjLength );


ACI_RC
ulsRecalcMBR(  ulsHandle                * aHandle,
               stdGeometryType          * aObj,
               stdMBR                   * aMbr  );

ACI_RC
ulsGetSRID(  ulsHandle         * aHandle,
             stdGeometryType   * aObj,
             acp_sint32_t      * aSRID  );

#endif /* _O_ULS_CREATE_OBJECT_H_ */


