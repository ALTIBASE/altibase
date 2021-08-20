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

#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dki.h>
#include <dkiv.h>
#include <dkif.h>
#include <dkiq.h>
#include <dkis.h>
#include <dkm.h>

#include <dkiSession.h>
#include <dkuProperty.h>
#include <dktNotifier.h>
#include <dktGlobalTxMgr.h>

/*
 *
 */ 
IDE_RC dkiDoPreProcessPhase()
{
    ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_I_PRE_PROCESS );
 
    IDE_TEST( dkmInitialize() != IDE_SUCCESS );

    IDE_TEST( dkisRegister2PCCallback() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_FORCE );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkiDoProcessPhase( void )
{
    ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_I_PROCESS );
    
    return IDE_SUCCESS;
}

/*
 *
 */ 
IDE_RC dkiDoControlPhase( void )
{
    ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_I_CONTROL );

    return IDE_SUCCESS;
}

/*
 *
 */ 
IDE_RC dkiDoMetaPhase( void )
{
    ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_I_META );
    
    return IDE_SUCCESS;
}

/*
 *
 */ 
IDE_RC dkiDoServicePhase( idvSQL * aStatistics )
{
    ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_I_SERVICE );
    
    /* for removing previous error messages. */
    IDE_CLEAR();
    
    IDE_TEST( dkiqRegisterQPCallback() != IDE_SUCCESS );

    IDE_TEST( dkisRegister() != IDE_SUCCESS );
    
    IDE_TEST( dkmStartAltilinkerProcess() != IDE_SUCCESS );

    IDE_TEST( dkmLoadDblinkFromMeta( aStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_FORCE );
    
    return IDE_FAILURE;    
}

/*
 *
 */ 
IDE_RC dkiDoShutdownPhase()
{
    ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_I_SHUTDOWN );
    
    IDE_TEST( dkisUnregister2PCCallback() != IDE_SUCCESS );

    IDE_TEST( dkmFinalize() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_FORCE );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkiInitializePerformanceView( void )
{
    IDE_TEST( dkivRegisterPerformaceView() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_FORCE );
    
    return IDE_FAILURE;
}

void dkiSessionInit( dkiSession * aSession )
{
    aSession->mSessionId = 0;
    aSession->mSession = NULL;
}

/*
 * Create DK session and initialize internal data structure.
 */ 
IDE_RC dkiSessionCreate( UInt aSessionID, dkiSession * aSession )
{
    IDE_TEST( dkmSessionInitialize( aSessionID,
                                    &(aSession->mSessionObj),
                                    &(aSession->mSession) )
              != IDE_SUCCESS );

    aSession->mSessionId = aSessionID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );

    return IDE_FAILURE;
}

/*
 * Free DK session and finalize internal data structure.
 */ 
IDE_RC dkiSessionFree( dkiSession * aSession )
{
    if ( aSession->mSession != NULL )
    {
        /* BUG-42100 */
        IDU_FIT_POINT("dki::dkiSessionFree::dkmSessionFinalize::sleep");
        /* BUG-37487 : void */
        (void)dkmSessionFinalize( aSession->mSession );

        aSession->mSession = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/*
 *
 */ 
void dkiSessionSetUserId( dkiSession * aSession, UInt aUserId )
{
    if ( aSession->mSession != NULL )
    {
        dkmSessionSetUserId( aSession->mSession, aUserId );
    }
    else
    {
        /* nothing to do */
    }
}

/*
 *
 */ 
IDE_RC dkiGetGlobalTransactionLevel( UInt * aValue )
{
    IDE_TEST( dkmGetGlobalTransactionLevel( aValue ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

idBool dkiIsGTx( UInt aValue )
{
    idBool sRet = ID_FALSE;

    if ( ( aValue == DKT_ADLP_TWO_PHASE_COMMIT ) ||
         ( aValue == DKT_ADLP_GCTX ) )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

idBool dkiIsGCTx( UInt aValue )
{
    idBool sRet = ID_FALSE;

    if ( aValue == DKT_ADLP_GCTX )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

/*
 *
 */ 
IDE_RC dkiGetRemoteStatementAutoCommit( UInt * aValue )
{
    IDE_TEST( dkmGetRemoteStatementAutoCommit( aValue ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkiSessionSetGlobalTransactionLevel( dkiSession * aSession,
                                            UInt aValue )
{
    IDE_TEST( dkiSessionCheckAndInitialize( aSession ) != IDE_SUCCESS );

    if ( aSession->mSession != NULL )
    {
        IDE_TEST( dkmSessionSetGlobalTransactionLevel( aSession->mSession,
                                                       aValue )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkiSessionSetRemoteStatementAutoCommit( dkiSession * aSession,
                                               UInt aValue )
{
    IDE_TEST( dkiSessionCheckAndInitialize( aSession ) != IDE_SUCCESS );

    if ( aSession->mSession != NULL )
    {
        IDE_TEST( dkmSessionSetRemoteStatementAutoCommit(
                      aSession->mSession,
                      aValue )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkiCommitPrepare( dkiSession * aSession, ID_XID * aSourceXID )
{
    if ( aSession->mSession != NULL )
    {
        IDE_TEST( dkmPrepare ( aSession->mSession, aSourceXID ) != IDE_SUCCESS );

        IDU_FIT_POINT( "dkiCommitPrepare::dkmIs2PCSession" );
        if ( dkmIs2PCSession(aSession->mSession) != ID_TRUE )
        {
            IDE_TEST( dkmCommit( aSession->mSession ) != IDE_SUCCESS );
        }
        else /* 2pc */
        {
            /*do nothing*/
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

IDE_RC dkiCommitPrepareForce( dkiSession * aSession )
{
    if ( aSession->mSession != NULL )
    {
        if ( dkmPrepare( aSession->mSession, NULL ) != IDE_SUCCESS )
        {
            IDE_TEST( dkmIs2PCSession(aSession->mSession) == ID_TRUE );
            /*simple tx or statement execution, ignore prepare result*/
            dkmSetGtxPreparedStatus( aSession->mSession );
        }
        else 
        {
            /*do nothing*/
        }

        if ( dkmIs2PCSession(aSession->mSession) != ID_TRUE )
        {
            (void)dkmCommit( aSession->mSession );
        }
        else
        {
            /*do nothing*/
        }
    }
    else
    {
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

void dkiCommit( dkiSession * aSession )
{
    if ( aSession->mSession != NULL )
    {
        if ( dkmIs2PCSession(aSession->mSession) != ID_TRUE )
        {
            /*do nothing,  the transaction was aleady committed*/
        }
        else /* 2pc */
        {
            (void)dkmCommit( aSession->mSession );
        }
    }
    else
    {
        /* nothing to do */
    }
}

/*
 *
 */
IDE_RC dkiRollback( dkiSession * aSession, const SChar * aSavepoint )
{
    if ( aSession->mSession != NULL )
    {
        if ( dkmIs2PCSession(aSession->mSession) != ID_TRUE )
        {
            if ( aSavepoint != NULL )
            {
                IDE_TEST( dkmRollback( aSession->mSession, (SChar *)aSavepoint ) != IDE_SUCCESS ); 
            }
            else
            {
                /*do nothing,  the transaction was aleady rolled back*/
            }
        }
        else /* 2pc */
        {
            IDE_TEST( dkmRollback( aSession->mSession,  (SChar *)aSavepoint ) != IDE_SUCCESS ); 
        }
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );

    return IDE_FAILURE;
}
/*
 * execute simple,statement rollback force except 2pc
 */ 
void dkiRollbackPrepareForce( dkiSession * aSession )
{
    if ( aSession->mSession != NULL )
    {
        if ( dkmIs2PCSession(aSession->mSession) != ID_TRUE )
        {
            IDE_TEST( dkmRollbackForce( aSession->mSession ) != IDE_SUCCESS );
        }
        else
        {
                /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
    
    return ;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return ;
}
/*
 * execute simple,statement level transaction rollback except 2pc
 */
IDE_RC dkiRollbackPrepare( dkiSession * aSession, const SChar * aSavepoint )
{
    if ( aSession->mSession != NULL )
    {
        if ( dkmIs2PCSession(aSession->mSession) != ID_TRUE )
        {
            if ( aSavepoint == NULL ) /*total rollback*/
            {
                IDE_TEST( dkmRollback( aSession->mSession, NULL ) != IDE_SUCCESS );
            }
            else
            {
                /*do nothing : rollback to savepoint  was executed by dkiRollback*/
            }
        }
        else
        {
            /* do nothing: rollback to savepoint abort was executed by dkiRollback*/
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}
/*
 *
 */ 
IDE_RC dkiSavepoint( dkiSession * aSession, const SChar * aSavepoint )
{
    if ( aSession->mSession != NULL )
    {
        IDE_TEST( dkmSavepoint( aSession->mSession, aSavepoint ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
        
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkiAddDatabaseLinkFunctions( void )
{
    IDE_TEST( dkifRegisterFunction() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}


/*
 * Only DK internal use.
 */
dkmSession * dkiSessionGetDkmSession( dkiSession * aSession )
{
    IDE_PUSH();
    
    (void)dkiSessionCheckAndInitialize( aSession );

    IDE_POP();
    
    return aSession->mSession;
}

/*
 * Only DK internal use.
 */ 
void dkiSessionSetDkmSessionNull( dkiSession * aSession )
{
    aSession->mSession = NULL;
}

/*
 * Only DK internal use.
 */
IDE_RC dkiSessionCheckAndInitialize( dkiSession * aSession )
{
    if ( aSession->mSession == NULL )
    {
        IDE_TEST( dkmSessionInitialize( aSession->mSessionId,
                                        &(aSession->mSessionObj),
                                        &(aSession->mSession) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

idBool dkiIsReadOnly( dkiSession * aSession )
{
    idBool  sIsExist  = ID_FALSE;
    idBool  sReadOnly = ID_FALSE;

    if ( aSession->mSession != NULL )
    {
        if ( dkmIsExistRemoteTx( aSession->mSession,
                                 &sIsExist )
             == IDE_SUCCESS )
        {
            if ( sIsExist == ID_FALSE )
            {
                sReadOnly = ID_TRUE;
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
    else
    {
        sReadOnly = ID_TRUE;
    }

    return sReadOnly;
}

IDE_RC dkiEndPendingPassiveDtxInfo( ID_XID * aXID, idBool aCommit )
{
    IDE_TEST( dkmEndPendingPassiveDtxInfo( aXID, aCommit ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkiEndPendingFailoverDtxInfo( ID_XID * aXID, 
                                     idBool   aCommit,
                                     smSCN  * aGlobalCommitSCN )
{
    IDE_TEST( dkmEndPendingFailoverDtxInfo( aXID, 
                                            aCommit,
                                            aGlobalCommitSCN ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-47863 
   Notifier에 등록된 Global Tx의 수를 출력한다. */
UInt dkiGetDtxInfoCnt()
{
    return (dktGlobalTxMgr::getNotifier())->getDtxInfoCnt();
}

/* BUG-47863
   Notifier에 등록되어있는 Transaction의 정보를 Trace Log에 남긴다. */
IDE_RC dkiPrintNotifierInfo()
{
    dkmNotifierTransactionInfo * sInfo      = NULL;
    UInt                         sInfoCount = 0;
    UInt                         i          = 0;
    UChar sXIDStr[DKT_2PC_XID_STRING_LEN];
    
    IDE_TEST( dkmGetNotifierTransactionInfo( &sInfo, &sInfoCount )
              != IDE_SUCCESS );

    ideLog::log( IDE_SERVER_0,
                 "< Global Transaction Information (cnt:%"ID_UINT32_FMT") >",
                 sInfoCount );

    for ( i = 0; i < sInfoCount; i++ )
    {
        (void)idaXaConvertXIDToString( NULL,
                                       &(sInfo[i].mXID),
                                       sXIDStr,
                                       ID_SIZEOF(sXIDStr) );

        ideLog::log( IDE_SERVER_0, 
                     "(%"ID_UINT32_FMT"/%"ID_UINT32_FMT")\n"
                     "GLOBAL_TX_ID : %"ID_UINT32_FMT"\n"
                     "TX_ID        : %"ID_UINT32_FMT"\n"
                     "XID          : %s\n"
                     "TX_RESULT    : %s\n"
                     "TARGET_INFO  : %s",
                     (i+1), sInfoCount,
                     sInfo[i].mGlobalTransactionId,
                     sInfo[i].mLocalTransactionId,
                     sXIDStr,
                     sInfo[i].mTransactionResult,
                     sInfo[i].mTargetInfo );
    }

    IDE_TEST( dkmFreeInfoMemory( sInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dkiCopyXID( ID_XID * aDst, ID_XID * aSrc )
{
    dktXid::copyXID( aDst, aSrc );
}

idBool dkiIsUsableNEqualXID( ID_XID * aTargetXID, ID_XID * aSourceXID )
{
    dktDtxInfo  * sDtxInfo = NULL;
    dktNotifier * sNotifier  = dktGlobalTxMgr::getNotifier();
    idBool        sIsUsableNEqual = ID_FALSE;

    IDE_DASSERT( sNotifier != NULL );

    if ( dktXid::isEqualGlobalXID( aTargetXID, aSourceXID ) == ID_TRUE )
    {
        sNotifier->lock();

        if ( sNotifier->findDtxInfoByXID( DK_NOTIFY_FAILOVER,
                                          ID_TRUE,
                                          aTargetXID, 
                                          &sDtxInfo )
             == ID_TRUE )
        {
            if ( sDtxInfo->mIsFailoverRequestNode == ID_TRUE )
            {
                sIsUsableNEqual = ID_TRUE;
            }
        }
        else
        {
            sIsUsableNEqual = ID_TRUE;
        }

        sNotifier->unlock();
    }

    return sIsUsableNEqual;
}

void dkiNotifierSetPause( idBool aPause )
{
    dktNotifier * sNotifier  = dktGlobalTxMgr::getNotifier();
    IDE_DASSERT( sNotifier != NULL );

    sNotifier->setPause( aPause );
}

void dkiNotifierWaitUntilFailoverRunOneCycle()
{
    dktNotifier * sNotifier  = dktGlobalTxMgr::getNotifier();
    IDE_DASSERT( sNotifier != NULL );

    sNotifier->waitUntilFailoverNotifierRunOneCycle();
}

IDE_RC dkiNotifierAddUnCompleteGlobalTxList( iduList * aGlobalTxList )
{
    dktNotifier * sNotifier  = dktGlobalTxMgr::getNotifier();
    IDE_DASSERT( sNotifier != NULL );

    return sNotifier->addUnCompleteGlobalTxList( aGlobalTxList );
}

dkmSession * dkiSessionGetDkmSessionWithoutCreate( dkiSession * aSession )
{
    return aSession->mSession;
}

