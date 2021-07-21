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
 * $Id: mtfXSNToLSN.cpp 53473 2012-05-30 03:11:55Z newdaily $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

 
extern mtfModule mtfXSNToLSN;

extern mtdModule mtdBigint;

static mtcName mtfXSNToLSNFunctionName[1] = {
    { NULL, 10, (void*)"XSN_TO_LSN" }
};

static IDE_RC mtfXSNToLSNEstimate( mtcNode     * aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallBack );

mtfModule mtfXSNToLSN = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfXSNToLSNFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfXSNToLSNEstimate
};

IDE_RC mtfXSNToLSNCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfXSNToLSNCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfXSNToLSNEstimate( mtcNode     * aNode,
                            mtcTemplate* aTemplate,
                            mtcStack    * aStack,
                            SInt,
                            mtcCallBack * aCallBack )
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    const mtdModule* sModules[1];
    const mtdModule* sModule;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdBigint;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    IDE_TEST( mtf::getCharFuncResultModule( &sModule, NULL ) != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     1,
                                     MTC_XSN_TO_LSN_MAX_PRECISION,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*FILENO:10, OFFSET:10*/
IDE_RC mtfXSNToLSNCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate )
{
    mtdBigintType sBigint;
    mtdCharType*  sResult ;
    smLSN         sLSN;
    SChar         sString[MTC_XSN_TO_LSN_MAX_PRECISION];
    UInt          sSize = 0;
    UInt*         sSNPtr = 0;
 
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if (aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE)
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sBigint = *(mtdBigintType*)aStack[1].value;
        sResult = (mtdCharType*)aStack[0].value;

        sSNPtr = (UInt*)(&sBigint);

#if defined(ENDIAN_IS_BIG_ENDIAN)
        sLSN.mFileNo = sSNPtr[0];
        sLSN.mOffset = sSNPtr[1];
#else
        sLSN.mFileNo = sSNPtr[1];
        sLSN.mOffset = sSNPtr[0];
#endif
        sSize = idlOS::snprintf( sString, 
                                 MTC_XSN_TO_LSN_MAX_PRECISION, 
                                 "FILENO:%"ID_UINT32_FMT", OFFSET:%"ID_UINT32_FMT, 
                                 sLSN.mFileNo, sLSN.mOffset );

        sResult->length = sSize;
        idlOS::memcpy( sResult->value,
                       sString,
                       sSize );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
