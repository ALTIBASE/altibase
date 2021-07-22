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
#include <ulnURL.h>

/*
 * BUGBUG : 지금은 일단 URL 은 무시하도록 하자.
 *          리팩토링 하려고 보니까 구조에 너무나 dependent 하다.
 */
// ACI_RC ulnSetConnAttrUrl(ulnFnContext *aContext,  SChar *aAttr)
// {
//     /*
//      * BUGBUG : 리팩토링 리팩토링 리팩토링 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//      */
//     ulnURL sUrl;
//     ulnDbc *sDbc = aContext->mHandle.mDbc;
//
//     ulnPeerLinkArg *sRoot = sDbc->mSession.mLinkArgs;
//     ulnPeerLinkArg *sArg  = NULL;
//
//     SChar *i, *sURI, *sVal;
//     UShort sId;
//
//     /* cleanup the properties if it URL */
//     if (sDbc->mIsURL == ID_FALSE)
//     {
//         ulnDbcSetPortNumber(sDbc, 0);
//         ulnSetConnAttrById(ULN_CONN_ATTR_DSN...);
//         ulnDbcSetCmiLinkImpl(sDbc, CMI_LINK_IMPL_INVALID);
//         idlOS::memset((void *)(&(aDbc->mConnectArg)), 0, ACI_SIZEOF(cmiConnectArg));
//
//         ulnError(aContext,
//                  ULN_EI_01S09,
//                  "CONNTYPE and PORT_NO properties are ignored, if any. Because URL string "
//                  "is being used.");
//
//         sDbc->mSession.mIsURL = ID_TRUE;
//     }
//
//     for(i =(SChar*)aAttr, sURI=idlOS::strsep(&i, ",");
//         sURI != NULL;
//         sURI  = idlOS::strsep(&i, ","))
//     {
//         if (*sURI != '\0')
//         {
//             IDE_TEST(urlParse(sURI, &sUrl) != ACI_SUCCESS);
//
//             /* get the schema id by list of keys */
//             for( sId = 0; gULN_CM_PROTOCOL[sId].mKey != NULL;  sId++)
//             {
//                 if ( 0 == idlOS::strcmp(gULN_CM_PROTOCOL[sId].mKey, sUrl.mSchema) )
//                 {
//                     if ( cmiIsSupportedLinkImpl(gULN_CM_PROTOCOL[sId].mVal) )
//                     {
//                         /* Allocate Argument */
//                         sArg = (ulnPeerLinkArg*)idlOS::malloc(ACI_SIZEOF(ulnPeerLinkArg));
//                         IDE_TEST_RAISE( sArg == NULL, ERR_MEMORY);
//                         IDU_LIST_INIT(&sArg->mList);
//
//                         /* LINK to List of Arguments */
//                         if (sRoot == NULL)
//                         {
//                             sRoot = sArg;
//                         }
//                         else
//                         {
//                             IDU_LIST_ADD_LAST( (iduList*)sRoot, (iduList*)sArg);
//                         }
//
//                         /* get the real Type of connection */
//                         sArg->mType = gULN_CM_PROTOCOL[sId].mVal;
//                         break;
//                     }
//                     else
//                     {
//                         /* ERROR  Such protocol is not Supported !!*/
//                         IDE_TEST( ulnError(aContext,
//                                            ULN_EI_0A000,
//                                            "CM protocol {%s://} does not supported.",
//                                            sUrl.mSchema ) != ACI_SUCCESS);
//                     }
//                 }
//             }
//             /* No schemas in support list */
//             switch (sArg->mType)
//             {
//                 case CMI_LINK_IMPL_TCP : /* TCP Implementation communication */
//                     sVal = sUrl.mAuthority;
//                     /* get HostName from it */
//                     sArg->mConnectArg.mTCP.mAddr = idlOS::strsep( &sVal, ":");
//
//                     IDE_TEST_RAISE( sVal == NULL, ERR_URL_FORMAT);
//
//                     /* get the port number for TCP connection */
//                     sArg->mConnectArg.mTCP.mPort = idlOS::strtol(sVal, &sVal, 10);
//
//                     /* just make sure the port is assigned well */
//                     IDE_TEST_RAISE( *sVal != 0, ERR_URL_FORMAT);
//                     // idlOS::printf(" tcp~>%s:%d\n", sArg->mConnectArg.mTCP.mAddr, sArg->mConnectArg.mTCP.mPort );
//                     break;
//
//                 case CMI_LINK_IMPL_UNIX: /* UNIX Domain communicatiom        */
//
//                     sArg->mConnectArg.mUNIX.mHomePath = sUrl.mPath;
//
//                     /* Path could not be NULL for UNIX */
//                     IDE_TEST_RAISE( sUrl.mPath == NULL, ERR_URL_FORMAT);
//                     //idlOS::printf(" unix~>%s\n",sArg->mConnectArg.mUNIX.mHomePath );
//                     break;
//
//                 case CMI_LINK_IMPL_IPC : /* SysV Shared Memory communication */
//
//                     /* Set the SysV Shared Memory Key */
//                     sArg->mConnectArg.mIPC.mShmKey = idlOS::strtol(sUrl.mAuthority, &sVal, 10);
//
//                     /* just make sure the port is assigned well */
//                     IDE_TEST_RAISE( *sVal != 0, ERR_URL_FORMAT);
//                     //idlOS::printf(" ipc~>%d\n", sArg->mConnectArg.mIPC.mShmKey );
//
//                     break;
//
//                 case CMI_LINK_IMPL_NONE: /* Unsupported URI for Connection   */
//                     IDE_RAISE(ERR_URI_PROTOCOL);
//                     break;
//
//                 default: IDE_ASSERT(0); /* Never ever reach there            */
//
//             }
//
//             /* URL->mQuery part of URL is an optinal properties */
//             if ( sUrl.mQuery != NULL )
//             {
//                 IDE_TEST(ulnConnStrParseIt(aContext,
//                                                sUrl.mQuery,
//                                                (SChar *)"&",
//                                                (tokenCallBack)ulnSetConnectAttrByAlias)
//                          != ACI_SUCCESS);
//             }
//
//             /* URL->mFragment parsing should be here */
//             if ( sUrl.mFragment != NULL)
//             {
//                 ;
//             }
//         }
//     }
//
//     /* BUGBUG-> MOVE TO Connect ???  Append The Arguments to the DataSource */
//     //    IDE_ASSERT( ulnDataSourceSetArgument( sDbc->
//     //                    mSession.mDataSource,  sRoot) == ACI_SUCCESS);
//
//     sDbc->mSession.mLinkArgs = sRoot;
//
//     return ACI_SUCCESS;
//
//     IDE_EXCEPTION(ERR_URI_PROTOCOL);
//     {
//         ulnError(aContext,ULN_EI_0A000,"URL schema = %s is not supported");
//     }
//
//     IDE_EXCEPTION(ERR_URL_FORMAT);
//     {
//         ulnError(aContext, ULN_EI_HY024, NULL);
//     }
//
//     IDE_EXCEPTION(ERR_MEMORY);
//     {
//         ulnError(aContext, ULN_EI_HY001, NULL);
//     }
//
//     IDE_EXCEPTION_END;
//
//     return ACI_FAILURE;
// }

ACI_RC ulnSetConnAttrUrl(ulnFnContext *aContext,  acp_char_t *aAttr)
{
    ACP_UNUSED(aContext);
    ACP_UNUSED(aAttr);

    return ACI_FAILURE;
}

#define TO_LOWER(c)  ( (c>='A' && c<='Z')?(c|0x20):c )
#define TO_UPPER(c)  ( (c>='a' && c<='z')?(c&0xDF):c )

/**
 * Just skiped the spaces and other
 * userless chars from the Lead and
 * set it to 0 value
 */
acp_char_t* urlCleanLead(acp_char_t *s)
{
    while ((*s==' ')||(*s=='\"')||(*s=='\t')||(*s=='{'))
    {
        *s=0;
        s++;
    }
    return s;
}

acp_char_t *urlCleanKeyWord(acp_char_t *aKeyString)
{
    acp_uint32_t  sReadPos   = 0;
    acp_uint32_t  sWritePos  = 0;

    if (aKeyString != NULL)
    {
        while (aKeyString[sReadPos] != 0)
        {
            if ((aKeyString[sReadPos] != ' ') && (aKeyString[sReadPos] != '\t'))
            {
                aKeyString[sWritePos] = aKeyString[sReadPos];
                sWritePos++;
            }

            sReadPos++;
        }

        /*
         * null terminate
         */

        aKeyString[sWritePos] = 0;
    }

    return aKeyString;
}

/**
 * Clean lead& tailed wrong symbols
 * set it to 0 value
 */
acp_char_t * urlCleanValue(acp_char_t *aVal)
{
    /*
     * -_-;; 나중에 다시 검증하자.
     */

    acp_char_t    sUp = 0;
    acp_char_t   *s, *i;
    acp_sint32_t  j = 0;
    acp_sint32_t  sValLen = 0;
    /*
     * BUG-28623 [CodeSonar]Ignored Return Value
     */
    /*
     * BUG-28980 [CodeSonar]Uninitialized Variable
     */
    if ( aVal != NULL )
    {
        s = aVal;
        if(aVal[0] != '\0')
        {
            sValLen = acpCStrLen(aVal, ACP_SINT32_MAX);
            for ( i = aVal; *i != 0; i++)
            {
                switch (*i)
                {
                    case '\t':
                        if ( sUp != 0 )
                        {
                            *s++ = *i;
                        }
                        break;

                    case  '{':
                        case '\"':
                            if(sUp == 0)
                            {
                                sUp = *i;
                            }
                            else
                            {
                                if ( sUp == '\"' )
                                {
                                    sUp = 0;
                                    break;
                                }
                                else
                                {
                                    *s++ = *i;
                                }
                            }
                        break;

                    case  '}':
                        if (sUp == '{')
                        {
                            sUp = 0;
                            break;
                        }
                        *s++ = *i;
                        break;

                    default:
                        *s++ = *i;
                }
                j++;
                if(j > sValLen)
                {
                    break;
                }
            }
            *s = 0;
        }
        else
        {
            *s = 0;
        }
    }
    else
    {
        /* do nothing */
    }
    return aVal;
}


/*
** Return the length of the next component of the URL in a[] given
** that the component starts at s[0].  The initial sequence of the
** component must be aInit[].  The component is terminated by any
** character in aTerm[].  The length returned is 0 if the component
** doesn't exist.  The length includes the aInit[] string, but not
** the termination character.
**
**        Component        aInit      aTerm
**        ----------       -------    -------
**        mSchema          ""         ":/?#"
**        mAuthority       "//"       "/?#"
**        mPath            "/"        "?#"
**        mQuery           "?"        "#"
**        mFragment        "#"        ""
**
*/
static acp_uint16_t   urlComponentLength( const acp_char_t *a,     // The URL here
                                          const acp_char_t *aInit, // Initial Sequence of token
                                          const acp_char_t *aTerm) // Terminate Sequence of token
{
    acp_uint16_t n,i;

    if ( *a == 0 )
    {
        return 0;
    }

    /* match init check */
    for (i = 0; aInit[i]; i++)
    {
        if (aInit[i] != a[i])
        {
            return 0;
        }
    }

    /* calculate component length till Term */
    while ( a[i] )
    {
        for (n=0; aTerm[n]; n++)
        {
            if ( a[i]==aTerm[n] ) return i;
        }
        i++;
    }
    return i;
}



ACI_RC urlParse(acp_char_t *aURI,   //  Input URI Buffer Copy of URL
                ulnURL     *aUrl )  //  Result keeper struct for tokens
{
    acp_uint16_t  i,sLen;
    acp_char_t   *sBuf  =  aURI;

    /* 0.0 skip all useless characters */
    aURI = urlCleanLead(aURI);
    acpMemSet(aUrl, 0, ACI_SIZEOF(ulnURL) );

    /* 1.0 Schema "tcp://" detection */
    sLen = urlComponentLength( aURI, "", ":/?# ");
    if ( (sLen > 0) && aURI[sLen]==':' )
    {
        aUrl->mSchema = sBuf;

        /* 1.1 make it lowcase  */
        for ( i = 0; i < sLen; i++)
        {
            sBuf[i] = TO_LOWER(aURI[i]);
        }
        sBuf[i] = 0;

        aURI += i+1;
        sBuf = aURI;
    }
    else
    {
        return ACI_FAILURE;      // Schema mast be defined for URL
    }


    /* 2.0 Authority "192.168.1.1:20545" part here */
    sLen = urlComponentLength( aURI, "//", "/?# \"\'");
    if ( sLen > 2)
    {
        aURI += 2;
        for (i = 0; i < sLen-2;   i++)
        {
            sBuf[i] = aURI[i];
        }

        aUrl->mAuthority = sBuf;
        sBuf[i] = 0;
        sBuf += i+1;
        aURI += i;
    }
    else
    {
        aURI += 2;
    }

    /* 3.0 Path there such as "/usr/altibase"  */
    sLen = urlComponentLength( aURI, "", "?# \"\'");
    if ( sLen > 1 )
    {
        aUrl->mPath = sBuf;
        for (i = 0; i < sLen; i++)
        {
            sBuf[i] = aURI[i];
        }
        sBuf[i] = 0;
        sBuf += i;

        aURI += sLen;
    }

    /* 4.0 Query, there such as "name=xxx&addr=none" */
    i = urlComponentLength( aURI, "?", "# \"\'");
    if ( i > 0 )
    {
        aURI[0] = 0;

        aUrl->mQuery = aURI+1;
        aURI += i;
    }

    /* 5.0 Freagment there such as "fragment"  */
    i = urlComponentLength( aURI,  "#", " \"\'");
    if ( i > 0 )
    {
        aURI[0] = aURI[i] = 0;
        aURI++;

        aUrl->mFragment = aURI;
    }

    return  ACI_SUCCESS;
}

#undef TO_LOWER
#undef TO_UPPER

