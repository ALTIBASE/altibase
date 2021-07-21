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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_IDU_SHM_HASH_H_
#define _O_IDU_SHM_HASH_H_ 1

#include <idl.h>

#include <idrLogMgr.h>
#include <iduShmList.h>
#include <iduShmMemPool.h>

//following structure will be refered from VLogShmHash
struct iduLtShmHashBase;
struct iduStShmHashBucket;
struct iduStShmHashBase;

#include <iduVLogShmHash.h>

typedef enum iduShmHashGenFuncID
{
    IDU_SHMHASH_ID_SMXTRANS_GEN_HASH_VALUE_FUNC = 0,
    IDU_SHMHASH_ID_SMXTABLEINFOMGR_HASHFUNC
} iduShmHashGenFuncID;

typedef enum iduShmHashCompFuncID
{
    IDU_SHMHASH_ID_SMXTRANS_COMPARE_FUNC = 0,
    IDU_SHMHASH_ID_SMXTABLEINFOMGR_COMPAREFUNC4HASH
} iduShmHashCompFuncID;

/*
 * Hash
 * 사용자는 해쉬 객체를 사용하기 위해 아래의 속성을 initialize()시 넘겨야 한다.
 *
 *  - 버켓 갯수   : 입력될 노드의 최대 갯수를 고려해 결정되어야 함.
 *  - 동시성 레벨 : 단일 객체가 이용하는 것인지, 다수 쓰레드가 동시에 이용하는
 *                  것인지에 따른 수치 (이 경우 일반적으로 CPU갯수 * 2)
 *  - Mutex를 사용할 것인지에 대한 Flag
 *  - 사용될 키의 길이
 *  - 키로부터 얻는 해쉬 함수
 *  - 키간의 비교 함수
 */

typedef struct iduStShmHashChain
{
    idShmAddr       mAddrSelf;
    iduShmListNode  mList;      /* for double-linked list  */
    idShmAddr       mAddrNode;  /* real Hash Target Node   */
    ULong           mKey[1];    /* 노드에 대한 키의 포인터가 저장 : 8로 align을  */
}iduStShmHashChain;

typedef struct iduStShmHashBucket
{
    idShmAddr       mAddrSelf;
    iduShmSXLatch      mLatch;     /* bucket에 대한 동시성 제어 */
    UInt            mCount;     /* 현재 bucket에 저장된 노드 갯수 */
    iduShmListNode  mBaseList;  /* double-link list의 base list */
}iduStShmHashBucket;

struct iduLtShmHashBase;

typedef struct iduShmHashLatchFunc
{
    IDE_RC (*findNode)  ( idvSQL             *aStatistics,
                          iduShmTxInfo       *aShmTxInfo,
                          iduLtShmHashBase   *aBase,
                          iduStShmHashBucket *aBucket,
                          void               *aKeyPtr,
                          void              **aNode );

    IDE_RC (*insertNode)( idvSQL             *aStatistics,
                          iduShmTxInfo       *aShmTxInfo,
                          iduLtShmHashBase   *aBase,
                          iduStShmHashBucket *aBucket,
                          void               *aKeyPtr,
                          idShmAddr           aAddr4Node,
                          void               *aNode );

    IDE_RC (*deleteNode)( idvSQL             *aStatistics,
                          iduShmTxInfo       *aShmTxInfo,
                          idrSVP             *aVSavepoint,
                          iduLtShmHashBase   *aBase,
                          iduStShmHashBucket *aBucket,
                          void               *aKeyPtr,
                          void              **aNode );
} iduShmHashLatchFunc;

typedef struct iduStShmHashBase
{
    idShmAddr          mAddrSelf;
    iduShmLatch        mLatch;          /* Hash 전체의 Latch */

    iduStShmMemPool    mShmMemPool;     /* Chain의 메모리 관리자 :
                                           Struct내부 Class 사용 불가. so, void */
    UInt               mKeyLength;
    UInt               mBucketCount;
    idShmAddr          mAddrBucketLst;  /* Bucket List */

    /* For Traverse  */
    idBool             mOpened;         /* Traverse의 Open 유무   */
    UInt               mCurBucket;      /* 접근중인   Bucket 번호  */
    idShmAddr          mAddrCurChain;   /* 접근 중인  Chain Addr  */

    /* fix BUG-21311 */
    idBool             mDidAllocChain;

    /* For logical logging */
    iduShmHashGenFuncID   mHashFuncID;
    iduShmHashCompFuncID  mCompFuncID;
    idBool                mUseLatch;
    UInt                  mHashTableSize;
} iduStShmHashBase;

typedef UInt (*iduShmHashGenFunc)(iduStShmHashBase * aStHashInfo, void *);
typedef SInt (*iduShmHashCompFunc)(void *, void *);

typedef struct iduLtShmHashBase
{
    iduStShmHashBase       * mStShmHashBase;
    iduShmHashLatchFunc    * mLatchVector;
    iduStShmMemPool        * mStShmMemPool;
    iduStShmHashBucket     * mBucket;
    iduShmHashGenFunc        mHashFunc;    /* HASH 함수 : callback */
    iduShmHashCompFunc       mCompFunc;    /* 비교 함수 : callback */
} iduLtShmHashBase;

class iduShmHash
{
public:
    static IDE_RC initialize( idvSQL               *aStatistics,
                              iduShmTxInfo         *aShmTxInfo,
                              UInt                  aConcurrentLevel,/* > 0 */
                              UInt                  aBucketCount,
                              UInt                  aKeyLength,
                              idBool                aUseLatch,
                              iduShmHashGenFuncID   aHashFuncID,
                              iduShmHashCompFuncID  aCompFuncID,
                              idShmAddr             aAddrStShmHashBase,
                              iduStShmHashBase    * aStShmHashBase,
                              iduLtShmHashBase    * aBase );

    static IDE_RC destroy( idvSQL            * aStatistics,
                           iduShmTxInfo      * aShmTxInfo,
                           iduLtShmHashBase  * aBase );

    static IDE_RC attach( idBool                aUseLatch,
                          iduStShmHashBase    * aStShmHashBase,
                          iduLtShmHashBase    * aLtHashBase );

    static IDE_RC detach( iduLtShmHashBase * aLtHashBase );

    static IDE_RC reset( idvSQL            * aStatistics,
                         iduShmTxInfo      * aShmTxInfo,
                         iduLtShmHashBase  * aBase );

    static IDE_RC findNode( idvSQL            * aStatistics,
                            iduShmTxInfo      * aShmTxInfo,
                            iduLtShmHashBase  * aBase,
                            void              * aKeyPtr,
                            void             ** aNode );

    static IDE_RC insertNode( idvSQL            * aStatistics,
                              iduShmTxInfo      * aShmTxInfo,
                              iduLtShmHashBase  * aBase,
                              void              * aKeyPtr,
                              idShmAddr           aAddrNode,
                              void              * aNode );

    static IDE_RC deleteNode( idvSQL            * aStatistics,
                              iduShmTxInfo      * aShmTxInfo,
                              idrSVP            * aVSavepoint,
                              iduLtShmHashBase  * aBase,
                              void              * aKeyPtr,
                              void             ** aNode );

    /*
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 주의 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *
     *  lock(), unlock() 함수는 findNode(), deleteNode(), insertNode() 등의
     *  연산함수와 무관하기 때문에
     *  lock(), unlock() 함수를 사용한다고 해서 이러한
     *  연산함수의 동작을 제어할 수 있다고 생각해서는 안된다!!!!
     *  이 함수의 목적은 외부에서 어떠한 의도로 사용되는 Mutex 의미를 위한
     *  서비스 함수 이상 아무 것도 아니다.
     */
    static void lock( idvSQL            * aStatistics,
                      iduShmTxInfo      * aShmTxInfo,
                      iduLtShmHashBase  * aBase );

    static void unlock( iduShmTxInfo *aShmTxInfo, iduLtShmHashBase  * aBase );

    /*
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 주의 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *
     *  아래의 함수를 호출할 경우에는 아무런 Concurrency Control을
     *  하지 않기 때문에, 호출자는 아래의 cut함수의 사용에 있어서
     *  조심해야 한다.
     *  만일, open(), cutNode() 과정에서 다른 쓰레드가 Insert 혹은 delete를
     *  한다면, 예기치 못한 상황이 발생할 수 있으므로,
     *  멤버 함수 lock(), unlock()을 이용해서 외부 호출 영역에서 명시적으로
     *  모든 접근 함수에 대한 처리를 해 주어야 한다.
     */

    // 내부 Traverse & Node 제거 작업
    // Fix BUG-21311
    static inline IDE_RC open( iduShmTxInfo     * aShmTxInfo,
                               iduLtShmHashBase * aBase );
    static inline IDE_RC cutNode( idvSQL           * aStatistics,
                                  iduShmTxInfo     * aShmTxInfo,
                                  iduLtShmHashBase * aBase,
                                  void **aNode );
    static inline IDE_RC close( iduShmTxInfo     *aShmTxInfo,
                                iduLtShmHashBase *aBase );
    static IDE_RC isEmpty( iduShmTxInfo     *aShmTxInfo,
                           iduLtShmHashBase *aBase,
                           idBool           *aIsEmpty );

    static IDE_RC deleteNodeNoLatch( idvSQL              * aStatistics,
                                     iduShmTxInfo        * aShmTxInfo,
                                     idrSVP              * aVSavepoint,
                                     iduLtShmHashBase    * aBase,
                                     iduStShmHashBucket  * aBucket,
                                     void                * aKeyPtr,
                                     void               ** aNode );

    static iduShmHashGenFunc   mArrHashFunction[2];
    static iduShmHashCompFunc  mArrCompFunction[2];

    static UInt genHashValueFunc( iduStShmHashBase * aStHashInfo, void * aData );
    static UInt hashFunc( iduStShmHashBase * aStHashInfo, void* aKeyPtr );
    static SInt compareFunc( void* aLhs, void* aRhs );
    static SInt compareFunc4Hash( void* aLhs, void* aRhs );

private:
    static iduShmHashLatchFunc mLatchVector[2];
    static iduStShmHashChain* findNodeInternal( iduLtShmHashBase   * aBase,
                                                iduStShmHashBucket * aBucket,
                                                void               * aKeyPtr );

    static IDE_RC findNodeLatch( idvSQL             * aStatistics,
                                 iduShmTxInfo       * aShmTxInfo,
                                 iduLtShmHashBase   * aBase,
                                 iduStShmHashBucket * aBucket,
                                 void               * aKeyPtr,
                                 void              ** aNode );

    static IDE_RC insertNodeLatch( idvSQL             * aStatistics,
                                   iduShmTxInfo       * aShmTxInfo,
                                   iduLtShmHashBase   * aBase,
                                   iduStShmHashBucket * aBucket,
                                   void               * aKeyPtr,
                                   idShmAddr            aAddr4Node,
                                   void               * aNode );

    static IDE_RC deleteNodeLatch( idvSQL             * aStatistics,
                                   iduShmTxInfo        * aShmTxInfo,
                                   idrSVP              * aVSavepoint,
                                   iduLtShmHashBase    * aBase,
                                   iduStShmHashBucket  * aBucket,
                                   void                * aKeyPtr,
                                   void               ** aNode );

    static IDE_RC findNodeNoLatch( idvSQL              * aStatistics,
                                   iduShmTxInfo        * aShmTxInfo,
                                   iduLtShmHashBase    * aBase,
                                   iduStShmHashBucket  * aBucket,
                                   void                * aKeyPtr,
                                   void               ** aNode );

    static IDE_RC insertNodeNoLatch( idvSQL              * aStatistics,
                                     iduShmTxInfo        * aShmTxInfo,
                                     iduLtShmHashBase    * aBase,
                                     iduStShmHashBucket  * aBucket,
                                     void                * aKeyPtr,
                                     idShmAddr             aAddr4Node,
                                     void                * aNode );

    static IDE_RC allocChain( idvSQL                * aStatistics,
                              iduShmTxInfo          * aShmTxInfo,
                              iduLtShmHashBase      * aBase,
                              iduStShmHashChain    ** aChain );

    static IDE_RC freeChain( idvSQL             * aStatistics,
                             iduShmTxInfo       * aShmTxInfo,
                             idrSVP             * aSavepoint,
                             iduLtShmHashBase   * aBase,
                             iduStShmHashChain  * aChain );

    static iduStShmHashChain *searchFirstNode( iduLtShmHashBase * aBase );
    static iduStShmHashChain *searchNextNode ( iduLtShmHashBase * aBase );

};

//fix BUG-21311 transform inline function.
inline IDE_RC iduShmHash::open( iduShmTxInfo     * /*aShmTxInfo*/,
                                iduLtShmHashBase * aBase )
{
    iduStShmHashChain   * sCurChainPtr;
    iduStShmHashBase    * sStHashBase;

    sStHashBase = aBase->mStShmHashBase;

    IDE_ASSERT( sStHashBase->mOpened == ID_FALSE );

    sStHashBase->mOpened    = ID_TRUE;
    sStHashBase->mCurBucket = 0; // before search

    if( sStHashBase->mDidAllocChain == ID_TRUE )
    {
        sCurChainPtr = searchFirstNode( aBase );

        if( sCurChainPtr != NULL )
        {
            sStHashBase->mAddrCurChain = sCurChainPtr->mAddrSelf;
        }
        else
        {
            sStHashBase->mAddrCurChain = IDU_SHM_NULL_ADDR;
        }
    }
    else
    {
        //empty
        sStHashBase->mAddrCurChain = IDU_SHM_NULL_ADDR;
    }

    return IDE_SUCCESS;
}

IDE_RC iduShmHash::cutNode( idvSQL           * aStatistics,
                            iduShmTxInfo     * aShmTxInfo,
                            iduLtShmHashBase * aBase,
                            void            ** aNode )
{
    iduStShmHashChain   * sDeleteChain;
    iduStShmHashChain   * sNxtChain;
    iduStShmHashBase    * sStHashBase;
    idrSVP                sSavepoint;

    sStHashBase = aBase->mStShmHashBase;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    if( sStHashBase->mAddrCurChain != IDU_SHM_NULL_ADDR )
    {
        // delete current node & return the node pointer
        sDeleteChain =
            (iduStShmHashChain*)IDU_SHM_GET_ADDR_PTR( sStHashBase->mAddrCurChain );
        *aNode       =
            (void*)IDU_SHM_GET_ADDR_PTR( sDeleteChain->mAddrNode );

        // find next
        sNxtChain = searchNextNode( aBase );

        iduVLogShmHash::writeCutNode( aShmTxInfo, sStHashBase );

        if( sNxtChain == NULL )
        {
            sStHashBase->mAddrCurChain = IDU_SHM_NULL_ADDR;
        }
        else
        {
            sStHashBase->mAddrCurChain = sNxtChain->mAddrSelf;
        }

        iduShmList::remove( aShmTxInfo, &sDeleteChain->mList );

        IDE_TEST( freeChain( aStatistics,
                             aShmTxInfo,
                             &sSavepoint,
                             aBase,
                             sDeleteChain ) != IDE_SUCCESS );
    }
    else
    {
        *aNode = NULL;
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     aShmTxInfo,
                                     &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                      aShmTxInfo,
                                      &sSavepoint ) == IDE_SUCCESS );

    return IDE_FAILURE;
}

inline IDE_RC iduShmHash::close( iduShmTxInfo     * /*aShmTxInfo*/,
                                 iduLtShmHashBase * aBase )
{
    IDE_ASSERT( aBase->mStShmHashBase->mOpened == ID_TRUE );

    aBase->mStShmHashBase->mOpened = ID_FALSE;

    return IDE_SUCCESS;
}

#endif  // _O_SCU_HASH_H_


