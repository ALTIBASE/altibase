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
 * $Id: stdTypes.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description: Geometry 객체 자료 구조
 **********************************************************************/

#include <mtcdTypes.h>

#ifndef _O_STD_TYPES_H_
#define _O_STD_TYPES_H_         (1)

//------------------------------------------------
// PROJ-1586, BUG-15570
// Client와 Server가 자료 구조를 공유할 수 있도록 함.
//------------------------------------------------

// Native Geometry Object Types
#include <stdNativeTypes_c.i>

// WKB Geometry Object Types
#include <stdWKBTypes_c.i>

//------------------------------------------------
// Definitions
//------------------------------------------------

#define STD_GEOMETRY_ID         (MTD_GEOMETRY_ID)

extern stdGeometryHeader    stdGeometryNull;
extern stdGeometryHeader    stdGeometryEmpty;

#define STD_GEOMETRY_NAME                   "GEOMETRY"
#define STD_POINT_NAME                      "POINT"
#define STD_LINESTRING_NAME                 "LINESTRING"
#define STD_POLYGON_NAME                    "POLYGON"
#define STD_MULTIPOINT_NAME                 "MULTIPOINT"
#define STD_MULTILINESTRING_NAME            "MULTILINESTRING"
#define STD_MULTIPOLYGON_NAME               "MULTIPOLYGON"
#define STD_GEOCOLLECTION_NAME              "GEOMETRYCOLLECTION"
#define STD_NULL_NAME                       "NULL"
#define STD_EMPTY_NAME                      "EMPTY"


#define STD_GEOMETRY_NAME_LEN                    8
#define STD_POINT_NAME_LEN                       5
#define STD_LINESTRING_NAME_LEN                  10
#define STD_POLYGON_NAME_LEN                     7
#define STD_MULTIPOINT_NAME_LEN                  10
#define STD_MULTILINESTRING_NAME_LEN             15
#define STD_MULTIPOLYGON_NAME_LEN                12
#define STD_GEOCOLLECTION_NAME_LEN               18
#define STD_NULL_NAME_LEN                        4
#define STD_EMPTY_NAME_LEN                       5

#define STD_GEOMETRY_ALIGN                  (ACI_SIZEOF(SDouble))

#define STD_GEOMETRY_NAME_MAXIMUM           (32)

#define STD_Z_VALUE                         (0.)
//==============================================================================
// Fix BUG-15999
#define STD_MBR_SIZE            ACI_SIZEOF(stdMBR)
#define STD_GEOHEAD_SIZE        ACI_SIZEOF(stdGeometryHeader)
#define STD_PGEOHEAD_SIZE       ACI_SIZEOF(stdGeometryHeader*)
#define STD_PT2D_SIZE           ACI_SIZEOF(stdPoint2D)
#define STD_RN2D_SIZE           ACI_SIZEOF(stdLinearRing2D)
#define STD_POINT2D_SIZE        ACI_SIZEOF(stdPoint2DType)
#define STD_LINE2D_SIZE         ACI_SIZEOF(stdLineString2DType)
#define STD_POLY2D_SIZE         ACI_SIZEOF(stdPolygon2DType)
#define STD_MPOINT2D_SIZE       ACI_SIZEOF(stdMultiPoint2DType)
#define STD_MLINE2D_SIZE        ACI_SIZEOF(stdMultiLineString2DType)
#define STD_MPOLY2D_SIZE        ACI_SIZEOF(stdMultiPolygon2DType)
#define STD_COLL2D_SIZE         ACI_SIZEOF(stdGeoCollection2DType)

// PROJ-2422 srid 지원
// srid가 추가된 확장 타입
#define STD_POINT2D_EXT_SIZE    ACI_SIZEOF(stdPoint2DExtType)
#define STD_LINE2D_EXT_SIZE     ACI_SIZEOF(stdLineString2DExtType)
#define STD_POLY2D_EXT_SIZE     ACI_SIZEOF(stdPolygon2DExtType)
#define STD_MPOINT2D_EXT_SIZE   ACI_SIZEOF(stdMultiPoint2DExtType)
#define STD_MLINE2D_EXT_SIZE    ACI_SIZEOF(stdMultiLineString2DExtType)
#define STD_MPOLY2D_EXT_SIZE    ACI_SIZEOF(stdMultiPolygon2DExtType)
#define STD_COLL2D_EXT_SIZE     ACI_SIZEOF(stdGeoCollection2DExtType)

#define STD_N_POINTS(x)         ((x)->mNumPoints)
#define STD_N_RINGS(x)          ((x)->mNumRings)
#define STD_N_OBJECTS(x)        ((x)->mNumObjects)
#define STD_N_GEOMS(x)          ((x)->mNumGeometries)
#define STD_GEOM_SIZE(x)        ((x)->mSize)

//-------------------------------------------------------------
// Get Primitive Type

// From stdPoint
#define STD_PREV_PT2D(p)        ((stdPoint2D*)(p)-1)
#define STD_NEXT_PT2D(p)        ((stdPoint2D*)(p)+1)
#define STD_NEXTN_PT2D(p,n)     ((stdPoint2D*)(p)+(n))
                                
// From stdLinearRing
#define STD_NEXT_RN2D(r)        (stdLinearRing2D*)((acp_uint8_t*)(r+1) + \
                                STD_PT2D_SIZE*STD_N_POINTS(r))
// From stdLinearRing, stdLineStringType
#define STD_FIRST_PT2D(x)       (stdPoint2D*)((x)+1)
#define STD_LAST_PT2D(x)        (STD_FIRST_PT2D(x)+STD_N_POINTS(x)-1)
#define STD_POS_PT2D(x,n)       (STD_FIRST_PT2D(x)+(n))

// From stdPolygonType
#define STD_FIRST_RN2D(A)       (stdLinearRing2D*)((A)+1)

//-------------------------------------------------------------
// Get Object

// From stdPointType
/* BUG-45646 ST Reverse */
#define STD_PREV_POINT2D(P)     (stdPoint2DType*)((P)-1)
#define STD_NEXT_POINT2D(P)     (stdPoint2DType*)((P)+1)
#define STD_NEXTN_POINT2D(P,n)  ((stdPoint2DType*)(P)+(n))

// From stdLineStringType
#define STD_NEXT_LINE2D(L)      (stdLineString2DType*)\
                                ((acp_uint8_t*)(L)+STD_GEOM_SIZE(L))

// From stdPolygonType
#define STD_NEXT_POLY2D(A)      (stdPolygon2DType*)\
                                ((acp_uint8_t*)(A)+STD_GEOM_SIZE(A))

// From stdMultiPointType
#define STD_FIRST_POINT2D(MP)   (stdPoint2DType*)\
                                ((stdMultiPoint2DType*)(MP)+1)
/* BUG-45646 ST Reverse */
#define STD_LAST_POINT2D(MP)    (STD_FIRST_POINT2D(MP)+STD_N_OBJECTS(MP)-1)

// From stdMultiLineStringType
#define STD_FIRST_LINE2D(ML)    (stdLineString2DType*)\
                                ((stdMultiLineString2DType*)(ML)+1)
// From stdMultiPolygonType
#define STD_FIRST_POLY2D(MA)    (stdPolygon2DType*)\
                                ((stdMultiPolygon2DType*)(MA)+1)
// From stdGeoCollectionType
#define STD_FIRST_COLL2D(CO)    (stdGeometryType*)\
                                ((stdGeoCollection2DType*)(CO)+1)

#define STD_NEXT_GEOM(x)        (stdGeometryType*)\
                                ((acp_uint8_t*)(x) + \
                                ((stdGeometryHeader*)(x))->mSize)


//-------------------------------------------------------------
// WKB
#define WKB_GEOHEAD_SIZE        (5) //ACI_SIZEOF(WKBHeader)             ( 1 + 4 )
#define WKB_PT_SIZE             (16)//ACI_SIZEOF(wkbPoint)              ( 8 + 8 )
#define WKB_RN_SIZE             (4) //ACI_SIZEOF(wkbLinearRing)
#define WKB_POINT_SIZE          (21)//ACI_SIZEOF(WKBPoint)              ( 5 + 16)
#define WKB_LINE_SIZE           (9) //ACI_SIZEOF(WKBLineString)         ( 5 + 4 )
#define WKB_POLY_SIZE           (9) //ACI_SIZEOF(WKBPolygon)            ( 5 + 4 )
#define WKB_MPOINT_SIZE         (9) //ACI_SIZEOF(WKBMultiPoint)         ( 5 + 4 )
#define WKB_MLINE_SIZE          (9) //ACI_SIZEOF(WKBMultiLineString)    ( 5 + 4 )
#define WKB_MPOLY_SIZE          (9) //ACI_SIZEOF(WKBMultiPolygon)       ( 5 + 4 )
#define WKB_COLL_SIZE           (9) //ACI_SIZEOF(WKBGeometryCollection) ( 5 + 4 )
#define WKB_INT32_SIZE          (4)
#define WKB_DOUBLE_SIZE         (8)

#define EWKB_SRID_SIZE          (4)
#define EWKB_POINT_SIZE         (25)//ID_SIZEOF(EWKBPoint)              ( 5 + 16 + 4)
#define EWKB_LINE_SIZE          (13)//ID_SIZEOF(EWKBLineString)         ( 5 + 4 + 4 )
#define EWKB_POLY_SIZE          (13)//ID_SIZEOF(EWKBPolygon)            ( 5 + 4 + 4 )
#define EWKB_MPOINT_SIZE        (13)//ID_SIZEOF(EWKBMultiPoint)         ( 5 + 4 + 4 )
#define EWKB_MLINE_SIZE         (13)//ID_SIZEOF(EWKBMultiLineString)    ( 5 + 4 + 4 )
#define EWKB_MPOLY_SIZE         (13)//ID_SIZEOF(EWKBMultiPolygon)       ( 5 + 4 + 4 )
#define EWKB_COLL_SIZE          (13)//ID_SIZEOF(EWKBGeometryCollection) ( 5 + 4 + 4 )

#define WKB_N_PT(x)             (*((acp_uint32_t*)(x)->mNumPoints))
#define WKB_N_RN(x)             (*((acp_uint32_t*)(x)->mNumRings))
#define WKB_N_POINTS(x)         (*((acp_uint32_t*)(x)->mNumWKBPoints))
#define WKB_N_LINES(x)          (*((acp_uint32_t*)(x)->mNumWKBLineStrings))
#define WKB_N_POLYS(x)          (*((acp_uint32_t*)(x)->mNumWKBPolygons))
#define WKB_N_GEOMS(x)          (*((acp_uint32_t*)(x)->mNumWKBGeometries))

// From wkbPoint
#define WKB_NEXT_PT(p)          (wkbPoint*)((acp_uint8_t*)(p)+WKB_PT_SIZE)

// From wkbLinearRing
#define WKB_FIRST_PTR(r)        (wkbPoint*)((acp_uint8_t*)(r)+WKB_RN_SIZE)

// From WKBPolygon
#define WKB_FIRST_RN(A)         (wkbLinearRing*)((acp_uint8_t*)(A)+WKB_POLY_SIZE)

// From EWKBPolygon
#define EWKB_FIRST_RN(A)        (wkbLinearRing*)((acp_uint8_t*)(A)+EWKB_POLY_SIZE)

//-------------------------------------------------------------
// Get Object

// From WKBPoint
#define WKB_NEXT_POINT(P)       (WKBPoint*)((acp_uint8_t*)(P)+WKB_POINT_SIZE)

// From WKBLineString
#define WKB_FIRST_PTL(L)        (wkbPoint*)((acp_uint8_t*)(L)+WKB_LINE_SIZE)

// From EWKBLineString
#define EWKB_FIRST_PTL(L)       (wkbPoint*)((acp_uint8_t*)(L)+EWKB_LINE_SIZE)

// From WKBPolygon

// From WKBMultiPoint
#define WKB_FIRST_POINT(MP)     (WKBPoint*)((acp_uint8_t*)(MP)+WKB_MPOINT_SIZE);

// From WKBMultiLineString
#define WKB_FIRST_LINE(ML)      (WKBLineString*)((acp_uint8_t*)(ML)+WKB_MLINE_SIZE);

// From stdMultiPolygonType
#define WKB_FIRST_POLY(MA)      (WKBPolygon*)((acp_uint8_t*)(MA)+WKB_MPOLY_SIZE);

// From stdGeoCollectionType
#define WKB_FIRST_COLL(CO)      (WKBGeometry*)((acp_uint8_t*)(CO)+WKB_COLL_SIZE);

#define WKB_NEXT_GEOM(x)

// From WKBMultiPoint
#define EWKB_FIRST_POINT(MP)    (WKBPoint*)((acp_uint8_t*)(MP)+EWKB_MPOINT_SIZE);

// From WKBMultiLineString
#define EWKB_FIRST_LINE(ML)     (WKBLineString*)((acp_uint8_t*)(ML)+EWKB_MLINE_SIZE);

// From stdMultiPolygonType
#define EWKB_FIRST_POLY(MA)     (WKBPolygon*)((acp_uint8_t*)(MA)+EWKB_MPOLY_SIZE);

// From stdGeoCollectionType
#define EWKB_FIRST_COLL(CO)     (WKBGeometry*)((acp_uint8_t*)(CO)+EWKB_COLL_SIZE);

#define EWKB_NEXT_GEOM(x)

// Fix BUG-15999
//==============================================================================

enum GeoGroupTypes
{
    STD_NULL_GROUP = 0,
    STD_POINT_2D_GROUP = 21,
    STD_LINE_2D_GROUP = 23,
    STD_POLYGON_2D_GROUP = 25
};

enum GeoStatusTypes
{
    STD_STAT_INT = 0,
    STD_STAT_DIS,
    STD_STAT_TCH,
    STD_STAT_OVR
};

//=======================================================
// BUGBUG - 상위 프로젝트에 의해 제거되어야 함.
//=======================================================

// PROJ-1583 BLOB 과의 통합
// PROJ-1587 MT의 가변길이 처리 기능
// To Fix BUG-15365
// MAX 길이의 경우 Type의 제한이며,
// Page 제한에 의해 최대 크기가 결정될 수 있다

#define STD_GEOMETRY_PRECISION_DEFAULT (32000)


//=======================================================
// BUG-28821
//=======================================================
// STD_GEOMETRY_PRECISION_MINIMUM은 하위 객체가 없는 멀티 객체를 최소 크키로 한다
// 이로 인해  ( acp_uint32_t(mNumObjects)+ SChar(padding[4]))의 크기가 되어 8이 된다.

#define STD_GEOMETRY_PRECISION_MINIMUM (8)                     // BUG-28821
#define STD_GEOMETRY_PRECISION_MAXIMUM (104857600)             // 100M

#define STD_GEOMETRY_STORE_PRECISION_MAXIMUM (104857600)       // BUG-28921

#endif /* _O_STD_TYPES_H_  */


