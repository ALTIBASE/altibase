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
 * $Id: mtfSubtract2.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtuProperty.h>
#include <mtdTypes.h>

extern mtfModule mtfSubtract2;

extern mtxModule mtxSubtract; /* PROJ-2632 */

static mtcName mtfSubtract2FunctionName[1] = {
    { NULL, 1, (void*)"-" }
};

static IDE_RC mtfSubtract2Initialize( void );

static IDE_RC mtfSubtract2Finalize( void );

static IDE_RC mtfSubtract2Estimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfSubtract2 = {
    1|MTC_NODE_OPERATOR_FUNCTION|
        MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSubtract2FunctionName,
    NULL,
    mtfSubtract2Initialize,
    mtfSubtract2Finalize,
    mtfSubtract2Estimate
};

static IDE_RC mtfSubtract2EstimateInteger( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           mtcCallBack* aCallBack );

// To Remove Warning
/*
static IDE_RC mtfSubtract2EstimateSmallint( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );
*/

static IDE_RC mtfSubtract2EstimateBigint( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

static IDE_RC mtfSubtract2EstimateFloat( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

static IDE_RC mtfSubtract2EstimateReal( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

static IDE_RC mtfSubtract2EstimateDouble( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

static IDE_RC mtfSubtract2EstimateDate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

static IDE_RC mtfSubtract2EstimateDateInterval( mtcNode*     aNode,
                                                mtcTemplate* aTemplate,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                mtcCallBack* aCallBack );

static IDE_RC mtfSubtract2EstimateIntervalInterval( mtcNode*     aNode,
                                                    mtcTemplate* aTemplate,
                                                    mtcStack*    aStack,
                                                    SInt         aRemain,
                                                    mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

// To Fix PR-8722

//static mtfSubModule mtfNN[6] = {
//    { mtfNN+1, mtfSubtract2EstimateInteger  },
//    { mtfNN+2, mtfSubtract2EstimateSmallint },
//    { mtfNN+3, mtfSubtract2EstimateBigint   },
//    { mtfNN+4, mtfSubtract2EstimateFloat    },
//    { mtfNN+5, mtfSubtract2EstimateReal     },
//    { NULL,    mtfSubtract2EstimateDouble   }
//};

// To Fix PR-8722
// [Overflow(Underflow)-PRONE 연산의 제거]

static mtfSubModule mtfNN[5] = {
    { mtfNN+1, mtfSubtract2EstimateInteger  },
    { mtfNN+2, mtfSubtract2EstimateBigint   },
    { mtfNN+3, mtfSubtract2EstimateFloat    },
    { mtfNN+4, mtfSubtract2EstimateReal     },
    { NULL,    mtfSubtract2EstimateDouble   }
};

static mtfSubModule mtfDD[1] = {
    { NULL, mtfSubtract2EstimateDate }
};

static mtfSubModule mtfDI[1] = {
    { NULL, mtfSubtract2EstimateDateInterval }
};

// BUG-45505
static mtfSubModule mtfII[1] = {
    { NULL, mtfSubtract2EstimateIntervalInterval }
};

static mtfSubModule* mtfGroupTable[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM] = {
/*               MISC   TEXT   NUMBE  DATE   INTER */
/* MISC     */ { mtfXX, mtfXX, mtfXX, mtfXX, mtfXX },
/* TEXT     */ { mtfXX, mtfNN, mtfNN, mtfDD, mtfDI },
/* NUMBER   */ { mtfXX, mtfNN, mtfNN, mtfXX, mtfII },
/* DATE     */ { mtfXX, mtfDD, mtfDI, mtfDD, mtfDI },
/* INTERVAL */ { mtfXX, mtfXX, mtfII, mtfXX, mtfII }
};

// BUG-41994
// high precision용 group table
static mtfSubModule mtfNP[3] = {
    { mtfNP+1, mtfSubtract2EstimateInteger  },
    { mtfNP+2, mtfSubtract2EstimateBigint   },
    { NULL,    mtfSubtract2EstimateFloat    }
};

static mtfSubModule* mtfGroupTableHighPrecision[MTD_GROUP_MAXIMUM][MTD_GROUP_MAXIMUM] = {
/*               MISC   TEXT   NUMBE  DATE   INTER */
/* MISC     */ { mtfXX, mtfXX, mtfXX, mtfXX, mtfXX },
/* TEXT     */ { mtfXX, mtfNP, mtfNP, mtfDD, mtfDI },
/* NUMBER   */ { mtfXX, mtfNP, mtfNP, mtfXX, mtfII },
/* DATE     */ { mtfXX, mtfDD, mtfDI, mtfDD, mtfDI },
/* INTERVAL */ { mtfXX, mtfXX, mtfII, mtfXX, mtfII }
};

/* BUG-46195 */
static mtfSubModule mtfNF[1] = {
    { NULL, mtfSubtract2EstimateFloat }
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

IDE_RC mtfSubtract2Initialize( void )
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

IDE_RC mtfSubtract2Finalize( void )
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

IDE_RC mtfSubtract2Estimate( mtcNode*     aNode,
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

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: INTEGER */

extern mtdModule mtdInteger;

IDE_RC mtfSubtract2CalculateInteger( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteInteger = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateInteger,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateInteger( mtcNode*     aNode,
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteInteger;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfSubtract2CalculateInteger( mtcNode*     aNode,
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
        sValue3 = sValue1 - sValue2;
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

IDE_RC mtfSubtract2CalculateSmallint( 
                                    mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteSmallint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateSmallint,
    NULL,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateSmallint( mtcNode*     aNode,
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

    aTuple[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteSmallint;
    
    IDE_TEST( mtdSmallint.estimate( aStack[0].column, 0, 0, 0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSubtract2CalculateSmallint( 
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
       *(mtdSmallintType*)aStack[1].value - *(mtdSmallintType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

*/

/* ZONE: BIGINT */

extern mtdModule mtdBigint;

IDE_RC mtfSubtract2CalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteBigint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateBigint( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteBigint;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfSubtract2CalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
    SLong sValue1;
    SLong sValue2;

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
        /* BUG-42651 A bigint is not considered overflow */
        sValue1 = *(mtdBigintType *)aStack[1].value;
        sValue2 = *(mtdBigintType *)aStack[2].value;

        if ( ( sValue1 > 0 ) && ( sValue2 < 0 ) )
        {
            IDE_TEST_RAISE( ( MTD_BIGINT_MAXIMUM + sValue2 ) < sValue1, ERR_OVERFLOW );
        }
        else
        {
            if ( ( sValue1 < 0  ) && ( sValue2 > 0 ) )
            {
                IDE_TEST_RAISE( ( MTD_BIGINT_MINIMUM + sValue2 ) > sValue1, ERR_OVERFLOW );
            }
            else
            {
                /* Nothing to do */
            }
        }

        *(mtdBigintType*)aStack[0].value = sValue1 - sValue2;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: FLOAT */

extern mtdModule mtdFloat;
    
IDE_RC mtfSubtract2CalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteFloat = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateFloat( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteFloat;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

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
 *    IDE_RC mtfSubtract2CalculateFloat()
 *
 * Argument :
 *    aStack - point of stack
 *
 * Description :
 *    Subtract Float : aStack[0] = aStack[1] - aStack[2] 
 *    실제적인 계산은 idaSubtract 이루어진다.
 * ---------------------------------------------------------------------------*/

IDE_RC mtfSubtract2CalculateFloat( mtcNode*     aNode,
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
        IDE_TEST( mtc::subtractFloat( (mtdNumericType*)aStack[0].value,
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
    
IDE_RC mtfSubtract2CalculateReal( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteReal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateReal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateReal( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteReal;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfSubtract2CalculateReal( mtcNode*     aNode,
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
               *(mtdRealType*)aStack[1].value - *(mtdRealType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

extern mtdModule mtdDouble;
    
IDE_RC mtfSubtract2CalculateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteDouble = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateDouble( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteDouble;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfSubtract2CalculateDouble( mtcNode*     aNode,
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
           *(mtdDoubleType*)aStack[1].value - *(mtdDoubleType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DATE */

extern mtdModule mtdDate;
extern mtdModule mtdInterval;

IDE_RC mtfSubtract2CalculateDate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteDate = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateDate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateDate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt,
                                 mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdDate,
        &mtdDate
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteDate;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdInterval.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInterval,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSubtract2CalculateDate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
    mtdIntervalType* sValue;
    mtdDateType*     sArgument1;
    mtdDateType*     sArgument2;
    mtdIntervalType  sInterval1;
    mtdIntervalType  sInterval2;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sValue     = (mtdIntervalType*)aStack[0].value;
    sArgument1 = (mtdDateType*)aStack[1].value;
    sArgument2 = (mtdDateType*)aStack[2].value;
    
    if( (mtdDate.isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE) ||
        (mtdDate.isNull( aStack[2].column,
                         aStack[2].value ) == ID_TRUE) )
    {
        mtdInterval.null( aStack[0].column,
                          aStack[0].value );
    }
    else
    {
        IDE_TEST( mtdDateInterface::convertDate2Interval( sArgument1,
                                                         &sInterval1 )
                  != IDE_SUCCESS);

        IDE_TEST( mtdDateInterface::convertDate2Interval( sArgument2,
                                                         &sInterval2 )
                  != IDE_SUCCESS);

        sValue->second      = sInterval1.second - sInterval2.second;
        sValue->microsecond =
            sInterval1.microsecond - sInterval2.microsecond;

        //BUG -28092
        //second와 microsecond는 date와 달리 부호를 가질 수 있다.
        //이로 인해 서로 다른 부호를 가질 수가 있었다.
        //또한 오버플로우에 대한 처리가 없었다.
        
        sValue->second      += (sValue->microsecond / 1000000);
        sValue->microsecond %= 1000000;

        if( (sValue->microsecond < 0) && (sValue->second > 0) )
        {
            sValue->second--;
            sValue->microsecond += 1000000;
        }
        else
        {
            //nothing to do
        }

        if ( (sValue->microsecond > 0) && (sValue->second < 0) )
        {
            sValue->second++;
            sValue->microsecond -= 1000000;
        }
        else
        {
            //nothing to do
        }
    }
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*  BUG-45505 ZONE: INTERVAL  INTERVAL */

IDE_RC mtfSubtract2CalculateIntervalInterval( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteIntervalInterval = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateIntervalInterval,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateIntervalInterval( mtcNode*     aNode,
                                             mtcTemplate* aTemplate,
                                             mtcStack*    aStack,
                                             SInt,
                                             mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdInterval,
        &mtdInterval
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteIntervalInterval;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInterval,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSubtract2CalculateIntervalInterval( mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate )
{
    mtdIntervalType  * sValue;
    mtdIntervalType  * sArgument1;
    mtdIntervalType  * sArgument2;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if (( aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE ) ||
        ( aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE ))
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue     = (mtdIntervalType*)aStack[0].value;
        sArgument1 = (mtdIntervalType*)aStack[1].value;
        sArgument2 = (mtdIntervalType*)aStack[2].value;
        
        sValue->second      = sArgument1->second - sArgument2->second;
        sValue->microsecond = sArgument1->microsecond - sArgument2->microsecond;
       
        //BUG -28092
        //second와 microsecond는 date와 달리 부호를 가질 수 있다.
        //이로 인해 서로 다른 부호를 가질 수가 있었다.
        //또한 오버플로우에 대한 처리가 없었다.

        sValue->second      += (sValue->microsecond / 1000000);
        sValue->microsecond %= 1000000;

        if (( sValue->microsecond < 0 ) && ( sValue->second > 0 ))
        {
            sValue->second--;
            sValue->microsecond += 1000000;
        }
        else
        {
            //nothing to do
        }

        if (( sValue->microsecond > 0 ) && ( sValue->second < 0 ))
        {
            sValue->second++;
            sValue->microsecond -= 1000000;
        }
        else
        {
            //nothing to do
        }

        IDE_TEST_RAISE( ( sValue->second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                        ( sValue->second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                        ( ( sValue->second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                          ( sValue->microsecond < 0 ) ),
                        ERR_INVALID_YEAR );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
/* ZONE: DATE INTERVAL */

extern mtdModule mtdDate;
extern mtdModule mtdInterval;
    
IDE_RC mtfSubtract2CalculateDateInterval( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

static const mtcExecute mtfSubtract2ExecuteDateInterval = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubtract2CalculateDateInterval,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubtract2EstimateDateInterval( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt,
                                         mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdDate,
        &mtdInterval
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSubtract2ExecuteDateInterval;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxSubtract.mGetExecute( sModules[0]->id, sModules[1]->id );

    //IDE_TEST( mtdDate.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSubtract2CalculateDateInterval( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate )
{
    mtdDateType*     sValue;
    mtdDateType*     sArgument1;
    mtdIntervalType* sArgument2;
    mtdIntervalType  sInterval;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (mtdDate.isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE)   ||
        (mtdInterval.isNull( aStack[2].column,
                             aStack[2].value ) == ID_TRUE) )
    {
        mtdDate.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        sValue     = (mtdDateType*)aStack[0].value;
        sArgument1 = (mtdDateType*)aStack[1].value;
        sArgument2 = (mtdIntervalType*)aStack[2].value;

        IDE_TEST( mtdDateInterface::convertDate2Interval( sArgument1,
                                                         &sInterval )
                  != IDE_SUCCESS );
        
        sInterval.second      -= sArgument2->second;
        sInterval.microsecond -= sArgument2->microsecond;

        //BUG -28092
        //second와 microsecond는 date와 달리 부호를 가질 수 있다.
        //이로 인해 서로 다른 부호를 가질 수가 있었다.
        //또한 오버플로우에 대한 처리가 없었다.

        sInterval.second      += (sInterval.microsecond / 1000000);
        sInterval.microsecond %= 1000000;

        if( (sInterval.microsecond < 0) && (sInterval.second > 0) )
        {
            sInterval.second--;
            sInterval.microsecond += 1000000;
        }
        else
        {
            //nothing to do
        }

        if ( (sInterval.microsecond > 0) && (sInterval.second < 0) )
        {
            sInterval.second++;
            sInterval.microsecond -= 1000000;
        }
        else
        {
            //nothing to do
        }

        IDE_TEST_RAISE( ( sInterval.second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                        ( sInterval.second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                        ( ( sInterval.second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                          ( sInterval.microsecond < 0 ) ),
                        ERR_INVALID_YEAR );

        IDE_TEST( mtdDateInterface::convertInterval2Date( &sInterval,
                                                           sValue )
                  != IDE_SUCCESS );

    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
