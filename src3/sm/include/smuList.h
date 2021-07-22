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
 * $Id: smuList.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMU_LIST_H_
#define _O_SMU_LIST_H_ 1

#include <idl.h>
#include <smDef.h>

/*
 *  SM 내부에서 사용되는 메모리 리스트 노드에 대한 라이브러리를 제공한다.
 *  smuList를 사용하여, 서로간의 링크를 유지한다.
 *
 *  주의 : !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *           BaseNode의 경우에는 반드시 초기화 매크로를 이용하여,
 *                                    (SMU_LIST_INIT_BASE)
 *           Circular 형태의 링크를 유지하도록 해야 한다. 그렇지 않으면,
 *           아래에 정의된 조작 매크로의 정확성을 보장할 수 없다.
 *         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * Sample)
 *  struct SampleNode {
 *     UInt mCount;
 *     smuList mBaseNode;
 *  };
 *
 *  void initializeSampleObject(struct SampleNode *aNode)
 *  {
 *       aNode->mCount = 0;
 *       SMU_LIST_INIT_BASE( &(aNode->mBaseNode) );  <== 참조!!!!!!!!!
 *  }
 *
 *  void operateNode(struct SampleNode *aNode)
 *  {
 *      smuList  sSample1;
 *      smuList  sSample2;
 *
 *      SMU_LIST_ADD_AFTER( &(aNode->mBaseNode), &sSample1);
 *      // 현재 상태 : BASE-->Sample1
 *
 *      SMU_LIST_ADD_BEFORE( &(aNode->mBaseNode)->mNext, &sSample2);
 *      // 현재 상태 : BASE-->Sample2-->Sample1 
 *      
 *      SMU_LIST_DELETE(&sSample1);
 *      // 현재 상태 : BASE-->Sample2
 *
 *      SMU_LIST_GET_PREV(&sSample1) = NULL;
 *      SMU_LIST_GET_NEXT(&sSample1) = NULL;
 *  }
 *
 */

typedef struct smuList
{
    smuList *mPrev;
    smuList *mNext;
    void    *mData;
} smuList;

/*
 * Example :
 *      struct { smuList mList; int a; } sBar;
 *      smuList * sIterator;
 *      SMU_LIST_ITERATE(&sBar.mList, sIterator)
 *      {
 *          ..... whatever you want to do
 *          단지, 리스트의 구성 멤버중 하나를 삭제하는 일을 해서는 안된다.
 *          리스트의 멤버를 삭제하고자 할 때에는
 *          SMU_LIST_ITERATE_SAFE() 를 쓰도록 해야 한다.
 *      }
 */

#define SMU_LIST_ITERATE(aHead, aIterator)                              \
    for ((aIterator) = (aHead)->mNext;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mNext)

#define SMU_LIST_ITERATE_BACK(aHead, aIterator)                         \
    for ((aIterator) = (aHead)->mPrev;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mPrev)

/*
 * Example :
 *      struct { smuList mList; int a; } sBar;
 *      smuList * sIterator;
 *      smuList * sNodeNext;
 *      SMU_LIST_ITERATE_SAFE(&sBar.mList, sIterator, sNodeNext)
 *      {
 *          ..... whatever you want to do
 *      }
 */

#define SMU_LIST_ITERATE_SAFE(aHead, aIterator, aNodeNext)              \
    for((aIterator) = (aHead)->mNext, (aNodeNext) = (aIterator)->mNext; \
        (aIterator) != (aHead);                                         \
        (aIterator) = (aNodeNext), (aNodeNext) = (aIterator)->mNext)

#define SMU_LIST_ITERATE_BACK_SAFE(aHead, aIterator, aNodeNext)         \
    for((aIterator) = (aHead)->mPrev, (aNodeNext) = (aIterator)->mPrev; \
        (aIterator) != (aHead);                                         \
        (aIterator) = (aNodeNext), (aNodeNext) = (aIterator)->mPrev)



//  Base Node에 대한 초기화 매크로 

#define SMU_LIST_INIT_BASE(node) { \
        (node)->mPrev = (node);    \
        (node)->mNext = (node);    \
        (node)->mData =  NULL;    \
}

#define SMU_LIST_IS_EMPTY(node) (((node)->mPrev == (node)) && ((node)->mNext == (node)))

#define SMU_LIST_IS_NODE_LINKED(node) (((node)->mPrev != NULL) && ((node)->mNext != NULL))

#define SMU_LIST_INIT_NODE(node) { \
        (node)->mPrev = NULL;      \
        (node)->mNext = NULL;      \
}

// Previous 얻기
#define SMU_LIST_GET_PREV(node)  ((node)->mPrev)

// Next 얻기
#define SMU_LIST_GET_NEXT(node)  ((node)->mNext)


// 처음 노드로 가기
#define SMU_LIST_GET_FIRST(base)  ((base)->mNext)

// 마지막 노드로 가기
#define SMU_LIST_GET_LAST(base)  ((base)->mPrev)


// tnode의 앞에 노드를 넣는다.
#define SMU_LIST_ADD_BEFORE(tnode, node) {   \
            (node)->mNext = (tnode);         \
            (node)->mPrev = (tnode)->mPrev;  \
            (tnode)->mPrev->mNext = (node);  \
            (tnode)->mPrev = (node);         \
}

// tnode의 뒤에 노드를 넣는다.
#define SMU_LIST_ADD_AFTER(tnode, node)   {  \
            (node)->mPrev = (tnode);         \
            (node)->mNext = (tnode)->mNext;  \
            (tnode)->mNext->mPrev = (node);  \
            (tnode)->mNext = (node);         \
}

// tnode의 뒤에 노드를 넣는다.
#define SMU_LIST_ADD_AFTER_SERIAL_OP(tnode, node)   {  \
        ID_SERIAL_BEGIN((node)->mPrev = (tnode));      \
        ID_SERIAL_END((node)->mNext = (tnode)->mNext); \
            (tnode)->mNext->mPrev = (node);  \
            (tnode)->mNext = (node);         \
}

#define SMU_LIST_ADD_FIRST_SERIAL_OP(base, node)  SMU_LIST_ADD_AFTER_SERIAL_OP(base, node)
#define SMU_LIST_ADD_FIRST(base, node)  SMU_LIST_ADD_AFTER(base, node)
#define SMU_LIST_ADD_LAST(base, node)   SMU_LIST_ADD_BEFORE(base, node)

#define SMU_LIST_CUT_BETWEEN(prev, next)   { \
            (prev)->mNext = NULL;            \
            (next)->mPrev = NULL;            \
}

#define SMU_LIST_ADD_LIST_FIRST(base, first, last) {  \
            (first)->mPrev = (base);                  \
            (last)->mNext = (base)->mNext;            \
            (base)->mNext->mPrev = (last);            \
            (base)->mNext = (first);                  \
}

#define SMU_LIST_ADD_LIST_FIRST_SERIAL_OP(base, first, last) { \
            ID_SERIAL_BEGIN((first)->mPrev = (base));          \
            ID_SERIAL_END((last)->mNext = (base)->mNext);      \
            (base)->mNext->mPrev = (last);                     \
            (base)->mNext = (first);                           \
}

// 노드를 리스트로부터 제거한다. 
#define SMU_LIST_DELETE(node)  {                  \
            (node)->mPrev->mNext = (node)->mNext; \
            (node)->mNext->mPrev = (node)->mPrev; \
}

#endif  // _O_SMU_LIST_H_
    
