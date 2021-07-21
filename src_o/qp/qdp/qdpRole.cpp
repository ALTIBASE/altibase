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
 * $Id$
 **********************************************************************/

#include <ide.h>
#include <qcg.h>
#include <qcmUser.h>
#include <qdpPrivilege.h>
#include <qcmTableSpace.h>
#include <qdpRole.h>
#include <qcgPlan.h>
#include <qdpDrop.h>

/***********************************************************************
 *
 * Description :
 *    CREATE ROLE ... 의 validation 수행
 *
 * Implementation :
 *    1. CREATE ROLE 권한이 있는지 체크
 *    2. 같은 이름의 사용자가 이미 있는지 체크
 *
 ***********************************************************************/
IDE_RC qdpRole::validateCreateRole( qcStatement * aStatement )
{
    qdUserParseTree     * sParseTree;
    UInt                  sUserID;
    qdUserType            sUserType;

    sParseTree = (qdUserParseTree *)aStatement->myPlan->parseTree;

    /* Default Role을 지원하지 않아 패스워드는 없다. */
    IDE_DASSERT( QC_IS_NULL_NAME( sParseTree->password ) == ID_TRUE );    

    // check grant
    IDE_TEST( qdpRole::checkDDLCreateRolePriv( aStatement )
              != IDE_SUCCESS );
    
    /* if same user exists, then ERR_DUPLCATED_USER */
    IDE_TEST_RAISE( qcmUser::getUserIDAndType( aStatement,
                                               sParseTree->userName,
                                               &sUserID,
                                               &sUserType )
                    == IDE_SUCCESS, ERR_DUPLICATED_USER );

    sParseTree->dataTBSID = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
    sParseTree->tempTBSID = SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUPLICATED_USER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDR_DUPLICATE_USER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    DROP ROLE ... 의 validation 수행
 *
 * Implementation :
 *    1. ROLE 인지 체크 
 *    2. DROP ANY ROLE 권한이 있는지 체크
 *
 ***********************************************************************/
IDE_RC qdpRole::validateDropRole( qcStatement * aStatement )
{
    qdDropParseTree    * sParseTree;
    qcuSqlSourceInfo     sqlInfo;
    qdUserType           sUserType;
    
    sParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;

    /* if specified user doesn't exist, the\n error */
    if ( qcmUser::getUserIDAndType( aStatement,
                                    sParseTree->objectName,
                                    &sParseTree->userID,
                                    &sUserType )
         != IDE_SUCCESS )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->objectName );
        IDE_RAISE( ERR_NOT_EXIST_ROLE );
    }
    else
    {
        /* Nothing To Do */
    }
    
    if ( sUserType != QDP_ROLE_TYPE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->objectName );
        IDE_RAISE( ERR_NOT_EXIST_ROLE );
    }
    else
    {
        /* Nothing To Do */
    }

    /* check grant DROP ANY ROLE */
    IDE_TEST( qdpRole::checkDDLDropAnyRolePriv( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_ROLE );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_ROLE,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *      CREATE ROLE 문의 execution 수행 함수
 *
 * Implementation :
 *      1. 새로운 사용자를 위한 user ID를 부여
 *      2. SYS_USERS_ 메타 테이블에 사용자 정보 입력
 *
 ***********************************************************************/
IDE_RC qdpRole::executeCreateRole( qcStatement * aStatement )
{
    qdUserParseTree     * sParseTree;
    UInt                  sUserID;
    UChar                 sUserName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar               * sSqlStr;
    vSLong                sRowCnt;

    sParseTree = (qdUserParseTree *)aStatement->myPlan->parseTree;

    QC_STR_COPY( sUserName, sParseTree->userName );    
    
    IDE_TEST( qcmUser::getNextUserID( aStatement, &sUserID )
              != IDE_SUCCESS);

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);
    
    // BUG-41230 SYS_USERS_ => DBA_USERS_
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO DBA_USERS_ VALUES( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "VARCHAR'%s', "
                     "NULL, "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "CHAR'N', "                 // ACCOUNT_LOCK
                     "NULL, "                    // ACCOUNT_LOCK_DATE
                     "CHAR'F', "                 // PASSWORD_LIMIT_FLAG
                     "INTEGER'0', "              // FAILED_LOGIN_ATTEMPTS
                     "INTEGER'0', "              // FAILED_LOGIN_COUNT
                     "INTEGER'0', "              // PASSWORD_LOCK_TIME
                     "NULL, "                    // PASSWORD_EXPIRY_DATE
                     "INTEGER'0', "              // PASSWORD_LIFE_TIME
                     "INTEGER'0', "              // PASSWORD_GRACE_TIME
                     "NULL, "                    // PASSWORD_REUSE_DATE
                     "INTEGER'0', "              // PASSWORD_REUSE_TIME
                     "INTEGER'0', "              // PASSWORD_REUSE_MAX
                     "INTEGER'0', "              // PASSWORD_REUSE_COUNT
                     "VARCHAR'', "               // PASSWORD_VERIFY_FUNCTION
                     "CHAR'R', "                 // USER_TYPE
                     "NULL, "                    // DISABLE_TCP
                     "SYSDATE, SYSDATE )",
                     sUserID,
                     sUserName,
                     (mtdIntegerType) sParseTree->dataTBSID,
                     (mtdIntegerType) sParseTree->tempTBSID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    DROP ROLE 수행
 *
 * Implementation :
 *    1. USER 존재 여부 검사
 *    4. SYS_USERS_ 메타 테이블에서 삭제
 *    5. 관련 권한 정보 삭제
 *    - SYS_USERS_, SYS_USER_ROLES_, SYS_GRANT_SYSTEM_, SYS_GRANT_OBJECT_
 *
 ***********************************************************************/
IDE_RC qdpRole::executeDropRole( qcStatement * aStatement )
{
    qdDropParseTree    * sParseTree;
    SChar              * sSqlStr;
    vSLong               sRowCnt;

    sParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;
    
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                     SChar,
                                     QD_MAX_SQL_LENGTH,
                                     &sSqlStr )
              != IDE_SUCCESS);

    // BUG-41230 SYS_USERS_ => DBA_USERS_
    /* SYS_USERS_ ROLE */
    idlOS::snprintf( (SChar*)sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM DBA_USERS_ "
                     "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"'",
                     sParseTree->userID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 (SChar*)sSqlStr, &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    /* PROJ-1812 ROLE
     * remove roleid = aUserID from SYS_USER_ROLES_ */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_USER_ROLES_ "
                     "WHERE ROLE_ID = INTEGER'%"ID_INT32_FMT"'",
                     sParseTree->userID );

    IDE_TEST( qcg::runDMLforDDL(QC_SMI_STMT( aStatement ),
                                (SChar*)sSqlStr, &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST( qdpDrop::removePriv4DropUser( aStatement, sParseTree->userID )
              != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *       ROLE을 부여 받은 GRANTEE LIST (USER) 를 생성한다.
 *       reference, cascade constraint 에 user를 이용하여 참조 무결성
 *       제약 조건을 삭제 할때 사용 된다.
 *
 * Implementation :
 *       SELECT * FROM SYS_USER_ROLES_
 *       WHERE ROLE_ID = aRoleID;
 *
 ***********************************************************************/
IDE_RC qdpRole::getGranteeListByRoleID( qcStatement              * aStatement,
                                        UInt                       aRoleID,
                                        qdReferenceGranteeList  ** aUserList )
{
    smiRange                  sRange;
    qtcMetaRangeColumn        sFirstMetaRange;
    mtdIntegerType            sIntDataOfRoleID;    
    smiTableCursor            sCursor;
    SInt                      sStage = 0;
    const void              * sRow = NULL;
    smiCursorProperties       sCursorProperty;
    scGRID                    sRid;
    mtcColumn               * sGranteeIDColumn;
    UInt                      sGranteeID;
    qdReferenceGranteeList  * sUserList;
    mtcColumn               * sFirstMtcColumn;

    *aUserList = NULL;
    
    IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                  QCM_USER_ROLES_GRANTEE_ID_COL_ORDER,
                                  (const smiColumn**)&sGranteeIDColumn )
              != IDE_SUCCESS );
    
    sIntDataOfRoleID = (mtdIntegerType) aRoleID;

    IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                  QCM_USER_ROLES_ROLE_ID_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sFirstMetaRange,
        sFirstMtcColumn,
        (const void *) &sIntDataOfRoleID,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmUserRoles,
                  gQcmUserRolesIndex[QCM_USER_ROLES_IDX3_ORDER],
                  smiGetRowSCN( gQcmUserRoles ),
                  NULL,
                  & sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  & sCursorProperty )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    
    while ( sRow != NULL )
    {
        sGranteeID = *(UInt *) ( (UChar*)sRow +
                                 sGranteeIDColumn->column.offset );

        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF( qdReferenceGranteeList ),
                                                 (void**)&sUserList )
                  != IDE_SUCCESS);

        sUserList->userID = sGranteeID;
        sUserList->next = NULL;
        
        if ( *aUserList == NULL )
        {
            *aUserList = sUserList;
        }
        else
        {
            sUserList->next = *aUserList;
            *aUserList = sUserList;
        }
            
        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( sStage != 0 )
    {
        (void) sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }  

    return IDE_FAILURE;
    
}

/***********************************************************************
 *
 * Description :
 *    사용 자가 부여 받은 role의 개수 구한다.
 *
 * Implementation :
 *     SELECT * FROM SYSTEM_.SYS_USER_ROLES_
 *     WHERE GRANTEE_ID = aGranteeID;
 *
 ***********************************************************************/
IDE_RC qdpRole::getRoleCountByUserID( qcStatement  * aStatement,
                                      UInt           aGranteeID,
                                      UInt         * aRoleCount )
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    mtdIntegerType         sIntDataOfGranteeID;
    smiTableCursor         sCursor;
    volatile SInt          sStage;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid;
    UInt                   sRoleCount = 0;
    mtcColumn            * sFirstMtcColumn;
    
    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qdpRole::getRoleCountByUserID::__FT__" );

    sStage = 0;

    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;

    IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                  QCM_USER_ROLES_GRANTEE_ID_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sFirstMetaRange,
        sFirstMtcColumn,
        (const void *) &sIntDataOfGranteeID,
        &sRange );
        
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, aStatement->mStatistics);

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmUserRoles,
                  gQcmUserRolesIndex[QCM_USER_ROLES_IDX2_ORDER],
                  smiGetRowSCN( gQcmUserRoles ),
                  NULL,
                  & sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  & sCursorProperty )
              != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT_FATAL( "qdpRole::getRoleCountByUserID::__FT__::STAGE1" );

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        
    while ( sRow != NULL )
    {
        sRoleCount++;
        
        IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }
        
    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );
    
    *aRoleCount = sRoleCount;
    
    IDE_FT_END();

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();
    
    if ( sStage != 0 )
    {
        (void) sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Role List array 구성
 *
 * Implementation :
 *     SELECT * FROM SYSTEM_.SYS_USER_ROLES_
 *     WHERE GRANTEE_ID = aSessionUserID;
 *
 ***********************************************************************/
IDE_RC qdpRole::getRoleListByUserID( qcStatement  * aStatement,
                                     UInt           aSessionUserID,
                                     UInt         * aRoleList )
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    mtdIntegerType         sIntDataOfGranteeID;
    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid;
    mtcColumn            * sRoleIDColumn;
    UInt                   sRoleID;
    UInt                   sRoleCount = 0;
    mtcColumn            * sFirstMtcColumn;

    aRoleList[sRoleCount] = aSessionUserID;
    sRoleCount++;
    
    if ( ( aSessionUserID != QC_SYSTEM_USER_ID) &&
         ( aSessionUserID != QC_SYS_USER_ID) )
    {
        IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                      QCM_USER_ROLES_ROLE_ID_COL_ORDER,
                                      (const smiColumn**)&sRoleIDColumn )
                  != IDE_SUCCESS );

        sIntDataOfGranteeID = (mtdIntegerType) aSessionUserID;

        IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                      QCM_USER_ROLES_GRANTEE_ID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn(
            &sFirstMetaRange,
            sFirstMtcColumn,
            (const void *) &sIntDataOfGranteeID,
            &sRange );
        
        sCursor.initialize();

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, aStatement->mStatistics);

        IDE_TEST( sCursor.open(
                      QC_SMI_STMT( aStatement ),
                      gQcmUserRoles,
                      gQcmUserRolesIndex[QCM_USER_ROLES_IDX2_ORDER],
                      smiGetRowSCN( gQcmUserRoles ),
                      NULL,
                      & sRange,
                      smiGetDefaultKeyRange(),
                      smiGetDefaultFilter(),
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      & sCursorProperty )
                  != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        
        while ( sRow != NULL )
        {
            sRoleID = *(UInt *) ( (UChar*)sRow +
                                  sRoleIDColumn->column.offset );

            IDE_TEST_RAISE( sRoleCount >= QDD_USER_TO_ROLES_MAX_COUNT, ERR_UNEXPECTED );
            
            aRoleList[sRoleCount] = sRoleID;

            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

            sRoleCount++;
        }
        
        sStage = 0;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }

    // end
    aRoleList[sRoleCount] = ID_UINT_MAX;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdpRole::getRoleListByUserID",
                                  "The count of the Role exceeds the maximum limit." ));
    }
    IDE_EXCEPTION_END;
    
    if ( sStage != 0 )
    {
        (void) sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
    
}

IDE_RC qdpRole::checkDDLCreateTablePriv(
    qcStatement   * aStatement,
    UInt            aTableOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateTablePriv( aStatement,
                                                    aTableOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}    

IDE_RC qdpRole::checkDDLCreateViewPriv(
    qcStatement   * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateViewPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreateIndexPriv(
    qcStatement   * aStatement,
    qcmTableInfo  * aTableInfo,
    UInt            aIndexOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateIndexPriv( aStatement,
                                                    aTableInfo,
                                                    aIndexOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreateSequencePriv(
    qcStatement   * aStatement,
    UInt            aSequenceOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateSequencePriv( aStatement,
                                                       aSequenceOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreatePSMPriv(
    qcStatement   * aStatement,
    UInt            aPSMOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreatePSMPriv( aStatement,
                                                  aPSMOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1685
IDE_RC qdpRole::checkDDLCreateLibraryPriv(
    qcStatement   * aStatement,
    UInt            aLibraryOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateLibraryPriv( aStatement,
                                                      aLibraryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropLibraryPriv(
    qcStatement   * aStatement,
    UInt            aLibraryOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropLibraryPriv( aStatement,
                                                    aLibraryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterLibraryPriv(
    qcStatement   * aStatement,
    UInt            aLibraryOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterLibraryPriv( aStatement,
                                                     aLibraryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLExecuteLibraryPriv(
    qcStatement    * aStatement,
    qdpObjID         aObjID,
    UInt             aLibraryOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLExecuteLibraryPriv( aStatement,
                                                       aObjID,
                                                       aLibraryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1359 Trigger
IDE_RC qdpRole::checkDDLCreateTriggerPriv(
    qcStatement   * aStatement,
    UInt            aTriggerOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateTriggerPriv( aStatement,
                                                      aTriggerOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// To Fix PR-10618
IDE_RC qdpRole::checkDDLCreateTriggerTablePriv(
    qcStatement * aStatement,
    UInt          aTableOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateTriggerTablePriv( aStatement,
                                                           aTableOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1076 Synonym */
IDE_RC qdpRole::checkDDLCreateSynonymPriv(
    qcStatement   * aStatement,
    UInt            aSynonymOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateSynonymPriv( aStatement,
                                                      aSynonymOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreatePublicSynonymPriv(
    qcStatement   * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreatePublicSynonymPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreateUserPriv(
    qcStatement   * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateUserPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLReplicationPriv(
    qcStatement   * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLReplicationPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterTablePriv(
    qcStatement   * aStatement,
    qcmTableInfo  * aTableInfo)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterTablePriv( aStatement,
                                                   aTableInfo )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterIndexPriv(
    qcStatement   * aStatement,
    qcmTableInfo  * aTableInfo,
    UInt            aIndexOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterIndexPriv( aStatement,
                                                   aTableInfo,
                                                   aIndexOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-16980
IDE_RC qdpRole::checkDDLAlterSequencePriv(
    qcStatement     * aStatement,
    qcmSequenceInfo * aSequenceInfo)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterSequencePriv( aStatement,
                                                      aSequenceInfo )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterPSMPriv(
    qcStatement   * aStatement,
    UInt            aPSMOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterPSMPriv( aStatement,
                                                 aPSMOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterTriggerPriv(
    qcStatement   * aStatement,
    UInt            aTriggerOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterTriggerPriv( aStatement,
                                                     aTriggerOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterUserPriv(
    qcStatement   * aStatement,
    UInt            aRealUserID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterUserPriv( aStatement,
                                                  aRealUserID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterSystemPriv(
    qcStatement   * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterSystemPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreateSessionPriv(
    qcStatement   * aStatement,
    UInt            aGranteeID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // BUG-41798
    if ( sRoleList[0] != aGranteeID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aGranteeID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        if ( qdpPrivilege::checkDDLCreateSessionPriv( aStatement,
                                                      sRoleList[i] )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropTablePriv(
    qcStatement   * aStatement,
    UInt            aTableOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropTablePriv( aStatement,
                                                  aTableOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropViewPriv(
    qcStatement   * aStatement,
    UInt            aViewOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropViewPriv( aStatement,
                                                 aViewOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropIndexPriv(
    qcStatement   * aStatement,
    qcmTableInfo  * aTableInfo,
    UInt            aIndexOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropIndexPriv( aStatement,
                                                  aTableInfo,
                                                  aIndexOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropSequencePriv(
    qcStatement   * aStatement,
    UInt            aSequenceOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropSequencePriv( aStatement,
                                                     aSequenceOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropPSMPriv(
    qcStatement   * aStatement,
    UInt            aPSMOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropPSMPriv( aStatement,
                                                aPSMOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1359 Trigger
IDE_RC qdpRole::checkDDLDropTriggerPriv(
    qcStatement   * aStatement,
    UInt            aTriggerOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropTriggerPriv( aStatement,
                                                    aTriggerOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropUserPriv(
    qcStatement   * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropUserPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1371 Directories
IDE_RC qdpRole::checkDDLCreateDirectoryPriv(
    qcStatement   * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateDirectoryPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropDirectoryPriv(
    qcStatement   * aStatement,
    UInt            aDirectoryOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropDirectoryPriv( aStatement,
                                                      aDirectoryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLExecutePSMPriv(
    qcStatement                        * aStatement,
    UInt                                 aProcOwnerID,
    UInt                                 aPrivilegeCount,
    UInt                               * aGranteeID,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;

        // PROJ-1436
        if ( aGetSmiStmt4Prepare == ID_TRUE )
        {
            IDE_TEST( qcgPlan::setSmiStmtCallback(
                          aStatement,
                          aGetSmiStmt4PrepareCallback,
                          aGetSmiStmt4PrepareContext )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLExecutePSMPriv( aStatement,
                                                   aProcOwnerID,
                                                   aPrivilegeCount,
                                                   aGranteeID,
                                                   aGetSmiStmt4Prepare,
                                                   aGetSmiStmt4PrepareCallback,
                                                   aGetSmiStmt4PrepareContext )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkReferencesPriv( qcStatement  * aStatement,
                                     UInt           aTableOwnerID,
                                     qcmTableInfo * aTableInfo )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    if ( sRoleList[0] != aTableOwnerID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aTableOwnerID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        if ( qdpPrivilege::checkReferencesPriv( aStatement,
                                                sRoleList[i],
                                                aTableInfo )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
// BUG-16980
IDE_RC qdpRole::checkDMLSelectSequencePriv(
    qcStatement                        * aStatement,
    UInt                                 aSequenceOwnerID,
    UInt                                 aSequenceID,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        // PROJ-1436
        if ( aGetSmiStmt4Prepare == ID_TRUE )
        {
            IDE_TEST( qcgPlan::setSmiStmtCallback(
                          aStatement,
                          aGetSmiStmt4PrepareCallback,
                          aGetSmiStmt4PrepareContext )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLSelectSequencePriv( aStatement,
                                                       aSequenceOwnerID,
                                                       aSequenceID,
                                                       aGetSmiStmt4Prepare,
                                                       aGetSmiStmt4PrepareCallback,
                                                       aGetSmiStmt4PrepareContext )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC qdpRole::checkDMLSelectTablePriv(
    qcStatement                        * aStatement,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        // PROJ-1436
        if ( aGetSmiStmt4Prepare == ID_TRUE )
        {
            IDE_TEST( qcgPlan::setSmiStmtCallback(
                          aStatement,
                          aGetSmiStmt4PrepareCallback,
                          aGetSmiStmt4PrepareContext )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLSelectTablePriv( aStatement,
                                                    aTableOwnerID,
                                                    aPrivilegeCount,
                                                    aPrivilegeInfo,
                                                    aGetSmiStmt4Prepare,
                                                    aGetSmiStmt4PrepareCallback,
                                                    aGetSmiStmt4PrepareContext )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLInsertTablePriv(
    qcStatement                        * aStatement,
    void                               * aTableHandle,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        // PROJ-1436
        if ( aGetSmiStmt4Prepare == ID_TRUE )
        {
            IDE_TEST( qcgPlan::setSmiStmtCallback(
                          aStatement,
                          aGetSmiStmt4PrepareCallback,
                          aGetSmiStmt4PrepareContext )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLInsertTablePriv( aStatement,
                                                    aTableHandle,
                                                    aTableOwnerID,
                                                    aPrivilegeCount,
                                                    aPrivilegeInfo,
                                                    aGetSmiStmt4Prepare,
                                                    aGetSmiStmt4PrepareCallback,
                                                    aGetSmiStmt4PrepareContext )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLDeleteTablePriv(
    qcStatement                        * aStatement,
    void                               * aTableHandle,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        // PROJ-1436
        if ( aGetSmiStmt4Prepare == ID_TRUE )
        {
            IDE_TEST( qcgPlan::setSmiStmtCallback(
                          aStatement,
                          aGetSmiStmt4PrepareCallback,
                          aGetSmiStmt4PrepareContext )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLDeleteTablePriv( aStatement,
                                                    aTableHandle,
                                                    aTableOwnerID,
                                                    aPrivilegeCount,
                                                    aPrivilegeInfo,
                                                    aGetSmiStmt4Prepare,
                                                    aGetSmiStmt4PrepareCallback,
                                                    aGetSmiStmt4PrepareContext )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLUpdateTablePriv(
    qcStatement                        * aStatement,
    void                               * aTableHandle,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        // PROJ-1436
        if ( aGetSmiStmt4Prepare == ID_TRUE )
        {
            IDE_TEST( qcgPlan::setSmiStmtCallback(
                          aStatement,
                          aGetSmiStmt4PrepareCallback,
                          aGetSmiStmt4PrepareContext )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLUpdateTablePriv( aStatement,
                                                    aTableHandle,
                                                    aTableOwnerID,
                                                    aPrivilegeCount,
                                                    aPrivilegeInfo,
                                                    aGetSmiStmt4Prepare,
                                                    aGetSmiStmt4PrepareCallback,
                                                    aGetSmiStmt4PrepareContext )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLLockTablePriv( qcStatement    * aStatement,
                                       UInt             aTableOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLLockTablePriv( aStatement,
                                                  aTableOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1371 Directories
IDE_RC qdpRole::checkDMLReadDirectoryPriv(
    qcStatement    * aStatement,
    qdpObjID         aObjID,
    UInt             aDirectoryOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLReadDirectoryPriv( aStatement,
                                                      aObjID,
                                                      aDirectoryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDMLWriteDirectoryPriv(
    qcStatement    * aStatement,
    qdpObjID         aObjID,
    UInt             aDirectoryOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDMLWriteDirectoryPriv( aStatement,
                                                       aObjID,
                                                       aDirectoryOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLCreateTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != aUserID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aUserID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        if ( qdpPrivilege::checkDDLCreateTableSpacePriv( aStatement,
                                                         sRoleList[i] )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != aUserID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aUserID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        if ( qdpPrivilege::checkDDLAlterTableSpacePriv( aStatement,
                                                        sRoleList[i] )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLManageTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != aUserID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aUserID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        if ( qdpPrivilege::checkDDLManageTableSpacePriv( aStatement,
                                                         sRoleList[i] )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != aUserID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aUserID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        if ( qdpPrivilege::checkDDLDropTableSpacePriv( aStatement,
                                                       sRoleList[i] )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkAccessAnyTBSPriv(
    qcStatement    * aStatement,
    UInt             aUserID,
    idBool         * aIsAccess)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsAccess = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != aUserID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aUserID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check grant
        IDE_TEST( qdpPrivilege::checkAccessAnyTBSPriv( aStatement,
                                                       sRoleList[i],
                                                       &sIsAccess )
                  != IDE_SUCCESS );

        if ( sIsAccess == ID_TRUE )
        {
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    *aIsAccess = sIsAccess;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkAccessTBS(
    qcStatement    * aStatement,
    UInt             aUserID,
    scSpaceID        aTBSID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != aUserID )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       aUserID,
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        // check access
        if ( qcmTablespace::checkAccessTBS( aStatement,
                                            sRoleList[i],
                                            aTBSID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1076 Synonym */
IDE_RC qdpRole::checkDDLDropSynonymPriv(
    qcStatement    * aStatement,
    UInt             aSynonymOwnerID)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropSynonymPriv( aStatement,
                                                    aSynonymOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropPublicSynonymPriv(
    qcStatement    * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropPublicSynonymPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-4990 changing the method of collecting index statistics */
IDE_RC qdpRole::checkDBMSStatPriv(
    qcStatement * aStatement)
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDBMSStatPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2211 Materialized View */
IDE_RC qdpRole::checkDDLCreateMViewPriv(
    qcStatement * aStatement,
    UInt          aOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateMViewPriv( aStatement,
                                                    aOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterMViewPriv(
    qcStatement  * aStatement,
    qcmTableInfo * aTableInfo )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterMViewPriv( aStatement,
                                                   aTableInfo )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropMViewPriv(
    qcStatement * aStatement,
    UInt          aOwnerID )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropMViewPriv( aStatement,
                                                  aOwnerID )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qdpRole::checkDDLCreateDatabaseLinkPriv( qcStatement * aStatement,
                                                idBool        aPublic )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateDatabaseLinkPriv( aStatement,
                                                           aPublic )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropDatabaseLinkPriv( qcStatement * aStatement,
                                              idBool        aPublic )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropDatabaseLinkPriv( aStatement,
                                                         aPublic )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1812 ROLE */
IDE_RC qdpRole::checkDDLCreateRolePriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateRolePriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropAnyRolePriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;


    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropAnyRolePriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC qdpRole::checkDDLGrantAnyRolePriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLGrantAnyRolePriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC qdpRole::checkDDLGrantAnyPrivilegesPriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;
        
        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLGrantAnyPrivilegesPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-41408 normal user create, alter , drop job */
IDE_RC qdpRole::checkDDLCreateAnyJobPriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;

        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }

    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLCreateAnyJobPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLDropAnyJobPriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;

        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }

    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLDropAnyJobPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpRole::checkDDLAlterAnyJobPriv( qcStatement * aStatement )
{
    UInt     sRoleListBuffer[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    UInt   * sRoleList;
    idBool   sIsPrivExist = ID_FALSE;
    UInt     i = 0;

    sRoleList = QCG_GET_SESSION_USER_ROLE_LIST( aStatement );

    // session user가 변경된 경우 다시 가져온다.
    if ( sRoleList[0] != QCG_GET_SESSION_USER_ID( aStatement ) )
    {
        sRoleList = sRoleListBuffer;

        IDE_TEST( getRoleListByUserID( aStatement,
                                       QCG_GET_SESSION_USER_ID( aStatement ),
                                       sRoleList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }

    for ( i = 0; sRoleList[i] != ID_UINT_MAX; i++ )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sRoleList[i] );

        // check grant
        if ( qdpPrivilege::checkDDLAlterAnyJobPriv( aStatement )
             == IDE_SUCCESS )
        {
            sIsPrivExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing To Do */
        }
    }

    QCG_SET_SESSION_USER_ID( aStatement, sRoleList[0] );

    IDE_TEST( sIsPrivExist == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

