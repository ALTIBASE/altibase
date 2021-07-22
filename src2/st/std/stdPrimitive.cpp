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
 * $Id: stdPrimitive.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * stdGeometry.cpp로 부터 호출되는 stdValue(), stdBinValue() 및
 * Endian 연산, validation 체크, Geometry 객체의 기본 속성 함수 구현
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>

#include <idu.h>
#include <ste.h>
#include <stcDef.h>
#include <stdTypes.h>
#include <stdUtils.h>
#include <stdParsing.h>
#include <stdPrimitive.h>
#include <stdUtils.h>
#include <stuProperty.h>

extern mtdModule stdGeometry;
extern mtdModule mtdSmallint;
extern mtdModule mtdInteger;
extern mtdModule mtdDouble;

void stdPrimitive::cvtEndianSRID( SInt * aSRID )
{
    mtdInteger.endian( aSRID );
}

/***********************************************************************
 * Description :
 * stdPoint2DType 객체의 Endian을 바꾼다.
 *
 * stdPoint2DType * aPoint(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianPoint2D( stdPoint2DType * aPoint)
{
    mtdDouble.endian(&aPoint->mPoint.mX);
    mtdDouble.endian(&aPoint->mPoint.mY);

    cvtEndianHeader( (stdGeometryHeader*) aPoint);
    return;
}

/***********************************************************************
 * Description :
 * stdLineString2DType 객체의 Endian을 바꾼다.
 *
 * stdLineString2DType * aLine(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianLineString2D( idBool aEqualEndian, stdLineString2DType * aLine)
{
    stdPoint2D*         sPt;
    UInt                i, sMax;

    sPt = STD_FIRST_PT2D(aLine);
    sMax = STD_N_POINTS(aLine);
    if( aEqualEndian != ID_TRUE )
    { 
        mtdInteger.endian( &sMax );
    }
        
    for( i = 0 ; i < sMax ; i++ )
    {    
        mtdDouble.endian(&sPt->mX);
        mtdDouble.endian(&sPt->mY);
        sPt = STD_NEXT_PT2D(sPt);
    }
    mtdInteger.endian(&aLine->mNumPoints);
    cvtEndianHeader( (stdGeometryHeader*) aLine );
}

/***********************************************************************
 * Description :
 * stdPolygon2DType 객체의 Endian을 바꾼다.
 *
 * stdPolygon2DType * aPoly(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianPolygon2D( idBool aEqualEndian, stdPolygon2DType * aPoly)
{
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPt;
    UInt                i, j, sMaxR, sMax;

    sRing = STD_FIRST_RN2D(aPoly);
    sMaxR = STD_N_RINGS(aPoly);
    if( aEqualEndian != ID_TRUE )
    { 
        mtdInteger.endian( &sMaxR );
    }
    for( i = 0 ; i < sMaxR ; i++ )
    {
        sPt = STD_FIRST_PT2D(sRing);
        sMax = STD_N_POINTS(sRing);
        if( aEqualEndian != ID_TRUE )
        { 
            mtdInteger.endian( &sMax );
        }
        for( j = 0 ; j < sMax ; j++ )
        {    
            mtdDouble.endian(&sPt->mX);
            mtdDouble.endian(&sPt->mY);
            sPt = STD_NEXT_PT2D(sPt);
        }
        mtdInteger.endian(&sRing->mNumPoints);        
        sRing = (stdLinearRing2D*)sPt;
    }
    mtdInteger.endian(&aPoly->mNumRings);
    cvtEndianHeader( (stdGeometryHeader*) aPoly );
}

/***********************************************************************
 * Description :
 * stdMultiPoint2DType 객체의 Endian을 바꾼다.
 *
 * stdMultiPoint2DType * aMPoint(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianMultiPoint2D( idBool aEqualEndian, stdMultiPoint2DType * aMPoint)
{
    stdPoint2DType*     sPt;
    UInt                i, sMax;

    sPt = STD_FIRST_POINT2D(aMPoint);
    sMax = STD_N_OBJECTS(aMPoint);
    if( aEqualEndian != ID_TRUE )
    { 
        mtdInteger.endian( &sMax );
    }
    for( i = 0 ; i < sMax ; i++ )
    {
        mtdDouble.endian(&sPt->mPoint.mX);
        mtdDouble.endian(&sPt->mPoint.mY);
        cvtEndianHeader( (stdGeometryHeader*) sPt );
        sPt = STD_NEXT_POINT2D(sPt);
    }    
    mtdInteger.endian(&aMPoint->mNumObjects);
    cvtEndianHeader( (stdGeometryHeader*) aMPoint );
}

/***********************************************************************
 * Description :
 * stdMultiLineString2DType 객체의 Endian을 바꾼다.
 *
 * stdMultiLineString2DType * aMLine(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianMultiLineString2D( idBool aEqualEndian,stdMultiLineString2DType * aMLine)
{
    stdLineString2DType* sLine;
    stdPoint2D*          sPt;
    UInt                 i, j, sMaxO, sMax;

    sLine = STD_FIRST_LINE2D(aMLine);
    sMaxO = STD_N_OBJECTS(aMLine);
    if( aEqualEndian != ID_TRUE )
    { 
        mtdInteger.endian( &sMaxO );
    }
    for( i = 0 ; i < sMaxO ; i++ )
    {
        sPt = STD_FIRST_PT2D(sLine);
        sMax = STD_N_POINTS(sLine);
        if( aEqualEndian != ID_TRUE )
        { 
            mtdInteger.endian( &sMax );
        }
        for( j = 0 ; j < sMax ; j++ )
        {    
            mtdDouble.endian(&sPt->mX);
            mtdDouble.endian(&sPt->mY);
            sPt = STD_NEXT_PT2D(sPt);
        }        
        mtdInteger.endian(&sLine->mNumPoints);
        cvtEndianHeader( (stdGeometryHeader*) sLine );
        sLine = (stdLineString2DType*)sPt;
    }    
    mtdInteger.endian(&aMLine->mNumObjects);
    cvtEndianHeader( (stdGeometryHeader*) aMLine );
}

/***********************************************************************
 * Description :
 * stdMultiPolygon2DType 객체의 Endian을 바꾼다.
 *
 * stdMultiPolygon2DType * aMPoly(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianMultiPolygon2D( idBool aEqualEndian, stdMultiPolygon2DType * aMPoly)
{
    stdPolygon2DType* sPolygon;
    stdLinearRing2D*  sRing;
    stdPoint2D*       sPt;
    UInt              i, j, k, sMaxO, sMaxR, sMax;

    sPolygon = STD_FIRST_POLY2D(aMPoly);
    sMaxO = STD_N_OBJECTS(aMPoly);
    if( aEqualEndian != ID_TRUE )
    { 
        mtdInteger.endian( &sMaxO );
    }
    for( i = 0 ; i < sMaxO ; i++ )
    {
        sRing = STD_FIRST_RN2D(sPolygon);
        sMaxR = STD_N_RINGS(sPolygon);
        if( aEqualEndian != ID_TRUE )
        { 
            mtdInteger.endian( &sMaxR );
        }
        for( j = 0 ; j < sMaxR ; j++ )
        {
            sPt = STD_FIRST_PT2D(sRing);
            sMax = STD_N_POINTS(sRing);
            if( aEqualEndian != ID_TRUE )
            { 
                mtdInteger.endian( &sMax );
            }
            for( k = 0 ; k < sMax ; k++ )
            {
                mtdDouble.endian(&sPt->mX);
                mtdDouble.endian(&sPt->mY);
                sPt = STD_NEXT_PT2D(sPt);
            }
            mtdInteger.endian(&sRing->mNumPoints);        
            sRing = (stdLinearRing2D*)sPt;
        }
        mtdInteger.endian(&sPolygon->mNumRings);
        cvtEndianHeader( (stdGeometryHeader*) sPolygon );
        sPolygon = (stdPolygon2DType*)sRing;
    }    
    mtdInteger.endian(&aMPoly->mNumObjects);
    cvtEndianHeader( (stdGeometryHeader*) aMPoly );
}

/***********************************************************************
 * Description :
 * stdGeoCollection2DType 객체의 Endian을 바꾼다.
 *
 * stdGeoCollection2DType * aCollect(InOut): Endian을 바꿀 객체
 **********************************************************************/
void stdPrimitive::cvtEndianGeoCollection2D( idBool aEqualEndian, stdGeoCollection2DType * aCollect)
{
    stdGeometryHeader* sGeom;
    UInt               i, sMax, sGeomSize;
    
    sGeom = (stdGeometryHeader*)STD_FIRST_COLL2D(aCollect);
    sMax = STD_N_GEOMS(aCollect);
    if( aEqualEndian != ID_TRUE )
    { 
        mtdInteger.endian( &sMax );
    }
    for( i = 0 ; i < sMax ; i++ )
    {
        switch( stdUtils::getType(sGeom) )
        {
        case STD_POINT_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdPoint2DExtType*)sGeom)->mSRID) );
        case STD_POINT_2D_TYPE:
            cvtEndianPoint2D((stdPoint2DType*)sGeom);
            break;
            
        case STD_LINESTRING_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdLineString2DExtType*)sGeom)->mSRID) );
        case STD_LINESTRING_2D_TYPE:
            cvtEndianLineString2D( aEqualEndian, (stdLineString2DType*)sGeom);
            break;
            
        case STD_POLYGON_2D_EXT_TYPE :
            cvtEndianSRID( &(((stdPolygon2DExtType*)sGeom)->mSRID) );
        case STD_POLYGON_2D_TYPE :
            cvtEndianPolygon2D( aEqualEndian, (stdPolygon2DType*)sGeom);
            break;
            
        case STD_MULTIPOINT_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdMultiPoint2DExtType*)sGeom)->mSRID) );
        case STD_MULTIPOINT_2D_TYPE:
            cvtEndianMultiPoint2D( aEqualEndian, (stdMultiPoint2DType*)sGeom);
            break;
            
        case STD_MULTILINESTRING_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdMultiLineString2DExtType*)sGeom)->mSRID) );
        case STD_MULTILINESTRING_2D_TYPE:
            cvtEndianMultiLineString2D( aEqualEndian, (stdMultiLineString2DType*)sGeom);    
            break;
            
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdMultiPolygon2DExtType*)sGeom)->mSRID) );
        case STD_MULTIPOLYGON_2D_TYPE:
            cvtEndianMultiPolygon2D( aEqualEndian, (stdMultiPolygon2DType*)sGeom);    
            break;
        default:
            // BUG-29422
            IDE_RAISE(NORMAL_EXIT);
        }

        sGeomSize = ((stdGeometryHeader*)sGeom)->mSize;
        if( aEqualEndian == ID_TRUE )
        {
            mtdInteger.endian( &sGeomSize );
        }
        sGeom = (stdGeometryHeader*) ( ((SChar*)sGeom) + sGeomSize );
    }
    mtdInteger.endian(&aCollect->mNumGeometries);
    cvtEndianHeader( (stdGeometryHeader*) aCollect );

    IDE_EXCEPTION_CONT(NORMAL_EXIT);
}

/***********************************************************************
 * Description :
 * 
 *    Object Header 구조체의 Endian을 바꾼다.
 *
 *    BUG-15972 Type과 Byte Order의 Endian도 변경하여야 함.
 *
 **********************************************************************/
void stdPrimitive::cvtEndianHeader( stdGeometryHeader * aObjHeader )
{
    // Type
    mtdSmallint.endian( & aObjHeader->mType );
    
    // Byte Order
    aObjHeader->mByteOrder = ( aObjHeader->mByteOrder == STD_BIG_ENDIAN ) ?
        STD_LITTLE_ENDIAN : STD_BIG_ENDIAN;

    // Size
    mtdInteger.endian( & aObjHeader->mSize );

    // MBR
    mtdDouble.endian(&aObjHeader->mMbr.mMinX);
    mtdDouble.endian(&aObjHeader->mMbr.mMinY);
    mtdDouble.endian(&aObjHeader->mMbr.mMaxX);
    mtdDouble.endian(&aObjHeader->mMbr.mMaxY);
}

/***********************************************************************
 * Description :
 * Geometry 객체를 읽어서 Type에 맞게 Endian을 바꾼다.
 *
 * stdGeometryHeader * aGeom(InOut): Endian을 바꿀 Geometry 객체
 **********************************************************************/
IDE_RC stdPrimitive::cvtEndianGeom(stdGeometryHeader * aGeom)
{
//    if( !isValidType(sVal->mType, false) ) return;
    idBool sEquiEndian;
    
    IDE_TEST( stdUtils::isEquiEndian( aGeom, &sEquiEndian ) != IDE_SUCCESS );
    
    switch( stdUtils::getType(aGeom) )
    {
        case STD_NULL_TYPE:
        case STD_EMPTY_TYPE:
            cvtEndianHeader( (stdGeometryHeader*)aGeom);
            break;
            
        case STD_POINT_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdPoint2DExtType*)aGeom)->mSRID) );
        case STD_POINT_2D_TYPE:
            cvtEndianPoint2D( (stdPoint2DType*)aGeom);
            break;
            
        case STD_LINESTRING_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdLineString2DExtType*)aGeom)->mSRID) );
        case STD_LINESTRING_2D_TYPE:
            cvtEndianLineString2D( sEquiEndian, (stdLineString2DType*)aGeom);
            break;
            
        case STD_POLYGON_2D_EXT_TYPE :
            cvtEndianSRID( &(((stdPolygon2DExtType*)aGeom)->mSRID) );
        case STD_POLYGON_2D_TYPE :
            cvtEndianPolygon2D( sEquiEndian, (stdPolygon2DType*)aGeom);
            break;

        case STD_MULTIPOINT_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdMultiPoint2DExtType*)aGeom)->mSRID) );
        case STD_MULTIPOINT_2D_TYPE:
            cvtEndianMultiPoint2D( sEquiEndian, (stdMultiPoint2DType*)aGeom);
            break;

        case STD_MULTILINESTRING_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdMultiLineString2DExtType*)aGeom)->mSRID) );
        case STD_MULTILINESTRING_2D_TYPE:
            cvtEndianMultiLineString2D( sEquiEndian, (stdMultiLineString2DType*)aGeom);    
            break;

        case STD_MULTIPOLYGON_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdMultiPolygon2DExtType*)aGeom)->mSRID) );
        case STD_MULTIPOLYGON_2D_TYPE:
            cvtEndianMultiPolygon2D( sEquiEndian, (stdMultiPolygon2DType*)aGeom);    
            break;
            
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
            cvtEndianSRID( &(((stdGeoCollection2DExtType*)aGeom)->mSRID) );
        case STD_GEOCOLLECTION_2D_TYPE:
            cvtEndianGeoCollection2D( sEquiEndian, (stdGeoCollection2DType*)aGeom);
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *    Geometry 객체의 유효성을 검사한다.
 *
 **********************************************************************/
IDE_RC
stdPrimitive::validate( iduMemory*   aQmxMem,
                        const void * aValue,
                        UInt         aValueSize )
{
    stdGeometryHeader * sGeom = (stdGeometryHeader *)aValue;
    
    IDE_TEST_RAISE(sGeom == NULL, ERR_INVALID_NULL);
    
    IDE_TEST_RAISE(aValueSize < ID_SIZEOF(stdGeometryHeader),
                    ERR_INVALID_LENGTH );
    
    IDE_TEST_RAISE( stdUtils::isValidType(sGeom->mType, ID_TRUE) != ID_TRUE,
                    ERR_INVALID_TYPE);

    switch(sGeom->mType)
    {
        case STD_EMPTY_TYPE:
            break;            
        case STD_POINT_2D_EXT_TYPE:
        case STD_POINT_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validatePoint2D(
                    (stdPoint2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validateLineString2D(
                    (stdLineString2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validatePolygon2D(
                    aQmxMem, (stdPolygon2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validateMultiPoint2D(
                    (stdMultiPoint2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validateMultiLineString2D(
                    (stdMultiLineString2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validateMultiPolygon2D(
                    aQmxMem, (stdMultiPolygon2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
            IDE_TEST_RAISE(
                stdPrimitive::validateGeoCollection2D(
                    aQmxMem, (stdGeoCollection2DType*)sGeom, aValueSize)
                != IDE_SUCCESS, ERR_INVALID_VALUE);
            break;
        default:
            IDE_RAISE( ERR_INVALID_TYPE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NULL);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_VALIDATE_INVALID_VALUE));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_VALIDATE_INVALID_LENGTH));
    }
    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        //validateXX 함수에서 에러 코드가 지정된다.
        //IDE_SET(ideSetErrorCode(stERR_ABORT_VALIDATE_INVALID_VALUE));
    }
    IDE_EXCEPTION( ERR_INVALID_TYPE );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description :
 * stdPoint2DType 객체의 유효성을 검사한다.
 *
 * const void* aCol: 사용하지 않는다
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validatePoint2D( const void*      /*aCol*/,
                                      UInt             aSize )
{
//    stdGeometryHeader * sGeom = (stdGeometryHeader *) aCol;
    IDE_TEST_RAISE( ( aSize != STD_POINT2D_SIZE ) && ( aSize != STD_POINT2D_EXT_SIZE ),
                    err_invalid_size );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_size);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_PRECISION));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

/***********************************************************************
 * Description :
 * stdLineString2DType 객체의 유효성을 검사한다.
 * 서로 다른 점 2개 이상으로 구성된 선분을 만족해야 한다.
 *
 * const void* aCol(In): 객체
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validateLineString2D(
                    const void*      aCol,
                    UInt             aSize )
{
    stdLineString2DType* sLine = (stdLineString2DType*)aCol;
    stdPoint2D*          sPt = STD_FIRST_PT2D(sLine);
    UInt                 sMax = STD_N_POINTS(sLine);
    UChar              * sFence;

    IDE_TEST_RAISE( sMax < 2, err_line_point_count );

    sFence = (UChar *)aCol + aSize;

    if( sMax == 2 )
    {
        /* BUG-48557 */
        IDE_TEST_RAISE ( (UChar *)STD_LAST_PT2D(sLine) >= sFence, err_invalid_size );

        IDE_TEST_RAISE( stdUtils::isSamePoints2D(sPt, STD_LAST_PT2D(sLine))
                        == ID_TRUE,
                        err_same_points );
    }

    // anyway this is error!.
    IDE_TEST_RAISE( aSize != STD_LINE2D_SIZE + STD_PT2D_SIZE * sLine->mNumPoints,
                    err_invalid_size);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_size);    
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_PRECISION));
    }
    
    IDE_EXCEPTION(err_line_point_count);    
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_LINE_POINT_COUNT));        
    }
    
    IDE_EXCEPTION(err_same_points);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_LINE_POINT_SAME));
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stdPrimitive::validateComplexPolygon2D( iduMemory*        aQmxMem,
                                               stdPolygon2DType* aPolygon,
                                               ULong             aTotalPoint)
{
    UInt                i;
    UInt                sIndexSegTotal = 0;
    ULong               sReuseSegCount = 0;
    SInt                sIntersectStatus;
    Segment**           sIndexSeg;
    Segment*            sCurrSeg;
    Segment*            sCmpSeg;    
    Segment**           sReuseSeg;
    Segment**           sTempIndexSeg;    
    iduPriorityQueue    sPQueue;
    idBool              sOverflow = ID_FALSE;
    idBool              sUnderflow = ID_FALSE;
    Segment*            sCurrNext;
    Segment*            sCurrPrev;
    stdPoint2D          sInterResult[4];
    SInt                sInterCount;
    idBool              sIsTouch;
    stdPoint2D          sInterPoint;
 
    IDE_TEST( aQmxMem->alloc( aTotalPoint * ID_SIZEOF(Segment*),
                              (void**) & sIndexSeg )
              != IDE_SUCCESS);

    IDE_TEST( stdUtils::classfyPolygonChain( aQmxMem,
                                             aPolygon,
                                             0,   // Single polygon
                                             sIndexSeg,
                                             &sIndexSegTotal,
                                             NULL,  // no need to create RingSegList
                                             NULL,
                                             ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIndexSegTotal == 0, ERR_ABORT_SEGMENT_COUNT_IS_ZERO );

    IDE_TEST(sPQueue.initialize(aQmxMem,
                                sIndexSegTotal,
                                ID_SIZEOF(Segment*),
                                cmpSegment )
             != IDE_SUCCESS);
    
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment*) * sIndexSegTotal,
                              (void**)  &sReuseSeg )
              != IDE_SUCCESS);
    
    for ( i=0; i< sIndexSegTotal; i++ )
    {
        sPQueue.enqueue( sIndexSeg++, &sOverflow);
        IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
    }

    sIsTouch = ID_FALSE;    
    
    while(1)
    {
        sPQueue.dequeue( (void*) &sCurrSeg, &sUnderflow );

        // BUG-37504 codesonar uninitialized variable
        if ( sUnderflow == ID_TRUE )
        {
            break;
        }
        else 
        {
            /* Nothing to do */
        }

        sCurrNext = stdUtils::getNextSeg( sCurrSeg );
        sCurrPrev = stdUtils::getPrevSeg( sCurrSeg );

        sReuseSegCount = 0;
        sTempIndexSeg = sReuseSeg;  

        while(1)
        {      
            sPQueue.dequeue( (void*) &sCmpSeg, &sUnderflow );

            if ( sUnderflow == ID_TRUE )
            {
                break;
            }

            sReuseSeg[sReuseSegCount++] = sCmpSeg;
            
            if ( sCmpSeg->mStart.mX > sCurrSeg->mEnd.mX )
            {
                //잘 못 뽑은 놈은 재사용에 넣어야 한다.                
                break;                
            }
            
            do
            {
                /*
                  여기서 intersect와 방향에 대한 개념을 넣어 쳐낼수 있다.                  
                 */
                if ( ( sCurrNext != sCmpSeg ) && ( sCurrPrev != sCmpSeg ) )
                {
                    IDE_TEST( stdUtils::intersectCCW( sCurrSeg->mStart, 
                                                      sCurrSeg->mEnd,
                                                      sCmpSeg->mStart,
                                                      sCmpSeg->mEnd,
                                                      &sIntersectStatus,
                                                      &sInterCount,
                                                      sInterResult )
                              != IDE_SUCCESS );

                    switch( sIntersectStatus )
                    {
                        case ST_POINT_TOUCH:

                            if ( (sCurrSeg->mParent->mPolygonNum == sCmpSeg->mParent->mPolygonNum) &&
                                 (sCurrSeg->mParent->mRingNum    == sCmpSeg->mParent->mRingNum) )
                            {
                                IDE_RAISE( polygon_intersects );
                            }
                            else
                            {
                                // Nothing to do
                            }

                            if ( sIsTouch == ID_FALSE)
                            {
                                sIsTouch = ID_TRUE;
                                sInterPoint.mX = sInterResult[0].mX;
                                sInterPoint.mY = sInterResult[0].mY;
                            }
                            else
                            {
                                if ( stdUtils::isSamePoints2D( sInterResult, &sInterPoint) == ID_FALSE )
                                {
                                    IDE_RAISE( polygon_intersects );  
                                }   
                            }
                            break;

                        case ST_TOUCH:                        
                        case ST_INTERSECT:
                        case ST_SHARE:
                            IDE_RAISE( polygon_intersects );
                            break;
                        case ST_NOT_INTERSECT:
                            break;                        
                    }
                }
                else
                {
                    // Nothing to do 
                }

                sCmpSeg = sCmpSeg->mNext;

                if ( sCmpSeg == NULL )
                {
                    break;                    
                }
            
                // 끝까지 조사한다.

            }while( sCmpSeg->mStart.mX <= sCurrSeg->mEnd.mX );            
        }

        // 재사용을 정리 한다.
    
        
        for ( i =0; i < sReuseSegCount ; i++)
        {
            sPQueue.enqueue( sTempIndexSeg++, &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
            //Overflow 검사 
        }

        if ( sCurrSeg->mNext != NULL )
        {
            sPQueue.enqueue( &(sCurrSeg->mNext), &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( polygon_intersects );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_RING_BOUND_CROSS,
                                sCurrSeg->mParent->mRingNum,
                                sCmpSeg->mParent->mRingNum ));
    } 
    IDE_EXCEPTION( ERR_ABORT_SEGMENT_COUNT_IS_ZERO )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdPrimitive::validateComplexPolygon2D",
                                  "total segment count is zero" ));
    }
    IDE_EXCEPTION( ERR_ABORT_ENQUEUE_ERROR )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdPrimitive::validateComplexPolygon2D",
                                  "enqueue overflow" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stdPrimitive::validateComplexMultiPolygon2D( iduMemory*             aQmxMem,
                                                    stdMultiPolygon2DType* aMultiPolygon,
                                                    ULong                  aTotalPoint)
{
    stdPolygon2DType*   sPolygon;    
    UInt                i;
    UInt                sIndexSegTotal = 0;
    ULong               sReuseSegCount = 0;
    SInt                sIntersectStatus;
    Segment**           sIndexSeg;
    Segment*            sCurrSeg;
    Segment*            sCmpSeg;    
    Segment**           sReuseSeg;
    Segment**           sTempIndexSeg;    
    iduPriorityQueue    sPQueue;
    idBool              sOverflow = ID_FALSE;
    idBool              sUnderflow = ID_FALSE;
    UInt                sMax;
    Segment*            sCurrNext;
    Segment*            sCurrPrev;
    stdPoint2D          sInterResult[4];
    SInt                sInterCount;
    UInt                sIndex;
    stdTouchPointInfo   sLocalTouchInfo[ST_MAX_TOUCH_INFO_COUNT];
    stdTouchPointInfo*  sTouchInfo;   

    IDE_TEST( aQmxMem->alloc( aTotalPoint * ID_SIZEOF(Segment*),
                              (void**) & sIndexSeg )
              != IDE_SUCCESS);

    sReuseSeg = sIndexSeg;
    sMax = STD_N_OBJECTS(aMultiPolygon);
       
    sPolygon = STD_FIRST_POLY2D(aMultiPolygon);

    for( i = 0; i < sMax; i++)
    {
        IDE_TEST( stdUtils::classfyPolygonChain( aQmxMem,
                                                 sPolygon,
                                                 i,
                                                 sIndexSeg,
                                                 &sIndexSegTotal,
                                                 NULL,
                                                 NULL, 
                                                 ID_TRUE )
                  != IDE_SUCCESS );
        
        sPolygon = STD_NEXT_POLY2D( sPolygon );
    }
    
    IDE_TEST_RAISE( sIndexSegTotal == 0, ERR_ABORT_SEGMENT_COUNT_IS_ZERO );

    IDE_TEST(sPQueue.initialize(aQmxMem,
                                sIndexSegTotal,
                                ID_SIZEOF(Segment*),
                                cmpSegment )
             != IDE_SUCCESS);
    
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment*) * sIndexSegTotal,
                              (void**)  &sReuseSeg )
              != IDE_SUCCESS);

    if ( sMax > ST_MAX_TOUCH_INFO_COUNT )
    {        
        IDE_TEST( aQmxMem->alloc( sMax * ID_SIZEOF(stdTouchPointInfo),
                                  (void**) & sTouchInfo )
                  != IDE_SUCCESS);
    }
    else
    {
        sTouchInfo = sLocalTouchInfo;        
    }

    for ( i = 0; i < sMax; i++ )
    {
        sTouchInfo[i].mIsTouch = ID_FALSE;        
    }
        
    for ( i = 0; i< sIndexSegTotal; i++ )
    {
        sPQueue.enqueue( sIndexSeg++, &sOverflow);
        IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
    }
    
    while(1)
    {
        sPQueue.dequeue( (void*) &sCurrSeg, &sUnderflow );

        // BUG-37504 codesonar uninitialized variable
        if ( sUnderflow == ID_TRUE )
        {    
            break;
        } 
        else
        {
            /* Nothing to do */
        }        

        sCurrNext = stdUtils::getNextSeg(sCurrSeg);
        sCurrPrev = stdUtils::getPrevSeg(sCurrSeg);
        
        sTempIndexSeg  = sReuseSeg;
        sReuseSegCount = 0;        

        while(1)
        {           
            sPQueue.dequeue( (void*) &sCmpSeg, &sUnderflow );

            if ( sUnderflow == ID_TRUE )
            {
                break;
            }

            sReuseSeg[sReuseSegCount++] = sCmpSeg;
            
            if ( sCmpSeg->mStart.mX > sCurrSeg->mEnd.mX )
            {
                //잘 못 뽑은 놈은 재사용에 넣어야 한다.                                      
                break;                
            }
            
            do
            {
                /*
                  여기서 intersect와 방향에 대한 개념을 넣어 쳐낼수 있다.                  
                */
                if ( (sCurrNext != sCmpSeg) && ( sCurrPrev != sCmpSeg ) )
                {
                    IDE_TEST( stdUtils::intersectCCW( sCurrSeg->mStart,
                                                      sCurrSeg->mEnd,
                                                      sCmpSeg->mStart,
                                                      sCmpSeg->mEnd,
                                                      &sIntersectStatus,
                                                      &sInterCount,
                                                      sInterResult)
                              != IDE_SUCCESS );

                    switch( sIntersectStatus )
                    {
                        case ST_POINT_TOUCH:
                            
                            if ( (sCurrSeg->mParent->mPolygonNum == sCmpSeg->mParent->mPolygonNum) &&
                                 (sCurrSeg->mParent->mRingNum    == sCmpSeg->mParent->mRingNum) )
                            {
                                IDE_RAISE( polygon_intersects ); 
                            }
                            else
                            {
                                // Nothing to do 
                            }

                            if ( sCurrSeg->mParent->mPolygonNum == sCmpSeg->mParent->mPolygonNum )
                            {
                                sIndex = sCurrSeg->mParent->mPolygonNum;
                                
                                if ( sTouchInfo[sIndex].mIsTouch == ID_FALSE )                             
                                {
                                    sTouchInfo[sIndex].mIsTouch = ID_TRUE;
                                    sTouchInfo[sIndex].mCoord   = sInterResult[0];                                                                 
                                }
                                else
                                {
                                    if ( stdUtils::isSamePoints2D( &sTouchInfo[sIndex].mCoord, sInterResult) == ID_FALSE )
                                    {
                                        IDE_RAISE( polygon_intersects );   
                                    }
                                    else
                                    {
                                        //nothing to do
                                    }
                                }
                            }
                            else
                            {
                                //nothing to do 
                            }
                        break;

                        case ST_TOUCH:                        
                        case ST_INTERSECT:
                        case ST_SHARE:
                            IDE_RAISE( polygon_intersects );
                            break;
                        case ST_NOT_INTERSECT:
                            break;                        
                    }
                }
                else
                {
                    // Nothing to do 
                }
                
                sCmpSeg = sCmpSeg->mNext;

                if ( sCmpSeg == NULL )
                {
                    break;                    
                }
            
                // 끝까지 조사한다.

            }while( sCmpSeg->mStart.mX <= sCurrSeg->mEnd.mX );            
        }

        // 재사용을 정리 한다.
        
        for ( i =0; i < sReuseSegCount ; i++)
        {
            sPQueue.enqueue( sTempIndexSeg++, &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
        }

        if ( sCurrSeg->mNext != NULL )
        {
            sPQueue.enqueue( &(sCurrSeg->mNext), &sOverflow);
            IDE_TEST_RAISE( sOverflow == ID_TRUE, ERR_ABORT_ENQUEUE_ERROR );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( polygon_intersects );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_POLYGON_INTERSECTS,
                                sCurrSeg->mParent->mPolygonNum,
                                sCurrSeg->mParent->mRingNum,
                                sCmpSeg->mParent->mPolygonNum,
                                sCmpSeg->mParent->mRingNum));
    }
    IDE_EXCEPTION( ERR_ABORT_SEGMENT_COUNT_IS_ZERO )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdPrimitive::validateComplexPolygon2D",
                                  "total segment count is zero" ));
    }
    IDE_EXCEPTION( ERR_ABORT_ENQUEUE_ERROR )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdPrimitive::validateComplexPolygon2D",
                                  "enqueue overflow" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * stdPolygon2DType 객체의 유효성을 검사한다.
 * 링은 3각형 이상이 되어야 하며 가장 큰 링이 첫 번째 링(Exterior Ring)이 되도록 정렬한다.
 * 모든 링은 교차하지 않아야 하며 내부링 안쪽에 링이 올 수 없다.
 *
 * const void* aCol(In): 객체
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validatePolygon2D(
                    iduMemory*       aQmxMem,
                    const void*      aCol,
                    UInt             aSize )
{
    stdPolygon2DType*   sPolygon = (stdPolygon2DType*)aCol;
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPt;
    UInt                sTotalSize = STD_POLY2D_SIZE;
    UInt                i, sMaxR, sMax;
    ULong               sTotalPoint = 0;
    UChar             * sFence;

    sRing = STD_FIRST_RN2D(sPolygon);
    sMaxR = STD_N_RINGS(sPolygon);
    sFence = (UChar *)aCol + aSize;

    if ( sMaxR > 0 )
    {
        for( i = 0; i < sMaxR; i++)
        {
            sMax = STD_N_POINTS(sRing);
            sTotalPoint += sMax;

            if (sMax < 4)
            {
                IDE_RAISE( ring_point_count_less_than_4 );   // digon 이상이 되어야한다.
            }
            else
            {
                sPt = STD_FIRST_PT2D(sRing);

                /* BUG-48557 */
                IDE_TEST_RAISE ( (UChar *)STD_LAST_PT2D(sRing) >= sFence, size_error );

                if (stdUtils::isSamePoints2D(sPt, STD_LAST_PT2D(sRing)) == ID_FALSE)
                {
                    IDE_RAISE(ring_start_end_diff);
                }
            }

            // 파싱에서 구해 놓은 orientaion을 이용한 검증
        
            sTotalSize += STD_RN2D_SIZE + STD_PT2D_SIZE * sMax;
        
            IDE_TEST_RAISE(aSize < sTotalSize, size_error);

            sRing = STD_NEXT_RN2D(sRing);
        }
    }
    else
    {
        // nothing to do 
    }

    IDE_TEST_RAISE( aSize != sTotalSize, size_error );

    if ( sMaxR > 0 )
    {
        // invalid인 경우 error 리턴
        IDE_TEST( validateComplexPolygon2D( aQmxMem,
                                            sPolygon,
                                            sTotalPoint)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothingo to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ring_point_count_less_than_4);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_RING_POINT_COUNT_LESS_THAN_4, i));
    }
    IDE_EXCEPTION(ring_start_end_diff);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_CLOSED_RING, i));
    }
    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_SIZE));
    }
        
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * stdMultiPoint2DType 객체의 유효성을 검사한다.
 *
 * const void* aCol(In): 객체
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validateMultiPoint2D(
                    const void*      aCol,
                    UInt             aSize )
{
    stdMultiPoint2DType* sMultiPoint = (stdMultiPoint2DType*)aCol;

    IDE_TEST_RAISE(aSize != STD_MPOINT2D_SIZE + STD_POINT2D_SIZE*STD_N_OBJECTS(sMultiPoint),
                   size_error);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_SIZE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * stdMultiLineString2DType 객체의 유효성을 검사한다.
 * 각각의 stdLineString2DType 객체의 유효성을 만족해야한다.
 *
 * const void* aCol(In): 객체
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validateMultiLineString2D(
                    const void*      aCol,
                    UInt             aSize )
{
    stdMultiLineString2DType* sMultiLine = (stdMultiLineString2DType*)aCol;
    stdLineString2DType*      sLine;
    UInt                      sTotalSize = STD_MLINE2D_SIZE;
    UInt                      i, sMaxO, sMax;
    UChar                   * sFence;

    sLine = STD_FIRST_LINE2D(sMultiLine);
    sMaxO = STD_N_OBJECTS(sMultiLine);
    sFence = (UChar *)aCol + aSize;

    for( i = 0; i < sMaxO; i++)
    {
        sMax = STD_N_POINTS(sLine);
        IDE_TEST_RAISE( sMax < 2 , line_point_count_error);

        if( sMax == 2 )
        {
            IDE_TEST_RAISE( (UChar *)STD_LAST_PT2D(sLine) >= sFence, size_error );

            IDE_TEST_RAISE(stdUtils::isSamePoints2D(STD_FIRST_PT2D(sLine), 
                                                    STD_LAST_PT2D(sLine)) == ID_TRUE,
                           line_point_same);
        }

        sTotalSize += STD_LINE2D_SIZE + STD_PT2D_SIZE*sMax;

        IDE_TEST_RAISE(aSize < sTotalSize, size_error);
        sLine = STD_NEXT_LINE2D(sLine);
    }

    IDE_TEST_RAISE( aSize != sTotalSize, size_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION(line_point_count_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_LINE_POINT_COUNT));
    }

    IDE_EXCEPTION(line_point_same);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_LINE_POINT_SAME));
    }

    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_SIZE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * stdMultiPolygon2DType 객체의 유효성을 검사한다.
 * 각각의 stdPolygon2DType 객체의 유효성을 만족해야한다.
 *
 * const void* aCol(In): 객체
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validateMultiPolygon2D(
                    iduMemory*       aQmxMem,
                    const void*      aCol,
                    UInt             aSize )
{
    stdMultiPolygon2DType* sMultiPolygon = (stdMultiPolygon2DType*)aCol;
    stdPolygon2DType*      sPolygon;
    stdLinearRing2D*       sRing;
    stdPoint2D*            sPt;
    UInt                   sTotalSize = STD_MPOLY2D_SIZE;
    UInt                   i, j, sMax, sMaxR, sMaxP;
    ULong                  sTotalPoint    = 0;
    UChar                * sFence;

    sPolygon = STD_FIRST_POLY2D(sMultiPolygon);
    sMax = STD_N_OBJECTS(sMultiPolygon);
    sFence = (UChar *)aCol + aSize;

    if ( sMax > 0 )
    {
        for( i = 0; i < sMax; i++)
        {
            sRing = STD_FIRST_RN2D(sPolygon);
            sMaxR = STD_N_RINGS(sPolygon);

            for( j = 0; j < sMaxR; j++)
            {
                sMaxP = STD_N_POINTS(sRing);
                sTotalPoint += sMaxP;
    
                if (sMaxP < 4)
                {
                    IDE_RAISE( ring_point_count_less_than_4 );   // digon 이상이 되어야한다.
                }
                else
                {
                    sPt = STD_FIRST_PT2D(sRing);

                    /* BUG-48557 */
                    IDE_TEST_RAISE ( (UChar *)STD_LAST_PT2D(sRing) >= sFence, size_error );

                    if (stdUtils::isSamePoints2D(sPt, STD_LAST_PT2D(sRing)) == ID_FALSE)
                    {
                        IDE_RAISE(ring_start_end_diff);
                    }
                }

                sRing = STD_NEXT_RN2D(sRing);
            }
        
            sTotalSize += STD_GEOM_SIZE(sPolygon);

            IDE_TEST_RAISE(aSize < sTotalSize, size_error);
        
            sPolygon = STD_NEXT_POLY2D(sPolygon);
        }
    }
    else
    {
        // nothing to do 
    }
    

    IDE_TEST_RAISE( aSize != sTotalSize, size_error );

    if ( sMax > 0 )
    {
        IDE_TEST( validateComplexMultiPolygon2D( aQmxMem,
                                                 sMultiPolygon,
                                                 sTotalPoint)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ring_point_count_less_than_4);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_RING_POINT_COUNT_LESS_THAN_4, i));
    }
    IDE_EXCEPTION(ring_start_end_diff);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_CLOSED_RING, i));
    }
    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_SIZE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * stdGeoCollection2DType 객체의 유효성을 검사한다.
 * 각각의 내부 객체는 해당 타입의 유효성을 만족해야한다.
 *
 * const void* aCol(In): 객체
 * UInt aSize(In): 객체의 크기
 **********************************************************************/
IDE_RC stdPrimitive::validateGeoCollection2D(
                    iduMemory*       aQmxMem,
                    const void*      aCol,
                    UInt             aSize )
{
    stdGeoCollection2DType* sCollection = (stdGeoCollection2DType*)aCol;
    stdGeometryHeader*      sObj;
    UInt                    sTotalSize = STD_COLL2D_SIZE;
    UInt                    sNumGeometries = 0;
    UInt                    i, sMax;

    sObj = (stdGeometryHeader*)STD_FIRST_COLL2D(sCollection);
    sMax = STD_N_GEOMS(sCollection);
    
    for( i = 0; i < sMax; i++)
    {    
        switch(sObj->mType)
        {
        case STD_POINT_2D_EXT_TYPE:
        case STD_POINT_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validatePoint2D(
                          (stdPoint2DType*)sObj, STD_GEOM_SIZE(sObj)) );
            break;
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validateLineString2D(
                          (stdLineString2DType*)sObj, STD_GEOM_SIZE(sObj)));
            break;
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validatePolygon2D(
                          aQmxMem, (stdPolygon2DType*)sObj, STD_GEOM_SIZE(sObj)));
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validateMultiPoint2D(
                          (stdMultiPoint2DType*)sObj, STD_GEOM_SIZE(sObj)));
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validateMultiLineString2D(
                          (stdMultiLineString2DType*)sObj, STD_GEOM_SIZE(sObj)));
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validateMultiPolygon2D(
                          aQmxMem, (stdMultiPolygon2DType*)sObj, STD_GEOM_SIZE(sObj)));
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
            IDE_TEST( IDE_SUCCESS != validateGeoCollection2D(
                          aQmxMem, (stdGeoCollection2DType*)sObj, STD_GEOM_SIZE(sObj)));
            break;
        default:
            IDE_RAISE(invalid_object_type);
        }
    
        sTotalSize += STD_GEOM_SIZE(sObj);
        sNumGeometries++;
        sObj = (stdGeometryHeader*)STD_NEXT_GEOM(sObj);
    }
    IDE_TEST_RAISE( sNumGeometries != sMax, count_error );
    IDE_TEST_RAISE( aSize != sTotalSize, size_error );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_SIZE));
    }

    IDE_EXCEPTION(invalid_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_OBJECT_IN_GEOMCOLLECTION,
                                sObj->mType, i));
    }
    
    IDE_EXCEPTION(count_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_INTEGRITY_VIOLATION));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/***********************************************************************
 * Description :
 * stdMultiPoint2DType 객체가 Simple하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하지 않으면 Simple 존재하면 non simple이다.
 *
 * const stdMultiPoint2DType* aMPoint(In): 객체
 **********************************************************************/
idBool stdPrimitive::isSimpleMPoint2D( const stdMultiPoint2DType* aMPoint )
{
    stdPoint2DType *sPt = STD_FIRST_POINT2D(aMPoint);
    UInt            i, j, sMax;

    sMax = STD_N_OBJECTS(aMPoint);
    if( sMax > 1)
    {
        for( i = 0; i < sMax - 1; i++ )
        {
            for( j = i+1; j < sMax; j++ )
            {
                if(stdUtils::isSamePoints2D(&STD_NEXTN_POINT2D(sPt,i)->mPoint,
                                            &STD_NEXTN_POINT2D(sPt,j)->mPoint) == ID_TRUE)
                {
                    IDE_RAISE( not_simple );
                }
            }
        }
    }
    
    return ID_TRUE;

    IDE_EXCEPTION( not_simple );
    {
    }
    IDE_EXCEPTION_END;
    
    return ID_FALSE;
}

/***********************************************************************
 * Description :
 * stdLineString2DType 객체가 Simple하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하지 않으면 Simple 존재하면 non simple이다.
 *
 * const stdLineString2DType* aLine(In): 객체
 **********************************************************************/
idBool stdPrimitive::isSimpleLine2D( const stdLineString2DType* aLine )
{
    stdPoint2D* sPt1, *sPt2;
    stdPoint2D* sPt3, *sPt4;
    stdPoint2D* sPtFenceE;
    stdPoint2D  sCrPt;
    SInt        i, j;
    SInt        sNumLines;
    SInt        sNumPoints;
    idBool      sIsEndPt1, sIsEndPt4;;
    SDouble     sDistanceSq;

    // Initialize
    sNumPoints = STD_N_POINTS(aLine);
    sPtFenceE  = STD_LAST_PT2D(aLine) + 1;
    sPt1       = STD_FIRST_PT2D(aLine);

    // Calc NumLines
    sNumLines  = 0;
    for( i=0; i<sNumPoints-1; i++ )
    {
        if( stdUtils::isSamePoints2D( STD_NEXTN_PT2D(sPt1,i),
                                      STD_NEXTN_PT2D(sPt1,i+1) ) ==ID_FALSE )
        {
            sNumLines++;
        }
    }

    for( i=0; i<sNumLines-1; i++ )
    {
        if( i==0 )
        {
            sIsEndPt1 = ID_TRUE;
        }
        else
        {
            sIsEndPt1 = ID_FALSE;
        }
        
        sPt2 = stdUtils::findNextPointInLine2D( sPt1, sPtFenceE );
        IDE_TEST_RAISE(sPt2 == NULL, ERR_INVALID_POINT);
        
        // 다음 인접 라인
        sPt3  = sPt2;
        sPt4  = stdUtils::findNextPointInLine2D( sPt3, sPtFenceE );
        IDE_TEST_RAISE(sPt4 == NULL, ERR_INVALID_POINT);
        
        sDistanceSq = stdUtils::getDistanceSqLineToPoint2D( sPt1, sPt2, sPt4, &sCrPt );
        if( sDistanceSq < STD_MATH_TOL_SQ )
        {
            // 둘 다 ID_TRUE인 경우, non-simple
            IDE_TEST( (stdUtils::checkBetween2D( sPt1, sPt2, sPt4 )==ID_TRUE) ||
                      (stdUtils::checkBetween2D( sPt3, sPt4, sPt1 )==ID_TRUE) );
        }

        // 비인접 라인
        sPt3 = sPt4;
        for( j=i+2; j<sNumLines; j++ )
        {
            if( j==(sNumLines-1) )
            {
                sIsEndPt4 = ID_TRUE;
            }
            else
            {
                sIsEndPt4 = ID_FALSE;
            }
            
            // 다음 라인
            sPt4 = stdUtils::findNextPointInLine2D( sPt3, sPtFenceE );
            IDE_TEST_RAISE(sPt4 == NULL, ERR_INVALID_POINT);

            // 교차점이 존재하면, non-simple이므로 ID_FALSE를 리턴한다.
            IDE_TEST( stdUtils::isIntersectsLineToLine2D( sPt1, sPt2, sIsEndPt1, ID_FALSE,
                                                          sPt3, sPt4, ID_FALSE,  sIsEndPt4 )
                      == ID_TRUE );
            sPt3 = sPt4;
        }

        sPt1 = sPt2;
    }

    return ID_TRUE;

    IDE_EXCEPTION( ERR_INVALID_POINT );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_INTEGRITY_VIOLATION));
    }
    IDE_EXCEPTION_END;

    return ID_FALSE;
}

/***********************************************************************
 * Description :
 * stdPolygon2DType 객체가 Simple하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하지 않으면 Simple 존재하면 non simple이다.
 *
 * const stdPolygon2DType* aPoly(In): 객체
 **********************************************************************/
idBool stdPrimitive::isSimplePoly2D( const stdPolygon2DType * aPoly )
{
    stdLinearRing2D*    sRing = STD_FIRST_RN2D(aPoly);
    stdPoint2D*         sPt = STD_FIRST_PT2D(sRing);
    SInt                i, j, sMaxI, sMaxJ;

    sMaxI = STD_N_POINTS(sRing)-2;
    sMaxJ = STD_N_POINTS(sRing)-1;

    for( i = 0; i < sMaxI; i++ )
    {
        for( j = i+1; j < sMaxJ; j++ )
        {
            if(stdUtils::intersectI2D(
                STD_NEXTN_PT2D(sPt,i),
                STD_NEXTN_PT2D(sPt,i+1),
                STD_NEXTN_PT2D(sPt,j),
                STD_NEXTN_PT2D(sPt,j+1))==ID_TRUE)
            {
                return ID_FALSE;
            }
        }
    }
    return ID_TRUE;
}


/***********************************************************************
 * Description :
 * stdMultiLineString2DType 객체가 Simple하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하지 않으면 Simple 존재하면 non simple이다.
 *
 * const stdMultiLineString2DType* aMLine(In): 객체
 **********************************************************************/
idBool stdPrimitive::isSimpleMLine2D( const stdMultiLineString2DType* aMLine )
{
    stdLineString2DType*    sLine = STD_FIRST_LINE2D(aMLine);
    stdLineString2DType*    sLineCP;
    UInt                    i, j, sMaxO = STD_N_OBJECTS(aMLine);

    // 하위 객체 자체 테스트 
    for( i = 0; i < sMaxO; i++ )
    {
        if(isSimpleLine2D(sLine)==ID_FALSE)
        {
            IDE_RAISE( not_simple );
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }

    // 하위 객체간  테스트 
    sLine = STD_FIRST_LINE2D(aMLine);
    for( i = 0; i < sMaxO-1; i++ )
    {
        sLineCP = sLine;
        for( j = i+1; j<sMaxO; j++ )
        {
            sLineCP = STD_NEXT_LINE2D(sLineCP);
            if( stdUtils::intersectLineToLine2D( sLine, sLineCP )==ID_TRUE )
            {
                IDE_RAISE( not_simple );
            }
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
        
    return ID_TRUE;
    
    IDE_EXCEPTION( not_simple );
    {
    }
    IDE_EXCEPTION_END;
    
    return ID_FALSE;
}


/***********************************************************************
 * Description :
 * stdMultiPolygon2DType 객체가 Simple하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하지 않으면 Simple 존재하면 non simple이다.
 *
 * const stdMultiPolygon2DType* aMPoly(In): 객체
 **********************************************************************/
idBool stdPrimitive::isSimpleMPoly2D( const stdMultiPolygon2DType * aMPoly )
{
    stdPolygon2DType* sPoly = STD_FIRST_POLY2D(aMPoly);
    UInt              i, sMaxIdx, sMaxO = STD_N_OBJECTS(aMPoly);
    
    sMaxIdx = sMaxO;
    for( i = 0; i < sMaxIdx; i++ )
    {
        if(isSimplePoly2D(sPoly)==ID_FALSE)
        {
            return ID_FALSE;
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }
    return ID_TRUE;
}


/***********************************************************************
 * Description :
 * stdGeoCollection2DType 객체가 Simple하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하지 않으면 Simple 존재하면 non simple이다.
 *
 * const stdGeoCollection2DType* aCollect(In): 객체
 **********************************************************************/
idBool stdPrimitive::isSimpleCollect2D( const stdGeoCollection2DType* aCollect )
{
    stdGeometryHeader *sGeom = (stdGeometryHeader*)STD_FIRST_COLL2D(aCollect);
    UInt               i, sMax = STD_N_GEOMS(aCollect);
    idBool             sRet;

    sRet = ID_TRUE;
    for( i = 0; i < sMax; i++ )
    {
        switch(sGeom->mType)
        {
        case STD_EMPTY_TYPE :
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            sRet = isSimpleLine2D((stdLineString2DType*)sGeom);
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
            sRet = isSimpleMLine2D((stdMultiLineString2DType*)sGeom);
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
            sRet = isSimpleMPoint2D((stdMultiPoint2DType*)sGeom);
            break;
        default :
            sRet = ID_FALSE;
            break;
        }
        if( sRet==ID_FALSE )
        {
            IDE_RAISE( not_simple );
        }
        sGeom = (stdGeometryHeader *)STD_NEXT_GEOM(sGeom);
    }
    return ID_TRUE;
    
    IDE_EXCEPTION( not_simple );
    {
    }
    IDE_EXCEPTION_END;
    
    return ID_FALSE;
}


/***********************************************************************
 * Description :
 * 폴리곤 내부의 stdLinearRing2D가 유효하면 ID_TRUE 아니면 ID_FALSE 리턴
 * 겹치는 점이 존재하면 유효하지 않다.
 *
 * const stdLinearRing2D* aRing(In): 링
 **********************************************************************/
IDE_RC stdPrimitive::isValidRing2D( stdLinearRing2D* aRing )
{
    stdPoint2D*     sPtS = STD_FIRST_PT2D(aRing);
    stdPoint2D*     sPt1;
    stdPoint2D*     sPt2;
    stdPoint2D*     sPt3;
    stdPoint2D*     sPt4;
    stdPoint2D*     sPtT;
    UInt            i, j, sMaxNumLines = 0;
    UInt            sNumPoints;
    UInt            sPos1, sPos2, sPos3, sPos4, sPosT;

    sNumPoints = STD_N_POINTS(aRing);
    
    for( i = 0; i < sNumPoints-1; i++ )
    {
        if( stdUtils::isSamePoints2D(
            STD_NEXTN_PT2D(sPtS,i), 
            STD_NEXTN_PT2D(sPtS,i+1)) == ID_FALSE )
        {
            sMaxNumLines++;
        }
    }
    // 처음과 마지막점 비교
    if( stdUtils::isSamePoints2D(
        STD_NEXTN_PT2D(sPtS,0), 
        STD_NEXTN_PT2D(sPtS,sNumPoints-1)) == ID_FALSE )
    {
        sMaxNumLines++;
    }

    IDE_TEST_RAISE( sMaxNumLines < 3 , ring_linecount_error);

    sPos1    = 0;

    if( sMaxNumLines > 3 )
    {
        // 중간의 크로스 라인 교차 여부 검사 : Shape X
        sPt2  = sPtS;
        sPos2 = 0;
        for( i = 0; i < sMaxNumLines-2; i++ )
        {
            sPt1  = sPt2;
            sPos1 = sPos2;
            sPt2  = stdUtils::findNextPointInRing2D( sPt1,
                                                     sPos1,
                                                     sNumPoints,
                                                     &sPos2 );
            sPtT  = sPt2;
            sPosT = sPos2;
            
            // 중간라인들 검사( 인접라인은 테스트하지 않는다. )
            for( j = i+2; j < sMaxNumLines; j++ )
            {
                sPt3 = stdUtils::findNextPointInRing2D( sPtT,
                                                        sPosT,
                                                        sNumPoints,
                                                        &sPos3 );
                sPt4 = stdUtils::findNextPointInRing2D( sPt3,
                                                        sPos3,
                                                        sNumPoints,
                                                        &sPos4 );
                if( (j==(sMaxNumLines-1)) &&
                    stdUtils::isSamePoints2D( sPt1, sPt4 )==ID_TRUE )
                {
                    break;
                }
                if ( stdUtils::intersect2D(sPt1,sPt2,sPt3,sPt4) == ID_TRUE )
                {
                    IDE_RAISE(ring_line_cross);
                }
                sPtT  = sPt3;
                sPosT = sPos3;
            }
            
        }
    }
    
    // 다시 갔다가 오는 라인 검사  : Zero Area발생여부 검사
    sPt2  = sPtS;
    sPos2 = 0;
    for( i = 0; i < sMaxNumLines; i++ )
    {
        sPt1 = sPt2;
        sPos1= sPos2;
        sPt2 = stdUtils::findNextPointInRing2D( sPt1,
                                                sPos1,
                                                sNumPoints,
                                               &sPos2 );
        sPt3 = stdUtils::findNextPointInRing2D( sPt2,
                                                sPos2,
                                                sNumPoints,
                                               &sPos3 );
        if ( stdUtils::isReturned2D(sPt1, sPt2, sPt3) == ID_TRUE)
        {
            IDE_RAISE(has_zero_area);
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ring_linecount_error );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_RING_LINE_COUNT));
    }
    IDE_EXCEPTION( has_zero_area );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_RING_ZERO_AREA));
    }
    IDE_EXCEPTION( ring_line_cross );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_RING_LINE_CROSS));
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description :
 * stdLineString2DType의 시작점과 끝점이 같으면 ID_TRUE 아니면 ID_FALSE 리턴
 * 시작점과 끝점이 같으면 닫혀있다.
 *
 * const stdLineString2DType* aLine(In): 판별할 객체
 **********************************************************************/
idBool stdPrimitive::isClosedLine2D( const stdLineString2DType* aLine )
{
    SInt        sMax = STD_N_POINTS(aLine);

    if((sMax > 2) && (stdUtils::isSamePoints2D(
        STD_FIRST_PT2D(aLine), STD_LAST_PT2D(aLine))==ID_TRUE))
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/***********************************************************************
 * Description :
 * stdLineString2DType의 Boundary인 양 끝 점을 stdMultiPoint2DType 객체로 리턴
 *
 * const stdLineString2DType* aLine(In): Boundary를 찾을 객체
 * stdGeometryHeader* aGeom(Out): Boundary 객체
 **********************************************************************/
IDE_RC stdPrimitive::getBoundaryLine2D(
                        const stdLineString2DType*      aLine, 
                        stdGeometryHeader*              aGeom,
                        UInt                            aFence )  // Fix Bug -25110
{
    UInt        sMax = STD_N_POINTS(aLine);

    if(sMax <= 1)
    {
        stdUtils::makeEmpty(aGeom);
        return IDE_FAILURE;
    }

    stdMultiPoint2DType* sMPointDst = (stdMultiPoint2DType*)aGeom;
    stdPoint2DType*      sPtDst = STD_FIRST_POINT2D(sMPointDst);
    stdPoint2D*          sPt = STD_FIRST_PT2D(aLine);

    IDE_TEST_RAISE( STD_MPOINT2D_SIZE + STD_POINT2D_SIZE * 2  > aFence, size_error);
    stdUtils::setType((stdGeometryHeader*)sMPointDst, STD_MULTIPOINT_2D_TYPE);
    sMPointDst->mSize = STD_MPOINT2D_SIZE + STD_POINT2D_SIZE*2;
    sMPointDst->mNumObjects = 2;
    
    stdUtils::setType((stdGeometryHeader*)sPtDst, STD_POINT_2D_TYPE);
    sPtDst->mSize = STD_POINT2D_SIZE;
    idlOS::memcpy(&sPtDst->mPoint, sPt, STD_PT2D_SIZE);
    IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sMPointDst, sPt)
              != IDE_SUCCESS );
    // PR-18113
    mtdDouble.null( NULL, &sPtDst->mMbr.mMaxX );
    mtdDouble.null( NULL, &sPtDst->mMbr.mMaxY );
    IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sPtDst, sPt)
              != IDE_SUCCESS );
    
    sPt += sMax-1;
    sPtDst = STD_NEXT_POINT2D(sPtDst);

    stdUtils::setType((stdGeometryHeader*)sPtDst, STD_POINT_2D_TYPE);
    sPtDst->mSize = STD_POINT2D_SIZE;
    idlOS::memcpy(&sPtDst->mPoint, sPt, STD_PT2D_SIZE);
    IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sMPointDst, sPt)
              != IDE_SUCCESS );
    // PR-18113
    mtdDouble.null( NULL, &sPtDst->mMbr.mMaxX );
    mtdDouble.null( NULL, &sPtDst->mMbr.mMaxY );
    IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sPtDst, sPt)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_BUFFER_OVERFLOW ));
    }  

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description :
 * stdMultiLineString2DType의 내부 stdLineString2DType들의 Boundary를
 * stdMultiPoint2DType 객체로 리턴
 *
 * const stdMultiLineString2DType* aMLine(In): Boundary를 찾을 객체
 * stdGeometryHeader* aGeom(Out): Boundary 객체
 **********************************************************************/
IDE_RC stdPrimitive::getBoundaryMLine2D(
    const stdMultiLineString2DType*     aMLine, 
    stdGeometryHeader*                  aGeom,
    UInt                                aFence )  // BUG -25110
{
    stdMultiPoint2DType*    sMPointDst = (stdMultiPoint2DType*)aGeom;
    stdPoint2DType*         sPtDst = STD_FIRST_POINT2D(sMPointDst);
    stdLineString2DType*    sLine = STD_FIRST_LINE2D(aMLine);
    stdLineString2DType*    sLineCP;
    stdPoint2D*             sPt;
    SInt                    sDupCount;

    idBool bFound;
    SInt i, j, sMaxI, sMaxJ;
    sMaxI = STD_N_OBJECTS(aMLine);
    sMaxJ = STD_N_OBJECTS(aMLine);

    stdUtils::setType((stdGeometryHeader*)sMPointDst, STD_MULTIPOINT_2D_TYPE);
    sMPointDst->mSize = STD_MPOINT2D_SIZE;
    sMPointDst->mNumObjects = 0;    
    for( i = 0; i < sMaxI; i++ )
    {
        if(isClosedLine2D( sLine )==ID_TRUE)
        {
            sLine = STD_NEXT_LINE2D(sLine);
            continue;
        }        
        // Start Point
        sPt = STD_FIRST_PT2D(sLine);        
        bFound = ID_FALSE;
        // BUG-16420
        sLineCP = STD_FIRST_LINE2D(aMLine);
        sDupCount = 0;
        for( j = 0; j < sMaxJ; j++ )
        {
            if( (i==j) || (isClosedLine2D( sLineCP )==ID_TRUE) )
            {
                sLineCP = STD_NEXT_LINE2D(sLineCP);
                continue;
            }
            if(stdUtils::isEndPoint2D(sPt,sLineCP)==ID_TRUE)
            {
                if( i>j )
                {
                    bFound = ID_FALSE;            // BUG-16717

                    break;
                }
                else
                {
                    bFound = ID_TRUE;
                    sDupCount++;
                }
            }
            sLineCP = STD_NEXT_LINE2D(sLineCP);
        }
        if( (bFound==ID_TRUE) && ((sDupCount%2)==1) )
        {
            bFound = ID_FALSE;
        }
        
        if(bFound==ID_FALSE)
        {
            IDE_TEST_RAISE(sMPointDst->mSize + STD_POINT2D_SIZE > aFence, size_error);
            stdUtils::setType((stdGeometryHeader*)sPtDst, STD_POINT_2D_TYPE);
            sPtDst->mSize = STD_POINT2D_SIZE;
            idlOS::memcpy(&sPtDst->mPoint, sPt, STD_PT2D_SIZE);
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sMPointDst, sPt)
                      != IDE_SUCCESS );
            // PR-18113
            mtdDouble.null( NULL, &sPtDst->mMbr.mMaxX );
            mtdDouble.null( NULL, &sPtDst->mMbr.mMaxY );
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sPtDst, sPt)
                      != IDE_SUCCESS );

            sMPointDst->mSize += STD_POINT2D_SIZE;
            sMPointDst->mNumObjects++;
            sPtDst = STD_NEXT_POINT2D(sPtDst);
        }
        
        // End Point
        sPt = STD_LAST_PT2D(sLine);
        bFound = ID_FALSE;
        // BUG-16420
        sLineCP = STD_FIRST_LINE2D(aMLine);
        sDupCount = 0;
        for( j = 0; j < sMaxJ; j++ )
        {
            if( (i==j) || (isClosedLine2D( sLineCP )==ID_TRUE) )
            {
                sLineCP = STD_NEXT_LINE2D(sLineCP);
                continue;
            }
            if(stdUtils::isEndPoint2D(sPt,sLineCP)==ID_TRUE)
            {
                if( i>j )
                {
                    bFound = ID_FALSE;            
                    break;
                }
                else
                {
                    bFound = ID_TRUE;
                    sDupCount++;
                }
            }
            sLineCP = STD_NEXT_LINE2D(sLineCP);
        }
        if( (bFound==ID_TRUE) && ((sDupCount%2)==1) )
        {
            bFound = ID_FALSE;
        }
        
        if(bFound==ID_FALSE)
        {
            IDE_TEST_RAISE(sMPointDst->mSize + STD_POINT2D_SIZE > aFence, size_error);
            stdUtils::setType((stdGeometryHeader*)sPtDst, STD_POINT_2D_TYPE);
            sPtDst->mSize = STD_POINT2D_SIZE;
            idlOS::memcpy(&sPtDst->mPoint, sPt, STD_PT2D_SIZE);
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sMPointDst, sPt)
                      != IDE_SUCCESS );
            // PR-18113
            mtdDouble.null( NULL, &sPtDst->mMbr.mMaxX );
            mtdDouble.null( NULL, &sPtDst->mMbr.mMaxY );
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sPtDst, sPt)
                      != IDE_SUCCESS );
            
            sMPointDst->mSize += STD_POINT2D_SIZE;
            sMPointDst->mNumObjects++;
            sPtDst = STD_NEXT_POINT2D(sPtDst);
        }
        sLine = STD_NEXT_LINE2D(sLine);
    }
    
    // 모든 Line이 closed
    if(STD_N_OBJECTS(sMPointDst) == 0)
    {
        stdUtils::makeEmpty(aGeom);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_BUFFER_OVERFLOW ));
    }  

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * stdPolygon2DType의 Boundary인 링을 라인객체로 리턴
 * 링이 하나면 stdLineString2DType 객체로 리턴
 * 링이 2개 이상이면 stdMultiLineString2DType 객체로 리턴
 *
 * const stdPolygon2DType* aPoly(In): Boundary를 찾을 객체
 * stdGeometryHeader* aGeom(Out): Boundary 객체
 *******************************************t***************************/
IDE_RC stdPrimitive::getBoundaryPoly2D(
                        const stdPolygon2DType*         aPoly,
                        stdGeometryHeader*              aGeom,
                        UInt                            aFence )  // Fix Bug - 25110
{
    UInt        sMaxR = STD_N_RINGS(aPoly);

    if( sMaxR <= 0 )
    {
        stdUtils::makeEmpty(aGeom);
        return IDE_FAILURE;
    }
    else if( sMaxR == 1 )
    {
        stdLineString2DType*    sLineDst = (stdLineString2DType*)aGeom;
        stdPoint2D*             sPtDst = STD_FIRST_PT2D(sLineDst);
        stdLinearRing2D*        sRing = STD_FIRST_RN2D(aPoly);
        stdPoint2D*             sPt = STD_FIRST_PT2D(sRing);
        UInt                    i, sMax;
 
        stdUtils::setType((stdGeometryHeader*)sLineDst, STD_LINESTRING_2D_TYPE);
        sLineDst->mSize = STD_LINE2D_SIZE;
        sLineDst->mNumPoints = 0;
        sMax = STD_N_POINTS(sRing);
       IDE_TEST_RAISE( STD_LINE2D_SIZE + sMax* STD_PT2D_SIZE  > aFence, size_error);
        for( i = 0; i < sMax; i++ )
        {
            idlOS::memcpy(sPtDst, sPt, STD_PT2D_SIZE);
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sLineDst, sPt)
                      != IDE_SUCCESS );
        
            sLineDst->mSize += STD_PT2D_SIZE;
            sLineDst->mNumPoints++;
            sPtDst = STD_NEXT_PT2D(sPtDst);
            sPt = STD_NEXT_PT2D(sPt);
        }
    }
    else
    {
        stdMultiLineString2DType* sMLineDst = (stdMultiLineString2DType*)aGeom;
        stdLineString2DType*    sLineDst = STD_FIRST_LINE2D(sMLineDst);
        stdLinearRing2D*        sRing = STD_FIRST_RN2D(aPoly);
        UInt                    i, j, sMax;
        stdPoint2D*             sPt;
        stdPoint2D*             sPtDst;

        stdUtils::setType((stdGeometryHeader*)sMLineDst, 
            STD_MULTILINESTRING_2D_TYPE);
        sMLineDst->mSize = STD_MLINE2D_SIZE;
        sMLineDst->mNumObjects = 0;
        for( i = 0; i < sMaxR; i++ )
        {
            sMLineDst->mSize += STD_LINE2D_SIZE;
            sMLineDst->mNumObjects++;
            stdUtils::setType((stdGeometryHeader*)sLineDst, 
                STD_LINESTRING_2D_TYPE);
            sLineDst->mSize = STD_LINE2D_SIZE;
            sLineDst->mNumPoints = 0;
            // PR-18113
            mtdDouble.null( NULL, &sLineDst->mMbr.mMaxX );
            mtdDouble.null( NULL, &sLineDst->mMbr.mMaxY );
            
            sPtDst = STD_FIRST_PT2D(sLineDst);
            sPt = STD_FIRST_PT2D(sRing);
            sMax = STD_N_POINTS(sRing);
            IDE_TEST_RAISE(  sLineDst->mSize + STD_PT2D_SIZE * sMax > aFence, size_error);
            for( j = 0; j < sMax; j++ )
            {
                idlOS::memcpy(sPtDst, sPt, STD_PT2D_SIZE);
                IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sMLineDst,
                                                        sPt)
                          != IDE_SUCCESS );
                IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sLineDst,
                                                        sPt)
                          != IDE_SUCCESS );
        
                sMLineDst->mSize += STD_PT2D_SIZE;
                sLineDst->mSize += STD_PT2D_SIZE;
                sLineDst->mNumPoints++;
                sPtDst = STD_NEXT_PT2D(sPtDst);
                sPt = STD_NEXT_PT2D(sPt);
            }
            sLineDst = (stdLineString2DType*)sPtDst;            
            sRing = (stdLinearRing2D*)sPt;
        }
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_BUFFER_OVERFLOW ));
    }  

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * stdMultiPolygon2DType의 Boundary인 링을 라인객체로 리턴
 * 링이 하나면 stdLineString2DType 객체로 리턴
 * 링이 2개 이상이면 stdMultiLineString2DType 객체로 리턴
 *
 * const stdMultiPolygon2DType* aMPoly(In): Boundary를 찾을 객체
 * stdGeometryHeader* aGeom(Out): Boundary 객체
 **********************************************************************/
IDE_RC stdPrimitive::getBoundaryMPoly2D(
                        const stdMultiPolygon2DType*    aMPoly,
                        stdGeometryHeader*              aGeom ,
                        UInt                            aFence)   //Fix BUG - 25110
{
    UInt        sMaxO = STD_N_OBJECTS(aMPoly);
    
    if( sMaxO <= 0 )
    {
        stdUtils::makeEmpty(aGeom);
        return IDE_FAILURE;
    }

    stdPolygon2DType*           sPolygon = STD_FIRST_POLY2D(aMPoly);
    stdLinearRing2D*            sRing = STD_FIRST_RN2D(sPolygon);
    stdPoint2D*                 sPt = STD_FIRST_PT2D(sRing);
    UInt                        sMaxR = STD_N_RINGS(sPolygon);
    
    if( sMaxO == 1 && sMaxR == 1 )
    {
        stdLineString2DType*    sLineDst = (stdLineString2DType*)aGeom;
        stdPoint2D*             sPtDst = STD_FIRST_PT2D(sLineDst);
        UInt                    i, sMax;

        stdUtils::setType((stdGeometryHeader*)sLineDst, STD_LINESTRING_2D_TYPE);
        sLineDst->mSize = STD_LINE2D_SIZE;
        sLineDst->mNumPoints = 0;
        sMax = STD_N_POINTS(sRing);
        IDE_TEST_RAISE( sLineDst->mSize + sMax * STD_PT2D_SIZE > aFence, size_error);
        for( i = 0; i < sMax; i++ )
        {
            idlOS::memcpy(sPtDst, sPt, STD_PT2D_SIZE);
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sLineDst, sPt)
                      != IDE_SUCCESS );
        
            sLineDst->mSize += STD_PT2D_SIZE;
            sLineDst->mNumPoints++;
            sPtDst = STD_NEXT_PT2D(sPtDst);
            sPt = STD_NEXT_PT2D(sPtDst);
        }
    }
    else
    {
        stdMultiLineString2DType* sMLineDst = (stdMultiLineString2DType*)aGeom;
        stdLineString2DType*    sLineDst = STD_FIRST_LINE2D(sMLineDst);
        UInt                    i, j, k, sMaxR, sMax;
        stdPoint2D*             sPtDst;

        stdUtils::setType((stdGeometryHeader*)sMLineDst, 
            STD_MULTILINESTRING_2D_TYPE);
        sMLineDst->mSize = STD_MLINE2D_SIZE;
        sMLineDst->mNumObjects = 0;
        for( i = 0; i < sMaxO; i++ )
        {
            sRing = STD_FIRST_RN2D(sPolygon);
            sMaxR = STD_N_RINGS(sPolygon);
            IDE_TEST_RAISE( sMLineDst->mSize + STD_LINE2D_SIZE * sMaxR> aFence, size_error);
            for( j = 0; j < sMaxR; j++ )
            {
                sMLineDst->mSize += STD_LINE2D_SIZE;
                sMLineDst->mNumObjects++;
                stdUtils::setType((stdGeometryHeader*)sLineDst, 
                    STD_LINESTRING_2D_TYPE);
                sLineDst->mSize = STD_LINE2D_SIZE;
                sLineDst->mNumPoints = 0;
                // PR-18113
                mtdDouble.null( NULL, &sLineDst->mMbr.mMaxX );
                mtdDouble.null( NULL, &sLineDst->mMbr.mMaxY );
            
                sPtDst = STD_FIRST_PT2D(sLineDst);
                sPt = STD_FIRST_PT2D(sRing);
                sMax = STD_N_POINTS(sRing);
                IDE_TEST_RAISE( sMLineDst->mSize + STD_PT2D_SIZE * sMax> aFence, size_error);
                for( k = 0; k < sMax; k++ )
                {
                    idlOS::memcpy(sPtDst, sPt, STD_PT2D_SIZE);
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sMLineDst,
                                                            sPt)
                              != IDE_SUCCESS );
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sLineDst,
                                                            sPt)
                              != IDE_SUCCESS );
        
                    sMLineDst->mSize += STD_PT2D_SIZE;
                    sLineDst->mSize += STD_PT2D_SIZE;
                    sLineDst->mNumPoints++;
                    sPtDst = STD_NEXT_PT2D(sPtDst);
                    sPt = STD_NEXT_PT2D(sPt);
                }
                sLineDst = (stdLineString2DType*)sPtDst;            
                sRing = (stdLinearRing2D*)sPt;
            }
            sPolygon = (stdPolygon2DType*)sRing;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(size_error);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_BUFFER_OVERFLOW ));
    }  

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * 2차원 Geometry 객체의 MBR을 stdPolygon2DType 객체로 리턴
 *
 * const stdGeometryHeader* aHeader(In): MBR을 찾을 객체
 * stdGeometryHeader* aGeom(Out): Polygon 객체
 **********************************************************************/
idBool stdPrimitive::GetEnvelope2D(
                        const stdGeometryHeader*        aHeader,
                        stdGeometryHeader*              aGeom )
{
    stdPolygon2DType*     sPolygon    = (stdPolygon2DType*)aGeom;
    stdLineString2DType*  sLineString = (stdLineString2DType*)aGeom;
    stdPoint2DType*       sPoint      = (stdPoint2DType*)aGeom;
    stdLinearRing2D*      sRing = STD_FIRST_RN2D(sPolygon);
    stdPoint2D*           sPt = STD_FIRST_PT2D(sRing);

    // BUG-16096
    if( (aHeader->mMbr.mMinX==aHeader->mMbr.mMaxX) &&
        (aHeader->mMbr.mMinY==aHeader->mMbr.mMaxY) )
    {
        stdUtils::setType((stdGeometryHeader*)aGeom, STD_POINT_2D_TYPE);
        sPoint->mSize = STD_POINT2D_SIZE; 
        sPoint->mMbr.mMinX = aHeader->mMbr.mMinX;
        sPoint->mMbr.mMinY = aHeader->mMbr.mMinY;
        sPoint->mMbr.mMaxX = aHeader->mMbr.mMaxX;
        sPoint->mMbr.mMaxY = aHeader->mMbr.mMaxY;
        sPoint->mPoint.mX  = aHeader->mMbr.mMinX;
        sPoint->mPoint.mY  = aHeader->mMbr.mMinY;
    }
    else if( (aHeader->mMbr.mMinX==aHeader->mMbr.mMaxX) ||
             (aHeader->mMbr.mMinY==aHeader->mMbr.mMaxY) )
    {
        stdUtils::setType( (stdGeometryHeader*)aGeom, STD_LINESTRING_2D_TYPE );
        sLineString->mSize = STD_LINE2D_SIZE + STD_PT2D_SIZE*2; 
        sLineString->mMbr.mMinX = aHeader->mMbr.mMinX;
        sLineString->mMbr.mMinY = aHeader->mMbr.mMinY;
        sLineString->mMbr.mMaxX = aHeader->mMbr.mMaxX;
        sLineString->mMbr.mMaxY = aHeader->mMbr.mMaxY;
        sLineString->mNumPoints = 2;
        sPt = STD_FIRST_PT2D( sLineString );
        sPt[0].mX = aHeader->mMbr.mMinX;
        sPt[0].mY = aHeader->mMbr.mMinY;
        sPt[1].mX = aHeader->mMbr.mMaxX;
        sPt[1].mY = aHeader->mMbr.mMaxY;
    }
    else
    {
        stdUtils::setType((stdGeometryHeader*)sPolygon, STD_POLYGON_2D_TYPE);
        sPolygon->mSize = STD_POLY2D_SIZE + STD_RN2D_SIZE + STD_PT2D_SIZE*5; 
        sPolygon->mNumRings = 1;
        sPolygon->mMbr.mMinX = aHeader->mMbr.mMinX;
        sPolygon->mMbr.mMinY = aHeader->mMbr.mMinY;
        sPolygon->mMbr.mMaxX = aHeader->mMbr.mMaxX;
        sPolygon->mMbr.mMaxY = aHeader->mMbr.mMaxY;
    
        sRing->mNumPoints = 5;
    
        sPt->mX = aHeader->mMbr.mMinX;
        sPt->mY = aHeader->mMbr.mMinY;
        sPt = STD_NEXT_PT2D(sPt);
        sPt->mX = aHeader->mMbr.mMinX;
        sPt->mY = aHeader->mMbr.mMaxY;
        sPt = STD_NEXT_PT2D(sPt);
        sPt->mX = aHeader->mMbr.mMaxX;
        sPt->mY = aHeader->mMbr.mMaxY;
        sPt = STD_NEXT_PT2D(sPt);
        sPt->mX = aHeader->mMbr.mMaxX;
        sPt->mY = aHeader->mMbr.mMinY;
        sPt = STD_NEXT_PT2D(sPt);
        sPt->mX = aHeader->mMbr.mMinX;
        sPt->mY = aHeader->mMbr.mMinY;
    }
    
    return ID_TRUE;
}

/***********************************************************************
 * BUG-48051
 *  Description:
 *     polygon의 size가 정확한지 확인합니다.
 *      - Polygon    : 1개 이상의 심플하며 클로즈드한 LineString을 갖는 객체
 *      - LineString : 2개 이상의 점 &
 *                     클로즈드인 경우엔 Ø, 아닌 경우엔 양 끝점
 *  Implementation : 
 *
 **********************************************************************/
IDE_RC stdPrimitive::validatePolygon2DSize( stdGeometryHeader   * aGeom )
{
    stdPolygon2DType*   sPolygon;
    stdLinearRing2D*    sRing;
    UInt                i;
    UInt                sRingCnt         = 0;
    UInt                sPointCnt        = 0;
    SLong               sTotalPointSize  = 0;
    SLong               sTotalSize       = 0;

    sPolygon    = (stdPolygon2DType*)aGeom;
    sRingCnt    = STD_N_RINGS(sPolygon);

    // 1개 이상 ring으로 구성  
    IDE_TEST_RAISE( ( sRingCnt < 1 ), invalid_error );

    sRing       = STD_FIRST_RN2D(sPolygon);

    for ( i = 0; i < sRingCnt; i++)
    {
        // LineString은 2개 이상의 점으로 구성
        sPointCnt = STD_N_POINTS(sRing);
        IDE_TEST_RAISE ( sPointCnt < 2, invalid_error ); 

        sTotalPointSize += sPointCnt * STD_PT2D_SIZE;
        // geometry 최대 size가 100MB이기 때문에
        IDE_TEST_RAISE( sTotalPointSize > ID_UINT_MAX , invalid_error );

        sRing = STD_NEXT_RN2D(sRing);
    }

    // POLYGON2D obj size = polygon header + ring
    sTotalSize = STD_POLY2D_SIZE + (STD_RN2D_SIZE * sRingCnt) + sTotalPointSize;
    IDE_TEST_RAISE( (SLong)sPolygon->mSize != sTotalSize, invalid_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION(invalid_error);
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_POLYGON_NAME ) );
    }  
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * BUG-48051
 *  Description:
 *     multipolygon의 size가 정확한지 확인합니다.
 *      - MultiPolygon : 1개 이상의 POLYGON
 *  Implementation : 
 *
 **********************************************************************/
IDE_RC stdPrimitive::validateMPolygon2DSize( stdGeometryHeader   * aGeom )
{
    stdMultiPolygon2DType    * sMultiPolygon;
    stdPolygon2DType         * sPolygon;
    UInt                       i;
    UInt                       sPolyCnt       = 0;
    SLong                      sTotalPolySize = 0;

    sMultiPolygon = (stdMultiPolygon2DType*)aGeom;
    sPolyCnt      = STD_N_OBJECTS(sMultiPolygon);

    // 1개 이상 polygon으로 구성
    IDE_TEST_RAISE( ( sPolyCnt < 1 ), invalid_error );

    sPolygon      = STD_FIRST_POLY2D(sMultiPolygon);

    for ( i = 0; i < sPolyCnt; i++ )
    {
        if ( ( sPolygon->mType == STD_POLYGON_2D_TYPE ) ||
             ( sPolygon->mType == STD_POLYGON_2D_EXT_TYPE ) )
        {
            IDE_TEST( validatePolygon2DSize( (stdGeometryHeader *) sPolygon )
                      != IDE_SUCCESS ); 

            // 성공한경우 polygon의 mSize가 정확한 polygon size이다.
            sTotalPolySize += sPolygon->mSize;
            IDE_TEST_RAISE( sTotalPolySize >= ID_UINT_MAX, invalid_error );
        }
        else
        {
            IDE_RAISE( invalid_error );
        }

        sPolygon = STD_NEXT_POLY2D(sPolygon);
    }

    IDE_TEST_RAISE( (SLong)sMultiPolygon->mSize != (SLong)(STD_MPOLY2D_SIZE + sTotalPolySize),
                    invalid_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_error );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_MULTIPOLYGON_NAME ) );
    }  
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stdPrimitive::validateGeoColl2DSize( stdGeometryHeader  * aGeom )
{
    stdGeoCollection2DType  * sCollection;
    stdGeometryHeader*        sObj;
    UInt                      i;
    UInt                      sObjCnt    = 0;
    SLong                     sTotalSize = 0;

    sCollection = (stdGeoCollection2DType*)aGeom;
    sObj        = (stdGeometryHeader*)STD_FIRST_COLL2D(sCollection);
    sObjCnt     = STD_N_GEOMS(sCollection);

    for ( i = 0; i< sObjCnt; i++)
    {
        switch( sObj->mType )
        {
            case STD_POLYGON_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
                IDE_TEST( validatePolygon2DSize( sObj )
                          != IDE_SUCCESS ); 
                break;
            case STD_MULTIPOLYGON_2D_TYPE:
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
                IDE_TEST( validateMPolygon2DSize( sObj )
                          != IDE_SUCCESS ); 
                break;
            case STD_POINT_2D_TYPE:
            case STD_LINESTRING_2D_TYPE:
            case STD_MULTIPOINT_2D_TYPE:
            case STD_MULTILINESTRING_2D_TYPE:
            case STD_POINT_2D_EXT_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
            case STD_MULTIPOINT_2D_EXT_TYPE:
            case STD_MULTILINESTRING_2D_EXT_TYPE:
                // Not check
                break;
            case STD_GEOCOLLECTION_2D_TYPE:        
            default:
                IDE_TEST( 1 );
                break;
        }

        // 성공한경우 polygon의 mSize가 정확한 polygon size이다.
        sTotalSize += sObj->mSize;
        IDE_TEST( sTotalSize >= ID_UINT_MAX );

        sObj = (stdGeometryHeader*)STD_NEXT_GEOM( sObj );
    }

    IDE_TEST( (SLong)sCollection->mSize != (SLong)(STD_COLL2D_SIZE + sTotalSize) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                              (char *)STD_GEOCOLLECTION_NAME ) );
    return IDE_FAILURE;

}

