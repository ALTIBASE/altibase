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
 * $Id: stfFunctions.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체가 타입 별로 가지는 속성 함수
 **********************************************************************/

#ifndef _O_STF_FUNCTIONS_H_
#define _O_STF_FUNCTIONS_H_        1

#include <idTypes.h>
#include <mtcDef.h>
#include <stdTypes.h>

typedef enum 
{
    ST_GET_MINX,
    ST_GET_MINY,
    ST_GET_MAXX,
    ST_GET_MAXY
}  stGetMinMaxXYParam;


class stfFunctions
{
public:

// Point -----------------------------------------------------------------------
    static IDE_RC coordX(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC coordY(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

// LineString/MultiLineString --------------------------------------------------
    static IDE_RC startPoint(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC endPoint(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    // MultiLineString
    static IDE_RC isClosed(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    // MultiLineString
    static IDE_RC isRing(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    // MultiLineString
    static IDE_RC length(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    // MultiLineString
    static IDE_RC numPoints(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC pointN(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

// Polygon ---------------------------------------------------------------------
    static IDE_RC centroid(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC pointOnSurface(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

     // MultiSurface
    static IDE_RC area(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC exteriorRing(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC numInteriorRing(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC interiorRingN(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

// GeoCollection ---------------------------------------------------------------
    static IDE_RC numGeometries(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );

    static IDE_RC geometryN(
                    mtcNode*     aNode,
                    mtcStack*    aStack,
                    SInt         aRemain,
                    void*        aInfo,
                    mtcTemplate* aTemplate );
                    
                    
                    
/**************************************************************************/
    static IDE_RC getStartPoint(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet );
                    
    static IDE_RC getEndPoint(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet );

    static IDE_RC testClose(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );

    static IDE_RC testRing(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );

    static IDE_RC getLength(
                    stdGeometryHeader*  aObj,
                    SDouble*            aRet );

    static IDE_RC getNumPoints(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );
                    
    static IDE_RC getPointN(
                    stdGeometryHeader*  aObj,
                    UInt                aNum,
                    stdGeometryHeader*  aRet );
                    
    static IDE_RC getCentroid(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet );

    static IDE_RC getPointOnSurface(
                    iduMemory*          aQmxMem,
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet );

    static IDE_RC getArea(
                    stdGeometryHeader*  aObj,
                    SDouble*            aRet );
                    
    static IDE_RC getExteriorRing(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet );
                    
    static IDE_RC getNumInteriorRing(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );

    static IDE_RC getInteriorRingN(
                    stdGeometryHeader*  aObj,
                    UInt                aNum,
                    stdGeometryHeader*  aRet );

    static IDE_RC getNumGeometries(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet );
                    
    static IDE_RC getGeometryN(
                    stdGeometryHeader*  aObj,
                    UInt                aNum,
                    stdGeometryHeader*  aRet );
    static IDE_RC minMaxXY(
        mtcNode*     aNode,
        mtcStack*    aStack,
        SInt         aRemain,
        void*        aInfo,
        mtcTemplate* aTemplate,
        stGetMinMaxXYParam aParam);

    /* BUG-45645 ST_Reverse, ST_MakeEnvelope 함수 지원 */
    static IDE_RC reverseGeometry( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

    static IDE_RC reverseAll( stdGeometryHeader * aObj,
                              stdGeometryHeader * aRet );

    static IDE_RC makeEnvelope( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

    /* BUG-47857 ST_MakePoint, ST_Point 함수 지원 */ 
    static IDE_RC makePoint( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );
};

#endif /* _O_STF_FUNCTIONS_H_ */
