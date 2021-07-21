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
 * $Id: qdpDrop.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdpDrop.h>
#include <qcmUser.h>
#include <qcmPriv.h>
#include <qcg.h>
#include <qcuSqlSourceInfo.h>

IDE_RC qdpDrop::removePriv4DropUser(
    qcStatement * aStatement,
    UInt          aUserID)
{
/***********************************************************************
 *
 * Description :
 *      권한 정보 삭제
 *
 * Implementation :
 *      1. SYS_GRANT_SYSTEM_, SYS_GRANT_OBJECT_ 메타 테이블에서
 *         해당 유저에 관련된 권한 삭제
 *    
 ***********************************************************************/

#define IDE_FN "qdpDrop::removePriv4DropUser"

    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDU_LIMITPOINT("qdpDrop::removePriv4DropUser::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
        != IDE_SUCCESS);

    // remove grantee = aUserID from SYS_GRANT_SYSTEM_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_SYSTEM_ "
                     "WHERE GRANTEE_ID = INTEGER'%"ID_INT32_FMT"'",
                     aUserID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);

    // GRANT ANY PRIVILEGE 인 경우 grantor 삭제 되는 경우 삭제.
    // remove grantor = aUserID from SYS_GRANT_SYSTEM_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_SYSTEM_ "
                     "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"'",
                     aUserID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);
    
    // remove grantee = aUserID from SYS_GRANT_OBJECT_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE GRANTEE_ID = INTEGER'%"ID_INT32_FMT"'",
                     aUserID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);
    
    // remove grantor = aUserID from SYS_GRANT_OBJECT_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"'",
                     aUserID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpDrop::removePriv4DropTable(
    qcStatement * aStatement,
    qdpObjID      aTableID)
{
/***********************************************************************
 *
 * Description :
 *      DROP TABLE 시 해당 오브텍트에 대한 권한 정보 삭제
 *
 * Implementation :
 *      1. SYS_GRANT_OBJECT_ 메타 테이블에서 해당 테이블에 관련된 정보 삭제
 *    
 ***********************************************************************/

#define IDE_FN "qdpDrop::removePriv4DropTable"

    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDU_LIMITPOINT("qdpDrop::removePriv4DropTable::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
        != IDE_SUCCESS);

    // remove OBJ_ID = aTableID from SYS_GRANT_OBJECT_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE ( OBJ_TYPE = 'T' OR OBJ_TYPE = 'V' "
                     "OR OBJ_TYPE = 'S' ) "
                     "AND     OBJ_ID = "QCM_SQL_BIGINT_FMT, 
                     QCM_OID_TO_BIGINT( aTableID ) );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpDrop::removePriv4DropProc(
    qcStatement * aStatement,
    qdpObjID      aProcOID)
{
/***********************************************************************
 *
 * Description :
 *      DROP PROC 시 해당 오브텍트에 대한 권한 정보 삭제
 *
 * Implementation :
 *      1. SYS_GRANT_OBJECT_ 메타 테이블에서 해당 테이블에 관련된 정보 삭제
 *    
 ***********************************************************************/

#define IDE_FN "qdpDrop::removePriv4DropProc"

    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDU_LIMITPOINT("qdpDrop::removePriv4DropProc::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
        != IDE_SUCCESS);

    // remove OBJ_ID = aProcID from SYS_GRANT_OBJECT_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE OBJ_TYPE = 'P' AND OBJ_ID = "QCM_SQL_BIGINT_FMT, 
                     QCM_OID_TO_BIGINT( aProcOID ));

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// PROJ-1371 Directories
IDE_RC qdpDrop::removePriv4DropDirectory(
    qcStatement * aStatement,
    qdpObjID      aDirectoryOID)
{
/***********************************************************************
 *
 * Description :
 *      DROP DIRECTORY 시 해당 오브텍트에 대한 권한 정보 삭제
 *
 * Implementation :
 *      1. SYS_GRANT_OBJECT_ 메타 테이블에서 해당 테이블에 관련된 정보 삭제
 *    
 ***********************************************************************/

#define IDE_FN "qdpDrop::removePriv4DropDirectory"

    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDU_LIMITPOINT("qdpDrop::removePriv4DropDirectory::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
        != IDE_SUCCESS);

    // remove OBJ_ID = aDirectoryID from SYS_GRANT_OBJECT_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE OBJ_TYPE = 'D' AND OBJ_ID = "QCM_SQL_BIGINT_FMT, 
                     QCM_OID_TO_BIGINT( aDirectoryOID ));

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// PROJ-1073 Package
IDE_RC qdpDrop::removePriv4DropPkg(
    qcStatement * aStatement,
    qdpObjID      aPkgOID)
{
    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
        != IDE_SUCCESS);

    // remove OBJ_ID = aPkgID from SYS_GRANT_OBJECT_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE OBJ_TYPE = 'A' AND OBJ_ID = "QCM_SQL_BIGINT_FMT, 
                     QCM_OID_TO_BIGINT( aPkgOID ));

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
