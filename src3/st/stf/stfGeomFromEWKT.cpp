/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfEWKT.h>
#include <stdTypes.h>
#include <qc.h>

extern mtfModule stfGeomFromEWKT;

static mtcName stfGeomFromEWKTFunctionName[2] = {
    { stfGeomFromEWKTFunctionName+1, 15, (void*)"ST_GEOMFROMEWKT" }, // Fix BUG-15519
    { NULL, 12, (void*)"GEOMFROMEWKT" }
};

static IDE_RC stfGeomFromEWKTEstimate( mtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

mtfModule stfGeomFromEWKT = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfGeomFromEWKTFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfGeomFromEWKTEstimate
};

IDE_RC stfGeomFromEWKTCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfGeomFromEWKTCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfGeomFromEWKTEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt      /* aRemain */,
                                mtcCallBack* aCallBack )
{
    mtcNode* sNode;
    ULong    sLflag;

    extern mtdModule stdGeometry;

    const mtdModule* sModules[1];

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for ( sNode  = aNode->arguments, sLflag = MTC_NODE_INDEX_UNUSABLE;
          sNode != NULL;
          sNode  = sNode->next )
    {
        if ( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
             MTC_NODE_COMPARISON_TRUE )
        {
            sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
        }
        else
        {
            // Nothing To Do
        }
        sLflag |= sNode->lflag & MTC_NODE_INDEX_MASK;
    }

    aNode->lflag &= ~(MTC_NODE_INDEX_MASK);
    aNode->lflag |= sLflag;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

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
                                     STD_GEOMETRY_PRECISION_FOR_WKT,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfGeomFromEWKTCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    IDE_RC            rc    = IDE_FAILURE;
    qcTemplate      * sQcTmplate;
    iduMemory       * sQmxMem;
    iduMemoryStatus   sQmxMemStatus;
    UInt              sStage = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        // To Fix BUG-16444
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sQcTmplate = (qcTemplate*) aTemplate;
        sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( sQmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS);
        sStage = 1;
        
        IDE_TEST( stfEWKT::geomFromEWKT( sQmxMem,
                                         aStack[1].value,
                                         aStack[0].value,
                                         (SChar*)(aStack[0].value) +aStack[0].column->column.size,
                                         &rc,
                                         STU_VALIDATION_ENABLE ) != IDE_SUCCESS );

        IDE_TEST_RAISE( rc != IDE_SUCCESS, ERR_INVALID_LITERAL );

        // Memory 재사용을 위한 Memory 이동
        sStage = 0;
        IDE_TEST( sQmxMem->setStatus( &sQmxMemStatus ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sQmxMem->setStatus( &sQmxMemStatus );
    }
    else
    {
        // Nothing To Do
    }
    
    return IDE_FAILURE;
}
 
