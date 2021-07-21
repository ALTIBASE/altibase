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
 * $Id:$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer 
 * SBCB  관리 및 기타 상태 정보를 관리한다 
 **********************************************************************/

#include <smDef.h>
#include <sdbDef.h>
#include <smErrorCode.h>
#include <sdsBCB.h>

/***********************************************************************
 * Description :
 *  aSBCBID          - [IN] SBCB 식별자
 ***********************************************************************/
IDE_RC sdsBCB::initialize( UInt aSBCBID )
{
    SChar sMutexName[128];
    SInt  sState    = 0;
    
    /* BUG-22041 */
    mSpaceID = 0;
    mPageID  = 0;
    mSBCBID  = aSBCBID;
    /* Buffer Pool에 있는 BCB 정보*/
    mBCB     = NULL;

    idlOS::snprintf( sMutexName,
                     ID_SIZEOF(sMutexName),
                     "SECONDARY_BCB_MUTEX_%"ID_UINT32_FMT,
                     aSBCBID );
    
    /* BUG-28331 */ 
    IDE_TEST( mBCBMutex.initialize( 
                          sMutexName,
                          IDU_MUTEX_KIND_NATIVE,
                          IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BCB_MUTEX )
              != IDE_SUCCESS );
    sState = 1;
    /*  */
    idlOS::snprintf( sMutexName,
                     ID_SIZEOF(sMutexName),
                     "SECONDARY_BCB_READIO_%"ID_UINT32_FMT,
                     aSBCBID );
    
    IDE_TEST( mReadIOMutex.initialize( 
                        sMutexName,
                        IDU_MUTEX_KIND_NATIVE,
                        IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_READIO_MUTEX )
              != IDE_SUCCESS );
    sState = 2;

    setFree();
    
    SMU_LIST_INIT_NODE( &mCPListItem );
    mCPListItem.mData = this;

    SMU_LIST_INIT_NODE( &mHashItem );
    mHashItem.mData   = this;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 2:
            IDE_ASSERT( mReadIOMutex.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mBCBMutex.destroy() == IDE_SUCCESS );
        default:
            break; 
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  sdsBCB 소멸자.
 ***********************************************************************/
IDE_RC sdsBCB::destroy()
{
    IDE_ASSERT( mBCBMutex.destroy() == IDE_SUCCESS );
    IDE_ASSERT( mReadIOMutex.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 *  sdsBCB를  FREE 상태로 변경
 ***********************************************************************/
IDE_RC sdsBCB::setFree()
{
    if( mBCB != NULL )
    {   
        if( ( mState == SDS_SBCB_DIRTY ) ||
            ( mState == SDS_SBCB_CLEAN ) )
        {
            mBCB->mSBCB = NULL;
        } 
        else 
        {
            /* OLD상태에서 호출되면 mBCB->mSBCB는 NULL/NextBCB 일수있음 */
            IDE_ASSERT( mState == SDS_SBCB_OLD );
        }
    
        mBCB = NULL;
    }

    SDB_INIT_CP_LIST( (sdBCB*)this ); 
    SM_LSN_MAX( mRecoveryLSN );
    mState   = SDS_SBCB_FREE;
    mSpaceID = 0;
    mPageID  = 0;
    SM_LSN_INIT( mPageLSN );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 *  aBCB    - [IN]  BCB
 ***********************************************************************/
IDE_RC sdsBCB::dump( sdsBCB * aSBCB )
{
    IDE_ASSERT( aSBCB != NULL );

    ideLog::log ( IDE_SM_0,
            "SECONDARY BCB Info..\n"
            "mID <%"ID_UINT32_FMT">\n"
            "mState <%"ID_UINT32_FMT">\n"
            "mSpaceID <%"ID_UINT32_FMT">\n"
            "mPageID <%"ID_UINT32_FMT">\n"
            "mRecoveryLSN <%"ID_UINT32_FMT">,<%"ID_UINT32_FMT">\n",
            aSBCB->mSBCBID,
            aSBCB->mState,
            aSBCB->mSpaceID,
            aSBCB->mPageID,
            aSBCB->mRecoveryLSN.mFileNo,
            aSBCB->mRecoveryLSN.mOffset );

    return IDE_SUCCESS;
}
