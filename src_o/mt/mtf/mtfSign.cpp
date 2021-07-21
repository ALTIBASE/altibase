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
 * $Id: mtfSign.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfSign;

static mtcName mtfSignFunctionName[1] = {
    { NULL, 4, (void*)"SIGN" }
};

static IDE_RC mtfSignInitialize( void );

static IDE_RC mtfSignFinalize( void );

static IDE_RC mtfSignEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfSign = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSignFunctionName,
    NULL,
    mtfSignInitialize,
    mtfSignFinalize,
    mtfSignEstimate
};

static IDE_RC mtfSignEstimateInteger( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

static IDE_RC mtfSignEstimateSmallint( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

static IDE_RC mtfSignEstimateBigint( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

static IDE_RC mtfSignEstimateFloat( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC mtfSignEstimateReal( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

static IDE_RC mtfSignEstimateDouble( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfSignEstimates[6] = {
    { mtfSignEstimates+1, mtfSignEstimateInteger  },
    { mtfSignEstimates+2, mtfSignEstimateSmallint },
    { mtfSignEstimates+3, mtfSignEstimateBigint   },
    { mtfSignEstimates+4, mtfSignEstimateFloat    },
    { mtfSignEstimates+5, mtfSignEstimateReal     },
    { NULL,           mtfSignEstimateDouble   }
};

static mtfSubModule** mtfTable = NULL;

IDE_RC mtfSignInitialize( void )
{
    return mtf::initializeTemplate( &mtfTable, mtfSignEstimates, mtfXX );
}

IDE_RC mtfSignFinalize( void )
{
    return mtf::finalizeTemplate( &mtfTable );
}

IDE_RC mtfSignEstimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        mtcCallBack* aCallBack )
{
    const mtfSubModule* sSubModule;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getSubModule1Arg( &sSubModule,
                                     mtfTable,
                                     aStack[1].column->module->no )
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

IDE_RC mtfSignCalculateInteger(   mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfSignExecuteInteger = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSignCalculateInteger,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSignEstimateInteger( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt,
                               mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdInteger
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSignExecuteInteger;
    
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

IDE_RC mtfSignCalculateInteger( mtcNode*     aNode,
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

    if( *(mtdIntegerType*)aStack[1].value == MTD_INTEGER_NULL )
    {
        *(mtdIntegerType*)aStack[0].value = MTD_INTEGER_NULL;
    }
    else
    {
        if( *(mtdIntegerType*)aStack[1].value > 0 )
        {
            *(mtdIntegerType*)aStack[0].value = 1;
        }
        else if( *(mtdIntegerType*)aStack[1].value < 0 )
        {
            *(mtdIntegerType*)aStack[0].value = -1;
        }
        else
        {
            *(mtdIntegerType*)aStack[0].value = 0;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: SMALLINT */

extern mtdModule mtdSmallint;

IDE_RC mtfSignCalculateSmallint(   mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfSignExecuteSmallint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSignCalculateSmallint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSignEstimateSmallint( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt,
                                mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdSmallint
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSignExecuteSmallint;
    
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

IDE_RC mtfSignCalculateSmallint( mtcNode*     aNode,
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

    if( *(mtdSmallintType*)aStack[1].value == MTD_SMALLINT_NULL )
    {
        *(mtdSmallintType*)aStack[0].value = MTD_SMALLINT_NULL;
    }
    else
    {
        if( *(mtdSmallintType*)aStack[1].value > 0 )
        {
            *(mtdSmallintType*)aStack[0].value = 1;
        }
        else if( *(mtdSmallintType*)aStack[1].value < 0 )
        {
            *(mtdSmallintType*)aStack[0].value = -1;
        }
        else
        {
            *(mtdSmallintType*)aStack[0].value = 0;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: BIGINT */

extern mtdModule mtdBigint;

IDE_RC mtfSignCalculateBigint(   mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfSignExecuteBigint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSignCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSignEstimateBigint( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt,
                              mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdBigint
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSignExecuteBigint;
    
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

IDE_RC mtfSignCalculateBigint( mtcNode*     aNode,
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

    if( *(mtdBigintType*)aStack[1].value == MTD_BIGINT_NULL )
    {
        *(mtdBigintType*)aStack[0].value = MTD_BIGINT_NULL;
    }
    else
    {
        if( *(mtdBigintType*)aStack[1].value > 0 )
        {
            *(mtdBigintType*)aStack[0].value = 1;
        }
        else if( *(mtdBigintType*)aStack[1].value < 0 )
        {
            *(mtdBigintType*)aStack[0].value = -1;
        }
        else
        {
            *(mtdBigintType*)aStack[0].value = 0;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: FLOAT */

extern mtdModule mtdFloat;

IDE_RC mtfSignCalculateFloat(   mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfSignExecuteFloat = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSignCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSignEstimateFloat( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt,
                             mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];

    mtc::makeFloatConversionModule( aStack + 1, &sModules[0] );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSignExecuteFloat;
    
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

IDE_RC mtfSignCalculateFloat( mtcNode*     aNode,
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
    
    if( mtdFloat.isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE )
    {
        mtdFloat.null( aStack[0].column,
                       aStack[0].value );
    }
    else
    {

        IDE_TEST( mtc::signFloat( (mtdNumericType*)aStack[0].value,
                                  (mtdNumericType*)aStack[1].value )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: REAL */

extern mtdModule mtdReal;
    
IDE_RC mtfSignCalculateReal(   mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute mtfSignExecuteReal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSignCalculateReal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSignEstimateReal( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt,
                            mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdReal
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSignExecuteReal;
    
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

IDE_RC mtfSignCalculateReal( mtcNode*     aNode,
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
    
    if( mtdReal.isNull( aStack[1].column,
                        aStack[1].value ) == ID_TRUE )
    {
        mtdReal.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        if( *(mtdRealType*)aStack[1].value > 0 )
        {
            *(mtdRealType*)aStack[0].value = 1.0;
        }
        else if( *(mtdRealType*)aStack[1].value < 0 )
        {
            *(mtdRealType*)aStack[0].value = -1.0;
        }
        else
        {
            *(mtdRealType*)aStack[0].value = 0.0;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

extern mtdModule mtdDouble;
    
IDE_RC mtfSignCalculateDouble(   mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfSignExecuteDouble = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSignCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSignEstimateDouble( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt,
                              mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdDouble
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSignExecuteDouble;
    
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

IDE_RC mtfSignCalculateDouble( mtcNode*     aNode,
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
    
    if( mtdDouble.isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE )
    {
        mtdDouble.null( aStack[0].column,
                        aStack[0].value );
    }
    else
    {
        if( *(mtdDoubleType*)aStack[1].value > 0 )
        {
            *(mtdDoubleType*)aStack[0].value = 1.0;
        }
        else if( *(mtdDoubleType*)aStack[1].value < 0 )
        {
            *(mtdDoubleType*)aStack[0].value = -1.0;
        }
        else
        {
            *(mtdDoubleType*)aStack[0].value = 0.0;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
} 
