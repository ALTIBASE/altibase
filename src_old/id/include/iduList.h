/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduList.h 34702 2009-08-12 04:31:04Z lswhh $
 **********************************************************************/

#ifndef _IDU_LIST_H_
#define _IDU_LIST_H_

typedef struct iduList iduList;
typedef struct iduList iduListNode;

struct iduList
{
    iduList * mPrev;
    iduList * mNext;

    void    * mObj;
};


/*
 * Example :
 *      struct { iduList mList; int a; } sBar;
 *      iduList * sIterator;
 *      IDU_LIST_ITERATE(&sBar.mList, sIterator)
 *      {
 *          ..... whatever you want to do
 *          단지, 리스트의 구성 멤버중 하나를 삭제하는 일을 해서는 안된다.
 *          리스트의 멤버를 삭제하고자 할 때에는
 *          IDU_LIST_ITERATE_SAFE() 를 쓰도록 해야 한다.
 *      }
 */

#define IDU_LIST_ITERATE(aHead, aIterator)                              \
    for ((aIterator) = (aHead)->mNext;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mNext)

#define IDU_LIST_ITERATE_BACK(aHead, aIterator)                         \
    for ((aIterator) = (aHead)->mPrev;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mPrev)
/***********************************************************************
*
* Description :
*  aHead를 갖는 리스트에서 aNode 다음 노드부터 마지막 노드까지 aIterator에 
*  반환해 준다.
*  aHead          - [IN]  리스트 헤더 포인터
*  aNode          - [IN]  Iterator를 통해 검색을 원하는 노드의 Previous 노드
*  aIterator      - [OUT] 반환되는 노드
*
**********************************************************************/
#define IDU_LIST_ITERATE_AFTER2LAST(aHead, aNode, aIterator)            \
    for ((aIterator) = (aNode)->mNext;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mNext)
/*
 * Example :
 *      struct { iduList mList; int a; } sBar;
 *      iduList * sIterator;
 *      iduList * sNodeNext;
 *      IDU_LIST_ITERATE_SAFE(&sBar.mList, sIterator, sNodeNext)
 *      {
 *          ..... whatever you want to do
 *      }
 */

#define IDU_LIST_ITERATE_SAFE(aHead, aIterator, aNodeNext)              \
    for((aIterator) = (aHead)->mNext, (aNodeNext) = (aIterator)->mNext; \
        (aIterator) != (aHead);                                         \
        (aIterator) = (aNodeNext), (aNodeNext) = (aIterator)->mNext)

#define IDU_LIST_ITERATE_BACK_SAFE(aHead, aIterator, aNodeNext)         \
    for((aIterator) = (aHead)->mPrev, (aNodeNext) = (aIterator)->mPrev; \
        (aIterator) != (aHead);                                         \
        (aIterator) = (aNodeNext), (aNodeNext) = (aIterator)->mPrev)

//fix PROJ-1596
#define IDU_LIST_ITERATE_SAFE2(aHead, aList, aIterator, aNodeNext)      \
    for((aIterator) = aList, (aNodeNext) = (aIterator)->mNext;          \
        (aIterator) != (aHead);                                         \
        (aIterator) = (aNodeNext), (aNodeNext) = (aIterator)->mNext)

/*
 * Head List 초기화
 */

#define IDU_LIST_INIT(aList)                                            \
    do                                                                  \
    {                                                                   \
        (aList)->mPrev = (aList);                                       \
        (aList)->mNext = (aList);                                       \
        (aList)->mObj  = NULL;                                          \
    } while (0)

/*
 * List Node 초기화
 */

#define IDU_LIST_INIT_OBJ(aList, aObj)                                  \
    do                                                                  \
    {                                                                   \
        (aList)->mPrev = (aList);                                       \
        (aList)->mNext = (aList);                                       \
        (aList)->mObj  = (aObj);                                        \
    } while (0)

/*
 * =====================================
 * List 에 item 추가하는 루틴들
 * =====================================
 *
 * 1. IDU_LIST_ADD_AFTER(aNode1, aNode2)
 *    aNode1 앞에 aNode2 추가
 *
 * 2. IDU_LIST_ADD_BEFORE(aNode1, aNode2)
 *    aNode1 뒤에 aNode2 추가
 *
 * 3. IDU_LIST_ADD_FIRST
 *    List 처음에 aNode 추가
 *
 *    삽입 전
 *     +------------------------------------------------+
 *     |                                                |
 *     +--> head <---> item1 <---> item2 <---> item3 <--+
 *                 ^
 *                 |
 *                 +-----삽입 포인트
 *    삽입 후
 *     +----------------------------------------------------------+
 *     |                                                          |
 *     +--> head <---> NEW <---> item1 <---> item2 <---> item3 <--+
 *
 *
 * 4. IDU_LIST_ADD_LAST
 *    List 마지막에 aNode 추가
 *
 *    삽입 전
 *     +--------------------------------------------------+
 *     |                                                  |
 *     +---> head <---> item1 <---> item2 <---> item3 <---+
 *                                                      ^
 *                                                      |
 *                        삽입 포인트-------------------+
 *    삽입 후
 *     +----------------------------------------------------------+
 *     |                                                          |
 *     +--> head <---> item1 <---> item2 <---> item3 <---> NEW <--+
 */

#define IDU_LIST_ADD_AFTER(aNode1, aNode2)                              \
    do                                                                  \
    {                                                                   \
        (aNode1)->mNext->mPrev = (aNode2);                              \
        (aNode2)->mNext        = (aNode1)->mNext;                       \
        (aNode2)->mPrev        = (aNode1);                              \
        (aNode1)->mNext        = (aNode2);                              \
    } while (0)


#define IDU_LIST_ADD_BEFORE(aNode1, aNode2)                             \
    do                                                                  \
    {                                                                   \
        (aNode1)->mPrev->mNext = (aNode2);                              \
        (aNode2)->mPrev        = (aNode1)->mPrev;                       \
        (aNode1)->mPrev        = (aNode2);                              \
        (aNode2)->mNext        = (aNode1);                              \
    } while (0)


#define IDU_LIST_ADD_FIRST(aHead, aNode) IDU_LIST_ADD_AFTER(aHead, aNode)

#define IDU_LIST_ADD_LAST(aHead, aNode)  IDU_LIST_ADD_BEFORE(aHead, aNode)

/*
 * ===================
 * List에서 aNode 삭제
 * ===================
 */

#define IDU_LIST_REMOVE(aNode)                                          \
    do                                                                  \
    {                                                                   \
        (aNode)->mNext->mPrev = (aNode)->mPrev;                         \
        (aNode)->mPrev->mNext = (aNode)->mNext;                         \
    } while (0)

/*
 * ======================
 * List가 비어있는지 검사
 * ======================
 */

#define IDU_LIST_IS_EMPTY(aList)                                        \
    ((((aList)->mPrev == (aList)) && ((aList)->mNext == (aList)))       \
     ? ID_TRUE : ID_FALSE)

/*
 * ==============================
 *
 * List에 다른 List의 Node를 합침
 *
 * ==============================
 *
 * ---------------------
 * 1. IDU_LIST_JOIN_NODE
 * ---------------------
 *
 *      Join 전
 *       +------------------------------------+
 *       |                                    |
 *       +--> List <---> item1 <---> item2 <--+
 *
 *                  +-------------------+
 *                  |                   |
 *       aNode --> ITEMA <---> ITEMB <--+
 *
 *      Join 후
 *       +------------------------------------------------------------+
 *       |                                                            |
 *       +--> List <---> item1 <---> item2 <---> ITEMA <---> ITEMB <--+
 *                                                 ^
 *                                                 |
 *             aNode ------------------------------+
 *
 * ---------------------
 * 2. IDU_LIST_JOIN_LIST
 * ---------------------
 *      - aList2가 비어있으면 아무 일도 없음
 *      - 합쳐진 후 aList2는 초기화됨
 *
 *      Join 전
 *       +------------------------------------+
 *       |                                    |
 *       +--> List1 <--> item1 <---> item2 <--+
 *
 *       +------------------------------------------------+
 *       |                                                |
 *       +--> List2 <--> ITEMA <---> ITEMB <---> ITEMC <--+
 *
 *      Join 후
 *       +------------------------------------------------------------------------+
 *       |                                                                        |
 *       +--> List1 <--> item1 <---> item2 <---> ITEMA <---> ITEMB <---> ITEMC <--+
 *
 *       +--> List2 <--+
 *       |             |
 *       +-------------+
 */

#define IDU_LIST_JOIN_NODE(aList, aNode)                                \
    do                                                                  \
    {                                                                   \
        iduList *sTmpNod;                                               \
                                                                        \
        (aList)->mPrev->mNext = (aNode);                                \
        (aNode)->mPrev->mNext = (aList);                                \
                                                                        \
        sTmpNod               = (aList)->mPrev;                         \
        (aList)->mPrev        = (aNode)->mPrev;                         \
        (aNode)->mPrev        = sTmpNod;                                \
    } while (0)

#define IDU_LIST_JOIN_LIST(aList1, aList2)                              \
    do                                                                  \
    {                                                                   \
        if (IDU_LIST_IS_EMPTY(aList2) != ID_TRUE)                       \
        {                                                               \
            IDU_LIST_REMOVE(aList2);                                    \
            IDU_LIST_JOIN_NODE(aList1, (aList2)->mNext);                \
            IDU_LIST_INIT(aList2);                                      \
        }                                                               \
    } while (0)

/*
 * ==============================
 *
 * List 를 두개의 List 로 나누기
 *
 * ==============================
 *
 * ----------------------
 * 1. IDU_LIST_SPLIT_LIST
 * ----------------------
 *   aSourceList 에서 aNode 부터 끝까지 뚝 떼어서 aNewList 를 만든다
 *
 *   a. aSourceList 와 aNode 가 같거나, aSourceList 가 비어 있는 경우
 *      아무일도 안일어난다.
 *
 *   b. aSourceList 에 노드가 하나밖에 없는 경우
 *
 *      Split 전
 *       +-------------------------------+
 *       |                               |
 *       +--> aSourceList <---> item1 <--+
 *                                ^
 *       +----------------+       |
 *       |                |       |
 *       +--> aNewList <--+       +-- aNode
 *
 *      Split 후
 *       +-------------------+
 *       |                   |
 *       +--> aSourceList <--+
 *
 *       +----------------------------+
 *       |                            |
 *       +--> aNewList <---> item1 <--+
 *                             ^
 *                             |
 *                             +------- aNode
 *   c. 일반적인 경우
 *
 *      Split 전
 *       +-------------------------------------------------------------------+
 *       |                                                                   |
 *       +--> aSourceList <---> item1 <---> item2 <---> item3 <---> item4 <--+
 *                                                        ^
 *       +----------------+                               |
 *       |                |                               |
 *       +--> aNewList <--+          aNode ---------------+
 *
 *      Split 후
 *       +-------------------------------------------+
 *       |                                           |
 *       +--> aSourceList <---> item1 <---> item2 <--+
 *
 *       +----------------------------------------+
 *       |                                        |
 *       +--> aNewList <---> item3 <---> item4 <--+
 *                             ^
 *                             |
 *                             +------- aNode
 *
 */

#define IDU_LIST_SPLIT_LIST(aSourceList, aNode, aNewList)           \
    do                                                              \
    {                                                               \
        if ((aNode) != (aSourceList))                               \
        {                                                           \
            if (IDU_LIST_IS_EMPTY((aSourceList)) != ID_TRUE)        \
            {                                                       \
                (aNewList)->mPrev           = (aSourceList)->mPrev; \
                (aNewList)->mNext           = (aNode);              \
                                                                    \
                (aSourceList)->mPrev        = (aNode)->mPrev;       \
                (aSourceList)->mPrev->mNext = (aSourceList);        \
                                                                    \
                (aNode)->mPrev              = (aNewList);           \
                (aNewList)->mPrev->mNext    = (aNewList);           \
            }                                                       \
        }                                                           \
    } while (0)

// Previous 얻기
#define IDU_LIST_GET_PREV(node)  (node)->mPrev

// Next 얻기
#define IDU_LIST_GET_NEXT(node)  (node)->mNext


// 처음 노드로 가기
#define IDU_LIST_GET_FIRST(base)  (base)->mNext

// 마지막 노드로 가기
#define IDU_LIST_GET_LAST(base)  (base)->mPrev

#endif /* _IDU_LIST_H_ */
