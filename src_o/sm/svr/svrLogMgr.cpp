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
 
/**********************************************************************
 * Volatile Tablespace 연산을 위한 로그 매니저
 *
 *    메모리에 로그를 기록하는 함수와 로그를 읽는 함수를 제공한다.
 *    로그가 기록되는 메모리는 자체적으로 관리를 한다.
 *
 * Abstraction
 *    svrLogMgr는 log record를 쓰고 읽는 인터페이스를 제공한다.
 *    사용자는 기록할 log의 body를 구성해 writeLog함수를 통해
 *    로그 레코드를 기록할 수 있다. 로그 버퍼는 무한정의 연속된
 *    메모리 공간으로 간주할 수 있다.
 *    다음의 인터페이스를 제공한다.
 *      - initializeStatic
 *      - destroyStatic
 *      - initEnv
 *      - destroyEnv
 *      - writeLog
 *      - readLog
 *      - getLastLSN
 *
 * Implementation
 *    로그는 고정 크기 메모리인 페이지 단위로 쪼개진 공간에 저장된다.
 *    로그를 읽을 때 복사 비용을 줄이기 위해 하나의 로그가 한 페이지 내에
 *    있어야 한다는 제약을 가정한다.
 *    in-place update 로그는 최대 32K까지 그 크기를 가지기 때문에
 *    페이지 하나는 32K가 되야 한다.
 *    
 **********************************************************************/

#include <ide.h>
#include <iduMemPool.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <svrLogMgr.h>

/* 아래 정의된 자료 구조들은 svrLogMgr.cpp 안에서만 볼 수 있게 하기 위해
 * cpp 파일에 정의를 한다. */

/*****************************************************************
 * typedef와 define 문
 *****************************************************************/
typedef struct svrLogPage
{
    svrLogPage     *mNext;
    SChar           mLogBuf[1];
} svrLogPage;

/* log의 종류
   - sub log를 가지지 않는 일반 로그 -> mPrev 세팅
   - sub log를 가지는 일반 로그      -> mPrev, mNext 세팅
   - sub log                         -> mNext 세팅  */
typedef struct svrLogRec
{
    svrLogPage     *mPageBelongTo;
    svrLogRec      *mPrevLogRec;
    svrLogRec      *mNextSubLogRec;
    UInt            mBodySize;
    SChar           mLogBody[1];
} svrLogRec;

#define SVR_LOG_PAGE_SIZE       (32768)
#define SVR_LOG_PAGE_HEAD_SIZE  (offsetof(svrLogPage, mLogBuf))
#define SVR_LOG_PAGE_BODY_SIZE  (SVR_LOG_PAGE_SIZE - SVR_LOG_PAGE_HEAD_SIZE)
#define SVR_LOG_HEAD_SIZE       (offsetof(svrLogRec, mLogBody))

/* mempool에서 메모리 할당 단위는 chunk 단위이다.
   즉 32개 페이지 씩 할당한다.
   초기엔 32개의 페이지가 할당된다. 즉 1M를 사용하게 된다. */
#define SVR_LOG_PAGE_COUNT_PER_CHUNK      (32)

/* mempool이 한번 늘었다가 줄어들어야 하는 최소 크기를 정한다.
   10개의 chunk는 10M이다.  */
#define SVR_LOG_PAGE_CHUNK_SHRINK_LIMIT   (10)
 
/*****************************************************************
 * 이 파일에서만 참조하는 private 용 static instance 정의
 *****************************************************************/
static iduMemPool   mLogMemPool;

/*****************************************************************
 * 이 파일에서만 참조하는 private 용 static 함수 선언
 *****************************************************************/
static IDE_RC allocNewLogPage(svrLogEnv *aEnv);

static UInt logRecSize(svrLogEnv *aEnv, UInt aLogDataSize);

static SChar* curPos(svrLogEnv *aEnv);

/* svrLogEnv.mPageOffset은 align이 고려되어야 한다.
   다음 두 함수에 의해 mPageOffset이 갱신되어야 한다. */
static void initOffset(svrLogEnv *aEnv);

static void updateOffset(svrLogEnv *aEnv, UInt aIncOffset);

static svrLogRec* getLastSubLogRec(svrLogRec *aLogRec);

/*****************************************************************
 * svrLogMgr 인터페이스 함수들 정의
 *****************************************************************/

/*****************************************************************
 * Description:
 *    volatile logging을 하기 위해서 이 함수를 호출해야 한다.
 *    volatile tablespace가 없다면 이 함수를 부르는 것은 메모리
 *    낭비일 뿐이다.
 *    이 함수를 호출하면 종료시 destory()를 호출해서 메모리를
 *    해제해야 한다.
 *****************************************************************/
IDE_RC svrLogMgr::initializeStatic()
{
    /* mLogMemPool을 초기화한다. */
    IDE_TEST(mLogMemPool.initialize(IDU_MEM_SM_SVR,
                                    (SChar*)"Volatile log memory pool",
                                    ID_SCALABILITY_SYS,
                                    SVR_LOG_PAGE_SIZE,
                                    SVR_LOG_PAGE_COUNT_PER_CHUNK,
                                    SVR_LOG_PAGE_CHUNK_SHRINK_LIMIT,	/* ChunkLimit */
                                    ID_TRUE,							/* UseMutex */
                                    IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                    ID_FALSE,							/* ForcePooling */
                                    ID_TRUE,							/* GarbageCollection */
                                    ID_TRUE,                            /* HWCacheLine */
                                    IDU_MEMPOOL_TYPE_LEGACY             /* mempool type*/) 
             != IDE_SUCCESS);			

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC svrLogMgr::destroyStatic()
{
    IDE_TEST(mLogMemPool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 * Desciption:
 *   volatile log를 기록하기 위해서는 svrLogEnv 자료구조가 필요하다.
 *   initEnv()를 통해서 svrLogEnv를 초기화한다.
 *   initEnv에서는 svrLogEnv의 memPool을 초기화하고 빈 페이지를 하나
 *   할당하며 offset을 초기화한다.
 *****************************************************************/
IDE_RC svrLogMgr::initEnv(svrLogEnv *aEnv, idBool aAlignForce)
{
    /* svrLogEnv의 각 맴버를 초기화한다. */
    aEnv->mHeadPage = NULL;
    aEnv->mCurrentPage = NULL;
    aEnv->mPageOffset = SVR_LOG_PAGE_SIZE;
    aEnv->mLastLogRec = SVR_LSN_BEFORE_FIRST;
    aEnv->mLastSubLogRec = SVR_LSN_BEFORE_FIRST;
    aEnv->mAlignForce = aAlignForce;
    aEnv->mAllocPageCount = 0;
    aEnv->mFirstLSN = SVR_LSN_BEFORE_FIRST;

    return IDE_SUCCESS;
}

/*****************************************************************
 * Desciption:
 *   initEnv에서 초기화했던 자원들을 destroyEnv()를 통해서 해제한다.
 *
 * Imeplementation:
 *    이때까지 alloc한 log page들을 순회하면서 모두 free해준다.
 *****************************************************************/
IDE_RC svrLogMgr::destroyEnv(svrLogEnv *aEnv)
{
    svrLogPage  *sPrevPage;
    svrLogPage  *sCurPage;
    UInt         sPageCount = 0;

    sPrevPage = aEnv->mHeadPage;

    while (sPrevPage != NULL)
    {
        sCurPage = sPrevPage->mNext;

        IDE_TEST(mLogMemPool.memfree(sPrevPage) != IDE_SUCCESS);

        sPrevPage = sCurPage;

        sPageCount++;
    }

    IDE_ASSERT(aEnv->mAllocPageCount == sPageCount);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 * Description:
 *   로그를 기록하기 위해 할당한 메모리의 총 크기를 구한다.
 *****************************************************************/
UInt svrLogMgr::getAllocMemSize(svrLogEnv *aEnv)
{
    return aEnv->mAllocPageCount * SVR_LOG_PAGE_SIZE;
}

/*****************************************************************
 * Description:
 *   log record를 로그 버퍼에 기록한다.
 *   log record는 sub log를 가질 수 있다.
 *   - aEnv는 initEnv()를 통해서 초기화된 상태이어야 하며,
 *   - aLogData는 로그 내용을 가리키고 있는 포인터이며,
 *   - aLogSize는 로그 내용의 길이이다.
 *
 * Implementation:
 *   현재 로그 버퍼 페이지를 체크해서 로그를 기록할 공간이 있는지
 *   먼저 검사한다. 만약 현재 페이지에 넣을 수 없으면
 *   새로운 페이지를 할당한다.
 *   mLastLogRec을 갱신하고 mLastSubLogRec을 NULL로 세팅한다.
 *****************************************************************/
IDE_RC svrLogMgr::writeLog(svrLogEnv *aEnv,
                           svrLog    *aLogData,
                           UInt       aLogSize)
{
    svrLogRec sLogRec;

    IDE_ASSERT(logRecSize(aEnv, aLogSize) <= SVR_LOG_PAGE_BODY_SIZE);

    /* 현재 페이지에 로그를 기록할 수 있는지 검사한다.
       기록할 수 없으면 새로운 page를 할당받는다. */
    if (aEnv->mPageOffset + logRecSize(aEnv, aLogSize) >
        SVR_LOG_PAGE_SIZE)
    {
        IDE_TEST_RAISE(allocNewLogPage(aEnv) != IDE_SUCCESS,
                       cannot_alloc_logbuffer);
    }

    /* 로그 헤드 세팅 */
    sLogRec.mPageBelongTo = aEnv->mCurrentPage;
    sLogRec.mPrevLogRec = aEnv->mLastLogRec;
    sLogRec.mNextSubLogRec = SVR_LSN_BEFORE_FIRST;
    sLogRec.mBodySize = aLogSize;

    /* 마지막 log record 포인터 갱신 */
    aEnv->mLastLogRec = (svrLogRec*)(curPos(aEnv));
    aEnv->mLastSubLogRec = SVR_LSN_BEFORE_FIRST;

    /* log head 기록 */
    idlOS::memcpy(curPos(aEnv),
                  &sLogRec,
                  SVR_LOG_HEAD_SIZE);
    updateOffset(aEnv, SVR_LOG_HEAD_SIZE);

    /* log body 기록 */
    idlOS::memcpy(curPos(aEnv),
                  aLogData,
                  aLogSize);
    updateOffset(aEnv, aLogSize);

    /* mFirstLSN 갱신 */
    if (aEnv->mFirstLSN == SVR_LSN_BEFORE_FIRST)
    {
        aEnv->mFirstLSN = aEnv->mLastLogRec;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(cannot_alloc_logbuffer)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CannotAllocLogBufferMemory));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 * Description:
 *   sub log record를 로그 버퍼에 기록한다.
 *   - aEnv는 initEnv()를 통해서 초기화된 상태이어야 하며,
 *   - aLogData는 로그 내용을 가리키고 있는 포인터이며,
 *   - aLogSize는 로그 내용의 길이이다.
 *
 * Implementation:
 *   현재 로그 버퍼 페이지를 체크해서 로그를 기록할 공간이 있는지
 *   먼저 검사한다. 만약 현재 페이지에 넣을 수 없으면
 *   새로운 페이지를 할당한다.
 *   mLastLogRec은 갱신하지 않고 mLastSubLogRec을 갱신한다.
 *****************************************************************/
IDE_RC svrLogMgr::writeSubLog(svrLogEnv *aEnv,
                              svrLog    *aLogData, 
                              UInt       aLogSize)
{
    svrLogRec sSubLogRec;

    IDE_ASSERT(logRecSize(aEnv, aLogSize) <= SVR_LOG_PAGE_BODY_SIZE);

    /* 현재 페이지에 로그를 기록할 수 있는지 검사한다.
       기록할 수 없으면 새로운 page를 할당받는다. */
    if (aEnv->mPageOffset + logRecSize(aEnv, aLogSize) >
        SVR_LOG_PAGE_SIZE)
    {
        IDE_TEST_RAISE(allocNewLogPage(aEnv) != IDE_SUCCESS,
                       cannot_alloc_logbuffer);
    }

    /* 로그 헤드 세팅 */
    sSubLogRec.mPageBelongTo = aEnv->mCurrentPage;
    sSubLogRec.mPrevLogRec = SVR_LSN_BEFORE_FIRST;
    sSubLogRec.mNextSubLogRec = SVR_LSN_BEFORE_FIRST;
    sSubLogRec.mBodySize = aLogSize;

    /* 이전 로그(mLastLogRec이거나 mLastSubLogRec)의 mNextSubLogRec을
       갱신한다. */
    if (aEnv->mLastSubLogRec == SVR_LSN_BEFORE_FIRST)
    {
        /* mLastSubLogRec이 SVR_LSN_BEFORE_FIRST라면
           아직 sub log가 기록되지 않았다.
           이 경우 last log의 mNextSubLogRec을 갱신해야 한다. */
        IDE_ASSERT(aEnv->mLastLogRec != SVR_LSN_BEFORE_FIRST);

        aEnv->mLastLogRec->mNextSubLogRec = (svrLogRec*)curPos(aEnv);
    }
    else
    {
        aEnv->mLastSubLogRec->mNextSubLogRec = (svrLogRec*)curPos(aEnv);
    }

    /* aEnv의 mLastSubLogRec을 갱신한다.
       mLastLogRec은 갱신하지 않는다. */
    aEnv->mLastSubLogRec = (svrLogRec*)curPos(aEnv);

    /* log head 기록 */
    idlOS::memcpy(curPos(aEnv),
                  &sSubLogRec,
                  SVR_LOG_HEAD_SIZE);
    updateOffset(aEnv, SVR_LOG_HEAD_SIZE);

    /* log body 기록 */
    idlOS::memcpy(curPos(aEnv),
                  aLogData,
                  aLogSize);
    updateOffset(aEnv, aLogSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(cannot_alloc_logbuffer)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_CannotAllocLogBufferMemory));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 * Description:
 *    이때까지 기록한 로그 레코드중 마지막 로그 레코드의 LSN을 얻는다.
 *****************************************************************/
svrLSN svrLogMgr::getLastLSN(svrLogEnv *aEnv)
{
    return aEnv->mLastLogRec;
}

idBool svrLogMgr::isOnceUpdated(svrLogEnv *aEnv)
{
    return ((aEnv->mLastLogRec != SVR_LSN_BEFORE_FIRST) ? ID_TRUE : ID_FALSE);
}

/*****************************************************************
 * Description:
 *    aLSNToRead가 가리키는 로그 레코드를 읽어 aBufToLoadAt에
 *    기록한다.
 *    undo 시 다음에 읽을 로그 레코드의 LSN을 aUndoNextLSN로
 *    반환한다.
 *****************************************************************/
IDE_RC svrLogMgr::readLogCopy(svrLogEnv *aEnv,
                              svrLSN     aLSNToRead,
                              svrLog    *aBufToLoadAt,
                              svrLSN    *aUndoNextLSN,
                              svrLSN    *aNextSubLSN)
{
    svrLogRec *sLogRec = aLSNToRead;
    UInt       sBodySize;

    IDE_ASSERT(sLogRec != SVR_LSN_BEFORE_FIRST);

    /* 다음 undo할 log record의 lsn을 세팅한다. */
    idlOS::memcpy(aUndoNextLSN, &sLogRec->mPrevLogRec, ID_SIZEOF(svrLSN*));

    /* 다음에 읽을 sub log record가 있으면 세팅한다. */
    idlOS::memcpy(aNextSubLSN, &sLogRec->mNextSubLogRec, ID_SIZEOF(svrLSN*));

    /* 로그 데이터를 버퍼에 복사한다. */
    if (aEnv->mAlignForce == ID_TRUE)
    {
        idlOS::memcpy(aBufToLoadAt, 
                      (SChar*)(vULong)idlOS::align8((ULong)(vULong)sLogRec->mLogBody),
                      sLogRec->mBodySize);
    }
    else
    {
        idlOS::memcpy(&sBodySize, &sLogRec->mBodySize, ID_SIZEOF(UInt));
        idlOS::memcpy(aBufToLoadAt, sLogRec->mLogBody, sBodySize);
    }

    return IDE_SUCCESS;
}

/*****************************************************************
 * Description:
 *    로그가 기록된 위치를 리턴한다. align이 안되어 있기 때문에
 *    primitive type 케스팅하면 안된다.
 *****************************************************************/
IDE_RC svrLogMgr::readLog(svrLogEnv *aEnv,
                          svrLSN     aLSNToRead,
                          svrLog   **aLogData,
                          svrLSN    *aUndoNextLSN,
                          svrLSN    *aNextSubLSN)
{
    svrLogRec *sLogRec = aLSNToRead;

    IDE_ASSERT(sLogRec != SVR_LSN_BEFORE_FIRST);

    /* 읽을 로그 위치를 세팅한다. */
    if (aEnv->mAlignForce == ID_TRUE)
    {
        *aLogData = (svrLog *)(vULong)
            idlOS::align8((ULong)(vULong)sLogRec->mLogBody);
    }
    else
    {
        /* mAlignForce가 flase일 경우엔 이 함수를 호출한 곳에서
           로그를 memcpy한 후 읽어야 한다. */
        *aLogData = (svrLog *)sLogRec->mLogBody;
    }

    /* 다음 undo할 log record의 lsn을 세팅한다. */
    idlOS::memcpy(aUndoNextLSN, &sLogRec->mPrevLogRec, ID_SIZEOF(svrLSN*));

    /* 다음에 읽을 sub log record가 있으면 세팅한다. */
    idlOS::memcpy(aNextSubLSN, &sLogRec->mNextSubLogRec, ID_SIZEOF(svrLSN*));

    return IDE_SUCCESS;
}

IDE_RC svrLogMgr::removeLogHereafter(svrLogEnv *aEnv,
                                     svrLSN     aThisLSN)
{
    svrLogPage *sPrevPage;
    svrLogPage *sCurPage;
    idBool      sAlignForce;
    svrLogRec  *sLastPosLogRec;

    if (aThisLSN == SVR_LSN_BEFORE_FIRST)
    {
        /* total rollback시 aThisLSN이 SVR_LSN_BEFORE_FIRST로
           넘어온다. 이 경우 aEnv를 완전 초기화해야 한다. */
        sAlignForce = aEnv->mAlignForce;
        destroyEnv(aEnv);
        initEnv(aEnv, sAlignForce);
    }
    else
    {
        /* aEnv의 각 멤버들을 조정한다. */
        aEnv->mLastLogRec = aThisLSN;
        aEnv->mLastSubLogRec = getLastSubLogRec(aEnv->mLastLogRec);

        if (aEnv->mLastSubLogRec == SVR_LSN_BEFORE_FIRST)
        {
            sLastPosLogRec = aEnv->mLastLogRec;
        }
        else
        {
            sLastPosLogRec = aEnv->mLastSubLogRec;
        }

        aEnv->mCurrentPage = sLastPosLogRec->mPageBelongTo;
        aEnv->mPageOffset = (SChar*)sLastPosLogRec - (SChar*)aEnv->mCurrentPage;

        updateOffset(aEnv, SVR_LOG_HEAD_SIZE);
        updateOffset(aEnv, sLastPosLogRec->mBodySize);

        /* mCurrentPage 이후의 페이지들을 모두 삭제한다. */
        sPrevPage = aEnv->mCurrentPage->mNext;
        while (sPrevPage != NULL)
        {
            sCurPage = sPrevPage->mNext;

            IDE_TEST(mLogMemPool.memfree(sPrevPage) != IDE_SUCCESS);

            sPrevPage = sCurPage;

            aEnv->mAllocPageCount--;
        }

        /* BUG-18018 : svrLogMgrTest가 실패합니다.
         *
         * aEnv->mCurrentPage이후 페이지를 Free한후에
         * aEnv->mCurrentPage이 Tail이 되는데 Tail의
         * mNext를 Null하지 않아서 추후에 이 link를 따라서
         * Free하는 Logic에서 죽습니다. */
        aEnv->mCurrentPage->mNext = NULL;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 * Description:
 *   새로운 로그 버퍼 페이지를 할당한다.
 *****************************************************************/
IDE_RC allocNewLogPage(svrLogEnv *aEnv)
{
    svrLogPage *sNewPage;

    /* svrLogMgr_allocNewLogPage_alloc_NewPage.tc */
    IDU_FIT_POINT("svrLogMgr::allocNewLogPage::alloc::NewPage");
    IDE_TEST(mLogMemPool.alloc((void**)&sNewPage)
             != IDE_SUCCESS);

    sNewPage->mNext = NULL;

    if (aEnv->mCurrentPage != NULL)
    {
        aEnv->mCurrentPage->mNext = sNewPage;
    }
    else
    {
        /* mCurrentPage가 NULL이면 처음 alloc하는 것이기 때문에
           next를 세팅할 필요가 없다. */
        aEnv->mHeadPage = sNewPage;
    }

    aEnv->mCurrentPage = sNewPage;

    initOffset(aEnv);

    aEnv->mAllocPageCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 * Description:
 *    로그 데이터 크기를 입력받아 실제 로그 레코드가 
 *    로그 버퍼에 저장될 때 차지하는 크기를 구한다.
 *    이 함수는 align을 고려한다.
 *****************************************************************/
UInt logRecSize(svrLogEnv *aEnv, UInt aLogDataSize)
{
    if (aEnv->mAlignForce == ID_TRUE)
    {
        return idlOS::align8((SInt)SVR_LOG_HEAD_SIZE) + 
               idlOS::align8(aLogDataSize);
    }

    return SVR_LOG_HEAD_SIZE + aLogDataSize;
}

/*****************************************************************
 * Description:
 *    로그를 기록할 위치를 가리키는 mPageOffset을 초기화한다.
 *    aEnv->mAlignForce를 참조하여 align 여부를 판단한다.
 *****************************************************************/
void initOffset(svrLogEnv *aEnv)
{
    if (aEnv->mAlignForce == ID_TRUE)
    {
        aEnv->mPageOffset = idlOS::align8((SInt)SVR_LOG_PAGE_HEAD_SIZE);
    }
    else
    {
        aEnv->mPageOffset = SVR_LOG_PAGE_HEAD_SIZE;
    }
}

/*****************************************************************
 * Description:
 *    로그를 기록할 위치를 가리키는 mPageOffset을 갱신한다.
 *    이때 align을 고려하여 갱신해야 한다.
 *****************************************************************/
void updateOffset(svrLogEnv *aEnv, UInt aIncOffset)
{
    if (aEnv->mAlignForce == ID_TRUE)
    {
        aEnv->mPageOffset += idlOS::align8(aIncOffset);
    }
    else
    {
        aEnv->mPageOffset += aIncOffset;
    }
}

/*****************************************************************
 * Description:
 *    aEnv가 가지고 있는 현재 레코딩 위치를 얻어온다.
 *
 * Implementation:
 *    mCurrentPage는 svrLogPage* 타입이기 때문에 SChar*로 캐스팅 후
 *    offset을 더해야 한다.
 *****************************************************************/
SChar* curPos(svrLogEnv *aEnv)
{
    return ((SChar*)aEnv->mCurrentPage) + aEnv->mPageOffset;
}

/*****************************************************************
 * Description:
 *    주어진 log record에 대해 그 log record의 마지막 sub log
 *    record를 구한다.
 *****************************************************************/
svrLogRec* getLastSubLogRec(svrLogRec *aLogRec)
{
    svrLogRec *sLastSubLogRec = SVR_LSN_BEFORE_FIRST;

    if (aLogRec->mNextSubLogRec != SVR_LSN_BEFORE_FIRST)
    {
        sLastSubLogRec = aLogRec->mNextSubLogRec;
        while (sLastSubLogRec->mNextSubLogRec != SVR_LSN_BEFORE_FIRST)
        {
            sLastSubLogRec = sLastSubLogRec->mNextSubLogRec;
        }
    }

    return sLastSubLogRec;
}

