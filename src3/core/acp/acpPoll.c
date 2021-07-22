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
 * $Id: acpPoll.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpAlign.h>
#include <acpMem.h>
#include <acpPoll.h>
#include <acpProc.h>
#include <acpMem.h>
/*
 * I will add description in this section
 * djin. 2008. 10. 21.
 */

#define ACP_POLL_CHECK_BEFORE_CREATE(aPollSet, aMaxCount) \
    do                                                    \
    {                                                     \
        if ((aMaxCount) <= 0)                             \
        {                                                 \
            return ACP_RC_EINVAL;                         \
        }                                                 \
        else                                              \
        {                                                 \
        }                                                 \
    } while (0)


#define ACP_POLL_CHECK_BEFORE_ADD_OBJECT(aPollSet, aEvent)                  \
    do                                                                      \
    {                                                                       \
        static const acp_sint32_t sMask_MACRO_LOCAL_VAR = ~(ACP_POLL_IN |   \
                                                            ACP_POLL_OUT);  \
                                                                            \
        if (((aEvent) == 0) || (((aEvent) & sMask_MACRO_LOCAL_VAR) != 0))   \
        {                                                                   \
            return ACP_RC_EINVAL;                                           \
        }                                                                   \
        else                                                                \
        {                                                                   \
            if ((aPollSet)->mCurCount >= (aPollSet)->mMaxCount)             \
            {                                                               \
                return ACP_RC_ENOMEM;                                       \
            }                                                               \
            else                                                            \
            {                                                               \
            }                                                               \
        }                                                                   \
    } while (0)


#define ACP_POLL_DO_IF_HASH_NOTFOUND(aPollSet, aStep, aStmt)    \
    do                                                          \
    {                                                           \
        if ((aStep) == (aPollSet)->mMaxCount)                   \
        {                                                       \
            aStmt ;                                             \
        }                                                       \
        else                                                    \
        {                                                       \
        }                                                       \
    } while (0)

#define ACP_POLL_FIND_OUT_OBJECT(aObjs, aIterator, aInSet, aOutSet)     \
    if (((FD_ISSET((aObjs)[(aIterator)].mSock->mHandle, &(aInSet))) |   \
         (FD_ISSET((aObjs)[(aIterator)].mSock->mHandle, &(aOutSet)))) != 0) \


#if !defined(ALTI_CFG_OS_WINDOWS)

/* 
 * Hash function to generate a key from aStep and aSize 
 */
ACP_INLINE acp_sint32_t acpPollHash(acp_sint32_t aKey,
                                    acp_sint32_t aStep,
                                    acp_sint32_t aSize)
{
    return (aKey + aStep) % aSize;
}

/*
 * Insert polling object using hash
 */
ACP_INLINE void acpPollAddObj(acp_poll_set_t  *aPollSet,
                              acp_poll_obj_t **aObj,
                              acp_sock_t      *aSock,
                              acp_sint32_t     aEvent,
                              void            *aUserData)
{
    acp_poll_obj_t *sObjs = aPollSet->mObjs;
    acp_sint32_t    sKey  = aSock->mHandle;
    acp_sint32_t    sStep = 0;

    do
    {
        ACP_POLL_DO_IF_HASH_NOTFOUND(aPollSet, sStep, acpProcAbort());

        *aObj = &sObjs[acpPollHash(sKey, sStep++, aPollSet->mMaxCount)];
    } while ((*aObj)->mReqEvent != 0);

    (*aObj)->mReqEvent = aEvent;
    (*aObj)->mRtnEvent = 0;
    (*aObj)->mSock     = aSock;
    (*aObj)->mUserData = aUserData;

    aPollSet->mCurCount++;
}

/*
 * What is the difference between unadd and remove?
 */
ACP_INLINE void acpPollUnAddObj(acp_poll_set_t *aPollSet, acp_poll_obj_t *aObj)
{
    aObj->mReqEvent = 0;
    aObj->mRtnEvent = 0;
    aObj->mSock     = NULL;
    aObj->mUserData = NULL;

    aPollSet->mCurCount--;
}

/*
 * Remove object from hash table
 */
ACP_INLINE void acpPollRemoveObj(acp_poll_set_t *aPollSet, acp_sock_t *aSock)
{
    acp_poll_obj_t *sObjs = aPollSet->mObjs;
    acp_poll_obj_t *sObj = NULL;
    acp_sint32_t    sKey  = aSock->mHandle;
    acp_sint32_t    sStep = 0;

    do
    {
        ACP_POLL_DO_IF_HASH_NOTFOUND(aPollSet, sStep, return);

        sObj = &sObjs[acpPollHash(sKey, sStep++, aPollSet->mMaxCount)];
    } while ((sObj->mReqEvent == 0) || (sObj->mSock != aSock));

    sObj->mReqEvent = 0;
    sObj->mRtnEvent = 0;
    sObj->mSock     = NULL;
    sObj->mUserData = NULL;

    aPollSet->mCurCount--;
}

/*
 * Extract an object from aPollSet
 */
ACP_INLINE void acpPollGetObj(acp_poll_set_t *aPollSet,
                              acp_poll_obj_t *aObj,
                              acp_sint32_t    aHandle)
{
    acp_poll_obj_t *sObjTmp = NULL;
    acp_poll_obj_t *sObjs = aPollSet->mObjs;
    acp_sint32_t    sStep = 0;

    do
    {
        ACP_POLL_DO_IF_HASH_NOTFOUND(aPollSet, sStep, acpProcAbort());

        sObjTmp = &sObjs[acpPollHash(aHandle, sStep++, aPollSet->mMaxCount)];
    } while ((sObjTmp->mReqEvent      == 0) ||
             (sObjTmp->mSock          == NULL)  ||
             (sObjTmp->mSock->mHandle != aHandle));

    aObj->mReqEvent = sObjTmp->mReqEvent;
    aObj->mSock     = sObjTmp->mSock;
    aObj->mUserData = sObjTmp->mUserData;
}

#endif


#if defined(ACP_POLL_USES_DEVPOLL) && !defined(ACP_CFG_DOXYGEN)

#include <acpPoll-DEVPOLL.c>

#elif defined(ACP_POLL_USES_EPOLL) && !defined(ACP_CFG_DOXYGEN)

#include <acpPoll-EPOLL.c>

#elif defined(ACP_POLL_USES_POLL) || defined(ACP_CFG_DOXYGEN)

#include <acpPoll-POLL.c>

#elif defined(ACP_POLL_USES_POLLSET) && !defined(ACP_CFG_DOXYGEN)

#include <acpPoll-POLLSET.c>

#elif defined(ACP_POLL_USES_PORT) && !defined(ACP_CFG_DOXYGEN)

#include <acpPoll-PORT.c>

#elif defined(ACP_POLL_USES_SELECT) && !defined(ACP_CFG_DOXYGEN)

#include <acpPoll-SELECT.c>

#elif defined(ACP_POLL_USES_KQUEUE) && !defined(ACP_CFG_DOXYGEN)

#include <acpPoll-KQUEUE.c>

#else

#error acpPoll implementation not selected

#endif
