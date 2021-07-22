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

#include <idd.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmtSessionManager.h>
#include <mtf.h>
#include <mtdTypes.h>

/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionInitRBHash(mmmPhase         /*aPhase*/,
                                       UInt             /*aOptionflag*/,
                                       mmmPhaseAction * /*aAction*/)
{
    IDE_TEST(iddRBHash::initializeStatic() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitRBHash =
{
    (SChar *)"Initialize Altibase Hash Tables",
    0,
    mmmPhaseActionInitRBHash
};

