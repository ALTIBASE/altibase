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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ioLib.h>
#include <envLib.h>

#define USE_RAW_DEVICE 1

extern "C" int  serverStart();
extern "C" int  serverStop();
extern "C" int  serverProcess();
extern "C" void createdb( int aArg1 );
extern "C" int  ifs( int aArg1, int aArg2, int aArg3, int aArg4, int aArg5 );

#if defined(USE_RAW_DEVICE)
char *gCF = "";
#else
char *gCF = "/ata0a";
#endif

int server( int aArg1 )
{
    char env[1024];

    sprintf( env, "%s=%s/altibase_home", IDP_HOME_ENV, gCF );

    putenv( env );
    putenv( "IF_NAME=elPci0");

    if( getenv( IDP_HOME_ENV ) == NULL )
    {
        return -1;
    }
    else
    {
        printf( "%s=%s\n", IDP_HOME_ENV, getenv( IDP_HOME_ENV ) );
    }

    if( getenv( "IF_NAME" ) == NULL )
    {
        return -1;
    }
    else
    {
        printf( "IF_NAME=%s\n", getenv( "IF_NAME" ) );
    }

    if( strcmp( "start", (char *)aArg1 ) == 0 )
    {
        return serverStart();
    }
    else if( strcmp( "process", (char *)aArg1 ) == 0 )
    {
        return serverProcess();
    }
    else if( strcmp( "stop", (char *)aArg1 ) == 0 )
    {
        return serverStop();
    }
    else if( strcmp( "create", (char *)aArg1 ) == 0 )
    {
        (void)createdb( 4 ); // 4MB
    }
    else if( strcmp( "ifs1", (char *)aArg1 ) == 0 )
    {
        (void)ifs( (int)"-c", (int)"DEV1:", 0, 0, 0 );
    }

    return 0;
}
