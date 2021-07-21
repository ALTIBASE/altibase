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
#include <qcmMView.h>

const void * gQcmMaterializedViews;
const void * gQcmMaterializedViewsIndex     [QCM_MAX_META_INDICES];

/***********************************************************************
 *
 * Description :
 *    Materialized View의 Materialized View ID를 얻는다.
 *      SELECT MVIEW_ID
 *        FROM SYS_MATERIALIZED_VIEWS_
 *       WHERE MVIEW_NAME = aMViewName AND USER_ID = aUserID;
 *
 * Implementation :
 *    1. SYS_MATERIALIZED_VIEWS_ 테이블의 USER_ID, MVIEW_NAME,
 *       MVIEW_ID 컬럼을 구한다.
 *    2. SYS_MATERIALIZED_VIEWS_ 테이블에 인덱스가 있으면,
 *       명시된 User ID, Materialized View Name 으로 keyRange 를 만들어 찾는다.
 *       인덱스가 없으면,
 *       명시된 User ID, Materialized View Name 으로 하나씩 비교해서 찾는다.
 *    3. 찾은 Row에서 Materialized View ID를 얻는다.
 *
 ***********************************************************************/
IDE_RC qcmMView::getMViewID(
                    smiStatement * aSmiStmt,
                    UInt           aUserID,
                    SChar        * aMViewName,
                    UInt           aMViewNameSize,
                    UInt         * aMViewID )
{
    idBool                sIsCursorOpen      = ID_FALSE;
    smiRange              sRange;
    smiTableCursor        sCursor;
    const void          * sRow               = NULL;
    mtcColumn           * sUserIDColumn      = NULL;
    mtcColumn           * sMViewNameColumn   = NULL;
    mtcColumn           * sMViewIDColumn     = NULL;
    UInt                  sUserID;
    mtdCharType         * sMViewName         = NULL;
    idBool                sIsFound           = ID_FALSE;
    qtcMetaRangeColumn    sFirstRangeColumn;
    qtcMetaRangeColumn    sSecondRangeColumn;

    qcNameCharBuffer      sMViewNameValueBuffer;
    mtdCharType         * sMViewNameValue = (mtdCharType *)&sMViewNameValueBuffer;

    scGRID                sRid; /* Disk Table을 위한 Record IDentifier */
    smiCursorProperties   sCursorProperty;

    sCursor.initialize();

    /* USER_ID */
    IDE_TEST( smiGetTableColumns( gQcmMaterializedViews,
                                  QCM_MATERIALIZED_VIEWS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sUserIDColumn )
              != IDE_SUCCESS );;

    /* MVIEW_NAME */
    IDE_TEST( smiGetTableColumns( gQcmMaterializedViews,
                                  QCM_MATERIALIZED_VIEWS_MVIEW_NAME_COL_ORDER,
                                  (const smiColumn**)&sMViewNameColumn )
              != IDE_SUCCESS );

    /* MVIEW_ID */
    IDE_TEST( smiGetTableColumns( gQcmMaterializedViews,
                                  QCM_MATERIALIZED_VIEWS_MVIEW_ID_COL_ORDER,
                                  (const smiColumn**)&sMViewIDColumn )
              != IDE_SUCCESS );

    if ( gQcmMaterializedViewsIndex[QCM_MATERIALIZED_VIEWS_IDX_NAME_USERID] == NULL )
    {
        SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

        /* this situtation occurs only from createdb.
         * sequencial access to metatable.
         */
        IDE_TEST( sCursor.open(
                        aSmiStmt,
                        gQcmMaterializedViews,
                        NULL,
                        smiGetRowSCN( gQcmMaterializedViews ),
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

        while ( (sRow != NULL) && (sIsFound != ID_TRUE) )
        {
            sUserID = *(UInt *)((UChar *)sRow +
                                sUserIDColumn->column.offset);

            sMViewName = (mtdCharType*)((UChar *)sRow +
                                        sMViewNameColumn->column.offset);

            if ( (sUserID == aUserID) &&
                 (idlOS::strMatch( (SChar*)&(sMViewName->value),
                                   sMViewName->length,
                                   aMViewName,
                                   aMViewNameSize ) == 0) )
            {
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* mtdModule 설정 */
        IDE_TEST( mtd::moduleById( &(sUserIDColumn->module),
                                   sUserIDColumn->type.dataTypeId )
                  != IDE_SUCCESS );
        IDE_TEST( mtd::moduleById( &(sMViewNameColumn->module),
                                   sMViewNameColumn->type.dataTypeId )
                  != IDE_SUCCESS );

        /* mtlModule 설정 */
        IDE_TEST( mtl::moduleById( &(sUserIDColumn->language),
                                   sUserIDColumn->type.languageId )
                  != IDE_SUCCESS );
        IDE_TEST( mtl::moduleById( &(sMViewNameColumn->language),
                                   sMViewNameColumn->type.languageId )
                  != IDE_SUCCESS );

        qtc::setVarcharValue( sMViewNameValue,
                              NULL,
                              aMViewName,
                              (SInt)aMViewNameSize );

        qcm::makeMetaRangeDoubleColumn(
                &sFirstRangeColumn,
                &sSecondRangeColumn,
                (const mtcColumn *) sMViewNameColumn,
                (const void *) sMViewNameValue,
                (const mtcColumn *) sUserIDColumn,
                (const void *) &aUserID,
                &sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

        IDE_TEST( sCursor.open(
                        aSmiStmt,
                        gQcmMaterializedViews,
                        gQcmMaterializedViewsIndex[QCM_MATERIALIZED_VIEWS_IDX_NAME_USERID],
                        smiGetRowSCN( gQcmMaterializedViews ),
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
    }
    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_MVIEW );

    *aMViewID = *(UInt *)((UChar *)sRow + sMViewIDColumn->column.offset);

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_MVIEW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_MVIEW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}
