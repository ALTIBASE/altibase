/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <sdf.h>

extern mtfModule sdfShardKey;

static mtcName sdfShardKeyFunctionName[1] = {
    { NULL, 9, (void*)"SHARD_KEY" }
};

static IDE_RC sdfShardKeyEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule sdfShardKey = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_COMPARISON_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity
    sdfShardKeyFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfShardKeyEstimate
};

IDE_RC sdfShardKeyCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute sdfShardKeyExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfShardKeyCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC sdfShardKeyEstimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt         /*aRemain*/,
                            mtcCallBack* /*aCallBack*/ )
{
    extern mtdModule mtdBoolean;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aTemplate->rows[aNode->table].execute[aNode->column] = sdfShardKeyExecute;

    // 첫번째 인자는 컬럼이어야 한다.
    IDE_TEST_RAISE(
        ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
          == MTC_TUPLE_VIEW_TRUE ) ||
        ( idlOS::strncmp((SChar*)aNode->arguments->module->names->string,
                         (const SChar*)"COLUMN", 6 ) != 0 ),
        ERR_ARGUMENT_NOT_APPLICABLE );

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfShardKeyCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        /*aInfo*/,
                             mtcTemplate* aTemplate )
{
    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    *(mtdBooleanType*)aStack->value = MTD_BOOLEAN_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
