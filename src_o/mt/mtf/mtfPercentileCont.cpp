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
 * $Id: mtfPercentileCont.cpp 85458 2019-05-16 07:13:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfPercentileCont;

extern mtdModule mtdFloat;
extern mtdModule mtdNumeric;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdDate;
extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

#define MTF_PERCENTILE_CONT_BUFFER_MAX     (80000)

typedef struct mtfPercentileContFuncDataBuffer
{
    UChar                            * data;
    ULong                              idx;
    mtfPercentileContFuncDataBuffer  * next;
} mtfPercentileContFuncDataBuffer;

typedef struct mtfPercentileContFuncData
{
    mtfPercentileContFuncDataBuffer  * list;       // chunk data
    UInt                               rowSize;    // float, double, bigint, date size
    idBool                             orderDesc;
    ULong                              totalCount;
    UChar                            * totalData;  // all data
} mtfPercentileContFuncData;


#define PERCENTILECONT_FLOATING_POINT_EQ( a, b )            \
    ( ( !((a)>(b)) && !((a)<(b)) ) ? ID_TRUE : ID_FALSE )

static IDE_RC makePercentileContFuncData( mtcTemplate                * aTemplate,
                                          mtfFuncDataBasicInfo       * aFuncDataInfo,
                                          mtcNode                    * aNode,
                                          mtfPercentileContFuncData ** aPercentileContFuncData );

static IDE_RC allocPercentileContFuncDataBuffer( mtcTemplate               * aTemplate,
                                                 mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                 mtcStack                  * aStack,
                                                 mtfPercentileContFuncData * aPercentileContFuncData );

static IDE_RC copyToPercentileContFuncDataBuffer( mtcTemplate               * aTemplate,
                                                  mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                  mtcStack                  * aStack,
                                                  mtfPercentileContFuncData * aPercentileContFuncData );

static IDE_RC makeTotalDataPercentileContFuncData( mtcTemplate               * aTemplate,
                                                   mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                   mtfPercentileContFuncData * aPercentileContFuncData );

static IDE_RC sortPercentileContFuncDataFloat( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                               mtfPercentileContFuncData * aPercentileContFuncData );

static IDE_RC sortPercentileContFuncDataDouble( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                mtfPercentileContFuncData * aPercentileContFuncData );

static IDE_RC sortPercentileContFuncDataBigint( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                                mtfPercentileContFuncData * aPercentileContFuncData );

/* BUG-43821 DATE Type Support */
static IDE_RC sortPercentileContFuncDataDate( mtfFuncDataBasicInfo      * aFuncDataInfo,
                                              mtfPercentileContFuncData * aPercentileContFuncData );

static mtcName mtfPercentileContFunctionName[1] = {
    { NULL, 15, (void*)"PERCENTILE_CONT" }
};

static IDE_RC mtfPercentileContInitialize( void );

static IDE_RC mtfPercentileContFinalize( void );

static IDE_RC mtfPercentileContEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfPercentileCont = {
    3 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfPercentileContFunctionName,
    NULL,
    mtfPercentileContInitialize,
    mtfPercentileContFinalize,
    mtfPercentileContEstimate
};

static IDE_RC mtfPercentileContEstimateFloat( mtcNode*     aNode,
                                              mtcTemplate* aTemplate,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              mtcCallBack* aCallBack );

static IDE_RC mtfPercentileContEstimateDouble( mtcNode*     aNode,
                                               mtcTemplate* aTemplate,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               mtcCallBack* aCallBack );

static IDE_RC mtfPercentileContEstimateBigint( mtcNode*     aNode,
                                               mtcTemplate* aTemplate,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               mtcCallBack* aCallBack );

/* BUG-43821 DATE Type Support */
static IDE_RC mtfPercentileContEstimateDate( mtcNode     * aNode,
                                             mtcTemplate * aTemplate,
                                             mtcStack    * aStack,
                                             SInt          aRemain,
                                             mtcCallBack * aCallBack );

IDE_RC mtfPercentileContInitialize( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfPercentileContAggregate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfPercentileContMerge( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC mtfPercentileContCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfPercentileContEstimates[4] = {
    { mtfPercentileContEstimates+1, mtfPercentileContEstimateDouble },
    { mtfPercentileContEstimates+2, mtfPercentileContEstimateBigint },
    { mtfPercentileContEstimates+3, mtfPercentileContEstimateDate },    /* BUG-43821 DATE Type Support */
    { NULL,                         mtfPercentileContEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfPercentileContEstimatesHighPerformance[3] = {
    { mtfPercentileContEstimatesHighPerformance+1, mtfPercentileContEstimateDouble },
    { mtfPercentileContEstimatesHighPerformance+2, mtfPercentileContEstimateBigint },
    { NULL,                                        mtfPercentileContEstimateDate }  /* BUG-43821 DATE Type Support */
};

static mtfSubModule** mtfTable = NULL;
static mtfSubModule** mtfTableHighPerformance = NULL;

IDE_RC mtfPercentileContInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfPercentileContEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfPercentileContEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfPercentileContFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfPercentileContEstimate( mtcNode*     aNode,
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
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
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

IDE_RC mtfPercentileContInitialize( mtcNode*     aNode,
                                    mtcStack*,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
    iduMemory                  * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo       * sFuncDataInfo;
    mtfPercentileContFuncData  * sPercentileContFuncData;
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
        IDU_FIT_POINT_RAISE( "mtfPercentileContInitialize::alloc::sFuncDataInfo",
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

    // make percentileCont function data
    IDE_TEST( makePercentileContFuncData( aTemplate,
                                          sFuncDataInfo,
                                          aNode,
                                          & sPercentileContFuncData )
              != IDE_SUCCESS );

    // set percentile function data
    sBinary->mLength = ID_SIZEOF(mtfPercentileContFuncData*);
    *((mtfPercentileContFuncData**)sBinary->mValue) = sPercentileContFuncData;

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

IDE_RC mtfPercentileContAggregate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    mtfFuncDataBasicInfo       * sFuncDataInfo;
    mtfPercentileContFuncData  * sPercentileContFuncData;
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
        sPercentileContFuncData = *((mtfPercentileContFuncData**)sBinary->mValue);

        // percentile option
        if ( sPercentileContFuncData->totalCount == 0 )
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
        IDE_TEST( copyToPercentileContFuncDataBuffer( aTemplate,
                                                      sFuncDataInfo,
                                                      aStack,
                                                      sPercentileContFuncData )
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

IDE_RC mtfPercentileContMerge( mtcNode     * aNode,
                               mtcStack    * ,
                               SInt          ,
                               void        * aInfo,
                               mtcTemplate * aTemplate )
{
    const mtcColumn                 * sColumn;
    UChar                           * sDstRow;
    UChar                           * sSrcRow;
    mtdBinaryType                   * sSrcValue;
    mtfPercentileContFuncData       * sSrcPercentileContFuncData;
    mtdBinaryType                   * sDstValue;
    mtfPercentileContFuncData       * sDstPercentileContFuncData;
    mtfPercentileContFuncDataBuffer * sPercentileContFuncDataBuffer;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;

    // src percentile function data.
    sSrcValue = (mtdBinaryType*)( sSrcRow + sColumn[1].column.offset );
    sSrcPercentileContFuncData = *((mtfPercentileContFuncData**)sSrcValue->mValue);

    // dst percentile function data.
    sDstValue = (mtdBinaryType*)( sDstRow + sColumn[1].column.offset );
    sDstPercentileContFuncData = *((mtfPercentileContFuncData**)sDstValue->mValue);

    if ( sSrcPercentileContFuncData->list == NULL )
    {
        // Nothing To Do
    }
    else
    {
        if ( sDstPercentileContFuncData->list == NULL )
        {
            *((mtfPercentileContFuncData**)sDstValue->mValue) =
                sSrcPercentileContFuncData;
        }
        else
        {
            for ( sPercentileContFuncDataBuffer = sDstPercentileContFuncData->list;
                  sPercentileContFuncDataBuffer->next != NULL;
                  sPercentileContFuncDataBuffer = sPercentileContFuncDataBuffer->next );
            
            sPercentileContFuncDataBuffer->next =
                sSrcPercentileContFuncData->list;

            sDstPercentileContFuncData->totalCount +=
                sSrcPercentileContFuncData->totalCount;
        }
    }

    // percentile option
    *(mtdDoubleType *)(sDstRow + sColumn[2].column.offset) =
        *(mtdDoubleType *)(sSrcRow + sColumn[2].column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfPercentileContCalculate( mtcNode*     aNode,
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

IDE_RC mtfPercentileContFinalizeFloat( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute mtfPercentileContExecuteFloat = {
    mtfPercentileContInitialize,
    mtfPercentileContAggregate,
    mtfPercentileContMerge,
    mtfPercentileContFinalizeFloat,
    mtfPercentileContCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileContEstimateFloat( mtcNode*     aNode,
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
        // percentile_cont 함수로 보면 2번째 인자이다.
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileContExecuteFloat;
    
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
                                     ID_SIZEOF( mtfPercentileContFuncData * ),
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

IDE_RC mtfPercentileContFinalizeFloat( mtcNode      * aNode,
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
    mtfPercentileContFuncData  * sPercentileContFuncData;
    SDouble                      sRowNum;
    SLong                        sCeilRowNum;
    SLong                        sFloorRowNum;
    UChar                        sCeilBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloorBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloatBuff1[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloatBuff2[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType             * sCeilResult     = (mtdNumericType*)sCeilBuff;
    mtdNumericType             * sFloorResult    = (mtdNumericType*)sFloorBuff;
    mtdNumericType             * sMultiplyValue1 = (mtdNumericType*)sFloatBuff1;
    mtdNumericType             * sMultiplyValue2 = (mtdNumericType*)sFloatBuff2;
    mtdNumericType             * sFloatArgument1;
    mtdNumericType             * sFloatArgument2;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileContFuncData = *((mtfPercentileContFuncData**)sBinary->mValue);

    sResult = (mtdNumericType*)(sRow + sColumn[0].column.offset);
    
    //------------------------------------------
    // sort
    //------------------------------------------
    
    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileContFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileContFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileContFuncDataFloat( sFuncDataBasicInfo,
                                               sPercentileContFuncData )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // 연산 수행    
    //------------------------------------------

    if ( sPercentileContFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType*)(sRow + sColumn[2].column.offset));
                             
        // RN( 중간 row위치) 을 구한다.
        sRowNum = ( 1 + ( sFirstValue * ( sPercentileContFuncData->totalCount - 1 ) ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {                        
            sFloatArgument1 = (mtdNumericType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );

            idlOS::memcpy( sResult,
                           sFloatArgument1,
                           ID_SIZEOF(UChar) + sFloatArgument1->length );
        }
        else
        {
            sFloatArgument1 = (mtdNumericType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );

            // BUG-41224 chage makeNumeric function.
            IDE_TEST( mtc::makeNumeric( sMultiplyValue1,
                                        (SDouble)sCeilRowNum - sRowNum )
                      != IDE_SUCCESS );
            
            IDE_TEST( mtc::multiplyFloat( sFloorResult,
                                          MTD_FLOAT_PRECISION_MAXIMUM,
                                          sFloatArgument1,
                                          sMultiplyValue1 )
                      != IDE_SUCCESS );
            
            sFloatArgument2 = (mtdNumericType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sCeilRowNum - 1 ) );

            // BUG-41224 chage makeNumeric function.
            IDE_TEST( mtc::makeNumeric( sMultiplyValue2,
                                        sRowNum - (SDouble)sFloorRowNum )
                      != IDE_SUCCESS );            
            
            IDE_TEST( mtc::multiplyFloat( sCeilResult,
                                          MTD_FLOAT_PRECISION_MAXIMUM,
                                          sFloatArgument2,
                                          sMultiplyValue2 )
                      != IDE_SUCCESS );
            
            IDE_TEST( mtc::addFloat( sResult,
                                     MTD_FLOAT_PRECISION_MAXIMUM,
                                     sFloorResult,
                                     sCeilResult )
                      != IDE_SUCCESS );
        }
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

IDE_RC mtfPercentileContFinalizeDouble( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfPercentileContExecuteDouble = {
    mtfPercentileContInitialize,
    mtfPercentileContAggregate,
    mtfPercentileContMerge,
    mtfPercentileContFinalizeDouble,
    mtfPercentileContCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileContEstimateDouble( mtcNode*     aNode,
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
        // percentile_cont 함수로 보면 2번째 인자이다.
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileContExecuteDouble;
    
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
                                     ID_SIZEOF( mtfPercentileContFuncData * ),
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

IDE_RC mtfPercentileContFinalizeDouble( mtcNode     * aNode,
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
    mtdDoubleType                sCeilResult;
    mtdDoubleType                sFloorResult;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo;
    mtfPercentileContFuncData  * sPercentileContFuncData;
    SDouble                      sRowNum;
    SLong                        sCeilRowNum;
    SLong                        sFloorRowNum;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileContFuncData = *((mtfPercentileContFuncData**)sBinary->mValue);

    sResult = (mtdDoubleType*)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------
    
    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileContFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileContFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileContFuncDataDouble( sFuncDataBasicInfo,
                                                sPercentileContFuncData )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // 연산 수행
    //------------------------------------------
    
    if ( sPercentileContFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType*)(sRow + sColumn[2].column.offset));

        // RN( 중간 row위치) 을 구한다.
        sRowNum = ( 1 + ( sFirstValue * ( sPercentileContFuncData->totalCount - 1 ) ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            *sResult = *(mtdDoubleType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );
        }
        else
        {
            *sResult = *(mtdDoubleType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );

            sFloorResult = ( sCeilRowNum - sRowNum ) * *sResult;
                        
            *sResult = *(mtdDoubleType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sCeilRowNum - 1 ) );
            
            sCeilResult = ( sRowNum - sFloorRowNum ) * *sResult;
            
            *sResult = sFloorResult + sCeilResult;
        }
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

IDE_RC mtfPercentileContFinalizeBigint( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute mtfPercentileContExecuteBigint = {
    mtfPercentileContInitialize,
    mtfPercentileContAggregate,
    mtfPercentileContMerge,
    mtfPercentileContFinalizeBigint,
    mtfPercentileContCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileContEstimateBigint( mtcNode*     aNode,
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
        // percentile_cont 함수로 보면 2번째 인자이다.
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileContExecuteBigint;

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
                                     ID_SIZEOF( mtfPercentileContFuncData * ),
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

IDE_RC mtfPercentileContFinalizeBigint( mtcNode     * aNode,
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
    mtdDoubleType                sCeilResult;
    mtdDoubleType                sFloorResult;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo;
    mtfPercentileContFuncData  * sPercentileContFuncData;
    SDouble                      sRowNum;
    SLong                        sCeilRowNum;
    SLong                        sFloorRowNum;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType*)(sRow + sColumn[1].column.offset);
    
    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileContFuncData = *((mtfPercentileContFuncData**)sBinary->mValue);

    sResult = (mtdDoubleType*)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------
    
    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileContFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileContFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileContFuncDataBigint( sFuncDataBasicInfo,
                                                sPercentileContFuncData )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // 연산 수행
    //------------------------------------------
    
    if ( sPercentileContFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType*)(sRow + sColumn[2].column.offset));

        // RN( 중간 row위치) 을 구한다.
        sRowNum = ( 1 + ( sFirstValue * ( sPercentileContFuncData->totalCount - 1 ) ) );
        
        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            *sResult = *(mtdBigintType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );
        }
        else
        {
            *sResult = *(mtdBigintType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );

            sFloorResult = ( sCeilRowNum - sRowNum ) * *sResult;
            
            *sResult = *(mtdBigintType*)
                ( sPercentileContFuncData->totalData +
                  sPercentileContFuncData->rowSize * ( sCeilRowNum - 1 ) );

            sCeilResult = ( sRowNum - sFloorRowNum ) * *sResult;

            *sResult = sFloorResult + sCeilResult;
        }
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

/* BUG-43821 DATE Type Support */
/* ZONE: DATE */

IDE_RC mtfPercentileContFinalizeDate( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

static const mtcExecute mtfPercentileContExecuteDate = {
    mtfPercentileContInitialize,
    mtfPercentileContAggregate,
    mtfPercentileContMerge,
    mtfPercentileContFinalizeDate,
    mtfPercentileContCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfPercentileContEstimateDate( mtcNode     * aNode,
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
        // percentile_cont 함수로 보면 2번째 인자이다.
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfPercentileContExecuteDate;

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
                                     ID_SIZEOF( mtfPercentileContFuncData * ),
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

IDE_RC mtfPercentileContFinalizeDate( mtcNode      * aNode,
                                      mtcStack     * ,
                                      SInt           ,
                                      void         * ,
                                      mtcTemplate  * aTemplate )
{
    const mtcColumn            * sColumn                 = NULL;
    mtdBinaryType              * sBinary                 = NULL;
    UChar                      * sRow                    = NULL;
    mtdDoubleType                sFirstValue             = 0.0;
    mtdDateType                * sDateArgument           = NULL;
    mtdDateType                * sResult                 = NULL;
    mtdIntervalType              sIntervalResult;
    mtdIntervalType              sIntervalCeilResult;
    mtdIntervalType              sIntervalFloorResult;
    UChar                        sResultBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sCeilBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloorBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloatBuff1[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloatBuff2[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloatRoundBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                        sFloatZeroBuff[20]      = { 1, 128, 0, };
    mtdNumericType             * sNumericResult          = (mtdNumericType *)sResultBuff;
    mtdNumericType             * sNumericCeilResult      = (mtdNumericType *)sCeilBuff;
    mtdNumericType             * sNumericFloorResult     = (mtdNumericType *)sFloorBuff;
    mtdNumericType             * sNumericArgument        = (mtdNumericType *)sFloatBuff1;
    mtdNumericType             * sMultiplyValue          = (mtdNumericType *)sFloatBuff2;
    mtdNumericType             * sNumericRoundResult     = (mtdNumericType *)sFloatRoundBuff;
    mtdNumericType             * sNumericZero            = (mtdNumericType *)sFloatZeroBuff;
    mtfFuncDataBasicInfo       * sFuncDataBasicInfo      = NULL;
    mtfPercentileContFuncData  * sPercentileContFuncData = NULL;
    SDouble                      sRowNum                 = 0.0;
    SLong                        sCeilRowNum             = ID_LONG(0);
    SLong                        sFloorRowNum            = ID_LONG(0);

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sPercentileContFuncData = *((mtfPercentileContFuncData **)sBinary->mValue);

    sResult = (mtdDateType *)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------

    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataPercentileContFuncData( aTemplate,
                                                   sFuncDataBasicInfo,
                                                   sPercentileContFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortPercentileContFuncDataDate( sFuncDataBasicInfo,
                                              sPercentileContFuncData )
              != IDE_SUCCESS );

    //------------------------------------------
    // 연산 수행
    //------------------------------------------

    if ( sPercentileContFuncData->totalCount > 0 )
    {
        // percentile option
        sFirstValue = *((mtdDoubleType *)(sRow + sColumn[2].column.offset));

        // RN( 중간 row위치) 을 구한다.
        sRowNum = ( 1 + ( sFirstValue * ( sPercentileContFuncData->totalCount - 1 ) ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( PERCENTILECONT_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            sDateArgument = (mtdDateType *)( sPercentileContFuncData->totalData +
                                             sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );

            idlOS::memcpy( sResult, sDateArgument, ID_SIZEOF(mtdDateType) );
        }
        else
        {
            sDateArgument = (mtdDateType *)( sPercentileContFuncData->totalData +
                                             sPercentileContFuncData->rowSize * ( sFloorRowNum - 1 ) );

            IDE_TEST( mtdDateInterface::convertDate2Interval( sDateArgument,
                                                              &sIntervalFloorResult )
                      != IDE_SUCCESS );

            sIntervalFloorResult.microsecond += sIntervalFloorResult.second * 1000000;

            // Ceil (SLong -> Numeric)
            mtc::makeNumeric( sNumericArgument,
                              sIntervalFloorResult.microsecond );

            IDE_TEST( mtc::makeNumeric( sMultiplyValue,
                                        (SDouble)sCeilRowNum - sRowNum )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::multiplyFloat( sNumericCeilResult,
                                          MTD_FLOAT_PRECISION_MAXIMUM,
                                          sNumericArgument,
                                          sMultiplyValue )
                      != IDE_SUCCESS );

            sDateArgument = (mtdDateType *)( sPercentileContFuncData->totalData +
                                             sPercentileContFuncData->rowSize * ( sCeilRowNum - 1 ) );

            IDE_TEST( mtdDateInterface::convertDate2Interval( sDateArgument,
                                                              &sIntervalCeilResult )
                      != IDE_SUCCESS );

            sIntervalCeilResult.microsecond += sIntervalCeilResult.second * 1000000;

            // Floor (SLong -> Numeric)
            mtc::makeNumeric( sNumericArgument,
                              sIntervalCeilResult.microsecond );

            IDE_TEST( mtc::makeNumeric( sMultiplyValue,
                                        sRowNum - (SDouble)sFloorRowNum )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::multiplyFloat( sNumericFloorResult,
                                          MTD_FLOAT_PRECISION_MAXIMUM,
                                          sNumericArgument,
                                          sMultiplyValue )
                      != IDE_SUCCESS );

            // Ceil + Floor (Numeric)
            IDE_TEST( mtc::addFloat( sNumericResult,
                                     MTD_FLOAT_PRECISION_MAXIMUM,
                                     sNumericFloorResult,
                                     sNumericCeilResult )
                      != IDE_SUCCESS );

            // Round (Numeric)
            IDE_TEST( mtc::roundFloat( sNumericRoundResult,
                                       sNumericResult,
                                       sNumericZero )
                      != IDE_SUCCESS );

            // Numeric -> SLong
            IDE_TEST( mtc::numeric2Slong( &sIntervalResult.microsecond,
                                          sNumericRoundResult )
                      != IDE_SUCCESS );

            sIntervalResult.second      = sIntervalResult.microsecond / 1000000;
            sIntervalResult.microsecond = sIntervalResult.microsecond % 1000000;

            IDE_TEST( mtdDateInterface::convertInterval2Date( &sIntervalResult,
                                                              sResult )
                      != IDE_SUCCESS );
        }
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

IDE_RC makePercentileContFuncData( mtcTemplate                * aTemplate,
                                   mtfFuncDataBasicInfo       * aFuncDataInfo,
                                   mtcNode                    * aNode,
                                   mtfPercentileContFuncData ** aPercentileContFuncData )
{
    mtfPercentileContFuncData  * sPercentileContFuncData;

    IDE_TEST_RAISE( aNode->funcArguments == NULL, ERR_INVALID_FUNCTION_ARGUMENT );

    /* BUG-46922 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ID_SIZEOF( mtfPercentileContFuncData ) )
              != IDE_SUCCESS );
            
    // function data
    IDU_FIT_POINT_RAISE( "makePercentileContFuncData::cralloc::sPercentileContFuncData",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->cralloc(
                        ID_SIZEOF(mtfPercentileContFuncData),
                        (void**)&sPercentileContFuncData )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
        
    if ( ( aNode->funcArguments->lflag & MTC_NODE_WITHIN_GROUP_ORDER_MASK )
         == MTC_NODE_WITHIN_GROUP_ORDER_DESC )
    {
        sPercentileContFuncData->orderDesc = ID_TRUE;
    }
    else
    {
        sPercentileContFuncData->orderDesc = ID_FALSE;
    }
    
    // return
    *aPercentileContFuncData = sPercentileContFuncData;

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

IDE_RC allocPercentileContFuncDataBuffer( mtcTemplate               * aTemplate,
                                          mtfFuncDataBasicInfo      * aFuncDataInfo,
                                          mtcStack                  * aStack,
                                          mtfPercentileContFuncData * aPercentileContFuncData )
{
    mtfPercentileContFuncDataBuffer  * sBuffer;
    UInt                               sRowSize;

    if ( aPercentileContFuncData->rowSize == 0 )
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

        aPercentileContFuncData->rowSize = sRowSize;
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-46922 */
    IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                               aFuncDataInfo->memoryMgr,
                                               ID_SIZEOF( mtfPercentileContFuncDataBuffer ) +
                                               aPercentileContFuncData->rowSize *
                                               MTF_PERCENTILE_CONT_BUFFER_MAX )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "allocPercentileContFuncDataBuffer::alloc::sBuffer",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc(
                        ID_SIZEOF(mtfPercentileContFuncDataBuffer) +
                        aPercentileContFuncData->rowSize * MTF_PERCENTILE_CONT_BUFFER_MAX,
                        (void**)&sBuffer )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
   
    sBuffer->data = (UChar*)sBuffer + ID_SIZEOF(mtfPercentileContFuncDataBuffer);
    sBuffer->idx  = 0;
    sBuffer->next = aPercentileContFuncData->list;

    // link
    aPercentileContFuncData->list = sBuffer;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_TYPE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "allocPercentileContFuncDataBuffer",
                                  "invalid arguemnt type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC copyToPercentileContFuncDataBuffer( mtcTemplate               * aTemplate,
                                           mtfFuncDataBasicInfo      * aFuncDataInfo,
                                           mtcStack                  * aStack,
                                           mtfPercentileContFuncData * aPercentileContFuncData )
{
    void   * sValue;
    idBool   sAlloc = ID_FALSE;
    
    if ( aPercentileContFuncData->list == NULL )
    {
        sAlloc = ID_TRUE;
    }
    else
    {
        if ( aPercentileContFuncData->list->idx == MTF_PERCENTILE_CONT_BUFFER_MAX )
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
        IDE_TEST( allocPercentileContFuncDataBuffer( aTemplate,
                                                     aFuncDataInfo,
                                                     aStack,
                                                     aPercentileContFuncData )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sValue = aPercentileContFuncData->list->data +
        aPercentileContFuncData->rowSize * aPercentileContFuncData->list->idx;
    
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

    aPercentileContFuncData->list->idx++;
    aPercentileContFuncData->totalCount++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC makeTotalDataPercentileContFuncData( mtcTemplate               * aTemplate,
                                            mtfFuncDataBasicInfo      * aFuncDataInfo,
                                            mtfPercentileContFuncData * aPercentileContFuncData )
{
    mtfPercentileContFuncDataBuffer  * sList;
    UChar                            * sValue;
    
    if ( aPercentileContFuncData->totalCount > 0 )
    {
        IDE_DASSERT( aPercentileContFuncData->list != NULL );

        if ( aPercentileContFuncData->list->next != NULL )
        {
            /* BUG-46922 */
            IDE_TEST( mtf::checkFuncDataMemoryMaximum( aTemplate,
                                                       aFuncDataInfo->memoryMgr,
                                                       aPercentileContFuncData->rowSize *
                                                       aPercentileContFuncData->totalCount )
                      != IDE_SUCCESS );

            IDU_FIT_POINT_RAISE( "makeTotalDataPercentileContFuncData::alloc::totalData",
                                 ERR_MEMORY_ALLOCATION );
            IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc(
                                aPercentileContFuncData->rowSize *
                                aPercentileContFuncData->totalCount,
                                (void**)&(aPercentileContFuncData->totalData) )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );
            
            sValue = aPercentileContFuncData->totalData;
        
            for ( sList = aPercentileContFuncData->list;
                  sList != NULL;
                  sList = sList->next )
            {
                idlOS::memcpy( sValue,
                               sList->data,
                               aPercentileContFuncData->rowSize * sList->idx );
            
                sValue += aPercentileContFuncData->rowSize * sList->idx;
            }
        }
        else
        {
            // chunk가 1개인 경우
            aPercentileContFuncData->totalData = aPercentileContFuncData->list->data;
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


extern "C" SInt comparePercentileContFuncDataFloatAsc( const void * aElem1,
                                                       const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdFloat.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileContFuncDataFloatDesc( const void * aElem1,
                                                        const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdFloat.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileContFuncDataDoubleAsc( const void * aElem1,
                                                        const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDouble.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileContFuncDataDoubleDesc( const void * aElem1,
                                                         const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDouble.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileContFuncDataBigintAsc( const void * aElem1,
                                                        const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdBigint.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileContFuncDataBigintDesc( const void * aElem1,
                                                         const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdBigint.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

/* BUG-43821 DATE Type Support */
extern "C" SInt comparePercentileContFuncDataDateAsc( const void * aElem1,
                                                      const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDate.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt comparePercentileContFuncDataDateDesc( const void * aElem1,
                                                       const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDate.logicalCompare[1]( &sValueInfo1, &sValueInfo2 );
}

IDE_RC sortPercentileContFuncDataFloat( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                        mtfPercentileContFuncData * aPercentileContFuncData )
{
    if ( aPercentileContFuncData->totalCount > 1 )
    {
        if ( aPercentileContFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataFloatDesc );
        }
        else
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataFloatAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC sortPercentileContFuncDataDouble( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                         mtfPercentileContFuncData * aPercentileContFuncData )
{
    if ( aPercentileContFuncData->totalCount > 1 )
    {
        if ( aPercentileContFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataDoubleDesc );
        }
        else
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataDoubleAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC sortPercentileContFuncDataBigint( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                         mtfPercentileContFuncData * aPercentileContFuncData )
{
    if ( aPercentileContFuncData->totalCount > 1 )
    {
        if ( aPercentileContFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataBigintDesc );
        }
        else
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataBigintAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

/* BUG-43821 DATE Type Support */
IDE_RC sortPercentileContFuncDataDate( mtfFuncDataBasicInfo      * /*aFuncDataInfo*/,
                                       mtfPercentileContFuncData * aPercentileContFuncData )
{
    if ( aPercentileContFuncData->totalCount > 1 )
    {
        if ( aPercentileContFuncData->orderDesc == ID_TRUE )
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataDateDesc );
        }
        else
        {
            idlOS::qsort( aPercentileContFuncData->totalData,
                          aPercentileContFuncData->totalCount,
                          aPercentileContFuncData->rowSize,
                          comparePercentileContFuncDataDateAsc );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

