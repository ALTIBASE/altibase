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
 * $Id: extprocAgent.cpp 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#include <idx.h>
#include <idp.h>
#include <mmErrorCode.h>

/* For message log of extproc agent processes */
/* Path : $ALTIBASE_HOME/trc/extprocAgent.log */
#define IDX_AGENT_MSGLOG_FILE               PRODUCT_PREFIX"extprocAgent.log"
#define IDX_AGENT_MSGLOG_FILE_PATH_MAXLEN   IDX_SOCK_PATH_MAXLEN
#define IDX_AGENT_MSGLOG_FILESIZE           10485760 // 10 * 1024 * 1024 (10M) 
#define IDX_AGENT_MSGLOG_MAX_FILENUM        10
#define EXTPROC_AGENT_TIMESTAMP_BUF_SIZE    20  // YY-MM-DD hh:mm:ss

/* Send error code if condition is true */
#define EXTPROC_AGENT_SEND_ERR_MSG(cond, errcode)                               \
    do {                                                                        \
        if(cond)                                                                \
        {                                                                       \
            idxProc::packExtProcErrorMsg( &sMemory,                             \
                                          errcode,                              \
                                          &sErrorBuffer );                      \
            IDE_TEST_RAISE( idxLocalSock::sendTimedWait( sClientSock,           \
                                                         sErrorBuffer,          \
                                                         *(UInt*)sErrorBuffer   \
                                                          + idlOS::align8( ID_SIZEOF(UInt) ),  \
                                                         sWaitTime,             \
                                                         &sIsTimeout )          \
                                != IDE_SUCCESS,                                 \
                            err_conn_send_msg_failure );                        \
            goto EXTPROC_FINISH;                                                \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            /* Nothing to do.  */                                               \
        }                                                                       \
    } while(0); 
    
/* Print a error message log into extprocAgent.log */
void
agentErrorLog( ideMsgLog * aMsgLog, SChar * aSockName, const SChar * aForm, ... )
{
    va_list         sVaList;

    PDL_Time_Value  sTimevalue;
    time_t          sTime;
    struct tm       sLocalTime;
    SChar           sTimeBuf[EXTPROC_AGENT_TIMESTAMP_BUF_SIZE];

    SChar         * sSessionIDStr;

    // Time
    sTimevalue = idlOS::gettimeofday();
    sTime      = (time_t) sTimevalue.sec();
    idlOS::localtime_r(&sTime, &sLocalTime);

    idlOS::snprintf( sTimeBuf, 
                     ID_SIZEOF(sTimeBuf),
                     "%4"ID_UINT32_FMT
                     "/%02"ID_UINT32_FMT
                     "/%02"ID_UINT32_FMT
                     " %02"ID_UINT32_FMT
                     ":%02"ID_UINT32_FMT
                     ":%02"ID_UINT32_FMT"",
                     sLocalTime.tm_year + 1900,
                     sLocalTime.tm_mon  + 1,
                     sLocalTime.tm_mday,
                     sLocalTime.tm_hour,
                     sLocalTime.tm_min,
                     sLocalTime.tm_sec );

    // Session ID
    sSessionIDStr = idlOS::strrchr( aSockName, '_' );

    // Print
    va_start( sVaList, aForm );

    (void) aMsgLog->open();

    ideLogEntry sLog( aMsgLog, ACP_TRUE, 0 );
    (void) sLog.appendFormat( "[%s]", sTimeBuf );
    (void) sLog.appendFormat( "[Session-%s] ", &sSessionIDStr[1] );
    (void) sLog.appendArgs( aForm, sVaList );
    (void) sLog.append( "\n" );
    sLog.write();

    (void) aMsgLog->close();

    va_end( sVaList );
}

/* Close inherited handle (only for Unix/Linux) */
void
closeHandle()
{
#if !defined(ALTI_CFG_OS_WINDOWS)
    /* forked process inherits all opened FDs.
     * They should be closed. */
    SInt i = 0;

    /* Close all handles */
    for( i = ( PDL::max_handles() - 1 ); i >= 0; i--) 
    {
        PDL_OS::close(i); 
    }
#endif /* !ALTI_CFG_OS_WINDOWS */
}

int
main( int aArgc, char **aArgv )
{
    IDE_RC                sRC               = IDE_SUCCESS;
    SInt                  sState            = 0;

    iduMemory             sMemory;
    iduMemoryStatus       sMemoryPos;

    /* Sockets */
    IDX_LOCALSOCK         sServerSock;
    IDX_LOCALSOCK         sClientSock;

    /* for dynamic function call */
    PDL_SHLIB_HANDLE      sDl               = PDL_SHLIB_INVALID_HANDLE;
    void                * sFuncPtr          = NULL;
    void               ** sFuncArgs;
    void               ** sReturnArgPtr;
    void                * sReturnStrArg     = NULL;
    void                * sReturnOtherArg   = NULL;

    /* String, buffer */
    SChar                 sLibPath[IDX_LIB_PATH_MAXLEN];
    SChar                 sBackupLibName[IDX_LIB_PATH_MAXLEN];
    SChar               * sErrorBuffer      = NULL;

    SChar               * sRecvBuffer       = NULL;
    SChar               * sReturnBuffer     = NULL;
    SChar               * sBufferPtr        = NULL;
    UInt                  sBufferSize       = 0;
    SChar               * sEnvHome          = NULL;

    /* backup params */
    idxParamInfo        * sOriginalParams   = NULL;
    idxExtProcMsg       * sReturnMsg        = NULL;

    /* message log */
    ideMsgLog           * sAgentMsgLog      = NULL;
    SChar                 sLogFilePath[IDX_AGENT_MSGLOG_FILE_PATH_MAXLEN];

    UInt                  i                 = 0;
    UInt                  sNumByteRead      = 0;
    idBool                sIsTimeout        = ID_FALSE;
    PDL_Time_Value        sWaitTime;    

    /* Close inherited handle */
    closeHandle();

    idlOS::memset( sBackupLibName, 0, IDX_LIB_PATH_MAXLEN );

    IDE_TEST( aArgc != 2 );

    /**********************************************************************
     * 0. Intialization
     **********************************************************************/
    /* Initialize */
    IDE_TEST( iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) != IDE_SUCCESS );
    sState = 2;

    sEnvHome = idlOS::getenv(IDP_HOME_ENV);
    IDE_TEST( !(sEnvHome && idlOS::strlen(sEnvHome) > 0) );
    IDE_TEST( idp::initialize(sEnvHome, NULL) !=IDE_SUCCESS );
    sState = 3;
    IDE_TEST( iduProperty::load() != IDE_SUCCESS );

    IDE_TEST( ideLog::initializeStaticForUtil() != IDE_SUCCESS );
    sState = 4;

    sMemory.init(IDU_MEM_ID_EXTPROC_AGENT);
    sState = 5;

    // Set idle Timeout
    sWaitTime.set( IDU_EXTPROC_AGENT_IDLE_TIMEOUT, 0 );

    // Initialize Message Logging
    idlOS::snprintf(sLogFilePath, ID_SIZEOF(sLogFilePath),
                    "%s%c%s",
                    iduProperty::getServerMsglogDir(),
                    IDL_FILE_SEPARATOR,
                    IDX_AGENT_MSGLOG_FILE );

    IDE_TEST( sMemory.alloc( ID_SIZEOF( ideMsgLog ),
                             (void **)&sAgentMsgLog ) != IDE_SUCCESS );

    sRC = sAgentMsgLog->initialize( sLogFilePath,
                                    IDX_AGENT_MSGLOG_FILESIZE, 
                                    IDX_AGENT_MSGLOG_MAX_FILENUM ); 
    IDE_TEST( sRC != IDE_SUCCESS );
    sState = 6;

    /**********************************************************************
     * 1. Connection Establishment
     **********************************************************************/

    /* Socket Open */
    idxLocalSock::initializeStatic();
    IDE_TEST_RAISE( idxLocalSock::initializeSocket( &sServerSock, &sMemory ) != IDE_SUCCESS,
                    err_initialize_socket_failure );
    IDE_TEST_RAISE( idxLocalSock::initializeSocket( &sClientSock, &sMemory ) != IDE_SUCCESS,
                    err_initialize_socket_failure );

    /* Socket information must be preserved. */
    IDE_TEST_RAISE( sMemory.getStatus( &sMemoryPos ) != IDE_SUCCESS,
                    err_memory_problem );
    IDE_TEST_RAISE( idxLocalSock::socket( &sServerSock ) != IDE_SUCCESS,
                    err_conn_prepare_failure );

    // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
    IDE_TEST_RAISE( idxLocalSock::bind( &sServerSock, aArgv[1], &sMemory ) != IDE_SUCCESS,
                    err_conn_prepare_failure );
    sState = 7;

    IDE_TEST_RAISE( idxLocalSock::listen( sServerSock ) != IDE_SUCCESS,
                    err_conn_prepare_failure );
    IDE_TEST_RAISE( idxLocalSock::setBlockMode( sServerSock ) != IDE_SUCCESS,
                    err_conn_prepare_failure );

    IDE_TEST_RAISE( idxLocalSock::accept( sServerSock, &sClientSock, sWaitTime, &sIsTimeout ) 
                        != IDE_SUCCESS,
                    err_conn_wait_failure );
                
    if( sIsTimeout == ID_TRUE )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_CONN_TASK_TIMEOUT,
                       "Waiting process of connection" );
        IDE_RAISE( EXTPROC_EXIT );
    }
    else
    {
        IDE_TEST_RAISE( IDX_IS_CONNECTED( sClientSock ) == ID_FALSE, 
                        err_conn_wait_failure );
        sState = 8;
    }

    while(1)
    {
        /* Re-initialize */
        sReturnStrArg       = NULL;

        /**********************************************************************
         * 2. Receive Message
         **********************************************************************/
        IDE_TEST_RAISE( sMemory.cralloc( idlOS::align8( ID_SIZEOF(UInt) ),
                                         (void **)&sBufferPtr ) != IDE_SUCCESS,
                        err_memory_problem );

        // BUG-40719 extprocAgent wait forever.
        IDE_TEST_RAISE( idxLocalSock::select( sClientSock, 
                                              NULL,
                                              &sIsTimeout ) !=  IDE_SUCCESS,
                        err_conn_wait_msg_failure ); 

        IDE_TEST_RAISE( idxLocalSock::recvTimedWait( sClientSock,
                                                     (void*)sBufferPtr,
                                                     idlOS::align8( ID_SIZEOF(UInt) ), 
                                                     sWaitTime,
                                                     &sNumByteRead,
                                                     &sIsTimeout )
                            != IDE_SUCCESS,
                        err_conn_recv_msg_failure );

        // BUG-37957 : Connection close by peer, normal exit
        IDE_TEST_RAISE( sNumByteRead == 0, EXTPROC_EXIT );
        IDE_TEST_RAISE( sNumByteRead != idlOS::align8( ID_SIZEOF(UInt) ), 
                        err_conn_recv_msg_failure );

        if( sIsTimeout == ID_TRUE )
        {
            agentErrorLog( sAgentMsgLog, 
                           aArgv[1], 
                           MM_TRC_EXTPROC_CONN_TASK_TIMEOUT,
                           "Waiting process of receiving first message" );
            break;
        }
        else
        {
            /* Nothing to do. */
        }

        sBufferSize = *(UInt*)sBufferPtr;

        /* BUG-40916
         * sBufferSize
         * 0           : terminate agent
         * other       : execute external procedure */
        if ( sBufferSize == 0 )
        {
            IDE_TEST_RAISE( idxLocalSock::sendTimedWait( sClientSock,
                                                         sBufferPtr,
                                                         idlOS::align8( ID_SIZEOF(UInt) ),
                                                         sWaitTime,
                                                         &sIsTimeout )
                            != IDE_SUCCESS,
                            err_conn_send_msg_failure );

            if ( sIsTimeout == ID_TRUE )
            {
                agentErrorLog( sAgentMsgLog,
                               aArgv[1],
                               MM_TRC_EXTPROC_CONN_TASK_TIMEOUT,
                               "Sending process of ACK terminating" );
            }
            else
            {
                // Nothing to do.
            }

            /*
            agentErrorLog( sAgentMsgLog,
                           aArgv[1],
                           "An external procedure agent is terminated by request." );
            */
            IDE_CONT( EXTPROC_EXIT );
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST_RAISE( sMemory.alloc( sBufferSize,
                                       (void **)&sRecvBuffer ) != IDE_SUCCESS,
                        err_memory_problem ); 

        IDE_TEST_RAISE( idxLocalSock::recvTimedWait( sClientSock, 
                                                     (void*)sRecvBuffer, 
                                                     sBufferSize, 
                                                     sWaitTime,
                                                     &sNumByteRead,
                                                     &sIsTimeout )
                            != IDE_SUCCESS,
                        err_conn_recv_msg_failure );
        
        IDE_TEST_RAISE( sNumByteRead == 0, err_conn_recv_msg_failure );
        IDE_TEST_RAISE( sNumByteRead != sBufferSize, err_conn_recv_msg_failure );

        if( sIsTimeout == ID_TRUE )
        {
            agentErrorLog( sAgentMsgLog, 
                           aArgv[1], 
                           MM_TRC_EXTPROC_CONN_TASK_TIMEOUT,
                           "Waiting process of execution message" );
            break;
        }
        else
        {
            /* Nothing to do. */
        }

        /**********************************************************************
         * 3. Unpack Message
         **********************************************************************/
        IDE_TEST_RAISE( idxProc::unpackExtProcMsg( &sMemory, sRecvBuffer, &sReturnMsg )
                            != IDE_SUCCESS,
                        err_msg_unpack_failure );
        
        /* Back-up original properties to compare with changed information */
        if( sReturnMsg->mParamCount > 0 )
        {
            IDE_TEST_RAISE( sMemory.alloc( ID_SIZEOF(idxParamInfo) * sReturnMsg->mParamCount,
                                           (void **)&sOriginalParams ) != IDE_SUCCESS,
                        err_memory_problem ); 

            idxProc::backupParamProperty( sOriginalParams,
                                          sReturnMsg->mParamInfos,
                                          sReturnMsg->mParamCount );
        }
        else
        {
            /* Nothing to do. */
        }
        
        /**********************************************************************
         * 4. Loading & Preparing for function call
         **********************************************************************/
        if( idlOS::strcmp( sBackupLibName, sReturnMsg->mLibName ) != 0 )
        {
            /* New library is loading.. */

            /* Close previous handle */
            if( sDl != PDL_SHLIB_INVALID_HANDLE )
            {
                idlOS::dlclose( sDl );
            }
            else
            {
                /* Nothing to do. */
            }

            // BUG-44652 Socket file path of EXTPROC AGENT could be set by property.
            idlOS::snprintf( sLibPath,
                             IDX_LIB_PATH_MAXLEN,
                             "%s%s%s",
                             idxLocalSock::mHomePath,
                             IDX_LIB_DEFAULT_DIR,
                             sReturnMsg->mLibName );

            /* Check whether library exists */
            sDl = idlOS::dlopen( sLibPath, RTLD_LAZY );
            EXTPROC_AGENT_SEND_ERR_MSG( sDl == PDL_SHLIB_INVALID_HANDLE,
                                        idERR_ABORT_IDX_LIBRARY_NOT_FOUND );

            /* Backup library name for next comparison */
            idlOS::strcpy( sBackupLibName, sReturnMsg->mLibName );
        }
        else
        {
            /* Nothing to do. Just re-use opened handle */
        }
        
        /* Check whether two functions exist */
        sFuncPtr = idlOS::dlsym( sDl, sReturnMsg->mFuncName );
        EXTPROC_AGENT_SEND_ERR_MSG( sFuncPtr == NULL,
                                    idERR_ABORT_IDX_FUNCTION_NOT_FOUND );
        sFuncPtr = idlOS::dlsym( sDl, "entryfunction" );
        EXTPROC_AGENT_SEND_ERR_MSG( sFuncPtr == NULL,
                                    idERR_ABORT_IDX_ENTRY_FUNCTION_NOT_FOUND );

        if( sReturnMsg->mParamCount > 0 )
        {
            IDE_TEST_RAISE( sMemory.alloc( ID_SIZEOF(void*) * sReturnMsg->mParamCount,
                                           (void **)&sFuncArgs ) != IDE_SUCCESS,
                            err_memory_problem ); 

            /* Prepare argument values & count output arguments */
            for( i = 0; i < sReturnMsg->mParamCount; i++ )
            {
                if( sReturnMsg->mParamInfos[i].mPropType == IDX_TYPE_PROP_NONE )
                {
                    IDE_TEST_RAISE( idxProc::setParamPtr( &sReturnMsg->mParamInfos[i], &sFuncArgs[i] ) 
                                    != IDE_SUCCESS,
                                    err_set_paramptr_failure );
                }
                else
                {
                    switch( sReturnMsg->mParamInfos[i].mPropType )
                    {
                        case IDX_TYPE_PROP_IND:
                            sFuncArgs[i] = &sReturnMsg->mParamInfos[i].mIndicator;
                            break;
                        case IDX_TYPE_PROP_LEN:
                            sFuncArgs[i] = &sReturnMsg->mParamInfos[i].mLength;
                            break;
                        case IDX_TYPE_PROP_MAX:
                            sFuncArgs[i] = &sReturnMsg->mParamInfos[i].mMaxLength;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        else
        {
            /* Nothing to do. */
        }
        
        if( sReturnMsg->mReturnInfo.mSize > 0 )
        {
            if( sReturnMsg->mReturnInfo.mType != IDX_TYPE_CHAR )
            {
                idxProc::setParamPtr( &sReturnMsg->mReturnInfo, &sReturnOtherArg );
                sReturnArgPtr = &sReturnOtherArg;
            }
            else
            {
                sReturnArgPtr = &sReturnStrArg;
            }
        }
        else
        {
            sReturnOtherArg = NULL;
            sReturnArgPtr   = &sReturnOtherArg;
        }
        
        /**********************************************************************
         * 5. Calling the function
         **********************************************************************/
        ((void (*)( SChar*, ...))sFuncPtr)( sReturnMsg->mFuncName,
                                            sReturnMsg->mParamCount,
                                            sFuncArgs,
                                            sReturnArgPtr );
        
        /**********************************************************************
         * 6. Validation
         **********************************************************************/
        for( i = 0; i < sReturnMsg->mParamCount; i++ )
        {
            if( sReturnMsg->mParamInfos[i].mPropType != IDX_TYPE_PROP_NONE )
            {
                /* MAXLEN change */
                EXTPROC_AGENT_SEND_ERR_MSG( sReturnMsg->mParamInfos[i].mMaxLength
                                            != sOriginalParams[i].mMaxLength,
                                            idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );

                /* LENGTH change in non-CHAR type */
                EXTPROC_AGENT_SEND_ERR_MSG(
                    ( sReturnMsg->mParamInfos[i].mType != IDX_TYPE_CHAR )
                    && ( sReturnMsg->mParamInfos[i].mLength
                         != sOriginalParams[i].mLength ),
                    idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );

                /* LENGTH change in IN type */
                EXTPROC_AGENT_SEND_ERR_MSG(
                    ( sReturnMsg->mParamInfos[i].mMode == IDX_MODE_IN )
                    && ( sReturnMsg->mParamInfos[i].mLength
                         != sOriginalParams[i].mLength ),
                    idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );

                /* Changed LENGTH is more than MAXLEN */
                EXTPROC_AGENT_SEND_ERR_MSG(
                    ( sReturnMsg->mParamInfos[i].mLength
                      > sOriginalParams[i].mMaxLength ),
                    idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );

                /* minus LENGTH value */
                EXTPROC_AGENT_SEND_ERR_MSG(
                    sReturnMsg->mParamInfos[i].mLength < 0,
                    idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );

                /* INDICATOR change in IN mode */
                EXTPROC_AGENT_SEND_ERR_MSG(
                    ( sReturnMsg->mParamInfos[i].mMode == IDX_MODE_IN )
                    && ( sReturnMsg->mParamInfos[i].mIndicator
                         != sOriginalParams[i].mIndicator ),
                    idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );

                /* Invalid INDICATOR value */
                EXTPROC_AGENT_SEND_ERR_MSG(
                    ( sReturnMsg->mParamInfos[i].mIndicator != 0 )
                    && ( sReturnMsg->mParamInfos[i].mIndicator != 1 ),
                    idERR_ABORT_IDX_INVALID_PROPERTY_MANIPULATION );
            }
            else
            {
                /* Nothing to do. */
            }

            // BUG-39814 IN Mode CHAR/LOB don't have to re-send
            if ( ( sReturnMsg->mParamInfos[i].mMode == IDX_MODE_IN ) && 
                 ( ( sReturnMsg->mParamInfos[i].mType == IDX_TYPE_CHAR ) ||
                   ( sReturnMsg->mParamInfos[i].mType == IDX_TYPE_BYTE ) ||
                   ( sReturnMsg->mParamInfos[i].mType == IDX_TYPE_LOB ) ) )
            {
                // 1 byte for padding
                sReturnMsg->mParamInfos[i].mSize = idlOS::align8( ID_SIZEOF(idxParamInfo) + 1);
                sReturnMsg->mParamInfos[i].mD.mPointer = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
        
        if( sReturnMsg->mReturnInfo.mSize > 0 
            && sReturnMsg->mReturnInfo.mType == IDX_TYPE_CHAR )
        {
            // memcpy to real buffer
            if( sReturnStrArg != NULL )
            {
                idlOS::memcpy( sReturnMsg->mReturnInfo.mD.mPointer,
                               sReturnStrArg,
                               sReturnMsg->mReturnInfo.mMaxLength );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }
        
        /**********************************************************************
         * 7. Returning
         **********************************************************************/
        IDE_TEST_RAISE( idxProc::packExtProcMsg( &sMemory, sReturnMsg, &sReturnBuffer )
                            != IDE_SUCCESS,
                        err_msg_pack_failure );

        IDE_TEST_RAISE( idxLocalSock::sendTimedWait( sClientSock, 
                                                     sReturnBuffer, 
                                                     *(UInt*)sReturnBuffer + idlOS::align8( ID_SIZEOF(UInt) ), 
                                                     sWaitTime,
                                                     &sIsTimeout )
                           != IDE_SUCCESS,
                        err_conn_send_msg_failure );  

        if( sIsTimeout == ID_TRUE )
        {
            agentErrorLog( sAgentMsgLog, 
                           aArgv[1], 
                           MM_TRC_EXTPROC_CONN_TASK_TIMEOUT,
                           "Sending process of return message" );
            break;
        }
        else
        {
            /* Nothing to do. */
        }


EXTPROC_FINISH:
        /**********************************************************************
         * 6. Release spaces during the loop, and wait next message
         **********************************************************************/
        IDE_TEST_RAISE( sMemory.setStatus( &sMemoryPos ) != IDE_SUCCESS,
                        err_memory_problem );
    }

    /* Finalize */
EXTPROC_EXIT:   // label for connection time-out                       
    sState = 7;
    IDE_TEST_RAISE( idxLocalSock::disconnect( &sClientSock ) != IDE_SUCCESS,
                    err_conn_disconnect_failure );
    sState = 6;
    IDE_TEST_RAISE( idxLocalSock::close( &sServerSock ) != IDE_SUCCESS,
                    err_conn_disconnect_failure );
    sState = 5;
    IDE_TEST( sAgentMsgLog->destroy() != IDE_SUCCESS );
    sState = 4;
    sMemory.destroy();
    sState = 3;
    IDE_TEST( ideLog::destroyStaticForUtil() != IDE_SUCCESS );
    sState = 2;
    IDE_TEST( idp::destroy() != IDE_SUCCESS );
    sState = 1;


    IDE_TEST( iduMutexMgr::destroyStatic() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( iduMemMgr::destroyStatic() != IDE_SUCCESS );

    return 0;

    IDE_EXCEPTION( err_memory_problem )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MEM_PROBLEM );
    }
    IDE_EXCEPTION( err_initialize_socket_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_CONN_INIT_FAILED );
    }
    IDE_EXCEPTION( err_conn_prepare_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_CONN_OPEN_FAILED );
    }
    IDE_EXCEPTION( err_conn_wait_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_CONN_WAIT_FAILED );
    }
    IDE_EXCEPTION( err_conn_disconnect_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_DISCONN_FAILED );
    }
    IDE_EXCEPTION( err_conn_wait_msg_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MSG_WAIT_FAILED );
    }
    IDE_EXCEPTION( err_conn_send_msg_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MSG_SEND_FAILED );
    }
    IDE_EXCEPTION( err_conn_recv_msg_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MSG_RECV_FAILED );
    }
    IDE_EXCEPTION( err_msg_pack_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MSG_PACK_FAILED );
    }
    IDE_EXCEPTION( err_msg_unpack_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MSG_UNPACK_FAILED );
    }
    IDE_EXCEPTION( err_set_paramptr_failure )
    {
        agentErrorLog( sAgentMsgLog, 
                       aArgv[1], 
                       MM_TRC_EXTPROC_MSG_PARAM_SET_FAILED );
    }
    IDE_EXCEPTION_END;

    if( sDl != PDL_SHLIB_INVALID_HANDLE )
    {
        idlOS::dlclose( sDl );
    }
    else
    {
        /* Nothing to do. */
    }

    switch( sState )
    {
        case 8:
            idxLocalSock::disconnect( &sClientSock );
            /*@fallthrough@*/
        case 7:
            idxLocalSock::close( &sServerSock );
            /*@fallthrough@*/
        case 6:
            sAgentMsgLog->destroy();
            /*@fallthrough@*/
        case 5:
            sMemory.destroy();
            /*@fallthrough@*/
        case 4:
            ideLog::destroyStaticForUtil();
        case 3:
            idp::destroy();
            /*@fallthrough@*/
        case 2:
            iduMutexMgr::destroyStatic();
            /*@fallthrough@*/
        case 1:
            iduMemMgr::destroyStatic();
            /*@fallthrough@*/
        default:
           break;
    }

    return -1;
}
