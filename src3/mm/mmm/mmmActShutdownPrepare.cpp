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
#include <mmtAdminManager.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>


void mmm::prepareShutdown(SInt aMode, idBool /*aNewFlag*/)
{
    mmcTask *sTask = mmtAdminManager::getTask();

    mmm::setServerStatus(aMode);

    switch(aMode)
    {
        case ALTIBASE_STATUS_SHUTDOWN_NORMAL:
            break;
        case ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE:
            break;
    }

    IDE_ASSERT(mmtThreadManager::stopListener() == IDE_SUCCESS);

    mmtSessionManager::stop();

    if (sTask != NULL)
    {
        sTask->setShutdownTask(ID_TRUE);

        cmiShutdownLink(sTask->getLink(), CMI_DIRECTION_RD);
    }
}


static IDE_RC mmmPhaseActionShutdownPrepare(mmmPhase         /*aPhase*/,
                                            UInt             /*aOptionflag*/,
                                            mmmPhaseAction * /*aAction*/)
{
    // Check Emergency Shutdwon
    IDE_TEST_RAISE( (mmm::getEmergencyFlag() & SMI_EMERGENCY_MASK) != 0,
                    sm_emergency_error);
    return IDE_SUCCESS;

    IDE_EXCEPTION(sm_emergency_error);
    {
        if (mmm::getEmergencyFlag() & SMI_EMERGENCY_DB_MASK)
        {
            ideLog::log(IDE_SERVER_0, MM_TRC_DATABASE_FILE_DISK_FULL);
        }
        if (mmm::getEmergencyFlag() & SMI_EMERGENCY_LOG_MASK)
        {
            ideLog::log(IDE_SERVER_0, MM_TRC_LOG_FILE_DISK_FULL);
        }

        ideLog::log(IDE_SERVER_0, MM_TRC_SYSTEM_ABNORMAL_SHUTDOWN);
        idlOS::exit(-1);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActShutdownPrepare =
{
    (SChar *)"Preparing Shutdown...",
    0,
    mmmPhaseActionShutdownPrepare
};

