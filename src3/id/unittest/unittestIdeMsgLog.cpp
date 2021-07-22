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
* $Id$
*
* Description :
*
*  Provides unit tests for lock-free trace logging.  PROJ-2278
*  introduced the lock-free trace logging.
*
*******************************************************************************/

#include <acp.h>
#include <act.h>
#include <ide.h>

#define FOR_EACH_LOG_FILE( aName )                                      \
    {                                                                   \
        acp_dir_t sDir;                                                 \
        acp_rc_t  sRC;                                                  \
        /* delete log files */                                          \
        sRC = acpDirOpen( &sDir, (char*)"." );                          \
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));                              \
                                                                        \
        for ( ; ; )                                                     \
        {                                                               \
            sRC = acpDirRead(&sDir, &aName);                            \
                                                                        \
            if ( ACP_RC_NOT_SUCCESS( sRC ) )                            \
            {                                                           \
                /* If do not break here,                                \
                 * for-loop increase sCount one more time,              \
                 * and then sCount is over than the number of files     \
                 */                                                     \
                break;                                                  \
            }                                                           \
            else                                                        \
            {                                                           \
                /* do nothing */                                        \
            }                                                           \
                                                                        \
            if ( beginsWith( aName, "unittestIdeMsgLog.log" ) )         \
            {

#define FOR_EACH_LOG_FILE_END                                           \
            }                                                           \
            else                                                        \
            {                                                           \
                /* do nothing */                                        \
            }                                                           \
        }                                                               \
                                                                        \
        acpDirClose( &sDir );                                           \
    }

const acp_char_t *const TEST_FILE_NAME         = "unittestIdeMsgLog.log";
const acp_char_t *const TEST_FILE_NAME_2       = "unittestIdeMsgLog.log-1";
const acp_char_t *const TEST_FILE_NAME_3       = "unittestIdeMsgLog.log-2";
const acp_char_t *const TEST_LOG_HEADER_PREFIX = "* ALTIBASE LOGFILE-1";

/* Current algorithm for stress test supports up to 10 concurrent
 * threads. */
const acp_uint32_t NUM_THREADS = ( idlVA::getProcessorCount() > 10 ) ? 10 : idlVA::getProcessorCount();
const acp_uint32_t NUM_ITER    = 5;

static ideMsgLog  gLog;
static acp_bool_t gOptVerbose    = ACP_FALSE;
static acp_bool_t gOptStressOnly = ACP_FALSE;

struct unittestIdeThreadInfo
{
    acp_sint32_t   mIndex;
};


static acp_bool_t beginsWith(acp_char_t *aStr, const acp_char_t *aWith)
{
    acp_sint32_t sFoundIndex;
    acp_rc_t     sRC;

    sRC = acpCStrFindCStr(aStr, (acp_char_t*)aWith, &sFoundIndex, 0, 0);
    return ACP_RC_IS_SUCCESS(sRC) && sFoundIndex == 0;
}

/*
 * Sets up trace logging test
 */
void unittestIdeSetupLog( ideMsgLogStrategy aStrategy,
                          acp_size_t aFileSize = 256,
                          acp_bool_t aNoClean = ACP_FALSE )
{
    acp_char_t *sEntryName;

    if ( !aNoClean )
    {
        /* delete log files */
        FOR_EACH_LOG_FILE( sEntryName )
        {
            ACT_ASSERT( ACP_RC_IS_SUCCESS( acpFileRemove( sEntryName ) ) );
        }
        FOR_EACH_LOG_FILE_END;
    }
    else
    {
        /* do nothing */
    }

    ACT_CHECK(IDE_SUCCESS == gLog.initialize(1,
                                             TEST_FILE_NAME,
                                             aFileSize,
                                             50,
                                             aStrategy));
    ACT_CHECK(IDE_SUCCESS == gLog.open());
}

/*
 * Tears down trace logging test
 */
void unittestIdeTeardownLog()
{
    ACT_CHECK(IDE_SUCCESS == gLog.close());
    ACT_CHECK(IDE_SUCCESS == gLog.destroy());
}

/*
 * Tests that the log file is created correctly
 */
void unittestIdeTestCreateLogFile( ideMsgLogStrategy aStrategy )
{
    acp_rc_t  sRC;

    unittestIdeSetupLog( aStrategy );
    unittestIdeTeardownLog();

    /* checking if the file was successfully created and has header
       that looks correct */
    acp_std_file_t sFile;
    acp_sint32_t   sFoundIndex;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, (acp_char_t*)TEST_FILE_NAME, "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpCStrFindCStr(sLine,
                          const_cast<acp_char_t*> (TEST_LOG_HEADER_PREFIX),
                          &sFoundIndex,
                          0,
                          0);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( sFoundIndex == 0 );
}


/*
 * Tests logging
 */
void unittestIdeTestLogging( ideMsgLogStrategy aStrategy )
{
    const acp_char_t * const TEST_MESSAGE = "Logging test from unittestIdeTestLogging()\n";

    acp_rc_t  sRC;

    unittestIdeSetupLog( aStrategy );

    ACT_CHECK(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<char*>(TEST_FILE_NAME), "r");
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(0 == acpCStrCmp(sLine, TEST_MESSAGE, 256));
}

/*
 * Tests log rotation
 */
void unittestIdeTestRotation( ideMsgLogStrategy aStrategy )
{
    const acp_char_t * const TEST_MESSAGE = "012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n";

    acp_rc_t  sRC;

    unittestIdeSetupLog( aStrategy );

    ACT_CHECK(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    /* this would cause log rotation */
    ACT_CHECK(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    unittestIdeTeardownLog();

    /* was there log rotation? */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<char*>(TEST_FILE_NAME_2), "r");
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(0 == acpCStrCmp(sLine, TEST_MESSAGE, 256));
}

/*
 * Tests log rotation numbering
 */
void unittestIdeTestRotationNumbering( ideMsgLogStrategy aStrategy )
{
    const acp_char_t * const TEST_MESSAGE = "012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n";

    acp_std_file_t sFile;
    acp_rc_t       sRC;

    sRC = acpFileRemove(const_cast<acp_char_t*>(TEST_FILE_NAME_3));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ENOENT(sRC));

    /* causing two log rotations */
    unittestIdeSetupLog( aStrategy );

    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));
    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));
    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    unittestIdeTeardownLog();

    /* check for the correct log header in each trace file */
    acp_sint32_t sFoundIndex;
    acp_char_t   sBuf[256];

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));
    ACT_ASSERT(0 == acpCStrFindCStr(sBuf, "* ALTIBASE LOGFILE-3", &sFoundIndex, 0, 0));

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME_2), "r")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));
    ACT_ASSERT(0 == acpCStrFindCStr(sBuf, "* ALTIBASE LOGFILE-1", &sFoundIndex, 0, 0));

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME_3), "r")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));
    ACT_ASSERT(0 == acpCStrFindCStr(sBuf, "* ALTIBASE LOGFILE-2", &sFoundIndex, 0, 0));
}

void unittestIdeTestCorruptLogHandling( ideMsgLogStrategy aStrategy )
{
    const acp_char_t * const TEST_MESSAGE = "Logging test from unittestIdeTestCorruptLogHandling()\n";

    acp_std_file_t sFile;
    acp_size_t     sWritten;

    /* create a corrupt log file */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "w")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdPutCString(&sFile, "oh shoot, the file is corrupted :-(", 50, &sWritten)));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));

    unittestIdeSetupLog( aStrategy, 256, ACP_TRUE );

    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    unittestIdeTeardownLog();

    /* verify the numbering has reset to 1 */
    acp_char_t   sBuf[256];
    acp_sint32_t sFoundIndex;

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));
    ACT_ASSERT(0 == acpCStrFindCStr(sBuf, "* ALTIBASE LOGFILE-1", &sFoundIndex, 0, 0));
}

void unittestIdeTestLogReserve( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *const TEST_MESSAGE = "Logging test from unittestIdeTestLogReserve()\n";

    acp_std_file_t sFile;

    /* Write log */
    unittestIdeSetupLog( aStrategy, 2048 );
    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));
    unittestIdeTeardownLog();

    /* Check the reserve bytes were correctly filled */
    acp_byte_t   sByte;
    acp_sint32_t i;
    acp_char_t   sBuf[256];

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256))); /*ignore*/
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256))); /*ignore*/
    for (i = 0; i < 7; i++)
    {
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetByte(&sFile, &sByte)));
        ACT_ASSERT(sByte == IDE_RESERVE_FILL_CHAR);
    }

    /* Check for new line characters.  Inserting them makes it easier
     * to view the trace log file in a text editor. */
    acp_bool_t sNewLineFound = ACP_FALSE;
    acp_bool_t sIsEOF        = ACP_FALSE;

    do
    {
        ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdGetByte(&sFile, &sByte) ) );

        if ( sByte == '\n' )
        {
            sNewLineFound = ACP_TRUE;
            break;
        }
        else
        {
            /* do nothing */
        }

        ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdIsEOF(&sFile, &sIsEOF) ) );
    }
    while ( !sIsEOF );

    ACT_ASSERT( sNewLineFound );

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));
}

/*
 * logic to run in our threads used for stress test
 */
acp_sint32_t unittestIdeLoggerProc(void *aArg)
{
    acp_char_t        sStr[32];
    const acp_char_t *sFormat = "Thread #%d: Iteration #%d\n";
    acp_uint32_t      i;
    unittestIdeThreadInfo *sInfo = static_cast<unittestIdeThreadInfo*>(aArg);

    for (i = 0; i < NUM_ITER; i++)
    {
        ACT_CHECK( ACP_RC_IS_SUCCESS( acpSnprintf( sStr, 32, sFormat, sInfo->mIndex, i ) ) );
        ACT_CHECK( ACP_RC_IS_SUCCESS( gLog.logBody( sStr ) ) );
    }

    acpMemFree(aArg);

    return 0;
}

/*
 * stress test
 */
void unittestIdeStressTest( ideMsgLogStrategy aStrategy )
{
    acp_char_t   *sEntryName;
    acp_thr_t    *sThr;
    acp_uint32_t  i;
    acp_uint32_t *sOccurrences;
    unittestIdeThreadInfo *sInfo;

    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpMemCalloc( (void**)&sOccurrences, NUM_THREADS * NUM_ITER, sizeof( acp_uint32_t ) ) ) );
    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpMemCalloc( (void**)&sThr, NUM_THREADS, sizeof( acp_thr_t ) ) ) );

    if ( gOptVerbose )
    {
        acpFprintf( ACP_STD_ERR, "Number of threads: %d\n", NUM_THREADS );
    }
    else
    {
        /* do nothing */
    }

    unittestIdeSetupLog( aStrategy );

    for (i = 0; i < NUM_THREADS; i++)
    {
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpMemAlloc(reinterpret_cast<void**>(&sInfo), sizeof(unittestIdeThreadInfo))));
        sInfo->mIndex = i;
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpThrCreate(&sThr[i], NULL, unittestIdeLoggerProc, sInfo)));
    }

    for (i = 0; i < NUM_THREADS; i++)
    {
        acp_sint32_t sRet;
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpThrJoin(&sThr[i], &sRet)));
    }

    unittestIdeTeardownLog();

    /* Check #1: Verify each line */
    /* Check #2: See if there's any line that's missing */

    /* Open each log file and parse the output lines */
    FOR_EACH_LOG_FILE( sEntryName )
    {
        const acp_uint32_t BUF_SIZE = 256;

        acp_char_t     sBuf[BUF_SIZE];
        acp_char_t    *sLine;
        acp_sint32_t   sLineCount = 0;
        acp_std_file_t sFile;
        acp_bool_t     sIsEOF;
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, sEntryName, "r")));
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, BUF_SIZE))); /*ignore the header*/
        while (ACP_RC_IS_SUCCESS(acpStdIsEOF(&sFile, &sIsEOF)) && !sIsEOF)
        {
            acpMemSet( (void*)sBuf, '\0', BUF_SIZE );

            sRC = acpStdGetCString(&sFile, sBuf, BUF_SIZE);
            ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

            /* Skip any leading reserve characters.  Note leading
             * reserve characters probably mean there's a logic
             * error. */
            sLine = sBuf;
            while ( ( *sLine == IDE_RESERVE_FILL_CHAR ) ||
                    ( *sLine == IDE_RESERVE_LINE_BREAK ) )
            {
                ++sLine;
            }

            if ( acpCStrLen( sLine, BUF_SIZE ) == 0 )
            {
                continue;
            }
            else
            {
                /* do nothing */
            }

            ACT_CHECK_DESC( sLine == sBuf, ( "Leading reserve characters" ) );

            ++sLineCount;

            acp_char_t   sThreadChar = sLine[8];
            acp_char_t   sIterChar   = sLine[22];
            acp_uint32_t sThread = 0;
            acp_uint32_t sIter = 0;
            acp_sint32_t sSign;
            acp_char_t   *sEnd;

            if ( gOptVerbose )
            {
                sLine[23] = ' ';
                acpFprintf( ACP_STD_ERR, "[ %s]\n", sLine );
            }
            else
            {
                /* do nothing */
            }

            ACT_ASSERT(ACP_RC_IS_SUCCESS(acpCStrToInt32(&sThreadChar, 1, &sSign,
                                                        &sThread, 10, &sEnd)));
            ACT_ASSERT(ACP_RC_IS_SUCCESS(acpCStrToInt32(&sIterChar, 1, &sSign,
                                                        &sIter, 10, &sEnd)));

            ACT_CHECK_DESC(24 == acpCStrLen(sLine, BUF_SIZE),
                           ("String of unexpected length: \"%s\"\n", sLine));
            ACT_CHECK_DESC(sThreadChar != ACP_CSTR_TERM && sIterChar != ACP_CSTR_TERM,
                           ("Found an unexpected log line: \"%s\"\n", sLine));

            if ( sThreadChar != ACP_CSTR_TERM && sIterChar != ACP_CSTR_TERM )
            {
                ACT_ASSERT( sThread < NUM_THREADS );
                ACT_ASSERT( sIter   < NUM_ITER );
                sOccurrences[ sThread * NUM_ITER + sIter ]++;
            }
            else
            {
                /* do nothing */
            }
        }

        ACT_CHECK_DESC(sLineCount != 0,
                       ("Unexpected log file with zero log entries: %s\n", sEntryName));

        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdClose(&sFile)));
    }
    FOR_EACH_LOG_FILE_END;

    /* Verify that each thread - iteration pair appears exactly
     * once */
    for ( i = 0 ; i < NUM_THREADS ; i++ )
    {
        for (acp_uint32_t j = 0; j < NUM_ITER; j++)
        {
            ACT_CHECK_DESC(sOccurrences[i * NUM_ITER + j] >= 1,
                           ("No occurence for thread #%u iteration #%u\n", i, j));
            ACT_CHECK_DESC(sOccurrences[i * NUM_ITER + j] <= 1,
                           ("More than one occurence for thread #%u iteration #%u\n", i, j));
        }
    }

    acpMemFree( sThr );
    acpMemFree( sOccurrences );
}

/*
 * Tests log rotation when the log file exists.
 */
void unittestIdeTestLogRotateExisting( ideMsgLogStrategy aStrategy )
{
    const acp_char_t * const TEST_MESSAGE = "012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n";

    acp_rc_t  sRC;

    unittestIdeSetupLog( aStrategy );

    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    unittestIdeTeardownLog();

    unittestIdeSetupLog( aStrategy, 256, ACP_TRUE);

    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));

    unittestIdeTeardownLog();

    /* was there log rotation? */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<char*>(TEST_FILE_NAME_2), "r");
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdClose(&sFile);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(0 == acpCStrCmp(sLine, TEST_MESSAGE, 256));
}


/*
 * Continue logging in the existing reserved trace log file.
 */
void unittestIdeTestContinueReservedLog( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *const TEST_MESSAGE  = "unittestIdeTestContinueReservedLog(): Test message 1\n";
    const acp_char_t *const TEST_MESSAGE2 = "unittestIdeTestContinueReservedLog(): Test message 2\n";

    acp_std_file_t sFile;

    /* Write log */
    unittestIdeSetupLog( aStrategy, 2048 );
    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE));
    unittestIdeTeardownLog();
    unittestIdeSetupLog( aStrategy, 2048, ACP_TRUE );
    ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE2));
    unittestIdeTeardownLog();

    /* Check the reserve bytes were correctly filled */
    acp_rc_t     sRC;
    acp_char_t   sBuf[256];
    acp_sint32_t sFoundIndex;

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r")));

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    sRC = acpCStrFindCStr(sBuf,
                          const_cast<acp_char_t*> (TEST_LOG_HEADER_PREFIX),
                          &sFoundIndex,
                          0,
                          0);
    ACT_ASSERT( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( sFoundIndex == 0 );

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    ACT_ASSERT(0 == acpCStrCmp(sBuf, TEST_MESSAGE, 256));

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    ACT_ASSERT(0 == acpCStrCmp(sBuf, TEST_MESSAGE2, 256));

    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdClose(&sFile) ) );
}

void unittestIdeTestRotateAndAppend( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *const TEST_MESSAGE_1 = "unittestIdeTestRotateAndAppend(): Test Message 1\n";
    const acp_char_t *const TEST_MESSAGE_2 = "unittestIdeTestRotateAndAppend(): Test Message 2\n";

    acp_char_t     sBuf[256];
    acp_sint32_t   i;
    acp_std_file_t sFile;

    /* Cause a rotation */
    unittestIdeSetupLog( aStrategy );
    for ( i = 0 ; i < 3 ; i++ )
    {
        ACT_ASSERT( IDE_SUCCESS == gLog.logBody( TEST_MESSAGE_1 ) );
    }
    unittestIdeTeardownLog();

    /* Cause an append */
    unittestIdeSetupLog( aStrategy, 256, ACP_TRUE );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( TEST_MESSAGE_2 ) );
    unittestIdeTeardownLog();

    /* read and ignore header */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r")));
    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));

    /* Was the message correctly appended? */
    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdGetCString( &sFile, sBuf, 256 ) ) );
    ACT_ASSERT( 0 == acpCStrCmp( sBuf, TEST_MESSAGE_1, 256 ) );
    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdGetCString( &sFile, sBuf, 256 ) ) );
    ACT_ASSERT( 0 == acpCStrCmp( sBuf, TEST_MESSAGE_2, 256 ) );

    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdClose(&sFile) ) );
}

void unittestIdeTestAppendToLargeFile( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *const TEST_MESSAGE_SUBSTR =
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345678\n";
    const acp_char_t *const TEST_MESSAGE = "TGIF";

    acp_std_file_t sFile;
    acp_sint32_t   i;

    /* Write log */
    unittestIdeSetupLog( aStrategy, 1024*1024);
    for ( i = 0 ; i < 10480 ; i++ )
    {
        ACT_ASSERT(IDE_SUCCESS == gLog.logBody(TEST_MESSAGE_SUBSTR));
    }
    unittestIdeTeardownLog();
    unittestIdeSetupLog( aStrategy, 1024*1024, ACP_TRUE);
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( TEST_MESSAGE ) );
    unittestIdeTeardownLog();

    /* Check the reserve bytes were correctly filled */
    acp_rc_t     sRC;
    acp_char_t   sBuf[256];
    acp_sint32_t sFoundIndex;

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r")));

    ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 256)));
    sRC = acpCStrFindCStr(sBuf,
                          const_cast<acp_char_t*> (TEST_LOG_HEADER_PREFIX),
                          &sFoundIndex,
                          0,
                          0);
    ACT_ASSERT( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( sFoundIndex == 0 );

    for ( i = 0 ; i < 10480 ; i++ )
    {
        ACT_ASSERT(ACP_RC_IS_SUCCESS(acpStdGetCString(&sFile, sBuf, 101)));
        ACT_ASSERT(0 == acpCStrCmp(sBuf, TEST_MESSAGE_SUBSTR, 101));
    }

    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdGetCString( &sFile, sBuf, 5 ) ) );
    ACT_ASSERT( 0 == acpCStrCmp( sBuf, TEST_MESSAGE, 5 ) );

    ACT_ASSERT( ACP_RC_IS_SUCCESS( acpStdClose(&sFile) ) );
}

void unittestIdeTestLargeMessage( ideMsgLogStrategy aStrategy )
{
    acp_char_t sMessage[512];
    acpMemSet( sMessage, '.', 511 );
    sMessage[511] = '\0';

    unittestIdeSetupLog( aStrategy );

    ACT_CHECK(IDE_FAILURE == gLog.logBody(sMessage));

    unittestIdeTeardownLog();
}

/*
 * Test case for enlarging a file and appending to it.
 */
void unittestIdeTestAppendToEnlargedFile( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *sMessage1 = "Hello,\n";
    const acp_char_t *sMessage2 = "Altibase!\n";
    acp_rc_t          sRC;

    unittestIdeSetupLog( aStrategy, 256 );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( sMessage1 ) );
    unittestIdeTeardownLog();

    unittestIdeSetupLog( aStrategy, 320, ACP_TRUE );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( sMessage2 ) );
    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<char*>(TEST_FILE_NAME), "r");
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore message 1 */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdClose(&sFile);
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    ACT_ASSERT(0 == acpCStrCmp(sLine, sMessage2, 256));
}

void unittestIdeTestEmptyLine( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *sMessage1 = "Hello\n";
    const acp_char_t *sMessage2 = "Altibase!\n";
    acp_rc_t          sRC;

    unittestIdeSetupLog( aStrategy );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( sMessage1 ) );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( "\n" ) );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( "\n" ) );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( "\n" ) );
    unittestIdeTeardownLog();
    unittestIdeSetupLog( aStrategy, 256, ACP_TRUE );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( sMessage2 ) );
    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<char*>(TEST_FILE_NAME), "r");
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* message 1 */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* empty line */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256); /* empty line */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256); /* empty line */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* message 2 */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    ACT_ASSERT(0 == acpCStrCmp(sLine, sMessage2, 256));

    sRC = acpStdClose(&sFile);
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
}

void unittestIdeTestLineWithSingleSpace( ideMsgLogStrategy aStrategy )
{
    const acp_char_t *sMessage1 = "Hello\n";
    const acp_char_t *sMessage2 = "Altibase!\n";
    acp_rc_t          sRC;

    unittestIdeSetupLog( aStrategy );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( sMessage1 ) );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( " \n" ) );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( " \n" ) );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( " \n" ) );
    unittestIdeTeardownLog();
    unittestIdeSetupLog( aStrategy, 256, ACP_TRUE );
    ACT_ASSERT( IDE_SUCCESS == gLog.logBody( sMessage2 ) );
    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<char*>(TEST_FILE_NAME), "r");
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* message 1 */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* empty line */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256); /* empty line */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    sRC = acpStdGetCString(&sFile, sLine, 256); /* empty line */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpStdGetCString(&sFile, sLine, 256); /* message 2 */
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
    ACT_ASSERT(0 == acpCStrCmp(sLine, sMessage2, 256));

    sRC = acpStdClose(&sFile);
    ACT_ASSERT(ACP_RC_IS_SUCCESS(sRC));
}


int main( acp_sint32_t argc, acp_char_t *argv[] )
{
    acp_sint32_t i;

    ACT_TEST_BEGIN();

    for ( i = 1 ; i < argc ; i++ )
    {
        if ( argv[i][0] == '-' )
        {
            switch ( argv[i][1] )
            {
            case 'v':
                gOptVerbose = ACP_TRUE;
                break;

            case 's':
                gOptStressOnly = ACP_TRUE;
                break;

            default:
                acpFprintf( ACP_STD_ERR, "Unknown option -%c\n", argv[i][1] );
            }
        }
        else
        {
            /* do nothing */
        }
    }

    if ( !gOptStressOnly )
    {
        unittestIdeTestCreateLogFile( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestLogging( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestRotation( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestRotationNumbering( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestCorruptLogHandling( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestLogReserve( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestLogRotateExisting( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestContinueReservedLog( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestAppendToLargeFile( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestRotateAndAppend( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestLargeMessage( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestAppendToEnlargedFile( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestEmptyLine( IDE_MSGLOG_STRATEGY_LOCK_FREE );
        unittestIdeTestLineWithSingleSpace( IDE_MSGLOG_STRATEGY_LOCK_FREE );

        /* Only perform tests relevant for the append strategy */
        unittestIdeTestCreateLogFile( IDE_MSGLOG_STRATEGY_APPEND );
        unittestIdeTestLogging( IDE_MSGLOG_STRATEGY_APPEND );
        unittestIdeTestAppendToLargeFile( IDE_MSGLOG_STRATEGY_APPEND );
    }
    else
    {
        /* do nothing */
    }

    unittestIdeStressTest( IDE_MSGLOG_STRATEGY_LOCK_FREE );

    ACT_TEST_END();

    return 0;
}
