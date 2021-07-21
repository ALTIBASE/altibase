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
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule stvNull2Geometry;
extern mtdModule stdGeometry;
extern mtdModule mtdNull;

static IDE_RC stvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC stvCalculate_Null2Geometry( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

mtvModule stvNull2Geometry = {
    &stdGeometry,
    &mtdNull,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    stvEstimate
};

static const mtcExecute stvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    stvCalculate_Null2Geometry,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC stvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
#define IDE_FN "IDE_RC stvEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = stvExecute;
    
    //IDE_TEST( stdGeometry.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC stvCalculate_Null2Geometry( mtcNode*,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* )
{
    stdGeometry.null( aStack[0].column,
                      aStack[0].value );

    return IDE_SUCCESS;
}
 
