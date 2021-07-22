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
 * $Id: smmTableSpace.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMM_TBS_STARTUP_SHUTDOWN_H_
#define _O_SMM_TBS_STARTUP_SHUTDOWN_H_ 1

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
  Memory Tablespace와 관련하여 Startup, Shutdown시의 처리사항을 구현한다.
 */
class smmTBSStartupShutdown
{
public :
    // 생성자 (아무것도 안함)
    smmTBSStartupShutdown();

    // Memory Tablespace 관리자의 초기화
    static IDE_RC initializeStatic();

    // Memory Tablespace관리자의 해제
    static IDE_RC destroyStatic();
    ////////////////////////////////////////////////////////////////////
    // 인터페이스 함수 ( SM의 다른 모듈에서 호출 )
    ////////////////////////////////////////////////////////////////////
    // 모든 Tablespace를 destroy한다. ( Shutdown시 호출 )
    static IDE_RC destroyAllTBSNode();
    

    ////////////////////////////////////////////////////////////////////
    // Server Startup시 Tablespace초기화 관련 함수
    ////////////////////////////////////////////////////////////////////
    // Server startup시 Log Anchor의 Tablespace Attribute를 바탕으로
    // Tablespace Node를 구축한다.
    static IDE_RC loadTableSpaceNode(
                      smiTableSpaceAttr   * aTBSAttr,
                      UInt                  aAnchorOffset );


    // Checkpoint Image Attribute를 초기화한다.
    static IDE_RC initializeChkptImageAttr(
                      smmChkptImageAttr * aChkptImageAttr,
                      smLSN             * aMemRedoLSN,
                      UInt                aAnchorOffset );


    // Loganchor로부터 읽어들인 Checkpoint Path Attribute로 Node를 생성한다.
    static IDE_RC createChkptPathNode( smiChkptPathAttr  * aChkptPathAttr,
                                       UInt                aAnchorOffset );


    // 모든 Tablespace의 다단계 초기화를 수행한다.
    static IDE_RC initFromStatePhase4AllTBS();
    
private :
    // smmTBSStartupShutdown::initFromStatePhase4AllTBS 의 액션 함수
    static IDE_RC initFromStatePhaseAction( idvSQL            * aStatistics, 
                                            sctTableSpaceNode * aTBSNode,
                                            void * /* aActionArg */ );
    
};

#endif /* _O_SMM_TBS_STARTUP_SHUTDOWN_H_ */
