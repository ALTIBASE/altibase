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
 * $Id: qdcDir.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdcDir.h>
#include <qcm.h>
#include <qcmUser.h>
#include <qdpPrivilege.h>
#include <qcmDirectory.h>
#include <qcuSqlSourceInfo.h>
#include <qdpDrop.h>
#include <qdpGrant.h>
#include <qdpRevoke.h>
#include <qcg.h>
#include <qdpRole.h>

IDE_RC qdcDir::validateCreate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     create directory에 대한 validation
 *
 * Implementation :
 *     1. replace라면 directory가 이미 존재하는지 검사
 *     1.1 directory가 없다면 replace를 FALSE로 세팅
 *     2. replace가 아니라면 directory가 이미 존재하는지 검사
 *     2.1 directory가 이미 존재한다면 에러
 *     3. 권한 검사
 *
 ***********************************************************************/
#define IDE_FN "qdcDir::validateCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qdDirectoryParseTree * sParseTree;
    qcmDirectoryInfo       sDirectoryInfo;
    idBool                 sExist;
    
    sParseTree = (qdDirectoryParseTree *)aStatement->myPlan->parseTree;

    sParseTree->userID = QCG_GET_SESSION_USER_ID(aStatement);

    IDE_TEST( qcmDirectory::getDirectory(
                             aStatement,
                             sParseTree->directoryName,
                             idlOS::strlen(sParseTree->directoryName),
                             &sDirectoryInfo,
                             &sExist )
              != IDE_SUCCESS );

    
    if( sParseTree->replace == ID_TRUE )
    {    
        if( sExist == ID_TRUE )
        {
            // replace의 경우 같은 user일 때만 가능?
        }
        else
        {
            sParseTree->replace = ID_FALSE;
        }
    }
    else
    {
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXISTS_DIRECTORY );
    }

    // 권한 검사해야 함
    IDE_TEST( qdpRole::checkDDLCreateDirectoryPriv( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXISTS_DIRECTORY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC qdcDir::executeCreate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     create /replace directory의 execution
 *
 * Implementation :
 *     1. qcmDirectory::addMetaInfo함수 호출
 *     2. 생성자의 권한을 삭제하고 새로 부여
 *
 *
 ***********************************************************************/
#define IDE_FN "qdcDir::executeCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qdDirectoryParseTree * sParseTree;
    qcmDirectoryInfo       sDirectoryInfo;
    idBool                 sExist;
    SChar                * sSqlStr;
    vSLong                 sRowCnt;
    
    sParseTree = (qdDirectoryParseTree *)aStatement->myPlan->parseTree;

    // To fix BUG-13035 생성 user는 무조건 sys가 된다.
    IDE_TEST( qcmDirectory::addMetaInfo( aStatement,
                                         QC_SYS_USER_ID,
                                         sParseTree->directoryName,
                                         sParseTree->directoryPath,
                                         sParseTree->replace )
              != IDE_SUCCESS );

    if( sParseTree->userID != QC_SYS_USER_ID )
    {
        // directory 정보를 가져옴
        IDE_TEST( qcmDirectory::getDirectory(
                             aStatement,
                             sParseTree->directoryName,
                             idlOS::strlen(sParseTree->directoryName),
                             &sDirectoryInfo,
                             &sExist )
              != IDE_SUCCESS );
        
        IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
        != IDE_SUCCESS);

        // with grant option이 있으면 삭제
        // 또는 grantor가 sys이면 삭제
        // delete from sys_grant_objects_ where
        //     ( objid = X and objtype = X and userid = X and grantee = X )
        //       and ( grantor = SYS or with_grant_option = 1 );
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM  SYS_GRANT_OBJECT_ WHERE "
                         "GRANTEE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                         "USER_ID = INTEGER'%"ID_INT32_FMT"' AND "
                         "OBJ_ID = BIGINT'%"ID_INT64_FMT"' AND "
                         "OBJ_TYPE = VARCHAR'%s' AND "
                         " ( "
                         "GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' OR "
                         "WITH_GRANT_OPTION = INTEGER'%"ID_INT32_FMT"' )",
                         sParseTree->userID,
                         QC_SYS_USER_ID,
                         QCM_OID_TO_BIGINT(sDirectoryInfo.directoryID),
                         "D",
                         QC_SYS_USER_ID,
                         QCM_WITH_GRANT_OPTION_TRUE );

        IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),  
                                    sSqlStr,
                                    & sRowCnt ) != IDE_SUCCESS);

        // 권한 부여
        // insert into sys_grant_objects_ values( ..read 권한 + with grant option );
        // insert into sys_grant_objects_ values( ..write 권한 + with grant option );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_GRANT_OBJECT_ VALUES ( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "VARCHAR'%s', "
                         "INTEGER'%"ID_INT32_FMT"')",
                         QC_SYS_USER_ID,
                         sParseTree->userID,
                         QCM_PRIV_ID_DIRECTORY_READ_NO,
                         QC_SYS_USER_ID,
                         QCM_OID_TO_BIGINT( sDirectoryInfo.directoryID ),
                         "D",
                         QCM_WITH_GRANT_OPTION_TRUE );
        
        IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                    sSqlStr,
                                    & sRowCnt ) != IDE_SUCCESS);
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_GRANT_OBJECT_ VALUES ( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "VARCHAR'%s', "
                         "INTEGER'%"ID_INT32_FMT"')",
                         QC_SYS_USER_ID,
                         sParseTree->userID,
                         QCM_PRIV_ID_DIRECTORY_WRITE_NO,
                         QC_SYS_USER_ID,
                         QCM_OID_TO_BIGINT( sDirectoryInfo.directoryID ),
                         "D",
                         QCM_WITH_GRANT_OPTION_TRUE );
        
        IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                    sSqlStr,
                                    & sRowCnt ) != IDE_SUCCESS);
    }
    else
    {
        // SYS유저가 생성한 디렉토리에 대해서는 권한을 부여하지 않는다.
        // SYS자신이 OWNER이기 때문이다.
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC qdcDir::validateDrop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     drop directory의 validation
 *
 * Implementation :
 *     1. directory가 존재하지 않으면 에러
 *     2. 권한 검사
 *     3. directory OID세팅
 *
 ***********************************************************************/
#define IDE_FN "qdcDir::validateDrop"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdDirectoryParseTree * sParseTree;
    idBool                 sExist;
    qcmDirectoryInfo       sDirectoryInfo;
    qcuSqlSourceInfo       sqlInfo;

    sParseTree = (qdDirectoryParseTree *)aStatement->myPlan->parseTree;

    sParseTree->userID = QCG_GET_SESSION_USER_ID(aStatement);

    IDE_TEST( qcmDirectory::getDirectory(
                               aStatement,
                               sParseTree->directoryName,
                               idlOS::strlen(sParseTree->directoryName),
                               &sDirectoryInfo,
                               &sExist )
              != IDE_SUCCESS );

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->directoryNamePos );
        IDE_RAISE( ERR_NOT_EXISTS );
    }
    else
    {
        IDE_TEST( qdpRole::checkDDLDropDirectoryPriv( aStatement,
                                                      sDirectoryInfo.userID )
                  != IDE_SUCCESS );

        sParseTree->directoryOID = (qdpObjID)sDirectoryInfo.directoryID;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXISTS);
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdcDir::executeDrop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     drop directory의 execution
 *
 * Implementation :
 *     1. qcmDirectory::delMetaInfoByDirectoryName함수 호출
 *     2. 관련 권한 삭제
 *
 ***********************************************************************/
#define IDE_FN "qdcDir::executeDrop"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qdDirectoryParseTree * sParseTree;
    
    sParseTree = (qdDirectoryParseTree *)aStatement->myPlan->parseTree;
    
    IDE_TEST( qcmDirectory::delMetaInfoByDirectoryName(
                                        aStatement,
                                        sParseTree->directoryName )
              != IDE_SUCCESS );
        
    IDE_TEST( qdpDrop::removePriv4DropDirectory(
                                        aStatement,
                                        sParseTree->directoryOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

