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
 * $Id: testAciMsgLog.c 10969 2010-04-29 09:04:59Z gurugio $
 ******************************************************************************/

#include <act.h>

#include <acpCStr.h>
#include <aciTypes.h>
#include <aciMsgLog.h>

#define ACI_TEST_LOG_FILENAME1 "acilog.log"
#define ACI_TEST_LOG_SIZE1     100
#define ACI_TEST_LOG_NUM1      1
#define ACI_TEST_STR1          "test\n"

void testLogBasic(void)
{
    ACI_RC   sRC = ACI_FAILURE;
    acp_rc_t sRet;
    ACP_STR_DECLARE_STATIC(sStr,ACI_TEST_LOG_SIZE1);

    acp_std_file_t *sLogFile;

    aci_msg_log_t sLog;
    acp_stat_t        sStat;

    sRC = aciMsgLogInit(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogCheckExist(&sLog);
    ACT_CHECK(sRC == ACI_FAILURE);

    sRC = aciMsgLogInitialize(&sLog,
                        1,
                        ACI_TEST_LOG_FILENAME1,
                        ACI_TEST_LOG_SIZE1,
                        ACI_TEST_LOG_NUM1);
    ACT_CHECK(sRC == ACI_SUCCESS);


    sRC = aciMsgLogOpen(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogBody(&sLog, ACI_TEST_STR1);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogFlush(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogClose(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);
 
    sRet = acpFileStatAtPath(ACI_TEST_LOG_FILENAME1,
                             &sStat, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRet));

    ACT_CHECK(sStat.mSize > 0);
    ACT_CHECK(sStat.mSize <= ACI_TEST_LOG_SIZE1);

    ACT_CHECK(ACP_RC_IS_SUCCESS(
                 acpStdOpen(&sLogFile,
                            ACI_TEST_LOG_FILENAME1,
                            ACP_STD_OPEN_READ)
                 ));

    ACP_STR_INIT_STATIC(sStr);

    /* Skip header */
    sRet = acpStdGetString(sLogFile, &sStr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRet));

    /* Read log string */
    sRet = acpStdGetString(sLogFile, &sStr);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRet));

    /* Compare log string */
    ACT_CHECK(acpCStrCmp(sStr.mString,
                            ACI_TEST_STR1,
                            4) == 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpStdClose(sLogFile)));

    sRC = aciMsgLogCheckExist(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogErase(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogCheckExist(&sLog);
    ACT_CHECK(sRC == ACI_FAILURE);

    sRC = aciMsgLogDestroy(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);
}

void testLogInitialize(void)
{
    ACI_RC        sRC = ACI_FAILURE;
    aci_msg_log_t sLog;
    acp_uint32_t  i = 0;
    acp_sint32_t  sLogFileNum = 0;

    sRC = aciMsgLogInit(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogInitialize(&sLog,
                               1,
                               ACI_TEST_LOG_FILENAME1,
                               ACI_TEST_LOG_SIZE1,
                               ACI_TEST_LOG_NUM1);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogOpen(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    /* Write more then ACI_TEST_LOG_SIZE1 */   
    for (i = 0; i < 256; i++)
    {
        sRC = aciMsgLogBody(&sLog, ACI_TEST_STR1);
        ACT_CHECK(sRC == ACI_SUCCESS);
    }

    ACT_CHECK(aciMsgLogReadFileNumber(&sLog, &sLogFileNum) 
              == ACI_SUCCESS);

    ACT_CHECK(sLogFileNum == ACI_TEST_LOG_NUM1);

    sRC = aciMsgLogClose(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogErase(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);

    sRC = aciMsgLogDestroy(&sLog);
    ACT_CHECK(sRC == ACI_SUCCESS);
}

void testTrcLog(void)
{

    aciLogOpen(1,
                ACI_SERVER,
                0);

    aciLogMessage(1,
                   ACI_SERVER,
                   0,
                   ACI_TEST_STR1);

     aciLogClose(1,
                 ACI_SERVER,
                 0);
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testLogBasic();

    testLogInitialize();

    testTrcLog();

    ACT_TEST_END();

    return 0;
}
