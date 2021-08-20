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
 * $Id: rpcManagerMisc.cpp 91413 2021-08-03 05:01:56Z lswhh $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduCheckLicense.h>

#include <smi.h>
#include <smiMisc.h>

#include <rpDef.h>
#include <rpcManager.h>

/**********************************************************************
 *
 * BUG-6093 DB File Signature 생성. smuMakeUniqueDBString()를 복제
 *
 * aUnique - [OUT] IDU_SYSTEM_INFO_LENGTH + 1 크기의 버퍼
 *
 **********************************************************************/
void rpcMakeUniqueDBString(SChar *aUnique)
{
    SChar          sHostID[IDU_SYSTEM_INFO_LENGTH + 1];
    PDL_Time_Value sTime;
    UInt           sPort;

    // Host ID
    idlOS::memset(sHostID, 0x00, IDU_SYSTEM_INFO_LENGTH + 1);
    iduCheckLicense::getHostUniqueString(sHostID, ID_SIZEOF(sHostID));

    // Time
    sTime = idlOS::gettimeofday();

    sPort = (UInt)RPU_REPLICATION_PORT_NO;

    idlOS::snprintf(aUnique, IDU_SYSTEM_INFO_LENGTH + 1,
                    "%s"
                    "-%08"ID_XINT32_FMT /* sec  */
                    ":%08"ID_XINT32_FMT /* usec */
                    "-%08"ID_XINT32_FMT,
                    sHostID,
                    (UInt)sTime.sec(),
                    (UInt)sTime.usec(),
                    sPort);
}

/**********************************************************************
 *
 * BUG-31374 Implicit Savepoint 이름의 배열을 생성한다.
 *
 **********************************************************************/
void rpcManager::makeImplSPNameArr()
{
    UInt i;

    for(i = 0; i < SMI_STATEMENT_DEPTH_MAX; i++)
    {
        idlOS::memset(mImplSPNameArr[i],
                      0x00,
                      RP_SAVEPOINT_NAME_LEN + 1);

        idlOS::snprintf(mImplSPNameArr[i],
                        RP_SAVEPOINT_NAME_LEN + 1,
                        SMR_IMPLICIT_SVP_NAME"%"ID_UINT32_FMT,
                        i + 1);
    }
}

/*
 *
 */
IDE_RC rpcManager::addLastSNEntry( iduMemPool * aSNPool,
                                   smSN         aSN,
                                   iduList    * aSNList )
{
    rpxSNEntry * sSNEntry = NULL;

    IDU_FIT_POINT( "rpcManager::addLastSNEntry::alloc::SNEntry" );
    IDE_TEST( aSNPool->alloc( (void **)&sSNEntry ) != IDE_SUCCESS );

    sSNEntry->mSN = aSN;

    IDU_LIST_INIT_OBJ( &(sSNEntry->mNode), sSNEntry );
    IDU_LIST_ADD_LAST( aSNList, &(sSNEntry->mNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
rpxSNEntry * rpcManager::searchSNEntry( iduList * aSNList, smSN aSN )
{
    iduListNode * sNode    = NULL;
    rpxSNEntry  * sSNEntry = NULL;
    rpxSNEntry  * sReturn  = NULL;

    IDU_LIST_ITERATE( aSNList, sNode )
    {
        sSNEntry = (rpxSNEntry *)sNode->mObj;

        if ( sSNEntry->mSN == aSN )
        {
            sReturn = sSNEntry;
            break;
        }
    }

    return sReturn;
}

/*
 *
 */
void rpcManager::removeSNEntry( iduMemPool * aSNPool,
                                   rpxSNEntry * aSNEntry )
{
    IDU_LIST_REMOVE( &aSNEntry->mNode );
    (void)aSNPool->memfree( aSNEntry );

    return;
}

/*
 *
 */
void rpcManager::printTcpInfo( cmiProtocolContext   * aProtocolContext,
                               SChar * aRepName )
{
    cmiLink * sLink = NULL;

    SChar sPeerIP[RP_IP_ADDR_LEN] = {0,};
    SInt  sPeerPort = 0;
    SChar sPeerPortStr[RP_PORT_LEN] = {0,};

    IDE_TEST(cmiGetLinkForProtocolContext(aProtocolContext, &sLink) != IDE_SUCCESS);
    IDE_TEST(rpnComm::isConnected( sLink ) != ID_TRUE);

    IDE_TEST(cmiGetLinkInfo(sLink,
                            sPeerIP,
                            RP_IP_ADDR_LEN,
                            CMI_LINK_INFO_REMOTE_IP_ADDRESS) != IDE_SUCCESS);
    IDE_TEST(cmiGetLinkInfo(sLink,
                            sPeerPortStr,
                            RP_PORT_LEN,
                            CMI_LINK_INFO_REMOTE_PORT) != IDE_SUCCESS);
    sPeerPort = idlOS::atoi(sPeerPortStr);

    ideLog::log( IDE_RP_0, RP_TRC_R_PEER_IP_PORT_NAME, sPeerIP, sPeerPort, aRepName );

    return;
    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, RP_TRC_R_PEER_IP_PORT_NAME, "Disconnected Link", 0, aRepName );
    return;
}

