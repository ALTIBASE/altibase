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
 *
 * Description :
 *     BUG-41311 array type인자의 element를 반환
 *
 * Syntax :
 *     table_function_element( var, 1, 1 )
 *     RETURN var array type 변수의 첫번째 key의 첫번째 컬럼을 반환
 *
 * Implementation :
 *     - 첫번째 인자가 record type의 array type인 경우
 *     - 첫번째 인자가 primitive type의 array type인 경우
 *     - 첫번째 인자가 record type인 경우
 *     - 첫번째 인자가 list type인 경우
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <qsvEnv.h>
#include <qcuSqlSourceInfo.h>
#include <qsParseTree.h>
#include <qsvProcVar.h>
#include <qsxArray.h>
#include <qcuSessionPkg.h>

extern mtdModule mtdInteger;

static mtcName qsfTableFuncElementFunctionName[1] = {
    { NULL, 22, (void*)"TABLE_FUNCTION_ELEMENT" }
};

static IDE_RC qsfTableFuncElementEstimate( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           mtcCallBack* aCallBack );

IDE_RC qsfTableFuncElementCalculate( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtfModule qsfTableFuncElementModule = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfTableFuncElementFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfTableFuncElementEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfTableFuncElementCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfTableFuncElementEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         /*aRemain*/,
                                    mtcCallBack* /*aCallBack*/ )
{
    qtcNode         * sNode;
    qtcModule       * sQtcModule;
    qsTypes         * sTypeInfo;
    qcmColumn       * sQcmColumn = NULL;
    mtcColumn       * sColumn;
    mtdSmallintType   sColumnOrder;
    mtcStack        * sStack;
    UInt              sCount;
    UInt              sFence;
    SInt              i;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2 ) &&
                    ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // 첫번째 인자는 반드시 loop_value pseudo column이고 array/record/list type이어야 함
    sNode = (qtcNode*)(aNode->arguments);
    
    IDE_TEST_RAISE( ( sNode->node.module != &qtc::passModule ) ||
                    ( ( sNode->lflag & QTC_NODE_LOOP_VALUE_MASK )
                      != QTC_NODE_LOOP_VALUE_EXIST ),
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    IDE_TEST_RAISE( ( aStack[1].column->module->id != MTD_ASSOCIATIVE_ARRAY_ID ) &&
                    ( aStack[1].column->module->id != MTD_ROWTYPE_ID ) &&
                    ( aStack[1].column->module->id != MTD_RECORDTYPE_ID ) &&
                    ( aStack[1].column->module->id != MTD_LIST_ID ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // 두번째 인자는 반드시 loop_level pseudo column이어야 함
    sNode = (qtcNode*)(aNode->arguments->next);
    
    IDE_TEST_RAISE( ( sNode->node.arguments != NULL ) ||
                    ( ( sNode->lflag & QTC_NODE_LOOP_LEVEL_MASK )
                      != QTC_NODE_LOOP_LEVEL_EXIST ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( aStack[2].column->module->id != MTD_BIGINT_ID,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    // 세번째 인자는 반드시 숫자타입의 상수이어야 함
    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
    {
        sNode = (qtcNode*)(aNode->arguments->next->next);

        IDE_TEST_RAISE( ( mtf::convertedNode( (mtcNode*)sNode, aTemplate )
                          != (mtcNode*)sNode ) ||
                        ( ( aTemplate->rows[sNode->node.table].lflag & MTC_TUPLE_TYPE_MASK )
                          != MTC_TUPLE_TYPE_CONSTANT ),
                        ERR_INVALID_FUNCTION_ARGUMENT );

        IDE_TEST_RAISE( aStack[3].column->module->id != MTD_SMALLINT_ID,
                        ERR_INVALID_FUNCTION_ARGUMENT );

        sColumnOrder = *(mtdSmallintType*)
            mtc::value( aStack[3].column,
                        aTemplate->rows[sNode->node.table].row,
                        MTD_OFFSET_USE );
    }
    else
    {
        // table function transform을 통해 세번째 인자가 정의된 경우
        IDE_TEST_RAISE( aNode->info > MTC_TUPLE_COLUMN_ID_MAXIMUM,
                        ERR_INVALID_FUNCTION_ARGUMENT );
        
        sColumnOrder = (mtdSmallintType)aNode->info;
    }

    // range 검사
    if ( aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
    {
        sQtcModule = (qtcModule*) aStack[1].column->module;
        sTypeInfo = sQtcModule->typeInfo;
    
        sQcmColumn = sTypeInfo->columns->next;  // element type
        sColumn    = sQcmColumn->basicInfo;
    
        if ( ( sColumn->module->id >= MTD_UDT_ID_MIN ) &&
             ( sColumn->module->id <= MTD_UDT_ID_MAX ) )
        {
            // UDT중 record type만 가능
            IDE_TEST_RAISE( ( sColumn->module->id != MTD_ROWTYPE_ID ) &&
                            ( sColumn->module->id != MTD_RECORDTYPE_ID ),
                            ERR_INVALID_FUNCTION_ARGUMENT );

            sQtcModule = (qtcModule*) sColumn->module;
            sTypeInfo = sQtcModule->typeInfo;
        
            IDE_TEST_RAISE( ( sColumnOrder == MTD_SMALLINT_NULL ) ||
                            ( sColumnOrder < 1 ) ||
                            ( sColumnOrder > (SInt)sTypeInfo->columnCount ),
                            ERR_INVALID_FUNCTION_ARGUMENT );
        
            for ( i = 1, sQcmColumn = sTypeInfo->columns;
                  i < sColumnOrder;
                  i++, sQcmColumn = sQcmColumn->next );
        
            sColumn = sQcmColumn->basicInfo;
        }
        else
        {
            // primitive type
            IDE_TEST_RAISE( ( sColumnOrder == MTD_SMALLINT_NULL ) ||
                            ( sColumnOrder != 1 ),
                            ERR_INVALID_FUNCTION_ARGUMENT );
        }
    }
    else if ( ( aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
              ( aStack[1].column->module->id == MTD_RECORDTYPE_ID ) )
    {
        sQtcModule = (qtcModule*) aStack[1].column->module;
        sTypeInfo = sQtcModule->typeInfo;
        
        IDE_TEST_RAISE( ( sColumnOrder == MTD_SMALLINT_NULL ) ||
                        ( sColumnOrder < 1 ) ||
                        ( sColumnOrder > (SInt)sTypeInfo->columnCount ),
                        ERR_INVALID_FUNCTION_ARGUMENT );
        
        for ( i = 1, sQcmColumn = sTypeInfo->columns;
              i < sColumnOrder;
              i++, sQcmColumn = sQcmColumn->next );
        
        sColumn = sQcmColumn->basicInfo;
    }
    else if ( aStack[1].column->module->id == MTD_LIST_ID )
    {
        IDE_TEST_RAISE( aStack[1].column->precision <= 0,
                        ERR_INVALID_FUNCTION_ARGUMENT );

        sQtcModule = NULL;
        sStack = (mtcStack*)aStack[1].value;

        // list의 모든 element의 type이 같아야 한다.
        for( sCount = 1, sFence = aStack[1].column->precision;
             sCount < sFence;
             sCount++ )
        {
            IDE_TEST_RAISE( ( sStack[0].column->module->id !=
                              sStack[sCount].column->module->id ) ||
                            ( sStack[0].column->language->id !=
                              sStack[sCount].column->language->id ) ||
                            ( sStack[0].column->precision !=
                              sStack[sCount].column->precision ) ||
                            ( sStack[0].column->scale !=
                              sStack[sCount].column->scale ),
                            ERR_CONVERSION_NOT_APPLICABLE );
        }

        IDE_TEST_RAISE( ( sColumnOrder == MTD_SMALLINT_NULL ) ||
                        ( sColumnOrder != 1 ),
                        ERR_INVALID_FUNCTION_ARGUMENT );
        
        sColumn = sStack[0].column;
    }
    else
    {
        IDE_ASSERT( 0 );
    }
    
    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    // calculateInfo에 QcmColumn을 기록한다.
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = sQcmColumn;
    
    // return type 초기화
    mtc::initializeColumn( aStack[0].column, sColumn );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfTableFuncElementCalculate( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    qsxArrayInfo    * sArrayInfo;
    qcmColumn       * sQcmColumn;
    mtcStack        * sStack;
    mtdBigintType     sIndex;
    void            * sKey;
    void            * sData;
    idBool            sFound;
    UChar           * sValue;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_DASSERT( aStack[2].column->module->id == MTD_BIGINT_ID );

    sQcmColumn = (qcmColumn*) aInfo;
    sIndex     = *(mtdBigintType*) aStack[2].value;
    
    IDE_TEST_RAISE( sIndex < 1, ERR_INVALID_INDEX );
    
    if ( aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
    {
        // PROJ-1904 Extend UDT
        sArrayInfo = *((qsxArrayInfo**)aStack[1].value);

        IDE_TEST_RAISE( sArrayInfo == NULL, ERR_INVALID_ARRAY );

        IDE_TEST( qsxArray::searchNth( sArrayInfo,
                                       sIndex - 1,
                                       & sKey,
                                       & sData,
                                       & sFound )
                  != IDE_SUCCESS );
        
        sValue = (UChar*)sData + sQcmColumn->basicInfo->column.offset;
    }
    else if ( ( aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
              ( aStack[1].column->module->id == MTD_RECORDTYPE_ID ) )
    {
        sValue = (UChar*)aStack[1].value + sQcmColumn->basicInfo->column.offset;

        if ( sIndex == 1 )
        {
            sFound = ID_TRUE;
        }
        else
        {
            sFound = ID_FALSE;
        }
    }
    else if ( aStack[1].column->module->id == MTD_LIST_ID )
    {
        if ( sIndex <= aStack[1].column->precision )
        {
            sStack = ((mtcStack*)aStack[1].value) + sIndex - 1;
            sValue = (UChar*)sStack->value;
            
            sFound = ID_TRUE;
        }
        else
        {
            sFound = ID_FALSE;
        }
    }
    else
    {
        IDE_ASSERT( 0 );
    }
    
    if ( sFound == ID_TRUE )
    {
        idlOS::memcpy( aStack[0].value,
                       sValue,
                       aStack[0].column->module->actualSize(
                           aStack[0].column, sValue ) );
    }
    else
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
        
    return IDE_SUCCESS;
        
    IDE_EXCEPTION( ERR_INVALID_INDEX )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsfTableFuncElementCalculate",
                                  "Invalid index" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ARRAY )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
