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
 *     명시한 key 에 해당 하는 메시지 큐에 메시지를 전송(Enqueue) 한다.
 * Syntax :
 *     MSG_SND_QUEUE( key value, message );
 *     return INTEGER;
 *     SUCCESS(0) - enqueue success
 *     FAILE(1)   - not exist msg queue or system call failed.
 *          (2)   - msg size > msg buffer error
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
    { NULL, 13, (void*)"MSG_SND_QUEUE" }
};

static IDE_RC mtfMsgSndQueueEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

mtfModule mtfMsgSndQueue = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE | MTC_NODE_EAT_NULL_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    mtfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMsgSndQueueEstimate
};

IDE_RC mtfMsgSndQueueCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMsgSndQueueCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMsgSndQueueEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt      /* aRemain */,
                               mtcCallBack* aCallBack )
{    
    const mtdModule* sModules[2];

    /* BUG-44091 where 절에 round(), trunc() 오는 경우 비정상 종료합니다.  */    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &mtdInteger;
    sModules[1] = aStack[2].column->module;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aStack[2].column->module->id == MTD_LIST_ID ) ||
                    ( aStack[2].column->module->id == MTD_ROWTYPE_ID ) ||
                    ( aStack[2].column->module->id == MTD_RECORDTYPE_ID ) ||
                    ( aStack[2].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
                    ( aStack[2].column->module->id == MTD_BLOB_ID ) ||
                    ( aStack[2].column->module->id == MTD_CLOB_ID ) ||
                    ( aStack[2].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                    ( aStack[2].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                    ( aStack[2].column->module->id == MTD_ECHAR_ID ) ||
                    ( aStack[2].column->module->id == MTD_EVARCHAR_ID ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMsgSndQueueCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
    mtdIntegerType * sKeyValue = NULL;
    mtdByteType    * sByteValue = NULL;
    mtcMsgBuffer     sMsgBuffer;
    SInt             sMsgQueueID = 0;
    UInt             sActualSize = 0;
    SInt             sVarbyteActualSize = 0;
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
    
    sActualSize = aStack[2].column->module->actualSize( aStack[2].column,
                                                        aStack[2].value );

    sByteValue = (mtdByteType*)sMsgBuffer.mMessage;

    // get message queue id
    sMsgQueueID = idlOS::msgget( (key_t)*sKeyValue, sFlag );
    
    if ( sMsgQueueID == -1 )
    {
        // not exist msg queue
        *(mtdIntegerType*)aStack[0].value = 1;
    }
    else
    {
        // make msg    
        sMsgBuffer.mType = MTC_MSG_VARBYTE_TYPE;

        if ( aStack[2].column->module->id == MTD_VARBYTE_ID )
        {            
            sByteValue->length = ((mtdByteType*)aStack[2].value)->length;

            sVarbyteActualSize = sActualSize;

            // check msg size
            if ( sVarbyteActualSize > MTC_MSG_MAX_BUFFER_SIZE )
            {
                *(mtdIntegerType*)aStack[0].value = 2;
                
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                idlOS::memcpy( sByteValue->value,
                               ((mtdByteType*)aStack[2].value)->value,
                               ((mtdByteType*)aStack[2].value)->length );
            }
        }
        else
        {
            sByteValue->length = sActualSize;

            sVarbyteActualSize = sActualSize + mtdVarbyte.headerSize();

            // check msg size
            if ( sVarbyteActualSize > MTC_MSG_MAX_BUFFER_SIZE )
            {
                *(mtdIntegerType*)aStack[0].value = 2;
                
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {                
                idlOS::memcpy( sByteValue->value,
                               (UChar*)aStack[2].value,
                               sActualSize );
            }
        }
        
        // enqueue
        if ( idlOS::msgsnd( sMsgQueueID,
                            &sMsgBuffer,
                            sVarbyteActualSize,
                            IPC_NOWAIT) == -1 )
        {
            // enqueue fail
            *(mtdIntegerType*)aStack[0].value = 1;
        }
        else
        {
            *(mtdIntegerType*)aStack[0].value = 0;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
