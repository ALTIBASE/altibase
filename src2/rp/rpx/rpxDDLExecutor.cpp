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
***********************************************************************/

#include <rpcManager.h>
#include <rpdCatalog.h>
#include <rpxDDLExecutor.h>

IDE_RC rpxDDLExecutor::initializeThread()
{
    return IDE_SUCCESS;
}

void rpxDDLExecutor::finalizeThread()
{
    mIsThreadDead = ID_TRUE;
}

IDE_RC rpxDDLExecutor::initialize( cmiProtocolContext * aProtocolContext, rpdVersion * aVersion )
{ 
    mEventFlag = ID_ULONG(0);

    IDE_DASSERT( aProtocolContext != NULL );

    mIsThreadDead = ID_FALSE;

    idlOS::memset( mRepName, 0, ID_SIZEOF( mRepName ) );

    mExitFlag        = ID_FALSE;
    mProtocolContext = aProtocolContext;
    IDU_LIST_INIT_OBJ( &( mNode ), (void*)this );
 
    idvManager::initSession( &( mSession ), 
                             0 /* unuse */, 
                             NULL /* unuse */ );
    idvManager::initSQL( &( mStatistics ),
                         &( mSession ),
                         &mEventFlag, 
                         NULL,
                         NULL, 
                         NULL, 
                         IDV_OWNER_UNKNOWN );
  
    idlOS::memcpy( &mVersion, aVersion, ID_SIZEOF( rpdVersion ) );

    IDE_TEST_RAISE( mThreadJoinCV.initialize() != IDE_SUCCESS, ERR_COND_INIT );

    IDE_TEST_RAISE( mThreadJoinMutex.initialize( (SChar *)"REPL_DDL_EXECUTOR_THR_JOIN_MUTEX",
                                                 IDU_MUTEX_KIND_POSIX,
                                                 IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrCondInit ) );
    }
    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;

    mIsThreadDead = ID_TRUE;

    return IDE_FAILURE;
}

void rpxDDLExecutor::finalize()
{
    mExitFlag = ID_TRUE;

    if ( mProtocolContext != NULL )
    {
        (void)rpnComm::destroyProtocolContext( mProtocolContext );
        mProtocolContext = NULL;
    }
    else
    {
        /* nothing to do */
    }
}

void rpxDDLExecutor::destroy()
{
    if( mThreadJoinCV.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* nothing to do */
    }

    if( mThreadJoinMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* Nothing to do */
    }
}

void rpxDDLExecutor::run()
{
    idBool           sIsDDLSyncBegin = ID_FALSE;
    SChar          * sSql            = NULL;
    idBool           sIsTxInit       = ID_FALSE;
    smiTrans         sDDLTrans;
    SChar            sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };
    SChar          * sUserName = NULL;

    smiStatement     sStatement;
    idBool           sIsStatementBegin = ID_FALSE;

    IDE_ASSERT( ideEnableFaultMgr(ID_TRUE) == IDE_SUCCESS );

    ideLog::log( IDE_RP_0, "[DDLExecutor] DDL Executor Start..." );

    IDE_TEST( sDDLTrans.initialize() != IDE_SUCCESS );
    sIsTxInit = ID_TRUE;

    IDE_TEST( rpcManager::ddlSyncBeginInternal( &mStatistics,
                                                mProtocolContext,
                                                &sDDLTrans,
                                                &mExitFlag,
                                                &mVersion,
                                                mRepName,
                                                &sUserName,
                                                &sSql )
              != IDE_SUCCESS );
    sIsDDLSyncBegin = ID_TRUE;

    IDE_TEST( sStatement.begin( &mStatistics,
                                sDDLTrans.getStatement(),
                                SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sIsStatementBegin = ID_TRUE;

    IDE_TEST( rpcDDLSyncManager::runDDL( &mStatistics,
                                         sSql,
                                         sUserName,
                                         &sStatement ) 
              != IDE_SUCCESS );
    ideLog::log( IDE_RP_0, "[DDLExecutor] Execute DDL <%s> Success", sSql );

    IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStatementBegin = ID_FALSE;

    sIsDDLSyncBegin = ID_FALSE;
    IDE_TEST( rpcManager::ddlSyncEndInternal( &sDDLTrans ) != IDE_SUCCESS );
    ideLog::log( IDE_RP_0, "[DDLExecutor] DDL Sync Success" );

    sIsTxInit = ID_FALSE;
    IDE_TEST( sDDLTrans.destroy( NULL ) != IDE_SUCCESS );

    (void)iduMemMgr::free( sSql );
    sSql = NULL;

    finalize();

    ideLog::log( IDE_RP_0, "[DDLExecutor] Normal Exit..." );

    return;

    IDE_EXCEPTION_END;

    idlOS::snprintf( sErrMsg,
                     RP_MAX_MSG_LEN,
                     "%s",
                     ideGetErrorMsg( ideGetErrorCode() ) );

    IDE_PUSH();

    if ( sIsStatementBegin == ID_TRUE )
    {
        sIsStatementBegin = ID_FALSE;
        (void)sStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsDDLSyncBegin == ID_TRUE )
    {
        sIsDDLSyncBegin = ID_FALSE;
        rpcManager::ddlSyncException( &sDDLTrans ); 
    }

    if ( sIsTxInit == ID_TRUE )
    {
        sIsTxInit = ID_FALSE;
        (void)sDDLTrans.destroy( NULL );
    }

    if ( sSql != NULL )
    {
        (void)iduMemMgr::free( sSql );
        sSql = NULL;
    }

    finalize();

    IDE_POP();

    IDE_ERRLOG( IDE_RP_0 );

    ideLog::log( IDE_RP_0, "[DDLExecutor] Error Exit..." );

    return;
}

IDE_RC rpxDDLExecutor::waitThreadJoin( idvSQL *aStatistics )
{
    PDL_Time_Value sTvCpu;
    PDL_Time_Value sTimeValue;
    idBool         sIsLock = ID_FALSE;
    UInt           sTimeoutMilliSec = 0;

    IDE_ASSERT( mThreadJoinMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsLock = ID_TRUE;

    sTimeValue.initialize( 0, 1000 );

    while( mIsThreadDead != ID_TRUE )
    {
        sTvCpu  = idlOS::gettimeofday();
        sTvCpu += sTimeValue;

        (void)mThreadJoinCV.timedwait( &mThreadJoinMutex, &sTvCpu );

        if ( aStatistics != NULL )
        {
            // BUG-22637 MM에서 QUERY_TIMEOUT, Session Closed를 설정했는지 확인
            IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
        }
        else
        {
            IDU_FIT_POINT_RAISE( "rpxDDLExecutor::waitThreadJoin::ERR_TIMEOUT", ERR_TIMEOUT );
            // 5 Sec
            IDE_TEST_RAISE( sTimeoutMilliSec >= 5000, ERR_TIMEOUT );
            sTimeoutMilliSec++;
        }
    }

    sIsLock = ID_FALSE;
    IDE_ASSERT( mThreadJoinMutex.unlock() == IDE_SUCCESS );

    if( join() != IDE_SUCCESS )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_JOIN_THREAD ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_CALLBACK_FATAL( "[Repl DDLExecutor] Thread join error" );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TIMEOUT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "rpxDDLExecutor::waitThreadJoin exceeds timeout" ) );
    }
    IDE_EXCEPTION_END;

    if( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        IDE_ASSERT( mThreadJoinMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* notihng to do */
    }

    return IDE_FAILURE;
}

