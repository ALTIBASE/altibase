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
 * $$Id:$
 **********************************************************************/

/****************************************************************
 * Description :
 *
 *
 * BCB Buffer Hash (PROJ-1568. SM - Buffer manager renewal)
 *
 * 알티베이스에서 공통으로 사용하는 hash table을 사용하지 않고,
 * 새로운 hash를 만들어서 오직 buffer manager에서만 사용하게 함.
 * 
 *
 * #특징
 *      - 해시 테이블 크기를 동적으로 변경할 수 있다.
 *      - 여러 bucket에 하나의 mutex를 매핑시켜 사용할 수 있다.
 *      - 해시 테이블의 크기를 최소한으로 유지한다.
 *      - BCB에 특화 시켰기 때문에 속도가 더 빠름.
 * 
 *****************************************************************/

#include <smDef.h>
#include <sdbBCBHash.h>
#include <smErrorCode.h>



/***********************************************************************
 * Description :
 *  특정 숫자를 입력으로 받아 이 숫자 보다 큰 2의 제곱 수를 리턴한다.
 *  즉, 입력이 3이면 4를 리턴하고, 입력이 5이면 8을 리턴한다.
 *
 *  aNum         - [IN] 입력 숫자
 ***********************************************************************/
static UInt makeSqure2(UInt aNum)
{
    UInt i;
    UInt sNum = aNum;

    for (i = 1; sNum != 1; i++)
    {
        sNum >>= 1;
    }

    for (; i != 0; i--)
    {
        sNum <<= 1;
    }

    return sNum;
}

/***********************************************************************
 * Description:
 * 
 *  aBucketCnt         - [IN]  해시가 유지할 총 bucket 개수
 *  aBucketCntPerLatch - [IN]  하나의 HashChainsLacth가 관리하는 bucket의 갯수
 *  aType              - [IN]  BufferMgr or Secondary BufferMgr에서 호출되었는지
 ***********************************************************************/ 
IDE_RC sdbBCBHash::initialize( UInt           aBucketCnt,
                               UInt           aBucketCntPerLatch,
                               sdLayerState   aType )
{
    SInt   sState = 0;
    UInt   i;
    SChar  sMutexName[128];

    /* mLatchMask 설정
     * bucket에 해당하는 mutex를 구할때 사용하는 변수를 설정한다.
     * mLatchArray[key & mLatchMask] 를 통해서 해당 래치를 구한다. */
    IDE_ASSERT(aBucketCntPerLatch != 0);
    
    mBucketCnt = aBucketCnt;
    mLatchCnt  = (mBucketCnt + aBucketCntPerLatch - 1) / aBucketCntPerLatch;

    if (mLatchCnt == mBucketCnt)
    {
        /* 아래와 같이 mask값을 정하면 버킷과 같은 값이 바로 나온다.
         * 즉, bucket과 latch가 1:1로 매핑된다. */
        mLatchMask = ID_UINT_MAX;
    }
    else
    {
        /* aLatchRatio는 반드시 1또는 2의 배수여야 한다.
         * 2의 배수인 경우에 아래의 assert는 반드시 통과된다.
         *
         * aLatchRatio는 반드시 0010000 이와 같은 형식이고,( 1이 오직 한개 )
         * 이 값에서 1을 빼면    0001111 과 같이 되므로 원하는 mask를 얻을 수 있다.
         * 예를 들어보면, mask가 0x0011이고 key가 0x1010인경우,
         * key가 0x1110, 0x0110, 0x0010인 것들과 같은 latch를 사용한다. */
        mLatchCnt = makeSqure2(mLatchCnt);
        mLatchMask = mLatchCnt - 1;

        /*aLatchRatio는 반드시 2의 제곱수여야 한다.*/
        IDE_ASSERT((mLatchMask & mLatchCnt) == 0);
    }

    /* TC/FIT/Limit/sm/sdb/sdbBCBHash_initialize_malloc1.sql */
    IDU_FIT_POINT_RAISE( "sdbBCBHash::initialize::malloc1",
                          insufficient_memory );


    /* mTable 설정 */
    /*BUG-30439  102GB 이상 BUFFER_AREA_SIZE를 할당할 경우, Mutex할당 계산 
                      오류로 메모리 할당을 잘못할 수 있습니다. */
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)ID_SIZEOF( sdbBCBHashBucket ) 
                                         * (mBucketCnt),
                                     (void**)&mTable) != IDE_SUCCESS,
                   insufficient_memory );
    
    idlOS::memset(mTable,
                  0x00,
                  (size_t)ID_SIZEOF( sdbBCBHashBucket ) * (mBucketCnt));
    sState = 1;

    /* TC/FIT/Limit/sm/sdb/sdbBCBHash_initialize_malloc2.sql */
    IDU_FIT_POINT_RAISE( "sdbBCBHash::initialize::malloc2",
                          insufficient_memory );

    /* mMutexArray 설정 */
    /*BUG-30439  102GB 이상 BUFFER_AREA_SIZE를 할당할 경우, Mutex할당 계산 
                      오류로 메모리 할당을 잘못할 수 있습니다. */
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     (ULong)ID_SIZEOF(iduLatch) 
                                         * mLatchCnt,
                                     (void**)&mMutexArray) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 2;

    for (i = 0; i < mBucketCnt; i++)
    {
        SMU_LIST_INIT_BASE(&mTable[i].mBase);
        mTable[i].mHashBucketNo = i;
    }

    if( aType == SD_LAYER_BUFFER_POOL )
    {
        for (i = 0; i < mLatchCnt; i++)
        {
            idlOS::snprintf( sMutexName,
                             ID_SIZEOF(sMutexName), 
                             "HASH_LATCH_%"ID_UINT32_FMT, i);

            IDE_ASSERT( mMutexArray[i].initialize( sMutexName) 
                        == IDE_SUCCESS);
        }
    }
    else
    {
        for (i = 0; i < mLatchCnt; i++)
        {
            idlOS::snprintf( sMutexName, 
                             ID_SIZEOF(sMutexName), 
                             "SECONDARY_BUFFER_HASH_LATCH_%"ID_UINT32_FMT, i );

            IDE_ASSERT( mMutexArray[i].initialize( sMutexName) 
                        == IDE_SUCCESS);
        }
    }

    // [주의]
    // 이 라인 아래에 IDE_TEST 구문을 추가하려면
    // EXCEPTION 처리 루틴에서 mMutexArray를 mLatchCnt만큼 돌면서 destroy해야 한다.

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            IDE_ASSERT(iduMemMgr::free(mMutexArray) == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(iduMemMgr::free(mTable) == IDE_SUCCESS);
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  제거 함수
 ***********************************************************************/
IDE_RC sdbBCBHash::destroy()
{
    UInt i;

    for (i = 0; i < mLatchCnt; i++)
    {
        IDE_TEST(mMutexArray[i].destroy() != IDE_SUCCESS);
    }
    IDE_TEST(iduMemMgr::free(mMutexArray) != IDE_SUCCESS);

    IDE_TEST(iduMemMgr::free(mTable) != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  BCB를 hash에 삽입. 삽입하는 BCB와 같은 (pid,spaceID)를 가진 BCB가 있다면 그
 *  BCB를 리턴
 *  
 *  aTargetBCB        - [IN]  삽입할 BCB
 *  aAlreadyExistBCB  - [OUT] 같은 (pid, spaceID)를 가진 BCB가 있다면,
 *                              삽입을 하는대신 이 BCB를 리턴한다.
 ***********************************************************************/
void sdbBCBHash::insertBCB( void  * aTargetBCB,
                            void ** aAlreadyExistBCB )
{
    sdBCB              *sBCB;
    smuList            *sListNode;
    smuList            *sBaseNode;
    sdbBCBHashBucket   *sBucket;
    scSpaceID           sSpaceID;
    scPageID            sPID;

    IDE_DASSERT( aTargetBCB != NULL );

    sSpaceID  = SD_GET_BCB_SPACEID( aTargetBCB );
    sPID      = SD_GET_BCB_PAGEID( aTargetBCB );
    sBucket   = &( mTable[hash(sSpaceID, sPID)] );
    sBaseNode = &sBucket->mBase;    
    sListNode = SMU_LIST_GET_FIRST( sBaseNode );
    
    while (sListNode != sBaseNode)
    {
        sBCB = (sdBCB*)sListNode->mData;
        if ((sBCB->mPageID == sPID) && (sBCB->mSpaceID == sSpaceID))
        {
            /*이미 같은 pid를 가진 BCB가 hash에 존재하는 경우*/
            break;
        }
        else
        {
            sListNode = SMU_LIST_GET_NEXT( sListNode );
        }
    }

    if( sListNode != sBaseNode )
    {
        *aAlreadyExistBCB = sBCB;
    }
    else
    {
        *aAlreadyExistBCB = NULL;
        SMU_LIST_ADD_FIRST( sBaseNode, 
                            SD_GET_BCB_HASHITEM( aTargetBCB ) );
        sBucket->mLength++;
    }

    SD_GET_BCB_HASHBUCKETNO( aTargetBCB ) = sBucket->mHashBucketNo;
}

/***********************************************************************
 * Description:
 *  BCB를 hash에서 삭제한다. 반드시 삭제할 BCB가 hash에 존재해야 한다. 그렇지
 *  않으면 서버 사망
 *
 * aTargetBCB   - [IN]  제거할 BCB  
 ***********************************************************************/
void sdbBCBHash::removeBCB( void *aTargetBCB )
{
    sdbBCBHashBucket  * sBucket;
    scSpaceID           sSpaceID;
    scPageID            sPageID;

    IDE_DASSERT( aTargetBCB != NULL );

    sSpaceID  = SD_GET_BCB_SPACEID( aTargetBCB );
    sPageID   = SD_GET_BCB_PAGEID( aTargetBCB );
 
    SMU_LIST_DELETE( SD_GET_BCB_HASHITEM( aTargetBCB ) );
    SDB_INIT_HASH_CHAIN_LIST( aTargetBCB );

    sBucket = &mTable[hash( sSpaceID, sPageID )];
    IDE_ASSERT( sBucket->mLength > 0 );
    sBucket->mLength--;

    SD_GET_BCB_HASHBUCKETNO( aTargetBCB ) = 0;
}

/***********************************************************************
 * Description:
 *  aSpaceID와 aPID에 해당하는 BCB를 얻고자 수행하는 함수, Hash에 BCB가 존재할
 *  경우엔 그 BCB를 리턴한다. 없으면 NULL을 리턴 
 *
 *  aSpaceID    - [IN]  table space ID
 *  aPID        - [IN]  page ID
 ***********************************************************************/
IDE_RC sdbBCBHash::findBCB( scSpaceID    aSpaceID,
                            scPageID     aPID,
                            void      ** aRetBCB )
{
    sdBCB             *sBCB = NULL;
    smuList           *sListNode;
    smuList           *sBaseNode;
    sdbBCBHashBucket  *sBucket;

    *aRetBCB  = NULL;
    sBucket   = &(mTable[hash( aSpaceID, aPID )]);
    sBaseNode = &sBucket->mBase;
    sListNode = SMU_LIST_GET_FIRST( sBaseNode );

    IDU_FIT_POINT_RAISE( "1.BUG-44102@sdbBCBHash::findBCB::invalidBCB",
                         ERR_INVALID_BCB );

    while ( sListNode != sBaseNode )
    {
        if ( sListNode != NULL )
        {
            sBCB = (sdBCB*)sListNode->mData;

            if ( sBCB != NULL )
            {
          
                if ( (sBCB->mPageID == aPID) && (sBCB->mSpaceID == aSpaceID) )
                {
                    break;
                }
                else
                {
                    sListNode = SMU_LIST_GET_NEXT(sListNode);
                }
            }
            else
            {
                IDE_RAISE( ERR_INVALID_BCB );
            }
        }
        else
        {
            IDE_RAISE( ERR_INVALID_BCB );
        }
    } //end of while

    if ( sListNode != sBaseNode )
    {
        *aRetBCB = sBCB;
    }
    else
    {
        *aRetBCB = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_BCB );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidBCB, 
                                  aSpaceID, 
                                  aPID ) );
        
        ideLog::log( IDE_ERR_0,
                     SM_TRC_INVALID_BCB,
                     aSpaceID, 
                     aPID,
                     sListNode,
                     sBCB );

        if ( sListNode != NULL )
        {
           ideLog::log( IDE_ERR_0,
                        SM_TRC_INVALID_BCB_LIST,
                        sListNode,
                        SMU_LIST_GET_PREV(sListNode),
                        SMU_LIST_GET_NEXT(sListNode),
                        (sdbBCB*)sListNode->mData );
        }
        else
        {
            /* nothing to do */
            ideLog::log( IDE_ERR_0, "null ListNode" );
        }

        if ( sBCB != NULL )
        {
            sdbBCB::dump( sBCB );
        }
        else
        {
            ideLog::log( IDE_ERR_0, "null BCB" );
        }
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  현재 가진 모든 BCB를 aNewHash로 옮긴다.
 *  이 함수 수행 중 절대 hash에 접근해서는 안된다.
 *
 *  aNewHash    - [IN]  이 hashTable로 모든 BCB를 옮긴다.
 ***********************************************************************/
void sdbBCBHash::moveDataToNewHash(sdbBCBHash *aNewHash)
{
    sdbBCBHashBucket   * sWorkTable;
    sdbBCBHashBucket   * sBucket;
    sdBCB              * sAlreadyExistBCB;
    smuList            * sBaseNode;
    smuList            * sListNode;
    UInt                 i;

    /* BUG-20861 버퍼 hash resize를 하기 위해서 다른 트랜잭션들을 모두 접근하지
     * 못하게 해야 합니다.
     * data move중에 트랜잭션이 접근한다면, 잘못된 정보를 볼 수 있으므로,
     * 이것은 절대 일어나지 말아야 한다.
     * 그렇기 때문에 mTable을 NULL로 해서, 접근하는 트랜잭션이 있다면
     * 세그먼트 에러를 발생하게 해서 서버를 죽인다.
     */
    sWorkTable = mTable;
    mTable = NULL;
    /* sWorkTable의 각 bucket단위로 move를 수행한다. 한번 move를 수행하고
     * 난 bucket에는 절대로 삽입이 일어날 수 없다.*/
    for (i = 0; i < mBucketCnt; i++)
    {
        sBucket   = &sWorkTable[i];
        sBaseNode = &sBucket->mBase;
        sListNode = SMU_LIST_GET_FIRST(sBaseNode);
 
        while (sListNode != sBaseNode)
        {
            SMU_LIST_DELETE(sListNode);
            sBucket->mLength--;
            SDB_INIT_HASH_CHAIN_LIST((sdBCB*)sListNode->mData);

            aNewHash->insertBCB( sListNode->mData,
                                 (void**)&sAlreadyExistBCB);

            IDE_ASSERT(sAlreadyExistBCB == NULL);

            sListNode = SMU_LIST_GET_FIRST(sBaseNode);
        }
    }
    mTable = sWorkTable;
    
    IDE_ASSERT( getBCBCount() == 0 );
}

/************************************************************************
 * Description: 해시테이블의 크기 및 latch당 bucket갯수
 ************************************************************************/
IDE_RC sdbBCBHash::resize( UInt aBucketCnt,
                           UInt aBucketCntPerLatch )
{
    sdbBCBHash sTempHash;

    IDE_TEST(sTempHash.initialize( aBucketCnt, 
                                   aBucketCntPerLatch, 
                                   SD_LAYER_BUFFER_POOL )
             != IDE_SUCCESS);

    moveDataToNewHash(&sTempHash);

    sdbBCBHash::exchangeHashContents(this, &sTempHash);

    IDE_TEST(sTempHash.destroy() != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/***********************************************************************
 * Description:     두 해시 테이블간의 내용을 변경한다.(swap)
 *  주의!! 반드시 BufferManager global x latch가 잡힌 상태에서 불려야 한다.
 *  즉, 두 Hash에 대한 접근이 전혀 없는 상태에서 이 함수가 불려야 한다.
 *
 *  aHash1  - [IN]  변경할 sdbBCBHash
 *  aHash2  - [IN]  변경할 sdbBCBHash
 ***********************************************************************/
void sdbBCBHash::exchangeHashContents(sdbBCBHash *aHash1, sdbBCBHash *aHash2)
{
    UInt              sTempBucketCnt;
    UInt              sTempLatchCnt;
    UInt              sTempLatchMask;
    iduLatch         *sTempMutexArray;
    sdbBCBHashBucket *sTempTable;

    sTempBucketCnt       = aHash1->mBucketCnt;
    aHash1->mBucketCnt   = aHash2->mBucketCnt;
    aHash2->mBucketCnt   = sTempBucketCnt;

    sTempLatchCnt        = aHash1->mLatchCnt;
    aHash1->mLatchCnt    = aHash2->mLatchCnt;
    aHash2->mLatchCnt    = sTempLatchCnt;
    
    sTempLatchMask       = aHash1->mLatchMask;
    aHash1->mLatchMask   = aHash2->mLatchMask;
    aHash2->mLatchMask   = sTempLatchMask;
    
    sTempMutexArray      = aHash1->mMutexArray;
    aHash1->mMutexArray  = aHash2->mMutexArray;
    aHash2->mMutexArray  = sTempMutexArray;

    sTempTable           = aHash1->mTable;
    aHash1->mTable       = aHash2->mTable;
    aHash2->mTable       = sTempTable;
}

/***********************************************************************
 * Description :
 *  hash table내에 존재하는 모든 BCB갯수 합 리턴. 래치를 잡지 않으므로,
 *  정확한 값은 아니다.
 ***********************************************************************/
UInt sdbBCBHash::getBCBCount()
{
    UInt sRet = 0;
    UInt i;

    for (i = 0; i < mBucketCnt; i++)
    {
        sRet += mTable[i].mLength;
    }

    return sRet;
}

