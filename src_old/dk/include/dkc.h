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

#ifndef _O_DKC_H_
#define _O_DKC_H_ 1

#define DKC_DBLINK_CONF_FILE            (SChar *)       \
    "conf"IDL_FILE_SEPARATORS"dblink.conf"

/*
 * TODO: dafault, min and max value have to be modified.
 */

#define DKC_ALTILINKER_ENABLE_DEFAULT                       ( 0 )
#define DKC_ALTILINKER_ENABLE_MIN                           ( 0 )
#define DKC_ALTILINKER_ENABLE_MAX                           ( 1 )

#define DKC_ALTILINKER_PORT_NO_DEFAULT                      ( 20500 )
#define DKC_ALTILINKER_PORT_NO_MIN                          ( 1024 )
#define DKC_ALTILINKER_PORT_NO_MAX                          ( 65535 )

#define DKC_ALTILINKER_RECEIVE_TIMEOUT_DEFAULT              ( 300 )
#define DKC_ALTILINKER_RECEIVE_TIMEOUT_MIN                  ( 0 )
#define DKC_ALTILINKER_RECEIVE_TIMEOUT_MAX                  ( ID_SINT_MAX )

#define DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_DEFAULT  ( 100 )
#define DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_MIN      ( 0 )
#define DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_MAX      ( ID_SINT_MAX )

#define DKC_ALTILINKER_QUERY_TIMEOUT_DEFAULT                ( 100 )
#define DKC_ALTILINKER_QUERY_TIMEOUT_MIN                    ( 0 )
#define DKC_ALTILINKER_QUERY_TIMEOUT_MAX                    ( ID_SINT_MAX )

#define DKC_ALTILINKER_NON_QUERY_TIMEOUT_DEFAULT            ( 100 )
#define DKC_ALTILINKER_NON_QUERY_TIMEOUT_MIN                ( 0 )
#define DKC_ALTILINKER_NON_QUERY_TIMEOUT_MAX                ( ID_SINT_MAX )

#define DKC_ALTILINKER_THREAD_COUNT_DEFAULT                 ( 16 )
#define DKC_ALTILINKER_THREAD_COUNT_MIN                     ( 0 )
#define DKC_ALTILINKER_THREAD_COUNT_MAX                     ( 65535 )

#define DKC_ALTILINKER_THREAD_SLEEP_TIME_DEFAULT            ( 200 )
#define DKC_ALTILINKER_THREAD_SLEEP_TIME_MIN                ( 0 )
#define DKC_ALTILINKER_THREAD_SLEEP_TIME_MAX                ( ID_SINT_MAX )

#define DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_DEFAULT    ( 128 )
#define DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_MIN        ( 0 )
#define DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_MAX        ( 65535 )

#define DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_DEFAULT          ( 10 )
#define DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_MIN              ( 10 )
#define DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_MAX              ( ID_SINT_MAX )

#define DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_DEFAULT         ( 10 )
#define DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_MIN             ( 1 )
#define DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_MAX             ( 65535 )

#define DKC_ALTILINKER_TRACE_LOGGING_LEVEL_DEFAULT          ( 2 )
#define DKC_ALTILINKER_TRACE_LOGGING_LEVEL_MIN              ( 0 )
#define DKC_ALTILINKER_TRACE_LOGGING_LEVEL_MAX              ( 6 )

#define DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_DEFAULT    ( 128 )
#define DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_MIN        ( 128 )
#define DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_MAX        ( 4096 )

#define DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_DEFAULT     ( 4096 )
#define DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_MIN         ( 512 )
#define DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_MAX         ( 32*1024 )

#define DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_DEFAULT     ( 1 )
#define DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_MIN         ( 0 )
#define DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_MAX         ( 1 )

/*
 *
 */ 
typedef struct dkcDblinkConfTarget {

    SChar * mName;

    SChar * mJdbcDriver;

    SChar * mJdbcDriverClassName;

    SChar * mConnectionUrl;

    SChar * mUser;

    SChar * mPassword;

    SChar * mXADataSourceClassName;

    SChar * mXADataSourceUrlSetterName;

    dkcDblinkConfTarget * mNext;
    
} dkcDblinkConfTarget;

/*
 *
 */ 
typedef struct dkcDblinkConf {

    SInt mAltilinkerEnable;     /* 0: Disable or 1: Active */

    SInt mAltilinkerPortNo;

    SInt mAltilinkerReceiveTimeout; /* Seconds */

    SInt mAltilinkerRemoteNodeReceiveTimeout; /* Seconds */
    
    SInt mAltilinkerQueryTimeout; /* Seconds */
    
    SInt mAltilinkerNonQueryTimeout; /* Seconds */

    SInt mAltilinkerThreadCount;

    SInt mAltilinkerThreadSleepTime; /* Micro seconds */

    SInt mAltilinkerRemoteNodeSessionCount;

    SChar * mAltilinkerTraceLogDir;
    
    SInt mAltilinkerTraceLogFileSize; /* Mega bytes */

    SInt mAltilinkerTraceLogFileCount;

    /*
     * 0 - OFF, 1 - FATAL, 2 - ERROR, 3 - WARNING,
     * 4 - INFO, 5 - DEBUG, 6 - TRACE
     * */
    SInt mAltilinkerTraceLoggingLevel; 

    SInt mAltilinkerJvmMemoryPoolInitSize; /* */
    
    SInt mAltilinkerJvmMemoryPoolMaxSize;

    /*
     * 0 - 32Bit, 1 - 64Bit
     * */
    SInt mAltilinkerJvmBitDataModelValue;
    
    dkcDblinkConfTarget * mTargetList;
    
} dkcDblinkConf;

extern IDE_RC dkcLoadDblinkConf( dkcDblinkConf ** aDblinkConf );

extern IDE_RC dkcFreeDblinkConf( dkcDblinkConf * aDblinkConf );

#endif /* _O_DKC_H_ */
