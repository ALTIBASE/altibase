/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  _O_MMC_CHILD_PCO_H_
#define  _O_MMC_CHILD_PCO_H_
#include <idl.h>
#include <iduLatch.h>
#include <iduList.h>
#include <qci.h>
#include <mmcDef.h>


class mmcChildPCO
{
public:
    void   initialize(SChar         *aSQLTextId,
                      const UInt     aChildID,
                      mmcChildPCOCR  aCreateReason,
                      const UInt     aRebuildedCnt,
                      UInt          *aHitCntPtr);    
    void   finalize();
    void   latchPrepareAsShared(idvSQL* aStatistics);
    void   latchPrepareAsExclusive(idvSQL* aStatistics);
    void   releasePrepareLatch();
    
    
    inline void   getPlanState(mmcChildPCOPlanState* aPlanState);
    inline mmcChildPCOPlanState getPlanState();
    //fix BUG-24607 hard-prepare대기 완료후에 child PCO가
    //old PCO list으로 옮겨질수 있다.
    void   wait4HardPrepare(idvSQL* aStatistics);
    void   freePreparedPrivateTemplate();
    void   assignEnv(qciPlanProperty *aEnvironment,
                     qciPlanBindInfo *aPlanBindInfo); // PROJ-2163
    void   assignPlan(void  *aSharedPlan,
                      UInt   aSharedPlanSize,
                      void  *aPreparedPrivateTemplate,
                      UInt   aPreparedPrivateTemplateSize);
    inline void*  getSharedPlan();
    inline void*  getPreparedPrivateTemplate();
    inline UInt   getSize();
    inline qciPlanProperty*     getEnvironment();
    inline mmcChildPCOEnvState  getEnvState();
    void  updatePlanState(mmcChildPCOPlanState aPlanState);
    inline UInt  getRebuildCnt();
    inline qciPlanBindInfo*     getPlanBindInfo(); // PROJ-2163
 
    //for performance view.
    SChar                *mSQLTextId;
    UInt                  mChildID;
    mmcChildPCOCR         mCreateReason;
    UInt                  mRebuildedCnt;
    mmcChildPCOPlanState  mPlanState;
    UInt                 *mHitCntPtr;
private:
    // PROJ-2408
    iduLatch              mPrepareLatch;
    qciPlanProperty       mEnvironment;
    qciPlanBindInfo       mPlanBindInfo; // PROJ-2163
    mmcChildPCOEnvState   mEnvState;
    void                 *mSharedPlan;
    void                 *mPreparedPrivateTemplate;
    UInt                  mPreparedPrivateTemplateSize;
    UInt                  mSharedPlanSize;
};




inline void* mmcChildPCO::getSharedPlan()
{
    return mSharedPlan;
}

inline void* mmcChildPCO::getPreparedPrivateTemplate()
{
    return mPreparedPrivateTemplate;
}

inline UInt  mmcChildPCO::getSize()
{
    return mSharedPlanSize+mPreparedPrivateTemplateSize;
}

inline qciPlanProperty*  mmcChildPCO::getEnvironment()
{
    return &mEnvironment;
}

inline mmcChildPCOEnvState mmcChildPCO::getEnvState()
{
    return mEnvState;
}

inline UInt  mmcChildPCO::getRebuildCnt()
{
    return mRebuildedCnt;
}

inline void  mmcChildPCO::getPlanState(mmcChildPCOPlanState* aPlanState)
{
    *aPlanState = mPlanState;
}

inline mmcChildPCOPlanState mmcChildPCO::getPlanState()
{
    return mPlanState;
}

// PROJ-2163
inline qciPlanBindInfo*  mmcChildPCO::getPlanBindInfo()
{
    return &mPlanBindInfo;
}
        
#endif
