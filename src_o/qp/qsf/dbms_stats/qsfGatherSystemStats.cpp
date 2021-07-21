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
 * $Id: qsfGatherSystemStats.cpp 29282 2008-11-13 08:03:38Z mhjeong $
 *
 * Description :
 *     TASK-4990 changing the method of collecting index statistics
 *     SYSTEM 관련 통계정보를 수집한다.
 *
 * Syntax :
 *    GATHER_SYSTEM_STATS ()
 *    RETURN Integer
 *
 **********************************************************************/

#include <qsf.h>
#include <qsxEnv.h>
#include <qdpPrivilege.h>
#include <qdpRole.h>
#include <smiDef.h>
#include <smiStatistics.h>

extern mtdModule mtdReal;
extern mtdModule mtdInteger;
extern mtdModule mtdBigint;
extern mtdModule mtdBoolean;

#define BENCH_LOOP_COUNT (1000000)
#define COMPARE_ADJ      (2.0)
#define STORE_ADJ        (100.0)

static mtcName qsfFunctionName[1] = {
    { NULL, 22, (void*)"SP_GATHER_SYSTEM_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule qsfGatherSystemStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_GatherSystemStats( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_GatherSystemStats,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1] = {&mtdInteger};

    const mtdModule* sModule = &mtdInteger;
 
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
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
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

SDouble benchHashTime()
{
    idvTime   sBeginTime;
    idvTime   sEndTime;
    ULong     sReadTime;
    ULong     sCount;
    UInt      sHashSeed = mtc::hashInitialValue;

    IDV_TIME_GET(&sBeginTime);

    for( sCount=0; sCount < BENCH_LOOP_COUNT; sCount++)
    {
        sHashSeed = mtc::hash( sHashSeed,
                              (const UChar*)(&sCount),
                               ID_SIZEOF( ULong ) );
    }

    IDV_TIME_GET(&sEndTime);

    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    return sReadTime / (SDouble)BENCH_LOOP_COUNT;
}

SDouble benchCompareTime()
{
    idvTime   sBeginTime;
    idvTime   sEndTime;
    ULong     sReadTime;
    UInt      sCount;
    UInt      sCompare;
    ULong     sValue1 =0;
    ULong     sValue2 =1;

    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;

    IDV_TIME_GET(&sBeginTime);

    for( sCount = 0, sCompare = 1; sCount < BENCH_LOOP_COUNT; sCount++)
    {
        sValueInfo1.column = NULL;
        sValueInfo1.value  = &sValue1;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = &sValue2;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        if( sCompare == 1 )
        {
            sCompare = mtdBigint.logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                        &sValueInfo2 );
        }
        else
        {
            sCompare = mtdBigint.logicalCompare[MTD_COMPARE_DESCENDING]( &sValueInfo1,
                                                                         &sValueInfo2 );
        }
    }

    IDV_TIME_GET(&sEndTime);

    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    return sReadTime / (SDouble)BENCH_LOOP_COUNT;
}

SDouble benchStoreTime()
{
    idvTime   sBeginTime;
    idvTime   sEndTime;
    ULong     sReadTime;
    UInt      sCount;
    UInt      sRemainder;
    iduList   sList;
    iduList * sListPtr;
    iduList * sPrev = NULL;

    IDV_TIME_GET(&sBeginTime);

    for( sCount = 0, sListPtr = &sList;
         sCount < BENCH_LOOP_COUNT;
         sCount++, sListPtr = sListPtr->mNext )
    {
        sRemainder = sCount % 127;

        if( (sRemainder % 3 ) == 0 )
        {
            sListPtr->mObj  = &sBeginTime;
        }
        else
        {
            sListPtr->mObj  = &sEndTime;
        }
        sListPtr->mPrev = sPrev;
        sListPtr->mNext = &sList;
        sPrev           = sListPtr;
    }

    IDV_TIME_GET(&sEndTime);

    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    return sReadTime / (SDouble)BENCH_LOOP_COUNT;
}

IDE_RC qsfCalculate_GatherSystemStats( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     GatherSystemStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL GatherSystemStats
 *
 ***********************************************************************/

    SDouble sHashTime;
    SDouble sCompareTime;
    SDouble sStoreTime;

    qcStatement        * sStatement;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /* Check Privilege */
    IDE_TEST( qdpPrivilege::checkDBMSStatPriv( sStatement ) != IDE_SUCCESS );

    IDE_TEST( smiStatistics::gatherSystemStats(ID_FALSE)
            != IDE_SUCCESS );

    // BUG-37125 tpch plan optimization
    // 측정된 결과치가 실제 상황을 제대로 반영하지 못한다.
    sHashTime    = benchHashTime();
    sCompareTime = benchCompareTime() * COMPARE_ADJ;
    sStoreTime   = benchStoreTime() * STORE_ADJ;

    IDE_TEST( smiStatistics::setSystemStatsByUser( NULL,
                                                   NULL,
                                                   NULL,
                                                   &sHashTime,
                                                   &sCompareTime,
                                                   &sStoreTime )
                != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
