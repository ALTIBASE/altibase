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
 

/*******************************************************************************
* $Id: mtfSOWAnova.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
*******************************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

#define SOWA_BUCKET_MAX 1021

typedef struct mtfSOWAnovaData
{
    void                  * anovaKey;
    mtdDoubleType           sumOfPow;
    mtdDoubleType           sum;
    ULong                   count;
    mtfSOWAnovaData       * next;
} mtfSOWAData;

typedef struct mtfSOWAFuncData
{
    mtfSOWAnovaData      ** bucket;

    mtcColumn             * compareColumn;
    mtdCompareFunc          compareFunc;

    mtdDoubleType           sum;
    ULong                   count;
    ULong                   anovaCount;
} mtfSOWAFuncData;

#define SOWA_OPTION_SSB     (0)
#define SOWA_OPTION_SSW     (1)
#define SOWA_OPTION_DFB     (2)
#define SOWA_OPTION_DFW     (3)
#define SOWA_OPTION_MSB     (4)
#define SOWA_OPTION_MSW     (5)
#define SOWA_OPTION_FRATIO  (6)
#define SOWA_OPTION_SIG     (7)


static const SChar * MTF_SOWA_OPT_STR[] =
{
    "SUM_SQUARES_BETWEEN",
    "SUM_SQUARES_WITHIN",
    "DF_BETWEEN",
    "DF_WITHIN",
    "MEAN_SQUARES_BETWEEN",
    "MEAN_SQUARES_WITHIN",
    "F_RATIO",
    "SIG"
};

#define SOWA_FLOAT_EQ( a, b ) \
            ( ( !((a)>(b)) && !((a)<(b)) ) ? \
              ID_TRUE : ID_FALSE )

static void mtfSOWAnovaGetSSB( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );

static void mtfSOWAnovaGetSSW( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );

static void mtfSOWAnovaGetDFB( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );

static void mtfSOWAnovaGetDFW( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );

static void mtfSOWAnovaGetMSB( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );

static void mtfSOWAnovaGetMSW( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );

static void mtfSOWAnovaGetFRATIO( mtfSOWAFuncData * aFuncData,
                                  mtdDoubleType   * aResult,
                                  idBool          * aDivZero );

static void mtfSOWAnovaGetSig( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero );


typedef void (*SOWAnovaFunc)( mtfSOWAFuncData * aFuncData,
                              mtdDoubleType   * aResult,
                              idBool          * aDivZero );


static SOWAnovaFunc AnovaFunc[] =
{
    mtfSOWAnovaGetSSB,
    mtfSOWAnovaGetSSW,
    mtfSOWAnovaGetDFB,
    mtfSOWAnovaGetDFW,
    mtfSOWAnovaGetMSB,
    mtfSOWAnovaGetMSW,
    mtfSOWAnovaGetFRATIO,
    mtfSOWAnovaGetSig
};


extern mtfModule mtfSOWAnova;
extern mtdModule mtdDouble;
extern mtdModule mtdBinary;
extern mtdModule mtdInteger;


static mtcName mtfSOWANovaFunctionName[1] =
{
    { NULL, 19, (void*)"STATS_ONE_WAY_ANOVA" }
};


static IDE_RC mtfSOWAnovaEstimate( mtcNode     * aNode,
                                   mtcTemplate * aTemplate,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcCallBack * aCallBack );

mtfModule mtfSOWAnova =
{
    3 | MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity
    mtfSOWANovaFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfSOWAnovaEstimate
};

IDE_RC mtfSOWAnovaInitialize( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

IDE_RC mtfSOWAnovaAggregate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

IDE_RC mtfSOWAnovaMerge( mtcNode     * aNode,
                         mtcStack    * aStack,
                         SInt          aRemain,
                         void        * aInfo,
                         mtcTemplate * aTemplate );

IDE_RC mtfSOWAnovaFinalize( mtcNode     * aNode,
                            mtcStack    * aStack,
                            SInt          aRemain,
                            void        * aInfo,
                            mtcTemplate * aTemplate );

IDE_RC mtfSOWAnovaCalculate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        * aInfo,
                             mtcTemplate * aTemplate );

static const mtcExecute mtfExecute =
{
    mtfSOWAnovaInitialize,
    mtfSOWAnovaAggregate,
    mtfSOWAnovaMerge,
    mtfSOWAnovaFinalize,
    mtfSOWAnovaCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static void mtfSOWAnovaGetSSB( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * )
{
    mtfSOWAnovaData * sData;
    mtdDoubleType     sAvgTotal;
    mtdDoubleType     sAvgAnova;
    mtdDoubleType     sAvgDiff;
    UInt              sIdxBucket   = 0;
    ULong             sCountDone   = 0;
    ULong             sAnovaCount  = 0;
    mtdDoubleType     sSSB         = 0;

    IDE_DASSERT( aFuncData->count != 0 );

    sAvgTotal   = aFuncData->sum / aFuncData->count;
    sAnovaCount = aFuncData->anovaCount;

    while ( (sIdxBucket < SOWA_BUCKET_MAX) && (sCountDone < sAnovaCount) )
    {
        sData = aFuncData->bucket[ sIdxBucket ];
        while ( sData != NULL )
        {
            IDE_DASSERT( sData->count != 0 );

            sAvgAnova = sData->sum / sData->count;

            sAvgDiff = sAvgAnova - sAvgTotal;

            sSSB += (sAvgDiff * sAvgDiff * sData->count);

            sData = sData->next;

            sCountDone++;
        }

        sIdxBucket++;
    }

    *aResult = sSSB;
}

static void mtfSOWAnovaGetSSW( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * )
{
    mtfSOWAnovaData * sData;
    mtdDoubleType     sSSW        = 0;
    UInt              sIdxBucket  = 0;
    ULong             sCountDone  = 0;
    ULong             sAnovaCount;

    sAnovaCount = aFuncData->anovaCount;

    while ( (sIdxBucket < SOWA_BUCKET_MAX) && (sCountDone < sAnovaCount) )
    {
        sData = aFuncData->bucket[ sIdxBucket ];
        while ( sData != NULL )
        {
            IDE_DASSERT( sData->count != 0 );

            sSSW += sData->sumOfPow - ( sData->sum * (sData->sum / sData->count) );

            sData = sData->next;

            sCountDone++;
        }

        sIdxBucket++;
    }

    *aResult = sSSW;
}

static void mtfSOWAnovaGetDFB( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * )
{
    *aResult = (mtdDoubleType)(aFuncData->anovaCount - 1);
}

static void mtfSOWAnovaGetDFW( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * )
{
    SDouble sTemp;

    IDE_DASSERT( aFuncData->anovaCount > 0 );

    sTemp = (SDouble)aFuncData->anovaCount *
            ( (SDouble)aFuncData->count / (SDouble)aFuncData->anovaCount - 1 );

    *aResult = idlOS::ceil( sTemp - 0.5 );
}

static void mtfSOWAnovaGetMSB( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero )
{
    mtdDoubleType sSSB;
    mtdDoubleType sDFB;

    *aDivZero = ID_FALSE;

    mtfSOWAnovaGetDFB( aFuncData, &sDFB, NULL );

    if ( SOWA_FLOAT_EQ( sDFB, (SDouble)0 ) == ID_TRUE )
    {
        *aDivZero = ID_TRUE;
    }
    else
    {
        mtfSOWAnovaGetSSB( aFuncData, &sSSB, NULL );

        *aResult = sSSB / sDFB;
    }

    return;
}

static void mtfSOWAnovaGetMSW( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero )
{
    mtdDoubleType sSSW;
    mtdDoubleType sDFW;

    *aDivZero = ID_FALSE;

    mtfSOWAnovaGetDFW( aFuncData, &sDFW, NULL );

    if ( SOWA_FLOAT_EQ( sDFW, (SDouble)0 ) == ID_TRUE )
    {
        *aDivZero = ID_TRUE;
    }
    else
    {
        mtfSOWAnovaGetSSW( aFuncData, &sSSW, NULL );

        *aResult = sSSW / sDFW;
    }

    return;
}

static void mtfSOWAnovaGetSSBnSSW( mtfSOWAFuncData * aFuncData,
                                   mtdDoubleType   * aResultSSB,
                                   mtdDoubleType   * aResultSSW,
                                   idBool          * )
{
    mtfSOWAnovaData * sData;
    mtdDoubleType     sAvgAnova;
    mtdDoubleType     sAvgTotal;
    mtdDoubleType     sAvgDiff;
    mtdDoubleType     sSSB        = 0;
    mtdDoubleType     sSSW        = 0;
    UInt              sIdxBucket  = 0;
    ULong             sCountDone  = 0;
    ULong             sAnovaCount;

    IDE_DASSERT( aFuncData->count != 0 );

    sAnovaCount = aFuncData->anovaCount;

    sAvgTotal = aFuncData->sum / aFuncData->count;

    while ( (sIdxBucket < SOWA_BUCKET_MAX) && (sCountDone < sAnovaCount) )
    {
        sData = aFuncData->bucket[ sIdxBucket ];
        while ( sData != NULL )
        {
            IDE_DASSERT( sData->count != 0 );

            // SSB
            sAvgAnova  = sData->sum / sData->count;
            sAvgDiff   = sAvgAnova - sAvgTotal;
            sSSB      += (sAvgDiff * sAvgDiff * sData->count);

            // SSW
            sSSW += sData->sumOfPow - ( sData->sum * (sData->sum / sData->count) );

            sData = sData->next;

            sCountDone++;
        }

        sIdxBucket++;
    }

    *aResultSSB = sSSB;
    *aResultSSW = sSSW;

    return;
}

static void mtfSOWAnovaGetFRATIO( mtfSOWAFuncData * aFuncData,
                                  mtdDoubleType   * aResult,
                                  idBool          * aDivZero )
{
    mtdDoubleType sSSB;
    mtdDoubleType sSSW;
    mtdDoubleType sDFB;
    mtdDoubleType sDFW;
    mtdDoubleType sMSB;
    mtdDoubleType sMSW;

    *aDivZero = ID_FALSE;

    mtfSOWAnovaGetDFB( aFuncData, &sDFB, NULL );

    if ( SOWA_FLOAT_EQ( sDFB, (SDouble)0 ) == ID_TRUE )
    {
        *aDivZero = ID_TRUE;
    }
    else
    {
        mtfSOWAnovaGetDFW( aFuncData, &sDFW, NULL );

        if ( SOWA_FLOAT_EQ( sDFW, (SDouble)0 ) == ID_TRUE )
        {
            *aDivZero = ID_TRUE;
        }
        else
        {
            mtfSOWAnovaGetSSBnSSW( aFuncData, &sSSB, &sSSW, NULL );

            sMSB = sSSB / sDFB;
            sMSW = sSSW / sDFW;

            if ( SOWA_FLOAT_EQ( sMSW, (SDouble)0 ) == ID_TRUE )
            {
                *aDivZero = ID_TRUE;
            }
            else
            {
                *aResult = sMSB / sMSW;
            }
        }
    }
}

static SDouble mtfSOWAnovaGetSigGammaln( SDouble aZ )
{
    SDouble sX;
    SDouble sY;
    SDouble sTmp;
    SDouble sSer;
    SInt    sJ;

    static const SDouble cof[ 6 ] =
    {
        76.18009172947146,
        -86.50532032941677,
        24.0140982408091,
        -1.231739572460155,
        0.1208650973866179e-2,
        -0.5395239384953e-5
    };

    sY = sX = aZ;
    sTmp = (sX + 0.5) * idlOS::log(sX + 5.5) - (sX + 5.5);
    sSer = 1.000000000190015;
    for ( sJ = 0; sJ < 6; sJ++ )
    {
        sSer += cof[sJ] / (sY + 1);
        sY = sY + 1;
    }
    return sTmp + idlOS::log( 2.5066282746310005 * sSer / sX );
}

static SDouble mtfSOWAnovaGetSigBeta( SDouble aX,
                                      SDouble aY )
{
    SDouble sResult = 0;
    if ( (aX <= 0) || (aY <= 0) )
    {
        sResult = 0;
    }
    else
    {
        sResult = idlOS::exp( mtfSOWAnovaGetSigGammaln(aX) +
                              mtfSOWAnovaGetSigGammaln(aY) -
                              mtfSOWAnovaGetSigGammaln(aX + aY) );
    }

    return sResult;
}

static SDouble mtfSOWAnovaGetSigAbslt( SDouble aX )
{
    if ( aX < 0.0 )
    {
        return -aX;
    }
    else
    {
        return aX;
    }
}

static SDouble mtfSOWAnovaGetSigFI( SInt    aN,
                                    SDouble aX,
                                    SDouble aA,
                                    SDouble aB )
{
    SInt    sN = aN / 2;
    SDouble sF = 0.0;
    SDouble sF1;
    SDouble sS1;
    SDouble sS2;
    SDouble sTmpU;
    SDouble sTmpV;
    SInt    sI;

    for ( sI = sN; sI >= 1; sI-- )
    {
        sTmpU = (aA + 2.0 * sI - 1.0) * (aA + 2.0 * sI);
        sS2   = sI * (aB - sI) * aX / sTmpU;
        sF1   = sS2 / (1.0 + sF);
        sTmpV = (aA + 2.0 * sI - 2.0) * (aA + 2.0 * sI - 1.0);
        sS1   = -(aA + sI - 1.0) * (aB + aA + sI - 1.0) * aX / sTmpV;
        sF    = sS1 / (1.0 + sF1);
    }

    return 1.0 / (1.0 + sF);
}

static SDouble mtfSOWAnovaGetSigIncomBetaBelow( SDouble aX,
                                                SDouble aA,
                                                SDouble aB,
                                                SDouble aC1,
                                                SDouble aC2,
                                                SDouble aC3 )
{
    SDouble sResult = 0;
    SDouble sF1;
    SDouble sF2;
    SInt    sN;

    sN = 1;

    while (1)
    {
        sF1 = mtfSOWAnovaGetSigFI( 2 * sN, aX, aA, aB );
        sF2 = mtfSOWAnovaGetSigFI( 2 * sN + 2, aX, aA, aB );

        if ( mtfSOWAnovaGetSigAbslt( sF2 - sF1 ) < 1.0e-30 )
        {
            if ( SOWA_FLOAT_EQ( aC3, (SDouble)0 ) == ID_TRUE )
            {
                // avoid div by zero
                return sF2 * aC1 * aC2 / aA;
            }
            else
            {
                return sF2 * aC1 * aC2 / aA / aC3;
            }
        }
        else
        {
            sN++;
        }
    }

    return sResult;
}

static SDouble mtfSOWAnovaGetSigIncomBetaAbove( SDouble aX,
                                                SDouble aA,
                                                SDouble aB,
                                                SDouble aC1,
                                                SDouble aC2,
                                                SDouble aC3 )
{
    SDouble sResult = 0;
    SDouble sF1;
    SDouble sF2;
    SInt    sN;

    if ( (mtfSOWAnovaGetSigAbslt( aX - 0.5 ) < 1.0e-30) &&
         (mtfSOWAnovaGetSigAbslt( aA - aB ) < 1.0e-30) )
    {
        return 0.5;
    }
    else
    {
        sN = 1;
        while (1)
        {
            sF1 = mtfSOWAnovaGetSigFI( 2 * sN, 1.0 - aX, aB, aA );
            sF2 = mtfSOWAnovaGetSigFI( 2 * sN + 2, 1.0 - aX, aB, aA );

            if ( mtfSOWAnovaGetSigAbslt( sF2 - sF1 ) < 1.0e-30 )
            {
                if ( SOWA_FLOAT_EQ( aC3, (SDouble)0 ) == ID_TRUE )
                {
                    // avoid div by zero
                    return 1.0 - sF2 * aC1 * aC2 / aB;
                }
                else
                {
                    return 1.0 - sF2 * aC1 * aC2 / aB / aC3;
                }
            }
            else
            {
                sN++;
            }
        }
    }

    return sResult;
}

static SDouble mtfSOWAnovaGetSigIncomBeta( SDouble aX,
                                           SDouble aA,
                                           SDouble aB )
{
    SDouble sC1;
    SDouble sC2;
    SDouble sC3;

    if ( (aA <= 0.0) || (aB <= 0.0) )
    {
        return 0.0;
    }
    else
    {
        // nothing to to
    }

    if ( mtfSOWAnovaGetSigAbslt(aX - 0.0) < 1.0e-30 )
    {
        return 0.0;
    }
    else
    {
        // nothing to to
    }

    sC1 = idlOS::pow(aX, aA);
    sC2 = idlOS::pow(1.0 - aX, aB);
    sC3 = mtfSOWAnovaGetSigBeta(aA, aB);
    if ( aX < ((aA + 1.0) / (aA + aB + 2.0)) )
    {
        return mtfSOWAnovaGetSigIncomBetaBelow( aX, aA, aB, sC1, sC2, sC3 );
    }
    else
    {
        return mtfSOWAnovaGetSigIncomBetaAbove( aX, aA, aB, sC1, sC2, sC3 );
    }
}

static void mtfSOWAnovaGetSig( mtfSOWAFuncData * aFuncData,
                               mtdDoubleType   * aResult,
                               idBool          * aDivZero )
{
    mtdDoubleType   sFRatio;
    mtdDoubleType   sSSB;
    mtdDoubleType   sSSW;
    mtdDoubleType   sDFB;
    mtdDoubleType   sDFW;
    mtdDoubleType   sMSB;
    mtdDoubleType   sMSW;
    mtdDoubleType   sDividend;

    *aResult  = 0;
    *aDivZero = ID_FALSE;

    mtfSOWAnovaGetDFB( aFuncData, &sDFB, NULL );

    mtfSOWAnovaGetDFW( aFuncData, &sDFW, NULL );

    if ( (SOWA_FLOAT_EQ( sDFB, (SDouble)0 ) == ID_TRUE) ||
         (SOWA_FLOAT_EQ( sDFW, (SDouble)0 ) == ID_TRUE) )
    {
        if ( (SOWA_FLOAT_EQ( sDFB, (SDouble)0 ) == ID_TRUE) &&
             (SOWA_FLOAT_EQ( sDFW, (SDouble)0 ) == ID_TRUE) )
        {
            // result is 0
        }
        else
        {
            // result is null
            *aDivZero = ID_TRUE;
        }
    }
    else
    {
        mtfSOWAnovaGetSSBnSSW( aFuncData, &sSSB, &sSSW, NULL );

        sMSB = sSSB / sDFB;
        sMSW = sSSW / sDFW;

        if ( SOWA_FLOAT_EQ( sMSW, (SDouble)0 ) == ID_TRUE )
        {
            *aResult = 1;
        }
        else
        {
            sFRatio = sMSB / sMSW;

            if ( sFRatio < 0.0 )
            {
                sFRatio = -sFRatio;
            }
            else
            {
                // nothing to do
            }

            sDividend = sDFW + sDFB * sFRatio;

            if ( SOWA_FLOAT_EQ( sDividend, (SDouble)0 ) == ID_TRUE )
            {
                *aDivZero = ID_TRUE;
            }
            else
            {
                *aResult = mtfSOWAnovaGetSigIncomBeta( sDFW / sDividend,
                                                       sDFW / 2.0,
                                                       sDFB / 2.0 );
            }
        }
    }
}

static IDE_RC mtfSOWAnovaCalValue( mtcNode     * aNode,
                                   mtcStack    * aStack,
                                   SInt          aRemain,
                                   mtcTemplate * aTemplate )
{
    mtcExecute * sExecute;

    sExecute = &aTemplate->rows[aNode->table].execute[aNode->column];

    IDE_TEST( sExecute->calculate( aNode,
                                   aStack,
                                   aRemain,
                                   sExecute->calculateInfo,
                                   aTemplate )
              != IDE_SUCCESS );

    if ( aNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( aNode,
                                         aStack,
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static void mtfSOWAnovaCompareOptStr( const SChar    * aStr,
                                      UInt             aOptNo,
                                      UInt             aLen,
                                      mtdIntegerType * aOption )
{
    if ( idlOS::strncmp( aStr,
                         MTF_SOWA_OPT_STR[aOptNo],
                         aLen ) == 0 )
    {
        *aOption = aOptNo;
    }
    else
    {
        // nothing to do
    }
}

static IDE_RC mtfSOWAnovaGetOption( mtcColumn      * aOptColumn,
                                    void           * aOptValue,
                                    mtdIntegerType * aOption )
{
    mtdIntegerType    sOption = -1;
    mtdCharType     * sOptValue;

    sOptValue = (mtdCharType *)aOptValue;

    if ( aOptColumn->module->isNull( aOptColumn, aOptValue ) == ID_TRUE )
    {
        sOption = SOWA_OPTION_SIG;
    }
    else
    {
        switch ( sOptValue->length )
        {
        case  3:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_SIG,
                                      3,
                                      &sOption );
            break;
        case  7:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_FRATIO,
                                      7,
                                      &sOption );
            break;
        case 9:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_DFW,
                                      9,
                                      &sOption );
            break;
        case 10:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_DFB,
                                      10,
                                      &sOption );
            break;
        case 18:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_SSW,
                                      18,
                                      &sOption );
            break;
        case 19:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_SSB,
                                      19,
                                      &sOption );
            if ( sOption == SOWA_OPTION_SSB )
            {
                break;
            }
            else
            {
                // nothing to do
            }

            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_MSW,
                                      19,
                                      &sOption );
            break;
        case 20:
            mtfSOWAnovaCompareOptStr( (const SChar*)sOptValue->value,
                                      SOWA_OPTION_MSB,
                                      20,
                                      &sOption );
            break;
        default:
            break;
        }

        IDE_TEST_RAISE( sOption < 0, ERR_ARGUMENT_NOT_APPLICABLE );
    }

    *aOption = sOption;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSOWAnovaEstimate( mtcNode     * aNode,
                            mtcTemplate * aTemplate,
                            mtcStack    * aStack,
                            SInt,
                            mtcCallBack * aCallBack )
{
    static const mtdModule *sModules[3];
    UInt sArgsNum;

    sArgsNum = (UInt)( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    IDE_TEST_RAISE( (sArgsNum < 2) || (sArgsNum > 3),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( mtf::isGreaterLessValidType( aStack[1].column->module ) == ID_FALSE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    sModules[0] = aStack[1].column->module;
    sModules[1] = &mtdDouble;

    if ( sArgsNum == 3 )
    {
        IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[2],
                                                    aStack[3].column->module )
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;


    // result value
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // function data
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     ID_SIZEOF(mtfSOWAFuncData *),
                                     0 )
              != IDE_SUCCESS );

    // option number
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // funcData 사용
    aNode->info = aTemplate->funcDataCnt;
    aTemplate->funcDataCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSOWAnovaInitialize( mtcNode*     aNode,
                              mtcStack*,
                              SInt,
                              void*,
                              mtcTemplate* aTemplate )
{
    iduMemory            * sMemoryMgr = NULL;
    mtfFuncDataBasicInfo * sFDBI      = NULL;
    mtcColumn            * sColumn;
    UChar                * sRow;
    mtfSOWAFuncData      * sFuncData  = NULL;
    mtdBinaryType        * sDataPtr;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;

    // 최초 등록
    if ( aTemplate->funcData[aNode->info] == NULL )
    {
        IDE_TEST( mtf::allocFuncDataMemory( &sMemoryMgr )
                  != IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "mtfSOWAnovaInitialize::alloc::sFDBI",
                             ERR_MEMORY_ALLOCATION );
        // function data alloc
        IDE_TEST_RAISE( sMemoryMgr->alloc( ID_SIZEOF(mtfFuncDataBasicInfo),
                                           (void**)&sFDBI )
                        != IDE_SUCCESS,
                        ERR_MEMORY_ALLOCATION );
    
        // initialize Function Data Basic Info
        IDE_TEST( mtf::initializeFuncDataBasicInfo( sFDBI,
                                                    sMemoryMgr )
                  != IDE_SUCCESS );

        // 등록
        aTemplate->funcData[aNode->info] = sFDBI;
    }
    else
    {
        sFDBI = aTemplate->funcData[aNode->info];
    }

    IDU_FIT_POINT_RAISE( "mtfSOWAnovaInitialize::cralloc::sFuncData",
                         ERR_MEMORY_ALLOCATION );
    // create function data
    IDE_TEST_RAISE( sFDBI->memoryMgr->cralloc( ID_SIZEOF(mtfSOWAFuncData),
                                               (void**)&sFuncData )
                    != IDE_SUCCESS,
                    ERR_MEMORY_ALLOCATION );

    IDU_FIT_POINT_RAISE( "mtfSOWAnovaInitialize::cralloc::sFuncData->bucket",
                         ERR_MEMORY_ALLOCATION );
    // create hash bucket of function data
    IDE_TEST_RAISE( sFDBI->memoryMgr->cralloc( ID_SIZEOF(mtfSOWAnovaData*) * SOWA_BUCKET_MAX,
                                              (void**)&sFuncData->bucket )
                    != IDE_SUCCESS,
                    ERR_MEMORY_ALLOCATION );

    // result value
    *(mtdDoubleType *)( sRow + sColumn[0].column.offset ) = 0;

    // function data
    sDataPtr = (mtdBinaryType *)( sRow + sColumn[1].column.offset );
    sDataPtr->mLength = ID_SIZEOF(mtfSOWAFuncData *);
    *( (mtfSOWAFuncData **)sDataPtr->mValue ) = sFuncData;

    // option number
    *(mtdIntegerType *)( sRow + sColumn[2].column.offset ) = -1;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }

    IDE_EXCEPTION_END;

    if ( sMemoryMgr != NULL )
    {
        mtf::freeFuncDataMemory( sMemoryMgr );

        aTemplate->funcData[aNode->info] = NULL;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_FAILURE;
}

IDE_RC mtfSOWAnovaAggregate( mtcNode     * aNode,
                             mtcStack    * aStack,
                             SInt          aRemain,
                             void        *,
                             mtcTemplate * aTemplate )
{
    mtcColumn            * sColumn;
    UChar                * sRow;
    mtfFuncDataBasicInfo * sFDBI;
    mtfSOWAFuncData      * sFuncData;
    mtfSOWAnovaData      * sAnovaData;
    mtcNode              * sNode;
    mtdBinaryType        * sDataPtr;
    UInt                   sIsNewAnova = 0;
    SDouble                sNewValue;
    mtdIntegerType         sOption;
    UInt                   sHashValue;
    UInt                   sBucketNo;
    mtdValueInfo           sValueInfo1;
    mtdValueInfo           sValueInfo2;

    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;

    sDataPtr  = (mtdBinaryType *)(sRow + sColumn[1].column.offset);
    sFuncData = *(mtfSOWAFuncData **)sDataPtr->mValue;

    sFDBI = aTemplate->funcData[aNode->info];
    
    sOption = *(mtdIntegerType *)(sRow + sColumn[2].column.offset);
    if ( sOption < 0 )
    {
        // Validate option string
        sNode = aNode->arguments->next->next;
        if ( sNode != NULL )
        {
            IDE_TEST( mtfSOWAnovaCalValue( sNode,
                                           aStack + 3,
                                           aRemain - 3,
                                           aTemplate )
                     != IDE_SUCCESS );

            IDE_TEST_RAISE( mtfSOWAnovaGetOption( aStack[3].column,
                                                  aStack[3].value,
                                                  &sOption )
                            != IDE_SUCCESS,
                            ERR_ARGUMENT_NOT_APPLICABLE );

            *(mtdIntegerType *)(sRow + sColumn[2].column.offset) = sOption;
        }
        else
        {
            *(mtdIntegerType *)(sRow + sColumn[2].column.offset) = SOWA_OPTION_SIG;
        }
    }
    else
    {
        // nothing to do
    }

    // calculate expr1's value
    IDE_TEST( mtfSOWAnovaCalValue( aNode->arguments,
                                   aStack + 1,
                                   aRemain - 1,
                                   aTemplate )
              != IDE_SUCCESS );

    // calculate expr2's value
    IDE_TEST( mtfSOWAnovaCalValue( aNode->arguments->next,
                                   aStack + 2,
                                   aRemain - 2,
                                   aTemplate )
              != IDE_SUCCESS );

    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_FALSE ) &&
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_FALSE ) )
    {
        sHashValue = aStack[1].column->module->hash( mtc::hashInitialValue,
                                                     aStack[1].column,
                                                     aStack[1].value );

        sBucketNo = sHashValue % SOWA_BUCKET_MAX;

        if ( sFuncData->count == 0 )
        {
            sFuncData->compareColumn = aStack[1].column;
            sFuncData->compareFunc   =
                aStack[1].column->module->logicalCompare[MTD_COMPARE_ASCENDING];
        }
        else
        {
            // Nothing to do.
        }
        
        sValueInfo1.column = aStack[1].column;
        sValueInfo1.value  = aStack[1].value;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = aStack[1].column;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        // Find anova data with the same key in the bucket
        sAnovaData = sFuncData->bucket[ sBucketNo ];
        while ( sAnovaData != NULL )
        {
            sValueInfo2.value = sAnovaData->anovaKey;

            if ( sFuncData->compareFunc( &sValueInfo1,
                                         &sValueInfo2 ) == 0 )
            {
                break;  // found
            }
            else
            {
                // nothing to do
            }

            sAnovaData = sAnovaData->next;
        }

        // When not found, add new anova data
        if ( sAnovaData == NULL )
        {
            IDU_FIT_POINT_RAISE( "mtfSOWAnovaAggregate::alloc::sAnovaData",
                                 ERR_MEMORY_ALLOCATION );
            IDE_TEST_RAISE( sFDBI->memoryMgr->alloc( ID_SIZEOF( mtfSOWAnovaData ),
                                                     (void **)&sAnovaData )
                            != IDE_SUCCESS,
                            ERR_MEMORY_ALLOCATION );

            IDU_FIT_POINT_RAISE( "mtfSOWAnovaAggregate::alloc::sAnovaData->anovaKey",
                                 ERR_MEMORY_ALLOCATION );
            IDE_TEST_RAISE( sFDBI->memoryMgr->alloc( aStack[1].column->column.size,
                                                     &sAnovaData->anovaKey )
                            != IDE_SUCCESS,
                            ERR_MEMORY_ALLOCATION );

            idlOS::memcpy( sAnovaData->anovaKey,
                           aStack[1].value,
                           aStack[1].column->column.size );

            sAnovaData->count    = 0;
            sAnovaData->sum      = 0;
            sAnovaData->sumOfPow = 0;
            sAnovaData->next     = NULL;

            // Add to bucket
            if ( sFuncData->bucket[ sBucketNo ] == NULL )
            {
                sFuncData->bucket[ sBucketNo ] = sAnovaData;
            }
            else
            {
                sAnovaData->next = sFuncData->bucket[ sBucketNo ];
                sFuncData->bucket[ sBucketNo ] = sAnovaData;
            }

            sIsNewAnova = 1;
        }
        else
        {
            // nothing to do
        }

        // Accumulate AnovaData
        sNewValue = *(SDouble*)aStack[2].value;

        sAnovaData->count++;
        sAnovaData->sumOfPow  += ( sNewValue * sNewValue );
        sAnovaData->sum       += sNewValue;

        // Accumulate FuncData
        sFuncData->sum        += sNewValue;
        sFuncData->count++;
        sFuncData->anovaCount += sIsNewAnova;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_MEMORY_ALLOCATION ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSOWAnovaMerge( mtcNode     * aNode,
                         mtcStack    *,
                         SInt,
                         void        * aInfo,
                         mtcTemplate * aTemplate )
{
    mtcColumn       * sColumn;
    UChar           * sDstRow;
    UChar           * sSrcRow;
    mtdBinaryType   * sBinPtr;
    mtfSOWAFuncData * sDstFData;
    mtfSOWAFuncData * sSrcFData;
    mtfSOWAnovaData * sDstAnova;
    mtfSOWAnovaData * sSrcAnova;
    mtfSOWAnovaData * sSrcMove;
    ULong             sCountDone = 0;
    UInt              sBucketNo  = 0;
    idBool            sFound;
    mtdIntegerType    sOption;
    mtdValueInfo      sSrcValInfo;
    mtdValueInfo      sDstValInfo;


    sDstRow   = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow   = (UChar*)aInfo;
    sColumn   = aTemplate->rows[aNode->table].columns + aNode->column;

    // Get Destination Function Data
    sBinPtr   = (mtdBinaryType *)(sDstRow + sColumn[1].column.offset);
    sDstFData = *(mtfSOWAFuncData **)sBinPtr->mValue;

    // Get Source Function Data
    sBinPtr   = (mtdBinaryType *)(sSrcRow + sColumn[1].column.offset);
    sSrcFData = *(mtfSOWAFuncData **)sBinPtr->mValue;

    if ( sSrcFData->count == 0 )
    {
        // nothing to do
    }
    else
    {
        sDstFData->compareColumn = sSrcFData->compareColumn;
        sDstFData->compareFunc = sSrcFData->compareFunc;

        sSrcValInfo.column = sSrcFData->compareColumn;
        sSrcValInfo.flag   = MTD_OFFSET_USELESS;
        sDstValInfo.column = sDstFData->compareColumn;
        sDstValInfo.flag   = MTD_OFFSET_USELESS;

        while ( (sBucketNo < SOWA_BUCKET_MAX) && (sCountDone < sSrcFData->anovaCount) )
        {
            sSrcAnova = sSrcFData->bucket[ sBucketNo ];
            while ( sSrcAnova != NULL )
            {
                sSrcValInfo.value = sSrcAnova->anovaKey;

                sFound = ID_FALSE;
                sDstAnova = sDstFData->bucket[ sBucketNo ];
                while ( sDstAnova != NULL )
                {
                    sDstValInfo.value = sDstAnova->anovaKey;

                    if ( sDstFData->compareFunc( &sSrcValInfo,
                                                 &sDstValInfo ) == 0 )
                    {
                        sFound = ID_TRUE;
                        break;
                    }
                    else
                    {
                        // nothing to do
                    }

                    sDstAnova = sDstAnova->next;
                }

                // If found, just accumulate src data to dest
                if ( sFound == ID_TRUE )
                {
                    sDstAnova->count    += sSrcAnova->count;
                    sDstAnova->sum      += sSrcAnova->sum;
                    sDstAnova->sumOfPow += sSrcAnova->sumOfPow;

                    sDstFData->count    += sSrcAnova->count;
                    sDstFData->sum      += sSrcAnova->sum;

                    sSrcAnova = sSrcAnova->next;
                }
                else // If not found, insert source anova data to destination bucket
                {
                    sSrcMove  = sSrcAnova;
                    sSrcAnova = sSrcAnova->next;

                    sSrcMove->next = sDstFData->bucket[ sBucketNo ];
                    sDstFData->bucket[ sBucketNo ] = sSrcMove;

                    sDstFData->anovaCount++;
                    sDstFData->count += sSrcMove->count;
                    sDstFData->sum   += sSrcMove->sum;
                }

                sCountDone++;
            }

            sBucketNo++;
        }

        // option number
        sOption = *(mtdIntegerType *)(sSrcRow + sColumn[2].column.offset);
        *(mtdIntegerType *)( sDstRow + sColumn[2].column.offset ) = sOption;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSOWAnovaFinalize( mtcNode*     aNode,
                            mtcStack*,
                            SInt,
                            void*,
                            mtcTemplate* aTemplate )
{
    mtcColumn       * sColumn;
    void            * sValueTemp;
    UChar           * sRow;
    mtfSOWAFuncData * sFuncData;
    mtdBinaryType   * sDataPtr;
    SInt              sOption;
    mtdDoubleType     sResult;
    idBool            sIsInfini  = ID_FALSE;

    // calculate option string
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;

    sDataPtr  = (mtdBinaryType *)(sRow + sColumn[1].column.offset);
    sFuncData = *(mtfSOWAFuncData **)sDataPtr->mValue;

    if ( sFuncData->count < 1 )
    {
        sValueTemp = (void*)(sRow + sColumn[0].column.offset);

        sColumn[0].module->null( sColumn + 0, sValueTemp );
    }
    else
    {
        sOption = *(mtdIntegerType *)(sRow + sColumn[2].column.offset);
        IDE_TEST_RAISE( (sOption < SOWA_OPTION_SSB) || (sOption > SOWA_OPTION_SIG),
                        ERR_ARGUMENT_NOT_APPLICABLE );

        AnovaFunc[ sOption ]( sFuncData, &sResult, &sIsInfini );

        if ( sIsInfini == ID_TRUE )
        {
            sValueTemp = (void*)(sRow + sColumn[0].column.offset);

            sColumn[0].module->null( sColumn + 0, sValueTemp );
        }
        else
        {
            *(mtdDoubleType*)(sRow + sColumn[0].column.offset) = sResult;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSOWAnovaCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt,
                             void*,
                             mtcTemplate* aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row +
                                       aStack->column->column.offset );
    
    return IDE_SUCCESS;
}
