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
 *
 * Description :
 *     명시한 key 로 메시지 큐를 생성한다.
 * Syntax :
 *     MSG_CREATE_QUEUE( key value );
 *     return INTEGER;
 *     SUCCESS(0) - create msg queue
 *     FAIL(1)    - exist msg queue or system call failed.
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtdModule mtdInteger;

static mtcName mtfFunctionName[1] = {
    { NULL, 16, (void*)"MSG_CREATE_QUEUE" }
};

static IDE_RC mtfMsgCreateQueueEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfMsgCreateQueue = {
    1|MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE | MTC_NODE_EAT_NULL_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    mtfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMsgCreateQueueEstimate
};

IDE_RC mtfMsgCreateQueueCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMsgCreateQueueCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMsgCreateQueueEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt      /* aRemain */,
                                  mtcCallBack* aCallBack )
{
    const mtdModule* sModule = &mtdInteger;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        &sModule )
              != IDE_SUCCESS );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMsgCreateQueueCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{    
    mtdIntegerType * sKeyValue = NULL;
    SInt             sFlag;
    
    if ( MTU_MSG_QUEUE_PERMISSION == 0 )
    {
        sFlag = MTC_MSG_PUBLIC_PERMISSION | IPC_CREAT | IPC_EXCL;
    }
    else
    {
        sFlag = MTC_MSG_PRIVATE_PERMISSION | IPC_CREAT | IPC_EXCL;
    }
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sKeyValue = (mtdIntegerType*)aStack[1].value;

    // create msg queue
    if ( idlOS::msgget( (key_t)*sKeyValue, sFlag ) == -1 )
    {
        *(mtdIntegerType*)aStack[0].value = 1;
    }
    else
    {
        *(mtdIntegerType*)aStack[0].value = 0;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
