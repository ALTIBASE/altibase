/**
 *  Copyright (c) 1999~2018, Altibase Corp. and/or its affiliates. All rights reserved.
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

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <mmm.h>
#include <sdi.h>


static IDE_RC mmmPhaseActionShutdownSD(mmmPhase         /* aPhase */,
                                       UInt             /* aOptionflag */,
                                       mmmPhaseAction * /* aAction */)
{
    sdi::finalize();

    return IDE_SUCCESS;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActShutdownSD =
{
    (SChar *)"Shutdown Sharding",
    0,
    mmmPhaseActionShutdownSD
};

