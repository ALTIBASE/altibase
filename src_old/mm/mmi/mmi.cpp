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

#include <mmi.h>
#include <mmErrorCode.h>
#include <idtContainer.h>

mmiServer * mmi::mServer        = NULL;
UInt        mmi::mServerOption  = 0;

SChar       mmi::mErrorMessage[MMI_ERROR_MESSAGE_SIZE] = ""; 
SInt        mmi::mErrorCode     = -1;

void
mmi::setError( SInt aErrorCode, const SChar * aErrorMessage )
{
    idlOS::snprintf( mErrorMessage, MMI_ERROR_MESSAGE_SIZE, aErrorMessage );

    mErrorCode = aErrorCode;
}

SInt 
mmi::serverStart( SInt aPhase, SInt aOption )
{
    SInt sRC       = 0;
    SInt sMaskDiff = 0xFFFFFFFF;

    mServerOption = aOption;

    // option checking :
    // MMI_DEBUG_MASK , MMI_DAEMON_MASK, MMI_SIGNAL_MASK, MMI_INNER_MASK
    sMaskDiff ^= ( MMI_DEBUG_MASK | MMI_DAEMON_MASK | MMI_SIGNAL_MASK | MMI_INNER_MASK );

    IDE_TEST_RAISE( (aOption & sMaskDiff) != 0x00000000, error_invalid_option );

    IDE_TEST_RAISE( idtContainer::initializeStatic(IDU_SERVER_TYPE)
                    != IDE_SUCCESS,
                    error_thread_initialize );
    if( (aOption & MMI_INNER_MASK) == MMI_INNER_TRUE )
    {
        IDE_TEST( idf::initializeStatic( NULL ) != IDE_SUCCESS );

        IDE_TEST( mmm::execute( (mmmPhase)aPhase, 0 ) != IDE_SUCCESS );

        IDE_TEST( mmtThreadManager::startListener() != IDE_SUCCESS );

        (void)mmtSessionManager::run();

        (void)mmtAdminManager::waitForAdminTaskEnd();

        IDE_TEST( mmm::execute( MMM_STARTUP_SHUTDOWN, 0 ) != IDE_SUCCESS);

        IDE_TEST( idf::finalizeStatic() != IDE_SUCCESS );

        sRC = 0;
    }
    else
    {
        // PHASE Check
        IDE_TEST_RAISE( (aPhase != MMI_STARTUP_PROCESS) && (aPhase != MMI_STARTUP_SERVICE),
                        error_invalid_phase );

        IDE_TEST_RAISE( mmi::isStarted() == 0, error_already_started );

        IDE_TEST_RAISE( (aOption & MMI_DAEMON_MASK) == MMI_DAEMON_TRUE, 
                        error_invalid_option );

        IDE_TEST( idf::initializeStatic( NULL ) != IDE_SUCCESS );

        mServer = new mmiServer( (mmmPhase)aPhase );

        // bug-27571: klocwork warnings
        // add NULL check
        IDE_TEST_RAISE( mServer == NULL, error_server_start );

        IDE_TEST_RAISE( mServer->initialize() != IDE_SUCCESS,
                        error_server_start );

        IDE_TEST_RAISE( mServer->start() != IDE_SUCCESS, error_server_start );

        IDE_TEST_RAISE( mServer->wait() != IDE_SUCCESS, error_server_start );

        IDE_TEST_RAISE( mServer->status() == -1, error_server_start );

        sRC = mServer->status();
    }
    
    return sRC;

    IDE_EXCEPTION( error_already_started );
    {
        IDE_SET( ideSetErrorCode(mmERR_ABORT_SERVER_ALREADY_STARTED) );
    }
    IDE_EXCEPTION( error_invalid_option );
    {
        // 예외 : msg 가 로딩되지 않은 상태에의 에러이므로
        // ErrorMessage 가 올바르게 출력되지 않는다.
        // 그래서, mmi 내에서 직접 처리해준다.
        mmi::setError( 0, MM_TRC_INVALID_OPTION );
    }
    IDE_EXCEPTION( error_thread_initialize );
    {
        mmi::setError( 0, MM_TRC_THREAD_INITFAIL );
    }
    IDE_EXCEPTION( error_invalid_phase );
    {
        // 예외 : msg 가 로딩되지 않은 상태에의 에러이므로
        // ErrorMessage 가 올바르게 출력되지 않는다.
        // 그래서, mmi 내에서 직접 처리해준다.
        mmi::setError( 0, MM_TRC_INVALID_PHASE );
    }
    IDE_EXCEPTION( error_server_start );
    {
        if(mServer != NULL)
        {
            (void)mServer->join();

            (void)mServer->finalize();

            delete mServer;

            mServer = NULL;
        }
    }
    IDE_EXCEPTION_END;

    return -1;
}

SInt 
mmi::serverStop()
{
    if( (mServerOption & MMI_INNER_MASK) == MMI_INNER_TRUE )
    {
        //pass
    }
    else
    { 
        IDE_TEST_RAISE( mmi::isStarted() != 0, error_not_started );

        (void)mmm::setServerStatus( ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE );

        (void)mmm::prepareShutdown( ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE, ID_TRUE );

        IDE_TEST( mServer->join() != IDE_SUCCESS );
        
        IDE_TEST( mServer->finalize() != IDE_SUCCESS );

        IDE_TEST( idf::finalizeStatic() != IDE_SUCCESS );

        IDE_TEST( idtContainer::destroyStatic() != IDE_SUCCESS );
        
        delete mServer;

        mServer = NULL;
    }

    return 0;

    IDE_EXCEPTION( error_not_started );
    {
        mmi::setError( 0, MM_TRC_SERVER_NOT_STARTED );
    }
    IDE_EXCEPTION_END;

    return -1;
}

SInt 
mmi::isStarted()
{
    return ( mServer != NULL ) ? 0 : -1;
}

SInt mmi::getErrorCode()
{
    SInt sErrorCode = 0;

    if ( mmi::isStarted() == 0 )
    {
        sErrorCode = E_ERROR_CODE(ideGetErrorCode());
    }
    else
    {
        sErrorCode = mErrorCode;
    }

    return sErrorCode;
}

SChar * mmi::getErrorMessage()
{
    SChar * sErrorMessage = NULL; 

    if ( isStarted() == 0 )
    {
        sErrorMessage = ideGetErrorMsg(ideGetErrorCode());
    }
    else
    {
        if ( mmi::getErrorCode() == -1 )
        {
            mmi::setError( -1, MM_TRC_SERVER_NOT_STARTED );
        }

        sErrorMessage = mErrorMessage;
    }

    return sErrorMessage;
}

void 
mmiServer::run()
{
    idBool sPost = ID_FALSE;

    IDE_TEST( mmm::execute( mPhase, 0 ) != IDE_SUCCESS );

    IDE_TEST( mmtThreadManager::startListener() != IDE_SUCCESS );

    sPost = ID_TRUE;
    IDE_TEST( post() != IDE_SUCCESS );

    (void)mmtSessionManager::run();

    (void)mmtAdminManager::waitForAdminTaskEnd();

    IDE_TEST( mmm::execute( MMM_STARTUP_SHUTDOWN, 0 ) != IDE_SUCCESS);

    return;

    IDE_EXCEPTION_END;

    mStatus = -1;

    mmi::setError( E_ERROR_CODE(ideGetErrorCode()),
                   ideGetErrorMsg(ideGetErrorCode()) ); 

    if( sPost != ID_TRUE )
    {
        (void)post();
    }

    return;
}

IDE_RC
mmiServer::initialize()
{
    mPost = ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC
mmiServer::finalize()
{
    mPost = ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC
mmiServer::wait()
{
    while( 1 )
    {
        if( mPost == ID_TRUE )
        {
            break;
        }
        else
        {
            idlOS::sleep( 1 );
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
mmiServer::post()
{
    mPost = ID_TRUE;

    return IDE_SUCCESS;
}

