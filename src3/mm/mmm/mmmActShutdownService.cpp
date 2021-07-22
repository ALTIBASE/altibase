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
#include <mmdManager.h>
#include <mmcLob.h>
#include <mmcStatementManager.h>
#include <mmcTransManager.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>
#include <mmqManager.h>
#include <mmcPlanCache.h>
#include <mmtAuditManager.h>
#include <mmtJobManager.h>
#include <mmtSnapshotExportManager.h>

static IDE_RC mmmPhaseActionShutdownService(mmmPhase         /*aPhase*/,
                                            UInt             /*aOptionflag*/,
                                            mmmPhaseAction * /*aAction*/)
{
    /* PROJ-2626 Snapshot Export */
    IDE_TEST(mmtSnapshotExportManager::finalize() != IDE_SUCCESS );

    /* PROJ-1438 Job Scheduler */
    IDE_TEST(mmtJobManager::finalize() != IDE_SUCCESS );

    IDE_TEST_RAISE(mmtSessionManager::shutdown() != IDE_SUCCESS, SessionShutdownFail);

    // bug-34789: stmt table should be cleared after stopServiceThreads
    // 논리상, servcie thread가 다 종료한 후에 stmt table을 clear해야 함
    // service thread 종료로직에서 freeTask를 호출하는 부분이 있다.
    IDE_TEST(mmtThreadManager::stopServiceThreads() != IDE_SUCCESS);

    IDE_TEST(mmcLob::finalize() != IDE_SUCCESS);
    //fix BUG-22575 [valgrind] XA mmcTrans::free invalid read of size 4
    IDE_TEST(mmdManager::finalize() != IDE_SUCCESS);

    IDE_TEST(mmcTransManager::finalizeManager() != IDE_SUCCESS);

    IDE_TEST(mmcStatementManager::finalize() != IDE_SUCCESS);

    IDE_TEST(mmtThreadManager::finalize() != IDE_SUCCESS);

    IDE_TEST(mmqManager::finalize() != IDE_SUCCESS);

    //PROJ-1436 SQL-Plan Cache.
    IDE_TEST( mmcPlanCache::finalize() != IDE_SUCCESS);

    /* PROJ-2223 Altibase Auditing */
    IDE_TEST(mmtAuditManager::finalize() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionShutdownFail);
    {
        idlOS::exit(-1);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActShutdownService =
{
    (SChar *)"Shutdown Service Module",
    0,
    mmmPhaseActionShutdownService
};

