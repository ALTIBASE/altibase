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

/*
 * BUG-46568
 *
 * these functions are for valgrind test.
 * only linux supported.
 */


IDE_RC iduMemMgr::innocense_initializeStatic(void)
{
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::innocense_destroyStatic(void)
{
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::innocense_malloc(iduMemoryClientIndex   aIndex,
                                   ULong                  aSize,
                                   void                 **aMemPtr)
{
    IDE_DASSERT(aMemPtr != NULL);

    PDL_UNUSED_ARG(aIndex);

    *aMemPtr = (SChar*)idlOS::malloc(aSize);
    IDE_TEST(*aMemPtr == NULL);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::innocense_malign(iduMemoryClientIndex   aIndex,
                                   ULong                  aSize,
                                   ULong                  aAlign,
                                   void                 **aMemPtr)
{
    IDE_DASSERT(aMemPtr != NULL);

    PDL_UNUSED_ARG(aIndex);

#if defined(ALTI_CFG_OS_LINUX) || defined(ALTI_CFG_OS_HPUX) 
    *aMemPtr = (SChar*)memalign(aAlign, aSize);
#elif defined(ALTI_CFG_OS_AIX)
    extern void * single_posixmemalign(ULong ,ULong);
    *aMemPtr = (SChar*)single_posixmemalign(aAlign, aSize);
#else
#error not_implemented
#endif

    IDE_TEST(*aMemPtr == NULL);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerError ));
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::innocense_calloc(iduMemoryClientIndex   aIndex,
                                   vSLong                 aCount,
                                   ULong                  aSize,
                                   void                 **aMemPtr)
{
    PDL_UNUSED_ARG(aIndex);

    *aMemPtr = (SChar*)idlOS::calloc(aCount, aSize);
    IDE_TEST(*aMemPtr == NULL);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::innocense_realloc(iduMemoryClientIndex  aIndex,
                                    ULong                 aSize,
                                    void                **aMemPtr)
{
    IDE_DASSERT(aMemPtr != NULL);
    IDE_DASSERT(*aMemPtr != NULL);

    PDL_UNUSED_ARG(aIndex);

    *aMemPtr = (SChar*)idlOS::realloc(*aMemPtr, aSize);
    IDE_TEST(*aMemPtr == NULL);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemMgr::innocense_free(void     *aMemPtr)
{
    idlOS::free(aMemPtr);
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::innocense_free4malign(void                   *aMemPtr,
                                        iduMemoryClientIndex    aIndex,
                                        ULong                   aSize)
{
    PDL_UNUSED_ARG(aIndex);
    PDL_UNUSED_ARG(aSize);

    idlOS::free(aMemPtr);
    return IDE_SUCCESS;
}

IDE_RC iduMemMgr::innocense_shrink(void)
{
    return IDE_SUCCESS;
}

