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
 
#ifndef _O_XLOG_COLLECTOR_H_
#define _O_XLOG_COLLECTOR_H_ 1

#include <aclMem.h>

#include <cmAllClient.h>
#include <mtccDef.h>
#include <ula.h>
#include <ulaXLogLinkedList.h>
#include <ulaTransTbl.h>
#include <ulaMeta.h>
#include <ulaLog.h>

#define ULA_MAX_AUTH_INFO_SIZE              (32)

#define ULA_MIN_PORT_NO                     (1024)
#define ULA_MAX_PORT_NO                     (65535)

#define ULA_MIN_XLOG_POOL_SIZE              (1)
#define ULA_MAX_INIT_XLOG_POOL_SIZE         (512)
#define ULA_MIN_ACK_PER_XLOG_SIZE           (1)

#define ULA_TIMEOUT_MIN                     (1)
#define ULA_TIMEOUT_MAX                     (ACP_UINT32_MAX - 1)
#define ULA_DEFAULT_HANDSHAKE_TIMEOUT       (600)
#define ULA_DEFAULT_RECEIVE_XLOG_TIMEOUT    (10)

#define ULA_AUTH_INFO_MUTEX_NAME            (acp_char_t *)"AUTH_INFO_MUTEX"
#define ULA_SEND_MUTEX_NAME                 (acp_char_t *)"SEND_MUTEX"
#define ULA_RECEIVE_MUTEX_NAME              (acp_char_t *)"RECEIVE_MUTEX"
#define ULA_XLOG_POOL_MUTEX_NAME            (acp_char_t *)"XLOG_POOL_MUTEX"
#define ULA_ACK_MUTEX_NAME                  (acp_char_t *)"ACK_MUTEX"
#define ULA_PSM_SAVEPOINT_NAME              (acp_char_t *)"$$PSM_SVP"

#define ULA_ACK_MSG_LEN                     (1024)

#define ULA_GET_MAJOR_VERSION(a) \
                    (acp_uint32_t)(((acp_uint64_t)(a) >> 48) & 0xFFFF)
#define ULA_GET_MINOR_VERSION(a) \
                    (acp_uint32_t)(((acp_uint64_t)(a) >> 32) & 0xFFFF)
#define ULA_GET_FIX_VERSION(a)   \
                    (acp_uint32_t)(((acp_uint64_t)(a) >> 16)  & 0xFFFF)
#define ULA_GET_ENDIAN_64BIT(a)  \
                    (acp_uint32_t)((acp_uint64_t)(a) & 0xFFFF)


#ifdef ENDIAN_IS_BIG_ENDIAN

#ifdef COMPILE_64BIT
#define REPLICATION_ENDIAN_64BIT            (1) // BIG 64bit
#else
#define REPLICATION_ENDIAN_64BIT            (2) // BIG 32bit
#endif /* COMPILE_64BIT */

#else

#ifdef COMPILE_64BIT
#define REPLICATION_ENDIAN_64BIT            (3) // LITTLE 64bit
#else
#define REPLICATION_ENDIAN_64BIT            (4) // LITTLE 32bit
#endif /* COMPILE_64BIT */

#endif /* ENDIAN_IS_BIG_ENDIAN */


/* Handshake Ack의 Flag 상태 비트 */
// 1번 비트 : Endian bit : 0 - Big Endian, 1 - Little Endian
#define ULA_ACK_LITTLE_ENDIAN               (0x00000001)
#define ULA_ACK_BIG_ENDIAN                  (0x00000000)
#define ULA_ACK_ENDIAN_MASK                 (0x00000001)

#define ULA_LOB_DUMMY_HEADER_LEN            (1)

#define ULA_WAKEUP_PEER_SENDER_FLAG_SET     (0x00000004)
#define ULA_WAKEUP_PEER_SENDER_FLAG_UNSET   (0x00000000)

typedef enum
{
    ULA_SOCKET_TYPE_NONE = -1,
    ULA_SOCKET_TYPE_TCP  = 0,
    ULA_SOCKET_TYPE_UNIX = 1
} ULA_SOCKET_TYPE;

typedef enum
{
    ULA_MSG_OK            = 0,
    ULA_MSG_DISCONNECT    = 1,  /* Not used for XLog Sender */
    ULA_MSG_DENY          = 2,  /* Not used for XLog Sender */
    ULA_MSG_NOK           = 3,  /* Not used for XLog Sender */
    ULA_MSG_PROTOCOL_DIFF = 4,
    ULA_MSG_META_DIFF     = 5,
    ULA_MSG_SELF_REPL     = 6   /* Not used for XLog Sender */
} ulaHandshakeResult;

typedef struct ulaXLogCollectorStatus
{
    acp_char_t   mMyIP[ULA_IP_LEN];              /* [TCP] XLog Collector IP */
    acp_sint32_t mMyPort;                        /* [TCP] XLog Collector Port */
    acp_char_t   mPeerIP[ULA_IP_LEN];            /* [TCP] XLog Sender IP */
    acp_sint32_t mPeerPort;                      /* [TCP] XLog Sender Port */
    acp_char_t   mSocketFile[ULA_SOCK_NAME_LEN]; /* [UNIX] Socket File Name */
    acp_uint32_t mXLogCountInPool;               /* XLog Count in XLog Pool */
    ulaSN        mLastArrivedSN;                 /* Last Arrived SN */
    ulaSN        mLastProcessedSN;               /* Last Processed SN */
    acp_bool_t   mNetworkValid;                  /* Network is valid? */
} ulaXLogCollectorStatus;


typedef struct ulaXLogCollector
{
    /* TLSF Memory */
    acl_mem_alloc_t * mMemAllocator;

    /* XLog Pool */
    acl_mem_pool_t    mXLogPool;
    acp_thr_mutex_t   mXLogPoolMutex;
    acp_uint32_t      mXLogPoolLimit;
    acp_sint32_t      mXLogFreeCount;

    /* Transaction Table */
    ulaTransTbl       mTransTbl;
    acp_bool_t        mUseCommittedTxBuffer;

    /* XLog Queue */
    ulaXLogLinkedList mXLogQueue;

    /* Network */
    acp_thr_mutex_t   mAuthInfoMutex;
    ULA_SOCKET_TYPE   mSocketType;

    // TCP Socket
    acp_sint32_t      mTCPPort;
    acp_sint32_t      mTCPIpStack;
    acp_uint32_t      mXLogSenderIPCount;
    acp_char_t        mXLogSenderIP[ULA_MAX_AUTH_INFO_SIZE][ULA_IP_LEN];
    acp_sint32_t      mXLogSenderPort[ULA_MAX_AUTH_INFO_SIZE];

    // UNIX Domain Socket
    acp_char_t   mUNIXSocketFile[ULA_SOCK_NAME_LEN];

    acp_uint32_t mACKPerXLogCount;

    acp_uint32_t mHandshakeTimeoutSec;
    acp_uint32_t mReceiveXLogTimeoutSec;

    acp_bool_t   mNetworkExitFlag;   // receiveXLog()와 sendACK()에서 사용
    acp_bool_t   mSessionValid;      // handshake()와 finishNetwork()에서 사용

    cmiProtocolContext  mProtocolContext;
    cmiLink            *mPeerLink;          // cmiAcceptLink()에서 할당 받음

    acp_thr_mutex_t     mSendMutex;
    acp_thr_mutex_t     mReceiveMutex;

    // PROJ-1663 : IMPLICIT SAVEPOINT SET
    ulaXLog            *mRemainedXLog;

    /* Meta */
    ulaMeta mMeta;

    /* ACK 관련 수집 정보 */
    acp_thr_mutex_t     mAckMutex;

    // Restart SN 관련
    ulaSN    mRestartSN;            // getXLog(), sendACK()
    ulaSN    mLastCommitSN;         // COMMIT/ABORT/KEEP_ALIVE XLog의 SN (Flush)

    // GAP 관련
    ulaSN    mLastArrivedSN;        // receiveXLog(), sendACK()
    ulaSN    mLastProcessedSN;      // sendACK()

    // ACK 조건 관련
    acp_uint64_t    mProcessedXLogCount;   // sendACK()

    // ACK 전송 여부
    acp_bool_t   mStopACKArrived;
    acp_bool_t   mKeepAliveArrived;

    acp_char_t   mXLogSenderName[ULA_NAME_LEN];
} ulaXLogCollector;

/*
 * -----------------------------------------------------------------------------
 *  ulaXLogCollector Interface
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorInitialize(ulaXLogCollector  *aCollector,
                                  const acp_char_t  *aXLogSenderName,
                                  const acp_char_t  *aSocketInfo,
                                  acp_sint32_t       aXLogPoolSize,
                                  acp_bool_t         aUseCommittedTxBuffer,
                                  acp_uint32_t       aACKPerXLogCount,
                                  ulaErrorMgr       *aErrorMgr);

ACI_RC ulaXLogCollectorSetXLogPoolSize(ulaXLogCollector *aCollector, 
                                       acp_sint32_t      aXLogPoolSize,
                                       ulaErrorMgr       *aErrorMgr);

ACI_RC ulaXLogCollectorDestroy(ulaXLogCollector *aCollector,
                               ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorInitializeTCP(ulaXLogCollector *aCollector,
                                     const acp_char_t *aXLogSenderIP,
                                     acp_sint32_t      aXLogSenderPort,
                                     acp_sint32_t      aXLogCollectorPort,
                                     acp_sint32_t      aXLogCollectorIpStack,
                                     ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorInitializeUNIX(ulaXLogCollector *aCollector,
                                      ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorFinishNetwork(ulaXLogCollector *aCollector,
                                     ulaErrorMgr      *aErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Auth Info
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorAddAuthInfo(ulaXLogCollector *aCollector,
                                   const acp_char_t *aAuthInfo,
                                   ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorRemoveAuthInfo(ulaXLogCollector *aCollector,
                                      const acp_char_t *aAuthInfo,
                                      ulaErrorMgr      *aErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Timeout
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorSetHandshakeTimeout(ulaXLogCollector *aCollector,
                                           acp_uint32_t      aSecond,
                                           ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorSetReceiveXLogTimeout(ulaXLogCollector *aCollector,
                                             acp_uint32_t      aSecond,
                                             ulaErrorMgr      *aErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Wakeup Sender
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaWakeupPeerSender( ulaXLogCollector * aCollector, 
                            ulaErrorMgr      * aErrorMgr,
                            acp_char_t       * aSenderIP,
                            acp_sint32_t       aSenderPort );

ACI_RC ulaXLogCollectorCheckRemoteVersion( cmiProtocolContext * aProtocolContext,
                                           acp_uint32_t         aTimeout,
                                           ulaErrorMgr        * aErrorMgr,
                                           acp_char_t         * aMsg,
                                           acp_uint32_t       * aMsgLen );

/*
 * -----------------------------------------------------------------------------
 *  Handshake
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorHandshakeBefore(ulaXLogCollector *aCollector,
                                       ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorHandshake(ulaXLogCollector *aCollector,
                                 ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorCheckProtocol(ulaXLogCollector *aCollector,
                                     ulaVersion       *aVersion,
                                     ulaErrorMgr      *aErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  XLog
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorReceiveXLog(ulaXLogCollector *aCollector,
                                   acp_bool_t       *aOutInsertXLogInQueue,
                                   ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorGetXLog(ulaXLogCollector  *aCollector,
                               ulaXLog          **aOutXLog,
                               ulaErrorMgr       *aErrorMgr);

ACI_RC ulaXLogCollectorFreeXLogMemory(ulaXLogCollector *aCollector,
                                      ulaXLog          *aXLog,
                                      ulaErrorMgr      *aErrorMgr);

ACI_RC ulaXLogCollectorSendACK(ulaXLogCollector *aCollector,
                               ulaErrorMgr      *aErrorMgr);

/*
 * -----------------------------------------------------------------------------
 *  Meta
 * -----------------------------------------------------------------------------
 */
ulaMeta *ulaXLogCollectorGetMeta(ulaXLogCollector *aCollector);

/*
 * -----------------------------------------------------------------------------
 *  Status of XLogCollector instance
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaXLogCollectorGetXLogCollectorStatus
                        (ulaXLogCollector       *aCollector,
                         ulaXLogCollectorStatus *aOutXLogCollectorStatus,
                         ulaErrorMgr            *aErrorMgr);

void ulaXLogCollectorGetVersionFromAck( acp_char_t     *aMsg,
                                        acp_uint32_t    aMsgLen,
                                        acp_uint64_t   *aVersion );

acp_uint64_t ulaXLogCollectorNtoh( acp_uint64_t Value );

#endif /* _O_XLOG_COLLECTOR_H_ */
