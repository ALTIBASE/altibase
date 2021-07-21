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

extern mtfModule mtfLagIgnoreNulls;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdDate;
extern mtdModule mtdFloat;
extern mtdModule mtdInterval;
extern mtdModule mtdDouble;
extern mtdModule mtdVarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdBigint;

static mtcName mtfLagIgnoreNullsName[1] = {
    { NULL,  16, ( void * )"LAG_IGNORE_NULLS" }
};

static IDE_RC mtfLagIgnoreNullsEstimate( mtcNode     * aNode,
                                         mtcTemplate * aTemplate,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         mtcCallBack * aCallBack );

mtfModule mtfLagIgnoreNulls = {
    2 | MTC_NODE_OPERATOR_AGGREGATION  |
    MTC_NODE_FUNCTION_ANALYTIC_TRUE |
    MTC_NODE_FUNCTION_RANKING_TRUE,
    ~( MTC_NODE_INDEX_MASK ),
    1.0, /* default selectivity ( 비교 연산자가 아님 ) */
    mtfLagIgnoreNullsName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLagIgnoreNullsEstimate
};

IDE_RC mtfLagIgnoreNullsInitialize( mtcNode     * aNode,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    void        * aInfo,
                                    mtcTemplate * aTemplate );

IDE_RC mtfLagIgnoreNullsAggregate( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfLagIgnoreNullsFinalize( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfLagIgnoreNullsCalculate( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfLagIgnoreNullsInitialize,
    mtfLagIgnoreNullsAggregate,
    mtf::calculateNA,
    mtfLagIgnoreNullsFinalize,
    mtfLagIgnoreNullsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLagIgnoreNullsEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          /* aRemain */,
                                  mtcCallBack * aCallBack )
{
    const mtdModule * sModules[3] = { NULL, &mtdBigint, NULL };
    UInt              sArguments = (UInt)( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    IDE_TEST_RAISE( ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ) ||
                    ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 3 ),
                      ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_ARGUMENT_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    
    if ( sArguments == 1 )
    {
        /* 인자가 1개일때는 인자의 타입 그대로 사용한다. */
        mtc::initializeColumn( aStack[0].column, aStack[1].column );
    }
    else
    {
        if ( sArguments == 2 )
        {
            /* 인자가 2개일때도 인자의 타입 그대로 사용한다. */
            sModules[0] = aStack[1].column->module;
            
            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
            
            mtc::initializeColumn( aStack[0].column, aStack[1].column );
        }
        else
        {
            /* 인자가 3개일때는 default value와의 대표타입을 사용한다. */
            IDE_TEST( mtf::getComparisonModule( &(sModules[0]),
                                                aStack[1].column->module->no,
                                                aStack[3].column->module->no )
                      != IDE_SUCCESS );
            
            IDE_TEST_RAISE( sModules[0] == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );
            
            sModules[2] = sModules[0];
        
            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            if( aStack[1].column->column.size > aStack[3].column->column.size )
            {
                mtc::initializeColumn( aStack[0].column, aStack[1].column );
            }
            else
            {
                mtc::initializeColumn( aStack[0].column, aStack[3].column );
            }
        }
    }
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION(ERR_ARGUMENT_NOT_APPLICABLE  );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLagIgnoreNullsInitialize( mtcNode     * aNode,
                                    mtcStack    * /* aStack  */,
                                    SInt          /* aRemain */,
                                    void        * /* aInfo   */,
                                    mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    void            * sValueTemp;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValueTemp = ( void * )mtd::valueForModule( ( smiColumn * )sColumn,
                                                aTemplate->rows[aNode->table].row,
                                                MTD_OFFSET_USE,
                                                sColumn->module->staticNull );

    sColumn->module->null( sColumn, sValueTemp );

    *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row
                        + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    return IDE_SUCCESS;
}

IDE_RC mtfLagIgnoreNullsAggregate( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * /* aInfo */,
                                   mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    const mtdModule * sModule;

    IDE_TEST_RAISE( aRemain < 4, ERR_STACK_OVERFLOW );

    sColumn      = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack[0].column->column.offset );

    sModule          = aStack[0].column->module;

    idlOS::memcpy( aStack[0].value,
                   aStack[1].value,
                   sModule->actualSize( aStack[1].column,
                                        aStack[1].value ) );

    *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row
                        + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLagIgnoreNullsFinalize( mtcNode     *  aNode     ,
                                  mtcStack    *  aStack    ,
                                  SInt           aRemain   ,
                                  void        * /* aInfo */,
                                  mtcTemplate *  aTemplate )
{
    mtdBooleanType    sIsSet = MTD_BOOLEAN_FALSE;
    const mtcColumn * sColumn;
    const mtdModule * sModule;

    IDE_TEST_RAISE( aRemain < 4, ERR_STACK_OVERFLOW );

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

        IDE_TEST( mtf::postfixCalculate( aNode,
                                         aStack,
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );

        aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
        aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                    + aStack[0].column->column.offset );

        sIsSet =  *(mtdBooleanType*) ( ( UChar * )aTemplate->rows[aNode->table].row
                                       + sColumn[1].column.offset );

        sModule          = aStack[0].column->module;

        if ( sIsSet == MTD_BOOLEAN_FALSE )
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[3].value,
                           sModule->actualSize( aStack[3].column,
                                                aStack[3].value ) );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLagIgnoreNullsCalculate( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          /* aRemain */,
                                   void        * /* aInfo */,
                                   mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = ( void * )( ( UChar * )aTemplate->rows[aNode->table].row
                                 + aStack->column->column.offset );
    return IDE_SUCCESS;
}
