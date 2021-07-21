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
 * $Id: stfIsCollection.cpp 88396 2020-08-21 02:10:02Z ykim $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <stfFunctions.h>
#include <mtdTypes.h>
#include <stdUtils.h>
#include <ste.h>

extern mtfModule stfIsCollection;
extern mtdModule mtdInteger;  
extern mtdModule stdGeometry;  

static mtcName stfIsCollectionFunctionName[1] = {
    { NULL, 15, (void*)"ST_ISCOLLECTION" }
};

static IDE_RC stfIsCollectionEstimate( mtcNode *     aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack *    aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule stfIsCollection = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    stfIsCollectionFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    stfIsCollectionEstimate
};

IDE_RC stfIsCollectionCalculate(
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
    stfIsCollectionCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC stfIsCollectionEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt      /* aRemain */,
                                mtcCallBack* aCallBack )
{
    const  mtdModule     * sModules[1];

    sModules[0] = &stdGeometry;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = stfExecute;
        
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
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

IDE_RC stfIsCollectionCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    stdGeometryHeader   * sValue = NULL;
    mtdIntegerType      * sRet   = NULL;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sRet = (mtdIntegerType*) aStack[0].value;
    sValue = (stdGeometryHeader*) aStack[1].value;
    
    // Fix BUG-15412 mtdModule.isNull 사용
    if( stdGeometry.isNull( NULL, sValue ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        switch( sValue->mType )
        {
            case STD_POINT_2D_EXT_TYPE :
            case STD_POINT_2D_TYPE :
            case STD_LINESTRING_2D_EXT_TYPE :
            case STD_LINESTRING_2D_TYPE :
            case STD_POLYGON_2D_EXT_TYPE :
            case STD_POLYGON_2D_TYPE :
            case STD_EMPTY_TYPE :
                *sRet = 0;
                break;
            case STD_MULTIPOINT_2D_EXT_TYPE :
            case STD_MULTIPOINT_2D_TYPE :
            case STD_MULTILINESTRING_2D_EXT_TYPE :
            case STD_MULTILINESTRING_2D_TYPE :
            case STD_MULTIPOLYGON_2D_EXT_TYPE :
            case STD_MULTIPOLYGON_2D_TYPE :
            case STD_GEOCOLLECTION_2D_EXT_TYPE:
            case STD_GEOCOLLECTION_2D_TYPE:
                *sRet = 1;
                break; 
            default:
                IDE_RAISE( err_invalid_object_mType );
        }
        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_object_mType);
    {
	    IDE_SET(ideSetErrorCode(stERR_ABORT_OBJECT_TYPE_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
