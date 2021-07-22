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

#ifndef _O_CMI_CLIENT_H_
#define _O_CMI_CLIENT_H_ 1


/*
 * cmn Defines
 */

#define CMI_DIRECTION_RD                    CMN_DIRECTION_RD
#define CMI_DIRECTION_WR                    CMN_DIRECTION_WR
#define CMI_DIRECTION_RDWR                  CMN_DIRECTION_RDWR

#define CMI_DISPATCHER_IMPL_SOCK            CMN_DISPATCHER_IMPL_SOCK
#define CMI_DISPATCHER_IMPL_TCP             CMN_DISPATCHER_IMPL_SOCK
#define CMI_DISPATCHER_IMPL_UNIX            CMN_DISPATCHER_IMPL_SOCK
#define CMI_DISPATCHER_IMPL_IPC             CMN_DISPATCHER_IMPL_IPC
#define CMI_DISPATCHER_IMPL_IPCDA           CMN_DISPATCHER_IMPL_IPCDA
#define CMI_DISPATCHER_IMPL_IB              CMN_DISPATCHER_IMPL_IB
#define CMI_DISPATCHER_IMPL_BASE            CMN_DISPATCHER_IMPL_BASE
#define CMI_DISPATCHER_IMPL_MAX             CMN_DISPATCHER_IMPL_MAX
#define CMI_DISPATCHER_IMPL_INVALID         CMN_DISPATCHER_IMPL_INVALID

#define CMI_LINK_FEATURE_SYSDBA             CMN_LINK_FEATURE_SYSDBA

#define CMI_LINK_TYPE_LISTEN                CMN_LINK_TYPE_LISTEN
#define CMI_LINK_TYPE_PEER_SERVER           CMN_LINK_TYPE_PEER_SERVER
#define CMI_LINK_TYPE_PEER_CLIENT           CMN_LINK_TYPE_PEER_CLIENT
#define CMI_LINK_TYPE_BASE                  CMN_LINK_TYPE_BASE
#define CMI_LINK_TYPE_MAX                   CMN_LINK_TYPE_MAX
#define CMI_LINK_TYPE_INVALID               CMN_LINK_TYPE_INVALID

#define CMI_LINK_IMPL_TCP                   CMN_LINK_IMPL_TCP
#define CMI_LINK_IMPL_UNIX                  CMN_LINK_IMPL_UNIX
#define CMI_LINK_IMPL_IPC                   CMN_LINK_IMPL_IPC
#define CMI_LINK_IMPL_IPCDA                 CMN_LINK_IMPL_IPCDA  /* PROJ-2616 */
#define CMI_LINK_IMPL_SSL                   CMN_LINK_IMPL_SSL    /* PROJ-2474 SSL/TLS */
#define CMI_LINK_IMPL_IB                    CMN_LINK_IMPL_IB     /* PROJ-2681 */

#define CMI_LINK_IMPL_BASE                  CMN_LINK_IMPL_BASE
#define CMI_LINK_IMPL_MAX                   CMN_LINK_IMPL_MAX
#define CMI_LINK_IMPL_INVALID               CMN_LINK_IMPL_INVALID

#define CMI_LINK_INFO_ALL                   CMN_LINK_INFO_ALL
#define CMI_LINK_INFO_IMPL                  CMN_LINK_INFO_IMPL
#define CMI_LINK_INFO_LOCAL_ADDRESS         CMN_LINK_INFO_LOCAL_ADDRESS
#define CMI_LINK_INFO_LOCAL_IP_ADDRESS      CMN_LINK_INFO_LOCAL_IP_ADDRESS
#define CMI_LINK_INFO_LOCAL_PORT            CMN_LINK_INFO_LOCAL_PORT
#define CMI_LINK_INFO_REMOTE_ADDRESS        CMN_LINK_INFO_REMOTE_ADDRESS
#define CMI_LINK_INFO_REMOTE_IP_ADDRESS     CMN_LINK_INFO_REMOTE_IP_ADDRESS
#define CMI_LINK_INFO_REMOTE_PORT           CMN_LINK_INFO_REMOTE_PORT
#define CMI_LINK_INFO_REMOTE_SOCKADDR       CMN_LINK_INFO_REMOTE_SOCKADDR
#define CMI_LINK_INFO_UNIX_PATH             CMN_LINK_INFO_UNIX_PATH
#define CMI_LINK_INFO_IPC_KEY               CMN_LINK_INFO_IPC_KEY
#define CMI_LINK_INFO_IPCDA_KEY             CMN_LINK_INFO_IPCDA_KEY
/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#define CMI_LINK_INFO_TCP_KERNEL_STAT       CMN_LINK_INFO_TCP_KERNEL_STAT

#define CMI_BLOCK_DEFAULT_SIZE              CMB_BLOCK_DEFAULT_SIZE

/* PROJ-2733-Protocol cmpDefDBClient.h */
#define CMI_IS_CONNECT_GROUP                CMP_IS_CONNECT_GROUP
#define CMI_IS_PROPERTY_SET_GROUP           CMP_IS_PROPERTY_SET_GROUP
#define CMI_IS_PROPERTY_GET_GROUP           CMP_IS_PROPERTY_GET_GROUP
#define CMI_IS_PARAM_DATA_IN_LIST_GROUP     CMP_IS_PARAM_DATA_IN_LIST_GROUP
#define CMI_IS_EXECUTE_GROUP                CMP_IS_EXECUTE_GROUP
#define CMI_IS_FETCH_GROUP                  CMP_IS_FETCH_GROUP
#define CMI_IS_TRANSACTION_GROUP            CMP_IS_TRANSACTION_GROUP
#define CMI_IS_SHARD_TRANSACTION_GROUP      CMP_IS_SHARD_TRANSACTION_GROUP
#define CMI_IS_SHARD_PREPARE_GROUP          CMP_IS_SHARD_PREPARE_GROUP
#define CMI_IS_SHARD_END_PENDING_GROUP      CMP_IS_SHARD_END_PENDING_GROUP
#define CMI_IS_PREPARE_GROUP                CMP_IS_PREPARE_GROUP

#ifndef SO_NONE
#define SO_NONE 0
#endif

/*PROJ-2616*/
#define CMI_IPCDA_SPIN_MAX_LOOP         100000
#define CMI_IPCDA_SPIN_MIN_LOOP         1

#if defined(ALTI_CFG_OS_LINUX)
/* Maximum size of a whole message queue */
#define CMI_IPCDA_MESSAGEQ_MAX_MESSAGE   10
/* Size of a message queue segment */
#define CMI_IPCDA_MESSAGEQ_MESSAGE_SIZE  1  
/* server의 응답이 전송되었음을 message queue로 알린다. */
#define CMI_IPCDA_MESSAGEQ_NOTIFY    1
#endif
/*
 * cmn Types
 */
typedef struct cmnDispatcher     cmiDispatcher;
typedef struct cmnLink           cmiLink;
typedef union  cmnLinkListenArg  cmiListenArg;
typedef union  cmnLinkConnectArg cmiConnectArg;
typedef        cmnDirection      cmiDirection;
typedef        cmnLinkType       cmiLinkType;
typedef        cmnLinkImpl       cmiLinkImpl;
typedef        cmnDispatcherImpl cmiDispatcherImpl;
typedef        cmnLinkInfoKey    cmiLinkInfoKey;

/*
 * cmm Types
 */

typedef struct cmmSession cmiSession;

/*
 * cmp Types
 */

typedef struct cmpProtocol cmiProtocol;

typedef cmpCallbackFunction cmiCallbackFunction;

/*
 * cmp protocol macros
 */

#define CMI_PROTOCOL_MODULE(aModule)         CMP_MODULE_ ## aModule
#define CMI_PROTOCOL_OPERATION(aModule, aOp) CMP_OP_ ## aModule ## _ ## aOp
#define CMI_PROTOCOL(aModule, aOp)           CMI_PROTOCOL_MODULE(aModule), CMI_PROTOCOL_OPERATION(aModule, aOp)

#define CMI_PROTOCOL_GET_ARG(aProtocol, aModule, aOp) &((aProtocol).mArg.m ## aModule.m ## aOp)

#define CMI_PROTOCOL_INITIALIZE(aProtocol, aArg, aModule, aOp)                  \
    do                                                                          \
    {                                                                           \
        ACI_TEST(cmiInitializeProtocol(&(aProtocol),                            \
                                       &(gCmpModuleClient ## aModule),          \
                                       CMI_PROTOCOL_OPERATION(aModule, aOp))    \
                 != ACP_RC_SUCCESS);                                            \
                                                                                \
        aArg = CMI_PROTOCOL_GET_ARG(aProtocol, aModule, aOp);                   \
    } while (0)

/* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
typedef enum cmiCompressType
{
    CMI_COMPRESS_NONE       = 0,
    CMI_COMPRESS_LZO        = 1,
    CMI_COMPRESS_LZ4        = 2,
    CMI_COMPRESS_MAX        
} cmiCompressType;

/* cmiProtocolContext */
typedef struct cmiProtocolContext
{
    struct cmnLinkPeer           *mLink;
    struct cmmSession            *mSession;
    struct cmpModule             *mModule;

    struct cmbBlock              *mReadBlock;
    struct cmbBlock              *mWriteBlock;
    acp_bool_t                    mIsAddReadBlock;

    cmpHeader                     mReadHeader;
    cmpHeader                     mWriteHeader;

    cmpMarshalState               mMarshalState;
    cmpProtocol                   mProtocol;

    acp_list_t                    mReadBlockList;
    acp_list_t                    mWriteBlockList;

    /*
     * BUG-19465 : CM_Buffer의 pending list를 제한
     */
    acp_uint32_t                  mListLength; /* BUG-44468 [cm] codesonar warning in CM */

    acp_uint32_t                  mCmSeqNo;

    acp_bool_t                    mIsDisconnect;

    /* proj_2160 cm_type removal */
    void                         *mOwner;
    acp_bool_t                    mSessionCloseNeeded;

    /* BUG-47078 */
    acp_uint32_t                  mCompressLevel;
    /* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지  */
    /* send시 압축을 하지 않아 CompressType이 NONE이어도 상대가 압축을 해서 보낼 경우 */ 
    /* recv시에는 어떤 타입으로 압축을 풀어야 될지 설정되어 있어야 하기 때문에 */
    /* CompressType과 DecompressType을 별도로 사용합니다. */
    cmiCompressType               mCompressType;
    cmiCompressType               mDecompressType;

    /* PROJ-2616 */
    cmbBlockSimpleQueryFetchIPCDA mSimpleQueryFetchIPCDAReadBlock;
    acp_uint32_t                  mIPDASpinMaxCount;
} cmiProtocolContext;

/*
 * Initialization
 */

/*
 * BUG-19465 : CM_Buffer의 pending list를 제한
 */
ACI_RC cmiInitialize( acp_uint32_t aCmMaxPendingList );
ACI_RC cmiFinalize();
void   cmiDestroy(); // BUG-39147

ACI_RC cmiSetCallback(acp_uint8_t aModuleID, acp_uint8_t aOpID, cmiCallbackFunction aCallbackFunction);

/*
 * Check Network Implementation Support
 */

acp_bool_t cmiIsSupportedLinkImpl(cmiLinkImpl aLinkImpl);
acp_bool_t cmiIsSupportedDispatcherImpl(cmiDispatcherImpl aDispatcherImpl);

/*
 * Link
 */

ACI_RC cmiAllocLink(cmiLink **aLink, cmiLinkType aType, cmiLinkImpl aImpl);
ACI_RC cmiFreeLink(cmiLink *aLink);
ACI_RC cmiCloseLink(cmiLink *aLink);

ACI_RC cmiWaitLink(cmiLink *aLink, acp_time_t aTimeout);
ACI_RC cmiListenLink(cmiLink *aLink, cmiListenArg *aListenArg);
ACI_RC cmiAcceptLink(cmiLink *aLinkListen, cmiLink **aLinkPeer);

/*
 * Peer Link
 */

ACI_RC cmiGetLinkInfo(cmiLink *aLink, acp_char_t *aBuf, acp_uint32_t aBufLen, cmiLinkInfoKey aKey);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmiGetLinkSndBufSize(cmiLink *aLink, acp_sint32_t *aSndBufSize);
ACI_RC cmiSetLinkSndBufSize(cmiLink *aLink, acp_sint32_t  aSndBufSize);
ACI_RC cmiGetLinkRcvBufSize(cmiLink *aLink, acp_sint32_t *aRcvBufSize);
ACI_RC cmiSetLinkRcvBufSize(cmiLink *aLink, acp_sint32_t  aRcvBufSize);

ACI_RC cmiCheckLink(cmiLink *aLink, acp_bool_t *aIsClosed);

ACI_RC cmiShutdownLink(cmiLink *aLink, cmiDirection aDirection);

/*
 * Session
 */

ACI_RC cmiAddSession(cmiSession *aSession, void *aOwner, acp_uint8_t aModuleID, cmiProtocolContext *aProtocolContext);
ACI_RC cmiRemoveSession(cmiSession *aSession);

ACI_RC cmiSetLinkForSession(cmiSession *aSession, cmiLink *aLink);
ACI_RC cmiGetLinkForSession(cmiSession *aSession, cmiLink **aLink);

ACI_RC cmiConnect(cmiProtocolContext *aCtx, cmiConnectArg *aConnectArg, acp_time_t aTimeout, acp_sint32_t aOption);
ACI_RC cmiConnectWithoutData(cmiProtocolContext *aCtx, cmiConnectArg *aConnectArg, acp_time_t aTimeout, acp_sint32_t aOption);

/*
 * Protocol Initialize
 */
/*
 * fix BUG-17947.
 */
ACI_RC cmiInitializeProtocol(cmiProtocol *aProtocol,cmpModule*  aModule, acp_uint8_t aOperationID);
/*fix BUG-30041 cmiReadProtocol에서 mFinalization 이초기화 되기전에
 실패하는 case에 cmiFinalization에서 비정상종료됩니다.*/
void  cmiInitializeProtocolNullFinalization(cmiProtocol *aProtocol);
ACI_RC cmiFinalizeProtocol(cmiProtocol *aProtocol);

/*
 * Protocol Context
 */

void   cmiSetProtocolContextLink(cmiProtocolContext *aProtocolContext, cmiLink *aLink);

acp_bool_t cmiProtocolContextHasPendingRequest(cmiProtocolContext *aProtocolContext);

/*
 * Read Protocol and Callback
 */

ACI_RC cmiReadProtocolAndCallback(cmiProtocolContext   *aProtocolContext,
                                  void                 *aUserContext,
                                  acp_time_t            aTimeout);

/*
 * Read One Protocol from Protocol Context
 */

ACI_RC cmiReadProtocol(cmiProtocolContext *aProtocolContext, cmiProtocol *aProtocol, acp_time_t aTimeout, acp_bool_t *aIsEnd);

/*
 * Write One Protocol to Protocol Context
 */

ACI_RC cmiWriteProtocol(cmiProtocolContext *aProtocolContext, cmiProtocol *aProtocol);

/*
 * Flush Written Protocol
 */

ACI_RC cmiFlushProtocol(cmiProtocolContext *aProtocolContext, acp_bool_t aIsEnd);


/*
 * fix BUG-17715
 */
ACI_RC cmiCheckFetch(cmiProtocolContext *aProtocolContext, acp_uint32_t aRecordSize);

acp_bool_t cmiCheckInVariable(cmiProtocolContext *aProtocolContext, acp_uint32_t aInVariableSize);
acp_bool_t cmiCheckInBinary(cmiProtocolContext *aProtocolContext, acp_uint32_t aInBinarySize);
acp_bool_t cmiCheckInBit(cmiProtocolContext *aProtocolContext, acp_uint32_t aInBitSize);
acp_bool_t cmiCheckInNibble(cmiProtocolContext *aProtocolContext, acp_uint32_t aInNibbleSize);

acp_uint8_t *cmiGetOpName( acp_uint32_t aModuleID, acp_uint32_t aOpID );

cmiLinkImpl cmiGetLinkImpl(cmiProtocolContext *aProtocolContext);

/*
 * bug-19279 remote sysdba enable + sys can kill session
 */
acp_bool_t cmiIsRemoteAccess(cmiLink* aLink);

ACI_RC cmiIPCDACheckReadFlag(void *aCtx, void *aBlock, acp_uint32_t aMicroSleepTime, acp_uint32_t  aExpireCount);
void   cmiLinkPeerFinalizeCliReadForIPCDA(cmiProtocolContext *aCtx);

/***********************************************************
 * proj_2160 cm_type removal
 *  new marshaling interface used in MM/CLI for A7 or higher
 * CMI_RD: recv: read  mReadBlock --> buffer
 * CMI_WR: send: write buffer     --> mWriteBlock
***********************************************************/

#ifdef ENDIAN_IS_BIG_ENDIAN

#define CM_ENDIAN_ASSIGN1(dst, src) \
    (dst) = (src)

#define CM_ENDIAN_ASSIGN2(dst, src)                                \
    do                                                             \
    {                                                              \
        *((acp_uint8_t *)(dst) + 0) = *((acp_uint8_t *)(src) + 0); \
        *((acp_uint8_t *)(dst) + 1) = *((acp_uint8_t *)(src) + 1); \
    } while (0)

#define CM_ENDIAN_ASSIGN4(dst, src)                                \
    do                                                             \
    {                                                              \
        *((acp_uint8_t *)(dst) + 0) = *((acp_uint8_t *)(src) + 0); \
        *((acp_uint8_t *)(dst) + 1) = *((acp_uint8_t *)(src) + 1); \
        *((acp_uint8_t *)(dst) + 2) = *((acp_uint8_t *)(src) + 2); \
        *((acp_uint8_t *)(dst) + 3) = *((acp_uint8_t *)(src) + 3); \
    } while (0)

#define CM_ENDIAN_ASSIGN8(dst, src)                                \
    do                                                             \
    {                                                              \
        *((acp_uint8_t *)(dst) + 0) = *((acp_uint8_t *)(src) + 0); \
        *((acp_uint8_t *)(dst) + 1) = *((acp_uint8_t *)(src) + 1); \
        *((acp_uint8_t *)(dst) + 2) = *((acp_uint8_t *)(src) + 2); \
        *((acp_uint8_t *)(dst) + 3) = *((acp_uint8_t *)(src) + 3); \
        *((acp_uint8_t *)(dst) + 4) = *((acp_uint8_t *)(src) + 4); \
        *((acp_uint8_t *)(dst) + 5) = *((acp_uint8_t *)(src) + 5); \
        *((acp_uint8_t *)(dst) + 6) = *((acp_uint8_t *)(src) + 6); \
        *((acp_uint8_t *)(dst) + 7) = *((acp_uint8_t *)(src) + 7); \
    } while (0)

#else /* LITTLE_ENDIAN */

#define CM_ENDIAN_ASSIGN1(dst, src) \
    (dst) = (src)

#define CM_ENDIAN_ASSIGN2(dst, src)                                \
    do                                                             \
    {                                                              \
        *((acp_uint8_t *)(dst) + 1) = *((acp_uint8_t *)(src) + 0); \
        *((acp_uint8_t *)(dst) + 0) = *((acp_uint8_t *)(src) + 1); \
    } while (0)

#define CM_ENDIAN_ASSIGN4(dst, src)                                \
    do                                                             \
    {                                                              \
        *((acp_uint8_t *)(dst) + 3) = *((acp_uint8_t *)(src) + 0); \
        *((acp_uint8_t *)(dst) + 2) = *((acp_uint8_t *)(src) + 1); \
        *((acp_uint8_t *)(dst) + 1) = *((acp_uint8_t *)(src) + 2); \
        *((acp_uint8_t *)(dst) + 0) = *((acp_uint8_t *)(src) + 3); \
    } while (0)

#define CM_ENDIAN_ASSIGN8(dst, src)                                \
    do                                                             \
    {                                                              \
        *((acp_uint8_t *)(dst) + 7) = *((acp_uint8_t *)(src) + 0); \
        *((acp_uint8_t *)(dst) + 6) = *((acp_uint8_t *)(src) + 1); \
        *((acp_uint8_t *)(dst) + 5) = *((acp_uint8_t *)(src) + 2); \
        *((acp_uint8_t *)(dst) + 4) = *((acp_uint8_t *)(src) + 3); \
        *((acp_uint8_t *)(dst) + 3) = *((acp_uint8_t *)(src) + 4); \
        *((acp_uint8_t *)(dst) + 2) = *((acp_uint8_t *)(src) + 5); \
        *((acp_uint8_t *)(dst) + 1) = *((acp_uint8_t *)(src) + 6); \
        *((acp_uint8_t *)(dst) + 0) = *((acp_uint8_t *)(src) + 7); \
    } while (0)

#endif /* ENDIAN_IS_BIG_ENDIAN */

ACP_INLINE void cmEndianAssign2( acp_uint16_t* aDst, acp_uint16_t* aSrc)
{
    CM_ENDIAN_ASSIGN2(aDst, aSrc);
}

ACP_INLINE void cmEndianAssign4( acp_uint32_t* aDst, acp_uint32_t* aSrc)
{
    CM_ENDIAN_ASSIGN4(aDst, aSrc);
}

ACP_INLINE void cmEndianAssign8( acp_uint64_t* aDst, acp_uint64_t* aSrc)
{
    CM_ENDIAN_ASSIGN8(aDst, aSrc);
}

ACP_INLINE void cmNoEndianAssign2( acp_uint16_t* aDst, acp_uint16_t* aSrc)
{
    *((acp_uint8_t *)(aDst) + 0) = *((acp_uint8_t *)(aSrc) + 0);
    *((acp_uint8_t *)(aDst) + 1) = *((acp_uint8_t *)(aSrc) + 1);
}

ACP_INLINE void cmNoEndianAssign4( acp_uint32_t* aDst, acp_uint32_t* aSrc)
{
    *((acp_uint8_t *)(aDst) + 0) = *((acp_uint8_t *)(aSrc) + 0);
    *((acp_uint8_t *)(aDst) + 1) = *((acp_uint8_t *)(aSrc) + 1);
    *((acp_uint8_t *)(aDst) + 2) = *((acp_uint8_t *)(aSrc) + 2);
    *((acp_uint8_t *)(aDst) + 3) = *((acp_uint8_t *)(aSrc) + 3);
}

ACP_INLINE void cmNoEndianAssign8( acp_uint64_t* aDst, acp_uint64_t* aSrc)
{
    *((acp_uint8_t *)(aDst) + 0) = *((acp_uint8_t *)(aSrc) + 0);
    *((acp_uint8_t *)(aDst) + 1) = *((acp_uint8_t *)(aSrc) + 1);
    *((acp_uint8_t *)(aDst) + 2) = *((acp_uint8_t *)(aSrc) + 2);
    *((acp_uint8_t *)(aDst) + 3) = *((acp_uint8_t *)(aSrc) + 3);
    *((acp_uint8_t *)(aDst) + 4) = *((acp_uint8_t *)(aSrc) + 4);
    *((acp_uint8_t *)(aDst) + 5) = *((acp_uint8_t *)(aSrc) + 5);
    *((acp_uint8_t *)(aDst) + 6) = *((acp_uint8_t *)(aSrc) + 6);
    *((acp_uint8_t *)(aDst) + 7) = *((acp_uint8_t *)(aSrc) + 7);
}

/* read */
/* 블럭에 직접 데이타를 읽고, 쓰기 위해서 사용된다. Align 을 맞추지 않고 1바이트씩 복사한다. */
#define CMI_RD1(aCtx, aDst)                                                                    \
    do                                                                                         \
    {                                                                                          \
        ACE_DASSERT( sizeof(aDst) == 1 );                                                      \
        CM_ENDIAN_ASSIGN1((aDst), *((aCtx)->mReadBlock->mData + (aCtx)->mReadBlock->mCursor)); \
        (aCtx)->mReadBlock->mCursor += 1;                                                      \
    } while (0)

#define CMI_RD2(aCtx, aDst)                                                                                 \
    do                                                                                                      \
    {                                                                                                       \
        ACE_DASSERT( sizeof(*aDst) == 2 );                                                                  \
        cmEndianAssign2((aDst), (acp_uint16_t *)((aCtx)->mReadBlock->mData + (aCtx)->mReadBlock->mCursor)); \
        (aCtx)->mReadBlock->mCursor += 2;                                                                   \
    } while (0)

#define CMI_RD4(aCtx, aDst)                                                                                 \
    do                                                                                                      \
    {                                                                                                       \
        ACE_DASSERT( sizeof(*aDst) == 4 );                                                                  \
        cmEndianAssign4((aDst), (acp_uint32_t *)((aCtx)->mReadBlock->mData + (aCtx)->mReadBlock->mCursor)); \
        (aCtx)->mReadBlock->mCursor += 4;                                                                   \
    } while (0)

#define CMI_RD8(aCtx, aDst)                                                                                 \
    do                                                                                                      \
    {                                                                                                       \
        ACE_DASSERT( sizeof(*aDst) == 8 );                                                                  \
        cmEndianAssign8((aDst), (acp_uint64_t *)((aCtx)->mReadBlock->mData + (aCtx)->mReadBlock->mCursor)); \
        (aCtx)->mReadBlock->mCursor += 8;                                                                   \
    } while (0)

#define CMI_RCP(aCtx, aDst, aLen)                                                             \
    do                                                                                        \
    {                                                                                         \
        if (aLen > 0)                                                                         \
        {                                                                                     \
            acpMemCpy((aDst), (aCtx)->mReadBlock->mData + (aCtx)->mReadBlock->mCursor, aLen); \
            (aCtx)->mReadBlock->mCursor += aLen;                                              \
        }                                                                                     \
    } while (0)

/* write */
#define CMI_WOP(aCtx, aOp)                                                                      \
    do                                                                                          \
    {                                                                                           \
        CM_ENDIAN_ASSIGN1(*((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aOp)); \
        (aCtx)->mWriteBlock->mCursor += 1;                                                      \
    } while (0)

#define CMI_WR1(aCtx, aSrc)                                                                      \
    do                                                                                           \
    {                                                                                            \
        ACE_DASSERT( (sizeof(aSrc) == 1) || (aSrc <= UCHAR_MAX) );                               \
        CM_ENDIAN_ASSIGN1(*((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc)); \
        (aCtx)->mWriteBlock->mCursor += 1;                                                       \
    } while (0)

#define CMI_WR2(aCtx, aSrc)                                                                                   \
    do                                                                                                        \
    {                                                                                                         \
        ACE_DASSERT( sizeof(*aSrc) == 2 );                                                                    \
        cmEndianAssign2((acp_uint16_t *)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc)); \
        (aCtx)->mWriteBlock->mCursor += 2;                                                                    \
    } while (0)

#define CMI_WR4(aCtx, aSrc)                                                                                   \
    do                                                                                                        \
    {                                                                                                         \
        ACE_DASSERT( sizeof(*aSrc) == 4 );                                                                    \
        cmEndianAssign4((acp_uint32_t *)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc)); \
        (aCtx)->mWriteBlock->mCursor += 4;                                                                    \
    } while (0)

#define CMI_WR8(aCtx, aSrc)                                                                                   \
    do                                                                                                        \
    {                                                                                                         \
        ACE_DASSERT( sizeof(*aSrc) == 8 );                                                                    \
        cmEndianAssign8((acp_uint64_t *)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc)); \
        (aCtx)->mWriteBlock->mCursor += 8;                                                                    \
    } while (0)

#define CMI_NE_WR2(aCtx, aSrc)                                                                                  \
    do {                                                                                                        \
        cmNoEndianAssign2((acp_uint16_t *)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc)); \
        (aCtx)->mWriteBlock->mCursor += 2;                                                                      \
    } while (0)

#define CMI_NE_WR4(aCtx, aSrc)                                                                                   \
    do {                                                                                                         \
        cmNoEndianAssign4((acp_uint32_t *)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc));  \
        (aCtx)->mWriteBlock->mCursor += 4;                                                                       \
    } while (0)

#define CMI_NE_WR8(aCtx, aSrc)                                                                                  \
    do {                                                                                                        \
        cmNoEndianAssign8((acp_uint64_t *)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor), (aSrc)); \
        (aCtx)->mWriteBlock->mCursor += 8;                                                                      \
    } while (0)

#define CMI_WCP(aCtx, aSrc, aLen)                                                                 \
    do                                                                                            \
    {                                                                                             \
        if ((aLen) > 0)                                                                           \
        {                                                                                         \
            ACI_TEST((aCtx)->mWriteBlock->mCursor + (aLen) > (aCtx)->mWriteBlock->mBlockSize);    \
            acpMemCpy((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor, (aSrc), (aLen)); \
            (aCtx)->mWriteBlock->mCursor += (aLen);                                               \
        }                                                                                         \
    } while (0)

#define CMI_WRITE_ALIGN8(aCtx) \
    ((aCtx)->mWriteBlock->mCursor) += (acp_ulong_t)ACP_ALIGN8_PTR((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor) - (acp_ulong_t)((aCtx)->mWriteBlock->mData + (aCtx)->mWriteBlock->mCursor)

/* CMI_WRITE_CHECK
 * aLen 의 길이만큼의 데이타를 1개의 블락에 넣을수 있는지 체크한다.
 * 프로토콜이 분할되지 않는 영역을 보장하기 위해서 사용된다.*/
#define CMI_WRITE_CHECK(aCtx, aLen)                                                                                    \
    do {                                                                                                               \
        if (cmiGetLinkImpl(aCtx) != CMN_LINK_IMPL_IPCDA)                                                               \
        {                                                                                                              \
            ACI_TEST((aLen) > CMB_BLOCK_DEFAULT_SIZE - CMP_HEADER_SIZE);                                               \
            if ((aCtx)->mWriteBlock->mCursor + (aLen) > CMB_BLOCK_DEFAULT_SIZE)                                        \
            {                                                                                                          \
                ACI_TEST(cmiSend((aCtx), ACP_FALSE) != ACI_SUCCESS);                                                   \
            }                                                                                                          \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ACI_TEST(cmiReadyToWriteBufferIPCDA(aCtx) == ACI_FAILURE);                                                 \
            ACI_TEST((aCtx)->mWriteBlock->mCursor + (aLen) > CMB_BLOCK_DEFAULT_SIZE);                              \
        }                                                                                                              \
    } while (0);

/* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
/* CMI_WRITE_CHECK_WITH_IPCDA
 * aLen 의 길이만큼의 데이타를 1개의 블락에 넣을수 있는지 체크한다.
 * 프로토콜이 분할되지 않는 영역을 보장하기 위해서 사용된다.
 * IPCDA일경우 size가 다른경우가 존재하기 때문에 추가 인자를 받아서 처리한다. */
#define CMI_WRITE_CHECK_WITH_IPCDA(aCtx, aLen, aLenIPCDA)                                                              \
    do {                                                                                                               \
        if (cmiGetLinkImpl(aCtx) != CMN_LINK_IMPL_IPCDA)                                                               \
        {                                                                                                              \
            ACI_TEST((aLen) > CMB_BLOCK_DEFAULT_SIZE - CMP_HEADER_SIZE);                                               \
            if ((aCtx)->mWriteBlock->mCursor + (aLen) > CMB_BLOCK_DEFAULT_SIZE)                                        \
            {                                                                                                          \
                ACI_TEST(cmiSend((aCtx), ACP_FALSE) != ACI_SUCCESS);                                                   \
            }                                                                                                          \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            ACI_TEST(cmiReadyToWriteBufferIPCDA(aCtx) == ACI_FAILURE);                                                 \
            ACI_TEST((aCtx)->mWriteBlock->mCursor + (aLenIPCDA) > CMB_BLOCK_DEFAULT_SIZE);                         \
        }                                                                                                              \
    } while (0);

#define CMI_IS_READ_ALL(aCtx) \
        (((aCtx)->mReadBlock->mDataSize == (aCtx)->mReadBlock->mCursor) ? ACP_TRUE : ACP_FALSE)

#define CMI_REMAIN_SPACE_IN_WRITE_BLOCK(aCtx) \
        ((aCtx)->mWriteBlock->mBlockSize - (aCtx)->mWriteBlock->mCursor)

#define CMI_REMAIN_DATA_IN_READ_BLOCK(aCtx) \
        ((aCtx)->mReadBlock->mDataSize - (aCtx)->mReadBlock->mCursor)

#define CMI_SKIP_READ_BLOCK(aCtx, aByte) \
        (((aCtx)->mReadBlock->mCursor) += (aByte))

/* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
#define CMI_SET_CURSOR(aCtx , aPos) \
        (((aCtx)->mWriteBlock->mCursor) = (aPos))

#define CMI_GET_CURSOR(aCtx) \
        ((aCtx)->mWriteBlock->mCursor)

/*********************************************************/
// proj_2160 cm_type removal
// cmpHeaderRead is put here because of a compile error
ACI_RC cmpHeaderInitialize(cmpHeader *aHeader);
ACI_RC cmpHeaderRead(cmnLinkPeer *aLink, cmpHeader *aHeader, cmbBlock *aBlock);
ACI_RC cmpHeaderWrite(cmpHeader *aHeader, cmbBlock *aBlock);

ACI_RC cmiMakeCmBlockNull(cmiProtocolContext *aCtx);
ACI_RC cmiAllocCmBlock(cmiProtocolContext* aCtx,
                       acp_uint8_t         aModuleCM,
                       cmiLink*            aLink,
                       void*               aOwner);
ACI_RC cmiFreeCmBlock(cmiProtocolContext* aCtx);
ACI_RC cmiRecv(cmiProtocolContext* aCtx,
               void*           aUserContext,
               acp_time_t      aTimeout);
ACI_RC cmiRecvIPCDA(cmiProtocolContext *aCtx,
                    void               *aUserContext,
                    acp_time_t          aTimeout,
                    acp_uint32_t        aMicroSleepTime);
ACI_RC cmiRecvNext(cmiProtocolContext* aCtx, acp_time_t aTimeout);
ACI_RC cmiSend(cmiProtocolContext *aCtx, acp_bool_t aIsEnd);
ACI_RC cmiCheckAndFlush( cmiProtocolContext * aProtocolContext, acp_uint32_t aLen, acp_bool_t aIsEnd );

ACI_RC cmiSplitRead( cmiProtocolContext *aCtx,
                     acp_uint64_t        aReadSize,
                     acp_uint8_t        *aBuffer,
                     acp_time_t          aTimeout );

ACI_RC cmiSplitSkipRead( cmiProtocolContext *aCtx,
                         acp_uint64_t        aReadSize,
                         acp_time_t          aTimeout );

ACI_RC cmiSplitWrite( cmiProtocolContext *aCtx,
                      acp_uint64_t        aWriteSize,
                      acp_uint8_t        *aBuffer );

/* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
void cmiEnableCompress( cmiProtocolContext * aCtx,
                        acp_uint32_t         aLevel,
                        cmiCompressType      aCompressType );
void cmiDisableCompress( cmiProtocolContext * aCtx );
void cmiSetDecompressType( cmiProtocolContext * aCtx,
                           cmiCompressType      aDecompressType );

/* PROJ-2616*/
ACP_INLINE void cmiIPCDAIncDataCount( cmiProtocolContext *aCtx )
{
    /* 현재의 데이터 커서의 위치를 데이터 사이즈로 갱신*/
    aCtx->mWriteBlock->mDataSize = aCtx->mWriteBlock->mCursor;
    /* BUG-46502 atomic 함수 적용  atomic에 mem_barrier가 포함되어 있다. */
    acpAtomicInc32( &(((cmbBlockIPCDA*)aCtx->mWriteBlock)->mOperationCount) );
}

/*********************************************************
 * PROJ-2616
 * cmiLinkPeerInitCliWriteForIPCDA
 *
 * initialize cmbBlockIPCDA int cmiProtocolContext.
 *
 * aCtx[in] - cmiProtocolContext
 *********************************************************/
ACP_INLINE ACI_RC cmiLinkPeerInitCliWriteForIPCDA(void *aCtx)
{
    cmiProtocolContext *sCtx        = (cmiProtocolContext *)aCtx;
    cmbBlockIPCDA      *sWriteBlock = (cmbBlockIPCDA*)sCtx->mWriteBlock;
    cmnLinkPeer        *sLink       = sCtx->mLink;
    acp_bool_t          sConClosed  = ACP_FALSE;

    /* BUG-46502 atomic 함수 적용 atomic에 mem_barrier가 포함되어 있다. */
    ACI_TEST_RAISE(acpAtomicGet32(&sWriteBlock->mWFlag) == CMB_IPCDA_SHM_ACTIVATED, ContInitCliWriteForIPCDA);

    /* 다른 쓰레드에서 데이터를 읽고 있는 상태에서는 대기 한다. */
    while (acpAtomicGet32(&sWriteBlock->mRFlag) == CMB_IPCDA_SHM_ACTIVATED)
    {
        sLink->mPeerOp->mCheck(sLink, &sConClosed);
        ACI_TEST_RAISE(sConClosed == ACP_TRUE, err_disconnect);
        acpThrYield();
    }

    /* BUG-46502 atomic 함수 적용 atomic에 mem_barrier가 포함되어 있다. */
    acpAtomicSet32(&sWriteBlock->mWFlag, CMB_IPCDA_SHM_ACTIVATED);

    sWriteBlock->mBlock.mData      = &sWriteBlock->mData;
    sWriteBlock->mBlock.mCursor    = CMP_HEADER_SIZE;
    sWriteBlock->mBlock.mDataSize  = CMP_HEADER_SIZE;
    sWriteBlock->mBlock.mBlockSize = CMB_BLOCK_DEFAULT_SIZE - sizeof(cmbBlockIPCDA);

    acpAtomicSet32(&sWriteBlock->mRFlag, CMB_IPCDA_SHM_ACTIVATED);

    ACI_EXCEPTION_CONT(ContInitCliWriteForIPCDA);

    return ACI_SUCCESS;

    ACI_EXCEPTION(err_disconnect)
    {
        sCtx->mIsDisconnect = ACP_TRUE;
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*********************************************************
 * PROJ-2616
 * cmiFinalizeSendBufferForIPCDA
 *
 * increase data-count and disable w-flag.
 *
 * aCtx[in] - cmiProtocolContext
 *********************************************************/
ACP_INLINE void cmiFinalizeSendBufferForIPCDA(cmiProtocolContext *aCtx)
{
    cmiProtocolContext *sCtx  = (cmiProtocolContext *)aCtx;
    cmbBlockIPCDA      *sWriteBlock = (cmbBlockIPCDA *)sCtx->mWriteBlock;

    /* Write marking for end of protocol. */
    CMI_WR1(sCtx, CMP_OP_DB_IPCDALastOpEnded);

    /* BUG-46502 */
    cmiIPCDAIncDataCount(sCtx);

    acpAtomicSet32(&sWriteBlock->mWFlag, CMB_IPCDA_SHM_DEACTIVATED);
}

ACP_INLINE ACI_RC cmiReadyToWriteBufferIPCDA(cmiProtocolContext *aCtx)
{
    ACI_RC sRC = ACI_SUCCESS;
    if( acpAtomicGet32(&(((cmbBlockIPCDA*)aCtx->mWriteBlock)->mWFlag)), CMB_IPCDA_SHM_DEACTIVATED )
    {
        sRC = cmiLinkPeerInitCliWriteForIPCDA((void*)aCtx);
    }
    return sRC;
}

/**************************************************
 * PROJ-2616
 * IPCDA는 cmiSplitRead를 사용하지 않는다.
 *
 * aCtx[in]      - cmiProtocolContext
 * aReadSize[in] - acp_uint64_t 읽을 데이터 사이즈
 * aBuffer1[in]  - acp_uint8_t  데이터의 저장된 주소를 참조한다.
 * aBuffer2[in]  - acp_uint8_t  데이터가 copy된다.
 **************************************************/
ACP_INLINE ACI_RC cmiSplitReadIPCDA(cmiProtocolContext *aCtx,
                                    acp_uint64_t        aReadSize,
                                    acp_uint8_t       **aBuffer1,
                                    acp_uint8_t        *aBuffer2)
{
    ACI_TEST(aCtx->mReadBlock->mDataSize < (aCtx->mReadBlock->mCursor + aReadSize) );

    if (aBuffer2 != NULL)
    {
        CMI_RCP( aCtx, aBuffer2, aReadSize );
    }
    else
    {
        *aBuffer1 = (acp_uint8_t *)(aCtx->mReadBlock->mData +
                                    aCtx->mReadBlock->mCursor);
        aCtx->mReadBlock->mCursor += aReadSize;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

#endif
