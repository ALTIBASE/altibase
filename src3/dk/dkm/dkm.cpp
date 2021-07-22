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
 * $Id $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <qci.h>
#include <sdi.h>
#include <dki.h>
#include <dkDef.h>
#include <dkm.h>

#include <dkuProperty.h>
#include <dkErrorCode.h>

#include <dkaLinkerProcessMgr.h>
#include <dkpProtocolMgr.h>
#include <dkoLinkObjMgr.h>
#include <dkoLinkInfo.h>
#include <dksSessionMgr.h>
#include <dktGlobalTxMgr.h>
#include <dkdDataBufferMgr.h>
#include <dkdResultSetMetaCache.h>


/************************************************************************
 * Description : 프로퍼티 파일에 database link 를 사용하도록 설정되어
 *               있는지 검사한다.
 *
 ************************************************************************/
IDE_RC dkmCheckDblinkEnabled( void )
{
    IDE_TEST_RAISE( DKU_DBLINK_ENABLE != DK_ENABLE, ERR_DBLINK_DISABLED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_DISABLED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_DISABLED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkmCheckDblinkOrShardMetaEnabled( void )
{
    IDE_TEST_RAISE( ( DKU_DBLINK_ENABLE != DK_ENABLE ) &&
                    ( sdi::isShardEnable() != ID_TRUE ),
                    ERR_DBLINK_DISABLED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_DISABLED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_DISABLED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool dkmDblinkStarted()
{
    idBool sRet = ID_FALSE;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) &&
         ( dkaLinkerProcessMgr::getLinkerStatus() != DKA_LINKER_STATUS_NON ) &&
         ( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() == ID_TRUE ) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }
    return sRet;
}

/************************************************************************
 * Description : DK module 의 각 컴포넌트들을 초기화한다.
 * PROJ-2569를 통해 sm 복구단계에서 dk정보와 콜백을 사용하므로
 * DK_ENABLE 프로퍼티와 상관없이 초기화해야함.
 ************************************************************************/
IDE_RC dkmInitialize()
{
    UInt    sStage = 0;

    /* ------------------------------------------------
        The order of initializing components :

            1. dka: dkaLinkerProcessMgr
            2. dkp: dkpProtocolMgr
            3. dko: dkoLinkObjMgr
            4. dks: dksSessionMgr
            5. dkt: dktGlobalTxMgr
            6. dkd: dkdDataBufferMgr
            7. dkdResultSetMetaCache
       -----------------------------------------------*/

    /* Load DK properties */
    IDE_TEST( dkuProperty::load() != IDE_SUCCESS );
    
    sStage = 1;

    /* 1. dka */
    IDE_TEST( dkaLinkerProcessMgr::initializeStatic() != IDE_SUCCESS );
    sStage = 2;

    /* 2. dkp */
    IDE_TEST( dkpProtocolMgr::initializeStatic() != IDE_SUCCESS );
    sStage = 3;

    /* 3. dko */
    IDE_TEST( dkoLinkObjMgr::initializeStatic() != IDE_SUCCESS );
    sStage = 4;

    /* 4. dks */
    IDE_TEST( dksSessionMgr::initializeStatic() != IDE_SUCCESS );
    sStage = 5;

    /* 5. dkt */
    IDE_TEST( dktGlobalTxMgr::initializeStatic() != IDE_SUCCESS );
    sStage = 6;

    /* 6. dkd */
    IDE_TEST( dkdDataBufferMgr::initializeStatic() != IDE_SUCCESS );
    sStage = 7;

    /* 7. dkdResultSetMetaCache */
    IDE_TEST( dkdResultSetMetaCacheInitialize() != IDE_SUCCESS );
    sStage = 8;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 8:
            (void)dkdResultSetMetaCacheFinalize();
            /* keep going */
        case 7:
            (void)dkdDataBufferMgr::finalizeStatic();
            /* keep going */
        case 6:
            (void)dktGlobalTxMgr::finalizeStatic();
            /* keep going */
        case 5:
            (void)dksSessionMgr::finalizeStatic();
            /* keep going */
        case 4:
            (void)dkoLinkObjMgr::finalizeStatic();
            /* keep going */
        case 3:
            (void)dkpProtocolMgr::finalizeStatic();
            /* keep going */
        case 2:
            (void)dkaLinkerProcessMgr::finalizeStatic();
            /* keep going */
        case 1:
            /* keep going */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : DK module 의 각 컴포넌트들을 정리해준다.
 *
 *  BUG-37487 : return 값을 IDE_RC --> void 로 변경.
 *
 ************************************************************************/
IDE_RC dkmFinalize()
{
    /* ------------------------------------------------
       The order of finalizing components :

       1. dkdResultSetMetaCache
       2. dkd: dkdDataBufferMgr
       3. dkt: dktGlobalTxMgr
       4. dks: dksSessionMgr
       5. dko: dkoLinkObjMgr
       6. dkp: dkpProtocolMgr
       7. dka: dkaLinkerProcessMgr
       -----------------------------------------------*/

    /* 1. dkdResultSetMetaCache */
    (void)dkdResultSetMetaCacheFinalize();

    /* >> BUG-37487 : void */
    /* 2. dkd */
    (void)dkdDataBufferMgr::finalizeStatic();

    /* 3. dkt */
    (void)dktGlobalTxMgr::finalizeStatic();

    /* 4. dks */
    (void)dksSessionMgr::finalizeStatic();

    /* 5. dko */
    (void)dkoLinkObjMgr::finalizeStatic();

    /* 6. dkp */
    (void)dkpProtocolMgr::finalizeStatic();

    /* 7. dka */
    (void)dkaLinkerProcessMgr::finalizeStatic();
    /* << BUG-37487 : void */

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Linker data session 을 생성하고 초기화해준다.
 *
 *  aSessionID  - [IN] 생성할 linker data session id
 *  aSession    - [OUT] 생성한 linker data session. dkmSession 으로
 *                      mapping 된다.
 *
 ************************************************************************/
IDE_RC dkmSessionInitialize( UInt          aSessionID,
                             dkmSession  * aSession_,
                             dkmSession ** aSession )
{
    if ( ( dkmDblinkStarted() == ID_TRUE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( dksSessionMgr::createDataSession( aSessionID,
                                                          aSession_ )
                        != IDE_SUCCESS, ERR_DKM_SESSION_INIT );

        *aSession = aSession_;
    }
    else
    {
        /* Altilinker process disabled */
        *aSession = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DKM_SESSION_INIT );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_SESSION_INIT_FAILED,
                                  aSessionID ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 linker data session 을 정리해준다.
 *
 *  aSession    - [IN] 정리할 linker data session
 *
 ************************************************************************/
IDE_RC dkmSessionFinalize( dkmSession   *aSession )
{
    dksDataSession  *sSession     = NULL;

    sSession = (dksDataSession *)aSession;

    if ( sSession != NULL )
    {
        /* BUG-37487 : void */
        (void)dksSessionMgr::closeDataSession( sSession );

        IDE_TEST( dksSessionMgr::destroyDataSession( sSession )
                  != IDE_SUCCESS );
    }
    else
    {
        /* don't need to finalize session */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 user id 를 linker data session 에 copy 한다.
 *
 *  aSession    - [IN] 입력받은 user id 를 설정할 대상 linker data session
 *  aUserId     - [IN] User id
 *
 ************************************************************************/
void dkmSessionSetUserId( dkmSession * aSession, UInt aUserId )
{
    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        aSession->mUserId = aUserId;
    }
    else
    {
        /* do nothing */
    }
}

/************************************************************************
 * Description : Global transaction level 을 얻어온다.
 *
 *  aValue      - [IN] Global transaction level
 *
 ************************************************************************/
IDE_RC dkmGetGlobalTransactionLevel( UInt * aValue )
{
    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        *aValue = DKU_GLOBAL_TRANSACTION_LEVEL;
    }
    else
    {
        *aValue = 0;
        /* success */
    }

    return IDE_SUCCESS;
}

/*
 *
 */
/************************************************************************
 * Description : Global transaction level 을 얻어온다.
 *
 *  aValue      - [IN] Global transaction level
 *
 ************************************************************************/
IDE_RC dkmGetRemoteStatementAutoCommit( UInt * aValue )
{
    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        *aValue = DKU_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT;
    }
    else
    {
        *aValue = 0;
        /* success */
    }

    return IDE_SUCCESS;

}

/************************************************************************
 * Description : Global transaction level 을 얻어온다.
 *
 *  aValue      - [IN] Global transaction level
 *
 ************************************************************************/
IDE_RC dkmSessionSetGlobalTransactionLevel( dkmSession * aSession,
                                            UInt         aValue )
{
    IDE_ASSERT( aSession != NULL );

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( aValue >= DKT_ADLP_MAX, ERR_INVALID_VALUE );

        /* PROJ-2569                                         *
         * 트랜잭션 진행중에는 트랜잭션 레벨 변경 할 수 없음 */
        IDE_TEST_RAISE( aSession->mGlobalTxId != DK_INIT_GTX_ID, ERR_CANNOT_CHANGE );

        aSession->mAtomicTxLevel = (dktAtomicTxLevel)aValue;

        if ( dkiIsGTx( aValue ) == ID_TRUE )
        {
            aSession->mSession.mIsXA = ID_TRUE;
        }
        else
        {
            aSession->mSession.mIsXA = ID_FALSE;
        }

    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_NOT_SUPPORT_GTX_LEVEL ) );
    }
    IDE_EXCEPTION( ERR_CANNOT_CHANGE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_CAN_NOT_CHANGE_GTX_LEVEL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description  : Linker data session 의 remote statement auto commit
 *                을 입력받은 값( aValue )으로 설정하고 AltiLinker 에
 *                연결되어 있는 모든 remote server 의 auto commit 설정
 *                값을 변경시킨다.
 *
 *  aSession    - [IN] Remote statement auto commit 을 설정할 session
 *  aValue      - [IN] Remote statement auto commit 설정값
 *
 ************************************************************************/
IDE_RC dkmSessionSetRemoteStatementAutoCommit( dkmSession   *aSession,
                                               UInt          aValue )
{
    UInt             sResultCode = DKP_RC_SUCCESS;
    ULong            sTimeoutSec;
    dksDataSession  *sSession    = NULL;
    SInt             sReceiveTimeout = 0;
    SInt             sRemoteNodeRecvTimeout = 0;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                        ERR_LINKER_PROCESS_NOT_RUNNING );

        IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
                  != IDE_SUCCESS );

        IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( &sRemoteNodeRecvTimeout )
                  != IDE_SUCCESS );

        sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

        IDE_TEST_RAISE( aValue != 0 && aValue != 1, ERR_INVALID_VALUE );

        IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                  != IDE_SUCCESS );

        IDE_TEST( dkpProtocolMgr::sendSetAutoCommitMode( &sSession->mSession,
                                                         sSession->mId,
                                                         (UChar)aValue )
                  != IDE_SUCCESS );

        IDE_TEST( dkpProtocolMgr::recvSetAutoCommitModeResult( &sSession->mSession,
                                                               sSession->mId,
                                                               &sResultCode,
                                                               sTimeoutSec )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

        IDE_TEST( dksSessionMgr::disconnectDataSession( sSession )
                  != IDE_SUCCESS );

        aSession->mAutoCommitMode = aValue;
    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_INVALID_VALUE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_NOT_SUPPORT_AUTOCOMMIT_MODE ) );
    }
    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            NULL, 
                                            0, 
                                            0, 
                                            NULL );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* session not exist */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Global transaction 을 Prepare 한다.
 *
 *  aSession    - [IN] Commit 할 linker data session
 *
 ************************************************************************/
IDE_RC dkmPrepare( dkmSession * aSession, ID_XID * aSourceXID )
{
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    dksDataSession       * sSession           = NULL;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

        sSession = (dksDataSession *)aSession;

        IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                         &sGlobalCoordinator )
                  != IDE_SUCCESS );

        if ( sGlobalCoordinator != NULL )
        {
            if ( dkmDblinkStarted() == ID_TRUE )
            {
                IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            /* BUG-46092 */
            /* DKT_COORD_FLAG_TRANSACTION_BROKEN_* flag is used for sharding. */
            IDE_TEST_RAISE( sGlobalCoordinator->getFlag( DKT_COORD_FLAG_TRANSACTION_BROKEN_MASK )
                            == DKT_COORD_FLAG_TRANSACTION_BROKEN_TRUE,
                            ERR_GTX_SHARD_NEED_ROLLBACK );

            if ( sGlobalCoordinator->getGTxStatus() == DKT_GTX_STATUS_PREPARED )
            {
                /* 이미 prepared 상태임 */
            }
            else
            {
                sGlobalCoordinator->setXID( aSourceXID );

                IDE_TEST_RAISE( sGlobalCoordinator->executePrepare() != IDE_SUCCESS,
                                ERR_GTX_PREPARE_PHASE_FAILED );

                sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARED );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                 "[dkmPrepare] aSession is NULL" ) );
    }
    IDE_EXCEPTION( ERR_GTX_SHARD_NEED_ROLLBACK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_COMMIT_FAILED ) );
    }
    IDE_EXCEPTION( ERR_GTX_PREPARE_PHASE_FAILED );
    {
        if ( sdi::isShardEnable() == ID_TRUE )
        {
            /* BUG-46092
             * Nothing to do.
             * Do not over-write error code.
             */
            if ( sGlobalCoordinator != NULL )
            {
                sGlobalCoordinator->setFlag( DKT_COORD_FLAG_TRANSACTION_BROKEN_MASK,
                                             DKT_COORD_FLAG_TRANSACTION_BROKEN_TRUE );
            }
        }
        else
        {
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_COMMIT_FAILED ) );
        }
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void dkmSetGtxPreparedStatus( dkmSession * aSession )
{
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    dksDataSession       * sSession           = NULL;

    IDE_DASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        (void)dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId, &sGlobalCoordinator );

        if ( sGlobalCoordinator != NULL )
        {
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARED );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }
}

/************************************************************************
 * Description : Global transaction 을 commit 한다.
 *
 *  aSession    - [IN] Commit 할 linker data session
 *
 ************************************************************************/
IDE_RC dkmCommit( dkmSession    *aSession )
{
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dksDataSession          *sSession           = NULL;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

        sSession = (dksDataSession *)aSession;

        IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                         &sGlobalCoordinator )
                  != IDE_SUCCESS );

        if ( sGlobalCoordinator != NULL )
        {
            if ( dkmDblinkStarted() == ID_TRUE )
            {
                IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Altilinker process disabled */
            }

            IDE_TEST( sGlobalCoordinator->executeCommit() != IDE_SUCCESS );

            if ( ( sGlobalCoordinator->isGTx() == ID_TRUE ) &&
                 ( sGlobalCoordinator->mDtxInfo != NULL ) )
            {
                sGlobalCoordinator->mDtxInfo->mResult = SMI_DTX_COMMIT;
            }
            else
            {
                /* Nothing to do */
            }

            /* Destroy global coordinator */
            dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
        }
        else
        {
            dksSessionMgr::setDataSessionGlobalTxId( sSession, DK_INIT_GTX_ID );
            dksSessionMgr::setDataSessionLocalTxId( sSession, DK_INIT_LTX_ID );
        }

        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                 "[dkmCommit] aSession is NULL" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sGlobalCoordinator != NULL )
    {
        if ( sdi::isShardEnable() == ID_TRUE )
        {
            sGlobalCoordinator->closeAllShardTransasction();
        }

        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }
    else
    {
        if ( sSession != NULL )
        {
            dksSessionMgr::setDataSessionGlobalTxId( sSession, DK_INIT_GTX_ID );
            dksSessionMgr::setDataSessionLocalTxId( sSession, DK_INIT_LTX_ID );
        }
        else
        {
            /* do nothing */
        }
    }

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();
    IDE_ERRLOG(IDE_DK_0);
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Global transaction 을 rollback 한다.
 *
 *  aSession    - [IN] Rollback 할 linker data session
 *  aSavepoint  - [IN] Rollback 할 지점을 가리키는 savepoint
 *
 ************************************************************************/
IDE_RC dkmRollback( dkmSession  *aSession, SChar  *aSavepoint )
{
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dksDataSession          *sSession           = NULL;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

        sSession = (dksDataSession *)aSession;

        IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                         &sGlobalCoordinator )
                  != IDE_SUCCESS );

        if ( sGlobalCoordinator != NULL )
        {
            if ( dkmDblinkStarted() == ID_TRUE )
            {
                IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( sGlobalCoordinator->executeRollback( aSavepoint ) != IDE_SUCCESS );

            if ( ( sGlobalCoordinator->getRemoteTxCount() == 0 ) ||
                 ( ( dkiIsGTx( sSession->mAtomicTxLevel ) == ID_TRUE ) &&
                   ( aSavepoint == NULL ) )  /*total rollback*/
               )
            {
                /* BUG-46092 */
                /* DKT_COORD_FLAG_TRANSACTION_BROKEN_* flag is used for sharding. */
                sGlobalCoordinator->setFlag( DKT_COORD_FLAG_TRANSACTION_BROKEN_MASK,
                                             DKT_COORD_FLAG_TRANSACTION_BROKEN_FALSE );

                dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
                (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            dksSessionMgr::setDataSessionGlobalTxId( sSession, DK_INIT_GTX_ID );
            dksSessionMgr::setDataSessionLocalTxId( sSession, DK_INIT_LTX_ID );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                 "[dkmRollback] aSession is NULL" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sGlobalCoordinator != NULL )
    {
        if ( ( sGlobalCoordinator->getRemoteTxCount() == 0 ) ||
             ( ( dkiIsGTx( sSession->mAtomicTxLevel ) == ID_TRUE ) &&
               ( aSavepoint == NULL ) ) /*total rollback*/
           )
        {
            /* BUG-46092 */
            /* DKT_COORD_FLAG_TRANSACTION_BROKEN_* flag is used for sharding. */
            sGlobalCoordinator->setFlag( DKT_COORD_FLAG_TRANSACTION_BROKEN_MASK,
                                         DKT_COORD_FLAG_TRANSACTION_BROKEN_FALSE );

            dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
            (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        if ( sSession != NULL )
        {
            dksSessionMgr::setDataSessionGlobalTxId( sSession, DK_INIT_GTX_ID );
            dksSessionMgr::setDataSessionLocalTxId( sSession, DK_INIT_LTX_ID );
            (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Global transaction 을 강제로 rollback 한다.
 *               여기서 강제로 rollback 한다는 것은 network failure 등의
 *               문제로 인해 일부 원격노드에 대해 global transaction 의
 *               rollback 을 수행할 수 없는 상황에서 rollback 이 가능한
 *               remote transaction 들에 대해 rollback 을 수행할 수 있게
 *               하기 위한 함수이다.
 *
 *  aSession    - [IN] Rollback 할 linker data session
 *
 ************************************************************************/
IDE_RC dkmRollbackForce( dkmSession * aSession )
{
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dksDataSession          *sSession           = NULL;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

        sSession = (dksDataSession *)aSession;

        IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                         &sGlobalCoordinator )
                  != IDE_SUCCESS );

        if ( sGlobalCoordinator != NULL )
        {
            if ( dkmDblinkStarted() == ID_TRUE )
            {
                IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Altilinker process disabled */
            }

            if ( sGlobalCoordinator->isGTx() == ID_TRUE )
            {
                IDE_TEST( sGlobalCoordinator->executeRollback(NULL) != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( sGlobalCoordinator->executeRollbackForce() != IDE_SUCCESS );
            }

            dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
        }
        else
        {
            dksSessionMgr::setDataSessionGlobalTxId( sSession, DK_INIT_GTX_ID );
            dksSessionMgr::setDataSessionLocalTxId( sSession, DK_INIT_LTX_ID );
        }

        /* clear transaction info of this session */
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                 "[dkmRollbackForce] aSession is NULL" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sGlobalCoordinator != NULL )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        if ( sSession != NULL )
        {
            dksSessionMgr::setDataSessionGlobalTxId( sSession, DK_INIT_GTX_ID );
            dksSessionMgr::setDataSessionLocalTxId( sSession, DK_INIT_LTX_ID );
            (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Savepoint 를 설정한다.
 *
 *  aSession    - [IN] Linker data session
 *  aSavepoint  - [IN] Savepoint name
 *
 ************************************************************************/
IDE_RC dkmSavepoint( dkmSession *aSession, const SChar  *aSavepoint )
{
    idBool                   sIsSet             = ID_FALSE;
    UInt                     sResultCode        = DKP_RC_SUCCESS;
    ULong                    sTimeoutSec;
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dksDataSession          *sSession           = NULL;
    SInt                     sReceiveTimeout    = 0;
    SInt                     sRemoteNodeRecvTimeout = 0;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

        sSession = (dksDataSession *)aSession;

        IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                         &sGlobalCoordinator )
                  != IDE_SUCCESS );

        if ( sGlobalCoordinator != NULL )
        {
            IDE_TEST_RAISE( ( dkiIsGTx( sSession->mAtomicTxLevel ) == ID_TRUE ) &&
                            ( sGlobalCoordinator->getLinkerType() != DKT_LINKER_TYPE_SHARD ),
                            ERR_CANNOT_SET_SAVEPOINT );

            switch ( sGlobalCoordinator->getLinkerType() )
            {
                case DKT_LINKER_TYPE_DBLINK:
                {
                    IDE_TEST( sGlobalCoordinator->setSavepoint( aSavepoint )
                            != IDE_SUCCESS );
                    sIsSet = ID_TRUE;

                    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf(
                                  &sReceiveTimeout )
                              != IDE_SUCCESS );

                    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf(
                                  &sRemoteNodeRecvTimeout )
                              != IDE_SUCCESS );

                    sTimeoutSec = sReceiveTimeout + sRemoteNodeRecvTimeout;

                    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                              != IDE_SUCCESS );

                    IDE_TEST( dkpProtocolMgr::sendSavepoint( &sSession->mSession,
                                                             sSession->mId,
                                                             aSavepoint )
                              != IDE_SUCCESS );

                    IDE_TEST( dkpProtocolMgr::recvSavepointResult( &sSession->mSession,
                                                                   sSession->mId,
                                                                   &sResultCode,
                                                                   sTimeoutSec )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

                    break;
                }

                case DKT_LINKER_TYPE_SHARD:
                {
                    /* BUG-48489 Shard에서는 Savepoint를 코디네이터에서 관리하지 않는다. */
                    IDE_TEST( sGlobalCoordinator->executeSavepointForShard( aSavepoint )
                              != IDE_SUCCESS );

                    break;
                }

                default:
                    break;
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                 "[dkmSavepoint] aSession is NULL" ) );
    }
    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            NULL, 
                                            0, 
                                            0, 
                                            NULL );
    }
    IDE_EXCEPTION( ERR_CANNOT_SET_SAVEPOINT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_CANNOT_SET_SAVEPOINT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsSet == ID_TRUE )
    {
        (void)sGlobalCoordinator->removeSavepoint( aSavepoint );
    }
    else
    {
        /* savepoint was not set */
    }

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker process 의 상태정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_ALTILINKER_STATUS
 *
 *  aStatus     - [IN] AltiLinker process 의 상태정보를 담을 구조체
 *
 ************************************************************************/
static IDE_RC dkmGetAltiLinkerStatusInternal( dkmAltiLinkerStatus  *aStatus )
{
    dkaLinkerProcInfo   *sInfo;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST( dkaLinkerProcessMgr::getLinkerProcessInfo( &sInfo )
              != IDE_SUCCESS );

    aStatus->mStatus                = sInfo->mStatus;
    aStatus->mSessionCount          = sInfo->mLinkerSessionCnt;
    aStatus->mRemoteSessionCount    = sInfo->mRemoteNodeSessionCnt;
    aStatus->mJvmMemoryPoolMaxSize  = sInfo->mJvmMemMaxSize;
    aStatus->mJvmMemoryUsage        = sInfo->mJvmMemUsage;

    idlOS::memcpy( aStatus->mStartTime,
                   sInfo->mStartTime,
                   ID_SIZEOF( sInfo->mStartTime ));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkmGetAltiLinkerStatus( dkmAltiLinkerStatus  *aStatus )
{
#ifdef ALTIBASE_PRODUCT_XDB
    switch ( idwPMMgr::getProcType() )
    {
        case IDU_PROC_TYPE_DAEMON:
            IDE_TEST( dkmGetAltiLinkerStatusInternal( aStatus ) != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERROR_WRONG_PROCESS_TYPE );
            break;
    }

#else

    IDE_TEST( dkmGetAltiLinkerStatusInternal( aStatus ) != IDE_SUCCESS );

#endif /* ALTIBASE_PRODUCT_XDB */

    return IDE_SUCCESS;

#ifdef ALTIBASE_PRODUCT_XDB
    IDE_EXCEPTION( ERROR_WRONG_PROCESS_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_ALTILINKER_STATUS ) );
    }
#endif /* ALTIBASE_PRODUCT_XDB */
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Database link 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_DATABASE_LINK_INFO
 *
 *  aInfo       - [OUT] Database link 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] Database link 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetDatabaseLinkInfo( dkmDatabaseLinkInfo **aInfo,
                               UInt                 *aInfoCount )
{
#ifdef ALTIBASE_PRODUCT_XDB
    DK_UNUSED( aInfo );
    DK_UNUSED( aInfoCount );

    return IDE_FAILURE;
#else
    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST( dkoLinkInfoGetForPerformanceView( (dkoLinkInfo**)aInfo, aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif /* ALTIBASE_PRODUCT_XDB */
}

/************************************************************************
 * Description : Linker session 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_LINKER_SESSION_INFO
 *
 *  aInfo       - [OUT] Linker session 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] Linker control session, linker data session 을
 *                      모두 포함한 linker session 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetLinkerSessionInfo( dkmLinkerSessionInfo **aInfo,
                                UInt                  *aInfoCount )
{
    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST( dksSessionMgr::getLinkerSessionInfo( (dksLinkerSessionInfo**)aInfo, aInfoCount ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Linker session 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_LINKER_CONTROL_SESSION_INFO
 *
 *  aInfo       - [OUT] Linker control session 의 정보
 *
 ************************************************************************/
IDE_RC dkmGetLinkerControlSessionInfo( dkmLinkerControlSessionInfo **aInfo )
{
    dksCtrlSessionInfo  *sInfo = NULL;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "dkmGetLinkerControlSessionInfo::malloc::Info",
                          ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dksCtrlSessionInfo ),
                                       (void **)&sInfo,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST( dksSessionMgr::getLinkerCtrlSessionInfo( sInfo )
              != IDE_SUCCESS );

    *aInfo = (dkmLinkerControlSessionInfo *)sInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sInfo != NULL )
    {
        (void)iduMemMgr::free( sInfo );
        sInfo = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Linker session 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_LINKER_DATA_SESSION_INFO
 *
 *  aInfo       - [OUT] Linker data session 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] Linker data session 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetLinkerDataSessionInfo( dkmLinkerDataSessionInfo **aInfo,
                                    UInt                      *aInfoCount )
{
    IDE_TEST( dkmCheckDblinkOrShardMetaEnabled() != IDE_SUCCESS );

    IDE_TEST( dksSessionMgr::getLinkerDataSessionInfo( (dksDataSessionInfo**)aInfo, aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 수행중인 모든 global transaction 의 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_GLOBAL_TRANSACTION_INFO
 *
 *  aInfo       - [OUT] Global transaction 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] 모든 global transaction 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetGlobalTransactionInfo( dkmGlobalTransactionInfo **aInfo,
                                    UInt                      *aInfoCount )
{
    IDE_TEST( dktGlobalTxMgr::getAllGlobalTransactonInfo( (dktGlobalTxInfo**)aInfo, aInfoCount )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 수행중인 모든 remote transaction 의 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_REMOTE_TRANSACTION_INFO
 *
 *  aInfo       - [OUT] Remote transaction 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] 모든 remote transaction 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetRemoteTransactionInfo( dkmRemoteTransactionInfo **aInfo,
                                    UInt                      *aInfoCount )
{
    IDE_TEST( dktGlobalTxMgr::getAllRemoteTransactonInfo( (dktRemoteTxInfo**)aInfo,
                                                          aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 수행중인 모든 notifier transaction 의 정보를 얻어온다.
 *
 *  aInfo       - [OUT] Remote transaction 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] 모든 notifier transaction 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetNotifierTransactionInfo( dkmNotifierTransactionInfo ** aInfo,
                                      UInt                        * aInfoCount )
{
    IDE_TEST( dktGlobalTxMgr::getNotifierTransactionInfo( (dktNotifierTransactionInfo **)aInfo,
                                                          aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkmGetShardNotifierTransactionInfo( dkmNotifierTransactionInfo ** aInfo,
                                           UInt                        * aInfoCount )
{
    IDE_TEST( dktGlobalTxMgr::getShardNotifierTransactionInfo( (dktNotifierTransactionInfo **)aInfo,
                                                               aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 수행중인 모든 remote statement 의 정보를 얻어온다.
 *
 *      - Related Performance View : V$DBLINK_REMOTE_STATEMENT_INFO
 *
 *  aInfo       - [OUT] Remote statement 의 정보를 담고 있는 구조체 배열
 *  aInfoCount  - [OUT] Remote statement 의 갯수
 *
 ************************************************************************/
IDE_RC dkmGetRemoteStatementInfo( dkmRemoteStatementInfo    **aInfo,
                                  UInt                       *aInfoCount )
{
    UInt                 sRemoteStmtCnt;
    dktRemoteStmtInfo   *sInfo = NULL;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST( dktGlobalTxMgr::getAllRemoteStmtCount( &sRemoteStmtCnt )
              != IDE_SUCCESS );

    if ( sRemoteStmtCnt > 0 )
    {
        IDU_FIT_POINT_RAISE( "dkmGetRemoteTransactionInfo::malloc::Info",
                              ERR_MEMORY_ALLOC );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( dktRemoteStmtInfo ) * sRemoteStmtCnt,
                                           (void **)&sInfo,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        IDE_TEST( dktGlobalTxMgr::getAllRemoteStmtInfo( sInfo,
                                                        &sRemoteStmtCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    *aInfo      = (dkmRemoteStatementInfo *)sInfo;
    *aInfoCount = sRemoteStmtCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sInfo != NULL )
    {
        (void)iduMemMgr::free( sInfo );
        sInfo = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Performance view 를 위한 정보제공을 목적으로 할당받은
 *               aInfo 의 메모리를 해제한다.
 *
 *  aInfo       - [IN] 해제할 메모리
 *
 ************************************************************************/
IDE_RC dkmFreeInfoMemory( void  *aInfo )
{
    if ( aInfo != NULL )
    {
        IDE_TEST( iduMemMgr::free( aInfo ) != IDE_SUCCESS );
    }
    else
    {
        /* no information to free */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Global transaction id 를 얻어온다.
 *
 *  aQcStatement         - [IN] SM 으로부터 transaction id 를 얻어오기
 *                              위해 필요한 정보
 *  aGlobalTransactionId - [OUT] global transaction 의 id
 *
 ************************************************************************/
static IDE_RC dkmGetLocalTransactionId( void   *aQcStatement,
                                        UInt   *aLocalTransactionId )
{
    UInt            sLocalTransactionId = 0;
    smiStatement   *sSmiStatement = NULL;
    smiTrans       *sSmiTrans     = NULL;

    IDE_TEST( qciMisc::getSmiStatement( aQcStatement, &sSmiStatement )
              != IDE_SUCCESS );

    sSmiTrans             = sSmiStatement->getTrans();
    sLocalTransactionId  = sSmiTrans->getTransID();

    *aLocalTransactionId = sLocalTransactionId;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC dkmAllocStatement( void            *aQcStatement,
                                 dkmSession      *aSession,
                                 SChar           *aDblinkName,
                                 SChar           *aStmtStr,
                                 UInt             aStmtType,
                                 SLong           *aStatementId )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    idBool                   sIsRemoteStmtCreated   = ID_FALSE;
    idBool                   sIsGTxCreated          = ID_FALSE;
    idBool                   sIsRTxCreated          = ID_FALSE;
    UInt                     sLocalTxId                 = 0;
    UShort                   sRemoteNodeSessionId   = 0;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession  = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Create remote statement object :
      _____________________________________________________________
     |                                                             |
     |  해당 세션에 global coordinator 가 할당되어 있는지 검사하고 |
     | 없으면 새로 할당. 호출하는 순서는 다음과 같다.              |
     |                                                             |
     |      a. global coordinator                                  |
     |      b. remote transaction                                  |
     |      c. remote statement                                    |
     |_____________________________________________________________|

     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                              &sRemoteTx )
                  != IDE_SUCCESS );

        if ( sRemoteTx != NULL )
        {
            /* remote transaction already exists */
        }
        else
        {
            /* make a new remote transaction */
            IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                          sSession,
                                                          &sLinkObj,
                                                          &sRemoteTx )
                      != IDE_SUCCESS );

            sIsRTxCreated = ID_TRUE;
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
        }
    }
    else
    {
        IDE_TEST( dkmGetLocalTransactionId( aQcStatement, &sLocalTxId )
                  != IDE_SUCCESS );

        IDE_TEST( dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( sSession,
                                                                            sLocalTxId,
                                                                            &sGlobalCoordinator )
                  != IDE_SUCCESS );
        sIsGTxCreated = ID_TRUE;

        IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                      sSession,
                                                      &sLinkObj,
                                                      &sRemoteTx )
                  != IDE_SUCCESS );

        sIsRTxCreated = ID_TRUE;
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
    }

    IDE_TEST( sRemoteTx->createRemoteStmt( aStmtType, 
                                           aStmtStr, 
                                           &sRemoteStmt ) 
              != IDE_SUCCESS );

    /* BUG-37630 */
    sIsRemoteStmtCreated = ID_TRUE;
    sRemoteTx->setStatus( DKT_RTX_STATUS_BEGIN );


    /*
       3. Prepare protocol operation
     */
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_TRUE;

    IDE_TEST( sRemoteStmt->prepare( &sSession->mSession,
                                    sSession->mId,
                                    sIsRTxCreated,
                                    &(sRemoteTx->mXID),
                                    &sLinkObj,
                                    &sRemoteNodeSessionId )
              != IDE_SUCCESS );

    sRemoteTx->setRemoteNodeSessionId( sRemoteNodeSessionId );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_FALSE;

    *aStatementId = sRemoteStmt->getStmtId();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    /* >> BUG-37630 */
    if ( sIsRemoteStmtCreated == ID_TRUE )
    {
        (void)sRemoteStmt->abortProtocol( &sSession->mSession, sSession->mId );
        sRemoteTx->destroyRemoteStmt( sRemoteStmt );
    }
    else
    {
        /* do nothing */
    }
    /* << BUG-37630 */

    /* >> BUG-37663 */
    if ( sIsRTxCreated == ID_TRUE )
    {
        if ( sGlobalCoordinator->checkAndCloseRemoteTx( sRemoteTx ) == IDE_SUCCESS )
        {
            sGlobalCoordinator->destroyRemoteTx( sRemoteTx );
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
        /* check global coordinator created */
    }

    if ( ( sIsGTxCreated == ID_TRUE ) && ( sGlobalCoordinator->getRemoteTxCount() == 0 ) )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }
    else
    {
        /* done */
    }
    /* << BUG-37663 */

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;    
}
    
/************************************************************************
 * Description : Remote statement 를 remote server 에 prepare 시킨다.
 *
 *  aQcStatement    - [IN] SM 으로부터 transaction id 를 얻어오기 위해 
 *                         필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStmtStr        - [IN] Remote statement 문자열 구문
 *  aStatementId    - [OUT] Remote statement id 
 *
 ************************************************************************/
IDE_RC dkmCalculateAllocStatement( void            *aQcStatement,
                                   dkmSession      *aSession,
                                   SChar           *aDblinkName,
                                   SChar           *aStmtStr,
                                   SLong           *aStatementId )
{
    IDE_TEST( dkmAllocStatement( aQcStatement,
                                 aSession,
                                 aDblinkName,
                                 aStmtStr,
                                 DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT,
                                 aStatementId )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkmCalculateAllocStatementBatch( void            *aQcStatement,
                                        dkmSession      *aSession,
                                        SChar           *aDblinkName,
                                        SChar           *aStmtStr,
                                        SLong           *aStatementId )
{
    IDE_TEST( dkmAllocStatement( aQcStatement,
                                 aSession,
                                 aDblinkName,
                                 aStmtStr,
                                 DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                                 aStatementId )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 'REMOTE_EXECUTE_STATEMENT' 를 수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStatementId    - [IN] Remote statement id
 *  aResult         - [OUT] Remote statement 의 수행결과
 *
 ************************************************************************/
IDE_RC dkmCalculateExecuteStatement( void           *aQcStatement,
                                     dkmSession     *aSession,
                                     SChar          *aDblinkName,
                                     SLong           aStatementId,
                                     SInt           *aResult )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    SInt                     sResult;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Check statement id and type
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_REMOTE_STMT );

	
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    sStmtType = sRemoteStmt->getStmtType();

    IDE_TEST_RAISE( ( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT ) &&
                    ( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT ) &&
                    ( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_NON_QUERY_STATEMENT ),
                    ERR_INVALID_REMOTE_STMT_TYPE );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_TRUE;

    /*
       3. Execute protocol operation
     */
    IDE_TEST( sRemoteStmt->executeProtocol( &sSession->mSession,
                                            sSession->mId,
                                            ID_FALSE, /* remoteTx가 Begin된 상태이므로 */
                                            &(sRemoteTx->mXID),
                                            &sLinkObj,
                                            &sResult )
              != IDE_SUCCESS );

    if ( sRemoteStmt->getStmtType() == DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT )
    {
        IDE_TEST( sRemoteStmt->createDataManager() != IDE_SUCCESS );

        IDE_TEST( sRemoteStmt->requestResultSetMetaProtocol( &sSession->mSession,
                                                             sSession->mId )
                  != IDE_SUCCESS );

        IDE_TEST( sRemoteStmt->activateRemoteDataManager( aQcStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sRemoteStmt->fetchProtocol( &sSession->mSession,
                                              sSession->mId,
                                              aQcStatement,
                                              ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* non query */
        *aResult = sResult;
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_FALSE;

    /*
       4. Check All remote transactions are preapared
     */
    if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
    {
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkmCalculateAddBatch( void * aQcStatement,
                             dkmSession * aSession,
                             SChar * aDblinkName,
                             SLong aStatementId,
                             SInt * aResult )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    SInt                     sResult                = 0;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON, 
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() 
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    /* 
       1. Validate DB-Link object 
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj ) 
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement, 
                                                       sSession, 
                                                       &sLinkObj ) 
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /* 
       2. Check statement id and type
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId, 
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_REMOTE_STMT );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx ) 
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT ); 

    sStmtType = sRemoteStmt->getStmtType();

    IDE_TEST_RAISE( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                    ERR_INVALID_REMOTE_STMT_TYPE ); 

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_TRUE;

    /*
       3. Execute protocol operation
     */
    IDE_TEST( sRemoteStmt->addBatch() != IDE_SUCCESS );
    *aResult = sResult;

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_FALSE;

    /*
       4. Check All remote transactions are preapared
     */
    if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
    {
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkmCalculateAddBatch] aSession is NULL" ) );
    }
    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* do nothing */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }
    
    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif
    
    IDE_POP();

    return IDE_FAILURE;    
}

/*
 *
 */ 
IDE_RC dkmCalculateExecuteBatch( void * aQcStatement,
                                 dkmSession * aSession,
                                 SChar * aDblinkName,
                                 SLong aStatementId,
                                 SInt * aResult )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    SInt                     sResult                = 0;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON, 
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() 
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );
    
    /* 
       1. Validate DB-Link object 
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj ) 
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement, 
                                                       sSession, 
                                                       &sLinkObj ) 
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /* 
       2. Check statement id and type
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId, 
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_REMOTE_STMT );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx ) 
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT ); 

    sStmtType = sRemoteStmt->getStmtType();

    IDE_TEST_RAISE( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                    ERR_INVALID_REMOTE_STMT_TYPE ); 

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_TRUE;

    /*
       3. Execute protocol operation
     */
    IDE_TEST( sRemoteStmt->executeBatchProtocol( &sSession->mSession, 
                                                 sSession->mId, 
                                                 &sResult )
              != IDE_SUCCESS );
    *aResult = sResult;
        
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_FALSE;

    /*
       4. Check All remote transactions are preapared
     */
    if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
    {
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkmCalculateExecuteBatch] aSession is NULL"  ) );
    }
    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* do nothing */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }
    
    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif
    
    IDE_POP();

    return IDE_FAILURE;    
}


/************************************************************************
 * Description : 'REMOTE_EXECUTE_IMMEDIATE' 를 수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStmtStr        - [IN] Remote statement 구문 문자열
 *  aResult         - [OUT] Remote statement 의 수행결과
 *
 ************************************************************************/
IDE_RC dkmCalculateExecuteImmediate( void           *aQcStatement,
                                     dkmSession     *aSession,
                                     SChar          *aDblinkName,
                                     SChar          *aStmtStr,
                                     SInt           *aResult )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    idBool                   sIsGTxCreated          = ID_FALSE;
    idBool                   sIsRTxCreated          = ID_FALSE;
    idBool                   sIsRemoteStmtCreated   = ID_FALSE;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    UInt                     sLocalTxId                 = 0;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession  = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    sStmtType = DKT_STMT_TYPE_REMOTE_EXECUTE_IMMEDIATE;

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Alloc remote statement
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                              &sRemoteTx )
                  != IDE_SUCCESS );

        if ( sRemoteTx == NULL )
        {
            /* make a new remote transaction */
            IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                          sSession,
                                                          &sLinkObj,
                                                          &sRemoteTx )
                      != IDE_SUCCESS );

            sIsRTxCreated = ID_TRUE;
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
        }
        else
        {
            /* remote transaction already exists */
        }
    }
    else
    {
        IDE_TEST( dkmGetLocalTransactionId( aQcStatement, &sLocalTxId )
                  != IDE_SUCCESS );

        /* this data session tries to execute remote stmt firstly */
        IDE_TEST( dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( sSession,
                                                                            sLocalTxId,
                                                                            &sGlobalCoordinator )
                  != IDE_SUCCESS );
        sIsGTxCreated = ID_TRUE;

        IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                      sSession,
                                                      &sLinkObj,
                                                      &sRemoteTx )
                  != IDE_SUCCESS );

        sIsRTxCreated = ID_TRUE;
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
    }

    IDE_TEST( sRemoteTx->createRemoteStmt( sStmtType,
                                           aStmtStr,
                                           &sRemoteStmt )
              != IDE_SUCCESS );

    sIsRemoteStmtCreated = ID_TRUE;
    sRemoteTx->setStatus( DKT_RTX_STATUS_BEGIN );

    /*
       3. Execute remote statement
     */
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_TRUE;

    IDE_TEST( sRemoteStmt->executeProtocol( &sSession->mSession,
                                            sSession->mId,
                                            sIsRTxCreated,
                                            &(sRemoteTx->mXID),
                                            &sLinkObj,
                                            aResult )
              != IDE_SUCCESS );

    /*
       4. Free statement
     */
    IDE_TEST( sRemoteStmt->free( &sSession->mSession,
                                 sSession->mId )
              != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_FALSE;

    sRemoteTx->destroyRemoteStmt( sRemoteStmt );
    sIsRemoteStmtCreated = ID_FALSE;

    if ( sRemoteTx->isEmptyRemoteTx() == ID_TRUE )
    {
        sRemoteTx->setStatus( DKT_RTX_STATUS_PREPARE_READY );

        if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
        {
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
        }
        else
        {
            /* global transaction is on going */
        }
    }
    else
    {
        /* remote transaction is on going */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsRemoteStmtCreated == ID_TRUE )
    {
        (void)sRemoteStmt->abortProtocol( &sSession->mSession, sSession->mId );
        (void)sRemoteTx->destroyRemoteStmt( sRemoteStmt );
    }
    else
    {
        /* check remote transaction created */
    }

    if ( sIsRTxCreated == ID_TRUE )
    {
        if ( sGlobalCoordinator->checkAndCloseRemoteTx( sRemoteTx ) == IDE_SUCCESS )
        {
            sGlobalCoordinator->destroyRemoteTx( sRemoteTx );
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
        /* do nothing */
    }

    if ( ( sIsGTxCreated == ID_TRUE ) && ( sGlobalCoordinator->getRemoteTxCount() == 0 ) )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }
    else
    {
        /* done */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkmBindVariable( void           *aQcStatement,
                               dkmSession     *aSession,
                               SChar          *aDblinkName,
                               SLong           aStatementId,
                               UInt            aVariableIndex,
                               SChar          *aValue )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    UInt                     sBindVarLen;
    UInt                     sBindVarType;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Check status
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    /*
       3. Find remote statement
     */
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_REMOTE_STMT_NOT_EXIST );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_TRUE;

    /*
       4. Execute bind
     */
    sBindVarLen  = idlOS::strlen( aValue );
    sBindVarType = DKP_COL_TYPE_VARCHAR;

    IDE_TEST( sRemoteStmt->bind( &sSession->mSession, 
                                 sSession->mId, 
                                 aVariableIndex, 
                                 sBindVarType, 
                                 sBindVarLen, 
                                 aValue )
              != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* do nothing */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    IDE_POP();

    return IDE_FAILURE;    
}

/************************************************************************
 * Description : Remote statement 에 대해 bind 를 수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStatementId    - [IN] Remote statement id
 *  aVariableIndex  - [IN] Bind variable 의 순서상 상대위치를 가리키는 
 *                         index
 *  aValue          - [IN] Bind value
 *
 ************************************************************************/
IDE_RC dkmCalculateBindVariable( void           *aQcStatement,
                                 dkmSession     *aSession,
                                 SChar          *aDblinkName,
                                 SLong           aStatementId,
                                 UInt            aVariableIndex,
                                 SChar          *aValue )
{
    IDE_TEST( dkmBindVariable( aQcStatement,
                               aSession,
                               aDblinkName,
                               aStatementId,
                               aVariableIndex,
                               aValue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkmCalculateBindVariableBatch( void           *aQcStatement,
                                      dkmSession     *aSession,
                                      SChar          *aDblinkName,
                                      SLong           aStatementId,
                                      UInt            aVariableIndex,
                                      SChar          *aValue )
{
    IDE_TEST( dkmBindVariable( aQcStatement,
                               aSession,
                               aDblinkName,
                               aStatementId,
                               aVariableIndex,
                               aValue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static  IDE_RC dkmFreeStatement( void           *aQcStatement,
                                 dkmSession     *aSession,
                                 SChar          *aDblinkName,
                                 SLong           aStatementId )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Check status
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    /*
       3. Find remote statement
     */
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_TRUE;

    /*
       4. Execute free remote statement protocol
     */
    IDE_TEST( sRemoteStmt->free( &sSession->mSession,
                                 sSession->mId )
              != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_FALSE;

    /*
       5. Destroy remote statement object
     */
    sRemoteTx->destroyRemoteStmt( sRemoteStmt );

    if ( sRemoteTx->isEmptyRemoteTx() == ID_TRUE )
    {
        sRemoteTx->setStatus( DKT_RTX_STATUS_PREPARE_READY );

        if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
        {
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
        }
        else
        {
            /* global transaction is on going */
        }
    }
    else
    {
        /* remote transaction is on going */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 'REMOTE_FREE_STATEMENT' 를 수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStatementId    - [IN] Remote statement id
 *
 ************************************************************************/
IDE_RC dkmCalculateFreeStatement( void           *aQcStatement,
                                  dkmSession     *aSession,
                                  SChar          *aDblinkName,
                                  SLong           aStatementId )
{
    IDE_TEST( dkmFreeStatement( aQcStatement,
                                aSession,
                                aDblinkName,
                                aStatementId )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkmCalculateFreeStatementBatch( void           *aQcStatement,
                                       dkmSession     *aSession,
                                       SChar          *aDblinkName,
                                       SLong           aStatementId )
{
    IDE_TEST( dkmFreeStatement( aQcStatement,
                                aSession,
                                aDblinkName,
                                aStatementId )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkmCalculateGetResultCountBatch( void * aQcStatement,
                                        dkmSession * aSession,
                                        SChar * aDblinkName,
                                        SLong aStatementId,
                                        SInt * aResultCount )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON, 
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() 
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    /* 
       1. Validate DB-Link object 
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj ) 
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement, 
                                                       sSession, 
                                                       &sLinkObj ) 
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /* 
       2. Check statement id and type
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId, 
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_REMOTE_STMT );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT ); 

    sStmtType = sRemoteStmt->getStmtType();

    IDE_TEST_RAISE( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                    ERR_INVALID_REMOTE_STMT_TYPE ); 

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_TRUE;

    /*
       3. Execute protocol operation
     */
    IDE_TEST( sRemoteStmt->getResultCountBatch( aResultCount )
              != IDE_SUCCESS );
        
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_FALSE;

    /*
       4. Check All remote transactions are preapared
     */
    if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
    {
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkmCalculateGetResultCountBatch] aSession is NULL" ) );
    }
    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
        sSession = NULL;
    }
    else
    {
        /* do nothing */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }
    
    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif
    
    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkmCalculateGetResultBatch( void * aQcStatement,
                                   dkmSession * aSession,
                                   SChar * aDblinkName,
                                   SLong aStatementId,
                                   SInt aIndex,
                                   SInt * aResult )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON, 
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() 
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    /* 
       1. Validate DB-Link object 
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj ) 
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement, 
                                                       sSession, 
                                                       &sLinkObj ) 
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /* 
       2. Check statement id and type
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId, 
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_REMOTE_STMT );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT ); 

    sStmtType = sRemoteStmt->getStmtType();

    IDE_TEST_RAISE( sStmtType != DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
                    ERR_INVALID_REMOTE_STMT_TYPE ); 

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_TRUE;

    /*
       3. Execute protocol operation
     */
    IDE_TEST( sRemoteStmt->getResultBatch( aIndex, aResult )
              != IDE_SUCCESS );
        
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId ) 
              != IDE_SUCCESS );
#endif
    
    sIsLinkObjUsed = ID_FALSE;

    /*
       4. Check All remote transactions are preapared
     */
    if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
    {
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkmCalculateGetResultBatch] aSession is NULL" ) );
    }
    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* do nothing */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }
    
    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif
    
    IDE_POP();

    return IDE_FAILURE;
}



/************************************************************************
 * Description : 'REMOTE_NEXT_ROW' 를 수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStatementId    - [IN] Remote statement id
 *  aResult         - [OUT] Remote statement 의 수행결과
 *                          0 : row 가 존재하는 경우
 *                         -1 : 더 이상의 row 가 존재하지 않는 경우
 *
 ************************************************************************/
extern IDE_RC dkmCalculateNextRow( void         *aQcStatement,
                                   dkmSession   *aSession,
                                   SChar        *aDblinkName,
                                   SLong         aStatementId,
                                   SInt         *aResult )
{
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    dkdDataMgr              *sDataMgr               = NULL;
    idBool                   sEndFlag               = ID_FALSE;
    idvSQL                  *sStatistics            = NULL;

    *aResult = -1;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Check global coordinator's status
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    /*
       3. Find remote statement
     */
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    IDE_TEST_RAISE( sRemoteStmt->getStmtType() != DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT,
                    ERR_INVALID_REMOTE_STMT_TYPE );

    /* 4. Move next */
    sDataMgr = sRemoteStmt->getDataMgr();
    IDE_TEST( sDataMgr->moveNextRow( &sEndFlag ) != IDE_SUCCESS );

    if ( sEndFlag != ID_TRUE )
    {
        *aResult = 0;
    }
    else
    {
        *aResult = -1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 'REMOTE_GET_COLUMN_VALUE_type' 를 수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aStatementId    - [IN] Remote statement id
 *  aColumnIndex    - [IN] Column index
 *  aColumn         - [OUT] mtcColumn 정보
 *  aColumnValue    - [OUT] Column value
 *
 ************************************************************************/
IDE_RC dkmCalculateGetColumnValue( void         *aQcStatement,
                                   dkmSession   *aSession,
                                   SChar        *aDblinkName,
                                   SLong         aStatementId,
                                   UInt          aColumnIndex,
                                   mtcColumn   **aColumn,
                                   void        **aColumnValue )
{
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    dkdDataMgr              *sDataMgr               = NULL;
    dkdTypeConverter        *sTypeConverter         = NULL;
    mtcColumn               *sColMetaArr            = NULL;
    UInt                     sColumnCount           = 0;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Check status
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    /*
       3. Find remote statement
     */
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    IDE_TEST_RAISE( sRemoteStmt->getStmtType() !=
                    DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT,
                    ERR_INVALID_REMOTE_STMT_TYPE );

    /*
       4. Get column value
     */
    sDataMgr       = sRemoteStmt->getDataMgr();
    sTypeConverter = sDataMgr->getTypeConverter();
    IDE_TEST_RAISE( sTypeConverter == NULL, ERR_GET_TYPE_CONVETER_NULL );

    IDE_TEST_RAISE( dkdTypeConverterGetConvertedMeta( sTypeConverter,
                                                      &sColMetaArr )
                    != IDE_SUCCESS, ERR_INVALID_COLUMN );

    IDE_TEST( dkdTypeConverterGetColumnCount( sTypeConverter,
                                              &sColumnCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (aColumnIndex == 0) || (aColumnIndex > sColumnCount),
                    ERR_INVALID_COLUMN_INDEX );

    *aColumn      = &sColMetaArr[aColumnIndex - 1];
    *aColumnValue = sDataMgr->getColValue( (*aColumn)->column.offset );

    IDE_TEST_RAISE( *aColumnValue == NULL, ERR_GET_COL_VAL_NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT_TYPE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT_TYPE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_COLUMN );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_COLUMN ) );
    }
    IDE_EXCEPTION( ERR_INVALID_COLUMN_INDEX );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_COLUMN_INDEX ) );
    }
    IDE_EXCEPTION( ERR_GET_COL_VAL_NULL );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_COLUMN_VALUE_NULL ) );
    }
    IDE_EXCEPTION( ERR_GET_TYPE_CONVETER_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, 
                                  "[dkmCalculateGetColumnValue] sTypeConverter is NULL" ) ); 
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 'REMOTE_GET_ERROR_INFO' 를 수행한다.
 *
 *  aQcStatement        - [IN] Link object validation 에 필요한 정보
 *  aSession            - [IN] Linker data session
 *  aDblinkName         - [IN] Database link name
 *  aStatementId        - [IN] Remote statement id
 *  aErrorInfoBuffer    - [IN/OUT] Error 정보를
 *  aErrorInfoBufferSize- [IN] Error info buffer 의 크기
 *  aErrorInfoLength    - [OUT] 실제 error 정보의 길이
 *
 ************************************************************************/
IDE_RC dkmCalculateGetRemoteErrorInfo( void         *aQcStatement,
                                       dkmSession   *aSession,
                                       SChar        *aDblinkName,
                                       SLong         aStatementId,
                                       SChar        *aErrorInfoBuffer,
                                       UInt          aErrorInfoBufferSize,
                                       UShort       *aErrorInfoLength )
{
    dkoLink                  sLinkObj;
    dksDataSession          *sSession           = NULL;
    dktRemoteStmt           *sRemoteStmt        = NULL;
    dktRemoteTx             *sRemoteTx          = NULL;
    dktErrorInfo            *sErrorInfo         = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    idvSQL                  *sStatistics        = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Find remote statement
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                          &sRemoteTx )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteTx == NULL, ERR_INVALID_TRANSACTION );

    sRemoteStmt = sRemoteTx->findRemoteStmt( aStatementId );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    /*
       3. Get error information
     */
    IDE_TEST_RAISE( sRemoteStmt->getStmtResult() != DKP_RC_FAILED_REMOTE_SERVER_ERROR,
                    ERR_GET_ERROR_INFO );

    sErrorInfo = sRemoteStmt->getErrorInfo();

    IDE_TEST_RAISE( idlOS::strcmp( sErrorInfo->mSQLState, "00000" ) == 0,
                    ERR_REMOTE_STMT_IS_NOT_FAILED );

    /*
       4. Write error information into a buffer
     */
    if ( sErrorInfo->mErrorDesc != NULL )
    {
        idlOS::snprintf( aErrorInfoBuffer,
                         aErrorInfoBufferSize,
                         PDL_TEXT( "[ %"ID_INT32_FMT
                                   ": %s ] %s" ),
                         sErrorInfo->mErrorCode,
                         sErrorInfo->mSQLState,
                         sErrorInfo->mErrorDesc );

    }
    else
    {
        idlOS::snprintf( aErrorInfoBuffer,
                         aErrorInfoBufferSize,
                         PDL_TEXT( "[ %"ID_INT32_FMT
                                   ": %s ] No description." ),
                         sErrorInfo->mErrorCode,
                         sErrorInfo->mSQLState );
    }

    *aErrorInfoLength = (UShort)idlOS::strlen( aErrorInfoBuffer );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_GET_ERROR_INFO );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_GET_ERROR_INFO ) );
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_IS_NOT_FAILED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_REMOTE_STMT_IS_NOT_FAILED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkmStartAltilinkerProcessInternal( void )
{
    SInt             sPortNo          = 0;
    idBool           sConnected       = ID_FALSE;
    idBool           sNotifierConnected = ID_FALSE;
    iduListNode     *sIterator        = NULL;
    iduList         *sDataSessionList = NULL;
    dksDataSession  *sDataSession     = NULL;
    dktNotifier     *sNotifier        = NULL;
    dksDataSession  *sNotifyDataSession = NULL;

    if ( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() == ID_TRUE )
    {
        IDE_TEST_RAISE( dkaLinkerProcessMgr::startLinkerProcess()
                        != IDE_SUCCESS, ERR_START_ALTILINKER_PROCESS );

        while ( dkaLinkerProcessMgr::getLinkerStatus()
                != DKA_LINKER_STATUS_ACTIVATED )
        {
            /* nothing to do */
        }

        IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerPortNoFromConf( &sPortNo )
                  != IDE_SUCCESS );

        IDE_TEST( dksSessionMgr::connectControlSession( (UShort)sPortNo )
                  != IDE_SUCCESS );
        sConnected = ID_TRUE;

        sNotifyDataSession = dksSessionMgr::getLinkerNotifyDataSession();
        IDE_TEST( dksSessionMgr::invalidateLinkerDataSession( sNotifyDataSession )
                  != IDE_SUCCESS );
        sNotifyDataSession->mSession.mPortNo = sPortNo;

        IDE_TEST( dksSessionMgr::connectDataSession( sNotifyDataSession )
                  != IDE_SUCCESS );
        sNotifierConnected = ID_TRUE;

        sNotifier = dktGlobalTxMgr::getNotifier();
        sNotifier->setSession( dksSessionMgr::getNotifyDataDksSession() );
        sNotifier->setPause( ID_FALSE );

        sDataSessionList = dksSessionMgr::getDataSessionList();

        IDU_LIST_ITERATE( sDataSessionList, sIterator )
        {
            sDataSession = (dksDataSession *)sIterator->mObj;
            IDE_TEST( dksSessionMgr::invalidateLinkerDataSession( sDataSession )
                      != IDE_SUCCESS );
        }

        dkaLinkerProcessMgr::setLinkerStatus( DKA_LINKER_STATUS_READY );
    }
    else
    {
        /* Only use homogeneous link */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_START_ALTILINKER_PROCESS );
    {
        ideLog::log(IDE_DK_0, "Fail to start Altilinker process" );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sConnected == ID_TRUE )
    {
        (void)dksSessionMgr::disconnectControlSession();
    }
    else
    {
        /* do nothing */
    }

    if ( sNotifierConnected == ID_TRUE )
    {
        (void)dksSessionMgr::disconnectDataSession( sNotifyDataSession );
        sNotifyDataSession = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스를 시작시킨다.
 *
 ************************************************************************/
IDE_RC dkmStartAltilinkerProcess( void )
{

    UInt             sLinkerStatus    = DKA_LINKER_STATUS_NON;
    dktNotifier     *sNotifier        = NULL;

    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        sLinkerStatus = dkaLinkerProcessMgr::getLinkerStatus();

        if ( sLinkerStatus == DKA_LINKER_STATUS_CLOSING )
        {
            dkaLinkerProcessMgr::setLinkerStatus( DKA_LINKER_STATUS_NON );
        }
        else
        {
            IDE_TEST_RAISE( sLinkerStatus != DKA_LINKER_STATUS_NON,
                            ERR_LINKER_ALREADY_RUNNING );
        }

        IDE_TEST( dkaLinkerProcessMgr::loadLinkerProperties() != IDE_SUCCESS );

        IDE_TEST( dkmStartAltilinkerProcessInternal() != IDE_SUCCESS );

    }
    else
    {
        if ( sdi::isShardEnable() == ID_TRUE )
        {
            sNotifier = dktGlobalTxMgr::getNotifier();
            sNotifier->setPause( ID_FALSE );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_ALREADY_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_ALREAY_RUNNING ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( dkaLinkerProcessMgr::getLinkerStatus() < DKA_LINKER_STATUS_READY )
    {
        (void)dkaLinkerProcessMgr::setLinkerStatus( DKA_LINKER_STATUS_NON );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스를 종료시킨다.
 *
 ************************************************************************/
IDE_RC dkmStopAltilinkerProcess( idBool aForceFlag )
{
    dksDataSession * sNotifyDataSession = NULL;
    dktNotifier    * sNotifier = NULL;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_IS_NOT_RUNNING );

    sNotifier = dktGlobalTxMgr::getNotifier();
    sNotifier->setPause( ID_TRUE );

    if ( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() == ID_TRUE )
    {
        IDE_TEST( dkaLinkerProcessMgr::shutdownLinkerProcess( aForceFlag )
                  != IDE_SUCCESS );

        sNotifyDataSession = dksSessionMgr::getLinkerNotifyDataSession();
        (void)dksSessionMgr::disconnectDataSession( sNotifyDataSession );
    }
    else
    {
        /* Altilinker process disabled */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_IS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스의 정보를 altibase_lk_dump.log에 출력한다.
 *
 ************************************************************************/
IDE_RC dkmDumpAltilinkerProcess( void )
{
    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_IS_NOT_RUNNING );

    if ( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() == ID_TRUE )
    {
        IDE_TEST( dkaLinkerProcessMgr::dumpStackTrace()
                  != IDE_SUCCESS );
    }
    else
    {
        /* Altilinker process disabled */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_IS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 'ALTER SESSION CLOSE DATABASE LINK ALL' 를 수행한다.
 *
 *  aSession            - [IN] Linker data session
 *
 ************************************************************************/
IDE_RC dkmCloseSessionAll( idvSQL * aStatistics, dkmSession   *aSession )
{
    dksDataSession  *sSession = NULL;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    if ( aSession != NULL )
    {
        sSession = (dksDataSession *)aSession;

        if ( dkmDblinkStarted() == ID_TRUE )
        {
            IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( dksSessionMgr::closeRemoteNodeSession( aStatistics, sSession, NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* session not exist */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 'ALTER SESSION CLOSE DATABASE LINK dblink_name'
 *               를 수행한다.
 *
 *  aSession    - [IN] Linker data session
 *  aDblinkName - [IN] Database link name
 *
 ************************************************************************/
IDE_RC dkmCloseSession( idvSQL       *aStatistics,
                        dkmSession   *aSession,
                        SChar        *aDblinkName )
{
    dksDataSession  *sSession = NULL;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( aDblinkName == NULL, ERR_INTERNAL_ERROR );

    sSession = (dksDataSession *)aSession;

    if ( sSession != NULL )
    {
        if ( dkmDblinkStarted() == ID_TRUE )
        {
            IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( dksSessionMgr::closeRemoteNodeSession( aStatistics, sSession, aDblinkName )
                  != IDE_SUCCESS );
    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "aDblinkName is NULL" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* session not exist */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 생성할 데이터베이스 링크 객체를 위해 validation 을
 *               수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aPublicFlag     - [IN] 생성할 데이터베이스 링크의 user mode
 *      @ Database link user mode:
 *          1. PUBLIC  : ID_TRUE / owner 이외의 사용자가 사용가능
 *          2. PRIVATE : ID_FALSE / owner 혹은 sysdba 만 사용가능
 *
 *  aDblinkName     - [IN] Database link name
 *  aUserID         - [IN] Target server 에 접속하기 위한 사용자 id
 *  aPassword       - [IN] Target server 에 접속하기 위한 사용자 password
 *  aTargetName     - [IN] 생성할 데이터베이스 링크가 가리킬 target
 *                         의 이름
 *
 ************************************************************************/
IDE_RC dkmValidateCreateDatabaseLink( void      *aQcStatement,
                                      idBool     aPublicFlag,
                                      SChar     *aDblinkName,
                                      SChar     */*aUserID*/,
                                      SChar     */*aPassword*/,
                                      SChar     *aTargetName )
{
    idBool  sExistFlag = ID_FALSE;
    idvSQL *sStatistics = NULL;

    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    /* 1. Check dblink object */
    IDE_TEST( dkoLinkObjMgr::isExistLinkObject( sStatistics,
                                                aDblinkName,
                                                &sExistFlag )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sExistFlag == ID_TRUE, ERR_DBLINK_ALREADY_EXIST );

    /* 2. Check Authorization */
    IDE_TEST( qciMisc::checkCreateDatabaseLinkPrivilege( aQcStatement,
                                                         aPublicFlag )
              != IDE_SUCCESS );

    /* 3. Check target information */
    IDE_TEST_RAISE( dkaLinkerProcessMgr::isPropertiesLoaded() != ID_TRUE,
                    ERR_DBLINK_PROPERTIES_LOAD_FAILED );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::validateTargetInfo( aTargetName )
                    != ID_TRUE, ERR_INVALID_TARGET );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_ALREADY_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_ALREADY_EXIST ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_PROPERTIES_LOAD_FAILED );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_PROPERTIES_LOAD_FAILED ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TARGET );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TARGET ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 데이터베이스 링크 객체를 생성한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aPublicFlag     - [IN] 생성할 데이터베이스 링크의 user mode
 *      @ Database link user mode:
 *          1. PUBLIC  : ID_TRUE / owner 이외의 사용자가 사용가능
 *          2. PRIVATE : ID_FALSE / owner 혹은 sysdba 만 사용가능
 *
 *  aDblinkName     - [IN] Database link name
 *  aRemoteUserID   - [IN] Target server 에 접속하기 위한 사용자 id
 *  aPassword       - [IN] Target server 에 접속하기 위한 사용자 password
 *  aTargetName     - [IN] 생성할 데이터베이스 링크가 가리킬 target
 *                         의 이름
 *
 ************************************************************************/
IDE_RC dkmExecuteCreateDatabaseLink( void       *aQcStatement,
                                     idBool      aPublicFlag,
                                     SChar      *aDblinkName,
                                     SChar      *aRemoteUserID,
                                     SChar      *aPassword,
                                     SChar      *aTargetName )
{
    UInt                    sNewDblinkId;
    UInt                    sUserId;
    SInt                    sUserMode;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    /* 1. Get user id and user mode */
    if ( aPublicFlag == ID_TRUE )
    {
        sUserMode = DKO_LINK_USER_MODE_PUBLIC;
        sUserId   = 0;
    }
    else
    {
        sUserMode = DKO_LINK_USER_MODE_PRIVATE;
        sUserId   = qciMisc::getSessionUserID( aQcStatement );
    }

    /* 2. Get a new dblink id */
    IDE_TEST( qciMisc::getNewDatabaseLinkId( aQcStatement, &sNewDblinkId )
              != IDE_SUCCESS );

    /* 3. Create dblink object */
    IDE_TEST( dkoLinkObjMgr::createLinkObject( aQcStatement,
                                               sUserId,
                                               sNewDblinkId,
                                               0, /* Heterogeneous link */
                                               sUserMode,
                                               aDblinkName,
                                               aTargetName,
                                               aRemoteUserID,
                                               aPassword,
                                               aPublicFlag )
              != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoSetStatus( sNewDblinkId, DKO_LINK_OBJ_CREATED )
              != IDE_SUCCESS );

    IDE_TEST( dkoLinkInfoSetStatus( sNewDblinkId, DKO_LINK_OBJ_META )
              != IDE_SUCCESS );

    /* 6. Update related views */
    IDE_TEST( dkoLinkInfoSetStatus( sNewDblinkId, DKO_LINK_OBJ_READY )
              != IDE_SUCCESS );
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 제거할 데이터베이스 링크 객체에 대해 validation 을
 *               수행한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aPublicFlag     - [IN] 제거할 데이터베이스 링크의 user mode
 *  aDblinkName     - [IN] Database link name
 *
 ************************************************************************/
IDE_RC dkmValidateDropDatabaseLink( void    *aQcStatement,
                                    idBool   aPublicFlag,
                                    SChar   *aDblinkName )
{
    dkoLink     sLinkObj;
    idvSQL     *sStatistics = NULL;

    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    /* 1. Validate dblink object */
    IDE_TEST( dkoLinkObjMgr::findLinkObject( sStatistics,
                                             aDblinkName,
                                             &sLinkObj )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aPublicFlag != dkoLinkObjMgr::isPublicLink( &sLinkObj ),
                    ERR_INVALID_USER_MODE );

    /* 2. Check Authorization */
    IDE_TEST( qciMisc::checkDropDatabaseLinkPrivilege( aQcStatement,
                                                       aPublicFlag )
              != IDE_SUCCESS );

    /* 3. Check Ownership */
    if ( aPublicFlag == ID_FALSE )
    {
        if ( qciMisc::isSysdba( aQcStatement ) == ID_TRUE )
        {
            /* can drop */
        }
        else
        {
            IDE_TEST_RAISE( qciMisc::getSessionUserID( aQcStatement )
                            != sLinkObj.mUserId, ERR_INVALID_USER );
        }
    }
    else
    {
        /* public */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_USER_MODE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_USER_MODE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_USER );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_USER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 데이터베이스 링크 객체를 제거한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aPublicFlag     - [IN] 제거할 데이터베이스 링크의 user mode
 *  aDblinkName     - [IN] Database link name
 *
 ************************************************************************/
IDE_RC dkmExecuteDropDatabaseLink( void     *aQcStatement,
                                   idBool    /*aPublicFlag*/,
                                   SChar    *aDblinkName )
{
    dkoLink     sLinkObj;
    idvSQL     *sStatistics = NULL;

    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    /* 1. Get link object */
    IDE_TEST( dkoLinkObjMgr::findLinkObject( sStatistics,
                                             aDblinkName,
                                             &sLinkObj )
              != IDE_SUCCESS );

    /* 2. Delete dblink from QP meta table */
    IDE_TEST( qciMisc::deleteDatabaseLinkItem( aQcStatement,
                                               sLinkObj.mLinkOID )
              != IDE_SUCCESS );

    /* 3. result set meta 정리 */
    IDE_TEST( dkdResultSetMetaCacheDelete( aDblinkName ) != IDE_SUCCESS );

    /* 4. Destroy dblink object */
    IDE_TEST( dkoLinkObjMgr::destroyLinkObject( aQcStatement, &sLinkObj )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 해당 사용자가 생성한 모든 데이터베이스 링크 객체를
 *               제거한다.
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aUserId         - [IN] User id
 *
 ************************************************************************/
IDE_RC dkmDropDatabaseLinkByUserId( idvSQL *aStatistics, void  *aQcStatement, UInt  aUserId )
{
    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        IDE_TEST( dkoLinkObjMgr::destroyLinkObjectByUserId( aStatistics, aQcStatement, aUserId ) != IDE_SUCCESS );

        IDE_TEST( qciMisc::deleteDatabaseLinkItemByUserId( aQcStatement, aUserId ) != IDE_SUCCESS );
    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Result set meta cache 로부터 column 정보를 얻어온다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aQcStatement        - [IN] Link object validation 에 필요한 정보
 *  aSession            - [IN] Linker data session
 *  aDblinkName         - [IN] Database link name
 *  aRemoteQuery        - [IN] Remote query 구문의 문자열
 *  aRemoteTableHandle  - [OUT] Remote table handle
 *  aColCnt             - [OUT] 얻어올 column 정보의 갯수
 *  aColInfoArr         - [OUT] 얻어올 column 정보들의 배열
 *
 ************************************************************************/
IDE_RC dkmGetColumnInfoFromCache( void              *aQcStatement,
                                  dkmSession        *aSession,
                                  SChar             *aDblinkName,
                                  SChar             *aRemoteQuery,
                                  void             **aRemoteTableHandle,
                                  SInt              *aColCnt,
                                  dkmColumnInfo    **aColInfoArr )
{
    UInt                 i;
    UInt                 sStage;
    UInt                 sColCnt        = 0;
    dkoLink              sLinkObj;
    dkpColumn           *sDkpColArr     = NULL;
    dksDataSession      *sSession       = NULL;
    mtcColumn           *sColMetaArr    = NULL;
    dkmColumnInfo       *sColInfoArr    = NULL;
    void                *sLinkObjHandle = NULL;
    idvSQL              *sStatistics    = NULL;

    sStage = 0;

    IDE_ASSERT( aSession != NULL );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    sSession = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    IDE_TEST( dkdResultSetMetaCacheGetAndHold( aDblinkName,
                                               aRemoteQuery,
                                               &sColCnt,
                                               &sDkpColArr,
                                               &sColMetaArr )
              != IDE_SUCCESS );

    sStage = 1;

    if ( sColCnt != 0 )
    {
        IDU_FIT_POINT_RAISE( "dkmGetColumnInfoFromCache::malloc::ColInfoArr",
                             ERR_ALLOC_COLUMN_INFO_ARR );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( dkmColumnInfo ) * sColCnt,
                                           (void **)&sColInfoArr,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS,  ERR_ALLOC_COLUMN_INFO_ARR );

        sStage = 2;

        for ( i = 0; i < sColCnt; i++ )
        {
            idlOS::memcpy( &(sColInfoArr[i].column),
                           &(sColMetaArr[i]),
                           ID_SIZEOF( mtcColumn ) );

            idlOS::strncpy( sColInfoArr[i].columnName,
                            sDkpColArr[i].mName,
                            ID_SIZEOF( sColInfoArr[i].columnName ) );
        }

        IDE_TEST( dkmGetLinkObjectHandle( sStatistics,
                                          aDblinkName,
                                          &sLinkObjHandle )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    sStage = 0;
    (void)dkdResultSetMetaCacheRelese();

    *aRemoteTableHandle = sLinkObjHandle;
    *aColCnt            = sColCnt;
    *aColInfoArr        = sColInfoArr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_ALLOC_COLUMN_INFO_ARR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)iduMemMgr::free( sColInfoArr );
            sColInfoArr = NULL;
        case 1:
            (void)dkdResultSetMetaCacheRelese();
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 aColumnInfoArray 에 할당된 메모리를 반납한다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aColumnInfoArray - [IN] Column information array
 *
 ************************************************************************/
IDE_RC dkmFreeColumnInfo( dkmColumnInfo * aColumnInfoArray )
{
    (void)iduMemMgr::free( aColumnInfoArray );

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Remote query 구문에 대해 원격서버에 prepare 및 execute
 *               를 수행하고 원격서버로부터 record 를 fetch 해온다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aRemoteQuery    - [IN] Remote query 구문의 문자열
 *  aStatementId    - [OUT] Remote statement id
 *
 ************************************************************************/
IDE_RC dkmAllocAndExecuteQueryStatement( void           *aQcStatement,
                                         dkmSession     *aSession,
                                         idBool          aIsStore,
                                         SChar          *aDblinkName,
                                         SChar          *aRemoteQuery,
                                         SLong          *aStatementId )
{
    idBool                   sIsLinkObjUsing        = ID_FALSE;
    idBool                   sIsRemoteStmtCreated   = ID_FALSE;
    idBool                   sIsGTxCreated          = ID_FALSE;
    idBool                   sIsRTxCreated          = ID_FALSE;
    SInt                     sResult;
    UInt                     sColCnt;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    UInt                     sLocalTxId                 = 0;
    UShort                   sRemoteNodeSessionId;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    dkdTypeConverter        *sTypeConverter         = NULL;
    dkpColumn               *sDkpColArr             = NULL;
    mtcColumn               *sColMetaArr            = NULL;

    dkdRemoteTableMgr       *sRemoteTableMgr        = NULL;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession  = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession ) != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Create remote statement object :
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                              &sRemoteTx )
                  != IDE_SUCCESS );

        if ( sRemoteTx != NULL )
        {
            /* remote transaction already exists */
        }
        else
        {
            /* make a new remote transaction */
            IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                          sSession,
                                                          &sLinkObj,
                                                          &sRemoteTx )
                      != IDE_SUCCESS );

            sIsRTxCreated = ID_TRUE;
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
        }
    }
    else
    {
        IDE_TEST( dkmGetLocalTransactionId( aQcStatement, &sLocalTxId )
                  != IDE_SUCCESS );

        IDE_TEST( dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( sSession,
                                                                            sLocalTxId,
                                                                            &sGlobalCoordinator )
                  != IDE_SUCCESS );
        sIsGTxCreated = ID_TRUE;

        IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                      sSession,
                                                      &sLinkObj,
                                                      &sRemoteTx )
                  != IDE_SUCCESS );

        sIsRTxCreated = ID_TRUE;
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
    }

    if ( aIsStore != ID_TRUE )
    {
        sStmtType = DKT_STMT_TYPE_REMOTE_TABLE;
    }
    else
    {
        sStmtType = DKT_STMT_TYPE_REMOTE_TABLE_STORE;
    }

    IDE_TEST( sRemoteTx->createRemoteStmt( sStmtType,
                                           aRemoteQuery,
                                           &sRemoteStmt )
              != IDE_SUCCESS );

    /* BUG-37630 */
    sIsRemoteStmtCreated = ID_TRUE;
    sRemoteTx->setStatus( DKT_RTX_STATUS_BEGIN );

    /*
       3. Prepare protocol operation
     */
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsing = ID_TRUE;

    IDE_TEST( sRemoteStmt->prepare( &sSession->mSession,
                                    sSession->mId,
                                    sIsRTxCreated,
                                    &(sRemoteTx->mXID),
                                    &sLinkObj,
                                    &sRemoteNodeSessionId )
              != IDE_SUCCESS );

    sRemoteTx->setRemoteNodeSessionId( sRemoteNodeSessionId );

    /*
       4. Execute protocol operation
     */
    IDE_TEST( sRemoteStmt->executeProtocol( &sSession->mSession,
                                            sSession->mId,
                                            ID_FALSE, /* 위 prepareProtocol() 호출에서 TX_BEGIN을 
                                                         세팅했으므로 FALSE를 세팅한다. */
                                            &(sRemoteTx->mXID),
                                            &sLinkObj,
                                            &sResult )
              != IDE_SUCCESS );

    /*
       5. Request result set meta
     */
    IDE_TEST( sRemoteStmt->requestResultSetMetaProtocol( &sSession->mSession,
                                                         sSession->mId )
              != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsing = ID_FALSE;

    /*
       6. Caching result set meta
     */
    IDE_TEST( sRemoteStmt->getTypeConverter( &sTypeConverter ) != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetColumnCount( sTypeConverter, &sColCnt )
              != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetOriginalMeta( sTypeConverter, &sDkpColArr )
              != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetConvertedMeta( sTypeConverter, &sColMetaArr )
              != IDE_SUCCESS );

    IDE_TEST( dkdResultSetMetaCacheInsert( aDblinkName,
                                           aRemoteQuery,
                                           sColCnt,
                                           sDkpColArr,
                                           sColMetaArr )
              != IDE_SUCCESS );

    /*
       7. Activate remote data manager
     */
    IDE_TEST( sRemoteStmt->activateRemoteDataManager( aQcStatement )
              != IDE_SUCCESS );

    /*
       (8). Fetch remote record
    */
    if ( sRemoteStmt->getStmtType() == DKT_STMT_TYPE_REMOTE_TABLE_STORE )
    {
        sIsLinkObjUsing = ID_TRUE;

#ifdef ALTIBASE_PRODUCT_XDB
        /* nothing to do */
#else
        IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
                  != IDE_SUCCESS );
#endif

        IDE_TEST( sRemoteStmt->fetchProtocol( &sSession->mSession,
                                              sSession->mId,
                                              aQcStatement,
                                              ID_TRUE )
                  != IDE_SUCCESS );

#ifdef ALTIBASE_PRODUCT_XDB
        /* nothing to do */
#else
        IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
                  != IDE_SUCCESS );
#endif

        sIsLinkObjUsing = ID_FALSE;
    }
    else
    {
        sRemoteTableMgr = sRemoteStmt->getRemoteTableMgr();
        IDE_TEST( sRemoteTableMgr->restart() != IDE_SUCCESS );

        IDE_TEST( sRemoteStmt->executeRemoteFetch( &aSession->mSession,
                                                   aSession->mId )
                  != IDE_SUCCESS );
    }

    *aStatementId = sRemoteStmt->getStmtId();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsRemoteStmtCreated == ID_TRUE )
    {
        (void)sRemoteStmt->abortProtocol( &sSession->mSession,
                                          sSession->mId );

        sRemoteTx->destroyRemoteStmt( sRemoteStmt );
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsing == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus );
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    if ( sIsRTxCreated == ID_TRUE )
    {
        if ( sGlobalCoordinator->checkAndCloseRemoteTx( sRemoteTx ) == IDE_SUCCESS )
        {
            sGlobalCoordinator->destroyRemoteTx( sRemoteTx );
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
        /* do nothing */
    }

    if ( ( sIsGTxCreated == ID_TRUE ) && ( sGlobalCoordinator->getRemoteTxCount() == 0 ) )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }
    else
    {
        /* done */
    }

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote query 구문에 대해 원격서버에 prepare 및 execute
 *               를 수행한다. 여기서는 실제로 record 를 가져오진 않고
 *               result set meta 를 얻어오기만 한다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *
 *  aQcStatement    - [IN] Link object validation 에 필요한 정보
 *  aSession        - [IN] Linker data session
 *  aDblinkName     - [IN] Database link name
 *  aRemoteQuery    - [IN] Remote query 구문의 문자열
 *  aStatementId    - [OUT] Remote statement id
 *
 ************************************************************************/
IDE_RC dkmAllocAndExecuteQueryStatementWithoutFetch( void           *aQcStatement,
                                                     dkmSession     *aSession,
                                                     idBool          aIsStore,
                                                     SChar          *aDblinkName,
                                                     SChar          *aRemoteQuery,
                                                     SLong          *aStatementId )
{
    idBool                   sIsLinkObjUsed         = ID_FALSE;
    idBool                   sIsRemoteStmtCreated   = ID_FALSE;
    idBool                   sIsGTxCreated          = ID_FALSE;
    idBool                   sIsRTxCreated          = ID_FALSE;
    SInt                     sResult;
    UInt                     sColCnt;
    UInt                     sStmtType              = DKT_STMT_TYPE_NONE;
    UInt                     sLocalTxId                 = 0;
    UShort                   sRemoteNodeSessionId;
    dktRemoteStmt           *sRemoteStmt            = NULL;
    dktRemoteTx             *sRemoteTx              = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator     = NULL;
    dksDataSession          *sSession               = NULL;
    dkoLink                  sLinkObj;
    idBool                   sLinkObjFound          = ID_FALSE;
    dkdTypeConverter        *sTypeConverter         = NULL;
    dkpColumn               *sDkpColArr             = NULL;
    mtcColumn               *sColMetaArr            = NULL;
    idvSQL                  *sStatistics            = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession  = (dksDataSession *)aSession;
    sStatistics = qciMisc::getStatisticsFromQcStatement( aQcStatement );

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession ) != IDE_SUCCESS );

    /*
       1. Validate DB-Link object
     */
    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( sStatistics,
                                                   aDblinkName,
                                                   &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );
    sLinkObjFound = ID_TRUE;

    IDE_TEST_RAISE( dkoLinkObjMgr::validateLinkObject( aQcStatement,
                                                       sSession,
                                                       &sLinkObj )
                    != IDE_SUCCESS, ERR_INVALID_DBLINK );

    /*
       2. Create remote statement object :
     */
    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        IDE_TEST( sGlobalCoordinator->findRemoteTxWithTarget( sLinkObj.mTargetName,
                                                              &sRemoteTx )
                  != IDE_SUCCESS );

        if ( sRemoteTx != NULL )
        {
            /* remote transaction already exists */
        }
        else
        {
            /* make a new remote transaction */
            IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                          sSession,
                                                          &sLinkObj,
                                                          &sRemoteTx )
                      != IDE_SUCCESS );

            sIsRTxCreated = ID_TRUE;
            sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
        }
    }
    else
    {
        IDE_TEST( dkmGetLocalTransactionId( aQcStatement, &sLocalTxId )
                  != IDE_SUCCESS );

        IDE_TEST( dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( sSession,
                                                                            sLocalTxId,
                                                                            &sGlobalCoordinator )
                  != IDE_SUCCESS );
        sIsGTxCreated = ID_TRUE;

        IDE_TEST( sGlobalCoordinator->createRemoteTx( sStatistics,
                                                      sSession,
                                                      &sLinkObj,
                                                      &sRemoteTx )
                  != IDE_SUCCESS );

        sIsRTxCreated = ID_TRUE;
        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
    }

    if ( aIsStore != ID_TRUE )
    {
        sStmtType = DKT_STMT_TYPE_REMOTE_TABLE;
    }
    else
    {
        sStmtType = DKT_STMT_TYPE_REMOTE_TABLE_STORE;
    }

    IDE_TEST( sRemoteTx->createRemoteStmt( sStmtType,
                                           aRemoteQuery,
                                           &sRemoteStmt )
              != IDE_SUCCESS );

    /* BUG-37630 */
    sIsRemoteStmtCreated = ID_TRUE;
    sRemoteTx->setStatus( DKT_RTX_STATUS_BEGIN );

    /*
       3. Prepare protocol operation
     */
#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoIncresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    sIsLinkObjUsed = ID_TRUE;

    IDE_TEST( sRemoteStmt->prepare( &sSession->mSession,
                                    sSession->mId,
                                    sIsRTxCreated,
                                    &(sRemoteTx->mXID),
                                    &sLinkObj,
                                    &sRemoteNodeSessionId )
              != IDE_SUCCESS );

    sRemoteTx->setRemoteNodeSessionId( sRemoteNodeSessionId );

    /*
      4. Request result set meta
     */
    if ( sRemoteStmt->requestResultSetMetaProtocol( &sSession->mSession, sSession->mId ) != IDE_SUCCESS )
    {
        /*
          5. Execute protocol operation
         */
        IDE_TEST( sRemoteStmt->executeProtocol( &sSession->mSession,
                                                sSession->mId,
                                                ID_FALSE, /* 위 prepareProtocol() 호출에서 TX_BEGIN을 세팅했으므로 FALSE를 세팅한다. */
                                                &(sRemoteTx->mXID),
                                                &sLinkObj,
                                                &sResult )
                != IDE_SUCCESS );

        IDE_TEST( sRemoteStmt->requestResultSetMetaProtocol( &sSession->mSession, sSession->mId ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    sIsLinkObjUsed = ID_FALSE;

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    IDE_TEST( dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId )
              != IDE_SUCCESS );
#endif

    /*
       6. Caching result set meta
     */
    IDE_TEST( sRemoteStmt->getTypeConverter( &sTypeConverter ) != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetColumnCount( sTypeConverter, &sColCnt )
              != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetOriginalMeta( sTypeConverter, &sDkpColArr )
              != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetConvertedMeta( sTypeConverter, &sColMetaArr )
              != IDE_SUCCESS );

    IDE_TEST( dkdResultSetMetaCacheInsert( aDblinkName,
                                           aRemoteQuery,
                                           sColCnt,
                                           sDkpColArr,
                                           sColMetaArr )
              != IDE_SUCCESS );

    *aStatementId = sRemoteStmt->getStmtId();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DBLINK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsRemoteStmtCreated == ID_TRUE )
    {
        (void)sRemoteStmt->abortProtocol( &sSession->mSession, sSession->mId );
        sRemoteTx->destroyRemoteStmt( sRemoteStmt );
    }

#ifdef ALTIBASE_PRODUCT_XDB
    /* nothing to do */
#else
    if ( sIsLinkObjUsed == ID_TRUE )
    {
        (void)dkoLinkInfoDecresaseReferenceCount( sLinkObj.mId );
    }
    else
    {
        /* do nothing */
    }

    if ( sLinkObjFound == ID_TRUE )
    {
        dkoLinkInfoStatus sStatus = DKO_LINK_OBJ_NON;

        (void)dkoLinkInfoGetStatus( sLinkObj.mId, &sStatus);
        if ( sStatus == DKO_LINK_OBJ_TOUCH )
        {
            (void)dkoLinkInfoSetStatus( sLinkObj.mId, DKO_LINK_OBJ_READY );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif
    if ( sIsRTxCreated == ID_TRUE )
    {
        if ( sGlobalCoordinator->checkAndCloseRemoteTx( sRemoteTx ) == IDE_SUCCESS )
        {
            sGlobalCoordinator->destroyRemoteTx( sRemoteTx );
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
        /* do nothing */
    }

    if ( ( sIsGTxCreated == ID_TRUE ) && ( sGlobalCoordinator->getRemoteTxCount() == 0 ) )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }
    else
    {
        /* done */
    }

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}


/************************************************************************
 * Description : Remote query 구문을 free 시켜준다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aSession        - [IN] Linker data session
 *  aStatementId    - [IN] Remote statement id
 *
 ************************************************************************/
IDE_RC dkmFreeQueryStatement( dkmSession    *aSession,
                              SLong          aStatementId )
{
    idBool                   sIsRemoteStmtCleaned = ID_FALSE;
    dktRemoteStmt           *sRemoteStmt          = NULL;
    dktRemoteTx             *sRemoteTx            = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator   = NULL;
    dksDataSession          *sSession             = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() == DKA_LINKER_STATUS_NON,
                    ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
                    != ID_TRUE, ERR_LINKER_PROCESS_NOT_RUNNING );

    IDE_TEST( dksSessionMgr::connectDataSession( sSession )
              != IDE_SUCCESS );

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteStmt( aStatementId,
                                                  &sRemoteStmt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    IDE_TEST( sRemoteStmt->free( &sSession->mSession,
                                 sSession->mId )
              != IDE_SUCCESS );

    IDE_TEST( sGlobalCoordinator->findRemoteTx( sRemoteStmt->mRTxId,
                                                &sRemoteTx )
              != IDE_SUCCESS );

    /* BUG-37630 */
    sRemoteTx->destroyRemoteStmt( sRemoteStmt );
    sIsRemoteStmtCleaned = ID_TRUE;

    if ( dksSessionMgr::getDataSessionAtomicTxLevel( sSession )
         == DKT_ADLP_REMOTE_STMT_EXECUTION )
    {
        if ( sGlobalCoordinator->getAllRemoteStmtCount() > 0 )
        {
            /* there exists any remained remote statement */
        }
        else
        {
            dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
        }
    }
    else
    {
        if ( sRemoteTx->isEmptyRemoteTx() == ID_TRUE )
        {
            sRemoteTx->setStatus( DKT_RTX_STATUS_PREPARE_READY );

            if ( sGlobalCoordinator->isAllRemoteTxPrepareReady() == ID_TRUE )
            {
                sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_PREPARE_READY );
            }
            else
            {
                /* global transaction is on going */
            }
        }
        else
        {
            /* remote transaction is on going */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    /* >> BUG-37630 */
    if ( sIsRemoteStmtCleaned != ID_TRUE )
    {
        if ( sRemoteTx != NULL )
        {
            sRemoteTx->destroyRemoteStmt( sRemoteStmt );
        }
        else
        {
            /* no remote transaction */
        }
    }
    else
    {
        /* this remote statement cleaned  */
    }
    /* << BUG-37630 */

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Database link object handle 을 얻어온다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aDblinkName     - [IN] Database link name
 *  aLinkObjHandle  - [OUT] Link object handle
 *
 ************************************************************************/
IDE_RC dkmGetLinkObjectHandle( idvSQL   *aStatistics,
                               SChar    *aDblinkName,
                               void    **aLinkObjHandle )
{
    dkoLink          sLinkObj;
    const void      *sTableHandle = NULL;

    IDE_TEST_RAISE( dkoLinkObjMgr::findLinkObject( aStatistics, aDblinkName, &sLinkObj )
                    != IDE_SUCCESS, ERR_DBLINK_NOT_EXIST );

    sTableHandle = smiGetTable( sLinkObj.mLinkOID );

    *aLinkObjHandle = (void *)sTableHandle;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_DBLINK ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Column 정보를 얻어온다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aSession         - [IN] Linker data session
 *  aStatementId     - [IN] Remote statement id
 *  aColumnCount     - [OUT] Column 의 갯수
 *  aColumnInfoArray - [OUT] 얻어온 column 정보들의 배열
 *
 ************************************************************************/
IDE_RC dkmGetColumnInfo( dkmSession         *aSession,
                         SLong               aStatementId,
                         SInt               *aColumnCount,
                         dkmColumnInfo     **aColumnInfoArray )
{
    UInt                     i;
    UInt                     sColCnt            = 0;
    dktRemoteStmt           *sRemoteStmt        = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dkdTypeConverter        *sTypeConverter     = NULL;
    dksDataSession          *sSession           = NULL;
    dkmColumnInfo           *sColInfoArr        = NULL;
    mtcColumn               *sColMetaArr        = NULL;
    dkpColumn               *sDkpColArr         = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteStmt( aStatementId,
                                                  &sRemoteStmt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    IDE_TEST( sRemoteStmt->getTypeConverter( &sTypeConverter ) != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetColumnCount( sTypeConverter,
                                              &sColCnt )
              != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetConvertedMeta( sTypeConverter,
                                                &sColMetaArr )
              != IDE_SUCCESS );

    IDE_TEST( dkdTypeConverterGetOriginalMeta( sTypeConverter,
                                               &sDkpColArr )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "dkmGetColumnInfo::malloc::ColInfoArr",
                          ERR_ALLOC_COLUMN_INFO_ARR );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dkmColumnInfo ) * sColCnt,
                                       (void **)&sColInfoArr,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_ALLOC_COLUMN_INFO_ARR );

    for ( i = 0; i < sColCnt; i++ )
    {
        idlOS::memcpy( &(sColInfoArr[i].column),
                       &(sColMetaArr[i]),
                       ID_SIZEOF( mtcColumn ) );

        idlOS::strncpy( sColInfoArr[i].columnName,
                        sDkpColArr[i].mName,
                        ID_SIZEOF( sColInfoArr[i].columnName ) );
    }

    *aColumnCount     = sColCnt;
    *aColumnInfoArray = sColInfoArr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALLOC_COLUMN_INFO_ARR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    if ( sColInfoArr != NULL )
    {
        (void)iduMemMgr::free(sColInfoArr);
        sColInfoArr = NULL;
    }
    else
    {
        /*do nothing*/
    }
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkmFetchNextRowInternal( dksDataSession          *aSession,
                                       dktRemoteStmt           *aRemoteStmt,
                                       void                   **aRow )
{
#ifdef ALTIBASE_PRODUCT_XDB
    dkdDataMgr *sDataMgr = NULL;

    sDataMgr = aRemoteStmt->getDataMgr();
    if ( sDataMgr->isNeedFetch() == ID_TRUE )
    {
        if ( dkmDblinkStarted() == ID_TRUE )
        {
            IDE_TEST( dksSessionMgr::connectDataSession( aSession ) != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aRemoteStmt->fetchProtocol( &aSession->mSession,
                                              aSession->mId,
                                              NULL,
                                              ID_FALSE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( sDataMgr->fetchRow( aRow ) != IDE_SUCCESS );

#else
    dkdRemoteTableMgr       *sRemoteTableMgr    = NULL;

    sRemoteTableMgr = aRemoteStmt->getRemoteTableMgr();

    if ( sRemoteTableMgr->getInsertRowCnt() == 0 )
    {

        if ( dkmDblinkStarted() == ID_TRUE )
        {
            IDE_TEST( dksSessionMgr::connectDataSession( aSession )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aRemoteStmt->executeRemoteFetch( &aSession->mSession,
                                                   aSession->mId )
                  != IDE_SUCCESS );
    }
    else
    {
        /* need not remote fetch */
    }

    sRemoteTableMgr->fetchRow( aRow );

#endif /* ALTIBASE_PRODUCT_XDB */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Cursor 가 현재 가리키는 row 를 반환한 뒤 cursor 가 다음
 *               row 를 가리키도록 이동시킨다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aSession        - [IN] Linker data session
 *  aStatementId    - [IN] Remote statement id
 *  aRow            - [OUT] Fetch 할 row
 *
 ************************************************************************/
IDE_RC dkmFetchNextRow( dkmSession      *aSession,
                        SLong            aStatementId,
                        void           **aRow )
{
    dktRemoteStmt           *sRemoteStmt        = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dkdDataMgr              *sDataMgr           = NULL;
    dksDataSession          *sSession           = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteStmt( aStatementId,
                                                  &sRemoteStmt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    if ( sRemoteStmt->getStmtType() == DKT_STMT_TYPE_REMOTE_TABLE )
    {
        IDE_TEST( dkmFetchNextRowInternal( sSession, sRemoteStmt, aRow ) != IDE_SUCCESS );
    }
    else
    {
        sDataMgr = sRemoteStmt->getDataMgr();

        IDE_TEST( sDataMgr->fetchRow( aRow ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkmRestartRowInternal( dksDataSession * aSession,
                                     dktRemoteStmt  * aRemoteStmt,
                                     ID_XID         * aXID )
{
    SInt                     sResult            = 0;
    dkoLink                 *sLinkObj           = NULL;
    dkdRemoteTableMgr       *sRemoteTableMgr    = NULL;

    sLinkObj = aRemoteStmt->getExecutedLinkObj();

    sRemoteTableMgr = aRemoteStmt->getRemoteTableMgr();
    if ( sRemoteTableMgr->isFirst() != ID_TRUE )
    {
        IDE_TEST( aRemoteStmt->executeProtocol( &aSession->mSession,
                                                aSession->mId,
                                                ID_FALSE, /* remoteTx가 Begin된 상태이므로 */
                                                aXID,
                                                sLinkObj,
                                                &sResult )
                  != IDE_SUCCESS );

        IDE_TEST( sRemoteTableMgr->restart() != IDE_SUCCESS );

        IDE_TEST( aRemoteStmt->executeRemoteFetch( &aSession->mSession,
                                                   aSession->mId )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Cursor restart 를 수행한다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 *  aSession        - [IN] Linker data session
 *  aStatementId    - [IN] Remote statement id
 *
 ************************************************************************/
IDE_RC dkmRestartRow( dkmSession    *aSession,
                      SLong          aStatementId )
{
    dktRemoteTx             *sRemoteTx          = NULL;
    dktRemoteStmt           *sRemoteStmt        = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dkdDataMgr              *sDataMgr           = NULL;
    dksDataSession          *sSession           = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    if ( dkmDblinkStarted() == ID_TRUE )
    {
        IDE_TEST( dksSessionMgr::connectDataSession( sSession )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxNStmt( aStatementId,
                                                     &sRemoteTx,
                                                     &sRemoteStmt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    if ( sRemoteStmt->getStmtType() == DKT_STMT_TYPE_REMOTE_TABLE )
    {
        IDE_TEST( dkmRestartRowInternal( sSession,
                                         sRemoteStmt,
                                         &(sRemoteTx->mXID) )
                  != IDE_SUCCESS );
    }
    else
    {
        sDataMgr = sRemoteStmt->getDataMgr();

        IDE_TEST( sDataMgr->restart() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSession != NULL )
    {
        (void)dksSessionMgr::checkAndDisconnectDataSession( sSession );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 메타에 저장되어 있는 정보를 통해 DK link object 를
 *               생성한다.
 *
 *      @ Related Keyword : REMOTE_TABLE
 *                          REMOTE_TABLE_STORE
 *
 ************************************************************************/
IDE_RC  dkmLoadDblinkFromMeta( idvSQL *aStatistics )
{
    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        IDE_TEST( dkoLinkObjMgr::createLinkObjectsFromMeta( aStatistics ) != IDE_SUCCESS );
    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC dkmCheckSessionAndStatus( dkmSession * aSession )
{
    if ( aSession == NULL )
    {
        IDE_TEST( dkmCheckDblinkEnabled() != IDE_SUCCESS );

        IDE_TEST_RAISE( dkaLinkerProcessMgr::getLinkerStatus() ==
                        DKA_LINKER_STATUS_NON,
                        ERR_LINKER_PROCESS_NOT_RUNNING );

        IDE_TEST_RAISE( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() ==
                        ID_FALSE,
                        ERR_LINKER_PROCESS_NOT_RUNNING );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LINKER_PROCESS_NOT_RUNNING );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_ALTILINKER_IS_NOT_RUNNING ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkmInvalidResultSetMetaCache( SChar * aDatabaseLinkName,
                                     SChar * aRemoteQuery )
{
    return dkdResultSetMetaCacheInvalid( aDatabaseLinkName,
                                         aRemoteQuery );
}

/*
 *
 */
IDE_RC dkmGetNullRow( dkmSession * aSession,
                      SLong        aStatementId,
                      void      ** aRow,
                      scGRID     * aRid )
{
    dktRemoteStmt           * sRemoteStmt = NULL;
    dktGlobalCoordinator    * sGlobalCoordinator = NULL;
    dkdDataMgr              * sDataMgr = NULL;
    dksDataSession          * sSession = NULL;

    IDE_ASSERT( aSession != NULL );

    sSession = (dksDataSession *)aSession;

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId, &sGlobalCoordinator ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteStmt( aStatementId, &sRemoteStmt ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    sDataMgr = sRemoteStmt->getDataMgr();
    IDE_TEST( sDataMgr->getNullRow( aRow, aRid ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_TRANSACTION )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static idBool dkmIsFirstInternal( dksDataSession          * /* aSession */,
                                  dktRemoteStmt           *aRemoteStmt )
{
    dkdRemoteTableMgr       *sRemoteTableMgr    = NULL;

    sRemoteTableMgr = aRemoteStmt->getRemoteTableMgr();

    return sRemoteTableMgr->isFirst();
}

/*
 *
 */
IDE_RC dkmIsFirst( dkmSession    *aSession,
                   SLong          aStatementId )
{
    dktRemoteTx             *sRemoteTx          = NULL;
    dktRemoteStmt           *sRemoteStmt        = NULL;
    dktGlobalCoordinator    *sGlobalCoordinator = NULL;
    dkdDataMgr              *sDataMgr           = NULL;
    dksDataSession          *sSession           = NULL;

    IDE_TEST_RAISE( aSession == NULL, ERR_INTERNAL_ERROR );

    sSession = (dksDataSession *)aSession;

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sGlobalCoordinator == NULL, ERR_INVALID_TRANSACTION );

    
    IDE_TEST( sGlobalCoordinator->findRemoteTxNStmt( aStatementId,
                                                     &sRemoteTx,
                                                     &sRemoteStmt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRemoteStmt == NULL, ERR_INVALID_REMOTE_STMT );

    if ( sRemoteStmt->getStmtType() == DKT_STMT_TYPE_REMOTE_TABLE )
    {
        IDE_TEST_RAISE( dkmIsFirstInternal( sSession, sRemoteStmt ) != ID_TRUE,
                        ERR_IS_NOT_FIRST );
    }
    else
    {
        sDataMgr = sRemoteStmt->getDataMgr();

        IDE_TEST_RAISE( sDataMgr->isFirst() != ID_TRUE,
                        ERR_IS_NOT_FIRST );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL_ERROR )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkmIsFirst] aSession is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_TRANSACTION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_IS_NOT_FIRST );
    {
        ideLog::log( DK_TRC_LOG_FORCE, "It can not move cusor to first data." );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dkmIsFirst] It can not move cusor to first data." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool dkmIs2PCSession( dkmSession *aSession )
{
    if ( dkiIsGTx( aSession->mAtomicTxLevel ) == ID_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

IDE_RC dkmOpenShardConnection( dkmSession     * aSession,
                               sdiConnectInfo * aConnectInfo )
{
    dksDataSession    * sSession = NULL;
    dktAtomicTxLevel    sTxLevel;

    sSession = (dksDataSession *)aSession;

    IDE_TEST_RAISE( sSession == NULL, ERR_SESSION_NOT_EXIST );

    sTxLevel = dksSessionMgr::getDataSessionAtomicTxLevel( sSession );

    switch ( sTxLevel )
    {
        case DKT_ADLP_SIMPLE_TRANSACTION_COMMIT:
        case DKT_ADLP_TWO_PHASE_COMMIT:
        case DKT_ADLP_GCTX:
            IDE_TEST( sdi::allocConnect( aConnectInfo ) != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_ATOMIC_TX_LEVEL );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_SESSION_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_ATOMIC_TX_LEVEL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INVALID_TX_LEVEL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dkmCloseShardConnection( sdiConnectInfo * aConnectInfo )
{
    sdi::freeConnectImmediately( aConnectInfo );
}

IDE_RC dkmAddShardTransaction( idvSQL         * aStatistics,
                               dkmSession     * aSession,
                               smTID            aTransID,
                               sdiClientInfo  * aClientInfo,
                               sdiConnectInfo * aConnectInfo )
{
    idBool                   sIsGTxCreated      = ID_FALSE;
    idBool                   sIsRTxCreated      = ID_FALSE;
    dktRemoteTx            * sRemoteTx          = NULL;
    dktGlobalCoordinator   * sGlobalCoordinator = NULL;
    dksDataSession         * sSession           = NULL;

    sSession = (dksDataSession *)aSession;

    IDE_TEST_RAISE( sSession == NULL, ERR_SESSION_NOT_EXIST );

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        IDE_TEST_RAISE( ( sGlobalCoordinator->isGTx() == ID_TRUE ) &&
                        ( sGlobalCoordinator->getGTxStatus() >= DKT_GTX_STATUS_PREPARE_REQUEST ),
                        ERR_GTX_SHARD_NEED_ROLLBACK );

        IDE_TEST( sGlobalCoordinator->findRemoteTxWithShardNode( aConnectInfo->mNodeId,
                                                                 &sRemoteTx )
                  != IDE_SUCCESS );

        if ( sRemoteTx != NULL )
        {
            /* remote transaction already exists */
        }
        else
        {
            /* make a new remote transaction */
            IDE_TEST( sGlobalCoordinator->createRemoteTxForShard( aStatistics,
                                                                  sSession,
                                                                  aConnectInfo,
                                                                  &sRemoteTx )
                      != IDE_SUCCESS );

            sIsRTxCreated = ID_TRUE;

            aConnectInfo->mFlag &= ~SDI_CONNECT_REMOTE_TX_CREATE_MASK;
            aConnectInfo->mFlag |= SDI_CONNECT_REMOTE_TX_CREATE_TRUE;
        }
    }
    else
    {
        IDE_TEST( dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( sSession,
                                                                            aTransID,
                                                                            &sGlobalCoordinator )
                  != IDE_SUCCESS );
        sIsGTxCreated = ID_TRUE;

        sGlobalCoordinator->setShardClientInfo( aClientInfo );

        IDE_TEST( sGlobalCoordinator->createRemoteTxForShard( aStatistics,
                                                              sSession,
                                                              aConnectInfo,
                                                              &sRemoteTx )
                  != IDE_SUCCESS );

        sIsRTxCreated = ID_TRUE;

        aConnectInfo->mFlag &= ~SDI_CONNECT_REMOTE_TX_CREATE_MASK;
        aConnectInfo->mFlag |= SDI_CONNECT_REMOTE_TX_CREATE_TRUE;
    }

    /* sharding에서는 remoteStmt 없이 remoteTx만 생성된다. */
    if ( ( sIsRTxCreated == ID_TRUE ) &&
         ( dkiIsGTx( dksSessionMgr::getDataSessionAtomicTxLevel( sSession ) ) == ID_TRUE ) )
    {
        aConnectInfo->mFlag &= ~SDI_CONNECT_COMMIT_PREPARE_MASK;
        aConnectInfo->mFlag |= SDI_CONNECT_COMMIT_PREPARE_FALSE;
    }
    else
    {
        /* Nothing to do. */
    }

    sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );
    sRemoteTx->setStatus( DKT_RTX_STATUS_BEGIN );

    aConnectInfo->mRemoteTx = (void*)sRemoteTx;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_SESSION_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_GTX_SHARD_NEED_ROLLBACK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_NOT_EXECUTE_DML ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsRTxCreated == ID_TRUE )
    {
        sGlobalCoordinator->destroyRemoteTx( sRemoteTx );

        aConnectInfo->mRemoteTx = NULL;
        aConnectInfo->mFlag &= ~SDI_CONNECT_REMOTE_TX_CREATE_MASK;
        aConnectInfo->mFlag |= SDI_CONNECT_REMOTE_TX_CREATE_FALSE;
    }
    else
    {
        /* check global coordinator created */
    }

    if ( ( sIsGTxCreated == ID_TRUE ) && ( sGlobalCoordinator->getRemoteTxCount() == 0 ) )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }
    else
    {
        /* done */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void dkmDelShardTransaction( dkmSession     * aSession,
                             sdiConnectInfo * aConnectInfo )
{
    dktRemoteTx            * sRemoteTx          = NULL;
    dktGlobalCoordinator   * sGlobalCoordinator = NULL;
    dksDataSession         * sSession           = NULL;

    sSession = (dksDataSession *)aSession;

    IDE_DASSERT( sSession != NULL );
    IDE_TEST_RAISE( sSession == NULL, ERR_SESSION_NOT_EXIST );

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        if ( ( aConnectInfo->mFlag & SDI_CONNECT_REMOTE_TX_CREATE_MASK )
               == SDI_CONNECT_REMOTE_TX_CREATE_TRUE )
        {
            sRemoteTx = (dktRemoteTx*)aConnectInfo->mRemoteTx;
            IDE_DASSERT( sRemoteTx != NULL );
            IDE_TEST_RAISE( sRemoteTx == NULL, ERR_REMOTE_TX_NOT_EXIST );

            if ( sGlobalCoordinator->checkAndCloseRemoteTx( sRemoteTx ) == IDE_SUCCESS )
            {
                sGlobalCoordinator->destroyRemoteTx( sRemoteTx );

                aConnectInfo->mRemoteTx = NULL;
                aConnectInfo->mFlag &= ~SDI_CONNECT_REMOTE_TX_CREATE_MASK;
                aConnectInfo->mFlag |= SDI_CONNECT_REMOTE_TX_CREATE_FALSE;
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
    }
    else
    {
        aConnectInfo->mFlag &= ~SDI_CONNECT_REMOTE_TX_CREATE_MASK;
        aConnectInfo->mFlag |= SDI_CONNECT_REMOTE_TX_CREATE_FALSE;

        aConnectInfo->mRemoteTx = NULL;
    }

    return;

    IDE_EXCEPTION( ERR_SESSION_NOT_EXIST );
    {
        ideLog::log(IDE_DK_0, "Session not found." );
    }
    IDE_EXCEPTION( ERR_REMOTE_TX_NOT_EXIST );
    {
        ideLog::log(IDE_DK_0, "Remote transaction is not found." );
    }
    IDE_EXCEPTION_END;

    return;
}

/* BUG-46092 */
IDE_RC dkmSetTransactionBrokenOnGlobalCoordinator( dkmSession * aSession,
                                                   smTID        aTransID )
{
    idBool                   sIsGTxCreated      = ID_FALSE;
    dktGlobalCoordinator   * sGlobalCoordinator = NULL;
    dksDataSession         * sSession           = NULL;

    sSession = (dksDataSession *)aSession;

    IDE_DASSERT( sSession != NULL );
    IDE_TEST_RAISE( sSession == NULL, ERR_SESSION_NOT_EXIST );

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        sGlobalCoordinator->setFlag( DKT_COORD_FLAG_TRANSACTION_BROKEN_MASK,
                                     DKT_COORD_FLAG_TRANSACTION_BROKEN_TRUE );
    }
    else
    {
        IDE_TEST( dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( sSession,
                                                                            aTransID,
                                                                            &sGlobalCoordinator )
                  != IDE_SUCCESS );
        sIsGTxCreated = ID_TRUE;

        sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );

        sGlobalCoordinator->setFlag( DKT_COORD_FLAG_TRANSACTION_BROKEN_MASK,
                                     DKT_COORD_FLAG_TRANSACTION_BROKEN_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_SESSION_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( ( sIsGTxCreated == ID_TRUE ) && ( sSession != NULL ) )
    {
        dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( sGlobalCoordinator, sSession );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC dkmIsExistRemoteTx( dkmSession * aSession,
                           idBool     * aIsExist )
{
    dktGlobalCoordinator   * sGlobalCoordinator = NULL;
    dksDataSession         * sSession           = NULL;
    idBool                   sIsExist           = ID_FALSE;

    sSession = (dksDataSession *)aSession;

    IDE_TEST_RAISE( sSession == NULL, ERR_SESSION_NOT_EXIST );

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        if ( sGlobalCoordinator->getRemoteTxCount() > 0 )
        {
            sIsExist = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aIsExist = sIsExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_SESSION_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkmEndPendingPassiveDtxInfo( ID_XID * aXID, idBool aCommit )
{
    dktNotifier    * sNotifier = NULL;

    sNotifier = dktGlobalTxMgr::getNotifier();
    sNotifier->setResultPassiveDtxInfo( aXID, aCommit );

    return IDE_SUCCESS;
}

IDE_RC dkmEndPendingFailoverDtxInfo( ID_XID * aXID, 
                                     idBool   aCommit,
                                     smSCN  * aGlobalCommitSCN )
{
    dktNotifier    * sNotifier = NULL;

    sNotifier = dktGlobalTxMgr::getNotifier();
    sNotifier->setResultFailoverDtxInfo( aXID, 
                                         aCommit,
                                         aGlobalCommitSCN );

    return IDE_SUCCESS;
}

IDE_RC dkmCheckGlobalTransactionStatus( dkmSession     * aSession )
{
    dktGlobalCoordinator   * sGlobalCoordinator = NULL;
    dksDataSession         * sSession           = NULL;

    sSession = (dksDataSession *)aSession;

    IDE_TEST_RAISE( sSession == NULL, ERR_SESSION_NOT_EXIST );

    IDE_TEST( dktGlobalTxMgr::findGlobalCoordinator( sSession->mGlobalTxId,
                                                     &sGlobalCoordinator )
              != IDE_SUCCESS );

    if ( sGlobalCoordinator != NULL )
    {
        IDE_TEST_RAISE( ( sGlobalCoordinator->isGTx() == ID_TRUE ) &&
                        ( sGlobalCoordinator->getGTxStatus() >= DKT_GTX_STATUS_PREPARE_REQUEST ),
                        ERR_GTX_SHARD_NEED_ROLLBACK );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_SESSION_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_GTX_SHARD_NEED_ROLLBACK );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_NOT_EXECUTE_DML ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkmAddDtxBranchTx( void   * aDtxInfo,
                          UChar    aCoordinatorType,
                          SChar  * aNodeName,
                          SChar  * aUserName,
                          SChar  * aUserPassword,
                          SChar  * aDataServerIP,
                          UShort   aDataPortNo,
                          UShort   aConnectType )
{
    dktDtxInfo * sDtxInfo = (dktDtxInfo*)aDtxInfo;

    IDE_DASSERT( aDtxInfo != NULL );

    IDE_TEST( sDtxInfo->addDtxBranchTx( &(sDtxInfo->mXID),
                                        (sdiCoordinatorType)aCoordinatorType,
                                        aNodeName,
                                        aUserName,
                                        aUserPassword,
                                        aDataServerIP,
                                        aDataPortNo,
                                        aConnectType )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
