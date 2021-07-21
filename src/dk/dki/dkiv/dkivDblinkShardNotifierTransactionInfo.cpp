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
 
#include <idl.h>
#include <ide.h>

#include <idu.h>

#include <qci.h>

#include <dkm.h>

/*
 * V$SHARD_NOTIFIER_TRANSACTION_INFO
 *
 *  GLOBAL_TRANSACTION_ID ( bigint )
 *  LOCAL_TRANSACTION_ID ( integer )
 *  XID ( varchar )
 *  GLOBAL_XID ( varchar )
 *  TRANSACTION_RESULT ( char )
 *  TARGET_INFO ( varchar )
 *  TRANSACTION_STATE( integer )
 *  GLOBAL_COMMIT_SCN( bigint )
 */

#define QUERY_FOR_VIEW "CREATE VIEW V$SHARD_NOTIFIER_TRANSACTION_INFO"      \
    "( GLOBAL_TRANSACTION_ID, TRANSACTION_ID, GLOBAL_XID, XID, REQUEST_TRANSACTION, TRANSACTION_RESULT, TARGET_INFO, TRANSACTION_STATE, COMMIT_SCN )" \
    "AS SELECT "                                                           \
    " GLOBAL_TRANSACTION_ID, TRANSACTION_ID, GLOBAL_XID, XID, REQUEST_TRANSACTION, TRANSACTION_RESULT, TARGET_INFO, TRANSACTION_STATE, COMMIT_SCN "         \
    "FROM X$SHARD_NOTIFIER_TRANSACTION_INFO"

static iduFixedTableColDesc gFixedTableColDesc[] =
{
    {
        (SChar *)"GLOBAL_TRANSACTION_ID",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mGlobalTransactionId ),
        IDU_FT_SIZEOF( dkmNotifierTransactionInfo, mGlobalTransactionId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANSACTION_ID",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mLocalTransactionId ),
        IDU_FT_SIZEOF( dkmNotifierTransactionInfo, mLocalTransactionId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"GLOBAL_XID",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mGlobalXID ),
        DKT_2PC_XID_STRING_LEN,
        IDU_FT_TYPE_VARCHAR,
        idaXaConvertXIDToString,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"XID",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mXID ),
        DKT_2PC_XID_STRING_LEN,
        IDU_FT_TYPE_VARCHAR,
        idaXaConvertXIDToString,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REQUEST_TRANSACTION",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mIsRequestNode ),
        IDU_FT_SIZEOF( dkmNotifierTransactionInfo, mIsRequestNode ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANSACTION_RESULT",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mTransactionResult ),
        10,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TARGET_INFO",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mTargetInfo ),
        DK_NAME_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SOURCE_XID",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mSourceXID ),
        DKT_2PC_XID_STRING_LEN,
        IDU_FT_TYPE_VARCHAR,
        idaXaConvertXIDToString,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANSACTION_STATE",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mTransactionState ),
        IDU_FT_SIZEOF( dkmNotifierTransactionInfo, mTransactionState ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"COMMIT_SCN",
        IDU_FT_OFFSETOF( dkmNotifierTransactionInfo, mGlobalCommitSCN ),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
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
    dkmNotifierTransactionInfo * sInfo = NULL;
    UInt sInfoCount = 0;
    UInt i = 0;
   
    IDE_TEST( dkmGetShardNotifierTransactionInfo( &sInfo,
                                                  &sInfoCount )
              != IDE_SUCCESS );

    for ( i = 0; i < sInfoCount; i++ )
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
    (SChar *)"X$SHARD_NOTIFIER_TRANSACTION_INFO",
    dkivBuildRecord,
    gFixedTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


IDE_RC dkivRegisterDblinkShardNotifierTransactionInfo( void )
{
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gFixedTableDesc );

    SChar * sQueryForView = (SChar *)QUERY_FOR_VIEW;

    IDE_TEST( qciMisc::addPerformanceView( sQueryForView ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
