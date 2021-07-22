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

/* **********************************************************************
 *   $Id: 
 *   NAME
 *     iddTRBTree.h - Threaded Red Black Tree
 *
 *   DESCRIPTION
 *     Red-Black Tree
 *
 *   Classes
 *     iddTRBTree : Key/Data pair를 저장할 수 있는 Threaded Red-Black 트리
 *     iddTRBLatchTree : iddTRBTree에 latch를 활용한 동시성 제어 추가
 *
 *   MODIFIED   (04/07/2017)
 ********************************************************************** */

#ifndef O_IDD_TRBTREE_H
#define O_IDD_TRBTREE_H   1

#include <idl.h>
#include <ideErrorMgr.h>
#include <iduLatch.h>
#include <idu.h>
#include <iddDef.h>

class iddTRBTree
{
public:
    typedef enum
    {
        IDD_RED = 0,
        IDD_BLACK
    } color;

    class node
    {
    public:
        color   mColor;
        node*   mParent;
        node*   mLeft;
        node*   mRight;
        node*   mPrev;
        node*   mNext;
        void*   mData;
        SChar   mKey[1];

        void*   getKey(void) const;
        void*   getData(void) const;
        color   getColor(void) const;
        void    setColor(const color);
        node*   setParent(node*);
        node*   getPrev(void);
        node*   getNext(void);
        node*   setPrev(node*);
        node*   setNext(node*);
        idBool  isLeftChild(void);
    };

    class finder
    {
    public:
        finder(iddCompFunc aFunc)
            : mFunc(aFunc), mValue(0), mFound(ID_FALSE), mLeft(0), mRight(0) {}

        inline SInt comp(const void* aKey1, const void* aKey2)
        {
            mValue = mFunc(aKey1, aKey2);
            if(mValue == 0)
            {
                mFound = ID_TRUE;
            }
            else
            {
                mFound = ID_FALSE;

                if(mValue > 0)
                {
                    mRight++;
                }
                else
                {
                    mLeft++;
                }
            }

            return mValue;

        }
        inline SInt operator()(const void* aKey1, const void* aKey2)
        {
            return comp(aKey1, aKey2);
        }
        inline SInt operator()(const node* aNode1, const node* aNode2)
        {
            return comp(aNode1->getKey(), aNode2->getKey());
        }

        inline idBool isFound(void)   {return (mValue == 0)? mFound :ID_FALSE;}
        inline idBool isGreater(void) {return (mValue >  0)? ID_TRUE:ID_FALSE;}
        inline idBool isLesser(void)  {return (mValue <  0)? ID_TRUE:ID_FALSE;}
        inline SInt   getLeftMove(void) {return mLeft;}
        inline SInt   getRightMove(void) {return mRight;}
    private:
        iddCompFunc    mFunc;
        SInt            mValue;
        idBool          mFound;
        SInt            mLeft;
        SInt            mRight;
    };

public:
    IDE_RC  initialize(const iduMemoryClientIndex, const ULong, const iddCompFunc);
    IDE_RC  destroy(void);
    IDE_RC  insert(const void*, void* = NULL);
    IDE_RC  update(const void*, void*, void** = NULL);
    IDE_RC  remove(const void*, void** = NULL);
    idBool  search(const void*, void** = NULL);
    IDE_RC  clear (void);
    void    clearStat(void);

public:
    node* findFirstNode(void) const;
    node* findNextNode(node*) const;
    inline idBool isEnd(node* aIter)
    {
        return (aIter == NULL)? ID_TRUE:ID_FALSE;
    }

#if defined(DEBUG)
    void dumpNode(node*, ideLogEntry&) const;
    SInt validateNode(node*, SInt) const;
    SInt validate(void) const;
#endif

private:
    IDE_RC              allocNode(node**);
    IDE_RC              freeNode(node*);

private:
    /* Search functions */
    node*   findPosition(const void*, finder&) const;
    node*   findSuccessor(const node*) const;

private:
    /* Grantparent and uncle node */
    node*   getGrandParent(const node*) const;
    node*   getUncle(const node*) const;
    node*   getSibling(const node*) const;

private:
    /* Adjust tree after insertion */
    void    adjustInsertCase1(node*);
    void    adjustInsertCase2(node*);
    void    adjustInsertCase3(node*);
    void    adjustInsertCase4(node*);
    void    adjustInsertCase5(node*);

    /* removals */
    IDE_RC  removeNode(node*);
    IDE_RC  freeAllNodes(node*);
    void    unlinkNode(const node*, node* = NULL);
    /* Adjust tree after removal */
    void    adjustRemove(node*);
    void    adjustRemoveCases(node*);

private:
    void    rotateLeft (node*);
    void    rotateRight(node*);
    void    replaceNode(node*, node*);


private:
    iduMemoryClientIndex    mMemIndex;
    ULong                   mNodeSize;
    ULong                   mNodeCount;
    node*                   mRoot;
    iddCompFunc            mCompFunc;

    /* stats */
private:
    ULong                   mKeyLength;
    ULong                   mCount;
    ULong                   mSearchCount;
    ULong                   mInsertLeft;
    ULong                   mInsertRight;

public:
    /**
     * iddTRBTree를 순회할 수 있는 iterator 클래스
     * 비교함수로 비교시 가장 작은 노드부터
     * 차례로 순회할 수 있다
     */
    class iterator
    {
    public:
        iterator(const iddTRBTree* aTree = NULL, const node* aNode = NULL)
            : mTree(aTree), mNode(aNode) {}
        inline iterator& moveNext(void)
        {
            mNode = mNode->mNext;
            return *this;
        }
        const inline iterator& operator=(const iterator& aIter)
        {
            mTree = aIter.mTree;
            mNode = aIter.mNode;
            return *this;
        }
        bool operator==(const iterator& aIter) const
        {
            return (mTree==aIter.mTree) && (mNode==aIter.mNode);
        }
        bool operator!=(const iterator& aIter) const
        {
            return (mTree!=aIter.mTree) || (mNode!=aIter.mNode);
        }
        inline iterator& operator++() {return moveNext();}
        inline iterator& operator++(int) {return moveNext();}
        inline void* getKey(void) {return mNode->getKey();}
        inline void* getData(void) {return mNode->getData();}

    private:
        const iddTRBTree* mTree;
        const node* mNode;
    };

    friend class iterator;
    inline const iterator begin(void) {return iterator(this, findFirstNode());}
    inline const iterator end(void) {return iterator(this, NULL);}

public:
    void fillStat(iddRBHashStat*);
};

class iddTRBLatchTree : public iddTRBTree
{
public:
    IDE_RC initialize(const iduMemoryClientIndex,
                      const ULong,
                      const iddCompFunc,
                      const idBool = ID_TRUE,
                      const iduLatchType = IDU_LATCH_TYPE_MAX);
    IDE_RC lockRead(void);
    IDE_RC lockWrite(void);
    IDE_RC unlock(void);

    IDE_RC insert(const void*, void* = NULL);
    IDE_RC search(const void*, void** = NULL);
    IDE_RC update(const void*, void*, void** = NULL);
    IDE_RC remove(const void*, void** = NULL);

private:
    iduLatch    mLatch;
    idBool      mUseLatch;
};

typedef iddTRBTree::iterator iddTRBTreeIterator;

#endif /* O_IDD_TRBTREE_H */

