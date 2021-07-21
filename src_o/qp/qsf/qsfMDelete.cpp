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
 * $Id: qsfMDelete.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1075 array type변수의 member function DELETE
 *
 * Syntax :
 *     arr_var.DELETE( [ index | index_lower, index_upper ] );
 *     RETURN INTEGER  <= delete된 count.
 *
 * Implementation :
 *     spec상 procedure가 되어야 하지만, function으로 처리. 대신
 *     delete된 count를 return한다.
 *
 *     1. index가 없으면 모두 삭제.
 *     2. index가 하나라면 해당 index의 element를 삭제.
 *     3. index가 두개라면 index_lower ~ index_upper까지의 element를 삭제.
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <qc.h>
#include <qsvEnv.h>
#include <qcuSqlSourceInfo.h>
#include <qsParseTree.h>
#include <qsvProcVar.h>
#include <qsxArray.h>
#include <qcuSessionPkg.h>

extern mtdModule mtdInteger;

static mtcName qsfMDeleteFunctionName[1] = {
    { NULL, 6, (void*)"DELETE" }
};


static IDE_RC qsfMDeleteEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

IDE_RC qsfMDeleteCalculateNoArg(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC qsfMDeleteCalculate1Arg(mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC qsfMDeleteCalculate2Args(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

mtfModule qsfMDeleteModule = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfMDeleteFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfMDeleteEstimate
};

const mtcExecute qsfExecuteNoArg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfMDeleteCalculateNoArg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute qsfExecute1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfMDeleteCalculate1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute qsfExecute2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfMDeleteCalculate2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfMDeleteEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description : PROJ-1075 delete함수의 estimate
 *
 * Implementation :
 *               기본적인 routine은 일반 qsf~함수들과 같으나,
 *               host variable binding을 허용하지 않고
 *               psm내부에서만 사용이 가능하다.
 *               delete함수의 argument는 0~2개까지 올 수 있다.
 *
 *            다음과 같은 유형으로 들어올 수 있다.
 *            (1.1) var_name.delete()
 *            (1.2) var_name.delete(index)
 *            (1.3) var_name.delete(index_lower, index_upper)
 *            (2.1) label_name.var_name.delete()
 *            (2.2) label_name.var_name.delete(index)
 *            (2.3) label_name.var_name.delete(index_lower, index_upper)
 *            var_name은 qtcNode의 tableName에 해당되므로 무조건 존재해야 한다.
 *            qtcNode->userName, tableName을 이용하여 array type variable을 검색.
 *            execute->calculateInfo에 찾은 변수의 정보를 연결하여 준다.
 *            index, return value의 유형은 해당 변수의 key column type과 동일하게 생성.
 *
 ***********************************************************************/

    qcStatement     * sStatement;
    qtcNode         * sNode;
    qsVariables     * sArrayVariable;
    idBool            sIsFound;
    qcuSqlSourceInfo  sqlInfo;
    qtcColumnInfo   * sColumnInfo;
    UInt              sArgumentCount;
    const mtdModule * sModule;
    const mtdModule * sModules[2];

    sStatement       = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sNode            = (qtcNode*)aNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // 적합성 검사. tableName은 반드시 존재해야 함.
    IDE_DASSERT( QC_IS_NULL_NAME(sNode->tableName) == ID_FALSE );

    // array type 변수를 검색.
    IDE_TEST( qsvProcVar::searchArrayVar( sStatement,
                                          sNode,
                                          &sIsFound,
                                          &sArrayVariable )
              != IDE_SUCCESS );

    if( sIsFound == ID_FALSE )
    {
        sqlInfo.setSourceInfo( sStatement,
                               &sNode->tableName );
        IDE_RAISE(ERR_NOT_FOUND_VAR);
    }
    else
    {
        // return값은 integer.
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdInteger,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "qsfMDelete::qsfMDeleteEstimate::alloc::ColumnInfo" );
        IDE_TEST( aCallBack->alloc( aCallBack->info,
                                    ID_SIZEOF( qtcColumnInfo ),
                                    (void**)&sColumnInfo )
                  != IDE_SUCCESS );

        sColumnInfo->table    = sArrayVariable->common.table;
        sColumnInfo->column   = sArrayVariable->common.column;
        sColumnInfo->objectId = sArrayVariable->common.objectID;

        sArgumentCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

        switch( sArgumentCount )
        {
            case 0:
                aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecuteNoArg;
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = (void*)sColumnInfo;
                break;

            case 1:
                aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute1Arg;
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = (void*)sColumnInfo;

                // typeInfo의 첫번째 컬럼이 index column임.
                sModule = sArrayVariable->typeInfo->columns->basicInfo->module;
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    &sModule )
                          != IDE_SUCCESS );
                break;

            case 2:
                aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute2Args;
                aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = (void*)sColumnInfo;

                // typeInfo의 첫번째 컬럼이 index column임.
                sModules[0] = sArrayVariable->typeInfo->columns->basicInfo->module;
                sModules[1] = sModules[0];
                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
                break;

            default:
                IDE_ASSERT(0);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION(ERR_NOT_FOUND_VAR);
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsfMDeleteCalculateNoArg(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1075 delete()함수의 calculate
 *
 * Implementation :
 *          aInfo에서 qsxArrayInfo를 가져와서
 *          truncate함수를 호출한다.
 *          argument가 없는 경우이므로 모두 삭제한다.
 *
 ***********************************************************************/    

    qsxArrayInfo   * sArrayInfo;
    mtcColumn      * sArrayColumn;
    qtcColumnInfo  * sColumnInfo;
    mtdIntegerType * sReturnValue;
    mtcTemplate    * sTemplateForArrayVar;

    sColumnInfo = (qtcColumnInfo*)aInfo;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /* BUG-38243
       array method 사용 시, 해당 array는 해당 aTemplate이 아닌
       다른 template에 정보가 있을 수 있다. */
    if ( sColumnInfo->objectId == QS_EMPTY_OID )
    {
        sTemplateForArrayVar = aTemplate;
    }
    else
    {
        IDE_TEST( qcuSessionPkg::getTmplate( ((qcTemplate *)aTemplate)->stmt,
                                             sColumnInfo->objectId,
                                             aStack,
                                             aRemain,
                                             &sTemplateForArrayVar )
                  != IDE_SUCCESS );

        IDE_DASSERT( sTemplateForArrayVar != NULL );
    }

    sArrayColumn = sTemplateForArrayVar->rows[sColumnInfo->table].columns + sColumnInfo->column;

    // PROJ-1904 Extend UDT
    sArrayInfo = *((qsxArrayInfo ** )( (UChar*) sTemplateForArrayVar->rows[sColumnInfo->table].row
                                    + sArrayColumn->column.offset ));

    // 적합성 검사.
    IDE_TEST_RAISE( sArrayInfo == NULL, ERR_INVALID_ARRAY );

    sReturnValue = (mtdIntegerType*)aStack[0].value;

    // 다 지워지는 것이므로 지우기 전의 count를 구함.
    *sReturnValue = qsxArray::getElementsCount( sArrayInfo );

    IDE_TEST( qsxArray::truncateArray( sArrayInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfMDeleteCalculate1Arg(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1075 delete(index)함수의 calculate
 *
 * Implementation :
 *          aInfo에서 qsxArrayInfo를 가져와서
 *          deleteOneElement함수를 호출한다.
 *          argument가 하나인 경우이므로
 *          지워진 개수(return값)는 1 또는 0이다.
 ***********************************************************************/    

    qsxArrayInfo   * sArrayInfo;
    mtcColumn      * sArrayColumn;
    qtcColumnInfo  * sColumnInfo;
    mtdIntegerType * sReturnValue;
    idBool           sDeleted = ID_FALSE;
    mtcTemplate    * sTemplateForArrayVar;   

    sColumnInfo = (qtcColumnInfo*)aInfo;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /* BUG-38243
       array method 사용 시, 해당 array는 해당 aTemplate이 아닌
       다른 template에 정보가 있을 수 있다. */
    if ( sColumnInfo->objectId == QS_EMPTY_OID )
    {
        sTemplateForArrayVar = aTemplate;
    }
    else
    {
        IDE_TEST( qcuSessionPkg::getTmplate( ((qcTemplate *)aTemplate)->stmt,
                                             sColumnInfo->objectId,
                                             aStack,
                                             aRemain,
                                             &sTemplateForArrayVar )
                  != IDE_SUCCESS );

        IDE_DASSERT( sTemplateForArrayVar != NULL );
    }

    sArrayColumn = sTemplateForArrayVar->rows[sColumnInfo->table].columns + sColumnInfo->column;

    // PROJ-1904 Extend UDT
    sArrayInfo = *((qsxArrayInfo ** )( (UChar*) sTemplateForArrayVar->rows[sColumnInfo->table].row
                                    + sArrayColumn->column.offset ));

    // 적합성 검사.
    IDE_TEST_RAISE( sArrayInfo == NULL, ERR_INVALID_ARRAY );

    sReturnValue = (mtdIntegerType*)aStack[0].value;

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        // argument가 NULL인 경우는 무조건 0(null key는 없다)
        *sReturnValue = 0;
    }
    else
    {
        // index에 해당하는 element하나를 삭제.
        IDE_TEST( qsxArray::deleteOneElement( sArrayInfo,
                                              aStack[1].column,
                                              aStack[1].value,
                                              &sDeleted )
                  != IDE_SUCCESS );

        if( sDeleted == ID_TRUE )
        {
            // 삭제에 성공하면 1(지워진 개수)
            *sReturnValue = 1;
        }
        else
        {
            // 삭제에 실패하면 0(지워진 개수)
            *sReturnValue = 0;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfMDeleteCalculate2Args(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1075 delete(index_lower, index_upper)함수의 calculate
 *
 * Implementation :
 *          aInfo에서 qsxArrayInfo를 가져와서
 *          deleteElementsRange함수를 호출한다.
 *          argument가 둘인 경우이므로
 *          지워진 개수(return값)는 0 ~ 이다.
 ***********************************************************************/    

    qsxArrayInfo   * sArrayInfo;
    mtcColumn      * sArrayColumn;
    qtcColumnInfo  * sColumnInfo;
    mtdIntegerType * sReturnValue;
    mtcTemplate    * sTemplateForArrayVar;

    sColumnInfo = (qtcColumnInfo*)aInfo;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /* BUG-38243
       array method 사용 시, 해당 array는 해당 aTemplate이 아닌
       다른 template에 정보가 있을 수 있다. */
    if ( sColumnInfo->objectId == QS_EMPTY_OID )
    {
        sTemplateForArrayVar = aTemplate;
    }
    else
    {
        IDE_TEST( qcuSessionPkg::getTmplate( ((qcTemplate *)aTemplate)->stmt,
                                             sColumnInfo->objectId,
                                             aStack,
                                             aRemain,
                                             &sTemplateForArrayVar )
                  != IDE_SUCCESS );

        IDE_DASSERT( sTemplateForArrayVar != NULL );
    }

    sArrayColumn = sTemplateForArrayVar->rows[sColumnInfo->table].columns + sColumnInfo->column;

    // PROJ-1904 Extend UDT
    sArrayInfo = *((qsxArrayInfo ** )( (UChar*) sTemplateForArrayVar->rows[sColumnInfo->table].row
                                    + sArrayColumn->column.offset ));

    // 적합성 검사.
    IDE_TEST_RAISE( sArrayInfo == NULL, ERR_INVALID_ARRAY );

    sReturnValue = (mtdIntegerType*)aStack[0].value;

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        // 두 argument중 하나라도 NULL이 존재하면 0
        *sReturnValue = 0;
    }
    else
    {
        // index에 해당하는 element들을 삭제.
        // index_lower <= index_upper조건은 내부에서 체크함.
        IDE_TEST( qsxArray::deleteElementsRange( sArrayInfo,
                                                 aStack[1].column,
                                                 aStack[1].value,
                                                 aStack[2].column,
                                                 aStack[2].value,
                                                 sReturnValue )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
