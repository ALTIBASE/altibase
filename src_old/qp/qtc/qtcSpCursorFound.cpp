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
 * $Id: qtcSpCursorFound.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsvCursor.h>
#include <qsxCursor.h>
#include <qsxRefCursor.h>
#include <qcuSqlSourceInfo.h>

static mtcName mtfFunctionName[1] = {
    { NULL, 8, (void*)"CURFOUND" }
};

static IDE_RC mtfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qtc::spCursorFoundModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_COMPARISON_TRUE|MTC_NODE_DML_UNUSABLE,
    ~0,
    1.0,              // TODO : default selectivity 
    mtfFunctionName,
    & qtc::spCursorNotFoundModule,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfEstimate
};


IDE_RC mtfCalculate_SpCusorFound( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfCalculate_SpRefCusorFound( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCalculate_SpCusorFound,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteRefCursor = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCalculate_SpRefCusorFound,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    extern mtdModule mtdBoolean;
    qtcCallBackInfo* sInfo = (qtcCallBackInfo*)aCallBack->info;
    qcuSqlSourceInfo sqlInfo;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE((aNode->lflag & MTC_NODE_BIND_MASK) != MTC_NODE_BIND_ABSENT,
                    ERR_INVALID_FUNCTION_ARGUMENT_HOSTVAR );

    if( aStack[1].column->module->id == MTD_REF_CURSOR_ID )
    {
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdBoolean,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteRefCursor;
    }
    else
    {
        if ( ( aNode->arguments->lflag & MTC_NODE_DML_MASK ) !=
             MTC_NODE_DML_UNUSABLE)  // arguments is not cursor.
        {
            sqlInfo.setSourceInfo( sInfo->statement,
                                   & ((qtcNode *)aNode)->position );
            IDE_RAISE(ERR_NOT_CURSOR);
        }

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdBoolean,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT_HOSTVAR );
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QTC_NO_HOSTVAR_ALLOWED));

    IDE_EXCEPTION( ERR_NOT_CURSOR );
    {
        (void)sqlInfo.init(sInfo->statement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_CURSOR,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfCalculate_SpCusorFound( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfCalculate_SpCusorFound"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( QSX_CURSOR_IS_FOUND( (qsxCursorInfo *) aStack[1].value ) == ID_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC mtfCalculate_SpRefCusorFound( mtcNode*     aNode,
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
    
    if( QSX_REF_CURSOR_IS_FOUND( (qsxRefCursorInfo *) aStack[1].value ) == ID_TRUE )
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_TRUE;
    }
    else
    {
        *(mtdBooleanType*)aStack[0].value = MTD_BOOLEAN_FALSE;
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
