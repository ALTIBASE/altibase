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
 * $Id: mtfLeast.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfLeast;

extern mtdModule mtdList;
extern mtdModule mtdDate;
extern mtdModule mtdFloat;
extern mtdModule mtdInterval;
extern mtdModule mtdVarchar;
extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;

static mtcName mtfLeastFunctionName[1] = {
    { NULL, 5, (void*)"LEAST" }
};

static IDE_RC mtfLeastEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfLeast = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfLeastFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLeastEstimate
};

IDE_RC mtfLeastCalculate(  mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLeastCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLeastEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    idBool           sNvarchar = ID_FALSE;
    UInt             sCount;
    UInt             sFence;
    UInt             sGroups[MTD_GROUP_MAXIMUM];
    const mtdModule* sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    for( sCount = 0; sCount < MTD_GROUP_MAXIMUM; sCount++ )
    {
        sGroups[sCount] = 0;
    }
    for( sCount = 1; sCount <= sFence; sCount++ )
    {
        sGroups[aStack[sCount].column->module->flag&MTD_GROUP_MASK] = 1;
    }
    if( sGroups[MTD_GROUP_NUMBER] != 0 )
    {
        sModules[0] = &mtdFloat;
    }
    else if( sGroups[MTD_GROUP_DATE] != 0 )
    {
        sModules[0] = &mtdDate;
    }
    else if( sGroups[MTD_GROUP_INTERVAL] != 0 )
    {
        sModules[0] = &mtdInterval;
    }
    else if( sGroups[MTD_GROUP_TEXT] != 0 )
    {
        /* BUG-34341 
         * 인자에 NCHAR/NVARCHAR가 하나라도 있다면,
         * NVARCHAR 타입으로 리턴한다. */

        for ( sCount = 1 ; sCount <= sFence ; sCount++ )
        {
            if ( ( aStack[sCount].column->module->id == mtdNvarchar.id ) ||
                 ( aStack[sCount].column->module->id == mtdNchar.id ) )
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
            sModules[0] = &mtdNvarchar;
        }
        else
        {
            sModules[0] = &mtdVarchar;
        }
    }
    else
    {
        sModules[0] = aStack[1].column->module;
        IDE_TEST_RAISE( sModules[0] == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
    }

    // BUG-31571
    IDE_TEST_RAISE( mtf::isGreaterLessValidType( sModules[0] ) != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    for( sCount = 1; sCount < sFence; sCount++ )
    {
        sModules[sCount] = sModules[0];
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    if( ( aStack[1].column->module->flag & MTD_NON_LENGTH_MASK ) == MTD_NON_LENGTH_TYPE )
    {
        for( sCount = 2; sCount <= sFence; sCount++ )
        {
            IDE_TEST_RAISE( !mtc::isSameType( aStack[1].column,
                                              aStack[sCount].column ),
                            ERR_INVALID_PRECISION );
        }
    }

    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( aStack[0].column, aStack[1].column );
    for( sCount = 2; sCount <= sFence; sCount++ )
    {
        if( aStack[0].column->column.size <
            aStack[sCount].column->column.size )
        {
            mtc::initializeColumn( aStack[0].column, aStack[sCount].column );
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

IDE_RC mtfLeastCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtcStack*        sLeast;
    mtcStack*        sStack;
    mtcStack*        sFence;
    mtdValueInfo     sValueInfo1;
    mtdValueInfo     sValueInfo2;    
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sModule          = aStack[0].column->module;
    
    for( sLeast = aStack + 1,
         sStack = sLeast + 1,
         sFence = sLeast +( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
         sStack < sFence;
         sStack++ )
    {
        sValueInfo1.column = sStack->column;
        sValueInfo1.value  = sStack->value;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = sLeast->column;
        sValueInfo2.value  = sLeast->value;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;
        
        if ( sModule->logicalCompare[MTD_COMPARE_DESCENDING]( &sValueInfo1,
                                                              &sValueInfo2 ) > 0 )
        {
            sLeast = sStack;
        }
    }
    
    idlOS::memcpy( aStack[0].value,
                   sLeast->value,
                   sModule->actualSize( sLeast->column,
                                        sLeast->value ) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
