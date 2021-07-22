/***********************************************************************
 * Copyright 1999-2017, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: stfMakeEnvelope.cpp 90194 2021-03-12 04:22:26Z donovan.seo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfFunctions.h>
#include <mtdTypes.h>
#include <stfBasic.h>

extern mtdModule mtdDouble;
extern mtdModule mtdInteger;
extern mtdModule stdGeometry;

extern mtfModule stfMakeEnvelope;

static mtcName stfMakeEnvelopeFunctionName[2] = {
    { stfMakeEnvelopeFunctionName+1, 15, (void*)"ST_MAKEENVELOPE" },
    { NULL, 12, (void*)"MAKEENVELOPE" }
};

static IDE_RC stfMakeEnvelopeEstimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule stfMakeEnvelope = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfMakeEnvelopeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfMakeEnvelopeEstimate
};

IDE_RC stfMakeEnvelopeCalculate( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate );

IDE_RC stfMakeEnvelopeCalculate5Args( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfMakeEnvelopeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


static const mtcExecute stfExecute5Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfMakeEnvelopeCalculate5Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfMakeEnvelopeEstimate( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt       /* aRemain */,
                                mtcCallBack * aCallBack )
{
    const mtdModule* sModules[5];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) < 4) ||
                      ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) > 5)),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 4 )
    { 
        sModules[0] = & mtdDouble;
        sModules[1] = & mtdDouble;
        sModules[2] = & mtdDouble;
        sModules[3] = & mtdDouble;

        aTemplate->rows[aNode->table].execute[aNode->column] = stfExecute;
    }
    else
    {
        // BUG-47919 add SRID argument in ST_MAKEENVELOPE function
        sModules[0] = & mtdDouble;
        sModules[1] = & mtdDouble;
        sModules[2] = & mtdDouble;
        sModules[3] = & mtdDouble;
        sModules[4] = & mtdInteger;

        aTemplate->rows[aNode->table].execute[aNode->column] = stfExecute5Args;
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // MAKEENVELOPE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
    // STD_POLY2D_SIZE == STD_POLY2D_EXT_SIZE
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     ( STD_PT2D_SIZE * 5 ) + STD_RN2D_SIZE + STD_POLY2D_SIZE,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfMakeEnvelopeCalculate(
                        mtcNode*     aNode,
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

    IDE_TEST( stfFunctions::makeEnvelope( aNode,
                                          aStack,
                                          aRemain,
                                          aInfo,
                                          aTemplate )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfMakeEnvelopeCalculate5Args(
    mtcNode*     aNode,
    mtcStack*    aStack,
    SInt         aRemain,
    void*        aInfo,
    mtcTemplate* aTemplate )
{
    stdGeometryHeader     * sRet;
    mtdDoubleType         * sX1;
    mtdDoubleType         * sY1;
    mtdDoubleType         * sX2;
    mtdDoubleType         * sY2;
    mtdIntegerType        * sSRID;  

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sRet  = (stdGeometryHeader *)aStack[0].value;
    sX1   = (mtdDoubleType *)aStack[1].value;
    sY1   = (mtdDoubleType *)aStack[2].value;
    sX2   = (mtdDoubleType *)aStack[3].value;
    sY2   = (mtdDoubleType *)aStack[4].value;
    sSRID = (mtdIntegerType *)aStack[5].value;

    if ( ( mtdDouble.isNull( NULL, sX1 ) == ID_TRUE ) ||
         ( mtdDouble.isNull( NULL, sY1 ) == ID_TRUE ) ||
         ( mtdDouble.isNull( NULL, sX2 ) == ID_TRUE ) ||
         ( mtdDouble.isNull( NULL, sY2 ) == ID_TRUE ) || 
         ( mtdInteger.isNull( NULL, sSRID ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        IDE_TEST( stfBasic::getRectangle( aTemplate,
                                          * sX1,
                                          * sY1,
                                          * sX2,
                                          * sY2,
                                          sRet )
                  != IDE_SUCCESS );

        // sRet는 NULL일 수 없다.
        IDE_TEST( stfBasic::setSRID( sRet,
                                     aStack[0].column->precision,
                                     *sSRID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
