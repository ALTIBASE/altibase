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
 * $Id: acpPoll-KQUEUE.c 1878 2009-08-03 05:34:42Z denisb $
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

    /* Create kqueue */
    aPollSet->mHandle = kqueue();
    ACP_TEST_RAISE((aPollSet->mHandle == -1), E_KQUEUE);
  
    /* Allocate memory for objects */
    sRC = acpMemCalloc((void **)&aPollSet->mObjs,
                       aMaxCount,
                       sizeof(acp_poll_obj_t));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MEM_OBJS);

    /* Allocate memory for events to be monitored */
    sRC = acpMemCalloc((void **)&aPollSet->mChangeList,
                       aMaxCount,
                       sizeof(struct kevent));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MEM_CHANGELIST);

    /* Allocate memory for triggered events */
    sRC = acpMemCalloc((void **)&aPollSet->mEventList,
                       aMaxCount,
                       sizeof(struct kevent));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MEM_EVENTLIST);


    aPollSet->mMaxCount  = aMaxCount;
    aPollSet->mCurCount  = 0;
    aPollSet->mEventsNum = 0;
    aPollSet->mCurrEvent = 0;

    return ACP_RC_SUCCESS;

    /* Exception section */
    ACP_EXCEPTION(E_KQUEUE)
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }

    ACP_EXCEPTION(E_MEM_OBJS)
    {
        (void)close(aPollSet->mHandle);
    }    

    ACP_EXCEPTION(E_MEM_CHANGELIST)
    {
        acpMemFree(aPollSet->mObjs);

        (void)close(aPollSet->mHandle);
    }

    ACP_EXCEPTION(E_MEM_EVENTLIST)
    {
        acpMemFree(aPollSet->mChangeList);
        acpMemFree(aPollSet->mObjs);

        (void)close(aPollSet->mHandle);
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/**
 * destroys a poll set
 *
 * @param aPollSet pointer to the poll set object
 */
ACP_EXPORT acp_rc_t acpPollDestroy(acp_poll_set_t *aPollSet)
{
    ACP_TEST(0 != close(aPollSet->mHandle));

    acpMemFree(aPollSet->mChangeList);
    acpMemFree(aPollSet->mEventList);
    acpMemFree(aPollSet->mObjs);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_NET_ERROR();
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
    acp_poll_obj_t  *sObj = NULL;

    ACP_POLL_CHECK_BEFORE_ADD_OBJECT(aPollSet, aEvent);
    
    /* Initialize events to be monitored for kqueue */
    EV_SET(&aPollSet->mChangeList[aPollSet->mCurCount], 
           aSock->mHandle,
           aEvent, EV_ADD, 0, 0, aUserData);

    /* Add object */
    acpPollAddObj(aPollSet, &sObj, aSock, aEvent, aUserData);

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
    acp_sint32_t  i;
    acp_sint32_t  sRet;
    acp_rc_t      sRC;

    struct kevent *sEvent = NULL;
    struct kevent sChange;
    
    /* Look for socket in list of polled objects */
    for (i = 0; i < aPollSet->mCurCount; i++)
    {
        sEvent = &aPollSet->mChangeList[i];

        /* Found required object */
        if (sEvent->ident == (uintptr_t)aSock->mHandle)
        {
            /* Delete this object from list of polled objects *
             * by overwriting it with last object             */
            memcpy(&aPollSet->mChangeList[i], 
                  &aPollSet->mChangeList[aPollSet->mCurCount - 1],
                  sizeof(struct kevent));

            /* Remove object */
            acpPollRemoveObj(aPollSet, aSock);

            /* Disable object in kqueue to stop receiving   *
             * events. When socket is closed events will be * 
             * deleted automatically.                       */
            EV_SET(&sChange, 
                   aSock->mHandle, 
                   sEvent->filter, 
                   EV_ADD | EV_DISABLE, 
                   0, 
                   0, 
                   NULL);

            sRet = kevent(aPollSet->mHandle,
                          &sChange,
                          1,
                          NULL,
                          0,
                          NULL);
            if (sRet < 0)
            {
                sRC = ACP_RC_GET_NET_ERROR();
            }
            else
            {
                sRC = ACP_RC_SUCCESS;
            }

            return sRC;
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
    acp_sint32_t     sEventNum;
    acp_rc_t         sRC;

    acp_uint16_t     i;
    acp_poll_obj_t   sObj;

    struct timespec *sTimeoutPtr = NULL;
    struct timespec  sTimeout;
    acp_sint64_t     sNsec;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeoutPtr = NULL;
    }
    else
    {
        sNsec = acpTimeToMsec(aTimeout) * ACP_SINT64_LITERAL(1000000);

        sTimeout.tv_sec  = sNsec / ACP_SINT64_LITERAL(1000000000);
        sTimeout.tv_nsec = sNsec - 
                           sTimeout.tv_sec * ACP_SINT64_LITERAL(1000000000);

        sTimeoutPtr = &sTimeout;
    }

    aPollSet->mEventsNum = 0;
    aPollSet->mCurrEvent = 0;

    /* Wait for events  */
    sEventNum = kevent(aPollSet->mHandle,
                       aPollSet->mChangeList,
                       aPollSet->mCurCount,
                       aPollSet->mEventList,
                       aPollSet->mCurCount,
                       sTimeoutPtr);
    
    ACP_TEST_RAISE((sEventNum < 0), E_ERROR);
    
    ACP_TEST_RAISE((sEventNum == 0), E_TIMEOUT);
       
    for (i = 0; i < sEventNum; i++)
    {
        acpPollGetObj(aPollSet, 
                      &sObj, 
                      aPollSet->mEventList[i].ident);

        sObj.mRtnEvent = aPollSet->mEventList[i].filter;

        ACP_TEST_RAISE(((*aCallback)(aPollSet, &sObj, aContext) != 0),
                       E_CANCEL);
    }

    return ACP_RC_SUCCESS;

    /* Handle exceptions */
    ACP_EXCEPTION(E_TIMEOUT)
    {
        /* No events */
        sRC = ACP_RC_ETIMEDOUT;
    }

    ACP_EXCEPTION(E_ERROR)
    {
         /* Error case  */
        sRC = ACP_RC_GET_NET_ERROR();
    }

    ACP_EXCEPTION(E_CANCEL)
    {
        /* Cancelled */
        aPollSet->mEventsNum = sEventNum;
        aPollSet->mCurrEvent = i;

        sRC = ACP_RC_ECANCELED;
    }

    ACP_EXCEPTION_END;

    return sRC;
}


/**
 * Resumes dispatching sockets after cancel
 *
 * @param aPollSet pointer to the poll set object
 * @param aCallback callback function to handle an event
 * @param aContext opaque data passed to the callback function
 * @return result code
 *
 * if the callback function returns non-zero value,
 * acpPollDispatch() will stop dispatching and return #ACP_RC_ECANCELED.
 * acpPollDispatchResumt can resume stopped dispatching
 * to check another event (This event is concurrent event of previous event).
 *
 * If there are events and acpPollDispatchResume() has dispatched every event,
 * acpPollDispatchResume() returns #ACP_RC_SUCCESS.
 *
 * If there is no event and acpPollDispatchResume() does not dispatch any event,
 * acpPollDispatchResume() returns #ACP_RC_EOF.
 * If you get #ACP_RC_EOF, you can call acpPollDispatch() to check next event.
 */
ACP_EXPORT acp_rc_t acpPollDispatchResume(acp_poll_set_t      *aPollSet,
                                         acp_poll_callback_t *aCallback,
                                          void                *aContext)
{
    acp_rc_t         sRC;
    acp_poll_obj_t   sObj;
    acp_uint32_t     i;

    ACP_TEST_RAISE(aPollSet->mEventsNum == 0, E_NODATA);
    ACP_TEST_RAISE(aPollSet->mCurrEvent == aPollSet->mEventsNum, E_NODATA);

    for (i = aPollSet->mCurrEvent + 1; i < aPollSet->mEventsNum; i++)
    {
        acpPollGetObj(aPollSet,
                       &sObj,
                       aPollSet->mEventList[i].ident);

        sObj.mRtnEvent = aPollSet->mEventList[i].filter;

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
