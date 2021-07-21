/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*****************************************************************************
 * $Id:
 ****************************************************************************/

/*****************************************************************************
 *   NAME
 *     iddRBHash.cpp - Hash Table with Red-Black Tree
 *
 *   DESCRIPTION
 *     Hash implementation
 *
 *   MODIFIED   (04/07/2017)
 ****************************************************************************/

#include <iddRBHash.h>

iddRBHashLink   iddRBHash::mGlobalLink;
iduMutex        iddRBHash::mGlobalMutex;

/**
 * Hash를 초기화한다
 *
 * @param aName : RB Hash의 이름
 * @param aIndex : 내부적으로 사용할 메모리 할당 인덱스
 * @param aKeyLength : 키 크기(bytes)
 * @param aUseLatch : 래치 사용 여부(동시성제어용)
 * @param aHashFunc : 해시값 생성용 함수
 *                    SInt(const void* aKey) 형태
 * @param aCompFunc : 키 비교용 함수
 *                    SInt(const void* aKey1, const void* aKey2) 형태이며
 *                    aKey1 >  aKey2이면 1 이상을,
 *                    aKey1 == aKey2이면 0 을
 *                    aKey1 < aKey2이면 -1 이하를 리턴해야 한다.
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddRBHash::initialize(const SChar*               aName,
                             const iduMemoryClientIndex aIndex,
                             const UInt                 aBucketCount,
                             const UInt                 aKeyLength,
                             const idBool               aUseLatch,
                             const iddHashFunc          aHashFunc,
                             const iddCompFunc          aCompFunc)
{
    UInt    i;
    UInt    j;
    SInt    sState = 0;

    mIndex          = aIndex;
    mBucketCount    = aBucketCount;
    mHashFunc       = aHashFunc;
    mOpened         = ID_FALSE;

    IDE_TEST( mMutex.initialize((SChar*)"IDD_HASH_MUTEX",
                                IDU_MUTEX_KIND_POSIX,
                                IDV_WAIT_INDEX_NULL)
            != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduMemMgr::malloc(mIndex,
                                sizeof(bucket) * mBucketCount,
                                (void**)&mBuckets) != IDE_SUCCESS );
    sState = 2;

    for(i = 0; i < mBucketCount; i++)
    {
        new (mBuckets + i) bucket();
        IDE_TEST( mBuckets[i].initialize(aIndex, aKeyLength, aCompFunc, aUseLatch)
                  != IDE_SUCCESS );
    }

    sState = 3;

    IDE_TEST( mGlobalMutex.lock(NULL) != IDE_SUCCESS );
    mNext = &(mGlobalLink);
    mPrev = mGlobalLink.mPrev;
    mPrev->mNext = this;
    mNext->mPrev = this;
    IDE_TEST( mGlobalMutex.unlock() != IDE_SUCCESS );

    idlOS::strncpy(mName, aName, 32);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    switch(sState)
    {
    case 3:
        IDE_ASSERT( mGlobalMutex.unlock() == IDE_SUCCESS );
        /* fall through */
    case 2:
        for(j = 0; j < i; j++)
        {
            IDE_ASSERT( mBuckets[j].destroy() == IDE_SUCCESS );
        }
        /* fall through */

    case 1:
        IDE_ASSERT( iduMemMgr::free(mBuckets) == IDE_SUCCESS );
        /* fall through */

    case 0:
        IDE_ASSERT( mMutex.destroy() == IDE_SUCCESS );
        break;

    default:
        IDE_ASSERT(0);
        break;
    }

    return IDE_FAILURE;
}

/**
 * Hash의 내부 데이터를 깡그리 비운다
 *
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddRBHash::reset(void)
{
    UInt i;
    
    for(i = 0; i < mBucketCount; i++)
    {
        IDE_TEST( mBuckets[i].clear() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Hash의 내부 데이터를 비우고 할당받은 모든 메모리를 해제한다
 *
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddRBHash::destroy(void)
{
    UInt i;
    SInt sState = 0;
    
    for(i = 0; i < mBucketCount; i++)
    {
        IDE_TEST( mBuckets[i].destroy() != IDE_SUCCESS );
    }
    IDE_TEST( iduMemMgr::free(mBuckets) != IDE_SUCCESS );
    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );
    
    IDE_TEST( mGlobalMutex.lock(NULL) != IDE_SUCCESS );
    sState = 1;
    mPrev->mNext = mNext;
    mNext->mPrev = mPrev;
    IDE_TEST( mGlobalMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    if(sState == 1)
    {
        IDE_ASSERT( mGlobalMutex.unlock() == IDE_SUCCESS );
    }
    return IDE_FAILURE;
}

/**
 * Hash Table에 키/데이터를 추가한다.
 *
 * @param aKey : 키
 * @param aData : 값을 지니고 있는 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 중복 값이 있거나 메모리 할당에 실패했을 때
 */
IDE_RC iddRBHash::insert(const void* aKey, void* aData)
{
    UInt sBucketNo = hash(aKey);
    IDE_TEST( mBuckets[sBucketNo].insert(aKey, aData) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Hash Table에서 aKey를 검색한다
 * 검색된 키에 해당하는 데이터는 aData에 저장된다
 *
 * @param aKey : 키
 * @param aData : 검색된 데이터를 지닐 포인터 데이터가 없으면 NULL이 저장된다
 * @return IDE_SUCCESS : 값이 없어도 SUCCESS를 리턴한다
 *         IDE_FAILURE : 래치 획득 실패
 */
IDE_RC iddRBHash::search(const void* aKey, void** aData)
{
    UInt sBucketNo = hash(aKey);
    IDE_TEST( mBuckets[sBucketNo].search(aKey, aData) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Hash Table에서 aKey에 해당하는 데이터를 aNewData로 대체한다
 * 과거의 값은 aOldData가 NULL이 아닐 때 저장한다
 *
 * @param aKey : 키
 * @param aNewData : 새 데이터
 * @param aOldData : 과거 데이터를 저장할 포인터 NULL이 올 수 있다
 * @return IDE_SUCCESS 값을 찾았을 때
 *         IDE_FAILURE 값이 없거나 래치 획득 실패
 */
IDE_RC iddRBHash::update(const void* aKey, void* aData, void** aOldData)
{
    UInt sBucketNo = hash(aKey);
    IDE_TEST( mBuckets[sBucketNo].update(aKey, aData, aOldData) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Hash Table에서 키와 데이터를 삭제한다
 * 삭제된 데이터는 aData에 저장된다.
 *
 * @param aKey : 키
 * @param aData : 삭제된 데이터를 지닐 포인터. aKey가 없으면 NULL이 저장된다.
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 해제, 혹은 래치 획득에 실패했을 때
 */
IDE_RC iddRBHash::remove(const void* aKey, void** aData)
{
    UInt sBucketNo = hash(aKey);
    IDE_TEST( mBuckets[sBucketNo].remove(aKey, aData) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Hash Table을 순회할 수 있도록 커서를 연다
 * @return 항상 IDE_SUCCESS
 */
IDE_RC iddRBHash::open(void)
{
    IDE_ASSERT(mOpened == ID_FALSE);

    mIterator = begin();
    mOpened = ID_TRUE;

    return IDE_SUCCESS;
}

/**
 * Hash Table의 순회를 마치고 커서를 닫는다
 * @return 항상 IDE_SUCCESS
 */
IDE_RC iddRBHash::close(void)
{
    IDE_ASSERT(mOpened == ID_TRUE);
    mOpened = ID_FALSE;
    return IDE_SUCCESS;
}

/**
 * 현재 열려 있는 커서가 Hash Table의 끝인가?
 * @return ID_TRUE/ID_FALSE
 */
idBool iddRBHash::isEnd(void)
{
    IDE_ASSERT(mOpened == ID_TRUE);
    return (mIterator == end())? ID_TRUE:ID_FALSE;
}

/**
 * 현재 열려 있는 커서 위치의 데이터를 얻는다
 * @return 커서 위치의 데이터
 */
void* iddRBHash::getCurNode(void)
{
    IDE_ASSERT(mOpened == ID_TRUE);
    return mIterator.getData();
}

/**
 * 현재 열려 있는 커서 다음 위치의 데이터를 얻는다
 * @return 커서 다음 위치의 데이터
 */
void* iddRBHash::getNxtNode(void)
{
    IDE_ASSERT(mOpened == ID_TRUE);
    iterator sNext = mIterator;
    sNext++;
    return sNext.getData();
}

/**
 * Hash Table에서 커서 위치의 키와 데이터를 삭제한다
 * 다음 위치의 데이터를 얻어온다
 *
 * @param aNxtData : 다음 위치의 데이터를 담을 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 해제, 혹은 래치 획득에 실패했을 때
 */
IDE_RC iddRBHash::delCurNode(void** aNxtData)
{
    IDE_ASSERT(mOpened == ID_TRUE);
    iterator sPrev = mIterator;
    mIterator++;

    IDE_TEST( mBuckets[sPrev.mCurrent].remove(sPrev.getKey()) != IDE_SUCCESS );
    if(aNxtData != NULL)
    {
        *aNxtData = mIterator.getData();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Hash Table에서 커서 위치의 키와 데이터를 삭제하고
 * 커서를 하나 뒤로 이동시킨다
 * 커서가 있던 위치의 데이터를 얻어온다
 *
 * @param aData : 데이터를 담을 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 해제, 혹은 래치 획득에 실패했을 때
 */
IDE_RC iddRBHash::cutCurNode(void** aData)
{
    IDE_ASSERT(mOpened == ID_TRUE);
    iterator sPrev = mIterator;
    mIterator++;

    IDE_TEST( mBuckets[sPrev.mCurrent].remove(sPrev.getKey(), aData) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iddRBHash::iterator::iterator(iddRBHash* aHash, SInt aCurrent)
{
    mHash = aHash;
    mCurrent = aCurrent;

    if(mHash != NULL)
    {
        mTreeIterator = mHash->mBuckets[aCurrent].begin();
        while( mTreeIterator == mHash->mBuckets[mCurrent].end() )
        {
            mCurrent++;
            if(mCurrent == mHash->mBucketCount)
            {
                mHash = NULL;
                break;
            }
            else
            {
                mTreeIterator = mHash->mBuckets[mCurrent].begin();
            }
        }
    }
}

iddRBHash::iterator& iddRBHash::iterator::moveNext(void)
{
    if(mHash != NULL)
    {
        mTreeIterator++;

        while( mTreeIterator == mHash->mBuckets[mCurrent].end() )
        {
            mCurrent++;
            if(mCurrent == mHash->mBucketCount)
            {
                mHash = NULL;
                break;
            }
            else
            {
                mTreeIterator = mHash->mBuckets[mCurrent].begin();
            }
        }
    }

    return *this;
}

const iddRBHash::iterator& iddRBHash::iterator::operator=(
        const iddRBHash::iterator& aIter)
{
    mHash = aIter.mHash;
    mCurrent = aIter.mCurrent;
    mTreeIterator = aIter.mTreeIterator;

    return *this;
}

bool iddRBHash::iterator::operator==(const iterator& aIter)
{
    return ((mHash==NULL) && (aIter.mHash == NULL))
        ||
        ((mHash == aIter.mHash) &&
         (mCurrent == aIter.mCurrent) &&
         (mTreeIterator == aIter.mTreeIterator));
}

IDE_RC iddRBHash::initializeStatic(void)
{
    IDE_TEST( mGlobalMutex.initialize((SChar*)"IDD_HASH_GLOBAL_MUTEX",
                                      IDU_MUTEX_KIND_POSIX,
                                      IDV_WAIT_INDEX_NULL)
            != IDE_SUCCESS );

    mGlobalLink.mPrev = mGlobalLink.mNext = &mGlobalLink;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iddRBHash::destroyStatic(void)
{
    IDE_ASSERT(mGlobalLink.mPrev == &mGlobalLink);
    IDE_ASSERT(mGlobalLink.mNext == &mGlobalLink);

    IDE_TEST( mGlobalMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 해시 통계 정보 중 이름을 복사한다
 *
 * @param aStat : 통계 구조체
 */
void iddRBHash::fillHashStat(iddRBHashStat* aStat)
{
    idlOS::strncpy(aStat->mName, mName, 32);
}

/**
 * 각 버킷의 통계를 통계정보에 복사한다
 *
 * @param aStat : 통계 구조체
 */
void iddRBHash::fillBucketStat(iddRBHashStat* aStat, const UInt aBucketNo)
{
    IDE_ASSERT(aBucketNo < mBucketCount);
    mBuckets[aBucketNo].fillStat(aStat);
    aStat->mBucketNo = aBucketNo;
}

/**
 * 해시의 통계 정보를 초기화한다
 */
void iddRBHash::clearStat(void)
{
    UInt i;

    for(i = 0; i < mBucketCount; i++)
    {
        mBuckets[i].clearStat();
    }
}

static iduFixedTableColDesc gRBHashColDesc[] =
{
    {
        (SChar *)"NAME",
        IDU_FT_OFFSETOF(iddRBHashStat, mName),
        IDU_FT_SIZEOF(iddRBHashStat, mName),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"BUCKET_NO",
        IDU_FT_OFFSETOF(iddRBHashStat, mBucketNo),
        IDU_FT_SIZEOF(iddRBHashStat, mBucketNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"KEY_LENGTH",
        IDU_FT_OFFSETOF(iddRBHashStat, mKeyLength),
        IDU_FT_SIZEOF(iddRBHashStat, mKeyLength),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NODE_COUNT",
        IDU_FT_OFFSETOF(iddRBHashStat, mCount),
        IDU_FT_SIZEOF(iddRBHashStat, mCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SEARCH_COUNT",
        IDU_FT_OFFSETOF(iddRBHashStat, mSearchCount),
        IDU_FT_SIZEOF(iddRBHashStat, mSearchCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"INSERT_LEFT_MOVE",
        IDU_FT_OFFSETOF(iddRBHashStat, mInsertLeft),
        IDU_FT_SIZEOF(iddRBHashStat, mInsertLeft),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"INSERT_RIGHT_MOVE",
        IDU_FT_OFFSETOF(iddRBHashStat, mInsertRight),
        IDU_FT_SIZEOF(iddRBHashStat, mInsertRight),
        IDU_FT_TYPE_UBIGINT,
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

static IDE_RC buildRecordForRBHash(idvSQL      *,
                                   void        *aHeader,
                                   void        * /* aDumpObj */,
                                   iduFixedTableMemory *aMemory)
{
    iddRBHash*      sHash;
    iddRBHashLink*  sLink;
    iddRBHashStat   sStat;
    UInt            sBucketCount;
    UInt            i;

    IDE_TEST( iddRBHash::mGlobalMutex.lock(NULL) != IDE_SUCCESS );

    sLink = iddRBHash::mGlobalLink.mNext;
    while(sLink != &(iddRBHash::mGlobalLink))
    {
        sHash = (iddRBHash*)sLink;
        sBucketCount = sHash->getBucketCount();
        sHash->fillHashStat(&sStat);

        for( i = 0; i < sBucketCount; i++ )
        {
            sHash->fillBucketStat(&sStat, i);
            IDE_TEST( iduFixedTable::buildRecord(aHeader,
                            aMemory,
                            (void *) &sStat)
                        != IDE_SUCCESS );
        }

        sLink = sLink->mNext;
    }

    IDE_TEST( iddRBHash::mGlobalMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT( iddRBHash::mGlobalMutex.unlock() == IDE_SUCCESS );
    return IDE_FAILURE;
}

iduFixedTableDesc gRBHashTableDesc =
{
    (SChar *)"X$RBHASH",
    buildRecordForRBHash,
    gRBHashColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

