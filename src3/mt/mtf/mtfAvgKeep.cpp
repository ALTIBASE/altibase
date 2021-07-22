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
 * $Id: mtfAvgKeep.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfAvgKeep;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfAvgKeepFunctionName[1] = {
    { NULL, 8, (void*)"AVG_KEEP" }
};

static IDE_RC mtfAvgKeepInitialize( void );

static IDE_RC mtfAvgKeepFinalize( void );

static IDE_RC mtfAvgKeepEstimate( mtcNode     * aNode,
                                  mtcTemplate * aTemplate,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  mtcCallBack * aCallBack );

mtfModule mtfAvgKeep = {
    5 | MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfAvgKeepFunctionName,
    NULL,
    mtfAvgKeepInitialize,
    mtfAvgKeepFinalize,
    mtfAvgKeepEstimate
};

static IDE_RC mtfAvgKeepEstimateFloat( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

static IDE_RC mtfAvgKeepEstimateDouble( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallBack );

static IDE_RC mtfAvgKeepEstimateBigint( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfAvgKeepEstimates[3] = {
    { mtfAvgKeepEstimates+1, mtfAvgKeepEstimateDouble },
    { mtfAvgKeepEstimates+2, mtfAvgKeepEstimateBigint },
    { NULL, mtfAvgKeepEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfAvgKeepEstimatesHighPerformance[2] = {
    { mtfAvgKeepEstimatesHighPerformance+1, mtfAvgKeepEstimateDouble },
    { NULL, mtfAvgKeepEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;
static mtfSubModule** mtfTableHighPerformance = NULL;

IDE_RC mtfAvgKeepInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfAvgKeepEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfAvgKeepEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgKeepFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgKeepEstimate( mtcNode     * aNode,
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

IDE_RC mtfAvgKeepInitializeFloat( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepAggregateFloat( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepFinalizeFloat( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepCalculateFloat( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

static const mtcExecute mtfAvgKeepExecuteFloat = {
    mtfAvgKeepInitializeFloat,
    mtfAvgKeepAggregateFloat,
    mtf::calculateNA,
    mtfAvgKeepFinalizeFloat,
    mtfAvgKeepCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAvgKeepEstimateFloat( mtcNode     * aNode,
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
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfAvgKeepExecuteFloat;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdNull,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 4,
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

IDE_RC mtfAvgKeepInitializeFloat( mtcNode     * aNode,
                                  mtcStack    *,
                                  SInt,
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

    sFloat = (mtdNumericType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                 sColumn[0].column.offset );
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;
    sFloat = (mtdNumericType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                 sColumn[1].column.offset);
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;
    *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset ) = 0;

    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                                        sColumn[4].column.offset);

    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );
        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfAvgKeepInitializeFloat::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfAvgKeepInitializeFloat::alloc::sKeepOrderData",
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

IDE_RC mtfAvgKeepAggregateFloat( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        *,
                                 mtcTemplate * aTemplate )
{
    const mtcColumn      * sColumn;
    mtdNumericType       * sFloatSum;
    mtdNumericType       * sFloatArgument;
    UChar                  sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType       * sFloatSumClone;
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
                               sColumn[4].column.offset);
    sKeepOrderData = *((mtfKeepOrderData**)sBinary->mValue);

    sFloatSum = (mtdNumericType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn[1].column.offset );
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

        if ( aStack[1].column->module->isNull( aStack[1].column,
                                               aStack[1].value ) != ID_TRUE )
        {
            idlOS::memcpy( sFloatSum, sFloatArgument, sFloatArgument->length + 1 );

            *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row +
                                        sColumn[2].column.offset ) = 1;
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
                idlOS::memcpy( sFloatSum, sFloatArgument, sFloatArgument->length + 1 );

                *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row +
                                            sColumn[2].column.offset ) = 1;
            }
            else
            {
                sFloatSum->length       = 1;
                sFloatSum->signExponent = 0x80;
                *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row +
                                            sColumn[2].column.offset ) = 0;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            if ( aStack[1].column->module->isNull( aStack[1].column,
                                                   aStack[1].value ) != ID_TRUE )
            {
                sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
                idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );

                IDE_TEST( mtc::addFloat( sFloatSum,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sFloatSumClone,
                                         sFloatArgument )
                          != IDE_SUCCESS );

                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                             sColumn[2].column.offset ) += 1;
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

IDE_RC mtfAvgKeepFinalizeFloat( mtcNode     * aNode,
                                mtcStack    *,
                                SInt,
                                void        *,
                                mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    mtdBigintType     sCount;
    mtdNumericType  * sFloatResult;
    mtdNumericType  * sFloatSum;
    UChar             sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType  * sFloatCount;
    void            * sValue;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sCount = *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                 sColumn[2].column.offset );
    if ( sCount == 0 )
    {
        sValue = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn + 0,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValue );
    }
    else
    {
        sFloatResult = (mtdNumericType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                           sColumn[0].column.offset );
        sFloatSum = (mtdNumericType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                        sColumn[1].column.offset );

        sFloatCount = (mtdNumericType*)sFloatCountBuff;

        mtc::makeNumeric( sFloatCount, (SLong)sCount );

        IDE_TEST( mtc::divideFloat( sFloatResult,
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatSum,
                                    sFloatCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgKeepCalculateFloat( mtcNode     * aNode,
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

IDE_RC mtfAvgKeepInitializeDouble( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepAggregateDouble( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepAggregateDoubleFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepFinalizeDouble( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepCalculateDouble( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

static const mtcExecute mtfAvgKeepExecuteDouble = {
    mtfAvgKeepInitializeDouble,
    mtfAvgKeepAggregateDouble,
    mtf::calculateNA,
    mtfAvgKeepFinalizeDouble,
    mtfAvgKeepCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfAvgKeepExecuteDoubleFast = {
    mtfAvgKeepInitializeDouble,
    mtfAvgKeepAggregateDoubleFast,
    mtf::calculateNA,
    mtfAvgKeepFinalizeDouble,
    mtfAvgKeepCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAvgKeepEstimateDouble( mtcNode     * aNode,
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
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfAvgKeepExecuteDouble;

    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;
    if ( sArgModule != NULL )
    {
        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if ( ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
               == MTC_TUPLE_VIEW_FALSE ) &&
             ( idlOS::strncmp((SChar*)sArgModule->names->string,
                             (const SChar*)"COLUMN", 6 )
               == 0 ) &&
             ( aNode->arguments->conversion == NULL ) )
        {
            // avg(i1) keep ( dense_rank first order by i1,i2,i3 )
            // order by column 도 순수 철럼인지 확인해야한다.
            for ( sNode = aNode->arguments->next->next;
                  sNode != NULL;
                  sNode = sNode->next )
            {
                // BUG-19856
                // view 컬럼인 경우 최적화된 execution을 달지않는다.
                if ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_VIEW_MASK )
                       == MTC_TUPLE_VIEW_TRUE ) ||
                     ( idlOS::strncmp((SChar*)sNode->module->names->string,
                                     (const SChar*)"COLUMN", 6 )
                       != 0 ) ||
                    ( aNode->arguments->conversion != NULL ) )
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
                    = mtfAvgKeepExecuteDoubleFast;
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


    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdNull,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 4,
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

IDE_RC mtfAvgKeepInitializeDouble( mtcNode     * aNode,
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

    *(mtdDoubleType*)( (UChar*) aTemplate->rows[aNode->table].row +
                       sColumn[1].column.offset) = 0;
    *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row +
                       sColumn[2].column.offset) = 0;

    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[4].column.offset);

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );
        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfAvgKeepInitializeDouble::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfAvgKeepInitializeDouble::alloc::sKeepOrderData",
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

IDE_RC mtfAvgKeepAggregateDouble( mtcNode     * aNode,
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
                               sColumn[4].column.offset);
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
                    sColumn[1].column.offset) = *(mtdDoubleType*)aStack[1].value;
            *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                    sColumn[2].column.offset) = 1;
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
                        sColumn[1].column.offset) = *(mtdDoubleType*)aStack[1].value;
                *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 1;
            }
            else
            {
                *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) = 0;
                *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 0;
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
                *(mtdDoubleType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                      sColumn[1].column.offset) += *(mtdDoubleType*)aStack[1].value;
                *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                      sColumn[2].column.offset) += 1;
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

IDE_RC mtfAvgKeepAggregateDoubleFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        *,
                                      mtcTemplate * aTemplate )
{
    mtcNode              * sNode;
    const mtcColumn      * sColumn;
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

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[4].column.offset);
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
                                sColumn[1].column.offset) =
                *(mtdDoubleType*)aStack[1].value;

            *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                                sColumn[2].column.offset) = 1;
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
                        sColumn[1].column.offset) = *(mtdDoubleType*)aStack[1].value;
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 1;
            }
            else
            {
                *(mtdDoubleType*) ((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) = 0;
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 0;
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
                *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) += *(mtdDoubleType*)aStack[1].value;
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) += 1;
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

IDE_RC mtfAvgKeepFinalizeDouble( mtcNode     * aNode,
                                 mtcStack    *,
                                 SInt,
                                 void        *,
                                 mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    mtdBigintType     sCount;
    void            * sValue;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sCount = *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn[2].column.offset);
    if ( sCount == 0 )
    {
        sValue = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn + 0,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValue );
    }
    else
    {
        *(mtdDoubleType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                            sColumn[0].column.offset ) =
        *(mtdDoubleType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                            sColumn[1].column.offset ) / sCount;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfAvgKeepCalculateDouble( mtcNode     * aNode,
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

IDE_RC mtfAvgKeepInitializeBigint( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepAggregateBigint( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepAggregateBigintFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepFinalizeBigint( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC mtfAvgKeepCalculateBigint( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

static const mtcExecute mtfAvgKeepExecuteBigint = {
    mtfAvgKeepInitializeBigint,
    mtfAvgKeepAggregateBigint,
    mtf::calculateNA,
    mtfAvgKeepFinalizeBigint,
    mtfAvgKeepCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfAvgKeepExecuteBigintFast = {
    mtfAvgKeepInitializeBigint,
    mtfAvgKeepAggregateBigintFast,
    mtf::calculateNA,
    mtfAvgKeepFinalizeBigint,
    mtfAvgKeepCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAvgKeepEstimateBigint( mtcNode     * aNode,
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
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfAvgKeepExecuteBigint;

    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;
    if ( sArgModule != NULL )
    {
        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if ( ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
               == MTC_TUPLE_VIEW_FALSE ) &&
             ( idlOS::strncmp((SChar*)sArgModule->names->string,
                             (const SChar*)"COLUMN", 6 )
              == 0 ) &&
            ( aNode->arguments->conversion == NULL ) )
        {
            // avg(i1) keep ( dense_rank first order by i1,i2,i3 )
            // order by column 도 순수 철럼인지 확인해야한다.
            for ( sNode = aNode->arguments->next->next;
                  sNode != NULL;
                  sNode = sNode->next )
            {
                // BUG-19856
                // view 컬럼인 경우 최적화된 execution을 달지않는다.
                if ( ( ( aTemplate->rows[sNode->table].lflag & MTC_TUPLE_VIEW_MASK )
                       == MTC_TUPLE_VIEW_TRUE ) ||
                     ( idlOS::strncmp((SChar*)sNode->module->names->string,
                                     (const SChar*)"COLUMN", 6 )
                       != 0 ) ||
                    ( aNode->arguments->conversion != NULL ) )
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
                    = mtfAvgKeepExecuteBigintFast;
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

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 4,
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

IDE_RC mtfAvgKeepInitializeBigint( mtcNode     * aNode,
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

    *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) = 0;
    *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 0;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[4].column.offset);

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );
        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfAvgKeepInitializeBigint::alloc::sFuncData",
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

    IDU_FIT_POINT_RAISE( "mtfAvgKeepInitializeBigint::alloc::sKeepOrderData",
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

IDE_RC mtfAvgKeepAggregateBigint( mtcNode     * aNode,
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
    sBinary = (mtdBinaryType*)((UChar *)aTemplate->rows[aNode->table].row +
                               sColumn[4].column.offset);
    sKeepOrderData = *((mtfKeepOrderData **)sBinary->mValue);

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

        if ( *(mtdBigintType *)aStack[1].value != MTD_BIGINT_NULL )
        {
            *(mtdBigintType *)((UChar *)aTemplate->rows[aNode->table].row +
                            sColumn[1].column.offset) = *(mtdBigintType*)aStack[1].value;
            *(mtdBigintType *)( (UChar*) aTemplate->rows[aNode->table].row +
                            sColumn[2].column.offset) = 1;
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
            if ( *(mtdBigintType *)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType *)((UChar *)aTemplate->rows[aNode->table].row +
                                sColumn[1].column.offset) = *(mtdBigintType*)aStack[1].value;
                *(mtdBigintType *)( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn[2].column.offset) = 1;
            }
            else
            {
                *(mtdBigintType *)((UChar *)aTemplate->rows[aNode->table].row +
                                sColumn[1].column.offset) = 0;
                *(mtdBigintType *)( (UChar*) aTemplate->rows[aNode->table].row +
                                sColumn[2].column.offset) = 0;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // mtdBigint.isNull() 를 호출하는 대신
            // 직접 null 검사를 한다.
            // aStack->value의 데이터 타입을 미리 알기 때문에
            // 직접 null 검사를 하는데 수행 속도를 위해서이다.
            if ( *(mtdBigintType *)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType *)((UChar *)aTemplate->rows[aNode->table].row +
                            sColumn[1].column.offset) += *(mtdBigintType*)aStack[1].value;
                *(mtdBigintType *)( (UChar*) aTemplate->rows[aNode->table].row +
                            sColumn[2].column.offset) += 1;
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

IDE_RC mtfAvgKeepAggregateBigintFast( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        *,
                                      mtcTemplate * aTemplate )
{
    mtcNode              * sNode;
    const mtcColumn      * sColumn;
    mtfKeepOrderData     * sKeepOrderData;
    mtfFuncDataBasicInfo * sFuncData;
    mtdBinaryType        * sBinary;
    mtcStack             * sStack;
    mtdCharType          * sOption;
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

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                               sColumn[4].column.offset);
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

        if ( *(mtdBigintType *)aStack[1].value != MTD_BIGINT_NULL )
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                    sColumn[1].column.offset) = *(mtdBigintType*)aStack[1].value;
            *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                    sColumn[2].column.offset) = 1;
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
            if ( *(mtdBigintType *)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) = *(mtdBigintType*)aStack[1].value;
                *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 1;
            }
            else
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) = 0;
                *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) = 0;
            }
        }
        else if ( sAction == MTF_KEEP_ACTION_AGGR )
        {
            // mtdBigint.isNull() 를 호출하는 대신
            // 직접 null 검사를 한다.
            // aStack->value의 데이터 타입을 미리 알기 때문에
            // 직접 null 검사를 하는데 수행 속도를 위해서이다.
            if ( *(mtdBigintType *)aStack[1].value != MTD_BIGINT_NULL )
            {
                *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                        sColumn[1].column.offset) += *(mtdBigintType*)aStack[1].value;
                *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                        sColumn[2].column.offset) += 1;
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

IDE_RC mtfAvgKeepFinalizeBigint( mtcNode     * aNode,
                                 mtcStack    *,
                                 SInt,
                                 void        *,
                                 mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    mtdBigintType     sCount;
    mtdBigintType     sSum;
    void            * sValue;

    mtdNumericType  * sFloatSum;
    mtdNumericType  * sFloatCount;
    UChar             sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar             sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sCount = *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn[2].column.offset);
    if ( sCount == 0 )
    {
        sValue = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn + 0,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValue );
    }
    else
    {
        sSum = *(mtdBigintType*) ( (UChar*) aTemplate->rows[aNode->table].row +
                                    sColumn[1].column.offset);

        sFloatSum = (mtdNumericType*)sFloatSumBuff;
        mtc::makeNumeric( sFloatSum, (SLong)sSum );

        sFloatCount = (mtdNumericType*)sFloatCountBuff;
        mtc::makeNumeric( sFloatCount, (SLong)sCount );

        IDE_TEST( mtc::divideFloat( (mtdNumericType*)
                                    ( (UChar*) aTemplate->rows[aNode->table].row +
                                      sColumn->column.offset),
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatSum,
                                    sFloatCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgKeepCalculateBigint( mtcNode     * aNode,
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

