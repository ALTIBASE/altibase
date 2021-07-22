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
 * $Id: acpPrintf.c 10912 2010-04-23 01:44:15Z djin $
 ******************************************************************************/

#include <acpMem.h>
#include <acpPrintf.h>
#include <acpPrintfPrivate.h>


#define ACP_PRINTF_BUFFER_SIZE 128


/*
 * output operation to file
 */
static acp_rc_t acpPrintfPutChrToFile(acpPrintfOutput *aOutput,
                                      acp_char_t       aChar)
{
    acp_rc_t    sRC;
    acp_size_t  sWritten;

    if (aOutput->mBufferWritten == aOutput->mBufferSize)
    {
        sRC = acpStdWrite(aOutput->mFile,
                          aOutput->mBuffer,
                          1,
                          aOutput->mBufferWritten,
                          &sWritten);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
        ACP_TEST_RAISE(sWritten != aOutput->mBufferWritten, E_EOF);
        aOutput->mBufferWritten = 0;
    }
    else
    {
        /* do nothing */
    }

    aOutput->mBuffer[aOutput->mBufferWritten] = aChar;

    aOutput->mBufferWritten++;
    aOutput->mWritten++;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_EOF)
    {
        sRC = ACP_RC_EOF;
    }
    ACP_EXCEPTION_END;
    return sRC;
}

static acp_rc_t acpPrintfPutStrToFile(acpPrintfOutput  *aOutput,
                                      const acp_char_t *aString,
                                      acp_size_t        aLen)
{
    acp_rc_t    sRC;
    acp_size_t  sWritten;

    if (aString == NULL)
    {
        aString = ACP_PRINTF_NULL_STRING;
        aLen    = ACP_PRINTF_NULL_LENGTH;
    }
    else
    {
        /* do nothing */
    }

    if ((aOutput->mBufferWritten + aLen) > aOutput->mBufferSize)
    {
        sRC = acpStdWrite(aOutput->mFile,
                          aOutput->mBuffer,
                          1,
                          aOutput->mBufferWritten,
                          &sWritten);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
        ACP_TEST_RAISE(sWritten != aOutput->mBufferWritten, E_EOF);
        aOutput->mBufferWritten = 0;

        if (aLen > aOutput->mBufferSize)
        {
            sRC = acpStdWrite(aOutput->mFile,
                              aString,
                              1,
                              aLen,
                              &sWritten);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
            ACP_TEST_RAISE(sWritten != aLen, E_EOF);
            aOutput->mWritten += aLen;
        }
        else
        {
            acpMemCpy(aOutput->mBuffer, aString, aLen);

            aOutput->mBufferWritten  = aLen;
            aOutput->mWritten       += aLen;
        }

        return ACP_RC_SUCCESS;
    }
    else
    {
        acpMemCpy(aOutput->mBuffer + aOutput->mBufferWritten, aString, aLen);

        aOutput->mBufferWritten += aLen;
        aOutput->mWritten       += aLen;

        return ACP_RC_SUCCESS;
    }

    ACP_EXCEPTION(E_EOF)
    {
        sRC = ACP_RC_EOF;
    }
    ACP_EXCEPTION_END;
    return sRC;
}

static acp_rc_t acpPrintfPutPadToFile(acpPrintfOutput *aOutput,
                                      acp_char_t       aPadder,
                                      acp_sint32_t     aLen)
{
    acp_sint32_t    sRemainSize;
    acp_sint32_t    i;
    acp_rc_t        sRC;
    acp_size_t      sWritten;

    while ((aOutput->mBufferWritten + (acp_size_t)aLen) > aOutput->mBufferSize)
    {
        sRemainSize        = (acp_sint32_t)(aOutput->mBufferSize -
                                            aOutput->mBufferWritten);
        aLen              -= sRemainSize;
        aOutput->mWritten += sRemainSize;

        while (aOutput->mBufferWritten < aOutput->mBufferSize)
        {
            aOutput->mBuffer[aOutput->mBufferWritten++] = aPadder;
        }

        sRC = acpStdWrite(aOutput->mFile,
                          aOutput->mBuffer,
                          1,
                          aOutput->mBufferWritten,
                          &sWritten);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
        ACP_TEST_RAISE(sWritten != aOutput->mBufferWritten, E_EOF);
        aOutput->mBufferWritten = 0;
    }

    for (i = 0; i < aLen; i++)
    {
        aOutput->mBuffer[aOutput->mBufferWritten++] = aPadder;
    }

    aOutput->mWritten += aLen;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_EOF)
    {
        sRC = ACP_RC_EOF;
    }
    ACP_EXCEPTION_END;
    return sRC;
}

static acpPrintfOutputOp gAcpPrintfOutputOpToFile =
{
    acpPrintfPutChrToFile,
    acpPrintfPutStrToFile,
    acpPrintfPutPadToFile
};


/*
 * output operation to buffer
 */
static acp_rc_t acpPrintfPutChrToBuffer(acpPrintfOutput *aOutput,
                                        acp_char_t       aChar)
{
    if (aOutput->mWritten < aOutput->mBufferSize)
    {
        aOutput->mBuffer[aOutput->mWritten] = aChar;
    }
    else
    {
        /* do nothing */
    }

    aOutput->mWritten++;

    return ACP_RC_SUCCESS;
}

static acp_rc_t acpPrintfPutStrToBuffer(acpPrintfOutput  *aOutput,
                                        const acp_char_t *aString,
                                        acp_size_t        aLen)
{
    acp_size_t sCopy;

    if (aString == NULL)
    {
        aString = ACP_PRINTF_NULL_STRING;
        aLen    = ACP_PRINTF_NULL_LENGTH;
    }
    else
    {
        /* do nothing */
    }

    if (aOutput->mWritten < aOutput->mBufferSize)
    {
        sCopy = ACP_MIN(aOutput->mBufferSize - aOutput->mWritten, aLen);

        acpMemCpy(aOutput->mBuffer + aOutput->mWritten, aString, sCopy);
    }
    else
    {
        /* do nothing */
    }

    aOutput->mWritten += aLen;

    return ACP_RC_SUCCESS;
}

static acp_rc_t acpPrintfPutPadToBuffer(acpPrintfOutput *aOutput,
                                        acp_char_t       aPadder,
                                        acp_sint32_t     aLen)
{
    acp_size_t sFill;

    if (aOutput->mWritten < aOutput->mBufferSize)
    {
        sFill = ACP_MIN(aOutput->mBufferSize - aOutput->mWritten,
                        (acp_size_t)aLen);

        acpMemSet(aOutput->mBuffer + aOutput->mWritten, aPadder, sFill);
    }
    else
    {
        /* do nothing */
    }

    aOutput->mWritten += aLen;

    return ACP_RC_SUCCESS;
}

static acpPrintfOutputOp gAcpPrintfOutputOpToBuffer =
{
    acpPrintfPutChrToBuffer,
    acpPrintfPutStrToBuffer,
    acpPrintfPutPadToBuffer
};


/**
 * Altibase Core implementation of standard printf() function
 * Refer to the online manual page of printf() for detail.
 * @param aFormat pointer to the format string
 * @param ... variable number of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpPrintf(const acp_char_t *aFormat, ...)
{
    va_list  sArgs;
    acp_rc_t sRC;

    va_start(sArgs, aFormat);

    sRC = acpVprintf(aFormat, sArgs);

    va_end(sArgs);

    return sRC;
}

/**
 * Altibase Core implementation of standard vprintf() function
 * Refer to the online manual page of vprintf() for detail.
 * @param aFormat pointer to the format string
 * @param aArgs @c va_list of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpVprintf(const acp_char_t *aFormat, va_list aArgs)
{
    acpPrintfOutput sOutput;
    acp_char_t      sBuffer[ACP_PRINTF_BUFFER_SIZE];
    acp_rc_t        sRC;
    acp_size_t      sWritten;

    sOutput.mWritten       = 0;
    sOutput.mFile          = ACP_STD_OUT;
    sOutput.mBufferWritten = 0;
    sOutput.mBuffer        = sBuffer;
    sOutput.mBufferSize    = sizeof(sBuffer);
    sOutput.mOp            = &gAcpPrintfOutputOpToFile;

    sRC = acpPrintfCore(&sOutput, aFormat, aArgs);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        if (sOutput.mBufferWritten > 0)
        {
            sRC = acpStdWrite(sOutput.mFile,
                              sOutput.mBuffer,
                              1,
                              sOutput.mBufferWritten,
                              &sWritten);
            ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
            ACP_TEST_RAISE(sWritten != sOutput.mBufferWritten, E_EOF);
            return ACP_RC_SUCCESS;
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }

    ACP_EXCEPTION(E_EOF)
    {
        sRC = ACP_RC_EOF;
    }
    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * Altibase Core implementation of standard fprintf() function
 * Refer to the online manual page of fprintf() for detail.
 * @param aFile pointer to the standard file object
 * @param aFormat pointer to the format string
 * @param ... variable number of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpFprintf(acp_std_file_t   *aFile,
                               const acp_char_t *aFormat, ...)
{
    va_list  sArgs;
    acp_rc_t sRC;

    va_start(sArgs, aFormat);

    sRC = acpVfprintf(aFile, aFormat, sArgs);

    va_end(sArgs);

    return sRC;
}

/**
 * Altibase Core implementation of standard vfprintf() function
 * Refer to the online manual page of vfprintf() for detail.
 * @param aFile pointer to the standard file object
 * @param aFormat pointer to the format string
 * @param aArgs @c va_list of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpVfprintf(acp_std_file_t   *aFile,
                                const acp_char_t *aFormat,
                                va_list           aArgs)
{
    acp_size_t      sWritten;
    acpPrintfOutput sOutput;
    acp_char_t      sBuffer[ACP_PRINTF_BUFFER_SIZE];
    acp_rc_t        sRC;

    sOutput.mWritten       = 0;
    sOutput.mFile          = aFile;
    sOutput.mBufferWritten = 0;
    sOutput.mBuffer        = sBuffer;
    sOutput.mBufferSize    = sizeof(sBuffer);
    sOutput.mOp            = &gAcpPrintfOutputOpToFile;

    sRC = acpPrintfCore(&sOutput, aFormat, aArgs);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        if (sOutput.mBufferWritten > 0)
        {
            sRC = acpStdWrite(sOutput.mFile,
                              sOutput.mBuffer,
                              1,
                              sOutput.mBufferWritten,
                              &sWritten);

            if(ACP_RC_NOT_SUCCESS(sRC))
            {
                return sRC;
            }
            else if (sWritten != sOutput.mBufferWritten)
            {
                return ACP_RC_EOF;
            }
            else
            {
                return ACP_RC_SUCCESS;
            }
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
}

/**
 * Altibase Core implementation of standard snprintf() function
 * Refer to the online manual page of snprintf() for detail.
 * @param aBuffer pointer to the output buffer
 * @param aSize size of output buffer
 * @param aFormat pointer to the format string
 * @param ... variable number of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpSnprintf(acp_char_t       *aBuffer,
                                acp_size_t        aSize,
                                const acp_char_t *aFormat, ...)
{
    va_list  sArgs;
    acp_rc_t sRC;

    va_start(sArgs, aFormat);

    sRC = acpVsnprintf(aBuffer, aSize, aFormat, sArgs);

    va_end(sArgs);

    return sRC;
}

/**
 * Altibase Core implementation of standard vsnprintf() function
 * Refer to the online manual page of vsnprintf() for detail.
 * @param aBuffer pointer to the output buffer
 * @param aSize size of output buffer
 * @param aFormat pointer to the format string
 * @param aArgs @c va_list of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpVsnprintf(acp_char_t       *aBuffer,
                                 acp_size_t        aSize,
                                 const acp_char_t *aFormat,
                                 va_list           aArgs)
{
    acpPrintfOutput sOutput;
    acp_rc_t        sRC;

    sOutput.mWritten    = 0;
    sOutput.mFile       = NULL;
    sOutput.mBuffer     = aBuffer;
    sOutput.mBufferSize = aSize;
    sOutput.mOp         = &gAcpPrintfOutputOpToBuffer;

    sRC = acpPrintfCore(&sOutput, aFormat, aArgs);

    /*
     * null terminate
     */
    if (aSize > 0)
    {
        aBuffer[ACP_MIN(sOutput.mWritten, aSize - 1)] = '\0';
    }
    else
    {
        /* do nothing */
    }

    /*
     * if the printed string is trucated
     */
    if (ACP_RC_IS_SUCCESS(sRC) && (sOutput.mWritten >= aSize))
    {
        sRC = ACP_RC_ETRUNC;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

/**
 * Altibase Core implementation of standard snprintf() function
 * Refer to the online manual page of snprintf() for detail.
 *
 * The difference between acpSnprintf() and acpSnprintfSize() is
 * that acpSnprintfSize() returns, via the aWritten parameter, the
 * number of bytes that have been written, or would have been written
 * if enough space had been available.
 *
 * @param aBuffer pointer to the output buffer
 * @param aSize size of output buffer
 * @param aWritten returns either the number of bytes written, or, in
 * case where the output string was truncated, the number of bytes
 * that would have been written if enough space had been available.
 * @param aFormat pointer to the format string
 * @param ... variable number of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpSnprintfSize(acp_char_t       *aBuffer,
                                    acp_size_t        aSize,
                                    acp_size_t       *aWritten,
                                    const acp_char_t *aFormat, ...)
{
    va_list  sArgs;
    acp_rc_t sRC;

    va_start(sArgs, aFormat);

    sRC = acpVsnprintfSize(aBuffer, aSize, aWritten, aFormat, sArgs);

    va_end(sArgs);

    return sRC;
}

/**
 * Altibase Core implementation of standard vsnprintf() function
 * Refer to the online manual page of vsnprintf() for detail.
 *
 * The difference between acpVsnprintf() and acpVsnprintfSize() is
 * that acpVsnprintfSize() returns, via the aWritten parameter, the
 * number of bytes that have been written, or would have been written
 * if enough space had been available.
 *
 * @param aBuffer pointer to the output buffer
 * @param aSize size of output buffer
 * @param aWritten returns either the number of bytes written, or, in
 * case where the output string was truncated, the number of bytes
 * that would have been written if enough space had been available.
 * @param aFormat pointer to the format string
 * @param aArgs @c va_list of arguments to be converted
 * @return Result code
 */
ACP_EXPORT acp_rc_t acpVsnprintfSize(acp_char_t       *aBuffer,
                                     acp_size_t        aSize,
                                     acp_size_t       *aWritten,
                                     const acp_char_t *aFormat,
                                     va_list           aArgs)
{
    acpPrintfOutput sOutput;
    acp_rc_t        sRC;

    sOutput.mWritten    = 0;
    sOutput.mFile       = NULL;
    sOutput.mBuffer     = aBuffer;
    sOutput.mBufferSize = aSize;
    sOutput.mOp         = &gAcpPrintfOutputOpToBuffer;

    sRC = acpPrintfCore(&sOutput, aFormat, aArgs);
    *aWritten = sOutput.mWritten;

    /*
     * null terminate
     */
    if (aSize > 0)
    {
        aBuffer[ACP_MIN(sOutput.mWritten, aSize - 1)] = '\0';
    }
    else
    {
        /* do nothing */
    }

    /*
     * if the printed string is trucated
     */
    if (ACP_RC_IS_SUCCESS(sRC) && (sOutput.mWritten >= aSize))
    {
        sRC = ACP_RC_ETRUNC;
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}
