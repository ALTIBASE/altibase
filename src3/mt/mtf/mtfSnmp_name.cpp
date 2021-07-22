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
 * $Id: mtfSnmp_name.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <idm.h>

extern mtfModule mtfSnmp_name;

static mtcName mtfSnmp_nameFunctionName[1] = {
    { NULL, 9, (void*)"SNMP_NAME" }
};

static IDE_RC mtfSnmp_nameEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfSnmp_name = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSnmp_nameFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfSnmp_nameEstimate
};

IDE_RC mtfSnmp_nameCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSnmp_nameCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSnmp_nameEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt      /* aRemain */,
                             mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    //IDE_TEST( sModules[0]->estimate( aStack[0].column, 1, 256, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
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
}

IDE_RC mtfSnmp_nameCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
    SChar        sAttribute[512];
    mtdCharType* sVarchar;
    UInt         sLength;
    vULong       sBuffer[100];
    idmId*       sId = (idmId*)sBuffer;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sVarchar = (mtdCharType*)aStack[1].value;
        IDE_TEST_RAISE( sVarchar->length >= ID_SIZEOF(sAttribute),
                        ERR_INVALID_LENGTH );
        
        idlOS::memcpy( sAttribute, sVarchar->value, sVarchar->length );
        sAttribute[sVarchar->length] = '\0';
        
        IDE_TEST( idm::translate( sAttribute,
                                  sId,
                                  ID_SIZEOF(sBuffer) / ID_SIZEOF(sBuffer[0]) - 1 )
                  != IDE_SUCCESS );
        
        IDE_TEST( idm::name( sAttribute, ID_SIZEOF(sAttribute), sId )
                  != IDE_SUCCESS );
        
        sVarchar = (mtdCharType*)aStack[0].value;
        
        sLength = idlOS::strlen( sAttribute );
        
        IDE_TEST_RAISE( sLength > (UInt)aStack[0].column->precision,
                        ERR_INVALID_LENGTH );
        
        sVarchar->length = sLength;
        idlOS::memcpy( sVarchar->value, sAttribute, sLength );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
