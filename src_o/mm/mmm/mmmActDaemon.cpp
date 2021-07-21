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
#include <idtContainer.h>
#include <mmm.h>
#include <mmi.h>

/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionDaemon(mmmPhase         /*aPhase*/,
                                   UInt             /*aOptionflag*/,
                                   mmmPhaseAction * /*aAction*/)
{
    //fix PROJ-1749
    if((mmi::getServerOption() & MMI_DAEMON_MASK) == MMI_DAEMON_TRUE)
    {
        //fix PROJ-1734
        if(idf::isVFS() == ID_TRUE )
        {
            IDE_TEST(ideLog::destroyStaticModule() != IDE_SUCCESS);

            IDE_TEST(idf::finalizeStatic() != IDE_SUCCESS);

            IDE_TEST(idlVA::daemonize(NULL, 1) != 0);

            IDE_TEST(idf::initializeStatic(NULL) != IDE_SUCCESS);

            IDE_TEST(ideLog::initializeStaticModule() != IDE_SUCCESS);
        }
        else
        {
            // daemonize되면, 모든 FD가 닫히므로, 다시 open 하도록 한다.
            IDE_TEST(ideLog::destroyStaticModule() != IDE_SUCCESS);
    
            IDE_TEST(idlVA::daemonize(idp::getHomeDir(), 1) != 0);
    
            IDE_TEST(ideLog::initializeStaticModule() != IDE_SUCCESS);
        }

        /*
         * PID가 달라졌으니 main thread의 bucket list를 재작성한다.
         */
        idtContainer::initializeBucket();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActDaemon =
{
    (SChar *)"Do Daemonizing..",
    0,
    mmmPhaseActionDaemon
};

