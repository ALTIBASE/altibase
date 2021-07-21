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
 * $Id: qsfCheckConnectReply.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
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
extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 19, (void*)"CHECK_CONNECT_REPLY" }
};

static IDE_RC qsfEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

mtfModule qsfCheckConnectReplyModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0, /* default selectivity (비교 연산자 아님) */
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_CheckConnectReply( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * aInfo,
                                       mtcTemplate * aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_CheckConnectReply,
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
    const mtdModule* sModules[2] =
    {
        &mtdInteger,
        &mtdVarchar
    };

    const mtdModule* sModule = & mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) == MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    /* Return값은 Integer */
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

IDE_RC qsfCalculate_CheckConnectReply( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * aInfo,
                                       mtcTemplate * aTemplate )
{
    mtdIntegerType  * sReturnValue;
    mtdCharType     * sReply;
    UInt              sProtocolType;

    IDU_FIT_POINT( "qsfCheckConnectReply::qsfCalculate_CheckConnectReply::coverage::1" );
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
    {
        /* error. value argument is invalid. */
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        /* return value */
        sReturnValue  = (mtdIntegerType *)aStack[0].value;
        *sReturnValue = -1;

        /* connect type */
        sProtocolType = *(UInt *)aStack[1].value;

        /* reply varchar */
        sReply = (mtdCharType *)aStack[2].value;

        if ( sReply->length > 0 )
        {
            switch ( sProtocolType )
            {
                case QC_PROTOCOL_TYPE_SMTP:
                    if ( ( sReply->value[0] == '4' ) ||
                         ( sReply->value[0] == '5' ) )
                    {
                        IDE_RAISE( ERR_FAILURE_REPLY );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    break;
                default:
                    IDE_RAISE( ERR_INVALID_PROTOCOL_TYPE );
                    break;
            }
        }
        else
        {
            /* Nothing to do */
        }

        *sReturnValue = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION( ERR_FAILURE_REPLY );
    {
        sReply->value[sReply->length] = '\0';

        IDE_SET( ideSetErrorCode( mtERR_ABORT_SMTP_REPLY_ERROR,
                                  sReply->value ) );
    }
    IDE_EXCEPTION( ERR_INVALID_PROTOCOL_TYPE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "qsfCalculate_CheckConnectReply",
                                  "Unsupported protocol type occurred" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

