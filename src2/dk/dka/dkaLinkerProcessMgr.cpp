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
 * $id$
 **********************************************************************/

#include <dkaLinkerProcessMgr.h>

idBool              dkaLinkerProcessMgr::mIsPropertiesLoaded;
SChar               dkaLinkerProcessMgr::mServerInfoStr[DK_MSG_LEN];
UInt                dkaLinkerProcessMgr::mDBCharSet;
dkaLinkerProcInfo   dkaLinkerProcessMgr::mLinkerProcessInfo;
iduMutex            dkaLinkerProcessMgr::mDkaMutex;
iduMutex            dkaLinkerProcessMgr::mConfMutex;
dkcDblinkConf *     dkaLinkerProcessMgr::mConf;
UInt                dkaLinkerProcessMgr::mAltilinkerRestartNumber;

/************************************************************************
 * Description : AltiLinker process manager component 를 초기화해준다. 
 *              
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::initializeStatic()
{
    UInt             sServerInfoStrLen = 0;
    SChar           *sServerInfoStr    = NULL;
    const SChar     *sCopyRightStr;
    const SChar     *sPackageInfoStr;
    const SChar     *sProductionTimeStr;
    const SChar     *sVersionStr;

    mIsPropertiesLoaded = ID_FALSE;

    idlOS::memset( mServerInfoStr, 0, ID_SIZEOF(mServerInfoStr) );

    mDBCharSet = MTL_UTF16_ID;

    mLinkerProcessInfo.mStatus               = DKA_LINKER_STATUS_NON;
    mLinkerProcessInfo.mLinkerSessionCnt     = 0;
    mLinkerProcessInfo.mRemoteNodeSessionCnt = 0;
    mLinkerProcessInfo.mJvmMemMaxSize        = 0;
    mLinkerProcessInfo.mJvmMemUsage          = 0;

    mConf = NULL;

    IDE_TEST_RAISE( mDkaMutex.initialize( (SChar *)"DKA_LINKER_PROC_MGR_MUTEX",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    IDE_TEST_RAISE( mConfMutex.initialize( (SChar *)"DKA_LINKER_PROC_MGR_CONF_MUTEX",
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    
    sCopyRightStr      = iduGetCopyRightString();
    sPackageInfoStr    = iduGetPackageInfoString();
    sProductionTimeStr = iduGetProductionTimeString();
    sVersionStr        = iduGetVersionString();

    sServerInfoStrLen += idlOS::strlen( sCopyRightStr );
    sServerInfoStrLen += idlOS::strlen( sPackageInfoStr );
    sServerInfoStrLen += idlOS::strlen( sProductionTimeStr );
    sServerInfoStrLen += idlOS::strlen( sVersionStr );

    (void)iduMemMgr::calloc( IDU_MEM_DK, 
                             1, 
                             sServerInfoStrLen + 4, 
                             (void **)&sServerInfoStr, 
                             IDU_MEM_FORCE ); 

    idlOS::snprintf( sServerInfoStr,
                     sServerInfoStrLen,
                     PDL_TEXT( "%s %s %s %s" ),
                     sCopyRightStr, 
                     sPackageInfoStr,
                     sProductionTimeStr, 
                     sVersionStr );

    if ( sServerInfoStrLen > ( ID_SIZEOF( mServerInfoStr ) - 1 ) )
    {
        sServerInfoStrLen = ID_SIZEOF( mServerInfoStr ) - 1;
    }
    else
    {
        /* use original size */
    }

    idlOS::strncpy( mServerInfoStr, sServerInfoStr, sServerInfoStrLen );

    mServerInfoStr[sServerInfoStrLen]='\0';

    (void)iduMemMgr::free( sServerInfoStr );

    mDBCharSet = mtl::mDBCharSet->id;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_SET( ideSetErrorCode( dkERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;
    
    if ( sServerInfoStr != NULL )
    {
        (void)iduMemMgr::free( sServerInfoStr );
        sServerInfoStr = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker process manager component 를 정리해준다. 
 *
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::finalizeStatic()
{
    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    if ( mConf != NULL )
    {
        IDE_TEST( dkcFreeDblinkConf( mConf ) != IDE_SUCCESS );
        mConf = NULL;
    }
    else
    {
        /* do nothing, just finalize linker process manager */
    }
        
    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    (void)mConfMutex.destroy();
    
    (void)mDkaMutex.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스를 start 시킨다. 
 *               여기서는 AltiLinker 프로세스와 관련된 프로퍼티를 load
 *               하고 AltiLinker 프로세스를 띄워주는 것까지 수행한다.
 *
 * Notice      : 최초 초기화를 수행한 후 AltiLinker 프로세스를 운영중에 
 *               AltiLinker 프로세스가 죽거나 사용자에 의해 shutdown 
 *               되는 경우 컴포넌트를 다시 초기화 하지 않고 AltiLinker
 *               프로세스를 기동(startLinkerProcess) 시킬 수 있어야 한다. 
 *               따라서 이 함수 수행중에 AltiLinker process property 들을
 *               (dblink.conf) 다시 load 할 필요가 있다.
 *
 * Parameter   : aMinus1OnlyRetry [0,1] 재시도 여부를 판단
 *                                재시도라면 sArgv 인자를 한칸씩 땡긴다.
 *               aPid (out) fork가 제대로 됐는지 확인하기 위해 pid를 리턴한다.
 *
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::startLinkerProcessInternal( SInt aMinus1onlyRetry , SInt *aPid )
{
#if !defined(VC_WINCE)
 #ifdef DEBUG
    UInt         sArgc = 13 - aMinus1onlyRetry;
 #else
    UInt         sArgc = 10 - aMinus1onlyRetry;
 #endif
    SChar     ** sArgv = NULL;
    SChar        sAEFPCmd[DK_PATH_LEN]; /* AEFP : AltiLinker Execution File Path */
    SChar        sAEFCmd[DK_PATH_LEN];  /* AEF: AltiLinker Execution File */
    SChar        sAEFPath[DK_PATH_LEN]; /* altilinker.jar file path */
    SChar        sXmsOpt[DK_NAME_LEN];  /* jvm memory initial size option */
    SChar        sXmxOpt[DK_NAME_LEN];  /* jvm memory maximum size option */
    SChar        sListenPortOpt[DK_NAME_LEN]; /* listen port option, value */
    SChar        sTrcDirOpt[DK_PATH_LEN]; /* trace log dir option, value */
    SChar        sJvmBitDataModel[DK_NAME_LEN]; /* jvm [32-bit|64-bit] data model option */
#else /* VC_WINCE */
 #ifdef DEBUG
    UInt         sArgc = 13 - aMinus1onlyRetry;
 #else
    UInt         sArgc = 10 - aMinus1onlyRetry;
 #endif
    ASYS_TCHAR **sArgv = NULL;
    ASYS_TCHAR   sAEFPCmd[DK_PATH_LEN];
    ASYS_TCHAR   sAEFCmd[DK_PATH_LEN];
    ASYS_TCHAR   sAEFPath[DK_PATH_LEN];
    ASYS_TCHAR   sXmsOpt[DK_NAME_LEN];
    ASYS_TCHAR   sXmxOpt[DK_NAME_LEN];
    ASYS_TCHAR   sListenPortOpt[DK_NAME_LEN];
    ASYS_TCHAR   sTrcDirOpt[DK_PATH_LEN];
    ASYS_TCHAR   sJvmBitDataModel[DK_NAME_LEN];
#endif 
    idBool       sIsLocked    = ID_FALSE;
    idBool       sIsAllocated = ID_FALSE;
    UInt         sPortNo;
    UInt         sHomeLen;
    UInt         sPathMemSize;
    UInt         sQuestionMark = 0;
    UInt         sJvmMemInitSize;
    UInt         sJvmMemMaxSize;
    SChar       *sUserInputTrcPath = NULL;
    SChar       *sUserInputTrcPathPoint = NULL;
    SChar       *sLinkerLogFilePath = NULL;
    SChar       *sFinalLinkerLogFilePath = NULL;
    SChar       *sHomeDir = NULL;
    SChar       *sJavaHome = NULL;
    pid_t        sChildPid;
    void        *sValue = NULL;
    SInt         i = 0;
    SInt         sJvmBitDataModelValue;
    
    ideLogEntry  sLog(IDE_DK_0); 

    IDE_ASSERT( mDkaMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /* 1. Load linker properties */
    IDE_TEST( loadLinkerProperties() != IDE_SUCCESS );

    sJavaHome          = idlOS::getenv( DKA_CMD_STR_JAVA_HOME );
    IDE_TEST_RAISE( sJavaHome == NULL, JAVA_HOME_ERR );

    sHomeDir           = idlOS::getenv( IDP_HOME_ENV );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    sPortNo            = mConf->mAltilinkerPortNo;
    sJvmMemInitSize    = mConf->mAltilinkerJvmMemoryPoolInitSize;
    sJvmMemMaxSize     = mConf->mAltilinkerJvmMemoryPoolMaxSize;
    sUserInputTrcPath  = mConf->mAltilinkerTraceLogDir;
    sJvmBitDataModelValue = mConf->mAltilinkerJvmBitDataModelValue;
    
    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
        
    sUserInputTrcPathPoint = sUserInputTrcPath;
    
    /* 2. Make an AltiLinker trace log directory path */
    for ( ; *sUserInputTrcPathPoint != 0; sUserInputTrcPathPoint++ )
    {
        if ( *sUserInputTrcPathPoint == '?' )
        {
            sQuestionMark++;
        }
        else
        {
            /* use absolute path */
        }
    }

    if ( sQuestionMark > 0 )
    {
        sHomeLen = idlOS::strlen( sHomeDir );

        sPathMemSize = idlOS::strlen( sUserInputTrcPath ) 
                     + ( ( sHomeLen + 1 ) * sQuestionMark ) 
                     + 1;

        sValue = iduMemMgr::mallocRaw( sPathMemSize, IDU_MEM_FORCE );
        sIsAllocated = ID_TRUE;

        IDE_ASSERT( sValue != NULL );

        idlOS::memset( sValue, 0, sPathMemSize );

        sLinkerLogFilePath = (SChar *)sValue;

        for ( ; *sUserInputTrcPath != 0; sUserInputTrcPath++ )
        {
            if ( *sUserInputTrcPath == '?' )
            {
                idlOS::strncpy( sLinkerLogFilePath, sHomeDir, sHomeLen );
                sLinkerLogFilePath += sHomeLen;
            }
            else
            {
                *sLinkerLogFilePath++ = *sUserInputTrcPath;
            }
        }

        sFinalLinkerLogFilePath = (SChar*)sValue;
    }
    else
    {
        sValue = iduMemMgr::mallocRaw( idlOS::strlen( sUserInputTrcPath ) + 1,
                                       IDU_MEM_FORCE );
        sIsAllocated = ID_TRUE;

        IDE_ASSERT( sValue != NULL );

        idlOS::strcpy( (SChar *)sValue, sUserInputTrcPath );

        sFinalLinkerLogFilePath = (SChar *)sValue;
    }

#ifdef DEBUG

#define DKA_LINKER_XDEBUG_STR_1     "-Xrunjdwp:transport=dt_socket,address=" 
#define DKA_LINKER_XDEBUG_STR_2     ",server=y,suspend=n"

 #if !defined(VC_WINCE)
    SChar       sLinkerXDebugStr[128];
 #else
    ASYS_TCHAR  sLinkerXDebugStr[128];
 #endif

    UInt        sLinkerDebugPortNo;

    sLinkerDebugPortNo = sPortNo + 5000;

    idlOS::snprintf( sLinkerXDebugStr, 
                     ID_SIZEOF( sLinkerXDebugStr ),
                     PDL_TEXT( DKA_LINKER_XDEBUG_STR_1
                               "%"ID_INT32_FMT
                               DKA_LINKER_XDEBUG_STR_2 ),
                     sLinkerDebugPortNo );
#endif

    /* 3. Fill argument vector array */
    idlOS::snprintf( sXmsOpt, 
                     ID_SIZEOF( sXmsOpt ),
                     PDL_TEXT( DKA_CMD_STR_XMS 
                               "%"ID_INT32_FMT
                               DKA_CMD_STR_MB ),
                     sJvmMemInitSize );

    idlOS::snprintf( sXmxOpt, 
                     ID_SIZEOF( sXmxOpt ),
                     PDL_TEXT( DKA_CMD_STR_XMX 
                               "%"ID_INT32_FMT
                               DKA_CMD_STR_MB ),
                     sJvmMemMaxSize );

    idlOS::snprintf( sListenPortOpt, 
                     ID_SIZEOF( sListenPortOpt ),
                     PDL_TEXT( DKA_CMD_STR_LISTEN_PORT
                               DKA_CMD_STR_EQUAL
                               "%"ID_INT32_FMT ),
                     sPortNo );

#ifdef ALTI_CFG_OS_WINDOWS
    /*
     * BUG-41880
     *      When file path contains space character(' '),
     *      idlOS::fork_exec_close() doesn't work correctly at Windows OS.
     */
    idlOS::snprintf( sTrcDirOpt, 
                     ID_SIZEOF( sTrcDirOpt ),
                     PDL_TEXT( DKA_CMD_STR_TRC_DIR
                               DKA_CMD_STR_EQUAL
                               "\"%s\"" ),
                     sFinalLinkerLogFilePath );
#else
    idlOS::snprintf( sTrcDirOpt, 
                     ID_SIZEOF( sTrcDirOpt ),
                     PDL_TEXT( DKA_CMD_STR_TRC_DIR
                               DKA_CMD_STR_EQUAL
                               "%s" ),
                     sFinalLinkerLogFilePath );
#endif
    
    sIsAllocated = ID_FALSE;
    (void)iduMemMgr::freeRaw( sFinalLinkerLogFilePath );

    idlOS::snprintf( sAEFPCmd, 
                     ID_SIZEOF( sAEFPCmd ),
                     PDL_TEXT( "%s%cbin%c"
                               DKA_CMD_STR_JAVA
                               PDL_PLATFORM_EXE_SUFFIX_A ),
                     sJavaHome, 
                     IDL_FILE_SEPARATOR,
                     IDL_FILE_SEPARATOR );

    idlOS::snprintf( sAEFCmd, 
                     ID_SIZEOF( sAEFCmd ),
                     PDL_TEXT( "%s%cbin%c"
                               DKA_CMD_STR_JAVA ),
                     sJavaHome, 
                     IDL_FILE_SEPARATOR, 
                     IDL_FILE_SEPARATOR );

    idlOS::snprintf( sAEFPath, 
                     ID_SIZEOF( sAEFPath ),
                     PDL_TEXT( "%s%cbin%c"
                               DKA_CMD_STR_FILE_NAME ),
                     sHomeDir, 
                     IDL_FILE_SEPARATOR, 
                     IDL_FILE_SEPARATOR );

    /* 4. Check java execution file */
    IDE_TEST_RAISE( idlOS::access( sAEFPCmd, X_OK ) == -1,
                    ERR_ACCESS_JAVA_EXEC_FILE );

    IDE_TEST_RAISE( idlOS::access( sAEFPath, R_OK ) == -1,
                    ERR_ACCESS_LINKER_EXEC_FILE );


#ifdef ALTI_CFG_OS_WINDOWS
    /*
     * BUG-41880
     *      When file path contains space character(' '),
     *      idlOS::fork_exec_close() doesn't work correctly at Windows OS.
     */    
    idlOS::snprintf( sAEFPath, 
                     ID_SIZEOF( sAEFPath ),
                     PDL_TEXT( "\"%s%cbin%c"
                               DKA_CMD_STR_FILE_NAME "\"" ),
                     sHomeDir, 
                     IDL_FILE_SEPARATOR, 
                     IDL_FILE_SEPARATOR );
#endif

    if ( sJvmBitDataModelValue == 0 )
    {
        idlOS::snprintf( sJvmBitDataModel, 
                         ID_SIZEOF( sJvmBitDataModel ),
                         PDL_TEXT( DKA_CMD_STR_32BIT ) );
    }
    else
    {
        idlOS::snprintf( sJvmBitDataModel, 
                         ID_SIZEOF( sJvmBitDataModel ),
                         PDL_TEXT( DKA_CMD_STR_64BIT ) );
    }
    
    sArgv = (SChar**)idlOS::malloc( ID_SIZEOF(SChar*) * sArgc );
    IDE_TEST_RAISE( sArgv == NULL, ERR_MALLOC_ARGV );

    sArgv[0] = sAEFCmd;
#if !defined(VC_WINCE)
 #ifdef DEBUG
    sArgv[1] = (SChar *)PDL_TEXT( "-ea" );         /* for assert */
    sArgv[2] = (SChar *)PDL_TEXT( "-Xdebug" ); 
    sArgv[3] = (SChar *)PDL_TEXT( sLinkerXDebugStr );
    sArgv[4] = (SChar *)PDL_TEXT( sXmsOpt );
    sArgv[5] = (SChar *)PDL_TEXT( sXmxOpt );
    sArgv[6] = (SChar *)PDL_TEXT( sJvmBitDataModel );
    sArgv[7-aMinus1onlyRetry] = (SChar *)PDL_TEXT( DKA_CMD_STR_JAR );
    sArgv[8-aMinus1onlyRetry] = (SChar *)PDL_TEXT( sAEFPath );
    sArgv[9-aMinus1onlyRetry] = (SChar *)PDL_TEXT( sListenPortOpt );
    sArgv[10-aMinus1onlyRetry] = (SChar *)PDL_TEXT( sTrcDirOpt );
    sArgv[11-aMinus1onlyRetry] = (SChar *)PDL_TEXT( DKA_CMD_STR_TRC_FILE_NAME );
 #else
    sArgv[1] = (SChar *)PDL_TEXT( sXmsOpt );
    sArgv[2] = (SChar *)PDL_TEXT( sXmxOpt );
    sArgv[3] = (SChar *)PDL_TEXT( sJvmBitDataModel );        
    sArgv[4-aMinus1onlyRetry] = (SChar *)PDL_TEXT( DKA_CMD_STR_JAR );
    sArgv[5-aMinus1onlyRetry] = (SChar *)PDL_TEXT( sAEFPath );
    sArgv[6-aMinus1onlyRetry] = (SChar *)PDL_TEXT( sListenPortOpt );
    sArgv[7-aMinus1onlyRetry] = (SChar *)PDL_TEXT( sTrcDirOpt );
    sArgv[8-aMinus1onlyRetry] = (SChar *)PDL_TEXT( DKA_CMD_STR_TRC_FILE_NAME );
 #endif
#else /* VC_WINCE */
 #ifdef DEBUG
    sArgv[1] = (SChar *)PDL_TEXT( "-ea" );         /* for assert */
    sArgv[2] = (ASYS_TCHAR *)PDL_TEXT( "-Xdebug");
    sArgv[3] = (ASYS_TCHAR *)PDL_TEXT( sLinkerXDebugStr );
    sArgv[4] = (ASYS_TCHAR *)PDL_TEXT( sXmsOpt );
    sArgv[5] = (ASYS_TCHAR *)PDL_TEXT( sXmxOpt );
    sArgv[6] = (ASYS_TCHAR *)PDL_TEXT( sJvmBitDataModel );        
    sArgv[7-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( DKA_CMD_STR_JAR );
    sArgv[8-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( sAEFPath );
    sArgv[9-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( sListenPortOpt );
    sArgv[10-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( sTrcDirOpt );
    sArgv[11-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( DKA_CMD_STR_TRC_FILE_NAME );
 #else
    sArgv[1] = (ASYS_TCHAR *)PDL_TEXT( sXmsOpt );
    sArgv[2] = (ASYS_TCHAR *)PDL_TEXT( sXmxOpt );
    sArgv[3] = (ASYS_TCHAR *)PDL_TEXT( sJvmBitDataModel );        
    sArgv[4-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( DKA_CMD_STR_JAR ); 
    sArgv[5-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( sAEFPath );
    sArgv[6-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( sListenPortOpt );
    sArgv[7-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( sTrcDirOpt );
    sArgv[8-aMinus1onlyRetry] = (ASYS_TCHAR *)PDL_TEXT( DKA_CMD_STR_TRC_FILE_NAME );
 #endif
#endif /* VC_WINCE */

    sArgv[sArgc-1] = NULL;
    
    sLog.appendFormat( "Altilinker execute command: " );
    for( i = 0; sArgv[i] != NULL; i++ )
    {
        sLog.appendFormat( "%s ", sArgv[i] ); 
    }
    sLog.write();
    /* 5. Fork AltiLinker process with argument vector */
    /* BUG-37600 */
    sChildPid = idlOS::fork_exec_close( sArgv );

    IDE_TEST_RAISE( sChildPid == -1, ERR_FORK_EXEC_LINKER_PROCESS );

    setJvmMemMaxSize( sJvmMemMaxSize );
    
    IDE_ASSERT( mDkaMutex.unlock() == IDE_SUCCESS );
    sIsLocked = ID_FALSE;

    idlOS::free( sArgv );
   
    * aPid = sChildPid;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ACCESS_LINKER_EXEC_FILE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_ACCESS_LINKER_EXEC_FILE ) );
    }
    IDE_EXCEPTION( ERR_FORK_EXEC_LINKER_PROCESS );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_FORK_EXEC_LINKER_PROCESS ) );
    }
    IDE_EXCEPTION( ERR_ACCESS_JAVA_EXEC_FILE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_JAVA_EXEC_ERR,  sAEFPCmd, idlOS::strerror(errno) ) );
    }
    IDE_EXCEPTION( JAVA_HOME_ERR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_JAVA_HOME_ERR ) );
    }
    IDE_EXCEPTION( ERR_MALLOC_ARGV );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        (void)mDkaMutex.unlock();
    }
    else
    {
        /* do nothing */
    }

    if ( sIsAllocated == ID_TRUE )
    {
        (void)iduMemMgr::freeRaw( sFinalLinkerLogFilePath );
    }
    else
    {
        /* do nothing */
    }

    if ( sArgv != NULL )
    {
        idlOS::free( sArgv );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkaLinkerProcessMgr::startLinkerProcess( void )
{
    SInt sPid;
    SInt sAlive;

    IDE_TEST_RAISE( mLinkerProcessInfo.mStatus != DKA_LINKER_STATUS_NON,
                    ERR_LINKER_ALREADY_RUNNING );

    IDE_TEST( startLinkerProcessInternal( 0, &sPid ) != IDE_SUCCESS );
  
    idlOS::sleep( 2 );
    sAlive = PDL::process_active( sPid ) ;
    
    if ( sAlive <= 0 )
    {
        IDE_TEST( startLinkerProcessInternal( 1, &sPid ) != IDE_SUCCESS );

        idlOS::sleep( 2 );
        sAlive = PDL::process_active( sPid ) ;

        IDE_TEST_RAISE( sAlive != 1, ERR_FORK_EXEC_LINKER_PROCESS );
    }

    setLinkerStartTime();
    setLinkerStatus( DKA_LINKER_STATUS_ACTIVATED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_ALREADY_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_ALREAY_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_FORK_EXEC_LINKER_PROCESS );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_FORK_EXEC_LINKER_PROCESS ) );
    }
    IDE_EXCEPTION_END;

    if ( mLinkerProcessInfo.mStatus == DKA_LINKER_STATUS_NON ) 
    {
        mIsPropertiesLoaded = ID_FALSE;
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스를 shutdown 시킨다. 
 *               이 함수는 내부적으로 flag 에 따라 AltiLinker 프로세스로 
 *               하여금 종료단계를 밟는 operation 을 전송한다.
 *
 * aFlag    - [IN] FORCE or not
 *                 이 flag 가 ID_TRUE 로 설정된 경우, 현재 operation 을 
 *                 수행중인 session 이 있다 할지라도 강제로 session 을 
 *                 close 하고 AltiLinker 프로세스를 종료한다. 
 *                 이 flag 가 ID_FALSE 로 설정된 경우, 모든 session 에 
 *                 수행중인 operation 이 존재하지 않는 경우에만
 *                 AltiLinker 프로세스를 종료한다. 
 *
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::shutdownLinkerProcessInternal( idBool  aFlag )
{
    idBool              sIsLocked   = ID_FALSE;
    UInt                sResultCode = DKP_RC_SUCCESS;
    dksCtrlSession     *sCtrlSession = NULL;

    sCtrlSession = dksSessionMgr::getLinkerCtrlSession();

    IDE_ASSERT( sCtrlSession->mMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    setLinkerStatus( DKA_LINKER_STATUS_CLOSING );

    if ( aFlag == ID_TRUE )
    {
        /* need not to prepare */
    }
    else
    {
        IDE_TEST( dkpProtocolMgr::sendPrepareLinkerShutdown( &sCtrlSession->mSession ) 
                  != IDE_SUCCESS );

        IDE_TEST( dkpProtocolMgr::recvPrepareLinkerShutdownResult( &sCtrlSession->mSession, 
                                                                   &sResultCode, 
                                                                   DKP_PREPARE_LINKER_SHUTDOWN_TIMEOUT )  
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS,
                        ERR_PREPARE_LINKER_SHUTDOWN_FAILED );
    }

    IDE_TEST_RAISE( dkpProtocolMgr::doLinkerShutdown( &sCtrlSession->mSession ) 
                    != IDE_SUCCESS, ERR_DO_LINKER_SHUTDOWN_FAILED );

    sIsLocked = ID_FALSE;
    IDE_ASSERT( sCtrlSession->mMutex.unlock() == IDE_SUCCESS );

    setLinkerStatus( DKA_LINKER_STATUS_NON );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PREPARE_LINKER_SHUTDOWN_FAILED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_PREPARE_LINKER_SHUTDOWN_FAILED ) );
    }
    IDE_EXCEPTION( ERR_DO_LINKER_SHUTDOWN_FAILED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKA_DO_LINKER_SHUTDOWN_FAILED ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( sCtrlSession->mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkaLinkerProcessMgr::shutdownLinkerProcess( idBool  aFlag )
{
#ifdef ALTIBASE_PRODUCT_XDB
    
    switch ( idwPMMgr::getProcType() )
    {
        case IDU_PROC_TYPE_DAEMON:
            IDE_TEST( shutdownLinkerProcessInternal( aFlag ) != IDE_SUCCESS );
            break;

        default:
            /* nothing to do */
            break;
    }
    
#else
    
    IDE_TEST( shutdownLinkerProcessInternal( aFlag ) != IDE_SUCCESS );
    
#endif /* ALTIBASE_PRODUCT_XDB */
    
    (void)initLinkerProcessInfo();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::dumpStackTraceInternal( void )
{
    idBool              sIsLocked    = ID_FALSE;
    UInt                sResultCode  = DKP_RC_SUCCESS;
    dksCtrlSession     *sCtrlSession = NULL; 
    SInt                sReceiveTimeout = 0;
    
    sCtrlSession = dksSessionMgr::getLinkerCtrlSession();
    
    IDE_ASSERT( sCtrlSession->mMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;
    
    IDE_TEST( dkpProtocolMgr::sendLinkerDump( &sCtrlSession->mSession ) 
              != IDE_SUCCESS );

    IDE_TEST( getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );
    
    IDE_TEST( dkpProtocolMgr::recvLinkerDumpResult( &sCtrlSession->mSession, 
                                                    &sResultCode, 
                                                    sReceiveTimeout )  
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_REQUEST_LINKER_DUMP_ERROR );

    sIsLocked = ID_FALSE;
    IDE_ASSERT( sCtrlSession->mMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REQUEST_LINKER_DUMP_ERROR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_LINKER_DUMP_ERROR ) );
    }
    IDE_EXCEPTION_END;
    
    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( sCtrlSession->mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::dumpStackTrace()
{
#ifdef ALTIBASE_PRODUCT_XDB
    switch ( idwPMMgr::getProcType() )
    {
        case IDU_PROC_TYPE_DAEMON:
            IDE_TEST( dumpStackTraceInternal() != IDE_SUCCESS );
            break;

        default:
            /* nothing to do */
            break;
    }
#else
    
    IDE_TEST( dumpStackTraceInternal() != IDE_SUCCESS );
    
#endif /* ALTIBASE_PRODUCT_XDB */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스를 위한 설정파일인 dblink.conf 파일로
 *               부터 프로퍼티 값을 읽어온다.
 *
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::loadLinkerProperties() 
{
    idBool sLockFlag = ID_FALSE;
    
    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sLockFlag = ID_TRUE;
    
    if ( mConf != NULL )
    {
        IDE_TEST( dkcFreeDblinkConf( mConf ) != IDE_SUCCESS );

        mConf = NULL;
    }
    else
    {
        /* mConf not exist */
    }

    IDE_TEST( dkcLoadDblinkConf( &mConf ) != IDE_SUCCESS );

    sLockFlag = ID_FALSE;
    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
        
    mIsPropertiesLoaded = ID_TRUE;

#ifdef ALTIBASE_PRODUCT_XDB
    mAltilinkerRestartNumber = dkuSharedProperty::getAltilinkerRestartNumber();
#endif
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLockFlag == ID_TRUE )
    {
        IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    mIsPropertiesLoaded = ID_FALSE;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스의 정보를 얻어온다.
 *
 *  aInfo       - [OUT] AltiLinker 프로세스의 정보가 담겨질 구조체
 *
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::getLinkerProcessInfo( dkaLinkerProcInfo **aInfo )
{
    UInt             sResultCode = DKP_RC_SUCCESS;
    ULong            sTimeoutSec = DKU_DBLINK_ALTILINKER_CONNECT_TIMEOUT;
    dksSession      *sSession    = NULL;

    IDE_DASSERT( aInfo != NULL );

#ifdef ALTIBASE_PRODUCT_XDB
    switch ( idwPMMgr::getProcType() )
    {
        case IDU_PROC_TYPE_DAEMON:
            /* nothing to do */
            break;

        default:
            IDE_RAISE( ERROR_NOT_DAEMON_PROCESS );
            break;
    }
#endif /* ALTIBASE_PRODUCT_XDB */
    
    if ( getLinkerStatus() != DKA_LINKER_STATUS_NON )
    {
        sSession = dksSessionMgr::getCtrlDksSession();

        /* Get informations from AltiLinker process */
        IDE_TEST( dkpProtocolMgr::sendRequestLinkerStatus( sSession ) 
                  != IDE_SUCCESS );

        IDE_TEST( dkpProtocolMgr::recvRequestLinkerStatusResult( sSession, 
                                                                 &sResultCode, 
                                                                 &(mLinkerProcessInfo.mStatus), 
                                                                 sTimeoutSec ) 
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

        IDE_TEST( dkpProtocolMgr::sendRequestDblinkAltiLnkerStatusPvInfo( sSession ) 
                  != IDE_SUCCESS );

        IDE_TEST( dkpProtocolMgr::recvRequestDblinkAltiLnkerStatusPvInfoResult( sSession, 
                                                                                &sResultCode, 
                                                                                &mLinkerProcessInfo, 
                                                                                sTimeoutSec ) 
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

        getLinkerSessionCount();
    }
    else
    {
        initLinkerProcessInfo();
    }

    *aInfo = &mLinkerProcessInfo;

    return IDE_SUCCESS;

#ifdef ALTIBASE_PRODUCT_XDB
    IDE_EXCEPTION( ERROR_NOT_DAEMON_PROCESS )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_ALTILINKER_STATUS ) );
    }
#endif /* ALTIBASE_PRODUCT_XDB */
    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            NULL, 
                                            0, 
                                            0, 
                                            NULL );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Target 이름을 입력받아 해당하는 target 정보를 얻어온다.
 *               해당하는 target 이 존재하지 않는 경우 IDE_FAILURE 를 
 *               반환한다.
 *
 *  aTargetName - [IN] Target name
 *  aInfo       - [OUT] Target 정보가 담겨질 구조체를 가리키는 포인터 
 *
 ************************************************************************/
IDE_RC  dkaLinkerProcessMgr::getTargetInfo( SChar            *aTargetName,
                                            dkaTargetInfo    *aInfo )
{
    idBool                  sIsExist = ID_FALSE;
    dkcDblinkConfTarget    *sTarget  = NULL;
    idBool                  sLockFlag = ID_FALSE;
    
    IDE_TEST( lockAndGetTargetListFromConf( &sTarget ) != IDE_SUCCESS );
    sLockFlag = ID_TRUE;

    while( sTarget != NULL )
    {
        if ( sTarget->mName == NULL )
        {
            /* next item */
            sTarget = sTarget->mNext;
            continue;
        }
        else
        {
            /* do nothing, next processing */
        }

        if ( idlOS::strcasecmp( sTarget->mName, aTargetName ) == 0 )
        {
            idlOS::strncpy( aInfo->mTargetName, sTarget->mName, DK_NAME_LEN + 1 );
            aInfo->mTargetName[DK_NAME_LEN] = '\0';

            if ( ( sTarget->mJdbcDriver != NULL ) && ( sTarget->mConnectionUrl != NULL ) )
            {
                idlOS::strncpy( aInfo->mJdbcDriverPath, sTarget->mJdbcDriver, DK_PATH_LEN + 1);
                aInfo->mJdbcDriverPath[DK_PATH_LEN] = '\0';


                idlOS::strncpy( aInfo->mRemoteServerUrl, sTarget->mConnectionUrl, DK_URL_LEN + 1);
                aInfo->mRemoteServerUrl[DK_URL_LEN] = '\0';


                if ( sTarget->mJdbcDriverClassName != NULL ) /* options */
                {
                    idlOS::strncpy( aInfo->mJdbcDriverClassName, sTarget->mJdbcDriverClassName, DK_DRIVER_CLASS_NAME_LEN + 1 );
                    aInfo->mJdbcDriverClassName[DK_DRIVER_CLASS_NAME_LEN] = '\0';
                }
                else
                {
                    /* do nothing */
                }

                /*bellow items are options*/
                if ( sTarget->mUser != NULL )
                {
                    idlOS::strncpy( aInfo->mRemoteServerUserID, sTarget->mUser, DK_USER_ID_LEN + 1 );
                    aInfo->mRemoteServerUserID[DK_USER_ID_LEN] = '\0';

                }
                else
                {
                    /* do nothing */
                }
                if ( sTarget->mPassword != NULL )
                {
                    idlOS::strncpy( aInfo->mRemoteServerPasswd, sTarget->mPassword, DK_USER_PW_LEN + 1 );
                    aInfo->mRemoteServerPasswd[DK_USER_PW_LEN] = '\0';
;
                }
                else
                {
                    /* do nothing */
                }
                if ( sTarget->mXADataSourceClassName != NULL )
                {
                    idlOS::strncpy( aInfo->mXADataSourceClassName,
                                sTarget->mXADataSourceClassName, DK_PATH_LEN + 1 );
                    aInfo->mXADataSourceClassName[DK_PATH_LEN]= '\0';
0;
                }
                else
                {
                    /* do nothing */
                }

                if ( sTarget->mXADataSourceUrlSetterName != NULL )
                {
                    idlOS::strncpy( aInfo->mXADataSourceUrlSetterName,
                                    sTarget->mXADataSourceUrlSetterName, DK_NAME_LEN + 1 );
                    aInfo->mXADataSourceUrlSetterName[DK_NAME_LEN] = '\0';
0;
                }
                else
                {
                    /* do nothing */
                }

                sIsExist = ID_TRUE;
                break;
            }
            else
            {
                /* next item */
            }
        }
        else
        {
            /* next item */
        }
        sTarget = sTarget->mNext;
    }

    sLockFlag = ID_FALSE;
    unlockTargetListFromConf();
    
    IDE_TEST_RAISE( sIsExist != ID_TRUE, ERR_INVALID_TARGET_NAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TARGET_NAME );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TARGET ) );
    }
    IDE_EXCEPTION_END;

    if ( sLockFlag == ID_TRUE )
    {
        unlockTargetListFromConf();
    }
    else
    {
        /* nothing to do */
    }
        
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Target 이름을 입력받아 해당하는 valid 한 target 정보가 
 *               존재하는 경우 aIsValid 에 ID_TRUE 를 반환한다.
 *
 * Return      : aTargetName 에 해당하는 valid 한 target 이 존재하는 경우
 *               ID_TRUE, 아니면 ID_FALSE.
 *
 *  aTargetName - [IN] Target name
 *
 ************************************************************************/
idBool  dkaLinkerProcessMgr::validateTargetInfo( SChar  *aTargetName )
{
    idBool                sIsValid = ID_FALSE;
    dkcDblinkConfTarget  *sTarget  = NULL;
    
    (void)lockAndGetTargetListFromConf( &sTarget );

    while( sTarget != NULL )
    {
        if (  sTarget->mName != NULL )
        {
            if ( idlOS::strcasecmp( sTarget->mName, aTargetName ) == 0 )
            {
                if ( ( sTarget->mJdbcDriver != NULL ) && 
                     ( sTarget->mConnectionUrl != NULL ) ) 
                {
                    sIsValid = ID_TRUE;
                    break;
                }
                else
                {
                    /* next target */
                }
            }
            else
            {
                /* next target */
            }
        }
        else
        {
            /* next target */
        }
                
        sTarget = sTarget->mNext;
    }

    unlockTargetListFromConf();
    
    return sIsValid;
}

/************************************************************************
 * Description : Target list 의 길이를 반환한다.
 *
 * Return      : Target list 의 길이
 *
 ************************************************************************/
UInt dkaLinkerProcessMgr::getTargetsLength( dkcDblinkConfTarget *aTargetItem )
{
    UInt                    sTotalLen = 0;
    dkcDblinkConfTarget    *sTarget   = aTargetItem;  

    while( sTarget != NULL )
    {
        sTotalLen += getTargetItemLength( sTarget );

        sTarget = sTarget->mNext;
    }

    return sTotalLen;
}

/************************************************************************
 * Description : 입력받은 target item 의 길이를 반환한다.
 *
 * Return      : Target item 의 길이
 *
 *  aTargetItem - [IN] 길이를 구할 target item
 *
 ************************************************************************/
UInt dkaLinkerProcessMgr::getTargetItemLength( dkcDblinkConfTarget *aTargetItem )
{
    UInt    sTotalLen = 0;

    if ( aTargetItem->mName != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mName );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mJdbcDriver != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mJdbcDriver );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mJdbcDriverClassName != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mJdbcDriverClassName );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mConnectionUrl != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mConnectionUrl );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mUser != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mUser );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mPassword != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mPassword );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mXADataSourceClassName != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mXADataSourceClassName );
    }
    else
    {
        /* next item */
    }

    if ( aTargetItem->mXADataSourceUrlSetterName != NULL )
    {
        sTotalLen += idlOS::strlen( aTargetItem->mXADataSourceUrlSetterName );
    }
    else
    {
        /* next item */
    }

    return sTotalLen;
}

/************************************************************************
 * Description : Target list 에 갖고 있는 target 의 갯수를 반환한다.
 *
 * Return      : Target 의 갯수
 *
 ************************************************************************/
UInt dkaLinkerProcessMgr::getTargetItemCount( dkcDblinkConfTarget *aTargetItem )
{
    UInt                    sCount  = 0;
    dkcDblinkConfTarget    *sTarget = aTargetItem;

    while( sTarget != NULL )
    {
        sCount++;
        sTarget = sTarget->mNext;
    }
    
    return sCount;
}

/*
 *
 */
IDE_RC dkaLinkerProcessMgr::checkAltilinkerRestartNumberAndReloadLinkerProperties( void )
{
#ifdef ALTIBASE_PRODUCT_XDB
    if ( dkuSharedProperty::getAltilinkerRestartNumber() != mAltilinkerRestartNumber )
    {
        IDE_TEST( loadLinkerProperties() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#else /* ALTIBASE_PRODUCT_XDB */

    return IDE_SUCCESS;
    
#endif /* ALTIBASE_PRODUCT_XDB */
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerPortNoFromConf( SInt *aPortNo )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aPortNo = mConf->mAltilinkerPortNo;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( SInt *aReceiveTimeout )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aReceiveTimeout = mConf->mAltilinkerReceiveTimeout;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( SInt *aRemoteNodeReceiveTimeout )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aRemoteNodeReceiveTimeout = mConf->mAltilinkerRemoteNodeReceiveTimeout;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerQueryTimeoutFromConf( SInt *aQueryTimeout )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aQueryTimeout = mConf->mAltilinkerQueryTimeout;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerNonQueryTimeoutFromConf( SInt *aNonQueryTimeout )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aNonQueryTimeout = mConf->mAltilinkerNonQueryTimeout;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerThreadCountFromConf( SInt *aThreadCount )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aThreadCount = mConf->mAltilinkerThreadCount;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerThreadSleepTimeFromConf( SInt *aThreadSleepTime )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aThreadSleepTime = mConf->mAltilinkerThreadSleepTime;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerRemoteNodeSessionCountFromConf( SInt *aRemoteNodeSessionCount )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aRemoteNodeSessionCount =  mConf->mAltilinkerRemoteNodeSessionCount;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerTraceLogFileSizeFromConf( SInt *aTraceLogFileSize )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aTraceLogFileSize = mConf->mAltilinkerTraceLogFileSize;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerTraceLogFileCountFromConf( SInt *aTraceLogFileCount )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aTraceLogFileCount = mConf->mAltilinkerTraceLogFileCount;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkaLinkerProcessMgr::getAltiLinkerTraceLoggingLevelFromConf( SInt *aTraceLoggingLevel )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aTraceLoggingLevel = mConf->mAltilinkerTraceLoggingLevel;

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * unlockTargetListFromConf() has to be called after this function is called.
 */ 
IDE_RC dkaLinkerProcessMgr::lockAndGetTargetListFromConf( dkcDblinkConfTarget **aTargetList )
{
    IDE_TEST( checkAltilinkerRestartNumberAndReloadLinkerProperties() != IDE_SUCCESS );

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    *aTargetList = mConf->mTargetList;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
void dkaLinkerProcessMgr::unlockTargetListFromConf( void )
{
    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
}
