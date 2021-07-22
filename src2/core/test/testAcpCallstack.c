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
 * $Id: testAcpCallstack.c 8773 2009-11-20 08:18:36Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpCallstack.h>
#include <acpMem.h>
#include <acpStr.h>

/* Originally test.h */
#if !defined (ACL_BINARYTREE_H)
#define ACL_BINARYTREE_H

#include <acpTypes.h>
#include <acpError.h>

ACP_EXTERN_C_BEGIN

/**
 * Binary Tree Structure 
 */
typedef struct testT
{
    void*                      mObj;    /**< pointer to the object */
    struct testT*   mL;      /**< Left child of this node */
    struct testT*   mR;      /**< Right child of this node */
} testT, testNodeT;

/**
 * Callback function types to initialize node
 */
typedef acp_rc_t testInitCallbackT(void** aObj, void* aParam);

/**
 * Callback function types to finalize node
 */
typedef acp_rc_t testFinalCallbackT(void* aObj);

/**
 * Callback function types to compare two nodes
 */
typedef acp_sint32_t testCompareCallbackT
    (const void* aObj1, const void* aObj2);

/**
 * Callback function types to traverse tree
 */
typedef void testTraverseCallbackT(void* aObj);

/**
 * Initialize a tree node
 * @param aNode points a binary tree node
 * @return aNode
 */
ACP_INLINE testNodeT* testInit(
    testNodeT* aNode
    )
{
    if(NULL == aNode)
    {
        return NULL;
    }
    else
    {
        aNode->mObj = NULL;
        aNode->mL = NULL;
        aNode->mR = NULL;

        return aNode;
    }
}

/**
 * Initialize a tree node with node object
 * @param aNode points a binary tree node
 * @param aObj points an object
 * @return aNode
 */
ACP_INLINE testNodeT* testInitObj(
    testNodeT* aNode,
    void* aObj
    )
{
    if(NULL == aNode)
    {
        return NULL;
    }
    else
    {
        aNode->mObj = aObj;
        aNode->mL = NULL;
        aNode->mR = NULL;

        return aNode;
    }
}

/**
 * Initialize a tree node with node object
 * @param aNode points a binary tree node
 * @param aFunc points a function to allocate and initialize object
 * @param aRet points a acp_rc_t value to receive result of aFunc
 * @param aParam points to pass to aFunc
 * @return aNode
 */
ACP_INLINE testNodeT* testInitFunc(
    testNodeT* aNode,
    testInitCallbackT aFunc,
    acp_rc_t* aRet,
    void* aParam
    )
{
    if(NULL == aNode)
    {
        return NULL;
    }
    else
    {
        *aRet = aFunc(&(aNode->mObj), aParam);
        aNode->mL = NULL;
        aNode->mR = NULL;

        return aNode;
    }
}

/**
 * Insert a new node aNewNode as a Left Child of aNode
 * aNewNode must be newly allocated node.
 * Original left child of aNode will be the left child of aNewNode
 * @param aNode points a binary tree node
 * @param aNewNode points a binary tree node to be inserted
 * @return aNode when done or NULL when aNode or aNewNode is NULL
 */
ACP_INLINE testNodeT* testInsertLeft(
    testNodeT* aNode,
    testNodeT* aNewNode
    )
{
    if(NULL == aNode || NULL == aNewNode)
    {
        return NULL;
    }
    else
    {
        aNewNode->mL = aNode->mL;
        aNode->mL = aNewNode;

        return aNode;
    }
}

/**
 * Insert a new node aNewNode as a Right Child of aNode
 * aNewNode must be newly allocated node.
 * Original right child of aNode will be the right child of aNewNode
 * @param aNode points a binary tree node
 * @param aNewNode points the binary tree node to be inserted
 * @return aNode when done or NULL when aNode or aNewNode is NULL
 */
ACP_INLINE testNodeT* testInsertRight(
    testNodeT* aNode,
    testNodeT* aNewNode
    )
{
    if(NULL == aNode || NULL == aNewNode)
    {
        return NULL;
    }
    else
    {
        aNewNode->mR = aNode->mR;
        aNode->mR = aNewNode;

        return aNode;
    }
}

/**
 * Traverse a binary tree in inorder.
 * Left child - Root - Right Child
 * @param aRoot points root node of tree to be inserted
 * @param aNewNode points the binary tree node to be inserted
 * @param aFunc points the function to compare two nodes
 * @return aRoot
 */
testNodeT* testInsertSorted(
    testNodeT* aRoot,
    testNodeT* aNewNode,
    testCompareCallbackT aFunc
    );

/**
 * Traverse a binary tree in inorder.
 * Left child - Root - Right Child
 * @param aRoot points the root node to traverse
 * @param aFunc points the function to take action for each node
 * @return aRoot
 */
testT* testTraverseInorder(
    testT* aRoot,
    testTraverseCallbackT aFunc
    );

/**
 * Traverse a binary tree in preorder.
 * Root - Left Child - Right Child
 * @param aRoot points root node to traverse
 * @param aCallback points to a function to take action for each node
 * @return aRoot
 */
testT* testTraversePreorder(
    testT* aRoot,
    testTraverseCallbackT aFunc
    );

/**
 * Traverse a binary tree in postorder.
 * Left child - Right Child - Root
 * @param aRoot points root node to traverse
 * @param aCallback points to a function to take action for each node
 * @return aRoot
 */
testT* testTraversePostorder(
    testT* aRoot,
    testTraverseCallbackT aFunc
    );

ACP_EXTERN_C_END

#endif /*ACL_BINARYTREE_H*/

/* Originally test.c */
static testNodeT* testInsertSortedInternal
(
    testNodeT* aRoot,
    testNodeT* aNewNode,
    testCompareCallbackT aFunc
)
{
    if(NULL == aRoot)
    {
        return aNewNode;
    }
    else
    {
        acp_sint32_t sResult = aFunc(aRoot->mObj, aNewNode->mObj);
        if(sResult <= 0)
        {
            aRoot->mL = testInsertSortedInternal
                (aRoot->mL, aNewNode, aFunc);
        }
        else
        {
            aRoot->mR = testInsertSortedInternal
                (aRoot->mR, aNewNode, aFunc);
        }

        return aRoot;
    }
}

testNodeT* testInsertSorted
(
    testNodeT* aRoot,
    testNodeT* aNewNode,
    testCompareCallbackT aFunc
)
{
    if(NULL == aNewNode)
    {
        return NULL;
    }
    else
    {
        return testInsertSortedInternal(aRoot, aNewNode, aFunc);
    }
}
   


/*
 * Traverse a binary tree in inorder.
 * With stack, this function can be implemented iteratively.
 * But, for convenient and fast tests, this function is implemented 
 * recursively for now.
 */
testT* testTraverseInorder(
    testT* aRoot,
    testTraverseCallbackT aFunc
    )
{
    if(NULL == aRoot)
    {
        return NULL;
    }
    else
    {
        (void)testTraverseInorder(aRoot->mL, aFunc);
        aFunc(aRoot->mObj);
        (void)testTraverseInorder(aRoot->mR, aFunc);

        return aRoot;
    }
}

/*
 * Traverse a binary tree in preorder.
 * With stack, this function can be implemented iteratively.
 * But, for convenient and fast tests, this function is implemented 
 * recursively for now.
 */
testT* testTraversePreorder(
    testT* aRoot,
    testTraverseCallbackT aFunc
    )
{
    if(NULL == aRoot)
    {
        return NULL;
    }
    else
    {
        aFunc(aRoot->mObj);
        (void)testTraversePreorder(aRoot->mL, aFunc);
        (void)testTraversePreorder(aRoot->mR, aFunc);

        return aRoot;
    }
}

/*
 * Traverse a binary tree in postorder.
 * With stack, this function can be implemented iteratively.
 * But, for convenient and fast tests, this function is implemented 
 * recursively for now.
 */
testT* testTraversePostorder(
    testT* aRoot,
    testTraverseCallbackT aFunc
    )
{
    if(NULL == aRoot)
    {
        return NULL;
    }
    else
    {
        (void)testTraversePostorder(aRoot->mL, aFunc);
        (void)testTraversePostorder(aRoot->mR, aFunc);
        aFunc(aRoot->mObj);

        return aRoot;
    }
}

static acp_str_t gSymbolName[] =
{
    ACP_STR_CONST("testInorderTraverse"),
    ACP_STR_CONST("testTraverseInorder"),
    ACP_STR_CONST("testTree"),
    ACP_STR_CONST("main"),
    ACP_STR_CONST(NULL)
};

static acp_sint32_t gIndex;

static acp_sint32_t stepCallstack(acp_callstack_frame_t *aFrame, void *aData)
{
    acp_sint32_t sIndex;
    acp_rc_t     sRC;
    acp_bool_t   sEq;

    ACT_CHECK(aData == gSymbolName);

    if (acpStrGetLength(&aFrame->mSymInfo.mFuncName) > 0)
    {
        sIndex = ACP_STR_INDEX_INITIALIZER;

        sRC = acpStrFindCString(&aFrame->mSymInfo.mFileName,
                                "testAcpCallstack",
                                &sIndex,
                                sIndex,
                                0);
        sEq = acpStrEqual(&aFrame->mSymInfo.mFuncName, &gSymbolName[gIndex], 0);

        if (ACP_RC_IS_SUCCESS(sRC) && (sEq == ACP_TRUE))
        {
            gIndex++;
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

    return 0;
}

static void exceptionHandler(acp_sint32_t aSignal, acp_callstack_t *aCallstack)
{
    acp_rc_t sRC;

    ACP_UNUSED(aSignal);

    gIndex = 0;

    sRC = acpCallstackTrace(aCallstack, stepCallstack, gSymbolName);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpStrGetLength(&gSymbolName[gIndex]) == 0);

    acpProcExit(0);
}

void makeSEGV(acp_sint32_t aN)
{
#if !defined(__INSURE__)
    acp_sint32_t *sPtr = (acp_sint32_t *)((acp_ulong_t)aN + 1);

    if (*sPtr == aN)
    {
        *sPtr = aN + 1;
    }
    else
    {
        /* Do nothing */
    }
#endif
}

typedef struct aclTestTreeObject
{
    acp_sint32_t    mKey;
    acp_char_t*     mProvince;
    acp_char_t*     mCity;
} aclTestTreeObject;

acp_char_t* gProvinces[] =
{
    "Seoul",
    "Incheon",
    "Daejeon",
    "Gwangju",
    "Daegu",
    "Busan",
    "Ulsan",
    "Gyeonggi",
    "Gangwon",
    "Chungnam",
    "Chungbuk",
    "Jeonnam",
    "Jeonbuk",
    "Gyeongnam",
    "Gyeongbuk",
    "Jeju"
};

acp_char_t* gCities[] = 
{
    "Seoul",
    "Incheon",
    "Daejeon",
    "Gwangju",
    "Daegu",
    "Busan",
    "Ulsan",
    "Suwon",
    "Chuncheon",
    "Daejon",
    "Cheongju",
    "Muan",
    "Jeonju",
    "Changwon",
    "Daegu",
    "Jeju"
};

acp_sint32_t testCompare(const void* aObj1, const void* aObj2)
{
    aclTestTreeObject* sObj1 = (aclTestTreeObject*)aObj1;
    aclTestTreeObject* sObj2 = (aclTestTreeObject*)aObj2;

    return ((sObj2->mKey) - (sObj1->mKey));
}

acp_rc_t testInitNode(void** aObj, void* aParam)
{
    aclTestTreeObject* sObj;
    acp_rc_t sRet = acpMemAlloc((void*)&sObj, sizeof(aclTestTreeObject));

    if(ACP_RC_IS_SUCCESS(sRet))
    {
        acp_uint32_t sIndex = *((acp_uint32_t*)aParam);
        sObj->mKey = sIndex;
        sObj->mProvince = gProvinces[sIndex];
        sObj->mCity = gCities[sIndex];

        *aObj = (void*)sObj;
    }
    else
    {
        /* Do Nothing */
        /* An error occured. Oh my god! */
    }

    return sRet;
}

void testInorderTraverse(void* aObj)
{
    static acp_sint32_t nLast = 0;

    aclTestTreeObject* sObj = (aclTestTreeObject*)aObj;

    ACE_ASSERT(nLast <= sObj->mKey);
    nLast = sObj->mKey;

    if(15 == sObj->mKey)
    {
        makeSEGV(sObj->mKey);
    }
    else
    {
        /* Do nothing */
    }
}

void testFinalizeTree(testNodeT* aRoot)
{
    if(NULL == aRoot)
    {
        /* Do Nothing */
    }
    else
    {
        testFinalizeTree(aRoot->mL);
        testFinalizeTree(aRoot->mR);

        acpMemFree(aRoot->mObj);
        acpMemFree(aRoot);
    }
}

void testTree(void)
{
    testNodeT* sNode;
    testNodeT* sPNode;
    testNodeT* sPRoot;
    acp_uint32_t i;
    acp_rc_t sRet = ACP_RC_SUCCESS;
    aclTestTreeObject* sObj;

    i = 0;
    ACE_ASSERT(ACP_RC_IS_SUCCESS(
            acpMemAlloc((void**)&sNode, sizeof(testNodeT))
            ));
    sPRoot = testInitFunc(sNode, testInitNode, &sRet, &i);
    ACE_ASSERT(ACP_RC_IS_SUCCESS(sRet));
    ACE_ASSERT(sPRoot == sNode);

    for(i = 1; i < 16; i++)
    {
        ACE_ASSERT(ACP_RC_IS_SUCCESS(
                acpMemAlloc((void**)&sNode,
                            sizeof(testNodeT)
                            )
                ));

        sPNode = testInitFunc(sNode, testInitNode, &sRet, &i);
        ACE_ASSERT(ACP_RC_IS_SUCCESS(sRet));
        ACE_ASSERT(sPNode == sNode);

        sPNode = testInsertSorted(sPRoot, sNode, testCompare);
        ACE_ASSERT(sPNode == sPRoot);

        sObj = (aclTestTreeObject*)(sNode->mObj);
    }

    (void)testTraverseInorder(sPRoot, testInorderTraverse);

    testFinalizeTree(sPRoot);
}


acp_sint32_t main(void)
{
    acp_rc_t sRC;

    ACT_TEST_BEGIN();

    sRC = acpSignalSetExceptionHandler(exceptionHandler);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    testTree();

    ACT_TEST_END();

    return 0;
}
