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
 * $Id: qdtDrop.cpp 88963 2020-10-19 03:33:18Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <smiTableSpace.h>
#include <qcg.h>
#include <qcmTableSpace.h>
#include <qcmProc.h>
#include <qcmView.h>
#include <qcmMView.h>
#include <qdd.h>
#include <qdm.h>
#include <qdtDrop.h>
#include <qdpPrivilege.h>
#include <qdpDrop.h>
#include <qdnTrigger.h>
#include <qdbComment.h>
#include <qdx.h>
#include <qcuTemporaryObj.h>
#include <qcmAudit.h>
#include <qcmPkg.h>
#include <qdpRole.h>

IDE_RC qdtDrop::validate(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *
 *    DROP TABLESPACE ... 의 validation 수행
 *
 * Implementation :
 *
 *    1. 권한 검사 qdpPrivilege::checkDDLAlterTableSpacePriv()
 *    2. 명시한 테이블스테이스명이 데이터베이스 내에 이미 존재하는지
 *       메타 검색
 *    3. 테이블스페이스가 SYSTEM TABLESPACE(DATA,TEMP,UNDO)이면 오류
 *    4. INCLUDING CONTENTS 명시 여부에 따른 오류 검사
 *    if ( INCLUDING CONTENTS 명시한 경우 )
 *    {
 *      if ( CASCADE CONSTRAINTS 명시한 경우 )
 *      {
 *        // nothing to do
 *      }
 *      else // CASCADE CONSTRAINTS 명시하지 않은 경우
 *      {
 *        명시한 테이블스페이스에 속한 primary key,unique key 와 관련된
 *        referential constraint 들이 다른 테이블스페이스에 존재하면 오류 발생
 *      }
 *    }
 *    else // INCLUDING CONTENTS 명시하지 않은 경우
 *    {
 *      테이블스페이스에 속한 객체가 하나 이상 존재하면 오류 발생
 *    }
 *
 ***********************************************************************/

#define IDE_FN "qdtDrop::validate"

    UInt                    i;
    qdDropTBSParseTree    * sParseTree;
    smiTableSpaceAttr       sTBSAttr;

    // BUG-28049
    qcmRefChildInfo       * sRefChildInfo;  
    qcmRefChildInfo       * sTempRefChildInfo;
    
    qcmTableInfo          * sTableInfo;
    qcmTableInfo          * sTablePartInfo;
    qcmTableInfo          * sIndexPartInfo;
    qcmTableInfoList      * sTableInfoList = NULL;
    qcmTableInfoList      * sTableInfoListHead = NULL;
    qcmTableInfoList      * sTablePartInfoList = NULL;
    qcmIndex              * sIndexInfo = NULL;
    qcmIndexInfoList      * sIndexInfoList = NULL;
    qcmIndexInfoList      * sIndexPartInfoList = NULL;
    qcmIndexInfoList      * sIndexInfoListHead = NULL;
    idBool                  sExist;
    idBool                  sFound = ID_FALSE;
    UInt                    sTableIDByPartID = 0;
    
    sParseTree = (qdDropTBSParseTree *)aStatement->myPlan->parseTree;

    //-----------------------------------------
    // 권한 검사
    //-----------------------------------------

    IDE_TEST( qdpRole::checkDDLDropTableSpacePriv(
                  aStatement,
                  QCG_GET_SESSION_USER_ID( aStatement ) )
              != IDE_SUCCESS );

    //-----------------------------------------
    // 테이블스페이스 이름으로 메타테이블 검색
    //-----------------------------------------

    IDE_TEST ( qcmTablespace::getTBSAttrByName(
                   aStatement,
                   sParseTree->TBSAttr->mName,
                   sParseTree->TBSAttr->mNameLength,
                   &sTBSAttr) != IDE_SUCCESS );

    //-----------------------------------------
    // 기본적으로 생성되는 테이블스페이스인지 검사(MEMORY,DATA,UNDO)
    //-----------------------------------------
    IDE_TEST_RAISE( smiTableSpace::isSystemTableSpace( sTBSAttr.mID )
                    == ID_TRUE,
                    ERR_NO_DROP_SYSTEM_TBS);

    /* To Fix BUG-17292 [PROJ-1548] Tablespace DDL시 Tablespace에 X락 잡는다
     * Tablespace에 Lock을 잡지 않고 Tablespace안의 Table을 조회하게 되면
     * 그 사이에 새로운 Table이 생겨날 수 있다.
     * 이러한 문제를 미연에 방지하기 위해
     * Tablespace에 X Lock을 먼저 잡고 Tablespace Validation/Execution을 수행 */
    IDE_TEST( smiValidateAndLockTBS(
                  QC_SMI_STMT( aStatement ),
                  sTBSAttr.mID,
                  SMI_TBS_LOCK_EXCLUSIVE,
                  SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

    if ( ( sParseTree->flag & QDT_DROP_INCLUDING_CONTENTS_MASK )
         == QDT_DROP_INCLUDING_CONTENTS_TRUE )
    {
        //-----------------------------------------
        // (1) DROP TABLESPACE tbs_name INCLUDING CONTENTS CASCADE CONSTRAINTS;
        //-----------------------------------------

        if ( ( sParseTree->flag & QDT_DROP_CASCADE_CONSTRAINTS_MASK )
             == QDT_DROP_CASCADE_CONSTRAINTS_TRUE )
        {
            // Nothing To Do
            // referential constraints가 다른 테이블스페이스에 존재하는지
            // validate 때 검사할 필요 없이 execute 때 delete.
            IDE_TEST( qcmTablespace::findTableInfoListInTBS(
                          aStatement,
                          sTBSAttr.mID,
                          ID_TRUE,
                          &sTableInfoListHead) != IDE_SUCCESS );

            IDE_TEST( qcmTablespace::findIndexInfoListInTBS(
                          aStatement,
                          sTBSAttr.mID,
                          ID_TRUE,
                          &sIndexInfoListHead) != IDE_SUCCESS );
        }
        else
        {

            //-----------------------------------------
            // (2) DROP TABLESPACE tbs_name INCLUDING CONTENTS;
            //-----------------------------------------

            // 테이블스페이스에 속한 테이블을 찾아서  sTableInfoList로 받아 옴.
            IDE_TEST( qcmTablespace::findTableInfoListInTBS(
                          aStatement,
                          sTBSAttr.mID,
                          ID_TRUE,
                          &sTableInfoListHead) != IDE_SUCCESS );

            sTableInfoList = sTableInfoListHead;

            while ( sTableInfoList != NULL )
            {
                sTableInfo = sTableInfoList->tableInfo;

                // 테이블의 primary key, unique key 와 관련된
                // referential integrity constraints 가
                // 다른 테이블스페이스에 존재 하는지  check
                for (i=0; i<sTableInfo->uniqueKeyCount; i++)
                {
                    sIndexInfo = sTableInfo->uniqueKeys[i].constraintIndex;

                    // BUG-28049
                    IDE_TEST( qcm::getChildKeys(aStatement,
                                                sIndexInfo,
                                                sTableInfo,
                                                & sRefChildInfo)
                              != IDE_SUCCESS );

                    sTempRefChildInfo = sRefChildInfo;
                    
                    while ( sTempRefChildInfo != NULL )
                    {
                        IDE_TEST_RAISE(
                            sTempRefChildInfo->childTableRef->tableInfo->TBSID !=
                            sTBSAttr.mID,
                            ERR_REFERENTIAL_CONSTRAINT_EXIST );
                        
                        sTempRefChildInfo = sTempRefChildInfo->next;
                    }
                }

                sTableInfoList = sTableInfoList->next;
            }

            // 테이블스페이스에 속한 인덱스를 찾아서 sIndexInfoList로 받아 옴.
            IDE_TEST( qcmTablespace::findIndexInfoListInTBS(
                          aStatement,
                          sTBSAttr.mID,
                          ID_TRUE,
                          &sIndexInfoListHead) != IDE_SUCCESS );

            sIndexInfoList = sIndexInfoListHead;

            while ( sIndexInfoList != NULL )
            {
                sTableInfo = sIndexInfoList->tableInfo;

                sIndexInfo = NULL;
                for ( i = 0; i < sTableInfo->indexCount; i++ )
                {
                    if ( sTableInfo->indices[i].indexId ==
                         sIndexInfoList->indexID )
                    {
                        sIndexInfo = &sTableInfo->indices[i];
                        break;
                    }
                }
                IDE_TEST_RAISE( sIndexInfo == NULL, ERR_NOT_EXIST_INDEX);

                // unique key 와 관련된
                // referential integrity constraints 가
                // 다른 테이블스페이스에 존재 하는지  check
                if ( sIndexInfo->isUnique == ID_TRUE )
                {
                    // BUG-28049
                    IDE_TEST( qcm::getChildKeys(aStatement,
                                                sIndexInfo,
                                                sTableInfo,
                                                & sRefChildInfo)
                              != IDE_SUCCESS );

                    sTempRefChildInfo = sRefChildInfo;
                    
                    while ( sTempRefChildInfo != NULL )
                    {
                        IDE_TEST_RAISE(
                            sTempRefChildInfo->childTableRef->tableInfo->TBSID !=
                            sTBSAttr.mID,
                            ERR_REFERENTIAL_CONSTRAINT_EXIST );
                        
                        sTempRefChildInfo = sTempRefChildInfo->next;
                    }
                }
                sIndexInfoList = sIndexInfoList->next;
            }
        }

        // PROJ-1502 PARTITIONED DISK TABLE
        // 테이블스페이스에 Partitioned Table은 존재하지 않고,
        // Table_Partition만 존재한다면 해당 테이블스페이스는 삭제될 수 없다.
        IDE_TEST( qcmTablespace::findTablePartInfoListInTBS(
                      aStatement,
                      sTBSAttr.mID,
                      &sTablePartInfoList) != IDE_SUCCESS );

        while ( sTablePartInfoList != NULL )
        {
            sTablePartInfo = sTablePartInfoList->tableInfo;

            IDE_TEST( qcmPartition::getTableIDByPartitionID(
                          QC_SMI_STMT(aStatement),
                          sTablePartInfo->partitionID,
                          & sTableIDByPartID )
                      != IDE_SUCCESS );

            sFound = ID_FALSE;

            sTableInfoList = sTableInfoListHead;
            while( sTableInfoList != NULL )
            {
                sTableInfo = sTableInfoList->tableInfo;

                if( sTableInfo->tableID == sTableIDByPartID )
                {
                    sFound = ID_TRUE;
                    break;
                }
                sTableInfoList = sTableInfoList->next;

            }
            IDE_TEST_RAISE( sFound == ID_FALSE,
                            ERR_PART_TABLE_IN_DIFFERENT_TBS );

            sTablePartInfoList = sTablePartInfoList->next;
        }

        // PROJ-1502 PARTITIONED DISK TABLE
        // 테이블스페이스에 Partitioned Table이나 Partitioned Index가 존재하지 않고,
        // Index Partition만 존재한다면 해당 테이블스페이스는 삭제될 수 없다.
        IDE_TEST( qcmTablespace::findIndexPartInfoListInTBS(
                      aStatement,
                      sTBSAttr.mID,
                      &sIndexPartInfoList) != IDE_SUCCESS );

        while ( sIndexPartInfoList != NULL )
        {
            sIndexPartInfo = sIndexPartInfoList->tableInfo;

            sFound = ID_FALSE;

            sTableInfoList = sTableInfoListHead;
            while( sTableInfoList != NULL )
            {
                sTableInfo = sTableInfoList->tableInfo;

                if( sTableInfo->tableID == sIndexPartInfo->tableID )
                {
                    sFound = ID_TRUE;
                    break;
                }
                sTableInfoList = sTableInfoList->next;
            }

            if( sFound == ID_FALSE )
            {
                sIndexInfoList = sIndexInfoListHead;
                while( sIndexInfoList != NULL )
                {
                    if( sIndexInfoList->indexID == sIndexPartInfoList->indexID )
                    {
                        sFound = ID_TRUE;
                        break;
                    }
                    /*
                     * BUG-24515 : 인덱스 파티션만 존재하는 테이블스페이스 삭제시
                     *             서버가 사망합니다.
                     */
                    sIndexInfoList = sIndexInfoList->next;
                }

                IDE_TEST_RAISE( sFound == ID_FALSE,
                                ERR_PART_INDEX_IN_DIFFERENT_TBS );
            }

            sIndexPartInfoList = sIndexPartInfoList->next;
        }
    }
    else
    {
        //-----------------------------------------
        // (3) DROP TABLESPACE tbs_name;
        //-----------------------------------------

        // 테이블스페이스에 속한 객체가 있는지 검사
        IDE_TEST( qcmTablespace::existObject( aStatement,
                                              sTBSAttr.mID,
                                              &sExist )
                  != IDE_SUCCESS );

        // 테이블스페이스에 속한 객체가 하나라도 있으면 에러 출력
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_OBJECT_EXIST );
    }

    sParseTree->TBSAttr->mID   = sTBSAttr.mID;
    sParseTree->TBSAttr->mType = sTBSAttr.mType;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_DROP_SYSTEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_DROP_SYSTEM_TBS));
    }
    IDE_EXCEPTION(ERR_REFERENTIAL_CONSTRAINT_EXIST);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_REFERENTIAL_CONSTRAINT_EXIST));
    }
    IDE_EXCEPTION(ERR_OBJECT_EXIST);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_OBJECT_EXIST));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXISTS_INDEX));
    }
    IDE_EXCEPTION(ERR_PART_TABLE_IN_DIFFERENT_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_PART_TABLE_IN_DIFFERENT_TBS));
    }
    IDE_EXCEPTION(ERR_PART_INDEX_IN_DIFFERENT_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_PART_INDEX_IN_DIFFERENT_TBS));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtDrop::execute(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    DROP TABLESPACE ... 의 execution 수행
 *
 * Implementation :
 *    if ( INCLUDING CONTENTS 명시한 경우 )
 *    {
 *        1. 테이블스페이스에 내의 테이블을 찾아서 sTableInfoList를 받아 옴.
 *        while()
 *        {
 *            executeDropTableInTBS() 호출
 *            : 각각의 테이블에 대해서 테이블 객체와 qp meta정보를 삭제
 *        }
 *        2. MView View와 Materialized View를 제거하고, MVIew View List를 받음
 *        3. 테이블스페이스 내의 인덱스가 소속된 테이블을 찾아서 sIndexInfoList를 받아 옴.
 *        while()
 *        {
 *            executeDropIndexInTBS() 호출
 *            : 각각의 인덱스에 대해서 인덱스 객체와 qp meta정보를 삭제
 *        }
 *    }
 *    else // INCLUDING CONTENTS 명시하지 않은 경우
 *    {
 *        // nothing to do
 *    }
 *    4. 테이블스페이스 객체를 삭제
 *    if ( INCLUDING CONTENTS 명시한 경우 )
 *    {
 *        5. 테이블스페이스 내의 인덱스가 소속된 테이블에 대해서 new cached meta를 만듦
 *           : new cached meta를 만들다가 실패할 경우(ABORT에러) 예외처리를 위해
 *           : sStage가 1로 유지됨.
 *        6. 테이블스페이스 내의 테이블이 가지는 Trigger cached meta인 qcmTriggerInfo를 삭제
 *        7. MVIew View의 Trigger cached meta를 삭제
 *        8. 테이블스페이스 내의 인덱스가 소속된 테이블에 대해서 old cached meta를 삭제
 *        9. 테이블스페이스 내의 테이블에 대해서 cached meta를 삭제
 *       10. MVIew View의 cached meta를 삭제
 *    }
 *    else // INCLUDING CONTENTS 명시하지 않은 경우
 *    {
 *        // nothing to do
 *    }
 *
 * Attention :
 *    위의 구현 순서 중 5. 6. 7. 8, 9, 10의 순서가 매우 중요 함.
 *
 * < DDL중 DROP EXECUTION 에서 반드시 지켜야 할 규칙 2가지>
 *
 * (1) 객체와, 객체와 관련된 qp meta 테이블의 레코드를 삭제 후
 *     QP에서 유지하고 있던 cached meta 를 free해야 한다.
 *     이유는,
 *         객체와, qp meta 테이블의 레코드를 삭제 할 때는
 *         sm 모듈에서 log를 남기므로, 중간에 실패를 하더라도 원복이 가능하나
 *         cached meta는 한번 free되면 원복이 힘들다.
 *         따라서 객체와 qp meta 테이블의 레코드를 모두 성공적으로 삭제 한 다음에
 *         cached meta를 free해 준다.
 *         그리고 cached meta가 실패한 경우는 거의 없다고 가정을 하며
 *         만약 실패한 경우는 FATAL 에러가 리턴되어 서버가 종료된다.
 *
 * (2) 객체와 관련된 qp meta테이블들의 레코드를 삭제할 때
 *     반드시 같은 순서로 접근을 해야 한다.
 *     예를 들면,
 *         qdd::executeDropTable에서는
 *             delete SYS_CONSTRAINTS_
 *             delete SYS_TABLES_
 *         순서대로 qp meta테이블을 접근하여 레코드를 삭제하고
 *         qdt::executeDropTableInTBS 에서는
 *             delete SYS_TABLES_
 *             delete SYS_CONSTRAINTS_
 *         순서대로 qp meta테이블을 접근하여 레코드를 삭제하고
 *
 *         다른 세션에서 위의 두 함수가 동시에 호출될 경우
 *         dead lock이 발생하게 된다.
 *
 ***********************************************************************/

#define IDE_FN "qdtDrop::execute"

    qdDropTBSParseTree    * sParseTree;
    smiTrans              * sTrans;
    qcmTableInfo          * sTableInfo;
    qcmTableInfoList      * sTableInfoList = NULL;
    qcmTableInfoList      * sMViewOfRelatedInfoList = NULL;
    qcmTableInfoList      * sTempTableInfoList = NULL;
    qcmIndexInfoList      * sIndexInfoList = NULL;
    qcmIndexInfoList      * sTempIndexInfoList = NULL;
    smiTouchMode            sTouchMode = SMI_ALL_NOTOUCH;
    UInt                    sPartTableCount = 0;
    UInt                    i = 0;
    qcmPartitionInfoList ** sTablePartInfoList = NULL;
    qcmPartitionInfoList ** sIndexPartInfoList = NULL;
    qcmPartitionInfoList  * sTempPartInfoList = NULL;
    qdIndexTableList     ** sIndexTableList = NULL;
    qdIndexTableList      * sIndexTable = NULL;

    sParseTree = (qdDropTBSParseTree *)aStatement->myPlan->parseTree;

    sTrans = (QC_SMI_STMT( aStatement ))->getTrans();

    // To Fix BUG-17292
    //        [PROJ-1548] Tablespace DDL시 Tablespace에 X락 잡는다
    //
    // Tablespace에 Lock을 잡지 않고 Tablespace안의 Table을 조회하게 되면
    // 그 사이에 새로운 Table이 생겨날 수 있다.
    //
    // 이러한 문제를 미연에 방지하기 위해
    // Tablespace에 X Lock을 먼저 잡고 Tablespace Validation/Execution을 수행
    IDE_TEST( smiValidateAndLockTBS(
                  QC_SMI_STMT( aStatement ),
                  sParseTree->TBSAttr->mID,
                  SMI_TBS_LOCK_EXCLUSIVE,
                  SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

    // INCLUDING CONTENTS 구문을 사용한 경우
    // 테이블스페이스 내의 모든 객체들을 삭제한다.
    if ( ( sParseTree->flag & QDT_DROP_INCLUDING_CONTENTS_MASK )
         == QDT_DROP_INCLUDING_CONTENTS_TRUE )
    {
        // 테이블스페이스에 속한 테이블을 찾아서 sTableInfoList로 받아 옴.
        IDE_TEST( qcmTablespace::findTableInfoListInTBS(
                      aStatement,
                      sParseTree->TBSAttr->mID,
                      ID_FALSE,
                      &sTableInfoList) != IDE_SUCCESS );

        /* PROJ-1407 Temporary table
         * session temporary table이 존재하는 경우 tablespace에
         * DDL을 할 수 없다. */
        for( sTempTableInfoList = sTableInfoList;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            /* tablespace lock이 잡았으므로 다른 Transaction이
             * table에 DDL을 할 수 없다. table lock을 잡지않고도
             * table info에 접근 할 수 있다.*/
            IDE_TEST_RAISE( qcuTemporaryObj::existSessionTable(
                                sTempTableInfoList->tableInfo ) == ID_TRUE,
                            ERR_SESSION_TEMPORARY_TABLE_EXIST );
        }

        // PROJ-1502 PARTITIONED DISK TABLE
        sPartTableCount = 0;
        for( sTempTableInfoList = sTableInfoList;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            sTableInfo = sTempTableInfoList->tableInfo;

            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                sPartTableCount++;
            }
        }

        if( sPartTableCount != 0 )
        {
            IDU_FIT_POINT( "qdtDrop::execute::alloc::sTablePartInfoList",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_COUNT( aStatement->qmxMem,
                                               qcmPartitionInfoList*,
                                               sPartTableCount,
                                               & sTablePartInfoList )
                      != IDE_SUCCESS);

            IDE_TEST( STRUCT_ALLOC_WITH_COUNT( aStatement->qmxMem,
                                               qdIndexTableList*,
                                               sPartTableCount,
                                               & sIndexTableList )
                      != IDE_SUCCESS);
        }

        // PROJ-1502 PARTITIONED DISK TABLE
        for( sTempTableInfoList = sTableInfoList, i = 0;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            sTableInfo = sTempTableInfoList->tableInfo;

            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_ASSERT( sTablePartInfoList != NULL );

                IDE_TEST( qcmPartition::getPartitionInfoList(
                              aStatement,
                              QC_SMI_STMT( aStatement ),
                              aStatement->qmxMem,
                              sTableInfo->tableID,
                              & sTablePartInfoList[i] )
                          != IDE_SUCCESS );

                // 모든 파티션에 LOCK(X)
                /* To Fix BUG-17285
                 * [PROJ-1548-test] Disk Tablespace OFFLINE/DICARD
                 * 후 DROP시 에러발생 */
                IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                          sTablePartInfoList[i],
                                                                          SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                                          SMI_TABLE_LOCK_X,
                                                                          smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()) )
                          != IDE_SUCCESS );

                // PROJ-1624 non-partitioned index
                IDE_TEST( qdx::makeAndLockIndexTableList(
                              aStatement,
                              ID_TRUE,
                              sTableInfo,
                              & sIndexTableList[i] )
                          != IDE_SUCCESS );
            
                /* To Fix BUG-17285
                 * [PROJ-1548-test] Disk Tablespace OFFLINE/DICARD
                 * 후 DROP시 에러발생 */
                IDE_TEST( qdx::validateAndLockIndexTableList( aStatement,
                                                              sIndexTableList[i],
                                                              SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                              SMI_TABLE_LOCK_X,
                                                              smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()) )
                          != IDE_SUCCESS );

                i++;
            }
        }

        // 테이블스페이스에 속한 인덱스를 찾아서 sIndexInfoList로 받아 옴.
        IDE_TEST( qcmTablespace::findIndexInfoListInTBS(
                      aStatement,
                      sParseTree->TBSAttr->mID,
                      ID_FALSE,
                      &sIndexInfoList) != IDE_SUCCESS );

        // PROJ-1502 PARTITIONED DISK TABLE
        sPartTableCount = 0;
        for( sTempIndexInfoList = sIndexInfoList;
             sTempIndexInfoList != NULL;
             sTempIndexInfoList = sTempIndexInfoList->next )
        {
            sTableInfo = sTempIndexInfoList->tableInfo;

            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                sPartTableCount++;
            }
        }

        if( sPartTableCount != 0 )
        {
            IDU_FIT_POINT( "qdtDrop::execute::alloc::sIndexPartInfoList",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_COUNT( aStatement->qmxMem,
                                               qcmPartitionInfoList,
                                               sPartTableCount,
                                               & sIndexPartInfoList )
                      != IDE_SUCCESS);
        }

        // PROJ-1502 PARTITIONED DISK TABLE
        for( sTempIndexInfoList = sIndexInfoList, i = 0;
             sTempIndexInfoList != NULL;
             sTempIndexInfoList = sTempIndexInfoList->next )
        {
            sTableInfo = sTempIndexInfoList->tableInfo;

            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_ASSERT( sIndexPartInfoList != NULL );
                
                IDE_TEST( qcmPartition::getPartitionInfoList(
                              aStatement,
                              QC_SMI_STMT( aStatement ),
                              aStatement->qmxMem,
                              sTableInfo->tableID,
                              & sIndexPartInfoList[i] )
                          != IDE_SUCCESS );

                // 모든 파티션에 LOCK(X)
                /* To Fix BUG-17285
                 * [PROJ-1548-test] Disk Tablespace OFFLINE/DICARD
                 * 후 DROP시 에러발생 */
                IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                          sIndexPartInfoList[i],
                                                                          SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                                          SMI_TABLE_LOCK_X,
                                                                          smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                          != IDE_SUCCESS );

                // PROJ-1624 global non-partitioned index
                // non-partitioned index하나만 lock
                if ( sTempIndexInfoList->isPartitionedIndex == ID_FALSE )
                {
                    sIndexTable = & sTempIndexInfoList->indexTable;
                
                    IDE_TEST(qcm::getTableInfoByID(aStatement,
                                                   sIndexTable->tableID,
                                                   &sIndexTable->tableInfo,
                                                   &sIndexTable->tableSCN,
                                                   &sIndexTable->tableHandle)
                             != IDE_SUCCESS);
                
                    /* To Fix BUG-17285
                     * [PROJ-1548-test] Disk Tablespace OFFLINE/DICARD
                     * 후 DROP시 에러발생 */
                    IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                                        sIndexTable->tableHandle,
                                                        sIndexTable->tableSCN,
                                                        SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                        SMI_TABLE_LOCK_X,
                                                        smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                                        ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                             != IDE_SUCCESS);
                }
                else
                {
                    // Nothing to do.
                }
                
                i++;
            }
        }

        for( sTempTableInfoList = sTableInfoList, i = 0;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            sTableInfo = sTempTableInfoList->tableInfo;

            //-----------------------------------------
            // 각각의 테이블에 대해서
            // 테이블 객체와 qp meta정보를 삭제
            //-----------------------------------------

            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_ASSERT( sTablePartInfoList != NULL );

                IDE_TEST( executeDropTableInTBS(aStatement,
                                                sTempTableInfoList,
                                                sTablePartInfoList[i],
                                                sIndexTableList[i] )
                          != IDE_SUCCESS );

                i++;
            }
            else
            {
                IDE_TEST( executeDropTableInTBS(aStatement,
                                                sTempTableInfoList,
                                                NULL,
                                                NULL )
                          != IDE_SUCCESS );
            }
        }

        /* PROJ-2211 Materialized View */
        IDE_TEST( executeDropMViewOfRelated(
                        aStatement,
                        sTableInfoList,
                        &sMViewOfRelatedInfoList )
                  != IDE_SUCCESS );

        for( sTempIndexInfoList = sIndexInfoList, i = 0;
             sTempIndexInfoList != NULL;
             sTempIndexInfoList = sTempIndexInfoList->next )
        {
            sTableInfo = sTempIndexInfoList->tableInfo;

            //-----------------------------------------
            // 각각의 인덱스에 대해서
            // 인덱스 객체와 qp meta정보를 삭제
            //-----------------------------------------

            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_ASSERT( sIndexPartInfoList != NULL );

                IDE_TEST( executeDropIndexInTBS(aStatement,
                                                sTempIndexInfoList,
                                                sIndexPartInfoList[i])
                        != IDE_SUCCESS );

                i++;
            }
            else
            {
                IDE_TEST( executeDropIndexInTBS(aStatement,
                                                sTempIndexInfoList,
                                                NULL)
                        != IDE_SUCCESS );
            }
        }

        //-----------------------------------------
        // AND DATAFILES 구문을 사용하면,
        // SMI_ALL_TOUCH을 셋팅해서 sm모듈로 넘겨 줌.
        //-----------------------------------------

        if ( ( sParseTree->flag & QDT_DROP_AND_DATAFILES_MASK )
             == QDT_DROP_AND_DATAFILES_TRUE )
        {
            sTouchMode = SMI_ALL_TOUCH;
        }
        else
        {
            sTouchMode = SMI_ALL_NOTOUCH;
        }
    }
    else
    {
        // Nothing To Do
    }

    //-----------------------------------------
    // 테이블스페이스 객체를 삭제
    //-----------------------------------------

    IDE_TEST( smiTableSpace::drop(aStatement->mStatistics,
                                  sTrans,
                                  sParseTree->TBSAttr->mID,
                                  sTouchMode)
          != IDE_SUCCESS );

    // INCLUDING CONTENTS 구문을 사용한 경우 테이블스페이스 내의 모든 객체들을 삭제한다.
    if ( ( sParseTree->flag & QDT_DROP_INCLUDING_CONTENTS_MASK )
         == QDT_DROP_INCLUDING_CONTENTS_TRUE )
    {
        //-----------------------------------------
        // 테이블스페이스 내의 인덱스가 소속된 테이블에 대해서
        // new cached meta를 만듦
        // 실패한 경우의 예외처리를 위해 sStage가 1로 유지됨
        //-----------------------------------------
        for( sTempIndexInfoList = sIndexInfoList, i = 0;
             sTempIndexInfoList != NULL;
             sTempIndexInfoList = sTempIndexInfoList->next )
        {
            sTableInfo = sTempIndexInfoList->tableInfo;

            IDE_TEST( qcm::touchTable(QC_SMI_STMT( aStatement ),
                                      sTableInfo->tableID,
                                      SMI_TBSLV_DROP_TBS)
                    != IDE_SUCCESS );

            // BUG-16422
            IDE_TEST( qcmTableInfoMgr::makeTableInfo( aStatement,
                                                      sTableInfo,
                                                      NULL )
                      != IDE_SUCCESS );

            // PROJ-1502 PARTITIONED DISK TABLE
            if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_ASSERT( sIndexPartInfoList != NULL );

                for( sTempPartInfoList = sIndexPartInfoList[i];
                     sTempPartInfoList != NULL;
                     sTempPartInfoList = sTempPartInfoList->next )
                {
                    IDE_TEST( smiTable::touchTable( QC_SMI_STMT( aStatement ),
                                                    sTempPartInfoList->partitionInfo->tableHandle,
                                                    SMI_TBSLV_DROP_TBS )
                              != IDE_SUCCESS );

                    IDE_TEST( qcmTableInfoMgr::makeTableInfo(
                                  aStatement,
                                  sTempPartInfoList->partitionInfo,
                                  NULL )
                              != IDE_SUCCESS);
                }
                
                // PROJ-1624 global non-partitioned index
                // index table tableinfo는 destroy한다.
                if ( sTempIndexInfoList->isPartitionedIndex == ID_FALSE )
                {
                    sIndexTable = & sTempIndexInfoList->indexTable;
                
                    IDE_TEST( qcmTableInfoMgr::destroyTableInfo(
                                  aStatement,
                                  sIndexTable->tableInfo )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                i++;
            }
        }

        //-----------------------------------------
        // 테이블스페이스 내의 테이블이 가지는
        // Trigger cached meta인 qcmTriggerInfo를 삭제
        //-----------------------------------------

        for( sTempTableInfoList = sTableInfoList;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            // 정상 실행되거나, 에러이면 FATAL이 이미 설정되어 있음.
            IDE_TEST( qdnTrigger::freeTriggerCaches4DropTable(
                          sTempTableInfoList->tableInfo ) != IDE_SUCCESS );
        }

        //-----------------------------------------
        // 테이블스페이스 내의 인덱스가 소속된 테이블에 대해서
        // old cached meta를 삭제
        //-----------------------------------------

        // 인덱스의 삭제는 tableInfo를 변경할 뿐이므로
        // tableInfo를 삭제하지는 않는다.

        //-----------------------------------------
        // 테이블스페이스 내의 테이블에 대해서
        // cached meta를 삭제
        //-----------------------------------------

        for( sTempTableInfoList = sTableInfoList, i = 0;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            // BUG-16422
            IDE_TEST( qcmTableInfoMgr::destroyTableInfo(
                          aStatement,
                          sTempTableInfoList->tableInfo )
                      != IDE_SUCCESS );

            // PROJ-1502 PARTITIONED DISK TABLE
            if( sTempTableInfoList->tableInfo->tablePartitionType
                == QCM_PARTITIONED_TABLE )
            {
                IDE_ASSERT( sTablePartInfoList != NULL );
                
                for( sTempPartInfoList = sTablePartInfoList[i];
                     sTempPartInfoList != NULL;
                     sTempPartInfoList = sTempPartInfoList->next )
                {
                    IDE_TEST( qcmTableInfoMgr::destroyTableInfo(
                                  aStatement,
                                  sTempPartInfoList->partitionInfo)
                              != IDE_SUCCESS );
                }

                IDE_ASSERT( sIndexTableList != NULL );
            
                for ( sIndexTable = sIndexTableList[i];
                      sIndexTable != NULL;
                      sIndexTable = sIndexTable->next )
                {
                    IDE_TEST( qcmTableInfoMgr::destroyTableInfo( aStatement,
                                                                 sIndexTable->tableInfo )
                              != IDE_SUCCESS );
                }

                i++;
            }
        }

        /* PROJ-2211 Materialized View */
        for( sTempTableInfoList = sMViewOfRelatedInfoList;
             sTempTableInfoList != NULL;
             sTempTableInfoList = sTempTableInfoList->next )
        {
            /* BUG-16422 */
            IDE_TEST( qcmTableInfoMgr::destroyTableInfo(
                          aStatement,
                          sTempTableInfoList->tableInfo )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_TEMPORARY_TABLE_EXIST )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDT_DROP_TBS_DISABLE_BECAUSE_TEMP_TABLE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtDrop::executeDropTableInTBS(qcStatement          * aStatement,
                                      qcmTableInfoList     * aTableInfoList,
                                      qcmPartitionInfoList * aPartInfoList,
                                      qdIndexTableList     * aIndexTables)
{
/***********************************************************************
 *
 * Description :
 *    qdtDrop::execute로 부터 호출
 *    테이블스페이스 내의 테이블 객체와 qp meta정보를 삭제
 *
 * Implementation :
 *    1. 테이블에 unique 인덱스가 있을 경우 referential constraint를
 *       찾아서 삭제
 *    2. SYS_CONSTRAINTS_ 에서 관련된 constraint 정보 삭제
 *    3. SYS_INDICES_, SYS_INDEX_COLUMNS_ 에서 테이블 정보 삭제
 *    4. SYS_TABLES_, SYS_COLUMNS_ 에서 테이블 정보 삭제
 *    5. SYS_GRANT_OBJECT_ 에서 테이블과 관련된 privilege 정보 삭제
 *    6. Trigger 객체 삭제와
 *       SYS_TRIGGERS_ ..등 에서 테이블과 관련된 Trigger 정보 삭제
 *    7. related PSM 을 invalid 상태로 변경
 *    8. related VIEW 을 invalid 상태로 변경
 *    9. Constraint와 관련된 Procedure에 대한 정보를 삭제
 *   10. Index와 관련된 Procedure에 대한 정보를 삭제
 *   11. 테이블의 객체를 삭제
 *
 ***********************************************************************/
#define IDE_FN "qdtDrop::executeDropTableInTBS"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdtDrop::executeDropTableInTBS"));

    qcmTableInfo          * sTableInfo;
    qcmIndex              * sIndexInfo = NULL;
    UInt                    i;
    qcmPartitionInfoList  * sPartInfoList = NULL;
    qcmTableInfo          * sPartInfo;
    qdIndexTableList      * sIndexTable;

    //-----------------------------------------
    // 테이블에 unique 인덱스가 있을 경우
    // referential constraint를 찾아서 삭제
    //-----------------------------------------

    sTableInfo = aTableInfoList->tableInfo;

    /* PROJ-1407 Temporary table
     * session temporary table이 존재하는 경우 DDL을 할 수 없다.
     * 앞에서 session table의 존재유무를 미리 확인하였다.
     * session table이 있을리 없다.*/
    IDE_DASSERT( qcuTemporaryObj::existSessionTable( sTableInfo ) == ID_FALSE );

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        for( sPartInfoList = aPartInfoList;
             sPartInfoList != NULL;
             sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            // BUG-35135
            IDE_TEST( qdd::dropTablePartition( aStatement,
                                               sPartInfo,
                                               ID_TRUE, /* aIsDropTablespace */
                                               ID_FALSE )
                      != IDE_SUCCESS );
        }
            
        // PROJ-1624 non-partitioned index
        for ( sIndexTable = aIndexTables;
              sIndexTable != NULL;
              sIndexTable = sIndexTable->next )
        {
            // BUG-35135
            IDE_TEST( qdx::dropIndexTable( aStatement,
                                           sIndexTable,
                                           ID_TRUE /* aIsDropTablespace */ )
                      != IDE_SUCCESS );
        }
    }

    for (i = 0; i < sTableInfo->uniqueKeyCount; i++)
    {
        sIndexInfo = sTableInfo->uniqueKeys[i].constraintIndex;

        IDE_TEST( qcm::getChildKeysForDelete( aStatement,
                                              QC_EMPTY_USER_ID,
                                              sIndexInfo,
                                              sTableInfo,
                                              ID_TRUE /* aDropTablespace */,
                                              &(aTableInfoList->childInfo) )
                  != IDE_SUCCESS );

        IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                            sTableInfo,
                                            aTableInfoList->childInfo,
                                            ID_TRUE /* aDropTablespace */ )
                  != IDE_SUCCESS );
    }

    //-----------------------------------------
    // SYS_CONSTRAINTS_ 에서 관련된 constraint 정보 삭제
    //-----------------------------------------

    IDE_TEST( qdd::deleteConstraintsFromMeta(aStatement,
                                             sTableInfo->tableID)
              != IDE_SUCCESS );

    //-----------------------------------------
    // SYS_INDICES_, SYS_INDEX_COLUMNS_ 에서 테이블 정보 삭제
    //-----------------------------------------

    IDE_TEST( qdd::deleteIndicesFromMeta(aStatement,
                                         sTableInfo)
              != IDE_SUCCESS );

    //-----------------------------------------
    // SYS_TABLES_, SYS_COLUMNS_ 에서 테이블 정보 삭제
    //-----------------------------------------

    IDE_TEST( qdd::deleteTableFromMeta(aStatement,
                                       sTableInfo->tableID)
              != IDE_SUCCESS );

    //-----------------------------------------
    // SYS_GRANT_OBJECT_ 에서 테이블과 관련된 privilege 정보 삭제
    //-----------------------------------------

    IDE_TEST( qdpDrop::removePriv4DropTable(aStatement,
                                            sTableInfo->tableID)
              != IDE_SUCCESS );

    //-----------------------------------------
    // Trigger 객체와
    // SYS_TRIGGERS_ ..등 에서 테이블과 관련된 Trigger 정보 삭제
    //-----------------------------------------

    IDE_TEST(qdnTrigger::dropTrigger4DropTable(aStatement, sTableInfo )
             != IDE_SUCCESS);

    /* PROJ-2197 PSM Renewal */
    //-----------------------------------------
    // related PSM 을 invalid 상태로 변경
    //-----------------------------------------

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                aStatement,
                sTableInfo->tableOwnerID,
                sTableInfo->name,
                idlOS::strlen((SChar*)sTableInfo->name),
                QS_TABLE) != IDE_SUCCESS );

    // PROJ-1073 Package
    //-----------------------------------------
    // related PSM 을 invalid 상태로 변경
    //-----------------------------------------
    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                aStatement,
                sTableInfo->tableOwnerID,
                sTableInfo->name,
                idlOS::strlen((SChar*)sTableInfo->name),
                QS_TABLE) != IDE_SUCCESS );
        
    //-----------------------------------------
    // related VIEW 을 invalid 상태로 변경
    //-----------------------------------------

    IDE_TEST( qcmView::setInvalidViewOfRelated(
                  aStatement,
                  sTableInfo->tableOwnerID,
                  sTableInfo->name,
                  idlOS::strlen((SChar*)sTableInfo->name),
                  QS_TABLE) != IDE_SUCCESS );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    IDE_TEST( qcmProc::relRemoveRelatedToConstraintByTableID(
                    aStatement,
                    sTableInfo->tableID )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relRemoveRelatedToIndexByTableID(
                    aStatement,
                    sTableInfo->tableID )
              != IDE_SUCCESS );

    //-----------------------------------------
    // BUG-21387 COMMENT
    // SYS_COMMENTS_에서 Comment를 삭제한다.
    //-----------------------------------------
    IDE_TEST( qdbComment::deleteCommentTable(
                  aStatement,
                  sTableInfo->tableOwnerName,
                  sTableInfo->name)
              != IDE_SUCCESS );

    // PROJ-2223 audit
    IDE_TEST( qcmAudit::deleteObject(
                  aStatement,
                  sTableInfo->tableOwnerID,
                  sTableInfo->name )
              != IDE_SUCCESS );

    // PROJ-2264 Dictionary table
    // SYS_COMPRESSION_TABLES_ 에서 관련 레코드를 삭제한다.
    IDE_TEST( qdd::deleteCompressionTableSpecFromMeta( aStatement,
                                                       sTableInfo->tableID )
              != IDE_SUCCESS );

    //-----------------------------------------
    // 테이블 객체를 삭제
    //-----------------------------------------

    IDE_TEST( smiTable::dropTable( QC_SMI_STMT( aStatement ),
                                   sTableInfo->tableHandle,
                                   SMI_TBSLV_DROP_TBS )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdtDrop::executeDropIndexInTBS(qcStatement          * aStatement,
                                      qcmIndexInfoList     * aIndexInfoList,
                                      qcmPartitionInfoList * aPartInfoList)
{
/***********************************************************************
 *
 * Description :
 *    qdtDrop::execute로 부터 호출
 *    테이블스페이스 내의 인덱스 객체와 qp meta정보를 삭제
 *
 * Implementation :
 *    1. unique 인덱스가 있을 경우 referential constraint를
 *       찾아서 삭제
 *    2. 인덱스 객체를 삭제
 *    3. SYS_INDICES_, SYS_INDEX_COLUMNS_ 에서 인덱스 정보 삭제
 *    4. related PSM 을 invalid 상태로 변경
 *
 ***********************************************************************/
#define IDE_FN "qdtDrop::executeDropIndexInTBS"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdtDrop::executeDropIndexInTBS"));

    qcmTableInfo          * sTableInfo;
    qcmIndex              * sIndexInfo = NULL;
    idBool                  sIsPrimary = ID_FALSE;
    UInt                    i;
    qcmPartitionInfoList  * sPartInfoList = NULL;
    void                  * sIndexHandle;

    sTableInfo = aIndexInfoList->tableInfo;

    sPartInfoList = aPartInfoList;

    for (i = 0; i < sTableInfo->indexCount; i++ )
    {
        if ( sTableInfo->indices[i].indexId ==
             aIndexInfoList->indexID )
        {
            sIndexInfo = &sTableInfo->indices[i];

            // BUG-28321 
            sIndexHandle = (void *)smiGetTableIndexByID(
                sTableInfo->tableHandle,
                sIndexInfo->indexId );

            break;
        }
    }
    IDE_TEST_RAISE( sIndexInfo == NULL, ERR_NOT_EXIST_INDEX);

    //-----------------------------------------
    // unique 인덱스가 있을 경우 referential constraint를 찾아서 삭제
    //-----------------------------------------

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( sTableInfo->primaryKey != NULL )
        {
            if ( sTableInfo->primaryKey->indexId == sIndexInfo->indexId )
            {
                sIsPrimary = ID_TRUE;
            }
            else
            {
                sIsPrimary = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
        
        // PROJ-1624 non-partitioned index
        if ( ( sIndexInfo->indexPartitionType == QCM_NONE_PARTITIONED_INDEX ) ||
             ( sIsPrimary == ID_TRUE ) )
        {
            // BUG-35135
            IDE_TEST( qdx::dropIndexTable( aStatement,
                                           & aIndexInfoList->indexTable,
                                           ID_TRUE /* aIsDropTablespace */ )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        if ( ( sIndexInfo->indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) ||
             ( sIsPrimary == ID_TRUE ) )
        {
            // BUG-35135
            IDE_TEST( qdd::dropIndexPartitions( aStatement,
                                                sPartInfoList,
                                                sIndexInfo->indexId,
                                                ID_TRUE /* aIsCascade */,
                                                ID_TRUE /* aIsDropTablespace */ )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sIndexInfo->isUnique == ID_TRUE )
    {
        IDE_TEST( qcm::getChildKeysForDelete( aStatement,
                                              QC_EMPTY_USER_ID,
                                              sIndexInfo,
                                              sTableInfo,
                                              ID_TRUE /* aDropTablespace */,
                                              &(aIndexInfoList->childInfo) )
                  != IDE_SUCCESS );

        IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                            sTableInfo,
                                            aIndexInfoList->childInfo,
                                            ID_TRUE /* aDropTablespace */ )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------------
    // 인덱스 객체를 삭제
    //-----------------------------------------

    IDE_TEST( smiTable::dropIndex(QC_SMI_STMT( aStatement ),
                                  sTableInfo->tableHandle,
                                  sIndexHandle,
                                  SMI_TBSLV_DROP_TBS)
              != IDE_SUCCESS );

    //-----------------------------------------
    // SYS_INDICES_, SYS_INDEX_COLUMNS_ 에서 인덱스 정보 삭제
    //-----------------------------------------

    IDE_TEST( qdd::deleteIndicesFromMetaByIndexID(aStatement,
                                                  sIndexInfo->indexId)
              != IDE_SUCCESS );

    //-----------------------------------------
    // BUG-17326
    // Constraint로 생성된 인덱스인 경우 Constraint도 함께 삭제
    // SYS_CONSTRAINTS_, SYS_CONSTRAINT_COLUMNS_ 에서 Constraint 정보 삭제
    //-----------------------------------------

    for ( i = 0; i < sTableInfo->uniqueKeyCount; i++ )
    {
        if ( sTableInfo->uniqueKeys[i].constraintIndex->indexId ==
             sIndexInfo->indexId )
        {
            IDE_TEST( qdd::deleteConstraintsFromMetaByConstraintID(
                          aStatement,
                          sTableInfo->uniqueKeys[i].constraintID )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXISTS_INDEX));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/***********************************************************************
 *
 * Description :
 *    qdtDrop::execute()에서 Materialized View의 MVIew Table을 제외한 부분을
 *    제거하기 위해 호출한다. (Meta Cache는 제외)
 *    나중에 Meta Cache를 제거하기 위해, MVIew View List를 반환한다.
 *
 *    Table List에 MView Table이 있으면,
 *    (1) MView View를 제거한다. (Meta Cache는 제외)
 *    (2) Materialized View를 Meta Table에서 제거한다.
 *
 * Implementation :
 *    MVIew Table이면, 아래의 작업을 수행한다.
 *    1. lock & get view information
 *    2. Meta Table에서 View 삭제
 *    3. Meta Table에서 Object Privilege 삭제 (View)
 *    4. Trigger 삭제 (Meta Table, Object)
 *    5. related PSM 을 Invalid 상태로 변경 (View)
 *    6. related VIEW 을 Invalid 상태로 변경 (View)
 *    7. smiTable::dropTable (View)
 *    8. Meta Table에서 Materialized View 삭제
 *    9. MVIew View List에 View를 추가한다.
 *
 ***********************************************************************/
IDE_RC qdtDrop::executeDropMViewOfRelated(
                        qcStatement       * aStatement,
                        qcmTableInfoList  * aTableInfoList,
                        qcmTableInfoList ** aMViewOfRelatedInfoList )
{
    qcmTableInfoList * sTableInfoList     = NULL;
    qcmTableInfoList * sMViewInfoList     = NULL;
    qcmTableInfoList * sLastTableInfoList = NULL;
    qcmTableInfoList * sNewTableInfoList  = NULL;
    qcmTableInfo     * sTableInfo         = NULL;

    SChar              sViewName[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmTableInfo     * sViewInfo          = NULL;
    void             * sViewHandle        = NULL;
    smSCN              sViewSCN;
    UInt               sMViewID;

    for ( sTableInfoList = aTableInfoList;
          sTableInfoList != NULL;
          sTableInfoList = sTableInfoList->next )
    {
        sTableInfo = sTableInfoList->tableInfo;

        if ( sTableInfo->tableType == QCM_MVIEW_TABLE )
        {
            (void)idlOS::memset( sViewName, 0x00, QC_MAX_OBJECT_NAME_LEN + 1 );
            (void)idlVA::appendString( sViewName,
                                       QC_MAX_OBJECT_NAME_LEN + 1,
                                       sTableInfo->name,
                                       idlOS::strlen( sTableInfo->name ) );
            (void)idlVA::appendString( sViewName,
                                       QC_MAX_OBJECT_NAME_LEN + 1,
                                       QDM_MVIEW_VIEW_SUFFIX,
                                       QDM_MVIEW_VIEW_SUFFIX_SIZE );

            /* lock & get view information */
            IDE_TEST( qcm::getTableHandleByName(
                            QC_SMI_STMT( aStatement ),
                            sTableInfo->tableOwnerID,
                            (UChar *)sViewName,
                            (SInt)idlOS::strlen( sViewName ),
                            &sViewHandle,
                            &sViewSCN )
                      != IDE_SUCCESS );

            IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                                 sViewHandle,
                                                 sViewSCN,
                                                 SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                 SMI_TABLE_LOCK_X,
                                                 smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                                 ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                      != IDE_SUCCESS );

            IDE_TEST( smiGetTableTempInfo( sViewHandle,
                                           (void**)&sViewInfo )
                      != IDE_SUCCESS );

            /* drop mview view */
            IDE_TEST( qdd::deleteViewFromMeta( aStatement, sViewInfo->tableID )
                      != IDE_SUCCESS );

            IDE_TEST( qdpDrop::removePriv4DropTable( aStatement, sViewInfo->tableID )
                      != IDE_SUCCESS);

            /* PROJ-1888 INSTEAD OF TRIGGER */
            IDE_TEST( qdnTrigger::dropTrigger4DropTable( aStatement, sViewInfo )
                      != IDE_SUCCESS );

            IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                                aStatement,
                                sTableInfo->tableOwnerID,
                                sViewName,
                                ID_SIZEOF(sViewName),
                                QS_TABLE )
                      != IDE_SUCCESS );

            // PROJ-1073 Package
            IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                                aStatement,
                                sTableInfo->tableOwnerID,
                                sViewName,
                                ID_SIZEOF(sViewName),
                                QS_TABLE )
                      != IDE_SUCCESS );

            IDE_TEST( qcmView::setInvalidViewOfRelated(
                                aStatement,
                                sTableInfo->tableOwnerID,
                                sViewName,
                                ID_SIZEOF(sViewName),
                                QS_TABLE )
                      != IDE_SUCCESS );

            IDE_TEST( smiTable::dropTable( QC_SMI_STMT( aStatement ),
                                           sViewInfo->tableHandle,
                                           SMI_TBSLV_DROP_TBS )
                      != IDE_SUCCESS );

            /* drop materialized view */
            IDE_TEST( qcmMView::getMViewID(
                                QC_SMI_STMT( aStatement ),
                                sTableInfo->tableOwnerID,
                                sTableInfo->name,
                                idlOS::strlen( sTableInfo->name ),
                                &sMViewID )
                      != IDE_SUCCESS );

            IDE_TEST( qdd::deleteMViewFromMeta( aStatement, sMViewID )
                      != IDE_SUCCESS );

            /* make mview views list */
            IDU_LIMITPOINT( "qdtDrop::executeDropMViewOfRelated::malloc");
            IDE_TEST( QC_QMX_MEM( aStatement )->alloc(
                    ID_SIZEOF(qcmTableInfoList),
                    (void **)&sNewTableInfoList )
                != IDE_SUCCESS);

            sNewTableInfoList->tableInfo = sViewInfo;
            sNewTableInfoList->childInfo = NULL;
            sNewTableInfoList->next      = NULL;

            if ( sMViewInfoList == NULL )
            {
                sMViewInfoList = sNewTableInfoList;
            }
            else
            {
                sLastTableInfoList->next = sNewTableInfoList;
            }
            sLastTableInfoList = sNewTableInfoList;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aMViewOfRelatedInfoList = sMViewInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
