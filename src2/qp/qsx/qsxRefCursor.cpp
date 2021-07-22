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
 * $Id: qsxRefCursor.cpp 23549 2007-09-21 00:58:54Z orc $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <qcuError.h>
#include <qsxRefCursor.h>
#include <qcd.h>
#include <qsvEnv.h>

IDE_RC
qsxRefCursor::initialize( qsxRefCursorInfo * aRefCurInfo,
                          UShort             aId )
{
/***********************************************************************
 *
 *  Description : Reference Cursor 초기화
 *
 *  Implementation : qsxRefCursorInfo구조체의 멤버를 모두 초기화함
 *
 ***********************************************************************/

    aRefCurInfo->rowCount        = 0;
    aRefCurInfo->rowCountIsNull  = ID_TRUE;
    aRefCurInfo->isOpen          = ID_FALSE;
    aRefCurInfo->isEndOfCursor   = ID_FALSE;
    aRefCurInfo->isNeedStmtFree  = ID_FALSE; // BUG-38767
    aRefCurInfo->nextRecordExist = ID_FALSE;
    aRefCurInfo->id              = aId;
    
    qcd::initStmt( &aRefCurInfo->hStmt );

    return IDE_SUCCESS;
}

IDE_RC
qsxRefCursor::finalize( qsxRefCursorInfo * aRefCurInfo )
{
/***********************************************************************
 *
 *  Description : Reference Cursor finalize
 *
 *  Implementation : initialize와 거의동일하나, id를 0으로 초기화
 *
 ***********************************************************************/

    aRefCurInfo->rowCount = 0;
    aRefCurInfo->rowCountIsNull = ID_TRUE;
    aRefCurInfo->isOpen = ID_FALSE;
    aRefCurInfo->isEndOfCursor = ID_FALSE;
    aRefCurInfo->nextRecordExist = ID_FALSE;

    // BUG-38767
    if ( aRefCurInfo->isNeedStmtFree == ID_TRUE )
    {
        IDE_TEST( qcd::freeStmt( aRefCurInfo->hStmt,
                                 ID_TRUE )
                  != IDE_SUCCESS );
        aRefCurInfo->isNeedStmtFree = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    qcd::initStmt( &aRefCurInfo->hStmt );
    aRefCurInfo->id = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxRefCursor::openFor( qsxRefCursorInfo * aRefCurInfo,
                       qcStatement      * aQcStmt,
                       qsUsingParam     * aUsingParams,
                       UInt               aUsingParamCount,
                       SChar            * aQueryString,
                       UInt               aQueryLen,
                       UInt               aSqlIdx )
{
/***********************************************************************
 *
 *  Description : 주어진 query string과 parameter로 prepare-execute
 *
 *  Implementation :
 *             (1) statement 할당
 *             (2) prepare후, select인지 검사
 *             (3) parameter info bind
 *             (4) parameter data bind
 *             (5) execute
 *             (6) execute시 한건 fetch를 내부적으로 시도해보기 때문에
 *                 record존재하는지 저장
 *
 ***********************************************************************/

    qsUsingParam   * sUsingParam;
    UShort           sBindParamId = 0;
    mtcColumn      * sMtcColumn;
    void           * sValue;
    UInt             sValueSize;
    idBool           sResultSetExist;
    qciStmtType      sStmtType;
    UInt             sPoolCount = 0;
    vSLong           sAffectedRowCount;
    qcStatement    * sExecQcStmt   = NULL;
    qsxStmtList    * sStmtList  = aQcStmt->spvEnv->mStmtList;
    qsxStmtList2   * sStmtList2 = aQcStmt->spvEnv->mStmtList2;

    if( aRefCurInfo->hStmt == NULL )
    {
        // BUG-38767
        if ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )
        {
            // BUG-43158 Enhance statement list caching in PSM
            IDE_ERROR( aSqlIdx != ID_UINT_MAX );

            IDU_FIT_POINT_RAISE( "qsxRefCursor::openFor::is_unused", err_sql_index );
            sPoolCount = aQcStmt->session->mQPSpecific.mStmtListInfo.mStmtPoolCount;
            IDE_TEST_RAISE( aSqlIdx >= sPoolCount, err_sql_index );

            if ( sStmtList != NULL )
            {
                if ( QSX_STMT_LIST_IS_UNUSED( sStmtList->mStmtPoolStatus, aSqlIdx )
                     == ID_TRUE )
                {
                    IDE_TEST( qcd::allocStmt( aQcStmt,
                                              &aRefCurInfo->hStmt )
                              != IDE_SUCCESS );

                    aRefCurInfo->isNeedStmtFree = ID_TRUE;

                    sStmtList->mStmtPool[aSqlIdx] = aRefCurInfo->hStmt;
                }
                else
                {
                    aRefCurInfo->hStmt = (void*)sStmtList->mStmtPool[aSqlIdx];
                    IDE_TEST( qcd::freeStmt( aRefCurInfo->hStmt, ID_FALSE )
                              != IDE_SUCCESS );
                }
            }

            if ( sStmtList2 != NULL )
            {
                if ( QSX_STMT_LIST_IS_UNUSED( sStmtList2->mStmtPoolStatus, aSqlIdx )
                     == ID_TRUE )
                {
                    IDE_TEST( qcd::allocStmt( aQcStmt,
                                              &aRefCurInfo->hStmt )
                              != IDE_SUCCESS );

                    aRefCurInfo->isNeedStmtFree = ID_TRUE;

                    sStmtList2->mStmtPool[aSqlIdx] = aRefCurInfo->hStmt;
                }
                else
                {
                    aRefCurInfo->hStmt = (void*)sStmtList2->mStmtPool[aSqlIdx];
                    IDE_TEST( qcd::freeStmt( aRefCurInfo->hStmt, ID_FALSE )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            IDE_TEST( qcd::allocStmt( aQcStmt,
                                      &aRefCurInfo->hStmt )
                      != IDE_SUCCESS );
            aRefCurInfo->isNeedStmtFree = ID_TRUE;
        }
    }
    else
    {
        // only close cursor
        IDE_TEST( qcd::freeStmt( aRefCurInfo->hStmt,
                                 ID_FALSE )
                  != IDE_SUCCESS );
    }

    aRefCurInfo->rowCount       = 0;
    aRefCurInfo->rowCountIsNull = ID_FALSE;
    aRefCurInfo->isOpen         = ID_FALSE;

    IDE_TEST( qcd::getQcStmt( aRefCurInfo->hStmt,
                              &sExecQcStmt )
              != IDE_SUCCESS );

    IDE_TEST( qcd::prepare( aRefCurInfo->hStmt,
                            aQcStmt,
                            sExecQcStmt,
                            &sStmtType,
                            aQueryString,
                            aQueryLen,
                            ID_TRUE )
              != IDE_SUCCESS );

    if ( ( ( sStmtList  != NULL ) || ( sStmtList2 != NULL ) ) &&
         ( aRefCurInfo->isNeedStmtFree == ID_TRUE ) )
    {
        IDU_FIT_POINT_RAISE( "qsxRefCursor::openFor::set_used", err_sql_index );
        IDE_TEST_RAISE( aSqlIdx >= sPoolCount, err_sql_index );
        aRefCurInfo->isNeedStmtFree = ID_FALSE;
        if ( sStmtList != NULL )
        {
            QSX_STMT_LIST_SET_USED( sStmtList->mStmtPoolStatus, aSqlIdx );
        }
        if ( sStmtList2 != NULL )
        {
            QSX_STMT_LIST_SET_USED( sStmtList2->mStmtPoolStatus, aSqlIdx );
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST_RAISE( ( sStmtType != QCI_STMT_SELECT ) &&
                    ( sStmtType != QCI_STMT_SELECT_FOR_FIXED_TABLE ),
                    err_not_query );

    IDE_TEST( qcd::checkBindParamCount( aRefCurInfo->hStmt,
                                        (UShort)aUsingParamCount )
              != IDE_SUCCESS );

    for( sUsingParam = aUsingParams;
         sUsingParam != NULL;
         sUsingParam = sUsingParam->next )
    {
        sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                      sUsingParam->paramNode );

        IDE_TEST( qcd::bindParamInfoSet( aRefCurInfo->hStmt,
                                         sMtcColumn,
                                         sBindParamId,
                                         sUsingParam->inOutType )
                  != IDE_SUCCESS );

        sBindParamId++;
    }

    sBindParamId = 0;

    for( sUsingParam = aUsingParams;
         sUsingParam != NULL;
         sUsingParam = sUsingParam->next )
    {
        IDE_TEST( qtc::calculate( sUsingParam->paramNode,
                                  QC_PRIVATE_TMPLATE(aQcStmt) )
                  != IDE_SUCCESS );

        sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                      sUsingParam->paramNode );

        sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

        sValueSize = sMtcColumn->module->actualSize(
            sMtcColumn,
            sValue );

        IDE_TEST( qcd::bindParamData( aRefCurInfo->hStmt,
                                      sValue,
                                      sValueSize,
                                      sBindParamId,
                                      aQcStmt->qmxMem,
                                      NULL,
                                      sUsingParam->inOutType )
                  != IDE_SUCCESS );

        sBindParamId++;
    }

    IDE_TEST( qcd::execute( aRefCurInfo->hStmt,
                            aQcStmt,
                            NULL,
                            &sAffectedRowCount,
                            &sResultSetExist,
                            &aRefCurInfo->nextRecordExist,
                            ID_TRUE )
              != IDE_SUCCESS );

    aRefCurInfo->isOpen = ID_TRUE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_not_query);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NOT_QUERY));
    }
    IDE_EXCEPTION(err_sql_index)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                 "[qsxRefCursor::openFor_sql_index_is_wrong]"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxRefCursor::close( qsxRefCursorInfo * aRefCurInfo )
{
/***********************************************************************
 *
 *  Description : close ref cursor
 *
 *  Implementation : cursor에 달려 있는 statement를 free하고,
 *                   변수 초기화
 *
 ***********************************************************************/

    IDE_TEST_RAISE(aRefCurInfo->isOpen != ID_TRUE, err_invalid_cursor);

    // only close cursor
    IDE_TEST( qcd::freeStmt( aRefCurInfo->hStmt,
                             ID_FALSE )
              != IDE_SUCCESS );

    aRefCurInfo->rowCount       = 0;
    aRefCurInfo->rowCountIsNull = ID_TRUE;
    aRefCurInfo->isOpen         = ID_FALSE;
    aRefCurInfo->isEndOfCursor  = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_cursor);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INVALID_CURSOR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsxRefCursor::fetchInto( qsxRefCursorInfo  * aRefCurInfo,
                         qcStatement       * aQcStmt,
                         qsProcStmtFetch   * aFetch )
{
/*****************************************************************************
 *
 *  Description : 레코드 한건을 fetch하여 into(bulk collection)절 변수에 대입
 *
 *  Implementation :
 *              (1) into절 변수의 데이터 타입으로 bindColumnInfo
 *              (2) into절 변수의 값을 bindColumnData
 *              (3) 한건 fetch, rowcount증가
 *              (4) 더이상 레코드가 없는지 여부 저장
 *
 *****************************************************************************/

    mtcColumn         * sMtcColumn;
    qcmColumn         * sQcmColumn;
    UShort              sBindColumnId = 0;
    void              * sValue;
    void              * sColumnValue;
    qciBindData       * sBindColumnDataList = NULL;
    qtcNode           * sNode;
    SInt                sStage = 0;
    iduMemoryStatus     sQmxMemStatus;
    /* BUG-41707 */
    qmsLimit          * sLimit       = NULL;
    vSLong              sRowCount    = 0;
    qtcNode           * sIndexNode   = NULL;
    void              * sIndexValue  = NULL;
    ULong               sLimitCount  = ((ULong)MTD_INTEGER_MAXIMUM);

    /* PROJ-2197 PSM Renewal */
    qmsInto           * sIntoVarNodes;

    sIntoVarNodes = aFetch->intoVariableNodes;
    sLimit = aFetch->limit;

    IDE_TEST_RAISE(aRefCurInfo->isOpen != ID_TRUE, err_invalid_cursor);

    IDE_TEST( QC_QMX_MEM(aQcStmt)-> getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    if( aRefCurInfo->nextRecordExist == ID_FALSE )
    {
        aRefCurInfo->isEndOfCursor = ID_TRUE;
        IDE_CONT( ERR_PASS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qcd::checkBindColumnCount( aRefCurInfo->hStmt,
                                         (UShort)aFetch->intoVarCount )
              != IDE_SUCCESS );

    /* BUG-41707 */
    // limit절의 count 값을 가져옵니다.
    if ( sIntoVarNodes->bulkCollect == ID_TRUE )
    {
        if ( sLimit != NULL )
        {
            IDE_TEST( qmsLimitI::getCountValue( QC_PRIVATE_TMPLATE(aQcStmt),
                                                sLimit,
                                                &sLimitCount )
                      != IDE_SUCCESS );
            IDE_TEST_CONT( sLimitCount == 0, ERR_PASS );
        }
        else
        {
            // Nothing to do.
            // 이 경우에는 sLimit = MTD_INTEGER_MAXIMUM
        }
    }
    else
    {
        sLimitCount = 1;
    }

    while ( ID_TRUE )
    {
        if ( sIntoVarNodes->bulkCollect == ID_TRUE )
        {
            for ( sNode = sIntoVarNodes->intoNodes;
                  sNode != NULL;
                  sNode = (qtcNode*)sNode->node.next )
            {
                IDE_DASSERT( sRowCount < (vSLong)MTD_INTEGER_MAXIMUM );

                sIndexNode   = (qtcNode*)sNode->node.arguments;
                sIndexValue  = QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aQcStmt),
                                                   sIndexNode );

                // bulk collect into에서 사용되는 array의 index는 integer type이여야 한다.
                IDE_DASSERT( QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                              sIndexNode )->module->id  == MTD_INTEGER_ID );

                // array의 index를 이전 index 값에 +1
                *(mtdIntegerType*)sIndexValue =
                    (mtdIntegerType)(sRowCount + 1);
            }
        }
        else
        {
            // Nothing to do.
        }

        sBindColumnId = 0;
        sBindColumnDataList = NULL;

        // bindColumnDataList생성
        if( aFetch->isIntoVarRecordType == ID_TRUE )
        {
            IDE_DASSERT( sIntoVarNodes->intoNodes != NULL );

            sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                          sIntoVarNodes->intoNodes );

            IDE_TEST( qtc::calculate( sIntoVarNodes->intoNodes,
                                      QC_PRIVATE_TMPLATE(aQcStmt) )
                      != IDE_SUCCESS );

            sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

            for( sQcmColumn = ((qtcModule*)(sMtcColumn->module))->typeInfo->columns;
                 sQcmColumn != NULL;
                 sQcmColumn = sQcmColumn->next )
            {
                sColumnValue = (void*)mtc::value( sQcmColumn->basicInfo,
                                                  sValue,
                                                  MTD_OFFSET_USE );

                IDE_TEST( qcd::addBindColumnDataList( aQcStmt->qmxMem,
                                                      &sBindColumnDataList,
                                                      sQcmColumn->basicInfo,
                                                      sColumnValue,
                                                      sBindColumnId )
                          != IDE_SUCCESS );

                sBindColumnId++;
            }
        }
        else
        {
            for( sNode = sIntoVarNodes->intoNodes;
                 sNode != NULL;
                 sNode = (qtcNode*)sNode->node.next )
            {
                sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                              sNode );

                IDE_TEST( qtc::calculate( sNode,
                                          QC_PRIVATE_TMPLATE(aQcStmt) )
                          != IDE_SUCCESS );

                sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

                IDE_TEST( qcd::addBindColumnDataList( aQcStmt->qmxMem,
                                                      &sBindColumnDataList,
                                                      sMtcColumn,
                                                      sValue,
                                                      sBindColumnId )
                          != IDE_SUCCESS );

                sBindColumnId++;
            }
        }

        // fetch
        IDE_TEST( qcd::fetch( aQcStmt,
                              QC_QMX_MEM(aQcStmt),
                              aRefCurInfo->hStmt,
                              sBindColumnDataList,
                              &aRefCurInfo->nextRecordExist )
                  != IDE_SUCCESS );

        aRefCurInfo->rowCount++;

        /* BUG-41707 */
        sRowCount++;
        sLimitCount--;

        // bulk collect into절과 함께온 limit 절의 count 수와 fetch한 레코드 수가 같은 경우
        // 더이상 데이터가 존재하지 않는 경우
        // fetch를 수행할 필요가 없다.
        if ( ( sLimitCount == 0 ) || ( aRefCurInfo->nextRecordExist == ID_FALSE ) )
        {
            break;
        }
        else
        {
            IDE_TEST_RAISE( sRowCount >= (vSLong)MTD_INTEGER_MAXIMUM,
                            ERR_TOO_MANY_ROWS );
        }
    }

    IDE_EXCEPTION_CONT(ERR_PASS);

    sStage=0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_cursor);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INVALID_CURSOR));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_ROWS);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_TOO_MANY_ROWS));
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
        {
            if ( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
            else
            {
                // Nothing to do.
            }
        }
        break;

        default:
            break;
    }
    sStage = 0;

    return IDE_FAILURE;
}
