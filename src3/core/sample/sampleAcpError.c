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
 
#include <acpError.h>


void readFromSocket1(acp_sock_t *aSock)
{
    acp_char_t sRecvBuffer[100];
    acp_size_t sRecvSize;
    acp_rc_t   sRC;

    sRC = acpSockRecv(aSock, sRecvBuffer, sizeof(sRecvBuffer), &sRecvSize, 0);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        /*
         * error
         */
    }
    else
    {
        /*
         * success
         */
    }
}

void readFromSocket2(acp_sock_t *aSock)
{
    acp_char_t sRecvBuffer[100];
    acp_size_t sRecvSize;
    acp_rc_t   sRC;

    sRC = acpSockRecv(aSock, sRecvBuffer, sizeof(sRecvBuffer), &sRecvSize, 0);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        /*
         * success
         */
    }
    else if (ACP_RC_IS_EAGAIN(sRC))
    {
        /*
         * EAGAIN
         */
    }
    else if (ACP_RC_IS_EINTR(sRC))
    {
        /*
         * EINTR
         */
    }
    else if (ACP_RC_IS_ECONNREFUSED(sRC))
    {
        /*
         * ECONNREFUSED
         */
    }
    else
    {
        /*
         * other error
         */
    }
}
