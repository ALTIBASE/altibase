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
 * $Id: mtfMod.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtuProperty.h>
#include <mtdTypes.h>

extern mtfModule mtfMod;

static mtcName mtfModFunctionName[1] = {
    { NULL, 3, (void*)"MOD" }
};

static IDE_RC mtfModInitialize( void );

static IDE_RC mtfModFinalize( void );

static IDE_RC mtfModEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

mtfModule mtfMod = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfModFunctionName,
    NULL,
    mtfModInitialize,
    mtfModFinalize,
    mtfModEstimate
};

static IDE_RC mtfModEstimateInteger( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

static IDE_RC mtfModEstimateSmallint( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

static IDE_RC mtfModEstimateBigint( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC mtfModEstimateFloat( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

static IDE_RC mtfModEstimateReal( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

static IDE_RC mtfModEstimateDouble( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfNN[6] = {
    { mtfNN+1, mtfModEstimateInteger  },
    { mtfNN+2, mtfModEstimateSmallint },
    { mtfNN+3, mtfModEstimateBigint   },
    { mtfNN+4, mtfModEstimateFloat    },
    { mtfNN+5, mtfModEstimateReal     },
    { NULL,    mtfModEstimateDouble   }
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
static mtfSubModule mtfNP[4] = {
    { mtfNP+1, mtfModEstimateInteger  },
    { mtfNP+2, mtfModEstimateSmallint },
    { mtfNP+3, mtfModEstimateBigint   },
    { NULL,    mtfModEstimateFloat    }
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
    { NULL, mtfModEstimateFloat }
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

IDE_RC mtfModInitialize( void )
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

IDE_RC mtfModFinalize( void )
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

IDE_RC mtfModEstimate( mtcNode*     aNode,
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

IDE_RC mtfModCalculateInteger(    mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfModExecuteInteger = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfModCalculateInteger,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfModEstimateInteger( mtcNode*     aNode,
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfModExecuteInteger;
    
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

IDE_RC mtfModCalculateInteger( mtcNode*     aNode,
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

    if( ( *(mtdIntegerType*)aStack[1].value == MTD_INTEGER_NULL ) ||
        ( *(mtdIntegerType*)aStack[2].value == MTD_INTEGER_NULL ) )
    {
        *(mtdIntegerType*)aStack[0].value = MTD_INTEGER_NULL;
    }
    else
    {
        IDE_TEST_RAISE( *(mtdIntegerType*)aStack[2].value == 0,
                        ERR_DIVIDE_BY_ZERO );
        *(mtdIntegerType*)aStack[0].value =
         *(mtdIntegerType*)aStack[1].value % *(mtdIntegerType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: SMALLINT */

extern mtdModule mtdSmallint;

IDE_RC mtfModCalculateSmallint(    mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfModExecuteSmallint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfModCalculateSmallint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfModEstimateSmallint( mtcNode*     aNode,
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
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfModExecuteSmallint;
    
    //IDE_TEST( mtdSmallint.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdSmallint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfModCalculateSmallint( mtcNode*     aNode,
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

    if( ( *(mtdSmallintType*)aStack[1].value == MTD_SMALLINT_NULL ) ||
        ( *(mtdSmallintType*)aStack[2].value == MTD_SMALLINT_NULL ) )
    {
        *(mtdSmallintType*)aStack[0].value = MTD_SMALLINT_NULL;
    }
    else
    {
        IDE_TEST_RAISE( *(mtdSmallintType*)aStack[2].value == 0,
                        ERR_DIVIDE_BY_ZERO );
        *(mtdSmallintType*)aStack[0].value =
       *(mtdSmallintType*)aStack[1].value % *(mtdSmallintType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    

    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: BIGINT */

extern mtdModule mtdBigint;

IDE_RC mtfModCalculateBigint(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfModExecuteBigint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfModCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfModEstimateBigint( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfModExecuteBigint;
    
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

IDE_RC mtfModCalculateBigint( mtcNode*     aNode,
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
        IDE_TEST_RAISE( *(mtdBigintType*)aStack[2].value == 0,
                        ERR_DIVIDE_BY_ZERO );
        *(mtdBigintType*)aStack[0].value =
           *(mtdBigintType*)aStack[1].value % *(mtdBigintType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: FLOAT */

extern mtdModule mtdFloat;

IDE_RC mtfModCalculateFloat(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfModExecuteFloat = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfModCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfModEstimateFloat( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfModExecuteFloat;
    
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

IDE_RC mtfModCalculateFloat( mtcNode*     aNode,
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
        IDE_TEST( mtc::modFloat( (mtdNumericType*)aStack[0].value,
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
    
IDE_RC mtfModCalculateReal(    mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute mtfModExecuteReal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfModCalculateReal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfModEstimateReal( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfModExecuteReal;
    
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

IDE_RC mtfModCalculateReal( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
    mtdDoubleType sValue1;
    mtdDoubleType sValue2;
    SChar         sTmpBuf[32];
        
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
        IDE_TEST_RAISE( *(mtdRealType*)aStack[2].value == 0.0,
                        ERR_DIVIDE_BY_ZERO );

        // idlOS::sprintf( sTmpBuf, "%"ID_FLOAT_G_FMT"",
        //                *(mtdRealType*)aStack[1].value );
        idlOS::snprintf( sTmpBuf, ID_SIZEOF(sTmpBuf),
                         "%"ID_FLOAT_G_FMT"",
                         *(mtdRealType*)aStack[1].value );
        sValue1 = idlOS::strtod( sTmpBuf, NULL );

        // idlOS::sprintf( sTmpBuf, "%"ID_FLOAT_G_FMT"",
        //                 *(mtdRealType*)aStack[2].value );
        idlOS::snprintf( sTmpBuf, ID_SIZEOF(sTmpBuf),
                         "%"ID_FLOAT_G_FMT"",
                        *(mtdRealType*)aStack[2].value );
        sValue2 = idlOS::strtod( sTmpBuf, NULL );      
        
        *(mtdRealType*)aStack[0].value = idlOS::fmod( sValue1, sValue2 );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

extern mtdModule mtdDouble;
    
IDE_RC mtfModCalculateDouble(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfModExecuteDouble = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfModCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfModEstimateDouble( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfModExecuteDouble;
    
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

IDE_RC mtfModCalculateDouble( mtcNode*     aNode,
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
        IDE_TEST_RAISE( *(mtdDoubleType*)aStack[2].value == 0.0,
                        ERR_DIVIDE_BY_ZERO );
        *(mtdDoubleType*)aStack[0].value =
            idlOS::fmod( *(mtdDoubleType*)aStack[1].value,
                         *(mtdDoubleType*)aStack[2].value );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
