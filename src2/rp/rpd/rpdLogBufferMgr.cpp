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
 * $Id: rpdLogBufferMgr.cpp 19967 2007-01-18 03:09:58Z lswhh $
 **********************************************************************/

#include <rpxSender.h>
#include <rpdLogBufferMgr.h>

IDE_RC rpdLogBufferMgr::initialize(UInt aSize, UInt aMaxSenderCnt)
{
    UInt      sIdx;
    smiLogHdr sLogHead;

    /*Buffer Info*/
    mWritableFlag = RP_OFF;
    mBufSize      = aSize;
    mEndPtr       = mBufSize - 1;
    mWrotePtr     = mEndPtr;
    mWritePtr     = getWritePtr(mWrotePtr);
    /*아무것도 쓰여있지 않다면 mWrotePtr과 mMinReadPtr이 같음*/
    mMinReadPtr   = mWrotePtr;

    mMinSN     = SM_SN_NULL;
    mMaxSN     = SM_SN_NULL;

    mMaxSenderCnt    = aMaxSenderCnt;
    mAccessInfoCnt   = 0;
    mAccessInfoList  = NULL;
    mBasePtr         = NULL;

    IDE_TEST_RAISE(mBufInfoMutex.initialize((SChar *)"RP_LOGBUFER_BUFINFO_MUTEX",
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    IDU_FIT_POINT_RAISE( "rpdLogBufferMgr::initialize::calloc::AccessInfoList",
                          ERR_MEMORY_ALLOC_ACCESS_INFO_LIST );
    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPD,
                                     mMaxSenderCnt,
                                     ID_SIZEOF(AccessingInfo),
                                     (void**)&mAccessInfoList,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_ACCESS_INFO_LIST);

    for(sIdx = 0; sIdx < mMaxSenderCnt; sIdx++)
    {
        mAccessInfoList[sIdx].mReadPtr = RP_BUF_NULL_PTR;
        mAccessInfoList[sIdx].mNextPtr = RP_BUF_NULL_PTR;
    }

    IDU_FIT_POINT_RAISE( "rpdLogBufferMgr::initialize::malloc::Buf",
                          ERR_BUF_ALLOC );
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPD,
                                     mBufSize,
                                     (void **)&mBasePtr,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_BUF_ALLOC);

    smiLogRec::initializeDummyLog(&mBufferEndLog);
    smiLogRec::getLogHdr(&mBufferEndLog, &sLogHead);
    mBufferEndLogSize = smiLogRec::getLogSizeFromLogHdr(&sLogHead);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MUTEX_INIT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrMutexInit));
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Mutex initialization error");
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_ACCESS_INFO_LIST);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpdLogBufferMgr::initialize",
                                "mAccessInfoList"));
    }
    IDE_EXCEPTION(ERR_BUF_ALLOC);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_LOGBUFFER_ALLOC));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);
    IDE_PUSH();

    (void)mBufInfoMutex.destroy();

    if(mAccessInfoList != NULL)
    {
        (void)iduMemMgr::free(mAccessInfoList);
        mAccessInfoList = NULL;
    }
    if(mBasePtr != NULL)
    {
        (void)iduMemMgr::free(mBasePtr);
        mBasePtr = NULL;
    }

    IDE_POP();
    return IDE_FAILURE;
}

void rpdLogBufferMgr::destroy()
{
    if(mBufInfoMutex.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mAccessInfoList != NULL)
    {
        (void)iduMemMgr::free(mAccessInfoList);
        mAccessInfoList = NULL;
    }

    if(mBasePtr != NULL)
    {
        (void)iduMemMgr::free(mBasePtr);
        mBasePtr = NULL;
    }

    return;
}

void rpdLogBufferMgr::finalize()
{
}

IDE_RC rpdLogBufferMgr::copyToRPLogBuf(idvSQL * aStatistics,
                                       UInt     aSize,
                                       SChar*   aLogPtr,
                                       smLSN    aLSN)
{
    smSN        sTmpSN    = SM_SN_NULL;
    SChar*      sCopyStartPtr = 0;
    smiLogRec   sLog;
    smiLogHdr   sLogHead;
    UInt        sWrotePtr = ID_UINT_MAX;
    flagSwitch  sWritableFlag = RP_OFF;
    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER);

    if(aStatistics != NULL)
    {
        RP_OPTIMIZE_TIME_BEGIN(aStatistics, IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER)
    }

   /* Service Thread가 복사할 공간이 부족한 경우에 mWritableFlag를
    * RP_OFF로 Set하며 이 때에는 복사를 할 수 없다.
    * 그렇지 않고 ON인 경우 복사를 시도한다
    * mWritableFlag는 Service Thread만이 갱신하기 때문에
    * Lock을 잡지않고 확인할 수 있다.
    * (Service Thread들 간의 동기화는 Log File Lock에서 지켜진다)*/
    if(mWritableFlag == RP_OFF)
    {
        /*이 경우 Accessing Info List Count만 확인하여 다시 ON으로 변경 가능한지 확인만 한다.*/
        IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
        if(mAccessInfoCnt == 0)
        {
            mMinSN      = SM_SN_NULL;
            mWrotePtr   = mEndPtr;
            mMinReadPtr = mWrotePtr;
            IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
            mWritePtr   = getWritePtr(mWrotePtr);
            sWritableFlag = RP_ON;
        }
        else
        {
            IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
            /*do nothing, retain previous values */
            sWritableFlag = RP_OFF;
        }
    }
    else
    {
        sWritableFlag = RP_ON;
    }

    if(sWritableFlag == RP_ON)
    {
        smiLogRec::getLogHdr(aLogPtr, &sLogHead);
        sTmpSN = smiLogRec::getSNFromLogHdr(&sLogHead);

        // PROJ-1705 파라메터 추가
        sLog.initialize(NULL,
                        NULL,
                        RPU_REPLICATION_POOL_ELEMENT_SIZE);

       /* Replication이 필요로하는 로그인지 확인하고 필요로하는
        * 로그인 경우 RP Log Buf에 copy한다.*/
        if ( sLog.needReplicationByType( &sLogHead,
                                         aLogPtr,
                                         &aLSN ) == ID_TRUE )
        {
            IDE_TEST_RAISE(((aSize + mBufferEndLogSize) > mBufSize), ERR_OVERFLOW_RPBUF);

            /*if not enough space, write buffer end Log */
            if((mWritePtr + aSize + mBufferEndLogSize) > mBufSize)
            {
                /*mWrotePtr을 mEndPtr로 만든다*/
                IDE_TEST(writeBufferEndLogToRPBuf() != IDE_SUCCESS);
            }

            /* write할 메모리가 Sender가 읽은 마지막 주소를 overwrite한다.
             * sWritePtr <= mMinReadPtr < sWritePtr + aSize */
            if((mWritePtr <= mMinReadPtr) &&
               (mMinReadPtr < mWritePtr + aSize))
            {
                updateMinReadPtr();                  //mMinReadPtr를 다시 계산
                if((mWritePtr <= mMinReadPtr) &&
                   (mMinReadPtr < mWritePtr + aSize))//더이상 복사할 공간이 없음
                {
                    IDE_RAISE(ERR_NO_SPACE);
                }
            }
            /*Log를 Buf에 Copy한다.*/
            sCopyStartPtr = (SChar*)(mBasePtr + mWritePtr);
            idlOS::memcpy(sCopyStartPtr, aLogPtr, aSize);

            /* BUG-22098 Buffer에 복사한 후에 mWrotePtr를 갱신해야 한다. */
            IDL_MEM_BARRIER;

            /*sWirtePtr도 포함하기 때문에 aSize -1을 함*/
            sWrotePtr = mWritePtr + (aSize - 1);
        }
        else
        {
            /*retain previous wroteptr value*/
            sWrotePtr = mWrotePtr;
        }

        IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
        mWrotePtr = sWrotePtr;
        if(mMinSN == SM_SN_NULL)
        {
            mMinSN = sTmpSN;
        }
        mMaxSN = sTmpSN;
        SM_GET_LSN(mMaxLSN, aLSN);//현재 로그의 aLSN의 값을 mMaxLSN에 copy
        mWritableFlag = RP_ON;
        IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
        mWritePtr = getWritePtr(mWrotePtr);
    }
    else
    {
        /*nothing to do*/
    }
    if(aStatistics != NULL)
    {
        RP_OPTIMIZE_TIME_END(aStatistics, IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_OVERFLOW_RPBUF);
    {
        ideLog::log(IDE_RP_4, RP_TRC_LB_OVERFLOW_RPBUF);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_OVERFLOW));
    }
    IDE_EXCEPTION(ERR_NO_SPACE);
    {
        ideLog::log(IDE_RP_4, RP_TRC_LB_NO_SPACE);
        IDE_SET(ideSetErrorCode(rpERR_IGNORE_RP_NO_SPACE));
    }

    IDE_EXCEPTION_END;

    if(aStatistics != NULL)
    {
        RP_OPTIMIZE_TIME_END(aStatistics, IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER);
    }
    if(mWritableFlag == RP_ON)
    {
        IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
        mWritableFlag = RP_OFF;
        IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
    }
    return IDE_FAILURE;
}

/*sender가 읽고 있는 메모리 때문에 Write하지 못하면
 *IDE_FAILURE를 반환한다
 */
IDE_RC rpdLogBufferMgr::writeBufferEndLogToRPBuf()
{
    SChar* sWriteStartPtr = NULL;
    UInt   sRemainBufSize = mEndPtr - mWrotePtr;
    /* write할 메모리가 Sender가 읽고 있는 주소를 overwrite한다.
     * mWritePtr <= mMinReadPtr <= mEndPtr
     */

    if((mWritePtr <= mMinReadPtr) && (mMinReadPtr <= mEndPtr))
    {
        updateMinReadPtr(); //cache되어있는 정보이므로 다시 계산

        if((mWritePtr <= mMinReadPtr) && (mMinReadPtr <= mEndPtr))
        {
            IDE_RAISE(ERR_NO_SPACE); //메모리가 부족함
        }
    }
    //아무로그도 안쓰여진 상태에서 buffer end가 쓰일 수 없음
    IDE_DASSERT(mMaxSN != SM_SN_NULL);

    /*write: buffer end log는 정상적인 로그가 아니고 RP Buf에서 임시로 만든 것이다.
     *이 로그는 버퍼 마지막에 쓰여지며 따로 SN을 갖지 않으므로 가장 최근에 처리된 SN을 Set한다.
     *이 SN은 min ptr update시 사용한다.*/
    smiLogRec::setSNOfDummyLog(&mBufferEndLog, mMaxSN);
    smiLogRec::setLogSizeOfDummyLog(&mBufferEndLog, sRemainBufSize);
    sWriteStartPtr = (SChar*)(mBasePtr + mWritePtr);
    idlOS::memcpy(sWriteStartPtr, &mBufferEndLog, mBufferEndLogSize);

    /* BUG-32475 Buffer에 복사한 후에 mWrotePtr를 갱신해야 한다. */
    IDL_MEM_BARRIER;

    IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    mWrotePtr = mEndPtr;
    IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
    mWritePtr = getWritePtr(mWrotePtr);
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_SPACE);
    {
        ideLog::log(IDE_RP_4, RP_TRC_LB_NO_SPACE);
        IDE_SET(ideSetErrorCode(rpERR_IGNORE_RP_NO_SPACE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpdLogBufferMgr::updateMinReadPtr()
{
    UInt sIdx;
    UInt sCnt    = 0;
    smSN sMinSN  = SM_SN_NULL;
    UInt sMinReadPtr  = mBufSize;
    UInt sReadPtr = RP_BUF_NULL_PTR;
    UInt sDistance    = ID_UINT_MAX;
    UInt sMinDistance = ID_UINT_MAX;
    void* sLogPtr = NULL;
    smiLogHdr sLogHead;

    IDE_DASSERT(mAccessInfoCnt <= mMaxSenderCnt);
    /*Sender가 들어올 수 없도록 mutex를 잡아야함*/
    IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    if(mAccessInfoCnt == 0)
    {
        //현재 리스트를 변경중인 Sender가 없으므로 바로 리턴함
        IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
        return;
    }
    /**************************************************************************
     *                               |-----write해야하는 곳-------|
     *     Sender1                                    Sender2
     * |---Readptr----------------WritePtr------------ReadPtr------------| Buf
     **************************************************************************/
    /*writePtr에서 가장 거리가 가까운 readptr(최소 read ptr) 구하기*/
    for(sIdx = 0; (sCnt < mAccessInfoCnt) && (sIdx < mMaxSenderCnt); sIdx++)
    {
        /*sender는 로그를 읽는 중에 read ptr을 RP_BUF_NULL_PTR로 만들 수 없고,
         *Buf를 들어올때 특정값으로 세팅하고 나갈 때 RP_BUF_NULL_PTR로 초기화 한다.*/
        sReadPtr = mAccessInfoList[sIdx].mReadPtr;
        if( sReadPtr != RP_BUF_NULL_PTR )
        {
            sCnt++;
            if( sReadPtr > mWrotePtr )
            {
                sDistance = sReadPtr - mWrotePtr;
            }
            else
            {
                sDistance = mBufSize - (mWrotePtr - sReadPtr);
            }

            if(sDistance < sMinDistance)
            {
                sMinDistance = sDistance;
                IDU_FIT_POINT( "1.BUG-34115@rpdLogBufferMgr::updateMinReadPtr" );
                sMinReadPtr  = sReadPtr;
            }
        }
    }
    if(sMinReadPtr != mMinReadPtr) //변경할 필요 없음
    {
        if(sMinReadPtr == mWrotePtr)//버퍼가 비어있는 상태임
        {
            sMinSN = SM_SN_NULL;
        }
        else //최소 SN을 버퍼에서 구함
        {
            sLogPtr = mBasePtr + getReadPtr(sMinReadPtr);
            smiLogRec::getLogHdr(sLogPtr, &sLogHead);
            if(isBufferEndLog(&sLogHead) == ID_TRUE)
            {
                if(mEndPtr == mWrotePtr)
                {
                    sMinSN = SM_SN_NULL;
                }
                else /* 버퍼의 마지막에 도달한 경우, 버퍼의 처음에 다음 로그가 있다. */
                {
                    sLogPtr = mBasePtr;
                    smiLogRec::getLogHdr(sLogPtr, &sLogHead);
                    sMinSN = smiLogRec::getSNFromLogHdr(&sLogHead);
                }
            }
            else
            {
                sMinSN = smiLogRec::getSNFromLogHdr(&sLogHead);
            }
        }

        IDE_DASSERT(sMinReadPtr != mBufSize);

        if( mMinSN != SM_SN_NULL )
        {
            if( mMinSN > sMinSN )
            {
                /*abnormal situation no update MinReadPtr*/
                ideLog::log(IDE_RP_0,
                            "Replication Log Buffer Error: sMinSN=%"ID_UINT64_FMT", "
                            "sMinReadPtr=%"ID_UINT32_FMT,
                            sMinSN, sMinReadPtr );
                printBufferInfo();
            }
            else
            {
                mMinSN      = sMinSN;
                mMinReadPtr = sMinReadPtr;
            }
        }
        else
        {
            mMinSN      = sMinSN;
            mMinReadPtr = sMinReadPtr;
        }
    }
    IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);

    return;

    IDE_EXCEPTION_END;

    return;
}

/***********************************************************************
 * 버퍼에서 로그를 반환한다. call by sender
 * Sender가 버퍼모드로 읽고 잇는 경우에 Sender ID를 이용해 로그를 반환한다.
 * [IN]  aSndrID: 호출하는 Sender의 ID(AccessInfoList의 Index)
 * [OUT] aLogHead: 검색된 로그의 align된 로그 head
 * [OUT] aSN: 검색된 로그의 SN, 혹은 검색된 로그가 없는 경우 가장 최근 SN
 *            아무것도 못 읽은 경우 SM_SN_NULL을 반환
 * [OUT] aLSN: 마지막 로그의 LSN, aIsLeave가 TRUE인 경우 마지막 로그의 LSN
 *             그렇지 않은 경우  INIT LSN을 가짐
 * [OUT] aIsLeave: 더 이상 읽을 로그가 없고, 모드를 전환해야 하는 경우 ID_TRUE
 * RETURN VALUE : 검색된 로그의 포인터, 없는 경우 NULL
 ************************************************************************/
IDE_RC rpdLogBufferMgr::readFromRPLogBuf(UInt       aSndrID,
                                        smiLogHdr* aLogHead,
                                        void**     aLogPtr,
                                        smSN*      aSN,
                                        smLSN*     aLSN,
                                        idBool*    aIsLeave)
{
    /*wrotePtr와 같은 주소 까지 읽으면 다 읽는 것임
     *만약 다 읽었다고 확인되고 OFF이면 버퍼 모드를 나감*/
    SChar*  sLogPtr = NULL;

    /*지금까지 읽은 마지막 ptr*/
    UInt sNextPtr = RP_BUF_NULL_PTR;
    UInt sNewReadPtr = RP_BUF_NULL_PTR;

    IDE_DASSERT(aSndrID < mMaxSenderCnt);

    *aLogPtr  = NULL;
    *aSN      = SM_SN_NULL;
    *aIsLeave = ID_FALSE;

    sNextPtr = mAccessInfoList[aSndrID].mNextPtr;

    if(mWrotePtr == sNextPtr) /*버퍼 내용을 모두 다 읽었음*/
    {
        /*RP_OFF인 경우 나가야함*/
        if(mWritableFlag == RP_OFF)
        {
            IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
            /* mWrotePtr == sReadPtr가 아닌 경우 service가 write했지만,
             * 처음 if(mWrotePtr == sReadPtr)에서 lock을 잡지 않고
             * mWrotePtr를 읽어서 이전 값을 읽었기 때문에 여기로
             * 들어올 수 있다. 그래서 다시 한번 확인하고 만약
             * 다르다면 다시 시도할 수 있도록 *aSN을 SM_SN_NULL로
             * 세팅하여 준다*/
            if((mWrotePtr == sNextPtr) && (mWritableFlag == RP_OFF))
            {
                *aSN = mMaxSN;
                SM_GET_LSN(*aLSN, mMaxLSN);
                *aIsLeave = ID_TRUE;
            }
            IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
        }
        else /*RP_ON인 경우 새로 생성된 로그가 없는 상황임*/
        {
            IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

            if((mWrotePtr == sNextPtr) && (mWritableFlag == RP_ON))
            {
                *aSN = mMaxSN;
            }
            /* *aIsLeave = ID_FALSE; */
            IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
            mAccessInfoList[aSndrID].mReadPtr = mAccessInfoList[aSndrID].mNextPtr;
        }
    }
    else /*읽어야 하는 ptr가 있음*/
    {
        IDL_MEM_BARRIER;
        sNewReadPtr = getReadPtr(sNextPtr);
        IDE_DASSERT(sNewReadPtr < mBufSize);
        sLogPtr = mBasePtr + sNewReadPtr;
        *aLogPtr = sLogPtr;
        smiLogRec::getLogHdr(sLogPtr, (smiLogHdr*)aLogHead);
        *aSN = smiLogRec::getSNFromLogHdr((smiLogHdr*)aLogHead);
        mAccessInfoList[aSndrID].mNextPtr = sNewReadPtr +
            smiLogRec::getLogSizeFromLogHdr(aLogHead) - 1;
        mAccessInfoList[aSndrID].mReadPtr = sNextPtr;

        /* BUG-32633 Buffer End 로그를 건너 뛴다. (최대 1회 수행) */
        if ( isBufferEndLog( aLogHead ) == ID_TRUE )
        {
            IDE_TEST( readFromRPLogBuf( aSndrID,
                                        aLogHead,
                                        aLogPtr,
                                        aSN,
                                        aLSN,
                                        aIsLeave )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpdLogBufferMgr::getSN(smSN* aMinSN, smSN* aMaxSN)
{
    IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    *aMinSN = mMinSN;
    *aMaxSN = mMaxSN;
    IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
}

/***********************************************************************
 * buffer모드로 진입 가능한지 테스트 하고 모드를 변경한다. call by sender
 * 진입 가능한 경우에 AccessInfoList에서 빈 슬롯을 찾아 해당 슬롯의
 * index번호를 ID로 반환해 준다.
 * [IN]     aNeedSN: Sender가 필요로하는 로그의 SN
 * [IN/OUT] aMinSN/aMaxSN: Sender가 알고있는(캐쉬된) 버퍼의 최소/최대 SN을 넘겨주면
 *                  필요한 경우 실제로 버퍼가 갖고있는 최소/최대 SN을 다시 넘겨준다.
 * [OUT] aSndrID : 진입이 성공한 경우 Sender에게 돌려주는 버퍼에서의 ID
 *                 진입이 실패한 경우 RP_BUF_NULL_ID를 반환한다.
 * [OUT] aIsEnter: 진입이 성공한 경우 ID_TRUE, 그렇지 않으면 ID_FALSE
 ************************************************************************/
idBool rpdLogBufferMgr::tryEnter(smSN    aNeedSN,
                                 smSN*   aMinSN,
                                 smSN*   aMaxSN,
                                 UInt*   aSndrID )
{
    UInt      sSndrID  = RP_BUF_NULL_ID;
    idBool    sIsEnter = ID_FALSE;

    IDE_DASSERT(aNeedSN != SM_SN_NULL);
    IDE_DASSERT(aMinSN != NULL);
    IDE_DASSERT(aMaxSN != NULL);
    IDE_DASSERT(aSndrID != NULL);

    if((*aMinSN <= aNeedSN) && (aNeedSN <= *aMaxSN))
    {
        if(mWritableFlag == RP_ON)
        {
            IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

            if((mMinSN != SM_SN_NULL) &&
               (mMaxSN != SM_SN_NULL) &&
               (mWritableFlag == RP_ON))
            {
                *aMinSN = mMinSN;
                *aMaxSN = mMaxSN;
                if((mMinSN <= aNeedSN) && (aNeedSN <= mMaxSN))
                {
                    //진입 가능함
                    for(sSndrID = 0; sSndrID < mMaxSenderCnt; sSndrID++)
                    {
                        if(mAccessInfoList[sSndrID].mReadPtr == RP_BUF_NULL_PTR)
                        {
                            mAccessInfoList[sSndrID].mReadPtr = mMinReadPtr;
                            mAccessInfoList[sSndrID].mNextPtr = mMinReadPtr;
                            mAccessInfoCnt++;
                            sIsEnter = ID_TRUE;
                            break;
                        }
                    }
                }
            }
            IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
            if(sSndrID >= mMaxSenderCnt)
            {
                sSndrID = RP_BUF_NULL_ID;
                sIsEnter = ID_FALSE;
            }
        }
        else // RP_OFF
        {
            *aMinSN = 0;
            *aMaxSN = SM_SN_MAX;
            sSndrID = RP_BUF_NULL_ID;
            sIsEnter = ID_FALSE;
        }
    }
    *aSndrID = sSndrID;

    return sIsEnter;
}

void rpdLogBufferMgr::leave(UInt aSndrID)
{
    if(aSndrID < mMaxSenderCnt)
    {
        IDE_ASSERT(mBufInfoMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
        mAccessInfoList[aSndrID].mReadPtr = RP_BUF_NULL_PTR;
        mAccessInfoList[aSndrID].mNextPtr = RP_BUF_NULL_PTR;
        mAccessInfoCnt--;
        IDE_ASSERT(mBufInfoMutex.unlock() == IDE_SUCCESS);
    }
}
void rpdLogBufferMgr::printBufferInfo()
{
    UInt sIdx = 0;

    for( sIdx = 0; sIdx < mMaxSenderCnt; sIdx++ )
    {
        if( mAccessInfoList[sIdx].mReadPtr != RP_BUF_NULL_PTR )
        {
            ideLog::log(IDE_RP_0,"Sender ID:%"ID_UINT32_FMT", "
                        "mReadPtr:%"ID_UINT32_FMT", "
                        "mNextPtr:%"ID_UINT32_FMT,
                        sIdx,
                        mAccessInfoList[sIdx].mReadPtr,
                        mAccessInfoList[sIdx].mNextPtr);
        }
        else
        {
            /*do nothing*/
        }
    }
    ideLog::log(IDE_RP_0,
                "mMinSN=%"ID_UINT64_FMT", "
                "mMinReadPtr=%"ID_UINT32_FMT", "
                "mWrotePtr=%"ID_UINT32_FMT,
                mMinSN, mMinReadPtr, mWrotePtr);
}
