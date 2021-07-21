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
 * $Id: mtfCountKeep.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfCountKeep;

extern mtdModule mtdList;
extern mtdModule mtdBigint;
extern mtdModule mtdBinary;

static mtcName mtfCountKeepFunctionName[1] = {
    { NULL, 10, (void*)"COUNT_KEEP" }
};

static IDE_RC mtfCountKeepEstimate( mtcNode     * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    mtcCallBack * aCallBack );

mtfModule mtfCountKeep = {
    2 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfCountKeepFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfCountKeepEstimate
};

IDE_RC mtfCountKeepInitialize( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate );

IDE_RC mtfCountKeepAggregateAsterisk( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfCountKeepAggregate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

IDE_RC mtfCountKeepAggregateXlobColumn( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

IDE_RC mtfCountKeepAggregateXlobLocator( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * aInfo,
                                         mtcTemplate * aTemplate );

IDE_RC mtfCountKeepFinalize( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

IDE_RC mtfCountKeepCalculate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

static const mtcExecute mtfExecuteAsterisk = {
    mtfCountKeepInitialize,
    mtfCountKeepAggregateAsterisk,
    mtf::calculateNA,
    mtfCountKeepFinalize,
    mtfCountKeepCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecute = {
    mtfCountKeepInitialize,
    mtfCountKeepAggregate,
    mtf::calculateNA,
    mtfCountKeepFinalize,
    mtfCountKeepCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteXlobColumn = {
    mtfCountKeepInitialize,
    mtfCountKeepAggregateXlobColumn,
    mtf::calculateNA,
    mtfCountKeepFinalize,
    mtfCountKeepCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteXlobLocator = {
    mtfCountKeepInitialize,
    mtfCountKeepAggregateXlobLocator,
    mtf::calculateNA,
    mtfCountKeepFinalize,
    mtfCountKeepCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCountKeepEstimate( mtcNode     * aNode,
                             mtcTemplate * aTemplate,
                             mtcStack    * aStack,
                             SInt          /*aRemain*/,
                             mtcCallBack * /*aCallBack*/ )
{
    mtcNode *   sNode;
    idBool      sTransForm = ID_TRUE;
    UInt        sArgCount;
    UInt        sFuncArgCount;
    idBool      sIsAsterisk = ID_FALSE;

    IDE_TEST_RAISE( aNode->funcArguments == NULL,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sArgCount = (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK);
    sFuncArgCount = (aNode->funcArguments->lflag & MTC_NODE_ARGUMENT_COUNT_MASK);

    if ( sArgCount == ( sFuncArgCount + 1 ) )
    {
        IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) !=
                        MTC_NODE_QUANTIFIER_TRUE,
                        ERR_INVALID_FUNCTION_ARGUMENT );
        sIsAsterisk = ID_TRUE;
    }
    else
    {
        IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) <= 2,
                        ERR_INVALID_FUNCTION_ARGUMENT );
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    sNode  = aNode->arguments;

    if ( sIsAsterisk == ID_TRUE )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteAsterisk;
    }
    else
    {
        if ( (aStack[1].column->module->id == MTD_BLOB_ID) ||
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
            // BUG-38416 CountKeep(column) to CountKeep(*)
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

                if ( ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
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

                // BUG-39537 CountKeep(*) 와 동일한 flag로 설정한다.
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
    }

    //IDE_TEST( mtdBigint.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfKeepOrderData * ),
                                     0 )
              != IDE_SUCCESS );

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 0 )
    {
        IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                        ERR_CONVERSION_NOT_APPLICABLE );
    }
    else
    {
        /* Nothing to do */
    }

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

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

IDE_RC mtfCountKeepInitialize( mtcNode     * aNode,
                               mtcStack    *,
                               SInt,
                               void        *,
                               mtcTemplate * aTemplate )
{
    const mtcColumn       * sColumn;
    iduMemory             * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo  * sFuncData;
    mtfKeepOrderData      * sKeepOrderData;
    mtdBinaryType         * sBinary;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                        sColumn->column.offset) = 0;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);

    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );

        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfCountKeepInitialize::alloc::sFuncData",
                             ERR_MEMORY_ALLOCATION );
        IDE_TEST_RAISE( sMemoryMgr->alloc( ID_SIZEOF(mtfFuncDataBasicInfo),
                                           (void**)&sFuncData )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

        // function data init
        IDE_TEST( mtf::initializeFuncDataBasicInfo( sFuncData,
                                                    sMemoryMgr )
                  != IDE_SUCCESS );

        // 등록
        aTemplate->funcData[aNode->info] = sFuncData;
    }
    else
    {
        sFuncData = aTemplate->funcData[aNode->info];
    }

    IDU_FIT_POINT_RAISE( "mtfCountKeepInitialize::alloc::sKeepOrderData",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( sFuncData->memoryMgr->alloc( ID_SIZEOF( mtfKeepOrderData ),
                                                 (void**)&sKeepOrderData )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    sKeepOrderData->mIsFirst               = ID_TRUE;
    sBinary->mLength                       = ID_SIZEOF( mtfKeepOrderData * );
    *((mtfKeepOrderData**)sBinary->mValue) = sKeepOrderData;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sMemoryMgr != NULL )
    {
        mtf::freeFuncDataMemory( sMemoryMgr );
        aTemplate->funcData[aNode->info] = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC mtfCountKeepAggregateAsterisk( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        *,
                                      mtcTemplate * aTemplate )
{
    const mtcColumn      * sColumn;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    mtdBinaryType        * sBinary;
    mtdCharType          * sOption;
    UInt                   sAction;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);
    sKeepOrderData = *((mtfKeepOrderData**)sBinary->mValue);

    if ( sKeepOrderData->mIsFirst == ID_TRUE )
    {
        sKeepOrderData->mIsFirst = ID_FALSE;
        sOption                  = (mtdCharType *)aStack[1].value;
        sFuncData                = aTemplate->funcData[aNode->info];

        IDE_TEST( mtf::setKeepOrderData( aNode->funcArguments->next,
                                         aStack,
                                         sFuncData->memoryMgr,
                                         (UChar *)sOption->value,
                                         sKeepOrderData,
                                         MTF_KEEP_ORDERBY_POS -1 )
                  != IDE_SUCCESS );

        *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                            sColumn->column.offset) = 1;
    }
    else
    {
        mtf::getKeepAction( aStack + MTF_KEEP_ORDERBY_POS - 1,
                            sKeepOrderData,
                            &sAction );

        if ( sAction == MTF_KEEP_ACTION_INIT )
        {
            *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn->column.offset) = 1;
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn->column.offset) += 1;
        }
        else
        {
            /* Nothing to do */
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCountKeepAggregate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        *,
                              mtcTemplate * aTemplate )
{
    const mtcColumn      * sColumn;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    mtdBinaryType        * sBinary;
    mtdCharType          * sOption;
    UInt                   sAction;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);
    sKeepOrderData = *((mtfKeepOrderData**)sBinary->mValue);

    if ( sKeepOrderData->mIsFirst == ID_TRUE )
    {
        sKeepOrderData->mIsFirst = ID_FALSE;
        sOption                  = (mtdCharType *)aStack[2].value;
        sFuncData                = aTemplate->funcData[aNode->info];

        IDE_TEST( mtf::setKeepOrderData( aNode->funcArguments->next,
                                         aStack,
                                         sFuncData->memoryMgr,
                                         (UChar *)sOption->value,
                                         sKeepOrderData,
                                         MTF_KEEP_ORDERBY_POS )
                  != IDE_SUCCESS );

        if ( aStack[1].column->module->isNull( aStack[1].column,
                                               aStack[1].value ) != ID_TRUE )
        {
            *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn->column.offset) = 1;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        mtf::getKeepAction( aStack + MTF_KEEP_ORDERBY_POS,
                            sKeepOrderData,
                            &sAction );

        if ( sAction == MTF_KEEP_ACTION_INIT )
        {
            if ( aStack[1].column->module->isNull( aStack[1].column,
                                                   aStack[1].value ) != ID_TRUE )
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) = 1;
            }
            else
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) = 0;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            if ( aStack[1].column->module->isNull( aStack[1].column,
                                                   aStack[1].value ) != ID_TRUE )
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) += 1;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCountKeepAggregateXlobColumn( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        *,
                                        mtcTemplate * aTemplate )
{
    MTC_CURSOR_PTR         sCursor;
    idBool                 sFound;
    void                 * sRow;
    idBool                 sIsNull;
    UShort                 sOrgTableID;
    const mtcColumn      * sOrgLobColumn;
    const mtcColumn      * sColumn;
    mtfFuncDataBasicInfo * sFuncData;
    mtfKeepOrderData     * sKeepOrderData;
    mtdBinaryType        * sBinary;
    mtdCharType          * sOption;
    UInt                   sAction;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    // Lob Locator를 얻는데 필요한 커서정보를 가져온다.
    IDE_TEST( aTemplate->getOpenedCursor( aTemplate,
                                          aNode->arguments->table,
                                          &sCursor,
                                          &sOrgTableID,
                                          &sFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sFound != ID_TRUE,
                    ERR_CONVERSION_NOT_APPLICABLE );

    sRow          = aTemplate->rows[sOrgTableID].row;
    sOrgLobColumn = aTemplate->rows[sOrgTableID].columns + aNode->arguments->column;

    if ( SMI_GRID_IS_VIRTUAL_NULL( aTemplate->rows[sOrgTableID].rid ) )
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

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);
    sKeepOrderData = *((mtfKeepOrderData**)sBinary->mValue);

    if ( sKeepOrderData->mIsFirst == ID_TRUE )
    {
        sKeepOrderData->mIsFirst = ID_FALSE;
        sOption                  = (mtdCharType *)aStack[2].value;
        sFuncData                = aTemplate->funcData[aNode->info];

        IDE_TEST( mtf::setKeepOrderData( aNode->funcArguments->next,
                                         aStack,
                                         sFuncData->memoryMgr,
                                         (UChar *)sOption->value,
                                         sKeepOrderData,
                                         MTF_KEEP_ORDERBY_POS )
                  != IDE_SUCCESS );

        if ( sIsNull == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn->column.offset) = 1;
        }
    }
    else
    {
        mtf::getKeepAction( aStack + MTF_KEEP_ORDERBY_POS,
                            sKeepOrderData,
                            &sAction );

        if ( sAction == MTF_KEEP_ACTION_INIT )
        {
            if ( sIsNull == ID_TRUE )
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) = 0;
            }
            else
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) = 1;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            if ( sIsNull == ID_TRUE )
            {
                // Nothing to do.
            }
            else
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) += 1;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCountKeepAggregateXlobLocator( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        *,
                                         mtcTemplate * aTemplate )
{
    const mtcColumn      * sColumn;
    mtdClobLocatorType     sLocator = MTD_LOCATOR_NULL;
    UInt                   sLength;
    idBool                 sIsNull;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    mtdBinaryType        * sBinary;
    mtdCharType          * sOption;
    UInt                   sAction;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    sLocator = *(mtdBlobLocatorType*)aStack[1].value;

    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);
    sKeepOrderData = *((mtfKeepOrderData**)sBinary->mValue);

    if ( sKeepOrderData->mIsFirst == ID_TRUE )
    {
        sKeepOrderData->mIsFirst = ID_FALSE;
        sOption                  = (mtdCharType *)aStack[2].value;
        sFuncData                = aTemplate->funcData[aNode->info];

        IDE_TEST( mtf::setKeepOrderData( aNode->funcArguments->next,
                                         aStack,
                                         sFuncData->memoryMgr,
                                         (UChar *)sOption->value,
                                         sKeepOrderData,
                                         MTF_KEEP_ORDERBY_POS )
                  != IDE_SUCCESS );

        if ( sIsNull == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn->column.offset) = 1;
        }
    }
    else
    {
        mtf::getKeepAction( aStack + MTF_KEEP_ORDERBY_POS,
                            sKeepOrderData,
                            &sAction );

        if ( sAction == MTF_KEEP_ACTION_INIT )
        {
            if ( sIsNull == ID_TRUE )
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) = 0;
            }
            else
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) = 1;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            if ( sIsNull == ID_TRUE )
            {
                // Nothing to do.
            }
            else
            {
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn->column.offset) += 1;
            }
        }
        else
        {
            /* Nothing to do */
        }

    }

    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );

    return IDE_FAILURE;
}

IDE_RC mtfCountKeepFinalize( mtcNode     *,
                             mtcStack    *,
                             SInt,
                             void        *,
                             mtcTemplate * )
{
    return IDE_SUCCESS;
}

IDE_RC mtfCountKeepCalculate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt,
                              void        *,
                              mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row +
                              aStack->column->column.offset );

    return IDE_SUCCESS;
}

