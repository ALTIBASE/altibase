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

/*****************************************************************************
 *   NAME
 *     idtCPUSet.cpp - CPU Set 클래스 구현
 *
 *   DESCRIPTION
 *     TASK-6764로 새로이 추가된 CPU Set 정보
 *
 *   PUBLIC FUNCTION(S)
 *
 *   PRIVATE FUNCTION(S)
 *
 *   NOTES
 *
 *   MODIFIED   (MM/DD/YY)
 ****************************************************************************/

#include <idtCPUSet.h>
#include <idu.h>

idtCPUCore  idtCPUSet::mCPUCores[IDT_MAX_CPU_CORES];
idtCPUSet   idtCPUSet::mCorePsets[IDT_MAX_CPU_CORES];
idtCPUSet   idtCPUSet::mNUMAPsets[IDT_MAX_NUMA_NODES];
idtCPUSet   idtCPUSet::mProcessPset;
SInt        idtCPUSet::mSystemCPUCount;
SInt        idtCPUSet::mNUMACount;
SInt        idtCPUSet::mAvailableCPUCount;

/*
 * Global init and destroy
 */

IDE_RC idtCPUSet::readCPUInfo(void)
{
    SInt         sRC;
    UInt         sCpuCount;
    UInt         sActualCpuCount=0;
    UInt         i;
    acp_pset_t   sPset;

    sRC = acpSysGetCPUCount(&sCpuCount);

    IDE_TEST(sRC != IDE_SUCCESS);

    idlOS::memset(mCPUCores, 0, sizeof(idtCPUCore) * IDT_MAX_CPU_CORES);
    for(i = 0; i < IDT_MAX_CPU_CORES; i++)
    {
        mCorePsets[i].clear();
    }
    for(i = 0; i < IDT_MAX_NUMA_NODES; i++)
    {
        mNUMAPsets[i].clear();
    }

    mAvailableCPUCount = IDT_MAX_CPU_CORES;

    /* ------------------------------------------------
     *  Get Actual CPU count by loop checking
     *  We are binding each cpu to this process in order to know
     *  whether this number of cpu exist actually or not.
     * ----------------------------------------------*/
        
    ACP_PSET_ZERO(&sPset);
    for (i = 0; sActualCpuCount < sCpuCount; i++)
    {
        ACP_PSET_SET(&sPset,i);
            
        if ( acpPsetBindThread(&sPset) == IDE_SUCCESS )
        {
            /* success of bind : that means, exist this cpu */

            mCPUCores[sActualCpuCount].mCoreID       = i;
            mCPUCores[sActualCpuCount].mLogicalID    = sActualCpuCount;
            mCPUCores[sActualCpuCount].mSocketID     = 0;
            mCPUCores[sActualCpuCount].mInUse        = ID_TRUE;

            mCorePsets[sActualCpuCount].addCPU(sActualCpuCount);
            mNUMAPsets[0].addCPU(sActualCpuCount);

            mProcessPset.addCPU(sActualCpuCount);
            sActualCpuCount++;
        }
        else
        {
            /* error on binding : this means there is no cpu number %i */
        }
        ACP_PSET_CLR(&sPset, i);
    }

    IDE_TEST(sActualCpuCount != sCpuCount);
    acpPsetUnbindThread();
    mNUMACount = 1;
    mSystemCPUCount = sCpuCount;
    mAvailableCPUCount = sActualCpuCount;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idtCPUSet::initializeStatic(void)
{
#if defined(ALTI_CFG_OS_LINUX)

# if (defined(ALTI_CFG_CPU_X86) || defined(ALTI_CFG_CPU_IA64))
#  define CORE_ID         "processor"
#  define MODEL_NAME      "model name"
# elif defined(ALTI_CFG_CPU_POWERPC)
#  define CORE_ID         "processor"
#  define MODEL_NAME      "cpu"
# endif

    PDL_HANDLE       sCPUInfo;

    DIR*             sNodes;
    DIR*             sCPUs;
    struct dirent*   sNodeEnt;
    struct dirent*   sCPUEnt;
    SInt             sSocketID;
    SInt             sCoreID;
    SInt             sCoreLen;
    SInt             sModelLen;
    SInt             sIndex;
    SInt             i;

    SChar*           sNumber;
    SChar            sPath[ID_MAX_FILE_NAME];
    SChar            sCPUInfoPath[ID_MAX_FILE_NAME];
    SInt             sSocketOfCPU[IDT_MAX_CPU_CORES];
    SInt             sIndexOfCPU[IDT_MAX_CPU_CORES];

    SChar            sModelName[128];

    SChar            sLine[16384];

    const SChar*     sNodePath = "/sys/devices/system/node";

    idlOS::memset(mCPUCores, 0, sizeof(idtCPUCore) * IDT_MAX_CPU_CORES);
    idlOS::memset(sSocketOfCPU, 0, IDT_MAX_CPU_CORES * sizeof(SInt));
    idlOS::memset(sIndexOfCPU,  0, IDT_MAX_CPU_CORES * sizeof(SInt));

    for(i = 0; i < IDT_MAX_CPU_CORES; i++)
    {
        mCorePsets[i].clear();
    }
    for(i = 0; i < IDT_MAX_NUMA_NODES; i++)
    {
        mNUMAPsets[i].clear();
    }

    mAvailableCPUCount = IDT_MAX_CPU_CORES;

    sNodes = idlOS::opendir(sNodePath);
    IDE_TEST_CONT(sNodes == NULL, EOLDKERNEL);
    sIndex = 0;

    while((sNodeEnt = idlOS::readdir(sNodes)) != NULL)
    {
        if(idlOS::strncmp(sNodeEnt->d_name, "node", 4) == 0)
        {
            idlOS::snprintf(sPath, ID_MAX_FILE_NAME, "%s/%s", sNodePath, sNodeEnt->d_name);
            sSocketID = (UInt)idlOS::strtol(sNodeEnt->d_name + 4, NULL, 0);

            IDE_TEST_RAISE(sSocketID >= IDT_MAX_NUMA_NODES, ENODEEXCEED);

            sCPUs = idlOS::opendir(sPath);
            IDE_TEST_RAISE(sCPUs == NULL, ENOSYSINFO);
            while((sCPUEnt = idlOS::readdir(sCPUs)) != NULL)
            {
                if((idlOS::strncmp(sCPUEnt->d_name, "cpu", 3) == 0) &&
                        (isdigit((UChar)sCPUEnt->d_name[3]) != 0))
                {
                     /* check online files(BUG-47105) */
                     idlOS::snprintf(sCPUInfoPath ,PATH_MAX ,"%s/%s/online" ,sPath ,sCPUEnt->d_name );
                     /* if there is no the file online,it means it's always available. */
                     if(idlOS::access(sCPUInfoPath, F_OK) == 0) /* if it exists. */
                     {

                        sCPUInfo = idf::open(sCPUInfoPath, O_RDONLY);
                        IDE_TEST_RAISE(sCPUInfo == PDL_INVALID_HANDLE, ENOSYSINFO);

                        if( (idf::fdgets(sLine, ID_SIZEOF(sLine), sCPUInfo) == NULL) || ( sLine[0] == '0' )  )
                        {
                            idf::close(sCPUInfo);
                            continue;
                        }
                        idf::close(sCPUInfo);

                    }
                    sCoreID    = (UInt)idlOS::strtol(sCPUEnt->d_name + 3, NULL, 0);
                    IDE_TEST_RAISE(sIndex >= IDT_MAX_CPU_CORES, ECPUEXCEED);

                    mCPUCores[sIndex].mCoreID       = sCoreID;
                    mCPUCores[sIndex].mSocketID     = sSocketID;
                    mCPUCores[sIndex].mLogicalID    = sIndex;
                    mCPUCores[sIndex].mInUse        = ID_FALSE;

                    mCorePsets[sIndex].addCPU(sIndex);
                    mNUMAPsets[sSocketID].addCPU(sIndex);

                    mNUMACount = IDL_MAX(mNUMACount, sSocketID);
                    sIndex++;
                }
                else
                {
                    /* continue */
                }
            }

            idlOS::closedir(sCPUs);
        }
        else
        {
            /* continue; */
        }
    }

    mNUMACount++;
    mSystemCPUCount = sIndex;

    sCPUInfo = idf::open("/proc/cpuinfo", O_RDONLY);
    IDE_TEST_RAISE(sCPUInfo == PDL_INVALID_HANDLE, ENOSYSINFO);

    sCoreLen  = idlOS::strlen(CORE_ID);
    sModelLen = idlOS::strlen(MODEL_NAME);

    while(idf::fdgets(sLine, sizeof(sLine), sCPUInfo) != NULL)
    {
        if(idlOS::strncmp(sLine, CORE_ID, sCoreLen) == 0)
        {
            sNumber = idlOS::strstr(sLine, ": ") + 2;
            sCoreID = idlOS::strtol(sNumber, NULL, 10);
            continue;
        }

        if(idlOS::strncmp(sLine, MODEL_NAME, sModelLen) == 0)
        {
            idlOS::strcpy(sModelName, idlOS::strstr(sLine, ": ") + 2);
            continue;
        }

        if(sLine[0] == '\n')
        {
            /* End of each CPU information reached */

            for( sIndex=0; sIndex < mSystemCPUCount ; sIndex++)
            {
                if( mCPUCores[sIndex].mCoreID == sCoreID )
                {
                    idlOS::strcpy(mCPUCores[sIndex].mModelName, sModelName);
                    break;
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT(EOLDKERNEL);
    /* Try again with /proc/cpuinfo */
    IDE_CALLBACK_SEND_SYM("Could not open NUMA information.");
    IDE_CALLBACK_SEND_SYM("Trying again with cpuinfo.");
    IDE_TEST(readCPUInfo() != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION(ENOSYSINFO)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_NO_SUCH_CPUINFO, 
                                errno));
    }
    IDE_EXCEPTION(ENODEEXCEED)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_EXCEED_NUMA_NODE, 
                                sSocketID,
                                IDT_MAX_NUMA_NODES ));
    }
    IDE_EXCEPTION(ECPUEXCEED)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_EXCEED_CPUNO, 
                                sIndex, sSocketID,
                                IDT_MAX_CPU_CORES, IDT_MAX_NUMA_NODES));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#else
    IDE_TEST(readCPUInfo() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

IDE_RC idtCPUSet::destroyStatic(void)
{
    return IDE_SUCCESS;
}

/* 
 * License CPU 개수 제약을 위해 mProcessPset을 재설정 한다.
 * 허용되는 CPU 개수, NUMA Node 개수를 인자로 받고,
 * System에서 뒷 Node, Node에서도 큰 CPU번호 순으로 허용한다.
 * 예를들면, (11,2) 인 경우 사용가능 cpu 번호는 25-29, 34-39 번 CPU이다.  
 * Node[0] xxxxxxxxxx (0~9)
 * Node[1] xxxxxxxxxx (10~19)
 * Node[2] xxxxxooooo (20~29)
 * Node[3] xxxxoooooo (30~39)
 */
IDE_RC idtCPUSet::relocateCPUs(SInt aCPUCount, SInt aNUMACount)
{
#if defined(ALTI_CFG_OS_LINUX)
    SInt sAllocCoreCount;
    SInt sAllocCoreIndex;
    SInt sAllocNUMACount;
    SInt sAllocNUMAIndex;

    SInt i, j;
    SInt sMore = 0;

    
    /* Take minimum of the count of NUMA nodes */
    sAllocNUMACount = IDL_MIN(mNUMACount, aNUMACount);
    IDE_TEST(sAllocNUMACount <= 0);

    /* Take minimum of the count of CPU cores */
    sAllocCoreCount = IDL_MIN(mSystemCPUCount, aCPUCount);
    IDE_TEST(sAllocCoreCount <= 0);
    mAvailableCPUCount = sAllocCoreCount; 

    if ( sAllocCoreCount > 1 )
    {
        sAllocCoreCount = mAvailableCPUCount / sAllocNUMACount;
        sMore = mAvailableCPUCount % sAllocNUMACount;
    }
     
    mProcessPset.clear();

    for(i = 0; i < sAllocNUMACount; i++)
    {
        /* 뒷 NUMA Node부터 탐색 */
        sAllocCoreIndex = IDT_EMPTY;
        sAllocNUMAIndex = mNUMACount - (i % sAllocNUMACount) - 1;

        if( mNUMAPsets[sAllocNUMAIndex].getCPUCount() > 0 )
        {
            for(j=0; j< sAllocCoreCount; j++)
            {
                /* Node의 뒤쪽 CPU부터 탐색 */
                if( mProcessPset.getCPUCount() < mProcessPset.getAvailableCPUCount() )
                {
                    if(sAllocCoreIndex == IDT_EMPTY )
                    {
                        sAllocCoreIndex = mNUMAPsets[sAllocNUMAIndex].findLastCPU();
                    }
                    else
                    {
                        sAllocCoreIndex = mNUMAPsets[sAllocNUMAIndex].findPrevCPU(sAllocCoreIndex);
                    }

                    if( sAllocCoreIndex == IDT_EMPTY )
                    {
                        break;
                       
                    }

                    mProcessPset.addCPU(sAllocCoreIndex);
                    mCPUCores[sAllocCoreIndex].mInUse = ID_TRUE;

                }
            }

            if ( (sMore > 0) && 
                 (mProcessPset.getCPUCount() < mProcessPset.getAvailableCPUCount()) )
            {
                        
                sAllocCoreIndex = mNUMAPsets[sAllocNUMAIndex].findPrevCPU(sAllocCoreIndex);

                if( sAllocCoreIndex != IDT_EMPTY)
                {
                    mProcessPset.addCPU(sAllocCoreIndex);
                    mCPUCores[sAllocCoreIndex].mInUse = ID_TRUE;
                    sMore --;
                }
            }
        }
        else
        {
            /* continue */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#else
    PDL_UNUSED_ARG(aNUMACount);
    PDL_UNUSED_ARG(aCPUCount);

    return IDE_SUCCESS;
#endif
}

/*
 * initialize와 같은 작동을 한다.
 */
idtCPUSet::idtCPUSet(const SInt aFill)
{
    initialize(aFill);
}

/*
 * CPU Set에 현재 시스템에 장착된 CPU 중
 * 라이센스에서 허가된 CPU들을 set한다
 */
void idtCPUSet::fill(void)
{
    copyFrom(mProcessPset);
}

/*
 * CPU Set을 모두 비워서 0으로 만든다.
 */
void idtCPUSet::clear(void)
{
    mCPUCount = 0;
    idlOS::memset(mPSet, 0, IDT_PSET_SIZE);
}

/*
 * aFill이 IDT_FILL이면 fill하고 IDT_EMPTY이면 clear한다.
 * IDT_FILL은 (SInt(0))과, IDT_EMPTY는 ((SInt)(-1))과 같다.
 */
void idtCPUSet::initialize(const SInt aFill)
{
    switch(aFill)
    {
    case IDT_FILL:
        fill();
        break;
    case IDT_EMPTY:
        clear();
        break;
    default:
        IDE_DASSERT(0);
        clear();
        break;
    }
}

/*
 * aCPUNo에 해당하는 CPU를 set한다.
 */
void idtCPUSet::addCPU(const SInt aCPUNo)
{
    SInt    sIndex;
    SInt    sOffset;
    ULong   sDelta;

    IDE_TEST( (aCPUNo < 0) || ((aCPUNo >= mAvailableCPUCount) && (aCPUNo >= mSystemCPUCount)) );
    IDE_TEST( mCPUCount >= mAvailableCPUCount );
    calcIndexAndDelta(aCPUNo, sIndex, sOffset, sDelta);

    if((mPSet[sIndex] & sDelta) == ID_ULONG(0))
    {
        mPSet[sIndex] |= sDelta;
        mCPUCount++;
    }
    else
    {
        /* Do nothing */
    }

    return;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                            aCPUNo, 
                            mSystemCPUCount));
    return;
}

/*
 * aCPUNo에 해당하는 CPU를 clear한다.
 */
void idtCPUSet::removeCPU(const SInt aCPUNo)
{
    SInt    sIndex;
    SInt    sOffset;
    ULong   sDelta;

    IDE_TEST( (aCPUNo < 0) || ((aCPUNo >= mAvailableCPUCount) && (aCPUNo >= mSystemCPUCount)) );
    calcIndexAndDelta(aCPUNo, sIndex, sOffset, sDelta);

    if((mPSet[sIndex] & sDelta) != ID_ULONG(0))
    {
        mPSet[sIndex] &= ~(sDelta);
        mCPUCount--;
    }
    else
    {
        /* Do nothing */
    }

    return;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                            aCPUNo, 
                            mSystemCPUCount));
    return;

}

/*
 * aCPUNo가 set되어 있으면 clear, clear상태이면 set한다.
 */
void idtCPUSet::toggleCPU(const SInt aCPUNo)
{
    SInt    sIndex;
    SInt    sOffset;
    ULong   sDelta;

    IDE_TEST( (aCPUNo < 0) || ((aCPUNo >= mAvailableCPUCount) && (aCPUNo >= mSystemCPUCount)) );
    calcIndexAndDelta(aCPUNo, sIndex, sOffset, sDelta);

    if((mPSet[sIndex] & sDelta) == ID_ULONG(0))
    {
        mPSet[sIndex] |= sDelta;
        mCPUCount++;
    }
    else
    {
        mPSet[sIndex] &= ~(sDelta);
        mCPUCount--;
    }

    return;

    IDE_EXCEPTION_END;
 
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                            aCPUNo, 
                            mSystemCPUCount));
    return;

}

/*
 * aNUMANo NUMA 노드에 해당하는 CPU를 모두 set한다.
 */
void idtCPUSet::addNUMA(const SInt aNUMANo)
{
    IDE_TEST( (aNUMANo < 0) || (aNUMANo >= mNUMACount) );

    mergeFrom(mNUMAPsets[aNUMANo]);
   
    return;
    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_NUMA_NODE_NO, 
                            aNUMANo,
                            mNUMACount));
    return;
}

/*
 * aNUMANo NUMA 노드에 해당하는 CPU를 모두 clear한다.
 */
void idtCPUSet::removeNUMA(const SInt aNUMANo)
{
    SInt   i;

    IDE_TEST( (aNUMANo < 0) || (aNUMANo >= mNUMACount) );
    mCPUCount = 0;
    for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
    {
        mPSet[i] &= ~(mNUMAPsets[aNUMANo].mPSet[i]);
        mCPUCount += acpBitCountSet64(mPSet[i]);
    }
   
    return;
    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_NUMA_NODE_NO, 
                            aNUMANo, 
                            mNUMACount));
    return;
}

/*
 * aCPUNo가 현재 CPU Set에 포함되어 있으면 ID_TRUE를 리턴한다.
 * 아니면 ID_FALSE를 리턴한다.
 */
idBool idtCPUSet::find(const SInt aCPUNo)
{
    SInt    sIndex;
    SInt    sOffset;
    ULong   sDelta;
    idBool  sRet;

    IDE_TEST( (aCPUNo < 0) || ((aCPUNo >= mAvailableCPUCount) && (aCPUNo >= mSystemCPUCount)) );
    calcIndexAndDelta(aCPUNo, sIndex, sOffset, sDelta);

    sRet = ((mPSet[sIndex] & sDelta) == ID_ULONG(0))?
        ID_FALSE : ID_TRUE;

    return sRet;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                            aCPUNo, 
                            mSystemCPUCount));
    return ID_FALSE;
}

/*
 * CPU Set에 설정되어있는 CPU 개수를 리턴한다.
 */
SInt idtCPUSet::getCPUCount(void)
{
    return mCPUCount;
}

/*
 * CPU Set에 설정된 CPU들이 NUMA 노드 몇 개를
 * 사용하고 있는가 리턴한다.
 */
SInt idtCPUSet::getNUMACount(void)
{
    SInt   sRet = 0;
    SInt   i;

    for(i = 0; i < IDT_MAX_NUMA_NODES; i++)
    {
        if( getCPUCountInNUMA(i) > 0 )
        {
            sRet++;
        }
    }
    return sRet;
}

/*
 * CPU Set에 설정된 CPU 중 aNUMANo에 속하는
 * CPU의 개수를 리턴한다.
 */
SInt idtCPUSet::getCPUCountInNUMA(const SInt aNUMANo)
{
    SInt   sRet = IDT_EMPTY;
    SInt   i;
    ULong  sDelta;

    IDE_TEST( (aNUMANo < 0) || (aNUMANo >= mNUMACount) );
    for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
    {
        sDelta = mPSet[i] & mNUMAPsets[aNUMANo].mPSet[i];
        sRet +=  acpBitCountSet64(sDelta);
    }
   
    return sRet;
    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_NUMA_NODE_NO, 
                            aNUMANo, 
                            mNUMACount));
    return sRet;

}

/*
 * 현재 CPU Set에 aNUMANo에 해당하는 CPU가 있는가 확인한다.
 */
idBool idtCPUSet::isInNUMA(const SInt aNUMANo)
{
    IDE_TEST( (aNUMANo < 0) || (aNUMANo >= mNUMACount) );

    return implies(mNUMAPsets[aNUMANo]);

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_NUMA_NODE_NO, 
                            aNUMANo, 
                            mNUMACount));
    return ID_FALSE;
}

/*
 * aCPUSet에 설정된 CPU Set을 현재 인스턴스로 복사해온다.
 * *this를 리턴한다.
 */
const idtCPUSet& idtCPUSet::copyFrom(const idtCPUSet& aCPUSet)
{
    idlOS::memcpy(mPSet, aCPUSet.mPSet, IDT_PSET_SIZE);
    mCPUCount = aCPUSet.mCPUCount;

    return *this;
}

/*
 * aCPUSet에 설정된 CPU Set과 현재 인스턴스에 설정된
 * CPU Set를 OR하여 합친다.
 * *this를 리턴한다.
 */
const idtCPUSet& idtCPUSet::mergeFrom(const idtCPUSet& aCPUSet)
{
    SInt    i;

    mCPUCount = 0;
    for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
    {
        mPSet[i] |= aCPUSet.mPSet[i];
        mCPUCount += acpBitCountSet64(mPSet[i]);
    }
    return *this;
}

/*
 * CPU Set에 설정된 CPU 중 logical ID가
 * 가장 작은 CPU 번호를 리턴한다.
 */
SInt idtCPUSet::findFirstCPU(void)
{
    SInt    sRet = IDT_EMPTY;
    SInt    sOffset;
    SInt    i;

    for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
    {
        if(mPSet[i] == 0)
        {
            continue;
        }
        else
        {
            sOffset = acpBitFfs64(mPSet[i]);
            sRet = i * 64 + sOffset;
            break;
        }
    }

    return sRet;
}

/* 
 * CPU Set에 설정되어있고 aCPUNo보다 logical ID가 큰 CPU 중
 * logical ID가 가장 작은 CPU 번호를 리턴한다.
 * 해당 CPU가 Set 내부에 없으면 IDT_EMPTY를 리턴한다.
 */
SInt idtCPUSet::findNextCPU(const SInt aCPUNo)
{
    SInt    sRet = IDT_EMPTY;
    SInt    sIndex;
    SInt    sOffset;
    ULong   sDelta;
    SInt    i;

    IDE_TEST( (aCPUNo < 0) || ((aCPUNo >= mAvailableCPUCount) && (aCPUNo >= mSystemCPUCount)) );
    calcIndexAndDelta(aCPUNo, sIndex, sOffset, sDelta);
 
    if ( sOffset != 63 )
    {
        sDelta = mPSet[sIndex] >> (sOffset + 1);
        sDelta = sDelta << (sOffset + 1);
    }
    else if( sIndex != IDT_PSET_MAX_INDEX )
    {
        sIndex++;
        sDelta = mPSet[sIndex];
    }
    else
    {
        sDelta = 0;
    }

    if(sDelta != ID_ULONG(0))
    {
        sOffset = acpBitFfs64(sDelta);
        sRet = sIndex * 64 + sOffset;
    }
    else
    {
        for(i = sIndex + 1 ; i < IDT_PSET_MAX_INDEX ; i++)
        {
            if( mPSet[i] == ID_ULONG(0) )
            {
                continue;
            }
            else
            {
                sOffset = acpBitFfs64(mPSet[i]);
                sRet = i * 64 + sOffset;
                break;
            }
        }
    }
    return sRet;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                            aCPUNo, 
                            mSystemCPUCount));
    return sRet;
}
/*
 * CPU Set에 설정된 CPU 중 logical ID가
 * 가장 큰 CPU 번호를 리턴한다.
 */
SInt idtCPUSet::findLastCPU(void)
{
    SInt    sRet = IDT_EMPTY;
    SInt    sOffset;
    SInt    i;

    for(i = IDT_PSET_MAX_INDEX -1 ; i >= 0 ; i--)
    {
        if(mPSet[i] == 0)
        {
            continue;
        }
        else
        {
            sOffset = acpBitFls64(mPSet[i]);
            sRet = i * 64 + sOffset;
            break;
        }
    }

    return sRet;
}
/* 
 * CPU Set에 설정되어있고 aCPUNo보다 logical ID가 작은 CPU 중
 * logical ID가 가장 큰 CPU 번호를 리턴한다.
 * 해당 CPU가 Set 내부에 없으면 IDT_EMPTY를 리턴한다.
 */
SInt idtCPUSet::findPrevCPU(const SInt aCPUNo)
{
    SInt    sRet = IDT_EMPTY;
    SInt    sIndex;
    SInt    sOffset;
    ULong   sDelta;
    SInt    i;

    IDE_TEST( (aCPUNo < 0) || ((aCPUNo >= mAvailableCPUCount) && (aCPUNo >= mSystemCPUCount)) );
    calcIndexAndDelta(aCPUNo, sIndex, sOffset, sDelta);
 
    if( sOffset != 0 )
    {
        sDelta = mPSet[sIndex] << ((sizeof(ULong) * 8) - sOffset);
        sDelta = sDelta >> ((sizeof(ULong) * 8) - sOffset);
    }
    else if (sIndex != 0)
    {
        sIndex--;
        sDelta = mPSet[sIndex];
    }
    else
    {
        sDelta = 0;
    }

    if(sDelta != ID_ULONG(0))
    {
        sOffset = acpBitFls64(sDelta);
        sRet = sIndex * 64 + sOffset;
    }
    else
    {
        for(i = sIndex - 1 ; i >= 0 ; i--)
        {
            if( mPSet[i] == ID_ULONG(0) )
            {
                continue;
            }
            else
            {
                sOffset = acpBitFls64(mPSet[i]);
                sRet = i * 64 + sOffset;
                break;
            }
        }
    }
    return sRet;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                            aCPUNo, 
                            mSystemCPUCount));
    return sRet;
}



/*
 * CPU Set에 설정되어 있고 aNUMANo에 속하는 CPU 중
 * logical ID가 가장 작은 CPU 번호를 리턴한다.
 */
SInt idtCPUSet::findFirstNUMA(const SInt aNUMANo)
{
    SInt sRet = IDT_EMPTY;

    IDE_TEST( (aNUMANo < 0) || (aNUMANo >= mNUMACount) );

    sRet = mNUMAPsets[aNUMANo].findFirstCPU();
    while( sRet != IDT_EMPTY )
    {
        if (find(sRet) == ID_TRUE )
        {
            break;
        }
        sRet = mNUMAPsets[aNUMANo].findNextCPU(sRet);
    }
    return sRet;
    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_NUMA_NODE_NO,
                            aNUMANo, 
                            mNUMACount));
    return sRet;
}

/*
 * CPU Set에 설정되어 있고 aNUMANo에 속해 있으며,
 * aCPUNo보다 logical ID가 큰 CPU 중
 * logical ID가 가장 작은 CPU 번호를 리턴한다.
 * 해당 CPU가 Set 내부에 없으면 IDT_EMPTY를 리턴한다.
 */
SInt idtCPUSet::findNextNUMA(const SInt aCPUNo, const SInt aNUMANo)
{
    SInt sRet = IDT_EMPTY;

    IDE_TEST_RAISE( (aCPUNo < 0) || (aCPUNo >= mSystemCPUCount),
                    ECPUINVALID);
    IDE_TEST_RAISE((aNUMANo < 0) || (aNUMANo >= mNUMACount), 
                   ENODEINVALID);

    sRet = mNUMAPsets[aNUMANo].findNextCPU(aCPUNo);
    while( sRet != IDT_EMPTY )
    {
        if (find(sRet) == ID_TRUE )
        {
            break;
        }
        sRet = mNUMAPsets[aNUMANo].findNextCPU(sRet);
    }
 
    return sRet;
    IDE_EXCEPTION(ECPUINVALID)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_CPUNO, 
                                aCPUNo, 
                                mSystemCPUCount));
    }
    IDE_EXCEPTION(ENODEINVALID)
    {
        IDE_SET(ideSetErrorCode(idERR_IGNORE_INVALID_NUMA_NODE_NO,
                                aNUMANo,
                                mNUMACount));
    }
    IDE_EXCEPTION_END;
    return sRet;
}

/*
 * 현재 인스턴스와 aCPUSet이 동일한가 비교한다.
 * 동일하면 ID_TRUE를, 아니면 ID_FALSE를 리턴한다.
 */
idBool idtCPUSet::compare(const idtCPUSet& aCPUSet)
{
    idBool sRet;

    if(mCPUCount == aCPUSet.mCPUCount)
    {
        sRet = (idlOS::memcmp(mPSet, aCPUSet.mPSet, IDT_PSET_SIZE) == 0)?
            ID_TRUE : ID_FALSE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

/*
 * aCPUSet이 현재 인스턴스의 부분집합인가 확인한다.
 * 부분집합이라면 ID_TRUE를, 아니면 ID_FALSE를 리턴한다.
 */
idBool idtCPUSet::implies(const idtCPUSet& aCPUSet)
{
    idBool sRet;
    SInt   i;

    if(mCPUCount >= aCPUSet.mCPUCount)
    {
        for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
        {
            if( (mPSet[i] | aCPUSet.mPSet[i]) != mPSet[i] )
            {
                sRet = ID_FALSE;
                break;
            }
            else
            {
                /* Do nothing */
            }
            sRet = ID_TRUE;
        }
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;

}
 
/*
 * 현재 인스턴스를 모두 비운 후
 * CPU Set 1과 CPU Set 2에 공통적으로 포함된 CPU Set들만을
 * 현재 instance에 set한다.
 * *this를 리턴한다.
 */
const idtCPUSet& idtCPUSet::makeIntersectionFrom(const idtCPUSet& aCPUSet1,
                                                 const idtCPUSet& aCPUSet2)
{
    SInt i;

    clear();
    for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
    {
        mPSet[i] = aCPUSet1.mPSet[i] & aCPUSet2.mPSet[i];
        mCPUCount += acpBitCountSet64(mPSet[i]);
    }
    return *this;
}

/*
 * 현재 인스턴스를 모두 비운 후
 * CPU Set 1과 CPU Set 2에 포함된 CPU Set들을 모두
 * 현재 instance에 set한다.
 * *this를 리턴한다.
 */
const idtCPUSet& idtCPUSet::makeUnionFrom(const idtCPUSet& aCPUSet1,
                                          const idtCPUSet& aCPUSet2)
{
    SInt i;

    clear();
    for(i = 0; i < IDT_PSET_MAX_INDEX; i++)
    {
        mPSet[i] = aCPUSet1.mPSet[i] | aCPUSet2.mPSet[i];
        mCPUCount += acpBitCountSet64(mPSet[i]);
    }
    return *this;
}

/*
 * 현재 CPU Set을 문자열로 변환하여 aString에 저장한다.
 * aString의 최대 길이는 aLen이며 항상 NUL('\0')로 끝난다.
 */
void idtCPUSet::dumpCPUsToString(SChar* aString, const size_t aLen)
{
    SInt    sPrevCoreID = IDT_EMPTY;       /* 마지막 core id를 저장 */
    SInt    i;
    size_t  sTotalLen;
    size_t  sLen;
    idBool  sIsCont = ID_FALSE;           /* 이전 core id와 연속중인지 확인하기위해 */
    idBool  sCoreIDs[IDT_MAX_CPU_CORES];  /* cpuid를 coreid 순으로 정렬 */
   
    sTotalLen = aLen;
    idlOS::memset(sCoreIDs, 0, IDT_MAX_CPU_CORES*sizeof(idBool));

    sortCoreID(sCoreIDs);

    for(i=0; i<IDT_MAX_CPU_CORES; i++)
    {
         if( sCoreIDs[i] == ID_TRUE )
         {
             if( sPrevCoreID == IDT_EMPTY )
             {
                 /* 최초 기록
                  * aString 에 i 추가 */
                 sLen = idlOS::snprintf(aString, sTotalLen, "%d", i);
                 IDE_TEST( aLen <= sLen );
                 aString += sLen;
                 sTotalLen -= sLen;
             }
             else if( i - sPrevCoreID == 1 )
             {
                 /* 이전 core id 와 연속 */

                 if( sIsCont == ID_FALSE )
                 {
                     /* 최초 연속
                      * aString 에 '-' 추가 */
                     sLen = idlOS::snprintf(aString, sTotalLen, "-");
                     IDE_TEST( aLen <= sLen );
                     aString += sLen;
                     sTotalLen -= sLen;

                 }
                 else
                 {
                     /* 연속 중 */
                 }
                 sIsCont = ID_TRUE;
             }
             else
             {
                 /* 이전 core id와 연속 안함 */

                 if( sIsCont == ID_TRUE )
                 {
                     /* 이전 core id 까지 연속이였고, 지금은 연속이 아님
                      * astring에 'sPrevCoreID, i' 추가 */
                     sLen = idlOS::snprintf(aString, sTotalLen, "%d, %d", sPrevCoreID, i);
                 }
                 else
                 {
                     /* 이전도 연속이 아니였고, 지금도 연속이 아님
                      * astring에 ', i' 추가*/
                     sLen = idlOS::snprintf(aString, sTotalLen, ", %d", i);
                 }
                 IDE_TEST( aLen <= sLen );
                 aString += sLen;
                 sTotalLen -= sLen;

                 sIsCont = ID_FALSE;
             }
                 
             sPrevCoreID = i;
         }
    }
    if( sIsCont == ID_TRUE )
    {
        /* 연속중이던 마지막 값 기록해야함
         * aString 에 'sPrevCoreID' 기록*/
        sLen = idlOS::snprintf(aString, sTotalLen, "%d", sPrevCoreID);
        IDE_TEST( aLen <= sLen );
        aString += sLen;
        sTotalLen -= sLen;
    }

    return;
    IDE_EXCEPTION_END;
    return;
}

/*
 * 현재 CPU Set을 문자열로 변환하여 aString에 저장한다.
 * aString의 최대 길이는 aLen이며 항상 NUL('\0')로 끝난다.
 */
void idtCPUSet::dumpCPUsToHexString(SChar* aString, const size_t aLen)
{
    ULong   sCoreIDSet[IDT_PSET_MAX_INDEX];
    ULong   sDelta;
    SInt    sIndex;
    SInt    sOffset;
    SInt    i;
    size_t  sLen;
    size_t  sTotalLen;
    idBool  sCoreIDs[IDT_MAX_CPU_CORES];  /* cpuid를 coreid 순으로 정렬 */
  
    sLen = 0;
    sTotalLen = aLen;
    idlOS::memset(sCoreIDSet, 0, IDT_PSET_SIZE);
    idlOS::memset(sCoreIDs, 0, IDT_MAX_CPU_CORES*sizeof(idBool));

    sortCoreID(sCoreIDs);

    for(i=0; i<IDT_MAX_CPU_CORES; i++)
    {
        if(sCoreIDs[i] == ID_TRUE)
        {
            calcIndexAndDelta(i, sIndex, sOffset, sDelta);
            sCoreIDSet[sIndex] |= sDelta;
        }
    }

    for(i=IDT_PSET_MAX_INDEX-1; i>=0; i--)
    {
        if( sLen == 0)
        {
            if( sCoreIDSet[i] != 0 )
            {
                sLen = idlOS::snprintf(aString, sTotalLen, "%016lX", sCoreIDSet[i]);
                IDE_TEST( aLen <= sLen );
                aString += sLen;
                sTotalLen -= sLen;
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            sLen = idlOS::snprintf(aString, sTotalLen, "%016lX", sCoreIDSet[i]);
            IDE_TEST( aLen <= sLen );
            aString += sLen;
            sTotalLen -= sLen;
        }
    }
    
    return;
    IDE_EXCEPTION_END;
    return;
}

void idtCPUSet::convertToPhysicalPset(acp_pset_t* aPSet)
{
    SInt sCPUNo;
    SInt sCoreID;

    ACP_PSET_ZERO(aPSet);

    sCPUNo = findFirstCPU();

    while(sCPUNo != IDT_EMPTY)
    {
        sCoreID = mCPUCores[sCPUNo].mCoreID;
        ACP_PSET_SET(aPSet, sCoreID);

        sCPUNo = findNextCPU(sCPUNo);
    }
}

/*
 * CPU Set에 설정된 CPU를 core ID 순으로 정렬하여
 * aCoreIDs에 저장한다.
 */
void idtCPUSet::sortCoreID(idBool* aCoreIDs)
{
    SInt   sRet = IDT_EMPTY;
    SInt   sIndex;

    sRet = findFirstCPU();
    while( sRet != IDT_EMPTY )
    {
        sIndex = mCPUCores[sRet].mCoreID;
        aCoreIDs[sIndex] = ID_TRUE;
        sRet = findNextCPU(sRet);
    }
}
void idtCPUSet::calcIndexAndDelta(const SInt    aCPUNo,
                                  SInt&         aIndex,
                                  SInt&         aOffset,
                                  ULong&        aDelta)
{
    IDE_DASSERT(aCPUNo <  IDT_MAX_CPU_CORES);
    IDE_DASSERT(aCPUNo >= 0);

    aIndex  = aCPUNo / (sizeof(ULong) * 8);
    aOffset = aCPUNo % (sizeof(ULong) * 8);
    aDelta  = (ID_ULONG(1) << aOffset);
}

/*
 * 스레드에 CPU 여러 개를 bind할 수 있는가를 리턴한다.
 * Linux에서는 ID_TRUE를, 여타 운영체제에서는 ID_FALSE를 리턴한다.
 * Linux를 제외한 운영체제에서는 한 스레드에 CPU 여러 개를 bind하려면
 * root 권한이 필요하다.
 */
idBool idtCPUSet::canSetMultipleCPUs(void)
{
#if defined(ALTI_CFG_OS_LINUX)
    return ID_TRUE;
#else
    return ID_FALSE;
#endif
}

/* 시스템에 설치되어 있는 전체 CPU 개수를 리턴한다. */
SInt idtCPUSet::getSystemCPUCount(void)
{
    return mSystemCPUCount;
}

/*
 * 시스템에 설치되어 있는 CPU 중 라이센스로 허가되어
 * 현재 IN_USE=YES인 CPU의 개수를 리턴한다.
 */
SInt idtCPUSet::getAvailableCPUCount(void)
{
    return mAvailableCPUCount;
}

/*
 * 시스템에 설치되어 있는 NUMA 노드 개수를 리턴한다.
 */
SInt idtCPUSet::getSystemNUMACount(void)
{
    return mNUMACount;
}

/*
 * 현재 스레드의 affinity를 CPU Set으로 설정한다.
 */
IDE_RC idtCPUSet::bindThread(void)
{
    acp_pset_t sPSet;

    convertToPhysicalPset(&sPSet);
    IDE_TEST(acpPsetBindThread(&sPSet) != ACP_RC_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_BINDTHREAD_FAILED));
    return IDE_FAILURE;
}

/*
 * 현재 스레드의 affinity를 해제한다.
 * Linux에서는 라이센스로 인하여 사용 가능한 CPU Core가 제한되어 있으면
 * 제한된 CPU Set에 현재 스레드를 bind한다.
 */

IDE_RC idtCPUSet::unbindThread(void)
{
#if defined(ALTI_CFG_OS_LINUX)
    IDE_TEST(mProcessPset.bindThread() != ACP_RC_SUCCESS);
#else
    IDE_TEST(acpPsetUnbindThread() != ACP_RC_SUCCESS);
#endif
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_UNBINDTHREAD_FAILED,
                            errno));

    return IDE_FAILURE;
}
     
/*
 * 현재 프로세스의 affinity를 CPU Set으로 설정한다.
 */
IDE_RC idtCPUSet::bindProcess(void)
{
    acp_pset_t sPSet;

    convertToPhysicalPset(&sPSet);
    IDE_TEST(acpPsetBindProcess(&sPSet) != ACP_RC_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_BIND_PROCESS_FAILED));
    return IDE_FAILURE;
}

/*
 * 현재 프로세스의 affinity를 해제한다.
 * Linux에서는 라이센스로 인하여 사용 가능한 CPU Core가 제한되어 있으면
 * 제한된 CPU Set에 현재 프로세스를 bind한다.
 */
IDE_RC idtCPUSet::unbindProcess(void)
{
    IDE_TEST(acpPsetUnbindProcess() != ACP_RC_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET(ideSetErrorCode(idERR_IGNORE_UNBIND_PROCESS_FAILED,
                            errno));
    return IDE_FAILURE;
}

struct idtCPUCoreStat
{
    SInt    mCoreID;
    SInt    mSocketID;
    SInt    mLogicalID;
    SChar   mModelName[128];
    SChar   mInUse[4];

    void setInfo(const idtCPUCore& aCoreInfo)
    {
        mCoreID         = aCoreInfo.mCoreID;
        mSocketID       = aCoreInfo.mSocketID;
        mLogicalID      = aCoreInfo.mLogicalID;

        idlOS::strcpy(mInUse, (aCoreInfo.mInUse == ID_TRUE)? "YES":"N0");
        idlOS::strcpy(mModelName, aCoreInfo.mModelName);
    }
};

static iduFixedTableColDesc gCPUCoreColDesc[] = 
{
    {
        (SChar*)"LOGICAL_ID",
        IDU_FT_OFFSETOF(idtCPUCoreStat, mLogicalID),
        sizeof(SInt),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NUMA_ID",
        IDU_FT_OFFSETOF(idtCPUCoreStat, mSocketID),
        sizeof(SInt),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CORE_ID",
        IDU_FT_OFFSETOF(idtCPUCoreStat, mCoreID),
        sizeof(SInt),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"IN_USE",
        IDU_FT_OFFSETOF(idtCPUCoreStat, mInUse),
        4,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MODEL_NAME",
        IDU_FT_OFFSETOF(idtCPUCoreStat, mModelName),
        128,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }

};

static IDE_RC buildRecordForCPUCore(idvSQL*       /* aSQL */,
        void        *aHeader,
        void        * /* aDumpObj */,
        iduFixedTableMemory *aMemory)
{
    SInt            i;
    idtCPUCoreStat  sCPUs;

    for(i = 0; i < idtCPUSet::mSystemCPUCount; i++)
    {
        sCPUs.setInfo(idtCPUSet::mCPUCores[i]);
        IDE_TEST( iduFixedTable::buildRecord(aHeader,
                    aMemory,
                    (void *)&sCPUs  )
                != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gCPUCoreTableDesc =
{
    (SChar *)"X$CPUCORE",
    buildRecordForCPUCore,
    gCPUCoreColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

struct idtCPUNUMAStat
{
    SInt    mNUMAID;
    SChar   mCPUIDs[128];
    SChar   mCPUIDsHex[128];

    void setInfo(const SInt aNUMAID, idtCPUSet& aNUMASet)
    {
        mNUMAID = aNUMAID;
        aNUMASet.dumpCPUsToString(mCPUIDs, 128);
        aNUMASet.dumpCPUsToHexString(mCPUIDsHex, 128);
    }
};

static iduFixedTableColDesc gNUMAStatColDesc[] = 
{
    {
        (SChar*)"NUMA_ID",
        IDU_FT_OFFSETOF(idtCPUNUMAStat, mNUMAID),
        sizeof(SInt),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CPUSET",
        IDU_FT_OFFSETOF(idtCPUNUMAStat, mCPUIDs),
        128,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CPUSETHEX",
        IDU_FT_OFFSETOF(idtCPUNUMAStat, mCPUIDsHex),
        128,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }

};

static IDE_RC buildRecordForNUMAStat(idvSQL*       /* aSQL */,
        void        *aHeader,
        void        * /* aDumpObj */,
        iduFixedTableMemory *aMemory)
{
    SInt            i;
    idtCPUNUMAStat  sCPUs;

    for(i = 0; i < idtCPUSet::mNUMACount; i++)
    {
        sCPUs.setInfo(i, idtCPUSet::mNUMAPsets[i]);
        IDE_TEST( iduFixedTable::buildRecord(aHeader,
                    aMemory,
                    (void *)&sCPUs  )
                != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gNUMAStatTableDesc =
{
    (SChar *)"X$NUMASTAT",
    buildRecordForNUMAStat,
    gNUMAStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



