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
 * $Id$
 **********************************************************************/

#include <idl.h> // for win32 porting
#include <mtf.h>

extern mtfModule stfSt_Geometry;
extern mtfModule stfDimension;
extern mtfModule stfGeometryType;
extern mtfModule stfAsText;
extern mtfModule stfAsBinary;
extern mtfModule stfIsEmpty;
extern mtfModule stfIsSimple;
extern mtfModule stfBoundary;
extern mtfModule stfEnvelope;
extern mtfModule stfCoordX;
extern mtfModule stfCoordY;

extern mtfModule stfEquals;
extern mtfModule stfDisjoint;
extern mtfModule stfTouches;
extern mtfModule stfWithin;
extern mtfModule stfOverlaps;
extern mtfModule stfCrosses;
extern mtfModule stfIntersects;
extern mtfModule stfContains;
extern mtfModule stfRelate;

extern mtfModule stfNotEquals;
extern mtfModule stfNotTouches;
extern mtfModule stfNotWithin;
extern mtfModule stfNotOverlaps;
extern mtfModule stfNotCrosses;
extern mtfModule stfNotContains;
extern mtfModule stfNotRelate;

extern mtfModule stfDistance;
extern mtfModule stfIntersection;
extern mtfModule stfDifference;
extern mtfModule stfUnion;
extern mtfModule stfSymDifference;
extern mtfModule stfBuffer;
extern mtfModule stfConvexHull;
extern mtfModule stfGeomFromText;
extern mtfModule stfPointFromText;
extern mtfModule stfLineFromText;
extern mtfModule stfPolyFromText;
extern mtfModule stfPolygonFromText;
extern mtfModule stfRectFromText;
extern mtfModule stfMPointFromText;
extern mtfModule stfMLineFromText;
extern mtfModule stfMPolyFromText;
extern mtfModule stfGeoCollectionFromText;
extern mtfModule stfGeomFromWKB;
extern mtfModule stfPointFromWKB;
extern mtfModule stfLineFromWKB;
extern mtfModule stfLinestringFromWKB;
extern mtfModule stfPolyFromWKB;
extern mtfModule stfRectFromWKB;
extern mtfModule stfMPointFromWKB;
extern mtfModule stfMLineFromWKB;
extern mtfModule stfMPolyFromWKB;
extern mtfModule stfGeoCollectionFromWKB;

// PROJ-2422 srid 지원
extern mtfModule stfAsEWKT;
extern mtfModule stfAsEWKB;
extern mtfModule stfGeomFromEWKT;
extern mtfModule stfSRID;
extern mtfModule stfSetSRID;

extern mtfModule stfStartPoint;
extern mtfModule stfEndPoint;
extern mtfModule stfIsRing;
extern mtfModule stfIsClosed;
extern mtfModule stfNumPoints;
extern mtfModule stfPointN;
extern mtfModule stfCentroid;
extern mtfModule stfPointOnSurface;
extern mtfModule stfArea;
extern mtfModule stfExteriorRing;
extern mtfModule stfNumInteriorRing;
extern mtfModule stfInteriorRingN;
extern mtfModule stfNumGeometries;
extern mtfModule stfGeometryN;
extern mtfModule stfLength;
// BUG-22924
extern mtfModule stfIsValid;
// BUG-33576
extern mtfModule stfIsValidHeader;

extern mtfModule stfMinX;
extern mtfModule stfMinY;
extern mtfModule stfMaxX;
extern mtfModule stfMaxY;

// BUG-23163 
extern mtfModule stfIsMBRIntersects;
extern mtfModule stfIsMBRContains;
extern mtfModule stfIsMBRWithin;

/* BUG-45645 ST_Reverse, ST_MakeEnvelope 함수 지원 */
extern mtfModule stfReverse;
extern mtfModule stfMakeEnvelope;

/* BUG-47857 ST_MakePoint, ST_Point 함수 지원 */
extern mtfModule stfMakePoint;

/* BUG-45883 ST_MakeLine 함수 지원 */
extern mtfModule stfMakeLine;

/* BUG-47816 ST_Transform 함수 지원 */
extern mtfModule stfTransform;

/* BUG-47931 ST_MakePolygon, ST_Polygon 함수 지원 */ 
extern mtfModule stfMakePolygon;
extern mtfModule stfPolygon;

/* BUG-48014 ST_Collect 함수지원 */
extern mtfModule stfCollect;

/* BUG-48038 ST_IsCollection 함수 지원 */ 
extern mtfModule stfIsCollection;

mtfModule* stfModules[] = {
    &stfSt_Geometry,
    &stfDimension,
    &stfGeometryType,
    &stfAsText,
    &stfAsBinary,
    &stfIsEmpty,
    &stfIsSimple,
    &stfBoundary,
    &stfEnvelope,
    &stfCoordX,
    &stfCoordY,

    &stfEquals,
    &stfDisjoint,
    &stfTouches,
    &stfWithin,
    &stfOverlaps,
    &stfCrosses,
    &stfIntersects,
    &stfContains,
    &stfRelate,

    &stfNotEquals,
    &stfNotTouches,
    &stfNotWithin,
    &stfNotOverlaps,
    &stfNotCrosses,
    &stfNotContains,
    &stfNotRelate,

    &stfDistance,
    &stfIntersection,
    &stfDifference,
    &stfUnion,
    &stfSymDifference,
    &stfBuffer,
    &stfConvexHull,
    &stfGeomFromText,
    &stfPointFromText,
    &stfLineFromText,
    &stfPolyFromText,
    &stfPolygonFromText,
    &stfRectFromText,
    &stfMPointFromText,
    &stfMLineFromText,
    &stfMPolyFromText,
    &stfGeoCollectionFromText,
    &stfGeomFromWKB,
    &stfPointFromWKB,
    &stfLineFromWKB,
    &stfLinestringFromWKB,
    &stfPolyFromWKB,
    &stfRectFromWKB,
    &stfMPointFromWKB,
    &stfMLineFromWKB,
    &stfMPolyFromWKB,
    &stfGeoCollectionFromWKB,

    // PROJ-2422 srid 지원
    &stfAsEWKT,
    &stfAsEWKB,
    &stfGeomFromEWKT,
    &stfSRID,
    &stfSetSRID,
    
    &stfStartPoint,
    &stfEndPoint,
    &stfIsRing,
    &stfIsClosed,
    &stfNumPoints,
    &stfPointN,
    &stfCentroid,
    &stfPointOnSurface,
    &stfArea,
    &stfExteriorRing,
    &stfNumInteriorRing,
    &stfInteriorRingN,
    &stfNumGeometries,
    &stfGeometryN,
    &stfLength,
    // BUG-22924
    &stfIsValid,
    // BUG-33576
    &stfIsValidHeader,
    &stfMinX,
    &stfMinY,
    &stfMaxX,
    &stfMaxY,
    &stfIsMBRIntersects,
    &stfIsMBRContains,
    &stfIsMBRWithin,
    /* BUG-45645 ST_Reverse, ST_MakeEnvelope 함수 지원 */
    &stfReverse,
    &stfMakeEnvelope,
    /* BUG-47857 ST_MakePoint, ST_Point 함수 지원 */
    &stfMakePoint,
    /* BUG-47883 ST_MakeLine 함수 지원 */
    &stfMakeLine,
    /* BUG-47816 ST_Transform 함수 지원 */
#if defined(ST_ENABLE_PROJ4_LIBRARY)
    &stfTransform,
#endif /* ST_ENABLE_PROJ4_LIBRARY */
    /* BUG-47931 ST_MakePolygon, ST_Polygon 함수 지원 */ 
    &stfMakePolygon,
    &stfPolygon,
    /* BUG-48014 ST_Collect 함수지원 */
    &stfCollect,
    /* BUG-48038 ST_IsCollection 함수 지원 */ 
    &stfIsCollection,
    NULL
};
