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

#if !defined(CM_DISABLE_IPC)

ACI_RC cmnDispatcherWaitLinkIPC(cmnLink      *aLink,
                                cmnDirection  aDirection,
                                acp_time_t    aTimeout)
{
    ACI_RC sRet = ACI_SUCCESS;

    // bug-27250 free Buf list can be crushed when client killed
    if (aDirection == CMN_DIRECTION_WR)
    {
        // receiver가 송신 허락 신호를 줄때까지 무한 대기
        // cmiWriteBlock에서 protocol end packet 송신시
        // pending block이 있는 경우 이 코드 수행
        if (aTimeout == ACP_TIME_INFINITE)
        {
            // client
            sRet = cmnLinkPeerWaitSendClientIPC(aLink);
        }
        // cmiWriteBlock에서 송신 대기 list를 넘어선 경우 수행.
        else
        {
            acpSleepMsec(1); // wait 1 msec
        }
    }
    return sRet;
}


#endif
