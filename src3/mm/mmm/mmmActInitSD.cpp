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


static IDE_RC mmmPhaseActionInitSD( mmmPhase         aPhase,
                                    UInt             /* aOptionflag */,
                                    mmmPhaseAction * /* aAction */ )
{
    switch( aPhase )
    {
        case MMM_STARTUP_PRE_PROCESS:
            IDE_TEST(sdi::initialize() != IDE_SUCCESS);
            break;
        case MMM_STARTUP_PROCESS:
            break;
        case MMM_STARTUP_CONTROL:
            break;
        case MMM_STARTUP_META:
            break;
        case MMM_STARTUP_DOWNGRADE: /* PROJ-2689 */
        case MMM_STARTUP_SERVICE:
            IDE_TEST( sdi::loadMetaNodeInfo() != IDE_SUCCESS );
            break;
        case MMM_STARTUP_SHUTDOWN:
        default:
            IDE_CALLBACK_FATAL( "invalid startup phase" );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitSD =
{
    (SChar *)"Initialize Sharding",
    0,
    mmmPhaseActionInitSD
};

