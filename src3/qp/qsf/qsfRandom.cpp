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
 * $Id: qsfRandom.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description : BUG-43605 [mt] random함수의 seed 설정을 session 단위로 변경해야 합니다.
 *
 * Syntax :
 *     RANDOM( INTEGER )
 **********************************************************************/

#include <qc.h>
#include <qsf.h>
#include <qsxEnv.h>
#include <qcuSessionObj.h>

extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 6, (void *)"RANDOM" }
};

/** BUG-42750 rand function
 * Mersenne Twister 알고리즘
 * MT19937 기준 상수
 * ( w, n, m, r ) = (32, 624, 397, 31 )
 * a = 0x9908B0DF
 * ( u, d ) = ( 11, 0xFFFFFFFF )
 * ( s, b ) = (  7, 0x9D2C5680 )
 * ( t, c ) = ( 15, 0xEFC60000 )
 * l        = 18
 */
#define QSF_RAND_MT_N               (624)
#define QSF_RAND_MT_M               (397)
#define QSF_UPPER_MASK              (0x80000000) /* ( 1 << r ) - 1 */
#define QSF_LOWER_MASK              (0x7FFFFFFF)
#define QSF_RAND_MT_INIT_MULTIPLIER (1812433253)
#define QSF_RAND_MT_W               (32)
#define QSF_RAND_MT_A               (0x9908B0DF)
#define QSF_RAND_MT_U               (11)
#define QSF_RAND_MT_MASK_D          (0xFFFFFFFF)
#define QSF_RAND_MT_S               (7 )
#define QSF_RAND_MT_MASK_B          (0x9D2C5680)
#define QSF_RAND_MT_T               (15)
#define QSF_RAND_MT_MASK_C          (0xEFC60000)
#define QSF_RAND_MT_L               (18)
#define QSF_RAND_MT_FACT            (1)

static IDE_RC qsfEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

mtfModule qsfRandomModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};

IDE_RC qsfCalculate_Random( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_Random,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode     * aNode,
                    mtcTemplate * aTemplate,
                    mtcStack    * aStack,
                    SInt          /* aRemain */,
                    mtcCallBack * aCallBack )
{
    const mtdModule * sModule = & mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        & sModule )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qsfRandSetSeed( UInt aSeed, qcRandomInfo * aInfo )
{
    UInt i;

    aInfo->mMap[0] = aSeed & QSF_RAND_MT_MASK_D;

    for ( i = 1; i < QSF_RAND_MT_N; i++ )
    {
        aInfo->mMap[i] = ( QSF_RAND_MT_INIT_MULTIPLIER *
                           ( aInfo->mMap[i - 1] ^
                             ( aInfo->mMap[i - 1] >>
                               ( QSF_RAND_MT_W - 2 ) ) ) + i ) &
            QSF_RAND_MT_MASK_D;
    }

    aInfo->mIndex = 0;
}

IDE_RC qsfCalculate_Random( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate )
{
    qcSession    * sSession   = NULL;
    qcRandomInfo * sInfo      = NULL;
    UInt           sSessionID = 0;
    UInt           sSeed      = 0;
    UInt           i          = 0;
    UInt           sValue     = 0;
    UInt           sXA        = 0;
    UInt           sTryCount  = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value )
         == ID_TRUE )
    {
        sSeed = 0;
    }
    else
    {
        sSeed = (UInt)*(mtdIntegerType *)aStack[1].value;
    }

    sSession = ((qcTemplate *)aTemplate)->stmt->spxEnv->mSession;
    sInfo    = & sSession->mQPSpecific.mRandomInfo;

    /* Seed 가 없다면, Session Seed를 참조한다. */
    if ( sSeed == 0 )
    {
        /* 만약 Session Seed가 초기화되지 않았으면, SessionID와 현재 시간을 합하여 결정한다. */
        if ( sInfo->mExistSeed == ID_FALSE )
        {
            sSessionID = qci::mSessionCallback.mGetSessionID( sSession );
            sSeed      = sSessionID + (UInt)idlOS::gettimeofday().msec();

            qsfRandSetSeed( sSeed, sInfo );

            sInfo->mExistSeed = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    /* 있다면, Seed에 맞도록 변경한다. */
    else
    {
        qsfRandSetSeed( sSeed, sInfo );
    }

    /* 값은 0 <= x < ID_SINT_MAX 값이 나와야 한다.
     * 재시도는 QSF_RAND_MT_N ( 624 ) 회 만큼 시도해보고
     * 그래도 않되면 seed 값을 다시 넣어서 값을 얻는다.
     */
    do
    {
        /* 계속 범위내 값이 나오지 않으면, 새로운 Seed로 변경한다. */
        sTryCount++;

        if ( sTryCount > QSF_RAND_MT_N )
        {
            sSeed = sSeed + (UInt)idlOS::gettimeofday().msec();

            qsfRandSetSeed( sSeed, sInfo );

            sTryCount = 0;
        }
        else
        {
            /* Nothing to do */
        }

        /* sInfo->mMap를 모두 사용하였으면, 현 Seed로 개선한다. */
        if ( sInfo->mIndex >= QSF_RAND_MT_N )
        {
            for ( i = 0; i < QSF_RAND_MT_N; i++ )
            {
                sValue = ( sInfo->mMap[i] & QSF_UPPER_MASK ) |
                    ( sInfo->mMap[(i + 1) % QSF_RAND_MT_N] & QSF_LOWER_MASK );

                sXA = sValue >> 1;

                if ( ( sValue % 2 ) != 0 )
                {
                    sXA = sXA ^ QSF_RAND_MT_A;
                }
                else
                {
                    /* Nothing to do */
                }

                sInfo->mMap[i] = sInfo->mMap[(i+QSF_RAND_MT_M) % QSF_RAND_MT_N] ^ sXA;
            }

            sInfo->mIndex = 0;
        }
        else
        {
            /* Nothing to do */
        }

        sValue = sInfo->mMap[sInfo->mIndex];
        sValue ^= ( sValue >> QSF_RAND_MT_U ) & QSF_RAND_MT_MASK_D;
        sValue ^= ( sValue << QSF_RAND_MT_S ) & QSF_RAND_MT_MASK_B;
        sValue ^= ( sValue << QSF_RAND_MT_T ) & QSF_RAND_MT_MASK_C;
        sValue ^= ( sValue >> QSF_RAND_MT_L );
        sValue = sValue >> QSF_RAND_MT_FACT;

        sInfo->mIndex++;
    } while ( sValue >= (UInt)ID_SINT_MAX );

    *(mtdIntegerType *)aStack[0].value = (mtdIntegerType)sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

