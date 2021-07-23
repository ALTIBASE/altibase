/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef _O_ULA_COMM_H_
#define _O_ULA_COMM_H_ 1

#include <aclMem.h>

#include <cmAllClient.h>
#include <ula.h>
#include <ulaErrorMgr.h>
#include <ulaMeta.h>

/* mFlags in cmpArgRPMetaReplCol */
#define ULA_META_COLUMN_NOTNULL_MASK    (0x00000004)
#define ULA_META_COLUMN_NOTNULL_TRUE    (0x00000004)
#define ULA_META_COLUMN_NOTNULL_FALSE   (0x00000000)

/* mColumnID in cmpArgRPMetaReplCol */
#define ULA_META_COLUMN_ID_MASK         (0x000003FF)

/* Implicit Savepoint 생성 정보 */
#define ULA_IMPLICIT_SVP_NAME           (acp_char_t *)"$$IMPLICIT"
#define ULA_IMPLICIT_SVP_NAME_SIZE      (10)
#define ULA_STATEMENT_DEPTH_NULL        (0)
#define ULA_STATEMENT_DEPTH_MAX         (255)
#define ULA_STATEMENT_DEPTH_MAX_SIZE    (3)


typedef struct
{
    acp_uint32_t    mAckType;           // ACK or STOP_ACK
    acp_uint32_t    mAbortTxCount;
    acp_uint32_t    mClearTxCount;
    ulaSN           mRestartSN;
    ulaSN           mLastCommitSN;
    ulaSN           mLastArrivedSN;
    ulaSN           mLastProcessedSN;
    ulaTID         *mAbortTxList;       // for Eager Mode
    ulaTID         *mClearTxList;       // for Eager Mode
} ulaXLogAck;


void ulaCommInitialize(void);
void ulaCommDestroy(void);

/* wakeupPeerSender */
ACI_RC ulaCommSendVersion( cmiProtocolContext * aProtocolContext,
                           ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommSendMetaRepl( cmiProtocolContext * aProtocolContext,
                            acp_char_t         * aRepName,
                            acp_uint32_t         aFlag,
                            acp_uint64_t         sRemoteVersion,
                            ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvHandshakeAck( cmiProtocolContext * aProtocolContext,
                                acp_bool_t         * aExitFlag,
                                acp_uint32_t       * aResult,
                                acp_char_t         * aMsg,
                                acp_uint32_t       * aMsgLen,
                                acp_uint32_t         aTimeOut,
                                ulaErrorMgr        * aOutErrorMgr );
/* Handshake */
ACI_RC ulaCommRecvVersion( cmiProtocolContext * aProtocolContext,
                           ulaVersion         * aOutReplVersion,
                           acp_bool_t         * aExitFlag,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr );
ACI_RC ulaCommRecvMetaRepl( cmiProtocolContext * aProtocolContext,
                            acp_bool_t         * aExitFlag,
                            ulaReplication     * aOutRepl,
                            ulaReplTempInfo    * aOutReplTempInfo,
                            acp_uint32_t         aTimeoutSec,
                            ulaErrorMgr        * aOutErrorMgr );
ACI_RC ulaCommRecvMetaReplTbl( cmiProtocolContext * aProtocolContext,
                               acp_bool_t         * aExitFlag,
                               ulaTable           * aOutTable,
                               acp_uint32_t         aTimeoutSec,
                               ulaErrorMgr        * aOutErrorMgr );
ACI_RC ulaCommRecvMetaReplCol( cmiProtocolContext * aProtocolContext,
                               acp_bool_t         * aExitFlag,
                               ulaColumn          * aOutColumn,
                               acp_uint32_t         aTimeoutSec,
                               ulaErrorMgr        * aOutErrorMgr );
ACI_RC ulaCommRecvMetaReplIdx( cmiProtocolContext * aProtocolContext,
                               acp_bool_t         * aExitFlag,
                               ulaIndex           * aOutIndex,
                               acp_uint32_t         aTimeoutSec,
                               ulaErrorMgr        * aOutErrorMgr);
ACI_RC ulaCommRecvMetaReplIdxCol( cmiProtocolContext * aProtocolContext,
                                  acp_bool_t         * aExitFlag,
                                  acp_uint32_t       * aOutColumnID,
                                  acp_uint32_t         aTimeoutSec,
                                  ulaErrorMgr        * aOutErrorMgr );
ACI_RC ulaCommRecvMetaReplCheck( cmiProtocolContext * aProtocolContext,
                                 acp_bool_t         * aExitFlag,
                                 ulaCheck           * aCheck,
                                 acp_uint32_t         aTimeoutSec,
                                 ulaErrorMgr        * aOutErrorMgr );
ACI_RC ulaCommSendHandshakeAck( cmiProtocolContext * aProtocolContext,
                                acp_uint32_t         aResult,
                                acp_char_t         * aMsg,
                                ulaErrorMgr        * aOutErrorMgr );
/* Receive XLog */
ACI_RC ulaCommRecvXLog( cmiProtocolContext * aProtocolContext,
                        acp_bool_t         * aExitFlag,
                        ulaXLog            * aOutXLog,
                        acl_mem_alloc_t    * aAllocator,
                        acp_uint32_t         aTimeoutSec,
                        ulaErrorMgr        * aOutErrorMgr );

// PROJ-1663 : BEGIN 패킷 미사용
ACI_RC ulaCommRecvTrBegin(acp_bool_t         *aExitFlag,
                          cmiProtocolContext *aProtocolContext,
                          ulaXLog            *aOutXLog,
                          acp_uint32_t        aTimeoutSec,
                          ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvTrCommit(acp_bool_t         *aExitFlag,
                           cmiProtocolContext *aProtocolContext,
                           ulaXLog            *aOutXLog,
                           acp_uint32_t        aTimeoutSec,
                           ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvTrAbort(acp_bool_t         *aExitFlag,
                          cmiProtocolContext *aProtocolContext,
                          ulaXLog            *aOutXLog,
                          acp_uint32_t        aTimeoutSec,
                          ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvSPSet( acp_bool_t         * aExitFlag,
                         cmiProtocolContext * aProtocolContext,
                         ulaXLog            * aOutXLog,
                         acl_mem_alloc_t    * aAllocator,
                         acp_uint32_t         aTimeoutSec,
                         ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvSPAbort( acp_bool_t         * aExitFlag,
                           cmiProtocolContext * aProtocolContext,
                           ulaXLog            * aOutXLog,
                           acl_mem_alloc_t    * aAllocator,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvInsert( acp_bool_t         * aExitFlag,
                          cmiProtocolContext * aProtocolContext,
                          ulaXLog            * aOutXLog,
                          acl_mem_alloc_t    * aAllocator,
                          acp_uint32_t         aTimeoutSec,
                          ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvUpdate( acp_bool_t         * aExitFlag,
                          cmiProtocolContext * aProtocolContext,
                          ulaXLog            * aOutXLog,
                          acl_mem_alloc_t    * aAllocator,
                          acp_uint32_t         aTimeoutSec,
                          ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvDelete( acp_bool_t         * aExitFlag,
                          cmiProtocolContext * aProtocolContext,
                          ulaXLog            * aOutXLog,
                          acl_mem_alloc_t    * aAllocator,
                          acp_uint32_t         aTimeoutSec,
                          ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvCID(acp_bool_t         *aExitFlag,
                      cmiProtocolContext *aProtocolContext,
                      acp_uint32_t       *aOutCID,
                      acp_uint32_t        aTimeoutSec,
                      ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvValue( acp_bool_t         * aExitFlag,
                         cmiProtocolContext * aProtocolContext,
                         ulaValue           * aOutValue,
                         acl_mem_alloc_t    * aAllocator,
                         acp_uint32_t         aTimeoutSec,
                         ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvStop(acp_bool_t         *aExitFlag,
                       cmiProtocolContext *aProtocolContext,
                       ulaXLog            *aOutXLog,
                       acp_uint32_t        aTimeoutSec,
                       ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvKeepAlive(acp_bool_t         *aExitFlag,
                            cmiProtocolContext *aProtocolContext,
                            ulaXLog            *aOutXLog,
                            acp_uint32_t        aTimeoutSec,
                            ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvLobCursorOpen( acp_bool_t         * aExitFlag,
                                 cmiProtocolContext * aProtocolContext,
                                 ulaXLog            * aOutXLog,
                                 acl_mem_alloc_t    * aAllocator,
                                 acp_uint32_t         aTimeoutSec,
                                 ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvLobCursorClose(acp_bool_t         *aExitFlag,
                                 cmiProtocolContext *aProtocolContext,
                                 ulaXLog            *aOutXLog,
                                 acp_uint32_t        aTimeoutSec,
                                 ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvLobPrepare4Write(acp_bool_t         *aExitFlag,
                                   cmiProtocolContext *aProtocolContext,
                                   ulaXLog            *aOutXLog,
                                   acp_uint32_t        aTimeoutSec,
                                   ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvLobPartialWrite( acp_bool_t         * aExitFlag,
                                   cmiProtocolContext * aProtocolContext,
                                   ulaXLog            * aOutXLog,
                                   acl_mem_alloc_t    * aAllocator,
                                   acp_uint32_t         aTimeoutSec,
                                   ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvLobFinish2Write(acp_bool_t         *aExitFlag,
                                  cmiProtocolContext *aProtocolContext,
                                  ulaXLog            *aOutXLog,
                                  acp_uint32_t        aTimeoutSec,
                                  ulaErrorMgr        *aOutErrorMgr);

ACI_RC ulaCommRecvLobTrim( acp_bool_t         * aExitFlag,
                           cmiProtocolContext * aProtocolContext,
                           ulaXLog            * aOutXLog,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr );

/* Send ACK */
ACI_RC ulaCommSendAck( cmiProtocolContext * aProtocolContext,
                       ulaXLogAck           aAck,
                       ulaErrorMgr        * OutErrorMgr );

ACI_RC ulaCommReadCmBlock( cmiProtocolContext * aProtocolContext,
                           acp_bool_t         * aExitFlag,
                           acp_bool_t         * aIsTimeOut,
                           acp_uint32_t         aTimeOut,
                           ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvHandshake( acp_bool_t         * aExitFlag,
                             cmiProtocolContext * aProtocolContext,
                             ulaXLog            * aOutXLog,
                             acp_uint32_t         aTimeoutSec,
                             ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvMetaPartitionCount( cmiProtocolContext * aProtocolContext,
                                      acp_bool_t         * aExitFlag,
                                      acp_uint32_t         aTimeoutSec,
                                      ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommSendMetaDictTableCount( cmiProtocolContext * aProtocolContext,
                                      ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvMetaDictTableCount( cmiProtocolContext * aProtocolContext,
                                      acp_bool_t         * aExitFlag,
                                      acp_uint32_t         aTimeoutSec,
                                      ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommSendMetaInitFlag( cmiProtocolContext * aProtocolContext,
                                ulaErrorMgr        * aOutErrorMgr );

ACI_RC ulaCommRecvMetaInitFlag( cmiProtocolContext * aProtocolContext,
                                acp_bool_t         * aExitFlag,
                                acp_uint32_t         aTimeoutSec,
                                ulaErrorMgr        * aOutErrorMgr );
#endif  /* _O_ULA_COMM_H_ */
