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
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <ulnPrivate.h>

#include <ulsd.h>
#include <ulsdConnString.h>

acp_char_t * ulsdStrTok(acp_char_t     *aString,
                        acp_char_t      aToken,
                        acp_char_t    **aCurrentTokenPtr)
{
    acp_rc_t        sRC;
    acp_char_t     *sReturnTokenPtr = NULL;
    acp_sint32_t    sFindIndex = 0;

    if ( aString != NULL )
    {
        sRC = acpCStrFindChar(aString,
                              aToken,
                              &sFindIndex,
                              0,
                              ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_SENSITIVE);

        if ( sRC == ACP_RC_SUCCESS )
        {
            sReturnTokenPtr = aString;
            *aCurrentTokenPtr = aString + sFindIndex + 1;
            sReturnTokenPtr[sFindIndex] = '\0';
        }
        else if ( sRC == ACP_RC_ENOENT )
        {
            sReturnTokenPtr = aString;
            *aCurrentTokenPtr = NULL;
        }
        else
        {
            /* Do Nothing */
        }
    }
    else
    {
        ACI_TEST(*aCurrentTokenPtr == NULL);

        sRC = acpCStrFindChar(*aCurrentTokenPtr,
                              aToken,
                              &sFindIndex,
                              0,
                              ACP_CSTR_SEARCH_FORWARD | ACP_CSTR_CASE_SENSITIVE);

        if ( sRC == ACP_RC_SUCCESS )
        {
            sReturnTokenPtr = *aCurrentTokenPtr;
            *aCurrentTokenPtr = *aCurrentTokenPtr + sFindIndex + 1;
            sReturnTokenPtr[sFindIndex] = '\0';
        }
        else if ( sRC == ACP_RC_ENOENT )
        {
            sReturnTokenPtr = *aCurrentTokenPtr;
            *aCurrentTokenPtr = NULL;
        }
        else
        {
            /* Do Nothing */
        }
    }

    return sReturnTokenPtr;

    ACI_EXCEPTION_END;

    return NULL;
}

ACI_RC ulsdRemoveConnStrAttribute(acp_char_t    *aConnString,
                                  acp_char_t    *aRemoveKeyword)
{
    acp_char_t     *sAttrPtr = NULL;
    acp_char_t     *sCurrentTokenPtr = NULL;
    acp_char_t      sOutConnString[ULSD_MAX_CONN_STR_LEN + 1] = {0,};
    acp_char_t      sTempConnString[ULSD_MAX_CONN_STR_LEN + 1] = {0,};
    acp_char_t      sKeyword[MAX_CONN_STR_KEYWORD_LEN + 1] = {0,};
    acp_sint16_t    sConnStringLength;
    acp_sint32_t    sFoundIndex = 0;
    acp_bool_t      sIsFirstAttr = ACP_TRUE;

    sConnStringLength = acpCStrLen(aConnString, ULSD_MAX_CONN_STR_LEN);

    ACI_TEST(acpCStrCpy(sTempConnString,
                        ULSD_MAX_CONN_STR_LEN + 1,
                        aConnString,
                        sConnStringLength));

    ACI_TEST(acpSnprintf(sKeyword,
                         MAX_CONN_STR_KEYWORD_LEN + 1,
                         "%s=",
                         aRemoveKeyword));

    sAttrPtr = ulsdStrTok(sTempConnString, ';', &sCurrentTokenPtr);

    if ( sAttrPtr != NULL )
    {
        if ( acpCStrFindCStr(sAttrPtr,
                             sKeyword,
                             &sFoundIndex,
                             0,
                             ACP_CSTR_CASE_INSENSITIVE) == ACP_RC_ENOENT )
        {
            ACI_TEST(acpCStrCat(sOutConnString,
                                ULSD_MAX_CONN_STR_LEN,
                                sAttrPtr,
                                acpCStrLen(sAttrPtr, ULSD_MAX_CONN_STR_LEN)));

            sIsFirstAttr = ACP_FALSE;
        }
        else
        {
            /* Do Nothing */
        }
    }
    else
    {
        /* Do Nothing */
    }

    while ( sAttrPtr != NULL )
    {
        sAttrPtr = ulsdStrTok(NULL, ';', &sCurrentTokenPtr);

        if ( sAttrPtr != NULL )
        {
            if ( acpCStrFindCStr(sAttrPtr,
                                 sKeyword,
                                 &sFoundIndex,
                                 0,
                                 ACP_CSTR_CASE_INSENSITIVE) == ACP_RC_ENOENT )
            {
                sOutConnString[acpCStrLen(sOutConnString, ULSD_MAX_CONN_STR_LEN)] = (sIsFirstAttr == ACP_FALSE) ? ';' : '\0';
                sIsFirstAttr = ACP_FALSE;

                ACI_TEST(acpCStrCat(sOutConnString,
                                    ULSD_MAX_CONN_STR_LEN,
                                    sAttrPtr,
                                    acpCStrLen(sAttrPtr, ULSD_MAX_CONN_STR_LEN)));
            }
            else
            {
                /* Do Nothing */
            }
        }
        else
        {
            /* Do Nothing */
        }
    }

    ACI_TEST(acpCStrCpy(aConnString,
                        ULSD_MAX_CONN_STR_LEN + 1,
                        sOutConnString,
                        acpCStrLen(sOutConnString, ULSD_MAX_CONN_STR_LEN)));

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulsdMakeNodeBaseConnStr(ulnFnContext    *aFnContext,
                               acp_char_t      *aConnString,
                               acp_sint16_t     aConnStringLength,
                               acp_bool_t       aIsTestEnable,
                               acp_char_t      *aOutConnString)
{
    acpCStrCpy(aOutConnString, ULSD_MAX_CONN_STR_LEN + 1, aConnString, aConnStringLength);

    ACI_TEST_RAISE( ulsdRemoveConnStrAttribute( aOutConnString, "Conntype" )
                     != ACI_SUCCESS,
                    LABEL_NODE_CONNECTION_STRING_MAKE_FAIL );

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Dsn")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Port")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "DataSourceName")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Host")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "HostName")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Server")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "ServerName")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Port")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Port_No")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "ShardSessionId")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "ShardLibType")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "AlternateServers")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Alternate_Servers")
                    != ACI_SUCCESS,
                   LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

    if ( aIsTestEnable == ACP_TRUE )
    {
        ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Uid")
                       != ACI_SUCCESS,
                       LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);

        ACI_TEST_RAISE(ulsdRemoveConnStrAttribute(aOutConnString, "Pwd")
                       != ACI_SUCCESS,
                       LABEL_NODE_CONNECTION_STRING_MAKE_FAIL);
    }
    else
    {
        // Nothing to do.
    }

    return ACI_SUCCESS;
 
    ACI_EXCEPTION(LABEL_NODE_CONNECTION_STRING_MAKE_FAIL)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "MakeNodeConnStr",
                 "Failure to make node connection string.");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
