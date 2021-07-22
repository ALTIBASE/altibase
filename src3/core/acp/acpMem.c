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
 * $Id: acpMem.h 5945 2009-06-10 08:54:24Z jykim $
 ******************************************************************************/

/**
 * @file
 * @ingroup CoreMem
 */

#include <acpMem.h>
#include <acpError.h>

ACP_EXTERN_C_BEGIN

#define ACP_MEM_BOUNDARY_MARKER 0xDEADBEEF


/**
 * allocates @a aSize byte(s) of memory
 *
 * @param aAddr the pointer to a variable which points to the allocated memory
 * @param aSize the size of memory to allocate
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aSize is zero,
 * it returns #ACP_RC_ENOMEM if the allocation fails
 */
ACP_EXPORT acp_rc_t acpMemAlloc(void **aAddr, acp_size_t aSize)
{
    if (aSize == 0)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        HANDLE sHeap;

        sHeap = GetProcessHeap();
        if(NULL == sHeap)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            *aAddr = HeapAlloc(sHeap, 0, aSize);
            if(NULL == (*aAddr))
            {
                return ACP_RC_GET_OS_ERROR();
            }
            else
            {
                return ACP_RC_SUCCESS;
            }
        }
#else
        *aAddr = malloc(aSize);

        /* easy to trace the codesonar code */
        if (*aAddr == NULL)
        {
            return ACP_RC_ENOMEM;
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
#endif
    }
}

/**
 * allocates @a aSize byte(s) of @a aAlignSize aligned memory
 *
 * @param aAddr the pointer to a variable which points to the allocated memory
 * that it should be a multiple of aAlignSize
 * @param aSize the size of memory to allocate
 * @param aAlignSize the bourdary size of memory alignment
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aSize is zero,
 * it returns #ACP_RC_EINVAL if @a aAlignSize is zero,
 * it returns #ACP_RC_ENOMEM if the allocation fails
 */
ACP_EXPORT acp_rc_t acpMemAllocAlign(void       **aAddr,
                                     acp_size_t   aSize,
                                     acp_size_t   aAlignSize)
{
    if (aSize == 0 || aAlignSize == 0)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
#if defined(ALTI_CFG_OS_LINUX)   ||              \
    defined(ALTI_CFG_OS_SOLARIS) ||              \
    (defined(ALTI_CFG_OS_HPUX) && (ALTI_CFG_OS_MAJOR==11) && (ALTI_CFG_OS_MINOR!=0))


        *aAddr = memalign(aAlignSize, aSize);

        /* easy to trace the codesonar code */
        if (*aAddr == NULL)
        {
            return ACP_RC_ENOMEM;
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
#else
        void        *sRetPtr = NULL;
        void        *sMem = NULL;
        acp_ulong_t  sAddr;
        acp_rc_t     sRC;

        /* [1] allocate memory */
        sRC = acpMemAlloc((void **)&sMem,
                          aSize + aAlignSize + (sizeof(void *) * 2));

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            sAddr = (acp_ulong_t)sMem + aAlignSize + (sizeof(void *) * 2);

            /* [2] calculate a starting position of aligned memory */
            sRetPtr = (void *)(sAddr - (sAddr % aAlignSize));

            /* [3] store memory starting position we allocate originally */
            ((void **)sRetPtr)[-1] = sMem;

            /* [4] set bounary marker to prevent an unexpected free operation */
            ((void **)sRetPtr)[-2] = (void *)ACP_MEM_BOUNDARY_MARKER;

            *aAddr = (void **)sRetPtr;

            return ACP_RC_SUCCESS;
        }
#endif
    }
}

/**
 * allocates @a aElements * @a aSize byte(s) of zero-initialized memory
 *
 * @param aAddr the pointer to a variable which points to the allocated memory
 * @param aElements number of elements
 * @param aSize size of element in bytes
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aElements or @a aSize is zero,
 * it returns #ACP_RC_ENOMEM if the allocation fails
 */
ACP_EXPORT acp_rc_t acpMemCalloc(void       **aAddr,
                                 acp_size_t   aElements,
                                 acp_size_t   aSize)
{
    if ((aElements == 0) || (aSize == 0))
    {
        return ACP_RC_EINVAL;
    }
    else
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        HANDLE sHeap;

        sHeap = GetProcessHeap();
        if(NULL == sHeap)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            *aAddr = HeapAlloc(sHeap, HEAP_ZERO_MEMORY, aElements * aSize);
            if(NULL == (*aAddr))
            {
                return ACP_RC_GET_OS_ERROR();
            }
            else
            {
                return ACP_RC_SUCCESS;
            }
        }
#else
        *aAddr = calloc(aElements, aSize);

#if defined(ALTI_CFG_OS_AIX)
        /*
         * BUG-34209
         * Because of the bug of AIX itself, calloc failes when memory is
         * available.
         * Thus, if calloc failes, try once more with malloc and set it zero.
         */
        if(*aAddr == NULL)
        {
            *aAddr = malloc(aElements * aSize);
            if(*aAddr == NULL)
            {
                /* Real error */
            }
            else
            {
                acpMemSet(*aAddr, 0, aElements * aSize);
            }
        }
        else
        {
            /* Do nothing */
        }
#endif

        return (*aAddr == NULL) ? ACP_RC_ENOMEM : ACP_RC_SUCCESS;
#endif
    }
}

/**
 * reallocates the memory at @a *aAddr to @a aSize byte(s)
 *
 * @param aAddr pointer to a variable which points to the reallocated memory
 * @param aSize the new size of memory to reallocate
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if @a aSize is zero.
 * if the reallocation fails,
 * it returns #ACP_RC_ENOMEM and the @a *aAddr will not be freed.
 */
ACP_EXPORT acp_rc_t acpMemRealloc(void **aAddr, acp_size_t aSize)
{
    void *sNewAddr = NULL;

    if (aSize == 0)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        HANDLE sHeap;

        sHeap = GetProcessHeap();
        if(NULL == sHeap)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            if(NULL == (*aAddr))
            {
                sNewAddr = HeapAlloc(sHeap, HEAP_ZERO_MEMORY, aSize);
            }
            else
            {
                sNewAddr = HeapReAlloc(sHeap, HEAP_ZERO_MEMORY, *aAddr, aSize);
            }

            if(NULL == sNewAddr)
            {
                return ACP_RC_GET_OS_ERROR();
            }
            else
            {
                *aAddr = sNewAddr;
                return ACP_RC_SUCCESS;
            }
        }
#else
        sNewAddr = realloc(*aAddr, aSize);

        if (sNewAddr == NULL)
        {
            return ACP_RC_ENOMEM;
        }
        else
        {
            *aAddr = sNewAddr;

            return ACP_RC_SUCCESS;
        }
#endif
    }
}

/**
 * frees the memory
 *
 * @param aAddr pointer to the memory
 */
ACP_EXPORT void acpMemFree(void *aAddr)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    (void)HeapFree(GetProcessHeap(), 0, aAddr);
#else
    free(aAddr);
#endif
}

/**
 * frees the aligned memory
 *
 * @param aAddr pointer to the memory
 */
ACP_EXPORT void acpMemFreeAlign(void *aAddr)
{
#if defined(ALTI_CFG_OS_LINUX)   ||              \
    defined(ALTI_CFG_OS_SOLARIS) ||              \
    defined(ALTI_CFG_OS_HPUX)
    acpMemFree(aAddr);
#else
# if defined(ACP_CFG_DEBUG)
    acp_ulong_t sMarker = (acp_ulong_t)((void**)aAddr)[-2];

    if (sMarker != ACP_MEM_BOUNDARY_MARKER)
    {
        abort();
    }
    else
    {
        /* do nothing */
    }
# endif

    /* we stored the original memory starting position in here */
# if defined(ALTI_CFG_OS_WINDOWS)
    (void)HeapFree(GetProcessHeap(), 0, ((void**)aAddr)[-1]);
# else
    free(((void**)aAddr)[-1]);
# endif
#endif
}

ACP_EXTERN_C_END
