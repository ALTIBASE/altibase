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
#include <mmi.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionInitBootMsgLog(mmmPhase        aPhase,
                                           UInt            /*aOptionflag*/,
                                           mmmPhaseAction *aAction)
{
    idBool sDebug;

    sDebug = ((mmi::getServerOption() & MMI_DEBUG_TRUE) == MMI_DEBUG_TRUE)?
        ID_TRUE : ID_FALSE;
    IDE_TEST(ideLog::initializeStaticBoot(iduGetShmProcType(), sDebug) != IDE_SUCCESS);

    ideLog::log(IDE_SERVER_0, MM_TRC_STARTUP_BOOT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    mmm::logToSystem(aPhase, aAction);

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitBootMsgLog =
{
    (SChar *)"Initialize Boot-Up Message Logging System",
    MMM_ACTION_NO_LOG,
    mmmPhaseActionInitBootMsgLog
};

