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
 * $Id: mtfTo_fpid.cpp 22529 2007-07-11 00:48:37Z sungminee $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfTo_fpid;

extern mtdModule mtdInteger;

static mtcName mtfTo_fpidFunctionName[1] = {
    { NULL, 7, (void*)"TO_FPID" }
};

static IDE_RC mtfTo_fpidEstimate( mtcNode     * aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack    * sStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule mtfTo_fpid = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfTo_fpidFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTo_fpidEstimate
};

IDE_RC mtfTo_fpidCalculate( mtcNode     * aNodeg,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_fpidCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_fpidEstimate( mtcNode     * aNode,
                           mtcTemplate* aTemplate,
                           mtcStack    * aStack,
                           SInt       /* aRemain */,
                           mtcCallBack * aCallBack )
{
    const mtdModule * sModules[1];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdInteger;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    //IDE_TEST( mtdInteger.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTo_fpidCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate )
{
    mtdIntegerType sInteger;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sInteger = *(mtdIntegerType*)aStack[1].value;

    // XXX SD_MAKE_FID를 smi로 바꾸어야 한다.
    *(mtdIntegerType*)aStack[0].value = SD_MAKE_FPID( sInteger );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
