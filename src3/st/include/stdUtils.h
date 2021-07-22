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
 * $Id: stdUtils.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 연산에 필요한 기본 연산자들
 **********************************************************************/

#ifndef _O_STD_UTILS_H_
#define _O_STD_UTILS_H_        1

#include <idTypes.h>
#include <iduMemory.h>
#include <mtcDef.h>

#include <stdTypes.h>
#include <stdCommon.h>

#define ST_MAX_POINT_SORT_BUFFER_COUNT   (1000)

#define ST_CLOCKWISE        ((SInt)  (-1))
#define ST_COUNTERCLOCKWISE ((SInt)   (1))
#define ST_PARALLEL         ((SInt)   (0))     

#define ST_INTERSECT        ((SInt)   (1))
#define ST_NOT_INTERSECT    ((SInt)   (2))
#define ST_TOUCH            ((SInt)   (3))
#define ST_SHARE            ((SInt)   (4))
#define ST_POINT_TOUCH      ((SInt)   (5))

#define ST_INCREASE         ((SInt)   (0))
#define ST_DECREASE         ((SInt)   (1))
#define ST_NOT_SET          ((SInt) (100))

#define ST_NOT_REVERSE      ((SInt)   (0))
#define ST_REVERSE          ((SInt)   (1))

/* polygon function number */
#define ST_CLIP_INTERSECT     ((SInt) (0))
#define ST_CLIP_UNION         ((SInt) (1))
#define ST_CLIP_DIFFERENCE    ((SInt) (2))
#define ST_CLIP_SYMDIFFERENCE ((SInt) (3))

/* struct Segment mLabel */
#define ST_SEG_LABEL_OUTSIDE  ((SInt) (0))
#define ST_SEG_LABEL_INSIDE   ((SInt) (1))
#define ST_SEG_LABEL_SHARED1  ((SInt) (2))
#define ST_SEG_LABEL_SHARED2  ((SInt) (3))

/* Segment Direction */
#define ST_SEG_DIR_FORWARD    ((SInt)  (0))
#define ST_SEG_DIR_BACKWARD   ((SInt)  (1))

/* Segment Usabillity */
#define ST_SEG_NOT_USE        ((SInt) (-1))
#define ST_SEG_USABLE         ((SInt)  (0))
#define ST_SEG_USED           ((SInt)  (1))

#define ST_CHAIN_NOT_OPPOSITE ((SInt)  (0))
#define ST_CHAIN_OPPOSITE     ((SInt)  (1))

/* BUFFER Multipiler */
#define ST_BUFFER_MULTIPLIER  ((SDouble) (10.0))

/* POINT - AREA */
#define ST_POINT_INSIDE  ((SInt) (-1))
#define ST_POINT_ONBOUND ((SInt) ( 0))
#define ST_POINT_OUTSIDE ((SInt) ( 1))

typedef struct Chain
{
    SShort        mReverse;
    SShort        mVaritation;
    UInt          mRingNum;
    UInt          mPolygonNum;
    SInt          mOrientaion;    
    SInt          mOpposite;
    struct Segment      *mBegin;
    struct Segment      *mEnd;
    struct Chain *mPrev;    
    struct Chain *mNext;    
}Chain;

typedef struct Vertex
{
    stdPoint2D          mCoord;
    struct VertexEntry *mList;
    struct Vertex      *mNext;    
}Vertex;

typedef struct VertexEntry{
    SDouble             mAngle;
    struct Segment     *mSeg;
    struct VertexEntry *mNext;
    struct VertexEntry *mPrev;
}VertexEntry;

typedef struct Segment
{
    stdPoint2D      mStart;
    stdPoint2D      mEnd;
    SChar           mUsed;
    SChar           mLabel;
    SChar           mPadding[6];    
    Chain*          mParent;
    struct Segment *mPrev;    
    struct Segment *mNext;
    struct Vertex  *mBeginVertex;
    struct Vertex  *mEndVertex;
}Segment;

typedef struct PrimInterSeg
{
    struct Segment*       mFirst;    
    struct Segment*       mSecond;
    SInt                  mStatus;
    SInt                  mInterCount;    
    stdPoint2D            mInterPt[2];
    struct PrimInterSeg *mNext;
}PrimInterSeg;

typedef struct stdRepPoint 
{
    stdPoint2D* mPoint;
    idBool      mIsValid;
} stdRepPoint;

class stdUtils
{
public:
    static void setType( stdGeometryHeader * aGeom,
                         UShort              aType );
    static UShort getType( stdGeometryHeader * aGeom );
    static IDE_RC isEquiEndian( stdGeometryHeader * aGeom,
                                idBool            * aIsEquiEndian );
    // BUG-24357 WKB Endian.
    static IDE_RC compareEndian( UChar aGeomEndian,
                                 idBool *aIsEquiEndian);
    static void getTypeText(
                    UShort aType,
                    SChar* aTypeText,
                    UShort* aLen );

    static idBool isMBRIntersects( stdMBR* sMBR1, stdMBR* sMBR2 );
    static idBool isMBRContains( stdMBR* sMBR1, stdMBR* sMBR2 );
    static idBool isMBRWithin( stdMBR* sMBR1, stdMBR* sMBR2 );
    static idBool isMBREquals( stdMBR* sMBR1, stdMBR* sMBR2 );

    // BUG-22338
    static void   get3PtMBR(stdPoint2D *aPt1,
                            stdPoint2D *aPt2,
                            stdPoint2D *aPt3,                         
                            stdMBR *aMBR);
    
    static IDE_RC mergeMBRFromHeader(stdGeometryHeader*  aDst,
                                     stdGeometryHeader*  aSrc);
    static IDE_RC mergeMBRFromPoint2D(stdGeometryHeader*  aGeom,
                                      const stdPoint2D*   aPoint);
    static void copyMBR(stdMBR* aDst, stdMBR* aSrc);
    static void makeCurrentDate(mtdDateType* aDate);

    static idBool isValidType(UShort aType, idBool bIncludeNull);
    static idBool is2DType(UShort aType);
    static idBool isExtendedType( UShort aType );

    static SInt getDimension( stdGeometryHeader*  aObj );

    static void nullify(stdGeometryHeader* aGeom);
    static void makeEmpty(stdGeometryHeader* aGeom);
    static idBool isNull(const stdGeometryHeader* aGeom);
    static idBool isEmpty(const stdGeometryHeader* aGeom);
    static void makeGeoHeader(stdGeometryHeader*    aDst,
                              stdGeometryHeader*    aSrc );

    static UInt getGeometryNum( stdGeometryHeader* aGeom );
    static stdGeometryHeader* getFirstGeometry( stdGeometryHeader* aGeom );
    static stdGeometryHeader* getNextGeometry( stdGeometryHeader* aGeom );

    static stdPoint2D* findNextPointInRing2D(
                            stdPoint2D* aPt,
                            UInt        aCurr,
                            UInt        aSize,
                            UInt      * aNext );

    static stdPoint2D* findPrevPointInRing2D(
                            stdPoint2D* aPt,
                            UInt        aCurr,
                            UInt        aSize,
                            UInt      * aNext );

    static stdPoint2D* findNextPointInLine2D(
                            stdPoint2D * aPt,
                            stdPoint2D * aFence );
    static stdPoint2D* findPrevPointInLine2D(
                            stdPoint2D * aPt,
                            stdPoint2D * aFence );

    static SDouble getAngle2D(
                        stdPoint2D*     aPt1,
                        stdPoint2D*     aPt2,
                        stdPoint2D*     aPt3 );

    static idBool isSamePoints2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2 );

    static idBool isEndPoint2D(
                    const stdPoint2D*               aPoint,
                    const stdLineString2DType*      aLine );


    static idBool isClosed2D( stdGeometryHeader*    aObj );

    static idBool isClosedPoint2D(
                    const stdPoint2D*               aPoint,
                    stdGeometryHeader*              aObj );

    static SDouble dot2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2 );

    static SDouble length2D( const stdPoint2D* aP );

    static void subVec2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    stdPoint2D*           aP3 );

    static SDouble area2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool left2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool leftOn2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool right2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool rightOn2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool collinear2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool quadLinear2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3,
                    const stdPoint2D*     aP4 );

    static idBool betweenI2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool between2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static idBool betweenOnLeft2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );


    static idBool betweenOnRight2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );

    static SInt sector2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3,
                    SDouble*              aPos );
    static idBool isReturned2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3 );
    // To Fix BUG-21196
    static idBool intersectLineMBR2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3,
                    const stdPoint2D*     aP4 );
    
    static idBool intersectI2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3,
                    const stdPoint2D*     aP4 );

    static idBool intersect2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    const stdPoint2D*     aP3,
                    const stdPoint2D*     aP4 );

    static GeoStatusTypes getStatus3Points3Points2D(
                    stdPoint2D*     aPtA1,
                    stdPoint2D*     aPtA2,
                    stdPoint2D*     aPtA3,
                    stdPoint2D*     aPtB1,
                    stdPoint2D*     aPtB2,
                    stdPoint2D*     aPtB3 );

    static idBool intersectLineToLine2D(
                    const stdLineString2DType*     aLine1,
                    const stdLineString2DType*     aLine2 );


    static idBool isRingContainsRing2D(
                        stdLinearRing2D*    aRing1,
                        stdMBR*             aMBR1,
                        stdLinearRing2D*    aRing2,
                        stdMBR*             aMBR2 );
    static idBool isRingContainsRing2D(
                        stdLinearRing2D*    aRing1,
                        stdLinearRing2D*    aRing2 );

    static idBool isRingNotDisjoint2D(
                        stdLinearRing2D*    aRing1,
                        stdMBR*             aMBR1,
                        stdLinearRing2D*    aRing2,
                        stdMBR*             aMBR2 );

    static idBool insideRing2D(
                    const stdPoint2D*		aPt,
                    const stdLinearRing2D*	aRing );

    static idBool insideRingI2D(
                    const stdPoint2D*		aPt,
                    const stdLinearRing2D*	aRing );

    static idBool isCCW2D( stdLinearRing2D*   aRing );

    static idBool getIntersection2D(
                    const stdPoint2D    *aPt1,
                    const stdPoint2D    *aPt2,
                    const stdPoint2D    *aPt3,
                    const stdPoint2D    *aPt4,
                    stdPoint2D          *aResult );

    static idBool getIntersection2D(
                    const stdPoint2D        *aPt1,
                    const stdPoint2D        *aPt2,
                    const stdPolygon2DType  *aPoly,
                    stdPoint2D              *aResult );

    static idBool getIntersects2D(
                    const stdPoint2D*       aSp0,
                    const stdPoint2D*       aEp0,
                    const stdPoint2D*       aSp1,
                    const stdPoint2D*       aEp1,
                    stdPoint2D*             aPointSet,
                    SInt*                   aNumPoints );

    static SDouble areaRing2D( stdLinearRing2D* aRing );

    static IDE_RC centroidRing2D(
                    stdLinearRing2D*        aRing,
                    stdPoint2D*             aResult );
    static IDE_RC centroidMPolygon2D(
                    stdMultiPolygon2DType*  aMPoly,
                    stdPoint2D*             aResult );

    static IDE_RC getPointOnSurface2D( iduMemory*              aQmxMem,
                                       const stdPolygon2DType* aPoly,
                                       stdPoint2D*             aResult );

////////////////////////////////////////////////////////////////////////////////
    static void shiftMultiObjToSingleObj(stdGeometryHeader* aMultiObj);

    static void copyGeometry(
                    stdGeometryHeader* aGeomDst,
                    stdGeometryHeader* aGeom );
    static GeoGroupTypes getGroup( stdGeometryHeader* aGeom );
    static void initMBR( stdMBR* aMBR );
    static IDE_RC setMBRFromPoint2D(stdMBR* aMBR, stdPoint2D* aPoint);
    static IDE_RC setMBRFromRing2D(stdMBR* aMBR, stdLinearRing2D*  aRing);

    static SDouble getDistanceSqLineToPoint2D( const stdPoint2D *aPt1,
                                               const stdPoint2D *aPt2,
                                               const stdPoint2D *aPt,
                                               stdPoint2D *aCrPt  );
    static SDouble getDistanceSqLineSegmentToPoint2D( const stdPoint2D *aPt1,
                                                      const stdPoint2D *aPt2,
                                                      const stdPoint2D *aPt );
    static idBool checkBetween2D( const stdPoint2D *aPt1,
                                  const stdPoint2D *aPt2,
                                  const stdPoint2D *aPt );

    static idBool getIntersectLineToLine2D( const stdPoint2D *aPt1,
                                            const stdPoint2D *aPt2,
                                            const stdPoint2D *aPt3,
                                            const stdPoint2D *aPt4,
                                            stdPoint2D *aPt,
                                            idBool *aCode );

    static idBool isIntersectsLineToLine2D( const stdPoint2D *aPt1,
                                            const stdPoint2D *aPt2,
                                            idBool aIsEnd1,
                                            idBool aIsEnd2,
                                            const stdPoint2D *aPt3,
                                            const stdPoint2D *aPt4,
                                            idBool aIsEnd3,
                                            idBool aIsEnd4  );
    // BUG-22338
    static idBool isMBRContainsPt(stdMBR *aMBR,
                                   stdPoint2D *aPt);

    // BUG-27518
    static void   isIndexableObject(stdGeometryHeader *aValue,
                                    idBool            *aIsIndexable);

    // BUG-28934
    static void   mergeOrMBR( stdMBR *aRet, stdMBR *aMBR1, stdMBR *aMBR2);

    // BUG-28934
    static void   mergeAndMBR( stdMBR *aRet, stdMBR *aMBR1, stdMBR *aMBR2);
    
    // BUG-28934
    static idBool isNullMBR( stdMBR *aMBR );

    // PROJ-1591: Disk Spatial Index
    static stdMBR* getMBRExtent( stdMBR *aDstMbr, stdMBR *aSrcMbr );
    static SDouble getMBRArea( stdMBR *aMbr );
    static SDouble getMBRDelta( stdMBR *aMbr1, stdMBR *aMbr2 );
    static SDouble getMBROverlap( stdMBR *aMbr1, stdMBR *aMbr2 );

    // PROJ-2166 Spatial Validation
    static SInt CCW( const stdPoint2D aPt1,
                     const stdPoint2D aPt2,
                     const stdPoint2D aPt3 );

    // PROJ-2166 Spatial Validation
    static IDE_RC intersectCCW( const stdPoint2D   aPt1,
                                const stdPoint2D   aPt2,
                                const stdPoint2D   aPt3,
                                const stdPoint2D   aPt4,
                                SInt             * aStatus,
                                SInt             * aNumPoints,
                                stdPoint2D       * aResultPoint);
    
    static SInt getOrientationFromRing( stdLinearRing2D * aRing);

    
    static idBool getIntersectPoint(const stdPoint2D aPt1,
                                    const stdPoint2D aPt2, 
                                    const stdPoint2D aPt3,
                                    const stdPoint2D aPt4,
                                    SInt*            aStatus,
                                    stdPoint2D*      aResult);


   static void getSegProperty(const stdPoint2D *aPt1,
                              const stdPoint2D *aPt2,
                              SInt             *aVaritation,
                              SInt             *aReverse );

    static void initChain(Segment *aSeg,
                          Chain   *aChain,
                          SInt     aVaritaion,
                          SInt     aReverse,
                          UInt     aRingNum,
                          UInt     aPolyNum,
                          SInt     aOrientation,
                          SInt     aOpposite);
    
    static void appendLastSeg( Segment *aSeg,
                               Chain   *aChain);
    

    static void appendFirstSeg( Segment *aSeg,
                                Chain   *aChain);


    static idBool pointInPolygon( Segment*    aIndexSeg,
                                  stdPoint2D* aPoint,
                                  ULong       aPolySide);
    
    static SInt getOrientaion( stdLinearRing2D* aRing );

    static IDE_RC classfyPolygonChain( iduMemory*        aQmxMem,
                                       stdPolygon2DType* aPolygon,
                                       UInt              aPolyNum,
                                       Segment**         aIndexSeg,
                                       UInt*             aIndexSegTotal,
                                       Segment**         aRingSegList,
                                       UInt*             aRingCount,
                                       idBool            aValidation );
    
    static Segment* getPrevSeg( Segment *aSeg );
    
    static Segment* getNextSeg( Segment *aSeg );


    static IDE_RC addPrimInterSeg( iduMemory*     aQmxMem,
                                   PrimInterSeg** aHead,
                                   Segment*       aFirst,
                                   Segment*       aSecond,
                                   SInt           aStatus,
                                   SInt           aInterCount,
                                   stdPoint2D*    aInterPt );

    static IDE_RC reassign( iduMemory*    aQmxMem,
                            PrimInterSeg *aPrimHead, 
                            idBool        aUseTolerance );
    
    static IDE_RC reassignSeg( iduMemory*  aQmxMem,
                               Segment*    aSeg1,
                               Segment*    aSeg2,
                               SInt        aStatus,
                               SInt        aInterCnt,
                               stdPoint2D* aInterPt,
                               idBool      aUseTolerance );

    static IDE_RC addSegmentToVertex( iduMemory* aQmxMem,
                                      Vertex*    aVertex,
                                      Segment*   aSegment );

    static IDE_RC addVertex( iduMemory*       aQmxMem,
                             Segment*         aSeg,
                             Vertex**         aVertex,
                             Vertex**         aResult,
                             const stdPoint2D aPt);    

    static idBool isRingInSide( Segment**  aIndexSeg,
                                UInt       aIndexSegTotal,
                                Segment*   aCmpSeg,
                                UInt       aPolyCmpMin,
                                UInt       aPolyCmpMax,
                                UChar*     aCount );

    static SInt isPointInside( Segment**   aIndexSeg,
                               UInt        aIndexSegTotal,
                               stdPoint2D* aPt,
                               UInt        aPolyCmpMin,
                               UInt        aPolyCmpMax );


    //BUG-33436
    static void   adjustVertex( Vertex*  aVertex, 
                                Segment* aOldSeg, 
                                Segment* aNewSeg );

    //BUG-33904
    static IDE_RC setVertex( iduMemory*       aQmxMem,
                             Segment*         aSeg,
                             idBool           aIsStart,
                             Vertex**         aVertexRoot,
                             const stdPoint2D aPt );

    static IDE_RC intersectCCW4Func( const stdPoint2D   aPt1,
                                     const stdPoint2D   aPt2,
                                     const stdPoint2D   aPt3,
                                     const stdPoint2D   aPt4,
                                     SInt             * aStatus,
                                     SInt             * aNumPoints,
                                     stdPoint2D       * aResultPoint);

    static idBool getIntersectPoint4Func( const stdPoint2D aPt1,
                                          const stdPoint2D aPt2, 
                                          const stdPoint2D aPt3,
                                          const stdPoint2D aPt4,
                                          SInt*            aStatus,
                                          stdPoint2D*      aResult);

    static idBool isSamePoints2D4Func( const stdPoint2D* aPt1,
                                       const stdPoint2D* aPt2 );

    static idBool checkBetween2D4Func( const stdPoint2D* aPt1,
                                       const stdPoint2D* aPt2,
                                       const stdPoint2D* aPt );

    // BUG-33576
    static IDE_RC checkValid( const stdGeometryHeader* aHeader );
};

// BUG-33436
#define ST_LOGICAL_XOR(aBool1,aBool2)                                   \
    (idBool)                                                            \
    (                                                                   \
        (                                                               \
            ( ((aBool1) == ID_TRUE)  && ((aBool2) == ID_FALSE) ) ||     \
            ( ((aBool1) == ID_FALSE) && ((aBool2) == ID_TRUE)  )        \
        )                                                               \
        ? ID_TRUE : ID_FALSE                                            \
    )

#define ST_LOGICAL_AND(aBool1,aBool2)                                   \
    (idBool)                                                            \
    (                                                                   \
        (                                                               \
            ( ((aBool1) == ID_TRUE)  && ((aBool2) == ID_TRUE) )         \
        )                                                               \
        ? ID_TRUE : ID_FALSE                                            \
    )
                

// BUG-22562
#define ST_XOR(a, b) (idBool) ( (! (a) ) ^ ( ! (b) ) )

// BUG-28693

typedef SInt (*cmpPointFunc) ( const void *, const void *);

extern "C" SInt cmpDouble(const void * aV1, const void * aV2);

extern "C" SInt cmpPointOrder(const void * aV1, const void *aV2, idBool aXOrder);

extern "C" SInt cmpSegment(const void * aV1, const void * aV2);

#endif /* _O_STD_UTILS_H_ */

