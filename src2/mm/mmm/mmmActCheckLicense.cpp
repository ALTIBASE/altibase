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
#include <idp.h>
#include <idtCPUSet.h>
#include <iduCheckLicense.h>
#include <mmi.h>
#include <mmErrorCode.h>
#include <mmtSessionManager.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

IDE_RC callbackBeforeUpdateLicense(
            idvSQL * /*aStatistics*/,  
            SChar * /*aName*/,
            void  * /*aOldValue*/,
            void  *aNewValue,
            void  * /*aArg*/)
{
    SInt    sNewValue;

    IDE_TEST_RAISE(mmtSessionManager::isSysdba() != ID_TRUE, ENOTSYSDBA);
    sNewValue = *(SInt*)aNewValue;

    if(sNewValue == 1)
    {
        IDE_TEST(iduCheckLicense::update() != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOTSYSDBA)
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_ONLY_SYSDBA_CANDOTHAT));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// BUGBUG : 라이센스 없을 때 어떤현상인지 확인 필요
static IDE_RC mmmPhaseActionCheckLicense(mmmPhase         /*aPhase*/,
                                         UInt             /*aOptionflag*/,
                                         mmmPhaseAction * /*aAction*/)
{
    /*
     * TASK-6327
     * Community, Standard, and Enterprise Edition
     * Initialize CPU core information
     */
    IDE_TEST(idtCPUSet::initializeStatic() != IDE_SUCCESS);
    IDE_TEST(iduCheckLicense::initializeStatic() != IDE_SUCCESS);
    IDE_TEST(iduCheckLicense::check() != IDE_SUCCESS);

    idp::setupBeforeUpdateCallback ("__UPDATE_LICENSE", callbackBeforeUpdateLicense);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActCheckLicense =
{
    (SChar *)"Check License",
    0,
    mmmPhaseActionCheckLicense
};

