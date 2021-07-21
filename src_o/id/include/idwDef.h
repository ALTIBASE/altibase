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
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDW_DEF_H_)
#define _O_IDW_DEF_H_ 1

#include <idTypes.h>
#include <iduShmDef.h>

#define IDW_LPID_NULL           (0xFFFFFFFF)
#define IDW_SEM_KEY_NULL        (0)
#define IDW_SEM_MAX_TRY_COUNT   (1000000)
#define IDW_MAX_SLEEP_SEC       (3)

#define IDW_THRID_MAKE( aLPID, aThrNum ) ( (aLPID) << 16 | (aThrNum) )
#define IDW_THRID_LPID( aThrID )         ( (idLPID)((aThrID) >> 16) )
#define IDW_THRID_THRNUM( aThrID )       ( (aThrID) & 0x0000FFFF)

typedef enum idwProcShutdownMode
{
	IDW_PROC_SHUTDOWN_NORMAL    = 11,
	IDW_PROC_SHUTDOWN_IMMEDIATE = 12,
	IDW_PROC_SHUTDOWN_EXIT      = 13
}idwProcShutdownMode;

typedef struct idwShmProcMgr
{
    iduShmProcInfo    * mSelfProcInfo;
    acp_sem_t           mSemHandle;
    iduShmProcMgrInfo * mProcMgrInfo;
    idwProcShutdownMode mShutdownMode;
} idwShmProcMgr;

#endif /* _O_IDW_DEF_H_ */
