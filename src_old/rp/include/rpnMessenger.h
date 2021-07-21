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
 

#ifndef _O_RPN_MESSENGER_H_
#define _O_RPN_MESSENGER_H_ 1

#include <idl.h>
#include <ideErrorMgr.h>

#include <rp.h>

#define RPN_STOP_MSG_NETWORK_TIMEOUT_SEC    (600)

typedef enum
{
    RPN_RELEASE_PROTOCOL_CONTEXT = 0,
    RPN_DO_NOT_RELEASE_PROTOCOL_CONTEXT = 1,
} rpnProtocolContextReleasePolicy;

class rpnMessenger
{

private:

    RP_SOCKET_TYPE     mSocketType;

    cmiLink          * mLink;
    cmiProtocolContext * mProtocolContext;
    rpdReplications  * mReplication;

    idBool           * mExitFlag;

    rprSNMapMgr      * mSNMapMgr;

    static rpValueLen  mSyncAfterMtdValueLen[QCI_MAX_COLUMN_COUNT];
    static rpValueLen  mSyncPKMtdValueLen[QCI_MAX_KEY_COLUMN_COUNT];
    idBool             mIsInitCmBlock;

    /* BUG-38716 */
    idBool             mIsBlockingMode;
    void             * mHBTResource;        /* for HBT */

    /* PROJ-2453 */
    iduMutex           mSocketMutex;
    idBool             mNeedSocketMutex;
    
    RP_MESSENGER_CHECK_VERSION  mCheckVersion;

    idBool             mIsFlushCommitXLog;

public:

    SChar              mLocalIP[IDL_IP_ADDR_MAX_LEN];
    SInt               mLocalPort;
    SChar              mRemoteIP[IDL_IP_ADDR_MAX_LEN];
    SInt               mRemotePort;

    rpIBLatency        mIBLatency;


private:
    IDE_RC connect( cmiConnectArg * aConnectArg );

    void initializeNetworkAddress( RP_SOCKET_TYPE aSocketType );
    
    IDE_RC initializeCmLink( RP_SOCKET_TYPE aSocketType );
    void   destroyCmLink( void );
    IDE_RC initializeCmBlock( void );
    void destroyCmBlock( void );

    IDE_RC checkRemoteReplVersion( rpMsgReturn * aResult,
                                   SChar       * aErrMsg,
                                   UInt        * aMsgLen );

    IDE_RC sendTrCommit( rpdTransTbl        * aTransTbl,
                         rpdSenderInfo      * aSenderInfo,
                         rpdLogAnalyzer     * aLogAnlz,
                         smSN                 aFlushSN );
    IDE_RC sendTrAbort( rpdTransTbl         * aTransTbl,
                        rpdSenderInfo       * aSenderInfo,
                        rpdLogAnalyzer      * aLogAnlz,
                        smSN                  aFlushSN );
    IDE_RC sendInsert( rpdTransTbl          * aTransTbl,
                       rpdSenderInfo        * aSenderInfo,
                       rpdLogAnalyzer       * aLogAnlz,
                       smSN                   aFlushSN );
    IDE_RC sendUpdate( rpdTransTbl          * aTransTbl,
                       rpdSenderInfo        * aSenderInfo,
                       rpdLogAnalyzer       * aLogAnlz,
                       smSN                   aFlushSN );
    IDE_RC sendDelete( rpdTransTbl          * aTransTbl,
                       rpdSenderInfo        * aSenderInfo,
                       rpdLogAnalyzer       * aLogAnlz,
                       smSN                   aFlushSN );
    IDE_RC sendSPSet( rpdTransTbl           * aTransTbl, 
                      rpdSenderInfo         * aSenderInfo,
                      rpdLogAnalyzer        * aLogAnlz,
                      smSN                    aFlushSN );
    IDE_RC sendSPAbort( rpdTransTbl         * aTransTbl,
                        rpdSenderInfo       * aSenderInfo,
                        rpdLogAnalyzer      * aLogAnlz,
                        smSN                  aFlushSN );
    IDE_RC sendLobCursorOpen( rpdTransTbl       * aTransTbl,
                              rpdSenderInfo     * aSenderInfo,
                              rpdLogAnalyzer    * aLogAnlz,
                              smSN                aFlushSN );
    IDE_RC sendLobCursorClose( rpdLogAnalyzer   * aLogAnlz,
                               rpdSenderInfo    * aSenderInfo,
                               smSN               aFlushSN );
    IDE_RC sendLobPrepareWrite( rpdLogAnalyzer  * aLogAnlz,
                                rpdSenderInfo   * aSenderInfo,
                                smSN              aFlushSN );
    IDE_RC sendLobPartialWrite( rpdLogAnalyzer  * aLogAnlz,
                                rpdSenderInfo   * aSenderInfo,
                                smSN              aFlushSN );
    IDE_RC sendLobFinishWrite( rpdLogAnalyzer   * aLogAnlz,
                               rpdSenderInfo    * aSenderInfo,
                               smSN               aFlushSN );
    IDE_RC sendLobTrim( rpdLogAnalyzer          * aLogAnlz,
                        rpdSenderInfo           * aSenderInfo,
                        smSN                      aFlushSN );    
    IDE_RC sendXaStartReq( rpdLogAnalyzer * aLogAnlz, 
                           rpdSenderInfo  * aSenderInfo );

    IDE_RC sendXaPrepareReq( rpdLogAnalyzer * aLogAnlz,
                             rpdSenderInfo  * aSenderInfo );

    IDE_RC sendXaPrepare( rpdLogAnalyzer * aLogAnlz,
                          rpdSenderInfo  * aSenderInfo );

    IDE_RC sendXaEnd( rpdLogAnalyzer * aLogAnlz, 
                      rpdSenderInfo  * aSenderInfo );

    IDE_RC sendNA( rpdLogAnalyzer * aLogAnlz );

    IDE_RC checkAndSendSvpList( rpdTransTbl     * aTransTbl,
                                rpdLogAnalyzer  * aLogAnlz,
                                smSN              aFlushSN );

    void setLastSN( rpdSenderInfo     * aSenderInfo,
                    smTID               aTransID,
                    smSN                aSN );

    void checkSupportOnRemoteVersion( rpdVersion     aRemoteVersion,
                                      rpMsgReturn  * aRC,
                                      SChar        * aRCMsg,
                                      SInt           aRCMsgLen );

public:

    rpnMessenger( void );

    IDE_RC initialize( cmiProtocolContext * aProtocolContext,
                       RP_MESSENGER_CHECK_VERSION    aCheckVersion, 
                       RP_SOCKET_TYPE       aSocketType,
                       idBool             * aExitFlag,
                       rpdReplications    * aReplication,
                       void               * aHBTResource,
                       idBool               aNeedLock );

    void destroy( rpnProtocolContextReleasePolicy aProtocolCtxReleasePolicy );

    IDE_RC connectThroughTcp( SChar * aHostIp, UInt aPortNo );
    IDE_RC connectThroughUnix( SChar * aFileName );
    IDE_RC connectThroughIB( SChar * aHostIp, UInt aPortNo, rpIBLatency aIBLatency );
    void disconnect( void );

    IDE_RC handshake( rpdMeta * aMeta );
    IDE_RC handshakeAndGetResults( rpdMeta      * aMeta,
                                   rpMsgReturn  * aRC,
                                   SChar        * aRCMsg,
                                   SInt           aRCMsgLen,
                                   idBool         aMetaInitFlag,
                                   SInt         * aFailbackStatus,
                                   smSN         * aReceiverXSN );

    void setRemoteTcpAddress( SChar * aRemoteIP, SInt aRemotePort );
    void setRemoteIBAddress( SChar * aRemoteIP, SInt aRemotePort, rpIBLatency aIBLatency );

    static void getVersionFromAck( SChar      *aMsg,
                                   UInt        aMsgLen,
                                   rpdVersion *aVersion );

    static void getCompressType( rpdVersion         aVersion, 
                                 cmiCompressType    aCompressType);

    void updateAddress( void );

    void setSnMapMgr( rprSNMapMgr * aSNMapMgr );

    void setHBTResource( void * aHBTResource );

    IDE_RC sendStop( smSN         aRestartSN,
                     idBool     * aExitFlag,
                     UInt         aTimeoutSec );
    IDE_RC sendStopAndReceiveAck( void );

    IDE_RC receiveAckDummy( void );
    IDE_RC receiveAck( rpXLogAck * aAck, 
                       idBool    * aExitFlag,
                       ULong       aTimeOut,
                       idBool    * aIsTimeOut );

    IDE_RC syncXLogInsert( smTID       aTransID,
                           SChar     * aTuple,
                           mtcColumn * aMtcCol,
                           UInt        aColCnt,
                           smOID       aTableOID );
    IDE_RC sendXLogInsert( smTID       aTransID,
                           SChar     * aTuple,
                           mtcColumn * aMtcCol,
                           UInt        aColCnt,
                           smOID       aTableOID );
    IDE_RC sendXLogLob( smTID               aTransID,
                        SChar             * aTuple,
                        scGRID              aRowGRID,
                        mtcColumn         * aMtcCol,
                        smOID               aTableOID,
                        smiTableCursor    * aCursor );

    IDE_RC sendSyncXLogTrCommit( smTID aTransID );
    IDE_RC sendSyncXLogTrAbort( smTID aTransID );

    IDE_RC sendXLog( rpdTransTbl * aTransTbl,
                     rpdSenderInfo * aSenderInfo,
                     rpdLogAnalyzer * aAnlz,
                     smSN aFlushSN,
                     idBool aNeedLock,
                     idBool aNeedFlush );

    IDE_RC sendXLogSPSet( smTID   aTID,
                          smSN    aSN,
                          smSN    aFulshSN,
                          UInt    aSpNameLen,
                          SChar * aSpName,
                          idBool  aNeedLock );
    IDE_RC sendXLogKeepAlive( smSN    aSN,
                              smSN    aFlushN,
                              smSN    aRestartSN,
                              idBool  aNeedLock );
    IDE_RC sendXLogHandshake( smTID   aTID,
                              smSN    aSN,
                              smSN    aFlushSN,
                              idBool  aNeedLock );
    IDE_RC sendXLogSyncPKBegin( void );
    IDE_RC sendXLogSyncPK( ULong        aTableOID,
                           UInt         aPKColCnt,
                           smiValue   * aPKCols,
                           rpValueLen * aPKLen );
    IDE_RC sendXLogSyncPKEnd( void );
    IDE_RC sendXLogFailbackEnd( void );
    IDE_RC sendXLogDelete( ULong        aTableOID,
                           UInt         aPKColCnt,
                           smiValue   * aPKCols,
                           rpValueLen * aPKLen );
    IDE_RC sendXLogTrCommit( void );
    IDE_RC sendXLogTrAbort( void );

    IDE_RC sendSyncTableInfo( rpdMetaItem * aItem );
    IDE_RC sendSyncTableNumber( UInt aSyncTableNumber );
    IDE_RC sendSyncStart( void );
    IDE_RC sendSyncEnd( void );
    IDE_RC sendFlush( void );
    /* PROJ-2453 */
    
    static void setCompressType( rpdVersion         aRemoteVersion,
                                 cmiCompressType    aCompressType );
public:
    IDE_RC sendAckOnDML( void );
    IDE_RC sendCmBlock( void );
    
    ULong getSendDataSize( void );
    ULong getSendDataCount( void );

    /* BUG-46252 Partition Merge / Split / Replace DDL asynchronization support */
    IDE_RC sendDDLASyncStart( UInt aType );

    IDE_RC recvDDLASyncStartAck( UInt * aType );

    IDE_RC sendDDLASyncExecute( UInt    aType,
                                SChar * aUserName,
                                UInt    aDDLEnableLevel,
                                UInt    aTargetCount,
                                SChar * aTargetTableName,
                                SChar * aTargetPartNames,
                                smSN    aDDLCommitSN,
                                SChar * aDDLStmt );

    IDE_RC recvDDLASyncExecuteAck( UInt  * aType,
                                   UInt  * aIsSuccess, 
                                   UInt  * aErrCode,
                                   SChar * aErrMsg );

    IDE_RC communicateConditionInfo( rpdConditionItemInfo  * aSendConditionInfo,
                                     SInt                    aItemCount,
                                     rpdConditionActInfo   * aRecvConditionActInfo );

    IDE_RC sendTruncate( ULong aTableOID );
};

#endif /* _O_RPN_MESSENGER_H_ */
