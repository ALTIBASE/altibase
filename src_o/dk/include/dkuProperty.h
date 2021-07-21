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
 * $Id$
 **********************************************************************/

#ifndef _O_DKU_PROPERTY_H_
#define _O_DKU_PROPERTY_H_ 1

#include <ide.h>

#ifdef ALTIBASE_PRODUCT_XDB

class dkuProperty
{
public:
    static IDE_RC load( void );
};

#else /* ALTIBASE_PRODUCT_XDB */

#define DKU_DBLINK_ENABLE                   ( dkuProperty::getDblinkEnable() )

#define DKU_DBLINK_DATA_BUFFER_BLOCK_COUNT   ( dkuProperty::getDblinkDataBufferBlockCount() )

#define DKU_DBLINK_DATA_BUFFER_BLOCK_SIZE ( dkuProperty::getDblinkDataBufferBlockSize() )

#define DKU_DBLINK_DATA_BUFFER_ALLOC_RATIO ( dkuProperty::getDblinkDataBufferAllocRatio() )

#define DKU_GLOBAL_TRANSACTION_LEVEL ( dkuProperty::getGlobalTransactionLevel() )

#define DKU_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT  ( dkuProperty::getDblinkRemoteStatementAutoCommit() )

#define DKU_DBLINK_ALTILINKER_CONNECT_TIMEOUT ( dkuProperty::getDblinkAltilinkerConnectTimeout() )

#define DKU_DBLINK_REMOTE_TABLE_BUFFER_SIZE ( dkuProperty::getDblinkRemoteTableBufferSize() )

#define DKU_DBLINK_RECOVERY_MAX_LOGFILE ( dkuProperty::getDblinkRecoveryMaxLogfile() )

class dkuProperty
{

public:
    static IDE_RC load( void );

    static UInt getDblinkEnable( void );

    static UInt getDblinkDataBufferBlockCount( void );

    static UInt getDblinkDataBufferBlockSize( void );

    static UInt getDblinkDataBufferAllocRatio( void );

    static UInt getGlobalTransactionLevel( void );

    static UInt getDblinkRemoteStatementAutoCommit( void );

    static UInt getDblinkAltilinkerConnectTimeout( void );    
    
    static UInt getDblinkRemoteTableBufferSize( void );

    static UInt getDblinkRecoveryMaxLogfile( void );

private:

    static UInt mDblinkEnable;

    static UInt mDblinkDataBufferBlockCount;

    static UInt mDblinkDataBufferBlockSize;

    static UInt mDblinkDataBufferAllocRatio;

    static UInt mGlobalTransactionLevel;

    static UInt mDblinkRemoteStatementAutoCommit;

    static UInt mDblinkAltilinkerConnectTimeout;
    
    static UInt mDblinkRemoteTableBufferSize;

    static UInt mDblinkRecoveryMaxLogfile;

    static IDE_RC notifyGLOBAL_TRANSACTION_LEVEL( idvSQL* /* aStatistics */,
                                                  SChar *aName,
                                                  void  *aOldValue,
                                                  void  *aNewValue,
                                                  void  *aArg );

    static IDE_RC notifyDBLINK_REMOTE_STATEMENT_AUTOCOMMIT( idvSQL* /* aStatistics */,
                                                            SChar *aName,
                                                            void  *aOldValue,
                                                            void  *aNewValue,
                                                            void  *aArg );

    static IDE_RC notifyDBLINK_REMOTE_TABLE_BUFFER_SIZE( idvSQL* /* aStatistics */,
                                                         SChar *aName,
                                                         void  *aOldValue,
                                                         void  *aNewValue,
                                                         void  *aArg );

    static IDE_RC notifyDBLINK_RECOVERY_MAX_LOGFILE( idvSQL* /* aStatistics */,
                                                     SChar *aName,
                                                     void  *aOldValue,
                                                     void  *aNewValue,
                                                     void  *aArg );

};
#endif /* ALTIBASE_PRODUCT_XDB */

#endif /* _O_DKU_PROPERTY_H_ */
