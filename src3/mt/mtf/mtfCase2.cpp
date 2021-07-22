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
 * $Id: mtfCase2.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfCase2;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdDate;
extern mtdModule mtdFloat;
extern mtdModule mtdInterval;
extern mtdModule mtdDouble;
extern mtdModule mtdVarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;

static mtcName mtfCase2FunctionName[2] = {
    {mtfCase2FunctionName+1 , 5, (void*)"CASE2" },
    { NULL, 4, (void*)"CASE" }
};

static IDE_RC mtfCase2Estimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfCase2 = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfCase2FunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfCase2Estimate
};

IDE_RC mtfCase2Calculate(  mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCase2Calculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCase2Estimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
#if defined(IA64_LINUX) || defined(IA64_SUSE_LINUX) /* IA64(Itanium) Series */
    // for IA64, PR-3539 
    volatile UInt    sCount;
#else
    UInt             sCount;
#endif

    UInt             sFence;
    UInt             sGroups[MTD_GROUP_MAXIMUM];
    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    idBool           sDouble   = ID_TRUE;
    idBool           sNvarchar = ID_FALSE;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sFence      = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    for( sCount = 0; sCount < sFence; sCount ++ )
    {
        sModules[sCount] = &mtdBoolean;
    }
    for( sCount = 0; sCount < MTD_GROUP_MAXIMUM; sCount++ )
    {
        sGroups[sCount] = 0;
    }
    for( sCount = 2; sCount <= sFence; sCount += 2 )
    {
        sGroups[aStack[sCount].column->module->flag&MTD_GROUP_MASK] = 1;
    }
    if( ( sFence & 1 ) == 1 )
    {
        sGroups[aStack[sFence].column->module->flag&MTD_GROUP_MASK] = 1;
    }
    if( sGroups[MTD_GROUP_NUMBER] != 0 )
    {
        sModules[1] = &mtdFloat;

        // BUG-11695
        // Case( expr1, result1, expr2, result2, ... )에서
        // result1, result2 등이 모두 double 타입이면 
        // 결과 타입을 float 대신 double로 한다.
        // performance view에서 double을 사용해야 함.
        for( sCount = 2; sCount <= sFence; sCount += 2 )
        {
            if( ( sDouble == ID_TRUE ) &&
                ( aStack[sCount].column->module->id == mtdDouble.id ) )
            {
                // Nothing to do
            }
            else
            {
                sDouble = ID_FALSE;
                break;
            }
        }
        if( ( sDouble == ID_TRUE ) &&
            ( ( sFence & 1 ) == 1 ) &&
            ( aStack[sFence].column->module->id == mtdDouble.id ) )
        {
            // Nothing to do
        }
        else
        {
            sDouble = ID_FALSE;
        }

        if( sDouble == ID_TRUE )
        {
            sModules[1] = &mtdDouble;
        }
    }
    else if( sGroups[MTD_GROUP_DATE] != 0 )
    {
        sModules[1] = &mtdDate;
    }
    else if( sGroups[MTD_GROUP_INTERVAL] != 0 )
    {
        sModules[1] = &mtdInterval;
    }
    else if( sGroups[MTD_GROUP_TEXT] != 0 )
    {
        /* BUG-34311 
         * Case( expr1, result1, expr2, result2, ... )에서
         * result 중 하나라도 NVARCHAR,NCHAR 타입인 경우 
         * 결과 타입을 NVARCHAR로 한다. */

        for( sCount = 2 ; sCount <= sFence; sCount += 2 )
        {
            if( ( aStack[sCount].column->module->id == mtdNvarchar.id ) ||
                ( aStack[sCount].column->module->id == mtdNchar.id    ) )
            {
                sNvarchar = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do
            }
        }
        if( ( sNvarchar == ID_FALSE ) && 
            ( ( sFence & 1 ) == 1 )   && 
            ( ( aStack[sFence].column->module->id == mtdNvarchar.id ) ||
              ( aStack[sFence].column->module->id == mtdNchar.id    ) ) )
        {
            sNvarchar = ID_TRUE;
        }
        else
        {
            // Nothing to do 
        }

        if( sNvarchar == ID_TRUE )
        {
            sModules[1] = &mtdNvarchar;
        }
        else
        {
            sModules[1] = &mtdVarchar;
        }
    }
    else
    {
        sModules[1] = aStack[2].column->module;
        IDE_TEST_RAISE( sModules[1] == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
    }

    for( sCount = 1; sCount < sFence; sCount += 2 )
    {
        sModules[sCount] = sModules[1];
    }
    if( ( sFence & 1 ) == 1 )
    {
        sModules[sFence-1] = sModules[1];
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    if( ( aStack[2].column->module->flag & MTD_NON_LENGTH_MASK )
        == MTD_NON_LENGTH_TYPE )
    {
        for( sCount = 4; sCount <= sFence; sCount += 2 )
        {
            IDE_TEST_RAISE( !mtc::isSameType( aStack[2].column,
                                              aStack[sCount].column ),
                            ERR_INVALID_PRECISION );
        }
        if( ( sFence & 1 ) == 1 )
        { 
            IDE_TEST_RAISE( !mtc::isSameType( aStack[2].column,
                                              aStack[sFence].column ),
                            ERR_INVALID_PRECISION );
        }
    }

    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( aStack[0].column, aStack[2].column );
    for( sCount = 4; sCount <= sFence; sCount += 2 )
    {
        if(aStack[0].column->column.size<aStack[sCount].column->column.size)
        {
            mtc::initializeColumn( aStack[0].column, aStack[sCount].column );
        }
    }
    if( ( sFence & 1 ) == 1 )
    {
        if(aStack[0].column->column.size<aStack[sFence].column->column.size)
        {
            mtc::initializeColumn( aStack[0].column, aStack[sFence].column );
        }
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCase2Calculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*,
                          mtcTemplate* aTemplate )
{
    mtcNode   * sNode;
    mtcExecute* sExecute;
    mtcStack    sTempStack;
    idBool      sIsSubQuery = ID_FALSE;
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*)aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );
    
    for( sNode  = aNode->arguments; sNode != NULL; sNode = sNode->next->next )
    {
        sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

        IDE_TEST( sExecute->calculate( sNode,
                                       aStack + 1,
                                       aRemain - 1,
                                       sExecute->calculateInfo,
                                       aTemplate )
                  != IDE_SUCCESS );

        /* BUG-44762 case when subquery
         * Case when 에 Subquery가 사용될 경우 첫번째 Calculate는 실제
         * subquery 를 수행한다. 이때 aStack[1]은 Eauql이고 aStack[2]는 Subquery
         * 의 결과를 가지고 있다 이 결과를 가지고 있다가 다음 when calculate시에
         * 활용한다.
         */
        if ( sNode->arguments != NULL )
        {
            if ( ( ( sNode->arguments->lflag & MTC_NODE_HAVE_SUBQUERY_MASK )
                   == MTC_NODE_HAVE_SUBQUERY_TRUE ) &&
                 ( sNode == aNode->arguments ) )
            {
                sTempStack = aStack[2];
                sIsSubQuery = ID_TRUE;
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

        if( sNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sNode, 
                                             aStack + 1,
                                             aRemain - 1,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }
        if( sNode->next != NULL )
        {
            if( *(mtdBooleanType*)aStack[1].value == MTD_BOOLEAN_TRUE )
            {
                sNode    = sNode->next;
                sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

                IDE_TEST( sExecute->calculate( sNode,
                                               aStack + 1,
                                               aRemain - 1,
                                               sExecute->calculateInfo,
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
                idlOS::memcpy( aStack[0].value,
                               aStack[1].value,
                               aStack[1].column->module->actualSize(
                                   aStack[1].column,
                                   aStack[1].value ) );
                break;
            }
        }
        else
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           aStack[1].column->module->actualSize(
                               aStack[1].column,
                               aStack[1].value ) );
            break;
        }

        /* BUG-44762 case when subquery
         * Conversion Node가 When 구문마다 각각 달릴 수 있기 때문에
         * 최조의 Subquery 결과를 계속 유지해줘야만 한다.
         */
        if ( sIsSubQuery == ID_TRUE )
        {
            aStack[2] = sTempStack;
        }
        else
        {
            /* Nothing to do */
        }
    }
    if( sNode == NULL )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
