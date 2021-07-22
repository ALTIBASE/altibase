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
 
/*******************************************************************************
 * $Id: testAcpPollEx.c 9236 2009-12-17 02:57:25Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpMem.h>
#include <acpPoll.h>
#include <acpSock.h>
#include <acpThr.h>
#include <acpSpinLock.h>
#include <acpByteOrder.h>

/* Number of polled sockets */
#define SOCK_NUM 10
#define TEST_POLL_DISPATCHED 0
#define TEST_POLL_CANCELED 1
#define TEST_POLL_NUM_SEND 5 /* five send events are tested */ 


acp_sock_t   gSock[SOCK_NUM];
static acp_uint32_t gCallbackCounter = 0;
static acp_spin_lock_t gThreadLock = ACP_SPIN_LOCK_INITIALIZER(-1);


typedef struct testPollSuit
{
    acp_sint32_t mSockIndex;
    acp_sint32_t mShouldSend; /* sent data? */
    acp_sint32_t mRetVal;     /* cancel or dispatch */
    acp_sint32_t mIsProcessed;
    acp_char_t *mMsg;
} testPollSuit;

testPollSuit gTestPollSuit[] =
{
    {0, ACP_FALSE, TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg0"},
    {1, ACP_TRUE,  TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg1"},
    {2, ACP_FALSE, TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg2"},
    {3, ACP_TRUE,  TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg3"},
    {4, ACP_FALSE, TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg4"},
    /* test that the number of events and index of canceled event  */
    /* has no relation. */
    {5, ACP_TRUE,  TEST_POLL_CANCELED,   ACP_FALSE, "testmsg5"},
    {6, ACP_FALSE, TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg6"},
    {7, ACP_TRUE,  TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg7"},
    {8, ACP_FALSE, TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg8"},
    {9, ACP_TRUE,  TEST_POLL_DISPATCHED, ACP_FALSE, "testmsg9"},
};


/*
 * Dispatch callback function
 */
static acp_sint32_t callbackServerThread(acp_poll_set_t       *aPollSet,
                                         const acp_poll_obj_t *aPollObj,
                                         void                 *aData)
{
    acp_bool_t sInRange;
    acp_sint32_t sSockIndex;
    acp_char_t sBuf[10];
    acp_size_t sRecvSize;
    acp_rc_t sRC;

    ACP_UNUSED(aPollSet);
    ACP_UNUSED(aData);

    /*
     * BUG-30255
     * Check that we are processing right socket
     * Note that aPollObj->mSock does not always come in order
     */
    sInRange = ((aPollObj->mSock >= &(gSock[0])) &&
                (aPollObj->mSock <= &(gSock[SOCK_NUM - 1]))
               )? ACP_TRUE : ACP_FALSE;
    ACT_CHECK_DESC(sInRange == ACP_TRUE,
                   ("aPollObj->mSock(%p) is not in gSock[%d](%p ~ %p)",
                    aPollObj->mSock,
                    SOCK_NUM,
                    &(gSock[0]),
                    &(gSock[SOCK_NUM - 1])
                   )
                  );

    sSockIndex = aPollObj->mSock - gSock;

    /*
     * validate data
     */

    ACT_CHECK(aPollObj->mRtnEvent | ACP_POLL_IN);
    ACT_CHECK_DESC(sSockIndex == gTestPollSuit[sSockIndex].mSockIndex,
                   ("Wrong socket index (%d), should be (%d)", sSockIndex,
                    gTestPollSuit[sSockIndex].mSockIndex));

    ACT_CHECK_DESC(gTestPollSuit[sSockIndex].mIsProcessed == ACP_FALSE,
                   ("Processed event of socket#%d is processed again",
                    sSockIndex));

    sRC = acpSockRecv(aPollObj->mSock, sBuf, sizeof(sBuf),
                      &sRecvSize, ACP_MSG_DONTWAIT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK_DESC(acpCStrCmp(sBuf,
                              gTestPollSuit[sSockIndex].mMsg,
                              sizeof(gTestPollSuit[sSockIndex].mMsg)) == 0,
                  ("Socket receives broken data"));
    sBuf[sRecvSize] = '\0';
    
    gTestPollSuit[sSockIndex].mIsProcessed = ACP_TRUE;

    (void)acpAtomicInc32(&gCallbackCounter);

    return gTestPollSuit[sSockIndex].mRetVal;
}

/*
 * Server thread
 */
static acp_sint32_t testServerThread(void *aArg)
{
    acp_rc_t         sRC;
    acp_poll_set_t   sPollSet;
    acp_sock_t      *sSockListen = (acp_sock_t *)aArg;
    acp_uint16_t     i;


    /* Create poll set */
    sRC = acpPollCreate(&sPollSet, SOCK_NUM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Accept connections */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockAccept(&gSock[i], sSockListen, NULL, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpPollAddSock(&sPollSet, &gSock[i], ACP_POLL_IN, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    /*
     * Use spinlock before starting dispatching
     * to wait until all data is sent  to sockets 
     */
    acpSpinLockLock(&gThreadLock);

    /* Test resume dispatching before poll dispatch is called */
    sRC = acpPollDispatchResume(&sPollSet,
                                callbackServerThread,
                                NULL);
    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    /* Start dispatching events from sockets */
    sRC = acpPollDispatch(&sPollSet,
                          acpTimeFromMsec(1000),
                          callbackServerThread,
                          NULL);

    /* One event should be canceled */
    if (ACP_RC_IS_ECANCELED(sRC))
    {
        sRC = acpPollDispatchResume(&sPollSet,
                                    callbackServerThread,
                                    NULL);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("resume should return ACP_RC_SUCCESS, but %d\n", sRC));
    }
    else
    {
        ACT_ERROR(("dispatch should return ACP_RC_ECANCELED, but %d\n", sRC));
    }
    
    /* no data remained */
    sRC = acpPollDispatchResume(&sPollSet,
                                callbackServerThread,
                                NULL);
    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    /*
     * Even though callback function is not terminated,
     * caller can be terminated
     * Therefore Callback function and caller must be synchronized.
     */
    ACP_SPIN_WAIT(acpAtomicGet32(&gCallbackCounter) >= 5, -1);

    /* end of test */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpPollRemoveSock(&sPollSet, &gSock[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC), 
                    ("Failed to remove sock from poll sRC=%d\n", sRC));
        sRC = acpSockClose(&gSock[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = acpPollDestroy(&sPollSet);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    acpSpinLockUnlock(&gThreadLock);

    return 0;
}

/*
 * Test acpPollDispatchResume
 */
static void testPollResume(void)
{
    acp_rc_t     sRC;
    acp_thr_t    sServerThread;
    acp_sock_t   sSock[SOCK_NUM];
    acp_uint16_t i;
    acp_sock_t   sSockListen;

    acp_sock_len_t  sAddrLen;
    acp_size_t      sSendSize;

    struct sockaddr_in  sAddrClient;
    struct sockaddr_in  sAddr;


    /* Open tcp server socket */
    sRC = acpSockOpen(&sSockListen, ACP_AF_INET, ACP_SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Bind tcp server socket */
    acpMemSet(&sAddr, 0, sizeof(sAddr));
    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = ACP_TON_BYTE2_ARG(0);

    sRC = acpSockBind(&sSockListen,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_FALSE);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Listen tcp server socket */
    sRC = acpSockListen(&sSockListen, SOCK_NUM);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockListen, (acp_sock_addr_t *)&sAddr, &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* 
     * Lock spinlock in order to postpone server thread from dispatching 
     * until data is sent to all sockets
     */
    acpSpinLockLock(&gThreadLock);

    /* Start server thread */
    sRC = acpThrCreate(&sServerThread,
                       NULL,
                       testServerThread,
                       (void *)&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Create client sockets  */
    acpMemSet(&sAddrClient, 0, sizeof(sAddrClient));
    sAddrClient.sin_family      = ACP_AF_INET;
    sAddrClient.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddrClient.sin_port        = sAddr.sin_port;

    /* Connect sockets and send data */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockOpen(&sSock[i], ACP_AF_INET, ACP_SOCK_STREAM, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpSockConnect(&sSock[i],
                             (acp_sock_addr_t *)&sAddrClient,
                             (acp_sock_len_t)sizeof(sAddrClient));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }


    for (i = 0; i < SOCK_NUM; i++)
    {
        if (gTestPollSuit[i].mShouldSend == ACP_TRUE)
        {
            sRC = acpSockSend(&sSock[gTestPollSuit[i].mSockIndex],
                              gTestPollSuit[i].mMsg,
                              (acp_size_t)sizeof(gTestPollSuit[i].mMsg),
                              &sSendSize,
                              0);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            /* continue */
        }
    }

    /* Unlock spinlock to let server thread start dispatching */
    acpSpinLockUnlock(&gThreadLock);

    /* 
     * Send data to one socket to make sure
     * that it will not affect dispatch resume
     */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockSend(&sSock[1], /* the first data-sending socket */
                          gTestPollSuit[i].mMsg,
                          (acp_size_t)sizeof(gTestPollSuit[i].mMsg),
                          &sSendSize,
                          0);
        /* 
         * At this point server thread may finish dispatching and close 
         * sockects so ECONNRESET is expected return status
         */
        ACT_CHECK((ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ECONNRESET(sRC)));
        /*
         * BUG-30217
         * When ACP_RC_IS_ECONNRESET, following acpSockSend will return
         * ACP_RC_EPIPE or the process will receive SIGPIPE.
         * Thus, when not success, the send will be stopped.
         */
        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            break;
        }
        else
        {
            /* continue */
        }
    }

    /* Join server thread */
    sRC = acpThrJoin(&sServerThread, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    
    for (i = 0; i < SOCK_NUM; i++)
    {
        if (gTestPollSuit[i].mShouldSend == ACP_TRUE)
        {
            ACT_CHECK_DESC(gTestPollSuit[i].mIsProcessed == ACP_TRUE,
                           ("%d-th event is not processed", i));
        }
        else
        {
            ACT_CHECK_DESC(gTestPollSuit[i].mIsProcessed == ACP_FALSE,
                           ("%d-th event is not processed", i));
        }
    }

    /* Close client threads */
    sRC = acpSockClose(&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* Close client sockets */
    for (i = 0; i < SOCK_NUM; i++)
    {
        sRC = acpSockClose(&sSock[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
}

/*
 * Test main funcion
 */
acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    (void)acpSignalBlockAll();
    testPollResume();

    ACT_TEST_END();

    return 0;
}
