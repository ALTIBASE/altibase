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
 * $Id: mtfIsNull.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfIsNotNull;

extern mtfModule mtfIsNull;

static mtcName mtfIsNullFunctionName[2] = {
    { mtfIsNullFunctionName+1, 7, (void*)"IS NULL" },
    { NULL, 6, (void*)"ISNULL" }
};

static IDE_RC mtfIsNullEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfIsNull = {
    1|MTC_NODE_OPERATOR_RANGED|MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_PRINT_FMT_POSTFIX_SP|MTC_NODE_EAT_NULL_TRUE,
    ~0,
    1.0/10.0,  // TODO : default selectivity
    mtfIsNullFunctionName,
    &mtfIsNotNull,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfIsNullEstimate
};

IDE_RC mtfIsNullExtractRange( mtcNode*       aNode,
                              mtcTemplate*   aTemplate,
                              mtkRangeInfo * aInfo,
                              smiRange*      aRange );

IDE_RC mtfIsNullCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

IDE_RC mtfIsNullCalculateXlobColumn( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC mtfIsNullCalculateXlobLocator( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfIsNullCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeDefault,
    mtfIsNullExtractRange
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static const mtcExecute mtfExecuteXlobValue = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfIsNullCalculate,
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
    mtfIsNullCalculateXlobColumn,
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
    mtfIsNullCalculateXlobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfIsNullEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt         /* aRemain */,
                          mtcCallBack* /* aCallBack */ )
{
    extern mtdModule mtdBoolean;

    mtcNode* sNode;
    ULong    sLflag;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for( sNode  = aNode->arguments, sLflag = MTC_NODE_INDEX_UNUSABLE;
         sNode != NULL;
         sNode  = sNode->next )
    {
        if( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
            MTC_NODE_COMPARISON_TRUE )
        {
            sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
            sNode->lflag |= MTC_NODE_INDEX_UNUSABLE;
        }
        sLflag |= sNode->lflag & MTC_NODE_INDEX_MASK;
    }

    aNode->lflag &= ~(MTC_NODE_INDEX_MASK);
    aNode->lflag |= sLflag;

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (aStack[1].column->module->id == MTD_LIST_ID ) ||
                    (aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
                    (aStack[1].column->module->id == MTD_RECORDTYPE_ID ) ||
                    (aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    if ( (aStack[1].column->module->id == MTD_BLOB_ID) ||
         (aStack[1].column->module->id == MTD_CLOB_ID) )
    {
        // PROJ-1362
        if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobColumn;
        }
        else
        {
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobValue;
        }
    }
    else if ( (aStack[1].column->module->id == MTD_BLOB_LOCATOR_ID) ||
              (aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID) )
    {
        // PROJ-1362
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocator;
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }

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

IDE_RC mtfIsNullExtractRange( mtcNode*,
                              mtcTemplate*,
                              mtkRangeInfo * aInfo,
                              smiRange*      aRange )
{
    mtkRangeCallBack* sMinimumCallBack;
    mtkRangeCallBack* sMaximumCallBack;
    
    sMinimumCallBack = (mtkRangeCallBack*)( aRange + 1 );
    sMaximumCallBack = sMinimumCallBack + 1;
    
    aRange->prev                 = NULL;
    aRange->next                 = NULL;

    //---------------------------
    // RangeCallBack 설정 
    //---------------------------

    if ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ||
         aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL )
    {
        aRange->minimum.callback = mtk::rangeCallBackGE4Mtd;
        aRange->maximum.callback = mtk::rangeCallBackLE4Mtd;
    }
    else
    {
        if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
             ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
        {
            aRange->minimum.callback = mtk::rangeCallBackGE4Stored;
            aRange->maximum.callback = mtk::rangeCallBackLE4Stored;
        }
        else
        {
            /* PROJ-2433 */
            aRange->minimum.callback = mtk::rangeCallBackGE4IndexKey;
            aRange->maximum.callback = mtk::rangeCallBackLE4IndexKey;
        }
    }
    
    aRange->minimum.data         = sMinimumCallBack;
    aRange->maximum.data         = sMaximumCallBack;

    //---------------------------
    // MinimumCallBack 정보 설정
    //---------------------------
            
    sMinimumCallBack->next       = NULL;
    if ( MTC_COLUMN_IS_NOT_SAME( sMinimumCallBack->columnDesc, aInfo->column ) )
    {
        sMinimumCallBack->columnDesc = *aInfo->column;
    }
    sMinimumCallBack->columnIdx  = aInfo->columnIdx;
    //sMinimumCallBack->valueDesc  = NULL;
    sMinimumCallBack->value      = NULL;
    
    sMaximumCallBack->next       = NULL;
    if ( MTC_COLUMN_IS_NOT_SAME( sMaximumCallBack->columnDesc, aInfo->column ) )
    {
        sMaximumCallBack->columnDesc = *aInfo->column;
    }
    sMaximumCallBack->columnIdx  = aInfo->columnIdx;
    //sMaximumCallBack->valueDesc  = NULL;
    sMaximumCallBack->value      = NULL;

    //---------------------------
    // MaximumCallBack 정보 설정
    //---------------------------

    if ( ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_INDEX_KEY_FIXED_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_INDEX_KEY_MTDVAL ) )
    {
        sMinimumCallBack->compare = mtk::compareMaximumLimit4Mtd;
        sMaximumCallBack->compare = mtk::compareMaximumLimit4Mtd;
    }
    else
    {
        sMinimumCallBack->compare = mtk::compareMaximumLimit4Stored;
        sMaximumCallBack->compare = mtk::compareMaximumLimit4Stored;
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfIsNullCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfIsNullCalculateXlobColumn( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    MTC_CURSOR_PTR   sCursor;
    idBool           sFound;
    void           * sRow;
    idBool           sIsNull;

    UShort           sOrgTableID;
    mtcColumn      * sOrgLobColumn;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    // PROJ-1362
    // Lob Locator를 얻는데 필요한 커서정보를 가져온다.
    IDE_TEST( aTemplate->getOpenedCursor( aTemplate,
                                          aNode->arguments->table,
                                          & sCursor,
                                          & sOrgTableID,
                                          & sFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sFound != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    sRow    = aTemplate->rows[sOrgTableID].row;

    sOrgLobColumn = aTemplate->rows[sOrgTableID].columns + aNode->arguments->column;

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
    
    if ( sIsNull == ID_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfIsNullCalculateXlobLocator( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    mtdBlobLocatorType sLocator = MTD_LOCATOR_NULL;
    UInt               sLength;
    idBool             sIsNull;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sLocator = *(mtdBlobLocatorType*)aStack[1].value;
    
    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );
    
    if ( sIsNull == ID_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
        
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}
