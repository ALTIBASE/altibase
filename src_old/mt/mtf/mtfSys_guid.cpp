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
 * $Id: mtfSys_guid.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * SYS_GUID() :
 * GUID( Globally Unique IDentifier )를 생성하고,
 * 16 BYTE 형태로 반환한다.
 *
 * ex)
 * SELECT SYS_GUID() FROM DUAL;
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

extern mtdModule mtdByte;

extern ULong    gInitialValue;
extern SChar    gHostID[MTF_SYS_HOSTID_LENGTH + 1];
extern UInt     gTimeSec;

static mtcName mtfSysGuidFunctionName[1] = {
    { NULL, 8, (void *)"SYS_GUID" }
};

static IDE_RC mtfSysGuidEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule mtfSysGuid = {
    1 | MTC_NODE_OPERATOR_FUNCTION | MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfSysGuidFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfSysGuidEstimate
};

static IDE_RC mtfSysGuidCalculate( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSysGuidCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfSysGuidEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          /* aRemain */,
                                  mtcCallBack * /* aCallBack */)
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdByte,
                                     1,
                                     MTF_SYS_GUID_LENGTH / 2,
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

static IDE_RC mtfSysGuidCalculate( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate )
{
    SChar         sBuffer[MTF_SYS_GUID_LENGTH + 1];
    ULong         sValue;
    mtdByteType * sByte;
    SInt          i;
    SInt          j;
    UChar         sHigh;
    UChar         sLow;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sValue = idCore::acpAtomicInc64( &gInitialValue );

    sByte = (mtdByteType *)aStack[0].value;

    idlOS::snprintf( sBuffer, MTF_SYS_GUID_LENGTH + 1, 
                     "%s"
                     "%016"ID_XINT64_FMT
                     "%08"ID_XINT32_FMT,
                     gHostID,
                     sValue,
                     gTimeSec);

    for ( i = 0, j = 0; i < MTF_SYS_GUID_LENGTH; i += 2, j++ )
    {
        IDE_TEST( mtf::hex2Ascii( sBuffer[i], &sHigh )
                  != IDE_SUCCESS );
        IDE_TEST( mtf::hex2Ascii( sBuffer[i+1], &sLow )
                  != IDE_SUCCESS );
        sByte->value[j] = ( sHigh << 4 ) | sLow;
    }

    sByte->length = MTF_SYS_GUID_LENGTH / 2;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
