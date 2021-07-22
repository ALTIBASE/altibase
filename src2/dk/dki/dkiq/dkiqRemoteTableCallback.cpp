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

#include <smi.h>

#include <mtc.h>

#include <qci.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkm.h>

#include <dkiSession.h>
#include <dkiq.h>

/*
 *
 */ 
static IDE_RC getRemoteTableMetaFromRemote(
    void * aQcStatement,
    dkmSession * aSession,
    idBool  aIsStore,
    SChar * aDatabaseLinkName,
    SChar * aRemoteQuery,
    void ** aRemoteTableHandle,
    SInt * aColumnCount,
    qciRemoteTableColumnInfo ** aColumnInfoArray )
{
    SInt sColumnCount = 0;    
    qciRemoteTableColumnInfo * sColumnInfoArray = NULL;
    void * sTableHandle = NULL;
    SLong sStatementId = 0;
    SInt sStage = 0;

    IDE_TEST_RAISE( aQcStatement == NULL, ERR_STATEMENT_IS_NULL );

    IDE_TEST( dkmAllocAndExecuteQueryStatementWithoutFetch( aQcStatement,
                                                            aSession,
                                                            aIsStore,
                                                            aDatabaseLinkName,
                                                            aRemoteQuery,
                                                            &sStatementId )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( dkmGetLinkObjectHandle( qciMisc::getStatisticsFromQcStatement( aQcStatement ),
                                      aDatabaseLinkName,
                                      &sTableHandle )
              != IDE_SUCCESS );
    
    IDE_TEST( dkmGetColumnInfo( aSession,
                                sStatementId,
                                &sColumnCount,
                                &sColumnInfoArray )
              != IDE_SUCCESS );
    sStage = 2;
    
    IDE_TEST( dkmFreeQueryStatement( aSession,
                                     sStatementId )
              != IDE_SUCCESS );

    *aRemoteTableHandle = sTableHandle;
    *aColumnCount = sColumnCount;
    *aColumnInfoArray = sColumnInfoArray;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)dkmFreeColumnInfo( sColumnInfoArray );
        case 1:
            (void)dkmFreeQueryStatement( aSession, sStatementId );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
static IDE_RC dkiqGetRemoteTableMeta(
    void * aQcStatement,
    void * aDkiSession,
    idBool  aIsStore,
    SChar * aDatabaseLinkName,
    SChar * aRemoteQuery,
    void ** aRemoteTableHandle,
    SInt * aColumnCount,
    qciRemoteTableColumnInfo ** aColumnInfoArray )
{
    dkmSession * sSession = NULL;
    SInt sColumnCount = 0;    
    qciRemoteTableColumnInfo * sColumnInfoArray = NULL;
    void * sTableHandle = NULL;

    IDE_TEST_RAISE( aQcStatement == NULL, ERR_STATEMENT_IS_NULL );

    sSession = dkiSessionGetDkmSession( (dkiSession *)aDkiSession );
    IDE_TEST( dkmCheckSessionAndStatus( sSession ) != IDE_SUCCESS );
    
    IDE_TEST( dkmGetColumnInfoFromCache( aQcStatement,
                                         sSession,
                                         aDatabaseLinkName,
                                         aRemoteQuery,
                                         &sTableHandle,
                                         &sColumnCount,
                                         &sColumnInfoArray )
              != IDE_SUCCESS );
    
    if ( sTableHandle == NULL )
    {
        /*
         * if not, then prepare and execute remote query and get result
         * set meta. make remote meta info.
         */
        IDE_TEST( getRemoteTableMetaFromRemote( aQcStatement,
                                                sSession,
                                                aIsStore,
                                                aDatabaseLinkName,
                                                aRemoteQuery,
                                                &sTableHandle,
                                                &sColumnCount,
                                                &sColumnInfoArray )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    *aRemoteTableHandle = sTableHandle;
    *aColumnCount = sColumnCount;
    *aColumnInfoArray = sColumnInfoArray;

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
static IDE_RC dkiqFreeRemoteTableColumnInfo(
    qciRemoteTableColumnInfo * aColumnInfoArray )
{
    IDE_PUSH();
    
    (void)dkmFreeColumnInfo( aColumnInfoArray );

    IDE_POP();

    return IDE_SUCCESS;
}

/*
 *
 */ 
static IDE_RC dkiqInvalidResultSetMetaCache( SChar * aDatabaseLinkName,
                                             SChar * aRemoteQuery )
{
    IDE_TEST( dkmInvalidResultSetMetaCache( aDatabaseLinkName, aRemoteQuery )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;    
}


static qciRemoteTableCallback gRemoteTableCallback =
{
    dkiqGetRemoteTableMeta,
    dkiqFreeRemoteTableColumnInfo,

    dkiqInvalidResultSetMetaCache
}; 

/*
 *
 */ 
IDE_RC dkiqRegisterRemoteTableCallback( void )
{
    IDE_TEST( qci::setRemoteTableCallback( &gRemoteTableCallback )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
