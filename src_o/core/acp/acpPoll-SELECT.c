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
 * $Id: acpPoll-SELECT.c 10600 2010-03-26 04:57:43Z denisb $
 ******************************************************************************/


ACP_INLINE void acpPollSetMaxHandle(acp_poll_set_t *aPollSet,
                                    acp_sock_t     *aSock)
{
#if defined (ALTI_CFG_OS_WINDOWS)
    ACP_UNUSED(aPollSet);
#else
    acp_poll_obj_t *sObjs   = aPollSet->mObjs;
    acp_sint32_t    sHandle = -1;
    acp_sint32_t    i;

    if (aSock->mHandle == aPollSet->mMaxHandle)
    {
        for (i = 0; i < aPollSet->mCurCount; i++)
        {
            if (sObjs[i].mSock->mHandle > sHandle)
            {
                sHandle = sObjs[i].mSock->mHandle;
            }
            else
            {
                /* do nothing */
            }
        }

        aPollSet->mMaxHandle = sHandle;
    }
    else
    {
        /* do nothing */
    }
#endif
}

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

#if defined(ALTI_CFG_OS_WINDOWS)
    if (aMaxCount > FD_SETSIZE)
    {
        acpMemFree(aPollSet->mObjs);
        return ACP_RC_ENOMEM;
    }
    else
    {
        /* do nothing */
    }
#endif

    FD_ZERO(&aPollSet->mInSet);
    FD_ZERO(&aPollSet->mOutSet);

    aPollSet->mMaxCount  = aMaxCount;
    aPollSet->mCurCount  = 0;
    aPollSet->mEventsNum = 0;
#if !defined(ALTI_CFG_OS_WINDOWS)
    aPollSet->mMaxHandle = -1;
#endif

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpPollDestroy(acp_poll_set_t *aPollSet)
{
    acpMemFree(aPollSet->mObjs);
    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpPollAddSock(acp_poll_set_t *aPollSet,
                                   acp_sock_t     *aSock,
                                   acp_sint32_t    aEvent,
                                   void           *aUserData)
{
    acp_poll_obj_t *sObj = NULL;

    ACP_POLL_CHECK_BEFORE_ADD_OBJECT(aPollSet, aEvent);

#if !defined(ALTI_CFG_OS_WINDOWS)
    if (aSock->mHandle > FD_SETSIZE)
    {
        return ACP_RC_ENOMEM;
    }
    else
    {
        /* do nothing */
    }

    if (aSock->mHandle > aPollSet->mMaxHandle)
    {
        aPollSet->mMaxHandle = aSock->mHandle;
    }
    else
    {
        /* do nothing */
    }
#endif

    if ((aEvent & ACP_POLL_IN) != 0)
    {
        FD_SET(aSock->mHandle, &aPollSet->mInSet);
    }
    else
    {
        /* do nothing */
    }

    if ((aEvent & ACP_POLL_OUT) != 0)
    {
        FD_SET(aSock->mHandle, &aPollSet->mOutSet);
    }
    else
    {
        /* do nothing */
    }

    sObj            = &aPollSet->mObjs[aPollSet->mCurCount];
    sObj->mReqEvent = aEvent;
    sObj->mRtnEvent = 0;
    sObj->mSock     = aSock;
    sObj->mUserData = aUserData;

    aPollSet->mCurCount++;

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpPollRemoveSock(acp_poll_set_t *aPollSet,
                                      acp_sock_t     *aSock)
{
    acp_poll_obj_t *sObjs = aPollSet->mObjs;
    acp_sint32_t    i;

    for (i = 0; i < aPollSet->mCurCount; i++)
    {
        if (sObjs[i].mSock->mHandle == aSock->mHandle)
        {
            aPollSet->mCurCount--;

            if ((sObjs[i].mReqEvent & ACP_POLL_IN) != 0)
            {
                FD_CLR(aSock->mHandle, &aPollSet->mInSet);
            }
            else
            {
                /* do nothing */
            }

            if ((sObjs[i].mReqEvent & ACP_POLL_OUT) != 0)
            {
                FD_CLR(aSock->mHandle, &aPollSet->mOutSet);
            }
            else
            {
                /* do nothing */
            }

            sObjs[i].mReqEvent = sObjs[aPollSet->mCurCount].mReqEvent;
            sObjs[i].mRtnEvent = sObjs[aPollSet->mCurCount].mRtnEvent;
            sObjs[i].mSock     = sObjs[aPollSet->mCurCount].mSock;
            sObjs[i].mUserData = sObjs[aPollSet->mCurCount].mUserData;

            if (aPollSet->mIterator >= i)
            {
                aPollSet->mIterator = i - 1;
            }
            else
            {
                /* do nothing */
            }

            acpPollSetMaxHandle(aPollSet, aSock);

            return ACP_RC_SUCCESS;
        }
        else
        {
            /* continue */
        }
    }

    return ACP_RC_ENOENT;
}

ACP_EXPORT acp_rc_t acpPollDispatch(acp_poll_set_t      *aPollSet,
                                    acp_time_t           aTimeout,
                                    acp_poll_callback_t *aCallback,
                                    void                *aContext)
{
    acp_poll_obj_t *sObjs = aPollSet->mObjs;
    acp_poll_obj_t  sObjReturn;
    acp_sint32_t    sRet;
    acp_sint32_t    sRtnEvent;
    fd_set          sInSet;
    fd_set          sOutSet;
    struct timeval  sTimeout;
    struct timeval *sTimeoutPtr = NULL;

    if (aTimeout == ACP_TIME_INFINITE)
    {
        sTimeoutPtr      = NULL;
    }
    else
    {
        sTimeout.tv_sec  = (long)acpTimeToSec(aTimeout);
        sTimeout.tv_usec = (long)acpTimeGetUsec(aTimeout);
        sTimeoutPtr      = &sTimeout;
    }

    acpMemCpy(&sInSet, &aPollSet->mInSet, sizeof(aPollSet->mInSet));
    acpMemCpy(&sOutSet, &aPollSet->mOutSet, sizeof(aPollSet->mOutSet));

    aPollSet->mEventsNum = 0;

#if defined(ALTI_CFG_OS_WINDOWS)
    sRet = select(0,
                  &sInSet,
                  &sOutSet,
                  NULL,
                  sTimeoutPtr);
#else
    sRet = select(aPollSet->mMaxHandle + 1,
                  &sInSet,
                  &sOutSet,
                  NULL,
                  sTimeoutPtr);
#endif

    /* in WIN32, (-1) means SOCKET_ERROR */
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
#if defined(ALTI_CFG_OS_WINDOWS)
      acpMemCpy(&aPollSet->mOldInSet, &sInSet, sizeof(aPollSet->mInSet));
      acpMemCpy(&aPollSet->mOldOutSet, &sOutSet, sizeof(aPollSet->mOutSet));
#endif
        aPollSet->mEventsNum = sRet;

        for (aPollSet->mIterator = 0;
             (aPollSet->mIterator < aPollSet->mCurCount) && (sRet > 0);
             aPollSet->mIterator++)
        {
            sRtnEvent = 0;

            if (FD_ISSET(sObjs[aPollSet->mIterator].mSock->mHandle, &sInSet))
            {
                sRtnEvent |= ACP_POLL_IN;
                FD_CLR(sObjs[aPollSet->mIterator].mSock->mHandle, &sInSet);
            }
            else
            {
                /* do nothing */
            }

            if (FD_ISSET(sObjs[aPollSet->mIterator].mSock->mHandle, &sOutSet))
            {
                sRtnEvent |= ACP_POLL_OUT;
                FD_CLR(sObjs[aPollSet->mIterator].mSock->mHandle, &sOutSet);
            }
            else
            {
                /* do nothing */
            }

            if (sRtnEvent != 0)
            {
                sObjReturn.mSock     = sObjs[aPollSet->mIterator].mSock;
                sObjReturn.mUserData = sObjs[aPollSet->mIterator].mUserData;
                sObjReturn.mReqEvent = sObjs[aPollSet->mIterator].mReqEvent;
                sObjReturn.mRtnEvent = sRtnEvent;

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
                /* do nothing */
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
    acp_rc_t        sRC;
    acp_poll_obj_t *sObjs = aPollSet->mObjs;
    acp_poll_obj_t  sObjReturn;
    fd_set          sInSet;
    fd_set          sOutSet;
    acp_sint32_t    sRtnEvent;
    acp_sint32_t    i;

    ACP_TEST_RAISE(aPollSet->mEventsNum == 0, E_NODATA);
    ACP_TEST_RAISE(aPollSet->mIterator >= aPollSet->mCurCount, E_NODATA);

    acpMemCpy(&sInSet, &aPollSet->mOldInSet, sizeof(aPollSet->mInSet));
    acpMemCpy(&sOutSet, &aPollSet->mOldOutSet, sizeof(aPollSet->mOutSet));

    for (i = aPollSet->mIterator + 1; i < aPollSet->mCurCount; i++)
    {
        sRtnEvent = 0;

        if (FD_ISSET(sObjs[i].mSock->mHandle, &sInSet))
        {
            sRtnEvent |= ACP_POLL_IN;
            FD_CLR(sObjs[i].mSock->mHandle, &sInSet);
        }
        else
        {
            /* do nothing */
        }

        if (FD_ISSET(sObjs[i].mSock->mHandle, &sOutSet))
        {
            sRtnEvent |= ACP_POLL_OUT;
            FD_CLR(sObjs[i].mSock->mHandle, &sOutSet);
        }
        else
        {
            /* do nothing */
        }

        if (sRtnEvent != 0)
        {

            sObjReturn.mSock     = sObjs[i].mSock;
            sObjReturn.mUserData = sObjs[i].mUserData;
            sObjReturn.mReqEvent = sObjs[i].mReqEvent;
            sObjReturn.mRtnEvent = sRtnEvent;
    
            ACP_TEST_RAISE(((*aCallback)(aPollSet, &sObjReturn, aContext) != 0),
                           E_CANCEL);
        }
        else
        {
            /* continue */
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
