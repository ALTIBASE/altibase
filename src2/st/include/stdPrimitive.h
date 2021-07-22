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
 * $Id: stdPrimitive.h 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * stdGeometry.cpp로 부터 호출되는 stdValue(), stdBinValue() 및
 * Endian 연산, validation 체크, Geometry 객체의 기본 속성 함수
 **********************************************************************/

#ifndef _O_STD_GEO_PRIMITIVE_H_
#define _O_STD_GEO_PRIMITIVE_H_        1

#include <idTypes.h>
#include <mtcDef.h>
#include <stdTypes.h>
#include <stdMethod.h>
#include <stdUtils.h>

typedef struct stdTouchPointInfo
{
    stdPoint2D          mCoord;
    idBool              mIsTouch;    
}stdTouchPointInfo;

#define ST_MAX_TOUCH_INFO_COUNT   (1000)

class stdPrimitive
{
public:
    // BUG-15981 : Add aEqualEndian 변수 추가, aEqualEndian은 변환전
    //             Geometry가 서버와 같은 Endian인지 여부를 나타낸다.
    static void cvtEndianSRID( SInt * aSRID );
    static void cvtEndianPoint2D(stdPoint2DType * aPoint);
    static void cvtEndianLineString2D( idBool aEqualEndian,
                                       stdLineString2DType * aLine);
    static void cvtEndianPolygon2D( idBool aEqualEndian,
                                    stdPolygon2DType * aPoly);
    static void cvtEndianMultiPoint2D( idBool aEqualEndian,
                                       stdMultiPoint2DType * aMPoint);
    static void cvtEndianMultiLineString2D( idBool aEqualEndian,
                                            stdMultiLineString2DType * aMLine);
    static void cvtEndianMultiPolygon2D( idBool aEqualEndian,
                                         stdMultiPolygon2DType * aMPoly);
    static void cvtEndianGeoCollection2D( idBool aEqualEndian,
                                          stdGeoCollection2DType * aCollect);
    static void cvtEndianHeader( stdGeometryHeader * aObjHeader );
    static IDE_RC cvtEndianGeom(stdGeometryHeader * aGeom);

    // To Fix BUG-16346
    static IDE_RC validate( iduMemory*   aQmxMem,
                            const void * aValue,
                            UInt         aValueSize );
    
    static IDE_RC validatePoint2D( const void*  aCol,
                                   UInt         aSize );
    
    static IDE_RC validateLineString2D( const void*  aCol,
                                        UInt         aSize );
    
    static IDE_RC validatePolygon2D(
        iduMemory *  aQmxMem,
        const void*  aCol,
        UInt         aSize );
    
    static IDE_RC validateMultiPoint2D(
        const void*  aCol,
        UInt         aSize );
    
    static IDE_RC validateMultiLineString2D(
        const void*  aCol,
        UInt         aSize );
    
    static IDE_RC validateMultiPolygon2D(
        iduMemory*   aQmxMem,
        const void*  aCol,
        UInt         aSize );
    
    static IDE_RC validateGeoCollection2D(
        iduMemory*   aQmxMem,
        const void*  aCol,
        UInt         aSize );
    
    static IDE_RC validateComplexPolygon2D(
        iduMemory*        aQmxMem,
        stdPolygon2DType* aPolygon,
        ULong             aTotal );

    static IDE_RC validateComplexMultiPolygon2D(
        iduMemory*             aQmxMem,
        stdMultiPolygon2DType* aMultiPolygon,
        ULong                  aTotalPoint );
    
    static idBool isSimpleMPoint2D(
        const stdMultiPoint2DType* aMPoint );
    static idBool isSimpleLine2D(
        const stdLineString2DType* aLine );
    static idBool isSimplePoly2D(
        const stdPolygon2DType * aPoly );
    static idBool isSimpleMLine2D(
        const stdMultiLineString2DType* aMLine );
    static idBool isSimpleMPoly2D(
        const stdMultiPolygon2DType * aMPoly );
    static idBool isSimpleCollect2D(
        const stdGeoCollection2DType* aCollect );
    static IDE_RC isValidRing2D( stdLinearRing2D* aRing );
    static idBool isClosedLine2D( const stdLineString2DType* aLine );
    static IDE_RC getBoundaryLine2D(
        const stdLineString2DType*      aLine, 
        stdGeometryHeader*              aGeom,
        UInt                            aFence );  //BUG - 25110
    static IDE_RC getBoundaryMLine2D(
        const stdMultiLineString2DType*     aMLine, 
        stdGeometryHeader*                  aGeom,
        UInt                                aFence );  //BUG - 25110
    static IDE_RC getBoundaryPoly2D(
        const stdPolygon2DType*         aPoly,
        stdGeometryHeader*              aGeom,
        UInt                            aFence );  //BUG - 25110
    static IDE_RC getBoundaryMPoly2D(
        const stdMultiPolygon2DType*    aMPoly,
        stdGeometryHeader*              aGeom,
        UInt                            aFence );  //BUG - 25110

    static idBool GetEnvelope2D(
        const stdGeometryHeader*        aHeader,
        stdGeometryHeader*              aGeom );

    // BUG-48051
    static IDE_RC validatePolygon2DSize( stdGeometryHeader    * aGeom );
    static IDE_RC validateMPolygon2DSize( stdGeometryHeader   * aGeom );
    static IDE_RC validateGeoColl2DSize( stdGeometryHeader    * aGeom );
};

#endif /* _O_STD_GEO_PRIMITIVE_H_ */
