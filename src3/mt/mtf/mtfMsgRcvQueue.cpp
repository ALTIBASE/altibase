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
 *     명시한 key 에 해당 하는 메시지 큐에 메시지를 수신(Dequeue) 한다.
 * Syntax :
 *     MSG_RCV_QUEUE( key value );
 *     return VARBYTE;
 *     SUCCESS - message
 *     FAIL    - NULL (no msg and system call failed)
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
extern mtdModule mtdVarbyte;

static mtcName mtfFunctionName[1] = {
    { NULL, 13, (void*)"MSG_RCV_QUEUE" }
};

static IDE_RC mtfMsgRcvQueueEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

mtfModule mtfMsgRcvQueue = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE | MTC_NODE_EAT_NULL_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    mtfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMsgRcvQueueEstimate
};

IDE_RC mtfMsgRcvQueueCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMsgRcvQueueCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMsgRcvQueueEstimate( mtcNode*     aNode,
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

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        &sModule )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarbyte,
                                     1,
                                     MTC_MSG_MAX_BUFFER_SIZE,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMsgRcvQueueCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
    mtdIntegerType * sKeyValue = NULL;
    mtcMsgBuffer     sMsgBuffer;
    SInt             sMsgQueueID = 0;
    UInt             sActualSize = 0;
    SInt             sFlag;

    if ( MTU_MSG_QUEUE_PERMISSION == 0 )
    {
        sFlag = MTC_MSG_PUBLIC_PERMISSION;
    }
    else
    {
        sFlag = MTC_MSG_PRIVATE_PERMISSION;
    }
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sKeyValue = (mtdIntegerType*)aStack[1].value;

    // get message queue id
    sMsgQueueID = idlOS::msgget( (key_t)*sKeyValue, sFlag );
    
    if ( sMsgQueueID == -1 )
    {
        // not exist msg queue
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // dequeue
        if ( idlOS::msgrcv( sMsgQueueID,
                            &sMsgBuffer,
                            MTC_MSG_MAX_BUFFER_SIZE,
                            0,
                            IPC_NOWAIT ) == -1 )
        {
            // dequeue fail
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // type check
            if ( MTC_MSG_VARBYTE_TYPE == sMsgBuffer.mType )
            {
                sActualSize = mtdVarbyte.actualSize( aStack[0].column,
                                                     sMsgBuffer.mMessage );

                IDE_TEST_RAISE( sActualSize > MTC_MSG_MAX_BUFFER_SIZE ,
                                ERR_INVALID_LENGTH );
         
                idlOS::memcpy( (mtdByteType*)aStack[0].value,
                               (mtdByteType*)sMsgBuffer.mMessage,
                               sActualSize );
            }
            else
            {
                aStack[0].column->module->null( aStack[0].column,
                                                aStack[0].value );
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
