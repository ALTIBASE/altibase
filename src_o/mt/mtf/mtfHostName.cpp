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
 * $Id: mtfHostName.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#define MTF_HOST_NAME_MAX   (64)

extern mtfModule mtfHostName;
extern mtdModule mtdVarchar;

static mtcName mtfHostNameFunctionName[1] = {
    { NULL, 9, ( void * )"HOST_NAME" }
};

static IDE_RC mtfHostNameEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallBack );

static IDE_RC mtfHostNameCalculate( mtcNode     * aNode,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    void        * aInfo,
                                    mtcTemplate * aTemplate );

mtfModule mtfHostName = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfHostNameFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfHostNameEstimate
};

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfHostNameCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfHostNameEstimate( mtcNode     * aNode,
                            mtcTemplate * aTemplate,
                            mtcStack    * aStack,
                            SInt,         /* aRemain */
                            mtcCallBack * /* aCallBack */ )
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     MTF_HOST_NAME_MAX,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfHostNameCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate )
{
    mtdCharType *   sResult;
    SChar           sHostName[ MTF_HOST_NAME_MAX + 1 ] = { 0, };
    UInt            sHostNameLen;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sResult = ( mtdCharType * )aStack[0].value;

    IDE_TEST_RAISE( acpSysGetHostName( sHostName, MTF_HOST_NAME_MAX )
                    != ACP_RC_SUCCESS,
                    ERR_UNEXPECTED_ERROR );

    sHostNameLen = idlOS::strnlen( sHostName, MTF_HOST_NAME_MAX );

    idlOS::memcpy( sResult->value, sHostName, sHostNameLen );

    sResult->length = sHostNameLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                              "mtfHostNameCalculate",
                              "Failed to get host name" ) );
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
