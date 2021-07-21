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
 

#include <qmsDefaultExpr.h>
#include <qcuSqlSourceInfo.h>
#include <qcm.h>
#include <qcmCache.h>
#include <qcmUser.h>
#include <qcpUtil.h>
#include <qdn.h>
#include <qtc.h>
#include <qcg.h>

IDE_RC
qmsDefaultExpr::isFunctionBasedIndex( qcmTableInfo * aTableInfo,
                                      qcmIndex     * aIndex,
                                      idBool       * aResult )
{
/***********************************************************************
 *
 * Description :
 *    Function-based Index인지 확인한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcColumn     * sMtcColumn = NULL;
    qcmColumn     * sQcmColumn = NULL;
    idBool          sResult    = ID_FALSE;
    UInt            i;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::isFunctionBasedIndex::__FT__" );

    for ( i = 0; i < aIndex->keyColCount; i++ )
    {
        sMtcColumn = &(aIndex->keyColumns[i]);

        IDE_TEST( qcmCache::getColumnByID( aTableInfo,
                                           sMtcColumn->column.id,
                                           &sQcmColumn )
                  != IDE_SUCCESS );

        if ( (sQcmColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
        {
            sResult = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::isBaseColumn( qcStatement  * aStatement,
                              qcmTableInfo * aTableInfo,
                              SChar        * aDefaultExprStr,
                              UInt           aColumnID,
                              idBool       * aResult )
{
/***********************************************************************
 *
 * Description :
 *    Default Expression에 해당 Column이 포함되어 있는지 확인한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode       * sNode[2];
    qtcNode       * sExprNode       = NULL;
    qcmColumn     * sExprColumnList;
    qcmColumn     * sExprColumn     = NULL;
    qcmColumn     * sExprColumnInfo = NULL;
    SChar         * sDefaultExprStr = aDefaultExprStr;
    idBool          sResult         = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::isBaseColumn::__FT__" );

    IDE_TEST( qcpUtil::makeDefaultExpression(
                  aStatement,
                  sNode,
                  sDefaultExprStr,
                  idlOS::strlen( sDefaultExprStr ) )
              != IDE_SUCCESS );

    sExprNode = sNode[0];

    /* adjust expression position */
    sExprNode->position.offset = 7; /* "RETURN " */
    sExprNode->position.size   = idlOS::strlen( sDefaultExprStr );

    sExprColumnList = NULL;
    IDE_TEST( makeColumnListFromExpression(
                  aStatement,
                  &sExprColumnList,
                  sExprNode )
              != IDE_SUCCESS );

    for ( sExprColumn = sExprColumnList;
          sExprColumn != NULL;
          sExprColumn = sExprColumn->next )
    {
        IDE_TEST( qcmCache::getColumn( aStatement,
                                       aTableInfo,
                                       sExprColumn->namePos,
                                       &sExprColumnInfo )
                  != IDE_SUCCESS );

        if ( sExprColumnInfo->basicInfo->column.id == aColumnID )
        {
            sResult = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::isRelatedToFunctionBasedIndex( qcStatement  * aStatement,
                                               qcmTableInfo * aTableInfo,
                                               qcmIndex     * aIndex,
                                               UInt           aColumnID,
                                               idBool       * aResult )
{
/***********************************************************************
 *
 * Description :
 *    Function-based Index 관련 Column인지 확인한다.
 *
 * Implementation :
 *    1. 해당 Index를 구성하는 Column에 해당 Column이 포함되어 있는지 확인한다.
 *    2. 1의 결과에 따라
 *      (1) 있으면, 해당 Index가 Function-based Index인지 확인한다.
 *      (2) 없으면, Hidden Column의 Default Expression에 해당 Column이
 *          포함되어 있는지 확인한다.
 *
 ***********************************************************************/

    mtcColumn     * sMtcColumn            = NULL;
    qcmColumn     * sQcmColumn            = NULL;
    SChar         * sDefaultExprStr = NULL;
    idBool          sResult               = ID_FALSE;
    UInt            i;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::isRelatedToFunctionBasedIndex::__FT__" );

    if ( qdn::intersectColumn( (UInt *)smiGetIndexColumns( aIndex->indexHandle ),
                               aIndex->keyColCount,
                               &aColumnID,
                               1 )
         == ID_TRUE )
    {
        IDE_TEST( isFunctionBasedIndex( aTableInfo,
                                        aIndex,
                                        &sResult )
                  != IDE_SUCCESS );
    }
    else
    {
        for ( i = 0; i < aIndex->keyColCount; i++ )
        {
            sMtcColumn = &(aIndex->keyColumns[i]);

            IDE_TEST( qcmCache::getColumnByID( aTableInfo,
                                               sMtcColumn->column.id,
                                               &sQcmColumn )
                      != IDE_SUCCESS );

            /* Hidden Column인지 확인한다. */
            if ( ( (sQcmColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                   == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
                 ( sQcmColumn->defaultValueStr != NULL ) )
            {
                /* Nothing to do */
            }
            else
            {
                continue;
            }

            /* Expression에 해당 Column이 포함되어 있는지 확인한다. */
            sDefaultExprStr = (SChar *)sQcmColumn->defaultValueStr;

            IDE_TEST( isBaseColumn( aStatement,
                                    aTableInfo,
                                    sDefaultExprStr,
                                    aColumnID,
                                    &sResult )
                      != IDE_SUCCESS );

            if ( sResult == ID_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::makeColumnListFromExpression( qcStatement   * aStatement,
                                              qcmColumn    ** aColumnList,
                                              qtcNode       * aNode )
{
/***********************************************************************
 *
 * Description :
 *  Function-based Index Column List를 만든다.
 *      - subquery를 지원하지 않는다.
 *
 * Implementation :
 *  (1) qtc::makeColumn()에서 지정한 항목으로 Column인지 확인하고,
 *      Column Name 중복이 없는 Function-based Index Column List를 구성한다.
 *      - Function-based Index Column에 User Name, Table Name을 지정할 수 없다.
 *  (2) arguments를 Recursive Call
 *  (3) next를 Recursive Call
 *
 ***********************************************************************/

    qcmColumn           * sColumn;
    qcmColumn           * sLastColumn = NULL;
    qcmColumn           * sNewColumn  = NULL;
    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::makeColumnListFromExpression::__FT__" );

    /* qtc::makeColumn()에서 지정한 항목으로 Column인지 확인하고,
     * Column Name 중복이 없는 Function-based Index Column List를 구성한다.
     */
    if ( (QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE) &&
         (aNode->node.module == &qtc::columnModule) )
    {
        /* Function-based Index Column에 User Name, Table Name을 지정할 수 없다. */
        if ( QC_IS_NULL_NAME( aNode->userName ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aNode->userName) );
            IDE_RAISE( ERR_CANNOT_SPECIFY_USER_NAME_OR_TABLE_NAME );
        }
        else if ( QC_IS_NULL_NAME( aNode->tableName ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aNode->tableName) );
            IDE_RAISE( ERR_CANNOT_SPECIFY_USER_NAME_OR_TABLE_NAME );
        }
        else
        {
            /* Nothing to do */
        }

        for ( sColumn = *aColumnList;
              sColumn != NULL;
              sColumn = sColumn->next )
        {
            if ( QC_IS_NAME_MATCHED( sColumn->namePos, aNode->columnName ) )
            {
                break;
            }
            else
            {
                sLastColumn = sColumn;
            }
        }

        if ( sColumn == NULL )
        {
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcmColumn, &sNewColumn )
                      != IDE_SUCCESS );
            QCM_COLUMN_INIT( sNewColumn );
            SET_POSITION( sNewColumn->namePos, aNode->columnName );

            if ( *aColumnList == NULL )
            {
                *aColumnList = sNewColumn;
            }
            else
            {
                sLastColumn->next = sNewColumn;
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

    /* user-defined function인 경우 user name을 명시해야한다. */
    if ( (QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE) &&
         (QC_IS_NULL_NAME( aNode->tableName ) == ID_TRUE) &&
         (aNode->node.module == &qtc::spFunctionCallModule) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(aNode->columnName) );
        IDE_RAISE( ERR_REQUIRE_OWNER_NAME );
    }
    else
    {
        /* Nothing to do */
    }
    
    /* arguments를 Recursive Call */
    if ( aNode->node.arguments != NULL )
    {
        IDE_TEST( makeColumnListFromExpression(
                      aStatement,
                      aColumnList,
                      (qtcNode *)aNode->node.arguments )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* next를 Recursive Call */
    if ( aNode->node.next != NULL )
    {
        IDE_TEST( makeColumnListFromExpression(
                      aStatement,
                      aColumnList,
                      (qtcNode *)aNode->node.next )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_SPECIFY_USER_NAME_OR_TABLE_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_CANNOT_SPECIFY_USER_NAME_OR_TABLE_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_REQUIRE_OWNER_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_REQUIRE_OWNER_NAME_IN_DEFAULT_EXPR,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::makeFunctionNameListFromExpression( qcStatement         * aStatement,
                                                    qdFunctionNameList ** aFunctionNameList,
                                                    qtcNode             * aTargetNode,
                                                    qtcNode             * aTopNode )
{
/***********************************************************************
 *
 * Description :
 *  Expression에서 Function Name List를 만든다.
 *      - subquery를 지원하지 않는다.
 *
 * Implementation :
 *  (1) qtc::makeNode()에서 지정한 항목으로 Function인지 확인하고,
 *      중복이 없는 Function Name List를 구성한다.
 *  (2) arguments를 Recursive Call
 *  (3) next를 Recursive Call
 *
 ***********************************************************************/

    qdFunctionNameList * sFunctionName;
    qdFunctionNameList * sLastFunctionName = NULL;
    qdFunctionNameList * sNewFunctionName  = NULL;
    UInt                 sUserID;
    qcNamePosition       sObjName;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::makeFunctionNameListFromExpression::__FT__" );

    /* qtc::makeNode()에서 지정한 항목으로 Function인지 확인하고,
     * 중복이 없는 Function Name List를 구성한다.
     * User-defined Function인 경우, User Name을 명시해야 한다.
     * (user.func1()은 tableName.columnName으로 파싱한다.)
     */
    if ( ( QC_IS_NULL_NAME( aTargetNode->tableName ) == ID_FALSE ) &&
         ( QC_IS_NULL_NAME( aTargetNode->columnName ) == ID_FALSE ) &&
         ( aTargetNode->node.module == &qtc::spFunctionCallModule ) )
    {
        if ( QC_IS_NULL_NAME( aTargetNode->userName ) == ID_FALSE )
        {
            // USER,TABLE,COLUMN이 모두 있으면 PKG SUB FUNCTION CALL 이다.
            // USER.TABLE.COLUMN() => USER.PKG.FUNC()
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          aTargetNode->userName,
                                          &sUserID )
                      != IDE_SUCCESS );

            sObjName = aTargetNode->tableName;
        }
        else
        {
            // TABLE,COLUMN만 있으면 FUNCTION CALL 이다.
            // TABLE.COLUMN() => USER.FUNC()
            // TABLE.COLUMN() != PKG.FUNC()
            IDE_TEST( qcmUser::getUserID( aStatement,
                                          aTargetNode->tableName,
                                          &sUserID )
                      != IDE_SUCCESS );

            sObjName = aTargetNode->columnName;
        }

        for ( sFunctionName = *aFunctionNameList;
              sFunctionName != NULL;
              sFunctionName = sFunctionName->next )
        {
            if ( QC_IS_NAME_MATCHED( sFunctionName->functionName, sObjName ) &&
                 ( sFunctionName->userID == sUserID ) )
            {
                if ( aTopNode != NULL )
                {
                    if ( ( aTopNode->position.offset <=
                           sFunctionName->functionName.offset ) &&
                         ( ( aTopNode->position.offset +
                             aTopNode->position.size ) >=
                           ( sFunctionName->functionName.offset +
                             sFunctionName->functionName.size ) ) )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                /* Nothing to do */
            }

            sLastFunctionName = sFunctionName;
        }

        if ( sFunctionName == NULL )
        {
            IDE_TEST( STRUCT_CRALLOC( QC_QMP_MEM(aStatement), qdFunctionNameList, &sNewFunctionName )
                      != IDE_SUCCESS );

            SET_POSITION( sNewFunctionName->functionName, sObjName );
            sNewFunctionName->userID = sUserID;
            sNewFunctionName->next   = NULL;

            if ( *aFunctionNameList == NULL )
            {
                *aFunctionNameList = sNewFunctionName;
            }
            else
            {
                sLastFunctionName->next = sNewFunctionName;
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

    /* arguments를 Recursive Call */
    if ( aTargetNode->node.arguments != NULL )
    {
        IDE_TEST( makeFunctionNameListFromExpression(
                      aStatement,
                      aFunctionNameList,
                      (qtcNode *)aTargetNode->node.arguments,
                      aTopNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* next를 Recursive Call */
    if ( aTargetNode->node.next != NULL )
    {
        IDE_TEST( makeFunctionNameListFromExpression(
                      aStatement,
                      aFunctionNameList,
                      (qtcNode *)aTargetNode->node.next,
                      aTopNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::makeBaseColumn( qcStatement    * aStatement,
                                qcmTableInfo   * aTableInfo,
                                qcmColumn      * aDefaultExpr,
                                qcmColumn     ** aBaseColumns )
{
/***********************************************************************
 *
 * Description :
 *    Default Expression을 구성하는 Column List를 만든다.
 *
 ***********************************************************************/
    
    qcmColumn     * sDefaultColumn;
    qcmColumn     * sFirstColumn    = NULL;
    qcmColumn     * sLastColumn     = NULL;
    qcmColumn     * sExprColumnList;
    qcmColumn     * sExprColumn     = NULL;
    qcmColumn     * sExprColumnInfo = NULL;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::makeBaseColumn::__FT__" );

    for ( sDefaultColumn = aDefaultExpr;
          sDefaultColumn != NULL;
          sDefaultColumn = sDefaultColumn->next )
    {
        /* Default Expr을 가진 Column인지 확인한다. */
        if ( ( (sDefaultColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
               == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
             ( sDefaultColumn->defaultValueStr != NULL ) )
        {
            /* Nothing to do */
        }
        else
        {
            continue;
        }

        /* Default Expr을 구성하는 Column List를 구한다. */
        sExprColumnList = NULL;
        IDE_TEST( makeColumnListFromExpression(
                      aStatement,
                      &sExprColumnList,
                      sDefaultColumn->defaultValue )
                  != IDE_SUCCESS );

        /* 중복되지 않게 Base Column List를 구성한다. */
        sExprColumn = sExprColumnList;
        while ( sExprColumn != NULL )
        {
            IDE_TEST( qcmCache::getColumn( aStatement,
                                           aTableInfo,
                                           sExprColumn->namePos,
                                           &sExprColumnInfo )
                      != IDE_SUCCESS );

            if ( qdbCommon::findColumnIDInColumnList(
                     sFirstColumn,
                     sExprColumnInfo->basicInfo->column.id )
                 != ID_TRUE )
            {
                QDB_SET_QCM_COLUMN( sExprColumn, sExprColumnInfo );

                if ( sFirstColumn == NULL )
                {
                    sFirstColumn = sExprColumn;
                }
                else
                {
                    sLastColumn->next = sExprColumn;
                }
                sLastColumn = sExprColumn;

                sExprColumn = sExprColumn->next;
                sLastColumn->next = NULL;
            }
            else
            {
                sExprColumn = sExprColumn->next;
            }
        }
    }

    *aBaseColumns = sFirstColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmsDefaultExpr::setUsedColumnToTableRef( mtcTemplate  * aTemplate,
                                         qmsTableRef  * aTableRef,
                                         qcmColumn    * aColumns,
                                         idBool         aIsList )
{
/***********************************************************************
 *
 * Description :
 *    TableRef에 해당 Column List를 사용한 것으로 설정한다.
 *
 ***********************************************************************/

    qcmColumn         * sQcmColumn;
    mtcColumn         * sMtcColumn;
    UShort              sTupleID;
    UInt                sColumnID;

    sTupleID = aTableRef->table;

    for ( sQcmColumn = aColumns;
          sQcmColumn != NULL;
          sQcmColumn = sQcmColumn->next )
    {
        sColumnID = sQcmColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

        sMtcColumn = &(aTemplate->rows[sTupleID].columns[sColumnID]);

        sMtcColumn->flag |= MTC_COLUMN_USE_COLUMN_TRUE;

        if ( aIsList != ID_TRUE )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
}

void
qmsDefaultExpr::setRowBufferFromBaseColumn( mtcTemplate  * aTemplate,
                                            UShort         aSrcTupleID,
                                            UShort         aDstTupleID,
                                            qcmColumn    * aBaseColumns,
                                            void         * aRowBuffer )
{
/***********************************************************************
 *
 * Description :
 *    Default Expression을 구성하는 Column의 값을 새로운 Row Buffer에 복사한다.
 *
 ***********************************************************************/

    qcmColumn         * sQcmColumn;
    UInt                sColumnID;
    UInt                sActualSize;

    void              * sSrcRowBuffer;
    void              * sDstRowBuffer;
    void              * sSrcMtdValue;
    void              * sDstMtdValue;
    mtcColumn         * sSrcMtcColumn;
    mtcColumn         * sDstMtcColumn;

    sSrcRowBuffer = aTemplate->rows[aSrcTupleID].row;
    sDstRowBuffer = aRowBuffer;

    for ( sQcmColumn = aBaseColumns;
          sQcmColumn != NULL;
          sQcmColumn = sQcmColumn->next )
    {
        sColumnID = sQcmColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

        sSrcMtcColumn = &(aTemplate->rows[aSrcTupleID].columns[sColumnID]);
        sDstMtcColumn = &(aTemplate->rows[aDstTupleID].columns[sColumnID]);

        sSrcMtdValue = (void *)mtc::value( sSrcMtcColumn,
                                           sSrcRowBuffer,
                                           MTD_OFFSET_USE );
        sDstMtdValue = (void *)((UChar *)sDstRowBuffer + sDstMtcColumn->column.offset);

        sActualSize = sSrcMtcColumn->module->actualSize( sSrcMtcColumn,
                                                         sSrcMtdValue );
        idlOS::memcpy( sDstMtdValue, sSrcMtdValue, sActualSize );
    }
}

IDE_RC
qmsDefaultExpr::setRowBufferFromSmiValueArray( mtcTemplate  * aTemplate,
                                               qmsTableRef  * aTableRef,
                                               qcmColumn    * aColumns,
                                               void         * aRowBuffer,
                                               smiValue     * aValueArr,
                                               idBool         aIsValueArrDisk )
{
/***********************************************************************
 *
 * Description :
 *    smiValue 배열을 Row Buffer에 복사한다.
 *
 ***********************************************************************/

    qcmColumn  * sQcmColumn;
    mtcColumn  * sMtcColumn;
    void       * sMtdValue;
    UShort       sTupleID;
    UInt         sColumnID;
    UInt         i;

    sTupleID = aTableRef->table;

    for ( sQcmColumn = aColumns, i = 0;
          sQcmColumn != NULL;
          sQcmColumn = sQcmColumn->next, i++ )
    {
        sColumnID = sQcmColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;
        sMtcColumn = &(aTemplate->rows[sTupleID].columns[sColumnID]);

        if ( ( sMtcColumn->module->id == MTD_BLOB_ID ) ||
             ( sMtcColumn->module->id == MTD_BLOB_LOCATOR_ID ) ||
             ( sMtcColumn->module->id == MTD_CLOB_ID ) ||
             ( sMtcColumn->module->id == MTD_CLOB_LOCATOR_ID ) )
        {
            /* LOB을 지원하지 않는다. */
        }
        else
        {
            sMtdValue = (void *)((UChar *)aRowBuffer + sMtcColumn->column.offset);

            /* PROJ-2464 hybrid partitioned table 지원 */
            if ( aIsValueArrDisk == ID_FALSE )
            {
                if ( aValueArr[i].length == 0 )
                {
                    IDE_DASSERT( aValueArr[i].value == NULL );

                    sMtcColumn->module->null( sMtcColumn, sMtdValue );
                }
                else
                {
                    idlOS::memcpy( sMtdValue, aValueArr[i].value, aValueArr[i].length );
                }
            }
            else
            {
                /* PROJ-2429 Dictionary based data compress for on-disk DB */
                IDE_TEST( sMtcColumn->module->storedValue2MtdValue[MTD_COLUMN_COPY_FUNC_NORMAL](
                              sMtcColumn->column.size,
                              sMtdValue,
                              0,
                              aValueArr[i].length,
                              aValueArr[i].value )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::calculateDefaultExpression( qcTemplate   * aTemplate,
                                            qmsTableRef  * aTableRef,
                                            qcmColumn    * aUpdateColumns,
                                            qcmColumn    * aDefaultExprColumns,
                                            const void   * aRowBuffer,
                                            smiValue     * aValueArr,
                                            qcmColumn    * aTableColumnsForValueArr )
{
/***********************************************************************
 *
 * Description :
 *    Row Buffer의 데이터로 Default Expression의 calculate를 수행한다.
 *    SM Interface에 바로 적용할 수 있는 형태로, 결과 값을 반환한다.
 *
 ***********************************************************************/

    qcmColumn         * sColumn;
    qcmColumn         * sTargetColumn;
    mtcColumn         * sValueColumn;
    void              * sValue;
    UInt                sStoringSize  = 0;
    void              * sStoringValue = NULL;
    UInt                i;
    UInt                j = 0;

    aTemplate->tmplate.rows[aTableRef->table].row = (void *)aRowBuffer;

    for ( sColumn = aDefaultExprColumns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
               == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
             ( sColumn->defaultValue != NULL ) )
        {
            /* 결과 값을 얻는다. */
            IDE_TEST( qtc::calculate( sColumn->defaultValue,
                                      aTemplate )
                      != IDE_SUCCESS );
            
            sValueColumn = aTemplate->tmplate.stack->column;
            sValue       = aTemplate->tmplate.stack->value;

            IDE_DASSERT( sColumn->basicInfo->type.dataTypeId ==
                         sValueColumn->type.dataTypeId );

            IDE_DASSERT( sColumn->basicInfo->precision ==
                         sValueColumn->precision );

            IDE_DASSERT( sColumn->basicInfo->scale ==
                         sValueColumn->scale );

            /* smiValue 배열 내의 위치를 구한다. */
            if ( aUpdateColumns != NULL )
            {
                for ( sTargetColumn = aUpdateColumns, i = 0;
                      sTargetColumn != NULL;
                      sTargetColumn = sTargetColumn->next, i++ )
                {
                    if ( sTargetColumn->basicInfo->column.id ==
                         sColumn->basicInfo->column.id )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                IDE_TEST_RAISE( sTargetColumn == NULL,
                                ERR_NOT_EXIST_IN_UPDATE_COLUMNS );
            }
            else
            {
                i = sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;
            }

            /* PROJ-2464 hybrid partitioned table 지원 */
            j = sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

            // PROJ-1705
            IDE_TEST( qdbCommon::mtdValue2StoringValue( aTableColumnsForValueArr[j].basicInfo,
                                                        sValueColumn,
                                                        sValue,
                                                        &sStoringValue )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::storingSize( aTableColumnsForValueArr[j].basicInfo,
                                              sValueColumn,
                                              sValue,
                                              &sStoringSize )
                      != IDE_SUCCESS );
            
            aValueArr[i].value = sStoringValue;
            aValueArr[i].length = sStoringSize;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_IN_UPDATE_COLUMNS );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsDefaultExpr::calculateDefaultExpr",
                                  "Not exist in update columns" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::makeNodeListForFunctionBasedIndex( qcStatement   * aStatement,
                                                   qmsTableRef   * aTableRef,
                                                   qmsExprNode  ** aDefaultExprList )
{
/***********************************************************************
 *
 * Description :
 *    Default Expression을 가진 Column의 Default Expression Node List와
 *    Hidden Column Node List를 만든다.
 *
 *    관련 함수 : findAndReplaceNodeForFunctionBasedIndex()
 *
 ***********************************************************************/

    qmsFrom             sFrom;
    qcmColumn         * sColumn;
    SChar             * sExpressionStr;
    qtcNode           * sNode[2];
    qmsExprNode       * sLastDefaultExprList = NULL;
    qmsExprNode       * sNewDefaultExprList  = NULL;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::makeNodeListForFunctionBasedIndex::__FT__" );

    *aDefaultExprList  = NULL;

    QCP_SET_INIT_QMS_FROM( (&sFrom) );
    sFrom.tableRef = aTableRef;

    for ( sColumn = aTableRef->tableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        /* Default Expression을 가진 Hidden Column인지 확인한다. */
        if ( ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
               == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
             ( sColumn->defaultValueStr != NULL ) )
        {
            /* Nothing to do */
        }
        else
        {
            continue;
        }

        /* Default Expression Node List를 만든다. */
        sExpressionStr = (SChar *)sColumn->defaultValueStr;
        IDE_TEST( qcpUtil::makeDefaultExpression(
                      aStatement,
                      sNode,
                      sExpressionStr,
                      idlOS::strlen( sExpressionStr ) )
                  != IDE_SUCCESS );

        // adjust expression position
        sNode[0]->position.offset = 7; /* "RETURN " */
        sNode[0]->position.size   = idlOS::strlen( sExpressionStr );

        IDE_TEST( qdbCommon::validateDefaultExprDefinition(
                      aStatement,
                      sNode[0],
                      NULL,
                      &sFrom )
                  != IDE_SUCCESS );

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qmsExprNode,
                                &sNewDefaultExprList )
                  != IDE_SUCCESS );
        
        sNewDefaultExprList->tableRef = aTableRef;
        sNewDefaultExprList->column   = sColumn;
        sNewDefaultExprList->node     = sNode[0];
        sNewDefaultExprList->next     = NULL;

        if ( *aDefaultExprList == NULL )
        {
            *aDefaultExprList = sNewDefaultExprList;
        }
        else
        {
            sLastDefaultExprList->next = sNewDefaultExprList;
        }
        sLastDefaultExprList = sNewDefaultExprList;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::findAndReplaceNodeForFunctionBasedIndex( qcStatement   * aStatement,
                                                         qtcNode       * aTargetNode,
                                                         qmsExprNode   * aExprNodeList,
                                                         qtcNode      ** aResultNode,
                                                         idBool        * aNeedToEstimate )
{
/***********************************************************************
 *
 * Description :
 *    Target Node에서 Find Node List 내의 Node를 찾아서,
 *    Replace Node List 내의 Node를 복제하여 교체한다.
 *
 ***********************************************************************/

    qmsExprNode       * sFindNode;
    qtcNode           * sTargetNode;
    qtcNode           * sColumnNode;
    idBool              sIsEquivalent   = ID_FALSE;
    idBool              sNeedToEstimate = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::findAndReplaceNodeForFunctionBasedIndex::__FT__" );

    IDE_DASSERT( aTargetNode != NULL );

    sTargetNode = aTargetNode;

    if ( ( (sTargetNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK)
           != MTC_NODE_LOGICAL_CONDITION_TRUE )
         &&
         ( (sTargetNode->node.lflag & MTC_NODE_COMPARISON_MASK)
           != MTC_NODE_COMPARISON_TRUE ) )
    {
        for ( sFindNode = aExprNodeList;
              sFindNode != NULL;
              sFindNode = sFindNode->next )
        {
            IDE_TEST( qtc::isEquivalentExpression(
                          aStatement,
                          sTargetNode,
                          sFindNode->node,
                          &sIsEquivalent )
                      != IDE_SUCCESS );

            if ( sIsEquivalent == ID_TRUE )
            {
                // make column node
                IDE_TEST( qtc::makeInternalColumn(
                              aStatement,
                              sFindNode->tableRef->table,
                              (UShort)sFindNode->column->basicInfo->column.id & SMI_COLUMN_ID_MASK,
                              & sColumnNode )
                          != IDE_SUCCESS);
                
                IDE_TEST( qtc::estimateNodeWithoutArgument(
                              aStatement,
                              sColumnNode )
                          != IDE_SUCCESS );
                
                /* Disk Table의 Fetch Column List에 Hidden Column을 추가 */
                setUsedColumnToTableRef( &(QC_SHARED_TMPLATE(aStatement)->tmplate),
                                         sFindNode->tableRef,
                                         sFindNode->column,
                                         ID_FALSE );
                
                /* Plan에 Expression을 표시하기 위해 보관 */
                sColumnNode->node.orgNode = (mtcNode *)sTargetNode;

                /* Conversion, Next, Flag를 유지 */
                sColumnNode->node.conversion     = sTargetNode->node.conversion;
                sColumnNode->node.leftConversion = sTargetNode->node.leftConversion;
                sColumnNode->node.next           = sTargetNode->node.next;
                sColumnNode->node.lflag         |= sTargetNode->node.lflag & MTC_NODE_MASK;
                sColumnNode->lflag              |= sTargetNode->lflag & QTC_NODE_MASK;

                sTargetNode = sColumnNode;
                *aNeedToEstimate = ID_TRUE;
                break;
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

    /* Arguments를 처리한다. */
    if ( ( ( (sTargetNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK)
             == MTC_NODE_LOGICAL_CONDITION_TRUE )
           ||
           ( (sTargetNode->node.lflag & MTC_NODE_COMPARISON_MASK)
             == MTC_NODE_COMPARISON_TRUE ) )
         &&  
         ( sTargetNode->node.arguments != NULL ) )
    {
        IDE_TEST( findAndReplaceNodeForFunctionBasedIndex(
                      aStatement,
                      (qtcNode *)sTargetNode->node.arguments,
                      aExprNodeList,
                      (qtcNode **)&(sTargetNode->node.arguments),
                      &sNeedToEstimate )
                  != IDE_SUCCESS );

        /* Index를 사용할 수 있도록 다시 estimate한다. */
        if ( sNeedToEstimate == ID_TRUE )
        {
            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     sTargetNode )
                      != IDE_SUCCESS );
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

    /* Next를 처리한다. */
    if ( sTargetNode->node.next != NULL )
    {
        IDE_TEST( findAndReplaceNodeForFunctionBasedIndex(
                      aStatement,
                      (qtcNode *)sTargetNode->node.next,
                      aExprNodeList,
                      (qtcNode **)&(sTargetNode->node.next),
                      aNeedToEstimate )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    *aResultNode = sTargetNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::applyFunctionBasedIndex( qcStatement   * aStatement,
                                         qtcNode       * aTargetNode,
                                         qmsFrom       * aStartFrom,
                                         qtcNode      ** aResultNode )
{
/***********************************************************************
 *
 * Description :
 *    From 구조를 순회하면서 Function-based Index를 적용한다.
 *
 *    Target Node에서 Find Node List 내의 Node를 찾아서,
 *    Replace Node List 내의 Node를 복제하여 교체한다.
 *
 ***********************************************************************/

    qtcNode           * sTargetNode;
    qmsFrom           * sFrom;
    idBool              sDummyFlag = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::applyFunctionBasedIndex::__FT__" );

    sTargetNode = aTargetNode;

    for ( sFrom = aStartFrom;
          sFrom != NULL;
          sFrom = sFrom->next )
    {
        if ( sFrom->joinType == QMS_NO_JOIN )
        {
            if ( sFrom->tableRef != NULL )
            {
                if ( sFrom->tableRef->defaultExprList != NULL )
                {
                    IDE_TEST( findAndReplaceNodeForFunctionBasedIndex(
                                  aStatement,
                                  sTargetNode,
                                  sFrom->tableRef->defaultExprList,
                                  &sTargetNode,
                                  &sDummyFlag )
                              != IDE_SUCCESS );
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
            if ( sFrom->left != NULL )
            {
                IDE_TEST( applyFunctionBasedIndex(
                              aStatement,
                              sTargetNode,
                              sFrom->left,
                              &sTargetNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            if ( sFrom->right != NULL )
            {
                IDE_TEST( applyFunctionBasedIndex(
                              aStatement,
                              sTargetNode,
                              sFrom->right,
                              &sTargetNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    *aResultNode = sTargetNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::addDefaultExpressionColumnsRelatedToColumn(
    qcStatement       * aStatement,
    qcmColumn        ** aDefaultExprColumns,
    qcmTableInfo      * aTableInfo,
    UInt                aColumnID )
{
/***********************************************************************
 *
 * Description :
 *    특정 Column과 관련된 Default Expression Column List를 얻는다.
 *
 * Implementation :
 *    각 Default Expression에 대해 아래를 반복한다.
 *    1. Hidden Column인지 확인한다.
 *    2. Expression에 해당 Column이 포함되어 있는지 확인한다.
 *    3. 중복을 확인한다.
 *    4. Parsing하여 Node를 생성한다.
 *    5. Default Expression Column List의 마지막에 추가한다.
 *
 ***********************************************************************/

    qcmColumn     * sColumn          = NULL;
    SChar         * sDefaultValueStr = NULL;
    idBool          sResult          = ID_FALSE;

    qcmColumn     * sCurrColumn      = NULL;
    qcmColumn     * sLastColumn      = NULL;
    qcmColumn     * sNewColumn       = NULL;
    qtcNode       * sNode[2];

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::addDefaultExpressionColumnsRelatedToColumn::__FT__" );

    for ( sColumn = aTableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        /* Hidden Column인지 확인한다. */
        if ( ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
               == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
             ( sColumn->defaultValueStr != NULL ) )
        {
            /* Nothing to do */
        }
        else
        {
            continue;
        }

        /* Expression에 해당 Column이 포함되어 있는지 확인한다. */
        sDefaultValueStr = (SChar *)sColumn->defaultValueStr;

        IDE_TEST( qmsDefaultExpr::isBaseColumn( aStatement,
                                                aTableInfo,
                                                sDefaultValueStr,
                                                aColumnID,
                                                &sResult )
                  != IDE_SUCCESS );

        if ( sResult != ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* 중복을 확인한다. */
        sLastColumn = NULL;
        for ( sCurrColumn = *aDefaultExprColumns;
              sCurrColumn != NULL;
              sCurrColumn = sCurrColumn->next )
        {
            if ( sCurrColumn->basicInfo->column.id ==
                 sColumn->basicInfo->column.id )
            {
                break;
            }
            else
            {
                sLastColumn = sCurrColumn;
            }
        }

        if ( sCurrColumn != NULL )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* Parsing하여 Node를 생성한다. */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qcmColumn,
                                &sNewColumn )
                  != IDE_SUCCESS );
        QCM_COLUMN_INIT( sNewColumn );
        QDB_SET_QCM_COLUMN( sNewColumn, sColumn );
        sNewColumn->defaultValueStr = sColumn->defaultValueStr;

        IDE_TEST( qcpUtil::makeDefaultExpression(
                      aStatement,
                      sNode,
                      sDefaultValueStr,
                      idlOS::strlen( sDefaultValueStr ) )
                  != IDE_SUCCESS );
        sNewColumn->defaultValue = sNode[0];

        /* adjust expression position */
        sNewColumn->defaultValue->position.offset = 7; /* "RETURN " */
        sNewColumn->defaultValue->position.size   = idlOS::strlen( sDefaultValueStr );

        /* Default Expression Column List의 마지막에 추가한다. */
        if ( *aDefaultExprColumns == NULL )
        {
            *aDefaultExprColumns = sNewColumn;
        }
        else
        {
            sLastColumn->next = sNewColumn;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsDefaultExpr::makeDefaultExpressionColumnsRelatedToTable(
    qcStatement       * aStatement,
    qcmColumn        ** aDefaultExprColumns,
    qcmTableInfo      * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *    Table과 관련된 Default Expression Column List를 얻는다.
 *
 * Implementation :
 *    각 Default Expression에 대해 아래를 반복한다.
 *    1. Hidden Column인지 확인한다.
 *    2. Parsing하여 Node를 생성한다.
 *    3. Default Expression Column List의 마지막에 추가한다.
 *
 ***********************************************************************/

    qcmColumn     * sColumn          = NULL;
    SChar         * sDefaultValueStr = NULL;

    qcmColumn     * sLastColumn      = NULL;
    qcmColumn     * sNewColumn       = NULL;
    qtcNode       * sNode[2];

    IDU_FIT_POINT_FATAL( "qmsDefaultExpr::makeDefaultExpressionColumnsRelatedToTable::__FT__" );

    *aDefaultExprColumns = NULL;

    for ( sColumn = aTableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        /* Hidden Column인지 확인한다. */
        if ( ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
               == QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
             ( sColumn->defaultValueStr != NULL ) )
        {
            /* Nothing to do */
        }
        else
        {
            continue;
        }

        sDefaultValueStr = (SChar *)sColumn->defaultValueStr;

        /* Parsing하여 Node를 생성한다. */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qcmColumn,
                                &sNewColumn )
                  != IDE_SUCCESS );
        QCM_COLUMN_INIT( sNewColumn );
        QDB_SET_QCM_COLUMN( sNewColumn, sColumn );
        sNewColumn->defaultValueStr = sColumn->defaultValueStr;

        IDE_TEST( qcpUtil::makeDefaultExpression(
                      aStatement,
                      sNode,
                      sDefaultValueStr,
                      idlOS::strlen( sDefaultValueStr ) )
                  != IDE_SUCCESS );
        sNewColumn->defaultValue = sNode[0];

        /* adjust expression position */
        sNewColumn->defaultValue->position.offset = 7; /* "RETURN " */
        sNewColumn->defaultValue->position.size   = idlOS::strlen( sDefaultValueStr );

        /* Default Expression Column List의 마지막에 추가한다. */
        if ( *aDefaultExprColumns == NULL )
        {
            *aDefaultExprColumns = sNewColumn;
        }
        else
        {
            sLastColumn->next = sNewColumn;
        }
        sLastColumn = sNewColumn;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
