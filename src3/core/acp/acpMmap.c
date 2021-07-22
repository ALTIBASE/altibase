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
 * $Id: acpMmap.c 10600 2010-03-26 04:57:43Z denisb $
 ******************************************************************************/

#include <acpMmap.h>
#include <acpSys.h>


#if defined(ALTI_CFG_OS_WINDOWS)

ACP_EXPORT acp_rc_t acpMmap(acp_mmap_t   *aMmap,
                            acp_size_t    aSize,
                            acp_sint32_t  aProtection,
                            acp_sint32_t  aFlag)
{
    DWORD sFlProtect     = 0;
    DWORD sDesiredAccess = 0;
 
    if (aSize == 0)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    if (aProtection & ACP_MMAP_WRITE)
    {
        sFlProtect     |= PAGE_READWRITE;
        sDesiredAccess |= FILE_MAP_WRITE;
    }
    else
    {
        if (aProtection & ACP_MMAP_READ)
        {
            sFlProtect     |= PAGE_READONLY;
            sDesiredAccess |= FILE_MAP_READ;
        }
        else
        {
            sFlProtect     |= PAGE_WRITECOPY;
            sDesiredAccess |= FILE_MAP_COPY;
        }
    }

    aMmap->mHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
                                       NULL,
                                       sFlProtect,
#if defined(ACP_CFG_COMPILE_64BIT)
                                       (DWORD)(aSize >> 32),
#else
                                       0,
#endif
                                       (DWORD)(aSize),
                                       NULL);

    if ((aMmap->mHandle == NULL) ||
        (aMmap->mHandle == INVALID_HANDLE_VALUE))
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    aMmap->mAddr = MapViewOfFile(aMmap->mHandle,
                                 sDesiredAccess,
                                 0,
                                 0,
                                 aSize);

    if (aMmap->mAddr == NULL)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aMmap->mSize = aSize;

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpMmapAttach(acp_mmap_t   *aMmap,
                                  acp_file_t   *aFile,
                                  acp_offset_t  aOffset,
                                  acp_size_t    aSize,
                                  acp_sint32_t  aProtection,
                                  acp_sint32_t  aFlag)
{
    DWORD sLowOffset;
    DWORD sHighOffset;
    DWORD sFlProtect     = 0;
    DWORD sDesiredAccess = 0;

    if (aSize == 0)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    if ((aFile          == NULL) ||
        (aFile->mHandle == INVALID_HANDLE_VALUE))
    {
        return ACP_RC_EBADF;
    }
    else
    {
        /* do nothing */
    }

    if (aProtection & ACP_MMAP_WRITE)
    {
        sFlProtect     |= PAGE_READWRITE;
        sDesiredAccess |= FILE_MAP_WRITE;
    }
    else
    {
        if (aProtection & ACP_MMAP_READ)
        {
            sFlProtect     |= PAGE_READONLY;
            sDesiredAccess |= FILE_MAP_READ;
        }
        else
        {
            sFlProtect     |= PAGE_WRITECOPY;
            sDesiredAccess |= FILE_MAP_COPY;
        }
    }

    aMmap->mHandle = CreateFileMapping(aFile->mHandle,
                                       NULL,
                                       sFlProtect,
                                       0,
                                       0,
                                       NULL);

    if ((aMmap->mHandle == NULL) ||
        (aMmap->mHandle == INVALID_HANDLE_VALUE))
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sLowOffset  = (DWORD)aOffset;
    sHighOffset = (DWORD)(aOffset >> 32);

    aMmap->mAddr = MapViewOfFile(aMmap->mHandle,
                                 sDesiredAccess,
                                 sHighOffset,
                                 sLowOffset,
                                 aSize);

    if (aMmap->mAddr == NULL)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aMmap->mSize = aSize;

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpMmapDetach(acp_mmap_t *aMmap)
{
    BOOL sRet;

    sRet = UnmapViewOfFile(aMmap->mAddr);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRet = CloseHandle(aMmap->mHandle);

        if (sRet == 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            /* do nothing */
        }

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpMmapSync(acp_mmap_t   *aMmap,
                                acp_size_t    aOffset,
                                acp_size_t    aSize,
                                acp_sint32_t  aFlag)
{
    BOOL sRet;

    ACP_UNUSED(aFlag);

    if ((aOffset == 0) && (aSize == 0))
    {
        aSize = aMmap->mSize;
    }
    else
    {
        /* do nothing */
    }

    if ((aOffset + aSize) > aMmap->mSize)
    {
        return ACP_RC_ENOMEM;
    }
    else
    {
        /* do nothing */
    }

    sRet = FlushViewOfFile((acp_char_t *)aMmap->mAddr + aOffset, aSize);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

#else

/**
 * creates a new anonymous mapping in the virtual address space
 * of the calling process.
 *
 * Mapping is not backed by any file.
 *
 * @param aMmap The newly created mmap'ed memory.
 * @param aSize The size of the mmapped memory area.
 * @param aProtection the desired memory protection.
 * @param aFlag bit-wise or of (ACP_MMAP_READ | APR_MMAP_WRITE).
 * @return result code.
 */
ACP_EXPORT acp_rc_t acpMmap(acp_mmap_t   *aMmap,
                            acp_size_t    aSize,
                            acp_sint32_t  aProtection,
                            acp_sint32_t  aFlag)
{
    void *sRet = mmap(NULL,
                      aSize,
                      aProtection,
#if defined(ALTI_CFG_OS_FREEBSD) || defined(ALTI_CFG_OS_DARWIN)
                      aFlag | MAP_ANON, 
#else
                      aFlag | MAP_ANONYMOUS,
#endif
                      -1,
                      0);

    if (sRet == MAP_FAILED)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aMmap->mAddr = sRet;
        aMmap->mSize = aSize;

        return ACP_RC_SUCCESS;
    }
}

/**
 * maps file to the memory
 *
 * @param aMmap The newly created mmap'ed file.
 * @param aFile The file turn into an mmap.
 * @param aOffset The offset into the file to start the data pointer at.
 * @param aSize The size of the file.
 * @param aProtection the desired memory protection.
 * @param aFlag bit-wise or of (ACP_MMAP_READ | APR_MMAP_WRITE).
 * @return result code.
 */
ACP_EXPORT acp_rc_t acpMmapAttach(acp_mmap_t   *aMmap,
                                  acp_file_t   *aFile,
                                  acp_offset_t  aOffset,
                                  acp_size_t    aSize,
                                  acp_sint32_t  aProtection,
                                  acp_sint32_t  aFlag)
{
    void *sRet = NULL;

    if (aFile == NULL)
    {
        return ACP_RC_EBADF;
    }
    else
    {
        /* do nothing */
    }

    sRet = mmap(NULL,
                aSize,
                aProtection,
                aFlag,
                aFile->mHandle,
                aOffset);

    if (sRet == MAP_FAILED)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        aMmap->mAddr = sRet;
        aMmap->mSize = aSize;

        return ACP_RC_SUCCESS;
    }
}

/**
 * unmaps memory
 *
 * @param aMmap mmap'ed file.
 * @return result code
 */
ACP_EXPORT acp_rc_t acpMmapDetach(acp_mmap_t *aMmap)
{
    acp_sint32_t sRet;

    sRet = munmap(aMmap->mAddr, aMmap->mSize);

    return (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

/**
 * sync mapped memory to the file
 *
 * @param aMmap mmap'ed file.
 * @param aOffset The offset into the file to start the data pointer at.
 * @param aSize The size of the file.
 * @param aFlag bit-wise or of (ACP_MMAP_SYNC | ACP_MMAP_ASYNC).
 * @return result code
 *
 * it syncs all mapped pages if @a aOffset and @a aSize are zero.
 *
 * it returns #ACP_RC_ENOMEM if out of the range.
 */
ACP_EXPORT acp_rc_t acpMmapSync(acp_mmap_t   *aMmap,
                                acp_size_t    aOffset,
                                acp_size_t    aSize,
                                acp_sint32_t  aFlag)
{
    acp_sint32_t sRet;
    acp_size_t   sPageSize = 0;

    if ((aOffset == 0) && (aSize == 0))
    {
        sRet = msync(aMmap->mAddr, aMmap->mSize, aFlag);
    }
    else
    {
        if ((aOffset + aSize) > aMmap->mSize)
        {
            return ACP_RC_ENOMEM;
        }
        else
        {
            /* do nothing */
        }

        sRet = acpSysGetPageSize(&sPageSize);

        if (sRet != ACP_RC_SUCCESS)
        {
            return sRet;
        }
        else
        {
            aSize   += aOffset % sPageSize;
            aOffset -= aOffset % sPageSize;

            sRet = msync((acp_char_t *)aMmap->mAddr + aOffset, aSize, aFlag);
        }
    }

    return (sRet != 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

#endif
