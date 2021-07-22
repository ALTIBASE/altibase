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
 * $Id: dumpxlf.cpp$
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smr.h>
#include <smxDef.h>
#include <rpDef.h>
#include <rpnComm.h>
#include <rpdQueue.h>
#include <rpxReceiver.h>

UInt         gStartXLogFileNumber = 0;
idBool       gStartSet = ID_FALSE;
UInt         gEndXLogFileNumber = 0;
idBool       gEndSet = ID_FALSE;
SChar      * gRepName         = NULL;
smTID        gTID             = 0;        // 특정 TID만 출력할것인가?
idBool       gDisplayValue    = ID_TRUE;  // Log의 Value부분을 출력할것인가?
idBool       gDisplayLogTypes = ID_FALSE; // LogType들을 출력한다.
idBool       gDisplayHeader   = ID_TRUE;
idBool       gInvalidArgs     = ID_FALSE;
idBool       gPathSet         = ID_FALSE; // 특정 경로가 설정되어 있는가.
SChar        gPath[SM_MAX_FILE_NAME+1] = {"", };

void   usage();
IDE_RC initProperty();
void   parseArgs( int    &aArgc,
                  char **&aArgv );
void   showCopyRight(void);       // Copy right 출력한다.

IDE_RC dumpXLog(SChar * aReplName);
IDE_RC readXLog(rpdXLogfileMgr * aXLogfileManager);
IDE_RC printXLog(rpdXLog * aXLog );

void usage()
{
    idlOS::printf( "\n%-6s: dumpxlf -r replication_name {-s start_xlog_file_number}\n"/*" {-e end_xlog_file_number} \n"*/, "Usage" );

    idlOS::printf( " %-4s : %s\n", "-r", "specify replication name" );
    //idlOS::printf( " %-4s : %s\n", "-s", "specify start xlog file number" );
    //idlOS::printf( " %-4s : %s\n", "-e", "specify end xlog file number" );
    idlOS::printf( "\n" );
}

static IDE_RC mainLoadErrorMsb(SChar *root_dir, SChar */*idn*/)
{
    SChar filename[512];

    idlOS::memset(filename, 0, 512);
    idlOS::sprintf(filename, "%s%cmsg%cE_ID_%s.msb", root_dir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, "US7ASCII");
    IDE_TEST_RAISE(ideRegistErrorMsb(filename) != IDE_SUCCESS,
                   load_msb_error);

    idlOS::memset(filename, 0, 512);
    idlOS::sprintf(filename, "%s%cmsg%cE_SM_%s.msb", root_dir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, "US7ASCII");
    IDE_TEST_RAISE(ideRegistErrorMsb(filename) != IDE_SUCCESS,
                   load_msb_error);

    idlOS::memset(filename, 0, 512);
    idlOS::sprintf(filename, "%s%cmsg%cE_RP_%s.msb", root_dir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, "US7ASCII");
    IDE_TEST_RAISE(ideRegistErrorMsb(filename) != IDE_SUCCESS,
                   load_msb_error);

    idlOS::memset(filename, 0, 512);
    idlOS::sprintf(filename, "%s%cmsg%cE_CM_%s.msb", root_dir, IDL_FILE_SEPARATOR, IDL_FILE_SEPARATOR, "US7ASCII");
    IDE_TEST_RAISE(ideRegistErrorMsb(filename) != IDE_SUCCESS,
                   load_msb_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(load_msb_error);
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC initProperty()
{
    IDE_TEST_RAISE( idp::initialize(NULL, NULL) != IDE_SUCCESS,
                    err_load_property );
    IDE_TEST_RAISE( iduProperty::load() != IDE_SUCCESS,
                    err_load_property );
    rpuProperty::load();
    IDE_TEST (mainLoadErrorMsb(idp::getHomeDir(),
                               (SChar *)"US7ASCII") != IDE_SUCCESS);
    return IDE_SUCCESS;
    IDE_EXCEPTION( err_load_property );
    {
        idlOS::printf( "\nFailed to loading LOG_DIR.\n" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void parseArgs( int &aArgc, char **&aArgv )
{
    SInt sOpr;

    sOpr = idlOS::getopt( aArgc, aArgv, "F:g:r:s:e:" );

    if(sOpr != EOF)
    {
        do
        {
            switch( sOpr )
            {
            case 'r':
                gRepName     = optarg;
                break;
            case 's':
                gStartXLogFileNumber     = idlOS::atoi( optarg );
                gStartSet = ID_TRUE;
                //gInvalidArgs     = ID_TRUE;
                break;
            case 'e':
                gEndXLogFileNumber     = idlOS::atoi( optarg );
                gEndSet = ID_TRUE;
                gInvalidArgs     = ID_TRUE;
                break;
            case 'F':
                idlOS::strcpy(gPath, optarg);
                gPathSet = ID_TRUE;
                gInvalidArgs     = ID_TRUE;
                break;
            default:
                gInvalidArgs     = ID_TRUE;
                break;
            }
        }
        while( (sOpr = idlOS::getopt(aArgc, aArgv, "F:g:r:s:e:")) != EOF );
    }
    else
    {
        gInvalidArgs = ID_TRUE;
    }
}

/******************************************************************
// atoi의 long형 버전
 *****************************************************************/
ULong atol(SChar *aSrc)
{
    ULong   sRet = 0;

    while ((*aSrc) != '\0')
    {
        sRet = sRet * 10;
        if ( ( (*aSrc) < '0' ) || ( '9' < (*aSrc) ) )
        {
            break;
        }
        else
        {
            // nothing to do...
        }

        sRet += (ULong)( (*aSrc) - '0');
        aSrc ++;
    }

    return sRet;
}

int main( int aArgc, char *aArgv[] )
{
    parseArgs( aArgc, aArgv );

    if( gDisplayHeader == ID_TRUE)
    {
        //showCopyRight();
    }

    IDE_TEST_RAISE( gInvalidArgs == ID_TRUE,
                    err_invalid_arguments );

    IDE_TEST_RAISE( gRepName == NULL,
                    err_invalid_arguments );

    IDE_TEST( initProperty() != IDE_SUCCESS );
    /* Global Memory Manager initialize */
    IDE_TEST( iduMemMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
    IDE_TEST( iduMutexMgr::initializeStatic( IDU_CLIENT_TYPE ) != IDE_SUCCESS );
    (void)iduLatch::initializeStatic(IDU_CLIENT_TYPE);
    IDE_TEST( iduCond::initializeStatic() != IDE_SUCCESS);

    IDE_TEST(dumpXLog(gRepName) != IDE_SUCCESS);
    idlOS::printf( "\nDump complete.\n" );


    IDE_ASSERT( idp::destroy() == IDE_SUCCESS );

    IDE_ASSERT( iduCond::destroyStatic() == IDE_SUCCESS);
    (void)iduLatch::destroyStatic();
    IDE_ASSERT( iduMutexMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( iduMemMgr::destroyStatic() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_arguments );
    {
        (void)usage();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void showCopyRight( void )
{
    SChar         sBuf[1024 + 1];
    SChar         sBannerFile[1024];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
    const SChar * sBanner = "DUMPLF.ban";

    sAltiHome = idlOS::getenv( IDP_HOME_ENV );
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home );

    // make full path banner file name
    idlOS::memset(   sBannerFile, 0, ID_SIZEOF( sBannerFile ) );
    idlOS::snprintf( sBannerFile,
                     ID_SIZEOF( sBannerFile ),
                     "%s%c%s%c%s",
                     sAltiHome,
                     IDL_FILE_SEPARATOR,
                     "msg",
                     IDL_FILE_SEPARATOR,
                     sBanner );

    sFP = idlOS::fopen( sBannerFile, "r" );
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    sCount = idlOS::fread( (void*) sBuf, 1, 1024, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf( "%s", sBuf );
    idlOS::fflush( stdout );

    idlOS::fclose( sFP );

    IDE_EXCEPTION( err_altibase_home );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_open );
    {
        // nothing to do
        // ignore error in ShowCopyright
    }
    IDE_EXCEPTION( err_file_read );
    {
        idlOS::fclose( sFP );
    }
    IDE_EXCEPTION_END;
}

IDE_RC dumpXLog(SChar * aReplName)
{
    rpdXLogfileMgr * sXLogfileManager = NULL;
    UInt sStage = 0;
    rpXLogLSN sXLogLSN = RP_XLOGLSN_INIT;
    rpXLogLSN sLastXLogLSN = RP_XLOGLSN_INIT;
    smLSN sReadLSN;
    rpXLogLSN sCurrentReadLSN;
    UInt    sFirstFileNo = 0;
    UInt    sLastFileNo = 0;
    idBool  sDummyExitFlag = ID_FALSE;
    rpdXLogfileHeader sHeader;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_RECEIVER,
                                       ID_SIZEOF( rpdXLogfileMgr),
                                       (void **)&sXLogfileManager,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMALLOC );
    sStage = 1;

    IDE_TEST(rpdXLogfileMgr::findFirstAndLastFileNo( aReplName, &sFirstFileNo, &sLastFileNo) != IDE_SUCCESS );
    sXLogLSN = RP_SET_XLOGLSN( sFirstFileNo, XLOGFILE_HEADER_SIZE_INIT );

    if ( gStartSet == ID_TRUE )
    {
        IDE_TEST_RAISE( gStartXLogFileNumber < sFirstFileNo, ERR_START_FILENO );
        sFirstFileNo = gStartXLogFileNumber;
        IDE_TEST(rpdXLogfileMgr::openAndReadHeader( aReplName,
                                                    RPU_REPLICATION_XLOGFILE_DIR_PATH,
                                                    gStartXLogFileNumber,
                                                    &sHeader ) != IDE_SUCCESS);
        sXLogLSN = RP_SET_XLOGLSN( gStartXLogFileNumber, sHeader.mFileFirstXLogOffset );
    }
    if ( gEndSet == ID_TRUE )
    {
        IDE_TEST_RAISE( gEndXLogFileNumber > sLastFileNo, ERR_END_FILENO );
        sLastFileNo = gEndXLogFileNumber;
        sLastXLogLSN = RP_SET_XLOGLSN( sLastFileNo, XLOGFILE_HEADER_SIZE_INIT );
    }

    IDE_TEST( sXLogfileManager->initialize( aReplName,
                                            sXLogLSN,
                                            1, // RPU_REPLICATION_RECEIVE_TIMEOUT
                                            &sDummyExitFlag,
                                            NULL,
                                            sLastXLogLSN)
              != IDE_SUCCESS );
    sStage = 2;

    sXLogfileManager->getReadInfo( &sCurrentReadLSN );
    RP_GET_XLOGLSN( sReadLSN.mFileNo, sReadLSN.mOffset, sCurrentReadLSN );
    idlOS::printf("file read start: fileno(%"ID_UINT32_FMT"), offset(%"ID_UINT32_FMT")\n",
                  sReadLSN.mFileNo, sReadLSN.mOffset);
    while ( sXLogfileManager->checkRemainXLog() == IDE_SUCCESS )
    {
        sXLogfileManager->getReadInfo( &sCurrentReadLSN );
            RP_GET_XLOGLSN( sReadLSN.mFileNo, sReadLSN.mOffset, sCurrentReadLSN );
            idlOS::printf("fileno(%"ID_UINT32_FMT"), offset(%"ID_UINT32_FMT"): ",
                          sReadLSN.mFileNo, sReadLSN.mOffset);
        if ( readXLog(sXLogfileManager) != IDE_SUCCESS)
        {
            break;
        }
        idlOS::printf("\n");
    }
    sXLogfileManager->getReadInfo( &sCurrentReadLSN );
    RP_GET_XLOGLSN( sReadLSN.mFileNo, sReadLSN.mOffset, sCurrentReadLSN );
    idlOS::printf("file read end: fileno(%"ID_UINT32_FMT"), offset(%"ID_UINT32_FMT")\n",
                  sReadLSN.mFileNo, sReadLSN.mOffset);
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMALLOC )
    {
        idlOS::printf("malloc error");
    }
    IDE_EXCEPTION( ERR_START_FILENO )
    {
        idlOS::printf("start file no error: user(%"ID_UINT32_FMT") exist(%"ID_UINT32_FMT")",
                      gStartXLogFileNumber, sFirstFileNo);
    }
    IDE_EXCEPTION( ERR_END_FILENO )
    {
        idlOS::printf("end file no error: user(%"ID_UINT32_FMT") exist(%"ID_UINT32_FMT")",
                      gEndXLogFileNumber, sLastFileNo);
    }

    IDE_EXCEPTION_END;
    idlOS::printf("dumpXLog error: %s \n", ideGetErrorMsg());
    switch( sStage )
    {
        case 2 :
            sXLogfileManager->finalize();
        case 1 :
            (void)iduMemMgr::free( sXLogfileManager );
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC readXLog(rpdXLogfileMgr * aXLogfileManager)
{
    rpxReceiverReadContext sReadCtx;
    rpdXLog   sXLog;
    idBool sDummyExitFlag = ID_FALSE;
    rpdMeta sDummyMeta;

    sDummyMeta.initialize();

    IDE_TEST( rpdQueue::initializeXLog( &sXLog,
                                        10240,
                                        ID_FALSE,
                                        NULL )
              != IDE_SUCCESS );
    sReadCtx.mXLogfileContext = aXLogfileManager;
    sReadCtx.mCurrentMode = RPX_RECEIVER_READ_XLOGFILE;
    (void) rpnComm::recvXLogA7(NULL,            /* allocator */
                               sReadCtx,        /* protocol or read context */
                               &sDummyExitFlag, /* aExitFlag */
                               &sDummyMeta,     /* replication meta, this case dummy meta */
                               &sXLog,          /* XLog */
                               (ULong)1 );      /* timeout sec */
    printXLog(&sXLog);

    rpdQueue::destroyXLog( &sXLog, NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    idlOS::printf("error: %s", ideGetErrorMsg());
    return IDE_FAILURE;
}

IDE_RC printXLog(rpdXLog * aXLog )
{
    FILE * sFP = stdout;
    rpdQueue::printXLog(sFP, aXLog);

    return IDE_SUCCESS;
}
