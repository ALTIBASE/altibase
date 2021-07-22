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

#ifndef _O_DKP_PROTOCOL_MGR_H_
#define _O_DKP_PROTOCOL_MGR_H_ 1


#include <idTypes.h>
#include <dkp.h>
#include <dktDef.h>
#include <dkoDef.h>
#include <dkpBatchStatementMgr.h>

class dktDtxInfo;

class dkpProtocolMgr
{
private:
    static IDE_RC   sendPrepareRemoteStmtInternal( dksSession       *aSession,
                                                   dkpOperationID    aOperationId,
                                                   UInt              aSessionId,
                                                   idBool            aIsTxBegin,
                                                   ID_XID           *aXID,
                                                   dkoLink          *aLinkInfo,
                                                   SLong             aRemoteStmtId,
                                                   SChar            *aRemoteStmtStr );

    static IDE_RC   recvPrepareRemoteStmtResultInternal( dksSession     *aSession,
                                                         dkpOperationID  aOperationId,
                                                         UInt            aSessionId,
                                                         SLong           aRemoteStmtId,
                                                         UInt           *aResultCode,
                                                         UShort         *aRemoteNodeSessionId,
                                                         ULong           aTimeoutSec );

    static IDE_RC   sendRequestFreeRemoteStmtInternal( dksSession     *aSession,
                                                       dkpOperationID  aOperationId,
                                                       UInt            aSessionId,
                                                       SLong           aRemoteStmtId );
    
    static IDE_RC   recvRequestFreeRemoteStmtResultInternal( dksSession     *aSession,
                                                             dkpOperationID  aOperationId,
                                                             UInt            aSessionId,
                                                             SLong           aRemoteStmtId,
                                                             UInt           *aResultCode,
                                                             ULong           aTimeoutSec );
    
    static IDE_RC   readLinkBufferInternal( dknLink * aLink, 
                                            void    * aDestination, 
                                            UInt      aDestinationLen, 
                                            UInt    * aReadBufferLen, 
                                            idBool    aIsSkipRemain );
    
public:

    static PDL_Time_Value   mTV1Sec;

    /* Initialize / Finalize */
    static IDE_RC   initializeStatic();
    static IDE_RC   finalizeStatic();


    /* =================== Common functions =================== */

    static IDE_RC   setProtocolHeader( dksSession   *aSession,
                                       UChar         aOpID, 
                                       UChar         aParam,
                                       UChar         aFlags,
                                       UInt          aSessionID,
                                       UInt          aDataLength );

    static IDE_RC   setProtocolHeader( dknPacket    *aPacket,
                                       UChar         aOpID, 
                                       UChar         aParam,
                                       UChar         aFlags,
                                       UInt          aSessionId,
                                       UInt          aDataLength );
    
    static IDE_RC   getCmBlock( dksSession          *aSession,
                                idBool              *aIsTimeOut,
                                ULong                aTimeoutSec );

    static IDE_RC   analyzeCommonHeader( dksSession         *aSession,
                                         dkpOperationID      aOpID,
                                         UChar              *aFlags,
                                         UInt                aSessionId,
                                         UInt               *aResultCode,
                                         UInt               *aDataLen );

    static UChar    makeHeaderFlags( dksSession         * aSession,
                                     idBool               aIsTxBegin );

    /* =================== Control operations =================== */

    /* Request for AltiLinker to create linker control session */
    static IDE_RC   sendCreateLinkerCtrlSession( dksSession    *aSession, 
                                                 SChar         *aProdInfoStr, 
                                                 UInt           aDbCharSet );
    static IDE_RC   recvCreateLinkerCtrlSessionResult( dksSession    *aSession,
                                                       UInt          *aResultCode,
                                                       UInt          *aProtocolVer,
                                                       ULong          aTimeoutSec );

    /* Request for AltiLinker to create a linker data session */
    static IDE_RC   sendCreateLinkerDataSession( dksSession    *aSession,
                                                 UInt           aSessionId );
    static IDE_RC   recvCreateLinkerDataSessionResult( dksSession    *aSession,
                                                       UInt           aSessionId,
                                                       UInt          *aResultCode,
                                                       ULong          aTimeoutSec );

    /* Destroy a linker data session */
    static IDE_RC   sendDestroyLinkerDataSession( dksSession    *aSession,
                                                  UInt           aSessionId );
    static IDE_RC   recvDestroyLinkerDataSessionResult( dksSession    *aSession,
                                                        UInt           aSessionId,
                                                        UInt          *aResultCode,
                                                        ULong          aTimeoutSec );

    /* Request AltiLinker process' status info */
    static IDE_RC   sendRequestLinkerStatus( dksSession    *aSession );
    static IDE_RC   recvRequestLinkerStatusResult( dksSession       *aSession,
                                                   UInt             *aResultCode,
                                                   dkaLinkerStatus  *aLinkerStatus,
                                                   ULong             aTimeoutSec );

    /* Request information for performance view : V$DBLINK_ALTILINKER_STATUS */
    static IDE_RC   sendRequestDblinkAltiLnkerStatusPvInfo( dksSession        *aSession );
    static IDE_RC   recvRequestDblinkAltiLnkerStatusPvInfoResult( dksSession        *aSession,
                                                                  UInt              *aResultCode,
                                                                  dkaLinkerProcInfo *aLinkerProcInfo,
                                                                  ULong              aTimeoutSec ); 

    /* Request Prepare AltiLinker process shutdown */
    static IDE_RC   sendPrepareLinkerShutdown( dksSession    *aSession );
    static IDE_RC   recvPrepareLinkerShutdownResult( dksSession    *aSession,
                                                     UInt          *aResultCode,
                                                     ULong          aTimeoutSec );

    /* Request AltiLinker process dump */
    static IDE_RC   sendLinkerDump( dksSession    *aSession );
    static IDE_RC   recvLinkerDumpResult( dksSession    *aSession,
                                          UInt          *aResultCode,
                                          ULong          aTimeoutSec );

    /* Request shutdown AltiLinker process */
    static IDE_RC   doLinkerShutdown( dksSession  *aSession );

    
    /* =================== Data operations =================== */

    /* Request prepare remote statement */
    static IDE_RC   sendPrepareRemoteStmt( dksSession       *aSession,
                                           UInt              aSessionId,
                                           idBool            aIsTxBegin,
                                           ID_XID           *aXID,
                                           dkoLink          *aLinkInfo,
                                           SLong             aRemoteStmtId,
                                           SChar            *aRemoteStmtStr );
    static IDE_RC   recvPrepareRemoteStmtResult( dksSession    *aSession,
                                                 UInt           aSessionId,
                                                 SLong          aRemoteStmtId,
                                                 UInt          *aResultCode,
                                                 UShort        *aRemoteNodeSessionId,
                                                 ULong          aTimeoutSec );

    static IDE_RC   sendPrepareRemoteStmtBatch( dksSession   *aSession, 
                                                UInt          aSessionId, 
                                                idBool        aIsTxBegin,
                                                ID_XID       *aXID,
                                                dkoLink      *aLinkInfo, 
                                                SLong         aRemoteStmtId, 
                                                SChar        *aRemoteStmtStr );

    static IDE_RC   recvPrepareRemoteStmtBatchResult( dksSession    *aSession,
                                                      UInt           aSessionId,
                                                      SLong          aRemoteStmtId,
                                                      UInt          *aResultCode,
                                                      UShort        *aRemoteNodeSessionId,
                                                      ULong          aTimeoutSec );
    
    /* Request remote error information */
    static IDE_RC   sendRequestRemoteErrorInfo( dksSession    *aSession,
                                                UInt           aSessionId,
                                                SLong          aRemoteStmtId );
    static IDE_RC   recvRequestRemoteErrorInfoResult( dksSession    *aSession,
                                                      UInt           aSessionId,
                                                      UInt          *aResultCode,
                                                      SLong          aRemoteStmtId,
                                                      dktErrorInfo  *aErrorInfo,
                                                      ULong          aTimeoutSec );

    /* Execute remote statement */
    static IDE_RC   sendExecuteRemoteStmt( dksSession   *aSession,
                                           UInt          aSessionId,
                                           idBool        aIsTxBegin,
                                           ID_XID       *aXID,
                                           dkoLink      *aLinkInfo,
                                           SLong         aRemoteStmtId,
                                           UInt          aRemoteStmtType,
                                           SChar        *aRemoteStmtStr );
    static IDE_RC   recvExecuteRemoteStmtResult( dksSession    *aSession,
                                                 UInt           aSessionId,
                                                 SLong          aRemoteStmtId,
                                                 UInt          *aResultCode,
                                                 UShort        *aRemoteNodeSessionId,
                                                 UInt          *aStmtType,
                                                 SInt          *aResult,
                                                 ULong          aTimeoutSec );

    static IDE_RC   sendPrepareAddBatch( dksSession           *aSession,
                                         UInt                  aSessionId,
                                         SLong                 aRemoteStmtId,
                                         dkpBatchStatementMgr *aManager );
    static IDE_RC   recvPrepareAddBatch( dksSession *aSession,
                                         UInt        aSessionId,
                                         SLong       aRemoteStmtId,
                                         UInt       *aResultCode,
                                         ULong       aTimeoutSec );

    static IDE_RC   sendAddBatches( dksSession           *aSession,
                                    UInt                  aSessionId,
                                    dkpBatchStatementMgr *aManager );
    static IDE_RC   recvAddBatchResult( dksSession    *aSession,
                                        UInt           aSessionId,
                                        SLong          aRemoteStmtId,
                                        UInt          *aResultCode,
                                        UInt          *aProcessedBatchCount,
                                        ULong          aTimeoutSec );
                                         
    static IDE_RC   sendExecuteBatch( dksSession   *aSession,
                                      UInt          aSessionId,
                                      SLong         aRemoteStmtId );
    static IDE_RC   recvExecuteBatch( dksSession    *aSession,
                                      UInt           aSessionId,
                                      SLong          aRemoteStmtId,
                                      SInt          *aResultArray,
                                      UInt          *aResultCode,
                                      ULong          aTimeoutSec );
    
    /* Request free remote statement */
    static IDE_RC   sendRequestFreeRemoteStmt( dksSession    *aSession,
                                               UInt           aSessionId,
                                               SLong          aRemoteStmtId );
    static IDE_RC   recvRequestFreeRemoteStmtResult( dksSession    *aSession,
                                                     UInt           aSessionId,
                                                     SLong          aRemoteStmtId,
                                                     UInt          *aResultCode,
                                                     ULong          aTimeoutSec );

    static IDE_RC   sendRequestFreeRemoteStmtBatch( dksSession    *aSession,
                                                    UInt           aSessionId,
                                                    SLong          aRemoteStmtId );
    static IDE_RC   recvRequestFreeRemoteStmtBatchResult( dksSession    *aSession,
                                                          UInt           aSessionId,
                                                          SLong          aRemoteStmtId,
                                                          UInt          *aResultCode,
                                                          ULong          aTimeoutSec );

    /* Fetch */
    static IDE_RC   sendFetchResultSet( dksSession      *aSession,
                                        UInt             aSessionId,
                                        SLong            aRemoteStmtId,
                                        UInt             aFetchRowCount,
                                        UInt             aBufferSize );
    static IDE_RC   recvFetchResultRow( dksSession      *aSession,
                                        UInt             aSessionId,
                                        SLong            aRemoteStmtId,
                                        SChar           *aRecvRowBuffer,
                                        UInt             aRecvRowBufferSize,
                                        UInt            *aRecvRowLen,
                                        UInt            *aRecvRowCnt,
                                        UInt            *aResultCode,
                                        ULong            aTimeoutSec );

    /* Request result set meta info */
    static IDE_RC   sendRequestResultSetMeta( dksSession    *aSession,
                                              UInt           aSessionId,
                                              SLong          aRemoteStmtId );
    static IDE_RC   recvRequestResultSetMetaResult( dksSession    *aSession,
                                                    UInt           aSessionId,
                                                    SLong          aRemoteStmtId,
                                                    UInt          *aResultCode,
                                                    UInt          *aColumnCnt,
                                                    dkpColumn    **aColMeta,
                                                    ULong          aTimeoutSec );

    /* Bind remote variable */
    static IDE_RC   sendBindRemoteVariable( dksSession    *aSession,
                                            UInt           aSessionId,
                                            SLong          aRemoteStmtId,
                                            UInt           aBindVarIdx,
                                            UInt           aBindVarType,
                                            UInt           aBindVarLen,
                                            SChar         *aBindVar );
    static IDE_RC   recvBindRemoteVariableResult( dksSession    *aSession, 
                                                  UInt           aSessionId,
                                                  SLong          aRemoteStmtId,
                                                  UInt          *aResultCode,
                                                  ULong          aTimeoutSec );

    /* Close remote node session */
    static IDE_RC   sendRequestCloseRemoteNodeSession( dksSession    *aSession,
                                                       UInt           aSessionId,
                                                       UShort         aRemoteNodeSessionId ); 
    static IDE_RC   recvRequestCloseRemoteNodeSessionResult( dksSession    *aSession, 
                                                             UInt           aSessionId, 
                                                             UShort         aRemoteNodeSessionId, 
                                                             UInt          *aResultCode, 
                                                             UInt          *aRemainedRemoteNodeSeesionCnt, 
                                                             ULong          aTimeoutSec ); 

    /* Set auto-commit mode to remote server */
    static IDE_RC   sendSetAutoCommitMode( dksSession   *aSession,
                                           UInt          aSessionId,
                                           UChar         aAutoCommitMode );
    static IDE_RC   recvSetAutoCommitModeResult( dksSession    *aSession,
                                                 UInt           aSessionId,
                                                 UInt          *aResultCode,
                                                 ULong          aTimeoutSec );

    /* Check remote node session */
    static IDE_RC   sendCheckRemoteSession( dksSession    *aSession,
                                            UInt           aSessionId,
                                            UShort         aRemoteNodeSessionId );
    static IDE_RC   recvCheckRemoteSessionResult( dksSession         *aSession,
                                                  UInt                aSessionId,
                                                  UInt               *aResultCode,
                                                  UInt               *aRemoteNodeCnt,
                                                  dksRemoteNodeInfo **aRemoteNodeSessionInfo,
                                                  ULong               aTimeoutSec );

    /* Commit */
    static IDE_RC   sendCommit( dksSession  *aSession,
                                UInt         aSessionId );
    static IDE_RC   recvCommitResult( dksSession        *aSession,
                                      UInt               aSessionId,
                                      UInt              *aResultCode,
                                      ULong              aTimeoutSec );

    /* Rollback */
    static IDE_RC   sendRollback( dksSession    *aSession,
                                  UInt           aSessionId,
                                  SChar         *aSavepointName,
                                  UInt           aRemoteNodeCnt,
                                  UShort        *aRemoteNodeIdArr );
    static IDE_RC   recvRollbackResult( dksSession      *aSession,
                                        UInt             aSessionId,
                                        UInt            *aResultCode,
                                        ULong            aTimeoutSec );

    /* Savepoint */
    static IDE_RC   sendSavepoint( dksSession   *aSession,
                                   UInt          aSessionId,
                                   const SChar  *aSavepointName );
    static IDE_RC   recvSavepointResult( dksSession     *aSession,
                                         UInt            aSessionId,
                                         UInt           *aResultCode,
                                         ULong           aTimeoutSec );

    /* Abort */
    static IDE_RC   sendAbortRemoteStmt( dksSession     *aSession,
                                         UInt            aSessionId,
                                         SLong           aRemoteStmtId );
    static IDE_RC   recvAbortRemoteStmtResult( dksSession  *aSession,
                                               UInt         aSessionId,
                                               SLong        aRemoteStmtId,
                                               UInt        *aResultCode,
                                               ULong        aTimeoutSec );
    /* PROJ-2569 */
    static IDE_RC  sendXAPrepare( dksSession    *aSession,
                                  UInt           aSessionId,
                                  dktDtxInfo    *aDtxInfo );
    static IDE_RC  recvXAPrepareResult( dksSession    *aSession,
                                        UInt           aSessionId,
                                        UInt          *aResultCode,
                                        ULong          aTimeoutSec,
                                        UInt          *aCountRDOnlyXID,
                                        ID_XID       **aRDOnlyXIDs,
                                        UInt          *aCountFailXID,
                                        ID_XID       **aFailXIDs,
                                        SInt         **aFailErrCodes );

    static IDE_RC  sendXACommit( dksSession    *aSession,
                                 UInt           aSessionId,
                                 dktDtxInfo    *aDtxInfo );
    static IDE_RC  recvXACommitResult( dksSession    *aSession,
                                       UInt           aSessionId,
                                       UInt          *aResultCode,
                                       ULong          aTimeoutSec,
                                       UInt          *aCountFailXID,
                                       ID_XID       **aFailXIDs,
                                       SInt         **aFailErrCodes,
                                       UInt          *aCountHeuristicXID,
                                       ID_XID       **aHeuristicXIDs );

    static IDE_RC  sendXARollback( dksSession    *aSession,
                                   UInt           aSessionId,
                                   dktDtxInfo    *aDtxInfo );
    static IDE_RC  recvXARollbackResult( dksSession    *aSession,
                                         UInt           aSessionId,
                                         UInt          *aResultCode,
                                         ULong          aTimeoutSec,
                                         UInt          *aCountFailXID,
                                         ID_XID       **aFailXIDs,
                                         SInt         **aFailErrCodes,
                                         UInt          *aCountHeuristicXID,
                                         ID_XID       **aHeuristicXIDs );

    static void freeXARecvResult( ID_XID * aRDOnlyXIDs,
                                  ID_XID * aFailXIDs,
                                  ID_XID * aHeuristicXIDs,
                                  SInt   * aFailErrCodes );

    static IDE_RC  getErrorInfoFromProtocol( dksSession       *aSession,
                                             UInt              aSessionId,
                                             SLong             aRemoteStmtId,
                                             dktErrorInfo     *aErrorInfo  );
    
    static void setResultErrorCode( UInt             aResultCode,
                                    dksSession      *aSession,
                                    UInt             aSessionId,
                                    SLong            aRemoteStmtId,
                                    dktErrorInfo    *aErrorInfo );
};

#endif /* _O_DKP_PROTOCOL_MGR_H_ */
