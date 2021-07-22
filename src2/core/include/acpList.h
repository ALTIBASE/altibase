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
 
/*******************************************************************************
 * $Id: acpList.h 3800 2008-12-02 05:36:48Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_LIST_H_)
#define _O_ACP_LIST_H_

/**
 * @file
 * @ingroup CoreCollection
 *
 * doubly circularly linked list
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN


/**
 * type for list head (same to the node, but no data)
 */
typedef struct acp_list_t acp_list_t;

/**
 * type for list node
 */
typedef struct acp_list_t acp_list_node_t;

/**
 * structure for list node
 */
struct acp_list_t
{
    acp_list_t *mPrev; /**< pointer to the previous list node      */
    acp_list_t *mNext; /**< pointer to the next list node          */
    void       *mObj;  /**< pointer to the object of the list node */
};


/**
 * do an interation forward
 * @param aHead pointer to the list head
 * @param aIterator pointer to the current list node
 *
 * the list node pointed by @a aIterator must not be deleted in the loop
 *
 * <pre>
 * Example :
 *      struct { acp_list_t mList; int a; } sBar;
 *      acp_list_t * sIterator;
 *      ACP_LIST_ITERATE(&sBar.mList, sIterator)
 *      {
 *          ..... whatever you want to do
 *          Neve delete any list node!
 *          If you want to delete a list node,
 *          you must use ACP_LIST_ITERATE_SAFE() instead of ACP_LIST_ITERATE().
 *      }
 * </pre>
 */
#define ACP_LIST_ITERATE(aHead, aIterator)                              \
    for ((aIterator) = (aHead)->mNext;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mNext)

/**
 * do an interation forward from a specified node
 * @param aHead pointer to the list head
 * @param aNode pointer to the list node to begin iteration
 * @param aIterator pointer to the current list node
 */
#define ACP_LIST_ITERATE_FROM(aHead, aNode, aIterator)                  \
    for ((aIterator) = aNode;                                           \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mNext)

/**
 * do an interation backward
 * @param aHead pointer to the list head
 * @param aIterator pointer to the current list node
 *
 * the list node pointed by @a aIterator must not be deleted in the loop
 */
#define ACP_LIST_ITERATE_BACK(aHead, aIterator)                         \
    for ((aIterator) = (aHead)->mPrev;                                  \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aIterator)->mPrev)

/**
 * do an interation forward with node-deletion safe
 * @param aHead pointer to the list head
 * @param aIterator pointer to the current list node
 * @param aNextNode pointer to the next list node
 *
 * <pre>
 * Example :
 *      struct { acp_list_t mList; int a; } sBar;
 *      acp_list_t * sIterator;
 *      acp_list_t * sNextNode;
 *      ACP_LIST_ITERATE_SAFE(&sBar.mList, sIterator, sNextNode)
 *      {
 *          ..... whatever you want to do
 *      }
 * </pre>
 */
#define ACP_LIST_ITERATE_SAFE(aHead, aIterator, aNextNode)              \
    for ((aIterator) = (aHead)->mNext,                                  \
             (aNextNode) = (aIterator)->mNext;                          \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aNextNode),                                     \
             (aNextNode) = (aIterator)->mNext)

/**
 * do an interation backward with node-deletion safe
 * @param aHead pointer to the list head
 * @param aIterator pointer to the current list node
 * @param aNextNode pointer to the next list node
 */
#define ACP_LIST_ITERATE_BACK_SAFE(aHead, aIterator, aNextNode)         \
    for ((aIterator) = (aHead)->mPrev,                                  \
             (aNextNode) = (aIterator)->mPrev;                          \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aNextNode),                                     \
             (aNextNode) = (aIterator)->mPrev)

/**
 * do an interation forward from a specified node with node-deletion safe
 * @param aHead pointer to the list head
 * @param aNode pointer to the list node to begin iteration
 * @param aIterator pointer to the current list node
 * @param aNextNode pointer to the next list node
 */
#define ACP_LIST_ITERATE_SAFE_FROM(aHead, aNode, aIterator, aNextNode)  \
    for ((aIterator) = aNode,                                           \
             (aNextNode) = (aIterator)->mNext;                          \
         (aIterator) != (aHead);                                        \
         (aIterator) = (aNextNode),                                     \
             (aNextNode) = (aIterator)->mNext)

/**
 * initializes a list
 * The initialized list has only head.
 * @param aList pointer to the list head
 */
ACP_INLINE void acpListInit(acp_list_t *aList)
{
    aList->mPrev = aList;
    aList->mNext = aList;
    aList->mObj  = NULL;
}

/**
 * initializes a list node with data object
 * @param aList pointer to the list node
 * @param aObj pointer to the data object
 */
ACP_INLINE void acpListInitObj(acp_list_node_t *aList, void *aObj)
{
    aList->mPrev = aList;
    aList->mNext = aList;
    aList->mObj  = aObj;
}

/**
 * get a first node of a list
 * @param aHead pointer to the list header
 */
ACP_INLINE acp_list_node_t * acpListGetFirstNode(acp_list_t *aHead)
{
    return aHead->mNext;
}

/**
 * get a last node of a list
 * @param aHead pointer to the list header
 */
ACP_INLINE acp_list_node_t * acpListGetLastNode(acp_list_t *aHead)
{
    return aHead->mPrev;
}

/**
 * get a next node of a node
 * @param aNode pointer to a node
 */
ACP_INLINE acp_list_node_t * acpListGetNextNode(acp_list_node_t *aNode)
{
    return aNode->mNext;
}

/**
 * get a previous node of a node
 * @param aNode pointer to a node
 */
ACP_INLINE acp_list_node_t * acpListGetPrevNode(acp_list_node_t *aNode)
{
    return aNode->mPrev;
}

/**
 * checks whether the list is empty or not
 * @param aList pointer to the list node
 * @return #ACP_TRUE if empty, #ACP_FALSE if not
 */
ACP_INLINE acp_bool_t acpListIsEmpty(acp_list_t *aList)
{
    return (((aList->mPrev == aList) && (aList->mNext == aList))
            ? ACP_TRUE : ACP_FALSE);
}

/**
 * inserts a node after
 * @param aAnchorNode anchor node
 * @param aNewNode new node to insert
 */
ACP_INLINE void acpListInsertAfter(acp_list_node_t *aAnchorNode,
                                   acp_list_node_t *aNewNode)
{
    aAnchorNode->mNext->mPrev = aNewNode;
    aNewNode->mNext           = aAnchorNode->mNext;
    aNewNode->mPrev           = aAnchorNode;
    aAnchorNode->mNext        = aNewNode;
}

/**
 * inserts a node before
 * @param aAnchorNode anchor node
 * @param aNewNode new node to insert
 */
ACP_INLINE void acpListInsertBefore(acp_list_node_t *aAnchorNode,
                                    acp_list_node_t *aNewNode)
{
    aAnchorNode->mPrev->mNext = aNewNode;
    aNewNode->mPrev           = aAnchorNode->mPrev;
    aAnchorNode->mPrev        = aNewNode;
    aNewNode->mNext           = aAnchorNode;
}

/**
 * inserts a node at the beginning of a list
 * @param aList list head
 * @param aNewNode new node to insert
 *
 * <pre>
 *    Insert aNewNode at the beginning of aList
 *
 *    before
 *     +------------------------------------------------+
 *     |                                                |
 *     +--> list <---> item1 <---> item2 <---> item3 <--+
 *
 *    after
 *     +----------------------------------------------------------+
 *     |                                                          |
 *     +--> list <---> NEW <---> item1 <---> item2 <---> item3 <--+
 * </pre>
 */
ACP_INLINE void acpListPrependNode(acp_list_t      *aList,
                                   acp_list_node_t *aNewNode)
{
    acpListInsertAfter(aList, aNewNode);
}

/**
 * inserts a list at the beginning of a list
 * @param aList list head
 * @param aListToPrepend list head to prepend
 *
 * <pre>
 *    Insert aListToPrepend at the beginning of aList
 *    - If aListToPrepend is empty, nothing will be done
 *    - After prepending, aListToPrepend will be initialized
 *
 *    before
 *     +-----------------------------------+
 *     |                                   |
 *     +--> list <--> item1 <---> item2 <--+
 *
 *     +--------------------------------------------+
 *     |                                            |
 *     +--> ListToPrepend <--> ITEMA <---> ITEMB <--+
 *
 *    after
 *     +-----------------------------------------------------------+
 *     |                                                           |
 *     +--> list <--> ITEMA <---> ITEMB <---> item1 <---> item2 <--+
 *
 *     +---------------------+
 *     |                     |
 *     +--> ListToPrepend <--+
 * </pre>
 */
ACP_INLINE void acpListPrependList(acp_list_t *aList,
                                   acp_list_t *aListToPrepend)
{
    if (acpListIsEmpty(aListToPrepend) != ACP_TRUE)
    {
        aListToPrepend->mPrev->mNext = aList->mNext;
        aList->mNext->mPrev          = aListToPrepend->mPrev;
        aList->mNext                 = aListToPrepend->mNext;
        aListToPrepend->mNext->mPrev = aList;

        acpListInit(aListToPrepend);
    }
    else
    {
        /* do nothing */
    }
}

/**
 * inserts a node at the end of a list
 * @param aList list head
 * @param aNewNode new node to insert
 *
 * <pre>
 *    Insert aNewNode at the end of aList
 *
 *    before
 *     +--------------------------------------------------+
 *     |                                                  |
 *     +---> list <---> item1 <---> item2 <---> item3 <---+
 *
 *    after
 *     +----------------------------------------------------------+
 *     |                                                          |
 *     +--> list <---> item1 <---> item2 <---> item3 <---> NEW <--+
 * </pre>
 */
ACP_INLINE void acpListAppendNode(acp_list_t      *aList,
                                  acp_list_node_t *aNewNode)
{
    acpListInsertBefore(aList, aNewNode);
}

/**
 * inserts a list at the end of a list
 * @param aList list head
 * @param aListToAppend list head to append
 *
 * <pre>
 *    Insert aListToAppend at the end of aList
 *    - If aListToPrepend is empty, nothing will be done
 *    - After apending, aListToPrepend will be initialized
 *
 *    before
 *     +-----------------------------------+
 *     |                                   |
 *     +--> list <--> item1 <---> item2 <--+
 *
 *     +-------------------------------------------+
 *     |                                           |
 *     +--> ListToAppend <--> ITEMA <---> ITEMB <--+
 *
 *    after
 *     +-----------------------------------------------------------+
 *     |                                                           |
 *     +--> list <--> item1 <---> item2 <---> ITEMA <---> ITEMB <--+
 *
 *     +--------------------+
 *     |                    |
 *     +--> ListToAppend <--+
 * </pre>
 */
ACP_INLINE void acpListAppendList(acp_list_t *aList,
                                  acp_list_t *aListToAppend)
{
    if (acpListIsEmpty(aListToAppend) != ACP_TRUE)
    {
        aList->mPrev->mNext         = aListToAppend->mNext;
        aListToAppend->mNext->mPrev = aList->mPrev;
        aList->mPrev                = aListToAppend->mPrev;
        aListToAppend->mPrev->mNext = aList;

        acpListInit(aListToAppend);
    }
    else
    {
        /* do nothing */
    }
}

/**
 * deletes a node from a list
 * @param aNodeToDelete list node to delete
 */
ACP_INLINE void acpListDeleteNode(acp_list_node_t *aNodeToDelete)
{
    aNodeToDelete->mNext->mPrev = aNodeToDelete->mPrev;
    aNodeToDelete->mPrev->mNext = aNodeToDelete->mNext;
}

/**
 * splits a list to two list
 * @param aSourceList source list head
 * @param aSplitPoint list node to split from
 * @param aNewList list head to split into
 *
 * <pre>
 *   split aSourceList from aSplitPoint to the end
 *   and then splited list is merged into aNewList
 *
 *   a. If aSourceList is empty, nothing will be done
 *
 *   b. aSourceList has onle one list node
 *
 *      before
 *       +-------------------------------+
 *       |                               |
 *       +--> aSourceList <---> item1 <--+
 *                                ^
 *       +----------------+       |
 *       |                |       |
 *       +--> aNewList <--+       +-- aSplitPoint
 *
 *      after
 *       +-------------------+
 *       |                   |
 *       +--> aSourceList <--+
 *
 *       +----------------------------+
 *       |                            |
 *       +--> aNewList <---> item1 <--+
 *                             ^
 *                             |
 *                             +------- aSplitPoint
 *   c. general case
 *
 *      before
 *       +-------------------------------------------------------------------+
 *       |                                                                   |
 *       +--> aSourceList <---> item1 <---> item2 <---> item3 <---> item4 <--+
 *                                                        ^
 *       +----------------+                               |
 *       |                |                               |
 *       +--> aNewList <--+       aSplitPoint ------------+
 *
 *      after
 *       +-------------------------------------------+
 *       |                                           |
 *       +--> aSourceList <---> item1 <---> item2 <--+
 *
 *       +----------------------------------------+
 *       |                                        |
 *       +--> aNewList <---> item3 <---> item4 <--+
 *                             ^
 *                             |
 *                             +------- aSplitPoint
 * </pre>
 */
ACP_INLINE void acpListSplit(acp_list_t      *aSourceList,
                             acp_list_node_t *aSplitPoint,
                             acp_list_t      *aNewList)
{
    if ((aSplitPoint) != (aSourceList))
    {
        if (acpListIsEmpty((aSourceList)) != ACP_TRUE)
        {
            aNewList->mPrev           = aSourceList->mPrev;
            aNewList->mNext           = aSplitPoint;

            aSourceList->mPrev        = aSplitPoint->mPrev;
            aSourceList->mPrev->mNext = aSourceList;

            aSplitPoint->mPrev        = aNewList;
            aNewList->mPrev->mNext    = aNewList;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
}

ACP_EXTERN_C_END

#endif
