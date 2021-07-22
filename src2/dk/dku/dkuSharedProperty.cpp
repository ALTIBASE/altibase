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
#include <smiMisc.h>

#include <dkuSharedProperty.h>

#ifdef ALTIBASE_PRODUCT_XDB

dkuStShmProps   *dkuSharedProperty::mStShmProps;
dkuShmProps     *dkuSharedProperty::mShmProps;

/*
 *
 */
IDE_RC dkuSharedProperty::load( idvSQL *aStatistics )
{
    if ( mShmProps == NULL )
    {
        IDE_ASSERT( allocShm4StProps( aStatistics ) == IDE_SUCCESS );

        mShmProps = &(mStShmProps->mProps);

        IDE_ASSERT( mShmProps != NULL )
    }
    else
    {
        /* do nothing */
    }

    if ( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        IDE_TEST( readProps() != IDE_SUCCESS );

        IDE_TEST( checkConflict() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( registNotifyFunc() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkuSharedProperty::readProps( void )
{
    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        IDE_ASSERT( idp::read( (SChar *)"DBLINK_ENABLE", &(mShmProps->mDblinkEnable) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_DATA_BUFFER_BLOCK_COUNT",
                               &(mShmProps->mDblinkDataBufferBlockCount) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_DATA_BUFFER_BLOCK_SIZE",
                               &(mShmProps->mDblinkDataBufferBlockSize) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_DATA_BUFFER_ALLOC_RATIO",
                               &(mShmProps->mDblinkDataBufferAllocRatio) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_GLOBAL_TRANSACTION_LEVEL",
                               &(mShmProps->mDblinkGlobalTransactionLevel) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
                               &(mShmProps->mDblinkRemoteStatementAutoCommit) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_ALTILINKER_CONNECT_TIMEOUT",
                               &(mShmProps->mDblinkAltilinkerConnectTimeout) )
                    == IDE_SUCCESS );

        IDE_ASSERT( idp::read( (SChar *)"DBLINK_REMOTE_TABLE_BUFFER_SIZE",
                               &(mShmProps->mDblinkRemoteTableBufferSize) )
                    == IDE_SUCCESS );        
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkuSharedProperty::unLoad( idvSQL *aStatistics )
{
    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        IDE_ASSERT( freeShm4StProps( aStatistics ) == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkuSharedProperty::registNotifyFunc( void )
{
    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar *)"DBLINK_GLOBAL_TRANSACTION_LEVEL",
                  dkuSharedProperty::notifyDBLINK_GLOBAL_TRANSACTION_LEVEL )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar *)"DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
                  dkuSharedProperty::notifyDBLINK_REMOTE_STATEMENT_AUTOCOMMIT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback(
                  (const SChar *)"DBLINK_REMOTE_TABLE_BUFFER_SIZE",
                  dkuSharedProperty::notifyDBLINK_REMOTE_TABLE_BUFFER_SIZE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkuSharedProperty::checkConflict( void )
{
    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkuSharedProperty::allocShm4StProps( idvSQL *aStatistics )
{
    idShmAddr         sAddr4StProps;
    idrSVP            sVSavepoint;
    iduShmTxInfo    * sShmTxInfo         = NULL;

    sAddr4StProps  = IDU_SHM_NULL_ADDR;
    sShmTxInfo     = IDV_WP_GET_THR_INFO( aStatistics );

    idrLogMgr::setSavepoint( sShmTxInfo, &sVSavepoint );

    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        IDE_TEST( iduShmMgr::allocMem( aStatistics,
                                       sShmTxInfo,
                                       IDU_MEM_DK,
                                       ID_SIZEOF( dkuStShmProps ),
                                       (void **)&mStShmProps )
                  != IDE_SUCCESS );

        mStShmProps->mAddrSelf = iduShmMgr::getAddr( mStShmProps );

        iduShmMgr::setMetaBlockAddr(
            (iduShmMetaBlockType)IDU_SHM_META_DK_PROPERTIES_BLOCK,
            mStShmProps->mAddrSelf );
    }
    else
    {
        /* try to attach at shared memory. */
        sAddr4StProps =
            iduShmMgr::getMetaBlockAddr( IDU_SHM_META_DK_PROPERTIES_BLOCK );

        /* attach shared memory to local memory. */
        IDE_ASSERT( sAddr4StProps != IDU_SHM_NULL_ADDR );

        mStShmProps = (dkuStShmProps *)IDU_SHM_GET_ADDR_PTR( sAddr4StProps );

        IDE_ASSERT( mStShmProps != NULL );
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     sShmTxInfo,
                                     &sVSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, sShmTxInfo, &sVSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;

}

/*
 *
 */
IDE_RC dkuSharedProperty::freeShm4StProps( idvSQL *aStatistics )
{
    idrSVP           sVSavepoint;
    iduShmTxInfo   * sShmTxInfo = NULL;

    IDE_ASSERT( aStatistics != NULL );

    sShmTxInfo = IDV_WP_GET_THR_INFO( aStatistics );

    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        idrLogMgr::setSavepoint( sShmTxInfo, &sVSavepoint );

        IDE_TEST( iduShmMgr::freeMem( aStatistics,
                                      sShmTxInfo,
                                      &sVSavepoint,
                                      mStShmProps)
                  != IDE_SUCCESS );


        IDE_ASSERT( idrLogMgr::commit2Svp( aStatistics,
                                           sShmTxInfo,
                                           &sVSavepoint )
                    == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, sShmTxInfo, &sVSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkEnable( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkEnable;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkDataBufferBlockCount( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkDataBufferBlockCount;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkDataBufferBlockSize( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkDataBufferBlockSize;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkDataBufferAllocRatio( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkDataBufferAllocRatio;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkGlobalTransactionLevel( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkGlobalTransactionLevel;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkRemoteStatementAutoCommit( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkRemoteStatementAutoCommit;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkAltilinkerConnectTimeout( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkAltilinkerConnectTimeout;
}

/*
 *
 */
UInt dkuSharedProperty::getDblinkRemoteTableBufferSize( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mDblinkRemoteTableBufferSize;
}

/*
 *
 */
IDE_RC dkuSharedProperty::notifyDBLINK_GLOBAL_TRANSACTION_LEVEL( idvSQL * /* aStatistics */,
                                                                 SChar  * /* Name */,
                                                                 void   * /* aOldValue */,
                                                                 void   * aNewValue,
                                                                 void   * /* aArg */ )
{
    IDE_ASSERT( mShmProps != NULL );

    idlOS::memcpy( &(mShmProps->mDblinkGlobalTransactionLevel),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkuSharedProperty::notifyDBLINK_REMOTE_STATEMENT_AUTOCOMMIT( idvSQL * /* aStatistics */,
                                                                    SChar  * /* Name */,
                                                                    void   * /* aOldValue */,
                                                                    void   * aNewValue,
                                                                    void   * /* aArg */ )
{
    IDE_ASSERT( mShmProps != NULL );

    idlOS::memcpy( &(mShmProps->mDblinkRemoteStatementAutoCommit),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dkuSharedProperty::notifyDBLINK_REMOTE_TABLE_BUFFER_SIZE( idvSQL * /* aStatistics */,
                                                                 SChar  * /* Name */,
                                                                 void   * /* aOldValue */,
                                                                 void   * aNewValue,
                                                                 void   * /* aArg */ )
{
    IDE_ASSERT( mShmProps != NULL );

    idlOS::memcpy( &(mShmProps->mDblinkRemoteTableBufferSize),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/*
 *
 */
UInt dkuSharedProperty::getAltilinkerRestartNumber( void )
{
    IDE_ASSERT( mShmProps != NULL );

    return mShmProps->mAltilinkerRestartNumber;
}

/*
 *
 */ 
void dkuSharedProperty::setAltilinkerRestartNumber( UInt aNumber )
{
    IDE_ASSERT( mShmProps != NULL );

    mShmProps->mAltilinkerRestartNumber = aNumber;
}

#endif /* ALTIBASE_PRODUCT_XDB */
