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
 * $Id: stfWKT.cpp 18883 2006-11-14 01:48:40Z sabbra $
 *
 * Description:
 * WKT(Well Known Text)로부터 Geometry 객체 생성하는 함수
 * 상세 구현은 stdParsing.cpp 에 있다.
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <mtdTypes.h>
#include <ste.h>
#include <stdTypes.h>
#include <stdParsing.h>
#include <stfWKT.h>

/***********************************************************************
 * Description:
 * WKT로부터 Geometry 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::geomFromText( iduMemory*   aQmxMem,
                             void*        aWKT,
                             void*        aBuf,
                             void*        aFence,
                             IDE_RC*      aResult,
                             UInt         aValidateOption,
                             idBool       aSRIDOption,
                             SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
}

/***********************************************************************
 * Description:
 * WKT로부터 포인트 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::pointFromText( iduMemory*   aQmxMem,
                              void*        aWKT,
                              void*        aBuf,
                              void*        aFence,
                              IDE_RC*      aResult,
                              UInt         aValidateOption,
                              idBool       aSRIDOption,
                              SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_POINT_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_POINT_NAME, 
                                            STD_POINT_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(err_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT로부터 라인 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::lineFromText( iduMemory*   aQmxMem,
                             void*        aWKT,
                             void*        aBuf,
                             void*        aFence,
                             IDE_RC*      aResult,
                             UInt         aValidateOption,
                             idBool       aSRIDOption,
                             SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_LINESTRING_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_LINESTRING_NAME, 
                                            STD_LINESTRING_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(err_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT로부터 폴리곤 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::polyFromText( iduMemory*   aQmxMem,
                             void*        aWKT,
                             void*        aBuf,
                             void*        aFence,
                             IDE_RC*      aResult,
                             UInt         aValidateOption,
                             idBool       aSRIDOption,
                             SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE(stdParsing::skipSpace(&sPtr, sWKTFence) != IDE_SUCCESS,
                   err_parsing);
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_POLYGON_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_POLYGON_NAME, 
                                            STD_POLYGON_NAME_LEN ) != 0,
                       err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(err_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  WKT로부터 RECTANGLE 객체를 읽어 들인다.
 *
 *  void   * aWKT(In)     : 읽어 들일 버퍼
 *  void   * aBuf(Out)    : 출력할 버퍼
 *  void   * aFence(In)   : 출력할 버퍼 펜스
 *  IDE_RC * aResult(Out) : Error code
 **********************************************************************/
IDE_RC stfWKT::rectFromText( iduMemory  * aQmxMem,
                             void       * aWKT,
                             void       * aBuf,
                             void       * aFence,
                             IDE_RC     * aResult,
                             UInt         aValidateOption,
                             idBool       aSRIDOption,
                             SInt         aSRID )
{
    UChar * sWKT       = ((mtdCharType *)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType *)aWKT)->length;

    UChar * sPtr       = sWKT;
    UChar * sWKTFence  = sWKT + sWKTLength;

    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sWKTFence ) != IDE_SUCCESS,
                    ERR_PARSING );
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_RECTANGLE_NAME_LEN >= sWKTFence, ERR_OBJECT_TYPE );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_RECTANGLE_NAME, 
                                            STD_RECTANGLE_NAME_LEN ) != 0,
                        ERR_OBJECT_TYPE );
    }
    else
    {
        // null object인 경우
    }

    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );

    IDE_EXCEPTION( ERR_PARSING );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    IDE_EXCEPTION( ERR_OBJECT_TYPE );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT로부터 멀티포인트 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::mpointFromText( iduMemory*   aQmxMem,
                               void*        aWKT,
                               void*        aBuf,
                               void*        aFence,
                               IDE_RC*      aResult,
                               UInt         aValidateOption,
                               idBool       aSRIDOption,
                               SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE(stdParsing::skipSpace(&sPtr, sWKTFence) != IDE_SUCCESS,
                   err_parsing);
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_MULTIPOINT_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_MULTIPOINT_NAME, 
                                            STD_MULTIPOINT_NAME_LEN ) != 0,
                       err_object_type );
    }
    else
    {
        // null object인 경우
    }
        
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    IDE_EXCEPTION( err_object_type );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE ) );
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT로부터 멀티라인 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::mlineFromText( iduMemory*   aQmxMem,
                              void*        aWKT,
                              void*        aBuf,
                              void*        aFence,
                              IDE_RC*      aResult,
                              UInt         aValidateOption,
                              idBool       aSRIDOption,
                              SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE(stdParsing::skipSpace(&sPtr, sWKTFence) != IDE_SUCCESS,
                   err_parsing);
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_MULTILINESTRING_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_MULTILINESTRING_NAME,
                                            STD_MULTILINESTRING_NAME_LEN ) != 0,
                       err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(err_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT로부터 멀티폴리곤 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::mpolyFromText( iduMemory*   aQmxMem,
                              void*        aWKT,
                              void*        aBuf,
                              void*        aFence,
                              IDE_RC*      aResult,
                              UInt         aValidateOption,
                              idBool       aSRIDOption,
                              SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE(stdParsing::skipSpace(&sPtr, sWKTFence) != IDE_SUCCESS,
                   err_parsing);
    
    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_MULTIPOLYGON_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_MULTIPOLYGON_NAME, 
                                            STD_MULTIPOLYGON_NAME_LEN ) != 0,
                       err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(err_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 * WKT로부터 콜렉션 객체를 읽어 들인다.
 *
 * void*    aWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfWKT::geoCollFromText( iduMemory*   aQmxMem,
                                void*        aWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption,
                                idBool       aSRIDOption,
                                SInt         aSRID )
{
    UChar*  sWKT = ((mtdCharType*)aWKT)->value;
    UInt    sWKTLength = ((mtdCharType*)aWKT)->length;
   
    UChar*  sPtr = sWKT;
    UChar*  sWKTFence = sWKT + sWKTLength;
    
    IDE_TEST_RAISE(stdParsing::skipSpace(&sPtr, sWKTFence) != IDE_SUCCESS,
                   err_parsing);

    if ( sPtr != sWKTFence )
    {
        IDE_TEST_RAISE( sPtr + STD_GEOCOLLECTION_NAME_LEN >= sWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_GEOCOLLECTION_NAME,
                                            STD_GEOCOLLECTION_NAME_LEN ) != 0,
                       err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sWKT,
                                 sWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 aSRIDOption,
                                 aSRID );
    
    IDE_EXCEPTION(err_parsing);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_INVALID_WKT));
    }
    IDE_EXCEPTION(err_object_type);
    {
        IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


