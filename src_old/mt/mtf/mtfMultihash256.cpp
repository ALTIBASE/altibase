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
 * $Id: mtfMultihash256.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idsSHA256.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfMultihash256;

extern mtdModule mtdByte;
extern mtdModule mtdBinary;
extern mtdModule mtdSmallint;

static mtcName mtfMultihash256FunctionName[1] = {
    { NULL, 12, (void*)"MULTIHASH256" }
};

static IDE_RC mtfMultihash256Estimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule mtfMultihash256 = {
    2|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfMultihash256FunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMultihash256Estimate
};

static IDE_RC mtfMultihash256Calculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultihash256Calculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfMultihash256CalculateFast( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate );

const mtcExecute mtfExecuteFast = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultihash256CalculateFast,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultihash256Estimate( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt          /* aRemain */,
                                mtcCallBack * /* aCallBack */ )
{
    mtcNode  * sNode;
    mtcStack * sStack;
    SInt       sModuleId;
    SInt       sPrecision = 0;
    UInt       sCount = 0;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for ( sNode = aNode->arguments, sStack = aStack + 1;
          sNode != NULL;
          sNode = sNode->next, sStack++ )
    {
        /* BUG-22611
         * switch-case에 UInt 형으로 음수값이 두번 이상 오면 서버 비정상 종료
         * ex )  case MTD_BIT_ID: ==> (UInt)-7
         *       case MTD_VARBIT_ID: ==> (UInt)-8
         * 따라서 SInt 형으로 타입 캐스팅 하도록 수정함
         */
        sModuleId = (SInt) sStack->column->module->id;

        switch ( sModuleId )
        {
            case MTD_BLOB_ID:
            case MTD_BLOB_LOCATOR_ID:
            case MTD_CLOB_ID:
            case MTD_CLOB_LOCATOR_ID:
            case MTD_LIST_ID:
            case MTD_UNDEF_ID:
            case MTD_GEOMETRY_ID:
                IDE_RAISE( ERR_ARGUMENT_NOT_APPICABLE );
                break;
            case MTD_NULL_ID:
                sPrecision += 2;
                break;
            default:
                sPrecision += sStack->column->column.size;
                break;
        }

        if ( idlOS::strlen( (SChar*)sNode->module->names->string ) == 6 )
        {
            if ( idlOS::strncmp( (SChar*)sNode->module->names->string,
                                 (const SChar*)"COLUMN", 6 )
                 != 0 )
            {
                sCount += 1;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( sCount != 0 )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteFast;
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdByte,
                                     1,
                                     32,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBinary,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPICABLE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMultihash256Calculate( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Multihash256 Calculate
 *
 * Implementation :
 *
 *  MULTIHASH256( I1, I2, I3, ... )
 *
 *  여러 컬럼을 연속적으로 해싱하는 멀티 해싱을 수행한다.
 *
 ***********************************************************************/

    mtcNode         * sNode;
    mtdByteType     * sMultihash;
    mtdBinaryType   * sBinary;
    mtdSmallintType   sOrder;
    mtcColumn       * sBinaryColumn;
    mtcColumn       * sColumn;
    void            * sValue;
    UChar           * sBuffer;
    UInt              sSize;
    UInt              sOffset = 0;
    UInt              sIdx    = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sMultihash = (mtdByteType*)aStack[0].value;

    sBinaryColumn = aTemplate->rows[aNode->table].columns + aNode->column + 1;
    sBinary = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row
         + sBinaryColumn[1].column.offset);

    sBuffer = (UChar*) sBinary->mValue;

    for ( sNode = aNode->arguments, sOrder = 0, sIdx =  1;
          sNode != NULL;
          sNode = sNode->next, sOrder++, sIdx++ )
    {
        sColumn = (mtcColumn*) aStack[sIdx].column;
        sValue = (void*) aStack[sIdx].value;

        if ( sColumn->module->isNull( sColumn, sValue ) == ID_TRUE )
        {
            // n번째 null과 n+1번째 null을 구분한다.
            sValue = (void*) &sOrder;
            sSize = 2;
        }
        else
        {
            sSize = sColumn->module->actualSize( sColumn, sValue );
            IDE_DASSERT( sSize > 0 );
        }

        idlOS::memcpy( sBuffer + sOffset, sValue, sSize );

/* BUG-39344 */
#ifdef ENDIAN_IS_BIG_ENDIAN
        if ( sValue == &sOrder )
        {
            mtdSmallint.endian( sBuffer + sOffset );
        }
        else
        {
            sColumn->module->endian( sBuffer + sOffset );
        }
#endif

        sOffset += sSize;

        IDE_DASSERT( sOffset <= (UInt)sBinaryColumn->precision );
    }

    idsSHA256::digestToByte( (UChar *)sMultihash->value,
                             sBuffer,
                             sOffset );

    sMultihash->length = 32;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMultihash256CalculateFast( mtcNode     * aNode,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     void        * /*aInfo*/,
                                     mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Multihash256 Calculate
 *
 * Implementation :
 *
 *  MULTIHASH256( I1, I2, I3, ... )
 *
 *  여러 컬럼을 연속적으로 해싱하는 멀티 해싱을 수행한다.
 *
 ***********************************************************************/

    mtcNode         * sNode;
    mtcColumn       * sBinaryColumn;
    mtcColumn       * sColumn;
    mtdByteType     * sMultihash;
    mtdBinaryType   * sBinary;
    mtdSmallintType   sOrder;
    void            * sValue;
    UInt              sSize;
    UChar           * sBuffer;
    UInt              sOffset = 0;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value = (void*) mtc::value( aStack->column,
                                        aTemplate->rows[aNode->table].row,
                                        MTD_OFFSET_USE );

    sMultihash = (mtdByteType*)aStack->value;

    sBinaryColumn = aTemplate->rows[aNode->table].columns + aNode->column + 1;
    sBinary = (mtdBinaryType*) mtc::value( sBinaryColumn,
                                           aTemplate->rows[aNode->table].row,
                                           MTD_OFFSET_USE );
    sBuffer = (UChar*) sBinary->mValue;

    for ( sNode = aNode->arguments, sOrder = 0;
          sNode != NULL;
          sNode = sNode->next, sOrder++ )
    {
        sColumn = aTemplate->rows[sNode->table].columns + sNode->column;
        sValue = (void*) mtc::value( sColumn,
                                     aTemplate->rows[sNode->table].row,
                                     MTD_OFFSET_USE );

        if ( sColumn->module->isNull( sColumn, sValue ) == ID_TRUE )
        {
            // n번째 null과 n+1번째 null을 구분한다.
            sValue = (void*) &sOrder;
            sSize = 2;
        }
        else
        {
            sSize = sColumn->module->actualSize( sColumn, sValue );
            IDE_DASSERT( sSize > 0 );
        }

        idlOS::memcpy( sBuffer + sOffset, sValue, sSize );

/* BUG-39344 */
#ifdef ENDIAN_IS_BIG_ENDIAN
       if ( sValue == &sOrder )
       {
           mtdSmallint.endian( sBuffer + sOffset );
       }
       else
       {
           sColumn->module->endian( sBuffer + sOffset );
       }
#endif

        sOffset += sSize;

        IDE_DASSERT( sOffset <= (UInt)sBinaryColumn->precision );
    }

    idsSHA256::digestToByte( (UChar *)sMultihash->value,
                             sBuffer,
                             sOffset );

    sMultihash->length = 32;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
