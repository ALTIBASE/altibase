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
#include <dki.h>

static IDE_RC mmmPhaseActionInitDK( mmmPhase        aPhase,
                                    UInt             /*aOptionflag*/,
                                    mmmPhaseAction * /*aAction*/)
{
#if defined(WRS_VXWORKS)
    return IDE_SUCCESS;
#else

    switch ( aPhase )
    {
        case MMM_STARTUP_PRE_PROCESS:
            IDE_TEST( dkiDoPreProcessPhase() != IDE_SUCCESS );
            break;
            
        case MMM_STARTUP_PROCESS:
            IDE_TEST( dkiDoProcessPhase() != IDE_SUCCESS );
            break;
            
        case MMM_STARTUP_CONTROL:
            IDE_TEST( dkiDoControlPhase() != IDE_SUCCESS );
            break;
            
        case MMM_STARTUP_META:
            IDE_TEST( dkiDoMetaPhase() != IDE_SUCCESS );
            break;
            
        case MMM_STARTUP_SERVICE:
            IDE_TEST( dkiDoServicePhase( NULL ) != IDE_SUCCESS );
            break;
            
        case MMM_STARTUP_SHUTDOWN:
        default:
            IDE_CALLBACK_FATAL( "invalid startup phase" );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#endif
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitDK =
{
    (SChar *)"Initialize DK Module",
    0,
    mmmPhaseActionInitDK
};
