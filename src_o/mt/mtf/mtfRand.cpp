/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtfRand.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfRand;

extern mtdModule mtdDouble;

static mtcName mtfRandFunctionName[1] = {
    { NULL, 4, (void*)"RAND" }
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
#define MTF_RAND_MT_N               (624)
#define MTF_RAND_MT_M               (397)
#define MTR_UPPER_MASK              (0x80000000) /* ( 1 << r ) - 1 */
#define MTR_LOWER_MASK              (0x7FFFFFFF)
#define MTF_RAND_MT_INIT_MULTIPLIER (1812433253)
#define MTF_RAND_MT_W               (32)
#define MTF_RAND_MT_A               (0x9908B0DF)
#define MTF_RAND_MT_U               (11)
#define MTF_RAND_MT_MASK_D          (0xFFFFFFFF)
#define MTF_RAND_MT_S               (7 )
#define MTF_RAND_MT_MASK_B          (0x9D2C5680)
#define MTF_RAND_MT_T               (15)
#define MTF_RAND_MT_MASK_C          (0xEFC60000)
#define MTF_RAND_MT_L               (18)
/* 2^32 - 1 ( 1.0 / 4294967295.0 ) */
#define MTF_RAND_MT_FACT            ((SDouble)1.0/(SDouble)4294967295.0)

UInt     gMersenneTwisterMap[MTF_RAND_MT_N];
UInt     gMersenneTwisterMapIndex;
iduMutex gMersenneTwisterMutex;

static IDE_RC mtfRandEstimate( mtcNode     * aNode,
                               mtcTemplate * aTemplate,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               mtcCallBack * aCallBack );

static IDE_RC mtfRandInitialize( void );
static IDE_RC mtfRandFinalize( void );
void mtfRandSetSeed( UInt aSeed );

mtfModule mtfRand = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfRandFunctionName,
    NULL,
    mtfRandInitialize,
    mtfRandFinalize,
    mtfRandEstimate
};

IDE_RC mtfRandCalculate( mtcNode     * aNode,
                         mtcStack    * aStack,
                         SInt          aRemain,
                         void        * aInfo,
                         mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRandCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRandInitialize( void )
{
    UInt sSeed;

    sSeed = (UInt) idlOS::gettimeofday().sec();

    mtfRandSetSeed( sSeed );

    IDE_TEST( gMersenneTwisterMutex.initialize( "RAND_MUTEX",
                                                IDU_MUTEX_KIND_POSIX,
                                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRandFinalize( void )
{
    (void)gMersenneTwisterMutex.destroy();

    return IDE_SUCCESS;
}

void mtfRandSetSeed( UInt aSeed )
{
    UInt i;

    gMersenneTwisterMap[0] = aSeed & MTF_RAND_MT_MASK_D;

    for ( i = 1; i < MTF_RAND_MT_N; i++ )
    {
        gMersenneTwisterMap[i] = ( MTF_RAND_MT_INIT_MULTIPLIER *
                                   ( gMersenneTwisterMap[i - 1] ^
                                     ( gMersenneTwisterMap[i - 1] >> ( MTF_RAND_MT_W - 2 ) ) ) + i ) &
                                 MTF_RAND_MT_MASK_D;
    }

    gMersenneTwisterMapIndex = 0;
}

void  mtfRandgetMersenneTwisterRandom( SDouble * aNumber )
{
    UInt sValue = 0;
    UInt sXA    = 0;
    UInt i;

    if ( gMersenneTwisterMapIndex >= MTF_RAND_MT_N )
    {
        for ( i = 0; i < MTF_RAND_MT_N; i++ )
        {
            sValue = ( gMersenneTwisterMap[i] & MTR_UPPER_MASK ) |
                     ( gMersenneTwisterMap[(i + 1) % MTF_RAND_MT_N] & MTR_LOWER_MASK );

            sXA = sValue >> 1;

            if ( ( sValue % 2 ) != 0 )
            {
                sXA = sXA ^ MTF_RAND_MT_A;
            }
            else
            {
                /* Nothing to do */
            }

            gMersenneTwisterMap[i] = gMersenneTwisterMap[(i+MTF_RAND_MT_M) % MTF_RAND_MT_N]
                                     ^ sXA;
        }
        gMersenneTwisterMapIndex = 0;
    }
    else
    {
        /* Nothing to do */
    }

    sValue = gMersenneTwisterMap[gMersenneTwisterMapIndex];
    sValue ^= ( sValue >> MTF_RAND_MT_U ) & MTF_RAND_MT_MASK_D;
    sValue ^= ( sValue << MTF_RAND_MT_S ) & MTF_RAND_MT_MASK_B;
    sValue ^= ( sValue << MTF_RAND_MT_T ) & MTF_RAND_MT_MASK_C;
    sValue ^= ( sValue >> MTF_RAND_MT_L );

    gMersenneTwisterMapIndex++;

    *aNumber = (SDouble)sValue * MTF_RAND_MT_FACT;
}

IDE_RC mtfRandEstimate( mtcNode     * aNode,
                        mtcTemplate * aTemplate,
                        mtcStack    * aStack,
                        SInt          /* aRemain */,
                        mtcCallBack * /* aCallBack */ )
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRandCalculate( mtcNode     * aNode,
                         mtcStack    * aStack,
                         SInt          aRemain,
                         void        * aInfo,
                         mtcTemplate * aTemplate )
{
    mtdDoubleType sValue    = 0.0;
    idBool        sIsLocked = ID_FALSE;
    UInt          sTryCount = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST( gMersenneTwisterMutex.lock( NULL )
              != IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    /**
     * BUG-42750
     *
     * 값은 0<= x < 1 은 값이 나와야 하며 만약 이런 값이
     * 나올 경우 재시도를 수행한다. 재시도는 MTF_RAND_MT_N ( 624 )
     * 회 만큼 시도해보고 그래도 않되면 seed 값을 다시 넣어서
     * 값을 얻는다.
     */
    do
    {
        sTryCount++;
        if ( sTryCount > MTF_RAND_MT_N )
        {
            mtfRandSetSeed( (UInt)idlOS::gettimeofday().sec() );
            sTryCount = 0;
        }
        else
        {
            /* Nothing to do */
        }

        mtfRandgetMersenneTwisterRandom( &sValue );

    } while ( ( sValue >= (mtdDoubleType)1.0 ) ||
              ( sValue < (mtdDoubleType)0.0 ) );

    sIsLocked = ID_FALSE;
    IDE_TEST( gMersenneTwisterMutex.unlock()
              != IDE_SUCCESS );

    *( mtdDoubleType * )aStack[0].value = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        ( void )gMersenneTwisterMutex.unlock();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

