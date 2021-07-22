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
#include <qci.h>


static IDE_RC mmmPhaseActionInitSecurity(mmmPhase         /*aPhase*/,
                                         UInt             /*aOptionflag*/,
                                         mmmPhaseAction * /*aAction*/)
{
    // PROJ-2002 Column Security
    (void) qciMisc::startSecurity(NULL);

    return IDE_SUCCESS;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitSecurity =
{
    (SChar *)"Initialize Security Module",
    0,
    mmmPhaseActionInitSecurity
};

 
