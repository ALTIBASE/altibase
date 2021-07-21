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
 * $Id$
 **********************************************************************/

#ifndef _O_DKM_H_
#define _O_DKM_H_ 1

#include <qci.h>
#include <dkDef.h>
#include <dksDef.h>

typedef struct dksDataSession dkmSession;

/*
 * DK initialize
 */ 
extern IDE_RC dkmCheckDblinkEnabled( void );
extern IDE_RC dkmInitialize();
extern IDE_RC dkmFinalize();

/*
 * Session initialize
 */ 
extern IDE_RC dkmSessionInitialize( UInt          aSessionID,
                                    dkmSession  * aSession_,
                                    dkmSession ** aSession );
extern IDE_RC dkmSessionFinalize( dkmSession * aSession );

extern void dkmSessionSetUserId( dkmSession * aSession, UInt aUserId );

extern IDE_RC dkmCheckSessionAndStatus( dkmSession * aSession );

/*
 * System property
 */ 
extern IDE_RC dkmGetGlobalTransactionLevel( UInt * aValue );
extern IDE_RC dkmGetRemoteStatementAutoCommit( UInt * aValue );

/*
 * Session Property
 */ 
extern IDE_RC dkmSessionSetGlobalTransactionLevel( dkmSession * aSession,
                                                   UInt aValue );
extern IDE_RC dkmSessionSetRemoteStatementAutoCommit( dkmSession * aSession,
                                                      UInt aValue );

/*
 * Transaction
 */
extern void dkmSetGtxPreparedStatus( dkmSession * aSession );
extern IDE_RC dkmPrepare( dkmSession * aSession, ID_XID * aSourceXID );
extern IDE_RC dkmCommit( dkmSession * aSession );
extern IDE_RC dkmRollback( dkmSession * aSession, SChar * aSavepoint );
extern IDE_RC dkmRollbackForce( dkmSession * aSession );
extern IDE_RC dkmSavepoint( dkmSession * aSession, const SChar * aSavepoint );

/*
 * Performance views.
 */
typedef struct dkmAltiLinkerStatus
{
    
    UInt mStatus;

    UInt mSessionCount;

    UInt mRemoteSessionCount;
    
    UInt mJvmMemoryPoolMaxSize;
    
    SLong mJvmMemoryUsage;
    
    SChar mStartTime[ 128 ];
    
} dkmAltiLinkerStatus;

extern IDE_RC dkmGetAltiLinkerStatus( dkmAltiLinkerStatus * aStatus );

typedef struct dkmDatabaseLinkInfo
{
    
    UInt mId;

    UInt mStatus;

    UInt mReferenceCount;

} dkmDatabaseLinkInfo;

extern IDE_RC dkmGetDatabaseLinkInfo( dkmDatabaseLinkInfo ** aInfo,
                                      UInt * aInfoCount );

typedef struct dkmLinkerSessionInfo
{
    
    UInt mId;

    UInt mStatus;

    UInt mType;
    
} dkmLinkerSessionInfo;

extern IDE_RC dkmGetLinkerSessionInfo( dkmLinkerSessionInfo ** aInfo,
                                       UInt * aInfoCount );

typedef struct dkmLinkerControlSessionInfo
{
    
    UInt mStatus;

    UInt mReferenceCount;
    
} dkmLinkerControlSessionInfo;

extern IDE_RC dkmGetLinkerControlSessionInfo(
    dkmLinkerControlSessionInfo ** aInfo );

typedef struct dkmLinkerDataSessionInfo
{

    UInt mId;

    UInt mStatus;

    UInt mLocalTransactionId;

    UInt mGlobalTransactionId;

} dkmLinkerDataSessionInfo;

extern IDE_RC dkmGetLinkerDataSessionInfo( dkmLinkerDataSessionInfo ** aInfo,
                                           UInt * aInfoCount );

typedef struct dkmGlobalTransactionInfo
{
    UInt mGlobalTxId;

    UInt mLocalTxId;
    
    UInt mStatus;

    UInt mSessionId;

    UInt mRemoteTransactionCount;

    UInt mTransactionLevel;
    
} dkmGlobalTransactionInfo;

extern IDE_RC dkmGetGlobalTransactionInfo( dkmGlobalTransactionInfo ** aInfo,
                                           UInt * aInfoCount );

typedef struct dkmRemoteTransactionInfo
{

    UInt mGlobalTxId;

    UInt mLocalTxId;
    
    UInt mRemoteTransactionId;

    ID_XID mXID;
    
    SChar mTargetInfo[ QCI_MAX_NAME_LEN + 1 ];

    UInt mStatus;
    
} dkmRemoteTransactionInfo;

extern IDE_RC dkmGetRemoteTransactionInfo( dkmRemoteTransactionInfo ** aInfo,
                                           UInt * aInfoCount );

typedef struct dkmRemoteStatementInfo
{

    UInt mGlobalTxId;

    UInt mLocalTxId;
        
    UInt mRemoteTransactionId;

    SLong mStatementId;

    SChar mQuery[ DK_MAX_SQLTEXT_LEN ];

} dkmRemoteStatementInfo;

extern IDE_RC dkmGetRemoteStatementInfo( dkmRemoteStatementInfo ** aInfo,
                                         UInt * aInfoCount );

extern IDE_RC dkmFreeInfoMemory( void * aInfo );

typedef struct dkmNotifierTransactionInfo
{
    UInt  mGlobalTransactionId;

    UInt  mLocalTransactionId;

    ID_XID mXID;
    
    ID_XID mGlobalXID;

    UInt  mIsRequestNode;

    SChar mTransactionResult[DK_TX_RESULT_STR_SIZE] ;

    SChar mTargetInfo[ DK_NAME_LEN + 1 ];

    ID_XID mSourceXID;

    UInt   mTransactionState;

    smSCN  mGlobalCommitSCN;
} dkmNotifierTransactionInfo;

extern IDE_RC dkmGetNotifierTransactionInfo( dkmNotifierTransactionInfo ** aInfo,
                                             UInt * aInfoCount );

extern IDE_RC dkmGetShardNotifierTransactionInfo( dkmNotifierTransactionInfo ** aInfo,
                                             UInt * aInfoCount );

/*
 * Shard
 */
extern IDE_RC dkmOpenShardConnection( dkmSession     * aSession,
                                      sdiConnectInfo * aConnectInfo );

extern void dkmCloseShardConnection( sdiConnectInfo * aConnectInfo );

extern IDE_RC dkmAddShardTransaction( idvSQL         * aStatistics,
                                      dkmSession     * aSession,
                                      smTID            aTransID,
                                      sdiClientInfo  * aClientInfo,
                                      sdiConnectInfo * aConnectInfo );

extern void dkmDelShardTransaction( dkmSession     * aSession,
                                    sdiConnectInfo * aConnectInfo );

extern IDE_RC dkmSetTransactionBrokenOnGlobalCoordinator( dkmSession * aSession,
                                                          smTID        aTransID );

/*
 * Functions.
 */

extern IDE_RC dkmCalculateAllocStatement( void * aQcStatement,
                                          dkmSession * aSession,
                                          SChar * aDblinkName,
                                          SChar * aStmtStr,
                                          SLong * aStatementId );

extern IDE_RC dkmCalculateExecuteStatement( void * aQcStatement,
                                            dkmSession * aSession,
                                            SChar * aDblinkName,
                                            SLong aStatementId,
                                            SInt * aResult );

extern IDE_RC dkmCalculateAddBatch( void * aQcStatement,
                                    dkmSession * aSession,
                                    SChar * aDblinkName,
                                    SLong aStatementId,
                                    SInt * aResult );

extern IDE_RC dkmCalculateExecuteBatch( void * aQcStatement,
                                        dkmSession * aSession,
                                        SChar * aDblinkName,
                                        SLong aStatementId,
                                        SInt * aResult );

extern IDE_RC dkmCalculateExecuteImmediate( void * aQcStatement,
                                            dkmSession * aSession,
                                            SChar * aDblinkName,
                                            SChar * aQueryString,
                                            SInt * aResult );


extern IDE_RC dkmCalculateBindVariable( void * aQcStatement,
                                        dkmSession * aSession,
                                        SChar * aDblinkName,
                                        SLong aStatementId,
                                        UInt aVariableIndex,
                                        SChar * aValue );

extern IDE_RC dkmCalculateBindVariableBatch( void * aQcStatement,
                                             dkmSession * aSession,
                                             SChar * aDblinkName,
                                             SLong aStatementId,
                                             UInt aVariableIndex,
                                             SChar * aValue );

extern IDE_RC dkmCalculateFreeStatement( void * aQcStatement,
                                         dkmSession * aSession,
                                         SChar * aDblinkName,
                                         SLong aStatementId );

extern IDE_RC dkmCalculateNextRow( void * aQcStatement,
                                   dkmSession * aSession,
                                   SChar * aDblinkName,
                                   SLong aStatementId,
                                   SInt * aResult );

extern IDE_RC dkmCalculateGetColumnValue( void * aQcStatement,
                                          dkmSession * aSession,
                                          SChar * aDblinkName,
                                          SLong aStatementId,
                                          UInt aColumnIndex,
                                          mtcColumn ** aColumn,
                                          void ** aColumnValue );


extern IDE_RC dkmCalculateGetRemoteErrorInfo( void * aQcStatement,
                                              dkmSession * aSession,
                                              SChar * aDblinkName,
                                              SLong aStatementId,
                                              SChar * aErrorInfoBuffer,
                                              UInt aErrorInfoBufferSize,
                                              UShort * aErrorInfoLength );

extern IDE_RC dkmCalculateAllocStatementBatch( void * aQcStatement,
                                               dkmSession * aSession,
                                               SChar * aDblinkName,
                                               SChar * aStmtStr,
                                               SLong * aStatementId );

extern IDE_RC dkmCalculateFreeStatementBatch( void * aQcStatement,
                                              dkmSession * aSession,
                                              SChar * aDblinkName,
                                              SLong aStatementId );

extern IDE_RC dkmCalculateGetResultCountBatch( void * aQcStatement,
                                               dkmSession * aSession,
                                               SChar * aDblinkName,
                                               SLong aStatementId,
                                               SInt * aResultCount );

extern IDE_RC dkmCalculateGetResultBatch( void * aQcStatement,
                                          dkmSession * aSession,
                                          SChar * aDblinkName,
                                          SLong aStatementId,
                                          SInt aIndex,
                                          SInt * aResult );

/*
 * Altilinker process control
 */
extern IDE_RC dkmStartAltilinkerProcess( void );
extern IDE_RC dkmStopAltilinkerProcess( idBool aForceFlag );
extern IDE_RC dkmDumpAltilinkerProcess( void );
/*
 * Close database link session.
 */ 
extern IDE_RC dkmCloseSessionAll( idvSQL *aStatistics, dkmSession *aSession );
extern IDE_RC dkmCloseSession( idvSQL *aStatistics,
                               dkmSession *aSession,
                               SChar *aDblinkName );


/*
 * Create database link.
 */ 
extern IDE_RC dkmValidateCreateDatabaseLink( void * aQcStatement,
                                             idBool aPublicFlag,
                                             SChar * aDatabaseLinkName,
                                             SChar * aUserID,
                                             SChar * aPassword,
                                             SChar * aTargetName );

extern IDE_RC dkmExecuteCreateDatabaseLink( void * aQcStatement,
                                            idBool aPublicFlag,
                                            SChar * aDatabaseLinkName,
                                            SChar * aUserID,
                                            SChar * aPassword,
                                            SChar * aTargetName );

/*
 * Drop database link.
 */ 
extern IDE_RC dkmValidateDropDatabaseLink( void * aQcStatement,
                                           idBool aPublicFlag,
                                           SChar * aDatabaseLinkName );

extern IDE_RC dkmExecuteDropDatabaseLink( void * aQcStatement,
                                          idBool aPublicFlag,
                                          SChar * aDatabaseLinkName );

extern IDE_RC dkmDropDatabaseLinkByUserId( idvSQL * aStatistics,
                                           void   * aQcStatement,
                                           UInt     aUserId );

/*
 * for remote_table()
 */
typedef qciRemoteTableColumnInfo dkmColumnInfo;

extern IDE_RC dkmGetColumnInfoFromCache( void * aQcStatement,
                                         dkmSession * aSession,
                                         SChar * aDatabaseLinkName,
                                         SChar * aRemoteQuery,
                                         void ** aRemoteTableHandle,
                                         SInt * aColumnCount,
                                         dkmColumnInfo ** aColumnInfoArray );

extern IDE_RC dkmInvalidResultSetMetaCache( SChar * aDatabaseLinkName,
                                            SChar * aRemoteQuery );

extern IDE_RC dkmFreeColumnInfo( dkmColumnInfo * aColumnInfoArray );

extern IDE_RC dkmAllocAndExecuteQueryStatement( void * aQcStatement,
                                                dkmSession * aSession,
                                                idBool  aIsStore,
                                                SChar * aDatabaseLinkName,
                                                SChar * aRemoteQuery,
                                                SLong * aStatementId );

extern IDE_RC dkmAllocAndExecuteQueryStatementWithoutFetch(
    void * aQcStatement,
    dkmSession * aSession,
    idBool  aIsStore,
    SChar * aDatabaseLinkName,
    SChar * aRemoteQuery,
    SLong * aStatementId );

extern IDE_RC dkmFreeQueryStatement( dkmSession * aSession,
                                     SLong aStatementId );

extern IDE_RC dkmGetLinkObjectHandle( idvSQL * aStatistics,
                                      SChar  * aDatabaseLinkName,
                                      void  ** aLinkObjectHandle );

extern IDE_RC dkmGetColumnInfo( dkmSession * aSession,
                                SLong aStatementId,
                                SInt * aColumnCount,
                                dkmColumnInfo ** aColumnInfoArray );

extern IDE_RC dkmFetchNextRow( dkmSession * aSession,
                               SLong aStatementId,
                               void ** aRow );

extern IDE_RC dkmRestartRow( dkmSession * aSession,
                             SLong aStatementId );

extern IDE_RC dkmLoadDblinkFromMeta( idvSQL * aStatistics );

extern IDE_RC dkmGetNullRow( dkmSession * aSession,
                             SLong        aStatementId,
                             void      ** aRow,
                             scGRID     * aRid );

extern IDE_RC dkmIsFirst( dkmSession    *aSession,
                          SLong          aStatementId );

extern idBool dkmIs2PCSession( dkmSession *aSession );

extern IDE_RC dkmIsExistRemoteTx( dkmSession * aSession,
                                  idBool     * aIsExist );

extern IDE_RC dkmEndPendingPassiveDtxInfo( ID_XID * aXID, idBool aCommit );
extern IDE_RC dkmEndPendingFailoverDtxInfo( ID_XID * aXID, 
                                            idBool   aCommit,
                                            smSCN  * aGlobalCommitSCN );

extern IDE_RC dkmCheckGlobalTransactionStatus( dkmSession     * aSession );

extern IDE_RC dkmAddDtxBranchTx( void   * aDtxInfo,
                                 UChar    aCoordinatorType,
                                 SChar  * aNodeName,
                                 SChar  * aUserName,
                                 SChar  * aUserPassword,
                                 SChar  * aDataServerIP,
                                 UShort   aDataPortNo,
                                 UShort   aConnectType );

#endif /* _O_DKM_H_ */
