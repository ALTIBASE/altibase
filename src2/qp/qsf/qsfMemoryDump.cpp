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
 * $Id:
 *
 * Description :
 *     PROJ-2408 Memory Manager Renewal
 *     FILE을 open하는 함수
 *
 * Syntax :
 *    __MEMORY_ALLOCATOR_DUMP_INTERNAL(dumptarget VARCHAR, dumplevel INT);
 *    level : 1 ~ 3
 *    RETURN VARCHAR : 0 - success
 *                     non-zero - errno when error
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <qsxEnv.h>
#include <qcmDirectory.h>
#include <qdpPrivilege.h>
#include <iduFileStream.h>
#include <iduMemMgr.h>
#include <qcuSessionObj.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 32, (void*)"__MEMORY_ALLOCATOR_DUMP_INTERNAL" }
};

#define QSF_MAX_LINESIZE  (64)

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfMemoryDumpModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_MemoryDump( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_MemoryDump,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC qsfEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule* sModules[2] =
    {
        &mtdVarchar,
        &mtdInteger
    };

    const mtdModule* sModule = &mtdVarchar;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     1,
                                     QSF_MAX_LINESIZE,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    }

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsfCalculate_MemoryDump( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     memory_dump calculate
 *
 * Implementation :
 *     1. sysdba가 아닌 경우 오류 리턴.
 *     3. dumptarget이 null이면 모든 메모리 관리자를 덤프.
 *     4. dumplevel이 null이면 1. 1, 2, 3이 아니면 오류.
 *     5. iduMemMgr::dumpAllMemory에
 *        dumptarget, dumplevel 수행
 *     6. 덤프한 메모리 관리자명 리턴
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_MemoryDump"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtdCharType    * sDumpTargetValue;  // Open mode
    mtdIntegerType   sDumpLevelValue;   // Open mode
    mtdCharType    * sReturnValue;  // Open mode

    SChar            sDumpTarget[MAXPATHLEN];

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    idlOS::memset(sDumpTarget, 0, ID_SIZEOF(sDumpTarget));
    if(aStack[1].column->module->isNull(aStack[1].column,
                                        aStack[1].value) == ID_TRUE)
    {
        idlOS::strcpy(sDumpTarget, "ALL");
    }
    else
    {
        sDumpTargetValue = (mtdCharType*)aStack[1].value;
        idlOS::strncpy(sDumpTarget,
                       (SChar*)sDumpTargetValue->value,
                       sDumpTargetValue->length);
    }

    if(aStack[2].column->module->isNull(aStack[2].column,
                                        aStack[2].value) == ID_TRUE)
    {
        sDumpLevelValue = (mtdIntegerType)1;
    }
    else
    {
        sDumpLevelValue = *(mtdIntegerType*)aStack[2].value;

        IDE_TEST_RAISE((sDumpLevelValue <= 0) || (sDumpLevelValue >= 4),
                       ERR_ARGUMENT_NOT_APPLICABLE );
    }

    IDE_TEST(iduMemMgr::dumpAllMemory(sDumpTarget, sDumpLevelValue)
             != IDE_SUCCESS);

    sReturnValue = (mtdCharType*)aStack[0].value;
    idlOS::strcpy((SChar*)sReturnValue->value, sDumpTarget);
    sReturnValue->length = idlOS::strlen(sDumpTarget);


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#undef IDE_FN
}

 
