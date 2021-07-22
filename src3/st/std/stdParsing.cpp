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
 * Copyright
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: stdParsing.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * 입력 버퍼로 부터 WKT(Well Known Text) 또는 WKB(Well Known Binary)를 읽어
 * Geometry 객체로 저장하는 모듈 구현
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>
#include <mtuProperty.h>

#include <ste.h>
#include <stdTypes.h>
#include <stdUtils.h>
#include <stdPrimitive.h>
#include <stdParsing.h>
#include <stcDef.h>
#include <stuProperty.h>

// BUG-24357 WKB Endian
extern mtdModule stdGeometry;
extern mtdModule mtdSmallint;
extern mtdModule mtdInteger;
extern mtdModule mtdDouble;

/***********************************************************************
 * Description:
 * 다음에 읽어들일 토큰이 어떤 유형인지 판별한다.
 *
 * STT_NUM_TOKEN: 숫자
 * STT_LPAREN_TOKEN: 좌소괄호
 * STT_RPAREN_TOKEN: 우소괄호
 * STT_COMMA_TOKEN: 쉼표
 * STT_SPACE_TOKEN: 공백 문자
 * STT_CHAR_TOKEN: 문자
 *
 * UChar **aPtr(In): 읽어들일 문자열 위치.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
STT_TOKEN stdParsing::testNextToken(UChar **aPtr, UChar *aWKTFence)
{
    STT_TOKEN sRet = STT_SPACE_TOKEN;    
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);

    // To Fix : BUG-15954  3D Point시 Z가 음수일 때 에러남.
    //          '-' 체크를 추가함
    if( ((**aPtr >= '0') && (**aPtr <= '9')) || (**aPtr == '-' ) )
    {
        sRet = STT_NUM_TOKEN;
    }
    else if(**aPtr == '(')
    {
        sRet = STT_LPAREN_TOKEN;
    }
    else if(**aPtr == ')')
    {
        sRet = STT_RPAREN_TOKEN;
    }
    else if(**aPtr == ',')
    {
        sRet = STT_COMMA_TOKEN;
    }
    else if( **aPtr == ';' )
    {
        sRet = STT_SEMICOLON_TOKEN;
    }
    else if( **aPtr == '=' )
    {
        sRet = STT_EQUAL_TOKEN;
    }
    else if(isSpace(**aPtr)==ID_TRUE)
    {
        //sRet = STT_SPACE_TOKEN;
    }
    else
    {
        sRet = STT_CHAR_TOKEN;
    }
    
    return sRet;
    
    IDE_EXCEPTION_END;

    return sRet;
}

/***********************************************************************
 * Description:
 * 공백 문자를 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipSpace(UChar **aPtr, UChar *aWKTFence)
{
    while(1)
    {
        // Fix BUG-15980
        IDE_TEST_RAISE( (*aPtr == aWKTFence) || (**aPtr == '\0') , err_parsing)  // end of WKT
        if( isSpace(**aPtr) == ID_TRUE )
        {
            (*aPtr)++;
        }
        else
        {
            break;
        }
    }
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * BUG-32531 Consider for GIS EMPTY
 * EMPTY 이후의 공백 문자를 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::emptyCheckSpace(UChar **aPtr, UChar *aWKTFence)
{
    while(1)
    {
        if (*aPtr == aWKTFence)
        {
			break;
        }
        else
        {
			// nothing todo
        }
			
        if( isSpace(**aPtr) == ID_TRUE )
        {
			(*aPtr)++;
        }
        else
        {
			return IDE_FAILURE;
        }
    }
			
    return IDE_SUCCESS;
}
			
/***********************************************************************
 * Description:
 * BUG-32531 Consider for GIS EMPTY
 * empty 인지 체크 하여 empty인 경우 retrun IDE_SUCCESS.
 * empty 구문 체크 이상시 error
 *
 * UChar **aPtr(In): 비교 할 문자
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::checkValidEmpty(UChar** aPtr, UChar *aWKTFence)
{
    UChar* sPtr = NULL;
			
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
			
    sPtr = *aPtr;
			
    sPtr = sPtr + STD_EMPTY_NAME_LEN;
			
    IDE_TEST_RAISE( emptyCheckSpace(&sPtr, aWKTFence) != IDE_SUCCESS,
                    err_parsing );
			
    *aPtr = sPtr;
			
    return IDE_SUCCESS;
			
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION_END;
			
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 좌소괄호를 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipLParen(UChar** aPtr, UChar *aWKTFence)
{
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
    IDE_TEST_RAISE(**aPtr != '(', err_parsing);
    (*aPtr)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 우소괄호를 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipRParen(UChar** aPtr, UChar *aWKTFence)
{
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
    IDE_TEST_RAISE(**aPtr != ')', err_parsing);
    (*aPtr)++;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 문장의 끝까지 공백으로 끝나는지 판별한다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipToLast(UChar** aPtr, UChar *aWKTFence)
{
    IDE_TEST_RAISE(skipSpace(aPtr, aWKTFence) == IDE_SUCCESS, err_parsing);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * Collection 내부 객체의 다음 객체가 있는지 판별하기 위해 사용한다.
 * 다음 객체가 있으면 해당 문자까지의 주소를 없으면 aWKTFence을 리턴한다.
 *
 * UChar **aPtr(In): 읽어들일 문자열 위치
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
UChar* stdParsing::findSubObjFence(UChar** aPtr, UChar* aWKTFence)
{
    UChar*  sPtr        = *aPtr;
    UChar*  sRet;
    UChar*  sLastComma  = NULL;
    UChar*  sLastRParen = NULL;
    UInt    sCnt  = 0;

    while(1)
    {
        if( (sCnt > 1 ) ||(sPtr >= aWKTFence))  // Found SubObj or end of WKT
        {
            break;
        }

        switch(*sPtr)
        {
        case ',' :
            sLastComma = sPtr;
        case ')' :
            sLastRParen = sPtr;
        case 'p' :
        case 'P' :            
            if(idlOS::strncasecmp((SChar*)sPtr, STD_POINT_NAME, 
                STD_POINT_NAME_LEN) == 0)
            {
                if( sCnt == 0 )
                {
                    sPtr += STD_POINT_NAME_LEN;
                }
                sCnt++;
            }
            else if(idlOS::strncasecmp((SChar*)sPtr, STD_POLYGON_NAME, 
                STD_POLYGON_NAME_LEN) == 0)
            {
                if( sCnt == 0 )
                {
                    sPtr += STD_POLYGON_NAME_LEN;
                }
                sCnt++;
            }
            break;
        case 'l' :
        case 'L' :  
            if(idlOS::strncasecmp((SChar*)sPtr, STD_LINESTRING_NAME, 
                STD_LINESTRING_NAME_LEN) == 0)
            {
                if( sCnt == 0 )
                {
                    sPtr += STD_LINESTRING_NAME_LEN;
                }
                sCnt++;
            }            
            break;
        case 'm' :
        case 'M' :            
            if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTIPOINT_NAME, 
                STD_MULTIPOINT_NAME_LEN) == 0)
            {
                if( sCnt == 0 )
                {
                    sPtr += STD_MULTIPOINT_NAME_LEN;
                }
                sCnt++;
            }
            else if(idlOS::strncasecmp((SChar*)sPtr,STD_MULTILINESTRING_NAME,
                STD_MULTILINESTRING_NAME_LEN) == 0)
            {
                if( sCnt == 0 )
                {
                    sPtr += STD_MULTILINESTRING_NAME_LEN;
                }
                sCnt++;
            }            
            else if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTIPOLYGON_NAME, 
                STD_MULTIPOLYGON_NAME_LEN) == 0)
            {
                if( sCnt == 0 )
                {
                    sPtr += STD_MULTIPOLYGON_NAME_LEN;
                }
                sCnt++;
            }
        default :
            break;
        }
        sPtr++;        
    } // while

    if( sCnt == 0 )
    {
        sRet = aWKTFence;
    }
    else if( sCnt == 1 )
    {
        if( sLastRParen != NULL )
        {
            sRet = sLastRParen;        
        }
        else
        {
            sRet = aWKTFence;
        }
    }
    else
    {
        if( sLastComma != NULL )
        {
            sRet = sLastComma;        
        }
        else
        {
            sRet = aWKTFence;
        }
    }
    
    return sRet;
}

/***********************************************************************
 * Description:
 * 쉼표를 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipComma(UChar** aPtr, UChar *aWKTFence)
{
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
    IDE_TEST_RAISE(**aPtr != ',', err_parsing);
    (*aPtr)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * ;을 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipSemicolon( UChar** aPtr, UChar *aWKTFence )
{
    IDE_TEST( skipSpace( aPtr, aWKTFence ) != IDE_SUCCESS );
    IDE_TEST_RAISE( **aPtr != ';', err_parsing );
    (*aPtr)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * =을 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipEqual( UChar** aPtr, UChar *aWKTFence )
{
    IDE_TEST( skipSpace( aPtr, aWKTFence ) != IDE_SUCCESS );
    IDE_TEST_RAISE( **aPtr != '=', err_parsing );
    (*aPtr)++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * 숫자 값을 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipNumber(UChar** aPtr, UChar *aWKTFence)
{
    UChar*      sPtr = *aPtr;
    SDouble     sD;

    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
    
    // prepare strtod (find NULL termination)
    while(1)
    {
        IDE_TEST_RAISE(sPtr == aWKTFence, err_parsing)  // end of WKT
        
        // To fix BUG-15413
        if( (*sPtr == '\0') || (isSpace(*sPtr) == ID_TRUE) || 
            (isSymbol(*sPtr) == ID_TRUE) )
        {
            break;
        }
        else
        {
            (sPtr)++;
        }
    }
    
    sPtr = NULL;
    sD = idlOS::strtod((SChar*)*aPtr, (SChar**)&sPtr);

    // underfloaw시 에러처리부분은 제거함. 그냥 수행
    IDE_TEST_RAISE((*aPtr == sPtr) || // no conversion is performed
             ((sD == HUGE_VAL) && (errno == ERANGE)), err_parsing); // range err

    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * 숫자 값을 읽어서 aD에 저장한다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * SDouble* aD(Out): DateTime
 **********************************************************************/
IDE_RC stdParsing::getNumber(UChar** aPtr, UChar *aWKTFence, SDouble* aD)
{
    UChar*      sPtr = *aPtr;

    IDE_TEST_RAISE(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS, err_parsing);
    
    // prepare strtod (find NULL termination)
    while(1)
    {
        // end of WKT
        IDE_TEST_RAISE( sPtr == aWKTFence, err_parsing );
        
        if ( ( *sPtr == '\0' ) || ( isSpace( *sPtr ) == ID_TRUE ) || 
             ( isSymbol( *sPtr ) == ID_TRUE ) )
        {
            break;
        }
        else
        {
            (sPtr)++;
        }
    }
    
    sPtr = NULL;    
    *aD = idlOS::strtod( (SChar*)*aPtr, (SChar**)&sPtr );

    // underfloaw시 에러처리부분은 제거함. 그냥 수행
    IDE_TEST_RAISE( ( *aPtr == sPtr ) || // no conversion is performed
                    ( ( *aD == HUGE_VAL ) && ( errno == ERANGE ) ), err_parsing );
    // range err
    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * 숫자 값을 건너 뛴다
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 **********************************************************************/
IDE_RC stdParsing::skipLong( UChar** aPtr, UChar *aWKTFence )
{
    UChar*      sPtr = *aPtr;

    IDE_TEST( skipSpace( aPtr, aWKTFence ) != IDE_SUCCESS );
    
    // prepare strtol (find NULL termination)
    while ( 1 )
    {
        IDE_TEST_RAISE( sPtr == aWKTFence, err_parsing )  // end of WKT
        
        // To fix BUG-15413
        if ( ( *sPtr == '\0' ) || ( isSpace( *sPtr ) == ID_TRUE ) || 
             ( isSymbol( *sPtr ) == ID_TRUE ) )
        {
            break;
        }
        else
        {
            (sPtr)++;
        }
    }
    
    sPtr = NULL;
    (void)idlOS::strtol( (SChar*)*aPtr, (SChar**)&sPtr, 10 );

    // underfloaw시 에러처리부분은 제거함. 그냥 수행
    IDE_TEST_RAISE( ( *aPtr == sPtr ) || // no conversion is performed
                    ( errno == ERANGE ), err_parsing ); // range err

    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * 숫자 값을 읽어서 aL에 저장한다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * SDouble* aD(Out): DateTime
 **********************************************************************/
IDE_RC stdParsing::getLong( UChar** aPtr, UChar *aWKTFence, SLong* aL )
{
    UChar*      sPtr = *aPtr;

    IDE_TEST_RAISE( skipSpace( aPtr, aWKTFence ) != IDE_SUCCESS, err_parsing );
    
    // prepare strtod (find NULL termination)
    while ( 1 )
    {
        if ( sPtr == aWKTFence )  // end of WKT
        {
            return IDE_FAILURE;
        }
        
        if ( (*sPtr == '\0') || (isSpace(*sPtr) == ID_TRUE) || 
            (isSymbol(*sPtr) == ID_TRUE) )
        {
            break;
        }
        else
        {
            (sPtr)++;
        }
    }
    
    sPtr = NULL;    
    *aL = idlOS::strtol( (SChar*)*aPtr, (SChar**)&sPtr, 10 );

    // underfloaw시 에러처리부분은 제거함. 그냥 수행
    IDE_TEST_RAISE( ( *aPtr == sPtr ) || // no conversion is performed
                    ( errno == ERANGE ), err_parsing );
    // range err
    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKT(Extended Well Known Text)를 읽어들여 SRID를 가져온다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::skipSRID( UChar**    aPtr,
                             UChar*     aWKTFence )
{
    UChar*       sPtr = *aPtr;
    STT_TOKEN    sTokenType;

    // =4326;
    sTokenType = testNextToken( &sPtr, aWKTFence );
    IDE_TEST_RAISE( sTokenType != STT_EQUAL_TOKEN, err_parsing );
    IDE_TEST( skipEqual( &sPtr, aWKTFence ) != IDE_SUCCESS );
    
    IDE_TEST( skipLong( &sPtr, aWKTFence ) != IDE_SUCCESS );
    
    sTokenType = testNextToken( &sPtr, aWKTFence );
    IDE_TEST_RAISE( sTokenType != STT_SEMICOLON_TOKEN, err_parsing );
    IDE_TEST( skipSemicolon( &sPtr, aWKTFence ) != IDE_SUCCESS );

    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKT(Extended Well Known Text)를 읽어들여 SRID를 가져온다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getSRID( UChar**    aPtr,
                            UChar*     aWKTFence,
                            SInt*      aSRID )
{
    UChar*   sPtr = *aPtr;
    SLong    sSRID;

    // 확인
    IDE_TEST( skipSRID( &sPtr, aWKTFence ) != IDE_SUCCESS );

    sPtr = *aPtr;

    // 변환
    IDE_TEST( skipEqual( &sPtr, aWKTFence ) != IDE_SUCCESS );
    IDE_TEST( getLong( &sPtr, aWKTFence, &sSRID ) != IDE_SUCCESS );
    IDE_TEST( skipSemicolon( &sPtr, aWKTFence ) != IDE_SUCCESS );

    IDE_TEST_RAISE( ( sSRID < ST_SRID_MIN ) || ( sSRID > ST_SRID_MAX ),
                    err_overflow );
    
    *aSRID = (SInt)sSRID;
    
    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_overflow );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_VALUE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdPoint2DType이나 stdPoint3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getPoint( UChar**                    aPtr,
                             UChar*                     aWKTFence,
                             stdGeometryHeader*         aGeom,
                             void*                      aFence,
                             IDE_RC*                    aResult,
                             UInt                       aValidateOption,
                             SInt                       aSRID )
{
    // 1.
    UChar*          sPtr = *aPtr;
    UInt            sTotalSize = 0;
    stdGeoTypes     sCurrentType = STD_NULL_TYPE;
    STT_TOKEN       sTokenType;
    // 2
    stdPoint2DType*    sGeom2D = NULL;
    stdPoint2DExtType* sGeom2DExt = NULL;
    idBool             sIsValid = ID_TRUE;

    /* BUG-32531 Consider for GIS EMPTY */
 	IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
 	
 	if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
 	{
        IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
 	}
 	else
 	{ 
    
        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);
        IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);

        sTokenType = testNextToken(&sPtr, aWKTFence);
        if(sTokenType == STT_RPAREN_TOKEN)
        {
            if ( aSRID == ST_SRID_INIT )
            {
                sCurrentType = STD_POINT_2D_TYPE;
                sTotalSize = STD_POINT2D_SIZE;
            }
            else
            {
                // srid가 추가된 확장타입
                sCurrentType = STD_POINT_2D_EXT_TYPE;
                sTotalSize = STD_POINT2D_EXT_SIZE;
            }
        }
        else 
        {
            IDE_RAISE(err_parsing);
        }
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);

        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);

        // 2. Storing Data =========================================================
        idlOS::memset(aGeom, 0x00, sTotalSize);
        // Fix BUG-15290
        stdUtils::nullify(aGeom);
    
        aGeom->mType = sCurrentType;
        aGeom->mSize = sTotalSize;    
        sPtr = *aPtr;
        if ( ( sCurrentType == STD_POINT_2D_TYPE ) ||
             ( sCurrentType == STD_POINT_2D_EXT_TYPE ) ) 
        {
            sGeom2D = (stdPoint2DType*)aGeom;

            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            IDE_TEST(getNumber(&sPtr, aWKTFence, &sGeom2D->mPoint.mX)!=IDE_SUCCESS);
            IDE_TEST(getNumber(&sPtr, aWKTFence, &sGeom2D->mPoint.mY)!=IDE_SUCCESS);
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, &sGeom2D->mPoint) != IDE_SUCCESS );

            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validatePoint2D(sGeom2D, sTotalSize) 
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }

            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }

        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_POINT_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdPoint2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS; 
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdLineString2DType이나 stdLineString3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getLineString( UChar**                    aPtr,
                                  UChar*                     aWKTFence,
                                  stdGeometryHeader*         aGeom,
                                  void*                      aFence,
                                  IDE_RC*                    aResult,
                                  UInt                       aValidateOption,
                                  SInt                       aSRID )
{
    // 1.
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sTotalSize = 0;
    stdGeoTypes             sCurrentType = STD_NULL_TYPE;
    STT_TOKEN               sTokenType = STT_SPACE_TOKEN;
    // 2.
    idBool                  sIsValid = ID_TRUE;
    stdLineString2DType*    sGeom2D = NULL;
    stdLineString2DExtType* sGeom2DExt = NULL;
    stdPoint2D*             sPt2D = NULL;
    UInt                    i;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
			
    if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
    {
        IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {
        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        while((sPtr < aWKTFence) && (*sPtr != '\0') && 
              (sTokenType != STT_RPAREN_TOKEN))
        {
            IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);
            IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);

            sTokenType = testNextToken(&sPtr, aWKTFence);
            if(sCurrentType == STD_NULL_TYPE)
            {
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    if ( aSRID == ST_SRID_INIT )
                    {
                        sCurrentType = STD_LINESTRING_2D_TYPE;
                    }
                    else
                    {
                        // srid가 추가된 확장타입
                        sCurrentType = STD_LINESTRING_2D_EXT_TYPE;
                    }
                    
                    sTotalSize += STD_PT2D_SIZE;
                }
                else if(sTokenType == STT_NUM_TOKEN)
                {
                    IDE_RAISE(err_parsing);
                }
                else
                {
                    break;    // Means that Geom has One Point!
                }
            }
            else
            {
                if ( ( sCurrentType == STD_LINESTRING_2D_TYPE ) ||
                     ( sCurrentType == STD_LINESTRING_2D_EXT_TYPE ) )
                {            
                    sTotalSize += STD_PT2D_SIZE;
                }
                else
                {
                    //??
                    IDE_RAISE(err_invalid_object_mType);
                }
            }

            IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
            sPtCnt++;                
        
            IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                           (sTokenType != STT_RPAREN_TOKEN), err_parsing);
            if(sTokenType == STT_COMMA_TOKEN) 
            {
                IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
            }
        }

    
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        if(sCurrentType == STD_LINESTRING_2D_TYPE) 
        {
            sTotalSize += STD_LINE2D_SIZE;
        }
        else if ( sCurrentType == STD_LINESTRING_2D_EXT_TYPE )
        {
            sTotalSize += STD_LINE2D_EXT_SIZE;
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
    
        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
    
        // 2. Storing Data =========================================================
        idlOS::memset(aGeom, 0x00, sTotalSize);
        // Fix BUG-15290
        stdUtils::nullify(aGeom);

        aGeom->mType = sCurrentType;
        aGeom->mSize = sTotalSize;    
        sPtr = *aPtr;
        if ( ( sCurrentType == STD_LINESTRING_2D_TYPE ) ||
             ( sCurrentType == STD_LINESTRING_2D_EXT_TYPE ) )
        {
            sGeom2D = (stdLineString2DType*)aGeom;
            sGeom2D->mNumPoints = sPtCnt;
        
            sPt2D = STD_FIRST_PT2D(sGeom2D);    // Move First Point
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            sTokenType = STT_SPACE_TOKEN;
            for(i = 0; (i < sPtCnt) && (sTokenType != STT_RPAREN_TOKEN) ; i++)
            {
                IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mX) != IDE_SUCCESS);
                IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mY) != IDE_SUCCESS);
                IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D) != IDE_SUCCESS );
        
                sTokenType = testNextToken(&sPtr, aWKTFence);
                IDE_TEST((sTokenType != STT_COMMA_TOKEN) && 
                         (sTokenType != STT_RPAREN_TOKEN));
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
                // Move Next Point
                sPt2D = STD_NEXT_PT2D(sPt2D);
            }    
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

            // BUG-27002
            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validateLineString2D(sGeom2D, sTotalSize)
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }
        
            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
        
        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_LINESTRING_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdLineString2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;  
    }    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdPolygon2DType이나 stdPolygon3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getPolygon( iduMemory*                 aQmxMem,
                               UChar**                    aPtr,
                               UChar*                     aWKTFence,
                               stdGeometryHeader*         aGeom,
                               void*                      aFence,
                               IDE_RC*                    aResult,
                               UInt                       aValidateOption,
                               SInt                       aSRID )
{
    // 1.
    UChar*              sPtr = *aPtr;
    UInt                sPtCnt = 0;
    UInt                sRingCnt = 0;
    UInt                sTotalSize = 0;
    stdGeoTypes         sCurrentType = STD_NULL_TYPE;
    STT_TOKEN           sTokenType = STT_SPACE_TOKEN;
    // 2.
    idBool               sIsValid = ID_TRUE;
    stdPolygon2DType*    sGeom2D = NULL;
    stdPolygon2DExtType* sGeom2DExt = NULL;
    stdLinearRing2D*     sRing2D = NULL;
    stdPoint2D*          sPt2D = NULL;
    UInt                 i;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
		
    if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
    {
		IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
		IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {
        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        while((sPtr < aWKTFence) && (*sPtr != '\0') && 
              (sTokenType != STT_RPAREN_TOKEN))
        {
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
            sTokenType = testNextToken(&sPtr, aWKTFence);
            while (sTokenType != STT_RPAREN_TOKEN)
            {
                IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);
                IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);

                sTokenType = testNextToken(&sPtr, aWKTFence);
                if(sCurrentType == STD_NULL_TYPE)
                {
                    if(sTokenType == STT_COMMA_TOKEN)
                    {
                        if ( aSRID == ST_SRID_INIT )
                        {
                            sCurrentType = STD_POLYGON_2D_TYPE;
                        }
                        else
                        {
                            // srid가 추가된 확장타입
                            sCurrentType = STD_POLYGON_2D_EXT_TYPE;
                        }
                        
                        sTotalSize += STD_PT2D_SIZE;
                    }
                    else if(sTokenType == STT_NUM_TOKEN)
                    {
                        IDE_RAISE(err_parsing);
                    }
                    else
                    {
                        break;    // Means that Geom has One Point!
                    }
                }
                else
                {
                    if ( ( sCurrentType == STD_POLYGON_2D_TYPE ) ||
                         ( sCurrentType == STD_POLYGON_2D_EXT_TYPE ) )
                    {            
                        sTotalSize += STD_PT2D_SIZE;
                    }
                    else
                    {
                        IDE_RAISE(err_invalid_object_mType);
                    }
                }
                IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
                sPtCnt++;

                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
            } 
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
            IDE_TEST_RAISE(sRingCnt == ID_UINT_MAX, err_parsing);
            sRingCnt++;

            if ( ( sCurrentType == STD_POLYGON_2D_TYPE ) ||
                 ( sCurrentType == STD_POLYGON_2D_EXT_TYPE ) )
            {
                sTotalSize += STD_RN2D_SIZE;
            }
            else
            {
                IDE_RAISE(err_parsing);
            }

            sTokenType = testNextToken(&sPtr, aWKTFence);
            IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                           (sTokenType != STT_RPAREN_TOKEN), err_parsing);

            if (sTokenType == STT_COMMA_TOKEN)
            {
                IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
            }
        }
    
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        if(sCurrentType == STD_POLYGON_2D_TYPE) 
        {
            sTotalSize += STD_POLY2D_SIZE;
        }
        else if ( sCurrentType == STD_POLYGON_2D_EXT_TYPE )
        {
            sTotalSize += STD_POLY2D_EXT_SIZE;
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
    
        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
 
        // 2. Storing Data =========================================================
        idlOS::memset(aGeom, 0x00, sTotalSize);
        // Fix BUG-15290
        stdUtils::nullify(aGeom);

        aGeom->mType = sCurrentType;
        sPtr = *aPtr;
        if ( ( sCurrentType == STD_POLYGON_2D_TYPE ) ||
             ( sCurrentType == STD_POLYGON_2D_EXT_TYPE ) )
        {
            sGeom2D = (stdPolygon2DType*)aGeom;
            sGeom2D->mNumRings = sRingCnt;
        
            sRing2D = STD_FIRST_RN2D(sGeom2D);    // Move First Ring
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            sTokenType = STT_SPACE_TOKEN;
            for(i = 0; (i < sRingCnt) && (sTokenType != STT_RPAREN_TOKEN) ; i++)
            {
                sPt2D = STD_FIRST_PT2D(sRing2D);    // Move First Point,
                sPtCnt = 0;
        
                IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
                while (sTokenType != STT_RPAREN_TOKEN)
                {
                    IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
                    sPtCnt++;

                    IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mX) != 
                             IDE_SUCCESS);
                    IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mY) != 
                             IDE_SUCCESS);
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D) != IDE_SUCCESS );
        
                    sTokenType = testNextToken(&sPtr, aWKTFence);
                    IDE_TEST((sTokenType != STT_COMMA_TOKEN) && 
                             (sTokenType != STT_RPAREN_TOKEN));
                    if(sTokenType == STT_COMMA_TOKEN)
                    {
                        IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                    }
                    // Move Next Point
                    sPt2D = STD_NEXT_PT2D(sPt2D);
                } 
                IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

                if( stdUtils::isSamePoints2D( STD_FIRST_PT2D(sRing2D),
                                              STD_PREV_PT2D(sPt2D)) == ID_FALSE )
                {
                    sTotalSize += STD_PT2D_SIZE;
                    IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence,
                                   retry_memory);
                    
                    // To fix BUG-33472 CodeSonar warining

                    if ( sPt2D != STD_FIRST_PT2D(sRing2D))
                    {                        
                        idlOS::memcpy( sPt2D, STD_FIRST_PT2D(sRing2D), STD_PT2D_SIZE);
                    }
                    else
                    {
                        // nothing to do   
                    }

                    sPt2D = STD_NEXT_PT2D(sPt2D);
                    sPtCnt++;
                }

                sRing2D->mNumPoints = sPtCnt;

                sTokenType = testNextToken(&sPtr, aWKTFence);
                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
                // Move Next Ring
                sRing2D = (stdLinearRing2D*)sPt2D;
            }
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

            // BUG-27002
            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validatePolygon2D(aQmxMem, sGeom2D, sTotalSize) 
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }
        
            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }

        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_POLYGON_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdPolygon2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
        
        aGeom->mSize = sTotalSize;
    }
        
    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;  
    }
    
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  WKT(Well Known Text)를 읽어들여 stdPolygon2DType이나 stdPolygon3DType 객체로 저장한다.
 *  3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 허용하지 않는다.
 *
 *  ST_RECTANGLE은 두 점(좌하단, 우상단)으로 이루어지는 직사각형을 표현한다.
 *  RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
 *
 *  UChar             ** aPtr(InOut)   : 읽어들일 문자열 위치, 읽은 다음 그만큼 증가한다.
 *  UChar              * aWKTFence(In) : 문자열 버퍼의 Fence
 *  stdGeometryHeader  * aGeom(Out)    : 저장될 버퍼(Geometry 객체)
 *  void               * aFence(In)    : 저장될 버퍼(Geometry 객체)의 Fence
 *  IDE_RC             * aResult(Out)  : Error Code
 **********************************************************************/
IDE_RC stdParsing::getRectangle( iduMemory          * aQmxMem,
                                 UChar             ** aPtr,
                                 UChar              * aWKTFence,
                                 stdGeometryHeader  * aGeom,
                                 void               * aFence,
                                 IDE_RC             * aResult,
                                 UInt                 aValidateOption )
{
    UChar            * sPtr       = *aPtr;

    idBool             sIsValid   = ID_TRUE;
    stdPolygon2DType * sGeom2D    = NULL;
    stdLinearRing2D  * sRing2D    = NULL;
    stdPoint2D       * sPt2D      = NULL;

    // RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
    UInt               sTotalSize = ( STD_PT2D_SIZE * 5 ) + STD_RN2D_SIZE + STD_POLY2D_SIZE;
    SDouble            sX1        = (SDouble)0.0;
    SDouble            sY1        = (SDouble)0.0;
    SDouble            sX2        = (SDouble)0.0;
    SDouble            sY2        = (SDouble)0.0;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST( skipSpace( aPtr, aWKTFence ) != IDE_SUCCESS );

    if ( idlOS::strncasecmp( (SChar *)*aPtr,
                             STD_EMPTY_NAME,
                             STD_EMPTY_NAME_LEN ) == 0 )
    {
        IDE_TEST( checkValidEmpty( &sPtr, aWKTFence ) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( ( sPtr >= aWKTFence ) || ( *sPtr == '\0' ), ERR_PARSING );

        // 1. Define whether 2D or 3D Type; Test validation;
        IDE_TEST( skipLParen( &sPtr, aWKTFence ) != IDE_SUCCESS );

        IDE_TEST( getNumber( &sPtr, aWKTFence, &sX1 ) != IDE_SUCCESS );
        IDE_TEST( getNumber( &sPtr, aWKTFence, &sY1 ) != IDE_SUCCESS );

        IDE_TEST( skipComma( &sPtr, aWKTFence ) != IDE_SUCCESS );

        IDE_TEST( getNumber( &sPtr, aWKTFence, &sX2 ) != IDE_SUCCESS );
        IDE_TEST( getNumber( &sPtr, aWKTFence, &sY2 ) != IDE_SUCCESS );

        IDE_TEST( skipRParen( &sPtr, aWKTFence ) != IDE_SUCCESS );

        // Fix BUG-15980
        IDE_TEST( skipToLast( &sPtr, aWKTFence ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ((UChar *)aGeom) + sTotalSize > (UChar *)aFence, RETRY_MEMORY );

        // 2. Storing Data =========================================================
        idlOS::memset( aGeom, 0x00, sTotalSize );

        // Fix BUG-15290
        stdUtils::nullify( aGeom );

        aGeom->mType        = STD_POLYGON_2D_TYPE;
        aGeom->mSize        = sTotalSize;
        sGeom2D             = (stdPolygon2DType *)aGeom;
        sGeom2D->mNumRings  = 1;
        sRing2D             = STD_FIRST_RN2D( sGeom2D );    // Move First Ring
        sRing2D->mNumPoints = 5;
        sPt2D               = STD_FIRST_PT2D( sRing2D );    // Move First Point

        // RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
        sPt2D->mX = sX1;
        sPt2D->mY = sY1;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

        // Move Next Point
        sPt2D = STD_NEXT_PT2D( sPt2D );

        sPt2D->mX = sX2;
        sPt2D->mY = sY1;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

        // Move Next Point
        sPt2D = STD_NEXT_PT2D( sPt2D );

        sPt2D->mX = sX2;
        sPt2D->mY = sY2;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

        // Move Next Point
        sPt2D = STD_NEXT_PT2D( sPt2D );

        sPt2D->mX = sX1;
        sPt2D->mY = sY2;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

        // Move Next Point
        sPt2D = STD_NEXT_PT2D( sPt2D );

        sPt2D->mX = sX1;
        sPt2D->mY = sY1;
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

        // BUG-27002
        if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
        {
            if ( stdPrimitive::validatePolygon2D( aQmxMem, sGeom2D, sTotalSize ) == IDE_SUCCESS )
            {
                sIsValid = ID_TRUE;
            }
            else
            {
                sIsValid = ID_FALSE;
            }
        }
        else
        {
            sIsValid = ID_FALSE;
        }

        setValidHeader( aGeom, sIsValid, aValidateOption );
    }

    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PARSING );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    IDE_EXCEPTION( RETRY_MEMORY );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdMultiPoint2DType이나 stdMultiPoint3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getMultiPoint( UChar**                    aPtr,
                                  UChar*                     aWKTFence,
                                  stdGeometryHeader*         aGeom,
                                  void*                      aFence,
                                  IDE_RC*                    aResult,
                                  UInt                       aValidateOption,
                                  SInt                       aSRID )
{
    // 1.
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sTotalSize = 0;
    stdGeoTypes             sCurrentType = STD_NULL_TYPE;
    STT_TOKEN               sTokenType = STT_SPACE_TOKEN;
    // 2.
    idBool                  sIsValid = ID_TRUE;
    stdMultiPoint2DType*    sGeom2D = NULL;
    stdMultiPoint2DExtType* sGeom2DExt = NULL;
    stdPoint2DType*         sPt2D = NULL;
    UInt                    i;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
			
    if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
    {
        IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {
        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        while((sPtr < aWKTFence) && (*sPtr != '\0') && 
              (sTokenType != STT_RPAREN_TOKEN))
        {
            IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);
            IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);

            sTokenType = testNextToken(&sPtr, aWKTFence);
            if(sCurrentType == STD_NULL_TYPE)
            {
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    if ( aSRID == ST_SRID_INIT )
                    {
                        sCurrentType = STD_MULTIPOINT_2D_TYPE;
                    }
                    else
                    {
                        // srid가 추가된 확장타입
                        sCurrentType = STD_MULTIPOINT_2D_EXT_TYPE;
                    }
                    
                    sTotalSize += STD_POINT2D_SIZE;
                }
                else if(sTokenType == STT_NUM_TOKEN)
                {
                    // bugbug praring error
                }
                // To Fix BUG-15954 : MultiPoint에서 2D Point가 하나일 때 처리
                else if( sTokenType == STT_RPAREN_TOKEN )
                {
                    if ( aSRID == ST_SRID_INIT )
                    {
                        sCurrentType = STD_MULTIPOINT_2D_TYPE;
                    }
                    else
                    {
                        sCurrentType = STD_MULTIPOINT_2D_EXT_TYPE;
                    }
                    
                    sTotalSize += STD_POINT2D_SIZE;
                    sPtCnt++;
                    break;    // Means that Geom has One Point!
                }
                else {
                    break;
                }
            }
            else
            {
                if ( ( sCurrentType == STD_MULTIPOINT_2D_TYPE ) ||
                     ( sCurrentType == STD_MULTIPOINT_2D_EXT_TYPE ) )
                {            
                    sTotalSize += STD_POINT2D_SIZE;
                }
                else
                {
                    IDE_TEST(ID_TRUE);                
                }
            }

            IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
            sPtCnt++;                
        
            IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                           (sTokenType != STT_RPAREN_TOKEN), err_parsing);

            if (sTokenType == STT_COMMA_TOKEN) 
            {
                IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
            }
        }
    
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        if ( ( sCurrentType == STD_MULTIPOINT_2D_TYPE ) ||
             ( sCurrentType == STD_MULTIPOINT_2D_EXT_TYPE ) )
        {
            sTotalSize += STD_MPOINT2D_SIZE;
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
    
        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
    
        // 2. Storing Data =========================================================
        idlOS::memset(aGeom, 0x00, sTotalSize);
        // Fix BUG-15290
        stdUtils::nullify(aGeom);

        aGeom->mType = sCurrentType;
        aGeom->mSize = sTotalSize;    
        sPtr = *aPtr;
        if ( ( sCurrentType == STD_MULTIPOINT_2D_TYPE ) ||
             ( sCurrentType == STD_MULTIPOINT_2D_EXT_TYPE ) )
        {
            sGeom2D = (stdMultiPoint2DType*)aGeom;
            sGeom2D->mNumObjects = sPtCnt;
        
            sPt2D = STD_FIRST_POINT2D(sGeom2D);    // Move First Point
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            sTokenType = STT_SPACE_TOKEN;
            for(i = 0; (i < sPtCnt) && (sTokenType != STT_RPAREN_TOKEN) ; i++)
            {
                IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mPoint.mX) != 
                         IDE_SUCCESS);
                IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mPoint.mY) != 
                         IDE_SUCCESS);
                stdUtils::nullify((stdGeometryHeader*) sPt2D); // BUG-16552
                sPt2D->mType = STD_POINT_2D_TYPE;
                sPt2D->mSize = STD_POINT2D_SIZE;
                IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sPt2D, 
                                                        &sPt2D->mPoint)
                          != IDE_SUCCESS );
                IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, &sPt2D->mPoint)
                          != IDE_SUCCESS );
        
                sTokenType = testNextToken(&sPtr, aWKTFence);
                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
                // Move Next Point
                sPt2D = STD_NEXT_POINT2D(sPt2D);
            }    
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validateMultiPoint2D(sGeom2D, sTotalSize)
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }

            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }

        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_MULTIPOINT_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdMultiPoint2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    } 
    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;  
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdMultiLineString2DType이나 stdMultiLineString3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getMultiLineString( UChar**                    aPtr,
                                       UChar*                     aWKTFence,
                                       stdGeometryHeader*         aGeom,
                                       void*                      aFence,
                                       IDE_RC*                    aResult,
                                       UInt                       aValidateOption,
                                       SInt                       aSRID )
{
    // 1.
    UChar*                      sPtr = *aPtr;
    UInt                        sPtCnt = 0;
    UInt                        sLineCnt = 0;
    UInt                        sTotalSize = 0;
    stdGeoTypes                 sCurrentType = STD_NULL_TYPE;
    STT_TOKEN                   sTokenType = STT_SPACE_TOKEN;
    // 2.
    idBool                       sIsValid = ID_TRUE;
    stdMultiLineString2DType*    sGeom2D = NULL;
    stdMultiLineString2DExtType* sGeom2DExt = NULL;
    stdLineString2DType*         sLine2D = NULL;
    stdPoint2D*                  sPt2D = NULL;
    UInt                         i;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
			
    if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
    {
        IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {
        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        while((sPtr < aWKTFence) && (*sPtr != '\0') && 
              (sTokenType != STT_RPAREN_TOKEN))
        {
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
            sTokenType = testNextToken(&sPtr, aWKTFence);
            while (sTokenType != STT_RPAREN_TOKEN)
            {
                IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);
                IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);

                sTokenType = testNextToken(&sPtr, aWKTFence);
                if(sCurrentType == STD_NULL_TYPE)
                {
                    if(sTokenType == STT_COMMA_TOKEN)
                    {
                        if ( aSRID == ST_SRID_INIT )
                        {
                            sCurrentType = STD_MULTILINESTRING_2D_TYPE;
                        }
                        else
                        {
                            // srid가 추가된 확장타입
                            sCurrentType = STD_MULTILINESTRING_2D_EXT_TYPE;
                        }
                        
                        sTotalSize += STD_PT2D_SIZE;
                    }
                    else if(sTokenType == STT_NUM_TOKEN)
                    {
                        IDE_RAISE(err_parsing);
                    }
                    else
                    {
                        break;    // Means that Geom has One Point!
                    }
                }
                else
                {
                    if ( ( sCurrentType == STD_MULTILINESTRING_2D_TYPE ) ||
                         ( sCurrentType == STD_MULTILINESTRING_2D_EXT_TYPE ) )
                    {            
                        sTotalSize += STD_PT2D_SIZE;
                    }
                    else
                    {
                        IDE_RAISE(err_parsing);                
                    }
                }
                IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
                sPtCnt++;

                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
            } 
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
            IDE_TEST_RAISE(sLineCnt == ID_UINT_MAX, err_parsing);
            sLineCnt++;

            if ( ( sCurrentType == STD_MULTILINESTRING_2D_TYPE ) ||
                 ( sCurrentType == STD_MULTILINESTRING_2D_EXT_TYPE ) )
            {
                sTotalSize += STD_LINE2D_SIZE;
            }
            else
            {
                IDE_RAISE(err_parsing);
            }

            sTokenType = testNextToken(&sPtr, aWKTFence);
            IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                           (sTokenType != STT_RPAREN_TOKEN), err_parsing);
        
            if(sTokenType == STT_COMMA_TOKEN)
            {
                IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
            }
        }
            
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
    
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        if(sCurrentType == STD_MULTILINESTRING_2D_TYPE) 
        {
            sTotalSize += STD_MLINE2D_SIZE;
        }
        else if ( sCurrentType == STD_MULTILINESTRING_2D_EXT_TYPE )
        {
            sTotalSize += STD_MLINE2D_EXT_SIZE;
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
    
        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
 
        // 2. Storing Data =========================================================
        idlOS::memset(aGeom, 0x00, sTotalSize);
        // Fix BUG-15290
        stdUtils::nullify(aGeom);

        aGeom->mType = sCurrentType;
        aGeom->mSize = sTotalSize;    
        sPtr = *aPtr;
        if ( ( sCurrentType == STD_MULTILINESTRING_2D_TYPE ) ||
             ( sCurrentType == STD_MULTILINESTRING_2D_EXT_TYPE ) )
        {
            sGeom2D = (stdMultiLineString2DType*)aGeom;
            sGeom2D->mNumObjects = sLineCnt;
        
            sLine2D = STD_FIRST_LINE2D(sGeom2D);    // Move First Line
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            sTokenType = STT_SPACE_TOKEN;
            for(i = 0; (i < sLineCnt) && (sTokenType != STT_RPAREN_TOKEN) ; i++)
            {
                stdUtils::nullify((stdGeometryHeader*) sLine2D);  // BUG-16552
                sLine2D->mType = STD_LINESTRING_2D_TYPE;
                sLine2D->mSize = STD_LINE2D_SIZE;
            
                sPt2D = STD_FIRST_PT2D(sLine2D);    // Move First Point,
                sPtCnt = 0;        
                IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);        
                while (sTokenType != STT_RPAREN_TOKEN)
                {
                    sLine2D->mSize += STD_PT2D_SIZE;
                    IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
                    sPtCnt++;
                
                    IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mX) != 
                             IDE_SUCCESS);
                    IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mY) != 
                             IDE_SUCCESS);
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( (stdGeometryHeader*)sLine2D, 
                                                             sPt2D )
                              != IDE_SUCCESS );
                    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D)
                              != IDE_SUCCESS );

                    sTokenType = testNextToken(&sPtr, aWKTFence);
                    IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                                   (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                    if(sTokenType == STT_COMMA_TOKEN)
                    {
                        IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                    }
                    // Move Next Point
                    sPt2D = STD_NEXT_PT2D(sPt2D);
                } 
                IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

                sLine2D->mNumPoints = sPtCnt;

                sTokenType = testNextToken(&sPtr, aWKTFence);
                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
                // Move Next Line
                sLine2D = (stdLineString2DType*)sPt2D;
            }
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

            // BUG-27002
            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validateMultiLineString2D(sGeom2D, sTotalSize)
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }

            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }

        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_MULTILINESTRING_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdMultiLineString2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    *aPtr = sPtr;
    
    *aResult = IDE_SUCCESS;
    return IDE_SUCCESS;
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;  
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdMultiPolygon2DType이나 stdMultiPolygon3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getMultiPolygon( iduMemory*                 aQmxMem,
                                    UChar**                    aPtr,
                                    UChar*                     aWKTFence,
                                    stdGeometryHeader*         aGeom,
                                    void*                      aFence,
                                    IDE_RC*                    aResult,
                                    UInt                       aValidateOption,
                                    SInt                       aSRID )
{
    // 1.
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sRingCnt = 0;
    UInt                    sPolyCnt = 0;
    UInt                    sTotalSize = 0;
    stdGeoTypes             sCurrentType = STD_NULL_TYPE;
    STT_TOKEN               sTokenType = STT_SPACE_TOKEN;
    // 2.
    idBool                    sIsValid = ID_TRUE;
    stdMultiPolygon2DType*    sGeom2D = NULL;
    stdMultiPolygon2DExtType* sGeom2DExt = NULL;
    stdPolygon2DType*         sPoly2D = NULL;
    stdLinearRing2D*          sRing2D = NULL;
    stdPoint2D*               sPt2D = NULL;
    UInt                      i;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
			
    if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
    {
		IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {    
        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        while((sPtr < aWKTFence) && (*sPtr != '\0') && 
              (sTokenType != STT_RPAREN_TOKEN))
        {
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);

            sTokenType = testNextToken(&sPtr, aWKTFence);
            while (sTokenType != STT_RPAREN_TOKEN)
            {
                IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
                sTokenType = testNextToken(&sPtr, aWKTFence);
                while (sTokenType != STT_RPAREN_TOKEN)
                {
                    IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);
                    IDE_TEST(skipNumber(&sPtr, aWKTFence) != IDE_SUCCESS);

                    sTokenType = testNextToken(&sPtr, aWKTFence);
                    if(sCurrentType == STD_NULL_TYPE)
                    {
                        if(sTokenType == STT_COMMA_TOKEN)
                        {
                            if ( aSRID == ST_SRID_INIT )
                            {
                                sCurrentType = STD_MULTIPOLYGON_2D_TYPE;
                            }
                            else
                            {
                                // srid가 추가된 확장타입
                                sCurrentType = STD_MULTIPOLYGON_2D_EXT_TYPE;
                            }
                            
                            sTotalSize += STD_PT2D_SIZE;
                        }
                        else if(sTokenType == STT_NUM_TOKEN)
                        {
                            IDE_RAISE(err_parsing);
                        }
                        else
                        {
                            break;    // Means that Geom has One Point!
                        }
                    }
                    else
                    {
                        if ( ( sCurrentType == STD_MULTIPOLYGON_2D_TYPE ) ||
                             ( sCurrentType == STD_MULTIPOLYGON_2D_EXT_TYPE ) )
                        {            
                            sTotalSize += STD_PT2D_SIZE;
                        }
                        else
                        {
                            IDE_RAISE(err_parsing);                
                        }
                    }
                    IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
                    sPtCnt++;

                    IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                                   (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                
                    if(sTokenType == STT_COMMA_TOKEN)
                    {
                        IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                    }
                } 
                IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
                IDE_TEST_RAISE(sRingCnt == ID_UINT_MAX, err_parsing);
                sRingCnt++;

                if ( ( sCurrentType == STD_MULTIPOLYGON_2D_TYPE ) ||
                     ( sCurrentType == STD_MULTIPOLYGON_2D_EXT_TYPE ) )
                {
                    sTotalSize += STD_RN2D_SIZE;
                }
                else
                {
                    IDE_RAISE(err_parsing);
                }

                sTokenType = testNextToken(&sPtr, aWKTFence);
                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);

                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
            }
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        
            IDE_TEST_RAISE(sPolyCnt == ID_UINT_MAX, err_parsing);
            sPolyCnt++;

            if ( ( sCurrentType == STD_MULTIPOLYGON_2D_TYPE ) ||
                 ( sCurrentType == STD_MULTIPOLYGON_2D_EXT_TYPE ) )
            {
                sTotalSize += STD_POLY2D_SIZE;
            }
            else
            {
                IDE_RAISE(err_parsing);
            }

            sTokenType = testNextToken(&sPtr, aWKTFence);
            IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                           (sTokenType != STT_RPAREN_TOKEN), err_parsing);

            if(sTokenType == STT_COMMA_TOKEN)
            {
                IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
            }    
        }
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        if(sCurrentType == STD_MULTIPOLYGON_2D_TYPE) 
        {
            sTotalSize += STD_MPOLY2D_SIZE;
        }
        else if ( sCurrentType == STD_MULTIPOLYGON_2D_EXT_TYPE )
        {
            sTotalSize += STD_MPOLY2D_EXT_SIZE;
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
    
        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
 
        // 2. Storing Data =========================================================
        idlOS::memset(aGeom, 0x00, sTotalSize);
        // Fix BUG-15290
        stdUtils::nullify(aGeom);

        aGeom->mType = sCurrentType;
        sPtr = *aPtr;
        if ( ( sCurrentType == STD_MULTIPOLYGON_2D_TYPE ) ||
             ( sCurrentType == STD_MULTIPOLYGON_2D_EXT_TYPE ) )
        {
            sGeom2D = (stdMultiPolygon2DType*)aGeom;
            sGeom2D->mNumObjects = sPolyCnt;
           
            sPoly2D = STD_FIRST_POLY2D(sGeom2D);    // Move First Polygon
            IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            sTokenType = STT_SPACE_TOKEN;
            for(i = 0; (i < sPolyCnt) && (sTokenType != STT_RPAREN_TOKEN) ; i++)
            {
                stdUtils::nullify((stdGeometryHeader*) sPoly2D);  // BUG-16552
                sPoly2D->mType = STD_POLYGON_2D_TYPE;
                sPoly2D->mSize = STD_POLY2D_SIZE;
        
                sRing2D = STD_FIRST_RN2D(sPoly2D);    // Move First Ring
                sRingCnt = 0;
                IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
                sTokenType = STT_SPACE_TOKEN;
                while (sTokenType != STT_RPAREN_TOKEN)
                {
                    sPoly2D->mSize += STD_RN2D_SIZE;        
                    IDE_TEST_RAISE(sRingCnt == ID_UINT_MAX, err_parsing);
                    sRingCnt++;

                    sPt2D = STD_FIRST_PT2D(sRing2D);    // Move First Point,
                    sPtCnt = 0;
                    IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);        
                    while (sTokenType != STT_RPAREN_TOKEN)
                    {
                        // To fix BUG 15312
                        sPoly2D->mSize += STD_PT2D_SIZE;
                        IDE_TEST_RAISE(sPtCnt == ID_UINT_MAX, err_parsing);
                        sPtCnt++;

                        IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mX) != 
                                 IDE_SUCCESS);
                        IDE_TEST(getNumber(&sPtr, aWKTFence, &sPt2D->mY) != 
                                 IDE_SUCCESS);
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D((stdGeometryHeader*)sPoly2D, 
                                                                sPt2D )
                                  != IDE_SUCCESS );
                        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D)
                                  != IDE_SUCCESS );
        
                        sTokenType = testNextToken(&sPtr, aWKTFence);
                        IDE_TEST((sTokenType != STT_COMMA_TOKEN) && 
                                 (sTokenType != STT_RPAREN_TOKEN));
                        if(sTokenType == STT_COMMA_TOKEN)
                        {
                            IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                        }
                        // Move Next Point
                        sPt2D = STD_NEXT_PT2D(sPt2D);
                    } // while
                    IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

                    if( stdUtils::isSamePoints2D( STD_FIRST_PT2D(sRing2D),
                                                  STD_PREV_PT2D(sPt2D)) == ID_FALSE )
                    {
                        // To fix BUG 15312
                        sPoly2D->mSize += STD_PT2D_SIZE;
                        sTotalSize += STD_PT2D_SIZE;
                        IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, 
                                       retry_memory);

                        // To fix BUG-33472 CodeSonar warining

                        if ( sPt2D != STD_FIRST_PT2D(sRing2D))
                        {                        
                            idlOS::memcpy( sPt2D, STD_FIRST_PT2D(sRing2D), STD_PT2D_SIZE);
                        }
                        else
                        {
                            // nothing to do   
                        }                        
                    
                        sPt2D = STD_NEXT_PT2D(sPt2D);
                        sPtCnt++;
                    }

                    sRing2D->mNumPoints = sPtCnt;

                    sTokenType = testNextToken(&sPtr, aWKTFence);
                    IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                                   (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                    if(sTokenType == STT_COMMA_TOKEN)
                    {
                        IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                    }
                    // Move Next Ring
                    sRing2D = (stdLinearRing2D*)sPt2D;
                } // while
                IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
            
                sPoly2D->mNumRings = sRingCnt;

                sTokenType = testNextToken(&sPtr, aWKTFence);
                IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                               (sTokenType != STT_RPAREN_TOKEN), err_parsing);
                if(sTokenType == STT_COMMA_TOKEN)
                {
                    IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
                }
                // Move Next Polygon
                sPoly2D = (stdPolygon2DType*)sRing2D;
            } // for    
            IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);

            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validateMultiPolygon2D(aQmxMem, sGeom2D, sTotalSize)
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }

            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }

        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_MULTIPOLYGON_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdMultiPolygon2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
        
        aGeom->mSize = sTotalSize;    
    }
    
    *aPtr = sPtr;
    
    *aResult = IDE_SUCCESS;
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    } 

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT(Well Known Text)를 읽어들여 
 * stdGeoCollection2DType이나 stdGeoCollection3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getGeoCollection( iduMemory*                 aQmxMem,
                                     UChar**                    aPtr,
                                     UChar*                     aWKTFence,
                                     stdGeometryHeader*         aGeom,
                                     void*                      aFence,
                                     IDE_RC*                    aResult,
                                     UInt                       aValidateOption,
                                     SInt                       aSRID )
{
    // 1.
    UChar*             sPtr = *aPtr;
    UChar*             sSubWKTFence;
    stdGeometryHeader* sCurrObj = (stdGeometryHeader*)STD_FIRST_COLL2D(aGeom); // 2D 3D 크기 같다.
    UInt               sObjectCnt = 0;
    UInt               sTotalSize = 0;
    stdGeoTypes        sCurrentType = STD_NULL_TYPE;
    STT_TOKEN          sTokenType = STT_SPACE_TOKEN;
    // 2.
    stdGeoCollection2DType*    sGeom2D = NULL;
    stdGeoCollection2DExtType* sGeom2DExt = NULL;
    idBool                     sIsValid = ID_FALSE;

    /* BUG-32531 Consider for GIS EMPTY */
    IDE_TEST(skipSpace(aPtr, aWKTFence) != IDE_SUCCESS);
			
    if ( idlOS::strncasecmp((SChar*)*aPtr,
                            STD_EMPTY_NAME, STD_EMPTY_NAME_LEN) == 0 )
    {
        IDE_TEST( checkValidEmpty(&sPtr, aWKTFence) != IDE_SUCCESS );
        IDE_TEST( getEmpty( aGeom, aFence, aResult ) != IDE_SUCCESS );
    }
    else
    {
        // BUG-41933
        idlOS::memset(aGeom, 0x00, STD_COLL2D_SIZE);
        stdUtils::nullify(aGeom);    // Fix BUG-15518

        // 1. Define whether 2D or 3D Type; Define sTotalSize; Test validation;
        IDE_TEST(skipLParen(&sPtr, aWKTFence) != IDE_SUCCESS);
        while((sPtr < aWKTFence) && (*sPtr != '\0') && 
              (sTokenType != STT_RPAREN_TOKEN))
        {
            IDE_TEST(skipSpace(&sPtr, aWKTFence) != IDE_SUCCESS);
            sSubWKTFence = findSubObjFence(&sPtr, aWKTFence);
        
            switch(*sPtr)
            {
                case 'p' :
                case 'P' :            
                    if(idlOS::strncasecmp((SChar*)sPtr, STD_POINT_NAME, 
                                          STD_POINT_NAME_LEN) == 0)
                    {
                        IDE_TEST_RAISE(sPtr + STD_POINT_NAME_LEN >= sSubWKTFence, err_parsing);
                        sPtr += STD_POINT_NAME_LEN;
                        // BUG-27002
                        IDE_TEST( getPoint( &sPtr,
                                            sSubWKTFence,
                                            sCurrObj,
                                            aFence,
                                            aResult,
                                            aValidateOption,
                                            aSRID )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
                    }
                    else if(idlOS::strncasecmp((SChar*)sPtr, STD_POLYGON_NAME, 
                                               STD_POLYGON_NAME_LEN) == 0)
                    {
                        IDE_TEST_RAISE(sPtr + STD_POLYGON_NAME_LEN >= sSubWKTFence, err_parsing);
                        sPtr += STD_POLYGON_NAME_LEN;
                        // BUG-27002                
                        IDE_TEST( getPolygon( aQmxMem,
                                              &sPtr,
                                              sSubWKTFence,
                                              sCurrObj,
                                              aFence,
                                              aResult,
                                              aValidateOption,
                                              aSRID )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
                    }
                    else
                    {
                        IDE_RAISE(err_parsing);
                    }
                    break;
                case 'l' :
                case 'L' :  
                    if(idlOS::strncasecmp((SChar*)sPtr, STD_LINESTRING_NAME, 
                                          STD_LINESTRING_NAME_LEN) == 0)
                    {
                        IDE_TEST_RAISE(sPtr + STD_LINESTRING_NAME_LEN >= sSubWKTFence, err_parsing);
                        sPtr += STD_LINESTRING_NAME_LEN;
                        // BUG-27002                
                        IDE_TEST( getLineString( &sPtr,
                                                 sSubWKTFence,
                                                 sCurrObj,
                                                 aFence,
                                                 aResult,
                                                 aValidateOption,
                                                 aSRID )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
                    }            
                    else
                    {
                        IDE_RAISE(err_parsing);
                    }
                    break;
                case 'm' :
                case 'M' :            
                    if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTIPOINT_NAME, 
                                          STD_MULTIPOINT_NAME_LEN) == 0)
                    {
                        IDE_TEST_RAISE(sPtr + STD_MULTIPOINT_NAME_LEN >= sSubWKTFence, err_parsing);
                        sPtr += STD_MULTIPOINT_NAME_LEN;
                        // BUG-27002                
                        IDE_TEST( getMultiPoint( &sPtr,
                                                 sSubWKTFence,
                                                 sCurrObj,
                                                 aFence,
                                                 aResult,
                                                 aValidateOption,
                                                 aSRID )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
                    }
                    else if(idlOS::strncasecmp((SChar*)sPtr,STD_MULTILINESTRING_NAME,
                                               STD_MULTILINESTRING_NAME_LEN) == 0)
                    {
                        IDE_TEST_RAISE(sPtr + STD_MULTILINESTRING_NAME_LEN >= sSubWKTFence, err_parsing);
                        sPtr += STD_MULTILINESTRING_NAME_LEN;
                        // BUG-27002                
                        IDE_TEST( getMultiLineString( &sPtr,
                                                      sSubWKTFence,
                                                      sCurrObj,
                                                      aFence,
                                                      aResult,
                                                      aValidateOption,
                                                      aSRID )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
                    }            
                    else if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTIPOLYGON_NAME, 
                                               STD_MULTIPOLYGON_NAME_LEN) == 0)
                    {
                        IDE_TEST_RAISE(sPtr + STD_MULTIPOLYGON_NAME_LEN >= sSubWKTFence, err_parsing);
                        sPtr += STD_MULTIPOLYGON_NAME_LEN;
                        // BUG-27002                
                        IDE_TEST( getMultiPolygon( aQmxMem,
                                                   &sPtr,
                                                   sSubWKTFence,
                                                   sCurrObj,
                                                   aFence,
                                                   aResult,
                                                   aValidateOption,
                                                   aSRID )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
                    }
                    else
                    {
                        IDE_RAISE(err_parsing);
                    }
                    break;
                default :
                    IDE_RAISE(err_parsing);
            } // switch
        
            if(sCurrentType == STD_NULL_TYPE)
            {
                IDE_TEST_RAISE( stdUtils::is2DType(sCurrObj->mType) == ID_FALSE,
                                err_parsing );
                
                if ( aSRID == ST_SRID_INIT )
                {
                    sCurrentType = STD_GEOCOLLECTION_2D_TYPE;
                }
                else
                {
                    // srid가 추가된 확장타입
                    sCurrentType = STD_GEOCOLLECTION_2D_EXT_TYPE;
                }
            }
            else
            {
                if ( ( sCurrentType == STD_GEOCOLLECTION_2D_TYPE ) ||
                     ( sCurrentType == STD_GEOCOLLECTION_2D_EXT_TYPE ) )
                {
                    IDE_TEST_RAISE( stdUtils::is2DType( sCurrObj->mType ) == ID_FALSE,
                                    err_parsing );
                }
                else
                {
                    IDE_RAISE(err_parsing);
                }
            }
        
            IDE_TEST_RAISE(sObjectCnt == ID_UINT_MAX, err_parsing);
            sObjectCnt++;

            IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, sCurrObj ) != IDE_SUCCESS );    // Fix BUG-15518
            sTotalSize += sCurrObj->mSize;
            sCurrObj = (stdGeometryHeader*)STD_NEXT_GEOM(sCurrObj);

            sTokenType = testNextToken(&sPtr, aWKTFence);
            IDE_TEST_RAISE((sTokenType != STT_COMMA_TOKEN) && 
                           (sTokenType != STT_RPAREN_TOKEN), err_parsing);

            if(sTokenType == STT_COMMA_TOKEN)
            {
                IDE_TEST(skipComma(&sPtr, aWKTFence) != IDE_SUCCESS);
            }
        } // while
        IDE_TEST_RAISE(sTotalSize == 0, err_parsing);
        IDE_TEST(skipRParen(&sPtr, aWKTFence) != IDE_SUCCESS);
    
        // Fix BUG-15980
        IDE_TEST(skipToLast(&sPtr, aWKTFence) != IDE_SUCCESS);    
    
        if ( sCurrentType == STD_GEOCOLLECTION_2D_TYPE )
        {
            sTotalSize += STD_COLL2D_SIZE;
        }
        else if ( sCurrentType == STD_GEOCOLLECTION_2D_EXT_TYPE )
        {
            sTotalSize += STD_COLL2D_EXT_SIZE;
        }
        else
        {
            IDE_RAISE( err_parsing );
        }
        
        IDE_TEST_RAISE( ( (UChar*)aGeom ) + sTotalSize > (UChar*)aFence, retry_memory );
        
        // 2. Storing Data =========================================================
        if ( stdUtils::is2DType( sCurrentType ) == ID_TRUE )
        {
            sGeom2D = (stdGeoCollection2DType*)aGeom;
            sGeom2D->mType = sCurrentType;
            sGeom2D->mSize = sTotalSize;
            sGeom2D->mNumGeometries = sObjectCnt;

            if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
            {
                if ( stdPrimitive::validateGeoCollection2D(aQmxMem, sGeom2D, sTotalSize)
                     == IDE_SUCCESS )
                {
                    sIsValid = ID_TRUE;
                }
                else
                {
                    sIsValid = ID_FALSE;
                }
            }
            else
            {
                sIsValid = ID_FALSE;
            }

            setValidHeader(aGeom, sIsValid, aValidateOption);
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
        
        // PROJ-2422 srid 지원
        if ( sCurrentType == STD_GEOCOLLECTION_2D_EXT_TYPE )
        {
            sGeom2DExt = (stdGeoCollection2DExtType*)aGeom;

            // set srid
            sGeom2DExt->mSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    *aPtr = sPtr;
    
    *aResult = IDE_SUCCESS;
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description:
 * BUG-32531 Consider for GIS EMPTY
 * WKT(Well Known Text)를 읽어들여
 * stdMultiPoint2DType이나 stdMultiPoint3DType 객체로 저장한다.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을
 * 허용하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 문자열 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): 문자열 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEmpty( stdGeometryHeader* aGeom,
                             void* aFence,
                             IDE_RC* aResult)
{
    UInt sTotalSize = 0;
			
    sTotalSize += STD_MPOINT2D_SIZE;
			
    IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
			
    idlOS::memset(aGeom, 0x00, sTotalSize);
			
    stdUtils::nullify(aGeom);
			
    aGeom->mType = STD_EMPTY_TYPE;
    aGeom->mSize = sTotalSize;
			
    *aResult = IDE_SUCCESS;
			
    return IDE_SUCCESS;
			
    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;
			
    return IDE_FAILURE;
}


// WKB 함수 ////////////////////////////////////////////////////////////////////

UChar *stdParsing::readWKB_Char( UChar *aBuf, UChar *aVal, UInt *aOffset )
{
    UChar *sNext;
    
    IDE_DASSERT( (aBuf!=NULL) && (aVal!=NULL) && (aOffset!=NULL) );
    
    sNext = aBuf + WKB_UCHAR_SIZE;
    idlOS::memcpy( aVal, aBuf, WKB_UCHAR_SIZE );
    *aOffset += WKB_UCHAR_SIZE;

    return sNext;
}

UChar *stdParsing::readWKB_UInt( UChar *aBuf, UInt *aVal, UInt *aOffset,
                                 idBool aEquiEndian)
{
    UChar *sNext;
    
    IDE_DASSERT( (aBuf!=NULL) && (aVal!=NULL) && (aOffset!=NULL) );
    
    sNext = aBuf + WKB_INT32_SIZE;
    idlOS::memcpy( aVal, aBuf, WKB_INT32_SIZE );

    // BUG-24357 WKB Endian
    if (aEquiEndian == ID_FALSE)
    {
        mtdInteger.endian(aVal);
    }    
    *aOffset += WKB_INT32_SIZE;

    return sNext;
}

UChar *stdParsing::readWKB_SInt( UChar *aBuf,
                                 SInt *aVal,
                                 UInt *aOffset,
                                 idBool aEquiEndian )
{
    UChar *sNext;
    
    IDE_DASSERT( ( aBuf!=NULL ) && ( aVal!=NULL ) && ( aOffset!=NULL ) );
    
    sNext = aBuf + WKB_INT32_SIZE;
    idlOS::memcpy( aVal, aBuf, WKB_INT32_SIZE );

    // BUG-24357 WKB Endian
    if ( aEquiEndian == ID_FALSE )
    {
        mtdInteger.endian( aVal );
    }    
    *aOffset += WKB_INT32_SIZE;

    return sNext;
}

UChar *stdParsing::readWKB_SDouble( UChar *aBuf, SDouble *aVal, UInt *aOffset,
                                    idBool aEquiEndian)
{
    UChar *sNext;
    
    IDE_DASSERT( (aBuf!=NULL) && (aVal!=NULL) && (aOffset!=NULL) );
    
    sNext = aBuf + WKB_DOUBLE_SIZE;
    idlOS::memcpy( aVal, aBuf, WKB_DOUBLE_SIZE );
    
    // BUG-24357 WKB Endian    
    if (aEquiEndian == ID_FALSE)
    {
        mtdDouble.endian(aVal);
    }
    
    *aOffset += WKB_DOUBLE_SIZE;

    return sNext;
}

IDE_RC stdParsing::readWKB_Header( UChar *aBuf, idBool *aIsEquiEndian, UInt *aType, UInt *aOffset, UChar **aNext )
{
    UChar *sNext;
    
    IDE_DASSERT( (aBuf!=NULL) && (aType!=NULL) && (aOffset!=NULL) );
    
    sNext = aBuf + WKB_GEOHEAD_SIZE;

    // BUG-24357 WKB Endian
    IDE_TEST( stdUtils::compareEndian(*aBuf, aIsEquiEndian) != IDE_SUCCESS );

    idlOS::memcpy( aType, aBuf+1, WKB_INT32_SIZE );

    if ( *aIsEquiEndian == ID_FALSE )
    {
        mtdInteger.endian(aType);
    }
    
    *aOffset += WKB_GEOHEAD_SIZE;

    if ( aNext != NULL )
    {
        *aNext = sNext;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UChar *stdParsing::readWKB_Point( UChar *aBuf, stdPoint2D *aPoint, UInt *aOffset,
                                  idBool aEquiEndian )
{
    UChar *sNext = aBuf;
    
    IDE_DASSERT( (aBuf!=NULL) && (aPoint!=NULL) && (aOffset!=NULL) );

    sNext = readWKB_SDouble( sNext, &aPoint->mX, aOffset, aEquiEndian );
    sNext = readWKB_SDouble( sNext, &aPoint->mY, aOffset, aEquiEndian );

    return sNext;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdPoint2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBPoint(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    UChar*          sPtr       = *aPtr;
    UInt            sWKBSize   = WKB_POINT_SIZE; // Fix BUG-15523
    UInt            sTotalSize = STD_POINT2D_SIZE;
    stdPoint2DType* sGeom2D    = (stdPoint2DType*)aGeom;
    WKBPoint*       sBPoint = (WKBPoint*)*aPtr;
    UInt            sWkbOffset = 0;
    UInt            sWkbType;

    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    // To Fix BUG-20892
    // BUGBUG : wkb parsing error.
    IDE_TEST_RAISE(sPtr + sWKBSize > aWKBFence, err_parsing);
    IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);

    idlOS::memset(aGeom, 0x00, sTotalSize);
    // Fix BUG-15290
    stdUtils::nullify(aGeom);

    aGeom->mType = STD_POINT_2D_TYPE;
    aGeom->mSize = sTotalSize;    

    IDE_TEST( readWKB_Header( (UChar*)sBPoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sWkbType != WKB_POINT_TYPE, err_parsing );
    
    readWKB_Point( (UChar*)&sBPoint->mPoint, &sGeom2D->mPoint, &sWkbOffset,
                   sEquiEndian );
    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing);
    
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, &sGeom2D->mPoint)
               != IDE_SUCCESS );
    
    *aPtr = sPtr + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }      

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdLineString2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBLineString(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sWKBSize = 0; // Fix BUG-15523
    UInt                    sTotalSize = 0;
    stdLineString2DType*    sGeom2D = (stdLineString2DType*)aGeom;
    stdPoint2D*             sPt2D = STD_FIRST_PT2D(sGeom2D);
    UInt                    i;
    WKBLineString*          sBLine = (WKBLineString*)*aPtr;
    wkbPoint*               sBPoint = WKB_FIRST_PTL(sBLine);
    UInt                    sWkbOffset = 0;
    UInt                    sWkbType;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;

    IDE_TEST_RAISE( (sPtr + WKB_LINE_SIZE) > aWKBFence, 
		    err_parsing);
    // { Calc WKB Size
    IDE_TEST( readWKB_Header( (UChar*)sBLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sWkbType != WKB_LINESTRING_TYPE , err_parsing);
    readWKB_UInt( sBLine->mNumPoints, &sPtCnt, &sWkbOffset,
                  sEquiEndian);
    sWKBSize = WKB_LINE_SIZE + WKB_PT_SIZE*sPtCnt;
    // } Calc WKB Size
    
    // Calc Geometry  Size
    sTotalSize = STD_LINE2D_SIZE + STD_PT2D_SIZE*sPtCnt;

    IDE_TEST_RAISE( sPtr + sWKBSize > aWKBFence, err_parsing);
    IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);
    
    sWkbOffset = 0;
    idlOS::memset(aGeom, 0x00, sTotalSize);
    // Fix BUG-15290
    stdUtils::nullify(aGeom);

    IDE_TEST( readWKB_Header( (UChar*)sBLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    readWKB_UInt( sBLine->mNumPoints, &sPtCnt, &sWkbOffset, sEquiEndian);

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sPtCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        aGeom->mType = STD_LINESTRING_2D_TYPE; 
    }
    
    aGeom->mSize = sTotalSize;    
    
    sGeom2D->mNumPoints = sPtCnt;
    for(i = 0; i < sPtCnt; i++)
    {
        readWKB_Point( (UChar*)sBPoint, sPt2D, &sWkbOffset, sEquiEndian );
        
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D )
                   != IDE_SUCCESS );
        
        sPt2D = STD_NEXT_PT2D(sPt2D);
        sBPoint = WKB_NEXT_PT(sBPoint);
    }

    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing );
    
    *aPtr    = (*aPtr) + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdPolygon2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBPolygon(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    UChar*              sPtr = *aPtr;
    UInt                sPtCnt = 0;
    UInt                sRingCnt = 0;
    UInt                sWKBSize = 0; // Fix BUG-15523
    UInt                sTotalSize = 0;
    stdPolygon2DType*   sGeom2D = (stdPolygon2DType*)aGeom;
    stdLinearRing2D*    sRing2D = NULL;
    stdPoint2D*         sPt2D = NULL;
    UInt                i, j;
    WKBPolygon*         sBPolygon = (WKBPolygon*)*aPtr;
    wkbLinearRing*      sBRing = NULL;
    wkbPoint*           sBPoint = NULL;
    UInt                sWkbOffset = 0;
    UInt                sWkbType;
    UInt                sNumPoints;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;

    IDE_TEST_RAISE( ( sPtr + WKB_POLY_SIZE ) > aWKBFence, err_parsing );
    
    // { Calc WKB Size
    IDE_TEST( readWKB_Header( (UChar*)sBPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != WKB_POLYGON_TYPE, err_parsing );
    readWKB_UInt( sBPolygon->mNumRings, &sRingCnt, &sWkbOffset, sEquiEndian );
    
    sBRing = WKB_FIRST_RN(sBPolygon);
    IDE_TEST_RAISE( (UChar*)sBRing > aWKBFence, err_parsing);

    for(i = 0; i < sRingCnt; i++)
    {
        // check valid position of numpoints
        IDE_TEST_RAISE(sBRing->mNumPoints > aWKBFence, err_parsing);        
                       
        readWKB_UInt( sBRing->mNumPoints, &sNumPoints, &sWkbOffset, sEquiEndian );
        
        IDE_TEST_RAISE(sNumPoints == 1 , err_parsing);
        
        sPtCnt += sNumPoints;
        // check valid postion of next ring.
        IDE_TEST_RAISE( (sPtr + sWkbOffset + WKB_PT_SIZE*sNumPoints) > aWKBFence, 
		  err_parsing);
        sBRing = (wkbLinearRing*)((UChar*)(sBRing) + 
                    WKB_RN_SIZE + WKB_PT_SIZE*sNumPoints);
    }
    
    sWKBSize = WKB_POLY_SIZE + WKB_INT32_SIZE*sRingCnt + WKB_PT_SIZE*sPtCnt;
    // } Calc WKB Size

    // Calc Geometry Size
    sTotalSize = STD_POLY2D_SIZE + STD_RN2D_SIZE*sRingCnt + STD_PT2D_SIZE*sPtCnt;

    IDE_TEST_RAISE(sPtr + sWKBSize > aWKBFence, err_parsing);
    IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);

    sWkbOffset = 0;    
    idlOS::memset(aGeom, 0x00, sTotalSize);
    // Fix BUG-15290
    stdUtils::nullify(aGeom);

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sRingCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        aGeom->mType = STD_POLYGON_2D_TYPE;
    }

    IDE_TEST( readWKB_Header( (UChar*)sBPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );

    readWKB_UInt( sBPolygon->mNumRings, &sRingCnt, &sWkbOffset, sEquiEndian );
    
    sGeom2D->mNumRings = sRingCnt;
        
    sRing2D = STD_FIRST_RN2D(sGeom2D);    // Move First Ring
    sBRing = WKB_FIRST_RN(sBPolygon);

    for(i = 0; i < sRingCnt; i++)
    {
        sPt2D = STD_FIRST_PT2D(sRing2D);    // Move First Point,
        readWKB_UInt( sBRing->mNumPoints, &sPtCnt, &sWkbOffset, sEquiEndian );

        sBPoint = WKB_FIRST_PTR(sBRing);
        sRing2D->mNumPoints = sPtCnt;
        
        for(j = 0; j < sPtCnt; j++)
        {
            IDE_TEST_RAISE( (UChar *)sBPoint > aWKBFence, err_parsing);
            readWKB_Point( (UChar*)sBPoint, sPt2D, &sWkbOffset, sEquiEndian );
            
            IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D )
                      != IDE_SUCCESS );            
        
            sPt2D = STD_NEXT_PT2D(sPt2D);
            sBPoint = WKB_NEXT_PT(sBPoint);
        }

        if( stdUtils::isSamePoints2D( STD_FIRST_PT2D(sRing2D),
                                      STD_PREV_PT2D(sPt2D)) == ID_FALSE )
        {
            sTotalSize += STD_PT2D_SIZE;
            IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence,
                           retry_memory);

            // To fix BUG-33472 CodeSonar warining

            if ( sPt2D != STD_FIRST_PT2D(sRing2D))
            {                        
                idlOS::memcpy( sPt2D, STD_FIRST_PT2D(sRing2D), STD_PT2D_SIZE);
            }
            else
            {
                // nothing to do   
            }
                
            sPt2D = STD_NEXT_PT2D(sPt2D);
            sRing2D->mNumPoints++;
        }       
        
        // Move Next Ring
        sRing2D = (stdLinearRing2D*)sPt2D;
        sBRing = (wkbLinearRing*)sBPoint;
        IDE_TEST_RAISE((UChar*) sBRing > aWKBFence, err_parsing);
    }

    // BUG-48051
    aGeom->mSize = sTotalSize;    

    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing );
    
    *aPtr    = sPtr + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }      

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  WKB(Well Known Binary)를 읽어들여 stdPolygon2DType 객체로 저장한다.
 *  WKB에는 3차원 데이터가 존재하지 않는다.
 *
 *  ST_RECTANGLE은 두 점(좌하단, 우상단)으로 이루어지는 직사각형을 표현한다.
 *  RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
 *
 *  UChar             ** aPtr(InOut)   : 읽어들일 WKB 버퍼 위치, 읽은 다음 그만큼 증가한다.
 *  UChar              * aWKTFence(In) : WKB 버퍼의 Fence
 *  stdGeometryHeader  * aGeom(Out)    : 저장될 버퍼(Geometry 객체)
 *  void               * aFence(In)    : 저장될 버퍼(Geometry 객체)의 Fence
 *  IDE_RC             * aResult(Out)  : Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBRectangle( UChar             ** aPtr,
                                    UChar              * aWKBFence,
                                    stdGeometryHeader  * aGeom,
                                    void               * aFence,
                                    IDE_RC             * aResult )
{
    UChar            * sPtr        = *aPtr;
    stdPoint2D         sPoint2D    = { (SDouble)0.0, (SDouble)0.0 };
    stdPolygon2DType * sGeom2D     = NULL;
    stdLinearRing2D  * sRing2D     = NULL;
    stdPoint2D       * sPt2D       = NULL;
    wkbPoint         * sBPoint     = NULL;
    UInt               sWkbOffset  = 0;
    UInt               sWkbType    = WKB_RECTANGLE_TYPE;
    // BUG-24357 WKB Endian
    idBool             sEquiEndian = ID_FALSE;

    // RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
    UInt               sWKBSize    = WKB_GEOHEAD_SIZE + ( WKB_PT_SIZE * 2 );
    UInt               sTotalSize  = ( STD_PT2D_SIZE * 5 ) + STD_RN2D_SIZE + STD_POLY2D_SIZE;
    SDouble            sX1         = (SDouble)0.0;
    SDouble            sY1         = (SDouble)0.0;
    SDouble            sX2         = (SDouble)0.0;
    SDouble            sY2         = (SDouble)0.0;

    IDE_TEST_RAISE( ( sPtr + sWKBSize ) > aWKBFence, ERR_PARSING );
    IDE_TEST_RAISE( ((UChar *)aGeom) + sTotalSize > (UChar *)aFence, RETRY_MEMORY );

    IDE_TEST( readWKB_Header( sPtr,
                              &sEquiEndian,
                              &sWkbType,
                              &sWkbOffset,
                              NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != WKB_RECTANGLE_TYPE, ERR_PARSING );

    sBPoint = (wkbPoint *)(sPtr + sWkbOffset);
    (void)readWKB_Point( (UChar *)sBPoint, &sPoint2D, &sWkbOffset, sEquiEndian );
    sX1 = sPoint2D.mX;
    sY1 = sPoint2D.mY;

    sBPoint = (wkbPoint *)(sPtr + sWkbOffset);
    (void)readWKB_Point( (UChar *)sBPoint, &sPoint2D, &sWkbOffset, sEquiEndian );
    sX2 = sPoint2D.mX;
    sY2 = sPoint2D.mY;

    idlOS::memset( aGeom, 0x00, sTotalSize );

    // Fix BUG-15290
    stdUtils::nullify( aGeom );

    aGeom->mType        = STD_POLYGON_2D_TYPE;
    aGeom->mSize        = sTotalSize;
    sGeom2D             = (stdPolygon2DType *)aGeom;
    sGeom2D->mNumRings  = 1;
    sRing2D             = STD_FIRST_RN2D( sGeom2D );    // Move First Ring
    sRing2D->mNumPoints = 5;
    sPt2D               = STD_FIRST_PT2D( sRing2D );    // Move First Point

    // RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
    sPt2D->mX = sX1;
    sPt2D->mY = sY1;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

    // Move Next Point
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX2;
    sPt2D->mY = sY1;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

    // Move Next Point
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX2;
    sPt2D->mY = sY2;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

    // Move Next Point
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX1;
    sPt2D->mY = sY2;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

    // Move Next Point
    sPt2D = STD_NEXT_PT2D( sPt2D );

    sPt2D->mX = sX1;
    sPt2D->mY = sY1;
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D ) != IDE_SUCCESS );

    *aPtr   += sWKBSize;
    *aResult = IDE_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PARSING );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }
    IDE_EXCEPTION( RETRY_MEMORY );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdMultiPoint2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBMultiPoint(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sWKBSize = 0; // Fix BUG-15523
    UInt                    sTotalSize = 0;
    stdMultiPoint2DType*    sGeom2D = (stdMultiPoint2DType*)aGeom;
    stdPoint2DType*         sPt2D = NULL;
    UInt                    i;
    WKBMultiPoint*          sBMpoint = (WKBMultiPoint*)*aPtr;
    WKBPoint*               sBPoint = NULL;
    UInt                    sWkbOffset = 0;
    UInt                    sWkbType;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    // { Calc WKB Size
    IDE_TEST_RAISE( ( sPtr + WKB_MPOINT_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBMpoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != WKB_MULTIPOINT_TYPE , err_parsing);
    readWKB_UInt( sBMpoint->mNumWKBPoints, &sPtCnt, &sWkbOffset, sEquiEndian );
    
    sWKBSize = WKB_MPOINT_SIZE + WKB_POINT_SIZE*sPtCnt;
    // } Calc WKB Size
    
    // Calc Geometry Size
    sTotalSize = STD_MPOINT2D_SIZE + STD_POINT2D_SIZE*sPtCnt;

    IDE_TEST_RAISE(sPtr + sWKBSize > aWKBFence, err_parsing);
    IDE_TEST_RAISE(((UChar*)aGeom) + sTotalSize > (UChar*)aFence, retry_memory);

    sWkbOffset = 0;
    idlOS::memset(aGeom, 0x00, sTotalSize);
    // Fix BUG-15290
    stdUtils::nullify(aGeom);

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sPtCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        aGeom->mType = STD_MULTIPOINT_2D_TYPE;
    }
    aGeom->mSize = sTotalSize;
    
    IDE_TEST( readWKB_Header( (UChar*)sBMpoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    readWKB_UInt( sBMpoint->mNumWKBPoints, &sPtCnt, &sWkbOffset, sEquiEndian );
    
    sGeom2D->mNumObjects = sPtCnt;
        
    sPt2D = STD_FIRST_POINT2D(sGeom2D);    // Move First Point
    sBPoint = WKB_FIRST_POINT(sBMpoint);
    for(i = 0; i < sPtCnt; i++)
    {
        IDE_TEST( readWKB_Header( (UChar*)sBPoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sWkbType != WKB_POINT_TYPE , err_parsing);
        readWKB_Point( (UChar*)&sBPoint->mPoint, &sPt2D->mPoint, &sWkbOffset,
                       sEquiEndian);
        
        sPt2D->mType = STD_POINT_2D_TYPE;
        sPt2D->mSize = STD_POINT2D_SIZE;
        stdUtils::mergeMBRFromPoint2D( (stdGeometryHeader*)sPt2D, 
            &sPt2D->mPoint );
        stdUtils::mergeMBRFromPoint2D( aGeom, &sPt2D->mPoint);
        
        // Move Next Point
        sPt2D = STD_NEXT_POINT2D(sPt2D);
        sBPoint = WKB_NEXT_POINT(sBPoint);
    }

    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing );
    
    *aPtr    = sPtr + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdMultiLineString2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBMultiLineString(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    stdMultiLineString2DType*   sGeom2D = (stdMultiLineString2DType*)aGeom;
    stdLineString2DType*        sLine2D = NULL;
    UChar*                      sPtr = *aPtr;
    UInt                        sObjectCnt;
    UInt                        sTotalSize = 0;
    UInt                        i;
    
    WKBMultiLineString*         sBMLine = (WKBMultiLineString*)*aPtr;
    WKBLineString*              sBLine;    
    UInt                        sWkbOffset = 0;
    UInt                        sWkbSize;
    UInt                        sWkbType;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    stdUtils::nullify(aGeom);

    IDE_TEST_RAISE( ( sPtr + WKB_MLINE_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBMLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != WKB_MULTILINESTRING_TYPE, err_parsing );
    readWKB_UInt( sBMLine->mNumWKBLineStrings, &sObjectCnt, &sWkbOffset,
                  sEquiEndian);

    sLine2D = STD_FIRST_LINE2D(sGeom2D);    // Move First Line
    sBLine = WKB_FIRST_LINE(sBMLine);
    for(i = 0; i < sObjectCnt; i++)
    {
        IDE_TEST_RAISE( (sPtr + sWkbOffset + WKB_GEOHEAD_SIZE) > aWKBFence, 
                        err_parsing);
        IDE_TEST( readWKB_Header( (UChar*)sBLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sWkbType != WKB_LINESTRING_TYPE, err_parsing );

        sWkbOffset -= WKB_GEOHEAD_SIZE;
       
        switch( sWkbType )
        {
        case WKB_LINESTRING_TYPE :
            IDE_TEST( getWKBLineString( (UChar**)&sBLine, aWKBFence, 
                (stdGeometryHeader*)sLine2D, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        default :
            IDE_RAISE(IDE_EXCEPTION_END_LABEL);
        } // switch
        
        IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, (stdGeometryHeader*)sLine2D )
                  != IDE_SUCCESS );
        sTotalSize += sLine2D->mSize;
        
        sLine2D = STD_NEXT_LINE2D(sLine2D);
    } // while

    sWkbSize   = ( ((SChar*)sBLine) - ((SChar*)sBMLine) ); // Fix BUG-16445
    sTotalSize += STD_MLINE2D_SIZE;

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sObjectCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        sGeom2D->mType = STD_MULTILINESTRING_2D_TYPE;
    }

    sGeom2D->mSize = sTotalSize;
    sGeom2D->mNumObjects = sObjectCnt;

    IDE_TEST( (sPtr + sWkbSize) > aWKBFence);
    *aPtr = sPtr + sWkbSize;  // Fix BUG-16445
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdMultiPolygon2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBMultiPolygon(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    stdMultiPolygon2DType*  sGeom2D = (stdMultiPolygon2DType*)aGeom;
    stdPolygon2DType*       sPoly2D = NULL;
    UChar*                  sPtr = *aPtr;
    UInt                    sObjectCnt;
    UInt                    sTotalSize = 0;
    UInt                    i;
    UInt                    sWkbSize;
    
    WKBMultiPolygon*        sBMPolygon = (WKBMultiPolygon*)*aPtr;
    WKBPolygon*             sBPolygon;
    UInt                    sWkbOffset = 0;
    UInt                    sWkbType;

    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    stdUtils::nullify(aGeom);

    IDE_TEST_RAISE( ( sPtr + WKB_MPOLY_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBMPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != WKB_MULTIPOLYGON_TYPE, err_parsing );
    readWKB_UInt( sBMPolygon->mNumWKBPolygons, &sObjectCnt, &sWkbOffset,
                  sEquiEndian);
    
    sPoly2D = STD_FIRST_POLY2D(sGeom2D);    // Move First Polygon
    sBPolygon = WKB_FIRST_POLY(sBMPolygon);
    for(i = 0; i < sObjectCnt; i++)
    {
        IDE_TEST_RAISE( (sPtr + sWkbOffset + WKB_GEOHEAD_SIZE) > aWKBFence, 
                        err_parsing);
        IDE_TEST( readWKB_Header( (UChar*)sBPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sWkbType != WKB_POLYGON_TYPE, err_parsing );
        sWkbOffset -= WKB_GEOHEAD_SIZE;
       
        switch( sWkbType )
        {
        case WKB_POLYGON_TYPE :
            IDE_TEST( getWKBPolygon( (UChar**)&sBPolygon, aWKBFence, 
                (stdGeometryHeader*)sPoly2D, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        default :
            IDE_RAISE(IDE_EXCEPTION_END_LABEL);
        } // switch
        
        IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, (stdGeometryHeader*)sPoly2D )
                  != IDE_SUCCESS );
        sTotalSize += sPoly2D->mSize;
        
        sPoly2D = STD_NEXT_POLY2D(sPoly2D);
    } // while
    
    sWkbSize   = ( ((SChar*)sBPolygon) - ((SChar*)sBMPolygon) ); // Fix BUG-16445
    sTotalSize += STD_MPOLY2D_SIZE;

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sObjectCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        sGeom2D->mType = STD_MULTIPOLYGON_2D_TYPE;
    }

    sGeom2D->mSize = sTotalSize;
    sGeom2D->mNumObjects = sObjectCnt;

    IDE_TEST( (sPtr + sWkbSize) > aWKBFence);
    
    *aPtr = sPtr + sWkbSize; // Fix BUG-16445
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKB(Well Known Binary)를 읽어들여 stdGeoCollection2DType 객체로 저장한다.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getWKBGeoCollection(
                    UChar**                    aPtr,
                    UChar*                     aWKBFence,
                    stdGeometryHeader*         aGeom,
                    void*                      aFence,
                    IDE_RC*                    aResult)
{
    stdGeoCollection2DType* sGeom2D = (stdGeoCollection2DType*)aGeom;
    stdGeometryHeader*      sCurrObj;
    UChar*                  sPtr = *aPtr;
    UInt                    sObjectCnt;
    UInt                    sTotalSize = 0;
    UInt                    i;
    
    WKBGeometryCollection*  sBColl = (WKBGeometryCollection*)*aPtr;
    WKBGeometry*            sBCurrObj;
    UInt                    sWkbOffset = 0;
    UInt                    sWkbType;

    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    stdUtils::nullify(aGeom);

    IDE_TEST_RAISE( ( sPtr + WKB_COLL_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBColl, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != WKB_COLLECTION_TYPE, err_parsing );
    readWKB_UInt( sBColl->mNumWKBGeometries, &sObjectCnt, &sWkbOffset, sEquiEndian );
    
    sBCurrObj = WKB_FIRST_COLL(sBColl);
    sCurrObj = (stdGeometryHeader*)STD_FIRST_COLL2D(sGeom2D);
    for(i = 0; i < sObjectCnt; i++)
    {
        IDE_TEST( readWKB_Header( (UChar*)sBCurrObj, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        sWkbOffset -= WKB_GEOHEAD_SIZE;
       
        switch( sWkbType )
        {
        case WKB_POINT_TYPE :
            IDE_TEST( getWKBPoint((UChar**)&sBCurrObj, 
                aWKBFence, sCurrObj, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        case WKB_LINESTRING_TYPE :
            IDE_TEST( getWKBLineString((UChar**)&sBCurrObj, 
                aWKBFence, sCurrObj, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        case WKB_POLYGON_TYPE :
            IDE_TEST( getWKBPolygon((UChar**)&sBCurrObj, 
                aWKBFence, sCurrObj, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        case WKB_MULTIPOINT_TYPE :
            IDE_TEST( getWKBMultiPoint((UChar**)&sBCurrObj, 
                aWKBFence, sCurrObj, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        case WKB_MULTILINESTRING_TYPE :
            IDE_TEST( getWKBMultiLineString((UChar**)&sBCurrObj, 
                aWKBFence, sCurrObj, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        case WKB_MULTIPOLYGON_TYPE :
            IDE_TEST( getWKBMultiPolygon((UChar**)&sBCurrObj, 
                aWKBFence, sCurrObj, aFence, aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);
            break;
        default :
            IDE_RAISE(IDE_EXCEPTION_END_LABEL);
        } // switch
        
        IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, sCurrObj )
                  != IDE_SUCCESS );
        sTotalSize += sCurrObj->mSize;
        
        sCurrObj = (stdGeometryHeader*)STD_NEXT_GEOM(sCurrObj);
    } // while
    
    sTotalSize += STD_COLL2D_SIZE;

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sObjectCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        sGeom2D->mType = STD_GEOCOLLECTION_2D_TYPE;
    }

    sGeom2D->mSize = sTotalSize;
    sGeom2D->mNumGeometries = sObjectCnt;

    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * WKT(Well Known Text)를 읽어들여 Geometry 객체를 생성하는 Core 함수.
 * 3차원 좌표가 들어있으면 3차원으로 간주하며, 2차원 3차원 값을 혼합 입력하는 것을 
 * 허용하지 않는다.
 *
 * UChar **aPtr(In): 읽어들일 문자열 위치
 * UInt aWKTLength(In): 문자열 버퍼의 길이
 * void* aBuf(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::stdValue( iduMemory*   aQmxMem,
                             UChar*       aWKT,
                             UInt         aWKTLength,
                             void*        aBuf,
                             void*        aFence,
                             IDE_RC*      aResult,
                             UInt         aValidateOption,
                             idBool       aSRIDOption,
                             SInt         aSRID )
{
    stdGeometryHeader * sGeom = (stdGeometryHeader*)aBuf;
    UChar* sPtr = aWKT;
    UChar* sWKTFence = aWKT + aWKTLength;
    UInt   sSize = STD_GEOHEAD_SIZE;
    SInt   sSRID = ST_SRID_INIT;

    IDE_TEST_RAISE(((UChar*)sGeom) + sSize > (UChar*)aFence, retry_memory);

    IDE_TEST_RAISE(aWKTLength == 0, err_null_object);

    /*
    // Fix BUG-15290
    // initialize... ( mMinX > mMaxX )
    sGeom->mMbr.mMinX = 0; 
    sGeom->mMbr.mMinY = 0;
    sGeom->mMbr.mMaxX = -1;
    sGeom->mMbr.mMaxY = -1;
    idlOS::memset((void*)&sGeom->mMbr.mMinT, 0x00, ID_SIZEOF(sGeom->mMbr.mMinT));
    idlOS::memset((void*)&sGeom->mMbr.mMaxT, 0x00, ID_SIZEOF(sGeom->mMbr.mMaxT));
    sGeom->mSize = 0;
    */

    IDE_TEST(stdParsing::skipSpace(&sPtr, sWKTFence) != IDE_SUCCESS);

    // PROJ-2422 srid 지원
    // SRID=4326;POINT(1 1)
    if ( aSRIDOption == ID_TRUE )
    {
        if ( sPtr + STD_SRID_NAME_LEN < sWKTFence )
        {
            if ( idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0 )
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( getSRID( &sPtr, sWKTFence, &sSRID ) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace( &sPtr, sWKTFence ) != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        if ( aSRID != ST_SRID_INIT )
        {
            // 상위에서 내려준 srid가 우선한다.
            sSRID = aSRID;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
    
    switch(*sPtr)
    {
    case 'p' :
    case 'P' :        
        if(idlOS::strncasecmp((SChar*)sPtr, STD_POINT_NAME, 
                              STD_POINT_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE( sPtr + STD_POINT_NAME_LEN >= sWKTFence,
                            err_parsing );
            sPtr += STD_POINT_NAME_LEN;
            IDE_TEST( stdParsing::getPoint( &sPtr,
                                            sWKTFence,
                                            sGeom,
                                            aFence,
                                            aResult,
                                            aValidateOption,
                                            sSRID )
                      != IDE_SUCCESS );
        }
        else if(idlOS::strncasecmp((SChar*)sPtr, STD_POLYGON_NAME, 
                                   STD_POLYGON_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE(sPtr + STD_POLYGON_NAME_LEN >= sWKTFence,
                           err_parsing);
            sPtr += STD_POLYGON_NAME_LEN;
            IDE_TEST( stdParsing::getPolygon( aQmxMem,
                                              &sPtr,
                                              sWKTFence,
                                              sGeom,
                                              aFence,
                                              aResult,
                                              aValidateOption,
                                              sSRID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
        break;
    case 'r' : /* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다. */
    case 'R' :
        if ( idlOS::strncasecmp( (SChar *)sPtr,
                                 STD_RECTANGLE_NAME,
                                 STD_RECTANGLE_NAME_LEN ) == 0 )
        {
            IDE_TEST_RAISE( sPtr + STD_RECTANGLE_NAME_LEN >= sWKTFence,
                            err_parsing );
            sPtr += STD_RECTANGLE_NAME_LEN;
            IDE_TEST( stdParsing::getRectangle( aQmxMem,
                                                &sPtr,
                                                sWKTFence,
                                                sGeom,
                                                aFence,
                                                aResult,
                                                aValidateOption )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE( err_parsing );
        }
        break;
    case 'l' :
    case 'L' :
        if(idlOS::strncasecmp((SChar*)sPtr, STD_LINESTRING_NAME, 
                              STD_LINESTRING_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE( sPtr + STD_LINESTRING_NAME_LEN >= sWKTFence,
                            err_parsing );
            sPtr += STD_LINESTRING_NAME_LEN;
            IDE_TEST( stdParsing::getLineString( &sPtr,
                                                 sWKTFence,
                                                 sGeom,
                                                 aFence,
                                                 aResult,
                                                 aValidateOption,
                                                 sSRID )
                      != IDE_SUCCESS );
        }            
        else
        {
            IDE_RAISE(err_parsing);
        }
        break;
    case 'm' :
    case 'M' :
        if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTIPOINT_NAME, 
                              STD_MULTIPOINT_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE(sPtr + STD_MULTIPOINT_NAME_LEN >= sWKTFence,
                           err_parsing);
            sPtr += STD_MULTIPOINT_NAME_LEN;
            IDE_TEST( stdParsing::getMultiPoint( &sPtr,
                                                 sWKTFence,
                                                 sGeom,
                                                 aFence,
                                                 aResult,
                                                 aValidateOption,
                                                 sSRID )
                      != IDE_SUCCESS );
        }            
        else if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTILINESTRING_NAME, 
                                   STD_MULTILINESTRING_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE(sPtr + STD_MULTILINESTRING_NAME_LEN >= sWKTFence,
                           err_parsing);
            sPtr += STD_MULTILINESTRING_NAME_LEN;
            IDE_TEST( stdParsing::getMultiLineString( &sPtr,
                                                      sWKTFence,
                                                      sGeom,
                                                      aFence,
                                                      aResult,
                                                      aValidateOption,
                                                      sSRID )
                      != IDE_SUCCESS );
        }            
        else if(idlOS::strncasecmp((SChar*)sPtr, STD_MULTIPOLYGON_NAME, 
                                   STD_MULTIPOLYGON_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE(sPtr + STD_MULTIPOLYGON_NAME_LEN >= sWKTFence,
                           err_parsing);
            sPtr += STD_MULTIPOLYGON_NAME_LEN;
            IDE_TEST( stdParsing::getMultiPolygon( aQmxMem,
                                                   &sPtr,
                                                   sWKTFence,
                                                   sGeom,
                                                   aFence,
                                                   aResult,
                                                   aValidateOption,
                                                   sSRID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
        break;
    case 'g' :
    case 'G' :        
        if(idlOS::strncasecmp((SChar*)sPtr, STD_GEOCOLLECTION_NAME, 
                              STD_GEOCOLLECTION_NAME_LEN) == 0)
        {
            IDE_TEST_RAISE(sPtr + STD_GEOCOLLECTION_NAME_LEN >= sWKTFence,
                           err_parsing);
            sPtr += STD_GEOCOLLECTION_NAME_LEN;
            IDE_TEST( stdParsing::getGeoCollection( aQmxMem,
                                                    &sPtr,
                                                    sWKTFence,
                                                    sGeom,
                                                    aFence,
                                                    aResult,
                                                    aValidateOption,
                                                    sSRID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE(err_parsing);
        }
        break;    
    default :
        IDE_RAISE(err_invalid_object_mType);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(retry_memory);
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS; 
    }
    IDE_EXCEPTION(err_null_object);
    {
        stdGeometry.null(stdGeometry.column, aBuf);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * WKB(Well Known Binary)를 읽어들여 Geometry 객체를 생성하는 Core 함수.
 * WKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar  *aWKB(In): 읽어들일 WKB 버퍼 위치
 * UInt    aWKBLength(In): WKB 버퍼의 길이
 * void   *aBuf(Out): 저장될 버퍼(Geometry 객체)
 * void   *aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC *aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::stdBinValue( iduMemory*   aQmxMem,
                                UChar*       aWKB,
                                UInt         aWKBLength,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption )
{
    stdGeometryHeader * sGeom = (stdGeometryHeader*)aBuf;
    UChar* sPtr = (UChar*)aWKB;
    UChar* sWKBFence = (UChar*)aWKB + aWKBLength;
    UInt   sSize = STD_GEOHEAD_SIZE;
    UInt   sWkbType;
    UInt   sWkbOffset = 0;
    idBool sIsValid = ID_TRUE;

    // BUG-24357 WKB Endian  
    idBool sIsEquiEndian = ID_FALSE;
    
    IDE_TEST_RAISE(((UChar*)sGeom) + sSize > (UChar*)aFence, retry_memory);
    
    if(aWKBLength == 0)
    {
        IDE_RAISE(err_null_object);
    }
    
    /*
    // Fix BUG-15290
    // initialize... ( mMinX > mMaxX )
    sGeom->mMbr.mMinX = 0; 
    sGeom->mMbr.mMinY = 0;
    sGeom->mMbr.mMaxX = -1;
    sGeom->mMbr.mMaxY = -1;
    idlOS::memset((void*)&sGeom->mMbr.mMinT, 0x00, ID_SIZEOF(sGeom->mMbr.mMinT));
    idlOS::memset((void*)&sGeom->mMbr.mMaxT, 0x00, ID_SIZEOF(sGeom->mMbr.mMaxT));
    sGeom->mSize = 0;
    */

    IDE_TEST_RAISE( stdParsing::readWKB_Header(aWKB, 
                                               &sIsEquiEndian,
                                               &sWkbType,
                                               &sWkbOffset,
                                               NULL)
                    != IDE_SUCCESS,
                    err_invalid_object_mType );
    
    switch(sWkbType)
    {
        case WKB_POINT_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBPoint(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_POINT_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBPoint(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_LINESTRING_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBLineString(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_LINESTRING_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBLineString(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_POLYGON_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBPolygon(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_POLYGON_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBPolygon(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_MULTIPOINT_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBMultiPoint(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_MULTIPOINT_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBMultiPoint(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_MULTILINESTRING_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBMultiLineString(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_MULTILINESTRING_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBMultiLineString(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_MULTIPOLYGON_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBMultiPolygon(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_MULTIPOLYGON_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBMultiPolygon(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_COLLECTION_TYPE :
            IDE_TEST_RAISE( stdParsing::getWKBGeoCollection(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult) != IDE_SUCCESS, err_parsing);
            IDE_TEST_RAISE(*aResult != IDE_SUCCESS, retry_memory);    
            break;
        case EWKB_COLLECTION_TYPE :
        /* PROJ-2422 srid */
            IDE_TEST_RAISE( stdParsing::getEWKBGeoCollection(
                                &sPtr, sWKBFence, sGeom, aFence,
                                aResult ) != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_RECTANGLE_TYPE :   /* BUG-44399 ST_RECTFROMTEXT(), ST_RECTFROMWKB()를 지원해야 합니다. */
            IDE_TEST_RAISE( stdParsing::getWKBRectangle( &sPtr,
                                                         sWKBFence,
                                                         sGeom,
                                                         aFence,
                                                         aResult )
                            != IDE_SUCCESS, err_parsing );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        default :
            IDE_RAISE(err_invalid_object_mType);
    }

    // FIX BUG-22257
    if ( aValidateOption == STU_VALIDATION_ENABLE_TRUE )
    {
        if ( stdPrimitive::validate(aQmxMem, sGeom, sGeom->mSize)
             == IDE_SUCCESS )
        {
            sIsValid = ID_TRUE;
        }
        else
        {
            sIsValid = ID_FALSE;
        }
    }
    else
    {
        sIsValid = ID_FALSE;
    }        

    setValidHeader(sGeom, sIsValid, aValidateOption);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(retry_memory);
    {
        // BUG-22088
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION(err_null_object);
    {
        stdGeometry.null(stdGeometry.column, aBuf);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKB));
    }
    IDE_EXCEPTION(err_invalid_object_mType);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_NOT_SUPPORTED_OBJECT_TYPE));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void stdParsing::setValidHeader( stdGeometryHeader * aGeom,
                                 idBool              aIsValid,
                                 UInt                aValidateOption)
{
    if (aValidateOption == STU_VALIDATION_ENABLE_FALSE)
    {
        // validate disable인 경우 validation이 무시되는 것들이 많아
        // valid하다고 하더라도 unknown으로 기록한다.
        if (aIsValid == ID_TRUE)
        {
            aGeom->mIsValid = ST_UNKNOWN;
        }
        else
        {
            aGeom->mIsValid = ST_INVALID;
        }
    }
    else
    {
        if (aIsValid == ID_TRUE)
        {
            aGeom->mIsValid = ST_VALID;
        }
        else
        {
            aGeom->mIsValid = ST_INVALID;
        }
    }
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdPoint2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBPoint( UChar**                    aPtr,
                                 UChar*                     aWKBFence,
                                 stdGeometryHeader*         aGeom,
                                 void*                      aFence,
                                 IDE_RC*                    aResult )
{
    UChar*             sPtr       = *aPtr;
    UInt               sWKBSize   = EWKB_POINT_SIZE; // Fix BUG-15523
    UInt               sTotalSize = STD_POINT2D_EXT_SIZE;
    stdPoint2DExtType* sGeom2D    = (stdPoint2DExtType*)aGeom;
    EWKBPoint*         sBPoint    = (EWKBPoint*)*aPtr;
    UInt               sWkbOffset = 0;
    UInt               sWkbType;

    // BUG-24357 WKB Endian
    idBool             sEquiEndian = ID_FALSE;
    
    // To Fix BUG-20892
    // BUGBUG : wkb parsing error.
    IDE_TEST_RAISE( sPtr + sWKBSize > aWKBFence, err_parsing );
    IDE_TEST_RAISE( ( (UChar*)aGeom ) + sTotalSize > (UChar*)aFence, retry_memory );

    idlOS::memset( aGeom, 0x00, sTotalSize );
    // Fix BUG-15290
    stdUtils::nullify( aGeom );

    aGeom->mType = STD_POINT_2D_EXT_TYPE;
    aGeom->mSize = sTotalSize;

    IDE_TEST( readWKB_Header( (UChar*)sBPoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sWkbType != EWKB_POINT_TYPE, err_parsing );
    
    readWKB_SInt( (UChar*)sBPoint->mSRID, &sGeom2D->mSRID, &sWkbOffset,
                   sEquiEndian );
    readWKB_Point( (UChar*)&sBPoint->mPoint, &sGeom2D->mPoint, &sWkbOffset,
                   sEquiEndian );
    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing);
    
    IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, &sGeom2D->mPoint)
               != IDE_SUCCESS );
    
    *aPtr = sPtr + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( retry_memory );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }      

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdLineString2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBLineString( UChar**                    aPtr,
                                      UChar*                     aWKBFence,
                                      stdGeometryHeader*         aGeom,
                                      void*                      aFence,
                                      IDE_RC*                    aResult )
{
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sWKBSize = 0; // Fix BUG-15523
    UInt                    sTotalSize = 0;
    stdLineString2DExtType* sGeom2D = (stdLineString2DExtType*)aGeom;
    stdPoint2D*             sPt2D = STD_FIRST_PT2D( sGeom2D );
    UInt                    i;
    EWKBLineString*         sBLine = (EWKBLineString*)*aPtr;
    wkbPoint*               sBPoint = EWKB_FIRST_PTL( sBLine );
    UInt                    sWkbOffset = 0;
    UInt                    sWkbType;
    SInt                    sSRID;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;

    IDE_TEST_RAISE( (sPtr + EWKB_LINE_SIZE) > aWKBFence, 
                    err_parsing );
    // { Calc WKB Size
    IDE_TEST( readWKB_Header( (UChar*)sBLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != EWKB_LINESTRING_TYPE , err_parsing );

    readWKB_SInt( sBLine->mSRID, &sSRID, &sWkbOffset,
                  sEquiEndian );
    readWKB_UInt( sBLine->mNumPoints, &sPtCnt, &sWkbOffset,
                  sEquiEndian);
    sWKBSize = EWKB_LINE_SIZE + WKB_PT_SIZE*sPtCnt;
    // } Calc WKB Size
    
    // Calc Geometry  Size
    sTotalSize = STD_LINE2D_EXT_SIZE + STD_PT2D_SIZE*sPtCnt;

    IDE_TEST_RAISE( sPtr + sWKBSize > aWKBFence, err_parsing );
    IDE_TEST_RAISE( ( (UChar*)aGeom ) + sTotalSize > (UChar*)aFence, retry_memory );
    
    sWkbOffset = 0;
    idlOS::memset( aGeom, 0x00, sTotalSize );
    // Fix BUG-15290
    stdUtils::nullify( aGeom );

    IDE_TEST( readWKB_Header( (UChar*)sBLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    readWKB_SInt( sBLine->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBLine->mNumPoints, &sPtCnt, &sWkbOffset, sEquiEndian );

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sPtCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        aGeom->mType = STD_LINESTRING_2D_EXT_TYPE; 
    }

    aGeom->mSize = sTotalSize;    
    
    sGeom2D->mSRID = sSRID;
    sGeom2D->mNumPoints = sPtCnt;
    for ( i = 0; i < sPtCnt; i++ )
    {
        readWKB_Point( (UChar*)sBPoint, sPt2D, &sWkbOffset, sEquiEndian );
        
        IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D )
                  != IDE_SUCCESS );
        
        sPt2D = STD_NEXT_PT2D( sPt2D );
        sBPoint = WKB_NEXT_PT( sBPoint );
    }

    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing );
    
    *aPtr    = (*aPtr) + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( retry_memory );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdPolygon2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBPolygon( UChar**                    aPtr,
                                   UChar*                     aWKBFence,
                                   stdGeometryHeader*         aGeom,
                                   void*                      aFence,
                                   IDE_RC*                    aResult )
{
    UChar*                sPtr = *aPtr;
    UInt                  sPtCnt = 0;
    UInt                  sRingCnt = 0;
    UInt                  sWKBSize = 0; // Fix BUG-15523
    UInt                  sTotalSize = 0;
    stdPolygon2DExtType*  sGeom2D = (stdPolygon2DExtType*)aGeom;
    stdLinearRing2D*      sRing2D = NULL;
    stdPoint2D*           sPt2D = NULL;
    UInt                  i, j;
    EWKBPolygon*          sBPolygon = (EWKBPolygon*)*aPtr;
    wkbLinearRing*        sBRing = NULL;
    wkbPoint*             sBPoint = NULL;
    UInt                  sWkbOffset = 0;
    UInt                  sWkbType;
    UInt                  sNumPoints;
    SInt                  sSRID;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;

    IDE_TEST_RAISE( ( sPtr + EWKB_POLY_SIZE ) > aWKBFence, err_parsing );
    
    // { Calc WKB Size
    IDE_TEST( readWKB_Header( (UChar*)sBPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != EWKB_POLYGON_TYPE, err_parsing );

    readWKB_SInt( sBPolygon->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBPolygon->mNumRings, &sRingCnt, &sWkbOffset, sEquiEndian );
    
    sBRing = EWKB_FIRST_RN( sBPolygon );
    IDE_TEST_RAISE( (UChar*)sBRing > aWKBFence, err_parsing );

    for ( i = 0; i < sRingCnt; i++ )
    {
        // check valid position of numpoints
        IDE_TEST_RAISE( sBRing->mNumPoints > aWKBFence, err_parsing );
                       
        readWKB_UInt( sBRing->mNumPoints, &sNumPoints, &sWkbOffset, sEquiEndian );
        
        IDE_TEST_RAISE( sNumPoints == 1 , err_parsing );
        
        sPtCnt += sNumPoints;
        // check valid postion of next ring.
        IDE_TEST_RAISE( ( sPtr + sWkbOffset + WKB_PT_SIZE*sNumPoints ) > aWKBFence,
                        err_parsing );
        sBRing = (wkbLinearRing*)( (UChar*)( sBRing ) + 
                                   WKB_RN_SIZE + WKB_PT_SIZE*sNumPoints );
    }
    
    sWKBSize = EWKB_POLY_SIZE + WKB_INT32_SIZE*sRingCnt + WKB_PT_SIZE*sPtCnt;
    // } Calc WKB Size

    // Calc Geometry Size
    sTotalSize = STD_POLY2D_EXT_SIZE + STD_RN2D_SIZE*sRingCnt + STD_PT2D_SIZE*sPtCnt;

    IDE_TEST_RAISE( sPtr + sWKBSize > aWKBFence, err_parsing );
    IDE_TEST_RAISE( ( (UChar*)aGeom ) + sTotalSize > (UChar*)aFence, retry_memory );

    sWkbOffset = 0;    
    idlOS::memset( aGeom, 0x00, sTotalSize );
    // Fix BUG-15290
    stdUtils::nullify( aGeom );

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sRingCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        aGeom->mType = STD_POLYGON_2D_EXT_TYPE;
    }

    IDE_TEST( readWKB_Header( (UChar*)sBPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );

    readWKB_SInt( sBPolygon->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBPolygon->mNumRings, &sRingCnt, &sWkbOffset, sEquiEndian );
    
    sGeom2D->mSRID = sSRID;
    sGeom2D->mNumRings = sRingCnt;
        
    sRing2D = STD_FIRST_RN2D( sGeom2D );    // Move First Ring
    sBRing = EWKB_FIRST_RN( sBPolygon );

    for ( i = 0; i < sRingCnt; i++ )
    {
        sPt2D = STD_FIRST_PT2D( sRing2D );    // Move First Point,
        readWKB_UInt( sBRing->mNumPoints, &sPtCnt, &sWkbOffset, sEquiEndian );

        sBPoint = WKB_FIRST_PTR( sBRing );
        sRing2D->mNumPoints = sPtCnt;
        
        for ( j = 0; j < sPtCnt; j++ )
        {
            IDE_TEST_RAISE( (UChar *)sBPoint > aWKBFence, err_parsing );
            readWKB_Point( (UChar*)sBPoint, sPt2D, &sWkbOffset, sEquiEndian );
            
            IDE_TEST( stdUtils::mergeMBRFromPoint2D( aGeom, sPt2D )
                      != IDE_SUCCESS );            
        
            sPt2D = STD_NEXT_PT2D( sPt2D );
            sBPoint = WKB_NEXT_PT( sBPoint );
        }

        if ( stdUtils::isSamePoints2D( STD_FIRST_PT2D( sRing2D ),
                                       STD_PREV_PT2D( sPt2D ) ) == ID_FALSE )
        {
            sTotalSize += STD_PT2D_SIZE;
            IDE_TEST_RAISE( ( (UChar*)aGeom ) + sTotalSize > (UChar*)aFence,
                            retry_memory );

            // To fix BUG-33472 CodeSonar warining

            if ( sPt2D != STD_FIRST_PT2D( sRing2D ) )
            {                        
                idlOS::memcpy( sPt2D, STD_FIRST_PT2D( sRing2D ), STD_PT2D_SIZE );
            }
            else
            {
                // nothing to do   
            }
                
            sPt2D = STD_NEXT_PT2D( sPt2D );
            sRing2D->mNumPoints++;
        }       
        
        // Move Next Ring
        sRing2D = (stdLinearRing2D*)sPt2D;
        sBRing = (wkbLinearRing*)sBPoint;
        IDE_TEST_RAISE( (UChar*) sBRing > aWKBFence, err_parsing );
    }

    // BUG-48051
    aGeom->mSize = sTotalSize;    

    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing );
    
    *aPtr    = sPtr + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( retry_memory );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }      

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdMultiPoint2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBMultiPoint( UChar**                    aPtr,
                                      UChar*                     aWKBFence,
                                      stdGeometryHeader*         aGeom,
                                      void*                      aFence,
                                      IDE_RC*                    aResult )
{
    UChar*                  sPtr = *aPtr;
    UInt                    sPtCnt = 0;
    UInt                    sWKBSize = 0; // Fix BUG-15523
    UInt                    sTotalSize = 0;
    stdMultiPoint2DExtType* sGeom2D = (stdMultiPoint2DExtType*)aGeom;
    stdPoint2DType*         sPt2D = NULL;
    UInt                    i;
    EWKBMultiPoint*         sBMpoint = (EWKBMultiPoint*)*aPtr;
    WKBPoint*               sBPoint = NULL;
    UInt                    sWkbOffset = 0;
    UInt                    sWkbType;
    SInt                    sSRID;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    // { Calc WKB Size
    IDE_TEST_RAISE( (sPtr + EWKB_MPOINT_SIZE) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBMpoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != EWKB_MULTIPOINT_TYPE , err_parsing );

    readWKB_SInt( sBMpoint->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBMpoint->mNumWKBPoints, &sPtCnt, &sWkbOffset, sEquiEndian );
    
    sWKBSize = EWKB_MPOINT_SIZE + WKB_POINT_SIZE*sPtCnt;
    // } Calc WKB Size
    
    // Calc Geometry Size
    sTotalSize = STD_MPOINT2D_EXT_SIZE + STD_POINT2D_SIZE*sPtCnt;

    IDE_TEST_RAISE( sPtr + sWKBSize > aWKBFence, err_parsing );
    IDE_TEST_RAISE( ( (UChar*)aGeom ) + sTotalSize > (UChar*)aFence, retry_memory );

    sWkbOffset = 0;
    idlOS::memset( aGeom, 0x00, sTotalSize );
    // Fix BUG-15290
    stdUtils::nullify( aGeom );

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sPtCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        aGeom->mType = STD_MULTIPOINT_2D_EXT_TYPE;
    }
    aGeom->mSize = sTotalSize;
    
    IDE_TEST( readWKB_Header( (UChar*)sBMpoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    readWKB_SInt( sBMpoint->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBMpoint->mNumWKBPoints, &sPtCnt, &sWkbOffset, sEquiEndian );
    
    sGeom2D->mSRID = sSRID;
    sGeom2D->mNumObjects = sPtCnt;
        
    sPt2D = STD_FIRST_POINT2D( sGeom2D );    // Move First Point
    sBPoint = EWKB_FIRST_POINT( sBMpoint );
    for ( i = 0; i < sPtCnt; i++ )
    {
        IDE_TEST( readWKB_Header( (UChar*)sBPoint, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sWkbType != WKB_POINT_TYPE , err_parsing );
        readWKB_Point( (UChar*)&sBPoint->mPoint, &sPt2D->mPoint, &sWkbOffset,
                       sEquiEndian );
        
        sPt2D->mType = STD_POINT_2D_TYPE;
        sPt2D->mSize = STD_POINT2D_SIZE;
        stdUtils::mergeMBRFromPoint2D( (stdGeometryHeader*)sPt2D, 
                                        &sPt2D->mPoint );
        stdUtils::mergeMBRFromPoint2D( aGeom, &sPt2D->mPoint );
        
        // Move Next Point
        sPt2D = STD_NEXT_POINT2D( sPt2D );
        sBPoint = WKB_NEXT_POINT( sBPoint );
    }

    IDE_TEST_RAISE( sWkbOffset != sWKBSize, err_parsing );
    
    *aPtr    = sPtr + sWKBSize;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( retry_memory );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdMultiLineString2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBMultiLineString( UChar**                    aPtr,
                                           UChar*                     aWKBFence,
                                           stdGeometryHeader*         aGeom,
                                           void*                      aFence,
                                           IDE_RC*                    aResult )
{
    stdMultiLineString2DExtType*  sGeom2D = (stdMultiLineString2DExtType*)aGeom;
    stdLineString2DType*          sLine2D = NULL;
    UChar*                        sPtr = *aPtr;
    UInt                          sObjectCnt;
    UInt                          sTotalSize = 0;
    UInt                          i;
    
    EWKBMultiLineString*          sBMLine = (EWKBMultiLineString*)*aPtr;
    WKBLineString*                sBLine;    
    UInt                          sWkbOffset = 0;
    UInt                          sWkbSize;
    UInt                          sWkbType;
    SInt                          sSRID;
    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    stdUtils::nullify( aGeom );

    IDE_TEST_RAISE( ( sPtr + EWKB_MLINE_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBMLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != EWKB_MULTILINESTRING_TYPE, err_parsing );

    readWKB_SInt( sBMLine->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBMLine->mNumWKBLineStrings, &sObjectCnt, &sWkbOffset,
                  sEquiEndian );

    sLine2D = STD_FIRST_LINE2D( sGeom2D );    // Move First Line
    sBLine = EWKB_FIRST_LINE( sBMLine );
    for ( i = 0; i < sObjectCnt; i++ )
    {
        IDE_TEST_RAISE( ( sPtr + sWkbOffset + WKB_GEOHEAD_SIZE ) > aWKBFence, 
                        err_parsing);
        IDE_TEST( readWKB_Header( (UChar*)sBLine, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sWkbType != WKB_LINESTRING_TYPE, err_parsing );

        sWkbOffset -= WKB_GEOHEAD_SIZE;
       
        switch( sWkbType )
        {
        case WKB_LINESTRING_TYPE :
            IDE_TEST( getWKBLineString( (UChar**)&sBLine,
                                        aWKBFence, 
                                        (stdGeometryHeader*)sLine2D,
                                        aFence,
                                        aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        default :
            IDE_RAISE( IDE_EXCEPTION_END_LABEL );
        } // switch
        
        IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, (stdGeometryHeader*)sLine2D )
                  != IDE_SUCCESS );
        sTotalSize += sLine2D->mSize;
        
        sLine2D = STD_NEXT_LINE2D( sLine2D );
    } // while

    sWkbSize   = ( ((SChar*)sBLine) - ((SChar*)sBMLine) ); // Fix BUG-16445
    sTotalSize += STD_MLINE2D_EXT_SIZE;

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sObjectCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        sGeom2D->mType = STD_MULTILINESTRING_2D_EXT_TYPE;
    }

    sGeom2D->mSize = sTotalSize;
    sGeom2D->mSRID = sSRID;
    sGeom2D->mNumObjects = sObjectCnt;

    IDE_TEST( (sPtr + sWkbSize) > aWKBFence );
    *aPtr = sPtr + sWkbSize;  // Fix BUG-16445
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( retry_memory );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdMultiPolygon2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBMultiPolygon( UChar**                    aPtr,
                                        UChar*                     aWKBFence,
                                        stdGeometryHeader*         aGeom,
                                        void*                      aFence,
                                        IDE_RC*                    aResult )
{
    stdMultiPolygon2DExtType*  sGeom2D = (stdMultiPolygon2DExtType*)aGeom;
    stdPolygon2DType*          sPoly2D = NULL;
    UChar*                     sPtr = *aPtr;
    UInt                       sObjectCnt;
    UInt                       sTotalSize = 0;
    UInt                       i;
    UInt                       sWkbSize;
    
    EWKBMultiPolygon*          sBMPolygon = (EWKBMultiPolygon*)*aPtr;
    WKBPolygon*                sBPolygon;
    UInt                       sWkbOffset = 0;
    UInt                       sWkbType;
    SInt                       sSRID;

    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    stdUtils::nullify( aGeom );

    IDE_TEST_RAISE( ( sPtr + EWKB_MPOLY_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBMPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != EWKB_MULTIPOLYGON_TYPE, err_parsing );

    readWKB_SInt( sBMPolygon->mSRID, &sSRID, &sWkbOffset,
                  sEquiEndian );
    readWKB_UInt( sBMPolygon->mNumWKBPolygons, &sObjectCnt, &sWkbOffset,
                  sEquiEndian );
    
    sPoly2D = STD_FIRST_POLY2D( sGeom2D );    // Move First Polygon
    sBPolygon = EWKB_FIRST_POLY( sBMPolygon );
    for ( i = 0; i < sObjectCnt; i++ )
    {
        IDE_TEST_RAISE( ( sPtr + sWkbOffset + WKB_GEOHEAD_SIZE ) > aWKBFence, 
                        err_parsing );
        IDE_TEST( readWKB_Header( (UChar*)sBPolygon, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sWkbType != WKB_POLYGON_TYPE, err_parsing );
        sWkbOffset -= WKB_GEOHEAD_SIZE;
       
        switch( sWkbType )
        {
        case WKB_POLYGON_TYPE :
            IDE_TEST( getWKBPolygon( (UChar**)&sBPolygon,
                                     aWKBFence,
                                     (stdGeometryHeader*)sPoly2D,
                                     aFence,
                                     aResult )!= IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        default :
            IDE_RAISE( IDE_EXCEPTION_END_LABEL );
        } // switch
        
        IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, (stdGeometryHeader*)sPoly2D )
                  != IDE_SUCCESS );
        sTotalSize += sPoly2D->mSize;
        
        sPoly2D = STD_NEXT_POLY2D( sPoly2D );
    } // while
    
    sWkbSize   = ( ( (SChar*)sBPolygon ) - ( (SChar*)sBMPolygon ) ); // Fix BUG-16445
    sTotalSize += STD_MPOLY2D_EXT_SIZE;

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sObjectCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        sGeom2D->mType = STD_MULTIPOLYGON_2D_EXT_TYPE;
    }

    sGeom2D->mSize = sTotalSize;
    sGeom2D->mSRID = sSRID;
    sGeom2D->mNumObjects = sObjectCnt;

    IDE_TEST( ( sPtr + sWkbSize ) > aWKBFence );
    
    *aPtr = sPtr + sWkbSize; // Fix BUG-16445
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( retry_memory );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKB(Extended Well Known Binary)를 읽어들여 stdGeoCollection2DExtType 객체로 저장한다.
 * EWKB에는 3차원 데이터가 존재하지 않는다.
 *
 * UChar **aPtr(InOut): 읽어들일 WKB 버퍼 위치, 읽은 다음 그 만큼 증가한다.
 * UChar *aWKTFence(In): WKB 버퍼의 Fence
 * stdGeometryHeader* aGeom(Out): 저장될 버퍼(Geometry 객체)
 * void* aFence(In): 저장될 버퍼(Geometry 객체)의 Fence
 * IDE_RC* aResult(Out): Error Code
 **********************************************************************/
IDE_RC stdParsing::getEWKBGeoCollection( UChar**                    aPtr,
                                         UChar*                     aWKBFence,
                                         stdGeometryHeader*         aGeom,
                                         void*                      aFence,
                                         IDE_RC*                    aResult )
{
    stdGeoCollection2DExtType* sGeom2D = (stdGeoCollection2DExtType*)aGeom;
    stdGeometryHeader*         sCurrObj;
    UChar*                     sPtr = *aPtr;
    UInt                       sObjectCnt;
    UInt                       sTotalSize = 0;
    UInt                       i;
    
    EWKBGeometryCollection*    sBColl = (EWKBGeometryCollection*)*aPtr;
    WKBGeometry*               sBCurrObj;
    UInt                       sWkbOffset = 0;
    UInt                       sWkbType;
    SInt                       sSRID;

    // BUG-24357 WKB Endian
    idBool          sEquiEndian = ID_FALSE;
    
    stdUtils::nullify( aGeom );

    IDE_TEST_RAISE( ( sPtr + EWKB_COLL_SIZE ) > aWKBFence, err_parsing );
    
    IDE_TEST( readWKB_Header( (UChar*)sBColl, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sWkbType != EWKB_COLLECTION_TYPE, err_parsing );

    readWKB_SInt( sBColl->mSRID, &sSRID, &sWkbOffset, sEquiEndian );
    readWKB_UInt( sBColl->mNumWKBGeometries, &sObjectCnt, &sWkbOffset, sEquiEndian );
    
    sBCurrObj = EWKB_FIRST_COLL( sBColl );
    sCurrObj = (stdGeometryHeader*)STD_FIRST_COLL2D( sGeom2D );
    for ( i = 0; i < sObjectCnt; i++ )
    {
        IDE_TEST( readWKB_Header( (UChar*)sBCurrObj, &sEquiEndian, &sWkbType, &sWkbOffset, NULL )
                  != IDE_SUCCESS );
        sWkbOffset -= WKB_GEOHEAD_SIZE;
       
        switch( sWkbType )
        {
        case WKB_POINT_TYPE :
            IDE_TEST( getWKBPoint( (UChar**)&sBCurrObj,
                                   aWKBFence,
                                   sCurrObj,
                                   aFence,
                                   aResult) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case EWKB_POINT_TYPE :
            IDE_TEST( getEWKBPoint( (UChar**)&sBCurrObj,
                                    aWKBFence,
                                    sCurrObj,
                                    aFence,
                                    aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_LINESTRING_TYPE :
            IDE_TEST( getWKBLineString( (UChar**)&sBCurrObj,
                                        aWKBFence,
                                        sCurrObj,
                                        aFence,
                                        aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case EWKB_LINESTRING_TYPE :
            IDE_TEST( getEWKBLineString( (UChar**)&sBCurrObj,
                                         aWKBFence,
                                         sCurrObj,
                                         aFence,
                                         aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_POLYGON_TYPE :
            IDE_TEST( getWKBPolygon( (UChar**)&sBCurrObj,
                                     aWKBFence,
                                     sCurrObj,
                                     aFence,
                                     aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case EWKB_POLYGON_TYPE :
            IDE_TEST( getEWKBPolygon( (UChar**)&sBCurrObj,
                                      aWKBFence,
                                      sCurrObj,
                                      aFence,
                                      aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_MULTIPOINT_TYPE :
            IDE_TEST( getWKBMultiPoint( (UChar**)&sBCurrObj,
                                        aWKBFence,
                                        sCurrObj,
                                        aFence,
                                        aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case EWKB_MULTIPOINT_TYPE :
            IDE_TEST( getEWKBMultiPoint( (UChar**)&sBCurrObj,
                                         aWKBFence,
                                         sCurrObj,
                                         aFence,
                                         aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_MULTILINESTRING_TYPE :
            IDE_TEST( getWKBMultiLineString( (UChar**)&sBCurrObj,
                                             aWKBFence,
                                             sCurrObj,
                                             aFence,
                                             aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case EWKB_MULTILINESTRING_TYPE :
            IDE_TEST( getEWKBMultiLineString( (UChar**)&sBCurrObj,
                                              aWKBFence,
                                              sCurrObj,
                                              aFence,
                                              aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case WKB_MULTIPOLYGON_TYPE :
            IDE_TEST( getWKBMultiPolygon( (UChar**)&sBCurrObj,
                                          aWKBFence,
                                          sCurrObj,
                                          aFence,
                                          aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        case EWKB_MULTIPOLYGON_TYPE :
            IDE_TEST( getEWKBMultiPolygon( (UChar**)&sBCurrObj,
                                           aWKBFence,
                                           sCurrObj,
                                           aFence,
                                           aResult ) != IDE_SUCCESS );
            IDE_TEST_RAISE( *aResult != IDE_SUCCESS, retry_memory );
            break;
        default :
            IDE_RAISE( IDE_EXCEPTION_END_LABEL );
        } // switch
        
        IDE_TEST( stdUtils::mergeMBRFromHeader( aGeom, sCurrObj )
                  != IDE_SUCCESS );
        sTotalSize += sCurrObj->mSize;
        
        sCurrObj = (stdGeometryHeader*)STD_NEXT_GEOM( sCurrObj );
    } // while
    
    sTotalSize += STD_COLL2D_EXT_SIZE;

    /* BUG-32531 Consider for GIS EMPTY */
    if ( sObjectCnt == 0 )
    {
        aGeom->mType = STD_EMPTY_TYPE;
    }
    else
    {
        sGeom2D->mType = STD_GEOCOLLECTION_2D_EXT_TYPE;
    }

    sGeom2D->mSize = sTotalSize;
    sGeom2D->mSRID = sSRID;
    sGeom2D->mNumGeometries = sObjectCnt;

    *aPtr = sPtr;
    *aResult = IDE_SUCCESS;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( retry_memory );
    {
        *aResult = IDE_FAILURE;
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKB ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
