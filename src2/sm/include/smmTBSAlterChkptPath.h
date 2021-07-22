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
 * $Id: smmTBSAlterChkptPath.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMM_TBS_ALTER_CHKPT_PATH_H_
#define _O_SMM_TBS_ALTER_CHKPT_PATH_H_ 1

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
  Memory Tablespace의
  Alter Tablespace Add/Rename/Drop Checkpoint Path구현
 */
class smmTBSAlterChkptPath
{
public :
    // 생성자 (아무것도 안함)
    smmTBSAlterChkptPath();

    // 새로운 Checkpoint Path를 추가한다.
    static IDE_RC alterTBSaddChkptPath( scSpaceID    aSpaceID,
                                        SChar      * aChkptPath );

    // 기존의 Checkpoint Path를 변경한다.
    static IDE_RC alterTBSrenameChkptPath( scSpaceID    aSpaceID,
                                           SChar      * aOrgChkptPath,
                                           SChar      * aNewChkptPath );
    
    // 기존의 Checkpoint Path를 제거한다.
    static IDE_RC alterTBSdropChkptPath( scSpaceID   aSpaceID,
                                         SChar     * aChkptPath );

};

#endif /* _O_SMM_TBS_ALTER_CHKPT_PATH_H_ */
