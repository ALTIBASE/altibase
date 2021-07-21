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
#include <iduVersion.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionVersionInfo(mmmPhase         /*aPhase*/,
                                        UInt             /*aOptionflag*/,
                                        mmmPhaseAction * /*aAction*/)
{
    ideLogEntry sLog(IDE_SERVER_0);
    sLog.appendFormat("%s %s %s",
                      iduVersionString,
                      iduGetSystemInfoString(),
                      iduGetProductionTimeString());
    sLog.write();

    return IDE_SUCCESS;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActVersionInfo =
{
    (SChar *)"Logging Altibase Version Information.",
    0,
    mmmPhaseActionVersionInfo
};

