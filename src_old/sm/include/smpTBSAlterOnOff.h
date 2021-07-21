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
 * $Id: smpTBSAlterOnOff.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMP_TBS_ALTER_ONOFF_H_
#define _O_SMP_TBS_ALTER_ONOFF_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>

/*
   Memory DB의 Tablespace관련 연산들을 구현한 class
   
   - Alter Tablespace Offline
   - Alter Tablespace Online
 */
class smpTBSAlterOnOff
{
public :
    // 생성자 (아무것도 안함)
    smpTBSAlterOnOff();

    ////////////////////////////////////////////////////////////////////
    // 인터페이스 함수 ( smiTableSpace에서 바로 호출 )
    ////////////////////////////////////////////////////////////////////

    //Memory Tablespace에 대해 Alter Tablespace Online/Offline을 수행
    static IDE_RC alterTBSStatus( void       * aTrans,
                                  smmTBSNode * aTBSNode,
                                  UInt         aState );

    ////////////////////////////////////////////////////////////////////
    // Restart Recovery시 호출되는 함수
    ////////////////////////////////////////////////////////////////////
    // Restart REDO, UNDO끝난 후에 Offline 상태인
    // Tablespace의 메모리를 해제
    static IDE_RC finiOfflineTBS();


    ////////////////////////////////////////////////////////////////////
    // Alter Tablespace Online/Offline 관련함수
    ////////////////////////////////////////////////////////////////////
    // Tablespace를 OFFLINE시킨 Tx가 Commit되었을 때 불리는 Pending함수
    static IDE_RC alterOfflineCommitPending(
                      idvSQL*             aStatistics, 
                      sctTableSpaceNode * aTBSNode,
                      sctPendingOp      * aPendingOp );

    // Tablespace를 ONLINE시킨 Tx가 Commit되었을 때 불리는 Pending함수
    static IDE_RC alterOnlineCommitPending(
                      idvSQL*             aStatistics, 
                      sctTableSpaceNode * aTBSNode,
                      sctPendingOp      * aPendingOp );

    
    
private:
    ////////////////////////////////////////////////////////////////////
    // Alter Tablespace Online/Offline 관련함수
    ////////////////////////////////////////////////////////////////////
    
    // META, SERVICE단계에서 Tablespace를 Offline상태로 변경한다.
    static IDE_RC alterTBSoffline(void       * aTrans,
                                  smmTBSNode * aTBSNode );


    // META/SERVICE단계에서 Tablespace를 Online상태로 변경한다.
    static IDE_RC alterTBSonline(void       * aTrans,
                                 smmTBSNode * aTBSNode );


    ////////////////////////////////////////////////////////////////////
    // Restart Recovery시 호출되는 함수
    ////////////////////////////////////////////////////////////////////
    // finiOfflineTBS를 위한 Action함수
    static IDE_RC finiOfflineTBSAction( idvSQL            * aStatistics,
                                        sctTableSpaceNode * aTBSNode,
                                        void * /* aActionArg */ );
    
    
    ////////////////////////////////////////////////////////////////////
    // Alter Tablespace Online/Offline 이 공통적으로 사용하는 함수
    ////////////////////////////////////////////////////////////////////

    // Alter Tablespace Online Offline 로그를 기록 
    static IDE_RC writeAlterTBSStateLog(
                      void                 * aTrans,
                      smmTBSNode           * aTBSNode,
                      sctUpdateType          aUpdateType,
                      smiTableSpaceState     aStateToRemove,
                      smiTableSpaceState     aStateToAdd,
                      UInt                 * aNewTBSState );
};

#endif /* _O_SMP_TBS_ALTER_ONOFF_H_ */
