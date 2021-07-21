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

#include <cm.h>
#include <mmm.h>
#include <mmtThreadManager.h>
#include <mmtAdminManager.h>
#include <mmuProperty.h>

static IDE_RC mmmPhaseActionShutdownCM(mmmPhase         /*aPhase*/,
                                       UInt             /*aOptionflag*/,
                                       mmmPhaseAction * /*aAction*/)
{
#if !defined(WRS_VXWORKS)
    if (idf::isVFS() != ID_TRUE)
    {
        // BUG-15876
        IDE_TEST(mmm::mCheckStat.release() != IDE_SUCCESS);
        /* BUG 18294 */
        IDE_TEST(mmm::mCheckStat.destroy() != IDE_SUCCESS);
    }
#endif

    IDE_TEST(mmtAdminManager::finalize() != IDE_SUCCESS);

    IDE_TEST(mmtSessionManager::finalize() != IDE_SUCCESS);

    IDE_TEST(cmiFinalize() != IDE_SUCCESS);

    /* BUG-44488 */
    if (mmuProperty::getSslEnable() == ID_TRUE)
    {
        cmiSslFinalize();
    }

    /* PROJ-2681 */
    if (mmuProperty::getIBEnable() == ID_TRUE)
    {
        (void)cmiIBFinalize();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActShutdownCM =
{
    (SChar *)"Shutdown CM Module",
    0,
    mmmPhaseActionShutdownCM
};

