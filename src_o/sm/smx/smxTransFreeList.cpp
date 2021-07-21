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
 * $Id: smxTransFreeList.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <iduMutex.h>
#include <smiDef.h>
#include <smxTrans.h>
#include <smxTransFreeList.h>
#include <smxTransMgr.h>


IDE_RC smxTransFreeList::initialize(smxTrans *aArrTrans,
                                    UInt      aSeqNumber,
                                    SInt      aFstItem,
                                    SInt      aLstItem)
{
    SInt  i;
    SChar sBuffer[IDU_MUTEX_NAME_LEN];

    idlOS::snprintf(sBuffer,
                    IDU_MUTEX_NAME_LEN,
                    "TRANS_FREELIST_MUTEX_%"ID_UINT32_FMT,
                    aSeqNumber);

    IDE_TEST(mMutex.initialize(sBuffer,
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    mFreeTransCnt = aLstItem - aFstItem + 1;
    mCurFreeTransCnt = mFreeTransCnt;

    for(i = aFstItem; i < aLstItem; i++)
    {
        IDE_ASSERT(aArrTrans[i].mStatus == SMX_TX_END);

        aArrTrans[i].mNxtFreeTrans = aArrTrans + i + 1;
        aArrTrans[i].mTransFreeList = this;
    }

    aArrTrans[i].mNxtFreeTrans = NULL;
    aArrTrans[i].mTransFreeList = this;

    mFstFreeTrans = aArrTrans + aFstItem;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smxTransFreeList::destroy()
{

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smxTransFreeList::allocTrans(smxTrans **aTrans)
{

    UInt   sState = 0;

    IDE_ASSERT(aTrans != NULL);

    *aTrans = NULL;

    if(mCurFreeTransCnt != 0)
    {
        IDE_TEST(lock() != IDE_SUCCESS);
        sState = 1;

        if(mFstFreeTrans != NULL)
        {
            /* BUG-19245: Transaction이 두번 Free되는 것을 Detect하기 위해 추가됨 */
            IDE_ASSERT( mFstFreeTrans->mIsFree == ID_TRUE );

            *aTrans = mFstFreeTrans;
            mFstFreeTrans = mFstFreeTrans->mNxtFreeTrans;
            (*aTrans)->mNxtFreeTrans = NULL;

            (*aTrans)->mIsFree = ID_FALSE;

            mCurFreeTransCnt--;
        }

        sState = 0;
        IDE_TEST(unlock() != IDE_SUCCESS);

        if( *aTrans != NULL )
        {
            if( (*aTrans)->mStatus != SMX_TX_END )
            {
                /* BUG-43595 새로 alloc한 transaction 객체의 state가
                 * begin인 경우가 있습니다. 로 인한 디버깅 정보 출력 추가*/
                ideLog::log(IDE_ERR_0,"Alloc invalid transaction.\n");
                (*aTrans)->dumpTransInfo();
                ideLog::logCallStack(IDE_ERR_0);
                IDE_DASSERT(0);

                *aTrans = NULL;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)unlock();
    }

    return IDE_FAILURE;

}

IDE_RC smxTransFreeList::freeTrans(smxTrans *aTrans)
{
    /* BUG-19245: Transaction이 두번 Free되는 것을 Detect하기 위해 추가됨 */
    IDE_ASSERT( aTrans->mIsFree == ID_FALSE );

    IDE_TEST(lock() != IDE_SUCCESS);

    aTrans->mIsFree       = ID_TRUE;
    // trans free list의 선두로 이동함
    aTrans->mNxtFreeTrans = mFstFreeTrans;
    mFstFreeTrans         = aTrans;
    mCurFreeTransCnt++;

    IDE_TEST(unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
#if 0
void smxTransFreeList::dump()
{

    smxTrans *sCurTrans;
    SInt      sFreeTransCnt = 0;

    sCurTrans = mFstFreeTrans;

    idlOS::fprintf(stderr, "\nBegin Transaction Free List Dump\n");

    while(sCurTrans != NULL)
    {
        idlOS::fprintf(stderr, "<%-5"ID_UINT32_FMT", %"ID_UINT32_FMT", %"ID_UINT32_FMT">  ",
               sCurTrans->mSlotN,
               sCurTrans->mStatus,
               sCurTrans->mCommitState);
        sCurTrans = sCurTrans->mNxtFreeTrans;
        sFreeTransCnt ++;
        if ( (sFreeTransCnt % 5 ) == 0 )
        {
            idlOS::fprintf(stderr,"\n");
        }
    }

    idlOS::fprintf(stderr, "\nTotal:%"ID_INT32_FMT"  End Transaction Free List Dump\n",
                   sFreeTransCnt);

}
#endif

IDE_RC smxTransFreeList::rebuild(UInt aSeqNumber,
                                 SInt aFstItem,
                                 SInt aLstItem)
{
    SInt       i;
    smxTrans  *sCurTrans;
    smxTrans  *sPrevFreeTrans = NULL;
    SChar      sBuffer[IDU_MUTEX_NAME_LEN];

    idlOS::snprintf(sBuffer,
                    IDU_MUTEX_NAME_LEN,
                    "TRANS_FREELIST_MUTEX_%"ID_UINT32_FMT,
                    aSeqNumber);

    IDE_TEST(mMutex.initialize(sBuffer,
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    mFreeTransCnt = 0;
    mFstFreeTrans = NULL;

    for(i = aFstItem; i <= aLstItem; i++)
    {
        sCurTrans = smxTransMgr::getTransBySID(i);

        IDE_ASSERT( (sCurTrans->mStatus == SMX_TX_END) ||
                    (sCurTrans->isPrepared() == ID_TRUE) );

        sCurTrans->mTransFreeList = this;
        sCurTrans->mNxtFreeTrans = NULL;

        if (sCurTrans->isPrepared() == ID_TRUE)
        {
            /* BUG-19245: Transaction이 두번 Free되는 것을 Detect하기 위해 추가됨 */
            sCurTrans->mIsFree = ID_FALSE;
        }
        else
        {
            mFreeTransCnt++;
            if( mFstFreeTrans == NULL )
            {
                mFstFreeTrans = sCurTrans;
            }

            if( sPrevFreeTrans != NULL )
            {
                sPrevFreeTrans->mNxtFreeTrans = sCurTrans;
            }
            sPrevFreeTrans = sCurTrans;

            sCurTrans->mIsFree = ID_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Free List에 존재하는 Transactino이 Free인지를 검증한다.
 ***********************************************************************/
void smxTransFreeList::validateTransFreeList()
{
    smxTrans *sCurTrans;

    sCurTrans = mFstFreeTrans;

    while( sCurTrans != NULL )
    {
        IDE_ASSERT( sCurTrans->mIsFree == ID_TRUE );
        sCurTrans = sCurTrans->mNxtFreeTrans;
    }
}
