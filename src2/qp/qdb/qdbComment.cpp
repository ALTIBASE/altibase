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
 * $Id: qdbComment.cpp 35065 2009-08-25 07:52:03Z junokun $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdbAlter.h>
#include <qtc.h>
#include <qcg.h>
#include <qcmCache.h>
#include <qcuSqlSourceInfo.h>
#include <qdpPrivilege.h>
#include <qdbComment.h>
#include <qcmSynonym.h>
#include <qdpRole.h>

const void * gQcmComments;
const void * gQcmCommentsIndex[ QCM_MAX_META_INDICES ];

IDE_RC qdbComment::validateCommentTable(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    COMMENT ON TABLE ... IS ... 의 validation 수행
 *
 * Implementation :
 *    1. 테이블 존재 여부 체크 (User Name 검증 포함)
 *    2. 테이블의 소유자가 SYSTEM_ 이면 comment 불가능
 *    3. 권한이 있는지 체크(ALTER object 혹은 ALTER ANY TABLE privilege)
 *    4. 주석 길이 검사
 *
 ***********************************************************************/
    qdCommentParseTree  * sParseTree  = NULL;
    idBool                sExist      = ID_FALSE;

    qcmSynonymInfo        sSynonymInfo;
    qcuSqlSourceInfo      sqlInfo;
    UInt                  sObjectType;
    void                * sObjectHandle = NULL;

    UInt                  sTableType;
    UInt                  sSequenceID;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );

    sParseTree = (qdCommentParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST(qcmSynonym::resolveObject(aStatement,
                                       sParseTree->userName,
                                       sParseTree->tableName,
                                       &sSynonymInfo,
                                       &sExist,
                                       &sParseTree->userID,
                                       &sObjectType,
                                       &sObjectHandle,
                                       &sSequenceID)
            != IDE_SUCCESS);

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
        IDE_RAISE(ERR_NOT_EXIST_OBJECT);
    }
    else
    {
        switch( sObjectType )
        {
            case QCM_OBJECT_TYPE_TABLE:
                // found the table
                sTableType = smiGetTableFlag(sObjectHandle) & SMI_TABLE_TYPE_MASK;
                
                // comment는 사용자가 생성한 table에만 가능하다.
                if( ( sTableType != SMI_TABLE_MEMORY ) &&
                    ( sTableType != SMI_TABLE_DISK ) &&
                    ( sTableType != SMI_TABLE_VOLATILE ) )
                {
                    sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                    IDE_RAISE(ERR_NOT_SUPPORT_OBJ);
                }
                break;
            case QCM_OBJECT_TYPE_SEQUENCE:
                sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                IDE_RAISE(ERR_NOT_SUPPORT_SEQ);
                break;
            case QCM_OBJECT_TYPE_PSM:
                sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                IDE_RAISE(ERR_NOT_SUPPORT_PSM);
                break;
            default:
                sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                IDE_RAISE(ERR_NOT_SUPPORT_OBJ);
                break;
        }
    }

    sParseTree->tableHandle = sObjectHandle;

    IDE_TEST( smiGetTableTempInfo( sParseTree->tableHandle,
                                   (void**)&sParseTree->tableInfo )
              != IDE_SUCCESS );
    
    sParseTree->tableSCN    = smiGetRowSCN( sParseTree->tableHandle );

    // Validation 위해 IS Lock
    IDE_TEST( qcm::lockTableForDDLValidation(aStatement,
                                             sParseTree->tableHandle,
                                             sParseTree->tableSCN)
              != IDE_SUCCESS );

    // Operatable Flag 검사
    if (QCM_IS_OPERATABLE_QP_COMMENT(
            sParseTree->tableInfo->operatableFlag )
        != ID_TRUE)
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
        IDE_RAISE(ERR_NOT_SUPPORT_OBJ);
    }
    else
    {
        // Nothing to do
    }

    // Alter Privilege 검사
    IDE_TEST( qdpRole::checkDDLAlterTablePriv( aStatement,
                                               sParseTree->tableInfo )
              != IDE_SUCCESS );
    
    // Literal 길이 검사 (4k)
    IDE_TEST_RAISE( sParseTree->comment.size > QC_MAX_COMMENT_LITERAL_LEN,
                    ERR_COMMENT_TOO_LONG );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_OBJECT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_SEQ);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_SEQUENCE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_PSM);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_PSM,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_OBJ);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_OBJ,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_COMMENT_TOO_LONG)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_COMMENT_TOO_LONG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::validateCommentColumn(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    COMMENT ON COLUMN ... IS ... 의 validation 수행
 *
 * Implementation :
 *    1. 테이블 존재 여부 체크 (User Name 검증 포함)
 *    2. 테이블의 소유자가 SYSTEM_ 이면 comment 불가능
 *    3. 권한이 있는지 체크(ALTER object 혹은 ALTER ANY TABLE privilege)
 *    4. 컬럼 존재 여부 체크
 *    5. 주석 길이 검사
 *
 ***********************************************************************/
    qdCommentParseTree  * sParseTree;
    idBool                sExist      = ID_FALSE;
    qcmColumn           * sColumn;

    qcmSynonymInfo        sSynonymInfo;
    qcuSqlSourceInfo      sqlInfo;
    UInt                  sObjectType;
    void                * sObjectHandle = NULL;

    UInt                  sTableType;
    UInt                  sSequenceID;

    // Parameter 검증
    IDE_DASSERT( aStatement != NULL );

    sParseTree = (qdCommentParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST(qcmSynonym::resolveObject(aStatement,
                                       sParseTree->userName,
                                       sParseTree->tableName,
                                       &sSynonymInfo,
                                       &sExist,
                                       &sParseTree->userID,
                                       &sObjectType,
                                       &sObjectHandle,
                                       &sSequenceID)
            != IDE_SUCCESS);

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
        IDE_RAISE(ERR_NOT_EXIST_OBJECT);
    }
    else
    {
        switch( sObjectType )
        {
            case QCM_OBJECT_TYPE_TABLE:
                // found the table
                sTableType = smiGetTableFlag(sObjectHandle) & SMI_TABLE_TYPE_MASK;

                // comment는 사용자가 생성한 table에만 가능하다.
                if( ( sTableType != SMI_TABLE_MEMORY ) &&
                    ( sTableType != SMI_TABLE_DISK ) &&
                    ( sTableType != SMI_TABLE_VOLATILE ) )
                {
                    sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                    IDE_RAISE(ERR_NOT_SUPPORT_OBJ);
                }
                break;
            case QCM_OBJECT_TYPE_SEQUENCE:
                sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                IDE_RAISE(ERR_NOT_SUPPORT_SEQ);
                break;
            case QCM_OBJECT_TYPE_PSM:
                sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                IDE_RAISE(ERR_NOT_SUPPORT_PSM);
                break;
            default:
                sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
                IDE_RAISE(ERR_NOT_SUPPORT_OBJ);
                break;
        }
    }

    sParseTree->tableHandle = sObjectHandle;

    IDE_TEST( smiGetTableTempInfo( sParseTree->tableHandle,
                                   (void**)&sParseTree->tableInfo )
              != IDE_SUCCESS );

    sParseTree->tableSCN    = smiGetRowSCN( sParseTree->tableHandle );

    // Validation 위해 IS Lock
    IDE_TEST( qcm::lockTableForDDLValidation(aStatement,
                                             sParseTree->tableHandle,
                                             sParseTree->tableSCN)
              != IDE_SUCCESS );

    // Operatable Flag 검사
    if (QCM_IS_OPERATABLE_QP_COMMENT(
            sParseTree->tableInfo->operatableFlag )
        != ID_TRUE)
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );
        IDE_RAISE(ERR_NOT_SUPPORT_OBJ);
    }
    else
    {
        // Nothing to do
    }

    // Alter Privilege 검사
    IDE_TEST( qdpRole::checkDDLAlterTablePriv( aStatement,
                                               sParseTree->tableInfo )
              != IDE_SUCCESS );

    // Column 존재 여부 검사
    IDE_TEST( qcmCache::getColumn( aStatement,
                                   sParseTree->tableInfo,
                                   sParseTree->columnName,
                                   &sColumn )
              != IDE_SUCCESS );

    // Literal 길이 검사 (4000)
    IDE_TEST_RAISE( sParseTree->comment.size > QC_MAX_COMMENT_LITERAL_LEN,
                    ERR_COMMENT_TOO_LONG );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_OBJECT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_SEQ);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_SEQUENCE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_PSM);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_PSM,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_OBJ);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMMENT_NOT_SUPPORT_OBJ,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_COMMENT_TOO_LONG)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_COMMENT_TOO_LONG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::executeCommentTable(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    COMMENT ON TABLE ... IS ... 수행
 *
 * Implementation :
 *    1. 테이블에 Lock 획득 (IS)
 *    2. Comment가 이미 존재하는지 체크
 *    3.   -> 존재하면 Update, 그렇지 않으면 Insert
 *    4. 메타에 반영
 *
 ***********************************************************************/
    qdCommentParseTree  * sParseTree   = NULL;
    SChar               * sSqlStr      = NULL;
    SChar               * sCommentStr  = NULL;
    idBool                sExist       = ID_FALSE;
    vSLong                sRowCnt      = 0;    

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );

    sParseTree = (qdCommentParseTree *)aStatement->myPlan->parseTree;

    // Table에 Lock 획득 (IS)
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_IS )
              != IDE_SUCCESS );    

    // Sql Query 공간 할당
    IDU_LIMITPOINT("qdbComment::executeCommentTable::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    // Comment 공간 할당
    IDU_LIMITPOINT("qdbComment::executeCommentTable::malloc2");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      sParseTree->comment.size+1,
                                      &sCommentStr )
              != IDE_SUCCESS );

    // Comment 내용 복사
    QC_STR_COPY( sCommentStr, sParseTree->comment );

    // Comment가 이미 존재하는지 체크
    IDE_TEST( existCommentTable( aStatement,
                                 sParseTree->tableInfo->tableOwnerName,
                                 sParseTree->tableInfo->name,
                                 &sExist)
              != IDE_SUCCESS );

    if (sExist == ID_TRUE)
    {
        // 이미 존재하면 Update
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COMMENTS_ "
                         "SET COMMENTS = VARCHAR'%s' "
                         "WHERE USER_NAME = VARCHAR'%s' "
                         "AND TABLE_NAME = VARCHAR'%s' "
                         "AND COLUMN_NAME IS NULL",
                         sCommentStr,
                         sParseTree->tableInfo->tableOwnerName,
                         sParseTree->tableInfo->name );
    }
    else
    {
        // 아직 존재하지 않으면 Insert
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_COMMENTS_ "
                         "( USER_NAME, TABLE_NAME, COLUMN_NAME, COMMENTS ) "
                         "VALUES ( VARCHAR'%s', VARCHAR'%s', NULL, VARCHAR'%s' )",
                         sParseTree->tableInfo->tableOwnerName,
                         sParseTree->tableInfo->name,
                         sCommentStr );
    }

    // 메타에 반영
    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbComment::executeCommentColumn(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    COMMENT ON COLUMN ... IS ... 수행
 *
 * Implementation :
 *    1. 테이블에 Lock 획득 (IS)
 *    2. Comment가 이미 존재하는지 체크
 *    3.   -> 존재하면 Update, 그렇지 않으면 Insert
 *    4. 메타에 반영
 *
 ***********************************************************************/
    qdCommentParseTree  * sParseTree   = NULL;
    SChar               * sSqlStr      = NULL;
    SChar               * sCommentStr  = NULL;
    SChar                 sColumnStr[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    idBool                sExist       = ID_FALSE;
    vSLong                sRowCnt      = 0;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );

    sParseTree = (qdCommentParseTree *)aStatement->myPlan->parseTree;

    // Table에 Lock 획득 (IS)
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_IS )
              != IDE_SUCCESS );
    
    // Sql Query 공간 할당
    IDU_LIMITPOINT("qdbComment::executeCommentColumn::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    // Comment 공간 할당
    IDU_LIMITPOINT("qdbComment::executeCommentColumn::malloc2");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      sParseTree->comment.size+1,
                                      &sCommentStr )
              != IDE_SUCCESS );

    // Comment 내용 복사
    QC_STR_COPY( sCommentStr, sParseTree->comment );

    // Column 내용 복사
    QC_STR_COPY( sColumnStr, sParseTree->columnName );

    // Comment가 이미 존재하는지 체크
    IDE_TEST( existCommentColumn( aStatement,
                                  sParseTree->tableInfo->tableOwnerName,
                                  sParseTree->tableInfo->name,
                                  sColumnStr,
                                  &sExist)
              != IDE_SUCCESS );

    if (sExist == ID_TRUE)
    {
        // 이미 존재하면 Update
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COMMENTS_ "
                         "SET COMMENTS = VARCHAR'%s' "
                         "WHERE USER_NAME = VARCHAR'%s' "
                         "AND TABLE_NAME = VARCHAR'%s' "
                         "AND COLUMN_NAME = VARCHAR'%s'",
                         sCommentStr,
                         sParseTree->tableInfo->tableOwnerName,
                         sParseTree->tableInfo->name,
                         sColumnStr );
    }
    else
    {
        // 아직 존재하지 않으면 Insert
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_COMMENTS_ "
                         "( USER_NAME, TABLE_NAME, COLUMN_NAME, COMMENTS ) "
                         "VALUES ( VARCHAR'%s', VARCHAR'%s', VARCHAR'%s', VARCHAR'%s' )",
                         sParseTree->tableInfo->tableOwnerName,
                         sParseTree->tableInfo->name,
                         sColumnStr,
                         sCommentStr );
    }

    // 메타에 반영
    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::updateCommentTable( qcStatement  * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aOldTableName,
                                       SChar        * aNewTableName)
{
/***********************************************************************
 *
 * Description :
 *    Comment 구문이 아닌 다른 구문에 의해 Table(View)의 이름이 바뀔경우
 *    Comment 메타의 해당 Object 이름을 변경
 *
 * Implementation :
 *    1. Comment의 Table 이름을 변경
 *
 ***********************************************************************/
    vSLong              sRowCnt        = 0;
    SChar             * sSqlStr        = NULL;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aUserName      != NULL );
    IDE_DASSERT( aOldTableName  != NULL );
    IDE_DASSERT( aNewTableName  != NULL );
    
    IDE_DASSERT( *aUserName     != '\0' );
    IDE_DASSERT( *aOldTableName != '\0' );
    IDE_DASSERT( *aNewTableName != '\0' );

    // 메타 변경
    IDU_LIMITPOINT("qdbComment::updateCommentTable::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    // Table Comment와 Column Comment 모두 수정하는 쿼리
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COMMENTS_ "
                     "SET TABLE_NAME = VARCHAR'%s' "
                     "WHERE USER_NAME = VARCHAR'%s' "
                     "AND TABLE_NAME = VARCHAR'%s' ",
                     aNewTableName,
                     aUserName,
                     aOldTableName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    // Table Comment, Column Comment 삭제 시는
    // 0건일수도 있고 (Comment 가 아직 달리기 전)
    // 여러 건일수도 있다. (Table Comment와 Column Comment가 여러 건)
    // 그래서 sRowCnt는 검사하지 않는다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::updateCommentColumn( qcStatement  * aStatement,
                                        SChar        * aUserName,
                                        SChar        * aTableName,
                                        SChar        * aOldColumnName,
                                        SChar        * aNewColumnName)
{
/***********************************************************************
 *
 * Description :
 *    Comment 구문이 아닌 다른 구문에 의해 Column의 이름이 바뀔경우
 *    Comment 메타의 해당 Object 이름을 변경
 *
 * Implementation :
 *    1. Comment의 Column 이름을 변경
 *
 ***********************************************************************/
    vSLong              sRowCnt        = 0;
    SChar             * sSqlStr        = NULL;

    // Parameter 검증
    IDE_DASSERT( aStatement      != NULL );
    IDE_DASSERT( aUserName       != NULL );
    IDE_DASSERT( aTableName      != NULL );
    IDE_DASSERT( aOldColumnName  != NULL );
    IDE_DASSERT( aNewColumnName  != NULL );
    
    IDE_DASSERT( *aUserName      != '\0' );
    IDE_DASSERT( *aTableName     != '\0' );
    IDE_DASSERT( *aNewColumnName != '\0' );
    IDE_DASSERT( *aNewColumnName != '\0' );

    // 메타 변경
    IDU_LIMITPOINT("qdbComment::updateCommentColumn::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    // Column Comment 수정하는 쿼리
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COMMENTS_ "
                     "SET COLUMN_NAME = VARCHAR'%s' "
                     "WHERE USER_NAME = VARCHAR'%s' "
                     "AND TABLE_NAME = VARCHAR'%s' "
                     "AND COLUMN_NAME = VARCHAR'%s' ",
                     aNewColumnName,
                     aUserName,
                     aTableName,
                     aOldColumnName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    // 삭제된 Column Comment가 1건보다 많다면 메타 비정상임
    IDE_TEST_RAISE( sRowCnt > 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::deleteCommentTable( qcStatement  * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aTableName)
{
/***********************************************************************
 *
 * Description :
 *    Table이 삭제 될 경우 대응하는 Comment를 삭제
 *
 * Implementation :
 *    1. 해당 Object에 속하는 Table Comment, Column Comment 삭제
 *
 ***********************************************************************/
    vSLong              sRowCnt        = 0;
    SChar             * sSqlStr        = NULL;
    
    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aUserName      != NULL );
    IDE_DASSERT( aTableName     != NULL );
    
    IDE_DASSERT( *aUserName     != '\0' );
    IDE_DASSERT( *aTableName    != '\0' );

    // 메타 변경
    IDU_LIMITPOINT("qdbComment::deleteCommentTable::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    // Table Comment, Column Comment 삭제하는 쿼리
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_COMMENTS_ "
                     "WHERE USER_NAME = VARCHAR'%s' "
                     "AND TABLE_NAME = VARCHAR'%s' ",
                     aUserName,
                     aTableName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    // Table Comment, Column Comment 삭제 시는
    // 0건일수도 있고 (Comment 가 아직 달리기 전)
    // 여러 건일수도 있다. (Table Comment와 Column Comment가 여러 건)
    // 그래서 sRowCnt는 검사하지 않는다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbComment::deleteCommentColumn( qcStatement  * aStatement,
                                        SChar        * aUserName,
                                        SChar        * aTableName,
                                        SChar        * aColumnName)
{
/***********************************************************************
 *
 * Description :
 *    Column이 삭제 될 경우 대응하는 Comment를 삭제
 *
 * Implementation :
 *    1. 해당 Column에 속하는 Comment 삭제
 *
 ***********************************************************************/
    vSLong              sRowCnt        = 0;
    SChar             * sSqlStr        = NULL;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aUserName      != NULL );
    IDE_DASSERT( aTableName     != NULL );
    IDE_DASSERT( aColumnName    != NULL );
    
    IDE_DASSERT( *aUserName     != '\0' );
    IDE_DASSERT( *aTableName    != '\0' );
    IDE_DASSERT( *aColumnName   != '\0' );

    // 메타 변경
    IDU_LIMITPOINT("qdbComment::deleteCommentColumn::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    // Column Comment 삭제하는 쿼리
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_COMMENTS_ "
                     "WHERE USER_NAME = VARCHAR'%s' "
                     "AND TABLE_NAME = VARCHAR'%s' "
                     "AND COLUMN_NAME = VARCHAR'%s' ",
                     aUserName,
                     aTableName,
                     aColumnName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    // 삭제된 Column Comment가 1건보다 많다면 메타 비정상임
    IDE_TEST_RAISE( sRowCnt > 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::existCommentTable( qcStatement    * aStatement,
                                      SChar          * aUserName,
                                      SChar          * aTableName,
                                      idBool         * aExist)
{
/***********************************************************************
 *
 * Description : Check Table Comment is exists
 *
 * Implementation :
 *     1. set search condition user name
 *     2. set search condition table name
 *     3. set search condition column name (set NULL)
 *     4. select comment from SYS_COMMENTS_ meta
 *
 ***********************************************************************/
    vSLong              sRowCount;

    smiRange            sRange;
    qtcMetaRangeColumn  sFirstMetaRange;
    qtcMetaRangeColumn  sSecondMetaRange;
    qtcMetaRangeColumn  sThirdMetaRange;

    qcNameCharBuffer    sUserNameBuffer;
    qcNameCharBuffer    sTableNameBuffer;
    mtdCharType       * sUserName;
    mtdCharType       * sTableName;
    mtcColumn         * sFirstMtcColumn;
    mtcColumn         * sSceondMtcColumn;
    mtcColumn         * sThirdMtcColumn;

    IDE_DASSERT( aStatement  != NULL );
    IDE_DASSERT( aUserName   != NULL );
    IDE_DASSERT( aTableName  != NULL );

    IDE_DASSERT( *aUserName  != '\0' );
    IDE_DASSERT( *aTableName != '\0' );

    // 초기화
    
    sUserName   = (mtdCharType *) &sUserNameBuffer;
    sTableName  = (mtdCharType *) &sTableNameBuffer;
    
    qtc::setVarcharValue( sUserName,
                          NULL,
                          aUserName,
                          idlOS::strlen(aUserName) );
    qtc::setVarcharValue( sTableName,
                          NULL,
                          aTableName,
                          idlOS::strlen(aTableName) );

    // 검색 조건
    qtc::initializeMetaRange( &sRange,
                              MTD_COMPARE_MTDVAL_MTDVAL );  // Meta is memory table

    IDE_TEST( smiGetTableColumns( gQcmComments,
                                  QCM_COMMENTS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    qtc::setMetaRangeColumn( &sFirstMetaRange,
                             sFirstMtcColumn,
                             (const void*)sUserName,
                             SMI_COLUMN_ORDER_ASCENDING,
                             0 );  // First column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sFirstMetaRange );

    IDE_TEST( smiGetTableColumns( gQcmComments,
                                  QCM_COMMENTS_TABLE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    qtc::setMetaRangeColumn( &sSecondMetaRange,
                             sSceondMtcColumn,
                             (const void*)sTableName,
                             SMI_COLUMN_ORDER_ASCENDING,
                             1 );  // Second column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sSecondMetaRange );

    IDE_TEST( smiGetTableColumns( gQcmComments,
                                  QCM_COMMENTS_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    qtc::setMetaRangeIsNullColumn(
        &sThirdMetaRange,
        sThirdMtcColumn,
        2 );  // Third column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sThirdMetaRange );

    qtc::fixMetaRange( &sRange );

    // 조회
    IDE_TEST( qcm::selectCount( QC_SMI_STMT(aStatement),
                                gQcmComments,
                                &sRowCount,
                                smiGetDefaultFilter(),
                                &sRange,
                                gQcmCommentsIndex[QCM_COMMENTS_IDX1_ORDER]
                              )
              != IDE_SUCCESS );

    if (sRowCount == 0)
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::existCommentColumn( qcStatement    * aStatement,
                                       SChar          * aUserName,
                                       SChar          * aTableName,
                                       SChar          * aColumnName,
                                       idBool         * aExist)
{
/***********************************************************************
 *
 * Description : Check Table Comment is exists
 *
 * Implementation :
 *     1. set search condition user name
 *     2. set search condition table name
 *     3. set search condition column name
 *     4. select comment from SYS_COMMENTS_ meta
 *
 ***********************************************************************/
    vSLong              sRowCount;

    smiRange            sRange;
    qtcMetaRangeColumn  sFirstMetaRange;
    qtcMetaRangeColumn  sSecondMetaRange;
    qtcMetaRangeColumn  sThirdMetaRange;

    qcNameCharBuffer    sUserNameBuffer;
    qcNameCharBuffer    sTableNameBuffer;
    qcNameCharBuffer    sColumnNameBuffer;
    mtdCharType       * sUserName;
    mtdCharType       * sTableName;
    mtdCharType       * sColumnName;
    mtcColumn         * sFirstMtcColumn;
    mtcColumn         * sSceondMtcColumn;
    mtcColumn         * sThirdMtcColumn;

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aUserName    != NULL );
    IDE_DASSERT( aTableName   != NULL );
    IDE_DASSERT( aColumnName  != NULL );
    
    IDE_DASSERT( *aUserName   != '\0' );
    IDE_DASSERT( *aTableName  != '\0' );
    IDE_DASSERT( *aColumnName != '\0' );

    // 초기화
    
    sUserName   = (mtdCharType *) &sUserNameBuffer;
    sTableName  = (mtdCharType *) &sTableNameBuffer;
    sColumnName = (mtdCharType *) &sColumnNameBuffer;
    
    qtc::setVarcharValue( sUserName,
                          NULL,
                          aUserName,
                          idlOS::strlen(aUserName) );
    qtc::setVarcharValue( sTableName,
                          NULL,
                          aTableName,
                          idlOS::strlen(aTableName) );
    qtc::setVarcharValue( sColumnName,
                          NULL,
                          aColumnName,
                          idlOS::strlen(aColumnName) );

    // 검색 조건
    qtc::initializeMetaRange( &sRange,
                              MTD_COMPARE_MTDVAL_MTDVAL );  // Meta is memory table

    IDE_TEST( smiGetTableColumns( gQcmComments,
                                  QCM_COMMENTS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    qtc::setMetaRangeColumn( &sFirstMetaRange,
                             sFirstMtcColumn,
                             (const void*)sUserName,
                             SMI_COLUMN_ORDER_ASCENDING,
                             0 );  // First column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sFirstMetaRange );

    IDE_TEST( smiGetTableColumns( gQcmComments,
                                  QCM_COMMENTS_TABLE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    qtc::setMetaRangeColumn( &sSecondMetaRange,
                             sSceondMtcColumn,
                             (const void*)sTableName,
                             SMI_COLUMN_ORDER_ASCENDING,
                             1 );  // Second column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sSecondMetaRange );

    IDE_TEST( smiGetTableColumns( gQcmComments,
                                  QCM_COMMENTS_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    qtc::setMetaRangeColumn( &sThirdMetaRange,
                             sThirdMtcColumn,
                             (const void*)sColumnName,
                             SMI_COLUMN_ORDER_ASCENDING,
                             2 );  // Third column of Index
    
    qtc::addMetaRangeColumn( &sRange,
                             &sThirdMetaRange );

    qtc::fixMetaRange( &sRange );

    // 조회
    IDE_TEST( qcm::selectCount( QC_SMI_STMT(aStatement),
                                gQcmComments,
                                &sRowCount,
                                smiGetDefaultFilter(),
                                &sRange,
                                gQcmCommentsIndex[QCM_COMMENTS_IDX1_ORDER]
                              )
              != IDE_SUCCESS );

    if (sRowCount == 0)
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbComment::copyCommentsMeta( qcStatement  * aStatement,
                                     qcmTableInfo * aSourceTableInfo,
                                     qcmTableInfo * aTargetTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      Comment를 복사한다. (Meta Table)
 *          - SYS_COMMENTS_
 *              - TABLE_NAME을 Target Table Name으로 지정한다.
 *              - Hidden Column이면 Function-based Index의 Column이므로, Hidden Column Name을 변경한다.
 *                  - Hidden Column Name에 Prefix를 붙인다.
 *                      - Hidden Column Name = Index Name + $ + IDX + Number
 *              - 나머지는 그대로 복사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn * sSourceColumn = NULL;
    qcmColumn * sTargetColumn = NULL;
    SChar     * sSqlStr       = NULL;
    vSLong      sRowCnt       = ID_vLONG(0);

    IDU_FIT_POINT( "qdbComment::copyCommentsMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    /* Table Comment */
    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_COMMENTS_ SELECT "
                     "USER_NAME, "                      // USER_NAME
                     "VARCHAR'%s', "                    // TABLE_NAME
                     "NULL, "                           // COLUMN_NAME
                     "COMMENTS "                        // COMMENTS
                     "FROM SYS_COMMENTS_ WHERE "
                     "USER_NAME = VARCHAR'%s' AND "     // USER_NAME
                     "TABLE_NAME = VARCHAR'%s' AND "    // TABLE_NAME
                     "COLUMN_NAME IS NULL ",
                     aTargetTableInfo->name,
                     aSourceTableInfo->tableOwnerName,
                     aSourceTableInfo->name );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    /* Column Comment */
    for ( sSourceColumn = aSourceTableInfo->columns, sTargetColumn = aTargetTableInfo->columns;
          ( sSourceColumn != NULL ) && ( sTargetColumn != NULL );
          sSourceColumn = sSourceColumn->next, sTargetColumn = sTargetColumn->next )
    {
        // Target Column에 Hidden Column Prefix를 이미 적용했다.
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_COMMENTS_ SELECT "
                         "USER_NAME, "                      // USER_NAME
                         "VARCHAR'%s', "                    // TABLE_NAME
                         "VARCHAR'%s', "                    // COLUMN_NAME
                         "COMMENTS "                        // COMMENTS
                         "FROM SYS_COMMENTS_ WHERE "
                         "USER_NAME = VARCHAR'%s' AND "     // USER_NAME
                         "TABLE_NAME = VARCHAR'%s' AND "    // TABLE_NAME
                         "COLUMN_NAME = VARCHAR'%s' ",      // COLUMN_NAME
                         aTargetTableInfo->name,
                         sTargetColumn->name,
                         aSourceTableInfo->tableOwnerName,
                         aSourceTableInfo->name,
                         sSourceColumn->name );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

