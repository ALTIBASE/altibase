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
 
#include <idl.h>
#include <act.h>
#include <iduVarMemList.h>
#include <ideErrorMgr.h>
#include <idu.h>
#include <idp.h>

#define IDU_TEST_BUFFER_SIZE 256
#define TEST_THREAD_COUNT  8
idCoreAcpThr gThrHandle[TEST_THREAD_COUNT];
vSLong gThrNum[TEST_THREAD_COUNT];

SInt doTest(void *)
{
    iduVarMemList       pool;
    iduVarMemListStatus oldPos;
    char           * p1 = NULL;
    char          * p2 = NULL;
    char          * p3 = NULL;
    iduList *sPoolList;

    ACT_ASSERT(pool.init(IDU_MEM_OTHER) == IDE_SUCCESS);

    ACT_ASSERT(pool.init(IDU_MEM_OTHER) == IDE_SUCCESS);
    
    ACT_ASSERT(pool.alloc( (size_t)8, (void **)&p1 ) == IDE_SUCCESS);
    p1[0] = 'a';
    p1[7] = 'z';

    ACT_ASSERT_DESC(pool.getStatus( &oldPos ) == IDE_SUCCESS,
                    ("getStatus failed"));

    ACT_CHECK(oldPos.mCursor != NULL);

    ACT_ASSERT(pool.alloc( (size_t)(IDU_TEST_BUFFER_SIZE - 8),
                           (void **)&p2 ) == IDE_SUCCESS);
    p2[0] = 'a';
    p2[7] = 'z';

    ACT_ASSERT(pool.alloc( (size_t)(IDU_TEST_BUFFER_SIZE),
                           (void **)&p3 ) == IDE_SUCCESS);
    p3[0] = 'a';
    p3[7] = 'z';
    
    // 8 + (IDU_TEST_BUFFER_SIZE - 8) + IDU_TEST_BUFFER_SIZE + 1(from getStatus())
    ACT_CHECK(pool.getAllocSize() == 513);
    ACT_CHECK(pool.getAllocCount() == 4);

    // get the first node
    sPoolList = pool.getNodeList()->mNext;

    ACT_ASSERT(sPoolList != NULL);

    ACT_CHECK(*(ULong *)sPoolList->mObj == 8);
    ACT_CHECK(*(ULong *)sPoolList->mNext->mObj == 1);
    ACT_CHECK(*(ULong *)sPoolList->mNext->mNext->mObj == (IDU_TEST_BUFFER_SIZE - 8));
    ACT_CHECK(*(ULong *)sPoolList->mNext->mNext->mNext->mObj == IDU_TEST_BUFFER_SIZE);

    ACT_ASSERT(pool.setStatus( &oldPos ) == IDE_SUCCESS);

    // 8 + 1(from getStatus)
    ACT_CHECK(pool.getAllocSize() == 9);
    ACT_CHECK(pool.getAllocCount() == 2);

    ACT_ASSERT(pool.freeAll() == IDE_SUCCESS);

    ACT_ASSERT(pool.freeAll() == IDE_SUCCESS);
    
    ACT_ASSERT(pool.destroy() == IDE_SUCCESS);

    ACT_ASSERT(pool.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

int main()
{
    SInt i;
    
    ACT_TEST_BEGIN();
    
    // idp and iduMutexMgr must be initlaized at the first of program
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);

    iduMutexMgr::initializeStatic(IDU_SERVER_TYPE);
    ACT_ASSERT_DESC(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS,
                    ("cannot initialize iduMemMgr"));

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        ACT_ASSERT(acpThrCreate(&gThrHandle[i],
                                NULL,
                                doTest,
                                NULL) == ACP_RC_SUCCESS);
    }
    
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        (void)acpThrJoin(&gThrHandle[i], NULL);
    }

    ACT_TEST_END();

    return 0;
}
