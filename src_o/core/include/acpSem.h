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
 * $Id: acpSem.h 5178 2009-04-08 06:18:28Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_SEM_H_)
#define _O_ACP_SEM_H_

/**
 * @file
 * @ingroup CoreSem
 */

#include <acpTypes.h>
#include <acpError.h>

ACP_EXTERN_C_BEGIN


/**
 * semaphore object
 */
typedef struct acp_sem_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE                 mHandle;
#else
    acp_sint32_t           mSemID;
#endif
} acp_sem_t;


ACP_EXPORT acp_rc_t acpSemCreate(acp_key_t aKey, acp_sint32_t aValue);
ACP_EXPORT acp_rc_t acpSemDestroy(acp_key_t aKey);

ACP_EXPORT acp_rc_t acpSemOpen(acp_sem_t *aSem, acp_key_t aKey);
ACP_EXPORT acp_rc_t acpSemClose(acp_sem_t *aSem);

ACP_EXPORT acp_rc_t acpSemAcquire(acp_sem_t *aSem);
ACP_EXPORT acp_rc_t acpSemTryAcquire(acp_sem_t *aSem);
ACP_EXPORT acp_rc_t acpSemRelease(acp_sem_t *aSem);


ACP_EXTERN_C_END


#endif
