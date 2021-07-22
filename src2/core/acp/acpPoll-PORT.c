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
 * $Id: acpPoll-PORT.c 10502 2010-03-22 05:18:52Z denisb $
 ******************************************************************************/


ACP_EXPORT acp_rc_t acpPollCreate(acp_poll_set_t *aPollSet,
                                  acp_sint32_t    aMaxCount)
{
    acp_rc_t sRC;

    ACP_POLL_CHECK_BEFORE_CREATE(aPollSet, aMaxCount);

    /* Create port */
    aPollSet->mHandle = port_create();

    if (aPollSet->mHandle == -1)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRC = acpMemCalloc((void **)&aPollSet->mObjs,
                       aMaxCount,
                       sizeof(acp_poll_obj_t));

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)close(aPollSet->mHandle);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpMemCalloc((void **)&aPollSet->mEvents,
                       aMaxCount,
                       sizeof(port_event_t));

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)close(aPollSet->mHandle);
        acpMemFree(aPollSet->mObjs);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    aPollSet->mMaxCount    = aMaxCount;
    aPollSet->mCurCount    = 0;
    aPollSet->mEventHandle = -1;
    aPollSet->mEventsNum   = 0;
    aPollSet->mCurrEvent   = 0;

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpPollDestroy(acp_poll_set_t *aPollSet)
{
    ACP_TEST(0 != close(aPollSet->mHandle));

    acpMemFree(aPollSet->mObjs);
    acpMemFree(aPollSet->mEvents);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_NET_ERROR();
}

ACP_EXPORT acp_rc_t acpPollAddSock(acp_poll_set_t *aPollSet,
                                   acp_sock_t     *aSock,
                                   acp_sint32_t    aEvent,
                                   void           *aUserData)
{
    acp_poll_obj_t *sObj = NULL;
    acp_sint32_t    sRet;

    ACP_POLL_CHECK_BEFORE_ADD_OBJECT(aPollSet, aEvent);

    acpPollAddObj(aPollSet, &sObj, aSock, aEvent, aUserData);

    /* Associates specific events of a given object with a port */
    sRet  = port_associate(aPollSet->mHandle,
                           PORT_SOURCE_FD,
                           (uintptr_t)aSock->mHandle,
                           aEvent,
                           sObj);

    /* Handle error code */
    if (sRet == -1)
    {
        acpPollUnAddObj(aPollSet, sObj);

        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpPollRemoveSock(acp_poll_set_t *aPollSet,
                                      acp_sock_t     *aSock)
{
    acp_sint32_t sRet;

    if (aPollSet->mEventHandle == aSock->mHandle)
    {
        acpPollRemoveObj(aPollSet, aSock);

        aPollSet->mEventHandle = -1;
    }
    else
    {
        sRet = port_dissociate(aPollSet->mHandle,
                               PORT_SOURCE_FD,
                               (uintptr_t)aSock->mHandle);

        /* Handle error code */
        if (sRet == -1)
        {
            return ACP_RC_GET_NET_ERROR();
        }
        else
        {
            acpPollRemoveObj(aPollSet, aSock);
        }
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpPollDispatch(acp_poll_set_t      *aPollSet,
                                    acp_time_t           aTimeout,
                                    acp_poll_callback_t *aCallback,
                                    void                *aContext)
{
    acp_rc_t         sRC;
    struct timespec  sTimeout;
    struct timespec *sTimeoutPtr = NULL;
    acp_poll_obj_t  *sObjEvent = NULL;
    acp_poll_obj_t   sObjReturn;
    acp_uint32_t     sCount;
    acp_sint32_t     sEventHandle;
    acp_sint32_t     sRet;
    acp_sint32_t     sCallbackRet;
    acp_uint32_t     i;
    acp_uint32_t     j;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeoutPtr      = NULL;
    }
    else
    {
        sTimeout.tv_sec  = (acp_slong_t)acpTimeToSec(aTimeout);
        sTimeout.tv_nsec = (acp_slong_t)acpTimeGetUsec(aTimeout) * 1000;
        sTimeoutPtr      = &sTimeout;
    }

    sCount = 1;
    aPollSet->mEventsNum = 0;
    aPollSet->mCurrEvent = 0;

    /* Retrieve events from port */
    sRet   = port_getn(aPollSet->mHandle,
                       aPollSet->mEvents,
                       aPollSet->mMaxCount,
                       &sCount,
                       sTimeoutPtr);

    if(-1 == sRet)
    {
        sRC = ACP_RC_GET_NET_ERROR();
        switch(sRC)
        {
        case ETIME:
            /* Not all events had occured.
             * break and handle occured events */
            break;
        case EINTR:
            /* Interrupted while waiting for the events. */
            return ACP_RC_EINTR;
        default:
            ACP_RAISE(E_GETN);
            break;
        }
    }
    else
    {
        /* do nothing */
    }

    ACP_TEST_RAISE((sCount == 0), E_TIMEOUT);

    for (i = 0; i < sCount; i++)
    {
        sObjEvent              = aPollSet->mEvents[i].portev_user;

        sObjReturn.mSock       = sObjEvent->mSock;
        sObjReturn.mUserData   = sObjEvent->mUserData;
        sObjReturn.mReqEvent   = sObjEvent->mReqEvent;
        sObjReturn.mRtnEvent   = aPollSet->mEvents[i].portev_events;

        sEventHandle           = sObjEvent->mSock->mHandle;
        aPollSet->mEventHandle = sObjEvent->mSock->mHandle;

        /* Invoke callback */
        sCallbackRet = (*aCallback)(aPollSet, &sObjReturn, aContext);

        if (aPollSet->mEventHandle == sEventHandle)
        {
            sRet = port_associate(aPollSet->mHandle,
                                  PORT_SOURCE_FD,
                                  (uintptr_t)sObjEvent->mSock->mHandle,
                                  sObjEvent->mReqEvent,
                                  sObjEvent);

            ACP_TEST_RAISE((sRet == -1), E_ASSOCIATE);

            aPollSet->mEventHandle = -1;
        }
        else
        {
            /* do nothing */
        }

        if (sCallbackRet != 0)
        {
            /* Associate sockets left in the list */
            for (j = i + 1; j < sCount; j++)
            {
                sObjEvent = aPollSet->mEvents[j].portev_user;

                sRet = port_associate(aPollSet->mHandle,
                                      PORT_SOURCE_FD,
                                      (uintptr_t)sObjEvent->mSock->mHandle,
                                      sObjEvent->mReqEvent,
                                      sObjEvent);

                ACP_TEST_RAISE((sRet == -1), E_ASSOCIATE); 
            }

            aPollSet->mEventsNum = sCount;
            aPollSet->mCurrEvent = i;

            /* Stop event dispatching */
            ACP_RAISE(E_CANCEL);
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_SUCCESS;

    /* Handle exceptions */
    ACP_EXCEPTION(E_GETN)
    {
        /* Handle nothing */
    }

    ACP_EXCEPTION(E_TIMEOUT)
    {
        sRC = ACP_RC_ETIMEDOUT;
    }

    ACP_EXCEPTION(E_CANCEL)
    {
        sRC = ACP_RC_ECANCELED;
    }

    ACP_EXCEPTION(E_ASSOCIATE)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/* 
 * Resumes dispatching sockets after cancel
 */
ACP_EXPORT acp_rc_t acpPollDispatchResume(acp_poll_set_t      *aPollSet,
                                         acp_poll_callback_t *aCallback,
                                          void                *aContext)
{
    acp_rc_t         sRC;
    acp_poll_obj_t  *sObjEvent = NULL;
    acp_poll_obj_t   sObjReturn;
    acp_uint32_t     i;

    ACP_TEST_RAISE(aPollSet->mEventsNum == 0, E_NODATA);
    ACP_TEST_RAISE(aPollSet->mCurrEvent == aPollSet->mEventsNum, E_NODATA);

    for (i = aPollSet->mCurrEvent + 1; i < aPollSet->mEventsNum; i++)
    {
        sObjEvent              = aPollSet->mEvents[i].portev_user;

        sObjReturn.mSock       = sObjEvent->mSock;
        sObjReturn.mUserData   = sObjEvent->mUserData;
        sObjReturn.mReqEvent   = sObjEvent->mReqEvent;
        sObjReturn.mRtnEvent   = aPollSet->mEvents[i].portev_events;

        ACP_TEST_RAISE(((*aCallback)(aPollSet, &sObjReturn, aContext) != 0),
                      E_CANCEL);
    }

    aPollSet->mCurrEvent = i;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NODATA)
    {
        sRC = ACP_RC_EOF;
    }

    ACP_EXCEPTION(E_CANCEL)
    {
        /* Cancelled */
        aPollSet->mCurrEvent = i;
        sRC = ACP_RC_ECANCELED;
    }

    ACP_EXCEPTION_END;

    return sRC;
}
