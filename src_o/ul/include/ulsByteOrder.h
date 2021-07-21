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
 * Spatio-Temporal 기타 함수 
 *
 ***********************************************************************/

#ifndef _O_ULS_BYTE_ORDER_H_
#define _O_ULS_BYTE_ORDER_H_    (1)

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>

#include <ulsEnvHandle.h>
#include <ulsTypes.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/

/* Adjust Byte Order*/
ACSRETURN ulsAdjustByteOrder( ulsHandle         * aHandle,
                              stdGeometryType   * aObject );

/* Change Byte Order*/
ACSRETURN ulsEndian( ulsHandle         * aHandle,
                     stdGeometryType   * aObject );


/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/

void ulsEndianDouble( void * aValue );
void ulsEndianInteger( void * aValue );
void ulsEndianShort( void * aValue );

ACI_RC ulsIsEquiEndian( ulsHandle         * aHandle,
                        stdGeometryHeader * aObjHeader,
                        acp_bool_t        * aIsEquiEndian );

ACI_RC ulsEndianHeader( ulsHandle         * aHandle,
                        stdGeometryHeader * aObjHeader );

ACI_RC ulsEndianSRID( ulsHandle     * aHandle,
                      acp_sint32_t  * aSRID );

ACI_RC ulsEndianPoint2D( ulsHandle         * aHandle,
                         acp_bool_t          aIsEquiEndian,
                         stdPoint2DType    * aObject );

ACI_RC ulsEndianLineString2D( ulsHandle              * aHandle,
                              acp_bool_t               aIsEquiEndian,
                              stdLineString2DType    * aObject );

ACI_RC ulsEndianLinearRing2D(  ulsHandle        * aHandle,
                               acp_bool_t         aIsEquiEndian,
                               stdLinearRing2D  * aObject );

ACI_RC ulsEndianPolygon2D(    ulsHandle         * aHandle,
                              acp_bool_t          aIsEquiEndian,
                              stdPolygon2DType  * aObject );

ACI_RC ulsEndianMultiPoint2D( ulsHandle            * aHandle,
                              acp_bool_t             aIsEquiEndian,
                              stdMultiPoint2DType  * aObject );

ACI_RC ulsEndianMultiLineString2D( ulsHandle                 * aHandle,
                                   acp_bool_t                  aIsEquiEndian,
                                   stdMultiLineString2DType  * aObject );

ACI_RC ulsEndianMultiPolygon2D(    ulsHandle              * aHandle,
                                   acp_bool_t               aIsEquiEndian,
                                   stdMultiPolygon2DType  * aObject );

ACI_RC ulsEndianGeomCollection2D(  ulsHandle               * aHandle,
                                   acp_bool_t                aIsEquiEndian,
                                   stdGeoCollection2DType  * aObject );


#endif /* _O_ULS_BYTE_ORDER_H_ */

