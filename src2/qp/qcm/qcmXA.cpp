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
 * $Id: qcmXA.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <ida.h>
#include <qcm.h>
#include <qcmXA.h>
#include <qcg.h>
#include <qdParseTree.h>

const void * gQcmXaHeuristicTrans;
const void * gQcmXaHeuristicTransIndex [ QCM_MAX_META_INDICES ];

IDE_RC qcmXA::insert(
    smiStatement    * aSmiStmt,
    /* BUG-18981 */
    ID_XID          * aXid,
    SInt              aStatus )
{
    qcmXaHeuristicTrans sXaHeuristicTrans;

    idlOS::memset( sXaHeuristicTrans.globalTxId,
                   0,
                   QCM_XID_FIELD_STRING_SIZE + 1 );

    idlOS::memset( sXaHeuristicTrans.branchQualifier,
                   0,
                   QCM_XID_FIELD_STRING_SIZE + 1 );

    /* bug-36037: invalid xid
       만약 invalid xid가 존재하여 heuristic rollback 하더라도
       view에 남지 않게 해야 한다. remove가 안되므로 */
    IDE_TEST(idaXaXidToString( aXid,
                 & sXaHeuristicTrans.formatId,
                 sXaHeuristicTrans.globalTxId,
                 sXaHeuristicTrans.branchQualifier ) != IDE_SUCCESS);

    sXaHeuristicTrans.status = aStatus;

    IDE_TEST( insert( aSmiStmt,
                      sXaHeuristicTrans ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmXA::remove(
    smiStatement    * aSmiStmt,
    /* BUG-18981 */
    ID_XID          * aXid,
    idBool          * aIsRemoved )
{
    vULong sFormatId;
    SChar  sGlobalTxId      [QCM_XID_FIELD_STRING_SIZE + 1];
    SChar  sBranchQualifier [QCM_XID_FIELD_STRING_SIZE + 1];

    idlOS::memset( sGlobalTxId,
                   0,
                   QCM_XID_FIELD_STRING_SIZE + 1);

    idlOS::memset( sBranchQualifier,
                   0,
                   QCM_XID_FIELD_STRING_SIZE + 1);

    idaXaXidToString( aXid,
                 &sFormatId,
                 sGlobalTxId,
                 sBranchQualifier );

    IDE_TEST( remove( aSmiStmt,
                      sFormatId,
                      sGlobalTxId,
                      sBranchQualifier,
                      aIsRemoved) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC qcmXA::select(
    smiStatement            * aSmiStmt,
    /* BUG-18981 */
    ID_XID                  * aXid,
    idBool                  * aIsFound,
    qcmXaHeuristicTrans  & aQcmXaHeuristicTrans)
{
    vULong sFormatId;
    SChar sGlobalTxId      [QCM_XID_FIELD_STRING_SIZE + 1];
    SChar sBranchQualifier [QCM_XID_FIELD_STRING_SIZE + 1];

    idlOS::memset( sGlobalTxId,
            0,
            QCM_XID_FIELD_STRING_SIZE + 1);

    idlOS::memset( sBranchQualifier,
            0,
            QCM_XID_FIELD_STRING_SIZE + 1);

    idaXaXidToString( aXid,
                 &sFormatId,
                 sGlobalTxId,
                 sBranchQualifier );

    IDE_TEST( select( aSmiStmt,
                      sFormatId,
                      sGlobalTxId,
                      sBranchQualifier,
                      aIsFound,
                      aQcmXaHeuristicTrans ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/*------------------ insert -----------------------------*/
/*
  NAME
  insert -

  DESCRIPTION

  ARGUMENTS

  RETURNS

  NOTES
*/
IDE_RC qcmXA::insert(
    smiStatement    * aSmiStmt,
    qcmXaHeuristicTrans  & aXaHeuristicTrans)
{
    vSLong        sRowCnt;
    SChar         sBuffer[ 400 ];

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_XA_HEURISTIC_TRANS_ VALUES ( "
                     QCM_SQL_BIGINT_FMT", "   // 1  FORMAT_ID
                     QCM_SQL_CHAR_FMT", "     // 2  GLOBAL_TX_ID
                     QCM_SQL_CHAR_FMT", "     // 3  BRANCH_QUALIFIER
                     QCM_SQL_INT32_FMT", "    // 4  STATUS
                     "SYSTIMESTAMP) ",        // 5  OCCUR_TIME
                     QCM_VULONG_TO_BIGINT( aXaHeuristicTrans.formatId ),
                     aXaHeuristicTrans.globalTxId,
                     aXaHeuristicTrans.branchQualifier,
                     (SInt) aXaHeuristicTrans.status );

    IDE_TEST( qcg::runDMLforDDL( aSmiStmt,
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_inserted_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_inserted_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmXA::insert] err_inserted_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*------------------ remove -----------------------------*/
/*
  NAME
  remove -

  DESCRIPTION

  ARGUMENTS
  a_procsName  (IN) - proc name

  RETURNS

  NOTES
*/
IDE_RC qcmXA::remove(
    smiStatement    * aSmiStmt,
    vULong            aFormatId,
    SChar           * aGlobalTxId,
    SChar           * aBranchQualifier,
    idBool          * aIsRemoved )
{
    vSLong        sRowCnt;
    SChar         sBuffer[ 400 ];

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_XA_HEURISTIC_TRANS_ "
                     " WHERE FORMAT_ID = "QCM_SQL_BIGINT_FMT
                     " AND GLOBAL_TX_ID = "QCM_SQL_CHAR_FMT
                     " AND BRANCH_QUALIFIER = "QCM_SQL_CHAR_FMT,
                     QCM_VULONG_TO_BIGINT( aFormatId ),
                     aGlobalTxId,
                     aBranchQualifier );

    IDE_TEST( qcg::runDMLforDDL( aSmiStmt,
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt > 1, err_removed_count_gt_1 );

    *aIsRemoved = ( sRowCnt == 1 ) ? ID_TRUE : ID_FALSE ;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_removed_count_gt_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmXA::remove]"
                                "err_removed_count_gt_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*------------------ select -----------------------------*/
/*
  NAME
  select -

  DESCRIPTION

  ARGUMENTS
  a_procname  (IN)   --
  o_infoRec    (OUT)  --

  RETURNS
  rc

  NOTES
*/
IDE_RC qcmXA::select(
    smiStatement            * aSmiStmt,
    vULong                    aFormatId,
    SChar                   * aGlobalTxId,
    SChar                   * aBranchQualifier,
    idBool                  * aIsFound,
    qcmXaHeuristicTrans     & aInfoRec )
{
    smiRange             sRange;
    qtcMetaRangeColumn   sFirstMetaRange;
    qtcMetaRangeColumn   sSecondMetaRange;
    qtcMetaRangeColumn   sThirdMetaRange;


    vSLong               sSelectedRowCount;
    mtdBigintType        sFormatIdData;
    UChar                sGlobalTxIdBuffer[ QCM_XID_FIELD_STRING_SIZE + 2]
        = {0,};
    UChar                sBranchQualifierBuffer[ QCM_XID_FIELD_STRING_SIZE + 2]
        = {0,};
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;
    mtcColumn           * sThirdMtcColumn;

    *aIsFound = ID_FALSE;

    sFormatIdData = ( mtdBigintType ) aFormatId ;

    qtc::setVarcharValue( (mtdCharType*) sGlobalTxIdBuffer,
                          NULL,
                          aGlobalTxId,
                          idlOS::strlen(aGlobalTxId) );

    qtc::setVarcharValue( (mtdCharType*) sBranchQualifierBuffer,
                          NULL,
                          aBranchQualifier,
                          idlOS::strlen(aBranchQualifier) );

    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_FORMAT_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_GLOBAL_TX_ID,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_BRANCH_QUALIFIER,
                                  (const smiColumn**)&sThirdMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeTripleColumn(
        & sFirstMetaRange,
        & sSecondMetaRange,
        & sThirdMetaRange,
        sFirstMtcColumn,
        & sFormatIdData,
        sSceondMtcColumn,
        (const void*) sGlobalTxIdBuffer,
        sThirdMtcColumn,
        (const void*) sBranchQualifierBuffer,
        & sRange );

    IDE_TEST(qcm::selectRow (
                 aSmiStmt,
                 gQcmXaHeuristicTrans,
                 smiGetDefaultFilter(), /* smiCallBack */
                 &sRange ,
                 gQcmXaHeuristicTransIndex[ QCM_XA_HEURISTIC_TRANS_FORMATID_GLOBALTXID_BRANCHQUALIFIER_IDX_ORDER ] ,
                 (qcmSetStructMemberFunc ) qcmXA::setMember,
                 &aInfoRec,
                 0, /* a_metaStructDistance */
                 1, /* a_metaStructMaxCount */
                 & sSelectedRowCount
                 ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sSelectedRowCount > 1, err_too_many_rows);

    (*aIsFound) = ( sSelectedRowCount == 1 ) ? ID_TRUE : ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_many_rows);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmXA::select] err_too_many_rows" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmXA::selectAll(
    smiStatement           * aSmiStmt,
    SInt                   * aNumHeuristicTrans,
    qcmXaHeuristicTrans    * aHeuristicTrans,
    SInt                     aMaxHeuristicTrans )
{
    vSLong  sSelectedRowCount;

    IDE_TEST( qcm::selectRow (
                  aSmiStmt,
                  gQcmXaHeuristicTrans,
                  smiGetDefaultFilter(), /* a_callback */
                  smiGetDefaultKeyRange(), /* a_range */
                  NULL, /* a_index */
                  (qcmSetStructMemberFunc) qcmXA::setMember,
                  aHeuristicTrans,
                  ID_SIZEOF(qcmXaHeuristicTrans), /* distance */
                  aMaxHeuristicTrans,
                  & sSelectedRowCount
                  ) != IDE_SUCCESS );

    *aNumHeuristicTrans = (SInt) sSelectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmXA::setMember(
    idvSQL              * /* aStatistics */,
    const void          * aRow,
    qcmXaHeuristicTrans * aXaHeuristicTrans )
{
    SLong       sSLongID;
    mtcColumn * sTramsFormatIDMtcColumn;
    mtcColumn * sTramsGlobalTxIDMtcColumn;
    mtcColumn * sTramsBranchQaulMtcColumn;
    mtcColumn * sTramsStatusMtcColumn;

    // PROJ-1362 LOB support.
    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_FORMAT_ID,
                                  (const smiColumn**)&sTramsFormatIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue (
        aRow,
        sTramsFormatIDMtcColumn,
        & sSLongID );
    // BUGBUG 32bit machine에서 동작 시 SLong(64bit)변수를 uVLong(32bit)변수로
    // 변환하므로 데이터 손실 가능성 있음
    aXaHeuristicTrans->formatId = (vULong)sSLongID;

    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_GLOBAL_TX_ID,
                                  (const smiColumn**)&sTramsGlobalTxIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue (
        aRow,
        sTramsGlobalTxIDMtcColumn,
        aXaHeuristicTrans->globalTxId );

    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_BRANCH_QUALIFIER,
                                  (const smiColumn**)&sTramsBranchQaulMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue (
        aRow,
        sTramsBranchQaulMtcColumn,
        aXaHeuristicTrans->branchQualifier );

    IDE_TEST( smiGetTableColumns( gQcmXaHeuristicTrans,
                                  QCM_XA_HEURISTIC_TRANS_STATUS,
                                  (const smiColumn**)&sTramsStatusMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue (
        aRow,
        sTramsStatusMtcColumn,
        & aXaHeuristicTrans->status );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

