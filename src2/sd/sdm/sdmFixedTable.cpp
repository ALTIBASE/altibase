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

#include <ideErrorMgr.h>
#include <sdm.h>
#include <sdmFixedTable.h>
#include <sdl.h>
#include <cmnDef.h>

iduFixedTableColDesc gShardConnectionInfoColDesc[]=
{
    {
        (SChar*)"NODE_ID",
        offsetof( sdmConnectInfo4PV, mNodeId ),
        IDU_FT_SIZEOF( sdmConnectInfo4PV, mNodeId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"NODE_NAME",
        offsetof( sdmConnectInfo4PV, mNodeName ),
        SDI_NODE_NAME_MAX_SIZE,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"COMM_NAME",
        offsetof( sdmConnectInfo4PV, mCommName ),
        IDL_IP_ADDR_MAX_LEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"TOUCH_COUNT",
        offsetof( sdmConnectInfo4PV, mTouchCount ),
        IDU_FT_SIZEOF(sdmConnectInfo4PV, mTouchCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"LINK_FAILURE",
        offsetof( sdmConnectInfo4PV, mLinkFailure ),
        IDU_FT_SIZEOF(sdmConnectInfo4PV, mLinkFailure),
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

iduFixedTableDesc  gShardConnectionInfoTableDesc =
{
    (SChar *)"X$SHARD_CONNECTION_INFO",
    sdmFixedTable::buildRecordForShardConnectionInfo,
    gShardConnectionInfoColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC sdmFixedTable::buildRecordForShardConnectionInfo(
    idvSQL              * aStatistics,
    void                * aHeader,
    void                * /*aDumpObj*/,
    iduFixedTableMemory * aMemory )
{
    sdmConnectInfo4PV   sConnectInfo4PV;
    idvSQL            * sStatistics;
    idvSession        * sSess;
    void              * sMmSess;
    qcSession         * sQcSess;
    sdiClientInfo     * sClientInfo;
    sdiConnectInfo    * sConnectInfo;
    UShort              i = 0;

    sStatistics = aStatistics;
    IDE_TEST_CONT( sStatistics == NULL, NORMAL_EXIT );

    sSess = sStatistics->mSess;
    IDE_TEST_CONT( sSess == NULL, NORMAL_EXIT );

    sMmSess = sSess->mSession;
    IDE_TEST_CONT( sMmSess == NULL, NORMAL_EXIT );

    sQcSess = qci::mSessionCallback.mGetQcSession( sMmSess );
    IDE_TEST_CONT( sQcSess == NULL, NORMAL_EXIT );

    sClientInfo = sQcSess->mQPSpecific.mClientInfo;
    IDE_TEST_CONT( sClientInfo == NULL, NORMAL_EXIT );

    sConnectInfo = sClientInfo->mConnectInfo;

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        if ( sConnectInfo->mDbc != NULL )
        {
            idlOS::memset( &sConnectInfo4PV, 0, ID_SIZEOF(sdmConnectInfo4PV) );

            // node ID
            sConnectInfo4PV.mNodeId = sConnectInfo->mNodeId;

            // node name
            idlOS::memcpy( sConnectInfo4PV.mNodeName,
                           sConnectInfo->mNodeName,
                           SDI_NODE_NAME_MAX_SIZE + 1 );

            // comm namae
            (void) sdl::getLinkInfo( sConnectInfo,
                                     sConnectInfo4PV.mCommName,
                                     IDL_IP_ADDR_MAX_LEN,
                                     CMN_LINK_INFO_ALL );

            // touch count
            sConnectInfo4PV.mTouchCount = sConnectInfo->mTouchCount;

            // link failure
            sConnectInfo4PV.mLinkFailure = sConnectInfo->mLinkFailure;

            // link failure가 아닌경우 현재 시점에서 한번 더 검사한다.
            if ( sConnectInfo4PV.mLinkFailure == ID_FALSE )
            {
                if ( sdl::checkDbcAlive( sConnectInfo->mDbc ) == ID_FALSE )
                {
                    sConnectInfo4PV.mLinkFailure = ID_TRUE;
                }
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                  aMemory,
                                                  (void *)&sConnectInfo4PV )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------------
 *  Fixed Table Define for Shard Meta Node Info
 * -------------------------------------------------------------------------- */
iduFixedTableColDesc gShardMetaNodeInfoColDesc[] =
{
    {
        (SChar *)"SMN",
        offsetof( sdmMetaNodeInfo4PV, mSMN ),
        IDU_FT_SIZEOF( sdmMetaNodeInfo4PV, mSMN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
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

iduFixedTableDesc gShardMetaNodeInfoTableDesc =
{
    (SChar *)"X$SHARD_META_NODE_INFO",
    sdmFixedTable::buildRecordForShardMetaNodeInfo,
    gShardMetaNodeInfoColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC sdmFixedTable::buildRecordForShardMetaNodeInfo( idvSQL              * /*aStatistics*/,
                                                       void                * aHeader,
                                                       void                * /* aDumpObj */,
                                                       iduFixedTableMemory * aMemory )
{
    sdmMetaNodeInfo4PV   sMetaNodeInfo = { ID_ULONG(0) };

    sMetaNodeInfo.mSMN = sdi::getSMNForMetaNode();

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sMetaNodeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------------
 *  Fixed Table Define for Shard Data Node Info
 * -------------------------------------------------------------------------- */
iduFixedTableColDesc gShardDataNodeInfoColDesc[] =
{
    {
        (SChar *)"SMN",
        offsetof( sdmDataNodeInfo4PV, mSMN ),
        IDU_FT_SIZEOF( sdmDataNodeInfo4PV, mSMN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SHARD_STATUS",
        offsetof( sdmDataNodeInfo4PV, mShardStatus ),
        IDU_FT_SIZEOF(sdmDataNodeInfo4PV, mShardStatus),
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

iduFixedTableDesc gShardDataNodeInfoTableDesc =
{
    (SChar *)"X$SHARD_DATA_NODE_INFO",
    sdmFixedTable::buildRecordForShardDataNodeInfo,
    gShardDataNodeInfoColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC sdmFixedTable::buildRecordForShardDataNodeInfo( idvSQL              * /*aStatistics*/,
                                                       void                * aHeader,
                                                       void                * /* aDumpObj */,
                                                       iduFixedTableMemory * aMemory )
{
    sdmDataNodeInfo4PV   sDataNodeInfo = { ID_ULONG(0), 0 };

    sDataNodeInfo.mSMN = sdi::getSMNForDataNode();
    
    sDataNodeInfo.mShardStatus = sdi::getShardStatus();

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sDataNodeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//BUG-48748
iduFixedTableColDesc gZookeeperInfoColDesc[]=
{
    {
        (SChar*)"ZOOKEEPER_PATH",
        offsetof( sdiZookeeperInfo4PV, mPath ),
        SDI_ZKC_PATH_LENGTH,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar*)"ZOOKEEPER_DATA",
        offsetof( sdiZookeeperInfo4PV, mData ),
        SDI_NODE_NAME_MAX_SIZE,
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

iduFixedTableDesc  gZookeeperInfoTableDesc =
{
    (SChar *)"X$ZOOKEEPER_DATA_INFO",
    sdmFixedTable::buildRecordForZookeeperInfo,
    gZookeeperInfoColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC sdmFixedTable::buildRecordForZookeeperInfo( idvSQL              * /*aStatistics*/,
                                                   void                * aHeader,
                                                   void                * /* aDumpObj */,
                                                   iduFixedTableMemory * aMemory )
{
    iduList               * sList = NULL;
    iduListNode           * sIterator = NULL;
    sdiZookeeperInfo4PV     sInfo;
    sdiZookeeperInfo4PV   * sZookeeperInfo = NULL;

    if( sdiZookeeper::isConnect() == ID_TRUE )
    {
        IDE_TEST( sdiZookeeper::getZookeeperInfo4PV( &sList ) != IDE_SUCCESS ); 

        IDU_LIST_ITERATE( sList, sIterator )
        {
            sZookeeperInfo = (sdiZookeeperInfo4PV*)sIterator->mObj;

            sInfo = *sZookeeperInfo;

            IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                  aMemory,
                                                  (void *)&sInfo )
                      != IDE_SUCCESS );
        }

        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_PRINT );
    }
    else
    {
        /* zookeeper와 연결이 되지 않았다면 출력할 데이터가 없다. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

