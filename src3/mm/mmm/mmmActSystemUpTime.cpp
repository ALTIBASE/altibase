/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mmm.h>
#include <mmErrorCode.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionSystemUpTime(mmmPhase         /*aPhase*/,
                                         UInt             /*aOptionflag*/,
                                         mmmPhaseAction * /*aAction*/)
{
    SChar                  sMakerBuf[1024];

    // get System Startup Time : PR-4455
    idlVA::getSystemUptime(sMakerBuf, sizeof(sMakerBuf) - 1);

    ideLog::log(IDE_SERVER_0, MM_TRC_SYSTEM_UPTIME, sMakerBuf);

    return IDE_SUCCESS;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActSystemUpTime =
{
    (SChar *)"Logging Host System Up-Time.",
    0,
    mmmPhaseActionSystemUpTime
};

