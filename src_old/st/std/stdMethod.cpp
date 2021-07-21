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
 * $Id: stdMethod.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 객체를 WKT(Well Known Text) 또는 WKB(Well Known Binary)로 
 * 출력하는 모듈 구현
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>
#include <stdTypes.h>
#include <stdMethod.h>
#include <ste.h>

/***********************************************************************
 * Description:
 * aPoint 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdPoint2DType*            aPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePointWKT2D(
                    stdPoint2DType*            aPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    SChar  sTempBuf[1024];
    SChar  s2DCoord[128];

    fill2DCoordString( s2DCoord, ID_SIZEOF(s2DCoord), & aPoint->mPoint );
    
    idlOS::snprintf( sTempBuf,
                     ID_SIZEOF(sTempBuf),
                     STD_POINT_NAME"(%s) ",
                     s2DCoord );
    // BUG-27685 Valgrind BUG
    // strlen(sting) + 1 > buffer_size because of null termination.
    IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
        >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, sTempBuf);
    *aOffset += idlOS::strlen(sTempBuf);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        idlOS::snprintf((SChar*)aBuf, aMaxSize, STD_POINT_NAME"( ... ) ");
        *aOffset = idlOS::strlen((SChar*)aBuf);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * aLine 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdLineString2DType*       aLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeLineStringWKT2D(
                    stdLineString2DType*       aLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    SChar             sTempBuf[10240];
    stdPoint2D*       sPoint;
    UInt              i, sMax;
    UChar           * sFence;
    
    idlOS::sprintf(sTempBuf, STD_LINESTRING_NAME"(" );
    // BUG-27685 Valgrind BUG
    IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
        >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, sTempBuf);
    *aOffset += idlOS::strlen(sTempBuf);

    sPoint = STD_FIRST_PT2D(aLine);
    sMax = STD_N_POINTS(aLine);
    sFence = (UChar *)aLine + aLine->mSize;

    for( i = 0; i < sMax; i++)
    {
        /* BUG-48557 */
        IDE_TEST_RAISE( (UChar *)sPoint >= sFence, invalid_error );

        fill2DCoordString( sTempBuf, ID_SIZEOF(sTempBuf), sPoint );
        if( i < sMax - 1 )
        {
            IDE_TEST_RAISE( (UInt)(idlVA::appendFormat(
                sTempBuf, ID_SIZEOF(sTempBuf), ", " ))
                > ID_SIZEOF(sTempBuf), err_large_object );
        }
        IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf)) 
            >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, sTempBuf);
        *aOffset += idlOS::strlen(sTempBuf);
        sPoint = STD_NEXT_PT2D(sPoint);
    }
    IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, ") ");
    *aOffset += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_large_object);
    {
        idlOS::snprintf((SChar*)aBuf, aMaxSize, STD_LINESTRING_NAME"( ... ) ");
        *aOffset = idlOS::strlen((SChar*)aBuf);
    }
    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_LINESTRING_NAME ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPolygon 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdPolygon2DType*          aPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePolygonWKT2D(
                    stdPolygon2DType*          aPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    SChar             sTempBuf[10240];
    stdLinearRing2D * sRing;
    stdPoint2D      * sPoint;
    UInt              i, j, sMaxR, sMax;
    UChar           * sFence;
    
    idlOS::sprintf(sTempBuf, STD_POLYGON_NAME"(" );
    IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
        >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, sTempBuf);
    *aOffset += idlOS::strlen(sTempBuf);

    sRing = STD_FIRST_RN2D(aPolygon);
    sMaxR = STD_N_RINGS(aPolygon);
    sFence = (UChar *)aPolygon + aPolygon->mSize;

    for(i = 0; i < sMaxR; i++)
    {
        IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, "(");
        *aOffset += 1;
        
        sPoint = STD_FIRST_PT2D(sRing);
        sMax = STD_N_POINTS(sRing);
        for( j = 0; j < sMax; j++)
        {
            /* BUG-48557 */
            IDE_TEST_RAISE( (UChar *)sPoint >= sFence, invalid_error );

            fill2DCoordString( sTempBuf, ID_SIZEOF(sTempBuf), sPoint );
            if( j < sMax -1 )
            {
                IDE_TEST_RAISE( (UInt)(idlVA::appendFormat(
                    sTempBuf, ID_SIZEOF(sTempBuf), ", " ))
                    > ID_SIZEOF(sTempBuf), err_large_object );
            }
                            
            IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
                >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, sTempBuf);
            *aOffset += idlOS::strlen(sTempBuf);
            sPoint = STD_NEXT_PT2D(sPoint);
        } // for j
        IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, ")");
        *aOffset += 1;
        
        if( i < sMaxR -1 )
        {
            IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, ", ");
            *aOffset += 2;
        }
        sRing = (stdLinearRing2D*)sPoint;
    } // for i    

    // BUG-48051 geomFromWKB에서 잘못 생성된 polygon 객체인지 확인
    IDE_TEST_RAISE( sRing != (stdLinearRing2D *)((UChar*)(aPolygon) + aPolygon->mSize),
                    invalid_error );

    IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, ") ");
    *aOffset += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_POLYGON_NAME ) );
    }
    IDE_EXCEPTION(err_large_object);
    {
        idlOS::snprintf((SChar*)aBuf, aMaxSize, STD_POLYGON_NAME"( ... ) ");
        *aOffset = idlOS::strlen((SChar*)aBuf);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * aMPoint 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdMultiPoint2DType*       aMPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPointWKT2D(
                    stdMultiPoint2DType*       aMPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    SChar             sTempBuf[10240];
    stdPoint2DType*   sPoint;
    UInt              i, sMaxO;
    UChar           * sFence;
    
    idlOS::sprintf(sTempBuf, STD_MULTIPOINT_NAME"(" );
    IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
        >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, sTempBuf);
    *aOffset += idlOS::strlen(sTempBuf);

    sPoint = STD_FIRST_POINT2D(aMPoint);
    sMaxO = STD_N_OBJECTS(aMPoint);
    sFence = (UChar *)aMPoint + aMPoint->mSize;

    for( i = 0; i < sMaxO; i++)
    {
        /* BUG-48557 */
        IDE_TEST_RAISE( (UChar *)sPoint >= sFence, invalid_error );

        fill2DCoordString( sTempBuf, ID_SIZEOF(sTempBuf), & sPoint->mPoint );
        if( i < sMaxO - 1 )
        {
            IDE_TEST_RAISE( (UInt)(idlVA::appendFormat(
                sTempBuf, ID_SIZEOF(sTempBuf), ", " ))
                > ID_SIZEOF(sTempBuf), err_large_object );
        }
                            
        IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf)) 
            >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, sTempBuf);
        *aOffset += idlOS::strlen(sTempBuf);
        sPoint = STD_NEXT_POINT2D(sPoint);
    }
    IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, ") ");
    *aOffset += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_large_object);
    {
        idlOS::snprintf((SChar*)aBuf, aMaxSize, STD_MULTIPOINT_NAME"( ... ) ");
        *aOffset = idlOS::strlen((SChar*)aBuf);
    }
    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_MULTIPOINT_NAME ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMLine 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdMultiLineString2DType*  aMLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiLineStringWKT2D(
                    stdMultiLineString2DType*  aMLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    SChar                sTempBuf[10240];
    stdLineString2DType* sLine;
    stdPoint2D*          sPoint;
    UInt                 i, j, sMaxO, sMax;
    UChar              * sFence;
    
    idlOS::sprintf(sTempBuf, STD_MULTILINESTRING_NAME"(" );
    IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
        >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, sTempBuf);
    *aOffset += idlOS::strlen(sTempBuf);

    sLine = STD_FIRST_LINE2D(aMLine);
    sMaxO = STD_N_OBJECTS(aMLine);
    sFence = (UChar *)aMLine + aMLine->mSize;

    for(i = 0; i < sMaxO; i++)
    {
        IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, "(");
        *aOffset += 1;
        
        sPoint = STD_FIRST_PT2D(sLine);
        sMax = STD_N_POINTS(sLine);
        for( j = 0; j < sMax; j++)
        {
            /* BUG-48557 */
            IDE_TEST_RAISE( (UChar *)sPoint >= sFence, invalid_error );

            fill2DCoordString( sTempBuf, ID_SIZEOF(sTempBuf), sPoint );
            if( j < sMax -1 )
            {
                IDE_TEST_RAISE( (UInt)(idlVA::appendFormat(
                    sTempBuf, ID_SIZEOF(sTempBuf), ", " ))
                    > ID_SIZEOF(sTempBuf), err_large_object );
            }
                            
            IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
                >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, sTempBuf);
            *aOffset += idlOS::strlen(sTempBuf);
            sPoint = STD_NEXT_PT2D(sPoint);
        } // for j
        IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, ")");
        *aOffset += 1;
        
        if( i < sMaxO -1 )
        {
            IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, ", ");
            *aOffset += 2;
        }
        sLine = (stdLineString2DType*)sPoint;
    } // for i    
    IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, ") ");
    *aOffset += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_MULTILINESTRING_NAME ) );
    }
    IDE_EXCEPTION(err_large_object);
    {
        idlOS::snprintf((SChar*)aBuf,
                        aMaxSize,
                        STD_MULTILINESTRING_NAME"( ... ) ");
        *aOffset = idlOS::strlen((SChar*)aBuf);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMPolygon 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdMultiPolygon2DType*     aMPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPolygonWKT2D(
                    stdMultiPolygon2DType*     aMPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    SChar             sTempBuf[10240];
    stdPolygon2DType* sPolygon;
    stdLinearRing2D*  sRing;
    stdPoint2D*       sPoint;
    UInt              i, j, k, sMaxO, sMaxR, sMax;
    UChar           * sFence;
    
    idlOS::sprintf(sTempBuf, STD_MULTIPOLYGON_NAME"(" );
    IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
        >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, sTempBuf);
    *aOffset += idlOS::strlen(sTempBuf);
    
    sPolygon = STD_FIRST_POLY2D(aMPolygon);
    sMaxO = STD_N_OBJECTS(aMPolygon);
    sFence = (UChar *)aMPolygon + aMPolygon->mSize;

    for(i = 0; i < sMaxO; i++)
    {
        IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, "(");
        *aOffset += 1;

        sRing = STD_FIRST_RN2D(sPolygon);
        sMaxR = STD_N_RINGS(sPolygon);
        for(j = 0; j < sMaxR; j++)
        {
            IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, "(");
            *aOffset += 1;
        
            sPoint = STD_FIRST_PT2D(sRing);
            sMax = STD_N_POINTS(sRing);            
            for( k = 0; k < sMax; k++)
            {
                /* BUG-48557 */
                IDE_TEST_RAISE( (UChar *)sPoint >= sFence, invalid_error );

                fill2DCoordString( sTempBuf, ID_SIZEOF(sTempBuf), sPoint );
                if( k < sMax -1 )
                {
                    IDE_TEST_RAISE( (UInt)(idlVA::appendFormat(
                        sTempBuf, ID_SIZEOF(sTempBuf), ", " ))
                        > ID_SIZEOF(sTempBuf), err_large_object );
                }
                            
                IDE_TEST_RAISE((UInt)((*aOffset) + idlOS::strlen(sTempBuf))
                    >= aMaxSize, err_large_object);
                idlOS::strcat((SChar*)aBuf, sTempBuf);
                *aOffset += idlOS::strlen(sTempBuf);
                sPoint = STD_NEXT_PT2D(sPoint);
            } // for k
            IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, ")");
            *aOffset += 1;
        
            if( j < sMaxR -1 )
            {
                IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, 
                    err_large_object);
                idlOS::strcat((SChar*)aBuf, ", ");
                *aOffset += 2;
            }
            sRing = (stdLinearRing2D*)sPoint;
        } // for j
        IDE_TEST_RAISE((UInt)((*aOffset)+1) >= aMaxSize, err_large_object);
        idlOS::strcat((SChar*)aBuf, ")");
        *aOffset += 1;
        
        if( i < sMaxO -1 )
        {
            IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, ", ");
            *aOffset += 2;
        }

        // BUG-48051 geomFromWKB에서 잘못 생성된 polygon 객체인지 확인
        IDE_TEST_RAISE ( (stdPolygon2DType*)sRing != STD_NEXT_POLY2D(sPolygon),
                         invalid_error );
        sPolygon = (stdPolygon2DType*)sRing;
    } // for i    
    IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, ") ");
    *aOffset += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_MULTIPOLYGON_NAME ) );
    }
    IDE_EXCEPTION(err_large_object);
    {
        idlOS::snprintf((SChar*)aBuf,
                        aMaxSize,
                        STD_MULTIPOLYGON_NAME"( ... ) ");
        *aOffset = idlOS::strlen((SChar*)aBuf);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aCollection 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdGeoCollection2DType*    aCollection(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeGeoCollectionWKT2D(
                    stdGeoCollection2DType*    aCollection,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)
{
    stdGeometryHeader* sGeom;
    UChar*             sBuf = aBuf;
    UInt               i, sGab, sMax;
    UInt               sMaxSize = aMaxSize;    
    UInt               sStartOffset = *aOffset;

    IDE_TEST_RAISE( (UInt)( (*aOffset) + STD_GEOCOLLECTION_NAME_LEN + 2 )
                     >= aMaxSize, err_large_object );
    idlOS::strcat( (SChar*)aBuf, STD_GEOCOLLECTION_NAME"( " );
    *aOffset += STD_GEOCOLLECTION_NAME_LEN + 2;

    sGeom = (stdGeometryHeader*)STD_FIRST_COLL2D(aCollection);
    sMax = STD_N_GEOMS(aCollection);
    for( i = 0; i < sMax; i++)
    {
        switch(sGeom->mType)
        {
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
            IDE_TEST_RAISE( writePointWKT2D(
                (stdPoint2DType*)sGeom,sBuf,sMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            IDE_TEST_RAISE( writeLineStringWKT2D(
                (stdLineString2DType*)sGeom,sBuf,sMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_POLYGON_2D_EXT_TYPE :        
        case STD_POLYGON_2D_TYPE :        
            IDE_TEST_RAISE( writePolygonWKT2D(
                (stdPolygon2DType*)sGeom,sBuf,sMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
            IDE_TEST_RAISE( writeMultiPointWKT2D(
                (stdMultiPoint2DType*)sGeom,sBuf,sMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
            IDE_TEST_RAISE( writeMultiLineStringWKT2D(
                (stdMultiLineString2DType*)sGeom,sBuf,sMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            IDE_TEST_RAISE( writeMultiPolygonWKT2D(
                (stdMultiPolygon2DType*)sGeom,sBuf,sMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
            
        default :
            IDE_RAISE(err_large_object);
        }        
        
        if( i < sMax - 1 )
        {
            IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
            idlOS::strcat((SChar*)aBuf, ", ");
            *aOffset += 2;
        }        
        
        sGeom = (stdGeometryHeader*)STD_NEXT_GEOM(sGeom);
        sGab = *aOffset - sStartOffset;
        
        // Fix BUG-15423
        IDE_TEST_RAISE( sGab >= aMaxSize, err_large_object);
        sBuf = aBuf + sGab;
        sMaxSize = aMaxSize - sGab;
        
    } // for i
    IDE_TEST_RAISE((UInt)((*aOffset)+2) >= aMaxSize, err_large_object);
    idlOS::strcat((SChar*)aBuf, ") ");
    *aOffset += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_large_object);
    {
        if ( ideGetErrorCode() == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB )
        {
            IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_GEOCOLLECTION_NAME ) );
        }
        else
        {
            idlOS::snprintf((SChar*)aBuf,
                            sMaxSize,
                            STD_GEOCOLLECTION_NAME"( ... ) ");
            *aOffset = idlOS::strlen((SChar*)aBuf);
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPoint 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdPoint2DExtType*         aPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePointEWKT2D( stdPoint2DExtType*         aPoint,
                                    UChar*                     aBuf,
                                    UInt                       aMaxSize,
                                    UInt*                      aOffset )
{
    IDE_TEST( writeSRID( aPoint->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );

    IDE_TEST( writePointWKT2D( (stdPoint2DType*) aPoint,
                               aBuf,
                               aMaxSize,
                               aOffset )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * aLine 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdLineString2DType*       aLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeLineStringEWKT2D( stdLineString2DExtType*    aLine,
                                         UChar*                     aBuf,
                                         UInt                       aMaxSize,
                                         UInt*                      aOffset )
{
    IDE_TEST( writeSRID( aLine->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );

    IDE_TEST( writeLineStringWKT2D( (stdLineString2DType*) aLine,
                                    aBuf,
                                    aMaxSize,
                                    aOffset )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPolygon 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdPolygon2DType*          aPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePolygonEWKT2D( stdPolygon2DExtType*       aPolygon,
                                      UChar*                     aBuf,
                                      UInt                       aMaxSize,
                                      UInt*                      aOffset )
{
    IDE_TEST( writeSRID( aPolygon->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );

    IDE_TEST( writePolygonWKT2D( (stdPolygon2DType*) aPolygon,
                                 aBuf,
                                 aMaxSize,
                                 aOffset )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/***********************************************************************
 * Description:
 * aMPoint 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdMultiPoint2DType*       aMPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPointEWKT2D( stdMultiPoint2DExtType*    aMPoint,
                                         UChar*                     aBuf,
                                         UInt                       aMaxSize,
                                         UInt*                      aOffset )
{
    IDE_TEST( writeSRID( aMPoint->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );
    
    IDE_TEST( writeMultiPointWKT2D( (stdMultiPoint2DType*) aMPoint,
                                    aBuf,
                                    aMaxSize,
                                    aOffset )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMLine 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdMultiLineString2DType*  aMLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiLineStringEWKT2D( stdMultiLineString2DExtType* aMLine,
                                              UChar*                       aBuf,
                                              UInt                         aMaxSize,
                                              UInt*                        aOffset )
{
    IDE_TEST( writeSRID( aMLine->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );

    IDE_TEST( writeMultiLineStringWKT2D( (stdMultiLineString2DType*) aMLine,
                                         aBuf,
                                         aMaxSize,
                                         aOffset )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMPolygon 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdMultiPolygon2DType*     aMPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPolygonEWKT2D( stdMultiPolygon2DExtType*  aMPolygon,
                                           UChar*                     aBuf,
                                           UInt                       aMaxSize,
                                           UInt*                      aOffset )
{
    IDE_TEST( writeSRID( aMPolygon->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );

    IDE_TEST( writeMultiPolygonWKT2D( (stdMultiPolygon2DType*) aMPolygon,
                                      aBuf,
                                      aMaxSize,
                                      aOffset )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aCollection 객체를 읽어서 버퍼에 문자열로 출력한다.
 * 
 * stdGeoCollection2DType*    aCollection(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeGeoCollectionEWKT2D( stdGeoCollection2DExtType* aCollection,
                                            UChar*                     aBuf,
                                            UInt                       aMaxSize,
                                            UInt*                      aOffset )
{
    IDE_TEST( writeSRID( aCollection->mSRID,
                         aBuf,
                         aMaxSize,
                         aOffset )
              != IDE_SUCCESS );

    IDE_TEST( writeGeoCollectionWKT2D( (stdGeoCollection2DType*) aCollection,
                                       aBuf,
                                       aMaxSize,
                                       aOffset )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aSRID를 버퍼에 문자열로 출력한다.
 * 
 * SInt                       aSRID(In): 출력할 SRID
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeSRID( SInt     aSRID,
                             UChar*   aBuf,
                             UInt     aMaxSize,
                             UInt*    aOffset )
{
    SChar  sTempBuf[128];

    idlOS::snprintf( sTempBuf,
                     ID_SIZEOF(sTempBuf),
                     STD_SRID_NAME"=%"ID_INT32_FMT";",
                     aSRID );
    // BUG-27685 Valgrind BUG
    // strlen(sting) + 1 > buffer_size because of null termination.
    IDE_TEST_RAISE( (UInt)( (*aOffset) + idlOS::strlen(sTempBuf) ) >= aMaxSize,
                    err_large_object );
    idlOS::strcat( (SChar*)aBuf, sTempBuf );
    *aOffset += idlOS::strlen( sTempBuf );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_large_object );
    {
        idlOS::snprintf( (SChar*)aBuf, aMaxSize, STD_SRID_NAME"=...;" );
        *aOffset = idlOS::strlen( (SChar*)aBuf );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
IDE_RC stdMethod::writeTemporalPointWKT2D(
                    stdTemporalPoint2DType   * aPoint,
                    UChar                    * aBuf,
                    UInt                       aMaxSize,
                    UInt                     * aOffset);
IDE_RC stdMethod::writeTemporalPointWKT3D(
                    stdTemporalPoint3DType   * aPoint,
                    UChar                    * aBuf,
                    UInt                       aMaxSize,
                    UInt                     * aOffset);
IDE_RC stdMethod::writeTemporalLineStringWKT2D(
                    stdTemporalLineString2DType * aLine,
                    UChar                       * aBuf,
                    UInt                          aMaxSize,
                    UInt                        * aOffset);
IDE_RC stdMethod::writeTemporalLineStringWKT3D(
                    stdTemporalLineString3DType * aLine,
                    UChar                       * aBuf,
                    UInt                          aMaxSize,
                    UInt                        * aOffset);
*/

UChar *stdMethod::writeWKB_Char( UChar *aBuf, UChar aVal, UInt *aOffset )
{
    IDE_DASSERT( aBuf != NULL );
    
    UChar *sNext = aBuf + ID_SIZEOF(UChar);
    idlOS::memcpy( aBuf, &aVal, ID_SIZEOF(UChar) );
    if( aOffset )
        *aOffset += ID_SIZEOF(UChar);
    
    return sNext;
}

UChar *stdMethod::writeWKB_UInt( UChar *aBuf, UInt aVal, UInt *aOffset )
{
    IDE_DASSERT( aBuf != NULL );
    
    UChar *sNext = aBuf + WKB_INT32_SIZE;
    idlOS::memcpy( aBuf, &aVal, WKB_INT32_SIZE );
    if( aOffset )
        *aOffset += WKB_INT32_SIZE;
    
    return sNext;
}

UChar *stdMethod::writeWKB_SInt( UChar *aBuf, SInt aVal, UInt *aOffset )
{
    IDE_DASSERT( aBuf != NULL );
    
    UChar *sNext = aBuf + WKB_INT32_SIZE;
    idlOS::memcpy( aBuf, &aVal, WKB_INT32_SIZE );
    if( aOffset )
        *aOffset += WKB_INT32_SIZE;
    
    return sNext;
}

UChar *stdMethod::writeWKB_SDouble( UChar *aBuf, SDouble aVal, UInt *aOffset )
{
    IDE_DASSERT( aBuf != NULL );
    
    UChar *sNext = aBuf + ID_SIZEOF(SDouble);
    idlOS::memcpy( aBuf, &aVal, ID_SIZEOF(SDouble) );
    if( aOffset )
        *aOffset += ID_SIZEOF(SDouble);
    
    return sNext;
}

UChar *stdMethod::writeWKB_Header( UChar * aBuf,
                                   UInt    aType,
                                   UInt  * aOffset )
{
    UChar *sNext = aBuf;

    IDE_DASSERT( aBuf != NULL );
    
   // Byte Order
#ifdef ENDIAN_IS_BIG_ENDIAN
    sNext = writeWKB_Char( sNext, 0, aOffset );
#else
    sNext = writeWKB_Char( sNext, 1, aOffset );
#endif
    
    // Type
    sNext = writeWKB_UInt( sNext, aType, aOffset );
    
    return sNext;
}

UChar *stdMethod::writeWKB_Point2D( UChar            * aBuf,
                                    const stdPoint2D * aPoint,
                                    UInt             * aOffset )
{
    IDE_DASSERT( aBuf != NULL );
    
    UChar *sNext = aBuf;
    
    // X, Y
    sNext = writeWKB_SDouble( sNext, aPoint->mX, aOffset );
    sNext = writeWKB_SDouble( sNext, aPoint->mY, aOffset );

    return sNext;
}


UChar *stdMethod::writeWKB_Point2Ds( UChar            * aBuf,
                                     UInt               aNumPoints,
                                     const stdPoint2D * aPoints,
                                     UInt             * aOffset )
{
    UInt   i;
    UChar *sNext = aBuf;

    IDE_DASSERT( aBuf != NULL );
    
    sNext = writeWKB_UInt( sNext, aNumPoints, aOffset );

    for( i=0; i<aNumPoints; i++ )
    {
        // X, Y
        sNext = writeWKB_SDouble( sNext, aPoints->mX, aOffset );
        sNext = writeWKB_SDouble( sNext, aPoints->mY, aOffset );

        aPoints++;
    }

    
    return sNext;
}


/***********************************************************************
 * Description: 	835	* Description:
 * BUG-32531 Consider for GIS EMPTY
 * aMPoint 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * Empty는 Multi point의 point 갯수가 0 일경우 empty로 처리한다.
 *
 * BUGBUG
 * ASBINARY 로 TYPE 정보를 출력하게 되면 EMPTY의 TYPE은 무조건
 * MULTIPOINT 값인 4로 출력 된다. 추후 WKB HEADER의 TYPE에 EMPTY를
 * 고려 해야 한다.
 *
 * stdMultiPoint2DType* aMPoint(In): 출력할 객체
 * UChar* aBuf(Out): 버퍼
 * UInt aMaxSize(In): 버퍼의 최대값
 * UInt* aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeEmptyWKB2D( stdMultiPoint2DType* /* aMPoint */,
                                   UChar*               aBuf,
                                   UInt                 aMaxSize,
                                   UInt*                aOffset)
{
    WKBMultiPoint* sBMpoint = (WKBMultiPoint*)aBuf;
    UInt sNumPoints         = 0;
    UInt sWKBSize           = WKB_MPOINT_SIZE + WKB_POINT_SIZE*sNumPoints;
    UInt sSize              = 0;
			
    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize,
                   err_large_object);
			
    writeWKB_Header( (UChar*)sBMpoint, WKB_MULTIPOINT_TYPE, &sSize );
    writeWKB_UInt( sBMpoint->mNumWKBPoints, sNumPoints, &sSize );
			
    IDE_DASSERT( sWKBSize == sSize );
			
    *aOffset += sWKBSize;
			
    return IDE_SUCCESS;
			
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
			writeWKB_Header( aBuf, 0, &sSize );
			*aOffset += WKB_GEOHEAD_SIZE;
        }
    }
    IDE_EXCEPTION_END;
			
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPoint 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdPoint2DType*            aPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePointWKB2D(
                    stdPoint2DType*            aPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    WKBPoint*   sBPoint = (WKBPoint*)aBuf;
    UInt        sWKBSize = WKB_POINT_SIZE;
    UInt        sSize    = 0;
    

    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize, 
           err_large_object);

    writeWKB_Header( (UChar*)sBPoint, WKB_POINT_TYPE, &sSize );    
    writeWKB_Point2D( (UChar*)&sBPoint->mPoint, &aPoint->mPoint, &sSize );
    
    IDE_DASSERT( sSize == sWKBSize );
    *aOffset += sSize;
    
    // Fix BUG-15428           
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)((*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aLine 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdLineString2DType*       aLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeLineStringWKB2D(
                    stdLineString2DType*       aLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    stdPoint2D*     sPoint = STD_FIRST_PT2D(aLine);
    WKBLineString*  sBLine = (WKBLineString*)aBuf;
    UInt            sNumPoints = STD_N_POINTS(aLine);
    UInt            sWKBSize = WKB_LINE_SIZE + WKB_PT_SIZE*sNumPoints;
    UInt            sSize  = 0;

    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize, 
           err_large_object);

    writeWKB_Header( (UChar*)sBLine, WKB_LINESTRING_TYPE, &sSize );    
    writeWKB_Point2Ds( sBLine->mNumPoints, sNumPoints, sPoint, &sSize );
    
    IDE_DASSERT( sSize == sWKBSize );
    *aOffset += sSize;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)((*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPolygon 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdPolygon2DType*          aPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePolygonWKB2D(
                    stdPolygon2DType*          aPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPoint;
    WKBPolygon*         sBPolygon = (WKBPolygon*)aBuf;
    wkbLinearRing*      sBRing;    
    UInt                sNumRings = STD_N_RINGS(aPolygon);
    UInt                sWKBSize = WKB_POLY_SIZE;
    UInt                sSize = 0;
    UInt                i,j, sMax;
    
    sRing = STD_FIRST_RN2D(aPolygon);
    for(i = 0; i < sNumRings; i++)
    {
        sWKBSize += WKB_RN_SIZE; // numPoints
        sPoint = STD_FIRST_PT2D(sRing);
        sMax = STD_N_POINTS(sRing);
        for(j = 0; j < sMax; j++)
        {
            sWKBSize += WKB_PT_SIZE; // ID_SIZEOF(wkbPoint);
            sPoint = STD_NEXT_PT2D(sPoint);
        }
        sRing = (stdLinearRing2D*)sPoint;
    }

    // BUG-48051 geomFromWKB에서 잘못 생성된 polygon 객체인지 확인
    IDE_TEST_RAISE( sRing != (stdLinearRing2D *)((UChar*)aPolygon + aPolygon->mSize),
                    invalid_error );

    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize, 
           err_large_object);

    writeWKB_Header( (UChar*)sBPolygon, WKB_POLYGON_TYPE, &sSize );
    writeWKB_UInt( sBPolygon->mNumRings, sNumRings, &sSize );
    
    sRing = STD_FIRST_RN2D(aPolygon);
    sBRing = WKB_FIRST_RN(sBPolygon);
    for(i = 0; i < sNumRings; i++)
    {
        sPoint = STD_FIRST_PT2D(sRing);
        
        sBRing = (wkbLinearRing*)writeWKB_Point2Ds(
            sBRing->mNumPoints, STD_N_POINTS(sRing), sPoint, &sSize );

        sRing = STD_NEXT_RN2D(sRing);
    }

    IDE_DASSERT( sWKBSize == sSize );
    
    *aOffset += sWKBSize;    // Fix BUG-15428
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_POLYGON_NAME ) );
    }
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)((*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMPoint 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdMultiPoint2DType*       aMPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPointWKB2D(
                    stdMultiPoint2DType*       aMPoint,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    stdPoint2DType* sPoint = STD_FIRST_POINT2D(aMPoint);
    WKBMultiPoint*  sBMpoint = (WKBMultiPoint*)aBuf;
    WKBPoint*       sBPoint = WKB_FIRST_POINT(sBMpoint);
    UInt            sNumPoints = STD_N_OBJECTS(aMPoint);
    UInt            sWKBSize = WKB_MPOINT_SIZE + WKB_POINT_SIZE*sNumPoints;
    UInt            sSize = 0;
    UInt            i;

    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize, 
           err_large_object);

    writeWKB_Header( (UChar*)sBMpoint, WKB_MULTIPOINT_TYPE, &sSize );
    writeWKB_UInt( sBMpoint->mNumWKBPoints, sNumPoints, &sSize );
    
    for(i = 0; i < sNumPoints; i++)
    {
        writeWKB_Header( (UChar*)sBPoint, WKB_POINT_TYPE, &sSize );    
        writeWKB_Point2D( (UChar*)&sBPoint->mPoint, &sPoint->mPoint, &sSize );
        
        sPoint = STD_NEXT_POINT2D(sPoint);
        sBPoint = WKB_NEXT_POINT(sBPoint);
    }
           
    IDE_DASSERT( sWKBSize == sSize );
    
    // Fix BUG-15428           
    *aOffset += sWKBSize;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMLine 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdMultiLineString2DType*  aMLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiLineStringWKB2D(
                    stdMultiLineString2DType*  aMLine,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    stdLineString2DType*    sLine;
    stdPoint2D*             sPoint;
    WKBMultiLineString*     sBMLine = (WKBMultiLineString*)aBuf;
    WKBLineString*          sBLine;    
    UInt                    sNumLines = STD_N_OBJECTS(aMLine);
    UInt                    sWKBSize = WKB_MLINE_SIZE;
    UInt                    sSize = 0;
    UInt                    i,j, sMax;
    
    sLine = STD_FIRST_LINE2D(aMLine);
    for(i = 0; i < sNumLines; i++)
    {
        sWKBSize += WKB_LINE_SIZE;
        sPoint = STD_FIRST_PT2D(sLine);
        sMax = STD_N_POINTS(sLine);
        for(j = 0; j < sMax; j++)
        {
            sWKBSize += WKB_PT_SIZE; // point
            sPoint = STD_NEXT_PT2D(sPoint);
        }
        sLine = (stdLineString2DType*)sPoint;
    }

    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize, 
           err_large_object);

    // MultiLineString Header
    writeWKB_Header( (UChar*)sBMLine, WKB_MULTILINESTRING_TYPE, &sSize );    
    writeWKB_UInt( sBMLine->mNumWKBLineStrings, sNumLines, &sSize );

    sLine = STD_FIRST_LINE2D(aMLine);
    sBLine = WKB_FIRST_LINE(sBMLine);
    for(i = 0; i < sNumLines; i++)
    {
        sPoint = STD_FIRST_PT2D(sLine);
        
        // LineString 
        writeWKB_Header( (UChar*)sBLine, WKB_LINESTRING_TYPE, &sSize );        
        sBLine = (WKBLineString*)writeWKB_Point2Ds( 
            sBLine->mNumPoints, STD_N_POINTS(sLine), sPoint, &sSize );
       
        sLine = STD_NEXT_LINE2D(sLine);
    }
           
    IDE_DASSERT( sWKBSize == sSize );
    // Fix BUG-15428           
    *aOffset += sWKBSize;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMPolygon 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdMultiPolygon2DType*     aMPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPolygonWKB2D(
                    stdMultiPolygon2DType*     aMPolygon,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    stdPolygon2DType*   sPolygon;
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPoint;
    WKBMultiPolygon*    sBMPolygon = (WKBMultiPolygon*)aBuf;
    WKBPolygon*         sBPolygon;
    wkbLinearRing*      sBRing;    
    UInt                sNumPolygons = STD_N_OBJECTS(aMPolygon);
    UInt                sWKBSize = WKB_MPOLY_SIZE;
    UInt                sSize = 0;
    UInt                i,j,k, sMaxR, sMax;
    
    sPolygon = STD_FIRST_POLY2D(aMPolygon);
    for(i = 0; i < sNumPolygons; i++)
    {
        sWKBSize += WKB_POLY_SIZE;
        sRing = STD_FIRST_RN2D(sPolygon);
        sMaxR = STD_N_RINGS(sPolygon);
        for(j = 0; j < sMaxR; j++)
        {
            sWKBSize += WKB_RN_SIZE;
            sPoint = STD_FIRST_PT2D(sRing);
            sMax = STD_N_POINTS(sRing);            
            for(k = 0; k < sMax; k++)
            {
                sWKBSize += WKB_PT_SIZE;
                sPoint = STD_NEXT_PT2D(sPoint);
            }
            sRing = (stdLinearRing2D*)sPoint;
        }
        // BUG-48051 geomFromWKB에서 잘못 생성된 polygon 객체인지 확인
        IDE_TEST_RAISE( (stdPolygon2DType*)sRing != STD_NEXT_POLY2D(sPolygon),
                        invalid_error );

        sPolygon = (stdPolygon2DType*)sRing;
    }

    IDE_TEST_RAISE((UInt)((*aOffset) + sWKBSize ) > aMaxSize, 
           err_large_object);

    writeWKB_Header( (UChar*)sBMPolygon, WKB_MULTIPOLYGON_TYPE, &sSize );
    writeWKB_UInt( sBMPolygon->mNumWKBPolygons, sNumPolygons, &sSize );
    
    sPolygon = STD_FIRST_POLY2D(aMPolygon);
    sBPolygon = WKB_FIRST_POLY(sBMPolygon);
    for(i = 0; i < sNumPolygons; i++)
    {
        writeWKB_Header( (UChar*)sBPolygon, WKB_POLYGON_TYPE, &sSize );
        writeWKB_UInt( sBPolygon->mNumRings, sPolygon->mNumRings, &sSize );
        
        sRing = STD_FIRST_RN2D(sPolygon);
        sBRing = WKB_FIRST_RN(sBPolygon);
        sMaxR = STD_N_RINGS(sPolygon);
        for(j = 0; j < sMaxR; j++)
        {
            sPoint = STD_FIRST_PT2D(sRing);
            
            sBRing = (wkbLinearRing*)writeWKB_Point2Ds( 
                sBRing->mNumPoints, STD_N_POINTS(sRing), sPoint, &sSize );
            
            sRing = STD_NEXT_RN2D(sRing);
            
        }
        sPolygon = (stdPolygon2DType*)sRing;
        sBPolygon = (WKBPolygon*)sBRing;
    }

    IDE_DASSERT( sWKBSize == sSize );
    
    // Fix BUG-15428           
    *aOffset += sWKBSize;
    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_MULTIPOLYGON_NAME ) );
    }
    IDE_EXCEPTION(err_large_object)
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aCollection 객체를 읽어서 버퍼에 WKB(Well Known Binary)로 출력한다.
 * 
 * stdGeoCollection2DType*    aCollection(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeGeoCollectionWKB2D(
                    stdGeoCollection2DType*    aCollection,
                    UChar*                     aBuf,
                    UInt                       aMaxSize,
                    UInt*                      aOffset)    // Fix BUG-15834
{
    WKBGeometryCollection*  sBCollection = (WKBGeometryCollection*)aBuf;
    stdGeometryHeader*      sGeom;
    UChar*                  sTraceWKB = aBuf;
    UInt                    sNumGeom = STD_N_GEOMS(aCollection);
    UInt                    sSize = 0;
    UInt                    i, sGab;
    UInt                    sStartOffset = *aOffset;

    IDE_TEST_RAISE((UInt)((*aOffset) + WKB_GEOHEAD_SIZE ) > aMaxSize,
                   err_large_object);
    
    writeWKB_Header( (UChar*)sBCollection, WKB_COLLECTION_TYPE, &sSize );
    writeWKB_UInt( sBCollection->mNumWKBGeometries, 
        STD_N_GEOMS(aCollection), &sSize );
    
    *aOffset += WKB_COLL_SIZE;
    sTraceWKB += WKB_COLL_SIZE;

    sGeom = (stdGeometryHeader*)STD_FIRST_COLL2D(aCollection);
    for( i = 0; i < sNumGeom; i++)
    {
        switch(sGeom->mType)
        {
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
            IDE_TEST_RAISE( writePointWKB2D(
                (stdPoint2DType*)sGeom,sTraceWKB,aMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            IDE_TEST_RAISE( writeLineStringWKB2D(
                (stdLineString2DType*)sGeom,sTraceWKB,aMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :        
            IDE_TEST_RAISE( writePolygonWKB2D(
                (stdPolygon2DType*)sGeom,sTraceWKB,aMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
            IDE_TEST_RAISE( writeMultiPointWKB2D(
                (stdMultiPoint2DType*)sGeom,sTraceWKB,aMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
            IDE_TEST_RAISE( writeMultiLineStringWKB2D(
                (stdMultiLineString2DType*)sGeom,sTraceWKB,aMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            IDE_TEST_RAISE( writeMultiPolygonWKB2D(
                (stdMultiPolygon2DType*)sGeom,sTraceWKB,aMaxSize,aOffset)
                != IDE_SUCCESS, err_large_object);
            break;
        default :
            IDE_RAISE(err_large_object);
        }        
        
        sGeom = (stdGeometryHeader*)STD_NEXT_GEOM(sGeom);
        sGab = *aOffset - sStartOffset;
        
        IDE_TEST_RAISE( sGab > aMaxSize, err_large_object);
        sTraceWKB = aBuf + sGab;
        
    } // for i

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        if ( ideGetErrorCode() == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB )
        {
            IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_GEOCOLLECTION_NAME) );
        }
        else
        {
            if( (UInt)( sStartOffset + WKB_GEOHEAD_SIZE ) < aMaxSize )
            {
                writeWKB_Header( aBuf, 0, &sSize );
                *aOffset = sStartOffset + WKB_GEOHEAD_SIZE;    // Fix BUG-15428
            }
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPoint 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdPoint2DExtType*         aPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePointEWKB2D( stdPoint2DExtType*   aPoint,
                                    UChar*               aBuf,
                                    UInt                 aMaxSize,
                                    UInt*                aOffset )  // Fix BUG-15834
{
    EWKBPoint*  sBPoint = (EWKBPoint*)aBuf;
    UInt        sWKBSize = EWKB_POINT_SIZE;
    UInt        sSize    = 0;

    IDE_TEST_RAISE( (UInt)( (*aOffset) + sWKBSize ) > aMaxSize, 
                    err_large_object );

    writeWKB_Header( (UChar*)sBPoint, EWKB_POINT_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBPoint->mSRID, aPoint->mSRID, &sSize );
    writeWKB_Point2D( (UChar*)&sBPoint->mPoint, &aPoint->mPoint, &sSize );
    
    IDE_DASSERT( sSize == sWKBSize );
    *aOffset += sSize;
    
    // Fix BUG-15428           
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_large_object );
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
        else
        {
            // Nothing To Do
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aLine 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdLineString2DExtType*    aLine(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeLineStringEWKB2D( stdLineString2DExtType* aLine,
                                         UChar*                  aBuf,
                                         UInt                    aMaxSize,
                                         UInt*                   aOffset )  // Fix BUG-15834
{
    stdPoint2D*     sPoint = STD_FIRST_PT2D( aLine );
    EWKBLineString* sBLine = (EWKBLineString*)aBuf;
    UInt            sNumPoints = STD_N_POINTS( aLine );
    UInt            sWKBSize = EWKB_LINE_SIZE + WKB_PT_SIZE*sNumPoints;
    UInt            sSize  = 0;

    IDE_TEST_RAISE( (UInt)( (*aOffset) + sWKBSize ) > aMaxSize, 
                    err_large_object );

    writeWKB_Header( (UChar*)sBLine, EWKB_LINESTRING_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBLine->mSRID, aLine->mSRID, &sSize );
    writeWKB_Point2Ds( sBLine->mNumPoints, sNumPoints, sPoint, &sSize );
    
    IDE_DASSERT( sSize == sWKBSize );
    *aOffset += sSize;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_large_object );
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
        else
        {
            // Nothing To Do
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aPolygon 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdPolygon2DExtType*       aPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writePolygonEWKB2D( stdPolygon2DExtType* aPolygon,
                                      UChar*               aBuf,
                                      UInt                 aMaxSize,
                                      UInt*                aOffset )    // Fix BUG-15834
{
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPoint;
    EWKBPolygon*        sBPolygon = (EWKBPolygon*)aBuf;
    wkbLinearRing*      sBRing;    
    UInt                sNumRings = STD_N_RINGS( aPolygon );
    UInt                sWKBSize = EWKB_POLY_SIZE;
    UInt                sSize = 0;
    UInt                i,j, sMax;
    
    sRing = STD_FIRST_RN2D( aPolygon );
    for ( i = 0; i < sNumRings; i++ )
    {
        sWKBSize += WKB_RN_SIZE; // numPoints
        sPoint = STD_FIRST_PT2D( sRing );
        sMax = STD_N_POINTS( sRing );
        for ( j = 0; j < sMax; j++ )
        {
            sWKBSize += WKB_PT_SIZE; // ID_SIZEOF(wkbPoint);
            sPoint = STD_NEXT_PT2D( sPoint );
        }
        sRing = (stdLinearRing2D*)sPoint;
    }

    // BUG-48051 geomFromWKB에서 잘못 생성된 polygon 객체인지 확인
    IDE_TEST_RAISE( sRing != (stdLinearRing2D *)((UChar*)aPolygon + aPolygon->mSize),
                    invalid_error );

    IDE_TEST_RAISE( (UInt)( (*aOffset) + sWKBSize ) > aMaxSize, 
                    err_large_object );

    writeWKB_Header( (UChar*)sBPolygon, EWKB_POLYGON_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBPolygon->mSRID, aPolygon->mSRID, &sSize );
    writeWKB_UInt( sBPolygon->mNumRings, sNumRings, &sSize );
    
    sRing = STD_FIRST_RN2D( aPolygon );
    sBRing = EWKB_FIRST_RN( sBPolygon );
    for ( i = 0; i < sNumRings; i++ )
    {
        sPoint = STD_FIRST_PT2D( sRing );
        
        sBRing = (wkbLinearRing*)writeWKB_Point2Ds(
            sBRing->mNumPoints, STD_N_POINTS(sRing), sPoint, &sSize );

        sRing = STD_NEXT_RN2D( sRing );
    }

    IDE_DASSERT( sWKBSize == sSize );
    
    *aOffset += sWKBSize;    // Fix BUG-15428
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_POLYGON_NAME ) );
    }
    IDE_EXCEPTION( err_large_object );
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
        else
        {
            // Nothing To Do
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMPoint 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdMultiPoint2DExtType*    aMPoint(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPointEWKB2D( stdMultiPoint2DExtType* aMPoint,
                                         UChar*                  aBuf,
                                         UInt                    aMaxSize,
                                         UInt*                   aOffset )  // Fix BUG-15834
{
    stdPoint2DType* sPoint = STD_FIRST_POINT2D( aMPoint );
    EWKBMultiPoint* sBMpoint = (EWKBMultiPoint*)aBuf;
    WKBPoint*       sBPoint = EWKB_FIRST_POINT( sBMpoint );
    UInt            sNumPoints = STD_N_OBJECTS( aMPoint );
    UInt            sWKBSize = EWKB_MPOINT_SIZE + WKB_POINT_SIZE*sNumPoints;
    UInt            sSize = 0;
    UInt            i;

    IDE_TEST_RAISE( (UInt)( (*aOffset) + sWKBSize ) > aMaxSize, 
                    err_large_object );

    writeWKB_Header( (UChar*)sBMpoint, EWKB_MULTIPOINT_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBMpoint->mSRID, aMPoint->mSRID, &sSize );
    writeWKB_UInt( sBMpoint->mNumWKBPoints, sNumPoints, &sSize );
    
    for ( i = 0; i < sNumPoints; i++ )
    {
        writeWKB_Header( (UChar*)sBPoint, WKB_POINT_TYPE, &sSize );    
        writeWKB_Point2D( (UChar*)&sBPoint->mPoint, &sPoint->mPoint, &sSize );
        
        sPoint = STD_NEXT_POINT2D( sPoint );
        sBPoint = WKB_NEXT_POINT( sBPoint );
    }
           
    IDE_DASSERT( sWKBSize == sSize );
    
    // Fix BUG-15428           
    *aOffset += sWKBSize;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_large_object );
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
        else
        {
            // Nothing To Do
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMLine 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdMultiLineString2DExtType*  aMLine(In): 출력할 객체
 * UChar*                        aBuf(Out): 버퍼
 * UInt                          aMaxSize(In): 버퍼의 최대값 
 * UInt*                         aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiLineStringEWKB2D( stdMultiLineString2DExtType* aMLine,
                                              UChar*                       aBuf,
                                              UInt                         aMaxSize,
                                              UInt*                        aOffset )    // Fix BUG-15834
{
    stdLineString2DType*    sLine;
    stdPoint2D*             sPoint;
    EWKBMultiLineString*    sBMLine = (EWKBMultiLineString*)aBuf;
    WKBLineString*          sBLine;    
    UInt                    sNumLines = STD_N_OBJECTS( aMLine );
    UInt                    sWKBSize = EWKB_MLINE_SIZE;
    UInt                    sSize = 0;
    UInt                    i,j, sMax;
    
    sLine = STD_FIRST_LINE2D( aMLine );
    for ( i = 0; i < sNumLines; i++ )
    {
        sWKBSize += WKB_LINE_SIZE;
        sPoint = STD_FIRST_PT2D( sLine );
        sMax = STD_N_POINTS( sLine );
        for ( j = 0; j < sMax; j++ )
        {
            sWKBSize += WKB_PT_SIZE; // point
            sPoint = STD_NEXT_PT2D( sPoint );
        }
        sLine = (stdLineString2DType*)sPoint;
    }

    IDE_TEST_RAISE( (UInt)( (*aOffset) + sWKBSize ) > aMaxSize, 
                    err_large_object );

    // MultiLineString Header
    writeWKB_Header( (UChar*)sBMLine, EWKB_MULTILINESTRING_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBMLine->mSRID, aMLine->mSRID, &sSize );
    writeWKB_UInt( sBMLine->mNumWKBLineStrings, sNumLines, &sSize );

    sLine = STD_FIRST_LINE2D( aMLine );
    sBLine = EWKB_FIRST_LINE( sBMLine );
    for ( i = 0; i < sNumLines; i++ )
    {
        sPoint = STD_FIRST_PT2D( sLine );
        
        // LineString 
        writeWKB_Header( (UChar*)sBLine, WKB_LINESTRING_TYPE, &sSize );        
        sBLine = (WKBLineString*)writeWKB_Point2Ds( 
            sBLine->mNumPoints, STD_N_POINTS( sLine ), sPoint, &sSize );
       
        sLine = STD_NEXT_LINE2D( sLine );
    }
           
    IDE_DASSERT( sWKBSize == sSize );
    // Fix BUG-15428           
    *aOffset += sWKBSize;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
        else
        {
            // Nothing To Do
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aMPolygon 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdMultiPolygon2DExtType*  aMPolygon(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeMultiPolygonEWKB2D( stdMultiPolygon2DExtType* aMPolygon,
                                           UChar*                    aBuf,
                                           UInt                      aMaxSize,
                                           UInt*                     aOffset ) // Fix BUG-15834
{
    stdPolygon2DType*   sPolygon;
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPoint;
    EWKBMultiPolygon*   sBMPolygon = (EWKBMultiPolygon*)aBuf;
    WKBPolygon*         sBPolygon;
    wkbLinearRing*      sBRing;    
    UInt                sNumPolygons = STD_N_OBJECTS( aMPolygon );
    UInt                sWKBSize = EWKB_MPOLY_SIZE;
    UInt                sSize = 0;
    UInt                i,j,k, sMaxR, sMax;
    
    sPolygon = STD_FIRST_POLY2D( aMPolygon );
    for ( i = 0; i < sNumPolygons; i++ )
    {
        sWKBSize += WKB_POLY_SIZE;
        sRing = STD_FIRST_RN2D( sPolygon );
        sMaxR = STD_N_RINGS( sPolygon );
        for ( j = 0; j < sMaxR; j++ )
        {
            sWKBSize += WKB_RN_SIZE;
            sPoint = STD_FIRST_PT2D( sRing );
            sMax = STD_N_POINTS( sRing );
            for(k = 0; k < sMax; k++)
            {
                sWKBSize += WKB_PT_SIZE;
                sPoint = STD_NEXT_PT2D( sPoint );
            }
            sRing = (stdLinearRing2D*)sPoint;
        }
        // BUG-48051 geomFromWKB에서 잘못 생성된 polygon 객체인지 확인
        IDE_TEST_RAISE ( (stdPolygon2DType*)sRing != STD_NEXT_POLY2D(sPolygon),
                         invalid_error );

        sPolygon = (stdPolygon2DType*)sRing;
    }

    IDE_TEST_RAISE( (UInt)( (*aOffset) + sWKBSize ) > aMaxSize, 
                    err_large_object );

    writeWKB_Header( (UChar*)sBMPolygon, EWKB_MULTIPOLYGON_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBMPolygon->mSRID, aMPolygon->mSRID, &sSize );
    writeWKB_UInt( sBMPolygon->mNumWKBPolygons, sNumPolygons, &sSize );
    
    sPolygon = STD_FIRST_POLY2D( aMPolygon );
    sBPolygon = EWKB_FIRST_POLY( sBMPolygon );
    for ( i = 0; i < sNumPolygons; i++ )
    {
        writeWKB_Header( (UChar*)sBPolygon, WKB_POLYGON_TYPE, &sSize );
        writeWKB_UInt( sBPolygon->mNumRings, sPolygon->mNumRings, &sSize );
        
        sRing = STD_FIRST_RN2D( sPolygon );
        sBRing = WKB_FIRST_RN( sBPolygon );
        sMaxR = STD_N_RINGS( sPolygon );
        for ( j = 0; j < sMaxR; j++ )
        {
            sPoint = STD_FIRST_PT2D( sRing );
            
            sBRing = (wkbLinearRing*)writeWKB_Point2Ds( 
                sBRing->mNumPoints, STD_N_POINTS( sRing ), sPoint, &sSize );
            
            sRing = STD_NEXT_RN2D( sRing );
            
        }
        sPolygon = (stdPolygon2DType*)sRing;
        sBPolygon = (WKBPolygon*)sBRing;
    }

    IDE_DASSERT( sWKBSize == sSize );
    
    // Fix BUG-15428           
    *aOffset += sWKBSize;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( invalid_error )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                  (char *)STD_MULTIPOLYGON_NAME ) );
    }
    IDE_EXCEPTION( err_large_object );
    {
        if( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) < aMaxSize )
        {
            writeWKB_Header( aBuf, 0, &sSize );
            *aOffset  += WKB_GEOHEAD_SIZE;    // Fix BUG-15428
        }
        else
        {
            // Nothing To Do
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * aCollection 객체를 읽어서 버퍼에 EWKB(Extended Well Known Binary)로 출력한다.
 * 
 * stdGeoCollection2DExtType* aCollection(In): 출력할 객체
 * UChar*                     aBuf(Out): 버퍼
 * UInt                       aMaxSize(In): 버퍼의 최대값 
 * UInt*                      aOffset(Out): 출력된 마지막 문자열 위치
 **********************************************************************/
IDE_RC stdMethod::writeGeoCollectionEWKB2D( stdGeoCollection2DExtType* aCollection,
                                            UChar*                     aBuf,
                                            UInt                       aMaxSize,
                                            UInt*                      aOffset )    // Fix BUG-15834
{
    EWKBGeometryCollection* sBCollection = (EWKBGeometryCollection*)aBuf;
    stdGeometryHeader*      sGeom;
    UChar*                  sTraceWKB = aBuf;
    UInt                    sNumGeom = STD_N_GEOMS( aCollection );
    UInt                    sSize = 0;
    UInt                    i, sGab;
    UInt                    sStartOffset = *aOffset;

    IDE_TEST_RAISE( (UInt)( (*aOffset) + WKB_GEOHEAD_SIZE ) > aMaxSize,
                    err_large_object );
    
    writeWKB_Header( (UChar*)sBCollection, EWKB_COLLECTION_TYPE, &sSize );
    writeWKB_SInt( (UChar*)sBCollection->mSRID, aCollection->mSRID, &sSize );
    writeWKB_UInt( sBCollection->mNumWKBGeometries, 
                   STD_N_GEOMS( aCollection ), &sSize );
    
    *aOffset += EWKB_COLL_SIZE;
    sTraceWKB += EWKB_COLL_SIZE;

    sGeom = (stdGeometryHeader*)STD_FIRST_COLL2D( aCollection );
    for ( i = 0; i < sNumGeom; i++ )
    {
        switch( sGeom->mType )
        {
        case STD_POINT_2D_TYPE :
            IDE_TEST_RAISE( writePointWKB2D( (stdPoint2DType*)sGeom,
                                             sTraceWKB,
                                             aMaxSize,
                                             aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_POINT_2D_EXT_TYPE :
            IDE_TEST_RAISE( writePointEWKB2D( (stdPoint2DExtType*)sGeom,
                                              sTraceWKB,
                                              aMaxSize,
                                              aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_LINESTRING_2D_TYPE :
            IDE_TEST_RAISE( writeLineStringWKB2D( (stdLineString2DType*)sGeom,
                                                  sTraceWKB,
                                                  aMaxSize,
                                                  aOffset )
                != IDE_SUCCESS, err_large_object );
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
            IDE_TEST_RAISE( writeLineStringEWKB2D( (stdLineString2DExtType*)sGeom,
                                                   sTraceWKB,
                                                   aMaxSize,
                                                   aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_POLYGON_2D_TYPE :        
            IDE_TEST_RAISE( writePolygonWKB2D( (stdPolygon2DType*)sGeom,
                                               sTraceWKB,
                                               aMaxSize,
                                               aOffset )
                != IDE_SUCCESS, err_large_object );
            break;
        case STD_POLYGON_2D_EXT_TYPE :        
            IDE_TEST_RAISE( writePolygonEWKB2D( (stdPolygon2DExtType*)sGeom,
                                                sTraceWKB,
                                                aMaxSize,
                                                aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_MULTIPOINT_2D_TYPE :
            IDE_TEST_RAISE( writeMultiPointWKB2D( (stdMultiPoint2DType*)sGeom,
                                                  sTraceWKB,
                                                  aMaxSize,
                                                  aOffset )
                != IDE_SUCCESS, err_large_object );
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
            IDE_TEST_RAISE( writeMultiPointEWKB2D( (stdMultiPoint2DExtType*)sGeom,
                                                   sTraceWKB,
                                                   aMaxSize,
                                                   aOffset )
                != IDE_SUCCESS, err_large_object );
            break;
        case STD_MULTILINESTRING_2D_TYPE :
            IDE_TEST_RAISE( writeMultiLineStringWKB2D( (stdMultiLineString2DType*)sGeom,
                                                       sTraceWKB,
                                                       aMaxSize,
                                                       aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
            IDE_TEST_RAISE( writeMultiLineStringEWKB2D( (stdMultiLineString2DExtType*)sGeom,
                                                        sTraceWKB,
                                                        aMaxSize,
                                                        aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_MULTIPOLYGON_2D_TYPE :
            IDE_TEST_RAISE( writeMultiPolygonWKB2D( (stdMultiPolygon2DType*)sGeom,
                                                    sTraceWKB,
                                                    aMaxSize,
                                                    aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
            IDE_TEST_RAISE( writeMultiPolygonEWKB2D( (stdMultiPolygon2DExtType*)sGeom,
                                                     sTraceWKB,
                                                     aMaxSize,
                                                     aOffset )
                            != IDE_SUCCESS, err_large_object );
            break;
        default :
            IDE_RAISE( err_large_object );
        }        
        
        sGeom = (stdGeometryHeader*)STD_NEXT_GEOM(sGeom);
        sGab = *aOffset - sStartOffset;
        
        IDE_TEST_RAISE( sGab > aMaxSize, err_large_object);
        sTraceWKB = aBuf + sGab;
        
    } // for i

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_large_object);
    {
        // BUG-48051
        if ( ideGetErrorCode() == stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB )
        {
            IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY_MADEBY_GEOMFROMWKB,
                                      (char *)STD_GEOCOLLECTION_NAME) );
        }
        else
        {
            if( (UInt)( sStartOffset + WKB_GEOHEAD_SIZE ) < aMaxSize )
            {
                writeWKB_Header( aBuf, 0, &sSize );
                *aOffset = sStartOffset + WKB_GEOHEAD_SIZE;    // Fix BUG-15428
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void stdMethod::fill2DCoordString( SChar      * aBuffer,
                                   SInt         aBufSize,
                                   stdPoint2D * aPoint )
{
    SChar sTemp[100];
    UChar sTempOffset;

    IDE_DASSERT( aBufSize > 100 );

    sTempOffset = 0;

    //--------------------------------
    // 참조) iSQLSpool::PrintWithDouble
    // Coord X 출력
    //--------------------------------
    
    if( ( aPoint->mX < 1E-7 ) && ( aPoint->mX > -1E-7 ) )
    {
        // 0에 가까운 작은 값은 0으로 출력함.
        sTempOffset += idlOS::snprintf( sTemp,
                                        ID_SIZEOF(sTemp),
                                        "0 ");
    }
    else
    {
        sTempOffset += idlOS::snprintf( sTemp,
                                        ID_SIZEOF(sTemp),
                                        "%"ID_FLOAT_G_FMT" ",
                                        aPoint->mX );
    }

    //--------------------------------
    // Coord Y 출력
    //--------------------------------
    
    if( ( aPoint->mY < 1E-7 ) && ( aPoint->mY > -1E-7 ) )
    {
        // 0에 가까운 작은 값은 0으로 출력함.
        sTempOffset += idlOS::snprintf( sTemp + sTempOffset,
                                        ID_SIZEOF(sTemp) - sTempOffset,
                                        "0");
    }
    else
    {
        sTempOffset += idlOS::snprintf( sTemp + sTempOffset,
                                        ID_SIZEOF(sTemp) - sTempOffset,
                                        "%"ID_FLOAT_G_FMT,
                                        aPoint->mY );
    }

    //--------------------------------
    // Fill Buffer
    //--------------------------------

    idlOS::memcpy( aBuffer, sTemp, sTempOffset );
    aBuffer[sTempOffset] = '\0';
}
