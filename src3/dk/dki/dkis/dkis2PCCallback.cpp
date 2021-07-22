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

#include <ide.h>
#include <idu.h>

#include <smi.h>
#include <smiMisc.h>

#include <dkErrorCode.h>

#include <dktNotifier.h>
#include <dktGlobalTxMgr.h>
/*
 *
 */
static smLSN dkisGetMinLSNFunc( void )
{
    return dktGlobalTxMgr::getDtxMinLSN();
}

static IDE_RC dkisManageDtxInfoFunc( ID_XID * aXID,
                                     UInt     aLocalTxId,
                                     UInt     aGlobalTxId,
                                     UInt     aBranchTxSize,
                                     UChar  * aBranchTxInfo,
                                     smLSN  * aPrepareLSN,
                                     smSCN  * aGlobalCommitSCN,
                                     UChar    aType )
{
    dktNotifier * sNotifier = dktGlobalTxMgr::getNotifier();

    IDE_TEST( sNotifier->manageDtxInfoListByLog( aXID,
                                                 aLocalTxId,
                                                 aGlobalTxId,
                                                 aBranchTxSize,
                                                 aBranchTxInfo,
                                                 aPrepareLSN,
                                                 aGlobalCommitSCN,
                                                 aType )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dkis2PCCallback( void )
{
    return smiSet2PCCallbackFunction( dkisGetMinLSNFunc,
                                      dkisManageDtxInfoFunc );
}

IDE_RC dkis2PCNullCallback( void )
{
    return smiSet2PCCallbackFunction( NULL,
                                      NULL );
}
