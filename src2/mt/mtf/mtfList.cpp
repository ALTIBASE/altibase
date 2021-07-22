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
 * $Id: mtfList.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfList;

static mtcName mtfListFunctionName[2] = {
    { mtfListFunctionName+1, 4, (void*)"LIST" },
    { NULL,              1, (void*)"("    }
};

static IDE_RC mtfListEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfList = {
    1|MTC_NODE_OPERATOR_LIST|
        MTC_NODE_PRINT_FMT_MISC,
    ~0,   // A4에서는 Index를 사용할 수 있도록 함.
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfListFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfListEstimate
};

IDE_RC mtfListCalculate(   mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfListCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfListEstimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt      /* aRemain */,
                        mtcCallBack* aCallBack )
{
    mtcNode * sNode;
    ULong     sLflag;

    extern mtdModule mtdList;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    /*
    IDE_TEST( mtdList.estimate( aStack[0].column,
                                1,
                                aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK,
                                0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn(aStack[0].column,
                                    & mtdList,
                                    1,
                                    aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK,
                                    0 )
              != IDE_SUCCESS );

    IDE_TEST(aCallBack->alloc( aCallBack->info,
                               aStack[0].column->column.size,
                               (void**)&(aStack[0].value))
             != IDE_SUCCESS);

    // list stack을 smiColumn.value에 기록해둔다.
    aStack[0].column->column.value = aStack[0].value;

    idlOS::memcpy( aStack[0].value,
                   aStack + 1,
                   aStack[0].column->column.size );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    //------------------------------------------------
    // List에 대한 Index 사용 가능 여부 검사
    // 모든 Argument가 Column일 경우에만
    // Node Transform에 의한 Index 사용이 가능하다.
    //------------------------------------------------

    sLflag = mtfList.lmask;
    for( sNode  = (mtcNode*)aNode->arguments;
         sNode != NULL;
         sNode  = sNode->next )
    {
        sLflag &= (sNode->lflag & MTC_NODE_INDEX_MASK);
    }
    aNode->lflag &= ~MTC_NODE_INDEX_MASK;
    aNode->lflag |= (sLflag & MTC_NODE_INDEX_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfListCalculate( mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt         aRemain,
                         void*        aInfo,
                         mtcTemplate* aTemplate )
{
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    idlOS::memcpy( aStack[0].value,
                   aStack + 1,
                   aStack[0].column->column.size );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
