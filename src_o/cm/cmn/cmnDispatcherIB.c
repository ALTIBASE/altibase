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

#include <cmAllClient.h>

extern cmnIB gIB;

ACI_RC cmnDispatcherWaitLinkIB( cmnLink      *aLink,
                                cmnDirection  aDirection,
                                acp_time_t    aTimeout )
{
    acp_sint32_t   *sSock;
    struct pollfd   sPollFd;
    acp_sint32_t    sTimeout = (aTimeout != ACP_TIME_INFINITE) ? acpTimeToMsec(aTimeout) : -1;
    acp_sint32_t    sRet;

    /* LinkÀÇ socket È¹µæ */
    ACI_TEST(aLink->mOp->mGetSocket(aLink, (void **)&sSock) != ACI_SUCCESS);
    
    sPollFd.fd = *sSock;

    switch (aDirection)
    {
        case CMN_DIRECTION_RD:
            sPollFd.events = POLLIN;
            break;
        case CMN_DIRECTION_WR:
            sPollFd.events = POLLOUT;
            break;
        case CMN_DIRECTION_RDWR:
            sPollFd.events = POLLIN | POLLOUT;
            break;
        default:
            sPollFd.events = 0;
            break;
    }

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    ACI_EXCEPTION_CONT(Restart);

    sPollFd.revents = 0;

    sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout); 

    ACI_TEST_RAISE(sRet == 0, TimedOut);

    if (sRet < 0)
    {
        /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
        ACI_TEST_RAISE(errno == EINTR, Restart);

        ACI_RAISE(PollError);
    }
    else
    {
        /* some events was occurred */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(PollError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RPOLL_ERROR));
    }
    ACI_EXCEPTION(TimedOut);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

