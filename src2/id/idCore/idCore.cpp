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
 * $Id: $
 **********************************************************************/


#include <idCore.h>
#include <acl.h>
#include <acp.h>



/**
 * @file
 * @ingroup idCore
 */


/*
 * initliaze
 * Every program should call idCore::acpInitialize at start of program,
 * idCore::acpFinalize at end of program.
 */
IDCORE_FUNC_RET_NOARG(SInt, acpInitialize)
IDCORE_FUNC_RET_NOARG(SInt, acpFinalize)


/*
 * Atomic Operations
 */
IDCORE_FUNC_RET_ARG1(SInt, acpAtomicGet32, volatile void *)
IDCORE_FUNC_RET_ARG2(SInt, acpAtomicSet32, volatile void *, volatile SInt)
IDCORE_FUNC_RET_ARG1(SInt, acpAtomicInc32, volatile void *)
IDCORE_FUNC_RET_ARG1(SInt, acpAtomicDec32, volatile void *)
IDCORE_FUNC_RET_ARG2(SInt, acpAtomicAdd32, volatile void *, volatile SInt)
IDCORE_FUNC_RET_ARG2(SInt, acpAtomicSub32, volatile void *, volatile SInt)
IDCORE_FUNC_RET_ARG3(SInt, acpAtomicCas32, volatile void *, volatile SInt, volatile SInt)

IDCORE_FUNC_RET_ARG1(SLong, acpAtomicGet64, volatile void *)
IDCORE_FUNC_RET_ARG2(SLong, acpAtomicSet64, volatile void *, volatile SLong)
IDCORE_FUNC_RET_ARG1(SLong, acpAtomicInc64, volatile void *)
IDCORE_FUNC_RET_ARG1(SLong, acpAtomicDec64, volatile void *)
IDCORE_FUNC_RET_ARG2(SLong, acpAtomicAdd64, volatile void *, volatile SLong)
IDCORE_FUNC_RET_ARG2(SLong, acpAtomicSub64, volatile void *, volatile SLong)
IDCORE_FUNC_RET_ARG3(SLong, acpAtomicCas64, volatile void *, volatile SLong, volatile SLong)

IDCORE_FUNC_RET_ARG1(vSLong, acpAtomicGet, volatile void *)
IDCORE_FUNC_RET_ARG2(vSLong, acpAtomicSet, volatile void *, vSLong)
IDCORE_FUNC_RET_ARG1(vSLong, acpAtomicInc, volatile void *)
IDCORE_FUNC_RET_ARG1(vSLong, acpAtomicDec, volatile void *)
IDCORE_FUNC_RET_ARG2(vSLong, acpAtomicAdd, volatile void *, vSLong)
IDCORE_FUNC_RET_ARG2(vSLong, acpAtomicSub, volatile void *, vSLong)
IDCORE_FUNC_RET_ARG3(vSLong, acpAtomicCas, volatile void *, volatile vSLong, volatile vSLong)

IDCORE_FUNC_RET_ARG3(UChar, acpAtomicCas32Bool,
                     volatile void *, volatile SInt, volatile SInt)
IDCORE_FUNC_RET_ARG3(UChar, acpAtomicCas64Bool,
                     volatile void *, volatile SLong, volatile SLong)
IDCORE_FUNC_RET_ARG3(UChar, acpAtomicCasBool,
                     volatile void *, volatile vSLong, volatile vSLong)


/*
 * Spin-Lock operations
 */

IDCORE_FUNC_NORET_ARG2(acpSpinLockInit, idCoreAcpSpinLock *, SInt)
IDCORE_FUNC_RET_ARG1(UChar, acpSpinLockTryLock, idCoreAcpSpinLock *)
IDCORE_FUNC_RET_ARG1(UChar, acpSpinLockIsLocked, idCoreAcpSpinLock *)
IDCORE_FUNC_NORET_ARG1(acpSpinLockLock, idCoreAcpSpinLock *)
IDCORE_FUNC_NORET_ARG1(acpSpinLockUnlock, idCoreAcpSpinLock *)
IDCORE_FUNC_NORET_ARG2(acpSpinLockSetCount, idCoreAcpSpinLock *, SInt)
IDCORE_FUNC_RET_ARG1(acp_sint32_t, acpSpinLockGetCount, idCoreAcpSpinLock *)


/*
 * Memory-Prefetch operations
 */

IDCORE_FUNC_NORET_ARG1(acpMemPrefetch, void *)
IDCORE_FUNC_NORET_ARG1(acpMemPrefetch0, void *)
IDCORE_FUNC_NORET_ARG1(acpMemPrefetch1, void *)
IDCORE_FUNC_NORET_ARG1(acpMemPrefetch2, void *)
IDCORE_FUNC_NORET_ARG1(acpMemPrefetchN, void *)

/*
 * Thread-Mutex operations
 */

IDCORE_FUNC_RET_ARG2(SInt, acpThrMutexCreate, idCoreAcpThrMutex *, SInt)
IDCORE_FUNC_RET_ARG1(SInt, acpThrMutexDestroy, idCoreAcpThrMutex *)
IDCORE_FUNC_RET_ARG1(SInt, acpThrMutexLock, idCoreAcpThrMutex *)
IDCORE_FUNC_RET_ARG1(SInt, acpThrMutexTryLock, idCoreAcpThrMutex *)
IDCORE_FUNC_RET_ARG1(SInt, acpThrMutexUnlock, idCoreAcpThrMutex *)

/*
 * ACL memory allocator
 */
IDCORE_FUNC_RET_ARG3(SInt, aclMemAllocGetInstance, idCoreAclMemAllocType, void *, idCoreAclMemAlloc **);
IDCORE_FUNC_RET_ARG1(SInt, aclMemAllocFreeInstance, idCoreAclMemAlloc *);
IDCORE_FUNC_NORET_ARG1(aclMemAllocSetDefault, idCoreAclMemAlloc *);
IDCORE_FUNC_RET_ARG3(SInt, aclMemAllocSetAttr, idCoreAclMemAlloc *, idCoreAclMemAllocAttr, void *);
IDCORE_FUNC_RET_ARG3(SInt, aclMemAllocGetAttr, idCoreAclMemAlloc *, idCoreAclMemAllocAttr, void *);
IDCORE_FUNC_RET_ARG2(SInt, aclMemAllocStatistic, idCoreAclMemAlloc *, void *);
IDCORE_FUNC_RET_ARG3(SInt, aclMemAlloc, idCoreAclMemAlloc *, void **, ULong);
IDCORE_FUNC_RET_ARG4(SInt, aclMemCalloc, idCoreAclMemAlloc *, void **, ULong, ULong);
IDCORE_FUNC_RET_ARG3(SInt, aclMemRealloc, idCoreAclMemAlloc *, void **, ULong);
IDCORE_FUNC_RET_ARG2(SInt, aclMemFree, idCoreAclMemAlloc *, void *);

/*
 * Process Operation
 */
IDCORE_FUNC_RET_ARG2(SInt, acpProcIsAliveByPID, UInt, UChar* );

/*
 * ACL queue
 */
IDCORE_FUNC_RET_ARG2(SInt, aclQueueCreate, idCoreAclQueue*, SInt);
IDCORE_FUNC_NORET_ARG1(aclQueueDestroy, idCoreAclQueue*);
IDCORE_FUNC_RET_ARG1(UChar, aclQueueIsEmpty, idCoreAclQueue*);
IDCORE_FUNC_RET_ARG2(SInt, aclQueueEnqueue, idCoreAclQueue*, void*);
IDCORE_FUNC_RET_ARG2(SInt, aclQueueDequeue, idCoreAclQueue*, void**);

/*
 * Directory Functions
 */
IDCORE_FUNC_RET_ARG1(SInt, acpDirRemove, SChar*);

/*
 * Bit Operations
 */
IDCORE_FUNC_RET_ARG1(SInt, acpBitFfs64, ULong);
IDCORE_FUNC_RET_ARG1(SInt, acpBitFfs32, UInt);
IDCORE_FUNC_RET_ARG1(SInt, acpBitFls64, ULong);
IDCORE_FUNC_RET_ARG1(SInt, acpBitFls32, UInt);

/*
 * PSet Operations
 */
IDCORE_FUNC_RET_ARG1(SInt,  acpPsetBindProcess, idCoreAcpPset* );
IDCORE_FUNC_RET_NOARG(SInt, acpPsetUnbindProcess );
