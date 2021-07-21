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
*  Provides a set of unit tests for ideLogEntry class, first created
*  in PROJ-2278, which introduces mostly lock-free trace logging. The
*  ideLogEntry class replaces logOpen, logClose and etc functions.
*
*******************************************************************************/

#include <acp.h>
#include <act.h>
#include <ide.h>
#include <idu.h>
#include <idp.h>

const acp_char_t * const TEST_FILE_NAME = "unittestIdeLogEntry.log";
const acp_size_t         TEST_LARGE_MESSAGE_SIZE = 30 * 1024 + 17;

acp_bool_t beginsWith( const acp_char_t *aStr, const acp_char_t *aWith )
{
    acp_sint32_t sFoundIndex;
    acp_rc_t     sRC;

    sRC = acpCStrFindCStr( const_cast< acp_char_t * > ( aStr ),
                           const_cast< acp_char_t * > ( aWith ),
                           &sFoundIndex,
                           0,
                           0);
    return ACP_RC_IS_SUCCESS(sRC) && sFoundIndex == 0;
}

acp_bool_t isValidTimestampFormat( acp_char_t *aLine )
{
    /* example of timestamp line
       "[2014/02/06 17:07:37] [Thread-47375929552896] [Level-0]" */
    const acp_char_t *sFormat = "[%u/%u/%u %u:%u:%u] [Thread-%lu] [Level-%u]";
    acp_uint32_t x;
    acp_ulong_t  y;

    return 8 == sscanf( aLine, sFormat, &x, &x, &x, &x, &x, &x, &y, &x );
}

/*
 * Sets up trace logging test
 */
void unittestIdeSetupLog()
{
    /* sanitise the testing environment */
    acp_rc_t sRC = acpFileRemove((acp_char_t*)TEST_FILE_NAME);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ENOENT(sRC) );

    /* Use a bit of short-cut to initialise the IDE_SERVER log */
    ACT_ASSERT( IDE_SUCCESS == ideLog::getMsgLog(IDE_SERVER)->initialize(1,
                                                                         TEST_FILE_NAME,
                                                                         1024 * 1024,
                                                                         20,
                                                                         IDE_MSGLOG_STRATEGY_LOCK_FREE) );
    ACT_ASSERT( IDE_SUCCESS == ideLog::getMsgLog(IDE_SERVER)->open() );
}

/*
 * Tears down trace logging test
 */
void unittestIdeTeardownLog()
{
    ACT_ASSERT( IDE_SUCCESS == ideLog::getMsgLog(IDE_SERVER)->close() );
    ACT_ASSERT( IDE_SUCCESS == ideLog::getMsgLog(IDE_SERVER)->destroy() );
}

/*
 * Tests logging
 */
void unittestIdeTestLogging()
{
    const acp_char_t * const TEST_MESSAGE = "Logging test from unittestIdeTestLogging()\n";

    acp_rc_t  sRC;

    unittestIdeSetupLog();

    ideLogEntry sLog(IDE_SERVER_0);
    ACT_ASSERT( IDE_SUCCESS == sLog.append(TEST_MESSAGE) );
    ACT_ASSERT( IDE_SUCCESS == sLog.write() );

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( beginsWith( sLine, "* ALTIBASE LOGFILE-1 (" ) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( isValidTimestampFormat( sLine ) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGE, 256) );
}

void unittestIdeTestNoLoggingFlag()
{
    const acp_char_t * const TEST_MESSAGE = "Logging test from unittestIdeTestNoLoggingFlag()\n";

    acp_rc_t  sRC;

    unittestIdeSetupLog();

    ideLogEntry sLog(ACP_FALSE, IDE_SERVER, 0);
    ACT_ASSERT( IDE_SUCCESS == sLog.append(TEST_MESSAGE) );
    ACT_ASSERT( IDE_SUCCESS == sLog.write() );

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( beginsWith( sLine, "* ALTIBASE LOGFILE-1 (" ) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    ACT_ASSERT( 0 == acpCStrCmp( sLine, "\n", 3 ) );
    ACT_ASSERT( !isValidTimestampFormat( sLine ) );
}

/*
 * Tests logging
 */
void unittestIdeTestNotLogging()
{
    const acp_char_t * const TEST_MESSAGE = "Logging test from unittestIdeTestNotLogging()\n";

    acp_rc_t  sRC;

    unittestIdeSetupLog();

    {
        ideLogEntry sLog(IDE_SERVER_0);
        ACT_ASSERT( IDE_SUCCESS == sLog.append(TEST_MESSAGE) );
        /* do not write */
    }

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( beginsWith( sLine, "* ALTIBASE LOGFILE-1 (" ) );

    /* log entry 가 기록되지 않았으면 timestamp가 없어야 한다 */
    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( !isValidTimestampFormat( sLine ) );
    ACT_ASSERT( 0 == acpCStrCmp( sLine, "\n", 3 ) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
}

/*
 * Tests multiline log output using ideLogEntry
 */
void unittestIdeTestMultiline()
{
    const acp_char_t * const TEST_MESSAGES[] = {
        "Logging test from unittestIdeTestMultiline()\n",
        "This is another line within the same multline log message\n",
        "Fantastic!\n"
    };

    acp_rc_t  sRC;

    unittestIdeSetupLog();

    ideLogEntry sLog(IDE_SERVER_0);
    ACT_ASSERT( IDE_SUCCESS == sLog.append(TEST_MESSAGES[0]) );
    ACT_ASSERT( IDE_SUCCESS == sLog.append(TEST_MESSAGES[1]) );
    ACT_ASSERT( IDE_SUCCESS == sLog.append(TEST_MESSAGES[2]) );
    ACT_ASSERT( IDE_SUCCESS == sLog.write() );

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore timestamp */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGES[0], 256) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGES[1], 256) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGES[2], 256) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
}

/*
 * Test with a large message that forces internal buffer expansion
 */
void unittestIdeTestLargeMessage()
{
    acp_char_t   sTestMessage[TEST_LARGE_MESSAGE_SIZE];
    acp_rc_t     sRC;

    /* fill up the test message */
    for ( acp_uint32_t i = 0 ; i < TEST_LARGE_MESSAGE_SIZE - 16 ; i += 16 )
    {
        ACT_CHECK( ACP_RC_IS_SUCCESS( acpCStrCpy( sTestMessage + i,
                                                  TEST_LARGE_MESSAGE_SIZE - i,
                                                  "0123456789012345",
                                                  16 ) ) );
    }
    sTestMessage[TEST_LARGE_MESSAGE_SIZE - 1] = '\0';

    unittestIdeSetupLog();

    ideLogEntry sLog(IDE_SERVER_0);
    ACT_ASSERT( IDE_SUCCESS == sLog.append(sTestMessage) );
    ACT_ASSERT( IDE_SUCCESS == sLog.write() );

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[TEST_LARGE_MESSAGE_SIZE];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, TEST_LARGE_MESSAGE_SIZE); /* ignore header */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpStdGetCString(&sFile, sLine, TEST_LARGE_MESSAGE_SIZE); /* ignore timestamp */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, TEST_LARGE_MESSAGE_SIZE);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, sTestMessage, TEST_LARGE_MESSAGE_SIZE) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
}

void unittestIdeTestLargeFormat()
{
    acp_char_t   sTestMessage[TEST_LARGE_MESSAGE_SIZE];
    acp_char_t   sFormat[TEST_LARGE_MESSAGE_SIZE];
    acp_rc_t     sRC;
    acp_uint32_t i;

    /* fill up the test message */
    for ( i = 0 ; i < TEST_LARGE_MESSAGE_SIZE - 16 ; i += 16 )
    {
        ACT_CHECK( ACP_RC_IS_SUCCESS( acpCStrCpy( sTestMessage + i,
                                                  TEST_LARGE_MESSAGE_SIZE - i,
                                                  "0123456789012345",
                                                  16 ) ) );
    }
    sTestMessage[TEST_LARGE_MESSAGE_SIZE - 1] = '\0';

    /* format string */
    acpMemSet( &sFormat, '\0', TEST_LARGE_MESSAGE_SIZE );
    for ( i = 0 ; i < TEST_LARGE_MESSAGE_SIZE - 16 ; i += 16 )
    {
        if ( i == 10 )
        {
            ACT_CHECK( ACP_RC_IS_SUCCESS( acpCStrCpy( sFormat + i,
                                                      TEST_LARGE_MESSAGE_SIZE - i,
                                                      "%02d2%s9%c%u",
                                                      16 ) ) );
        }
        else
        {
            ACT_CHECK( ACP_RC_IS_SUCCESS( acpCStrCpy( sFormat + i,
                                                      TEST_LARGE_MESSAGE_SIZE - i,
                                                      "0123456789012345",
                                                      16 ) ) );
        }
    }

    unittestIdeSetupLog();

    ideLogEntry sLog(IDE_SERVER_0);
    ACT_ASSERT( IDE_SUCCESS == sLog.appendFormat( sFormat,
                                                  1,
                                                  "345678",
                                                  0,
                                                  12345L ) );
    ACT_ASSERT( IDE_SUCCESS == sLog.write() );

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[TEST_LARGE_MESSAGE_SIZE];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, TEST_LARGE_MESSAGE_SIZE); /* ignore header */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpStdGetCString(&sFile, sLine, TEST_LARGE_MESSAGE_SIZE); /* ignore timestamp */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, TEST_LARGE_MESSAGE_SIZE);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, sTestMessage, TEST_LARGE_MESSAGE_SIZE) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
}

/*
 * Tests multiple formatted string output using ideLogEntry
 */
void unittestIdeTestMultilineFormat()
{
    const acp_char_t * const TEST_MESSAGES[] = {
        "Logging test from unittestIdeTestMultilineFormat()\n",
        "PI = 3.141592\n",
        "123456789012345\n"
    };

    const acp_char_t * const TEST_FORMATS[] = {
        "Logging test from %s\n",
        "PI = %f\n",
        "%lu\n"
    };

    acp_rc_t  sRC;

    unittestIdeSetupLog();

    ideLogEntry sLog(IDE_SERVER_0);
    for ( acp_sint32_t i = 0; i < 1000; i++ )
    {
        ACT_ASSERT( IDE_SUCCESS == sLog.appendFormat(TEST_FORMATS[0], "unittestIdeTestMultilineFormat()") );
        ACT_ASSERT( IDE_SUCCESS == sLog.appendFormat(TEST_FORMATS[1], 3.141592) );
        ACT_ASSERT( IDE_SUCCESS == sLog.appendFormat(TEST_FORMATS[2], 123456789012345L) );
    }
    ACT_ASSERT( IDE_SUCCESS == sLog.write() );

    unittestIdeTeardownLog();

    /* checking if the log entry was appended correctly */
    acp_std_file_t sFile;
    acp_char_t     sLine[256];

    sRC = acpStdOpen(&sFile, const_cast<acp_char_t*>(TEST_FILE_NAME), "r");
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore header */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    sRC = acpStdGetCString(&sFile, sLine, 256); /* ignore timestamp */
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGES[0], 256) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGES[1], 256) );

    sRC = acpStdGetCString(&sFile, sLine, 256);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
    ACT_ASSERT( 0 == acpCStrCmp(sLine, TEST_MESSAGES[2], 256) );

    sRC = acpStdClose(&sFile);
    ACT_CHECK( ACP_RC_IS_SUCCESS(sRC) );
}

int main()
{
    ACT_TEST_BEGIN();

    ACT_ASSERT( IDE_SUCCESS == idp::initialize(NULL, NULL) );
    ACT_ASSERT( IDE_SUCCESS == iduMemMgr::initializeStatic(IDU_CLIENT_TYPE) );
    ACT_ASSERT( IDE_SUCCESS == iduMutexMgr::initializeStatic(IDU_CLIENT_TYPE) );

    unittestIdeTestLogging();
    unittestIdeTestNoLoggingFlag();
    unittestIdeTestNotLogging();
    unittestIdeTestMultiline();
    unittestIdeTestLargeMessage();
    unittestIdeTestLargeFormat();
    unittestIdeTestMultilineFormat();

    ACT_CHECK( IDE_SUCCESS == iduMutexMgr::destroyStatic() );
    ACT_CHECK( IDE_SUCCESS == iduMemMgr::destroyStatic() );
    ACT_CHECK( IDE_SUCCESS == idp::destroy() );

    ACT_TEST_END();

    return 0;
}
