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
 * $Id: smrUTransQueue.cpp 82426 2018-03-09 05:12:27Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <smDef.h>
#include <smr.h>
#include <smrReq.h>

/*
  Transaction 객체를 iduPriorityQueue에 삽입하여 Transaction의
  mLstUndoLSN으로 Sorting한다. 그리고 remove호출시 가장 큰
  mLstUndoLSN을 가진 Transaction객체를 return해준다.
  이 클래스는 restart recovery의 undo시 Active Transaction중
  가장 큰 mLstUndoLSN에 해당하는 Log부터 Undo하기 위해
  만들어졌다.
*/
smrUTransQueue::smrUTransQueue()
{
}

smrUTransQueue::~smrUTransQueue()
{
}

/***********************************************************************
 * Description : smrUTransQueue를 초기화한다. 
 *
 * aTransCount   - [IN[ Active Transaction 갯수
 **********************************************************************/
IDE_RC smrUTransQueue::initialize(SInt aTransCount)
{
    IDE_DASSERT(aTransCount > 0);
    
    IDE_TEST(mPQueueTransInfo.initialize(IDU_MEM_SM_SMR,
                                         aTransCount,
                                         ID_SIZEOF(smrUndoTransInfo*),
                                         smrUTransQueue::compare)
             != IDE_SUCCESS);

    /* smrUTransQueue_initialize_calloc_ArrUndoTransInfo.tc */
    IDU_FIT_POINT("smrUTransQueue::initialize::calloc::ArrUndoTransInfo");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMR,
                               aTransCount,
                               ID_SIZEOF(smrUndoTransInfo),
                               (void**)&mArrUndoTransInfo)
             != IDE_SUCCESS);

    mCurUndoTransCount = 0;
    mMaxTransCount = aTransCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smrUTransQueue를 해제. 
 *
 **********************************************************************/
IDE_RC smrUTransQueue::destroy()
{
    IDE_TEST(mPQueueTransInfo.destroy() != IDE_SUCCESS);

    IDE_TEST(iduMemMgr::free(mArrUndoTransInfo)
             != IDE_SUCCESS);
    mArrUndoTransInfo = NULL;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Transaction객체를 iduPriorityQueue에 삽입한다. undo전에
 *               UTransQueue를 만들기 위해 사용한다.
 *
 * aTrans  - [IN] smxTrans객체
 **********************************************************************/
IDE_RC smrUTransQueue::insertActiveTrans(void * aTrans)
{
    smLSN           sLSN;
    void          * sQueueData;
    idBool          sOverflow           = ID_FALSE;
    smrCompRes    * sCompRes;
    
    IDE_DASSERT(aTrans != NULL);
    
    IDE_ASSERT(mCurUndoTransCount < mMaxTransCount);
    
    mArrUndoTransInfo[mCurUndoTransCount].mTrans = aTrans;
    mArrUndoTransInfo[mCurUndoTransCount].mLogFilePtr = NULL;
    
    sLSN = smLayerCallback::getLstUndoNxtLSN( aTrans );
    smLayerCallback::setCurUndoNxtLSN( aTrans, &sLSN );

    // 트랜잭션의 로그 압축/압축해제 리소스를 가져온다 
    IDE_TEST( smLayerCallback::getTransCompRes( aTrans, &sCompRes )
              != IDE_SUCCESS );
    
    IDE_TEST(smrLogMgr::readLog(
                            & sCompRes->mDecompBufferHandle, 
                            &sLSN,
                            ID_TRUE, /* close Log File When aLogFile doesn't include aLSN */
                            &(mArrUndoTransInfo[mCurUndoTransCount].mLogFilePtr),
                            &(mArrUndoTransInfo[mCurUndoTransCount].mLogHead),
                            &(mArrUndoTransInfo[mCurUndoTransCount].mLogPtr),
                            &(mArrUndoTransInfo[mCurUndoTransCount].mIsValid),
                            &(mArrUndoTransInfo[mCurUndoTransCount].mLogSizeAtDisk))
             != IDE_SUCCESS);
    
    IDE_ASSERT(mArrUndoTransInfo[mCurUndoTransCount].mIsValid
               == ID_TRUE);

    //PQueue에 주소값을 저장하기 위해서는 이런식으로 해야 한다.
    sQueueData = (void*)&(mArrUndoTransInfo[mCurUndoTransCount]);
    mPQueueTransInfo.enqueue(&sQueueData, &sOverflow);
    IDE_ASSERT( sOverflow == ID_FALSE);
    
    mCurUndoTransCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Transaction객체를 iduPriorityQueue에 삽입한다.
 *
 * aUTransInfo  - [IN] smrUndoTransInfo 
 **********************************************************************/
IDE_RC smrUTransQueue::insert(smrUndoTransInfo    * aUTransInfo)
{
    smLSN           sLSN;
    idBool          sOverflow           = ID_FALSE;
    smrCompRes    * sCompRes;
    
    IDE_ASSERT(mCurUndoTransCount < mMaxTransCount);
    mCurUndoTransCount++;
    
    sLSN = smLayerCallback::getCurUndoNxtLSN( aUTransInfo->mTrans );

    // 트랜잭션의 로그 압축/압축해제 리소스를 가져온다 
    IDE_TEST( smLayerCallback::getTransCompRes( aUTransInfo->mTrans, &sCompRes )
              != IDE_SUCCESS );
    
    IDE_TEST(smrLogMgr::readLog(
                 & sCompRes->mDecompBufferHandle, 
                 &sLSN,
                 ID_TRUE, /* close Log File When aLogFile doesn't include aLSN */
                 &(aUTransInfo->mLogFilePtr),
                 &(aUTransInfo->mLogHead),
                 &(aUTransInfo->mLogPtr),
                 &(aUTransInfo->mIsValid),
                 &(aUTransInfo->mLogSizeAtDisk) )
             != IDE_SUCCESS);
    
    IDE_ASSERT(aUTransInfo->mIsValid == ID_TRUE);

    mPQueueTransInfo.enqueue((void*)&aUTransInfo, &sOverflow);
    IDE_ASSERT(sOverflow == ID_FALSE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Transaction객체를 iduPriorityQueue에서 가장 큰 mLstUndoNxtLSN
 *               을 가진 Transaction을 꺼낸다.
 *
 **********************************************************************/
smrUndoTransInfo* smrUTransQueue::remove()
{
    smrUndoTransInfo *sTransInfo;
    idBool            sUnderflow = ID_FALSE;
    
    if(mCurUndoTransCount != 0)
    {
        mCurUndoTransCount--;

        mPQueueTransInfo.dequeue( (void*)&sTransInfo, &sUnderflow);
        
        if( sUnderflow == ID_FALSE)
        {
            return sTransInfo;
        }
    }
    
    return NULL;
}

/***********************************************************************
 * Description : iduPriorityQueue에서 삽입된 객체들에 대해서 compare할때
 * 사용하는 Callback function이다. 여기서는 Transaction객체들의
 * UndoNxtLSN으로  비교하고 있다.
 *
 **********************************************************************/
SInt smrUTransQueue::compare(const void *arg1,const void *arg2)
{
    smrUndoTransInfo* sUTransInfo1;
    smrUndoTransInfo* sUTransInfo2;
    smLSN sLSN1;
    smLSN sLSN2;

    IDE_DASSERT(arg1 != NULL);
    IDE_DASSERT(arg2 != NULL);

    sUTransInfo1 = *((smrUndoTransInfo**)arg1);
    sUTransInfo2 = *((smrUndoTransInfo**)arg2);

    sLSN1 = smrLogHeadI::getLSN( &sUTransInfo1->mLogHead ); 
    sLSN2 = smrLogHeadI::getLSN( &sUTransInfo2->mLogHead ); 

    if ( smrCompareLSN::isGT(&sLSN1, &sLSN2 ) == ID_TRUE )
    {
        return -1;
    }
    else
    {
        if ( smrCompareLSN::isLT(&sLSN1, &sLSN2 ) == ID_TRUE )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}
