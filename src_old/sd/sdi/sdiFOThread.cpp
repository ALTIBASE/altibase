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
 * $Id: sdiFOThread.cpp 15589 2006-04-18 01:47:21Z leekmo $
 **********************************************************************/

#include <ide.h>
#include <sdiFOThread.h>

sdiFOThread::sdiFOThread() : idtBaseThread()
{

}

sdiFOThread::~sdiFOThread()
{

}

/* ------------------------------------------------
 * Description:
 *
 * thread 작업 시작 루틴
 * ----------------------------------------------*/
IDE_RC sdiFOThread::threadRun( sdiFOThread  * aThreads )
{
    IDE_TEST( aThreads[0].start() != IDE_SUCCESS );

    IDE_TEST( aThreads[0].join() != IDE_SUCCESS );

    if( aThreads[0].mIsSuccess == ID_FALSE )
    {
        IDE_TEST_RAISE( aThreads[0].mErrorCode != 0,
                        err_thread_join );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_thread_join );
    {
        ideCopyErrorInfo( ideGetErrorMgr(),
                          aThreads->getErrorMgr() );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiFOThread::failoverExec( SChar         * aMyNodeName,
                                  SChar         * aTargetNodeName,
                                  UInt            aThreadCnt )
{
    sdiFOThread         * sThreads  = NULL;
    UInt                  i         = 0;
    UInt                  sState    = 0;
    UInt                  sThrState = 0;

    IDE_DASSERT( aThreadCnt > 0 );

    IDE_TEST_RAISE(iduMemMgr::malloc( IDU_MEM_SDI,
                                (ULong)ID_SIZEOF(sdiFOThread) * aThreadCnt,
                                (void**)&sThreads,
                                IDU_MEM_IMMEDIATE ) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    for(i = 0; i < aThreadCnt; i++)
    {
        new (sThreads + i) sdiFOThread;

        IDE_TEST( sThreads[i].initialize( aMyNodeName,
                                          aTargetNodeName )
                  != IDE_SUCCESS);

        sThrState++;
    }

    IDE_TEST( threadRun( sThreads )
              != IDE_SUCCESS );

    while( sThrState > 0 )
    {
        IDE_TEST( sThreads[--sThrState].destroy() != IDE_SUCCESS);
    }
    sState = 0;
    IDE_TEST( iduMemMgr::free( sThreads ) != IDE_SUCCESS);
    sThreads = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    
    IDE_PUSH();

    for(i = 0; i < sThrState; i++)
    {
        (void)sThreads[i].destroy();
    }

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free(sThreads);
            sThreads = NULL;
            break;
    }
        
    IDE_POP();
    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * Index build 쓰레드 초기화
 * ----------------------------------------------*/
IDE_RC sdiFOThread::initialize( SChar * aMyNodeName,
                                SChar * aTargetNodeName )
{
    mIsSuccess = ID_TRUE;
    mErrorCode = 0;

    idlOS::strncpy( mMyNodeName,
                    aMyNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    idlOS::strncpy( mTargetNodeName,
                    aTargetNodeName,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    return IDE_SUCCESS;
}

/* BUG-27403 쓰레드 정리 */
IDE_RC sdiFOThread::destroy()
{
    return IDE_SUCCESS;
}

/* ------------------------------------------------
 * Description :
 *
 * 쓰레드 메인 실행 루틴
 * ----------------------------------------------*/
void sdiFOThread::run()
{
    SChar sSqlStr[QD_MAX_SQL_LENGTH + 1];

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "ALTER DATABASE SHARD FAILOVER FORCE %s ",
                     mTargetNodeName );

    IDE_TEST( sdi::shardExecTempSQLWithoutSession( sSqlStr,
                                                   mMyNodeName,
                                                   0,
                                                   QCI_STMT_MASK_MAX )
              != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    mErrorCode = ideGetErrorCode();
    ideCopyErrorInfo( &mErrorMgr, ideGetErrorMgr() );

    mIsSuccess = ID_FALSE;
}
