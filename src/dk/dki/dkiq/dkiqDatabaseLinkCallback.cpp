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

#include <qci.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkm.h>
#include <dkiSession.h>

#include <dkiq.h>

/*
 *
 */ 
static IDE_RC dkiqStartDatabaseLinker( void )
{
    IDE_TEST( dkmStartAltilinkerProcess() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkiqStopDatabaseLinker( idBool aForceFlag )
{
    IDE_TEST( dkmStopAltilinkerProcess( aForceFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkiqDumpDatabaseLinker( void )
{
    IDE_TEST( dkmDumpAltilinkerProcess() != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}
/*
 *
 */
static IDE_RC dkiqCloseDatabaseLinkAll( idvSQL * aStatistics, void * aDkiSession )
{
    dkmSession * sSession = NULL;

    sSession = dkiSessionGetDkmSession( (dkiSession *)aDkiSession );

    if ( sSession != NULL )
    {
        IDE_TEST( dkmCloseSessionAll( aStatistics, sSession ) != IDE_SUCCESS );

        (void)dkiSessionSetDkmSessionNull( (dkiSession *)aDkiSession );
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
static IDE_RC dkiqCloseDatabaseLink( idvSQL * aStatistics,
                                     void   * aDkiSession,
                                     SChar  * aDatabaseLinkName )
{
    dkmSession * sSession = NULL;
    
    sSession = dkiSessionGetDkmSession( (dkiSession *)aDkiSession );

    if ( sSession != NULL )
    {
        IDE_TEST( dkmCloseSession( aStatistics, sSession, aDatabaseLinkName )
                  != IDE_SUCCESS );

        (void)dkiSessionSetDkmSessionNull( (dkiSession *)aDkiSession );
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
static IDE_RC dkiqValidateCreateDatabaseLink( void * aQcStatement,
                                              idBool aPublicFlag,
                                              SChar * aDatabaseLinkName,
                                              SChar * aUserId,
                                              SChar * aPassword,
                                              SChar * aTargetName )
{
    IDE_TEST( dkmValidateCreateDatabaseLink( aQcStatement,
                                             aPublicFlag,
                                             aDatabaseLinkName,
                                             aUserId,
                                             aPassword,
                                             aTargetName )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkiqExecuteCreateDatabaseLink( void * aQcStatement,
                                             idBool aPublicFlag,
                                             SChar * aDatabaseLinkName,
                                             SChar * aUserId,
                                             SChar * aPassword,
                                             SChar * aTargetName )
{
    IDE_TEST_RAISE( aQcStatement == NULL, ERR_STATEMENT_IS_NULL );
    
    IDE_TEST( dkmExecuteCreateDatabaseLink( aQcStatement,
                                            aPublicFlag,
                                            aDatabaseLinkName,
                                            aUserId,
                                            aPassword,
                                            aTargetName )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkiqValidateDropDatabaseLink( void * aQcStatement,
                                            idBool aPublicFlag,
                                            SChar * aDatabaseLinkName )
{
    IDE_TEST_RAISE( aQcStatement == NULL, ERR_STATEMENT_IS_NULL );
    
    IDE_TEST( dkmValidateDropDatabaseLink( aQcStatement,
                                           aPublicFlag,
                                           aDatabaseLinkName )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */  
static IDE_RC dkiqExecuteDropDatabaseLink( void * aQcStatement,
                                           idBool aPublicFlag,
                                           SChar * aDatabaseLinkName )
{
    IDE_TEST_RAISE( aQcStatement == NULL, ERR_STATEMENT_IS_NULL );
    
    IDE_TEST( dkmExecuteDropDatabaseLink( aQcStatement,
                                          aPublicFlag,
                                          aDatabaseLinkName )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC dkiqDropDatabaseLinkByUserId( void * aQcStatement, UInt aUserId )
{
    IDE_TEST_RAISE( aQcStatement == NULL, ERR_STATEMENT_IS_NULL );
    
    IDE_TEST( dkmDropDatabaseLinkByUserId( qciMisc::getStatisticsFromQcStatement( aQcStatement ),
                                           aQcStatement,
                                           aUserId )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;    
}

static IDE_RC dkiOpenShardConnection( void * aDataNode )
{
    sdiConnectInfo * sDataNode = (sdiConnectInfo*)aDataNode;
    dkmSession     * sSession  = NULL;

    IDE_DASSERT( sDataNode != NULL );
    IDE_DASSERT( sDataNode->mDkiSession != NULL );

    sSession = dkiSessionGetDkmSessionWithoutCreate( (dkiSession*)sDataNode->mDkiSession );

    IDE_TEST( dkmOpenShardConnection( sSession, sDataNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );

    return IDE_FAILURE;
}

static void dkiCloseShardConnection( void * aDataNode )
{
    sdiConnectInfo * sDataNode = (sdiConnectInfo*)aDataNode;

    IDE_DASSERT( sDataNode != NULL );

    dkmCloseShardConnection( sDataNode );
}

static IDE_RC dkiAddShardTransaction( idvSQL        * aStatistics,
                                      smTID           aTransID,
                                      sdiClientInfo * aClientInfo,
                                      void          * aDataNode )
{
    sdiConnectInfo * sDataNode = (sdiConnectInfo*)aDataNode;
    dkmSession     * sSession  = NULL;

    IDE_DASSERT( sDataNode != NULL );
    IDE_DASSERT( sDataNode->mDkiSession != NULL );

    sSession = dkiSessionGetDkmSessionWithoutCreate( (dkiSession*)sDataNode->mDkiSession );

    IDE_TEST( dkmAddShardTransaction( aStatistics,
                                      sSession,
                                      aTransID,
                                      aClientInfo,
                                      sDataNode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );

    return IDE_FAILURE;
}

static void dkiDelShardTransaction( void * aDataNode )
{
    sdiConnectInfo * sDataNode = (sdiConnectInfo*)aDataNode;
    dkmSession     * sSession  = NULL;

    IDE_DASSERT( sDataNode != NULL );
    IDE_DASSERT( sDataNode->mDkiSession != NULL );

    sSession = dkiSessionGetDkmSessionWithoutCreate( (dkiSession*)sDataNode->mDkiSession );

    dkmDelShardTransaction( sSession, sDataNode );
}

static IDE_RC dkiSetTransactionBrokenOnGlobalCoordinator( void  * aDkiSession,
                                                          smTID   aTransID )
{
    dkmSession * sSession  = NULL;

    IDE_DASSERT( aDkiSession != NULL );

    sSession = dkiSessionGetDkmSessionWithoutCreate( (dkiSession*)aDkiSession );

    IDE_TEST( dkmSetTransactionBrokenOnGlobalCoordinator( sSession, aTransID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );

    return IDE_FAILURE;
}

static IDE_RC dkiCheckGloablTransactionStatus( void          * aDataNode )
{
    sdiConnectInfo * sDataNode = (sdiConnectInfo*)aDataNode;
    dkmSession     * sSession  = NULL;

    IDE_DASSERT( sDataNode != NULL );
    IDE_DASSERT( sDataNode->mDkiSession != NULL );

    sSession = dkiSessionGetDkmSessionWithoutCreate( (dkiSession*)sDataNode->mDkiSession );

    IDE_TEST( dkmCheckGlobalTransactionStatus( sSession ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC dkiAddDtxBranchTx( void   * aDtxInfo,
                                 UChar    aCoordinatorType,
                                 SChar  * aNodeName,
                                 SChar  * aUserName,
                                 SChar  * aUserPassword,
                                 SChar  * aDataServerIP,
                                 UShort   aDataPortNo,
                                 UShort   aConnectType )
{
    IDE_TEST( dkmAddDtxBranchTx( aDtxInfo,
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

static qciDatabaseLinkCallback gDatabaseLinkCallack =
{
    dkiqStartDatabaseLinker,
    dkiqStopDatabaseLinker,
    dkiqDumpDatabaseLinker,
    
    dkiqCloseDatabaseLinkAll,
    dkiqCloseDatabaseLink,
    
    dkiqValidateCreateDatabaseLink,
    dkiqExecuteCreateDatabaseLink,
    
    dkiqValidateDropDatabaseLink,
    dkiqExecuteDropDatabaseLink,

    dkiqDropDatabaseLinkByUserId,

    dkiOpenShardConnection,
    dkiCloseShardConnection,

    dkiAddShardTransaction,
    dkiDelShardTransaction,

    dkiSetTransactionBrokenOnGlobalCoordinator,

    dkiCheckGloablTransactionStatus,

    dkiAddDtxBranchTx
};

/*
 *
 */ 
IDE_RC dkiqRegisterDatabaseLinkCallback( void )
{
    IDE_TEST( qci::setDatabaseLinkCallback( &gDatabaseLinkCallack )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}
