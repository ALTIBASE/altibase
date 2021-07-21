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
 

/***********************************************************************
 * $id$
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <idu.h>

#include <qci.h>

#include <dkm.h>

/*
 * DBLINK_ALTILINKER_STATUS
 *
 *  STATUS ( integer )
 *  SESSION_COUNT ( integer )
 *  REMOTE_SESSION_COUNT ( integer )
 *  JVM_MEMORY_POOL_MAX_SIZE ( integer )
 *  JVM_MEMORY_USAGE ( bigint )
 *  START_TIME ( varchar )
 */

#define QUERY_FOR_VIEW "CREATE VIEW V$DBLINK_ALTILINKER_STATUS "        \
    "( STATUS, SESSION_COUNT, REMOTE_SESSION_COUNT,"                    \
    " JVM_MEMORY_POOL_MAX_SIZE, JVM_MEMORY_USAGE, START_TIME ) "        \
    "AS SELECT "                                                        \
    "STATUS, SESSION_COUNT, REMOTE_SESSION_COUNT, "                     \
    "JVM_MEMORY_POOL_MAX_SIZE, JVM_MEMORY_USAGE, START_TIME "           \
    "FROM X$DBLINK_ALTILINKER_STATUS"                                  

static iduFixedTableColDesc gFixedTableColDesc[] =
{
    {
        (SChar *)"STATUS",
        IDU_FT_OFFSETOF( dkmAltiLinkerStatus, mStatus ),
        IDU_FT_SIZEOF( dkmAltiLinkerStatus, mStatus ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SESSION_COUNT",
        IDU_FT_OFFSETOF( dkmAltiLinkerStatus, mSessionCount ),
        IDU_FT_SIZEOF( dkmAltiLinkerStatus, mSessionCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REMOTE_SESSION_COUNT",
        IDU_FT_OFFSETOF( dkmAltiLinkerStatus, mRemoteSessionCount ),
        IDU_FT_SIZEOF( dkmAltiLinkerStatus, mRemoteSessionCount ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"JVM_MEMORY_POOL_MAX_SIZE",
        IDU_FT_OFFSETOF( dkmAltiLinkerStatus, mJvmMemoryPoolMaxSize ),
        IDU_FT_SIZEOF( dkmAltiLinkerStatus, mJvmMemoryPoolMaxSize ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"JVM_MEMORY_USAGE",
        IDU_FT_OFFSETOF( dkmAltiLinkerStatus, mJvmMemoryUsage ),
        IDU_FT_SIZEOF( dkmAltiLinkerStatus, mJvmMemoryUsage ),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"START_TIME",
        IDU_FT_OFFSETOF( dkmAltiLinkerStatus, mStartTime ),
        IDU_FT_SIZEOF( dkmAltiLinkerStatus, mStartTime ),        
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }    
};

static IDE_RC dkivBuildRecord( idvSQL * /* aStatistics */,
			       void * aHeader,
			       void * /* aDumpObj */,
			       iduFixedTableMemory * aMemory )
{
    dkmAltiLinkerStatus sStatus;

    IDE_TEST( dkmGetAltiLinkerStatus( &sStatus ) != IDE_SUCCESS );
    
    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

static iduFixedTableDesc gFixedTableDesc =
{
    (SChar *)"X$DBLINK_ALTILINKER_STATUS",
    dkivBuildRecord,
    gFixedTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


IDE_RC dkivRegisterDblinkAltiLinkerStatus( void )
{
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gFixedTableDesc );
    
    SChar * sQueryForView = (SChar *)QUERY_FOR_VIEW;

    IDE_TEST( qciMisc::addPerformanceView( sQueryForView ) != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
