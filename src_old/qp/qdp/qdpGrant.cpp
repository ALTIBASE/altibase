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
 * $Id: qdpGrant.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdpGrant.h>
#include <qcmUser.h>
#include <qcmPriv.h>
#include <qcmProc.h>
#include <qcg.h>
#include <qsx.h>
#include <qcuSqlSourceInfo.h>
#include <qdpPrivilege.h>
#include <qcmSynonym.h>
#include <qcmView.h>
#include <qcmPkg.h>
#include <qcmDirectory.h>
#include <qmv.h>
#include <smErrorCode.h>
#include <qdpRole.h>

/***********************************************************************
 * VALIDATE
 **********************************************************************/

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::validateGrantSystem
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    semantic checking of GRANT SYSTEM statement
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::validateGrantSystem(qcStatement * aStatement)
{
#define IDE_FN "qdpGrant::validatGrantSystem"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::validateGrantSystem"));

    qdGrantParseTree    * sParseTree;
    qdPrivileges        * sPrivilege;
    idBool                sExist     = ID_FALSE;
    qdGrantees          * sGrantee;
    SChar                 sErrUserName[QC_MAX_OBJECT_NAME_LEN+1];
    SChar                 sErrRoleName[QC_MAX_OBJECT_NAME_LEN+1];
    UInt                  sErrPrivID;
    idBool                sIsRoleExist = ID_FALSE;
    idBool                sIsSysPrivExist = ID_FALSE;
    qdUserType            sUserType;
    UInt                  sPrivilegeListRoleCount = 0;
    UInt                  sRoleCount;

    sParseTree = (qdGrantParseTree*) aStatement->myPlan->parseTree;
    sParseTree->grantorID = QCG_GET_SESSION_USER_ID(aStatement);
    
    // check grantees
    IDE_TEST( checkGrantees(aStatement, sParseTree->grantees) != IDE_SUCCESS );
    
    /* PROJ-1812 ROLE */
    for (sPrivilege = sParseTree->privileges;
         sPrivilege != NULL;
         sPrivilege = sPrivilege->next)
    {
        /* GRANT ANY PRIVILEGES, GRANT ANY ROLE을 privilege list role, system priv
         * 존재 여부 체크 */
        if ( sPrivilege->privType == QDP_ROLE_PRIV )
        {
            /* GET ROLE ID */
            IDE_TEST(qcmUser::getUserIDAndType( aStatement,
                                                sPrivilege->privOrRoleName,
                                                &sPrivilege->privOrRoleID,
                                                &sUserType )
                     != IDE_SUCCESS);

            /* privilege list에 role type 검사 */
            IDE_TEST_RAISE ( sUserType != QDP_ROLE_TYPE, ERR_NOT_EXIST_ROLE );

            sIsRoleExist = ID_TRUE;

            sPrivilegeListRoleCount++;
        }
        else
        {
            sIsSysPrivExist = ID_TRUE;
        }
    }

    // check duplicate privileges
    IDE_TEST( checkDuplicatePrivileges( aStatement,
                                        sParseTree->privileges )
              != IDE_SUCCESS );

    // check grant
    if ( sParseTree->grantorID != QC_SYS_USER_ID &&
         sParseTree->grantorID != QC_SYSTEM_USER_ID )
    {
        /*  GRANT ANY PRIVILEGES 권한 검사 */
        if ( sIsSysPrivExist == ID_TRUE )
        {
            IDE_TEST( qdpRole::checkDDLGrantAnyPrivilegesPriv( aStatement )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing To Do */
        }

        /* GRANT ANY ROLE 권한 검사 */
        if ( sIsRoleExist == ID_TRUE )
        {
            IDE_TEST( qdpRole::checkDDLGrantAnyRolePriv( aStatement )
                      != IDE_SUCCESS );
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

    // To fix BUG-12188 check exist privileges
    // 각각의 system privilege, 각각의 grantee에 대해
    // 해당 권한이 존재하는지 검사.
    
    /* PROJ-1812 ROLE */
    for (sPrivilege = sParseTree->privileges;
         sPrivilege != NULL;
         sPrivilege = sPrivilege->next)
    {            
        for (sGrantee = sParseTree->grantees;
             sGrantee != NULL;
             sGrantee = sGrantee->next)
        {
            if ( sPrivilege->privType == QDP_ROLE_PRIV )
            {   
                /* ROLE TO PUBLIC 검사 */
                IDE_TEST_RAISE ( sGrantee->userOrRoleID == QC_PUBLIC_USER_ID,
                                 ERR_NOT_SUPPORT_ROLE_TO_PUBLIC );
                
                /* ROLE TO ROLE 검사 */
                IDE_TEST_RAISE( sGrantee->userType == QDP_ROLE_TYPE,
                                ERR_NOT_SUPPORT_ROLE_TO_ROLE );   
                
                /* ROLE 중복 부여 검사 */
                IDE_TEST( qcmPriv::checkRoleWithGrantor(
                              aStatement,
                              sParseTree->grantorID,
                              sGrantee->userOrRoleID,
                              sPrivilege->privOrRoleID,
                              &sExist )
                          != IDE_SUCCESS );

                if( sExist == ID_TRUE )
                {
                    QC_STR_COPY( sErrUserName, sGrantee->userOrRoleName );

                    QC_STR_COPY( sErrRoleName, sPrivilege->privOrRoleName );
                    
                    IDE_RAISE( ERR_EXIST_GRANT_ROLE );
                }
                else
                {
                    // Nothing to do.
                }
               
                /* USER TO ROLES MAX COUNT 검사 */
                IDE_TEST( qdpRole::getRoleCountByUserID( aStatement,
                                                         sGrantee->userOrRoleID,
                                                         &sRoleCount )
                          != IDE_SUCCESS );

                /* current userid, public 를 제외 한 것이 role 최대 부여 개수
                 * public는 sRoleCunt 포함
                 * current user는 QDD_USER_TO_ROLES_MAX_COUNT - 1 */
                IDE_TEST_RAISE( ( sPrivilegeListRoleCount + sRoleCount ) >
                                ( QDD_USER_TO_ROLES_MAX_COUNT - 1 ),            
                                ERR_USER_TO_ROLES_MAX_COUNT );
            }
            else
            {
                IDE_TEST( qcmPriv::checkSystemPrivWithGrantor(
                              aStatement,
                              sParseTree->grantorID,
                              sGrantee->userOrRoleID,
                              sPrivilege->privOrRoleID,
                              &sExist )
                          != IDE_SUCCESS );

                if( sExist == ID_TRUE )
                {
                    QC_STR_COPY( sErrUserName, sGrantee->userOrRoleName );
                    sErrPrivID = sPrivilege->privOrRoleID;
                    IDE_RAISE( ERR_EXIST_GRANT );
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_EXIST_GRANT );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDP_EXISTS_PRIVILEGE,
                                 sErrUserName, sErrPrivID ) );
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORT_ROLE_TO_PUBLIC );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_SUPPORTED_ROLE_TO_PUBLIC ) );
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORT_ROLE_TO_ROLE );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDP_NOT_SUPPORTED_ROLE_TO_ROLE ) );
    }
    IDE_EXCEPTION( ERR_USER_TO_ROLES_MAX_COUNT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_USER_TO_ROLES_MAX_COUNT ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_ROLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_ROLE, "" ) );
    }
    IDE_EXCEPTION( ERR_EXIST_GRANT_ROLE );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDP_EXISTS_ROLE,
                                 sErrUserName, sErrRoleName ) );
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::validateGrantObject
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    semantic checking of GRANT OBJECT statement
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::validateGrantObject(qcStatement * aStatement)
{
#define IDE_FN "qdpGrant::validateGrantObject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::validateGrantObject"));

    qdGrantParseTree    * sParseTree;
    qdPrivileges        * sPrivilege;
    qdGrantees          * sGrantee;
    
    sParseTree = (qdGrantParseTree*) aStatement->myPlan->parseTree;
    sPrivilege = sParseTree->privileges;
    sParseTree->grantorID = QCG_GET_SESSION_USER_ID(aStatement);

    // check object ( table, sequence, procedure )
    IDE_TEST( checkObjectInfo( aStatement,
                               sParseTree->userName,
                               sParseTree->objectName,
                               &sParseTree->userID,
                               &sParseTree->objectID,
                               sParseTree->objectType)
              != IDE_SUCCESS);

    // check grantees
    IDE_TEST( checkGrantees(aStatement, sParseTree->grantees)
              != IDE_SUCCESS );

    // check duplicate privileges
    IDE_TEST( checkDuplicatePrivileges( aStatement, sPrivilege )
              != IDE_SUCCESS );

    /* PROJ-1812 ROLE */
    for ( sGrantee = sParseTree->grantees;
          sGrantee != NULL;
          sGrantee = sGrantee->next )
    {        
        /* ROLE WITH GRANT OPTION 쳬크
         * GRANT SELECT ON T1 TO PUBLIC WITH GRANT OPTION은 제외 */
        IDE_TEST_RAISE( ( sGrantee->userType == QDP_ROLE_TYPE ) &&
                        ( sGrantee->userOrRoleID != QC_PUBLIC_USER_ID ) &&
                        ( sParseTree->grantOption == ID_TRUE ),
                        ERR_NOT_SUPPORT_ROLE_WITH_GRANT_OPTION );

    }
            
    // check privilege object
    if (sParseTree->grantorID != sParseTree->userID)
    {
        // In case of ALL PRIVILEGES
        // check each privileges separately
        if ( sParseTree->privileges->privOrRoleID ==
             QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO )
        {
            if ( sParseTree->objectType[0] == 'T')
            {
                // object type is TABLE
                IDE_TEST(haveObjectWithGrantOfTable(
                             aStatement,
                             sParseTree->grantorID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
            else if ( sParseTree->objectType[0] == 'S' )
            {
                // object type is SEQUENCE
                IDE_TEST(haveObjectWithGrantOfSequence(
                             aStatement,
                             sParseTree->grantorID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
            else if ( sParseTree->objectType[0] == 'P' )
            {
                // object type is PROCEDURE
                IDE_TEST(haveObjectWithGrantOfProcedure(
                             aStatement,
                             sParseTree->grantorID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
            // PROJ-1073 Package
            else if ( sParseTree->objectType[0] == 'A' )
            {
                // object type is PACKAGE
                IDE_TEST(haveObjectWithGrantOfPkg(
                             aStatement,
                             sParseTree->grantorID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
            else if (sParseTree->objectType[0] == 'D' )
            {
                // object type is DIRECTORY
                IDE_TEST(haveObjectWithGrantOfDirectory(
                             aStatement,
                             sParseTree->grantorID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
            // PROJ-1685
            else if (sParseTree->objectType[0] == 'Y' )
            {
                // object type is LIBRARY
                IDE_TEST(haveObjectWithGrantOfLibrary(
                             aStatement,
                             sParseTree->grantorID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
            else
            {
                IDE_DASSERT(0);
            }
        }
        else
        {
            for (sPrivilege = sParseTree->privileges;
                 sPrivilege != NULL;
                 sPrivilege = sPrivilege->next)
            {
                // SELECT *
                // FROM SYS_GRANT_OBJECT_
                // WHERE
                // ( GRANTEE_ID = PUBLIC
                //   OR GRANTEE_ID = sParseTree->grantorID )
                // AND PRIV_ID = sPrivilege->privOrRoleID
                // AND OBJ_ID = sParseTree->objectID
                // AND OBJ_TYPE = sParseTree->objectType
                // AND WITH_GRANT_OPTION = QCM_WITH_GRANT_OPTION_TRUE;

                IDE_TEST(haveObjectWithGrant(
                             aStatement,
                             sParseTree->grantorID,
                             sPrivilege->privOrRoleID,
                             sParseTree->objectID,
                             sParseTree->objectType) != IDE_SUCCESS);
            }
        }
    }

    // check useful privilege for object
    for (sPrivilege = sParseTree->privileges;
         sPrivilege != NULL;
         sPrivilege = sPrivilege->next)
    {
        IDE_TEST(checkUsefulPrivilege4Object( aStatement,
                                              sPrivilege,
                                              sParseTree->objectType,
                                              sParseTree->objectID )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_ROLE_WITH_GRANT_OPTION );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDP_NOT_SUPPORTED_ROLE_WITH_GRANT_OPTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::checkGrantees
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGrantees  - grantee list in parse tree.
 *
 * Description :
 *    semantic checking of grantees in GRANT, REVOKE statement
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::checkGrantees(
    qcStatement * aStatement,
    qdGrantees  * aGrantees)
{
#define IDE_FN "qdpGrant::checkGrantee"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::checkGrantee"));
    
    qdGrantees      * sGrantee;
    qdGrantees      * sGranteeNext;
    qcuSqlSourceInfo  sqlInfo;

    for (sGrantee = aGrantees; sGrantee != NULL; sGrantee = sGrantee->next)
    {
        /* PROJ-1812 ROLE */
        IDE_TEST( qcmUser::getUserIDAndType( aStatement,
                                             sGrantee->userOrRoleName,
                                             &sGrantee->userOrRoleID,
                                             &sGrantee->userType )
                  != IDE_SUCCESS );
        
        IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(aStatement) == sGrantee->userOrRoleID,
                        ERR_SELF_GRANT );
    }

    // check duplicate grantees
    for (sGrantee = aGrantees; sGrantee != NULL; sGrantee = sGrantee->next)
    {
        for (sGranteeNext = sGrantee->next;
             sGranteeNext != NULL;
             sGranteeNext = sGranteeNext->next)
        {
            if (sGrantee->userOrRoleID == sGranteeNext->userOrRoleID)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sGranteeNext->userOrRoleName );
                IDE_RAISE(ERR_DUPLICATE_GRANTEE);
            }
            else
            {
                /* Nothing To Do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SELF_GRANT)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_SELF_GRANT_OR_REVOKE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DUPLICATE_GRANTEE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_DUPLICATE_GRANTEE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::checkObjectInfo
 *
 * Argument :
 *    aStatement  - which have parse tree, iduMemory, session information.
 *    aTableInfo  - table information list in parse tree. ( OUT )
 *    aUserName   - current session user name. ( IN )
 *    aObjectName - object name in parse tree. ( IN )
 *    aUserID     - current session user ID. ( OUT )
 *    aObjID   - object ID in parse tree. ( OUT )
 *    aObjectType - object type in parse tree. ( IN )
 *    aTableSCN   - Table SCN. ( IN )
 *
 * Description :
 *    semantic checking of object in GRANT, REVOKE statement
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::checkObjectInfo(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aObjectName,
    UInt            * aUserID,
    qdpObjID        * aObjID,
    SChar           * aObjectType)
{
#define IDE_FN "qdpGrant::checkObjectInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::checkObjectInfo"));

    smSCN                sSCN;
    qcmTableInfo       * sTableInfo     = NULL;
    UInt                 sTableType;

    UInt                 sObjectType;
    void               * sObjectHandle   = NULL;

    UInt                 sObjectID;

    qcmDirectoryInfo     sDirectoryInfo;
    SChar                sObjNameBuffer[QC_MAX_OBJECT_NAME_LEN+1];
    idBool               sExist = ID_FALSE;
    qcmSynonymInfo       sSynonymInfo;

    qcuSqlSourceInfo     sqlInfo;

    if( aObjectType[0] == 'N' ) // not determined object type
    {
        IDE_TEST(qcmSynonym::resolveObject(
                    aStatement,
                    aUserName,
                    aObjectName,
                    &sSynonymInfo,
                    &sExist,
                    aUserID,
                    &sObjectType,
                    &sObjectHandle,
                    &sObjectID) != IDE_SUCCESS);
        
        if ( sExist == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, & aObjectName );
            IDE_RAISE(ERR_NOT_EXIST_OBJECT);
        }
        else
        {
            switch( sObjectType )
            {
                case QCM_OBJECT_TYPE_TABLE:
                    aObjectType[0] = 'T';
                    aObjectType[1] = '\0';

                    sTableType = smiGetTableFlag(sObjectHandle) & SMI_TABLE_TYPE_MASK;
                    
                    // grant는 사용자가 생성한 table에만 가능하다.
                    if( ( sTableType != SMI_TABLE_MEMORY ) &&
                        ( sTableType != SMI_TABLE_DISK ) &&
                        ( sTableType != SMI_TABLE_VOLATILE ) )
                    {
                        sqlInfo.setSourceInfo( aStatement, &aObjectName );
                        IDE_RAISE(ERR_NOT_SUPPORT_OBJECT);
                    }
                    
                    sSCN = smiGetRowSCN( sObjectHandle );

                    IDE_TEST(qcm::lockTableForDDLValidation(
                                aStatement,
                                sObjectHandle,
                                sSCN) != IDE_SUCCESS);

                    IDE_TEST( smiGetTableTempInfo( sObjectHandle,
                                                   (void**)&sTableInfo )
                              != IDE_SUCCESS );
                    *aObjID = sTableInfo->tableID;

                    // PROJ-2264 Dictionary table
                    // Dictionary table 에 대한 DDL 은 모두 금지한다.
                    IDE_TEST_RAISE( sTableInfo->isDictionary == ID_TRUE,
                                    ERR_CANNOT_DDL_DICTIONARY_TABLE );

                    // BUG-45366
                    if ( sTableInfo->tableType == QCM_INDEX_TABLE )
                    {
                        sqlInfo.setSourceInfo( aStatement, & aObjectName );
                        IDE_RAISE ( ERR_NOT_EXIST_OBJECT );
                    }
                    else
                    {
                        // nothing to do
                    }

                    break;
                case QCM_OBJECT_TYPE_SEQUENCE:
                    aObjectType[0] = 'S';
                    aObjectType[1] = '\0';
                    *aObjID = sObjectID;
                    break;
                case QCM_OBJECT_TYPE_PSM:
                    aObjectType[0] = 'P';
                    aObjectType[1] = '\0';
                    *aObjID = smiGetTableId( sObjectHandle );
                    break;
                // PROJ-1073 Package
                case QCM_OBJECT_TYPE_PACKAGE:
                    aObjectType[0] = 'A';
                    aObjectType[1] = '\0';
                    *aObjID = smiGetTableId( sObjectHandle );
                    break;
                // PROJ-1685
                case QCM_OBJECT_TYPE_LIBRARY:
                    aObjectType[0] = 'Y';
                    aObjectType[1] = '\0';
                    *aObjID = sObjectID;
                    break;
                default:
                    IDE_RAISE(ERR_NOT_EXIST_OBJECT);
                    break;
            }
        }
    }
    else if( aObjectType[0] == 'D' ) // directory type
    {
        QC_STR_COPY( sObjNameBuffer, aObjectName );

        IDE_TEST( qcmDirectory::getDirectory(
                      aStatement,
                      sObjNameBuffer,
                      idlOS::strlen(sObjNameBuffer),
                      &sDirectoryInfo,
                      &sExist )
                  != IDE_SUCCESS );

        if (sExist == ID_FALSE)
        {
            sqlInfo.setSourceInfo( aStatement, & aObjectName );
            IDE_RAISE(ERR_NOT_EXIST_OBJECT);
        }
        else
        {
            *aUserID = sDirectoryInfo.userID;
            *aObjID = (qdpObjID)sDirectoryInfo.directoryID;
        }
    }
    else
    {
        IDE_DASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_OBJECT)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_OBJECT)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_GRANT_NOT_SUPPORT_OBJ,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_CANNOT_DDL_DICTIONARY_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CANNOT_DDL_DML_DICTIONARY_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::checkUsefulPrivilege4Object
 *
 * Argument :
 *    aStatement  - which have parse tree, iduMemory, session information.
 *    aPrivileges - privileges infomation list in parse tree. ( IN )
 *    aObjectType - object type in parse tree. ( IN )
 *    aObjID   - object ID in parse tree. ( IN )
 *
 * Description :
 *     check useful privilege to grant
 *     TABLE - ALTER, INDEX, INSERT, DELETE, UPDATE, SELECT, LOCK
 *     SEQUENCE - ALTER, SELECT
 *     PSM - EXECUTE
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::checkUsefulPrivilege4Object(
    qcStatement     * aStatement,
    qdPrivileges    * aPrivilege,
    SChar           * aObjectType,
    qdpObjID          aObjID )
{
#define IDE_FN "qdpGrant::checkUsefulPrivilege4Object"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::checkUsefulPrivilege4Object"));

    qcmTableInfo        * sTableInfo;
    smSCN                 sSCN;
    qcuSqlSourceInfo      sqlInfo;
    void                * sTableHandle;

    if ( aObjectType[0] == 'T' )
    {
        // table or view
        IDE_TEST( qcm::getTableInfoByID( aStatement,
                                       aObjID,
                                       &sTableInfo,
                                       &sSCN,
                                       &sTableHandle )
                 != IDE_SUCCESS );

        IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                                sTableHandle,
                                                sSCN )
                 != IDE_SUCCESS );

        if ( ( aPrivilege->privOrRoleID != QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO ) &&
             ( aPrivilege->privOrRoleID == QCM_PRIV_ID_OBJECT_EXECUTE_NO ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aPrivilege->privOrRoleName );
            IDE_RAISE( ERR_NOT_ALLOWED_PRIV );
        }
        else
        {
            /* Nothing To Do */
        }
    }
    else if ( aObjectType[0] == 'S' )
    {
        // sequence
        if ( ( aPrivilege->privOrRoleID != QCM_PRIV_ID_OBJECT_ALTER_NO ) &&
             ( aPrivilege->privOrRoleID != QCM_PRIV_ID_OBJECT_SELECT_NO ) &&
             ( aPrivilege->privOrRoleID != QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aPrivilege->privOrRoleName );
            IDE_RAISE( ERR_NOT_ALLOWED_PRIV );
        }
        else
        {
            /* Nothing To Do */
        }
    }
    else if ( aObjectType[0] == 'P' )
    {
        // procedure
        // BUG-37304 grant all
        if ( ( aPrivilege->privOrRoleID != QCM_PRIV_ID_OBJECT_EXECUTE_NO ) &&
             ( aPrivilege->privOrRoleID != QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aPrivilege->privOrRoleName );
            IDE_RAISE( ERR_NOT_ALLOWED_PRIV );
        }
        else
        {
            /* Nothing To Do */
        }
    }
    // PROJ-1073 Package
    else if ( aObjectType[0] == 'A' )
    {
        // package 
        // BUG-37304 grant all
        if ( ( aPrivilege->privOrRoleID != QCM_PRIV_ID_OBJECT_EXECUTE_NO ) &&
             ( aPrivilege->privOrRoleID != QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aPrivilege->privOrRoleName );
            IDE_RAISE( ERR_NOT_ALLOWED_PRIV );
        }
        else
        {
            /* Nothing To Do */
        }
    }
    else if ( aObjectType[0] == 'D' )
    {
        // directory
        if ( ( aPrivilege->privOrRoleID != QCM_PRIV_ID_DIRECTORY_READ_NO ) &&
             ( aPrivilege->privOrRoleID != QCM_PRIV_ID_DIRECTORY_WRITE_NO ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aPrivilege->privOrRoleName );
            IDE_RAISE( ERR_NOT_ALLOWED_PRIV );
        }
        else
        {
            /* Nothing To Do */
        }
    }
    // PROJ-1685
    else if ( aObjectType[0] == 'Y' )
    {
        // library
        // BUG-37304 grant all
        if( ( aPrivilege->privOrRoleID != QCM_PRIV_ID_OBJECT_EXECUTE_NO ) &&
            ( aPrivilege->privOrRoleID != QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aPrivilege->privOrRoleName );
            IDE_RAISE( ERR_NOT_ALLOWED_PRIV );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED_PRIV )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDP_NOT_ALLOWED_PRIV,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::checkDuplicatePrivileges
 *
 * Argument :
 *    aStatement  - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check duplicate Privileges.
 *    Same Privilege is only once it will be able to use.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::checkDuplicatePrivileges(
    qcStatement     * aStatement,
    qdPrivileges    * aPrivilege)
{
#define IDE_FN "qdpGrant::checkDuplicatePrivileges"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::checkDuplicatePrivileges"));

    qdPrivileges        * sPrivilege;
    qdPrivileges        * sPrivilegeNext;
    qcuSqlSourceInfo      sqlInfo;

    sPrivilege = aPrivilege;

    while (sPrivilege != NULL)
    {
        for (sPrivilegeNext = sPrivilege->next;
             sPrivilegeNext != NULL;
             sPrivilegeNext = sPrivilegeNext->next)
        {
            if (sPrivilege->privOrRoleID == sPrivilegeNext->privOrRoleID)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sPrivilegeNext->privOrRoleName );
                IDE_RAISE(ERR_DUPLICATE_PRIVILEGE);
            }
            else
            {
                // ALL PRIVILEGES is used only once.
                // Can not use with other privileges together.
                if ((sPrivilege->privOrRoleID ==
                     QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO) ||
                    (sPrivilegeNext->privOrRoleID ==
                     QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO))
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sPrivilegeNext->privOrRoleName );
                    IDE_RAISE(ERR_DUPLICATE_PRIVILEGE);
                }
                else
                {
                    /* Nothing To Do */
                }
            }
        }

        sPrivilege = sPrivilege->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUPLICATE_PRIVILEGE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_DUPLICATE_PRIVILEGE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::haveObjectWithGrantOfSequence
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID
 *    aObjID     - object ID
 *    aObjType   - object Type ( 'T' - sequence )
 *
 * Description :
 *    grant all privilege on sequence.
 *    check granted ALTER, SELECT privilege WITH GRANT OPTION
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::haveObjectWithGrantOfSequence(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
#define IDE_FN "qdpGrant::haveObjectWithGrantOfSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::haveObjectWithGrantOfSequence"));

    // check ALTER privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_ALTER_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check SELECT privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_SELECT_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::haveObjectWithGrantOfTable
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID
 *    aObjID     - object ID
 *    aObjType   - object Type ( 'T' - table )
 *
 * Description :
 *    grant all privilege on sequence.
 *    check granted ALTER,
 *                  DELETE,
 *                  INDEX,
 *                  INSERT,
 *                  SELECT,
 *                  REFERENCE,
 *                  UPDATE privilege WITH GRANT OPTION
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::haveObjectWithGrantOfTable(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
#define IDE_FN "qdpGrant::haveObjectWithGrantOfTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::haveObjectWithGrantOfTable"));

    // check ALTER privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_ALTER_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check DELETE privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_DELETE_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check INDEX privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_INDEX_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check INSERT privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_INSERT_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check REFERENCES privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_REFERENCES_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check SELECT privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_SELECT_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check UPDATE privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_UPDATE_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::haveObjectWithGrantOfProcedure
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID
 *    aObjID     - object ID
 *    aObjType   - object Type ( 'P' - procedure )
 *
 * Description :
 *    grant all privilege on sequence.
 *    check granted EXECUTE privilege WITH GRANT OPTION
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::haveObjectWithGrantOfProcedure(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
#define IDE_FN "qdpGrant::haveObjectWithGrantOfProcedure"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(
                        "qdpGrant::haveObjectWithGrantOfProcedure"));

    // check EXECUTE privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::haveObjectWithGrant
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID.
 *    aPrivID    - privilege ID.
 *    aObjID     - object ID.
 *    aObjType   - object type.
 *
 *
 * Description :
 *    SELECT *
 *        FROM SYS_GRANT_OBJECT_
 *      WHERE
 *          ( GRANTEE_ID = PUBLIC OR GRANTEE_ID = sParseTree->grantorID )
 *        AND PRIV_ID = sPrivilege->privOrRoleID
 *        AND OBJ_ID = sParseTree->objectID
 *        AND OBJ_TYPE = sParseTree->objectType
 *        AND WITH_GRANT_OPTION = QCM_WITH_GRANT_OPTION_TRUE;
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::haveObjectWithGrant(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    UInt              aPrivID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
#define IDE_FN "qdpGrant::haveObjectWithGrant"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::haveObjectWithGrant"));

    idBool    sExist = ID_FALSE;
    
    IDE_TEST(qcmPriv::checkObjectPrivWithGrantOption(
                 aStatement,
                 QC_PUBLIC_USER_ID,
                 aPrivID,
                 aObjID,
                 aObjType,
                 QCM_WITH_GRANT_OPTION_TRUE,
                 &sExist)
             != IDE_SUCCESS);

    if (sExist == ID_FALSE)
    {
        IDE_TEST(qcmPriv::checkObjectPrivWithGrantOption(
                     aStatement,
                     aGranteeID,
                     aPrivID,
                     aObjID,
                     aObjType,
                     QCM_WITH_GRANT_OPTION_TRUE,
                     &sExist)
                 != IDE_SUCCESS);

        IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT);
    {
        // BUG-37775
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_GRANT_INSUFFICIENT_PRIVILEGES ) );
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
 *    qdpGrant::executeGrantSystem
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    execution of GRANT SYSTEM statement
 *        - insert into SYS_GRANT_SYSTEM_
 *
 * In case GRANT ALL PRIVILEGES :
 *    One row insert into SYS_GRANT_SYSTEM_ PRIV_ID = 1
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::executeGrantSystem(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      GRANT privilege_list TO grantees 수행
 *
 * Implementation :
 *      1. SYS_GRANT_SYSTEM_ 메타 테이블에 권한 입력
 *
 ***********************************************************************/

#define IDE_FN "qdd::executeGrantSystem"

    qdGrantParseTree    * sParseTree;
    qdPrivileges        * sPrivilege;
    qdGrantees          * sGrantee;
    SChar               * sSqlStr;
    vSLong                sRowCnt;

    sParseTree = (qdGrantParseTree*) aStatement->myPlan->parseTree;

    IDU_FIT_POINT( "qdpGrant::executeGrantSystem::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

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
                /* insert into SYS_USER_ROLES_ */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_USER_ROLES_ VALUES ( "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"')",
                                 sParseTree->grantorID,
                                 sGrantee->userOrRoleID,
                                 sPrivilege->privOrRoleID );

                IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                            sSqlStr,
                                            & sRowCnt )
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
            }
            else
            {
                /* insert into SYS_GRANT_SYSTEM_ */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"')",
                                 sParseTree->grantorID,
                                 sGrantee->userOrRoleID,
                                 sPrivilege->privOrRoleID);
                
                IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                            sSqlStr,
                                            & sRowCnt )
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
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
 *    qdpGrant::executeGrantObject
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    execution of GRANT OBJECT statement
 *        - insert into SYS_GRANT_OBJECT_
 *
 * In case GRANT ALL (PRIVILEGES) :
 *    each privilege insert into SYS_GRANT_OBJECT_
 *
 *    table     - ALTER, DELETE, INDEX, INSERT, SELECT, REFERENCE, UPDATE.
 *    sequence  - ALTER, SELECT.
 *    procedure - EXECUTE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::executeGrantObject(qcStatement * aStatement)
{
#define IDE_FN "qdd::executeGrantObject"

    qdGrantParseTree    * sParseTree;
    qdPrivileges        * sPrivilege;
    qdGrantees          * sGrantee;
    UInt                  sWithGrantOption;
    qcmTableInfo        * sOldTableInfo    = NULL;
    qcmTableInfo        * sNewTableInfo    = NULL;
    smSCN                 sSCN;
    smOID                 sTableOID;
    void                * sTableHandle;
    qcuSqlSourceInfo      sqlInfo;
    UInt                  sSqlCode;
    UInt                  sState = 0;

    sParseTree = (qdGrantParseTree*) aStatement->myPlan->parseTree;

    if (sParseTree->grantOption == ID_TRUE)
    {
        sWithGrantOption = QCM_WITH_GRANT_OPTION_TRUE;
    }
    else
    {
        sWithGrantOption = QCM_WITH_GRANT_OPTION_FALSE;
    }

    sPrivilege = sParseTree->privileges;

    // when object type is table or view, get sOldTableInfo
    if (sParseTree->objectType[0] == 'T')
    {
        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sParseTree->objectID,
                                       &sOldTableInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);

        IDE_TEST(qcm::validateAndLockTable(aStatement,
                                           sTableHandle,
                                           sSCN,
                                           SMI_TABLE_LOCK_X)
                 != IDE_SUCCESS);
        sState = 1;
        // PROJ-1567
        // BUG-17503
        // IDE_TEST_RAISE(sOldTableInfo->replicationCount > 0,
        //                ERR_DDL_WITH_REPLICATED_TABLE);
    }
    else
    {
        // Sequence or Procedure or Directory, Nothing to do
    }

    // insert ALL PRIVILEGES into SYS_GRANT_OBJECT_
    if (sPrivilege->privOrRoleID == QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO)
    {
        for (sGrantee = sParseTree->grantees;
             sGrantee != NULL;
             sGrantee = sGrantee->next)
        {
            if (sParseTree->objectType[0] == 'T')
            {
                // TABLE
                // insert ALTER privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_ALTER_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert DELETE privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_DELETE_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert INDEX privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_INDEX_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert INSERT privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_INSERT_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert REFERENCES privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_REFERENCES_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert SELECT privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_SELECT_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert UPDATE privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_UPDATE_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
            else if(sParseTree->objectType[0] == 'S')
            {
                // SEQUENCE
                // insert ALTER privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_ALTER_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert SELECT privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_SELECT_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
            else if(sParseTree->objectType[0] == 'P')
            {
                // PROCEDURE
                // insert EXECUTE privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
            // PROJ-1073 Package
            else if(sParseTree->objectType[0] == 'A')
            {
                // PACKAGE
                // insert EXECUTE privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
            else if(sParseTree->objectType[0] == 'D')
            {
                // DIRECTORY
                // insert READ privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_DIRECTORY_READ_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);

                // insert WRITE privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_DIRECTORY_WRITE_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
            // PROJ-1685
            else if(sParseTree->objectType[0] == 'Y')
            {
                // LIBRARY 
                // insert EXECUTE privilege
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
                             QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                             sParseTree->userID,
                             sParseTree->objectID,
                             sParseTree->objectType,
                             sWithGrantOption)
                         != IDE_SUCCESS);
            }
            else
            {
                IDE_DASSERT(0);
            }
        }
    }
    else
    {
        // insert each Privileges into SYS_GRANT_OBJECT_
        for (sPrivilege = sParseTree->privileges;
             sPrivilege != NULL;
             sPrivilege = sPrivilege->next)
        {
            for (sGrantee = sParseTree->grantees;
                 sGrantee != NULL;
                 sGrantee = sGrantee->next)
            {
                IDE_TEST(insertObjectPrivIntoMeta(
                             aStatement,
                             sParseTree->grantorID,
                             sGrantee,
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
        // BUG-14509
        // 관련 psm들이 rebuild될 수 있도록 invalid상태로 변경.
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sOldTableInfo->name,
                      idlOS::strlen((SChar*)sOldTableInfo->name),
                      QS_TABLE ) != IDE_SUCCESS );

        // PROJ-1073 Package
        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated(
                      aStatement,
                      sParseTree->userID,
                      sOldTableInfo->name,
                      idlOS::strlen((SChar*)sOldTableInfo->name),
                      QS_TABLE ) != IDE_SUCCESS );

        // table or sequence
        // rebuild qcmTableInfo
        IDE_TEST(qcm::touchTable( QC_SMI_STMT( aStatement ),
                                  sParseTree->objectID,
                                  SMI_TBSLV_DDL_DML )
            != IDE_SUCCESS);

        // rebuild tableInfo
        sTableOID = smiGetTableId(sOldTableInfo->tableHandle);

        IDE_TEST(qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                              sParseTree->objectID,
                                              sTableOID)
                 != IDE_SUCCESS);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sParseTree->objectID,
                                       &sNewTableInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);

        // BUG-11266
        IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                     aStatement,
                     sNewTableInfo->tableOwnerID,
                     sNewTableInfo->name,
                     idlOS::strlen( (SChar *)sNewTableInfo->name ),
                     QS_TABLE)
                 != IDE_SUCCESS);
    }
    else if (sParseTree->objectType[0] == 'P')
    {
        // procedure
        // rebuild qsxProcInfo
        IDE_TEST(qsx::rebuildQsxProcInfoPrivilege( aStatement,
                                                   sParseTree->objectID )
                 != IDE_SUCCESS);

        // BUG-11266
        IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                     aStatement,
                     sParseTree->userID,
                     (SChar *) (sParseTree->objectName.stmtText +
                                sParseTree->objectName.offset),
                     sParseTree->objectName.size,
                     QS_PROC)
                 != IDE_SUCCESS);
   
        IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                     aStatement,
                     sParseTree->userID,
                     (SChar *) (sParseTree->objectName.stmtText +
                                sParseTree->objectName.offset),
                     sParseTree->objectName.size,
                     QS_FUNC)
                 != IDE_SUCCESS);
    }
    // PROJ-1073 Package
    else if (sParseTree->objectType[0] == 'A')
    {
        // package 
        // rebuild qsxPkgInfo
        IDE_TEST(qsx::rebuildQsxPkgInfoPrivilege( aStatement,
                                                  sParseTree->objectID )
                 != IDE_SUCCESS);
      
        // BUG-11266
        IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
                     aStatement,
                     sParseTree->userID,
                     (SChar *) (sParseTree->objectName.stmtText +
                                sParseTree->objectName.offset),
                     sParseTree->objectName.size,
                     QS_PKG)
                 != IDE_SUCCESS);
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

    (void)qcm::destroyQcmTableInfo( sOldTableInfo );

    return IDE_SUCCESS;

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

    if ( sState > 0 )
    {
        (void)qcm::destroyQcmTableInfo( sNewTableInfo );

        qcmPartition::restoreTempInfo( sOldTableInfo,
                                       NULL,
                                       NULL );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpGrant::grantDefaultPrivs4CreateUser
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGrantorID - grantor ID
 *    aGranteeID - grantee ID ( A new created user ID )
 *
 * Description :
 *    This function is called to grant DEFUALT PRIVILEGES in creating a user.
 *        - grant SYSTEM PRIVILEGE : CREATE SESSION
 *        - grant SYSTEM PRIVILEGE : CREATE TABLE
 *        - grant SYSTEM PRIVILEGE : CREATE SEQUENCE
 *        - grant SYSTEM PRIVILEGE : CREATE PROCEDURE
 *        - grant SYSTEM PRIVILEGE : CREATE VIEW
 *        - grant SYSTEM PRIVILEGE : CREATE TRIGGER
 *        - grant SYSTEM PRIVILEGE : CREATE SYNONYM
 *        - grant SYSTEM PRIVILEGE : CREATE MATERIALIZED VIEW
 *        - grant SYSTEM PRIVILEGE : CREATE LIBRARY 
 *        - grant SYSTEM PRIVILEGE : CREATE DATABASE LINK
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpGrant::grantDefaultPrivs4CreateUser(
    qcStatement * aStatement,
    UInt          aGrantorID,
    UInt          aGranteeID)
{
/***********************************************************************
 *
 * Description :
 *      CREATE USER 시 디폴트 권한 입력
 *
 * Implementation :
 *      1. SYS_GRANT_SYSTEM_ 메타 테이블에 디폴트 권한 입력
 *
 ***********************************************************************/

#define IDE_FN "qdpGrant::grantDefaultPrivs4CreateUser"

    UInt                  sPrivID;
    SChar               * sSqlStr;
    vSLong                sRowCnt;

    //----------------------------------------------
    // grant SYSTEM PRIVILEGE : CREATE SESSION
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_SESSION_NO;

    IDU_FIT_POINT( "qdpGrant::grantDefaultPrivs4CreateUser::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
    
    //----------------------------------------------
    // grant SYSTEM PRIVILEGE : CREATE TABLE
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_TABLE_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
    
    //----------------------------------------------
    // grant SYSTEM PRIVILEGE : CREATE SEQUENCE
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_SEQUENCE_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    //----------------------------------------------
    // grant SYSTEM PRIVILEGE : CREATE PROCEDURE
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_PROCEDURE_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    //----------------------------------------------
    // grant SYSTEM PRIVILEGE : CREATE VIEW
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_VIEW_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID);

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS );

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    //----------------------------------------------
    // To Fix PR-10617
    // grant SYSTEM PRIVILEGE : CREATE TRIGGER
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID);

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    //----------------------------------------------
    // grant SYSTEM PRIVILEGE : CREATE SYNONYM
    //----------------------------------------------
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_SYNONYM_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     sPrivID);

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    /* grant SYSTEM PRIVILEGE : CREATE MATERIALIZED VIEW */
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_MATERIALIZED_VIEW_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"' )",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    // PROJ-1685
    /* grant SYSTEM PRIVILEGE : CREATE LIBRARY */
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_LIBRARY_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"' )",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    /*
     * PROJ-1832 New database link
     * 
     * GRANT SYSTEM PRIVILEGE : CREATE DATABASE LINK
     */ 
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_DATABASE_LINK_NO;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_SYSTEM_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"' )",
                     aGrantorID,
                     aGranteeID,
                     sPrivID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

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
 *    qdpGrant::insertObjectPrivIntoMeta
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
 *    INSERT INTO SYS_GRANT_OBJECT_VALUES (
 *        aGrantorID, aGranteeID, aPrivID, aUserID,
 *        aObjID, aObjectType, aWithGrantOption );
 *
 * ---------------------------------------------------------------------------*/
IDE_RC qdpGrant::insertObjectPrivIntoMeta(
    qcStatement     * aStatement,
    UInt              aGrantorID,
    qdGrantees      * aGrantee,
    UInt              aPrivID,
    UInt              aUserID,
    qdpObjID          aObjID,
    SChar           * aObjectType,
    UInt              aWithGrantOption)
{
/***********************************************************************
 *
 * Description :
 *      OBJECT 권한 입력
 *
 * Implementation :
 *      1. SYS_GRANT_OBJECT_ 메타 테이블에 주어진 권한 입력
 *
 ***********************************************************************/

#define IDE_FN "qdpGrant::insertObjectPrivIntoMeta"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdpGrant::insertObjectPrivIntoMeta"));

    SChar               * sSqlStr;
    vSLong                sRowCnt;
    SChar                 sErrUserName[QC_MAX_OBJECT_NAME_LEN+1];
    UInt                  sErrPrivID;
    SInt                  sRc = 0;
    UInt                  sErrorCode;

    IDU_FIT_POINT( "qdpGrant::insertObjectPrivIntoMeta::alloc::sSqlStr",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_GRANT_OBJECT_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "VARCHAR'%s', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGrantee->userOrRoleID,
                     aPrivID,
                     aUserID,
                     QCM_OID_TO_BIGINT( aObjID ),
                     aObjectType,
                     aWithGrantOption );

    sRc = qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                             sSqlStr,
                             & sRowCnt );

    if( sRc != 0 )
    {
        sErrorCode = ideGetErrorCode();
        if( sErrorCode == smERR_ABORT_smnUniqueViolation )
        {
            QC_STR_COPY( sErrUserName, aGrantee->userOrRoleName );
            sErrPrivID = aPrivID;
            IDE_RAISE( ERR_EXISTS );
        }
        else
        {
            IDE_RAISE(ERR_PASS);
        }
    }
    else
    {
        // Nothing to do.
    }


    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION(ERR_EXISTS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_EXISTS_PRIVILEGE,
                                sErrUserName, sErrPrivID ));
    }
    IDE_EXCEPTION(ERR_PASS)
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpGrant::haveObjectWithGrantOfDirectory(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
/***********************************************************************
 *
 * Description : directory에 대한 object의 grant 권한이 존재하는지 검사
 *
 * Implementation :
 *     1. read 또는 write에 대한 grant 권한이 존재하는지 검사.
 *
 ***********************************************************************/

#define IDE_FN "qdpGrant::haveObjectWithGrantOfDirectory"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    // check READ privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_DIRECTORY_READ_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    // check WRITE privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_DIRECTORY_WRITE_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// PROJ-1685
IDE_RC qdpGrant::haveObjectWithGrantOfLibrary(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
    // check EXECUTE privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1073 Package
IDE_RC qdpGrant::haveObjectWithGrantOfPkg(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType)
{
    // check EXECUTE privilege
    IDE_TEST(haveObjectWithGrant(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                 aObjID,
                 aObjType) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : PROJ-1812 ROLE
 *      CREATE USER 시 PUBLIC ROLE을 부여
 *
 * Implementation :
 *      
 *
 ***********************************************************************/
IDE_RC qdpGrant::grantPublicRole4CreateUser( qcStatement * aStatement,
                                             UInt          aGrantorID,
                                             UInt          aGranteeID )
{
    SChar   * sSqlStr;
    vSLong    sRowCnt;
    
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_USER_ROLES_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aGrantorID,
                     aGranteeID,
                     QC_PUBLIC_USER_ID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt )
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}
