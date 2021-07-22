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

#include <ide.h>
#include <idl.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkc.h>

#include <dkcUtil.h>

/*
 *
 */
void dkcUtilInitializeDblinkConf( dkcDblinkConf * aDblinkConf )
{
    aDblinkConf->mAltilinkerEnable = DKC_ALTILINKER_ENABLE_DEFAULT;
    
    aDblinkConf->mAltilinkerPortNo = DKC_ALTILINKER_PORT_NO_DEFAULT;
    
    aDblinkConf->mAltilinkerReceiveTimeout =
        DKC_ALTILINKER_RECEIVE_TIMEOUT_DEFAULT;
    
    aDblinkConf->mAltilinkerRemoteNodeReceiveTimeout =
        DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_DEFAULT;

    aDblinkConf->mAltilinkerQueryTimeout =
        DKC_ALTILINKER_QUERY_TIMEOUT_DEFAULT;
    
    aDblinkConf->mAltilinkerNonQueryTimeout =
        DKC_ALTILINKER_NON_QUERY_TIMEOUT_DEFAULT;
    
    aDblinkConf->mAltilinkerThreadCount = DKC_ALTILINKER_THREAD_COUNT_DEFAULT;
    
    aDblinkConf->mAltilinkerThreadSleepTime =
        DKC_ALTILINKER_THREAD_SLEEP_TIME_DEFAULT;
    
    aDblinkConf->mAltilinkerRemoteNodeSessionCount =
        DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_DEFAULT;

    aDblinkConf->mAltilinkerTraceLogDir = NULL;
        
    aDblinkConf->mAltilinkerTraceLogFileSize =
        DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_DEFAULT;
    
    aDblinkConf->mAltilinkerTraceLogFileCount =
        DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_DEFAULT;
    
    aDblinkConf->mAltilinkerTraceLoggingLevel =
        DKC_ALTILINKER_TRACE_LOGGING_LEVEL_DEFAULT;
    
    aDblinkConf->mAltilinkerJvmMemoryPoolInitSize =
        DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_DEFAULT;
    
    aDblinkConf->mAltilinkerJvmMemoryPoolMaxSize =
        DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_DEFAULT;

    aDblinkConf->mAltilinkerJvmBitDataModelValue =
        DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_DEFAULT;
            
    aDblinkConf->mTargetList = NULL;
}

/*
 *
 */
void dkcUtilFinalizeDblinkConf( dkcDblinkConf * aDblinkConf )
{
    dkcDblinkConfTarget * sCurrent = aDblinkConf->mTargetList;
    dkcDblinkConfTarget * sNext = NULL;
    
    if ( aDblinkConf->mAltilinkerTraceLogDir != NULL )
    {
        (void)iduMemMgr::free( aDblinkConf->mAltilinkerTraceLogDir );
    }

    while ( sCurrent != NULL )
    {
        sNext = sCurrent->mNext;

        dkcUtilFinalizeDblinkConfTarget( sCurrent );
        (void)iduMemMgr::free( sCurrent );
        
        sCurrent = sNext;
    }
    aDblinkConf->mTargetList = NULL;
}

/*
 *
 */ 
void dkcUtilInitializeDblinkConfTarget( dkcDblinkConfTarget * aTarget )
{
    aTarget->mName = NULL;

    aTarget->mJdbcDriver = NULL;

    aTarget->mJdbcDriverClassName = NULL;

    aTarget->mConnectionUrl = NULL;

    aTarget->mUser = NULL;

    aTarget->mPassword = NULL;

    aTarget->mXADataSourceClassName = NULL;

    aTarget->mXADataSourceUrlSetterName = NULL;

    aTarget->mNext = NULL;
}

/*
 *
 */
void dkcUtilFinalizeDblinkConfTarget( dkcDblinkConfTarget * aTarget )
{
    if ( aTarget->mName != NULL )
    {
        (void)iduMemMgr::free( aTarget->mName );
        aTarget->mName = NULL;
    }

    if ( aTarget->mJdbcDriver != NULL )
    {
        (void)iduMemMgr::free( aTarget->mJdbcDriver );
        aTarget->mJdbcDriver = NULL;
    }

    if ( aTarget->mJdbcDriverClassName != NULL )
    {
        (void)iduMemMgr::free( aTarget->mJdbcDriverClassName );
    }

    if ( aTarget->mConnectionUrl != NULL )
    {
        (void)iduMemMgr::free( aTarget->mConnectionUrl );
        aTarget->mConnectionUrl= NULL;
    }

    if ( aTarget->mUser != NULL )
    {
        (void)iduMemMgr::free( aTarget->mUser );
        aTarget->mUser = NULL;
    }

    if ( aTarget->mPassword != NULL )
    {
        (void)iduMemMgr::free( aTarget->mPassword );
        aTarget->mPassword = NULL;
    }

    if ( aTarget->mXADataSourceClassName != NULL )
    {
        (void)iduMemMgr::free( aTarget->mXADataSourceClassName );
        aTarget->mXADataSourceClassName = NULL;
    }
    else
    {
        /*do nothing*/
    }

    if ( aTarget->mXADataSourceUrlSetterName != NULL )
    {
        (void)iduMemMgr::free( aTarget->mXADataSourceUrlSetterName );
        aTarget->mXADataSourceUrlSetterName = NULL;
    }
    else
    {
        /*do nothing*/
    }
}

/*
 * if there is a memory error, then aMemory will be NULL.
 */
void dkcUtilSetString( SChar * aSource, UInt aSourceLength, SChar ** aTarget )
{
    SChar * sTarget = NULL;
    SInt sStage = 0;

    if ( *aTarget != NULL )
    {
        (void)iduMemMgr::free( *aTarget );
        *aTarget = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 aSourceLength + 1, /* for NULL */
                                 (void **)&sTarget,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sStage = 1;

    /* copy */

    (void)idlOS::memcpy( sTarget, aSource, aSourceLength );
    sTarget[ aSourceLength ] = '\0';
    
    *aTarget = sTarget;

    return;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void)iduMemMgr::free( sTarget );
        default:
            break;
    }
    
    return;
}

/*
 * If ID_FALSE is returned, then memory allocation has failed.
 */ 
idBool dkcUtilAppendDblinkConfTarget( dkcDblinkConf * aDblinkConf,
                                      dkcDblinkConfTarget * aTarget )
{
    dkcDblinkConfTarget * sTarget = NULL;
    dkcDblinkConfTarget * sLast = NULL;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 ID_SIZEOF( *sTarget ),
                                 (void **)&sTarget,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    /* All memory pointer is moved. */
    *sTarget = *aTarget;
    dkcUtilInitializeDblinkConfTarget( aTarget );

    if ( aDblinkConf->mTargetList == NULL )
    {
        aDblinkConf->mTargetList = sTarget;
    }
    else
    {
        sLast = aDblinkConf->mTargetList;
        while ( sLast->mNext != NULL )
        {
            sLast = sLast->mNext;
        }
        sLast->mNext = sTarget;
    }

    return ID_TRUE;
    
    IDE_EXCEPTION_END;
    
    if ( sTarget != NULL )
    {
        (void)iduMemMgr::free( sTarget );
        sTarget = NULL;
    }
    else
    {
        /* do nothing */
    }

    return ID_FALSE;
}


/*
 *
 */
void dkcUtilMoveDblinkConf( dkcDblinkConf * aSrc, dkcDblinkConf * aDest )
{
    aDest->mAltilinkerEnable = aSrc->mAltilinkerEnable;
    
    aDest->mAltilinkerPortNo = aSrc->mAltilinkerPortNo;
    
    aDest->mAltilinkerReceiveTimeout = aSrc->mAltilinkerReceiveTimeout;
    
    aDest->mAltilinkerRemoteNodeReceiveTimeout =
        aSrc->mAltilinkerRemoteNodeReceiveTimeout;

    aDest->mAltilinkerQueryTimeout = aSrc->mAltilinkerQueryTimeout;
    
    aDest->mAltilinkerNonQueryTimeout = aSrc->mAltilinkerNonQueryTimeout;
    
    aDest->mAltilinkerThreadCount = aSrc->mAltilinkerThreadCount;
    
    aDest->mAltilinkerThreadSleepTime = aSrc->mAltilinkerThreadSleepTime;
    
    aDest->mAltilinkerRemoteNodeSessionCount =
        aSrc->mAltilinkerRemoteNodeSessionCount;

    aDest->mAltilinkerTraceLogDir = aSrc->mAltilinkerTraceLogDir;
    aSrc->mAltilinkerTraceLogDir = NULL;
        
    aDest->mAltilinkerTraceLogFileSize = aSrc->mAltilinkerTraceLogFileSize;

    aDest->mAltilinkerTraceLogFileCount = aSrc->mAltilinkerTraceLogFileCount;
    
    aDest->mAltilinkerTraceLoggingLevel = aSrc->mAltilinkerTraceLoggingLevel;
    
    aDest->mAltilinkerJvmMemoryPoolInitSize =
        aSrc->mAltilinkerJvmMemoryPoolInitSize;
    
    aDest->mAltilinkerJvmMemoryPoolMaxSize =
        aSrc->mAltilinkerJvmMemoryPoolMaxSize;
    
    aDest->mAltilinkerJvmBitDataModelValue =
        aSrc->mAltilinkerJvmBitDataModelValue;
    
    aDest->mTargetList = aSrc->mTargetList;
    aSrc->mTargetList = NULL;
}
