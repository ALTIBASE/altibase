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
#include <mtdTypes.h>
#include <stdTypes.h>
#include <stfWKT.h>
#include <qc.h>

extern mtfModule stfRectFromText;

static mtcName stfRectFromTextFunctionName[2] = {
    { stfRectFromTextFunctionName + 1, 15, (void *)"ST_RECTFROMTEXT" },
    { NULL,                            12, (void *)"RECTFROMTEXT" }
};

static IDE_RC stfRectFromTextEstimate(
                        mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt         aRemain,
                        mtcCallBack* aCallBack );

mtfModule stfRectFromText = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfRectFromTextFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfRectFromTextEstimate
};

IDE_RC stfRectFromTextCalculate(
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
    stfRectFromTextCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfRectFromTextEstimate( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt          /* aRemain */,
                                mtcCallBack * aCallBack )
{
    mtcNode * sNode  = NULL;
    ULong     sLflag = MTC_NODE_INDEX_UNUSABLE;

    extern mtdModule   stdGeometry;

    const mtdModule  * sModules[1] = { NULL };

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) == MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for ( sNode  = aNode->arguments;
          sNode != NULL;
          sNode  = sNode->next )
    {
        if ( ( sNode->lflag & MTC_NODE_COMPARISON_MASK ) == MTC_NODE_COMPARISON_TRUE )
        {
            sNode->lflag &= ~(MTC_NODE_INDEX_MASK);
        }
        else
        {
            /* Nothing to do */
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

    // RECTANGLE(x1 y1, x2 y2) -> POLYGON((x1 y1, x2 y1, x2 y2, x1 y2, x1 y1))
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     1,
                                     ( STD_PT2D_SIZE * 5 ) + STD_RN2D_SIZE + STD_POLY2D_SIZE,
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

IDE_RC stfRectFromTextCalculate( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate )
{
    IDE_RC            sRC           = IDE_SUCCESS;
    qcTemplate      * sQcTmplate    = NULL;
    iduMemory       * sQmxMem       = NULL;
    iduMemoryStatus   sQmxMemStatus;
    idBool            sSRIDOption = ID_FALSE;
    SInt              sSRID = ST_SRID_INIT;
    UInt              sStage        = 0;

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

        sQcTmplate = (qcTemplate *)aTemplate;
        sQmxMem    = QC_QMX_MEM( sQcTmplate->stmt );

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( sQmxMem->getStatus( &sQmxMemStatus ) != IDE_SUCCESS );
        sStage = 1;

        // PROJ-2422 srid 지원
        if ( (aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 )
        {
            if ( aStack[2].column->module->isNull( aStack[2].column,
                                                   aStack[2].value ) == ID_FALSE )
            {
                sSRIDOption = ID_TRUE;
                sSRID = *(mtdIntegerType*)aStack[2].value;
            }
            else
            {
                // Nothing to do.
            }
        }

        IDE_TEST( stfWKT::rectFromText( sQmxMem,
                                        aStack[1].value,
                                        aStack[0].value,
                                        (SChar *)(aStack[0].value) + aStack[0].column->column.size,
                                        &sRC,
                                        STU_VALIDATION_ENABLE,
                                        sSRIDOption,
                                        sSRID )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRC != IDE_SUCCESS, ERR_INVALID_LITERAL );

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
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

