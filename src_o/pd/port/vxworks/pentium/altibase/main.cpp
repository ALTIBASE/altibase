/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <mmi.h>

PDL_MAIN_OBJECT_MANAGER

extern "C" SInt serverStart()
{
    if( idlOS::getenv(IDP_HOME_ENV) == NULL )
    {
        idlOS::printf( "$s is NULL\n", IDP_HOME_ENV );

        return -1;
    }
    else
    {
        return mmi::serverStart( STARTUP_SERVICE, DEBUG_TRUE | SIGNAL_TRUE );
    }

    return 0;
}

extern "C" SInt serverProcess()
{
    if( idlOS::getenv(IDP_HOME_ENV) == NULL )
    {
        idlOS::printf( "$s is NULL\n", IDP_HOME_ENV );

        return -1;
    }
    else
    {
        return mmi::serverStart( STARTUP_PROCESS, DEBUG_TRUE | SIGNAL_TRUE );
    }

    return 0;
}

extern "C" SInt serverStop()
{
    if( idlOS::getenv(IDP_HOME_ENV) == NULL )
    {
        idlOS::printf( "$s is NULL\n", IDP_HOME_ENV );

        return -1;
    }
    else
    {
        return mmi::serverStop();
    }

    return 0;
}

extern "C" SInt serverKill()
{
    //BUGBUG
    return serverStop();
}
