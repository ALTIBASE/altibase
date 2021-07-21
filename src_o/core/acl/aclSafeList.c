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
 
#include <aclSafeList.h>
#include <ace.h>

#define ACL_SAFELIST_MARKER ((acp_ulong_t)(0x01))

#define ACL_SAFELIST_LIVENODE ((acp_ulong_t)(0x00))
#define ACL_SAFELIST_DEADNODE ((acp_ulong_t)(0x01))

#define ACL_SAFELIST_GETMARKER(aRef)                            \
    ((acp_ulong_t)aRef & ACL_SAFELIST_MARKER)
#define ACL_SAFELIST_GETPOINTER(aRef)                           \
    (acl_safelist_node_t*)((acp_ulong_t)aRef & ~ACL_SAFELIST_MARKER)

#define ACL_SAFELIST_MAKEREF(aRef, aMark)                       \
    (acl_safelist_node_t*)((aMark == ACL_SAFELIST_LIVENODE)?        \
     ((acp_ulong_t)aRef & ~ACL_SAFELIST_MARKER) :               \
     ((acp_ulong_t)aRef | ACL_SAFELIST_MARKER))

#define ACL_SAFELIST_GETNEXT(aNode)                   \
        ACL_SAFELIST_GETPOINTER(acpAtomicGet(&(aNode->mNext)))

#define ACL_SAFELIST_GETPREV(aNode)                   \
        ((acl_safelist_node_t*)(acpAtomicGet(&aNode->mPrev)))

#define ACL_SAFELIST_SETNEXT(aNode, aPointer, aFlag)            \
        (void)acpAtomicSet(&(aNode->mNext),                     \
                           (acp_slong_t)ACL_SAFELIST_MAKEREF(aPointer, aFlag))
#define ACL_SAFELIST_SETPREV(aNode, aPointer)                   \
        (void)acpAtomicSet(&(aNode->mPrev), (acp_slong_t)aPointer)

ACP_INLINE acp_bool_t aclSafeListCASNext(
    acl_safelist_node_t* aThisNode,
    acl_safelist_node_t* aOldRef, acp_ulong_t aOldMark,
    acl_safelist_node_t* aNewRef, acp_ulong_t aNewMark)
{
    return acpAtomicCasBool(
        &(aThisNode->mNext),
        (acp_slong_t)ACL_SAFELIST_MAKEREF(aNewRef, aNewMark),
        (acp_slong_t)ACL_SAFELIST_MAKEREF(aOldRef, aOldMark)
        );
}

/**
 * Is @a aNode alive?
 * @return #ACP_TRUE if @a aNode is alive, #ACP_FALSE if dead
 */
ACP_EXPORT acp_bool_t aclSafeListIsNodeAlive(acl_safelist_node_t* aNode)
{
    return (ACL_SAFELIST_GETMARKER(acpAtomicGet(&(aNode->mNext)))
            == ACL_SAFELIST_LIVENODE)? ACP_TRUE:ACP_FALSE;
}


/*
 * Update mPrev according to mNext.
 * This make the mPrev link correspondent to the predecessor's mNext
 */
ACP_EXPORT acp_rc_t    aclSafeListCreate(acl_safelist_t* aList)
{
    aList->mHead.mPrev = NULL;
    aList->mTail.mNext = NULL;
    aList->mHead.mNext = &(aList->mTail);
    aList->mTail.mPrev = &(aList->mHead);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t aclSafeListDestroy(acl_safelist_t* aList)
{
    ACP_UNUSED(aList);
    return ACP_RC_SUCCESS;
}

/**
 * Next node of @a aAnchorNode.
 * Finds next node of @a aAnchorNode and store it into @a *aNode.
 * @param aList Thread-safe doubly linked list structure
 * @param aAnchorNode Node to find the next node of which
 * @param aNode Node pointer to store next node of @a aAnchorNode
 * @return #ACP_RC_SUCCESS Finding successful
 * @return #ACP_RC_EINVAL @a aList, @a aAnchorNode or @a aNode is NULL
 * @return #ACP_RC_ENOENT No next node of @a aAnchorNode
 */
ACP_EXPORT acp_rc_t aclSafeListNext(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t**   aNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNext = NULL;
    acl_safelist_node_t*    sAnchorNext = NULL;

    ACP_TEST_RAISE((aList == NULL || aAnchorNode == NULL || aNode == NULL),
                   ENULL);
    ACP_TEST_RAISE(aAnchorNode == &(aList->mTail), EBEYONDLIST);

    sAnchorNext = sNext = ACL_SAFELIST_GETNEXT(aAnchorNode);

    while(ACP_TRUE)
    {
        /* End of list encountered */
        ACP_TEST_RAISE(sNext == NULL, EBEYONDLIST);

        if(aclSafeListIsNodeAlive(sNext) == ACP_TRUE)
        {
            /* Live node encountered */
            break;
        }
        else
        {
            /* Passing through dead node. */
            sNext = ACL_SAFELIST_GETNEXT(sNext);
        }
    }

    /* 
     * sNext now contains the live next node of aAnchorNode
     * CAS Next of aAnchorNode
     * if aAnchorNode is already marked as dead, this CAS does nothing
     */
    if(aclSafeListCASNext(aAnchorNode,
                          sAnchorNext,   ACL_SAFELIST_LIVENODE,
                          sNext,         ACL_SAFELIST_LIVENODE)
       == ACP_TRUE)
    {
        ACL_SAFELIST_SETPREV(sNext, aAnchorNode);
    }
    else
    {
        /* Do not update for dead node */
    }

    *aNode = sNext;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(EBEYONDLIST)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

static acp_rc_t aclSafeListPrevNext(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aBeginNode,
    acl_safelist_node_t*    aEndNode,
    acl_safelist_node_t**   aNode)
{
    acp_rc_t                sRC;

    acl_safelist_node_t*    sNext = NULL;
    acl_safelist_node_t*    sLive = NULL;
    acl_safelist_node_t*    sLiveNext = NULL;

    ACP_UNUSED(aList);

    sLive = aBeginNode;
    sLiveNext = ACL_SAFELIST_GETNEXT(sLive);
    sNext = ACL_SAFELIST_GETNEXT(aBeginNode);

    while(sNext != ACL_SAFELIST_GETNEXT(aEndNode))
    {
        /* aBeginNode or aEndNode was deleted during search */
        ACP_TEST_RAISE(aclSafeListIsNodeAlive(aBeginNode) == ACP_FALSE,
                       ENOSUCHANCHOR);
        ACP_TEST_RAISE(aclSafeListIsNodeAlive(aEndNode) == ACP_FALSE,
                       ENOSUCHANCHOR);

        if(aclSafeListIsNodeAlive(sNext) == ACP_TRUE)
        {
            (void)aclSafeListCASNext(sLive,
                                     sLiveNext, ACL_SAFELIST_LIVENODE,
                                     sNext,     ACL_SAFELIST_LIVENODE);
            ACL_SAFELIST_SETPREV(sNext, sLive);

            sLive = sNext;
            sLiveNext = ACL_SAFELIST_GETNEXT(sLive);
        }
        else
        {
            /* Pass dead node */
        }
            
        sNext = ACL_SAFELIST_GETNEXT(sNext);
    }

    /* sLive is the most-forward live node */
    *aNode = sLive;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENOSUCHANCHOR)
    {
        sRC = ACP_RC_ESRCH;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Previous node of @a aAnchorNode.
 * Finds previous node of @a aAnchorNode and store it into @a *aNode.
 * @param aList Thread-safe doubly linked list structure
 * @param aAnchorNode Node to find the previous node of which
 * @param aNode Node pointer to store previous node of @a aAnchorNode
 * @return #ACP_RC_SUCCESS Finding successful
 * @return #ACP_RC_EINVAL @a aList, @a aAnchorNode or @a aNode is NULL
 * @return #ACP_RC_ENOENT No previous node of @a aAnchorNode
 * @return #ACP_RC_ESRCH  @a aAnchorNode was deleted during search
 */
ACP_EXPORT acp_rc_t    aclSafeListPrev(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t**   aNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sPrev = NULL;

    ACP_TEST_RAISE((aList == NULL || aAnchorNode == NULL || aNode == NULL),
                   ENULL);
    ACP_TEST_RAISE(aAnchorNode == &(aList->mHead), EBEYONDLIST);

    sPrev = ACL_SAFELIST_GETPREV(aAnchorNode);

    while(ACP_TRUE)
    {
        /* aAnchor was deleted during search */
        ACP_TEST_RAISE(aclSafeListIsNodeAlive(aAnchorNode) == ACP_FALSE,
                       ENOSUCHANCHOR);

        /* End of list encountered */
        ACP_TEST_RAISE(sPrev == NULL, EBEYONDLIST);

        if(aclSafeListIsNodeAlive(sPrev) == ACP_TRUE)
        {
            if(ACL_SAFELIST_GETNEXT(sPrev) == aAnchorNode)
            {
                ACL_SAFELIST_SETPREV(aAnchorNode, sPrev);
                /* Gotcha! */
                break;
            }
            else
            {
                /* Find more-forward live node */
                sRC = aclSafeListPrevNext(aList, sPrev, aAnchorNode, &sPrev);
                ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
                break;
            }
        }
        else
        {
            /* Dead node encountered */
            sPrev = ACL_SAFELIST_GETPREV(sPrev);
        }
    }
            
    *aNode = sPrev;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(EBEYONDLIST)
    {
        sRC = ACP_RC_ENOENT;
    }
    ACP_EXCEPTION(ENOSUCHANCHOR)
    {
        sRC = ACP_RC_ESRCH;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Insert @a aNewNode into @a aList after the position of @a aAnchorNode.
 * @param aList Thread-safe doubly linked list structure
 * @param aAnchorNode Anchor node to insert @a aNewNode of which
 * @param aNewNode Node pointer storing new node
 * @return #ACP_RC_SUCCESS Insertion successful
 * @return #ACP_RC_EINVAL @a aList, @a aAnchorNode or @a aNode is NULL
 * @return #ACP_RC_ENOENT Tried to insert beyond the list
 * @return #ACP_RC_ESRCH @a aAnchorNode was deleted during insertion
 */
ACP_EXPORT acp_rc_t    aclSafeListInsertAfter(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t*    aNewNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNext = NULL;

    /* The pointer must be the multiple of 2. */
    ACE_DASSERT(((acp_ulong_t)aNewNode) % 2 == 0);

    ACP_TEST_RAISE((aList == NULL || aAnchorNode == NULL || aNewNode == NULL),
                   ENULL);
    ACP_TEST_RAISE(aAnchorNode == &(aList->mTail), EBEYONDLIST);

    while(ACP_TRUE)
    {
        /* Someone deleted aAnchorNode */
        ACP_TEST_RAISE(aclSafeListIsNodeAlive(aAnchorNode) == ACP_FALSE,
                       ENOSUCHANCHOR);

        sRC = aclSafeListNext(aList, aAnchorNode, &sNext);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        /* Link new node with proper position */
        ACL_SAFELIST_SETNEXT(aNewNode, sNext, ACL_SAFELIST_LIVENODE);
        ACL_SAFELIST_SETPREV(aNewNode, aAnchorNode);

        /* Splice into list */
        if(aclSafeListCASNext(aAnchorNode,
                              sNext,    ACL_SAFELIST_LIVENODE,
                              aNewNode, ACL_SAFELIST_LIVENODE) == ACP_TRUE)
        {
            /* We're done! */
            ACL_SAFELIST_SETPREV(sNext, aNewNode);
            break;
        }
        else
        {
            /* 
             * Someone touched the list.
             * Try again.
             */
        }
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(EBEYONDLIST)
    {
        sRC = ACP_RC_ENOENT;
    }
    ACP_EXCEPTION(ENOSUCHANCHOR)
    {
        sRC = ACP_RC_ESRCH;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

ACP_EXPORT acp_rc_t    aclSafeListInsertBefore(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aAnchorNode,
    acl_safelist_node_t*    aNewNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sPrev = NULL;

    /* The pointer must be the multiple of 2. */
    ACE_DASSERT(((acp_ulong_t)aNewNode) % 2 == 0);

    ACP_TEST_RAISE((aList == NULL || aAnchorNode == NULL || aNewNode == NULL),
                   ENULL);
    ACP_TEST_RAISE(aAnchorNode == &(aList->mHead), EBEYONDLIST);

    while(ACP_TRUE)
    {
        /* Someone deleted aAnchorNode */
        ACP_TEST_RAISE(aclSafeListIsNodeAlive(aAnchorNode) == ACP_FALSE,
                       ENOSUCHANCHOR);

        sRC = aclSafeListPrev(aList, aAnchorNode, &sPrev);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        /* Link new node with proper position */
        ACL_SAFELIST_SETNEXT(aNewNode, aAnchorNode, ACL_SAFELIST_LIVENODE);
        ACL_SAFELIST_SETPREV(aNewNode, sPrev);

        /* Splice into list */
        if(aclSafeListCASNext(sPrev,
                              aAnchorNode, ACL_SAFELIST_LIVENODE,
                              aNewNode, ACL_SAFELIST_LIVENODE) == ACP_TRUE)
        {
            /* We're done! */
            ACL_SAFELIST_SETPREV(aAnchorNode, aNewNode);
            break;
        }
        else
        {
            /* 
             * Someone touched the list.
             * Try again.
             */
        }
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(EBEYONDLIST)
    {
        sRC = ACP_RC_ENOENT;
    }
    ACP_EXCEPTION(ENOSUCHANCHOR)
    {
        sRC = ACP_RC_ESRCH;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

ACP_EXPORT acp_rc_t    aclSafeListPopHead(
    acl_safelist_t*         aList,
    acl_safelist_node_t**   aPopNode)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNode = NULL;

    ACP_TEST_RAISE(aList == NULL, ENULL);

    sRC = aclSafeListNext(aList, &(aList->mHead), aPopNode);
    /* Handle the case where one or more arguments are NULL or
       empty. */
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), ENULL);
    ACP_TEST_RAISE(*aPopNode == &(aList->mTail), EEMPTY);


    sRC = aclSafeListDeleteNode(aList, *aPopNode, ACP_TRUE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), EDELETE);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(EEMPTY)
    {
        sRC = ACP_RC_ENOENT;
    }
    ACP_EXCEPTION(EDELETE)
    {
        /* Delete failed */
    }

    ACP_EXCEPTION_END;

    return sRC;
}

/**
 * Delete @a aNode from @a aList.
 * @param aList Thread-safe doubly linked list structure
 * @param aNode Node pointer to be deleted
 * @param aRetry Retry deletion when failure,
 * except @a aNode was already deleted.
 * If @a aRetry was #ACP_TRUE, #ACP_RC_EINTR will not be returned.
 * @return #ACP_RC_SUCCESS Deletion successful
 * @return #ACP_RC_EINVAL @a aList, @a aAnchorNode or @a aNode is NULL
 * @return #ACP_RC_ESRCH @a aNode was deleted during deletion
 * @return #ACP_RC_EINTR @a aNode was modified during deletion
 */
ACP_EXPORT acp_rc_t    aclSafeListDeleteNode(
    acl_safelist_t*         aList,
    acl_safelist_node_t*    aNode,
    acp_bool_t              aRetry)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNext = NULL;
    acl_safelist_node_t*    sLiveNext = NULL;
    acl_safelist_node_t*    sLivePrev = NULL;

    ACP_TEST_RAISE((aList == NULL || aNode == NULL),
                   ENULL);

    ACP_TEST_RAISE((aNode == &(aList->mHead) || aNode == &(aList->mTail)),
                   EDELETESYS);

    while(ACP_TRUE)
    {
        ACP_TEST_RAISE(aclSafeListIsNodeAlive(aNode) == ACP_FALSE,
                       EALREADYDELETED);

        sNext = ACL_SAFELIST_GETNEXT(aNode);
        sRC   = aclSafeListNext(aList, aNode, &sLiveNext);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
        sRC   = aclSafeListPrev(aList, aNode, &sLivePrev);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

        if(aclSafeListCASNext(aNode,
                              sNext, ACL_SAFELIST_LIVENODE,
                              sNext, ACL_SAFELIST_DEADNODE) == ACP_TRUE)
        {
            /* Just for cleanup */
            (void)aclSafeListPrevNext(aList, sLivePrev, sLiveNext, &sNext);
            break;
        }
        else
        {
            /* Someone changed node. Retry? */
            ACP_TEST_RAISE(aRetry == ACP_FALSE, EINTERRUPTED);
        }
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(ENULL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(EALREADYDELETED)
    {
        sRC = ACP_RC_ESRCH;
    }
    ACP_EXCEPTION(EINTERRUPTED)
    {
        sRC = ACP_RC_EINTR;
    }
    ACP_EXCEPTION(EDELETESYS)
    {
        sRC = ACP_RC_EINVAL;
        (void)acpPrintf("Try to delete mHead/mTail\n");
    }

    ACP_EXCEPTION_END;
    return sRC;
}

