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
#include <idu.h>
#include <idp.h>
#include <iduMemMgr.h>
#include <idtContainer.h>
#include <idtBaseThread.h>
#include <ideErrorMgr.h>
#include <iduMemMgr.h>

#define TEST_ALIGNSIZE   4096
#define TEST_MALLOCSIZE  64
#define TEST_REALLOCSIZE 64

#define TEST_CHUNKSIZE (256*1024)
#define TEST_MALLOCLARGESIZE  (16 * 1024 * 1024)
#define TEST_REALLOCLARGESIZE (32 * 1024 * 1024)

#define TEST_LOOP   (1024)
#define TEST_COUNT  (1024)
#define TEST_THREADS    (32)

class testThread : public idtBaseThread
{
public:
    virtual void run(void)
    {
        SInt i;
        SInt j;

        for(i = 0; i < TEST_LOOP; i++)
        {
            for(j = 0; j < TEST_COUNT; j++)
            {
                ACT_ASSERT(iduMemMgr::malloc(
                        IDU_MEM_MMC, j * 16, &mMem[j])
                    == IDE_SUCCESS);
            }
            for(j = 0; j < TEST_COUNT; j++)
            {
                ACT_ASSERT(iduMemMgr::free(mMem[j])
                    == IDE_SUCCESS);
            }
        }
    }

private:
    void* mMem[TEST_COUNT];
};


class testInstanceThread : public idtBaseThread
{
public:
    static void initializeStatic(void)
    {
        ACT_ASSERT(mAlloc.initialize((SChar*)"TEST_MULTI_TLSF",
                                     ID_TRUE, TEST_CHUNKSIZE)
                   == IDE_SUCCESS);
    }

    static void destroyStatic(void)
    {
        ACT_ASSERT(mAlloc.destroy() == IDE_SUCCESS);
    }

    virtual void run(void)
    {
        SInt    i;
        SInt    j;
        void*   sPtr;

        for(j = 0; i < TEST_LOOP; i++)
        {
            for(i = 0; i < TEST_CHUNKSIZE; i++)
            {
                if(mAlloc.malloc(IDU_MEM_MMC, i, &sPtr) != IDE_SUCCESS)
                {
                    break;
                }
                else
                {
                    ACT_ASSERT(mAlloc.free(sPtr) == IDE_SUCCESS);
                }
            }

            ACT_CHECK_DESC(i == TEST_CHUNKSIZE,
                           ("Warning! allocation stopped at %d, not %d",
                            i, TEST_CHUNKSIZE));

            for(i = 0; i < TEST_CHUNKSIZE; i++)
            {
                if(mAlloc.malign(IDU_MEM_MMC, i, 4096, &sPtr) != IDE_SUCCESS)
                {
                    break;
                }
                else
                {
                    ACT_ASSERT(mAlloc.free(sPtr) == IDE_SUCCESS);
                }
            }

            ACT_CHECK_DESC(i == TEST_CHUNKSIZE,
                           ("Warning! allocation stopped at %d, not %d",
                            i, TEST_CHUNKSIZE));
        }
    }

private:
    static iduMemTlsfMmap   mAlloc;
};

    
testThread          gThread[TEST_THREADS];
testInstanceThread  gInstanceThread[TEST_THREADS];
iduMemTlsfMmap      testInstanceThread::mAlloc;

void testInstance(void)
{
    UInt            sType;
    iduMemTlsfMmap  sTlsf;
    SInt            i;

    void**          sPtrs;

    ACT_ASSERT(idp::read((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (void**)(&sType)) == IDE_SUCCESS);
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                (UInt)0) == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);

    sPtrs = (void**)idlOS::malloc(sizeof(void*) * TEST_CHUNKSIZE);
    ACT_ASSERT(sPtrs != NULL);

    ACT_ASSERT(sTlsf.initialize((SChar*)"TEST_TLSF", ID_FALSE, TEST_CHUNKSIZE)
               == IDE_SUCCESS);

    for(i = 0; i < TEST_CHUNKSIZE; i++)
    {
        if(sTlsf.malloc(IDU_MEM_MMC, i, &(sPtrs[i])) != IDE_SUCCESS)
        {
            break;
        }
        else
        {
            /* continue */
        }
    }

    ACT_CHECK_DESC(i == TEST_CHUNKSIZE,
                   ("Warning! allocation stopped at %d, not %d",
                    i, TEST_CHUNKSIZE));

    do
    {
        i--;
        ACT_ASSERT(sTlsf.free(sPtrs[i]) == IDE_SUCCESS);
    } while(i != 0);

    for(i = 0; i < TEST_CHUNKSIZE; i++)
    {
        if(sTlsf.malign(IDU_MEM_MMC, i, 4096, &(sPtrs[i])) != IDE_SUCCESS)
        {
            break;
        }
        else
        {
            /* continue */
        }
    }

    ACT_CHECK_DESC(i == TEST_CHUNKSIZE,
                   ("Warning! allocation stopped at %d, not %d",
                    i, TEST_CHUNKSIZE));

    do
    {
        i--;
        ACT_ASSERT(sTlsf.free(sPtrs[i]) == IDE_SUCCESS);
    } while(i != 0);


    ACT_ASSERT(sTlsf.destroy() == IDE_SUCCESS);

    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStaticContainer() == IDE_SUCCESS);

    testInstanceThread::initializeStatic();
    for(i = 0; i < TEST_THREADS; i++)
    {
        gInstanceThread[i].start();
    }

    for(i = 0; i < TEST_THREADS; i++)
    {
        gInstanceThread[i].join();
    }
    testInstanceThread::destroyStatic();

    ACT_ASSERT(idtContainer::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);
    /* Restore property */
    ACT_ASSERT(idp::updateForce((SChar*)"MEMORY_ALLOCATOR_TYPE",
                                sType) == IDE_SUCCESS);
}

void testGlobalAllocator(void)
{
    SInt    i;
    SChar*  sPtr;

    void*   sTemps[128];

    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStaticContainer() == IDE_SUCCESS);

    for(i = 0; i < 128; i++)
    {
        ACT_ASSERT(iduMemMgr::malloc(IDU_MEM_MMC, 128 * i,
                                     (void**)&sTemps[i])
                   == IDE_SUCCESS);
    }
    for(i = 0; i < 128; i++)
    {
        ACT_ASSERT(iduMemMgr::free(sTemps[i]) == IDE_SUCCESS);
    }

    ACT_ASSERT(iduMemMgr::malloc(IDU_MEM_MMC, TEST_MALLOCSIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCSIZE; i++)
    {
        sPtr[i] = (SChar)i;
    }
    ACT_ASSERT(iduMemMgr::realloc(IDU_MEM_MMC, TEST_REALLOCSIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCSIZE; i++)
    {
        ACT_ASSERT(sPtr[i] == (SChar)i);
    }
    ACT_ASSERT(iduMemMgr::free(sPtr) == IDE_SUCCESS);

    ACT_ASSERT(iduMemMgr::calloc(IDU_MEM_MMC, 1, TEST_MALLOCSIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCSIZE; i++)
    {
        ACT_ASSERT(sPtr[i] == (SChar)0);
    }
    ACT_ASSERT(iduMemMgr::realloc(IDU_MEM_MMC, TEST_REALLOCSIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCSIZE; i++)
    {
        ACT_ASSERT(sPtr[i] == 0);
    }
    ACT_ASSERT(iduMemMgr::free(sPtr) == IDE_SUCCESS);

    ACT_ASSERT(iduMemMgr::malign(IDU_MEM_MMC, TEST_MALLOCSIZE, TEST_ALIGNSIZE,
                                 (void**)&sPtr) == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    ACT_ASSERT((ULong)sPtr % TEST_ALIGNSIZE == 0);
    for(i = 0; i < TEST_MALLOCSIZE; i++)
    {
        sPtr[i] = (SChar)i;
    }
    ACT_ASSERT(iduMemMgr::realloc(IDU_MEM_MMC, TEST_REALLOCSIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCSIZE; i++)
    {
        ACT_ASSERT(sPtr[i] == (SChar)i);
    }
    ACT_ASSERT(iduMemMgr::free(sPtr) == IDE_SUCCESS);

    ACT_ASSERT(iduMemMgr::malloc(IDU_MEM_MMC, TEST_MALLOCLARGESIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCLARGESIZE; i++)
    {
        sPtr[i] = (SChar)i;
    }
    ACT_ASSERT(iduMemMgr::realloc(IDU_MEM_MMC, TEST_REALLOCLARGESIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCLARGESIZE; i++)
    {
        ACT_ASSERT(sPtr[i] == (SChar)i);
    }
    ACT_ASSERT(iduMemMgr::free(sPtr) == IDE_SUCCESS);

    ACT_ASSERT(iduMemMgr::malign(IDU_MEM_MMC, TEST_MALLOCLARGESIZE, TEST_ALIGNSIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCLARGESIZE; i++)
    {
        sPtr[i] = (SChar)i;
    }
    ACT_ASSERT(iduMemMgr::realloc(IDU_MEM_MMC, TEST_REALLOCLARGESIZE, (void**)&sPtr)
               == IDE_SUCCESS);
    ACT_ASSERT(sPtr != NULL);
    for(i = 0; i < TEST_MALLOCLARGESIZE; i++)
    {
        ACT_ASSERT(sPtr[i] == (SChar)i);
    }
    ACT_ASSERT(iduMemMgr::free(sPtr) == IDE_SUCCESS);

    ACT_ASSERT(idtContainer::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);
}

void testMultithread(void)
{
    SInt i;

    ACT_ASSERT(iduMemMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStatic(IDU_SERVER_TYPE) == IDE_SUCCESS);
    ACT_ASSERT(idtContainer::initializeStaticContainer() == IDE_SUCCESS);

    for(i = 0; i < TEST_THREADS; i++)
    {
        gThread[i].start();
    }

    for(i = 0; i < TEST_THREADS; i++)
    {
        gThread[i].join();
    }

    ACT_ASSERT(idtContainer::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMutexMgr::destroyStatic() == IDE_SUCCESS);
    ACT_ASSERT(iduMemMgr::destroyStatic() == IDE_SUCCESS);
}

SInt main( void )
{
    /* idp must be initlaized at the first of program */
    ACT_ASSERT(idp::initialize(NULL, NULL) == IDE_SUCCESS);
    ACT_ASSERT(iduProperty::load() == IDE_SUCCESS);

    /* Test an instance */
    testInstance();

#if 0
    /* Test allocator */
    testGlobalAllocator();

    /* Test multithreaded allocation */
    testMultithread();
    
#endif
    return IDE_SUCCESS;
}

