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

#include <uln.h>
#include <ulnPrivate.h>

#define ULN_PROPERTIES_ENV_HOME        "ALTIBASE_HOME"
#define ULN_PROPERTIES_ENV_UNIXPATH    "ALTIBASE_UNIXDOMAIN_FILEPATH"
#define ULN_PROPERTIES_ENV_IPCPATH     "ALTIBASE_IPC_FILEPATH"

/* PROJ-2616 */
#define ULN_PROPERTIES_ENV_IPCDAPATH                  "ALTIBASE_IPCDA_FILEPATH"
#define ULN_PROPERTIES_ENV_IPCDACLIENTSLEEPTIME       "ALTIBASE_IPCDA_CLIENT_SLEEP_TIME"
#define ULN_PROPERTIES_ENV_IPCDACLIENTEXPIRECOUNT     "ALTIBASE_IPCDA_CLIENT_EXPIRE_COUNT"
#define ULN_PROPERTIES_ENV_IPCDACLIENTMESSAGEQTIMEOUT "ALTIBASE_IPCDA_CLIENT_MESSAGEQ_TIMEOUT"

#define ULN_PROPERTIES_DIR_CONF        "conf"
#define ULN_PROPERTIES_DIR_TRC         "trc"
#define ULN_PROPERTIES_FILE_UNIX       "cm-unix"
#define ULN_PROPERTIES_FILE_IPC        "cm-ipc"
#define ULN_PROPERTIES_FILE_IPCDA      "cm-ipcda"
#define ULN_PROPERTIES_FILE_PROPERTIES "altibase.properties"

enum
{
    HOME_PATH                     =   0,
    CONF_FILEPATH                 =   1,
    UNIXDOMAIN_FILEPATH           = 100,
    IPC_FILEPATH                  = 101,
    IPCDA_FILEPATH                = 104,
    IPCDA_CLIENT_SLEEP_TIME       = 105,
    IPCDA_CLIENT_EXPIRE_COUNT     = 106,
    IPCDA_CLIENT_MESSAGEQ_TIMEOUT = 107
};

/**
 *  Declare Inside Function
 */
static void ulnPropertiesExpandValuesStr(ulnProperties *aProperties,
                                         acp_str_t     *aDest,
                                         acp_str_t     *aSrc);

static acp_sint32_t ulnPropertiesCallbackString(acp_sint32_t    aDepth,
                                                acl_conf_key_t *aKey,
                                                acp_sint32_t    aLineNumber,
                                                void           *aValue,
                                                void           *aContext);

static acp_sint32_t ulnPropertiesCallbackInt32(
                                                acp_sint32_t    aDepth,
                                                acl_conf_key_t *aKey,
                                                acp_sint32_t    aLineNumber,
                                                void            *aValue,
                                                void            *aContext);

static void ulnPropertiesSetDefault(ulnProperties *aProperties,
                                    acp_char_t    *aHomepath);

static void ulnPropertiesSetStringAsEnv(ulnProperties *aProperties,
                                        acp_str_t     *aProperty,
                                        acp_char_t    *aPropertyName,
                                        acp_bool_t     aNeedExpansion);

static void ulnPropertiesSetIntegerAsEnv(ulnProperties *aProperties,
                                  acp_uint32_t  *aProperty,
                                  acp_char_t    *aPropertyName);

/**
 *  gUlnPropertiesConfDef
 *
 *  읽을 프로퍼티 정의 구조체
 */
static acl_conf_def_t gUlnPropertiesConfDef[] =
{
    ACL_CONF_DEF_STRING("UNIXDOMAIN_FILEPATH",
                        UNIXDOMAIN_FILEPATH,
                        "",
                        1,
                        ulnPropertiesCallbackString),

    ACL_CONF_DEF_STRING("IPC_FILEPATH",
                        IPC_FILEPATH,
                        "",
                        1,
                        ulnPropertiesCallbackString),

    ACL_CONF_DEF_STRING("IPCDA_FILEPATH",
                        IPCDA_FILEPATH,
                        "",
                        1,
                        ulnPropertiesCallbackString),
    
    ACL_CONF_DEF_UINT32("IPCDA_CLIENT_SLEEP_TIME",
                        IPCDA_CLIENT_SLEEP_TIME,
                        400,
                        0,
                        1000,
                        0,
                        ulnPropertiesCallbackInt32),

    ACL_CONF_DEF_UINT32("IPCDA_CLIENT_EXPIRE_COUNT",
                        IPCDA_CLIENT_EXPIRE_COUNT,
                        0,
                        0,
                        1000000,
                        0,
                        ulnPropertiesCallbackInt32),
    
    ACL_CONF_DEF_UINT32("IPCDA_CLIENT_MESSAGEQ_TIMEOUT",
                        IPCDA_CLIENT_MESSAGEQ_TIMEOUT,
                        100,
                        1,
                        65535,
                        0,
                        ulnPropertiesCallbackInt32),
    ACL_CONF_DEF_END()
};

/**
 *  ulnPropertiesCreate
 */
void ulnPropertiesCreate(ulnProperties *aProperties)
{
    acp_char_t *sHomepath = NULL;

    ACI_TEST(aProperties == NULL);

    ACP_STR_INIT_DYNAMIC(aProperties->mHomepath,           128, 128);
    ACP_STR_INIT_DYNAMIC(aProperties->mConfFilepath,       128, 128);
    ACP_STR_INIT_DYNAMIC(aProperties->mUnixdomainFilepath, 128, 128);
    ACP_STR_INIT_DYNAMIC(aProperties->mIpcFilepath,        128, 128);
    ACP_STR_INIT_DYNAMIC(aProperties->mIPCDAFilepath,      128, 128);

    if (acpEnvGet(ULN_PROPERTIES_ENV_HOME, &sHomepath) == ACP_RC_SUCCESS)
    {
        if (sHomepath[0] != '\0')
        {
            ulnPropertiesSetDefault(aProperties, sHomepath);
        }
        else
        {
            /* Nothing */
        }

        /* 프로퍼티 파일이 없을수도 있기 때문에 리턴값 체크는 생략한다. */
        aclConfLoad(&aProperties->mConfFilepath,
                    gUlnPropertiesConfDef,
                    NULL,
                    NULL,
                    NULL,
                    (void *)aProperties);
    }
    else
    {
        /* Nothing */
    }

    /* 환경 변수가 있으면 읽는다 */
    ulnPropertiesSetStringAsEnv(aProperties,
                                &aProperties->mUnixdomainFilepath,
                                ULN_PROPERTIES_ENV_UNIXPATH,
                                ACP_TRUE);

    ulnPropertiesSetStringAsEnv(aProperties,
                                &aProperties->mIpcFilepath,
                                ULN_PROPERTIES_ENV_IPCPATH,
                                ACP_TRUE);

    ulnPropertiesSetStringAsEnv(aProperties,
                                &aProperties->mIPCDAFilepath,
                                ULN_PROPERTIES_ENV_IPCDAPATH,
                                ACP_TRUE);
    
    /* ClientExpireCount를 읽어 들인다. */
    ulnPropertiesSetIntegerAsEnv(aProperties,
                                 &aProperties->mIpcDaClientExpireCount,
                                 ULN_PROPERTIES_ENV_IPCDACLIENTEXPIRECOUNT);

    /* mIpcDaClientSleepTime을 읽어 들인다. */

    ulnPropertiesSetIntegerAsEnv(aProperties,
                                 &aProperties->mIpcDaClientSleepTime,
                                 ULN_PROPERTIES_ENV_IPCDACLIENTSLEEPTIME);

    /* 메세지 큐의 TIME_OUT을 읽어 들인다. */
    ulnPropertiesSetIntegerAsEnv(aProperties,
                                 &aProperties->mIpcDaClientMessageQTimeout,
                                 ULN_PROPERTIES_ENV_IPCDACLIENTMESSAGEQTIMEOUT);
    return;

    ACI_EXCEPTION_END;

    return;
}

/**
 *  ulnPropertiesDestroy
 */
void ulnPropertiesDestroy(ulnProperties *aProperties)
{
    ACI_TEST(aProperties == NULL);

    ACP_STR_FINAL(aProperties->mHomepath);
    ACP_STR_FINAL(aProperties->mConfFilepath);
    ACP_STR_FINAL(aProperties->mUnixdomainFilepath);
    ACP_STR_FINAL(aProperties->mIpcFilepath);
    ACP_STR_FINAL(aProperties->mIPCDAFilepath);

    ACI_EXCEPTION_END;

    return;
}

/* PROJ-2616 */
/*
 * ulnPropertiesCallbackInt32
 * 
 * 환경파일의 프로퍼티를 aProperties 구조체에 저장한다.
 **/
static acp_sint32_t ulnPropertiesCallbackInt32(
    acp_sint32_t    aDepth,
    acl_conf_key_t *aKey,
    acp_sint32_t    aLineNumber,
    void            *aValue,
    void            *aContext)
{
    acp_uint32_t *sKey = NULL;

    ulnProperties *sProperties = (ulnProperties *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aLineNumber);

    switch (aKey->mID)
    {
        case IPCDA_CLIENT_SLEEP_TIME:
            sKey = &sProperties->mIpcDaClientSleepTime;
            break;
        case IPCDA_CLIENT_EXPIRE_COUNT:
            sKey = &sProperties->mIpcDaClientExpireCount;
            break;
        case IPCDA_CLIENT_MESSAGEQ_TIMEOUT:
            sKey = &sProperties->mIpcDaClientMessageQTimeout;
            break;
        default:
            break;
    }

    if (sKey != NULL)
    {
        if (aValue != NULL)
        {
            *sKey = *((acp_uint32_t *)aValue);
        }
        else
        {
            /* do nothing. */
        }
    }
    else
    {
        /* do nothing. */
    }

    return 0;
}


/**
 *  ulnPropertiesCallbackString
 *
 *  환경파일의 프로퍼티를 aProperties 구조체에 저장한다.
 */
acp_sint32_t ulnPropertiesCallbackString(acp_sint32_t    aDepth,
                                         acl_conf_key_t *aKey,
                                         acp_sint32_t    aLineNumber,
                                         void           *aValue,
                                         void           *aContext)
{
    ACP_STR_DECLARE_DYNAMIC(*sDest);
    ulnProperties *sProperties = (ulnProperties *)aContext;

    acp_bool_t sNeedExpansion = ACP_FALSE;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aLineNumber);

    switch (aKey->mID)
    {
        case UNIXDOMAIN_FILEPATH:
            sDest = &sProperties->mUnixdomainFilepath;
            sNeedExpansion = ACP_TRUE;
            break;

        case IPC_FILEPATH:
            sDest = &sProperties->mIpcFilepath;
            sNeedExpansion = ACP_TRUE;
            break;

        case IPCDA_FILEPATH:
            sDest = &sProperties->mIPCDAFilepath;
            sNeedExpansion = ACP_TRUE;
            break;

        default:
            sDest = NULL;
            break;
    }

    if (sDest != NULL)
    {
        if (sNeedExpansion == ACP_TRUE)
        {
            ulnPropertiesExpandValuesStr(sProperties,
                                         sDest,
                                         (acp_str_t *)aValue);
        }
        else
        {
            acpStrCpy(sDest, (acp_str_t *)aValue);
        }
    }
    else
    {
        /* Nothing */
    }

    return 0;
}

/**
 *  ulnPropertiesExpandValuesStr
 *
 *  ?를 $ALTIBASE_HOME 경로로 확장한다.
 */
void ulnPropertiesExpandValuesStr(ulnProperties *aProperties,
                                  acp_str_t     *aDest,
                                  acp_str_t     *aSrc)
{
    acp_sint32_t i = 0;
    acp_sint32_t sSrcLen = acpStrGetLength(aSrc);
    acp_char_t  *sSrcBuf = acpStrGetBuffer(aSrc);

    ACI_TEST(sSrcLen <= 0);

    acpStrClear(aDest);

    for (i = 0; i < sSrcLen; i++)
    {
        if (sSrcBuf[i] == '?')
        {
            acpStrCat(aDest, &aProperties->mHomepath);
        }
        else
        {
            acpStrCatBuffer(aDest, sSrcBuf + i, 1);
        }
    }

    ACI_EXCEPTION_END;

    return;
}

/**
 *  ulnPropertiesSetDefault
 *
 *  $ALTIBASE_HOME은 컴파일 시간에 알 수 없기 때문에
 *  gUlnPropertiesConfDef에 default 값을 설정할 수 없다.
 */
void ulnPropertiesSetDefault(ulnProperties *aProperties,
                             acp_char_t    *aHomepath)
{
    acpStrCpyFormat(&aProperties->mHomepath,
                    "%s",
                    aHomepath);

    acpStrCpyFormat(&aProperties->mConfFilepath,
                    "%s%s%s%s%s",
                    aHomepath,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_DIR_CONF,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_FILE_PROPERTIES);

    acpStrCpyFormat(&aProperties->mUnixdomainFilepath,
                    "%s%s%s%s%s",
                    aHomepath,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_DIR_TRC,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_FILE_UNIX);

    acpStrCpyFormat(&aProperties->mIpcFilepath,
                    "%s%s%s%s%s",
                    aHomepath,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_DIR_TRC,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_FILE_IPC);

    /* PROJ-2616 */
    acpStrCpyFormat(&aProperties->mIPCDAFilepath,
                    "%s%s%s%s%s",
                    aHomepath,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_DIR_TRC,
                    ACI_DIRECTORY_SEPARATOR_STR_A,
                    ULN_PROPERTIES_FILE_IPCDA);
    /* PROJ-2616 */
    aProperties->mIpcDaClientSleepTime       = IPCDA_DEF_CLIENT_SLEEP_TIME;
    aProperties->mIpcDaClientExpireCount     = IPCDA_DEF_CLIENT_EXP_CNT;
    aProperties->mIpcDaClientMessageQTimeout = IPCDA_DEF_MESSAGEQ_TIMEOUT;
    return;
}

/**
 *  ulnPropertiesSetIntegerAsEnv
 *
 *  환경변수로 프로퍼티를 INTEGER로 설정한다.
 */
void ulnPropertiesSetIntegerAsEnv(ulnProperties *aProperties,
                                  acp_uint32_t  *aProperty,
                                  acp_char_t    *aPropertyName)
{
    acp_char_t   *sEnvValue = NULL;
    acp_sint32_t  sValue = 0;

    ACP_UNUSED(aProperties);

    ACI_TEST(aPropertyName == NULL || aProperty == NULL);

    acpEnvGet(aPropertyName, &sEnvValue);

    if (sEnvValue != NULL)
    {
        if (sEnvValue[0] != '\0')
        {
            sValue = atoi(sEnvValue);
            if (sValue < 0)
            {
                *aProperty = 0;
            }
            else
            {
                *aProperty = (acp_uint32_t)sValue;
            }
        }
        else
        {
            /* Nothing */
        }
    }
    else
    {
        /* Nothing */
    }

    ACI_EXCEPTION_END;

    return;
}

/**
 *  ulnPropertiesSetStringAsEnv
 *
 *  환경변수로 프로퍼티를 설정한다.
 *  aNeedExpansion이 TRUE이면 ?를 $ALTIBASE_HOME으로 확장한다.
 */
void ulnPropertiesSetStringAsEnv(ulnProperties *aProperties,
                                 acp_str_t     *aProperty,
                                 acp_char_t    *aPropertyName,
                                 acp_bool_t     aNeedExpansion)
{
    acp_char_t *sEnvValue = NULL;
    ACP_STR_DECLARE_DYNAMIC(sEnvValueStr);

    ACI_TEST(aPropertyName == NULL || aProperty == NULL);

    acpEnvGet(aPropertyName, &sEnvValue);

    if (sEnvValue != NULL)
    {
        if (sEnvValue[0] != '\0')
        {
            if (aNeedExpansion == ACP_TRUE)
            {
                ACP_STR_INIT_DYNAMIC(sEnvValueStr, 128, 128);

                acpStrCpyFormat(&sEnvValueStr,
                                "%s",
                                sEnvValue);
                ulnPropertiesExpandValuesStr(aProperties,
                                             aProperty,
                                             &sEnvValueStr);

                ACP_STR_FINAL(sEnvValueStr);
            }
            else
            {
                acpStrCpyFormat(aProperty,
                                "%s",
                                sEnvValue);
            }
        }
        else
        {
            /* Nothing */
        }
    }
    else
    {
        /* Nothing */
    }

    ACI_EXCEPTION_END;

    return;
}

/**
 *  ulnPropertiesExpandValues
 *
 *  ?를 $ALTIBASE_HOME으로 확장한다.
 */
ACI_RC ulnPropertiesExpandValues( ulnProperties *aProperties,
                                  acp_char_t    *aDest,
                                  acp_sint32_t   aDestSize,
                                  acp_char_t    *aSrc,
                                  acp_sint32_t   aSrcLen )
{
    acp_char_t   *sHomepath    = NULL;
    acp_sint32_t  sHomepathLen = 0;
    acp_sint32_t  i = 0;
    acp_rc_t      sRC;

    ACI_TEST( aProperties == NULL || aDest == NULL || aDestSize <= 0 );
 
    aDest[0] = '\0';
    ACI_TEST_RAISE( aSrc == NULL || aSrcLen <= 0, NO_NEED_WORK );

    sHomepath    = acpStrGetBuffer(&aProperties->mHomepath);
    sHomepathLen = acpStrGetLength(&aProperties->mHomepath);

    for (i = 0; i < aSrcLen; i++)
    {
        if (aSrc[i] == '?')
        {
            sRC = acpCStrCat(aDest,
                             aDestSize,
                             sHomepath,
                             sHomepathLen);

            if (sRC == ACP_RC_ETRUNC)
            {
                break;
            }
            else
            {
                aDestSize -= sHomepathLen;
            }
        }
        else
        {
            sRC = acpCStrCat(aDest,
                             aDestSize,
                             aSrc + i,
                             1);

            if (sRC == ACP_RC_ETRUNC)
            {
                break;
            }
            else
            {
                aDestSize -= 1;
            }
        }
    }

    ACI_EXCEPTION_CONT( NO_NEED_WORK );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
