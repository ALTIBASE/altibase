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
 * $Id: rpnComm.h 16598 2006-06-09 01:39:24Z lswhh $
 **********************************************************************/

#ifndef _O_RPN_COMM_H_
#define _O_RPN_COMM_H_ 1

#include <idl.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <smrDef.h>
#include <qci.h>
#include <rp.h>
#include <rpdLogAnalyzer.h>
#include <rpdMeta.h>
#include <rpdTransTbl.h>
#include <rpdQueue.h>
#include <rpxReceiver.h>
#include <rpuProperty.h>


class rpnComm
{
// Operation
public:
    rpnComm();
    virtual ~rpnComm() {};

    static IDE_RC initialize();
    static void   destroy();
    static void   shutdown();
    static void   finalize();
    static idBool isConnected( cmiLink * aCmiLink );

    static void setLengthForEachXLogType();

    static IDE_RC readCmBlock( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               idBool             * aIsTimeout,
                               ULong                aTimeout );
    
    /* PROJ-2677 */
    static IDE_RC recvOperationInfo( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UChar              * aOpCode,
                                     ULong                aTimeoutSec );

    static void   destroyProtocolContext( cmiProtocolContext * aProtocolContext );

    static IDE_RC sendVersion( void                 * aHBTResource,
                               cmiProtocolContext   * aProtocolContext,
                               idBool               * aExitFlag,
                               rpdVersion           * aReplVersion,
                               UInt                   aTimeoutSec );
    static IDE_RC recvVersion( cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               rpdVersion *aReplVersion,
                               ULong       aTimeOut);
    static IDE_RC sendMetaRepl( void                    * aHBTResource,
                                cmiProtocolContext      * aProtocolContext,
                                idBool                  * aExitFlag,
                                rpdReplications         * aRepl,
                                UInt                      aTimeoutSec );
    static IDE_RC recvMetaRepl( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpdReplications    * aRepl,
                                ULong                aTimeOut);
 
    static IDE_RC sendTempSyncMetaRepl( void                    * aHBTResource,
                                        cmiProtocolContext      * aProtocolContext,
                                        idBool                  * aExitFlag,
                                        rpdReplications         * aRepl,
                                        UInt                      aTimeoutSec );
    static IDE_RC recvTempSyncMetaRepl( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        rpdReplications    * aRepl,
                                        ULong                aTimeOut);
   
    static IDE_RC sendTempSyncReplItem( void                    * aHBTResource,
                                        cmiProtocolContext      * aProtocolContext,
                                        idBool                  * aExitFlag,
                                        rpdReplSyncItem         * aTempSyncItem,
                                        UInt                      aTimeoutSec );
    static IDE_RC recvTempSyncReplItem( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        rpdReplSyncItem    * aTempSyncItem,
                                        ULong                aTimeOut);

    static IDE_RC sendMetaReplTbl( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdMetaItem        * aItem,
                                   UInt                 aTimeoutSec );
    static IDE_RC recvMetaReplTbl( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdMetaItem        * aItem,
                                   ULong                aTimeOut);
    /* BUG-46120 */
    static IDE_RC sendMetaPartitionCount( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aCount,
                                          UInt                 aTimeoutSec );
    static IDE_RC sendMetaPartitionCountA7( void               * aHBTResource,
                                            cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            UInt               * aCount,
                                            UInt                 aTimeoutSec );
    static IDE_RC recvMetaPartitionCount( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aCount,
                                          ULong                aTimeoutSec );
    static IDE_RC recvMetaPartitionCountA7( cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            UInt               * aCount,
                                            ULong                aTimeoutSec );

    static IDE_RC sendMetaReplCol( void                 * aHBTResource,
                                   cmiProtocolContext   * aProtocolContext,
                                   idBool               * aExitFlag,
                                   rpdColumn            * aColumn,
                                   rpdVersion             aRemoteVersion,
                                   UInt                   aTimeoutSec );
    static IDE_RC recvMetaReplCol( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdColumn          * aColumn,
                                   rpdVersion           aRemoteVersion,
                                   ULong                aTimeOut );
    static IDE_RC sendMetaReplIdx( void                 * aHBTResource,
                                   cmiProtocolContext   * aProtocolContext,
                                   idBool               * aExitFlag,
                                   qcmIndex             * aIndex,
                                   UInt                   aTimeoutSec );
    static IDE_RC recvMetaReplIdx( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   qcmIndex     *aIndex,
                                   ULong         aTimeOut);
    static IDE_RC sendMetaReplIdxCol( void                  * aHBTResource,
                                      cmiProtocolContext    * aProtocolContext,
                                      idBool                * aExitFlag,
                                      UInt                    aColumnID,
                                      UInt                    aKeyColumnFlag,
                                      UInt                    aTimeoutSec );
    static IDE_RC recvMetaReplIdxCol( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt               * aColumnID,
                                      UInt               * aKeyColumnFlag,
                                      ULong                aTimeOut);

    /* BUG-34360 Check Constraint */
    static IDE_RC sendMetaReplCheck( void                   * aHBTResource,
                                     cmiProtocolContext     * aProtocolContext,
                                     idBool                 * aExitFlag,
                                     qcmCheck               * aCheck,
                                     UInt                     aTimeoutSec );

    /* BUG-34360 Check Constraint */
    static IDE_RC recvMetaReplCheck( cmiProtocolContext   * aProtocolContext,
                                     idBool               * aExitFlag,
                                     qcmCheck             * aCheck,
                                     const ULong            aTimeoutSec );

    /* BUG-34360 Check Constraint */
    static IDE_RC recvMetaReplCheckA7( cmiProtocolContext   * aProtocolContext,
                                       idBool               * aExitFlag,
                                       qcmCheck             * aCheck,
                                       const ULong            aTimeoutSec );
    /* BUG-38759 */
    static IDE_RC sendMetaDictTableCount( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          SInt               * aDictTableCount,
                                          UInt                 aTimeoutSec );

    static IDE_RC sendMetaDictTableCountA7( void               * aHBTResource,
                                            cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            SInt               * aDictTableCount,
                                            UInt                 aTimeoutSec );
    /* BUG-38759 */
    static IDE_RC recvMetaDictTableCount( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          SInt               * aDictTableCount,
                                          ULong                aTimeoutSec );
    /* BUG-38759 */
    static IDE_RC recvMetaDictTableCountA7( cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            SInt               * aDictTableCount,
                                            ULong                aTimeoutSec );

    static IDE_RC sendHandshakeAck( cmiProtocolContext      * aProtocolContext,
                                    idBool                  * aExitFlag,
                                    UInt                      aResult,
                                    SInt                      aFailbackStatus,
                                    ULong                     aXSN,
                                    const SChar             * aMsg,
                                    UInt                      aTimeoutSec );
    static IDE_RC recvHandshakeAck( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool          *aExitFlag,
                                    UInt            *aResult,
                                    SInt            *aFailbackStatus,
                                    ULong           *aXSN,
                                    SChar           *aMsg,
                                    UInt            *aMsgLen,
                                    ULong            aTimeOut);

    static IDE_RC sendTempSyncHandshakeAck( cmiProtocolContext    * aProtocolContext,
                                            idBool                * aExitFlag,
                                            UInt                    aResult,
                                            SChar                 * aMsg,
                                            UInt                    aTimeoutSec );
    static IDE_RC recvTempSyncHandshakeAck( cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            UInt               * aResult,
                                            SChar              * aMsg,
                                            UInt               * aMsgLen,
                                            ULong                aTimeOut );

    static IDE_RC recvXLog( iduMemAllocator   * aAllocator,
                            rpxReceiverReadContext aReadContext,
                            idBool             *aExitFlag,
                            rpdMeta            *aMeta,  // BUG-20506
                            rpdXLog            *aXLog,
                            ULong               aTimeOutSec);
    static IDE_RC sendTrBegin( void                 * aHBTResource,
                               cmiProtocolContext   * aProtocolContext,
                               idBool               * aExitFlag,
                               smTID                  aTID,
                               smSN                   aSN,
                               smSN                   aSyncSN,
                               UInt                   aTimeoutSec );
    static IDE_RC sendTrCommit( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                idBool               aForceFlush,
                                UInt                 aTimeoutSec );
    static IDE_RC sendTrAbort( void                 * aHBTResource,
                               cmiProtocolContext   * aProtocolContext,
                               idBool               * aExitFlag,
                               smTID                  aTID,
                               smSN                   aSN,
                               smSN                   aSyncSN,
                               UInt                   aTimeoutSec );
    static IDE_RC sendSPSet( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aSPNameLen,
                             SChar              * aSPName,
                             UInt                 aTimeoutSec );
    static IDE_RC sendSPAbort( void                 * aHBTResource,
                               cmiProtocolContext   * aProtocolContext,
                               idBool               * aExitFlag,
                               smTID                  aTID,
                               smSN                   aSN,
                               smSN                   aSyncSN,
                               UInt                   aSPNameLen,
                               SChar                * aSPName,
                               UInt                   aTimeoutSec );
    static IDE_RC sendStmtBegin( void               * aHBTResource,
                                 cmiProtocolContext * aCmiProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdLogAnalyzer     * aLA,
                                 UInt                 aTimeoutSec );
    static IDE_RC sendStmtEnd( void                 * aHBTResource,
                               cmiProtocolContext   * aCmiProtocolContext,
                               idBool               * aExitFlag,
                               rpdLogAnalyzer       * aLA,
                               UInt                   aTimeoutSec );
    static IDE_RC sendCursorOpen( void                  * aHBTResource,
                                  cmiProtocolContext    * aCmiProtocolContext,
                                  idBool                * aExitFlag,
                                  rpdLogAnalyzer        * aLA,
                                  UInt                    aTimeoutSec );
    static IDE_RC sendCursorClose( void                 * aHBTResource,
                                   cmiProtocolContext   * aCmiProtocolContext,
                                   idBool               * aExitFlag,
                                   rpdLogAnalyzer       * aLA,
                                   UInt                   aTimeoutSec );
    static IDE_RC sendInsert( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              UInt                 aImplSPDepth,
                              ULong                aTableOID,
                              UInt                 aColCnt,
                              smiValue           * aACols,
                              rpValueLen         * aAMtdValueLen,
                              UInt                 aTimeoutSec );
    static IDE_RC sendUpdate( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              UInt                 aImplSPDepth,
                              ULong                aTableOID,
                              UInt                 aPKColCnt,
                              UInt                 aUpdateColCnt,
                              smiValue           * aPKCols,
                              UInt               * aCIDs,
                              smiValue           * aBCols,
                              smiChainedValue    * aBChainedCols, // BUG-1705
                              UInt               * aBChainedColsTotalLen, /* BUG-33722 */
                              smiValue           * aACols,
                              rpValueLen         * aPKMtdValueLen,
                              rpValueLen         * aAMtdValueLen,
                              rpValueLen         * aBMtdValueLen,
                              UInt                 aTimeoutSec );
    static IDE_RC sendDelete( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              UInt                aImplSPDepth,
                              ULong                aTableOID,
                              UInt                 aPKColCnt,
                              smiValue           * aPKCols,
                              rpValueLen         * aPKMtdValueLen,
                              UInt                 aColCnt,
                              UInt               * aCIDs,
                              smiValue           * aBCols,
                              smiChainedValue    * aBChainedCols, // PROJ-1705
                              rpValueLen         * aBLen,
                              UInt               * aBChainedColsTotalLen,
                              UInt                 aTimeoutSec ); /* BUG-33722 */

    static IDE_RC sendChainedValueForA7( void               * aHBTResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         smiChainedValue    * aChainedValue,
                                         rpValueLen           aBMtdValueLen,
                                         UInt                 aChainedValueLen,
                                         UInt                 aTimeoutSec );
    static IDE_RC sendStop( void                * aHBTResource,
                            cmiProtocolContext  * aProtocolContext,
                            idBool              * aExitFlag,
                            smTID                 aTID,
                            smSN                  aSN,
                            smSN                  aSyncSN,
                            smSN                  aRestartSN,
                            UInt                  aTimeoutSec );         // BUG-17748
    static IDE_RC sendKeepAlive( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 smSN                 aRestartSN,
                                 UInt                 aTimeoutSec );    // BUG-17748
    static IDE_RC sendFlush( void                   * aHBTResource,
                             cmiProtocolContext     * aProtocolContext,
                             idBool                 * aExitFlag,
                             smTID                    aTID,
                             smSN                     aSN,
                             smSN                     aSyncSN,
                             UInt                     aFlushOption,
                             UInt                     aTimeoutSec );

    static IDE_RC sendAck(cmiProtocolContext * aProtocolContext,
                          idBool             * aExitFlag,
                          rpXLogAck          * aAck,
                          UInt                 aTimeoutSec );

    static IDE_RC recvAck(iduMemAllocator    * aAllocator,
                          void               * aHBTResource,
                          cmiProtocolContext * aProtocolContext,
                          idBool         *aExitFlag,
                          rpXLogAck      *aAck,
                          ULong           aTimeOut,
                          idBool         *aIsTimeOut);
    static IDE_RC sendLobCursorOpen( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     ULong                aTableOID,
                                     ULong                aLobLocator,
                                     UInt                 aLobColumnID,
                                     UInt                 aPKColCnt,
                                     smiValue           * aPKCols,
                                     rpValueLen         * aPKMtdValueLen,
                                     UInt                 aTimeoutSec );
    static IDE_RC sendLobCursorClose( void                  * aHBTResource,
                                      cmiProtocolContext    * aProtocolContext,
                                      idBool                * aExitFlag,
                                      smTID                   aTID,
                                      smSN                    aSN,
                                      smSN                    aSyncSN,
                                      ULong                   aLobLocator,
                                      UInt                    aTimeoutSec );
    static IDE_RC sendLobPrepare4Write( void                * aHBTResource,
                                        cmiProtocolContext  * aProtocolContext,
                                        idBool              * aExitFlag,
                                        smTID                 aTID,
                                        smSN                  aSN,
                                        smSN                  aSyncSN,
                                        ULong                 aLobLocator,
                                        UInt                  aLobOffset,
                                        UInt                  aLobOldSize,
                                        UInt                  aLobNewSize,
                                        UInt                  aTimeoutSec );

    static IDE_RC sendLobTrim( void                * aHBTResource,
                               cmiProtocolContext  * aProtocolContext,
                               idBool              * aExitFlag,
                               smTID                 aTID,
                               smSN                  aSN,
                               smSN                  aSyncSN,
                               ULong                 aLobLocator,
                               UInt                  aLobOffset,
                               UInt                  aTimeoutSec );

    static IDE_RC recvLobTrim( iduMemAllocator     * /*aAllocator*/,
                               idBool              * /* aExitFlag */,
                               rpxReceiverReadContext aReadContext,
                               rpdXLog             * aXLog,
                               ULong                /*aTimeOutSec*/ );
    
    static IDE_RC sendLobPartialWrite( void                * aHBTResource,
                                       cmiProtocolContext  * aProtocolContext,
                                       idBool              * aExitFlag,
                                       smTID                 aTID,
                                       smSN                  aSN,
                                       smSN                  aSyncSN,
                                       ULong                 aLobLocator,
                                       UInt                  aLobOffset,
                                       UInt                  aLobPieceLen,
                                       SChar               * aLobPiece,
                                       UInt                  aTimeoutSec );
    static IDE_RC sendLobFinish2Write( void                * aHBTResource,
                                       cmiProtocolContext  * aProtocolContext,
                                       idBool              * aExitFlag,
                                       smTID                 aTID,
                                       smSN                  aSN,
                                       smSN                  aSyncSN,
                                       ULong                 aLobLocator,
                                       UInt                  aTimeoutSec );

    static IDE_RC sendHandshake( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKBegin( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendSyncPK( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              ULong                aTableOID,
                              UInt                 aPKColCnt,
                              smiValue           * aPKCols,
                              rpValueLen         * aPKMtdValueLen,
                              UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKEnd( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendFailbackEnd( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec );

    /* PROJ-2184 RP Sync performance improvement */
    static IDE_RC sendSyncStart( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 UInt                 aTimeoutSec );

    static IDE_RC recvSyncStart( iduMemAllocator     * /*aAllocator*/,
                                 idBool              * /*aExitFlag*/,
                                 cmiProtocolContext  * /*aProtocolContext*/,
                                 rpdXLog             * aXLog,
                                 ULong                /*aTimeOutSec*/ );

    static IDE_RC sendSyncTableNumber( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       UInt                 aSyncTableNumber,
                                       UInt                 aTimeoutSec );
    static IDE_RC recvSyncTableNumber( cmiProtocolContext * aProtocolContext,
                                       UInt               * aSyncTableNumber,
                                       ULong                aTimeOut );
    static IDE_RC sendSyncInsert( void                  * aHBTResource,
                                  cmiProtocolContext    * aProtocolContext,
                                  idBool                * aExitFlag,
                                  smTID                   aTID,
                                  smSN                    aSN,
                                  smSN                    aSyncSN,
                                  UInt                    aImplSPDepth,
                                  ULong                   aTableOID,
                                  UInt                    aColCnt,
                                  smiValue              * aACols,
                                  rpValueLen            * aAMtdValueLen,
                                  UInt                    aTimeoutSec );
    static IDE_RC sendSyncEnd( void                * aHBTResource,
                               cmiProtocolContext  * aProtocolContext,
                               idBool              * aExitFlag,
                               UInt                  aTimeoutSec );
    static IDE_RC recvSyncEnd( iduMemAllocator     * /*aAllocator*/,
                               idBool              * /*aExitFlag*/,
                               cmiProtocolContext  * /*aProtocolContext*/,
                               rpdXLog             * aXLog,
                               ULong                 /*aTimeOutSec*/ );

    static IDE_RC sendMetaInitFlag( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    idBool               aMetaInitFlag,
                                    UInt                 aTimeoutSec );
    static IDE_RC recvMetaInitFlag( cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    idBool             * aMetaInitFlag,
                                    ULong                aTimeoutSec );

    static IDE_RC sendConditionInfo( void                 * aHBTResource,
                                     cmiProtocolContext   * aProtocolContext,
                                     idBool               * aExitFlag,
                                     rpdConditionItemInfo * aConditionInfo,
                                     UInt                   aConditionCnt,
                                     UInt                   aTimeoutSec );

    static IDE_RC recvConditionInfo( cmiProtocolContext    * aProtocolContext,
                                     idBool                * aExitFlag,
                                     rpdConditionItemInfo  * aConditionInfo,
                                     UInt                  * aConditionCnt,
                                     UInt                    aTimeoutSec );

    static IDE_RC sendConditionInfoResult( cmiProtocolContext * aProtocolContext,
                                           idBool             * aExitFlag,
                                         rpdConditionActInfo  * aConditionInfo,
                                           UInt                 aConditionCnt,
                                           UInt                 aTimeoutSec );

    static IDE_RC recvConditionInfoResult( cmiProtocolContext * aProtocolContext,
                                           idBool             * aExitFlag,
                                         rpdConditionActInfo  * aConditionInfo,
                                           UInt                 aTimeoutSec );

    static IDE_RC sendTruncate( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                ULong                aTableOID,
                                UInt                 aTimeoutSec );

    static IDE_RC recvTruncate( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpdMeta            * aMeta,
                                rpdXLog            * aXLog );
         
    static IDE_RC sendDDLSyncInfo( void                * aHBTResource,
                                   cmiProtocolContext  * aProtocolContext,
                                   idBool              * aExitFlag,
                                   UInt                  aInfoMsgType,
                                   SChar               * aRepName,
                                   SChar               * aUserName,
                                   SChar               * aTableName,
                                   UInt                  aDDLSyncPartInfoCount,
                                   SChar                 aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                   UInt                  aDDLEnableLevel,
                                   UInt                  aSQLMsgType,
                                   SChar               * aSql,
                                   UInt                  aSqlLen,
                                   ULong                 aTimeout );
    static IDE_RC recvDDLSyncInfo( cmiProtocolContext  * aProtocolContext,
                                   idBool              * aExitFlag,
                                   UInt                  aInfoMsgType,
                                   SChar               * aRepName,
                                   SChar               * aUserName,
                                   SChar               * aTableName,
                                   UInt                  aDDLSyncMaxPartInfoCount,
                                   UInt                * aDDLSyncPartInfoCount,
                                   UInt                  aDDLSyncMaxPartNameLen,
                                   SChar                 aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                   UInt                * aDDLEnableLevel,
                                   UInt                  aSQLMsgType,
                                   SChar              ** aSql, 
                                   ULong                 aRecvTimeout );

    static IDE_RC sendDDLSyncSQL(  void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,                                   
                                   UInt                 aMsgType,
                                   UInt                 aSqlLen,
                                   SChar              * aSql,
                                   ULong                aTimeout );

    static IDE_RC recvDDLSyncSQL( cmiProtocolContext  * aProtocolContext,
                                  idBool              * aExitFlag,
                                  UInt                  aMsgType,
                                  SChar              ** aSql,
                                  ULong                 aRecvTimeout );

    static IDE_RC sendDDLSyncInfoAck( void               * aHBTResource,
                                      cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt                 aMsgType,
                                      UInt                 aResult,
                                      SChar              * aErrMsg,
                                      ULong                aTimeout );
    static IDE_RC recvDDLSyncInfoAck( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt                 aMsgType,
                                      UInt               * aResult,
                                      SChar              * aErrMsg,
                                      ULong                aRecvTimeout );

    static IDE_RC sendDDLSyncMsg( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  UInt                 aMsgType,
                                  ULong                aTimeout );
    static IDE_RC recvDDLSyncMsg( cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  UInt                 aMsgType,
                                  ULong                aRecvTimeout );

    static IDE_RC sendDDLSyncMsgAck( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UInt                 aMsgType,
                                     UInt                 aResult,
                                     SChar              * aErrMsg,
                                     ULong                aTimeout );
    static IDE_RC recvDDLSyncMsgAck( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UInt                 aMsgType,
                                     UInt               * aResult,
                                     SChar              * aErrMsg,
                                     ULong                aRecvTimeout );

    static IDE_RC sendDDLSyncCancel( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     SChar              * aRepName,
                                     ULong                aTimeout );
    static IDE_RC recvDDLSyncCancel( cmiProtocolContext * aProtocolContext, 
                                     idBool             * aExitFlag,
                                     SChar              * aRepName,
                                     ULong                aRecvTimeout );

    static IDE_RC sendDDLASyncStart( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UInt                 aType,
                                     ULong                aSendTimeout );

    static IDE_RC sendDDLASyncStartAck( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt                 aType,
                                        ULong                aSendTimeout );

    static IDE_RC recvDDLASyncStartAck( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aType,
                                        ULong                aRecvTimeout );

    static IDE_RC sendDDLASyncExecute( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       UInt                 aType,
                                       SChar              * aUserName,
                                       UInt                 aDDLEnableLevel,
                                       UInt                 aTargetCount,
                                       SChar              * aTargetTableName,
                                       SChar              * aTargetPartNames,
                                       smSN                 aDDLCommitSN,
                                       rpdVersion         * aReplVersion,
                                       SChar              * aDDLStmt,
                                       ULong                aSendTimeout );

    static  IDE_RC recvDDLASyncExecute( cmiProtocolContext  * aProtocolContext,
                                        idBool              * aExitFlag,
                                        UInt                * aType,
                                        SChar               * aUserName,
                                        UInt                * aDDLEnableLevel,
                                        UInt                * aTargetCount,
                                        SChar               * aTargetTableName,
                                        SChar              ** aTargetPartNames,
                                        smSN                * aDDLCommitSN,
                                        rpdVersion          * aReplVersion,
                                        SChar              ** aDDLStmt,
                                        ULong                 aRecvTimeout );

    static IDE_RC sendDDLASyncExecuteAck( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt                 aType,
                                          UInt                 aIsSuccess,
                                          UInt                 aErrCode,
                                          SChar              * aErrMsg,
                                          ULong                aSendTimeout );

    static IDE_RC recvDDLASyncExecuteAck( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aType,
                                          UInt               * aIsSuccess,
                                          UInt               * aErrCode,
                                          SChar              * aErrMsg,
                                          ULong                aRecvTimeout );

    static IDE_RC sendQueryString(  void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    UInt                 aType,
                                    UInt                 aSqlLen,
                                    SChar              * aSql,
                                    ULong                aTimeout );

    static IDE_RC recvQueryString( cmiProtocolContext  * aProtocolContext,
                                   idBool              * aExitFlag,
                                   UInt                * aType,
                                   SChar              ** aSql,
                                   ULong                 aRecvTimeout );

    static IDE_RC sendXaStartReq( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  ID_XID             * aXID,
                                  smTID                aTID,
                                  smSN                 aSN,
                                  ULong                aSendTimeout );


    static IDE_RC sendXaPrepareReq( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    ID_XID             * aXID,
                                    smTID                aTID,
                                    smSN                 aSN,
                                    ULong                aSendTimeout );

    static IDE_RC sendXaPrepare( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 ID_XID             * aXID,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 ULong                aSendTimeout );

    static IDE_RC sendXaCommit( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                smSCN                aGlobalCommitSCN,
                                idBool               aForceFlush,
                                UInt                 aTimeoutSec );

    static IDE_RC sendXaEnd( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             ULong                aSendTimeout );

    static IDE_RC recvXLogA7( iduMemAllocator    * aAllocator,
                              rpxReceiverReadContext aProtocolContext,
                              idBool             * aExitFlag,
                              rpdMeta            * aMeta,
                              rpdXLog            * aXLog,
                              ULong                aTimeoutSec );
private:
    static IDE_RC allocNullValue(iduMemAllocator * aAllocator,
                                 iduMemory       * aMemory,
                                 smiValue        * aValue,
                                 const mtcColumn * aColumn,
                                 const mtdModule * aModule);

    static IDE_RC readCmProtocol( cmiProtocolContext * aProtocolContext,
                                  cmiProtocol        * aProtocol,
                                  idBool             * aExitFlag,
                                  idBool             * aIsTimeOut,
                                  ULong                aTimeoutSec );

    static IDE_RC recvVersionA5( cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdVersion         * aReplVersion,
                                 ULong                aTimeOut );
    static IDE_RC recvVersionA7( cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdVersion         * aReplVersion );

    static IDE_RC recvOperationInfoA7( cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       UChar              * aOpCode,
                                       ULong                aTimeoutSec );

    static IDE_RC sendHandshakeAckA5( cmiProtocolContext    * aProtocolContext,
                                      idBool                * aExitFlag,
                                      UInt                    aResult,
                                      SInt                    aFailbackStatus,
                                      ULong                   aXSN,
                                      const SChar           * aMsg,
                                      UInt                    aTimeoutSec );

    static IDE_RC recvHandshakeAckA5( cmiProtocolContext * aProtocolContext,
                                      idBool          *aExitFlag,
                                      UInt            *aResult,
                                      SInt            *aFailbackStatus,
                                      ULong           *aXSN,
                                      SChar           *aMsg,
                                      UInt            *aMsgLen,
                                      ULong            aTimeOut);

    static IDE_RC sendHandshakeAckA7( cmiProtocolContext    * aProtocolContext,
                                      idBool                * aExitFlag,
                                      UInt                    aResult,
                                      SInt                    aFailbackStatus,
                                      ULong                   aXSN,
                                      const SChar           * aMsg,
                                      UInt                    aTimeoutSec );

    static IDE_RC recvHandshakeAckA7( void               * aHBTResource,
                                      cmiProtocolContext * aProtocolContext,
                                      idBool          *aExitFlag,
                                      UInt            *aResult,
                                      SInt            *aFailbackStatus,
                                      ULong           *aXSN,
                                      SChar           *aMsg,
                                      UInt            *aMsgLen,
                                      ULong            aTimeOut);

    static IDE_RC sendTempSyncHandshakeAckA7( cmiProtocolContext    * aProtocolContext,
                                              idBool                * aExitFlag,
                                              UInt                    aResult,
                                              SChar                 * aMsg,
                                              UInt                    aTimeoutSec );
    static IDE_RC recvTempSyncHandshakeAckA7( cmiProtocolContext * aProtocolContext,
                                              idBool             * aExitFlag,
                                              UInt               * aResult,
                                              SChar              * aMsg,
                                              UInt               * aMsgLen,
                                              ULong                aTimeOut );

    static IDE_RC recvMetaReplA5( cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  rpdReplications    * aRepl,
                                  ULong                aTimeoutSec );
    static IDE_RC recvMetaReplA7( cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  rpdReplications    * aRepl,
                                  ULong                aTimeoutSec );
 
    static IDE_RC recvTempSyncMetaReplA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          rpdReplications    * aRepl,
                                          ULong                aTimeoutSec );
 
    static IDE_RC recvTempSyncReplItemA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          rpdReplSyncItem    * aTempSyncItem,
                                          ULong                aTimeoutSec );


    static IDE_RC recvMetaReplTblA5( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdMetaItem        * aItem,
                                     ULong                aTimeoutSec );
    static IDE_RC recvMetaReplTblA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdMetaItem        * aItem,
                                     ULong                aTimeoutSec );

    static IDE_RC recvMetaReplColA5( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdColumn          * aColumn,
                                     ULong                aTimeoutSec );
    static IDE_RC recvMetaReplColA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdColumn          * aColumn,
                                     rpdVersion           aRemoteVersion,                                
                                     ULong                aTimeoutSec );

    static IDE_RC recvMetaReplIdxA5( cmiProtocolContext * aProtocolContext,
                                     idBool             * /*aExitFlag*/,
                                     qcmIndex           * aIndex,
                                     ULong                aTimeoutSec );
    static IDE_RC recvMetaReplIdxA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     qcmIndex           * aIndex,
                                     ULong                aTimeoutSec );

    static IDE_RC recvMetaReplIdxColA5( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aColumnID,
                                        UInt               * aKeyColumnFlag,
                                        ULong                aTimeoutSec );
    static IDE_RC recvMetaReplIdxColA7( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aColumnID,
                                        UInt               * aKeyColumnFlag,
                                        ULong                aTimeoutSec );

    static IDE_RC recvXLogA5( iduMemAllocator    * aAllocator,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              rpdMeta            * aMeta,
                              rpdXLog            * aXLog,
                              ULong                aTimeoutSec );

    static IDE_RC recvTrBeginA5( cmiProtocol         * aProtocol,
                                 rpdXLog             * aXLog );
    static IDE_RC recvTrBeginA7( rpxReceiverReadContext aReadContext,
                                 idBool              * aExitFlag,
                                 rpdXLog             * aXLog );

    static IDE_RC recvTrCommitA5( cmiProtocol        * aProtocol,
                                  rpdXLog            * aXLog );
    static IDE_RC recvTrCommitA7( rpxReceiverReadContext aReadContext,
                                  idBool             * aExitFlag,
                                  rpdXLog            * aXLog );

    static IDE_RC recvTrAbortA5( cmiProtocol         * aProtocol,
                                 rpdXLog             * aXLog );
    static IDE_RC recvTrAbortA7( rpxReceiverReadContext aReadContext,
                                 idBool              * aExitFlag,
                                 rpdXLog             * aXLog );
    
    static IDE_RC recvSPSetA5( cmiProtocol         * aProtocol,
                               rpdXLog             * aXLog );
    static IDE_RC recvSPSetA7( rpxReceiverReadContext aReadContext,
                               idBool              * aExitFlag,
                               rpdXLog             * aXLog );

    static IDE_RC recvSPAbortA5( cmiProtocol         * aProtocol,
                                 rpdXLog             * aXLog );
    static IDE_RC recvSPAbortA7( rpxReceiverReadContext aReadContext,
                                 idBool              * aExitFlag,
                                 rpdXLog             * aXLog );

    static IDE_RC recvInsertA5( iduMemAllocator    * aAllocator,
                                idBool             * aExitFlag,
                                cmiProtocolContext * aProtocolContext,
                                cmiProtocol        * aProtocol,
                                rpdMeta            * aMeta,
                                rpdXLog            * aXLog,
                                ULong                aTimeOutSec );
    static IDE_RC recvInsertA7( iduMemAllocator    * aAllocator,
                                idBool             * aExitFlag,
                                rpxReceiverReadContext aReadContext,
                                rpdMeta            * aMeta,   // BUG-20506
                                rpdXLog            * aXLog,
                                ULong                aTimeOutSec );

    static IDE_RC recvUpdateA5( iduMemAllocator     * aAllocator,
                                idBool              * aExitFlag,
                                cmiProtocolContext  * aProtocolContext,
                                cmiProtocol         * aProtocol,
                                rpdMeta             * aMeta,
                                rpdXLog             * aXLog,
                                ULong                 aTimeOutSec );
    static IDE_RC recvUpdateA7( iduMemAllocator     * aAllocator,
                                idBool              * aExitFlag,
                                rpxReceiverReadContext aReadContext,
                                rpdMeta             * aMeta,   // BUG-20506
                                rpdXLog             * aXLog,
                                ULong                 aTimeOutSec );

    static IDE_RC recvDeleteA5( iduMemAllocator     * aAllocator,
                                idBool              * aExitFlag,
                                cmiProtocolContext  * aProtocolContext,
                                cmiProtocol         * aProtocol,
                                rpdXLog             * aXLog,
                                ULong                 aTimeOutSec );
    static IDE_RC recvDeleteA7( iduMemAllocator     * aAllocator,
                                idBool              * aExitFlag,
                                rpxReceiverReadContext aReadContext,
                                rpdMeta             * aMeta,   // BUG-20506
                                rpdXLog             * aXLog,
                                ULong                 aTimeOutSec );

    static IDE_RC recvStopA5( cmiProtocol        * aProtocol,
                              rpdXLog            * aXLog );
    static IDE_RC recvStopA7( rpxReceiverReadContext aReadContext,
                              idBool              * aExitFlag,
                              rpdXLog            * aXLog );
    
    static IDE_RC recvKeepAliveA5( cmiProtocol        * aProtocol,
                                   rpdXLog            * aXLog );
    static IDE_RC recvKeepAliveA7( rpxReceiverReadContext aReadContext,
                                   idBool             * aExitFlag,
                                   rpdXLog            * aXLog );
    
    static IDE_RC recvFlushA5( cmiProtocol         * aProtocol,
                               rpdXLog             * aXLog );
    static IDE_RC recvFlushA7( rpxReceiverReadContext aReadContext,
                               idBool              * aExitFlag,
                               rpdXLog             * aXLog );

    static IDE_RC recvLobCursorOpenA5( iduMemAllocator     * aAllocator,
                                       idBool              * aExitFlag,
                                       cmiProtocolContext  * aProtocolContext,
                                       cmiProtocol         * aProtocol,
                                       rpdMeta             * aMeta,
                                       rpdXLog             * aXLog,
                                       ULong                 aTimeOutSec );
    static IDE_RC recvLobCursorOpenA7( iduMemAllocator     * aAllocator,
                                       idBool              * aExitFlag,
                                       rpxReceiverReadContext aReadContext,
                                       rpdMeta             * aMeta,    // BUG-20506
                                       rpdXLog             * aXLog,
                                       ULong                 aTimeOutSec );
        
    static IDE_RC recvLobCursorCloseA5( cmiProtocol         * aProtocol,
                                        rpdXLog             * aXLog );
    static IDE_RC recvLobCursorCloseA7( rpxReceiverReadContext aReadContext,
                                        idBool              * aExitFlag,
                                        rpdXLog             * aXLog );
    
    static IDE_RC recvLobPrepare4WriteA5( cmiProtocol        * aProtocol,
                                          rpdXLog            * aXLog );
    static IDE_RC recvLobPrepare4WriteA7( rpxReceiverReadContext aReadContext,
                                          idBool             * aExitFlag,
                                          rpdXLog            * aXLog );
    
    static IDE_RC recvLobPartialWriteA5( iduMemAllocator    * aAllocator,
                                         cmiProtocol        * aProtocol,
                                         rpdXLog            * aXLog );
    static IDE_RC recvLobPartialWriteA7( iduMemAllocator    * aAllocator,
                                         idBool             * aExitFlag,
                                         rpxReceiverReadContext aReadContext,
                                         rpdXLog            * aXLog,
                                         ULong                aTimeOutSec );
    
    static IDE_RC recvLobFinish2WriteA5( cmiProtocol        * aProtocol,
                                         rpdXLog            * aXLog );
    static IDE_RC recvLobFinish2WriteA7( rpxReceiverReadContext aReadContext,
                                         idBool             * aExitFlag,
                                         rpdXLog            * aXLog );

    static IDE_RC recvHandshakeA5( cmiProtocol        * aProtocol,
                                   rpdXLog            * aXLog );
    static IDE_RC recvHandshakeA7( rpxReceiverReadContext aReadContext,
                                   idBool             * aExitFlag,
                                   rpdXLog            * aXLog );

    static IDE_RC recvSyncPKBeginA5( cmiProtocol        * aProtocol,
                                     rpdXLog            * aXLog );
    static IDE_RC recvSyncPKBeginA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdXLog            * aXLog );

    static IDE_RC recvSyncPKA5( idBool             * aExitFlag,
                                cmiProtocolContext * aProtocolContext,
                                cmiProtocol        * aProtocol,
                                rpdXLog            * aXLog,
                                ULong                aTimeOutSec );
    static IDE_RC recvSyncPKA7( idBool             * aExitFlag,
                                cmiProtocolContext * aProtocolContext,
                                rpdXLog            * aXLog,
                                ULong                aTimeOutSec );    

    static IDE_RC recvSyncPKEndA5( cmiProtocol        * aProtocol,
                                   rpdXLog            * aXLog );
    static IDE_RC recvSyncPKEndA7( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdXLog            * aXLog );
    
    static IDE_RC recvFailbackEndA5( cmiProtocol        * aProtocol,
                                     rpdXLog            * aXLog );
    static IDE_RC recvFailbackEndA7( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdXLog            * aXLog );
    
    static IDE_RC recvValueA5( iduMemAllocator    * aAllocator,
                               idBool             * aExitFlag,
                               cmiProtocolContext * aProtocolContext,
                               iduMemory          * aMemory,
                               smiValue           * aValue,
                               ULong                aTimeOutSec );
    // PROJ-1705
    static IDE_RC recvValueA7( iduMemAllocator    * aAllocator,
                               idBool             * aExitFlag,
                               rpxReceiverReadContext aReadContext,
                               iduMemory          * aMemory,
                               smiValue           * aValue,
                               ULong                aTimeOutSec,
                               mtcColumn          * aMtcColumn );

    static IDE_RC recvCIDA5( idBool             * aExitFlag,
                             cmiProtocolContext * aProtocolContext,
                             UInt               * aCID,
                             ULong                aTimeOutSec );
    static IDE_RC recvCIDA7( idBool             * aExitFlag,
                             rpxReceiverReadContext aReadContext,
                             UInt               * aCID,
                             ULong                aTimeOutSec );
    
    static IDE_RC sendAckA5( cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             rpXLogAck          * aAck,
                             UInt                 aTimeoutSec );
    static IDE_RC sendAckA7( cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             rpXLogAck          * aAck,
                             UInt                 aTimeoutSec );

    static IDE_RC recvAckA5(iduMemAllocator   * aAllocator,
                            cmiProtocolContext * aProtocolContext,
                            idBool         *aExitFlag,
                            rpXLogAck      *aAck,
                            ULong           aTimeOut,
                            idBool         *aIsTimeOut);

    static IDE_RC recvAckA7(iduMemAllocator    * aAllocator,
                            void               * aHBTResource,
                            cmiProtocolContext * aProtocolContext,
                            idBool         *aExitFlag,
                            rpXLogAck      *aAck,
                            ULong           aTimeOut,
                            idBool         *aIsTimeOut);

    static IDE_RC sendTxAckA5( cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               UInt                 aTimeoutSec );
    static IDE_RC sendTxAckA7( cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               UInt                 aTimeoutSec );

    static IDE_RC validateA7Protocol( cmiProtocolContext * aProtocolContext );

    static IDE_RC writeProtocol( void                 * aHBTResource,
                                 cmiProtocolContext   * aProtocolContext,
                                 cmiProtocol          * aProtocol);

    static IDE_RC flushProtocol( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 idBool               aIsEnd,
                                 UInt                 aTimeoutSec );

    static IDE_RC flushPedningBlock( void                 * aHBTResource,
                                     cmiProtocolContext   * aProtocolContext,
                                     idBool               * aExitFlag,
                                     UInt                   aTimeoutSec );

    static IDE_RC checkAndFlush( void                   * aHBTResource,
                                 cmiProtocolContext     * aProtocolContext,
                                 idBool                 * aExitFlag,
                                 UInt                     aLen,
                                 idBool                   aIsEnd,
                                 UInt                     aTimeoutSec );

    static IDE_RC sendVersionA5( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdVersion         * aReplVersion,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplA5( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  rpdReplications    * aRepl,
                                  UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplTblA5( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdMetaItem        * aItem,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplColA5( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdColumn          * aColumn,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplIdxA5( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     qcmIndex           * aIndex,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplIdxColA5( void                * aHBTResource,
                                        cmiProtocolContext  * aProtocolContext,
                                        idBool              * aExitFlag,
                                        UInt                  aColumnID,
                                        UInt                  aKeyColumnFlag,
                                        UInt                  aTimeoutSec );

    static IDE_RC sendMetaReplCheckA5( void                 * aHBTResource,
                                       cmiProtocolContext   * aProtocolContext,
                                       idBool               * aExitFlag,
                                       qcmCheck             * aCheck,
                                       UInt                   aTimeoutSec );

    static IDE_RC sendTrBeginA5( void                * aHBTResource,
                                 cmiProtocolContext  * aProtocolContext,
                                 idBool              * aExitFlag,
                                 smTID                 aTID,
                                 smSN                  aSN,
                                 smSN                  aSyncSN,
                                 UInt                  aTimeoutSec );

    static IDE_RC sendTrCommitA5( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  smTID                aTID,
                                  smSN                 aSN,
                                  smSN                 aSyncSN,
                                  idBool               aForceFlush,
                                  UInt                 aTimeoutSec );

    static IDE_RC sendTrAbortA5( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendSPSetA5( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aSPNameLen,
                               SChar              * aSPName,
                               UInt                 aTimeoutSec );

    static IDE_RC sendSPAbortA5( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aSPNameLen,
                                 SChar              * aSPName,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendInsertA5( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                UInt                 aImplSPDepth,
                                ULong                aTableOID,
                                UInt                 aColCnt,
                                smiValue           * aACols,
                                rpValueLen         * aALen,
                                UInt                 aTimeoutSec );

    static IDE_RC sendUpdateA5( void                * aHBTResource,
                                cmiProtocolContext  * aProtocolContext,
                                idBool              * aExitFlag,
                                smTID                 aTID,
                                smSN                  aSN,
                                smSN                  aSyncSN,
                                UInt                  aImplSPDepth,
                                ULong                 aTableOID,
                                UInt                  aPKColCnt,
                                UInt                  aUpdateColCnt,
                                smiValue            * aPKCols,
                                UInt                * aCIDs,
                                smiValue            * aBCols,
                                smiChainedValue     * aBChainedCols,
                                UInt                * aBChainedColsTotalLen,
                                smiValue            * aACols,
                                rpValueLen          * aPKLen,
                                rpValueLen          * aALen,
                                rpValueLen          * aBMtdValueLen,
                                UInt                  aTimeoutSec );

    static IDE_RC sendDeleteA5( void              * aHBTResource,
                                cmiProtocolContext *aProtocolContext,
                                idBool            * aExitFlag,
                                smTID               aTID,
                                smSN                aSN,
                                smSN                aSyncSN,
                                UInt                aImplSPDepth,
                                ULong               aTableOID,
                                UInt                aPKColCnt,
                                smiValue           *aPKCols,
                                rpValueLen         *aPKLen,
                                UInt                aColCnt,
                                UInt              * aCIDs,
                                smiValue          * aBCols,
                                smiChainedValue   * aBChainedCols,
                                rpValueLen        * aBMtdValueLen,
                                UInt              * aBChainedColsTotalLen,
                                UInt                aTimeoutSec );

    static IDE_RC sendCIDForA5( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                UInt                 aCID,
                                UInt                 aTimeoutSec );

    static IDE_RC sendValueForA5( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  smiValue           * aValue,
                                  rpValueLen           aValueLen,
                                  UInt                 aTimeoutSec );

    static IDE_RC sendPKValueForA5( void                * aHBTHostResource,
                                    cmiProtocolContext  * aProtocolContext,
                                    idBool              * aExitFlag,
                                    smiValue            * aValue,
                                    rpValueLen            aPKLen,
                                    UInt                  aTimeoutSec);

    static IDE_RC sendChainedValueForA5( void               * aHBTHostResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         smiChainedValue    * aChainedValue,
                                         rpValueLen           aBMtdValueLen,
                                         UInt                 aTimeoutSec );

    static IDE_RC sendStopA5( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              smSN                 aRestartSN,
                              UInt                 aTimeoutSec );

    static IDE_RC sendKeepAliveA5( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   smSN                 aRestartSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendFlushA5( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aFlushOption,
                               UInt                 aTimeoutSec );

    static IDE_RC sendLobCursorOpenA5( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       smTID                aTID,
                                       smSN                 aSN,
                                       smSN                 aSyncSN,
                                       ULong                aTableOID,
                                       ULong                aLobLocator,
                                       UInt                 aLobColumnID,
                                       UInt                 aPKColCnt,
                                       smiValue           * aPKCols,
                                       rpValueLen         * aPKLen,
                                       UInt                 aTimeoutSec );

    static IDE_RC sendLobCursorCloseA5( void                * aHBTResource,
                                        cmiProtocolContext  * aProtocolContext,
                                        idBool              * aExitFlag,
                                        smTID                 aTID,
                                        smSN                  aSN,
                                        smSN                  aSyncSN,
                                        ULong                 aLobLocator,
                                        UInt                  aTimeoutSec );

    static IDE_RC sendLobPrepare4WriteA5( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          smTID                aTID,
                                          smSN                 aSN,
                                          smSN                 aSyncSN,
                                          ULong                aLobLocator,
                                          UInt                 aLobOffset,
                                          UInt                 aLobOldSize,
                                          UInt                 aLobNewSize,
                                          UInt                 aTimeoutSec );

    static IDE_RC sendLobTrimA5( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 ULong                aLobLocator,
                                 UInt                 aLobOffset,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendLobPartialWriteA5( void               * aHBTResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         smTID                aTID,
                                         smSN                 aSN,
                                         smSN                 aSyncSN,
                                         ULong                aLobLocator,
                                         UInt                 aLobOffset,
                                         UInt                 aLobPieceLen,
                                         SChar              * aLobPiece,
                                         UInt                 aTimeoutSec );

    static IDE_RC sendLobFinish2WriteA5( void               * aHBTResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         smTID                aTID,
                                         smSN                 aSN,
                                         smSN                 aSyncSN,
                                         ULong                aLobLocator,
                                         UInt                 aTimeoutSec );

    static IDE_RC sendHandshakeA5( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKBeginA5( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKA5( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                ULong                aTableOID,
                                UInt                 aPKColCnt,
                                smiValue           * aPKCols,
                                rpValueLen         * aPKLen,
                                UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKEndA5( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendFailbackEndA5( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendVersionA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdVersion         * aReplVersion,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  rpdReplications    * aRepl,
                                  UInt                 aTimeoutSec );

    static IDE_RC sendTempSyncMetaReplA7( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          rpdReplications    * aRepl,
                                          UInt                 aTimeoutSec );


    static IDE_RC sendTempSyncReplItemA7( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          rpdReplSyncItem    * aTempSyncItem,
                                          UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplTblA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdMetaItem        * aItem,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplColA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     rpdColumn          * aColumn,
                                     rpdVersion           aRemoteVersion,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplIdxA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     qcmIndex           * aIndex,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendMetaReplIdxColA7( void                * aHBTResource,
                                        cmiProtocolContext  * aProtocolContext,
                                        idBool              * aExitFlag,
                                        UInt                  aColumnID,
                                        UInt                  aKeyColumnFlag,
                                        UInt                  aTimeoutSec );

    static IDE_RC sendMetaReplCheckA7( void                 * aHBTResource,
                                       cmiProtocolContext   * aProtocolContext,
                                       idBool               * aExitFlag,
                                       qcmCheck             * aCheck,
                                       UInt                   aTimeoutSec );

    static IDE_RC sendTrBeginA7( void                * aHBTResource,
                                 cmiProtocolContext  * aProtocolContext,
                                 idBool              * aExitFlag,
                                 smTID                 aTID,
                                 smSN                  aSN,
                                 smSN                  aSyncSN,
                                 UInt                  aTimeoutSec );

    static IDE_RC sendTrCommitA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  smTID                aTID,
                                  smSN                 aSN,
                                  smSN                 aSyncSN,
                                  idBool               aForceFlush,
                                  UInt                 aTimeoutSec );

    static IDE_RC sendTrAbortA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendSPSetA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aSPNameLen,
                               SChar              * aSPName,
                               UInt                 aTimeoutSec );

    static IDE_RC sendSPAbortA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aSPNameLen,
                                 SChar              * aSPName,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendInsertA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                UInt                 aImplSPDepth,
                                ULong                aTableOID,
                                UInt                 aColCnt,
                                smiValue           * aACols,
                                rpValueLen         * aALen,
                                UInt                 aTimeoutSec );

    static IDE_RC sendUpdateA7( void            * aHBTResource,
                                cmiProtocolContext *aProtocolContext,
                                idBool          * aExitFlag,
                                smTID             aTID,
                                smSN              aSN,
                                smSN              aSyncSN,
                                UInt              aImplSPDepth,
                                ULong             aTableOID,
                                UInt              aPKColCnt,
                                UInt              aUpdateColCnt,
                                smiValue        * aPKCols,
                                UInt            * aCIDs,
                                smiValue        * aBCols,
                                smiChainedValue * aBChainedCols,
                                UInt            * aBChainedColsTotalLen,
                                smiValue        * aACols,
                                rpValueLen      * aPKLen,
                                rpValueLen      * aALen,
                                rpValueLen      * aBMtdValueLen,
                                UInt              aTimeoutSec );

    static IDE_RC sendDeleteA7( void              * aHBTResource,
                                cmiProtocolContext *aProtocolContext,
                                idBool            * aExitFlag,
                                smTID               aTID,
                                smSN                aSN,
                                smSN                aSyncSN,
                                UInt                aImplSPDepth,
                                ULong               aTableOID,
                                UInt                aPKColCnt,
                                smiValue           *aPKCols,
                                rpValueLen         *aPKLen,
                                UInt                aColCnt,
                                UInt              * aCIDs,
                                smiValue          * aBCols,
                                smiChainedValue   * aBChainedCols,
                                rpValueLen        * aBMtdValueLen,
                                UInt              * aBChainedColsTotalLen,
                                UInt                aTimeoutSec );

    static IDE_RC sendCIDForA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                UInt                 aCID,
                                UInt                 aTimeoutSec );

    static IDE_RC recvTxAckForA7( idBool             * aExitFlag,
                                  cmiProtocolContext * aProtocolContext,
                                  smTID              * aTID,
                                  smSN               * aSN,
                                  ULong                aTimeoutSec );

    static IDE_RC recvTxAckForA5( idBool             * aExitFlag,
                                  cmiProtocolContext * aProtocolContext,
                                  smTID              * aTID,
                                  smSN               * aSN,
                                  ULong                aTimeoutSec );

    static IDE_RC sendValueForA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  smiValue           * aValue,
                                  rpValueLen           aValueLen,
                                  UInt                 aTimeoutSec );

    // PROJ-1705
    static IDE_RC sendPKValueForA7( void                * aHBTHostResource,
                                    cmiProtocolContext  * aProtocolContext,
                                    idBool              * aExitFlag,
                                    smiValue            * aValue,
                                    rpValueLen            aPKLen,
                                    UInt                  aTimeoutSec );

    static IDE_RC sendStopA7( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              smSN                 aRestartSN,
                              UInt                 aTimeoutSec );

    static IDE_RC sendKeepAliveA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   smSN                 aRestartSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendFlushA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aFlushOption,
                               UInt                 aTimeoutSec );

    static IDE_RC sendLobCursorOpenA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       smTID                aTID,
                                       smSN                 aSN,
                                       smSN                 aSyncSN,
                                       ULong                aTableOID,
                                       ULong                aLobLocator,
                                       UInt                 aLobColumnID,
                                       UInt                 aPKColCnt,
                                       smiValue           * aPKCols,
                                       rpValueLen         * aPKLen,
                                       UInt                 aTimeoutSec );

    static IDE_RC sendLobCursorCloseA7( void                * aHBTResource,
                                        cmiProtocolContext  * aProtocolContext,
                                        idBool              * aExitFlag,
                                        smTID                 aTID,
                                        smSN                  aSN,
                                        smSN                  aSyncSN,
                                        ULong                 aLobLocator,
                                        UInt                  aTimeoutSec );

    static IDE_RC sendLobPrepare4WriteA7( void               * aHBTResource,
                                          cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          smTID                aTID,
                                          smSN                 aSN,
                                          smSN                 aSyncSN,
                                          ULong                aLobLocator,
                                          UInt                 aLobOffset,
                                          UInt                 aLobOldSize,
                                          UInt                 aLobNewSize,
                                          UInt                 aTimeoutSec );

    static IDE_RC sendLobTrimA7( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 ULong                aLobLocator,
                                 UInt                 aLobOffset,
                                 UInt                 aTimeoutSec );

    static IDE_RC sendLobPartialWriteA7( void               * aHBTResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         smTID                aTID,
                                         smSN                 aSN,
                                         smSN                 aSyncSN,
                                         ULong                aLobLocator,
                                         UInt                 aLobOffset,
                                         UInt                 aLobPieceLen,
                                         SChar              * aLobPiece,
                                         UInt                 aTimeoutSec );

    static IDE_RC sendLobFinish2WriteA7( void               * aHBTResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         smTID                aTID,
                                         smSN                 aSN,
                                         smSN                 aSyncSN,
                                         ULong                aLobLocator,
                                         UInt                 aTimeoutSec );

    static IDE_RC sendHandshakeA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKBeginA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKA7( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                ULong                aTableOID,
                                UInt                 aPKColCnt,
                                smiValue           * aPKCols,
                                rpValueLen         * aPKLen,
                                UInt                 aTimeoutSec );

    static IDE_RC sendSyncPKEndA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec );

    static IDE_RC sendFailbackEndA7( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     UInt                 aTimeoutSec );

    static IDE_RC sendDDLASyncStartA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       UInt                 aType,
                                       ULong                aSendTimeout );

    static IDE_RC recvDDLASyncStartA7( cmiProtocolContext * aProtocolContext,
                                       idBool             * /*aExitFlag*/,
                                       rpdXLog            * aXLog );

    static IDE_RC sendDDLASyncStartAckA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt                 aType,
                                          ULong                aSendTimeout );

    static IDE_RC recvDDLASyncStartAckA7( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aType,
                                          ULong                aRecvTimeout );

    static IDE_RC sendDDLASyncExecuteA7( void               * aHBTResource,
                                         cmiProtocolContext * aProtocolContext,
                                         idBool             * aExitFlag,
                                         UInt                 aType,
                                         SChar              * aUserName,
                                         UInt                 aDDLEnableLevel,
                                         UInt                 aTargetCount,
                                         SChar              * aTargetTableName,
                                         SChar              * aTargetPartNames,
                                         smSN                 aDDLCommitSN,
                                         rpdVersion         * aReplVersion,
                                         SChar              * aDDLStmt,
                                         ULong                aSendTimeout );

    static  IDE_RC recvDDLASyncExecuteA7( cmiProtocolContext  * aProtocolContext,
                                          idBool              * aExitFlag,
                                          UInt                * aType,
                                          SChar               * aUserName,
                                          UInt                * aDDLEnableLevel,
                                          UInt                * aTargetCount,
                                          SChar               * aTargetTableName,
                                          SChar              ** aTargetPartNames,
                                          smSN                * aDDLCommitSN,
                                          rpdVersion          * aReplVersion,
                                          SChar              ** aDDLStmt,
                                          ULong                 aRecvTimeout );

    static IDE_RC sendDDLASyncExecuteAckA7( cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            UInt                 aType,
                                            UInt                 aIsSuccess,
                                            UInt                 aErrCode,
                                            SChar              * aErrMsg,
                                            ULong                aSendTimeout );

    static IDE_RC recvDDLASyncExecuteAckA7( cmiProtocolContext * aProtocolContext,
                                            idBool             * aExitFlag,
                                            UInt               * aType,
                                            UInt               * aIsSuccess,
                                            UInt               * aErrCode,
                                            SChar              * aErrMsg,
                                            ULong                aRecvTimeout );
                                       
    static  IDE_RC sendMetaInitFlagA7( void               * aHBTResource,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * aExitFlag,
                                       idBool               aMetaInitFlag,
                                       UInt                 aTimeoutSec );

    static IDE_RC recvMetaInitFlagA7( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      idBool             * aMetaInitFlag,
                                      ULong                aTimeoutSec );

    static IDE_RC sendConditionInfoA7( void                 * aHBTResource,
                                       cmiProtocolContext   * aProtocolContext,
                                       idBool               * aExitFlag,
                                       rpdConditionItemInfo * aConditionInfo,
                                       UInt                   aConditionCnt,
                                       UInt                   aTimeoutSec );

    static IDE_RC recvConditionInfoA7( cmiProtocolContext   * aProtocolContext,
                                       idBool               * aExitFlag,
                                       rpdConditionItemInfo * aRecvConditionInfo,
                                       UInt                 * aRecvConditionCnt,
                                       ULong                   aTimeoutSec );

    static IDE_RC sendConditionInfoResultA7( cmiProtocolContext   * aProtocolContext,
                                             idBool               * aExitFlag,
                                             rpdConditionActInfo  * aConditionInfo,
                                             UInt                   aConditionCnt,
                                             UInt                   aTimeoutSec );

    static IDE_RC recvConditionInfoResultA7( cmiProtocolContext  * aProtocolContext,
                                             idBool              * aExitFlag,
                                             rpdConditionActInfo * aConditionInfo,
                                             ULong                 aTimeoutSec );

    static IDE_RC sendTruncateA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  ULong                aTableOID,
                                  UInt                 aTimeoutSec );

    static IDE_RC recvTruncateA7( cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  rpdMeta            * aMeta,
                                  rpdXLog            * aXLog );

    static IDE_RC sendXaStartReqA7( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    ID_XID             * aXID,
                                    smTID                aTID,
                                    smSN                 aSN,
                                    ULong                aSendTimeout );
    static IDE_RC recvXaStartReqA7( rpxReceiverReadContext   aReadContext,
                                    idBool                 * /*aExitFlag*/,
                                    rpdXLog                * aXLog );


    static IDE_RC sendXaPrepareReqA7( void               * aHBTResource,
                                      cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      ID_XID             * aXID,
                                      smTID                aTID,
                                      smSN                 aSN,
                                      ULong                aSendTimeout );
    static IDE_RC recvXaPrepareReqA7( rpxReceiverReadContext   aReadContext,
                                      idBool                 * /*aExitFlag*/,
                                      rpdXLog                * aXLog );


    static IDE_RC sendXaPrepareA7( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   ID_XID             * aXID,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   ULong                aSendTimeout );
    static IDE_RC recvXaPrepareA7( rpxReceiverReadContext   aReadContext,
                                   idBool                 * /*aExitFlag*/,
                                   rpdXLog                * aXLog );

    static IDE_RC sendXaCommitA7( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  smTID                aTID,
                                  smSN                 aSN,
                                  smSN                 aSyncSN,
                                  smSCN                aGlobalCommitSCN,
                                  idBool               aForceFlush,
                                  UInt                 aTimeoutSec );

    static IDE_RC recvXaCommitA7( rpxReceiverReadContext aReadContext,
                                  idBool             * aExitFlag,
                                  rpdXLog            * aXLog );

    static IDE_RC sendXaEndA7( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               ULong                aSendTimeout );
    static IDE_RC recvXaEndA7( rpxReceiverReadContext   aReadContext,
                               idBool                 * /*aExitFlag*/,
                               rpdXLog                * aXLog );

    static IDE_RC readXLogfile( rpdXLogfileMgr * aXLogfileManager );

    // Attribute
public:
    static PDL_Time_Value    mTV1Sec;

    static UInt rpnXLogLengthForEachType[CMP_OP_RP_MAX_VER1 + 1];
    /* PROJ-2453 */
public:
    static IDE_RC sendAckOnDML( void               *aHBTResource,
                                cmiProtocolContext *aProtocolContext,
                                idBool             *aExitFlag,
                                UInt                aTimeoutSec );
    static IDE_RC sendAckEager( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpXLogAck          * aAck,
                                UInt                 aTimeoutSec );
    static IDE_RC sendCmBlock( void                * aHBTResource,
                               cmiProtocolContext  * aContext,
                               idBool              * aExitFlag,
                               idBool                aIsEnd,
                               UInt                  aTimeoutSec );
private:
    static IDE_RC recvAckOnDML( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpdXLog            * aXLog );
};


#endif  /* _O_RPN_COMM_H_ */
