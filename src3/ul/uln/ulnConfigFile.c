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

#include <ulnConfigFile.h>
#include <ulnURL.h>
#include <ulnString.h>

ulnConfigFile    gULConfigFile;

//static function.
static void   ulnConfigFileTrim(acp_char_t* aBuffer);
static acp_char_t *ulnConfigFileSkipSpace(acp_char_t* aBuffer);
static ulnDataSource * ulnConfigFileRegisterDataSource(acp_char_t *aDataSourceName);
static ulnDataSource * ulnConfigFileGetDataSourceInternal(acp_char_t *aDataSourceName);


static acp_bool_t  ulConfigFileOpen()
{

    acp_char_t   sConfigFilePath[ULN_CONFIG_FILE_PATH_LEN];
    acp_char_t  *sEnvStr;
    acp_rc_t     sRet = ACP_RC_EEXIST;

    if (acpEnvGet("ALTIBASE_HOME", &sEnvStr) == ACP_RC_SUCCESS)
    {
        acpSnprintf(sConfigFilePath,
                    ULN_CONFIG_FILE_PATH_LEN,
                    "%s" ACI_DIRECTORY_SEPARATOR_STR_A "conf" ACI_DIRECTORY_SEPARATOR_STR_A "%s",
                    sEnvStr,
                    ULN_CONFIG_FILE_NAME);
        sRet = acpStdOpen(&gULConfigFile.mFile, sConfigFilePath, ACP_STD_OPEN_READ_TEXT);
    }

    if(ACP_RC_NOT_SUCCESS(sRet))
    {
        if (acpEnvGet("HOME", &sEnvStr) == ACP_RC_SUCCESS)
        {
            acpSnprintf(sConfigFilePath,
                        ULN_CONFIG_FILE_PATH_LEN,
                        "%s" ACI_DIRECTORY_SEPARATOR_STR_A "%s",
                        sEnvStr,
                        ULN_CONFIG_FILE_NAME);
            sRet = acpStdOpen(&gULConfigFile.mFile, sConfigFilePath, ACP_STD_OPEN_READ_TEXT);
        }
    }

    if(ACP_RC_NOT_SUCCESS(sRet))
    {
        acpSnprintf(sConfigFilePath,ULN_CONFIG_FILE_PATH_LEN,"%s",ULN_CONFIG_FILE_NAME);
        sRet = acpStdOpen(&gULConfigFile.mFile, sConfigFilePath, ACP_STD_OPEN_READ_TEXT);
    }

    if(ACP_RC_NOT_SUCCESS(sRet))
    {
        return ACP_FALSE;
    }
    else
    {
        return ACP_TRUE;
    }
}

static   void ulnConfigFileInitDataSourceHash()
{
    acp_uint32_t i ;

    for(i = 0 ; i < ULN_DATASOURCE_BUCKET_COUNT; i++)
    {
        acpListInit(&(gULConfigFile.mBucketTable[i].mChain));
    }//for
}

void ulnConfigFileLoad()
{
    acp_rc_t            sRC;
    acp_char_t         *sTrimmedBuffer;
    acp_char_t         *sDataSourceName =  NULL;
    acp_char_t         *sDataSourcePostfix;
    acp_char_t         *sCommentPrefix;
    ulnDataSource      *sDataSource = NULL;
    acp_char_t         *sKeyWordStr;
    acp_char_t         *sValueStr = NULL;
    ulnConnAttrID       sConnAttr;
    acp_char_t          sBuffer[ULN_CONFIG_READ_BUFF_SIZE];
    acp_bool_t          sIsEOF = ACP_FALSE;

    //initialize DataSource Hash
    ulnConfigFileInitDataSourceHash();
    if( ulConfigFileOpen() == ACP_TRUE)
    {

        while(1)
        {
            sBuffer[0] = '\0';
            sRC = acpStdGetCString(&gULConfigFile.mFile,
                                   sBuffer,
                                   ULN_CONFIG_READ_BUFF_SIZE);
            if(ACP_RC_NOT_SUCCESS(sRC))
            {
                break;
            }
            else
            {
                acpStdIsEOF(&gULConfigFile.mFile, &sIsEOF);
                if((sIsEOF == ACP_TRUE) && (sBuffer[0] == '\0'))
                {
                    break;
                }
                sTrimmedBuffer = ulnConfigFileSkipSpace(sBuffer);
                //skip empty line
                if( (acpCStrLen(sTrimmedBuffer, ACP_SINT32_MAX) == 0) || (sTrimmedBuffer[0] == ULN_CONFIG_COMMENT_CHAR ) ||
                    (sTrimmedBuffer[0] == '\n' )   ||  (sTrimmedBuffer[0] == '\r' ))
                {
                    continue;
                }
                if(sTrimmedBuffer[0] == ULN_DATASOURCE_PREFIX)
                {
                    sDataSourcePostfix = ulnStrRchr(sTrimmedBuffer,
                                                    ULN_DATASOURCE_POSTFIX);

                    if(sDataSourcePostfix == NULL)
                    {
                        continue;
                    }
                    else
                    {
                        // new DataSource  [ DataSource ]  begin...
                        *sDataSourcePostfix = ULN_EOS;
                        sDataSourceName = ulnConfigFileSkipSpace((acp_char_t*)(sTrimmedBuffer+1));
                        ulnConfigFileTrim(sDataSourceName);
                        if(ulnConfigFileGetDataSourceInternal(sDataSourceName) == NULL )
                        {
                            //해당 DataSource 가 등록되어 있지않음.
                            sDataSource = ulnConfigFileRegisterDataSource(sDataSourceName);
                            if(sDataSource == NULL)
                            {
                                // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
                                // 고려하지 않음.
                                continue;
                            }
                            sDataSourceName = sDataSource->mDataSourceName;
                        }
                        else
                        {
                            //해당 DataSource 가 등록되어 있음.
                            sDataSourceName = NULL;
                        }//else
                    }//else
                }//if
                else
                {
                    if(sDataSourceName == NULL)
                    {
                        // wrong state  skip
                        continue;
                    }
                    // keyword.
                    sKeyWordStr = ulnStrSep(&sTrimmedBuffer, "=");
                    sKeyWordStr = urlCleanKeyWord(sKeyWordStr);
                    // value
                    sValueStr  =  urlCleanKeyWord(sTrimmedBuffer);

                    //BUG-28623 [CodeSonar]Null Pointer Dereference
                    if ( sValueStr == NULL )
                    {
                        continue;
                    }

                    ulnConfigFileTrim(sValueStr);

                    // skip  tailing comment
                    // exmaple , MEM_DB_DIR          = ?/dbs # Memory DB Directory
                    sCommentPrefix = ulnStrRchr(sValueStr,ULN_CONFIG_COMMENT_CHAR);
                    if(sCommentPrefix != NULL)
                    {
                        *sCommentPrefix = ULN_EOS;
                    }
                    sConnAttr = ulnGetConnAttrIDfromKEYWORD(sKeyWordStr, acpCStrLen(sKeyWordStr, ACP_SINT32_MAX));
                    sDataSource = ulnConfigFileGetDataSourceInternal(sDataSourceName);
                    // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
                    // 고려하지 않음.
                    if(sDataSource == NULL)
                    {
                        continue;
                    }
                    if(sConnAttr  < ULN_CONN_ATTR_MAX)
                    {
                        // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
                        // 고려하지 않음.
                        // 메모리 부족일때 , Connection attribute loss를 허용한다.
                        (void)ulnDataSourceAddConnAttr(sDataSource,sConnAttr,sValueStr);
                    }
                    else
                    {
                        continue;
                    }//else
                }//else
            }//else
        }//while

        ACE_ASSERT(acpStdClose(&gULConfigFile.mFile) == ACP_RC_SUCCESS);
    }// ulConfigFileOpen
    else
    {
        //nothing to do
    }
}


static  ulnDataSource * ulnConfigFileRegisterDataSource(acp_char_t *aDataSourceName)
{
    ulnDataSource *sDataSource = NULL;
    acp_uint32_t   sStage = 0;
    acp_size_t     sDataSourceNameLen;

    acp_uint32_t   sHashKeyVal = aclHashHashCString((const void*)aDataSourceName,
                                                    acpCStrLen(aDataSourceName, ACP_SINT32_MAX));

    acp_uint32_t   sBucket = (sHashKeyVal % ULN_DATASOURCE_BUCKET_COUNT);

    // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
    // 고려하지 않음.
    ACI_TEST(acpMemAlloc((void**)&sDataSource, ACI_SIZEOF(ulnDataSource)) != ACP_RC_SUCCESS);
    sStage = 1;

    sDataSourceNameLen = acpCStrLen(aDataSourceName, ACP_SINT32_MAX);

    // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
    // 고려하지 않음.
    ACI_TEST(acpMemAlloc((void**)&sDataSource->mDataSourceName, sDataSourceNameLen + 1)
             != ACP_RC_SUCCESS);

    acpCStrCpy(sDataSource->mDataSourceName,
               sDataSourceNameLen + 1,
               aDataSourceName,
               sDataSourceNameLen);

    acpListInitObj(&sDataSource->mChain,sDataSource);
    acpListInit(&sDataSource->mConnAttrList);

    acpListAppendNode(&(gULConfigFile.mBucketTable[sBucket].mChain),&sDataSource->mChain);

    return sDataSource;

    ACI_EXCEPTION_END;
    // fix BUG-25971 UL-FailOver추가한 함수에서 메모리 한계상황을
    // 고려하지 않음.
    switch (sStage )
    {
        case 1:
            (void)acpMemFree(sDataSource);
            break;
    }
    return  (ulnDataSource*)NULL;
}

//skip space char.
static acp_char_t *ulnConfigFileSkipSpace(acp_char_t* aBuffer)
{
    acp_uint32_t  i;
    acp_char_t   *sTrimBuffer = aBuffer;

    for( i = 0 ;aBuffer[i] != ULN_EOS ; i++)
    {
        if( aBuffer[i] == ' ')
        {
            sTrimBuffer++;
        }
        else
        {
            break;
        }
    }//for
    return sTrimBuffer;
}

static void ulnConfigFileTrim(acp_char_t* aBuffer)
{
    acp_uint32_t i;

    for( i = 0 ;aBuffer[i] != ULN_EOS ; i++)
    {
        if( (aBuffer[i] == ' ') || (aBuffer[i] == '\r' ) || (aBuffer[i] == '\n' )  )
        {
            aBuffer[i] = ULN_EOS;
            break;
        }
        else
        {
        }
    }//for
}

static ulnDataSource *ulnConfigFileGetDataSourceInternal(acp_char_t *aDataSourceName)
{

    ulnDataSource   *sDataSource = NULL;
    acp_list_t      *sChain;
    acp_list_node_t *sIterator;

    acp_uint32_t     sHashKeyVal = aclHashHashCString((const void*)aDataSourceName,
                                                      acpCStrLen(aDataSourceName, ACP_SINT32_MAX));
    acp_uint32_t     sBucket = (sHashKeyVal % ULN_DATASOURCE_BUCKET_COUNT);
    sChain    = &(gULConfigFile.mBucketTable[sBucket].mChain);
    ACP_LIST_ITERATE(sChain, sIterator)
    {
        sDataSource = (ulnDataSource*)sIterator->mObj;
        if(acpCStrCmp(sDataSource->mDataSourceName,
                      aDataSourceName,
                      ACP_MAX(
                          acpCStrLen(sDataSource->mDataSourceName, ACP_SINT32_MAX),
                          acpCStrLen(aDataSourceName, ACP_SINT32_MAX))) == 0)
        {
            break;
        }
        else
        {
            sDataSource = NULL;
        }
    }
    return sDataSource;
}

ulnDataSource *ulnConfigFileGetDataSource(acp_char_t *aDataSourceName)
{

    ulnDataSource   *sDataSource = NULL;
    acp_list_t      *sChain;
    acp_list_node_t *sIterator;
    acp_uint32_t     sHashKeyVal;
    acp_uint32_t     sBucket;


    ACI_TEST( aDataSourceName == NULL);

    sHashKeyVal = aclHashHashCString((const void*)aDataSourceName,
                                     acpCStrLen(aDataSourceName, ACP_SINT32_MAX));

    sBucket = (sHashKeyVal % ULN_DATASOURCE_BUCKET_COUNT);
    sChain    = &(gULConfigFile.mBucketTable[sBucket].mChain);

    ACP_LIST_ITERATE(sChain, sIterator)
    {
        sDataSource = (ulnDataSource*)sIterator->mObj;
        if(acpCStrCmp(sDataSource->mDataSourceName,
                      aDataSourceName,
                      ACP_MAX(acpCStrLen(sDataSource->mDataSourceName, ACP_SINT32_MAX),
                              acpCStrLen(aDataSourceName, ACP_SINT32_MAX))) == 0)
        {
            break;
        }
        else
        {
            sDataSource = NULL;
        }
    }

    return sDataSource;

    ACI_EXCEPTION_END;

    return (ulnDataSource *)NULL;
}


void ulnConfigFileDump()
{
    acp_uint32_t     i;
    acp_list_t      *sChain;
    acp_list_node_t *sIterator;
    ulnDataSource *sDataSource = NULL;

    for(i = 0 ; i < ULN_DATASOURCE_BUCKET_COUNT; i++)
    {
       sChain = &(gULConfigFile.mBucketTable[i].mChain);

       ACP_LIST_ITERATE(sChain, sIterator)
       {
           sDataSource = (ulnDataSource*)sIterator->mObj;
           ulnDataSourceDumpAttributes(sDataSource);
       }
    }//for
}

/* BUG-28793 Add function to get a DataSource Contents using a Datasource Name */
void ulnConfigFileDumpFromName(acp_char_t* aDataSourceName)
{
    ulnDataSource   *sDataSource = NULL;
    acp_list_t      *sChain;
    acp_list_node_t *sIterator;
    acp_uint32_t     sHashKeyVal;
    acp_uint32_t     sBucket;

    if (aDataSourceName != NULL)
    {
        sHashKeyVal = aclHashHashCString((const void*)aDataSourceName,
                                            acpCStrLen(aDataSourceName, ACP_SINT32_MAX));

        sBucket = (sHashKeyVal % ULN_DATASOURCE_BUCKET_COUNT);
        sChain  = &(gULConfigFile.mBucketTable[sBucket].mChain);

        ACP_LIST_ITERATE(sChain, sIterator)
        {
            sDataSource = (ulnDataSource*)sIterator->mObj;
            if(acpCStrCmp(sDataSource->mDataSourceName,
                          aDataSourceName,
                          ACP_MAX(
                              acpCStrLen(sDataSource->mDataSourceName, ACP_SINT32_MAX),
                              acpCStrLen(aDataSourceName, ACP_SINT32_MAX))) == 0)
            {
                ulnDataSourceDumpAttributes(sDataSource);
                break;
            }
        }
    }
    else
    {
        //Nothing to do.
    }
}

void ulnConfigFileUnLoad()
{
    acp_uint32_t     i;
    acp_list_t      *sChain;
    acp_list_node_t *sIterator;
    acp_list_node_t *sNodeNext;
    ulnDataSource *sDataSource = NULL;

    for(i = 0 ; i < ULN_DATASOURCE_BUCKET_COUNT; i++)
    {
       sChain = &(gULConfigFile.mBucketTable[i].mChain);

       ACP_LIST_ITERATE_SAFE(sChain, sIterator,sNodeNext)
       {
           sDataSource = (ulnDataSource*)sIterator->mObj;
           acpListDeleteNode(&(sDataSource->mChain));
           ulnDataSourceDestroyAttributes(sDataSource);
           acpMemFree(sDataSource->mDataSourceName);
           acpMemFree(sDataSource);
       }
    }//for
}
