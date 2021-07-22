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
 
/***********************************************************************
 * $Id:
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <iduMemMgr.h>

/***********************************************************************
 * iduMemMgr_single.cpp : IDU_SINGLE_TYPE, IDU_MEMMGR_CLIENT에서 사용
 * iduMemMgr을 서버 모드로 초기화하기 이전,
 * 혹은 client는 이 모듈을 사용한다.
 * 메모리 통계정보를 기록하지 않는다.
 **********************************************************************/
IDE_RC iduMemMgr::single_initializeStatic(void)
{
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::single_destroyStatic(void)
{
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::single_malloc(iduMemoryClientIndex   aIndex,
                                ULong                  aSize,
                                void                 **aMemPtr)
{
#define IDE_FN "iduMemMgr::single_malloc()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    PDL_UNUSED_ARG(aIndex);

    *aMemPtr = idlOS::malloc(aSize);

    IDE_TEST(*aMemPtr == NULL);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#undef IDE_FN
}

#if defined(ALTI_CFG_OS_AIX)
void* single_posixmemalign(ULong aSize, ULong aAlign)
{
    void*   sPtr;
    int     sRet;

    sRet = posix_memalign(&sPtr, aSize, aAlign);

    if(sRet != 0)
    {
        sPtr = NULL;
    }
    else
    {
        /* fall through */
    }

    return sPtr;
}
#endif

IDE_RC iduMemMgr::single_malign(iduMemoryClientIndex   aIndex,
                                ULong                  aSize,
                                ULong                  aAlign,
                                void                 **aMemPtr)
{
    void* sMemPtr = NULL;
    
    IDE_ASSERT(aSize != 0);

    PDL_UNUSED_ARG(aIndex);
#if defined(ALTI_CFG_OS_WINDOWS)
    sMemPtr = _aligned_malloc(aSize, aAlign);
#elif defined(ALTI_CFG_OS_AIX)
    sMemPtr = single_posixmemalign(aAlign, aSize);
#else
    sMemPtr = memalign(aAlign, aSize);
#endif
    IDE_TEST(sMemPtr == NULL);

    *aMemPtr = sMemPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::single_calloc(iduMemoryClientIndex   aIndex,
                                vSLong                 aCount,
                                ULong                  aSize,
                                void                 **aMemPtr)
{
#define IDE_FN "iduMemMgr::single_calloc()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    PDL_UNUSED_ARG(aIndex);
    *aMemPtr = idlOS::calloc(aCount, aSize);

    IDE_TEST(*aMemPtr == NULL);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC iduMemMgr::single_realloc(iduMemoryClientIndex  aIndex,
                                 ULong                 aSize,
                                 void                **aMemPtr)
{
#define IDE_FN "iduMemMgr::single_realloc()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    PDL_UNUSED_ARG(aIndex);

    *aMemPtr = idlOS::realloc(*aMemPtr, aSize);

    IDE_TEST(*aMemPtr == NULL);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC iduMemMgr::single_free(void                 *aMemPtr)

{
#define IDE_FN "iduMemMgr::single_free()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    idlOS::free(aMemPtr);
    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC iduMemMgr::single_free4malign(void                 *aMemPtr,
                                     iduMemoryClientIndex  aIndex,
                                     ULong                 aSize)

{
    PDL_UNUSED_ARG(aIndex);
    PDL_UNUSED_ARG(aSize);

    return single_free(aMemPtr);
}

IDE_RC iduMemMgr::single_shrink(void)
{
    /* Do nothing */
    return IDE_SUCCESS;
}

