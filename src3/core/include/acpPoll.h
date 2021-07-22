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
 * $Id: acpPoll.h 11312 2010-06-22 08:08:48Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_POLL_H_)
#define _O_ACP_POLL_H_

/**
 * @file
 * @ingroup CoreNet
 */

#include <acpSock.h>


ACP_EXTERN_C_BEGIN


#if defined(ALTI_CFG_OS_AIX)
#if (ALTI_CFG_OS_MAJOR >= 5) && (ALTI_CFG_OS_MINOR >= 3)
#if (ACP_CFG_AIX_USEPOLL == 0)
#define ACP_POLL_USES_POLLSET
#else
#define ACP_POLL_USES_POLL
#endif
#else
#define ACP_POLL_USES_POLL
#endif
#elif defined(ALTI_CFG_OS_HPUX)
#if (ALTI_CFG_OS_MAJOR > 11) ||                                      \
    ((ALTI_CFG_OS_MAJOR == 11) && (ALTI_CFG_OS_MINOR >= 23))
#define ACP_POLL_USES_DEVPOLL
#else
#define ACP_POLL_USES_POLL
#endif
#elif defined(ALTI_CFG_OS_LINUX)
#if (ALTI_CFG_OS_MAJOR > 2) ||                                       \
    ((ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR >= 6))
#define ACP_POLL_USES_EPOLL
#else
#define ACP_POLL_USES_POLL
#endif
#elif defined(ALTI_CFG_OS_FREEBSD)
#define ACP_POLL_USES_KQUEUE
#elif defined(ALTI_CFG_OS_DARWIN)
#define ACP_POLL_USES_KQUEUE
#elif defined(ALTI_CFG_OS_SOLARIS)
#if (ALTI_CFG_OS_MAJOR > 2) ||                                       \
    ((ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR >= 10))
#define ACP_POLL_USES_PORT
#elif ((ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR >= 8))
#define ACP_POLL_USES_DEVPOLL
#else
#define ACP_POLL_USES_POLL
#endif
#elif defined(ALTI_CFG_OS_TRU64)
#define ACP_POLL_USES_POLL
#elif defined(ALTI_CFG_OS_WINDOWS)
#define ACP_POLL_USES_SELECT
#endif


#if defined(ACP_CFG_DOXYGEN)

/**
 * event for data in (read)
 */
#define ACP_POLL_IN
/**
 * event for data out (write)
 */
#define ACP_POLL_OUT

#else

#if defined(ACP_POLL_USES_EPOLL)
#define ACP_POLL_IN  EPOLLIN
#define ACP_POLL_OUT EPOLLOUT
#elif defined(ACP_POLL_USES_SELECT)
#define ACP_POLL_IN  1
#define ACP_POLL_OUT 2
#elif defined(ACP_POLL_USES_KQUEUE)
#define ACP_POLL_IN  EVFILT_READ
#define ACP_POLL_OUT EVFILT_WRITE
#else
#define ACP_POLL_IN  POLLIN
#define ACP_POLL_OUT POLLOUT
#endif

#endif


/**
 * object to describe returned event
 */
typedef struct acp_poll_obj_t
{
    acp_sint32_t  mReqEvent; /**< requested events            */
    acp_sint32_t  mRtnEvent; /**< returned events             */
    acp_sock_t   *mSock;     /**< the socket                  */
    void         *mUserData; /**< associated opaque user data */
} acp_poll_obj_t;

/**
 * poll set
 */
typedef struct acp_poll_set_t
{
    acp_sint32_t        mMaxCount;    /* number of object can be registered   */
    acp_sint32_t        mCurCount;    /* number of registered object          */
    acp_poll_obj_t     *mObjs;        /* registered object array              */
#if defined(ACP_POLL_USES_POLL)
    acp_sint32_t        mIterator;    /* dispatcher loop iterator             */
    struct pollfd      *mPolls;       /* pollfd array to invoke poll()        */
    acp_uint32_t        mEventsNum;   /* number of polled descriptors         */
#elif defined(ACP_POLL_USES_DEVPOLL)
    acp_sint32_t        mHandle;      /* file descriptor to /dev/poll         */
    struct pollfd      *mPolls;       /* returned pollfd array from /dev/poll */
    acp_uint32_t        mCurrEvent;   /* cancelled descriptor for resume      */
    acp_uint32_t        mEventsNum;   /* number of polled descriptors         */
#elif defined(ACP_POLL_USES_POLLSET)
    acp_sint32_t        mHandle;      /* pollset descriptor                   */
    struct pollfd      *mPolls;       /* returned pollfd array from pollset   */
    acp_uint32_t        mCurrEvent;   /* cancelled descriptor for resume      */
    acp_uint32_t        mEventsNum;   /* number of polled descriptors         */
#elif defined(ACP_POLL_USES_EPOLL)
    acp_sint32_t        mHandle;      /* epoll descriptor                     */
    struct epoll_event *mEvents;      /* returned event array from epoll      */
    acp_uint32_t        mCurrEvent;   /* cancelled descriptor for resume      */
    acp_uint32_t        mEventsNum;   /* number of polled descriptors         */
#elif defined(ACP_POLL_USES_PORT)
    acp_sint32_t        mHandle;      /* port descriptor                      */
    acp_sint32_t        mEventHandle; /* current dispatching event handle     */
    port_event_t       *mEvents;      /* returned event array from port       */
    acp_uint32_t        mCurrEvent;   /* cancelled descriptor for resume      */
    acp_uint32_t        mEventsNum;   /* number of polled descriptors         */
#elif defined(ACP_POLL_USES_SELECT)
#if !defined(ALTI_CFG_OS_WINDOWS)
    acp_sint32_t        mMaxHandle;   /* max handle value                     */
#endif
    acp_sint32_t        mIterator;    /* dispatcher loop iterator             */
    fd_set              mInSet;       /* fd set for select() - readibility    */
    fd_set              mOutSet;      /* fd set for select() - writability    */
    fd_set              mOldInSet;    /* fd set for select() - for resume     */
    fd_set              mOldOutSet;   /* fd set for select() - for resume     */
    acp_uint32_t        mEventsNum;   /* number of polled descriptors         */
#elif defined(ACP_POLL_USES_KQUEUE)
    acp_sint32_t         mHandle;     /* Kqueue descriptor                    */
    struct kevent       *mChangeList; /* Monitored events                     */
    struct kevent       *mEventList;  /* Triggered events                     */
    acp_uint32_t         mCurrEvent;  /* cancelled descriptor for resume      */
    acp_uint32_t         mEventsNum;  /* number of polled descriptors         */
#endif
} acp_poll_set_t;

/**
 * event handling callback function for acpPollDispatch()
 * @param aPollSet pointer to the poll set
 * @param aPollObj pointer to the returned object
 * @param aContext opaque data passed into acpPollDispatch()
 * @return 0 to continue polling, non-zero to stop polling
 */
typedef acp_sint32_t acp_poll_callback_t(acp_poll_set_t       *aPollSet,
                                         const acp_poll_obj_t *aPollObj,
                                         void                 *aContext);


ACP_EXPORT acp_rc_t acpPollCreate(acp_poll_set_t *aPollSet,
                                  acp_sint32_t    aMaxCount);

ACP_EXPORT acp_rc_t acpPollDestroy(acp_poll_set_t *aPollSet);

ACP_EXPORT acp_rc_t acpPollAddSock(acp_poll_set_t *aPollSet,
                                   acp_sock_t     *aSock,
                                   acp_sint32_t    aEvent,
                                   void           *aUserData);

ACP_EXPORT acp_rc_t acpPollRemoveSock(acp_poll_set_t *aPollSet,
                                      acp_sock_t     *aSock);

ACP_EXPORT acp_rc_t acpPollDispatch(acp_poll_set_t      *aPollSet,
                                    acp_time_t           aTimeout,
                                    acp_poll_callback_t *aCallback,
                                    void                *aContext);

ACP_EXPORT acp_rc_t acpPollDispatchResume(acp_poll_set_t      *aPollSet,
                                          acp_poll_callback_t *aCallback,
                                          void                *aContext);

ACP_EXTERN_C_END


#endif
