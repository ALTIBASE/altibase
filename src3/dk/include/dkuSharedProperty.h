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
#include <idp.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideMsgLog.h>

#ifndef _O_DKU_SHARED_PROPERTY_H_
#define _O_DKU_SHARED_PROPERTY_H_ 1

#ifdef ALTIBASE_PRODUCT_XDB

#define DKU_DBLINK_ENABLE                       ( dkuSharedProperty::getDblinkEnable() )
#define DKU_DBLINK_DATA_BUFFER_BLOCK_COUNT      ( dkuSharedProperty::getDblinkDataBufferBlockCount() )
#define DKU_DBLINK_DATA_BUFFER_BLOCK_SIZE       ( dkuSharedProperty::getDblinkDataBufferBlockSize() )
#define DKU_DBLINK_DATA_BUFFER_ALLOC_RATIO      ( dkuSharedProperty::getDblinkDataBufferAllocRatio() )
#define DKU_DBLINK_GLOBAL_TRANSACTION_LEVEL     ( dkuSharedProperty::getDblinkGlobalTransactionLevel() )
#define DKU_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT  ( dkuSharedProperty::getDblinkRemoteStatementAutoCommit() )
#define DKU_DBLINK_ALTILINKER_CONNECT_TIMEOUT   ( dkuSharedProperty::getDblinkAltilinkerConnectTimeout() )
#define DKU_DBLINK_REMOTE_TABLE_BUFFER_SIZE     ( dkuSharedProperty::getDblinkRemoteTableBufferSize() )

typedef struct dkuShmProps
{
    UInt mDblinkEnable;

    UInt mDblinkDataBufferBlockCount;

    UInt mDblinkDataBufferBlockSize;

    UInt mDblinkDataBufferAllocRatio;

    UInt mDblinkGlobalTransactionLevel;

    UInt mDblinkRemoteStatementAutoCommit;

    UInt mDblinkAltilinkerConnectTimeout;

    UInt mDblinkRemoteTableBufferSize;    

    /*
     * This is not property, but it is used to share infomation.
     */
    UInt mAltilinkerRestartNumber;
    
} dkuShmProps;

typedef struct dkuStShmProps
{
    idShmAddr        mAddrSelf;
    dkuShmProps      mProps;
} dkuStShmProps;

class dkuSharedProperty
{
private:
    static dkuStShmProps    *mStShmProps;
    static dkuShmProps      *mShmProps;

    static IDE_RC   allocShm4StProps( idvSQL *aStatistics );
    static IDE_RC   freeShm4StProps( idvSQL *aStatistics );
    static IDE_RC   readProps( void );
    static IDE_RC   registNotifyFunc( void );

    static IDE_RC notifyDBLINK_GLOBAL_TRANSACTION_LEVEL( idvSQL * aStatistics,
                                                         SChar  * Name,
                                                         void   * aOldValue,
                                                         void   * aNewValue,
                                                         void   * aArg );

    static IDE_RC notifyDBLINK_REMOTE_STATEMENT_AUTOCOMMIT( idvSQL * aStatistics,
                                                            SChar  * Name,
                                                            void   * aOldValue,
                                                            void   * aNewValue,
                                                            void   * aArg );

    static IDE_RC notifyDBLINK_REMOTE_TABLE_BUFFER_SIZE( idvSQL * aStatistics,
                                                         SChar  * Name,
                                                         void   * aOldValue,
                                                         void   * aNewValue,
                                                         void   * aArg );

public:

    static IDE_RC   load( idvSQL *aStatistics );
    static IDE_RC   unLoad( idvSQL *aStatistics );
    static IDE_RC   checkConflict( void );

    static UInt getDblinkEnable( void );

    static UInt getDblinkDataBufferBlockCount( void );

    static UInt getDblinkDataBufferBlockSize( void );

    static UInt getDblinkDataBufferAllocRatio( void );

    static UInt getDblinkGlobalTransactionLevel( void );

    static UInt getDblinkRemoteStatementAutoCommit( void );

    static UInt getDblinkAltilinkerConnectTimeout( void );

    static UInt getDblinkRemoteTableBufferSize( void );
    
    /*
     * This is not property, but it is used to share infomation.
     */
    static UInt getAltilinkerRestartNumber( void );
    static void setAltilinkerRestartNumber( UInt aNumber );

} ;

#endif /* ALTIBASE_PRODUCT_XDB */

#endif
