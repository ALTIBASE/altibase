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
 * V$DBLINK_DATABASE_LINK_INFO
 *
 *  ID ( integer )
 *  STATUS ( integer )
 *  REFERENCE_COUNT ( integer )
 *  USER_MODE ( varchar )
 *  TARGET_INFO ( varchar )
 *  LINK_NAME ( varchar )
 */

#define QUERY_FOR_VIEW "CREATE VIEW V$DBLINK_DATABASE_LINK_INFO "          \
    "( ID, STATUS, REFERENCE_COUNT ) "                                     \
    "AS SELECT "                                                           \
    " ID, STATUS, REFERENCE_COUNT "                                        \
    "FROM X$DBLINK_DATABASE_LINK_INFO"                                  


/*
 * X$DBLINK_DATABASE_LINK_INFO
 *
 *  ID ( integer )
 *  STATUS ( integer )
 *  REFERENCE_COUNT ( integer )
 *  USER_MODE ( integer )
 *  TARGET_INFO ( varchar )
 *  LINK_NAME ( varchar )
 */

static iduFixedTableColDesc gFixedTableColDesc[] =
{
    {
        (SChar *)"ID",
        IDU_FT_OFFSETOF( dkmDatabaseLinkInfo, mId ),
        IDU_FT_SIZEOF( dkmDatabaseLinkInfo, mId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STATUS",
        IDU_FT_OFFSETOF( dkmDatabaseLinkInfo, mStatus ),
        IDU_FT_SIZEOF( dkmDatabaseLinkInfo, mStatus ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REFERENCE_COUNT",
        IDU_FT_OFFSETOF( dkmDatabaseLinkInfo, mReferenceCount ),
        IDU_FT_SIZEOF( dkmDatabaseLinkInfo, mReferenceCount ),
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
    dkmDatabaseLinkInfo * sDatabaseLinkInfo = NULL;
    UInt sDatabaseLinkInfoCount = 0;
    UInt i = 0;
    
    IDE_TEST( dkmGetDatabaseLinkInfo( &sDatabaseLinkInfo,
                                      & sDatabaseLinkInfoCount )
              != IDE_SUCCESS );

    for ( i = 0; i < sDatabaseLinkInfoCount; i ++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&(sDatabaseLinkInfo[i]) )
                  != IDE_SUCCESS );
    }

    IDE_TEST( dkmFreeInfoMemory( sDatabaseLinkInfo ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

static iduFixedTableDesc gFixedTableDesc =
{
    (SChar *)"X$DBLINK_DATABASE_LINK_INFO",
    dkivBuildRecord,
    gFixedTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


IDE_RC dkivRegisterDblinkDatabaseLinkInfo( void )
{
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gFixedTableDesc );

    SChar * sQueryForView = (SChar *)QUERY_FOR_VIEW;

    IDE_TEST( qciMisc::addPerformanceView( sQueryForView ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
