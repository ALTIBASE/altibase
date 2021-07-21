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
 * $Id: smuHash.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMU_HASH_H_
#define _O_SMU_HASH_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smuList.h>
#include <iduLatch.h>
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


typedef struct smuHashChain
{
    smuList  mList; /* for double-linked list  */
    void    *mNode; /* real Hash Target Node   */
    ULong    mKey[1];  /* 노드에 대한 키의 포인터가 저장 : 8로 align을  */
}smuHashChain;

typedef struct smuHashBucket
{
    iduLatch     * mLock;      /* bucket에 대한 동시성 제어 */
    UInt           mCount;     /* 현재 bucket에 저장된 노드 갯수 */
    smuList        mBaseList;  /* double-link list의 base list */
}smuHashBucket;

typedef UInt (*smuHashGenFunc)(void *);
typedef SInt (*smuHashCompFunc)(void *, void *);

struct smuHashBase;

typedef struct smuHashLatchFunc
{
    IDE_RC (*findNode)  (smuHashBase   *aBase, 
                         smuHashBucket *aBucket, 
                         void          *aKeyPtr, 
                         void         **aNode);
    IDE_RC (*insertNode)(smuHashBase   *aBase, 
                         smuHashBucket *aBucket, 
                         void          *aKeyPtr, 
                         void          *aNode);
    IDE_RC (*deleteNode)(smuHashBase   *aBase, 
                         smuHashBucket *aBucket, 
                         void          *aKeyPtr, 
                         void         **aNode);
}smuHashLatchFunc;

typedef struct smuHashBase
{
    iduMutex           mMutex;       /* Hash 전체의 mutex */

    void              *mMemPool;     /* Chain의 메모리 관리자 : 
                                        struct내부 class 사용 불가. so, void */
    UInt               mKeyLength;
    UInt               mBucketCount;
    smuHashBucket     *mBucket;      /* Bucket List */
    smuHashLatchFunc  *mLatchVector;

    smuHashGenFunc     mHashFunc;    /* HASH 함수 : callback */
    smuHashCompFunc    mCompFunc;    /* 비교 함수 : callback */

    /* for Traverse  */
    idBool             mOpened;      /* Traverse의 Open 유무 */
    UInt               mCurBucket;   /* 접근중인   Bucket 번호 */
    smuHashChain      *mCurChain;    /* 접근 중인  Chain 포인터 */
    //fix BUG-21311
    idBool             mDidAllocChain;

} smuHashBase;


class smuHash
{
    static smuHashLatchFunc mLatchVector[2];

    static smuHashChain* findNodeInternal(smuHashBase   *aBase, 
                                          smuHashBucket *aBucket, 
                                          void          *aKeyPtr);

    static IDE_RC findNodeLatch(smuHashBase   *aBase, 
                                smuHashBucket *aBucket, 
                                void          *aKeyPtr, 
                                void         **aNode);
    static IDE_RC insertNodeLatch(smuHashBase   *aBase, 
                                  smuHashBucket *aBucket, 
                                  void          *aKeyPtr, 
                                  void          *aNode);
    static IDE_RC deleteNodeLatch(smuHashBase   *aBase, 
                                  smuHashBucket *aBucket, 
                                  void          *aKeyPtr, 
                                  void         **aNode);

    static IDE_RC findNodeNoLatch(smuHashBase   *aBase, 
                                  smuHashBucket *aBucket, 
                                  void          *aKeyPtr, 
                                  void         **aNode);
    static IDE_RC insertNodeNoLatch(smuHashBase   *aBase, 
                                    smuHashBucket *aBucket, 
                                    void          *aKeyPtr, 
                                    void          *aNode);
    static IDE_RC deleteNodeNoLatch(smuHashBase   *aBase, 
                                    smuHashBucket *aBucket, 
                                    void          *aKeyPtr, 
                                    void         **aNode);

    static IDE_RC allocChain(smuHashBase *aBase, smuHashChain **aChain);
    static IDE_RC freeChain( smuHashBase *aBase, smuHashChain *aChain);

    static smuHashChain *searchFirstNode(smuHashBase *aBase);
    static smuHashChain *searchNextNode (smuHashBase *aBase);
    
    

public:
    static IDE_RC initialize(smuHashBase    *aBase, 
                             UInt            aConcurrentLevel,/* > 0 */
                             UInt            aBucketCount,
                             UInt            aKeyLength,
                             idBool          aUseLatch,
                             smuHashGenFunc  aHashFunc,
                             smuHashCompFunc aCompFunc);

    static IDE_RC destroy(smuHashBase  *aBase);
    static IDE_RC reset(smuHashBase  *aBase);
    
    static IDE_RC findNode(smuHashBase  *aBase, void *aKeyPtr, void **aNode);
    static IDE_RC insertNode(smuHashBase  *aBase, void *aKeyPtr, void *aNode);
    static IDE_RC deleteNode(smuHashBase  *aBase, void *aKeyPtr, void **aNode);

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
    static IDE_RC lock(smuHashBase  *aBase);
    static IDE_RC unlock(smuHashBase  *aBase);

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
    //fix BUG-21311
    static inline IDE_RC open(smuHashBase *aBase);
    static inline IDE_RC cutNode(smuHashBase *aBase, void **aNode);
    static inline IDE_RC close(smuHashBase *aBase);
    static IDE_RC isEmpty(smuHashBase *aBase, idBool *aIsEmpty);

    /* BUG-40427 */
    static inline IDE_RC getCurNode(smuHashBase *aBase, void **aNode);
    static inline IDE_RC getNxtNode(smuHashBase *aBase, void **aNode);
    static IDE_RC delCurNode(smuHashBase *aBase, void **aNode);
};
//fix BUG-21311 transform inline function.
inline IDE_RC smuHash::open(smuHashBase *aBase)
{
    IDE_ASSERT(aBase->mOpened == ID_FALSE);
    
    aBase->mOpened    = ID_TRUE;
    aBase->mCurBucket = 0; // before search
    if(aBase->mDidAllocChain == ID_TRUE)
    {    
        aBase->mCurChain  = searchFirstNode(aBase);
    }
    else
    {
        //empty
        aBase->mCurChain  = NULL;
    }
    return IDE_SUCCESS;
}

IDE_RC smuHash::cutNode(smuHashBase *aBase, void **aNode)
{
    smuHashChain  *sDeleteChain;

    if (aBase->mCurChain != NULL)
    {
        // delete current node & return the node pointer
        sDeleteChain = aBase->mCurChain;
        *aNode       = sDeleteChain->mNode;

        // find next
        aBase->mCurChain = searchNextNode(aBase);
        
        SMU_LIST_DELETE(&(sDeleteChain->mList));
        IDE_TEST(freeChain(aBase, sDeleteChain) != IDE_SUCCESS);
    
    }
    else // Traverse? ??.
    {
        *aNode = NULL; 
    }
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

inline IDE_RC smuHash::close(smuHashBase *aBase)
{
    IDE_ASSERT(aBase->mOpened == ID_TRUE);
    
    aBase->mOpened = ID_FALSE;
    
    return IDE_SUCCESS;
}

IDE_RC smuHash::getCurNode(smuHashBase *aBase, void **aNode)
{
    if (aBase->mCurChain != NULL)
    {   
        // return the node pointer
        *aNode = aBase->mCurChain->mNode;
    }   
    else // Traverse 실패
    {   
        *aNode = NULL; 
    }   
    return IDE_SUCCESS;
}

IDE_RC smuHash::getNxtNode(smuHashBase *aBase, void **aNode)
{
    if (aBase->mCurChain != NULL)
    {   
        // find next
        aBase->mCurChain = searchNextNode(aBase);

        if( aBase->mCurChain != NULL )
        {   
            *aNode = aBase->mCurChain->mNode;
        }   
        else
        {   
            *aNode = NULL; 
        }   

    }   
    else
    {   
        *aNode = NULL;
    }   

    return IDE_SUCCESS;
}
#endif  // _O_SMU_HASH_H_
    
