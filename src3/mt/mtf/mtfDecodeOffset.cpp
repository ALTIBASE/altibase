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
 * $Id: mtfDecodeOffset.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdFloat;
extern mtdModule mtdInteger;
extern mtdModule mtdDate;
extern mtdModule mtdInterval;

static mtcName mtfDecodeOffsetFunctionName[1] = {
    { NULL, 13, (void*)"DECODE_OFFSET" }
};

static IDE_RC mtfDecodeOffsetEstimate( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

mtfModule mtfDecodeOffset= {
    1 | MTC_NODE_OPERATOR_FUNCTION | MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeOffsetFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDecodeOffsetEstimate
};

IDE_RC mtfDecodeOffsetCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDecodeOffsetCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDecodeOffsetEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt      /* aRemain */,
                                mtcCallBack* aCallBack )
{
    UInt             sCount;
    UInt             sFence;
    UInt             sGroups[MTD_GROUP_MAXIMUM];
    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    idBool           sNvarchar = ID_FALSE;

    // Argument가 최소 두개 있어야 한다. EX : DECODE_OFFSET( 1, A )
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    // DECODE_OFFSET의 첫 번 째 인자는 offset을 나타내는 정수형이 온다.
    sModules[0] = &mtdInteger;

    // Initialization for type group
    for ( sCount = 0; sCount < MTD_GROUP_MAXIMUM; sCount++ )
    {
        sGroups[sCount] = 0;
    }

    // 두 번 째 인자부터, 어떤 그룹에 속하는 type인지 구한다.
    for ( sCount = 2; sCount <= sFence; sCount++ )
    {
        sGroups[aStack[sCount].column->module->flag & MTD_GROUP_MASK] = 1;
    }

    if ( ( sGroups[MTD_GROUP_NUMBER] != 0 ) && ( sGroups[MTD_GROUP_TEXT] == 0  ) &&
         ( sGroups[MTD_GROUP_DATE]   == 0 ) && ( sGroups[MTD_GROUP_MISC] == 0 ) &&
         ( sGroups[MTD_GROUP_INTERVAL] == 0 )  )
    {
        // 숫자 형 만 있을 경우 리턴 타입은 float이 된다.
        sModules[1] = &mtdFloat;
    }
    else if ( ( sGroups[MTD_GROUP_NUMBER] == 0 ) && ( sGroups[MTD_GROUP_TEXT] == 0 ) &&
              ( sGroups[MTD_GROUP_DATE] != 0 ) && ( sGroups[MTD_GROUP_MISC] == 0 ) &&
              ( sGroups[MTD_GROUP_INTERVAL] == 0 ) )
    {
        // Date 형 만 있을 경우 리턴 타입은 date가 된다.
        sModules[1] = &mtdDate;
    }
    else if ( ( sGroups[MTD_GROUP_NUMBER] == 0 ) && ( sGroups[MTD_GROUP_TEXT] == 0 ) &&
              ( sGroups[MTD_GROUP_DATE] == 0 ) && ( sGroups[MTD_GROUP_MISC] == 0 ) &&
              ( sGroups[MTD_GROUP_INTERVAL] != 0 ) )
    {
        // Interval 형 만 있을 경우 리턴 타입은 interval이 된다.
        sModules[1] = &mtdInterval;
    }
    else
    {
        // NULL 타입을 포함한 그 밖의 타입들이 존재 할 경우,
        // 리턴 타입은 varchar가 된다.
        for ( sCount = 2; sCount <= sFence; sCount++ )
        {
            // 특별히, Nvarchar 또는 Nchar가 존재 할 경우 리턴 타입은 Nvarchar로 결정된다.
            if ( ( aStack[sCount].column->module->id == mtdNvarchar.id ) ||
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

        if ( sNvarchar == ID_TRUE )
        {
            sModules[1] = &mtdNvarchar;
        }
        else
        {
            sModules[1] = &mtdVarchar;
        }
    }

    // 위에서 구해진 대표타입( 리턴타입 )으로 컨버젼 노드를 생성한다. ( 첫 번 째 인자는 integer )
    for ( sCount = 2; sCount < sFence; sCount++ )
    {
        sModules[sCount] = sModules[1];
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // Byte는 length 필드가 없다.
    if ( ( aStack[1].column->module->flag & MTD_NON_LENGTH_MASK )
         == MTD_NON_LENGTH_TYPE )
    {
        // Byte의 경우 Precision, Scale이 동일한 경우에만 동작하도록 한다.
        for ( sCount = 2; sCount <= sFence; sCount++ )
        {
            IDE_TEST_RAISE( !mtc::isSameType( aStack[1].column,
                                              aStack[sCount].column ),
                            ERR_INVALID_PRECISION );
        }
    }
    else
    {
        // Nothing to do.
    }

    // 리턴 타입의 초기화
    mtc::initializeColumn( aStack[0].column, aStack[2].column );

    for ( sCount = 3; sCount <= sFence; sCount++ )
    {
        // Size가 가장 큰 column으로 초기화 환다.
        if ( aStack[0].column->column.size < aStack[sCount].column->column.size )
        {
            mtc::initializeColumn( aStack[0].column, aStack[sCount].column );
        }
        else
        {
            // Nothing to do.
        }
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDecodeOffsetCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*,
                                 mtcTemplate* aTemplate )
{
    mtcNode        * sNode;
    SInt             sValue;
    SInt             sCount;    
    UInt             sFence;
    mtcExecute     * sExecute;    

    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );

    // Set return stack
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*)aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );


    // Calculate the first argument( offset )
    sNode    = aNode->arguments;
    sExecute = MTC_TUPLE_EXECUTE( &aTemplate->rows[sNode->table], sNode );

    IDE_TEST( sExecute->calculate( sNode,
                                   aStack + 1,
                                   aRemain - 1,
                                   sExecute->calculateInfo,
                                   aTemplate )
              != IDE_SUCCESS );

    if ( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sValue = *(mtdIntegerType*)aStack[1].value;

    if ( sValue == MTD_INTEGER_NULL )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

        if ( sValue == 0 )
        {
            sValue++;
        }
        else
        {
            // Nothing to do.
        }

        // DECODE_OFFSET( offset_value, 0 or 1 or -5, 2 or -4, 3 or -3, 4 or -2, 5 or -1 )
        if ( sValue < 0 )
        {
            sValue += sFence;
        }
        else
        {
            // Nothing to do.
        }

        // 범위를 넘으면 에러
        IDE_TEST_RAISE( ( sValue <= 0 ) || ( sValue > ( sFence - 1 ) ),
                         ERR_ARGUMENT_NOT_APPLICABLE );

        // Calculate the Nth( offset_value ) node
        for ( sNode = sNode->next, sCount = 0;
              sCount < sValue - 1;
              sNode = sNode->next, sCount++ )
        {
            // Nothing to do.
        }

        sExecute = MTC_TUPLE_EXECUTE( &aTemplate->rows[sNode->table], sNode );

        IDE_TEST( sExecute->calculate( sNode,
                                       aStack + 2,
                                       aRemain - 2,
                                       sExecute->calculateInfo,
                                       aTemplate )
                  != IDE_SUCCESS );

        if ( sNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sNode, 
                                             aStack + 2,
                                             aRemain - 2,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nohting to do.
        }

        if ( aStack[2].column->module->isNull( aStack[2].column,
                                               aStack[2].value )
             == ID_TRUE )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[2].value,
                           aStack[2].column->module->actualSize( aStack[2].column,
                                                                 aStack[2].value ) );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
