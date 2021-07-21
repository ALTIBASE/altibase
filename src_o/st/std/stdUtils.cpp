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
 * $Id: stdUtils.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * Geometry 연산에 필요한 기본 연산자들 구현
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>
#include <iduPriorityQueue.h>

#include <ste.h>
#include <stdTypes.h>
#include <stdUtils.h>
#include <stfRelation.h>
#include <stuProperty.h>

extern mtdModule mtdDouble;
extern mtdModule mtdSmallint;

/***********************************************************************
 * Description:
 * Geometry 객체의 Type을 설정한다.
 *
 * stdGeometryHeader * Geom(Out): Geometry 객체 포인터
 * UShort aType(In): 설정할 Type
 **********************************************************************/
void stdUtils::setType( stdGeometryHeader * aGeom, UShort aType )
{
    aGeom->mType = aType;

    // To Fix BUG-15972
#ifdef ENDIAN_IS_BIG_ENDIAN
    aGeom->mByteOrder = STD_BIG_ENDIAN;
#else
    aGeom->mByteOrder = STD_LITTLE_ENDIAN;
#endif
    // BUG-22924
    aGeom->mIsValid = ST_VALID;
}
 
/***********************************************************************
 * Description:
 *
 *    Geometry 객체로 부터 Type을 읽어온다.
 *
 * stdGeometryHeader * Geom(In): Geometry 객체 포인터
 *
 * Implementation:
 *
 *    Byte Order에 관계없이 일정한 값을 주어야 함
 *
 **********************************************************************/
UShort stdUtils::getType( stdGeometryHeader * aGeom )
{
    UShort sType;
    idBool sEquiEndian;

    sType = aGeom->mType;

    if( isEquiEndian( aGeom, & sEquiEndian ) != IDE_SUCCESS )
    {
        sType = STD_UNKNOWN_TYPE;
    }
    else
    {
        if ( sEquiEndian == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            mtdSmallint.endian( & sType );
        }
    }
    
    return sType;
}

/***********************************************************************
 * Description:
 *
 *   To Fix BUG-15854
 *   클라이언트로부터 전송된 Binary 객체가 Endian이 동일한지를 검사
 *
 **********************************************************************/

IDE_RC stdUtils::isEquiEndian( stdGeometryHeader * aGeom,
                               idBool            * aIsEquiEndian )
{
    UChar  sByteOrder;
    sByteOrder = aGeom->mByteOrder;

    IDE_TEST( compareEndian(sByteOrder, aIsEquiEndian) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-24357 WKB Endian
IDE_RC stdUtils::compareEndian(UChar   aGeomEndian,
                               idBool *aIsEquiEndian)
{
    switch (aGeomEndian)
    {
#ifdef ENDIAN_IS_BIG_ENDIAN

        case STD_BIG_ENDIAN:
            *aIsEquiEndian = ID_TRUE;
            break;
        case STD_LITTLE_ENDIAN:
            *aIsEquiEndian = ID_FALSE;
            break;
            
#else /* ENDIAN_IS_BIG_ENDIAN */
            
        case STD_BIG_ENDIAN:
            *aIsEquiEndian = ID_FALSE;
            break;
        case STD_LITTLE_ENDIAN:
            *aIsEquiEndian = ID_TRUE;
            break;
            
#endif /* ENDIAN_IS_BIG_ENDIAN */
            
        default:
            IDE_RAISE( ERR_INVALID_BYTE_ORDER );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_INVALID_BYTE_ORDER);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_BYTE_ORDER));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 Type을 입력받아 문자열 버퍼에 쓴다.
 *
 * UShort aType(In): Geometry 객체 Type
 * SChar* aTypeText(Out): 문자열 버퍼
 * UShort* aLen(Out): 문자열의 길이
 **********************************************************************/
void stdUtils::getTypeText( UShort aType, SChar* aTypeText, UShort* aLen )
{
    if ( (aTypeText != NULL) && (aLen != NULL) )
    {
        switch(aType)
        {
        // Fix Bug-15416 aLen에 널문자를 제외한 길이를 넣어야한다.
        case STD_EMPTY_TYPE :
            *aLen = STD_EMPTY_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_EMPTY_NAME);
            break;
        case STD_POINT_2D_EXT_TYPE :
        case STD_POINT_2D_TYPE :
            *aLen = STD_POINT_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_POINT_NAME);
            break;
        case STD_LINESTRING_2D_EXT_TYPE :
        case STD_LINESTRING_2D_TYPE :
            *aLen = STD_LINESTRING_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_LINESTRING_NAME);
            break;
        case STD_POLYGON_2D_EXT_TYPE :
        case STD_POLYGON_2D_TYPE :
            *aLen = STD_POLYGON_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_POLYGON_NAME);
            break;
        case STD_MULTIPOINT_2D_EXT_TYPE :
        case STD_MULTIPOINT_2D_TYPE :
            *aLen = STD_MULTIPOINT_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_MULTIPOINT_NAME);
            break;
        case STD_MULTILINESTRING_2D_EXT_TYPE :
        case STD_MULTILINESTRING_2D_TYPE :
            *aLen = STD_MULTILINESTRING_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_MULTILINESTRING_NAME);
            break;            
        case STD_MULTIPOLYGON_2D_EXT_TYPE :
        case STD_MULTIPOLYGON_2D_TYPE :
            *aLen = STD_MULTIPOLYGON_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_MULTIPOLYGON_NAME);
            break;
        case STD_GEOCOLLECTION_2D_EXT_TYPE :
        case STD_GEOCOLLECTION_2D_TYPE :
            *aLen = STD_GEOCOLLECTION_NAME_LEN;
            idlOS::strcpy(aTypeText, STD_GEOCOLLECTION_NAME);
            break;
        default :
            *aLen = 0;
            idlOS::strcpy(aTypeText, "");
            break;
        }
    }
    else
    {
        if ( aTypeText != NULL )
        {
            idlOS::strcpy(aTypeText, "");
        }
        if ( aLen != NULL )
        {
            *aLen = 0;
        }
    }
}


/***********************************************************************
 * Description:
 * 2개의 MBR을 읽어서 MBR1이 MBR2와 Intersect하면 ID_TRUE 아니면 ID_FALSE
 *
 * stdMBR* sMBR1(In): MBR1
 * stdMBR* sMBR2(In): MBR2
 **********************************************************************/
idBool stdUtils::isMBRIntersects( stdMBR* sMBR1, stdMBR* sMBR2 )
{
    if((sMBR1->mMinX > sMBR2->mMaxX) || (sMBR1->mMaxX < sMBR2->mMinX) ||
       (sMBR1->mMinY > sMBR2->mMaxY) || (sMBR1->mMaxY < sMBR2->mMinY) )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

/***********************************************************************
 * Description:
 * 2개의 MBR을 읽어서 MBR1이 MBR2를 Contains하면 ID_TRUE 아니면 ID_FALSE
 *
 * stdMBR* sMBR1(In): MBR1
 * stdMBR* sMBR2(In): MBR2
 **********************************************************************/
idBool stdUtils::isMBRContains( stdMBR* sMBR1, stdMBR* sMBR2 )
{
    if((sMBR1->mMinX <= sMBR2->mMinX) && (sMBR1->mMaxX >= sMBR2->mMaxX) &&
       (sMBR1->mMinY <= sMBR2->mMinY) && (sMBR1->mMaxY >= sMBR2->mMaxY) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * 2개의 MBR을 읽어서 MBR1이 MBR2에 Within하면 ID_TRUE 아니면 ID_FALSE
 *
 * stdMBR* sMBR1(In): MBR1
 * stdMBR* sMBR2(In): MBR2
 **********************************************************************/
idBool stdUtils::isMBRWithin( stdMBR* sMBR1, stdMBR* sMBR2 )
{
    if((sMBR1->mMinX >= sMBR2->mMinX) && (sMBR1->mMaxX <= sMBR2->mMaxX) &&
       (sMBR1->mMinY >= sMBR2->mMinY) && (sMBR1->mMaxY <= sMBR2->mMaxY) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * 2개의 MBR을 읽어서 MBR1이 MBR2와 Equals하면 ID_TRUE 아니면 ID_FALSE
 *
 * stdMBR* sMBR1(In): MBR1
 * stdMBR* sMBR2(In): MBR2
 **********************************************************************/
idBool stdUtils::isMBREquals( stdMBR* sMBR1, stdMBR* sMBR2 )
{
    if((sMBR1->mMinX == sMBR2->mMinX) && (sMBR1->mMaxX == sMBR2->mMaxX) &&
       (sMBR1->mMinY == sMBR2->mMinY) && (sMBR1->mMaxY == sMBR2->mMaxY)  )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * aDst, aSrc의 Geometry 객체를 읽어서 비교한 후 aDst에 기록한다.
 * min~ : 가장 작은 값
 * max~ : 가장 큰 값
 *
 * stdGeometryHeader*  aDst(InOut): 출력이 기록될 Geometry 객체
 * stdGeometryHeader*  aSrc(In): 입력되어 비교되는 Geometry 객체
 **********************************************************************/
IDE_RC stdUtils::mergeMBRFromHeader(stdGeometryHeader*  aDst, 
                                    stdGeometryHeader*  aSrc)
{
    if((mtdDouble.isNull(NULL, &aDst->mMbr.mMaxX)==ID_TRUE) ||
       (mtdDouble.isNull(NULL, &aDst->mMbr.mMaxY)==ID_TRUE) )
    {
        aDst->mMbr.mMinX = aSrc->mMbr.mMinX;
        aDst->mMbr.mMinY = aSrc->mMbr.mMinY;
        aDst->mMbr.mMaxX = aSrc->mMbr.mMaxX;
        aDst->mMbr.mMaxY = aSrc->mMbr.mMaxY;
    }
    else
    {
        aDst->mMbr.mMinX = (aSrc->mMbr.mMinX < aDst->mMbr.mMinX) ?
            aSrc->mMbr.mMinX : aDst->mMbr.mMinX;
        aDst->mMbr.mMinY = (aSrc->mMbr.mMinY < aDst->mMbr.mMinY) ?
            aSrc->mMbr.mMinY : aDst->mMbr.mMinY;
        aDst->mMbr.mMaxX = (aSrc->mMbr.mMaxX > aDst->mMbr.mMaxX) ?
            aSrc->mMbr.mMaxX : aDst->mMbr.mMaxX;
        aDst->mMbr.mMaxY = (aSrc->mMbr.mMaxY > aDst->mMbr.mMaxY) ?
            aSrc->mMbr.mMaxY : aDst->mMbr.mMaxY;
    }
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description:
 * 2차원 Point 값을 읽어서 비교한 후 aDst에 기록한다.
 * min~ : 가장 작은 값
 * max~ : 가장 큰 값
 *
 * stdGeometryHeader*  aDst(InOut): 출력이 기록될 Geometry 객체
 * const stdPoint2D*   aPoint(In): 입력되어 비교되는 2차원 Point
 **********************************************************************/
IDE_RC stdUtils::mergeMBRFromPoint2D(stdGeometryHeader*  aGeom, 
                                     const stdPoint2D*   aPoint)
{
    // Fix BUG-15412 mtdModule.isNull 사용
    if((mtdDouble.isNull(NULL, &aGeom->mMbr.mMaxX)==ID_TRUE) ||
       (mtdDouble.isNull(NULL, &aGeom->mMbr.mMaxY)==ID_TRUE) )
    {
        aGeom->mMbr.mMinX = aGeom->mMbr.mMaxX = aPoint->mX;
        aGeom->mMbr.mMinY = aGeom->mMbr.mMaxY = aPoint->mY;
    }
    else
    {
        aGeom->mMbr.mMinX = (aPoint->mX < aGeom->mMbr.mMinX) ?
            aPoint->mX : aGeom->mMbr.mMinX;
        aGeom->mMbr.mMinY = (aPoint->mY < aGeom->mMbr.mMinY) ?
            aPoint->mY : aGeom->mMbr.mMinY;
        aGeom->mMbr.mMaxX = (aPoint->mX > aGeom->mMbr.mMaxX) ?
            aPoint->mX : aGeom->mMbr.mMaxX;
        aGeom->mMbr.mMaxY = (aPoint->mY > aGeom->mMbr.mMaxY) ?
            aPoint->mY : aGeom->mMbr.mMaxY;
    }
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description:
 * aSrc MBR을 aDst MBR에 복사한다.
 *
 * stdMBR* aDst(Out): 출력
 * stdMBR* aSrc(In): 입력
 **********************************************************************/
void stdUtils::copyMBR(stdMBR* aDst, stdMBR* aSrc)
{
    idlOS::memcpy( aDst, aSrc, STD_MBR_SIZE);
}

/***********************************************************************
 * Description:
 * 입력된 Geometry Type이 유효하면 ID_TRUE 아니면 ID_FALSE
 *
 * UShort aType(In): Geometry Type
 * idBool bIncludeNull(In): Null을 유효하게 볼것인지 아닌지 옵션 설정
 **********************************************************************/
idBool stdUtils::isValidType(UShort aType, idBool bIncludeNull)
{
    if( (aType == STD_NULL_TYPE) || (aType == STD_EMPTY_TYPE) )
    {
        if( bIncludeNull )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }
    
    switch(aType)
    {
    case STD_POINT_2D_EXT_TYPE:
    case STD_POINT_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_LINESTRING_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_POLYGON_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        return ID_TRUE;    
    default:
        return ID_FALSE;    
    }
}

/***********************************************************************
 * Description:
 * 입력된 Geometry Type이 2차원이면 ID_TRUE 아니면 ID_FALSE
 *
 * UShort aType(In): Geometry Type
 **********************************************************************/
idBool stdUtils::is2DType(UShort aType)
{
    switch(aType)
    {
    case STD_POINT_2D_EXT_TYPE:
    case STD_POINT_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_LINESTRING_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_POLYGON_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        return ID_TRUE;    
    default:
        return ID_FALSE;    
    }
}

/***********************************************************************
 * Description:
 * 입력된 Geometry Type이 확장타입이면 ID_TRUE 아니면 ID_FALSE
 *
 * UShort aType(In): Geometry Type
 **********************************************************************/
idBool stdUtils::isExtendedType(UShort aType)
{
    switch(aType)
    {
    case STD_POINT_2D_EXT_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
        return ID_TRUE;    
    default:
        return ID_FALSE;    
    }
}

/***********************************************************************
 * Description:
 * 입력된 Geometry 객체의 차원값을 리턴
 *
 * stdGeometryHeader*  aObj(In): Geometry 객체
 **********************************************************************/
SInt stdUtils::getDimension( stdGeometryHeader*  aObj )
{
    stdGeometryHeader*  sCurr;
    SInt                sResult = -2;
    SInt                sTempDim;
    SInt                sCnt;
    SInt                i;
    
    switch(aObj->mType)
    {
    case STD_EMPTY_TYPE :
        sResult = -1;
        break;
    case STD_POINT_2D_EXT_TYPE :
    case STD_POINT_2D_TYPE :
    case STD_MULTIPOINT_2D_EXT_TYPE :
    case STD_MULTIPOINT_2D_TYPE :
        sResult = 0;
        break;
    case STD_LINESTRING_2D_EXT_TYPE :
    case STD_LINESTRING_2D_TYPE :
    case STD_MULTILINESTRING_2D_EXT_TYPE :
    case STD_MULTILINESTRING_2D_TYPE :
        sResult = 1;
        break;                
    case STD_POLYGON_2D_EXT_TYPE :
    case STD_POLYGON_2D_TYPE :
    case STD_MULTIPOLYGON_2D_EXT_TYPE :
    case STD_MULTIPOLYGON_2D_TYPE :
        sResult = 2;
        break;
    case STD_GEOCOLLECTION_2D_EXT_TYPE :
    case STD_GEOCOLLECTION_2D_TYPE :
        sCnt = getGeometryNum(aObj);
        
        sCurr = getFirstGeometry(aObj);
        if(sCurr == NULL)
        {
            break;
        }

        for( i = 0; i < sCnt; i++ )
        {
            sTempDim = getDimension(sCurr);
            
            if( sResult < sTempDim )
            {
                sResult = sTempDim;
            }
            
            if( sResult == 2 )
            {
                break;
            }
            
            sCurr = getNextGeometry(sCurr);
            
            if ( sCurr == NULL )
            {
                break;
            }
        }
        break;        
    default :
        break;
    }
    return sResult;
}

/***********************************************************************
 * Description:
 * Geometry 객체를 Null 객체로 만든다.
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
void stdUtils::nullify(stdGeometryHeader* aGeom)
{
    idlOS::memcpy(aGeom, &stdGeometryNull, STD_GEOHEAD_SIZE);
}

/***********************************************************************
 * Description:
 * Geometry 객체를 Empty 객체로 만든다.
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
void stdUtils::makeEmpty(stdGeometryHeader* aGeom)
{
    idlOS::memcpy(aGeom, &stdGeometryEmpty, STD_GEOHEAD_SIZE);
}
   
/***********************************************************************
 * Description:
 * Geometry 객체가 Null 객체이면 ID_TRUE 아니면 ID_FALSE
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
idBool stdUtils::isNull(const stdGeometryHeader* aGeom)
{
    // fix BUG-15862 : Geometry타입의 Null검사는 타입만 가지고 비교해야한다.
    if ( aGeom->mType==STD_NULL_TYPE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * Geometry 객체가 Empty 객체이면 ID_TRUE 아니면 ID_FALSE
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
idBool stdUtils::isEmpty(const stdGeometryHeader* aGeom)
{
    // fix BUG-15862 : Geometry타입의 Null검사는 타입만 가지고 비교해야한다.
    if ( aGeom->mType==STD_EMPTY_TYPE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:y
 * aSrc Geometry 헤더 내용을 aDst에 복사한다
 * 
 * stdGeometryHeader*    aDst(Out): 출력될 Geometry 헤더
 * stdGeometryHeader*    aSrc(In): 입력되는 Geometry 헤더
 **********************************************************************/
void stdUtils::makeGeoHeader(
                    stdGeometryHeader*    aDst,
                    stdGeometryHeader*    aSrc )
{
    SInt sSize = STD_GEOHEAD_SIZE;
    idlOS::memcpy(aDst, aSrc, sSize);
    
    aSrc->mSize += sSize;
}

/***********************************************************************
 * Description:
 * Geometry 객체의 개수를 리턴
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
UInt stdUtils::getGeometryNum( stdGeometryHeader* aGeom )
{
    UInt sCnt = 0;
    
    if( aGeom == NULL )
    {
        // Nothing to do.
    }
    else
    {
        stdGeometryType* sGType = (stdGeometryType*)aGeom;
        switch( aGeom->mType )
        {
            case STD_EMPTY_TYPE:
                break;
            case STD_POINT_2D_EXT_TYPE:
            case STD_POINT_2D_TYPE:
            case STD_LINESTRING_2D_EXT_TYPE:
            case STD_LINESTRING_2D_TYPE:
            case STD_POLYGON_2D_EXT_TYPE:
            case STD_POLYGON_2D_TYPE:
                sCnt = 1;
                break;
            case STD_MULTIPOINT_2D_EXT_TYPE:
            case STD_MULTIPOINT_2D_TYPE:
                sCnt = sGType->mpoint2D.mNumObjects;
                break;
            case STD_MULTILINESTRING_2D_EXT_TYPE:
            case STD_MULTILINESTRING_2D_TYPE:
                sCnt =  sGType->mlinestring2D.mNumObjects;
                break;
            case STD_MULTIPOLYGON_2D_EXT_TYPE:
            case STD_MULTIPOLYGON_2D_TYPE:
                sCnt =  sGType->mpolygon2D.mNumObjects;
                break;
            case STD_GEOCOLLECTION_2D_EXT_TYPE:
            case STD_GEOCOLLECTION_2D_TYPE:
                sCnt =  sGType->collection2D.mNumGeometries;
                break;
            default:
                break;
        }
    }
    
    return sCnt;
}

/***********************************************************************
 * Description:
 * 단순 Geometry 객체이면 그 객체를, 복합 Geometry 객체이면 첫번째 객체를 리턴
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
stdGeometryHeader* stdUtils::getFirstGeometry( stdGeometryHeader* aGeom )
{
    if( aGeom == NULL )
    {
        return NULL;
    }
    
    switch( aGeom->mType )
    {
    case STD_POINT_2D_EXT_TYPE:
    case STD_POINT_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_LINESTRING_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_POLYGON_2D_TYPE:
        return aGeom;
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
        return (stdGeometryHeader*)((UChar*)aGeom + STD_MPOINT2D_SIZE);
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
        return (stdGeometryHeader*)((UChar*)aGeom + STD_MLINE2D_SIZE);
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
        return (stdGeometryHeader*)((UChar*)aGeom + STD_MPOLY2D_SIZE);
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        return (stdGeometryHeader*)((UChar*)aGeom + STD_COLL2D_SIZE);
    default:
        break;
    }
    return NULL;
}

/***********************************************************************
 * Description:
 * 현재 Geometry 객체의 다음 객체를 리턴한다.
 * 단순 Geometry 객체이면 Null을, 복합 Geometry 객체이면 다음 객체를 가져온다.
 * 이 함수는 자체적으로 마지막 객체를 판단할 수 없으므로 이 함수의 외부에서 
 * getGeometryNum()을 이용하여 Fence를 체크해야한다.
 *
 * stdGeometryHeader* aGeom(In): Geometry 객체
 **********************************************************************/
stdGeometryHeader* stdUtils::getNextGeometry( stdGeometryHeader* aGeom )
{
    if( aGeom == NULL )
    {
        return NULL;
    }
    switch( aGeom->mType )
    {
    case STD_POINT_2D_EXT_TYPE:
    case STD_POINT_2D_TYPE:
    case STD_LINESTRING_2D_EXT_TYPE:
    case STD_LINESTRING_2D_TYPE:
    case STD_POLYGON_2D_EXT_TYPE:
    case STD_POLYGON_2D_TYPE:
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        return (stdGeometryHeader*)STD_NEXT_GEOM(aGeom);
    default:
        break;
    }
    return NULL;
}

/***********************************************************************
 * Description:
 * 현재 stdPoint2D와 좌표값이 중복되지 않는 다음 stdPoint2D를 리턴한다.
 * 이 함수는 mNumPoints를 갖는 개체인  stdLinearRing과 stdLineString에서
 * 사용할 수 있다.
 *
 * stdPoint2D* aPt: stdPoint2D 배열의 시작 위치
 * UInt aCurr: 현재 stdPoint2D의 위치
 * UInt aSize: 배열 인자의 개수
 **********************************************************************/
stdPoint2D* stdUtils::findNextPointInRing2D(
                            stdPoint2D* aPt,
                            UInt        aCurr,
                            UInt        aSize,
                            UInt      * aNext )
{
    stdPoint2D* sRet;
    SInt        i = 0, sRight = (SInt)(aSize-aCurr);
    
    do
    {
        i++;
        if( i >= sRight)
        {
            i -= aSize;
        }
        
        sRet = STD_NEXTN_PT2D(aPt,i);
    }
    while( (i != 0) && (isSamePoints2D(aPt, sRet)==ID_TRUE));

    if( aNext != NULL )
    {
        *aNext = aCurr + i;
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 현재 stdPoint2D와 좌표값이 중복되지 않는 이전 stdPoint2D를 리턴한다.
 * 이 함수는 mNumPoints를 갖는 개체인  stdLinearRing과 stdLineString에서
 * 사용할 수 있다.
 *
 * stdPoint2D* aPt: stdPoint2D 배열의 시작 위치
 * UInt aCurr: 현재 stdPoint2D의 위치
 * UInt aSize: 배열 인자의 개수
 **********************************************************************/
stdPoint2D* stdUtils::findPrevPointInRing2D(
                            stdPoint2D* aPt,
                            UInt        aCurr,
                            UInt        aSize,
                            UInt      * aNext )
{
    stdPoint2D* sRet;
    SInt        i = 0, sLeft = (SInt)(-aCurr);
    
    do
    {
        i--;
        if( i < sLeft)
        {
            i += aSize;
        }
        
        sRet = STD_NEXTN_PT2D(aPt,i);
    }
    while( (i != 0) && (isSamePoints2D(aPt, sRet)==ID_TRUE));

    if( aNext != NULL )
    {
        *aNext = aCurr + i;
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 현재 stdPoint2D와 좌표값이 중복되지 않는 다음 stdPoint2D를 리턴한다.
 * 이 함수는 자체적으로 마지막 stdPoint2D를 판단할 수 없으므로 이 함수의 외부에서 
 * mNumPoints를 이용하여 Fence를 체크해야한다.
 * 이 함수는 mNumPoints를 갖는 개체인  stdLinearRing과 stdLineString에서
 * 사용할 수 있다.
 *
 * stdPoint2D* aPt(In): 현재 stdPoint2D
 **********************************************************************/
stdPoint2D* stdUtils::findNextPointInLine2D(
                                 stdPoint2D * aPt,
                                 stdPoint2D * aFence )
{
    stdPoint2D* sRet = STD_NEXT_PT2D(aPt);

    while( (sRet < aFence) && (isSamePoints2D(aPt, sRet) == ID_TRUE) )
    {
        sRet = STD_NEXT_PT2D(sRet);
    }

    if( sRet >= aFence )
    {
        return NULL;
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 현재 stdPoint2D와 좌표값이 중복되지 않는 이전 stdPoint2D를 리턴한다.
 * 이 함수는 자체적으로 마지막 stdPoint2D를 판단할 수 없으므로 이 함수의 외부에서 
 * mNumPoints를 이용하여 Fence를 체크해야한다.
 * 이 함수는 mNumPoints를 갖는 개체인  stdLinearRing과 stdLineString에서
 * 사용할 수 있다.
 *
 * stdPoint2D* aPt(In): 현재 stdPoint2D
 **********************************************************************/
stdPoint2D* stdUtils::findPrevPointInLine2D(
                                 stdPoint2D * aPt,
                                 stdPoint2D * aFence )
{
    stdPoint2D* sRet = STD_PREV_PT2D(aPt);
 
    while( (sRet > aFence) && (isSamePoints2D(aPt, sRet) == ID_TRUE) )
    {
        sRet = STD_PREV_PT2D(sRet);
    }
    if( sRet <= aFence )
    {
        return NULL;
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * aPt2를 중점으로 하는 각을 구한다.
 *
 * stdPoint2D* aPt1(In): 시작점
 * stdPoint2D* aPt2(In): 중간점
 * stdPoint2D* aPt3(In): 끝점
 **********************************************************************/
SDouble stdUtils::getAngle2D(
                        stdPoint2D*     aPt1,
                        stdPoint2D*     aPt2,
                        stdPoint2D*     aPt3 )
{
    stdPoint2D  sBA;
    stdPoint2D  sBC;
    SDouble     sCosX;
    SDouble     sRadian;
    
    // fix BUG-16388
    if( isSamePoints2D( aPt1, aPt3 )==ID_TRUE )
    {
        sRadian = 0.0;
    }
    else
    {
        subVec2D( aPt1, aPt2, &sBA );
        subVec2D( aPt3, aPt2, &sBC );
    
        sCosX = dot2D( &sBA, &sBC ) / 
            ( idlOS::sqrt(dot2D( &sBA, &sBA )) * idlOS::sqrt(dot2D( &sBC, &sBC )) );
        
    
        sRadian = idlOS::acos(sCosX);
    }
    
    return sRadian;
}

/***********************************************************************
 * Description:
 * 두점이 같은 점이면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 **********************************************************************/
idBool stdUtils::isSamePoints2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2 )
{
    if((aP1->mX == aP2->mX) && (aP1->mY == aP2->mY))
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * aPoint가 aLine의 끝 점이면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * const stdPoint2D* aPoint(In): 비교될 점
 * const stdLineString2DType* aLine(In): 비교될 라인
 **********************************************************************/
idBool stdUtils::isEndPoint2D(
                    const stdPoint2D*               aPoint,
                    const stdLineString2DType*      aLine )
{
    if( STD_N_POINTS(aLine) <= 0)
    {
        return ID_FALSE;
    }

    stdPoint2D* sPt = STD_FIRST_PT2D(aLine);
    if(isSamePoints2D(aPoint,sPt)==ID_TRUE)
    {
        return ID_TRUE;
    }
    
    sPt = STD_LAST_PT2D(aLine);
    if(isSamePoints2D(aPoint,sPt)==ID_TRUE)
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/***********************************************************************
 * Description:
 * 입력되는 Line 객체가 닫혀있으면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * stdGeometryHeader*    aObj(In): Line 객체
 **********************************************************************/
idBool stdUtils::isClosed2D( stdGeometryHeader*    aObj )
{
    stdLineString2DType*        sLine2D;
    stdMultiLineString2DType*   sMLine2D;
    stdPoint2D*                 sPtFirst2D;
    stdPoint2D*                 sPtPrev2D;
    stdPoint2D*                 sPtCurr2D;
    UInt                        i, sMaxO;

    if( ( aObj->mType == STD_LINESTRING_2D_TYPE ) ||
        ( aObj->mType == STD_LINESTRING_2D_EXT_TYPE ) )
    {
        sLine2D = (stdLineString2DType*)aObj;
        sPtFirst2D = STD_FIRST_PT2D(sLine2D);
        sPtPrev2D = STD_LAST_PT2D(sLine2D);        
        if( stdUtils::isSamePoints2D(sPtFirst2D, sPtPrev2D) == ID_TRUE )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }
    else if( ( aObj->mType == STD_MULTILINESTRING_2D_TYPE ) ||
             ( aObj->mType == STD_MULTILINESTRING_2D_EXT_TYPE ) ) 
    {
        sMLine2D = (stdMultiLineString2DType*)aObj;
        sMaxO = STD_N_OBJECTS(sMLine2D);
        if(sMaxO <= 0)
        {
            return ID_FALSE;
        }
        
        sLine2D = STD_FIRST_LINE2D(sMLine2D);
        sPtFirst2D = STD_FIRST_PT2D(sLine2D);
        sPtPrev2D = STD_LAST_PT2D(sLine2D);
        if( sMaxO == 1 )
        {
            if(stdUtils::isSamePoints2D(sPtFirst2D, sPtPrev2D) == ID_TRUE)
            {
                return ID_TRUE;
            }
            else
            {
                return ID_FALSE;
            }
        }
        else
        {
            sLine2D = (stdLineString2DType*)STD_NEXT_PT2D(sPtPrev2D);
            for( i = 1; i < sMaxO; i++ )
            {
                sPtCurr2D = STD_FIRST_PT2D(sLine2D);
                if(stdUtils::isSamePoints2D(sPtPrev2D, sPtCurr2D) != ID_TRUE)
                {
                    return ID_FALSE;
                }
                sPtPrev2D = STD_LAST_PT2D(sLine2D);
                sLine2D = (stdLineString2DType*)STD_NEXT_PT2D(sPtPrev2D);
            }
            if(stdUtils::isSamePoints2D(sPtPrev2D, sPtFirst2D) != ID_TRUE)
            {
                return ID_FALSE;
            }
            return ID_TRUE;
        }
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * aPoint가 입력되는 Line 객체의 Closed된 포인트이면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * const stdPoint2D* aPoint(In): 비교될 점
 * const stdLineString2DType* aLine(In): 비교될 라인
 **********************************************************************/
idBool stdUtils::isClosedPoint2D(
                    const stdPoint2D*               aPoint,
                    stdGeometryHeader*              aObj )
{
    stdLineString2DType*    sLine;
    stdPoint2D*             sPt;
    UInt                    i, sMax, sClosedCnt;
    if( stdUtils::getGroup(aObj) != STD_LINE_2D_GROUP )
    {
        return ID_FALSE;
    }
    
    sClosedCnt = 0;
    sMax = stdUtils::getGeometryNum(aObj);
    sLine = (stdLineString2DType*)stdUtils::getFirstGeometry(
                                        (stdGeometryHeader*)aObj);
    if( sLine == NULL)
    {
        return ID_FALSE;
    }
    
    for( i = 0; i < sMax; i++ )
    {
        if ( sLine == NULL )
        {
            break;
        }
        else
        {
            //nothing to do
        }

        sPt = STD_FIRST_PT2D(sLine);
        
        if( stdUtils::isSamePoints2D(aPoint, sPt) == ID_TRUE )
        {
            sClosedCnt++;
        }

        sPt = STD_LAST_PT2D(sLine);
        
        if( stdUtils::isSamePoints2D(aPoint, sPt) == ID_TRUE )
        {
            sClosedCnt++;
        }
    
        sLine = (stdLineString2DType*)stdUtils::getNextGeometry(
                                            (stdGeometryHeader*)sLine);
    }
    
    if( sClosedCnt > 1 )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***********************************************************************
 * Description:
 * 두 벡터의 내적
 *
 * const stdPoint2D* aP1(In): 벡터1
 * const stdPoint2D* aP2(In): 벡터2
 **********************************************************************/
SDouble stdUtils::dot2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2 )
{
    return ( ( aP1->mX * aP2->mX ) + ( aP1->mY * aP2->mY ) );    
}

/***********************************************************************
 * Description:
 * 한 벡터의 내적( = 길이의 제곱)
 *
 * const stdPoint2D* aP(In): 벡터
 **********************************************************************/
SDouble stdUtils::length2D( const stdPoint2D *aP )
{
    return dot2D( aP, aP );
}

/***********************************************************************
 * Description:
 * 두 벡터의 차 벡터3 = 벡터1 - 벡터2
 *
 * const stdPoint2D* aP1(In): 벡터1
 * const stdPoint2D* aP2(In): 벡터2
 * stdPoint2D *aP3(Out): 벡터3
 **********************************************************************/
void stdUtils::subVec2D(
                    const stdPoint2D*     aP1,
                    const stdPoint2D*     aP2,
                    stdPoint2D*           aP3 )
{
    aP3->mX = aP1->mX - aP2->mX;
    aP3->mY = aP1->mY - aP2->mY;    
}

//==============================================================================
// TASK-2015 선분을 기준으로 점의 좌우 판별
//==============================================================================

/***********************************************************************
 * Description:
 * 점1 점2의 선분과 점3와의 관계
 * 결과 값 > 0 선분의 왼쪽
 * 결과 값 = 0 선분과 같은 직선상에 존재
 * 결과 값 < 0 선분의 오른쪽
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
SDouble stdUtils::area2D( 
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    SDouble sArea;
    
    // Fix BUG-16388
    if( (isSamePoints2D( aP1, aP3 )==ID_TRUE) ||
        (isSamePoints2D( aP1, aP2 )==ID_TRUE) ||
        (isSamePoints2D( aP2, aP3 )==ID_TRUE) )
    {
        sArea = 0.0;
    }
    else
    {
        sArea = ( ( aP1->mX * aP2->mY ) - ( aP1->mY * aP2->mX ) +
                  ( aP1->mY * aP3->mX ) - ( aP1->mX * aP3->mY ) +
                  ( aP2->mX * aP3->mY ) - ( aP3->mX * aP2->mY ) );
    }
    return sArea;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 왼쪽에 점3이 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::left2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    if( (area2D( aP1, aP2, aP3 ) > 0 ) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 왼쪽에 점3이 있거나 선분 위에 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::leftOn2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    if( ( area2D( aP1, aP2, aP3 ) >= 0 ) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 오른쪽에 점3이 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::right2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    
    if( area2D( aP1, aP2, aP3 ) < 0 ) 
    {
        return ID_TRUE;
    }
    return ID_FALSE;    
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 오른쪽에 점3이 있거나 선분 위에 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::rightOn2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    if( area2D( aP1, aP2, aP3 ) <= 0 )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}


/***********************************************************************
 * Description:
 * 세점이 한 직선 위에 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::collinear2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    if( area2D( aP1, aP2, aP3 ) == 0 )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/***********************************************************************
 * Description:
 * 네점이 한 직선 위에 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 * const stdPoint2D* aP4(In): 점4
 **********************************************************************/
idBool stdUtils::quadLinear2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3,
                        const stdPoint2D*   aP4 )
{
    if( (area2D( aP1, aP2, aP3 ) == 0) && (area2D( aP1, aP2, aP4 ) == 0) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

//==============================================================================
// TASK-2015 선분에 점이 포함되는지 판별
//==============================================================================

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 내부에 점3이 있으면 ID_TRUE 아니면 ID_FALSE
 * 선분 끝에 점3이 있을때 제외
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::betweenI2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    stdPoint2D sAB;
    stdPoint2D sAC;
    idBool     sRet;

    // fix BUG-16388
    if( (isSamePoints2D( aP1, aP3 )==ID_TRUE) ||
        (isSamePoints2D( aP2, aP3 )==ID_TRUE) )
    {
        sRet = ID_FALSE;
    }
    else
    {
        if( collinear2D( aP1, aP2, aP3 ) == ID_FALSE )
        {
            sRet =  ID_FALSE;
        }
        else
        {
            subVec2D( aP2, aP1, &sAB );
            subVec2D( aP3, aP1, &sAC );

            if ( ( dot2D( &sAC, &sAB ) > 0 ) &&
                 ( length2D( &sAC ) < length2D( &sAB ) ) )
            {
                sRet = ID_TRUE;
            }
            else
            {
                sRet = ID_FALSE;
            }
        }
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 내부에 점3이 있으면 ID_TRUE 아니면 ID_FALSE
 * 선분 끝에 점3이 있을때 OK
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::between2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    stdPoint2D sAB;
    stdPoint2D sAC;
    idBool     sRet;
    
    // fix BUG-16388
    if( (isSamePoints2D( aP1, aP3 )==ID_TRUE) ||
        (isSamePoints2D( aP2, aP3 )==ID_TRUE) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        if( collinear2D( aP1, aP2, aP3 ) == ID_FALSE )
        {
            sRet = ID_FALSE;
        }
        else
        {
            subVec2D( aP2, aP1, &sAB );
            subVec2D( aP3, aP1, &sAC );

            if ( ( dot2D( &sAC, &sAB ) >= 0 ) &&
                 ( length2D( &sAC ) <= length2D( &sAB ) ) )
            {
                sRet =  ID_TRUE;
            }
            else
            {
                sRet = ID_FALSE;
            }
        }
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 내부에 점3이 있으면 ID_TRUE 아니면 ID_FALSE
 * 점1과 점3이 같을때 OK
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::betweenOnLeft2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    stdPoint2D sAB;
    stdPoint2D sAC;
    idBool     sRet;
    
    // fix BUG-16388
    if( (isSamePoints2D( aP1, aP3 )==ID_TRUE) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        if( collinear2D( aP1, aP2, aP3 ) == ID_FALSE )
        {
            sRet = ID_FALSE;
        }
        else
        {
            subVec2D( aP2, aP1, &sAB );
            subVec2D( aP3, aP1, &sAC );

            if ( ( dot2D( &sAC, &sAB ) >= 0 ) &&
                 ( length2D( &sAC ) < length2D( &sAB ) ) )
            {
                sRet =  ID_TRUE;
            }
            else
            {
                sRet = ID_FALSE;
            }
        }
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 내부에 점3이 있으면 ID_TRUE 아니면 ID_FALSE
 * 점2와 점3이 같을때 OK
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 **********************************************************************/
idBool stdUtils::betweenOnRight2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    stdPoint2D sAB;
    stdPoint2D sAC;
    idBool     sRet;
    
    // fix BUG-16388
    if( (isSamePoints2D( aP2, aP3 )==ID_TRUE) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        if( collinear2D( aP1, aP2, aP3 ) == ID_FALSE )
        {
            sRet = ID_FALSE;
        }
        else
        {
            subVec2D( aP2, aP1, &sAB );
            subVec2D( aP3, aP1, &sAC );

            if ( ( dot2D( &sAC, &sAB ) > 0 ) &&
                 ( length2D( &sAC ) <= length2D( &sAB ) ) )
            {
                sRet =  ID_TRUE;
            }
            else
            {
                sRet = ID_FALSE;
            }
        }
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 점1 점2의 선분의 내부에 점3이 있으면 0 좌측이면 -1 우측이면 +1 
 * 선분 끝에 점3이 있을때 0
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 * UInt*             aPos(Out): 선분내의 점3의 위치값.
 **********************************************************************/
SInt stdUtils::sector2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3,
                        SDouble*            aPos )
{
    stdPoint2D sAB;
    stdPoint2D sAC;

    subVec2D( aP2, aP1, &sAB );
    subVec2D( aP3, aP1, &sAC );
    
    *aPos = length2D( &sAC );
    
    if( dot2D( &sAC, &sAB ) < 0 )
    {
        return -1;
    }

    if( *aPos > length2D( &sAB ) )
    {
        return 1;
    }
    return 0;
}

//==============================================================================
// TASK-2015 선분이 중첩되는지 판별
//==============================================================================

/***********************************************************************
 * Description:
 * 점1 점2 점3이 한 직선에 있고 점2에서 되돌아 온다. 
 * 즉 선분이 중첩되면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D* aP1(In): 시작점
 * const stdPoint2D* aP2(In): 중간점
 * const stdPoint2D* aP3(In): 끝점
 **********************************************************************/
idBool stdUtils::isReturned2D(
                        const stdPoint2D*   aP1,
                        const stdPoint2D*   aP2,
                        const stdPoint2D*   aP3 )
{
    stdPoint2D sAB;
    stdPoint2D sBC;

    if( collinear2D( aP1, aP2, aP3 ) == ID_FALSE )
    {
        return ID_FALSE;
    }

    subVec2D( aP2, aP1, &sAB );
    subVec2D( aP3, aP2, &sBC );

    if ( dot2D( &sBC, &sAB ) < 0 )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}


//==============================================================================
// TASK-2015 두 선분이 교차하는지 판별
//==============================================================================
/***********************************************************************
 * Description:
 *    To Fix BUG-21196
 *    선분12와 선분34의 MBR 영역이 교차하는 지를 판단.
 *    MBR 이 교차하지 않는다면 선분이 교차하지 않음을 보장할 수 있다.
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 * const stdPoint2D* aP4(In): 점4
 **********************************************************************/

idBool stdUtils::intersectLineMBR2D( const stdPoint2D* aP1,
                                     const stdPoint2D* aP2,
                                     const stdPoint2D* aP3,
                                     const stdPoint2D* aP4 )
{
    idBool sResult;
    
    SDouble sLine1MinX = ( aP1->mX < aP2->mX ) ? aP1->mX : aP2->mX;
    SDouble sLine1MaxX = ( aP1->mX > aP2->mX ) ? aP1->mX : aP2->mX;
    SDouble sLine1MinY = ( aP1->mY < aP2->mY ) ? aP1->mY : aP2->mY;
    SDouble sLine1MaxY = ( aP1->mY > aP2->mY ) ? aP1->mY : aP2->mY;
    
    SDouble sLine2MinX = ( aP3->mX < aP4->mX ) ? aP3->mX : aP4->mX;
    SDouble sLine2MaxX = ( aP3->mX > aP4->mX ) ? aP3->mX : aP4->mX;
    SDouble sLine2MinY = ( aP3->mY < aP4->mY ) ? aP3->mY : aP4->mY;
    SDouble sLine2MaxY = ( aP3->mY > aP4->mY ) ? aP3->mY : aP4->mY;

    if ( (sLine1MinX > sLine2MaxX) || (sLine1MaxX < sLine2MinX ) ||
         (sLine1MinY > sLine2MaxY) || (sLine1MaxY < sLine2MinY ) )
    {
        sResult = ID_FALSE;
    }
    else
    {
        sResult = ID_TRUE;
    }

    return sResult;
}
             

/***********************************************************************
 * Description:
 * 선분12와 선분34가 교차하면 ID_TRUE 아니면 ID_FALSE
 * 선분 끝에서 만나는 것 제외
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 * const stdPoint2D* aP4(In): 점4
 **********************************************************************/
idBool stdUtils::intersectI2D(
                            const stdPoint2D*   aP1,
                            const stdPoint2D*   aP2,
                            const stdPoint2D*   aP3,
                            const stdPoint2D*   aP4 )
{
    // To Fix BUG-21196 (Description 참조)
    // 선분이 교차하는지 검사하기 전에 MBR 영역이 교차하는 지를 먼저 판단한다.
    // 그렇지 않을 경우 연산 정밀도로 인해 선분이 거의 일직선에 가까울 경우 문제가 된다.
    if ( intersectLineMBR2D( aP1, aP2, aP3, aP4 ) == ID_FALSE )
    {
        return ID_FALSE;
    }
    
    if( (collinear2D( aP1, aP2, aP3 ) == ID_TRUE) ||
        (collinear2D( aP1, aP2, aP4 ) == ID_TRUE) ||
        (collinear2D( aP3, aP4, aP1 ) == ID_TRUE) ||
        (collinear2D( aP3, aP4, aP2 ) == ID_TRUE) )
    {
        return ID_FALSE;
    }

    // BUG-22582
    if( ST_XOR( left2D( aP1, aP2, aP3 ), left2D( aP1, aP2, aP4 ) ) == ID_TRUE &&
        ST_XOR( left2D( aP3, aP4, aP1 ), left2D( aP3, aP4, aP2 ) ) == ID_TRUE )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/***********************************************************************
 * Description:
 * 선분12와 선분34가 교차하면 ID_TRUE 아니면 ID_FALSE
 * 선분 끝에서 만나도 OK
 *
 * const stdPoint2D* aP1(In): 점1
 * const stdPoint2D* aP2(In): 점2
 * const stdPoint2D* aP3(In): 점3
 * const stdPoint2D* aP4(In): 점4
 **********************************************************************/
idBool stdUtils::intersect2D(
                            const stdPoint2D*   aP1,
                            const stdPoint2D*   aP2,
                            const stdPoint2D*   aP3,
                            const stdPoint2D*   aP4 )
{
    // Fix BUG-22558
    if ( intersectLineMBR2D( aP1, aP2, aP3, aP4 ) == ID_TRUE )
    {
        if( (intersectI2D( aP1, aP2, aP3, aP4 ) == ID_TRUE ) ||
            (between2D( aP1, aP2, aP3 ) == ID_TRUE) ||
            (between2D( aP1, aP2, aP4 ) == ID_TRUE) ||
            (between2D( aP3, aP4, aP1 ) == ID_TRUE) ||
            (between2D( aP3, aP4, aP2 ) == ID_TRUE) )
        {
            return ID_TRUE;
        }
    }
    return ID_FALSE;    
}

/***********************************************************************
 * Description:
 * 연속되는 3점으로 이루어진 선분 A123 과 선분 B123이 
 * 교차하면                             STD_STAT_INT
 * 떨어져 있으면                      STD_STAT_DIS
 * 접촉하면                             STD_STAT_TCH
 * 중첩하거나 2점이상 만나면    STD_STAT_OVR
 *
 * const stdPoint2D*     aPtA1(In): 점A1
 * const stdPoint2D*     aPtA2(In): 점A2
 * const stdPoint2D*     aPtA3(In): 점A3
 * const stdPoint2D*     aPtB1(In): 점B1
 * const stdPoint2D*     aPtB2(In): 점B2
 * const stdPoint2D*     aPtB3(In): 점B3
 **********************************************************************/
GeoStatusTypes stdUtils::getStatus3Points3Points2D(
                stdPoint2D*     aPtA1,
                stdPoint2D*     aPtA2,
                stdPoint2D*     aPtA3,
                stdPoint2D*     aPtB1,
                stdPoint2D*     aPtB2,
                stdPoint2D*     aPtB3 )
{
    idBool  sSectorA1, sSectorA2;
    idBool  sOnPointA2 = ID_FALSE;
    idBool  sSectorB1 = ID_FALSE;
    idBool  sSectorB2;
    idBool  sOnPointB2 = ID_FALSE;
    idBool  sInner1 = ID_FALSE;
    idBool  sInner2 = ID_FALSE;
    SDouble sBaseAngle, sAngle1, sAngle2, sAngle3;
    GeoStatusTypes sStatus;
    
    UInt    sOnPointA = 0;
    UInt    sOnPointB = 0;

    sStatus = STD_STAT_DIS;
    
    if( isSamePoints2D(aPtA2, aPtB2) == ID_TRUE )
    {
        // BUG-15925
        sBaseAngle = getAngle2D( aPtA1, aPtA2, aPtA3 );
        if( idlOS::fabs( sBaseAngle-_PIE ) < STD_MATH_TOL )
        {
            if( area2D(aPtA1, aPtA3, aPtB1) * area2D(aPtA1, aPtA3, aPtB3) < 0)
            {
                sStatus =  STD_STAT_INT;
            }
            else
            {
                sStatus =  STD_STAT_TCH;
            }
            IDE_RAISE( goto_return );
        }
        else
        {
            sAngle1    = getAngle2D( aPtA1, aPtA2, aPtB1 );
            sAngle2    = getAngle2D( aPtA3, aPtA2, aPtB1 );
            sAngle3    = sAngle1 + sAngle2;
        
            if( idlOS::fabs( sBaseAngle - sAngle3 ) < STD_MATH_TOL )
            {
                sInner1 = ID_TRUE;
            }

            sAngle1    = getAngle2D( aPtA1, aPtA2, aPtB3 );
            sAngle2    = getAngle2D( aPtA3, aPtA2, aPtB3 );
            sAngle3    = sAngle1 + sAngle2;
        
            if( idlOS::fabs( sBaseAngle - sAngle3 ) < STD_MATH_TOL )
            {
                sInner2 = ID_TRUE;
            }

            if( sInner1 != sInner2 )
            {
                sStatus =  STD_STAT_INT;
            }
            else
            {
                sStatus =  STD_STAT_TCH;
            }
            IDE_RAISE( goto_return );
        }
        
    }
    
    if( (intersectI2D( aPtA1, aPtA2, aPtB1, aPtB2 ) == ID_TRUE) ||
        (intersectI2D( aPtA1, aPtA2, aPtB2, aPtB3 ) == ID_TRUE) ||
        (intersectI2D( aPtA2, aPtA3, aPtB1, aPtB2 ) == ID_TRUE) ||
        (intersectI2D( aPtA2, aPtA3, aPtB2, aPtB3 ) == ID_TRUE) )
    {
        sStatus =  STD_STAT_INT;
        IDE_RAISE( goto_return );
    }

    if( (between2D( aPtB1, aPtB2, aPtA1 ) == ID_TRUE) ||
        (between2D( aPtB2, aPtB3, aPtA1 ) == ID_TRUE) )
    {
        sOnPointA++;
    }
    if( (between2D( aPtB1, aPtB2, aPtA2 ) == ID_TRUE) ||
        (between2D( aPtB2, aPtB3, aPtA2 ) == ID_TRUE) )
    {
        sOnPointA++;
        if( ((sSectorA1 = betweenOnRight2D( aPtB1, aPtB2, aPtA2 )) == ID_TRUE)||
            ((sSectorA2 = betweenOnLeft2D( aPtB2, aPtB3, aPtA2 )) == ID_TRUE) )
        {
            sOnPointA2 = ID_TRUE;
        }   
    }
    if( (between2D( aPtB1, aPtB2, aPtA3 ) == ID_TRUE) ||
        (between2D( aPtB2, aPtB3, aPtA3 ) == ID_TRUE) )
    {
        sOnPointA++;
    }
    if( sOnPointA > 1 )
    {
        sStatus =  STD_STAT_OVR;
        IDE_RAISE( goto_return );
    }
    
    if( (between2D( aPtA1, aPtA2, aPtB1 ) == ID_TRUE) ||
        (between2D( aPtA2, aPtA3, aPtB1 ) == ID_TRUE) )
    {
        sOnPointB++;
    }
    if( (betweenOnRight2D( aPtA1, aPtA2, aPtB2 ) == ID_TRUE) ||
        (betweenOnLeft2D( aPtA2, aPtA3, aPtB2 ) == ID_TRUE) )
    {
        sOnPointB++;
        if( ((sSectorB1 = betweenOnRight2D( aPtA1, aPtA2, aPtB2 )) == ID_TRUE)||
            ((sSectorB2 = betweenOnLeft2D( aPtA2, aPtA3, aPtB2 )) == ID_TRUE) )
        {
            sOnPointB2 = ID_TRUE;
        }   
    }
    if( (between2D( aPtA1, aPtA2, aPtB3 ) == ID_TRUE) ||
        (between2D( aPtA2, aPtA3, aPtB3 ) == ID_TRUE) )
    {
        sOnPointB++;
    }    
    if( sOnPointB > 1 )
    {
        sStatus =  STD_STAT_OVR;
        IDE_RAISE( goto_return );
    }
    
    if( (sOnPointA2 == ID_TRUE) && (sOnPointB2 == ID_TRUE) )
    {
        sStatus =  STD_STAT_OVR;
        IDE_RAISE( goto_return );
    }
    else if(sOnPointA2 == ID_TRUE)
    {
        if( sSectorA1 == ID_TRUE )
        {
            if( area2D(aPtB1, aPtB2, aPtA1) * area2D(aPtB1, aPtB2, aPtA3) < 0)
            {
                sStatus =  STD_STAT_INT;
            }
            else
            {
                sStatus =  STD_STAT_TCH;
            }
            IDE_RAISE( goto_return );
        }
        /* 
        Fix BUG-15925
        sSectorA1만 검사해야 Loop을 돌면서 2번 검출되지 않는다.
        else
        {
            if( area2D(aPtB2, aPtB3, aPtA1) * area2D(aPtB2, aPtB3, aPtA3) < 0)
            {
                return STD_STAT_INT;
            }
            return STD_STAT_TCH;
        }
        */
    }
    else if(sOnPointB2 == ID_TRUE)
    {
        if( sSectorB1 == ID_TRUE )
        {
            if( area2D(aPtA1, aPtA2, aPtB1) * area2D(aPtA1, aPtA2, aPtB3) < 0)
            {
                sStatus =  STD_STAT_INT;
            }
            else
            {
                sStatus =  STD_STAT_TCH;
            }
            IDE_RAISE( goto_return );
            
        }
        /*
        Fix BUG-15925
        sSectorB1만 검사해야 Loop을 돌면서 2번 검출되지 않는다.
        else
        {
            if( area2D(aPtA2, aPtA3, aPtB1) * area2D(aPtA2, aPtA3, aPtB3) < 0)
            {
                return STD_STAT_INT;
            }
            return STD_STAT_TCH;
        }
        */
    }

    IDE_EXCEPTION_CONT( goto_return );
        
    return sStatus;
    
}

/***********************************************************************
 * Description:
 * 두개의 stdLineString2DType 객체가 교차하면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdLineString2DType* aLine1(In): 라인1
 * const stdLineString2DType* aLine2(In): 라인2
 **********************************************************************/
idBool stdUtils::intersectLineToLine2D(
                            const stdLineString2DType*     aLine1,
                            const stdLineString2DType*     aLine2 )
{
    stdPoint2D *sPt1, *sPt2;
    stdPoint2D *sPt3, *sPt4;
    stdPoint2D *sPtFence1E, *sPtFence2E;
    SInt        i, j;
    idBool      sIsEndPt1, sIsEndPt2, sIsEndPt3, sIsEndPt4;
    SInt        sNumLines1,  sNumLines2;
    SInt        sNumPoints1, sNumPoints2;
    idBool      sIsClosed1,  sIsClosed2;

    // Initialize
    sPt1 = STD_FIRST_PT2D(aLine1);
    sPt3 = STD_FIRST_PT2D(aLine2);
    sNumPoints1 = STD_N_POINTS(aLine1);
    sNumPoints2 = STD_N_POINTS(aLine2);
    sPtFence1E  = STD_LAST_PT2D(aLine1) + 1;
    sPtFence2E  = STD_LAST_PT2D(aLine2) + 1;

    // Calc Line Segment Count
    sNumLines1  = 0;
    for( i=0; i<sNumPoints1-1; i++ )
    {
        if( stdUtils::isSamePoints2D( STD_NEXTN_PT2D(sPt1,i),
                                      STD_NEXTN_PT2D(sPt1,i+1) ) ==ID_FALSE )
        {
            sNumLines1++;
        }
    }
    
    // Calc Line Segment Count
    sNumLines2  = 0;
    for( i=0; i<sNumPoints2-1; i++ )
    {
        if( stdUtils::isSamePoints2D( STD_NEXTN_PT2D(sPt3,i),
                                      STD_NEXTN_PT2D(sPt3,i+1) ) ==ID_FALSE )
        {
            sNumLines2++;
        }
    }
    // Check Closed LineString
    sIsClosed1 = stdUtils::isSamePoints2D( STD_NEXTN_PT2D(sPt1,0),
                                           STD_NEXTN_PT2D(sPt1,sNumPoints1-1) );
    sIsClosed2 = stdUtils::isSamePoints2D( STD_NEXTN_PT2D(sPt3,0),
                                           STD_NEXTN_PT2D(sPt3,sNumPoints2-1) );

    // Test Intersects
    sPt1 = STD_FIRST_PT2D(aLine1);
    for( i = 0; i < sNumLines1; i++ )
    {
        sPt2 = stdUtils::findNextPointInLine2D( sPt1, sPtFence1E );
        IDE_TEST_RAISE(sPt2 == NULL, ERR_INVALID_POINT);

        if( sIsClosed1==ID_TRUE )
        {
            sIsEndPt1 = ID_FALSE;
            sIsEndPt2 = ID_FALSE;
        }
        else
        {
            sIsEndPt1 = (i==0)             ? ID_TRUE : ID_FALSE;
            sIsEndPt2 = (i==(sNumLines1-1)) ? ID_TRUE : ID_FALSE;
        }
        
        sPt3 = STD_FIRST_PT2D(aLine2);
        for( j = 0; j < sNumLines2; j++ )
        {
            sPt4 = stdUtils::findNextPointInLine2D( sPt3, sPtFence2E );
            IDE_TEST_RAISE(sPt4 == NULL, ERR_INVALID_POINT);

            if( sIsClosed2==ID_TRUE )
            {
                sIsEndPt3 = ID_FALSE;
                sIsEndPt4 = ID_FALSE;
            }
            else
            {
                sIsEndPt3 = (j==0)             ? ID_TRUE : ID_FALSE;
                sIsEndPt4 = (j==(sNumLines2-1)) ? ID_TRUE : ID_FALSE;
            }

            // Test Internal Intersects
            IDE_TEST( stdUtils::isIntersectsLineToLine2D(
                          sPt1, sPt2, sIsEndPt1, sIsEndPt2,
                          sPt3, sPt4, sIsEndPt3, sIsEndPt4 )
                      == ID_TRUE);

            sPt3 = sPt4;
        }
        sPt1 = sPt2;
    }
    return ID_FALSE;

    IDE_EXCEPTION(ERR_INVALID_POINT);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_INTEGRITY_VIOLATION));
    }
    IDE_EXCEPTION_END;

    return ID_TRUE;
}

/***********************************************************************
 * Description:
 * aRing1? aRing를 포함하면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * stdLinearRing2D*    aRing1(In): 링1
 * stdMBR*             aMBR1(In): 링1의 MBR
 * stdLinearRing2D*    aRing2(In): 링2
 * stdMBR*             aMBR2(In): 링2의 MBR
 **********************************************************************/
idBool stdUtils::isRingContainsRing2D(
                        stdLinearRing2D*    aRing1,
                        stdMBR*             aMBR1,
                        stdLinearRing2D*    aRing2,
                        stdMBR*             aMBR2 )
{
    stdPoint2D          *sPt1, *sPtPre1, *sPtNxt1;
    stdPoint2D          *sPt2, *sPtPre2, *sPtNxt2;
    UInt                i, j, sMax1, sMax2, sDelay = 0;
    GeoStatusTypes      sStatus;

    // BUG-22338
    stdMBR              sFirstMBR;
    stdMBR              sSecondMBR;    
    
    if( isMBRContains( aMBR1, aMBR2 ) == ID_FALSE )
    {
        return ID_FALSE;
    }
    
    sPt2 = STD_FIRST_PT2D(aRing2);
    sMax2 = STD_N_POINTS(aRing2) - 1;

    for( j = 0; j < sMax2; j++ )
    {
        if(insideRing2D(sPt2, aRing1) == ID_FALSE)
        {
            return ID_FALSE;
        }
        
        sPtPre2 = stdUtils::findPrevPointInRing2D( sPt2, j, sMax2, NULL );
        sPtNxt2 = stdUtils::findNextPointInRing2D( sPt2, j, sMax2, NULL );

        get3PtMBR(sPtPre2, sPt2, sPtNxt2, &sFirstMBR);    

        sPt1 = STD_FIRST_PT2D(aRing1);
        sMax1 = STD_N_POINTS(aRing1)-1;
        for( i = 0; i < sMax1; i++ )
        {
            sPtPre1 = stdUtils::findPrevPointInRing2D( sPt1, i, sMax1, NULL );
            sPtNxt1 = stdUtils::findNextPointInRing2D( sPt1, i, sMax1, NULL );
            // BUG-22338
            get3PtMBR(sPtPre1, sPt1, sPtNxt1, &sSecondMBR);

            if (isMBRIntersects(&sFirstMBR, &sSecondMBR) == ID_TRUE)
            {
            
                sStatus = stdUtils::getStatus3Points3Points2D( 
                    sPtPre1, sPt1, sPtNxt1, sPtPre2, sPt2, sPtNxt2 );
                
                
                if( sStatus == STD_STAT_TCH )
                {
                    if( sDelay == 0)
                    {
                        sDelay++;
                    }
                    else
                    {
                        return ID_FALSE;
                    }
                } 
                else if( (sStatus == STD_STAT_INT) || (sStatus == STD_STAT_OVR) )
                {
                    return ID_FALSE;
                }
            }
            
            sPt1 = STD_NEXT_PT2D(sPt1);
        }
        sPt2 = STD_NEXT_PT2D(sPt2);
    }
    return ID_TRUE;
}

/***********************************************************************
 * Description:
 * aRing1이 aRing2를 포함하면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * stdLinearRing2D*    aRing1(In): 링1
 * stdLinearRing2D*    aRing2(In): 링2
 **********************************************************************/
idBool stdUtils::isRingContainsRing2D(
                        stdLinearRing2D*    aRing1,
                        stdLinearRing2D*    aRing2 )
{
    stdPoint2D          *sPt1, *sPtPre1, *sPtNxt1;
    stdPoint2D          *sPt2, *sPtPre2, *sPtNxt2;
    UInt                i, j, sMax1, sMax2, sDelay = 0;
    GeoStatusTypes      sStatus;
    // BUG-22338
    stdMBR              sFirstMBR;
    stdMBR              sSecondMBR;    
    
    sPt2 = STD_FIRST_PT2D(aRing2);
    sMax2 = STD_N_POINTS(aRing2)-1;
    for( j = 0; j < sMax2; j++ )
    {
        sPtPre2 = stdUtils::findPrevPointInRing2D( sPt2, j, sMax2, NULL );
        sPtNxt2 = stdUtils::findNextPointInRing2D( sPt2, j, sMax2, NULL );
        get3PtMBR(sPtPre2, sPt2, sPtNxt2, &sFirstMBR);
        
        sPt1 = STD_FIRST_PT2D(aRing1);
        sMax1 = STD_N_POINTS(aRing1)-1;
        for( i = 0; i < sMax1; i++ )
        {
            sPtPre1 = stdUtils::findPrevPointInRing2D( sPt1, i, sMax1, NULL );
            sPtNxt1 = stdUtils::findNextPointInRing2D( sPt1, i, sMax1, NULL );
            // BUG-22338
            get3PtMBR(sPtPre1, sPt1, sPtNxt1, &sSecondMBR);

            if (isMBRIntersects(&sFirstMBR, &sSecondMBR) == ID_TRUE)
            {
                sStatus = stdUtils::getStatus3Points3Points2D( 
                    sPtPre1, sPt1, sPtNxt1, sPtPre2, sPt2, sPtNxt2 );
                
                if( sStatus == STD_STAT_TCH )
                {
                    if( sDelay == 0)
                    {
                        sDelay++;
                    }
                    else
                    {
                        return ID_FALSE;
                    }
                } 
                else if( (sStatus == STD_STAT_INT) ||
                         (sStatus == STD_STAT_OVR) )
                {
                    return ID_FALSE;
                }
            }
            else
            {
                // do nothing!!
            }
            
            sPt1 = STD_NEXT_PT2D(sPt1);
        }

        if(insideRing2D(sPt2, aRing1) == ID_FALSE)
        {
            return ID_FALSE;
        }
                
        sPt2 = STD_NEXT_PT2D(sPt2);
    }
    return ID_TRUE;
}

/***********************************************************************
 * Description:
 * aRing1이 aRing2를 포함하면 ID_TRUE 아니면 ID_FALSE 리턴
 *
 * stdLinearRing2D*    aRing1(In): 링1
 * stdMBR*             aMBR1(In): 링1의 MBR
 * stdLinearRing2D*    aRing2(In): 링2
 * stdMBR*             aMBR2(In): 링2의 MBR
 **********************************************************************/
idBool stdUtils::isRingNotDisjoint2D(
                        stdLinearRing2D*    aRing1,
                        stdMBR*             aMBR1,
                        stdLinearRing2D*    aRing2,
                        stdMBR*             aMBR2 )
{
    stdPoint2D          *sPt1, *sPtPre1, *sPtNxt1;
    stdPoint2D          *sPt2, *sPtPre2, *sPtNxt2;
    UInt                i, j, sMax1, sMax2, sDelay = 0;
    GeoStatusTypes      sStatus;
    stdMBR              sFirstMBR;
    stdMBR              sSecondMBR;
    
    if( isMBRIntersects( aMBR1, aMBR2 ) == ID_FALSE )
    {
        return ID_FALSE;
    }

    sPt1 = STD_FIRST_PT2D(aRing1);
    sMax1 = STD_N_POINTS(aRing1)-1;
    for( i = 0; i < sMax1; i++ )
    {
        sPtPre1 = stdUtils::findPrevPointInRing2D( sPt1, i, sMax1, NULL );
        sPtNxt1 = stdUtils::findNextPointInRing2D( sPt1, i, sMax1, NULL );
        get3PtMBR(sPtPre1, sPt1, sPtNxt1, &sFirstMBR);
        sPt2 = STD_FIRST_PT2D(aRing2);
        sMax2 = STD_N_POINTS(aRing2)-1;
        for( j = 0; j < sMax2; j++ )
        {
            sPtPre2 = stdUtils::findPrevPointInRing2D( sPt2, j, sMax2, NULL );
            sPtNxt2 = stdUtils::findNextPointInRing2D( sPt2, j, sMax2, NULL );
            // BUG-22338
            get3PtMBR(sPtPre2, sPt2, sPtNxt2, &sSecondMBR);
            if (isMBRIntersects(&sFirstMBR, &sSecondMBR) == ID_TRUE)
            {
                sStatus = stdUtils::getStatus3Points3Points2D( 
                    sPtPre1, sPt1, sPtNxt1, sPtPre2, sPt2, sPtNxt2 );
                
                if( sStatus == STD_STAT_TCH )
                {
                    if( sDelay == 0)
                    {
                        sDelay++;
                    }
                    else
                    {
                        return ID_TRUE;
                    }
                } 
                else if( (sStatus == STD_STAT_INT) ||
                         (sStatus == STD_STAT_OVR) )
                {
                    return ID_TRUE;
                }
            }
            else
            {
                // do nothing.
            }
            
            sPt2 = STD_NEXT_PT2D(sPt2);
        }
        // pt1이 ring2 안에 있는지를 확신하지 못한다.
        
        if (isMBRContainsPt(aMBR2, sPt1) == ID_TRUE)
        {
            if (insideRingI2D(sPt1, aRing2) == ID_TRUE)
            {
                return ID_TRUE;
            }
        }
        
        sPt1 = STD_NEXT_PT2D(sPt1);
    }
    
    sPt2 = STD_FIRST_PT2D(aRing2);
    sMax2 = STD_N_POINTS(aRing2) - 1;
    
    for( i = 0; i < sMax2; i++ )   
    {
        if (isMBRContainsPt(aMBR1, sPt2) == ID_TRUE)
        {
            if (insideRingI2D(sPt2, aRing1) == ID_TRUE)
            {
                return ID_TRUE;
            }
        }
        sPt2 = STD_NEXT_PT2D(sPt2);
    }
    
    return ID_FALSE;
}

//==============================================================================
// TASK-2015 한 점이 링 내부에 존재하는지 판별
//==============================================================================

/***********************************************************************
 * Description:
 * 점이 Boundary를 포함한 Ring의 내부에 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D*       aPt(In): 점
 * const stdLinearRing2D* aRing(In): 링
 **********************************************************************/
idBool stdUtils::insideRing2D(
                            const stdPoint2D*          aPt,
                            const stdLinearRing2D*     aRing )
{
    stdPoint2D* sVtx = STD_FIRST_PT2D(aRing);
    SInt        sCnt = 0;
    UInt        i, sMax;
    
    sMax = STD_N_POINTS(aRing)-1;
    for( i = 0; i < sMax; i++)
    {
        if( between2D(
            STD_NEXTN_PT2D(sVtx,i), 
            STD_NEXTN_PT2D(sVtx,i+1),
            aPt) == ID_TRUE )
        {
            return ID_TRUE;
        }
        
        if( STD_NEXTN_PT2D(sVtx,i)->mY <= aPt->mY)
        {
            if( STD_NEXTN_PT2D(sVtx,i+1)->mY > aPt->mY)  // 선분이 위로 진행
            {
                if(area2D(
                    STD_NEXTN_PT2D(sVtx,i), 
                    STD_NEXTN_PT2D(sVtx,i+1), 
                    aPt) > 0)  // aPt이 왼쪽
                {
                    ++sCnt;
                }
            }
        }
        else
        {
            if( STD_NEXTN_PT2D(sVtx,i+1)->mY <= aPt->mY)  // 선분이 아래로 진행
            {
                if(area2D(
                    STD_NEXTN_PT2D(sVtx,i), 
                    STD_NEXTN_PT2D(sVtx,i+1), 
                    aPt) < 0)   // aPt이 오른쪽
                {
                    --sCnt;
                }
            }
        }
    }
    if( sCnt == 0 )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

/***********************************************************************
 * Description:
 * 점이 Ring의 내부에 있으면 ID_TRUE 아니면 ID_FALSE
 *
 * const stdPoint2D*       aPt(In): 점
 * const stdLinearRing2D* aRing(In): 링
 **********************************************************************/
idBool stdUtils::insideRingI2D(
                            const stdPoint2D*          aPt,
                            const stdLinearRing2D*     aRing )
{
    stdPoint2D* sVtx = STD_FIRST_PT2D(aRing);
    SInt        sCnt = 0;
    UInt        i, sMax;
    
    sMax = STD_N_POINTS(aRing)-1;
    
    for( i = 0; i < sMax; i++)
    {
        if( between2D(
            STD_NEXTN_PT2D(sVtx,i), 
            STD_NEXTN_PT2D(sVtx,i+1), 
            aPt) == ID_TRUE )
        {
            return ID_FALSE;
        }
        
        if( STD_NEXTN_PT2D(sVtx,i)->mY <= aPt->mY)
        {
            if( STD_NEXTN_PT2D(sVtx,i+1)->mY > aPt->mY)  // 선분이 위로 진행
            {
                if(area2D(
                    STD_NEXTN_PT2D(sVtx,i), 
                    STD_NEXTN_PT2D(sVtx,i+1), 
                    aPt) > 0)  // aPt이 왼쪽
                {
                    ++sCnt;
                }
            }
        }
        else
        {
            if( STD_NEXTN_PT2D(sVtx,i+1)->mY <= aPt->mY)  // 선분이 아래로 진행
            {
                if(area2D(
                    STD_NEXTN_PT2D(sVtx,i), 
                    STD_NEXTN_PT2D(sVtx,i+1), 
                    aPt) < 0)   // aPt이 오른쪽
                {
                    --sCnt;
                }
            }
        }
    }
    if( sCnt == 0 )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

/***********************************************************************
 * Description:
 * 링의 좌표 정렬이 Counter Clock Wise이면 ID_TRUE 아니면 ID_FALSE
 *
 * stdLinearRing2D* aRing(In): 링
 **********************************************************************/
idBool stdUtils::isCCW2D( stdLinearRing2D *aRing )
{
    SInt   sSpidx;
    SInt   sEpidx;
    SInt   sCpidx;
    UInt   i, sMax;
    
    sMax = STD_N_POINTS(aRing);
    sSpidx = sMax - 2;
    sEpidx = 0;
    sCpidx = 1;

    stdPoint2D *sPts = STD_FIRST_PT2D(aRing);
    
    
    
    for( i = 1; i < sMax; i++ )
    {
        if( ( STD_NEXTN_PT2D(sPts,i)->mY < STD_NEXTN_PT2D(sPts,sEpidx)->mY ) ||
            ( ( STD_NEXTN_PT2D(sPts,i)->mY == STD_NEXTN_PT2D(sPts,sEpidx)->mY ) &&
              ( STD_NEXTN_PT2D(sPts,i)->mX > STD_NEXTN_PT2D(sPts,sEpidx)->mX ) ) ) 
        {
            sSpidx = i - 1;
            sEpidx = i;
            sCpidx = i + 1;            
        }
    }
    if( sCpidx >= (SInt)sMax )
    {
        sCpidx = 1;
    }
    return left2D( STD_NEXTN_PT2D(sPts,sSpidx), STD_NEXTN_PT2D(sPts,sEpidx),
                    STD_NEXTN_PT2D(sPts,sCpidx) );
}

//==============================================================================
// TASK-2015 두 선분의 교점 찾기
//==============================================================================

/***********************************************************************
 * Description:
 * 선분12 선분34의 교차점을 aResult로 받아 온다.
 * 이 함수의 외부에서 선분이 평행하거나 동일한 경우를 제외해야한다.
 *
 * const stdPoint2D*   aPt1(In): 점1
 * const stdPoint2D*   aPt2(In): 점2
 * const stdPoint2D*   aPt3(In): 점3
 * const stdPoint2D*   aPt4(In): 점4
 * stdPoint2D* aResult(Out): 교차점
 **********************************************************************/
idBool stdUtils::getIntersection2D(
                        const stdPoint2D*   aPt1,
                        const stdPoint2D*   aPt2,
                        const stdPoint2D*   aPt3,
                        const stdPoint2D*   aPt4,
                        stdPoint2D*         aResult )
{
    SDouble     sDesc, sLeftT, sLeftB;
    SDouble     sLT, sLB, sRT, sRB;
    
    if( between2D( aPt1, aPt2, aPt3 ) == ID_TRUE )
    {
        aResult->mX = aPt3->mX;
        aResult->mY = aPt3->mY;
        return ID_TRUE;
    }
    if( between2D( aPt1, aPt2, aPt4 ) == ID_TRUE )
    {
        aResult->mX = aPt4->mX;
        aResult->mY = aPt4->mY;
        return ID_TRUE;
    }
    if( between2D( aPt3, aPt4, aPt1 ) == ID_TRUE )
    {
        aResult->mX = aPt1->mX;
        aResult->mY = aPt1->mY;
        return ID_TRUE;
    }
    if( between2D( aPt3, aPt4, aPt2 ) == ID_TRUE )
    {
        aResult->mX = aPt2->mX;
        aResult->mY = aPt2->mY;
        return ID_TRUE;
    }
    
    sLT = aPt1->mX - aPt2->mX;
    sLB = aPt3->mX - aPt4->mX;
    sRT = aPt1->mY - aPt2->mY;
    sRB = aPt3->mY - aPt4->mY;
    sDesc = sLT*sRB - sLB*sRT;
            
    if( sDesc == 0 )
    {
        return ID_FALSE;
    }
    
    sLeftT = aPt1->mX*aPt2->mY - aPt2->mX*aPt1->mY;
    sLeftB = aPt3->mX*aPt4->mY - aPt4->mX*aPt3->mY;
    
    aResult->mX = (sLeftT*sLB - sLeftB*sLT)/ sDesc;
    aResult->mY = (sLeftT*sRB - sLeftB*sRT)/ sDesc;
    
    return ID_TRUE;
}

/***********************************************************************
 * Description:
 * 선분12와 폴리곤의 교차점을 aResult로 받아 온다.
 * 이 함수의 외부에서 교차하지 않는 경우를 제외해야한다.
 *
 * const stdPoint2D*        aPt1(In): 점1
 * const stdPoint2D*        aPt2(In): 점2
 * const stdPolygon2DType*  aPoly(In): 폴리곤
 * stdPoint2D*              aResult(Out): 교차점
 **********************************************************************/
idBool stdUtils::getIntersection2D(
                        const stdPoint2D*       aPt1,
                        const stdPoint2D*       aPt2,
                        const stdPolygon2DType* aPoly,
                        stdPoint2D*             aResult )
{
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPt;
    UInt                i, j, sMaxR, sMax;
    
    sRing = STD_FIRST_RN2D(aPoly);
    sMaxR = STD_N_RINGS(aPoly);
    for( i = 0; i < sMaxR; i++ )
    {
        sPt = STD_FIRST_PT2D(sRing);
        sMax = STD_N_POINTS(sRing)-1;
        for( j = 0; j < sMax; j++ )
        {
            if( intersectI2D(aPt1,aPt2,sPt,STD_NEXT_PT2D(sPt)) == ID_TRUE )
            {
                return getIntersection2D(aPt1,aPt2,sPt,STD_NEXT_PT2D(sPt),aResult);
            }
        }
        sRing = STD_NEXT_RN2D(sRing);
    }
    
    return ID_FALSE;
}

/***********************************************************************
 * Description:
 * 선분12 선분34의 교차점을 aPointSet에 받아 온다.
 * 
 * const stdPoint2D*   aPt1(In): 점1
 * const stdPoint2D*   aPt2(In): 점2
 * const stdPoint2D*   aPt3(In): 점3
 * const stdPoint2D*   aPt4(In): 점4
 * stdPoint2D* aResult(Out): 교차점
 * aNumPoints: 교차점의 개수
 **********************************************************************/
idBool stdUtils::getIntersects2D( 
                            const stdPoint2D*   aSp0,
                            const stdPoint2D*   aEp0,
                            const stdPoint2D*   aSp1,
                            const stdPoint2D*   aEp1,
                            stdPoint2D*         aPointSet,
                            SInt*               aNumPoints )
{
    stdPoint2D sMinP;
    stdPoint2D sMaxP;

    SDouble a =  ( aEp0->mY - aSp0->mY );
    SDouble b = -( aEp0->mX - aSp0->mX );
    SDouble c =  ( aEp1->mY - aSp1->mY );
    SDouble d = -( aEp1->mX - aSp1->mX );
    SDouble p = a * aSp0->mX + b * aSp0->mY;
    SDouble q = c * aSp1->mX + d * aSp1->mY;
    SDouble ad_bc = ( a * d ) - ( b * c );
    SShort  slope = ( ( a * b ) >= 0 ) ? 1 : 0;

    *aNumPoints = 0;

    if( ( (a!=0.) && (c!=0.) ) || ( (b!=0.) && (d!=0.) ) ||
        ( ( a * d == b * c ) ) )
    {
        if( collinear2D( aSp0, aEp0, aSp1 ) == ID_TRUE )
        {
            sMinP.mX = max( min ( aSp0->mX, aEp0->mX ), min( aSp1->mX, aEp1->mX ) );
            sMinP.mY = max( min ( aSp0->mY, aEp0->mY ), min( aSp1->mY, aEp1->mY ) );
            sMaxP.mX = min( max ( aSp0->mX, aEp0->mX ), max( aSp1->mX, aEp1->mX ) );
            sMaxP.mY = min( max ( aSp0->mY, aEp0->mY ), max( aSp1->mY, aEp1->mY ) );

            if( ( sMinP.mX < sMaxP.mX ) || ( sMinP.mY < sMaxP.mY ) )
            {
                *aNumPoints = 2;
                if( slope != 0 )
                {
                    aPointSet[0].mX = sMinP.mX;
                    aPointSet[0].mY = sMinP.mY;
                    aPointSet[1].mX = sMaxP.mX;
                    aPointSet[1].mY = sMaxP.mY;
                }
                else
                {
                    aPointSet[0].mX = sMinP.mX;
                    aPointSet[0].mY = sMaxP.mY;
                    aPointSet[1].mX = sMaxP.mX;
                    aPointSet[1].mY = sMinP.mY;                   
                }
            }
            else if( sMinP.mX == sMaxP.mX )
            {
                *aNumPoints = 1;
                aPointSet[0].mX = sMinP.mX;
                aPointSet[0].mY = sMinP.mY;                
            }
        }
        else
        {
            if( between2D( aSp0, aEp0, aSp1 ) == ID_TRUE )
            {
                *aNumPoints = 1;
                aPointSet[0].mX = aSp1->mX;
                aPointSet[0].mY = aSp1->mY;                
            }
            else if( between2D( aSp0, aEp0, aEp1 ) == ID_TRUE )
            {
                *aNumPoints = 1;
                aPointSet[0].mX = aEp1->mX;
                aPointSet[0].mY = aEp1->mY;                
            }
            else if( between2D( aSp1, aEp1, aSp0 ) == ID_TRUE )
            {
                *aNumPoints = 1;
                aPointSet[0].mX = aSp0->mX;
                aPointSet[0].mY = aSp0->mY;                
            }
            else if( between2D( aSp1, aEp1, aEp0 ) == ID_TRUE )
            {
                *aNumPoints = 1;
                aPointSet[0].mX = aEp0->mX;
                aPointSet[0].mY = aEp0->mY;                
            }
            else
            {
                sMinP.mX = ( ( d * p ) - ( b * q ) ) / ad_bc;
                sMinP.mY = ( ( a * q ) - ( c * p ) ) / ad_bc;
                if( (between2D( aSp0, aEp0, &sMinP ) == ID_TRUE) &&
                    (between2D( aSp1, aEp1, &sMinP ) == ID_TRUE) )
                {
                    *aNumPoints = 1;
                    aPointSet[0].mX = sMinP.mX;
                    aPointSet[0].mY = sMinP.mY;
                }
            }
        }
    }
    if ( *aNumPoints > 0 )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

//==============================================================================
// TASK-2015 삼각형의 넓이
//==============================================================================

/***********************************************************************
 * Description:
 * 삼각형의 넓이의 2배의 값을 리턴
 * 
 * stdPoint2D*   aPt1(In): 점1
 * stdPoint2D*   aPt2(In): 점2
 * stdPoint2D*   aPt3(In): 점3
 **********************************************************************/
inline SDouble areaDigon2D(
                            stdPoint2D* aPt1,
                            stdPoint2D* aPt2,
                            stdPoint2D* aPt3 )
{
    return (aPt2->mX - aPt1->mX)*(aPt3->mY - aPt1->mY) - 
       (aPt3->mX - aPt1->mX)*(aPt2->mY - aPt1->mY);
}

/***********************************************************************
 * Description:
 * Ring의 넓이 리턴
 * 
 * stdLinearRing2D* aRing(In): 링
 **********************************************************************/
SDouble stdUtils::areaRing2D( stdLinearRing2D* aRing )
{
    stdPoint2D* sPt;
    SDouble     sAreasum = 0;
    SDouble     sArea;
    UInt        i, sMax;
    
    sPt = STD_FIRST_PT2D(aRing);
    sMax = STD_N_POINTS(aRing)-1;

    for( i = 1; i < sMax; i++) {
        if( (isSamePoints2D(sPt, 
                STD_NEXTN_PT2D(sPt,i) ) == ID_TRUE) ||
            (isSamePoints2D(sPt, 
                STD_NEXTN_PT2D(sPt,i+1) ) == ID_TRUE) )
        {
            continue;
        }
        
        // Fix BUG-15436
        sArea = areaDigon2D( 
            sPt, 
            STD_NEXTN_PT2D(sPt,i), 
            STD_NEXTN_PT2D(sPt,i+1) );
        if( sArea < 0. )
        {
            sArea *= -1.;        
        }
        sAreasum += sArea;
    }
    return (sAreasum/2.0);
}

//==============================================================================
// TASK-2015 Ring의 무게 중심
//==============================================================================

inline void centroidDigon2D(
                            stdPoint2D* aPt1,
                            stdPoint2D* aPt2,
                            stdPoint2D* aPt3,
                            stdPoint2D* aResult )
{
    aResult->mX = aPt1->mX + aPt2->mX + aPt3->mX;
    aResult->mY = aPt1->mY + aPt2->mY + aPt3->mY;
    return;
}

/***********************************************************************
 * Description:
 * Ring의 무게 중심을 aResult로 리턴
 * 
 * stdLinearRing2D* aRing(In): 링
 * stdPoint2D* aResult(Out): 무게 중심
 **********************************************************************/
IDE_RC stdUtils::centroidRing2D(
                    stdLinearRing2D* aRing,
                    stdPoint2D* aResult )
{
    UInt        i, nCnt, sMax;
    SDouble     sAreasum, sPatialsum = 0;
    stdPoint2D  sPtCent, * sPt;
    
    sPt = STD_FIRST_PT2D(aRing);

    aResult->mX = 0;
    aResult->mY = 0;
    nCnt = 0;
    sMax = STD_N_POINTS(aRing)-1;
    for( i = 1; i < sMax; i++)
    {
        centroidDigon2D( 
            sPt, 
            STD_NEXTN_PT2D(sPt,i), 
            STD_NEXTN_PT2D(sPt,i+1), 
            &sPtCent );
        sAreasum =  areaDigon2D( 
            sPt, 
            STD_NEXTN_PT2D(sPt,i), 
            STD_NEXTN_PT2D(sPt,i+1) );
        if( (isSamePoints2D(sPt, STD_NEXTN_PT2D(sPt,i)) == ID_TRUE) ||
            (isSamePoints2D(sPt, STD_NEXTN_PT2D(sPt,i+1) ) == ID_TRUE) )
        {
            continue;
        }
        aResult->mX += sAreasum * sPtCent.mX;
        aResult->mY += sAreasum * sPtCent.mY;
        sPatialsum += sAreasum;
        nCnt++;
    }
    if( nCnt==0 )
    {
        aResult->mX = sPt->mX;
        aResult->mY = sPt->mY;
    }
    else
    {
        aResult->mX /= 3 * sPatialsum;
        aResult->mY /= 3 * sPatialsum;
    }

    return IDE_SUCCESS;

    //IDE_EXCEPTION_END;

    //return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 폴리곤의 무게 중심을 aResult로 리턴
 * 멀티 폴리곤의 무게 중심은 각자의 폴리곤의 무게 중심을 구하여 이것을 평균을 낸다
 * BUG-28690
 * stdMultiPolygon2DType* aMPoly(In): 링
 * stdPoint2D*          aResult(Out): 무게 중심
 **********************************************************************/
IDE_RC stdUtils::centroidMPolygon2D(
                    stdMultiPolygon2DType*  aMPoly,
                    stdPoint2D*             aResult )
{
    stdPolygon2DType*       sPoly = STD_FIRST_POLY2D(aMPoly);
    stdLinearRing2D*        sRing;
    UInt                    i, sMaxO, sCount = 0;
    stdPoint2D              sTempPoint;

    // Fix BUG-16413
    aResult->mX = 0;
    aResult->mY = 0;

    sMaxO    = STD_N_OBJECTS(aMPoly);
    IDE_TEST_RAISE(sMaxO <= 0, ERR_INVALID_OBJECT);

    for( i = 0; i < sMaxO; i++)
    {
        sRing = STD_FIRST_RN2D(sPoly);
        if( sRing != NULL )
        {
            IDE_TEST( centroidRing2D( sRing, &sTempPoint) != IDE_SUCCESS );
            aResult->mX +=sTempPoint.mX;
            aResult->mY +=sTempPoint.mY;
            sCount++;
        }
        sPoly = STD_NEXT_POLY2D(sPoly);
    }

    if ( sCount != 0 )
    {
        aResult->mX /= sCount;
        aResult->mY /= sCount;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_OBJECT);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_OBJECT );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_INTEGRITY_VIOLATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//==============================================================================
// TASK-2015 Polygon 내부의 한 점 구하기
//==============================================================================

extern "C" SInt cmpDouble(const void * aV1, const void * aV2)
{
    SDouble sValue1 = *(SDouble*)aV1;
    SDouble sValue2 = *(SDouble*)aV2;
    
    if( sValue1 > sValue2 )
    {
        return 1;
    }
    else if( sValue1 < sValue2 )
    {
        return -1;
    }
    return 0;
}

//BUG-28693
extern "C" SInt cmpPointOrder (const void * aV1, const void *aV2, idBool aXOrder)
{
    stdPoint2D sValue1 = *(stdPoint2D*) aV1;
    stdPoint2D sValue2 = *(stdPoint2D*) aV2;
    SDouble sFirst, sSecond;

    if ( aXOrder == ID_TRUE )
    {
        sFirst  = sValue1.mX;
        sSecond = sValue2.mX;
    }
    else
    {
        sFirst  = sValue1.mY;
        sSecond = sValue2.mY;
    }

    if ( sFirst < sSecond )
    {
        return 1;
    }
    else if ( sFirst > sSecond )
    {
        return -1;
    }
    return 0;
}

extern "C" SInt cmpSegment(const void * aV1, const void * aV2)
{
    Segment **sValue1 = (Segment**)aV1;
    Segment **sValue2 = (Segment**)aV2;
    SInt      sRet    = 0;    
    
    if( (*sValue1)->mStart.mX > (*sValue2)->mStart.mX )
    {
        sRet = 1;        
    }
    else
    {
        if( (*sValue1)->mStart.mX < (*sValue2)->mStart.mX )
        {
            sRet = -1;            
        }
    }
    
    return sRet;
}


/***********************************************************************
 * Description:
 * 폴리곤 내부의 한 점을 aResult로 리턴
 * 
 * stdMultiPolygon2DType* aMPoly(In): 링
 * stdPoint2D*          aResult(Out): 무게 중심
 **********************************************************************/
IDE_RC
stdUtils::getPointOnSurface2D( iduMemory*              aQmxMem,
                               const stdPolygon2DType* aPoly,
                               stdPoint2D*             aResult )
{
    stdLinearRing2D*    sRing = STD_FIRST_RN2D(aPoly);
    stdPoint2D*         sPt;
    stdPoint2D          sPtNear, sPtX;
    UInt                i, j, sMaxCnt, sCurCnt, sMaxR, sMax;
    SDouble             sLocalBuffer[ST_MAX_POINT_SORT_BUFFER_COUNT];
    SDouble*            sBuffer;

    IDE_TEST( centroidRing2D( sRing, aResult ) != IDE_SUCCESS );
    
    if( stfRelation::spiTosai(aResult, aPoly) == '0' )
    {
        IDE_RAISE( STD_UTIL_ON_SURFACE_2D );
    }

    sMaxCnt = STD_GEOM_SIZE(aPoly) / STD_PT2D_SIZE * 2;

    // BUG-33123
    IDE_TEST_RAISE( sMaxCnt == 0, STD_UTIL_ERR_INVALID_GEOMETRY );
    
    if ( sMaxCnt > ST_MAX_POINT_SORT_BUFFER_COUNT )
    {
        IDE_TEST( aQmxMem->alloc( ID_SIZEOF(SDouble)*sMaxCnt,
                                  (void**) & sBuffer )
                  != IDE_SUCCESS );
    }
    else
    {
        sMaxCnt = ST_MAX_POINT_SORT_BUFFER_COUNT;
        sBuffer = sLocalBuffer;
    }
    
    if( aResult->mX > 0 )
    {
        sPtNear.mX = aResult->mX-1;
        sPtNear.mY = aResult->mY;
    }
    else
    {
        sPtNear.mX = aResult->mX+1;
        sPtNear.mY = aResult->mY;
    }
    
    sCurCnt = 0;
    sMaxR = STD_N_RINGS(aPoly);
    for( i = 0; i < sMaxR; i++ )
    {
        sPt = STD_FIRST_PT2D(sRing);
        
        // Fix Bug-15605
        sMax = STD_N_POINTS(sRing)-1;
        for( j = 0; j < sMax; j++ )
        {
            if( ((sPt->mY <= aResult->mY) && 
                    (aResult->mY <= STD_NEXT_PT2D(sPt)->mY)) ||
                ((sPt->mY >= aResult->mY) && 
                    (aResult->mY >= STD_NEXT_PT2D(sPt)->mY)) )
            {
                if( STD_NEXT_PT2D(sPt)->mY == sPt->mY )
                {
                    sBuffer[sCurCnt] = sPt->mX;
                    sCurCnt++;
                    IDE_TEST_RAISE( sCurCnt >= sMaxCnt, STD_UTIL_BUFFER_OVERFLOW );
                    sBuffer[sCurCnt] = STD_NEXT_PT2D(sPt)->mX;
                    sCurCnt++;
                    IDE_TEST_RAISE( sCurCnt >= sMaxCnt, STD_UTIL_BUFFER_OVERFLOW );
                }
                else
                {
                    if(getIntersection2D(
                           sPt, 
                           STD_NEXT_PT2D(sPt), 
                           aResult, 
                           &sPtNear, 
                           &sPtX ) == ID_TRUE)
                    {
                        sBuffer[sCurCnt] = sPtX.mX;
                        sCurCnt++;
                        IDE_TEST_RAISE( sCurCnt >= sMaxCnt, STD_UTIL_BUFFER_OVERFLOW );
                    }
                }
            }
            sPt = STD_NEXT_PT2D(sPt);
        }
        sRing = (stdLinearRing2D*)STD_NEXT_PT2D(sPt);
    }
    if( sCurCnt < 2 )
    {
        IDE_RAISE( STD_UTIL_ON_SURFACE_2D );
    }
    
    idlOS::qsort( sBuffer, sCurCnt, ID_SIZEOF(SDouble), cmpDouble);

    for( i = 0; i < sCurCnt; i++ )
    {
        if( sBuffer[i] != sBuffer[i+1] )
        {
            break;
        }
    }
    
    aResult->mX = sBuffer[i]+(sBuffer[i+1]-sBuffer[i])/2.;

    IDE_EXCEPTION_CONT( STD_UTIL_ON_SURFACE_2D );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( STD_UTIL_ERR_INVALID_GEOMETRY );
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE ));
    }
    IDE_EXCEPTION( STD_UTIL_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::getPointOnSurface2D",
                                  "Buffer Overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

////////////////////////////////////////////////////////////////////////////////

void stdUtils::shiftMultiObjToSingleObj(stdGeometryHeader* aMultiObj)
{
    SChar   *sSrc, *sDst = (SChar*)aMultiObj;
    UInt    i, sSize;
    
    switch( aMultiObj->mType )
    {
    case STD_MULTIPOINT_2D_EXT_TYPE:
    case STD_MULTIPOINT_2D_TYPE:
        if( STD_N_OBJECTS((stdMultiPoint2DType*)aMultiObj) != 1 )
        {
            return;
        }
        sSrc = (SChar*)STD_FIRST_POINT2D(aMultiObj);
        sSize = STD_GEOM_SIZE(STD_FIRST_POINT2D(aMultiObj));
        break;
    case STD_MULTILINESTRING_2D_EXT_TYPE:
    case STD_MULTILINESTRING_2D_TYPE:
        if( STD_N_OBJECTS((stdMultiLineString2DType*)aMultiObj) != 1 )
        {
            return;
        }
        sSrc = (SChar*)STD_FIRST_LINE2D(aMultiObj);
        sSize = STD_GEOM_SIZE(STD_FIRST_LINE2D(aMultiObj));
        break;
    case STD_MULTIPOLYGON_2D_EXT_TYPE:
    case STD_MULTIPOLYGON_2D_TYPE:
        if( STD_N_OBJECTS((stdMultiPolygon2DType*)aMultiObj) != 1 )
        {
            return;
        }
        sSrc = (SChar*)STD_FIRST_POLY2D(aMultiObj);
        sSize = STD_GEOM_SIZE(STD_FIRST_POLY2D(aMultiObj));
        break;
    case STD_GEOCOLLECTION_2D_EXT_TYPE:
    case STD_GEOCOLLECTION_2D_TYPE:
        if( STD_N_GEOMS((stdGeoCollection2DType*)aMultiObj) != 1 )
        {
            return;
        }
        sSrc = (SChar*)STD_FIRST_COLL2D(aMultiObj);
        sSize = STD_GEOM_SIZE((stdGeometryHeader*)STD_FIRST_COLL2D(aMultiObj));
        break;
    default:
        return;
    }
    
    i = 0;
    while(i < sSize)
    {
        sDst[i] = sSrc[i];        
        i++;
    }
    
    if( (aMultiObj->mType == STD_GEOCOLLECTION_2D_TYPE))
    {
        shiftMultiObjToSingleObj(aMultiObj);
    }
}

void stdUtils::copyGeometry(
                    stdGeometryHeader* aGeomDst,
                    stdGeometryHeader* aGeom )
{
    idlOS::memcpy( aGeomDst, aGeom, STD_GEOM_SIZE(aGeom) );
}

GeoGroupTypes stdUtils::getGroup( stdGeometryHeader* aGeom )
{
    switch( aGeom->mType )
    {
        case STD_POINT_2D_EXT_TYPE:
        case STD_POINT_2D_TYPE:
        case STD_MULTIPOINT_2D_EXT_TYPE:
        case STD_MULTIPOINT_2D_TYPE:
            return STD_POINT_2D_GROUP;
        case STD_LINESTRING_2D_EXT_TYPE:
        case STD_LINESTRING_2D_TYPE:
        case STD_MULTILINESTRING_2D_EXT_TYPE:
        case STD_MULTILINESTRING_2D_TYPE:
            return STD_LINE_2D_GROUP;
        case STD_POLYGON_2D_EXT_TYPE:
        case STD_POLYGON_2D_TYPE:
        case STD_MULTIPOLYGON_2D_EXT_TYPE:
        case STD_MULTIPOLYGON_2D_TYPE:
            return STD_POLYGON_2D_GROUP;
    }
    return STD_NULL_GROUP;
}

void stdUtils::initMBR( stdMBR* aMBR )
{
    mtdDouble.null( NULL, &aMBR->mMinX );
    mtdDouble.null( NULL, &aMBR->mMinY );
    mtdDouble.null( NULL, &aMBR->mMaxX );
    mtdDouble.null( NULL, &aMBR->mMaxY );
}

// BUG-27518
void stdUtils::isIndexableObject(stdGeometryHeader *aValue,
                                 idBool            *aIsIndexable)
{
    /* BUG-25248 Empty Object insertion */
    if ( stdUtils::isNull(aValue) == ID_TRUE ||
         stdUtils::isEmpty(aValue) == ID_TRUE )
    {
        *aIsIndexable = ID_FALSE;
    }
    else
    {
        // check validity of mbr
        if ((mtdDouble.isNull(NULL, &(aValue->mMbr.mMaxX)) == ID_TRUE) ||
            (mtdDouble.isNull(NULL, &(aValue->mMbr.mMinX)) == ID_TRUE) ||
            (mtdDouble.isNull(NULL, &(aValue->mMbr.mMaxY)) == ID_TRUE) ||
            (mtdDouble.isNull(NULL, &(aValue->mMbr.mMinY)) == ID_TRUE) )
        {
            *aIsIndexable = ID_FALSE;
        }
        else
        {
            *aIsIndexable = ID_TRUE;
        }
    }
}


IDE_RC stdUtils::setMBRFromPoint2D(stdMBR* aMBR, stdPoint2D* aPoint)
{
    if((mtdDouble.isNull(NULL, &aMBR->mMaxX)==ID_TRUE)||
       (mtdDouble.isNull(NULL, &aMBR->mMaxY)==ID_TRUE))
    {
        aMBR->mMinX = aMBR->mMaxX = aPoint->mX;
        aMBR->mMinY = aMBR->mMaxY = aPoint->mY;
    }
    else
    {
        aMBR->mMinX = (aPoint->mX < aMBR->mMinX) ? aPoint->mX : aMBR->mMinX;
        aMBR->mMinY = (aPoint->mY < aMBR->mMinY) ? aPoint->mY : aMBR->mMinY;
        aMBR->mMaxX = (aPoint->mX > aMBR->mMaxX) ? aPoint->mX : aMBR->mMaxX;
        aMBR->mMaxY = (aPoint->mY > aMBR->mMaxY) ? aPoint->mY : aMBR->mMaxY;
    }    
    return IDE_SUCCESS;
}


IDE_RC stdUtils::setMBRFromRing2D(stdMBR* aMBR, stdLinearRing2D*  aRing)
{
    stdPoint2D*     sPt = STD_FIRST_PT2D(aRing);
    UInt            i, sMax;
    
    mtdDouble.null(NULL, &aMBR->mMaxX);

    sMax = STD_N_POINTS(aRing);
    for( i = 0; i < sMax; i++)
    {
        setMBRFromPoint2D(aMBR, sPt);
        sPt = STD_NEXT_PT2D(sPt);
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 * Line(Pt1,Pt2)와 Point(Pt)간의 Distance^2을 구한다.
 * Point로 부터 라인에 그은 법선간의 거리를 구한다.
 **********************************************************************/
SDouble stdUtils::getDistanceSqLineToPoint2D( const stdPoint2D *aPt1,
                                              const stdPoint2D *aPt2,
                                              const stdPoint2D *aPt,
                                              stdPoint2D *aCrPt  )
{
    SDouble    sC1, sC2, sF, sDelX, sDelY;
    SDouble    sDistanceSquare;
    
    sDelX = aPt2->mX - aPt1->mX;
    sDelY = aPt2->mY - aPt1->mY;
    
    sC1   = sDelX * (aPt->mX - aPt1->mX) + sDelY * (aPt->mY - aPt1->mY);
    sC2   = sDelX * sDelX                + sDelY * sDelY;
        
    sF = sC1 / sC2;
     
    aCrPt->mX = aPt1->mX + sDelX * sF;
    aCrPt->mY = aPt1->mY + sDelY * sF;

    sDistanceSquare = (aPt->mX - aCrPt->mX) * (aPt->mX - aCrPt->mX) +
                      (aPt->mY - aCrPt->mY) * (aPt->mY - aCrPt->mY);
    
    return sDistanceSquare;
}

/***********************************************************************
 * Description :
 * LineSegment(Pt1,Pt2)와 Point(Pt)간의 Distance^2을 구한다. 
 * Point로 부터 라인에 그은 법선이 LineSegment중간이라면
 * 그간의 거리를 구한다. 아니라면 가까운 끝점까지의 거리를 구한다.
 **********************************************************************/
SDouble stdUtils::getDistanceSqLineSegmentToPoint2D( const stdPoint2D *aPt1,
                                                     const stdPoint2D *aPt2,
                                                     const stdPoint2D *aPt )
{
    SDouble    sC1, sC2, sF, sX, sY, sDelX, sDelY;
    SDouble    sDistanceSquare;
    
    sDelX = aPt2->mX - aPt1->mX;
    sDelY = aPt2->mY - aPt1->mY;
    
    sC1   = sDelX * (aPt->mX - aPt1->mX) + sDelY * (aPt->mY - aPt1->mY);
    
    if( sC1 <= 0 ) // below
    {
        //d( p, p1 )
        sDistanceSquare = (aPt1->mX - aPt->mX) * (aPt1->mX - aPt->mX) +
                          (aPt1->mY - aPt->mY) * (aPt1->mY - aPt->mY);
    }
    else
    {
        sC2 = ( aPt2->mX - aPt1->mX ) * ( aPt2->mX - aPt1->mX ) +
              ( aPt2->mY - aPt1->mY ) * ( aPt2->mY - aPt1->mY );
        
        if( sC2 <= sC1 ) // above
        {
            //d( p, p2 )    
            sDistanceSquare = (aPt2->mX - aPt->mX) * (aPt2->mX - aPt->mX) +
                              (aPt2->mY - aPt->mY) * (aPt2->mY - aPt->mY);
        }
        else
        {
            sF = sC1 / sC2;
            sX = aPt1->mX + sDelX * sF;
            sY = aPt1->mY + sDelY * sF;

            sDistanceSquare = (aPt->mX - sX) * (aPt->mX - sX) +
                              (aPt->mY - sY) * (aPt->mY - sY);
        }
    }
    
    return sDistanceSquare; // 실거리 Distance = sqrt( sDistanceSquare );
}


idBool stdUtils::checkBetween2D( const stdPoint2D *aPt1,
                                 const stdPoint2D *aPt2,
                                 const stdPoint2D *aPt )
{
    idBool sRet;

    if( ( (aPt1->mX - aPt->mX ) * (aPt->mX - aPt2->mX ) >= 0 ) &&
        ( (aPt1->mY - aPt->mY ) * (aPt->mY - aPt2->mY ) >= 0 ) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }
    return sRet;
}

/***********************************************************************
 * Description :
 * Line(Pt1,Pt2)와 Line(Pt3,Pt4)가  교차점을 구한다. 
 * aCode 가 1이면 평행관계이다.
 **********************************************************************/
idBool stdUtils::getIntersectLineToLine2D(  const stdPoint2D *aPt1,
                                            const stdPoint2D *aPt2,
                                            const stdPoint2D *aPt3,
                                            const stdPoint2D *aPt4,
                                            stdPoint2D        *aPt,
                                            idBool           *aCode )
{
    SDouble sA1, sA2, sB1, sB2, sC1, sC2;// Coefficients of line eqns.
    SDouble sDenom, sDistanceSq;
    idBool  sRet;
    
    sA1 = aPt2->mY - aPt1->mY; 
    sB1 = aPt1->mX - aPt2->mX; 
    sC1 = aPt2->mX * aPt1->mY - aPt1->mX * aPt2->mY;
    // sA1*x + sB1*y + sC1 = 0 is line 1 

    sA2 = aPt4->mY - aPt3->mY; 
    sB2 = aPt3->mX - aPt4->mX; 
    sC2 = aPt4->mX * aPt3->mY - aPt3->mX * aPt4->mY;
    // sA2*x + sB2*y + sC2 = 0 is line 2 
    
    sDenom = sA1*sB2 - sA2*sB1;
    if( sDenom != 0 )
    {
        *aCode = ID_FALSE;
        
        aPt->mX = (sB1*sC2 - sB2*sC1) / sDenom;
        aPt->mY = (sA2*sC1 - sA1*sC2) / sDenom;
 
        if( checkBetween2D( aPt1, aPt2, aPt ) &&
            checkBetween2D( aPt3, aPt4, aPt ) )
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }
    else
    {
        *aCode = ID_TRUE;

        sDistanceSq = getDistanceSqLineToPoint2D( aPt1, aPt2, aPt3, aPt );
        if( sDistanceSq < STD_MATH_TOL_SQ )
        {
            if( (checkBetween2D( aPt1, aPt2, aPt3 )==ID_TRUE) ||
                (checkBetween2D( aPt1, aPt2, aPt4 )==ID_TRUE) ||
                (checkBetween2D( aPt3, aPt4, aPt1 )==ID_TRUE) ||
                (checkBetween2D( aPt3, aPt4, aPt2 )==ID_TRUE) )
            {
                sRet = ID_TRUE;
            }
            else
            {
                sRet = ID_FALSE;
            }
        }
        else
        {
            sRet = ID_FALSE;
        }
    }

    return sRet;
}

/***********************************************************************
 * Description :
 * LineString의 Interior Intersects인지 체크하는 서브루틴 
 * Line(Pt1,Pt2)와 Line(Pt3,Pt4)가  교차하는지 여부 조사
 * LineString상의 끝점인지 여부가 중요
 **********************************************************************/
idBool stdUtils::isIntersectsLineToLine2D( const stdPoint2D *aPt1,
                                           const stdPoint2D *aPt2,
                                           idBool            aIsEnd1,
                                           idBool            aIsEnd2,
                                           const stdPoint2D *aPt3,
                                           const stdPoint2D *aPt4,
                                           idBool            aIsEnd3,
                                           idBool            aIsEnd4  )
{
    idBool     sIsIntersects, sCode, sRet;
    stdPoint2D sPt;
    idBool     sIsSamePt13, sIsSamePt14, sIsSamePt23, sIsSamePt24;
    SInt       sSamePtCnt;

    // check MBR : disjointMBR
    if( ( IDL_MIN( aPt1->mX, aPt2->mX ) > IDL_MAX( aPt3->mX, aPt4->mX ) ) ||
        ( IDL_MIN( aPt1->mY, aPt2->mY ) > IDL_MAX( aPt3->mY, aPt4->mY ) ) ||
        ( IDL_MIN( aPt3->mX, aPt4->mX ) > IDL_MAX( aPt1->mX, aPt2->mX ) ) ||
        ( IDL_MIN( aPt3->mY, aPt4->mY ) > IDL_MAX( aPt1->mY, aPt2->mY ) ) )
    {
        IDE_RAISE( is_not_intersects );        
    }
    
    // Check EndPoint
    sSamePtCnt  = 0;
    
    sIsSamePt13 = isSamePoints2D( aPt1, aPt3 );
    if( sIsSamePt13==ID_TRUE )
    {
        sSamePtCnt++;
        if( (aIsEnd1==ID_FALSE) && (aIsEnd3==ID_FALSE) )
        {
            IDE_RAISE( is_intersects );
        }
    }
    
    sIsSamePt14 = isSamePoints2D( aPt1, aPt4 );
    if( sIsSamePt14==ID_TRUE )
    {
        sSamePtCnt++;
        if( (aIsEnd1==ID_FALSE) && (aIsEnd4==ID_FALSE) )
        {
            IDE_RAISE( is_intersects );
        }
    }
    
    sIsSamePt23 = isSamePoints2D( aPt2, aPt3 );
    if( sIsSamePt23==ID_TRUE )
    {
        sSamePtCnt++;
        if( (aIsEnd2==ID_FALSE) && (aIsEnd3==ID_FALSE) )
        {
            IDE_RAISE( is_intersects );
        }
    }
    
    sIsSamePt24 = stdUtils::isSamePoints2D( aPt2, aPt4 );
    if( sIsSamePt24==ID_TRUE )
    {
        sSamePtCnt++;
        if( (aIsEnd2==ID_FALSE) && (aIsEnd4==ID_FALSE) )
        {
            IDE_RAISE( is_intersects );
        }
    }

    if( sSamePtCnt>=2 ) // Check EqualLines
    {
        IDE_RAISE( is_intersects );
    }

    sIsIntersects = getIntersectLineToLine2D( aPt1,aPt2,aPt3,aPt4,&sPt,&sCode );
    if( sIsIntersects == ID_FALSE )
    {
        IDE_RAISE( is_not_intersects );
    }
    
    if( sCode==1 ) // 평행선 
    {
        if( sSamePtCnt==0 )
        {
            sRet = ID_TRUE;
        }
        else 
        {
            // sSamePtCnt==1
            sRet = ID_FALSE;
            if( sIsSamePt13==ID_TRUE )
            {
                if( (checkBetween2D( aPt1, aPt2, aPt4 )==ID_TRUE) ||
                    (checkBetween2D( aPt3, aPt4, aPt2 )==ID_TRUE) )
                {
                    sRet = ID_TRUE;
                }
            }
            else if( sIsSamePt14==ID_TRUE )
            {
                if( (checkBetween2D( aPt1, aPt2, aPt3 )==ID_TRUE) ||
                    (checkBetween2D( aPt3, aPt4, aPt2 )==ID_TRUE) )
                {
                    sRet = ID_TRUE;
                }
            }
            else if( sIsSamePt23==ID_TRUE )
            {
                if( (checkBetween2D( aPt1, aPt2, aPt4 )==ID_TRUE) ||
                    (checkBetween2D( aPt3, aPt4, aPt1 )==ID_TRUE) )
                {
                    sRet = ID_TRUE;
                }
            }
            else if( sIsSamePt24==ID_TRUE )
            {
                if( (checkBetween2D( aPt1, aPt2, aPt3 )==ID_TRUE) ||
                    (checkBetween2D( aPt3, aPt4, aPt1 )==ID_TRUE) )
                {
                    sRet = ID_TRUE;
                }
            }
        }
    }
    else
    {
        if( sSamePtCnt==1 )
        {
            // 선분이 끝점,끝점에서 만나는 경우 
            sRet = ID_FALSE;
        }
        else
        {
            // 선분 끝점이 다른 선분 중간에 만나는 경우 처리
            if( isSamePoints2D( aPt1, &sPt )==ID_TRUE )
            {
                sRet = (aIsEnd1==ID_FALSE) ? ID_TRUE:ID_FALSE;
            }
            else if( isSamePoints2D( aPt2, &sPt )==ID_TRUE )
            {
                sRet = (aIsEnd2==ID_FALSE) ? ID_TRUE:ID_FALSE;
            }
            else if( isSamePoints2D( aPt3, &sPt )==ID_TRUE )
            {
                sRet = (aIsEnd3==ID_FALSE) ? ID_TRUE:ID_FALSE;
            }
            else if( isSamePoints2D( aPt4, &sPt )==ID_TRUE )
            {
                sRet = (aIsEnd4==ID_FALSE) ? ID_TRUE:ID_FALSE;
            }
            else
            {
                // 선분이 서로 중간에서 만나는 경우 
                sRet = ID_TRUE;
            }
        }
    }
    return sRet;
    
    IDE_EXCEPTION( is_intersects );
    {
        sRet = ID_TRUE;
    }
    IDE_EXCEPTION( is_not_intersects );
    {
        sRet = ID_FALSE;
    }
    IDE_EXCEPTION_END;
    
    return sRet;
}


// BUG-22338
// to optimize multipolygon validation
idBool stdUtils::isMBRContainsPt(stdMBR *aMBR,
                                  stdPoint2D *aPt)
{

    IDE_TEST_RAISE(aMBR->mMinX > aPt->mX, false_return);
    IDE_TEST_RAISE(aMBR->mMaxX < aPt->mX, false_return);    
    IDE_TEST_RAISE(aMBR->mMinY > aPt->mY, false_return);    
    IDE_TEST_RAISE(aMBR->mMaxY < aPt->mY, false_return);

    return ID_TRUE;

    IDE_EXCEPTION(false_return);
    {
    }
    IDE_EXCEPTION_END;
    return ID_FALSE;
}
    
// BUG-22338
// to optimize multipolygon validation
void stdUtils::get3PtMBR(stdPoint2D *aPt1,
                         stdPoint2D *aPt2,
                         stdPoint2D *aPt3,                         
                         stdMBR *aMBR)
{
    stdPoint2D *sPt;
    sPt = aPt1;
    // initialize MBR wit first point
    aMBR->mMinX = sPt->mX;
    aMBR->mMinY = sPt->mY;
    aMBR->mMaxX = sPt->mX;
    aMBR->mMaxY = sPt->mY;

    // expand mbr with 2,3 pts.
    sPt = aPt2;
    
    if (sPt->mX > aMBR->mMaxX)
    {
        aMBR->mMaxX = sPt->mX;
    }
    else if (sPt->mX < aMBR->mMinX)
    {
        aMBR->mMinX = sPt->mX;
    }
    if (sPt->mY > aMBR->mMaxY)
    {
        aMBR->mMaxY = sPt->mY;
    }
    else if (sPt->mY < aMBR->mMinY)
    {
        aMBR->mMinY = sPt->mY;            
    }

    sPt = aPt3;
    if (sPt->mX > aMBR->mMaxX)
    {
        aMBR->mMaxX = sPt->mX;
    }
    else if (sPt->mX < aMBR->mMinX)
    {
        aMBR->mMinX = sPt->mX;
    }
    if (sPt->mY > aMBR->mMaxY)
    {
        aMBR->mMaxY = sPt->mY;
    }
    else if (sPt->mY < aMBR->mMinY)
    {
        aMBR->mMinY = sPt->mY;            
    }
}

void stdUtils::mergeOrMBR( stdMBR *aRet, stdMBR *aMBR1, stdMBR *aMBR2)
{
    stdMBR sMBR;
    
    if ( aMBR1->mMinX < aMBR2->mMinX )
    {
        sMBR.mMinX = aMBR1->mMinX;
    }
    else
    {
        sMBR.mMinX = aMBR2->mMinX;
    }

    if ( aMBR1->mMinY < aMBR2->mMinY )
    {
        sMBR.mMinY = aMBR1->mMinY;
    }
    else
    {
        sMBR.mMinY = aMBR2->mMinY;
    }

    if ( aMBR1->mMaxX > aMBR2->mMaxX )
    {
        sMBR.mMaxX = aMBR1->mMaxX;
    }
    else
    {
        sMBR.mMaxX = aMBR2->mMaxX;
    }

    if ( aMBR1->mMaxY > aMBR2->mMaxY )
    {
        sMBR.mMaxY = aMBR1->mMaxY;
    }
    else
    {
        sMBR.mMaxY = aMBR2->mMaxY;
    }

    *aRet = sMBR;
}

void stdUtils::mergeAndMBR( stdMBR *aRet, stdMBR *aMBR1, stdMBR *aMBR2)
{
    stdMBR sMBR;

    if ( aMBR1->mMinX > aMBR2->mMinX )
    {
        sMBR.mMinX = aMBR1->mMinX;
    }
    else
    {
        sMBR.mMinX = aMBR2->mMinX;
    }

    if ( aMBR1->mMinY > aMBR2->mMinY )
    {
        sMBR.mMinY = aMBR1->mMinY;
    }
    else
    {
        sMBR.mMinY = aMBR2->mMinY;
    }

    if ( aMBR1->mMaxX < aMBR2->mMaxX )
    {
        sMBR.mMaxX = aMBR1->mMaxX;
    }
    else
    {
        sMBR.mMaxX = aMBR2->mMaxX;
    }

    if ( aMBR1->mMaxY < aMBR2->mMaxY )
    {
        sMBR.mMaxY = aMBR1->mMaxY;
    }
    else
    {
        sMBR.mMaxY = aMBR2->mMaxY;
    }

    *aRet = sMBR;
}

idBool stdUtils::isNullMBR( stdMBR *aMBR )
{
    if( (mtdDouble.isNull(NULL, &aMBR->mMinX) == ID_TRUE) &&
        (mtdDouble.isNull(NULL, &aMBR->mMinY) == ID_TRUE) &&
        (mtdDouble.isNull(NULL, &aMBR->mMaxX) == ID_TRUE) &&
        (mtdDouble.isNull(NULL, &aMBR->mMaxY) == ID_TRUE) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

// PROJ-1591: Disk Spatial Index    
stdMBR* stdUtils::getMBRExtent( stdMBR* aDstMbr, stdMBR* aSrcMbr )
{
    aDstMbr->mMinX = aDstMbr->mMinX < aSrcMbr->mMinX ?
        aDstMbr->mMinX : aSrcMbr->mMinX;
    
    aDstMbr->mMinY = aDstMbr->mMinY < aSrcMbr->mMinY ?
        aDstMbr->mMinY : aSrcMbr->mMinY;
    
    aDstMbr->mMaxX = aDstMbr->mMaxX > aSrcMbr->mMaxX ?
        aDstMbr->mMaxX : aSrcMbr->mMaxX;
    
    aDstMbr->mMaxY = aDstMbr->mMaxY > aSrcMbr->mMaxY ?
        aDstMbr->mMaxY : aSrcMbr->mMaxY;
    
    return aDstMbr;
}

SDouble stdUtils::getMBRArea( stdMBR* aMbr )
{
    return ( ( aMbr->mMaxX - aMbr->mMinX ) * ( aMbr->mMaxY - aMbr->mMinY ) );
}

SDouble stdUtils::getMBRDelta( stdMBR* aMbr1, stdMBR* aMbr2 )
{
    stdMBR     sMbr;

    idlOS::memcpy( &sMbr, aMbr1, ID_SIZEOF(stdMBR) );
    
    return ( getMBRArea( getMBRExtent(&sMbr, aMbr2) ) -
             getMBRArea( aMbr1 ) );    
}

SDouble stdUtils::getMBROverlap( stdMBR* aMbr1, stdMBR* aMbr2 )
{
    SDouble sOverlap = 0.0;
    SDouble sX1 = 0.0;
    SDouble sX2 = 0.0;
    SDouble sY1 = 0.0;
    SDouble sY2 = 0.0;

    // aMbr1
    if( aMbr1->mMinX >= aMbr2->mMinX &&
        aMbr1->mMinX <= aMbr2->mMaxX )
    {
        sX1 = 0.0;
    }

    if( aMbr1->mMaxX >= aMbr2->mMinX &&
        aMbr1->mMaxX <= aMbr2->mMaxX )
    {
        sX2 = 0.0;
    }

    if( aMbr1->mMinY >= aMbr2->mMinY &&
        aMbr1->mMinY <= aMbr2->mMaxY )
    {
        sY1 = 0.0;
    }

    if( aMbr1->mMaxY >= aMbr2->mMinY &&
        aMbr1->mMaxY <= aMbr2->mMaxY )
    {
        sY2 = 0.0;
    }

    // aMbr2
    if( aMbr2->mMinX >= aMbr1->mMinX &&
        aMbr2->mMinX <= aMbr1->mMaxX )
    {
        sX1 = 0.0;
    }

    if( aMbr2->mMaxX >= aMbr1->mMinX &&
        aMbr2->mMaxX <= aMbr1->mMaxX )
    {
        sX2 = 0.0;
    }

    if( aMbr2->mMinY >= aMbr1->mMinY &&
        aMbr2->mMinY <= aMbr1->mMaxY )
    {
        sY1 = 0.0;
    }

    if( aMbr2->mMaxY >= aMbr1->mMinY &&
        aMbr2->mMaxY <= aMbr1->mMaxY )
    {
        sY2 = 0.0;
    }

    sOverlap = idlOS::fabs( (sX1 - sX2) * (sY1 - sY2) );

    return sOverlap;
}

SInt stdUtils::CCW(const stdPoint2D aPt1,
                   const stdPoint2D aPt2,
                   const stdPoint2D aPt3)
{
    SDouble sTemp;
    SInt    sRet;
    
    sTemp  = aPt2.mX * aPt3.mY - aPt1.mY * aPt2.mX - aPt1.mX * aPt3.mY
        - aPt2.mY * aPt3.mX + aPt1.mX * aPt2.mY + aPt1.mY * aPt3.mX;

    if ( sTemp > 0 ) 
    {
        sRet = ST_COUNTERCLOCKWISE;
    }
    else if ( sTemp < 0 ) 
    {
        sRet = ST_CLOCKWISE;
    }
    else
    {
        sRet = ST_PARALLEL;
    }

    return sRet;
}

IDE_RC stdUtils::intersectCCW(const stdPoint2D   aPt1,
                              const stdPoint2D   aPt2,
                              const stdPoint2D   aPt3,
                              const stdPoint2D   aPt4,
                              SInt             * aStatus,
                              SInt             * aNumPoints,
                              stdPoint2D       * aResultPoint)

{    
    stdMBR sMBR1;
    stdMBR sMBR2;    

    *aStatus = ST_NOT_INTERSECT;

    IDE_TEST_RAISE ( aPt1.mX > aPt2.mX, ERR_ABORT_INVALID_POINT_ORDER );
    IDE_TEST_RAISE ( aPt3.mX > aPt4.mX, ERR_ABORT_INVALID_POINT_ORDER );
    
    if ( aPt1.mY > aPt2.mY )
    {
        sMBR1.mMinY = aPt2.mY;
        sMBR1.mMaxY = aPt1.mY;           
    }
    else
    {
        sMBR1.mMinY = aPt1.mY;
        sMBR1.mMaxY = aPt2.mY;     
    }
    
    sMBR1.mMinX = aPt1.mX;
    sMBR1.mMaxX = aPt2.mX;

    if ( aPt3.mY > aPt4.mY )
    {
        sMBR2.mMinY = aPt4.mY;
        sMBR2.mMaxY = aPt3.mY;          
    }
    else
    {
        sMBR2.mMinY = aPt3.mY;
        sMBR2.mMaxY = aPt4.mY;        
    }

    sMBR2.mMinX = aPt3.mX;
    sMBR2.mMaxX = aPt4.mX;
    
    if (isMBRIntersects( &sMBR1, &sMBR2 ) == ID_FALSE )
    {
        *aStatus = ST_NOT_INTERSECT;    
    }
    else
    {
        if ( getIntersectPoint( aPt1, aPt2, aPt3, aPt4, aStatus, aResultPoint ) == ID_TRUE )
        {
            switch( *aStatus )
            {
                case ST_TOUCH:
                case ST_POINT_TOUCH:
                case ST_INTERSECT:
                    *aNumPoints = 1;
                    break;
                case ST_SHARE:
                    *aNumPoints = 2;
                    break;
                case ST_NOT_INTERSECT:
                    break;
                default:
                    IDE_RAISE( ERR_ABORT_INVALID_SEGMENT );
                    break;

            }
        }
        else
        {
            // Nothing to do 
        }
    }        
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_POINT_ORDER )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::intersectCCW",
                                  "invalid point order" ));
    }

    IDE_EXCEPTION( ERR_ABORT_INVALID_SEGMENT )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::intersectCCW",
                                  "invalid segment"));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


SInt stdUtils::getOrientationFromRing( stdLinearRing2D* aRing )
{
    stdPoint2D*     sPt        = STD_FIRST_PT2D(aRing);
    stdPoint2D*     sMaxPt;
    stdPoint2D*     sPrevPt;
    UInt            sNumPoints;
    UInt            i;
    UInt            sMaxIndex  = 0;
    
    sNumPoints = STD_N_POINTS(aRing);

    sMaxPt = sPt;

    for( i = 1; i < sNumPoints - 1; i++ )
    {
        sPt = STD_NEXT_PT2D(sPt);

        /* BUG-33634 
         * Y값이 가장 큰 점이 3개 이상있는 경우 링의 방향이 0 (ST_PARALLEL)인
         * 경우가 발생할 수 있어서 Y값이 가장 큰 점들 중에서 X값이 가장 큰 점을
         * 선택하도록 수정하였다. */

        if( sMaxPt->mY < sPt->mY )
        {
            sMaxPt    = sPt;
            sMaxIndex = i;
        }
        else
        {
            if( sMaxPt->mY == sPt->mY )
            {
                if( sMaxPt->mX <= sPt->mX )
                {
                    sMaxPt    = sPt;
                    sMaxIndex = i;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    if ( sMaxIndex == 0 )
    {
        sPrevPt = STD_PREV_PT2D(STD_LAST_PT2D(aRing));        
    }
    else
    {
        sPrevPt = STD_PREV_PT2D(sMaxPt);        
    }

    return stdUtils::CCW( *(sPrevPt), *(sMaxPt), *(STD_NEXT_PT2D(sMaxPt)));
}

idBool stdUtils::getIntersectPoint(const stdPoint2D aPt1,
                                   const stdPoint2D aPt2, 
                                   const stdPoint2D aPt3,
                                   const stdPoint2D aPt4,
                                   SInt*            aStatus,
                                   stdPoint2D*      aResult)    
{
    UInt       i;
    idBool     sIncl[4];
    stdPoint2D sTmp[4];
    idBool     sRet = ID_FALSE;
    SDouble    sT;
    SDouble    sT2;
    SDouble    sS;
    SDouble    sS2;
    SDouble    sUnder;

    sTmp[0] = aPt1;
    sTmp[1] = aPt2;
    sTmp[2] = aPt3;
    sTmp[3] = aPt4;

    *aStatus = ST_NOT_INTERSECT;

    sIncl[0] = between2D(&aPt3, &aPt4, &aPt1);
    sIncl[1] = between2D(&aPt3, &aPt4, &aPt2);
    sIncl[2] = between2D(&aPt1, &aPt2, &aPt3);
    sIncl[3] = between2D(&aPt1, &aPt2, &aPt4);

    for ( i = 0 ; i < 4 ; i++ )
    {
        if ( sIncl[i] == ID_TRUE )
        {
            if ( *aStatus == ST_NOT_INTERSECT ) 
            {
                *aStatus    = ST_TOUCH;
                aResult[0]  = sTmp[i];
                sRet        = ID_TRUE;
            }
            else
            {
                if ( isSamePoints2D( &aResult[0], &sTmp[i] ) != ID_TRUE )
                {
                    *aStatus    = ST_SHARE;
                    aResult[1]  = sTmp[i];
                    sRet        = ID_TRUE;
                }
                else
                {
                    *aStatus = ST_POINT_TOUCH;
                }
                break;
            }
        }
        else
        {
            // Nothing to do
        }
    }
    
    if ( ( sRet == ID_FALSE ) && ( *aStatus == ST_NOT_INTERSECT ) )
    {
        sUnder = (aPt4.mY - aPt3.mY) * (aPt2.mX - aPt1.mX) - (aPt4.mX - aPt3.mX) * (aPt2.mY - aPt1.mY);
        sT2 = (aPt4.mX - aPt3.mX) * (aPt1.mY - aPt3.mY) - (aPt4.mY - aPt3.mY) * (aPt1.mX - aPt3.mX);
        sS2 = (aPt2.mX - aPt1.mX) * (aPt1.mY - aPt3.mY) - (aPt2.mY - aPt1.mY) * (aPt1.mX - aPt3.mX); 

        sT = sT2 / sUnder;
        sS = sS2 / sUnder;
        if ( (sT < 0.0) || (sT > 1.0) || (sS < 0.0) || (sS > 1.0) )
        {
            *aStatus = ST_NOT_INTERSECT;                    
            sRet     = ID_FALSE;            
        }
        else
        {
            *aStatus = ST_INTERSECT;
            sRet     = ID_TRUE;

            aResult[0].mX = aPt1.mX + sT * (aPt2.mX - aPt1.mX);
            aResult[0].mY = aPt1.mY + sT * (aPt2.mY - aPt1.mY);
        }
    }
    else
    {
        // Nohting to do
    }

    return sRet;
}



IDE_RC stdUtils::classfyPolygonChain( iduMemory*        aQmxMem,
                                      stdPolygon2DType* aPolygon,
                                      UInt              aPolyNum,
                                      Segment**         aIndexSeg,
                                      UInt*             aIndexSegTotal,
                                      Segment**         aRingSegList,
                                      UInt*             aRingCount,
                                      idBool            aValidation )    
{
    stdLinearRing2D*    sRing;
    stdPoint2D*         sPt;
    stdPoint2D*         sNextPt;    
    UInt                i, j, sMaxR, sMax;
    SInt                sVaritation;
    SInt                sReverse;
    SInt                sPreVar        = ST_NOT_SET ;
    SInt                sPreRev        = ST_NOT_SET ;
    SInt                sOrientation;    
    Segment*            sSeg;
    Chain*              sChain         = NULL;
    Chain*              sParent        = NULL;
    Chain*              sPrevChain     = NULL;
    Chain*              sFirstChain    = NULL;    
    idBool              sIsCreateChain;    
    idBool              sIsSame;

    sRing = STD_FIRST_RN2D(aPolygon);
    sMaxR = STD_N_RINGS(aPolygon);    
  
    for ( i = 0 ; i < sMaxR; i++)
    {
        sMax    = STD_N_POINTS(sRing);        
        sPt     = STD_FIRST_PT2D(sRing);
        sOrientation = stdUtils::getOrientationFromRing(sRing);

        // 최소 링에 포함된 점은 4개 이상을 보장한다.
        IDE_TEST_RAISE( sMax < 4, ERR_ABORT_INVALID_NUM_OF_POINTS );
        
        sPrevChain = NULL;
        sIsCreateChain = ID_FALSE;
        sPreVar        = ST_NOT_SET;
        sPreRev        = ST_NOT_SET;
        sParent        = NULL;       

        for ( j = 0 ; j < sMax - 1 ; j++ )
        {
            sNextPt = STD_NEXT_PT2D(sPt);

            sIsSame = stdUtils::isSamePoints2D( sPt, sNextPt );
          
            if ( sIsSame == ID_TRUE )
            {
                if ( aValidation == ID_TRUE )
                {
                    IDE_RAISE(err_same_points);
                }
                else
                {
                    sPt = sNextPt;
                    continue;
                }
            }

            stdUtils::getSegProperty( sPt, 
                                      sNextPt,
                                      &sVaritation,
                                      &sReverse );

            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment),
                                      (void**) & sSeg )
                      != IDE_SUCCESS);

            sSeg->mBeginVertex = NULL;
            sSeg->mEndVertex = NULL;            

            if ( sReverse == ST_NOT_REVERSE )
            {
                idlOS::memcpy( &(sSeg->mStart), sPt, ID_SIZEOF(stdPoint2D));
                idlOS::memcpy( &(sSeg->mEnd), sNextPt, ID_SIZEOF(stdPoint2D));
            }
            else
            {
                idlOS::memcpy( &(sSeg->mEnd), sPt, ID_SIZEOF(stdPoint2D));
                idlOS::memcpy( &(sSeg->mStart), sNextPt, ID_SIZEOF(stdPoint2D));
            }

            sSeg->mPrev       = NULL;            
            sSeg->mNext       = NULL;                

            if ( sVaritation != sPreVar || sReverse != sPreRev )
            {
                if ( sParent != NULL)
                {
                    aIndexSeg[*aIndexSegTotal] = sParent->mBegin;                
                    (*aIndexSegTotal)++;
                }
                
                // 속성이 다르면 체인을 생성한다.
                IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Chain),
                                          (void**) & sChain )
                          != IDE_SUCCESS);
                
                if ( sIsCreateChain == ID_FALSE)
                {
                    sIsCreateChain = ID_TRUE;
                    sFirstChain = sChain;
                }
                
                initChain( sSeg, 
                           sChain, 
                           sVaritation, 
                           sReverse , 
                           i, 
                           aPolyNum, 
                           sOrientation,
                           ST_CHAIN_NOT_OPPOSITE );

                if ( sPrevChain != NULL )
                {
                    sChain->mPrev = sPrevChain;                    
                    sPrevChain->mNext = sChain;                    
                }
                else
                {
                    sChain->mNext = NULL;
                    sChain->mPrev = NULL;                    
                }
                
                sParent = sChain;
                IDE_TEST_RAISE( sParent == NULL, ERR_ABORT_PARENT_IS_NULL );
                sSeg->mParent = sParent;                    

                //aIndexSeg[*aIndexSegTotal] = sSeg;
                // Chain close 
                //(*aIndexSegTotal)++;
                
                sPreVar = sVaritation;
                sPreRev = sReverse;                                    
            }
            else
            {
                sParent = sChain;   
                IDE_TEST_RAISE( sParent == NULL, ERR_ABORT_PARENT_IS_NULL );
                sSeg->mParent = sParent;
                    
                if ( sReverse == ST_NOT_REVERSE )
                {
                    appendLastSeg( sSeg, sChain);                        
                }
                else
                {
                    appendFirstSeg( sSeg, sChain);                        
                }
            }
            sPt = sNextPt;
            sPrevChain = sChain;
        }

        if ( (sIsCreateChain == ID_TRUE) && (sChain != sFirstChain) )
        {
            aIndexSeg[*aIndexSegTotal] = sParent->mBegin;                
            (*aIndexSegTotal)++;  
            
            sChain->mNext =  sFirstChain;
            sFirstChain->mPrev = sChain;
            
            if( aRingSegList != NULL && aRingCount != NULL )
            {
                aRingSegList[*aRingCount] = sChain->mBegin;
                (*aRingCount)++;
            }

        }
        
        sRing = STD_NEXT_RN2D(sRing);        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_NUM_OF_POINTS )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::classfyPolygonChain",
                                  "invalid number of points" ));
    }
    IDE_EXCEPTION( ERR_ABORT_PARENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::classfyPolygonChain",
                                  "NULL segment parent" ));
    }
    IDE_EXCEPTION(err_same_points);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_POINTS));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


Segment* stdUtils::getNextSeg( Segment* aSeg )
{
    SInt     sReverse  = aSeg->mParent->mReverse;
    SInt     sOpposite = aSeg->mParent->mOpposite;
    Chain*   sChain; 
    Segment* sReturn;
   
    /* BUG-33634 
     * 방향이 잘못된 링의 방향을 바꾼 경우를 고려하도록 수정함. */ 
    if( sReverse == ST_NOT_REVERSE  )
    {
        sReturn = aSeg->mNext;
    }
    else
    {
        sReturn = aSeg->mPrev;
    }

    if( sReturn == NULL )
    {
        if ( sOpposite == ST_CHAIN_NOT_OPPOSITE )
        {
            sChain = aSeg->mParent->mNext;
        }
        else
        {
            sChain = aSeg->mParent->mPrev;
        }

        if( sChain->mReverse == ST_NOT_REVERSE )
        {
            sReturn = sChain->mBegin;            
        }
        else
        {
            sReturn = sChain->mEnd;            
        }
    }
    return sReturn;
}

Segment* stdUtils::getPrevSeg( Segment* aSeg )
{
    SInt     sReverse  = aSeg->mParent->mReverse;
    SInt     sOpposite = aSeg->mParent->mOpposite;
    Chain*   sChain;
    Segment* sReturn;

    /* BUG-33634 
     * 방향이 잘못된 링의 방향을 바꾼 경우를 고려하도록 수정함. */
    if( sReverse == ST_NOT_REVERSE )
    {
        sReturn = aSeg->mPrev;
    }
    else
    {
        sReturn = aSeg->mNext;
    }

    if( sReturn == NULL )
    {
        if( sOpposite == ST_CHAIN_NOT_OPPOSITE )
        {
            sChain = aSeg->mParent->mPrev;
        }
        else
        {
            sChain = aSeg->mParent->mNext;
        }

        if( sChain->mReverse == ST_NOT_REVERSE )
        {
            sReturn = sChain->mEnd;
        }
        else
        {
            sReturn = sChain->mBegin;
        }
    }   
    return sReturn;
}

void stdUtils::getSegProperty( const stdPoint2D *aPt1,
                               const stdPoint2D *aPt2,
                               SInt             *aVaritation,
                               SInt             *aReverse )
{

    
    if ( aPt1->mX == aPt2->mX )
    {
        *aVaritation = ST_INCREASE;
        if ( aPt1->mY <= aPt2->mY )
        {
            *aReverse = ST_NOT_REVERSE;
        }
        else
        {
            *aReverse = ST_REVERSE;
        }
    }
    else if ( aPt1->mX < aPt2->mX )
    {
    	*aReverse = ST_NOT_REVERSE;
    	if ( aPt1->mY <= aPt2->mY)
    	{		
    		*aVaritation = ST_INCREASE;
    	}
    	else
    	{
    		*aVaritation = ST_DECREASE;
    	}
    }
    else
    {
    	*aReverse = ST_REVERSE;
    	if ( aPt1->mY >= aPt2->mY)
    	{
    		*aVaritation = ST_INCREASE;
    	}
    	else
    	{
    		*aVaritation = ST_DECREASE;
    	}
    }
}

void stdUtils::initChain( Segment *aSeg,
                          Chain   *aChain,                              
                          SInt     aVaritaion,
                          SInt     aReverse,
                          UInt     aRingNum,
                          UInt     aPolygonNum,
                          SInt     aOrientation,
                          SInt     aOpposite )
{
    aChain->mBegin      = aChain->mEnd = aSeg;
    aChain->mVaritation = aVaritaion;
    aChain->mReverse    = aReverse;
    aChain->mRingNum    = aRingNum;
    aChain->mPolygonNum = aPolygonNum;
    aChain->mOrientaion = aOrientation;
    aChain->mOpposite   = aOpposite;
    aChain->mNext       = NULL;
    aChain->mPrev       = NULL;    
}

void stdUtils::appendLastSeg( Segment *aSeg,
                              Chain   *aChain)
{
    aSeg->mPrev = aChain->mEnd;
    aChain->mEnd->mNext = aSeg;
    aChain->mEnd        = aSeg;
}

void stdUtils::appendFirstSeg( Segment *aSeg,
                               Chain   *aChain)
{
    aChain->mBegin->mPrev = aSeg;    
    aSeg->mNext           = aChain->mBegin;
    aChain->mBegin        = aSeg;
}

IDE_RC stdUtils::addPrimInterSeg( iduMemory*     aQmxMem,
                                  PrimInterSeg** aHead,
                                  Segment*       aFirst,
                                  Segment*       aSecond,
                                  SInt           aStatus,
                                  SInt           aInterCount,
                                  stdPoint2D*    aInterPt )
{
    PrimInterSeg* sPrim;
    
    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(PrimInterSeg),
                              (void**)  &sPrim )
              != IDE_SUCCESS);

    sPrim->mFirst      = aFirst;
    sPrim->mSecond     = aSecond;
    sPrim->mStatus     = aStatus;
    sPrim->mInterCount = aInterCount;
    
    idlOS::memcpy(&(sPrim->mInterPt), aInterPt, ID_SIZEOF(stdPoint2D)* aInterCount);
        
    if ( *aHead == NULL )
    {
        (*aHead)        = sPrim;
        (*aHead)->mNext = NULL;
    }
    else
    {
        sPrim->mNext    = (*aHead)->mNext;
        (*aHead)->mNext = sPrim;        
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;     
}
 
IDE_RC stdUtils::addVertex( iduMemory*       aQmxMem,
                            Segment*         aSeg,
                            Vertex**         aVertex,
                            Vertex**         aResult,
                            const stdPoint2D aPt)
{
    Vertex*      sPtrVer;
    Vertex*      sPreVer;
    Vertex*      sNewVer;    
    idBool       sFound        = ID_FALSE;
    VertexEntry* sNewVertexEn;
    VertexEntry* sVertexEn;

    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(VertexEntry),
                              (void**)  &sNewVertexEn )
              != IDE_SUCCESS);

    sNewVertexEn ->mSeg = aSeg;    
    
    if (*aVertex == NULL )
    {
        IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Vertex),
                                  (void**) aVertex  )
                  != IDE_SUCCESS);

        *aResult            = *aVertex;
        (*aResult)->mCoord  = aPt;
        (*aResult)->mList   = sNewVertexEn;        
        (*aResult)->mNext   = NULL;
        sNewVertexEn->mNext = NULL;
        sNewVertexEn->mPrev = NULL;
        // 값 초기화 
    }
    else
    {
        sPtrVer = *aVertex;

        while( sPtrVer != NULL )
        {
            if ( isSamePoints2D4Func( &(sPtrVer->mCoord), &aPt ) == ID_TRUE )
            {
                sFound = ID_TRUE;                
                break;                
            }
            sPreVer = sPtrVer;            
            sPtrVer = sPtrVer-> mNext;
        }

        if ( sFound == ID_FALSE )
        {
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Vertex),
                                      (void**) &sNewVer )
                      != IDE_SUCCESS);

            // 값 초기화
            
            sPreVer->mNext      = sNewVer;            
            *aResult            = sNewVer;
            (*aResult)->mCoord  = aPt;
            (*aResult)->mList   = sNewVertexEn;        
            (*aResult)->mNext   = NULL;
            sNewVertexEn->mNext = NULL;
            sNewVertexEn->mPrev = NULL;
        }
        else
        {
            sVertexEn           = sPtrVer->mList;
            sNewVertexEn->mNext = sVertexEn->mNext;
            sVertexEn->mNext    = sNewVertexEn;
            sNewVertexEn->mPrev = sVertexEn;

            if ( sNewVertexEn->mNext != NULL )
            {
                sNewVertexEn->mNext->mPrev = sNewVertexEn;
            }

            *aResult = sPtrVer;        
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;   
    
}

IDE_RC stdUtils::reassign( iduMemory*    aQmxMem,
                           PrimInterSeg *aPrimHead,
                           idBool        aUseTolerance )

{
    PrimInterSeg * sPrimPtr;
    Segment*       sFindSeg;
    Segment*       sFindSeg2;
    
    sPrimPtr = aPrimHead;
   
    while( sPrimPtr != NULL )
    {
        sFindSeg = sPrimPtr->mFirst;

        while( sFindSeg != NULL )
        {
            // BUG-33436
            // Between2D를 checkBetween2D로 수정
            // 세그먼트가 분리됨에 따라 기존에 구한 교점과의 Area2D연산이 
            // 0이 아닌값이 나오는 경우가 있음
            // 세그먼트의 시작점과 끝점이 이루는 사각형 안에 포인트가 
            // 포함되는지 판단하는 것으로 대체함

            // BUG-40707
            // 부동소수점 연산오차로 인해 overlaps 연산시 에러발생
            // checkBetween2D를 checkBetween2D4Func로 수정
            
            if ( checkBetween2D4Func( &(sFindSeg->mStart),
                                      &(sFindSeg->mEnd),
                                      sPrimPtr->mInterPt )
                 == ID_TRUE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
            
            sFindSeg = sFindSeg->mNext;                    
        }

        sFindSeg2 = sPrimPtr->mSecond;
    
        while( sFindSeg2 != NULL )
        {
            if ( checkBetween2D4Func( &(sFindSeg2->mStart),
                                      &(sFindSeg2->mEnd),
                                      sPrimPtr->mInterPt )
                 == ID_TRUE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
            
            sFindSeg2 = sFindSeg2->mNext;
        }

        IDE_TEST_RAISE( sFindSeg  == NULL, ERR_ABORT_INVALID_SEGMENT );
        IDE_TEST_RAISE( sFindSeg2 == NULL, ERR_ABORT_INVALID_SEGMENT );

        IDE_TEST( reassignSeg( aQmxMem, 
                               sFindSeg,
                               sFindSeg2,
                               sPrimPtr->mStatus,
                               sPrimPtr->mInterCount,
                               sPrimPtr->mInterPt,
                               aUseTolerance )
                   != IDE_SUCCESS );
        
        sPrimPtr = sPrimPtr->mNext;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_SEGMENT )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::reassign",
                                  "Invalid segment" ));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

IDE_RC stdUtils::reassignSeg( iduMemory*  aQmxMem,
                              Segment*    aSeg1,
                              Segment*    aSeg2,
                              SInt        /* aStatus */,
                              SInt        aInterCnt,
                              stdPoint2D* aInterPt,
                              idBool      aUseTolerance )
{
    Segment*       sSeg      = NULL;
    Segment*       sSeg2     = NULL;    
    Segment*       sFindSeg1;
    Segment*       sFindSeg2;
    Segment*       sAnotherSeg1;
    Segment*       sAnotherSeg2;
    Vertex**       sVertex11;
    Vertex**       sVertex12;
    Vertex**       sVertex21;
    Vertex**       sVertex22;
    Vertex*        sNewVertex;
    VertexEntry*   sTempEntry;
    SInt           i;
    SInt           sState;

    sFindSeg1  = aSeg1;
    sFindSeg2  = aSeg2;

    for( i = 0 ; i < aInterCnt; i++ )
    {
        if ( aUseTolerance == ID_TRUE )
        {
            if ( isSamePoints2D4Func( &aInterPt[i], &sFindSeg1->mStart ) == ID_TRUE ) 
            {
                sState = 1;
            }
            else if ( isSamePoints2D4Func( &aInterPt[i], &sFindSeg1->mEnd ) == ID_TRUE )
            {
                sState = 2;
            }
            else
            {
                sState = 3;
            }
        }
        else
        {
            if ( isSamePoints2D( &aInterPt[i], &sFindSeg1->mStart ) == ID_TRUE ) 
            {
                sState = 1;
            }
            else if ( isSamePoints2D( &aInterPt[i], &sFindSeg1->mEnd ) == ID_TRUE )
            {
                sState = 2;
            }
            else
            {
                sState = 3;
            }
        }

        switch( sState )
        {
        case 1:
            sVertex11 = &sFindSeg1->mBeginVertex;

            if ( sFindSeg1->mParent->mReverse == ST_NOT_REVERSE )
            {
                sAnotherSeg1 = getPrevSeg( sFindSeg1 );
            }
            else
            {
                sAnotherSeg1 = getNextSeg( sFindSeg1 );
            }

            if ( sFindSeg1->mParent->mReverse == sAnotherSeg1->mParent->mReverse )
            {
                sVertex12 = &sAnotherSeg1->mEndVertex;
            }
            else
            {
                sVertex12 = &sAnotherSeg1->mBeginVertex;
            }
            break;

        case 2:
            sVertex11 = &sFindSeg1->mEndVertex;

            if ( sFindSeg1->mParent->mReverse == ST_NOT_REVERSE )
            {
                sAnotherSeg1 = getNextSeg( sFindSeg1 );
            }
            else
            {
                sAnotherSeg1 = getPrevSeg( sFindSeg1 );
            }

            if ( sFindSeg1->mParent->mReverse == sAnotherSeg1->mParent->mReverse )
            {
                sVertex12 = &sAnotherSeg1->mBeginVertex;
            }
            else
            {
                sVertex12 = &sAnotherSeg1->mEndVertex;
            }
            break;

        case 3:
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment),
                                      (void**)  &sSeg )
                      != IDE_SUCCESS);

            sSeg->mStart         = aInterPt[i];
            sSeg->mEnd           = sFindSeg1->mEnd;                
            sSeg->mParent        = sFindSeg1->mParent;
            sSeg->mPrev          = sFindSeg1;
            sSeg->mNext          = sFindSeg1->mNext;
            sSeg->mUsed          = ST_SEG_USABLE;
            sSeg->mLabel         = ST_SEG_LABEL_OUTSIDE;

            // BUG-33436 
            // 기존 세그먼트의 교점 정보를 새 세그먼트로 옮기고 
            // 기존 세그먼트를 가리키고 있는 것을 새 세그먼트로 수정
            sSeg->mEndVertex     = sFindSeg1->mEndVertex;
            sFindSeg1->mEndVertex = NULL;
            if ( sSeg->mEndVertex != NULL )
            {
                adjustVertex( sSeg->mEndVertex, sFindSeg1, sSeg );
            }
            else
            {
                // Nothing to do 
            }

            if ( sSeg->mNext != NULL )
            {                    
                sSeg->mNext->mPrev = sSeg;
            }
            
            sSeg->mBeginVertex = NULL;                

            if ( sFindSeg1->mParent->mEnd == sFindSeg1 )
            {
                sFindSeg1->mParent->mEnd = sSeg;                    
            }

            sFindSeg1->mEnd     = aInterPt[i];
            sFindSeg1->mNext    = sSeg;

            sAnotherSeg1 = sFindSeg1;
            sFindSeg1    = sFindSeg1->mNext;

            sVertex11 = &sFindSeg1->mBeginVertex;
            sVertex12 = &sAnotherSeg1->mEndVertex;
            break;
        }

        if ( aUseTolerance == ID_TRUE )
        {
            if ( isSamePoints2D4Func( &aInterPt[i], &sFindSeg2->mStart ) == ID_TRUE ) 
            {
                sState = 1;
            }
            else if ( isSamePoints2D4Func( &aInterPt[i], &sFindSeg2->mEnd ) == ID_TRUE )
            {
                sState = 2;
            }
            else
            {
                sState = 3;
            }
        }
        else
        {
            if ( isSamePoints2D( &aInterPt[i], &sFindSeg2->mStart ) == ID_TRUE ) 
            {
                sState = 1;
            }
            else if ( isSamePoints2D( &aInterPt[i], &sFindSeg2->mEnd ) == ID_TRUE )
            {
                sState = 2;
            }
            else
            {
                sState = 3;
            }
        }

        switch( sState )
        {
        case 1:
            sVertex21 = &sFindSeg2->mBeginVertex;

            if ( sFindSeg2->mParent->mReverse == ST_NOT_REVERSE )
            {
                sAnotherSeg2 = getPrevSeg( sFindSeg2 );
            }
            else
            {
                sAnotherSeg2 = getNextSeg( sFindSeg2 );
            }

            if ( sFindSeg2->mParent->mReverse == sAnotherSeg2->mParent->mReverse )
            {
                sVertex22 = &sAnotherSeg2->mEndVertex;
            }
            else
            {
                sVertex22 = &sAnotherSeg2->mBeginVertex;
            }
            break;

        case 2:
            sVertex21 = &sFindSeg2->mEndVertex;

            if ( sFindSeg2->mParent->mReverse == ST_NOT_REVERSE )
            {
                sAnotherSeg2 = getNextSeg( sFindSeg2 );
            }
            else
            {
                sAnotherSeg2 = getPrevSeg( sFindSeg2 );
            }

            if ( sFindSeg2->mParent->mReverse == sAnotherSeg2->mParent->mReverse )
            {
                sVertex22 = &sAnotherSeg2->mBeginVertex;
            }
            else
            {
                sVertex22 = &sAnotherSeg2->mEndVertex;
            }
            break;

        case 3:
            IDE_TEST( aQmxMem->alloc( ID_SIZEOF(Segment),
                                      (void**)  &sSeg2) 
                      != IDE_SUCCESS);

            sSeg2->mBeginVertex = NULL;                

            sSeg2->mStart         = aInterPt[i];
            sSeg2->mEnd           = sFindSeg2->mEnd;                
            sSeg2->mParent        = sFindSeg2->mParent;
            sSeg2->mPrev          = sFindSeg2;
            sSeg2->mNext          = sFindSeg2->mNext;
            sSeg2->mUsed          = ST_SEG_USABLE;
            sSeg2->mLabel         = ST_SEG_LABEL_OUTSIDE;

            // BUG-33436 
            // 기존 세그먼트의 교점 정보를 새 세그먼트로 옮기고 
            // 기존 세그먼트를 가리키고 있는 것을 새 세그먼트로 수정
            sSeg2->mEndVertex     = sFindSeg2->mEndVertex;
            sFindSeg2->mEndVertex = NULL;
            if ( sSeg2->mEndVertex != NULL )
            {
                adjustVertex( sSeg2->mEndVertex, sFindSeg2, sSeg2 );
            }
            else
            {
                // Nothing to do
            }

            if ( sSeg2->mNext != NULL )
            {
                sSeg2->mNext ->mPrev = sSeg2;
            }

            if (sFindSeg2->mParent->mEnd == sFindSeg2 )
            {
                sFindSeg2->mParent->mEnd = sSeg2;                    
            }

            sFindSeg2->mEnd  = aInterPt[i];
            sFindSeg2->mNext = sSeg2;

            sAnotherSeg2 = sFindSeg2;
            sFindSeg2    = sFindSeg2->mNext;

            sVertex21 = &sFindSeg2->mBeginVertex;
            sVertex22 = &sAnotherSeg2->mEndVertex;
            break;
        }

        IDE_TEST_RAISE ( *sVertex11 != *sVertex12, ERR_ABORT_INVALID_VERTEX_INFO );
        IDE_TEST_RAISE ( *sVertex21 != *sVertex22, ERR_ABORT_INVALID_VERTEX_INFO );

        if ( ( *sVertex11 != NULL ) && ( *sVertex22 != NULL ) )
        {
            if ( *sVertex11 != *sVertex22 )
            {
                if ( (*sVertex11)->mList != (*sVertex22)->mList )
                {
                    (*sVertex11)->mList->mNext->mPrev = (*sVertex22)->mList;
                    (*sVertex22)->mList->mNext->mPrev = (*sVertex11)->mList;
                    sTempEntry = (*sVertex11)->mList->mNext;
                    (*sVertex11)->mList->mNext = (*sVertex22)->mList->mNext;
                    (*sVertex22)->mList->mNext = sTempEntry;
                    (*sVertex22)->mList = (*sVertex11)->mList;
                }
                else
                {
                    // Nothing to do 
                }

                *sVertex21 = *sVertex11;
                *sVertex22 = *sVertex11;
            }
            else
            {
                // Nothing to do 
            }

        }
        else
        {
            if ( *sVertex11 != NULL )
            {
                // sVertex2 가 NULL
                IDE_TEST( addSegmentToVertex( aQmxMem, *sVertex11, sFindSeg2 )    != IDE_SUCCESS );
                IDE_TEST( addSegmentToVertex( aQmxMem, *sVertex11, sAnotherSeg2 ) != IDE_SUCCESS );
                *sVertex21 = *sVertex11;
                *sVertex22 = *sVertex11;
            }
            else if ( *sVertex22 != NULL )
            {
                // sVertex1 이 NULL
                IDE_TEST( addSegmentToVertex( aQmxMem, *sVertex22, sFindSeg1 )    != IDE_SUCCESS );
                IDE_TEST( addSegmentToVertex( aQmxMem, *sVertex22, sAnotherSeg1 ) != IDE_SUCCESS );
                *sVertex11 = *sVertex22;
                *sVertex12 = *sVertex22;
            }
            else
            {
                // 둘다 NULL
                IDE_TEST( aQmxMem->alloc( ID_SIZEOF( Vertex ),
                                          (void**) &sNewVertex )
                          != IDE_SUCCESS );

                sNewVertex->mList  = NULL;
                sNewVertex->mCoord = aInterPt[i];
                sNewVertex->mNext  = NULL;

                IDE_TEST( addSegmentToVertex( aQmxMem, sNewVertex, sFindSeg2 )    != IDE_SUCCESS );
                IDE_TEST( addSegmentToVertex( aQmxMem, sNewVertex, sAnotherSeg2 ) != IDE_SUCCESS );
                IDE_TEST( addSegmentToVertex( aQmxMem, sNewVertex, sFindSeg1 )    != IDE_SUCCESS );
                IDE_TEST( addSegmentToVertex( aQmxMem, sNewVertex, sAnotherSeg1 ) != IDE_SUCCESS );

                *sVertex11 = sNewVertex;
                *sVertex12 = sNewVertex;
                *sVertex21 = sNewVertex;
                *sVertex22 = sNewVertex;
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_VERTEX_INFO )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::reassignSeg",
                                  "Invalid Vertex Info" ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

IDE_RC stdUtils::addSegmentToVertex( iduMemory* aQmxMem,
                                     Vertex*    aVertex,
                                     Segment*   aSegment )
{
    VertexEntry* sNewEntry;

    IDE_TEST( aQmxMem->alloc( ID_SIZEOF(VertexEntry),
                              (void**) &sNewEntry )
              != IDE_SUCCESS );

    sNewEntry->mSeg  = aSegment;

    IDE_TEST_RAISE( aVertex == NULL, ERR_ABORT_INVALID_VERTEX );

    if ( aVertex->mList == NULL )
    {
        aVertex->mList = sNewEntry;
        sNewEntry->mPrev = sNewEntry;
        sNewEntry->mNext = sNewEntry;
    }
    else
    {
        sNewEntry->mNext = aVertex->mList;
        sNewEntry->mPrev = aVertex->mList->mPrev;
        aVertex->mList->mPrev->mNext = sNewEntry;
        aVertex->mList->mPrev = sNewEntry;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_VERTEX )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::addSegmentToVertex",
                                  "Invalid Vertex" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool stdUtils::isRingInSide( Segment**  aIndexSeg,
                               UInt       aIndexSegTotal,
                               Segment*   aCmpSeg,
                               UInt       aPolyCmpMin,
                               UInt       aPolyCmpMax,
                               UChar*     aCount)
{
    UInt       i;
    Segment*   sCurrSeg;
    Chain*     sParentChain;
    UInt       sCount        = 0;
    idBool     sReturn       = ID_FALSE;
    stdPoint2D sTempPt;    
    SDouble    sSlope;
    SDouble    sY;

    if ( aCount != NULL )
    {
        // Init Count array
        for ( i = 0 ; i < aPolyCmpMax ; i++ )
        {
            aCount[i] = 0;
        }
    }
    else
    {
        // Nothing to do
    }

    sTempPt.mX = ( aCmpSeg->mStart.mX + aCmpSeg->mEnd.mX ) / 2;
    sTempPt.mY = ( aCmpSeg->mStart.mY + aCmpSeg->mEnd.mY ) / 2;

    for ( i=0; i< aIndexSegTotal && aIndexSeg[i]->mStart.mX <= sTempPt.mX ; i++ )
    {
        sParentChain = aIndexSeg[i]->mParent;

        if ( sParentChain->mEnd->mEnd.mX < sTempPt.mX )
        {
            continue;
        }
        
        if ( ( sParentChain->mPolygonNum < aPolyCmpMin ) || ( sParentChain->mPolygonNum > aPolyCmpMax ) )
        {
            continue;            
        }
        
        if ( ( sParentChain->mPolygonNum == aCmpSeg->mParent->mPolygonNum ) &&
             ( sParentChain->mRingNum    == aCmpSeg->mParent->mRingNum ) )
        {
            continue;            
        }

        sCurrSeg = aIndexSeg[i];

        while ( sCurrSeg != NULL)
        {   
            if ( sCurrSeg->mEnd.mX > sTempPt.mX  )
            {
                break;
            }
            sCurrSeg = sCurrSeg->mNext;            
        }

        if ( sCurrSeg != NULL )
        {
            if ( sCurrSeg->mStart.mX != sCurrSeg->mEnd.mX )
            {
                if( sCurrSeg->mStart.mX == sTempPt.mX )
                {
                    sY = sCurrSeg->mStart.mY;
                }
                else
                {
                    sSlope = ( sCurrSeg->mStart.mY - sCurrSeg->mEnd.mY ) / 
                             ( sCurrSeg->mStart.mX - sCurrSeg->mEnd.mX );
        
                    sY     = ( sSlope * ( sTempPt.mX - sCurrSeg->mStart.mX ) ) + 
                             sCurrSeg->mStart.mY;
                }

                if( sTempPt.mY >= sY )
                {
                    if ( aCount == NULL )
                    {
                        sCount++;
                    }
                    else
                    {
                        /* BUG-33904
                         * UChar의 최대값 255에서 1을 더 했을때 0이 되므로
                         * 홀, 짝 계산에 문제가 없다 */
                        aCount[sCurrSeg->mParent->mPolygonNum]++;
                    }
                }
                else
                {
                    /* nothing to do */
                }
            }            
        }
    }
    
    if ( aCount == NULL )
    {
        if ( sCount % 2  == 1 )
        {
            sReturn = ID_TRUE;        
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        for ( i = 0 ; i < aPolyCmpMax ; i++ )
        {
            if ( aCount[i] % 2 == 1 )
            {
                sReturn = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do
            }
        }
    }

    return sReturn;
}

//BUG-33436
/***********************************************************************
 * Description:
 *  세그먼트 분리 후 기존 세그먼트를 가리키고 있는 교점 정보를 
 *  수정하여 새 세그먼트를 가리키도록 수정한다.
 *
 *  Vertex*  aVertex (InOut) : 교점에 대한 포인터
 *  Segment* aOldSeg (In)    : 기존 세그먼트의 포인터
 *  Segment* aNewSeg (In)    : 새 세그먼트의 포인터
 **********************************************************************/
void stdUtils::adjustVertex( Vertex*  aVertex, 
                             Segment* aOldSeg, 
                             Segment* aNewSeg )
{
    VertexEntry* sVerEntry;

    IDE_ASSERT( aVertex != NULL );
    IDE_ASSERT( aVertex->mList != NULL );

    sVerEntry = aVertex->mList;
    do
    {
        if( sVerEntry->mSeg == aOldSeg )
        {
            sVerEntry->mSeg = aNewSeg;
        }
        sVerEntry = sVerEntry->mNext;
    } while ( sVerEntry != aVertex->mList );
}

IDE_RC stdUtils::setVertex( iduMemory*       aQmxMem,
                            Segment*         aSeg,
                            idBool           aIsStart,
                            Vertex**         aVertexRoot,
                            const stdPoint2D aPt )
{
    Vertex*  sInterVer;
    Segment* sAnotherSeg;

    if ( aIsStart == ID_TRUE ) 
    {
        sAnotherSeg = aSeg->mPrev;
        if ( sAnotherSeg == NULL )
        {
            if ( aSeg->mParent->mReverse == ST_NOT_REVERSE )
            {
                sAnotherSeg = getPrevSeg( aSeg );
            }
            else
            {
                sAnotherSeg = getNextSeg( aSeg );
            }
        }
        else
        {
            // Nothing to do 
        }
    }
    else
    {
        sAnotherSeg = aSeg->mNext;
        if ( sAnotherSeg == NULL )
        {
            if ( aSeg->mParent->mReverse == ST_NOT_REVERSE )
            {
                sAnotherSeg = getNextSeg( aSeg );
            }
            else
            {
                sAnotherSeg = getPrevSeg( aSeg );
            }
        }
        else
        {
            // Nothing to do 
        }
    }

    IDE_TEST( addVertex( aQmxMem, aSeg, aVertexRoot, &sInterVer, aPt ) 
              != IDE_SUCCESS );
    IDE_TEST( addVertex( aQmxMem, sAnotherSeg, aVertexRoot, &sInterVer, aPt )
              != IDE_SUCCESS );

    if ( aIsStart == ID_TRUE )
    {
        aSeg->mBeginVertex = sInterVer;
        if ( aSeg->mParent->mReverse == sAnotherSeg->mParent->mReverse )
        {
            sAnotherSeg->mEndVertex = sInterVer;
        }
        else
        {
            sAnotherSeg->mBeginVertex = sInterVer;
        }
    }
    else
    {
        aSeg->mEndVertex = sInterVer;
        if ( aSeg->mParent->mReverse == sAnotherSeg->mParent->mReverse )
        {
            sAnotherSeg->mBeginVertex = sInterVer;
        }
        else
        {
            sAnotherSeg->mEndVertex = sInterVer;
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}

IDE_RC stdUtils::intersectCCW4Func( const stdPoint2D   aPt1,
                                    const stdPoint2D   aPt2,
                                    const stdPoint2D   aPt3,
                                    const stdPoint2D   aPt4,
                                    SInt             * aStatus,
                                    SInt             * aNumPoints,
                                    stdPoint2D       * aResultPoint)

{    
    stdMBR sMBR1;
    stdMBR sMBR2;    

    *aStatus = ST_NOT_INTERSECT;

    IDE_TEST_RAISE ( aPt1.mX > aPt2.mX, ERR_ABORT_INVALID_POINT_ORDER );
    IDE_TEST_RAISE ( aPt3.mX > aPt4.mX, ERR_ABORT_INVALID_POINT_ORDER );
    
    if ( aPt1.mY > aPt2.mY )
    {
        sMBR1.mMinY = aPt2.mY;
        sMBR1.mMaxY = aPt1.mY;           
    }
    else
    {
        sMBR1.mMinY = aPt1.mY;
        sMBR1.mMaxY = aPt2.mY;     
    }
    
    sMBR1.mMinX = aPt1.mX;
    sMBR1.mMaxX = aPt2.mX;

    if ( aPt3.mY > aPt4.mY )
    {
        sMBR2.mMinY = aPt4.mY;
        sMBR2.mMaxY = aPt3.mY;          
    }
    else
    {
        sMBR2.mMinY = aPt3.mY;
        sMBR2.mMaxY = aPt4.mY;        
    }

    sMBR2.mMinX = aPt3.mX;
    sMBR2.mMaxX = aPt4.mX;
    
    if (isMBRIntersects( &sMBR1, &sMBR2 ) == ID_FALSE )
    {
        *aStatus = ST_NOT_INTERSECT;    
    }
    else
    {
        if ( getIntersectPoint4Func( aPt1, aPt2, aPt3, aPt4, aStatus, aResultPoint ) 
             == ID_TRUE )
        {
            switch( *aStatus )
            {
                case ST_TOUCH:
                case ST_POINT_TOUCH:
                case ST_INTERSECT:
                    *aNumPoints = 1;
                    break;
                case ST_SHARE:
                    *aNumPoints = 2;
                    break;
                case ST_NOT_INTERSECT:
                    break;
                default:
                    IDE_RAISE( ERR_ABORT_INVALID_SEGMENT );
                    break;
            }
        }
        else
        {
            // Nothing to do 
        }
    }        
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_INVALID_POINT_ORDER )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::intersectCCW",
                                  "invalid point order" ));
    }

    IDE_EXCEPTION( ERR_ABORT_INVALID_SEGMENT )
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_UNEXPECTED_ERROR,
                                  "stdUtils::intersectCCW",
                                  "invalid segment"));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool stdUtils::getIntersectPoint4Func( const stdPoint2D aPt1,
                                         const stdPoint2D aPt2, 
                                         const stdPoint2D aPt3,
                                         const stdPoint2D aPt4,
                                         SInt*            aStatus,
                                         stdPoint2D*      aResult)    
{
    UInt       i;
    idBool     sIncl[4];
    stdPoint2D sTmp[4];
    idBool     sRet = ID_FALSE;
    SDouble    sT;
    SDouble    sT2;
    SDouble    sS;
    SDouble    sS2;
    SDouble    sUnder;

    sTmp[0] = aPt1;
    sTmp[1] = aPt2;
    sTmp[2] = aPt3;
    sTmp[3] = aPt4;

    *aStatus = ST_NOT_INTERSECT;

    sIncl[0] = ( getDistanceSqLineSegmentToPoint2D(&aPt3, &aPt4, &aPt1) <= STU_CLIP_TOLERANCESQ ) ?
               ID_TRUE : ID_FALSE;
    sIncl[1] = ( getDistanceSqLineSegmentToPoint2D(&aPt3, &aPt4, &aPt2) <= STU_CLIP_TOLERANCESQ ) ?
               ID_TRUE : ID_FALSE;
    sIncl[2] = ( getDistanceSqLineSegmentToPoint2D(&aPt1, &aPt2, &aPt3) <= STU_CLIP_TOLERANCESQ ) ?
               ID_TRUE : ID_FALSE;   
    sIncl[3] = ( getDistanceSqLineSegmentToPoint2D(&aPt1, &aPt2, &aPt4) <= STU_CLIP_TOLERANCESQ ) ?
               ID_TRUE : ID_FALSE;

    for ( i = 0 ; i < 4 ; i++ )
    {
        if ( sIncl[i] == ID_TRUE )
        {
            if ( *aStatus == ST_NOT_INTERSECT ) 
            {
                *aStatus    = ST_TOUCH;
                aResult[0]  = sTmp[i];
                sRet        = ID_TRUE;
            }
            else
            {
                if ( isSamePoints2D4Func( &aResult[0], &sTmp[i] ) != ID_TRUE )
                {
                    *aStatus    = ST_SHARE;
                    aResult[1]  = sTmp[i];
                    sRet        = ID_TRUE;
                }
                else
                {
                    *aStatus = ST_POINT_TOUCH;
                }
                break;
            }
        }
        else
        {
            // Nothing to do
        }
    }
    
    if ( ( sRet == ID_FALSE ) && ( *aStatus == ST_NOT_INTERSECT ) )
    {
        sUnder = (aPt4.mY - aPt3.mY) * (aPt2.mX - aPt1.mX) - (aPt4.mX - aPt3.mX) * (aPt2.mY - aPt1.mY);
        sT2 = (aPt4.mX - aPt3.mX) * (aPt1.mY - aPt3.mY) - (aPt4.mY - aPt3.mY) * (aPt1.mX - aPt3.mX);
        sS2 = (aPt2.mX - aPt1.mX) * (aPt1.mY - aPt3.mY) - (aPt2.mY - aPt1.mY) * (aPt1.mX - aPt3.mX); 

        sT = sT2 / sUnder;
        sS = sS2 / sUnder;
        if ( (sT < 0.0) || (sT > 1.0) || (sS < 0.0) || (sS > 1.0) )
        {
            *aStatus = ST_NOT_INTERSECT;                    
            sRet     = ID_FALSE;            
        }
        else
        {
            *aStatus = ST_INTERSECT;
            sRet     = ID_TRUE;

            aResult[0].mX = aPt1.mX + sT * (aPt2.mX - aPt1.mX);
            aResult[0].mY = aPt1.mY + sT * (aPt2.mY - aPt1.mY);
        }
    }
    else
    {
        // Nohting to do
    }

    return sRet;
}


idBool stdUtils::isSamePoints2D4Func( const stdPoint2D* aPt1,
                                      const stdPoint2D* aPt2 )
{
    SDouble dist;
    idBool  sRet;

    dist = (aPt1->mX - aPt2->mX) * (aPt1->mX - aPt2->mX ) + 
           (aPt1->mY - aPt2->mY) * (aPt1->mY - aPt2->mY );

    if ( dist < STU_CLIP_TOLERANCESQ ) 
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

idBool stdUtils::checkBetween2D4Func( const stdPoint2D *aPt1,
                                      const stdPoint2D *aPt2,
                                      const stdPoint2D *aPt )
{
    idBool sRet;

    SDouble sMinX;
    SDouble sMaxX;
    SDouble sMinY;
    SDouble sMaxY;

    if( aPt1->mX < aPt2->mX )
    {
        sMinX = aPt1->mX - STU_CLIP_TOLERANCE;
        sMaxX = aPt2->mX + STU_CLIP_TOLERANCE;
    }
    else
    {
        sMinX = aPt2->mX - STU_CLIP_TOLERANCE;
        sMaxX = aPt1->mX + STU_CLIP_TOLERANCE;
    }

    if( aPt1->mY < aPt2->mY )
    {
        sMinY = aPt1->mY - STU_CLIP_TOLERANCE;
        sMaxY = aPt2->mY + STU_CLIP_TOLERANCE;
    }
    else
    {
        sMinY = aPt2->mY - STU_CLIP_TOLERANCE;
        sMaxY = aPt1->mY + STU_CLIP_TOLERANCE;
    }
  
    if( ( (sMinX - aPt->mX) * (aPt->mX - sMaxX) >= 0 ) &&
        ( (sMinY - aPt->mY) * (aPt->mY - sMaxY) >= 0 ) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

IDE_RC stdUtils::checkValid( const stdGeometryHeader* aHeader )
{

    IDE_TEST_RAISE( aHeader->mIsValid == ST_INVALID, ERR_INVALID_OBJ );
    IDE_TEST_RAISE( aHeader->mIsValid == ST_UNKNOWN, ERR_UNKNOWN_OBJ );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_OBJ );
    {
        IDE_SET(ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY ));
    }

    IDE_EXCEPTION( ERR_UNKNOWN_OBJ );
    {
        IDE_SET(ideSetErrorCode( stERR_ABORT_INVALID_GEOMETRY ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SInt stdUtils::isPointInside( Segment**   aIndexSeg,
                              UInt        aIndexSegTotal,
                              stdPoint2D* aPt,
                              UInt        aPolyCmpMin,
                              UInt        aPolyCmpMax )
{
    UInt     i;
    UChar    sCount = 0;
    Chain*   sChain;
    Segment* sCurrSeg;
    SDouble  sSlope;
    SDouble  sY;
    SDouble  sMinY;
    SDouble  sMaxY;

    for ( i = 0 ; ( i < aIndexSegTotal ) && ( aIndexSeg[i]->mStart.mX <= aPt->mX ) ; i++ )
    {
        sChain = aIndexSeg[i]->mParent;

        sMinY = ( sChain->mVaritation == ST_INCREASE ) ? 
            sChain->mBegin->mStart.mY :
            sChain->mEnd->mEnd.mY;

        sMaxY = ( sChain->mVaritation == ST_INCREASE ) ? 
            sChain->mEnd->mEnd.mY :
            sChain->mBegin->mStart.mY;

        if ( sChain->mEnd->mEnd.mX < aPt->mX )
        {
            continue;
        }

        if ( ( sChain->mPolygonNum < aPolyCmpMin ) || ( sChain->mPolygonNum > aPolyCmpMax ) )
        {
            continue;            
        }

        if ( sMinY > aPt->mY )
        {
            continue;
        }

        if ( ( sMaxY < aPt->mY ) && ( aIndexSeg[i]->mStart.mX != aPt->mX ) )
        {
            sCount++;
            continue;
        }

        sCurrSeg = aIndexSeg[i];

        while ( sCurrSeg != NULL )
        {   
            if ( ( sCurrSeg->mStart.mX <= aPt->mX ) && ( aPt->mX <= sCurrSeg->mEnd.mX ) )
            {
                if ( between2D( &sCurrSeg->mStart, &sCurrSeg->mEnd, aPt ) == ID_TRUE )
                {
                    return ST_POINT_ONBOUND;
                }
                else
                {
                    if ( sCurrSeg->mStart.mX != aPt->mX )
                    {
                        sSlope = ( sCurrSeg->mStart.mY - sCurrSeg->mEnd.mY ) / 
                            ( sCurrSeg->mStart.mX - sCurrSeg->mEnd.mX );

                        sY     = ( sSlope * ( aPt->mX - sCurrSeg->mStart.mX ) ) + 
                            sCurrSeg->mStart.mY;

                        if( aPt->mY >= sY )
                        {
                            sCount++;
                        }
                        else
                        {
                            // Nothing to do 
                        }
                    }
                }
            }
            else
            {
                if ( sCurrSeg->mStart.mX > aPt->mX )
                {
                    break;
                }
                else
                {
                    // Nothing to do 
                }
            }
            sCurrSeg = sCurrSeg->mNext;            
        }
    }

    return ( sCount % 2 == 1 ) ? ST_POINT_INSIDE : ST_POINT_OUTSIDE;
}
 
