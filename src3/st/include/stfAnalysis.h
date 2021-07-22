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
 * $Id: stfAnalysis.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체 분석 함수
 **********************************************************************/

#ifndef _O_STF_ANALYSIS_H_
#define _O_STF_ANALYSIS_H_        1

#include <idTypes.h>

#include <mtcDef.h>

#include <stdTypes.h>
#include <stdGpc.h>
#include <stdUtils.h>

typedef struct _lineLink2D
{
    stdPoint2D              Point1;
    stdPoint2D              Point2;
    struct _lineLink2D*     pNext;
} lineLink2D;

typedef struct _lineMainLink2D
{
    lineLink2D*             pItemFirst;
    lineLink2D*             pItemLast;
    struct _lineMainLink2D* pNext;
} lineMainLink2D;

typedef struct _pointLink2D
{
    stdPoint2D              Point;
    struct _pointLink2D*    pNext;
} pointLink2D;

typedef struct _objLink2D 
{
    stdPolygon2DType*       pObj;
    struct _objLink2D*      pNext;
} objLink2D;

typedef struct _objMainLink2D
{
    objLink2D*                pItem;
    struct _objMainLink2D*    pNext;
} objMainLink2D;

typedef struct _ringLink2D 
{
    stdLinearRing2D*        pRing;
    struct _ringLink2D*     pNext;
} ringLink2D;

typedef struct _ringMainLink2D
{
    ringLink2D*             pInterior;
    stdPolygon2DType*       pPoly;
    struct _ringMainLink2D* pNext;
} ringMainLink2D;

typedef struct
{
    SDouble fArea;      // area
    stdMBR  stMBR;      // MBR
    SInt    nPnum;      // Polygon Number
    SInt    nRnum;      // Ring Number
    SInt    nStIdx;     // Structure Idx
} areaIdx;

class stfAnalysis
{
public:

    static IDE_RC distance(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );
    
    static IDE_RC buffer(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC intersection(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC difference(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC unions(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );
          
    static IDE_RC symDifference(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC convexHull(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

//////////////////////////////////////////////////////////////////////////////
// Distance
//////////////////////////////////////////////////////////////////////////////

    static IDE_RC  getDistanceTwoObject(
                    stdGeometryType*    aObj1,
                    stdGeometryType*    aObj2,
                    SDouble*            aResult );
    
    static SDouble primDistancePToP2D(
                                const stdPoint2D* aP1,
                                const stdPoint2D* aP2 );

    static SDouble primDistancePToL2D(
                                const stdPoint2D* aP,
                                const stdPoint2D* aQ1,
                                const stdPoint2D* aQ2 );
                                 
    static SDouble primDistanceLToL2D(
                                const stdPoint2D* aP1,
                                const stdPoint2D* aP2,
                                const stdPoint2D* aQ1,
                                const stdPoint2D* aQ2 );

    static SDouble primDistancePToR2D(
                                const stdPoint2D*       aPt,
                                const stdLinearRing2D*  aRing );

    static SDouble primDistanceLToR2D(
                                const stdPoint2D*       aP1,
                                const stdPoint2D*       aP2,
                                const stdLinearRing2D*  aRing );

    static SDouble primDistanceRToR2D(
                                const stdLinearRing2D*  aRing1,
                                const stdLinearRing2D*  aRing2 );

                                 
// Point With Other ------------------------------------------------------------
    static SDouble getDistanceSpToSp2D(
                                const stdPoint2DType*    aPt,
                                const stdPoint2DType*    aDstPt );

    static SDouble getDistanceSpToSl2D(
                                const stdPoint2DType*       aPt,
                                const stdLineString2DType*  aDstLine );

    static SDouble getDistanceSpToSa2D(
                                const stdPoint2DType*       aPt,
                                const stdPolygon2DType*     aDstPoly );
    static SDouble getDistanceSpToMp2D(
                                const stdPoint2DType*        aPt,
                                const stdMultiPoint2DType*   aDstMPt );
    static SDouble getDistanceSpToMl2D(
                                const stdPoint2DType*           aPt,
                                const stdMultiLineString2DType* aDstMLine );
    static SDouble getDistanceSpToMa2D(
                                const stdPoint2DType*           aPt,
                                const stdMultiPolygon2DType*    aDstMPoly );

// Line With Other -------------------------------------------------------------
    static SDouble getDistanceSlToSl2D(
                                const stdLineString2DType*      aLine,
                                const stdLineString2DType*      aDstLine );
    static SDouble getDistanceSlToSa2D(
                                const stdLineString2DType*      aLine,
                                const stdPolygon2DType*         aDstPoly );
    static SDouble getDistanceSlToMp2D(
                                const stdLineString2DType*      aLine,
                                const stdMultiPoint2DType*      aDstMPt );
    static SDouble getDistanceSlToMl2D(
                                const stdLineString2DType*      aLine,
                                const stdMultiLineString2DType* aDstMLine );
    static SDouble getDistanceSlToMa2D(
                                const stdLineString2DType*      aLine,
                                const stdMultiPolygon2DType*    aDstMPoly );

// Polygon With Other ----------------------------------------------------------
    static SDouble getDistanceSaToSa2D(
                                const stdPolygon2DType*     aPoly,
                                const stdPolygon2DType*     aDstPoly );
    static SDouble getDistanceSaToMp2D(
                                const stdPolygon2DType*     aPoly,
                                const stdMultiPoint2DType*  aDstMPt );
    static SDouble getDistanceSaToMl2D(
                                const stdPolygon2DType*         aPoly,
                                const stdMultiLineString2DType* aDstMLine );
    static SDouble getDistanceSaToMa2D(
                                const stdPolygon2DType*         aPoly,
                                const stdMultiPolygon2DType*    aDstMPoly );

// Multi Point With Other ------------------------------------------------------
    static SDouble getDistanceMpToMp2D(
                                const stdMultiPoint2DType*      aMPt,
                                const stdMultiPoint2DType*      aDstMPt );

    static SDouble getDistanceMpToMl2D(
                                const stdMultiPoint2DType*      aMPt,
                                const stdMultiLineString2DType* aDstMLine );
    static SDouble getDistanceMpToMa2D(
                                const stdMultiPoint2DType*      aMPt,
                                const stdMultiPolygon2DType*    aDstMPoly );

// Multi Line With Other -------------------------------------------------------
    static SDouble getDistanceMlToMl2D(
                                const stdMultiLineString2DType* aMLine,
                                const stdMultiLineString2DType* aDstMLine );

    static SDouble getDistanceMlToMa2D(
                                const stdMultiLineString2DType* aMLine,
                                const stdMultiPolygon2DType*    aDstMPoly );

// Multi Polygon With Other --------------------------------------------------
    
    static SDouble getDistanceMaToMa2D(
                                const stdMultiPolygon2DType*    aMPoly,
                                const stdMultiPolygon2DType*    aDstMPoly );
//////////////////////////////////////////////////////////////////////////////
// Buffer
//////////////////////////////////////////////////////////////////////////////
    static IDE_RC getBuffer(
                    iduMemory*                      aQmxMem,
                    stdGeometryType*                aObj,
                    SDouble                         aDist,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110
    
    static IDE_RC getPointBufferObject2D(
                    const stdPoint2D*               aPt,
                    const SDouble                   aDistance,
                    const SInt                      aPrecision,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110

    static IDE_RC getLinestringBufferObject2D(
                    iduMemory*                      aQmxMem,
                    const stdLineString2DType*      aLine,
                    const SDouble                   aDistance,
                    const SInt                      aPrecision,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110

    static IDE_RC getPolygonBufferObject2D(
                    iduMemory*                      aQmxMem,
                    const stdPolygon2DType*         aPoly,
                    const SDouble                   aDistance,
                    const SInt                      aPrecision,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110
    
    static IDE_RC getMultiPointBufferObject2D(
                    iduMemory*                      aQmxMem,
                    const stdMultiPoint2DType*      aMPoint,
                    const SDouble                   aDistance,
                    const SInt                      aPrecision,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110
                    
    static IDE_RC getMultiLinestringBufferObject2D(
                    iduMemory*                      aQmxMem,
                    const stdMultiLineString2DType* aMLine,
                    const SDouble                   aDistance,
                    const SInt                      aPrecision,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110
                    
    static IDE_RC getMultiPolygonBufferObject2D(
                    iduMemory*                      aQmxMem,
                    const stdMultiPolygon2DType*    aMPoly,
                    const SDouble                   aDistance,
                    const SInt                      aPrecision,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence );  //Fix BUG - 25110
    
//////////////////////////////////////////////////////////////////////////////
// Intersection
//////////////////////////////////////////////////////////////////////////////
    static IDE_RC getIntersection(
                    iduMemory*                  aQmxMem,
                    stdGeometryType*            aObj1,
                    stdGeometryType*            aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getIntersectionPointPoint2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getIntersectionPointLine2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getIntersectionPointArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC getIntersectionLineLine2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getIntersectionLineArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getIntersectionAreaArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110  
                    
    static IDE_RC getIntersectionAreaArea2D4Clip(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110  
                    
    static IDE_RC getIntersectionAreaArea2D4Gpc(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110  
                    
//////////////////////////////////////////////////////////////////////////////
// Difference
//////////////////////////////////////////////////////////////////////////////
    static IDE_RC getDifference(
                    iduMemory*                  aQmxMem,
                    stdGeometryType*            aObj1,
                    stdGeometryType*            aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getDifferencePointPoint2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getDifferencePointLine2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getDifferencePointArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC getDifferenceLineLine2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getDifferenceAreaArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getDifferenceAreaArea2D4Clip(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getDifferenceAreaArea2D4Gpc(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

//////////////////////////////////////////////////////////////////////////////
// Union
//////////////////////////////////////////////////////////////////////////////
    static IDE_RC getUnion(
                    iduMemory*                  aQmxMem,
                    stdGeometryType*            aObj1,
                    stdGeometryType*            aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getUnionPointPoint2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
                    
    static IDE_RC getUnionPointLine2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getUnionPointArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getUnionLineLine2D(
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
                    
    static IDE_RC getUnionAreaArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC getUnionAreaArea2D4Clip(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC getUnionAreaArea2D4Gpc(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC getUnionCollection2D(
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

//////////////////////////////////////////////////////////////////////////////
// SymDifference
//////////////////////////////////////////////////////////////////////////////
    
    static IDE_RC getSymDifference(
                    iduMemory*                  aQmxMem,
                    stdGeometryType*            aObj1,
                    stdGeometryType*            aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getSymDifferencePointPoint2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getSymDifferenceLineLine2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet, 
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getSymDifferenceAreaArea2D(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getSymDifferenceAreaArea2D4Clip(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC getSymDifferenceAreaArea2D4Gpc(
                    iduMemory*                  aQmxMem,
                    const stdGeometryType*      aObj1,
                    const stdGeometryType*      aObj2,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

//////////////////////////////////////////////////////////////////////////////
// ConvexHull
//////////////////////////////////////////////////////////////////////////////
    
    static IDE_RC getConvexHull(
                    iduMemory*                  aQmxMem,
                    stdGeometryType*            aObj,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC getPolygonConvexHull2D(
                    iduMemory*                  aQmxMem,
                    const stdPolygon2DType*     aPoly,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );   //Fix BUG - 25110

    static IDE_RC getMultiPolygonConvexHull2D(
                    iduMemory*                      aQmxMem,
                    const stdMultiPolygon2DType*    aMPoly,
                    stdGeometryHeader*              aRet,
                    UInt                            aFence ); //Fix BUG - 25110
    
    static UInt primMakeConvexHull2D(
                    stdLinearRing2D*            aRingRet );
    
    static UInt makeChain2D( 
                    stdPoint2D*                 aPt, 
                    UInt                        aMax, 
                    SInt (*cmpPoint) (const void*, const void*) );
                    
//////////////////////////////////////////////////////////////////////////////
// Primitive Analysis Functions 
//////////////////////////////////////////////////////////////////////////////
    
    static IDE_RC primPointBuffer2D( const stdPoint2D * aPt,
                                     const SDouble      aDistance,
                                     const SInt         aPrecision,
                                     stdGeometryHeader* aValue,
                                     UInt               aFence );  //Fix BUG - 25110
    
    static IDE_RC primLineBuffer2D( const stdPoint2D * aPt1,
                                    const stdPoint2D * aPt2,
                                    const SDouble      aDistance,
                                    const SInt         aPrecision,
                                    stdGeometryHeader* aValue,
                                    UInt               aFence );  // Fix BUG - 25110
                        
    static IDE_RC primRingBuffer2D( iduMemory*                  aQmxMem,
                                    const stdPolygon2DType*     aPoly,
                                    const SDouble               aDistance,
                                    const SInt                  aPrecision,
                                    stdGeometryHeader*          aValue,
                                    UInt                        aFence );  //Fix BUG - 25110

    static IDE_RC primUnionObjects2D( iduMemory*          aQmxMem,
                                      stdGeometryHeader** aGeoms,
                                      SInt                aCnt,
                                      stdGeometryHeader * aValue,
                                      UInt                aFence );  //Fix BUG - 25110
    
    static IDE_RC primUnionObjectsPolygon2D( iduMemory*          aQmxMem,
                                             stdGeometryHeader** aGeoms,
                                             SInt                aCnt,
                                             stdGeometryHeader*  aResult,
                                             UInt                aFence );  //Fix BUG - 25110

    static IDE_RC createMultiObject2D( iduMemory*          aQmxMem,
                                       stdGeometryHeader** aGeoms,
                                       SInt                aCnt,
                                       stdGeometryHeader*  aResult,
                                       UInt                aFence );  //Fix BUG - 25110

////////////////////////////////////////////////////////////////////////////
// GPC
////////////////////////////////////////////////////////////////////////////

    static void initGpcPolygon( stdGpcPolygon * aGpcPoly );

    static IDE_RC setGpcPolygonObjLnk2D( iduMemory     * aQmxMem,
                                         stdGpcPolygon * aGpcPoly,
                                         objMainLink2D * aMLink );
    
    static IDE_RC setGpcPolygon2D( iduMemory*          aQmxMem,
                                   stdGpcPolygon    * aGpcPoly,
                                   stdPolygon2DType * aPoly );

    static IDE_RC getPolygonFromGpc2D( iduMemory             * aQmxMem,
                                       stdMultiPolygon2DType * aMPoly,
                                       stdGpcPolygon         * aGpcPoly,
                                       UInt                    aFence );  //Fix BUG - 25110
    
//////////////////////////////////////////////////////////////////////////////
// Object Link
//////////////////////////////////////////////////////////////////////////////

    static IDE_RC allocMPolyFromObjLink2D( iduMemory*              aQmxMem,
                                           stdMultiPolygon2DType** aMPoly,
                                           objMainLink2D*          aMLink,
                                           UInt                    aFence );  //Fix BUG - 25110
    
    static IDE_RC insertObjLink2D( iduMemory        * aQmxMem,
                                   objMainLink2D    * aHeader,
                                   stdPolygon2DType * aPoly );
    
//////////////////////////////////////////////////////////////////////////////
// Point Link, Line Link
//////////////////////////////////////////////////////////////////////////////

    static IDE_RC primIntersectionLineLine2D(
                    iduMemory*                  aQmxMem,
                    const stdPoint2D*           aPt11,
                    const stdPoint2D*           aPt12,
                    const stdPoint2D*           aPt21,
                    const stdPoint2D*           aPt22,
                    stdPoint2D**                aRet1,
                    stdPoint2D**                aRet2 );

    static IDE_RC primUnionLineLine2D(
                    iduMemory*                  aQmxMem,
                    const stdPoint2D*           aPt11,
                    const stdPoint2D*           aPt12,
                    const stdPoint2D*           aPt21,
                    const stdPoint2D*           aPt22,
                    stdPoint2D**                aRet1,
                    stdPoint2D**                aRet2 );

    //BUG-28693

    static IDE_RC primIntersectionLineArea2D( iduMemory                 * aQmxMem,
                                              const stdLineString2DType * aObj1,
                                              const stdPolygon2DType    * aObj2,
                                              pointLink2D              ** aPtHeader,
                                              lineLink2D               ** aLineHeader,
                                              UInt                      * aPtCnt,
                                              UInt                      * aLineCnt,
                                              UInt                        aFence);

    static IDE_RC makeGeomFromPointLineLink2D(
                    iduMemory*                  aQmxMem,
                    pointLink2D**               aPtHeader,
                    SInt                        aPtCnt,
                    lineLink2D*                 aLineHeader,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110
    
    static IDE_RC makeMPointFromPointLink2D(
                    pointLink2D*                aPtHeader,
                    stdGeometryHeader*          aRet,
                    UInt                        aFence );  //Fix BUG - 25110

    //BUG-28693

    static IDE_RC makeLineFrom2PointLink( stdPoint2D        * aFisrst,
                                          stdPoint2D        * aSecond,
                                          stdGeometryHeader * aRet,
                                          UInt                aFence);

    static IDE_RC makeMLineFromLineLink2D( iduMemory         * aQmxMem,
                                           lineLink2D        * aLineHeader,
                                           stdGeometryHeader * aRet,
                                           UInt                aFence );  //Fix BUG - 25110

    static IDE_RC makeLineLinkFromLine2D(  iduMemory*            aQmxMem,
                                           stdLineString2DType * aLine,
                                           lineLink2D**          aRetHeader );
    
    static IDE_RC makeIntersectionLineLink2D( iduMemory   * aQmxMem,
                                              lineLink2D  * aLineHeaderSubj,
                                              lineLink2D  * aLineHeaderClip,
                                              lineLink2D ** aLineHeaderRet );
    
    static IDE_RC makeDifferenceLineLink2D( iduMemory  * aQmxMem,
                                            lineLink2D * aLineHeaderSubj,
                                            lineLink2D * aLineHeaderClip );

    static IDE_RC insertPointLink2D( iduMemory    * aQmxMem,
                                     pointLink2D ** aHeader,
                                     stdPoint2D   * aPt,
                                     idBool       * aInserted );

    //BUG-28693

    static IDE_RC insertSortPointLink2D( iduMemory    * aQmxMem,
                                         pointLink2D ** aHeader,
                                         stdPoint2D   * aPt,
                                         idBool         aXOrder,
                                         idBool       * aInserted);

    static IDE_RC insertLineLink2D( iduMemory   * aQmxMem,
                                    lineLink2D ** aHeader,
                                    stdPoint2D  * aPt1,
                                    stdPoint2D  * aPt2 );
    
    static IDE_RC getPolygonClipAreaArea2D( iduMemory*               aQmxMem,
                                            const stdGeometryHeader* aObj,
                                            const stdGeometryHeader* aObj2,
                                            stdGeometryHeader*       aRet,
                                            SInt                     aOpType,
                                            UInt                     aFence );
    
};

extern "C" SInt cmpPointOrder(const void * aV1, const void *aV2, idBool aXOrder);

#endif /* _O_STF_ANALYSIS_H_ */

