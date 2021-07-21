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
 * $Id: acpShm.h 5857 2009-06-02 10:39:22Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_SHM_H_)
#define _O_ACP_SHM_H_

/**
 * @file
 * @ingroup CoreShm
 */

#include <acpFile.h>


ACP_EXTERN_C_BEGIN


/**
 * shared memory object
 */
typedef struct acp_shm_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE        mHandle;
#else
    acp_sint32_t  mShmID;
#endif
    void         *mAddr;
    acp_size_t    mSize;
} acp_shm_t;


ACP_EXPORT acp_rc_t acpShmCreate(
        acp_key_t  aKey,
        acp_size_t aSize,
        acp_mode_t aPermission
        );
ACP_EXPORT acp_rc_t acpShmDestroy(acp_key_t aKey);

ACP_EXPORT acp_rc_t acpShmAttach(acp_shm_t *aShm, acp_key_t aKey);
ACP_EXPORT acp_rc_t acpShmDetach(acp_shm_t *aShm);

/**
 * gets shared memory address
 * @param aShm pointer to the shared memory object
 * @return memory address
 */
ACP_INLINE void *acpShmGetAddress(acp_shm_t *aShm)
{
    return aShm->mAddr;
}

/**
 * gets size of shared memory
 * @param aShm pointer to the shared memory object
 * @return size of shared memory
 */
ACP_INLINE acp_size_t acpShmGetSize(acp_shm_t *aShm)
{
    return aShm->mSize;
}


ACP_EXTERN_C_END


#endif
