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
 * $Id: mtfLdump.cpp 36181 2009-10-21 00:56:27Z sungminee $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfLdump;

extern mtdModule mtdList;
extern mtdModule mtdInteger;

static mtcName mtfLdumpFunctionName[2] = {
    { mtfLdumpFunctionName+1, 7, (void*)"LE_DUMP" },
    { NULL, 5, (void*)"LDUMP" }
};

static IDE_RC mtfLdumpEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfLdump = {
    2|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfLdumpFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLdumpEstimate
};

static IDE_RC mtfLdumpCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLdumpCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLdumpEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];
    UInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( (( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1) ||
                    (( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( (aStack[1].column->module->id == MTD_LIST_ID ) ||
                    (aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
                    (aStack[1].column->module->id == MTD_RECORDTYPE_ID ) ||
                    (aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
                    (aStack[1].column->module->id == MTD_BLOB_ID ) ||
                    (aStack[1].column->module->id == MTD_CLOB_ID ) ||
                    (aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                    (aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                    (aStack[1].column->module->id == MTD_ECHAR_ID ) ||
                    (aStack[1].column->module->id == MTD_EVARCHAR_ID ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0], NULL )
              != IDE_SUCCESS );

    sModules[1] = &mtdInteger;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments->next,
                                        aTemplate,
                                        aStack + 2,
                                        aCallBack,
                                        sModules + 1 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    sPrecision = ( aStack[1].column->column.size << 2 ) + 50;

    if( sPrecision > MTD_CHAR_PRECISION_MAXIMUM )
    {
        sPrecision = MTD_CHAR_PRECISION_MAXIMUM;
    }

    /*
      IDE_TEST( sModule->estimate( aStack[0].column,
      1,
      sPrecision,
      0 )
      != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    mtc::initializeColumn( aStack[0].column + 1,
                           aStack[1].column );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static UChar* mtfCalculateCopyDecNumber( UChar* aIterator,
                                         UChar* aFence,
                                         UInt   aValue )
{
/***********************************************************************
 *
 * Description : Copy Decimal Number
 *
 * Implementation :
 *    aValue를 aIterator에 복사
 *
 ***********************************************************************/
    if( aValue >= 10 )
    {
        aIterator = mtfCalculateCopyDecNumber( aIterator, aFence, aValue/10 );
        aValue   %= 10;
    }

    if( aIterator < aFence )
    {
        *aIterator = aValue + '0';
        aIterator++;
    }

    return aIterator;
}

static UChar* mtfCalculateCopyHexNumber( UChar* aIterator,
                                         UChar* aFence,
                                         UInt   aValue )
{
/***********************************************************************
 *
 * Description : Copy Hex Number
 *
 * Implementation :
 *    aValue를 aIterator에 복사 
 *
 ***********************************************************************/
    if( aValue >= 16 )
    {
        aIterator = mtfCalculateCopyHexNumber( aIterator, aFence, aValue/16 );
        aValue   %= 16;
    }
    
    if( aIterator < aFence )
    {
        switch( aValue )
        {
            case 10:
                *aIterator = 'a';
                break;
            case 11:
                *aIterator = 'b';
                break;
            case 12:
                *aIterator = 'c';
                break;
            case 13:
                *aIterator = 'd';
                break;
            case 14:
                *aIterator = 'e';
                break;
            case 15:
                *aIterator = 'f';
                break;
            default:
                *aIterator = aValue + '0';
                break;
        }
        aIterator++;
    }
    
    return aIterator;
}

static UChar * mtfCalculateCopyString( UChar *     aIterator,
                                       UChar *     aFence,
                                       const char* aString )
{
/***********************************************************************
 *
 * Description : Copy String
 *
 * Implementation :
 *    aString을 aIterator에 복사 
 *
 ***********************************************************************/
    for( ; *aString != '\0' && aIterator < aFence; aString++, aIterator++ )
    {
        *aIterator = (const UChar)*aString;
    }

    return aIterator;
}

IDE_RC mtfLdumpCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Ldump Calculate
 *
 * Implementation :
 *    DUMP( expression )
 *
 *    aStack[0] : 입력된 자료를 분석하여 '자료형, 길이, 메모리 내용'의
 *                형식으로 저장한 값
 *    aStack[1] : 입력된 자료
 *    aStack[2] : 메모리 내용을 10진수 또는 16진수로 볼 수 있게 한다.
 *                (10 또는 16만 입력으로 받을 수 있다.)
 *                인자가 없거나 NULL이면 10진수로 보여준다.
 *
 *    ex ) DUMP(ename)
 *         ==> Type=CHAR(ENGLISH) Length=22: 0,20,83,87,78,...
 *
 ***********************************************************************/
    
    const mtcColumn* sColumn;
    const mtdModule* sModule;
    const mtlModule* sLanguage;
    mtdCharType*     sResult;
    UChar *          sCurResultVal;
    UChar*           sFence;
    UChar*           sValue;
    UChar*           sValueFence;
    UChar*           sTempValue;
    UInt             sActualSize;
    mtdIntegerType   sDecOrHex;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    // 결과값
    // sResult->value : 자료형 + 길이 + 메모리 내용
    // sResult->length : value에 저장된 결과값의 길이
    sResult        = (mtdCharType*)aStack[0].value;
    
    sCurResultVal  = sResult->value; 
    sFence         = sCurResultVal + aStack[0].column->precision;

    // data module : data type 정보 획득 시 필요
    sModule   = aStack[1].column->module;

    // language module : language type 정보 획득 시 필요 
    sLanguage = aStack[1].column->language;
    
    if( sModule->isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE )
    {
        // 입력된 자료가 NULL인 경우
        sCurResultVal = mtfCalculateCopyString( sCurResultVal, sFence, "NULL" );
    }
    else
    {
        // expression에 해당하는 값을 복사한다.
        sTempValue = (UChar*)aStack[1].value;
        sActualSize = sModule->actualSize( aStack[1].column,
                                          aStack[1].value );

        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sValue = ((UChar*)aTemplate->rows[aNode->table].row
                  + sColumn[1].column.offset);
        sValueFence = sValue + sActualSize;
        
        idlOS::memcpy( sValue, sTempValue, sActualSize );
        
#ifdef ENDIAN_IS_BIG_ENDIAN
        sModule->endian( (void*) sValue );
#endif
        
        //--------------------------------------
        // 메모리 내용을 보여줄 형태를 결정(10진수 또는 16진수)
        //--------------------------------------
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            sDecOrHex = 10;
        }
        else
        {
            if( aStack[2].column->module->isNull( aStack[2].column,
                                                  aStack[2].value ) == ID_TRUE)
            {
                sDecOrHex = 10;
            }
            else
            {
                IDE_TEST_RAISE( (*(mtdIntegerType*)aStack[2].value != 10) &&
                                (*(mtdIntegerType*)aStack[2].value != 16)
                                , ERR_ONLY_USE_DEC_OR_HEX );

                sDecOrHex = *(mtdIntegerType*)aStack[2].value;
            }
        }
        
        //--------------------------------------
        // 데이타 타입 정보 설정 ( ex. Type=CHAR )
        //--------------------------------------

        sCurResultVal =
            mtfCalculateCopyString( sCurResultVal, sFence, "Type=" );

        sCurResultVal =
            mtfCalculateCopyString(sCurResultVal,
                                   sFence,
                                   (const char*)sModule->names->string );
        
        //--------------------------------------
        // Language 정보 설정 ( ex. (ENGLISH) )
        //--------------------------------------
        
        if( sLanguage != NULL )
        {
            sCurResultVal =
                mtfCalculateCopyString( sCurResultVal, sFence, "(" );

            sCurResultVal =
                mtfCalculateCopyString(sCurResultVal,
                                       sFence, 
                                       (const char*)sLanguage->names->string );

            sCurResultVal =
                mtfCalculateCopyString( sCurResultVal, sFence, ")" );
        }
        else
        {
            // nothing to do
        }

        //--------------------------------------
        // 길이 정보 설정 ( ex. Length : 22 )
        //--------------------------------------

        sCurResultVal =
            mtfCalculateCopyString( sCurResultVal, sFence, " Length=" );

        sCurResultVal =
            mtfCalculateCopyDecNumber( sCurResultVal,
                                       sFence,
                                       sValueFence - sValue );

        //--------------------------------------
        // 메모리 내용
        //--------------------------------------

         sCurResultVal = mtfCalculateCopyString( sCurResultVal, sFence, ":" );
         
        if( sDecOrHex == 10 )
        {
            if( sValue < sValueFence )
            {
                sCurResultVal =
                    mtfCalculateCopyString( sCurResultVal, sFence, " " );
                
                sCurResultVal =
                    mtfCalculateCopyDecNumber( sCurResultVal,
                                               sFence,
                                               *sValue );
            }
            else
            {
                // nothing to do
            }

            for( sValue++; sValue < sValueFence; sValue++ )
            {
                sCurResultVal =
                    mtfCalculateCopyString( sCurResultVal, sFence, "," );
                
                sCurResultVal =
                    mtfCalculateCopyDecNumber( sCurResultVal,
                                               sFence,
                                               *sValue );
            }
        }
        else
        {
            if( sValue < sValueFence )
            {
                sCurResultVal =
                    mtfCalculateCopyString( sCurResultVal, sFence, " " );
                
                sCurResultVal =
                    mtfCalculateCopyHexNumber( sCurResultVal,
                                               sFence,
                                               *sValue );
            }
            else
            {
                // nothing to do
            }

            for( sValue++; sValue < sValueFence; sValue++ )
            {
                sCurResultVal =
                    mtfCalculateCopyString( sCurResultVal, sFence, "," );
                
                sCurResultVal =
                    mtfCalculateCopyHexNumber( sCurResultVal,
                                               sFence,
                                               *sValue );
            }
        }
    }
    
    sResult->length = sCurResultVal - sResult->value;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ONLY_USE_DEC_OR_HEX );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ONLY_USE_DEC_OR_HEX_IN_DUMP_FUNC));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
