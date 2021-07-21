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

#include <mmi.h>
#include <mmm.h>
#include <mmErrorCode.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionInitModuleMsgLog(mmmPhase         /*aPhase*/,
                                             UInt             /*aOptionflag*/,
                                             mmmPhaseAction * /*aAction*/)
{
    idBool sDebug;

    sDebug = ( ( mmi::getServerOption() & MMI_DEBUG_TRUE ) == MMI_DEBUG_TRUE )?
        ID_TRUE : ID_FALSE;


    /* ------------------------
     * 메시지 로깅 모듈 초기화
     * ---------------------- */
    IDE_TEST(ideLog::destroyStaticBoot() != IDE_SUCCESS);
    IDE_TEST(ideLog::initializeStaticModule(sDebug) != IDE_SUCCESS);

    ideLog::log(IDE_QP_0, MM_TRC_STARTUP);

    /* BUG-49108 */
    ideLog::log(IDE_JOB_0, MM_TRC_STARTUP);

    /* BUG-46138 */
    if ( iduProperty::getSdTrcEnable() == 1 )
    {
        ideLog::log( IDE_SD_0, MM_TRC_STARTUP );
    }

    ideLog::log(IDE_RP_0, MM_TRC_STARTUP);

    ideLog::log(IDE_SM_0, MM_TRC_STARTUP);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitModuleMsgLog =
{
    (SChar *)"Initialize All Module Message Logging System",
    0,
    mmmPhaseActionInitModuleMsgLog
};

