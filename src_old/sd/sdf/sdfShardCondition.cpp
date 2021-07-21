/***********************************************************************
 * Copyright 1999-2015, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 *
 * Description :
 *
 *     ALTIBASE SHARD manage ment function
 *
 * Syntax :
 *    SHARD_CONDITION(i1, '');
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>
#include <sdi.h>
#include <sdpjManager.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdBinary;

static mtcName sdfFunctionName[1] = {
    { NULL, 15, (void*)"SHARD_CONDITION" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

static IDE_RC sdfConvertObjectToAnalyzeInfo( SChar           * aCondition,
                                             const mtdModule * aKeyModule,
                                             const mtdModule * aSubKeyModule,
                                             sdpjObject      * aObject,
                                             sdiAnalyzeInfo  * aAnalyzeInfo,
                                             sdiNodeInfo     * aNodeInfo );

static IDE_RC sdfConvertArrayToRangeInfo( SChar          * aCondition,
                                          sdpjArray      * aArray,
                                          SInt           * aKeyCount,
                                          sdiNodeInfo    * aNodeInfo,
                                          sdiAnalyzeInfo * aAnalyzeInfo );

static IDE_RC sdfFindAndAddDataNode( SChar        * aCondition,
                                     sdpjString   * aString,
                                     sdiNodeInfo  * aNodeInfo,
                                     UInt         * aNodeId );

static IDE_RC sdfSetSplitMethod( SChar            aSplitMethodText,
                                 sdiSplitMethod * aSplitMethod );

static IDE_RC sdfSetValue( SChar          * aCondition,
                           sdpjString     * aString,
                           sdiSplitMethod   aSplitMethod,
                           UInt             aKeyType,
                           sdiValue       * aValue );

mtfModule sdfShardConditionModule = {
    4|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE|MTC_NODE_EAT_NULL_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_ShardCondition( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_ShardCondition,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC sdfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /*aRemain*/,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];
    UInt             sArgCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
    UInt             sKeyType;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    // 첫번째 인자는 컬럼이어야 한다.
    //IDE_TEST_RAISE(
    //    ( ( aTemplate->rows[aNode->arguments->table].lflag & MTC_TUPLE_VIEW_MASK )
    //      == MTC_TUPLE_VIEW_TRUE ) ||
    //    ( idlOS::strncmp((SChar*)aNode->arguments->module->names->string,
    //                     (const SChar*)"COLUMN", 6 ) != 0 ),
    //    ERR_ARGUMENT_NOT_APPLICABLE );

    sKeyType = aStack[1].column->module->id;
    IDE_TEST_RAISE( ( sKeyType != MTD_SMALLINT_ID ) &&
                    ( sKeyType != MTD_INTEGER_ID  ) &&
                    ( sKeyType != MTD_BIGINT_ID   ) &&
                    ( sKeyType != MTD_CHAR_ID     ) &&
                    ( sKeyType != MTD_VARCHAR_ID  ),
                    ERR_INVALID_SHARD_KEY_TYPE );

    if ( sArgCount == 2 )
    {
        sModules[0] = aStack[1].column->module;
        sModules[1] = &mtdVarchar;
    }
    else if ( sArgCount == 3 )
    {
        // 두번째 인자도 컬럼이어야 한다.
        //IDE_TEST_RAISE(
        //    ( ( aTemplate->rows[aNode->arguments->next->table].lflag & MTC_TUPLE_VIEW_MASK )
        //      == MTC_TUPLE_VIEW_TRUE ) ||
        //    ( idlOS::strncmp((SChar*)aNode->arguments->next->module->names->string,
        //                     (const SChar*)"COLUMN", 6 ) != 0 ),
        //    ERR_ARGUMENT_NOT_APPLICABLE );

        sKeyType = aStack[2].column->module->id;
        IDE_TEST_RAISE( ( sKeyType != MTD_SMALLINT_ID ) &&
                        ( sKeyType != MTD_INTEGER_ID  ) &&
                        ( sKeyType != MTD_BIGINT_ID   ) &&
                        ( sKeyType != MTD_CHAR_ID     ) &&
                        ( sKeyType != MTD_VARCHAR_ID  ),
                        ERR_INVALID_SHARD_KEY_TYPE );

        sModules[0] = aStack[1].column->module;
        sModules[1] = aStack[2].column->module;
        sModules[2] = &mtdVarchar;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_FUNCTION_ARGUMENT );
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     1,
                                     SDI_NODE_NAME_MAX_SIZE,
                                     0 )
              != IDE_SUCCESS );

    /* BUG-45718
     * ( ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT ) +
     * ( ID_SIZEOF(sdiValueInfo) * 2 )
     * 추가
     */
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF(sdiAnalyzeInfo) +
                                     ( ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT ) +
                                     ( ID_SIZEOF(sdiValueInfo*) + ID_SIZEOF(sdiValueInfo) ) + // sdiValueInfo->mValuePtrArray
                                     ( ID_SIZEOF(sdiValueInfo*) + ID_SIZEOF(sdiValueInfo) ) + // sdiValueInfo->mSubValuePtrArray
                                     ID_SIZEOF(sdiNodeInfo),
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBinary,
                                     1,
                                     1024 * 40,  // 40KB parsing memory buffer
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdBinary,
                                     1,
                                     aStack[sArgCount].column->precision,  // condition backup
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = sdfExecute;

    /* BUG-44740 mtfRegExpression 재사용을 위해 Tuple Row를 초기화한다. */
    aTemplate->rows[aNode->table].lflag &= ~MTC_TUPLE_ROW_MEMSET_MASK;
    aTemplate->rows[aNode->table].lflag |= MTC_TUPLE_ROW_MEMSET_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    //IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    //IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE );
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE, "(","",")" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfCalculate_ShardCondition( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     Shard Condition
 *
 * Implementation :
 *
 ***********************************************************************/

    qcTemplate       * sQcTemplate   = (qcTemplate*)aTemplate;
    mtcTuple         * sKeyTuple     = NULL;
    mtcTuple         * sSubKeyTuple  = NULL;
    const mtcColumn  * sColumn       = NULL;
    const mtdModule  * sKeyModule    = NULL;
    const mtdModule  * sSubKeyModule = NULL;
    mtdCharType      * sCondition;
    mtdCharType      * sResult;
    mtdBinaryType    * sBinary;
    sdiAnalyzeInfo   * sAnalyzeInfo;
    sdiRange         * sRanges;
    sdiValueInfo     * sValue;
    sdiValueInfo    ** sValuePtr = NULL;
    sdiValueInfo     * sSubValue;
    sdiValueInfo    ** sSubValuePtr = NULL;
    sdiNodeInfo      * sNodeInfo;
    sdiNode          * sNode;
    mtdBinaryType    * sParserBuffer;
    sdpjObject       * sObject;
    UInt               sArgCount;
    UInt               sNodeId;
    idBool             sNeedParsing = ID_FALSE;

    /* PROJ-2655 Composite shard key */
    UShort  sRangeIndex[SDI_VALUE_MAX_COUNT];
    UShort  sRangeIndexCount = 0;

    idBool  sExecDefaultNode = ID_FALSE;
    idBool  sIsAllNodeExec  = ID_FALSE;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sArgCount  = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
    sResult    = (mtdCharType*)aStack[0].value;
    sKeyModule = aStack[1].column->module;

    if ( sArgCount == 2 )
    {
        sCondition = (mtdCharType*)aStack[2].value;
    }
    else
    {
        IDE_DASSERT( sArgCount == 3 );

        sSubKeyModule = aStack[2].column->module;
        sCondition    = (mtdCharType*)aStack[3].value;
    }

    sResult->length = 0;

    if ( sCondition->length == 0 )
    {
        // Nothing to do.
    }
    else
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

        sBinary = (mtdBinaryType*)
            ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);
        sAnalyzeInfo = (sdiAnalyzeInfo*)sBinary->mValue;

        /* BUG-45718 */
        sRanges = (sdiRange*)(sBinary->mValue + ID_SIZEOF(sdiAnalyzeInfo));
        sAnalyzeInfo->mRangeInfo.mRanges = sRanges;

        sValue = (sdiValueInfo*)(sBinary->mValue +
                                 ID_SIZEOF(sdiAnalyzeInfo) +
                                 (ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT));
        sValuePtr = (sdiValueInfo**)(sBinary->mValue +
                                     ID_SIZEOF(sdiAnalyzeInfo) +
                                     (ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT) +
                                     ID_SIZEOF(sdiValueInfo));
        *sValuePtr = sValue;
        sAnalyzeInfo->mValuePtrArray = sValuePtr;

        sSubValue = (sdiValueInfo*)(sBinary->mValue +
                                    ID_SIZEOF(sdiAnalyzeInfo) +
                                    (ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT) +
                                    ID_SIZEOF(sdiValueInfo) + ID_SIZEOF(sdiValueInfo*));
        sSubValuePtr = (sdiValueInfo**)(sBinary->mValue +
                                        ID_SIZEOF(sdiAnalyzeInfo) +
                                        (ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT) +
                                        ID_SIZEOF(sdiValueInfo) + ID_SIZEOF(sdiValueInfo*) +
                                        ID_SIZEOF(sdiValueInfo));
        *sSubValuePtr = sSubValue;
        sAnalyzeInfo->mSubValuePtrArray = sSubValuePtr;

        sNodeInfo = (sdiNodeInfo*)(sBinary->mValue +
                                   ID_SIZEOF(sdiAnalyzeInfo) +
                                   (ID_SIZEOF(sdiRange) * SDI_RANGE_MAX_COUNT) +
                                   (ID_SIZEOF(sdiValueInfo) * 2) +
                                   (ID_SIZEOF(sdiValueInfo*) * 2) );

        sParserBuffer = (mtdBinaryType*)
            ((UChar*)aTemplate->rows[aNode->table].row + sColumn[2].column.offset);

        sBinary = (mtdBinaryType*)
            ((UChar*)aTemplate->rows[aNode->table].row + sColumn[3].column.offset);

        // compare backup
        if ( sBinary->mLength != sCondition->length )
        {
            // parsing
            sNeedParsing = ID_TRUE;
        }
        else
        {
            if ( idlOS::memcmp( sBinary->mValue, sCondition->value, sBinary->mLength ) != 0 )
            {
                // parsing
                sNeedParsing = ID_TRUE;
            }
            else
            {
                if ( sAnalyzeInfo->mKeyDataType != sKeyModule->id )
                {
                    // parsing
                    sNeedParsing = ID_TRUE;
                }
                else
                {
                    if ( sSubKeyModule != NULL )
                    {
                        if ( sAnalyzeInfo->mSubKeyDataType != sSubKeyModule->id )
                        {
                            // parsing
                            sNeedParsing = ID_TRUE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }

        //---------------------------------------
        // condition parsing & make analyzeInfo
        //---------------------------------------

        if ( sNeedParsing == ID_TRUE )
        {
            // parsing
            IDE_TEST( sdpjManager::parseIt( (SChar*)sParserBuffer->mValue,
                                            sColumn[2].precision,
                                            (SChar*)sCondition->value,
                                            sCondition->length,
                                            &sObject )
                      != IDE_SUCCESS );

            // make analyzeInfo, NodeInfo
            IDE_TEST( sdfConvertObjectToAnalyzeInfo( (SChar*)sCondition->value,
                                                     sKeyModule,
                                                     sSubKeyModule,
                                                     sObject,
                                                     sAnalyzeInfo,
                                                     sNodeInfo )
                      != IDE_SUCCESS );

            // backup condition
            IDE_DASSERT( sCondition->length <= sColumn[3].precision );

            sBinary->mLength = sCondition->length;
            idlOS::memcpy( sBinary->mValue, sCondition->value, sCondition->length );
        }
        else
        {
            // Nothing to do.
        }

        //---------------------------------------
        // set column value
        //---------------------------------------

        sAnalyzeInfo->mValuePtrCount = 1;
        sAnalyzeInfo->mValuePtrArray[0]->mType = SDI_VALUE_INFO_HOST_VAR;  // column
        sAnalyzeInfo->mValuePtrArray[0]->mDataValueType = MTD_SMALLINT_ID;
        sAnalyzeInfo->mValuePtrArray[0]->mValue.mBindParamId = aNode->arguments->column;
        sKeyTuple = &(aTemplate->rows[aNode->arguments->table]);

        if ( sArgCount == 3 )
        {
            sAnalyzeInfo->mSubValuePtrCount = 1;
            sAnalyzeInfo->mSubValuePtrArray[0]->mType = SDI_VALUE_INFO_HOST_VAR;  // column
            sAnalyzeInfo->mSubValuePtrArray[0]->mDataValueType = MTD_SMALLINT_ID;
            sAnalyzeInfo->mSubValuePtrArray[0]->mValue.mBindParamId = aNode->arguments->next->column;
            sSubKeyTuple = &(aTemplate->rows[aNode->arguments->next->table]);
        }
        else
        {
            // Nothing to do.
        }

        //---------------------------------------
        // set node name
        //---------------------------------------

        IDE_TEST( sdi::getExecNodeRangeIndex( sQcTemplate,
                                              sKeyTuple,
                                              sSubKeyTuple,
                                              sAnalyzeInfo,
                                              sRangeIndex,
                                              &sRangeIndexCount,
                                              &sExecDefaultNode,
                                              &sIsAllNodeExec )
                  != IDE_SUCCESS );

        if ( ( sRangeIndexCount == 0 ) &&
             ( sExecDefaultNode == ID_FALSE ) &&
             ( sIsAllNodeExec == ID_FALSE ) )
        {
            sResult->length = 5;
            idlOS::memcpy( sResult->value, SDM_NA_STR, SDM_NA_STR_SIZE );
        }
        else
        {
            IDE_TEST_RAISE( sIsAllNodeExec == ID_TRUE, ERR_DATA_NODE );
            IDE_TEST_RAISE( sRangeIndexCount > 1, ERR_DATA_NODE );

            if ( sRangeIndexCount == 1 )
            {
                sNodeId = sAnalyzeInfo->mRangeInfo.mRanges[sRangeIndex[0]].mNodeId;
                IDE_DASSERT( sNodeId < sNodeInfo->mCount );
                sNode = sNodeInfo->mNodes + sNodeId;

                sResult->length = idlOS::strlen( sNode->mNodeName );
                idlOS::memcpy( sResult->value,
                               sNode->mNodeName,
                               sResult->length );
            }
            else
            {
                if ( ( sExecDefaultNode == ID_TRUE ) &&
                     ( sAnalyzeInfo->mDefaultNodeId != SDI_NODE_NULL_ID ) )
                {
                    sNodeId = sAnalyzeInfo->mDefaultNodeId;
                    IDE_DASSERT( sNodeId < sNodeInfo->mCount );
                    sNode = sNodeInfo->mNodes + sNodeId;

                    sResult->length = idlOS::strlen( sNode->mNodeName );
                    idlOS::memcpy( sResult->value,
                                   sNode->mNodeName,
                                   sResult->length );
                }
                else
                {
                    sResult->length = 5;
                    idlOS::memcpy( sResult->value, SDM_NA_STR, SDM_NA_STR_SIZE );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DATA_NODE );
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdfCalculate_ShardCondition",
                              "invalid data node" ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfConvertObjectToAnalyzeInfo( SChar           * aCondition,
                                      const mtdModule * aKeyModule,
                                      const mtdModule * aSubKeyModule,
                                      sdpjObject      * aObject,
                                      sdiAnalyzeInfo  * aAnalyzeInfo,
                                      sdiNodeInfo     * aNodeInfo )
{
    sdpjObject   * sObject;
    sdpjKeyValue * sKeyValue;
    sdpjArray    * sArray;
    SInt           sKeyCount = 0;  // not defined
    SChar        * sErrMsg   = (SChar*)"";

    // 초기화
    aAnalyzeInfo->mIsShardQuery     = ID_TRUE;
    aAnalyzeInfo->mSplitMethod      = SDI_SPLIT_NONE;
    aAnalyzeInfo->mKeyDataType      = aKeyModule->id;
    aAnalyzeInfo->mSubKeyExists     = ID_FALSE;
    aAnalyzeInfo->mSubValuePtrCount = 0;
    aAnalyzeInfo->mSubSplitMethod   = SDI_SPLIT_NONE;
    aAnalyzeInfo->mSubKeyDataType   = ID_UINT_MAX;
    aAnalyzeInfo->mDefaultNodeId    = SDI_NODE_NULL_ID;
    aAnalyzeInfo->mRangeInfo.mCount = 0;

    if ( aSubKeyModule != NULL )
    {
        aAnalyzeInfo->mSubKeyExists   = ID_TRUE;
        aAnalyzeInfo->mSubKeyDataType = aSubKeyModule->id;
    }
    else
    {
        // Nothing to do.
    }

    aNodeInfo->mCount = 0;

    // find split method
    // find default node
    // find range info
    for ( sObject = aObject; sObject != NULL; sObject = sObject->mNext )
    {
        sKeyValue = &(sObject->mKeyValue);

        if ( ( sKeyValue->mKey.mSize == 11 ) &&
             ( idlOS::strncasecmp( aCondition + sKeyValue->mKey.mOffset,
                                   "SplitMethod",
                                   11 ) == 0 ) )
        {
            if ( sKeyValue->mValue.mType == SDPJ_STRING )
            {
                sErrMsg = (SChar*)"the number of shard keys is not defined correctly";
                IDE_TEST_RAISE( sKeyCount == 2, ERR_CONVERT );
                IDE_TEST_RAISE( aSubKeyModule != NULL, ERR_CONVERT );
                if ( sKeyCount == 0 )
                {
                    sKeyCount = 1;
                }
                else
                {
                    // Nothing to do.
                }

                sErrMsg = (SChar*)"split method string is too long";
                IDE_TEST_RAISE( sKeyValue->mValue.mValue.mString.mSize != 1, ERR_CONVERT );

                // set split method
                IDE_TEST( sdfSetSplitMethod(
                              aCondition[sKeyValue->mValue.mValue.mString.mOffset],
                              &(aAnalyzeInfo->mSplitMethod) )
                          != IDE_SUCCESS );
            }
            else if ( sKeyValue->mValue.mType == SDPJ_ARRAY )
            {
                sErrMsg = (SChar*)"the number of shard keys is not defined correctly";
                IDE_TEST_RAISE( sKeyCount == 1, ERR_CONVERT );
                IDE_TEST_RAISE( aSubKeyModule == NULL, ERR_CONVERT );
                if ( sKeyCount == 0 )
                {
                    sKeyCount = 2;
                }
                else
                {
                    // Nothing to do.
                }

                sArray = sKeyValue->mValue.mValue.mArray;
                sErrMsg = (SChar*)"split method is not defined";
                IDE_TEST_RAISE( sArray == NULL, ERR_CONVERT );
                sErrMsg = (SChar*)"split method accepts only string";
                IDE_TEST_RAISE( sArray->mValue.mType != SDPJ_STRING, ERR_CONVERT );
                sErrMsg = (SChar*)"split method string is too long";
                IDE_TEST_RAISE( sArray->mValue.mValue.mString.mSize != 1, ERR_CONVERT );

                // set split method
                IDE_TEST( sdfSetSplitMethod(
                              aCondition[sArray->mValue.mValue.mString.mOffset],
                              &(aAnalyzeInfo->mSplitMethod) )
                          != IDE_SUCCESS );

                sArray = sArray->mNext;
                sErrMsg = (SChar*)"sub-split method is not defined";
                IDE_TEST_RAISE( sArray == NULL, ERR_CONVERT );
                sErrMsg = (SChar*)"split method accepts only string";
                IDE_TEST_RAISE( sArray->mValue.mType != SDPJ_STRING, ERR_CONVERT );
                sErrMsg = (SChar*)"split method string is too long";
                IDE_TEST_RAISE( sArray->mValue.mValue.mString.mSize != 1, ERR_CONVERT );

                // set sub-split method
                IDE_TEST( sdfSetSplitMethod(
                              aCondition[sArray->mValue.mValue.mString.mOffset],
                              &(aAnalyzeInfo->mSubSplitMethod) )
                          != IDE_SUCCESS );
            }
            else
            {
                sErrMsg = (SChar*)"split method accepts only string";
                IDE_RAISE( ERR_CONVERT );
            }
        }
        else if ( ( sKeyValue->mKey.mSize == 11 ) &&
                  ( idlOS::strncasecmp( aCondition + sKeyValue->mKey.mOffset,
                                        "DefaultNode",
                                        11 ) == 0 ) )
        {
            sErrMsg = (SChar*)"default node accepts only string";
            IDE_TEST_RAISE( sKeyValue->mValue.mType != SDPJ_STRING, ERR_CONVERT );

            // set default node
            if ( sKeyValue->mValue.mValue.mString.mSize > 0 )
            {
                IDE_TEST( sdfFindAndAddDataNode( aCondition,
                                                 &(sKeyValue->mValue.mValue.mString),
                                                 aNodeInfo,
                                                 &(aAnalyzeInfo->mDefaultNodeId) )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( ( sKeyValue->mKey.mSize == 9 ) &&
                  ( idlOS::strncasecmp( aCondition + sKeyValue->mKey.mOffset,
                                        "RangeInfo",
                                        9 ) == 0 ) )
        {
            sErrMsg = (SChar*)"default node accepts only array";
            IDE_TEST_RAISE( sKeyValue->mValue.mType != SDPJ_ARRAY, ERR_CONVERT );
            sErrMsg = (SChar*)"range infomation is not defined";
            IDE_TEST_RAISE( sKeyValue->mValue.mValue.mArray == NULL, ERR_CONVERT );

            // set range info
            IDE_TEST( sdfConvertArrayToRangeInfo( aCondition,
                                                  sKeyValue->mValue.mValue.mArray,
                                                  &sKeyCount,
                                                  aNodeInfo,
                                                  aAnalyzeInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    // check split method
    sErrMsg = (SChar*)"split method is not defined";
    IDE_TEST_RAISE( aAnalyzeInfo->mSplitMethod == SDI_SPLIT_NONE, ERR_CONVERT );

    // check sub-split method
    if ( sKeyCount == 2 )
    {
        sErrMsg = (SChar*)"sub-split method is not defined";
        IDE_TEST_RAISE( aAnalyzeInfo->mSubSplitMethod == SDI_SPLIT_NONE, ERR_CONVERT );
    }
    else
    {
        // Nothing to do.
    }

    // check range info
    sErrMsg = (SChar*)"range infomation is not defined";
    IDE_TEST_RAISE( aAnalyzeInfo->mRangeInfo.mCount == 0, ERR_CONVERT );

    // sort range info
    IDE_TEST( sdm::shardRangeSort( aAnalyzeInfo->mSplitMethod,
                                   aAnalyzeInfo->mKeyDataType,
                                   aAnalyzeInfo->mSubKeyExists,
                                   aAnalyzeInfo->mSubSplitMethod,
                                   aAnalyzeInfo->mSubKeyDataType,
                                   &(aAnalyzeInfo->mRangeInfo) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(sdERR_ABORT_SDPJ_CONVERT, sErrMsg ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfConvertArrayToRangeInfo( SChar          * aCondition,
                                   sdpjArray      * aArray,
                                   SInt           * aKeyCount,
                                   sdiNodeInfo    * aNodeInfo,
                                   sdiAnalyzeInfo * aAnalyzeInfo )
{
    sdiRangeInfo * sRangeInfo = &(aAnalyzeInfo->mRangeInfo);
    sdpjObject   * sObject;
    sdpjArray    * sArray;
    sdpjArray    * sArray2;
    sdpjKeyValue * sKeyValue;
    SChar        * sErrMsg = (SChar*)"";
    SInt           sValueCount;

    for ( sArray = aArray; sArray != NULL; sArray = sArray->mNext )
    {
        sErrMsg = (SChar*)"range infomation is not defined correctly";
        IDE_TEST_RAISE( sArray->mValue.mType != SDPJ_OBJECT, ERR_CONVERT );

        sErrMsg = (SChar*)"range infomation is too large";
        IDE_TEST_RAISE( sRangeInfo->mCount + 1 >= SDI_RANGE_MAX_COUNT, ERR_CONVERT );

        // init
        sValueCount = 0;
        sRangeInfo->mRanges[sRangeInfo->mCount].mNodeId = ID_UINT_MAX;

        for ( sObject = sArray->mValue.mValue.mObject;
              sObject != NULL;
              sObject = sObject->mNext )
        {
            sKeyValue = &(sObject->mKeyValue);

            // find value
            // find node
            if ( ( sKeyValue->mKey.mSize == 5 ) &&
                 ( idlOS::strncasecmp( aCondition + sKeyValue->mKey.mOffset,
                                       "Value",
                                       5 ) == 0 ) )
            {
                if ( sKeyValue->mValue.mType == SDPJ_STRING )
                {
                    sErrMsg = (SChar*)"the number of shard keys is not defined correctly";
                    IDE_TEST_RAISE( *aKeyCount == 2, ERR_CONVERT );
                    if ( *aKeyCount == 0 )
                    {
                        *aKeyCount = 1;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sErrMsg = (SChar*)"value of shard key is empty";
                    IDE_TEST_RAISE( sKeyValue->mValue.mValue.mString.mSize == 0, ERR_CONVERT );

                    // set value
                    IDE_TEST( sdfSetValue( aCondition,
                                           &(sKeyValue->mValue.mValue.mString),
                                           aAnalyzeInfo->mSplitMethod,
                                           aAnalyzeInfo->mKeyDataType,
                                           &(sRangeInfo->mRanges[sRangeInfo->mCount].mValue) )
                              != IDE_SUCCESS );

                    sValueCount = 1;
                }
                else if ( sKeyValue->mValue.mType == SDPJ_ARRAY )
                {
                    sErrMsg = (SChar*)"the number of shard keys is not defined correctly";
                    IDE_TEST_RAISE( *aKeyCount == 1, ERR_CONVERT );
                    if ( *aKeyCount == 0 )
                    {
                        *aKeyCount = 2;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sArray2 = sKeyValue->mValue.mValue.mArray;
                    sErrMsg = (SChar*)"range value is not defined";
                    IDE_TEST_RAISE( sArray2 == NULL, ERR_CONVERT );
                    sErrMsg = (SChar*)"range value accepts only string";
                    IDE_TEST_RAISE( sArray2->mValue.mType != SDPJ_STRING, ERR_CONVERT );
                    sErrMsg = (SChar*)"range value is empty";
                    IDE_TEST_RAISE( sArray2->mValue.mValue.mString.mSize == 0, ERR_CONVERT );

                    // set value
                    IDE_TEST( sdfSetValue( aCondition,
                                           &(sArray2->mValue.mValue.mString),
                                           aAnalyzeInfo->mSplitMethod,
                                           aAnalyzeInfo->mKeyDataType,
                                           &(sRangeInfo->mRanges[sRangeInfo->mCount].mValue) )
                              != IDE_SUCCESS );

                    sArray2 = sArray2->mNext;
                    sErrMsg = (SChar*)"sub-range value is not defined";
                    IDE_TEST_RAISE( sArray2 == NULL, ERR_CONVERT );
                    sErrMsg = (SChar*)"sub-range value accepts only string";
                    IDE_TEST_RAISE( sArray2->mValue.mType != SDPJ_STRING, ERR_CONVERT );
                    sErrMsg = (SChar*)"sub-range value is empty";
                    IDE_TEST_RAISE( sArray2->mValue.mValue.mString.mSize == 0, ERR_CONVERT );

                    // set sub-range value
                    IDE_TEST( sdfSetValue( aCondition,
                                           &(sArray2->mValue.mValue.mString),
                                           aAnalyzeInfo->mSubSplitMethod,
                                           aAnalyzeInfo->mSubKeyDataType,
                                           &(sRangeInfo->mRanges[sRangeInfo->mCount].mSubValue) )
                              != IDE_SUCCESS );

                    sValueCount = 2;
                }
                else
                {
                    sErrMsg = (SChar*)"range value accepts only string";
                    IDE_RAISE( ERR_CONVERT );
                }
            }
            else if ( ( sKeyValue->mKey.mSize == 4 ) &&
                      ( idlOS::strncasecmp( aCondition + sKeyValue->mKey.mOffset,
                                            "Node",
                                            4 ) == 0 ) )
            {
                sErrMsg = (SChar*)"node accepts only string";
                IDE_TEST_RAISE( sKeyValue->mValue.mType != SDPJ_STRING, ERR_CONVERT );
                sErrMsg = (SChar*)"node is empty";
                IDE_TEST_RAISE( sKeyValue->mValue.mValue.mString.mSize == 0, ERR_CONVERT );

                // set node
                IDE_TEST( sdfFindAndAddDataNode( aCondition,
                                                 &(sKeyValue->mValue.mValue.mString),
                                                 aNodeInfo,
                                                 &(sRangeInfo->mRanges[sRangeInfo->mCount].mNodeId) )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }

        // check value
        sErrMsg = (SChar*)"range value is not defined";
        IDE_TEST_RAISE( sValueCount < 1, ERR_CONVERT );

        // check sub-value
        if ( *aKeyCount == 2 )
        {
            sErrMsg = (SChar*)"range sub-value is not defined";
            IDE_TEST_RAISE( sValueCount < 2, ERR_CONVERT );
        }
        else
        {
            // Nothing to do.
        }

        // check node
        sErrMsg = (SChar*)"range node is not defined";
        IDE_TEST_RAISE( sRangeInfo->mRanges[sRangeInfo->mCount].mNodeId == ID_UINT_MAX,
                        ERR_CONVERT );

        sRangeInfo->mCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(sdERR_ABORT_SDPJ_CONVERT, sErrMsg ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfFindAndAddDataNode( SChar        * aCondition,
                              sdpjString   * aString,
                              sdiNodeInfo  * aNodeInfo,
                              UInt         * aNodeId )
{
    sdiNode   * sNode;
    UInt        sNodeId = ID_UINT_MAX;
    SChar     * sErrMsg = (SChar*)"";
    UInt        i;

    for ( i = 0, sNode = aNodeInfo->mNodes;
          i < aNodeInfo->mCount;
          i++, sNode++ )
    {
        if ( ( (SInt)idlOS::strlen( sNode->mNodeName ) == aString->mSize ) &&
             ( idlOS::strncasecmp( sNode->mNodeName,
                                   aCondition + aString->mOffset,
                                   aString->mSize ) == 0 ) )
        {
            sNodeId = i;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sNodeId == ID_UINT_MAX )
    {
        // add node
        sErrMsg = (SChar*)"node infomation is too large";
        IDE_TEST_RAISE( aNodeInfo->mCount == SDI_NODE_MAX_COUNT, ERR_CONVERT );

        sNode = aNodeInfo->mNodes + aNodeInfo->mCount;
        sNodeId = aNodeInfo->mCount;

        sErrMsg = (SChar*)"node name is too long";
        IDE_TEST_RAISE( aString->mSize > SDI_CHECK_NODE_NAME_MAX_SIZE, ERR_CONVERT );

        // init
        sNode->mNodeId = sNodeId;
        idlOS::strncpy( sNode->mNodeName,
                        aCondition + aString->mOffset,
                        aString->mSize );
        sNode->mNodeName[aString->mSize] = '\0';
        sNode->mServerIP[0] = '\0';
        sNode->mPortNo = 0;
        sNode->mAlternateServerIP[0] = '\0';
        sNode->mAlternatePortNo = 0;

        aNodeInfo->mCount++;
    }
    else
    {
        // Nothing to do.
    }

    *aNodeId = sNodeId;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(sdERR_ABORT_SDPJ_CONVERT, sErrMsg ));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfSetSplitMethod( SChar            aSplitMethodText,
                          sdiSplitMethod * aSplitMethod )
{
    sdiSplitMethod  sMethod = SDI_SPLIT_NONE;

    switch ( aSplitMethodText )
    {
        case 'H':
        case 'h':
            sMethod = SDI_SPLIT_HASH;
            break;
        case 'R':
        case 'r':
            sMethod = SDI_SPLIT_RANGE;
            break;
        case 'L':
        case 'l':
            sMethod = SDI_SPLIT_LIST;
            break;
        case 'C':
        case 'c':
            IDE_RAISE( ERR_UNSUPPORTED_SPLIT_METHOD );
            break;
        case 'S':
        case 's':
            IDE_RAISE( ERR_UNSUPPORTED_SPLIT_METHOD );
            break;
        default:
            IDE_RAISE( ERR_CONVERT );
            break;
    }

    *aSplitMethod = sMethod;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(sdERR_ABORT_SDPJ_CONVERT, "invalid split method" ));

    IDE_EXCEPTION( ERR_UNSUPPORTED_SPLIT_METHOD );
    IDE_SET(ideSetErrorCode(sdERR_ABORT_SDF_UNSUPPORTED_SHARD_SPLIT_METHOD_NAME));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfSetValue( SChar          * aCondition,
                    sdpjString     * aString,
                    sdiSplitMethod   aSplitMethod,
                    UInt             aKeyType,
                    sdiValue       * aValue )
{
    UInt  sKeyType;

    if ( aSplitMethod == SDI_SPLIT_HASH )
    {
        sKeyType = MTD_INTEGER_ID;
    }
    else
    {
        sKeyType = aKeyType;
    }

    IDE_TEST( sdm::convertRangeValue( aCondition + aString->mOffset,
                                      aString->mSize,
                                      sKeyType,
                                      aValue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
