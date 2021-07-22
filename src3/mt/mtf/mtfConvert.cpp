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
 * $Id: mtfConvert.cpp 24813 2008-01-11 01:45:03Z orc $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfConvert;

extern mtdModule mtdInteger;
extern mtdModule mtdBigint;
extern mtdModule mtdVarchar;

static mtcName mtfConvertFunctionName[1] = {
    { NULL, 7, (void*)"CONVERT"    }
};

static IDE_RC mtfConvertEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfConvert = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfConvertFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfConvertEstimate
};

static IDE_RC mtfConvertCalculateFor2Args( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );


static IDE_RC mtfConvertCalculateFor3Args( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

static IDE_RC mtfConvertCalculateNcharFor2Args( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );


static IDE_RC mtfConvertCalculateNcharFor3Args( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

IDE_RC mtfConvertCharSet4Char( mtcStack        * aStack,
                               mtdCharType     * aSource,
                               mtdCharType     * aResult,
                               const mtlModule * aSrcCharSet,
                               const mtlModule * aDestCharSet );

IDE_RC mtfConvertCharSet4Nchar( mtcStack        * aStack,
                                mtdNcharType    * aSource,
                                mtdCharType     * aResult,
                                const mtlModule * aSrcCharSet,
                                const mtlModule * aDestCharSet );

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfConvertCalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfConvertCalculateFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfConvertCalculateNcharFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfConvertCalculateNcharFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfConvertEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];
    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    sModules[1] = &mtdVarchar;
    sModules[2] = &mtdVarchar;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments->next,
                                        aTemplate,
                                        aStack + 2,
                                        aCallBack,
                                        sModules + 1 )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
        (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNcharFor2Args;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNcharFor3Args;
        }

        // NCHAR의 precision은 문자의 개수이므로
        // 결과 타입인 char의 precision은 max precision만큼 늘어날 수 있다.
        sPrecision = aStack[1].column->precision * MTL_MAX_PRECISION;

        sPrecision = IDL_MIN( aStack[1].column->precision * MTL_MAX_PRECISION,
                              (SInt)MTD_UTF8_NCHAR_PRECISION_MAXIMUM );
    }
    else
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor2Args;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor3Args;
        }

        // 캐릭터 셋 변환 후, 최대 2배까지 늘어날 수 있다.
        // ex) ASCII => UTF16
        sPrecision = aStack[1].column->precision * 2;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     sPrecision,
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

IDE_RC mtfConvertCalculateFor2Args( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Convert Calculate for CHAR
 *
 * Implementation :
 *    CONVERT( string, , dest_char_set )
 *
 *    aStack[0] : 변환된 문자열
 *    aStack[1] : 변환할 문자열
 *    aStack[2] : 목표 캐릭터 셋
 *
 *    ex ) CONVERT( 'SALESMAN', 'UTF8' ) ==> 
 *          입력 문자열을 현재 데이터베이스 캐릭터 셋에서 
 *          UTF8 캐릭터 셋으로 변환한다. 
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdCharType     * sDestCharSetStr;
    const mtlModule * sSrcCharSet;
    const mtlModule * sDestCharSet;

    UChar             sDestCharValue[MTL_NAME_LEN_MAX];
    UInt              sDestCharValueLen;
    
    sDestCharValue[0] = '\0';
    sDestCharValueLen = 0;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;

        sDestCharSetStr = (mtdCharType*)aStack[2].value;

        if( sDestCharSetStr->length <= MTL_NAME_LEN_MAX )
        {
            idlOS::memcpy(sDestCharValue,
                          sDestCharSetStr->value,
                          sDestCharSetStr->length);
            
            sDestCharValueLen = sDestCharSetStr->length;
            
            idlOS::strUpper(sDestCharValue, sDestCharValueLen);
        }
        else
        {
            // Nothing to do.
        }

        // Src CharSet
        sSrcCharSet = aStack[1].column->language;

        // Dest CharSet
        IDE_TEST( mtl::moduleByName( (const mtlModule **) & sDestCharSet,
                                     sDestCharValue,
                                     sDestCharValueLen )
                  != IDE_SUCCESS );

        IDE_TEST( mtfConvertCharSet4Char( aStack,
                                          sSource,
                                          sResult,
                                          sSrcCharSet,
                                          sDestCharSet )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConvertCalculateFor3Args( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Convert Calculate for CHAR
 *
 * Implementation :
 *    CONVERT( string, , dest_char_set, src_char_set )
 *
 *    aStack[0] : 변환된 문자열
 *    aStack[1] : 변환할 문자열
 *    aStack[2] : 목표 캐릭터 셋
 *    aStack[3] : 소스 캐릭터 셋
 *
 *    ex ) CONVERT( 'SALESMAN', 'UTF8', 'KO16KSC5601' ) ==> 
 *          입력 문자열을 KO16KSC5601 => UTF8로 변환한다.
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdCharType     * sSrcCharSetStr;
    mtdCharType     * sDestCharSetStr;
    const mtlModule * sSrcCharSet;
    const mtlModule * sDestCharSet;

    UChar             sSrcCharValue[MTL_NAME_LEN_MAX];
    UChar             sDestCharValue[MTL_NAME_LEN_MAX];
    UInt              sSrcCharValueLen;
    UInt              sDestCharValueLen;
    
    sSrcCharValue[0] = '\0';
    sDestCharValue[0] = '\0';

    sSrcCharValueLen = 0;
    sDestCharValueLen = 0;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;

        sSrcCharSetStr  = (mtdCharType*)aStack[3].value;
        sDestCharSetStr = (mtdCharType*)aStack[2].value;

        if( sSrcCharSetStr->length <= MTL_NAME_LEN_MAX )
        {    
            idlOS::memcpy(sSrcCharValue,
                          sSrcCharSetStr->value,
                          sSrcCharSetStr->length);
            
            sSrcCharValueLen = sSrcCharSetStr->length;

            idlOS::strUpper(sSrcCharValue, sSrcCharValueLen);
        }
        else
        {
            // Nothing to do.
        }
        
        if( sDestCharSetStr->length <= MTL_NAME_LEN_MAX )
        {
            idlOS::memcpy(sDestCharValue,
                          sDestCharSetStr->value,
                          sDestCharSetStr->length);
            
            sDestCharValueLen = sDestCharSetStr->length;
            
            idlOS::strUpper(sDestCharValue, sDestCharValueLen);
        }
        else
        {
            // Nothing to do.
        }

        // Src CharSet
        IDE_TEST( mtl::moduleByName( (const mtlModule **) & sSrcCharSet,
                                     sSrcCharValue,
                                     sSrcCharValueLen )
                  != IDE_SUCCESS );

        // Dest CharSet
        IDE_TEST( mtl::moduleByName( (const mtlModule **) & sDestCharSet,
                                     sDestCharValue,
                                     sDestCharValueLen )
                  != IDE_SUCCESS );

        IDE_TEST( mtfConvertCharSet4Char( aStack,
                                          sSource,
                                          sResult,
                                          sSrcCharSet,
                                          sDestCharSet )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConvertCalculateNcharFor2Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Convert Calculate for NCHAR
 *
 * Implementation :
 *    CONVERT( string, , dest_char_set )
 *
 *    aStack[0] : 변환된 문자열
 *    aStack[1] : 변환할 문자열
 *    aStack[2] : 목표 캐릭터 셋
 *
 *    ex ) CONVERT( 'SALESMAN', 'UTF8' ) ==> 
 *          입력 문자열을 현재 데이터베이스 캐릭터 셋에서 
 *          UTF8 캐릭터 셋으로 변환한다. 
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdNcharType    * sSource;
    mtdCharType     * sDestCharSetStr;
    const mtlModule * sSrcCharSet;
    const mtlModule * sDestCharSet;

    UChar             sDestCharValue[MTL_NAME_LEN_MAX];
    UInt              sDestCharValueLen;
    
    sDestCharValue[0] = '\0';
    sDestCharValueLen = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sSource = (mtdNcharType*)aStack[1].value;
        sResult = (mtdCharType*)aStack[0].value;

        sDestCharSetStr = (mtdCharType*)aStack[2].value;

        if( sDestCharSetStr->length <= MTL_NAME_LEN_MAX )
        {
            idlOS::memcpy(sDestCharValue,
                          sDestCharSetStr->value,
                          sDestCharSetStr->length);
            
            sDestCharValueLen = sDestCharSetStr->length;
            
            idlOS::strUpper(sDestCharValue, sDestCharValueLen);
        }
        else
        {
            // Nothing to do.
        }

        // Src CharSet
        sSrcCharSet = aStack[1].column->language;

        // Dest CharSet
        IDE_TEST( mtl::moduleByName( (const mtlModule **) & sDestCharSet,
                                     sDestCharValue,
                                     sDestCharValueLen )
                  != IDE_SUCCESS );

        IDE_TEST( mtfConvertCharSet4Nchar( aStack,
                                           sSource,
                                           sResult,
                                           sSrcCharSet,
                                           sDestCharSet )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConvertCalculateNcharFor3Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Convert Calculate for NCHAR
 *
 * Implementation :
 *    CONVERT( string, , dest_char_set, src_char_set )
 *
 *    aStack[0] : 변환된 문자열
 *    aStack[1] : 변환할 문자열
 *    aStack[2] : 목표 캐릭터 셋
 *    aStack[3] : 소스 캐릭터 셋
 *
 *    ex ) CONVERT( 'SALESMAN', 'UTF8', 'KO16KSC5601' ) ==> 
 *          입력 문자열을 KO16KSC5601 => UTF8로 변환한다.
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdNcharType    * sSource;
    mtdCharType     * sSrcCharSetStr;
    mtdCharType     * sDestCharSetStr;
    const mtlModule * sSrcCharSet;
    const mtlModule * sDestCharSet;

    UChar             sSrcCharValue[MTL_NAME_LEN_MAX];
    UChar             sDestCharValue[MTL_NAME_LEN_MAX];
    UInt              sSrcCharValueLen;
    UInt              sDestCharValueLen;
    
    sSrcCharValue[0] = '\0';
    sDestCharValue[0] = '\0';

    sSrcCharValueLen = 0;
    sDestCharValueLen = 0;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sSource = (mtdNcharType*)aStack[1].value;
        sResult = (mtdCharType*)aStack[0].value;

        sSrcCharSetStr  = (mtdCharType*)aStack[3].value;
        sDestCharSetStr = (mtdCharType*)aStack[2].value;

        if( sSrcCharSetStr->length <= MTL_NAME_LEN_MAX )
        {    
            idlOS::memcpy(sSrcCharValue,
                          sSrcCharSetStr->value,
                          sSrcCharSetStr->length);
            
            sSrcCharValueLen = sSrcCharSetStr->length;

            idlOS::strUpper(sSrcCharValue, sSrcCharValueLen);
        }
        else
        {
            // Nothing to do.
        }
        
        if( sDestCharSetStr->length <= MTL_NAME_LEN_MAX )
        {
            idlOS::memcpy(sDestCharValue,
                          sDestCharSetStr->value,
                          sDestCharSetStr->length);
            
            sDestCharValueLen = sDestCharSetStr->length;
            
            idlOS::strUpper(sDestCharValue, sDestCharValueLen);
        }
        else
        {
            // Nothing to do.
        }
        // Src CharSet
        IDE_TEST( mtl::moduleByName( (const mtlModule **) & sSrcCharSet,
                                     sSrcCharValue,
                                     sSrcCharValueLen )
                  != IDE_SUCCESS );

        // Dest CharSet
        IDE_TEST( mtl::moduleByName( (const mtlModule **) & sDestCharSet,
                                     sDestCharValue,
                                     sDestCharValueLen )
                  != IDE_SUCCESS );

        IDE_TEST( mtfConvertCharSet4Nchar( aStack,
                                           sSource,
                                           sResult,
                                           sSrcCharSet,
                                           sDestCharSet )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConvertCharSet4Char( mtcStack        * aStack,
                               mtdCharType     * aSource,
                               mtdCharType     * aResult,
                               const mtlModule * aSrcCharSet,
                               const mtlModule * aDestCharSet )
{
    idnCharSetList    sIdnSrcCharSet;
    idnCharSetList    sIdnDestCharSet;
    UChar           * sSourceIndex;
    UChar           * sTempIndex;
    UChar           * sSourceFence;
    UChar           * sResultValue;
    UChar           * sResultFence;
    SInt              sSrcRemain = 0;
    SInt              sDestRemain = 0;
    SInt              sTempRemain = 0;

    // 변환 결과의 크기를 체크하기 위함
    sDestRemain = aStack[0].column->precision;

    sSourceIndex = aSource->value;
    sSrcRemain   = aSource->length;
    sSourceFence = sSourceIndex + aSource->length;

    sResultValue = aResult->value;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      -1 /* mNlsNcharConvExcp */ )
                      != IDE_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        IDE_TEST_RAISE ( sDestRemain < aSource->length, ERR_INVALID_DATA_LENGTH );
        idlOS::memcpy( aResult->value,
                       aSource->value,
                       aSource->length );

        aResult->length = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConvertCharSet4Nchar( mtcStack        * aStack,
                                mtdNcharType    * aSource,
                                mtdCharType     * aResult,
                                const mtlModule * aSrcCharSet,
                                const mtlModule * aDestCharSet )
{
    idnCharSetList    sIdnSrcCharSet;
    idnCharSetList    sIdnDestCharSet;
    UChar           * sSourceIndex;
    UChar           * sTempIndex;
    UChar           * sSourceFence;
    UChar           * sResultValue;
    UChar           * sResultFence;
    SInt              sSrcRemain = 0;
    SInt              sDestRemain = 0;
    SInt              sTempRemain = 0;
    UShort            sSourceLen;

    sSourceIndex = aSource->value;
    sSourceLen   = aSource->length;
    sSrcRemain   = sSourceLen;
    sSourceFence = sSourceIndex + sSrcRemain;

    sResultValue = aResult->value;
    sDestRemain  = aStack[0].column->precision;
    sResultFence = sResultValue + sDestRemain;

    if( aSrcCharSet->id != aDestCharSet->id )
    {
        sIdnSrcCharSet = mtl::getIdnCharSet( aSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( aDestCharSet );

        //-----------------------------------------
        // 캐릭터 셋 변환
        //-----------------------------------------        
        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;
            
            IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                      sIdnDestCharSet,
                                      sSourceIndex,
                                      sSrcRemain,
                                      sResultValue,
                                      & sDestRemain,
                                      -1 /* mNlsNcharConvExcp */ )
                      != IDE_SUCCESS );

            sTempIndex = sSourceIndex;

            (void)aSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );
            
            sResultValue += (sTempRemain - sDestRemain);

            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }

        aResult->length = sResultValue - aResult->value;
    }
    else
    {
        IDE_TEST_RAISE ( sDestRemain < aSource->length, ERR_INVALID_DATA_LENGTH );
        idlOS::memcpy( aResult->value,
                       aSource->value,
                       aSource->length );

        aResult->length = aSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
