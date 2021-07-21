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
 * $Id: mtfBetween.cpp 86698 2020-02-18 07:17:43Z donovan.seo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNotBetween;

extern mtfModule mtfBetween;

extern mtdModule mtdList;

static mtcName mtfBetweenFunctionName[1] = {
    { NULL, 7, (void*)"BETWEEN" }
};

static IDE_RC mtfBetweenEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfBetween = {
    1|MTC_NODE_OPERATOR_RANGED|MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_PRINT_FMT_MISC,
    ~0,
    1.0/9.0,  // TODO : default selectivity 
    mtfBetweenFunctionName,
    &mtfNotBetween,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBetweenEstimate
};

static IDE_RC mtfBetweenExtractRange( mtcNode*       aNode,
                                      mtcTemplate*   aTemplate,
                                      mtkRangeInfo * aInfo,
                                      smiRange*      aRange );

IDE_RC mtfBetweenCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBetweenCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeDefault,
    mtfBetweenExtractRange
};

IDE_RC mtfBetweenEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    extern mtdModule mtdBoolean;

    mtcNode* sNode;

    const mtdModule* sModules[3];

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sNode = aNode->arguments;
    if( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
        MTC_NODE_COMPARISON_TRUE )
    {
        sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
    }
    aNode->lflag &= ~(MTC_NODE_INDEX_MASK);
    aNode->lflag |= sNode->lflag & MTC_NODE_INDEX_MASK;
    for( sNode  = sNode->next;
         sNode != NULL;
         sNode  = sNode->next )
    {
        sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // BUG-11177 fix
    // i1 between a and b에서
    // a, b가 항상 i1 컬럼의 타입으로 변환되서는 안된다.
    // mtf의 comparisonTable을 참조해서
    // i1, a, b가 서로 비교될 수 있는 동일된 타입을 구해와야 한다.
    // by kumdory, 2005-04-15
    IDE_TEST( mtf::getComparisonModule( &sModules[0],
                                        aStack[1].column->module->no,
                                        aStack[2].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sModules[0] == NULL, ERR_CONVERSION_NOT_APPLICABLE );

    // To Fix BUG-15205
    // 호환 가능한 연산자 타입 여부 검사
    IDE_TEST_RAISE( mtf::isGreaterLessValidType( sModules[0] ) != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    IDE_TEST( mtf::getComparisonModule( &sModules[0],
                                        sModules[0]->no,
                                        aStack[3].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sModules[0] == NULL, ERR_CONVERSION_NOT_APPLICABLE );

    // To Fix BUG-15205
    IDE_TEST_RAISE( mtf::isGreaterLessValidType( sModules[0] ) != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    sModules[1] = sModules[0];
    sModules[2] = sModules[0];

    IDE_TEST_RAISE( sModules[0] == &mtdList, ERR_CONVERSION_NOT_APPLICABLE );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

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

IDE_RC mtfBetweenExtractRange( mtcNode*      aNode,
                               mtcTemplate*  aTemplate,
                               mtkRangeInfo* aInfo,
                               smiRange*     aRange )
{
    mtcNode*          sIndexNode = NULL;
    mtcNode*          sValueNode1 = NULL;
    mtcNode*          sValueNode2 = NULL;
    mtkRangeCallBack* sMinimumCallBack;
    mtkRangeCallBack* sMaximumCallBack;
    mtcColumn*        sValueColumn1;
    mtcColumn*        sValueColumn2;
    void*             sValue1;
    void*             sValue2;
    void*             sValue1Temp;
    void*             sValue2Temp;

    switch( aInfo->direction )
    {
        case MTD_COMPARE_ASCENDING:
            sIndexNode  = aNode->arguments;
            sValueNode1 = sIndexNode->next;
            sValueNode2 = sValueNode1->next;
            break;
        case MTD_COMPARE_DESCENDING:
            sIndexNode  = aNode->arguments;
            sValueNode2 = sIndexNode->next;
            sValueNode1 = sValueNode2->next;
            break;

        default:
            ideLog::log( IDE_ERR_0,
                         "aInfo->direction : %du\n",
                         aInfo->direction );
            IDE_ASSERT( 0 );
    }
    
    sValueNode1 = mtf::convertedNode( sValueNode1, aTemplate );
    sValueNode2 = mtf::convertedNode( sValueNode2, aTemplate );
    
    sMinimumCallBack = (mtkRangeCallBack*)( aRange + 1 );
    sMaximumCallBack = sMinimumCallBack + 1;
    
    aRange->prev           = NULL;
    aRange->next           = NULL;
    aRange->minimum.data   = sMinimumCallBack;
    aRange->maximum.data   = sMaximumCallBack;
    sMinimumCallBack->next = NULL;
    sMaximumCallBack->next = NULL;
    sMinimumCallBack->flag = 0;
    sMaximumCallBack->flag = 0;
    
    sValueColumn1 = aTemplate->rows[sValueNode1->table].columns
                    + sValueNode1->column;
    sValue1       = aTemplate->rows[sValueNode1->table].row;
    sValue1Temp   = (void*)mtd::valueForModule(
                                (smiColumn*)sValueColumn1,
                                sValue1,
                                MTD_OFFSET_USE,
                                sValueColumn1->module->staticNull );

    sValueColumn2 = aTemplate->rows[sValueNode2->table].columns
                    + sValueNode2->column;
    sValue2       = aTemplate->rows[sValueNode2->table].row;
    sValue2Temp   = (void*)mtd::valueForModule(
                                (smiColumn*)sValueColumn2,
                                sValue2,
                                MTD_OFFSET_USE,
                                sValueColumn2->module->staticNull );

    if( ( sValueColumn1->module->isNull( sValueColumn1,
                                         sValue1Temp ) == ID_TRUE ) ||
        ( sValueColumn2->module->isNull( sValueColumn2,
                                         sValue2Temp ) == ID_TRUE ) )
    {
        if ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
             aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL )
        {
            // mtd type의 column value에 대한 range callback
            aRange->minimum.callback     = mtk::rangeCallBackGT4Mtd;
            aRange->maximum.callback     = mtk::rangeCallBackLT4Mtd;
        }
        else
        {
            if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
                 ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
            {
                /* MTD_COMPARE_STOREDVAL_MTDVAL
                   stored type의 column value에 대한 range callback */
                aRange->minimum.callback     = mtk::rangeCallBackGT4Stored;
                aRange->maximum.callback     = mtk::rangeCallBackLT4Stored;
            }
            else
            {
                /* PROJ-2433 */
                aRange->minimum.callback     = mtk::rangeCallBackGT4IndexKey;
                aRange->maximum.callback     = mtk::rangeCallBackLT4IndexKey;
            }
        }
        
        sMinimumCallBack->compare    = mtk::compareMinimumLimit;
        //sMinimumCallBack->columnDesc = NULL;
        //sMinimumCallBack->valueDesc  = NULL;
        sMinimumCallBack->value      = NULL;
        sMaximumCallBack->compare    = mtk::compareMinimumLimit;
        //sMaximumCallBack->columnDesc = NULL;
        //sMaximumCallBack->valueDesc  = NULL;
        sMaximumCallBack->value      = NULL;

    }
    else
    {
        // BETWEEN의 경우 Key Filter에 대한 별도의 고려가 필요 없다.
        // Key Filter를 위한 성능 향상을 위한 별도의 조치가 없음.

        //---------------------------
        // RangeCallBack 설정
        //---------------------------
        
        if ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
             aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL )
        {
            // mtd type의 column value에 대한 range callback
            aRange->minimum.callback     = mtk::rangeCallBackGE4Mtd;
            aRange->maximum.callback     = mtk::rangeCallBackLE4Mtd;
        }
        else
        {
            if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
                 ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
            {
                /* MTD_COMPARE_STOREDVAL_MTDVAL
                   stored type의 column value에 대한 range callback */
                aRange->minimum.callback     = mtk::rangeCallBackGE4Stored;
                aRange->maximum.callback     = mtk::rangeCallBackLE4Stored;
            }
            else
            {
                /* PROJ-2433 */
                aRange->minimum.callback     = mtk::rangeCallBackGE4IndexKey;
                aRange->maximum.callback     = mtk::rangeCallBackLE4IndexKey;
            }
        }

        //----------------------------------------------
        // MinimumCallBack & MaximumCallBack 정보 설정
        //----------------------------------------------
        
        sMinimumCallBack->columnIdx  =  aInfo->columnIdx;
        if ( MTC_COLUMN_IS_NOT_SAME( sMinimumCallBack->columnDesc, aInfo->column ) )
        {
            sMinimumCallBack->columnDesc = *aInfo->column;
        }
        if ( MTC_COLUMN_IS_NOT_SAME( sMinimumCallBack->valueDesc, sValueColumn1 ) )
        {
            sMinimumCallBack->valueDesc  = *sValueColumn1;
        }

        sMinimumCallBack->value      = sValue1;
        
        sMaximumCallBack->columnIdx  =  aInfo->columnIdx;
        if ( MTC_COLUMN_IS_NOT_SAME( sMaximumCallBack->columnDesc, aInfo->column ) )
        {
            sMaximumCallBack->columnDesc = *aInfo->column;
        }
        if ( MTC_COLUMN_IS_NOT_SAME( sMaximumCallBack->valueDesc, sValueColumn2 ) )
        {
            sMaximumCallBack->valueDesc  = *sValueColumn2;
        }

        sMaximumCallBack->value      = sValue2;
         
        // PROJ-1364
        if( aInfo->isSameGroupType == ID_FALSE )
        {
            sMinimumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
            sMinimumCallBack->flag |= MTK_COMPARE_SAMEGROUP_FALSE;

            sMaximumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
            sMaximumCallBack->flag |= MTK_COMPARE_SAMEGROUP_FALSE;

            if ( aInfo->direction == MTD_COMPARE_ASCENDING )
            {
                sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;

                sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;
            }
            else
            {
                sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;

                sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;
            }
            
            sMinimumCallBack->compare    =
                aInfo->column->module->keyCompare[aInfo->compValueType]
                                                 [aInfo->direction];
            
            sMaximumCallBack->compare    =
                aInfo->column->module->keyCompare[aInfo->compValueType]
                                                 [aInfo->direction];
        }
        else
        {
            sMinimumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
            sMinimumCallBack->flag |= MTK_COMPARE_SAMEGROUP_TRUE;

            sMaximumCallBack->flag &= ~MTK_COMPARE_SAMEGROUP_MASK;
            sMaximumCallBack->flag |= MTK_COMPARE_SAMEGROUP_TRUE;

            if ( aInfo->direction == MTD_COMPARE_ASCENDING )
            {
                sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;

                sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_ASC;
            }
            else
            {
                sMinimumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMinimumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;

                sMaximumCallBack->flag &= ~MTK_COMPARE_DIRECTION_MASK;
                sMaximumCallBack->flag |= MTK_COMPARE_DIRECTION_DESC;
            }
                
            sMinimumCallBack->compare =
                mtd::findCompareFunc( aInfo->column,
                                      sValueColumn1,
                                      aInfo->compValueType,
                                      aInfo->direction );
            
            sMaximumCallBack->compare =
                mtd::findCompareFunc( aInfo->column,
                                      sValueColumn2,
                                      aInfo->compValueType,
                                      aInfo->direction );

        }
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfBetweenCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
    const mtdModule * sModule;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sModule = aStack[1].column->module;
    
    if( ( sModule->isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE ) ||
        ( sModule->isNull( aStack[2].column,
                           aStack[2].value ) == ID_TRUE ) ||
        ( sModule->isNull( aStack[3].column,
                           aStack[3].value ) == ID_TRUE ) )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_NULL;
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
                                                             &sValueInfo2 ) >= 0 )
        {
            sValueInfo1.column = aStack[1].column;
            sValueInfo1.value  = aStack[1].value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aStack[3].column;
            sValueInfo2.value  = aStack[3].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
                    
            if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                 &sValueInfo2 ) <= 0 )
            {
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
            }
            else
            {
                *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
            }
        }
        else
        {
            *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
        }
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
