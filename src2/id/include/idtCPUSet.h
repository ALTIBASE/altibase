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
 
/*****************************************************************************
 * $Id:
 ****************************************************************************/

/* **********************************************************************
 *   $Id: 
 *   NAME
 *     idtCPUSet.h - CPU Set 정보
 *
 *   DESCRIPTION
 *     TASK-6764로 새로이 추가된 CPU Set 클래스
 *
 *   PUBLIC FUNCTION(S)
 *
 *   PRIVATE FUNCTION(S)
 *
 *   NOTES
 *
 *   MODIFIED   (MM/DD/YY)
 ********************************************************************** */

#ifndef O_IDT_CPUSET_H
#define O_IDT_CPUSET_H   1

#include <acp.h>
#include <idl.h>
#include <ideErrorMgr.h>

#define IDT_MAX_NUMA_NODES      (32)
#define IDT_MAX_CPU_CORES       ACP_PSET_MAX_CPU

#define IDT_FILL                (0)
#define IDT_EMPTY               (-1)
#define IDT_PSET_SIZE           (IDT_MAX_CPU_CORES / 8)
#define IDT_PSET_MAX_INDEX      ((SInt)((size_t)IDT_PSET_SIZE / (size_t)sizeof(ULong)))


struct idtCPUCore
{
    SInt        mCoreID;
    SInt        mSocketID;
    SInt        mLogicalID;
    idBool      mInUse;
    SChar       mModelName[128];
};

class idtCPUSet
{
public:
    idtCPUSet(const SInt = IDT_EMPTY);

    void fill(void);
    void clear(void);
    void initialize(const SInt = IDT_EMPTY);
    void addCPU(const SInt);
    void removeCPU(const SInt);
    void toggleCPU(const SInt);
    void addNUMA(const SInt);
    void removeNUMA(const SInt);

    idBool              find(const SInt);
    SInt                getCPUCount(void);
    SInt                getNUMACount(void);

    SInt                getCPUCountInNUMA(const SInt);
    idBool              isInNUMA(const SInt);
    const idtCPUSet&    copyFrom(const idtCPUSet&);
    const idtCPUSet&    mergeFrom(const idtCPUSet&);
    SInt                findFirstCPU(void);
    SInt                findNextCPU(const SInt);
    SInt                findLastCPU(void);
    SInt                findPrevCPU(const SInt);
    SInt                findFirstNUMA(const SInt);
    SInt                findNextNUMA(const SInt, const SInt);

public:
    IDE_RC              bindThread(void);
    IDE_RC              bindProcess(void);
    static IDE_RC       unbindThread(void);
    static IDE_RC       unbindProcess(void);

    idBool              compare(const idtCPUSet&);
    idBool              implies(const idtCPUSet& aCPUSet);
    const idtCPUSet&    makeIntersectionFrom(const idtCPUSet&,
                                             const idtCPUSet&);
    const idtCPUSet&    makeUnionFrom(const idtCPUSet&,
                                      const idtCPUSet&);

    void                dumpCPUsToString(SChar*, const size_t);
    void                dumpCPUsToHexString(SChar*, const size_t);

    static idBool       canSetMultipleCPUs(void);
    static SInt         getSystemCPUCount(void);
    static SInt         getAvailableCPUCount(void);
    static SInt         getSystemNUMACount(void);

public:
    /* Global init and destroy functions */
    static IDE_RC       initializeStatic(void);
    static IDE_RC       destroyStatic(void);
    static IDE_RC       relocateCPUs(SInt, SInt);

private:
    static IDE_RC       readCPUInfo(void);

private:
    void        convertToPhysicalPset(acp_pset_t*);
    void        calcIndexAndDelta(const SInt, SInt&, SInt&, ULong&);
    void        sortCoreID(idBool*);

public:
    inline idBool operator[](const SInt aCPUNo)
    {
        return find(aCPUNo);
    }

private:
    /* members */
    ULong       mPSet[IDT_PSET_MAX_INDEX];
    SInt        mCPUCount;

    /* Global data */
public:
    static idtCPUCore       mCPUCores[IDT_MAX_CPU_CORES];
    static idtCPUSet        mCorePsets[IDT_MAX_CPU_CORES];
    static idtCPUSet        mNUMAPsets[IDT_MAX_NUMA_NODES];
    static idtCPUSet        mProcessPset;
    static SInt             mSystemCPUCount;
    static SInt             mNUMACount;
    static SInt             mAvailableCPUCount;
};

#endif 

