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
 * V$DBLINK_LINKER_DATA_SESSION_INFO
 *
 *  ID ( integer )
 *  STATUS ( integer )
 *  GLOBAL_TRANSACTION_ID ( integer )
 */

#define QUERY_FOR_VIEW "CREATE VIEW V$DBLINK_LINKER_DATA_SESSION_INFO"     \
    "( ID, STATUS, LOCAL_TRANSACTION_ID, GLOBAL_TRANSACTION_ID ) "         \
    "AS SELECT "                                                           \
    " ID, STATUS, LOCAL_TRANSACTION_ID, GLOBAL_TRANSACTION_ID "            \
    "FROM X$DBLINK_LINKER_DATA_SESSION_INFO"  

/*
 * X$DBLINK_LINKER_DATA_SESSION_INFO
 *
 *  ID ( integer )
 *  STATUS ( integer )
 *  LOCAL_TRANSACTION_ID ( integer )
 *  GLOBAL_TRANSACTION_ID ( bigint )
 */

static iduFixedTableColDesc gFixedTableColDesc[] =
{
    {
        (SChar *)"ID",
        IDU_FT_OFFSETOF( dkmLinkerDataSessionInfo, mId ),
        IDU_FT_SIZEOF( dkmLinkerDataSessionInfo, mId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STATUS",
        IDU_FT_OFFSETOF( dkmLinkerDataSessionInfo, mStatus ),
        IDU_FT_SIZEOF( dkmLinkerDataSessionInfo, mStatus ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LOCAL_TRANSACTION_ID",
        IDU_FT_OFFSETOF( dkmLinkerDataSessionInfo, mLocalTransactionId ),
        IDU_FT_SIZEOF( dkmLinkerDataSessionInfo, mLocalTransactionId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"GLOBAL_TRANSACTION_ID",
        IDU_FT_OFFSETOF( dkmLinkerDataSessionInfo, mGlobalTransactionId ),
        IDU_FT_SIZEOF( dkmLinkerDataSessionInfo, mGlobalTransactionId ),
        IDU_FT_TYPE_UINTEGER,
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
    dkmLinkerDataSessionInfo * sInfo = NULL;
    UInt sInfoCount = 0;
    UInt i = 0;
    
    IDE_TEST( dkmGetLinkerDataSessionInfo( &sInfo,
                                           &sInfoCount  )
              != IDE_SUCCESS );

    for ( i = 0; i < sInfoCount; i ++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&(sInfo[i]) )
                  != IDE_SUCCESS );
    }

    IDE_TEST( dkmFreeInfoMemory( sInfo ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

static iduFixedTableDesc gFixedTableDesc =
{
    (SChar *)"X$DBLINK_LINKER_DATA_SESSION_INFO",
    dkivBuildRecord,
    gFixedTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


IDE_RC dkivRegisterDblinkLinkerDataSessionInfo( void )
{
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gFixedTableDesc );

    SChar * sQueryForView = (SChar *)QUERY_FOR_VIEW;

    IDE_TEST( qciMisc::addPerformanceView( sQueryForView ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
