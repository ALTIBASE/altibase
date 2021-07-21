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
 * $Id: smmTBSMediaRecovery.h 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#ifndef _O_SMM_TBS_MEDIA_RECOVERY_H_
#define _O_SMM_TBS_MEDIA_RECOVERY_H_ 1

#include <idu.h>
#include <iduMemPool.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>
#include <smu.h>
#include <smriDef.h>

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
   Memory DB의 Tablespace관련 연산들을 구현한 class
   - Create Tablespace 
   - Drop Tablespace
   - Alter Tablespace Add/Rename/Drop Checkpoint Path
   - Alter Tablespace AutoExtend ...

   참고로 Alter Tablespace Online/Offline은
   smpTBSAlterOnOff class에 구현되어 있다.
 */
class smmTBSMediaRecovery
{
public :
    // 생성자 (아무것도 안함)
    smmTBSMediaRecovery();

public :
    // 모든 Chkpt Images들의 메타헤더에 체크포인정보를 갱신한다. 
    static IDE_RC updateDBFileHdr4AllTBS();

    // 테이블스페이스의 마지막 생성된 파일번호를 반환한다. 
    static UInt   getLstCreatedDBFile( smmTBSNode    * aTBSNode )
                  { return aTBSNode->mLstCreatedDBFile; }

//    // 테이블스페이스의 N번째 데이타파일의 페이지 구간 반환
//    static void getPageRangeOfNthFile( smmTBSNode * aTBSNode, 
//                                       UInt         aFileNum,
//                                       scPageID   * aFstPageID, 
//                                       scPageID   * aLstPageID );

    ////////////////////////////////////////////////////////////////////
    // Backup 관련 함수
    ////////////////////////////////////////////////////////////////////

    // PRJ-1548 User Memory Tablespace 
    // 모든 메모리 테이블스페이스를 백업한다. 
    static IDE_RC backupAllMemoryTBS(
                        idvSQL * aStatistics,
                        void   * aTrans,
                        SChar  * aBackupDir );

    
    // PROJ-2133 incremental backup
    // 모든 메모리 테이블스페이스를 incremental backup한다. 
    static IDE_RC incrementalBackupAllMemoryTBS(
                        idvSQL     * aStatistics,
                        void       * aTrans,
                        smriBISlot * aCommonBackupInfo,
                        SChar      * aBackupDir );

    ////////////////////////////////////////////////////////////////////
    // Media Recovery 관련 함수
    ////////////////////////////////////////////////////////////////////

    // 테이블스페이스의 미디어오류가 있는 데이타파일 목록을 만든다.
    static IDE_RC makeMediaRecoveryDBFList( sctTableSpaceNode * sSpaceNode, 
                                            smiRecoverType      aRecoveryType,
                                            UInt              * aFailureChkptImgCount, 
                                            smLSN             * aFromRedoLSN,
                                            smLSN             * aToRedoLSN );

    // 모든 메모리 테이블스페이스의 모든 메모리 DBFile들의 
    // 미디어 복구 필요 여부를 체크한다.
    static IDE_RC identifyDBFilesOfAllTBS( idBool aIsOnCheckPoint );

    // 모든 데이타파일의 메타헤더를 판독하여 
    // 미디어 오류여부를 확인한다. 
    static IDE_RC doActIdentifyAllDBFiles( 
                               idvSQL            * aStatistics, 
                               sctTableSpaceNode  * aSpaceNode,
                               void               * aActionArg );

    // 새로 생성된 데이타파일의 런타임 헤더에 CreateLSN을 
    static IDE_RC setCreateLSN4NewDBFiles( smmTBSNode * aSpaceNode,
                                           smLSN      * aCreateLSN );

    // 미디어오류로 인해 미디어복구를 진행한 메모리 데이타파일들을 
    // 찾아서 파일헤더를 복구한다. 
    static IDE_RC doActRepairDBFHdr( 
                               idvSQL             * aStatistics, 
                               sctTableSpaceNode  * aSpaceNode,
                               void               * aActionArg );

    // 모든 테이블스페이스의 데이타파일에서 입력된 페이지 ID를 가지는 
    // Failure 데이타파일의 존재여부를 반환한다. 
    static IDE_RC findMatchFailureDBF( scSpaceID   aTBSID,
                                       scPageID    aPageID, 
                                       idBool    * aIsExistTBS,
                                       idBool    * aIsFailureDBF );

    // 미디어복구시 테이블스페이스들에 할당했던 자원들을 리셋한다.. 
    static IDE_RC resetMediaFailureMemTBSNodes();
    
    // online backup을 수행한다. 
    static IDE_RC doActOnlineBackup( 
                             idvSQL            * aStatistics, 
                             sctTableSpaceNode * aSpaceNode,
                             void              * aActionArg );


    // 데이타파일의 메타헤더를 갱신한다. 
    static IDE_RC doActUpdateAllDBFileHdr(
                             idvSQL            * aStatistics, 
                             sctTableSpaceNode * aSpaceNode,
                             void              * aActionArg );


    // 미디어복구시 테이블스페이스에 할당했던
    // 자원들을 리셋한다. ( Action 함수 )
    static IDE_RC doActResetMediaFailureTBSNode( 
                            idvSQL            * aStatistics, 
                            sctTableSpaceNode * aTBSNode,
                            void              * /* aActionArg */ );

    // Tablespace를 State 단계로 내렸다가 다시 Page단계로 올린다. 
    static IDE_RC resetTBSNode(smmTBSNode * aTBSNode);

private:
    // 하나의 Tablespace에 속한 모든 DB file의 Header에 Redo LSN을 기록
    static IDE_RC flushRedoLSN4AllDBF( smmTBSNode * aSpaceNode );
};

#endif /* _O_SMM_TBS_MEDIA_RECOVERY_H_ */
