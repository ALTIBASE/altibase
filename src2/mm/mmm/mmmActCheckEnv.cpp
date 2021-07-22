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

#include <idp.h>
#include <mmm.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionCheckShellEnv(mmmPhase        aPhase,
                                          UInt            /*aOptionflag*/,
                                          mmmPhaseAction *aAction)
{
    SChar *sHomeDir;

    sHomeDir = idlOS::getenv(IDP_HOME_ENV);

    IDE_TEST(sHomeDir == NULL);
    IDE_TEST(idlOS::strlen(sHomeDir) == 0);

    /*
      OK : ALTIBASE_HOME environment set
     */
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    mmm::logToSystem(aPhase, aAction);

    return IDE_FAILURE;

}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActCheckShellEnv =
{
    (SChar *)"Checking Altibase HOME Environemnts..",
    MMM_ACTION_NO_LOG,
    mmmPhaseActionCheckShellEnv
};

