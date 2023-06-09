/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfWKB.h>
#include <mtdTypes.h>
#include <stdTypes.h> // BUG-24594
#include <qc.h>

extern mtfModule stfPointFromWKB;

static mtcName stfPointFromWKBFunctionName[2] = {
    { stfPointFromWKBFunctionName+1, 15, (void*)"ST_POINTFROMWKB" }, // Fix BUG-15519
    { NULL, 12, (void*)"POINTFROMWKB" } // Fix BUG-15247
};

static IDE_RC stfPointFromWKBEstimate(
                        mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        mtcCallBack* aCallBack );

mtfModule stfPointFromWKB = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfPointFromWKBFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfPointFromWKBEstimate
};

IDE_RC stfPointFromWKBCalculate(
                        mtcNode*     aNode,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        void*        aInfo,
                        mtcTemplate* aTemplate );

static const mtcExecute stfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stfPointFromWKBCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfPointFromWKBEstimate(
                        mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt      /* aRemain */,
                        mtcCallBack* aCallBack )
{
    mtcNode* sNode;
    ULong    sLflag;

    extern mtdModule stdGeometry;
    extern mtdModule mtdBinary;   // Fix BUG-15834

    const mtdModule* sModules[1];

    *sModules = &mtdBinary;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for( sNode  = aNode->arguments, sLflag = MTC_NODE_INDEX_UNUSABLE;
         sNode != NULL;
         sNode  = sNode->next )
    {
        if( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) ==
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

    // Fix Bug-24594
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     STD_POINT2D_SIZE,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stfPointFromWKBCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    IDE_RC sRC;
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

    // Fix BUG-24514 WKB Null problem.
    if (aStack[1].column->module->isNull(aStack[1].column,
                                         aStack[1].value) == ID_TRUE)
    {
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
        
        // Fix BUG-16448
        IDE_TEST( stfWKB::pointFromWKB(
                      sQmxMem,
                      aStack[1].value,
                      aStack[0].value,
                      (SChar*)(aStack[0].value) +
                      aStack[0].column->column.size,
                      &sRC,
                      STU_VALIDATION_ENABLE ) != IDE_SUCCESS);

        // Memory 재사용을 위한 Memory 이동
        sStage = 0;
        IDE_TEST( sQmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS);  
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sQmxMem->setStatus(&sQmxMemStatus);
    }     
    
    return IDE_FAILURE;
}
 
