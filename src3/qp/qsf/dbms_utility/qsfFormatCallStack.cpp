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
 * $Id: qsfPrint.cpp 67758 2014-12-01 09:59:34Z khkwak $
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

extern mtdModule mtdVarchar;

static mtcName qsfFunctionName[1] = {
    { NULL, 20, (void*)"SP_FORMAT_CALL_STACK" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFormatCallStackModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpFormatCallStack( 
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
    qsfCalculate_SpFormatCallStack,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* /* aCallBack */)
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     MTD_VARCHAR_PRECISION_MAXIMUM,
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

IDE_RC qsfCalculate_SpFormatCallStack( 
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
    qcStatement      * sStatement;
    mtdCharType      * sBuffer;
    SInt               i;
    UInt               sSize;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sBuffer = (mtdCharType *)aStack[0].value;

    sSize = 0;
    sBuffer->value[0] = '\0';

    idlOS::sprintf( (SChar *)&(sBuffer->value[sSize]),
                    "%-10s %-10s %s\n",
                    "object", "line", "object" ); 

    sSize += idlOS::strlen( (SChar *)&(sBuffer->value[sSize]) ); 

    idlOS::sprintf( (SChar *)&(sBuffer->value[sSize]),
                    "%-10s %-10s %s\n",
                    "handle", "number", "name" ); 

    sSize += idlOS::strlen( (SChar *)&(sBuffer->value[sSize]) ); 

    for ( i = QC_QSX_ENV( sStatement )->mStackCursor; i >= 0; --i )
    {
        if( QC_QSX_ENV( sStatement )->mStackBuffer[i].mOID != 0 )
        {
            idlOS::sprintf( (SChar *)&(sBuffer->value[sSize]),
                            "%-10ld %-10d %s \"%s\"\n",
                            QC_QSX_ENV( sStatement )->mStackBuffer[i].mOID,
                            QC_QSX_ENV( sStatement )->mStackBuffer[i].mLineno,
                            QC_QSX_ENV( sStatement )->mStackBuffer[i].mObjectType,
                            QC_QSX_ENV( sStatement )->mStackBuffer[i].mUserAndObjectName );
        }
        else
        {
            // Nothing to do.
        }

        sSize += idlOS::strlen( (SChar *)&(sBuffer->value[sSize]) ); 
    }

    sBuffer->length = idlOS::strlen( (SChar *)&(sBuffer->value[0]) );

    if( sBuffer->length > 0 )
    {
        sBuffer->length--;
    }

    IDE_DASSERT( sBuffer->length <= MTD_VARCHAR_PRECISION_MAXIMUM );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
