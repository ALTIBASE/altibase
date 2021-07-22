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
 * $Id: testAclList.c 4486 2009-02-10 07:37:20Z jykim $
 ******************************************************************************/

#include <acp.h>
#include <act.h>
#include <acpList.h>

#define TEST_ACP_LIST_MAX 5
#define TEST_ACP_LIST_ALPHABET_MAX 26
#define TEST_ACP_LIST_NUMBER_MAX 10

acp_list_t gHeadAlphabet;
acp_list_t gHeadNumber;

acp_list_node_t gListAlphabet[TEST_ACP_LIST_ALPHABET_MAX];
acp_list_node_t gListNumber[TEST_ACP_LIST_NUMBER_MAX];

acp_char_t gDataAlphabet[] = "abcdefghijklmnopqrstuvwxyz";
acp_char_t gDataNumber[] = "9876543210";

/* ALINT:C13-DISABLE */
acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_size_t i;
    acp_list_node_t *sNode;
    acp_list_node_t *sNext;

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();


    acpListInit(&gHeadAlphabet);
    acpListInit(&gHeadNumber);

    ACT_CHECK(acpListIsEmpty(&gHeadAlphabet) == ACP_TRUE);
    ACT_CHECK(acpListIsEmpty(&gHeadNumber) == ACP_TRUE);
    
    
    for (i = 0; i < TEST_ACP_LIST_ALPHABET_MAX; i++)
    {
        acpListInitObj(&gListAlphabet[i], (void *)&gDataAlphabet[i]);
    }

    for (i = 0; i < TEST_ACP_LIST_NUMBER_MAX; i++)
    {
        acpListInitObj(&gListNumber[i], (void *)&gDataNumber[i]);
    }

    sNode = &gHeadAlphabet;
    for (i = 0; i < TEST_ACP_LIST_ALPHABET_MAX; i++)
    {
        acpListInsertAfter(sNode, &gListAlphabet[i]);
        sNode = &gListAlphabet[i];
    }

    sNode = &gHeadNumber;
    for (i = 0; i < TEST_ACP_LIST_NUMBER_MAX; i++)
    {
        acpListInsertBefore(sNode, &gListNumber[i]);
        sNode = &gListNumber[i];
    }


    sNode = acpListGetFirstNode(&gHeadAlphabet);
    ACT_CHECK(*(acp_char_t *)(sNode->mObj) == 'a');
    sNode = acpListGetLastNode(&gHeadAlphabet);
    ACT_CHECK(*(acp_char_t *)(sNode->mObj) == 'z');

    sNode = acpListGetNextNode(&gListNumber[5]); /* next of 4 */
    ACT_CHECK(*(acp_char_t *)(sNode->mObj) == '5');
    sNode = acpListGetPrevNode(&gListNumber[5]); /* prev of 4 */
    ACT_CHECK(*(acp_char_t *)(sNode->mObj) == '3');


    i = 0;
    ACP_LIST_ITERATE(&gHeadAlphabet, sNode)
    {
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataAlphabet[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }

    i = 0;
    ACP_LIST_ITERATE_BACK(&gHeadNumber, sNode)
    {
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataNumber[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }


    /* acpListPrependNode only calls acpListInsertAfter
       so that no test needs for acpListPrependNode */
    /* acpListAppendNode only calls acpListInsertBefore
       so that no test needs for acpListAppendNode */


    acpListAppendList(&gHeadAlphabet, &gHeadNumber);
    acpListSplit(&gHeadAlphabet,
                 &gListNumber[TEST_ACP_LIST_NUMBER_MAX-1],
                 &gHeadNumber);

    i = 0;
    ACP_LIST_ITERATE(&gHeadAlphabet, sNode)
    {
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataAlphabet[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }

    i = 0;
    ACP_LIST_ITERATE_BACK(&gHeadNumber, sNode)
    {
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataNumber[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }

    
    acpListPrependList(&gHeadAlphabet, &gHeadNumber);
    acpListSplit(&gHeadAlphabet, &gListAlphabet[0], &gHeadNumber);

    /* Finally gHeadAlphabet has number data and */
    /* gHeadNumber has alphabet data. */

    i = 0;
    ACP_LIST_ITERATE_BACK(&gHeadAlphabet, sNode)
    {
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataNumber[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }

    i = 0;
    ACP_LIST_ITERATE(&gHeadNumber, sNode)
    {
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataAlphabet[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }

    i = 0;
    ACP_LIST_ITERATE_BACK_SAFE(&gHeadAlphabet, sNode, sNext)
    {
        acpListDeleteNode(sNode);
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataNumber[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }

    i = 0;
    ACP_LIST_ITERATE_SAFE(&gHeadNumber, sNode, sNext)
    {
        acpListDeleteNode(sNode);
        ACT_CHECK_DESC(((acp_char_t *)(sNode->mObj)) == &gDataAlphabet[i],
                       ("node value = %c\n", *(acp_char_t *)(sNode->mObj))
            );
        i++;
    }


    /* come empty, return empty */
    ACT_CHECK(acpListIsEmpty(&gHeadAlphabet) == ACP_TRUE);
    ACT_CHECK(acpListIsEmpty(&gHeadNumber) == ACP_TRUE);

    ACT_TEST_END();

    return 0;
}
/* ALINT:C17-ENABLE */
