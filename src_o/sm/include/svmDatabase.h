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
 
#ifndef _O_SVM_DATABSE_H_
#define _O_SVM_DATABSE_H_ 1

#include <svmDef.h>

class svmDatabase
{
public :
     // membase를 초기화한다.
    static IDE_RC initializeMembase( svmTBSNode * aTBSNode,
                                     SChar      * aDBName,
                                     vULong       aChunkPageCount );

    /*-----------------------------
       Basic Interfaces for Membase
    -------------------------------*/
    // mAllocPersPageCount
    static inline vULong getAllocPersPageCount(svmMemBase * aMemBase);
/* not used 
    static inline void setAllocPersPageCount(svmMemBase * aMemBase,
                                             vULong       aAllocPersPageCount);
*/    
    // mExpandChunkPageCnt
    static inline vULong getExpandChunkPageCnt(svmMemBase * aMemBase);
    static inline void setExpandChunkPageCnt(svmMemBase * aMemBase,
                                             vULong       aExpandChunkPageCnt);
    
    // mmCurrentExpandChunkCnt
    static inline vULong getCurrentExpandChunkCnt(svmMemBase * aMemBase);
    static inline void setCurrentExpandChunkCnt(svmMemBase * aMemBase,
                                                vULong       amCurrentExpandChunkCnt);
    
    // mFreePageListCount
    static inline UInt getFreePageListCount(svmMemBase * aMemBase);
    static inline void setFreePageListCount(svmMemBase * aMemBase,
                                            UInt         aFreePageListCount);
    
    // mFreePageLists
    static inline svmDBFreePageList getFreePageList(svmMemBase * aMemBase,
                                                    UInt         aFPListIdx);
    static inline void setFreePageList(svmMemBase * aMemBase,
                                       UInt         aFreePageListIdx,
                                       scPageID     aFirstFreePageID,
                                       vULong       aFreePageCount);

    static IDE_RC checkExpandChunkProps(svmMemBase * aMemBase);
};

// mAllocPersPageCount
inline vULong svmDatabase::getAllocPersPageCount(svmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mAllocPersPageCount;
}
/* not used 
inline void svmDatabase::setAllocPersPageCount(svmMemBase * aMemBase,
                                               vULong       aAllocPersPageCount)
{
    aMemBase->mAllocPersPageCount = aAllocPersPageCount;
}
*/
// mExpandChunkPageCnt
inline vULong svmDatabase::getExpandChunkPageCnt(svmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mExpandChunkPageCnt;
}
inline void svmDatabase::setExpandChunkPageCnt(svmMemBase * aMemBase,
                                               vULong       aExpandChunkPageCnt)
{
    IDE_DASSERT( aMemBase != NULL );
    
    aMemBase->mExpandChunkPageCnt = aExpandChunkPageCnt;
}

// mCurrentExpandChunkCnt
inline vULong svmDatabase::getCurrentExpandChunkCnt(svmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mCurrentExpandChunkCnt;
}
inline void svmDatabase::setCurrentExpandChunkCnt(svmMemBase * aMemBase,
                                                  vULong       aCurrentExpandChunkCnt)
{
    IDE_DASSERT( aMemBase != NULL );
    
    aMemBase->mCurrentExpandChunkCnt = aCurrentExpandChunkCnt;
}

// mFreePageListCount
inline UInt svmDatabase::getFreePageListCount(svmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mFreePageListCount;
}
inline void svmDatabase::setFreePageListCount(svmMemBase * aMemBase,
                                              UInt         aFreePageListCount)
{
    IDE_DASSERT( aMemBase != NULL );
    
    aMemBase->mFreePageListCount = aFreePageListCount;
}

// mFreePageLists
inline svmDBFreePageList svmDatabase::getFreePageList(svmMemBase * aMemBase,
                                                      UInt         aFPListIdx)
{
    IDE_DASSERT( aMemBase != NULL );
    
    return aMemBase->mFreePageLists[aFPListIdx];
}
inline void svmDatabase::setFreePageList(svmMemBase * aMemBase,
                                         UInt         aFreePageListIdx,
                                         scPageID     aFirstFreePageID,
                                         vULong       aFreePageCount)
{
    IDE_DASSERT( aMemBase != NULL );
    
    aMemBase->mFreePageLists[aFreePageListIdx].mFirstFreePageID = aFirstFreePageID;
    aMemBase->mFreePageLists[aFreePageListIdx].mFreePageCount = aFreePageCount;
}

#endif //_O_SVM_DATABSE_H_
