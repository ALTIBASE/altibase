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
 * $Id: mtfDivide.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtuProperty.h>
#include <mtdTypes.h>

extern mtfModule mtfDivide;

extern mtxModule mtxDivide; /* PROJ-2632 */

static mtcName mtfDivideFunctionName[1] = {
    { NULL, 1, (void*)"/" }
};

static IDE_RC mtfDivideInitialize( void );

static IDE_RC mtfDivideFinalize( void );

static IDE_RC mtfDivideEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfDivide = {
    1|MTC_NODE_OPERATOR_FUNCTION|
        MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDivideFunctionName,
    NULL,
    mtfDivideInitialize,
    mtfDivideFinalize,
    mtfDivideEstimate
};

static IDE_RC mtfDivideEstimateFloat( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

static IDE_RC mtfDivideEstimateReal( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

static IDE_RC mtfDivideEstimateDouble( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfNN[3] = {
    { mtfNN+1, mtfDivideEstimateFloat    },
    { mtfNN+2, mtfDivideEstimateReal     },
    { NULL,    mtfDivideEstimateDouble   }
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
    { NULL, mtfDivideEstimateFloat }
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
    { NULL, mtfDivideEstimateFloat }
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

IDE_RC mtfDivideInitialize( void )
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

IDE_RC mtfDivideFinalize( void )
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

IDE_RC mtfDivideEstimate( mtcNode*     aNode,
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

/* ZONE: FLOAT */

extern mtdModule mtdFloat;

IDE_RC mtfDivideCalculateFloat( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfDivideExecuteFloat = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDivideCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDivideEstimateFloat( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDivideExecuteFloat;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxDivide.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfDivideCalculateFloat( mtcNode*     aNode,
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
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        IDE_TEST( mtc::divideFloat( (mtdNumericType*)aStack[0].value,
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

IDE_RC mtfDivideCalculateReal( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute mtfDivideExecuteReal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDivideCalculateReal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDivideEstimateReal( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDivideExecuteReal;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxDivide.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfDivideCalculateReal( mtcNode*     aNode,
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
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        IDE_TEST_RAISE( *(mtdRealType*)aStack[2].value == 0.0,
                        ERR_DIVIDE_BY_ZERO );
        *(mtdRealType*)aStack[0].value =
               *(mtdRealType*)aStack[1].value / *(mtdRealType*)aStack[2].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

extern mtdModule mtdDouble;

IDE_RC mtfDivideCalculateDouble( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfDivideExecuteDouble = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDivideCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDivideEstimateDouble( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDivideExecuteDouble;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxDivide.mGetExecute( sModules[0]->id, sModules[1]->id );

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

IDE_RC mtfDivideCalculateDouble( mtcNode*     aNode,
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
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        IDE_TEST_RAISE( *(mtdDoubleType*)aStack[2].value == 0.0,
                        ERR_DIVIDE_BY_ZERO );
        *(mtdDoubleType*)aStack[0].value =
           *(mtdDoubleType*)aStack[1].value / *(mtdDoubleType*)aStack[2].value;

        // fix BUG-13757
        // double'0' / double'-1' = double'-0' 이 나오므로
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
    
    IDE_EXCEPTION( ERR_DIVIDE_BY_ZERO );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DIVIDE_BY_ZERO));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 
