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
#include <iduMemory.h>
#include <ideErrorMgr.h>
#include <idu.h>
#include <idp.h>

#define IDU_TEST_BUFFER_SIZE (65536)
#define IDUM_HEADER_SIZE ( idlOS::align8((UInt)sizeof(iduMemoryHeader)) )
#define TEST_THREAD_COUNT  8
acp_thr_t gThrHandle[TEST_THREAD_COUNT];
vSLong gThrNum[TEST_THREAD_COUNT];


SInt doTest(void *)
{
    iduMemory       pool;
    iduMemoryStatus pos;
    iduMemoryStatus oldPos;
    UInt            i = 0;
    char           * p1 = NULL;
    char          * p2 = NULL;
    char          * p3 = NULL;
    char *p4 = NULL;

    pool.init(IDU_MEM_OTHER, IDU_TEST_BUFFER_SIZE);

    // Normal extend
    ACT_ASSERT_DESC(pool.getStatus( &pos ) == IDE_SUCCESS,
                    ("getStatus failed"));
    ACT_CHECK(pos.mSavedCurr == NULL);
    ACT_CHECK(pos.mSavedCursor == 0);
        
    ACT_ASSERT(pool.alloc( (size_t)8, (void **)&p1 ) == IDE_SUCCESS);

    ACT_ASSERT_DESC(pool.getStatus( &oldPos ) == IDE_SUCCESS,
                    ("getStatus failed"));
    ACT_CHECK(oldPos.mSavedCurr != NULL);
    ACT_CHECK(oldPos.mSavedCursor == (8+IDUM_HEADER_SIZE));

    ACT_ASSERT(pool.alloc( (size_t)(IDU_TEST_BUFFER_SIZE - IDUM_HEADER_SIZE - 8),
                           (void **)&p2 ) == IDE_SUCCESS);
    ACT_ASSERT(pool.alloc( (size_t)(IDU_TEST_BUFFER_SIZE - IDUM_HEADER_SIZE),
                           (void **)&p3 ) == IDE_SUCCESS);

    IDE_ASSERT(pool.setStatus( &oldPos ) == IDE_SUCCESS);

    ACT_ASSERT_DESC(pool.getStatus( &pos ) == IDE_SUCCESS,
                    ("getStatus failed"));
    ACT_CHECK(oldPos.mSavedCurr == pos.mSavedCurr);
    ACT_CHECK(oldPos.mSavedCursor == pos.mSavedCursor);

    // cralloc is implemented with alloc,
    // only memory clearing is added.
    ACT_ASSERT(pool.cralloc( (size_t)8, (void **)&p4 ) == IDE_SUCCESS);

    for (i = 0; i < 8; i++)
    {
        ACT_CHECK(p4[i] == 0);
    }

    pool.destroy();

    return IDE_SUCCESS;
}


int main()
{
    SInt i;

    ACT_TEST_BEGIN();
    
    // iduMemory needs iduMemMgr and some properties    
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);

    ACT_ASSERT_DESC(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS,
                    ("cannot initialize iduMemMgr"));
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);

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

    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);

    ACT_TEST_END();

    return 0;
}
