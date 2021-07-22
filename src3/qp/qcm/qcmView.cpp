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
 * $Id: qcmView.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <mtuProperty.h>
#include <qdd.h>
#include <qdv.h>
#include <qcm.h>
#include <qcg.h>
#include <qcmUser.h>
#include <qcmView.h>
#include <qcpManager.h>
#include <qsvEnv.h>
#include <qsxRelatedProc.h>

const void * gQcmViews;
const void * gQcmViewParse;
const void * gQcmViewRelated;
const void * gQcmViewsIndex      [ QCM_MAX_META_INDICES ];
const void * gQcmViewParseIndex  [ QCM_MAX_META_INDICES ];
const void * gQcmViewRelatedIndex[ QCM_MAX_META_INDICES ];

IDE_RC qcmView::getStatusOfViews(
    smiStatement        * aSmiStmt,
    UInt                  aViewID,
    qcmViewStatusType   * aStatus )
{
/***********************************************************************
 *
 * Description :
 *    명시한 뷰의 상태를 구한다
 *
 * Implementation :
 *    1. 명시한 viewID 로 keyRange 를 만든다
 *    2. viewID 에 해당하는 뷰의 상태를 구한다.
 *
 ***********************************************************************/

    smiRange                sRange;
    mtcColumn             * sViewIDCol = NULL;
    mtdIntegerType          sIntDataOfViewID;
    mtdIntegerType          sIntDataOfStatus;
    qtcMetaRangeColumn      sRangeColumn;
    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;

    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties     sCursorProperty;
    mtcColumn             * sStatusMtcColumn;

    if (gQcmViews == NULL)
    {
        *aStatus = QCM_VIEW_INVALID;
    }
    else
    {
        // SELECT STATUS FROM SYS_VIEWS_ WHERE VIEW_ID = aViewID
        sCursor.initialize();

        // set data of view ID
        sIntDataOfViewID = (mtdIntegerType) aViewID;

        IDE_TEST( smiGetTableColumns( gQcmViews,
                                      QCM_VIEWS_VIEWID_COL_ORDER,
                                      (const smiColumn**)&sViewIDCol )
                  != IDE_SUCCESS );
        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sViewIDCol->module,
                                  sViewIDCol->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sViewIDCol->language,
                                  sViewIDCol->type.languageId )
                 != IDE_SUCCESS);

        qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                        sViewIDCol,
                                        &sIntDataOfViewID,
                                        &sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

        IDE_TEST( sCursor.open(
                      aSmiStmt,
                      gQcmViews,
                      gQcmViewsIndex[QCM_VIEWS_VIEWID_IDX_ORDER],
                      smiGetRowSCN(gQcmViews),
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
        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

        if ( sRow != NULL )
        {
            IDE_TEST( smiGetTableColumns( gQcmViews,
                                          QCM_VIEWS_STATUS_COL_ORDER,
                                          (const smiColumn**)&sStatusMtcColumn )
                      != IDE_SUCCESS );

            sIntDataOfStatus = *(mtdIntegerType*)
                ((UChar*) sRow + sStatusMtcColumn->column.offset);

            if (sIntDataOfStatus == 0)
            {
                *aStatus = QCM_VIEW_VALID;
            }
            else
            {
                *aStatus = QCM_VIEW_INVALID;
            }
        }
        else
        {
            // BUG-45459
            ideLog::log( IDE_SERVER_0,
                         "INVALID VIEW ID [%"ID_UINT32_FMT"]\n",
                         aViewID );

            *aStatus = QCM_VIEW_INVALID;
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

IDE_RC qcmView::getReadOnlyOfViews(
    smiStatement        * aSmiStmt,
    UInt                  aViewID,
    qcmViewReadOnly     * aReadOnly )
{
/***********************************************************************
 *
 * Description :  BUG-36350 Updatable Join DML WITH READ ONLY
 *    명시한 뷰의 READ ONLY 여부를 구한다
 *
 * Implementation :
 *    1. 명시한 viewID 로 keyRange 를 만든다
 *    2. viewID 에 해당하는 뷰의 상태를 구한다.
 *
 ***********************************************************************/

    smiRange                sRange;
    mtcColumn             * sViewIDCol = NULL;
    mtdIntegerType          sIntDataOfViewID;
    qtcMetaRangeColumn      sRangeColumn;
    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;

    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties     sCursorProperty;

    mtcColumn             * sReadOnlyColumn   = NULL;
    mtdCharType           * sReadOnly         = NULL;

    // SELECT READ_ONLY FROM SYS_VIEWS_ WHERE VIEW_ID = aViewID
    sCursor.initialize();

    // set data of view ID
    sIntDataOfViewID = (mtdIntegerType) aViewID;

    IDE_TEST( smiGetTableColumns( gQcmViews,
                                  QCM_VIEWS_VIEWID_COL_ORDER,
                                  (const smiColumn**)&sViewIDCol )
              != IDE_SUCCESS );
    // mtdModule 설정
    IDE_TEST(mtd::moduleById( &sViewIDCol->module,
                              sViewIDCol->type.dataTypeId )
             != IDE_SUCCESS);

    // mtlModule 설정
    IDE_TEST(mtl::moduleById( &sViewIDCol->language,
                              sViewIDCol->type.languageId )
             != IDE_SUCCESS);

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    sViewIDCol,
                                    &sIntDataOfViewID,
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    /* READ_ONLY */
    IDE_TEST( smiGetTableColumns( gQcmViews,
                                  QCM_VIEWS_READ_ONLY_COL_ORDER,
                                  (const smiColumn**)&sReadOnlyColumn )
              != IDE_SUCCESS );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  gQcmViews,
                  gQcmViewsIndex[QCM_VIEWS_VIEWID_IDX_ORDER],
                  smiGetRowSCN(gQcmViews),
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
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    // BUG-45459
    IDE_TEST_RAISE ( sRow == NULL, ERR_META_CRASH );

    sReadOnly = (mtdCharType*)((UChar *)sRow +
                               sReadOnlyColumn->column.offset);

    if ( idlOS::strMatch( (SChar*)&(sReadOnly->value),
                          sReadOnly->length,
                          "Y",
                          1 ) == 0 )
    {
        *aReadOnly = QCM_VIEW_READ_ONLY;
    }
    else
    {
        *aReadOnly = QCM_VIEW_NON_READ_ONLY;
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if (sStage != 0)
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;

}

IDE_RC qcmView::setInvalidViewOfRelated(
    qcStatement    * aStatement,
    UInt             aRelatedUserID,
    SChar          * aRelatedObjectName,
    UInt             aRelatedObjectNameSize,
    qsObjectType     aRelatedObjectType)
{
/***********************************************************************
 *
 * Description :
 *    명시한 오브젝트에 연결된 뷰를 찾아서 invalid 상태로 변경
 *
 * Implementation :
 *    1. 명시한 오브젝트로 연결된 뷰를 찾기 위한 keyRange 를 만든다
 *    2. 한 건식 읽으면서 viewID 를 구해서 invalid 상태로 만든다
 *    3. 메타 캐쉬를 새로 만든다
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    qtcMetaRangeColumn    sThirdMetaRange;
    mtcColumn            *sColumn;
    mtdIntegerType        sObjectTypeIntData;
    mtdIntegerType        sUserIdData;

    qcNameCharBuffer      sObjectNameBuffer;
    mtdCharType         * sObjectName = ( mtdCharType * ) & sObjectNameBuffer;

    mtdIntegerType        sIntDataOfViewID;
    smiTableCursor        sCursor;
    const void          * sRow = NULL;
    SInt                  sStage = 0;
    qcmTableInfo        * sTableInfo;
    smSCN                 sSCN;
    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    void                * sTableHandle;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;
    mtcColumn           * sThirdMtcColumn;

    if (gQcmViewRelated != NULL)
    {
        sCursor.initialize();
        // RELATED_USER_ID
        sUserIdData = (mtdIntegerType) aRelatedUserID;

        // RELATED_OBJECT_NAME
        qtc::setVarcharValue( sObjectName,
                              NULL,
                              aRelatedObjectName,
                              aRelatedObjectNameSize );

        // RELATED_OBJECT_TYPE
        sObjectTypeIntData = (mtdIntegerType) aRelatedObjectType;

        // make key range
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDUSERID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTNAME_COL_ORDER,
                                      (const smiColumn**)&sSceondMtcColumn )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTTYPE_COL_ORDER,
                                      (const smiColumn**)&sThirdMtcColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeTripleColumn(
            & sFirstMetaRange,
            & sSecondMetaRange,
            & sThirdMetaRange,
            sFirstMtcColumn,
            & sUserIdData,
            sSceondMtcColumn,
            (const void*) sObjectName,
            sThirdMtcColumn,
            & sObjectTypeIntData,
            & sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      QC_SMI_STMT( aStatement ),
                      gQcmViewRelated,
                      gQcmViewRelatedIndex
                      [QCM_VIEW_RELATED_RELUSERID_RELOBJNAME_RELOBJTYPE],
                      smiGetRowSCN(gQcmViewRelated),
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

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_VIEWID_COL_ORDER,
                                      (const smiColumn**)&sColumn )
                  != IDE_SUCCESS );

        while (sRow != NULL)
        {
            // get VIEW_ID
            sIntDataOfViewID = *(mtdIntegerType*)
                ((UChar*) sRow + sColumn->column.offset);

            // UPDATE SYS_VIEWS_ SET STATUS = invalid
            //      WHERE VIEW_ID = view_id of SYS_VIEW_RELATED_
            IDE_TEST( updateStatusOfView( aStatement,
                                          (UInt) sIntDataOfViewID,
                                          QCM_VIEW_INVALID )
                      != IDE_SUCCESS);

            // make new qcmTableInfo for VIEW
            IDE_TEST( qcm::getTableInfoByID( aStatement,
                                             sIntDataOfViewID,
                                             &sTableInfo,
                                             &sSCN,
                                             &sTableHandle)
                      != IDE_SUCCESS);

            // BUG-20387
            // view를 invalid 할때는 명시적으로 X lock을 잡아야 한다.
            IDE_TEST( qcm::validateAndLockTable( aStatement,
                                                 sTableHandle,
                                                 sSCN,
                                                 SMI_TABLE_LOCK_X )
                      != IDE_SUCCESS );

            // BUG-16422
            IDE_TEST( qcmTableInfoMgr::makeTableInfo( aStatement,
                                                      sTableInfo,
                                                      NULL )
                      != IDE_SUCCESS);

            // read next row
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
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

IDE_RC qcmView::recompileAndSetValidViewOfRelated(
    qcStatement    * aStatement,
    UInt             aRelatedUserID,
    SChar          * aRelatedObjectName,
    UInt             aRelatedObjectNameSize,
    qsObjectType     aRelatedObjectType)
{
/***********************************************************************
 *
 * Description :
 *    BUG-11266
 *    명시한 오브젝트에 연결된 뷰를 찾아서
 *    recompile을 시도하고 성공하면 invalid -> valid로 update
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmTableInfoList    * sTableInfoList = NULL;
    qcmTableInfo        * sNewTableInfo;
    UInt                  sTableID;
    smSCN                 sSCN;
    void                * sTableHandle;

    // 관련된 뷰를 찾는다.
    IDE_TEST( findTableInfoListViewOfRelated(
                  aStatement,
                  aRelatedUserID,
                  aRelatedObjectName,
                  aRelatedObjectNameSize,
                  aRelatedObjectType,
                  & sTableInfoList )
              != IDE_SUCCESS );

    while ( sTableInfoList != NULL )
    {
        sTableID = sTableInfoList->tableInfo->tableID;

        // recompile
        if ( recompileView( aStatement,
                            sTableID )
             == IDE_SUCCESS )
        {
            // UPDATE SYS_VIEWS_ SET STATUS = valid
            //      WHERE VIEW_ID = view_id of SYS_VIEW_RELATED_
            IDE_TEST( updateStatusOfView( aStatement,
                                          sTableID,
                                          QCM_VIEW_VALID )
                      != IDE_SUCCESS);

            // BUG-17119
            // recompileView에서 이미 lock을 획득했음.
            // get qcmTableInfo
            IDE_TEST( qcm::getTableInfoByID( aStatement,
                                             sTableID,
                                             & sNewTableInfo,
                                             & sSCN,
                                             & sTableHandle )
                      != IDE_SUCCESS);

            // recompile view of view
            IDE_TEST( recompileAndSetValidViewOfRelated(
                          aStatement,
                          sNewTableInfo->tableOwnerID,
                          sNewTableInfo->name,
                          idlOS::strlen((SChar*)sNewTableInfo->name),
                          QS_TABLE)
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        sTableInfoList = sTableInfoList->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmView::updateStatusOfView(
    qcStatement       * aStatement,
    UInt                aViewID,
    qcmViewStatusType   aStatus )
{
/***********************************************************************
 *
 * Description :
 *    view 와 관련된 테이블 변경시 메타 테이블 status 변경
 *
 * Implementation :
 *    명시한 status 값으로 SYS_VIEWS_ 메타 테이블의 STATUS 값을 변경한다.
 *
 ***********************************************************************/

    SChar               * sSqlStr;
    vSLong                sRowCnt;

    IDU_LIMITPOINT("qcmView::updateStatusOfView::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    if ( aStatus == QCM_VIEW_VALID )
    {
        // set invalid
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_VIEWS_ SET STATUS = 0 "
                         "WHERE VIEW_ID = INTEGER'%"ID_INT32_FMT"'", aViewID);
    }
    else
    {
        // set invalid
        idlOS::snprintf(sSqlStr, QD_MAX_SQL_LENGTH,
                        "UPDATE SYS_VIEWS_ SET STATUS = 1 "
                        "WHERE VIEW_ID = INTEGER'%"ID_INT32_FMT"'", aViewID);
    }

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
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
}

IDE_RC qcmView::checkCircularView(
    qcStatement       * aStatement,
    UInt                aUserID,
    SChar             * aViewName,
    UInt                aRelatedViewID,
    idBool            * aCircularViewExist )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    mtdIntegerType        sIntDataOfViewID;
    mtdIntegerType        sIntDataOfObjType;
    smiTableCursor        sCursor;
    const void          * sRow = NULL;
    SInt                  sStage = 0;
    mtcColumn           * sRelatedUserIDColInfo;
    mtcColumn           * sRelatedObjNameColInfo;
    UInt                  sUserID;
    mtdCharType         * sObjName;
    SChar                 sObjNameStr[QC_MAX_OBJECT_NAME_LEN + 1];

    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;

    *aCircularViewExist = ID_FALSE;

    if (gQcmViewRelated != NULL)
    {
        sCursor.initialize();


        // RELATED_USER_ID
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDUSERID_COL_ORDER,
                                      (const smiColumn**)&sRelatedUserIDColInfo )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sRelatedUserIDColInfo->module,
                                  sRelatedUserIDColInfo->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sRelatedUserIDColInfo->language,
                                  sRelatedUserIDColInfo->type.languageId )
                 != IDE_SUCCESS);

        // RELATED_OBJECT_NAME
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTNAME_COL_ORDER,
                                      (const smiColumn**)&sRelatedObjNameColInfo )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sRelatedObjNameColInfo->module,
                                  sRelatedObjNameColInfo->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sRelatedObjNameColInfo->language,
                                  sRelatedObjNameColInfo->type.languageId )
                 != IDE_SUCCESS);

        // VIEW_ID
        sIntDataOfViewID = (mtdIntegerType) aRelatedViewID;

        // RELEATED_OBJECT_TYPE
        sIntDataOfObjType = (mtdIntegerType) QS_TABLE;



        // make key range
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_VIEWID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTTYPE_COL_ORDER,
                                      (const smiColumn**)&sSceondMtcColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeDoubleColumn(
            &sFirstMetaRange,
            &sSecondMetaRange,
            sFirstMtcColumn,
            &sIntDataOfViewID,
            sSceondMtcColumn,
            &sIntDataOfObjType,
            &sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      QC_SMI_STMT( aStatement ),
                      gQcmViewRelated,
                      gQcmViewRelatedIndex
                      [QCM_VIEW_RELATED_VIEWID_IDX_ORDER],
                      smiGetRowSCN(gQcmViewRelated),
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

        while (sRow != NULL)
        {
            sUserID  = *(UInt *) ( (UChar *)sRow +
                                   sRelatedUserIDColInfo->column.offset );
            sObjName = (mtdCharType*)
                ((UChar*)sRow + sRelatedObjNameColInfo->column.offset);

            // sObjNameStr 문자열 생성
            // IDE_ASSERT( sObjName->length <= QC_MAX_OBJECT_NAME_LEN )
            idlOS::memcpy( sObjNameStr,
                           sObjName->value,
                           sObjName->length );
            sObjNameStr[sObjName->length] = '\0';

            if ( (sUserID == aUserID) &&
                 (idlOS::strCaselessMatch( sObjNameStr,
                                           aViewName ) == 0) )
            {
                *aCircularViewExist = ID_TRUE;
                break;
            }
            else
            {
                IDE_TEST( qcmView::checkCircularViewByName(
                              aStatement,
                              aUserID,
                              aViewName,
                              sUserID,
                              sObjNameStr,
                              aCircularViewExist) != IDE_SUCCESS );
                if ( *aCircularViewExist == ID_TRUE )
                {
                    break;
                }
            }
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
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

IDE_RC qcmView::checkCircularViewByName(
    qcStatement       * aStatement,
    UInt                aUserID,
    SChar             * aViewName,
    UInt                aRelatedUserID,
    SChar             * aRelatedObjName,
    idBool            * aCircularViewExist )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    mtdIntegerType        sIntDataOfViewID;
    mtdIntegerType        sIntDataOfObjType;
    smiTableCursor        sCursor;
    const void          * sRow = NULL;
    SInt                  sStage = 0;
    void                * sRelatedObjHandle;
    UInt                  sRelatedViewID;
    mtcColumn           * sRelatedUserIDColInfo;
    mtcColumn           * sRelatedObjNameColInfo;
    UInt                  sUserID;
    mtdCharType         * sObjName;
    SChar                 sObjNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    smSCN                 sSCN;

    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties   sCursorProperty;
    qcmTableInfo        * sRelatedViewTableInfo;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;

    *aCircularViewExist = ID_FALSE;

    IDE_TEST(qcm::getTableHandleByName(
                 QC_SMI_STMT( aStatement ),
                 aRelatedUserID,
                 (UChar *)aRelatedObjName,
                 idlOS::strlen(aRelatedObjName),
                 &sRelatedObjHandle,
                 &sSCN) != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sRelatedObjHandle,
                                   (void**)&sRelatedViewTableInfo )
              != IDE_SUCCESS );

    sRelatedViewID = sRelatedViewTableInfo->tableID;

    if (gQcmViewRelated != NULL)
    {
        sCursor.initialize();
        // RELATED_USER_ID
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDUSERID_COL_ORDER,
                                      (const smiColumn**)&sRelatedUserIDColInfo )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sRelatedUserIDColInfo->module,
                                  sRelatedUserIDColInfo->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sRelatedUserIDColInfo->language,
                                  sRelatedUserIDColInfo->type.languageId )
                 != IDE_SUCCESS);

        // RELATED_OBJECT_NAME
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTNAME_COL_ORDER,
                                      (const smiColumn**)&sRelatedObjNameColInfo )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &sRelatedObjNameColInfo->module,
                                  sRelatedObjNameColInfo->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &sRelatedObjNameColInfo->language,
                                  sRelatedObjNameColInfo->type.languageId )
                 != IDE_SUCCESS);

        // VIEW_ID
        sIntDataOfViewID = (mtdIntegerType) sRelatedViewID;

        // RELEATED_OBJECT_TYPE
        sIntDataOfObjType = (mtdIntegerType) QS_TABLE;


        // make key range
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_VIEWID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTTYPE_COL_ORDER,
                                      (const smiColumn**)&sSceondMtcColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeDoubleColumn(
            &sFirstMetaRange,
            &sSecondMetaRange,
            sFirstMtcColumn,
            &sIntDataOfViewID,
            sSceondMtcColumn,
            &sIntDataOfObjType,
            &sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      QC_SMI_STMT( aStatement ),
                      gQcmViewRelated,
                      gQcmViewRelatedIndex
                      [QCM_VIEW_RELATED_VIEWID_IDX_ORDER],
                      smiGetRowSCN(gQcmViewRelated),
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

        while (sRow != NULL)
        {
            sUserID  = *(UInt *) ( (UChar *)sRow +
                                   sRelatedUserIDColInfo->column.offset );
            sObjName = (mtdCharType*)
                ((UChar*)sRow + sRelatedObjNameColInfo->column.offset);

            // sObjNameStr 문자열 생성
            // IDE_ASSERT( sObjName->length <= QC_MAX_OBJECT_NAME_LEN )
            idlOS::memcpy( sObjNameStr,
                           sObjName->value,
                           sObjName->length );
            sObjNameStr[sObjName->length] = '\0';

            if ( (sUserID == aUserID) &&
                 (idlOS::strCaselessMatch( sObjNameStr,
                                           aViewName ) == 0) )
            {
                *aCircularViewExist = ID_TRUE;
                break;
            }
            else
            {
                IDE_TEST( qcmView::checkCircularViewByName(
                              aStatement,
                              aUserID,
                              aViewName,
                              sUserID,
                              sObjNameStr,
                              aCircularViewExist) != IDE_SUCCESS );
                if ( *aCircularViewExist == ID_TRUE )
                {
                    break;
                }
            }
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
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

IDE_RC qcmView::findTableInfoListViewOfRelated(
    qcStatement       * aStatement,
    UInt                aRelatedUserID,
    SChar             * aRelatedObjectName,
    UInt                aRelatedObjectNameSize,
    qsObjectType        aRelatedObjectType,
    qcmTableInfoList ** aTableInfoList)
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    qtcMetaRangeColumn    sThirdMetaRange;
    mtcColumn            *sColumn;
    mtdIntegerType        sObjectTypeIntData;
    mtdIntegerType        sUserIdData;

    qcNameCharBuffer      sObjectNameBuffer;
    mtdCharType         * sObjectName = ( mtdCharType * ) & sObjectNameBuffer;

    mtdIntegerType        sIntDataOfViewID;
    smiTableCursor        sCursor;
    const void          * sRow = NULL;
    SInt                  sStage = 0;
    qcmTableInfo        * sTableInfo;
    qcmTableInfoList    * sTableInfoList = NULL;
    qcmTableInfoList    * sTempTableInfoList = NULL;
    smSCN                 sSCN;
    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    void                * sTableHandle;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;
    mtcColumn           * sThirdMtcColumn;

    if (gQcmViewRelated != NULL)
    {
        sCursor.initialize();
        // RELATED_USER_ID
        sUserIdData = (mtdIntegerType) aRelatedUserID;

        // RELATED_OBJECT_NAME
        qtc::setVarcharValue( sObjectName,
                              NULL,
                              aRelatedObjectName,
                              aRelatedObjectNameSize );

        // RELATED_OBJECT_TYPE
        sObjectTypeIntData = (mtdIntegerType) aRelatedObjectType;

        // make key range
        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDUSERID_COL_ORDER,
                                      (const smiColumn**)&sFirstMtcColumn )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTNAME_COL_ORDER,
                                      (const smiColumn**)&sSceondMtcColumn )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_RELATEDOBJECTTYPE_COL_ORDER,
                                      (const smiColumn**)&sThirdMtcColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeTripleColumn(
            & sFirstMetaRange,
            & sSecondMetaRange,
            & sThirdMetaRange,
            sFirstMtcColumn,
            & sUserIdData,
            sSceondMtcColumn,
            (const void*) sObjectName,
            sThirdMtcColumn,
            & sObjectTypeIntData,
            & sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST( sCursor.open(
                      QC_SMI_STMT(aStatement),
                      gQcmViewRelated,
                      gQcmViewRelatedIndex
                      [QCM_VIEW_RELATED_RELUSERID_RELOBJNAME_RELOBJTYPE],
                      smiGetRowSCN(gQcmViewRelated),
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

        IDE_TEST( smiGetTableColumns( gQcmViewRelated,
                                      QCM_VIEW_RELATED_VIEWID_COL_ORDER,
                                      (const smiColumn**)&sColumn )
                  != IDE_SUCCESS );

        while (sRow != NULL)
        {
            // get VIEW_ID
            sIntDataOfViewID = *(mtdIntegerType*)
                ((UChar*) sRow + sColumn->column.offset);

            // make new qcmTableInfo for VIEW
            IDE_TEST( qcm::getTableInfoByID( aStatement,
                                             sIntDataOfViewID,
                                             &sTableInfo,
                                             &sSCN,
                                             &sTableHandle)
                      != IDE_SUCCESS);

            // DDL의 Execute 단계에서 호출하므로, X Lock을 잡는다.
            IDE_TEST( qcm::validateAndLockTable(aStatement,
                                                sTableHandle,
                                                sSCN,
                                                SMI_TABLE_LOCK_X)
                      != IDE_SUCCESS );

            if (sTableInfoList == NULL)
            {
                IDU_LIMITPOINT("qcmView::findTableInfoListViewOfRelated::malloc1");
                IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(qcmTableInfoList),
                                                   (void**)&(sTableInfoList))
                         != IDE_SUCCESS);

                sTableInfoList->tableInfo = sTableInfo;
                sTableInfoList->childInfo = NULL;
                sTableInfoList->next = NULL;
                sTempTableInfoList = sTableInfoList;
                *aTableInfoList = sTableInfoList;
            }
            else
            {
                IDU_LIMITPOINT("qcmView::findTableInfoListViewOfRelated::malloc2");
                IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(qcmTableInfoList),
                                                   (void**)&sTableInfoList)
                         != IDE_SUCCESS);

                sTableInfoList->tableInfo = sTableInfo;
                sTableInfoList->childInfo = NULL;
                sTableInfoList->next = NULL;
                sTempTableInfoList->next = sTableInfoList;
                sTempTableInfoList = sTableInfoList;
            }

            // read next row
            IDE_TEST( sCursor.readRow( & sRow, & sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
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

IDE_RC qcmView::recompileView(
    qcStatement       * aStatement,
    UInt                aTableID )
{
    qcStatement         * sCreateStatement;
    qdTableParseTree    * sCreateParseTree;
    SChar               * sStmtText;
    UInt                  sStmtTextLen;
    qcmTableInfo        * sOldTableInfo = NULL;
    qcmTableInfo        * sNewTableInfo = NULL;
    smOID                 sNewTableOID;
    smSCN                 sSCN;
    void                * sTableHandle;
    UInt                  sUserID = 0;
    UInt                  sStage = 0;

    // get qcmTableInfo
    IDE_TEST(qcm::getTableInfoByID( aStatement,
                                    aTableID,
                                    &sOldTableInfo,
                                    &sSCN,
                                    &sTableHandle )
             != IDE_SUCCESS);

    // BUG-17119
    IDE_TEST( qcm::validateAndLockTable(aStatement,
                                        sTableHandle,
                                        sSCN,
                                        SMI_TABLE_LOCK_X)
              != IDE_SUCCESS );

    // userID 백업
    sUserID = QCG_GET_SESSION_USER_ID(aStatement);

    // userID를 view의 소유자로 바꾸기
    QCG_SET_SESSION_USER_ID( aStatement, sOldTableInfo->tableOwnerID );

    //---------------------------------------------------------------
    // get the string of CREATE OR REPLACE VIEW statement and parsing
    //---------------------------------------------------------------
    // get the size of stmt text
    smiObject::getObjectInfoSize( sOldTableInfo->tableHandle,
                                  &sStmtTextLen );

    IDU_LIMITPOINT("qcmView::recompileView::malloc1");
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( sStmtTextLen + 1, (void **)(&sStmtText) )
             != IDE_SUCCESS);

    // get stmt text
    smiObject::getObjectInfo( sOldTableInfo->tableHandle,
                              (void **)(&sStmtText) );

    // BUG-42581 valgrind warning
    // string null termination
    sStmtText[sStmtTextLen] = '\0';

    // alloc qcStatement for view
    IDU_LIMITPOINT("qcmView::recompileView::malloc2");
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sCreateStatement)
             != IDE_SUCCESS);

    // set meber of qcStatement
    idlOS::memcpy( sCreateStatement, aStatement, ID_SIZEOF(qcStatement) );

    // myPlan을 재설정한다.
    sCreateStatement->myPlan = & sCreateStatement->privatePlan;
    sCreateStatement->myPlan->planEnv     = NULL;

    sCreateStatement->myPlan->stmtText    = sStmtText;
    sCreateStatement->myPlan->stmtTextLen = sStmtTextLen;
    sCreateStatement->myPlan->parseTree   = NULL;
    sCreateStatement->myPlan->plan        = NULL;

    QC_SHARED_TMPLATE(sCreateStatement)   = NULL;

    sCreateStatement->spvEnv              = NULL;

    // alloc template
    IDE_TEST( qcg::allocSharedTemplate( sCreateStatement,
                                        QCG_GET_SESSION_STACK_SIZE( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( qsvEnv::allocEnv( sCreateStatement )
              != IDE_SUCCESS );

    // parsing view
    sStage = 1;
    IDE_TEST( qcpManager::parseIt( sCreateStatement ) != IDE_SUCCESS );

    sCreateParseTree = (qdTableParseTree *)(sCreateStatement->myPlan->parseTree);
    sCreateParseTree->flag &= ~QDV_OPT_REPLACE_MASK;
    sCreateParseTree->flag |= QDV_OPT_REPLACE_TRUE;

    //---------------------------------------------------------------
    // validation
    //---------------------------------------------------------------
    IDE_TEST( sCreateStatement->myPlan->parseTree->validate( sCreateStatement )
              != IDE_SUCCESS );

    IDE_TEST(qtc::fixAfterValidation( QC_QMP_MEM(sCreateStatement),
                                      QC_SHARED_TMPLATE(sCreateStatement) )
             != IDE_SUCCESS);

    //---------------------------------------------------------------
    // optimization
    //---------------------------------------------------------------
    IDE_TEST( sCreateStatement->myPlan->parseTree->optimize( sCreateStatement )
              != IDE_SUCCESS );

    IDE_TEST(qtc::fixAfterOptimization( sCreateStatement )
             != IDE_SUCCESS);

    //---------------------------------------------------------------
    // execution
    //---------------------------------------------------------------
    // BUG-26416
    if ( ( sCreateParseTree->flag & QDV_OPT_STATUS_MASK )
         == QDV_OPT_STATUS_VALID )
    {
        // create new smiTable
        IDE_TEST( qdbCommon::createTableOnSM( sCreateStatement,
                                              sCreateParseTree->columns,
                                              sOldTableInfo->tableOwnerID,
                                              aTableID,
                                              sCreateParseTree->maxrows,
                                              sCreateParseTree->TBSAttr.mID,
                                              sCreateParseTree->segAttr,
                                              sCreateParseTree->segStoAttr,
                                              0, // tableAttrMask
                                              0, // tableAttrFlag
                                              1, // parallel degree
                                              &sNewTableOID,
                                              sCreateStatement->myPlan->stmtText,
                                              sCreateStatement->myPlan->stmtTextLen )
                  != IDE_SUCCESS );

        // get old qcmTableInfo
        IDE_TEST( qcm::getTableInfoByID( sCreateStatement,
                                         aTableID,
                                         &sOldTableInfo,
                                         &sSCN,
                                         &sTableHandle )
                  != IDE_SUCCESS);

        IDE_TEST( qcm::validateAndLockTable( sCreateStatement,
                                             sTableHandle,
                                             sSCN,
                                             SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS);

        // delete from META tables
        IDE_TEST( qdd::deleteViewFromMeta( sCreateStatement, aTableID )
                  != IDE_SUCCESS );

        // insert into META tables
        IDE_TEST( qdv::insertViewSpecIntoMeta( sCreateStatement,
                                               aTableID,
                                               sNewTableOID )
                  != IDE_SUCCESS );

        // drop old smiTable
        IDE_TEST( smiTable::dropTable( QC_SMI_STMT( sCreateStatement ),
                                       sOldTableInfo->tableHandle,
                                       SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS);

        // free PSM latch
        sStage = 0;
        IDE_TEST( qsxRelatedProc::unlatchObjects( sCreateStatement->spvEnv->procPlanList )
                  != IDE_SUCCESS );
        sCreateStatement->spvEnv->latched = ID_FALSE;

        // BUG-19209
        // make META caching structure ( qcmTableInfo )
        IDE_TEST( qcmTableInfoMgr::makeTableInfoFirst( aStatement,
                                                       aTableID,
                                                       sNewTableOID,
                                                       & sNewTableInfo )
                  != IDE_SUCCESS );

        // set VALID
        sNewTableInfo->status = QCM_VIEW_VALID;

        // BUG-19209
        // 제거할 이전의 TableInfo를 제거 목록에 등록함
        IDE_TEST( qcmTableInfoMgr::destroyTableInfo( aStatement,
                                                     sOldTableInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // userID 원복
    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            if ( qsxRelatedProc::unlatchObjects( sCreateStatement->spvEnv->procPlanList )
                 == IDE_SUCCESS )
            {
                sCreateStatement->spvEnv->latched = ID_FALSE;
            }
            else
            {
                IDE_ERRLOG(IDE_QP_1);
            }
    }

    if ( sUserID != 0 )
    {
        QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    }

    return IDE_FAILURE;
}

