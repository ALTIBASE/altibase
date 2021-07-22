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
#include <ulnConnAttribute.h>
#include <ulnConnString.h>
#include <ulnString.h>

// BUG-39102
acp_sint32_t ulnCStrLenOfInt(acp_sint32_t aVal)
{
    acp_sint32_t sLen;
    if (aVal == 0)
    {
        sLen = 1;
    }
    else
    {
        sLen = (acp_sint32_t) acpMathFastLog10(acpMathFastFabs(aVal)) + 1;
        if (aVal < 0)
        {
            sLen++;
        }
    }
    return sLen;
}

ACI_RC ulnConnStrBuildOutConnString(ulnFnContext *aContext,
                                    acp_char_t   *aOut,
                                    acp_sint16_t  aOutBufLen,
                                    acp_sint16_t *aOutLenPtr)
{
    ulnDbc       *sDbc = aContext->mHandle.mDbc;
    acp_uint16_t  i;
    acp_sint16_t  sLen;
    acp_sint16_t  sLenLeft;
    acp_sint16_t  sTotLen = 0;
    acp_rc_t      sRet = ACP_RC_SUCCESS;

    ACI_TEST_RAISE( (aOut == NULL || aOutBufLen < 0) && (aOutLenPtr == NULL),
                    LABEL_SKIP );

    if (aOut == NULL || aOutBufLen <= 0)
    {
        sLenLeft = 0;
    }
    else
    {
        aOut[0] = '\0';
        sLenLeft = aOutBufLen;
    }

    if (ulnDbcGetDsnString(sDbc) != NULL)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "DSN=%s;", ulnDbcGetDsnString(sDbc));
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(ulnDbcGetDsnString(sDbc), ACP_SINT16_MAX) + 5;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if (ulnDbcGetUserName(sDbc) != NULL)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                               gUlnConnAttrTable[ULN_CONN_ATTR_UID].mKey,
                               ulnDbcGetUserName(sDbc));
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_UID].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(ulnDbcGetUserName(sDbc), ACP_SINT16_MAX) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrAutoCommit != SQL_UNDEF )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                               gUlnConnAttrTable[ULN_CONN_ATTR_AUTOCOMMIT].mKey,
                               gULN_BOOL[sDbc->mAttrAutoCommit].mKey);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_AUTOCOMMIT].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(gULN_BOOL[sDbc->mAttrAutoCommit].mKey, ACP_SINT16_MAX) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    for(i = 0; gSQL_OV_ODBC[i].mKey != NULL; i++)
    {
        if (sDbc->mOdbcVersion == (acp_uint32_t)gSQL_OV_ODBC[i].mValue)
        {
            if (sLenLeft > 0)
            {
                sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                                   gUlnConnAttrTable[ ULN_CONN_ATTR_ODBC_VERSION ].mKey,
                                   gSQL_OV_ODBC[i].mKey);
            }
            if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
            {
                sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_ODBC_VERSION].mKey, ACP_SINT16_MAX)
                     + acpCStrLen(gSQL_OV_ODBC[i].mKey, ACP_SINT16_MAX) + 2;
            }
            else
            {
                sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
            }
            sTotLen  += sLen;
            sLenLeft -= sLen;
            break;
        }
    }

    if (sDbc->mDateFormat != NULL)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_DATE_FORMAT ].mKey,
                               sDbc->mDateFormat);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_DATE_FORMAT].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mDateFormat, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mTimezoneString != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf( aOut + sTotLen, sLenLeft, "%s=%s;",
                                gUlnConnAttrTable[ ULN_CONN_ATTR_TIME_ZONE ].mKey,
                                sDbc->mTimezoneString );
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_TIME_ZONE].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mTimezoneString, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mNlsLangString != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_NLS_USE ].mKey,
                               sDbc->mNlsLangString);//   gNLS_USE_ARRAY[sDbc->mNlsLang].mKey);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_NLS_USE].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mNlsLangString, ACP_SINT16_MAX) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    /* PROJ-1579 NCHAR */
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "NLS_NCHAR_LITERAL_REPLACE=%"ACI_UINT32_FMT";",
                               sDbc->mNlsNcharLiteralReplace);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = ulnCStrLenOfInt(sDbc->mNlsNcharLiteralReplace) + 27;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( ulnDbcGetConnectionTimeout(sDbc) != ULN_CONN_TMOUT_DEFAULT)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "CONNECTION_TIMEOUT=%"ACI_UINT32_FMT";",
                               ulnDbcGetConnectionTimeout(sDbc));
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = ulnCStrLenOfInt(ulnDbcGetConnectionTimeout(sDbc)) + 10;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( (acp_uint32_t)gUlnConnAttrTable[ ULN_CONN_ATTR_QUERY_TIMEOUT ].mDefValue != sDbc->mAttrQueryTimeout)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "%s=%"ACI_UINT32_FMT";",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_QUERY_TIMEOUT ].mKey,
                               sDbc->mAttrQueryTimeout);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_QUERY_TIMEOUT].mKey, ACP_SINT16_MAX)
                 + ulnCStrLenOfInt(sDbc->mAttrQueryTimeout) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    if ( (acp_uint32_t)gUlnConnAttrTable[ ULN_CONN_ATTR_DDL_TIMEOUT ].mDefValue != sDbc->mAttrDdlTimeout)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "%s=%"ACI_UINT32_FMT";",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_DDL_TIMEOUT ].mKey,
                               sDbc->mAttrDdlTimeout);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_DDL_TIMEOUT].mKey, ACP_SINT16_MAX)
                 + ulnCStrLenOfInt(sDbc->mAttrDdlTimeout) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( (acp_uint32_t)gUlnConnAttrTable[ ULN_CONN_ATTR_LOGIN_TIMEOUT ].mDefValue != sDbc->mAttrLoginTimeout)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "%s=%"ACI_UINT32_FMT";",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_LOGIN_TIMEOUT ].mKey,
                               sDbc->mAttrLoginTimeout);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_LOGIN_TIMEOUT].mKey, ACP_SINT16_MAX)
                 + ulnCStrLenOfInt(sDbc->mAttrLoginTimeout) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrConnPooling != SQL_UNDEF)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "CONNECTION_POOLING=%s;",
                               gULN_BOOL[sDbc->mAttrConnPooling].mKey);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gULN_BOOL[sDbc->mAttrConnPooling].mKey, ACP_SINT16_MAX) + 20;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( gUlnConnAttrTable[ ULN_CONN_ATTR_DISCONNECT_BEHAVIOR ].mDefValue != sDbc->mAttrDisconnect)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "%s=%s,%1d;",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_DISCONNECT_BEHAVIOR ].mKey,
                               gULN_POOL[sDbc->mAttrDisconnect].mKey, sDbc->mAttrDisconnect );
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_DISCONNECT_BEHAVIOR].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(gULN_POOL[sDbc->mAttrDisconnect].mKey, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }
    /*
     * TXN_ISOLATION
     * BUGBUG : 지금 매뉴얼에는 없는 것. 구현 안됨.
     */
    if ( (acp_uint32_t)gUlnConnAttrTable[ ULN_CONN_ATTR_TXN_ISOLATION ].mDefValue != sDbc->mAttrTxnIsolation)
    {
        for(i = 0; gULN_SQL_TXN[i].mKey != NULL; i++)
        {
            if ((acp_uint32_t)gULN_SQL_TXN[i].mValue == sDbc->mAttrTxnIsolation)
            {
                if (sLenLeft > 0)
                {
                    sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                                       gUlnConnAttrTable[ ULN_CONN_ATTR_TXN_ISOLATION ].mKey,
                                       gULN_SQL_TXN[i].mKey);
                }
                if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
                {
                    sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_TXN_ISOLATION].mKey, ACP_SINT16_MAX)
                         + acpCStrLen(gULN_SQL_TXN[i].mKey, ACP_SINT16_MAX) + 4;
                }
                else
                {
                    sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
                }
                sTotLen  += sLen;
                sLenLeft -= sLen;
                break;
            }
        }
    }

    if ( sDbc->mConnType == ULN_CONNTYPE_TCP
      || sDbc->mConnType == ULN_CONNTYPE_SSL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "PORT_NO=%"ACI_UINT32_FMT";", sDbc->mPortNumber);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = ulnCStrLenOfInt(sDbc->mPortNumber) + 9;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mConnType != ULN_CONNTYPE_INVALID )
    {
        if (sLenLeft > 0)
        {
            (void) acpSnprintf(aOut + sTotLen, sLenLeft,
                               "CONNTYPE=%"ACI_UINT32_FMT";", sDbc->mConnType);
        }
        sTotLen  += 11;
        sLenLeft -= 11;
    }

    /*
     * STACK_SIZE ~> ID_UINT_MAX is not assign
     */
    if ( sDbc->mAttrStackSize != ACP_UINT32_MAX)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "%s=%"ACI_UINT32_FMT";",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_STACK_SIZE ].mKey,
                               sDbc->mAttrStackSize);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_STACK_SIZE].mKey, ACP_SINT16_MAX)
                 + ulnCStrLenOfInt(sDbc->mAttrStackSize) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( (acp_uint32_t)gUlnConnAttrTable[ ULN_CONN_ATTR_PACKET_SIZE ].mDefValue != sDbc->mAttrPacketSize)
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft,
                               "%s=%"ACI_UINT32_FMT";",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_PACKET_SIZE ].mKey,
                               sDbc->mAttrPacketSize);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_PACKET_SIZE].mKey, ACP_SINT16_MAX)
                 + ulnCStrLenOfInt(sDbc->mAttrPacketSize) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    /* PROJ-1891 Deferred Prepare */
    if ( sDbc->mAttrDeferredPrepare != SQL_UNDEF )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                               gUlnConnAttrTable[ ULN_CONN_ATTR_DEFERRED_PREPARE ].mKey, 
                               gULN_BOOL[sDbc->mAttrDeferredPrepare].mKey);
        }
        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_DEFERRED_PREPARE].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(gULN_BOOL[sDbc->mAttrDeferredPrepare].mKey, ACP_SINT16_MAX) + 1;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    /* PROJ-2474 SSL/TLS */
    if ( sDbc->mAttrSslCa != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf( aOut + sTotLen, sLenLeft, "%s=%s;",
                                gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CA].mKey,
                                sDbc->mAttrSslCa);
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CA].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mAttrSslCa, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }

        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrSslCaPath != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf( aOut + sTotLen, sLenLeft, "%s=%s;",
                                gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CAPATH].mKey,
                                sDbc->mAttrSslCaPath);
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CAPATH].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mAttrSslCaPath, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }

        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrSslCert != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf( aOut + sTotLen, sLenLeft, "%s=%s;",
                                gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CERT].mKey,
                                sDbc->mAttrSslCert );
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CERT].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mAttrSslCert, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }

        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrSslKey != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf( aOut + sTotLen, sLenLeft, "%s=%s;",
                                gUlnConnAttrTable[ULN_CONN_ATTR_SSL_KEY].mKey,
                                sDbc->mAttrSslKey );
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_SSL_KEY].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mAttrSslKey, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }

        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrSslCipher != NULL )
    {
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf( aOut + sTotLen, sLenLeft, "%s=%s;",
                                gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CIPHER].mKey,
                                sDbc->mAttrSslCipher );
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_SSL_CIPHER].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(sDbc->mAttrSslCipher, ACP_SINT16_MAX) + 4;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }

        sTotLen  += sLen;
        sLenLeft -= sLen;
    }

    if ( sDbc->mAttrSslVerify != SQL_UNDEF )
    {   
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%s;",
                               gUlnConnAttrTable[ULN_CONN_ATTR_SSL_VERIFY].mKey,
                               gULN_BOOL[sDbc->mAttrSslVerify].mKey);
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_SSL_VERIFY].mKey, ACP_SINT16_MAX)
                 + acpCStrLen(gULN_BOOL[sDbc->mAttrSslVerify].mKey, ACP_SINT16_MAX) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }  

    if ( sDbc->mTransactionalDDL != SQL_UNDEF )
    {   
        if (sLenLeft > 0)
        {
            sRet = acpSnprintf(aOut + sTotLen, sLenLeft, "%s=%"ACI_UINT32_FMT";",
                               gUlnConnAttrTable[ULN_CONN_ATTR_TRANSACTIONAL_DDL].mKey,
                               (acp_uint32_t)sDbc->mTransactionalDDL);
        }
        else
        {
            /* do nothing */
        }

        if (sLenLeft <= 0 || ACP_RC_IS_ETRUNC(sRet))
        {
            sLen = acpCStrLen(gUlnConnAttrTable[ULN_CONN_ATTR_TRANSACTIONAL_DDL].mKey, ACP_SINT16_MAX)
                + ulnCStrLenOfInt(sDbc->mTransactionalDDL) + 2;
        }
        else
        {
            sLen = acpCStrLen(aOut + sTotLen, sLenLeft);
        }
        sTotLen  += sLen;
        sLenLeft -= sLen;
    }  

    /* assign The Length */
    if (aOutLenPtr != NULL)
    {
        *aOutLenPtr = sTotLen;
    }

    if (aOut != NULL && aOutBufLen > 0 && sLenLeft < 0)
    {
        ulnError(aContext, ulERR_IGNORE_RIGHT_TRUNCATED);
    }

    ACI_EXCEPTION_CONT(LABEL_SKIP);

    return ACI_SUCCESS;
}

