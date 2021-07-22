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
 * $Id: mtfNvl2.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNvl2;

extern mtdModule mtdList;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;

static mtcName mtfNvl2FunctionName[1] = {
    { NULL, 4, (void*)"NVL2" }
};

static IDE_RC mtfNvl2Estimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfNvl2 = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfNvl2FunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNvl2Estimate
};

IDE_RC mtfNvl2Calculate( mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt         aRemain,
                         void*        aInfo,
                         mtcTemplate* aTemplate );

IDE_RC mtfNvl2CalculateXlobColumn( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfNvl2CalculateXlobLocator( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNvl2Calculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteXlobColumn = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNvl2CalculateXlobColumn,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteXlobLocator = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNvl2CalculateXlobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNvl2Estimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt      /* aRemain */,
                        mtcCallBack* aCallBack )
{
    const mtdModule* sTarget;
    const mtdModule* sModules[3];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::getComparisonModule(
                  &sTarget,
                  aStack[2].column->module->no,
                  aStack[3].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sTarget == NULL || sTarget == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );

    // PROJ-2002 Column Security
    // 보안 컬럼인 경우 원본 컬럼으로 바꾼다.
    if( sTarget == &mtdEchar )
    {
        sModules[0] = aStack[1].column->module;
        sModules[1] = &mtdChar;
        sModules[2] = &mtdChar;
    }
    else if( sTarget == &mtdEvarchar )
    {
        sModules[0] = aStack[1].column->module;
        sModules[1] = &mtdVarchar;
        sModules[2] = &mtdVarchar;
    }
    else
    {
        sModules[0] = aStack[1].column->module;
        sModules[1] = sTarget;
        sModules[2] = sTarget;
    }

    IDE_TEST_RAISE( sModules[2] == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    if( ( aStack[2].column->module->flag & MTD_NON_LENGTH_MASK ) == MTD_NON_LENGTH_TYPE )
    {
        IDE_TEST_RAISE( !mtc::isSameType( aStack[2].column, aStack[3].column ),
                        ERR_INVALID_PRECISION );
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // BUG-16394
    if ( (aStack[1].column->module->id == MTD_BLOB_ID) ||
         (aStack[1].column->module->id == MTD_CLOB_ID) )
    {
        if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobColumn;
        }
        else
        {
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
        }
    }
    else if ( (aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID) ||
              (aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID) )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocator;
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }

    // BUG-23102
    // mtcColumn으로 초기화한다.
    if( aStack[2].column->column.size > aStack[3].column->column.size )
    {
        mtc::initializeColumn( aStack[0].column, aStack[2].column );
    }
    else
    {
        mtc::initializeColumn( aStack[0].column, aStack[3].column );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_PRECISION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNvl2Calculate( mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt         aRemain,
                         void*,
                         mtcTemplate* aTemplate )
{
    mtcNode   * sNode;
    mtcExecute* sExecute;
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );
    
    sNode    = aNode->arguments;
    sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

    IDE_TEST( sExecute->calculate( sNode,
                                   aStack + 1,
                                   aRemain - 1,
                                   sExecute->calculateInfo,
                                   aTemplate )
              != IDE_SUCCESS );

    sNode = sNode->next;

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        sNode = sNode->next;
    }

    sExecute = MTC_TUPLE_EXECUTE(&aTemplate->rows[sNode->table], sNode);

    IDE_TEST( sExecute->calculate( sNode,
                                   aStack + 1,
                                   aRemain - 1,
                                   sExecute->calculateInfo,
                                   aTemplate )
              != IDE_SUCCESS );

    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    idlOS::memcpy( aStack[0].value,
                   aStack[1].value,
                   aStack[1].column->module->actualSize( aStack[1].column,
                                                         aStack[1].value )
                 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNvl2CalculateXlobColumn( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    mtcNode*         sNode;
    MTC_CURSOR_PTR   sCursor;
    idBool           sFound;
    void           * sRow;
    idBool           sIsNull;

    UShort           sOrgTableID;
    mtcColumn      * sOrgLobColumn;
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );
    
    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );

    // Lob Locator를 얻는데 필요한 커서정보를 가져온다.
    IDE_TEST( aTemplate->getOpenedCursor( aTemplate,
                                          sNode->table,
                                          & sCursor,
                                          & sOrgTableID,
                                          & sFound )
              != IDE_SUCCESS );

    if ( sFound == ID_FALSE )
    {
        /* BUG-48006 partition table nvl2 function error */
        if ( ( aTemplate->rows[sOrgTableID].lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
             == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
        {
            sIsNull = ID_TRUE;
        }
        else
        {
            IDE_RAISE( ERR_CONVERSION_NOT_APPLICABLE );
        }
    }
    else
    {
        sRow    = aTemplate->rows[sOrgTableID].row;

        sOrgLobColumn = aTemplate->rows[sOrgTableID].columns + sNode->column;

        if( SMI_GRID_IS_VIRTUAL_NULL(aTemplate->rows[sOrgTableID].rid))
        {
            sIsNull = ID_TRUE;
        }
        else
        {
            IDE_TEST( mtc::isNullLobRow( sRow,
                                         & sOrgLobColumn->column,
                                         & sIsNull )
                      != IDE_SUCCESS );
        }
    }
    
    sNode = sNode->next;
    
    if( sIsNull == ID_TRUE )
    {
        sNode = sNode->next;
    }
    
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    
    idlOS::memcpy( aStack[0].value,
                   aStack[1].value,
                   aStack[1].column->module->actualSize( aStack[1].column,
                                                         aStack[1].value )
                   );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfNvl2CalculateXlobLocator( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*,
                                    mtcTemplate* aTemplate )
{
    mtcNode            * sNode;
    mtdBlobLocatorType   sLocator = MTD_LOCATOR_NULL;
    UInt                 sLength;
    idBool               sIsNull;
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack->column->column.offset );
    
    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );

    sLocator = *(mtdBlobLocatorType*)aStack[1].value;
    
    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );

    sNode = sNode->next;
    
    if( sIsNull == ID_TRUE )
    {
        sNode = sNode->next;
    }
    
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack + 1,
                                         aRemain - 1,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    
    idlOS::memcpy( aStack[0].value,
                   aStack[1].value,
                   aStack[1].column->module->actualSize( aStack[1].column,
                                                         aStack[1].value ) );
    
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}
