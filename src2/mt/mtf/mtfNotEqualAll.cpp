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
 * $Id: mtfNotEqualAll.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfEqualAny;

extern mtfModule mtfNotEqualAll;

extern mtdModule mtdList;

extern mtdModule mtdBinary;
    
static mtcName mtfNotEqualAllFunctionName[1] = {
    { NULL, 5, (void*)"<>ALL" }
};

static IDE_RC mtfNotEqualAllEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

/* Seconds Column use
 * for Saving & Maintain mtfQuantArgData */
mtfModule mtfNotEqualAll = {
    2|MTC_NODE_OPERATOR_NOT_EQUAL|
        MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_GROUP_COMPARISON_TRUE|
        MTC_NODE_GROUP_ALL|
        MTC_NODE_PRINT_FMT_INFIX,
    // ~(MTC_NODE_INDEX_MASK),
    ~0,        // A4에서는 Node Transform에 의해 인덱스 사용할 수 있음
    2.0/3.0,  // TODO : default selectivity 
    mtfNotEqualAllFunctionName,
    &mtfEqualAny,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNotEqualAllEstimate
};

IDE_RC mtfNotEqualAllCalculateSelect( mtcNode *     aNode,
                                      mtcStack *    aStack,
                                      SInt          aRemain,
                                      void *        aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfNotEqualAllCalculateFast( mtcNode *     aNode,
                                    mtcStack *    aStack,
                                    SInt          aRemain,
                                    void *        aInfo,
                                    mtcTemplate * aTemplate );

IDE_RC mtfNotEqualAllCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfNotEqualAllCalculateList( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfNotEqualAllCalculateSubquery( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfNotEqualAllCalculateSubqueryList( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotEqualAllCalculateSelect,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteList = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotEqualAllCalculateList,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteSubquery = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotEqualAllCalculateSubquery,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteSubqueryList = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotEqualAllCalculateSubqueryList,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNotEqualAllEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt      /* aRemain */,
                               mtcCallBack* aCallBack )
{
    extern mtdModule mtdBoolean;

    const mtdModule* sTarget;
    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack         sStack[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack*        sStack1;
    mtcStack*        sStack2;
    mtcStack*        sStack3;
    UInt             sCount;
    UInt             sFence;
    mtcNode*         sNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    /* allocQuantArgData at Second Column */
    IDE_TEST( mtf::allocQuantArgData( aNode,
    		                          aTemplate,
    		                          aStack,
    		                          aCallBack )
              != IDE_SUCCESS );

    if( ( aNode->arguments->next->lflag & MTC_NODE_OPERATOR_MASK )
        != MTC_NODE_OPERATOR_SUBQUERY )
    {
        if( aStack[1].column->module != &mtdList )
        {
            IDE_TEST_RAISE( aStack[2].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );

            IDE_TEST_RAISE( aStack[2].column->precision <= 0,
                            ERR_INVALID_FUNCTION_ARGUMENT );

            for( sCount  = 0,
                 sFence  = aStack[2].column->precision,
                 sStack2 = (mtcStack*)aStack[2].value;
                 sCount  < sFence;
                 sCount++ )
            {
                IDE_TEST( mtf::getComparisonModule(
                    &sTarget,
                    aStack[1].column->module->no,
                    sStack2[sCount].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL,
                                ERR_CONVERSION_NOT_APPLICABLE );

                // BUG-31571
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );

                sModules[sCount] = sTarget;
                sStack[sCount]   = aStack[1];
            }

            IDE_TEST( mtf::makeConversionNodes(                         aNode,
                                            aNode->arguments->next->arguments,
                                                                    aTemplate,
                                                   (mtcStack*)aStack[2].value,
                                                                    aCallBack,
                                                                     sModules )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeLeftConversionNodes(                     aNode,
                                            aNode->arguments->next->arguments,
                                                                    aTemplate,
                                                                       sStack,
                                                                    aCallBack,
                                                                     sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
        }
        else
        {
            IDE_TEST_RAISE( aStack[2].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );

            sStack1 = (mtcStack*)aStack[1].value;

            for( sNode   = aNode->arguments->next->arguments,
                 sStack2 = (mtcStack*)aStack[2].value;
                 sNode  != NULL;
                 sNode   = sNode->next, sStack2++ )
            {
                IDE_TEST_RAISE( sStack2->column->module != &mtdList,
                                ERR_CONVERSION_NOT_APPLICABLE );

                IDE_TEST_RAISE( (aStack[1].column->precision !=
                                 sStack2->column->precision) ||
                                (aStack[1].column->precision <= 0),
                                ERR_INVALID_FUNCTION_ARGUMENT );

                idlOS::memcpy( sStack,
                               aStack[1].value,
                               ID_SIZEOF(mtcStack)*aStack[1].column->precision );

                for( sCount  = 0,
                     sFence  = aStack[1].column->precision,
                     sStack3 = (mtcStack*)sStack2->value;
                     sCount  < sFence;
                     sCount++ )
                {
                    IDE_TEST( mtf::getComparisonModule(
                        &sTarget,
                        sStack1[sCount].column->module->no,
                        sStack3[sCount].column->module->no )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sTarget == NULL || sTarget == &mtdList,
                                    ERR_CONVERSION_NOT_APPLICABLE );

                    // BUG-31571
                    IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                    ERR_CONVERSION_NOT_APPLICABLE );

                    sModules[sCount] = sTarget;
                }

                IDE_TEST( mtf::makeConversionNodes( aNode,
                                                    sNode->arguments,
                                                    aTemplate,
                                                    sStack3,
                                                    aCallBack,
                                                    sModules )
                          != IDE_SUCCESS );

                IDE_TEST( mtf::makeLeftConversionNodes( aNode,
                                                        sNode->arguments,
                                                        aTemplate,
                                                        sStack,
                                                        aCallBack,
                                                        sModules )
                          != IDE_SUCCESS );
            }

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteList;
        }
    }
    else
    {
        if( aStack[1].column->module != &mtdList )
        {
            IDE_TEST( mtf::getComparisonModule(
                &sTarget,
                aStack[1].column->module->no,
                aStack[2].column->module->no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            // BUG-31571
            IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                            ERR_CONVERSION_NOT_APPLICABLE );

            sModules[0] = sTarget;
            sModules[1] = sTarget;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteSubquery;
        }
        else
        {
            IDE_TEST_RAISE( aStack[2].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );

            IDE_TEST_RAISE( (aStack[1].column->precision !=
                             aStack[2].column->precision) ||
                            (aStack[1].column->precision <= 0),
                            ERR_INVALID_FUNCTION_ARGUMENT );

            sStack1 = (mtcStack*)aStack[1].value;
            sStack2 = (mtcStack*)aStack[2].value;

            for( sCount = 0, sFence = aStack[1].column->precision;
                 sCount < sFence;
                 sCount++ )
            {
                IDE_TEST( mtf::getComparisonModule(
                    &sTarget,
                    sStack1[sCount].column->module->no,
                    sStack2[sCount].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL || sTarget == &mtdList,
                                ERR_CONVERSION_NOT_APPLICABLE );

                // BUG-31571
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );

                sModules[sCount] = sTarget;
            }

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->arguments,
                                                aTemplate,
                                                sStack1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes(                         aNode,
                                            aNode->arguments->next->arguments,
                                                                    aTemplate,
                                                                      sStack2,
                                                                    aCallBack,
                                                                     sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteSubqueryList;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotEqualAllCalculateSelect( mtcNode *     aNode,
                                      mtcStack *    aStack,
                                      SInt          aRemain,
                                      void *        aInfo,
                                      mtcTemplate * aTemplate )
{
	mtfQuantFuncCalcType  sCalcType;
    void *                sInfo;

    mtfCalculateFunc sFastCalcFunc;
    mtfCalculateFunc sNormalCalcFunc;

    mtfQuantFuncArgData * sData = NULL;

    sFastCalcFunc   = mtfNotEqualAllCalculateFast;
    sNormalCalcFunc = mtfNotEqualAllCalculate;

    sInfo  = mtf::getCalcInfoPtr( aNode, aTemplate );

    /* BUG-41025 */
    if (aTemplate->execInfo != NULL)
    {
        /* FASTER Function Separate First */
        if ( aTemplate->execInfo[(*(UInt*)sInfo)] == MTF_QUANTIFY_FUNC_FAST_CALC )
        {
            IDE_TEST( sFastCalcFunc( aNode, aStack, aRemain, aInfo, aTemplate )
                      != IDE_SUCCESS );
        }
        else if ( aTemplate->execInfo[(*(UInt*)sInfo)] == MTF_QUANTIFY_FUNC_NORMAL_CALC )
        {
            IDE_TEST( sNormalCalcFunc( aNode, aStack, aRemain, aInfo, aTemplate )
                      != IDE_SUCCESS );
        }
        else if ( aTemplate->execInfo[(*(UInt*)sInfo)] == MTF_QUANTIFY_FUNC_UNKNOWN )
        {
            /* Get Calculate Type */
            IDE_TEST( mtf::getQuantFuncCalcType( aNode,
                                                 aTemplate,
                                                 &sCalcType )
                      != IDE_SUCCESS );

            switch ( sCalcType )
            {
                case MTF_QUANTIFY_FUNC_NORMAL_CALC:
                    aTemplate->execInfo[(*(UInt*)sInfo)] = MTF_QUANTIFY_FUNC_NORMAL_CALC;

                    IDE_TEST( sNormalCalcFunc( aNode,
                                               aStack,
                                               aRemain,
                                               aInfo,
                                               aTemplate )
                              != IDE_SUCCESS );
                    break;

                case MTF_QUANTIFY_FUNC_FAST_CALC:
                    // BUG-37556
                    sData = (mtfQuantFuncArgData*)mtf::getArgDataPtr( aNode, aTemplate );
                    sData->mSelf = NULL;

                    aTemplate->execInfo[(*(UInt*)sInfo)] = MTF_QUANTIFY_FUNC_FAST_CALC;

                    IDE_TEST( sFastCalcFunc( aNode,
                                             aStack,
                                             aRemain,
                                             aInfo,
                                             aTemplate )
                              != IDE_SUCCESS );
                    break;

                default:
                    IDE_DASSERT( 0 );
                    break;
            }
        }
    }
    else
    {
        IDE_TEST( sNormalCalcFunc( aNode, aStack, aRemain, aInfo, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotEqualAllCalculateFast( mtcNode *     aNode,
                                    mtcStack *    aStack,
                                    SInt          aRemain,
                                    void *        aInfo,
                                    mtcTemplate * aTemplate )
{
    const mtdModule*      sMtdModule;
    mtdBooleanType        sValue;
    mtcStack              sStack;
    mtcStack *            sStackList;
    mtcNode *             sNode;
    mtcTuple *            sTuple;
    UInt                  sFence;
    mtdValueInfo          sValueInfo1;
    mtdValueInfo       	  sValueInfo2;
    mtfQuantFuncArgData * sData;

    SInt                  sFirst;
    SInt                  sMiddle;
    SInt                  sLast;
    SInt                  sCompareResult;

    IDE_TEST_RAISE( aNode->arguments->next == NULL,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sData  = (mtfQuantFuncArgData*)mtf::getArgDataPtr( aNode, aTemplate );
    if ( sData->mSelf != sData )
    {
    	IDE_TEST( mtf::buildQuantArgData( aNode,
    			                          aStack,
    			                          aRemain,
    			                          aInfo,
    			                          aTemplate )
    	          != IDE_SUCCESS );
    }
    else
    {
        /* Semi postfixCalculate :
         *    a little parts of postfixCalculate code.
         *    in this situation,
         *    only needs to get aStack[0], and aStack[1].
         *    But, postfixCalculate calculate every child nodes,
         *    is cause of low performances.                      */
        sTuple         = &aTemplate->rows[aNode->table];
        aStack->column = sTuple->columns + aNode->column;
        aStack->value  = (void*)( (UChar*) sTuple->row
                                  + aStack->column->column.offset );

        sNode  = aNode->arguments;
        sTuple = &aTemplate->rows[sNode->table];
        IDE_TEST( sTuple->execute[sNode->column].calculate(
                             sNode,
                             aStack + 1,
                             aRemain - 1,
                             sTuple->execute[sNode->column].calculateInfo,
                             aTemplate )
                  != IDE_SUCCESS);
    }

    sNode      = aNode->arguments->next->arguments;
    sStack     = aStack[1];
    sStackList = sData->mStackList;
    sFence     = sData->mElements;

    IDE_TEST( mtf::convertLeftCalculate( sNode,
                                         &sStack,
                                         1,
                                         NULL,
                                         aTemplate )
              != IDE_SUCCESS );

    sMtdModule = sStack.column->module;

    /* Binary Search */
    sFirst  = 0;
    sLast   = sFence - 1;
    sMiddle = ( sFirst + sLast ) / 2;

    sValue = MTD_BOOLEAN_TRUE;

    if ( sMtdModule->isNull( sStack.column,
                             sStack.value ) == ID_TRUE )
    {
        sValue = MTD_BOOLEAN_NULL;
    }
    else
    {
        sValueInfo1.column = sStack.column;
        sValueInfo1.value  = sStack.value;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        if ( sData->mIsNullList[sFirst] == ID_TRUE )
        {
            sValue = MTD_BOOLEAN_NULL;
        }
        else
        {
            while ( sFirst <= sLast )
            {
                sValueInfo2.column = sStackList[sMiddle].column;
                sValueInfo2.value  = sStackList[sMiddle].value;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                sCompareResult = sMtdModule->logicalCompare[MTD_COMPARE_ASCENDING](
                                                         &sValueInfo1,
                                                         &sValueInfo2 );

                if ( sCompareResult == 0 )
                {
                    /* Find and Break out.
                     * does not needs to more compare. */
                    sValue = MTD_BOOLEAN_FALSE;
                    break;
                }
                else if ( sCompareResult > 0 )
                {
                    sFirst = sMiddle + 1;
                }
                else
                {
                    sLast = sMiddle - 1;
                }

                sMiddle = ( sFirst + sLast ) / 2;
            }
        }
    }

    *(mtdBooleanType*)aStack[0].value = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC mtfNotEqualAllCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtdBooleanType   sValue;
    mtcStack         sStack;
    mtcStack*        sStack2;
    mtcNode*         sNode;
    UInt             sCount;
    UInt             sFence;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sValue = MTD_BOOLEAN_TRUE;
    
    // TASK-3876 codesonar
    IDE_TEST_RAISE( aNode->arguments->next == NULL,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    for( sCount  = 0,
         sFence  = aStack[2].column->precision,
         sNode   = aNode->arguments->next->arguments,
         sStack2 = (mtcStack*)aStack[2].value;
         sCount  < sFence && sValue != MTD_BOOLEAN_FALSE;
         sCount++, sNode = sNode->next )
    {
        sStack = aStack[1];
        
        IDE_TEST( mtf::convertLeftCalculate( sNode,
                                             &sStack,
                                             1,
                                             NULL,
                                             aTemplate )
                  != IDE_SUCCESS );
        
        sModule = sStack.column->module;
        
        if( (sModule->isNull( sStack.column,
                              sStack.value ) == ID_TRUE) ||
            (sModule->isNull( sStack2[sCount].column,
                              sStack2[sCount].value ) == ID_TRUE) )
        {
            sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_NULL];
        }
        else
        {
            sValueInfo1.column = sStack.column;
            sValueInfo1.value  = sStack.value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = sStack2[sCount].column;
            sValueInfo2.value  = sStack2[sCount].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 ) != 0 )
            {
                sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_TRUE];
            }
            else
            {
                sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_FALSE];
            }
        }
    }
    
    *(mtdBooleanType*)aStack[0].value = sValue;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotEqualAllCalculateList( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtdBooleanType   sValue;
    mtdBooleanType   sSubValue;
    mtcStack         sStack;
    mtcStack*        sStack1;
    mtcStack*        sStack2;
    mtcStack*        sStack3;
    mtcNode*         sNode2;
    mtcNode*         sNode3;
    UInt             sCount1;
    UInt             sFence1;
    UInt             sCount2;
    UInt             sFence2;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;    
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sValue = MTD_BOOLEAN_TRUE;
    
    // TASK-3876 codesonar
    IDE_TEST_RAISE( aNode->arguments->next == NULL,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    for( sCount2 = 0,
         sFence1 = aStack[1].column->precision,
         sFence2 = aStack[2].column->precision,
         sNode2  = aNode->arguments->next->arguments,
         sStack1 = (mtcStack*)aStack[1].value,
         sStack2 = (mtcStack*)aStack[2].value;
         sCount2 < sFence2 && sValue != MTD_BOOLEAN_FALSE;
         sCount2++, sNode2 = sNode2->next )
    {
        sSubValue = MTD_BOOLEAN_FALSE;
        for( sCount1 = 0,
             sNode3  = sNode2->arguments,
             sStack3 = (mtcStack*)sStack2[sCount2].value;
             sCount1 < sFence1 && sSubValue != MTD_BOOLEAN_TRUE;
             sCount1++, sNode3 = sNode3->next )
        {
            sStack = sStack1[sCount1];
        
            IDE_TEST( mtf::convertLeftCalculate( sNode3,
                                                 &sStack,
                                                 1,
                                                 NULL,
                                                 aTemplate )
                      != IDE_SUCCESS );
        
            sModule = sStack.column->module;
            
            if( (sModule->isNull( sStack.column,
                                  sStack.value ) == ID_TRUE) ||
                (sModule->isNull( sStack3[sCount1].column,
                                  sStack3[sCount1].value ) == ID_TRUE) )
            {
                sSubValue = mtf::orMatrix[sSubValue][MTD_BOOLEAN_NULL];
            }
            else
            {
                sValueInfo1.column = sStack.column;
                sValueInfo1.value  = sStack.value;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = sStack3[sCount1].column;
                sValueInfo2.value  = sStack3[sCount1].value;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;
                        
                if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                     &sValueInfo2 ) != 0 )
                {
                    sSubValue = mtf::orMatrix[sSubValue][MTD_BOOLEAN_TRUE];
                }
                else
                {
                    sSubValue = mtf::orMatrix[sSubValue][MTD_BOOLEAN_FALSE];
                }
            }
        }
        sValue = mtf::andMatrix[sValue][sSubValue];
    }
    
    *(mtdBooleanType*)aStack[0].value = sValue;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNotEqualAllCalculateSubquery( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*,
                                        mtcTemplate* aTemplate )
{
    UInt             sStage = 0;
    const mtdModule* sModule;
    mtdBooleanType   sValue;
    mtcStack*        sStack;
    SInt             sRemain;
    mtcNode*         sNode = NULL;
    idBool           sRowExist;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;
    
    sStack                 = aTemplate->stack;
    sRemain                = aTemplate->stackRemain;
    aTemplate->stack       = aStack + 2;
    aTemplate->stackRemain = aRemain - 2;
    
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );
    
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    
    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode, 
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    sModule = aStack[1].column->module;

    sNode   = sNode->next;

    sStage = 1;
    IDE_TEST( aTemplate->initSubquery( sNode, aTemplate ) != IDE_SUCCESS );
    
    IDE_TEST( aTemplate->fetchSubquery( sNode, aTemplate, &sRowExist )
              != IDE_SUCCESS );
    
    sValue = MTD_BOOLEAN_TRUE;
    
    while( sRowExist == ID_TRUE && sValue != MTD_BOOLEAN_FALSE )
    {
        if( (sModule->isNull( aStack[1].column,
                              aStack[1].value ) == ID_TRUE) ||
            (sModule->isNull( aStack[2].column,
                              aStack[2].value ) == ID_TRUE) )
        {
            sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_NULL];
        }
        else
        {
            sValueInfo1.column = aStack[1].column;
            sValueInfo1.value  = aStack[1].value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aStack[2].column;
            sValueInfo2.value  = aStack[2].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
                
            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 ) != 0 )
            {
                sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_TRUE];
            }
            else
            {
                sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_FALSE];
            }
        }
        
        IDE_TEST( aTemplate->fetchSubquery( sNode, aTemplate, &sRowExist )
                  != IDE_SUCCESS );
    }
    
    sStage = 0;
    IDE_TEST( aTemplate->finiSubquery( sNode, aTemplate ) != IDE_SUCCESS );
    
    *(mtdBooleanType*)aStack[0].value = sValue;
    
    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    switch( sStage )
    {
     case 1:
        IDE_ASSERT( aTemplate->finiSubquery( sNode, aTemplate ) == IDE_SUCCESS );
     default:
        break;
    }
    
    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}

IDE_RC mtfNotEqualAllCalculateSubqueryList( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*,
                                            mtcTemplate* aTemplate )
{
    UInt             sStage = 0;
    const mtdModule* sModule;
    mtdBooleanType   sValue;
    mtdBooleanType   sSubValue;
    mtcStack*        sStack;
    mtcStack*        sStack1;
    mtcStack*        sStack2;
    SInt             sRemain;
    mtcNode*         sNode = NULL;
    idBool           sRowExist;
    UInt             sCount;
    UInt             sFence;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;    
    sStack                 = aTemplate->stack;
    sRemain                = aTemplate->stackRemain;
    aTemplate->stack       = aStack + 2;
    aTemplate->stackRemain = aRemain - 2;
    
    IDE_TEST_RAISE( aTemplate->stackRemain < 1, ERR_STACK_OVERFLOW );
    
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    
    sStack1 = (mtcStack*)aStack[1].value;
    sStack2 = aStack + 2;
    sFence  = aStack[1].column->precision;
    
    sNode = sNode->next;

    sStage = 1;
    IDE_TEST( aTemplate->initSubquery( sNode, aTemplate ) != IDE_SUCCESS );
    
    IDE_TEST( aTemplate->fetchSubquery( sNode, aTemplate, &sRowExist )
              != IDE_SUCCESS );
    
    sValue = MTD_BOOLEAN_TRUE;
    
    while( sRowExist == ID_TRUE && sValue != MTD_BOOLEAN_FALSE )
    {
        sSubValue = MTD_BOOLEAN_FALSE;
        for( sCount = 0;
             sCount < sFence && sSubValue != MTD_BOOLEAN_TRUE;
             sCount++ )
        {
            sModule = sStack1[sCount].column->module;
            if( (sModule->isNull( sStack1[sCount].column,
                                  sStack1[sCount].value ) == ID_TRUE) ||
                (sModule->isNull( sStack2[sCount].column,
                                  sStack2[sCount].value ) == ID_TRUE) )
            {
                sSubValue = mtf::orMatrix[sSubValue][MTD_BOOLEAN_NULL];
            }
            else
            {
                sValueInfo1.column = sStack1[sCount].column;
                sValueInfo1.value  = sStack1[sCount].value;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = sStack2[sCount].column;
                sValueInfo2.value  = sStack2[sCount].value;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;
                                
                if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                     &sValueInfo2 ) != 0 )
                {
                    sSubValue = mtf::orMatrix[sSubValue][MTD_BOOLEAN_TRUE];
                }
                else
                {
                    sSubValue = mtf::orMatrix[sSubValue][MTD_BOOLEAN_FALSE];
                }
            }
        }
        
        sValue = mtf::andMatrix[sValue][sSubValue];

        IDE_TEST( aTemplate->fetchSubquery( sNode, aTemplate, &sRowExist )
                  != IDE_SUCCESS );
    }
    
    sStage = 0;
    IDE_TEST( aTemplate->finiSubquery( sNode, aTemplate ) != IDE_SUCCESS );
    
    *(mtdBooleanType*)aStack[0].value = sValue;
    
    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    switch( sStage )
    {
     case 1:
        IDE_ASSERT( aTemplate->finiSubquery( sNode, aTemplate ) == IDE_SUCCESS );
     default:
        break;
    }
    
    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}
 
