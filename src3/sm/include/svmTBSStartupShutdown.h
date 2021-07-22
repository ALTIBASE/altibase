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
 
#ifndef _O_SVM_TBS_STARTUP_SHUTDOWN_H_
#define _O_SVM_TBS_STARTUP_SHUTDOWN_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <svmDef.h>
#include <smu.h>

/*

  [Volatile]  Startup, Shutdown시의 Tablespace관련 처리를 구현
  
  참고> svm 모듈 안의 소스는 다음과 같이 Layering되어 있다.
  ----------------------------------------------------------------------------
  svmTBSCreate          ; Create Tablespace 구현
  svmTBSDrop            ; Drop Tablespace 구현
  svmTBSAlterAutoExtend ; Alter Tablespace Auto Extend 구현
  svmTBSStartupShutdown ; Startup, Shutdown시의 Tablespace관련 처리를 구현
  ----------------------------------------------------------------------------
  svmManager       ; Tablespace의 내부 구현 
  svmFPLManager    ; Tablespace Free Page List의 내부 구현
  svmExpandChunk   ; Chunk의 내부구조 구현
  ----------------------------------------------------------------------------
 */
class svmTBSStartupShutdown
{
public :
    // 생성자 (아무것도 안함)
    svmTBSStartupShutdown();

    // Volatile Tablespace 관리자의 초기화
    static IDE_RC initializeStatic();
    
    // Volatile Tablespace관리자의 해제
    static IDE_RC destroyStatic();
    

    // Log anchor가 읽은 TBSAttr로부터 TBSNode를 생성한다.
    static IDE_RC loadTableSpaceNode(smiTableSpaceAttr *aTBSAttr,
                                     UInt               aAnchorOffset);

    // 모든 Volatile TBS를 초기화한다.
    // TBSNode들은 이미 smrLogAchorMgr에서 초기화된 상태이어야 한다.
    static IDE_RC prepareAllTBS();

    // 모든 Volatile TBSNode들을 해제한다.
    static IDE_RC destroyAllTBSNode();

private :

    // 하나의 Volatile TBS를 초기화한다.
    static IDE_RC prepareTBSAction(idvSQL*            aStatistics,
                                   sctTableSpaceNode *aTBSNode,
                                   void              */* aArg */);
};

#endif /* _O_SVM_TBS_STARTUP_SHUTDOWN_H_ */
