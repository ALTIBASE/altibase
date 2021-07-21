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
 * $Id: smmTBSPhase.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMM_TBS_MULTI_PHASE_H_
#define _O_SMM_TBS_MULTI_PHASE_H_ 1

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
    smmTBSMultiPhase ; Tablespace의 다단계 초기화 구현

    ------------------------------------------------------------------------
    Tablespace의 다단계 초기화/파괴
    ------------------------------------------------------------------------   
    
    Tablespace는 상태에 따라 다음과 같이 다단계로
    초기화를 진행한다.

    상위 단계로의 초기화를 위해서는 하위 단계의 초기화가 필수적이다.
    (ex> 2. MEDIA단계로 초기화를 위해서는 1.STATE단계의 초기화가 필수)
    
    1. STATE  - State Phase
       가능한 동작 :
         - Tablespace의 State에 접근 가능
         - 동시성 제어를 위한 Lock과 Mutex를 사용가능
         
       초기화 :
         - Tablespace의 Attribute초기화
         - Tablespace의 Lock, Mutex초기화
       
    2. MEDIA  - Media Phase
       가능한 동작 :
         - Tablespace의 DB File 파일에 읽기/쓰기 가능
         
       초기화 :
         - DB File 객체 초기화, Open
    
    3. PAGE   - Page Phase
       가능한 동작 :
         - Prepare/Restore (DB File => Page Memory로의 데이터 로드 )
         - Tablespace의 Page 접근/할당/반납 가능
         
       초기화 :
         - PCH* Array할당
         - Page Memory Pool할당
         - Free Page List, Chunk 관리자 초기화

    ------------------------------------------------------------------------
    Tablespace State별 초기화 단계
    ------------------------------------------------------------------------   
    A. ONLINE
       초기화 단계 : PAGE
       이유 : Page의 할당및 반납, Page Data의 접근이 가능해야함
       
    B. OFFLINE
       초기화 단계 : MEDIA
       이유 : Checkpoint마다 DB File의 Header에 RedoLSN을 갱신해야함
       
    C. DISCARDED
       초기화 단계 : MEDIA
       이유 : 여러 Transaction이 동시에 DISCARD된 Tablespace에 접근가능
               Discard된 Tablespace에 Drop Tablespace를 제외한
               모든 DML/DDL의 수행이 거절되므로,
               Tablespace의 상태만 조회할 수 있으면 된다.
               
               Drop Tablespace가 가능하기 때문에, DB File객체에 접근이
               가능해야 하며, 이를 위해 MEDIA단계까지 초기화가
               되어있어야 한다.
       
    D. DROPPED
       초기화 단계 : STATE
       이유 : 여러 Transaction이 동시에 DROP된 Tablespace에 접근가능
               Drop된 Tablespace에 모든 DML/DDL의 수행이 거절되므로,
               Tablespace의 상태만 조회할 수 있으면 된다.
    

    ------------------------------------------------------------------------   
    Meta/Service상태에서의 Tablespace Node의 상태전이
    ------------------------------------------------------------------------   
     - ONLINE => OFFLINE
        [ PAGE => MEDIA ] finiPAGE
       
     - OFFLINE => ONLINE
        [ MEDIA => PAGE ] initPAGE
       
     - ONLINE => DROPPED
        [ PAGE => STATE ] finiPAGE, finiMEDIA

     - OFFLINE => DROPPED
        [ STATE => PAGE ] initMEDIA, initPAGE
    ------------------------------------------------------------------------   
    Control상태에서의 Tablespace Node의 상태전이
     - ONLINE => DISCARDED
        [ PAGE => STATE ] finiPAGE, finiMEDIA
       
     - OFFLINE => DISCARDED
        [ STATE => PAGE ] initMEDIA, initPAGE
               
 */



class smmTBSMultiPhase
{
public :
    // 생성자 (아무것도 안함)
    smmTBSMultiPhase();

    
    // STATE Phase 로부터 Tablespace의 상태에 따라
    // 필요한 단게까지 다단계 초기화
    static IDE_RC initTBS( smmTBSNode * aTBSNode,
                           smiTableSpaceAttr * aTBSAttr );
    
    // Server Shutdown시에 Tablespace의 상태에 따라
    // Tablespace의 초기화된 단계들을 파괴한다.
    static IDE_RC finiTBS( smmTBSNode * aTBSNode );
    
    // PAGE, MEDIA Phase로부터 STATE Phase 로 전이
    static IDE_RC finiToStatePhase( smmTBSNode * aTBSNode );

    // 다단계 해제를 통해 MEDIA단계로 전이
    static IDE_RC finiToMediaPhase( UInt         aTBSState,
                                    smmTBSNode * aTBSNode );
    
    // Tablespace의 STATE단계를 초기화
    static IDE_RC initStatePhase( smmTBSNode * aTBSNode,
                                  smiTableSpaceAttr * aTBSAttr );

    // STATE단계까지 초기화된 Tablespace의 다단계초기화
    static IDE_RC initFromStatePhase( smmTBSNode * aTBSNode );
    
    // Tablespace의 MEDIA단계를 초기화
    static IDE_RC initMediaPhase( smmTBSNode * aTBSNode );

    // Tablespace의 PAGE단계를 초기화
    static IDE_RC initPagePhase( smmTBSNode * aTBSNode );

    // Tablespace의 STATE단계를 파괴
    static IDE_RC finiStatePhase( smmTBSNode * aTBSNode );

    // Tablespace의 MEDIA단계를 파괴
    static IDE_RC finiMediaPhase( smmTBSNode * aTBSNode );

    // Tablespace의 PAGE단계를 파괴
    static IDE_RC finiPagePhase( smmTBSNode * aTBSNode );
};

#endif /* _O_SMM_TBS_MULTI_PHASE_H_ */
