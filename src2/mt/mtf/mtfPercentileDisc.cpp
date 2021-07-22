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
 * $Id: mtfPercentileDisc.cpp 85458 2019-05-16 07:13:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfPercentileDisc;

extern mtdModule mtdFloat;
extern mtdModule mtdNumeric;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdDate;
extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

#define MTF_PERCENTILE_DISC_BUFFER_MAX     (80000)

typedef struct mtfPercentileDiscFuncDataBuffer
{
    UChar                            * data;
    ULong                              idx;
    mtfPercentileDiscFuncDataBuffer  * next;
} mtfPercentileDiscFuncDataBuffer;

typedef struct mtfPercentileDiscFuncData
{
    mtfPercentileDiscFuncDataBuffer  * list;       // chunk data
    UInt                               rowSize;    // float, double, bigint, date size
    idBool                             orderDesc;
    ULong                              totalCount;
    UChar                            * totalData;  // all data
} mtfPercentileDiscFuncData;

static IDE_RC makePercentileDiscFuncData( mtcTemplate                * aTemplate,
                                          mtfFuncDataBasicInfo       * aFuncDataInfo,
                                          mtcNode                    * aNode,
                                          mtfPercentileDiscFuncData ** aPercentileDiscFuncData );

static IDE_RC allocPercentileDiscFuncDataBuffer( mtcTemplate               * aTemplate,
                                                 mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                 mtcStack                  * aStack,
                                                 mtfPercentileDiscFuncData * aPercentileDiscFuncData );

static IDE_RC copyToPercentileDiscFuncDataBuffer( mtcTemplate               * aTemplate,
                                                  mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                  mtcStack                  * aStack,
                                                  mtfPercentileDiscFuncData * aPercentileDiscFuncData );

static IDE_RC makeTotalDataPercentileDiscFuncData( mtcTemplate               * aTemplate,
                                                   mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                   mtfPercentileDiscFuncData * aPercentileDiscFuncData );

static IDE_RC sortPercentileDiscFuncDataFloat( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                               mtfPercentileDiscFuncData * aPercentileDiscFuncData );

static IDE_RC sortPercentileDiscFuncDataDouble( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                mtfPercentileDiscFuncData * aPercentileDiscFuncData );

static IDE_RC sortPercentileDiscFuncDataBigint( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                mtfPercentileDiscFuncData * aPercentileDiscFuncData );

/* BUG-43821 DATE Type Support */
static IDE_RC sortPercentileDiscFuncDataDate( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                              mtfPercentileDiscFuncData * aPercentileDiscFuncData );

static mtcName mtfPercentileDiscFunctionName[1] = {
    { NULL, 15, (void*)"PERCENTILE_DISC" }
};

static IDE_RC mtfPercentileDiscInitialize( void );

static IDE_RC mtfPercentileDiscFinalize( void );

static IDE_RC mtfPercentileDiscEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfPercentileDisc = {
    3 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfPercentileDiscFunctionName,
    NULL,
    mtfPercentileDiscInitialize,
    mtfPercentileDiscFinalize,
    mtfPercentileDiscEstimate
};

static IDE_RC mtfPercentileDiscEstimateFloat( mtcNode*     aNode,
                                              mtcTemplate* aTemplate,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              mtcCallBack* aCallBack );

static IDE_RC mtfPercentileDiscEstimateDouble( mtcNode*     aNode,
                                               mtcTemplate* aTemplate,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               mtcCallBack* aCallBack );

static IDE_RC mtfPercentileDiscEstimateBigint( mtcNode*     aNode,
                                               mtcTemplate* aTemplate,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               mtcCallBack* aCallBack );

/* BUG-43821 DATE Type Support */
static IDE_RC mtfPercentileDiscEstimateDate( mtcNode     * aNode,
                                             mtcTemplate * aTemplate,
                                             mtcStack    * aStack,
                                             SInt          aRemain,
                                             mtcCallBack * aCallBack );

IDE_RC mtfPercentileDiscInitialize( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfPercentileDiscAggregate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfPercentileDiscMerge( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC mtfPercentileDiscCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfPercentileDiscEstimates[4] = {
    { mtfPercentileDiscEstimates+1, mtfPercentileDiscEstimateDouble },
    { mtfPercentileDiscEstimates+2, mtfPercentileDiscEstimateBigint },
    { mtfPercentileDiscEstimates+3, mtfPercentileDiscEstimateDate },    /* BUG-43821 DATE Type Support */
    { NULL,                         mtfPercentileDiscEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfPercentileDiscEstimatesHighPerformance[3] = {
    { mtfPercentileDiscEstimatesHighPerformance+1, mtfPercentileDiscEstimateDouble },
    { mtfPercentileDiscEstimatesHighPerformance+2, mtfPercentileDiscEstimateBigint },
    { NULL,                                        mtfPercentileDiscEstimateDate }  /* BUG-43821 DATE Type Support */
};

static mtfSubModule** mtfTable = NULL;
static mtfSubModule** mtfTableHighPerformance = NULL;

IDE_RC mtfPercentileDiscInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfPercentileDiscEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfPercentileDiscEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfPercentileDiscFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfPercentileDiscEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack )
{
    const mtfSubModule  * sSubModule;
    mtfSubModule       ** sTable;

    // within group by가 있어야 함
    IDE_TEST_RAISE( aNode->funcArguments == NULL,
                    ERR_WITHIN_GROUP_MISSING_WITHIN_GROUP );
    
    // within group by에 하나만 가능
    IDE_TEST_RAISE( aNode->funcArguments->next != NULL,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    // stack[2]가 반드시 필요하다.
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) !=2,
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
                                     aStack[2].column->module->no )
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
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION( ERR_WITHIN_GROUP_MISSING_WITHIN_GROUP )
    {   
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MISSING_WITHIN_GROUP ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfPercentileDiscInitialize( mtcNode*     aNode,
                                    mtcStack*,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
    iduMemory                  * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo       * sFuncDataInfo;
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData;
    const mtcColumn            * sColumn;
    mtdBinaryType              * sBinary;
    UChar                      * sRow;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );

        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfPercentileDiscInitialize::alloc::sFuncDataInfo",
                             ERR_MEMORY_ALLOCATION );
        IDE_TEST_RAISE( sMemoryMgr->alloc( ID_SIZEOF(mtfFuncDataBasicInfo),
                                           (void**)&sFuncDataInfo )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
        
        // function data init
        IDE_TEST( mtf::initializeFuncDataBasicInfo( sFuncDataInfo,
                                                    sMemoryMgr )
                  != IDE_SUCCESS );

        // 등록
        aTemplate->funcData[aNode->info] = sFuncDataInfo;

        /* BUG-46922 */
        sMemoryMgr = sFuncDataInfo->memoryMgr;

        IDE_TEST( sMemoryMgr->getStatus( &( sFuncDataInfo->mMemStatus ) )
                  != IDE_SUCCESS );
    }
    else
    {
        sFuncDataInfo = aTemplate->funcData[aNode->info];

        /* BUG-46922 */
        sMemoryMgr = sFuncDataInfo->memoryMgr;

        IDE_TEST( sMemoryMgr->setStatus( &( sFuncDataInfo->mMemStatus ) )
                  != IDE_SUCCESS );
    }

    // make percentileDisc function data
    IDE_TEST( makePercentileDiscFuncData( aTemplate,
                                          sFuncDataInfo,
                                          aNode,
                                          & sPercentileDiscFuncData )
              != IDE_SUCCESS );

    // set percentile function data
    sBinary->mLength = ID_SIZEOF(mtfPercentileDiscFuncData*);
    *((mtfPercentileDiscFuncData**)sBinary->mValue) = sPercentileDiscFuncData;

    // percentile option
    *(mtdDoubleType*)( sRow + sColumn[2].column.offset ) = 0;
    
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

IDE_RC mtfPercentileDiscAggregate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    mtfFuncDataBasicInfo       * sFuncDataInfo;
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData;
    const mtcColumn            * sColumn;
    mtdBinaryType              * sBinary;
    
    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if ( aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sBinary = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row +
                                   sColumn[1].column.offset);
    
        sFuncDataInfo = aTemplate->funcData[aNode->info];
        sPercentileDiscFuncData = *((mtfPercentileDiscFuncData**)sBinary->mValue);

        // percentile option
        if ( sPercentileDiscFuncData->totalCount == 0 )
        {
            *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row +
                              sColumn[2].column.offset) = *(mtdDoubleType*)aStack[1].value;
            
            IDE_TEST_RAISE ( ( *(mtdDoubleType*)aStack[1].value > 1 ) ||
                             ( *(mtdDoubleType*)aStack[1].value < 0 ),
                             ERR_INVALID_PERCENTILE_VALUE );
        }
        else
        {
            // Nothing to do.
        }

        // copy function data
        IDE_TEST( copyToPercentileDiscFuncDataBuffer( aTemplate,
                                                      sFuncDataInfo,
                                                      aStack,
                                                      sPercentileDiscFuncData )
                  != IDE_SUCCESS );
    }    
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION( ERR_INVALID_PERCENTILE_VALUE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_PERCENTILE_VALUE ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfPercentileDiscMerge( mtcNode     * aNode,
                               mtcStack    * ,
                               SInt          ,
                               void        * aInfo,
                               mtcTemplate * aTemplate )
{
    const mtcColumn                 * sColumn;
    UChar                           * sDstRow;
    UChar                           * sSrcRow;
    mtdBinaryType                   * sSrcValue;
    mtfPercentileDiscFuncData       * sSrcPercentileDiscFuncData;
    mtdBinaryType                   * sDstValue;
    mtfPercentileDiscFuncData       * sDstPercentileDiscFuncData;
    mtfPercentileDiscFuncDataBuffer * sPercentileDiscFuncDataBuffer;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;

    // src percentile function data.
    sSrcValue = (mtdBinaryType*)( sSrcRow + sColumn[1].column.offset );
    sSrcPercentileDiscFuncData = *((mtfPercentileDiscFuncData**)sSrcValue->mValue);

    // dst percentile function data.
    sDstValue = (mtdBinaryType*)( sDstRow + sColumn[1].column.offset );
    sDstPercentileDiscFuncData = *((mtfPercentileDiscFuncData**)sDstValue->mValue);

    if ( sSrcPercentileDiscFuncData->list == NULL )
    {
        // Nothing To Do
    }
    else
    {
        if ( sDstPercentileDiscFuncData->list == NULL )
        {
            *((mtfPercentileDiscFuncData**)sDstValue->mValue) =
                sSrcPercentileDiscFuncData;
        }
        else
        {
            for ( sPercentileDiscFuncDataBuffer = sDstPercentileDiscFuncData->list;
                  sPercentileDiscFuncDataBuffer->next != NULL;
                  sPercentileDiscFuncDataBuffer = sPercentileDiscFuncDataBuffer->next );
            
            sPercentileDiscFuncDataBuffer->next =
                sSrcPercentileDiscFuncData->list;

            sDstPercentileDiscFuncData->totalCount +=
                sSrcPercentileDiscFuncData->totalCount;
        }
    }

    // percentile option
    *(mtdDoubleType *)(sDstRow + sColumn[2].column.offset) =
        *(mtdDoubleType *)(sSrcRow + sColumn[2].column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfPercentileDiscCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row +
                              aStack->column->column.offset );
    
    return IDE_SUCCESS;
}

/* ZONE: FLOAT */

IDE_RC mtfPercentileDiscFinalizeFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute mtfPercentileDiscExecuteFloat = {
    mtfPercentileDiscInitialize,
    mtfPercentileDiscAggregate,
    mtfPercentileDiscMerge,
    mtfPercentileDiscFinalizeFloat,
    mtfPercentileDiscCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileDiscEstimateFloat( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt,
                                       mtcCallBack* aCallBack )
{
    const mtdModule * sModules[2] = {
        &mtdDouble,
        NULL
    };
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // submodule 초기화시
    if ( ( aCallBack->flag & MTC_ESTIMATE_INITIALIZE_MASK )
         == MTC_ESTIMATE_INITIALIZE_TRUE )
    {
        mtc::makeFloatConversionModule( aStack + 1, sModules + 1 );
        
        // initializeTemplate시에는 인자가 1개로 estimate되고
        // percentile_disc 함수로 보면 2번째 인자이다.
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules + 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                        ( aStack[2].column->module == &mtdList ) ,
                        ERR_CONVERSION_NOT_APPLICABLE );
        
        mtc::makeFloatConversionModule( aStack + 2, sModules + 1 );
    
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileDiscExecuteFloat;
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // percentile function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfPercentileDiscFuncData * ),
                                     0 )
              != IDE_SUCCESS );

    // percentile option
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfPercentileDiscFinalizeFloat( mtcNode      * aNode,
                                       mtcStack     * ,
                                       SInt           ,
                                       void         * ,
                                       mtcTemplate  * aTemplate )
{
    const mtcColumn            * sColumn;
    mtdBinaryType              * sBinary;
    UChar                      * sRow;
    mtdDoubleType                sFirstValue;
    mtdNumericType             * sResult;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo;
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData;
    SDouble                      sRowNum;
    SLong                        sCeilRowNum;
    SLong                        sFloorRowNum;
    mtdNumericType             * sFloatArgument1;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileDiscFuncData = *((mtfPercentileDiscFuncData**)sBinary->mValue);

    sResult = (mtdNumericType*)(sRow + sColumn[0].column.offset);
    
    //------------------------------------------
    // sort
    //------------------------------------------
    
    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileDiscFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileDiscFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileDiscFuncDataFloat( sFuncDataBasicInfo,
                                               sPercentileDiscFuncData )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // 연산 수행    
    //------------------------------------------

    if ( sPercentileDiscFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType*)(sRow + sColumn[2].column.offset));

        // RN( 중간 row위치) 을 구한다.
        sRowNum = sFirstValue * sPercentileDiscFuncData->totalCount;

        sFloorRowNum  = idlOS::floor( sRowNum );
                
        if ( sFloorRowNum == 0 )
        {
            sCeilRowNum = 0;
        }
        else
        {
            // ceil row num
            sCeilRowNum  = idlOS::ceil( sRowNum );
            sCeilRowNum--;
        }

        sFloatArgument1 = (mtdNumericType*)
            ( sPercentileDiscFuncData->totalData +
              sPercentileDiscFuncData->rowSize * sCeilRowNum );

        idlOS::memcpy( sResult,
                       sFloatArgument1,
                       ID_SIZEOF(UChar) + sFloatArgument1->length );
    }
    else
    {
        // set null
        sResult->length = 0;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

IDE_RC mtfPercentileDiscFinalizeDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfPercentileDiscExecuteDouble = {
    mtfPercentileDiscInitialize,
    mtfPercentileDiscAggregate,
    mtfPercentileDiscMerge,
    mtfPercentileDiscFinalizeDouble,
    mtfPercentileDiscCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileDiscEstimateDouble( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt,
                                        mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdDouble,
        &mtdDouble
    };

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    // submodule 초기화시
    if ( ( aCallBack->flag & MTC_ESTIMATE_INITIALIZE_MASK )
         == MTC_ESTIMATE_INITIALIZE_TRUE )
    {
        // initializeTemplate시에는 인자가 1개로 estimate되고
        // percentile_disc 함수로 보면 2번째 인자이다.
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules + 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                        ( aStack[2].column->module == &mtdList ) ,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileDiscExecuteDouble;
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // percentile function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfPercentileDiscFuncData * ),
                                     0 )
              != IDE_SUCCESS );

    // percentile option
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

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

IDE_RC mtfPercentileDiscFinalizeDouble( mtcNode     * aNode,
                                        mtcStack    * ,
                                        SInt          ,
                                        void        * ,
                                        mtcTemplate * aTemplate )
{
    const mtcColumn            * sColumn;
    mtdBinaryType              * sBinary;
    UChar                      * sRow;
    mtdDoubleType                sFirstValue;
    mtdDoubleType              * sResult;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo;
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData;
    SDouble                      sRowNum;
    SLong                        sCeilRowNum;
    SLong                        sFloorRowNum;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileDiscFuncData = *((mtfPercentileDiscFuncData**)sBinary->mValue);

    sResult = (mtdDoubleType*)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------
    
    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileDiscFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileDiscFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileDiscFuncDataDouble( sFuncDataBasicInfo,
                                                sPercentileDiscFuncData )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // 연산 수행
    //------------------------------------------
    
    if ( sPercentileDiscFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType*)(sRow + sColumn[2].column.offset));        

        // RN( 중간 row위치) 을 구한다.
        sRowNum = sFirstValue * sPercentileDiscFuncData->totalCount;

        sFloorRowNum  = idlOS::floor( sRowNum );
        
        if ( sFloorRowNum == 0 )
        {
            sCeilRowNum = 0;
        }
        else
        {
            // ceil row num
            sCeilRowNum  = idlOS::ceil( sRowNum );
            sCeilRowNum--;
        }

        *sResult = *(mtdDoubleType*)
            ( sPercentileDiscFuncData->totalData +
              sPercentileDiscFuncData->rowSize * sCeilRowNum );
    }
    else
    {
        // set null
        *sResult = *(mtdDoubleType*)mtdDouble.staticNull;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: BIGINT */

IDE_RC mtfPercentileDiscFinalizeBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfPercentileDiscExecuteBigint = {
    mtfPercentileDiscInitialize,
    mtfPercentileDiscAggregate,
    mtfPercentileDiscMerge,
    mtfPercentileDiscFinalizeBigint,
    mtfPercentileDiscCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileDiscEstimateBigint( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt,
                                        mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdDouble,
        &mtdBigint
    };
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    // submodule 초기화시
    if ( ( aCallBack->flag & MTC_ESTIMATE_INITIALIZE_MASK )
         == MTC_ESTIMATE_INITIALIZE_TRUE )
    {
        // initializeTemplate시에는 인자가 1개로 estimate되고
        // percentile_disc 함수로 보면 2번째 인자이다.
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules + 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                        ( aStack[2].column->module == &mtdList ) ,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileDiscExecuteBigint;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // percentile function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfPercentileDiscFuncData * ),
                                     0 )
              != IDE_SUCCESS );

    // percentile option
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
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

IDE_RC mtfPercentileDiscFinalizeBigint( mtcNode     * aNode,
                                        mtcStack    * ,
                                        SInt          ,
                                        void        * ,
                                        mtcTemplate * aTemplate )
{
    const mtcColumn            * sColumn;
    mtdBinaryType              * sBinary;
    UChar                      * sRow;
    mtdDoubleType                sFirstValue;
    mtdBigintType              * sResult;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo;
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData;
    SDouble                      sRowNum;
    SLong                        sCeilRowNum;
    SLong                        sFloorRowNum;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileDiscFuncData = *((mtfPercentileDiscFuncData**)sBinary->mValue);

    sResult = (mtdBigintType*)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------
    
    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileDiscFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileDiscFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileDiscFuncDataBigint( sFuncDataBasicInfo,
                                                sPercentileDiscFuncData )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // 연산 수행
    //------------------------------------------
    
    if ( sPercentileDiscFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType*)(sRow + sColumn[2].column.offset));
        
        // RN( 중간 row위치) 을 구한다.
        sRowNum = sFirstValue * sPercentileDiscFuncData->totalCount;

        sFloorRowNum  = idlOS::floor( sRowNum );
    
        if ( sFloorRowNum == 0 )
        {
            sCeilRowNum = 0;
        }
        else
        {
            // ceil row num
            sCeilRowNum  = idlOS::ceil( sRowNum );
            sCeilRowNum--;
        }
        
        *sResult = *(mtdBigintType*)
            ( sPercentileDiscFuncData->totalData +
              sPercentileDiscFuncData->rowSize * sCeilRowNum );

    }
    else
    {
        // set null
        *sResult = *(mtdBigintType*)mtdBigint.staticNull;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* BUG-43821 DATE Type Support */
/* ZONE: DATE */

IDE_RC mtfPercentileDiscFinalizeDate( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

static const mtcExecute mtfPercentileDiscExecuteDate = {
    mtfPercentileDiscInitialize,
    mtfPercentileDiscAggregate,
    mtfPercentileDiscMerge,
    mtfPercentileDiscFinalizeDate,
    mtfPercentileDiscCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileDiscEstimateDate( mtcNode     * aNode,
                                      mtcTemplate * aTemplate,
                                      mtcStack    * aStack,
                                      SInt,
                                      mtcCallBack * aCallBack )
{
    static const mtdModule * sModules[2] = {
        &mtdDouble,
        &mtdDate
    };

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // submodule 초기화시
    if ( ( aCallBack->flag & MTC_ESTIMATE_INITIALIZE_MASK )
                          == MTC_ESTIMATE_INITIALIZE_TRUE )
    {
        // initializeTemplate시에는 인자가 1개로 estimate되고
        // percentile_disc 함수로 보면 2번째 인자이다.
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules + 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                        ( aStack[2].column->module == &mtdList ) ,
                        ERR_CONVERSION_NOT_APPLICABLE );

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileDiscExecuteDate;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // percentile function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfPercentileDiscFuncData * ),
                                     0 )
              != IDE_SUCCESS );

    // percentile option
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

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

IDE_RC mtfPercentileDiscFinalizeDate( mtcNode      * aNode,
                                      mtcStack     * ,
                                      SInt           ,
                                      void         * ,
                                      mtcTemplate  * aTemplate )
{
    const mtcColumn            * sColumn                 = NULL;
    mtdBinaryType              * sBinary                 = NULL;
    UChar                      * sRow                    = NULL;
    mtdDoubleType                sFirstValue             = 0.0;
    mtdDateType                * sResult                 = NULL;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo      = NULL;
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData = NULL;
    SDouble                      sRowNum                 = 0.0;
    SLong                        sCeilRowNum             = ID_LONG(0);
    SLong                        sFloorRowNum            = ID_LONG(0);
    mtdDateType                * sDateArgument           = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileDiscFuncData = *((mtfPercentileDiscFuncData **)sBinary->mValue);

    sResult = (mtdDateType *)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------

    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileDiscFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileDiscFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileDiscFuncDataDate( sFuncDataBasicInfo,
                                              sPercentileDiscFuncData )
              != IDE_SUCCESS );

    //------------------------------------------
    // 연산 수행
    //------------------------------------------

    if ( sPercentileDiscFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType *)(sRow + sColumn[2].column.offset));

        // RN( 중간 row위치) 을 구한다.
        sRowNum = sFirstValue * sPercentileDiscFuncData->totalCount;

        sFloorRowNum = idlOS::floor( sRowNum );

        if ( sFloorRowNum == 0 )
        {
            sCeilRowNum = 0;
        }
        else
        {
            // ceil row num
            sCeilRowNum = idlOS::ceil( sRowNum );
            sCeilRowNum--;
        }

        sDateArgument = (mtdDateType *)( sPercentileDiscFuncData->totalData +
                                         sPercentileDiscFuncData->rowSize * sCeilRowNum );

        idlOS::memcpy( sResult, sDateArgument, ID_SIZEOF(mtdDateType) );
    }
    else
    {
        // set null
        *sResult = *(mtdDateType *)mtdDate.staticNull;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC makePercentileDiscFuncData( mtcTemplate                * aTemplate,
                                   mtfFuncDataBasicInfo       * aFuncDataInfo,
                                   mtcNode                    * aNode,
                                   mtfPercentileDiscFuncData ** aPercentileDiscFuncData )
{
    mtfPercentileDiscFuncData  * sPercentileDiscFuncData;

    IDE_TEST_RAISE( aNode->funcArguments == NULL, ERR_INVALID_FUNCTION_ARGUMENT );

    /* BUG-46922 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ID_SIZEOF( mtfPercentileDiscFuncData ) )
              != IDE_SUCCESS );

    // function data
    IDU_FIT_POINT_RAISE( "makePercentileDiscFuncData::cralloc::sPercentileDiscFuncData",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->cralloc(
                        ID_SIZEOF(mtfPercentileDiscFuncData),
                        (void**)&sPercentileDiscFuncData )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
        
    if ( ( aNode->funcArguments->lflag & MTC_NODE_WITHIN_GROUP_ORDER_MASK )
         == MTC_NODE_WITHIN_GROUP_ORDER_DESC )
    {
        sPercentileDiscFuncData->orderDesc = ID_TRUE;
    }
    else
    {
        sPercentileDiscFuncData->orderDesc = ID_FALSE;
    }
    
    // return
    *aPercentileDiscFuncData = sPercentileDiscFuncData;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC allocPercentileDiscFuncDataBuffer( mtcTemplate               * aTemplate,
                                          mtfFuncDataBasicInfo      * aFuncDataInfo,
                                          mtcStack                  * aStack,
                                          mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    mtfPercentileDiscFuncDataBuffer  * sBuffer;
    UInt                               sRowSize;

    if ( aPercentileDiscFuncData->rowSize == 0 )
    {
        if ( ( aStack[2].column->module == &mtdFloat ) ||
             ( aStack[2].column->module == &mtdNumeric ) )
        {
            sRowSize = aStack[2].column->column.size;
        }
        else if ( aStack[2].column->module == &mtdDouble )
        {
            sRowSize = ID_SIZEOF(mtdDoubleType);
        }
        else if ( aStack[2].column->module == &mtdBigint )
        {
            sRowSize = ID_SIZEOF(mtdBigintType);
        }
        /* BUG-43821 DATE Type Support */
        else if ( aStack[2].column->module == &mtdDate )
        {
            sRowSize = ID_SIZEOF(mtdDateType);
        }
        else
        {
            IDE_RAISE( ERR_ARGUMENT_TYPE );
        }

        aPercentileDiscFuncData->rowSize = sRowSize;
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-46922 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ID_SIZEOF( mtfPercentileDiscFuncDataBuffer ) +
                                               aPercentileDiscFuncData->rowSize *
                                               MTF_PERCENTILE_DISC_BUFFER_MAX )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "allocPercentileDiscFuncDataBuffer::alloc::sBuffer",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc(
                        ID_SIZEOF(mtfPercentileDiscFuncDataBuffer) +
                        aPercentileDiscFuncData->rowSize *
                        MTF_PERCENTILE_DISC_BUFFER_MAX,
                        (void**)&sBuffer )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
        
    sBuffer->data = (UChar*)sBuffer + ID_SIZEOF(mtfPercentileDiscFuncDataBuffer);
    sBuffer->idx = 0;
    sBuffer->next = aPercentileDiscFuncData->list;

    // link
    aPercentileDiscFuncData->list = sBuffer;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_TYPE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "allocPercentileDiscFuncDataBuffer",
                                  "invalid arguemnt type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC copyToPercentileDiscFuncDataBuffer( mtcTemplate               * aTemplate,
                                           mtfFuncDataBasicInfo      * aFuncDataInfo,
                                           mtcStack                  * aStack,
                                           mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    void   * sValue;
    idBool   sAlloc = ID_FALSE;
    
    if ( aPercentileDiscFuncData->list == NULL )
    {
        sAlloc = ID_TRUE;
    }
    else
    {
        if ( aPercentileDiscFuncData->list->idx == MTF_PERCENTILE_DISC_BUFFER_MAX )
        {
            sAlloc = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sAlloc == ID_TRUE )
    {
        IDE_TEST( allocPercentileDiscFuncDataBuffer( aTemplate,
                                                     aFuncDataInfo,
                                                     aStack,
                                                     aPercentileDiscFuncData )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sValue = aPercentileDiscFuncData->list->data +
        aPercentileDiscFuncData->rowSize * aPercentileDiscFuncData->list->idx;
    
    if ( ( aStack[2].column->module == &mtdFloat ) ||
         ( aStack[2].column->module == &mtdNumeric ) )
    {
        idlOS::memcpy( sValue, aStack[2].value,
                       ID_SIZEOF(UChar) + ((mtdNumericType*)aStack[2].value)->length );
    }
    else if ( aStack[2].column->module == &mtdDouble )
    {
        *(mtdDoubleType*)sValue = *(mtdDoubleType*)aStack[2].value;
    }
    else if ( aStack[2].column->module == &mtdBigint )
    {
        *(mtdBigintType*)sValue = *(mtdBigintType*)aStack[2].value;
    }
    /* BUG-43821 DATE Type Support */
    else if ( aStack[2].column->module == &mtdDate )
    {
        idlOS::memcpy( sValue, aStack[2].value, ID_SIZEOF(mtdDateType) );
    }
    else
    {
        // Nothing to do.
    }

    aPercentileDiscFuncData->list->idx++;
    aPercentileDiscFuncData->totalCount++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC makeTotalDataPercentileDiscFuncData( mtcTemplate               * aTemplate,
                                            mtfFuncDataBasicInfo      * aFuncDataInfo,
                                            mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    mtfPercentileDiscFuncDataBuffer  * sList;
    UChar                            * sValue;
    
    if ( aPercentileDiscFuncData->totalCount > 0 )
    {
        IDE_DASSERT( aPercentileDiscFuncData->list != NULL );

        if ( aPercentileDiscFuncData->list->next != NULL )
        {
            /* BUG-46922 */
            IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                                       aFuncDataInfo->memoryMgr,
                                                       aPercentileDiscFuncData->rowSize *
                                                       aPercentileDiscFuncData->totalCount )
                      != IDE_SUCCESS );

            IDU_FIT_POINT_RAISE( "makeTotalDataPercentileDiscFuncData::alloc::totalData",
                                 ERR_MEMORY_ALLOCATION );
            IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc(
                                aPercentileDiscFuncData->rowSize *
                                aPercentileDiscFuncData->totalCount,
                                (void**)&(aPercentileDiscFuncData->totalData) )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
            
            sValue = aPercentileDiscFuncData->totalData;
        
            for ( sList = aPercentileDiscFuncData->list;
                  sList != NULL;
                  sList = sList->next )
            {
                idlOS::memcpy( sValue,
                               sList->data,
                               aPercentileDiscFuncData->rowSize * sList->idx );
            
                sValue += aPercentileDiscFuncData->rowSize * sList->idx;
            }
        }
        else
        {
            // chunk가 1개인 경우
            aPercentileDiscFuncData->totalData = aPercentileDiscFuncData->list->data;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


extern "C" SInt comparePercentileDiscFuncDataFloatAsc( const void * aElem1,
                                                       const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdFloat.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileDiscFuncDataFloatDesc( const void * aElem1,
                                                        const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdFloat.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileDiscFuncDataDoubleAsc( const void * aElem1,
                                                        const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDouble.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileDiscFuncDataDoubleDesc( const void * aElem1,
                                                         const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDouble.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileDiscFuncDataBigintAsc( const void * aElem1,
                                                        const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdBigint.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileDiscFuncDataBigintDesc( const void * aElem1,
                                                         const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdBigint.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

/* BUG-43821 DATE Type Support */
extern "C" SInt comparePercentileDiscFuncDataDateAsc( const void * aElem1,
                                                      const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDate.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileDiscFuncDataDateDesc( const void * aElem1,
                                                       const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDate.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

IDE_RC sortPercentileDiscFuncDataFloat( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                        mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    if ( aPercentileDiscFuncData->totalCount > 1 )
    {
        if ( aPercentileDiscFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataFloatDesc );
        }
        else
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataFloatAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC sortPercentileDiscFuncDataDouble( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                         mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    if ( aPercentileDiscFuncData->totalCount > 1 )
    {
        if ( aPercentileDiscFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataDoubleDesc );
        }
        else
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataDoubleAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC sortPercentileDiscFuncDataBigint( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                         mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    if ( aPercentileDiscFuncData->totalCount > 1 )
    {
        if ( aPercentileDiscFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataBigintDesc );
        }
        else
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataBigintAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

/* BUG-43821 DATE Type Support */
IDE_RC sortPercentileDiscFuncDataDate( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                       mtfPercentileDiscFuncData * aPercentileDiscFuncData )
{
    if ( aPercentileDiscFuncData->totalCount > 1 )
    {
        if ( aPercentileDiscFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataDateDesc );
        }
        else
        {
            idlOS::qsort( aPercentileDiscFuncData->totalData,
                          aPercentileDiscFuncData->totalCount,
                          aPercentileDiscFuncData->rowSize,
                          comparePercentileDiscFuncDataDateAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

