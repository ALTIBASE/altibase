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
#include <mtfRegExp.h>

extern mtfModule mtfRegExpLike;
extern mtfModule mtfNotRegExpLike;

extern mtdModule mtdBoolean;
extern mtdModule mtdVarchar;
extern mtdModule mtdBinary;

extern IDE_RC mtfRegExpLikeCalculate( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

extern IDE_RC mtfRegExpLikeCalculateFast( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

extern IDE_RC mtfCompileExpression( mtcNode            * aPatternNode,
                                    mtcTemplate        * aTemplate,
                                    mtfRegExpression  ** aCompiledExpression,
                                    mtcCallBack        * aCallBack );

static mtcName mtfNotRegExpLikeFunctionName[1] = {
    { NULL, 15, (void*)"NOT REGEXP_LIKE" }
};

static IDE_RC mtfNotRegExpLikeEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfNotRegExpLike = {
    2|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_COMPARISON_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    2.0/3.0,  // TODO : default selectivity 
    mtfNotRegExpLikeFunctionName,
    &mtfRegExpLike,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNotRegExpLikeEstimate
};

static IDE_RC mtfNotRegExpLikeCalculate( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static IDE_RC mtfNotRegExpLikeCalculateFast( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotRegExpLikeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNotRegExpLikeEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         /*aRemain*/,
                                 mtcCallBack* aCallBack )
{
    mtfRegExpression  * sCompiledExpression;
    const mtdModule   * sModules[2];
    mtcNode           * sPatternNode;
    UInt                sPrecision;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) != 2 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &mtdVarchar;
    sModules[1] = sModules[0];
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // regexp_like의 결과를 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    /* regexp_like의 compiled pattern을 저장함 */
    sPrecision = MTF_REG_EXPRESSION_SIZE( aStack[2].column->precision );
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
         == MTC_NODE_REESTIMATE_FALSE )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }
    else
    {
        // not regexp_like( i1, '^ab' )
        // not regexp_like( i1, ere2bre('\d') )

        sPatternNode = mtf::convertedNode( aNode->arguments->next,
                                           aTemplate );

        if( ( sPatternNode == aNode->arguments->next )
            &&
            ( ( aTemplate->rows[sPatternNode->table].lflag & MTC_TUPLE_TYPE_MASK )
              == MTC_TUPLE_TYPE_CONSTANT ) )
        {
            IDE_TEST ( mtfCompileExpression( sPatternNode,
                                             aTemplate,
                                             &sCompiledExpression,
                                             aCallBack )
                       != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                mtfNotRegExpLikeCalculateFast;
            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                sCompiledExpression;

            // 더이상 사용하지 않음
            IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                            & mtdBinary,
                                            1,
                                            0,
                                            0 )
                     != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next ) == ID_TRUE )
         &&
         ( ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK )
             == MTC_TUPLE_TYPE_CONSTANT ) ||
           ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK )
             == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;

        // BUG-38070 undef type으로 re-estimate하지 않는다.
        if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
             ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
        {
            if ( aTemplate->rows[aTemplate->variableRow].
                 columns->module->id == MTD_UNDEF_ID )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
    }

    /* BUG-44740 mtfRegExpression 재사용을 위해 Tuple Row를 초기화한다. */
    aTemplate->rows[aNode->table].lflag &= ~MTC_TUPLE_ROW_MEMSET_MASK;
    aTemplate->rows[aNode->table].lflag |= MTC_TUPLE_ROW_MEMSET_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotRegExpLikeCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
    IDE_TEST( mtfRegExpLikeCalculate( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );

    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC mtfNotRegExpLikeCalculateFast( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    IDE_TEST( mtfRegExpLikeCalculateFast( aNode, aStack, aRemain, aInfo, aTemplate )
              != IDE_SUCCESS );
    
    if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else if( *(mtdBooleanType*)aStack[0].value == MTD_BOOLEAN_FALSE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
