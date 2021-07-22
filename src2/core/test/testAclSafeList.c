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
 * $Id:
 ******************************************************************************/

#include <act.h>
#include <aclSafeList.h>
#include <acp.h>

typedef struct testData
{
    acp_uint64_t mID;
    acp_sint32_t mSeq;
} testData;

#define TESTLOOPS (32768)
#define TESTOUTLOOPS (4)
#define TESTMULT (16)

acp_sint32_t gLoops;
acp_sint32_t gOutLoops;
acp_sint32_t gMult;
acp_uint32_t gInserted = 0;
acp_uint32_t gDeleted  = 0;
acp_uint32_t gThrCount = 0;
acl_safelist_t          gList;

acp_sint32_t testList(void* aParam)
{
    acl_safelist_t*         sList = (acl_safelist_t*)aParam;
    acl_safelist_node_t*    sNode;
    testData*               sData;

    acp_sint32_t            i;
    acp_sint32_t            j;
    acp_rc_t                sRC;

    (void)acpAtomicInc32(&gThrCount);

    for(j = 0; j < gOutLoops; j++)
    {
        for(i = 0; i < gLoops; i++)
        {
            sRC = acpMemAlloc((void**)&sData, sizeof(testData));
            if(ACP_RC_IS_SUCCESS(sRC))
            {
                sData->mID =  acpThrGetSelfID();
                sData->mSeq = i;
            }
            else
            {
                ACT_ERROR(("Memalloc failed! Oh, my god!!!\n"));
                continue;
            }

            sRC = acpMemAlloc((void**)&sNode, sizeof(acl_safelist_node_t));
            if(ACP_RC_IS_SUCCESS(sRC))
            {
                sNode->mData = sData;
                sRC = aclSafeListPushHead(sList, sNode);
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
                (void)acpAtomicInc32(&gInserted);
            }
            else
            {
                ACT_ERROR(("Memalloc failed! Oh, my god!!!\n"));
            }
        }

        for(i = 0; i < gLoops / 2; i++)
        {
            do
            {
                (void)aclSafeListFirst(sList, &sNode);
                sRC = aclSafeListDeleteNode(sList, sNode, ACP_TRUE);
            } while(ACP_RC_NOT_SUCCESS(sRC));
            if(ACP_RC_IS_SUCCESS(sRC))
            {
                /*
                   acpMemFree(sNode->mData);
                   acpMemFree(sNode);
                   */
                (void)acpAtomicInc32(&gDeleted);
            }
            else
            {
                /* Do nothing */
            }
        }
    }

    return 0;
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_rc_t                sRC;
    acl_safelist_node_t*    sNode;
    acp_thr_t*              sThreads;

    acp_sint32_t            i;
    acp_uint32_t            sThrCount;
    acp_sint32_t            sThrRet;
    acp_sint32_t            sFreed = 0;

    if(aArgc < 4)
    {
        gLoops      = TESTLOOPS;
        gOutLoops   = TESTOUTLOOPS;
        gMult       = TESTMULT;
    }
    else
    {
        (void)acpCStrToInt32(aArgv[1], 10, &i, &gLoops,     10, NULL);
        (void)acpCStrToInt32(aArgv[2], 10, &i, &gOutLoops,  10, NULL);
        (void)acpCStrToInt32(aArgv[3], 10, &i, &gMult,      10, NULL);
    }

    ACT_TEST_BEGIN(); 

    (void)acpSysGetCPUCount(&sThrCount);
    sThrCount *= gMult;

    (void)aclSafeListCreate(&gList);
    sRC = acpMemAlloc((void**)&sThreads, sizeof(acp_thr_t) * sThrCount);

    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Fall through */
    }
    else
    {
        ACT_ERROR(("Memalloc failed! Oh, my god!!!\n"));
        acpProcExit(1);
    }

    for(i = 0; i < sThrCount; i++)
    {
        sRC = acpThrCreate(&(sThreads[i]), NULL, testList, &gList);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    
    /* Wait for inserting threads finished */
    for(i = 0; i < sThrCount; i++)
    {
        sRC = acpThrJoin(&(sThreads[i]), &sThrRet);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sThrRet == 0);
    }

    do
    {
        (void)aclSafeListFirst(&gList, &sNode);
        sRC = aclSafeListDeleteNode(&gList, sNode, ACP_TRUE);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            sFreed++;
        }
        else
        {
            /* free failed */
        }
    } while(ACP_RC_IS_SUCCESS(sRC));

    /* Check whether all nodes were deleted */
    sRC = aclSafeListFirst(&gList, &sNode);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    ACT_CHECK_DESC((gDeleted + sFreed == gInserted),
                   ("%d + %d != %d", gDeleted, sFreed, gInserted));
    acpMemFree(sThreads);

    ACT_CHECK(gList.mHead.mNext == &(gList.mTail));
    ACT_CHECK(gList.mTail.mPrev == &(gList.mHead));

    ACT_TEST_END();

    return 0;
}
