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
 

#include <ide.h>
#include <idl.h>

#include <qc.h>
#include <qcg.h>
#include <qcm.h>

#include <qdkParseTree.h>
#include <qdk.h>

static qcDatabaseLinkCallback gCallback = {
    NULL,   /* mStartDatabaseLinker        */
    NULL,   /* mStopDatabaseLinker         */
    NULL,   /* mDumpDatabaseLinker         */
    NULL,   /* mCloseDatabaseLinkAll       */
    NULL,   /* mCloseDatabaseLink          */
    NULL,   /* mValidateCreateDatabaseLink */
    NULL,   /* mExecuteCreateDatabaseLink  */
    NULL,   /* mValidateDropDatabaseLink   */
    NULL,   /* mExecuteDropDatabaseLink    */
    NULL,   /* mDropDatabaseLinkByUserId   */
    NULL,   /* mOpenShardConnection        */
    NULL,   /* mCloseShardConnection       */
    NULL,   /* mAddShardTransaction        */
    NULL,   /* mDelShardTransaction        */
    NULL,   /* mSetTransactionBrokenOnGlobalCoordinator */
    NULL,   /* mCheckGlobalTransactionStatus */
    NULL    /* mAddDtxBranchTx             */
};


/*
 *
 */
static void qdkConvertQcNamePositionToCString(
    SChar           * aString,
    qcNamePosition  * aQcNamePosition)
{
    if ( ( aQcNamePosition->stmtText != NULL ) &&
         ( aQcNamePosition->size > 0 ) )
    {
        idlOS::strncpy( aString,
                        aQcNamePosition->stmtText + aQcNamePosition->offset,
                        aQcNamePosition->size );
        aString[ aQcNamePosition->size ] = '\0';        
    }
    else
    {
        aString[ 0 ] = '\0';                
    }
}

/*
 *
 */ 
IDE_RC qdkSetDatabaseLinkCallback( qcDatabaseLinkCallback * aCallback )
{
    gCallback = *aCallback;
    
    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC qdkControlDatabaseLinker( qcStatement                   * aStatement,
                                 qdkDatabaseLinkAlterParseTree * aParseTree )
{
    // check privileges
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_IS_SYSDBA( aStatement ) != ID_TRUE,
                    ERR_NO_GRANT_SYSDBA );
    
    switch ( aParseTree->control )
    {
        case QDK_DATABASE_LINKER_START:
            if ( gCallback.mStartDatabaseLinker != NULL )
            {
                IDE_TEST( gCallback.mStartDatabaseLinker() != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
            break;
            
        case QDK_DATABASE_LINKER_STOP:
            if ( gCallback.mStopDatabaseLinker != NULL )
            {
                IDE_TEST( gCallback.mStopDatabaseLinker(
                              aParseTree->forceFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
            break;
        case QDK_DATABASE_LINKER_DUMP:
            if ( gCallback.mDumpDatabaseLinker != NULL )
            {
                IDE_TEST( gCallback.mDumpDatabaseLinker() != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_SYSDBA )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_SYSDBA_STR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC qdkCloseDatabaseLink( qcStatement                   * aStatement,
                             void                          * aDatabaseLinkSession,
                             qdkDatabaseLinkCloseParseTree * aParseTree )
{
    SChar sName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };
    
    switch ( aParseTree->allFlag )
    {
        case ID_TRUE:
            if ( gCallback.mCloseDatabaseLinkAll != NULL )
            {
                IDE_TEST( gCallback.mCloseDatabaseLinkAll(
                              aStatement->mStatistics,
                              aDatabaseLinkSession )
                          != IDE_SUCCESS );                
            }
            else
            {
                /* do nothing */
            }
            
            break;

        case ID_FALSE:
            qdkConvertQcNamePositionToCString(
                sName,
                &(aParseTree->databaseLinkName) );
                                               
            if ( gCallback.mCloseDatabaseLink != NULL )
            {
                IDE_TEST( gCallback.mCloseDatabaseLink( aStatement->mStatistics,
                                                        aDatabaseLinkSession,
                                                        sName )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC qdkValidateCreateDatabaseLink( qcStatement * aStatement )
{
    qdkDatabaseLinkCreateParseTree * sParseTree =
        (qdkDatabaseLinkCreateParseTree *)QC_PARSETREE( aStatement );
    
    SChar sDatabaseLinkName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };
    SChar sUserId[ QC_MAX_OBJECT_NAME_LEN + 1 ]           = { 0, };
    SChar sPassword[ QC_MAX_OBJECT_NAME_LEN + 1 ]         = { 0, };
    SChar sTargetName[ QC_MAX_OBJECT_NAME_LEN + 1 ]       = { 0, };

    if ( gCallback.mValidateCreateDatabaseLink != NULL )
    {
        qdkConvertQcNamePositionToCString( sDatabaseLinkName,
                                           &(sParseTree->name) );
        qdkConvertQcNamePositionToCString( sUserId,
                                           &(sParseTree->userId) );
        qdkConvertQcNamePositionToCString( sPassword,
                                           &(sParseTree->password) );
        qdkConvertQcNamePositionToCString( sTargetName,
                                           &(sParseTree->targetName) );
        
        IDE_TEST( gCallback.mValidateCreateDatabaseLink(
                      (void *)aStatement,
                      sParseTree->publicFlag,
                      sDatabaseLinkName,
                      sUserId,
                      sPassword,
                      sTargetName )
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

/*
 *
 */ 
IDE_RC qdkExecuteCreateDatabaseLink( qcStatement * aStatement )
{
    qdkDatabaseLinkCreateParseTree * sParseTree =
        (qdkDatabaseLinkCreateParseTree *)QC_PARSETREE( aStatement );
    
    SChar sDatabaseLinkName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };
    SChar sUserId[ QC_MAX_OBJECT_NAME_LEN + 1 ]           = { 0, };
    SChar sPassword[ QC_MAX_NAME_LEN + 1 ]         = { 0, };
    SChar sTargetName[ QC_MAX_OBJECT_NAME_LEN + 1 ]       = { 0, };

    if ( gCallback.mExecuteCreateDatabaseLink != NULL )
    {
        qdkConvertQcNamePositionToCString( sDatabaseLinkName,
                                           &(sParseTree->name) );
        qdkConvertQcNamePositionToCString( sUserId,
                                           &(sParseTree->userId) );
        qdkConvertQcNamePositionToCString( sPassword,
                                           &(sParseTree->password) );
        qdkConvertQcNamePositionToCString( sTargetName,
                                           &(sParseTree->targetName) );
        
        IDE_TEST( gCallback.mExecuteCreateDatabaseLink(
                      (void *)aStatement,
                      sParseTree->publicFlag,
                      sDatabaseLinkName,
                      sUserId,
                      sPassword,
                      sTargetName )
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

/*
 *
 */ 
IDE_RC qdkValidateDropDatabaseLink( qcStatement * aStatement )
{
    qdkDatabaseLinkDropParseTree * sParseTree =
        (qdkDatabaseLinkDropParseTree *)QC_PARSETREE( aStatement );
    
    SChar sDatabaseLinkName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };

    if ( gCallback.mValidateDropDatabaseLink != NULL )
    {
        qdkConvertQcNamePositionToCString( sDatabaseLinkName,
                                           &(sParseTree->name) );
        
        IDE_TEST( gCallback.mValidateDropDatabaseLink( (void *)aStatement,
                                                       sParseTree->publicFlag,
                                                       sDatabaseLinkName )
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

/*
 *
 */ 
IDE_RC qdkExecuteDropDatabaseLink( qcStatement * aStatement )
{
    qdkDatabaseLinkDropParseTree * sParseTree =
        (qdkDatabaseLinkDropParseTree *)QC_PARSETREE( aStatement );
    
    SChar sDatabaseLinkName[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };

    if ( gCallback.mExecuteDropDatabaseLink != NULL )
    {
        qdkConvertQcNamePositionToCString( sDatabaseLinkName,
                                           &(sParseTree->name) );
        
        IDE_TEST( gCallback.mExecuteDropDatabaseLink( (void *)aStatement,
                                                       sParseTree->publicFlag,
                                                       sDatabaseLinkName )
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

/*
 *
 */
IDE_RC qdkDropDatabaseLinkByUserId( qcStatement * aStatement, UInt aUserId )
{
    if ( gCallback.mDropDatabaseLinkByUserId != NULL )
    {
        IDE_TEST( gCallback.mDropDatabaseLinkByUserId( (void *)aStatement,
                                                       aUserId )
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

IDE_RC qdkOpenShardConnection( sdiConnectInfo * aDataNode )
{
    if ( gCallback.mOpenShardConnection != NULL )
    {
        IDE_TEST( gCallback.mOpenShardConnection( (void*)aDataNode )
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

void qdkCloseShardConnection( sdiConnectInfo * aDataNode )
{
    if ( gCallback.mCloseShardConnection != NULL )
    {
        gCallback.mCloseShardConnection( (void *)aDataNode );
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC qdkAddShardTransaction( idvSQL         * aStatistics,
                               smTID            aTransID,
                               sdiClientInfo  * aClientInfo,
                               sdiConnectInfo * aDataNode )
{
    if ( gCallback.mAddShardTransaction != NULL )
    {
        IDE_TEST( gCallback.mAddShardTransaction( aStatistics,
                                                  aTransID,
                                                  aClientInfo,
                                                  (void *)aDataNode )
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

void qdkDelShardTransaction( sdiConnectInfo * aDataNode )
{
    if ( gCallback.mDelShardTransaction != NULL )
    {
        gCallback.mDelShardTransaction( (void *)aDataNode );
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC qdkSetTransactionBrokenOnGlobalCoordinator( void * aDkiSession,
                                                   smTID  aTransID )
{
    if ( gCallback.mSetTransactionBrokenOnGlobalCoordinator != NULL )
    {
        IDE_TEST( gCallback.mSetTransactionBrokenOnGlobalCoordinator( aDkiSession,
                                                                      aTransID )
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

IDE_RC qdkCheckGlobalTransactionStatus( void * aDataNode )
{
    if ( gCallback.mCheckGlobalTransactionStatus != NULL )
    {
        IDE_TEST(  gCallback.mCheckGlobalTransactionStatus( aDataNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdkAddDtxBranchTx( void   * aDtxInfo,
                          UChar    aCoordinatorType,
                          SChar  * aNodeName,
                          SChar  * aUserName,
                          SChar  * aUserPassword,
                          SChar  * aDataServerIP,
                          UShort   aDataPortNo,
                          UShort   aConnectType )
{
    if ( gCallback.mAddDtxBranchTx != NULL )
    {
        IDE_TEST(  gCallback.mAddDtxBranchTx( aDtxInfo,
                                              aCoordinatorType,
                                              aNodeName,
                                              aUserName,
                                              aUserPassword,
                                              aDataServerIP,
                                              aDataPortNo,
                                              aConnectType )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

