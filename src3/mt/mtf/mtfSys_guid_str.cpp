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
 * $Id: mtfSys_guid_str.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * SYS_GUID_STR() :
 * 128bit GUID( Globally Unique IDentifier )를 생성하고,
 * VARCHAR타입의 32글자 Hex string 형태로 반환한다.
 * 
 * ex) 
 * SELECT SYS_GUID_STR() FROM DUAL;
 *
 * SYS_GUID_STR
 * -----------------------------------------------
 * A8C04F0335873CAD5502F6745043E51C
 * 1 row selected.
 *
 * GUID는 다음과 같이 구성된다. 
 *
 * ( 8 Bytes ) : HOST_ID 
 * (16 Bytes ) : INCREMENTAL VALUE
 * ( 8 Bytes ) : SERVER STARTUP TIME
 *
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <idCore.h>

extern mtdModule   mtdVarchar;

extern ULong          gInitialValue;
extern SChar          gHostID[MTF_SYS_HOSTID_LENGTH + 1];
extern UInt           gTimeSec;

static mtcName mtfSys_guidFunctionName[1] = {
    { NULL, 12, (void*)"SYS_GUID_STR" }
};

static IDE_RC mtfSys_guidEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfSys_guid_str = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSys_guidFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfSys_guidEstimate
};

static IDE_RC mtfSys_guidCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSys_guidCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSys_guidEstimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt,
                            mtcCallBack* /* aCallBack */ )
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     MTF_SYS_GUID_LENGTH,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSys_guidCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    mtdCharType*   sResult;
    SChar          sBuffer[MTF_SYS_GUID_LENGTH + 1];
    ULong          sValue;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sValue = idCore::acpAtomicInc64( &gInitialValue ); 

    idlOS::snprintf( sBuffer, MTF_SYS_GUID_LENGTH + 1, 
                     "%s"
                     "%016"ID_XINT64_FMT
                     "%08"ID_XINT32_FMT,
                     gHostID,
                     sValue,
                     gTimeSec);

    sResult = (mtdCharType*)aStack[0].value;
    idlOS::memcpy( sResult->value, sBuffer, MTF_SYS_GUID_LENGTH );
    sResult->length = MTF_SYS_GUID_LENGTH;
 
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
