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

#include <qc.h>
#include <qcmAudit.h>
#include <qcg.h>

const void * gQcmAudit;

const void * gQcmAuditAllOpts;
const void * gQcmAuditAllOptsIndex[QCM_MAX_META_INDICES];

IDE_RC qcmAudit::getOption( smiStatement      * aSmiStmt,
                            UInt                aUserID,
                            SChar             * aObjectName,
                            UInt                aObjectNameSize,
                            qciAuditOperation * aOperation,
                            idBool            * aExist )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool                sIsCursorOpen = ID_FALSE;
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    smiTableCursor        sCursor;
    const void          * sRow              = NULL;
    mtcColumn           * sUserIDColumn     = NULL;
    mtcColumn           * sObjectNameColumn = NULL;
    mtcColumn           * sOperColumn[QCI_AUDIT_OPER_MAX][2];
    qtcMetaRangeColumn    sFirstRangeColumn;
    qtcMetaRangeColumn    sSecondRangeColumn;

    UInt                  sUserID;
    qcNameCharBuffer      sObjectNameValueBuffer;
    mtdCharType         * sObjectNameValue = (mtdCharType *)&sObjectNameValueBuffer;
    mtdCharType         * sObjectNameStr;
    mtdCharType         * sSuccessStr;
    mtdCharType         * sFailureStr;

    scGRID                sRid;
    smiCursorProperties   sCursorProperty;
    UInt                  i;
    UInt                  j;

    *aExist = ID_FALSE;

    sCursor.initialize();

    /* USER_ID */
    IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                  QCM_AUDIT_ALL_OPTS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sUserIDColumn )
              != IDE_SUCCESS );

    /* OBJECT_NAME */
    IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                  QCM_AUDIT_ALL_OPTS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sObjectNameColumn )
              != IDE_SUCCESS );

    for ( i = 0, j = QCM_AUDIT_ALL_OPTS_SELECT_SUCCESS_COL_ORDER;
          i < QCI_AUDIT_OPER_MAX;
          i++ )
    {
        /* OPER_SUCCESS */
        IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                      j++,
                                      (const smiColumn**)&sOperColumn[i][0] )
                  != IDE_SUCCESS );

        /* OPER_FAILURE */
        IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                      j++,
                                      (const smiColumn**)&sOperColumn[i][1] )
                  != IDE_SUCCESS );
    }

    /* mtdModule 설정 */
    IDE_TEST( mtd::moduleById( &(sUserIDColumn->module),
                               sUserIDColumn->type.dataTypeId )
              != IDE_SUCCESS );
    IDE_TEST( mtd::moduleById( &(sObjectNameColumn->module),
                               sObjectNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    /* mtlModule 설정 */
    IDE_TEST( mtl::moduleById( &(sUserIDColumn->language),
                               sUserIDColumn->type.languageId )
              != IDE_SUCCESS );
    IDE_TEST( mtl::moduleById( &(sObjectNameColumn->language),
                               sObjectNameColumn->type.languageId )
              != IDE_SUCCESS );

    if ( aObjectNameSize > 0 )
    {
        /* value 설정 */
        qtc::setVarcharValue( sObjectNameValue,
                              NULL,
                              aObjectName,
                              (SInt)aObjectNameSize );

        qcm::makeMetaRangeDoubleColumn(
            &sFirstRangeColumn,
            &sSecondRangeColumn,
            (const mtcColumn *) sUserIDColumn,
            (const void *) & aUserID,
            (const mtcColumn *) sObjectNameColumn,
            (const void *) sObjectNameValue,
            & sRange );
    }
    else
    {
        // 검색 조건 (user_id, null)
        qtc::initializeMetaRange( &sRange,
                                  MTD_COMPARE_MTDVAL_MTDVAL );  // Meta is memory table

        qtc::setMetaRangeColumn( &sFirstMetaRange,
                                 (const mtcColumn *) sUserIDColumn,
                                 (const void *) & aUserID,
                                 SMI_COLUMN_ORDER_ASCENDING,
                                 0 );  // First column of Index

        qtc::addMetaRangeColumn( &sRange,
                                 &sFirstMetaRange );

        qtc::setMetaRangeIsNullColumn( &sSecondMetaRange,
                                       (const mtcColumn *) sObjectNameColumn,
                                       1 );  // Second column of Index

        qtc::addMetaRangeColumn( &sRange,
                                 &sSecondMetaRange );

        qtc::fixMetaRange( &sRange );
    }

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  gQcmAuditAllOpts,
                  gQcmAuditAllOptsIndex[QCM_AUDIT_ALL_OPTS_USERID_OBJNAME_IDX_ORDER],
                  smiGetRowSCN( gQcmAuditAllOpts ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        sUserID = *(UInt*)((UChar *)sRow + sUserIDColumn->column.offset);
        sObjectNameStr = (mtdCharType*)((UChar *)sRow + sObjectNameColumn->column.offset);

        aOperation->userID = sUserID;

        idlOS::strncpy( aOperation->objectName,
                        (SChar*)sObjectNameStr->value,
                        sObjectNameStr->length );
        aOperation->objectName[sObjectNameStr->length] = 0;

        for ( i = 0; i < QCI_AUDIT_OPER_MAX; i++ )
        {
            sSuccessStr = (mtdCharType*)((UChar *)sRow + sOperColumn[i][0]->column.offset);
            sFailureStr = (mtdCharType*)((UChar *)sRow + sOperColumn[i][1]->column.offset);

            aOperation->operation[i] = 0;

            if ( idlOS::strMatch( (SChar*)sSuccessStr->value,
                                  sSuccessStr->length,
                                  "S",
                                  1 ) == 0 )
            {
                aOperation->operation[i] |= QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE;
            }
            else if ( idlOS::strMatch( (SChar*)sSuccessStr->value,
                                       sSuccessStr->length,
                                       "A",
                                       1 ) == 0 )
            {
                aOperation->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE;
            }
            else if ( idlOS::strMatch( (SChar*)sSuccessStr->value,
                                       sSuccessStr->length,
                                       "T",
                                       1 ) == 0 )
            {
                aOperation->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE;
                //aOperation->operation[i] |= QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            if ( idlOS::strMatch( (SChar*)sFailureStr->value,
                                  sFailureStr->length,
                                  "S",
                                  1 ) == 0 )
            {
                aOperation->operation[i] |= QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE;
            }
            else if ( idlOS::strMatch( (SChar*)sFailureStr->value,
                                       sFailureStr->length,
                                       "A",
                                       1 ) == 0 )
            {
                aOperation->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE;
            }
            else if ( idlOS::strMatch( (SChar*)sFailureStr->value,
                                       sFailureStr->length,
                                       "T",
                                       1 ) == 0 )
            {
                aOperation->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE;
                //aOperation->operation[i] |= QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }

        *aExist = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmAudit::getAllOptions( smiStatement        * aSmiStmt,
                                qciAuditOperation  ** aObjectOptions,
                                UInt                * aObjectOptionCount,
                                qciAuditOperation  ** aUserOptions,
                                UInt                * aUserOptionCount )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool                sIsCursorOpen = ID_FALSE;
    smiTableCursor        sCursor;
    const void          * sRow              = NULL;
    mtcColumn           * sUserIDColumn     = NULL;
    mtcColumn           * sObjectNameColumn = NULL;
    mtcColumn           * sOperColumn[QCI_AUDIT_OPER_MAX][2];

    qciAuditOperation   * sObjectOptions = NULL;
    UInt                  sObjectOptionCount = 0;
    qciAuditOperation   * sUserOptions = NULL;
    UInt                  sUserOptionCount = 0;
    qciAuditOperation   * sOption;

    UInt                  sUserID;
    mtdCharType         * sObjectNameStr;
    mtdCharType         * sSuccessStr;
    mtdCharType         * sFailureStr;

    scGRID                sRid;
    smiCursorProperties   sCursorProperty;
    UInt                  i;
    UInt                  j;

    sCursor.initialize();

    /* USER_ID */
    IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                  QCM_AUDIT_ALL_OPTS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sUserIDColumn )
              != IDE_SUCCESS );

    /* OBJECT_NAME */
    IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                  QCM_AUDIT_ALL_OPTS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sObjectNameColumn )
              != IDE_SUCCESS );

    for ( i = 0, j = QCM_AUDIT_ALL_OPTS_SELECT_SUCCESS_COL_ORDER;
          i < QCI_AUDIT_OPER_MAX;
          i++ )
    {
        /* OPER_SUCCESS */
        IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                      j++,
                                      (const smiColumn**)&sOperColumn[i][0] )
                  != IDE_SUCCESS );

        /* OPER_FAILURE */
        IDE_TEST( smiGetTableColumns( gQcmAuditAllOpts,
                                      j++,
                                      (const smiColumn**)&sOperColumn[i][1] )
                  != IDE_SUCCESS );
    }

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  gQcmAuditAllOpts,
                  gQcmAuditAllOptsIndex[QCM_AUDIT_ALL_OPTS_USERID_OBJNAME_IDX_ORDER],
                  smiGetRowSCN( gQcmAuditAllOpts ),
                  NULL,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sObjectNameStr = (mtdCharType*)((UChar *)sRow + sObjectNameColumn->column.offset);

        if ( sObjectNameStr->length > 0 )
        {
            sObjectOptionCount++;
        }
        else
        {
            sUserOptionCount++;
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    if ( sObjectOptionCount > 0 )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF(qciAuditOperation) * sObjectOptionCount,
                                     (void**) & sObjectOptions )
                  != IDE_SUCCESS);

        *aObjectOptions     = sObjectOptions;
        *aObjectOptionCount = sObjectOptionCount;
    }
    else
    {
        *aObjectOptions     = NULL;
        *aObjectOptionCount = 0;
    }

    if ( sUserOptionCount > 0 )
    {
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF(qciAuditOperation) * sUserOptionCount,
                                     (void**) & sUserOptions )
                  != IDE_SUCCESS);

        *aUserOptions     = sUserOptions;
        *aUserOptionCount = sUserOptionCount;
    }
    else
    {
        *aUserOptions     = NULL;
        *aUserOptionCount = 0;
    }

    if ( sObjectOptionCount + sUserOptionCount > 0 )
    {
        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        while ( sRow != NULL )
        {
            sUserID = *(UInt*)((UChar *)sRow + sUserIDColumn->column.offset);
            sObjectNameStr = (mtdCharType*)((UChar *)sRow + sObjectNameColumn->column.offset);

            if ( sObjectNameStr->length > 0 )
            {
                sOption = sObjectOptions;
                sObjectOptions++;

                sOption->userID = sUserID;

                idlOS::strncpy( sOption->objectName,
                                (SChar*)sObjectNameStr->value,
                                sObjectNameStr->length );
                sOption->objectName[sObjectNameStr->length] = '\0';
            }
            else
            {
                sOption = sUserOptions;
                sUserOptions++;

                sOption->userID = sUserID;
                sOption->objectName[0] = '\0';
            }

            for ( i = 0; i < QCI_AUDIT_OPER_MAX; i++ )
            {
                sSuccessStr = (mtdCharType*)((UChar *)sRow + sOperColumn[i][0]->column.offset);
                sFailureStr = (mtdCharType*)((UChar *)sRow + sOperColumn[i][1]->column.offset);

                sOption->operation[i] = 0;

                if ( idlOS::strMatch( (SChar*)sSuccessStr->value,
                                      sSuccessStr->length,
                                      "S",
                                      1 ) == 0 )
                {
                    sOption->operation[i] |= QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE;
                }
                else if ( idlOS::strMatch( (SChar*)sSuccessStr->value,
                                           sSuccessStr->length,
                                           "A",
                                           1 ) == 0 )
                {
                    sOption->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE;
                }
                else if ( idlOS::strMatch( (SChar*)sSuccessStr->value,
                                           sSuccessStr->length,
                                           "T",
                                           1 ) == 0 )
                {
                    sOption->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE;
                    //sOption->operation[i] |= QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE;
                }
                else
                {
                    // Nothing to do.
                }

                if ( idlOS::strMatch( (SChar*)sFailureStr->value,
                                      sFailureStr->length,
                                      "S",
                                      1 ) == 0 )
                {
                    sOption->operation[i] |= QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE;
                }
                else if ( idlOS::strMatch( (SChar*)sFailureStr->value,
                                           sFailureStr->length,
                                           "A",
                                           1 ) == 0 )
                {
                    sOption->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE;
                }
                else if ( idlOS::strMatch( (SChar*)sFailureStr->value,
                                           sFailureStr->length,
                                           "T",
                                           1 ) == 0 )
                {
                    sOption->operation[i] |= QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE;
                    //sOption->operation[i] |= QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }

            IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    if ( sObjectOptions != NULL )
    {
        (void)iduMemMgr::free(sObjectOptions);
    }

    if ( sUserOptions != NULL )
    {
        (void)iduMemMgr::free(sUserOptions);
    }

    return IDE_FAILURE;
}

IDE_RC qcmAudit::getStatus( smiStatement  * aSmiStmt,
                            idBool        * aIsStarted )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool                sIsCursorOpen = ID_FALSE;
    smiTableCursor        sCursor;
    const void          * sRow             = NULL;
    mtcColumn           * sIsStartedColumn = NULL;
    SInt                  sIsStarted;
    scGRID                sRid;
    smiCursorProperties   sCursorProperty;

    *aIsStarted = ID_FALSE;

    sCursor.initialize();

    /* IS_STARTED */
    IDE_TEST( smiGetTableColumns( gQcmAudit,
                                  QCM_AUDIT_IS_STARTED_COL_ORDER,
                                  (const smiColumn**)&sIsStartedColumn )
              != IDE_SUCCESS );

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  gQcmAudit,
                  NULL,
                  smiGetRowSCN( gQcmAudit ),
                  NULL,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_META_CRASH );

    sIsStarted = *(SInt *)((UChar *)sRow + sIsStartedColumn->column.offset);

    if ( sIsStarted > 0 )
    {
        *aIsStarted = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmAudit::updateObjectName( qcStatement  * aStatement,
                                   UInt           aUserID,
                                   SChar        * aOldObjectName,
                                   SChar        * aNewObjectName )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    vSLong   sRowCnt = 0;
    SChar  * sSqlStr = NULL;

    // 메타 변경
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_AUDIT_ALL_OPTS_ "
                     "SET OBJECT_NAME = VARCHAR'%s' "
                     "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND OBJECT_NAME = VARCHAR'%s'",
                     aNewObjectName,
                     aUserID,
                     aOldObjectName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    // 0건일수도 있고 1건일수도 있으므로
    // sRowCnt는 검사하지 않는다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmAudit::deleteObject( qcStatement  * aStatement,
                               UInt           aUserID,
                               SChar        * aObjectName )
{
/***********************************************************************
 *
 * Description : PROJ-2223 audit
 *
 * Implementation :
 *
 ***********************************************************************/

    vSLong   sRowCnt = 0;
    SChar  * sSqlStr = NULL;

    // 메타 변경
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_AUDIT_ALL_OPTS_ "
                     "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND OBJECT_NAME = VARCHAR'%s'",
                     aUserID,
                     aObjectName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    // 0건일수도 있고 1건일수도 있으므로
    // sRowCnt는 검사하지 않는다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

