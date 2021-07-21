/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: qsfWriteRawValue.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <qsf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxUtil.h>
#include <qcuSessionObj.h>

extern mtdModule mtsConnect;
extern mtdModule mtdVarbyte;
extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 15, (void*)"WRITE_RAW_VALUE" }
};

static IDE_RC qsfEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

mtfModule qsfWriteRawValueModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0, /* default selectivity (비교 연산자 아님) */
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_WriteRawValue( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_WriteRawValue,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode     * aNode,
                    mtcTemplate * aTemplate,
                    mtcStack    * aStack,
                    SInt       /* aRemain */,
                    mtcCallBack * aCallBack )
{
    const mtdModule* sModules[3] =
    {
        &mtsConnect,
        &mtdVarbyte,
        &mtdInteger
    };

    const mtdModule* sModule = &mtdInteger;

    IDU_FIT_POINT_RAISE( "qsfWriteRawValue::qsfEstimate::coverage::1", ERR_NOT_AGGREGATION );
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDU_FIT_POINT_RAISE( "qsfWriteRawValue::qsfEstimate::coverage::2", ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDU_FIT_POINT( "qsfWriteRawValue::qsfEstimate::coverage::3" );
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    /* return값은 Integer */
    IDU_FIT_POINT( "qsfWriteRawValue::qsfEstimate::coverage::4" );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Varbyte 타입의 Value에 기록되는 Length 데이터 크기 */
#define MTD_VARBAYE_LENGTH_SIZE 2

IDE_RC qsfCalculate_WriteRawValue( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate )
{
    qcStatement     * sStatement;
    qcSession       * sSession;
    mtsConnectType  * sConnectType;
    mtdByteType     * sMessage;
    mtdIntegerType    sMessageLength;
    mtdIntegerType  * sReturnValue;
    SInt              sMsgLen;
    PDL_SOCKET        sSocket;
    
    sStatement = ((qcTemplate*)aTemplate)->stmt;
    
    sSession  = sStatement->spxEnv->mSession;

    IDU_FIT_POINT( "qsfWriteRawValue::qsfCalculate_WriteRawValue::coverage::1" );
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) ||
         (aStack[3].column->module->isNull( aStack[3].column,
                                            aStack[3].value ) == ID_TRUE) )
    {
        /* error. value argument is invalid. */
        IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
    }
    else
    {
        /* return value */
        sReturnValue  = (mtdIntegerType*)aStack[0].value;
        *sReturnValue = -1;

        /* connect type */
        sConnectType = (mtsConnectType*)aStack[1].value;
        
        /* get socket */
        IDU_FIT_POINT( "qsfWriteRawValue::qsfCalculate_WriteRawValue::coverage::2" );
        IDE_TEST( qcuSessionObj::getConnectionSocket( (qcSessionObjInfo*)(sSession->mQPSpecific.mSessionObj),
                                                      sConnectType->connectionNodeKey,
                                                      &sSocket )
                  != IDE_SUCCESS );
        
        if ( sSocket == PDL_INVALID_SOCKET )
        {
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
        
        /* message varbyte */
        sMessage = (mtdByteType*)aStack[2].value;
        
        /* message length */
        sMessageLength = *(mtdIntegerType*)aStack[3].value;

        IDU_FIT_POINT_RAISE( "qsfWriteRawValue::qsfCalculate_WriteRawValue::coverage::3", ERR_MSGLEN );
        IDE_TEST_RAISE( ( ( sMessageLength > MTD_VARCHAR_PRECISION_MAXIMUM ) ||
                          ( sMessageLength < 0 ) ),
                        ERR_MSGLEN );

        IDU_FIT_POINT_RAISE( "qsfWriteRawValue::qsfCalculate_WriteRawValue::coverage::4", ERR_MSGLEN );
        IDE_TEST_RAISE( ( sMessage->length - MTD_VARBAYE_LENGTH_SIZE ) < sMessageLength,
                        ERR_MSGLEN );
       
        sMsgLen = idlVA::send_i( sSocket,
                                 (SChar*)sMessage->value + MTD_VARBAYE_LENGTH_SIZE,
                                 sMessageLength );

        /* for coverage and fit test, remove if statement */
        IDU_FIT_POINT_RAISE( "qsfWriteRawValue::qsfCalculate_WriteRawValue::coverage::5", NORMAL_PROCESS );
        IDE_TEST_RAISE( sMsgLen >= 0, NORMAL_PROCESS );

        qcuSessionObj::setConnectionState( (qcSessionObjInfo*)(sSession->mQPSpecific.mSessionObj),
                                           sConnectType->connectionNodeKey,
                                           QC_CONNECTION_STATE_NOCONNECT );

        IDE_EXCEPTION_CONT( NORMAL_PROCESS );

        *sReturnValue = sMsgLen;
    }
    
    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MSGLEN );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_VALIDATE_INVALID_LENGTH ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;
   
    return IDE_FAILURE;
}

