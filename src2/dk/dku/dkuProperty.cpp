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

/*
 * All system properties are registered at idpDescResource.cpp file.
 * Then each module manages module's system properties like this file.
 * If some property is writable, update callback function has to be
 * registered.
 */

#include <idl.h>
#include <ide.h>
#include <idp.h>

#include <dkDef.h>
#include <dkuProperty.h>

#ifdef ALTIBASE_PRODUCT_XDB

/*
 * Load and register update callback function.
 */ 
IDE_RC dkuProperty::load( void )
{
    return IDE_SUCCESS;
}

#else /* ALTIBASE_PRODUCT_XDB */

UInt dkuProperty::mDblinkEnable;
UInt dkuProperty::mDblinkDataBufferBlockCount;
UInt dkuProperty::mDblinkDataBufferBlockSize;
UInt dkuProperty::mDblinkDataBufferAllocRatio;
UInt dkuProperty::mGlobalTransactionLevel;
UInt dkuProperty::mDblinkRemoteStatementAutoCommit;
UInt dkuProperty::mDblinkAltilinkerConnectTimeout;
UInt dkuProperty::mDblinkRemoteTableBufferSize;
UInt dkuProperty::mDblinkRecoveryMaxLogfile;

/*
 * Load and register update callback function.
 */ 
IDE_RC dkuProperty::load( void )
{
    IDE_ASSERT( idp::read( (SChar *)"DBLINK_ENABLE", &mDblinkEnable )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"DBLINK_DATA_BUFFER_BLOCK_COUNT",
                           &mDblinkDataBufferBlockCount )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"DBLINK_DATA_BUFFER_BLOCK_SIZE",
                           &mDblinkDataBufferBlockSize )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"DBLINK_DATA_BUFFER_ALLOC_RATIO",
                           &mDblinkDataBufferAllocRatio )
                == IDE_SUCCESS );

    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        IDE_ASSERT( idp::read( (SChar *)"DBLINK_GLOBAL_TRANSACTION_LEVEL",
                               &mGlobalTransactionLevel )
                    == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( idp::read( (SChar *)"GLOBAL_TRANSACTION_LEVEL",
                               &mGlobalTransactionLevel )
                    == IDE_SUCCESS );
    }

    IDE_ASSERT( idp::read( (SChar *)"DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
                           &mDblinkRemoteStatementAutoCommit )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"DBLINK_ALTILINKER_CONNECT_TIMEOUT",
                           &mDblinkAltilinkerConnectTimeout )
                == IDE_SUCCESS );

    IDE_ASSERT( idp::read( (SChar *)"DBLINK_REMOTE_TABLE_BUFFER_SIZE",
                           &mDblinkRemoteTableBufferSize )
                == IDE_SUCCESS );

    if ( DKU_DBLINK_ENABLE == DK_ENABLE )
    {
        IDE_TEST( idp::setupAfterUpdateCallback(
                (const SChar *)"DBLINK_GLOBAL_TRANSACTION_LEVEL",
                dkuProperty::notifyGLOBAL_TRANSACTION_LEVEL )
            != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( idp::setupAfterUpdateCallback(
                (const SChar *)"GLOBAL_TRANSACTION_LEVEL",
                dkuProperty::notifyGLOBAL_TRANSACTION_LEVEL )
            != IDE_SUCCESS );
    }

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar *)"DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
                  dkuProperty::notifyDBLINK_REMOTE_STATEMENT_AUTOCOMMIT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                   (const SChar *)"DBLINK_REMOTE_TABLE_BUFFER_SIZE",
                   dkuProperty::notifyDBLINK_REMOTE_TABLE_BUFFER_SIZE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                   (const SChar *)"DBLINK_RECOVERY_MAX_LOGFILE",
                   dkuProperty::notifyDBLINK_RECOVERY_MAX_LOGFILE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt dkuProperty::getDblinkEnable( void )
{
    return mDblinkEnable;
}

UInt dkuProperty::getDblinkDataBufferBlockCount( void )
{
    return mDblinkDataBufferBlockCount;
}

UInt dkuProperty::getDblinkDataBufferBlockSize( void )
{
    return mDblinkDataBufferBlockSize;
}

UInt dkuProperty::getDblinkDataBufferAllocRatio( void )
{
    return mDblinkDataBufferAllocRatio;
}

UInt dkuProperty::getGlobalTransactionLevel( void )
{
    return mGlobalTransactionLevel;
}

UInt dkuProperty::getDblinkRemoteStatementAutoCommit( void )
{
    return mDblinkRemoteStatementAutoCommit;
}

UInt dkuProperty::getDblinkAltilinkerConnectTimeout( void )
{
    return mDblinkAltilinkerConnectTimeout;
}

UInt dkuProperty::getDblinkRemoteTableBufferSize( void )
{
    return mDblinkRemoteTableBufferSize;
}

UInt dkuProperty::getDblinkRecoveryMaxLogfile( void )
{
    return mDblinkRecoveryMaxLogfile;
}

IDE_RC dkuProperty::notifyGLOBAL_TRANSACTION_LEVEL( idvSQL* /* aStatistics */,
                                                    SChar * /* Name */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */ )
{
    idlOS::memcpy( &mGlobalTransactionLevel,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC dkuProperty::notifyDBLINK_REMOTE_STATEMENT_AUTOCOMMIT( idvSQL* /* aStatistics */,
                                                              SChar * /* Name */,
                                                              void  * /* aOldValue */,
                                                              void  * aNewValue,
                                                              void  * /* aArg */ )
{
    idlOS::memcpy( &mDblinkRemoteStatementAutoCommit,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC dkuProperty::notifyDBLINK_REMOTE_TABLE_BUFFER_SIZE( idvSQL* /* aStatistics */,
                                                           SChar * /* Name */,
                                                           void  * /* aOldValue */,
                                                           void  * aNewValue,
                                                           void  * /* aArg */ )
{
    idlOS::memcpy( &mDblinkRemoteTableBufferSize,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC dkuProperty::notifyDBLINK_RECOVERY_MAX_LOGFILE( idvSQL* /* aStatistics */,
                                                       SChar * /* aName */,
                                                       void  * /* aOldValue */,
                                                       void  * aNewValue,
                                                       void  * /* aArg */ )
{
    idlOS::memcpy( &mDblinkRecoveryMaxLogfile,
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

#endif /* ALTIBASE_PRODUCT_XDB */
