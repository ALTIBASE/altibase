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
 * $Id: qcmProc.cpp 90009 2021-02-17 06:54:43Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <qcuProperty.h>
#include <qcm.h>
#include <qcg.h>
#include <qsvEnv.h>
#include <qcmProc.h>
#include <qcmView.h>
#include <qcmPkg.h>
#include <qsxProc.h>
#include <qdParseTree.h>
#include <qcmUser.h>
#include <qdpDrop.h>
#include <mtuProperty.h>
#include <mtf.h>

const void * gQcmProcedures;
const void * gQcmProcParas;
const void * gQcmProcRelated;
const void * gQcmProcParse;
const void * gQcmProceduresIndex [ QCM_MAX_META_INDICES ];
const void * gQcmProcParasIndex  [ QCM_MAX_META_INDICES ];
const void * gQcmProcParseIndex  [ QCM_MAX_META_INDICES ];
const void * gQcmProcRelatedIndex[ QCM_MAX_META_INDICES ];

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
const void * gQcmConstraintRelated;
const void * gQcmIndexRelated;
const void * gQcmConstraintRelatedIndex[ QCM_MAX_META_INDICES ];
const void * gQcmIndexRelatedIndex     [ QCM_MAX_META_INDICES ];

extern mtlModule mtlUTF16;

/*************************************************************
                       common functions
**************************************************************/

#define TYPE_ARG( MtcCol )                                      \
    ( (MtcCol)-> flag & MTC_COLUMN_ARGUMENT_COUNT_MASK )

#define TYPE_PRECISION( MtcCol )                                \
    ( TYPE_ARG( MtcCol ) == 1 ||                                \
      TYPE_ARG( MtcCol ) == 2 ) ? (MtcCol)-> precision : -1

#define TYPE_SCALE( MtcCol )                            \
    ( TYPE_ARG( MtcCol ) == 2 ) ? (MtcCol)-> scale : -1

#define MAX_COLDATA_SQL_LENGTH (255)

IDE_RC qcmProc::getTypeFieldValues (
    qcStatement * aStatement,
    qtcNode     * aTypeNode,
    SChar       * aReturnTypeType,
    SChar       * aReturnTypeLang,
    SChar       * aReturnTypeSize ,
    SChar       * aReturnTypePrecision,
    SChar       * aReturnTypeScale,
    UInt          aMaxBufferLength
    )
{
    mtcColumn          * sReturnTypeNodeColumn       = NULL;

    sReturnTypeNodeColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aStatement),
                                             aTypeNode );

    idlOS::snprintf( aReturnTypeType, aMaxBufferLength,
                     QCM_SQL_INT32_FMT,
                     (SInt) sReturnTypeNodeColumn-> type. dataTypeId );

    idlOS::snprintf( aReturnTypeLang, aMaxBufferLength,
                     QCM_SQL_INT32_FMT,
                     (SInt) sReturnTypeNodeColumn-> type. languageId );

    idlOS::snprintf( aReturnTypeSize , aMaxBufferLength,
                     QCM_SQL_UINT32_FMT,
                     sReturnTypeNodeColumn-> column. size);

    idlOS::snprintf( aReturnTypePrecision , aMaxBufferLength,
                     QCM_SQL_INT32_FMT,
                     sReturnTypeNodeColumn->precision );

    idlOS::snprintf( aReturnTypeScale , aMaxBufferLength,
                     QCM_SQL_INT32_FMT,
                     sReturnTypeNodeColumn->scale );

    return IDE_SUCCESS;
}

/*************************************************************
                      main functions
**************************************************************/
IDE_RC qcmProc::insert (
    qcStatement      * aStatement,
    qsProcParseTree  * aProcParse )
{
    qsRelatedObjects  * sRelObjs;
    qsRelatedObjects  * sTmpRelObjs;
    qsRelatedObjects  * sNewRelObj;
    idBool              sExist = ID_FALSE;
    qsOID               sPkgBodyOID;

    IDE_TEST( qcmProc::procInsert ( aStatement,
                                    aProcParse )
              != IDE_SUCCESS );

    if ( aProcParse-> paraDecls != NULL )
    {
        IDE_TEST( qcmProc::paraInsert ( aStatement,
                                        aProcParse,
                                        aProcParse->paraDecls )
                  != IDE_SUCCESS );
    }


    IDE_TEST( qcmProc::prsInsert( aStatement,
                                  aProcParse )
              != IDE_SUCCESS );


    /* PROJ-2197 PSM Renewal
     * aStatement->spvEnv->relatedObjects 대신
     * aProcParse->procInfo->relatedObjects를 사용한다. */
    for ( sRelObjs = aProcParse->procInfo->relatedObjects;
          sRelObjs != NULL;
          sRelObjs = sRelObjs->next )
    {
        // BUG-36975
        for( sTmpRelObjs = sRelObjs->next ;
             sTmpRelObjs != NULL ;
             sTmpRelObjs = sTmpRelObjs->next )
        {
            if( ( sRelObjs->objectID == sTmpRelObjs->objectID ) &&
                ( idlOS::strMatch( sRelObjs->objectName.name,
                                   sRelObjs->objectName.size,
                                   sTmpRelObjs->objectName.name,
                                   sTmpRelObjs->objectName.size ) == 0 ) )
            {
                sExist = ID_TRUE;
                break;
            }
        }

        if( sExist == ID_FALSE )
        {
            IDE_TEST( qcmProc::relInsert( aStatement,
                                          aProcParse,
                                          sRelObjs )
                      != IDE_SUCCESS );

            // BUG-36975
            if( sRelObjs->objectType == QS_PKG )
            {
                if( qcmPkg::getPkgExistWithEmptyByNamePtr( aStatement,
                                                           sRelObjs->userID,
                                                           (SChar*)(sRelObjs->objectNamePos.stmtText +
                                                                    sRelObjs->objectNamePos.offset),
                                                           sRelObjs->objectNamePos.size,
                                                           QS_PKG_BODY,
                                                           &sPkgBodyOID )
                    == IDE_SUCCESS )
                {
                    if( sPkgBodyOID != QS_EMPTY_OID )
                    {
                        IDE_TEST( qcmPkg::makeRelatedObjectNodeForInsertMeta(
                                           aStatement,
                                           sRelObjs->userID,
                                           sRelObjs->objectID,
                                           sRelObjs->objectNamePos,
                                           (SInt)QS_PKG_BODY,
                                           &sNewRelObj )
                                  != IDE_SUCCESS );

                        IDE_TEST( qcmProc::relInsert( aStatement,
                                                      aProcParse,
                                                      sNewRelObj )
                                  != IDE_SUCCESS );
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
            }
            else
            {
                // Nothing to do.
                //  package만 spec과 body로 분류된다.
            }
        }
        else
        {
            sExist = ID_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::remove (
    qcStatement      * aStatement,
    qsOID              aProcOID,
    qcNamePosition     aUserNamePos,
    qcNamePosition     aProcNamePos,
    idBool             aPreservePrivInfo )
{
    UInt sUserID;

    IDE_TEST( qcmUser::getUserID( aStatement,
                                  aUserNamePos,
                                  & sUserID ) != IDE_SUCCESS );

    IDE_TEST( remove( aStatement,
                      aProcOID,
                      sUserID,
                      aProcNamePos.stmtText + aProcNamePos.offset,
                      aProcNamePos.size,
                      aPreservePrivInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::remove (
    qcStatement      * aStatement,
    qsOID              aProcOID,
    UInt               aUserID,
    SChar            * aProcName,
    UInt               aProcNameSize,
    idBool             aPreservePrivInfo )
{
    //-----------------------------------------------
    // related procedure, function or typeset
    //-----------------------------------------------

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                  aStatement,
                  aUserID,
                  aProcName,
                  aProcNameSize,
                  QS_PROC )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                  aStatement,
                  aUserID,
                  aProcName,
                  aProcNameSize,
                  QS_FUNC )
              != IDE_SUCCESS );

    // PROJ-1075 TYPESET 추가.
    IDE_TEST( qcmProc::relSetInvalidProcOfRelated (
                  aStatement,
                  aUserID,
                  aProcName,
                  aProcNameSize,
                  QS_TYPESET )
              != IDE_SUCCESS );

    // PROJ-1073 Package
    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                 aStatement,
                 aUserID,
                 aProcName,
                 aProcNameSize,
                 QS_PROC )
              != IDE_SUCCESS );

    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                 aStatement,
                 aUserID,
                 aProcName,
                 aProcNameSize,
                 QS_FUNC )
              != IDE_SUCCESS );

    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated (
                 aStatement,
                 aUserID,
                 aProcName,
                 aProcNameSize,
                 QS_TYPESET )
              != IDE_SUCCESS );

    //-----------------------------------------------
    // related view
    // PROJ-1075 TYPESET은 view와 무관함.
    //-----------------------------------------------
    IDE_TEST( qcmView::setInvalidViewOfRelated(
                  aStatement,
                  aUserID,
                  aProcName,
                  aProcNameSize,
                  QS_PROC )
              != IDE_SUCCESS );

    IDE_TEST( qcmView::setInvalidViewOfRelated(
                  aStatement,
                  aUserID,
                  aProcName,
                  aProcNameSize,
                  QS_FUNC )
              != IDE_SUCCESS );

    //-----------------------------------------------
    // remove
    //-----------------------------------------------
    IDE_TEST( qcmProc::procRemove( aStatement,
                                   aProcOID )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::paraRemoveAll( aStatement,
                                      aProcOID )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::prsRemoveAll( aStatement,
                                     aProcOID )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relRemoveAll ( aStatement,
                                      aProcOID )
              != IDE_SUCCESS );

    if( aPreservePrivInfo != ID_TRUE )
    {
        IDE_TEST( qdpDrop::removePriv4DropProc( aStatement,
                                                aProcOID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*************************************************************
                       qcmProcedures
**************************************************************/

IDE_RC qcmProc::procInsert(
    qcStatement      * aStatement,
    qsProcParseTree  * aProcParse )
{
/***********************************************************************
 *
 * Description :
 *    createProcOrFunc 시에 메타 테이블에 입력
 *
 * Implementation :
 *    명시된 ParseTree 로부터 SYS_PROCEDURES_ 메타 테이블에 입력할 데이터를
 *    추출한 후에 입력 쿼리를 만들어서 수행
 *
 ***********************************************************************/

    qsVariableItems    * sParaDecls;
    SInt                 sParaCount = 0;
    vSLong               sRowCnt;
    SChar                sProcName            [ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar              * sBuffer;
    SChar                sReturnTypeType      [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypeLang      [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypeSize      [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypePrecision [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypeScale     [ MAX_COLDATA_SQL_LENGTH ];
    // SChar                sNatGroupOID         [ MAX_COLDATA_SQL_LENGTH ];
    SInt                 sAuthidType = 0;

    QC_STR_COPY( sProcName, aProcParse-> procNamePos );

    for ( sParaDecls = aProcParse-> paraDecls ;
          sParaDecls != NULL                  ;
          sParaDecls = sParaDecls-> next )
    {
        sParaCount ++;
    }

    if ( aProcParse->returnTypeVar != NULL )  // stored function
    {
        IDE_TEST( qcmProc::getTypeFieldValues( aStatement,
                                               aProcParse->returnTypeVar->variableTypeNode,
                                               sReturnTypeType,
                                               sReturnTypeLang,
                                               sReturnTypeSize,
                                               sReturnTypePrecision,
                                               sReturnTypeScale,
                                               MAX_COLDATA_SQL_LENGTH )
                  != IDE_SUCCESS );
    }
    else // stored procedure
    {
        idlOS::snprintf( sReturnTypeType,      MAX_COLDATA_SQL_LENGTH, "NULL");
        idlOS::snprintf( sReturnTypeLang,      MAX_COLDATA_SQL_LENGTH, "NULL");
        idlOS::snprintf( sReturnTypeSize,      MAX_COLDATA_SQL_LENGTH, "NULL");
        idlOS::snprintf( sReturnTypePrecision, MAX_COLDATA_SQL_LENGTH, "NULL");
        idlOS::snprintf( sReturnTypeScale,     MAX_COLDATA_SQL_LENGTH, "NULL");
    }

    /* BUG-45306 Authid */
    if ( aProcParse->isDefiner == ID_TRUE )
    {
        sAuthidType = (SInt)QCM_PROC_DEFINER;
    }
    else
    {
        sAuthidType = (SInt)QCM_PROC_CURRENT_USER;
    }

    IDU_LIMITPOINT("qcmProc::procInsert::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_PROCEDURES_ VALUES ( "
                     QCM_SQL_UINT32_FMT", "     // 1
                     QCM_SQL_BIGINT_FMT", "     // 2
                     QCM_SQL_CHAR_FMT", "       // 3
                     QCM_SQL_INT32_FMT", "      // 4
                     QCM_SQL_INT32_FMT", "      // 5 
                     QCM_SQL_INT32_FMT", "      // 6
                     "%s, "                     // 7
                     "%s, "                     // 8
                     "%s, "                     // 9
                     "%s, "                     // 10
                     "%s, "                     // 11
                     QCM_SQL_INT32_FMT", "      // 12
                     QCM_SQL_INT32_FMT", "      // 13
                     "SYSDATE, SYSDATE, "
                     QCM_SQL_INT32_FMT" )",     // 16
                     aProcParse->userID,                             // 1
                     QCM_OID_TO_BIGINT( aProcParse->procOID ),       // 2
                     sProcName,                                      // 3
                     (SInt) aProcParse->objType,                     // 4
                     (SInt)
                         (aProcParse->procInfo->isValid == ID_TRUE)
                         ? QCM_PROC_VALID : QCM_PROC_INVALID,        // 5
                     (SInt) sParaCount,                              // 6
                     sReturnTypeType,                                // 7
                     sReturnTypeLang,                                // 8
                     sReturnTypeSize,                                // 9
                     sReturnTypePrecision,                           // 10
                     sReturnTypeScale,                               // 11
                     (SInt )
                     ( (aStatement->myPlan->stmtTextLen / QCM_MAX_PROC_LEN)
                       + (((aStatement->myPlan->stmtTextLen % QCM_MAX_PROC_LEN) == 0)
                          ? 0 : 1) ),                                // 12
                     aStatement->myPlan->stmtTextLen,                // 13
                     sAuthidType );                                  // 16

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_inserted_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inserted_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::procInsert] err_inserted_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSetProcOIDOfQcmProcedures(
    idvSQL              * /* aStatistics */,
    const void          * aRow,
    qsOID               * aProcID )
{
    SLong   sSLongID;
    mtcColumn            * sProcIDMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sProcIDMtcColumn,
        & sSLongID );
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    *aProcID = (qsOID)sSLongID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::getProcExistWithEmptyByName(
    qcStatement     * aStatement,
    UInt              aUserID,
    qcNamePosition    aProcName,
    qsOID           * aProcOID )
{
    IDE_TEST(
        getProcExistWithEmptyByNamePtr(
            aStatement,
            aUserID,
            (SChar*) (aProcName.stmtText +
                      aProcName.offset),
            aProcName.size,
            aProcOID)
        != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::getProcExistWithEmptyByNamePtr(
    qcStatement     * aStatement,
    UInt              aUserID,
    SChar           * aProcName,
    SInt              aProcNameSize,
    qsOID           * aProcOID )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;

    mtdIntegerType        sIntData;
    vSLong                sRowCount;
    qsOID                 sSelectedProcOID;

    qcNameCharBuffer      sProcNameBuffer;
    mtdCharType         * sProcName = ( mtdCharType * ) & sProcNameBuffer;
    mtcColumn            * sFstMtcColumn;
    mtcColumn            * sSndMtcColumn;

    if ( aProcNameSize > QC_MAX_OBJECT_NAME_LEN )
    {
        *aProcOID = QS_EMPTY_OID;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    qtc::setVarcharValue( sProcName,
                          NULL,
                          aProcName,
                          aProcNameSize );

    sIntData = (mtdIntegerType) aUserID;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCNAME_COL_ORDER,
                                  (const smiColumn**)&sFstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_USERID_COL_ORDER,
                                  (const smiColumn**)&sSndMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                    & sSecondMetaRange,
                                    sFstMtcColumn,
                                    (const void*) sProcName,
                                    sSndMtcColumn,
                                    & sIntData,
                                    & sRange );

    IDE_TEST( qcm::selectRow(
                  QC_SMI_STMT( aStatement ),
                  gQcmProcedures,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmProceduresIndex
                  [ QCM_PROCEDURES_PROCNAME_USERID_IDX_ORDER ],
                  (qcmSetStructMemberFunc ) qcmSetProcOIDOfQcmProcedures,
                  & sSelectedProcOID,
                  0,
                  1,
                  & sRowCount )
              != IDE_SUCCESS );

    if (sRowCount == 1)
    {
        ( *aProcOID ) = sSelectedProcOID;
    }
    else
    {
        if (sRowCount == 0)
        {
            ( *aProcOID ) = QS_EMPTY_OID;
        }
        else
        {
            IDE_TEST_RAISE( sRowCount != 1, err_selected_count_is_not_1 );
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_selected_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::getProcExistWithEmptyByName]"
                                "err_selected_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::procUpdateStatus(
    qcStatement             * aStatement,
    qsOID                     aProcOID,
    qcmProcStatusType         aStatus)
{
/***********************************************************************
 *
 * Description :
 *    alterProcOrFunc, recompile, rebuild 시에 메타 테이블 변경
 *
 * Implementation :
 *    명시된 ProcOID, status 값으로 SYS_PROCEDURES_ 메타 테이블의
 *    STATUS 값을 변경한다.
 *
 ***********************************************************************/

    SChar    sBuffer[QD_MAX_SQL_LENGTH];
    vSLong   sRowCnt;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_PROCEDURES_ "
                     "SET    STATUS   = "QCM_SQL_INT32_FMT", "
                     "       LAST_DDL_TIME = SYSDATE "
                     "WHERE  PROC_OID = "QCM_SQL_BIGINT_FMT,
                     (SInt) aStatus,
                     QCM_OID_TO_BIGINT( aProcOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_updated_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_updated_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::procUpdateStatus]"
                                "err_updated_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::procUpdateStatusTx(
    qcStatement             * aQcStmt,
    qsOID                     aProcOID,
    qcmProcStatusType         aStatus)
{
    smiTrans       sSmiTrans;
    smiStatement * sDummySmiStmt = NULL;
    smiStatement   sSmiStmt;
    smiStatement * sSmiStmtOrg   = NULL;
    SInt           sState        = 0;
    UInt           sSmiStmtFlag  = 0;

retry:
    // transaction initialze
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    // transaction begin
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aQcStmt->mStatistics)
              != IDE_SUCCESS );
    sState = 2;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    // statement begin
    IDE_TEST( sSmiStmt.begin( aQcStmt->mStatistics, sDummySmiStmt, sSmiStmtFlag )
              != IDE_SUCCESS );

    // swap
    qcg::getSmiStmt( aQcStmt, &sSmiStmtOrg );
    qcg::setSmiStmt( aQcStmt, &sSmiStmt );
    sState = 3;

    if ( qcmProc::procUpdateStatus( aQcStmt,
                                    aProcOID,
                                    aStatus ) != IDE_SUCCESS )
    {
        switch ( ideGetErrorCode() & E_ACTION_MASK )
        {
            case E_ACTION_RETRY:
                qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );

                sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );

                sSmiTrans.rollback();

                sSmiTrans.destroy( aQcStmt->mStatistics );

                goto retry;
                break;
            default:
                IDE_TEST( 1 );
                break;
        }
    }
    else
    {
        // Nothing to do.
    }

    // restore
    qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );

    // statement close
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )!= IDE_SUCCESS );

    // transaction commit
    sState = 1;
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );

    // transaction destroy
    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aQcStmt->mStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 3:
            qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );
            sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            sSmiTrans.rollback();
        case 1:
            sSmiTrans.destroy( aQcStmt->mStatistics );
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmProc::procRemove(
    qcStatement     * aStatement,
    qsOID             aProcOID )
{
/***********************************************************************
 *
 * Description :
 *    replace, drop 시에 메타 테이블에서 삭제
 *
 * Implementation :
 *    명시된 ProcOID 에 해당하는 데이터를 SYS_PROCEDURES_ 메타 테이블에서
 *    삭제한다.
 *
 ***********************************************************************/

    SChar    * sBuffer;
    vSLong     sRowCnt;

    IDU_LIMITPOINT("qcmProc::procRemove::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_PROCEDURES_ "
                     "WHERE  PROC_OID = "QCM_SQL_BIGINT_FMT,
                     QCM_OID_TO_BIGINT( aProcOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_removed_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_removed_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::procRemove]"
                                "err_removed_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::procSelectCount (
    smiStatement         * aSmiStmt,
    vSLong               * aRowCount )
{
    vSLong selectedRowCount;

    IDE_TEST( qcm::selectCount
              (
                  aSmiStmt,
                  gQcmProcedures,
                  &selectedRowCount,
                  smiGetDefaultFilter(),
                  smiGetDefaultKeyRange(),
                  NULL
              ) != IDE_SUCCESS );

    *aRowCount = (vSLong)selectedRowCount ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::procSelectAll (
    smiStatement         * aSmiStmt,
    vSLong                 aMaxProcedureCount,
    vSLong               * aSelectedProcedureCount,
    qcmProcedures        * aProcedureArray )
{
    vSLong selectedRowCount;

    IDE_TEST( qcm::selectRow
              (
                  aSmiStmt,
                  gQcmProcedures,
                  smiGetDefaultFilter(), /* a_callback */
                  smiGetDefaultKeyRange(), /* a_range */
                  NULL, /* a_index */
                  (qcmSetStructMemberFunc ) qcmProc::procSetMember,
                  aProcedureArray,
                  ID_SIZEOF(qcmProcedures), /* distance */
                  aMaxProcedureCount,
                  &selectedRowCount
                  ) != IDE_SUCCESS );

    *aSelectedProcedureCount = (vSLong) selectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::procSelectCountWithUserId (
    qcStatement          * aStatement,
    UInt                   aUserId,
    vSLong               * aRowCount )
{
    vSLong                selectedRowCount;
    qtcMetaRangeColumn    sFirstMetaRange;
    smiRange              sRange;
    mtdIntegerType        sIntData;
    mtcColumn           * sUserIDMtcColumn;

    sIntData = (mtdIntegerType) aUserId;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_USERID_COL_ORDER,
                                  (const smiColumn**)&sUserIDMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                    sUserIDMtcColumn,
                                    & sIntData,
                                    & sRange );

    IDE_TEST( qcm::selectCount
              (
                  QC_SMI_STMT( aStatement ),
                  gQcmProcedures,
                  &selectedRowCount,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmProceduresIndex
                  [ QCM_PROCEDURES_USERID_IDX_ORDER ]
              ) != IDE_SUCCESS );

    *aRowCount = (vSLong)selectedRowCount ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::procSelectAllWithUserId (
    qcStatement          * aStatement,
    UInt                   aUserId,
    vSLong                 aMaxProcedureCount,
    vSLong               * aSelectedProcedureCount,
    qcmProcedures        * aProcedureArray )
{
    vSLong                selectedRowCount;
    qtcMetaRangeColumn    sFirstMetaRange;
    smiRange              sRange;
    mtdIntegerType        sIntData;
    mtcColumn           * sUserIDMtcColumn;

    sIntData = (mtdIntegerType) aUserId;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_USERID_COL_ORDER,
                                  (const smiColumn**)&sUserIDMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                    sUserIDMtcColumn,
                                    & sIntData,
                                    & sRange );

    IDE_TEST( qcm::selectRow
              (
                  QC_SMI_STMT( aStatement ),
                  gQcmProcedures,
                  smiGetDefaultFilter(), /* a_callback */
                  & sRange,
                  gQcmProceduresIndex
                  [ QCM_PROCEDURES_USERID_IDX_ORDER ],
                  (qcmSetStructMemberFunc ) qcmProc::procSetMember,
                  aProcedureArray,
                  ID_SIZEOF(qcmProcedures), /* distance */
                  aMaxProcedureCount,
                  &selectedRowCount
                  ) != IDE_SUCCESS );

    *aSelectedProcedureCount = (vSLong) selectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*************************************************************
                       qcmProcParas
**************************************************************/
IDE_RC qcmProc::paraInsert(
    qcStatement     * aStatement,
    qsProcParseTree * aProcParse,
    qsVariableItems * aParaDeclParse)
{
/***********************************************************************
 *
 * Description :
 *    create 시에 메타 테이블에 프로시져의 인자 정보 입력
 *
 * Implementation :
 *    명시된 ParseTree 로부터 인자정보를 추출하여 SYS_PROC_PARAS_
 *    메타 테이블에 입력하는 쿼리문을 만든 후 수행
 *
 ***********************************************************************/

    vSLong               sRowCnt;
    SInt                 sParaOrder = 0;
    SChar                sParaName            [ QC_MAX_OBJECT_NAME_LEN + 1    ];
    SChar                sDefaultValueString  [ QCM_MAX_DEFAULT_VAL + 2];
    SChar              * sBuffer;
    SChar                sReturnTypeType      [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypeLang      [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypeSize      [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypePrecision [ MAX_COLDATA_SQL_LENGTH ];
    SChar                sReturnTypeScale     [ MAX_COLDATA_SQL_LENGTH ];
    qsVariables        * sParaVar;

    while( aParaDeclParse != NULL )
    {
        sParaVar = (qsVariables*)aParaDeclParse;

        sParaOrder ++ ;

        QC_STR_COPY( sParaName, aParaDeclParse->name );

        if (sParaVar->defaultValueNode != NULL)
        {
            prsCopyStrDupAppo(
                sDefaultValueString,
                sParaVar->defaultValueNode->position.stmtText +
                sParaVar->defaultValueNode->position.offset,
                sParaVar->defaultValueNode->position.size );
        }
        else
        {
            sDefaultValueString[0] = '\0';
        }

        IDE_TEST( qcmProc::getTypeFieldValues( aStatement,
                                               sParaVar-> variableTypeNode,
                                               sReturnTypeType,
                                               sReturnTypeLang,
                                               sReturnTypeSize,
                                               sReturnTypePrecision,
                                               sReturnTypeScale,
                                               MAX_COLDATA_SQL_LENGTH )
                  != IDE_SUCCESS );

        IDU_LIMITPOINT("qcmProc::paraInsert::malloc");
        IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                        SChar,
                                        QD_MAX_SQL_LENGTH,
                                        &sBuffer)
                 != IDE_SUCCESS);

        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_PROC_PARAS_ VALUES ( "
                         QCM_SQL_UINT32_FMT", "                     // 1
                         QCM_SQL_BIGINT_FMT", "                     // 2
                         QCM_SQL_CHAR_FMT", "                       // 3
                         QCM_SQL_INT32_FMT", "                      // 4
                         QCM_SQL_INT32_FMT", "                      // 5
                         "%s ,"                                     // 6
                         "%s ,"                                     // 7
                         "%s ,"                                     // 8
                         "%s ,"                                     // 9
                         "%s ,"                                     // 10
                         QCM_SQL_CHAR_FMT" ) ",                     // 11
                         aProcParse-> userID,                       // 1
                         QCM_OID_TO_BIGINT( aProcParse-> procOID ), // 2
                         sParaName,                                 // 3
                         sParaOrder,                                // 4
                         (SInt) sParaVar->inOutType,                // 5
                         sReturnTypeType,                           // 6
                         sReturnTypeLang,                           // 7
                         sReturnTypeSize,                           // 8
                         sReturnTypePrecision,                      // 9
                         sReturnTypeScale,                          // 10
                         sDefaultValueString );                     // 11

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, err_inserted_count_is_not_1 );

        aParaDeclParse = aParaDeclParse->next ;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inserted_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::paraInsert] err_inserted_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::paraRemoveAll(
    qcStatement     * aStatement,
    qsOID             aProcOID)
{
/***********************************************************************
 *
 * Description :
 *    삭제시에 메타 테이블에 프로시져의 인자 정보 삭제
 *
 * Implementation :
 *    명시된 ProcOID 에 해당한는 데이터를 SYS_PROC_PARAS_ 메타 테이블에서
 *    삭제하는 쿼리문을 만든 후 수행
 *
 ***********************************************************************/

    SChar    * sBuffer;
    vSLong     sRowCnt;

    IDU_LIMITPOINT("qcmProc::paraRemoveAll::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_PROC_PARAS_ "
                     "WHERE  PROC_OID = "QCM_SQL_BIGINT_FMT,
                     QCM_OID_TO_BIGINT( aProcOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    // [original code] no error is raised even if no row is deleted.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************
                       qcmProcParse
**************************************************************/
IDE_RC qcmProc::prsInsert(
    qcStatement     * aStatement,
    qsProcParseTree * aProcParse )
{
    SInt            sCurrPos = 0;
    SInt            sCurrLen = 0;
    SInt            sSeqNo   = 0;
    UInt            sAddSize = 0;
    SChar         * sStmtBuffer = NULL;
    UInt            sStmtBufferLen = 0;
    qcNamePosList * sNcharList = NULL;
    qcNamePosList * sTempNamePosList = NULL;
    qcNamePosition  sNamePos;
    UInt            sBufferSize = 0;
    SChar* sIndex;
    SChar* sStartIndex;
    SChar* sPrevIndex;
    
    const mtlModule* sModule;

    sNcharList = aProcParse->ncharList;

    /* PROJ-2550 PSM Encryption
       system_.sys_proc_parse_의 메타테이블에서는
       입력받은 쿼리가 insert되어야 한다.
       즉, encrypted text로 입력받았으면, encrypted text가
       일반 쿼리로 입력받았으면, 해당 쿼리가 insert 된다. */
    if ( aStatement->myPlan->encryptedText == NULL )
    {
        // PROJ-1579 NCHAR
        // 메타테이블에 저장하기 위해 스트링을 분할하기 전에
        // N 타입이 있는 경우 U 타입으로 변환한다.
        if ( sNcharList != NULL )
        {
            for ( sTempNamePosList = sNcharList;
                  sTempNamePosList != NULL;
                  sTempNamePosList = sTempNamePosList->next )
            {
                sNamePos = sTempNamePosList->namePos;

                // U 타입으로 변환하면서 늘어나는 사이즈 계산
                // N'안' => U'\C548' 으로 변환된다면
                // '안'의 캐릭터 셋이 KSC5601이라고 가정했을 때,
                // single-quote안의 문자는 2 byte -> 5byte로 변경된다.
                // 즉, 1.5배가 늘어나는 것이다.
                //(전체 사이즈가 아니라 증가하는 사이즈만 계산하는 것임)
                // 하지만, 어떤 예외적인 캐릭터 셋이 들어올지 모르므로
                // * 2로 충분히 잡는다.
                sAddSize += (sNamePos.size - 3) * 2;
            }

            sBufferSize = aStatement->myPlan->stmtTextLen + sAddSize;

            IDU_LIMITPOINT("qcmProc::prsInsert::malloc");
            IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                            SChar,
                                            sBufferSize,
                                            & sStmtBuffer)
                     != IDE_SUCCESS);

            IDE_TEST( convertToUTypeString( aStatement,
                                            sNcharList,
                                            sStmtBuffer,
                                            sBufferSize )
                      != IDE_SUCCESS );

            sStmtBufferLen = idlOS::strlen( sStmtBuffer );
        }
        else
        {
            sStmtBufferLen = aStatement->myPlan->stmtTextLen;
            sStmtBuffer = aStatement->myPlan->stmtText;
        }
    }
    else
    {
        sStmtBufferLen = aStatement->myPlan->encryptedTextLen;
        sStmtBuffer    = aStatement->myPlan->encryptedText;
    }

    sModule = mtl::mDBCharSet;

    sStartIndex = sStmtBuffer;
    sIndex = sStartIndex;

    // To fix BUG-21299
    // 100bytes 단위로 자르되, 캐릭터셋에 맞게 문자를 자른다.
    // 즉, 다음 캐릭터를 읽었을 때 100바이트를 넘는 경우가 생기는데,
    // 이때는 그 이전 캐릭터를 읽었을 때로 돌아가서 거기까지만 잘라서 기록을 하고,
    // 그 다음에 이어서 기록을 한다.
    while (1)
    {
        sPrevIndex = sIndex;
        
        (void)sModule->nextCharPtr( (UChar**) &sIndex,
                                    (UChar*)(sStmtBuffer +
                                             sStmtBufferLen) );
        
        if( (sStmtBuffer +
             sStmtBufferLen) <= sIndex )
        {
            // 끝까지 간 경우.
            // 기록을 한 후 break.
            sSeqNo++;

            sCurrPos = sStartIndex - sStmtBuffer;
            sCurrLen = sIndex - sStartIndex;

            // insert v_qcm_proc_parse
            IDE_TEST( qcmProc::prsInsertFragment(
                          aStatement,
                          sStmtBuffer,
                          aProcParse,
                          sSeqNo,
                          sCurrPos,
                          sCurrLen )
                      != IDE_SUCCESS );
            break;
        }
        else
        {
            if( sIndex - sStartIndex >= QCM_MAX_PROC_LEN )
            {
                // 아직 끝가지 안 갔고, 읽다보니 100바이트 또는 초과한 값이
                // 되었을 때 잘라서 기록
                sCurrPos = sStartIndex - sStmtBuffer;
                
                if( sIndex - sStartIndex == QCM_MAX_PROC_LEN )
                {
                    // 딱 떨어지는 경우
                    sCurrLen = QCM_MAX_PROC_LEN;
                    sStartIndex = sIndex;
                }
                else
                {
                    // 삐져나간 경우 그 이전 캐릭터 위치까지 기록
                    sCurrLen = sPrevIndex - sStartIndex;
                    sStartIndex = sPrevIndex;
                }

                sSeqNo++;
                
                // insert v_qcm_proc_parse
                IDE_TEST( qcmProc::prsInsertFragment(
                              aStatement,
                              sStmtBuffer,
                              aProcParse,
                              sSeqNo,
                              sCurrPos,
                              sCurrLen )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::prsCopyStrDupAppo (
    SChar         * aDest,
    SChar         * aSrc,
    UInt            aLength )
{

    while( aLength-- > 0 )
    {
        if ( *aSrc == '\'' )
        {
            *aDest ++ = '\'';
            *aDest ++ = '\'';
        }
        else
        {
            *aDest ++ = *aSrc;
        }
        aSrc ++;
    }
    *aDest = '\0';

    return IDE_SUCCESS;
}

IDE_RC qcmProc::prsInsertFragment(
    qcStatement     * aStatement,
    SChar           * aStmtBuffer,
    qsProcParseTree * aProcParse,
    UInt              aSeqNo,
    UInt              aOffset,
    UInt              aLength )
{
/***********************************************************************
 *
 * Description :
 *    생성시에 사용된 쿼리문장을 SYS_PROC_PARSE_ 에 저장
 *
 * Implementation :
 *    생성시에 사용된 쿼리문장이 적절한 사이즈로 시퀀스와 함께 전달되면,
 *    SYS_PROC_PARSE_ 메타 테이블에 입력하는 쿼리를 만들어서 수행
 *
 ***********************************************************************/

    vSLong sRowCnt;
    SChar  sParseStr [ QCM_MAX_PROC_LEN * 2 + 2 ];
    SChar *sBuffer;

    if( aStmtBuffer != NULL )
    {
        prsCopyStrDupAppo( sParseStr,
                           aStmtBuffer + aOffset,
                           aLength );
    }
    else
    {
        prsCopyStrDupAppo( sParseStr,
                           aStatement->myPlan->stmtText + aOffset,
                           aLength );
    }

    IDU_LIMITPOINT("qcmProc::prsInsertFragment::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_PROC_PARSE_ VALUES ( "
                     QCM_SQL_UINT32_FMT", "                     // 1
                     QCM_SQL_BIGINT_FMT", "                     // 2
                     QCM_SQL_UINT32_FMT", "                     // 3
                     QCM_SQL_CHAR_FMT") ",                      // 4
                     aProcParse-> userID,                       // 1
                     QCM_OID_TO_BIGINT( aProcParse-> procOID ), // 2
                     aSeqNo,                                    // 3
                     sParseStr );                               // 4

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_inserted_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inserted_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::prsInsertFragment]"
                                "err_inserted_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::prsRemoveAll(
    qcStatement     * aStatement,
    qsOID             aProcOID)
{
/***********************************************************************
 *
 * Description :
 *    drop 시에 SYS_PROC_PARSE_ 테이블로부터 삭제
 *
 * Implementation :
 *    SYS_PROC_PARSE_ 메타 테이블에서 명시된 ProcID 의 데이터를 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDU_LIMITPOINT("qcmProc::prsRemoveAll::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_PROC_PARSE_ "
                     "WHERE  PROC_OID = "QCM_SQL_BIGINT_FMT,
                     QCM_OID_TO_BIGINT( aProcOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    // [original code] no error is raised even if no row is deleted.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qcmProc::convertToUTypeString( qcStatement   * aStatement,
                                      qcNamePosList * aNcharList,
                                      SChar         * aDest,
                                      UInt            aBufferSize )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1579 NCHAR
 *
 *      N'안'과 같은 스트링을 메타테이블에 저장할 경우
 *      ALTIBASE_NLS_NCHAR_LITERAL_REPLACE = 1 인 경우
 *      U'\C548'과 같이 저장된다.
 *
 * Implementation :
 *
 *      aStatement->namePosList가 stmt에 나온 순서대로 정렬되어 있다고
 *      가정한다.
 *
 *      EX) create view v1 
 *          as select * from t1 where i1 = n'안' and i2 = n'녕' and i3 = 'A';
 *
 *      1. loop(n type이 있는 동안)
 *          1-1. 'n'-1까지 memcpy
 *          1-2. u' memcpy
 *
 *          1-3. loop(n type literal을 캐릭터 단위로 반복)
 *              1) \ 복사
 *              2) 안 => C548와 같이 유니코드 포인트 형태로 변형해서 복사
 *                 (ASCII는 그대로 복사한다.)
 *
 *      2. stmt의 맨 뒷부분 복사
 *
 ***********************************************************************/

    qcNamePosList   * sNcharList;
    qcNamePosList   * sTempNamePosList;
    qcNamePosition    sNamePos;
    const mtlModule * sClientCharSet;

    SChar           * sSrcVal;
    UInt              sSrcLen = 0;

    SChar           * sNTypeVal;
    UInt              sNTypeLen = 0;
    SChar           * sNTypeFence;

    UInt              sSrcValOffset = 0;
    UInt              sDestOffset = 0;
    UInt              sCharLength = 0;

    sNcharList = aNcharList;
    
    sClientCharSet = QCG_GET_SESSION_LANGUAGE(aStatement);
    
    sSrcVal = aStatement->myPlan->stmtText;
    sSrcLen = aStatement->myPlan->stmtTextLen;
    
    if( sNcharList != NULL )
    {
        for( sTempNamePosList = sNcharList;
             sTempNamePosList != NULL;
             sTempNamePosList = sTempNamePosList->next )
        {
            sNamePos = sTempNamePosList->namePos;

            // -----------------------------------
            // N 바로 전까지 복사
            // -----------------------------------
            idlOS::memcpy( aDest + sDestOffset,
                           sSrcVal + sSrcValOffset,
                           sNamePos.offset - sSrcValOffset );

            sDestOffset += (sNamePos.offset - sSrcValOffset );

            // -----------------------------------
            // U'\ 복사
            // -----------------------------------
            idlOS::memcpy( aDest + sDestOffset,
                           "U\'",
                           2 );

            sDestOffset += 2;

            // -----------------------------------
            // N 타입 리터럴의 캐릭터 셋 변환
            // 클라이언트 캐릭터 셋 => 내셔널 캐릭터 셋
            // -----------------------------------
            sNTypeVal = aStatement->myPlan->stmtText + sNamePos.offset + 2;
            sNTypeLen = sNamePos.size - 3;
            sNTypeFence = sNTypeVal + sNTypeLen;

            IDE_TEST( mtf::makeUFromChar( sClientCharSet,
                                          (UChar *)sNTypeVal,
                                          (UChar *)sNTypeFence,
                                          (UChar *)aDest + sDestOffset,
                                          (UChar *)aDest + aBufferSize,
                                          & sCharLength )
                      != IDE_SUCCESS );

            sDestOffset += sCharLength;

            sSrcValOffset = sNamePos.offset + sNamePos.size - 1;
        }

        // -----------------------------------
        // '부터 끝까지 복사
        // -----------------------------------
        idlOS::memcpy( aDest + sDestOffset,
                       sSrcVal + sNamePos.offset + sNamePos.size - 1,
                       sSrcLen - (sNamePos.offset + sNamePos.size - 1) );

        sDestOffset += sSrcLen - (sNamePos.offset + sNamePos.size - 1);

        aDest[sDestOffset] = '\0';
    }
    else
    {
        // N리터럴이 없으므로 memcpy함.
        idlOS::memcpy( aDest, sSrcVal, sSrcLen );
        
        aDest[sSrcLen] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*************************************************************
                       qcmProcRelated
**************************************************************/
IDE_RC qcmProc::relInsert(
    qcStatement         * aStatement,
    qsProcParseTree     * aProcParse,
    qsRelatedObjects    * aRelatedObjList )
{
/***********************************************************************
 *
 * Description :
 *    프로시져 생성과 관련된 오브젝트에 대한 정보를 입력한다.
 *
 * Implementation :
 *    SYS_PROC_RELATED_ 메타 테이블에 명시한 오브젝트들을 입력한다.
 *
 ***********************************************************************/

    vSLong sRowCnt;
    UInt   sRelUserID ;
    SChar  sRelObjectName [ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar *sBuffer;

    // BUG-46395 public synonym을 고려한다.
    if ( ( aRelatedObjList->objectType == QS_SYNONYM ) &&
         ( aRelatedObjList->userName.size == 0 ) )
    {
        sRelUserID = QC_PUBLIC_USER_ID;
    }
    else
    {
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      aRelatedObjList->userName.name,
                                      aRelatedObjList->userName.size,
                                      & sRelUserID ) != IDE_SUCCESS );
    }

    idlOS::memcpy(sRelObjectName,
                  aRelatedObjList-> objectName.name,
                  aRelatedObjList-> objectName.size);
    sRelObjectName[ aRelatedObjList->objectName.size ] = '\0';

    IDU_LIMITPOINT("qcmProc::relInsert::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    if ( sRelUserID != QC_PUBLIC_USER_ID )
    {
        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_PROC_RELATED_ VALUES ( "
                         QCM_SQL_UINT32_FMT", "                     // 1
                         QCM_SQL_BIGINT_FMT", "                     // 2
                         QCM_SQL_INT32_FMT", "                      // 3
                         QCM_SQL_CHAR_FMT", "                       // 4
                         QCM_SQL_INT32_FMT") ",                     // 5
                         aProcParse-> userID,                       // 1
                         QCM_OID_TO_BIGINT( aProcParse-> procOID ), // 2
                         sRelUserID,                                // 3
                         sRelObjectName,                            // 4
                         aRelatedObjList-> objectType );            // 5
    }
    else
    {
        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_PROC_RELATED_ VALUES ( "
                         QCM_SQL_UINT32_FMT", "                     // 1
                         QCM_SQL_BIGINT_FMT", "                     // 2
                         "NULL, "                                   // NULL value
                         QCM_SQL_CHAR_FMT", "                       // 4
                         QCM_SQL_INT32_FMT") ",                     // 5
                         aProcParse-> userID,                       // 1
                         QCM_OID_TO_BIGINT( aProcParse-> procOID ), // 2
                         sRelObjectName,                            // 4
                         aRelatedObjList-> objectType );            // 5
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_inserted_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inserted_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::relInsert] err_inserted_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


// for each row in SYS_PROC_RELATED_ concered,
// update SYS_PROCEDURES_ to QCM_PROC_INVALID
IDE_RC qcmModifyStatusOfRelatedProcToInvalid (
    idvSQL              * /* aStatistics */,
    const void          * aRow,
    qcStatement         * aStatement )
{
    qsOID           sProcOID;
    SLong           sSLongOID;
    mtcColumn     * sProcIDMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmProcRelated,
                                  QCM_PROC_RELATED_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sProcIDMtcColumn,
        & sSLongOID );
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    sProcOID = (qsOID)sSLongOID;

    IDE_TEST( qsxProc::makeStatusInvalid( aStatement,
                                          sProcOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-46416
IDE_RC qcmModifyStatusOfRelatedProcToInvalidTx (
    idvSQL              * /* aStatistics */,
    const void          * aRow,
    qcStatement         * aStatement )
{
    qsOID           sProcOID;
    SLong           sSLongOID;
    mtcColumn     * sProcIDMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmProcRelated,
                                  QCM_PROC_RELATED_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sProcIDMtcColumn,
        & sSLongOID );
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    sProcOID = (qsOID)sSLongOID;

    IDE_TEST( qsxProc::makeStatusInvalidTx( aStatement,
                                            sProcOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relSetInvalidProcOfRelated(
    qcStatement    * aStatement,
    UInt             aRelatedUserID,
    SChar          * aRelatedObjectName,
    UInt             aRelatedObjectNameSize,
    qsObjectType     aRelatedObjectType,
    idBool           aIsUseTx )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    qtcMetaRangeColumn    sThirdMetaRange;
    mtdIntegerType        sObjectTypeIntData;
    mtdIntegerType        sUserIdData;
    vSLong                sRowCount;

    qcNameCharBuffer      sObjectNameBuffer;
    mtdCharType         * sObjectName = ( mtdCharType * ) & sObjectNameBuffer ;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSecondMtcColumn;
    mtcColumn           * sThirdMtcColumn;

    sUserIdData = (mtdIntegerType) aRelatedUserID;

    qtc::setVarcharValue( sObjectName,
                          NULL,
                          aRelatedObjectName,
                          aRelatedObjectNameSize );

    sObjectTypeIntData = (mtdIntegerType) aRelatedObjectType;

    IDE_TEST( smiGetTableColumns( gQcmProcRelated,
                                  QCM_PROC_RELATED_RELATEDUSERID_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmProcRelated,
                                  QCM_PROC_RELATED_RELATEDOBJECTNAME_COL_ORDER,
                                  (const smiColumn**)&sSecondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmProcRelated,
                                  QCM_PROC_RELATED_RELATEDOBJECTTYPE_COL_ORDER,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    // BUG-46395 public synonym을 고려한다.
    // Public synonym은 user id가 NULL이다.
    // 검색 조건
    qtc::initializeMetaRange( &sRange,
                              MTD_COMPARE_MTDVAL_MTDVAL );  // Meta is memory table

    if ( aRelatedUserID != QC_PUBLIC_USER_ID )
    {
        qtc::setMetaRangeColumn( &sFirstMetaRange,
                                 sFirstMtcColumn,
                                 & sUserIdData,
                                 SMI_COLUMN_ORDER_ASCENDING,
                                 0 );  // First column of Index
    }
    else
    {
        qtc::setMetaRangeIsNullColumn(
            &sFirstMetaRange,
            sFirstMtcColumn,
            0 );  // First column of Index
    }

    qtc::addMetaRangeColumn( &sRange,
                             &sFirstMetaRange );

    qtc::setMetaRangeColumn( &sSecondMetaRange,
                             sSecondMtcColumn,
                             (const void*) sObjectName,
                             SMI_COLUMN_ORDER_ASCENDING,
                             1 );  // Second column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sSecondMetaRange );

    qtc::setMetaRangeColumn( &sThirdMetaRange,
                             sThirdMtcColumn,
                             & sObjectTypeIntData,
                             SMI_COLUMN_ORDER_ASCENDING,
                             2 );  // Third column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sThirdMetaRange );

    qtc::fixMetaRange( &sRange );

    IDE_TEST( qcm::selectRow(
                  QC_SMI_STMT( aStatement ),
                  gQcmProcRelated,
                  smiGetDefaultFilter(), /* a_callback */
                  & sRange,
                  gQcmProcRelatedIndex
                  [ QCM_PROC_RELATED_RELUSERID_RELOBJNAME_RELOBJTYPE ],
                  (aIsUseTx == ID_FALSE)?(qcmSetStructMemberFunc )qcmModifyStatusOfRelatedProcToInvalid:
                                         (qcmSetStructMemberFunc )qcmModifyStatusOfRelatedProcToInvalidTx,
                  aStatement, /* argument to qcmSetStructMemberFunc */
                  0,          /* aMetaStructDistance.
                                 0 means do not change pointer to aStatement */
                  ID_UINT_MAX,/* no limit on selected row count */
                  & sRowCount )
              != IDE_SUCCESS );

    // no row of qcmProcRelated is updated
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // no row of qcmProcRelated is updated
    return IDE_FAILURE;
}


IDE_RC qcmProc::relRemoveAll(
    qcStatement     * aStatement,
    qsOID             aProcOID)
{
/***********************************************************************
 *
 * Description :
 *    프로시져와 관련된 오브젝트에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    SYS_PROC_RELATED_ 메타 테이블에서 명시한 ProcOID 에 해당하는
 *    데이터를 삭제한다.
 *
 ***********************************************************************/

    SChar   * sBuffer;
    vSLong    sRowCnt;

    IDU_LIMITPOINT("qcmProc::relRemoveAll::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sBuffer)
             != IDE_SUCCESS);

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_PROC_RELATED_ "
                     "WHERE  PROC_OID = "QCM_SQL_BIGINT_FMT,
                     QCM_OID_TO_BIGINT( aProcOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    // [original code] no error is raised even if no row is deleted.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************
                       qcmConstraintRelated
**************************************************************/
IDE_RC qcmProc::relInsertRelatedToConstraint(
    qcStatement             * aStatement,
    qsConstraintRelatedProc * aRelatedProc )
{
/***********************************************************************
 *
 * Description :
 *    Constraint와 관련된 Procedure에 대한 정보를 입력한다.
 *
 * Implementation :
 *    SYS_CONSTRAINT_RELATED_ 메타 테이블에 데이터를 입력한다.
 *
 ***********************************************************************/

    vSLong   sRowCnt;
    SChar    sRelatedProcName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar  * sBuffer;

    QC_STR_COPY( sRelatedProcName, aRelatedProc->relatedProcName );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "INSERT INTO SYS_CONSTRAINT_RELATED_ VALUES ( "
                            QCM_SQL_UINT32_FMT", "          // 1
                            QCM_SQL_UINT32_FMT", "          // 2
                            QCM_SQL_UINT32_FMT", "          // 3
                            QCM_SQL_UINT32_FMT", "          // 4
                            QCM_SQL_CHAR_FMT" ) ",          // 5
                            aRelatedProc->userID,           // 1
                            aRelatedProc->tableID,          // 2
                            aRelatedProc->constraintID,     // 3
                            aRelatedProc->relatedUserID,    // 4
                            sRelatedProcName );             // 5

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_INSERTED_COUNT_IS_NOT_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSERTED_COUNT_IS_NOT_1 );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_INTERNAL_ARG,
                                  "[qcmProc::relInsertRelatedToConstraint] ERR_INSERTED_COUNT_IS_NOT_1" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relRemoveRelatedToConstraintByUserID( qcStatement * aStatement,
                                                      UInt          aUserID )
{
/***********************************************************************
 *
 * Description :
 *    Constraint와 관련된 Procedure에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    명시한 User ID에 해당하는 데이터를 SYS_CONSTRAINT_RELATED_
 *    메타 테이블에서 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "DELETE FROM SYS_CONSTRAINT_RELATED_ "
                            "WHERE  USER_ID = "QCM_SQL_UINT32_FMT,
                            aUserID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    /* [original code] no error is raised, even if no row is deleted. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relRemoveRelatedToConstraintByTableID( qcStatement * aStatement,
                                                       UInt          aTableID )
{
/***********************************************************************
 *
 * Description :
 *    Constraint와 관련된 Procedure에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    명시한 Table ID에 해당하는 데이터를 SYS_CONSTRAINT_RELATED_
 *    메타 테이블에서 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "DELETE FROM SYS_CONSTRAINT_RELATED_ "
                            "WHERE  TABLE_ID = "QCM_SQL_UINT32_FMT,
                            aTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    /* [original code] no error is raised, even if no row is deleted. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relRemoveRelatedToConstraintByConstraintID( qcStatement * aStatement,
                                                            UInt          aConstraintID )
{
/***********************************************************************
 *
 * Description :
 *    Constraint와 관련된 Procedure에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    명시한 Constraint ID에 해당하는 데이터를 SYS_CONSTRAINT_RELATED_
 *    메타 테이블에서 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "DELETE FROM SYS_CONSTRAINT_RELATED_ "
                            "WHERE  CONSTRAINT_ID = "QCM_SQL_UINT32_FMT,
                            aConstraintID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    /* [original code] no error is raised, even if no row is deleted. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relIsUsedProcByConstraint( qcStatement    * aStatement,
                                           qcNamePosition * aRelatedProcName,
                                           UInt             aRelatedUserID,
                                           idBool         * aIsUsed )
{
/***********************************************************************
 *
 * Description :
 *    Procedure가 Constraint에서 사용 중인지 확인한다.
 *
 * Implementation :
 *    SYS_CONSTRAINT_RELATED_ 메타 테이블에서 Procedure를 검색한다.
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;

    qcNameCharBuffer      sRelatedProcNameBuffer;
    mtdCharType         * sRelatedProcName = (mtdCharType *)&sRelatedProcNameBuffer;
    mtdIntegerType        sRelatedUserID;

    smiTableCursor        sCursor;
    idBool                sCursorOpened = ID_FALSE;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sProperty;
    mtcColumn           * sFstMtcColumn;
    mtcColumn           * sSndMtcColumn;

    qtc::setVarcharValue( sRelatedProcName,
                          NULL,
                          aRelatedProcName->stmtText + aRelatedProcName->offset,
                          aRelatedProcName->size );
    sRelatedUserID = (mtdIntegerType)aRelatedUserID;

    IDE_TEST( smiGetTableColumns( gQcmConstraintRelated,
                                  QCM_CONSTRAINT_RELATED_RELATEDPROCNAME_COL_ORDER,
                                  (const smiColumn**)&sFstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmConstraintRelated,
                                  QCM_CONSTRAINT_RELATED_RELATEDUSERID_COL_ORDER,
                                  (const smiColumn**)&sSndMtcColumn )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sFirstMetaRange,
        &sSecondMetaRange,
        sFstMtcColumn,
        (const void *)sRelatedProcName,
        sSndMtcColumn,
        &sRelatedUserID,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sProperty, NULL );

    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gQcmConstraintRelated,
                            gQcmConstraintRelatedIndex[QCM_CONSTRAINT_RELATED_RELPROCNAME_RELUSERID],
                            smiGetRowSCN( gQcmConstraintRelated ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sCursorOpened = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        *aIsUsed = ID_TRUE;
    }
    else
    {
        *aIsUsed = ID_FALSE;
    }

    sCursorOpened = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sCursorOpened == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}


/*************************************************************
                       qcmIndexRelated
**************************************************************/
IDE_RC qcmProc::relInsertRelatedToIndex(
    qcStatement        * aStatement,
    qsIndexRelatedProc * aRelatedProc )
{
/***********************************************************************
 *
 * Description :
 *    Index와 관련된 Procedure에 대한 정보를 입력한다.
 *
 * Implementation :
 *    SYS_INDEX_RELATED_ 메타 테이블에 데이터를 입력한다.
 *
 ***********************************************************************/

    vSLong   sRowCnt;
    SChar    sRelatedProcName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar  * sBuffer;

    QC_STR_COPY( sRelatedProcName, aRelatedProc->relatedProcName );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "INSERT INTO SYS_INDEX_RELATED_ VALUES ( "
                            QCM_SQL_UINT32_FMT", "          // 1
                            QCM_SQL_UINT32_FMT", "          // 2
                            QCM_SQL_UINT32_FMT", "          // 3
                            QCM_SQL_UINT32_FMT", "          // 4
                            QCM_SQL_CHAR_FMT" ) ",          // 5
                            aRelatedProc->userID,           // 1
                            aRelatedProc->tableID,          // 2
                            aRelatedProc->indexID,          // 3
                            aRelatedProc->relatedUserID,    // 4
                            sRelatedProcName );             // 5

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_INSERTED_COUNT_IS_NOT_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSERTED_COUNT_IS_NOT_1 );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_INTERNAL_ARG,
                                  "[qcmProc::relInsertRelatedToIndex] ERR_INSERTED_COUNT_IS_NOT_1" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relRemoveRelatedToIndexByUserID( qcStatement * aStatement,
                                                 UInt          aUserID )
{
/***********************************************************************
 *
 * Description :
 *    Index와 관련된 Procedure에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    명시한 User ID에 해당하는 데이터를 SYS_INDEX_RELATED_
 *    메타 테이블에서 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "DELETE FROM SYS_INDEX_RELATED_ "
                            "WHERE  USER_ID = "QCM_SQL_UINT32_FMT,
                            aUserID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    /* [original code] no error is raised, even if no row is deleted. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relRemoveRelatedToIndexByTableID( qcStatement * aStatement,
                                                  UInt          aTableID )
{
/***********************************************************************
 *
 * Description :
 *    Index와 관련된 Procedure에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    명시한 Table ID에 해당하는 데이터를 SYS_INDEX_RELATED_
 *    메타 테이블에서 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "DELETE FROM SYS_INDEX_RELATED_ "
                            "WHERE  TABLE_ID = "QCM_SQL_UINT32_FMT,
                            aTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    /* [original code] no error is raised, even if no row is deleted. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relRemoveRelatedToIndexByIndexID( qcStatement * aStatement,
                                                  UInt          aIndexID )
{
/***********************************************************************
 *
 * Description :
 *    Index와 관련된 Procedure에 대한 정보를 삭제한다.
 *
 * Implementation :
 *    명시한 Index ID에 해당하는 데이터를 SYS_INDEX_RELATED_
 *    메타 테이블에서 삭제한다.
 *
 ***********************************************************************/

    SChar  * sBuffer;
    vSLong   sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sBuffer )
              != IDE_SUCCESS );

    (void) idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                            "DELETE FROM SYS_INDEX_RELATED_ "
                            "WHERE  INDEX_ID = "QCM_SQL_UINT32_FMT,
                            aIndexID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    /* [original code] no error is raised, even if no row is deleted. */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::relIsUsedProcByIndex( qcStatement    * aStatement,
                                      qcNamePosition * aRelatedProcName,
                                      UInt             aRelatedUserID,
                                      idBool         * aIsUsed )
{
/***********************************************************************
 *
 * Description :
 *    Procedure가 Index에서 사용 중인지 확인한다.
 *
 * Implementation :
 *    SYS_INDEX_RELATED_ 메타 테이블에서 Procedure를 검색한다.
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;

    qcNameCharBuffer      sRelatedProcNameBuffer;
    mtdCharType         * sRelatedProcName = (mtdCharType *)&sRelatedProcNameBuffer;
    mtdIntegerType        sRelatedUserID;

    smiTableCursor        sCursor;
    idBool                sCursorOpened = ID_FALSE;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sProperty;
    mtcColumn           * sFstMtcColumn;
    mtcColumn           * sSndMtcColumn;

    qtc::setVarcharValue( sRelatedProcName,
                          NULL,
                          aRelatedProcName->stmtText + aRelatedProcName->offset,
                          aRelatedProcName->size );
    sRelatedUserID = (mtdIntegerType)aRelatedUserID;

    IDE_TEST( smiGetTableColumns( gQcmIndexRelated,
                                  QCM_INDEX_RELATED_RELATEDPROCNAME_COL_ORDER,
                                  (const smiColumn**)&sFstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexRelated,
                                  QCM_INDEX_RELATED_RELATEDUSERID_COL_ORDER,
                                  (const smiColumn**)&sSndMtcColumn )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sFirstMetaRange,
        &sSecondMetaRange,
        sFstMtcColumn,
        (const void *)sRelatedProcName,
        sSndMtcColumn,
        &sRelatedUserID,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sProperty, NULL );

    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gQcmIndexRelated,
                            gQcmIndexRelatedIndex[QCM_INDEX_RELATED_RELPROCNAME_RELUSERID],
                            smiGetRowSCN( gQcmIndexRelated ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sCursorOpened = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        *aIsUsed = ID_TRUE;
    }
    else
    {
        *aIsUsed = ID_FALSE;
    }

    sCursorOpened = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sCursorOpened == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}


IDE_RC qcmSetProcUserIDOfQcmProcedures(
    idvSQL     * /* aStatistics */,
    const void * aRow,
    SInt       * aUserID )
{
/*******************************************************************
 * Description : To Fix BUG-19839
 *               Procedure oid 를 사용해 소유자의 UserID를 검색
 *
 * Implementation : 
 ********************************************************************/

    mtcColumn * sUserIDMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_USERID_COL_ORDER,
                                  (const smiColumn**)&sUserIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        aRow,
        sUserIDMtcColumn,
        aUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmSetProcObjTypeOfQcmProcedures(
    idvSQL     * /* aStatistics */,
    const void * aRow,
    SInt       * aProcObjType )
{
    mtcColumn            * sObjectTypeMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_OBJECTTYP_COL_ORDER,
                                  (const smiColumn**)&sObjectTypeMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        aRow,
        sObjectTypeMtcColumn,
        aProcObjType );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::getProcUserID ( qcStatement * aStatement,
                                qsOID         aProcOID,
                                UInt        * aProcUserID )
{
/*******************************************************************
 * Description : To Fix BUG-19839
 *               Procedure oid 를 사용해 소유자의 UserID를 검색
 *
 * Implementation : 
 ********************************************************************/
    
    smiRange           sRange;
    qtcMetaRangeColumn sMetaRange;
    vSLong             sRowCount;
    SLong              sProcOID;
    mtcColumn        * sProcIDMtcColumn;

    sProcOID = QCM_OID_TO_BIGINT( aProcOID );

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    (void)qcm::makeMetaRangeSingleColumn( & sMetaRange,
                                          sProcIDMtcColumn,
                                          & sProcOID,
                                          & sRange );

    IDE_TEST( qcm::selectRow( QC_SMI_STMT( aStatement ),
                              gQcmProcedures,
                              smiGetDefaultFilter(),
                              & sRange,
                              gQcmProceduresIndex
                              [ QCM_PROCEDURES_PROCOID_IDX_ORDER ],
                              (qcmSetStructMemberFunc ) qcmSetProcUserIDOfQcmProcedures,
                              aProcUserID,
                              0,
                              1,
                              & sRowCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount != 1, err_selected_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_selected_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                   "[qcmProc::getProcUserID]"
                   "err_selected_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::getProcObjType( qcStatement * aStatement,
                                qsOID         aProcOID,
                                SInt        * aProcObjType )
{
/***********************************************************************
 *
 * Description :  PROJ-1075 TYPESET
 *                procedure oid를 통해 object type을 검색.
 *                typeset 추가로 object type에 따라
 *                ddl 수행여부를 구분하기 위함.
 * Implementation :
 *
 ***********************************************************************/

    smiRange           sRange;
    qtcMetaRangeColumn sMetaRange;
    vSLong             sRowCount;
    SLong              sProcOID;
    mtcColumn        * sProcIDMtcColumn;

    // To fix BUG-14439
    // OID를 keyrange로 사용할 때는 bigint 타입으로 사용해야 함.
    sProcOID = QCM_OID_TO_BIGINT( aProcOID );

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );
    (void)qcm::makeMetaRangeSingleColumn( & sMetaRange,
                                          sProcIDMtcColumn,
                                          & sProcOID,
                                          & sRange );

    IDE_TEST( qcm::selectRow( QC_SMI_STMT( aStatement ),
                              gQcmProcedures,
                              smiGetDefaultFilter(),
                              & sRange,
                              gQcmProceduresIndex
                              [ QCM_PROCEDURES_PROCOID_IDX_ORDER ],
                              (qcmSetStructMemberFunc ) qcmSetProcObjTypeOfQcmProcedures,
                              aProcObjType,
                              0,
                              1,
                              & sRowCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount != 1, err_selected_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_selected_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmProc::getProcObjType]"
                                "err_selected_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************
                     SetMember functions
          for more members, see qcmProc.cpp rev.1.11
**************************************************************/

IDE_RC qcmProc::procSetMember(
    idvSQL        * /* aStatistics */,
    const void    * aRow,
    qcmProcedures * aProcedures )
{
    SLong       sSLongOID;
    mtcColumn * sUserIDMtcColumn;
    mtcColumn * sProcIDMtcColumn;
    mtcColumn * sProcNameMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_USERID_COL_ORDER,
                                  (const smiColumn**)&sUserIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        aRow,
        sUserIDMtcColumn,
        & aProcedures->userID);

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sProcIDMtcColumn,
        & sSLongOID);
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    aProcedures->procOID = (qsOID)sSLongOID;

    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCNAME_COL_ORDER,
                                  (const smiColumn**)&sProcNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue (
        aRow,
        sProcNameMtcColumn,
        aProcedures->procName);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* ------------------------------------------------
 *  X$PROCTEXT
 * ----------------------------------------------*/

#define QCM_PROC_TEXT_LEN  64

typedef struct qcmProcText
{
    ULong  mProcOID;
    UInt   mPiece;
    SChar *mText;

}qcmProcText;


static iduFixedTableColDesc gPROCTEXTColDesc[] =
{
    {
        (SChar *)"PROC_OID",
        offsetof(qcmProcText, mProcOID),
        IDU_FT_SIZEOF(qcmProcText, mProcOID),
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"PIECE",
        offsetof(qcmProcText, mPiece),
        IDU_FT_SIZEOF(qcmProcText, mPiece),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"TEXT",
        offsetof(qcmProcText, mText),
        QCM_PROC_TEXT_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


/*************************************************************
            For Fixed Table
**************************************************************/

// 인자를 임시로 넘기기 위함.
typedef struct qcmTempFixedTableInfo
{
    void                  *mHeader;
    iduFixedTableMemory   *mMemory;
} qcmTempFixedTableInfo;


IDE_RC qcmProc::procSelectAllForFixedTable (
    smiStatement         * aSmiStmt,
    vSLong                 aMaxProcedureCount,
    vSLong               * aSelectedProcedureCount,
    void                 * aFixedTableInfo )
{
    vSLong selectedRowCount;

    IDE_TEST( qcm::selectRow
              (
                  aSmiStmt,
                  gQcmProcedures,
                  smiGetDefaultFilter(), /* a_callback */
                  smiGetDefaultKeyRange(), /* a_range */
                  NULL, /* a_index */
                  (qcmSetStructMemberFunc ) qcmProc::buildProcText,
                  aFixedTableInfo,
                  0, /* distance */
                  aMaxProcedureCount,
                  &selectedRowCount
                  ) != IDE_SUCCESS );

    *aSelectedProcedureCount = (vSLong) selectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmProc::buildProcText( idvSQL        * aStatistics,
                               const void    * aRow,
                               void          * aFixedTableInfo )
{
    UInt           sState = 0;
    qsOID          sOID;
    SLong          sSLongOID;
    SInt           sProcLen = 0;
    qcmProcText    sProcText;
    SChar        * sProcBuf = NULL;
    qcStatement    sStatement;
    mtcColumn    * sProcIDMtcColumn;
    // PROJ-2446
    smiTrans       sMetaTx;
    smiStatement * sDummySmiStmt;
    smiStatement   sSmiStmt;
    UInt           sProcState = 0;
    qcmTempFixedTableInfo * sFixedTableInfo;

    void         * sIndexValues[1];
    SChar        * sIndex;
    SChar        * sStartIndex;
    SChar        * sPrevIndex;
    SInt           sCurrPos = 0;
    SInt           sCurrLen = 0;
    SInt           sSeqNo   = 0;
    SChar          sParseStr [ QCM_PROC_TEXT_LEN * 2 + 2 ] = { 0, };
         
    const mtlModule* sModule;

    IDE_TEST(sMetaTx.initialize() != IDE_SUCCESS);
    sProcState = 1;

    IDE_TEST(sMetaTx.begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_TRANSACTION_UNTOUCHABLE |
                            SMI_ISOLATION_CONSISTENT    |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sProcState = 2;

    IDE_TEST(sSmiStmt.begin( aStatistics,
                             sDummySmiStmt,
                             SMI_STATEMENT_UNTOUCHABLE |
                             SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);
    sProcState = 3;

    /* ------------------------------------------------
     *  Get Proc OID
     * ----------------------------------------------*/
    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sProcIDMtcColumn,
        & sSLongOID);
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    sOID = (qsOID)sSLongOID;
    sIndexValues[0] = &sOID;
    sFixedTableInfo = (qcmTempFixedTableInfo *)aFixedTableInfo;

    /* BUG-43006 FixedTable Indexing Filter
     * Column Index 를 사용해서 전체 Record를 생성하지않고
     * 부분만 생성해 Filtering 한다.
     * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
     * 해당하는 값을 순서대로 넣어주어야 한다.
     * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
     * 어 주어야한다.
     */
    if ( iduFixedTable::checkKeyRange( sFixedTableInfo->mMemory,
                                       gPROCTEXTColDesc,
                                       sIndexValues )
         == ID_TRUE )
    {
        /* ------------------------------------------------
         *  Generate Proc Text
         * ----------------------------------------------*/

        if (qsxProc::latchS( sOID ) == IDE_SUCCESS)
        {
            sState = 1;

            IDE_TEST( qcg::allocStatement( & sStatement,
                                           NULL,
                                           NULL,
                                           NULL )
                      != IDE_SUCCESS);

            sState = 2;

            sProcBuf = NULL; // allocate inside
            IDE_TEST(qsxProc::getProcText(&sStatement,
                                          sOID,
                                          &sProcBuf,
                                          &sProcLen)
                     != IDE_SUCCESS);

            sProcText.mProcOID = sOID;

            // BUG-44978
            sModule     = mtl::mDBCharSet;
            sStartIndex = sProcBuf;
            sIndex      = sStartIndex;

            while(1)
            {
                sPrevIndex = sIndex;

                (void)sModule->nextCharPtr( (UChar**) &sIndex,
                                            (UChar*) ( sProcBuf + sProcLen ));

                if (( sProcBuf + sProcLen ) <= sIndex )
                {
                    // 끝까지 간 경우.
                    // 기록을 한 후 break.
                    sSeqNo++;

                    sCurrPos = sStartIndex - sProcBuf;
                    sCurrLen = sIndex - sStartIndex;

                    qcg::prsCopyStrDupAppo( sParseStr,
                                            sProcBuf + sCurrPos,
                                            sCurrLen );

                    sProcText.mPiece = sSeqNo;
                    sProcText.mText  = sParseStr;

                    IDE_TEST(iduFixedTable::buildRecord(((qcmTempFixedTableInfo *)aFixedTableInfo)->mHeader,
                                                        ((qcmTempFixedTableInfo *)aFixedTableInfo)->mMemory,
                                                        (void *) &sProcText)
                             != IDE_SUCCESS);
                    break;
                }
                else
                {
                    if( sIndex - sStartIndex >= QCM_PROC_TEXT_LEN )
                    {
                        // 아직 끝가지 안 갔고, 읽다보니 64바이트 또는 초과한 값이
                        // 되었을 때 잘라서 기록
                        sCurrPos = sStartIndex - sProcBuf;
                
                        if( sIndex - sStartIndex == QCM_PROC_TEXT_LEN )
                        {
                            // 딱 떨어지는 경우
                            sCurrLen = QCM_PROC_TEXT_LEN;
                            sStartIndex = sIndex;
                        }
                        else
                        {
                            // 삐져나간 경우 그 이전 캐릭터 위치까지 기록
                            sCurrLen = sPrevIndex - sStartIndex;
                            sStartIndex = sPrevIndex;
                        }

                        sSeqNo++;

                        qcg::prsCopyStrDupAppo( sParseStr,
                                                sProcBuf + sCurrPos,
                                                sCurrLen );

                        sProcText.mPiece  = sSeqNo;
                        sProcText.mText   = sParseStr;

                        IDE_TEST(iduFixedTable::buildRecord(((qcmTempFixedTableInfo *)aFixedTableInfo)->mHeader,
                                                            ((qcmTempFixedTableInfo *)aFixedTableInfo)->mMemory,
                                                            (void *) &sProcText)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        
            sState = 1;

            IDE_TEST(qcg::freeStatement(&sStatement) != IDE_SUCCESS);

            sState = 0;

            IDE_TEST(qsxProc::unlatch( sOID )
                     != IDE_SUCCESS);
        }
        else
        {
#if defined(DEBUG)
            ideLog::log(IDE_QP_0, "Latch Error \n");
#endif
        }
    }
    else
    {
        /* Nothing to do */
    }

    sProcState = 2;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sProcState = 1;
    IDE_TEST(sMetaTx.commit() != IDE_SUCCESS);

    sProcState = 0;
    IDE_TEST(sMetaTx.destroy( aStatistics ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 2:
            IDE_ASSERT(qcg::freeStatement(&sStatement) == IDE_SUCCESS);
        case 1:
            /* PROJ-2446 ONE SOURCE XDB BUGBUG statement pointer를 넘겨야한다. */
            IDE_ASSERT(qsxProc::unlatch( sOID )
                       == IDE_SUCCESS);
        case 0:
            //nothing
            break;
        default:
            IDE_CALLBACK_FATAL("Can't be here");
    }

    switch(sProcState)
    {
        case 3:
            IDE_ASSERT(sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);

        case 2:
            IDE_ASSERT(sMetaTx.commit() == IDE_SUCCESS);

        case 1:
            IDE_ASSERT(sMetaTx.destroy( aStatistics ) == IDE_SUCCESS);

        case 0:
            //nothing
            break;
        default:
            IDE_CALLBACK_FATAL("Can't be here");
    }

    return IDE_FAILURE ;
}

IDE_RC qcmProc::buildRecordForPROCTEXT( idvSQL      * aStatistics,
                                        void        * aHeader,
                                        void        * /* aDumpObj */,
                                        iduFixedTableMemory   *aMemory)
{
    smiTrans               sMetaTx;
    smiStatement          *sDummySmiStmt;
    smiStatement           sStatement;
    vSLong                 sRecCount;
    qcmTempFixedTableInfo  sInfo;
    UInt                   sState = 0;

    sInfo.mHeader = aHeader;
    sInfo.mMemory = aMemory;

    /* ------------------------------------------------
     *  Build Text Record
     * ----------------------------------------------*/

    IDE_TEST(sMetaTx.initialize() != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(sMetaTx.begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_TRANSACTION_UNTOUCHABLE |
                            SMI_ISOLATION_CONSISTENT    |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sState = 2;

    IDE_TEST(sStatement.begin( aStatistics,
                               sDummySmiStmt,
                               SMI_STATEMENT_UNTOUCHABLE |
                               SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);
    sState = 3;


    IDE_TEST(procSelectAllForFixedTable(&sStatement,
                                        ID_UINT_MAX,
                                        &sRecCount,
                                        &sInfo) != IDE_SUCCESS);

    sState = 2;
    IDE_TEST(sStatement.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sState = 1;
    IDE_TEST(sMetaTx.commit() != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(sMetaTx.destroy( aStatistics ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch(sState)
        {
            case 3:
                IDE_ASSERT(sStatement.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);

            case 2:
                IDE_ASSERT(sMetaTx.commit() == IDE_SUCCESS);

            case 1:
                IDE_ASSERT(sMetaTx.destroy( aStatistics ) == IDE_SUCCESS);

            case 0:
                //nothing
                break;
            default:
                IDE_CALLBACK_FATAL("Can't be here");
        }
        return IDE_FAILURE;
    }
}


iduFixedTableDesc gPROCTEXTTableDesc =
{
    (SChar *)"X$PROCTEXT",
    qcmProc::buildRecordForPROCTEXT,
    gPROCTEXTColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


typedef struct qcmProcInfo
{
    ULong mProcOID;
    UInt  mModifyCount;
    UInt  mStatus;
    UInt  mSessionID;
    UInt  mProcType;
    UInt  mShardSplitMethod; // TASK-7244 Set shard split method to PSM info
}qcmProcInfo;

// PROJ-2717 Internal procedure
//   X$PROCINFO
//     PROC_OID
//     MODIFY_COUNT
//     Status [VALID(0), INVALID(1)]
//     SESSION_ID
//     PROC_TYPE [NORMAL(0), EXTERNAL_C(1), INTERNAL_C(2)]
//     SHARD_SPLIT_METHOD [NONE(0), HASH(1), RANGE(2), LIST(3), CLONE(4), SOLO(5)]

static iduFixedTableColDesc gPROCInfoColDesc[] =
{
    {
        (SChar *)"PROC_OID",
        offsetof(qcmProcInfo, mProcOID),
        IDU_FT_SIZEOF(qcmProcInfo, mProcOID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"MODIFY_COUNT",
        offsetof(qcmProcInfo, mModifyCount),
        IDU_FT_SIZEOF(qcmProcInfo, mModifyCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"STATUS",
        offsetof(qcmProcInfo, mStatus),
        IDU_FT_SIZEOF(qcmProcInfo, mStatus),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"SESSION_ID",
        offsetof(qcmProcInfo, mSessionID),
        IDU_FT_SIZEOF(qcmProcInfo, mSessionID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"PROC_TYPE",
        offsetof(qcmProcInfo, mProcType),
        IDU_FT_SIZEOF(qcmProcInfo, mProcType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    // TASK-7244 Set shard split method to PSM info
    {
        (SChar *)"SHARD_SPLIT_METHOD",
        offsetof(qcmProcInfo, mShardSplitMethod),
        IDU_FT_SIZEOF(qcmProcInfo, mShardSplitMethod),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc gPROCInfoDesc =
{
    (SChar *)"X$PROCINFO",
    qcmProc::buildRecordForPROCInfo,
    gPROCInfoColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



IDE_RC qcmProc::buildRecordForPROCInfo( idvSQL      * aStatistics,
                                        void        * aHeader,
                                        void        * /* aDumpObj */,
                                        iduFixedTableMemory   *aMemory)
{
    smiTrans               sMetaTx;
    smiStatement          *sDummySmiStmt;
    smiStatement           sStatement;
    vSLong                 sRecCount;
    qcmTempFixedTableInfo  sInfo;
    UInt                   sState = 0;

    sInfo.mHeader = aHeader;
    sInfo.mMemory = aMemory;

    /* ------------------------------------------------
     *  Build Info Record
     * ----------------------------------------------*/

    IDE_TEST(sMetaTx.initialize( ) != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(sMetaTx.begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_TRANSACTION_UNTOUCHABLE |
                            SMI_ISOLATION_CONSISTENT    |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sState = 2;

    IDE_TEST(sStatement.begin( aStatistics,
                               sDummySmiStmt,
                               SMI_STATEMENT_UNTOUCHABLE |
                               SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);
    sState = 3;


    IDE_TEST(procSelectAllForBuildProcInfo(&sStatement,
                                           ID_UINT_MAX,
                                           &sRecCount,
                                           &sInfo) != IDE_SUCCESS);

    sState = 2;
    IDE_TEST(sStatement.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sState = 1;
    IDE_TEST(sMetaTx.commit() != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(sMetaTx.destroy( aStatistics ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch(sState)
        {
            case 3:
                IDE_ASSERT(sStatement.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);

            case 2:
                IDE_ASSERT(sMetaTx.commit() == IDE_SUCCESS);

            case 1:
                IDE_ASSERT(sMetaTx.destroy( aStatistics ) == IDE_SUCCESS);

            case 0:
                //nothing
                break;
            default:
                IDE_CALLBACK_FATAL("Can't be here");
        }
        return IDE_FAILURE;
    }
}


IDE_RC qcmProc::procSelectAllForBuildProcInfo(
    smiStatement         * aSmiStmt,
    vSLong                 aMaxProcedureCount,
    vSLong               * aSelectedProcedureCount,
    void                 * aFixedTableInfo )
{
    vSLong selectedRowCount;

    IDE_TEST( qcm::selectRow
              (
                  aSmiStmt,
                  gQcmProcedures,
                  smiGetDefaultFilter(), /* a_callback */
                  smiGetDefaultKeyRange(), /* a_range */
                  NULL, /* a_index */
                  (qcmSetStructMemberFunc ) qcmProc::buildProcInfo,
                  aFixedTableInfo,
                  0, /* distance */
                  aMaxProcedureCount,
                  &selectedRowCount
                  ) != IDE_SUCCESS );

    *aSelectedProcedureCount = (vSLong) selectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmProc::buildProcInfo( idvSQL        * aStatistics,
                               const void    * aRow,
                               void          * aFixedTableInfo )
{
    UInt           sState = 0;
    qsOID          sOID;
    SLong          sSLongOID;
    mtcColumn    * sProcIDMtcColumn;
    // PROJ-2446
    smiTrans       sMetaTx;
    smiStatement * sDummySmiStmt;
    smiStatement   sSmiStmt;
    UInt           sProcState = 0;
    qsxProcInfo  * sProcInfo;
    qcmProcInfo    sQcmProcInfo;
         
    IDE_TEST(sMetaTx.initialize( ) != IDE_SUCCESS);
    sProcState = 1;

    IDE_TEST(sMetaTx.begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_TRANSACTION_UNTOUCHABLE |
                            SMI_ISOLATION_CONSISTENT    |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sProcState = 2;

    IDE_TEST(sSmiStmt.begin( aStatistics,
                             sDummySmiStmt,
                             SMI_STATEMENT_UNTOUCHABLE |
                             SMI_STATEMENT_MEMORY_CURSOR)
             != IDE_SUCCESS);
    sProcState = 3;

    /* ------------------------------------------------
     *  Get Proc OID
     * ----------------------------------------------*/
    IDE_TEST( smiGetTableColumns( gQcmProcedures,
                                  QCM_PROCEDURES_PROCOID_COL_ORDER,
                                  (const smiColumn**)&sProcIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sProcIDMtcColumn,
        & sSLongOID);
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    sOID = (qsOID)sSLongOID;

    /* ------------------------------------------------
     *  Generate Proc Info 
     * ----------------------------------------------*/

    if (qsxProc::latchS( sOID ) == IDE_SUCCESS)
    {
        sState = 1;

        IDE_TEST(qsxProc::getProcInfo( sOID,
                                       &sProcInfo)
                 != IDE_SUCCESS);

        sQcmProcInfo.mProcOID     = sProcInfo->procOID;
        sQcmProcInfo.mModifyCount = sProcInfo->modifyCount;
        sQcmProcInfo.mStatus      = (sProcInfo->isValid == ID_TRUE)?0:1;
        sQcmProcInfo.mSessionID   = sProcInfo->sessionID;
        // TASK-7244 Set shard split method to PSM info
        sQcmProcInfo.mShardSplitMethod = sProcInfo->shardSplitMethod;

        if ( sProcInfo->planTree!= NULL )
        {
            sQcmProcInfo.mProcType = sProcInfo->planTree->procType;
        }
        else
        {
            sQcmProcInfo.mProcType = ID_UINT_MAX;
        }

        IDE_TEST(iduFixedTable::buildRecord(((qcmTempFixedTableInfo *)aFixedTableInfo)->mHeader,
                                            ((qcmTempFixedTableInfo *)aFixedTableInfo)->mMemory,
                                            (void *) &sQcmProcInfo)
                 != IDE_SUCCESS);

        sState = 0;
        IDE_TEST(qsxProc::unlatch( sOID )
                 != IDE_SUCCESS);
    }
    else
    {
#if defined(DEBUG)
        ideLog::log(IDE_QP_0, "Latch Error \n");
#endif
    }

    sProcState = 2;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sProcState = 1;
    IDE_TEST(sMetaTx.commit() != IDE_SUCCESS);

    sProcState = 0;
    IDE_TEST(sMetaTx.destroy( aStatistics ) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 1:
            /* PROJ-2446 ONE SOURCE XDB BUGBUG statement pointer를 넘겨야한다. */
            IDE_ASSERT(qsxProc::unlatch( sOID )
                       == IDE_SUCCESS);
        case 0:
            //nothing
            break;
        default:
            IDE_CALLBACK_FATAL("Can't be here");
    }

    switch(sProcState)
    {
        case 3:
            IDE_ASSERT(sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);

        case 2:
            IDE_ASSERT(sMetaTx.commit() == IDE_SUCCESS);

        case 1:
            IDE_ASSERT(sMetaTx.destroy( aStatistics ) == IDE_SUCCESS);

        case 0:
            //nothing
            break;
        default:
            IDE_CALLBACK_FATAL("Can't be here");
    }

    return IDE_FAILURE ;
}

