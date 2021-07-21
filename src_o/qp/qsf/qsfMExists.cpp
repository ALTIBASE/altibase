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
 * $Id: qsfMExists.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1075 array type변수의 member function EXISTS
 *
 * Syntax :
 *     arr_var.EXISTS( index );
 *     RETURN BOOLEAN  <= 해당 element의 존재 유무를 리턴
 *
 * Implementation :
 *     1. index에 해당하는 element가 있으면 TRUE, 없으면 FALSE
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <mtd.h>
#include <mtdTypes.h>
#include <qsvEnv.h>
#include <qcuSqlSourceInfo.h>
#include <qsParseTree.h>
#include <qsvProcVar.h>
#include <qsxArray.h>
#include <qcuSessionPkg.h>

extern mtdModule mtdBoolean;

static mtcName qsfMExistsFunctionName[1] = {
    { NULL, 6, (void*)"EXISTS" }
};


static IDE_RC qsfMExistsEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

IDE_RC qsfMExistsCalculate(mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

mtfModule qsfMExistsModule = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfMExistsFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfMExistsEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfMExistsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfMExistsEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description : PROJ-1075 exists함수의 estimate
 *
 * Implementation :
 *            기본적인 routine은 일반 qsf~함수들과 같으나,
 *            host variable binding을 허용하지 않고
 *            psm내부에서만 사용이 가능하다.
 *            argument가 1개인지 체크.
 *
 *            다음과 같은 유형으로 들어올 수 있다.
 *            (1) var_name.exists(index)
 *            (2) label_name.var_name.exists(index)
 *            var_name은 qtcNode의 tableName에 해당되므로 무조건 존재해야 한다.
 *            qtcNode->userName, tableName을 이용하여 array type variable을 검색.
 *            execute->calculateInfo에 찾은 변수의 정보를 연결하여 준다.
 *            index는 array변수의 key column의 module로 conversion node생성.
 *            return type은 boolean.
 *
 ***********************************************************************/    

    qcStatement     * sStatement;
    qtcNode         * sNode;
    qsVariables     * sArrayVariable;
    idBool            sIsFound;
    qcuSqlSourceInfo  sqlInfo;
    qtcColumnInfo   * sColumnInfo;
    const mtdModule * sModule;

    sStatement       = ((qcTemplate*)aTemplate)->stmt;

    // 반드시 psm 내부에서만 사용.
    IDE_TEST_RAISE( (sStatement->spvEnv->createProc == NULL) &&
                    (sStatement->spvEnv->createPkg == NULL),
                    ERR_NOT_ALLOWED );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
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
        sqlInfo.setSourceInfo(
            sStatement,
            &sNode->tableName );
        IDE_RAISE(ERR_NOT_FOUND_VAR);
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

        // 변수의 table, column정보를 execute->calculateInfo에 연결한다.
        IDE_TEST( aCallBack->alloc( aCallBack->info,
                                    ID_SIZEOF( qtcColumnInfo ),
                                    (void**)&sColumnInfo )
                  != IDE_SUCCESS );

        sColumnInfo->table    = sArrayVariable->common.table;
        sColumnInfo->column   = sArrayVariable->common.column;
        sColumnInfo->objectId = sArrayVariable->common.objectID;

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

        // return type은 boolean
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdBoolean,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED );
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_FUNCTION_NOT_ALLOWED));

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


IDE_RC qsfMExistsCalculate(mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1075 exists함수의 calculate
 *
 * Implementation :
 *          aInfo에서 qsxArrayInfo를 가져와서
 *          exists함수를 호출한다.
 *          단, index가 null인 경우도 에러를 내지 않고 정책상 FALSE.
 *
 ***********************************************************************/    

    qsxArrayInfo   * sArrayInfo;
    mtcColumn      * sArrayColumn;
    qtcColumnInfo  * sColumnInfo;
    mtdBooleanType * sReturnValue;
    idBool           sExists;
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

    sReturnValue = (mtdBooleanType*)aStack[0].value;

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        // index가 null인 경우도 error내지 않고 FALSE
        *sReturnValue = MTD_BOOLEAN_FALSE;
    }
    else
    {
        IDE_TEST( qsxArray::exists( sArrayInfo,
                                    aStack[1].column,
                                    aStack[1].value,
                                    &sExists )
                  != IDE_SUCCESS );

        if( sExists == ID_TRUE )
        {
            *sReturnValue = MTD_BOOLEAN_TRUE;
        }
        else
        {
            *sReturnValue = MTD_BOOLEAN_FALSE;
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
