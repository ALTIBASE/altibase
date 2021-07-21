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
 * $Id: qcmPriv.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <iduMemory.h>
#include <ide.h>
#include <qcm.h>
#include <qcmPriv.h>
#include <qcg.h>

const void * gQcmPrivileges;
const void * gQcmPrivilegesIndex [ QCM_MAX_META_INDICES ];
const void * gQcmGrantSystem;
const void * gQcmGrantSystemIndex [ QCM_MAX_META_INDICES ];
const void * gQcmGrantObject;
const void * gQcmGrantObjectIndex [ QCM_MAX_META_INDICES ];

/* PROJ-1812 ROLE */
const void * gQcmUserRoles;
const void * gQcmUserRolesIndex [ QCM_MAX_META_INDICES ];

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::getQcmPrivileges()
 *
 * Argument :
 *    aSmiStmt   - smiStatement for opening smiCursor.
 *    aTableID   - caching table ID
 *    aTableInfo - table meta caching structure
 *
 * Description :
 *    This function is called to cache privilege information of table
 *        in starting server or in executing DDL.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::getQcmPrivileges(
    smiStatement  * aSmiStmt,
    qdpObjID        aTableID,
    qcmTableInfo  * aTableInfo)
{
    qcmPrivilege          * sQcmPriv   = NULL;
    smiRange                sRange;
    qtcMetaRangeColumn      sFirstMetaRange;
    qtcMetaRangeColumn      sSecondMetaRange;
    mtcColumn             * sObjIDCol;
    mtcColumn             * sObjTypeCol;
    mtdBigintType           sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;
    qcmGrantObject          sGrantObject;
    UInt                    sNextPrivInfo = 0;
    UInt                    i;

    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties     sCursorProperty;

    // initialize
    aTableInfo->privilegeCount = 0;

    if (gQcmGrantObject == NULL)
    {
        // on createdb : not yet created CONSTRAINTS META
        aTableInfo->privilegeInfo  = NULL;
    }
    else
    {
        //----------------------------------
        // SELECT COUNT(*)
        //   FROM SYS_GRANT_OBJECT_
        //  WHERE OBJ_ID = aTableID
        //    AND OBJ_TYPE = 'T';
        //----------------------------------
        sCursor.initialize();


        // set data of object ID
        sBigIntDataOfObjID = (mtdBigintType) aTableID;

        // set data of object type
        qtc::setVarcharValue( sCharDataOfObjType,
                              NULL,
                              (SChar*) "T",
                              1 );

        IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                      QCM_GRANT_OBJECT_OBJ_ID,
                                      (const smiColumn**)&sObjIDCol )
                  != IDE_SUCCESS );


        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sObjIDCol->module,
                                  sObjIDCol->type.dataTypeId)
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sObjIDCol->language,
                                  sObjIDCol->type.languageId)
                 != IDE_SUCCESS);

        IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                      QCM_GRANT_OBJECT_OBJ_TYPE,
                                      (const smiColumn**)&sObjTypeCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sObjTypeCol->module,
                                  sObjTypeCol->type.dataTypeId)
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sObjTypeCol->language,
                                  sObjTypeCol->type.languageId)
                 != IDE_SUCCESS);

        // make key range for index
        // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTOR_ID, GRANTEE_ID, USER_ID)
        qcm::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                        & sSecondMetaRange,
                                        sObjIDCol,
                                        & sBigIntDataOfObjID,
                                        sObjTypeCol,
                                        (const void*) sCharDataOfObjType,
                                        & sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      aSmiStmt,
                      gQcmGrantObject,
                      gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX1_ORDER],
                      smiGetRowSCN( gQcmGrantObject ),
                      NULL,
                      & sRange,
                      smiGetDefaultKeyRange(),
                      smiGetDefaultFilter(),
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      &sCursorProperty)
                  != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

        // counting
        while (sRow != NULL)
        {
            aTableInfo->privilegeCount++;

            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        }

        if (aTableInfo->privilegeCount != 0)
        {
            // alloc aTableInfo->privilegeInfo
            IDU_FIT_POINT( "qcmPriv::getQcmPrivileges::malloc::privilegeInfo",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                                       ID_SIZEOF(qcmPrivilege) * aTableInfo->privilegeCount,
                                       (void**)&(aTableInfo->privilegeInfo))
                     != IDE_SUCCESS);

            // initialize aTableInfo->privilegeInfo
            for (i = 0; i < aTableInfo->privilegeCount; i++)
            {
                aTableInfo->privilegeInfo[i].userID = ID_UINT_MAX;
                aTableInfo->privilegeInfo[i].privilegeBitMap = 0;
            }

            //----------------------------------
            // SELECT GRANTEE_ID, PRIV_ID
            //   FROM SYS_GRANT_OBJECT_
            //  WHERE OBJ_ID = aTableID
            //    AND OBJ_TYPE = 'T';
            //----------------------------------
            IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

            // set aTableInfo->privilegeInfo
            while (sRow != NULL)
            {
                // set qcmGrantObject member
                IDE_TEST( setQcmGrantObject( (void *)sRow, &sGrantObject )
                          != IDE_SUCCESS );;

                // search sGrantObject.granteeID in qcmTableInfo
                IDE_TEST(searchPrivilegeInfo(aTableInfo,
                                             sGrantObject.granteeID,
                                             &sQcmPriv)
                         != IDE_SUCCESS);

                if (sQcmPriv == NULL)
                {
                    // first found sGrantObject.granteeID
                    sQcmPriv = &(aTableInfo->privilegeInfo[sNextPrivInfo]);
                    sQcmPriv->userID = sGrantObject.granteeID;
                    sNextPrivInfo++;
                }

                // set privilege ID
                IDE_TEST(setPrivilegeBitMap(sGrantObject.privID, sQcmPriv)
                         != IDE_SUCCESS);

                // next
                IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                          != IDE_SUCCESS );
            } // end of while
        }
        else
        {
            aTableInfo->privilegeInfo  = NULL;
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::getGranteeOfPSM()
 *
 * Argument :
 *    aSmiStmt    - smiStatement for opening smiCursor.
 *    aPSMID      - procedure ID in parse tree.
 *    aPrivilegeCount -
 *    aGranteeIDs -
 *
 * Description :
 *    This function is called to cache privilege information of procedure.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::getGranteeOfPSM(
    smiStatement  * aSmiStmt,
    qdpObjID        aPSMID,
    UInt          * aPrivilegeCount,
    UInt         ** aGranteeIDs)
{
    UInt                    sPrivilegeCount = 0;
    UInt                  * sGranteeIDs = NULL;
    smiRange                sRange;
    qtcMetaRangeColumn      sFirstMetaRange;
    qtcMetaRangeColumn      sSecondMetaRange;
    mtcColumn             * sObjIDCol;
    mtcColumn             * sObjTypeCol;
    mtdBigintType           sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;
    qcmGrantObject          sGrantObject;
    UInt                    sNextPrivInfo = 0;

    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties     sCursorProperty;

    if (gQcmGrantObject == NULL)
    {
        // on createdb : not yet created CONSTRAINTS META

        // Nothing to do.
    }
    else
    {
        //----------------------------------
        // SELECT COUNT(*)
        //   FROM SYS_GRANT_OBJECT_
        //  WHERE OBJ_ID = aPSMID
        //    AND OBJ_TYPE = 'P';
        //----------------------------------
        sCursor.initialize();


        // set data of object ID
        sBigIntDataOfObjID = (mtdBigintType) aPSMID;
        // set data of object type
        qtc::setVarcharValue( sCharDataOfObjType,
                              NULL,
                              (SChar*) "P",
                              1 );

        IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                      QCM_GRANT_OBJECT_OBJ_ID,
                                      (const smiColumn**)&sObjIDCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sObjIDCol->module,
                                  sObjIDCol->type.dataTypeId)
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sObjIDCol->language,
                                  sObjIDCol->type.languageId)
                 != IDE_SUCCESS);

        IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                      QCM_GRANT_OBJECT_OBJ_TYPE,
                                      (const smiColumn**)&sObjTypeCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sObjTypeCol->module,
                                  sObjTypeCol->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sObjTypeCol->language,
                                  sObjTypeCol->type.languageId )
                 != IDE_SUCCESS);

        // make key range for index
        // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTOR_ID, GRANTEE_ID, USER_ID)
        qcm::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                        & sSecondMetaRange,
                                        sObjIDCol,
                                        & sBigIntDataOfObjID,
                                        sObjTypeCol,
                                        (const void*) sCharDataOfObjType,
                                        & sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      aSmiStmt,
                      gQcmGrantObject,
                      gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX1_ORDER],
                      smiGetRowSCN( gQcmGrantObject ),
                      NULL,
                      & sRange,
                      smiGetDefaultKeyRange(),
                      smiGetDefaultFilter(),
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      &sCursorProperty)
                  != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // counting
        while (sRow != NULL)
        {
            sPrivilegeCount++;
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }

        if (sPrivilegeCount != 0)
        {
            // alloc sGranteeIDs
            IDU_FIT_POINT( "qcmPriv::getGranteeOfPSM::malloc::sGranteeIDs",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                                       ID_SIZEOF(UInt) * sPrivilegeCount,
                                       (void**)&sGranteeIDs)
                     != IDE_SUCCESS);

            //----------------------------------
            // SELECT GRANTEE_ID, PRIV_ID
            //   FROM SYS_GRANT_OBJECT_
            //  WHERE OBJ_ID = aPSMID
            //    AND OBJ_TYPE = 'P';
            //----------------------------------
            IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );

            // set sGranteeIDs
            while (sRow != NULL)
            {
                // set qcmGrantObject member
                IDE_TEST( setQcmGrantObject( (void *)sRow, &sGrantObject )
                          != IDE_SUCCESS );

                sGranteeIDs[sNextPrivInfo] = sGrantObject.granteeID;
                sNextPrivInfo++;

                // next
                IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                          != IDE_SUCCESS );
            } // end of while
        }
        else
        {
            // Nothing to do.
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    *aPrivilegeCount = sPrivilegeCount;
    *aGranteeIDs     = sGranteeIDs;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    // to fix BUG-24905
    // granteeid를 alloc한 이후 실패한 경우
    // free를 해 주어야 함
    if( sGranteeIDs != NULL )
    {
        (void)iduMemMgr::free(sGranteeIDs);
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qcmPriv::searchPrivilegeInfo()
 *
 * Argument :
 *    aTableInfo - cached table meta information    ( IN )
 *    aGranteeID - user ID of searching privilege   ( IN )
 *    aQcmPriv   - searching privilege information  ( OUT )
 *
 * Description :
 *    searching qcmPrivilege of aGranteeID in qcmTableInfo
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::searchPrivilegeInfo(
    qcmTableInfo  * aTableInfo,
    UInt            aGranteeID,
    qcmPrivilege ** aQcmPriv)
{
    UInt              i;
    qcmPrivilege    * sQcmPriv = NULL;

    *aQcmPriv = NULL;

    for ( i = 0; i < aTableInfo->privilegeCount; i++ )
    {
        sQcmPriv = &(aTableInfo->privilegeInfo[i]);

        if (sQcmPriv->userID == aGranteeID)
        {
            // FOUND
            *aQcmPriv = sQcmPriv;
            break;
        }
    }

    return IDE_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qcmPriv::checkPrivilegeInfo()
 *
 * Argument :
 *    aTableInfo - cached table meta information    ( IN )
 *    aGranteeID - user ID of searching privilege   ( IN )
 *    aPrivilegeID - user ID of searching privilege   ( IN )
 *    aQcmPriv   - searching privilege information  ( OUT )
 *
 * Description :
 *    searching qcmPrivilege of aGranteeID in qcmTableInfo
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::checkPrivilegeInfo(
    UInt                   aPrivilegeCount,
    struct qcmPrivilege  * aPrivilegeInfo,
    UInt                   aGranteeID,
    UInt                   aPrivilegeID,
    qcmPrivilege        ** aQcmPriv)
{
    UInt              i;

    qcmPrivilege    * sQcmPriv = NULL;

    *aQcmPriv = NULL;

    for ( i = 0; i < aPrivilegeCount; i++ )
    {
        sQcmPriv = &(aPrivilegeInfo[i]);

        if ((sQcmPriv->userID == aGranteeID) || (
                (sQcmPriv->userID == QC_PUBLIC_USER_ID) &&
                (aGranteeID != QC_PUBLIC_USER_ID)))
        {
            switch (aPrivilegeID)
            {
                case QCM_PRIV_ID_OBJECT_ALTER_NO :

                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_ALTER_ON) ==
                        QCM_OBJECT_PRIV_ALTER_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_DELETE_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_DELETE_ON) ==
                        QCM_OBJECT_PRIV_DELETE_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_EXECUTE_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_EXECUTE_ON) ==
                        QCM_OBJECT_PRIV_EXECUTE_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_INDEX_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_INDEX_ON) ==
                        QCM_OBJECT_PRIV_INDEX_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_INSERT_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_INSERT_ON) ==
                        QCM_OBJECT_PRIV_INSERT_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_REFERENCES_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_REFERENCES_ON) ==
                        QCM_OBJECT_PRIV_REFERENCES_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_SELECT_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_SELECT_ON) ==
                        QCM_OBJECT_PRIV_SELECT_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                case QCM_PRIV_ID_OBJECT_UPDATE_NO :
                    if ((sQcmPriv->privilegeBitMap &
                         QCM_OBJECT_PRIV_UPDATE_ON) ==
                        QCM_OBJECT_PRIV_UPDATE_ON )
                    {
                        // FOUND
                        *aQcmPriv = sQcmPriv;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return IDE_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qcmPriv::setPrivilegeBitMap()
 *
 * Argument :
 *    aPrivID  - privilege ID
 *    aQcmPriv - cached privilege information in qcmTableInfo
 *
 * Description :
 *    setting aPrivID in qcmPrivilege->privilegeBitMap
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::setPrivilegeBitMap(
    UInt            aPrivID,
    qcmPrivilege  * aQcmPriv)
{
    switch (aPrivID)
    {
        case QCM_PRIV_ID_OBJECT_ALTER_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_ALTER_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_ALTER_ON;
            break;
        case QCM_PRIV_ID_OBJECT_DELETE_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_DELETE_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_DELETE_ON;
            break;
        case QCM_PRIV_ID_OBJECT_EXECUTE_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_EXECUTE_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_EXECUTE_ON;
            break;
        case QCM_PRIV_ID_OBJECT_INDEX_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_INDEX_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_INDEX_ON;
            break;
        case QCM_PRIV_ID_OBJECT_INSERT_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_INSERT_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_INSERT_ON;
            break;
        case QCM_PRIV_ID_OBJECT_REFERENCES_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_REFERENCES_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_REFERENCES_ON;
            break;
        case QCM_PRIV_ID_OBJECT_SELECT_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_SELECT_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_SELECT_ON;
            break;
        case QCM_PRIV_ID_OBJECT_UPDATE_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_UPDATE_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_UPDATE_ON;
            break;
        case QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO :
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_ALTER_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_ALTER_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_DELETE_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_DELETE_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_EXECUTE_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_EXECUTE_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_INDEX_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_INDEX_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_INSERT_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_INSERT_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_REFERENCES_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_REFERENCES_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_SELECT_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_SELECT_ON;
            aQcmPriv->privilegeBitMap &= ~QCM_OBJECT_PRIV_UPDATE_MASK;
            aQcmPriv->privilegeBitMap |= QCM_OBJECT_PRIV_UPDATE_ON;
            break;
        default:
            IDE_RAISE(ERR_NOT_DEFINED_PRIV_ID);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_DEFINED_PRIV_ID);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "NOT defined Privilege ID"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qcmPriv::checkSystemPrivWithGrantor()
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGrantorID - grantor ID
 *    aGranteeID - grantee ID
 *    aPrivID    - privilege ID
 *    aExist     - existence of result set ( OUT )
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_GRANT_SYSTEM
 *      WHERE GRANTOR_ID = aGrantorID
 *        AND GRANTEE_ID = aGranteeID
 *        AND PRIV_ID = aPrivID;
 *
 *    If result set exists, then aExist = ID_TRUE
 *                          else aExist = ID_FALSE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::checkSystemPrivWithGrantor(
    qcStatement     * aStatement,
    UInt              aGrantorID,
    UInt              aGranteeID,
    UInt              aPrivID,
    idBool          * aExist)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    mtdIntegerType         sIntDataOfGrantorID;
    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfPrivID;
    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;

    sCursor.initialize();


    // set data of grantee ID
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    // set data of privilege ID
    sIntDataOfPrivID = (mtdIntegerType) aPrivID;
    // set data of grantor ID
    sIntDataOfGrantorID = (mtdIntegerType) aGrantorID;

    IDE_TEST( smiGetTableColumns( gQcmGrantSystem,
                                  QCM_GRANT_SYSTEM_GRANTEE_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantSystem,
                                  QCM_GRANT_SYSTEM_PRIV_ID,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantSystem,
                                  QCM_GRANT_SYSTEM_GRANTOR_ID,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );
    // make key range for index ( GRANTEE_ID, PRIV_ID, GRANTOR_ID )
    qcm::makeMetaRangeTripleColumn( & sFirstMetaRange,
                                    & sSecondMetaRange,
                                    & sThirdMetaRange,
                                    sFirstMtcColumn,
                                    & sIntDataOfGranteeID,
                                    sSceondMtcColumn,
                                    & sIntDataOfPrivID,
                                    sThirdMtcColumn,
                                    & sIntDataOfGrantorID,
                                    & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, aStatement->mStatistics);

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmGrantSystem,
                  gQcmGrantSystemIndex[QCM_GRANT_SYSTEM_IDX1_ORDER],
                  smiGetRowSCN( gQcmGrantSystem ),
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

    if ( sRow != NULL )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        *aExist = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qcmPriv::checkSystemPrivWithoutGrantor()
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGranteeID - grantee ID in parse tree.
 *    aPrivID    - privilege ID in parse tree.
 *    aExist     - existence of result set ( OUT )
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_GRANT_SYSTEM_
 *      WHERE GRANTEE_ID = aGranteeID
 *        AND PRIV_ID = aPrivID;
 *
 *    If result set exists, then aExist = ID_TRUE
 *                          else aExist = ID_FALSE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::checkSystemPrivWithoutGrantor(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    UInt              aPrivID,
    idBool          * aExist)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfPrivID;
    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;

    sCursor.initialize();

    // set data of grantee ID
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    // set data of privilege ID
    sIntDataOfPrivID = (mtdIntegerType) aPrivID;

    IDE_TEST( smiGetTableColumns( gQcmGrantSystem,
                                  QCM_GRANT_SYSTEM_GRANTEE_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantSystem,
                                  QCM_GRANT_SYSTEM_PRIV_ID,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );
    // make key range for index ( GRANTEE_ID, PRIV_ID, GRANTOR_ID )
    qcm::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                    & sSecondMetaRange,
                                    sFirstMtcColumn,
                                    & sIntDataOfGranteeID,
                                    sSceondMtcColumn,
                                    & sIntDataOfPrivID,
                                    & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmGrantSystem,
                  gQcmGrantSystemIndex[QCM_GRANT_SYSTEM_IDX1_ORDER],
                  smiGetRowSCN( gQcmGrantSystem ),
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

    if ( sRow != NULL )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        *aExist = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::checkObjectPrivWithGrantOption()
 *
 * Argument :
 *     aStatement - which have parse tree, iduMemory, session information.
 *     aGranteeID - grantee ID in parse tree.
 *     aPrivID    - privilege ID in parse tree.
 *     aObjID     - object ID in parse tree.
 *     aObjType   - object type in parse tree.
 *     aWithGrantOption - set of WITH GRANT OPTIN
 *     aExist     - existence of result set ( OUT )
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_GRANT_OBJECT_
 *      WHERE GRANTEE_ID = aGranteeID
 *        AND PRIV_ID = aPrivID
 *        AND OBJ_ID = aObjID
 *        AND OBJ_TYPE = aObjType
 *        AND WITH_GRANT_OPTION = aWithGrantOption
 *
 *    If result set exists, then aExist = ID_TRUE
 *                          else aExist = ID_FALSE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::checkObjectPrivWithGrantOption(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    UInt              aPrivID,
    qdpObjID          aObjID,
    SChar           * aObjType,
    UInt              aWithGrantOption,
    idBool          * aExist)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    qtcMetaRangeColumn     sFourthMetaRange;
    qtcMetaRangeColumn     sFifthMetaRange;
    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfPrivID;
    mtdBigintType          sBigIntDataOfObjID;
    mtdIntegerType         sIntDataOfWithGrantOption;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;
    mtcColumn            * sFourthMtcColumn;
    mtcColumn            * sFifthMtcColumn;

    sCursor.initialize();

    // set data of grantee ID
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    // set data of privilege ID
    sIntDataOfPrivID = (mtdIntegerType) aPrivID;
    // set data of object ID
    sBigIntDataOfObjID = (mtdBigintType) aObjID;
    // set data of object type
    qtc::setVarcharValue( sCharDataOfObjType,
                          NULL,
                          aObjType,
                          1 );
    // set data of with_grant_option
    sIntDataOfWithGrantOption = (mtdIntegerType) aWithGrantOption;

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_TYPE,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_PRIV_ID,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTEE_ID,
                                  (const smiColumn**)&sFourthMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_WITH_GRANT_OPTION,
                                  (const smiColumn**)&sFifthMtcColumn )
              != IDE_SUCCESS );
    // make key range for index
    // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTEE_ID, WITH_GRANT_OPTION)
    qcm::makeMetaRangeFiveColumn( & sFirstMetaRange,
                                  & sSecondMetaRange,
                                  & sThirdMetaRange,
                                  & sFourthMetaRange,
                                  & sFifthMetaRange,
                                  sFirstMtcColumn,
                                  & sBigIntDataOfObjID,
                                  sSceondMtcColumn,
                                  (const void*) sCharDataOfObjType,
                                  sThirdMtcColumn,
                                  & sIntDataOfPrivID,
                                  sFourthMtcColumn,
                                  & sIntDataOfGranteeID,
                                  sFifthMtcColumn,
                                  & sIntDataOfWithGrantOption,
                                  & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmGrantObject,
                  gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX2_ORDER],
                  smiGetRowSCN( gQcmGrantObject ),
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
    if ( sRow != NULL )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        *aExist = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    return IDE_FAILURE;
}

// PROJ-1371 Directories
IDE_RC qcmPriv::checkObjectPriv(
    qcStatement     * aStatement,
    UInt              aGranteeID,
    UInt              aPrivID,
    qdpObjID          aObjID,
    SChar           * aObjType,
    idBool          * aExist)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    qtcMetaRangeColumn     sFourthMetaRange;


    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfPrivID;
    mtdBigintType          sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;
    mtcColumn            * sFourthMtcColumn;

    sCursor.initialize();


    // set data of grantee ID
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    // set data of privilege ID
    sIntDataOfPrivID = (mtdIntegerType) aPrivID;
    // set data of object ID
    sBigIntDataOfObjID = (mtdBigintType) aObjID;
    // set data of object type
    qtc::setVarcharValue( sCharDataOfObjType,
                          NULL,
                          aObjType,
                          1 );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_TYPE,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_PRIV_ID,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTEE_ID,
                                  (const smiColumn**)&sFourthMtcColumn )
              != IDE_SUCCESS );

    // make key range for index
    // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTEE_ID)
    qcm::makeMetaRangeFourColumn( & sFirstMetaRange,
                                  & sSecondMetaRange,
                                  & sThirdMetaRange,
                                  & sFourthMetaRange,
                                  sFirstMtcColumn,
                                  & sBigIntDataOfObjID,
                                  sSceondMtcColumn,
                                  (const void*) sCharDataOfObjType,
                                  sThirdMtcColumn,
                                  & sIntDataOfPrivID,
                                  sFourthMtcColumn,
                                  & sIntDataOfGranteeID,
                                  & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmGrantObject,
                  gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX2_ORDER],
                  smiGetRowSCN( gQcmGrantObject ),
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
    if ( sRow != NULL )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        *aExist = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::getGrantObjectWithGrantee()
 *
 * Argument :
 *     aStatement - which have parse tree, iduMemory, session information.
 *     aMemory    -
 *     aGrantorID - grantor ID in parse tree.
 *     aGranteeID - grantee ID in parse tree.
 *     aPrivID    - privilege ID in parse tree.
 *     aObjID     - object ID in parse tree.
 *     aObjType   - object type in parse tree.
 *     aGrantObject - meta cache of grant object.
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_GRANT_OBJECT
 *      WHERE GRANTOR_ID = aGrantorID
 *        AND GRANTEE_ID = aGranteeID
 *        AND PRIV_ID = aPrivID
 *        AND OBJ_ID = aObjID
 *        AND OBJ_TYPE = aObjType;
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::getGrantObjectWithGrantee(
    qcStatement     * aStatement,
    iduVarMemList   * aMemory,
    UInt              aGrantorID,
    UInt              aGranteeID,
    UInt              aPrivID,
    qdpObjID          aObjID,
    SChar           * aObjType,
    qcmGrantObject ** aGrantObject)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    qtcMetaRangeColumn     sFourthMetaRange;
    qtcMetaRangeColumn     sFifthMetaRange;
    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfGrantorID;
    mtdIntegerType         sIntDataOfPrivID;
    mtdBigintType          sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    qcmGrantObject       * sGrantObject;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;
    mtcColumn            * sFourthMtcColumn;
    mtcColumn            * sFifthMtcColumn;

    sCursor.initialize();


    // set data of grantor ID
    sIntDataOfGrantorID = (mtdIntegerType) aGrantorID;
    // set data of grantee ID
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    // set data of privilege ID
    sIntDataOfPrivID = (mtdIntegerType) aPrivID;
    // set data of object ID
    sBigIntDataOfObjID = (mtdBigintType) aObjID;
    // set data of object type
    qtc::setVarcharValue( sCharDataOfObjType,
                          NULL,
                          aObjType,
                          1 );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_TYPE,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_PRIV_ID,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTOR_ID,
                                  (const smiColumn**)&sFourthMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTEE_ID,
                                  (const smiColumn**)&sFifthMtcColumn )
              != IDE_SUCCESS );

    // make key range for index
    // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTOR_ID, GRANTEE_ID, USER_ID)
    qcm::makeMetaRangeFiveColumn( & sFirstMetaRange,
                                  & sSecondMetaRange,
                                  & sThirdMetaRange,
                                  & sFourthMetaRange,
                                  & sFifthMetaRange,
                                  sFirstMtcColumn,
                                  & sBigIntDataOfObjID,
                                  sSceondMtcColumn,
                                  (const void*) sCharDataOfObjType,
                                  sThirdMtcColumn,
                                  & sIntDataOfPrivID,
                                  sFourthMtcColumn,
                                  & sIntDataOfGrantorID,
                                  sFifthMtcColumn,
                                  & sIntDataOfGranteeID,
                                  & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmGrantObject,
                  gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX1_ORDER],
                  smiGetRowSCN( gQcmGrantObject ),
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

    // initialize
    *aGrantObject = NULL;

    if ( sRow != NULL )
    {
        while (sRow != NULL)
        {
            // make qcmGrantObject
            IDE_TEST( makeQcmGrantObject( aMemory,
                                          (void *)sRow,
                                          &sGrantObject )
                      != IDE_SUCCESS );

            // link
            if (*aGrantObject == NULL)
            {
                *aGrantObject = sGrantObject;
            }
            else
            {
                sGrantObject->next = *aGrantObject;
                *aGrantObject = sGrantObject;
            }

            // get next row
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        } // end of while
    }
    else
    {
        IDE_RAISE(ERR_NO_GRANT);
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED,
                                aPrivID));
    }
    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    *aGrantObject = NULL;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    qcmPriv::getGrantObjectWithoutGrantee()
 *
 * Argument :
 *     aStatement - which have parse tree, iduMemory, session information.
 *     aMemory    -
 *     aGrantorID - grantor ID in parse tree.
 *     aPrivID    - privilege ID in parse tree.
 *     aObjID     - object ID in parse tree.
 *     aObjType   - object type in parse tree.
 *     aGrantObject - meta cache of grant object.
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_GRANT_OBJECT
 *      WHERE GRANTOR_ID = aGrantorID
 *        AND PRIV_ID = aPrivID
 *        AND OBJ_ID = aObjID
 *        AND OBJ_TYPE = aObjType;
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::getGrantObjectWithoutGrantee(
    qcStatement     * aStatement,
    iduMemory       * aMemory,
    UInt              aGrantorID,
    UInt              aPrivID,
    qdpObjID          aObjID,
    SChar           * aObjType,
    qcmGrantObject ** aGrantObject)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    qtcMetaRangeColumn     sFourthMetaRange;

    mtdIntegerType         sIntDataOfGrantorID;
    mtdIntegerType         sIntDataOfPrivID;
    mtdBigintType          sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
                                ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    qcmGrantObject       * sGrantObject;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;
    mtcColumn            * sFourthMtcColumn;

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    sCursor.initialize();

    // set data of grantor ID
    sIntDataOfGrantorID = (mtdIntegerType) aGrantorID;
    // set data of privilege ID
    sIntDataOfPrivID = (mtdIntegerType) aPrivID;
    // set data of object ID
    sBigIntDataOfObjID = (mtdBigintType) aObjID;
    // set data of object type
    qtc::setVarcharValue( sCharDataOfObjType,
                          NULL,
                          aObjType,
                          1 );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_TYPE,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_PRIV_ID,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTOR_ID,
                                  (const smiColumn**)&sFourthMtcColumn )
              != IDE_SUCCESS );

    // make key range for index
    // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTOR_ID, GRANTEE_ID, USER_ID)
    qcm::makeMetaRangeFourColumn( & sFirstMetaRange,
                                  & sSecondMetaRange,
                                  & sThirdMetaRange,
                                  & sFourthMetaRange,
                                  sFirstMtcColumn,
                                  & sBigIntDataOfObjID,
                                  sSceondMtcColumn,
                                  (const void*) sCharDataOfObjType,
                                  sThirdMtcColumn,
                                  & sIntDataOfPrivID,
                                  sFourthMtcColumn,
                                  & sIntDataOfGrantorID,
                                  & sRange );

    IDE_TEST( sCursor.open(
            QC_SMI_STMT( aStatement ),
            gQcmGrantObject,
            gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX1_ORDER],
            smiGetRowSCN( gQcmGrantObject ),
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

    // initialize
    *aGrantObject = NULL;

    if ( sRow != NULL )
    {
        while (sRow != NULL)
        {
            // make qcmGrantObject
            IDE_TEST( makeQcmGrantObject( aMemory,
                                          (void *)sRow,
                                          &sGrantObject )
                != IDE_SUCCESS );

            // link
            if (*aGrantObject == NULL)
            {
                *aGrantObject = sGrantObject;
            }
            else
            {
                sGrantObject->next = *aGrantObject;
                *aGrantObject = sGrantObject;
            }

            // get next row
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        } // end of while
    }
    else
    {
        *aGrantObject = NULL;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    *aGrantObject = NULL;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::getGrantObjectWithoutPrivilege()
 *
 * Argument :
 *     aStatement - which have parse tree, iduMemory, session information.
 *     aMemory    -
 *     aGrantorID - grantor ID in parse tree.
 *     aGranteeID - grantee ID in parse tree.
 *     aObjID     - object ID in parse tree.
 *     aObjType   - object type in parse tree.
 *     aGrantObject - meta cache of grant object.
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_GRANT_OBJECT_
 *      WHERE GRANTOR_ID = aGrantorID
 *        AND GRANTEE_ID = aGranteeID
 *        AND OBJ_ID = aObjID
 *        AND OBJ_TYPE = aObjType;
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::getGrantObjectWithoutPrivilege(
    qcStatement     * aStatement,
    iduVarMemList   * aMemory,
    UInt              aGrantorID,
    UInt              aGranteeID,
    qdpObjID          aObjID,
    SChar           * aObjType,
    qcmGrantObject ** aGrantObject)
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    qtcMetaRangeColumn     sFourthMetaRange;
    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfGrantorID;
    mtdBigintType          sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    qcmGrantObject       * sGrantObject;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;
    mtcColumn            * sFourthMtcColumn;

    sCursor.initialize();

    // set data of grantor ID
    sIntDataOfGrantorID = (mtdIntegerType) aGrantorID;
    // set data of grantee ID
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    // set data of object ID
    sBigIntDataOfObjID = (mtdBigintType) aObjID;
    // set data of object type
    qtc::setVarcharValue( sCharDataOfObjType,
                          NULL,
                          aObjType,
                          1 );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_OBJ_TYPE,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTOR_ID,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTEE_ID,
                                  (const smiColumn**)&sFourthMtcColumn )
              != IDE_SUCCESS );

    // make key range for index
    // (OBJ_ID, OBJ_TYPE, GRANTOR_ID, GRANTEE_ID)
    qcm::makeMetaRangeFourColumn( & sFirstMetaRange,
                                  & sSecondMetaRange,
                                  & sThirdMetaRange,
                                  & sFourthMetaRange,
                                  sFirstMtcColumn,
                                  & sBigIntDataOfObjID,
                                  sSceondMtcColumn,
                                  (const void*) sCharDataOfObjType,
                                  sThirdMtcColumn,
                                  & sIntDataOfGrantorID,
                                  sFourthMtcColumn,
                                  & sIntDataOfGranteeID,
                                  & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmGrantObject,
                  gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX3_ORDER],
                  smiGetRowSCN( gQcmGrantObject ),
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

    // initialize
    *aGrantObject = NULL;

    if ( sRow != NULL )
    {
        while (sRow != NULL)
        {
            // make qcmGrantObject
            IDE_TEST( makeQcmGrantObject( aMemory,
                                          (void *)sRow,
                                          &sGrantObject )
                      != IDE_SUCCESS );

            // link
            if (*aGrantObject == NULL)
            {
                *aGrantObject = sGrantObject;
            }
            else
            {
                sGrantObject->next = *aGrantObject;
                *aGrantObject = sGrantObject;
            }

            // get next row
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        } // end of while
    }
    else
    {
        IDE_RAISE(ERR_NO_GRANT);
    }
    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GRANT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDP_REVOKE_PRIV_NOT_GRANTED,
                                QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_NO));
    }
    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    *aGrantObject = NULL;

    return IDE_FAILURE;
}
/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::makeQcmGrantObject()
 *
 * Argument :
 *     aMemory -
 *     aRow    -
 *
 * Description :
 *        make meta cache structure of grant object.
 *
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::makeQcmGrantObject(
    iduMemory       * aMemory,
    void            * aRow,
    qcmGrantObject ** aGrantObject)
{
    qcmGrantObject      * sGrantObject;

    // alloc memory
    IDU_FIT_POINT( "qcmPriv::makeQcmGrantObject::alloc::sGrantObject1",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC(aMemory, qcmGrantObject, &sGrantObject) != IDE_SUCCESS);

    // set qcmGrantObject member
    IDE_TEST( setQcmGrantObject( aRow, sGrantObject)
              != IDE_SUCCESS );

    // return
    *aGrantObject = sGrantObject;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aGrantObject = NULL;

    return IDE_FAILURE;
}

IDE_RC qcmPriv::makeQcmGrantObject(
    iduVarMemList   * aMemory,
    void            * aRow,
    qcmGrantObject ** aGrantObject)
{
    qcmGrantObject      * sGrantObject;

    // alloc memory
    IDU_FIT_POINT( "qcmPriv::makeQcmGrantObject::alloc::sGrantObject2",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(STRUCT_ALLOC(aMemory, qcmGrantObject, &sGrantObject)
             != IDE_SUCCESS);

    // set qcmGrantObject member
    IDE_TEST( setQcmGrantObject( aRow, sGrantObject)
              != IDE_SUCCESS );

    // return
    *aGrantObject = sGrantObject;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aGrantObject = NULL;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::setQcmGrantObject()
 *
 * Argument :
 *     aMemory -
 *     aRow    -
 *
 * Description :
 *     set meta cache structure of grant object.
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::setQcmGrantObject(
    void            * aRow,
    qcmGrantObject  * aGrantObject)
{
    mtdCharType         * sCharData;
    mtdIntegerType        sIntData;
    qcmGrantObject      * sGrantObject = aGrantObject;
    SLong                 sObjectID;
    mtcColumn            * sObjectGrantorIDMtcColumn;
    mtcColumn            * sObjectGranteeIDMtcColumn;
    mtcColumn            * sObjectPrivIDMtcColumn;
    mtcColumn            * sObjectUserIDMtcColumn;
    mtcColumn            * sObjectIDMtcColumn;
    mtcColumn            * sObjectTypeMtcColumn;
    mtcColumn            * sWithGrantOptionMtcColumn;

    // set sGrantObject->grantorID
    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTOR_ID,
                                  (const smiColumn**)&sObjectGrantorIDMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + sObjectGrantorIDMtcColumn->column.offset );
    sGrantObject->grantorID = (SInt) sIntData;

    // set sGrantObject->granteeID
    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_GRANTEE_ID,
                                  (const smiColumn**)&sObjectGranteeIDMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + sObjectGranteeIDMtcColumn->column.offset );
    sGrantObject->granteeID = (SInt) sIntData;

    // set sGrantObject->privID
    IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                  QCM_GRANT_OBJECT_PRIV_ID,
                                  (const smiColumn**)&sObjectPrivIDMtcColumn )
              != IDE_SUCCESS );
     sIntData = *(mtdIntegerType*)
     ((UChar*) aRow + sObjectPrivIDMtcColumn->column.offset );

    sGrantObject->privID = (SInt) sIntData;

    // set sGrantObject->userID
     IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                   QCM_GRANT_OBJECT_USER_ID,
                                   (const smiColumn**)&sObjectUserIDMtcColumn )
               != IDE_SUCCESS );
     sIntData = *(mtdIntegerType*)
     ((UChar*) aRow + sObjectUserIDMtcColumn->column.offset );
     sGrantObject->userID = (SInt) sIntData;

     // set sGrantObject->objID
     IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                   QCM_GRANT_OBJECT_OBJ_ID,
                                   (const smiColumn**)&sObjectIDMtcColumn )
               != IDE_SUCCESS );
     qcm::getBigintFieldValue (
         aRow,
         sObjectIDMtcColumn,
         & sObjectID );
     // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
     // 변환하므로 데이터 손실 가능성 있음
     sGrantObject->objID = (qdpObjID)sObjectID;

     // set sGrantObject->objType
     IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                   QCM_GRANT_OBJECT_OBJ_TYPE,
                                   (const smiColumn**)&sObjectTypeMtcColumn )
               != IDE_SUCCESS );
     sCharData = (mtdCharType*)
     ((UChar*) aRow + sObjectTypeMtcColumn->column.offset);
     idlOS::memcpy( &(sGrantObject->objType),
                    sCharData->value,
                    1 );

     // set sGrantObject->withGrantOption
     IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                   QCM_GRANT_OBJECT_WITH_GRANT_OPTION,
                                   (const smiColumn**)&sWithGrantOptionMtcColumn )
               != IDE_SUCCESS );
     sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + sWithGrantOptionMtcColumn->column.offset );
    sGrantObject->withGrantOption = (SInt) sIntData;

    sGrantObject->next = NULL;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     qcmPriv::updateLastDDLTime()
 *
 * Argument :
 *
 * Description :
 *
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::updateLastDDLTime( qcStatement * aStatement,
                                   SChar       * aObjectType,
                                   qdpObjID      aObjID )
{
    SChar               * sSqlStr;
    vSLong                sRowCnt;

    if( ( aObjectType[0] == 'T' ) ||
        ( aObjectType[0] == 'S' ) ||
        ( aObjectType[0] == 'P' ) ||
        ( aObjectType[0] == 'A' ) ||
        ( aObjectType[0] == 'D' ) )
    {
        // fix BUG-14394
        IDU_FIT_POINT( "qcmPriv::updateLastDDLTime::alloc::sSqlStr",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          &sSqlStr )
                  != IDE_SUCCESS);

        if( ( aObjectType[0] == 'T' ) ||
            ( aObjectType[0] == 'S' ) )
        {
            // table, sequence일 때는 aObjID가 tableID, sequenceID로
            // UInt 타입이다.
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_TABLES_ "
                             "SET LAST_DDL_TIME = SYSDATE "
                             "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                             UInt(aObjID) );
        }
        else if( aObjectType[0] == 'P' )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_PROCEDURES_ "
                             "SET    LAST_DDL_TIME = SYSDATE "
                             "WHERE  PROC_OID = BIGINT'%"ID_INT64_FMT"'",
                             ULong(aObjID) );
        }
        // PROJ-1073 Package
        else if( aObjectType[0] == 'A' )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_PACKAGES_ "
                             "SET    LAST_DDL_TIME = SYSDATE "
                             "WHERE  PACKAGE_OID = BIGINT'%"ID_INT64_FMT"'",
                             ULong(aObjID) );
        }
        else /* if( aObjectType[0] == 'D' )*/
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_DIRECTORIES_ "
                             "SET LAST_DDL_TIME = SYSDATE "
                             "WHERE DIRECTORY_ID = BIGINT'%"ID_INT64_FMT"'",
                             ULong(aObjID) );
        }

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, err_updated_count_is_not_1 );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_updated_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                   "[qcmPriv::updateLastDDLTime]"
                   "err_updated_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1073 Package
IDE_RC qcmPriv::getGranteeOfPkg(
    smiStatement  * aSmiStmt,
    qdpObjID        aPkgID,
    UInt          * aPrivilegeCount,
    UInt         ** aGranteeIDs)
{
    UInt                    sPrivilegeCount = 0;
    UInt                  * sGranteeIDs = NULL;
    smiRange                sRange;
    qtcMetaRangeColumn      sFirstMetaRange;
    qtcMetaRangeColumn      sSecondMetaRange;
    mtcColumn             * sObjIDCol;
    mtcColumn             * sObjTypeCol;
    mtdBigintType           sBigIntDataOfObjID;

    qcNameCharBuffer        sCharDataOfObjTypeBuffer;
    mtdCharType           * sCharDataOfObjType =
        ( mtdCharType * ) & sCharDataOfObjTypeBuffer;

    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;
    qcmGrantObject          sGrantObject;
    UInt                    sNextPrivInfo = 0;

    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties     sCursorProperty;

    if (gQcmGrantObject == NULL)
    {
        // on createdb : not yet created CONSTRAINTS META

        // Nothing to do.
    }
    else
    {
        //----------------------------------
        // SELECT COUNT(*)
        //   FROM SYS_GRANT_OBJECT_
        //  WHERE OBJ_ID = aPkgID
        //    AND OBJ_TYPE = 'A';
        //----------------------------------
        sCursor.initialize();


        // set data of object ID
        sBigIntDataOfObjID = (mtdBigintType) aPkgID;
        // set data of object type
        qtc::setVarcharValue( sCharDataOfObjType,
                              NULL,
                              (SChar*) "A",
                              1 );

        IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                      QCM_GRANT_OBJECT_OBJ_ID,
                                      (const smiColumn**)&sObjIDCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sObjIDCol->module,
                                  sObjIDCol->type.dataTypeId)
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sObjIDCol->language,
                                  sObjIDCol->type.languageId)
                 != IDE_SUCCESS);

        IDE_TEST( smiGetTableColumns( gQcmGrantObject,
                                      QCM_GRANT_OBJECT_OBJ_TYPE,
                                      (const smiColumn**)&sObjTypeCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sObjTypeCol->module,
                                  sObjTypeCol->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sObjTypeCol->language,
                                  sObjTypeCol->type.languageId )
                 != IDE_SUCCESS);

        // make key range for index
        // (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTOR_ID, GRANTEE_ID, USER_ID)
        qcm::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                        & sSecondMetaRange,
                                        sObjIDCol,
                                        & sBigIntDataOfObjID,
                                        sObjTypeCol,
                                        (const void*) sCharDataOfObjType,
                                        & sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      aSmiStmt,
                      gQcmGrantObject,
                      gQcmGrantObjectIndex[QCM_GRANT_OBJECT_IDX1_ORDER],
                      smiGetRowSCN( gQcmGrantObject ),
                      NULL,
                      & sRange,
                      smiGetDefaultKeyRange(),
                      smiGetDefaultFilter(),
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      &sCursorProperty)
                  != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // counting
        while (sRow != NULL)
        {
            sPrivilegeCount++;
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }

        if (sPrivilegeCount != 0)
        {
            // alloc sGranteeIDs
            IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                                       ID_SIZEOF(UInt) * sPrivilegeCount,
                                       (void**)&sGranteeIDs)
                     != IDE_SUCCESS);

            //----------------------------------
            // SELECT GRANTEE_ID, PRIV_ID
            //   FROM SYS_GRANT_OBJECT_
            //  WHERE OBJ_ID = aPkgID
            //    AND OBJ_TYPE = 'A';
            //----------------------------------
            IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );

            // set sGranteeIDs
            while (sRow != NULL)
            {
                // set qcmGrantObject member
                IDE_TEST( setQcmGrantObject( (void *)sRow, &sGrantObject )
                          != IDE_SUCCESS );

                sGranteeIDs[sNextPrivInfo] = sGrantObject.granteeID;
                sNextPrivInfo++;

                // next
                IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                          != IDE_SUCCESS );
            } // end of while
        }
        else
        {
            // Nothing to do.
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    *aPrivilegeCount = sPrivilegeCount;
    *aGranteeIDs     = sGranteeIDs;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        sCursor.close();
    }

    // to fix BUG-24905
    // granteeid를 alloc한 이후 실패한 경우
    // free를 해 주어야 함
    if( sGranteeIDs != NULL )
    {
        (void)iduMemMgr::free(sGranteeIDs);
    }

    return IDE_FAILURE;
}
/*-----------------------------------------------------------------------------
 * Name : PROJ-1812 ROLE
 *    qcmPriv::checkRoleWithGrantor()
 *
 * Argument :
 *    aStatement - which have parse tree, iduMemory, session information.
 *    aGrantorID - grantor ID
 *    aGranteeID - grantee ID
 *    aPrivID    - privilege ID
 *    aExist     - existence of result set ( OUT )
 *
 * Description :
 *
 *     SELECT *
 *       FROM SYS_USER_ROLES_
 *      WHERE GRANTOR_ID = aGrantorID
 *        AND GRANTEE_ID = aGranteeID
 *        AND ROLE_ID = aGrantedRoleID;
 *
 *    If result set exists, then aExist = ID_TRUE
 *                          else aExist = ID_FALSE
 *
 * ---------------------------------------------------------------------------*/

IDE_RC qcmPriv::checkRoleWithGrantor( qcStatement     * aStatement,
                                      UInt              aGrantorID,
                                      UInt              aGranteeID,
                                      UInt              aRoleID,
                                      idBool          * aExist )
{
    smiRange               sRange;
    qtcMetaRangeColumn     sFirstMetaRange;
    qtcMetaRangeColumn     sSecondMetaRange;
    qtcMetaRangeColumn     sThirdMetaRange;
    mtdIntegerType         sIntDataOfGrantorID;
    mtdIntegerType         sIntDataOfGranteeID;
    mtdIntegerType         sIntDataOfRoleID;
    smiTableCursor         sCursor;
    SInt                   sStage = 0;
    const void           * sRow = NULL;
    smiCursorProperties    sCursorProperty;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn            * sFirstMtcColumn;
    mtcColumn            * sSceondMtcColumn;
    mtcColumn            * sThirdMtcColumn;

    sCursor.initialize();

    sIntDataOfGrantorID = (mtdIntegerType) aGrantorID;
    sIntDataOfGranteeID = (mtdIntegerType) aGranteeID;
    sIntDataOfRoleID = (mtdIntegerType) aRoleID;

    // make key range for index ( GRANTEE_ID, PRIV_ID, GRANTOR_ID )
    IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                  QCM_USER_ROLES_GRANTOR_ID_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                  QCM_USER_ROLES_GRANTEE_ID_COL_ORDER,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmUserRoles,
                                  QCM_USER_ROLES_ROLE_ID_COL_ORDER,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeTripleColumn(
        & sFirstMetaRange,
        & sSecondMetaRange,
        & sThirdMetaRange,
        sFirstMtcColumn,
        & sIntDataOfGrantorID,
        sSceondMtcColumn,
        & sIntDataOfGranteeID,
        sThirdMtcColumn,
        & sIntDataOfRoleID,
        & sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, aStatement->mStatistics);

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  gQcmUserRoles,
                  gQcmUserRolesIndex[QCM_USER_ROLES_IDX1_ORDER],
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

    if ( sRow != NULL )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        *aExist = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        (void) sCursor.close();
    }
    else
    {
        /* Notihg To Do */
    }
    
    return IDE_FAILURE;   
}
