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
#ifndef _O_QDP_ROLE_H_
#define  _O_QDP_ROLE_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qci.h>

class qdpRole
{

public:
    static IDE_RC validateCreateRole( qcStatement * aStatement );
    static IDE_RC validateDropRole( qcStatement * aStatement );
    
    static IDE_RC executeCreateRole( qcStatement * aStatement );
    static IDE_RC executeDropRole( qcStatement * aStatement );    

    static IDE_RC getGranteeListByRoleID( qcStatement              * aStatement,
                                          UInt                       aRoleID,
                                          qdReferenceGranteeList  ** aUserList );

    static IDE_RC getRoleCountByUserID( qcStatement  * aStatement,
                                        UInt           aGranteeID,
                                        UInt         * aRoleCount );

    static IDE_RC getRoleListByUserID( qcStatement  * aStatement,
                                       UInt           aSessionUserID,
                                       UInt         * aRoleList );    
    
    static IDE_RC checkDDLCreateTablePriv(
        qcStatement   * aStatement,
        UInt            aTableOwnerID);

    static IDE_RC checkDDLCreateViewPriv(
        qcStatement   * aStatement);

    static IDE_RC checkDDLCreateIndexPriv(
        qcStatement   * aStatement,
        qcmTableInfo  * aTableInfo,
        UInt            aIndexOwnerID);

    static IDE_RC checkDDLCreateSequencePriv(
        qcStatement   * aStatement,
        UInt            aSequenceOwnerID);

    static IDE_RC checkDDLCreatePSMPriv(
        qcStatement   * aStatement,
        UInt            aPSMOwnerID);

    // PROJ-1685
    static IDE_RC checkDDLCreateLibraryPriv(
        qcStatement   * aStatement,
        UInt            aLibraryOwnerID);

    static IDE_RC checkDDLDropLibraryPriv(
        qcStatement   * aStatement,
        UInt            aLibraryOwnerID);

    static IDE_RC checkDDLAlterLibraryPriv(
        qcStatement   * aStatement,
        UInt            aLibraryOwnerID);

    static IDE_RC checkDMLExecuteLibraryPriv(
        qcStatement    * aStatement,
        qdpObjID         aObjID,
        UInt             aLibraryOwnerID );

    // PROJ-1359 Trigger
    static IDE_RC checkDDLCreateTriggerPriv(
        qcStatement   * aStatement,
        UInt            aTriggerOwnerID);

    // To Fix PR-10618
    static IDE_RC checkDDLCreateTriggerTablePriv(
        qcStatement * aStatement,
        UInt          aTableOwnerID);

    /* PROJ-1076 Synonym */
    static IDE_RC checkDDLCreateSynonymPriv(
        qcStatement   * aStatement,
        UInt            aSynonymOwnerID);

    static IDE_RC checkDDLCreatePublicSynonymPriv(
        qcStatement   * aStatement);

    static IDE_RC checkDDLCreateUserPriv(
        qcStatement   * aStatement);

    static IDE_RC checkDDLReplicationPriv(
        qcStatement   * aStatement);

    static IDE_RC checkDDLAlterTablePriv(
        qcStatement   * aStatement,
        qcmTableInfo  * aTableInfo);

    static IDE_RC checkDDLAlterIndexPriv(
        qcStatement   * aStatement,
        qcmTableInfo  * aTableInfo,
        UInt            aIndexOwnerID);

    // BUG-16980
    static IDE_RC checkDDLAlterSequencePriv(
        qcStatement     * aStatement,
        qcmSequenceInfo * aSequenceInfo);

    static IDE_RC checkDDLAlterPSMPriv(
        qcStatement   * aStatement,
        UInt            aPSMOwnerID);

    static IDE_RC checkDDLAlterTriggerPriv(
        qcStatement   * aStatement,
        UInt            aTriggerOwnerID );

    static IDE_RC checkDDLAlterUserPriv(
        qcStatement   * aStatement,
        UInt            aRealUserID);

    static IDE_RC checkDDLAlterSystemPriv(
        qcStatement   * aStatement);

    static IDE_RC checkDDLCreateSessionPriv(
        qcStatement   * aStatement,
        UInt            aGranteeID);

    static IDE_RC checkDDLDropTablePriv(
        qcStatement   * aStatement,
        UInt            aTableOwnerID);

    static IDE_RC checkDDLDropViewPriv(
        qcStatement   * aStatement,
        UInt            aViewOwnerID);

    static IDE_RC checkDDLDropIndexPriv(
        qcStatement   * aStatement,
        qcmTableInfo  * aTableInfo,
        UInt            aIndexOwnerID);

    static IDE_RC checkDDLDropSequencePriv(
        qcStatement   * aStatement,
        UInt            aSequenceOwnerID);

    static IDE_RC checkDDLDropPSMPriv(
        qcStatement   * aStatement,
        UInt            aPSMOwnerID);

    // PROJ-1359 Trigger
    static IDE_RC checkDDLDropTriggerPriv(
        qcStatement   * aStatement,
        UInt            aTriggerOwnerID);

    static IDE_RC checkDDLDropUserPriv(
        qcStatement   * aStatement);

    // PROJ-1371 Directories
    static IDE_RC checkDDLCreateDirectoryPriv(
        qcStatement   * aStatement );

    static IDE_RC checkDDLDropDirectoryPriv(
        qcStatement   * aStatement,
        UInt            aDirectoryOwnerID );

    static IDE_RC checkDMLExecutePSMPriv(
        qcStatement                        * aStatement,
        UInt                                 aProcOwnerID,
        UInt                                 aPrivilegeCount,
        UInt                               * aGranteeID,
        idBool                               aGetSmiStmt4Prepare,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );

    static IDE_RC checkReferencesPriv( qcStatement  * aStatement,
                                       UInt           aTableOwnerID,
                                       qcmTableInfo * aTableInfo );
    
    // BUG-16980
    static IDE_RC checkDMLSelectSequencePriv(
        qcStatement                        * aStatement,
        UInt                                 aSequenceOwnerID,
        UInt                                 aSequenceID,
        idBool                               aGetSmiStmt4Prepare,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );
    
    static IDE_RC checkDMLSelectTablePriv(
        qcStatement                        * aStatement,
        UInt                                 aTableOwnerID,
        UInt                                 aPrivilegeCount,
        struct qcmPrivilege                * aPrivilegeInfo,
        idBool                               aGetSmiStmt4Prepare,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );

    static IDE_RC checkDMLInsertTablePriv(
        qcStatement                        * aStatement,
        void                               * aTableHandle,
        UInt                                 aTableOwnerID,
        UInt                                 aPrivilegeCount,
        struct qcmPrivilege                * aPrivilegeInfo,
        idBool                               aGetSmiStmt4Prepare,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );

    static IDE_RC checkDMLDeleteTablePriv(
        qcStatement                        * aStatement,
        void                               * aTableHandle,
        UInt                                 aTableOwnerID,
        UInt                                 aPrivilegeCount,
        struct qcmPrivilege                * aPrivilegeInfo,
        idBool                               aGetSmiStmt4Prepare,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );

    static IDE_RC checkDMLUpdateTablePriv(
        qcStatement                        * aStatement,
        void                               * aTableHandle,
        UInt                                 aTableOwnerID,
        UInt                                 aPrivilegeCount,
        struct qcmPrivilege                * aPrivilegeInfo,
        idBool                               aGetSmiStmt4Prepare,
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext );

    static IDE_RC checkDMLLockTablePriv( qcStatement * aStatement,
                                         UInt          aTableOwnerID );

    // PROJ-1371 Directories
    static IDE_RC checkDMLReadDirectoryPriv(
        qcStatement    * aStatement,
        qdpObjID         aObjID,
        UInt             aDirectoryOwnerID );

    static IDE_RC checkDMLWriteDirectoryPriv(
        qcStatement    * aStatement,
        qdpObjID         aObjID,
        UInt             aDirectoryOwnerID );

    static IDE_RC checkDDLCreateTableSpacePriv(
        qcStatement    * aStatement,
        UInt             aUserID);

    static IDE_RC checkDDLAlterTableSpacePriv(
        qcStatement    * aStatement,
        UInt             aUserID);

    static IDE_RC checkDDLManageTableSpacePriv(
        qcStatement    * aStatement,
        UInt             aUserID);

    static IDE_RC checkDDLDropTableSpacePriv(
        qcStatement    * aStatement,
        UInt             aUserID);

    static IDE_RC checkAccessAnyTBSPriv(
        qcStatement    * aStatement,
        UInt             aUserID,
        idBool         * aIsAccess);

    static IDE_RC checkAccessTBS(
        qcStatement    * aStatement,
        UInt             aUserID,
        scSpaceID        aTBSID );

    /* PROJ-1076 Synonym */
    static IDE_RC checkDDLDropSynonymPriv(
        qcStatement    * aStatement,
        UInt             aSynonymOwnerID);

    static IDE_RC checkDDLDropPublicSynonymPriv(
        qcStatement    * aStatement);

    /* TASK-4990 changing the method of collecting index statistics */
    static IDE_RC checkDBMSStatPriv(
        qcStatement * aStatement);

    /* PROJ-2211 Materialized View */
    static IDE_RC checkDDLCreateMViewPriv(
        qcStatement * aStatement,
        UInt          aOwnerID );

    static IDE_RC checkDDLAlterMViewPriv(
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo );

    static IDE_RC checkDDLDropMViewPriv(
        qcStatement * aStatement,
        UInt          aOwnerID );

    /*
     * PROJ-1832 New database link
     */
    static IDE_RC checkDDLCreateDatabaseLinkPriv( qcStatement * aStatement,
                                                  idBool        aPublic );
    static IDE_RC checkDDLDropDatabaseLinkPriv( qcStatement * aStatement,
                                                idBool        aPublic );

    /* PROJ-1812 ROLE */
    static IDE_RC checkDDLCreateRolePriv( qcStatement * aStatement );

    static IDE_RC checkDDLDropAnyRolePriv( qcStatement * aStatement );
    
    static IDE_RC checkDDLGrantAnyRolePriv( qcStatement * aStatement );
    
    static IDE_RC checkDDLGrantAnyPrivilegesPriv( qcStatement * aStatement );

    /* BUG-41408 normal user create, alter , drop job */
    static IDE_RC checkDDLCreateAnyJobPriv( qcStatement * aStatement );
    static IDE_RC checkDDLDropAnyJobPriv( qcStatement * aStatement );
    static IDE_RC checkDDLAlterAnyJobPriv( qcStatement * aStatement );
};

#endif // _O_QDP_ROLE_H_
