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

/***********************************************************************
 *
 * NAME
 *   idCore.h
 *
 * DESCRIPTION
 *
 * PUBLIC FUNCTION(S)
 *
 * PRIVATE FUNCTION(S)
 *
 * NOTES
 *
 * MODIFIED    (MM/DD/YYYY)
 *
 **********************************************************************/

#ifndef _O_IDCORE_H_
#define _O_IDCORE_H_ 1


/**
 * @file
 * @ingroup idCore
 * Detail reference for using Alticore and idCore are
 * - http://nok.altibase.com/pages/viewpage.action?pageId=8193083 and 
 * - http://nok.altibase.com/display/rnd/Alticore+Handbook+for+Altibase+developer
 *
 * Primitive data type translation from Alticore type into id type
 * - acp_sint16_t -> SShort
 * - acp_sint32_t -> SInt
 * - acp_sint64_t -> SLong
 * - acp_slong_t -> vSLong
 * - acp_bool_t -> UChar
 * - acp_rc_t -> SInt
 * - acp_size_t -> ULong
 * - acp_char_t -> SChar
 */


#include <acp.h>
#include <acpPset.h>
#include <acl.h>
#include <idTypes.h>

#include <stddef.h>


/**
 * define function with no return value and no argument
 * @param name function name to be defined
 */
#define IDCORE_FUNC_NORET_NOARG(name)           \
    void idCore::name(void)                     \
    {                                           \
        (void)::name();                         \
    }

/**
 * define function with return value and no argument
 * @param type data type of return value
 * @param name function name to be defined
 */
#define IDCORE_FUNC_RET_NOARG(type, name)       \
    type idCore::name(void)                     \
    {                                           \
        return ::name();                        \
    }

/**
 * define function with no return value and one argument
 * @param name function name to be defined
 * @param type1 data type of the first argument 
 */
#define IDCORE_FUNC_NORET_ARG1(name, type1)     \
    void idCore::name(type1 arg1)               \
    {                                           \
        (void)::name(arg1);                     \
    }

/**
 * define function with return value and one argument
 * @param type data type of return value
 * @param name function name to be defined
 * @param type1 data type of the first argument 
 */
#define IDCORE_FUNC_RET_ARG1(type, name, type1) \
    type idCore::name(type1 arg1)               \
    {                                           \
        return ::name(arg1);                    \
    }

/**
 * define function with no return value and two argument
 * @param name function name to be defined
 * @param type1 data type of the first argument
 * @param type2 data type of the second argument
 */
#define IDCORE_FUNC_NORET_ARG2(name, type1, type2)  \
    void idCore::name(type1 arg1, type2 arg2)       \
    {                                               \
        (void)::name(arg1,arg2);                    \
    }

/**
 * define function with return value and two argument
 * @param type data type of return value
 * @param name function name to be defined
 * @param type1 data type of the first argument 
 * @param type2 data type of the second argument
 */
#define IDCORE_FUNC_RET_ARG2(type, name, type1, type2)  \
    type idCore::name(type1 arg1, type2 arg2)           \
    {                                                   \
        return ::name(arg1,arg2);                       \
    }

/**
 * define function with no return value and three argument
 * @param name function name to be defined
 * @param type1 data type of the first argument
 * @param type2 data type of the second argument
 * @param type3 data type of the third argument
 */
#define IDCORE_FUNC_NORET_ARG3(name, type1, type2, type3)   \
    void idCore::name(type1 arg1, type2 arg2, type3 arg3)   \
    {                                                       \
        (void)::name(arg1,arg2,arg3);                       \
    }

/**
 * define function with return value and three argument
 * @param type data type of return value
 * @param name function name to be defined
 * @param type1 data type of the first argument
 * @param type2 data type of the second argument
 * @param type3 data type of the third argument
 */
#define IDCORE_FUNC_RET_ARG3(type, name, type1, type2, type3)   \
    type idCore::name(type1 arg1, type2 arg2, type3 arg3)       \
    {                                                           \
        return ::name(arg1,arg2,arg3);                          \
    }

/**
 * define function with return value and four argument
 * @param type data type of return value
 * @param name function name to be defined
 * @param type1 data type of the first argument
 * @param type2 data type of the second argument
 * @param type3 data type of the third argument
 * @param type3 data type of the fourth argument
 */
#define IDCORE_FUNC_RET_ARG4(type, name, type1, type2, type3, type4)    \
    type idCore::name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)   \
    {                                                                   \
        return ::name(arg1,arg2,arg3,arg4);                             \
    }


/**
 * redefine acp_spin_lock_t
 */
#define idCoreAcpSpinLock acp_spin_lock_t
/**
 * redefine acp_thr_mutex_t
 */
#define idCoreAcpThrMutex acp_thr_mutex_t
/**
 * redefine acl_mem_alloc_t
 */
#define idCoreAclMemAlloc acl_mem_alloc_t
/**
 * redefine acl_mem_alloc_type_t
 */
#define idCoreAclMemAllocType acl_mem_alloc_type_t
/**
 * redefine acl_mem_alloc_attr_t
 */
#define idCoreAclMemAllocAttr acl_mem_alloc_attr_t
/**
 * redefine acl_mem_tlsf_init_t
 */
#define idCoreAclMemTlsfInit acl_mem_tlsf_init_t
/**
 * redefine acl_mem_tlsf_stat_t
 */
#define idCoreAclMemTlsfStat acl_mem_tlsf_stat_t
/**
 * redefine acp_thr_t
 */
#define idCoreAcpThr acp_thr_t
/**
 * redefine acl_queue_t
 */
#define idCoreAclQueue acl_queue_t

/**
 * redefine acp_pset_t
 */
#define idCoreAcpPset acp_pset_t

/**
 * @class idCore
 * @brief The interfaces of the Alticore should be redefined as the methods of idCore class.
 * Refer Alticore manual for the detail description.
 */
class idCore
{
public:

    /*
     * constructor & destructor
     */

    /** initialize alticore
     * - Every program must call acpInitialize before any calling of idCore methods.
     */
    static SInt acpInitialize(void);
    /** finalize alticore */
    static SInt acpFinalize(void);

    /*
     * Atomic Operations
     */

    /* 32-bit */
    static SInt acpAtomicGet32(volatile void *aAddr);
    static SInt acpAtomicSet32(volatile void *aAddr,
                               volatile SInt  aVal);
    static SInt acpAtomicInc32(volatile void *aAddr);
    static SInt acpAtomicDec32(volatile void *aAddr);
    static SInt acpAtomicAdd32(volatile void         *aAddr,
                               volatile SInt  aVal);
    static SInt acpAtomicSub32(volatile void         *aAddr,
                               volatile SInt  aVal);
    static SInt acpAtomicCas32(volatile void         *aAddr,
                               volatile SInt  aWith,
                               volatile SInt  aCmp);

    /* 64-bit */
    static SLong acpAtomicGet64(volatile void *aAddr);
    static SLong acpAtomicSet64(volatile void  *aAddr,
                                volatile SLong  aVal);
    static SLong acpAtomicInc64(volatile void *aAddr);
    static SLong acpAtomicDec64(volatile void *aAddr);
    static SLong acpAtomicAdd64(volatile void         *aAddr,
                                volatile SLong  aVal);
    static SLong acpAtomicSub64(volatile void         *aAddr,
                                volatile SLong  aVal);
    static SLong acpAtomicCas64(volatile void *aAddr,
                                volatile SLong   aWith,
                                volatile SLong   aCmp);

    /* general */
    static vSLong acpAtomicGet(volatile void *aAddr);
    static vSLong acpAtomicSet(volatile void *aAddr, vSLong aVal);
    static vSLong acpAtomicInc(volatile void *aAddr);
    static vSLong acpAtomicDec(volatile void *aAddr);
    static vSLong acpAtomicAdd(volatile void *aAddr, vSLong aVal);
    static vSLong acpAtomicSub(volatile void *aAddr, vSLong aVal);
    static vSLong acpAtomicCas(volatile void        *aAddr,
                               volatile vSLong  aWith,
                               volatile vSLong  aCmp);

    /* bool */
    static UChar acpAtomicCas32Bool(volatile void *aAddr,
                                     volatile SInt   aWith,
                                     volatile SInt   aCmp);
    static UChar acpAtomicCas64Bool(volatile void *aAddr,
                                     volatile SLong   aWith,
                                     volatile SLong   aCmp);
    static UChar acpAtomicCasBool(volatile void        *aAddr,
                                   volatile vSLong  aWith,
                                   volatile vSLong  aCmp);

    /*
     * Spin-Lock operations
     */
    static void acpSpinLockInit(idCoreAcpSpinLock *aLock, SInt aSpinCount);
    static UChar acpSpinLockTryLock(idCoreAcpSpinLock *aLock);
    static UChar acpSpinLockIsLocked(idCoreAcpSpinLock *aLock);
    static void acpSpinLockLock(idCoreAcpSpinLock *aLock);
    static void acpSpinLockUnlock(idCoreAcpSpinLock *aLock);
    static void acpSpinLockSetCount(idCoreAcpSpinLock *aLock,
                                    SInt     aSpinCount);
    static SInt acpSpinLockGetCount(idCoreAcpSpinLock *aLock);
    
    
    /*
     * Memory-Prefetch operations
     */
    static void acpMemPrefetch(void* aPointer);
    static void acpMemPrefetch0(void* aPointer);
    static void acpMemPrefetch1(void* aPointer);
    static void acpMemPrefetch2(void* aPointer);
    static void acpMemPrefetchN(void* aPointer);

    
    /*
     * Thread-Mutex operations
     */
    static SInt acpThrMutexCreate(idCoreAcpThrMutex *aMutex,
                                  SInt     aFlag);
    static SInt acpThrMutexDestroy(idCoreAcpThrMutex *aMutex);
    static SInt acpThrMutexLock(idCoreAcpThrMutex *aMutex);
    static SInt acpThrMutexTryLock(idCoreAcpThrMutex *aMutex);
    static SInt acpThrMutexUnlock(idCoreAcpThrMutex *aMutex);


    /*
     * ACL memory manager
     */
    static SInt aclMemAllocGetInstance(idCoreAclMemAllocType  aAllocatorType,
                                       void                 *aArgs,
                                       idCoreAclMemAlloc      **aAllocator);
    static SInt aclMemAllocFreeInstance (idCoreAclMemAlloc *aAllocator);
    static void  aclMemAllocSetDefault(idCoreAclMemAlloc *aAllocator);
    static SInt  aclMemAllocSetAttr(idCoreAclMemAlloc *aAllocator,
                                    idCoreAclMemAllocAttr aAttrName,
                                    void *aAttrValue);
    static SInt  aclMemAllocGetAttr(idCoreAclMemAlloc      *aAllocator,
                                    idCoreAclMemAllocAttr  aAttrName,
                                    void                 *aAttrValue);
    static SInt aclMemAllocStatistic(idCoreAclMemAlloc *aAllocator, 
                                     void *aStat);
    static SInt aclMemAlloc(idCoreAclMemAlloc *aAllocator,
                            void **aAddr,
                            ULong aSize);
    static SInt aclMemCalloc(idCoreAclMemAlloc *aAllocator,
                             void  **aAddr,
                             ULong   aElements,
                             ULong   aSize);
    static SInt aclMemRealloc(idCoreAclMemAlloc *aAllocator,
                              void **aAddr,
                              ULong aSize);
    static SInt aclMemFree(idCoreAclMemAlloc *aAllocator, void *aAddr);
    static SInt acpProcIsAliveByPID(UInt aPID, UChar *aIsAlive);

    /*
     * ACL queue
     */
    static SInt aclQueueCreate(idCoreAclQueue  *aQueue,
                                  SInt  aParallelFactor);
    static void aclQueueDestroy(idCoreAclQueue *aQueue);
    static UChar aclQueueIsEmpty(idCoreAclQueue *aQueue);
    static SInt aclQueueEnqueue(idCoreAclQueue *aQueue, void *aObj);
    static SInt aclQueueDequeue(idCoreAclQueue *aQueue, void **aObj);

    /*
     * Directory Functions
     */
    static SInt acpDirRemove(SChar* aPath);

    /*
     * Bit Operations
     */
    static SInt acpBitFfs32(UInt aValue);
    static SInt acpBitFfs64(ULong aValue);
    static SInt acpBitFls32(UInt aValue);
    static SInt acpBitFls64(ULong aValue);

    /*
     * PSet Operations
     */
    static SInt acpPsetBindProcess(idCoreAcpPset *aPset);
    static SInt acpPsetUnbindProcess(void);

    /*
     * To be continue...
     */
};

#endif // _O_IDCORE_H_
