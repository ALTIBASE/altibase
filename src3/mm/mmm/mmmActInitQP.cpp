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

#include <qci.h>
#include <mmm.h>


static IDE_RC mmmPhaseActionInitQP(mmmPhase        aPhase,
                                   UInt             /*aOptionflag*/,
                                   mmmPhaseAction * /*aAction*/)
{
    qciStartupPhase sStartupPhase = QCI_STARTUP_MAX;

    switch(aPhase)
    {
        case MMM_STARTUP_PRE_PROCESS:
            sStartupPhase = QCI_STARTUP_PRE_PROCESS;
            break;
        case MMM_STARTUP_PROCESS:
            sStartupPhase = QCI_STARTUP_PROCESS;
            break;
        case MMM_STARTUP_CONTROL:
            sStartupPhase = QCI_STARTUP_CONTROL;
            break;
        case MMM_STARTUP_META:
            sStartupPhase = QCI_STARTUP_META;
            break;
        case MMM_STARTUP_SERVICE:
            sStartupPhase = QCI_STARTUP_SERVICE;
            break;
        case MMM_STARTUP_DOWNGRADE:
            /* PROJ-2689 */
            sStartupPhase = QCI_META_DOWNGRADE;
            break;
        case MMM_STARTUP_SHUTDOWN:
        default:
            IDE_CALLBACK_FATAL("invalid startup phase");
            break;
    }

    IDE_TEST(qci::startup(NULL, sStartupPhase) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitQP =
{
    (SChar *)"Initialize Query Processor",
    0,
    mmmPhaseActionInitQP
};
