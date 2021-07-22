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
 * $Id: acpPoll-POLL.c 10502 2010-03-22 05:18:52Z denisb $
 ******************************************************************************/


/**
 * creates a poll set
 *
 * @param aPollSet pointer to the poll set object
 * @param aMaxCount maximum number of objects that this poll set can hold
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aMaxCount is less than or equal to zero
 */
ACP_EXPORT acp_rc_t acpPollCreate(acp_poll_set_t *aPollSet,
                                  acp_sint32_t    aMaxCount)
{
    acp_rc_t sRC;

    ACP_POLL_CHECK_BEFORE_CREATE(aPollSet, aMaxCount);

    sRC = acpMemCalloc((void **)&aPollSet->mObjs,
                       aMaxCount,
                       sizeof(acp_poll_obj_t));

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
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

    return ACP_RC_SUCCESS;
}

/**
 * destroys a poll set
 *
 * @param aPollSet pointer to the poll set object
 */
ACP_EXPORT acp_rc_t acpPollDestroy(acp_poll_set_t *aPollSet)
{
    acpMemFree(aPollSet->mObjs);
    acpMemFree(aPollSet->mPolls);

    return ACP_RC_SUCCESS;
}

/**
 * adds a socket to the poll set
 *
 * @param aPollSet pointer to the poll set object
 * @param aSock pointer to the socket object to add
 * @param aEvent event mask to poll
 * (this mask is composed of #ACP_POLL_IN, #ACP_POLL_OUT)
 * @param aUserData associated opaque data
 * @return result code
 *
 * it returns #ACP_RC_ENOMEM if there is no more memory to hold the object
 *
 * it returns #ACP_RC_EINVAL if @a aEvent is empty (zero) or has invalid value
 */
ACP_EXPORT acp_rc_t acpPollAddSock(acp_poll_set_t *aPollSet,
                                   acp_sock_t     *aSock,
                                   acp_sint32_t    aEvent,
                                   void           *aUserData)
{
    struct pollfd  *sPoll = NULL;
    acp_poll_obj_t *sObj = NULL;

    ACP_POLL_CHECK_BEFORE_ADD_OBJECT(aPollSet, aEvent);

    sPoll           = &aPollSet->mPolls[aPollSet->mCurCount];
    sObj            = &aPollSet->mObjs[aPollSet->mCurCount];

    sPoll->fd       = aSock->mHandle;
    sPoll->events   = aEvent;
    sPoll->revents  = 0;

    sObj->mReqEvent = aEvent;
    sObj->mRtnEvent = 0;
    sObj->mSock     = aSock;
    sObj->mUserData = aUserData;

    aPollSet->mCurCount++;

    return ACP_RC_SUCCESS;
}

/**
 * removes a socket from the poll set
 *
 * @param aPollSet pointer to the poll set object
 * @param aSock pointer to the socket object to remove
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if there is no such socket in the poll set
 */
ACP_EXPORT acp_rc_t acpPollRemoveSock(acp_poll_set_t *aPollSet,
                                      acp_sock_t     *aSock)
{
    struct pollfd  *sPolls = aPollSet->mPolls;
    acp_poll_obj_t *sObjs  = aPollSet->mObjs;
    acp_sint32_t   i;

    for (i = 0; i < aPollSet->mCurCount; i++)
    {
        if (sPolls[i].fd == aSock->mHandle)
        {
            aPollSet->mCurCount--;

            sPolls[i].fd       = sPolls[aPollSet->mCurCount].fd;
            sPolls[i].events   = sPolls[aPollSet->mCurCount].events;
            sPolls[i].revents  = sPolls[aPollSet->mCurCount].revents;

            sObjs[i].mReqEvent = sObjs[aPollSet->mCurCount].mReqEvent;
            sObjs[i].mRtnEvent = sObjs[aPollSet->mCurCount].mRtnEvent;
            sObjs[i].mSock     = sObjs[aPollSet->mCurCount].mSock;
            sObjs[i].mUserData = sObjs[aPollSet->mCurCount].mUserData;

            if ((aPollSet->mIterator >= i) && (sPolls[i].revents != 0))
            {
                aPollSet->mIterator = i - 1;
            }
            else
            {
                /* do nothing */
            }

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

/**
 * dispatches events from the poll set
 *
 * @param aPollSet pointer to the poll set object
 * @param aTimeout timeout for waiting events
 * @param aCallback callback function to handle an event
 * @param aContext opaque data passed to the callback function
 * @return result code
 *
 * if the callback function returns non-zero value,
 * it will stop dispatching and return #ACP_RC_ECANCELED
 *
 * it returns #ACP_RC_ETIMEDOUT
 * if there is no event during the requested @a aTimeout
 *
 * specifying a @a aTimeout of #ACP_TIME_INFINITE
 * makes acpPollDispatch() wait indefinitely
 */
ACP_EXPORT acp_rc_t acpPollDispatch(acp_poll_set_t      *aPollSet,
                                    acp_time_t           aTimeout,
                                    acp_poll_callback_t *aCallback,
                                    void                *aContext)
{
    struct pollfd  *sPolls = aPollSet->mPolls;
    acp_poll_obj_t *sObjs  = aPollSet->mObjs;
    acp_poll_obj_t  sObjReturn;
    acp_sint32_t    sTimeout;
    acp_sint32_t    sRet;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeout = -1;
    }
    else
    {
        sTimeout = (acp_sint32_t)acpTimeToMsec(aTimeout + 999);
    }

    aPollSet->mEventsNum = 0;

    sRet = poll(sPolls, (nfds_t)aPollSet->mCurCount, sTimeout);

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
        aPollSet->mEventsNum = sRet;

        for (aPollSet->mIterator = 0;
             (aPollSet->mIterator < aPollSet->mCurCount) && (sRet > 0);
             aPollSet->mIterator++)
        {
            if (sPolls[aPollSet->mIterator].revents != 0)
            {
                sObjReturn.mSock     = sObjs[aPollSet->mIterator].mSock;
                sObjReturn.mUserData = sObjs[aPollSet->mIterator].mUserData;
                sObjReturn.mReqEvent = sObjs[aPollSet->mIterator].mReqEvent;
                sObjReturn.mRtnEvent = sPolls[aPollSet->mIterator].revents;

                sPolls[aPollSet->mIterator].revents = 0;

                if ((*aCallback)(aPollSet, &sObjReturn, aContext) != 0)
                {
                    return ACP_RC_ECANCELED;
                }
                else
                {
                    sRet--;
                }
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
 * Resumes dispatching sockets after cancel
 */
ACP_EXPORT acp_rc_t acpPollDispatchResume(acp_poll_set_t      *aPollSet,
                                          acp_poll_callback_t *aCallback,
                                          void                *aContext)
{
    acp_rc_t         sRC;
    struct pollfd   *sPolls = aPollSet->mPolls;
    acp_poll_obj_t  *sObjs  = aPollSet->mObjs;
    acp_poll_obj_t   sObjReturn;
    acp_uint32_t     i;

    ACP_TEST_RAISE(aPollSet->mEventsNum == 0, E_NODATA);
    ACP_TEST_RAISE(aPollSet->mIterator == aPollSet->mCurCount, E_NODATA);

    for (i = aPollSet->mIterator + 1; i < aPollSet->mCurCount; i++)
    {
        if (sPolls[i].revents != 0)
        {

            sObjReturn.mSock     = sObjs[i].mSock;
            sObjReturn.mUserData = sObjs[i].mUserData;
            sObjReturn.mReqEvent = sObjs[i].mReqEvent;
            sObjReturn.mRtnEvent = sPolls[i].revents;

            sPolls[i].revents = 0;

            ACP_TEST_RAISE(((*aCallback)(aPollSet, &sObjReturn, aContext) != 0),
                           E_CANCEL);
        }
        else
        {
            /* Do nothing */
        }
    }

    aPollSet->mIterator = i;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_NODATA)
    {
        sRC = ACP_RC_EOF;
    }

    ACP_EXCEPTION(E_CANCEL)
    {
        /* Cancelled */
        aPollSet->mIterator = i;
        sRC = ACP_RC_ECANCELED;
    }

    ACP_EXCEPTION_END;

    return sRC;
}
