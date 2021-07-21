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
 *     iddTRBTree.cpp - Red Black tree
 *
 *   DESCRIPTION
 *     Red-Black tree implementation
 *
 *   MODIFIED   (04/07/2017)
 ****************************************************************************/

#include <iddTRBTree.h>

void* iddTRBTree::node::getKey(void) const
{
    return (this==NULL)? NULL:(void*)mKey;
}

void* iddTRBTree::node::getData(void) const
{
    return (this==NULL)? NULL:(void*)mData;
}

iddTRBTree::color iddTRBTree::node::getColor(void) const
{
    return (this==NULL)? IDD_BLACK:mColor;
}

void iddTRBTree::node::setColor(const iddTRBTree::color aColor)
{
    IDE_DASSERT(this != NULL);
    IDE_DASSERT((aColor == IDD_RED) || (aColor == IDD_BLACK));
    mColor = aColor;
}

iddTRBTree::node* iddTRBTree::node::setParent(iddTRBTree::node* aParent)
{
    iddTRBTree::node* sOldParent;
    if(this != NULL)
    {
        sOldParent = mParent;
        mParent = aParent;
    }
    else
    {
        sOldParent = NULL;
    }

    return sOldParent;
}

iddTRBTree::node* iddTRBTree::node::getPrev(void)
{
    return (this == NULL)? NULL:mPrev;
}

iddTRBTree::node* iddTRBTree::node::getNext(void)
{
    return (this == NULL)? NULL:mNext;
}

iddTRBTree::node* iddTRBTree::node::setPrev(iddTRBTree::node* aNewPrev)
{
    iddTRBTree::node* sOldPrev;
    if(this != NULL)
    {
        sOldPrev = mPrev;
        mPrev = aNewPrev;
    }
    else
    {
        sOldPrev = NULL;
    }

    return sOldPrev;
}

iddTRBTree::node* iddTRBTree::node::setNext(iddTRBTree::node* aNewNext)
{
    iddTRBTree::node* sOldNext;
    if(this != NULL)
    {
        sOldNext = mNext;
        mNext = aNewNext;
    }
    else
    {
        sOldNext = NULL;
    }

    return sOldNext;
}

idBool iddTRBTree::node::isLeftChild(void)
{
    IDE_DASSERT(this != NULL);
    IDE_DASSERT(mParent != NULL);
    return (mParent->mLeft == this)? ID_TRUE:ID_FALSE;
}

/**
 * Red-Black Tree를 초기화한다.
 *
 * @param aIndex : 내부적으로 사용할 메모리 할당 인덱스
 * @param aKeyLength : 키 크기(bytes)
 * @param aCompFunc : 비교용 함수.
 *                    SInt(const void* aKey1, const void* aKey2) 형태이며
 *                    aKey1 >  aKey2이면 1 이상을,
 *                    aKey1 == aKey2이면 0 을
 *                    aKey1 < aKey2이면 -1 이하를 리턴해야 한다.
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddTRBTree::initialize(const iduMemoryClientIndex    aIndex,
                             const ULong                    aKeyLength,
                             const iddCompFunc              aCompFunc)
{
    mMemIndex   = aIndex;
    mKeyLength  = aKeyLength;
    mNodeSize   = idlOS::align8(offsetof(iddTRBTree::node, mKey) + mKeyLength);
    mNodeCount  = 0;
    mRoot       = NULL;
    mCompFunc   = aCompFunc;

    mCount              = 0;
    mSearchCount        = 0;

    return IDE_SUCCESS;
}

/**
 * clear한다
 * 추가 destructor 행위가 있을 수 있음
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddTRBTree::destroy(void)
{
    /* free all nodes */
    return clear();
}

/**
 * Red-Black 트리에 키/데이터를 추가하고 불균형을 해소한다
 *
 * @param aKey : 키
 * @param aData : 값을 지니고 있는 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 중복 값이 있거나 메모리 할당에 실패했을 때
 */
IDE_RC iddTRBTree::insert(const void* aKey, void* aData)
{
    iddTRBTree::node*   sNewNode;
    iddTRBTree::node*   sNode;
    iddTRBTree::finder  sFinder(mCompFunc);

    sNode = findPosition(aKey, sFinder);
    IDE_TEST_RAISE( (sNode != NULL) && (sFinder.isFound() == ID_TRUE),
                    EDUPLICATED );
    IDE_TEST( allocNode(&sNewNode) != IDE_SUCCESS );
    idlOS::memcpy(sNewNode->mKey, aKey, mKeyLength);
    sNewNode->mData = aData;
    
    if(mRoot == NULL)
    {
        mRoot = sNewNode;
        mRoot->setPrev(NULL);
        mRoot->setNext(NULL);
    }
    else
    {
        if(sFinder.isLesser())
        {
            sNode->mLeft = sNewNode;

            sNewNode->setNext(sNode);
            sNewNode->setPrev(sNode->getPrev());
            sNode->getPrev()->setNext(sNewNode);
            sNode->setPrev(sNewNode);
        }
        else
        {
            sNode->mRight = sNewNode;

            sNewNode->setPrev(sNode);
            sNewNode->setNext(sNode->getNext());
            sNode->getNext()->setPrev(sNewNode);
            sNode->setNext(sNewNode);
        }

        sNewNode->mParent = sNode;
    }

    adjustInsertCase1(sNewNode);
    mCount++;
    mInsertLeft     += sFinder.getLeftMove();
    mInsertRight    += sFinder.getRightMove();

    return IDE_SUCCESS;

    IDE_EXCEPTION( EDUPLICATED )
    {
        /* do nothing */
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Red-Black 트리에서 키를 검색한다
 * 검색된 키에 해당하는 데이터는 aData에 저장된다
 *
 * @param aKey : 키
 * @param aData : 검색된 값을 지닐 포인터. 값이 없으면 NULL이 저장된다.
 * @return ID_TRUE  aKey를 찾았을 때
 *         ID_FALSE aKey를 찾지 못했을 때
 */
idBool iddTRBTree::search(const void* aKey, void** aData)
{
    iddTRBTree::node*    sNode;
    iddTRBTree::finder   sFinder(mCompFunc);
    void*               sData;

    sNode = findPosition(aKey, sFinder);

    if(sFinder.isFound() == ID_TRUE)
    {
        sData = sNode->mData;
        mSearchCount++;
    }
    else
    {
        sData  = NULL;
    }

    if(aData != NULL)
    {
        *aData = sData;
    }

    return sFinder.isFound();
}

/**
 * Red-Black Tree에서 aKey에 해당하는 데이터를 aNewData로 대체한다
 * 과거의 데이터는 aOldData에 실려나온다
 *
 * @param aKey : 키
 * @param aNewData : 새 값
 * @param aOldData : 과거 데이터를 저장할 포인터. NULL이 올 수 있다.
 * @return IDE_SUCCESS 값을 찾았을 때
 *         IDE_FAILURE 값이 없을 때
 */
IDE_RC iddTRBTree::update(const void* aKey, void* aNewData, void** aOldData)
{
    iddTRBTree::node*    sNode;
    void*               sData = NULL;
    iddTRBTree::finder   sFinder(mCompFunc);

    sNode = findPosition(aKey, sFinder);
    IDE_TEST( sFinder.isFound() == ID_FALSE );
    sData = sNode->mData;
    sNode->mData = aNewData;

    if(aOldData != NULL)
    {
        *aOldData = sData;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Red-Black 트리에서 키/데이터를 삭제한다
 * 삭제된 값은 aData에 저장된다.
 *
 * @param aKey : 키
 * @param aData : 삭제된 값을 지닐 포인터. 값이 없으면 NULL이 저장된다.
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 해제에 실패했을 때
 */
IDE_RC iddTRBTree::remove(const void* aKey, void** aData)
{
    iddTRBTree::node*    sNode;
    void*               sData   = NULL;
    iddTRBTree::finder   sFinder(mCompFunc);

    sNode = findPosition(aKey, sFinder);
    if(sFinder.isFound() == ID_TRUE)
    {
        sData = sNode->mData;
        IDE_TEST( removeNode(sNode) != IDE_SUCCESS );
        mCount--;
    }
    else
    {
        /* do nothing */
    }

    if(aData != NULL)
    {
        *aData = sData;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Red-Black 트리에서 모든 키/데이터를 삭제하고 메모리를 반환한다
 *
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 해제에 실패했을 때
 */
IDE_RC iddTRBTree::clear(void)
{
    IDE_TEST( freeAllNodes(mRoot) != IDE_SUCCESS );
    mCount  = 0;
    mRoot   = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * Red-Black 트리의 통계정보를 모두 초기화한다
 */
void iddTRBTree::clearStat(void)
{
    mSearchCount    = 0;
    mInsertLeft     = 0;
    mInsertRight    = 0;
}

/**
 * Red-Black 트리에 사용할 새로운 노드 할당
 *
 * @param aNode : 새 노드 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 할당에 실패했을 때
 */
IDE_RC iddTRBTree::allocNode(iddTRBTree::node** aNode)
{
    iddTRBTree::node* sNewNode;
    IDE_TEST( iduMemMgr::malloc(mMemIndex, mNodeSize,
                                (void**)&sNewNode) != IDE_SUCCESS );
    sNewNode->mColor    = IDD_RED;
    sNewNode->mParent   = NULL;
    sNewNode->mLeft     = NULL;
    sNewNode->mRight    = NULL;
    sNewNode->mPrev     = NULL;
    sNewNode->mNext     = NULL;
    sNewNode->mData     = NULL;

    *aNode = sNewNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 노드의 메모리를 해제한다
 *
 * @param aNode : 해제할 노드 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 메모리 해제에 실패했을 때
 */
IDE_RC iddTRBTree::freeNode(iddTRBTree::node* aNode)
{
    return iduMemMgr::free(aNode);
}

/**
 * 트리에서 가장 작은 노드를 찾는다.
 *
 * @return 노드의 포인터. 트리가 비어있으면 NULL
 */
iddTRBTree::node* iddTRBTree::findFirstNode(void) const
{
    iddTRBTree::node* sNode = mRoot;
    iddTRBTree::node* sPrev = mRoot;

    while(sNode != NULL)
    {
        sPrev = sNode;
        sNode = sPrev->mLeft;
    }

    return sPrev;
}

/**
 * 트리에서 aIter보다 큰 가장 작은 노드를 찾는다
 * DEBUG 모드에서는 aIter가 현재 인스턴스 안에 있는가
 * 검색하여 없으면 altibase_misc.log에 현재 트리를
 * 덤프하고 ASSERTION한다
 * RELEASE 모드에서는 aIter가 현재 인스턴스 안에 없을 때의 행동은
 * 정의되지 않았다
 *
 * @param aIter : 노드
 * @return 노드의 포인터. 트리가 비어있으면 NULL
 */
iddTRBTree::node* iddTRBTree::findNextNode(iddTRBTree::node* aIter)
    const
{
    iddTRBTree::node* sNode = aIter;
    iddTRBTree::node* sPrev;

#if defined(DEBUG)
    {
        iddTRBTree::finder sFinder(mCompFunc);
        findPosition(aIter->getKey(), sFinder);
        if(sFinder.isFound() == ID_FALSE)
        {
            validate();
            idlOS::fprintf(stderr, "Not a valid node : %p\n", aIter);
            IDE_DASSERT(0);
        }
    }
#endif

    if(aIter->mRight == NULL)
    {
        sPrev = sNode->mParent;
        if(sPrev == NULL)
        {
            /* root and no right child */
            sNode = NULL;
        }
        else if(sNode == sPrev->mLeft)
        {
            sNode = sPrev;
        }
        else
        {
            do
            {
                sPrev = sNode;
                sNode = sPrev->mParent;
            } while( (sNode != NULL) && (sPrev == sNode->mRight) );
        }
    }
    else
    {
        sNode = aIter->mRight;
        while(sNode != NULL)
        {
            sPrev = sNode;
            sNode = sPrev->mLeft;
        }
        sNode = sPrev;
    }

    return sNode;
}

/**
 * 트리 내부에서 aKey의 위치를 찾는다
 *
 * @param aKey : 키
 * @param aFinder : 탐색용 함수자.
 *                  동시에 탐색시 왼쪽/오른쪽 이동 회수를 기록한다.
 * @return 노드의 포인터. aKey를 찾으면 해당 노드의 포인터를,
 *         찾지 못하면 aKey와 가장 가까운 키를 지닌 노드를 리턴한다.
 *         insert할 때라면 리턴값이 새 노드의 부모가 된다.
 *         트리가 비어있으면 NULL을 리턴한다.
 */
iddTRBTree::node* iddTRBTree::findPosition(const void*        aKey,
                                         iddTRBTree::finder& aFinder)
    const
{
    iddTRBTree::node*    sNode = mRoot;
    idBool              sFound = ID_FALSE;
    SInt                sValue;

    while((sNode != NULL) && (sFound != ID_TRUE))
    {
        sValue = aFinder(aKey, sNode->mKey);

        if(sValue == 0)
        {
            sFound = ID_TRUE;
        }
        else
        {
            if(sValue > 0)
            {
                if(sNode->mRight == NULL)
                {
                    sFound = ID_TRUE;
                }
                else
                {
                    sNode = sNode->mRight;
                }
            }
            else
            {
                if(sNode->mLeft == NULL)
                {
                    sFound = ID_TRUE;
                }
                else
                {
                    sNode = sNode->mLeft;
                }
            }
        }
    }

    return sNode;
}

/**
 * aNode의 부모 노드의 부모 노드를 찾아낸다
 *
 * @param aNode : 노드 포인터
 * @return aNode의 부모 노드의 부모 노드. 없다면 NULL을 리턴한다.
 */
iddTRBTree::node* iddTRBTree::getGrandParent(const iddTRBTree::node* aNode) const
{
    iddTRBTree::node* sNode;
    IDE_DASSERT(aNode != NULL);

    sNode = aNode->mParent->mParent;
    return sNode;
}

/**
 * aNode의 부모 노드의 형제 노드를 찾아낸다
 *
 * @param aNode : 노드 포인터
 * @return aNode의 부모 노드의 형제 노드. 없다면 NULL을 리턴한다.
 */
iddTRBTree::node* iddTRBTree::getUncle(const iddTRBTree::node* aNode) const
{
    iddTRBTree::node* sNode;
    iddTRBTree::node* sGP;
    IDE_DASSERT(aNode != NULL);

    sGP = getGrandParent(aNode);

    if(sGP != NULL)
    {
        sNode = aNode->mParent;

        if(sNode == sGP->mLeft)
        {
            sNode = sGP->mRight;
        }
        else
        {
            sNode = sGP->mLeft;
        }
    }
    else
    {
        sNode = NULL;
    }

    return sNode;
}

/**
 * aNode의 형제 노드를 찾아낸다
 *
 * @param aNode : 노드 포인터
 * @return aNode의 형제 노드. 없다면 NULL을 리턴한다.
 */
iddTRBTree::node* iddTRBTree::getSibling(const iddTRBTree::node* aNode) const
{
    iddTRBTree::node* sNode;
    IDE_DASSERT(aNode != NULL);

    sNode = aNode->mParent;
    if(aNode == sNode->mLeft)
    {
        sNode = sNode->mRight;
    }
    else
    {
        sNode = sNode->mLeft;
    }

    return sNode;
}

/**
 * insert시에 생긴 불균형을 해결한다 : case 1
 * aNode가 root라면 BLACK으로 설정하고 끝
 * 아니면 case 2로 진행
 * Note : 새로 할당받는 노드는 항상 RED이다
 *
 * @param aNode : 불균형을 해결할 노드 포인터
 */
void iddTRBTree::adjustInsertCase1(iddTRBTree::node* aNode)
{
    IDE_DASSERT(aNode != NULL);
    if( aNode->mParent == NULL)
    {
        aNode->setColor(IDD_BLACK);
    }
    else
    {
        adjustInsertCase2(aNode);
    }
}

/**
 * insert시에 생긴 불균형을 해결한다 : case 2
 * aNode가 BLACK이라면 조정할 필요가 없다 끝
 * 아니면 case 3로 진행
 *
 * @param aNode : 불균형을 해결할 노드 포인터
 */
void iddTRBTree::adjustInsertCase2(iddTRBTree::node* aNode)
{
    if( aNode->mParent->mColor == IDD_BLACK )
    {
        /* return; */
    }
    else
    {
        adjustInsertCase3(aNode);
    }
}

/**
 * insert시에 생긴 불균형을 해결한다 : case 3
 * 신규 노드 N은 빨간색이다
 * 부모 노드 P와 삼촌 노드 U가 모두 RED이면
 * P와 U를 까맣게 칠하고 할아버지 노드 GP를 빨갛게 칠한다
 * 이후 GP에 대해 case 1부터 다시 수행한다
 * 그렇지 않으면 case 4를 수행한다
 *
 * @param aNode : 불균형을 해결할 노드 포인터
 */
void iddTRBTree::adjustInsertCase3(iddTRBTree::node* aNode)
{
    iddTRBTree::node* sUncle;
    iddTRBTree::node* sGP;

    sUncle = getUncle(aNode);
    if( (sUncle != NULL) && (sUncle->mColor == IDD_RED) )
    {
        aNode->mParent->mColor = IDD_BLACK;
        sUncle->mColor = IDD_BLACK;
        sGP = getGrandParent(aNode);
        IDE_DASSERT(sGP != NULL);
        sGP->mColor = IDD_RED;

        adjustInsertCase1(sGP);
    }
    else
    {
        adjustInsertCase4(aNode);
    }
}

/**
 * insert시에 생긴 불균형을 해결한다 : case 4
 * 신규 노드 N은 빨간색이다
 * 신규 노드 N의 부모 노드 P가 RED이고 삼촌 노드 U가 BLACK일 때
 * P를 회전시켜 P의 위치에 N을 집어넣고 P에 대해 5번 케이스로 진행한다
 *
 * @param aNode : 불균형을 해결할 노드 포인터
 */
void iddTRBTree::adjustInsertCase4(iddTRBTree::node* aNode)
{
    iddTRBTree::node* sGP = getGrandParent(aNode);
    iddTRBTree::node* sNode;
    IDE_DASSERT(sGP != NULL);

    if( (aNode == aNode->mParent->mRight) &&
        (aNode->mParent == sGP->mLeft) )
    {
        rotateLeft(aNode->mParent);
        sNode = aNode->mLeft;
    }
    else if( (aNode == aNode->mParent->mLeft) &&
             (aNode->mParent == sGP->mRight) )
    {
        rotateRight(aNode->mParent);
        sNode = aNode->mRight;
    }
    else
    {
        sNode = aNode;
    }

    adjustInsertCase5(sNode);
}

/**
 * insert시에 생긴 불균형을 해결한다 : case 5
 * 신규 노드 N은 빨간색이다
 * N을 까맣게 칠하고 할아버지 노드 GP를 빨갛게 칠한다
 * 이후 GP를 회전시켜 N의 부모노드 P가
 * GP와 N의 부모가 되게 한다
 * 이제 P는 N, GP, P 중 유일한 빨간색 노드이다
 * Before : GP(?)->P (?)->N (R)
 *               ->U (B)
 * After  :  P(B)->GP(R)->U (B)
 *               ->N (R)
 *
 * @param aNode : 불균형을 해결할 노드 포인터
 */
void iddTRBTree::adjustInsertCase5(iddTRBTree::node* aNode)
{
    iddTRBTree::node* sGP = getGrandParent(aNode);
    IDE_DASSERT(sGP != NULL);

    aNode->mParent->mColor = IDD_BLACK;
    sGP->mColor = IDD_RED;

    if(aNode == aNode->mParent->mLeft)
    {
        rotateRight(sGP);
    }
    else
    {
        rotateLeft(sGP);
    }
}

/**
 * aNode를 중심으로 왼쪽 회전
 */
void iddTRBTree::rotateLeft(iddTRBTree::node* aNode)
{
    iddTRBTree::node* sChild  = aNode->mRight;
    iddTRBTree::node* sParent = aNode->mParent;

    if( sChild != NULL )
    {
        if( sChild->mLeft != NULL )
        {
            sChild->mLeft->mParent = aNode;
        }

        aNode->mRight = sChild->mLeft;
        aNode->setParent(sChild);
        sChild->mLeft  = aNode;
        sChild->mParent = sParent;

        if( sParent != NULL )
        {
            if( sParent->mLeft == aNode)
            {
                sParent->mLeft = sChild;
            }
            else
            {
                sParent->mRight = sChild;
            }
        }
        else
        {
            mRoot = sChild;
        }
    }
}

/**
 * aNode를 중심으로 오른쪽 회전
 */
void iddTRBTree::rotateRight(iddTRBTree::node* aNode)
{
    iddTRBTree::node* sChild  = aNode->mLeft;
    iddTRBTree::node* sParent = aNode->mParent;

    if( sChild != NULL )
    {
        if( sChild->mRight != NULL )
        {
            sChild->mRight->mParent = aNode;
        }

        aNode->mLeft = sChild->mRight;
        aNode->mParent = sChild;
        sChild->mRight  = aNode;
        sChild->mParent = sParent;

        if( sParent != NULL )
        {
            if( sParent->mRight == aNode)
            {
                sParent->mRight = sChild;
            }
            else
            {
                sParent->mLeft = sChild;
            }
        }
        else
        {
            mRoot = sChild;
        }
    }
}

/**
 * aNode(D)를 트리에서 떼어내 메모리를 해제하고 불균형을 해소한다
 * 1 D의 자식노드가 모두 NULL이라면
 * 1-1 D를 떼어낸다
 * 1-2 T<-D 하고 5
 * 2 D의 자식노드가 하나라면
 * 2-1 D를 떼어내고 자식 노드(C)로 대체한다
 * 2-2 A<-C, T<-D
 * 2-3 4 수행
 * 3 D의 자식이 둘이라면 D의 successor S를 찾아낸다
 * 3-1 S의 Key, Data를 D에 덮어쓴다 이 때 D의 색깔은 바뀌지 않는다
 * 3-2 S의 왼쪽 자식은 항상 NULL이다 S의 오른쪽 자식(C)을 S와 대체한다(A)
 * 3-3 A<-C, T<-S
 * 3-4 4 수행
 * 4 A에 대해 불균형을 해소한다
 * 5 T를 해제한다
 * 6 끝
 *
 * @param aNode : 삭제할 노드.
 * @return IDE_SUCCESS
 *         IDE_FAILRE 메모리 해제 실패시
 */
IDE_RC iddTRBTree::removeNode(iddTRBTree::node* aNode)
{
    iddTRBTree::node* sSuccessor;
    iddTRBTree::node* sTarget;
    iddTRBTree::node* sAdjust;

    IDE_DASSERT(aNode != NULL);

    if(aNode->mLeft == NULL && aNode->mRight == NULL)
    {
        if(aNode->mParent == NULL)
        {
            mRoot = NULL;
        }
        else
        {
            unlinkNode(aNode);
        }

        sTarget = aNode;
    }
    else
    {
        if(aNode->mRight == NULL)
        {
            unlinkNode(aNode, aNode->mLeft);
            sTarget = aNode;
            sAdjust = aNode->mLeft;
        }
        else if(aNode->mLeft == NULL)
        {
            unlinkNode(aNode, aNode->mRight);
            sTarget = aNode;
            sAdjust = aNode->mRight;
        }
        else
        {
            sSuccessor = findSuccessor(aNode);
            IDE_DASSERT(sSuccessor != NULL);
            IDE_DASSERT(sSuccessor->mLeft == NULL);
            replaceNode(aNode, sSuccessor);
            unlinkNode(sSuccessor, sSuccessor->mRight);

            sTarget = sSuccessor;
            sAdjust = sSuccessor->mRight;
        }

        if(sTarget->getColor() == IDD_BLACK)
        {
            adjustRemove(sAdjust);
        }
    }

    if(mRoot != NULL)
    {
        mRoot->setColor(IDD_BLACK);
    }

    sTarget->getPrev()->setNext(sTarget->getNext());
    sTarget->getNext()->setPrev(sTarget->getPrev());
    IDE_TEST( freeNode(sTarget) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * aNode와 그 자손 노드의 메모리를 모두 해제한다
 *
 * @param aNode : 삭제할 노드.
 * @return IDE_SUCCESS
 *         IDE_FAILRE 메모리 해제 실패시
 */
IDE_RC iddTRBTree::freeAllNodes(iddTRBTree::node* aNode)
{
    if(aNode != NULL)
    {
        /* BUGBUG iteration으로 바꿀 방법을 생각해볼것 */
        IDE_TEST( freeAllNodes(aNode->mLeft) != IDE_SUCCESS );
        IDE_TEST( freeAllNodes(aNode->mRight) != IDE_SUCCESS );
        IDE_TEST( freeNode(aNode) != IDE_SUCCESS );
    }
    else
    {
        /* fall through */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * aOldNode를 떼어내고 그 위치에 aNewNode를 넣는다
 *
 * @param aOldNode : 삭제할 노드.
 * @param aNewNode : 신규 노드.
 */
void iddTRBTree::unlinkNode(const iddTRBTree::node* aOldNode,
                           iddTRBTree::node*       aNewNode)
{
    iddTRBTree::node* sParent;
    IDE_DASSERT(aOldNode != NULL);

    sParent = aOldNode->mParent;
    if(sParent == NULL)
    {
        /* this is root */
        mRoot = aNewNode;
    }
    else
    {
        if(sParent->mLeft == aOldNode)
        {
            sParent->mLeft = aNewNode;
        }
        else
        {
            IDE_DASSERT(sParent->mRight == aOldNode);
            sParent->mRight = aNewNode;
        }
    }

    if(aNewNode != NULL)
    {
        aNewNode->mParent = sParent;

        if(aNewNode == aOldNode->mLeft)
        {
            IDE_DASSERT(aOldNode->mRight == NULL);
        }
        else if(aNewNode == aOldNode->mRight)
        {
            IDE_DASSERT(aOldNode->mLeft == NULL);
        }
        else
        {
            aNewNode->mLeft   = aOldNode->mLeft;
            aNewNode->mRight  = aOldNode->mRight;
            aNewNode->mLeft->setParent(aNewNode);
            aNewNode->mRight->setParent(aNewNode);
        }
    }
    else
    {
        /* fall through */
    }
}

/**
 * 노드 대체
 * aSrcNode의 Key와 Data를 aDestNode에 덮어쓴다
 * 이 때 aSrcNode의 색깔은 덮어씌어지지 않는다
 *
 * @param aDestNode : 대상 노드
 * @param aSrcNode : 원본 노드
 */
void iddTRBTree::replaceNode(iddTRBTree::node* aDestNode,
                            iddTRBTree::node* aSrcNode)
{
    IDE_DASSERT(aDestNode != NULL);
    IDE_DASSERT(aSrcNode != NULL);

    idlOS::memcpy(aDestNode->mKey, aSrcNode->mKey, mKeyLength);
    aDestNode->mData = aSrcNode->mData;
}

/**
 * 노드를 삭제한 후 발생한 불균형을 해소한다 : case 1
 * aNode가 루트이면 그냥 까맣게 칠하고 끝
 * aNode가 루트가 아니면 case 2로 진행한다.
 *
 * @param aNode : 균형을 잡을 노드.
 */
void iddTRBTree::adjustRemove(iddTRBTree::node* aNode)
{
    if(aNode != NULL)
    {
        if(aNode->getColor() == IDD_RED)
        {
            aNode->setColor(IDD_BLACK);
        }
        else if(aNode != mRoot)
        {
            adjustRemoveCases(aNode);
        }
    }
    else
    {
        /* return; */
    }
}

/**
 * 노드를 삭제한 후 발생한 불균형을 해소한다 : case 2
 * 자세한 설명은 알고리즘 교과서를 참조하세요
 *
 * @param aNode : 균형을 잡을 노드.
 */
void iddTRBTree::adjustRemoveCases(iddTRBTree::node* aNode)
{
    idBool              sDone = ID_FALSE;
    iddTRBTree::color    sColor;
    iddTRBTree::node*    sNode = aNode;
    iddTRBTree::node*    sSibling = getSibling(sNode);
    IDE_DASSERT(aNode->getColor() == IDD_BLACK);

    if(sSibling == NULL)
    {
        /* just return */
    }
    else
    {
        do
        {
            if( sSibling->getColor() == IDD_RED )
            {
                sNode->mParent->setColor(IDD_RED);
                sSibling->setColor(IDD_BLACK);
                if( sNode->isLeftChild() == ID_TRUE )
                {
                    rotateLeft(sNode->mParent);
                }
                else
                {
                    rotateRight(sNode->mParent);
                }

                sSibling = getSibling(sNode);
                if( sSibling == NULL )
                {
                    break;
                }
            }

            if( (sSibling->getColor() == IDD_BLACK)         &&
                (sSibling->mLeft->getColor() == IDD_BLACK)  &&
                (sSibling->mRight->getColor() == IDD_BLACK) )
            {
                sSibling->setColor(IDD_RED);
                sNode = sNode->mParent;

                if( sNode->getColor() == IDD_RED )
                {
                    sNode->setColor(IDD_BLACK);
                    sDone = ID_TRUE;
                }
                else
                {
                    /* sDone = ID_FALSE; */
                }
            }
            else
            {
                if( sNode->isLeftChild() == ID_TRUE )
                {
                    if( (sSibling->getColor() == IDD_BLACK)         &&
                        (sSibling->mLeft->getColor() == IDD_RED)    &&
                        (sSibling->mRight->getColor() == IDD_BLACK) )
                    {
                        sSibling->mLeft->setColor(IDD_BLACK);
                        sSibling->setColor(IDD_RED);
                        rotateRight(sSibling);

                        sSibling = getSibling(sNode);
                    }

                    if( (sSibling->getColor() == IDD_BLACK) &&
                        (sSibling->mRight->getColor() == IDD_RED) )
                    {
                        sColor = sNode->mParent->getColor();
                        sNode->mParent->setColor(sSibling->getColor());
                        sSibling->setColor(sColor);
                        rotateLeft(sNode->mParent);

                        sDone = ID_TRUE;
                    }
                }
                else
                {
                    if( (sSibling->getColor() == IDD_BLACK)         &&
                        (sSibling->mRight->getColor() == IDD_RED)    &&
                        (sSibling->mLeft->getColor() == IDD_BLACK) )
                    {
                        sSibling->mRight->setColor(IDD_BLACK);
                        sSibling->setColor(IDD_RED);
                        rotateLeft(sSibling);

                        sSibling = getSibling(sNode);
                    }

                    if( (sSibling->getColor() == IDD_BLACK) &&
                        (sSibling->mLeft->getColor() == IDD_RED) )
                    {
                        sColor = sNode->mParent->getColor();
                        sNode->mParent->setColor(sSibling->getColor());
                        sSibling->setColor(sColor);
                        rotateRight(sNode->mParent);

                        sDone = ID_TRUE;
                    }
                }
            }
        } while( (sNode != mRoot) && (sDone == ID_FALSE) );
    }
}

/**
 * aNode의 successor를 찾는다
 * successor는 aNode의 오른쪽 자식의 가장 왼쪽 후손이다
 *
 * @param aNode : 찾을 노드
 * @return successor 노드 없으면 NULL을 리턴한다
 */
iddTRBTree::node* iddTRBTree::findSuccessor(const iddTRBTree::node* aNode) const
{
    iddTRBTree::node* sPrev;
    iddTRBTree::node* sNode = aNode->mRight;

    sPrev = sNode;
    while(sNode != NULL)
    {
        sPrev = sNode;
        sNode = sNode->mLeft;
    }
    sNode = sPrev;

    return sPrev;
}

#if defined(DEBUG)

void iddTRBTree::dumpNode(iddTRBTree::node* aNode, ideLogEntry& aLog) const
{
    if(aNode == NULL)
    {
        aLog.append("NULL[BLACK]");
    }
    else
    {
        aLog.appendFormat("%p[%s]", aNode,
                aNode->getColor() == IDD_BLACK?"IDD_BLACK":"IDD_RED");
    }
}

SInt iddTRBTree::validateNode(iddTRBTree::node* aNode, SInt aLevel) const
{
    SInt sRet;
    SInt i;

    if(aNode != NULL)
    {
        ideLogEntry sLog(IDE_MISC_0);
        for(i = 0; i < aLevel; i++)
        {
            sLog.append(" ");
        }
        dumpNode(aNode, sLog);
        sLog.append("->");
        dumpNode(aNode->mLeft, sLog);
        sLog.append("/");
        dumpNode(aNode->mRight, sLog);

        if(aNode->getColor() == IDD_RED)
        {
            if(aNode->mLeft->getColor() == IDD_RED)
            {
                sLog.append(" invalid(left)!");
            }
            if(aNode->mRight->getColor() == IDD_RED)
            {
                sLog.append(" invalid(right)!");
            }
        }

        sLog.write();

        sRet = 1;
        sRet += validateNode(aNode->mLeft, aLevel + 1);
        sRet += validateNode(aNode->mRight, aLevel + 1);
    }
    else
    {
        sRet = 0;
    }

    return sRet;
}

SInt iddTRBTree::validate() const
{
    return validateNode(mRoot, 0);
}

#endif

/**
 * 현재 RBTree 인스턴스의 통계정보 수집
 *
 * @param aStat : 통계정보 수집용 구조체
 */
void iddTRBTree::fillStat(iddRBHashStat* aStat)
{
	aStat->mKeyLength       = mKeyLength;
    aStat->mCount           = mCount;
	aStat->mSearchCount     = mSearchCount;
    aStat->mInsertLeft      = mInsertLeft;
    aStat->mInsertRight     = mInsertRight;
}

/**
 * 동시성 제어 기능이 있는 Red-Black Tree를 초기화한다.
 *
 * @param aIndex : 내부적으로 사용할 메모리 할당 인덱스
 * @param aKeyLength : 키 크기(bytes)
 * @param aCompFunc : 비교용 함수.
 *                    SInt(const void* aKey1, const void* aKey2) 형태이며
 *                    aKey1 >  aKey2이면 1 이상을,
 *                    aKey1 == aKey2이면 0 을
 *                    aKey1 < aKey2이면 -1 이하를 리턴해야 한다.
 * @param aUseLatch : 래치 사용 여부(ID_TRUE by default)
 * @param aLatchType : 래치 종류. 지정하지 않으면 LATCH_TYPE
 *                     프로퍼티를 따른다.
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddTRBLatchTree::initialize(const iduMemoryClientIndex    aIndex,
                                  const ULong                   aKeyLength,
                                  const iddCompFunc            aCompFunc,
                                  const idBool                  aUseLatch,
                                  const iduLatchType            aLatchType)
{
    iduLatchType sLatchType;

    mUseLatch = aUseLatch;

    if( mUseLatch == ID_TRUE )
    {
        sLatchType = (aLatchType == IDU_LATCH_TYPE_MAX)?
            (iduLatchType)iduProperty::getLatchType() :
            aLatchType;

        IDE_TEST( mLatch.initialize((SChar*)"Latch4RBTree", sLatchType)
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( iddTRBTree::initialize(aIndex, aKeyLength, aCompFunc)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 공유 락 획득
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddTRBLatchTree::lockRead(void)
{
    if( mUseLatch == ID_TRUE )
    {
        IDE_TEST( mLatch.lockRead(NULL, NULL) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 배제 락 획득
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddTRBLatchTree::lockWrite(void)
{
    if( mUseLatch == ID_TRUE )
    {
        IDE_TEST( mLatch.lockWrite(NULL, NULL) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 락 해제
 * @return IDE_SUCCESS
 *         IDE_FAILURE
 */
IDE_RC iddTRBLatchTree::unlock(void)
{
    if( mUseLatch == ID_TRUE )
    {
        IDE_TEST( mLatch.unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 배제 락을 획득하고 키/데이터를 추가한다
 * @param aKey : 키
 * @param aData : 값을 지니고 있는 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 중복 값이 있거나 락 획득, 메모리 할당에 실패했을 때
 */
IDE_RC iddTRBLatchTree::insert(const void* aKey, void* aData)
{
    IDE_TEST      ( lockWrite() != IDE_SUCCESS );
    IDE_TEST_RAISE( iddTRBTree::insert(aKey, aData) != IDE_SUCCESS,
                    INSERTFAILED );
    IDE_TEST      ( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( INSERTFAILED )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 공유 락을 획득하고 키/데이터를 검색한다
 * @param aKey : 키
 * @param aData : 찾아낸 값을 저장할 포인터
 * @return IDE_SUCCESS
 *         IDE_FAILURE 중복 값이 있거나 락 획득, 메모리 할당에 실패했을 때
 */
IDE_RC iddTRBLatchTree::search(const void* aKey, void** aData)
{
    IDE_TEST( lockRead() != IDE_SUCCESS );
    (void)iddTRBTree::search(aKey, aData);
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 배제 락을 획득하고 aKey에 해당하는 데이터를 aNewData로 대체한다
 * 과거의 데이터는 aOldData에 실려나온다
 *
 * @param aKey : 키
 * @param aNewData : 새 값
 * @param aOldData : 과거 데이터를 저장할 포인터. NULL이 올 수 있다.
 * @return IDE_SUCCESS 값을 찾았을 때
 *         IDE_FAILURE 값이 없을 때
 */
IDE_RC iddTRBLatchTree::update(const void* aKey, void* aNewData, void** aOldData)
{
    IDE_TEST      ( lockWrite() != IDE_SUCCESS );
    IDE_TEST_RAISE( iddTRBTree::update(aKey, aNewData, aOldData) != IDE_SUCCESS,
                    UPDATEFAILED );
    IDE_TEST      ( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( UPDATEFAILED )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/**
 * 배제 락을 획득하고 aKey에 해당하는 키/데이터를 제거한다
 * 과거의 데이터는 aOldData에 실려나온다
 *
 * @param aKey : 키
 * @param aNewData : 새 값
 * @param aOldData : 과거 데이터를 저장할 포인터. NULL이 올 수 있다.
 * @return IDE_SUCCESS 값을 찾았을 때
 *         IDE_FAILURE 값이 없을 때
 */
IDE_RC iddTRBLatchTree::remove(const void* aKey, void** aData)
{
    IDE_TEST      ( lockWrite() != IDE_SUCCESS );
    IDE_TEST_RAISE( iddTRBTree::remove(aKey, aData) != IDE_SUCCESS,
                    REMOVEFAILED );
    IDE_TEST      ( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( REMOVEFAILED )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

