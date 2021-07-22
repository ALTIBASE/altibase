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
 * $Id: mtfCovarSamp.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

/**
 * COVAR_SAMP( expr1, expr2 )
 *
 * 계산식은 다음과 같다.
 *
 * ( SUM( expr1 * expr2 ) - SUM( expr1 ) * SUM( expr2 ) / N ) / ( N - 1 )
 *
 * N 은 expr1 과 expr2 가 둘다 NULL 이 아닌 값의 count를 말한다.
 */
extern mtfModule mtfCovarSamp;

extern mtdModule mtdDouble;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBigint;

static mtcName mtfCovarSampFunctionName[1] = {
    { NULL, 10, (void*)"COVAR_SAMP" }
};

static IDE_RC mtfCovarSampInitialize( void );

static IDE_RC mtfCovarSampFinalize( void );

static IDE_RC mtfCovarSampEstimate( mtcNode     * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    mtcCallBack * aCallBack );

mtfModule mtfCovarSamp = {
    5 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfCovarSampFunctionName,
    NULL,
    mtfCovarSampInitialize,
    mtfCovarSampFinalize,
    mtfCovarSampEstimate
};

static IDE_RC mtfCovarSampEstimateDouble( mtcNode     * aNode,
                                          mtcTemplate * aTemplate,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          mtcCallBack * aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfNN[1] = {
    { NULL,    mtfCovarSampEstimateDouble }
};

static mtfSubModule* mtfGroupTable[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM] = {
/*               MISC   TEXT   NUMBE  DATE   INTER */
/* MISC     */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* TEXT     */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* NUMBER   */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* DATE     */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* INTERVAL */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN }
};

static mtfSubModule *** mtfTable = NULL;

IDE_RC mtfCovarSampInitialize( void )
{
    IDE_TEST( mtf::initializeComparisonTemplate( &mtfTable,
                                                 mtfGroupTable,
                                                 mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCovarSampFinalize( void )
{
    IDE_TEST( mtf::finalizeComparisonTemplate( &mtfTable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCovarSampEstimate( mtcNode     * aNode,
                             mtcTemplate * aTemplate,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             mtcCallBack * aCallBack )
{
    const mtfSubModule * sSubModule = NULL;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getSubModule2Args( &sSubModule,
                                      mtfTable,
                                      aStack[1].column->module->no,
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
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE  );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

IDE_RC mtfCovarSampInitializeDouble( mtcNode     * aNode,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     void        * aInfo,
                                     mtcTemplate * aTemplate );

IDE_RC mtfCovarSampAggregateDouble( mtcNode     * aNode,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    void        * aInfo,
                                    mtcTemplate * aTemplate );

IDE_RC mtfCovarSampMergeDouble( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

IDE_RC mtfCovarSampFinalizeDouble( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   void        * aInfo,
                                   mtcTemplate * aTemplate );

IDE_RC mtfCovarSampCalculateDouble( mtcNode     * aNode,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    void        * aInfo,
                                    mtcTemplate * aTemplate );

static const mtcExecute mtfCovarSampExecuteDouble = {
    mtfCovarSampInitializeDouble,
    mtfCovarSampAggregateDouble,
    mtfCovarSampMergeDouble,
    mtfCovarSampFinalizeDouble,
    mtfCovarSampCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCovarSampEstimateDouble( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          /*aRemain*/,
                                   mtcCallBack * aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdDouble,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfCovarSampExecuteDouble;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // SUM( ARG1 * ARG 2 )
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // SUM( ARG1 )
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // SUM( ARG2 )
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // N -- Not Null Pair Count
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 4,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList    ) ||
                    ( aStack[1].column->module == &mtdBoolean ) ||
                    ( aStack[2].column->module == &mtdList    ) ||
                    ( aStack[2].column->module == &mtdBoolean ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCovarSampInitializeDouble( mtcNode     * aNode,
                                     mtcStack    * /*aStack*/,
                                     SInt          /*aRemain*/,
                                     void        * /*aInfo*/,
                                     mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    UChar           * sValue = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sValue = (UChar*) aTemplate->rows[aNode->table].row
                      + sColumn->column.offset;

    mtdDouble.null( sColumn, (void *)sValue );

    // SUM( ARG1 * ARG 2 )
    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                      + sColumn[1].column.offset) = 0;

    // SUM( ARG1 )
    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                      + sColumn[2].column.offset) = 0;

    // SUM( ARG2 )
    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                      + sColumn[3].column.offset) = 0;

    // Not Null Pair Count
    *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row
                               + sColumn[4].column.offset) = 0;

    return IDE_SUCCESS;
}

IDE_RC mtfCovarSampAggregateDouble( mtcNode     * aNode,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    void        * aInfo,
                                    mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    mtdDoubleType   * sMultiply = NULL;
    mtdDoubleType   * sArg1Sum  = NULL;
    mtdDoubleType   * sArg2Sum  = NULL;
    mtdDoubleType   * sArg1     = NULL;
    mtdDoubleType   * sArg2     = NULL;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    // mtdDouble.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if ( ( ( *(ULong*)(aStack[1].value) & MTD_DOUBLE_EXPONENT_MASK )
           != MTD_DOUBLE_EXPONENT_MASK ) &&
         ( ( *(ULong*)(aStack[2].value) & MTD_DOUBLE_EXPONENT_MASK )
           != MTD_DOUBLE_EXPONENT_MASK ) )

    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

        sMultiply = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                      + sColumn[1].column.offset);
        sArg1Sum  = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                      + sColumn[2].column.offset);
        sArg2Sum  = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                      + sColumn[3].column.offset);

        sArg1 = (mtdDoubleType *)aStack[1].value;
        sArg2 = (mtdDoubleType *)aStack[2].value;

        *sMultiply += (*sArg1) * (*sArg2);
        *sArg1Sum  += *sArg1;
        *sArg2Sum  += *sArg2;

        *(mtdBigintType *) ((UChar *)aTemplate->rows[aNode->table].row
                           + sColumn[4].column.offset ) += 1;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCovarSampMergeDouble( mtcNode     * aNode,
                                mtcStack    * /*aStack*/,
                                SInt          /*aRemain*/,
                                void        * aInfo,
                                mtcTemplate * aTemplate )
{
    mtcColumn   * sColumn = NULL;
    UChar       * sDstRow = NULL;
    UChar       * sSrcRow = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;

    // SUM( ARG1 * ARG 2 )
    *(mtdDoubleType*)( sDstRow + sColumn[1].column.offset) +=
    *(mtdDoubleType*)( sSrcRow + sColumn[1].column.offset);

    // SUM( ARG1 )
    *(mtdDoubleType*)( sDstRow + sColumn[2].column.offset) +=
    *(mtdDoubleType*)( sSrcRow + sColumn[2].column.offset);

    // SUM( ARG2 )
    *(mtdDoubleType*)( sDstRow + sColumn[3].column.offset) +=
    *(mtdDoubleType*)( sSrcRow + sColumn[3].column.offset);

    // Not Null Pair Count
    *(mtdBigintType*)( sDstRow + sColumn[4].column.offset) +=
    *(mtdBigintType*)( sSrcRow + sColumn[4].column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfCovarSampFinalizeDouble( mtcNode     * aNode,
                                   mtcStack    * /*aStack*/,
                                   SInt          /*aRemain*/,
                                   void        * /*aInfo*/,
                                   mtcTemplate * aTemplate )
{
    mtcColumn     * sColumn = NULL;
    mtdDoubleType * sResult = NULL;
    mtdDoubleType * sMultiply = NULL;
    mtdDoubleType * sArg1Sum = NULL;
    mtdDoubleType * sArg2Sum = NULL;
    mtdDoubleType   sTemp;
    mtdBigintType   sN;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sN = *(mtdBigintType *) ((UChar *)aTemplate->rows[aNode->table].row
                            + sColumn[4].column.offset );

    if ( sN > 1 )
    {
        sResult    = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                       + sColumn[0].column.offset);
        sMultiply  = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                       + sColumn[1].column.offset);
        sArg1Sum   = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                       + sColumn[2].column.offset);
        sArg2Sum   = (mtdDoubleType *) ((UChar *) aTemplate->rows[aNode->table].row
                                       + sColumn[3].column.offset);

        sTemp   = (*sArg1Sum) * ( (*sArg2Sum) / (mtdDoubleType)sN );
        *sResult = ( (*sMultiply) - sTemp ) / ( (mtdDoubleType)(sN - 1) );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;
}

IDE_RC mtfCovarSampCalculateDouble( mtcNode     * aNode,
                                    mtcStack    * aStack,
                                    SInt          /*aRemain*/,
                                    void        * /*aInfo*/,
                                    mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
}

