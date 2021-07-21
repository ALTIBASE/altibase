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
 * $Id: $
 ******************************************************************************/

#if !defined(_O_ACL_SAFELIST_H_)
#define _O_ACL_SAFELIST_H_

#include <acp.h>

ACP_EXTERN_C_BEGIN

typedef struct acl_safelist_node_t {
    struct acl_safelist_node_t*     mPrev;      /* Previous Node */
    struct acl_safelist_node_t*     mNext;      /* <Next Node, Deleted> */
    void*                           mData;      /* Data pointer */
} acl_safelist_node_t;

typedef struct acl_safelist_t {
    acl_safelist_node_t     mHead;      /* Head Dummy Node */
    acl_safelist_node_t     mTail;      /* Tail Dummy Node */
} acl_safelist_t;

ACP_EXPORT acp_rc_t    aclSafeListCreate(acl_safelist_t* aList);
ACP_EXPORT acp_rc_t    aclSafeListDestroy(acl_safelist_t* aList);

ACP_EXPORT acp_rc_t    aclSafeListNext(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t**   aNode);
ACP_EXPORT acp_rc_t    aclSafeListPrev(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t**   aNode);

ACP_INLINE acp_rc_t    aclSafeListFirst(
    acl_safelist_t*         aList,
    acl_safelist_node_t**   aNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNode = NULL;

    sRC = aclSafeListNext(aList, &(aList->mHead), &sNode);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    ACP_TEST_RAISE(sNode == &(aList->mTail), EEMPTY);

    *aNode = sNode;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EEMPTY)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION_END;
    return sRC;
}
ACP_INLINE acp_rc_t    aclSafeListLast(
    acl_safelist_t*         aList,
    acl_safelist_node_t**   aNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNode = NULL;

    sRC = aclSafeListPrev(aList, &(aList->mTail), &sNode);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    ACP_TEST_RAISE(sNode == &(aList->mHead), EEMPTY);

    *aNode = sNode;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(EEMPTY)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

ACP_EXPORT acp_rc_t    aclSafeListInsertAfter(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t*    aNewNode);

ACP_INLINE acp_rc_t    aclSafeListPushHead(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aNewNode)
{
    return aclSafeListInsertAfter(aList, &(aList->mHead), aNewNode);
}

ACP_EXPORT acp_rc_t    aclSafeListPopHead(
    acl_safelist_t*         aList,
    acl_safelist_node_t**   aPopNode);


ACP_EXPORT acp_rc_t    aclSafeListInsertBefore(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t*    aNewNode);

ACP_INLINE acp_rc_t    aclSafeListPushBack(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aNewNode)
{
    return aclSafeListInsertBefore(aList, &(aList->mTail), aNewNode);
}


ACP_EXPORT acp_bool_t  aclSafeListIsNodeAlive(acl_safelist_node_t* aNode);
ACP_EXPORT acp_rc_t    aclSafeListDeleteNode(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aNode,
    acp_bool_t              aRetry);

ACP_EXTERN_C_END

#endif

