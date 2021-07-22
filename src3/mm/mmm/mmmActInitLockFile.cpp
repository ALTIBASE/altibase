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
#include <mmuProperty.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionInitLockFile(mmmPhase         /*aPhase*/,
                                         UInt             /*aOptionflag*/,
                                         mmmPhaseAction * /*aAction*/)
{
    /* ------------------------
     * 메시지 로깅 모듈 초기화
     * ---------------------- */
#if !defined(WRS_VXWORKS)
    idBool sRetry;

    if( idf::isVFS() != ID_TRUE )
    {
        IDE_TEST(mmm::mCheckStat.initialize() != IDE_SUCCESS);
        //fix BUG-17545.
        IDE_TEST(mmm::mCheckStat.createLockFile(&sRetry) != IDE_SUCCESS);
        /* BUG 18294 */
        IDE_TEST(mmm::mCheckStat.initFileLock() != IDE_SUCCESS);
        IDE_TEST(mmm::mCheckStat.tryhold() != IDE_SUCCESS);
    } 
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitLockFile =
{
    (SChar *)"Initialize Process Lock File",
    0,
    mmmPhaseActionInitLockFile
};

