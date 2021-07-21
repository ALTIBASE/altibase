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
 
#ifndef _O_SVM_TBS_CREATE_H_
#define _O_SVM_TBS_CREATE_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <svmDef.h>
#include <smu.h>


#define SVM_CNO_NONE             (0)
#define SVM_CNO_X_LOCK_TBS_NODE  (0x01)


/*
  [Volatile] Create Tablespace의 구현
  
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
class svmTBSCreate
{
public :
    // 생성자 (아무것도 안함)
    svmTBSCreate();

    // 사용자 테이블 스페이스를 생성한다.
    static IDE_RC createTBS( void                 * aTrans,
                             SChar                * aDBName,
                             SChar                * aTBSName,
                             smiTableSpaceType      aType,
                             UInt                   aAttrFlag,
                             ULong                  aInitSize,
                             idBool                 aIsAutoExtend,
                             ULong                  aNextSize,
                             ULong                  aMaxSize,
                             UInt                   aState,
                             scSpaceID            * aSpaceID );

    // Tablespace를 Create한 Tx가 Commit되었을 때 불리는 Pending함수
    static IDE_RC createTableSpacePending( idvSQL*             aStatistics,
                                           sctTableSpaceNode * aSpaceNode,
                                           sctPendingOp      * /*aPendingOp*/ );
    
private :
    
    // 데이터베이스를 생성한다.
    static IDE_RC createTableSpaceInternal(
                      void                 * aTrans,
                      SChar                * aDBName,
                      smiTableSpaceAttr    * aTBSAttr,
                      scPageID               aCreatePageCount,
                      svmTBSNode          ** aCreatedTBSNode );
    
    
    // TBSNode를 생성하여 sctTableSpaceMgr에 등록한다.
    static IDE_RC createAndLockTBSNode(void              *aTrans,
                                       smiTableSpaceAttr *aTBSAttr);

    // Tablespace Node를 Log Anchor에 flush한다.
    static IDE_RC flushTBSNode(svmTBSNode * aTBSNode);

    //  Tablespace Attribute를 초기화 한다.
    static IDE_RC initializeTBSAttr( smiTableSpaceAttr    * aTBSAttr,
                                     smiTableSpaceType      aType,
                                     SChar                * aName,
                                     UInt                   aAttrFlag,
                                     ULong                  aInitSize,
                                     UInt                   aState);
};

#endif /* _O_SVM_TBS_CREATE_H_ */
