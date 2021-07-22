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
 * $Id: stfRelation.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체와 Geometry 객체간의 관계 함수
 **********************************************************************/

#ifndef _O_STF_RELATION_H_
#define _O_STF_RELATION_H_        1

#include <idTypes.h>
#include <mtcDef.h>
#include <stdTypes.h>

//==============================================================
// BUG-16952
// Angle상에서의 점의 위치
//
//                 Am
//       1         |                               1
//                 |        
//                 4     2              Ap---3->>--Am---4---An
//                 |
//     Ap---->>-3--An                              2
//
//              2
//
//==============================================================

typedef enum
{
    STF_UNKNOWN_ANGLE_POS = 0,
    STF_INSIDE_ANGLE_POS = 1,
    STF_OUTSIDE_ANGLE_POS = 2,
    STF_MIN_ANGLE_POS = 3,
    STF_MAX_ANGLE_POS = 4,
    STF_MAXMAX_ANGLE_POS
} stfAnglePos;

class stfRelation
{
public:
    /* Calculate MBR ******************************************************/
    /* 연산 결과가 TRUE일 경우 1, 그렇지 않을 경우 0을 리턴
       인덱스의 Compare를 위하여
    */
    
    static SInt isMBRIntersects( mtdValueInfo * aValueInfo1,
                                 mtdValueInfo * aValueInfo2 );
    
    static SInt isMBRContains( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 );
    
    static SInt isMBRWithin( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 );
    
    static SInt isMBREquals( mtdValueInfo * aValueInfo1,
                             mtdValueInfo * aValueInfo2 );

    // BUG-16478
    static idBool  isValidPatternContents( const SChar *aPattern );

    static SInt compareMBR( mtdValueInfo * aValueInfo1,
                            mtdValueInfo * aValueInfo2 );

    static SInt compareMBRKey( mtdValueInfo * aValueInfo1,
                               mtdValueInfo * aValueInfo2 );
    
    static SInt stfMinXLTEMinX( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 );

    
    static SInt stfMinXLTEMaxX( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2  );
    
    static SInt stfFilterIntersects( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * aValueInfo2 );
    

        
    static SInt stfFilterContains( mtdValueInfo * aValueInfo1,
                                   mtdValueInfo * aValueInfo2 );
        
    /* Relation Functions **************************************************/
    static IDE_RC isEquals( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );
    static IDE_RC isDisjoint( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );
    static IDE_RC isIntersects( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );
    static IDE_RC isWithin( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );
    static IDE_RC isContains( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );
    static IDE_RC isCrosses( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );
    static IDE_RC isOverlaps( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );
                    
    static IDE_RC isTouches( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

    static IDE_RC isRelate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

    static IDE_RC matrixEquals( stdGeometryType*    aGeom1,
                                stdGeometryType*    aGeom2,
                                SChar*              aPattern );
    static IDE_RC matrixDisjoint( stdGeometryType*    aGeom1,
                                  stdGeometryType*    aGeom2,
                                  SChar*              aPattern );
    static IDE_RC matrixWithin( stdGeometryType*    aGeom1,
                                stdGeometryType*    aGeom2,
                                SChar*              aPattern );
    static IDE_RC matrixCrosses( stdGeometryType*    aGeom1,
                                 stdGeometryType*    aGeom2,
                                 SChar*              aPattern );
    static IDE_RC matrixOverlaps( stdGeometryType*    aGeom1,
                                  stdGeometryType*    aGeom2,
                                  SChar*              aPattern );
                    
    static IDE_RC matrixTouches( stdGeometryType*    aGeom1,
                                 stdGeometryType*    aGeom2,
                                 SChar               aPattern[3][10],
                                 UInt&               aPatternCnt );

    /* Calculate Relation *************************************************/
    // relate를 호출 하기 전에 2D와 3D에 대한 필터링이 끝나므로
    // relate 펑션 하부에서 수행되는 펑션들은 차원에 대해 비교할 필요 없다.
    static IDE_RC relate( iduMemory*             aQmxMem,
                          const stdGeometryType* aObj1,
                          const stdGeometryType* aObj2,
                          SChar*                 aPattern,
                          mtdBooleanType*        aResult);

    // SPvsG
    static SChar pointTogeometry( SInt                           aIndex,
                                  const stdGeometryType*         aObj1,
                                  const stdGeometryType*         aObj2 );    
    // SLvsG
    static SChar linestringTogeometry( SInt                           aIndex,
                                       const stdGeometryType*         aObj1,
                                       const stdGeometryType*         aObj2 );    
    // SAvsG
    static IDE_RC polygonTogeometry( iduMemory*                     aQmxMem,
                                     SInt                           aIndex,
                                     const stdGeometryType*         aObj1,
                                     const stdGeometryType*         aObj2,
                                     SChar*                         aResult);
    // MPvsG
    static SChar multipointTogeometry( SInt                           aIndex,
                                       const stdGeometryType*         aObj1,
                                       const stdGeometryType*         aObj2 );
    // MLvsG
    static SChar multilinestringTogeometry( SInt                           aIndex,
                                            const stdGeometryType*         aObj1,
                                            const stdGeometryType*         aObj2 );
    // MAvsG
    static IDE_RC multipolygonTogeometry( iduMemory*                     aQmxMem,
                                          SInt                           aIndex,
                                          const stdGeometryType*         aObj1,
                                          const stdGeometryType*         aObj2,
                                          SChar*                         aResult);
    // SGvsG
    static IDE_RC collectionTogeometry( iduMemory*                     aQmxMem,
                                        SInt                           aIndex,
                                        const stdGeometryType*         aObj1,
                                        const stdGeometryType*         aObj2,
                                        SChar*                         aResult);

    /* Relation Primitives ************************************************/

    static mtdBooleanType checkMatch( SChar aPM, SChar aResult );

    // To Fix BUG-16912
    // Intersection Model Comparison between Line Segments.
    static SChar compLineSegment( stdPoint2D * aSeg1Pt1,         // LineSeg 1
                                  stdPoint2D * aSeg1Pt2,         // LineSeg 1
                                  idBool       aIsTermSeg1Pt1,   // is terminal
                                  idBool       aIsTermSeg1Pt2,   // is terminal
                                  stdPoint2D * aSeg2Pt1,         // LineSeg 2
                                  stdPoint2D * aSeg2Pt2,         // LineSeg 2 
                                  idBool       aIsTermSeg2Pt1,   // is terminal
                                  idBool       aIsTermSeg2Pt2 ); // is terminal

    static SChar EndLineToMidLine( stdPoint2D*     aPtEnd,      // End Point
                                   stdPoint2D*     aPtEndNext,
                                   stdPoint2D*     aPtMid1,
                                   stdPoint2D*     aPtMid2 );
    static SChar MidLineToMidLine( stdPoint2D*     aPt11,
                                   stdPoint2D*     aPt12,
                                   stdPoint2D*     aPt21,
                                   stdPoint2D*     aPt22 );

    static idBool lineInLineString( stdPoint2D*                     aPt1,
                                    stdPoint2D*                     aPt2,
                                    const stdLineString2DType*      aLine );
    static idBool lineInRing( stdPoint2D*             aPt1,
                              stdPoint2D*             aPt2,
                              stdLinearRing2D*        aRing );
    
    /* Point **************************************************************/
    // Single  Point        Interior
    // Multi   Linestring   Boundary
    //         Polygon      Exterior
    //         Collection

    // point to point 
    static SChar spiTospi( const stdPoint2D*                aObj1,
                           const stdPoint2D*                aObj2 );
    static SChar spiTospe( const stdPoint2D*                aObj1,
                           const stdPoint2D*                aObj2 );
    // point vs linestring 
    static SChar spiTosli( const stdPoint2D*                aObj1,
                           const stdLineString2DType*       aObj2 );    
    static SChar spiToslb( const stdPoint2D*                aObj1,
                           const stdLineString2DType*       aObj2 );
    static SChar spiTosle( const stdPoint2D*                aObj1,
                           const stdLineString2DType*       aObj2 );
    // point vs polygon
    static SChar spiTosai( const stdPoint2D*                aObj1,
                           const stdPolygon2DType*          aObj2 );    

    static SChar spiTosab( const stdPoint2D*                aObj1,
                           const stdPolygon2DType*          aObj2 );    
    static SChar spiTosae( const stdPoint2D*                aObj1,
                           const stdPolygon2DType*          aObj2 );
                           
    // point vs multipoint
    static SChar spiTompi( const stdPoint2D*                aObj1,
                           const stdMultiPoint2DType*       aObj2 );    
    static SChar spiTompe( const stdPoint2D*                aObj1,
                           const stdMultiPoint2DType*       aObj2 );    
    static SChar speTompi( const stdPoint2D*                aObj1,
                           const stdMultiPoint2DType*       aObj2 );    
    // point vs multilinestring
    static SChar spiTomli( const stdPoint2D*                aObj1,
                           const stdMultiLineString2DType*  aObj2 );
    static SChar spiTomlb( const stdPoint2D*                aObj1,
                           const stdMultiLineString2DType*  aObj2 );
    static SChar spiTomle( const stdPoint2D*                aObj1,
                           const stdMultiLineString2DType*  aObj2 );   
    // point vs multipolygon
    static SChar spiTomai( const stdPoint2D*                aObj1,
                           const stdMultiPolygon2DType*     aObj2 );
    static SChar spiTomab( const stdPoint2D*                aObj1,
                           const stdMultiPolygon2DType*     aObj2 );    
    static SChar spiTomae( const stdPoint2D*                aObj1,
                           const stdMultiPolygon2DType*     aObj2 );    
    // point vs collection
    static SChar spiTogci( const stdPoint2D*                aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar spiTogcb( const stdPoint2D*                aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar spiTogce( const stdPoint2D*                aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar speTogci( const stdPoint2D*                aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar speTogcb( const stdPoint2D*                aObj1,
                           const stdGeoCollection2DType*    aObj2 );

    /* linestring *********************************************************/
    // linestring vs linestring
    static SChar sliTosli( const stdLineString2DType*        aObj1,
                           const stdLineString2DType*        aObj2 );
    static SChar sliToslb( const stdLineString2DType*        aObj1,
                           const stdLineString2DType*        aObj2 );
    static SChar sliTosle( const stdLineString2DType*        aObj1,
                           const stdLineString2DType*        aObj2 );
                           
    static SChar slbToslb( const stdLineString2DType*        aObj1,
                           const stdLineString2DType*        aObj2 );
    static SChar slbTosle( const stdLineString2DType*        aObj1,
                           const stdLineString2DType*        aObj2 );
    // linestring vs polygon
    static SChar sliTosai( const stdLineString2DType*        aLineObj,
                           const stdPolygon2DType*           aAreaObj );
    static SChar sliTosab( const stdLineString2DType*        aObj1,
                           const stdPolygon2DType*            aObj2 );
    static SChar sliTosae( const stdLineString2DType*        aLineObj,
                           const stdPolygon2DType*           aAreaObj );
    static SChar slbTosai( const stdLineString2DType*        aObj1,
                           const stdPolygon2DType*            aObj2 );
    static SChar slbTosab( const stdLineString2DType*        aObj1,
                           const stdPolygon2DType*            aObj2 );
    static SChar slbTosae( const stdLineString2DType*        aObj1,
                           const stdPolygon2DType*            aObj2 );
    static SChar sleTosab( const stdLineString2DType*        aObj1,
                           const stdPolygon2DType*            aObj2 );
                           
    // linestring vs multipoint
    static SChar sliTompi( const stdLineString2DType*        aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    static SChar slbTompi( const stdLineString2DType*        aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    static SChar slbTompe( const stdLineString2DType*        aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    static SChar sleTompi( const stdLineString2DType*        aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    // linestring vs multilinestring
    static SChar sliTomli( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sliTomlb( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sliTomle( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar slbTomli( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar slbTomlb( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar slbTomle( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sleTomli( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sleTomlb( const stdLineString2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
                           
    // linestring vs multipolygon
    static SChar sliTomai( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar sliTomab( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar sliTomae( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar slbTomai( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar slbTomab( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar slbTomae( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar sleTomab( const stdLineString2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    // linestring vs geometrycollection
    static SChar sliTogci( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar sliTogcb( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar sliTogce( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar slbTogci( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar slbTogcb( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar slbTogce( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar sleTogci( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar sleTogcb( const stdLineString2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );

    /* polygon ************************************************************/
    // polygon to polygon
    static IDE_RC saiTosai( iduMemory*                         aQmxMem,
                            const stdPolygon2DType*            a1stArea,
                            const stdPolygon2DType*            a2ndArea,
                            SChar*                             aResult );
    static SChar saiTosab( const stdPolygon2DType*            aAreaInt,
                           const stdPolygon2DType*            aAreaBnd );
    static IDE_RC saiTosae( iduMemory*                         aQmxMem,
                            const stdPolygon2DType*            aAreaInt,
                            const stdPolygon2DType*            aAreaExt,
                            SChar*                             aResult );
    static SChar sabTosab( const stdPolygon2DType*            aObj1,
                           const stdPolygon2DType*            aObj2 );
    static SChar sabTosae( const stdPolygon2DType*            aAreaBnd,
                           const stdPolygon2DType*            aAreaExt );
    // polygon vs multipoint
    static SChar saiTompi( const stdPolygon2DType*            aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    static SChar sabTompi( const stdPolygon2DType*            aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    static SChar saeTompi( const stdPolygon2DType*            aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    // polygon vs multilinestring
    static SChar saiTomli( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar saiTomlb( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sabTomli( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sabTomlb( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar sabTomle( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar saeTomli( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar saeTomlb( const stdPolygon2DType*            aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    // polygon vs multipolygon
    static IDE_RC saiTomai( iduMemory*                      aQmxMem,
                            const stdPolygon2DType*         aObj1,
                            const stdMultiPolygon2DType*    aObj2,
                            SChar*                          aResult );
    static SChar saiTomab( const stdPolygon2DType*          aObj1,
                           const stdMultiPolygon2DType*     aObj2 );
    static IDE_RC saiTomae( iduMemory*                      aQmxMem,
                            const stdPolygon2DType*         aObj1,
                            const stdMultiPolygon2DType*    aObj2,
                            SChar*                          aResult);
    static SChar sabTomai( const stdPolygon2DType*          aObj1,
                           const stdMultiPolygon2DType*     aObj2 );
    static SChar sabTomab( const stdPolygon2DType*          aObj1,
                           const stdMultiPolygon2DType*     aObj2 );
    static SChar sabTomae( const stdPolygon2DType*          aObj1,
                           const stdMultiPolygon2DType*     aObj2 );
    static IDE_RC saeTomai( iduMemory*                      aQmxMem,
                            const stdPolygon2DType*         aObj1,
                            const stdMultiPolygon2DType*    aObj2,
                            SChar*                          aResult);
    static SChar saeTomab( const stdPolygon2DType*          aObj1,
                           const stdMultiPolygon2DType*     aObj2 );
                           
    // polygon vs geometrycollection
    static IDE_RC saiTogci( iduMemory*                      aQmxMem,
                            const stdPolygon2DType*         aObj1,
                            const stdGeoCollection2DType*   aObj2,
                            SChar*                          aResult );
    static SChar saiTogcb( const stdPolygon2DType*          aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static IDE_RC saiTogce( iduMemory*                      aQmxMem,
                            const stdPolygon2DType*         aObj1,
                            const stdGeoCollection2DType*   aObj2,
                            SChar*                          aResult );
    static SChar sabTogci( const stdPolygon2DType*          aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar sabTogcb( const stdPolygon2DType*          aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar sabTogce( const stdPolygon2DType*          aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static IDE_RC saeTogci( iduMemory*                      aQmxMem,
                            const stdPolygon2DType*         aObj1,
                            const stdGeoCollection2DType*   aObj2,
                            SChar*                          aResult);
    static SChar saeTogcb( const stdPolygon2DType*          aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    
    /* multipoint *********************************************************/
    // multipoint vs multipoint
    static SChar mpiTompi( const stdMultiPoint2DType*        aObj1,
                           const stdMultiPoint2DType*        aObj2 );
    static SChar mpiTompe( const stdMultiPoint2DType*        aObj1,
                           const stdMultiPoint2DType*        aObj2 );    
    // multipoint vs multilinestring
    static SChar mpiTomli( const stdMultiPoint2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mpiTomlb( const stdMultiPoint2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mpiTomle( const stdMultiPoint2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mpeTomlb( const stdMultiPoint2DType*        aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    // multipoint vs multipolygon
    static SChar mpiTomai( const stdMultiPoint2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mpiTomab( const stdMultiPoint2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mpiTomae( const stdMultiPoint2DType*        aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    // multipoint vs geometrycollection
    static SChar mpiTogci( const stdMultiPoint2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mpiTogcb( const stdMultiPoint2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mpiTogce( const stdMultiPoint2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mpeTogci( const stdMultiPoint2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mpeTogcb( const stdMultiPoint2DType*        aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    
    /* multilinestring ****************************************************/
    // multilinestring vs multilinestring
    static SChar mliTomli( const stdMultiLineString2DType*    aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mliTomlb( const stdMultiLineString2DType*    aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mliTomle( const stdMultiLineString2DType*    aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mlbTomlb( const stdMultiLineString2DType*    aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    static SChar mlbTomle( const stdMultiLineString2DType*    aObj1,
                           const stdMultiLineString2DType*    aObj2 );
    // multilinestring vs multipolygon
    static SChar mliTomai( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mliTomab( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mliTomae( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mlbTomai( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mlbTomab( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mlbTomae( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mleTomab( const stdMultiLineString2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    // multilinestring vs geometrycollection
    static SChar mliTogci( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mliTogcb( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mliTogce( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mlbTogci( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mlbTogcb( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mlbTogce( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mleTogci( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mleTogcb( const stdMultiLineString2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );

    /* multipolygon *******************************************************/
    // multipolygon vs multipolygon
    static IDE_RC maiTomai( iduMemory*                     aQmxMem,
                            const stdMultiPolygon2DType*   aObj1,
                            const stdMultiPolygon2DType*   aObj2,
                            SChar*                         aResult );
    static SChar maiTomab( const stdMultiPolygon2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static IDE_RC maiTomae( iduMemory*                     aQmxMem,
                            const stdMultiPolygon2DType*   aObj1,
                            const stdMultiPolygon2DType*   aObj2,
                            SChar*                         aResult);
    static SChar mabTomab( const stdMultiPolygon2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    static SChar mabTomae( const stdMultiPolygon2DType*    aObj1,
                           const stdMultiPolygon2DType*    aObj2 );
    // multipolygon vs geometrycollection 
    static IDE_RC maiTogci( iduMemory*                      aQmxMem,
                            const stdMultiPolygon2DType*    aObj1,
                            const stdGeoCollection2DType*   aObj2,
                            SChar*                          aResult );
    static SChar maiTogcb( const stdMultiPolygon2DType*     aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static IDE_RC maiTogce( iduMemory*                      aQmxMem,
                            const stdMultiPolygon2DType*    aObj1,
                            const stdGeoCollection2DType*   aObj2,
                            SChar*                          aResult );
    static SChar mabTogci( const stdMultiPolygon2DType*     aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mabTogcb( const stdMultiPolygon2DType*     aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar mabTogce( const stdMultiPolygon2DType*     aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static IDE_RC maeTogci( iduMemory*                      aQmxMem,
                            const stdMultiPolygon2DType*    aObj1,
                            const stdGeoCollection2DType*   aObj2,
                            SChar*                          aResult );
    static SChar maeTogcb( const stdMultiPolygon2DType*     aObj1,
                           const stdGeoCollection2DType*    aObj2 );

    /* geometrycollection *************************************************/
    // geometrycollection vs geometrycollection
    static IDE_RC gciTogci( iduMemory*                       aQmxMem,
                            const stdGeoCollection2DType*    aObj1,
                            const stdGeoCollection2DType*    aObj2,
                            SChar*                           aResult );
    static SChar gciTogcb( const stdGeoCollection2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static IDE_RC gciTogce( iduMemory*                       aQmxMem,
                            const stdGeoCollection2DType*    aObj1,
                            const stdGeoCollection2DType*    aObj2,
                            SChar*                           aResult );
    static SChar gcbTogcb( const stdGeoCollection2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );
    static SChar gcbTogce( const stdGeoCollection2DType*    aObj1,
                           const stdGeoCollection2DType*    aObj2 );

    // BUG-17010
    // Line Segment와 Ring Segment의 원하는 관계가 존재하는 지 검사
    static idBool hasRelLineSegRingSeg( idBool      aIsExtRing,
                                        idBool      aIsCCWiseRing,
                                        stdPoint2D* aRingPrevPt,
                                        stdPoint2D* aRingNextPt,
                                        stdPoint2D* aMeetPoint,
                                        stdPoint2D* aLinePrevPt,
                                        stdPoint2D* aLineNextPt,
                                        stfAnglePos aWantPos );

    // BUG-16952
    // 점이 각의 어느 위치에 존재하는지를 판단
    static stfAnglePos wherePointInAngle( stdPoint2D * aAnglePrevPt,
                                          stdPoint2D * aAngleMiddPt,
                                          stdPoint2D * aAngleNextPt,
                                          stdPoint2D * aTestPt );

    // BUG-34878
    static IDE_RC relateAreaArea( iduMemory*             aQmxMem,
                                  const stdGeometryType* aObj1, 
                                  const stdGeometryType* aObj2,
                                  SChar*                 aPattern,
                                  mtdBooleanType*        aReturn );

    static void setDE9MatrixValue( SChar* aMatrix, 
                                   SInt   aMatrixIndex, 
                                   SInt   aOrder );

    static idBool checkRelateResult( SChar*          aPattern, 
                                     SChar*          aResultMatrix, 
                                     mtdBooleanType* aResult );
};

/* De-9im Matrix Index */
#define STF_INTER_INTER (0)
#define STF_INTER_BOUND (1)
#define STF_INTER_EXTER (2)
#define STF_BOUND_INTER (3)
#define STF_BOUND_BOUND (4)
#define STF_BOUND_EXTER (5)
#define STF_EXTER_INTER (6)
#define STF_EXTER_BOUND (7)
#define STF_EXTER_EXTER (8)


#define STF_INTERSECTS_DIM_NOT (0)
#define STF_INTERSECTS_DIM_0   (1)
#define STF_INTERSECTS_DIM_1   (2)
#define STF_INTERSECTS_DIM_2   (3)

#endif /* _O_STF_RELATION_H_ */

