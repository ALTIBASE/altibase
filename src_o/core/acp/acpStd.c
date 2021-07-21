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
 * $Id: acpStd.c 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#include <acpStd.h>

#if defined(ALTI_CFG_OS_WINDOWS)

#define ACP_STD_DEFAULT_PERM (0755)

/*
 * Global handles for Standard I/Os
 */
static acp_std_file_t gAcpStdin     = {NULL};
static acp_std_file_t gAcpStdout    = {NULL};
static acp_std_file_t gAcpStderr    = {NULL};

ACP_EXPORT acp_std_file_t* acpStdGetStdin(void)
{
    gAcpStdin.mHandle.mHandle = GetStdHandle(STD_INPUT_HANDLE);
    return &gAcpStdin;
}

ACP_EXPORT acp_std_file_t* acpStdGetStdout(void)
{
    gAcpStdout.mHandle.mHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    return &gAcpStdout;
}

ACP_EXPORT acp_std_file_t* acpStdGetStderr(void)
{
    gAcpStderr.mHandle.mHandle = GetStdHandle(STD_ERROR_HANDLE);
    return &gAcpStderr;
}

#else
/*
 * Global handles for Standard I/Os
 */
static acp_std_file_t gAcpStdin     = {NULL};
static acp_std_file_t gAcpStdout    = {NULL};
static acp_std_file_t gAcpStderr    = {NULL};

ACP_EXPORT acp_std_file_t* acpStdGetStdin(void)
{
    gAcpStdin.mFP = stdin;
    return &gAcpStdin;
}

ACP_EXPORT acp_std_file_t* acpStdGetStdout(void)
{
    gAcpStdout.mFP = stdout;
    return &gAcpStdout;
}

ACP_EXPORT acp_std_file_t* acpStdGetStderr(void)
{
    gAcpStderr.mFP = stderr;
    return &gAcpStderr;
}



#endif /* End of Windows code */

/**
 * clears the stream Error code
 * @param aFile pointer to the file object
 * @return result code
 */
ACP_EXPORT void acpStdClearError(acp_std_file_t* aFile)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aFile->mError = ACP_RC_SUCCESS;
#else
    clearerr(aFile->mFP);
#endif
}

#if defined(ALTI_CFG_OS_WINDOWS)

static acp_rc_t acpStdParseMode(const acp_char_t* aMode, acp_sint32_t* aFlag)
{
    acp_rc_t        sRC;
    acp_sint32_t    sFlag = 0;

    ACP_TEST_RAISE(NULL == aMode, E_FAULT);

    switch(*aMode)
    {
    case 'r':
        sFlag = ACP_O_RDONLY;
        break;
    case 'w':
        sFlag = ACP_O_CREAT | ACP_O_WRONLY | ACP_O_TRUNC;
        break;
    case 'a':
        sFlag = ACP_O_CREAT | ACP_O_WRONLY | ACP_O_APPEND;
        break;
    default:
        ACP_RAISE(E_INVAL);
        break;
    }

    /* Step forward */
    aMode++;
    if('b' == *aMode)
    {
        /* In mode string, 'b' has no effect */
        aMode++;
    }
    else
    {
        /* Do nothing */
    }

    if('+' == *aMode)
    {
        /* Open as read-write mode */
        sFlag &= ~ACP_O_RDONLY;
        sFlag &= ~ACP_O_WRONLY;

        sFlag |= ACP_O_RDWR;
    }
    else
    {
        /* Trailing mode chars has no effect */
    }

    *aFlag = sFlag;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_FAULT)
    {
        sRC = ACP_RC_EFAULT;
    }
    ACP_EXCEPTION(E_INVAL)
    {
        sRC = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION_END;
    return sRC;
}

#endif


/**
 * opens a file stream
 * @param aFile pointer to store file stream object opened
 * @param aPath file name
 * @param aMode mode for open; it is same as standard fopen()
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdOpen(acp_std_file_t*      aFile,
                               const acp_char_t*    aPath,
                               const acp_char_t*    aMode)
{
    acp_path_pool_t sPool;
    acp_char_t*     sPath = NULL;
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_sint32_t    sFlag;
#endif
    acp_rc_t        sRC;

    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);

    ACP_TEST_RAISE(NULL == sPath, E_PATHNULL);
#if defined(ALTI_CFG_OS_WINDOWS)
    sRC = acpStdParseMode(aMode, &sFlag);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_INVAL);
    sRC = acpFileOpen(&(aFile->mHandle), sPath, sFlag, ACP_STD_DEFAULT_PERM);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_OPENFAILWIN);

    aFile->mError = ACP_RC_SUCCESS;
    aFile->mUngetc = 0;
    aFile->mIsUngetc = ACP_FALSE;
#else
    aFile->mFP = fopen(sPath, aMode);
    ACP_TEST_RAISE(NULL == aFile->mFP, E_OPENFAIL);
#endif

    acpPathPoolFinal(&sPool);
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_PATHNULL);
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
#if defined(ALTI_CFG_OS_WINDOWS)
    ACP_EXCEPTION(E_INVAL)
    {
        aFile->mError = ACP_RC_EINVAL;
    }
    ACP_EXCEPTION(E_OPENFAILWIN)
    {
        aFile->mError = sRC;
    }
#else
    ACP_EXCEPTION(E_OPENFAIL)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
#endif

    acpPathPoolFinal(&sPool);
    ACP_EXCEPTION_END;
    return sRC;
}

/**
 * closes a file stream
 * @param aFile file stream object to close
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdClose(acp_std_file_t *aFile)
{
    acpStdClearError(aFile);
#if defined(ALTI_CFG_OS_WINDOWS)
    aFile->mError = acpFileClose(&(aFile->mHandle));
    return aFile->mError;
#else
    return (0 == fclose(aFile->mFP)) ? ACP_RC_SUCCESS : ACP_RC_GET_OS_ERROR();
#endif
}

/**
 * flushes the stream
 * @param aFile pointer to the file object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdFlush(acp_std_file_t *aFile)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t sRC;

    /* In Windows, syncing console std's generates error */
    if((aFile != ACP_STD_IN) &&
       (aFile != ACP_STD_OUT) &&
       (aFile != ACP_STD_ERR) &&
       (aFile != NULL))
    {
        aFile->mError = acpFileSync(&(aFile->mHandle));
        sRC = aFile->mError;
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }
        
    return sRC;
#else
    return (0 == fflush(aFile->mFP)) ? ACP_RC_SUCCESS : ACP_RC_GET_OS_ERROR();
#endif
}

/**
 * checks EOF of the stream
 * @param aFile pointer to the file object
 * @param aIsEOF pointer to the bool value to store whether aFile is EOF
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdIsEOF(acp_std_file_t* aFile,
                                acp_bool_t*     aIsEOF)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t        sRC;
    acp_stat_t      sStat;
    acp_offset_t    sOffset;

    sRC = acpFileStat(&(aFile->mHandle), &sStat);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    sRC = acpStdSeek(aFile, 0, ACP_SEEK_CUR, &sOffset);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));

    *aIsEOF = (sOffset == sStat.mSize)? ACP_TRUE : ACP_FALSE;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    aFile->mError = sRC;
    return sRC;
#else
    *aIsEOF = (0 == feof(aFile->mFP)) ? ACP_FALSE : ACP_TRUE;
    return ACP_RC_SUCCESS;
#endif
}

/**
 * checks whether the stream is terminal
 * @param aFile pointer to the file object
 * @return result code
 */
ACP_EXPORT acp_bool_t acpStdIsTTY(acp_std_file_t* aFile)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_bool_t sRet;

    /*
     * Check whether aFile is equivalent to stdios
     */
    if((aFile->mHandle.mHandle == GetStdHandle(STD_INPUT_HANDLE)) ||
       (aFile->mHandle.mHandle == GetStdHandle(STD_OUTPUT_HANDLE)) ||
       (aFile->mHandle.mHandle == GetStdHandle(STD_ERROR_HANDLE)))
    {
        sRet = (INVALID_HANDLE_VALUE == aFile->mHandle.mHandle)?
                ACP_FALSE : ACP_TRUE;
    }
    else
    {
        sRet = ACP_FALSE;
    }

    return sRet;
#else
    return isatty(fileno(aFile->mFP))? ACP_TRUE : ACP_FALSE;
#endif
}

/**
 * gets the error number the stream
 * @param aFile pointer to the file object
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdGetError(acp_std_file_t* aFile)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return aFile->mError;
#else
    return ferror(aFile->mFP);
#endif
}

/**
 * gets the current position of the file stream
 * @param aFile file stream object
 * @param aPos pointer to the variable to store the current position
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdTell(acp_std_file_t *aFile, acp_offset_t *aPos)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return (aFile->mError =
            acpFileSeek(&(aFile->mHandle), 0, ACP_SEEK_CUR, aPos));
#else
    acp_offset_t sRet = ftello(aFile->mFP);

    if(aPos == NULL)
    {
        /* Nothing to do with NULL */
    }
    else
    {
        *aPos = sRet;
    }

    return (-1 != sRet)? ACP_RC_SUCCESS : ACP_RC_GET_OS_ERROR();
#endif
}

/**
 * gets the current position of the file stream
 * @param aFile file stream object
 * @param aPosition position to move
 * @param aWhence position to move from
 * ACP_STD_SEEKCUR current position
 * ACP_STD_SEEKSET the beginning of the file
 * ACP_STD_SEEKEND the end of the file
 * @param aAbsPosition pointer to the variable to store the current position
 *       after seek
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdSeek(acp_std_file_t*  aFile,
                               acp_offset_t     aPosition,
                               acp_sint32_t     aWhence,
                               acp_offset_t*    aAbsPosition)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aFile->mIsUngetc = ACP_FALSE;
    return (aFile->mError = 
            acpFileSeek(&(aFile->mHandle), aPosition, aWhence, aAbsPosition));
#else
    ACP_TEST(0 != fseeko(aFile->mFP, aPosition, aWhence));
    return acpStdTell(aFile, aAbsPosition);

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();
#endif
}

/**
 * reads a byte from the stream
 * @param aFile pointer to the file object
 * @param aByte pointer to a variable to store the byte read
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdGetByte(
    acp_std_file_t* aFile,
    acp_byte_t* aByte)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t sRC;
    if(ACP_TRUE == aFile->mIsUngetc)
    {
        *aByte = (acp_byte_t)aFile->mUngetc;
        aFile->mIsUngetc = ACP_FALSE;
        sRC = ACP_RC_SUCCESS;
    }
    else
    {
        sRC = acpFileRead(&(aFile->mHandle), aByte, 1, NULL);
    }

    return sRC;
#else
    acp_sint32_t sRet = getc(aFile->mFP);
    acp_rc_t     sRC;

    ACP_TEST_RAISE(sRet == EOF, E_CANNOTREAD);
    *aByte = (acp_byte_t)sRet;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_CANNOTREAD)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    ACP_EXCEPTION_END;
    return (ACP_RC_IS_SUCCESS(sRC))? ACP_RC_EOF : sRC;
#endif
}

/**
 * pushes back a byte to the stream. only one pushback is guaranteed
 * @param aFile pointer to the file object
 * @param aByte the byte to push back
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdUngetByte(
    acp_std_file_t* aFile,
    acp_byte_t aByte)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aFile->mUngetc      = (acp_sint32_t)aByte;
    aFile->mIsUngetc    = ACP_TRUE;
    aFile->mError       = ACP_RC_SUCCESS;

    return ACP_RC_SUCCESS;
#else
    return (EOF == ungetc((acp_sint32_t)aByte, aFile->mFP))?
        ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
#endif
}

/**
 * writes a character into the stream
 * @param aFile pointer to the file object
 * @param aByte byte to put to aFile
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdPutByte(
    acp_std_file_t* aFile,
    acp_byte_t aByte)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return (aFile->mError = acpFileWrite(&(aFile->mHandle), &aByte, 1, NULL));
#else
    return (EOF == putc((acp_sint32_t)aByte, aFile->mFP))?
        ACP_RC_GET_OS_ERROR() :
        ACP_RC_SUCCESS;
#endif
}

/**
 * reads a character from the stream
 * @param aFile pointer to the file object
 * @param aChar pointer to a variable to store the character read
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdGetChar(acp_std_file_t *aFile, acp_char_t *aChar)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t sRC;
    if(ACP_TRUE == aFile->mIsUngetc)
    {
        *aChar = (acp_char_t)aFile->mUngetc;
        aFile->mIsUngetc = ACP_FALSE;
        aFile->mError    = ACP_RC_SUCCESS;
        sRC = ACP_RC_SUCCESS;
    }
    else
    {
        sRC = acpFileRead(&(aFile->mHandle), aChar, 1, NULL);
    }

    return sRC;
#else
    return acpStdGetByte(aFile, (acp_byte_t*)aChar);
#endif
}

/**
 * writes a character into the stream
 * @param aFile pointer to the file object
 * @param aChar character to put to aFile
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdPutChar(acp_std_file_t* aFile, acp_char_t aChar)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return (aFile->mError = acpFileWrite(&(aFile->mHandle), &aChar, 1, NULL));
#else
    return acpStdPutByte(aFile, (acp_byte_t)aChar);
#endif
}

/**
 * reads a NULL-terminated string from the stream until newline is detected
 * You must check EOF with acpStdIsEOF at every time calling acpStdGetCString.
 * @param aFile pointer to the file object
 * @param aStr pointer to store string read. '\\0' is always appended.
 * @param aMaxLen max length + 1 to read. +1 is for '\\0'.
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdGetCString(acp_std_file_t* aFile,
                                     acp_char_t*     aStr,
                                     acp_size_t      aMaxLen)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t sRC = ACP_RC_SUCCESS;
    ACP_TEST(aMaxLen <= 0);

    if(1 == aMaxLen)
    {
        *aStr = ACP_CSTR_TERM;
        ACP_RAISE(E_DONE);
    }
    else
    {
        /* Fall through */
    }

    if(ACP_TRUE == aFile->mIsUngetc)
    {
        aFile->mIsUngetc = ACP_FALSE;
        *aStr = aFile->mUngetc;

        aStr++;
        aMaxLen--;
    }
    else
    {
        /* No char to pop out */
    }

    /* Reserve last one byte for NULL */
    while(aMaxLen > 1)
    {
        sRC = acpStdGetChar(aFile, aStr);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC) && ACP_RC_NOT_EOF(sRC));

        /*
         * BUG-30507
         * When EOF is detected, unix returns the string read so far.
         * Emulating it.
         */
        if(ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else
        {
            /* fall through */
        }

        if(0x0D == (*aStr))
        {
            /* pass Line Feed */
        }
        else if(0x0A == (*aStr))
        {
            aStr++;
            aMaxLen--;
            break;
        }
        else
        {
            aStr++;
            aMaxLen--;
        }
    }

    *aStr = ACP_CSTR_TERM;
    aFile->mError = ACP_RC_SUCCESS;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_DONE)
    {
        /* Just one byte to read. But it is filled with NUL */
        sRC = ACP_RC_SUCCESS;
    }
    ACP_EXCEPTION_END;
    aFile->mError = sRC;
    return sRC;
#else
    return (NULL == fgets(aStr, (acp_sint32_t)aMaxLen, aFile->mFP)) ?
        acpStdGetError(aFile) : ACP_RC_SUCCESS;
#endif
}

/**
 * writes a NULL-terminated string to @a aFile
 * @param aFile pointer to the file object
 * @param aStr a NULL-terminated string to be written.
 * @param aMaxLen max length to be written.
 * @param aWritten pointer to store the number of records written.
 * Trailing '\\0' shall not be written
 * @return result code
 */
ACP_EXPORT acp_rc_t acpStdPutCString(acp_std_file_t*    aFile,
                                     acp_char_t*        aStr,
                                     acp_size_t         aMaxLen,
                                     acp_size_t*        aWritten)
{
    /* Want to use fputs, but it does not support maxlen :( */
    acp_rc_t sRC;

    *aWritten = 0;
    while(0 != aMaxLen && '\0' != *aStr)
    {
        sRC = acpStdPutChar(aFile, *aStr);
        ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
            
        aMaxLen--;
        aStr++;
        (*aWritten)++;
    }

#if defined(ALTI_CFG_OS_WINDOWS)
    aFile->mError = ACP_RC_SUCCESS;
#endif
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return acpStdGetError(aFile);
}

/**
 * Reads @a aNoRecords blocks of @a aSizeRecord size from @a aFile
 * @param aFile pointer to the file object to be read
 * @param aBuf pointer to the buffer that will store the data read
 * @param aSizeRecord size of a record to be read
 * @param aNoRecords number of records to be read
 * @param aRecordsRead pointer to store the number of records read.
 * If an error occured, this value will differ from @a aNoRecords.
 * @return Result code.
 */
ACP_EXPORT acp_rc_t acpStdRead(acp_std_file_t*      aFile,
                               void*                aBuf,
                               acp_size_t           aSizeRecord,
                               acp_size_t           aNoRecords,
                               acp_size_t*          aRecordsRead)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t    sRC;
    acp_size_t  sTotalRead      = 0;
    acp_size_t  sUngetcRead     = 0;

    if(ACP_TRUE == aFile->mIsUngetc)
    {
        aFile->mIsUngetc = ACP_FALSE;
        *(acp_char_t*)aBuf = aFile->mUngetc;
        aBuf = (acp_char_t*)aBuf + 1;
        sUngetcRead = 1;
    }
    else
    {
        /* Nothing to pop out */
    }
    sRC = acpFileRead(&(aFile->mHandle), aBuf,
                      (aSizeRecord * aNoRecords) - sUngetcRead, &sTotalRead);
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    sTotalRead += sUngetcRead;
    if(NULL != aRecordsRead)
    {
        *aRecordsRead = sTotalRead / aSizeRecord;
    }
    else
    {
        /* Do nothing */
    }

    ACP_TEST_RAISE(sTotalRead < aSizeRecord * aNoRecords, E_EOF);

    aFile->mError = ACP_RC_SUCCESS;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(E_EOF)
    {
        sRC = ACP_RC_EOF;
    }

    ACP_EXCEPTION_END;
    aFile->mError = sRC;
    return sRC;

#else
    *aRecordsRead = fread(aBuf, aSizeRecord, aNoRecords, aFile->mFP);
    if (aNoRecords != (*aRecordsRead))
    {
        if (feof(aFile->mFP) != 0)
        {
            return ACP_RC_EOF;
        }
        else
        {
            return acpStdGetError(aFile);
        }
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
#endif
}

/**
 * Writes @a aNoRecords blocks of @a aSizeRecord size to @a aFile
 * @param aFile pointer to the file object to be written
 * @param aBuf pointer to the buffer storing the data to be written
 * @param aSizeRecord size of a record in @a aBuf
 * @param aNoRecords number of records in @a aBuf
 * @param aRecordsWritten pointer to store the number of records written.
 * If an error occured, this value will differ from @a aNoRecords.
 * @return Result code.
 */
ACP_EXPORT acp_rc_t acpStdWrite(acp_std_file_t*     aFile,
                                const void*         aBuf,
                                acp_size_t          aSizeRecord,
                                acp_size_t          aNoRecords,
                                acp_size_t*         aRecordsWritten)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t    sRC;
    acp_size_t  sTotalWritten = 0;

    sRC = acpFileWrite(&(aFile->mHandle), aBuf,
                      aSizeRecord * aNoRecords, &sTotalWritten);
    if(NULL != aRecordsWritten)
    {
        *aRecordsWritten = sTotalWritten / aSizeRecord;
    }
    else
    {
        /* Do nothing */
    }
    ACP_TEST(ACP_RC_NOT_SUCCESS(sRC));
    aFile->mError = ACP_RC_SUCCESS;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    aFile->mError = sRC;
    return sRC;

#else
    acp_size_t  sTotalWritten;
    sTotalWritten = fwrite(aBuf, aSizeRecord, aNoRecords, aFile->mFP);
    if(NULL != aRecordsWritten)
    {
        *aRecordsWritten = sTotalWritten;
    }
    else
    {
        /* Do nothing */
    }

    return (aNoRecords == sTotalWritten)?
        ACP_RC_SUCCESS : ACP_RC_EPERM;
#endif
}

/**
 * reads a string from the stream
 * @param aFile pointer to the file object
 * @param aString pointer to the string object
 * @return result code
 *
 * it reads a string to end of file or newline.
 * the newline character will be included and null terminated
 *
 * it returns #ACP_RC_EOF if end of file.
 * it returns #ACP_RC_ETRUNC if the string is truncated
 * because of insufficient buffer.
 * it returns #ACP_RC_ENOMEM if it cannot allocate internal buffer.
 */

ACP_EXPORT acp_rc_t acpStdGetString(acp_std_file_t *aFile, acp_str_t *aString)
{
    acp_char_t   *sBuffer = acpStrGetBuffer(aString);
    acp_sint32_t  sOffset;
    acp_sint32_t  sLen;
    acp_rc_t      sRC;

    if (sBuffer == NULL)
    {
        return ACP_RC_ENOMEM;
    }
    else
    {
        /* just go to process*/
    }
    
    sOffset = 0;
    sLen    = (acp_sint32_t)acpStrGetBufferSize(aString);

    if (sLen == 0)
    {
        if (aString->mExtendSize > 0)
        {
            sRC = acpStrExtendBuffer(aString, 1);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                return sRC;
            }
            else
            {
                sBuffer = acpStrGetBuffer(aString);
                sLen    = (acp_sint32_t)acpStrGetBufferSize(aString);
            }
        }
        else
        {
            return ACP_RC_ENOMEM;
        }
    }
    else
    {
        /* do nothing */
    }

    while (1)
    {
        sRC = acpStdGetCString(aFile, sBuffer + sOffset, sLen - sOffset);

        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            if(ACP_RC_IS_EOF(sRC))
            {
                acpStrClear(aString);
                return ACP_RC_EOF;
            }
            else
            {
                acpStrResetLength(aString);
                return ACP_RC_SUCCESS;
            }
        }
        else
        {
            acpStrResetLength(aString);

            sOffset = (acp_sint32_t)acpStrGetLength(aString);

#if defined(__STATIC_ANALYSIS_DOING__)
            /* actually, sOffset can't be zero,
               but, codesonar assume it can be zero,
               so skip the condition only on codesonar state
            */
            sOffset = (sOffset == 0) ? 1 : sOffset;
#endif            
            if (acpStrGetChar(aString, sOffset - 1) == '\n')
            {
                return ACP_RC_SUCCESS;
            }
            else
            {
                if (aString->mExtendSize > 0)
                {
                    sRC = acpStrExtendBuffer(aString, sLen + 1);
                        
                    if (ACP_RC_NOT_SUCCESS(sRC))
                    {
                        return ACP_RC_ETRUNC;
                    }
                    else
                    {
                        sLen = (acp_sint32_t)acpStrGetBufferSize(aString);
                        sBuffer = acpStrGetBuffer(aString);
                    }
                }
                else
                {
                    return ACP_RC_ETRUNC;
                }
            }
        }
    }
}

ACP_EXPORT acp_bool_t acpStdDupStderr(acp_std_file_t aFile)
{
    return (dup2(fileno(aFile.mFP), fileno(stderr)) != -1) ? ACP_TRUE : ACP_FALSE;
}
