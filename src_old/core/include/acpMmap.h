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
 * $Id: acpMmap.h 10305 2010-03-08 01:48:38Z denisb $
 ******************************************************************************/

#if !defined(_O_ACP_MMAP_H_)
#define _O_ACP_MMAP_H_

/**
 * @file
 * @ingroup CoreFile
 * @ingroup CoreMem
 */

#include <acpFile.h>


ACP_EXTERN_C_BEGIN


/*
 * protection flag
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * no permission specified to the mapped memory
 * @see acpMmapAttach()
 */
#define ACP_MMAP_NONE
/**
 * can read from the mapped memory
 * @see acpMmapAttach()
 */
#define ACP_MMAP_READ
/**
 * can write to the mapped memory
 * @see acpMmapAttach()
 */
#define ACP_MMAP_WRITE
/**
 * can execute codes in the mapped memory
 * @see acpMmapAttach()
 */
#define ACP_MMAP_EXEC

#else

#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_MMAP_NONE    0x000
#define ACP_MMAP_READ    0x001
#define ACP_MMAP_WRITE   0x002
#define ACP_MMAP_EXEC    0x004
#else
#define ACP_MMAP_NONE    PROT_NONE
#define ACP_MMAP_READ    PROT_READ
#define ACP_MMAP_WRITE   PROT_WRITE
#define ACP_MMAP_EXEC    PROT_EXEC
#endif

#endif

/*
 * attach flag
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * the mapped memory will be shared between the processes
 * @see acpMmapAttach()
 */
#define ACP_MMAP_SHARED
/**
 * the mapped memory will be used only in the current process
 * @see acpMmapAttach()
 */
#define ACP_MMAP_PRIVATE

#else

#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_MMAP_SHARED  0x010
#define ACP_MMAP_PRIVATE 0x020
#else
#define ACP_MMAP_SHARED  MAP_SHARED
#define ACP_MMAP_PRIVATE MAP_PRIVATE
#endif

#endif

/*
 * sync flag
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * synchronus sync to the file
 * @see acpMmapSync()
 */
#define ACP_MMAP_SYNC
/**
 * asynchronus sync to the file
 * @see acpMmapSync()
 */
#define ACP_MMAP_ASYNC

#else

#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_MMAP_SYNC    0x100
#define ACP_MMAP_ASYNC   0x200
#else
#define ACP_MMAP_SYNC    MS_SYNC
#define ACP_MMAP_ASYNC   MS_ASYNC
#endif

#endif

/*
 * memory map descriptor
 */
/**
 * memory map object
 */
typedef struct acp_mmap_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE      mHandle;
#endif
    void       *mAddr;
    acp_size_t  mSize;
} acp_mmap_t;


ACP_EXPORT acp_rc_t acpMmap(acp_mmap_t   *aMmap,
                            acp_size_t    aSize,
                            acp_sint32_t  aProtection,
                            acp_sint32_t  aFlag);

ACP_EXPORT acp_rc_t acpMmapAttach(acp_mmap_t   *aMmap,
                                  acp_file_t   *aFile,
                                  acp_offset_t  aOffset,
                                  acp_size_t    aSize,
                                  acp_sint32_t  aProtection,
                                  acp_sint32_t  aFlag);
ACP_EXPORT acp_rc_t acpMmapDetach(acp_mmap_t *aMmap);

ACP_EXPORT acp_rc_t acpMmapSync(acp_mmap_t   *aMmap,
                                acp_size_t    aOffset,
                                acp_size_t    aSize,
                                acp_sint32_t  aFlag);

/**
 * gets the address of the mapped memory
 * @param aMmap pointer to the memory map object
 * @returns address of mapped memory
 */
ACP_INLINE void *acpMmapGetAddress(acp_mmap_t *aMmap)
{
    return aMmap->mAddr;
}

/**
 * gets the size of mapped memory
 * @param aMmap pointer to the memory map object
 * @returns size of mapped memory
 */
ACP_INLINE acp_size_t acpMmapGetSize(acp_mmap_t *aMmap)
{
    return aMmap->mSize;
}


ACP_EXTERN_C_END


#endif
