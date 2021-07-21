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
 * $Id: stfFunctions.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체가 타입 별로 가지는 속성 함수 구현
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>
#include <ste.h>
#include <mtc.h>
#include <mtd.h>

#include <qc.h>

#include <stdUtils.h>
#include <stfBasic.h>
#include <stfAnalysis.h>
#include <stfFunctions.h>
#include <stdPrimitive.h>
#include <stuProperty.h>

extern mtdModule stdGeometry;
extern mtdModule mtdInteger;
extern mtdModule mtdDouble;

// -----------------------------------------------------------------------------
// Point -----------------------------------------------------------------------
// -----------------------------------------------------------------------------

/***********************************************************************
 * Description:
 * stdPoint2DType, stdPoint3DType의 x좌표를 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::coordX(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdPoint2DType*         sPoint2D;
    void*                   sRet = aStack[0].value;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        switch(sValue->mType)
        {
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
            sPoint2D = (stdPoint2DType*)sValue;
            idlOS::memcpy( sRet, &sPoint2D->mPoint.mX, ID_SIZEOF(SDouble));
            break;
        default:
            IDE_RAISE( err_invalid_object_mType );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdPoint2DType, stdPoint3DType의 y좌표를 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::coordY(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdPoint2DType*         sPoint2D;
    void*                   sRet = aStack[0].value;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        switch(sValue->mType)
        {
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
            sPoint2D = (stdPoint2DType*)sValue;
            idlOS::memcpy( sRet, &sPoint2D->mPoint.mY, ID_SIZEOF(SDouble));
            break;
        default:
            IDE_RAISE( err_invalid_object_mType );
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// -----------------------------------------------------------------------------
// LineString ------------------------------------------------------------------
// -----------------------------------------------------------------------------

/***********************************************************************
 * Description:
 * stdLineString2DType, stdLineString3DType의 시작점을 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::startPoint(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504
    
        IDE_TEST( getStartPoint( sValue, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdLineString2DType, stdLineString3DType의 끝점을 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::endPoint(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getEndPoint( sValue, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdLineString2DType, stdLineString3DType, stdMultiLineString2DType
 * stdMultiLineString3DType이 닫혀 있는지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::isClosed(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*         sRet = (mtdIntegerType*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );
        IDE_TEST( testClose( sValue, sRet )      != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdLineString2DType, stdLineString3DType, stdMultiLineString2DType
 * stdMultiLineString3DType이 링인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::isRing(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*         sRet = (mtdIntegerType*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );
        IDE_TEST( testRing( sValue, sRet )       != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * stdLineString2DType, stdLineString3DType, stdMultiLineString2DType
 * stdMultiLineString3DType의 총길이 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::length(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    void*                   sRet = aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue )      != IDE_SUCCESS );
        IDE_TEST( getLength( sValue, (SDouble*)sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * Geometry 객체의 Point개수  리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::numPoints(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*         sRet = (mtdIntegerType*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );
        IDE_TEST( getNumPoints( sValue, sRet )   != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체의 N 번째 Point 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::pointN(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet   = (stdGeometryHeader*)aStack[0].value;
    UInt*                   sNum   = (UInt*)aStack[2].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue )==ID_TRUE) ||
        (mtdInteger.isNull( NULL, sNum )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getPointN( sValue, *sNum, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// -----------------------------------------------------------------------------
// Polygon ---------------------------------------------------------------------
// -----------------------------------------------------------------------------

/***********************************************************************
 * Description:
 * stdPolygon2DType, stdPolygon3DType, stdMultiPolygon2DType, 
 * stdMultiPolygon3DType객체의 무게 중심 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::centroid(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getCentroid( sValue, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdPolygon2DType, stdPolygon3DType, stdMultiPolygon2DType, 
 * stdMultiPolygon3DType객체의 내부의 한 점 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::pointOnSurface(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    
    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;
    
    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
        sStage = 1;
        
        IDE_TEST( getPointOnSurface( sQmxMem, sValue, sRet ) != IDE_SUCCESS );
        
        // Memory 재사용을 위한 Memory 이동
        sStage = 0;
        IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdPolygon2DType, stdPolygon3DType, stdMultiPolygon2DType, 
 * stdMultiPolygon3DType객체의 넓이 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::area(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    void*                   sRet = aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576 
        IDE_TEST( stdUtils::checkValid( sValue )    != IDE_SUCCESS );
        IDE_TEST( getArea( sValue, (SDouble*)sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * stdPolygon2DType, stdPolygon3DType, stdMultiPolygon2DType, 
 * stdMultiPolygon3DType객체의 외부링 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::exteriorRing(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getExteriorRing( sValue, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdPolygon2DType, stdPolygon3DType, stdMultiPolygon2DType, 
 * stdMultiPolygon3DType객체의 내부링의 개수 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::numInteriorRing(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*         sRet = (mtdIntegerType*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue )     != IDE_SUCCESS );
        IDE_TEST( getNumInteriorRing( sValue, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * stdPolygon2DType, stdPolygon3DType, stdMultiPolygon2DType, 
 * stdMultiPolygon3DType객체의 N번 째 내부링 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::interiorRingN(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    UInt*                   sNum = (UInt*)aStack[2].value;
    
                            
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue )==ID_TRUE) ||
        (mtdInteger.isNull( NULL, sNum )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getInteriorRingN( sValue, *sNum, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 개수 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::numGeometries(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*         sRet = (mtdIntegerType*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        //BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue )   != IDE_SUCCESS );
        IDE_TEST( getNumGeometries( sValue, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description:
 * Geometry 객체의 N번째 객체 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::geometryN(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*      sRet = (stdGeometryHeader*)aStack[0].value;
    UInt*                   sNum = (UInt*)aStack[2].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( (stdGeometry.isNull( NULL, sValue )==ID_TRUE) ||
        (mtdInteger.isNull( NULL, sNum )==ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // BUG-33576
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getGeometryN( sValue, *sNum, sRet ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체의 시작점 리턴
 *
 * stdGeometryHeader*  aObj(In): 라인 객체
 * stdGeometryHeader*  aRet(Out): 포인트 객체
 **********************************************************************/
IDE_RC stfFunctions::getStartPoint(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet )
{
    stdLineString2DType*            sLine2D;
    stdPoint2DType*                 sRet2D;
    
    switch(aObj->mType)
    {
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        sLine2D = (stdLineString2DType*)aObj;
        sRet2D = (stdPoint2DType*)aRet;
        sRet2D->mType = STD_POINT_2D_TYPE;
        sRet2D->mSize = STD_POINT2D_SIZE;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D,
                                                STD_FIRST_PT2D(sLine2D))
                  != IDE_SUCCESS );
        idlOS::memcpy( &sRet2D->mPoint, STD_FIRST_PT2D(sLine2D), STD_PT2D_SIZE);
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }    
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체의 끝점 리턴
 *
 * stdGeometryHeader*  aObj(In): 라인 객체
 * stdGeometryHeader*  aRet(Out): 포인트 객체
 **********************************************************************/
IDE_RC stfFunctions::getEndPoint(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet )
{
    stdLineString2DType*            sLine2D;
    stdPoint2DType*                 sRet2D;
    stdPoint2D*                     sPt2D;

    switch(aObj->mType)
    {
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        sLine2D = (stdLineString2DType*)aObj;
        sRet2D = (stdPoint2DType*)aRet;
        sRet2D->mType = STD_POINT_2D_TYPE;
        sRet2D->mSize = STD_POINT2D_SIZE;
        // Fix BUG-15418
        sPt2D = STD_LAST_PT2D(sLine2D);
        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D, sPt2D)
                  != IDE_SUCCESS );
        idlOS::memcpy( &sRet2D->mPoint, sPt2D, STD_PT2D_SIZE);
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체가 닫혀있으면 1 아니면 0리턴
 *
 * stdGeometryHeader*  aObj(In): 라인 객체
 * mtdIntegerType*     aRet(Out): 결과
 **********************************************************************/
IDE_RC stfFunctions::testClose(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    switch(aObj->mType)
    {
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
        if( stdUtils::isClosed2D(aObj) == ID_TRUE )
        {
            *aRet = 1;
        }
        else
        {
            *aRet = 0;
        }
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체가 링이면 1 아니면 0리턴
 *
 * stdGeometryHeader*  aObj(In): 라인 객체
 * mtdIntegerType*     aRet(Out): 결과
 **********************************************************************/
IDE_RC stfFunctions::testRing(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    mtdIntegerType      sClosed;
    mtdIntegerType      sSimple;
    
    IDE_TEST( testClose(aObj, &sClosed) != IDE_SUCCESS );
    IDE_TEST( stfBasic::testSimple(aObj, &sSimple) != IDE_SUCCESS );
    
    if( (sClosed == 1) && (sSimple == 1) )
    {
        *aRet = 1;
    }
    else
    {
        *aRet = 0;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체의 길이 리턴
 *
 * stdGeometryHeader*  aObj(In): 라인 객체
 * SDouble*            aRet(Out): 결과
 **********************************************************************/
IDE_RC stfFunctions::getLength(
                    stdGeometryHeader*  aObj,
                    SDouble*            aRet )
{
    stdLineString2DType*            sLine2D;
    stdMultiLineString2DType*       sMLine2D;
    stdPoint2D*                     sPt2D;
    SDouble                         sLength = 0;
    SDouble                         sTmpLen;
    UInt                            i, sMax;

    switch(aObj->mType)
    {
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        sLine2D = (stdLineString2DType*)aObj;
        sPt2D = STD_FIRST_PT2D(sLine2D);
        sMax = STD_N_POINTS(sLine2D)-1;
        for( i = 0; i < sMax; i++ )
        {
            if( stdUtils::isSamePoints2D(
                sPt2D, STD_NEXT_PT2D(sPt2D)) == ID_FALSE )
            {
                sLength += stfAnalysis::primDistancePToP2D(
                    sPt2D, STD_NEXT_PT2D(sPt2D));
            }
            sPt2D = STD_NEXT_PT2D(sPt2D);
        }
        break;
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
        sMLine2D = (stdMultiLineString2DType*)aObj;
        sMax = STD_N_OBJECTS(sMLine2D);
        if(sMax <= 0)
        {
            break;
        }
        sLine2D = STD_FIRST_LINE2D(sMLine2D);
        for( i = 0; i < sMax; i++ )
        {
            getLength( (stdGeometryHeader*)sLine2D, &sTmpLen );            
            sLength += sTmpLen;
            
            sLine2D = STD_NEXT_LINE2D(sLine2D);
        }        
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    idlOS::memcpy( aRet, &sLength, ID_SIZEOF(SDouble));
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 객체의 포인트 개수 리턴
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * mtdIntegerType*     aRet(Out): 포인트 개수
 **********************************************************************/
IDE_RC stfFunctions::getNumPoints(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    stdGeometryType*            sGeom = (stdGeometryType*)aObj;
    UInt                        sMax = 0;
    UInt                        i;
    
    // Fix BUG-24475 : numpoints datatype expension
    stdLinearRing2D*            sRing2D;
    SInt                        sNumPoint = 0;
    SInt                        sTmpNum = 0;
    
    switch(aObj->mType)
    {
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
	         *aRet = 1;
             break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            *aRet = sGeom->linestring2D.mNumPoints;
            break;
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            sRing2D = STD_FIRST_RN2D(&(sGeom->polygon2D)); 	 
            sMax = STD_N_RINGS(&(sGeom->polygon2D)); 	 
            for( i = 0; i < sMax; i++ ) 	 
            { 	 
	             sNumPoint += STD_N_POINTS(sRing2D); 	 
	             sRing2D = STD_NEXT_RN2D(sRing2D); 	 
            } 	 
            *aRet = sNumPoint; 	 
            break; 	 
        case STD_MULTIPOINT_2D_EXT_TYPE : 	 
        case STD_MULTIPOINT_2D_TYPE : 	 
            *aRet = STD_N_OBJECTS(&(sGeom->mpoint2D)); 	 
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE : 	 
        case STD_MULTILINESTRING_2D_TYPE : 	 
            sMax = STD_N_OBJECTS(&(sGeom->mlinestring2D));
            sGeom = (stdGeometryType *)STD_FIRST_LINE2D(&(sGeom->mlinestring2D)); 	  
            for( i = 0; i < sMax; i++ ) 	 
            { 	 
                IDE_TEST( getNumPoints((stdGeometryHeader*)sGeom, &sTmpNum) 	 
	                  != IDE_SUCCESS ); 	 
                sNumPoint += sTmpNum; 	 
                sGeom = STD_NEXT_GEOM(sGeom);
            } 	 
            *aRet = sNumPoint;
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            sMax = STD_N_OBJECTS(&(sGeom->mpolygon2D));
            sGeom = (stdGeometryType *)STD_FIRST_POLY2D(&(sGeom->mpolygon2D)); 
            for( i = 0; i < sMax; i++ ) 
            {
                IDE_TEST( getNumPoints((stdGeometryHeader*)sGeom, &sTmpNum) 	 
                          != IDE_SUCCESS ); 	 
                sNumPoint += sTmpNum; 	 
                sGeom = (stdGeometryType*) STD_NEXT_GEOM(sGeom); 	 
            }
            *aRet = sNumPoint; 	 
            break; 
        case STD_GEOCOLLECTION_2D_EXT_TYPE:
        case STD_GEOCOLLECTION_2D_TYPE:
            sMax    = sGeom->collection2D.mNumGeometries;
            sGeom   = STD_FIRST_COLL2D(&(sGeom->collection2D));
            for( i = 0; i < sMax; i++ ) 
            {
                IDE_TEST( getNumPoints((stdGeometryHeader*)sGeom, &sTmpNum) 	 
                          != IDE_SUCCESS ); 	 
                sNumPoint += sTmpNum; 	 
                sGeom = STD_NEXT_GEOM(sGeom); 	 
            }
            *aRet = sNumPoint;            
            break;
            
        default:
            IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 라인 객체의 N번째 포인트 리턴
 *
 * stdGeometryHeader*  aObj(In): 라인 객체
 * UInt                aNum(In): 포인트 위치
 * stdGeometryHeader*  aRet(Out): 포인트 객체
 **********************************************************************/
IDE_RC stfFunctions::getPointN(
                    stdGeometryHeader*  aObj,
                    UInt                aNum,
                    stdGeometryHeader*  aRet )
{
    stdLineString2DType*    sLine2D;
    stdPoint2DType*         sRet2D;
    stdPoint2D*             sPt2D;
    
    switch(aObj->mType)
    {
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        sLine2D = (stdLineString2DType*)aObj;
        
        IDE_TEST_RAISE( (aNum == 0) ||(aNum > STD_N_POINTS(sLine2D) ),
            err_invalid_range);
        
        sPt2D = STD_FIRST_PT2D(sLine2D);
        sPt2D = STD_NEXTN_PT2D(sPt2D,aNum-1);
        
        sRet2D = (stdPoint2DType*)aRet;
        sRet2D->mType = STD_POINT_2D_TYPE;
        sRet2D->mSize = STD_POINT2D_SIZE;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D, sPt2D)
                  != IDE_SUCCESS );
        idlOS::memcpy( &sRet2D->mPoint, sPt2D, STD_PT2D_SIZE);
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_range);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤 객체의 무게 중심 리턴
 *
 * stdGeometryHeader*  aObj(In): 폴리곤 객체
 * stdGeometryHeader*  aRet(Out): 포인트 객체
 **********************************************************************/
IDE_RC stfFunctions::getCentroid(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet )
{
    stdPolygon2DType*               sPoly2D;
    stdLinearRing2D*                sRing2D;
    stdPoint2DType*                 sRet2D;
    
    switch(aObj->mType)
    {
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        sPoly2D = (stdPolygon2DType*)aObj;
        sRing2D = STD_FIRST_RN2D(sPoly2D);
        sRet2D = (stdPoint2DType*)aRet;
        sRet2D->mType = STD_POINT_2D_TYPE;
        sRet2D->mSize = STD_POINT2D_SIZE;
        IDE_TEST( stdUtils::centroidRing2D( sRing2D, &sRet2D->mPoint )
                  != IDE_SUCCESS );
        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D, 
                                                &sRet2D->mPoint)
                  != IDE_SUCCESS );
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE : // Fix BUG-15427
    case STD_MULTIPOLYGON_2D_TYPE : // Fix BUG-15427
        sRet2D = (stdPoint2DType*)aRet;
        sRet2D->mType = STD_POINT_2D_TYPE;
        sRet2D->mSize = STD_POINT2D_SIZE;
        IDE_TEST( stdUtils::centroidMPolygon2D(
                      (stdMultiPolygon2DType*)aObj, &sRet2D->mPoint )
                  != IDE_SUCCESS );
        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D, 
                                                &sRet2D->mPoint)
                  != IDE_SUCCESS );
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤 객체의 내부의 한 점 리턴
 *
 * stdGeometryHeader*  aObj(In): 폴리곤 객체
 * stdGeometryHeader*  aRet(Out): 포인트 객체
 **********************************************************************/
IDE_RC stfFunctions::getPointOnSurface( iduMemory*          aQmxMem,
                                        stdGeometryHeader*  aObj,
                                        stdGeometryHeader*  aRet )
{
    stdMultiPolygon2DType*          sMPoly2D;
    stdPolygon2DType*               sPoly2D;
    stdPoint2DType*                 sRet2D;
    
    switch(aObj->mType)
    {
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        sPoly2D = (stdPolygon2DType*)aObj;
        sRet2D = (stdPoint2DType*)aRet;
        sRet2D->mType = STD_POINT_2D_TYPE;
        sRet2D->mSize = STD_POINT2D_SIZE;
        IDE_TEST( stdUtils::getPointOnSurface2D( aQmxMem, sPoly2D, &sRet2D->mPoint )
                  != IDE_SUCCESS );
        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D, 
                                                &sRet2D->mPoint)
                  != IDE_SUCCESS );
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE : // Fix BUG-15430
    case STD_MULTIPOLYGON_2D_TYPE : // Fix BUG-15430
        sMPoly2D = (stdMultiPolygon2DType*)aObj;
        sPoly2D = STD_FIRST_POLY2D(sMPoly2D);
        IDE_TEST( getPointOnSurface(aQmxMem, (stdGeometryHeader*)sPoly2D, aRet)
             != IDE_SUCCESS );
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤 객체의 넓이 리턴
 *
 * stdGeometryHeader*  aObj(In): 폴리곤 객체
 * SDouble*            aRet(Out): 넓이
 **********************************************************************/
IDE_RC stfFunctions::getArea(
                    stdGeometryHeader*  aObj,
                    SDouble*            aRet )
{
    stdMultiPolygon2DType*          sMPoly2D;
    stdPolygon2DType*               sPoly2D;
    stdLinearRing2D*                sRing2D;
    
    SDouble                         sTotal = 0;
    SDouble                         sTmpArea;
    SDouble                         sHole = 0;
    UInt                            i, sMax;
    
    switch(aObj->mType)
    {
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        sPoly2D = (stdPolygon2DType*)aObj;
        sRing2D = STD_FIRST_RN2D(sPoly2D);
        sMax = STD_N_RINGS(sPoly2D);
        
        sTotal = stdUtils::areaRing2D(sRing2D);
        for( i = 1; i < sMax; i++ )
        {
            sRing2D = STD_NEXT_RN2D(sRing2D);
            sHole += stdUtils::areaRing2D(sRing2D);   // Fix BUG-15436
        }
        sTotal -= sHole;
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE : // Fix BUG-15431
    case STD_MULTIPOLYGON_2D_TYPE : // Fix BUG-15431
        sMPoly2D = (stdMultiPolygon2DType*)aObj;
        sPoly2D = STD_FIRST_POLY2D(sMPoly2D);
        sMax = STD_N_OBJECTS(sMPoly2D);
        for( i = 0; i < sMax; i++ )
        {
            IDE_TEST( getArea((stdGeometryHeader*)sPoly2D, &sTmpArea)
                 != IDE_SUCCESS );
            sTotal += sTmpArea;
            sPoly2D = STD_NEXT_POLY2D(sPoly2D);
        }
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    idlOS::memcpy( aRet, &sTotal, ID_SIZEOF(SDouble));
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤 객체의 외부링 리턴
 *
 * stdGeometryHeader*  aObj(In): 폴리곤 객체
 * stdGeometryHeader*  aRet(Out): 라인스트링 객체
 **********************************************************************/
IDE_RC stfFunctions::getExteriorRing(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet )
{
    stdPolygon2DType*               sPoly2D;
    stdLinearRing2D*                sRing2D;
    stdLineString2DType*            sRet2D;
    
    switch(aObj->mType)
    {
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        sPoly2D = (stdPolygon2DType*)aObj;
        sRing2D = STD_FIRST_RN2D(sPoly2D);
        sRet2D = (stdLineString2DType*)aRet;
        sRet2D->mType = STD_LINESTRING_2D_TYPE;
        sRet2D->mSize = STD_LINE2D_SIZE + STD_N_POINTS(sRing2D)*STD_PT2D_SIZE;
        stdUtils::copyMBR(&sRet2D->mMbr, &sPoly2D->mMbr);
        sRet2D->mNumPoints = STD_N_POINTS(sRing2D);     // Fix BUG-15420
        idlOS::memcpy( STD_FIRST_PT2D(sRet2D), STD_FIRST_PT2D(sRing2D), 
            STD_N_POINTS(sRing2D)*STD_PT2D_SIZE);
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤 객체의 내부링의 개수 리턴
 *
 * stdGeometryHeader*  aObj(In): 폴리곤 객체
 * mtdIntegerType*     aRet(Out): 내부링의 개수
 **********************************************************************/
IDE_RC stfFunctions::getNumInteriorRing(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    stdMultiPolygon2DType*      sMPoly2D;
    stdPolygon2DType*           sPoly2D;
    SInt                        sNumring = 0;    
    UInt                        i, sMax;
    
    switch(aObj->mType)
    {
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        sPoly2D = (stdPolygon2DType*)aObj;
        *aRet = STD_N_RINGS(sPoly2D) - 1;
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE :
    case STD_MULTIPOLYGON_2D_TYPE :
        sMPoly2D = (stdMultiPolygon2DType*)aObj;
        sPoly2D = STD_FIRST_POLY2D(sMPoly2D);
        sMax = STD_N_OBJECTS(sMPoly2D);
        for( i = 0; i < sMax; i++ )
        {
            sNumring += STD_N_RINGS(sPoly2D) - 1;
            sPoly2D = STD_NEXT_POLY2D(sPoly2D);
        }
        *aRet = sNumring;
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤 객체의 N번째 내부링 리턴
 *
 * stdGeometryHeader*  aObj(In): 폴리곤 객체
 * UInt                aNum(In): 링 위치
 * stdGeometryHeader*  aRet(Out): 라인스트링 객체
 **********************************************************************/
IDE_RC stfFunctions::getInteriorRingN(
                    stdGeometryHeader*  aObj,
                    UInt                aNum,
                    stdGeometryHeader*  aRet )
{
    stdPolygon2DType*               sPoly2D;
    stdLinearRing2D*                sRing2D;
    stdPoint2D*                     sPt2D;
    stdLineString2DType*            sRet2D;
    UInt                            i, sMax;
    
    switch(aObj->mType)
    {
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        sPoly2D = (stdPolygon2DType*)aObj;

        IDE_TEST_RAISE( (aNum == 0) ||(aNum > STD_N_RINGS(sPoly2D)-1),
            err_invalid_range);
        
        sRing2D = STD_FIRST_RN2D(sPoly2D);
        for( i = 0; i < aNum; i++ )
        {
            sRing2D = STD_NEXT_RN2D(sRing2D);
        }

        sMax = STD_N_POINTS(sRing2D);
        
        sRet2D = (stdLineString2DType*)aRet;
        sRet2D->mType = STD_LINESTRING_2D_TYPE;
        sRet2D->mSize = STD_LINE2D_SIZE + sMax*STD_PT2D_SIZE;
        sRet2D->mNumPoints = sMax; // Fix BUG-15422
        idlOS::memcpy( STD_FIRST_PT2D(sRet2D), STD_FIRST_PT2D(sRing2D), 
            sMax*STD_PT2D_SIZE);
            
        sPt2D = STD_FIRST_PT2D(sRing2D);
        for( i = 0; i < sMax; i++ )
        {
            IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sRet2D, sPt2D)
                      != IDE_SUCCESS );
            sPt2D = STD_NEXT_PT2D(sPt2D);
        }
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_range);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 내부 객체의 개수 리턴
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * mtdIntegerType*     aRet(Out): 내부 객체의 개수
 **********************************************************************/
IDE_RC stfFunctions::getNumGeometries(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    UInt sCnt;

    // Fix BUG-16096
    switch( aObj->mType )
    {
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
            
    // Fix BUG-15424
    sCnt = stdUtils::getGeometryNum(aObj);
    IDE_TEST_RAISE( sCnt <= 0, err_invalid_object_mType );
    
    *aRet = sCnt;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 N번째 내부 객체 리턴
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * UInt                aNum(In): 내부 객체 위치
 * stdGeometryHeader*  aRet(Out): 내부 객체
 **********************************************************************/
IDE_RC stfFunctions::getGeometryN(
                    stdGeometryHeader*  aObj,
                    UInt                aNum,
                    stdGeometryHeader*  aRet )
{
    stdGeometryHeader*          sNextObj;
    UInt                        i, sCnt;

    // Fix BUG-16101
    switch( aObj->mType )
    {
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        break;
    default:
        IDE_RAISE( err_invalid_object_mType );
    }
    
    // Fix BUG-15425
    sCnt = stdUtils::getGeometryNum(aObj);
    IDE_TEST_RAISE( sCnt <= 0, err_invalid_object_mType );
    
    IDE_TEST_RAISE( sCnt < aNum, err_invalid_range );
    
    sNextObj = stdUtils::getFirstGeometry(aObj);
    IDE_TEST_RAISE( sNextObj == NULL, err_invalid_object_mType );
    for( i = 1; i < aNum; i++ )
    {
        sNextObj = stdUtils::getNextGeometry(sNextObj);
        IDE_TEST_RAISE( sNextObj == NULL, err_invalid_range );
    }
    
    idlOS::memcpy( aRet, sNextObj, STD_GEOM_SIZE(sNextObj) );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_invalid_range);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * return geometry's mbr min-max-xy
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfFunctions::minMaxXY(
    mtcNode*     /* aNode */,
    mtcStack*    aStack,
    SInt         /* aRemain */,
    void*        /* aInfo */,
    mtcTemplate* /* aTemplate */ ,
    stGetMinMaxXYParam aParam)
{
    stdGeometryHeader*      sValue = (stdGeometryHeader *)aStack[1].value;
    void*                   sRet = aStack[0].value;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        switch (aParam)
        {
            case ST_GET_MINX:
                idlOS::memcpy(sRet, &(sValue->mMbr.mMinX), ID_SIZEOF(SDouble));
                break;
            case ST_GET_MINY:
                idlOS::memcpy(sRet, &(sValue->mMbr.mMinY), ID_SIZEOF(SDouble));
                break;
            case ST_GET_MAXX:
                idlOS::memcpy(sRet, &(sValue->mMbr.mMaxX), ID_SIZEOF(SDouble));
                break;
            case ST_GET_MAXY:
                idlOS::memcpy(sRet, &(sValue->mMbr.mMaxY), ID_SIZEOF(SDouble));
                break;
        }
    }
    
    return IDE_SUCCESS;
}

IDE_RC stfFunctions::reverseGeometry( mtcNode     */* aNode */,
                                      mtcStack    * aStack,
                                      SInt         /* aRemain */,
                                      void        */* aInfo */,
                                      mtcTemplate */* aTemplate */)
{
    stdGeometryHeader * sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader * sRet   = (stdGeometryHeader *)aStack[0].value;

    /* Fix BUG-15412 mtdModule.isNull 사용 */
    if ( stdGeometry.isNull( NULL, sValue ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* BUG-33576 */
        IDE_TEST( stdUtils::checkValid( sValue ) != IDE_SUCCESS );

        /* Geometry 복사 */
        stdUtils::copyGeometry( sRet, sValue );

        IDE_TEST( reverseAll( sValue, sRet ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfFunctions::reverseAll( stdGeometryHeader * aObj,
                                 stdGeometryHeader * aRet )
{
    stdGeometryType * sSrc      = (stdGeometryType *)aObj;
    stdGeometryType * sDst      = (stdGeometryType *)aRet;
    stdLinearRing2D * sSrcRing  = NULL;
    stdLinearRing2D * sDstRing  = NULL;
    stdPoint2DType  * sSrcPoint = NULL;
    stdPoint2DType  * sDstPoint = NULL;
    stdPoint2D      * sSrcXY    = NULL;
    stdPoint2D      * sDstXY    = NULL;
    UInt              sMax      = 0;
    UInt              sCnt      = 0;
    UInt              i         = 0;
    UInt              j         = 0;

    IDE_ERROR( aObj != NULL );
    IDE_ERROR( aRet != NULL );

    switch ( aObj->mType )
    {
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
             break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            sMax   = STD_N_POINTS( (stdLineString2DType *)sSrc );
            sSrcXY = STD_LAST_PT2D( (stdLineString2DType *)sSrc );
            sDstXY = STD_FIRST_PT2D( (stdLineString2DType *)sDst );

            for ( i = 0; i < sMax; i++ )
            {
                idlOS::memcpy( sDstXY, sSrcXY, STD_PT2D_SIZE );

                sSrcXY = STD_PREV_PT2D( sSrcXY );
                sDstXY = STD_NEXT_PT2D( sDstXY );
            }

            break;
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            sMax     = STD_N_RINGS( (stdPolygon2DType *)sSrc );
            sSrcRing = STD_FIRST_RN2D( (stdPolygon2DType *)sSrc );
            sDstRing = STD_FIRST_RN2D( (stdPolygon2DType *)sDst );

            for ( i = 0; i < sMax; i++ )
            {
                sCnt      = STD_N_POINTS( (stdLinearRing2D * )sSrcRing ) ;
                sSrcXY = STD_LAST_PT2D( (stdLinearRing2D * )sSrcRing );
                sDstXY = STD_FIRST_PT2D( (stdLinearRing2D * )sDstRing );

                for( j = 0; j < sCnt; j++ )
                {
                    idlOS::memcpy( sDstXY, sSrcXY, STD_PT2D_SIZE );

                    sSrcXY = STD_PREV_PT2D( sSrcXY );
                    sDstXY = STD_NEXT_PT2D( sDstXY );
                }

                sSrcRing = STD_NEXT_RN2D( sSrcRing );
                sDstRing = STD_NEXT_RN2D( sDstRing );
            }

            break;
        case STD_MULTIPOINT_2D_EXT_TYPE : 	 
        case STD_MULTIPOINT_2D_TYPE :
            sMax      = STD_N_OBJECTS( (stdMultiPoint2DType *)sSrc );
            sSrcPoint = STD_LAST_POINT2D( (stdMultiPoint2DType *)sSrc );
            sDstPoint = STD_FIRST_POINT2D( (stdMultiPoint2DType *)sDst );

            for ( i = 0; i < sMax; i++ )
            {
                idlOS::memcpy( sDstPoint, sSrcPoint, STD_POINT2D_SIZE );

                sSrcPoint = STD_PREV_POINT2D( sSrcPoint );
                sDstPoint = STD_NEXT_POINT2D( sDstPoint );
            }

            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :            
        case STD_MULTILINESTRING_2D_TYPE :
            sMax = STD_N_OBJECTS( (stdMultiLineString2DType *)sSrc );
            sSrc = (stdGeometryType *)STD_FIRST_LINE2D( sSrc );
            sDst = (stdGeometryType *)STD_FIRST_LINE2D( sDst );

            for ( i = 0; i < sMax; i++ )
            {
                IDE_TEST( reverseAll( (stdGeometryHeader *)sSrc,
                                      (stdGeometryHeader *)sDst )
                          != IDE_SUCCESS );

                sSrc = STD_NEXT_GEOM( sSrc );
                sDst = STD_NEXT_GEOM( sDst );
            }

            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            sMax = STD_N_OBJECTS( (stdMultiPolygon2DType *)sSrc );
            sSrc = (stdGeometryType *)STD_FIRST_POLY2D( sSrc );
            sDst = (stdGeometryType *)STD_FIRST_POLY2D( sDst );

            for ( i = 0; i < sMax; i++ )
            {
                IDE_TEST( reverseAll( (stdGeometryHeader *)sSrc,
                                      (stdGeometryHeader *)sDst )
                          != IDE_SUCCESS );

                sSrc = STD_NEXT_GEOM( sSrc );
                sDst = STD_NEXT_GEOM( sDst );
            }

            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE:            
        case STD_GEOCOLLECTION_2D_TYPE:
            sMax = STD_N_GEOMS( (stdGeoCollection2DType *)sSrc );
            sSrc = STD_FIRST_COLL2D( sSrc );
            sDst = STD_FIRST_COLL2D( sDst );

            for ( i = 0; i < sMax; i++ )
            {
                IDE_TEST( reverseAll( (stdGeometryHeader *)sSrc,
                                      (stdGeometryHeader *)sDst )
                          != IDE_SUCCESS );

                sSrc = STD_NEXT_GEOM( sSrc );
                sDst = STD_NEXT_GEOM( sDst );
            }

            break;
        default:
            IDE_RAISE( ERR_INVALID_OBJECT_MTYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_OBJECT_MTYPE );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfFunctions::makeEnvelope( mtcNode     */* aNode */,
                                   mtcStack    * aStack,
                                   SInt         /* aRemain */,
                                   void        */* aInfo */,
                                   mtcTemplate * aTemplate )
{
    stdGeometryHeader * sRet  = (stdGeometryHeader *)aStack[0].value;
    mtdDoubleType     * sX1   = (mtdDoubleType *)aStack[1].value;
    mtdDoubleType     * sY1   = (mtdDoubleType *)aStack[2].value;
    mtdDoubleType     * sX2   = (mtdDoubleType *)aStack[3].value;
    mtdDoubleType     * sY2   = (mtdDoubleType *)aStack[4].value;

    /* Fix BUG-15412 mtdModule.isNull 사용 */
    if( ( mtdDouble.isNull( NULL, sX1 ) == ID_TRUE ) ||
        ( mtdDouble.isNull( NULL, sY1 ) == ID_TRUE ) ||
        ( mtdDouble.isNull( NULL, sX2 ) == ID_TRUE ) ||
        ( mtdDouble.isNull( NULL, sY2 ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        IDE_TEST( stfBasic::getRectangle( aTemplate,
                                          * sX1,
                                          * sY1,
                                          * sX2,
                                          * sY2,
                                          sRet )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-47857 ST_MakePoint, ST_Point 함수 지원 */
IDE_RC stfFunctions::makePoint( mtcNode     */* aNode */,
                                mtcStack    * aStack,
                                SInt         /* aRemain */,
                                void        */* aInfo */,
                                mtcTemplate */* aTemplate */ )
{
    stdGeometryHeader * sRet = (stdGeometryHeader *)aStack[0].value;
    mtdDoubleType     * sX   = (mtdDoubleType *)aStack[1].value;
    mtdDoubleType     * sY   = (mtdDoubleType *)aStack[2].value;
    UInt               sSize = STD_POINT2D_SIZE;
    stdPoint2DType   * sGeom = NULL;

    /* Fix BUG-15412 mtdModule.isNull 사용 */
    if ( ( mtdDouble.isNull( NULL, sX ) == ID_TRUE ) ||
         ( mtdDouble.isNull( NULL, sY ) == ID_TRUE ) ) 
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* Fix BUG-15290 */
        stdUtils::nullify( sRet );

        /* POINT(x y) */
        sGeom        = (stdPoint2DType *)sRet;
        sGeom->mType = STD_POINT_2D_TYPE;
        sGeom->mSize = sSize;

        /* Set Point */
        sGeom->mPoint.mX = *sX;
        sGeom->mPoint.mY = *sY;

        IDE_TEST( stdUtils::mergeMBRFromPoint2D( sRet, &sGeom->mPoint ) != IDE_SUCCESS );

        if ( STU_VALIDATION_ENABLE == STU_VALIDATION_ENABLE_TRUE ) 
        {
            if ( stdPrimitive::validatePoint2D( NULL, sSize )
                 == IDE_SUCCESS ) 
            {
                sGeom->mIsValid = ST_VALID;
            }
            else
            {
                sGeom->mIsValid = ST_INVALID;
            }
        }
        else
        {
            sGeom->mIsValid = ST_INVALID;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
