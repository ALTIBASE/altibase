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
 * $Id: qdpPrivilege.cpp 88963 2020-10-19 03:33:18Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qmmParseTree.h>
#include <qmsParseTree.h>
#include <qdpGrant.h>
#include <qcmUser.h>
#include <qcmPriv.h>
#include <qcuSqlSourceInfo.h>
#include <qdpPrivilege.h>
#include <qcgPlan.h>

/***********************************************************************
 * VALIDATE
 **********************************************************************/

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLCreateTablePriv
 *
 * Argument :
 *    aStatement    - which have parse tree, iduMemory, session information.
 *    aTableOwnerID - table owner ID in parse tree
 *
 * Description :
 *    check privilege to execute CREATE TABLE statement
 *        - CREATE TABLE ( system privilege )
 *        - CREATE ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateTablePriv(
    qcStatement * aStatement,
    UInt          aTableOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLCreateTablePriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == aTableOwnerID)
        {
            // check system privilege : CREATE TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY TABLE
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_CREATE_ANY_TABLE);
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_CREATE_ANY_TABLE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aTableOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TABLE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLCreateViewPriv
 *
 * Argument :
 *    aStatement   - which have parse tree, iduMemory, session information.
 *    aViewOwnerID - view owner ID in parse tree
 *
 * Description :
 *    check privilege to execute CREATE VIEW statement
 *        - CREATE VIEW ( system privilege )
 *        - CREATE ANY VIEW ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateViewPriv(
    qcStatement * aStatement )
{
#define IDE_FN "qdpPrivilege::checkDDLCreateViewPriv"

    qdTableParseTree    * sParseTree;
    UInt                  sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    UInt                  sViewOwnerID;
    idBool                sExist = ID_FALSE;

    sParseTree   = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sViewOwnerID = sParseTree->userID;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == sViewOwnerID)
        {
            // check system privilege : CREATE VIEW
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_VIEW_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY VIEW
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_CREATE_ANY_VIEW);
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY VIEW
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_CREATE_ANY_VIEW);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( sViewOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_VIEW);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_VIEW_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLCreateIndexPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute CREATE INDEX statement
 *        - INDEX ( object privilege )
 *        - CREATE ANY INDEX ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateIndexPriv(
    qcStatement  * aStatement,
    qcmTableInfo * aTableInfo,
    UInt           aIndexOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLCreateIndexPriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if( (sGranteeID != aTableInfo->tableOwnerID) ||
            (sGranteeID != aIndexOwnerID) )
        {
            // check object privilege : INDEX
            IDE_TEST(qcmPriv::checkPrivilegeInfo(
                         aTableInfo->privilegeCount,
                         aTableInfo->privilegeInfo,
                         sGranteeID,
                         QCM_PRIV_ID_OBJECT_INDEX_NO,
                         &sQcmPriv)
                     != IDE_SUCCESS);

            if (sQcmPriv == NULL)
            {
                // check system privilege : CREATE ANY INDEX
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_INDEX_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_INDEX_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_CREATE_ANY_INDEX);
                }
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aTableInfo->tableOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_INDEX_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLCreateSequencePriv
 *
 * Argument :
 *    aStatement    - which have parse tree, iduMemory, session information.
 *    aTableOwnerID - table owner ID in parse tree
 *
 * Description :
 *    check privilege to execute CREATE SEQUENCE statement
 *        - CREATE SEQUENCE ( system privilege )
 *        - CREATE ANY SEQUENCE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateSequencePriv(
    qcStatement * aStatement,
    UInt          aSequenceOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLCreateSequencePriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == aSequenceOwnerID)
        {
            // check system privilege : CREATE SEQUENCE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_SEQUENCE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY SEQUENCE
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_CREATE_ANY_SEQUENCE);
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY SEQUENCE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_CREATE_ANY_SEQUENCE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aSequenceOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_SEQUENCE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_SEQUENCE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLCreatePSMPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aPSMOwnerID - Procedure owner ID in parse tree
 *
 * Description :
 *    check privilege to execute CREATE PROCEDURE statement
 *        - CREATE PROCEDURE ( system privilege )
 *        - CREATE ANY PROCEDURE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreatePSMPriv(
    qcStatement * aStatement,
    UInt          aPSMOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLCreatePSMPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == aPSMOwnerID)
        {
            // check system privilege : CREATE PROCEDURE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_PROCEDURE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY PROCEDURE
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_CREATE_ANY_PROCEDURE);
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY PROCEDURE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_CREATE_ANY_PROCEDURE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aPSMOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_PROCEDURE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_PROCEDURE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdpPrivilege::checkDDLCreateTriggerPriv( qcStatement * aStatement,
                                         UInt          aTriggerOwnerID )
{
/***********************************************************************
 *
 * Description :
 *
 *    CREATE TRIGGER를 위한 권한 검사를 수행함.
 *    check privilege to execute CREATE TRIGGER statement
 *        - CREATE TRIGGER ( system privilege )
 *        - CREATE ANY TRIGGER ( system privilege )
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLCreateTriggerPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == aTriggerOwnerID)
        {
            // check system privilege : CREATE TRIGGER
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY TRIGGER
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE( sExist == ID_FALSE,
                                    ERR_NO_GRANT_CREATE_ANY_TRIGGER );
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY TRIGGER
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE( sExist == ID_FALSE,
                                ERR_NO_GRANT_CREATE_ANY_TRIGGER );
            }
        }
    }

    // To Fix PR-10709
    // SYSTEM_ User에는 Trigger를 생성할 수 없음.
    IDE_TEST_RAISE ( aTriggerOwnerID == QC_SYSTEM_USER_ID,
                     err_invalid_trigger );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_TRIGGER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TRIGGER_STR));
    }
    IDE_EXCEPTION(err_invalid_trigger);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_NO_DDL_FOR_META ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdpPrivilege::checkDDLCreateTriggerTablePriv( qcStatement * aStatement,
                                              UInt          aTableOwnerID )
{
/***********************************************************************
 *
 * Description :
 *
 *    To Fix PR-10618
 *    CREATE TRIGGER를 위해서는 Table Owner에 대한 검사가 필요함.
 *    check privilege to execute CREATE TRIGGER statement
 *        - CREATE TRIGGER ( system privilege )
 *        - CREATE ANY TRIGGER ( system privilege )
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLCreateTriggerTablePriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == aTableOwnerID)
        {
            // check system privilege : CREATE TRIGGER
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY TRIGGER
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE( sExist == ID_FALSE,
                                    ERR_NO_GRANT_CREATE_ANY_TRIGGER );
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY TRIGGER
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE( sExist == ID_FALSE,
                                ERR_NO_GRANT_CREATE_ANY_TRIGGER );
            }
        }
    }

    // To Fix PR-10709
    // SYSTEM_ User에는 Trigger를 생성할 수 없음.
    IDE_TEST_RAISE ( aTableOwnerID == QC_SYSTEM_USER_ID,
                     err_invalid_trigger );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_TRIGGER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TRIGGER_STR));
    }
    IDE_EXCEPTION(err_invalid_trigger);
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_NO_DDL_FOR_META ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLCreateUserPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute CREATE USER statement
 *        - CREATE USER ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateUserPriv(qcStatement * aStatement)
{
#define IDE_FN "qdpPrivilege::checkDDLCreateUserPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : CREATE USER
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_CREATE_USER_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_USER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_CREATE_USER);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_USER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_USER_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLReplicationPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute CREATE REPLICATION,
 *                               ALTER REPLICATION,
 *                               DROP REPLICATION statement
 *        - Only SYS user
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLReplicationPriv(qcStatement * aStatement)
{
#define IDE_FN "qdpPrivilege::checkDDLReplicationPriv"

    // check Grant : A Current user is SYS or SYSTEM user
    IDE_TEST_RAISE( !( QCG_GET_SESSION_USER_ID(aStatement) == QC_SYS_USER_ID || 
                     QCG_GET_SESSION_USER_ID(aStatement) == QC_SYSTEM_USER_ID ),
                    ERR_NO_GRANT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QRC_NO_GRANT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterTablePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute ALTER TABLE statement
 *        - ALTER ( object privilege )
 *        - ALTER ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLAlterTablePriv(
    qcStatement  * aStatement,
    qcmTableInfo * aTableInfo)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterTablePriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableInfo->tableOwnerID))
    {
        // check object privilege : ALTER
        IDE_TEST(qcmPriv::checkPrivilegeInfo(
                     aTableInfo->privilegeCount,
                     aTableInfo->privilegeInfo,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_ALTER_NO,
                     &sQcmPriv)
                 != IDE_SUCCESS);

        if (sQcmPriv == NULL)
        {
            // check system privilege : ALTER ANY TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_ALTER_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_ALTER_ANY_TABLE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aTableInfo->tableOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_ANY_TABLE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterIndexPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *    aIndexOwnerID - index owner ID in parse tree.
 *
 * Description :
 *    check privilege to execute ALTER INDEX statement
 *        - INDEX ( object privilege )
 *        - ALTER ANY INDEX ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLAlterIndexPriv(
    qcStatement  * aStatement,
    qcmTableInfo * aTableInfo,
    UInt           aIndexOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterIndexPriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID != aIndexOwnerID)
        {
            // check object privilege : INDEX
            IDE_TEST(qcmPriv::checkPrivilegeInfo(
                         aTableInfo->privilegeCount,
                         aTableInfo->privilegeInfo,
                         sGranteeID,
                         QCM_PRIV_ID_OBJECT_INDEX_NO,
                         &sQcmPriv)
                     != IDE_SUCCESS);

            if (sQcmPriv == NULL)
            {
                // check system privilege : ALTER ANY INDEX
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_ALTER_ANY_INDEX_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_ALTER_ANY_INDEX_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_ALTER_ANY_INDEX);
                }
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aIndexOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_ANY_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_ANY_INDEX_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterSequencePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute ALTER SEQUENCE statement
 *        - ALTER ( object privilege )
 *        - ALTER ANY SEQUENCE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLAlterSequencePriv(
    qcStatement     * aStatement,
    qcmSequenceInfo * aSequenceInfo)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterSequencePriv"

    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aSequenceInfo->sequenceOwnerID))
    {
        // check object privilege : ALTER
        IDE_TEST(qcmPriv::checkObjectPriv(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_ALTER_NO,
                     aSequenceInfo->sequenceID,
                     (SChar*)"S",
                     &sExist)
                 != IDE_SUCCESS);

        if ( sExist == ID_FALSE )
        {
            // BUG-38451 check PUBLIC OBJECT_ALTER priv.
            IDE_TEST( qcmPriv::checkObjectPriv( aStatement,
                                                QC_PUBLIC_USER_ID,
                                                QCM_PRIV_ID_OBJECT_ALTER_NO,
                                                aSequenceInfo->sequenceID,
                                                (SChar*)"S",
                                                &sExist )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        if (sExist == ID_FALSE)
        {
            // check system privilege : ALTER ANY SEQUENCE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_ANY_SEQUENCE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_ALTER_ANY_SEQUENCE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_ALTER_ANY_SEQUENCE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aSequenceInfo->sequenceOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_ANY_SEQUENCE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_ANY_SEQUENCE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterPSMPrivePriv
 *
 * Argument :
 *    aStatement    - which have parse tree, iduMemory, session information.
 *    aTableOwnerID - Procedure owner ID in parse tree
 *
 * Description :
 *    check privilege to execute REPLACE PROCEDURE statement
 *        - ALTER ANY PROCEDURE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLAlterPSMPriv(
    qcStatement * aStatement,
    UInt          aPSMOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterPSMPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID != aPSMOwnerID)
        {
            // check system privilege : ALTER ANY PROCEDURE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_ANY_PROCEDURE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_ALTER_ANY_PROCEDURE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_ALTER_ANY_PROCEDURE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aPSMOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_ANY_PROCEDURE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_ANY_PROCEDURE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdpPrivilege::checkDDLAlterTriggerPriv( qcStatement * aStatement,
                                        UInt          aTriggerOwnerID )
{
/***********************************************************************
 *
 * Description :
 *    ALTER TRIGGER를 위한 Privilege 검사
 *
 *    check privilege to execute ALTER TRIGGER statement
 *        - ALTER ANY TRIGGER ( system privilege )
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLAlterTriggerPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID != aTriggerOwnerID)
        {
            // check system privilege : ALTER ANY TRIGGER
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_ANY_TRIGGER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_ALTER_ANY_TRIGGER_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_ALTER_ANY_TRIGGER);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aTriggerOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_ANY_TRIGGER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_ANY_TRIGGER_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterUserPriv
 *
 * Argument :
 *    aStatement  - which have parse tree, iduMemory, session information.
 *    aRealUserID - current session UserID
 *
 * Description :
 *    check privilege to execute ALTER USER statement
 *        - ALTER USER ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLAlterUserPriv(
    qcStatement * aStatement,
    UInt          aRealUserID)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterUserPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != aRealUserID) &&
        (sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : ALTER USER
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_ALTER_USER_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_USER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_ALTER_USER);
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aRealUserID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_USER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_USER_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterSystemPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute ALTER SYSTEM statement
 *        - ALTER SYSTEM ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLAlterSystemPriv(qcStatement * aStatement)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterSystemPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : ALTER SYSTEM
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_ALTER_SYSTEM_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_SYSTEM_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_ALTER_SYSTEM);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_SYSTEM);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_SYSTEM_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLAlterSessionPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID in parse tree.
 *
 * Description :
 *    check privilege to execute CONNECT statement
 *        - ALTER SESSION ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateSessionPriv(
    qcStatement * aStatement,
    UInt          aGranteeID)
{
#define IDE_FN "qdpPrivilege::checkDDLCreateSessionPriv"

    idBool      sExist     = ID_FALSE;

    // check system privilege : ALTER SESSION
    if ((aGranteeID != QC_SYSTEM_USER_ID) &&
        (aGranteeID != QC_SYS_USER_ID))
    {
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     aGranteeID,
                     QCM_PRIV_ID_SYSTEM_CREATE_SESSION_NO,
                     &sExist)
                 != IDE_SUCCESS);

        /*
          if (sExist == ID_FALSE)

          // check system privilege
          // in case PUBLIC or All PRIVILEGES
          IDE_TEST(checkSystemPrivAllAndPublic(
          aStatement,
          aGranteeID,
          QCM_PRIV_ID_SYSTEM_CREATE_SESSION_NO,
          &sExist)
          != IDE_SUCCESS);
        */

        IDE_TEST_RAISE(sExist == ID_FALSE,
                       ERR_NO_GRANT_CREATE_SESSION);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_SESSION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_SESSION_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLDropTablePriv
 *
 * Argument :
 *    aStatement    - which have parse tree, iduMemory, session information.
 *    aTableOwnerID - table owner ID in parse tree
 *
 * Description :
 *    check privilege to execute DROP TALE statement
 *        - owner of table
 *        - DROP ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropTablePriv(
    qcStatement   * aStatement,
    UInt            aTableOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropTablePriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableOwnerID))
    {
        // check system privilege : DROP ANY TABLE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_ANY_TABLE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_ANY_TABLE);
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aTableOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_TABLE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLDropViewPriv
 *
 * Argument :
 *    aStatement   - which have parse tree, iduMemory, session information.
 *    aViewOwnerID - view owner ID in parse tree
 *
 * Description :
 *    check privilege to execute DROP VIEW statement
 *        - owner of view
 *        - DROP ANY VIEW ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropViewPriv(
    qcStatement   * aStatement,
    UInt            aViewOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropViewPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aViewOwnerID))
    {
        // check system privilege : DROP ANY VIEW
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_ANY_VIEW_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_VIEW_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_ANY_VIEW);
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aViewOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_VIEW);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_VIEW_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLDropIndexPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *    aIndexOwnerID - index owner ID in parse Tree
 *
 * Description :
 *    check privilege to execute DROP INDEX statement
 *        - owner of table
 *        - DROP ANY INDEX ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropIndexPriv(
    qcStatement   * aStatement,
    qcmTableInfo  * aTableInfo,
    UInt            aIndexOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropIndexPriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID != aIndexOwnerID)
        {
            // check object privilege : INDEX
            IDE_TEST(qcmPriv::checkPrivilegeInfo(
                         aTableInfo->privilegeCount,
                         aTableInfo->privilegeInfo,
                         sGranteeID,
                         QCM_PRIV_ID_OBJECT_INDEX_NO,
                         &sQcmPriv)
                     != IDE_SUCCESS);

            if (sQcmPriv == NULL)
            {
                // check system privilege : DROP ANY INDEX
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_DROP_ANY_INDEX_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_DROP_ANY_INDEX_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_DROP_ANY_INDEX);
                }
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aIndexOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_INDEX_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLDropSequencePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableOwnerID - table owner ID in parse tree.
 *
 * Description :
 *    check privilege to execute DROP SEQUENCE statement
 *        - owner of sequence
 *        - DROP ANY SEQUENCE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropSequencePriv(
    qcStatement   * aStatement,
    UInt            aSequenceOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropSequencePriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aSequenceOwnerID))
    {
        // check system privilege : DROP ANY SEQUENCE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_ANY_SEQUENCE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_SEQUENCE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_ANY_SEQUENCE);
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aSequenceOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_SEQUENCE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_SEQUENCE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLDropPSMPriv
 *
 * Argument :
 *    aStatement  - which have parse tree, iduMemory, session information.
 *    aPSMOwnerID - Procedure owner ID in parse tree
 *
 * Description :
 *    check privilege to execute DROP PROCEDURE statement
 *        - owner of procedure
 *        - DROP ANY PROCEDURE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropPSMPriv(
    qcStatement * aStatement,
    UInt          aPSMOwnerID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropPSMPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID != aPSMOwnerID)
        {
            // check system privilege : DROP ANY PROCEDURE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_PROCEDURE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_DROP_ANY_PROCEDURE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_DROP_ANY_PROCEDURE);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aPSMOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_PROCEDURE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_PROCEDURE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdpPrivilege::checkDDLDropTriggerPriv( qcStatement * aStatement,
                                       UInt          aTriggerOwnerID )
{
/***********************************************************************
 *
 * Description :
 *    DROP TRIGGER를 위한 Validation을 수행함.
 *
 *    check privilege to execute DROP TRIGGER statement
 *        - owner of procedure
 *        - DROP ANY TRIGGER ( system privilege )
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLDropTriggerPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID != aTriggerOwnerID)
        {
            // check system privilege : DROP ANY TRIGGER
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_TRIGGER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_DROP_ANY_TRIGGER_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_DROP_ANY_TRIGGER );
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aTriggerOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_TRIGGER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_TRIGGER_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDDLDropUserPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute DROP USER statement
 *        - DROP USER ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropUserPriv(qcStatement * aStatement)
{
#define IDE_FN "qdpPrivilege::checkDDLDropUserPriv"

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : DROP USER
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_USER_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_USER_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_USER);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_USER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_USER_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLExecutePSMPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aProcInfo - Procedure information list in parse tree.
 *
 * Description :
 *    check privilege to execute EXECUTE statement
 *        - EXECUTE ( object privilege )
 *        - EXECUTE ANY PROCEDURE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLExecutePSMPriv(
    qcStatement                        * aStatement,
    UInt                                 aProcOwnerID,
    UInt                                 aPrivilegeCount,
    UInt                               * aGranteeID,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
#define IDE_FN "qdpPrivilege::checkDMLExecutePSMPriv"

    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    UInt              i;
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (aProcOwnerID != QC_SYSTEM_USER_ID))
    {
        if (sGranteeID != aProcOwnerID)
        {
            // check object privilege : EXECUTE
            // BUG-38451 check PUBLIC OBJECT_EXECUTE priv.
            for (i = 0; i < aPrivilegeCount; i++)
            {
                if ( ( aGranteeID[i] == sGranteeID ) ||
                     ( aGranteeID[i] == QC_PUBLIC_USER_ID ) )
                {
                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if (sExist == ID_FALSE)
            {
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

                // check system privilege : EXECUTE ANY PROCEDURE
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_EXECUTE_ANY_PROCEDURE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_EXECUTE_ANY_PROCEDURE_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_EXECUTE_ANY_PROCEDURE);
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_EXECUTE_ANY_PROCEDURE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_EXECUTE_ANY_PROCEDURE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkReferencesPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - referenced table information list in parse tree.
 *
 * Description :
 *    check privilege to execute REFERENCES(foreign key) statement
 *        - REFERENCES ( object privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkReferencesPriv( qcStatement  * aStatement,
                                          UInt           aTableOwnerID,
                                          qcmTableInfo * aTableInfo )
{
    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);

    // BUG-27950
    // 상위호출지에서 table 권한검사는 모두 완료된 상태로
    // 소유자와 참조객체의 소유자가 같을경우 객체권한 통과
    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) &&
         ( aTableOwnerID != aTableInfo->tableOwnerID ) )
    {
        // 소유자와 참조객체의 소유자가 다를경우
        // check object privilege : REFERENCES

        IDE_TEST( qcmPriv::checkPrivilegeInfo(
                      aTableInfo->privilegeCount,
                      aTableInfo->privilegeInfo,
                      aTableOwnerID,
                      QCM_PRIV_ID_OBJECT_REFERENCES_NO,
                      &sQcmPriv )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sQcmPriv == NULL, ERR_NO_GRANT_REFERENCES );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_REFERENCES);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_OBJECT_REFERENCES_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLSelectTablePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableRef  - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute SELECT statement
 *        - SELECT ( object privilege )
 *        - SELECT ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLSelectTablePriv(
    qcStatement                        * aStatement,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
#define IDE_FN "qdpPrivilege::checkDMLSelectTablePriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableOwnerID) &&
        (aTableOwnerID != QC_SYSTEM_USER_ID))
    {
        // check object privilege : SELECT
        IDE_TEST(qcmPriv::checkPrivilegeInfo(
                     aPrivilegeCount,
                     aPrivilegeInfo,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_SELECT_NO,
                     &sQcmPriv)
                 != IDE_SUCCESS);

        if (sQcmPriv == NULL)
        {
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

            // check system privilege : SELECT ANY TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_SELECT_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_SELECT_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);
                
                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_SELECT_ANY_TABLE);
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_SELECT_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_SELECT_ANY_TABLE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLSelectSequencePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute SEQUENCE.CURVAL,
 *                               SEQUENCE.NEXTVAL statement
 *        - SELECT ( object privilege )
 *        - SELECT ANY SEQUENCE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLSelectSequencePriv(
    qcStatement                        * aStatement,
    UInt                                 aSequenceOwnerID,
    UInt                                 aSequenceID,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
#define IDE_FN "qdpPrivilege::checkDMLSelectSequencePriv"

    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aSequenceOwnerID) &&
        (aSequenceOwnerID != QC_SYSTEM_USER_ID))
    {
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

        // check object privilege : SELECT
        IDE_TEST(qcmPriv::checkObjectPriv(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_SELECT_NO,
                     aSequenceID,
                     (SChar*)"S",
                     &sExist)
                 != IDE_SUCCESS);
        
        if (sExist == ID_FALSE)
        {
            // BUG-38451 check PUBLIC OBJECT_SELECT priv.
            IDE_TEST(qcmPriv::checkObjectPriv(
                         aStatement,
                         QC_PUBLIC_USER_ID,
                         QCM_PRIV_ID_OBJECT_SELECT_NO,
                         aSequenceID,
                         (SChar*)"S",
                         &sExist)
                     != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }

        if (sExist == ID_FALSE)
        {
            // check system privilege : SELECT ANY SEQUENCE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_SELECT_ANY_SEQUENCE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_SELECT_ANY_SEQUENCE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_SELECT_ANY_SEQUENCE);
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_SELECT_ANY_SEQUENCE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_SELECT_ANY_SEQUENCE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLInsertTablePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute INSERT statement
 *        - INSERT ( object privilege )
 *        - INSERT ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLInsertTablePriv(
    qcStatement                        * aStatement,
    void                               * aTableHandle,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
#define IDE_FN "qdpPrivilege::checkDMLInsertTablePriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableOwnerID))
    {
        // check object privilege : INSERT
        IDE_TEST(qcmPriv::checkPrivilegeInfo(
                     aPrivilegeCount,
                     aPrivilegeInfo,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_INSERT_NO,
                     &sQcmPriv)
                 != IDE_SUCCESS);

        if (sQcmPriv == NULL)
        {
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
            
            // check system privilege : INSERT ANY TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_INSERT_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_INSERT_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_INSERT_ANY_TABLE);
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    // in case of META table
    // except "SYS_DN_USERS_"
    if ( (aTableOwnerID == QC_SYSTEM_USER_ID) &&
         (aTableHandle != gQcmDNUsers) )
    {
        IDE_TEST_RAISE(sGranteeID != QC_SYSTEM_USER_ID,
                       ERR_NO_GRANT_DML_META_TABLE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_INSERT_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_INSERT_ANY_TABLE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DML_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QDP_NO_GRANT_DML_PRIV_OF_META_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLDeleteTablePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute DELETE statement
 *        - DELETE ( object privilege )
 *        - DELETE ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLDeleteTablePriv(
    qcStatement                        * aStatement,
    void                               * aTableHandle,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
#define IDE_FN "qdpPrivilege::checkDMLDeleteTablePriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableOwnerID))
    {
        // check object privilege : DELETE
        IDE_TEST(qcmPriv::checkPrivilegeInfo(
                     aPrivilegeCount,
                     aPrivilegeInfo,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_DELETE_NO,
                     &sQcmPriv)
                 != IDE_SUCCESS);

        if (sQcmPriv == NULL)
        {
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
            
            // check system privilege : DELETE ANY TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DELETE_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_DELETE_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);
                
                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_DELETE_ANY_TABLE);
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    // in case of META table
    // except "SYS_DN_USERS_"
    if ( (aTableOwnerID == QC_SYSTEM_USER_ID) &&
         (aTableHandle != gQcmDNUsers) )
    {
        IDE_TEST_RAISE(sGranteeID != QC_SYSTEM_USER_ID,
                       ERR_NO_GRANT_DML_META_TABLE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DELETE_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DELETE_ANY_TABLE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DML_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QDP_NO_GRANT_DML_PRIV_OF_META_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLUpdateTablePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute UPDATE statement
 *        - UPDATE ( object privilege )
 *        - UPDATE ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLUpdateTablePriv(
    qcStatement                        * aStatement,
    void                               * aTableHandle,
    UInt                                 aTableOwnerID,
    UInt                                 aPrivilegeCount,
    struct qcmPrivilege                * aPrivilegeInfo,
    idBool                               aGetSmiStmt4Prepare,
    qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
    void                               * aGetSmiStmt4PrepareContext )
{
#define IDE_FN "qdpPrivilege::checkDMLUpdateTablePriv"

    qcmPrivilege    * sQcmPriv   = NULL;
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableOwnerID))
    {
        // check object privilege : UPDATE
        IDE_TEST(qcmPriv::checkPrivilegeInfo(
                     aPrivilegeCount,
                     aPrivilegeInfo,
                     sGranteeID,
                     QCM_PRIV_ID_OBJECT_UPDATE_NO,
                     &sQcmPriv)
                 != IDE_SUCCESS);

        if (sQcmPriv == NULL)
        {
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

            // check system privilege : UPDATE ANY TABLE
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_UPDATE_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_UPDATE_ANY_TABLE_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_UPDATE_ANY_TABLE);
            }
        }
    }

    // in case of META table
    // except "SYS_DN_USERS_"
    if ( (aTableOwnerID == QC_SYSTEM_USER_ID) &&
         (aTableHandle != gQcmDNUsers) )
    {
        IDE_TEST_RAISE(sGranteeID != QC_SYSTEM_USER_ID,
                       ERR_NO_GRANT_DML_META_TABLE);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_UPDATE_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_UPDATE_ANY_TABLE_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DML_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QDP_NO_GRANT_DML_PRIV_OF_META_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDMLLockTablePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aTableInfo - table information list in parse tree.
 *
 * Description :
 *    check privilege to execute LOCK statement
 *        - LOCK ANY TABLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDMLLockTablePriv( qcStatement   * aStatement,
                                            UInt            aTableOwnerID )
{
#define IDE_FN "qdpPrivilege::checkDMLLockTablePriv"

    UInt    sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool  sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aTableOwnerID))
    {
        // check system privilege : LOCK ANY TABLE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_LOCK_ANY_TABLE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_LOCK_ANY_TABLE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_LOCK_ANY_TABLE);
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_LOCK_ANY_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_LOCK_ANY_TABLE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
/*-----------------------------------------------------------------------------
 * Name :
 :   qdpPrivilege::checkSystemPrivAllAndPublic
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID
 *    aPrivID    - privileges ID
 *    aExist     - existence of result set ( OUT )
 *
 * Description :
 *    check ALL PRIVILEGES and PUBLIC.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkSystemPrivAllAndPublic(
    qcStatement    * aStatement,
    UInt             aGranteeID,
    UInt             aPrivID,
    idBool         * aExist)
{
#define IDE_FN "qdpPrivilege::checkSystemPrivAllAndPublic"

    idBool            sExist     = ID_FALSE;

    // check ALL PRIVILEGES
    IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                 aStatement,
                 aGranteeID,
                 QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO,
                 &sExist)
             != IDE_SUCCESS);

    if (sExist == ID_FALSE)
    {
        // check PUBLIC
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     QC_PUBLIC_USER_ID,
                     aPrivID,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check ALL PRIVILEGES & PUBLIC
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         QC_PUBLIC_USER_ID,
                         QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO,
                         &sExist)
                     != IDE_SUCCESS);
        }
    }

    *aExist = sExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLCreateDatabasePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
#define IDE_FN "qdpPrivilege::checkDDLCreateDatabasePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : ALTER DATABASE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_SYSDBA_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_SYSDBA_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_CREATE_DATABASE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_DATABASE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_SYSDBA_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLAlterDatabasePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
#define IDE_FN "qdpPrivilege::checkDDLAlterDatabasePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : ALTER DATABASE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_ALTER_DATABASE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_DATABASE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_ALTER_DATABASE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_DATABASE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_DATABASE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLDropDatabasePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropDatabasePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : DROP DATABASE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_DATABASE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_DATABASE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_DROP_DATABASE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_DATABASE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_DATABASE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLCreateTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
/***********************************************************************
 *
 * Description :
 *    CREATE TABLESPACE 권한 검사
 *
 * Implementation :
 *    (1) SYS 사용자 이거나 tablespace 생성권한을 가진 사용자인지 검사
 *    (2) PUBLIC 또는 All PRIVILEGES인지 검사
 *
 ***********************************************************************/
#define IDE_FN "qdpPrivilege::checkDDLCreateTableSpacePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        //--------------------------------
        // SYS, SYSTEM 사용자가 아닌 경우
        //--------------------------------

        // create tablespace 권한을 가졌는지 검사
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_CREATE_TABLESPACE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            //--------------------------------
            // PUBLIC 또는 All PRIVILEGES인지 검사
            // - PUBLIC : 모든 사용자에게 시스템 접근 권한을 부여
            // - ALL PRIVILEGES : 시스템 접근 권한에 관한 모든 권한 부여
            //--------------------------------

            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_TABLESPACE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_CREATE_TABLESPACE);
        }
    }
    else
    {
        //--------------------------------
        // SYS, SYSTEM 사용자인 경우
        //--------------------------------
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_TABLESPACE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_TABLESPACE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLAlterTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
/***********************************************************************
 *
 * Description :
 *    ALTER TABLESPACE 권한 검사
 *
 * Implementation :
 *    (1) SYS 사용자 이거나 tablespace 변경 권한을 가진 사용자인지 검사
 *    (2) PUBLIC 또는 All PRIVILEGES인지 검사
 *
 ***********************************************************************/
#define IDE_FN "qdpPrivilege::checkDDLAlterTableSpacePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        //--------------------------------
        // SYS, SYSTEM 사용자가 아닌 경우
        //--------------------------------

        // alter tablespace 권한을 가졌는지 검사
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_ALTER_TABLESPACE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            //--------------------------------
            // PUBLIC 또는 All PRIVILEGES인지 검사
            // - PUBLIC : 모든 사용자에게 시스템 접근 권한을 부여
            // - ALL PRIVILEGES : 시스템 접근 권한에 관한 모든 권한 부여
            //--------------------------------

            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_TABLESPACE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_ALTER_TABLESPACE);
        }
    }
    else
    {
        //--------------------------------
        // SYS, SYSTEM 사용자인 경우
        //--------------------------------
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_TABLESPACE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_TABLESPACE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLManageTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
#define IDE_FN "qdpPrivilege::checkDDLManageTableSpacePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : MANAGE TABLESPACE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_MANAGE_TABLESPACE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_MANAGE_TABLESPACE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_ALTER_TABLESPACE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_TABLESPACE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_TABLESPACE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
IDE_RC qdpPrivilege::checkDDLDropTableSpacePriv(
    qcStatement    * aStatement,
    UInt             aUserID)
{
#define IDE_FN "qdpPrivilege::checkDDLDropTableSpacePriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGranteeID = aUserID;
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : DROP TABLESPACE
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_TABLESPACE_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_TABLESPACE_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_DROP_TABLESPACE);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_TABLESPACE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_TABLESPACE_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkAccessAnyTBSPriv(
    qcStatement    * aStatement,
    UInt             aUserID,
    idBool         * aIsAccess)
{
#define IDE_FN "qdpPrivilege::checkAccessAnyTBSPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool      sExist     = ID_FALSE;

    *aIsAccess = ID_TRUE;
    if ((aUserID != QC_SYSTEM_USER_ID) &&
        (aUserID != QC_SYS_USER_ID))
    {
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     aUserID,
                     QCM_PRIV_ID_SYSTEM_SYSDBA_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         aUserID,
                         QCM_PRIV_ID_SYSTEM_SYSDBA_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                *aIsAccess = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qdpPrivilege::checkDDLCreateSynonymPriv(
    qcStatement    * aStatement,
    UInt             aSynonymOwnerID)
{
/***********************************************************************
 *
 * Description : Private Synonym 생성권한을 검사하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLCreateSynonymPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGrantorID = QCG_GET_SESSION_USER_ID(aStatement);
    UInt        sPrivID;
    idBool      sExist     = ID_FALSE;

    // SYSTEM_USER, SYS_USER는 모든 권한을 가짐
    if( (QC_SYSTEM_USER_ID == sGrantorID) ||
        (QC_SYS_USER_ID == sGrantorID) )
    {
        return IDE_SUCCESS;
    }

    // 1. 자신의 Synonym을 생성하는 경우
    if(sGrantorID == aSynonymOwnerID)
    {
        // 1.1 CREATE SYNONYM 권한 체크
        sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_SYNONYM_NO;

        IDE_TEST(
            qcmPriv::checkSystemPrivWithoutGrantor(
                aStatement,
                sGrantorID,
                sPrivID,
                &sExist)
            != IDE_SUCCESS);

        if(sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGrantorID,
                         sPrivID,
                         &sExist)
                     != IDE_SUCCESS);
        }

        if(sExist == ID_FALSE)
        {
            // 1.2 CREATE ANY SYNONYM 권한 체크
            sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_ANY_SYNONYM_NO;

            IDE_TEST(
                qcmPriv::checkSystemPrivWithoutGrantor(
                    aStatement,
                    sGrantorID,
                    sPrivID,
                    &sExist)
                != IDE_SUCCESS);

            if(sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGrantorID,
                             sPrivID,
                             &sExist)
                         != IDE_SUCCESS);

                // CREATE SYNONYM 권한이 없음
                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_CREATE_SYNONYM);
            }
        }
    }
    // 2. 타 사용자의 Synonym을 생성하는 경우
    else
    {
        // 2.1 CREATE ANY SYNONYM 권한 체크
        sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_ANY_SYNONYM_NO;

        IDE_TEST(
            qcmPriv::checkSystemPrivWithoutGrantor(
                aStatement,
                sGrantorID,
                sPrivID,
                &sExist)
            != IDE_SUCCESS);

        if(sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGrantorID,
                         sPrivID,
                         &sExist)
                     != IDE_SUCCESS);

            // To fix BUG-13761
            // CREATE_ANY_SYNONYM 권한이 없다고 나와야 함.
            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_CREATE_ANY_SYNONYM);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_SYNONYM);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_SYNONYM_STR));
    }
    // To fix BUG-13761
    // CREATE_ANY_SYNONYM 권한이 없다고 나와야 함.
    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_SYNONYM);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_SYNONYM_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qdpPrivilege::checkDDLCreatePublicSynonymPriv(
    qcStatement    * aStatement)
{
/***********************************************************************
 *
 * Description : Public Synonym 생성권한을 검사하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qdpPrivilege::checkDDLCreatePublicSynonymPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGrantorID = QCG_GET_SESSION_USER_ID(aStatement);
    UInt        sPrivID;
    idBool      sExist     = ID_FALSE;

    // SYSTEM_USER, SYS_USER는 모든 권한을 가짐
    if( (QC_SYSTEM_USER_ID == sGrantorID) ||
        (QC_SYS_USER_ID == sGrantorID) )
    {
        return IDE_SUCCESS;
    }


    // CREATE PUBLIC SYNONYM 권한 체크
    sPrivID = QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_SYNONYM_NO;

    IDE_TEST(
        qcmPriv::checkSystemPrivWithoutGrantor(
            aStatement,
            sGrantorID,
            sPrivID,
            &sExist)
        != IDE_SUCCESS);

    if(ID_FALSE == sExist)
    {
        // check system privilege
        // in case PUBLIC or All PRIVILEGES
        IDE_TEST(checkSystemPrivAllAndPublic(
                     aStatement,
                     sGrantorID,
                     sPrivID,
                     &sExist)
                 != IDE_SUCCESS);

        // CREATE PUBLIC SYNONYM 권한이 없음
        IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT_CREATE_PUBLIC_SYNONYM);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_PUBLIC_SYNONYM);
    {
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                            QCM_PRIV_NAME_SYSTEM_CREATE_PUBLIC_SYNONYM_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qdpPrivilege::checkDDLDropSynonymPriv(
    qcStatement    * aStatement,
    UInt             aSynonymOwnerID)
{
/***********************************************************************
 *
 * Description : Private Synonym 소멸(Drop)권한을 검사하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLDropSynonymPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGrantorID = QCG_GET_SESSION_USER_ID(aStatement);
    UInt        sPrivID;
    idBool      sExist     = ID_FALSE;

    // SYSTEM_USER, SYS_USER는 모든 권한을 가짐
    if( (QC_SYSTEM_USER_ID == sGrantorID) ||
        (QC_SYS_USER_ID == sGrantorID) )
    {
        return IDE_SUCCESS;
    }

    // 자신의 Synonym을 소멸(Drop)하는 경우
    if(sGrantorID == aSynonymOwnerID)
    {
        // Synonym 소유자는 자신의 Private Synonym을 삭제할 수 있음
        return IDE_SUCCESS;
    }
    // 타 사용자의 Synonym을 소멸(Drop)하는 경우
    else
    {
        // DROP ANY SYNONYM 권한 체크
        sPrivID = QCM_PRIV_ID_SYSTEM_DROP_ANY_SYNONYM_NO;

        IDE_TEST(
            qcmPriv::checkSystemPrivWithoutGrantor(
                aStatement,
                sGrantorID,
                sPrivID,
                &sExist)
            != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGrantorID,
                         sPrivID,
                         &sExist)
                     != IDE_SUCCESS);

            // DROP ANY SYNONYM 권한이 없음
            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_SYNONYM);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_SYNONYM);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_SYNONYM_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLDropPublicSynonymPriv(
    qcStatement    * aStatement)
{
/***********************************************************************
 *
 * Description : Public Synonym 소멸(Drop)권한을 검사하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qdpPrivilege::checkDDLDropPublicSynonymPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sGrantorID = QCG_GET_SESSION_USER_ID(aStatement);
    UInt        sPrivID;
    idBool      sExist     = ID_FALSE;

    // SYSTEM_USER, SYS_USER는 모든 권한을 가짐
    if( (QC_SYSTEM_USER_ID == sGrantorID) ||
        (QC_SYS_USER_ID == sGrantorID) )
    {
        return IDE_SUCCESS;
    }

    // DROP PUBLIC SYNONYM 권한 체크
    sPrivID = QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_SYNONYM_NO;

    IDE_TEST(
        qcmPriv::checkSystemPrivWithoutGrantor(
            aStatement,
            sGrantorID,
            sPrivID,
            &sExist)
        != IDE_SUCCESS);

    if (sExist == ID_FALSE)
    {
        // check system privilege
        // in case PUBLIC or All PRIVILEGES
        IDE_TEST(checkSystemPrivAllAndPublic(
                     aStatement,
                     sGrantorID,
                     sPrivID,
                     &sExist)
                 != IDE_SUCCESS);

        // DROP PUBLIC SYNONYM 권한이 없음
        IDE_TEST_RAISE(sExist == ID_FALSE,
                       ERR_NO_GRANT_DROP_PUBLIC_SYNONYM);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_PUBLIC_SYNONYM);
    {
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                            QCM_PRIV_NAME_SYSTEM_DROP_PUBLIC_SYNONYM_STR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


// PROJ-1371 Directories

IDE_RC qdpPrivilege::checkDDLCreateDirectoryPriv(
    qcStatement   * aStatement )
{
/***********************************************************************
 *
 * Description : create directory 권한 검사
 *
 * Implementation :
 *       1. system유저가 아니라면 create any directory권한 검사
 *       2. 만약 권한이 없다면 public이나 all privilege가 있는지 검사
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLCreateDirectoryPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        // check system privilege : CREATE ANY DIRECTORY
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_CREATE_ANY_DIRECTORY_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_DIRECTORY_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_CREATE_ANY_DIRECTORY);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_DIRECTORY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_DIRECTORY_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDDLDropDirectoryPriv(
    qcStatement   * aStatement,
    UInt            aDirectoryOwnerID )
{
/***********************************************************************
 *
 * Description : drop directory 권한 검사
 *
 * Implementation :
 *       1. system유저또는 owner가 아니라면 drop any directory권한 검사
 *       2. 만약 권한이 없다면 public이나 all privilege가 있는지 검사
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDDLDropDirectoryPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aDirectoryOwnerID))
    {
        // check system privilege : DROP ANY DIRECTORY
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_ANY_DIRECTORY_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_DIRECTORY_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_ANY_DIRECTORY);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_DIRECTORY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_DIRECTORY_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDMLReadDirectoryPriv(
    qcStatement    * aStatement,
    qdpObjID         aObjID,
    UInt             aDirectoryOwnerID )
{
/***********************************************************************
 *
 * Description : read on directory 권한 검사
 *
 * Implementation :
 *       1. system유저또는 owner가 아니라면 read on directory권한 검사
 *       2. public->grantee순서로 검사
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDMLReadDirectoryPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;
    SChar             sObjType[2];

    sObjType[0] = 'D';
    sObjType[1] = '\0';

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aDirectoryOwnerID))
    {
        IDE_TEST(qcmPriv::checkObjectPriv(
                     aStatement,
                     QC_PUBLIC_USER_ID,
                     QCM_PRIV_ID_DIRECTORY_READ_NO,
                     aObjID,
                     sObjType,
                     &sExist)
                 != IDE_SUCCESS);

        if( sExist == ID_FALSE )
        {
            IDE_TEST(qcmPriv::checkObjectPriv(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_DIRECTORY_READ_NO,
                         aObjID,
                         sObjType,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT);
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_DIRECTORY_READ_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdpPrivilege::checkDMLWriteDirectoryPriv(
    qcStatement    * aStatement,
    qdpObjID         aObjID,
    UInt             aDirectoryOwnerID )
{
/***********************************************************************
 *
 * Description : write on directory 권한 검사
 *
 * Implementation :
 *       1. system유저또는 owner가 아니라면 write on directory권한 검사
 *       2. public->grantee순서로 검사
 *
 ***********************************************************************/

#define IDE_FN "qdpPrivilege::checkDMLWriteDirectoryPriv"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;
    SChar             sObjType[2];

    sObjType[0] = 'D';
    sObjType[1] = '\0';

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aDirectoryOwnerID))
    {
        IDE_TEST(qcmPriv::checkObjectPriv(
                     aStatement,
                     QC_PUBLIC_USER_ID,
                     QCM_PRIV_ID_DIRECTORY_WRITE_NO,
                     aObjID,
                     sObjType,
                     &sExist)
                 != IDE_SUCCESS);

        if( sExist == ID_FALSE )
        {
            IDE_TEST(qcmPriv::checkObjectPriv(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_DIRECTORY_WRITE_NO,
                         aObjID,
                         sObjType,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT);
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_DIRECTORY_WRITE_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qdpPrivilege::checkDBMSStatPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute GATHER_DATABASE_STATS
 *                               GATHER_TABLE_STATS
 *                               GATHER_INDEX_STATS
 *        - Only SYS user
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDBMSStatPriv(qcStatement * aStatement)
{

    // check Grant : A Current user is only SYS user
    IDE_TEST_RAISE(QCG_GET_SESSION_USER_ID(aStatement) != QC_SYS_USER_ID,
                   ERR_NO_GRANT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QRC_NO_GRANT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *    Materialized View 생성 권한을 검사한다.
 *
 * Implementation :
 *    1. 사용자가 SYSTEM, SYS이면, 권한을 가지고 있는 것으로 간주한다.
 *    2. 사용자가 소유자이면, CREATE MATERIALIZED VIEW, CREATE ANY MATERIALIZED VIEW,
 *       PUBLIC or All PRIVILEGES 권한을 확인한다.
 *    3. 사용자가 소유자가 아니면, CREATE ANY MATERIALIZED VIEW,
 *       PUBLIC or All PRIVILEGES 권한을 확인한다.
 *
 ***********************************************************************/
IDE_RC qdpPrivilege::checkDDLCreateMViewPriv(
        qcStatement * aStatement,
        UInt          aOwnerID )
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool sExist     = ID_FALSE;

    if ( (sGranteeID != QC_SYSTEM_USER_ID) &&
         (sGranteeID != QC_SYS_USER_ID) )
    {
        if ( sGranteeID == aOwnerID )
        {
            /* check system privilege : CREATE MATERIALIZED VIEW */
            IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                            aStatement,
                            sGranteeID,
                            QCM_PRIV_ID_SYSTEM_CREATE_MATERIALIZED_VIEW_NO,
                            &sExist )
                      != IDE_SUCCESS );

            if ( sExist == ID_FALSE )
            {
                /* check system privilege : CREATE ANY MATERIALIZED VIEW */
                IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                                aStatement,
                                sGranteeID,
                                QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_NO,
                                &sExist )
                          != IDE_SUCCESS );

                if ( sExist == ID_FALSE )
                {
                    /* check system privilege : in case PUBLIC or All PRIVILEGES */
                    IDE_TEST( checkSystemPrivAllAndPublic(
                                    aStatement,
                                    sGranteeID,
                                    QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_NO,
                                    &sExist )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sExist == ID_FALSE,
                                    ERR_NO_GRANT_CREATE_ANY_MATERIALIZED_VIEW );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* check system privilege : CREATE ANY MATERIALIZED VIEW */
            IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                            aStatement,
                            sGranteeID,
                            QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_NO,
                            &sExist )
                      != IDE_SUCCESS );

            if ( sExist == ID_FALSE )
            {
                /* check system privilege : in case PUBLIC or All PRIVILEGES */
                IDE_TEST( checkSystemPrivAllAndPublic(
                                aStatement,
                                sGranteeID,
                                QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_NO,
                                &sExist )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sExist == ID_FALSE,
                                ERR_NO_GRANT_CREATE_ANY_MATERIALIZED_VIEW );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* To Fix PR-11549
     * SYSTEM_ 계정 이외에는 SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
     */
    IDE_TEST_RAISE( ((aOwnerID == QC_SYSTEM_USER_ID) &&
                     (sGranteeID != QC_SYSTEM_USER_ID)),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_CREATE_ANY_MATERIALIZED_VIEW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_STR ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT_DDL_META_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NO_DDL_FOR_META ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Materialized View 변경 권한을 검사한다.
 *
 * Implementation :
 *    1. 사용자가 SYSTEM, SYS, 소유자이면, 권한을 가지고 있는 것으로 간주한다.
 *    2. ALTER 객체 권한을 확인한다.
 *    3. ALTER ANY MATERIALIZED VIEW, PUBLIC or All PRIVILEGES 권한을 확인한다.
 *
 ***********************************************************************/
IDE_RC qdpPrivilege::checkDDLAlterMViewPriv(
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo )
{
    qcmPrivilege * sQcmPriv   = NULL;
    UInt           sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool         sExist     = ID_FALSE;

    if ( (sGranteeID != QC_SYSTEM_USER_ID) &&
         (sGranteeID != QC_SYS_USER_ID) &&
         (sGranteeID != aTableInfo->tableOwnerID) )
    {
        /* check object privilege : ALTER */
        IDE_TEST( qcmPriv::checkPrivilegeInfo(
                        aTableInfo->privilegeCount,
                        aTableInfo->privilegeInfo,
                        sGranteeID,
                        QCM_PRIV_ID_OBJECT_ALTER_NO,
                        &sQcmPriv )
                  != IDE_SUCCESS );

        if ( sQcmPriv == NULL )
        {
            /* check system privilege : ALTER ANY MATERIALIZED VIEW */
            IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                            aStatement,
                            sGranteeID,
                            QCM_PRIV_ID_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_NO,
                            &sExist )
                      != IDE_SUCCESS );

            if ( sExist == ID_FALSE )
            {
                /* check system privilege : in case PUBLIC or All PRIVILEGES */
                IDE_TEST( checkSystemPrivAllAndPublic(
                                aStatement,
                                sGranteeID,
                                QCM_PRIV_ID_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_NO,
                                &sExist )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sExist == ID_FALSE,
                                ERR_NO_GRANT_ALTER_ANY_MATERIALIZED_VIEW );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* To Fix PR-11549
     * SYSTEM_ 계정 이외에는 SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
     */
    IDE_TEST_RAISE( ((aTableInfo->tableOwnerID == QC_SYSTEM_USER_ID) &&
                     (sGranteeID != QC_SYSTEM_USER_ID)),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_ALTER_ANY_MATERIALIZED_VIEW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_STR ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT_DDL_META_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NO_DDL_FOR_META ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Materialized View 삭제 권한을 검사한다.
 *
 * Implementation :
 *    1. 사용자가 SYSTEM, SYS, 소유자이면, 권한을 가지고 있는 것으로 간주한다.
 *    2. DROP ANY MATERIALIZED VIEW, PUBLIC or All PRIVILEGES 권한을 확인한다.
 *
 ***********************************************************************/
IDE_RC qdpPrivilege::checkDDLDropMViewPriv(
        qcStatement * aStatement,
        UInt          aOwnerID )
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool sExist     = ID_FALSE;

    if ( (sGranteeID != QC_SYSTEM_USER_ID) &&
         (sGranteeID != QC_SYS_USER_ID) &&
         (sGranteeID != aOwnerID) )
    {
        /* check system privilege : DROP ANY MATERIALIZED VIEW */
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                        aStatement,
                        sGranteeID,
                        QCM_PRIV_ID_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_NO,
                        &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            /* check system privilege : in case PUBLIC or All PRIVILEGES */
            IDE_TEST( checkSystemPrivAllAndPublic(
                            aStatement,
                            sGranteeID,
                            QCM_PRIV_ID_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_NO,
                            &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE,
                            ERR_NO_GRANT_DROP_ANY_MATERIALIZED_VIEW );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* To Fix PR-11549
     * SYSTEM_ 계정 이외에는 SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
     */
    IDE_TEST_RAISE( ((aOwnerID == QC_SYSTEM_USER_ID) &&
                     (sGranteeID != QC_SYSTEM_USER_ID)),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DROP_ANY_MATERIALIZED_VIEW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_STR ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT_DDL_META_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_NO_DDL_FOR_META ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1685
IDE_RC qdpPrivilege::checkDDLCreateLibraryPriv(
    qcStatement * aStatement,
    UInt          aLibraryOwnerID)
{
    UInt        sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool      sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID))
    {
        if (sGranteeID == aLibraryOwnerID)
        {
            // check system privilege : CREATE LIBRARY
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_LIBRARY_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege : CREATE ANY LIBRARY
                IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_NO,
                             &sExist)
                         != IDE_SUCCESS);

                if (sExist == ID_FALSE)
                {
                    // check system privilege
                    // in case PUBLIC or All PRIVILEGES
                    IDE_TEST(checkSystemPrivAllAndPublic(
                                 aStatement,
                                 sGranteeID,
                                 QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_NO,
                                 &sExist)
                             != IDE_SUCCESS);

                    IDE_TEST_RAISE(sExist == ID_FALSE,
                                   ERR_NO_GRANT_CREATE_ANY_LIBRARY);
                }
            }
        }
        else
        {
            // check system privilege : CREATE ANY LIBRARY
            IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_NO,
                         &sExist)
                     != IDE_SUCCESS);

            if (sExist == ID_FALSE)
            {
                // check system privilege
                // in case PUBLIC or All PRIVILEGES
                IDE_TEST(checkSystemPrivAllAndPublic(
                             aStatement,
                             sGranteeID,
                             QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_NO,
                             &sExist)
                         != IDE_SUCCESS);

                IDE_TEST_RAISE(sExist == ID_FALSE,
                               ERR_NO_GRANT_CREATE_ANY_LIBRARY);
            }
        }
    }

    // To Fix PR-11549
    // SYSTEM_ 계정 이외에는
    // SYSTEM_ 유저에 어떠한 DDL도 수행할 수 없다.
    IDE_TEST_RAISE( ( ( aLibraryOwnerID == QC_SYSTEM_USER_ID ) &&
                      ( sGranteeID != QC_SYSTEM_USER_ID ) ),
                    ERR_NO_GRANT_DDL_META_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_CREATE_ANY_LIBRARY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_CREATE_ANY_LIBRARY_STR));
    }
    IDE_EXCEPTION(ERR_NO_GRANT_DDL_META_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_NO_DDL_FOR_META));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpPrivilege::checkDDLDropLibraryPriv(
    qcStatement * aStatement,
    UInt          aLibraryOwnerID)
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aLibraryOwnerID))
    {
        // check system privilege : DROP ANY LIBRARY
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_DROP_ANY_LIBRARY_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_DROP_ANY_LIBRARY_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_DROP_ANY_LIBRARY);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_DROP_ANY_LIBRARY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_DROP_ANY_LIBRARY_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpPrivilege::checkDDLAlterLibraryPriv(
    qcStatement * aStatement,
    UInt          aLibraryOwnerID)
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool sExist     = ID_FALSE;

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aLibraryOwnerID))
    {
        // check system privilege : ALTER ANY LIBRARY
        IDE_TEST(qcmPriv::checkSystemPrivWithoutGrantor(
                     aStatement,
                     sGranteeID,
                     QCM_PRIV_ID_SYSTEM_ALTER_ANY_LIBRARY_NO,
                     &sExist)
                 != IDE_SUCCESS);

        if (sExist == ID_FALSE)
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST(checkSystemPrivAllAndPublic(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_SYSTEM_ALTER_ANY_LIBRARY_NO,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE,
                           ERR_NO_GRANT_ALTER_ANY_LIBRARY);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT_ALTER_ANY_LIBRARY);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_NAME_SYSTEM_ALTER_ANY_LIBRARY_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpPrivilege::checkDMLExecuteLibraryPriv(
    qcStatement    * aStatement,
    qdpObjID         aObjID,
    UInt             aLibraryOwnerID )
{
    UInt              sGranteeID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool            sExist     = ID_FALSE;
    SChar             sObjType[2];

    sObjType[0] = 'Y';
    sObjType[1] = '\0';

    if ((sGranteeID != QC_SYSTEM_USER_ID) &&
        (sGranteeID != QC_SYS_USER_ID) &&
        (sGranteeID != aLibraryOwnerID))
    {
        IDE_TEST(qcmPriv::checkObjectPriv(
                     aStatement,
                     QC_PUBLIC_USER_ID,
                     QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                     aObjID,
                     sObjType,
                     &sExist)
                 != IDE_SUCCESS);

        if( sExist == ID_FALSE )
        {
            IDE_TEST(qcmPriv::checkObjectPriv(
                         aStatement,
                         sGranteeID,
                         QCM_PRIV_ID_OBJECT_EXECUTE_NO,
                         aObjID,
                         sObjType,
                         &sExist)
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sExist == ID_FALSE, ERR_NO_GRANT);
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                QCM_PRIV_ID_OBJECT_EXECUTE_STR));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qdpPrivilege::checkSystemPrivilege( qcStatement * aStatement,
                                           UInt          aGrantorID,
                                           UInt          aPrivilegeId,
                                           idBool      * aExist )
{
    IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor( aStatement,
                                                      aGrantorID,
                                                      aPrivilegeId,
                                                      aExist )
              != IDE_SUCCESS );
    if ( *aExist == ID_FALSE )
    {
        IDE_TEST( checkSystemPrivAllAndPublic( aStatement,
                                               aGrantorID,
                                               aPrivilegeId,
                                               aExist )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 *
 * This privilege is checked when "CREATE DATABASE LINK ... " is executed.
 */
IDE_RC qdpPrivilege::checkDDLCreateDatabaseLinkPriv(
    qcStatement * aStatement,
    idBool        aPublic )
{
    UInt sGrantorID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool sExist = ID_FALSE;

    IDE_TEST_CONT( sGrantorID == QC_SYSTEM_USER_ID, NORMAL_EXIT );
    IDE_TEST_CONT( sGrantorID == QC_SYS_USER_ID, NORMAL_EXIT );

    if ( aPublic == ID_TRUE )
    {
        /* CREATE PUBLIC DATABASE LINK */
        IDE_TEST( checkSystemPrivilege(
                      aStatement,
                      sGrantorID,
                      QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_NO,
                      &sExist )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_FALSE,
                        ERR_NO_GRANT_CREATE_PUBLIC_DATABASE_LINK );
    }
    else
    {
        /* CREATE DATABASE LINK */
        IDE_TEST( checkSystemPrivilege(
                      aStatement,
                      sGrantorID,
                      QCM_PRIV_ID_SYSTEM_CREATE_DATABASE_LINK_NO,
                      &sExist )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_FALSE,
                        ERR_NO_GRANT_CREATE_DATABASE_LINK );
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_CREATE_PUBLIC_DATABASE_LINK )
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                     QCM_PRIV_NAME_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_STR ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT_CREATE_DATABASE_LINK )
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                     QCM_PRIV_NAME_SYSTEM_CREATE_DATABASE_LINK_STR ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qdpPrivilege::checkDDLDropDatabaseLinkPriv( qcStatement * aStatement,
                                                   idBool        aPublic )
{
    UInt sGrantorID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool sExist = ID_FALSE;

    IDE_TEST_CONT( sGrantorID == QC_SYSTEM_USER_ID, NORMAL_EXIT );
    IDE_TEST_CONT( sGrantorID == QC_SYS_USER_ID, NORMAL_EXIT );

    if ( aPublic == ID_TRUE )
    {
        /* DROP PUBLIC DATABASE LINK */
        IDE_TEST( checkSystemPrivilege(
                      aStatement,
                      sGrantorID,
                      QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_DATABASE_LINK_NO,
                      &sExist )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_FALSE,
                        ERR_NO_GRANT_DROP_PUBLIC_DATABASE_LINK );
    }
    else
    {
        /* do nothing */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DROP_PUBLIC_DATABASE_LINK )
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                     QCM_PRIV_NAME_SYSTEM_DROP_PUBLIC_DATABASE_LINK_STR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name : PROJ-1812 ROLE
 *    qdpPrivilege::checkDDLCreateRolePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute CREATE ROLE statement
 *        - CREATE ROLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLCreateRolePriv( qcStatement * aStatement )
{
    UInt        sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool      sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                      aStatement,
                      sGranteeID,
                      QCM_PRIV_ID_SYSTEM_CREATE_ROLE_NO,
                      &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic(
                          aStatement,
                          sGranteeID,
                          QCM_PRIV_ID_SYSTEM_CREATE_ROLE_NO,
                          &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE,
                            ERR_NO_GRANT_CREATE_ROLE );
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
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NO_GRANT_CREATE_ROLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_CREATE_ROLE_STR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name : PROJ-1812 ROLE
 *    qdpPrivilege::checkDDLDropRolerPriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *
 * Description :
 *    check privilege to execute DROP ROLE statement
 *        - DROP ANY ROLE ( system privilege )
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLDropAnyRolePriv(qcStatement * aStatement )
{
    UInt        sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool      sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                      aStatement,
                      sGranteeID,
                      QCM_PRIV_ID_SYSTEM_DROP_ANY_ROLE_NO,
                      &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic(
                          aStatement,
                          sGranteeID,
                          QCM_PRIV_ID_SYSTEM_DROP_ANY_ROLE_NO,
                          &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE,
                            ERR_NO_GRANT_DROP_ANY_ROLE );
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
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NO_GRANT_DROP_ANY_ROLE );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                 QCM_PRIV_NAME_SYSTEM_DROP_ANY_ROLE_STR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name : PROJ-1812 ROLE
 :   qdpPrivilege::checkSystemGrantAnyRolePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantor ID
 *    aPrivID    - privileges ID
 *    aExist     - existence of result set ( OUT )
 *
 * Description :
 *    GRANT ANY ROLE SYSTEM PRIVILEGE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLGrantAnyRolePriv( qcStatement * aStatement )
{
    UInt        sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool      sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                      aStatement,
                      sGranteeID,
                      QCM_PRIV_ID_SYSTEM_GRANT_ANY_ROLE_NO,
                      &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic(
                          aStatement,
                          sGranteeID,
                          QCM_PRIV_ID_SYSTEM_GRANT_ANY_ROLE_NO,
                          &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE,
                            ERR_NO_GRANT_GRANT_ANY_ROLE );
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

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NO_GRANT_GRANT_ANY_ROLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_GRANT_ANY_ROLE_STR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name : PROJ-1812 ROLE
 :   qdpPrivilege::checkSystemGrantAnyRolePriv
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantor ID
 *    aPrivID    - privileges ID
 *    aExist     - existence of result set ( OUT )
 *
 * Description :
 *    GRANT ANY PRIVILEGES SYSTEM PRIVILEGE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qdpPrivilege::checkDDLGrantAnyPrivilegesPriv( qcStatement * aStatement )
{
    UInt        sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool      sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor(
                      aStatement,
                      sGranteeID,
                      QCM_PRIV_ID_SYSTEM_GRANT_ANY_PRIVILEGES_NO,
                      &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic(
                          aStatement,
                          sGranteeID,
                          QCM_PRIV_ID_SYSTEM_GRANT_ANY_PRIVILEGES_NO,
                          &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE,
                            ERR_NO_GRANT_GRANT_ANY_PRIVILEGES );
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
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NO_GRANT_GRANT_ANY_PRIVILEGES );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_GRANT_ANY_PRIVILEGES_STR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-41408 normal user create, alter , drop job */
IDE_RC qdpPrivilege::checkDDLCreateAnyJobPriv( qcStatement * aStatement )
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        // check system privilege : CREATE ANY DIRECTORY
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor( aStatement,
                                                          sGranteeID,
                                                          QCM_PRIV_ID_SYSTEM_CREATE_ANY_JOB_NO,
                                                          &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic( aStatement,
                                                   sGranteeID,
                                                   QCM_PRIV_ID_SYSTEM_CREATE_ANY_JOB_NO,
                                                   &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NO_GRANT_CREATE_ANY_JOB );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_CREATE_ANY_JOB );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_CREATE_ANY_JOB_STR ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpPrivilege::checkDDLDropAnyJobPriv( qcStatement * aStatement )
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        // check system privilege : CREATE ANY DIRECTORY
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor( aStatement,
                                                          sGranteeID,
                                                          QCM_PRIV_ID_SYSTEM_DROP_ANY_JOB_NO,
                                                          &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic( aStatement,
                                                   sGranteeID,
                                                   QCM_PRIV_ID_SYSTEM_DROP_ANY_JOB_NO,
                                                   &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NO_GRANT_DROP_ANY_JOB );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_DROP_ANY_JOB );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_DROP_ANY_JOB_STR ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdpPrivilege::checkDDLAlterAnyJobPriv( qcStatement * aStatement )
{
    UInt   sGranteeID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool sExist     = ID_FALSE;

    if ( ( sGranteeID != QC_SYSTEM_USER_ID ) &&
         ( sGranteeID != QC_SYS_USER_ID ) )
    {
        // check system privilege : CREATE ANY DIRECTORY
        IDE_TEST( qcmPriv::checkSystemPrivWithoutGrantor( aStatement,
                                                          sGranteeID,
                                                          QCM_PRIV_ID_SYSTEM_ALTER_ANY_JOB_NO,
                                                          &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            // check system privilege
            // in case PUBLIC or All PRIVILEGES
            IDE_TEST( checkSystemPrivAllAndPublic( aStatement,
                                                   sGranteeID,
                                                   QCM_PRIV_ID_SYSTEM_ALTER_ANY_JOB_NO,
                                                   &sExist )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NO_GRANT_ALTER_ANY_JOB );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_GRANT_ALTER_ANY_JOB );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_ALTER_ANY_JOB_STR ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

