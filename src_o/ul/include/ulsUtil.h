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

#ifndef _O_ULS_UTIL_H_
#define _O_ULS_UTIL_H_    (1)

stdPoint2D *ulsM_GetPointsLS2D( stdLineString2DType *aLineString );
stdPoint2D *ulsM_GetPointsLR2D( stdLinearRing2D *aLinearRing );

void
ulsM_GetMBR2D( acp_uint32_t       aNumPoints,
               stdPoint2D       * aPoints,
               stdMBR           * aMbr );

void
ulsM_ExpandMBR( stdMBR *aMaster, stdMBR *aOther );

acp_sint32_t  ulsM_IsGeometry2DType( stdGeoTypes aType );

acp_sint32_t  ulsM_IsGeometry2DExtType( stdGeoTypes aType );

acp_sint32_t  ulsM_GetSRID( stdGeometryType * aGeometry );

void
ulsM_SetSRID( stdGeometryType * aGeometry, acp_sint32_t aSRID );

/**/
/* Internal Function*/
/**/


/*read WKB Header*/
ACI_RC
readWKB_Header( ulsHandle       *aHandle,
                acp_uint8_t     *aBuf,
                acp_bool_t      *aIsEquiEndian,
                acp_uint32_t    *aType,
                acp_uint32_t    *aOffset );

/*read Unsigned Int*/
void
readWKB_UInt( acp_uint8_t   *aBuf,
              acp_uint32_t  *aVal,
              acp_uint32_t  *aOffset,
              acp_bool_t     aEquiEndian);

/*read Signed Int*/
void
readWKB_SInt( acp_uint8_t   *aBuf,
              acp_sint32_t  *aVal,
              acp_uint32_t  *aOffset,
              acp_bool_t     aEquiEndian );

/* Calc Geometry Size*/

ACI_RC
ulsGetPoint2DSize( ulsHandle              * aHandle,
                   acp_uint32_t           * aSize );

ACI_RC
ulsGetPoint2DExtSize( ulsHandle              * aHandle,
                      acp_uint32_t           * aSize );

ACI_RC
ulsGetLineString2DSize( ulsHandle              * aHandle,
                        acp_uint32_t             aNumPoints,
                        acp_uint32_t           * aSize  );

ACI_RC
ulsGetLinearRing2DSize( ulsHandle              * aHandle,
                        acp_uint32_t             aNumPoints,
                        acp_uint32_t           * aSize  );

ACI_RC
ulsGetPolygon2DSize( ulsHandle                 * aHandle,
                     acp_uint32_t                aNumRings,
                     stdLinearRing2D          ** aRings,
                     acp_uint32_t              * aSize  );

ACSRETURN
ulsGetPointSizeFromWKB( ulsHandle              * aHandle,
                        acp_uint8_t           ** aPtr,
                        acp_uint8_t            * aWKBFence,
                        acp_uint32_t           * aSize);

ACSRETURN
ulsGetLineStringSizeFromWKB( ulsHandle         * aHandle,
                             acp_uint8_t      ** aPtr,
                             acp_uint8_t       * aWKBFence,
                             acp_uint32_t      * aSize);


ACSRETURN
ulsGetPolygonSizeFromWKB( ulsHandle            * aHandle,
                          acp_uint8_t         ** aPtr,
                          acp_uint8_t          * aWKBFence,
                          acp_uint32_t         * aSize);

ACSRETURN
ulsGetMultiPointSizeFromWKB( ulsHandle         * aHandle,
                             acp_uint8_t      ** aPtr,
                             acp_uint8_t       * aWKBFence,
                             acp_uint32_t      * aSize);

ACSRETURN
ulsGetMultiLineStringSizeFromWKB( ulsHandle    * aHandle,
                                  acp_uint8_t ** aPtr,
                                  acp_uint8_t  * aWKBFence,
                                  acp_uint32_t * aSize);


ACSRETURN
ulsGetMultiPolygonSizeFromWKB( ulsHandle       * aHandle,
                               acp_uint8_t    ** aPtr,
                               acp_uint8_t     * aWKBFence,
                               acp_uint32_t    * aSize);


ACSRETURN
ulsGetGeoCollectionSizeFromWKB( ulsHandle      * aHandle,
                                acp_uint8_t   ** aPtr,
                                acp_uint8_t    * aWKBFence,
                                acp_uint32_t   * aSize);

ACSRETURN
ulsGetPointSizeFromEWKB( ulsHandle              * aHandle,
                         acp_uint8_t           ** aPtr,
                         acp_uint8_t            * aWKBFence,
                         acp_uint32_t           * aSize );

ACSRETURN
ulsGetLineStringSizeFromEWKB( ulsHandle         * aHandle,
                              acp_uint8_t      ** aPtr,
                              acp_uint8_t       * aWKBFence,
                              acp_uint32_t      * aSize );


ACSRETURN
ulsGetPolygonSizeFromEWKB( ulsHandle            * aHandle,
                           acp_uint8_t         ** aPtr,
                           acp_uint8_t          * aWKBFence,
                           acp_uint32_t         * aSize );

ACSRETURN
ulsGetMultiPointSizeFromEWKB( ulsHandle         * aHandle,
                              acp_uint8_t      ** aPtr,
                              acp_uint8_t       * aWKBFence,
                              acp_uint32_t      * aSize );

ACSRETURN
ulsGetMultiLineStringSizeFromEWKB( ulsHandle    * aHandle,
                                   acp_uint8_t ** aPtr,
                                   acp_uint8_t  * aWKBFence,
                                   acp_uint32_t * aSize );


ACSRETURN
ulsGetMultiPolygonSizeFromEWKB( ulsHandle       * aHandle,
                                acp_uint8_t    ** aPtr,
                                acp_uint8_t     * aWKBFence,
                                acp_uint32_t    * aSize );


ACSRETURN
ulsGetGeoCollectionSizeFromEWKB( ulsHandle      * aHandle,
                                 acp_uint8_t   ** aPtr,
                                 acp_uint8_t    * aWKBFence,
                                 acp_uint32_t   * aSize );

/* Geometry Trace*/
ACI_RC
ulsSeekFirstRing2D( ulsHandle              * aHandle,
                    stdPolygon2DType       * aPolygon,
                    stdLinearRing2D       ** aRing );

ACI_RC
ulsSeekNextRing2D( ulsHandle              * aHandle,
                   stdLinearRing2D        * aRing,
                   stdLinearRing2D       ** aNextRing );
ACI_RC
ulsSeekFirstGeometry( ulsHandle              * aHandle,
                      stdGeometryType        * aGeometry,
                      stdGeometryType     ** aFirstGeometry );

ACI_RC
ulsSeekNextGeometry( ulsHandle              * aHandle,
                     stdGeometryType        * aGeometry,
                     stdGeometryType       ** aNextGeometry );



#endif /* _O_ULS_UTILE_H_ */
