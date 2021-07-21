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
 

/***********************************************************************
 * $Id: smxTableInfoMgr.h 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

#ifndef _O_SMX_TABLEINFO_H_
#define _O_SMX_TABLEINFO_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduHash.h>
#include <smDef.h>
#include <smxDef.h>

class smxTrans;

class smxTableInfoMgr
{
public:
    IDE_RC          initialize();
    IDE_RC          destroy();

    IDE_RC          init();
    IDE_RC          getTableInfo(smOID          aTableOID,
                                 smxTableInfo **aTableInfoPtr,
                                 idBool         aIsSearch = ID_FALSE);

    /* PROJ-2462 Result Cache */
    void            addTablesModifyCount( void );

    /*
     * + Page List Entry의 record cnt를 갱신하는 절차 by PRJ-1496
     *
     * 1) 모든 table에 entry mutex를 잡으면서 max rows를 검사한다.
     *    -> requestAllEntryForCheckMaxRow
     *
     * 2) 메모리 테이블의 증가된 row 개수를 갱신하고 entry mutex를 release한다.
     *    -> releaseEntryAndUpdateMemTableInfoForIns
     *
     * 3) 디스크 테이블의 증감된 row 개수를 갱신과 함께 commit하고
     *    entry mutex를 release한다.
     *    -> releaseEntryAndUpdateDiskTableInfoWithCommit
     *
     * 4) 메모리 테이블의 감소된 row 개수를 갱신한다.
     *    -> updateMemTableInfoForDel
     */

    IDE_RC        requestAllEntryForCheckMaxRow();
    IDE_RC        releaseEntryAndUpdateMemTableInfoForIns();
    void          updateMemTableInfoForDel();

    IDE_RC        releaseEntryAndUpdateDiskTableInfoWithCommit(
                                             idvSQL   * aStatistics,
                                             smxTrans * aTransPtr,
                                             smSCN      aCommitSCN,
                                             smLSN    * aEndLSN );

    IDE_RC        processAtDPathInsCommit();

    // for layercallback function
    static void   incRecCntOfTableInfo(void  *aTableInfoPtr);
    static void   decRecCntOfTableInfo(void  *aTableInfoPtr);
    static SLong  getRecCntOfTableInfo(void  *aTableInfoPtr);

    static inline idBool isExistDPathIns( void     * aTableInfoPtr );
    static void   setExistDPathIns( void    * aTableInfoPtr,
                                    idBool    aExistDPathIns );

    static inline void getHintDataPID( smxTableInfo * aTableInfoPtr,
                                       scPageID     * aHintDataPID );
    static inline void setHintDataPID( smxTableInfo * aTableInfoPtr,
                                       scPageID       aHintDataPID );

private:
    IDE_RC          allocTableInfo(smxTableInfo **aTableInfoPtr);
    IDE_RC          freeTableInfo(smxTableInfo   *aTableInfoPtr);

    inline void     initTableInfo(smxTableInfo   *aTableInfoPtr);
    inline void     addTableInfoToFreeList(smxTableInfo      *aTableInfoPtr);
    inline void     removeTableInfoFromFreeList(smxTableInfo *aTableInfoPtr);

    inline void     addTableInfoToList(smxTableInfo      *aTableInfoPtr);
    inline void     removeTableInfoFromList(smxTableInfo *aTableInfoPtr);

public: /* POD class type should make non-static data members as public */
    iduHash         mHash;
    smxTableInfo    mTableInfoFreeList;
    smxTableInfo    mTableInfoList; //Table OID
    UInt            mCacheCount;
    UInt            mFreeTableInfoCount;
};


void smxTableInfoMgr::initTableInfo(smxTableInfo *aTableInfoPtr)
{
    aTableInfoPtr->mPrevPtr             = aTableInfoPtr;
    aTableInfoPtr->mNextPtr             = aTableInfoPtr;
    aTableInfoPtr->mRecordCnt           = 0;
    aTableInfoPtr->mHintDataPID         = SD_NULL_PID;
    aTableInfoPtr->mExistDPathIns       = ID_FALSE;
}

inline void smxTableInfoMgr::addTableInfoToFreeList(smxTableInfo *aTableInfoPtr)
{
    aTableInfoPtr->mPrevPtr = &mTableInfoFreeList;
    aTableInfoPtr->mNextPtr = mTableInfoFreeList.mNextPtr;

    mTableInfoFreeList.mNextPtr->mPrevPtr = aTableInfoPtr;
    mTableInfoFreeList.mNextPtr           = aTableInfoPtr;
}

inline void smxTableInfoMgr::removeTableInfoFromFreeList(smxTableInfo *aTableInfoPtr)
{
    aTableInfoPtr->mPrevPtr->mNextPtr = aTableInfoPtr->mNextPtr;
    aTableInfoPtr->mNextPtr->mPrevPtr = aTableInfoPtr->mPrevPtr;
    aTableInfoPtr->mNextPtr = aTableInfoPtr;
    aTableInfoPtr->mPrevPtr = aTableInfoPtr;
}

inline void smxTableInfoMgr::addTableInfoToList(smxTableInfo *aTableInfoPtr)
{
    smxTableInfo *sCurTableInfo;
    smxTableInfo *sPrvTableInfo;

    sPrvTableInfo = &mTableInfoList;
    sCurTableInfo = mTableInfoList.mNextPtr;

    while((sCurTableInfo != &mTableInfoList)
          && (sCurTableInfo->mTableOID < aTableInfoPtr->mTableOID))
    {
        sPrvTableInfo = sCurTableInfo;
        sCurTableInfo = sCurTableInfo->mNextPtr;
    }

    sCurTableInfo = sPrvTableInfo;

    aTableInfoPtr->mNextPtr = sCurTableInfo->mNextPtr;
    aTableInfoPtr->mPrevPtr = sCurTableInfo;

    sCurTableInfo->mNextPtr->mPrevPtr = aTableInfoPtr;
    sCurTableInfo->mNextPtr = aTableInfoPtr;
}

inline void smxTableInfoMgr::removeTableInfoFromList(smxTableInfo *aTableInfoPtr)
{
    aTableInfoPtr->mPrevPtr->mNextPtr = aTableInfoPtr->mNextPtr;
    aTableInfoPtr->mNextPtr->mPrevPtr = aTableInfoPtr->mPrevPtr;
    aTableInfoPtr->mNextPtr = aTableInfoPtr;
    aTableInfoPtr->mPrevPtr = aTableInfoPtr;
}

inline void smxTableInfoMgr::getHintDataPID( smxTableInfo * aTableInfoPtr,
                                             scPageID     * aHintDataPID )
{
    IDE_ASSERT( aTableInfoPtr != NULL );
    *aHintDataPID = aTableInfoPtr->mHintDataPID;
    return;
}

inline void smxTableInfoMgr::setHintDataPID( smxTableInfo * aTableInfoPtr,
                                             scPageID       aHintDataPID )
{
    IDE_ASSERT( aTableInfoPtr != NULL );
    aTableInfoPtr->mHintDataPID = aHintDataPID;
    return;
}

/***********************************************************************
 * Description : smxTableInfo 객체의 ExistDPathIns를 반환한다.
 ***********************************************************************/
inline idBool smxTableInfoMgr::isExistDPathIns( void * aTableInfoPtr )
{
    IDE_DASSERT( aTableInfoPtr != NULL );
    return ( ((smxTableInfo *)aTableInfoPtr)->mExistDPathIns );
}

#endif  // _O_SMX_TABLEINFO_H_
