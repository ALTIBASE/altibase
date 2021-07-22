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
 * $Id: testAciConv.c 11125 2010-05-25 06:21:45Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <aciConv.h>

#define TEST_ACI_MAX_FILE_SIZE 4096

typedef enum testConvertNodeNum
{
    TEST_NODE_ASCII = 0,

    TEST_NODE_UTF8_KR,
    TEST_NODE_UTF8_JP,
    TEST_NODE_UTF8_CI_BIG5,
    TEST_NODE_UTF8_CI_GB231280,
    TEST_NODE_UTF8_CI_MS936,

    TEST_NODE_UTF16_KR,
    TEST_NODE_UTF16_JP,
    TEST_NODE_UTF16_CI_BIG5,
    TEST_NODE_UTF16_CI_GB231280,
    TEST_NODE_UTF16_CI_MS936,

    TEST_NODE_KSC5601,
    TEST_NODE_MS949,

    TEST_NODE_EUCJP,
    TEST_NODE_SHIFTJIS,

    TEST_NODE_BIG5,
    TEST_NODE_GB231280,
    TEST_NODE_MS936

} testConvNodeNum;

typedef struct testConvNode
{
    aciConvCharSetList mCharSet;
    const acp_char_t *mFileName;
} testConvNode;

testConvNode gTestConvNodeTable[] =
{
    {ACICONV_ASCII_ID, "ascii.txt"},

    {ACICONV_UTF8_ID, "utf8_kr.txt"},
    {ACICONV_UTF8_ID, "utf8_jp.txt"},
    {ACICONV_UTF8_ID, "utf8_ci_big5.txt"},
    {ACICONV_UTF8_ID, "utf8_ci_gb231280.txt"},
    {ACICONV_UTF8_ID, "utf8_ci_ms936.txt"},

    {ACICONV_UTF16_ID, "utf16_be_kr.txt"},
    {ACICONV_UTF16_ID, "utf16_be_jp.txt"},
    {ACICONV_UTF16_ID, "utf16_be_ci_big5.txt"},
    {ACICONV_UTF16_ID, "utf16_be_ci_gb231280.txt"},
    {ACICONV_UTF16_ID, "utf16_be_ci_ms936.txt"},

    
    {ACICONV_KSC5601_ID, "ksc5601.txt"},
    {ACICONV_MS949_ID, "ms949.txt"},

    {ACICONV_EUCJP_ID, "eucjp.txt"},
    {ACICONV_SHIFTJIS_ID, "shiftjis.txt"},

    {ACICONV_BIG5_ID, "big5.txt"},
    {ACICONV_GB231280_ID, "gb231280.txt"},
    {ACICONV_MS936_ID, "ms936.txt"}
};

typedef struct testConvSet
{
    acp_sint32_t mSource;
    acp_sint32_t mTargetList[ACICONV_MAX_CHARSET_ID];
} testConvSet;

testConvSet gTestConvList[] =
{
    /* KOREAN */
    {
        TEST_NODE_UTF8_KR,
        {TEST_NODE_UTF16_KR, TEST_NODE_KSC5601, TEST_NODE_MS949, -1}
    },
    {
        TEST_NODE_UTF16_KR,
        {TEST_NODE_UTF8_KR, TEST_NODE_KSC5601, TEST_NODE_MS949, -1}
    },
    {
        TEST_NODE_KSC5601,
        {TEST_NODE_UTF8_KR, TEST_NODE_UTF16_KR, TEST_NODE_MS949, -1}
    },
    {
        TEST_NODE_MS949,
        {TEST_NODE_UTF8_KR, TEST_NODE_UTF16_KR, TEST_NODE_KSC5601, -1}
    },

    /* JAPANESE */
    {
        TEST_NODE_UTF8_JP,
        {TEST_NODE_UTF16_JP, TEST_NODE_EUCJP, TEST_NODE_SHIFTJIS, -1}
    },
    {
        TEST_NODE_UTF16_JP,
        {TEST_NODE_UTF8_JP, TEST_NODE_EUCJP, TEST_NODE_SHIFTJIS, -1}
    },
    {
        TEST_NODE_EUCJP,
        {TEST_NODE_UTF8_JP, TEST_NODE_UTF16_JP, TEST_NODE_SHIFTJIS, -1}
    },
    {
        TEST_NODE_SHIFTJIS,
        {TEST_NODE_UTF8_JP, TEST_NODE_UTF16_JP, TEST_NODE_EUCJP, -1}
    },

    /* BIG5 and GB231280 are completly different character-set,
       so that converting between them cannot be tested. */
    
    /* CHINESE-BIG5 */
    {
        TEST_NODE_UTF8_CI_BIG5,
        {TEST_NODE_UTF16_CI_BIG5, TEST_NODE_BIG5, -1}
    },
    {
        TEST_NODE_UTF16_CI_BIG5,
        {TEST_NODE_UTF8_CI_BIG5, TEST_NODE_BIG5, -1}
    },
    {
        TEST_NODE_BIG5,
        {TEST_NODE_UTF8_CI_BIG5, TEST_NODE_UTF16_CI_BIG5, -1}
    },

    /* CHINESE-GB231280 */
    {
        TEST_NODE_UTF8_CI_GB231280,
        {TEST_NODE_UTF16_CI_GB231280, TEST_NODE_GB231280, -1}
    },
    {
        TEST_NODE_UTF16_CI_GB231280,
        {TEST_NODE_UTF8_CI_GB231280, TEST_NODE_GB231280, -1}
    },
    {
        TEST_NODE_GB231280,
        {TEST_NODE_UTF8_CI_GB231280, TEST_NODE_UTF16_CI_GB231280, -1}
    },

    /* CHINESE-MS936 */
    {
        TEST_NODE_UTF8_CI_MS936,
        {TEST_NODE_UTF16_CI_MS936, TEST_NODE_MS936, -1}
    },
    {
        TEST_NODE_UTF16_CI_MS936,
        {TEST_NODE_UTF8_CI_MS936, TEST_NODE_MS936, -1}
    },
    {
        TEST_NODE_MS936,
        {TEST_NODE_UTF8_CI_MS936, TEST_NODE_UTF16_CI_MS936, -1}
    },

    /* SENTINEL */
    {
        -1,
        {-1,}
    }
};


acp_rc_t testAciConvReadFile(const acp_char_t *aName,
                             acp_char_t *aBuf,
                             acp_size_t aBufSize,
                             acp_size_t *aFileLen)
{
    acp_rc_t sRC;
    acp_std_file_t sFile;
    acp_size_t sRead;
    
    sRC = acpStdOpen(&sFile, (acp_char_t *)aName, "r");
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("Fail to open file,%s",aName));
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), OPEN_FAIL);
    
    sRC = acpStdRead(&sFile, aBuf, sizeof(acp_char_t),
                     aBufSize, &sRead);

    ACP_TEST_RAISE(ACP_RC_NOT_EOF(sRC), READ_FAIL);
    ACT_CHECK_DESC(sRead < aBufSize,
                   ("Read (%d)-bytes, buffer size (%d) is too small",
                    (acp_sint32_t)sRead, (acp_sint32_t)aBufSize));
    
    aBuf[sRead] = '\0';
    *aFileLen = sRead;

    sRC = acpStdClose(&sFile);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CLOSE_FAIL);

    return ACP_RC_SUCCESS;
    
    ACP_EXCEPTION(OPEN_FAIL)
    {
    }

    ACP_EXCEPTION(READ_FAIL)
    {
        (void)acpStdClose(&sFile);
    }

    ACP_EXCEPTION(CLOSE_FAIL)
    {

    }

    ACP_EXCEPTION_END;
    return sRC;
}
    
/* ALINT:C13-DISABLE */
acp_size_t testAciConvGetNextOffset(aciConvCharSetList aCharSet,
                                    acp_char_t *aBuf)
{
    acp_size_t sRet;
    
    switch (aCharSet)
    {
        case ACICONV_ASCII_ID:

            sRet = 1;
            break;
        case ACICONV_UTF8_ID:
            if ((*aBuf & 0x80) == 0)
            {
                sRet = 1;
            }
            else if ((*(aBuf + 1) & 0xc0) == 0x80
                     && (*aBuf & 0xe0) == 0xc0)
            {
                sRet = 2;
            }
            else if ((*(aBuf + 2) & 0xc0) == 0x80
                     && (*(aBuf + 1) & 0xc0) == 0x80
                     && (*aBuf & 0xe0) == 0xe0)
            {
                sRet = 3;
            }
            else if ((*(aBuf + 3) & 0xc0) == 0x80
                     && (*(aBuf + 2) & 0xc0) == 0x80
                     && (*(aBuf + 1) & 0xc0) == 0x80
                     && (*aBuf & 0xf8) == 0xf0)
            {
                sRet = 4;
            }
            else
            {
                sRet = 0;
            }
            break;
        case ACICONV_UTF16_ID:

            sRet = 2;
            break;
        case ACICONV_KSC5601_ID:
        case ACICONV_MS949_ID:
        case ACICONV_SHIFTJIS_ID:
        case ACICONV_EUCJP_ID:
            if ((*aBuf & 0x80) != 0)
            {
                sRet = 2;
            }
            else
            {
                sRet = 1;
            }
            break;
        case ACICONV_BIG5_ID:
            if ((unsigned char)*aBuf > 0xa0 &&
                (unsigned char)*(aBuf + 1) >= 0x40)
            {
                sRet = 2;
            }
            else
            {
                sRet = 1;
            }
            break;
        case ACICONV_GB231280_ID:
            if ((unsigned char)*aBuf >= 0xa1 &&
                (unsigned char)*aBuf <= 0xf7)
            {
                sRet = 2;
            }
            else
            {
                sRet = 1;
            }
            break;
        case ACICONV_MS936_ID:
            if ((unsigned char)*aBuf >= 0x81 &&
                (unsigned char)*aBuf < 0xff)
            {
                sRet = 2;
            }
            else
            {
                sRet = 1;
            }
            break;
        default:
            sRet = 0;
    }
            
    return sRet;       
}
/* ALINT:C13-ENABLE */

void testAciConv(void)
{
    acp_rc_t sRC;

    acp_sint32_t sTestSetIndex = 0;
    acp_sint32_t sTargetNodeIndex = 0;
    
    acp_char_t sSrcBuf[TEST_ACI_MAX_FILE_SIZE];
    acp_char_t sTargetBuf[TEST_ACI_MAX_FILE_SIZE];
    acp_char_t sTmpBuf[TEST_ACI_MAX_FILE_SIZE];

    acp_size_t sSrcLen = 0;
    acp_size_t sTargetLen = 0;

    testConvNode *sSrcNode;
    testConvNode *sTargetNode;

    acp_size_t sSrcBufIndex;
    acp_size_t sTmpBufIndex;
    acp_sint32_t sTmpRemain;


    while (gTestConvList[sTestSetIndex].mSource != -1)
    {
        acpMemSet(sSrcBuf, 0, TEST_ACI_MAX_FILE_SIZE);
        acpMemSet(sTargetBuf, 0, TEST_ACI_MAX_FILE_SIZE);
        acpMemSet(sTmpBuf, 0, TEST_ACI_MAX_FILE_SIZE);
        
        sSrcNode = &gTestConvNodeTable[gTestConvList[sTestSetIndex].mSource];

        sRC = testAciConvReadFile(sSrcNode->mFileName,
                                  sSrcBuf,
                                  TEST_ACI_MAX_FILE_SIZE,
                                  &sSrcLen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CONV_FAIL);

        while (gTestConvList[sTestSetIndex].mTargetList[sTargetNodeIndex] != -1)
        {
            sTargetNode =
                &gTestConvNodeTable[
                    gTestConvList[
                        sTestSetIndex].mTargetList[sTargetNodeIndex]];

            sTargetNodeIndex++;
    
            sRC = testAciConvReadFile(sTargetNode->mFileName,
                                      sTargetBuf,
                                      TEST_ACI_MAX_FILE_SIZE,
                                      &sTargetLen);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CONV_FAIL);
            
            sTmpRemain = TEST_ACI_MAX_FILE_SIZE;
            sSrcBufIndex = 0;
            sTmpBufIndex = 0;

            while (sSrcBufIndex <= sSrcLen)
            {
                acp_sint32_t sOldRemain = sTmpRemain;

                sRC = aciConvConvertCharSet(sSrcNode->mCharSet,
                                            sTargetNode->mCharSet,
                                            (void *)&sSrcBuf[sSrcBufIndex],
                                            (acp_sint32_t)(sSrcLen -
                                                           sSrcBufIndex),
                                            (void *)&sTmpBuf[sTmpBufIndex],
                                            &sTmpRemain,
                                            -1);
                ACT_CHECK(sRC == ACI_SUCCESS);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CONV_FAIL);
                
                sTmpBufIndex += (sOldRemain - sTmpRemain);
                sSrcBufIndex += testAciConvGetNextOffset(sSrcNode->mCharSet,
                                                         &sSrcBuf[
                                                             sSrcBufIndex]);
            }

            ACT_CHECK_DESC(acpCStrCmp(sTargetBuf, sTmpBuf, sTargetLen) == 0,
                           ("Fail to convert from [%s] to [%s]",
                            sSrcNode->mFileName, sTargetNode->mFileName));
        }
        sTestSetIndex++;
        sTargetNodeIndex = 0;
    }

    return;

    ACP_EXCEPTION(CONV_FAIL)
    {
    }

    ACP_EXCEPTION_END;
    return;
}

void testAciConv2(void)
{
    acp_rc_t sRC;

    acp_sint32_t sTestSetIndex = 0;
    acp_sint32_t sTargetNodeIndex = 0;
    
    acp_char_t sSrcBuf[TEST_ACI_MAX_FILE_SIZE];
    acp_char_t sTargetBuf[TEST_ACI_MAX_FILE_SIZE];
    acp_char_t sTmpBuf[TEST_ACI_MAX_FILE_SIZE];

    acp_size_t sSrcLen = 0;
    acp_size_t sTargetLen = 0;

    testConvNode *sSrcNode;
    testConvNode *sTargetNode;

    acp_size_t sSrcBufIndex;
    acp_size_t sTmpBufIndex;
    acp_sint32_t sSrcRemain;
    acp_sint32_t sTmpRemain;


    while (gTestConvList[sTestSetIndex].mSource != -1)
    {
        acpMemSet(sSrcBuf, 0, TEST_ACI_MAX_FILE_SIZE);
        acpMemSet(sTargetBuf, 0, TEST_ACI_MAX_FILE_SIZE);
        acpMemSet(sTmpBuf, 0, TEST_ACI_MAX_FILE_SIZE);
        
        sSrcNode = &gTestConvNodeTable[gTestConvList[sTestSetIndex].mSource];

        sRC = testAciConvReadFile(sSrcNode->mFileName,
                                  sSrcBuf,
                                  TEST_ACI_MAX_FILE_SIZE,
                                  &sSrcLen);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CONV_FAIL);

        while (gTestConvList[sTestSetIndex].mTargetList[sTargetNodeIndex] != -1)
        {
            sTargetNode =
                &gTestConvNodeTable[
                    gTestConvList[
                        sTestSetIndex].mTargetList[sTargetNodeIndex]];

            sTargetNodeIndex++;
    
            sRC = testAciConvReadFile(sTargetNode->mFileName,
                                      sTargetBuf,
                                      TEST_ACI_MAX_FILE_SIZE,
                                      &sTargetLen);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CONV_FAIL);
            
            sSrcRemain = (acp_sint32_t) sSrcLen;
            sTmpRemain = TEST_ACI_MAX_FILE_SIZE;
            sSrcBufIndex = 0;
            sTmpBufIndex = 0;

            while (sSrcBufIndex <= sSrcLen)
            {
                acp_sint32_t sOldRemain = sTmpRemain;
                acp_sint32_t sOldSrcRemain = sSrcRemain;

                sRC = aciConvConvertCharSet2(sSrcNode->mCharSet,
                                             sTargetNode->mCharSet,
                                             (void *)&sSrcBuf[sSrcBufIndex],
                                             &sSrcRemain,
                                             (void *)&sTmpBuf[sTmpBufIndex],
                                             &sTmpRemain,
                                             -1);
                ACT_CHECK(sRC == ACI_SUCCESS);
                ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CONV_FAIL);

                sTmpBufIndex += (sOldRemain - sTmpRemain);
                sSrcBufIndex += (sOldSrcRemain - sSrcRemain);
            }


            ACT_CHECK_DESC(acpCStrCmp(sTargetBuf, sTmpBuf, sTargetLen) == 0,
                           ("Fail to convert from [%s] to [%s]",
                            sSrcNode->mFileName, sTargetNode->mFileName));
        }
        sTestSetIndex++;
        sTargetNodeIndex = 0;
    }

    return;

    ACP_EXCEPTION(CONV_FAIL)
    {
    }

    ACP_EXCEPTION_END;
    return;
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testAciConv();

    /* BUG-34225 Unit-tests aciConvConvertCharSet2() designed to
     * improve performance by removing the need to call nextCharPtr()
     * functions */
    testAciConv2();

    ACT_TEST_END();

    return 0;
}

