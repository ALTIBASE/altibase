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
 * $Id: mtfSumKeep.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfSumKeep;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfSumKeepFunctionName[1] = {
    { NULL, 8, (void*)"SUM_KEEP" }
};

static IDE_RC mtfSumKeepInitialize( void );

static IDE_RC mtfSumKeepFinalize( void );

static IDE_RC mtfSumKeepEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule mtfSumKeep = {
    3 | MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSumKeepFunctionName,
    NULL,
    mtfSumKeepInitialize,
    mtfSumKeepFinalize,
    mtfSumKeepEstimate
};

static IDE_RC mtfSumKeepEstimateFloat( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

static IDE_RC mtfSumKeepEstimateDouble( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallBack );

static IDE_RC mtfSumKeepEstimateBigint( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfSumKeepEstimates[3] = {
    { mtfSumKeepEstimates+1, mtfSumKeepEstimateDouble },
    { mtfSumKeepEstimates+2, mtfSumKeepEstimateBigint },
    { NULL, mtfSumKeepEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfSumKeepEstimatesHighPerformance[2] = {
    { mtfSumKeepEstimatesHighPerformance+1, mtfSumKeepEstimateDouble },
    { NULL, mtfSumKeepEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;
static mtfSubModule** mtfTableHighPerformance = NULL;

IDE_RC mtfSumKeepInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfSumKeepEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfSumKeepEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack )
{
    const mtfSubModule  * sSubModule;
    mtfSubModule       ** sTable;

    IDE_TEST_RAISE( ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) <= 2 ) ||
                    ( aNode->funcArguments == NULL ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // BUG-41994
    aTemplate->arithmeticOpModeRef = ID_TRUE;
    if ( aTemplate->arithmeticOpMode == MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL2 )
    {
        sTable = mtfTableHighPerformance;
    }
    else
    {
        sTable = mtfTable;
    }

    IDE_TEST( mtf::getSubModule1Arg( &sSubModule,
                                     sTable,
                                     aStack[1].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST( sSubModule->estimate( aNode,
                                    aTemplate,
                                    aStack,
                                    aRemain,
                                    aCallBack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: FLOAT */

IDE_RC mtfSumKeepInitializeFloat( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfSumKeepAggregateFloat( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfSumKeepFinalizeFloat( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

IDE_RC mtfSumKeepCalculateFloat( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

static const mtcExecute mtfSumKeepExecuteFloat = {
    mtfSumKeepInitializeFloat,
    mtfSumKeepAggregateFloat,
    mtf::calculateNA,
    mtfSumKeepFinalizeFloat,
    mtfSumKeepCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSumKeepEstimateFloat( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt,
                                mtcCallBack * aCallBack )
{
    const mtdModule * sModules[1];
    mtcNode         * sNode;

    mtc::makeFloatConversionModule( aStack + 1, &sModules[0] );

    sNode = aNode->arguments->next;
    aNode->arguments->next = NULL;
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    aNode->arguments->next = sNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSumKeepExecuteFloat;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // SumKeep 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     &mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfKeepOrderData * ),
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepInitializeFloat( mtcNode     * aNode,
                                  mtcStack    *,
                                  SInt         ,
                                  void        *,
                                  mtcTemplate * aTemplate )
{
    const mtcColumn       * sColumn;
    mtdNumericType        * sFloat;
    iduMemory             * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo  * sFuncData;
    mtfKeepOrderData      * sKeepOrderData;
    mtdBinaryType         * sBinary;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sFloat = (mtdNumericType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[0].column.offset);
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row +
                                sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                                        sColumn[2].column.offset);
    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );
        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfSumKeepInitializeFloat::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfSumKeepInitializeFloat::alloc::sKeepOrderData",
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

IDE_RC mtfSumKeepAggregateFloat( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        *,
                                 mtcTemplate * aTemplate )
{
    const mtdModule      * sModule;
    const mtcColumn      * sColumn;
    mtdNumericType       * sFloatSumKeep;
    mtdNumericType       * sFloatArgument;
    UChar                  sFloatSumKeepBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType       * sFloatSumKeepClone;
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

    sModule = aStack[1].column->module;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[2].column.offset);
    sKeepOrderData = *((mtfKeepOrderData**)sBinary->mValue);
    sFloatSumKeep  = (mtdNumericType*)((UChar*)aTemplate->rows[aNode->table].row +
                                                sColumn->column.offset);
    sFloatArgument = (mtdNumericType*)aStack[1].value;

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

        if ( sModule->isNull( aStack[1].column,
                              aStack[1].value ) != ID_TRUE )
        {
            idlOS::memcpy( sFloatSumKeep, sFloatArgument, sFloatArgument->length + 1 );

            *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                        sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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
            if ( sModule->isNull( aStack[1].column,
                                  aStack[1].value ) != ID_TRUE )
            {
                idlOS::memcpy( sFloatSumKeep, sFloatArgument, sFloatArgument->length + 1 );
                *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                            sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
            }
            else
            {
                sFloatSumKeep->length       = 1;
                sFloatSumKeep->signExponent = 0x80;

                *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row +
                                            sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            if ( sModule->isNull( aStack[1].column,
                                  aStack[1].value ) != ID_TRUE )
            {
                sFloatSumKeepClone = (mtdNumericType*)sFloatSumKeepBuff;
                idlOS::memcpy( sFloatSumKeepClone, sFloatSumKeep, sFloatSumKeep->length + 1 );

                IDE_TEST( mtc::addFloat( sFloatSumKeep,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sFloatSumKeepClone,
                                         sFloatArgument )
                          != IDE_SUCCESS );

                *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                            sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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

IDE_RC mtfSumKeepFinalizeFloat( mtcNode     * aNode,
                                mtcStack    *,
                                SInt,
                                void        *,
                                mtcTemplate * aTemplate )
{
    mtcColumn * sSumKeepColumn;
    void      * sValueTemp;

    sSumKeepColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                    sSumKeepColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumKeepColumn,
                                 aTemplate->rows[aNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumKeepColumn->module->staticNull );

        mtdFloat.null( sSumKeepColumn,
                       sValueTemp );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumKeepCalculateFloat( mtcNode     * aNode,
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

/* ZONE: DOUBLE */

IDE_RC mtfSumKeepInitializeDouble( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfSumKeepAggregateDouble( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfSumKeepAggregateDoubleFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfSumKeepFinalizeDouble( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfSumKeepFinalizeDouble( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfSumKeepCalculateDouble( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

static const mtcExecute mtfSumKeepExecuteDouble = {
    mtfSumKeepInitializeDouble,
    mtfSumKeepAggregateDouble,
    mtf::calculateNA,
    mtfSumKeepFinalizeDouble,
    mtfSumKeepCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// 최적의 SumKeep을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfSumKeepExecuteDoubleFast = {
    mtfSumKeepInitializeDouble,
    mtfSumKeepAggregateDoubleFast,
    mtf::calculateNA,
    mtfSumKeepFinalizeDouble,
    mtfSumKeepCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSumKeepEstimateDouble( mtcNode     * aNode,
                                 mtcTemplate * aTemplate,
                                 mtcStack    * aStack,
                                 SInt,
                                 mtcCallBack * aCallBack )
{
    const mtfModule        * sArgModule;
    mtcNode                * sNode;
    static const mtdModule * sModules[1] = {
        &mtdDouble
    };

    sNode = aNode->arguments->next;
    aNode->arguments->next = NULL;
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    aNode->arguments->next = sNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSumKeepExecuteDouble;

    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;

    // mtf::initializeTemplate에서 각 subModule에 대해
    // estimateBound를 호출하는데 이때에는 node에 module이
    // 안달려있기 때문에 NULL 체크를 해야 한다.
    if ( sArgModule != NULL )
    {
        // SumKeep(i1) 처럼 i1가 단일 컬럼이고 conversion되지 않는다면
        // 최적화된 execution을 달아준다.
        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if ( ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
               == MTC_TUPLE_VIEW_FALSE ) &&
             ( idlOS::strncmp((SChar*)sArgModule->names->string,
                              (const SChar*)"COLUMN", 6 )
              == 0 ) &&
             ( aNode->arguments->conversion == NULL ) )
        {
            // sum(i1) keep ( dense_rank first order by i1,i2,i3 )
            // order by column 도 순수 철럼인지 확인해야한다.
            for ( sNode = aNode->arguments->next->next;
                  sNode != NULL;
                  sNode = sNode->next )
            {
                if ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_VIEW_MASK )
                       == MTC_TUPLE_VIEW_TRUE ) ||
                     ( idlOS::strncmp((SChar*)sNode->module->names->string,
                                     (const SChar*)"COLUMN", 6 )
                       != 0 ) ||
                     ( sNode->conversion != NULL ) )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sNode == NULL )
            {
                aTemplate->rows[aNode->table].execute[aNode->column]
                    = mtfSumKeepExecuteDoubleFast;
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
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // SumKeep 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     &mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfKeepOrderData * ),
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepInitializeDouble( mtcNode     * aNode,
                                   mtcStack    *,
                                   SInt         ,
                                   void        *,
                                   mtcTemplate * aTemplate )
{
    const mtcColumn       * sColumn;
    iduMemory             * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo  * sFuncData;
    mtfKeepOrderData      * sKeepOrderData;
    mtdBinaryType         * sBinary;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row +
                      sColumn->column.offset) = 0;
    *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                       sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                                        sColumn[2].column.offset);
    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );
        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfSumKeepInitializeDouble::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfSumKeepInitializeDouble::alloc::sKeepOrderData",
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

IDE_RC mtfSumKeepAggregateDouble( mtcNode     * aNode,
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
                               sColumn[2].column.offset);
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

        if ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
             != MTD_DOUBLE_EXPONENT_MASK )
        {
            *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row +
                              sColumn->column.offset) =
                *(mtdDoubleType*)aStack[1].value;

            *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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
            if ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
                 != MTD_DOUBLE_EXPONENT_MASK )
            {
                *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row +
                                  sColumn->column.offset) =
                    *(mtdDoubleType*)aStack[1].value;
                *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
            }
            else
            {
                *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sColumn->column.offset) = 0;
                *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // mtdDouble.isNull() 를 호출하는 대신
            // 직접 null 검사를 한다.
            // aStack->value의 데이터 타입을 미리 알기 때문에
            // 직접 null 검사를 하는데 수행 속도를 위해서이다.
            if ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
                 != MTD_DOUBLE_EXPONENT_MASK )
            {
                *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row +
                                  sColumn->column.offset) +=
                    *(mtdDoubleType*)aStack[1].value;

                *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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

IDE_RC mtfSumKeepAggregateDoubleFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        *,
                                      mtcTemplate * aTemplate )
{
    mtcNode              * sNode;
    const mtcColumn      * sSumKeepColumn;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    mtdBinaryType        * sBinary;
    mtdCharType          * sOption;
    UInt                   sAction;
    SInt                   sRemain;
    mtcStack             * sStack;

    for ( sNode  = aNode->arguments, sStack = aStack + 1, sRemain = aRemain - 1;
          sNode != NULL;
          sNode  = sNode->next, sStack++, sRemain-- )
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

        sStack->column = & aTemplate->rows[sNode->table].columns[sNode->column];
        sStack->value  = (void*) mtc::value( sStack->column,
                                             aTemplate->rows[sNode->table].row,
                                             MTD_OFFSET_USE );
    }

    sSumKeepColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sSumKeepColumn[2].column.offset);
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

        if ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
             != MTD_DOUBLE_EXPONENT_MASK )
        {
            *(mtdDoubleType*) ((UChar*) aTemplate->rows[aNode->table].row +
                               sSumKeepColumn->column.offset) =
                *(mtdDoubleType*)aStack[1].value;

            *(mtdBooleanType*) ((UChar*) aTemplate->rows[aNode->table].row +
                                sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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
            if ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
                 != MTD_DOUBLE_EXPONENT_MASK )
            {
                *(mtdDoubleType*) ((UChar*) aTemplate->rows[aNode->table].row +
                                   sSumKeepColumn->column.offset) =
                    *(mtdDoubleType*)aStack[1].value;
                *(mtdBooleanType*) ((UChar*) aTemplate->rows[aNode->table].row +
                                    sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
            }
            else
            {
                *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sSumKeepColumn->column.offset) = 0;
                *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // mtdDouble.isNull() 를 호출하는 대신
            // 직접 null 검사를 한다.
            // aStack->value의 데이터 타입을 미리 알기 때문에
            // 직접 null 검사를 하는데 수행 속도를 위해서이다.
            if ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
                 != MTD_DOUBLE_EXPONENT_MASK )
            {
                *(mtdDoubleType*) ((UChar*) aTemplate->rows[aNode->table].row +
                                    sSumKeepColumn->column.offset) +=
                    *(mtdDoubleType*)aStack[1].value;

                *(mtdBooleanType*) ((UChar*) aTemplate->rows[aNode->table].row +
                                    sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepFinalizeDouble( mtcNode     * aNode,
                                 mtcStack    *,
                                 SInt,
                                 void        *,
                                 mtcTemplate * aTemplate )
{
    mtcColumn * sSumKeepColumn;
    void      * sValueTemp;

    sSumKeepColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aNode->table].row +
                             sSumKeepColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumKeepColumn,
                                 aTemplate->rows[aNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumKeepColumn->module->staticNull );

        mtdDouble.null( sSumKeepColumn,
                        sValueTemp );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumKeepCalculateDouble( mtcNode     * aNode,
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

/* ZONE: BIGINT */

IDE_RC mtfSumKeepInitializeBigint( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfSumKeepAggregateBigint( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfSumKeepAggregateBigintFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfSumKeepFinalizeBigint( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfSumKeepCalculateBigint( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

static const mtcExecute mtfSumKeepExecuteBigint = {
    mtfSumKeepInitializeBigint,
    mtfSumKeepAggregateBigint,
    mtf::calculateNA,
    mtfSumKeepFinalizeBigint,
    mtfSumKeepCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


// 최적의 SumKeep을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfSumKeepExecuteBigintFast = {
    mtfSumKeepInitializeBigint,
    mtfSumKeepAggregateBigintFast,
    mtf::calculateNA,
    mtfSumKeepFinalizeBigint,
    mtfSumKeepCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSumKeepEstimateBigint( mtcNode     * aNode,
                                 mtcTemplate * aTemplate,
                                 mtcStack    * aStack,
                                 SInt,
                                 mtcCallBack * aCallBack )
{
    const mtfModule        * sArgModule;
    mtcNode                * sNode;
    static const mtdModule * sModules[1] = {
        &mtdBigint
    };

    sNode = aNode->arguments->next;
    aNode->arguments->next = NULL;
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    aNode->arguments->next = sNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSumKeepExecuteBigint;

    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;

    // mtf::initializeTemplate에서 각 subModule에 대해
    // estimateBound를 호출하는데 이때에는 node에 module이
    // 안달려있기 때문에 NULL 체크를 해야 한다.
    if ( sArgModule != NULL )
    {
        // SumKeep(i1) 처럼 i1가 단일 컬럼이고 conversion되지 않는다면
        // 최적화된 execution을 달아준다.

        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if ( ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_FALSE ) &&
             ( idlOS::strncmp((SChar*)sArgModule->names->string,
                             (const SChar*)"COLUMN", 6 )
              == 0 ) &&
            ( aNode->arguments->conversion == NULL ) )
        {
            // sum(i1) keep ( dense_rank first order by i1,i2,i3 )
            // order by column 도 검사해줘야한다.
            for ( sNode = aNode->arguments->next->next;
                  sNode != NULL;
                  sNode = sNode->next )
            {
                if ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_VIEW_MASK )
                       == MTC_TUPLE_VIEW_TRUE ) ||
                     ( idlOS::strncmp((SChar*)sNode->module->names->string,
                                     (const SChar*)"COLUMN", 6 )
                       != 0 ) ||
                     ( sNode->conversion != NULL ) )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sNode == NULL )
            {
                aTemplate->rows[aNode->table].execute[aNode->column]
                    = mtfSumKeepExecuteBigintFast;
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
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // SumKeep 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     &mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfKeepOrderData * ),
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepInitializeBigint( mtcNode     * aNode,
                                   mtcStack    *,
                                   SInt         ,
                                   void        *,
                                   mtcTemplate * aTemplate )
{
    const mtcColumn       * sColumn;
    iduMemory             * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo  * sFuncData;
    mtfKeepOrderData      * sKeepOrderData;
    mtdBinaryType         * sBinary;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row +
                       sColumn->column.offset) = 0;
    *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                       sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                                        sColumn[2].column.offset);

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );
        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfSumKeepInitializeBigint::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfSumKeepInitializeBigint::alloc::sKeepOrderData",
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

IDE_RC mtfSumKeepAggregateBigint( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        *,
                                  mtcTemplate * aTemplate )
{
    const mtcColumn       * sColumn;
    mtfKeepOrderData      * sKeepOrderData;
    mtfFuncDataBasicInfo  * sFuncData;
    mtdBinaryType         * sBinary;
    mtdCharType           * sOption;
    UInt                    sAction;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[2].column.offset);
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

        if ( *(mtdBigintType*)aStack[1].value != MTD_BIGINT_NULL )
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                                sColumn->column.offset) =
                *(mtdBigintType*)aStack[1].value;
            *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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
            if ( *(mtdBigintType*)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sColumn->column.offset) =
                    *(mtdBigintType*)aStack[1].value;
                *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
            }
            else
            {
                *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row +
                                   sColumn->column.offset) = 0;
                *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // mtdBigint.isNull() 를 호출하는 대신
            // 직접 null 검사를 한다.
            // aStack->value의 데이터 타입을 미리 알기 때문에
            // 직접 null 검사를 하는데 수행 속도를 위해서이다.
            if ( *(mtdBigintType*)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sColumn->column.offset) +=
                    *(mtdBigintType*)aStack[1].value;
                *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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

IDE_RC mtfSumKeepAggregateBigintFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        *,
                                      mtcTemplate * aTemplate )
{
    mtcNode              * sNode;
    const mtcColumn      * sSumKeepColumn;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    mtdBinaryType        * sBinary;
    mtdCharType          * sOption;
    mtcStack             * sStack;
    UInt                   sAction;
    SInt                   sRemain;

    for ( sNode  = aNode->arguments, sStack = aStack + 1, sRemain = aRemain - 1;
          sNode != NULL;
          sNode  = sNode->next, sStack++, sRemain-- )
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

        sStack->column = & aTemplate->rows[sNode->table].columns[sNode->column];
        sStack->value  = (void*) mtc::value( sStack->column,
                                             aTemplate->rows[sNode->table].row,
                                             MTD_OFFSET_USE );
    }

    sSumKeepColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sSumKeepColumn[2].column.offset);
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

        if ( *(mtdBigintType*)aStack[1].value != MTD_BIGINT_NULL )
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                              sSumKeepColumn->column.offset) =
                *(mtdBigintType*)aStack[1].value;

            *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                              sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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
            if ( *(mtdBigintType*)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sSumKeepColumn->column.offset) =
                    *(mtdBigintType*)aStack[1].value;
                *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
            }
            else
            {
                *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row +
                                   sSumKeepColumn->column.offset) = 0;
                *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row +
                                   sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // mtdBigint.isNull() 를 호출하는 대신
            // 직접 null 검사를 한다.
            // aStack->value의 데이터 타입을 미리 알기 때문에
            // 직접 null 검사를 하는데 수행 속도를 위해서이다.
            if ( *(mtdBigintType*)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sSumKeepColumn->column.offset) +=
                    *(mtdBigintType*)aStack[1].value;
                *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row +
                                  sSumKeepColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumKeepFinalizeBigint( mtcNode     * aNode,
                                 mtcStack    *,
                                 SInt,
                                 void        *,
                                 mtcTemplate * aTemplate )
{
    mtcColumn * sSumKeepColumn;
    void      * sValueTemp;

    sSumKeepColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aNode->table].row +
                                sSumKeepColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumKeepColumn,
                                 aTemplate->rows[aNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumKeepColumn->module->staticNull );

        mtdBigint.null( sSumKeepColumn, sValueTemp );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumKeepCalculateBigint( mtcNode     * aNode,
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

