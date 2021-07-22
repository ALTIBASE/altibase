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
 
#include <idu.h>
#include <dktDtxInfo.h>

IDE_RC dktDtxInfo::initialize( ID_XID * aGlobalXID,
                               UInt     aLocalTxId, 
                               UInt     aGlobalTxId,
                               idBool   aIsRequestNode )
{
    SChar sMutexName[IDU_MUTEX_NAME_LEN + 1] = { 0, };

    mLocalTxId = aLocalTxId;
    mGlobalTxId = aGlobalTxId;
    mResult = SMI_DTX_ROLLBACK;
    mBranchTxCount = 0;
    mLinkerType = DKT_LINKER_TYPE_NONE;
    mIsFailoverRequestNode = aIsRequestNode;
    mFailoverTrans = NULL;

    idlOS::memset( &(mPrepareLSN), 0x00, ID_SIZEOF( smLSN ) );
    IDU_LIST_INIT( &mBranchTxInfo );

    dktXid::initXID( &mXID );
    dktXid::initXID( &mGlobalXID );
    mIsPassivePending = ID_FALSE;

    if ( aGlobalXID != NULL )
    {
        dktXid::copyXID( &mGlobalXID, aGlobalXID );
    }

    SM_INIT_SCN( &mGlobalCommitSCN );

    idlOS::snprintf( sMutexName, 
                     IDU_MUTEX_NAME_LEN, 
                     "DKT_DTXINFO_GLOBAL_TX_"ID_UINT32_FMT"_MUTEX", 
                     aGlobalTxId );
    IDE_TEST_RAISE( mDtxInfoGlobalTxResultMutex.initialize( (SChar *)sMutexName,
                                                            IDU_MUTEX_KIND_POSIX,
                                                            IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_SET( ideSetErrorCode( dkERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dktDtxInfo::finalize()
{
    if ( mDtxInfoGlobalTxResultMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_DK_3 );
    }
}

IDE_RC dktDtxInfo::removeDtxBranchTx( ID_XID * aXID )
{
    iduList * sIterator = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;
    idBool               sIsRemoved = ID_FALSE;

    IDU_LIST_ITERATE( &mBranchTxInfo, sIterator )
    {
        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        if ( dktXid::isEqualXID( &(sBranchTxNode->mXID), aXID ) == ID_TRUE )
        {
            IDU_LIST_REMOVE( sIterator );
            (void)iduMemMgr::free( sBranchTxNode );
            sIsRemoved = ID_TRUE;
            mBranchTxCount--;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST_RAISE( sIsRemoved != ID_TRUE, ERR_NOT_FOUND_BRANCH_TX_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_BRANCH_TX_NODE );
    {
        ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_NOT_EXIST_DTX_BRANCH_INFO );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktDtxInfo::removeDtxBranchTx] sIsRemoved is ID_FALSE" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dktDtxInfo::removeDtxBranchTx( dktDtxBranchTxInfo * aDtxBranchTxInfo )
{
    iduList * sIterator = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;
    idBool               sIsRemoved = ID_FALSE;
    iduListNode *sNodeNext;

    IDU_LIST_ITERATE_SAFE( &mBranchTxInfo, sIterator, sNodeNext )
    {
        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        if ( dktXid::isEqualXID( &(sBranchTxNode->mXID),
                                 &(aDtxBranchTxInfo->mXID) ) == ID_TRUE )
        {
            IDU_LIST_REMOVE( sIterator );
            (void)iduMemMgr::free( sBranchTxNode );
            sIsRemoved = ID_TRUE;
            mBranchTxCount--;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_TEST_RAISE( sIsRemoved != ID_TRUE, ERR_NOT_FOUND_BRANCH_TX_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_BRANCH_TX_NODE );
    {
        ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_NOT_EXIST_DTX_BRANCH_INFO );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktDtxInfo::removeDtxBranchTx] sIsRemoved is ID_FALSE" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

dktDtxBranchTxInfo * dktDtxInfo::getDtxBranchTx( ID_XID * aXID )
{
    iduList * sIterator = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;

    IDU_LIST_ITERATE( &mBranchTxInfo, sIterator )
    {
        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        if ( dktXid::isEqualXID( &(sBranchTxNode->mXID), aXID ) == ID_TRUE )
        {
            break;
        }
        else
        {
            sBranchTxNode = NULL;
        }
    }

    return sBranchTxNode;
}

void dktDtxInfo::removeAllBranchTx()
{
    iduList            * sIterator     = NULL;
    iduListNode        * sNext         = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;

    IDU_LIST_ITERATE_SAFE( &mBranchTxInfo, sIterator, sNext )
    {
        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        IDU_LIST_REMOVE( sIterator );
        (void)iduMemMgr::free( sBranchTxNode );
    }

    mBranchTxCount = 0;

    return;
}

IDE_RC  dktDtxInfo::addDtxBranchTx( ID_XID * aXID,
                                    SChar  * aTarget )
{
    dktDtxBranchTxInfo * sDtxBranchTxInfo = NULL;

    /* 이미 shard로 사용하고 있는 경우 에러 */
    IDE_TEST_RAISE( mLinkerType == DKT_LINKER_TYPE_SHARD,
                    ERR_SHARD_TX_ALREADY_EXIST );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dktDtxBranchTxInfo ),
                                       (void **)&sDtxBranchTxInfo,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DTX_INFO );

    sDtxBranchTxInfo->mLinkerType = 'D'; /* dblink */
    sDtxBranchTxInfo->mIsValid    = ID_TRUE;

    idlOS::strncpy( sDtxBranchTxInfo->mData.mTargetName, aTarget, DK_NAME_LEN );
    sDtxBranchTxInfo->mData.mTargetName[ DK_NAME_LEN ] = '\0';
    dktXid::copyXID( &(sDtxBranchTxInfo->mXID), aXID );

    IDU_LIST_INIT_OBJ( &(sDtxBranchTxInfo->mNode), sDtxBranchTxInfo );
    IDU_LIST_ADD_LAST( &mBranchTxInfo, &(sDtxBranchTxInfo->mNode) );

    mBranchTxCount++;

    if ( mLinkerType == DKT_LINKER_TYPE_NONE )
    {
        mLinkerType = DKT_LINKER_TYPE_DBLINK;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_TX_ALREADY_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_IS_BEING_USED ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DTX_INFO );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    if ( sDtxBranchTxInfo != NULL )
    {
        (void)iduMemMgr::free( sDtxBranchTxInfo );
        sDtxBranchTxInfo = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

IDE_RC  dktDtxInfo::addDtxBranchTx( ID_XID              * aXID,
                                    sdiCoordinatorType    aCoordinatorType,
                                    SChar               * aNodeName,
                                    SChar               * aUserName,
                                    SChar               * aUserPassword,
                                    SChar               * aDataServerIP,
                                    UShort                aDataPortNo,
                                    UShort                aConnectType )
{
    dktDtxBranchTxInfo * sDtxBranchTxInfo = NULL;

    /* 이미 dblink로 사용하고 있는 경우 에러 */
    IDE_TEST_RAISE( mLinkerType == DKT_LINKER_TYPE_DBLINK,
                    ERR_DBLINK_TX_ALREADY_EXIST );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dktDtxBranchTxInfo ),
                                       (void **)&sDtxBranchTxInfo,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DTX_INFO );

    sDtxBranchTxInfo->mLinkerType = 'S'; /* shard node */
    sDtxBranchTxInfo->mIsValid    = ID_TRUE;

    /* set shard node info */
    sDtxBranchTxInfo->mData.mNode.mCoordinatorType = (dktCoordinatorType)aCoordinatorType;
    if ( aCoordinatorType == SDI_COORDINATOR_RESHARD )
    {
        if ( mIsPassivePending == ID_FALSE )
        {
            mIsPassivePending = ID_TRUE;
        }
    }
    idlOS::strncpy( sDtxBranchTxInfo->mData.mNode.mNodeName,
                    aNodeName,
                    DK_NAME_LEN + 1 );
    idlOS::strncpy( sDtxBranchTxInfo->mData.mNode.mUserName,
                    aUserName,
                    QCI_MAX_OBJECT_NAME_LEN + 1 );
    idlOS::strncpy( sDtxBranchTxInfo->mData.mNode.mUserPassword,
                    aUserPassword,
                    IDS_MAX_PASSWORD_LEN + 1 );
    idlOS::strncpy( sDtxBranchTxInfo->mData.mNode.mServerIP,
                    aDataServerIP,
                    SDI_SERVER_IP_SIZE );
    sDtxBranchTxInfo->mData.mNode.mPortNo = aDataPortNo;
    sDtxBranchTxInfo->mData.mNode.mConnectType = aConnectType;

    dktXid::copyXID( &(sDtxBranchTxInfo->mXID), aXID );

    IDU_LIST_INIT_OBJ( &(sDtxBranchTxInfo->mNode), sDtxBranchTxInfo );
    IDU_LIST_ADD_LAST( &mBranchTxInfo, &(sDtxBranchTxInfo->mNode) );

    mBranchTxCount++;

    if ( mLinkerType == DKT_LINKER_TYPE_NONE )
    {
        mLinkerType = DKT_LINKER_TYPE_SHARD;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DBLINK_TX_ALREADY_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_IS_BEING_USED ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DTX_INFO );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sDtxBranchTxInfo != NULL )
    {
        (void)iduMemMgr::free( sDtxBranchTxInfo );
        sDtxBranchTxInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

void dktDtxInfo::dumpBranchTx( SChar  * aBuf,
                               SInt     aBufSize,
                               UInt   * aBranchTxCnt ) /* out */
{
    iduList            * sIterator = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;
    UInt                 sShardBranchCounter = 0;
    UInt                 sBranchTxCnt = 0;
    UChar                sXidString[SMR_XID_DATA_MAX_LEN];
    SInt                 sLen = 0;

    IDU_LIST_ITERATE( &mBranchTxInfo, sIterator )
    {
        sLen += idlOS::snprintf( aBuf + sLen,
                                 aBufSize - sLen,
                                 "[ %"ID_UINT32_FMT" ] ",
                                 sBranchTxCnt );
        sBranchTxCnt++;

        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        (void)idaXaConvertXIDToString(NULL, &(sBranchTxNode->mXID), sXidString, SMR_XID_DATA_MAX_LEN);

        sLen += idlOS::snprintf( aBuf + sLen,
                                 aBufSize - sLen,
                                 "XID: %s, "
                                 "LinkerType: %c, ",
                                 sXidString,
                                 sBranchTxNode->mLinkerType );

        if ( sBranchTxNode->mLinkerType == 'D' )
        {
            sLen += idlOS::snprintf( aBuf + sLen,
                                     aBufSize - sLen,
                                     "TargetName: %s\n",
                                     sBranchTxNode->mData.mTargetName );
        }
        else
        {
            IDE_DASSERT( sBranchTxNode->mLinkerType == 'S' );

            if ( sShardBranchCounter == 0 )
            {
                (void)idaXaConvertXIDToString(NULL, &mXID, sXidString, SMR_XID_DATA_MAX_LEN);
                sLen += idlOS::snprintf( aBuf + sLen,
                                         aBufSize - sLen,
                                         "FromXID: %s, ",
                                         sXidString );
            }
            ++sShardBranchCounter;

           sLen += idlOS::snprintf( aBuf + sLen,
                                    aBufSize - sLen,
                                    "CoordinatorType: %"ID_UINT32_FMT", "
                                    "NodeName: %s, "
                                    "IP: %s, "
                                    "PORT: %"ID_UINT32_FMT", "
                                    "ConnectType: %"ID_UINT32_FMT"\n",
                                    sBranchTxNode->mData.mNode.mCoordinatorType,
                                    sBranchTxNode->mData.mNode.mNodeName,
                                    sBranchTxNode->mData.mNode.mServerIP,
                                    sBranchTxNode->mData.mNode.mPortNo,
                                    sBranchTxNode->mData.mNode.mConnectType );
        }
    }

    *aBranchTxCnt = sBranchTxCnt;
}

UInt  dktDtxInfo::estimateSerializeBranchTx()
{
    iduList            * sIterator = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;
    UInt                 sSize = 0;
    UInt                 sShardBranchCounter = 0;

    /* 길이 4byte */
    sSize += 4;
    /* 갯수 4byte */
    sSize += 4;

    IDU_LIST_ITERATE( &mBranchTxInfo, sIterator )
    {
        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        /* XID 길이 1byte */
        sSize += 1;
        /* XID */
        sSize += dktXid::sizeofXID( &(sBranchTxNode->mXID) );

        /* linker type 1byte */
        sSize += 1;

        if ( sBranchTxNode->mLinkerType == 'D' )
        {
            /* target name 길이 1byte */
            sSize += 1;
            /* target name (null 포함) */
            sSize += idlOS::strlen( sBranchTxNode->mData.mTargetName ) + 1;
        }
        else
        {
            IDE_DASSERT( sBranchTxNode->mLinkerType == 'S' );

            if ( sShardBranchCounter == 0 )
            {
                /* XID 길이 1byte */
                sSize += 1;
                /* XID */
                sSize += dktXid::sizeofXID( &(sBranchTxNode->mXID) );
            }
            ++sShardBranchCounter;

            /* node type 길이 1byte */
            sSize += 1;
            /* node name 길이 1byte */
            sSize += 1;
            /* node name (null 포함) */
            sSize += idlOS::strlen( sBranchTxNode->mData.mNode.mNodeName ) + 1;
            /* user name 길이 1byte */
            sSize += 1;
            /* user name (null 포함) */
            sSize += idlOS::strlen( sBranchTxNode->mData.mNode.mUserName ) + 1;
            /* user password 길이 1byte */
            sSize += 1;
            /* user password (null 포함) */
            sSize += idlOS::strlen( sBranchTxNode->mData.mNode.mUserPassword ) + 1;
            /* ip 길이 1byte */
            sSize += 1;
            /* ip (null 포함) */
            sSize += idlOS::strlen( sBranchTxNode->mData.mNode.mServerIP ) + 1;
            /* port no 2byte */
            sSize += 2;
            /* connect type 2byte */
            sSize += 2;
        }
    }

    return sSize;
}

/* aBranchTxInfo는 호출이전에 estimate한 크기로 할당되어 있다. */
IDE_RC  dktDtxInfo::serializeBranchTx( UChar * aBranchTxInfo, UInt aSize )
{
    iduList            * sIterator = NULL;
    dktDtxBranchTxInfo * sBranchTxNode = NULL;
    UChar              * sBuffer = NULL;
    UChar              * sFence = NULL;
    UChar                sXidLen;
    UInt                 sShardBranchCounter = 0;

    sBuffer = aBranchTxInfo;
    sFence = sBuffer + aSize;

    /* 길이 4byte */
    /* 마지막에 기록한다. */
    sBuffer += 4;

    /* 갯수 4byte */
    IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
    ID_4_BYTE_ASSIGN( sBuffer, &mBranchTxCount );
    sBuffer += 4;

    IDU_LIST_ITERATE( &mBranchTxInfo, sIterator )
    {
        sBranchTxNode = (dktDtxBranchTxInfo *) sIterator->mObj;

        /* XID 길이 1byte */
        sXidLen = dktXid::sizeofXID(&(sBranchTxNode->mXID));
        IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
        ID_1_BYTE_ASSIGN( sBuffer, &sXidLen );
        sBuffer += 1;

        /* XID */
        IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
        idlOS::memcpy( sBuffer, &(sBranchTxNode->mXID), sXidLen );
        sBuffer += sXidLen;

        /* linker type 1byte */
        IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
        ID_1_BYTE_ASSIGN( sBuffer, &(sBranchTxNode->mLinkerType) );
        sBuffer += 1;

        if ( sBranchTxNode->mLinkerType == 'D' )
        {
            /* target name string */
            IDE_TEST( copyString( &sBuffer, sFence, sBranchTxNode->mData.mTargetName )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_DASSERT( sBranchTxNode->mLinkerType == 'S' );

            if ( sShardBranchCounter == 0 )
            {
                /* XID 길이 1byte */
                sXidLen = dktXid::sizeofXID( &mXID );
                IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
                ID_1_BYTE_ASSIGN( sBuffer, &sXidLen );
                sBuffer += 1;

                /* XID */
                IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
                idlOS::memcpy( sBuffer, &mXID, sXidLen );
                sBuffer += sXidLen;
            }
            ++sShardBranchCounter;

            /* node type 1byte */
            IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
            ID_1_BYTE_ASSIGN( sBuffer, &(sBranchTxNode->mData.mNode.mCoordinatorType) );
            sBuffer += 1;

            /* node name string */
            IDE_TEST( copyString( &sBuffer, sFence, sBranchTxNode->mData.mNode.mNodeName )
                      != IDE_SUCCESS );

            /* user name string */
            IDE_TEST( copyString( &sBuffer, sFence, sBranchTxNode->mData.mNode.mUserName )
                      != IDE_SUCCESS );

            /* user password string */
            IDE_TEST( copyString( &sBuffer, sFence, sBranchTxNode->mData.mNode.mUserPassword )
                      != IDE_SUCCESS );

            /* server ip string */
            IDE_TEST( copyString( &sBuffer, sFence, sBranchTxNode->mData.mNode.mServerIP )
                      != IDE_SUCCESS );

            /* port no 2byte */
            IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
            ID_2_BYTE_ASSIGN( sBuffer, &(sBranchTxNode->mData.mNode.mPortNo) );
            sBuffer += 2;

            /* connect type 2byte */
            IDE_TEST_RAISE( sBuffer >= sFence, ERR_OVERFLOW );
            ID_2_BYTE_ASSIGN( sBuffer, &(sBranchTxNode->mData.mNode.mConnectType) );
            sBuffer += 2;
        }
    }

    IDE_TEST_RAISE( sBuffer != sFence, ERR_OVERFLOW );

    /* 길이 4byte */
    ID_4_BYTE_ASSIGN( aBranchTxInfo, &aSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktDtxInfo::serializeBranchTx] buffer overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* serialize된 branchTxInfo를 unserialize하고 추가한다. */
IDE_RC  dktDtxInfo::unserializeAndAddDtxBranchTx( UChar * aBranchTxInfo, UInt aSize )
{
    UInt           sBranchTxSize = 0;
    UInt           sBranchTxCount = 0;
    UChar        * sBuffer = NULL;
    UChar          sLen = 0;
    SChar          sLinkerType = 0;
    UChar          sCoordinatorType = 0;
    SChar        * sTargetName = NULL;
    SChar        * sNodeName = NULL;
    SChar        * sUserName = NULL;
    SChar        * sUserPassword = NULL;
    SChar        * sDataServerIP = NULL;
    UShort         sDataPortNo = 0;
    UShort         sConnectType = 0;
    ID_XID         sXID;
    UChar          sXidLen;
    UInt           i;
    UInt           sShardBranchCounter = 0;

    sBuffer = aBranchTxInfo;

    /* 길이 4byte */
    ID_4_BYTE_ASSIGN( &sBranchTxSize, sBuffer );
    sBuffer += 4;

    IDE_TEST_RAISE( sBranchTxSize != aSize, ERR_INVALID_BRANCH_TX_INFO );

    /* 갯수 4byte */
    ID_4_BYTE_ASSIGN( &sBranchTxCount, sBuffer );
    sBuffer += 4;

    for ( i = 0; i < sBranchTxCount; i++ )
    {
        /* XID 길이 1byte */
        ID_1_BYTE_ASSIGN( &sXidLen, sBuffer );
        sBuffer += 1;

        /* XID */
        idlOS::memcpy( &sXID, sBuffer, sXidLen );
        sBuffer += sXidLen;

        /* linker type 1byte */
        ID_1_BYTE_ASSIGN( &sLinkerType, sBuffer );
        sBuffer += 1;

        if ( sLinkerType == 'D' )
        {
            /* target name string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sTargetName = (SChar*)sBuffer;
            sBuffer += sLen;

            /* add dtx branch tx */
            IDE_TEST( addDtxBranchTx( &sXID, sTargetName ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( sLinkerType != 'S', ERR_INVALID_BRANCH_TX_INFO );

            if ( sShardBranchCounter == 0 )
            {
                /* XID 길이 1byte */
                ID_1_BYTE_ASSIGN( &sXidLen, sBuffer );
                sBuffer += 1;

                /* XID */
                idlOS::memcpy( &mXID, sBuffer, sXidLen );
                sBuffer += sXidLen;
            }
            ++sShardBranchCounter;

            /* node type 1byte */
            ID_1_BYTE_ASSIGN( &sCoordinatorType, sBuffer );
            sBuffer += 1;

            /* node name string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sNodeName = (SChar*)sBuffer;
            sBuffer += sLen;

            /* user name string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sUserName = (SChar*)sBuffer;
            sBuffer += sLen;

            /* user password string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sUserPassword = (SChar*)sBuffer;
            sBuffer += sLen;

            /* ip string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sDataServerIP = (SChar*)sBuffer;
            sBuffer += sLen;

            /* port no 2byte */
            ID_2_BYTE_ASSIGN( &sDataPortNo, sBuffer );
            sBuffer += 2;

            /* connect type 2byte */
            ID_2_BYTE_ASSIGN( &sConnectType, sBuffer );
            sBuffer += 2;

            /* add dtx branch shard tx */
            IDE_TEST( addDtxBranchTx( &sXID,
                                      (sdiCoordinatorType)sCoordinatorType,
                                      sNodeName,
                                      sUserName,
                                      sUserPassword,
                                      sDataServerIP,
                                      sDataPortNo,
                                      sConnectType )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_BRANCH_TX_INFO )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktDtxInfo::unserializeAndAddDtxBranchTx] invalid branch tx info" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC  dktDtxInfo::copyString( UChar ** aBuffer,
                                UChar  * aFence,
                                SChar  * aString )
{
    UInt   sLen4 = 0;
    UChar  sLen1 = 0;

    sLen4 = idlOS::strlen( aString ) + 1;   /* null까지 포함한다. */
    IDE_TEST_RAISE( sLen4 > 255, ERR_OVERFLOW );
    IDE_TEST_RAISE( (*aBuffer) + 1 + sLen4 > aFence, ERR_OVERFLOW );

    sLen1 = (UChar)sLen4;
    ID_1_BYTE_ASSIGN( (*aBuffer), &sLen1 );
    (*aBuffer) += 1;

    idlOS::memcpy( (*aBuffer), aString, sLen1 );
    (*aBuffer) += sLen1;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktDtxInfo::copyString] buffer overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC  dktDtxInfo::addDtxBranchTx( dktDtxBranchTxInfo * aDtxBranchTxInfo )
{
    dktDtxBranchTxInfo * sDtxBranchTxInfo = NULL;

    if ( aDtxBranchTxInfo->mLinkerType == 'D' )
    {
        /* 이미 shard로 사용하고 있는 경우 에러 */
        IDE_TEST_RAISE( mLinkerType == DKT_LINKER_TYPE_SHARD,
                        ERR_SHARD_TX_ALREADY_EXIST );
    }
    else
    {
        IDE_DASSERT( aDtxBranchTxInfo->mLinkerType == 'S' );

        /* 이미 dblink로 사용하고 있는 경우 에러 */
        IDE_TEST_RAISE( mLinkerType == DKT_LINKER_TYPE_DBLINK,
                        ERR_DBLINK_TX_ALREADY_EXIST );
    }

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dktDtxBranchTxInfo ),
                                       (void **)&sDtxBranchTxInfo,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DTX_INFO );

    idlOS::memcpy( sDtxBranchTxInfo, aDtxBranchTxInfo, ID_SIZEOF(dktDtxBranchTxInfo) );

    IDU_LIST_INIT_OBJ( &(sDtxBranchTxInfo->mNode), sDtxBranchTxInfo );

    IDU_LIST_ADD_LAST( &mBranchTxInfo, &(sDtxBranchTxInfo->mNode) );

    mBranchTxCount++;

    if ( mLinkerType == DKT_LINKER_TYPE_NONE )
    {
        if ( aDtxBranchTxInfo->mLinkerType == 'D' )
        {
            mLinkerType = DKT_LINKER_TYPE_DBLINK;
        }
        else
        {
            mLinkerType = DKT_LINKER_TYPE_SHARD;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_TX_ALREADY_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_IS_BEING_USED ) );
    }
    IDE_EXCEPTION( ERR_DBLINK_TX_ALREADY_EXIST )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKO_DBLINK_IS_BEING_USED ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DTX_INFO );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sDtxBranchTxInfo != NULL )
    {
        (void)iduMemMgr::free( sDtxBranchTxInfo );
        sDtxBranchTxInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

void dktXid::initXID( ID_XID * aXID )
{
    aXID->formatID     = 0;
    aXID->gtrid_length = DKT_2PC_MAXGTRIDSIZE;
    aXID->bqual_length = DKT_2PC_MAXBQUALSIZE;
    idlOS::memset( aXID->data, 0x00, ID_MAXXIDDATASIZE );
}

void dktXid::copyXID( ID_XID * aDst, ID_XID * aSrc )
{
    SLong   sLength = aSrc->gtrid_length + aSrc->bqual_length;

    if ( sLength <= ID_MAXXIDDATASIZE )
    {
        aDst->formatID     = aSrc->formatID;
        aDst->gtrid_length = aSrc->gtrid_length;
        aDst->bqual_length = aSrc->bqual_length;

        if ( sLength > 0 )
        {
            idlOS::memcpy( aDst->data, aSrc->data, sLength );
        }
        else
        {
            /* Nothing to do. */
        }

        if ( sLength < ID_MAXXIDDATASIZE )
        {
            idlOS::memset( aDst->data + sLength, 0x00, ID_MAXXIDDATASIZE - sLength );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        IDE_DASSERT(0);

        /* 혹시라도 로그가 깨졌거나 잘못된 경우 초기화한다. */
        idlOS::memset( aDst, 0x00, ID_SIZEOF(ID_XID) );
    }
}

idBool dktXid::isEqualXID( ID_XID * aXID1, ID_XID * aXID2 )
{
    idBool  sEqual = ID_FALSE;
    SLong   sLength;

    if ( ( aXID1->formatID     == aXID2->formatID ) &&
         ( aXID1->gtrid_length == aXID2->gtrid_length ) &&
         ( aXID1->bqual_length == aXID2->bqual_length ) )
    {
        sLength = aXID1->gtrid_length + aXID1->bqual_length;

        if ( ( sLength > 0 ) && ( sLength <= ID_MAXXIDDATASIZE ) )
        {
            if ( idlOS::memcmp( aXID1->data, aXID2->data, sLength ) == 0 )
            {
                sEqual = ID_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return sEqual;
}

void dktXid::copyGlobalXID( ID_XID * aDst, ID_XID * aSrc )
{
    SLong   sLength = aSrc->gtrid_length;

    if ( sLength <= ID_MAXXIDDATASIZE )
    {
        aDst->formatID     = aSrc->formatID;
        aDst->gtrid_length = aSrc->gtrid_length;
        aDst->bqual_length = aSrc->bqual_length;

        if ( sLength > 0 )
        {
            idlOS::memcpy( aDst->data, aSrc->data, sLength );
        }
        else
        {
            /* Nothing to do. */
        }

        if ( sLength < ID_MAXXIDDATASIZE )
        {
            idlOS::memset( aDst->data + sLength, 0x00, ID_MAXXIDDATASIZE - sLength );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        IDE_DASSERT(0);

        /* 혹시라도 로그가 깨졌거나 잘못된 경우 초기화한다. */
        idlOS::memset( aDst, 0x00, ID_SIZEOF(ID_XID) );
    }
}

idBool dktXid::isEqualGlobalXID( ID_XID * aXID1, ID_XID * aXID2 )
{
    idBool  sEqual = ID_FALSE;
    SLong   sLength;

    if ( ( aXID1->formatID     == aXID2->formatID ) &&
         ( aXID1->gtrid_length == aXID2->gtrid_length ) &&
         ( aXID1->bqual_length == aXID2->bqual_length ) )
    {
        sLength = aXID1->gtrid_length;

        if ( ( sLength > 0 ) && ( sLength <= ID_MAXXIDDATASIZE ) )
        {
            if ( idlOS::memcmp( aXID1->data, aXID2->data, sLength ) == 0 )
            {
                sEqual = ID_TRUE;
            }
        }
    }

    return sEqual;
}

UChar dktXid::sizeofXID( ID_XID * aXID )
{
    SLong   sLength = aXID->gtrid_length + aXID->bqual_length;

    IDE_DASSERT( sLength <= ID_MAXXIDDATASIZE );

    sLength += ID_SIZEOF(vSLong);  /* formatID */
    sLength += ID_SIZEOF(vSLong);  /* gtrid_length */
    sLength += ID_SIZEOF(vSLong);  /* bqual_length */

    IDE_DASSERT( sLength <= 255 );

    return (UChar)sLength;
}

UInt dktXid::getGlobalTxIDFromXID( ID_XID * aXID )
{
    UInt sGlobalTxID = SM_NULL_TID;

    IDE_DASSERT( aXID != NULL );

    idlOS::memcpy( &sGlobalTxID,
                   (SChar*)(aXID->data) + ( DKT_2PC_MAXGTRIDSIZE - ID_SIZEOF(sGlobalTxID) ),
                   ID_SIZEOF(sGlobalTxID) );

    return sGlobalTxID;
}

UInt dktXid::getLocalTxIDFromXID( ID_XID * aXID )
{
    UInt sLocalTxID = SM_NULL_TID;

    IDE_DASSERT( aXID != NULL );

    idlOS::memcpy( &sLocalTxID,
                   (SChar*)(aXID->data) + DKT_2PC_MAXGTRIDSIZE, 
                   ID_SIZEOF(sLocalTxID) );

    return sLocalTxID;
}
