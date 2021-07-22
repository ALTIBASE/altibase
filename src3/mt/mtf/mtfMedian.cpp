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
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfMedian;

extern mtdModule mtdFloat;
extern mtdModule mtdNumeric;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdDate;
extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

#define MTF_MEDIAN_BUFFER_MAX               (80000)
#define MEDIAN_FLOATING_POINT_EQ( a, b )    ( ( !((a) > (b)) && !((a) < (b)) ) ? ID_TRUE : ID_FALSE )

typedef struct mtfMedianFuncDataBuffer
{
    UChar                   * data;
    ULong                     idx;
    mtfMedianFuncDataBuffer * next;
} mtfMedianFuncDataBuffer;

typedef struct mtfMedianFuncData
{
    mtfMedianFuncDataBuffer * list;       // chunk data
    UInt                      rowSize;    // float, double, bigint, date size
    ULong                     totalCount;
    UChar                   * totalData;  // all data
} mtfMedianFuncData;

static IDE_RC makeMedianFuncData( mtfFuncDataBasicInfo  * aFuncDataInfo,
                                  mtcNode               * aNode,
                                  mtfMedianFuncData    ** aMedianFuncData );

static IDE_RC allocMedianFuncDataBuffer( mtfFuncDataBasicInfo * aFuncDataInfo,
                                         mtcStack             * aStack,
                                         mtfMedianFuncData    * aMedianFuncData );

static IDE_RC copyToMedianFuncDataBuffer( mtfFuncDataBasicInfo * aFuncDataInfo,
                                          mtcStack             * aStack,
                                          mtfMedianFuncData    * aMedianFuncData );

static IDE_RC makeTotalDataMedianFuncData( mtfFuncDataBasicInfo * aFuncDataInfo,
                                           mtfMedianFuncData    * aMedianFuncData );

static IDE_RC sortMedianFuncDataFloat( mtfFuncDataBasicInfo * aFuncDataInfo,
                                       mtfMedianFuncData    * aMedianFuncData );

static IDE_RC sortMedianFuncDataDouble( mtfFuncDataBasicInfo * aFuncDataInfo,
                                        mtfMedianFuncData    * aMedianFuncData );

static IDE_RC sortMedianFuncDataBigint( mtfFuncDataBasicInfo * aFuncDataInfo,
                                        mtfMedianFuncData    * aMedianFuncData );

/* BUG-43821 DATE Type Support */
static IDE_RC sortMedianFuncDataDate( mtfFuncDataBasicInfo * aFuncDataInfo,
                                      mtfMedianFuncData    * aMedianFuncData );

static mtcName mtfMedianFunctionName[1] = {
    { NULL, 6, (void *)"MEDIAN" }
};

static IDE_RC mtfMedianInitialize( void );

static IDE_RC mtfMedianFinalize( void );

static IDE_RC mtfMedianEstimate( mtcNode     * aNode,
                                 mtcTemplate * aTemplate,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 mtcCallBack * aCallBack );

mtfModule mtfMedian = {
    2 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfMedianFunctionName,
    NULL,
    mtfMedianInitialize,
    mtfMedianFinalize,
    mtfMedianEstimate
};

static IDE_RC mtfMedianEstimateFloat( mtcNode     * aNode,
                                      mtcTemplate * aTemplate,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      mtcCallBack * aCallBack );

static IDE_RC mtfMedianEstimateDouble( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

static IDE_RC mtfMedianEstimateBigint( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

/* BUG-43821 DATE Type Support */
static IDE_RC mtfMedianEstimateDate( mtcNode     * aNode,
                                     mtcTemplate * aTemplate,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     mtcCallBack * aCallBack );

IDE_RC mtfMedianInitialize( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

IDE_RC mtfMedianAggregate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

IDE_RC mtfMedianMerge( mtcNode     * aNode,
                       mtcStack    * aStack,
                       SInt          aRemain,
                       void        * aInfo,
                       mtcTemplate * aTemplate );

IDE_RC mtfMedianCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfMedianEstimates[4] = {
    { mtfMedianEstimates + 1, mtfMedianEstimateDouble },
    { mtfMedianEstimates + 2, mtfMedianEstimateBigint },
    { mtfMedianEstimates + 3, mtfMedianEstimateDate },      /* BUG-43821 DATE Type Support */
    { NULL,                   mtfMedianEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfMedianEstimatesHighPerformance[3] = {
    { mtfMedianEstimatesHighPerformance + 1, mtfMedianEstimateDouble },
    { mtfMedianEstimatesHighPerformance + 2, mtfMedianEstimateBigint },
    { NULL,                                  mtfMedianEstimateDate }    /* BUG-43821 DATE Type Support */
};

static mtfSubModule ** mtfTable = NULL;
static mtfSubModule ** mtfTableHighPerformance = NULL;

IDE_RC mtfMedianInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfMedianEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfMedianEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianEstimate( mtcNode     * aNode,
                          mtcTemplate * aTemplate,
                          mtcStack    * aStack,
                          SInt          aRemain,
                          mtcCallBack * aCallBack )
{
    const mtfSubModule  * sSubModule = NULL;
    mtfSubModule       ** sTable     = NULL;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
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
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianInitialize( mtcNode     * aNode,
                            mtcStack    * ,
                            SInt          ,
                            void        * ,
                            mtcTemplate * aTemplate )
{
    iduMemory            * sMemoryMgr      = NULL;
    mtfFuncDataBasicInfo * sFuncDataInfo   = NULL;
    mtfMedianFuncData    * sMedianFuncData = NULL;
    const mtcColumn      * sColumn         = NULL;
    mtdBinaryType        * sBinary         = NULL;
    UChar                * sRow            = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );

        // function data alloc
        IDU_FIT_POINT_RAISE( "mtfMedianInitialize::alloc::sFuncDataInfo",
                             ERR_MEMORY_ALLOCATION );
        IDE_TEST_RAISE( sMemoryMgr->alloc( ID_SIZEOF(mtfFuncDataBasicInfo),
                                           (void **)&sFuncDataInfo )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

        // function data init
        IDE_TEST( mtf::initializeFuncDataBasicInfo( sFuncDataInfo,
                                                    sMemoryMgr )
                  != IDE_SUCCESS );

        // 등록
        aTemplate->funcData[aNode->info] = sFuncDataInfo;
    }
    else
    {
        sFuncDataInfo = aTemplate->funcData[aNode->info];
    }

    // make Median function data
    IDE_TEST( makeMedianFuncData( sFuncDataInfo,
                                  aNode,
                                  & sMedianFuncData )
              != IDE_SUCCESS );

    // set Median function data
    sBinary->mLength = ID_SIZEOF(mtfMedianFuncData *);
    *((mtfMedianFuncData **)sBinary->mValue) = sMedianFuncData;

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
        // Nothing to do
    }

    return IDE_FAILURE;
}

IDE_RC mtfMedianAggregate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * ,
                           mtcTemplate * aTemplate )
{
    mtfFuncDataBasicInfo * sFuncDataInfo   = NULL;
    mtfMedianFuncData    * sMedianFuncData = NULL;
    const mtcColumn      * sColumn         = NULL;
    mtdBinaryType        * sBinary         = NULL;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sBinary = (mtdBinaryType *)((UChar *)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);
        sFuncDataInfo = aTemplate->funcData[aNode->info];
        sMedianFuncData = *((mtfMedianFuncData **)sBinary->mValue);

        // copy function data
        IDE_TEST( copyToMedianFuncDataBuffer( sFuncDataInfo,
                                              aStack,
                                              sMedianFuncData )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianMerge( mtcNode     * aNode,
                       mtcStack    * ,
                       SInt          ,
                       void        * aInfo,
                       mtcTemplate * aTemplate )
{
    const mtcColumn         * sColumn               = NULL;
    UChar                   * sDstRow               = NULL;
    UChar                   * sSrcRow               = NULL;
    mtdBinaryType           * sSrcValue             = NULL;
    mtfMedianFuncData       * sSrcMedianFuncData    = NULL;
    mtdBinaryType           * sDstValue             = NULL;
    mtfMedianFuncData       * sDstMedianFuncData    = NULL;
    mtfMedianFuncDataBuffer * sMedianFuncDataBuffer = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstRow = (UChar *)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar *)aInfo;

    // src Median function data.
    sSrcValue = (mtdBinaryType *)( sSrcRow + sColumn[1].column.offset );
    sSrcMedianFuncData = *((mtfMedianFuncData **)sSrcValue->mValue);

    // dst Median function data.
    sDstValue = (mtdBinaryType *)( sDstRow + sColumn[1].column.offset );
    sDstMedianFuncData = *((mtfMedianFuncData **)sDstValue->mValue);

    if ( sSrcMedianFuncData->list == NULL )
    {
        // Nothing to do
    }
    else
    {
        if ( sDstMedianFuncData->list == NULL )
        {
            *((mtfMedianFuncData **)sDstValue->mValue) = sSrcMedianFuncData;
        }
        else
        {
            for ( sMedianFuncDataBuffer = sDstMedianFuncData->list;
                  sMedianFuncDataBuffer->next != NULL;
                  sMedianFuncDataBuffer = sMedianFuncDataBuffer->next );

            sMedianFuncDataBuffer->next = sSrcMedianFuncData->list;

            sDstMedianFuncData->totalCount += sSrcMedianFuncData->totalCount;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC mtfMedianCalculate( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          ,
                           void        * ,
                           mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void *)((UChar *) aTemplate->rows[aNode->table].row + aStack->column->column.offset);

    return IDE_SUCCESS;
}

/* ZONE: FLOAT */

IDE_RC mtfMedianFinalizeFloat( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate );

static const mtcExecute mtfMedianExecuteFloat = {
    mtfMedianInitialize,
    mtfMedianAggregate,
    mtfMedianMerge,
    mtfMedianFinalizeFloat,
    mtfMedianCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMedianEstimateFloat( mtcNode     * aNode,
                               mtcTemplate * aTemplate,
                               mtcStack    * aStack,
                               SInt          ,
                               mtcCallBack * aCallBack )
{
    const mtdModule * sModules[1] = { NULL };

    mtc::makeFloatConversionModule( aStack + 1, &sModules[0] );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMedianExecuteFloat;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Median function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfMedianFuncData * ),
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
        IDE_SET(ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianFinalizeFloat( mtcNode      * aNode,
                               mtcStack     * ,
                               SInt           ,
                               void         * ,
                               mtcTemplate  * aTemplate )
{
    const mtcColumn      * sColumn            = NULL;
    mtdBinaryType        * sBinary            = NULL;
    UChar                * sRow               = NULL;
    mtdNumericType       * sResult            = NULL;
    mtfFuncDataBasicInfo * sFuncDataBasicInfo = NULL;
    mtfMedianFuncData    * sMedianFuncData    = NULL;
    SDouble                sRowNum            = 0.0;
    SLong                  sCeilRowNum        = ID_LONG(0);
    SLong                  sFloorRowNum       = ID_LONG(0);
    UChar                  sCeilBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloorBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloatBuff1[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloatBuff2[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType       * sCeilResult        = (mtdNumericType *)sCeilBuff;
    mtdNumericType       * sFloorResult       = (mtdNumericType *)sFloorBuff;
    mtdNumericType       * sMultiplyValue1    = (mtdNumericType *)sFloatBuff1;
    mtdNumericType       * sMultiplyValue2    = (mtdNumericType *)sFloatBuff2;
    mtdNumericType       * sFloatArgument1    = NULL;
    mtdNumericType       * sFloatArgument2    = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sMedianFuncData = *((mtfMedianFuncData **)sBinary->mValue);

    sResult = (mtdNumericType *)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------

    // chunk를 하나로 합쳐  data를 생성한다.
    IDE_TEST( makeTotalDataMedianFuncData( sFuncDataBasicInfo,
                                           sMedianFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortMedianFuncDataFloat( sFuncDataBasicInfo,
                                       sMedianFuncData )
              != IDE_SUCCESS );

    //------------------------------------------
    // 연산 수행
    //------------------------------------------

    if ( sMedianFuncData->totalCount > 0 )
    {
        // RN(중간 row위치)을 구한다.
        sRowNum = 1 + ( 0.5 * ( sMedianFuncData->totalCount - 1 ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            sFloatArgument1 = (mtdNumericType *) ( sMedianFuncData->totalData +
                                                   sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );

            idlOS::memcpy( sResult,
                           sFloatArgument1,
                           ID_SIZEOF(UChar) + sFloatArgument1->length );
        }
        else
        {
            sFloatArgument1 = (mtdNumericType *) ( sMedianFuncData->totalData +
                                                   sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );

            // BUG-41224 chage makeNumeric function.
            IDE_TEST( mtc::makeNumeric( sMultiplyValue1,
                                        (SDouble)sCeilRowNum - sRowNum )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::multiplyFloat( sFloorResult,
                                          MTD_FLOAT_PRECISION_MAXIMUM,
                                          sFloatArgument1,
                                          sMultiplyValue1 )
                      != IDE_SUCCESS );

            sFloatArgument2 = (mtdNumericType *) ( sMedianFuncData->totalData +
                                                   sMedianFuncData->rowSize * ( sCeilRowNum - 1 ) );

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

IDE_RC mtfMedianFinalizeDouble( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

static const mtcExecute mtfMedianExecuteDouble = {
    mtfMedianInitialize,
    mtfMedianAggregate,
    mtfMedianMerge,
    mtfMedianFinalizeDouble,
    mtfMedianCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMedianEstimateDouble( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt          ,
                                mtcCallBack * aCallBack )
{
    static const mtdModule * sModules[1] = {
        &mtdDouble
    };

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMedianExecuteDouble;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Median function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfMedianFuncData * ),
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
        IDE_SET(ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianFinalizeDouble( mtcNode     * aNode,
                                mtcStack    * ,
                                SInt          ,
                                void        * ,
                                mtcTemplate * aTemplate )
{
    const mtcColumn      * sColumn            = NULL;
    mtdBinaryType        * sBinary            = NULL;
    UChar                * sRow               = NULL;
    mtdDoubleType        * sResult            = NULL;
    mtfFuncDataBasicInfo * sFuncDataBasicInfo = NULL;
    mtfMedianFuncData    * sMedianFuncData    = NULL;
    mtdDoubleType          sCeilResult        = 0.0;
    mtdDoubleType          sFloorResult       = 0.0;
    SDouble                sRowNum            = 0.0;
    SLong                  sCeilRowNum        = ID_LONG(0);
    SLong                  sFloorRowNum       = ID_LONG(0);

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sMedianFuncData = *((mtfMedianFuncData **)sBinary->mValue);

    sResult = (mtdDoubleType *)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------

    // chunk를 하나로 합쳐  data를 생성한다.
    IDE_TEST( makeTotalDataMedianFuncData( sFuncDataBasicInfo,
                                           sMedianFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortMedianFuncDataDouble( sFuncDataBasicInfo,
                                        sMedianFuncData )
              != IDE_SUCCESS );

    //------------------------------------------
    // 연산 수행
    //------------------------------------------

    if ( sMedianFuncData->totalCount > 0 )
    {
        // RN(중간 row위치)을 구한다.
        sRowNum = 1 + ( 0.5 * ( sMedianFuncData->totalCount - 1 ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            *sResult = *(mtdDoubleType *) ( sMedianFuncData->totalData +
                                            sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );
        }
        else
        {
            *sResult = *(mtdDoubleType *) ( sMedianFuncData->totalData +
                                            sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );

            sFloorResult = ( sCeilRowNum - sRowNum ) * (*sResult);

            *sResult = *(mtdDoubleType *) ( sMedianFuncData->totalData +
                                            sMedianFuncData->rowSize * ( sCeilRowNum - 1 ) );

            sCeilResult = ( sRowNum - sFloorRowNum ) * (*sResult);

            *sResult = sFloorResult + sCeilResult;
        }
    }
    else
    {
        // set null
        *sResult = *(mtdDoubleType *) mtdDouble.staticNull;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: BIGINT */

IDE_RC mtfMedianFinalizeBigint( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

static const mtcExecute mtfMedianExecuteBigint = {
    mtfMedianInitialize,
    mtfMedianAggregate,
    mtfMedianMerge,
    mtfMedianFinalizeBigint,
    mtfMedianCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMedianEstimateBigint( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt          ,
                                mtcCallBack * aCallBack )
{
    static const mtdModule * sModules[1] = {
        &mtdBigint
    };

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMedianExecuteBigint;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Median function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfMedianFuncData * ),
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
        IDE_SET(ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianFinalizeBigint( mtcNode     * aNode,
                                mtcStack    * ,
                                SInt          ,
                                void        * ,
                                mtcTemplate * aTemplate )
{
    const mtcColumn      * sColumn            = NULL;
    mtdBinaryType        * sBinary            = NULL;
    UChar                * sRow               = NULL;
    mtdDoubleType        * sResult            = NULL;
    mtfFuncDataBasicInfo * sFuncDataBasicInfo = NULL;
    mtfMedianFuncData    * sMedianFuncData    = NULL;
    mtdDoubleType          sCeilResult        = 0.0;
    mtdDoubleType          sFloorResult       = 0.0;
    SDouble                sRowNum            = 0.0;
    SLong                  sCeilRowNum        = ID_LONG(0);
    SLong                  sFloorRowNum       = ID_LONG(0);

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sMedianFuncData = *((mtfMedianFuncData **)sBinary->mValue);

    sResult = (mtdDoubleType *)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------

    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataMedianFuncData( sFuncDataBasicInfo,
                                           sMedianFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortMedianFuncDataBigint( sFuncDataBasicInfo,
                                        sMedianFuncData )
              != IDE_SUCCESS );

    //------------------------------------------
    // 연산 수행
    //------------------------------------------

    if ( sMedianFuncData->totalCount > 0 )
    {
        // RN(중간 row위치)을 구한다.
        sRowNum = 1 + ( 0.5 * ( sMedianFuncData->totalCount - 1 ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            *sResult = *(mtdBigintType *) ( sMedianFuncData->totalData +
                                            sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );
        }
        else
        {
            *sResult = *(mtdBigintType *) ( sMedianFuncData->totalData +
                                            sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );

            sFloorResult = ( sCeilRowNum - sRowNum ) * (*sResult);

            *sResult = *(mtdBigintType *) ( sMedianFuncData->totalData +
                                            sMedianFuncData->rowSize * ( sCeilRowNum - 1 ) );

            sCeilResult = ( sRowNum - sFloorRowNum ) * (*sResult);

            *sResult = sFloorResult + sCeilResult;
        }
    }
    else
    {
        // set null
        *sResult = *(mtdDoubleType *)mtdDouble.staticNull;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-43821 DATE Type Support */
/* ZONE: DATE */

IDE_RC mtfMedianFinalizeDate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

static const mtcExecute mtfMedianExecuteDate = {
    mtfMedianInitialize,
    mtfMedianAggregate,
    mtfMedianMerge,
    mtfMedianFinalizeDate,
    mtfMedianCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMedianEstimateDate( mtcNode     * aNode,
                              mtcTemplate * aTemplate,
                              mtcStack    * aStack,
                              SInt,
                              mtcCallBack * aCallBack )
{
    static const mtdModule * sModules[1] = {
        &mtdDate
    };

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMedianExecuteDate;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Median function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF( mtfMedianFuncData * ),
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
        IDE_SET(ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMedianFinalizeDate( mtcNode      * aNode,
                              mtcStack     * ,
                              SInt           ,
                              void         * ,
                              mtcTemplate  * aTemplate )
{
    const mtcColumn      * sColumn                 = NULL;
    mtdBinaryType        * sBinary                 = NULL;
    UChar                * sRow                    = NULL;
    mtdDateType          * sDateArgument           = NULL;
    mtdDateType          * sResult                 = NULL;
    mtdIntervalType        sIntervalResult;
    mtdIntervalType        sIntervalCeilResult;
    mtdIntervalType        sIntervalFloorResult;
    UChar                  sResultBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sCeilBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloorBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloatBuff1[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloatBuff2[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloatRoundBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar                  sFloatZeroBuff[20]      = { 1, 128, 0, };
    mtdNumericType       * sNumericResult          = (mtdNumericType *)sResultBuff;
    mtdNumericType       * sNumericCeilResult      = (mtdNumericType *)sCeilBuff;
    mtdNumericType       * sNumericFloorResult     = (mtdNumericType *)sFloorBuff;
    mtdNumericType       * sNumericArgument        = (mtdNumericType *)sFloatBuff1;
    mtdNumericType       * sMultiplyValue          = (mtdNumericType *)sFloatBuff2;
    mtdNumericType       * sNumericRoundResult     = (mtdNumericType *)sFloatRoundBuff;
    mtdNumericType       * sNumericZero            = (mtdNumericType *)sFloatZeroBuff;
    mtfFuncDataBasicInfo * sFuncDataBasicInfo      = NULL;
    mtfMedianFuncData    * sMedianFuncData = NULL;
    SDouble                sRowNum                 = 0.0;
    SLong                  sCeilRowNum             = ID_LONG(0);
    SLong                  sFloorRowNum            = ID_LONG(0);

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sBinary = (mtdBinaryType *)(sRow + sColumn[1].column.offset);

    sFuncDataBasicInfo = aTemplate->funcData[aNode->info];
    sMedianFuncData = *((mtfMedianFuncData **)sBinary->mValue);

    sResult = (mtdDateType *)(sRow + sColumn[0].column.offset);

    //------------------------------------------
    // sort
    //------------------------------------------

    // chunk를 하나로 합쳐 total data를 생성한다.
    IDE_TEST( makeTotalDataMedianFuncData( sFuncDataBasicInfo,
                                           sMedianFuncData )
              != IDE_SUCCESS );

    // sort total data
    IDE_TEST( sortMedianFuncDataDate( sFuncDataBasicInfo,
                                      sMedianFuncData )
              != IDE_SUCCESS );

    //------------------------------------------
    // 연산 수행
    //------------------------------------------

    if ( sMedianFuncData->totalCount > 0 )
    {
        // RN( 중간 row위치) 을 구한다.
        sRowNum = ( 1 + ( 0.5 * ( sMedianFuncData->totalCount - 1 ) ) );

        // ceil row num
        sCeilRowNum  = idlOS::ceil( sRowNum );

        // floor row num
        sFloorRowNum = idlOS::floor( sRowNum );

        // row num = ceil, row num = floor 같은 경우 또는 row num 총개가 홀수.
        if ( ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sCeilRowNum ) ) &&
             ( MEDIAN_FLOATING_POINT_EQ( sRowNum, (SDouble)sFloorRowNum ) ) )
        {
            sDateArgument = (mtdDateType *)( sMedianFuncData->totalData +
                                             sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );

            idlOS::memcpy( sResult, sDateArgument, ID_SIZEOF(mtdDateType) );
        }
        else
        {
            sDateArgument = (mtdDateType *)( sMedianFuncData->totalData +
                                             sMedianFuncData->rowSize * ( sFloorRowNum - 1 ) );

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

            sDateArgument = (mtdDateType *)( sMedianFuncData->totalData +
                                             sMedianFuncData->rowSize * ( sCeilRowNum - 1 ) );

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

IDE_RC makeMedianFuncData( mtfFuncDataBasicInfo  * aFuncDataInfo,
                           mtcNode               * /*aNode*/,
                           mtfMedianFuncData    ** aMedianFuncData )
{
    mtfMedianFuncData * sMedianFuncData = NULL;

    // function data
    IDU_FIT_POINT_RAISE( "makeMedianFuncData::cralloc::sMedianFuncData",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->cralloc( ID_SIZEOF(mtfMedianFuncData),
                                                       (void **) & sMedianFuncData )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    // return
    *aMedianFuncData = sMedianFuncData;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC allocMedianFuncDataBuffer( mtfFuncDataBasicInfo * aFuncDataInfo,
                                  mtcStack             * aStack,
                                  mtfMedianFuncData    * aMedianFuncData )
{
    mtfMedianFuncDataBuffer * sBuffer  = NULL;
    UInt                      sRowSize = 0;

    if ( aMedianFuncData->rowSize == 0 )
    {
        if ( ( aStack[1].column->module == &mtdFloat ) ||
             ( aStack[1].column->module == &mtdNumeric ) )
        {
            sRowSize = aStack[1].column->column.size;
        }
        else if ( aStack[1].column->module == &mtdDouble )
        {
            sRowSize = ID_SIZEOF(mtdDoubleType);
        }
        else if ( aStack[1].column->module == &mtdBigint )
        {
            sRowSize = ID_SIZEOF(mtdBigintType);
        }
        /* BUG-43821 DATE Type Support */
        else if ( aStack[1].column->module == &mtdDate )
        {
            sRowSize = ID_SIZEOF(mtdDateType);
        }
        else
        {
            IDE_RAISE( ERR_ARGUMENT_TYPE );
        }

        aMedianFuncData->rowSize = sRowSize;
    }
    else
    {
        // Nothing to do
    }

    IDU_FIT_POINT_RAISE( "allocMedianFuncDataBuffer::alloc::sBuffer",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc( ID_SIZEOF(mtfMedianFuncDataBuffer) +
                                                     aMedianFuncData->rowSize * MTF_MEDIAN_BUFFER_MAX,
                                                     (void **) & sBuffer )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    sBuffer->data = (UChar *)sBuffer + ID_SIZEOF(mtfMedianFuncDataBuffer);
    sBuffer->idx  = 0;
    sBuffer->next = aMedianFuncData->list;

    // link
    aMedianFuncData->list = sBuffer;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_TYPE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "allocMedianFuncDataBuffer",
                                  "invalid arguemnt type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC copyToMedianFuncDataBuffer( mtfFuncDataBasicInfo * aFuncDataInfo,
                                   mtcStack             * aStack,
                                   mtfMedianFuncData    * aMedianFuncData )
{
    void   * sValue = NULL;
    idBool   sAlloc = ID_FALSE;

    if ( aMedianFuncData->list == NULL )
    {
        sAlloc = ID_TRUE;
    }
    else
    {
        if ( aMedianFuncData->list->idx == MTF_MEDIAN_BUFFER_MAX )
        {
            sAlloc = ID_TRUE;
        }
        else
        {
            // Nothing to do
        }
    }

    if ( sAlloc == ID_TRUE )
    {
        IDE_TEST( allocMedianFuncDataBuffer( aFuncDataInfo,
                                             aStack,
                                             aMedianFuncData )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    sValue = aMedianFuncData->list->data + aMedianFuncData->rowSize * aMedianFuncData->list->idx;

    if ( ( aStack[1].column->module == &mtdFloat ) ||
         ( aStack[1].column->module == &mtdNumeric ) )
    {
        idlOS::memcpy( sValue, aStack[1].value,
                       ID_SIZEOF(UChar) + ((mtdNumericType *)aStack[1].value)->length );
    }
    else if ( aStack[1].column->module == &mtdDouble )
    {
        *(mtdDoubleType *)sValue = *(mtdDoubleType *)aStack[1].value;
    }
    else if ( aStack[1].column->module == &mtdBigint )
    {
        *(mtdBigintType *)sValue = *(mtdBigintType *)aStack[1].value;
    }
    /* BUG-43821 DATE Type Support */
    else if ( aStack[1].column->module == &mtdDate )
    {
        idlOS::memcpy( sValue, aStack[1].value, ID_SIZEOF(mtdDateType) );
    }
    else
    {
        // Nothing to do
    }

    aMedianFuncData->list->idx++;
    aMedianFuncData->totalCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC makeTotalDataMedianFuncData( mtfFuncDataBasicInfo * aFuncDataInfo,
                                    mtfMedianFuncData    * aMedianFuncData )
{
    mtfMedianFuncDataBuffer * sList  = NULL;
    UChar                   * sValue = NULL;

    if ( aMedianFuncData->totalCount > 0 )
    {
        IDE_DASSERT( aMedianFuncData->list != NULL );

        if ( aMedianFuncData->list->next != NULL )
        {
            IDU_FIT_POINT_RAISE( "makeTotalDataMedianFuncData::alloc::totalData",
                                 ERR_MEMORY_ALLOCATION );
            IDE_TEST_RAISE( aFuncDataInfo->memoryMgr->alloc( aMedianFuncData->rowSize *
                                                             aMedianFuncData->totalCount,
                                                             (void **)&(aMedianFuncData->totalData) )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

            sValue = aMedianFuncData->totalData;

            for ( sList = aMedianFuncData->list;
                  sList != NULL;
                  sList = sList->next )
            {
                idlOS::memcpy( sValue,
                               sList->data,
                               aMedianFuncData->rowSize * sList->idx );

                sValue += aMedianFuncData->rowSize * sList->idx;
            }
        }
        else
        {
            // chunk가 1개인 경우
            aMedianFuncData->totalData = aMedianFuncData->list->data;
        }
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


extern "C" SInt compareMedianFuncDataFloatAsc( const void * aElem1,
                                               const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdFloat.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt compareMedianFuncDataDoubleAsc( const void * aElem1,
                                                const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDouble.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

extern "C" SInt compareMedianFuncDataBigintAsc( const void * aElem1,
                                                const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdBigint.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

/* BUG-43821 DATE Type Support */
extern "C" SInt compareMedianFuncDataDateAsc( const void * aElem1,
                                              const void * aElem2 )
{
    mtdValueInfo  sValueInfo1;
    mtdValueInfo  sValueInfo2;

    sValueInfo1.value = aElem1;
    sValueInfo2.value = aElem2;

    return mtdDate.logicalCompare[0]( &sValueInfo1, &sValueInfo2 );
}

IDE_RC sortMedianFuncDataFloat( mtfFuncDataBasicInfo * /*aFuncDataInfo*/,
                                mtfMedianFuncData    * aMedianFuncData )
{
    if ( aMedianFuncData->totalCount > 1 )
    {
        idlOS::qsort( aMedianFuncData->totalData,
                      aMedianFuncData->totalCount,
                      aMedianFuncData->rowSize,
                      compareMedianFuncDataFloatAsc );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;
}

IDE_RC sortMedianFuncDataDouble( mtfFuncDataBasicInfo * /*aFuncDataInfo*/,
                                 mtfMedianFuncData    * aMedianFuncData )
{
    if ( aMedianFuncData->totalCount > 1 )
    {
        idlOS::qsort( aMedianFuncData->totalData,
                      aMedianFuncData->totalCount,
                      aMedianFuncData->rowSize,
                      compareMedianFuncDataDoubleAsc );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;
}

IDE_RC sortMedianFuncDataBigint( mtfFuncDataBasicInfo * /*aFuncDataInfo*/,
                                 mtfMedianFuncData    * aMedianFuncData )
{
    if ( aMedianFuncData->totalCount > 1 )
    {
        idlOS::qsort( aMedianFuncData->totalData,
                      aMedianFuncData->totalCount,
                      aMedianFuncData->rowSize,
                      compareMedianFuncDataBigintAsc );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;
}

/* BUG-43821 DATE Type Support */
IDE_RC sortMedianFuncDataDate( mtfFuncDataBasicInfo * /*aFuncDataInfo*/,
                               mtfMedianFuncData    * aMedianFuncData )
{
    if ( aMedianFuncData->totalCount > 1 )
    {
        idlOS::qsort( aMedianFuncData->totalData,
                      aMedianFuncData->totalCount,
                      aMedianFuncData->rowSize,
                      compareMedianFuncDataDateAsc );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

