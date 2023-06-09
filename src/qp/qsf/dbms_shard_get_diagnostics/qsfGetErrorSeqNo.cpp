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
 * $Id: qsfGetErrorSeqNo.cpp 89988 2021-02-15 02:01:46Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <mtc.h>
#include <mtk.h>
#include <qtc.h>
#include <qsxEnv.h>

extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 30, (void*)"SP_GET_ERROR_SEQNUM_BY_NODE_ID" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfGetErrorSeqNumModule = {
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpGetErrorSeqNum( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static const mtcExecute qsfGetErrorSeqNum= {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpGetErrorSeqNum,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdInteger
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_CODET_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0, 
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfGetErrorSeqNum;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CODET_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpGetErrorSeqNum( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
    UInt          sNodeID;
    SInt          sSeqNum = -1;
    iduList     * sErrorList;
    iduList     * sNode;
    qcStatement * sStatement;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sNodeID = *(mtdIntegerType*)aStack[1].value;

    if ( sStatement->spxEnv->mErrorListCount > 0 )
    {
        sErrorList = &sStatement->spxEnv->mErrorList;

        if ( IDU_LIST_IS_EMPTY( sErrorList ) == ID_FALSE )
        {
            sSeqNum = 1;
            IDU_LIST_ITERATE( sErrorList, sNode )
            {
                if ( sNode->mObj != NULL )
                {
                    if ( ((ideErrorCollectionStack*)sNode->mObj)->mNodeId == sNodeID )
                    {
                        break;
                    }
                }
                else
                {
                    IDE_RAISE(ERR_UNEXPECTED);
                }
                sSeqNum++;
            }

            // 못찾으면 -1을 반환한다.
            if ( sSeqNum > sStatement->spxEnv->mErrorListCount )
            {
                sSeqNum = -1;
            }
        }
        else
        {
            // Error list count가 0이 아닌데 error list가 비어있을 수 없다.
            IDE_RAISE(ERR_UNEXPECTED);
        }
    }

    *((mtdIntegerType*)aStack[0].value) = sSeqNum;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                 "qsfGetErrorSeqNo",
                                 "Error collection is unexpected status" ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

