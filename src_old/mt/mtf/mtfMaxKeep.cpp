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
 * $Id: mtfMaxKeep.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfMaxKeep;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfMaxKeepFunctionName[1] = {
    { NULL, 8, (void*)"MAX_KEEP" }
};

static IDE_RC mtfMaxKeepEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule mtfMaxKeep = {
    2 | MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfMaxKeepFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMaxKeepEstimate
};

IDE_RC mtfMaxKeepInitialize( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

IDE_RC mtfMaxKeepAggregate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

IDE_RC mtfMaxKeepFinalize( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

IDE_RC mtfMaxKeepCalculate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfMaxKeepInitialize,
    mtfMaxKeepAggregate,
    mtf::calculateNA,
    mtfMaxKeepFinalize,
    mtfMaxKeepCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMaxKeepEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          /* aRemain */,
                           mtcCallBack * /* aCallBack */)
{
    aNode->lflag &= ~MTC_NODE_DISTINCT_MASK;

    IDE_TEST_RAISE( ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) <= 2 ) ||
                    ( aNode->funcArguments == NULL ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // PROJ-2002 Column Security
    // min함수와 같다.
    aNode->baseTable  = aNode->arguments->baseTable;
    aNode->baseColumn = aNode->arguments->baseColumn;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( aStack[0].column, aStack[1].column );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfKeepOrderData * ),
                                     0 )
              != IDE_SUCCESS );

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

IDE_RC mtfMaxKeepInitialize( mtcNode     * aNode,
                             mtcStack    *,
                             SInt         ,
                             void        *,
                             mtcTemplate * aTemplate )
{
    const mtcColumn       * sColumn;
    void                  * sValueTemp;
    iduMemory             * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo  * sFuncData;
    mtfKeepOrderData      * sKeepOrderData;
    mtdBinaryType         * sBinary;

    sColumn    = aTemplate->rows[aNode->table].columns + aNode->column;
    sValueTemp = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

    sColumn->module->null( sColumn, sValueTemp );

    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset);

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );

        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfMaxKeepInitialize::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfMaxKeepInitialize::alloc::sKeepOrderData",
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

IDE_RC mtfMaxKeepAggregate( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        *,
                            mtcTemplate * aTemplate )
{
    const mtdModule      * sModule;
    mtdValueInfo           sValueInfo1;
    mtdValueInfo           sValueInfo2;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    const mtcColumn      * sColumn;
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
    sModule        = aStack[0].column->module;

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

        idlOS::memcpy( aStack[0].value,
                       aStack[1].value,
                       sModule->actualSize( aStack[1].column,
                                            aStack[1].value ) );
    }
    else
    {
        mtf::getKeepAction( aStack + MTF_KEEP_ORDERBY_POS,
                            sKeepOrderData,
                            &sAction );

        if ( sAction == MTF_KEEP_ACTION_INIT )
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           sModule->actualSize( aStack[1].column,
                                                aStack[1].value ) );
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // NULL을 비교 대상에서 제외하기 위하여 Descending Key Compare를 사용함.
            sValueInfo1.column = aStack[0].column;
            sValueInfo1.value  = aStack[0].value;
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = aStack[1].column;
            sValueInfo2.value  = aStack[1].value;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            if ( sModule->logicalCompare[MTD_COMPARE_DESCENDING]( &sValueInfo1,
                                                                  &sValueInfo2 ) > 0 )
            {
                idlOS::memcpy( aStack[0].value,
                               aStack[1].value,
                               sModule->actualSize( aStack[1].column,
                                                    aStack[1].value ) );
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


IDE_RC mtfMaxKeepFinalize( mtcNode     *,
                           mtcStack    *,
                           SInt,
                           void        *,
                           mtcTemplate * )
{
    return IDE_SUCCESS;
}

IDE_RC mtfMaxKeepCalculate( mtcNode     * aNode,
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

