/***********************************************************************
 * Copyright 1999-2006, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

#ifndef _O_STD_WKB_TYPES_H_
#define _O_STD_WKB_TYPES_H_    (1)

/*----------------------------------------------------------------*/
/*  WKB(Well Known Binary) Data Types                             */
/*                                                                */
/*  !!! WARNING !!!                                               */
/*                                                                */
/*    BE CAREFUL to modify this file                              */
/*                                                                */
/*    Altibase server and user applications                       */
/*      both use these data types.                                */
/*                                                                */
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/* Primitive Types                                                */
/*----------------------------------------------------------------*/

/* WKB Point */
typedef struct wkbPoint
{
    UChar           mX[8];                /* x coord */
    UChar           mY[8];                /* y coord */
} wkbPoint;

/* WKB Linear Ring */
typedef struct wkbLinearRing
{
    UChar             mNumPoints[4];           /* the number of points */
/*  wkbPoint          mPoints[mNumPoints];  // array of points */
} wkbLinearRing;

/* WKB Byte Order */
typedef enum wkbByteOrder
{
    wkbXDR           = 0,                /* Big Endian */
    wkbNDR           = 1                 /* Little Endian */
} wkbByteOrder;

/*----------------------------------------------------------------*/
/* WKB Geometry Types                                             */
/*----------------------------------------------------------------*/

/* WKB Geometry Type ID  */
typedef enum wkbGeometryType
{
    WKB_POINT_TYPE           = 1,
    WKB_LINESTRING_TYPE      = 2,
    WKB_POLYGON_TYPE         = 3,
    WKB_MULTIPOINT_TYPE      = 4,
    WKB_MULTILINESTRING_TYPE = 5,
    WKB_MULTIPOLYGON_TYPE    = 6,
    WKB_COLLECTION_TYPE      = 7,
    
    EWKB_POINT_TYPE           = 101,
    EWKB_LINESTRING_TYPE      = 102,
    EWKB_POLYGON_TYPE         = 103,
    EWKB_MULTIPOINT_TYPE      = 104,
    EWKB_MULTILINESTRING_TYPE = 105,
    EWKB_MULTIPOLYGON_TYPE    = 106,
    EWKB_COLLECTION_TYPE      = 107
} wkbGeometryType;

/* WKB Geometry Header  */
typedef struct WKBHeader
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* well-known binary type */
} WKBHeader;

/* WKB Point Object  */
typedef struct WKBPoint
{
    UChar             mByteOrder;
    UChar             mWkbType[4];       /* WKB_POINT_TYPE (1) */
    wkbPoint          mPoint;            /* point */
} WKBPoint;

/* WKB LineString Object  */
typedef struct WKBLineString
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_LINESTRING_TYPE (2) */
    UChar             mNumPoints[4];     /* the number of points */
/*  wkbPoint          mPoints[mNumPoints];  // array of points */
} WKBLineString;

/* WKB Polygon Object  */
typedef struct WKBPolygon
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_POLYGON_TYPE (3) */
    UChar             mNumRings[4];      /* the number of rings */
/*  wkbLinearRing     mRings[mNumRings]; // list of rings */
} WKBPolygon;

/* WKB Multi-Point Object  */
typedef struct WKBMultiPoint
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_MULTIPOINT_TYPE (4) */
    UChar             mNumWKBPoints[4];  /* the number of WKB points */
/*  WKBPoint          mWKBPoints[mNumWKBPoints];    // array of WKB points */
} WKBMultiPoint;

/* WKB Multi-LineString Object  */
typedef struct WKBMultiLineString
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_MULTILINESTRING_TYPE (5) */
    UChar             mNumWKBLineStrings[4];/* the number of WKB line strings */
/*  WKBLineString     mWKBLineStrings[mNumWKBLineStrings];// list of WKB line strings */
} WKBMultiLineString;

/* WKB Multi-Polygon Object  */
typedef struct WKBMultiPolygon
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_MULTIPOLYGON_TYPE (6) */
    UChar             mNumWKBPolygons[4];/* the number of WKB polygons */
/*  WKBPolygon        mWKBPolygons[mNumWKBPolygons];    // list of WKB polygons */
} WKBMultiPolygon;

struct WKBGeometry;

/* WKB Collection Object */
typedef struct WKBGeometryCollection
{
    UChar            mByteOrder;         /* byte order */
    UChar            mWkbType[4];        /* WKB_COLLECTION_TYPE (7) */
    UChar            mNumWKBGeometries[4];/* the number of WKB geometries */
/*  WKBGeometry      mWKBGeometries[mNumWKBGeometries];// list of WKB geometries */
} WKBGeometryCollection;

/* Extended WKB Point Object  */
typedef struct EWKBPoint
{
    UChar             mByteOrder;
    UChar             mWkbType[4];       /* WKB_POINT_TYPE (1) */
    UChar             mSRID[4];          /* spatial reference system id */
    wkbPoint          mPoint;            /* point */
} EWKBPoint;

/* Extended WKB LineString Object  */
typedef struct EWKBLineString
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_LINESTRING_TYPE (2) */
    UChar             mSRID[4];          /* spatial reference system id */
    UChar             mNumPoints[4];     /* the number of points */
/*  wkbPoint          mPoints[mNumPoints];  // array of points */
} EWKBLineString;

/* Extended WKB Polygon Object  */
typedef struct EWKBPolygon
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_POLYGON_TYPE (3) */
    UChar             mSRID[4];          /* spatial reference system id */
    UChar             mNumRings[4];      /* the number of rings */
/*  wkbLinearRing     mRings[mNumRings]; // list of rings */
} EWKBPolygon;

/* Extended WKB Multi-Point Object  */
typedef struct EWKBMultiPoint
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_MULTIPOINT_TYPE (4) */
    UChar             mSRID[4];          /* spatial reference system id */
    UChar             mNumWKBPoints[4];  /* the number of WKB points */
/*  WKBPoint          mWKBPoints[mNumWKBPoints];    // array of WKB points */
} EWKBMultiPoint;

/* Extended WKB Multi-LineString Object  */
typedef struct EWKBMultiLineString
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_MULTILINESTRING_TYPE (5) */
    UChar             mSRID[4];          /* spatial reference system id */
    UChar             mNumWKBLineStrings[4];/* the number of WKB line strings */
/*  WKBLineString     mWKBLineStrings[mNumWKBLineStrings];// list of WKB line strings */
} EWKBMultiLineString;

/* Extended WKB Multi-Polygon Object  */
typedef struct EWKBMultiPolygon
{
    UChar             mByteOrder;        /* byte order */
    UChar             mWkbType[4];       /* WKB_MULTIPOLYGON_TYPE (6) */
    UChar             mSRID[4];          /* spatial reference system id */
    UChar             mNumWKBPolygons[4];/* the number of WKB polygons */
/*  WKBPolygon        mWKBPolygons[mNumWKBPolygons];    // list of WKB polygons */
} EWKBMultiPolygon;

/* Extended WKB Collection Object */
typedef struct EWKBGeometryCollection
{
    UChar            mByteOrder;         /* byte order */
    UChar            mWkbType[4];        /* WKB_COLLECTION_TYPE (7) */
    UChar            mSRID[4];           /* spatial reference system id */
    UChar            mNumWKBGeometries[4];/* the number of WKB geometries */
/*  WKBGeometry      mWKBGeometries[mNumWKBGeometries];// list of WKB geometries */
} EWKBGeometryCollection;

/* WKB Geometry Object */
typedef struct WKBGeometry
{
    union {
        WKBHeader                header;
        WKBPoint                 point;
        WKBLineString            linestring;
        WKBPolygon               polygon;
        WKBMultiPoint            mpoint;
        WKBMultiLineString       mlinestring;
        WKBMultiPolygon          mpolygon;
        EWKBPoint                pointExt;
        EWKBLineString           linestringExt;
        EWKBPolygon              polygonExt;
        EWKBMultiPoint           mpointExt;
        EWKBMultiLineString      mlinestringExt;
        EWKBMultiPolygon         mpolygonExt;
    };
} WKBGeometry;

#endif /* _O_STD_WKB_TYPES_H_  */


