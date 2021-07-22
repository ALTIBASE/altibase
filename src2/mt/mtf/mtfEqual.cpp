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
 * $Id: mtfEqual.cpp 86698 2020-02-18 07:17:43Z donovan.seo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNotEqual;

extern mtfModule mtfEqual;

extern mtdModule mtdList;

/* PROJ-2632 */
extern mtxModule mtxEqual;

static mtcName mtfEqualFunctionName[1] = {
    { NULL, 1, (void*)"=" }
};

static IDE_RC mtfEqualEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfEqual = {
    1|MTC_NODE_OPERATOR_EQUAL|MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_INDEX_ARGUMENT_BOTH|MTC_NODE_INDEX_JOINABLE_TRUE|
        MTC_NODE_PRINT_FMT_INFIX,
    ~0,
    1.0/10.0,  // TODO : default selectivity 
    mtfEqualFunctionName,
    &mtfNotEqual,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfEqualEstimate
};

IDE_RC mtfEqualExtractRange(  mtcNode*       aNode,
                              mtcTemplate*   aTemplate,
                              mtkRangeInfo * aInfo,
                              smiRange*      aRange );

IDE_RC mtfEqualCalculate(  mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

IDE_RC mtfEqualCalculateList(  mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfEqualCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeDefault,
    mtfEqualExtractRange
};

static const mtcExecute mtfExecuteList = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfEqualCalculateList,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfEqualEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    extern mtdModule mtdBoolean;

    mtcNode* sNode;
    mtcNode* sLeftList;
    mtcNode* sRightList;
    mtcNode* sSubTarget;
    ULong    sLflag;

    const mtdModule* sTarget;
    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack*        sStack1;
    mtcStack*        sStack2;
    UInt             sCount;
    UInt             sFence;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
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

    if( ( aCallBack->flag & MTC_ESTIMATE_ARGUMENTS_MASK )
        == MTC_ESTIMATE_ARGUMENTS_ENABLE )
    {
        for( sNode  = aNode->arguments, sLflag = MTC_NODE_INDEX_UNUSABLE;
             sNode != NULL;
             sNode  = sNode->next )
        {
            if( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
                MTC_NODE_COMPARISON_TRUE )
            {
                sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
            }
            sLflag |= sNode->lflag & MTC_NODE_INDEX_MASK;
        }

        aNode->lflag &= ~(MTC_NODE_INDEX_MASK);
        aNode->lflag |= sLflag;

        if( aStack[1].column->module != &mtdList )
        {
            IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                aStack[1].column->module->no,
                                                aStack[2].column->module->no )
                      != IDE_SUCCESS )

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            //fix BUG-17610
            if( (aNode->lflag & MTC_NODE_EQUI_VALID_SKIP_MASK) !=
                MTC_NODE_EQUI_VALID_SKIP_TRUE )
            {
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget )
                                != ID_TRUE, ERR_CONVERSION_NOT_APPLICABLE );
            }

            sModules[0] = sTarget;
            sModules[1] = sTarget;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

            /* PROJ-2632 */
            aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
                = mtxEqual.mGetExecute( sModules[0]->id, sModules[1]->id );
        }
        else
        {
            IDE_TEST_RAISE( aStack[2].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );

            IDE_TEST_RAISE( (aStack[1].column->precision !=
                             aStack[2].column->precision) ||
                            (aStack[1].column->precision <= 0),
                            ERR_INVALID_FUNCTION_ARGUMENT );

            // BUG-38133, BUG-38305
            // 아래와 같은 형태의 PASS 노드 상위의 (=) 에 대한 estimation
            // ex) CASE2
            //      |
            //     (=)-----(1)-----(=)-----(2)-----(3)
            //      |               |
            //    (SubQ)-(LIST)   (PASS)-(LIST)
            //      | ^    |       |       |
            //      | |   (1)-(1)  |      (2)-(2)
            //      | |____________|
            //      |
            //    (i1)--(i2)

            if ( ( aNode->arguments->lflag & MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK )
                == MTC_NODE_CASE_EXPRESSION_PASSNODE_TRUE )
            {
                // aNode->argumnets : Left PASS node
                // aNode->arguments->arguments : SUBQUERY node
                IDE_DASSERT( aNode->arguments->arguments != NULL );
                IDE_DASSERT( aNode->arguments->arguments->arguments != NULL );
                sLeftList = aNode->arguments->arguments;
                sSubTarget = sLeftList->arguments;

                // aStack 을 재사용할 경우 앞서 수행된 Constant 값이 훼손
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF( mtcStack ) * aStack[1].column->precision,
                                            (void**) & sStack1 )
                          != IDE_SUCCESS );

                sCount = 0;
                while( sSubTarget != NULL )
                {
                    sStack1[sCount++].column = aTemplate->rows[sSubTarget->table].columns +
                                               sSubTarget->column;
                    sSubTarget = sSubTarget->next;
                }
            }
            else
            {
                sLeftList = aNode->arguments;
                sStack1 = (mtcStack*)aStack[1].value;
            }

            if ( ( aNode->arguments->next->lflag & MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK )
                == MTC_NODE_CASE_EXPRESSION_PASSNODE_TRUE )
            {
                // aNode->argumnets->next : Right PASS node
                // aNode->arguments->next->arguments : SUBQUERY node
                IDE_DASSERT( aNode->arguments->next->arguments != NULL );
                IDE_DASSERT( aNode->arguments->next->arguments->arguments != NULL );
                sRightList= aNode->arguments->next->arguments;
                sSubTarget = sRightList->arguments;

                // aStack 을 재사용할 경우 앞서 수행된 Constant 값이 훼손
                IDE_TEST( aCallBack->alloc( aCallBack->info,
                                            ID_SIZEOF( mtcStack ) * aStack[2].column->precision,
                                            (void**) & sStack2 )
                          != IDE_SUCCESS );

                sCount = 0;
                while( sSubTarget != NULL )
                {
                    sStack2[sCount++].column = aTemplate->rows[sSubTarget->table].columns +
                                               sSubTarget->column;
                    sSubTarget = sSubTarget->next;
                }
            }
            else
            {
                sRightList= aNode->arguments->next;
                sStack2 = (mtcStack*)aStack[2].value;
            }

            for( sCount = 0, sFence = aStack[1].column->precision;
                 sCount < sFence;
                 sCount++ )
            {
                IDE_TEST( mtf::getComparisonModule( &sTarget,
                                                    sStack1[sCount].column->module->no,
                                                    sStack2[sCount].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sTarget == NULL || sTarget == &mtdList,
                                ERR_CONVERSION_NOT_APPLICABLE );

                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );

                sModules[sCount] = sTarget;
            }

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                sLeftList->arguments,
                                                aTemplate,
                                                sStack1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                sRightList->arguments,
                                                aTemplate,
                                                sStack2,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteList;
        }
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
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

IDE_RC mtfEqualExtractRange( mtcNode*       aNode,
                             mtcTemplate*   aTemplate,
                             mtkRangeInfo * aInfo,
                             smiRange*      aRange )
{
    mtcNode*          sIndexNode;
    mtcNode*          sValueNode;
    mtkRangeCallBack* sMinimumCallBack;
    mtkRangeCallBack* sMaximumCallBack;
    mtcColumn*        sValueColumn;
    void*             sValue;
    void*             sValueTemp;

    IDE_TEST_RAISE( aInfo->argument >= 2, ERR_INVALID_FUNCTION_ARGUMENT );

    if( aInfo->argument == 0 )
    {
        sIndexNode = aNode->arguments;
        sValueNode = sIndexNode->next;
    }
    else
    {
        sValueNode = aNode->arguments;
        sIndexNode = sValueNode->next;
    }

    sValueNode = mtf::convertedNode( sValueNode, aTemplate );

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
    
    sValueColumn = aTemplate->rows[sValueNode->table].columns
        + sValueNode->column;
    sValue       = aTemplate->rows[sValueNode->table].row;

    //-------------------------------------------------------------------
    // [Subquery List]
    // Node Transform에 의하여 Key Range를 생성 시,
    // 다음과 같은 질의에서 [Subquery List] 가 Value Node로 선택될 수 있다.
    //    - WHERE (I1, I2) = ( SELECT SUM(A1), SUM(A2) FROM ... )
    //
    //       [=]----------------------------------->[=]
    //        |                                      |
    //        V                                      ...
    //      [ind]----------->[ind]
    //        |                |
    //        V                V
    //       [i1]            [subquery]    : List형 Subquery는
    //                         |           : Indirection이 안됨
    //                         V
    //                       [SUM]------>[SUM]
    // 
    // 따라서, 이와 같은 경우 List형 Subquery 노드의 특성을 고려하여
    // 하위 Node의 정보를 이용한다.
    // 하위 Node의 정보는 List형 Column 정보로부터 획득할 수 있다.
    //
    // [참조]  List형 Column의 Data 구성
    //        [ [column,value], [column,value], ... ]
    //-------------------------------------------------------------------

    if ( sValueColumn->module == & mtdList )
    {
        // To Fix PR-8259
        // List형 Subquery를 의미하는 노드이다.
        // Subquery의 Argument를 이용하여 Value정보를 얻는다.
        sValueNode   = sValueNode->arguments;
        sValueNode   = mtf::convertedNode( sValueNode, aTemplate );
        
        sValueColumn = aTemplate->rows[sValueNode->table].columns +
            sValueNode->column;
        sValue       = aTemplate->rows[sValueNode->table].row;
    }
    else
    {
        // Nothing To Do
    }

    sValueTemp = (void*)mtd::valueForModule(
                             (smiColumn*)sValueColumn,
                             sValue,
                             MTD_OFFSET_USE,
                             sValueColumn->module->staticNull );

    if( sValueColumn->module->isNull( sValueColumn,
                                      sValueTemp ) == ID_TRUE )
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
        sMinimumCallBack->columnIdx  = aInfo->columnIdx;
        //sMinimumCallBack->columnDesc = NULL;
        //sMinimumCallBack->valueDesc  = NULL;
        sMinimumCallBack->value      = NULL;

        sMaximumCallBack->compare    = mtk::compareMinimumLimit;
        sMaximumCallBack->columnIdx  = aInfo->columnIdx;
        //sMaximumCallBack->columnDesc = NULL;
        //sMaximumCallBack->valueDesc  = NULL;
        sMaximumCallBack->value      = NULL;
    }
    else
    {
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
        
        sMinimumCallBack->columnIdx  = aInfo->columnIdx;
        if ( MTC_COLUMN_IS_NOT_SAME( sMinimumCallBack->columnDesc, aInfo->column ) )
        {
            sMinimumCallBack->columnDesc = *aInfo->column;
        }
        if ( MTC_COLUMN_IS_NOT_SAME( sMinimumCallBack->valueDesc, sValueColumn ) )
        {
            sMinimumCallBack->valueDesc  = *sValueColumn;
        }

        sMinimumCallBack->value      = sValue;

        sMaximumCallBack->columnIdx  = aInfo->columnIdx;
        if ( MTC_COLUMN_IS_NOT_SAME( sMaximumCallBack->columnDesc, aInfo->column ) )
        {
            sMaximumCallBack->columnDesc = *aInfo->column;
        }
        if ( MTC_COLUMN_IS_NOT_SAME( sMaximumCallBack->valueDesc, sValueColumn ) )
        {
            sMaximumCallBack->valueDesc  = *sValueColumn;
        }

        sMaximumCallBack->value      = sValue;

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
            
            sMinimumCallBack->compare = sMaximumCallBack->compare =
                mtd::findCompareFunc( aInfo->column,
                                      sValueColumn,
                                      aInfo->compValueType,
                                      aInfo->direction );
            
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfEqualCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
    const mtdModule * sModule;
    mtcStack        * sStack;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    //--------------------------------------------------------------------
    // [Subquery List]
    // Node Transform에 의하여 Key Range를 생성 시,
    // 다음과 같은 질의에서 [Subquery List] 가 Value Node로 선택될 수 있다.
    //    - WHERE (I1, I2) = ( SELECT SUM(A1), SUM(A2) FROM ... )
    //
    //       [=]----------------------------------->[=]
    //        |                                      |
    //        V                                      ...
    //      [ind]----------->[ind]
    //        |                |
    //        V                V
    //       [i1]            [subquery]    : List형 Subquery는
    //                         |           : Indirection이 안됨
    //                         V
    //                       [SUM]------>[SUM]
    //
    // 만약, 이에 대한 Key Range 생성이 실패할 경우,
    // Filter로 처리되는 데 이에 대한 고려가 필요하다.
    //
    // [참조]  List형 Column의 Data 구성
    //        [ [column,value], [column,value], ... ]
    //--------------------------------------------------------------------
    
    if ( aStack[1].column->module == & mtdList )
    {
        /* BUG-31981 Equal 연산시 list 타입처리가 잘못됨
         * 인자의 값을 접근할때 해당 인자의 node를 사용해야 합니다.
         */
        sStack = (mtcStack*) ( (SChar*) aTemplate->rows[aNode->arguments->table].row +
                               aStack[1].column->column.offset );
        aStack[1] = sStack[0];
    }
    else
    {
        // Nothing To Do
    }
    
    if ( aStack[2].column->module == & mtdList )
    {
        /* BUG-31981 Equal 연산시 list 타입처리가 잘못됨
         * 인자의 값을 접근할때 해당 인자의 node를 사용해야 합니다.
         */
        sStack = (mtcStack*) ( (SChar*) aTemplate->rows[aNode->arguments->next->table].row +
                               aStack[2].column->column.offset );
        aStack[2] = sStack[0];
    }
    else
    {
        // Nothing To Do
    }

    sModule = aStack[1].column->module;

    // BUG-22071
    IDE_ASSERT_MSG( aStack[1].column->module->id == aStack[2].column->module->id,
                    "aStack[1].column->module->id : %"ID_UINT32_FMT"\n"
                    "aStack[2].column->module->id : %"ID_UINT32_FMT"\n",
                    aStack[1].column->module->id,
                    aStack[2].column->module->id );

    if( ( sModule->isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE ) ||
        ( sModule->isNull( aStack[2].column,
                           aStack[2].value ) == ID_TRUE ) )
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
                                                             &sValueInfo2 ) == 0 )
        {
            *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
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

IDE_RC mtfEqualCalculateList( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtcStack*        sStack1;
    mtcStack*        sStack2;
    mtdBooleanType   sValue;
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
    
    sStack1 = (mtcStack*)aStack[1].value;
    sStack2 = (mtcStack*)aStack[2].value;

    sValue = MTD_BOOLEAN_TRUE;

    for( sCount = 0, sFence = aStack[1].column->precision;
         sCount < sFence && sValue != MTD_BOOLEAN_FALSE;
         sCount++ )
    {
        sModule = sStack1[sCount].column->module;
        
        if( ( sModule->isNull( sStack1[sCount].column,
                               sStack1[sCount].value ) == ID_TRUE ) ||
            ( sModule->isNull( sStack2[sCount].column,
                               sStack2[sCount].value ) == ID_TRUE ) )
        {
            sValue = mtf::andMatrix[sValue][MTD_BOOLEAN_NULL];
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
                                                                 &sValueInfo2 ) == 0 )
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
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

