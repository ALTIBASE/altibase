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
 * $Id: acpPoll-DEVPOLL.c 10502 2010-03-22 05:18:52Z denisb $
 ******************************************************************************/


ACP_EXPORT acp_rc_t acpPollCreate(acp_poll_set_t *aPollSet,
                                  acp_sint32_t    aMaxCount)
{
    acp_rc_t sRC;

    ACP_POLL_CHECK_BEFORE_CREATE(aPollSet, aMaxCount);

    aPollSet->mHandle = open("/dev/poll", O_RDWR);

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

    sRC = acpMemCalloc((void **)&aPollSet->mPolls,
                       aMaxCount,
                       sizeof(struct pollfd));

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

    aPollSet->mMaxCount  = aMaxCount;
    aPollSet->mCurCount  = 0;
    aPollSet->mEventsNum = 0;
    aPollSet->mCurrEvent = 0;

    acpMemSet(aPollSet->mObjs, 0, sizeof(acp_poll_obj_t) * aMaxCount);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpPollDestroy(acp_poll_set_t *aPollSet)
{
    ACP_TEST(0 != close(aPollSet->mHandle));

    acpMemFree(aPollSet->mObjs);
    acpMemFree(aPollSet->mPolls);
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_NET_ERROR();
}

ACP_EXPORT acp_rc_t acpPollAddSock(acp_poll_set_t *aPollSet,
                                   acp_sock_t     *aSock,
                                   acp_sint32_t    aEvent,
                                   void           *aUserData)
{
    struct pollfd   sPoll;
    acp_poll_obj_t *sObj = NULL;
    acp_ssize_t     sRet;

    ACP_POLL_CHECK_BEFORE_ADD_OBJECT(aPollSet, aEvent);

    sPoll.fd      = aSock->mHandle;
    sPoll.events  = aEvent;
    sPoll.revents = 0;

    sRet = write(aPollSet->mHandle, &sPoll, sizeof(sPoll));

    if (sRet == sizeof(sPoll))
    {
        acpPollAddObj(aPollSet, &sObj, aSock, aEvent, aUserData);

        return ACP_RC_SUCCESS;
    }
    else if (sRet == -1)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        return ACP_RC_EINVAL;
    }
}

ACP_EXPORT acp_rc_t acpPollRemoveSock(acp_poll_set_t *aPollSet,
                                      acp_sock_t     *aSock)
{
    struct pollfd sPoll;
    acp_ssize_t   sRet;

    sPoll.fd      = aSock->mHandle;
    sPoll.events  = POLLREMOVE;
    sPoll.revents = 0;

    sRet = write(aPollSet->mHandle, &sPoll, sizeof(sPoll));

    if (sRet == sizeof(sPoll))
    {
        acpPollRemoveObj(aPollSet, aSock);

        return ACP_RC_SUCCESS;
    }
    else if (sRet == -1)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        return ACP_RC_EINVAL;
    }
}

ACP_EXPORT acp_rc_t acpPollDispatch(acp_poll_set_t      *aPollSet,
                                    acp_time_t           aTimeout,
                                    acp_poll_callback_t *aCallback,
                                    void                *aContext)
{
    struct dvpoll  sPollCtl;
    acp_poll_obj_t sObj;
    acp_sint32_t   sTimeout;
    acp_sint32_t   sRet;
    acp_sint32_t   i;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeout = -1;
    }
    else
    {
        sTimeout = (acp_sint32_t)acpTimeToMsec(aTimeout + 999);
    }

    sPollCtl.dp_fds     = aPollSet->mPolls;
    sPollCtl.dp_nfds    = aPollSet->mMaxCount;
    sPollCtl.dp_timeout = sTimeout;

    aPollSet->mEventsNum = 0;
    aPollSet->mCurrEvent = 0;

    sRet = ioctl(aPollSet->mHandle, DP_POLL, &sPollCtl);

    if (sRet == -1)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else if (sRet == 0)
    {
        return ACP_RC_ETIMEDOUT;
    }
    else
    {
        for (i = 0; i < sRet; i++)
        {
            acpPollGetObj(aPollSet, &sObj, sPollCtl.dp_fds[i].fd);

            sObj.mRtnEvent = sPollCtl.dp_fds[i].revents;

            if ((*aCallback)(aPollSet, &sObj, aContext) != 0)
            {
                aPollSet->mEventsNum = sRet;
                aPollSet->mCurrEvent = i;

                return ACP_RC_ECANCELED;
            }
            else
            {
                /* continue */
            }
        }

        return ACP_RC_SUCCESS;
    }
}

/*
 * Resumes dispatching sockets after cancell
 */
ACP_EXPORT acp_rc_t acpPollDispatchResume(acp_poll_set_t      *aPollSet,
                                          acp_poll_callback_t *aCallback,
                                          void                *aContext)
{
    acp_rc_t       sRC;
    acp_poll_obj_t sObj;
    acp_uint32_t   i;
    struct dvpoll  sPollCtl;

    ACP_TEST_RAISE(aPollSet->mEventsNum == 0, E_NODATA);
    ACP_TEST_RAISE(aPollSet->mCurrEvent == aPollSet->mEventsNum, E_NODATA);

    sPollCtl.dp_fds = aPollSet->mPolls;

    for (i = aPollSet->mCurrEvent + 1; i < aPollSet->mEventsNum; i++)
    {
        acpPollGetObj(aPollSet, &sObj, sPollCtl.dp_fds[i].fd);
        sObj.mRtnEvent = sPollCtl.dp_fds[i].revents;

        ACP_TEST_RAISE(((*aCallback)(aPollSet, &sObj, aContext) != 0),
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
