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
 * $Id: acpPrintfRender.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpCStr.h>
#include <acpProc.h>
#include <acpPrintfPrivate.h>


#define ACP_PRINTF_SURE(aExpr)                  \
    do                                          \
    {                                           \
        acp_rc_t sExprRC = (aExpr);             \
                                                \
        if (ACP_RC_NOT_SUCCESS(sExprRC))        \
        {                                       \
            return sExprRC;                     \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

#define ACP_PRINTF_SURE2(aExpr, aCleanup)       \
    do                                          \
    {                                           \
        acp_rc_t sExprRC = (aExpr);             \
                                                \
        if (ACP_RC_NOT_SUCCESS(sExprRC))        \
        {                                       \
            aCleanup;                           \
            return sExprRC;                     \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

#define ACP_PRINTF_GET_VALUE_AND_SIGN(aSignedType, aUnsignedType)       \
    do                                                                  \
    {                                                                   \
        if ((sSigned == ACP_TRUE) && ((aSignedType)sSource < 0))        \
        {                                                               \
            sValue = (acp_uint64_t)((aUnsignedType)-sSource);           \
            sIsNeg = ACP_TRUE;                                          \
        }                                                               \
        else                                                            \
        {                                                               \
            sValue = (acp_uint64_t)((aUnsignedType)sSource);            \
            sIsNeg = ACP_FALSE;                                         \
        }                                                               \
    } while (0)


typedef acpCStrDoubleType acpPrintfConvertFloat(acp_double_t       aValue,
                                            acp_sint32_t      *aPrecision,
                                            acp_sint32_t      *aPoint,
                                            acp_bool_t        *aIsNeg,
                                            acp_char_t       **aDigits,
                                            acp_char_t       **aEndPtr);

typedef acp_bool_t acpPrintfPrepareFloat(acp_sint32_t   aDigitLen,
                                         acp_sint32_t   aPoint,
                                         acp_bool_t     aIsNeg,
                                         acp_sint32_t  *aPrintLen,
                                         acpPrintfConv *aConv);


static const acp_char_t *gAcpPrintfDigitsLowercase = "0123456789abcdef";
static const acp_char_t *gAcpPrintfDigitsUppercase = "0123456789ABCDEF";


ACP_INLINE acp_sint16_t acpPrintfGetSignLen(acp_bool_t   aIsNeg,
                                            acp_sint32_t aFlag)
{
    if ((aIsNeg == ACP_TRUE) ||
        (aFlag & (ACP_PRINTF_FLAG_SIGN_SPACE | ACP_PRINTF_FLAG_SIGN_PLUS)) != 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

ACP_INLINE acp_sint16_t acpPrintfGetExponentLen(acp_sint32_t aExp)
{
    return ((aExp <= -100) || (aExp >= 100)) ? 5 : 4;
}

ACP_INLINE acp_char_t *acpPrintfConvertInt(acp_uint64_t      aValue,
                                           acp_char_t       *aBuffer,
                                           acp_size_t        aBufferSize,
                                           acp_uint32_t      aBase,
                                           const acp_char_t *aDigits)
{
    acp_char_t *sPtr = aBuffer + aBufferSize - 1;

    *sPtr = '\0';

    do
    {
        sPtr--;
        *sPtr   = aDigits[aValue % aBase];
        aValue /= aBase;
    } while (aValue > 0);

    return sPtr;
}

ACP_INLINE acpCStrDoubleType acpPrintfConvertFloatE(acp_double_t       aVal,
                                                acp_sint32_t      *aPrec,
                                                acp_sint32_t      *aPt,
                                                acp_bool_t        *aNeg,
                                                acp_char_t       **aDigits,
                                                acp_char_t       **aEndPtr)
{
    return acpCStrDoubleToString(aVal, 2, *aPrec + 1, aPt,
                                 aNeg, aDigits, aEndPtr);
}

ACP_INLINE acpCStrDoubleType acpPrintfConvertFloatF(acp_double_t       aVal,
                                                acp_sint32_t      *aPrec,
                                                acp_sint32_t      *aPt,
                                                acp_bool_t        *aNeg,
                                                acp_char_t       **aDigits,
                                                acp_char_t       **aEndPtr)
{
    return acpCStrDoubleToString(aVal, 3, *aPrec, aPt, aNeg, aDigits, aEndPtr);
}

ACP_INLINE acpCStrDoubleType acpPrintfConvertFloatG(acp_double_t       aVal,
                                                acp_sint32_t      *aPrec,
                                                acp_sint32_t      *aPt,
                                                acp_bool_t        *aNeg,
                                                acp_char_t       **aDigits,
                                                acp_char_t       **aEndPtr)
{
    *aPrec = (*aPrec == 0) ? 1 : *aPrec;

    return acpCStrDoubleToString(aVal, 4, *aPrec, aPt, aNeg, aDigits, aEndPtr);
}

ACP_INLINE acp_bool_t acpPrintfPrepareFloatE(acp_sint32_t   aDigitLen,
                                             acp_sint32_t   aPoint,
                                             acp_bool_t     aIsNeg,
                                             acp_sint32_t  *aPrintLen,
                                             acpPrintfConv *aConv)
{
    ACP_UNUSED(aDigitLen);

    *aPrintLen = aConv->mPrecision + 1 +
        acpPrintfGetSignLen(aIsNeg, aConv->mFlag) +
        acpPrintfGetExponentLen(aPoint - 1);

    if ((aConv->mPrecision > 0) ||
        ((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0))
    {
        (*aPrintLen)++;
    }
    else
    {
        /* do nothing */
    }

    return ACP_TRUE;
}

ACP_INLINE acp_bool_t acpPrintfPrepareFloatF(acp_sint32_t   aDigitLen,
                                             acp_sint32_t   aPoint,
                                             acp_bool_t     aIsNeg,
                                             acp_sint32_t  *aPrintLen,
                                             acpPrintfConv *aConv)
{
    ACP_UNUSED(aDigitLen);

    *aPrintLen = aConv->mPrecision + acpPrintfGetSignLen(aIsNeg, aConv->mFlag);

    if (aPoint > 0)
    {
        *aPrintLen += aPoint;
    }
    else if (aPoint < 0)
    {
        *aPrintLen += -aPoint;
    }
    else
    {
        (*aPrintLen)++;
    }

    if ((aConv->mPrecision > 0) ||
        ((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0))
    {
        (*aPrintLen)++;
    }
    else
    {
        /* do nothing */
    }

    return ACP_FALSE;
}

ACP_INLINE acp_bool_t acpPrintfPrepareFloatG(acp_sint32_t   aDigitLen,
                                             acp_sint32_t   aPoint,
                                             acp_bool_t     aIsNeg,
                                             acp_sint32_t  *aPrintLen,
                                             acpPrintfConv *aConv)
{
    if ((aPoint > -4) && (aPoint <= aConv->mPrecision))
    {
        *aPrintLen = aDigitLen + acpPrintfGetSignLen(aIsNeg, aConv->mFlag);

        if (aPoint <= 0)
        {
            *aPrintLen += 2 - aPoint;
        }
        else if (aPoint < aDigitLen)
        {
            (*aPrintLen)++;
        }
        else
        {
            if ((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0)
            {
                (*aPrintLen)++;
            }
            else
            {
                /* do nothing */
            }
        }

        return ACP_FALSE;
    }
    else
    {
        *aPrintLen = acpPrintfGetSignLen(aIsNeg, aConv->mFlag) +
            acpPrintfGetExponentLen(aPoint - 1);

        if ((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0)
        {
            *aPrintLen += aConv->mPrecision + 1;
        }
        else
        {
            *aPrintLen += aDigitLen + ((aDigitLen > 1) ? 1 : 0);
        }

        return ACP_TRUE;
    }
}

ACP_INLINE acp_rc_t acpPrintfPrintPadLeft(acp_sint32_t     aPrintLen,
                                          acp_char_t       aPadder,
                                          acpPrintfConv   *aConv,
                                          acpPrintfOutput *aOutput)
{
    acp_sint32_t sLen = aConv->mFieldWidth - aPrintLen;

    if ((aConv->mPadder == aPadder) && (sLen > 0))
    {
        return (*aOutput->mOp->mPutPad)(aOutput, aPadder, sLen);
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpPrintfPrintPadRight(acp_sint32_t     aPrintLen,
                                           acp_char_t       aPadder,
                                           acpPrintfConv   *aConv,
                                           acpPrintfOutput *aOutput)
{
    acp_sint32_t sLen = aConv->mFieldWidth + aPrintLen;

    if (sLen < 0)
    {
        return (*aOutput->mOp->mPutPad)(aOutput, aPadder, -sLen);
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpPrintfPrintFillZero(acp_sint32_t     aPrintLen,
                                           acpPrintfConv   *aConv,
                                           acpPrintfOutput *aOutput)
{
    acp_sint32_t sLen = aConv->mPrecision - aPrintLen;

    if (sLen > 0)
    {
        return (*aOutput->mOp->mPutPad)(aOutput, '0', sLen);
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpPrintfPrintTrailingZero(acp_sint32_t     aPrintLen,
                                               acpPrintfConv   *aConv,
                                               acpPrintfOutput *aOutput)
{
    if ((acpCharToLower(aConv->mSpecifier) != 'g') ||
        ((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0))
    {
        return acpPrintfPrintFillZero(aPrintLen, aConv, aOutput);
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpPrintfPrintSign(acp_bool_t       aIsNeg,
                                       acp_sint32_t     aFlag,
                                       acpPrintfOutput *aOutput)
{
    if (aIsNeg == ACP_TRUE)
    {
        return (*aOutput->mOp->mPutChr)(aOutput, '-');
    }
    else
    {
        if ((aFlag & ACP_PRINTF_FLAG_SIGN_PLUS) != 0)
        {
            return (*aOutput->mOp->mPutChr)(aOutput, '+');
        }
        else if ((aFlag & ACP_PRINTF_FLAG_SIGN_SPACE) != 0)
        {
            return (*aOutput->mOp->mPutChr)(aOutput, ' ');
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
}

ACP_INLINE acp_rc_t acpPrintfPrintFloatInDecimal(const acp_char_t *aDigits,
                                                 acp_sint32_t      aDigitLen,
                                                 acp_sint32_t      aPoint,
                                                 acpPrintfConv    *aConv,
                                                 acpPrintfOutput  *aOutput)
{
    if (aPoint <= 0)
    {
        /*
         * print zeros before significants
         */
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '0'));

        if (((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0) ||
            (aConv->mPrecision > 0))
        {
            ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '.'));
        }
        else
        {
            /* do nothing */
        }

        ACP_PRINTF_SURE((*aOutput->mOp->mPutPad)(aOutput, '0', -aPoint));

        /*
         * print significant digits
         */
        ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput, aDigits, aDigitLen));

        /*
         * print trailing zeros except non-alternate form of g, G conversion
         */
        ACP_PRINTF_SURE(acpPrintfPrintTrailingZero(aDigitLen - aPoint,
                                                   aConv,
                                                   aOutput));
    }
    else
    {
        if (aDigitLen > aPoint)
        {
            /*
             * print significant digits before decimal point
             */
            ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput, aDigits, aPoint));

            /*
             * print decimal point
             */
            ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '.'));

            /*
             * print significant digits after decimal point
             */
            ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput,
                                                     aDigits + aPoint,
                                                     aDigitLen - aPoint));

            /*
             * print trailing zeros except non-alternate form of g, G conversion
             */
            ACP_PRINTF_SURE(acpPrintfPrintTrailingZero(aDigitLen - aPoint,
                                                       aConv,
                                                       aOutput));
        }
        else
        {
            /*
             * print significant digits
             */
            ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput,
                                                     aDigits,
                                                     aDigitLen));

            /*
             * fill zero to decimal point
             */
            ACP_PRINTF_SURE((*aOutput->mOp->mPutPad)(aOutput,
                                                     '0',
                                                     aPoint - aDigitLen));

            if ((aConv->mPrecision > 0) &&
                ((acpCharToLower(aConv->mSpecifier) != 'g') ||
                 ((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0)))
            {
                /*
                 * print decimal point
                 */
                ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '.'));

                /*
                 * print trailing zeros
                 */
                ACP_PRINTF_SURE((*aOutput->mOp->mPutPad)(aOutput,
                                                         '0',
                                                         aConv->mPrecision));
            }
            else
            {
                /* do nothing */
            }
        }
    }

    return ACP_RC_SUCCESS;
}

ACP_INLINE acp_rc_t acpPrintfPrintFloatInExponent(const acp_char_t *aDigits,
                                                  acp_sint32_t      aDigitLen,
                                                  acp_sint32_t      aPoint,
                                                  acpPrintfConv    *aConv,
                                                  acpPrintfOutput  *aOutput)
{
    acp_sint32_t sExp = aPoint - 1;

    /*
     * print first significant digit
     */
    ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, *aDigits));

    if('g' == acpCharToLower(aConv->mSpecifier))
    {
        /* The %XXXg specifier is always [Digit]{.[Number]}E(+|-)[Number]
         * No trailing Zeros
         * 12.345E20 with %g is 1.2345e+21 
         * 0.1E20 with %g is 1e+19
         * 1E10 with %12.10g is 1e+10, not 1.000..e+10 */
        if(aDigitLen > 1)
        {
            /* Print decimal point only when precesion > 1 */
            ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '.'));
            ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(
                    aOutput, aDigits + 1, aDigitLen - 1));
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        /* Other specifiers must fill the space with padder */
        if (((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0) ||
            (aConv->mPrecision > 0))
        {
            ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '.'));
        }
        else
        {
            /* do nothing */
        }

        /* print remaining significant digits */
        ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(
                aOutput, aDigits + 1, aDigitLen - 1));
        
        /*
         * print trailing zeros
         */
        ACP_PRINTF_SURE(acpPrintfPrintTrailingZero(aDigitLen - 1,
                                                   aConv,
                                                   aOutput));
    }

    /*
     * print exponent
     */
    ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput,
                                             (aConv->mSpecifier >= 'a') ?
                                             'e' : 'E'));

    if (sExp < 0)
    {
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '-'));
        sExp = -sExp;
    }
    else
    {
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '+'));
    }

    if (sExp >= 100)
    {
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, sExp / 100 + '0'));
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(
                aOutput, ((sExp / 10) % 10) + '0')
            );
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, sExp % 10 + '0'));
    }
    else if (sExp >= 10)
    {
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, sExp / 10 + '0'));
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, sExp % 10 + '0'));
    }
    else
    {
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, '0'));
        ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, sExp + '0'));
    }

    return ACP_RC_SUCCESS;
}

acp_rc_t acpPrintfRenderInt(acpPrintfOutput *aOutput, acpPrintfConv *aConv)
{
    const acp_char_t *sPrefix;
    const acp_char_t *sDigits;
    acp_char_t       *sPtr = NULL;
    acp_char_t        sBuf[64];
    acp_uint16_t      sPrefixLen;
    acp_bool_t        sSigned;
    acp_bool_t        sIsNeg = ACP_FALSE;
    acp_uint64_t      sValue = 0;
    acp_sint64_t      sSource;
    acp_sint32_t      sDigitLen;
    acp_sint32_t      sPrintLen;
    acp_uint32_t      sBase;

    /*
     * get the signed, base, digit case from conversion specifier
     */
    switch (aConv->mSpecifier)
    {
        case 'd':
        case 'i':
            sSigned = ACP_TRUE;
            sBase   = 10;
            sDigits = gAcpPrintfDigitsLowercase;
            break;
        case 'u':
            sSigned = ACP_FALSE;
            sBase   = 10;
            sDigits = gAcpPrintfDigitsLowercase;
            break;
        case 'o':
            sSigned = ACP_FALSE;
            sBase   = 8;
            sDigits = gAcpPrintfDigitsLowercase;
            break;
        case 'x':
            sSigned = ACP_FALSE;
            sBase   = 16;
            sDigits = gAcpPrintfDigitsLowercase;
            break;
        case 'X':
            sSigned = ACP_FALSE;
            sBase   = 16;
            sDigits = gAcpPrintfDigitsUppercase;
            break;
        default:
            acpProcAbort();
            return -1;
    }

    /*
     * get the sign and value
     */
    sSource = aConv->mArg->mValue.mLong;

    switch (aConv->mLength)
    {
        case ACP_PRINTF_LENGTH_CHAR:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_sint8_t, acp_uint8_t);
            break;
        case ACP_PRINTF_LENGTH_SHORT:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_sint16_t, acp_uint16_t);
            break;
        case ACP_PRINTF_LENGTH_DEFAULT:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_sint32_t, acp_uint32_t);
            break;
        case ACP_PRINTF_LENGTH_LONG:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_sint64_t, acp_uint64_t);
            break;
        case ACP_PRINTF_LENGTH_VINT:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_slong_t, acp_ulong_t);
            break;
        case ACP_PRINTF_LENGTH_SIZE:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_ssize_t, acp_size_t);
            break;
        case ACP_PRINTF_LENGTH_PTRDIFF:
            ACP_PRINTF_GET_VALUE_AND_SIGN(acp_ptrdiff_t, acp_uint64_t);
            break;
    }

    /*
     * convert to ascii
     */
    sPtr = acpPrintfConvertInt(sValue, sBuf, sizeof(sBuf), sBase, sDigits);

    /*
     * get length
     */
    sDigitLen = (acp_sint32_t)sizeof(sBuf) - (acp_sint32_t)(sPtr - sBuf) - 1;
    sPrintLen = sDigitLen +
        ((sSigned == ACP_TRUE) ? acpPrintfGetSignLen(sIsNeg, aConv->mFlag) : 0);

    /*
     * set the prefix if alternate form requested
     */
    if (((aConv->mFlag & ACP_PRINTF_FLAG_ALT_FORM) != 0) && (sValue != 0))
    {
        switch (aConv->mSpecifier)
        {
            case 'o':
                sPrefix     = "0";
                sPrefixLen  = 1;
                sPrintLen  += 1;
                break;
            case 'x':
                sPrefix     = "0x";
                sPrefixLen  = 2;
                sPrintLen  += 2;
                break;
            case 'X':
                sPrefix     = "0X";
                sPrefixLen  = 2;
                sPrintLen  += 2;
                break;
            default:
                sPrefix    = "";
                sPrefixLen = 0;
                break;
        }
    }
    else
    {
        sPrefix    = "";
        sPrefixLen = 0;
    }

    /*
     * if the precision is given
     */
    if (aConv->mPrecision > 0)
    {
        /*
         * ignore zero padding flag ('0')
         */
        aConv->mPadder = ' ';

        /*
         * adjust print length
         */
        sPrintLen += (aConv->mPrecision > sDigitLen) ?
            (aConv->mPrecision - sDigitLen) : 0;
    }
    else
    {
        /* do nothing */
    }

    /*
     * left padding with blanks
     */
    ACP_PRINTF_SURE(acpPrintfPrintPadLeft(sPrintLen, ' ', aConv, aOutput));

    /*
     * print sign
     */
    if (sSigned == ACP_TRUE)
    {
        ACP_PRINTF_SURE(acpPrintfPrintSign(sIsNeg, aConv->mFlag, aOutput));
    }
    else
    {
        /* do nothing */
    }

    /*
     * print the prefix
     */
    ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput, sPrefix, sPrefixLen));

    /*
     * left padding with zeros
     */
    if (aConv->mPrecision > 0)
    {
        ACP_PRINTF_SURE(acpPrintfPrintFillZero(sDigitLen, aConv, aOutput));
    }
    else
    {
        ACP_PRINTF_SURE(acpPrintfPrintPadLeft(sPrintLen, '0', aConv, aOutput));
    }

    /*
     * print digits
     */
    ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput, sPtr, sDigitLen));

    /*
     * right padding
     */
    ACP_PRINTF_SURE(acpPrintfPrintPadRight(sPrintLen, ' ', aConv, aOutput));

    return ACP_RC_SUCCESS;
}

acp_rc_t acpPrintfRenderFloat(acpPrintfOutput *aOutput, acpPrintfConv *aConv)
{
    static acpPrintfConvertFloat *sConvert[3] =
        {
            acpPrintfConvertFloatE,
            acpPrintfConvertFloatF,
            acpPrintfConvertFloatG,
        };
    static acpPrintfPrepareFloat *sPrepare[3] =
        {
            acpPrintfPrepareFloatE,
            acpPrintfPrepareFloatF,
            acpPrintfPrepareFloatG,
        };

    acp_char_t       *sString   = NULL;
    acp_char_t       *sEndPtr   = NULL;
    acp_char_t       *sCleanup  = NULL;
    acpCStrDoubleType     sDoubleType;
    acp_sint32_t      sPoint;
    acp_sint32_t      sDigitLen = 0;
    acp_sint32_t      sPrintLen = 0;
    acp_sint32_t      sSpec     = acpCharToLower(aConv->mSpecifier) - 'e';
    acp_bool_t        sExpStyle = ACP_FALSE;
    acp_bool_t        sIsNeg;

    /*
     * default precision
     */
    if (aConv->mPrecision < 0)
    {
        aConv->mPrecision = 6;
    }
    else
    {
        /* do nothing */
    }

    /*
     * convert to ascii
     */
    sDoubleType = (*sConvert[sSpec])(aConv->mArg->mValue.mDouble,
                                     &aConv->mPrecision,
                                     &sPoint,
                                     &sIsNeg,
                                     &sString,
                                     &sEndPtr);

    /*
     * get string, length, printing style
     */
    switch (sDoubleType)
    {
        case ACP_DOUBLE_NORMAL:
            sCleanup  = sString;
            sDigitLen = (acp_sint32_t)(sEndPtr - sString);
            sExpStyle = (*sPrepare[sSpec])(sDigitLen,
                                           sPoint,
                                           sIsNeg,
                                           &sPrintLen,
                                           aConv);
            break;

        case ACP_DOUBLE_INF:
            aConv->mPadder = ' ';
            sString        = (aConv->mSpecifier >= 'a') ? "inf" : "INF";
            sDigitLen      = 3;
            sPrintLen      = 3 + acpPrintfGetSignLen(sIsNeg, aConv->mFlag);
            break;

        case ACP_DOUBLE_NAN:
            aConv->mPadder = ' ';
            sString        = (aConv->mSpecifier >= 'a') ? "nan" : "NAN";
            sIsNeg         = ACP_FALSE;
            sDigitLen      = 3;
            sPrintLen      = 3 + acpPrintfGetSignLen(sIsNeg, aConv->mFlag);
            break;
    }

    /*
     * left padding with blanks
     */
    ACP_PRINTF_SURE2(acpPrintfPrintPadLeft(sPrintLen, ' ', aConv, aOutput),
                     acpCStrDoubleToStringFree(sCleanup));

    /*
     * print sign
     */
    ACP_PRINTF_SURE2(acpPrintfPrintSign(sIsNeg, aConv->mFlag, aOutput),
                     acpCStrDoubleToStringFree(sCleanup));

    /*
     * print digits
     */
    if (sEndPtr != NULL)
    {
        /*
         * left padding with zeros
         */
        ACP_PRINTF_SURE2(acpPrintfPrintPadLeft(sPrintLen, '0', aConv, aOutput),
                         acpCStrDoubleToStringFree(sCleanup));

        if (sExpStyle == ACP_TRUE)
        {
            /*
             * print digits in exponent notation
             */
            ACP_PRINTF_SURE2(acpPrintfPrintFloatInExponent(sString,
                                                           sDigitLen,
                                                           sPoint,
                                                           aConv,
                                                           aOutput),
                             acpCStrDoubleToStringFree(sCleanup));
        }
        else
        {
            /*
             * print digits in decimal notation
             */
            ACP_PRINTF_SURE2(acpPrintfPrintFloatInDecimal(sString,
                                                          sDigitLen,
                                                          sPoint,
                                                          aConv,
                                                          aOutput),
                             acpCStrDoubleToStringFree(sCleanup));
        }
    }
    else
    {
        /*
         * no digits
         */
        ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput, sString, sDigitLen));
    }

    /*
     * right padding
     */
    ACP_PRINTF_SURE2(acpPrintfPrintPadRight(sPrintLen, ' ', aConv, aOutput),
                     acpCStrDoubleToStringFree(sCleanup));

    acpCStrDoubleToStringFree(sCleanup);

    return ACP_RC_SUCCESS;
}

acp_rc_t acpPrintfRenderChar(acpPrintfOutput *aOutput, acpPrintfConv *aConv)
{
    acp_char_t sChr;

    /*
     * left padding
     */
    ACP_PRINTF_SURE(acpPrintfPrintPadLeft(1, aConv->mPadder, aConv, aOutput));

    /*
     * print character
     */
    if (aConv->mSpecifier == '%')
    {
        sChr = '%';
    }
    else
    {
        sChr = (acp_char_t)aConv->mArg->mValue.mLong;
    }

    ACP_PRINTF_SURE((*aOutput->mOp->mPutChr)(aOutput, sChr));

    /*
     * right padding
     */
    ACP_PRINTF_SURE(acpPrintfPrintPadRight(1, ' ', aConv, aOutput));

    return ACP_RC_SUCCESS;
}

acp_rc_t acpPrintfRenderString(acpPrintfOutput *aOutput, acpPrintfConv *aConv)
{
    void         *sObject = NULL;
    acp_char_t   *sString = NULL;
    acp_size_t    sLength;
    acp_sint32_t  i;

    /*
     * take string to print
     */
    sObject = aConv->mArg->mValue.mPointer;

    if (sObject == NULL)
    {
        sString = NULL;
        sLength = 0;
    }
    else
    {
        switch (aConv->mSpecifier)
        {
            case 's':
                sString = (acp_char_t *)sObject;

                if (aConv->mPrecision >= 0)
                {
                    for (i = 0;
                         (i < aConv->mPrecision) && (sString[i] != 0);
                         i++)
                    {
                        /* do nothing; just counting the length of string */
                    }

                    sLength = (acp_size_t)i;
                }
                else
                {
                    sLength = strlen(sString);
                }
                break;
            case 'S':
                sString = acpStrGetBuffer((acp_str_t *)sObject);
                sLength = acpStrGetLength((acp_str_t *)sObject);

                if (aConv->mPrecision >= 0)
                {
                    sLength = ACP_MIN(sLength, (acp_size_t)aConv->mPrecision);
                }
                else
                {
                    /* do nothing */
                }
                break;
            default:
                sString = NULL;
                sLength = 0;
                break;
        }
    }

    /*
     * left padding
     */
    ACP_PRINTF_SURE(acpPrintfPrintPadLeft((acp_sint32_t)sLength,
                                          aConv->mPadder,
                                          aConv,
                                          aOutput));

    /*
     * print string
     */
    ACP_PRINTF_SURE((*aOutput->mOp->mPutStr)(aOutput, sString, sLength));

    /*
     * right padding
     */
    ACP_PRINTF_SURE(acpPrintfPrintPadRight((acp_sint32_t)sLength,
                                           ' ',
                                           aConv,
                                           aOutput));

    return ACP_RC_SUCCESS;
}
