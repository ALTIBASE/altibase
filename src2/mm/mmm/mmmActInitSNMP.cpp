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
#include <mmsSNMP.h>
#include <mmsSNMPModule.h>
#include <idmSNMP.h>

static IDE_RC mmmPhaseActionInitSNMP(mmmPhase         /*aPhase*/,
                                     UInt             /*aOptionflag*/,
                                     mmmPhaseAction * /*aAction*/)
{
    IDE_TEST(idm::initialize(mmsSNMPModule::getModules(),
                             IDM_USE_FOR_SERVER) != IDE_SUCCESS);

    IDE_TEST(gMmsSNMP.initialize() != IDE_SUCCESS);

    IDE_TEST(idmSNMP::initialize() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitSNMP =
{
    (SChar *)"Initialize SNMP Service",
    0,
    mmmPhaseActionInitSNMP
};
