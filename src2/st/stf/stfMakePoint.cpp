/***********************************************************************
 * Copyright 1999-2017, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: stfMakePoint.cpp 87649 2020-05-29 08:09:47Z ykim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfFunctions.h>
#include <mtdTypes.h>

extern mtfModule stfMakePoint;

static mtcName stfMakePointFunctionName[2] = {
    { stfMakePointFunctionName+1, 12, (void*)"ST_MAKEPOINT" },
    { NULL, 8, (void*)"ST_POINT" }
};

static IDE_RC stfMakePointEstimate( mtcNode     * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack    * aStack,
                                    SInt          aRemain,
                                    mtcCallBack * aCallBack );

mtfModule stfMakePoint = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfMakePointFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfMakePointEstimate
};

IDE_RC stfMakePointCalculate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfMakePointCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfMakePointEstimate( mtcNode     * aNode,
                             mtcTemplate * aTemplate,
                             mtcStack    * aStack,
                             SInt       /* aRemain */,
                             mtcCallBack * aCallBack )
{
    mtcNode* sNode;
    ULong    sLflag;

    extern mtdModule mtdDouble;
    extern mtdModule stdGeometry;

    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = & mtdDouble;
    sModules[1] = & mtdDouble;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    for ( sNode  = aNode->arguments, sLflag = MTC_NODE_INDEX_UNUSABLE;
          sNode != NULL;
          sNode  = sNode->next )
    {
        if ( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
            MTC_NODE_COMPARISON_TRUE )
        {
            sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
        }
        sLflag |= sNode->lflag & MTC_NODE_INDEX_MASK;
    }

    aNode->lflag &= ~(MTC_NODE_INDEX_MASK);
    aNode->lflag |= sLflag;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = stfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     STD_POINT2D_SIZE,
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

IDE_RC stfMakePointCalculate( mtcNode*     aNode,
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

    IDE_TEST( stfFunctions::makePoint( aNode,
                                       aStack,
                                       aRemain,
                                       aInfo,
                                       NULL )
              != IDE_SUCCESS )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 
