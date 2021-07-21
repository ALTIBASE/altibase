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
#include <idtContainer.h>
#include <idtCPUSet.h>
#include <iddRBHash.h>


/* =======================================================
 * Action Function
 * => Setup All Callback Functions
 * =====================================================*/

static IDE_RC mmmPhaseActionEndShutdown(mmmPhase         /*aPhase*/,
                                        UInt             /*aOptionflag*/,
                                        mmmPhaseAction * /*aAction*/)
{
    IDE_TEST(iddRBHash::destroyStatic() != IDE_SUCCESS);

    IDE_TEST(iduFatalCallback::destroyStatic() != IDE_SUCCESS);

    IDE_TEST(idtContainer::destroyStatic() != IDE_SUCCESS);

    IDE_TEST(idtCPUSet::destroyStatic() != IDE_SUCCESS);

    IDE_TEST(iduCond::destroyStatic() != IDE_SUCCESS);

    iduLatch::destroyStatic();

    IDE_TEST(iduMutexMgr::destroyStatic() != IDE_SUCCESS);

    IDE_TEST(iduMemMgr::logMemoryLeak() != IDE_SUCCESS);

    /*
     * BUG-32751
     * Change the order of destroy so that
     * every malloc'ed elements can be referenced
     * before the destruction of memory allocators
     */
    IDE_TEST(iduMemMgr::destroyStatic() != IDE_SUCCESS);
    
    ideLog::log(IDE_QP_0, MM_TRC_SHUTDOWN);

    /* BUG-49108 */
    ideLog::log(IDE_JOB_0, MM_TRC_SHUTDOWN);

    /* BUG-46138 */
    if ( iduProperty::getSdTrcEnable() == 1 )
    {
        ideLog::log( IDE_SD_0, MM_TRC_SHUTDOWN );
    }

    ideLog::log(IDE_RP_0, MM_TRC_SHUTDOWN);

    ideLog::log(IDE_SM_0, MM_TRC_SHUTDOWN);

    ideLog::log(IDE_SERVER_0, MM_TRC_SHUTDOWN_BOOT);

    IDE_TEST(ideLog::destroyStaticModule() != IDE_SUCCESS);
    IDE_TEST(ideLog::destroyStaticError() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActEndShutdown =
{
    (SChar *)"Setup End Shutdown..",
    MMM_ACTION_NO_LOG,
    mmmPhaseActionEndShutdown
};

