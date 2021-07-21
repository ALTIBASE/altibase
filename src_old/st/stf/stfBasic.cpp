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
 * $Id: stfBasic.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체의 기본 속성 함수 구현
 * 상세 구현을 필요로 하는 함수는 stdPrimitive.cpp를 실행한다.
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ste.h>
#include <mtd.h>
#include <mtdTypes.h>

#include <qc.h>
#include <qci.h>

#include <stdTypes.h>
#include <stdUtils.h>
#include <stcDef.h>
#include <stdPrimitive.h>
#include <stdMethod.h>
#include <stfBasic.h>
#include <stuProperty.h>
#include <stm.h>

extern mtdModule stdGeometry;
extern mtdModule mtdInteger;

/***********************************************************************
 * Description:
 * Geometry 객체의 차원을 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::dimension(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*     sRet = (mtdIntegerType*)aStack[0].value;

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
        IDE_TEST( getDimension( sValue, sRet )   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 타입을 문자열로 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::geometryType(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )    
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdCharType*        sRet = (mtdCharType*)aStack[0].value;
    UShort              sLen;

    IDE_ASSERT( sRet != NULL );
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        stdUtils::getTypeText( sValue->mType, 
                               (SChar*)sRet->value,
                               &sLen );
        sRet->length = sLen;
    }

    return IDE_SUCCESS;
    
    //IDE_EXCEPTION_END;

    //aStack[0].column->module->null( aStack[0].column,
    //                                aStack[0].value );
    
    //return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체를 WKT(Well Known Text)로 출력
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::asText(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )    
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    void*               aRow0  = aStack[0].value;
    mtdCharType*        sBuf   = (mtdCharType*)aRow0;
    UInt                sMaxSize = aStack[0].column->precision;    
    IDE_RC              sReturn;
    UInt                sSize = 0;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        IDE_TEST( getText( sValue, sBuf->value, sMaxSize, &sSize, &sReturn )
                  != IDE_SUCCESS );

        sBuf->length = (UShort)sSize;

        // BUG-48051
        if ( sReturn != IDE_SUCCESS ) 
        {
            IDE_TEST( ideGetErrorCode() == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB );
        }
        else
        {
            IDE_CLEAR();
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체를 EWKT(Extended Well Known Text)로 출력
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::asEWKT( mtcNode*     /* aNode */,
                         mtcStack*    aStack,
                         SInt         /* aRemain */,
                         void*        /* aInfo */ ,
                         mtcTemplate* /* aTemplate */ )    
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    void*               aRow0  = aStack[0].value;
    mtdCharType*        sBuf   = (mtdCharType*)aRow0;
    UInt                sMaxSize = aStack[0].column->precision;    
    IDE_RC              sReturn;
    UInt                sSize = 0;

    // Fix BUG-15412 mtdModule.isNull 사용
    if ( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        IDE_TEST( getEWKT( sValue, sBuf->value, sMaxSize, &sSize, &sReturn )
                  != IDE_SUCCESS );

        sBuf->length = (UShort)sSize;

        // BUG-48051
        if ( sReturn != IDE_SUCCESS ) 
        {
            IDE_TEST( ideGetErrorCode() == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB );
        }
        else
        {
            IDE_CLEAR();
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체를 WKB(Well Known Binary)로 출력
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::asBinary(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )    
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    void*               aRow0  = aStack[0].value;
    mtdBinaryType*      sBuf   = (mtdBinaryType*)aRow0;   // Fix BUG-15834
    UInt                sMaxSize = aStack[0].column->precision;    

    // Fix BUG-15412 mtdModule.isNull 사용
    if ( stdGeometry.isNull( NULL, sValue ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        IDE_TEST( getBinary( sValue, sBuf->mValue, sMaxSize, &sBuf->mLength )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체를 EWKB(Extended Well Known Binary)로 출력
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::asEWKB( mtcNode*     /* aNode */,
                         mtcStack*    aStack,
                         SInt         /* aRemain */,
                         void*        /* aInfo */ ,
                         mtcTemplate* /* aTemplate */ )    
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    void*               aRow0  = aStack[0].value;
    mtdBinaryType*      sBuf   = (mtdBinaryType*)aRow0;   // Fix BUG-15834
    UInt                sMaxSize = aStack[0].column->precision;    

    // Fix BUG-15412 mtdModule.isNull 사용
    if ( stdGeometry.isNull( NULL, sValue ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        IDE_TEST( getEWKB( sValue, sBuf->mValue, sMaxSize, &sBuf->mLength )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 Boundary를 Geometry 객체로 출력
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::boundary(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*  sRet = (stdGeometryHeader*)aStack[0].value;
    UInt                sFence = aStack[0].column->precision;

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

        IDE_TEST( getBoundary( sValue, sRet, sFence) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 MBR을 폴리곤 객체로 출력
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::envelope(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    stdGeometryHeader*  sRet = (stdGeometryHeader*)aStack[0].value;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        stdGeometry.null(NULL, sRet);  // Fix BUG-15504

        IDE_TEST( getEnvelope( sValue, sRet ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체가 Empty 객체인지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::isEmpty(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )    
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*     sRet = (mtdIntegerType*)aStack[0].value;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        switch(sValue->mType)
        {
        case STD_EMPTY_TYPE :
            *sRet = 1;
            break;
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
        case STD_GEOCOLLECTION_2D_TYPE :
            *sRet = 0;
            break;
        default :
            IDE_RAISE(err_invalid_object_type);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_object_type);
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );

        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));                                        
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체가 Simple한지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::isSimple(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        void*        /* aInfo */ ,
                        mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*     sRet = (mtdIntegerType*)aStack[0].value;

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
        IDE_TEST( testSimple( sValue, sRet )     != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체가 Valid한지 판별
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::isValid(
                        mtcNode*     /* aNode */,
                        mtcStack*    aStack,
                        SInt         /* aRemain */,
                        idBool       aCheckSize,
                        mtcTemplate* aTemplate )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*     sRet = (mtdIntegerType*)aStack[0].value;
    
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
        /* BUG-33576 
         * IsValid는 항상 validate를 호출하여 객체가 Valid한지 판별합니다.
         * 헤더의 mIsValid값만 읽기 위해서는 IsValidHeader 함수를 사용해야합니다. */

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
        sStage = 1;

        if ( aCheckSize == ID_FALSE )
        {
            if ( stdPrimitive::validate( sQmxMem,
                                         sValue,
                                         sValue->mSize ) == IDE_SUCCESS)
            {
                *sRet = 1;
            }
            else
            {
                *sRet = ST_INVALID;
            }
        }
        else      
        {
            // BUG-48051
            *(mtdIntegerType*)aStack[0].value = 1;
            switch ( sValue->mType )
            {
                case STD_POLYGON_2D_TYPE:
                case STD_POLYGON_2D_EXT_TYPE:
                    if ( stdPrimitive::validatePolygon2DSize( sValue )
                         != IDE_SUCCESS )
                    {
                        *(mtdIntegerType*)aStack[0].value = -1;
                        IDE_CLEAR();
                    }
                    break;
                case STD_MULTIPOLYGON_2D_TYPE:
                case STD_MULTIPOLYGON_2D_EXT_TYPE:
                    if ( stdPrimitive::validateMPolygon2DSize( sValue ) 
                         != IDE_SUCCESS )
                    {
                        *(mtdIntegerType*)aStack[0].value = -1;
                        IDE_CLEAR();
                    }
                    break;
                case STD_GEOCOLLECTION_2D_TYPE:        
                case STD_GEOCOLLECTION_2D_EXT_TYPE:        
                    if( stdPrimitive::validateGeoColl2DSize( sValue )
                        != IDE_SUCCESS )
                    {
                        *(mtdIntegerType*)aStack[0].value = -1;
                        IDE_CLEAR();
                    } 
                default:
                    // Nothing to do.
                    break;
            }
        } 

        /* Memory 재사용을 위한 Memory 이동 */
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

// BUG-33576
/***********************************************************************
 * Description:
 * Geometry 객체가 Valid한지 판별
 * ( Header의 mIsValid 값만 참조 )
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::isValidHeader(
                                mtcNode*     /* aNode */,
                                mtcStack*    aStack,
                                SInt         /* aRemain */,
                                void*        /* aInfo */ ,
                                mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*     sRet = (mtdIntegerType*)aStack[0].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        if (sValue->mIsValid == ST_VALID)
        {
            *sRet = 1;
        }
        else
        {
            *sRet = 0;
        }
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체의 SRID 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::getSRID( mtcNode*     /* aNode */,
                          mtcStack*    aStack,
                          SInt         /* aRemain */,
                          void*        /* aInfo */ ,
                          mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sValue = (stdGeometryHeader *)aStack[1].value;
    mtdIntegerType*     sRet = (mtdIntegerType*)aStack[0].value;

    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue )==ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        switch ( sValue->mType )
        {
            case STD_EMPTY_TYPE :
            case STD_POINT_2D_TYPE :
            case STD_LINESTRING_2D_TYPE :
            case STD_POLYGON_2D_TYPE :
            case STD_MULTIPOINT_2D_TYPE :
            case STD_MULTILINESTRING_2D_TYPE :
            case STD_MULTIPOLYGON_2D_TYPE :
            case STD_GEOCOLLECTION_2D_TYPE :
                *sRet = ST_SRID_UNDEFINED;
                break;
                
            case STD_POINT_2D_EXT_TYPE :
            case STD_LINESTRING_2D_EXT_TYPE :
            case STD_POLYGON_2D_EXT_TYPE :
            case STD_MULTIPOINT_2D_EXT_TYPE :
            case STD_MULTILINESTRING_2D_EXT_TYPE :
            case STD_MULTIPOLYGON_2D_EXT_TYPE :
            case STD_GEOCOLLECTION_2D_EXT_TYPE :
                *sRet = getSRID( sValue );
                break;
                
            default :
                IDE_RAISE( err_unsupported_object_type );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_unsupported_object_type );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체의 SRID 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
SInt stfBasic::getSRID( stdGeometryHeader  * aObj )
{
    SInt  sSRID = ST_SRID_UNDEFINED;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if ( stdGeometry.isNull( NULL, aObj ) == ID_TRUE )
    {
        // Nothing to do.
    }
    else    
    {
        switch ( aObj->mType )
        {
            case STD_POINT_2D_EXT_TYPE :
                sSRID = ( (stdPoint2DExtType*)aObj )->mSRID;
                break;
                
            case STD_LINESTRING_2D_EXT_TYPE :
                sSRID = ( (stdLineString2DExtType*)aObj )->mSRID;
                break;
                
            case STD_POLYGON_2D_EXT_TYPE :
                sSRID = ( (stdPolygon2DExtType*)aObj )->mSRID;
                break;
                
            case STD_MULTIPOINT_2D_EXT_TYPE :
                sSRID = ( (stdMultiPoint2DExtType*)aObj )->mSRID;
                break;
                
            case STD_MULTILINESTRING_2D_EXT_TYPE :
                sSRID = ( (stdMultiLineString2DExtType*)aObj )->mSRID;
                break;
                
            case STD_MULTIPOLYGON_2D_EXT_TYPE :
                sSRID = ( (stdMultiPolygon2DExtType*)aObj )->mSRID;
                break;
                
            case STD_GEOCOLLECTION_2D_EXT_TYPE :
                sSRID = ( (stdGeoCollection2DExtType*)aObj )->mSRID;
                break;
                
            default :
                break;
        }
    }

    return sSRID;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 차원을 리턴
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * mtdIntegerType*     aRet(Out): 차원값
 **********************************************************************/
IDE_RC stfBasic::getDimension(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    SInt    sDim;
    
    sDim = stdUtils::getDimension(aObj);    // Fix BUG-15752
    
    IDE_TEST_RAISE( sDim == -2, err_invalid_object_type);
    
    *aRet = sDim;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }    
    IDE_EXCEPTION_END;
    
    IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_APPLICABLE));

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체를 WKT로 출력
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * UChar*              aBuf(Out): 출력할 버퍼
 * UInt                aMaxSize(In): 버퍼 사이즈
 * UInt*               aOffset(Out): WKT 길이
 * IDE_RC              aReturn(Out): encoding 성공 여부
 **********************************************************************/
IDE_RC stfBasic::getText(
                    stdGeometryHeader*  aObj,
                    UChar*              aBuf,
                    UInt                aMaxSize,
                    UInt*               aOffset,
                    IDE_RC*             aReturn)
{
    // BUG-27630
    *aOffset = 0;
    
    switch(aObj->mType)
    {
    case STD_EMPTY_TYPE :
        *aOffset = STD_EMPTY_NAME_LEN;
        idlOS::strcpy((SChar*)aBuf, (SChar*)STD_EMPTY_NAME);
        *aReturn = IDE_SUCCESS;
        break;
    case STD_POINT_2D_EXT_TYPE :
    case STD_POINT_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writePointWKT2D((stdPoint2DType*)aObj,
                                              aBuf,
                                              aMaxSize,
                                              aOffset);
        break;    
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeLineStringWKT2D((stdLineString2DType*)aObj,
                                                   aBuf,
                                                   aMaxSize,
                                                   aOffset);
        break;
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writePolygonWKT2D((stdPolygon2DType*)aObj,
                                                aBuf,
                                                aMaxSize,
                                                aOffset);
        break;
    case STD_MULTIPOINT_2D_EXT_TYPE :
    case STD_MULTIPOINT_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeMultiPointWKT2D((stdMultiPoint2DType*)aObj,
                                                   aBuf,
                                                   aMaxSize,
                                                   aOffset);
        break;
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeMultiLineStringWKT2D((stdMultiLineString2DType*)aObj,
                                                        aBuf,
                                                        aMaxSize,
                                                        aOffset);
        break;

    case STD_MULTIPOLYGON_2D_EXT_TYPE :
    case STD_MULTIPOLYGON_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeMultiPolygonWKT2D((stdMultiPolygon2DType*)aObj,
                                                     aBuf,
                                                     aMaxSize,
                                                     aOffset);
        break;
    case STD_GEOCOLLECTION_2D_EXT_TYPE :
    case STD_GEOCOLLECTION_2D_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeGeoCollectionWKT2D((stdGeoCollection2DType*)aObj,
                                                      aBuf,
                                                      aMaxSize,
                                                      aOffset);
        break;
    default :
        IDE_RAISE(err_unsupported_object_type);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unsupported_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }    
    IDE_EXCEPTION_END;
    
    IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_APPLICABLE));

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * Geometry 객체를 WKB로 출력
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * UChar*              aBuf(Out): 출력할 버퍼
 * UInt                aMaxSize(In): 버퍼 사이즈
 * UInt*               aOffset(Out): WKB 길이
 **********************************************************************/
IDE_RC stfBasic::getBinary(
                    stdGeometryHeader*  aObj,
                    UChar*              aBuf,
                    UInt                aMaxSize,
                    UInt*               aOffset )
{
    IDE_RC  sRc;
    UInt    sErrorCode = 0;

    *aOffset = 0;
    switch(aObj->mType)
    {
        /* BUG-32531 Consider for GIS EMPTY */
        case STD_EMPTY_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writeEmptyWKB2D((stdMultiPoint2DType*)aObj,
                                             aBuf,
                                             aMaxSize,
                                             aOffset);
            break;
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writePointWKB2D((stdPoint2DType*)aObj,
                                             aBuf,
                                             aMaxSize,
                                             aOffset);
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writeLineStringWKB2D((stdLineString2DType*)aObj,
                                                  aBuf,
                                                  aMaxSize,
                                                  aOffset);
            break;
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writePolygonWKB2D((stdPolygon2DType*)aObj,
                                               aBuf,
                                               aMaxSize,
                                               aOffset);
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiPointWKB2D((stdMultiPoint2DType*)aObj,
                                                  aBuf,
                                                  aMaxSize,
                                                  aOffset);
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiLineStringWKB2D((stdMultiLineString2DType*)aObj,
                                                       aBuf,
                                                       aMaxSize,
                                                       aOffset);
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiPolygonWKB2D((stdMultiPolygon2DType*)aObj,
                                                    aBuf,
                                                    aMaxSize,
                                                    aOffset);
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
        case STD_GEOCOLLECTION_2D_TYPE :
            aBuf[0] = '\0';
            sRc = stdMethod::writeGeoCollectionWKB2D((stdGeoCollection2DType*)aObj,
                                                     aBuf,
                                                     aMaxSize,
                                                     aOffset);
            break;
        default :
            IDE_RAISE(err_unsupported_object_type);
    }

    // BUG-48051
    if ( sRc != IDE_SUCCESS )
    {
        sErrorCode = ideGetErrorCode();
        IDE_TEST( sErrorCode == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB );
    }
    else
    {
        IDE_CLEAR();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unsupported_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    IDE_EXCEPTION_END;

    if ( sErrorCode != stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB )
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_APPLICABLE));
    }
    else
    {
        // Nothing to do.
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 Boundary를 Geometry 객체로 출력
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * stdGeometryHeader*  aRet(Out): Boundary Geometry 객체
 **********************************************************************/
IDE_RC stfBasic::getBoundary(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet,
                    UInt                sFence )  //Fix Bug 25110
{
    switch(aObj->mType)
    {
    case STD_EMPTY_TYPE :
    case STD_POINT_2D_EXT_TYPE :
    case STD_POINT_2D_TYPE :
    case STD_MULTIPOINT_2D_EXT_TYPE :
    case STD_MULTIPOINT_2D_TYPE :
    case STD_GEOCOLLECTION_2D_EXT_TYPE :
    case STD_GEOCOLLECTION_2D_TYPE :
        stdUtils::makeEmpty(aRet);
        break;        
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        IDE_TEST(stdPrimitive::getBoundaryLine2D((stdLineString2DType*)aObj, aRet, sFence) != IDE_SUCCESS );
        break;
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
        IDE_TEST(stdPrimitive::getBoundaryMLine2D((stdMultiLineString2DType*)aObj, aRet, sFence ) != IDE_SUCCESS );
        break;
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
        IDE_TEST(stdPrimitive::getBoundaryPoly2D((stdPolygon2DType*)aObj, aRet, sFence ) != IDE_SUCCESS );
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE :
    case STD_MULTIPOLYGON_2D_TYPE :
        IDE_TEST(stdPrimitive::getBoundaryMPoly2D((stdMultiPolygon2DType*)aObj, aRet, sFence ) != IDE_SUCCESS );
        break;
    // OGC 표준 외의 타입 ////////////////////////////////////////////////////////
    default :
        IDE_RAISE(err_unsupported_object_type);
    }

    /* PROJ-2422 srid */
    if ( stdUtils::isExtendedType( aObj->mType ) == ID_TRUE )
    {
        IDE_TEST( setSRID( aRet, sFence, aObj )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unsupported_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }    
    IDE_EXCEPTION_END;
    



    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * Geometry 객체의 MBR을 폴리곤 객체로 출력
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * stdGeometryHeader*  aRet(Out): 폴리곤 객체
 **********************************************************************/
IDE_RC stfBasic::getEnvelope(
                    stdGeometryHeader*  aObj,
                    stdGeometryHeader*  aRet )
{
    switch(aObj->mType)
    {
    case STD_EMPTY_TYPE :
        stdUtils::makeEmpty(aRet);
        break;  // Fix BUG-16440
    case STD_POINT_2D_EXT_TYPE :           // Fix BUG-16096
    case STD_POINT_2D_TYPE :           // Fix BUG-16096
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
    case STD_MULTIPOINT_2D_EXT_TYPE :
    case STD_MULTIPOINT_2D_TYPE :
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
    case STD_MULTIPOLYGON_2D_EXT_TYPE :
    case STD_MULTIPOLYGON_2D_TYPE :
    case STD_GEOCOLLECTION_2D_EXT_TYPE :
    case STD_GEOCOLLECTION_2D_TYPE :
        stdPrimitive::GetEnvelope2D( aObj, aRet );
        break;
    // OGC 표준 외의 타입 ////////////////////////////////////////////////////////
    default :
        IDE_RAISE(err_unsupported_object_type);
    }

    /* PROJ-2422 srid */
    if ( stdUtils::isExtendedType(aObj->mType) == ID_TRUE )
    {
        IDE_TEST( setSRID( aRet, ID_UINT_MAX, aObj )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unsupported_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }    
    IDE_EXCEPTION_END;
    
    IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_APPLICABLE));

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * Geometry 객체가 Simple 객체이면 1 아니면 0을 리턴
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * mtdIntegerType*     aRet(Out): 결과
 **********************************************************************/
IDE_RC stfBasic::testSimple(
                    stdGeometryHeader*  aObj,
                    mtdIntegerType*     aRet )
{
    *aRet = 0;
    switch(aObj->mType)
    {
    case STD_EMPTY_TYPE :
    case STD_POINT_2D_EXT_TYPE :
    case STD_POINT_2D_TYPE :
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
    case STD_MULTIPOLYGON_2D_EXT_TYPE : // BUG-16364
    case STD_MULTIPOLYGON_2D_TYPE : // BUG-16364
        *aRet = 1;
        break;
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
        if(stdPrimitive::isSimpleLine2D((stdLineString2DType*)aObj)==ID_TRUE)
        {
            *aRet = 1;
        }
        break;
    case STD_MULTIPOINT_2D_EXT_TYPE :
    case STD_MULTIPOINT_2D_TYPE :
        if(stdPrimitive::isSimpleMPoint2D((stdMultiPoint2DType*)aObj)==ID_TRUE)
        {
            *aRet = 1;
        }
        break;
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
        if(stdPrimitive::isSimpleMLine2D((stdMultiLineString2DType*)aObj)
            ==ID_TRUE)
        {
            *aRet = 1;
        }
        break;
    case STD_GEOCOLLECTION_2D_EXT_TYPE :
    case STD_GEOCOLLECTION_2D_TYPE :
        if(stdPrimitive::isSimpleCollect2D((stdGeoCollection2DType*)aObj)
            ==ID_TRUE)
        {
            *aRet = 1;
        }
        break;
    default :
        IDE_RAISE(err_unsupported_object_type);
    }      

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_unsupported_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }    
    IDE_EXCEPTION_END;
    
    IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_APPLICABLE));

    return IDE_FAILURE;    
}

IDE_RC stfBasic::getRectangle( mtcTemplate       * aTemplate,
                               mtdDoubleType       aX1,
                               mtdDoubleType       aY1,
                               mtdDoubleType       aX2,
                               mtdDoubleType       aY2,
                               stdGeometryHeader * aRet )
{
    SDouble            sX1        = (SDouble)aX1;
    SDouble            sY1        = (SDouble)aY1;
    SDouble            sX2        = (SDouble)aX2;
    SDouble            sY2        = (SDouble)aY2;
    UInt               sSize      = ( STD_PT2D_SIZE * 5 ) + STD_RN2D_SIZE + STD_POLY2D_SIZE;
    stdPolygon2DType * sGeom      = NULL;
    stdLinearRing2D  * sRing      = NULL;
    stdPoint2D       * sPt2D      = NULL;
    qcTemplate       * sQcTmplate = NULL;
    iduMemory        * sQmxMem    = NULL;

    IDE_ERROR( aTemplate != NULL );
    IDE_ERROR( aRet      != NULL );

    sQcTmplate = (qcTemplate*) aTemplate;
    sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

    /* Storing Data */
    idlOS::memset( aRet, 0x00, sSize );

    /* Fix BUG-15290 */
    stdUtils::nullify( aRet );

    /* POLYGON( ( x1 y1, x2 y1, x2 y2, x1 y2, x1 y1 ) ) */
    aRet->mType       = STD_POLYGON_2D_TYPE;
    aRet->mSize       = sSize;
    sGeom             = (stdPolygon2DType *)aRet;
    sGeom->mNumRings  = 1;

    /* Move First Ring */
    sRing             = STD_FIRST_RN2D( sGeom );
    sRing->mNumPoints = 5;

    /* Move First Point */
    sPt2D = STD_FIRST_PT2D( sRing );

    sPt2D->mX = sX1;
    sPt2D->mY = sY1;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aRet,
                                             sPt2D )
              != IDE_SUCCESS );

    /* Move Next Point */
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX2;
    sPt2D->mY = sY1;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aRet,
                                             sPt2D )
              != IDE_SUCCESS );

    /* Move Next Point */
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX2;
    sPt2D->mY = sY2;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aRet,
                                             sPt2D )
              != IDE_SUCCESS );

    /* Move Next Point */
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX1;
    sPt2D->mY = sY2;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aRet,
                                             sPt2D )
              != IDE_SUCCESS );

    /* Move Next Point */
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX1;
    sPt2D->mY = sY1;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aRet,
                                             sPt2D )
              != IDE_SUCCESS );

    /* BUG-27002 */
    if ( stdPrimitive::validatePolygon2D( sQmxMem,
                                          sGeom,
                                          sSize )
         == IDE_SUCCESS )
    {
        /* stdParsing::setValidHeader( sGeom, ID_TRUE, STU_VALIDATION_ENABLE_TRUE ); */
        sGeom->mIsValid = ST_VALID;
    }
    else
    {
        /* stdParsing::setValidHeader( sGeom, ID_FALSE, STU_VALIDATION_ENABLE_TRUE ); */
        sGeom->mIsValid = ST_INVALID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체를 EWKT로 출력
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * UChar*              aBuf(Out): 출력할 버퍼
 * UInt                aMaxSize(In): 버퍼 사이즈
 * UInt*               aOffset(Out): WKT 길이
 * IDE_RC              aReturn(Out): encoding 성공 여부
 **********************************************************************/
IDE_RC stfBasic::getEWKT( stdGeometryHeader*  aObj,
                          UChar*              aBuf,
                          UInt                aMaxSize,
                          UInt*               aOffset,
                          IDE_RC*             aReturn )
{
    // BUG-27630
    *aOffset = 0;
    
    switch ( aObj->mType )
    {
    case STD_EMPTY_TYPE :
        *aOffset = STD_EMPTY_NAME_LEN;
        idlOS::strcpy( (SChar*)aBuf, (SChar*)STD_EMPTY_NAME );
        *aReturn = IDE_SUCCESS;
        break;
    case STD_POINT_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writePointEWKT2D( (stdPoint2DExtType*)aObj,
                                                aBuf,
                                                aMaxSize,
                                                aOffset );
        break;    
    case STD_POINT_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writePointWKT2D( (stdPoint2DType*)aObj,
                                               aBuf,
                                               aMaxSize,
                                               aOffset );
        break;    
    case STD_LINESTRING_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeLineStringEWKT2D( (stdLineString2DExtType*)aObj,
                                                     aBuf,
                                                     aMaxSize,
                                                     aOffset );
        break;
    case STD_LINESTRING_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writeLineStringWKT2D( (stdLineString2DType*)aObj,
                                                    aBuf,
                                                    aMaxSize,
                                                    aOffset );
        break;
    case STD_POLYGON_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writePolygonEWKT2D( (stdPolygon2DExtType*)aObj,
                                                  aBuf,
                                                  aMaxSize,
                                                  aOffset );
        break;
    case STD_POLYGON_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writePolygonWKT2D( (stdPolygon2DType*)aObj,
                                                 aBuf,
                                                 aMaxSize,
                                                 aOffset );
        break;
    case STD_MULTIPOINT_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeMultiPointEWKT2D( (stdMultiPoint2DExtType*)aObj,
                                                     aBuf,
                                                     aMaxSize,
                                                     aOffset );
        break;
    case STD_MULTIPOINT_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writeMultiPointWKT2D( (stdMultiPoint2DType*)aObj,
                                                    aBuf,
                                                    aMaxSize,
                                                    aOffset );
        break;
    case STD_MULTILINESTRING_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeMultiLineStringEWKT2D( (stdMultiLineString2DExtType*)aObj,
                                                          aBuf,
                                                          aMaxSize,
                                                          aOffset );
        break;
    case STD_MULTILINESTRING_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writeMultiLineStringWKT2D( (stdMultiLineString2DType*)aObj,
                                                         aBuf,
                                                         aMaxSize,
                                                         aOffset );
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeMultiPolygonEWKT2D( (stdMultiPolygon2DExtType*)aObj,
                                                       aBuf,
                                                       aMaxSize,
                                                       aOffset );
        break;
    case STD_MULTIPOLYGON_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writeMultiPolygonWKT2D( (stdMultiPolygon2DType*)aObj,
                                                      aBuf,
                                                      aMaxSize,
                                                      aOffset );
        break;
    case STD_GEOCOLLECTION_2D_EXT_TYPE :
        aBuf[0] = '\0';
        *aReturn = stdMethod::writeGeoCollectionEWKT2D( (stdGeoCollection2DExtType*)aObj,
                                                        aBuf,
                                                        aMaxSize,
                                                        aOffset );
        break;
    case STD_GEOCOLLECTION_2D_TYPE :
        aBuf[0] = '\0';
        (void) stdMethod::writeSRID( ST_SRID_UNDEFINED,
                                     aBuf,
                                     aMaxSize,
                                     aOffset );
        *aReturn = stdMethod::writeGeoCollectionWKT2D( (stdGeoCollection2DType*)aObj,
                                                       aBuf,
                                                       aMaxSize,
                                                       aOffset );
        break;
    default :
        IDE_RAISE( err_unsupported_object_type );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_unsupported_object_type );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE ) );
    }    
    IDE_EXCEPTION_END;
    
    IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_APPLICABLE ) );

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체를 EWKB로 출력
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 * UChar*              aBuf(Out): 출력할 버퍼
 * UInt                aMaxSize(In): 버퍼 사이즈
 * UInt*               aOffset(Out): WKB 길이
 **********************************************************************/
IDE_RC stfBasic::getEWKB( stdGeometryHeader*  aObj,              
                          UChar*              aBuf,
                          UInt                aMaxSize,
                          UInt*               aOffset )
{
    IDE_RC  sRc;
    UInt    sErrorCode = 0;

    switch( aObj->mType )
    {
        /* BUG-32531 Consider for GIS EMPTY */
        case STD_EMPTY_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeEmptyWKB2D( (stdMultiPoint2DType*)aObj,
                                              aBuf,
                                              aMaxSize,
                                              aOffset );
            break; 
        case STD_POINT_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writePointWKB2D( (stdPoint2DType*)aObj,
                                              aBuf,
                                              aMaxSize,
                                              aOffset );
            break;    
        case STD_POINT_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writePointEWKB2D( (stdPoint2DExtType*)aObj,
                                               aBuf,
                                               aMaxSize,
                                               aOffset );
            break;    
        case STD_LINESTRING_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeLineStringWKB2D( (stdLineString2DType*)aObj,
                                                   aBuf,
                                                   aMaxSize,
                                                   aOffset );
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeLineStringEWKB2D( (stdLineString2DExtType*)aObj,
                                                    aBuf,
                                                    aMaxSize,
                                                    aOffset );
            break;
        case STD_POLYGON_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writePolygonWKB2D( (stdPolygon2DType*)aObj,
                                                aBuf,
                                                aMaxSize,
                                                aOffset );
            break;
        case STD_POLYGON_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writePolygonEWKB2D( (stdPolygon2DExtType*)aObj,
                                                 aBuf,
                                                 aMaxSize,
                                                 aOffset );
            break;
        case STD_MULTIPOINT_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiPointWKB2D( (stdMultiPoint2DType*)aObj,
                                                   aBuf,
                                                   aMaxSize,
                                                   aOffset );
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiPointEWKB2D( (stdMultiPoint2DExtType*)aObj,
                                                    aBuf,
                                                    aMaxSize,
                                                    aOffset );
            break;
        case STD_MULTILINESTRING_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiLineStringWKB2D( (stdMultiLineString2DType*)aObj,
                                                        aBuf,
                                                        aMaxSize,
                                                        aOffset );
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiLineStringEWKB2D( (stdMultiLineString2DExtType*)aObj,
                                                         aBuf,
                                                         aMaxSize,
                                                         aOffset );
            break;
        case STD_MULTIPOLYGON_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiPolygonWKB2D( (stdMultiPolygon2DType*)aObj,
                                                     aBuf,
                                                     aMaxSize,
                                                     aOffset );
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeMultiPolygonEWKB2D( (stdMultiPolygon2DExtType*)aObj,
                                                      aBuf,
                                                      aMaxSize,
                                                      aOffset );
            break;
        case STD_GEOCOLLECTION_2D_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeGeoCollectionWKB2D( (stdGeoCollection2DType*)aObj,
                                                      aBuf,
                                                      aMaxSize,
                                                      aOffset );
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
            *aOffset = 1;
            aBuf[0] = '\0';
            sRc = stdMethod::writeGeoCollectionEWKB2D( (stdGeoCollection2DExtType*)aObj,
                                                       aBuf,
                                                       aMaxSize,
                                                       aOffset );
            break;
        default :
            IDE_RAISE( err_unsupported_object_type );
    }

    // BUG-48051
    if ( sRc != IDE_SUCCESS )
    {
        sErrorCode = ideGetErrorCode();
        IDE_TEST( sErrorCode == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB );
    }
    else
    {
        IDE_CLEAR();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_unsupported_object_type );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE ) );
    }    
    IDE_EXCEPTION_END;

    if ( sErrorCode != stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_APPLICABLE ) );
    }
    else
    {
        //Nothing to do.
    }
    return IDE_FAILURE;    
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체의 SRID 리턴
 *
 * mtcStack*    aStack(InOut):
 **********************************************************************/
IDE_RC stfBasic::setSRID( mtcNode*     /* aNode */,
                          mtcStack*    aStack,
                          SInt         /* aRemain */,
                          void*        /* aInfo */ ,
                          mtcTemplate* /* aTemplate */ )
{
    stdGeometryHeader*  sRet = (stdGeometryHeader*)aStack[0].value;
    stdGeometryHeader*  sValue = (stdGeometryHeader*)aStack[1].value;
    mtdIntegerType*     sSRID = (mtdIntegerType*)aStack[2].value;
    UInt                sFence = aStack[0].column->precision;

    // Fix BUG-15412 mtdModule.isNull 사용
    if ( ( stdGeometry.isNull( NULL, sValue ) == ID_TRUE ) ||
         ( mtdInteger.isNull( NULL, sSRID ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else    
    {
        IDE_TEST_RAISE( sFence < sValue->mSize, ERR_INVALID_LENGTH );

        // copy
        idlOS::memcpy( (void*)sRet, (void*)sValue, sValue->mSize );

        IDE_TEST( setSRID( sRet, sFence, *sSRID ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Geometry 객체의 SRID 설정
 **********************************************************************/
IDE_RC stfBasic::setSRID( stdGeometryHeader  * aObj,
                          UInt                 aFence,
                          SInt                 aSRID )
{
    switch ( aObj->mType )
    {
        case STD_EMPTY_TYPE :
            break;
                
        case STD_POINT_2D_TYPE :
            IDE_TEST_RAISE( aFence < STD_POINT2D_EXT_SIZE, ERR_INVALID_LENGTH );
            idlOS::memset( (UChar*)aObj + aObj->mSize,
                           0x00,
                           STD_POINT2D_EXT_SIZE - STD_POINT2D_SIZE );
            aObj->mType = STD_POINT_2D_EXT_TYPE;
            aObj->mSize = STD_POINT2D_EXT_SIZE;
            ((stdPoint2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_LINESTRING_2D_TYPE :
            IDE_DASSERT( STD_LINE2D_SIZE == STD_LINE2D_EXT_SIZE );
            aObj->mType = STD_LINESTRING_2D_EXT_TYPE;
            ((stdLineString2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_POLYGON_2D_TYPE :
            IDE_DASSERT( STD_POLY2D_SIZE == STD_POLY2D_EXT_SIZE );
            aObj->mType = STD_POLYGON_2D_EXT_TYPE;
            ((stdPolygon2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_MULTIPOINT_2D_TYPE :
            IDE_DASSERT( STD_MPOINT2D_SIZE == STD_MPOINT2D_EXT_SIZE );
            aObj->mType = STD_MULTIPOINT_2D_EXT_TYPE;
            ((stdMultiPoint2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_MULTILINESTRING_2D_TYPE :
            IDE_DASSERT( STD_MLINE2D_SIZE == STD_MLINE2D_EXT_SIZE );
            aObj->mType = STD_MULTILINESTRING_2D_EXT_TYPE;
            ((stdMultiLineString2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_MULTIPOLYGON_2D_TYPE :
            IDE_DASSERT( STD_MPOLY2D_SIZE == STD_MPOLY2D_EXT_SIZE );
            aObj->mType = STD_MULTIPOLYGON_2D_EXT_TYPE;
            ((stdMultiPolygon2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_GEOCOLLECTION_2D_TYPE :
            IDE_DASSERT( STD_COLL2D_SIZE == STD_COLL2D_EXT_SIZE );
            aObj->mType = STD_GEOCOLLECTION_2D_EXT_TYPE;
            ((stdGeoCollection2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_POINT_2D_EXT_TYPE :
            ((stdPoint2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_LINESTRING_2D_EXT_TYPE :
            ((stdLineString2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_POLYGON_2D_EXT_TYPE :
            ((stdPolygon2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_MULTIPOINT_2D_EXT_TYPE :
            ((stdMultiPoint2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_MULTILINESTRING_2D_EXT_TYPE :
            ((stdMultiLineString2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
            ((stdMultiPolygon2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
            ((stdGeoCollection2DExtType*)aObj)->mSRID = aSRID;
            break;
                
        default :
            IDE_RAISE( err_unsupported_object_type );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION( err_unsupported_object_type );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Src Geometry 객체의 SRID를 Dest Geometry 객체의 SRID로 설정
 **********************************************************************/
IDE_RC stfBasic::setSRID( stdGeometryHeader  * aDstObj,
                          UInt                 aDstFence,
                          stdGeometryType    * aSrcObj )
{
    SInt   sSRID = ST_SRID_UNDEFINED;
    
    switch ( aSrcObj->header.mType )
    {
        case STD_POINT_2D_EXT_TYPE :
            sSRID = aSrcObj->point2DExt.mSRID;
            break;
                
        case STD_LINESTRING_2D_EXT_TYPE :
            sSRID = aSrcObj->linestring2DExt.mSRID;
            break;
                
        case STD_POLYGON_2D_EXT_TYPE :
            sSRID = aSrcObj->polygon2DExt.mSRID;
            break;
                
        case STD_MULTIPOINT_2D_EXT_TYPE :
            sSRID = aSrcObj->mpoint2DExt.mSRID;
            break;
                
        case STD_MULTILINESTRING_2D_EXT_TYPE :
            sSRID = aSrcObj->mlinestring2DExt.mSRID;
            break;
                
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
            sSRID = aSrcObj->mpolygon2DExt.mSRID;
            break;
                
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
            sSRID = aSrcObj->collection2DExt.mSRID;
            break;
                
        default :
            break;
    }

    // undefined srid라도 설정한다.
    IDE_TEST( setSRID( aDstObj, aDstFence, sSRID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * Src Geometry 객체의 SRID를 Dest Geometry 객체의 SRID로 설정
 **********************************************************************/
IDE_RC stfBasic::setSRID( stdGeometryHeader  * aDstObj,
                          UInt                 aDstFence,
                          stdGeometryHeader  * aSrcObj )
{
    SInt   sSRID = ST_SRID_UNDEFINED;
    
    switch ( aSrcObj->mType )
    {
        case STD_POINT_2D_EXT_TYPE :
            sSRID = ((stdPoint2DExtType*)aSrcObj)->mSRID;
            break;
                
        case STD_LINESTRING_2D_EXT_TYPE :
            sSRID = ((stdLineString2DExtType*)aSrcObj)->mSRID;
            break;
                
        case STD_POLYGON_2D_EXT_TYPE :
            sSRID = ((stdPolygon2DExtType*)aSrcObj)->mSRID;
            break;
                
        case STD_MULTIPOINT_2D_EXT_TYPE :
            sSRID = ((stdMultiPoint2DExtType*)aSrcObj)->mSRID;
            break;
                
        case STD_MULTILINESTRING_2D_EXT_TYPE :
            sSRID = ((stdMultiLineString2DExtType*)aSrcObj)->mSRID;
            break;
                
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
            sSRID = ((stdMultiPolygon2DExtType*)aSrcObj)->mSRID;
            break;
                
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
            sSRID = ((stdGeoCollection2DExtType*)aSrcObj)->mSRID;
            break;
                
        default :
            break;
    }

    // undefined srid라도 설정한다.
    IDE_TEST( setSRID( aDstObj, aDstFence, sSRID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: BUG-47816 ST_Transform 함수 지원
 * 입력 받은 SRID에 해당하는 PROJ.4 문자열을 반환
 **********************************************************************/
IDE_RC stfBasic::getProj4TextFromSRID ( qcStatement      * aStatement, 
                                        mtdIntegerType   * aSRID, 
                                        mtdCharType     ** aProj4Text )
{
    mtcColumn           * sSRIDColumn        = NULL;
    mtcColumn           * sProj4TextColumn   = NULL;
    
    smiTableCursor        sCursor;
    idBool                sIsCursorOpen      = ID_FALSE;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sCursorProperty;
    
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;

    IDU_FIT_POINT( "stfBasic::getProj4TextFromSRID" );

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gStmUserSrs,
                                  STM_USER_SRS_SRID_COL_ORDER,
                                  (const smiColumn **)&sSRIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gStmUserSrs,
                                  STM_USER_SRS_PROJ4TEXT_COL_ORDER,
                                  (const smiColumn **)&sProj4TextColumn )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn( &sRangeColumn,
                                        (const mtcColumn *)sSRIDColumn,
                                        (const void *)aSRID,
                                        &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, QC_STATISTICS ( aStatement ) );

    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gStmUserSrs,
                            gStmUserSrsIndex[STM_USER_SRS_SRID_IDX_ORDER],
                            smiGetRowSCN( gStmUserSrs ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty ) != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sRow == NULL, ERR_UNKNOWN_SRID );

    *aProj4Text = (mtdCharType *) mtc::value( sProj4TextColumn, sRow, MTD_OFFSET_USE );

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNKNOWN_SRID )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNKNOWN_SRID, *aSRID ) );
    }

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC stfBasic::changeExtTypeToBasic ( stdGeometryHeader  * aObj )
{
    switch ( aObj->mType )
    {
        case STD_POINT_2D_EXT_TYPE :
            aObj->mType = STD_POINT_2D_TYPE;
            aObj->mSize = STD_POINT2D_SIZE;
            idlOS::memset( (UChar*)aObj + aObj->mSize,                             
                           0x00,                                                   
                           STD_POINT2D_EXT_SIZE - STD_POINT2D_SIZE );              
            break;
                
        case STD_LINESTRING_2D_EXT_TYPE :
            aObj->mType = STD_LINESTRING_2D_TYPE;
            ((stdLineString2DExtType*)aObj)->mSRID = ST_SRID_UNDEFINED;         
            break;
                
        case STD_POLYGON_2D_EXT_TYPE :
            aObj->mType = STD_POLYGON_2D_TYPE;
            ((stdPolygon2DExtType*)aObj)->mSRID = ST_SRID_UNDEFINED;         
            break;
                
        case STD_MULTIPOINT_2D_EXT_TYPE :
            aObj->mType = STD_MULTIPOINT_2D_TYPE;
            ((stdMultiPoint2DExtType*)aObj)->mSRID = ST_SRID_UNDEFINED;         
            break;
                
        case STD_MULTILINESTRING_2D_EXT_TYPE :
            aObj->mType = STD_MULTILINESTRING_2D_TYPE;
            ((stdMultiLineString2DExtType*)aObj)->mSRID = ST_SRID_UNDEFINED;         
            break;
                
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
            aObj->mType = STD_MULTIPOLYGON_2D_TYPE;
            ((stdMultiPolygon2DExtType*)aObj)->mSRID = ST_SRID_UNDEFINED;         
            break;
                
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
            aObj->mType = STD_GEOCOLLECTION_2D_TYPE;
            ((stdGeoCollection2DExtType*)aObj)->mSRID = ST_SRID_UNDEFINED;         
            break;
                
        default :
            IDE_RAISE(err_invalid_object_mType);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
