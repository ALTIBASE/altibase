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
 * $Id$
 **********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <qcsModule.h>

extern mtdModule mtdVarchar;

static mtcName qsfSecureHashB64FunctionName[2] = {
    { qsfSecureHashB64FunctionName+1, 15, (void*)"SECURE_HASH_B64" },
    { NULL,                           13, (void*)"DAMO_HASH_B64" }
};

static IDE_RC qsfSecureHashB64Estimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

IDE_RC qsfSecureHashB64Calculate(mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

mtfModule qsfSecureHashB64Module = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfSecureHashB64FunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfSecureHashB64Estimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfSecureHashB64Calculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfSecureHashB64Estimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         /* aRemain */,
                                 mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC qsfSecureHashB64Estimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule* sModules[2];
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[0],
                                                aStack[1].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[1],
                                                aStack[2].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     256,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsfSecureHashB64Calculate(mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC qsfSecureHashB64Calculate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    mtdCharType  * sResult;
    mtdCharType  * sHashType;
    mtdCharType  * sPlainText;
    SChar          sType[QCS_HASH_TYPE_MAX_SIZE + 1];
    UChar          sHash[QCS_HASH_BUFFER_SIZE];
    UShort         sHashLength;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sResult    = (mtdCharType*)aStack[0].value;
    sPlainText = (mtdCharType*)aStack[1].value;
    sHashType  = (mtdCharType*)aStack[2].value;

    /* hash type은 10 byte를 넘지 않아야 한다. */
    IDE_TEST_RAISE( ( sHashType->length == 0 ) ||
                    ( sHashType->length > QCS_HASH_TYPE_MAX_SIZE ),
                    ERR_INVALID_HASH_TYPE );

    /* hash type은 null terminate여야 한다. */
    idlOS::memcpy( sType, sHashType->value, sHashType->length );
    sType[sHashType->length] = '\0';

    IDE_TEST( qcsModule::hashB64( sType,
                                  sPlainText->value,
                                  sPlainText->length,
                                  sHash,
                                  &sHashLength )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sHashLength > aStack[0].column->precision,
                    ERR_INVALID_HASH_LENGTH );

    if ( sHashLength > 0 )
    {
        idlOS::memcpy( sResult->value, sHash, sHashLength );
    }
    else
    {
        /* Nothing to do */
    }

    sResult->length = sHashLength;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_HASH_TYPE );
    IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ARGUMENT_TYPE,
                              "HashType" ) );
    
    IDE_EXCEPTION( ERR_INVALID_HASH_LENGTH );
    IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_ARGUMENT_TYPE,
                              "HashLength" ) );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}
