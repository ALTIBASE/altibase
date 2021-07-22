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

#include <idl.h>
#include <ide.h>
#include <mtd.h>

#include <qcm.h>
#include <qcg.h>
#include <qmn.h>
#include <qmv.h>
#include <qdm.h>
#include <qdn.h>
#include <qcmUser.h>
#include <qcmView.h>
#include <qcmMView.h>
#include <qcmProc.h>
#include <qdbCreate.h>
#include <qdbCommon.h>
#include <qdpPrivilege.h>
#include <qcpManager.h>
#include <qcuSqlSourceInfo.h>
#include <qsvEnv.h>
#include <qdpRole.h>

/***********************************************************************
 *
 * Description :
 *    CREATE MATERIALIZED VIEW ... 의 validation 수행
 *
 * Implementation :
 *    1. Materialized View, Table, View가 이미 존재하는지 확인
 *    2. Materialized View 생성 권한이 있는지 확인
 *    3. 지원하는 Refresh인지 확인
 *    4. validation of TABLESPACE
 *    5. validation of SELECT statement
 *    6. select 문에 sequence(currval, nextval)가 있으면 오류
 *    7. validation of column and target
 *    8. check circular view definition
 *
 ***********************************************************************/
IDE_RC qdm::validateCreate( qcStatement * aStatement )
{
    qdTableParseTree     * sParseTree      = NULL;
    qsOID                  sProcID;
    idBool                 sExist          = ID_FALSE;
    qdPartitionAttribute * sPartAttr       = NULL;
    UInt                   sPartCount      = 0;
    qmsPartCondValList     sOldPartCondVal;
    qcmColumn            * sColumn         = NULL;
    UInt                   sColumnCount    = 0;
    qcuSqlSourceInfo       sqlInfo;

    qcNamePosition         sViewNamePosition;
    idBool                 sCircularViewExist = ID_FALSE;
    qsRelatedObjects     * sRelatedObject     = NULL;
    UInt                   sRelatedObjectUserID;
    scSpaceID              sTBSAttrID         = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
    smiTableSpaceType      sTBSAttrType       = SMI_MEMORY_SYSTEM_DICTIONARY;

#ifdef ALTI_CFG_EDITION_DISK
    SInt                   sCountMemType  = 0;
    SInt                   sCountVolType  = 0;
#endif

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->tableName) );
        IDE_RAISE( ERR_RESERVED_WORD_IN_OBJECT_NAME );
    }
    else
    {
        /* Nothing to do */
    }

    QC_STR_COPY( sParseTree->mviewViewName, sParseTree->tableName );
    (void)idlVA::appendString( sParseTree->mviewViewName,
                               QC_MAX_OBJECT_NAME_LEN + 1,
                               QDM_MVIEW_VIEW_SUFFIX,
                               QDM_MVIEW_VIEW_SUFFIX_SIZE );
    sViewNamePosition.stmtText = sParseTree->mviewViewName;
    sViewNamePosition.offset   = 0;
    sViewNamePosition.size     = idlOS::strlen( sParseTree->mviewViewName );

    /* check user existence */
    if ( QC_IS_NULL_NAME( sParseTree->userName ) == ID_TRUE )
    {
        sParseTree->userID = QCG_GET_SESSION_USER_ID( aStatement );
    }
    else
    {
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      sParseTree->userName,
                                      &(sParseTree->userID) )
                  != IDE_SUCCESS );
    }

    /* check the materialized view exists */
    if ( (sParseTree->flag & QDV_OPT_REPLACE_MASK) == QDV_OPT_REPLACE_FALSE )
    {
        IDE_TEST( existMViewByName( QC_SMI_STMT( aStatement ),
                                    sParseTree->userID,
                                    (sParseTree->tableName.stmtText +
                                     sParseTree->tableName.offset),
                                    (UInt)sParseTree->tableName.size,
                                    &sExist )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );

        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,
                                    sParseTree->tableName,
                                    QS_OBJECT_MAX,
                                    &(sParseTree->userID),
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );

        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,
                                    sViewNamePosition,
                                    QS_OBJECT_MAX,
                                    &(sParseTree->userID),
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );
    }
    else
    {
        /* 'ALTER VIEW ... COMPILE'에서 호출한 경우 */
    }

    /* check grant */
    IDE_TEST( qdpRole::checkDDLCreateMViewPriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );

    /* check refresh */
    switch ( sParseTree->mviewBuildRefresh.refreshType )
    {
        case QD_MVIEW_REFRESH_FORCE :
        case QD_MVIEW_REFRESH_COMPLETE :
            break;

        case QD_MVIEW_REFRESH_NONE :    /* Unreachable */
        case QD_MVIEW_REFRESH_FAST :
        case QD_MVIEW_REFRESH_NEVER :
        default :
            IDE_RAISE( ERR_NOT_SUPPORTED_REFRESH_OPTION );
    }

    switch ( sParseTree->mviewBuildRefresh.refreshTime )
    {
        case QD_MVIEW_REFRESH_ON_DEMAND :
            break;

        case QD_MVIEW_REFRESH_ON_NONE : /* Unreachable */
        case QD_MVIEW_REFRESH_ON_COMMIT :
        default :
            IDE_RAISE( ERR_NOT_SUPPORTED_REFRESH_OPTION );
    }

    /* View is valid. */
    sParseTree->flag &= ~QDV_OPT_STATUS_MASK;
    sParseTree->flag |= QDV_OPT_STATUS_VALID;

    /* validation of TABLESPACE */
    IDE_TEST( qdbCreate::validateTableSpace( aStatement ) != IDE_SUCCESS );

    /* validation of SELECT statement */
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        &= ~(QMV_VIEW_CREATION_MASK);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        |= (QMV_VIEW_CREATION_TRUE);

    // PROJ-2204 join update, delete
    // create view에 사용되는 SFWGH임을 표시한다.
    if ( ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->SFWGH != NULL )
    {
        ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->SFWGH->lflag
            &= ~QMV_SFWGH_UPDATABLE_VIEW_MASK;
        ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->SFWGH->lflag
            |= QMV_SFWGH_UPDATABLE_VIEW_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmv::validateSelect( sParseTree->select ) != IDE_SUCCESS );

    /* check SEQUENCE */
    if ( sParseTree->select->myPlan->parseTree->currValSeqs != NULL )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->select->myPlan->parseTree->
                                 currValSeqs->sequenceNode->position) );
        IDE_RAISE( ERR_USE_SEQUENCE_IN_VIEW );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->select->myPlan->parseTree->nextValSeqs != NULL )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->select->myPlan->parseTree->
                                 nextValSeqs->sequenceNode->position) );
        IDE_RAISE( ERR_USE_SEQUENCE_IN_VIEW );
    }
    else
    {
        /* Nothing to do */
    }

    /* validation of column and target
     * Column을 지정하지 않은 경우, validateTargetAndMakeColumnList()에서 Column List를 만든다.
     */
    IDE_TEST( qdbCreate::validateTargetAndMakeColumnList( aStatement )
              != IDE_SUCCESS );

    for ( sColumn = sParseTree->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        sColumnCount++;
    }

    /* 하위 View에서 사용할 columns를 복사한다. */
    IDE_TEST( qcm::copyQcmColumns( QC_QMP_MEM( aStatement ),
                    sParseTree->columns,
                    &(sParseTree->mviewViewColumns),
                    sColumnCount )
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::validateColumnListForCreate( aStatement,
                                                      sParseTree->columns,
                                                      ID_FALSE )
              != IDE_SUCCESS );

    /* PROJ-1362
     * validation of lob column attributes
     */
    IDE_TEST( qdbCommon::validateLobAttributeList(
                            aStatement,
                            NULL,
                            sParseTree->columns,
                            &(sParseTree->TBSAttr),
                            sParseTree->lobAttr )
              != IDE_SUCCESS );

    /* View를 Memory Table에 넣기 위해 Tablespace ID와 Type을 잠시 변경한다. */
    sTBSAttrID   = sParseTree->TBSAttr.mID;
    sTBSAttrType = sParseTree->TBSAttr.mType;
    sParseTree->TBSAttr.mID   = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
    sParseTree->TBSAttr.mType = SMI_MEMORY_SYSTEM_DICTIONARY;

    IDE_TEST( qdbCommon::validateColumnListForCreate( aStatement,
                                                      sParseTree->mviewViewColumns,
                                                      ID_FALSE )
              != IDE_SUCCESS );

    /* Tablespace ID와 Type을 복원한다. */
    sParseTree->TBSAttr.mID   = sTBSAttrID;
    sParseTree->TBSAttr.mType = sTBSAttrType;

    /* PROJ-1502 PARTITIONED DISK TABLE */
    if ( sParseTree->partTable != NULL )
    {
        IDE_TEST( qdbCommon::validatePartitionedTable( aStatement,
                                                       ID_TRUE ) /* aIsCreateTable */
                  != IDE_SUCCESS );

        if ( (sParseTree->partTable->partMethod == QCM_PARTITION_METHOD_RANGE) ||
             (sParseTree->partTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH) ||
             (sParseTree->partTable->partMethod == QCM_PARTITION_METHOD_LIST) )
        {
            for ( sPartAttr = sParseTree->partTable->partAttr, sPartCount = 0;
                  sPartAttr != NULL;
                  sPartAttr = sPartAttr->next, sPartCount++ )
            {
                IDU_LIMITPOINT( "qdm::validateCreate::malloc1" );
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qdAlterPartition,
                                        (void**)&sPartAttr->alterPart )
                          != IDE_SUCCESS );
                QD_SET_INIT_ALTER_PART( sPartAttr->alterPart );

                IDU_LIMITPOINT( "qdm::validateCreate::malloc2" );
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qmsPartCondValList,
                                        (void**)&sPartAttr->alterPart->partCondMinVal )
                          != IDE_SUCCESS );

                IDU_LIMITPOINT( "qdm::validateCreate::malloc3" );
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        qmsPartCondValList,
                                        (void**)&sPartAttr->alterPart->partCondMaxVal )
                          != IDE_SUCCESS );

                if ( sPartCount == 0 )
                {
                    sPartAttr->alterPart->partCondMinVal->partCondValCount = 0;
                    sPartAttr->alterPart->partCondMinVal->partCondValType =
                        QMS_PARTCONDVAL_NORMAL;
                    sPartAttr->alterPart->partCondMinVal->partCondValues[0] = 0;

                    sPartAttr->alterPart->partCondMaxVal
                        = &sPartAttr->partCondVal;
                }
                else
                {
                    idlOS::memcpy( sPartAttr->alterPart->partCondMinVal,
                                   &sOldPartCondVal,
                                   ID_SIZEOF(qmsPartCondValList) );

                    sPartAttr->alterPart->partCondMaxVal
                        = &sPartAttr->partCondVal;
                }

                sOldPartCondVal = *(sPartAttr->alterPart->partCondMaxVal);
            }
        }
        else
        {
            /* Nothing to do */
        }

        /* PROJ-2359 Table/Partition Access Option */
        for ( sPartAttr = sParseTree->partTable->partAttr;
              sPartAttr != NULL;
              sPartAttr = sPartAttr->next )
        {
            IDE_TEST_RAISE( sPartAttr->accessOption != QCM_ACCESS_OPTION_NONE,
                            ERR_ACCESS_NOT_SUPPORT_MVIEW );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2639 Altibase Disk Edition */
#ifdef ALTI_CFG_EDITION_DISK
    if ( QCG_GET_SESSION_USER_ID(aStatement) == QC_SYSTEM_USER_ID )
    {
        /* Nothing to do */
    }
    else
    {
        IDE_TEST_RAISE( smiTableSpace::isDiskTableSpaceType( sParseTree->TBSAttr.mType ) == ID_FALSE,
                        ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );

        if ( sParseTree->partTable != NULL )
        {
            qdbCommon::getTableTypeCountInPartAttrList( NULL,
                                                        sParseTree->partTable->partAttr,
                                                        NULL,
                                                        & sCountMemType,
                                                        & sCountVolType );

            IDE_TEST_RAISE( ( sCountMemType + sCountVolType ) > 0,
                            ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );
        }
        else
        {
            /* Nothing to do */
        }
    }
#endif

    /* Attribute List로부터 Table의 Flag 계산 */
    IDE_TEST( qdbCreate::calculateTableAttrFlag( aStatement, sParseTree )
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - PRJ-1671 Bitmap TableSpace And Segment Space Management
     *     Segment Storage 속성 validation
     */
    IDE_TEST( qdbCommon::validatePhysicalAttr( sParseTree )
              != IDE_SUCCESS );

    /* check circular view definition */
    for ( sRelatedObject = aStatement->spvEnv->relatedObjects;
          sRelatedObject != NULL;
          sRelatedObject = sRelatedObject->next )
    {
        /* (1) public synonym에 대한 circular view definition검사 */
        if ( (sRelatedObject->objectType == QS_SYNONYM) &&
             (sRelatedObject->userID == QC_PUBLIC_USER_ID) )
        {
            /* BUG-32964
             * 동일한 이름을 가진 Public Synonym이 존재하면,
             * circular view definition이 발생한다.
             */
            if ( idlOS::strMatch(
                        sParseTree->mviewViewName,
                        idlOS::strlen( sParseTree->mviewViewName ),
                        sRelatedObject->objectName.name,
                        sRelatedObject->objectName.size ) == 0 )
            {
                sqlInfo.setSourceInfo(
                        aStatement,
                        &(sRelatedObject->objectNamePos) );
                IDE_RAISE( ERR_CIRCULAR_VIEW_DEF );
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

        /* (2) view 또는 table에 대한 circular view definition검사 */
        if ( sRelatedObject->objectType == QS_TABLE )
        {
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          sRelatedObject->userName.name,
                                          sRelatedObject->userName.size,
                                          &sRelatedObjectUserID )
                      != IDE_SUCCESS );

            if ( sParseTree->userID == sRelatedObjectUserID )
            {
                if ( idlOS::strMatch(
                            sParseTree->mviewViewName,
                            idlOS::strlen( sParseTree->mviewViewName ),
                            sRelatedObject->objectName.name,
                            sRelatedObject->objectName.size ) == 0 )
                {
                    sqlInfo.setSourceInfo(
                            aStatement,
                            &(sRelatedObject->objectNamePos) );
                    IDE_RAISE( ERR_CIRCULAR_VIEW_DEF );
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

            /* BUG-8922
             * find v1 -> v2 -> v1
             */
            IDE_TEST( qcmView::checkCircularView(
                                aStatement,
                                sParseTree->userID,
                                sParseTree->mviewViewName,
                                sRelatedObject->tableID,
                                &sCircularViewExist )
                      != IDE_SUCCESS );

            if ( sCircularViewExist == ID_TRUE )
            {
                sqlInfo.setSourceInfo(
                        aStatement,
                        &(sRelatedObject->objectNamePos) );
                IDE_RAISE( ERR_CIRCULAR_VIEW_DEF );
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

    /* 'ALTER VIEW ... COMPILE'에서 호출한 경우, qdv::executeAlter()에서 필요한 정보를 설정한다. */
    if ( (sParseTree->flag & QDV_OPT_REPLACE_MASK) == QDV_OPT_REPLACE_TRUE )
    {
        sParseTree->flag &= ~QDV_MVIEW_TABLE_DDL_MASK;
        sParseTree->flag |= QDV_MVIEW_TABLE_DDL_FALSE;
        sParseTree->flag &= ~QDV_MVIEW_VIEW_DDL_MASK;
        sParseTree->flag |= QDV_MVIEW_VIEW_DDL_TRUE;
        SET_POSITION( sParseTree->tableName, sViewNamePosition );
        sParseTree->columns       = sParseTree->mviewViewColumns;
        sParseTree->maxrows       = 0;
        sParseTree->TBSAttr.mID   = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
        sParseTree->TBSAttr.mType = SMI_MEMORY_SYSTEM_DICTIONARY;
        QD_SEGMENT_OPTION_INIT( &(sParseTree->segAttr), &(sParseTree->segStoAttr) );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIST_OBJECT_NAME );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    IDE_EXCEPTION( ERR_RESERVED_WORD_IN_OBJECT_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_REFRESH_OPTION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDM_NOT_SUPPORTED_REFRESH_OPTION ) );
    }
    IDE_EXCEPTION( ERR_USE_SEQUENCE_IN_VIEW )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_USE_SEQUENCE_IN_VIEW,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_CIRCULAR_VIEW_DEF )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDV_CIRCULAR_VIEW,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_ACCESS_NOT_SUPPORT_MVIEW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDM_ACCESS_NOT_SUPPORT_MVIEW ) );
    }
#ifdef ALTI_CFG_EDITION_DISK
    IDE_EXCEPTION( ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USE_ONLY_DISK_TABLE_PARTITION_IN_DISK_EDITION ) );
    }
#endif
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    User ID, Materialized View Name으로 Materialized View 존재 여부를 확인한다.
 *
 * Implementation :
 *    1. SYS_MATERIALIZED_VIEWS_ 테이블의 USER_ID, MVIEW_NAME 컬럼을 구한다.
 *    2. SYS_MATERIALIZED_VIEWS_ 테이블에 인덱스가 있으면,
 *       명시된 User ID, Materialized View Name 으로 keyRange 를 만들어 찾는다.
 *       인덱스가 없으면,
 *       명시된 User ID, Materialized View Name 으로 하나씩 비교해서 찾는다.
 *    3. 찾은 Row가 있는지 확인한다.
 *
 ***********************************************************************/
IDE_RC qdm::existMViewByName(
                    smiStatement * aSmiStmt,
                    UInt           aUserID,
                    SChar        * aMViewName,
                    UInt           aMViewNameSize,
                    idBool       * aExist )
{
    volatile idBool      sIsCursorOpen;
    smiRange             sRange;
    smiTableCursor       sCursor;
    const void         * sRow             = NULL;
    mtcColumn          * sUserIDColumn    = NULL;
    mtcColumn          * sMViewNameColumn = NULL;
    UInt                 sUserID;
    mtdCharType        * sMViewName       = NULL;
    idBool               sIsFound         = ID_FALSE;
    qtcMetaRangeColumn   sFirstRangeColumn;
    qtcMetaRangeColumn   sSecondRangeColumn;

    qcNameCharBuffer     sMViewNameValueBuffer;
    mtdCharType        * sMViewNameValue = (mtdCharType *)&sMViewNameValueBuffer;

    scGRID               sRid; /* Disk Table을 위한 Record IDentifier */
    smiCursorProperties  sCursorProperty;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qdm::existMViewByName::__FT__" );

    sIsCursorOpen = ID_FALSE;

    sCursor.initialize();

    /* USER_ID */
    IDE_TEST( smiGetTableColumns( gQcmMaterializedViews,
                                  QCM_MATERIALIZED_VIEWS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sUserIDColumn )
              != IDE_SUCCESS );

    /* MVIEW_NAME */
    IDE_TEST( smiGetTableColumns( gQcmMaterializedViews,
                                  QCM_MATERIALIZED_VIEWS_MVIEW_NAME_COL_ORDER,
                                  (const smiColumn**)&sMViewNameColumn )
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

        IDU_FIT_POINT_FATAL( "qdm::existMViewByName::__FT__::CursorCreateDB" );

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

        IDU_FIT_POINT_FATAL( "qdm::existMViewByName::__FT__::CursorCreateMV" );

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    if ( sRow != NULL )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        *aExist = ID_FALSE;
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    CREATE MATERIALIZED VIEW ... 의 execution 수행
 *
 * Implementation :
 *    1. 새로운 Materialized View, Table, View의 ID를 구하기
 *    2. SYS_MATERIALIZED_VIEWS_에 Materialized View 정보 입력
 *    3. createTableOnSM (Table)
 *    4. SYS_TABLES_, SYS_COLUMNS_에 Table 정보 입력
 *    5. Table Meta Cache 구성 (Table)
 *    6. createTableOnSM (View)
 *    7. SYS_TABLES_, SYS_COLUMNS_,
 *       SYS_VIEWS_, SYS_VIEW_PARSE_, SYS_VIEW_RELATED_에 View 정보 입력
 *    8. Table Meta Cache 구성 (View)
 *    9. Build Type이 IMMEDIATE이면, 데이터를 구성
 *
 ***********************************************************************/
IDE_RC qdm::executeCreate( qcStatement * aStatement )
{
    qdTableParseTree     * sParseTree         = NULL;
    smiValue             * sInsertedRow       = NULL;
    smiValue             * sSmiValues         = NULL;
    smiValue               sInsertedRowForPartition[QC_MAX_COLUMN_COUNT];
    smOID                  sTableOID;
    UInt                   sTableID;
    qmcRowFlag             sRowFlag;
    smSCN                  sSCN;
    smSCN                  sViewSCN           = SM_SCN_INIT;
    qcmTableInfo         * sTempTableInfo     = NULL;
    qcmTableInfo         * sTempViewInfo      = NULL;
    smiTableCursor         sCursor;
    smiTableCursor       * sPartitionCursor   = NULL;
    qcmColumn            * sColumn            = NULL;
    idBool                 sIsCursorOpen      = ID_FALSE;
    UInt                   sColumnCount       = 0;
    UInt                   sLobColumnCount    = 0;
    iduMemoryStatus        sQmxMemStatus;
    smiCursorProperties    sCursorProperty;
    void                 * sTableHandle       = NULL;
    void                 * sViewHandle        = NULL;
    qmxLobInfo           * sLobInfo           = NULL;
    void                 * sRow               = NULL;
    scGRID                 sRowGRID;
    idBool                 sIsPartitioned     = ID_FALSE;
    qcmColumn            * sPartKeyColumns    = NULL;
    UInt                   sPartKeyCount      = 0;
    qcmPartitionInfoList * sPartInfoList      = NULL;
    qcmPartitionInfoList * sOldPartInfoList   = NULL;
    qcmPartitionInfoList * sTempPartInfoList  = NULL;
    qcmTableInfo         * sPartInfo          = NULL;
    UInt                   sPartCount         = 0;

    qmsTableRef          * sTableRef          = NULL;
    qmsPartitionRef      * sPartitionRef      = NULL;
    qmsPartitionRef      * sFirstPartitionRef = NULL;
    qmcInsertCursor        sInsertCursorMgr;
    qdPartitionAttribute * sPartAttr          = NULL;
    qdPartitionAttribute * sTempPartAttr      = NULL;

    UInt                   sMViewID;
    UInt                   sViewID;
    smOID                  sViewOID;
    qcNamePosition         sViewNamePosition;
    qcNamePosition         sTableNamePosition;
    smiSegAttr             sSegAttr;
    smiSegStorageAttr      sSegStorageAttr;

    qcmTableInfo         * sSelectedPartitionInfo = NULL;

    sRowFlag = QMC_ROW_INITIALIZE;

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sCursorProperty, aStatement->mStatistics );
    sCursorProperty.mIsUndoLogging = ID_FALSE;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sViewNamePosition.stmtText = sParseTree->mviewViewName;
    sViewNamePosition.offset   = 0;
    sViewNamePosition.size     = idlOS::strlen( sParseTree->mviewViewName );

    for ( sColumn = sParseTree->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        sColumnCount++;

        /* PROJ-1362 */
        if ( (sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK)
             == MTD_COLUMN_TYPE_LOB )
        {
            sLobColumnCount++;
        }
        else
        {
            /* Nothing to do */
        }

        // TODO : Source의 LOB Column 수를 정확하게 지정하면, 메모리 사용량을 줄일 수 있다.
        sLobColumnCount++;
    }

    /* PROJ-1502 PARTITIONED DISK TABLE
     * 논파티션드 테이블 생성
     */
    if ( sParseTree->partTable == NULL )
    {
        sIsPartitioned = ID_FALSE;
    }
    /* 파티션드 테이블 생성 */
    else
    {
        sIsPartitioned = ID_TRUE;

        sPartKeyCount = 0;

        for ( sPartKeyColumns = sParseTree->partTable->partKeyColumns;
              sPartKeyColumns != NULL;
              sPartKeyColumns = sPartKeyColumns->next )
        {
            sPartKeyCount++;
        }
    }

    /* PROJ-1362 */
    IDE_TEST( qmx::initializeLobInfo( aStatement,
                                      &sLobInfo,
                                      (UShort)sLobColumnCount )
              != IDE_SUCCESS );

    /* BUG-36211 각 Row Insert 후, 해당 Lob Cursor를 바로 해제합니다. */
    qmx::setImmediateCloseLobInfo( sLobInfo, ID_TRUE );

    /* get Materialized View, Table ID */
    IDE_TEST( getNextMViewID( aStatement, &sMViewID ) != IDE_SUCCESS );
    IDE_TEST( qcm::getNextTableID( aStatement, &sTableID ) != IDE_SUCCESS );
    IDE_TEST( qcm::getNextTableID( aStatement, &sViewID ) != IDE_SUCCESS );

    /* insert into Materialized View META tables */
    IDE_TEST( insertMViewSpecIntoMeta( aStatement,
                                       sMViewID,
                                       sTableID,
                                       sViewID )
              != IDE_SUCCESS );

    /* Table 작업을 하기 전에 Parse Tree를 설정한다. */
    sParseTree->flag &= ~QDV_MVIEW_TABLE_DDL_MASK;
    sParseTree->flag |= QDV_MVIEW_TABLE_DDL_TRUE;
    sParseTree->flag &= ~QDV_MVIEW_VIEW_DDL_MASK;
    sParseTree->flag |= QDV_MVIEW_VIEW_DDL_FALSE;

    IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                          sParseTree->columns,
                                          sParseTree->userID,
                                          sTableID,
                                          sParseTree->maxrows,
                                          sParseTree->TBSAttr.mID,
                                          sParseTree->segAttr,
                                          sParseTree->segStoAttr,
                                          sParseTree->tableAttrMask,
                                          sParseTree->tableAttrValue,
                                          sParseTree->parallelDegree,
                                          &sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::insertTableSpecIntoMeta( aStatement,
                                                  sIsPartitioned,
                                                  sParseTree->flag,
                                                  sParseTree->tableName,
                                                  sParseTree->userID,
                                                  sTableOID,
                                                  sTableID,
                                                  sColumnCount,
                                                  sParseTree->maxrows,
                                                  /* PROJ-2359 Table/Partition Access Option */
                                                  QCM_ACCESS_OPTION_READ_WRITE,
                                                  sParseTree->TBSAttr.mID,
                                                  sParseTree->segAttr,
                                                  sParseTree->segStoAttr,
                                                  QCM_TEMPORARY_ON_COMMIT_NONE,
                                                  sParseTree->parallelDegree,   // PROJ-1071
                                                  sParseTree->mShardFlag )      // TASK-7307
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::insertColumnSpecIntoMeta( aStatement,
                                                   sParseTree->userID,
                                                   sTableID,
                                                   sParseTree->columns,
                                                   ID_FALSE /* is queue */ )
              != IDE_SUCCESS );

    /* PROJ-1502 PARTITIONED DISK TABLE */
    if ( sIsPartitioned == ID_TRUE )
    {
        IDE_TEST( qdbCommon::insertPartTableSpecIntoMeta( aStatement,
                                                          sParseTree->userID,
                                                          sTableID,
                                                          sParseTree->partTable->partMethod,
                                                          sPartKeyCount,
                                                          sParseTree->isRowmovement )
                  != IDE_SUCCESS );

        IDE_TEST( qdbCommon::insertPartKeyColumnSpecIntoMeta( aStatement,
                                                              sParseTree->userID,
                                                              sTableID,
                                                              sParseTree->columns,
                                                              sParseTree->partTable->partKeyColumns,
                                                              QCM_TABLE_OBJECT_TYPE )
                  != IDE_SUCCESS );

        IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                               sTableID,
                                               sTableOID )
                  != IDE_SUCCESS );

        IDE_TEST( qcm::getTableInfoByID( aStatement,
                                         sTableID,
                                         &sTempTableInfo,
                                         &sSCN,
                                         &sTableHandle )
                  != IDE_SUCCESS );

        IDE_TEST( qdbCreate::executeCreateTablePartition( aStatement,
                                                          sTableID,
                                                          sTempTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      aStatement->qmxMem,
                                                      sTableID,
                                                      &sTempPartInfoList )
                  != IDE_SUCCESS );

        /* 모든 파티션에 LOCK(X) */
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sTempPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sParseTree->partTable->partInfoList = sTempPartInfoList;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sTempTableInfo != NULL )
    {
        (void)qcm::destroyQcmTableInfo( sTempTableInfo );
    }
    else
    {
        /* Nothing to do */
    }
    sTempTableInfo = NULL;

    /* Table을 생성했으므로, Lock을 획득한다. */
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTempTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    (void)qcm::destroyQcmTableInfo( sTempTableInfo );
    sTempTableInfo = NULL;

    /* Table 정보를 다시 얻는다. */
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTempTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    /* PROJ-1502 PARTITIONED DISK TABLE */
    if ( sIsPartitioned == ID_TRUE )
    {
        sOldPartInfoList = sTempPartInfoList;
        IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                      sTempTableInfo,
                                                                      sOldPartInfoList,
                                                                      & sTempPartInfoList )
                  != IDE_SUCCESS );

        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }
    else
    {
        /* Nothing to do */
    }

    /* View 작업을 하기 전에 Parse Tree를 설정한다. */
    sParseTree->flag &= ~QDV_MVIEW_TABLE_DDL_MASK;
    sParseTree->flag |= QDV_MVIEW_TABLE_DDL_FALSE;
    sParseTree->flag &= ~QDV_MVIEW_VIEW_DDL_MASK;
    sParseTree->flag |= QDV_MVIEW_VIEW_DDL_TRUE;
    SET_POSITION( sTableNamePosition, sParseTree->tableName );
    SET_POSITION( sParseTree->tableName, sViewNamePosition );
    QD_SEGMENT_OPTION_INIT( &sSegAttr, &sSegStorageAttr );

    /* View 객체를 생성한다. */
    IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                          sParseTree->mviewViewColumns,
                                          sParseTree->userID,
                                          sViewID,
                                          0, /* aMaxRows */
                                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                          sSegAttr,
                                          sSegStorageAttr,
                                          0, /* Table Flag Mask */ 
                                          0, /* Table Flag Value */
                                          1, /* Parallel Degree */
                                          &sViewOID,
                                          aStatement->myPlan->stmtText,
                                          aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    IDE_TEST( qdv::insertViewSpecIntoMeta( aStatement,
                                           sViewID,
                                           sViewOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sViewID,
                                           sViewOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sViewID,
                                     & sTempViewInfo,
                                     & sViewSCN,
                                     & sViewHandle )
              != IDE_SUCCESS );

    /* BUG-11266 */
    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated(
                        aStatement,
                        sParseTree->userID,
                        sParseTree->mviewViewName,
                        idlOS::strlen( sParseTree->mviewViewName ),
                        QS_TABLE )
              != IDE_SUCCESS );

    /* View 작업을 끝낸 후에 Parse Tree를 복원한다. */
    sParseTree->flag &= ~QDV_MVIEW_TABLE_DDL_MASK;
    sParseTree->flag |= QDV_MVIEW_TABLE_DDL_FALSE;
    sParseTree->flag &= ~QDV_MVIEW_VIEW_DDL_MASK;
    sParseTree->flag |= QDV_MVIEW_TABLE_DDL_FALSE;
    SET_POSITION( sParseTree->tableName, sTableNamePosition );

    /* BUG-16535
     * set SYSDATE
     */
    IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aStatement ) ) != IDE_SUCCESS );

    /* Build Type이 IMMEDIATE이면, 데이터를 구성한다. */
    if ( sParseTree->mviewBuildRefresh.buildType == QD_MVIEW_BUILD_IMMEDIATE )
    {
        IDU_LIMITPOINT( "qdm::executeCreate::malloc1");
        IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(smiValue) * sColumnCount,
                                             (void**)&sInsertedRow )
                  != IDE_SUCCESS);

        IDE_TEST( qmnPROJ::init( QC_PRIVATE_TMPLATE( aStatement ),
                                 sParseTree->select->myPlan->plan )
                  != IDE_SUCCESS );

        /* PROJ-1502 PARTITIONED DISK TABLE */
        if ( sIsPartitioned == ID_FALSE )
        {
            sCursor.initialize();

            IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                                    sTempTableInfo->tableHandle,
                                    NULL,
                                    smiGetRowSCN( sTempTableInfo->tableHandle ),
                                    NULL,
                                    smiGetDefaultKeyRange(),
                                    smiGetDefaultKeyRange(),
                                    smiGetDefaultFilter(),
                                    SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|
                                    SMI_PREVIOUS_DISABLE,
                                    SMI_INSERT_CURSOR,
                                    &sCursorProperty )
                      != IDE_SUCCESS );
            sIsCursorOpen = ID_TRUE;

            IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE( aStatement ),
                                     sParseTree->select->myPlan->plan,
                                     &sRowFlag )
                      != IDE_SUCCESS );

            /* To fix BUG-13978
             * bitwise operation(&) for flag check
             */
            while ( (sRowFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( QC_QMX_MEM( aStatement )->getStatus( &sQmxMemStatus )
                          != IDE_SUCCESS );

                (void)qmx::clearLobInfo( sLobInfo );

                IDE_TEST( qmx::makeSmiValueWithResult( sParseTree->columns,
                                                       QC_PRIVATE_TMPLATE( aStatement ),
                                                       sTempTableInfo,
                                                       sInsertedRow,
                                                       sLobInfo )
                          != IDE_SUCCESS );

                IDE_TEST( sCursor.insertRow( sInsertedRow,
                                             &sRow,
                                             &sRowGRID )
                          != IDE_SUCCESS );

                IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                      sLobInfo,
                                                      &sCursor,
                                                      sRow,
                                                      sRowGRID )
                          != IDE_SUCCESS );

                IDE_TEST( QC_QMX_MEM( aStatement )->setStatus( &sQmxMemStatus )
                          != IDE_SUCCESS );

                IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE( aStatement ),
                                         sParseTree->select->myPlan->plan,
                                         &sRowFlag )
                          != IDE_SUCCESS );
            } /* while */

            sIsCursorOpen = ID_FALSE;
            IDE_TEST( sCursor.close() != IDE_SUCCESS );
        }
        else
        {
            IDU_LIMITPOINT( "qdm::executeCreate::malloc2" );
            IDE_TEST( STRUCT_ALLOC( aStatement->qmxMem,
                                    qmsTableRef,
                                    (void**)&sTableRef )
                      != IDE_SUCCESS );
            QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

            sTableRef->userName = sParseTree->userName;
            sTableRef->tableName = sParseTree->tableName;
            sTableRef->userID = sParseTree->userID;
            sTableRef->tableInfo = sTempTableInfo;

            /* partitionRef 구성 */
            for ( sPartInfoList = sTempPartInfoList, sPartCount = 0,
                      sPartAttr = sParseTree->partTable->partAttr;
                  sPartInfoList != NULL;
                  sPartInfoList = sPartInfoList->next,
                      sPartAttr = sPartAttr->next, sPartCount++ )
            {
                sPartInfo = sPartInfoList->partitionInfo;

                IDU_LIMITPOINT( "qdm::executeCreate::malloc3" );
                IDE_TEST( STRUCT_ALLOC( aStatement->qmxMem,
                                        qmsPartitionRef,
                                        (void**)&sPartitionRef )
                          != IDE_SUCCESS );
                QCP_SET_INIT_QMS_PARTITION_REF( sPartitionRef );

                sPartitionRef->partitionID = sPartInfo->partitionID;
                sPartitionRef->partitionInfo = sPartInfo;
                sPartitionRef->partitionSCN = sPartInfoList->partSCN;
                sPartitionRef->partitionHandle = sPartInfoList->partHandle;

                if ( sParseTree->partTable->partMethod != QCM_PARTITION_METHOD_HASH )
                {
                    for ( sTempPartAttr = sParseTree->partTable->partAttr;
                          sTempPartAttr != NULL;
                          sTempPartAttr = sTempPartAttr->next )
                    {
                        if ( idlOS::strMatch( (sTempPartAttr->tablePartName.stmtText +
                                               sTempPartAttr->tablePartName.offset),
                                              sTempPartAttr->tablePartName.size,
                                              sPartInfo->name,
                                              idlOS::strlen( sPartInfo->name ) ) == 0 )
                        {
                            sPartitionRef->minPartCondVal =
                                *( sTempPartAttr->alterPart->partCondMinVal );
                            sPartitionRef->maxPartCondVal =
                                *( sTempPartAttr->alterPart->partCondMaxVal );
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }

                    /* minValue의 partCondValType 조정 */
                    if ( sPartitionRef->minPartCondVal.partCondValCount == 0 )
                    {
                        sPartitionRef->minPartCondVal.partCondValType
                            = QMS_PARTCONDVAL_MIN;
                    }
                    else
                    {
                        sPartitionRef->minPartCondVal.partCondValType
                            = QMS_PARTCONDVAL_NORMAL;
                    }

                    /* maxValue의 partCondValType 조정 */
                    if ( sPartitionRef->maxPartCondVal.partCondValCount == 0 )
                    {
                        sPartitionRef->maxPartCondVal.partCondValType
                            = QMS_PARTCONDVAL_DEFAULT;
                    }
                    else
                    {
                        sPartitionRef->maxPartCondVal.partCondValType
                            = QMS_PARTCONDVAL_NORMAL;
                    }
                }
                else
                {
                    sPartitionRef->partOrder = sPartAttr->partOrder;
                }

                if ( sFirstPartitionRef == NULL )
                {
                    sPartitionRef->next = NULL;
                    sFirstPartitionRef = sPartitionRef;
                }
                else
                {
                    sPartitionRef->next = sFirstPartitionRef;
                    sFirstPartitionRef = sPartitionRef;
                }
            } /* for */

            sTableRef->partitionRef = sFirstPartitionRef;
            sTableRef->partitionCount = sPartCount;

            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sTableRef )
                      != IDE_SUCCESS );

            IDE_TEST( sInsertCursorMgr.initialize( aStatement->qmxMem,
                                                   sTableRef,
                                                   ID_TRUE,
                                                   ID_TRUE ) /* init all partitions */
                      != IDE_SUCCESS );

            IDE_TEST( sInsertCursorMgr.openCursor( aStatement,
                                                   SMI_LOCK_WRITE|
                                                   SMI_TRAVERSE_FORWARD|
                                                   SMI_PREVIOUS_DISABLE,
                                                   &sCursorProperty )
                      != IDE_SUCCESS );
            sIsCursorOpen = ID_TRUE;

            IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE( aStatement ),
                                     sParseTree->select->myPlan->plan,
                                     &sRowFlag )
                      != IDE_SUCCESS );

            /* To fix BUG-13978
             * bitwise operation(&) for flag check
             */
            while ( (sRowFlag & QMC_ROW_DATA_MASK) ==  QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( QC_QMX_MEM( aStatement )->getStatus( &sQmxMemStatus )
                          != IDE_SUCCESS );

                (void)qmx::clearLobInfo( sLobInfo );

                IDE_TEST( qmx::makeSmiValueWithResult( sParseTree->columns,
                                                       QC_PRIVATE_TMPLATE( aStatement ),
                                                       sTempTableInfo,
                                                       sInsertedRow,
                                                       sLobInfo )
                          != IDE_SUCCESS );

                /* INSERT를 수행 */
                IDE_TEST( sInsertCursorMgr.partitionFilteringWithRow(
                              sInsertedRow,
                              sLobInfo,
                              &sSelectedPartitionInfo )
                          != IDE_SUCCESS );

                IDE_TEST( sInsertCursorMgr.getCursor( &sPartitionCursor )
                          != IDE_SUCCESS );

                if ( QCM_TABLE_TYPE_IS_DISK( sTempTableInfo->tableFlag ) !=
                     QCM_TABLE_TYPE_IS_DISK( sSelectedPartitionInfo->tableFlag ) )
                {
                    /* PROJ-2464 hybrid partitioned table 지원
                     * Partitioned Table을 기준으로 만든 smiValue Array를 Table Partition에 맞게 변환한다.
                     */
                    IDE_TEST( qmx::makeSmiValueWithSmiValue( sTempTableInfo,
                                                             sSelectedPartitionInfo,
                                                             sTempTableInfo->columns,
                                                             sColumnCount,
                                                             sInsertedRow,
                                                             sInsertedRowForPartition )
                              != IDE_SUCCESS );

                    sSmiValues = sInsertedRowForPartition;
                }
                else
                {
                    sSmiValues = sInsertedRow;
                }

                IDE_TEST( sPartitionCursor->insertRow( sSmiValues,
                                                       &sRow,
                                                       &sRowGRID )
                          != IDE_SUCCESS );

                /* INSERT를 수행후 Lob 컬럼을 처리 */
                IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                      sLobInfo,
                                                      sPartitionCursor,
                                                      sRow,
                                                      sRowGRID )
                          != IDE_SUCCESS );

                IDE_TEST( QC_QMX_MEM( aStatement )->setStatus( &sQmxMemStatus )
                          != IDE_SUCCESS );

                IDE_TEST( qmnPROJ::doIt( QC_PRIVATE_TMPLATE( aStatement ),
                                         sParseTree->select->myPlan->plan,
                                         &sRowFlag )
                          != IDE_SUCCESS );
            } /* while */

            sIsCursorOpen = ID_FALSE;
            IDE_TEST( sInsertCursorMgr.closeCursor() != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
     * DDL Statement Text의 로깅
     */
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sTempTableInfo->name )
                  != IDE_SUCCESS );

        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                                                                aStatement,
                                                                0,
                                                                sTableOID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        if ( sIsPartitioned == ID_FALSE )
        {
            sCursor.close();
        }
        else
        {
            sInsertCursorMgr.closeCursor();
        }
    }

    (void)qmx::finalizeLobInfo( aStatement, sLobInfo );

    (void)qcm::destroyQcmTableInfo( sTempTableInfo );
    (void)qcm::destroyQcmTableInfo( sTempViewInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sTempPartInfoList );

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Materialized View의 메타 정보를 메타 테이블에 입력한다.
 *
 * Implementation :
 *    1. SYS_MATERIALIZED_VIEWS_ 입력
 *
 ***********************************************************************/
IDE_RC qdm::insertMViewSpecIntoMeta(
                    qcStatement * aStatement,
                    UInt          aMViewID,
                    UInt          aTableID,
                    UInt          aViewID )
{
    qdTableParseTree * sParseTree;
    SChar              sMViewName[QC_MAX_OBJECT_NAME_LEN + 1];

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    QC_STR_COPY( sMViewName, sParseTree->tableName );

    /* insert SYS_MATERIALIZED_VIEWS_ */
    IDE_TEST( insertIntoMViewsMeta( aStatement,
                                    sParseTree->userID,
                                    aMViewID,
                                    sMViewName,
                                    aTableID,
                                    aViewID,
                                    sParseTree->mviewBuildRefresh.refreshType,
                                    sParseTree->mviewBuildRefresh.refreshTime )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    SYS_MATERIALIZED_VIEWS_에 Materialized View 정보를 추가한다.
 *
 * Implementation :
 *    1. SYS_MATERIALIZED_VIEWS_에 Row를 추가한다.
 *
 ***********************************************************************/
IDE_RC qdm::insertIntoMViewsMeta(
                    qcStatement        * aStatement,
                    UInt                 aUserID,
                    UInt                 aMViewID,
                    SChar              * aMViewName,
                    UInt                 aTableID,
                    UInt                 aViewID,
                    qdMViewRefreshType   aRefreshType,
                    qdMViewRefreshTime   aRefreshTime )
{
    SChar  * sSqlStr;
    const SChar  * sRefreshTypeStr[] = { "", "R", "C", "F", "" };
    const SChar  * sRefreshTimeStr[] = { "", "D", "C" };
    vSLong   sRowCnt;

    IDU_LIMITPOINT( "qdm::insertIntoMViewsMeta::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_MATERIALIZED_VIEWS_ VALUES( "
                     QCM_SQL_INT32_FMT", "
                     QCM_SQL_INT32_FMT", "
                     QCM_SQL_CHAR_FMT", "
                     QCM_SQL_INT32_FMT", "
                     QCM_SQL_INT32_FMT", "
                     QCM_SQL_CHAR_FMT", "
                     QCM_SQL_CHAR_FMT", "
                     "SYSDATE, SYSDATE, NULL )",
                     aUserID,
                     aMViewID,
                     aMViewName,
                     aTableID,
                     aViewID,
                     sRefreshTypeStr[aRefreshType],
                     sRefreshTimeStr[aRefreshTime] );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt ) != IDE_SUCCESS );

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
 *    ALTER MATERIALIZED VIEW ... REFRESH ... 의 validation 수행
 *
 * Implementation :
 *    1. Meta Table인지 확인
 *    2. Materialized View 변경 권한이 있는지 확인
 *    3. 지원하는 Refresh인지 확인
 *
 ***********************************************************************/
IDE_RC qdm::validateAlterRefresh( qcStatement * aStatement )
{
    qdTableParseTree * sParseTree = NULL;
    qcmTableInfo     * sTableInfo = NULL;
    qcuSqlSourceInfo   sqlInfo;
    UInt               sMViewID;
    
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->tableName) );
        IDE_RAISE( ERR_RESERVED_WORD_IN_OBJECT_NAME );
    }
    else
    {
        /* Nothing to do */
    }

    /* check user existence */
    if ( QC_IS_NULL_NAME( sParseTree->userName ) == ID_TRUE )
    {
        sParseTree->userID = QCG_GET_SESSION_USER_ID( aStatement );
    }
    else
    {
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      sParseTree->userName,
                                      &(sParseTree->userID) )
                  != IDE_SUCCESS );
    }

    /* check materialized view existence */
    IDE_TEST( qcmMView::getMViewID(
                        QC_SMI_STMT( aStatement ),
                        sParseTree->userID,
                        (sParseTree->tableName.stmtText +
                         sParseTree->tableName.offset),
                        (UInt)sParseTree->tableName.size,
                        &sMViewID )
              != IDE_SUCCESS );

    /* check the table exists */
    IDE_TEST( qcm::getTableHandleByName(
                    QC_SMI_STMT( aStatement ),
                    sParseTree->userID,
                    (UChar *)(sParseTree->tableName.stmtText +
                              sParseTree->tableName.offset),
                    sParseTree->tableName.size,
                    &(sParseTree->tableHandle),
                    &(sParseTree->tableSCN) )
              != IDE_SUCCESS );

    IDE_TEST( qcm::lockTableForDDLValidation(
                    aStatement,
                    sParseTree->tableHandle,
                    sParseTree->tableSCN )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sParseTree->tableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );
    sParseTree->tableInfo = sTableInfo;

    if ( QCG_GET_SESSION_USER_ID( aStatement ) != QC_SYSTEM_USER_ID )
    {
        /* sParseTree->userID is a owner of materialized view */
        IDE_TEST_RAISE( sParseTree->userID == QC_SYSTEM_USER_ID,
                        ERR_NOT_ALTER_META );
    }
    else
    {
        IDE_TEST_RAISE( (aStatement->session->mQPSpecific.mFlag &
                           QC_SESSION_ALTER_META_MASK)
                        == QC_SESSION_ALTER_META_DISABLE,
                        ERR_NOT_ALTER_META );
    }

    /* check grant */
    IDE_TEST( qdpRole::checkDDLAlterMViewPriv( aStatement,
                                               sTableInfo )
              != IDE_SUCCESS );

    /* check refresh */
    switch ( sParseTree->mviewBuildRefresh.refreshType )
    {
        case QD_MVIEW_REFRESH_NONE :    /* 지정하지 않은 경우 */
        case QD_MVIEW_REFRESH_FORCE :
        case QD_MVIEW_REFRESH_COMPLETE :
            break;

        case QD_MVIEW_REFRESH_FAST :
        case QD_MVIEW_REFRESH_NEVER :
        default :
            IDE_RAISE( ERR_NOT_SUPPORTED_REFRESH_OPTION );
    }

    switch ( sParseTree->mviewBuildRefresh.refreshTime )
    {
        case QD_MVIEW_REFRESH_ON_NONE : /* 지정하지 않은 경우 */
        case QD_MVIEW_REFRESH_ON_DEMAND :
            break;

        case QD_MVIEW_REFRESH_ON_COMMIT :
        default :
            IDE_RAISE( ERR_NOT_SUPPORTED_REFRESH_OPTION );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALTER_META );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDD_NO_DROP_META_TABLE ) );
    }
    IDE_EXCEPTION( ERR_RESERVED_WORD_IN_OBJECT_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_REFRESH_OPTION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDM_NOT_SUPPORTED_REFRESH_OPTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    ALTER MATERIALIZED VIEW ... REFRESH ... 의 execution 수행
 *
 * Implementation :
 *    1. Meta Table에서 Refresh Type 변경
 *    2. Meta Table에서 Refresh Time 변경
 *    3. Meta Table에서 Last DDL Time 변경
 *
 ***********************************************************************/
IDE_RC qdm::executeAlterRefresh( qcStatement * aStatement )
{
    qdTableParseTree * sParseTree   = NULL;
    UInt               sMViewID;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( qcmMView::getMViewID(
                        QC_SMI_STMT( aStatement ),
                        sParseTree->userID,
                        (sParseTree->tableName.stmtText +
                         sParseTree->tableName.offset),
                        (UInt)sParseTree->tableName.size,
                        &sMViewID )
              != IDE_SUCCESS );

    if ( sParseTree->mviewBuildRefresh.refreshType != QD_MVIEW_REFRESH_NONE )
    {
        IDE_TEST( updateRefreshTypeFromMeta(
                        aStatement,
                        sMViewID,
                        sParseTree->mviewBuildRefresh.refreshType )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->mviewBuildRefresh.refreshTime != QD_MVIEW_REFRESH_ON_NONE )
    {
        IDE_TEST( updateRefreshTimeFromMeta(
                        aStatement,
                        sMViewID,
                        sParseTree->mviewBuildRefresh.refreshTime )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( updateLastDDLTimeFromMeta( aStatement, sMViewID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Meta Table에서 Materialized View의 Refresh Type을 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qdm::updateRefreshTypeFromMeta(
                    qcStatement        * aStatement,
                    UInt                 aMViewID,
                    qdMViewRefreshType   aRefreshType )
{
    const SChar  * sRefreshTypeStr[] = { "", "R", "C", "F", "" };
    SChar  * sSqlStr;
    vSLong   sRowCnt;

    IDU_LIMITPOINT( "qdm::updateRefreshTypeFromMeta::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                            SChar,
                                            QD_MAX_SQL_LENGTH,
                                            &sSqlStr )
                    != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_MATERIALIZED_VIEWS_ "
                     "SET REFRESH_TYPE = "QCM_SQL_CHAR_FMT
                     "WHERE MVIEW_ID = "QCM_SQL_INT32_FMT,
                     sRefreshTypeStr[aRefreshType],
                     aMViewID );
    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Meta Table에서 Materialized View의 Refresh Time을 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qdm::updateRefreshTimeFromMeta(
                    qcStatement        * aStatement,
                    UInt                 aMViewID,
                    qdMViewRefreshTime   aRefreshTime )
{
    const SChar  * sRefreshTimeStr[] = { "", "D", "C" };
    SChar  * sSqlStr;
    vSLong   sRowCnt;

    IDU_LIMITPOINT( "qdm::updateRefreshTimeFromMeta::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_MATERIALIZED_VIEWS_ "
                     "SET REFRESH_TIME = "QCM_SQL_CHAR_FMT
                     "WHERE MVIEW_ID = "QCM_SQL_INT32_FMT,
                     sRefreshTimeStr[aRefreshTime],
                     aMViewID );
    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Meta Table에서 Materialized View의 Last DDL Time을 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qdm::updateLastDDLTimeFromMeta(
                    qcStatement        * aStatement,
                    UInt                 aMViewID )
{
    SChar  * sSqlStr;
    vSLong   sRowCnt;

    IDU_LIMITPOINT( "qdm::updateLastDDLTimeFromMeta::malloc");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_MATERIALIZED_VIEWS_ "
                     "SET LAST_DDL_TIME = SYSDATE "
                     "WHERE MVIEW_ID = "QCM_SQL_INT32_FMT,
                     aMViewID );
    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    새로운 Materialized View ID를 생성한다.
 *
 * Implementation :
 *    1. Sequence에서 새로운 ID 얻기
 *    2. SYS_MATERIALIZED_VIEWS_에서 이미 사용 중인지 검사
 *    3. 사용 중이면, 1~2 과정을 다시 수행
 *
 ***********************************************************************/
IDE_RC qdm::getNextMViewID( qcStatement * aStatement,
                            UInt        * aMViewID )
{
    void   * sSequenceHandle = NULL;
    idBool   sExist;
    idBool   sFirst;
    SLong    sSeqVal;
    SLong    sSeqValFirst;

    sExist = ID_TRUE;
    sFirst = ID_TRUE;

    IDE_TEST( qcm::getSequenceHandleByName(
                QC_SMI_STMT( aStatement ),
                QC_SYSTEM_USER_ID,
                (UChar*)gDBSequenceName[QCM_DB_SEQUENCE_MVIEWID],
                idlOS::strlen( gDBSequenceName[QCM_DB_SEQUENCE_MVIEWID] ),
                &sSequenceHandle )
              != IDE_SUCCESS );

    while ( sExist == ID_TRUE )
    {
        IDE_TEST( smiTable::readSequence( QC_SMI_STMT( aStatement ),
                        sSequenceHandle,
                        SMI_SEQUENCE_NEXT,
                        &sSeqVal,
                        NULL )
                  != IDE_SUCCESS );

        /* sSeqVal은 비록 SLong이지만, sequence를 생성할 때
         * max를 integer max를 안넘도록 하였기 때문에
         * 여기서 overflow체크는 하지 않는다.
         */
        IDE_TEST( searchMViewID( QC_SMI_STMT( aStatement ),
                                 (SInt)sSeqVal,
                                 &sExist )
                  != IDE_SUCCESS );

        if ( sFirst == ID_TRUE )
        {
            sSeqValFirst = sSeqVal;
            sFirst = ID_FALSE;
        }
        else
        {
            /* 찾다찾다 한바퀴 돈 경우, object가 꽉 찬 것을 의미 */
            IDE_TEST_RAISE( sSeqVal == sSeqValFirst, ERR_OBJECTS_OVERFLOW );
        }
    }

    *aMViewID = (UInt)sSeqVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OBJECTS_OVERFLOW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                  "materialized view",
                                  QCM_META_SEQ_MAXVALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Materialized View ID가 존재하는지 확인한다.
 *
 * Implementation :
 *    1. SYS_MATERIALIZED_VIEWS_에서 이미 사용 중인지 검사
 *
 ***********************************************************************/
IDE_RC qdm::searchMViewID( smiStatement * aSmiStmt,
                           SInt           aMViewID,
                           idBool       * aExist )
{
    idBool               sIsCursorOpen  = ID_FALSE;
    smiRange             sRange;
    smiTableCursor       sCursor;
    const void         * sRow           = NULL;
    UInt                 sFlag          = QCM_META_CURSOR_FLAG;
    mtcColumn          * sMViewIDColumn = NULL;
    qtcMetaRangeColumn   sRangeColumn;

    scGRID               sRid; /* Disk Table을 위한 Record IDentifier */
    smiCursorProperties  sCursorProperty;

    if ( gQcmMaterializedViewsIndex[QCM_MATERIALIZED_VIEWS_IDX_MVIEWID] == NULL )
    {
        /* createdb하는 경우, 검사할 필요가 없다. */
        *aExist = ID_FALSE;
    }
    else
    {
        sCursor.initialize();

        IDE_TEST( smiGetTableColumns( gQcmMaterializedViews,
                                      QCM_MATERIALIZED_VIEWS_MVIEW_ID_COL_ORDER,
                                      (const smiColumn**)&sMViewIDColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn(
                &sRangeColumn,
                (const mtcColumn *)sMViewIDColumn,
                (const void *)&aMViewID,
                &sRange);

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

        IDE_TEST( sCursor.open(
                    aSmiStmt,
                    gQcmMaterializedViews,
                    gQcmMaterializedViewsIndex[QCM_MATERIALIZED_VIEWS_IDX_MVIEWID],
                    smiGetRowSCN( gQcmMaterializedViews ),
                    NULL,
                    &sRange,
                    smiGetDefaultKeyRange(),
                    smiGetDefaultFilter(),
                    sFlag,
                    SMI_SELECT_CURSOR,
                    &sCursorProperty )
                  != IDE_SUCCESS );
        sIsCursorOpen = ID_TRUE;

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

        if ( sRow != NULL )
        {
            *aExist = ID_TRUE;
        }
        else
        {
            *aExist = ID_FALSE;
        }

        sIsCursorOpen = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}
