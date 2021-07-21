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

#ifndef _O_MMC_PCB_H_
#define _O_MMC_PCB_H_ 1

#include <iduMutex.h>
#include <iduList.h>
#include <mmcChildPCO.h>
#include <mmcDef.h>


class mmcParentPCO;


// Plan Cache Control Block
class mmcPCB
{
public:
    //fix BUG-21429
    void   initialize(SChar* aSQLTextID,
                      UInt   aChildPCOId);
    void   finalize();
    void   planFix(idvSQL* aStatistics);
    void   planUnFix(idvSQL* aStatistics);
    void   resetFrequency(idvSQL* aStatistics);
    /* fix BUG-31212
      When a PCO move from cold to hot lru region, it would be better to update the frequency of a pco as
      (the frequency - SQL_PLAN_CACHE_HOT_REGION_FREQUENCY) instead of 0.*/
    void   decFrequency(idvSQL* aStatistics);
    
    void   setLRURegion(idvSQL* aStatistics,
                        mmcPCBLRURegion  aLRURegion);
    
    void   getLRURegion(idvSQL* aStatistics,
                        mmcPCBLRURegion  *aLRURegion);
    
    void   assignPCO(idvSQL* aStatistics,
                     mmcParentPCO* aParentPCO,
                     mmcChildPCO* aChildPCO);
    
    void   getFixCountFrequency(idvSQL* aStatistics,
                                UInt* aFixCount,
                                UInt* aFrequency);
    void   getFixCount(idvSQL* aStatistics,
                       UInt* aFixCount);
    
    void   getFrequency(UInt* aFrequency);
    
    mmcChildPCO*         getChildPCO();
    mmcParentPCO*         getParentPCO();
    inline UInt*   getFrequencyPtr();
    inline iduListNode*  getChildLstNode();
    inline iduListNode*  getLRuListNode();
    inline UInt          getChildPCOSize();

    
private:
    UInt             mFixCount;
    UInt             mFrequency;
    mmcPCBLRURegion  mLRURegion;
    iduMutex         mMutex;
    mmcChildPCO     *mChildPCO;
    mmcParentPCO    *mParentPCO;
    //LRU List에서 node 
    iduListNode      mLRuListNode;
    //Parent PCO에서 child PCO list에 사용된다.
    iduListNode      mChildLstNode;
};

inline iduListNode* mmcPCB::getChildLstNode()
{
    return &mChildLstNode;
}

inline iduListNode* mmcPCB::getLRuListNode()
{
    return &mLRuListNode;
}




inline UInt       mmcPCB::getChildPCOSize()
{
    if(mChildPCO == NULL)
    {
        return  (0);
    }
    else
    {
        return  mChildPCO->getSize();
    }
}

inline UInt*   mmcPCB::getFrequencyPtr()
{
    return &mFrequency;
}

#endif
