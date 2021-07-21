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

extern mtvModule stvUndef2Geometry;

extern mtdModule mtdUndef;
extern mtdModule stdGeometry;

static IDE_RC stvEstimateGeometry( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

IDE_RC stvCalculate_Undef2All( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

mtvModule stvUndef2Geometry = {
    &stdGeometry,
    &mtdUndef,
    0,
    stvEstimateGeometry
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,          // initialize
    mtf::calculateNA,          // aggregate
    mtf::calculateNA,
    mtf::calculateNA,          // finalize
    stvCalculate_Undef2All,      // calculate
    NULL,                      // additional info for calculate (NA)
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC stvEstimateGeometry( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & stdGeometry,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC stvCalculate_Undef2All( mtcNode*,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* )
{
    // undef 타입은 실제 conversion 이 실행되면 안되므로 ASSERT 처리한다.
    IDE_ASSERT(0);

    return IDE_SUCCESS;
}
