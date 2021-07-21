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
 * $Id: mtfIsNotNull.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfIsNull;

extern mtfModule mtfIsNotNull;

/* PROJ-2632 */
extern mtxModule mtxIsNotNull;

static mtcName mtfIsNotNullFunctionName[1] = {
    { NULL, 11, (void*)"IS NOT NULL" }
};

static IDE_RC mtfIsNotNullEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfIsNotNull = {
    1|MTC_NODE_OPERATOR_NOT_RANGED|MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_PRINT_FMT_POSTFIX_SP|MTC_NODE_EAT_NULL_TRUE,
    ~0,
    9.0/10.0,  // TODO : default selectivity 
    mtfIsNotNullFunctionName,
    &mtfIsNull,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfIsNotNullEstimate
};

IDE_RC mtfIsNotNullExtractRange( mtcNode*      aNode,
                                 mtcTemplate*  aTemplate,
                                 mtkRangeInfo* aInfo,
                                 smiRange*     aRange );

IDE_RC mtfIsNotNullCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

IDE_RC mtfIsNotNullCalculateXlobColumn( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfIsNotNullCalculateXlobLocator( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfIsNotNullCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeDefault,
    mtfIsNotNullExtractRange
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static const mtcExecute mtfExecuteXlobValue = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfIsNotNullCalculate,
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
    mtfIsNotNullCalculateXlobColumn,
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
    mtfIsNotNullCalculateXlobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfIsNotNullEstimate( mtcNode*     aNode,
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
        
        /* PROJ-2632 */
        aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
            = mtxIsNotNull.mGetExecute( aStack[1].column->module->id,
                                        aStack[1].column->module->id );
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

IDE_RC mtfIsNotNullExtractRange( mtcNode*,
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
        aRange->maximum.callback = mtk::rangeCallBackLT4Mtd;
    }
    else
    {
        if ( ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL ) ||
             ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_STOREDVAL ) )
        {
            aRange->minimum.callback = mtk::rangeCallBackGE4Stored;
            aRange->maximum.callback = mtk::rangeCallBackLT4Stored;
        }
        else
        {
            /* PROJ-2433 */
            aRange->minimum.callback = mtk::rangeCallBackGE4IndexKey;
            aRange->maximum.callback = mtk::rangeCallBackLT4IndexKey;
        }
    }

    aRange->minimum.data         = sMinimumCallBack;
    aRange->maximum.data         = sMaximumCallBack;

    //---------------------------
    // MinimumCallBack 정보 설정
    //---------------------------
            
    sMinimumCallBack->next       = NULL;
    sMinimumCallBack->compare    = mtk::compareMinimumLimit;
    sMinimumCallBack->columnIdx  = aInfo->columnIdx;
    //sMinimumCallBack->columnDesc = NULL;
    //sMinimumCallBack->valueDesc  = NULL;
    sMinimumCallBack->value      = NULL;

    //---------------------------
    // MaximumCallBack 정보 설정
    //---------------------------
    
    sMaximumCallBack->next       = NULL;
    if ( MTC_COLUMN_IS_NOT_SAME( sMaximumCallBack->columnDesc, aInfo->column ) )
    {
        sMaximumCallBack->columnDesc = *aInfo->column;
    }
    sMaximumCallBack->columnIdx  = aInfo->columnIdx;
    //sMaximumCallBack->valueDesc  = NULL;
    sMaximumCallBack->value      = NULL;

    if ( ( aInfo->compValueType == MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_MTDVAL_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_INDEX_KEY_FIXED_MTDVAL ) ||
         ( aInfo->compValueType == MTD_COMPARE_INDEX_KEY_MTDVAL ) )
    {
        sMaximumCallBack->compare    = mtk::compareMaximumLimit4Mtd;
    }
    else // ( aInfo->compValueType == MTD_COMPARE_STOREDVAL_MTDVAL )
    {
        sMaximumCallBack->compare    = mtk::compareMaximumLimit4Stored;
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfIsNotNullCalculate( mtcNode*     aNode,
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
                                          aStack[1].value ) == ID_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfIsNotNullCalculateXlobColumn( mtcNode*     aNode,
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
    
    sRow = aTemplate->rows[sOrgTableID].row;

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
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfIsNotNullCalculateXlobLocator( mtcNode*     aNode,
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
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }

    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}
