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
 * $Id: mtfGroupConcat.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfGroupConcat;

extern mtdModule mtdBoolean;
extern mtdModule mtdVarchar;
extern mtlModule mtlAscii;

static mtcName mtfGroupConcatFunctionName[1] = {
    { NULL, 12, (void*)"GROUP_CONCAT" }
};

static IDE_RC mtfGroupConcatEstimate( mtcNode     * aNode,
                                      mtcTemplate * aTemplate,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      mtcCallBack * aCallBack );

mtfModule mtfGroupConcat = {
    2 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfGroupConcatFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfGroupConcatEstimate
};

IDE_RC mtfGroupConcatInitialize( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfGroupConcatAggregate( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

IDE_RC mtfGroupConcatFinalize( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate );

IDE_RC mtfGroupConcatCalculate( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfGroupConcatInitialize,
    mtfGroupConcatAggregate,
    mtf::calculateNA,
    mtfGroupConcatFinalize,
    mtfGroupConcatCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfGroupConcatEstimate( mtcNode     * aNode,
                               mtcTemplate * aTemplate,
                               mtcStack    * aStack,
                               SInt          /* aRemain */,
                               mtcCallBack * aCallBack )
{
    const mtdModule * sModules[2];
    UInt              sArguments;

    sArguments = (UInt)( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    switch ( sArguments )
    {
        case 2:
            IDE_TEST_RAISE ( ( aStack[2].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                             ( aStack[2].column->module->id == MTD_CLOB_ID ) ||
                             ( aStack[2].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                             ( aStack[2].column->module->id == MTD_BLOB_ID ),
                             ERR_ARGUMENT_NOT_APPICABLE );
        case 1:
            IDE_TEST_RAISE ( ( aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID ) ||
                             ( aStack[1].column->module->id == MTD_CLOB_ID ) ||
                             ( aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID ) ||
                             ( aStack[1].column->module->id == MTD_BLOB_ID ),
                             ERR_ARGUMENT_NOT_APPICABLE );
            break;
        default:
            IDE_RAISE( ERR_INVALID_FUNCTION_ARGUMENT );
            break;
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[0],
                                                aStack[1].column->module )
              != IDE_SUCCESS );

    sModules[1] = sModules[0];

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     MTU_GROUP_CONCAT_PRECISION,
                                     0 )
              != IDE_SUCCESS );

    // BUG-37247
    aTemplate->groupConcatPrecisionRef = ID_TRUE;

    // is first
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfGroupConcatInitialize( mtcNode     * aNode,
                                 mtcStack    * ,
                                 SInt          ,
                                 void        * ,
                                 mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    void            * sValueTemp;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sValueTemp = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

    sColumn->module->null( sColumn, sValueTemp );

    *(mtdBooleanType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset) =
        (mtdBooleanType) MTD_BOOLEAN_TRUE;

    return IDE_SUCCESS;
}

IDE_RC mtfGroupConcatAggregate( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * ,
                                mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    mtcNode         * sNode;
    mtdCharType     * sResult;
    mtdCharType     * sFirstString;
    mtdCharType     * sSecondString;
    mtdBooleanType  * sIsFirst;
    UInt              sCopySize;

    // BUG-33674
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    
    sNode = aNode->arguments;
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value )
         != ID_TRUE )
    {
        sResult      = (mtdCharType*) aStack[0].value;
        sFirstString = (mtdCharType*) aStack[1].value;

        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        
        sIsFirst = (mtdBooleanType*)
            ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

        if ( *sIsFirst == MTD_BOOLEAN_NULL )
        {
            // BUG-38046
            // max precision을 넘은 경우 더이상 수행하지 않는다.
            
            // Nothing to do.
        }
        else
        {
            if ( *sIsFirst == MTD_BOOLEAN_TRUE )
            {
                *sIsFirst = MTD_BOOLEAN_FALSE;    
            }
            else
            {
                //-----------------------------------------
                // 두번째 인자 계산 (seperator)
                //-----------------------------------------
                sNode = sNode->next;
                if ( sNode != NULL )
                {
                    if ( aStack[2].column->module->isNull( aStack[2].column,
                                                           aStack[2].value )
                         != ID_TRUE )
                    {
                        sSecondString = (mtdCharType*) aStack[2].value;

                        IDE_DASSERT( sResult->length <= aStack[0].column->precision );

                        if ( aStack[2].column->language != &mtlAscii )
                        {
                            IDE_TEST_RAISE( aStack[0].column->language->id !=
                                            aStack[2].column->language->id, ERR_COLUMN_LANGUAGE );
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        IDE_TEST( mtf::copyString(
                                          sResult->value + sResult->length,
                                          aStack[0].column->precision - sResult->length,
                                          & sCopySize,
                                          sSecondString->value,
                                          sSecondString->length,
                                          aStack[2].column->language )
                                      != IDE_SUCCESS );

                        sResult->length += sCopySize;

                        if ( sCopySize < sSecondString->length )
                        {
                            *sIsFirst = MTD_BOOLEAN_NULL;

                            IDE_CONT( NORMAL_EXIT );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }

            // add aggregate column
            IDE_DASSERT( sResult->length <= aStack[0].column->precision );
            if ( aStack[1].column->language != &mtlAscii )
            {
                IDE_TEST_RAISE( aStack[0].column->language->id !=
                                aStack[1].column->language->id, ERR_COLUMN_LANGUAGE );
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( mtf::copyString(
                          sResult->value + sResult->length,
                          aStack[0].column->precision - sResult->length,
                          & sCopySize,
                          sFirstString->value,
                          sFirstString->length,
                          aStack[1].column->language )
                      != IDE_SUCCESS );

            sResult->length += sCopySize;
            
            if ( sCopySize < sFirstString->length )
            {
                *sIsFirst = MTD_BOOLEAN_NULL;
                                
                IDE_CONT( NORMAL_EXIT );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_COLUMN_LANGUAGE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtfGroupConcatAggregate",
                                  "different column language" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfGroupConcatFinalize( mtcNode     * ,
                               mtcStack    * ,
                               SInt          ,
                               void        * ,
                               mtcTemplate *  )
{
    return IDE_SUCCESS;
}

IDE_RC mtfGroupConcatCalculate( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          ,
                                void        * ,
                                mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
}
