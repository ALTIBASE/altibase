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
 * $Id: smrDPListMgr.h 19996 2007-01-18 13:00:36Z bskim $
 **********************************************************************/

#ifndef _O_SMR_DPLIST_MGR_
#define _O_SMR_DPLIST_MGR_ 1

#include <iduHash.h>
#include <smm.h>
#include <smrDef.h>

/*
    Dirty Page기록 옵션
    - writeDirtyPages4AllTBS 에 옵션인자로 넘긴다.
 */
typedef enum smrWriteDPOption
{
    SMR_WDP_NONE = 0,
    SMR_WDP_NO_PID_LOGGING = 1, // Media Recovery 완료후에는 PID로깅 안함
    SMR_WDP_FINAL_WRITE    = 2  // Shutdown 이전의 최종 Flush
} smrWriteDPOption;


// 여러 Tablespace의 Dirty Page관리자 객체들(smrDirtyPageList)을
// 관리하는 Class.

class smrDPListMgr
{
private:
    // Key:scSpaceID => data:smrDirtyPageList
    static iduHash   mMgrHash;
    
public :

    // Tablespace별 Dirty Page관리자를 관리하는 smrDPListMgr을 초기화
    static IDE_RC initializeStatic();

    // Tablespace별 Dirty Page관리자를 관리하는 smrDPListMgr을 파괴
    static IDE_RC destroyStatic();

    // 특정 Tablespace의 Dirty Page관리자에 Dirty Page를 추가한다.
    static IDE_RC add(scSpaceID aSpaceID,
                      smmPCH*   aPCHPtr,
                      scPageID  aPageID);

    // 모든 Tablespace의 Dirty Page수를 리턴 
    static IDE_RC getTotalDirtyPageCnt( ULong * aDirtyPageCount);
    
    // 특정 Tablespace의 Dirty Page수를 계산한다.
    static IDE_RC getDirtyPageCountOfTBS( scSpaceID   aTBSID,
                                          UInt      * aDirtyPageCount );


    // 모든 Tablespace의 Dirty Page를 Checkpoint Image에 기록 
    static IDE_RC writeDirtyPages4AllTBS(
                       sctStateSet                  aStateSet,
                       ULong                      * aTotalCnt,
                       ULong                      * aRemoveCnt,
                       ULong                      * aWaitTime,
                       ULong                      * aSyncTime,
                       smmGetFlushTargetDBNoFunc    aGetFlushTargetDBNoFunc,
                       smrWriteDPOption             aOption );
    
    // 모든 Tablespace의 Dirty Page를 SMM=>SMR로 이동한다.
    static IDE_RC moveDirtyPages4AllTBS ( 
                           sctStateSet  aStateSet,
                           ULong *      aNewCnt,
                           ULong *      aDupCnt );

    // 모든 Tablespace의 Dirty Page를 Discard 시킨다. 
    static IDE_RC discardDirtyPages4AllTBS();
    
private :
    // 특정 Tablespace를 위한 Dirty Page관리자를 생성한다.
    static IDE_RC createDPList(scSpaceID aSpaceID );

    // 특정 Tablespace의 Dirty Page관리자를 제거한다.
    static IDE_RC removeDPList( scSpaceID aSpaceID );


    // 특정 Tablespace를 위한 Dirty Page관리자를 찾아낸다.
    // 찾지 못한 경우 NULL이 리턴된다.
    static IDE_RC findDPList( scSpaceID           aSpaceID,
                              smrDirtyPageList ** aDPList );

    // smrDirtyPageList::writeDirtyPage를 수행하는 Action함수
    static IDE_RC writeDirtyPageAction( idvSQL            * aStatistics,
                                        sctTableSpaceNode * aTBSNode,
                                        void * aActionArg );

    // smrDirtyPageList::writePIDLogs를 수행하는 Action함수
    static IDE_RC writePIDLogAction( idvSQL            * aStatistics,
                                     sctTableSpaceNode * aTBSNode,
                                     void              * aActionArg );
    
    // 각각의 Hash Element에 대해 호출될 Visitor함수
    static IDE_RC destoyingVisitor( vULong   aKey,
                                    void   * aData,
                                    void   * aVisitorArg);

    //  Hashtable에 등록된 모든 smrDirtyPageList를 destroy한다.
    static IDE_RC destroyAllMgrs();

    // 특정 Tablespace의 Dirty Page를 SMM=>SMR로 이동한다.
    static IDE_RC moveDirtyPages4TBS( scSpaceID   aSpaceID,
                                      UInt      * aNewCnt,
                                      UInt      * aDupCnt );

    //  moveDirtyPages4AllTBS 를 위한 Action함수 
    static IDE_RC moveDPAction( idvSQL*             aStatistics,
                                sctTableSpaceNode * aTBSNode,
                                void * aActionArg );

    // getTotalDirtyPageCnt 를 위한 Action함수 
    static IDE_RC countDPAction( idvSQL*             aStatistics,
                                 sctTableSpaceNode * aSpaceNode,
                                 void * aActionArg );

    // 특정 Tablespace의 SMR에 있는 Dirty Page를 제거한다. 
    static IDE_RC discardDirtyPages4TBS( scSpaceID aSpaceID );

    //  discardDirtyPages4AllTBS 를 위한 Action함수 
    static IDE_RC discardDPAction( idvSQL            * aStatistics,
                                   sctTableSpaceNode * aTBSNode,
                                   void * aActionArg );
};

#endif // _O_SMR_DPLIST_MGR_
