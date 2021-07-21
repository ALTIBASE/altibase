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

#include <cmAll.h>
#include <idsGPKI.h>
#include <cmuProperty.h>
#include <iduLZ4.h>
#include <iduCompression.h>
#include <idsRC4.h>

UChar * cmnLinkPeerGetReadBlock(UInt aChannelID);
UChar * cmnLinkPeerGetWriteBlock(UInt aChannelID);

extern cmpOpMap gCmpOpBaseMap[];
extern cmpOpMap gCmpOpDBMap[];
extern cmpOpMap gCmpOpRPMap[];
extern cmpOpMap gCmpOpDKMap[];

// BUG-19465 : CM_Buffer의 pending list를 제한
UInt     gMaxPendingList;

//BUG-21080
static pthread_once_t     gCMInitOnce  = PTHREAD_ONCE_INIT;
static PDL_thread_mutex_t gCMInitMutex;
static SInt               gCMInitCount;
/* bug-33841: ipc thread's state is wrongly displayed */
static cmiCallbackSetExecute gCMCallbackSetExecute = NULL;

/* PROJ-2681 */
extern "C" const acp_uint32_t aciVersionID;

const acp_char_t *gCmErrorFactory[] =
{
#include "E_CM_US7ASCII.c"
};

inline IDE_RC cmiIPCDACheckLinkAndWait(cmiProtocolContext *aCtx,
                                       UInt                aMicroSleepTime,
                                       UInt               *aCurLoopCount)
{
    idBool            sLinkIsClosed = ID_FALSE;
    cmnLinkPeerIPCDA *sLink         = (cmnLinkPeerIPCDA *)aCtx->mLink;

    if( ++(*aCurLoopCount) == aCtx->mIPDASpinMaxCount )
    {
        sLink->mLinkPeer.mPeerOp->mCheck(aCtx->mLink, &sLinkIsClosed);
        IDE_TEST_RAISE(sLinkIsClosed == ID_TRUE, err_disconnected);

        if (aMicroSleepTime == 0)
        {
            /* IPCDA_SLEEP_TIME의 값이 0 인 경우,
             * thread_yield를 수행합니다. */
            idlOS::thr_yield();
        }
        else
        {
            acpSleepUsec(aMicroSleepTime);
        }
        (*aCurLoopCount) = 0;

        if( (aCtx->mIPDASpinMaxCount / 2) < CMI_IPCDA_SPIN_MIN_LOOP )
        {
            aCtx->mIPDASpinMaxCount = CMI_IPCDA_SPIN_MIN_LOOP;
        }
        else
        {
            aCtx->mIPDASpinMaxCount = aCtx->mIPDASpinMaxCount / 2;
        }
    }
    else
    {
        idlOS::thr_yield();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_disconnected)
    {
         IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC cmiIPCDACheckReadFlag(cmiProtocolContext *aCtx, UInt aMicroSleepTime)
{
    cmiProtocolContext *sCtx          = (cmiProtocolContext *)aCtx;
    cmbBlockIPCDA      *sReadBlock    = NULL;
    UInt                sLoopCount    = 0;

    sReadBlock = (cmbBlockIPCDA*)sCtx->mReadBlock;

    IDE_TEST(aCtx->mIsDisconnect == ID_TRUE);
    
    /* BUG-46502 atomic 함수 적용 */
    while ( acpAtomicGet32( &sReadBlock->mRFlag) == CMB_IPCDA_SHM_DEACTIVATED )
    {
        IDE_TEST( cmiIPCDACheckLinkAndWait(aCtx,
                                           aMicroSleepTime,
                                           &sLoopCount) == IDE_FAILURE);
    }

    if( (aCtx->mIPDASpinMaxCount * 2) > CMI_IPCDA_SPIN_MAX_LOOP )
    {
        aCtx->mIPDASpinMaxCount = CMI_IPCDA_SPIN_MAX_LOOP;
    }
    else
    {
        aCtx->mIPDASpinMaxCount = aCtx->mIPDASpinMaxCount * 2;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC cmiIPCDACheckDataCount(cmiProtocolContext *aCtx,
                                     volatile UInt      *aCount,
                                     UInt                aCompValue,
                                     UInt                aMicroSleepTime)
{
    IDE_RC sRC = IDE_FAILURE;
    UInt   sLoopCount    = 0;

    IDE_TEST(aCtx->mIsDisconnect == ID_TRUE);

    IDE_TEST(aCount == NULL);

    /* BUG-46502 atomic 함수 적용 */
    while ( (UInt)acpAtomicGet32(aCount) <= aCompValue )
    {
        sRC = cmiIPCDACheckLinkAndWait(aCtx,
                                       aMicroSleepTime,
                                       &sLoopCount);
        IDE_TEST(sRC == IDE_FAILURE);
    }

    if( (aCtx->mIPDASpinMaxCount * 2) > CMI_IPCDA_SPIN_MAX_LOOP )
    {
        aCtx->mIPDASpinMaxCount = CMI_IPCDA_SPIN_MAX_LOOP;
    }
    else
    {
        aCtx->mIPDASpinMaxCount = aCtx->mIPDASpinMaxCount * 2;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static void cmiInitializeOnce( void )
{
    IDE_ASSERT(idlOS::thread_mutex_init(&gCMInitMutex) == 0);

    gCMInitCount = 0;
}

static void cmiDump(cmiProtocolContext   *aCtx,
                    cmpHeader            *aHeader,
                    cmbBlock             *aBlock,
                    UInt                  aFromIndex,
                    UInt                  aLen);

#define CMI_CHECK_BLOCK_FOR_READ(aBlock)  ((aBlock)->mCursor < (aBlock)->mDataSize)
#define CMI_CHECK_BLOCK_FOR_WRITE(aBlock) ((aBlock)->mCursor < (aBlock)->mBlockSize)

/* BUG-38102 */
#define RC4_KEY         "7dcb6f959b9d37bf"
#define RC4_KEY_LEN     (16) /* 16 byte ( 128 bit ) */

/*
 * Packet Header로부터 Module을 구한다.
 */
static IDE_RC cmiGetModule(cmpHeader *aHeader, cmpModule **aModule)
{
    /*
     * Module 획득
     */
    IDE_TEST_RAISE(aHeader->mA5.mModuleID >= CMP_MODULE_MAX, UnknownModule);

    *aModule = gCmpModule[aHeader->mA5.mModuleID];

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnknownModule);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNKNOWN_MODULE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ProtocolContext에 Free 대기중인 Read Block들을 반환한다.
 */
static IDE_RC cmiFreeReadBlock(cmiProtocolContext *aProtocolContext)
{
    cmnLinkPeer *sLink;
    cmbBlock    *sBlock;
    iduListNode *sIterator;
    iduListNode *sNodeNext;

    IDE_TEST_CONT(cmiGetLinkImpl(aProtocolContext) == CMN_LINK_IMPL_IPCDA,
                  ContFreeReadBlock);

    /*
     * Protocol Context로부터 Link 획득
     */
    sLink = aProtocolContext->mLink;

    /*
     * Read Block List의 Block들 반환
     */
    IDU_LIST_ITERATE_SAFE(&aProtocolContext->mReadBlockList, sIterator, sNodeNext)
    {
        sBlock = (cmbBlock *)sIterator->mObj;

        IDE_TEST(sLink->mPeerOp->mFreeBlock(sLink, sBlock) != IDE_SUCCESS);
    }


    IDE_EXCEPTION_CONT(ContFreeReadBlock);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Block을 읽어온다.
 */
static IDE_RC cmiReadBlock(cmiProtocolContext *aProtocolContext, PDL_Time_Value *aTimeout)
{
    UInt sCmSeqNo;

    IDE_TEST_RAISE(aProtocolContext->mIsDisconnect == ID_TRUE, Disconnected);

    /*
     * Link로부터 Block을 읽어옴
     */
    IDE_TEST(aProtocolContext->mLink->mPeerOp->mRecv(aProtocolContext->mLink,
                                                     &aProtocolContext->mReadBlock,
                                                     &aProtocolContext->mReadHeader,
                                                     aTimeout) != IDE_SUCCESS);

    /* BUG-45184 */
    aProtocolContext->mReceiveDataSize += aProtocolContext->mReadBlock->mDataSize;
    aProtocolContext->mReceiveDataCount++;
    
    /*
     * Sequence 검사
     */
    sCmSeqNo = CMP_HEADER_SEQ_NO(&aProtocolContext->mReadHeader);

    IDE_TEST_RAISE(sCmSeqNo != aProtocolContext->mCmSeqNo, InvalidProtocolSeqNo);

    /*
     * Next Sequence 세팅
     */
    if (CMP_HEADER_PROTO_END_IS_SET(&aProtocolContext->mReadHeader) == ID_TRUE)
    {
        aProtocolContext->mCmSeqNo = 0;
    }
    else
    {
        aProtocolContext->mCmSeqNo = sCmSeqNo + 1;
    }

    /*
     * Module 획득
     */
    IDE_TEST(cmiGetModule(&aProtocolContext->mReadHeader,
                          &aProtocolContext->mModule) != IDE_SUCCESS);

    /*
     * ReadHeader로부터 WriteHeader에 필요한 정보를 획득
     */
    aProtocolContext->mWriteHeader.mA5.mModuleID        = aProtocolContext->mReadHeader.mA5.mModuleID;
    aProtocolContext->mWriteHeader.mA5.mModuleVersion   = aProtocolContext->mReadHeader.mA5.mModuleVersion;
    aProtocolContext->mWriteHeader.mA5.mSourceSessionID = aProtocolContext->mReadHeader.mA5.mTargetSessionID;
    aProtocolContext->mWriteHeader.mA5.mTargetSessionID = aProtocolContext->mReadHeader.mA5.mSourceSessionID;

    return IDE_SUCCESS;

    IDE_EXCEPTION(Disconnected);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(InvalidProtocolSeqNo);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Block을 전송한다.
 */
static IDE_RC cmiWriteBlock(cmiProtocolContext *aProtocolContext, idBool aIsEnd, PDL_Time_Value *aTimeout = NULL)
{
    cmnLinkPeer *sLink   = aProtocolContext->mLink;
    cmbBlock    *sBlock  = aProtocolContext->mWriteBlock;
    cmpHeader   *sHeader = &aProtocolContext->mWriteHeader;
    cmbBlock    *sPendingBlock;
    iduListNode *sIterator;
    iduListNode *sNodeNext;
    idBool       sSendSuccess;
    UInt         sSendDataSize = 0;

    // bug-27250 IPC linklist can be crushed.
    PDL_Time_Value      sWaitTime;
    cmnDispatcherImpl   sImpl;

    /*
     * 프로토콜 끝이라면 Sequence 종료 세팅
     */
    if (aIsEnd == ID_TRUE)
    {
        CMP_HEADER_SET_PROTO_END(sHeader);

        if (aProtocolContext->mIsAddReadBlock == ID_FALSE)
        {
            if (aProtocolContext->mReadBlock != NULL)
            {
                aProtocolContext->mIsAddReadBlock = ID_TRUE;

                /*
                 * 현재 Block을 Free 대기를 위한 Read Block List에 추가
                 */
                IDU_LIST_ADD_LAST(&aProtocolContext->mReadBlockList,
                                  &aProtocolContext->mReadBlock->mListNode);

                aProtocolContext->mReadBlock = NULL;
                IDE_TEST(cmiFreeReadBlock(aProtocolContext) != IDE_SUCCESS);
            }
        }
    }

    /*
     * Protocol Header 기록
     */
    IDE_TEST(cmpHeaderWrite(sHeader, sBlock) != IDE_SUCCESS);

    /*
     * Pending Write Block들을 전송
     */
    IDU_LIST_ITERATE_SAFE(&aProtocolContext->mWriteBlockList, sIterator, sNodeNext)
    {
        sPendingBlock = (cmbBlock *)sIterator->mObj;

        sSendSuccess = ID_TRUE;

        sSendDataSize = sPendingBlock->mDataSize;
        // BUG-19465 : CM_Buffer의 pending list를 제한
        while (sLink->mPeerOp->mSend(sLink, sPendingBlock) != IDE_SUCCESS)
        {
            sSendSuccess = ID_FALSE;

            IDE_TEST_RAISE(ideIsRetry() != IDE_SUCCESS, SendFail);

            /* BUG-44468 [cm] codesonar warning in CM */
            if( aProtocolContext->mListLength <= gMaxPendingList )
            {
                break;
            }

            //BUG-30338 
            sImpl = cmnDispatcherImplForLinkImpl(((cmnLink*)sLink)->mImpl);
            if (aTimeout != NULL)
            {
                IDE_TEST(cmnDispatcherWaitLink((cmiLink *)sLink,
                                                CMN_DIRECTION_WR, 
                                                aTimeout) != IDE_SUCCESS);
            } 
            else 
            {
                // Timeout is null.
                if (sImpl == CMI_DISPATCHER_IMPL_TCP)
                {
                    //BUG-34070
                    //In case of using TCP to communicate clients, 
                    //the write sockets should not be blocked forever.
                    sWaitTime.set(cmuProperty::getSockWriteTimeout(), 0);

                    IDE_TEST(cmnDispatcherWaitLink((cmiLink *)sLink,
                                                    CMN_DIRECTION_WR, 
                                                    &sWaitTime) != IDE_SUCCESS);
                }
                else
                {
                    IDE_TEST(cmnDispatcherWaitLink((cmiLink *)sLink, 
                                                    CMN_DIRECTION_WR,
                                                    NULL) != IDE_SUCCESS);
                }
            }

            sSendSuccess = ID_TRUE;
        }

        if (sSendSuccess == ID_FALSE)
        {
            break;
        }
        
        /* BUG-45184 */
        aProtocolContext->mSendDataSize += sSendDataSize;
        aProtocolContext->mSendDataCount++;
            
        aProtocolContext->mListLength--;
    }

    /*
     * Pending Write Block이 없으면 현재 Block 전송
     */
    if (sIterator == &aProtocolContext->mWriteBlockList)
    {
        sSendDataSize = sBlock->mDataSize;
        if (sLink->mPeerOp->mSend(sLink, sBlock) != IDE_SUCCESS)
        {
            IDE_TEST_RAISE(ideIsRetry() != IDE_SUCCESS, SendFail);

            IDU_LIST_ADD_LAST(&aProtocolContext->mWriteBlockList, &sBlock->mListNode);
            aProtocolContext->mListLength++;
        }
        
        /* BUG-45184 */
        aProtocolContext->mSendDataSize += sSendDataSize;
        aProtocolContext->mSendDataCount++;
    }
    else
    {
        IDU_LIST_ADD_LAST(&aProtocolContext->mWriteBlockList, &sBlock->mListNode);
        aProtocolContext->mListLength++;
    }

    /*
     * Protocol Context의 Write Block 삭제
     */
    sBlock                        = NULL;
    aProtocolContext->mWriteBlock = NULL;

    /*
     * Sequence Number
     */
    if (aIsEnd == ID_TRUE)
    {
        aProtocolContext->mWriteHeader.mA5.mCmSeqNo = 0;

        /*
         * 프로토콜 끝이라면 모든 Block이 전송되어야 함
         */
        IDU_LIST_ITERATE_SAFE(&aProtocolContext->mWriteBlockList, sIterator, sNodeNext)
        {
            sPendingBlock = (cmbBlock *)sIterator->mObj;

            sSendDataSize = sPendingBlock->mDataSize;
            while (sLink->mPeerOp->mSend(sLink, sPendingBlock) != IDE_SUCCESS)
            {
                IDE_TEST_RAISE(ideIsRetry() != IDE_SUCCESS, SendFail);
                
                //BUG-30338
                sImpl = cmnDispatcherImplForLinkImpl(((cmnLink*)sLink)->mImpl);
                if (aTimeout != NULL)
                {
                    IDE_TEST(cmnDispatcherWaitLink((cmiLink *)sLink, 
                                                    CMN_DIRECTION_WR, 
                                                    &sWaitTime) != IDE_SUCCESS);
                }
                else
                {
                    //BUG-34070
                    if (sImpl == CMI_DISPATCHER_IMPL_TCP)
                    {
                        sWaitTime.set(cmuProperty::getSockWriteTimeout(), 0);
                        IDE_TEST(cmnDispatcherWaitLink((cmiLink *)sLink, 
                                                        CMN_DIRECTION_WR, 
                                                        &sWaitTime) != IDE_SUCCESS);
                    }
                    else
                    {
                        IDE_TEST(cmnDispatcherWaitLink((cmiLink *)sLink,
                                                        CMN_DIRECTION_WR, 
                                                        NULL) != IDE_SUCCESS);
                    }
                }
            }
            
            /* BUG-45184 */
            aProtocolContext->mSendDataSize += sSendDataSize;
            aProtocolContext->mSendDataCount++;
        }

        sLink->mPeerOp->mReqComplete(sLink);
    }
    else
    {
        aProtocolContext->mWriteHeader.mA5.mCmSeqNo++;
    }

    return IDE_SUCCESS;

    // bug-27250 IPC linklist can be crushed.
    // 모든 에러에 대하여 pending block이 있으면 해제하도록 변경.
    // sendfail은 empty로 남겨둠.
    IDE_EXCEPTION(SendFail);
    {
    }
    IDE_EXCEPTION_END;
    {
        /* BUG-44468 [cm] codesonar warning in CM */
        if ( ( sBlock != NULL ) && ( aProtocolContext->mListLength > gMaxPendingList ) )
        {
            aProtocolContext->mResendBlock = ID_FALSE;
        }
        else
        {
            /* do nothing */
        }

        if ( aProtocolContext->mResendBlock == ID_TRUE )
        {
            if ( sBlock != NULL )
            {
                /*timeout error인 경우에 나중에 보낼 수 있도록 pending block에 삽입한다.*/
                IDU_LIST_ADD_LAST( &aProtocolContext->mWriteBlockList, &sBlock->mListNode );
                aProtocolContext->mListLength++;

                /*
                 * Protocol Context의 Write Block 삭제
                 */
                sBlock                        = NULL;
                aProtocolContext->mWriteBlock = NULL;
                if ( aIsEnd == ID_TRUE )
                {
                    aProtocolContext->mWriteHeader.mA5.mCmSeqNo = 0;
                }
                else
                {
                    aProtocolContext->mWriteHeader.mA5.mCmSeqNo++;
                } 
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            IDU_LIST_ITERATE_SAFE(&aProtocolContext->mWriteBlockList, sIterator, sNodeNext)
            {
                sPendingBlock = (cmbBlock *)sIterator->mObj;

                IDE_ASSERT(sLink->mPeerOp->mFreeBlock(sLink, sPendingBlock) == IDE_SUCCESS);
            }

            if (sBlock != NULL)
            {
                IDE_ASSERT(sLink->mPeerOp->mFreeBlock(sLink, sBlock) == IDE_SUCCESS);
            }

            aProtocolContext->mWriteBlock               = NULL;
            aProtocolContext->mWriteHeader.mA5.mCmSeqNo = 0;
        }
    }

    return IDE_FAILURE;
}


/*
 * Protocol을 읽어온다.
 */
static IDE_RC cmiReadProtocolInternal(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      PDL_Time_Value     *aTimeout)
{
    cmpMarshalFunction sMarshalFunction;
    UChar              sOpID;

    /*
     * Operation ID 읽음
     */
    CMB_BLOCK_READ_BYTE1(aProtocolContext->mReadBlock, &sOpID);

    /*
     * 프로토콜을 처음부터 읽어야 하는 상황
     */
    if (CMP_MARSHAL_STATE_IS_COMPLETE(aProtocolContext->mMarshalState) == ID_TRUE)
    {
        /*
         * Operation ID 검사
         */
        IDE_TEST_RAISE(sOpID >= aProtocolContext->mModule->mOpMaxA5, InvalidOpError);

        /*
         * Protocol 초기화
         */
        //fix BUG-17947.
        IDE_TEST(cmiInitializeProtocol(aProtocol,
                                       aProtocolContext->mModule,
                                       sOpID) != IDE_SUCCESS);
    }
    else
    {
        /*
         * 프로토콜이 연속되는 경우 프로토콜 OpID가 같은지 검사
         */
        IDE_TEST_RAISE(sOpID != aProtocol->mOpID, InvalidProtocolSeqNo);
    }

    /*
     * Get Marshal Function
     */
    sMarshalFunction = aProtocolContext->mModule->mReadFunction[sOpID];

    /*
     * Marshal Protocol
     */
    IDE_TEST(sMarshalFunction(aProtocolContext->mReadBlock,
                              aProtocol,
                              &aProtocolContext->mMarshalState) != IDE_SUCCESS);

    /*
     * Protocol Marshal이 완료되지 않았으면 다음 Block을 계속 읽어온 후 진행
     */
    while (CMP_MARSHAL_STATE_IS_COMPLETE(aProtocolContext->mMarshalState) != ID_TRUE)
    {
        /*
         * 현재 Block을 Free 대기를 위한 Read Block List에 추가
         */
        IDU_LIST_ADD_LAST(&aProtocolContext->mReadBlockList,
                          &aProtocolContext->mReadBlock->mListNode);

        aProtocolContext->mReadBlock = NULL;

        /*
         * 다음 Block을 읽어옴
         */
        IDE_TEST(cmiReadBlock(aProtocolContext, aTimeout) != IDE_SUCCESS);

        /*
         * Block에서 Operation ID 읽음
         */
        if (CMI_CHECK_BLOCK_FOR_READ(aProtocolContext->mReadBlock))
        {
            CMB_BLOCK_READ_BYTE1(aProtocolContext->mReadBlock, &sOpID);

            IDE_TEST_RAISE(sOpID != aProtocol->mOpID, InvalidProtocolSeqNo);

            /*
             * Marshal Protocol
             */
            IDE_TEST(sMarshalFunction(aProtocolContext->mReadBlock,
                                      aProtocol,
                                      &aProtocolContext->mMarshalState) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidOpError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_OPERATION));
    }
    IDE_EXCEPTION(InvalidProtocolSeqNo);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmiDispatcherWaitLink( cmiLink             * aLink,
                                     cmiDirection          aDirection,
                                     PDL_Time_Value      * aTimeout )
{
    cmnDispatcherImpl   sImpl;
    PDL_Time_Value      sWaitTime;
    PDL_Time_Value    * sWaitTimePtr = NULL;

    // 변경전: all timeout NULL, 변경후: 1 msec for IPC
    // IPC의 경우 무한대기하면 안된다.
    sImpl = cmnDispatcherImplForLinkImpl( aLink->mImpl );
    if ( aTimeout != NULL )
    {
        sWaitTimePtr = aTimeout;
    }
    else
    {
        if ( sImpl == CMI_DISPATCHER_IMPL_IPC )
        {
            sWaitTime.set( 0, 1000 );
            sWaitTimePtr = &sWaitTime;
        }
        else
        {
            sWaitTimePtr = NULL;
        }
    }

    IDE_TEST( cmnDispatcherWaitLink( (cmiLink *)aLink,
                                     aDirection,
                                     sWaitTimePtr ) 
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiInitialize( UInt   aCmMaxPendingList )
{
    cmbPool *sPoolLocal;

    // Property loading 
    IDE_TEST(cmuProperty::load() != IDE_SUCCESS);

    //BUG-21080   
    idlOS::pthread_once(&gCMInitOnce, cmiInitializeOnce);

    IDE_ASSERT(idlOS::thread_mutex_lock(&gCMInitMutex) == 0);

    IDE_TEST(gCMInitCount < 0);

    if (gCMInitCount == 0)
    {         
        
        /*
        * Shared Pool 생성 및 등록
        */
        //fix BUG-17864.
        IDE_TEST(cmbPoolAlloc(&sPoolLocal, CMB_POOL_IMPL_LOCAL,CMB_BLOCK_DEFAULT_SIZE,0) != IDE_SUCCESS);
        IDE_TEST(cmbPoolSetSharedPool(sPoolLocal, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);
    
    // fix BUG-18848
    #if !defined(CM_DISABLE_IPC)
        cmbPool *sPoolIPC;
    
        IDE_TEST(cmbPoolAlloc(&sPoolIPC, CMB_POOL_IMPL_IPC,CMB_BLOCK_DEFAULT_SIZE,0) != IDE_SUCCESS);
        IDE_TEST(cmbPoolSetSharedPool(sPoolIPC, CMB_POOL_IMPL_IPC) != IDE_SUCCESS);
    
        /*
        * IPC Mutex 초기화
        */
        IDE_TEST(cmbShmInitializeStatic() != IDE_SUCCESS);
    #endif

    #if !defined(CM_DISABLE_IPCDA)
        /*
         * IPC Mutex 초기화
         */
        IDE_TEST(cmbShmIPCDAInitializeStatic() != IDE_SUCCESS);
    #endif

        /*
        * cmmSession 초기화
        */
        IDE_TEST(cmmSessionInitializeStatic() != IDE_SUCCESS);
    
        /*
        * cmtVariable Piece Pool 초기화
        */
        IDE_TEST(cmtVariableInitializeStatic() != IDE_SUCCESS);
    
        /*
        * cmpModule 초기화
        */
        IDE_TEST(cmpModuleInitializeStatic() != IDE_SUCCESS);
    
        /*
        * DB Protocol 통계정보 초기화
        */
        idlOS::memset( gDBProtocolStat, 0x00, ID_SIZEOF(ULong) * CMP_OP_DB_MAX );
    
        // BUG-19465 : CM_Buffer의 pending list를 제한
        gMaxPendingList = aCmMaxPendingList;

        /* BUG-38951 Support to choice a type of CM dispatcher on run-time */
        IDE_TEST(cmnDispatcherInitialize() != IDE_SUCCESS);
    }
    
    gCMInitCount++;

    /* PROJ-2681 */
    (void)aciRegistErrorFromBuffer(ACI_E_MODULE_CM,
                                   aciVersionID,
                                   ID_SIZEOF(gCmErrorFactory) / ID_SIZEOF(gCmErrorFactory[0]), /* count */
                                   (acp_char_t **)&gCmErrorFactory);

    IDE_ASSERT(idlOS::thread_mutex_unlock(&gCMInitMutex) == 0);
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        gCMInitCount = -1;

        IDE_ASSERT(idlOS::thread_mutex_unlock(&gCMInitMutex) == 0);
          
    }         

    return IDE_FAILURE;
}

IDE_RC cmiFinalize()
{
    cmbPool *sPoolLocal;

    //BUG-21080 
    IDE_ASSERT(idlOS::thread_mutex_lock(&gCMInitMutex) == 0);

    IDE_TEST(gCMInitCount < 0);

    gCMInitCount--;

    if (gCMInitCount == 0)
    {
        /*
        * cmpModule 정리
        */
        IDE_TEST(cmpModuleFinalizeStatic() != IDE_SUCCESS);
    
        /*
        * cmtVariable Piece Pool 해제
        */
        IDE_TEST(cmtVariableFinalizeStatic() != IDE_SUCCESS);
    
        /*
        * cmmSession 정리
        */
        IDE_TEST(cmmSessionFinalizeStatic() != IDE_SUCCESS);
    
    // fix BUG-18848
    #if !defined(CM_DISABLE_IPC)
        cmbPool *sPoolIPC;
    
        /* IPC Mutex 해제 */
        IDE_TEST(cmbShmFinalizeStatic() != IDE_SUCCESS);
    
        /* Shared Pool 해제 */
        IDE_TEST(cmbPoolGetSharedPool(&sPoolIPC, CMB_POOL_IMPL_IPC) != IDE_SUCCESS);
        IDE_TEST(cmbPoolFree(sPoolIPC) != IDE_SUCCESS);

        /* Shared Memory 해제 */
        IDE_TEST(cmbShmDestroy() != IDE_SUCCESS);
    #endif

    #if !defined(CM_DISABLE_IPCDA)
        /* IPCDA Mutex 해제 */
        IDE_TEST(cmbShmIPCDAFinalizeStatic() != IDE_SUCCESS);

        /* Shared Memory 해제 */
         IDE_TEST(cmbShmIPCDADestroy() != IDE_SUCCESS);
    #endif

        IDE_TEST(cmbPoolGetSharedPool(&sPoolLocal, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);
        IDE_TEST(cmbPoolFree(sPoolLocal) != IDE_SUCCESS);
    }

    IDE_ASSERT(idlOS::thread_mutex_unlock(&gCMInitMutex) == 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        gCMInitCount = -1;

        IDE_ASSERT(idlOS::thread_mutex_unlock(&gCMInitMutex) == 0);
    }

    return IDE_FAILURE;
}

IDE_RC cmiSetCallback(UChar aModuleID, UChar aOpID, cmiCallbackFunction aCallbackFunction)
{
    /*
     * Module ID 검사
     */
    IDE_TEST_RAISE((aModuleID == CMP_MODULE_BASE) ||
                   (aModuleID >= CMP_MODULE_MAX), InvalidModule);

    /*
     * Operation ID 검사
     */
    IDE_TEST_RAISE(aOpID >= gCmpModule[aModuleID]->mOpMax, InvalidOperation);

    /*
     * Callback Function 세팅
     */
    if (aCallbackFunction == NULL)
    {
        gCmpModule[aModuleID]->mCallbackFunction[aOpID] = cmpCallbackNULL;
    }
    else
    {
        gCmpModule[aModuleID]->mCallbackFunction[aOpID] = aCallbackFunction;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidModule);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_MODULE));
    }
    IDE_EXCEPTION(InvalidOperation);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_OPERATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool cmiIsSupportedLinkImpl(cmiLinkImpl aLinkImpl)
{
    return cmnLinkIsSupportedImpl(aLinkImpl);
}

idBool cmiIsSupportedDispatcherImpl(cmiDispatcherImpl aDispatcherImpl)
{
    return cmnDispatcherIsSupportedImpl(aDispatcherImpl);
}

IDE_RC cmiAllocLink(cmiLink **aLink, cmiLinkType aType, cmiLinkImpl aImpl)
{
    /*
     * Link 할당
     */
    IDE_TEST(cmnLinkAlloc(aLink, aType, aImpl) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiFreeLink(cmiLink *aLink)
{
    /*
     * Link 해제
     */
    IDE_TEST(cmnLinkFree(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiCloseLink(cmiLink *aLink)
{
    /*
     * Link Close
     */
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiWaitLink(cmiLink *aLink, PDL_Time_Value *aTimeout)
{
    return cmnDispatcherWaitLink(aLink, CMI_DIRECTION_RD, aTimeout);
}

UInt cmiGetLinkFeature(cmiLink *aLink)
{
    return aLink->mFeature;
}

IDE_RC cmiListenLink(cmiLink *aLink, cmiListenArg *aListenArg)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    /*
     * Listen Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);

    /*
     * listen
     */
    IDE_TEST(sLink->mListenOp->mListen(sLink, aListenArg) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiAcceptLink(cmiLink *aLinkListen, cmiLink **aLinkPeer)
{
    cmnLinkListen *sLinkListen = (cmnLinkListen *)aLinkListen;
    cmnLinkPeer   *sLinkPeer   = NULL;

    /*
     * Listen Type 검사
     */
    IDE_ASSERT(aLinkListen->mType == CMN_LINK_TYPE_LISTEN);

    /*
     * accept
     */
    IDE_TEST(sLinkListen->mListenOp->mAccept(sLinkListen, &sLinkPeer) != IDE_SUCCESS);

    /*
     * accept된 Link 반환
     */
    *aLinkPeer = (cmiLink *)sLinkPeer;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiAllocChannel(cmiLink *aLink, SInt *aChannelID)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER);

    /*
     * Alloc Channel for IPC
     */
    IDE_TEST(sLink->mPeerOp->mAllocChannel(sLink, aChannelID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiHandshake(cmiLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    /*
     * Handshake
     */
    IDE_TEST(sLink->mPeerOp->mHandshake(sLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSetLinkBlockingMode(cmiLink *aLink, idBool aBlockingMode)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    /*
     * Set Blocking Mode
     */
    IDE_TEST(sLink->mPeerOp->mSetBlockingMode(sLink, aBlockingMode) != IDE_SUCCESS);

    if (aBlockingMode == ID_FALSE)
    {
        aLink->mFeature |= CMN_LINK_FLAG_NONBLOCK;
    }
    else
    {
        aLink->mFeature &= ~CMN_LINK_FLAG_NONBLOCK;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool cmiIsLinkBlockingMode( cmiLink * aLink )
{
    idBool  sIsBlockingMode = ID_TRUE;

    if ( ( aLink->mFeature & CMN_LINK_FLAG_NONBLOCK ) != CMN_LINK_FLAG_NONBLOCK )
    {
        sIsBlockingMode = ID_TRUE;
    }
    else
    {
        sIsBlockingMode = ID_FALSE;
    }

    return sIsBlockingMode;
    
}

IDE_RC cmiGetLinkInfo(cmiLink *aLink, SChar *aBuf, UInt aBufLen, cmiLinkInfoKey aKey)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    /*
     * Get Info
     */
    return sLink->mPeerOp->mGetInfo(sLink, aBuf, aBufLen, aKey);
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmiGetLinkSndBufSize(cmiLink *aLink, SInt *aSndBufSize)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    if (sLink->mPeerOp->mGetSndBufSize != NULL)
    {
        IDE_TEST(sLink->mPeerOp->mGetSndBufSize(sLink, aSndBufSize) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSetLinkSndBufSize(cmiLink *aLink, SInt aSndBufSize)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    if (sLink->mPeerOp->mSetSndBufSize != NULL)
    {
        IDE_TEST(sLink->mPeerOp->mSetSndBufSize(sLink, aSndBufSize) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiGetLinkRcvBufSize(cmiLink *aLink, SInt *aRcvBufSize)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    if (sLink->mPeerOp->mGetRcvBufSize != NULL)
    {
        IDE_TEST(sLink->mPeerOp->mGetRcvBufSize(sLink, aRcvBufSize) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSetLinkRcvBufSize(cmiLink *aLink, SInt aRcvBufSize)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    if (sLink->mPeerOp->mSetRcvBufSize != NULL)
    {
        IDE_TEST(sLink->mPeerOp->mSetRcvBufSize(sLink, aRcvBufSize) != IDE_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiCheckLink(cmiLink *aLink, idBool *aIsClosed)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    /*
     * Connection Closed 검사
     */
    return sLink->mPeerOp->mCheck(sLink, aIsClosed);
}

/* PROJ-2474 SSL/TLS */
IDE_RC cmiGetPeerCertIssuer(cmnLink *aLink,
                            SChar *aBuf,
                            UInt aBufLen)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    IDE_ASSERT( (aLink->mType == CMN_LINK_TYPE_PEER_SERVER) ||
                (aLink->mType == CMN_LINK_TYPE_PEER_CLIENT) );

    if ( aLink->mImpl == CMN_LINK_IMPL_SSL )
    {
        return sLink->mPeerOp->mGetSslPeerCertIssuer(sLink, aBuf, aBufLen);
    }
    else
    {
        aBuf[0] = '\0';
        aBufLen = 0;
    }

    return IDE_SUCCESS;

}

IDE_RC cmiGetPeerCertSubject(cmnLink *aLink,
                             SChar *aBuf,
                             UInt aBufLen)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    IDE_ASSERT( (aLink->mType == CMN_LINK_TYPE_PEER_SERVER) ||
                (aLink->mType == CMN_LINK_TYPE_PEER_CLIENT) );


    if ( aLink->mImpl == CMN_LINK_IMPL_SSL )
    {
        return sLink->mPeerOp->mGetSslPeerCertSubject(sLink, aBuf, aBufLen);
    }
    else
    {
        aBuf[0] = '\0';
        aBufLen = 0;
    }

    return IDE_SUCCESS;
}

IDE_RC cmiGetSslCipherInfo(cmnLink *aLink,
                           SChar *aBuf,
                           UInt aBufLen)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    IDE_ASSERT( (aLink->mType == CMN_LINK_TYPE_PEER_SERVER) ||
                (aLink->mType == CMN_LINK_TYPE_PEER_CLIENT) );

    if ( aLink->mImpl == CMN_LINK_IMPL_SSL )
    {
        return sLink->mPeerOp->mGetSslCipher(sLink, aBuf, aBufLen);
    }
    else
    {
        aBuf[0] = '\0';
        aBufLen = 0;
    }

    return IDE_SUCCESS;
}

/* only called in db-link */
idBool cmiLinkHasPendingRequest(cmiLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    /*
     * Pending Request 존재 여부 리턴
     */
    return sLink->mPeerOp->mHasPendingRequest(sLink);
}

idvSession *cmiGetLinkStatistics(cmiLink *aLink)
{
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    return cmnLinkPeerGetStatistics((cmnLinkPeer *)aLink);
}

void cmiSetLinkStatistics(cmiLink *aLink, idvSession *aStatistics)
{
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    cmnLinkPeerSetStatistics((cmnLinkPeer *)aLink, aStatistics);
}

void *cmiGetLinkUserPtr(cmiLink *aLink)
{
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    return cmnLinkPeerGetUserPtr((cmnLinkPeer *)aLink);
}

void cmiSetLinkUserPtr(cmiLink *aLink, void *aUserPtr)
{
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    cmnLinkPeerSetUserPtr((cmnLinkPeer *)aLink, aUserPtr);
}

IDE_RC cmiShutdownLink(cmiLink *aLink, cmiDirection aDirection)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);

    // bug-28277 ipc: server stop failed when idle clis exist
    // server stop시에만 shutdown_mode_force 넘기도록 함.
    IDE_TEST(sLink->mPeerOp->mShutdown(sLink, aDirection,
                                       CMN_SHUTDOWN_MODE_NORMAL)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// bug-28227: ipc: server stop failed when idle cli exists
// server stop시 mmtSessionManager::shutdown() 에서 다음함수 호출.
// IPC에 대해서만 shutdown_mode_force로 설정하여 shutdown 호출.
IDE_RC cmiShutdownLinkForce(cmiLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    if (((aLink->mImpl == CMN_LINK_IMPL_IPC) ||
         (aLink->mImpl == CMN_LINK_IMPL_IPCDA)) &&
        (aLink->mType == CMN_LINK_TYPE_PEER_SERVER))
    {
        (void)sLink->mPeerOp->mShutdown(sLink, CMI_DIRECTION_RD,
                                        CMN_SHUTDOWN_MODE_FORCE);
    }
    return IDE_SUCCESS;
}

cmiDispatcherImpl cmiDispatcherImplForLinkImpl(cmiLinkImpl aLinkImpl)
{
    return cmnDispatcherImplForLinkImpl(aLinkImpl);
}

cmiDispatcherImpl cmiDispatcherImplForLink(cmiLink *aLink)
{
    return cmnDispatcherImplForLinkImpl(aLink->mImpl);
}

IDE_RC cmiAllocDispatcher(cmiDispatcher **aDispatcher, cmiDispatcherImpl aImpl, UInt aMaxLink)
{
    /*
     * Dispatcher 할당
     */
    IDE_TEST(cmnDispatcherAlloc(aDispatcher, aImpl, aMaxLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiFreeDispatcher(cmiDispatcher *aDispatcher)
{
    /*
     * Dispatcher 해제
     */
    IDE_TEST(cmnDispatcherFree(aDispatcher) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiAddLinkToDispatcher(cmiDispatcher *aDispatcher, cmiLink *aLink)
{
    /*
     * Dispatcher에 Link 추가
     */
    IDE_TEST(aDispatcher->mOp->mAddLink(aDispatcher, aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiRemoveLinkFromDispatcher(cmiDispatcher *aDispatcher, cmiLink *aLink)
{
    /*
     * Dispatcher에 Link 삭제
     */
    IDE_TEST(aDispatcher->mOp->mRemoveLink(aDispatcher, aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiRemoveAllLinksFromDispatcher(cmiDispatcher *aDispatcher)
{
    /*
     * Dispatcher에서 모든 Link 삭제
     */
    IDE_TEST(aDispatcher->mOp->mRemoveAllLinks(aDispatcher) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSelectDispatcher(cmiDispatcher  *aDispatcher,
                           iduList        *aReadyList,
                           UInt           *aReadyCount,
                           PDL_Time_Value *aTimeout)
{
    /*
     * select
     */
    IDE_TEST(aDispatcher->mOp->mSelect(aDispatcher,
                                       aReadyList,
                                       aReadyCount,
                                       aTimeout) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiAddSession(cmiSession         *aSession,
                     void               *aOwner,
                     UChar               aModuleID,
                     cmiProtocolContext */* aProtocolContext */)
{
    /*
     * 파라미터 범위 검사
     */
    IDE_ASSERT(aModuleID > CMP_MODULE_BASE);

    IDE_TEST_RAISE(aModuleID >= CMP_MODULE_MAX, UnknownModule);

    /*
     * Session 추가
     */
    IDE_TEST(cmmSessionAdd(aSession) != IDE_SUCCESS);

    /*
     * Session 초기화
     */
    aSession->mOwner       = aOwner;
    aSession->mBaseVersion = CMP_VER_BASE_NONE;

    aSession->mLink           = NULL;
    aSession->mCounterpartID  = 0;
    aSession->mModuleID       = aModuleID;
    aSession->mModuleVersion  = CMP_VER_BASE_NONE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnknownModule);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNKNOWN_MODULE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiRemoveSession(cmiSession *aSession)
{
    /*
     * Session의 Session ID가 0이면 등록되지 않은 Session
     */
    IDE_TEST_RAISE(aSession->mSessionID == 0, SessionNotAdded);

    /*
     * Session 삭제
     */
    IDE_TEST(cmmSessionRemove(aSession) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionNotAdded);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SESSION_NOT_ADDED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSetLinkForSession(cmiSession *aSession, cmiLink *aLink)
{
    if (aLink != NULL)
    {
        /*
         * Session에 Link가 이미 등록된 상태에서 새로운 Link를 세팅할 수 없음
         */
        IDE_TEST_RAISE(aSession->mLink != NULL, LinkAlreadyRegistered);

        /*
         * Link가 Peer Type인지 검사
         */
        //BUG-28119 for RP PBT
        IDE_TEST_RAISE((aLink->mType != CMN_LINK_TYPE_PEER_CLIENT) && 
                       (aLink->mType != CMN_LINK_TYPE_PEER_SERVER), InvalidLinkType);
    }

    /*
     * Session에 Link 세팅
     */
    aSession->mLink = (cmnLinkPeer *)aLink;

    return IDE_SUCCESS;

    IDE_EXCEPTION(LinkAlreadyRegistered);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_LINK_ALREADY_REGISTERED));
    }
    IDE_EXCEPTION(InvalidLinkType);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_LINK_TYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiGetLinkForSession(cmiSession *aSession, cmiLink **aLink)
{
    /*
     * Session의 Link 반환
     */
    *aLink = (cmiLink *)aSession->mLink;

    return IDE_SUCCESS;
}

IDE_RC cmiSetOwnerForSession(cmiSession *aSession, void *aOwner)
{
    /*
     * Session에 Owner 세팅
     */
    aSession->mOwner = aOwner;

    return IDE_SUCCESS;
}

IDE_RC cmiGetOwnerForSession(cmiSession *aSession, void **aOwner)
{
    /*
     * Session의 Owner 반환
     */
    *aOwner = aSession->mOwner;

    return IDE_SUCCESS;
}

IDE_RC cmiSetOwnerForProtocolContext( cmiProtocolContext *aCtx, void *aOwner )
{
    /*
     * ProtocolContext에 Owner 세팅
     */
    aCtx->mOwner = aOwner;

    return IDE_SUCCESS;
}

IDE_RC cmiGetOwnerForProtocolContext( cmiProtocolContext *aCtx, void **aOwner )
{
    /*
     * ProtocolContext의 Owner 반환
     */
    *aOwner = aCtx->mOwner;

    return IDE_SUCCESS;
}

IDE_RC cmiGetLinkForProtocolContext( cmiProtocolContext *aCtx, cmiLink **aLink )
{
    /*
     * ProtocolContext의 Link 반환
     */
    *aLink = (cmiLink *)(aCtx->mLink);

    return IDE_SUCCESS;
}

//===========================================================
// proj_2160 cm_type removal
// 함수를 2개로 나눈 이유:
// cmiConnect            : DB 프로토콜용
// cmiConnectWithoutData : RP 프로토콜용 (DB_Handshake 하지 않음)
// RP에서는 DB_Handshake를 처리하기가 어렵고,
// (BASE 프로토콜이 없어지면서 opcode 값이 DB 내에서만 유효해짐),
// 또 안해도 문제가 없다고 생각되어 하지 않도록 한다
// 별도 함수를 사용하는 것이  if-else 처리 보다 낫다고 협의하였음
//===========================================================
IDE_RC cmiConnect(cmiProtocolContext *aCtx, cmiConnectArg *aConnectArg, PDL_Time_Value *aTimeout, SInt aOption)
{
    idBool                     sConnectFlag      = ID_FALSE;
    UChar                      sOpID;

    UInt                       sErrIndex;
    UInt                       sErrCode;
    UShort                     sErrMsgLen;
    SChar                      sErrMsg[MAX_ERROR_MSG_LEN]; // 2048

    IDE_TEST_RAISE(aCtx->mModule->mModuleID != CMP_MODULE_DB,
                   InvalidModuleError);
    
    IDU_FIT_POINT( "cmiConnect::Connect" );

    /* Link Connect */
    IDE_TEST(aCtx->mLink->mPeerOp->mConnect(aCtx->mLink,
                                            aConnectArg,
                                            aTimeout,
                                            aOption) != IDE_SUCCESS);
    sConnectFlag = ID_TRUE;
    // STF인 경우 때문에 여기서 다시 초기화시켜줘야함
    aCtx->mWriteHeader.mA7.mCmSeqNo = 0; // send seq
    aCtx->mCmSeqNo = 0;                  // recv seq

    IDE_TEST(aCtx->mLink->mPeerOp->mHandshake(aCtx->mLink) != IDE_SUCCESS);

    CMI_WR1(aCtx, CMP_OP_DB_Handshake);
    CMI_WR1(aCtx, aCtx->mModule->mModuleID); // DB: 1, RP: 2
    CMI_WR1(aCtx, CM_MAJOR_VERSION);
    CMI_WR1(aCtx, CM_MINOR_VERSION);
    CMI_WR1(aCtx, CM_PATCH_VERSION);
    CMI_WR1(aCtx, 0);
    IDE_TEST( cmiSend(aCtx, ID_TRUE) != IDE_SUCCESS);

    //fix BUG-17942
    // cmiRecvNext() 대신에 cmiRecv()를 호출한다
    // DB_HandshakeResult에 대한 callback은 존재하지 않음
    IDE_TEST(cmiRecvNext(aCtx, aTimeout) != IDE_SUCCESS);
    CMI_RD1(aCtx, sOpID);
    if (sOpID != CMP_OP_DB_HandshakeResult)
    {
        if (sOpID == CMP_OP_DB_ErrorResult)
        {
            IDE_RAISE(HandshakeError);
        }
        else
        {
            IDE_RAISE(InvalidProtocolSeqError);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(HandshakeError);
    {
        CMI_SKIP_READ_BLOCK(aCtx, 1); /* skip error op ID */
        /* BUG-44556  Handshake 과정중에 발생한 에러의 프로토콜 해석이 잘못되었습니다.*/
        CMI_RD4(aCtx, &sErrIndex);
        CMI_RD4(aCtx, &sErrCode);
        CMI_RD2(aCtx, &sErrMsgLen);
        if (sErrMsgLen >= MAX_ERROR_MSG_LEN)
        {
            sErrMsgLen = MAX_ERROR_MSG_LEN - 1;
        }
        CMI_RCP(aCtx, sErrMsg, sErrMsgLen);
        sErrMsg[sErrMsgLen] = '\0';
        IDE_SET(ideSetErrorCodeAndMsg(sErrCode, sErrMsg));
    }
    IDE_EXCEPTION(InvalidModuleError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_MODULE));
    }
    IDE_EXCEPTION(InvalidProtocolSeqError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));
    }
    IDE_EXCEPTION_END;
    {
        IDE_PUSH();
        // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close 해야 합니다
        if(sConnectFlag == ID_TRUE)
        {
            (void)aCtx->mLink->mPeerOp->mShutdown(aCtx->mLink,
                                                  CMI_DIRECTION_RDWR,
                                                  CMN_SHUTDOWN_MODE_NORMAL);

            (void)aCtx->mLink->mLink.mOp->mClose(&aCtx->mLink->mLink);
        }
        IDE_POP();
    }

    return IDE_FAILURE;
}

// RP 프로토콜용 (DB_Handshake 하지 않음)
IDE_RC cmiConnectWithoutData( cmiProtocolContext * aCtx,
                              cmiConnectArg * aConnectArg,
                              PDL_Time_Value * aTimeout,
                              SInt aOption )
{
    idBool sConnectFlag = ID_FALSE;

    IDU_FIT_POINT( "cmiConnectWithoutData::Connect" );
    
    /* Link Connect */
    IDE_TEST(aCtx->mLink->mPeerOp->mConnect(aCtx->mLink,
                                            aConnectArg,
                                            aTimeout,
                                            aOption) != IDE_SUCCESS);
    sConnectFlag = ID_TRUE;

    IDE_TEST(aCtx->mLink->mPeerOp->mHandshake(aCtx->mLink));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close 해야 합니다
    if(sConnectFlag == ID_TRUE)
    {
        (void)aCtx->mLink->mPeerOp->mShutdown(aCtx->mLink,
                                              CMI_DIRECTION_RDWR,
                                              CMN_SHUTDOWN_MODE_NORMAL);

        (void)aCtx->mLink->mLink.mOp->mClose(&aCtx->mLink->mLink);
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC cmiInitializeProtocol(cmiProtocol *aProtocol, cmpModule*  aModule, UChar aOperationID)
{
    //fix BUG-17947.
    /*
     * Operation ID 세팅
     */
    aProtocol->mOpID = aOperationID;
    aProtocol->mClientLastOpID = 0;  /* PROJ-2733-Protocol Unused at A5 */

    /*
     * Protocol Finalize 함수 세팅
     */
    aProtocol->mFinalizeFunction = (void *)aModule->mArgFinalizeFunction[aOperationID];

    /*
     * Protocol 초기화
     */
    if (aModule->mArgInitializeFunction[aOperationID] != cmpArgNULL)
    {
        IDE_TEST_RAISE((aModule->mArgInitializeFunction[aOperationID])(aProtocol) != IDE_SUCCESS,
                       InitializeFail);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InitializeFail);
    {
        aProtocol->mFinalizeFunction = (void *)cmpArgNULL;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*fix BUG-30041 cmiReadProtocol에서 mFinalization 이초기화 되기전에
 실패하는 case에 cmiFinalization에서 비정상종료됩니다.*/
void  cmiInitializeProtocolNullFinalization(cmiProtocol *aProtocol)
{
    aProtocol->mFinalizeFunction = (void *)cmpArgNULL;
}

IDE_RC cmiFinalizeProtocol(cmiProtocol *aProtocol)
{
    if (aProtocol->mFinalizeFunction != (void *)cmpArgNULL)
    {
        IDE_TEST(((cmpArgFunction)aProtocol->mFinalizeFunction)(aProtocol) != IDE_SUCCESS);

        aProtocol->mFinalizeFunction = (void *)cmpArgNULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-2296
 *
 * 이중화 프로토콜 호환을 위해 추가함. 전송을 위한 별도의
 * cmiProtocolContext를 이 함수로 만든다.
 */ 
IDE_RC cmiInitializeProtocolContext( cmiProtocolContext * aCtx,
                                     UChar                aModuleID,
                                     cmiLink            * aLink,
                                     idBool               aResendBlock )
{
    aCtx->mModule         = gCmpModule[ aModuleID ];
    aCtx->mLink           = (cmnLinkPeer*)aLink;
    aCtx->mOwner          = NULL;
    aCtx->mIsAddReadBlock = ID_FALSE;
    aCtx->mSession        = NULL; // deprecated

    aCtx->mReadBlock      = NULL;
    aCtx->mWriteBlock     = NULL;
    /*PROJ-2616*/
    aCtx->mSimpleQueryFetchIPCDAWriteBlock.mData = NULL;

    cmpHeaderInitializeForA5( &aCtx->mWriteHeader );

    aCtx->mWriteHeader.mA5.mModuleID = aModuleID;
    
    CMP_MARSHAL_STATE_INITIALIZE( aCtx->mMarshalState );
    
    aCtx->mProtocol.mFinalizeFunction = (void *)cmpArgNULL;

    IDU_LIST_INIT( &aCtx->mReadBlockList );
    IDU_LIST_INIT( &aCtx->mWriteBlockList );

    aCtx->mListLength             = 0;
    aCtx->mCmSeqNo                = 0;
    aCtx->mIsDisconnect           = ID_FALSE;
    aCtx->mSessionCloseNeeded     = ID_FALSE;
    /*PROJ-2616*/
    aCtx->mIPDASpinMaxCount       = CMI_IPCDA_SPIN_MIN_LOOP;

    cmiDisableCompress( aCtx );
    /* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
    /* 압축 해제 방식은 default 값이 LZO */ 
    cmiSetDecompressType( aCtx, CMI_COMPRESS_LZO );

    /* BUG-38102 */
    cmiDisableEncrypt( aCtx );

    /* 
     * BUG-38716 
     * [rp-sender] It needs a property to give sending timeout to replication sender. 
     */
    aCtx->mResendBlock = aResendBlock;

    aCtx->mSendDataSize           = 0;
    aCtx->mReceiveDataSize        = 0;
    aCtx->mSendDataCount          = 0;
    aCtx->mReceiveDataCount       = 0;

    aCtx->mProtocol.mClientLastOpID = 0;  /* PROJ-2733-Protocol Unused at RP  */
    
    return IDE_SUCCESS;
}

/*
 * PROJ-2296
 *
 * cmiInitializeProtocolContext()로 만들어진 Protocol Context를 정리한다.
 */
IDE_RC cmiFinalizeProtocolContext( cmiProtocolContext   * aProtocolContext )
{
    IDE_TEST_CONT(cmiGetLinkImpl(aProtocolContext) == CMN_LINK_IMPL_IPCDA,
                  ContFinPtcCtx);

    return cmiFreeCmBlock( aProtocolContext );
    
    IDE_EXCEPTION_CONT(ContFinPtcCtx);
    
    return IDE_SUCCESS;
}


void cmiSetProtocolContextLink(cmiProtocolContext *aProtocolContext, cmiLink *aLink)
{
    /*
     * Protocol Context에 Link 세팅
     */
    aProtocolContext->mLink = (cmnLinkPeer *)aLink;
}

/*
 * PROJ-2296 
 */ 
void cmiProtocolContextCopyA5Header( cmiProtocolContext * aCtx,
                                     cmpHeader          * aHeader )
{
    aCtx->mWriteHeader.mA5.mHeaderSign      = CMP_HEADER_SIGN_A5;
    aCtx->mWriteHeader.mA5.mModuleID        = aHeader->mA5.mModuleID;
    aCtx->mWriteHeader.mA5.mModuleVersion   = aHeader->mA5.mModuleVersion;
    aCtx->mWriteHeader.mA5.mSourceSessionID = aHeader->mA5.mTargetSessionID;
    aCtx->mWriteHeader.mA5.mTargetSessionID = aHeader->mA5.mSourceSessionID;
}


idBool cmiProtocolContextHasPendingRequest(cmiProtocolContext *aCtx)
{
    idBool sRet = ID_FALSE;

    /* proj_2160 cm_type removal: A7 uses static mReadBlock */
    if (aCtx->mLink->mLink.mPacketType != CMP_PACKET_TYPE_A5)
    {
        sRet = aCtx->mLink->mPeerOp->mHasPendingRequest(aCtx->mLink);
    }
    else
    {
        if (aCtx->mReadBlock != NULL)
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = aCtx->mLink->mPeerOp->mHasPendingRequest(aCtx->mLink);
        }
    }
    return sRet;
}

IDE_RC cmiReadProtocolAndCallback(cmiProtocolContext      *aProtocolContext,
                                  void                    *aUserContext,
                                  PDL_Time_Value          *aTimeout,
                                  void                    *aTask)
{
    cmpCallbackFunction  sCallbackFunction;
    IDE_RC               sRet = IDE_SUCCESS;
    cmnLinkPeer         *sLink;

    /*
     * 읽어온 Block이 하나도 없으면 읽어옴
     */
    if (aProtocolContext->mReadBlock == NULL)
    {
        IDE_TEST(cmiReadBlock(aProtocolContext, aTimeout) != IDE_SUCCESS);

        /* bug-33841: ipc thread's state is wrongly displayed.
           IPC인 경우 패킷 수신후에 execute 상태로 변경 */
        (void) gCMCallbackSetExecute(aUserContext, aTask);
    }

    while (1)
    {
        /*
         * Protocol 읽음
         */
        if (CMI_CHECK_BLOCK_FOR_READ(aProtocolContext->mReadBlock))
        {
            IDE_TEST(cmiReadProtocolInternal(aProtocolContext,
                                             &aProtocolContext->mProtocol,
                                             aTimeout) != IDE_SUCCESS);

            /*
             * Callback Function 획득
             */
            // proj_2160 cm_type removal: call mCallbackFunctionA5
            sCallbackFunction = aProtocolContext->mModule->mCallbackFunctionA5[aProtocolContext->mProtocol.mOpID];

            /*
             * Callback 호출
             */
            sRet = sCallbackFunction(aProtocolContext,
                                     &aProtocolContext->mProtocol,
                                     aProtocolContext->mOwner,
                                     aUserContext);
            
            /*
             * PROJ-1697 Performance view for Protocols
             */
            if( aProtocolContext->mModule->mModuleID == CMP_MODULE_DB )
            {
                CMP_DB_PROTOCOL_STAT_ADD( aProtocolContext->mProtocol.mOpID, 1 );
            }

            /*
             * Protocol Finalize
             */
            IDE_TEST(cmiFinalizeProtocol(&aProtocolContext->mProtocol) != IDE_SUCCESS);

            /*
             * Free Block List에 달린 Block 해제
             */
            IDE_TEST(cmiFreeReadBlock(aProtocolContext) != IDE_SUCCESS);

            /*
             * Callback 결과 확인
             */
            if (sRet != IDE_SUCCESS)
            {
                break;
            }

            if (aProtocolContext->mIsAddReadBlock == ID_TRUE)
            {
                IDE_RAISE(ReadBlockEnd);
            }
        }
        else
        {
            if (aProtocolContext->mIsAddReadBlock == ID_FALSE)
            {
                /*
                 * 현재 Block을 Free 대기를 위한 Read Block List에 추가
                 */
                IDU_LIST_ADD_LAST(&aProtocolContext->mReadBlockList,
                                  &aProtocolContext->mReadBlock->mListNode);
            }

            IDE_EXCEPTION_CONT(ReadBlockEnd);
            {
                aProtocolContext->mReadBlock = NULL;
                aProtocolContext->mIsAddReadBlock = ID_FALSE;

                if (CMP_HEADER_PROTO_END_IS_SET(&aProtocolContext->mReadHeader) == ID_TRUE)
                {
                    IDE_TEST(cmiFreeReadBlock(aProtocolContext) != IDE_SUCCESS);
                    /*
                     * Protocol Sequence 완료
                     */

                    sLink = aProtocolContext->mLink;
                    sLink->mPeerOp->mResComplete(sLink);
                    break;
                }
                else
                {
                    /*
                     * 다음 Block을 읽어옴
                     */
                    IDE_TEST(cmiReadBlock(aProtocolContext, aTimeout) != IDE_SUCCESS);
                }

            }
        }
    }

    return sRet;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiReadProtocol(cmiProtocolContext *aProtocolContext,
                       cmiProtocol        *aProtocol,
                       PDL_Time_Value     *aTimeout,
                       idBool             *aIsEnd)
{
    cmpCallbackFunction sCallbackFunction;
    IDE_RC              sRet;
    cmnLinkPeer        *sLink;

    /*
     * 이전 Read Protocol이 정상적으로 완료되었을 경우
     */
    if (CMP_MARSHAL_STATE_IS_COMPLETE(aProtocolContext->mMarshalState) == ID_TRUE)
    {
        /*
         * Read Block 반환
         */
        IDE_TEST(cmiFreeReadBlock(aProtocolContext) != IDE_SUCCESS);

        sLink = aProtocolContext->mLink;
        sLink->mPeerOp->mResComplete(sLink);

        /*
         * Protocol Finalize 함수 초기화
         *
         * cmiReadProtocol 함수는 cmiFinalizeProtocol호출의 책임을 상위레이어가 가짐
         */
        aProtocol->mFinalizeFunction = (void *)cmpArgNULL;
    }

    /*
     * 읽어온 Block이 하나도 없으면 읽어옴
     */
    if (aProtocolContext->mReadBlock == NULL)
    {
        IDE_TEST(cmiReadBlock(aProtocolContext, aTimeout) != IDE_SUCCESS);
    }

    while (1)
    {
        /*
         * Protocol 읽음
         */
        if (CMI_CHECK_BLOCK_FOR_READ(aProtocolContext->mReadBlock))
        {
            IDE_TEST(cmiReadProtocolInternal(aProtocolContext,
                                             aProtocol,
                                             aTimeout) != IDE_SUCCESS);

            /*
             * BASE Module이면 Callback
             */
            if (aProtocolContext->mReadHeader.mA5.mModuleID == CMP_MODULE_BASE)
            {
                /*
                 * Callback Function 획득
                 */
                // proj_2160 cm_type removal: call mCallbackFunctionA5
                sCallbackFunction = aProtocolContext->mModule->mCallbackFunctionA5[aProtocol->mOpID];

                /*
                 * Callback
                 */
                sRet = sCallbackFunction(aProtocolContext,
                                         aProtocol,
                                         aProtocolContext->mOwner,
                                         NULL);

                /*
                 * Protocol Finalize
                 */
                IDE_TEST(cmiFinalizeProtocol(aProtocol) != IDE_SUCCESS);

                IDE_TEST(sRet != IDE_SUCCESS);

                // BUG-18846
                if (aProtocolContext->mIsAddReadBlock == ID_TRUE)
                {
                    IDE_RAISE(ReadBlockEnd);
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            // BUG-18846
            if (aProtocolContext->mIsAddReadBlock == ID_FALSE)
            {
                IDU_LIST_ADD_LAST(&aProtocolContext->mReadBlockList,
                                  &aProtocolContext->mReadBlock->mListNode);
            }

            IDE_EXCEPTION_CONT(ReadBlockEnd);
            {
                aProtocolContext->mReadBlock      = NULL;
                aProtocolContext->mIsAddReadBlock = ID_FALSE;
            }

            IDE_TEST(cmiReadBlock(aProtocolContext, aTimeout) != IDE_SUCCESS);
        }
    }

    /*
     * 현재 Read Block을 끝까지 읽었으면
     */
    if (!CMI_CHECK_BLOCK_FOR_READ(aProtocolContext->mReadBlock))
    {
        // BUG-18846
        if (aProtocolContext->mIsAddReadBlock == ID_FALSE)
        {
            IDU_LIST_ADD_LAST(&aProtocolContext->mReadBlockList,
                              &aProtocolContext->mReadBlock->mListNode);

        }

        aProtocolContext->mReadBlock      = NULL;
        aProtocolContext->mIsAddReadBlock = ID_FALSE;

        if (CMP_HEADER_PROTO_END_IS_SET(&aProtocolContext->mReadHeader))
        {
            *aIsEnd = ID_TRUE;
        }
    }
    else
    {
        *aIsEnd = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC cmiWriteProtocol(cmiProtocolContext *aProtocolContext, cmiProtocol *aProtocol, PDL_Time_Value *aTimeout)
{
    cmpMarshalState     sMarshalState;
    cmpMarshalFunction  sMarshalFunction;

    /*
     * Write Block이 할당되어있지 않으면 할당
     */
    if (aProtocolContext->mWriteBlock == NULL)
    {
        IDE_TEST(aProtocolContext->mLink->mPeerOp->mAllocBlock(aProtocolContext->mLink,
                                                               &aProtocolContext->mWriteBlock)
                 != IDE_SUCCESS);
    }

    /*
     * Marshal State 초기화
     */
    CMP_MARSHAL_STATE_INITIALIZE(sMarshalState);

    /*
     * Module 획득
     */
    if (aProtocolContext->mModule == NULL)
    {
        IDE_TEST(cmiGetModule(&aProtocolContext->mWriteHeader,
                              &aProtocolContext->mModule) != IDE_SUCCESS);
    }

    /*
     * Operation ID 검사
     */
    IDE_TEST_RAISE(aProtocol->mOpID >= aProtocolContext->mModule->mOpMaxA5, InvalidOpError);

    /*
     * Marshal Function 획득
     */
    sMarshalFunction = aProtocolContext->mModule->mWriteFunction[aProtocol->mOpID];

    /*
     * PROJ-1697 Performance view for Protocols
     */
    if( aProtocolContext->mModule->mModuleID == CMP_MODULE_DB )
    {
        CMP_DB_PROTOCOL_STAT_ADD( aProtocol->mOpID, 1 );
    }
    
    /*
     * Marshal Loop
     */
    while (1)
    {
        /*
         * Operation ID를 기록하고 Marshal
         */
        if (CMI_CHECK_BLOCK_FOR_WRITE(aProtocolContext->mWriteBlock))
        {
            CMB_BLOCK_WRITE_BYTE1(aProtocolContext->mWriteBlock, aProtocol->mOpID);

            IDE_TEST(sMarshalFunction(aProtocolContext->mWriteBlock,
                                      aProtocol,
                                      &sMarshalState) != IDE_SUCCESS);

            /*
             * 프로토콜 쓰기가 완료되었으면 Loop 종료
             */
            if (CMP_MARSHAL_STATE_IS_COMPLETE(sMarshalState) == ID_TRUE)
            {
                break;
            }
        }

        /*
         * 전송
         */
        if ( cmiWriteBlock(aProtocolContext, ID_FALSE, aTimeout) != IDE_SUCCESS )
        {
            IDE_TEST( aProtocolContext->mResendBlock != ID_TRUE );
        }
        else
        {
            /* do nothing */
        }

        /*
         * 새로운 Block 할당
         */
        IDE_TEST(aProtocolContext->mLink->mPeerOp->mAllocBlock(aProtocolContext->mLink,
                                                               &aProtocolContext->mWriteBlock)
                 != IDE_SUCCESS);
    }

    /*
     * Protocol Finalize
     */
    IDE_TEST(cmiFinalizeProtocol(aProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidOpError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_OPERATION));
    }
    IDE_EXCEPTION_END;
    {
        IDE_ASSERT(cmiFinalizeProtocol(aProtocol) == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC cmiFlushProtocol(cmiProtocolContext *aProtocolContext, idBool aIsEnd, PDL_Time_Value *aTimeout)
{
    /*PROJ-2616*/
    IDE_TEST_CONT((cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_DUMMY) ||
                  (cmiGetLinkImpl(aProtocolContext) == CMN_LINK_IMPL_IPCDA),
                  LABEL_SKIP_FLUSH);
                  
    if (aProtocolContext->mWriteBlock != NULL)
    {
        /*
         * Write Block이 할당되어 있으면 전송
         */
        IDE_TEST(cmiWriteBlock(aProtocolContext, aIsEnd, aTimeout) != IDE_SUCCESS);
    }
    else
    {
        if ((aIsEnd == ID_TRUE) &&
            (aProtocolContext->mWriteHeader.mA5.mCmSeqNo != 0) &&
            (CMP_HEADER_PROTO_END_IS_SET(&aProtocolContext->mWriteHeader) == ID_FALSE))
        {
            /*
             * Sequence End가 전송되지 않았으면 빈 Write Block을 할당하여 전송
             */
            IDE_TEST(aProtocolContext->mLink->mPeerOp->mAllocBlock(aProtocolContext->mLink,
                                                                   &aProtocolContext->mWriteBlock)
                     != IDE_SUCCESS);

            IDE_TEST(cmiWriteBlock(aProtocolContext, ID_TRUE, aTimeout) != IDE_SUCCESS);
        }
    }

    IDE_EXCEPTION_CONT(LABEL_SKIP_FLUSH);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// fix BUG-17715
// 현재 통신 버퍼에 레코드가 들어갈 수 있는지 검사한다.
IDE_RC cmiCheckFetch(cmiProtocolContext *aProtocolContext, UInt aRecordSize)
{
    // 데이터 타입마다 부가적인 정보가 추가로 들어가며
    // char, varchar일 경우 cmtAny에 가장 많은 정보가 들어간다.
    // 따라서 계산은 가장 많은 부가 정보가 들어갔을 경우로 계산한다.

    // OPCODE(1) + STMTID(4) + RSTID(2) + ROWNO(2) + COLNO(2) + TYPEID(1) + OFFSET(4) + SIZE(2) + END(1) + DATA(x)

    return ((aProtocolContext->mWriteBlock->mCursor + 19 + aRecordSize) < CMB_BLOCK_DEFAULT_SIZE) ?
        IDE_SUCCESS : IDE_FAILURE;
}

idBool cmiCheckInVariable(cmiProtocolContext *aProtocolContext, UInt aInVariableSize)
{
    UInt sCurSize;
    
    if( aProtocolContext->mWriteBlock == NULL )
    {
        // mWriteBlock이 null일 경우는 현재 아무것도 채워지지 않은 상태이기 때문에
        // 채워지는 프로토콜에 따라서 sCurSize가 달라질수 있다.
        // 따라서, sCurSize를 가능한 최대값(CMP_HEADER_SIZE)으로 설정한다.
        // cmtInVariable은 CM 자체의 내부 타입이기 때문에 상관없을 듯...
        sCurSize = CMP_HEADER_SIZE;
    }
    else
    {
        sCurSize = aProtocolContext->mWriteBlock->mCursor;
    }
    
    // TYPE(1) + SIZE(4) + DATA(x) + DELIMETER(1)
    return ((sCurSize + 5 + aInVariableSize + 1 + ID_SIZEOF(cmpProtocol)) < CMB_BLOCK_DEFAULT_SIZE) ?
        ID_TRUE : ID_FALSE;
}

idBool cmiCheckInBinary(cmiProtocolContext *aProtocolContext, UInt aInBinarySize)
{
    UInt sCurSize;
    
    if( aProtocolContext->mWriteBlock == NULL )
    {
        sCurSize = CMP_HEADER_SIZE;
    }
    else
    {
        sCurSize = aProtocolContext->mWriteBlock->mCursor;
    }
    
    // TYPE(1) + SIZE(4) + DATA(x) + DELIMETER(1)
    return ((sCurSize + 5 + aInBinarySize + 1 + ID_SIZEOF(cmpProtocol)) < CMB_BLOCK_DEFAULT_SIZE) ?
        ID_TRUE : ID_FALSE;
}

idBool cmiCheckInBit(cmiProtocolContext *aProtocolContext, UInt aInBitSize)
{
    UInt sCurSize;

    if( aProtocolContext->mWriteBlock == NULL )
    {
        // mWriteBlock이 null일 경우는 현재 아무것도 채워지지 않은 상태이기 때문에
        // 채워지는 프로토콜에 따라서 sCurSize가 달라질수 있다.
        // 따라서, sCurSize를 가능한 최대값(CMP_HEADER_SIZE)으로 설정한다.
        // cmtInVariable은 CM 자체의 내부 타입이기 때문에 상관없을 듯...
        sCurSize = CMP_HEADER_SIZE;
    }
    else
    {
        sCurSize = aProtocolContext->mWriteBlock->mCursor;
    }

    // TYPE(1) + PRECISION(4) + SIZE(4) + DATA(x)
    return ((sCurSize + 9 + aInBitSize + ID_SIZEOF(cmpProtocol)) < CMB_BLOCK_DEFAULT_SIZE) ?
        ID_TRUE : ID_FALSE;
}

idBool cmiCheckInNibble(cmiProtocolContext *aProtocolContext, UInt aInNibbleSize)
{
    UInt sCurSize;

    if( aProtocolContext->mWriteBlock == NULL )
    {
        // mWriteBlock이 null일 경우는 현재 아무것도 채워지지 않은 상태이기 때문에
        // 채워지는 프로토콜에 따라서 sCurSize가 달라질수 있다.
        // 따라서, sCurSize를 가능한 최대값(CMP_HEADER_SIZE)으로 설정한다.
        // cmtInVariable은 CM 자체의 내부 타입이기 때문에 상관없을 듯...
        sCurSize = CMP_HEADER_SIZE;
    }
    else
    {
        sCurSize = aProtocolContext->mWriteBlock->mCursor;
    }

    // TYPE(1) + PRECISION(4) + SIZE(4) + DATA(x)
    return ((sCurSize + 9 + aInNibbleSize + ID_SIZEOF(cmpProtocol)) < CMB_BLOCK_DEFAULT_SIZE) ?
        ID_TRUE : ID_FALSE;
}

// IN 타입중에 가장 큰 헤더를 갖는 것은 IN_NIBBLE이나 IN_BIT이다.
UInt cmiGetMaxInTypeHeaderSize()
{
    // TYPE(1) + PRECISION(4) + SIZE(4) + DATA(x)
    return 9;
}

UChar *cmiGetOpName( UInt aModuleID, UInt aOpID )
{
    UChar *sOpName = NULL;

    IDE_DASSERT( aModuleID < CMP_MODULE_MAX );

    switch( aModuleID )
    {
        case CMP_MODULE_BASE:
            IDE_DASSERT( aOpID < CMP_OP_BASE_MAX );
            sOpName = (UChar*)gCmpOpBaseMap[aOpID].mCmpOpName;
            break;

        case CMP_MODULE_DB:
            IDE_DASSERT( aOpID < CMP_OP_DB_MAX );
            sOpName = (UChar*)gCmpOpDBMap[aOpID].mCmpOpName;
            break;

        case CMP_MODULE_RP:
            IDE_DASSERT( aOpID < CMP_OP_RP_MAX );
            sOpName = (UChar*)gCmpOpRPMap[aOpID].mCmpOpName;
            break;

        case CMP_MODULE_DK:
            IDE_DASSERT( aOpID < CMP_OP_DK_MAX );
            sOpName = (UChar*)gCmpOpDKMap[aOpID].mCmpOpName;
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return sOpName;
}

cmiLinkImpl cmiGetLinkImpl(cmiProtocolContext *aProtocolContext)
{
    return (cmiLinkImpl)(aProtocolContext->mLink->mLink.mImpl);
}

/**
 * cmpCollectionDBBindColumnInfoGetResult의 최대 크기를 얻는다.
 *
 * cmtAny인 이름 값은 최대 50 길이인 것으로 본다.
 *
 * @return cmpCollectionDBBindColumnInfoGetResult의 최대 크기
 */
UInt cmiGetBindColumnInfoStructSize( void )
{
    return ID_SIZEOF(UInt)  /* mDataType */
         + ID_SIZEOF(UInt)  /* mLanguage */
         + ID_SIZEOF(UChar) /* mArguments */
         + ID_SIZEOF(SInt)  /* mPrecision */
         + ID_SIZEOF(SInt)  /* mScale */
         + ID_SIZEOF(UChar) /* mFlags */
         + (56 * 7);        /* cmtAny(1+4+50+1=56) * 7 */
}

// bug-19279 remote sysdba enable + sys can kill session
// client가 원격에서 접속했으면 true
// local에서 접속했으면 false 반환
// tcp 방식이고 IP가 127.0.0.1이 아닌 경우에 원격으로 간주한다
// 주의: local이라도 127.0.0.l이 아닌 주소라면 원격으로 간주.
// remote sysdba 를 허용할지 여부를 결정할 때 사용
IDE_RC cmiCheckRemoteAccess(cmiLink* aLink, idBool* aIsRemote)
{
    struct sockaddr*         sAddrCommon = NULL ;
    struct sockaddr_storage  sAddr;
    struct sockaddr_in*      sAddr4 = NULL;
    struct sockaddr_in6*     sAddr6 = NULL;
    UInt                     sAddrInt = 0;
    UInt*                    sUIntPtr = NULL;

    *aIsRemote = ID_FALSE;
    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    if ((aLink->mImpl == CMN_LINK_IMPL_TCP) ||
        (aLink->mImpl == CMN_LINK_IMPL_SSL) ||
        (aLink->mImpl == CMN_LINK_IMPL_IB))
    {
        /* proj-1538 ipv6 */
        idlOS::memset(&sAddr, 0x00, ID_SIZEOF(sAddr));
        IDE_TEST(cmiGetLinkInfo(aLink, (SChar *)&sAddr, ID_SIZEOF(sAddr),
                                CMI_LINK_INFO_REMOTE_SOCKADDR)
                 != IDE_SUCCESS);

        /* bug-30541: ipv6 code review bug.
         * use sockaddr.sa_family instead of sockaddr_storage.ss_family.
         * because the name is different on AIX 5.3 tl1 as __ss_family
         */
        sAddrCommon = (struct sockaddr*)&sAddr;
        /* ipv4 */
        if (sAddrCommon->sa_family == AF_INET)
        {
            sAddr4 = (struct sockaddr_in*)&sAddr;
            sAddrInt = *((UInt*)&sAddr4->sin_addr); // network byte order
            if (sAddrInt != htonl(INADDR_LOOPBACK))
            {
                *aIsRemote = ID_TRUE; /* remote */
            }
        }
        /* ipv6 including v4mapped */
        else
        {
            sAddr6 = (struct sockaddr_in6*)&sAddr;
            /* if v4mapped-ipv6, extract ipv4 (4bytes)
               ex) ::ffff:127.0.0.1 => 127.0.0.1 */
            if (idlOS::in6_is_addr_v4mapped(&(sAddr6->sin6_addr)))
            {
                /* sin6_addr: 16bytes => 4 UInts */
                sUIntPtr = (UInt*)&(sAddr6->sin6_addr);
                sAddrInt = *(sUIntPtr + 3);
                if (sAddrInt != htonl(INADDR_LOOPBACK))
                {
                    *aIsRemote = ID_TRUE; /* remote */
                }
            }
            /* pure ipv6 addr */
            else
            {
                if (idlOS::in6_is_addr_loopback(&sAddr6->sin6_addr) != ID_TRUE)
                {
                    *aIsRemote = ID_TRUE; /* remote */
                }
            }
        }
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// fix BUG-30835
idBool cmiIsValidIPFormat(SChar * aIP)
{
    struct  sockaddr sSockAddr;
    SChar *          sCurrentPos = NULL;
    SChar            sIP[IDL_IP_ADDR_MAX_LEN + 1] = {0,};
    idBool           sRes = ID_FALSE;
    UInt             sIPaddrLen;

    if (aIP != NULL)
    {
        /* trim zone index because inet_pton does not support rfc4007 zone index */
        sIPaddrLen = IDL_MIN((IDL_IP_ADDR_MAX_LEN), idlOS::strlen(aIP));
        
        idlOS::strncpy(sIP, aIP, sIPaddrLen);
                
        for(sCurrentPos = sIP; *sCurrentPos != '\0'; sCurrentPos++)
        {
            if(*sCurrentPos == '%')
            {
                *sCurrentPos = '\0';
                break;
            }
        }
        
        /* inet_pton : success is not zero  */
        if( (PDL_OS::inet_pton(AF_INET, sIP, &sSockAddr) > 0) || 
            (PDL_OS::inet_pton(AF_INET6, sIP, &sSockAddr) > 0) )
        {
            sRes = ID_TRUE;
        }
        else
        {
            sRes = ID_FALSE;
        }
    }
    
    return sRes;
}

/***********************************************************
 * proj_2160 cm_type removal
 * cmbBlock 포인터 2개를 NULL로 만든다
 * cmiAllocCmBlock을 호출하기 전에 이 함수를 반드시 먼저
 * 호출해서 cmbBlock 할당이 제대로 되도록 해야 한다.
***********************************************************/
IDE_RC cmiMakeCmBlockNull(cmiProtocolContext *aCtx)
{
    aCtx->mLink = NULL;
    aCtx->mReadBlock = NULL;
    aCtx->mWriteBlock = NULL;
    return IDE_SUCCESS;
}

/***********************************************************
 * proj_2160 cm_type removal
 **********************************************************
 *  cmiAllocCmBlock:
 * 1. 이 함수는 예전의 cmiAddSession() 과
 *  cmiInitializeProtocolContext() 를 대체하는 함수이다
 * 2. 이 함수는 cmiProtocolContext를 초기화하고
 *  2개의 cmbBlock(recv, send)을 할당한다.
 * 3. 주의할 점은 호출하기 전에 cmbBlock 포인터가 NULL로
 *  초기화되어 있어야 한다는 것이다(cmiMakeCmBlockNull)
 * 4. A5 client가 접속하는 경우에도 서버에서는 이 함수를
 *  사용하는데 문제가 없다(A7에서 A5로 전환됨)
 **********************************************************
 *  변경사항:
 * 1. 예전에는 송수신마다 cmbBlock이 할당/해제가 되었는데,
 *  A7부터는 한번만 할당한 후 연결이 끊길때까지
 *  계속 유지되도록 변경되었다.
 * 2. cmiProtocolContext도 한번만 초기화를 해야한다. 이유는
 *  패킷일련번호를 세션내에서 계속 유지해야 하기 때문이다
 * 3. cmmSession 구조체는 더이상 사용하지 않는다
 **********************************************************
 *  사용방법(함수 호출 순서):
 *  1. cmiMakeCmBlockNull(ctx);   : cmbBlock 포인터 NULL 세팅
 *  2. cmiAllocLink(&link);       : Link 구조체 할당
 *  3. cmiAllocCmBlock(ctx, link);: cmbBlock 2개 할당
 *  4.  connected   ...           : 연결 성공
 *  5.  send/recv   ...           : cmbBlock을 통해 송수신
 *  6.  disconnected ..           : 연결 종료
 *  7. cmiFreeCmBlock(ctx);       : cmbBlock 2개 해제
 *  8. cmiFreeLink(link);         : Link 구조체 해제
***********************************************************/
IDE_RC cmiAllocCmBlock(cmiProtocolContext* aCtx,
                       UChar               aModuleID,
                       cmiLink*            aLink,
                       void*               aOwner,
                       idBool              aResendBlock)
{
    cmbPool*   sPool;

    // cmnLink has to be prepared
    IDE_TEST(aLink == NULL);
    // memory allocation allowed only once
    IDE_TEST(aCtx->mReadBlock != NULL);

    aCtx->mModule  = gCmpModule[aModuleID];
    aCtx->mLink    = (cmnLinkPeer*)aLink;
    aCtx->mOwner   = aOwner;
    aCtx->mIsAddReadBlock = ID_FALSE;
    aCtx->mSession = NULL; // deprecated
    
    /*PROJ-2616*/
    IDE_TEST_CONT(cmiGetLinkImpl(aCtx) == CMN_LINK_IMPL_IPCDA,
                  ContAllocCmBlock);

    // allocate readBlock, writeBlock statically.
    sPool = aCtx->mLink->mPool;
    IDE_TEST(sPool->mOp->mAllocBlock(
            sPool, &aCtx->mReadBlock) != IDE_SUCCESS);
    IDE_TEST(sPool->mOp->mAllocBlock(
            sPool, &aCtx->mWriteBlock) != IDE_SUCCESS);
    aCtx->mWriteBlock->mDataSize = CMP_HEADER_SIZE;
    aCtx->mWriteBlock->mCursor   = CMP_HEADER_SIZE;

    cmpHeaderInitialize(&aCtx->mWriteHeader);
    CMP_MARSHAL_STATE_INITIALIZE(aCtx->mMarshalState);
    aCtx->mProtocol.mFinalizeFunction = (void *)cmpArgNULL;

    IDU_LIST_INIT(&aCtx->mReadBlockList);
    IDU_LIST_INIT(&aCtx->mWriteBlockList);

    /*PROJ-2616*/
    IDE_EXCEPTION_CONT(ContAllocCmBlock);

    aCtx->mListLength             = 0;
    aCtx->mCmSeqNo                = 0; // started from 0
    aCtx->mIsDisconnect           = ID_FALSE;
    aCtx->mSessionCloseNeeded     = ID_FALSE;
    aCtx->mIPDASpinMaxCount       = CMI_IPCDA_SPIN_MIN_LOOP; /*PROJ-2616*/
    
    /* BUG-45184 */
    aCtx->mSendDataSize           = 0;
    aCtx->mReceiveDataSize        = 0;
    aCtx->mSendDataCount          = 0;
    aCtx->mReceiveDataCount       = 0;
    
    cmiDisableCompress( aCtx );
    /* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
    /* 압축 해제 방식은 default 값이 LZO */ 
    cmiSetDecompressType( aCtx, CMI_COMPRESS_LZO );
    
    /* BUG-38102*/
    cmiDisableEncrypt( aCtx );

    aCtx->mResendBlock = aResendBlock;

    aCtx->mProtocol.mClientLastOpID = CMP_OP_DB_LAST_OP_ID_OF_VER_7_1;  /* PROJ-2733-Protocol */
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmiAllocCmBlockForA5( cmiProtocolContext* aCtx,
                             UChar               aModuleID,
                             cmiLink*            aLink,
                             void*               aOwner,
                             idBool              aResendBlock )
 {
     cmbPool*   sPool = NULL;

     // cmnLink has to be prepared
     IDE_TEST(aLink == NULL);
     // memory allocation allowed only once
     IDE_TEST(aCtx->mReadBlock != NULL);

     aCtx->mModule  = gCmpModule[aModuleID];
     aCtx->mLink    = (cmnLinkPeer*)aLink;
     aCtx->mOwner   = aOwner;
     aCtx->mIsAddReadBlock = ID_FALSE;
     aCtx->mSession = NULL; // deprecated

     // allocate readBlock, writeBlock statically.
     sPool = aCtx->mLink->mPool;
     IDE_TEST(sPool->mOp->mAllocBlock(
             sPool, &aCtx->mReadBlock) != IDE_SUCCESS);

     IDE_TEST(sPool->mOp->mAllocBlock(
             sPool, &aCtx->mWriteBlock) != IDE_SUCCESS);

     aCtx->mWriteBlock->mDataSize = CMP_HEADER_SIZE;
     aCtx->mWriteBlock->mCursor   = CMP_HEADER_SIZE;

     cmpHeaderInitializeForA5(&aCtx->mWriteHeader);
     aCtx->mWriteHeader.mA5.mModuleID = aModuleID;

     // initialize for readheader
     cmpHeaderInitializeForA5(&aCtx->mReadHeader);
     aCtx->mReadHeader.mA5.mModuleID = aModuleID;

     CMP_MARSHAL_STATE_INITIALIZE(aCtx->mMarshalState);
     aCtx->mProtocol.mFinalizeFunction = (void *)cmpArgNULL;

     IDU_LIST_INIT(&aCtx->mReadBlockList);
     IDU_LIST_INIT(&aCtx->mWriteBlockList);

     aCtx->mListLength   = 0;
     aCtx->mCmSeqNo      = 0; // started from 0
     aCtx->mIsDisconnect = ID_FALSE;
     aCtx->mSessionCloseNeeded  = ID_FALSE;
     
     /* BUG-45184 */
     aCtx->mSendDataSize           = 0;
     aCtx->mReceiveDataSize        = 0;
     aCtx->mSendDataCount          = 0;
     aCtx->mReceiveDataCount       = 0;
     
     cmiDisableCompress( aCtx );
     /* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
     /* 압축 해제 방식은 default 값이 LZO */ 
     cmiSetDecompressType( aCtx, CMI_COMPRESS_LZO );

     /* BUG-38102*/
     cmiDisableEncrypt( aCtx );

     aCtx->mResendBlock = aResendBlock;

     aCtx->mProtocol.mClientLastOpID = 0;  /* PROJ-2733-Protocol Unused at A5 */

     return IDE_SUCCESS;
     IDE_EXCEPTION_END;
     return IDE_FAILURE;
 }

/***********************************************************
 * 이 함수는 세션이 종료된후에는 메모리 반납을 위해
 * 반드시 호출되어야 한다
 * 내부에서는 A7과 A5 세션을 동시에 처리하도록 되어 있다
***********************************************************/
IDE_RC cmiFreeCmBlock(cmiProtocolContext* aCtx)
{
    cmbPool*  sPool;

    iduListNode     * sIterator = NULL;
    iduListNode     * sNodeNext = NULL;
    cmbBlock        * sPendingBlock = NULL;

    /*PROJ-2616*/
    IDE_TEST_CONT(cmiGetLinkImpl(aCtx) == CMN_LINK_IMPL_IPCDA,
                  ContFreeCmBlock);

    IDE_TEST(aCtx->mLink == NULL);

    if (aCtx->mLink->mLink.mPacketType != CMP_PACKET_TYPE_A5)
    {
        sPool = aCtx->mLink->mPool;
        IDE_ASSERT(aCtx->mReadBlock != NULL && aCtx->mWriteBlock != NULL);

        // timeout으로 세션이 끊길경우 에러메시지를 포함한
        // 응답 데이터가 아직 cmBlock에 남아 있다. 이를 전송
        if (aCtx->mWriteBlock->mCursor > CMP_HEADER_SIZE)
        {
            (void)cmiSend(aCtx, ID_TRUE);
        }

        IDE_TEST(sPool->mOp->mFreeBlock(sPool, aCtx->mReadBlock)
                 != IDE_SUCCESS);
        aCtx->mReadBlock = NULL;

        IDE_TEST(sPool->mOp->mFreeBlock(sPool, aCtx->mWriteBlock)
                 != IDE_SUCCESS);
        aCtx->mWriteBlock = NULL;
    }
    else
    {
        if (aCtx->mReadBlock != NULL)
        {
            IDU_LIST_ADD_LAST(&aCtx->mReadBlockList,
                              &aCtx->mReadBlock->mListNode);
        }

        aCtx->mReadBlock = NULL;
        IDE_TEST_RAISE(cmiFlushProtocol(aCtx, ID_TRUE)
                       != IDE_SUCCESS, FlushFail);
        IDE_TEST(cmiFinalizeProtocol(&aCtx->mProtocol)
                 != IDE_SUCCESS);

        /*
         * Free All Read Blocks
         */
        IDE_TEST(cmiFreeReadBlock(aCtx) != IDE_SUCCESS);
    }

    /* BUG-46608 mWriteBlockList 에 할당되어 있는 Block 이 있으면 해제 한다. */
    IDU_LIST_ITERATE_SAFE(&aCtx->mWriteBlockList, sIterator, sNodeNext)
    {
        sPool = aCtx->mLink->mPool;

        sPendingBlock = (cmbBlock *)sIterator->mObj;
        IDE_TEST( sPool->mOp->mFreeBlock(sPool, sPendingBlock) != IDE_SUCCESS );
        aCtx->mListLength--;
    }
    
    IDE_EXCEPTION_CONT(ContFreeCmBlock);

    return IDE_SUCCESS;

    IDE_EXCEPTION(FlushFail);
    {
        IDE_PUSH();

        IDE_ASSERT(cmiFinalizeProtocol(&aCtx->mProtocol) == IDE_SUCCESS);

        IDE_ASSERT(cmiFreeReadBlock(aCtx) == IDE_SUCCESS);

        IDE_POP();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * CM 프로토콜의 A5 버전의 Handshake를 처리한다.
 */ 
static IDE_RC cmiHandshakeA5( cmiProtocolContext * aCtx )
{
    UChar                 sOpID;
    cmpArgBASEHandshake * sResultA5 = NULL;
    cmiProtocol           sProtocol;
    SInt                  sStage = 0;
    
    /* PROJ-2563 */
    if ( aCtx->mReadHeader.mA5.mModuleID == CMP_MODULE_RP )
    {
        /* do nothing */
        /* 서버로 A5프로토콜로 접속을 시도하는 클라이언트가 있을 때,
         * UL인경우 ACK를 전송하며, UL외의 다른 모듈일 경우 접속을 끊는다.
         * a631에서 A5를 이용한 RP 클라이언트를 오류로 인식하므로,
         * RP클라이언트로 접속할 때 오류로 인식않게 하며, 또한 ACK를 전송하지 않는 것으로 수정한다.
         */
    }
    else
    {
        IDE_TEST_RAISE( aCtx->mReadHeader.mA5.mModuleID != CMP_MODULE_BASE,
                        INVALID_MODULE );
        
        CMI_RD1( aCtx, sOpID );
        IDE_TEST_RAISE( sOpID != CMP_OP_BASE_Handshake, INVALID_PROTOCOL );
            
        CMI_SKIP_READ_BLOCK(aCtx,3); //cmpArgBASEHandshake
    
        /* initialize writeBlock's header using recved header */
        aCtx->mWriteHeader.mA5.mHeaderSign      = CMP_HEADER_SIGN_A5;
        aCtx->mWriteHeader.mA5.mModuleID        = aCtx->mReadHeader.mA5.mModuleID;
        aCtx->mWriteHeader.mA5.mModuleVersion   = aCtx->mReadHeader.mA5.mModuleVersion;
        aCtx->mWriteHeader.mA5.mSourceSessionID = aCtx->mReadHeader.mA5.mTargetSessionID;
        aCtx->mWriteHeader.mA5.mTargetSessionID = aCtx->mReadHeader.mA5.mSourceSessionID;
    
        /* mModule has to be set properly for cmiWriteProtocol */
        aCtx->mModule = gCmpModule[ CMP_MODULE_BASE ];
        CMI_PROTOCOL_INITIALIZE( sProtocol, sResultA5, BASE, Handshake );
        sStage = 1;

        sResultA5->mBaseVersion   = CMP_VER_BASE_MAX - 1;
        sResultA5->mModuleID      = CMP_MODULE_DB;
        sResultA5->mModuleVersion = CM_PATCH_VERSION;

        IDE_TEST( cmiWriteProtocol( aCtx, &sProtocol ) != IDE_SUCCESS );

        IDE_TEST( cmiFlushProtocol( aCtx, ID_TRUE ) != IDE_SUCCESS );
    
        sStage = 0;
        IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_MODULE )
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_INVALID_MODULE ) );
    }
    IDE_EXCEPTION( INVALID_PROTOCOL )
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 * CM Block except CM Packet header is compressed by using iduCompression. then
 * Original CM Block is replaced with compressed one.
 */
/* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
static IDE_RC cmiCompressLZOCmBlock( cmiProtocolContext * aCtx,
                                     cmbBlock           * aBlock )
{
    UChar sOutBuffer[ IDU_COMPRESSION_MAX_OUTSIZE( CMB_BLOCK_DEFAULT_SIZE ) ] = { 0, };
    ULong sWorkMemory[ IDU_COMPRESSION_WORK_SIZE / ID_SIZEOF(ULong) ] = { 0, };
    UInt sCompressSize = 0;

    IDE_TEST_RAISE(
        iduCompression::compress( aBlock->mData + CMP_HEADER_SIZE,
                                  aBlock->mCursor - CMP_HEADER_SIZE,
                                  sOutBuffer,
                                  sizeof( sOutBuffer ),
                                  &sCompressSize,
                                  sWorkMemory )
        != IDE_SUCCESS, COMPRESS_ERROR );

    aBlock->mCursor = CMP_HEADER_SIZE;
    CMI_WCP( aCtx, sOutBuffer, sCompressSize );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( COMPRESS_ERROR )
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_COMPRESS_DATA_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmiCompressLZ4CmBlock( cmiProtocolContext * aCtx,
                                     cmbBlock           * aBlock )
{
    SInt  sCompressSize = 0;
    UChar sCompressBuffer[ IDU_LZ4_COMPRESSBOUND( CMB_BLOCK_DEFAULT_SIZE ) ];
    SInt  sMaxCompressSize = IDU_LZ4_COMPRESSBOUND( CMB_BLOCK_DEFAULT_SIZE );

    /* BUG-47078 Apply LZ4 Algorithm */
    sCompressSize = iduLZ4_compress( (SChar*)aBlock->mData + CMP_HEADER_SIZE,
                                     (SChar*)sCompressBuffer,
                                     aBlock->mCursor - CMP_HEADER_SIZE,
                                     sMaxCompressSize,
                                     aCtx->mCompressLevel );

    IDU_FIT_POINT_RAISE( "cmiCompressCmBlock::COMPRESS_ERROR",
                         COMPRESS_ERROR );

    IDE_TEST_RAISE( ( sCompressSize <= 0 ) ||
                    ( sCompressSize > sMaxCompressSize ),
                    COMPRESS_ERROR );

    aBlock->mCursor = CMP_HEADER_SIZE;
    CMI_WCP( aCtx, sCompressBuffer, sCompressSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( COMPRESS_ERROR )
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_COMPRESS_DATA_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmiCompressCmBlock( cmpHeader          * aHeader,
                                  cmiProtocolContext * aCtx,
                                  cmbBlock           * aBlock )
{
    switch ( aCtx->mCompressType )
    {
        case CMI_COMPRESS_NONE :
            break;
            
        case CMI_COMPRESS_LZO :
            if ( cmiCompressLZOCmBlock( aCtx, aBlock ) == IDE_SUCCESS )
            {
                CMP_HEADER_FLAG_SET_COMPRESS( aHeader );        
            }
            else
            {
                /* BUG-47020 압축에 실패하더라도 에러만 출력 후 비 압축 로그로 보낸다. 
                 * 패킷 압축은 이중화에서만 하므로 altibase_rp.log 에 출력해준다. */
                 IDE_ERRLOG( IDE_RP_0 );
            }
            break;
            
        case CMI_COMPRESS_LZ4 :
            if ( cmiCompressLZ4CmBlock( aCtx, aBlock ) == IDE_SUCCESS )
            {
                CMP_HEADER_FLAG_SET_COMPRESS( aHeader );        
            }
            else
            {
                /* BUG-47020 압축에 실패하더라도 에러만 출력 후 비 압축 로그로 보낸다. 
                 * 패킷 압축은 이중화에서만 하므로 altibase_rp.log 에 출력해준다. */
                 IDE_ERRLOG( IDE_RP_0 );
            }
            break; 
            
        default :
            IDE_DASSERT( 0 );
            IDE_RAISE( InvalidCompressType );
            break;             
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidCompressType )
    {
        ideLog::log( IDE_RP_0, CM_TRC_COMPRESS_TYPE_ERROR, aCtx->mCompressType );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * CM Block except CM Packet header is decompressed by using iduCompression.
 * then Original CM Block is replaced with decompressed one.
 */
/* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
static IDE_RC cmiDecompressLZOCmBlock( cmbBlock           * aBlock,
                                       UInt                 aDataLength )
{
    UChar sOutBuffer[ CMB_BLOCK_DEFAULT_SIZE ] = { 0, };
    UInt sDecompressSize = 0;

    IDE_TEST_RAISE(
        iduCompression::decompress( aBlock->mData + CMP_HEADER_SIZE,
                                    aDataLength,
                                    sOutBuffer,
                                    sizeof( sOutBuffer ),
                                    &sDecompressSize )
        != IDE_SUCCESS, DECOMPRESS_ERROR );

    idlOS::memcpy( aBlock->mData + CMP_HEADER_SIZE,
                   sOutBuffer,
                   sDecompressSize );
    aBlock->mDataSize = sDecompressSize + CMP_HEADER_SIZE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( DECOMPRESS_ERROR )
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_DECOMPRESS_DATA_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmiDecompressLZ4CmBlock( cmbBlock           * aBlock,
                                       UInt                 aDataLength )
{
    SInt  sDecompressSize = 0;
    UChar sDecompressBuffer[ IDU_LZ4_COMPRESSBOUND( CMB_BLOCK_DEFAULT_SIZE ) ];
    SInt  sMaxDecompressSize = IDU_LZ4_COMPRESSBOUND( CMB_BLOCK_DEFAULT_SIZE );

    /* BUG-47078 Apply LZ4 Algorithm */
    sDecompressSize = iduLZ4_decompress( (SChar*)aBlock->mData + CMP_HEADER_SIZE,
                                         (SChar*)sDecompressBuffer,
                                         aDataLength,
                                         sMaxDecompressSize );
    IDE_TEST_RAISE( ( sDecompressSize <= 0 ) ||
                    ( sDecompressSize > sMaxDecompressSize ),
                    DECOMPRESS_ERROR );

    idlOS::memcpy( aBlock->mData + CMP_HEADER_SIZE,
                   sDecompressBuffer,
                   sDecompressSize );
    aBlock->mDataSize = sDecompressSize + CMP_HEADER_SIZE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( DECOMPRESS_ERROR )
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_DECOMPRESS_DATA_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmiDecompressCmBlock( cmiProtocolContext * aCtx,
                                    UInt                 aDataLength )
{
    switch ( aCtx->mDecompressType )
    {
        case CMI_COMPRESS_NONE :
            break;
            
        case CMI_COMPRESS_LZO :
            IDE_TEST( cmiDecompressLZOCmBlock( aCtx->mReadBlock,
                                               aDataLength )
                      != IDE_SUCCESS );
            break;
            
        case CMI_COMPRESS_LZ4 :
            IDE_TEST( cmiDecompressLZ4CmBlock( aCtx->mReadBlock,
                                               aDataLength )
                      != IDE_SUCCESS );
            break;
            
        default :
            IDE_DASSERT( 0 );
            IDE_RAISE( InvalidDecompressType );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InvalidDecompressType )
    {
        ideLog::log( IDE_RP_0, CM_TRC_DECOMPRESS_TYPE_ERROR, aCtx->mDecompressType );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 * CM Block except CM Packet header is encrypted by using RC4. 
 * Then original CM Block is replaced with encrypted one.
 */
static IDE_RC cmiEncryptCmBlock( cmbBlock   * aBlock )
{
    UChar      *sSrcData     = NULL;
    UChar       sEncryptedData[ CMB_BLOCK_DEFAULT_SIZE ] = {0, };
    idsRC4      sRC4;
    RC4Context  sRC4Ctx;

    IDE_TEST_CONT( aBlock->mCursor == CMP_HEADER_SIZE, noDataToEncrypt );

    sRC4.setKey( &sRC4Ctx, (UChar *)RC4_KEY, RC4_KEY_LEN );

    sSrcData = (UChar *)aBlock->mData + CMP_HEADER_SIZE;

    sRC4.convert( &sRC4Ctx, 
                  (const UChar *)sSrcData, 
                  aBlock->mCursor - CMP_HEADER_SIZE, 
                  sEncryptedData );    
        
    idlOS::memcpy( aBlock->mData + CMP_HEADER_SIZE,
                   sEncryptedData,
                   aBlock->mCursor - CMP_HEADER_SIZE );

    IDE_EXCEPTION_CONT( noDataToEncrypt );
    
    return IDE_SUCCESS;
}

/*
 * CM Block except CM Packet header is decrypted by using RC4.
 * Then original CM Block is replaced with decrypted one.
 */
static IDE_RC cmiDecryptCmBlock( cmbBlock   * aBlock, 
                                 UInt         aDataLength )
{
    UChar       sDecryptedData[ CMB_BLOCK_DEFAULT_SIZE ] = {0, };
    idsRC4      sRC4;
    RC4Context  sRC4Ctx;

    sRC4.setKey( &sRC4Ctx, (UChar *)RC4_KEY, RC4_KEY_LEN );

    sRC4.convert( &sRC4Ctx,
                  aBlock->mData + CMP_HEADER_SIZE,
                  aDataLength,
                  sDecryptedData );

    idlOS::memcpy( aBlock->mData + CMP_HEADER_SIZE,
                   sDecryptedData,
                   aDataLength );

    aBlock->mDataSize = aDataLength + CMP_HEADER_SIZE;

    return IDE_SUCCESS;
}

/*************************************************************
 * proj_2160 cm_type removal
 * 1. 이 함수는 A7 이상 전용이다
 * 2. 패킷 수신및 해당 프로토콜에 대응하는 콜백함수를
 *  자동 호출하기 위해 사용되는 함수이다
 * 3. 패킷 한개를 읽어들인 후 패킷에 여러개의 프로토콜이 들어
 *  있을 수 있으므로 반복문에서 반복적으로 콜백을 호출한다
 * 4. 반복문이 끝나는 조건은 패킷 데이터를 전부 다 읽은 경우이다
 * 5. 분할 패킷을 수신한 경우(큰 프로토콜)는 여기서 처리하지 않으며
 *  해당 프로토콜 콜백안에서 반복문을 사용하여 알아서 처리한다
**************************************************************
 * 6. 그룹 프로토콜 (분할 패킷인데 패킷마다 완성된 형태를 가짐)인
 *  경우 콜백 내부에서 반복문으로 처리하기가 힘들기 때문에
 *  여기서 goto를 사용하여 특별처리 한다 (ex) 메시지 프로토콜)
 * 7. A5 client가 접속하면 handshakeProtocol을 호출하게 되며
 *  해당 콜백안에서 A5 응답을 주면서 A5로 전환이 되게 된다
 * 8. 패킷마다 세션내 고유일련번호(수신시마다 1씩 증가)를 부여하여
 *  잘못되거나 중복된 패킷이 수신되는 것을 막는다
 *  (참고로 A5에서는 분할 패킷에 대해서만 일련번호를 부여했었다)
 * 9. RP(ALA 포함) 모듈도 본 함수를 공동 사용한다. 다만 RP의 경우
 *  콜백구조가 아니기 때문에 패킷 수신후 바로 함수를 빠져나간다
 * 10. CMI_DUMP: 개발자 디버깅 용도로 넣어두었다. 단순히 수신
 *  프로토콜 이름과 패킷길이 정보만 출력한다. 추후에 alter system
 *  으로 패킷덤프를 할수 있도록 변경하는 것도 좋을 것 같다
 * 11. 이 함수는 A5의 cmiReadBlock + cmiReadProtocolAndCallback
 *  을 대체한다
*************************************************************/
// #define CMI_DUMP 1
IDE_RC cmiRecv(cmiProtocolContext* aCtx,
               void*           aUserContext,
               PDL_Time_Value* aTimeout,
               void*           aTask)
{
    cmpCallbackFunction  sCallbackFunction;
    cmpHeader*           sHeader = &(aCtx->mReadHeader);
    UInt                 sCmSeqNo;
    UChar                sOpID;
    IDE_RC               sRet = IDE_SUCCESS;

beginToRecv:
    aCtx->mReadBlock->mDataSize  = 0;
    aCtx->mReadBlock->mCursor   = 0;

    IDU_FIT_POINT_RAISE( "cmiRecv::Server::Disconnected", Disconnected ); 
    IDU_FIT_POINT( "cmiRecv::Server::Recv" ); 
    
    IDE_TEST_RAISE(aCtx->mIsDisconnect == ID_TRUE, Disconnected);
    IDE_TEST(aCtx->mLink->mPeerOp->mRecv(aCtx->mLink,
                                         &aCtx->mReadBlock,
                                         sHeader, 
                                         aTimeout) != IDE_SUCCESS);
    
    /* BUG-45184 */
    aCtx->mReceiveDataSize += aCtx->mReadBlock->mDataSize;
    aCtx->mReceiveDataCount++;
    
    /* bug-33841: ipc thread's state is wrongly displayed.
       IPC인 경우 패킷 수신후에 execute 상태로 변경.
       RP, DK 모듈인 경우 SetExecute와 관계 없다 */
    if (aCtx->mModule->mModuleID == CMP_MODULE_DB)
    {
        (void) gCMCallbackSetExecute(aUserContext, aTask);
    }

    // 이 if문은 A5 client가 접속한 경우에 한해 최초 한번만 수행된다
    // call A7's DB handshake directly.
    if ( cmiGetPacketType( aCtx ) == CMP_PACKET_TYPE_A5 )
    {
        IDE_TEST( cmiHandshakeA5( aCtx ) != IDE_SUCCESS );

        aCtx->mIsAddReadBlock = ID_FALSE; // this is needed.

        IDE_RAISE( CmiRecvReturn );
    }

    /* BUG-38102 */
    if ( CMP_HEADER_FLAG_ENCRYPT_IS_SET( sHeader ) == ID_TRUE )
    {
        IDE_TEST( cmiDecryptCmBlock( aCtx->mReadBlock, 
                                     sHeader->mA7.mPayloadLength )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    if ( CMP_HEADER_FLAG_COMPRESS_IS_SET( sHeader ) == ID_TRUE )
    {
        /* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
        IDE_TEST( cmiDecompressCmBlock( aCtx,
                                        sHeader->mA7.mPayloadLength )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }
    
    sCmSeqNo = CMP_HEADER_SEQ_NO(sHeader);
#ifdef CMI_DUMP
    ideLog::logLine(IDE_CM_4, "[%5d] recv [%5d]",
                    sCmSeqNo, sHeader->mA7.mPayloadLength);
#endif

    /* BUG-41909 Add dump CM block when a packet error occurs */
    IDU_FIT_POINT_RAISE( "cmiRecv::Server::InvalidProtocolSeqNo", InvalidProtocolSeqNo );

    // 모든 패킷은 세션내애서 고유일련번호를 갖는다.
    // 범위: 0 ~ 0x7fffffff, 최대값에 다다르면 0부터 다시 시작된다
    IDE_TEST_RAISE(sCmSeqNo != aCtx->mCmSeqNo, InvalidProtocolSeqNo);
    if (aCtx->mCmSeqNo == CMP_HEADER_MAX_SEQ_NO)
    {
        aCtx->mCmSeqNo = 0;
    }
    else
    {
        aCtx->mCmSeqNo++;
    }

    // RP(ALA) 모듈에서는 callback을 사용하지 않는다.
    // 따라서, RP인 경우 callback 호출없이 바로 return 한다.
    if ( ( aCtx->mModule->mModuleID == CMP_MODULE_RP ) ||
         ( aCtx->mModule->mModuleID == CMP_MODULE_DK ) )
    {
        IDE_RAISE(CmiRecvReturn);
    }

    while (1)
    {
        CMI_RD1(aCtx, sOpID);
        IDE_TEST_RAISE(sOpID >= aCtx->mModule->mOpMax, InvalidOpError);  /* BUG-45804 */
#ifdef CMI_DUMP
        ideLog::logLine(IDE_CM_4, "%s", gCmpOpDBMap[sOpID].mCmpOpName);
#endif

        aCtx->mSessionCloseNeeded  = ID_FALSE;
        /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
        aCtx->mProtocol.mOpID      = sOpID;
        sCallbackFunction = aCtx->mModule->mCallbackFunction[sOpID];
        
        sRet = sCallbackFunction(aCtx,
                                 &aCtx->mProtocol,
                                 aCtx->mOwner,
                                 aUserContext);

        if (aCtx->mModule->mModuleID == CMP_MODULE_DB)
        {
            CMP_DB_PROTOCOL_STAT_ADD(sOpID, 1);
        }
        // dequeue의 경우 IDE_CM_STOP이 반환될 수 있다.
        IDE_TEST_RAISE(sRet != IDE_SUCCESS, CmiRecvReturn);

        /* BUG-41909 Add dump CM block when a packet error occurs */
        IDU_FIT_POINT_RAISE( "cmiRecv::Server::MarshalErr", MarshalErr );

        // 수신한 패킷에 대한 모든 프로토콜 처리가 끝난 경우
        if (aCtx->mReadBlock->mCursor == aCtx->mReadBlock->mDataSize)
        {
            break;
        }
        // 프로토콜 해석이 잘못되어 cursor가 패킷을 넘어간 경우
        else if (aCtx->mReadBlock->mCursor > aCtx->mReadBlock->mDataSize)
        {
            IDE_RAISE(MarshalErr);
        }

        IDE_TEST_RAISE(aCtx->mIsDisconnect == ID_TRUE, Disconnected);
    }

    // special 프로토콜 처리(Message, LobPut protocol)
    // msg와 lobput 프로토콜의 경우 프로토콜 group으로 수신이 가능함
    // (패킷마다 각각 opID를 가지며 패킷헤더에 종료 flag가 0이다)
    // 이들은 여러번 수신하더라도 마지막 한번만 응답송신해야 한다.
    if (CMP_HEADER_PROTO_END_IS_SET(sHeader) == ID_FALSE)
    {
        goto beginToRecv;
    }

    IDE_EXCEPTION_CONT(CmiRecvReturn);

    return sRet; // IDE_SUCCESS, IDE_FAILURE, IDE_CM_STOP

    IDE_EXCEPTION(Disconnected)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(InvalidProtocolSeqNo)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));

        cmiDump(aCtx, sHeader, aCtx->mReadBlock, 0, aCtx->mReadBlock->mDataSize);
    }
    IDE_EXCEPTION(MarshalErr)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_MARSHAL_ERROR));

        cmiDump(aCtx, sHeader, aCtx->mReadBlock, 0, aCtx->mReadBlock->mDataSize);
    }
    IDE_EXCEPTION(InvalidOpError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_OPERATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************
 * PROJ-2616
 * cmiRecvIPCDA
 *
 * cmiRecv for IPCDA.
 *
 * aCtx[in]           - cmiProtocolContext
 * aUserContext[in]   - UserContext
 * aMicroSleepTime[in] - Sleep time in spinlock
 ****************************************************/
IDE_RC cmiRecvIPCDA(cmiProtocolContext *aCtx,
                    void               *aUserContext,
                    UInt                aMicroSleepTime)
{
    cmpCallbackFunction  sCallbackFunction;
    UChar                sOpID;
    IDE_RC               sRet = IDE_SUCCESS;

    cmnLinkPeerIPCDA    *sLinkIPCDA     = NULL;
    cmbBlockIPCDA       *sOrgBlock = NULL;
    cmbBlock             sTmpBlock;

    UInt                 sCurReadOperationCount     = 0;

    sLinkIPCDA = (cmnLinkPeerIPCDA *)aCtx->mLink;

    sOrgBlock              = (cmbBlockIPCDA *)aCtx->mReadBlock;

    IDE_TEST_RAISE(aCtx->mIsDisconnect == ID_TRUE, Disconnected);

    sTmpBlock.mBlockSize   = sOrgBlock->mBlock.mBlockSize;
    sTmpBlock.mCursor      = CMP_HEADER_SIZE;
    sTmpBlock.mDataSize    = sOrgBlock->mBlock.mDataSize;
    sTmpBlock.mIsEncrypted = sOrgBlock->mBlock.mIsEncrypted;
    sTmpBlock.mData        = &sOrgBlock->mData;

    IDE_TEST_RAISE(cmiIPCDACheckReadFlag(aCtx, aMicroSleepTime) == IDE_FAILURE, Disconnected);

    aCtx->mReadBlock = &sTmpBlock;

    while(1)
    {
        IDE_TEST_RAISE(cmiIPCDACheckDataCount(aCtx,
                                              &sOrgBlock->mOperationCount,
                                              sCurReadOperationCount,
                                              aMicroSleepTime) == IDE_FAILURE, Disconnected);

        sTmpBlock.mDataSize = sOrgBlock->mBlock.mDataSize;

        CMI_RD1(aCtx, sOpID);

        IDE_TEST_RAISE(sOpID >= aCtx->mModule->mOpMax, InvalidOpError);

        /* Check end-of protocol */
        /* BUG-46502 */
        if ( sOpID == CMP_OP_DB_IPCDALastOpEnded )
        {
            break;
        }

        if (sCurReadOperationCount++ == 0)
        {
            IDE_TEST_RAISE(cmnLinkPeerInitSvrWriteIPCDA((void*)aCtx) == IDE_FAILURE, Disconnected);
        }

        /* Callback Function 획득 */
        aCtx->mSessionCloseNeeded  = ID_FALSE;
        /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
        aCtx->mProtocol.mOpID      = sOpID;

#if defined(ALTI_CFG_OS_LINUX)
        sLinkIPCDA->mMessageQ.mNeedToNotify = ID_TRUE;
#endif

        /* Callback 호출 */
        sCallbackFunction = aCtx->mModule->mCallbackFunction[sOpID];
        sRet = sCallbackFunction(aCtx,
                                 &aCtx->mProtocol,
                                 aCtx->mOwner,
                                 aUserContext);
        /* PROJ-1697 Performance view for Protocols */
        CMP_DB_PROTOCOL_STAT_ADD( aCtx->mProtocol.mOpID, 1 );

        /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
        /* BUG-46502 */
        if ( sRet == IDE_CM_STOP )
        {
            break;
        }
        IDE_TEST(sRet == IDE_FAILURE);
    }

    aCtx->mReadBlock = (cmbBlock *)sOrgBlock;

    return sRet; // IDE_SUCCESS, IDE_FAILURE, IDE_CM_STOP

    IDE_EXCEPTION(Disconnected);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(InvalidOpError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_OPERATION));
    }
    IDE_EXCEPTION_END;

    /* BUG-44468 [cm] codesonar warning in CM */
    aCtx->mReadBlock = (cmbBlock *)sOrgBlock;

    return IDE_FAILURE;
}

#if defined(ALTI_CFG_OS_LINUX)
/* PROJ-2616 Send message through MessageQ.*/
IDE_RC cmiMessageQNotify(cmnLinkPeerIPCDA *aLink)
{
    char sSendBuf[1];
    struct timespec   sMessageQWaitTime;

    IDE_TEST_CONT((cmuProperty::isIPCDAMessageQ() != ID_TRUE), ContMQNOtify);

    sSendBuf[0] = CMI_IPCDA_MESSAGEQ_NOTIFY;

    clock_gettime(CLOCK_REALTIME, &sMessageQWaitTime);
    sMessageQWaitTime.tv_sec += cmuProperty::getIPCDAMessageQTimeout();
        
    IDE_TEST( mq_timedsend( aLink->mMessageQ.mId,
                            sSendBuf,
                            aLink->mMessageQ.mAttr.mq_msgsize,
                            0,
                            &sMessageQWaitTime) != 0);

    IDE_EXCEPTION_CONT(ContMQNOtify);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

/*************************************************************
 * proj_2160 cm_type removal
 * 1. 이 함수는 콜백 안에서 분할 패킷을 연속적으로 수신하는 경우에
 *  사용하기 위해 만들어졌다.
 * 2. cmiRecv()와의 차이점은 콜백을 호출하는 반복문이 없다
*************************************************************/
IDE_RC cmiRecvNext(cmiProtocolContext* aCtx, PDL_Time_Value* aTimeout)
{
    cmpHeader*           sHeader = &(aCtx->mReadHeader);
    UInt                 sCmSeqNo;

    aCtx->mReadBlock->mDataSize  = 0;
    aCtx->mReadBlock->mCursor    = 0;

    IDE_TEST_RAISE(aCtx->mIsDisconnect == ID_TRUE, Disconnected);
    IDE_TEST(aCtx->mLink->mPeerOp->mRecv(aCtx->mLink,
                                         &aCtx->mReadBlock,
                                         sHeader, 
                                         aTimeout) != IDE_SUCCESS);
    
    /* BUG-45184 */
    aCtx->mReceiveDataSize += aCtx->mReadBlock->mDataSize;
    aCtx->mReceiveDataCount++;
    
    sCmSeqNo = CMP_HEADER_SEQ_NO(sHeader);
#ifdef CMI_DUMP
    ideLog::logLine(IDE_CM_4, "[%5d] recv [%5d]",
                    sCmSeqNo, sHeader->mA7.mPayloadLength);
#endif

    /* BUG-41909 Add dump CM block when a packet error occurs */
    IDU_FIT_POINT_RAISE( "cmiRecvNext::Server::InvalidProtocolSeqNo", InvalidProtocolSeqNo );

    // 모든 패킷은 세션내애서 고유일련번호를 갖는다.
    // 범위: 0 ~ 0x7fffffff, 최대값에 다다르면 0부터 다시 시작된다
    IDE_TEST_RAISE(sCmSeqNo != aCtx->mCmSeqNo, InvalidProtocolSeqNo);
    if (aCtx->mCmSeqNo == CMP_HEADER_MAX_SEQ_NO)
    {
        aCtx->mCmSeqNo = 0;
    }
    else
    {
        aCtx->mCmSeqNo++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Disconnected);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(InvalidProtocolSeqNo);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PROTOCOL_SEQUENCE));

        cmiDump(aCtx, sHeader, aCtx->mReadBlock, 0, aCtx->mReadBlock->mDataSize);
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*************************************************************
 * proj_2160 cm_type removal
 * 1. 이 함수는 A5의 cmiWriteBlock을 대체한다
 * 2. 이 함수에서는 패킷 헤더를 만들어 패킷을 송신한다
 * 3. A5과 마찬가지로 pendingList를 유지한다. 이유는
 *  Altibase에서는 비동기 통신이 가능(ex) client가 송신하고 있는
 *  도중인데, 서버에서는 응답을 바로 생성하며 한 패킷을 넘기는 경우
 *  바로 송신이 되어지도록 되어 있음) 한데, 이 때 소켓버퍼가
 *  꽉 차서 실패한 경우 버퍼링을 하지 않고 무한 대기하게 되면
 *  서로를 애타게 기다리는 상황이 벌어질수도 있다
*************************************************************/
IDE_RC cmiSend( cmiProtocolContext  * aCtx, 
                idBool                aIsEnd, 
                PDL_Time_Value      * aTimeout )
{
    cmnLinkPeer *sLink   = aCtx->mLink;
    cmbBlock    *sBlock  = aCtx->mWriteBlock;
    cmpHeader   *sHeader = &aCtx->mWriteHeader;
    cmbPool     *sPool   = aCtx->mLink->mPool;

    cmbBlock    *sPendingBlock;
    iduListNode *sIterator;
    iduListNode *sNodeNext;
    idBool       sSendSuccess;
    idBool       sNeedToSave = ID_FALSE;
    cmbBlock    *sNewBlock;

    UInt         sCmSeqNo = 0;
    UInt         sSendDataSize = 0;

    IDE_TEST_CONT((sLink->mLink.mImpl == CMN_LINK_IMPL_IPCDA), noDataToSend);
    IDE_TEST_CONT(sBlock->mCursor == CMP_HEADER_SIZE, noDataToSend);

    if (aIsEnd == ID_TRUE)
    {
        CMP_HEADER_SET_PROTO_END(sHeader);
    }
    else
    {
        CMP_HEADER_CLR_PROTO_END(sHeader);
    }

    CMP_HEADER_FLAG_CLR_COMPRESS( sHeader );

    /* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
    IDE_TEST( cmiCompressCmBlock( sHeader, 
                                  aCtx,
                                  sBlock ) 
              != IDE_SUCCESS );

    /* BUG-38102 */
    if ( aCtx->mEncryptFlag == ID_TRUE )
    {
        IDE_TEST( cmiEncryptCmBlock( sBlock ) != IDE_SUCCESS );

        CMP_HEADER_FLAG_SET_ENCRYPT( sHeader );        
    }
    else
    {
        CMP_HEADER_FLAG_CLR_ENCRYPT( sHeader );
    }

    sBlock->mDataSize = sBlock->mCursor;
    IDE_TEST(cmpHeaderWrite(sHeader, sBlock) != IDE_SUCCESS);
    sNeedToSave = ID_TRUE;
    
    IDU_FIT_POINT_RAISE( "cmiSend::Server::ideIsRetry", SendFail );

    // Pending Write Block들을 전송 (send previous packets)
    IDU_LIST_ITERATE_SAFE(&aCtx->mWriteBlockList, sIterator, sNodeNext)
    {
        sPendingBlock = (cmbBlock *)sIterator->mObj;

        sSendSuccess = ID_TRUE;

        sSendDataSize = sPendingBlock->mDataSize;
        // BUG-19465 : CM_Buffer의 pending list를 제한
        while (sLink->mPeerOp->mSend(sLink, sPendingBlock) != IDE_SUCCESS)
        {
            sSendSuccess = ID_FALSE;

            IDE_TEST_RAISE(ideIsRetry() != IDE_SUCCESS, SendFail);

            /* BUG-44468 [cm] codesonar warning in CM */
            if( aCtx->mListLength < gMaxPendingList )
            {
                break;
            }

            // bug-27250 IPC linklist can be crushed.
            
            IDE_TEST( cmiDispatcherWaitLink( (cmiLink*)sLink,
                                             CMN_DIRECTION_WR,
                                             aTimeout )
                      != IDE_SUCCESS );

            sSendSuccess = ID_TRUE;
        }

        if (sSendSuccess == ID_FALSE)
        {
            break;
        }

        /* BUG-45184 */
        aCtx->mSendDataSize += sSendDataSize;
        aCtx->mSendDataCount++;

        IDE_TEST(sPool->mOp->mFreeBlock(sPool, sPendingBlock) != IDE_SUCCESS);
        aCtx->mListLength--;
    }

    // send current block if there is no pendng block
    if (sIterator == &aCtx->mWriteBlockList)
    {
        sSendDataSize = sBlock->mDataSize;
        if (sLink->mPeerOp->mSend(sLink, sBlock) != IDE_SUCCESS)
        {
            IDE_TEST_RAISE(ideIsRetry() != IDE_SUCCESS, SendFail);
            sNeedToSave = ID_TRUE;
        }
        else
        {
            sNeedToSave = ID_FALSE;
            
            /* BUG-45184 */
            aCtx->mSendDataSize += sSendDataSize;
            aCtx->mSendDataCount++;
        }
    }
    else
    {
        sNeedToSave = ID_TRUE;
    }

    // 현재 block을 pendingList 맨뒤에 저장해 둔다
    if (sNeedToSave == ID_TRUE)
    {
        sNewBlock = NULL;
        IDE_TEST(sPool->mOp->mAllocBlock(sPool, &sNewBlock) != IDE_SUCCESS);
        sNewBlock->mDataSize = sNewBlock->mCursor = CMP_HEADER_SIZE;
        IDU_LIST_ADD_LAST(&aCtx->mWriteBlockList, &sBlock->mListNode);
        aCtx->mWriteBlock = sNewBlock;
        sBlock            = sNewBlock;
        aCtx->mListLength++;
        sNeedToSave = ID_FALSE;
    }

    // 모든 패킷은 세션내애서 고유일련번호를 갖는다.
    // 범위: 0 ~ 0x7fffffff, 최대값에 다다르면 0부터 다시 시작된다
    sCmSeqNo = CMP_HEADER_SEQ_NO(sHeader);
    if (sCmSeqNo == CMP_HEADER_MAX_SEQ_NO)
    {
        sHeader->mA7.mCmSeqNo = 0;
    }
    else
    {
        sHeader->mA7.mCmSeqNo = sCmSeqNo + 1;
    }

    if (aIsEnd == ID_TRUE)
    {
        // 프로토콜 끝이라면 모든 Block이 전송되어야 함
        IDU_LIST_ITERATE_SAFE(&aCtx->mWriteBlockList, sIterator, sNodeNext)
        {
            sPendingBlock = (cmbBlock *)sIterator->mObj;

            sSendDataSize = sPendingBlock->mDataSize;
            while (sLink->mPeerOp->mSend(sLink, sPendingBlock) != IDE_SUCCESS)
            {
                IDE_TEST_RAISE(ideIsRetry() != IDE_SUCCESS, SendFail);

                IDE_TEST( cmiDispatcherWaitLink( (cmiLink*)sLink,
                                                 CMN_DIRECTION_WR,
                                                 aTimeout )
                          != IDE_SUCCESS );
            }
            
            /* BUG-45184 */
            aCtx->mSendDataSize += sSendDataSize;
            aCtx->mSendDataCount++;

            IDE_TEST(sPool->mOp->mFreeBlock(sPool, sPendingBlock) != IDE_SUCCESS);
            aCtx->mListLength--;
        }

        // for IPC
        sLink->mPeerOp->mReqComplete(sLink);
    }

    IDE_EXCEPTION_CONT(noDataToSend);
    sBlock->mDataSize = sBlock->mCursor = CMP_HEADER_SIZE;
    return IDE_SUCCESS;

    IDE_EXCEPTION(SendFail);
    {
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if( ( sNeedToSave == ID_TRUE ) &&
        ( aCtx->mListLength >= gMaxPendingList ) ) /* BUG-44468 [cm] codesonar warning in CM */
    {
        aCtx->mResendBlock = ID_FALSE;
    }
    else
    {
        /* do nothing */
    }

    /*
     *  BUG-38716 
     *  [rp-sender] It needs a property to give sending timeout to replication sender. 
     */
    if ( ( sNeedToSave == ID_TRUE ) && 
         ( aCtx->mResendBlock == ID_TRUE ) ) 
    {
        sNewBlock = NULL;
        (void)sPool->mOp->mAllocBlock(sPool, &sNewBlock);
        sNewBlock->mDataSize = sNewBlock->mCursor = CMP_HEADER_SIZE;
        IDU_LIST_ADD_LAST(&aCtx->mWriteBlockList, &sBlock->mListNode);
        aCtx->mWriteBlock = sNewBlock;
        sBlock            = sNewBlock;
        aCtx->mListLength++;
    }
    else
    {
        /* do nohting */
    }

    IDE_POP();

    sBlock->mDataSize = sBlock->mCursor = CMP_HEADER_SIZE;
    return IDE_FAILURE;
}

/*************************************************************
 * BUG-46163 IPCDA Send
 * IPCDA에서 Server -> Client로 응답 Protocol 전송이
 * 완료 되었을때 호출한다.
*************************************************************/
IDE_RC cmiLinkPeerFinalizeSvrForIPCDA( cmiProtocolContext  * aCtx )
{
    cmnLinkPeerIPCDA  *sLinkIPCDA = NULL;

    sLinkIPCDA = (cmnLinkPeerIPCDA *)aCtx->mLink;

    IDE_TEST_RAISE(aCtx->mIsDisconnect == ID_TRUE, Disconnected);

    /* BUG-46163 이미 데이터는 shard memory에 copy되어 있으므로 완료 신호만 보낸다. */
#if defined(ALTI_CFG_OS_LINUX)
    /* message queue */
    if( sLinkIPCDA->mMessageQ.mNeedToNotify == ID_TRUE )
    {
        sLinkIPCDA->mMessageQ.mNeedToNotify = ID_FALSE;
        IDE_TEST(cmiMessageQNotify(sLinkIPCDA) != IDE_SUCCESS);
    }
#endif

    /* BUG-46502 */
    if (aCtx->mWriteBlock != NULL)
    {
        /* Write marking for end-of-protocol. */
        CMI_WOP(aCtx, CMP_OP_DB_IPCDALastOpEnded);
        cmiIPCDAIncDataCount(aCtx);
        acpAtomicSet32( &(((cmbBlockIPCDA *)aCtx->mWriteBlock)->mWFlag), CMB_IPCDA_SHM_DEACTIVATED );
    }

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    if (aCtx->mReadBlock != NULL)
    {
        /* BUG-46502 operationCount 초기화 */
        acpAtomicSet32( &(((cmbBlockIPCDA *)aCtx->mReadBlock)->mOperationCount), 0 );

        /* BUG-46502 atomic 함수 적용 */
        acpAtomicSet32( &(((cmbBlockIPCDA *)aCtx->mReadBlock)->mRFlag), CMB_IPCDA_SHM_DEACTIVATED );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Disconnected);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION_END;

    /* BUG-46502 */
    if (aCtx->mWriteBlock != NULL)
    {
        /* Write marking for end-of-protocol. */
        CMI_WOP(aCtx, CMP_OP_DB_IPCDALastOpEnded);
        cmiIPCDAIncDataCount(aCtx);
        acpAtomicSet32( &(((cmbBlockIPCDA *)aCtx->mWriteBlock)->mWFlag), CMB_IPCDA_SHM_DEACTIVATED);
    }

    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    if (aCtx->mReadBlock != NULL)
    {
        /* BUG-46502 operationCount 초기화 */
        acpAtomicSet32( &(((cmbBlockIPCDA *)aCtx->mReadBlock)->mOperationCount), 0 );

        /* BUG-46502 atomic 함수 적용 */
        acpAtomicSet32( &(((cmbBlockIPCDA *)aCtx->mReadBlock)->mRFlag), CMB_IPCDA_SHM_DEACTIVATED );
    }
    
    return IDE_FAILURE;
}

/*
 *  BUG-38716 
 *  [rp-sender] It needs a property to give sending timeout to replication sender. 
 *
 *  Pending Block 에 저장되어 있는 Block 들을 전송 합니다.
 *  현재 RP 에서만 사용
 */
IDE_RC cmiFlushPendingBlock( cmiProtocolContext * aCtx,
                             PDL_Time_Value     * aTimeout )
{
    cmnLinkPeer     * sLink = aCtx->mLink;
    cmbPool         * sPool = aCtx->mLink->mPool;

    cmbBlock        * sPendingBlock = NULL;
    iduListNode     * sIterator = NULL;
    iduListNode     * sNodeNext = NULL;
    UInt              sSendDataSize = 0;

    IDU_LIST_ITERATE_SAFE( &aCtx->mWriteBlockList, sIterator, sNodeNext )
    {
        sPendingBlock = (cmbBlock *)sIterator->mObj;

        sSendDataSize = sPendingBlock->mDataSize;
        while ( sLink->mPeerOp->mSend( sLink, sPendingBlock ) != IDE_SUCCESS )
        {
            IDE_TEST( ideIsRetry() != IDE_SUCCESS );

            IDE_TEST( cmnDispatcherWaitLink( (cmiLink *)sLink,
                                             CMN_DIRECTION_WR,
                                             aTimeout ) 
                      != IDE_SUCCESS );
        }
        
        /* BUG-45184 */
        aCtx->mSendDataSize += sSendDataSize;
        aCtx->mSendDataCount++;
        
        IDE_TEST( sPool->mOp->mFreeBlock( sPool, sPendingBlock ) != IDE_SUCCESS );
        aCtx->mListLength--;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC cmiCheckAndFlush( cmiProtocolContext * aProtocolContext,
                         UInt aLen,
                         idBool aIsEnd,
                         PDL_Time_Value * aTimeout )
{
    if ( aProtocolContext->mWriteBlock->mCursor + aLen >
         aProtocolContext->mWriteBlock->mBlockSize )
    {
        IDE_TEST( cmiSend( aProtocolContext, 
                           aIsEnd, 
                           aTimeout ) 
                  != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSplitRead( cmiProtocolContext *aCtx,
                     ULong               aReadSize,
                     UChar              *aBuffer,
                     PDL_Time_Value     *aTimeout )
{
    UChar *sBuffer      = aBuffer;
    ULong  sReadSize    = aReadSize;
    UInt   sRemainSize  = aCtx->mReadBlock->mDataSize -
                          aCtx->mReadBlock->mCursor;
    UInt   sCopySize;

    while( sReadSize > sRemainSize )
    {
        sCopySize = IDL_MIN( sReadSize, sRemainSize );

        CMI_RCP ( aCtx, sBuffer, sCopySize );
        IDE_TEST( cmiRecvNext( aCtx, aTimeout ) != IDE_SUCCESS );

        sRemainSize = aCtx->mReadBlock->mDataSize - CMP_HEADER_SIZE;
        sBuffer    += sCopySize;
        sReadSize  -= sCopySize;
    }
    CMI_RCP( aCtx, sBuffer, sReadSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSplitSkipRead( cmiProtocolContext *aCtx,
                         ULong               aReadSize,
                         PDL_Time_Value     *aTimeout )
{
    ULong  sReadSize    = aReadSize;
    UInt   sRemainSize  = 0;
    UInt   sCopySize    = 0;

    if (cmiGetLinkImpl(aCtx) == CMI_LINK_IMPL_IPCDA)
    {
        IDE_TEST( aCtx->mReadBlock->mCursor + aReadSize >= CMB_BLOCK_DEFAULT_SIZE);
    }
    else
    {
        sRemainSize = aCtx->mReadBlock->mDataSize - aCtx->mReadBlock->mCursor;
        while( sReadSize > sRemainSize )
        {
            IDE_TEST( cmiRecvNext( aCtx, aTimeout ) != IDE_SUCCESS );

            sCopySize = IDL_MIN( sReadSize, sRemainSize );

            sRemainSize = aCtx->mReadBlock->mDataSize - CMP_HEADER_SIZE;
            sReadSize  -= sCopySize;
        }
    }
    aCtx->mReadBlock->mCursor += sReadSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiSplitWrite( cmiProtocolContext *aCtx,
                      ULong               aWriteSize,
                      UChar              *aBuffer )
{
    UChar *sBuffer      = aBuffer;
    ULong  sWriteSize   = aWriteSize;
    UInt   sRemainSize  = 0;
    UInt   sCopySize    = 0;

    if(cmiGetLinkImpl(aCtx) == CMI_LINK_IMPL_IPCDA)
    {
        IDE_TEST( aCtx->mWriteBlock->mCursor + aWriteSize >= CMB_BLOCK_DEFAULT_SIZE);
    }
    else
    {
        sRemainSize  = aCtx->mWriteBlock->mBlockSize - aCtx->mWriteBlock->mCursor;
        while( sWriteSize > sRemainSize )
        {
            sCopySize = IDL_MIN( sWriteSize, sRemainSize );

            CMI_WCP ( aCtx, sBuffer, sCopySize );
            IDE_TEST( cmiSend ( aCtx, ID_FALSE ) != IDE_SUCCESS );

            sRemainSize  = CMB_BLOCK_DEFAULT_SIZE - CMP_HEADER_SIZE;
            sBuffer     += sCopySize;
            sWriteSize  -= sCopySize;
        }
    }
    CMI_WCP( aCtx, sBuffer, sWriteSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-5894 Permit sysdba via IPC */
IDE_RC cmiPermitConnection(cmiLink *aLink,
                           idBool   aHasSysdba,
                           idBool   aIsSysdba)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Peer Type 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER);

    /* IPC에서만 mPermitConnection을 체크하면 된다 */
    if (sLink->mPeerOp->mPermitConnection != NULL)
    {
        IDE_TEST(sLink->mPeerOp->mPermitConnection(sLink,
                                                   aHasSysdba,
                                                   aIsSysdba) != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

cmnLinkImpl cmiGetLinkImpl(cmiLink *aLink)
{
    return aLink->mImpl;
}

/* bug-33841: ipc thread's state is wrongly displayed
   mmtThreadManager에서 mmtServiceThread::setExecuteCallback을 등록 */
IDE_RC cmiSetCallbackSetExecute(cmiCallbackSetExecute aCallback)
{
    gCMCallbackSetExecute = aCallback;
    return IDE_SUCCESS;
}

/* BUG-48871 압축 방식 LZ4로 변경 후 이중화 하위 호환성 유지 */
/*
 *
 */ 
void cmiEnableCompress( cmiProtocolContext * aCtx,
                        UInt                 aLevel,
                        cmiCompressType      aCompressType )
{
    aCtx->mCompressLevel    = aLevel;
    aCtx->mCompressType     = aCompressType;
}

/*
 *
 */ 
void cmiDisableCompress( cmiProtocolContext * aCtx )
{
    aCtx->mCompressLevel    = 0;
    aCtx->mCompressType     = CMI_COMPRESS_NONE;
}

/*
 *
 */ 
void cmiSetDecompressType( cmiProtocolContext * aCtx,
                           cmiCompressType      aDecompressType )
{
    aCtx->mDecompressType = aDecompressType;
}

/* 
 * BUG-38102: Add a replication functionality for XLog encryption 
 */ 
void cmiEnableEncrypt( cmiProtocolContext * aCtx )
{
    aCtx->mEncryptFlag = ID_TRUE;
}

/*
 * BUG-38102: Add a replication functionality for XLog encryption
 */ 
void cmiDisableEncrypt( cmiProtocolContext * aCtx )
{
    aCtx->mEncryptFlag = ID_FALSE;    
}

idBool cmiIsResendBlock( cmiProtocolContext * aProtocolContext )
{
    return aProtocolContext->mResendBlock;
}

void cmiLinkSetPacketTypeA5( cmiLink *aLink )
{
    IDE_DASSERT( aLink != NULL );

    ((cmnLink*)aLink)->mPacketType = CMP_PACKET_TYPE_A5;
}

/**
 *  cmiDump
 *
 *  Ctx, Packet등 에러 발생시 유용한 정보를 덤프한다.
 *
 *  @aCtx       : cmiProtocolContext
 *  @aHeader    : CM Packet Header
 *  @aBlock     : CM Packet Block
 *  @aFromIndex : 출력할 시작 Index
 *  @aLen       : 출력할 Length
 */
static void cmiDump(cmiProtocolContext   *aCtx,
                    cmpHeader            *aHeader,
                    cmbBlock             *aBlock,
                    UInt                  aFromIndex,
                    UInt                  aLen)
{
    /* BUG-41909 Add dump CM block when a packet error occurs */
    SChar *sHexBuf      = NULL;
    UInt   sHexBufIndex = 0;
    /* 
     * One Line Size = 16 * 3 + 11 = 59
     * Line Count    = 2^15(CMB_BLOCK_DEFAULT_SIZE) / 2^4 = 2^11
     * Needed Buffer = 59 * 2^11 = 120832byte
     *
     * 32KB 패킷 전체를 출력하기 위해서는 120KB가 필요하다.
     * CTX, HEADER등도 출력하기 때문에 128KB 정도면 현재 충분하다.
     * 1byte를 HEX 값으로 출력하기 위해서 대략 4byte가 필요한 것이다.
     */
    UInt   sHexBufSize  = CMB_BLOCK_DEFAULT_SIZE * 4;
    UInt   sToIndex     = aFromIndex + aLen - 1;
    UInt   sAddr        = 0;
    UInt   i;

    IDE_TEST_CONT( (aCtx == NULL) || (aHeader == NULL) || (aBlock == NULL), NO_NEED_WORK);

    /*
     * cmiProtocolContext에 HexBuffer를 달면 Session마다 128KB 버퍼를 가져야 한다.
     * Packet error는 극히 드문 상황이므로 실제 출력을 할 때 할당해서 쓴다.
     */
    IDU_FIT_POINT_RAISE( "cmiDump::Server::MemAllocError", MemAllocError );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMI,
                                     sHexBufSize,
                                     (void **)&sHexBuf,
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, MemAllocError);

    /* Ctx */
    sHexBufIndex += idlOS::snprintf(&sHexBuf[sHexBufIndex], sHexBufSize - sHexBufIndex,
                                    CM_TRC_DUMP_CTX,
                                    aCtx->mCmSeqNo);

    /* Header */
    sHexBufIndex += idlOS::snprintf(&sHexBuf[sHexBufIndex], sHexBufSize - sHexBufIndex,
                                    CM_TRC_DUMP_PACKET_HEADER,
                                    aHeader->mA7.mHeaderSign,
                                    aHeader->mA7.mPayloadLength,
                                    CMP_HEADER_SEQ_NO(aHeader),
                                    aBlock->mDataSize);

    /* MSG 파일에서 %02X를 인식못하는 버그가 있다. %x만 인식한다. */
    sHexBufIndex += idlOS::snprintf(&sHexBuf[sHexBufIndex], sHexBufSize - sHexBufIndex,
                                    "# BLOCK HEADER  = "
                                    "%02X %02X %02X%02X %02X%02X%02X%02X "
                                    "%02X%02X %02X%02X %02X%02X%02X%02X\n",
                                    aBlock->mData[0],
                                    aBlock->mData[1],
                                    aBlock->mData[2],
                                    aBlock->mData[3],
                                    aBlock->mData[4],
                                    aBlock->mData[5],
                                    aBlock->mData[6],
                                    aBlock->mData[7],
                                    aBlock->mData[8],
                                    aBlock->mData[9],
                                    aBlock->mData[10],
                                    aBlock->mData[11],
                                    aBlock->mData[12],
                                    aBlock->mData[13],
                                    aBlock->mData[14],
                                    aBlock->mData[15]);

    IDE_TEST_CONT( (aFromIndex >= CMB_BLOCK_DEFAULT_SIZE) ||
                   (aLen       >  CMB_BLOCK_DEFAULT_SIZE) ||
                   (sToIndex   >= CMB_BLOCK_DEFAULT_SIZE),
                   NO_NEED_WORK );

    /* Payload */
    sHexBufIndex += idlOS::snprintf(&sHexBuf[sHexBufIndex], sHexBufSize - sHexBufIndex,
                                    CM_TRC_DUMP_PACKET_PAYLOAD,
                                    aFromIndex,
                                    sToIndex);

    for (i = aFromIndex, sAddr = 0; i <= sToIndex; i++)
    {
        /* 한 라인에 HEX값 16개를 출력한다. */
        if (sAddr % 16 == 0)
        {
            sHexBufIndex += idlOS::snprintf(&sHexBuf[sHexBufIndex], sHexBufSize - sHexBufIndex, "\n%08X: ", sAddr);
        }
        else
        {
            /* Nothing */
        }
        sAddr += 1;

        sHexBuf[sHexBufIndex++] = "0123456789ABCDEF"[aBlock->mData[i] >> 4 & 0xF];
        sHexBuf[sHexBufIndex++] = "0123456789ABCDEF"[aBlock->mData[i]      & 0xF];
        sHexBuf[sHexBufIndex++] = ' ';
    }

    IDE_EXCEPTION_CONT(NO_NEED_WORK);

    if (sHexBufIndex > 0)
    {
        sHexBuf[sHexBufIndex] = '\0';
        ideLog::log(IDE_CM_1, "%s", sHexBuf);
    }
    else
    {
        /* Nothing */
    }

    /* 메모리 할당 실패시 TRC Log만 출력하고 넘어가자 */
    IDE_EXCEPTION(MemAllocError)
    {
        ideLog::log(IDE_CM_0, CM_TRC_MEM_ALLOC_ERROR, errno, sHexBufSize);
    }
    IDE_EXCEPTION_END;

    if (sHexBuf != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(sHexBuf) == IDE_SUCCESS)
    }

    return;
}

/* PROJ-2474 SSL/TLS, BUG-44488 */
IDE_RC cmiSslInitialize( void )
{
#if !defined(CM_DISABLE_SSL)
    return cmnOpenssl::initialize();
#endif
    return IDE_SUCCESS;
}

IDE_RC cmiSslFinalize( void )
{
#if !defined(CM_DISABLE_SSL)
    return cmnOpenssl::destroy();
#endif
    return IDE_SUCCESS;
}

/* PROJ-2681 */
IDE_RC cmiIBInitialize(void)
{
    IDE_TEST_RAISE(cmnIBInitialize() != ACI_SUCCESS, InitFailed);

    ideLog::log(IDE_SERVER_0, CM_TRC_IB_INITIALIZED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InitFailed)
    {
        IDE_SET(ideSetErrorCodeAndMsg(aciGetErrorCode(), aciGetErrorMsg(aciGetErrorCode())));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmiIBFinalize(void)
{
    ACI_TEST(cmnIBFinalize() != ACI_SUCCESS);

    return IDE_SUCCESS;

    ACI_EXCEPTION_END;

    return IDE_FAILURE;
}

