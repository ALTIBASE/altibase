/***********************************************************************
 * Copyright 1999-2013, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idxProc.cpp 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

/***********************************************************************
 * BUG-40819 Remove latch for managing external procedure agent list.
 * 1. Replace linked list with array to manage agents.
 *    (linked list -> array)
 * 2. Remove latch for managing agents.
 *    (no latch)
 * 3. In callExtProc(), retry getAgentProcess() when failed to sending
 *    a message in 1st phase.
 *    ( getAgentProcess() <ok> -> send <fail> 
 *            ^                          |
 *      retry |__________________________| )
 * 4. Detailed error message (for send/receive) is added.
 **********************************************************************/

#include <idx.h>

idxAgentProc * idxProc::mAgentProcList = NULL;
SChar          idxProc::mAgentPath[IDX_AGENT_PATH_MAXLEN];
SChar        * idxProc::mTempBuffer = NULL;

IDE_RC
idxProc::initializeStatic()
{
    UInt    sListCount;
    UInt    sLen;
    UInt    i;
    SChar * sAgentPath = idxProc::mAgentPath;
    size_t const AGENT_PATH_SIZE = sizeof( idxProc::mAgentPath );

    idxLocalSock::initializeStatic();

    // session 개수만큼 agent list를 생성한다.
    // BUG-40945 session id max = MAX_CLIENT
    //                          + JOB_THREAD_COUNT
    //                          + CONC_EXEC_DEGREE_MAX
    //                          + 1 (SYSDBA)
    // session id는 1부터 시작하기 때문에 추가로 1을 더한다.
    sListCount = iduProperty::getMaxClient()
                 + iduProperty::getJobThreadCount()
                 + iduProperty::getConcExecDegreeMax()
                 + 2;

    IDE_ASSERT( iduMemMgr::calloc( IDU_MEM_ID_EXTPROC,
                                   sListCount,
                                   ID_SIZEOF(idxAgentProc),
                                   (void **)&mAgentProcList,
                                   IDU_MEM_IMMEDIATE )
                == IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_ID_EXTPROC,
                                   idlOS::align8(ID_SIZEOF(UInt)) * sListCount,
                                   (void**)&mTempBuffer)
                == IDE_SUCCESS );

    for ( i = 0; i < sListCount; i++ )
    {
        mAgentProcList[i].mState = IDX_PROC_ALLOC;

        idlOS::snprintf( mAgentProcList[i].mSockFile,
                         ID_SIZEOF(mAgentProcList[i].mSockFile),
                         "%s_%d",
                         IDX_SOCK_NAME_PREFIX,
                         i );

        mAgentProcList[i].mTempBuffer = (SChar*)(mTempBuffer + (i * idlOS::align8(ID_SIZEOF(UInt))));
    }

    idlOS::strncpy( sAgentPath, idxLocalSock::mHomePath, AGENT_PATH_SIZE );
    sLen = idlOS::strlen( idxLocalSock::mHomePath );
    IDE_TEST_RAISE( sLen >= AGENT_PATH_SIZE, err_buffer_overflow );

    idlOS::strncpy( (sAgentPath + sLen), IDX_AGENT_DEFAULT_DIR, AGENT_PATH_SIZE - sLen );
    sLen += idlOS::strlen( IDX_AGENT_DEFAULT_DIR );
    IDE_TEST_RAISE( sLen >= AGENT_PATH_SIZE, err_buffer_overflow );

    idlOS::strncpy( (sAgentPath + sLen), IDX_AGENT_PROC_NAME, AGENT_PATH_SIZE - sLen );
    sLen += idlOS::strlen( IDX_AGENT_PROC_NAME );
    IDE_TEST_RAISE( sLen >= AGENT_PATH_SIZE, err_buffer_overflow );

    sAgentPath[sLen] = '\0';
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_buffer_overflow )
        ;

    IDE_EXCEPTION_END
        return IDE_FAILURE;
}

void
idxProc::destroyStatic()
{
    IDE_ASSERT( iduMemMgr::free( mTempBuffer ) == IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::free( mAgentProcList ) == IDE_SUCCESS );

    idxLocalSock::destroyStatic();
}

IDE_RC
idxProc::createAgentProcess( UInt            aSessionID,
                             idxAgentProc ** aAgentProc, 
                             iduMemory     * aExeMem )
{
/***********************************************************************
 *
 *  Description : Agent Process를 생성하고, 연결을 맺는다.
 *
 *  - 생성 과정에서 실패하면 START FAILURE 에러
 *  - 연결 과정에서 실패하면 CONNECTION FAILURE 에러
 *  - 연결 시간 초과로 연결이 실패하면 CONNECTION TIMEOUT 에러
 *    (이 경우, EXTPROC_AGENT_CONNECT_TIMEOUT을 조정해야 한다.)
 *
 ***********************************************************************/

    idxAgentProc  * sNewNode        = NULL;
    pid_t           sChildProc      = 0;
    UInt            sState          = 0;
    UInt            sConnectCoin    = 0;
    SInt            sAliveCode      = 0;
    IDE_RC          sRC             = IDE_SUCCESS;

    SChar         * sArgv[3];
    SChar           sSockPath[IDX_SOCK_PATH_MAXLEN];
    PDL_Time_Value  sTimevalue;
    PDL_Time_Value  sSleepTime;
    SChar         * sAgentPath = idxProc::mAgentPath;

    idBool          sIsRefused = ID_FALSE;

    /**********************************************************************************
     * 1. Alloc & Initialize
     *********************************************************************************/

    sNewNode = &(mAgentProcList[aSessionID]);

    idxLocalSock::setPath( sSockPath, sNewNode->mSockFile, ID_FALSE );

    /* BUG-40538 agent file check */
    IDE_TEST_RAISE( idlOS::access( sAgentPath, R_OK | X_OK ) == -1,
                    err_fail_to_create_agent_proc );

    /* If socket file is exist.. delete. */
    if( 0 == idlOS::access( sSockPath, F_OK ) ) 
    {
        /* if unlink is failed, then */
        idlOS::unlink( sSockPath );
    }
    else
    {
        /* Nothing to do. */
    }

    /**********************************************************************************
     * 2. Fork_exec
     *********************************************************************************/

    sArgv[0] = sAgentPath;
    sArgv[1] = sSockPath;
    sArgv[2] = NULL;

    sChildProc = idlOS::fork_exec( sArgv );
    IDE_TEST_RAISE( sChildProc == -1, err_fail_to_create_agent_proc );
    sAliveCode = PDL::process_active( sChildProc );
    IDE_TEST_RAISE( sAliveCode != 1, err_fail_to_create_agent_proc );
    sState = 1;

    sTimevalue              = idlOS::gettimeofday();
    sNewNode->mSID          = aSessionID;
    sNewNode->mPID          = sChildProc;
    sNewNode->mCreated      = sTimevalue.sec();
    sNewNode->mLastReceived = 0;
    sNewNode->mLastSent     = 0;
    sNewNode->mState        = IDX_PROC_INITED;

    idxLocalSock::initializeSocket( &sNewNode->mSocket, 
                                    IDU_MEM_ID_EXTPROC );

    /**********************************************************************************
     * 3. Connect
     *********************************************************************************/

    sSleepTime.set( 0, IDX_WAITTIME_PER_CONN * 1000 ); // wait X msec

    IDE_TEST_RAISE( idxLocalSock::socket( &sNewNode->mSocket )
                    != IDE_SUCCESS, err_agent_connection_failure );
    sState = 2;

    while( sConnectCoin < IDX_CONNECTION_MAX_COUNT )
    {
        sRC = idxLocalSock::connect( &sNewNode->mSocket, sSockPath, aExeMem, &sIsRefused );

        if( sRC != IDE_SUCCESS )
        {
            /* socket is not listened, it is not connected immediately,
             * or socket is not binding yet */
            idlOS::sleep( sSleepTime );

            // BUG-37957
            // If a connection has been refused, re-create socket
            if( sIsRefused == ID_TRUE )
            {
                // Call close() and socket().
                // It goes to fail if either of them returns FAIL.
                IDE_TEST_RAISE( idxLocalSock::close( &sNewNode->mSocket )
                                != IDE_SUCCESS, err_agent_connection_failure );
                IDE_TEST_RAISE( idxLocalSock::socket( &sNewNode->mSocket )
                                != IDE_SUCCESS, err_agent_connection_failure );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* Connected */
            IDE_TEST_RAISE( idxLocalSock::setBlockMode( sNewNode->mSocket ) != IDE_SUCCESS,
                            err_agent_connection_failure );
            break;
        }

        sConnectCoin++;
    }
    /* if we cannot make it after CONNECT_TIMEOUT, this becomes an error */
    IDE_TEST_RAISE( sRC != IDE_SUCCESS, err_agent_connection_time_out );

    *aAgentProc = sNewNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_to_create_agent_proc )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_PROCESS_FAILURE ) );
    }
    IDE_EXCEPTION( err_agent_connection_failure )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_CONNECTION_FAILURE ) );
    }
    IDE_EXCEPTION( err_agent_connection_time_out )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_CONNECTION_FAILURE,
                                  "Connection Time-out" ) );
    }
    IDE_EXCEPTION_END;

    *aAgentProc = NULL;

    switch( sState )
    {
        case 2:
            // close socket
            (void)idxLocalSock::close( &sNewNode->mSocket );
            /*@fallthrough*/
        case 1:
            // kill process 
            (void)PDL::terminate_process( sNewNode->mPID );
            PDL_OS::waitpid( sNewNode->mPID, NULL, 0 );
            /*@fallthrough*/
        default:
            sNewNode->mState = IDX_PROC_ALLOC;
            break;
    }

    return IDE_FAILURE;
}

// BUG-40916 Send a termination message to the agent.
IDE_RC idxProc::terminateProcess( idxAgentProc * aAgentProc )
{
    SChar * sBuffer;
    UInt    sNumByteRead;

    IDE_TEST_CONT( aAgentProc->mState == IDX_PROC_ALLOC,
                   SKIP_TERMINATE );

    sBuffer         = aAgentProc->mTempBuffer;
    *(UInt*)sBuffer = 0;

    IDE_TEST( idxLocalSock::send( aAgentProc->mSocket,
                                  sBuffer,
                                  idlOS::align8( ID_SIZEOF(UInt) ) )
         != IDE_SUCCESS );

    IDE_TEST( idxLocalSock::recv( aAgentProc->mSocket,
                                  (void*)sBuffer,
                                  idlOS::align8( ID_SIZEOF(UInt) ),
                                  &sNumByteRead )
              != IDE_SUCCESS );

    IDE_TEST( sNumByteRead != idlOS::align8( ID_SIZEOF(UInt) ) )

    ideLog::log(IDE_QP_0, ID_TRC_EXTPROC_PROC_TERMINATED, aAgentProc->mPID );

    IDE_EXCEPTION_CONT( SKIP_TERMINATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idxProc::destroyAgentProcess( UInt aSessionID )
{
    idxAgentProc * sNode = NULL;
    SChar          sSockPath[IDX_SOCK_PATH_MAXLEN];

    sNode = &(mAgentProcList[aSessionID]);

    if ( sNode->mState < IDX_PROC_ALLOC )
    {
        // BUG-40916 terminate process
        // * Just send a termination message to the agent.
        // * Ignore return value.
        (void)idxProc::terminateProcess( sNode );

        sNode->mState = IDX_PROC_ALLOC;

        /* close socket */
        (void)idxLocalSock::close( &sNode->mSocket );

        // BUG-37957 finalize socket area (named pipe event handler)
        (void)idxLocalSock::finalizeSocket( &sNode->mSocket );

        PDL_OS::waitpid( sNode->mPID, NULL, 0 );

        idxLocalSock::setPath( sSockPath, sNode->mSockFile, ID_FALSE );

        // BUG-44628 Remove socket file while close the session.
        /* If socket file is exist.. delete. */
        if( 0 == idlOS::access( sSockPath, F_OK ) ) 
        {
            /* if unlink is failed, then */
            (void)idlOS::unlink( sSockPath );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. : 프로세스/정보 모두 존재하지 않음 */
    }
}

IDE_RC idxProc::getAgentProcess( UInt            aSessionID,
                                 idxAgentProc ** aAgentProc,
                                 idBool        * aIsFirstTime )
{
/***********************************************************************
 *
 *  Description : Agent Process를 반환한다.
 *
 *   - 검색된 Agent Node가 존재하지 않으면 NULL
 *     ( sIsFirstTime = TRUE )
 * 
 *   - 검색된 Agent Node가 존재하지만, 프로세스가 존재하지 않으면 NULL
 *     ( sIsFirstTime = FALSE ) 
 *
 *   - 검색된 Agent Node가 존재하고, 프로세스도 존재하면 해당 Node 반환
 *
 ***********************************************************************/

    idxAgentProc * sNode    = NULL;
    idBool         sIsAlive = ID_TRUE;

    sNode = &(mAgentProcList[aSessionID]);

    if( sNode->mState == IDX_PROC_ALLOC )
    {
        /* 리스트에 정보가 없다면 새로운 프로세스 생성 */

        // 처음 생성해야 함을 표시
        *aIsFirstTime = ID_TRUE;
        *aAgentProc   = NULL;
    }
    else
    {
        /* 리스트에 정보가 있다면 프로세스 연결 확인 */
        IDE_TEST( idxLocalSock::ping( sNode->mPID,
                                      &sNode->mSocket,
                                      &sIsAlive ) != IDE_SUCCESS );

        if( sIsAlive == ID_FALSE )
        {
            sNode->mState = IDX_PROC_ALLOC;

            /* 프로세스는 존재하지 않지만, 리스트에는 정보가 남아있는 경우 */
            // force-close
            IDE_TEST( idxLocalSock::close( &sNode->mSocket ) != IDE_SUCCESS );

            // free the agent node
            (void)idxLocalSock::finalizeSocket( &sNode->mSocket );

            // 재생성임을 표시
            *aIsFirstTime = ID_FALSE;
            *aAgentProc   = NULL;
        }
        else
        {
            /* 프로세스가 존재하고, 관련 정보도 이미 리스트에 남아있는 경우 */

            /* 생성하거나 얻은 프로세스 정보를 반환 */
            *aAgentProc = sNode;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
idxProc::copyParamInfo( idxParamInfo * aSrcParam,
                        idxParamInfo * aDestParam )
{
    /* Property */
    aDestParam->mIndicator = aSrcParam->mIndicator;
    aDestParam->mLength    = aSrcParam->mLength;
    aDestParam->mMaxLength = aSrcParam->mMaxLength;

    /* Value */
    // BUG-39814
    // If you want to add LOB type as OUT mode or return type,
    // You should add some conditions below
    if( aDestParam->mMode != IDX_MODE_IN 
        && aDestParam->mPropType == IDX_TYPE_PROP_NONE )
    {
        if( aDestParam->mType == IDX_TYPE_CHAR )
        {
            idlOS::memcpy( aDestParam->mD.mPointer,
                           aSrcParam->mD.mPointer,
                           aDestParam->mMaxLength );
        }
        else
        {
            aDestParam->mD = aSrcParam->mD;
        }
    }
    else
    {
        /* Nothing to do. */
    }
}

IDE_RC
idxProc::packExtProcMsg( iduMemory      * aExeMem,
                         idxExtProcMsg  * aMsg,
                         SChar         ** aOutBuffer )
{
    idxParamInfo    * sStructObj      = NULL;
    SChar           * sMsgBuffer      = NULL;
    UInt              sParamSize      = 0;
    UInt              sTotalSize      = 0;
    UInt              i               = 0;

    idxExtProcMsg   * sDummyMsg       = NULL;
    UInt            * sMsgSize        = NULL;

    SChar           * sDummyBuffer    = NULL;

    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        sParamSize += aMsg->mParamInfos[i].mSize;
    }

    sParamSize += aMsg->mReturnInfo.mSize;

    sTotalSize = idlOS::align8( ID_SIZEOF(UInt) )
                 + idlOS::align8( ID_SIZEOF(idxExtProcMsg) )
                 + sParamSize;

    IDE_TEST( aExeMem->alloc( sTotalSize, (void **)&sMsgBuffer ) != IDE_SUCCESS );

    sMsgSize  = (UInt*)sMsgBuffer;
    *sMsgSize = sTotalSize - idlOS::align8( ID_SIZEOF(UInt) );

    sDummyMsg = (idxExtProcMsg*)( sMsgBuffer + idlOS::align8( ID_SIZEOF(UInt) ) );
    idlOS::memcpy( sDummyMsg, aMsg, ID_SIZEOF(idxExtProcMsg) );

    sParamSize = idlOS::align8( ID_SIZEOF(UInt) )
                 + idlOS::align8( ID_SIZEOF(idxExtProcMsg) );

    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        sStructObj = (idxParamInfo*)( sMsgBuffer + sParamSize );

        idlOS::memcpy( sStructObj, &aMsg->mParamInfos[i], ID_SIZEOF(idxParamInfo) );

        if( aMsg->mParamInfos[i].mPropType == 0 )
        {
            switch( aMsg->mParamInfos[i].mType )
            {
                case IDX_TYPE_CHAR:
                case IDX_TYPE_LOB:  // BUG-39814 IN mode LOB Parameter in Extproc
                case IDX_TYPE_BYTE: // PROJ-2717 Internal procedure

                    sStructObj->mD.mPointer = (void*)( sMsgBuffer +
                                                       sParamSize + 
                                                       idlOS::align8( ID_SIZEOF(idxParamInfo) ) );

                    if ( aMsg->mParamInfos[i].mD.mPointer != NULL )
                    {
                        idlOS::memcpy( sStructObj->mD.mPointer,
                                       aMsg->mParamInfos[i].mD.mPointer,
                                       ( aMsg->mParamInfos[i].mMaxLength + 1 ) );
                        ((SChar*)sStructObj->mD.mPointer)[aMsg->mParamInfos[i].mMaxLength] = '\0';
                    }
                    else
                    {
                        /* [ BUG-40219 ]
                         *
                         * 다음의 경우, aMsg->mParamInfos[i].mD.mPointer == NULL 이다.
                         * 모두, 값 영역을 보낼 필요가 없는 경우이다.
                         *
                         *  (1) IN Mode 이고 값이 없을 때, HDB에서 packing 하는 경우
                         *  (2) IN Mode 이고, Agent에서 packing 하는 경우
                         *
                         * 이 경우 메시지 버퍼에서의 값 영역 크기는, 가짜 값인 1 Byte이다.
                         * 이 영역을 Null-terminating 한다.
                         */
                        ((SChar*)sStructObj->mD.mPointer)[0] = '\0';
                    }
                    break;
                default:
                    break;
            }
        }
        else
        {
            /* Nothing to do. */
        }

        sParamSize += aMsg->mParamInfos[i].mSize;
    }

    /* return info. */
    if( sDummyMsg->mReturnInfo.mType == IDX_TYPE_CHAR )
    {
        sDummyBuffer = (SChar*)( sMsgBuffer + sParamSize );
        sDummyMsg->mReturnInfo.mD.mPointer = sDummyBuffer;
        idlOS::memcpy( sDummyMsg->mReturnInfo.mD.mPointer,
                       aMsg->mReturnInfo.mD.mPointer,
                       aMsg->mReturnInfo.mMaxLength );
    }
    else
    {
        sDummyMsg->mReturnInfo.mD = aMsg->mReturnInfo.mD;
    }

    *aOutBuffer  = sMsgBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
idxProc::packExtProcErrorMsg( iduMemory      * aExeMem,
                              UInt             aErrorCode,
                              SChar         ** aOutBuffer )
{
    SChar         * sMsgBuffer = NULL;
    UInt            sTotalSize = 0;
    idxExtProcMsg * sDummyMsg  = NULL;

    sTotalSize = idlOS::align8( ID_SIZEOF(UInt) )
                 + idlOS::align8( ID_SIZEOF(idxExtProcMsg) );

    IDE_TEST( aExeMem->alloc( idlOS::align8( sTotalSize ),
                              (void **)&sMsgBuffer ) != IDE_SUCCESS );

    *(UInt*)sMsgBuffer = idlOS::align8( sTotalSize ) - idlOS::align8( ID_SIZEOF(UInt) );

    sDummyMsg = (idxExtProcMsg *)( sMsgBuffer + idlOS::align8( ID_SIZEOF(UInt) ) );
    sDummyMsg->mErrorCode = aErrorCode;

    *aOutBuffer  = sMsgBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
idxProc::unpackExtProcMsg( iduMemory      * aExeMem,
                           SChar          * aInBuffer,
                           idxExtProcMsg ** aMsg )
{
    UInt        i             = 0;
    UInt        sTotalSize    = 0;
    SChar     * sDataPtr      = NULL;

    idxParamInfo   * sRetStructArray = NULL;
    idxParamInfo   * sStruct     = NULL;
    idxExtProcMsg  * sRecvMsg    = NULL;

    SChar          * sTempBuf    = NULL;

    sRecvMsg = (idxExtProcMsg*)aInBuffer;
    IDE_TEST_RAISE( sRecvMsg->mErrorCode != 0, err_agent_error_issued );

    sTotalSize = idlOS::align8( ID_SIZEOF(idxExtProcMsg) );

    if( sRecvMsg->mParamCount > 0 )
    {
        IDE_TEST( aExeMem->alloc( idlOS::align8( ID_SIZEOF(idxParamInfo) ) * sRecvMsg->mParamCount,
                                  (void **)&sRetStructArray ) != IDE_SUCCESS );

        for( i = 0; i < sRecvMsg->mParamCount; i++ )
        {
            sStruct = (idxParamInfo*)(aInBuffer + sTotalSize);

            idlOS::memcpy( &sRetStructArray[i], sStruct, idlOS::align8( ID_SIZEOF(idxParamInfo) ) );

            if( sStruct->mPropType == 0 )
            {
                switch( sStruct->mType )
                {
                    case IDX_TYPE_CHAR:
                    case IDX_TYPE_LOB:  // BUG-39814 IN mode LOB Parameter in Extproc
                    case IDX_TYPE_BYTE: // PROJ-2717 Internal procedure

                        IDE_DASSERT( sStruct->mSize >= idlOS::align8( ID_SIZEOF(idxParamInfo) ) );
                        sDataPtr   = (SChar*)sStruct + idlOS::align8( ID_SIZEOF(idxParamInfo) );

                        /* [ BUG-40195 ]
                         *
                         * Unpacking 과정에서 CHAR/LOB Value를 받기 위해 다시 할당받는 부분을 삭제.
                         * 대신, 수신된 메시지 버퍼의 CHAR/LOB Value 영역을 호출 과정에서 그대로 사용한다.
                         * 수신된 메시지 버퍼는 호출 과정에서 해제되지 않는다.
                         *
                         * [ BUG-40219 ]
                         * 
                         * Packing 과정에서 Parameter Pointer가 NULL인 Parameter가 존재하더라도,
                         * 메시지 버퍼에는 Pointer가 NULL인 Parameter는 없다. (가짜 값 1 Byte)
                         *
                         * 따라서, unpacking 과정에서는 구별 없이
                         * 진짜 값 영역 또는 가짜 값 1 byte를 Pointer가 가리키도록 한다.
                         */
                        sRetStructArray[i].mD.mPointer = sDataPtr;
                    default:
                        break;
                }
            }

            sTotalSize += sStruct->mSize;
        }
    }
    else
    {

    }

    // RETURN
    if( sRecvMsg->mReturnInfo.mType == IDX_TYPE_CHAR )
    {
        sRecvMsg->mReturnInfo.mD.mPointer = (SChar*)( aInBuffer + sTotalSize );

        IDE_TEST( aExeMem->alloc( sRecvMsg->mReturnInfo.mSize + 1,
                                  (void **)&sTempBuf ) != IDE_SUCCESS );

        idlOS::memcpy( sTempBuf,
                       sRecvMsg->mReturnInfo.mD.mPointer,
                       sRecvMsg->mReturnInfo.mSize );

        sTempBuf[sRecvMsg->mReturnInfo.mSize] ='\0';

        sRecvMsg->mReturnInfo.mD.mPointer = sTempBuf;
    }
    else
    {
        /* Nothing to do. */
    }

    sRecvMsg->mParamInfos = sRetStructArray;

    *aMsg = sRecvMsg;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_agent_error_issued )
    {
        /* return error message */
        *aMsg = sRecvMsg;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
idxProc::callExtProc( iduMemory      * aExeMem,
                      UInt             aSessionID,
                      idxExtProcMsg  * aMsg )
{
    idxAgentProc      * sAgentProc      = NULL;
    idxExtProcMsg     * sRecvMsg        = NULL;
    IDX_LOCALSOCK       sSock;
    UInt                i               = 0;

    SChar             * sSendBuffer     = NULL;
    SChar             * sRecvBuffer     = NULL;
    UInt                sRecvBufferSize = 0;
    UInt                sNumByteRead    = 0;
    idBool              sIsFirstTime    = ID_FALSE;
    PDL_Time_Value      sTimevalue;
    UInt                sStage          = 0;

    IDE_RC              sRc             = IDE_FAILURE;
    UInt                sRetryCount     = 0;
    UInt                sRetryCountProp = 0;

    /**********************************************************************************
     * 1. Get - (Re)Create Agent Process
     *********************************************************************************/

IDE_EXCEPTION_CONT( RETRY_GET_AGENT );
    IDE_TEST_RAISE( getAgentProcess( aSessionID, 
                                     & sAgentProc, 
                                     & sIsFirstTime ) != IDE_SUCCESS,
                    err_fail_to_get_agent_proc );

    if ( sAgentProc == NULL )
    {
        // createAgentProcess()는 내부에서 에러를 반환한다.
        IDE_TEST( createAgentProcess( aSessionID,
                                      & sAgentProc, 
                                      aExeMem ) != IDE_SUCCESS );

        if ( sIsFirstTime == ID_TRUE )
        {
            ideLog::log(IDE_QP_0, ID_TRC_EXTPROC_PROC_CREATED, sAgentProc->mPID );
        }
        else // sIsFirstTime == ID_FALSE
        {
            ideLog::log(IDE_QP_0, ID_TRC_EXTPROC_PROC_RECREATED, sAgentProc->mPID );
        }
    }
    else
    {
        // Nothing to do.
    }

    /**********************************************************************************
     * 2. Pack a calling message & Send it
     *********************************************************************************/

    if ( sRetryCount == 0 )
    {
        IDE_TEST_RAISE( packExtProcMsg( aExeMem, aMsg, &sSendBuffer ) != IDE_SUCCESS,
                        err_fail_to_call_function );
    }
    else
    {
        // 이전에 packing한 message를 재사용한다.
    }

    sSock = sAgentProc->mSocket;

    sTimevalue = idlOS::gettimeofday();
    sAgentProc->mLastReceived = sTimevalue.sec();
    sAgentProc->mState        = IDX_PROC_RUNNING;

    sRc = idxLocalSock::send( sSock, 
                              (void*)sSendBuffer, 
                              *(UInt*)sSendBuffer + idlOS::align8( ID_SIZEOF(UInt) ) );

    if ( sRc == IDE_FAILURE )
    {
        if ( sRetryCountProp == 0 ) 
        {
            sRetryCountProp = iduProperty::getExtprocAgentCallRetryCount();
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST_RAISE( sRetryCount >= sRetryCountProp,
                        err_agent_communication_failure_send );
        ideLog::log(IDE_QP_0, "RETRY_GET_AGENT RETRY_COUNT[<%d>] PID[<%d>]", sRetryCount, sAgentProc->mPID );
        sRetryCount++;
        sAgentProc   = NULL;
        sIsFirstTime = ID_FALSE;
        IDE_CONT( RETRY_GET_AGENT );
    }
    else
    {
        // Nothing to do.
    }
    sStage = 1;

    /**********************************************************************************
     * 3. Receive a returning message & Unpack it
     *********************************************************************************/

    sAgentProc->mLastSent = sTimevalue.sec();

    sRecvBufferSize = idlOS::align8( ID_SIZEOF(UInt) );

    IDE_TEST_RAISE( idxLocalSock::recv( sSock,
                                        sAgentProc->mTempBuffer,
                                        sRecvBufferSize,
                                        &sNumByteRead )
                    != IDE_SUCCESS, err_agent_communication_failure_recv );
    sStage = 2;

    // BUG-37957 : Connection close by peer
    IDE_TEST_RAISE( ( sNumByteRead == 0 ) || ( sNumByteRead != sRecvBufferSize ), 
                    err_agent_communication_failure_close_by_peer );
    sStage = 3;

    sRecvBufferSize = *(UInt*)(sAgentProc->mTempBuffer);

    IDE_TEST( aExeMem->alloc( sRecvBufferSize,
                              (void **)&sRecvBuffer ) != IDE_SUCCESS );

    IDE_TEST_RAISE( idxLocalSock::recv( sSock,
                                        (void*)sRecvBuffer,
                                        sRecvBufferSize,
                                        &sNumByteRead )
                    != IDE_SUCCESS, err_agent_communication_failure_recv );
    sStage = 4;

    // BUG-37957 : Connection close by peer
    IDE_TEST_RAISE( ( sNumByteRead == 0 ) || ( sNumByteRead != sRecvBufferSize ), 
                    err_agent_communication_failure_close_by_peer );
    sStage = 5;

    IDE_TEST_RAISE( unpackExtProcMsg( aExeMem, sRecvBuffer, &sRecvMsg ) != IDE_SUCCESS,
                    err_fail_to_get_return_value );

    /**********************************************************************************
     * 4. Copy Parameter/Return values
     *********************************************************************************/

    /* Copy parameters & return information */
    for( i = 0; i < aMsg->mParamCount; i++ )
    {
        copyParamInfo( &sRecvMsg->mParamInfos[i], &aMsg->mParamInfos[i] );
    }

    copyParamInfo( &sRecvMsg->mReturnInfo, &aMsg->mReturnInfo );

    /* 상태를 STOPPED로 변경 */
    sAgentProc->mState = IDX_PROC_STOPPED;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_fail_to_get_agent_proc )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_PROCESS_FAILURE ) );
    }
    IDE_EXCEPTION( err_fail_to_call_function )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_CALL_PROCEDURE_FAILURE ) );
    }
    IDE_EXCEPTION( err_fail_to_get_return_value )
    {
        if( sRecvMsg != NULL )
        {
            /* Path of library must be shown if relevant error code is set. */
            if( sRecvMsg->mErrorCode == idERR_ABORT_IDX_LIBRARY_NOT_FOUND )
            {
                IDE_SET( ideSetErrorCode( sRecvMsg->mErrorCode, 
                                          idxLocalSock::mHomePath,
                                          IDX_LIB_DEFAULT_DIR,
                                          aMsg->mLibName ) );
            }
            else
            {
                IDE_SET( ideSetErrorCode( sRecvMsg->mErrorCode ) );
            }
        }
        else
        {
            /* 메시지 변환 중 에러 */
            IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_CALL_PROCEDURE_FAILURE ) );
        }
    }
    IDE_EXCEPTION( err_agent_communication_failure_recv )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_CONNECTION_LOST_WHILE_RECV,
                                  sStage ) );
    }
    IDE_EXCEPTION( err_agent_communication_failure_send )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_CONNECTION_LOST_WHILE_SEND,
                                  sStage ) );
    }
    IDE_EXCEPTION( err_agent_communication_failure_close_by_peer )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDX_AGENT_CONNECTION_LOST_BY_PEER,
                                  sStage, sNumByteRead, sRecvBufferSize ) );
    }
    IDE_EXCEPTION_END;

    /* 상태를 FAILED로 변경 */
    if ( sAgentProc != NULL )
    {
        sAgentProc->mState = IDX_PROC_FAILED;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}
