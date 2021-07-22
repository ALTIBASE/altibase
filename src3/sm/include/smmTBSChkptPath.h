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

#ifndef _O_SMM_TBS_CHKPT_PATH_H_
#define _O_SMM_TBS_CHKPT_PATH_H_ 1

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
   Memory Tablespace의 Checkpoint Path의 추가,변경,삭제를 구현한다.

   smmTBSAlterChkptPath에서 이 Class를 사용하여 Alter Checkpoint Path기능을 구현한다.
   
 */
class smmTBSChkptPath
{
public :
    // 생성자 (아무것도 안함)
    smmTBSChkptPath();
    
    // Loganchor로부터 읽어들인 Checkpoint Path Attribute로 Node를 생성한다.
    static IDE_RC createChkptPathNode( smiChkptPathAttr  * aChkptPathAttr,
                                       UInt                aAnchorOffset );

    // Checkpoint Path Attribute에 Checkpoint Path를 설정한다.
    static IDE_RC setChkptPath( smiChkptPathAttr * aCPathAttr,
                                SChar            * aChkptPath );
    

    // Checkpoint Path Node의 수를 리턴한다.
    static IDE_RC getChkptPathNodeCount( smmTBSNode * aTBSNode,
                                         UInt       * aChkptPathCount );

    // N번째 Checkpoint Path Node를 리턴한다.
    static IDE_RC getChkptPathNodeByIndex(
                      smmTBSNode        * aTBSNode,
                      UInt                aIndex,
                      smmChkptPathNode ** aCPathNode );


    // Checkpoint Path Node를 특정 Tablespace에 추가한다. 
    static IDE_RC addChkptPathNode(  smmTBSNode      * aTBSNode,
                                    smmChkptPathNode * aChkptPathNode );

    // 특정 Tablespace에서 특정 Checkpoint Path Node를 찾는다.
    static IDE_RC findChkptPathNode(
                      smmTBSNode        * aTBSNode,
                      SChar             * aChkptPath,
                      smmChkptPathNode ** aChkptPathNode );

    // Checkpoint Path의 접근 가능여부 체크 
    static IDE_RC checkAccess2ChkptPath( SChar * aChkptPath );

    // Checkpoint Path Node를 할당하고 초기화한다.
    static IDE_RC makeChkptPathNode( scSpaceID           aSpaceID,
                                     SChar             * aChkptPath,
                                     smmChkptPathNode ** aCPathNode );


    // Checkpoint Path Node의 Checkpoint Path를 변경한다.
    static IDE_RC renameChkptPathNode(
                      smmChkptPathNode * aChkptPathNode,
                      SChar            * aChkptPath );


    // Checkpoint Path Node를 특정 Tablespace에서 제거한다.
    static IDE_RC removeChkptPathNode(
                      smmTBSNode       * aTBSNode,
                      smmChkptPathNode * aChkptPathNode );
    

    // 하나 혹은 그 이상의 Checkpoint Path를 TBSNode에서 제거한다.
    static IDE_RC removeChkptPathNodesIfExist(
                      smmTBSNode           * aTBSNode,
                      smiChkptPathAttrList * aChkptPathList );


    // Tablespace의 다단계 해제중 Media Phase를 내릴때 호출된다
    static IDE_RC freeAllChkptPathNode( smmTBSNode * aTBSNode );
    
private :


    // Checkpoint Path Attribute를 초기화한다.
    static IDE_RC initializeChkptPathAttr(
                      smiChkptPathAttr * aCPathAttr,
                      scSpaceID          aSpaceID,
                      SChar            * aChkptPath );

    // Checkpoint Path Attribute를 초기화한다.
    static IDE_RC initializeChkptPathNode(
                      smmChkptPathNode * aCPathNode,
                      scSpaceID          aSpaceID,
                      SChar            * aChkptPath );

    // Checkpoint Path Node를 파괴한다.
    static IDE_RC destroyChkptPathNode(smmChkptPathNode * aCPathNode );
    
    // Checkpoint Path Node와 Checkpoint Path 문자열을 비교하여
    // 같은 Checkpoint Path인지 체크한다.
    static idBool isSameChkptPath(smmChkptPathNode * aCPathNode,
                                  SChar            * aChkptPath);
    

    
};

#endif /* _O_SMM_TBS_CHKPT_PATH_H_ */
