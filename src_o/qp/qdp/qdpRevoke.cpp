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
 * $Id: qdpRevoke.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdd.h>
#include <qdpGrant.h>
#include <qdpRevoke.h>
#include <qcmUser.h>
#include <qcmProc.h>
#include <qcmPriv.h>
#include <qcg.h>
#include <qmv.h>
#include <qsx.h>
#include <qcuSqlSourceInfo.h>
#include <qcmPkg.h>
#include <qdpRole.h>

/***********************************************************************
 * VALIDATE
 **********************************************************************/

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::validateRevokeSystem
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    semantic checking of REVOKE SYSTEM statement
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpRevoke::validateRevokeSystem(qcStatement * aStatement)
{
#define IDE_FN "qdpRevoke::validatRevokeSystem"

    qdRevokeParseTree    * sParseTree;
    qdPrivileges         * sPrivilege;
    qdGrantees           * sGrantee;
    idBool                 sExist = ID_FALSE;
    UInt                   sErrorPrivID = 0;
    qdUserType             sUserType;
    qcuSqlSourceInfo       sqlInfo;
    

    sParseTree = (qdRevokeParseTree*) aStatement->myPlan->parseTree;
    sParseTree->grantorID = QCG_GET_SESSION_USER_ID(aStatement);

    // check grantees
    IDE_TEST( qdpGrant::checkGrantees(aStatement, sParseTree->grantees)
              != IDE_SUCCESS );

    for (sPrivilege = sParseTree->privileges;
         sPrivilege != NULL;
         sPrivilege = sPrivilege->next)
    {
        /* PROJ-1812 ROLE
         * ROLE 존재 여부 검사 */
        if ( sPrivilege->privType == QDP_ROLE_PRIV )
        {
            /* GET ROLE ID */
            IDE_TEST( qcmUser::getUserIDAndType( aStatement,
                                                 sPrivilege->privOrRoleName,
                                                 &sPrivilege->privOrRoleID,
                                                 &sUserType )
                      != IDE_SUCCESS );

            /* privilege list에 role type 검사 */
            IDE_TEST_RAISE ( sUserType != QDP_ROLE_TYPE, ERR_NOT_EXIST_ROLE );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    // check duplicate privileges
    IDE_TEST( qdpGrant::checkDuplicatePrivileges( aStatement,
                                                  sParseTree->privileges )
              != IDE_SUCCESS );

    // check granted privileges
    for (sPrivilege = sParseTree->privileges;
         sPrivilege != NULL;
         sPrivilege = sPrivilege->next)
    {
        for (sGrantee = sParseTree->grantees;
             sGrantee != NULL;
             sGrantee = sGrantee->next)
        {
            /* PROJ-1812 ROLE
             * SYS_USER_ROLES_예 revoke 할 권한 존재 여부 검사 */
            if ( sPrivilege->privType == QDP_ROLE_PRIV )
            {
                IDE_TEST( qcmPriv::checkRoleWithGrantor(
                              aStatement,
                              sParseTree->grantorID,
                              sGrantee->userOrRoleID,
                              sPrivilege->privOrRoleID,
                              &sExist )
                          != IDE_SUCCESS );

                if ( sExist == ID_FALSE )
                {    
                    sqlInfo.setSourceInfo( aStatement,
                                           &sPrivilege->privOrRoleName );
                    
                    IDE_RAISE( ERR_REVOKE_PRIV_NOT_GRANTED_ROLE );
                }
                else
                {
                    /* Nothing To Do */
                }
            }
            else
            {
                // BUG-41489 revoke all privileges
                if ( sPrivilege->privOrRoleID
                     == QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO )
                {
                    /* Nothing To Do */
                }
                else
                {
                    // SELECT *
                    //   FROM SYS_GRANT_SYSTEM
                    //  WHERE GRANTOR_ID = aGrantorID
                    //    AND GRANTEE_ID = aGranteeID
                    //    AND PRIV_ID = aPrivID;
                    IDE_TEST(qcmPriv::checkSystemPrivWithGrantor(
                                 aStatement,
                                 sParseTree->grantorID,
                                 sGrantee->userOrRoleID,
                                 sPrivilege->privOrRoleID,
                                 &sExist )
                             != IDE_SUCCESS);

                    if (sExist == ID_FALSE)
                    {
                        // check granted ALL PRIVILEGES
                        IDE_TEST(qcmPriv::checkSystemPrivWithGrantor(
                                     aStatement,
                                     sParseTree->grantorID,
                                     sGrantee->userOrRoleID,
                                     QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO,
                                     &sExist )
                                 != IDE_SUCCESS);

                        if (sExist != ID_TRUE)
                        {
                            sErrorPrivID = sPrivilege->privOrRoleID;

                            IDE_RAISE(ERR_REVOKE_PRIV_NOT_GRANTED);
                        }
                        else
                        {
                            /* Nothing To Do */
                        }
                    }
                    else
                    {
                        /* Nothing To Do */
                    }
                }
            }            
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REVOKE_PRIV_NOT_GRANTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED,
                                sErrorPrivID ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_ROLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_ROLE, "" ) );
    }
    IDE_EXCEPTION( ERR_REVOKE_PRIV_NOT_GRANTED_ROLE );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED_ROLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();   
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::validateRevokeObject
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    semantic checking of REVOKE OBJECT statement
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpRevoke::validateRevokeObject(qcStatement * aStatement)
{
#define IDE_FN "qdpRevoke::validatRevokeObject"

    qdRevokeParseTree    * sParseTree;
    qdPrivileges         * sPrivilege;
    qdGrantees           * sGrantee;
    qcmGrantObject       * sGrantObject = NULL;
    qcmGrantObject       * sGrantObjectTemp = NULL;
    UInt                   sErrorPrivID = 0;
        
    sParseTree = (qdRevokeParseTree*) aStatement->myPlan->parseTree;
    sPrivilege = sParseTree->privileges;
    sParseTree->grantorID = QCG_GET_SESSION_USER_ID(aStatement);

    // check object ( table, sequence, procedure )
    IDE_TEST( qdpGrant::checkObjectInfo( aStatement,
                                         sParseTree->userName,
                                         sParseTree->objectName,
                                         &sParseTree->userID,
                                         &sParseTree->objectID,
                                         sParseTree->objectType)
              != IDE_SUCCESS);

    // check grantees
    IDE_TEST( qdpGrant::checkGrantees(aStatement, sParseTree->grantees)
              != IDE_SUCCESS );

    // check duplicate privileges
    IDE_TEST( qdpGrant::checkDuplicatePrivileges(aStatement, sPrivilege)
              != IDE_SUCCESS );

    sPrivilege = sParseTree->privileges;

    // revoke ALL PRIVILEGES
    if ( sPrivilege->privOrRoleID == QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO )
    {
        for (sGrantee = sParseTree->grantees;
             sGrantee != NULL;
             sGrantee = sGrantee->next)
        {
            // check granted privilege
            //
            // SELECT *
            //   FROM SYS_GRANT_OBJECT
            //  WHERE GRANTOR_ID = aGrantorID
            //    AND GRANTEE_ID = aGranteeID
            //    AND OBJ_ID = aObjID
            //    AND OBJ_TYPE = aObjType;
            IDE_TEST(qcmPriv::getGrantObjectWithoutPrivilege(
                         aStatement,
                         QC_QMP_MEM(aStatement),
                         sParseTree->grantorID,
                         sGrantee->userOrRoleID,
                         sParseTree->objectID,
                         sParseTree->objectType,
                         &sGrantObject)
                     != IDE_SUCCESS);

            sGrantObjectTemp = sGrantObject;

            if (sGrantObjectTemp == NULL)
            {
                sErrorPrivID = sPrivilege->privOrRoleID;

                IDE_RAISE(ERR_REVOKE_PRIV_NOT_GRANTED);
            }
            else
            {
                while ( sGrantObjectTemp != NULL )
                {
                    if (sGrantObjectTemp->withGrantOption == ID_TRUE)
                    {
                        sGrantee->grantOption = ID_TRUE;
                    }
                    else
                    {
                        sGrantee->grantOption = ID_FALSE;
                    }
                    sGrantObjectTemp = sGrantObjectTemp->next;
                }
            }
        }
    }
    else
    {
        // check granted privileges
        for (sPrivilege = sParseTree->privileges;
             sPrivilege != NULL;
             sPrivilege = sPrivilege->next)
        {
            for (sGrantee = sParseTree->grantees;
                 sGrantee != NULL;
                 sGrantee = sGrantee->next)
            {                
                // SELECT *
                //   FROM SYS_GRANT_OBJECT
                //  WHERE GRANTOR_ID = aGrantorID
                //    AND GRANTEE_ID = aGranteeID
                //    AND PRIV_ID = aPrivID
                //    AND OBJ_ID = aObjID
                //    AND OBJ_TYPE = aObjType;
                IDE_TEST(qcmPriv::getGrantObjectWithGrantee(
                             aStatement,
                             QC_QMP_MEM(aStatement),
                             sParseTree->grantorID,
                             sGrantee->userOrRoleID,
                             sPrivilege->privOrRoleID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             &sGrantObject)
                         != IDE_SUCCESS);

                if (sGrantObject == NULL)
                {
                    sErrorPrivID = sPrivilege->privOrRoleID;

                    IDE_RAISE(ERR_REVOKE_PRIV_NOT_GRANTED);
                }
                else
                {
                    if (sGrantObject->withGrantOption == ID_TRUE)
                    {
                        sGrantee->grantOption = ID_TRUE;
                    }
                    else
                    {
                        sGrantee->grantOption = ID_FALSE;
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_REVOKE_PRIV_NOT_GRANTED)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED,
                                sErrorPrivID));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/***********************************************************************
 * EXECUTE
 **********************************************************************/

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::executeRevokeSystem
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    execution of REVOKE SYSTEM statement
 *        - delete from SYS_GRANT_SYSTEM_
 *
 * In case REVOKE ALL PRIVILEGES :
 *    if existence PRIV_ID = 1 (ALL PRIVILEGES) in SYS_GRANT_SYSTEM_
 *        then revoke one row PRIV_ID = 1 (ALL PRIVILEGES)
 *
 * In case REVOKE each privileges
 *    the privileges is not existence and ALL PRIVILEGES is existence :
 *
 *    1. delete ALL PRIVILEGES ( delege 1 row )
 *    2. insert all system privileges ( insert 32 rows )
 *    3. delete the privileges
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpRevoke::executeRevokeSystem(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      REVOKE privilege FROM grantees 수행
 *
 * Implementation :
 *      1. SYS_GRANT_SYSTEM_ 메타 테이블에서 privilege 삭제
 *
 ***********************************************************************/

#define IDE_FN "qdpRevoke::executeRevokeSystem"

    qdRevokeParseTree   * sParseTree;
    qdPrivileges        * sPrivilege;
    qdGrantees          * sGrantee;
    SChar               * sSqlStr;
    vSLong                sRowCnt;
    idBool                sExist = ID_FALSE;

    sParseTree = (qdRevokeParseTree*) aStatement->myPlan->parseTree;

    IDU_LIMITPOINT("qdpRevoke::executeRevokeSystem::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    for (sPrivilege = sParseTree->privileges;
         sPrivilege != NULL;
         sPrivilege = sPrivilege->next)
    {
        for (sGrantee = sParseTree->grantees;
             sGrantee != NULL;
             sGrantee = sGrantee->next)
        {
            /* PROJ-1812 ROLE */
            if ( sPrivilege->privType == QDP_ROLE_PRIV )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "DELETE FROM SYS_USER_ROLES_ "
                                 "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' "
                                 "AND GRANTEE_ID = INTEGER'%"ID_INT32_FMT"' "
                                 "AND ROLE_ID =  INTEGER'%"ID_INT32_FMT"'",
                                 sParseTree->grantorID,
                                 sGrantee->userOrRoleID,
                                 sPrivilege->privOrRoleID);

                IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT( aStatement ),
                                           sSqlStr,
                                           & sRowCnt ) != IDE_SUCCESS);

                IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
            }
            else
            {
                // BUG-41489 revoke all privileges
                if ( sPrivilege->privOrRoleID
                     == QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO )
                {
                    // grantor가 sys인 경우 기본 권한은 삭제하지 않는다.
                    if ( sParseTree->grantorID == QC_SYS_USER_ID )
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "DELETE FROM SYS_GRANT_SYSTEM_ "
                                         "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' "
                                         "AND GRANTEE_ID = INTEGER'%"ID_INT32_FMT"' "
                                         "AND PRIV_ID NOT IN ("
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"',"
                                         "INTEGER'%"ID_INT32_FMT"')",
                                         sParseTree->grantorID,
                                         sGrantee->userOrRoleID,
                                         QCM_PRIV_ID_SYSTEM_CREATE_SESSION_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_TABLE_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_SEQUENCE_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_PROCEDURE_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_VIEW_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_SYNONYM_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_MATERIALIZED_VIEW_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_LIBRARY_NO,
                                         QCM_PRIV_ID_SYSTEM_CREATE_DATABASE_LINK_NO );
                    }
                    else
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "DELETE FROM SYS_GRANT_SYSTEM_ "
                                         "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' "
                                         "AND GRANTEE_ID = INTEGER'%"ID_INT32_FMT"'",
                                         sParseTree->grantorID,
                                         sGrantee->userOrRoleID );
                    }

                    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                              != IDE_SUCCESS );
                }
                else
                {
                    // the privilege is not existence, but have ALL PRIVILEGES.
                    // 1. delete ALL PRIVILEGES ( delege 1 row )
                    // 2. insert all system privileges ( insert 32 rows )
                    // 3. delete the privileges
                    IDE_TEST(qcmPriv::checkSystemPrivWithGrantor(
                                 aStatement,
                                 sParseTree->grantorID,
                                 sGrantee->userOrRoleID,
                                 sPrivilege->privOrRoleID,
                                 &sExist )
                             != IDE_SUCCESS);

                    if (sExist != ID_TRUE)
                    {
                        IDE_TEST(qcmPriv::checkSystemPrivWithGrantor(
                                     aStatement,
                                     sParseTree->grantorID,
                                     sGrantee->userOrRoleID,
                                     QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO,
                                     &sExist )
                                 != IDE_SUCCESS);

                        if (sExist == ID_TRUE)
                        {
                            IDE_TEST(deleteSystemAllPrivileges(
                                         aStatement,
                                         sParseTree->grantorID,
                                         sGrantee->userOrRoleID)
                                     != IDE_SUCCESS);
                        }
                        else
                        {
                            /* Nothing To Do */
                        }
                    }
                    else
                    {
                        /* Nothing To Do */
                    }
                
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "DELETE FROM SYS_GRANT_SYSTEM_ "
                                     "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' "
                                     "AND GRANTEE_ID = INTEGER'%"ID_INT32_FMT"' "
                                     "AND PRIV_ID =  INTEGER'%"ID_INT32_FMT"'",
                                     sParseTree->grantorID,
                                     sGrantee->userOrRoleID,
                                     sPrivilege->privOrRoleID);

                    IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT( aStatement ),
                                               sSqlStr,
                                               & sRowCnt ) != IDE_SUCCESS);

                    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::executeRevokeObject
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    execution of REVOKE OBJECT statement
 *        - delete from SYS_GRANT_OBJECT_
 *
 * In case REVOKE ALL (PRIVILEGES) :
 *    revoke all privileges which grantee has privilege.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpRevoke::executeRevokeObject(qcStatement * aStatement)
{
#define IDE_FN "qdpRevoke::executeRevokeObject"

    qdRevokeParseTree   * sParseTree;
    qdPrivileges        * sPrivilege;
    qdGrantees          * sGrantee;
    UInt                  sPrivID;
    UInt                  sWithGrantOption;
    qcmTableInfo        * sTableInfo = NULL;
    qcmGrantObject      * sGrantObject = NULL;
    qcmRefChildInfo     * sChildInfo;  // BUG-28049
    smSCN                 sSCN;
    void                * sTableHandle;
    idBool                sExist = ID_FALSE;
    SChar                 sObjectName[ QC_MAX_OBJECT_NAME_LEN + 1];
    qcuSqlSourceInfo      sqlInfo;
    UInt                  sSqlCode;
    qdReferenceGranteeList   * sUserList;
    qdReferenceGranteeList   * sCheckUserList;

    sParseTree = (qdRevokeParseTree*) aStatement->myPlan->parseTree;
    sPrivilege = sParseTree->privileges;

    // when object type is table or view, get sTableInfo
    if (sParseTree->objectType[0] == 'T')
    {
        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sParseTree->objectID,
                                       &sTableInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);

        IDE_TEST(qcm::validateAndLockTable(aStatement,
                                           sTableHandle,
                                           sSCN,
                                           SMI_TABLE_LOCK_X)
                 != IDE_SUCCESS);

        // PROJ-1567
        // BUG-17503
        // IDE_TEST_RAISE(sTableInfo->replicationCount > 0,
        //                ERR_DDL_WITH_REPLICATED_TABLE);
    }
    else
    {
        // Sequence or Procedure or Directory, Nothing to do
    }

    if ( sPrivilege->privOrRoleID == QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO)
    {
        // REVOKE ALL [PRIVILEGES] 인 경우
        for (sGrantee = sParseTree->grantees;
             sGrantee != NULL;
             sGrantee = sGrantee->next)
        {
            IDE_TEST(qcmPriv::getGrantObjectWithoutPrivilege(
                         aStatement,
                         QC_QMP_MEM(aStatement),
                         sParseTree->grantorID,
                         sGrantee->userOrRoleID,
                         sParseTree->objectID,
                         sParseTree->objectType,
                         &sGrantObject)
                     != IDE_SUCCESS);

            while ( sGrantObject != NULL )
            {
                sPrivID = sGrantObject->privID;

                if ( sGrantObject->withGrantOption == ID_TRUE )
                {
                    IDE_TEST(deleteChainObjectByGrantOption(
                                 aStatement,
                                 sTableInfo,
                                 sPrivID,
                                 sGrantee->userOrRoleID,
                                 sParseTree->grantorID)
                             != IDE_SUCCESS);

                }

                // check CASCADE CONSTRAINTS
                if (sPrivID == QCM_PRIV_ID_OBJECT_REFERENCES_NO)
                {
                    // REFERENCES 권한을 가진다면, 항상 테이블이어야 한다.
                    IDE_TEST_RAISE( sTableInfo == NULL,
                                    ERR_INAPPROPRIATE_REF_PRIV );

                    /* PROJ-1812 ROLE */
                    if ( sGrantee->userType == QDP_ROLE_TYPE )
                    {
                        IDE_TEST( qdpRole::getGranteeListByRoleID(  aStatement,
                                                                    sGrantee->userOrRoleID,
                                                                    &sUserList )
                                  != IDE_SUCCESS );

                        /* ROLE을 부여 받은 user에 대해 cascade option 존재 여부 체크 */
                        for ( sCheckUserList = sUserList;
                              sCheckUserList != NULL;
                              sCheckUserList = sCheckUserList->next )
                        {
                            if (sParseTree->cascadeConstr == ID_FALSE)
                            {
                                IDE_TEST(qcm::checkCascadeOption(
                                             aStatement,
                                             sCheckUserList->userID,
                                             sParseTree->objectID,
                                             &sExist) != IDE_SUCCESS);

                                IDE_TEST_RAISE(sExist == ID_TRUE,
                                               ERR_WITHOUT_CASCADE_OPTION);
                            }
                            else
                            {
                                // 관련된 foreign key constraint를 모두 삭제함
                                IDE_TEST( qcm::getChildKeysForDelete(
                                              aStatement,
                                              sCheckUserList->userID,
                                              NULL,
                                              sTableInfo,
                                              ID_FALSE /* aDropTablespace */,
                                              &sChildInfo )
                                          != IDE_SUCCESS );

                                IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                                                    sTableInfo,
                                                                    sChildInfo,
                                                                    ID_FALSE /* aDropTablespace */ )
                                          != IDE_SUCCESS );
                            }
                        }   
                    }
                    else
                    {
                        if (sParseTree->cascadeConstr == ID_FALSE)
                        {                        
                        
                            IDE_TEST(qcm::checkCascadeOption(
                                         aStatement,
                                         sGrantee->userOrRoleID,
                                         sParseTree->objectID,
                                         &sExist) != IDE_SUCCESS);

                            IDE_TEST_RAISE(sExist == ID_TRUE,
                                           ERR_WITHOUT_CASCADE_OPTION);
                        }
                        else
                        {
                            // 관련된 foreign key constraint를 모두 삭제함
                            IDE_TEST( qcm::getChildKeysForDelete( aStatement,
                                                                  sGrantee->userOrRoleID,
                                                                  NULL,
                                                                  sTableInfo,
                                                                  ID_FALSE /* aDropTablespace */,
                                                                  &sChildInfo )
                                      != IDE_SUCCESS );

                            IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                                                sTableInfo,
                                                                sChildInfo,
                                                                ID_FALSE /* aDropTablespace */ )
                                      != IDE_SUCCESS );
                        }
                    }                    
                }

                // delete privilege from SYS_GRANT_OBJECT_
                IDE_TEST(deleteObjectPrivFromMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee->userOrRoleID,
                             sPrivID,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sGrantObject->withGrantOption)
                         != IDE_SUCCESS);

                sGrantObject = sGrantObject->next;
            }
        }
    }
    else
    {
        // REVOKE ALL [PRIVILEGES]를 제외한 PRIVELEGE인 경우
        // delete from SYS_GRANT_OBJECT_
        for (sPrivilege = sParseTree->privileges;
             sPrivilege != NULL;
             sPrivilege = sPrivilege->next)
        {
            for (sGrantee = sParseTree->grantees;
                 sGrantee != NULL;
                 sGrantee = sGrantee->next)
            {
                if (sGrantee->grantOption == ID_TRUE)
                {
                    sWithGrantOption = QCM_WITH_GRANT_OPTION_TRUE;
                }
                else
                {
                    sWithGrantOption = QCM_WITH_GRANT_OPTION_FALSE;
                }                

                if (sGrantee->grantOption == ID_TRUE)
                {
                    IDE_TEST(deleteChainObjectByGrantOption(
                                 aStatement,
                                 sTableInfo,
                                 sPrivilege->privOrRoleID,
                                 sGrantee->userOrRoleID,
                                 sParseTree->grantorID)
                             != IDE_SUCCESS);
                }                

                // check CASCADE CONSTRAINTS
                if (sPrivilege->privOrRoleID == QCM_PRIV_ID_OBJECT_REFERENCES_NO)
                {
                    // REFERENCES 권한을 가진다면, 항상 테이블이어야 한다.
                    IDE_TEST_RAISE( sTableInfo == NULL,
                                    ERR_INAPPROPRIATE_REF_PRIV );

                    /* PROJ-1812 ROLE */
                    if ( sGrantee->userType == QDP_ROLE_TYPE )
                    {
                        IDE_TEST( qdpRole::getGranteeListByRoleID(  aStatement,
                                                                    sGrantee->userOrRoleID,
                                                                    &sUserList )
                                  != IDE_SUCCESS );

                        /* ROLE을 부여 받은 user에 대해 cascade option 존재 여부 체크 */
                        for ( sCheckUserList = sUserList;
                              sCheckUserList != NULL;
                              sCheckUserList = sCheckUserList->next )
                        {
                            if (sParseTree->cascadeConstr == ID_FALSE)
                            {
                                IDE_TEST(qcm::checkCascadeOption(
                                             aStatement,
                                             sCheckUserList->userID,
                                             sParseTree->objectID,
                                             &sExist) != IDE_SUCCESS);

                                IDE_TEST_RAISE(sExist == ID_TRUE,
                                               ERR_WITHOUT_CASCADE_OPTION);
                            }
                            else
                            {
                                // 관련된 foreign key constraint를 모두 삭제함
                                IDE_TEST( qcm::getChildKeysForDelete(
                                              aStatement,
                                              sCheckUserList->userID,
                                              NULL,
                                              sTableInfo,
                                              ID_FALSE /* aDropTablespace */,
                                              &sChildInfo )
                                          != IDE_SUCCESS );

                                IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                                                    sTableInfo,
                                                                    sChildInfo,
                                                                    ID_FALSE /* aDropTablespace */ )
                                          != IDE_SUCCESS );
                            }

                        }
                    }
                    else
                    {
                        if (sParseTree->cascadeConstr == ID_FALSE)
                        {
                            IDE_TEST(qcm::checkCascadeOption(
                                         aStatement,
                                         sGrantee->userOrRoleID,
                                         sParseTree->objectID,
                                         &sExist) != IDE_SUCCESS);

                            IDE_TEST_RAISE(sExist == ID_TRUE,
                                           ERR_WITHOUT_CASCADE_OPTION);
                        }
                        else
                        {
                            // 관련된 foreign key constraint를 모두 삭제함
                            IDE_TEST( qcm::getChildKeysForDelete( aStatement,
                                                                  sGrantee->userOrRoleID,                                                              
                                                                  NULL,
                                                                  sTableInfo,
                                                                  ID_FALSE /* aDropTablespace */,
                                                                  &sChildInfo )
                                      != IDE_SUCCESS );

                            IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                                                sTableInfo,
                                                                sChildInfo,
                                                                ID_FALSE /* aDropTablespace */ )
                                      != IDE_SUCCESS );
                        }
                    }                    
                }
                else
                {
                    /* Notihg To Do */
                }
                
                // delete privilege from SYS_GRANT_OBJECT_
                IDE_TEST(deleteObjectPrivFromMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee->userOrRoleID,
                             sPrivilege->privOrRoleID,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
        }
    }

    if (sParseTree->objectType[0] == 'T')
    {
        // table or view
        // rebuild qcmTableInfo
        IDE_TEST(qcm::touchTable(QC_SMI_STMT( aStatement ),
                                 sParseTree->objectID,
                                 SMI_TBSLV_DDL_DML )
            != IDE_SUCCESS);

        // BUG-14509
        // 관련 psm들이 rebuild될 수 있도록 invalid상태로 변경.
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sTableInfo->name,
                      idlOS::strlen((SChar*)sTableInfo->name),
                      QS_TABLE )
                  != IDE_SUCCESS );

        // PROJ-1073 Package
        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sTableInfo->name,
                      idlOS::strlen((SChar*)sTableInfo->name),
                      QS_TABLE )
                  != IDE_SUCCESS );

        IDE_TEST(qcmTableInfoMgr::makeTableInfo( aStatement,
                                                 sTableInfo,
                                                 NULL )
                 != IDE_SUCCESS);
    }
    else if (sParseTree->objectType[0] == 'S')
    {
        // sequence
        QC_STR_COPY( sObjectName, sParseTree->objectName );

        // 관련 psm들이 rebuild될 수 있도록 invalid상태로 변경.
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_TABLE )
                  != IDE_SUCCESS );

        // PROJ-1073 Package
        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_TABLE )
                  != IDE_SUCCESS );
    }
    else if (sParseTree->objectType[0] == 'P')
    {
        // procedure
        // rebuild qsxProcInfo
        IDE_TEST(qsx::rebuildQsxProcInfoPrivilege( aStatement,
                                                   sParseTree->objectID )
                 != IDE_SUCCESS);

        QC_STR_COPY( sObjectName, sParseTree->objectName );

        // To fix BUG-14508
        // user권한 체크는 compile시에만 체크하므로,
        // 관련 psm들이 rebuild될 수 있도록 invalid상태로 변경.
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_PROC )
                  != IDE_SUCCESS );

        IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_FUNC )
                  != IDE_SUCCESS );

        IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_TYPESET )
                  != IDE_SUCCESS );

        // PROJ-1073 Package
        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_PROC )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_FUNC )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_TYPESET )
                  != IDE_SUCCESS );
    }
    // PROJ-1073 Package
    else if (sParseTree->objectType[0] == 'A')
    {
        // package 
        // rebuild qsxPkgInfo
        IDE_TEST(qsx::rebuildQsxPkgInfoPrivilege( aStatement,
                                                  sParseTree->objectID )
                 != IDE_SUCCESS);
 
        QC_STR_COPY( sObjectName, sParseTree->objectName );

        // PROJ-1073 Package
        // 관련 package들이 rebuild될 수 있도록 invalid상태로 변경.
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_PKG )
                  != IDE_SUCCESS );
    } 
    else if( sParseTree->objectType[0] == 'Y')
    {
        // BUG-37304
        QC_STR_COPY( sObjectName, sParseTree->objectName );

        IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_LIBRARY )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                      aStatement,
                      sParseTree->userID,
                      sObjectName,
                      idlOS::strlen(sObjectName),
                      QS_LIBRARY )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }    

    // fix BUG-14394
    IDE_TEST( qcmPriv::updateLastDDLTime( aStatement,
                                          sParseTree->objectType,
                                          sParseTree->objectID )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_WITHOUT_CASCADE_OPTION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_WITHOUT_CASCADE_OPTION));
    }
    IDE_EXCEPTION( ERR_INAPPROPRIATE_REF_PRIV )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdpRevoke::executeRevokeObject",
                                  "REFERENCES privilege might have been granted on non-table object" ));
    }    
    IDE_EXCEPTION_END;

    // sqlSourceInfo가 없는 error라면...
    if ( ideHasErrorPosition() == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->objectName );

        sSqlCode = ideGetErrorCode();

        (void)sqlInfo.initWithBeforeMessage(QC_QME_MEM(aStatement));
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                            sqlInfo.getBeforeErrMessage(),
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

        // overwrite wrapped errorcode to original error code.
        ideGetErrorMgr()->Stack.LastError = sSqlCode;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::deleteChainObjectByGrantOption
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - 만약 객체의 종류가 table이라면 설정되어 넘어옴
 *    aPrivID    - privilege ID in parse tree ( IN )
 *    aNextGrantorID  - who was Grant to others with WITH GRANT OPTION ( IN )
 *    aFirstGrantorID - Grantor ID in parse tree ( IN )
 *
 * Description :
 *    recursive function for execution of REVOKE OBJECT WITH GRANT OPTION
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpRevoke::deleteChainObjectByGrantOption(
    qcStatement    * aStatement,
    qcmTableInfo   * aTableInfo,
    UInt             aPrivID,
    UInt             aNextGrantorID,
    UInt             aFirstGrantor)
{
#define IDE_FN "qdpRevoke::deleteChainObjectByGrantOption"

    qdRevokeParseTree   * sParseTree;
    qcmGrantObject      * sGrantObject = NULL;
    qcmGrantObject      * sGrantObjectTemp;
    qcmRefChildInfo     * sChildInfo;  // BUG-28049
    UInt                  sGrantorID;
    UInt                  sNextGrantorID;
    idBool                sExist = ID_FALSE;

    sParseTree    = (qdRevokeParseTree*) aStatement->myPlan->parseTree;
    sGrantorID    = aNextGrantorID;

    // 이미 이 함수를 호출하는 시점에서
    // 만약 sParseTree->objectID가 table이라면 적절한 lock이 이미 획득된 상태임
    
    IDE_TEST(qcmPriv::getGrantObjectWithoutGrantee(
                aStatement,
                aStatement->qmxMem,
                sGrantorID,
                aPrivID,
                sParseTree->objectID,
                sParseTree->objectType,
                &sGrantObject) != IDE_SUCCESS);
    if( sGrantObject != NULL )
    {
        sNextGrantorID = sGrantObject->granteeID;
        sGrantObjectTemp = sGrantObject;

        while (sGrantObjectTemp != NULL)
        {
            if ( sGrantObjectTemp->withGrantOption == 1 &&
                 aFirstGrantor != sGrantObject->granteeID )
            {
                sParseTree->grantees->grantOption = ID_TRUE;

                IDE_TEST(deleteChainObjectByGrantOption(
                             aStatement,
                             aTableInfo,
                             aPrivID,
                             sNextGrantorID,
                             aFirstGrantor)
                         != IDE_SUCCESS);
            }

            // check CASCADE CONSTRAINTS
            if( aPrivID == QCM_PRIV_ID_OBJECT_REFERENCES_NO )
            {
                // REFERENCES 권한을 가진다면, 항상 테이블이어야 한다.
                IDE_TEST_RAISE( aTableInfo == NULL,
                                ERR_INAPPROPRIATE_REF_PRIV );

                if( sParseTree->cascadeConstr == ID_FALSE )
                {
                    IDE_TEST(qcm::checkCascadeOption(
                                 aStatement,
                                 sNextGrantorID,
                                 sParseTree->objectID,
                                 &sExist) != IDE_SUCCESS);
                    
                    IDE_TEST_RAISE(sExist == ID_TRUE,
                                   ERR_WITHOUT_CASCADE_OPTION);
                }
                else
                {
                    // 관련된 foreign key constraint를 모두 삭제함
                    IDE_TEST( qcm::getChildKeysForDelete( aStatement,
                                                          sNextGrantorID,
                                                          NULL,
                                                          aTableInfo,
                                                          ID_FALSE /* aDropTablespace */,
                                                          &sChildInfo )
                              != IDE_SUCCESS );
                    
                    IDE_TEST( qdd::deleteFKConstraints( aStatement,
                                                        aTableInfo,
                                                        sChildInfo,
                                                        ID_FALSE /* aDropTablespace */ )
                              != IDE_SUCCESS );                    
                }
            }

            // delete privilege from SYS_GRANT_OBJECT_
            IDE_TEST(deleteObjectPrivFromMeta(
                         aStatement,
                         sGrantObject->grantorID,
                         sGrantObject->granteeID,
                         aPrivID,
                         sParseTree->userID,
                         sParseTree->objectID,
                         sParseTree->objectType,
                         sGrantObjectTemp->withGrantOption)
                     != IDE_SUCCESS);

            sGrantObjectTemp = sGrantObjectTemp->next;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_WITHOUT_CASCADE_OPTION)
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QDP_WITHOUT_CASCADE_OPTION));
    }
    IDE_EXCEPTION( ERR_INAPPROPRIATE_REF_PRIV )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdpRevoke::deleteChainObjectByGrantOption",
                                  "REFERENCES privilege might have been granted on non-table object" ));
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::deleteChainObjectByGrantOption
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGrantorID - grantor ID in parse tree ( IN )
 *    aGranteeID - grantee ID in parse tree ( IN )
 *    aPrivID    - privileges ID in parse tree ( IN )
 *
 * Description :
 *    execution of REVOKE ALL (SYSTEM) PRIVILEGES statement
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpRevoke::deleteSystemAllPrivileges(
    qcStatement    * aStatement,
    UInt             aGrantorID,
    UInt             aGranteeID)
{
/***********************************************************************
 *
 * Description :
 *      모든 권한 삭제
 *
 * Implementation :
 *      1. SYS_GRANT_SYSTEM_ 메타 테이블에서 모든 권한 삭제
 *
 ***********************************************************************/

#define IDE_FN "qdpRevoke::deleteSystemAllPrivileges"

    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDU_LIMITPOINT("qdpRevoke::deleteSystemAllPrivileges::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    // delete ALL PRIVILEGES
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_SYSTEM_ "
                     "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND GRANTEE_ID = INTEGER'%"ID_INT32_FMT"'",
                     aGrantorID,
                     aGranteeID);

    IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT( aStatement ),
                               sSqlStr,
                               & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt < 0, ERR_META_CRASH);

    /* BUG-45561 특정 상황에 ALL 권한 부여시 중복 부여, 반복 회수, 사용불가능한 권한 부여 문제가 발생합니다.
     *  - ALL권한과 SYS 고유 권한을 제외한 사용가능한 모든 권한을 부여한다. 사용불가능한 권한 부여 방지한다.
     */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_"
                     " ( SELECT"
                     " '%"ID_INT32_FMT"',"
                     " '%"ID_INT32_FMT"',"
                     " PRIV_ID"
                     " FROM SYS_PRIVILEGES_"
                     " WHERE PRIV_TYPE = 2"
                     " AND PRIV_ID != '%"ID_INT32_FMT"'"
                     " AND PRIV_ID != '%"ID_INT32_FMT"'"
                     " AND PRIV_ID != '%"ID_INT32_FMT"'"
                     " AND PRIV_ID != '%"ID_INT32_FMT"'"
                     " AND PRIV_ID != '%"ID_INT32_FMT"'"
                     " )",
                     aGrantorID,
                     aGranteeID,
                     QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO,
                     QCM_PRIV_ID_SYSTEM_ALTER_DATABASE_NO,
                     QCM_PRIV_ID_SYSTEM_DROP_DATABASE_NO,
                     QCM_PRIV_ID_SYSTEM_MANAGE_TABLESPACE_NO,
                     QCM_PRIV_ID_SYSTEM_SYSDBA_NO );

    IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT( aStatement ),
                               sSqlStr,
                               & sRowCnt ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpRevoke::deleteObjectPrivFromMeta
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGrantorID - grantor ID.
 *    aGranteeID - grantee ID.
 *    aPrivID    - privilege ID.
 *    aUser      - the object's owner.
 *    aObjID  - object ID.
 *    aObjectType      - object Type ( T : table / P : procedure )
 *    aWithGrantOption - WITH GRANT OPTION.
 *
 * Description :
 *        DELETE privilege from SYS_GRANT_OBJECT_
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpRevoke::deleteObjectPrivFromMeta(
    qcStatement     * aStatement,
    UInt              aGrantorID,
    UInt              aGranteeID,
    UInt              aPrivID,
    UInt              aUserID,
    qdpObjID          aObjID,
    SChar           * aObjectType,
    UInt              aWithGrantOption)
{
/***********************************************************************
 *
 * Description :
 *      OBJECT 권한 삭제
 *
 * Implementation :
 *      1. SYS_GRANT_OBJECT_ 메타 테이블에서 명시된 object 에 관한
 *         명시된 권한 삭제
 *
 ***********************************************************************/

#define IDE_FN "qdpRevoke::deleteObjectPrivFromMeta"

    SChar               * sSqlStr;
    vSLong           sRowCnt;

    IDU_LIMITPOINT("qdpRevoke::deleteObjectPrivFromMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_GRANT_OBJECT_ "
                     "WHERE GRANTOR_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND GRANTEE_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND PRIV_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND USER_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND OBJ_ID = BIGINT'%"ID_INT64_FMT"' "
                     "AND OBJ_TYPE = VARCHAR'%s' "
                     "AND WITH_GRANT_OPTION = INTEGER'%"ID_INT32_FMT"'",
                     aGrantorID,
                     aGranteeID,
                     aPrivID,
                     aUserID,
                     QCM_OID_TO_BIGINT( aObjID ),
                     aObjectType,
                     aWithGrantOption );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
