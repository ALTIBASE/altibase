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
#include <smnIndexFile.h>
#include <smu.h>

SChar  *gFileName;
idBool  gEndian=ID_FALSE;

void   usage();
IDE_RC parseArgs( UInt aArgc, SChar **aArgv );
IDE_RC loadProperty();

/******************************************************************************
 * Description :
 *  Index persistent File을 읽는 Utility
 *
 *  aArgc - [IN]  인자의 수
 *  aArgv - [IN]  인자의 포인터 배열
 ******************************************************************************/
int main( SInt aArgc, SChar* aArgv[] )
{
    smnIndexFile sIndexFile;

    IDE_TEST_RAISE( parseArgs( aArgc , aArgv ) != IDE_SUCCESS,
                    invalid_argument );

    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );

    IDE_TEST( idp::initialize(NULL, NULL) != IDE_SUCCESS );

    // Property Load
    IDE_TEST_RAISE( loadProperty() != IDE_SUCCESS, property_error );

    sIndexFile.dump( gFileName, gEndian );

    IDE_ASSERT( idp::destroy() == IDE_SUCCESS );

    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(property_error);
    {
        smuUtility::outputMsg( "\nproperty error\n\n" );
    }
    IDE_EXCEPTION( invalid_argument );
    {
        usage();
    }
    IDE_EXCEPTION_END;

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

    sOpr = idlOS::getopt( aArgc, aArgv, "f:e" );

    // parseArgs의 호출부에서 에러를 찍는다.
    // 여기서 설정은 불필요.
    IDE_TEST( sOpr == EOF );

    gFileName = NULL;

    do
    {
        switch( sOpr )
        {
            case 'f':
                // f - 변경할 대상 파일의 file name
                gFileName = optarg;
                break;
            case 'e':
                gEndian = ID_TRUE;
                break;
            default:
                IDE_TEST( ID_TRUE );
                break;
        }
    }
    while( ( sOpr = idlOS::getopt( aArgc, aArgv, "f:e" ) ) != EOF ) ;

    // 인자값들을 검증한다.
    IDE_TEST_RAISE( gFileName == NULL, invalid_argument_filename );

    IDE_TEST_RAISE( idlOS::access( gFileName, F_OK ) != 0,
                    err_file_notfound );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_notfound );
    {
        idlOS::printf( "Invalid argument : file not found\n" );
    }
    IDE_EXCEPTION( invalid_argument_filename );
    {
        idlOS::printf( "Invalid argument : file name\n" );
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
    idlOS::printf("\n%-6s: dumpidxfile {-f file} [-e]\n",
                  "Usage" );
    idlOS::printf("\n" );
    idlOS::printf(" %-4s : %s\n", "-f", "specify file name" );
    idlOS::printf(" %-4s : %s\n", "-e", "change endian" );
    idlOS::printf("\n" );
}

IDE_RC loadProperty()
{
    IDE_TEST(iduProperty::load() != IDE_SUCCESS);

    IDE_TEST( smuProperty::load()
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


