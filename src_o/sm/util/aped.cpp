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
 * $Id:
 **********************************************************************/
#include <smDef.h>

SChar *gFileName;
SInt   gPageID;
UInt   gOffset;
UInt   gBitIdx;
SChar *gEditData;
UInt   gEditLen;
UInt   gEditCnt;
SChar  gEditMode;

void   usage();
IDE_RC writeBit( iduFile *aFile );
IDE_RC writeByte( iduFile *aFile );
IDE_RC writeString( iduFile *aFile );
IDE_RC parseArgs( UInt aArgc, SChar **aArgv );
IDE_RC hex2Int( SChar aHex, UInt *aResult );

/******************************************************************************
 * Description :
 *  DB 파일의 특정 위치를 변경하는 프로그램이다.
 *  Bit, Byte, String 단위로 변경이 가능하다.
 *
 *  aArgc - [IN]  인자의 수
 *  aArgv - [IN]  인자의 포인터 배열
 ******************************************************************************/
int main( SInt aArgc, SChar* aArgv[] )
{
    iduFile sFile;
    UInt    sState = 0;

    IDE_TEST_RAISE( parseArgs( aArgc , aArgv ) != IDE_SUCCESS,
                    invalid_argument );

    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    //fix TASK-3870
    (void)iduLatch::initializeStatic( IDU_CLIENT_TYPE );

    IDE_TEST(iduCond::initializeStatic() != IDE_SUCCESS);

    IDE_TEST( sFile.initialize( IDU_MEM_SM_SDD,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    // fix BUG-25544 [CodeSonar::DoubleClose]
    sState = 1;

    IDE_TEST( sFile.setFileName( gFileName ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sFile.open() != IDE_SUCCESS , err_open_fail );

    sState = 2;

    switch( gEditMode )
    {
        case '1':
            // DB 파일을 bit 단위로 변경
            IDE_TEST( writeBit( &sFile ) != IDE_SUCCESS );
            break;

        case '8':
            // DB 파일을 byte 단위로 변경
            IDE_TEST( writeByte( &sFile ) != IDE_SUCCESS );
            break;

        case 's':
            // DB 파일을 string 단위로 변경
            IDE_TEST( writeString( &sFile ) != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( invalid_argument );
            break;
    }

    // fix BUG-25544 [CodeSonar::DoubleClose]
    sState = 1;
    IDE_TEST_RAISE( sFile.close()!= IDE_SUCCESS, err_close_fail );
    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS);
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_argument );
    {
        usage();
    }
    IDE_EXCEPTION( err_open_fail );
    {
        idlOS::printf( "file open error\n" );
    }
    IDE_EXCEPTION( err_close_fail );
    {
        idlOS::printf( "file close error\n" );
    }
    IDE_EXCEPTION_END;

    // fix BUG-25544 [CodeSonar::DoubleClose]
    switch( sState )
    {
        case 2:
            IDE_ASSERT( sFile.close()   == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );
        default :
            break;
    }

    return IDE_FAILURE;
}


/******************************************************************************
 * Description :
 *  프로그램이 넘겨 받은 인자를 파싱해서 전역 변수에 지정한다.
 *
 *  aArgc - [IN]  인자의 수
 *  aArgv - [IN]  인자의 포인터 배열
 ******************************************************************************/
IDE_RC parseArgs( UInt aArgc, SChar **aArgv )
{
    SInt sOpr;

    sOpr = idlOS::getopt( aArgc, aArgv, "m:o:p:f:i:c:d:" );

    // parseArgs의 호출부에서 에러를 찍는다.
    // 여기서 설정은 불필요.
    IDE_TEST( sOpr == EOF );

    gEditCnt  = 1;
    gFileName = NULL;
    gPageID   = 0;
    gOffset   = 0;
    gBitIdx   = 0;
    gEditData = NULL;
    gEditLen  = 0;
    gEditMode = 0;

    do
    {
        switch( sOpr )
        {
            case 'm':
                // m - edit mode 1 : bit , 8 : byte , s : string
                gEditMode = optarg[0];
                break;

            case 'f':
                // f - 변경할 대상 파일의 file name
                gFileName = optarg;
                break;

            case 'p':
                // p - 변경할 DB 파일 내의 page id
                gPageID = idlOS::atoi( optarg );
                break;

            case 'o':
                // o - 변경할 page내의 offset
                gOffset = idlOS::atoi( optarg );
                break;

            case 'd':
                // d - offset에 덮어 쓸 데이터
                //     mode에 따라 bit, byte, string이 올수있다.
                gEditData = optarg;
                gEditLen = idlOS::strlen( gEditData );
                break;

            case 'c':
                // 반복 횟수 (생략가능, default 1)
                gEditCnt = idlOS::atoi( optarg );
                break;

            case 'i':
                // i - 해당 offset의 byte내의 idx다. 0~7의 값을가지며
                //     bit mode에서만 사용한다. (생략가능, default 0)
                gBitIdx = idlOS::atoi( optarg );
                break;

            default:
                IDE_TEST( ID_TRUE );
                break;
        }
    }
    while( ( sOpr = idlOS::getopt( aArgc, aArgv, "m:o:p:f:i:c:d:" ) ) != EOF ) ;

    // 인자값들을 검증한다.
    IDE_TEST_RAISE( gFileName == NULL, invalid_argument_filename );

    IDE_TEST_RAISE( idlOS::access( gFileName, F_OK ) != 0,
                    err_file_notfound );

    IDE_TEST_RAISE( gEditMode == 0, invalid_argument_mode );

    IDE_TEST_RAISE( gPageID < -1, invalid_argument_pageid );

    IDE_TEST_RAISE( gEditData == NULL, invalid_argument_data );

    IDE_TEST_RAISE( gOffset >= SD_PAGE_SIZE, invalid_argument_offset );

    IDE_TEST_RAISE( ( gEditMode == '1' ) && ( gBitIdx  > 7 ),
                    invalid_argument_idx );

    IDE_TEST_RAISE( ( gEditMode == '8' ) && ( ( gEditLen % 2 ) == 1 ),
                    invalid_argument_datalen );

    // 첫번째 page는 file hdr page로 제외해야 한다.
    // 두번째 page(offset 0x2000)가 pid 0 이 된다.
    gOffset += SD_PAGE_SIZE * gPageID + SD_PAGE_SIZE ;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_notfound );
    {
        idlOS::printf( "Invalid argument : file not found\n" );
    }
    IDE_EXCEPTION( invalid_argument_filename );
    {
        idlOS::printf( "Invalid argument : file name\n" );
    }
    IDE_EXCEPTION( invalid_argument_mode );
    {
        idlOS::printf( "Invalid argument : edit mode\n" );
    }
    IDE_EXCEPTION( invalid_argument_pageid );
    {
        idlOS::printf( "Invalid argument : page id\n" );
    }
    IDE_EXCEPTION( invalid_argument_data );
    {
        idlOS::printf( "Invalid argument : edit data\n" );
    }
    IDE_EXCEPTION( invalid_argument_offset );
    {
        idlOS::printf( "Invalid argument : offset in page(%u) >= (%d)\n",
                       gOffset, SD_PAGE_SIZE );
    }
    IDE_EXCEPTION( invalid_argument_idx );
    {
        idlOS::printf( "Invalid argument : bit_idx(%u) > 7 \n", gBitIdx );
    }
    IDE_EXCEPTION( invalid_argument_datalen );
    {
        idlOS::printf( "Invalid argument : len(%u) of data(%s) is odd"
                       " in byte mode\n", gEditLen, gEditData);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/******************************************************************************
 * Description :
 *  프로그램의 사용법을 사용자에게 출력한다.
 ******************************************************************************/
void usage()
{
    idlOS::printf("\n%-6s: aped {-m mode} {-f file} {-p pid} {-o offset} {-d data}"
                  " [-c edit_cnt] [-i bit_idx]\n", "Usage" );
    idlOS::printf("\n" );
    idlOS::printf(" %-4s : %s\n", "-m",
                  "edit mode : 1 (bit), 8 (byte), s (string)" );
    idlOS::printf(" %-4s : %s\n", "-f", "specify file name" );
    idlOS::printf(" %-4s : %s\n", "-p", "specify page id,"
                  " if you want to edit file hdr : -p -1");
    idlOS::printf(" %-4s : %s\n", "-o", "specify offset" );
    idlOS::printf(" %-4s : %s\n", "-d", "edit data" );
    idlOS::printf(" %-4s : %s\n", "-c", "edit cnt" );
    idlOS::printf(" %-4s : %s\n", "-i", "specify bit index in bit mode" );
    idlOS::printf("\n" );
}

/******************************************************************************
 * Description :
 *  bit 단위로 offset이 가리키는 곳의 데이터를 변경한다.
 *
 *  aFp - [IN]  변경할 DB 파일의 파일 포인터
 ******************************************************************************/
IDE_RC writeBit( iduFile * sFile )
{
    UInt   i,j;
    SChar  sTempBuf;
    UInt   sEditBit;

    IDE_ASSERT( sFile != NULL );

    IDE_TEST_RAISE( sFile->read( NULL, /*idvSQL*/
                                 gOffset,
                                 &sTempBuf,
                                 ID_SIZEOF( SChar ) )
                    != IDE_SUCCESS, err_write_fail );

    // gEditCnt - gEditData를 gEditCnt 수 만큼 반복해서 반영한다.
    for( i = 0 ; i < gEditCnt ; i++ )
    {
        // gEditLen - gEditData의 길이
        for( j = 0 ; j < gEditLen ; j++ )
        {
            if( gEditData[j] == '0' )
            {
                // set 0
                sEditBit = 1;
                sEditBit = ~( sEditBit << ( 7 - gBitIdx ) );
                sTempBuf &= sEditBit ;
            }
            else
            {
                if( gEditData[j] == '1' )
                {
                    // set 1
                    sEditBit = 1;
                    sEditBit <<= ( 7 - gBitIdx );
                    sTempBuf |= sEditBit ;
                }
                else
                {
                    IDE_RAISE( invalid_argument_bit );
                }
            }

            if( ( gBitIdx % 8 ) == 7 )
            {
                // 한 Byte를 넘어갈 때 마다 DB파일에 반영한다.

                IDE_TEST_RAISE( sFile->write( NULL, /*idvSQL*/
                                              gOffset,
                                              &sTempBuf,
                                              ID_SIZEOF( SChar ) )
                                != IDE_SUCCESS, err_write_fail );
                gOffset++;

                // 자동으로 다음 offset으로 넘어가므로 seek 할 필요는 없다.
                IDE_TEST_RAISE( sFile->read( NULL, /*idvSQL*/
                                             gOffset,
                                             &sTempBuf,
                                             ID_SIZEOF( SChar ) )
                                != IDE_SUCCESS, err_read_fail );

                gBitIdx = 0;
            }
            else
            {
                gBitIdx++;
            }
        }
    }

    if( ( gBitIdx % 8 ) != 0 )
    {
         // 남은 Data를 DB파일에 반영한다.

        IDE_TEST_RAISE( sFile->write( NULL, /*idvSQL*/
                                      gOffset,
                                      &sTempBuf,
                                      ID_SIZEOF( SChar ) )
                        != IDE_SUCCESS, err_write_fail );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_argument_bit );
    {
        idlOS::printf( "Invalid argument : data(%s) is not bi in bit mode\n",
                       gEditData );
    }
    IDE_EXCEPTION( err_read_fail );
    {
        idlOS::printf( "file read error\n" );
    }
    IDE_EXCEPTION( err_write_fail );
    {
        idlOS::printf("file write error\n" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/******************************************************************************
 * Description :
 *  byte 단위로 offset이 가리키는 곳의 데이터를 변경한다.
 *
 *  aFp - [IN]  변경할 DB 파일의 파일 포인터
 ******************************************************************************/
IDE_RC writeByte( iduFile * sFile )
{
    UInt    i, j;
    UInt    sConvByte[2] = {0,};
    SChar   sEditByte;

    IDE_ASSERT( sFile != NULL );

    for( i = 0 ; i < gEditCnt ; i++ )
    {
        for( j = 2 ; j <= gEditLen ; j += 2 )
        {
            // gEditData에는 0~9,A~F 까지의 값이 들어있다.
            // 두개씩 묶어 byte값으로 변경한다.

            IDE_TEST( hex2Int( gEditData[j-2],
                               &sConvByte[0] )
                      != IDE_SUCCESS);

            IDE_TEST( hex2Int( gEditData[j-1],
                               &sConvByte[1] )
                      != IDE_SUCCESS);

            sEditByte = ( sConvByte[0] << 4 ) + sConvByte[1] ;

            IDE_TEST_RAISE( sFile->write( NULL, /*idvSQL*/
                                          gOffset,
                                          &sEditByte,
                                          ID_SIZEOF( SChar ) )
                            != IDE_SUCCESS, err_write_fail );
            gOffset++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_write_fail );
    {
        idlOS::printf( "file write error\n" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  Hex 문자를 넘겨받아 Int값으로 변경한다.
 *
 *  aHex    - [IN]  0~9,A~F의 변경 char
 *  aResult - [OUT] aHex를 Int로 변경한 값
 ******************************************************************************/
IDE_RC hex2Int( SChar  aHex ,
                UInt * aResult )
{
    if( ( aHex >= '0' ) && ( aHex <= '9' ) )
    {
        *aResult = aHex - '0';
    }
    else
    {
        if( ( aHex >= 'A' ) && ( aHex <= 'F' ) )
        {
            *aResult = aHex - 'A' + 10;
        }
        else
        {
            if( ( aHex >= 'a' ) && ( aHex <= 'f' ) )
            {
                *aResult = aHex - 'a' + 10;
            }
            else
            {
                IDE_RAISE( invalid_argument_data );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_argument_data );
    {
        idlOS::printf( "Invalid argument : data(%s) is not Hex in byte mode\n",
                       gEditData );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  string 단위로 offset이 가리키는 곳의 데이터를 변경한다.
 *
 *  aFp - [IN]  변경할 DB 파일의 파일 포인터
 ******************************************************************************/

IDE_RC writeString( iduFile * sFile )
{
    UInt i;

    IDE_ASSERT( sFile != NULL );

    for( i = 0 ; i < gEditCnt ; i++ )
    {
        IDE_TEST_RAISE( sFile->write( NULL, /*idvSQL*/
                                      gOffset,
                                      gEditData,
                                      (size_t)gEditLen * ID_SIZEOF( SChar ) )
                        != IDE_SUCCESS, err_write_fail );
        gOffset += gEditLen * ID_SIZEOF( SChar );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_write_fail );
    {
        idlOS::printf( "file write error\n" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

