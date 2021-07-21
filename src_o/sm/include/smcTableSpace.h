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
 * $Id: smcTableSpace.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMC_TABLE_SPACE_H_
#define _O_SMC_TABLE_SPACE_H_ 1


#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <smcDef.h>



/*
   Table Space안에 있는 각각의 Table에 대해 수행할 Action함수

   aSlotFlag     - Table Header가 저장된 Catalog Slot의 Drop Flag
   aSlotSCN      - Table Header가 저장된 Catalog Slot의 SCN
   aTableHeader  - Table의 Header
   aActionArg    - Action 함수 Argument

*/
typedef IDE_RC (*smcAction4Table)( idvSQL*          aStatistics,
                                   ULong            aSlotFlag,
                                   smSCN            aSlotSCN,
                                   smcTableHeader * aTableHeader,
                                   void           * aActionArg );
/*
   Tablespace관련 코드중 catalog table에 접근이 필요한 연산을 구현


   [ Offline Tablespace에 안의 Table에 대해 유지하는 정보 ]
     - smcTableHeader
       - mLock
         - Offline/Online Tablespace모두 Table Lock을 획득후
           Tablespace가 사용가능(Online)인지 체크해야 하기 때문
       - mRuntimeInfo
         - QP에서 설정하는 자료
         - Table의 qcmTableInfo나 Stored Procedure의 Plan Tree
         - Tablespace의 Page Memory를 가리키지 않으므로 그대로 둔다.

       - 다음 필드는 NULL로 설정됨 ( 참조되어서는 안됨 )
         - mDropIndexLst, mDropIndex
           - Drop되었지만 dropIndexPnding이 아직 수행되지 않은 Index에 대해
             Catalog Table의 smnIndexHeader를 복사하고 있는 정보

         - mFixed.mMRDB.mRuntimeEntry
           - Tablespace의 Page Memory를 직접 가리키는 자료구조
           - 해당 메모리/객체가 모두 Destroy되고 NULL로 세팅된다.
           - Offline Tablespace에 속한 Table의 경우
             이 Field를 접근해서는 안됨

   [ Online Tablespace에 안의 Table에 대해 유지하는 정보 ]
     - smcTableHeader.mLock
       - mLock
       - mFixed.mMRDB.mRuntimeEntry
       - mRuntimeInfo
       - mDropIndexLst
       - mDropIndex


   [ Alter Tablespace Offline시 Destroy할 필드들 ]
     - mDropIndexLst, mDropIndex
       - Tablespace X락을 잡은 상태라면 이 두 필드는 NULL, 0 이어야 함
         - Drop Index를 수항한 Tx의 Commit/Abort시에
           smxTrans::addListToAger에서
           smcTable::dropIndexList를 호출하여 dropIndexPending 수행하고
           이 두 필드를 초기화 하기 때문

     - mFixed.mMRDB.mRuntimeEntry
       - Alloced Page List, Free Page List와 같이 Tablespace의
         Page Memory를 참조하는 자료구조이다.
       - Offline으로 변하는 Tablespace의 Page Memory가
         모두 Free될 것이므로 이 자료구조의 데이터가 아무 의미가 없다.

   [ Alter Tablespace Online시 초기화할 필드들 ]
     - mFixed.mMRDB.mRuntimeEntry
       - Tablespace를 Disk Checkpoint Image -> Memory로 Restore 한 후
         Tablespace안의 Page Memory접근이 가능해진다.
       - Table의 Page Memory를 바탕으로 Refine작업을 수행하여
         mRuntimeEntry를 새로 구축한다.

 */
class smcTableSpace
{
public :
    // 생성자 (아무것도 안함)
    smcTableSpace();


    // Tablespace안의 각각의 Table에 대해 특정 작업을 수행한다.
    static IDE_RC run4TablesInTBS( idvSQL*           aStatistics,
                                   scSpaceID         aTBSID,
                                   smcAction4Table   aActionFunc,
                                   void            * aActionArg);


    // Online->Offline으로 변하는 Tablespace에 속한 Table들에 대한 처리
    static IDE_RC alterTBSOffline4Tables(idvSQL*      aStatistics,
                                         scSpaceID    aTBSID );


    // Offline->Online으로 변하는 Tablespace에 속한 Table들에 대한 처리
    static IDE_RC alterTBSOnline4Tables(idvSQL     * aStatistics,
                                        void       * aTrans,
                                        scSpaceID    aTBSID );


private:
    // Catalog Table안의 특정 Tablespace에 저장된 Table에 대해
    // Action함수를 수행한다.
    static IDE_RC run4TablesInTBS( idvSQL*           aStatistics,
                                   smcTableHeader  * aCatTableHeader,
                                   scSpaceID         aTBSID,
                                   smcAction4Table   aActionFunc,
                                   void            * aActionArg);


    // Online->Offline으로 변하는 Tablespace에 속한 Table에 대한 Action함수
    static IDE_RC alterTBSOfflineAction( idvSQL*          aStatistics,
                                         ULong            aSlotFlag,
                                         smSCN            aSlotSCN,
                                         smcTableHeader * aTableHeader,
                                         void           * aActionArg );


    // Offline->Online 으로 변하는 Tablespace에 속한 Table에 대한 Action함수
    static IDE_RC alterTBSOnlineAction( idvSQL*          aStatistics,
                                        ULong            aSlotFlag,
                                        smSCN            aSlotSCN,
                                        smcTableHeader * aTableHeader,
                                        void           * aActionArg );


    // Offline->Online 으로 변하는 Disk Tablespace에 속한
    // Table에 대한 Action함수
    static IDE_RC alterDiskTBSOnlineAction( idvSQL*          aStatistics,
                                            ULong            aSlotFlag,
                                            smSCN            aSlotSCN,
                                            smcTableHeader * aTableHeader,
                                            void           * aActionArg );


    // Online->Offline 으로 변하는 Disk Tablespace에 속한
    // Table에 대한 Action함수
    static IDE_RC alterDiskTBSOfflineAction( ULong            aSlotFlag,
                                             smSCN            aSlotSCN,
                                             smcTableHeader * aTableHeader,
                                             void           * aActionArg );


    // Offline->Online 으로 변하는 Memory Tablespace에 속한
    // Table에 대한 Action함수
    static IDE_RC alterMemTBSOnlineAction( ULong            aSlotFlag,
                                           smSCN            aSlotSCN,
                                           smcTableHeader * aTableHeader,
                                           void           * aActionArg );


    // Online->Offline 으로 변하는 Memory Tablespace에 속한
    // Table에 대한 Action함수
    static IDE_RC alterMemTBSOfflineAction( smcTableHeader * aTableHeader );
};

#endif /* _O_SMC_TABLE_SPACE_H_ */
