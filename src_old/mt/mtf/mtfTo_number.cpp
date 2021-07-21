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
 * $Id: mtfTo_number.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtlTerritory.h>

extern mtfModule mtfTo_number;

extern mtdModule mtdNumeric;
extern mtdModule mtdFloat;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfTo_numberFunctionName[2] = {
    { mtfTo_numberFunctionName+1, 9, (void*)"TO_NUMBER" },
    { NULL,              4, (void*)"PLUS"      }
};

static IDE_RC mtfTo_numberInitialize( void );

static IDE_RC mtfTo_numberFinalize( void );

static IDE_RC mtfTo_numberEstimate( mtcNode     * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    mtcCallBack * aCallBack );

IDE_RC checkNumberFormat( UChar * aFmt,
                          UInt   aLength,
                          UChar * aToken );

IDE_RC normalNumberFormat( UChar       * aString,
                           UInt        * aStringLen,
                           UChar       * aNumFmt,
                           UInt        * aNumFmtLen,
                           UChar       * aResultTemp,
                           UInt        * aResultLen,
                           mtlCurrency * aCurrency,
                           UChar       * aToken );

mtfModule mtfTo_number = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTo_numberFunctionName,
    NULL,
    mtfTo_numberInitialize,
    mtfTo_numberFinalize,
    mtfTo_numberEstimate
};

static IDE_RC mtfTo_numberCalculateFor2Args( mtcNode     * aNode,
                                             mtcStack    * aStack,
                                             SInt          aRemain,
                                             void        * aInfo,
                                             mtcTemplate * aTemplate );

const mtcExecute mtfTo_numberExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfTo_numberEstimateInteger( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           mtcCallBack* aCallBack );

static IDE_RC mtfTo_numberEstimateSmallint( mtcNode*     aNode,
                                            mtcTemplate* aTemplate,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            mtcCallBack* aCallBack );

static IDE_RC mtfTo_numberEstimateBigint( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

static IDE_RC mtfTo_numberEstimateNumeric( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           mtcCallBack* aCallBack );

static IDE_RC mtfTo_numberEstimateFloat( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

static IDE_RC mtfTo_numberEstimateReal( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

static IDE_RC mtfTo_numberEstimateDouble( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfTo_numberEstimates[7] = {
    { mtfTo_numberEstimates+1, mtfTo_numberEstimateInteger  },
    { mtfTo_numberEstimates+2, mtfTo_numberEstimateSmallint },
    { mtfTo_numberEstimates+3, mtfTo_numberEstimateBigint   },
    { mtfTo_numberEstimates+4, mtfTo_numberEstimateNumeric  },
    { mtfTo_numberEstimates+5, mtfTo_numberEstimateFloat    },
    { mtfTo_numberEstimates+6, mtfTo_numberEstimateReal     },
    { NULL,           mtfTo_numberEstimateDouble   }
};

static mtfSubModule** mtfTable = NULL;

IDE_RC mtfTo_numberInitialize( void )
{
    return mtf::initializeTemplate( &mtfTable, mtfTo_numberEstimates, mtfXX );
}

IDE_RC mtfTo_numberFinalize( void )
{
    return mtf::finalizeTemplate( &mtfTable );
}

IDE_RC mtfTo_numberEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];
    const mtdModule* sModule;
    const mtfSubModule* sSubModule;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
    {
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
    }
    else
    {
        // PROJ-1579 NCHAR
        IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[0],
                                                    aStack[1].column->module )
                  != IDE_SUCCESS );

        sModules[1] = sModules[0];

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteFor2Args;

        // BUG-16483
        if ( (aStack[1].column->module->id == MTD_CHAR_ID) ||
             (aStack[1].column->module->id == MTD_VARCHAR_ID) )
        {
            sModule = & mtdFloat;
        }
        else
        {
            sModule = & mtdNumeric;
        }

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         sModule,
                                         0, 
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: INTEGER */

extern mtdModule mtdInteger;

IDE_RC mtfTo_numberCalculateInteger( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteInteger = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateInteger,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateInteger( mtcNode*     aNode,
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteInteger;
    
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

IDE_RC mtfTo_numberCalculateInteger( mtcNode*     aNode,
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
    
    *(mtdIntegerType*)aStack[0].value = *(mtdIntegerType*)aStack[1].value;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: SMALLINT */

extern mtdModule mtdSmallint;

IDE_RC mtfTo_numberCalculateSmallint( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteSmallint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateSmallint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateSmallint( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteSmallint;
    
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

IDE_RC mtfTo_numberCalculateSmallint( mtcNode*     aNode,
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
    
    *(mtdSmallintType*)aStack[0].value = *(mtdSmallintType*)aStack[1].value;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: BIGINT */

extern mtdModule mtdBigint;

IDE_RC mtfTo_numberCalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteBigint = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateBigint( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteBigint;
    
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

IDE_RC mtfTo_numberCalculateBigint( mtcNode*     aNode,
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
    
    *(mtdBigintType*)aStack[0].value = *(mtdBigintType*)aStack[1].value;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: NUMERIC */

IDE_RC mtfTo_numberCalculateNumeric( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteNumeric = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateNumeric,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateNumeric( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt,
                                    mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdNumeric
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteNumeric;

    /*
    IDE_TEST( mtdNumeric.estimate( aStack[0].column,
                                   aStack[1].column->flag &
                                   MTC_COLUMN_ARGUMENT_COUNT_MASK,
                                   aStack[1].column->precision,
                                   aStack[1].column->scale )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNumeric,
                                     aStack[1].column->flag &
                                     MTC_COLUMN_ARGUMENT_COUNT_MASK,
                                     aStack[1].column->precision,
                                     aStack[1].column->scale )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfTo_numberCalculateNumeric( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    mtdNumericType* sValue;
    mtdNumericType* sArgument;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sValue    = (mtdNumericType*)aStack[0].value;
    sArgument = (mtdNumericType*)aStack[1].value;
    
    idlOS::memcpy( sValue, sArgument, sArgument->length + 1 );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: FLOAT */

extern mtdModule mtdFloat;

IDE_RC mtfTo_numberCalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteFloat = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateFloat( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteFloat;

    /*
    IDE_TEST( mtdFloat.estimate( aStack[0].column,
                                 aStack[1].column->flag &
                                 MTC_COLUMN_ARGUMENT_COUNT_MASK,
                                 aStack[1].column->precision,
                                 aStack[1].column->scale )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     aStack[1].column->flag &
                                     MTC_COLUMN_ARGUMENT_COUNT_MASK,
                                     aStack[1].column->precision,
                                     aStack[1].column->scale )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfTo_numberCalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
    mtdNumericType* sValue;
    mtdNumericType* sArgument;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sValue    = (mtdNumericType*)aStack[0].value;
    sArgument = (mtdNumericType*)aStack[1].value;
    
    idlOS::memcpy( sValue, sArgument, sArgument->length + 1 );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: REAL */

extern mtdModule mtdReal;
    
IDE_RC mtfTo_numberCalculateReal( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteReal = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateReal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateReal( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteReal;
    
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

IDE_RC mtfTo_numberCalculateReal( mtcNode*     aNode,
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
    
    *(mtdRealType*)aStack[0].value = *(mtdRealType*)aStack[1].value;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* ZONE: DOUBLE */

extern mtdModule mtdDouble;
    
IDE_RC mtfTo_numberCalculateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfTo_numberExecuteDouble = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_numberCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_numberEstimateDouble( mtcNode*     aNode,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfTo_numberExecuteDouble;
    
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

IDE_RC mtfTo_numberCalculateDouble( mtcNode*     aNode,
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
    
    *(mtdDoubleType*)aStack[0].value = *(mtdDoubleType*)aStack[1].value;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/* ZONE: 2 Arguments */

IDE_RC mtfTo_numberCalculateFor2Args( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : To_number for 2 Args
 *
 * Implementation :
 *    TO_NUMBER( char1, [number_format] )
 *
 *    aStack[0] : number_format에 맞게 입력된 char1을  numeric type으로 변환한다.
 *    aStack[1] : char1 ( 입력 문자열 )
 *    aStack[2] : number format
 *
 *    ex) select to_number ( '<$123.45>', '$999.99PR' ) from t1;
 *
 *        TO_NUMBER('<$123.45>','$999.99PR')
 *        ----------------------------------
 *                                   -123.45
 *
 ***********************************************************************/

    mtdCharType    * sVarchar1 = NULL;
    mtdCharType    * sVarchar2 = NULL;
    mtlCurrency      sCurrency;

    UChar          * sString    = NULL;
    UInt             sStringLen = 0;
    UChar          * sNumFmt    = NULL;
    UInt             sNumFmtLen = 0;

    UChar            sToken[MTD_NUMBER_MAX];
    UChar            sResultTemp[MTD_NUMBER_FORMAT_BUFFER_LEN];
    UInt             sResultLen = 0;

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
        sVarchar1 = (mtdCharType*)aStack[1].value;
        sVarchar2 = (mtdCharType*)aStack[2].value;

        sString = sVarchar1->value;
        sStringLen = sVarchar1->length;
        sNumFmt = sVarchar2->value;
        sNumFmtLen = sVarchar2->length;

        IDE_TEST_RAISE( ( sStringLen > MTC_TO_CHAR_MAX_PRECISION ) ||
                        ( sNumFmtLen > MTC_TO_CHAR_MAX_PRECISION ),
                        ERR_TO_CHAR_MAX_PRECISION );

        IDE_TEST( checkNumberFormat( sNumFmt, sNumFmtLen, sToken )
                  != IDE_SUCCESS );

        idlOS::memset( &sCurrency, 0x0, sizeof( mtlCurrency ));

        if ( ( sToken[MTD_NUMBER_FORMAT_C] > 0 ) ||
             ( sToken[MTD_NUMBER_FORMAT_D] > 0 ) ||
             ( sToken[MTD_NUMBER_FORMAT_G] > 0 ) ||
             ( sToken[MTD_NUMBER_FORMAT_L] > 0 ) )
        {

            IDE_TEST( aTemplate->nlsCurrency( aTemplate, &sCurrency )
                      != IDE_SUCCESS );
            aTemplate->nlsCurrencyRef = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sToken[MTD_NUMBER_FORMAT_XXXX] == 1 )
        {
            UShort            sIndex = 0;
            UShort            sCnt = 4;
            SLong             sHexResult = 0;
            UChar             sOneChar;

            IDE_TEST_RAISE ( sStringLen > 8,
                             ERR_INVALID_LENGTH );

            while ( sIndex < sStringLen )
            {
                sOneChar = sString[sIndex];

                if ( sOneChar >= '0' && sOneChar <= '9' )
                {
                    sHexResult |= ( sString[sIndex] - '0' )
                               << ( sStringLen * 4 - sCnt );
                }
                else if ( sOneChar >= 'A' && sOneChar <= 'F' )
                {
                    sHexResult |= ( sString[sIndex] - 'A' + 10 )
                               << ( sStringLen * 4 - sCnt );
                }
                else if ( sOneChar >= 'a' && sOneChar <= 'f' )
                {
                    sHexResult |= ( sString[sIndex] - 'a' + 10 )
                               << ( sStringLen * 4 - sCnt );
                }
                else
                {
                    IDE_RAISE ( ERR_INVALID_LITERAL );
                }

                sCnt += 4;
                sIndex++;
            }
            mtc::makeNumeric( (mtdNumericType*)aStack[0].value,
                                        (SLong) sHexResult );
        }
        else
        {
            IDE_TEST( normalNumberFormat( sString,
                                          &sStringLen,
                                          sNumFmt,
                                          &sNumFmtLen,
                                          sResultTemp,
                                          &sResultLen,
                                          &sCurrency,
                                          sToken  )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::makeNumeric( (mtdNumericType*)aStack[0].value,
                                        MTD_NUMERIC_MANTISSA_MAXIMUM,
                                        sResultTemp,
                                        sResultLen )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION( ERR_TO_CHAR_MAX_PRECISION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_TO_CHAR_MAX_PRECISION,
                                MTC_TO_CHAR_MAX_PRECISION));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
};

IDE_RC normalNumberFormat( UChar       * aString,
                           UInt        * aStringLen,
                           UChar       * aNumFmt,
                           UInt        * aNumFmtLen,
                           UChar       * aResultTemp,
                           UInt        * aResultLen,
                           mtlCurrency * aCurrency,
                           UChar       * aToken )
{
/***********************************************************************
 *
 * Description : string1과 비교하기 위해 string2(number format)를 규칙대로 변환한다.
 *               기본적인 규칙은 sign + $ + number + . + number의 형태이다.
 * 
 * #include <mtdTypes.h>
 * # define MTD_NUMBER_FORMAT_COMMA  (1)
 * # define MTD_NUMBER_FORMAT_PERIOD (2)
 * # define MTD_NUMBER_FORMAT_DOLLAR (3)
 * # define MTD_NUMBER_FORMAT_ZERO   (4)
 * # define MTD_NUMBER_FORMAT_NINE   (5)
 * # define MTD_NUMBER_FORMAT_B      (6)
 * # define MTD_NUMBER_FORMAT_EEEE   (7)
 * # define MTD_NUMBER_FORMAT_MI     (8)
 * # define MTD_NUMBER_FORMAT_PR     (9)
 * # define MTD_NUMBER_FORMAT_RN     (10)
 * # define MTD_NUMBER_FORMAT_S      (11)
 * # define MTD_NUMBER_FORMAT_V      (12)
 * # define MTD_NUMBER_FORMAT_XXXX   (13)
 *
 * Implementation :
 *
 ***********************************************************************/
    UChar  sString[MTD_NUMBER_FORMAT_BUFFER_LEN];
    UInt   sStringLen   = *aStringLen;
    UChar  sFormat[MTD_NUMBER_FORMAT_BUFFER_LEN];
    UInt   sFormatLen   = *aNumFmtLen;
    UInt   sFormatIndex = 0;
    UInt   sStringIndex = 0;
    UInt   sIterator    = 0;
    UInt   sResultLen   = 0;
    UInt   sResultIndex = 0;
    UInt   sCursor      = 0;
    UChar  sTemp[MTD_NUMBER_FORMAT_BUFFER_LEN];
    idBool sIsPeriod    = ID_FALSE;
    UShort sIntNumCnt   = 0;
    UInt   sCount       = 0;
    UChar *sStringChar  = NULL;
    UChar *sFormatChar  = NULL;
    UChar *sResultChar  = NULL;

    UShort sDollarCnt   = aToken[MTD_NUMBER_FORMAT_DOLLAR];
    UShort sMICnt       = aToken[MTD_NUMBER_FORMAT_MI];
    UShort sPRCnt       = aToken[MTD_NUMBER_FORMAT_PR];
    UShort sSCnt        = aToken[MTD_NUMBER_FORMAT_S];
    UShort sIntZeroCnt  = aToken[MTD_COUNT_ZERO];
    UShort sIntNineCnt  = aToken[MTD_COUNT_NINE];
    UShort sDCnt        = aToken[MTD_NUMBER_FORMAT_D];
    // To fix BUG-19751
    // PR 포멧을 해석하기 위한 <>의 열림 닫힘 여부를 판별하기 위한 변수
    SInt   sPRStage = 0;
    
    // sTemp array 초기화
    idlOS::memset( sTemp, 0x0, MTD_NUMBER_FORMAT_BUFFER_LEN );

    // format model을 기호가 나오는 순서에 맞게 다시 변환한다.
    // 규칙은 sign + $ + number or comma + . + number 또는
    //        sign + $ + number + . + number + eeee 이다.
    // string의 앞쪽 공백은 모두 제거한다.
    for ( sIterator = 0; sIterator < sStringLen; sIterator++ )
    {
        if ( *(aString + sIterator) != ' ' )
        {
            break;
        }
    }
    
    // 전부 공백만 있을 경우 error
    IDE_TEST_RAISE( ( sStringLen > 0 ) &&
                    ( sStringLen == sIterator ),
                    ERR_INVALID_LITERAL );
    
    idlOS::memcpy( sString,
                   aString + sIterator,
                   sStringLen - sIterator );
    sStringLen -= sIterator;
    
    idlOS::memcpy( sFormat,
                   aNumFmt,
                   sFormatLen );

    while ( sFormatIndex < sFormatLen )
    {
        // B 또는 $기호는 삭제한다.
        if ( ( *( sFormat + sFormatIndex ) == 'B' ) ||
             ( *( sFormat + sFormatIndex ) == 'b' ) ||
             ( *( sFormat + sFormatIndex ) == '$' ) )
        {
            //fix BUG-16273
            for( sCursor = sFormatIndex;
                 sCursor < (sFormatLen - 1);
                 sCursor++ )
            {
                sFormat[sCursor] = sFormat[sCursor + 1];
            }        
            //idlOS::memcpy( sFormat + sFormatIndex,
            //               sFormat + sFormatIndex + 1,
            //               sFormatLen - sFormatIndex - 1 );
            sFormatLen--;
        }
        sFormatIndex++;
    }

    sIterator = 0;
    // string에서 정수 부분의 숫자 개수를 센다.
    for ( sIterator = 0; sIterator < sStringLen; sIterator++ )
    {
        if ( *( sString + sIterator ) >= '0' &&
             *( sString + sIterator ) <= '9' )
        {
            if ( sIsPeriod == ID_FALSE )
            {
                sIntNumCnt++;
            }
        }

        if ( sDCnt == 0 )
        {
            if ( *( sString + sIterator ) == '.' )
            {
                sIsPeriod = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( *( sString + sIterator ) == aCurrency->D )
            {
                sIsPeriod = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    /* 정수 부분을 쓰기 이전의 숫자 중, 0이 나오기
     * 전까지의 9와 ,를 맨 앞으로 보낸다.
     */
    sFormatIndex = 0;
    sCount = sIntNineCnt + sIntZeroCnt;
    sFormatChar  = &sFormat[0];
    while ( sCount >= sIntNumCnt )
    {
        if ( sCount > sIntNumCnt )
        {
            if ( ( *sFormatChar == '0' ) || ( *sFormatChar == '.' ) ||
                 ( idlOS::strCaselessMatch( sFormatChar, 1, "D", 1 ) == 0 ) )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( ( *sFormatChar != ',' ) &&
                 ( idlOS::strCaselessMatch( sFormatChar, 1, "G", 1 ) != 0 ))
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( ( *sFormatChar == '9' ) || ( *sFormatChar == ',' ) ||
             idlOS::strCaselessMatch( sFormatChar, 1, "G", 1 ) == 0 )
        {
            if ( *sFormatChar == '9' )
            {
                sIntNineCnt--;
                sCount = sIntNineCnt + sIntZeroCnt;
            }
            else
            {
                /* Nothing to do */
            }
            for ( sCursor = sFormatIndex;
                  sCursor < ( sFormatLen - 1 ) ;
                  sCursor++ )
            {
                sFormat[sCursor] = sFormat[sCursor + 1];
            }
            sFormatLen--;
        }
        else
        {
            sFormatIndex++;
            sFormatChar = &sFormat[sFormatIndex];
        }
    }

    sStringIndex = 0;
    sFormatIndex = 0;
    sIterator = 0;

    // str1의 기호가 - 이고, str2에 부호 기호가 없을 경우,
    // str2의 맨앞에 임시로 @기호를 추가한다.
    if ( sStringLen > 0 )
    {
        if ( *sString == '-' && sSCnt == 0 && sPRCnt == 0 && sMICnt == 0 )
        {
            idlOS::memcpy( sTemp,
                           sFormat,
                           sFormatLen );
            
            idlOS::memset( sFormat,
                           '@',
                           1 );
            
            idlOS::memcpy( sFormat + 1,
                           sTemp,
                           sFormatLen );
            
            sFormatIndex++;
            sFormatLen++;
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    // <123.45>와 999.99PR과 같은 형태를 비교하기 쉽게 하기 위해
    // PR이 있는 경우, 맨 앞으로 R을 가져온다.
    // string에 <가 없는 경우(양수)에는 그냥 놔둠
    if ( sStringLen > 0 )
    {
        if ( sPRCnt == 1 && *sString == '<' )
        {
            if ( sFormatLen > 1 )
            {
                idlOS::memcpy( sTemp,
                               sFormat,
                               sFormatLen );
            
                idlOS::memcpy( sFormat,
                               sFormat + sFormatLen - 1,
                               1 );

                idlOS::memcpy( sFormat + 1,
                               sTemp,
                               sFormatLen - 1 );
            }
            else
            {
                //nothing to do
            }
        }
        // MI가 있는 경우 맨뒤의 I를 삭제한다. 
        else if ( sMICnt == 1 )
        {
            sFormatLen--;
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }
        
    // $기호는 부호 기호 바로 다음으로 보낸다.
    if ( sDollarCnt == 1 )
    {
        if ( idlOS::strCaselessMatch( sFormat, 1, "@", 1 ) == 0 ||
             idlOS::strCaselessMatch( sFormat, 1, "R", 1 ) == 0 ||
             idlOS::strCaselessMatch( sFormat, 1, "S", 1 ) == 0 )
        {
            idlOS::memcpy( sTemp,
                           sFormat,
                           sFormatLen );

            idlOS::memcpy( sFormat,
                           sTemp,
                           1 );

            idlOS::memset( sFormat + 1,
                           '$',
                           1 );
            
            if ( sFormatLen > 1 )
            {
                idlOS::memcpy( sFormat + 2,
                               sTemp + 1,
                               sFormatLen - 1 );
            }
            else
            {
                //nothing to do
            }
            
            sFormatLen++;
        }
        else
        {
            idlOS::memcpy( sTemp,
                           sFormat,
                           sFormatLen );

            idlOS::memset( sFormat,
                           '$',
                           1 );

            idlOS::memcpy( sFormat + 1,
                           sTemp,
                           sFormatLen );
            sFormatLen++;
        }
    }
    
    sStringIndex = 0;
    sFormatIndex = 0;

    while ( sStringIndex < sStringLen &&
            sFormatIndex < sFormatLen )
    {
        sStringChar = &sString[sStringIndex];
        sFormatChar = &sFormat[sFormatIndex];
        sResultChar = &sTemp[sResultIndex];
        switch ( *sFormatChar )
        {
        case '@' :
            if ( *sStringChar == '-' )
            {
                idlOS::memcpy( sResultChar, sStringChar, 1 );
                ++sResultIndex;
                ++sResultLen;
            }
            else
            {
                /* Nothing to do */
            }
            break;
        case 'R' :
        case 'r' :
            IDE_TEST_RAISE( *sStringChar != '<', ERR_INVALID_LITERAL );
            sPRStage++;
            idlOS::memset( sResultChar, '-', 1 );
            ++sResultIndex;
            ++sResultLen;
            break;
        case 'P' :
        case 'p' :
            IDE_TEST_RAISE( *sStringChar != '>', ERR_INVALID_LITERAL );
            sPRStage--;
            break;
        case 'M' :
        case 'm' :
            IDE_TEST_RAISE( idlOS::strCaselessMatch( sFormatChar, 2, "MI", 2 )
                                   != 0, ERR_INVALID_LITERAL );
            IDE_TEST_RAISE( ( *sStringChar != '-' ) &&
                            ( *sStringChar != '+' ), ERR_INVALID_LITERAL );
            idlOS::memcpy( sTemp, sStringChar, 1 );
            break;
        case 'S' :
        case 's' :
            IDE_TEST_RAISE( ( *sStringChar != '+' ) && ( *sStringChar != '-' ),
                            ERR_INVALID_LITERAL );
            idlOS::memcpy( sTemp, sStringChar, 1 );
            if ( sResultIndex == 0 )
            {
                ++sResultIndex;
                ++sResultLen;
            }
            else
            {
                /* Nothing to do */
            }
            break;
        case '$' :
            if ( *sStringChar == '$' )
            {
                break;
            } 
            else if ( ( *sStringChar == '+' ) || ( *sStringChar == '-' ) )
            {
                IDE_TEST_RAISE( *( sStringChar + 1 ) != '$', ERR_INVALID_LITERAL );
                idlOS::memcpy( sResultChar, sStringChar, 1 );
                ++sResultIndex;
                ++sResultLen;
                ++sStringIndex;
            } 
            else
            {
                IDE_RAISE( ERR_INVALID_LITERAL );
            }
            break;
        case ',' :
            IDE_TEST_RAISE( *sStringChar != ',', ERR_INVALID_LITERAL );
            break;
        case '.' :
            IDE_TEST_RAISE( *sStringChar != '.', ERR_INVALID_LITERAL );
            idlOS::memset( sResultChar, '.', 1 );
            ++sResultIndex;
            ++sResultLen;
            break;
        case '0' :
            IDE_TEST_RAISE( ( sIntNumCnt < ( sIntNineCnt + sIntZeroCnt ) ) &&
                                       ( *sStringChar != '0' ) ,
                                       ERR_INVALID_LITERAL );
        case '9' :
            if ( ( *sStringChar >= '0' ) && ( *sStringChar <= '9' ))
            {
                if ( sResultIndex == 0 )
                {
                    idlOS::memset( sResultChar, '+', 1 );
                    ++sResultIndex;
                    ++sResultLen;
                    sResultChar = &sTemp[sResultIndex];
                }
                else
                {
                    /* Nothing to do */
                }
                idlOS::memcpy( sResultChar, sStringChar, 1 );
                ++sResultIndex;
                ++sResultLen;
            }
            else
            {
                ++sFormatIndex;
                continue;
            }            
            break;
        case 'C' :
        case 'c' :
            if ( ( *sStringChar == '+' ) || ( *sStringChar == '-' ) )
            {
                idlOS::memcpy( sTemp, sStringChar, 1 );
                if ( sResultIndex == 0 )
                {
                    ++sResultIndex;
                    ++sResultLen;
                    sResultChar = &sTemp[sResultIndex];
                    ++sStringIndex;
                    sStringChar = &sString[sStringIndex];
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
            IDE_TEST_RAISE( idlOS::strCaselessMatch( sStringChar, 3, 
                                                     aCurrency->C, 3 ) != 0,
                            ERR_INVALID_LITERAL );
            sStringIndex += 2;            
            break;
        case 'L' :
        case 'l' :
            if ( ( *sStringChar == '+' ) || ( *sStringChar == '-' ) )
            {
                idlOS::memcpy( sTemp, sStringChar, 1 );
                if ( sResultIndex == 0 )
                {
                    ++sResultIndex;
                    ++sResultLen;
                    sResultChar = &sTemp[sResultIndex];
                    ++sStringIndex;
                    sStringChar = &sString[sStringIndex];
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
            IDE_TEST_RAISE( idlOS::strCaselessMatch( sStringChar, aCurrency->len, 
                                                     aCurrency->L, aCurrency->len ) != 0,
                            ERR_INVALID_LITERAL );
            sStringIndex += aCurrency->len - 1;            
            break;
        case 'D' :
        case 'd' :
            IDE_TEST_RAISE( idlOS::strCaselessMatch( sStringChar, 1,
                                                     &aCurrency->D, 1 ) != 0,
                            ERR_INVALID_LITERAL );
            idlOS::memset( sResultChar, '.', 1 );
            ++sResultIndex;
            ++sResultLen;
            break;
        case 'G' :
        case 'g' :
            IDE_TEST_RAISE( idlOS::strCaselessMatch( sStringChar, 1,
                                                     &aCurrency->G, 1 ) != 0,
                            ERR_INVALID_LITERAL );
            break;
        case 'E' :
        case 'e' :

            break;
        default: 
            break;
        }
        ++sFormatIndex;
        ++sStringIndex;
    }
    
    if ( sStringIndex < sStringLen )
    {
        IDE_RAISE( ERR_INVALID_LITERAL );
    }
    else if ( sFormatIndex < sFormatLen )
    {
        while ( sFormatIndex < sFormatLen )
        {
            sFormatChar = &sFormat[sFormatIndex];

            if ( ( sFormatLen > 1 ) && ( sMICnt > 0 ) )
            {
                IDE_TEST_RAISE( idlOS::strCaselessMatch( sFormatChar, 2,
                                                         "MI", 2 ) == 0,
                                ERR_INVALID_LITERAL );
            }
            else
            {
                /* Nothing to do */
            }

            if ( sDCnt <= 0 )
            {
                IDE_TEST_RAISE( ( sMICnt + sPRCnt ) == 0 &&
                                ( *sFormatChar != '0' ) &&
                                ( *sFormatChar != '9' ) &&
                                ( *sFormatChar != '.' ),
                                ERR_INVALID_LITERAL );
            }
            else
            {
                IDE_TEST_RAISE( ( sMICnt + sPRCnt ) == 0 &&
                                ( *sFormatChar != '0' ) &&
                                ( *sFormatChar != '9' ) &&
                                ( idlOS::strCaselessMatch( sFormatChar, 1,
                                                         "D", 1 ) != 0 ),
                                ERR_INVALID_LITERAL );
            }
            sFormatIndex++;
        }
    }
    
    IDE_TEST_RAISE( sPRStage != 0, ERR_INVALID_LITERAL );

    *aResultLen = sResultLen;

    idlOS::memcpy( aResultTemp,
                   sTemp,
                   sResultLen );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC checkNumberFormat( UChar* aFmt, UInt aLength, UChar* aToken )
{
/***********************************************************************
 *
 * Description : number format을 체크하는 함수
 *              다른 함수에서 필요한 number format의 각 개수는 aToken에 저장된다.
 *
 * #include <mtdTypes.h>
 * # define MTD_NUMBER_FORMAT_COMMA  (1)
 * # define MTD_NUMBER_FORMAT_PERIOD (2)
 * # define MTD_NUMBER_FORMAT_DOLLAR (3)
 * # define MTD_NUMBER_FORMAT_ZERO   (4)
 * # define MTD_NUMBER_FORMAT_NINE   (5)
 * # define MTD_NUMBER_FORMAT_B      (6)
 * # define MTD_NUMBER_FORMAT_EEEE   (7)
 * # define MTD_NUMBER_FORMAT_MI     (8)
 * # define MTD_NUMBER_FORMAT_PR     (9)
 * # define MTD_NUMBER_FORMAT_RN     (10)
 * # define MTD_NUMBER_FORMAT_S      (11)
 * # define MTD_NUMBER_FORMAT_V      (12)
 * # define MTD_NUMBER_FORMAT_XXXX   (13)
 *
 * Implementation :
 *
 ***********************************************************************/
    UShort sCommaCnt   = 0;
    UShort sPeriodCnt  = 0;
    UShort sDollarCnt  = 0;
    UShort sZeroCnt    = 0;
    UShort sNineCnt    = 0;
    UShort sBCnt       = 0;
    UShort sEEEECnt    = 0;
    UShort sMICnt      = 0;
    UShort sPRCnt      = 0;
    UShort sSCnt       = 0;
    UShort sXXXXCnt    = 0;
    UShort sCCnt       = 0;
    UShort sLCnt       = 0;
    UShort sGCnt       = 0;
    UShort sDCnt       = 0;

    idBool sIsPeriod   = ID_FALSE;
    idBool sIsFirstS   = ID_FALSE;
    UShort sIntZeroCnt = 0;
    UShort sIntNineCnt = 0;

    UShort sFormatIndex = 0;
    UChar* sFormat      = aFmt;
    UInt   sFormatLen   = aLength;

    while ( sFormatLen > 0 )
    {
        if ( idlOS::strMatch( sFormat, 1, ",", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sPeriodCnt != 0 ) || ( sFormatIndex == 0 ) ||
                            ( sNineCnt + sZeroCnt == 0 ) ||
                            ( sGCnt != 0 ) || ( sDCnt != 0 ),
                            ERR_INVALID_LITERAL );
            sCommaCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strMatch( sFormat, 1, ".", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sPeriodCnt != 0 ) || ( sDCnt != 0 ) ||
                            ( sGCnt != 0 ),
                            ERR_INVALID_LITERAL );

            sIsPeriod = ID_TRUE;
            sPeriodCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strMatch( sFormat, 1, "$", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sDollarCnt != 0 ) ||
                            ( sLCnt != 0 )      ||
                            ( sCCnt != 0 ),
                            ERR_INVALID_LITERAL );
            sDollarCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strMatch( sFormat, 1, "0", 1 ) == 0 )
        {
            if ( ( sIsPeriod == ID_FALSE ) && ( sDCnt == 0 ) )
            {
                sIntZeroCnt++;
            }
            sZeroCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strMatch( sFormat, 1, "9", 1 ) == 0 )
        {
            if ( ( sIsPeriod == ID_FALSE ) && ( sDCnt == 0 ) )
            {
                sIntNineCnt++;
            }
            sNineCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 1, "B", 1 ) == 0 )
        {
            IDE_TEST_RAISE( sBCnt == 1, ERR_INVALID_LITERAL );

            sBCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 2, "MI", 2 ) == 0 )
        {
            if ( sFormatIndex != aLength - 2 || 
                 sSCnt != 0 || sPRCnt != 0 || sMICnt != 0 )
            {
                IDE_RAISE( ERR_INVALID_LITERAL );
            }
            sMICnt++;
            sFormat += 2;
            sFormatLen -= 2;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 2, "PR", 2 ) == 0 )
        {
            if ( sFormatIndex != aLength - 2 || 
                 sSCnt != 0 || sPRCnt != 0 || sMICnt != 0 )
            {
                IDE_RAISE( ERR_INVALID_LITERAL );
            }
            sPRCnt++;
            sFormat += 2;
            sFormatLen -= 2;
            sFormatIndex++;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 1, "S", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sFormatIndex != 0 ) &&
                            ( sFormatIndex != aLength - 1 ),
                            ERR_INVALID_LITERAL );
            IDE_TEST_RAISE( ( sSCnt != 0 ) || ( sPRCnt != 0 ) || ( sMICnt != 0 ),
                            ERR_INVALID_LITERAL );
            if ( sFormatIndex == 0 )
            {
                sIsFirstS = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
            sSCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 4, "XXXX", 4 ) == 0 )
        {
            IDE_TEST_RAISE( aLength != 4, ERR_INVALID_LITERAL );

            sXXXXCnt++;
            sFormat += 4;
            sFormatLen -= 4;
            sFormatIndex += 3;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 1, "C", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sLCnt != 0 ) || ( sCCnt != 0 ) ||
                            ( sDollarCnt != 0 ),
                            ERR_INVALID_LITERAL );
            if ( ( sFormatIndex > 0 ) &&
                 ( sFormatIndex < aLength -1 ))
            {
                if ( sIsFirstS == ID_FALSE )
                {
                    if ( sFormatLen > 3 )
                    {
                        IDE_RAISE( ERR_INVALID_LITERAL );
                    }
                    else if ( sFormatLen == 3 )
                    {
                        if ( (idlOS::strCaselessMatch( sFormat + 1, 2,
                                                       "PR", 2 ) != 0 ) &&
                             ( idlOS::strCaselessMatch( sFormat + 1, 2,
                                                       "MI", 2 ) != 0 ) )
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    } else if ( sFormatLen == 2 )
                    {
                        if ( idlOS::strCaselessMatch( sFormat + 1, 1,
                                                      "S", 1 ) != 0 )
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        IDE_RAISE( ERR_INVALID_LITERAL );
                    }
                }
                else
                {
                    IDE_TEST_RAISE( sFormatIndex != 1, ERR_INVALID_LITERAL );
                }
            }
            else
            {
                /* Nothing to do */
            }
            sCCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 1, "L", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sLCnt != 0 ) || ( sCCnt != 0 ) ||
                            ( sDollarCnt != 0 ),
                            ERR_INVALID_LITERAL );
            if ( ( sFormatIndex > 0 ) &&
                 ( sFormatIndex < aLength -1 ))
            {
                if ( sIsFirstS == ID_FALSE )
                {
                    if ( sFormatLen > 3 )
                    {
                        IDE_RAISE( ERR_INVALID_LITERAL );
                    }
                    else if ( sFormatLen == 3 )
                    {
                        if ( (idlOS::strCaselessMatch( sFormat + 1, 2,
                                                       "PR", 2 ) != 0 ) &&
                             ( idlOS::strCaselessMatch( sFormat + 1, 2,
                                                       "MI", 2 ) != 0 ) )
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    } else if ( sFormatLen == 2 )
                    {
                        if ( idlOS::strCaselessMatch( sFormat + 1, 1,
                                                      "S", 1 ) != 0 )
                        {
                            IDE_RAISE( ERR_INVALID_LITERAL );
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        IDE_RAISE( ERR_INVALID_LITERAL );
                    }
                }
                else
                {
                    IDE_TEST_RAISE( sFormatIndex != 1, ERR_INVALID_LITERAL );
                }
            }
            else
            {
                /* Nothing to do */
            }
            sLCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 1, "G", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sPeriodCnt != 0 ) || ( sFormatIndex == 0 ) ||
                            ( sNineCnt + sZeroCnt == 0 ) ||
                            ( sCommaCnt != 0 ) || ( sDCnt != 0 ),
                            ERR_INVALID_LITERAL );

            sGCnt++;
            sFormat++;
            sFormatLen--;
        }
        else if ( idlOS::strCaselessMatch( sFormat, 1, "D", 1 ) == 0 )
        {
            IDE_TEST_RAISE( ( sDCnt != 0 ) || ( sPeriodCnt != 0 ) ||
                            ( sCommaCnt != 0 ),
                            ERR_INVALID_LITERAL );
            sDCnt++;
            sFormat++;
            sFormatLen--;
        }
        else 
        {
            IDE_RAISE( ERR_INVALID_LITERAL );
        }
        sFormatIndex++;
    }

    aToken[MTD_NUMBER_FORMAT_COMMA]  = sCommaCnt;
    aToken[MTD_NUMBER_FORMAT_PERIOD] = sPeriodCnt;
    aToken[MTD_NUMBER_FORMAT_DOLLAR] = sDollarCnt;
    aToken[MTD_NUMBER_FORMAT_ZERO]   = sZeroCnt;
    aToken[MTD_NUMBER_FORMAT_NINE]   = sNineCnt;
    aToken[MTD_NUMBER_FORMAT_B]      = sBCnt;
    aToken[MTD_NUMBER_FORMAT_EEEE]   = sEEEECnt;
    aToken[MTD_NUMBER_FORMAT_MI]     = sMICnt;
    aToken[MTD_NUMBER_FORMAT_PR]     = sPRCnt;
    aToken[MTD_NUMBER_FORMAT_S]      = sSCnt;
    aToken[MTD_NUMBER_FORMAT_XXXX]   = sXXXXCnt;
    aToken[MTD_NUMBER_FORMAT_C]      = sCCnt;
    aToken[MTD_NUMBER_FORMAT_L]      = sLCnt;
    aToken[MTD_NUMBER_FORMAT_G]      = sGCnt;
    aToken[MTD_NUMBER_FORMAT_D]      = sDCnt;
    aToken[MTD_COUNT_ZERO]           = sIntZeroCnt;
    aToken[MTD_COUNT_NINE]           = sIntNineCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
