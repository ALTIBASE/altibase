/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>

extern mtfModule mtfNotInlist;
extern mtfModule mtfInlist;
extern mtdModule mtdVarchar;
extern mtdModule mtdChar;
extern mtdModule mtdFloat;
extern mtdModule mtdList;

extern IDE_RC mtfInlistCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static mtcName mtfNotInlistFunctionName[1] = {
    { NULL, 10, (void*)"NOT INLIST" }
};

static IDE_RC mtfNotInlistEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfNotInlist = {
    1 | MTC_NODE_OPERATOR_NOT_EQUAL |
        MTC_NODE_COMPARISON_TRUE |
        MTC_NODE_PRINT_FMT_PREFIX_PA,
    ~(MTC_NODE_INDEX_MASK),
    2.0/3.0,  // TODO : default selectivity
    mtfNotInlistFunctionName,
    &mtfInlist,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNotInlistEstimate
};

IDE_RC mtfNotInlistCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotInlistCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNotInlistEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt      /* aRemain */,
                             mtcCallBack* aCallBack )
{
    extern mtdModule mtdBoolean;

    const mtdModule* sTarget;
    const mtdModule* sModules[2];

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if ( ( aCallBack->flag & MTC_ESTIMATE_ARGUMENTS_MASK ) ==
         MTC_ESTIMATE_ARGUMENTS_ENABLE )
    {
        /* list type is not supported */
        IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                        ( aStack[2].column->module == &mtdList ),
                        ERR_INVALID_FUNCTION_ARGUMENT );

        if ( aStack[2].column->module == &mtdChar )
        {
            IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                aStack[1].column->module->no,
                                                mtdChar.no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            //fix BUG-17610
            if ( ( aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK ) !=
                 MTC_NODE_EQUI_VALID_SKIP_TRUE )
            {
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
            }

            /* column conversion 을 붙인다. */
            sModules[0] = sTarget;
            sModules[1] = &mtdChar;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( aStack[2].column->module == &mtdVarchar )
            {
                IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                    aStack[1].column->module->no,
                                                    mtdVarchar.no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                //fix BUG-17610
                if ( (aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK) !=
                     MTC_NODE_EQUI_VALID_SKIP_TRUE )
                {
                    // To Fix PR-15208
                    IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                    != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
                }

                /* column conversion 을 붙인다. */
                sModules[0] = sTarget;
                sModules[1] = &mtdVarchar;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
            else
            {
                /*char varchar 가 아닌 경우 강제로 varchar 로 conversion */
                IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                    aStack[1].column->module->no,
                                                    mtdVarchar.no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                //fix BUG-17610
                if ( (aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK) !=
                     MTC_NODE_EQUI_VALID_SKIP_TRUE )
                {
                    // To Fix PR-15208
                    IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                    != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
                }

                /* column conversion 을 붙인다. */
                sModules[0] = sTarget;
                sModules[1] = &mtdVarchar;

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    aNode->arguments,
                                                    aTemplate,
                                                    aStack + 1,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );
            }
        }
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotInlistCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
    /* mtfInlistCalculate 함수의 결과의 반대의 결과를 리턴하도록 한다. */
    IDE_TEST( mtfInlistCalculate( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if ( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if ( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
