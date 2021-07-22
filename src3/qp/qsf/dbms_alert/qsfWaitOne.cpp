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
 * $Id: qsfWaitOne.cpp 18910 2006-11-13 01:56:34Z shsuh $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxUtil.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
 
static mtcName mtfFunctionName[1] = {
    { NULL, 10, (void*)"SP_WAITONE" }
};

static IDE_RC mtfEstimate( mtcNode*     aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfWaitOneModule = {
    1|MTC_NODE_OPERATOR_MISC,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    mtfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfEstimate
};


IDE_RC mtfCalculate_SpWaitOne( 
                            mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCalculate_SpWaitOne,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfEstimate( mtcNode*     aNode,
                    mtcTemplate*    aTemplate,
                    mtcStack*    aStack,
                    SInt,
                    mtcCallBack* aCallBack )
{
    const mtdModule *sModule[4] = { &mtdVarchar, &mtdVarchar, &mtdInteger, &mtdInteger };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 4,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModule ) != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     ID_SIZEOF( mtdIntegerType ),
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

IDE_RC mtfCalculate_SpWaitOne( 
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
    void           * sSession = ((qcTemplate*)aTemplate)->stmt->session->mMmSession;
    mtdCharType    * sArg1;
    mtdCharType    * sArg2;
    mtdIntegerType * sArg3;
    mtdIntegerType * sArg4;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate ) != IDE_SUCCESS );

    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[4].column->module->isNull( aStack[4].column,
                                            aStack[4].value ) == ID_TRUE) )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        sArg1 = (mtdCharType *)aStack[1].value;
        sArg2 = (mtdCharType *)aStack[2].value;
        sArg3 = (mtdIntegerType *)aStack[3].value;
        sArg4 = (mtdIntegerType *)aStack[4].value;

        idlOS::strUpper( sArg1->value,
                         sArg1->length );

        IDE_TEST( qci::mSessionCallback.mWaitOne( sSession,
                                                  ((qcTemplate*)aTemplate)->stmt->mStatistics,
                                                  (SChar *)sArg1->value,
                                                  &sArg1->length,
                                                  (SChar *)sArg2->value,
                                                  &sArg2->length,
                                                  sArg3,
                                                  *sArg4 ) != IDE_SUCCESS );

        *(mtdIntegerType *)aStack[0].value = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
