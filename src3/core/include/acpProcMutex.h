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
 * $Id: acpProcMutex.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_PROC_MUTEX_H_)
#define _O_ACP_PROC_MUTEX_H_

/**
 * @file
 * @ingroup CoreProc
 */

#include <acpTypes.h>
#include <acpSem.h>

ACP_EXTERN_C_BEGIN


/**
 * process mutex object
 */
typedef struct acp_proc_mutex_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE       mHandle;
#else
    acp_sem_t    mSem;
#endif
} acp_proc_mutex_t;


ACP_EXPORT acp_rc_t acpProcMutexCreate(acp_key_t aKey);
ACP_EXPORT acp_rc_t acpProcMutexDestroy(acp_key_t aKey);

ACP_EXPORT acp_rc_t acpProcMutexOpen(acp_proc_mutex_t *aMutex, acp_key_t aKey);
ACP_EXPORT acp_rc_t acpProcMutexClose(acp_proc_mutex_t *aMutex);

ACP_EXPORT acp_rc_t acpProcMutexLock(acp_proc_mutex_t *aMutex);
ACP_EXPORT acp_rc_t acpProcMutexTryLock(acp_proc_mutex_t *aMutex);
ACP_EXPORT acp_rc_t acpProcMutexUnlock(acp_proc_mutex_t *aMutex);


ACP_EXTERN_C_END


#endif
