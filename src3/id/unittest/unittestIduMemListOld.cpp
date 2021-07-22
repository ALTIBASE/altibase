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
#include <iduMemListOld.h>
#include <ideErrorMgr.h>
#include <idu.h>
#include <idp.h>

#define IDU_TEST_BUFFER_SIZE 256
#define TEST_THREAD_COUNT  8
idCoreAcpThr gThrHandle[TEST_THREAD_COUNT];

SInt doTest(void *aThrArg)
{
    SChar         *sBuf[ IDU_TEST_BUFFER_SIZE ];
    SChar const   *sMessage = "Testing in progress in unittestIduMemListOld.cpp.";
    UInt           i        = 0;
    iduMemListOld *sPool    = (iduMemListOld *)aThrArg;

    // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Test iduMemListOld::alloc()
    // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    for ( i = 0; i < IDU_TEST_BUFFER_SIZE; i++ )
    {
        ACT_ASSERT( sPool->alloc( (void**)&sBuf[ i ] ) == IDE_SUCCESS );
        acpMemCpy( sBuf[ i ], sMessage, sizeof( sMessage ) );
    }

    for ( i = 0; i < IDU_TEST_BUFFER_SIZE; i++ )
    {
        ACT_ASSERT( sPool->memfree( sBuf[ i ] ) == IDE_SUCCESS );
        sBuf[ i ] = NULL;
    }

    // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    // Test iduMemListOld::cralloc()
    // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    for ( i = 0; i < IDU_TEST_BUFFER_SIZE; i++ )
    {
        ACT_ASSERT( sPool->cralloc( (void**)&sBuf[ i ] ) == IDE_SUCCESS );
        acpMemCpy( sBuf[ i ], sMessage, sizeof( sMessage ) );
    }

    for ( i = 0; i < IDU_TEST_BUFFER_SIZE; i++ )
    {
        ACT_ASSERT( sPool->memfree( sBuf[ i ] ) == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

int main()
{
    iduMemListOld pool;
    SInt          i;
    
    ACT_TEST_BEGIN();

    // idp and iduMutexMgr must be initlaized at the first of program
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);

    iduMutexMgr::initializeStatic(IDU_SERVER_TYPE);
    ACT_ASSERT_DESC(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS,
                    ("cannot initialize iduMemMgr"));

    ACT_ASSERT(pool.initialize(IDU_MEM_OTHER,
                               0,
                               (SChar*)"unittestIduMemListOld",
                               ID_SIZEOF(char) * 256,
                               IDU_TEST_BUFFER_SIZE * TEST_THREAD_COUNT,
                               1,
                               ID_TRUE) == IDE_SUCCESS);

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        ACT_ASSERT(acpThrCreate(&gThrHandle[i],
                                NULL,
                                doTest,
                                &pool) == ACP_RC_SUCCESS);
    }
    
    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        (void)acpThrJoin(&gThrHandle[i], NULL);
    }

    ACT_ASSERT(pool.destroy() == IDE_SUCCESS);

    ACT_TEST_END();

    return 0;
}
