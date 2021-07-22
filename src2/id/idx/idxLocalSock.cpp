/***********************************************************************
 * Copyright 1999-2013, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idxLocalSock.cpp 83346 2018-06-26 03:32:55Z kclee $
 **********************************************************************/

#include <idx.h>
#include <idp.h>

SChar* idxLocalSock::mHomePath = NULL;

void idxLocalSock::initializeStatic()
{
    idxLocalSock::mHomePath = idlOS::getenv(IDP_HOME_ENV);
}

void idxLocalSock::destroyStatic()
{
    /* Nothing to do. */
}

IDE_RC idxLocalSock::ping( UInt            aPID,
                           IDX_LOCALSOCK * aSocket,
                           idBool        * aIsAlive )
{
/***********************************************************************
 *
 * Description : Agent Process가 연결 가능한 상태인지 확인한다.
 *          
 *       - Named Pipe는 프로세스가 존재하는지를 검증하려면 추가 핸들이 필요한데, 
 *         이 비용이 꽤 크기 때문에 이미 가지고 있는 Pipe 핸들을 이용해 Peek을 수행한다.
 *         
 *       - Unix Domain Socket은 kill -0 를 수행해 프로세스가 존재하는지 검증한다. 
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    BOOL  sIsAliveConn;
    DWORD sErr;
    
    PDL_UNUSED_ARG( aPID );

    sIsAliveConn = PeekNamedPipe( aSocket->mHandle, 
                                  NULL,
                                  1,
                                  NULL,
                                  NULL,
                                  NULL );
                       
    if ( sIsAliveConn == TRUE )
    {
        *aIsAlive = ID_TRUE;
    }
    else
    {
        sErr = GetLastError();
        IDE_TEST( ( sErr != ERROR_BROKEN_PIPE ) && 
                  ( sErr != ERROR_PIPE_NOT_CONNECTED ) );
        
        // if Last Error is BROKEN_PIPE || PIPE_NOT_CONNECTED, then...
        *aIsAlive = ID_FALSE;
    }
#else
    int retval = PDL_OS::kill ( aPID, 0 );

    PDL_UNUSED_ARG( aSocket );
    
    if ( retval == 0 )
    {
        *aIsAlive = ID_TRUE;
    }
    else
    {
        IDE_TEST( errno != ESRCH );
        
        // if Last Error is NO_SUCH_PROCESS, then...
        *aIsAlive = ID_FALSE;
    }
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void idxLocalSock::setPath( SChar         * aBuffer,
                            SChar         * aSockName,
                            idBool          aNeedUniCode )
{
/***********************************************************************
 *
 * Description : Socket 파일(Named Pipe 파일)의 경로를 생성한다.
 *          
 *       - Named Pipe는 경로가 고정되어 있다. (IDX_PIPEPATH 참고)
 *         여기에 Named Pipe 이름만 추가하면 되지만, 
 *         Unicode 문제로 인해 HDB/Agent의 이름 생성 방식을 달리 한다.
 *  
 *       - Unix Domain Socket은 $ALTIBASE_HOME/trc/(Socket이름)이다. 
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    UInt    sLen = idlOS::strlen(IDX_PIPEPATH);

    idlOS::strcpy( aBuffer, IDX_PIPEPATH );

    if( aNeedUniCode == ID_TRUE )
    {
        idlOS::memcpy( (aBuffer + sLen),
                       PDL_TEXT(aSockName),
                       idlOS::strlen(aSockName) ); 

        sLen           += idlOS::strlen(aSockName);
        aBuffer[sLen]   = '\0';
        aBuffer[sLen+1] = '\0';
    }
    else
    {
        idlOS::memcpy( (aBuffer + sLen),
                       aSockName,
                       idlOS::strlen(aSockName) ); 

        sLen           += idlOS::strlen(aSockName);
        aBuffer[sLen]   = '\0';
    }
#else

    PDL_UNUSED_ARG( aNeedUniCode );

    // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
    idlOS::snprintf( aBuffer,
                     IDX_SOCK_PATH_MAXLEN,
                     "%s%c%s",
                     iduProperty::getExtprocAgentSocketFilepath(),
                     IDL_FILE_SEPARATOR,
                     aSockName );
#endif /* ALTI_CFG_OS_WINDOWS */
}

IDE_RC idxLocalSock::initializeSocket( IDX_LOCALSOCK * aSocket,
                                       iduMemory     * aMemory )
{
/***********************************************************************
 *
 * Description : Socket을 초기화한다. (iduMemory)
 *
 *       - Named Pipe는 Overlapped I/O에 쓰일 Event Handle을 생성한다.
 *         이 때, iduMemory를 가지고, iduMemory::alloc()을 호출한다.
 *
 *       - Unix Domain Socket은 Socket 값을 초기화한다. 
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    iduMemoryStatus sMemoryStatus;
    UInt            sState = 0;

    aSocket->mHandle = PDL_INVALID_HANDLE;

    IDE_TEST( aMemory->getStatus( &sMemoryStatus ) != IDE_SUCCESS );
    IDE_TEST( aMemory->alloc( sizeof(OVERLAPPED), 
                              (void**)&aSocket->mOverlap ) != IDE_SUCCESS );
    sState = 1;
               
    // BUG-39814 offset / offsetHigh 초기화
    aSocket->mOverlap->Offset = 0;
    aSocket->mOverlap->OffsetHigh = 0;
    
    aSocket->mOverlap->hEvent = CreateEvent( 
                                    NULL,    // default security attribute 
                                    TRUE,    // manual-reset event 
                                    TRUE,    // initial state = signaled 
                                    NULL);   // unnamed event object 
    IDE_TEST( aSocket->mOverlap->hEvent == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)aMemory->setStatus( &sMemoryStatus );
            /*@fallthrough*/
        default :
            break;
    }

    return IDE_FAILURE;
#else
    PDL_UNUSED_ARG( aMemory );
    *aSocket = PDL_INVALID_HANDLE;
    return IDE_SUCCESS;
#endif /* ALTI_CFG_OS_WINDOWS */
}

IDE_RC idxLocalSock::initializeSocket( IDX_LOCALSOCK        * aSocket,
                                       iduMemoryClientIndex   aIndex )
{
/***********************************************************************
 *
 * Description : Socket을 초기화한다. (iduMemMgr)
 *
 *       - Named Pipe는 Overlapped I/O에 쓰일 Event Handle을 생성한다.
 *         이 때, iduMemoryClientIndex로 iduMemMgr::malloc()을 호출한다.
 *
 *       - Unix Domain Socket은 Socket 값을 초기화한다. 
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    UInt sState = 0;

    aSocket->mHandle = PDL_INVALID_HANDLE;
    IDE_TEST( iduMemMgr::malloc( aIndex,
                                 ID_SIZEOF(OVERLAPPED),
                                 (void**)&aSocket->mOverlap,
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );
    sState = 1;
    
    // BUG-39814 offset / offsetHigh 초기화
    aSocket->mOverlap->Offset = 0;
    aSocket->mOverlap->OffsetHigh = 0;
    
    aSocket->mOverlap->hEvent = CreateEvent( 
                                    NULL,    // default security attribute 
                                    TRUE,    // manual-reset event 
                                    TRUE,    // initial state = signaled 
                                    NULL);   // unnamed event object 
    IDE_TEST( aSocket->mOverlap->hEvent == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free( aSocket->mOverlap );
            /*@fallthrough*/
        default :
            break;
    }

    return IDE_FAILURE;
#else
    PDL_UNUSED_ARG( aIndex );
    *aSocket = PDL_INVALID_HANDLE;
    return IDE_SUCCESS;
#endif /* ALTI_CFG_OS_WINDOWS */
}

void idxLocalSock::finalizeSocket( IDX_LOCALSOCK * aSock )
{
/***********************************************************************
 *
 * Description : Socket 내부에 할당된 공간을 해제한다. (iduMemMgr)
 *
 *          - Agent Process는 종료 시 같이 해제되므로, 호출하지 않는다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    (void)iduMemMgr::free( aSock->mOverlap );
#else
    PDL_UNUSED_ARG( aSock );
    // Nothing to do.
#endif
}

IDE_RC idxLocalSock::socket( IDX_LOCALSOCK * aSock )
{
/***********************************************************************
 *
 * Description : Socket 값을 초기화한다.
 *
 *       - Named Pipe는 이미 initializeSocket 에서 초기화되었다.
 *
 *       - Unix Domain Socket은 socket() 함수를 호출, 새 값을 받는다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    /* Nothing to do. */
#else
    *aSock = idlOS::socket( AF_UNIX, SOCK_STREAM, 0 );
    IDE_TEST( *aSock == PDL_INVALID_HANDLE );
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::bind( IDX_LOCALSOCK * aSock,
                           SChar         * aPath,
                           iduMemory     * aMemory )
{
/***********************************************************************
 *
 * Description : Socket 객체가 쓸 Socket 파일(Named Pipe 파일)을 생성한다.
 *
 *       - Named Pipe는, 파일 위치는 wchar_t로 나타내야 하므로
 *         기존 Path를 wchar_t로 변환해 Named Pipe를 생성한다.
 *
 *       - Unix Domain Socket은 파일 위치를 가지고 bind()를 호출한다.
 *         소켓 파일을 unlink()로 지워줘야 새로운 소켓 파일이 생성된다.
 *         그 다음, 소켓 옵션을 REUSEADDR로 둬서 파일이 이미 Binding
 *         되어 있다고 해도 재사용이 가능하도록 조정한다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    wchar_t         * sWStr;
    iduMemoryStatus   sMemoryStatus;
    UInt              sState = 0;
    SInt              sLen = idlOS::strlen( aPath ) + 1;

    IDE_TEST( aMemory->getStatus( &sMemoryStatus ) != IDE_SUCCESS );
    IDE_TEST( aMemory->alloc( ( ID_SIZEOF(wchar_t) * sLen ),
                              (void **)&sWStr ) 
              != IDE_SUCCESS );
    sState = 1;
    
    // char > wchar_t
    mbstowcs( sWStr, aPath, sLen );
    
    aSock->mHandle = CreateNamedPipeW(
        sWStr,                      // name of the pipe
        PIPE_ACCESS_DUPLEX |        // 2-way pipe
        FILE_FLAG_OVERLAPPED,       // overlapped mode
        PIPE_TYPE_BYTE |            // send data as a byte stream
        PIPE_WAIT,                  // blocking mode
        1,                          // only allow 1 instance of this pipe
        0,                          // no outbound buffer
        0,                          // no inbound buffer
        0,                          // use default wait time
        NULL                        // use default security attributes
    );
    
    IDE_TEST( aSock->mHandle == 0 || aSock->mHandle == INVALID_HANDLE_VALUE )

    sState = 0;
    IDE_TEST( aMemory->setStatus( &sMemoryStatus ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)aMemory->setStatus( &sMemoryStatus );
            /*@fallthrough*/
        default :
            break;
    }

    return IDE_FAILURE;
#else
    struct sockaddr_un sAddr;
    SInt               sOption;
    
    PDL_UNUSED_ARG( aMemory );

    // Socket is already connected.
    IDE_TEST( *aSock == PDL_INVALID_HANDLE );

    // Add socket option : SO_REUSEADDR
    sOption = 1;
    IDE_TEST( idlOS::setsockopt( *aSock,
                                 SOL_SOCKET,
                                 SO_REUSEADDR,
                                 (SChar *)&sOption,
                                 ID_SIZEOF(sOption) ) < 0 );

    idlOS::memset( &sAddr, 0, ID_SIZEOF( sAddr ) );
    sAddr.sun_family  = AF_UNIX;
    idlOS::strcpy( (SChar*)sAddr.sun_path, aPath );

    // unlink socket file before binding
    idlOS::unlink( sAddr.sun_path );

    IDE_TEST( idlOS::bind( *aSock,
                           (struct sockaddr *)&sAddr,
                           ID_SIZEOF(sAddr) ) != 0 );
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif /* ALTI_CFG_OS_WINDOWS */
}

IDE_RC idxLocalSock::listen( IDX_LOCALSOCK aSock )
{
/***********************************************************************
 *
 * Description : Socket 연결을 기다린다.
 *
 *       - Named Pipe는 accept()에서 연결을 기다린다.
 *
 *       - Unix Domain Socket은 
 *         listen()으로 연결 신호가 올 때 까지 Blocking 된다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    /* Nothing to do. */
#else
    IDE_TEST( idlOS::listen( aSock, IDX_LOCALSOCK_BACKLOG ) < 0 );
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::accept( IDX_LOCALSOCK   aServSock,
                             IDX_LOCALSOCK * aClntSock,
                             PDL_Time_Value  aWaitTime,  
                             idBool        * aIsTimeout )
{
/***********************************************************************
 *
 * Description : Socket 연결을 수락한다.
 *
 *       - Named Pipe는 연결 이벤트를 Overlapped I/O로 대기한 후,
 *         연결 이벤트가 일어나면 연결된 파이프를 반환한다.
 *
 *       - Unix Domain Socket은 listen()으로 연결 신호를 받았으므로
 *         accept()만 호출하게 된다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    IDE_RC  sRC = IDE_SUCCESS;
    
    BOOL    sPending;
    BOOL    sResult;
    DWORD   sWait;
    DWORD   sTransferred;
    DWORD   sWaitMsec;
    
    /* Non-blocking mode에서는 결과가 0인데도 다음과 같은 (정상) 에러가 발생할 수 있다.
     *
     * - ERROR_IO_PENDING : 지금은 연결할 클라이언트가 아직 발견되지 않았다.
     *                      그래서 PENDING을 true로 두고 IDLE_TIMEOUT 까지 기다려 본다.
     * - ERROR_PIPE_CONNECTED : 다행히, 지금 연결할 클라이언트가 존재한다.
     *                          PENDING 할 필요 없이 연결을 수락한다.
     *
     * 결과가 0이 아닌 경우에는 에러 상황이며, 이 경우 accept() 함수를 실패로 돌린다.
     */

    sWaitMsec = (DWORD)( aWaitTime.sec() * 1000 );
    *aIsTimeout = ID_FALSE;
    
    sResult = ConnectNamedPipe( aServSock.mHandle, aServSock.mOverlap );
    IDE_TEST( sResult != 0 );
    
    switch (GetLastError()) 
    { 
        // The overlapped connection in progress. 
        case ERROR_IO_PENDING: 
            sPending = ID_TRUE; 
            break; 
        // Client is already connected
        case ERROR_PIPE_CONNECTED: 
            sPending = ID_FALSE; 
            break; 
        // If an error occurs during the connect operation
        default: 
            IDE_RAISE(IDE_EXCEPTION_END_LABEL);
            break;
    }
    
    // in connection mode, we don't have to put while loop in the code.
    if( sPending == ID_TRUE )
    {
        // waits IDLE_TIMEOUT second
        sWait = WaitForSingleObject( aServSock.mOverlap->hEvent, sWaitMsec );        

        if( sWait == WAIT_TIMEOUT )
        {
            // Connection timeout
            *aIsTimeout = ID_TRUE;
        }
        else
        {
            sResult = GetOverlappedResult( 
                            aServSock.mHandle,      // pipe handle
                            aServSock.mOverlap,     // OVERLAPPED structure 
                            &sTransferred,          // bytes transferred 
                            FALSE );                // do not wait
                            
            // if it is still pending, it has an error.
            IDE_TEST( sResult != TRUE )
            // set established handle to client handle
            aClntSock->mHandle = aServSock.mHandle;
        }
    }
    else
    {
        // set established handle to client handle
        aClntSock->mHandle = aServSock.mHandle;
    }
#else
    struct sockaddr_un    sClntAddr;
    SInt                  sAddrLen;

    PDL_UNUSED_ARG( aWaitTime );
    PDL_UNUSED_ARG( aIsTimeout );

    sAddrLen  = ID_SIZEOF( sClntAddr );
    *aClntSock = idlOS::accept(
                         aServSock,
                         (struct sockaddr *)&sClntAddr,
                         &sAddrLen );

    IDE_TEST( *aClntSock == PDL_INVALID_HANDLE );
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::connect( IDX_LOCALSOCK * aSock,
                              SChar         * aPath,
                              iduMemory     * aMemory,
                              idBool        * aIsRefused )
{
/***********************************************************************
 *
 * Description : Socket 연결을 시도한다.
 *
 *       - Named Pipe는, 파일 위치는 wchar_t로 나타내야 하므로
 *         기존 Path를 wchar_t로 변환해 Named Pipe를 열어본다.
 *
 *       - Unix Domain Socket은 connect()를 호출해 연결을 시도한다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    wchar_t         * sWStr;
    iduMemoryStatus   sMemoryStatus;
    UInt              sState = 0;
    SInt              sLen = idlOS::strlen( aPath ) + 1;
    
    IDE_TEST( aMemory->getStatus( &sMemoryStatus ) != IDE_SUCCESS );
    IDE_TEST( aMemory->alloc( ( ID_SIZEOF(wchar_t) * sLen ),
                              (void **)&sWStr ) 
              != IDE_SUCCESS );
    sState = 1;
    
    // char > wchar_t
    mbstowcs( sWStr, aPath, sLen );

    aSock->mHandle = CreateFileW( 
                           sWStr,                               // location
                           GENERIC_WRITE | GENERIC_READ,        // only write
                           FILE_SHARE_READ | FILE_SHARE_WRITE,  // file flags
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );
                           
    IDE_TEST( aSock->mHandle == 0 || aSock->mHandle == INVALID_HANDLE_VALUE )

    sState = 0;
    IDE_TEST( aMemory->setStatus( &sMemoryStatus ) != IDE_SUCCESS );

    // BUG-37957
    // Windows platform doesn't care about ECONNREFUSED
    *aIsRefused = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)aMemory->setStatus( &sMemoryStatus );
            /*@fallthrough*/
        default :
            break;
    }
    return IDE_FAILURE;
#else
    struct sockaddr_un sAddr;
    SInt   sSockRet = 0;

    PDL_UNUSED_ARG( aMemory );

    idlOS::memset( &sAddr, 0, ID_SIZEOF( sAddr ) );
    sAddr.sun_family  = AF_UNIX;
    idlOS::strncpy( (SChar*)sAddr.sun_path, aPath, ID_SIZEOF(sAddr.sun_path) - 1 ); 
    sAddr.sun_path[ID_SIZEOF(sAddr.sun_path) - 1] = 0; 

    sSockRet = idlOS::connect( *aSock,
                               (struct sockaddr *)&sAddr,
                               ID_SIZEOF(sAddr) );
    IDE_TEST( sSockRet < 0 );

    *aIsRefused = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-37957
     * ECONNREFUSED가 발생하면, 소켓을 재생성해야 한다.
     */
    if( errno == ECONNREFUSED )
    {
        *aIsRefused = ID_TRUE;
    }
    else
    {
        *aIsRefused = ID_FALSE;
    }

    return IDE_FAILURE;
#endif /* ALTI_CFG_OS_WINDOWS */
}

IDE_RC idxLocalSock::select( IDX_LOCALSOCK    aSock,
                             PDL_Time_Value * aWaitTime,
                             idBool         * aIsTimeout )
{
/***********************************************************************
 *
 * Description : Socket의 상태를 대기한다.
 *
 *       - Named Pipe는, Overlapped I/O를 recv(), send()에 사용하므로
 *         여기서는 아무런 일도 하지 않는다.
 *
 *       - Unix Domain Socket은 select()로 소켓 파일 디스크립터의
 *         상태를 관찰한다. 해당 파일이 읽을 수 있는 상태로 전환되면 
 *         select()는 Blocking을 풀지만, 그 외에는 IDLE_TIMEOUT까지
 *         대기하다가 Blocking을 푼다 (TIMEOUT)
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    /* Nothing to do. */
#else
    SInt    sNum = 0;
    fd_set  sSocketFd;
    
    *aIsTimeout = ID_FALSE;

    FD_ZERO( &sSocketFd );
    FD_SET( aSock, &sSocketFd );

    sNum = idlOS::select( aSock + 1, &sSocketFd, NULL, NULL, aWaitTime );
    IDE_TEST( sNum < 0 );
    
    if( sNum == 0 )
    {
        *aIsTimeout = ID_TRUE;
    }
    else
    {
        /* Nothing to do : socket is signaled */
    }
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::send( IDX_LOCALSOCK   aSock,
                           void          * aBuffer,
                           UInt            aBufferSize )
{
/***********************************************************************
 *
 * Description : Blocking Mode Message Send
 *
 *       - Named Pipe는, Pipe 파일에 메시지를 작성한다.
 *
 *       - Unix Domain Socket은 send() 함수를 호출한다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    DWORD numByteWritten;
    BOOL result;

    IDE_TEST( aBufferSize <= 0 );
    result = WriteFile( aSock.mHandle, aBuffer, aBufferSize, &numByteWritten, NULL );
    IDE_TEST( !result ); 
    IDE_TEST( numByteWritten != (DWORD)aBufferSize );
#else
    SInt sSockRet = 0;

    IDE_TEST( aBufferSize <= 0 );
    sSockRet = idlVA::send_i( aSock, aBuffer, aBufferSize );
    IDE_TEST( sSockRet <= 0 );
    IDE_TEST( (UInt)sSockRet != aBufferSize );
#endif /* ALTI_CFG_OS_WINDOWS */
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::sendTimedWait( IDX_LOCALSOCK   aSock,
                                    void          * aBuffer,
                                    UInt            aBufferSize,
                                    PDL_Time_Value  aWaitTime,
                                    idBool        * aIsTimeout )
{
/***********************************************************************
 *
 * Description : Non-Blocking Mode Message Send
 *
 *       - Named Pipe는, 메시지를 전부 보낼 때 까지 Overlapped I/O를 사용,
 *         메시지를 보낼 수 있을 때 까지 기다렸다가 메시지를 계속 보낸다.
 *         상대방이 메시지를 받을 수 없는 경우, Timeout Error를 반환한다.
 *
 *       - Unix Domain Socket은 send() 함수와 동일하다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    /* Send with Overlapped I/O-based Asynchronous Communication */
    IDE_RC sRC = IDE_SUCCESS;

    DWORD sWaitMsec;
    DWORD numByteWritten = 0;
    DWORD sErr;
    DWORD sWait;
    DWORD sTransferred;
    BOOL  sResult;
    BOOL  sPending;
    
    sWaitMsec = (DWORD)( aWaitTime.sec() * 1000 );
    *aIsTimeout = ID_FALSE;
    
    sResult = WriteFile( 
                    aSock.mHandle, 
                    aBuffer, 
                    aBufferSize, 
                    &numByteWritten, 
                    aSock.mOverlap );
                    
    if( ( sResult == TRUE ) && ( numByteWritten == aBufferSize ) )
    {
        sPending = FALSE; 
    }
    else if ( ( sResult == TRUE ) && ( numByteWritten < aBufferSize ) )
    {
        sPending = TRUE; 
    }
    else
    {
        sErr = GetLastError(); 
        IDE_TEST( ( sResult != FALSE ) || ( sErr != ERROR_IO_PENDING ) ) ;
        sPending = TRUE; 
    }
    
    if( sPending == TRUE )
    {
        while( numByteWritten < aBufferSize )
        {
            sWait = WaitForSingleObject( aSock.mOverlap->hEvent, sWaitMsec );        
            sErr = GetLastError();
            IDE_TEST( sErr == ERROR_BROKEN_PIPE ); 
                        
            if( sWait == WAIT_TIMEOUT )
            {
                *aIsTimeout = ID_TRUE;
                break;
            }
            else
            {
                sResult = GetOverlappedResult( 
                            aSock.mHandle,      // pipe handle
                            aSock.mOverlap,     // OVERLAPPED structure 
                            &sTransferred,      // bytes transferred 
                            FALSE );            // do not wait
                        
                // if (pending condition)
                if( ( sResult == FALSE ) || sTransferred == 0 ) 
                { 
                    continue;
                }
                else
                {
                    numByteWritten += sTransferred;
                }
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }
#else
    /* Same as send() */
    PDL_UNUSED_ARG( aWaitTime  );
    PDL_UNUSED_ARG( aIsTimeout );
    IDE_TEST( send( aSock, aBuffer, aBufferSize ) != IDE_SUCCESS );
#endif /* ALTI_CFG_OS_WINDOWS */
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::recv( IDX_LOCALSOCK   aSock,
                           void          * aBuffer,
                           UInt            aBufferSize,
                           UInt          * aReceivedSize )
{
/***********************************************************************
 *
 * Description : Blocking Mode Message Receive 
 *
 *       - Named Pipe는, Pipe 파일에 쓰여진 메시지를 읽는다.
 *
 *       - Unix Domain Socket은 recv() 함수를 호출한다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    DWORD numByteRead;
    BOOL result;

    IDE_TEST( aBufferSize <= 0 );
    result = ReadFile( aSock.mHandle, aBuffer, aBufferSize, &numByteRead, NULL );
    IDE_TEST( !result ); 
    *aReceivedSize = numByteRead;
#else
    SInt           sSockRet       = 0;
    UInt           sTotalRecvSize = 0;
    SChar *        sBuffer        = NULL; 
    PDL_Time_Value sWaitTime;
    idBool         sIsTimeout     = ID_FALSE;

    /* BUG-39814
     * recv()는, 전송되는 정보를 다 받을 때 까지 기다리지 않는다.
     * 만약 recv()가 원하는 길이보다 적은 길이의 정보를 받았다면
     * 얼마간의 시간 동안 더 많은 데이터가 들어오는지 감지한 다음 다시 recv() 해야 한다.
     * 이 시간 동안 데이터가 더 이상 들어오지 않는다면, recv()는 종료한다.
     */

    IDE_TEST( aBufferSize <= 0 );

    sWaitTime.set( IDX_LOCALSOCK_RECV_WAITSEC, 0 );
    *aReceivedSize  = 0;
    sBuffer         = (SChar*)aBuffer;
    
    // 첫 recv_i()를 호출해 본다.
    sSockRet = idlVA::recv_i( aSock, 
                              (void*)sBuffer, 
                              aBufferSize );
    
    IDE_TEST( sSockRet < 0 );
    sTotalRecvSize += (UInt)sSockRet;

    // 받아야 할 만큼 받지 못했다면, 계속 받는다.
    while ( sTotalRecvSize < aBufferSize )
    {
        IDE_TEST( select( aSock, &sWaitTime, &sIsTimeout ) );

        if ( sIsTimeout == ID_TRUE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        /* recv_i()의 반환값 (Blocking-mode 임을 상기하자)
         * 
         *  return > 0  : 실제로 읽을 데이터가 존재해서, 능력껏 읽어온 경우
         *  return = 0  : select() 는 '읽을 수 있다' 고 했지만 
         *                사실 end-of-file이라 그냥 종료되는 경우
         *  return = -1 : 에러 상황 (읽다가 연결이 끊기거나, 다른 이유로)
         */
        sSockRet = idlVA::recv_i( aSock, 
                                  (void*)(sBuffer + sTotalRecvSize), 
                                  (aBufferSize - sTotalRecvSize) );
    
        IDE_TEST( sSockRet < 0 );
        
        if ( sSockRet == 0 )
        {
            // end-of-file. 즉 소켓이 닫혔지만 recv_i() 가 호출된 상황
            break;
        }
        else
        {
            // recv_i() 가 정상적으로 데이터를 받은 상황
            sTotalRecvSize += (UInt)sSockRet;
        }
    }

    *aReceivedSize = sTotalRecvSize;
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::recvTimedWait( IDX_LOCALSOCK   aSock,
                                    void          * aBuffer,
                                    UInt            aBufferSize,
                                    PDL_Time_Value  aWaitTime,
                                    UInt          * aReceivedSize,
                                    idBool        * aIsTimeout )
{
/***********************************************************************
 *
 * Description : Non-Blocking Mode Message Receive 
 *
 *       - Named Pipe는, 메시지를 전부 받을 때 까지 Overlapped I/O를 사용,
 *         메시지를 받을 수 있을 때 까지 기다렸다가 메시지를 계속 받는다.
 *         상대방이 메시지를 보낼 의사가 없는 경우, Timeout을 반환한다.
 *
 *       - Unix Domain Socket은 recv() 함수와 동일하다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    /* Recv with Overlapped I/O-based Asynchronous Communication */
    IDE_RC  sRC = IDE_SUCCESS;
    
    DWORD sWaitMsec;
    DWORD numByteRead = 0;
    DWORD sErr;
    DWORD sWait;
    DWORD sTransferred;
    BOOL  sResult;
    BOOL  sPending;

    *aReceivedSize = 0;
    sWaitMsec = (DWORD)( aWaitTime.sec() * 1000 );
    *aIsTimeout = ID_FALSE;
    
    sResult = ReadFile( 
                    aSock.mHandle, 
                    aBuffer, 
                    aBufferSize, 
                    &numByteRead, 
                    aSock.mOverlap );
                    
    if( ( sResult == TRUE ) && ( numByteRead == aBufferSize ) )
    {
        sPending = FALSE; 
    }
    else if ( ( sResult == TRUE ) && ( numByteRead < aBufferSize ) )
    {
        sPending = TRUE;
    }
    else
    {
        sErr = GetLastError(); 
        
        if( ( sResult == FALSE ) && ( sErr == ERROR_IO_PENDING ) )
        {
            sPending = TRUE;
        }
        else
        {
            IDE_TEST( sErr != ERROR_BROKEN_PIPE );
            
            // sErr should be ERROR_BROKEN_PIPE
            // return SUCCESS but set numByteRead = 0
            sPending = FALSE;
        }
    }
    
    if( sPending == TRUE )
    {
        while( numByteRead < aBufferSize )
        {
            sWait = WaitForSingleObject( aSock.mOverlap->hEvent, sWaitMsec );        
            sErr = GetLastError();
            
            // sErr can be BROKEN_PIPE
            if( sErr == ERROR_BROKEN_PIPE )
            {
                break;
            }
            else
            {
                // Nothing to do..
            }

            if( sWait == WAIT_TIMEOUT )
            {
                *aIsTimeout = ID_TRUE;
                break;
            }
            else
            {
                sResult = GetOverlappedResult( 
                            aSock.mHandle,      // pipe handle
                            aSock.mOverlap,     // OVERLAPPED structure 
                            &sTransferred,      // bytes transferred 
                            FALSE );            // do not wait
                        
                // if (pending condition)
                if( ( sResult == FALSE ) || sTransferred == 0 ) 
                {
                    continue;
                }
                else
                {
                    numByteRead += sTransferred;
                }
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    *aReceivedSize = numByteRead;
#else
    /* Same as recv() */
    PDL_UNUSED_ARG( aWaitTime  );
    PDL_UNUSED_ARG( aIsTimeout );
    IDE_TEST( recv( aSock, aBuffer, aBufferSize, aReceivedSize ) != IDE_SUCCESS );
#endif /* ALTI_CFG_OS_WINDOWS */
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::setBlockMode( IDX_LOCALSOCK aSock )
{
/***********************************************************************
 *
 * Description : 연결을 Blocking Mode로 전환한다.
 *               [PROJ-1685] Extproc Execution에는 Blocking Mode로만 수행
 *
 *       - Named Pipe는 사실 큰 의미가 없는데, Overlapped I/O는
 *         Blocking Mode와는 무관하게 작동하기 때문이다.
 *
 *       - Unix Domain Socket은 setBlock()을 호출한다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    DWORD dwMode = PIPE_WAIT;
    BOOL  result = SetNamedPipeHandleState( aSock.mHandle, &dwMode, NULL, NULL );
    IDE_TEST( !result );
#else
    IDE_TEST( idlVA::setBlock( aSock ) != IDE_SUCCESS );
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::setNonBlockMode( IDX_LOCALSOCK aSock )
{
/***********************************************************************
 *
 * Description : 연결을 Non-Blocking Mode로 전환한다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    DWORD dwMode = PIPE_NOWAIT;
    BOOL  result = SetNamedPipeHandleState( aSock.mHandle, &dwMode, NULL, NULL );
    IDE_TEST( !result );
#else
    IDE_TEST( idlVA::setNonBlock( aSock ) != IDE_SUCCESS );
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::close( IDX_LOCALSOCK *aSock )
{
/***********************************************************************
 *
 * Description : 연결을 닫는다.
 *
 *       - Named Pipe는 현재 가지고 있는 Pipe 객체에 서버/클라이언트
 *         구분이 없다. close() 함수는 Pipe 자체를 폐기하는 함수이다.
 *
 *       - Unix Domain Socket은 현재 가지고 있는 Socket 객체만 닫는다.
 *         즉, Client가 close()를 하더라도 Server Socket은 살아있다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    if( aSock->mHandle != PDL_INVALID_HANDLE )
    {
        CloseHandle( aSock->mHandle );
        aSock->mHandle = PDL_INVALID_HANDLE;
    }
    else
    {
        // Nothing to do.
    }
#else
    if( *aSock != PDL_INVALID_HANDLE )
    {
        IDE_TEST( idlOS::closesocket( *aSock ) != IDE_SUCCESS );
        *aSock = PDL_INVALID_HANDLE;
    }
    else
    {
        // Nothing to do.
    }
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idxLocalSock::disconnect( IDX_LOCALSOCK * aSock )
{
/***********************************************************************
 *
 * Description : 클라이언트 측 연결을 닫는다.
 *
 *       - Named Pipe는 현재 가지고 있는 Pipe 객체에 서버/클라이언트
 *         구분이 없다. disconnect() 함수는 접속된 클라이언트 연결만 끊는다.
 *
 *       - Unix Domain Socket은 현재 가지고 있는 Socket 객체만 닫는다.
 *         사실상, close() 함수와 동일하다.
 *
 ***********************************************************************/
#if defined(ALTI_CFG_OS_WINDOWS)
    BOOL  result;

    if( aSock->mHandle != PDL_INVALID_HANDLE )
    {
        result = DisconnectNamedPipe( aSock->mHandle );
        IDE_TEST( !result );
        aSock->mHandle = PDL_INVALID_HANDLE;
    }
    else
    {
        // Nothing to do.
    }
#else
    if( *aSock != PDL_INVALID_HANDLE )
    {
        IDE_TEST( idlOS::closesocket( *aSock ) != IDE_SUCCESS );
        *aSock = PDL_INVALID_HANDLE;
    }
    else
    {
        // Nothing to do.
    }
#endif /* ALTI_CFG_OS_WINDOWS */

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
