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
 * $Id: sddTableSpace.h 91197 2021-07-12 01:15:29Z emlee $
 *
 * Description :
 *
 * 본 파일은 디스크관리자의 tablespace 노드에 대한 헤더파일이다.
 *
 * # 목적
 *
 * 알티베이스4 이후로 대용량 디스크를 지원함으로써 하나 또는 하나 이상의
 * 물리적인 데이타파일을 논리적으로 구성하는 파일 level의 자료구조
 *
 * # 타입과 특징
 *
 * 1) 시스템 tablespace (tablespace ID 0 부터 3까지)
 *   시스템 테이블스페이스는 createdb시에 자동으로 생성되며, 모든 user의
 *   기본적인 tablespace로 지정된다. 또한, 시스템 구동시 필요한 정보가
 *   포함되어 있다.
 *
 * - system memory tablespace
 *   기존 메모리 기반 테이블에 대한 테이블스페이스
 *   시스템 전반적으로 tablespace의 관리의 일관성을 위해 정의하며,
 *   별다른 의미는 가지지 않는다.
 *
 * - system data tablespace
 *   시스템의 기본 데이터를 저장하기 위한 공간이다.
 *
 * - system undo tablespace
 *   undo 세그먼트와 TSS(transaction status slot)를 저장하기 위한 공간이다.
 *
 * - system temporary tablespace
 *   데이타베이스의 각종 연산을 처리하기 위한 임시공간이다.
 *
 * 2) 비시스템 tablespace (tablespace ID 4 이후)
 *   runtime시 DDL에 의해 생성가능하다.
 *
 * - user data tablespace
 *   사용자의 데이타를 저장하기 위한 공간
 *
 * - user temporary tablespace
 *   사용자를 위한 임시공간이며, 사용자마다 하나씩만 소유 가능
 *
 *
 *
 * # 구조
 *
 *   - sddTableSpaceNode
 *               ____________
 *               |*_________|   ____ ____
 *               |          |---|@_|-|@_|- ... : sddDatabaseFileNode list
 *               |          |
 *        ____   |          |   ____
 *   prev |*_|...|          |...|*_| next      : sddTableSpaceNode list
 *               |__________|
 *
 * !!] 특징
 * - 테이블스페이스의 모든 정보를 저장함
 * - 데이타파일 노드들의 연결 list를 관리함
 *
 *
 * # 관련 자료구조
 *
 *   sddTableSpaceNode 구조체
 *   sddDataFileNode   구조체
 *
 *
 * # 참고
 *
 * - 4.1.1.1 테이블스페이스 UI.doc
 * - 4.2.1.8 (SDD-SDB) Disk Manager 및 Buffer Manager 관리모듈.doc
 *
 **********************************************************************/

#ifndef _O_SDD_TABLESPACE_H_
#define _O_SDD_TABLESPACE_H_ 1

#include <smDef.h>
#include <sddDef.h>

class sddTableSpace
{
public:

    /* tablespace 노드 초기화 */
    static IDE_RC initialize( sddTableSpaceNode*  aSpaceNode,
                              smiTableSpaceAttr*  aSpaceAttr );

    /* tablespace 노드 해제 */
    static IDE_RC destroy( sddTableSpaceNode* aSpaceNode );

    /* tablespace 노드의 datafile 노드 및 datafile 들을 최초생성 */
    static IDE_RC createDataFiles( idvSQL             * aStatistics,
                                   void               * aTrans,
                                   sddTableSpaceNode  * aSpaceNode,
                                   smiDataFileAttr   ** aFileAttr,
                                   UInt                 aAttrCnt,
                                   smiTouchMode         aTouchMode,
                                   UInt                 aMaxDataFileSize );

    /* PROJ-1923 ALTIBASE HDB Disaster Recovery
     * tablespace 노드의 datafile 노드 및 datafile 을 생성 */
    static IDE_RC createDataFile4Redo( idvSQL             * aStatistics,
                                       void               * aTrans,
                                       sddTableSpaceNode  * aSpaceNode,
                                       smiDataFileAttr    * aFileAttr,
                                       smLSN                aCurLSN,
                                       smiTouchMode         aTouchMode,
                                       UInt                 aMaxDataFileSize );

    /* 하나의 datafile 노드 및 datafile을 제거 */
    static IDE_RC removeDataFile( idvSQL*             aStatistics,
                                  void*               aTrans,
                                  sddTableSpaceNode*  aSpaceNode,
                                  sddDataFileNode*    aFileNode,
                                  smiTouchMode        aTouchMode,
                                  idBool              aDoGhostMark );

    /* 해당 tablespace 노드의 모든 datafile 노드 및 datafile을 제거 */
    static IDE_RC removeAllDataFiles( idvSQL*             aStatistics,
                                      void*               aTrans,
                                      sddTableSpaceNode*  aSpaceNode,
                                      smiTouchMode        aTouchMode,
                                      idBool              aDoGhostMark);

    /* datafile 노드의 remove 가능여부 확인 */
    static IDE_RC canRemoveDataFileNodeByName(
                                     sddTableSpaceNode* aSpaceNode,
                                     SChar*             aDataFileName,
                                     scPageID           aUsedPageLimit,
                                     sddDataFileNode**  aFileNode );

    /* 페이지 ID가 유효할때 DBF Node를 반환 */
    static IDE_RC getDataFileNodeByPageID( sddTableSpaceNode * aSpaceNode,
                                           scPageID            aPageID,
                                           sddDataFileNode  ** aFileNode,
                                           idBool              aFatal );

    /* 페이지 ID가 유효할때 DBF Node를 반환 */
    static void getDataFileNodeByPageIDWithoutException(
                                           sddTableSpaceNode * aSpaceNode,
                                           scPageID            aPageID,
                                           sddDataFileNode  ** aFileNode );

    /* 해당 파일명을 가지는 datafile 노드 반환 */
    static IDE_RC getDataFileNodeByName( sddTableSpaceNode   * aSpaceNode,
                                         SChar               * aFileName,
                                         sddDataFileNode    ** aFileNode );

    /*PRJ-1671 다음 파일노드를 얻어낸다*/
    static void getNextFileNode( sddTableSpaceNode * aSpaceNode,
                                 sdFileID            aCurFileID,
                                 sddDataFileNode  ** aFileNode );


    /* autoextend 모드를 설정할 datafile 노드를 검색 */
    static IDE_RC getDataFileNodeByAutoExtendMode(
                                         sddTableSpaceNode * aSpaceNode,
                                         SChar             * aDataFileName,
                                         idBool              aAutoExtendMode,
                                         scPageID            aUsedPageLimit,
                                         sddDataFileNode  ** aFileNode );

    /* DBF ID로 FileNode 반환 없다면 Exception 발생 */
    static IDE_RC getDataFileNodeByID( sddTableSpaceNode*  aSpaceNode,
                                       UInt                aFileID,
                                       sddDataFileNode**   aFileNode );

    /* 데이타파일이 존재하지 않으면 Null을 반환한다 */
    static void getDataFileNodeByIDWithoutException(
                                         sddTableSpaceNode  * aSpaceNode,
                                         UInt                 aFileID,
                                         sddDataFileNode   ** aFileNode);

    /* datafile 노드의 page 구간을 반환 */
    static IDE_RC getPageRangeByName( sddTableSpaceNode* aSpaceNode,
                                      SChar*             aDataFileName,
                                      sddDataFileNode**  aFileNode,
                                      scPageID*          aFstPageID,
                                      scPageID*          aLstPageID );
    // 데이타파일노드를 검색하여 페이지 구간을 얻는다.
    static IDE_RC getPageRangeInFileByID( sddTableSpaceNode * sSpaceNode,
                                          UInt                aFileID,
                                          scPageID          * aFstPageID,
                                          scPageID          * aLstPageID );

    /* 테이블스페이스의  총 페이지 개수 반환 */
    static ULong getTotalPageCount( sddTableSpaceNode* aSpaceNode );

    /* 테이블스페이스의  총 DBF 개수 반환 */
    static UInt  getTotalFileCount( sddTableSpaceNode* aSpaceNode );

    /* datafile 연결 리스트에 노드 추가 */
    static void addDataFileNode( sddTableSpaceNode*  aSpaceNode,
                                 sddDataFileNode*    aFileNode );

    static void removeMarkDataFileNode( sddDataFileNode * aFileNode );

    static void getTableSpaceAttr( sddTableSpaceNode* aTableSpaceNode,
                                   smiTableSpaceAttr* aTableSpaceAttr);

    static inline UInt getTBSAttrFlag( sddTableSpaceNode  * aSpaceNode );
    static inline void setTBSAttrFlag( sddTableSpaceNode  * aSpaceNode,
                                       UInt                 aAttrFlag );

    static sdFileID getNewFileID( sddTableSpaceNode* aTableSpaceNode );

    static void setOnlineTBSLSN4Idx ( sddTableSpaceNode* aSpaceNode,
                                      smLSN *            aOnlineTBSLSN4Idx )
    { SM_GET_LSN( aSpaceNode->mOnlineTBSLSN4Idx, *aOnlineTBSLSN4Idx ); }

    static smLSN getOnlineTBSLSN4Idx ( void * aSpaceNode )
    { return ((sddTableSpaceNode*)aSpaceNode)->mOnlineTBSLSN4Idx; }

    /* tablespace 노드의 정보를 출력 */
    static IDE_RC dumpTableSpaceNode( sddTableSpaceNode* aSpaceNode );

    /* tablespace 노드의 datafile 노드 리스트를 출력 */
    static void dumpDataFileList( sddTableSpaceNode* aSpaceNode );

    // 서버구동시 복구이후에 테이블스페이스의
    // DataFileCount와 TotalPageCount를 계산하여 설정한다.
    static void calculateFileSizeOfTBS( sddTableSpaceNode * aSpaceNode );
    
    // 테이블스페이스 백업을 처리한다.
    static IDE_RC doActOnlineBackup( idvSQL            * aStatistics,
                                     sctTableSpaceNode * aSpaceNode,
                                     void              * aActionArg );

    // 테이블스페이스의 Dirty된 데이타파일을 Sync 한다.
    static IDE_RC doActSyncTBSInNormal( idvSQL            * aStatistics,
                                        sctTableSpaceNode * aSpaceNode,
                                        void              * aActionArg );

};


/*
   기능 : tablespace 속성값의 주소를 반환한다.
          tablespace 속성값을 변경하고자 할때 사용한다.
*/
UInt sddTableSpace::getTBSAttrFlag( sddTableSpaceNode  * aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    return aSpaceNode->mAttrFlag;
}

void sddTableSpace::setTBSAttrFlag( sddTableSpaceNode  * aSpaceNode,
                                    UInt                 aAttrFlag )
{
    IDE_DASSERT( aSpaceNode != NULL );

    aSpaceNode->mAttrFlag = aAttrFlag;
}

#endif // _O_SDD_TABLESPACE_H_

