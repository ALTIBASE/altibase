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
 * $Id: qsfSleep.cpp 18910 2006-11-13 01:56:34Z shsuh $
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

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName qsfFunctionName[1] = {
    { NULL, 8, (void*)"SP_PARSE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfParseModule = {
    1|MTC_NODE_OPERATOR_MISC,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpParse( 
                            mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpParse,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
    const mtdModule *sModule[2] = { &mtdInteger, &mtdVarchar };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModule )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     ID_SIZEOF( mtdInteger ),
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpParse( 
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
    qcStatement     * sStatement;
    
    sStatement      = ((qcTemplate*)aTemplate)->stmt ;
    mtdIntegerType   sCursorId;
    mtdCharType    * sSql = NULL;
    SChar          * sBuffer = NULL;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        IDE_RAISE( err_argument_not_applicable );
    }
    else
    {
        // Nothing to do.
    }

    if ( aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE )
    {
        IDE_RAISE( err_argument_not_applicable );
    }
    else
    {
        // Nothing to do.
    }
    
    sCursorId = *(mtdIntegerType *)aStack[1].value;

    sSql = (mtdCharType *)aStack[2].value;

    IDE_TEST( sStatement->qmxMem->alloc( sSql->length + 1,
                                         (void **)&sBuffer ) != IDE_SUCCESS );

    idlOS::memcpy( sBuffer,
                   sSql->value,
                   sSql->length );

    sBuffer[sSql->length] = '\0';

    IDE_TEST( qcg::parse( sStatement,
                          sCursorId,
                          sBuffer ) != IDE_SUCCESS );

    *(mtdIntegerType *)aStack[0].value = 0;
 
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_argument_not_applicable );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
