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
 * $Id: smmTBSDrop.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMM_TBS_DROP_H_
#define _O_SMM_TBS_DROP_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>
#include <smu.h>

/*
  [참고] SMM안에서의 File간의 Layer및 역할은 다음과 같다.
         하위 Layer의 코드에서는 상위 Layer의 코드를 사용할 수 없다.

  ----------------------------------------------------------------------------
  smmTBSCreate          ; Create Tablespace 구현
  smmTBSDrop            ; Drop Tablespace 구현
  smmTBSAlterAutoExtend ; Alter Tablespace Auto Extend 구현
  smmTBSAlterChkptPath  ; Alter Tablespace Add/Rename/Drop Checkpoint Path구현
  smmTBSAlterDiscard    ; Alter Tablespace Discard 구현
  smmTBSStartupShutdown ; Startup, Shutdown시의 Tablespace관련 처리를 구현
  ----------------------------------------------------------------------------
  smmTBSChkptPath  ; Tablespace의 Checkpoint Path 관리
  smmTBSMultiPhase ; Tablespace의 다단계 초기화
  ----------------------------------------------------------------------------
  smmManager       ; Tablespace의 내부 구현 
  smmFPLManager    ; Tablespace Free Page List의 내부 구현
  smmExpandChunk   ; Chunk의 내부구조 구현
  ----------------------------------------------------------------------------
  
  c.f> Memory Tablespace의 Alter Online/Offline은 smp layer에 구현되어 있다.
*/


/*
  Drop Memory Tablespace 구현
 */
class smmTBSDrop
{
public :
    // 생성자 (아무것도 안함)
    smmTBSDrop();

         
    // 사용자 테이블 스페이스를 DROP한다.
    static IDE_RC dropTBS(void            * aTrans,
                          smmTBSNode      * aTBSNode,
                          smiTouchMode      aTouchMode);

public :
    // Tablespace를 DROP한 Tx가 Commit되었을 때 불리는 Pending함수
    static IDE_RC dropTableSpacePending( idvSQL            * aStatistics, 
                                         sctTableSpaceNode * aTBSNode,
                                         sctPendingOp      * aPendingOp );
    // 실제 Drop Tablespace수행 코드 
    static IDE_RC doDropTableSpace( sctTableSpaceNode * aTBSNode,
                                    smiTouchMode        aTouchMode );

    // Tablespace의 상태를 DROPPED로 설정하고 Log Anchor에 Flush!
    static IDE_RC flushTBSStatusDropped( sctTableSpaceNode * aTBSNode );
    
private :

    //Tablespace의 상태를 변경하고 Log Anchor에 Flush한다.
    static IDE_RC flushTBSStatus( sctTableSpaceNode * aSpaceNode,
                                  UInt                aNewSpaceState);
    
    // 메모리 테이블 스페이스를 DROP한다.
    static IDE_RC dropTableSpace(void       * aTrans,
                                 smmTBSNode * aTBSNode,
                                 smiTouchMode   aTouchMode);

};

#endif /* _O_SMM_TBS_DROP_H_ */
