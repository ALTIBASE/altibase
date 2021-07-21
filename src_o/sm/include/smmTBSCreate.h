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
 * $Id: smmTBSCreate.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMM_TBS_CREATE_H_
#define _O_SMM_TBS_CREATE_H_ 1

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
   Create Memory Tablespace 구현
 */
class smmTBSCreate
{
public :
    // 생성자 (아무것도 안함)
    smmTBSCreate();

    ////////////////////////////////////////////////////////////////////
    // 인터페이스 함수 ( smiTableSpace에서 바로 호출 )
    ////////////////////////////////////////////////////////////////////
    // PROJ-1923 ALTIBASE HDB Disaster Recovery
    // 사용자 테이블 스페이스를 생성한다.
    static IDE_RC createTBS4Redo( void                 * aTrans,
                                  smiTableSpaceAttr    * aTBSAttr,
                                  smiChkptPathAttrList * aChkptPathList );
 
    // 사용자 테이블 스페이스를 생성한다.
    static IDE_RC createTBS( void                 * aTrans,
                             SChar                * aDBName,
                             SChar                * aTBSName,
                             UInt                   aAttrFlag,
                             smiTableSpaceType      aType,
                             smiChkptPathAttrList * aChkptPathList,
                             ULong                  aSplitFileSize,
                             ULong                  aInitSize,
                             idBool                 aIsAutoExtend,
                             ULong                  aNextSize,
                             ULong                  aMaxSize,
                             idBool                 aIsOnline,
                             SChar                * aDBCharSet,
                             SChar                * aNationalCharSet,
                             scSpaceID            * aTBSID );
    
public :
    // Tablespace를 Create한 Tx가 Commit되었을 때 불리는 Pending함수
    static IDE_RC createTableSpacePending( idvSQL            * aStatistics, 
                                           sctTableSpaceNode * aTBSNode,
                                           sctPendingOp      * aPendingOp );
    
private :
    // 사용자가 Tablespace생성을 위한 옵션을 지정하지 않은 경우
    // 기본값을 설정하는 함수
    static IDE_RC makeDefaultArguments( ULong  * aSplitFileSize,
                                        ULong  * aInitSize);
    
    //  Tablespace Attribute를 초기화 한다.
    static IDE_RC initializeTBSAttr( smiTableSpaceAttr    * aTBSAttr,
                                     scSpaceID              aSpaceID,
                                     smiTableSpaceType      aType,
                                     SChar                * aName,
                                     UInt                   aAttrFlag,
                                     ULong                  aSplitFileSize,
                                     ULong                  aInitSize);
    

    // Tablespace Attribute에 대한 에러체크를 실시한다.
    static IDE_RC checkErrorOfTBSAttr( SChar * aTBSName, 
                                       ULong   aSplitFileSize,
                                       ULong   aInitSize);

    // 사용자가 지정한, 혹은 시스템의 기본 Checkpoint Path를
    // Tablespace에 추가한다.
    static IDE_RC createDefaultOrUserChkptPaths(
                      smmTBSNode           * aTBSNode,
                      smiChkptPathAttrList * aChkptPathAttrList );
    

    // 하나 혹은 그 이상의 Checkpoint Path를 TBSNode의 Tail에 추가한다.
    static IDE_RC createChkptPathNodes( smmTBSNode           * aTBSNode,
                                        smiChkptPathAttrList * aChkptPathList );
    
    
    // 새로운  Tablespace가 사용할 Tablespace ID를 할당받는다.
    static IDE_RC allocNewTBSID( scSpaceID * aSpaceID );

    // Tablespace를 생성하고 생성이 완료되면 NTA로 묶는다.
    static IDE_RC createTBSWithNTA4Redo(
                    void                  * aTrans,
                    smiTableSpaceAttr     * aTBSAttr,
                    smiChkptPathAttrList  * aChkptPathList );
    
    // Tablespace를 생성하고 생성이 완료되면 NTA로 묶는다.
    static IDE_RC createTBSWithNTA(
                      void                  * aTrans,
                      SChar                 * aDBName,
                      smiTableSpaceAttr     * aTBSAttr,
                      smiChkptPathAttrList  * aChkptPathList,
                      scPageID                aInitPageCount,
                      SChar                 * aDBCharSet,
                      SChar                 * aNationalCharSet,
                      smmTBSNode           ** aCreatedTBSNode);

    // Tablespace를 시스템에 등록하고 내부 자료구조를 생성한다.
    static IDE_RC createTBSInternal4Redo(
                      void                 * aTrans,
                      smmTBSNode           * aTBSNode,
                      smiChkptPathAttrList * aChkptPathAttrList );
   
    // Tablespace를 시스템에 등록하고 내부 자료구조를 생성한다.
    static IDE_RC createTBSInternal(
                      void                 * aTrans,
                      smmTBSNode           * aTBSNode,
                      SChar                * aDBName,
                      smiChkptPathAttrList * aChkptPathAttrList,
                      scPageID               aInitPageCount,
                      SChar                * aDBCharSet,
                      SChar                * aNationalCharSet);

    // smmTBSNode를 생성하고 X Lock을 획득한 후
    // sctTableSpaceMgr에 등록한다.
    static IDE_RC allocAndInitTBSNode(
                      void                * aTrans,
                      smiTableSpaceAttr   * aTBSAttr,
                      smmTBSNode         ** aCreatedTBSNode );

    // Tablespace Node를 다단계 해제수행후 free한다.
    static IDE_RC finiAndFreeTBSNode( smmTBSNode * aTBSNode );
    
    // 다른 Transaction들이 Tablespace이름으로 Tablespace를 찾을 수 있도록
    // 시스템에 등록한다.
    static IDE_RC registerTBS( smmTBSNode * aTBSNode );
    

    // Tablespace Node와 그에 속한 Checkpoint Path Node들을 모두
    // Log Anchor에 Flush한다.
    static IDE_RC flushTBSAndCPaths(smmTBSNode * aTBSNode);
    

};

#endif /* _O_SMM_TBS_CREATE_H_ */
 
