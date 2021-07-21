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
 * $Id: mtfCount.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfCount;

extern mtdModule mtdList;
extern mtdModule mtdBigint;

static mtcName mtfCountFunctionName[1] = {
    { NULL, 5, (void*)"COUNT" }
};

static IDE_RC mtfCountEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfCount = {
    1|MTC_NODE_OPERATOR_AGGREGATION|
      MTC_NODE_PRINT_FMT_MISC|
      MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfCountFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfCountEstimate
};

IDE_RC mtfCountInitialize(  mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

IDE_RC mtfCountAggregateAsterisk(  mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfCountAggregate(  mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

IDE_RC mtfCountAggregateXlobColumn( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfCountAggregateXlobLocator(  mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

IDE_RC mtfCountMerge(  mtcNode*     aNode,
                       mtcStack*    aStack,
                       SInt         aRemain,
                       void*        aInfo,
                       mtcTemplate* aTemplate );

IDE_RC mtfCountFinalize(  mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

IDE_RC mtfCountCalculate(  mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecuteAsterisk = {
    mtfCountInitialize,
    mtfCountAggregateAsterisk,
    mtfCountMerge,
    mtfCountFinalize,
    mtfCountCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecute = {
    mtfCountInitialize,
    mtfCountAggregate,
    mtfCountMerge,
    mtfCountFinalize,
    mtfCountCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteXlobColumn = {
    mtfCountInitialize,
    mtfCountAggregateXlobColumn,
    mtfCountMerge,
    mtfCountFinalize,
    mtfCountCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteXlobLocator = {
    mtfCountInitialize,
    mtfCountAggregateXlobLocator,
    mtfCountMerge,
    mtfCountFinalize,
    mtfCountCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCountEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt         /* aRemain */,
                         mtcCallBack* /* aCallBack */ )
{
    mtcNode*    sNode;
    idBool      sTransForm = ID_TRUE;

    if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 0 )
    {
        IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) != 
                        MTC_NODE_QUANTIFIER_TRUE,
                        ERR_INVALID_FUNCTION_ARGUMENT );
    }
    else
    {
        IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                        ERR_INVALID_FUNCTION_ARGUMENT );
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    sNode  = aNode->arguments;

    if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 0 )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteAsterisk;
    }
    else if ( (aStack[1].column->module->id == MTD_BLOB_ID) ||
              (aStack[1].column->module->id == MTD_CLOB_ID) )
    {
        if ( aTemplate->isBaseTable( aTemplate, sNode->table ) == ID_TRUE )
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
        // BUG-38416 count(column) to count(*)
        if ( MTU_COUNT_COLUMN_TO_COUNT_ASTAR == 1 )
        {
            if ( (aNode->lflag & MTC_NODE_DISTINCT_MASK)
                == MTC_NODE_DISTINCT_TRUE )
            {
                sTransForm = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }

            if( ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
                    == MTC_TUPLE_VIEW_TRUE ) ||
                  ( idlOS::strncmp((SChar*)aNode->arguments->module->names->string,
                                  (const SChar*)"COLUMN", 6 ) != 0 ) )
            {
                sTransForm = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }

            if ( (aStack[1].column->flag & MTC_COLUMN_NOTNULL_MASK) ==
                MTC_COLUMN_NOTNULL_FALSE )
            {
                sTransForm = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sTransForm = ID_FALSE;
        }

        if ( sTransForm == ID_TRUE )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteAsterisk;

            aNode->arguments = NULL;
            aNode->lflag    &= ~MTC_NODE_ARGUMENT_COUNT_MASK;

            // BUG-39537 COUNT(*) 와 동일한 flag로 설정한다.
            aNode->lflag &= ~MTC_NODE_QUANTIFIER_MASK;
            aNode->lflag |= MTC_NODE_QUANTIFIER_TRUE;

            // BUG-38935 인자가 최적화에 의해 제거되었음
            aNode->lflag &= ~MTC_NODE_REMOVE_ARGUMENTS_MASK;
            aNode->lflag |= MTC_NODE_REMOVE_ARGUMENTS_TRUE;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
        }
    }

    //IDE_TEST( mtdBigint.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 0 )
    {
        IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCountInitialize( mtcNode*     aNode,
                           mtcStack*,
                           SInt,
                           void*,
                           mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn->column.offset) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfCountAggregateAsterisk( mtcNode*     aNode,
                                  mtcStack*,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn->column.offset) += 1;
    
    return IDE_SUCCESS;
}

IDE_RC mtfCountAggregate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*,
                          mtcTemplate* aTemplate )
{
    mtcNode*         sNode;
    const mtcColumn* sColumn;
    
    // BUG-33674
    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                       aStack,
                                                                      aRemain,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[0].column->module->isNull( aStack[0].column,
                                          aStack[0].value ) != ID_TRUE )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn->column.offset) += 1;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfCountAggregateXlobColumn( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*,
                                    mtcTemplate* aTemplate )
{
    mtcNode*         sNode;
    const mtcColumn* sColumn;
    MTC_CURSOR_PTR   sCursor;
    idBool           sFound;
    void           * sRow;
    idBool           sIsNull;
    UShort           sOrgTableID;
    const mtcColumn* sOrgLobColumn;
    
    // BUG-33674
    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                       aStack,
                                                                      aRemain,
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
    
    IDE_TEST_RAISE( sFound != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

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
    
    if ( sIsNull == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn->column.offset) += 1;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfCountAggregateXlobLocator( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*,
                                     mtcTemplate* aTemplate )
{
    mtcNode            * sNode;
    const mtcColumn    * sColumn;
    mtdClobLocatorType   sLocator = MTD_LOCATOR_NULL;
    UInt                 sLength;
    idBool               sIsNull;
    
    // BUG-33674
    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                       aStack,
                                                                      aRemain,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );

    sLocator = *(mtdBlobLocatorType*)aStack[0].value;
    
    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );
    
    if ( sIsNull == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn->column.offset) += 1;
    }
    
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}

IDE_RC mtfCountMerge(  mtcNode*     aNode,
                       mtcStack*    ,
                       SInt         ,
                       void*        aInfo,
                       mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    *(mtdBigintType*)(sDstRow + sColumn->column.offset) +=
    *(mtdBigintType*)(sSrcRow + sColumn->column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfCountFinalize( mtcNode*,
                         mtcStack*,
                         SInt,
                         void*,
                         mtcTemplate* )
{
    return IDE_SUCCESS;
}

IDE_RC mtfCountCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt,
                          void*,
                          mtcTemplate* aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
}
 
