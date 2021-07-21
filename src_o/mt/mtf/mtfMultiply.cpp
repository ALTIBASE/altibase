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
 * $Id: mtfMultiply.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtuProperty.h>
#include <mtdTypes.h>

extern mtfModule mtfMultiply;

extern mtxModule mtxMultiply; /* PROJ-2632 */

static mtcName mtfMultiplyFunctionName[1] = {
    { NULL, 1, (void*)"*" }
};

static IDE_RC mtfMultiplyInitialize( void );

static IDE_RC mtfMultiplyFinalize( void );

static IDE_RC mtfMultiplyEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfMultiply = {
    1|MTC_NODE_OPERATOR_FUNCTION|
        MTC_NODE_PRINT_FMT_MISC|
        MTC_NODE_COMMUTATIVE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfMultiplyFunctionName,
    NULL,
    mtfMultiplyInitialize,
    mtfMultiplyFinalize,
    mtfMultiplyEstimate
};

// fix BUG-13757
/*
static IDE_RC mtfMultiplyEstimateInteger( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );
*/

// To Remove Warning
/*
static IDE_RC mtfMultiplyEstimateSmallint( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );
*/

// fix BUG-13757
/*
static IDE_RC mtfMultiplyEstimateBigint( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );
*/

static IDE_RC mtfMultiplyEstimateFloat( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

static IDE_RC mtfMultiplyEstimateReal( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

static IDE_RC mtfMultiplyEstimateDouble( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

// To Fix PR-8722
//static mtfSubModule mtfNN[6] = {
//    { mtfNN+1, mtfMultiplyEstimateInteger  },
//    { mtfNN+2, mtfMultiplyEstimateSmallint },
//    { mtfNN+3, mtfMultiplyEstimateBigint   },
//    { mtfNN+4, mtfMultiplyEstimateFloat    },
//    { mtfNN+5, mtfMultiplyEstimateReal     },
//    { NULL,    mtfMultiplyEstimateDouble   }
//};

// To Fix PR-8722
// [Overflow(Underflow)-PRONE 연산의 제거]
// A4 에서는 Parsing 단계에서 숫자형에 대한 Type 결정시
// 해당 숫자가 포함될 수 있는 최소 범위 내의 Data Type으로
// 결정된다.  ( ex) 1 ==> SMALLINT, 100000 ==> INTEGER )
// 이 때, SMALLINT * SMALLINT 값 등을 SMALLINT로 처리할 경우
// (24*60*60)과 같은 값들은 SMALLINT 값의 범위를 벗어나
// 잘못된 결과값을 갖는 경우가 많다.
// 이러한 연산은 DATA Type에 대한 Second값등을 증가시키기 위해 사용되는 데,
// A3에서는 Parsing 단계에서 모든 정수형을 INTEGER로 결정하기 때문에
// 문제가 발생하지 않았다.
// 이러한 문제를 해결하기 위하여 SMALLINT만을 이용한 연산 방식은 제거한다.
// 물론, 이렇게 해결한다 하더라도 A3와 마찬가지로
// INTEGER * INTEGER 등이 INTEGER의 범위를 벗어날 수 있다.

//static mtfSubModule mtfNN[5] = {
//    { mtfNN+1, mtfMultiplyEstimateInteger  },
//    { mtfNN+2, mtfMultiplyEstimateBigint   },
//    { mtfNN+3, mtfMultiplyEstimateFloat    },
//    { mtfNN+4, mtfMultiplyEstimateReal     },
//    { NULL,    mtfMultiplyEstimateDouble   }
//};

// fix BUG-13757
// integer계열의 곱하기 연산결과를 double타입으로 처리
// 
// (1) 기존정책
//     smallint * smallint = integer
//     integer  * integer  = integer
//     bigint   * bigint   = bigint
//
//     a3, a4에서 이렇게 정책을 결정하게 된 이유는,
//     인덱스 활용도를 높이기 위함임
//     예) i1이 integer column일 경우,
//         A. A2에서는
//            i1 = 3 * 3은 integer = double(?)이므로 인덱스를 탈 수 없음
//         B. A3, A4에서는
//            i1 = 3 * 3은 integer = integer이므로 인덱스를 탈 수 있음.
//
// (2) 변경된정책
//     smallint * smallint = double
//     integer  * integer  = double
//     bigint   * bigint   = double
//
// 정책변경으로 인한 문제점과 타당성
// (1) 문제점
//     연산결과를 double type으로 conversion하는 비용이 발생함.
// (2) 타당성
//     A. 인덱스 활용은 여전히 유지됨
//        동일계열 데이타타입의 인덱스활용도개선(PROJ-1364)으로 인해
//        integer = double인 경우에도 인덱스를 탈 수 있음.
//     B. isql상의 표기 문제
//        곱하기연산의 결과가 XXXX.0 으로 나오는 경우,
//        .0을 제거하고  XXXX로 출력됨.
//        예) select 3*3 from t1일 경우, 9.0이 아닌 9로 출력됨.
//     C. client program에서 곱하기 연산결과를 integer type변수에 저장시,
//        .곱하기 연산결과가 integer범위내에 들 경우, 정상동작
//        .곱하기 연산결과가 integer범위를 넘을 경우,client program에서 에러남
//         예) (Error : [-1] The data value could not be converted to 
//                      the data type specified by TargetType in SQLBindCol)

static mtfSubModule mtfNN[3] = {
    { mtfNN+1, mtfMultiplyEstimateFloat    },
    { mtfNN+2, mtfMultiplyEstimateReal     },
    { NULL,    mtfMultiplyEstimateDouble   }
};

static mtfSubModule* mtfGroupTable[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM] = {
/*               MISC   TEXT   NUMBE  DATE   INTER */
/* MISC     */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* TEXT     */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* NUMBER   */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* DATE     */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN },
/* INTERVAL */ { mtfNN, mtfNN, mtfNN, mtfNN, mtfNN }
};

// BUG-41994
// high precision용 group table
static mtfSubModule mtfNP[1] = {
    { NULL, mtfMultiplyEstimateFloat }
};

static mtfSubModule* mtfGroupTableHighPrecision[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM] = {
/*               MISC   TEXT   NUMBE  DATE   INTER */
/* MISC     */ { mtfNP, mtfNP, mtfNP, mtfNP, mtfNP },
/* TEXT     */ { mtfNP, mtfNP, mtfNP, mtfNP, mtfNP },
/* NUMBER   */ { mtfNP, mtfNP, mtfNP, mtfNP, mtfNP },
/* DATE     */ { mtfNP, mtfNP, mtfNP, mtfNP, mtfNP },
/* INTERVAL */ { mtfNP, mtfNP, mtfNP, mtfNP, mtfNP }
};

/* BUG-46195 */
static mtfSubModule mtfNF[1] = {
    { NULL, mtfMultiplyEstimateFloat }
};

static mtfSubModule * mtfGroupTableMaxPrecision[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM] = {
/*               MISC   TEXT   NUMBE  DATE   INTER */
/* MISC     */ { mtfNF, mtfNF, mtfNF, mtfNF, mtfNF },
/* TEXT     */ { mtfNF, mtfNF, mtfNF, mtfNF, mtfNF },
/* NUMBER   */ { mtfNF, mtfNF, mtfNF, mtfNF, mtfNF },
/* DATE     */ { mtfNF, mtfNF, mtfNF, mtfNF, mtfNF },
/* INTERVAL */ { mtfNF, mtfNF, mtfNF, mtfNF, mtfNF }
};

static mtfSubModule*** mtfTable = NULL;
static mtfSubModule*** mtfTableHighPrecision = NULL;

/* BUG-46195 */
static mtfSubModule*** mtfTableMaxPrecision = NULL;

IDE_RC mtfMultiplyInitialize( void )
{
    IDE_TEST( mtf::initializeComparisonTemplate( &mtfTable,
                                                 mtfGroupTable,
                                                 mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeComparisonTemplate( &mtfTableHighPrecision,
                                                 mtfGroupTableHighPrecision,
                                                 mtfXX )
              != IDE_SUCCESS );

    /* BUG-46195 */
    IDE_TEST( mtf::initializeComparisonTemplate( &mtfTableMaxPrecision,
                                                 mtfGroupTableMaxPrecision,
                                                 mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMultiplyFinalize( void )
{
    IDE_TEST( mtf::finalizeComparisonTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeComparisonTemplate( &mtfTableHighPrecision )
              != IDE_SUCCESS );

    /* BUG-46195 */
    IDE_TEST( mtf::finalizeComparisonTemplate( &mtfTableMaxPrecision )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfMultiplyEstimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            mtcCallBack* aCallBack )
{
    const mtfSubModule   * sSubModule;
    mtfSubModule       *** sTable;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // BUG-41994
    aTemplate->arithmeticOpModeRef = ID_TRUE;
    if ( aTemplate->arithmeticOpMode == MTC_ARITHMETIC_OPERATION_PRECISION )
    {
        sTable = mtfTableHighPrecision;
    }
    /* BUG-46195 */
    else if ( aTemplate->arithmeticOpMode == MTC_ARITHMETIC_OPERATION_MAX_PRECISION )
    {
        sTable = mtfTableMaxPrecision;
    }
    else
    {
        sTable = mtfTable;
    }

    IDE_TEST( mtf::getSubModule2Args( &sSubModule,
                                      sTable,
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

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: INTEGER */

extern mtdModule mtdInteger;

IDE_RC mtfMultiplyCalculateInteger( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfMultiplyExecuteInteger = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultiplyCalculateInteger,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultiplyEstimateInteger( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdInteger,
        &mtdInteger
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMultiplyExecuteInteger;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxMultiply.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdInteger.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMultiplyCalculateInteger( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    SLong sValue1;
    SLong sValue2;
    SLong sValue3;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( ( *(mtdIntegerType*)aStack[1].value == MTD_INTEGER_NULL ) ||
        ( *(mtdIntegerType*)aStack[2].value == MTD_INTEGER_NULL ) )
    {
        *(mtdIntegerType*)aStack[0].value = MTD_INTEGER_NULL;
    }
    else
    {
        sValue1 = *(mtdIntegerType*)aStack[1].value;
        sValue2 = *(mtdIntegerType*)aStack[2].value;
        sValue3 = sValue1 * sValue2;
        IDE_TEST_RAISE( sValue3 > MTD_INTEGER_MAXIMUM , ERR_OVERFLOW );
        IDE_TEST_RAISE( sValue3 < MTD_INTEGER_MINIMUM , ERR_OVERFLOW );

        *(mtdIntegerType*)aStack[0].value = sValue3;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: SMALLINT */

// To Remove Warning
/*
extern mtdModule mtdSmallint;

IDE_RC mtfMultiplyCalculateSmallint( 
                                    mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfMultiplyExecuteSmallint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultiplyCalculateSmallint,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultiplyEstimateSmallint( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt,
                                    mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdSmallint,
        &mtdSmallint
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTuple,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTuple[aNode->table].execute[aNode->column] = mtfMultiplyExecuteSmallint;
    
    IDE_TEST( mtdSmallint.estimate( aStack[0].column, 0, 0, 0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMultiplyCalculateSmallint( 
                             mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aTuple,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( mtdSmallint.isNull( aStack[1].column,
                            aStack[1].value ) ||
        mtdSmallint.isNull( aStack[2].column,
                            aStack[2].value )  )
    {
        mtdSmallint.null( aStack[0].column,
                          aStack[0].value );
    }
    else
    {
        *(mtdSmallintType*)aStack[0].value =
       *(mtdSmallintType*)aStack[1].value * *(mtdSmallintType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
*/

/* ZONE: BIGINT */

extern mtdModule mtdBigint;

IDE_RC mtfMultiplyCalculateBigint( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfMultiplyExecuteBigint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultiplyCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultiplyEstimateBigint( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdBigint,
        &mtdBigint
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMultiplyExecuteBigint;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxMultiply.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdBigint.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMultiplyCalculateBigint( mtcNode*     aNode,
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

    if( ( *(mtdBigintType*)aStack[1].value == MTD_BIGINT_NULL ) ||
        ( *(mtdBigintType*)aStack[2].value == MTD_BIGINT_NULL ) )
    {
        *(mtdBigintType*)aStack[0].value = MTD_BIGINT_NULL;
    }
    else
    {
        *(mtdBigintType*)aStack[0].value =
           *(mtdBigintType*)aStack[1].value * *(mtdBigintType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: FLOAT */

extern mtdModule mtdFloat;
    
IDE_RC mtfMultiplyCalculateFloat( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfMultiplyExecuteFloat = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultiplyCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultiplyEstimateFloat( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt,
                                 mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    mtc::makeFloatConversionModule( aStack + 1, &sModules[0] );
    mtc::makeFloatConversionModule( aStack + 2, &sModules[1] );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMultiplyExecuteFloat;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxMultiply.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdFloat.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtfMultiplyCalculateFloat()
 *
 * Argument :
 *    aStack - point of stack
 *
 * Description :
 *    Multiply Float : aStack[0] = aStack[1] * aStack[2] 
 *    실제적인 계산은 idaMultiply 이루어진다.
 * ---------------------------------------------------------------------------*/
 
IDE_RC mtfMultiplyCalculateFloat( mtcNode*     aNode,
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
    
    if( (mtdFloat.isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE) ||
        (mtdFloat.isNull( aStack[2].column,
                          aStack[2].value ) == ID_TRUE) )
    {
        mtdFloat.null( aStack[0].column,
                       aStack[0].value );
    }
    else
    {
        IDE_TEST( mtc::multiplyFloat( (mtdNumericType*)aStack[0].value,
                                      MTD_FLOAT_PRECISION_MAXIMUM,
                                      (mtdNumericType*)aStack[1].value,
                                      (mtdNumericType*)aStack[2].value )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: REAL */

extern mtdModule mtdReal;
    
IDE_RC mtfMultiplyCalculateReal( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfMultiplyExecuteReal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultiplyCalculateReal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultiplyEstimateReal( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt,
                                mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdReal,
        &mtdReal
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMultiplyExecuteReal;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxMultiply.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdReal.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdReal,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMultiplyCalculateReal(  mtcNode*     aNode,
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
    
    if( (mtdReal.isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE) ||
        (mtdReal.isNull( aStack[2].column,
                         aStack[2].value ) == ID_TRUE) )
    {
        mtdReal.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        *(mtdRealType*)aStack[0].value =
               *(mtdRealType*)aStack[1].value * *(mtdRealType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

extern mtdModule mtdDouble;
    
IDE_RC mtfMultiplyCalculateDouble( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfMultiplyExecuteDouble = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMultiplyCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMultiplyEstimateDouble( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* aCallBack )
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfMultiplyExecuteDouble;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxMultiply.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdDouble.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfMultiplyCalculateDouble( mtcNode*     aNode,
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
    
    if( (mtdDouble.isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE) ||
        (mtdDouble.isNull( aStack[2].column,
                           aStack[2].value ) == ID_TRUE) )
    {
        mtdDouble.null( aStack[0].column,
                        aStack[0].value );
    }
    else
    {
        *(mtdDoubleType*)aStack[0].value =
           *(mtdDoubleType*)aStack[1].value * *(mtdDoubleType*)aStack[2].value;

        // fix BUG-13757
        // double'0' * double'-1' = double'-0' 이 나오므로
        // -0 값을 0 값으로 처리하기 위한 코드임.
        /* PATCH(BEGIN): GNU MATH LIBRARY - REMOVE MINUS ZERO */
        if( *(mtdDoubleType*)aStack[0].value == 0 )
        {
            *(mtdDoubleType*)aStack[0].value = 0;
        }
        else
        {
            // Nothing To Do
        }
        /* PATCH(END): GNU MATH LIBRARY - REMOVE MINUS ZERO */
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
