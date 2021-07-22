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
#include <mmcLob.h>
#include <mmcStatementManager.h>
#include <mmcTransManager.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>
#include <mmtAuditManager.h>
#include <mmErrorCode.h>


static IDE_RC mmmPhaseActionInitService(mmmPhase         /*aPhase*/,
                                        UInt             /*aOptionflag*/,
                                        mmmPhaseAction * /*aAction*/)
{
    IDE_RC sRC;

    IDE_TEST(mmtSessionManager::initialize() != IDE_SUCCESS);

    IDE_TEST(mmcStatementManager::initialize() != IDE_SUCCESS);

    IDE_TEST(mmcLob::initialize() != IDE_SUCCESS);

    IDE_TEST(mmcTransManager::initializeManager() != IDE_SUCCESS);

#if !defined(WRS_VXWORKS) && !defined(SYMBIAN)
    IDE_TEST(mmtThreadManager::addListener(CMI_LINK_IMPL_UNIX, NULL, 0) != IDE_SUCCESS);

    /* TASK-5894 Permit sysdba via IPC */
    IDE_TEST(mmtThreadManager::addListener(CMI_LINK_IMPL_IPC, NULL, 0) != IDE_SUCCESS);

    /*PROJ-2616*/
    sRC = mmtThreadManager::addListener(CMI_LINK_IMPL_IPCDA, NULL, 0);
    IDE_TEST( sRC != IDE_SUCCESS && ideGetErrorCode() != mmERR_IGNORE_UNABLE_TO_MAKE_LISTENER );

#else
    sRC = mmtThreadManager::addListener(CMI_LINK_IMPL_TCP, NULL, 0);
    IDE_TEST( sRC != IDE_SUCCESS && ideGetErrorCode() != mmERR_IGNORE_UNABLE_TO_MAKE_LISTENER );
    
    /* TASK-5894 Permit sysdba via IPC */
    IDE_TEST(mmtThreadManager::addListener(CMI_LINK_IMPL_IPC, NULL, 0) != IDE_SUCCESS);

    sRC = mmtThreadManager::addListener(CMI_LINK_IMPL_SSL, NULL, 0);
    IDE_TEST( sRC != IDE_SUCCESS && ideGetErrorCode() != mmERR_IGNORE_UNABLE_TO_MAKE_LISTENER );

#endif

    IDE_TEST(mmtThreadManager::startServiceThreads() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitService =
{
    (SChar *)"Initialize Service Manager",
    0,
    mmmPhaseActionInitService
};

