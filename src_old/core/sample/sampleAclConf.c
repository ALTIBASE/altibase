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
 * $Id: sampleAclConf.c $
 ******************************************************************************/


#include <acp.h>
#include <acl.h>


/* Sample config file
 * ROUTERS = (
 *     (
 *         DOMAINID = 1
 *         DOMAINIP = 192.168.0.1
 *         CHANNELS = (
 *             (
 *                 LOCALIP = 192.168.0.2
 *                 LOCALPORT = 45000
 *                 REMOTEIP = 192.168.0.3
 *                 REMOTEPORT = 45001
 *             ),
 *             (
 *                 LOCALIP = 192.168.0.7
 *                 LOCALPORT = 45002
 *                 REMOTEIP = 192.168.0.8
 *                 REMOTEPORT = 45003
 *             )
 *         )
 *     ),
 *     (
 *         DOMAINID = 2
 *         DOMAINIP = 192.168.0.4
 *         CHANNELS = (
 *             (
 *                 LOCALIP = 192.168.0.5
 *                 LOCALPORT = 45001
 *                 REMOTEIP = 192.168.0.6
 *                 REMOTEPORT = 45000
 *             ),
 *             (
 *                 LOCALIP = 192.168.0.9
 *                 LOCALPORT = 45004
 *                 REMOTEIP = 192.168.0.10
 *                 REMOTEPORT = 45005
 *             )
 *         )
 *     )
 *  )
 */


/*
 * callback functions
 */
static acp_sint32_t sampleAclConfCallbackSInt32(acp_sint32_t    aDepth,
                               acl_conf_key_t *aKey,
                               acp_sint32_t   aLineNumber,
                               void           *aValue,
                               void           *aContext);
static acp_sint32_t sampleAclConfCallbackUInt32(acp_sint32_t    aDepth,
                               acl_conf_key_t *aKey,
                               acp_sint32_t   aLineNumber,
                               void           *aValue,
                               void           *aContext);
static acp_sint32_t sampleAclConfCallbackString(acp_sint32_t    aDepth,
                               acl_conf_key_t *aKey,
                               acp_sint32_t   aLineNumber,
                               void           *aValue,
                               void           *aContext);

/*
 * configuration key definitions
 */
enum
{
    SAMPLE_CONF_ROUTERS    = 1,
    SAMPLE_CONF_CHANNELS   = 2,
    SAMPLE_CONF_DOMAINIP   = 3,
    SAMPLE_CONF_DOMAINID   = 4,
    SAMPLE_CONF_LOCALIP    = 5,
    SAMPLE_CONF_LOCALPORT  = 6,
    SAMPLE_CONF_REMOTEIP   = 7,
    SAMPLE_CONF_REMOTEPORT = 8
};

static acl_conf_def_t gTestConfChannelsDef[] =
{
    ACL_CONF_DEF_STRING("LOCALIP",
                        SAMPLE_CONF_LOCALIP,
                        "",
                        1,
                        sampleAclConfCallbackString),
    ACL_CONF_DEF_UINT32("LOCALPORT",
                        SAMPLE_CONF_LOCALPORT,
                        65536,
                        0,
                        0,
                        1,
                        sampleAclConfCallbackUInt32),
    ACL_CONF_DEF_STRING("REMOTEIP",
                        SAMPLE_CONF_REMOTEIP,
                        "",
                        1,
                        sampleAclConfCallbackString),
    ACL_CONF_DEF_UINT32("REMOTEPORT",
                        SAMPLE_CONF_REMOTEPORT,
                        65536,
                        0,
                        0,
                        1,
                        sampleAclConfCallbackUInt32),
    ACL_CONF_DEF_END()
};

static acl_conf_def_t gTestConfRoutersDef[] =
{
    ACL_CONF_DEF_SINT32("DOMAINID",
                        SAMPLE_CONF_DOMAINID,
                        1,
                        1,
                        64,
                        1,
                        sampleAclConfCallbackSInt32),
    ACL_CONF_DEF_STRING("DOMAINIP",
                        SAMPLE_CONF_DOMAINIP,
                        "",
                        1,
                        sampleAclConfCallbackString),
    ACL_CONF_DEF_CONTAINER("CHANNELS",
                           SAMPLE_CONF_CHANNELS,
                           gTestConfChannelsDef,
                           0),
    ACL_CONF_DEF_END()
};

static acl_conf_def_t gTestConfDef[] =
{
    ACL_CONF_DEF_CONTAINER("ROUTERS",
                           SAMPLE_CONF_ROUTERS,
                           gTestConfRoutersDef,
                           0),
    ACL_CONF_DEF_END()
};


static void sampleAclConfGetKeyString(acp_sint32_t    aDepth,
                         acl_conf_key_t *aKey,
                         acp_str_t      *aStr)
{
    acp_sint32_t i;

    for (i = 0; i < aDepth; i++)
    {
        (void)acpStrCatFormat(aStr,
                              "%s%S[%d]",
                              ((i == 0) ? "" : "."),
                              &aKey[i].mKey,
                              aKey[i].mIndex);
    }
}

static acp_sint32_t sampleAclConfCallbackSInt32(acp_sint32_t    aDepth,
                               acl_conf_key_t *aKey,
                               acp_sint32_t   aLineNumber,
                               void           *aValue,
                               void           *aContext)
{
    ACP_STR_DECLARE_STATIC(sKey, 128);
    ACP_STR_INIT_STATIC(sKey);

    ACP_UNUSED(aContext);
    ACP_UNUSED(aLineNumber);

    sampleAclConfGetKeyString(aDepth, aKey, &sKey);

    (void)acpPrintf("%S <- %u\n", &sKey, *(acp_sint32_t *)aValue);

    return 0;
}

static acp_sint32_t sampleAclConfCallbackUInt32(acp_sint32_t    aDepth,
                               acl_conf_key_t *aKey,
                               acp_sint32_t   aLineNumber,
                               void           *aValue,
                               void           *aContext)
{
    ACP_STR_DECLARE_STATIC(sKey, 128);
    ACP_STR_INIT_STATIC(sKey);

    ACP_UNUSED(aContext);
    ACP_UNUSED(aLineNumber);

    sampleAclConfGetKeyString(aDepth, aKey, &sKey);

    (void)acpPrintf("%S <- %u\n", &sKey, *(acp_uint32_t *)aValue);

    return 0;
}

static acp_sint32_t sampleAclConfCallbackString(acp_sint32_t    aDepth,
                               acl_conf_key_t *aKey,
                               acp_sint32_t   aLineNumber,
                               void           *aValue,
                               void           *aContext)
{
    ACP_STR_DECLARE_STATIC(sKey, 128);
    ACP_STR_DECLARE_STATIC(sType, 128);

    ACP_STR_INIT_STATIC(sKey);
    ACP_STR_INIT_STATIC(sType);

    ACP_UNUSED(aContext);
    ACP_UNUSED(aLineNumber);

    sampleAclConfGetKeyString(aDepth, aKey, &sKey);

    switch(aKey[aDepth - 1].mID)
    {
        case SAMPLE_CONF_LOCALIP:
            (void)acpStrCpyCString(&sType, "Local IP");
            break;
        case SAMPLE_CONF_REMOTEIP:
            (void)acpStrCpyCString(&sType, "Remote IP");
            break;
        case SAMPLE_CONF_DOMAINIP:
            (void)acpStrCpyCString(&sType, "Domain IP");
            break;
        default:
            (void)acpStrCpyCString(&sType, "Undefined");
            break;
    }

    (void)acpPrintf("%S <- <%S> %S\n", &sKey, &sType, (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t sampleAclConfCallbackError(acp_str_t    *aFilePath,
                              acp_sint32_t  aLine,
                              acp_str_t    *aError,
                              void         *aContext)
{
    ACP_UNUSED(aFilePath);
    ACP_UNUSED(aLine);
    ACP_UNUSED(aContext);

    (void)acpPrintf("%S\n", aError);

    return 0;
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_rc_t sRC;
    acp_sint32_t sCallbackResult;

    ACP_STR_DECLARE_STATIC(sPath, 256);
    ACP_STR_INIT_STATIC(sPath);

    if (aArgc == 2)
    {
        (void)acpStrCpyFormat(&sPath, "%s", aArgv[1]);

        sRC = aclConfLoad(&sPath,
                          gTestConfDef,
                          sampleAclConfCallbackString,
                          sampleAclConfCallbackError,
                          &sCallbackResult,
                          NULL);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpPrintf("%S file successfully loaded\n", &sPath);
        }
        else
        {
            (void)acpPrintf("%S file loading failed\n", &sPath);
        }

    }
    else
    {
        /* do nothing */
    }

    return 0;
}
