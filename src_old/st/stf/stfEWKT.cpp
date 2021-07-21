/***********************************************************************
 * Copyright 1999-2006, ALTIBase Corporation or its subsididiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 *
 * Description:
 * EWKT(Extended Well Known Text)로부터 Geometry 객체 생성하는 함수
 * 상세 구현은 stdParsing.cpp 에 있다.
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideMsgLog.h>
#include <mtdTypes.h>
#include <ste.h>
#include <stdTypes.h>
#include <stdParsing.h>
#include <stfEWKT.h>

// BUG-47966
extern mtdModule stdGeometry;

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKT로부터 Geometry 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::geomFromEWKT( iduMemory*   aQmxMem,
                              void*        aEWKT,
                              void*        aBuf,
                              void*        aFence,
                              IDE_RC*      aResult,
                              UInt         aValidateOption )
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKT로부터 포인트 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::pointFromEWKT( iduMemory*   aQmxMem,
                               void*        aEWKT,
                               void*        aBuf,
                               void*        aFence,
                               IDE_RC*      aResult,
                               UInt         aValidateOption )               
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sEWKTFence )
    {
        if ( sPtr + STD_SRID_NAME_LEN < sEWKTFence )
        {
            if (idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0)
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( stdParsing::skipSRID( &sPtr, sEWKTFence ) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS );
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

        IDE_TEST_RAISE( sPtr + STD_POINT_NAME_LEN >= sEWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_POINT_NAME,
                                            STD_POINT_NAME_LEN ) != 0,
                        err_object_type);
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
    
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
 * Description: PROJ-2422 SRID
 * EWKT로부터 라인 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::lineFromEWKT( iduMemory*   aQmxMem,
                              void*        aEWKT,
                              void*        aBuf,
                              void*        aFence,
                              IDE_RC*      aResult,
                              UInt         aValidateOption )     
                  
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sEWKTFence )
    {
        if ( sPtr + STD_SRID_NAME_LEN < sEWKTFence )
        {
            if ( idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0 )
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( stdParsing::skipSRID(&sPtr, sEWKTFence) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace(&sPtr, sEWKTFence) != IDE_SUCCESS );
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

        IDE_TEST_RAISE( sPtr + STD_LINESTRING_NAME_LEN >= sEWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_LINESTRING_NAME, 
                                            STD_LINESTRING_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
    
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
 * Description: PROJ-2422 SRID
 * WKT나EWKT로부터 폴리곤 객체를 읽어 들인다.
 * BUG-47966 WKT 또는 EWKT를 먼저 파싱한 후에 객체의 타입을 확인하도록 수정
 *
 * void*    aEWKT(In): 읽어 들일 버퍼( WKT, EWKT 모두 올 수 있다.)
 * void*    aBuf(Out): 출력할 버퍼 (GEOMETRY)
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code - stdParsing::stdValue 결과
 * SInt     aSRID(In): 설정할 SRID값
 **********************************************************************/
IDE_RC stfEWKT::polyFromEWKT( iduMemory*   aQmxMem,
                              void*        aEWKT,
                              void*        aBuf,
                              void*        aFence,
                              IDE_RC*      aResult,
                              UInt         aValidateOption,
                              SInt         aSRID )     
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;

    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;

    IDE_TEST( stdParsing::stdValue( aQmxMem,
                                    sEWKT,
                                    sEWKTLength,
                                    aBuf,
                                    aFence,
                                    aResult,
                                    aValidateOption,
                                    ID_TRUE,
                                    aSRID )
              != IDE_SUCCESS );

    if ( *aResult == IDE_SUCCESS )
    { 
        if ( ((stdGeometryHeader *)aBuf)->mType == STD_EMPTY_TYPE )
        {
            IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                            err_parsing );

            if ( sPtr + STD_SRID_NAME_LEN <= sEWKTFence )
            {
                if ( idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0 )
                {
                    sPtr += STD_SRID_NAME_LEN;
                    IDE_TEST( stdParsing::skipSRID( &sPtr, sEWKTFence ) != IDE_SUCCESS );
                    IDE_TEST( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS );
                }
            }

            if ( sPtr + STD_POLYGON_NAME_LEN <= sEWKTFence )
            {
                if ( idlOS::strncasecmp( (SChar*)sPtr,
                                         STD_POLYGON_NAME, 
                                         STD_POLYGON_NAME_LEN ) != 0 )
                {
                    // retrun NULL
                    stdGeometry.null(stdGeometry.column, aBuf);
                }
                else
                {
                    // polygon empty 인 경우 
                    // Nothing to do.
                }
            }
            else
            {
                // retrun NULL
                stdGeometry.null(stdGeometry.column, aBuf);
            }
        }
        else
        {
            if ( ( ((stdGeometryHeader *)aBuf)->mType != STD_POLYGON_2D_TYPE ) &&
                 ( ((stdGeometryHeader *)aBuf)->mType != STD_POLYGON_2D_EXT_TYPE ) )    
            {
                // retrun NULL
                stdGeometry.null(stdGeometry.column, aBuf);
            }
            else
            {
                // polygon_2d 또는 polygon_2d_ext type 인 경우 
                // Nothing to do.
            }
        }
    }
    else
    {
        // 발생할 수 있는 에러 : retry_memory( 정의한 메모리 크기를 넘어가는 경우)
        // Nothing to do.
    }

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
 * EWKT로부터 멀티포인트 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::mpointFromEWKT( iduMemory*   aQmxMem,
                                void*        aEWKT,
                                void*        aBuf,
                                void*        aFence,
                                IDE_RC*      aResult,
                                UInt         aValidateOption )     
                               
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sEWKTFence )
    {
        if ( sPtr + STD_SRID_NAME_LEN < sEWKTFence )
        {
            if ( idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0 )
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( stdParsing::skipSRID( &sPtr, sEWKTFence ) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS );
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

        IDE_TEST_RAISE( sPtr + STD_MULTIPOINT_NAME_LEN >= sEWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_MULTIPOINT_NAME, 
                                            STD_MULTIPOINT_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
        
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
    
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
 * Description: PROJ-2422 SRID
 * EWKT로부터 멀티라인 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::mlineFromEWKT( iduMemory*   aQmxMem,
                               void*        aEWKT,
                               void*        aBuf,
                               void*        aFence,
                               IDE_RC*      aResult,
                               UInt         aValidateOption )     
                              
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sEWKTFence )
    {
        if ( sPtr + STD_SRID_NAME_LEN < sEWKTFence )
        {
            if ( idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0 )
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( stdParsing::skipSRID(&sPtr, sEWKTFence) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace(&sPtr, sEWKTFence) != IDE_SUCCESS );
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

        IDE_TEST_RAISE( sPtr + STD_MULTILINESTRING_NAME_LEN >= sEWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_MULTILINESTRING_NAME,
                                            STD_MULTILINESTRING_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
    
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
 * Description: PROJ-2422 SRID
 * EWKT로부터 멀티폴리곤 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::mpolyFromEWKT( iduMemory*   aQmxMem,
                               void*        aEWKT,
                               void*        aBuf,
                               void*        aFence,
                               IDE_RC*      aResult,
                               UInt         aValidateOption )     
                       
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                    err_parsing );
    
    if ( sPtr != sEWKTFence )
    {
        if ( sPtr + STD_SRID_NAME_LEN < sEWKTFence )
        {
            if ( idlOS::strncasecmp( (SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN ) == 0 )
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( stdParsing::skipSRID( &sPtr, sEWKTFence ) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS );
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

        IDE_TEST_RAISE( sPtr + STD_MULTIPOLYGON_NAME_LEN >= sEWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_MULTIPOLYGON_NAME, 
                                            STD_MULTIPOLYGON_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
    
    IDE_EXCEPTION( err_parsing );
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_INVALID_WKT ) );
    }
    IDE_EXCEPTION( err_object_type);
    {
        IDE_SET( ideSetErrorCode( stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE ) );
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description: PROJ-2422 SRID
 * EWKT로부터 콜렉션 객체를 읽어 들인다.
 *
 * void*    aEWKT(In): 읽어 들일 버퍼
 * void*    aBuf(Out): 출력할 버퍼
 * void*    aFence(In): 출력할 버퍼 펜스
 * IDE_RC*  aResult(Out): Error code
 **********************************************************************/
IDE_RC stfEWKT::geoCollFromEWKT( iduMemory*   aQmxMem,
                                 void*        aEWKT,
                                 void*        aBuf,
                                 void*        aFence,
                                 IDE_RC*      aResult,
                                 UInt         aValidateOption )     
                  
{
    UChar*  sEWKT = ((mtdCharType*)aEWKT)->value;
    UInt    sEWKTLength = ((mtdCharType*)aEWKT)->length;
   
    UChar*  sPtr = sEWKT;
    UChar*  sEWKTFence = sEWKT + sEWKTLength;
    
    IDE_TEST_RAISE( stdParsing::skipSpace( &sPtr, sEWKTFence ) != IDE_SUCCESS,
                    err_parsing );

    if ( sPtr != sEWKTFence )
    {
        if (sPtr + STD_SRID_NAME_LEN < sEWKTFence )
        {
            if (idlOS::strncasecmp((SChar*)sPtr, STD_SRID_NAME, STD_SRID_NAME_LEN) == 0)
            {
                sPtr += STD_SRID_NAME_LEN;
                IDE_TEST( stdParsing::skipSRID(&sPtr, sEWKTFence) != IDE_SUCCESS );
                IDE_TEST( stdParsing::skipSpace(&sPtr, sEWKTFence) != IDE_SUCCESS );
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
        
        IDE_TEST_RAISE( sPtr + STD_GEOCOLLECTION_NAME_LEN >= sEWKTFence, err_object_type );
        IDE_TEST_RAISE( idlOS::strncasecmp( (SChar*)sPtr, STD_GEOCOLLECTION_NAME,
                                            STD_GEOCOLLECTION_NAME_LEN ) != 0,
                        err_object_type );
    }
    else
    {
        // null object인 경우
    }
   
    return stdParsing::stdValue( aQmxMem,
                                 sEWKT,
                                 sEWKTLength,
                                 aBuf,
                                 aFence,
                                 aResult,
                                 aValidateOption,
                                 ID_TRUE,  // enable srid
                                 ST_SRID_INIT );
    
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
