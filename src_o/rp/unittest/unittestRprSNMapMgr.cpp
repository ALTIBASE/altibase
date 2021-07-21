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
 
#include <rprSNMapMgr.h>
#include <actTest.h>

/* unittest
 * rprSNMapMgr::insertEntry
 * rprSNMapMgr::deleteTxByReplicatedCommitSN
 * rprSNMapMgr::getMinReplicatedSN
 */
acp_sint32_t main( void )
{
    rprSNMapMgr     sSNMapMgr;
    rpdRecoveryInfo sInfo;
    acp_uint32_t    sCount;
    idBool          sIsExist;

    ACT_TEST_BEGIN();

    iduMutexMgr::initializeStatic( IDU_CLIENT_TYPE );

    sSNMapMgr.initialize( "REP_NAME" );

    for ( sCount = 0; sCount < 4; sCount ++ )
    {
        sInfo.mMasterBeginSN      = sCount + 10;
        sInfo.mMasterCommitSN     = sCount + 20;
        sInfo.mReplicatedBeginSN  = sCount + 30;
        sInfo.mReplicatedCommitSN = sCount + 40;

        ACT_CHECK( sSNMapMgr.insertEntry( &sInfo ) == ACP_RC_SUCCESS );
    }
    /* getMinReplicatedSN function returns mReplicatedBeginSN of first node */
    ACT_CHECK( 30 == sSNMapMgr.getMinReplicatedSN() );

    /* deleteTxByReplicatedCommitSN function removes nodes that have the specified commitSN */
    sSNMapMgr.deleteTxByReplicatedCommitSN( 40, &sIsExist );
    ACT_CHECK( 31 == sSNMapMgr.getMinReplicatedSN() );

    sSNMapMgr.destroy();

    ACT_TEST_END();

    return 0;
}
