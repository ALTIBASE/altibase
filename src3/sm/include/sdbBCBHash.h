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
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ***********************************************************************/

#ifndef _O_SDB_BCB_HASH_H_
#define _O_SDB_BCB_HASH_H_ 1

#include <idl.h>
#include <idu.h>
#include <smDef.h>
#include <smuProperty.h>
#include <sdbBCB.h>
#include <smuList.h>

/***********************************************************************
 * Description :
 *  sdbBCBHash bucket의 자료구조. 실제 hash table은  이것들의 배열로 되어있다.
 *  Bucket에는 연결 리스트를 이용해 같은 hash key를 가진 것들을 연결한다.
 *  이것을 hash chain 이라고 한다.
 ***********************************************************************/
typedef struct sdbBCBHashBucket
{
    /* hash chain의 길이 */
    UInt    mLength;
    /* hash chain의 mBase */
    smuList mBase;

    UInt    mHashBucketNo;
} sdbBCBHashBucket;

typedef iduLatch sdbHashChainsLatchHandle;

class sdbBCBHash
{
public:
    IDE_RC  initialize( UInt         aBucketCnt,
                        UInt         aBucketCntPerLatch,
                        sdLayerState aType );

    IDE_RC  destroy();

    /*현재 hash의 크기및 latch비율을 변경. 본 연산이 수행되는 도중에
     *다른 트랜잭션에의한 현재 hash에 대한 접근은 허용.*/
    IDE_RC  resize( UInt aBucketCnt,
                    UInt aBucketCntPerLatch );

    void insertBCB( void   * aTargetBCB,
                    void  ** aAlreadyExistBCB );
    
    void removeBCB( void  * aTargetBCB );

    IDE_RC findBCB( scSpaceID    aSpaceID,
                    scPageID     aPID,
                    void      ** aRetBCB );

    inline sdbHashChainsLatchHandle *lockHashChainsSLatch( idvSQL    *aStatistics,
                                                           scSpaceID  aSpaceID,
                                                           scPageID   aPID );

    inline sdbHashChainsLatchHandle *lockHashChainsXLatch( idvSQL    *aStatistics,
                                                           scSpaceID  aSpaceID,
                                                           scPageID   aPID );

    inline void unlockHashChainsLatch( sdbHashChainsLatchHandle *aMutex );
    
    inline UInt getBucketCount();

    inline UInt getLatchCount();

    UInt getBCBCount();

private:
    inline UInt hash( scSpaceID aSpaceID,
                      scPageID  aPID );

    void moveDataToNewHash( sdbBCBHash *aNewHash );

    /*이 함수 수행중에 트랜잭션이 hash에 접근하면 안된다.
     *즉, 이 함수를 호출하기 전 반드시 BufferManager의 래치가 x로 잡혀 있어야 한다.*/
    static void exchangeHashContents( sdbBCBHash *aHash1, sdbBCBHash *aHash2 );

    /* hash chains latch의 갯수 */
    UInt mLatchCnt;
    
    /* bucket의 갯수, 즉 mTable의 배열의 size */
    UInt mBucketCnt;
    
    /* 0x000111 와 같은 형태를 가진다.
     * bucket index 값을 mLatchMask와 &(and) 연산하면 latch index가 나온다.  */
    UInt mLatchMask;
    
    /* 현재 hash chains latch는 성능이유로 mutex로 구현되어 동작한다.
     * 이 hash chains latch를 배열형태로 모아놓은 것이 mMutexArray이다.*/
    iduLatch      *mMutexArray;
    
    /* 실제 BCB를 유지하는 테이블. sdbBCBHashBucket의 배열형태로
     * 구현되어 있다. */
    sdbBCBHashBucket *mTable;
};

/****************************************************************
 * Description:
 *      pageID를 입력으로 받아서 key를 리턴하는 함수
 *
 * aSpaceID - [IN]  spaceID
 * aPID     - [IN]  pageID 
 *****************************************************************/
UInt sdbBCBHash::hash(scSpaceID aSpaceID,
                      scPageID  aPID)
{

    /* BUG-32750 [sm-disk-resource] The hash bucket numbers of BCB(Buffer
     * Control Block) are concentrated in low number.
     * 기존 ( FID + SPACEID + FPID ) % HASHSIZE 는 각 DataFile의 크기가 작을
     * 경우 앞쪽으로 HashValue가 몰리는 단점이 있다. 이에 따라 다음과 같이
     * 수정한다
     * ( ( SPACEID * PERMUTE1 +  FID ) * PERMUTE2 + PID ) % HASHSIZE
     * PERMUTE1은 Tablespace가 다를때 값이 어느정도 간격이 되는가,
     * PERMUTE2는 Datafile FID가 다를때 값이 어느정도 간격이 되는가,
     * 입니다.
     * PERMUTE1은 Tablespace당 Datafile 평균 개수보다 조금 작은 값이 적당하며
     * PERMUTE2은 Datafile당 Page 평균 개수보다 조금 작은 값이 적당합니다. */
    return ( ( aSpaceID * smuProperty::getBufferHashPermute1()
                    + SD_MAKE_FID(aPID) )
            * smuProperty::getBufferHashPermute2() + aPID ) % mBucketCnt ;
}

sdbHashChainsLatchHandle *sdbBCBHash::lockHashChainsSLatch(
        idvSQL    *aStatistics,
        scSpaceID  aSpaceID,
        scPageID   aPID)
{
    iduLatch *sMutex;

    sMutex = &mMutexArray[hash(aSpaceID, aPID) & mLatchMask];
    (void)sMutex->lockRead(aStatistics, NULL);
    return (sdbHashChainsLatchHandle*)sMutex;
}

sdbHashChainsLatchHandle *sdbBCBHash::lockHashChainsXLatch(
        idvSQL    *aStatistics,
        scSpaceID  aSpaceID,
        scPageID   aPID)
{
    iduLatch *sMutex;

    sMutex = &mMutexArray[hash(aSpaceID, aPID) & mLatchMask];
    (void)sMutex->lockWrite(aStatistics, NULL);
    return (sdbHashChainsLatchHandle*)sMutex;
}

void sdbBCBHash::unlockHashChainsLatch(sdbHashChainsLatchHandle *aMutex)
{
    (void)aMutex->unlock();
}

UInt sdbBCBHash::getBucketCount()
{
    return mBucketCnt;
}

UInt sdbBCBHash::getLatchCount()
{
    return mLatchCnt;
}

#endif // _O_SDB_BCB_HASH_H_
