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
 

/**********************************************************************
 * $Id: ulsdnDbc.c 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>
#include <mtcc.h>

#include <ulsd.h>


/* PROJ-2638 shard native linker */
acp_char_t * ulsdDbcGetShardTargetDataNodeName(ulnDbc *aDbc)
{
    return aDbc->mShardDbcCxt.mShardTargetDataNodeName;
}

/* PROJ-2638 shard native linker */
void ulsdDbcSetShardTargetDataNodeName(ulnDbc *aDbc, acp_uint8_t * aNodeName)
{
    acpSnprintf(aDbc->mShardDbcCxt.mShardTargetDataNodeName,
                ULSD_MAX_NODE_NAME_LEN + 1,
                "%s",
                aNodeName);
}

/* PROJ-2638 shard native linker */
void ulsdDbcGetLinkInfo( ulnDbc       * aDbc,
                         acp_char_t   * aOutBuff,
                         acp_uint32_t   aOutBufLength,
                         acp_sint32_t   aKey )
{
    ulnDbc * sDbc = NULL;

    sDbc = ulnDbcSwitch( aDbc );

    if ( sDbc != NULL )
    {
        if ( cmiGetLinkInfo( sDbc->mLink,
                             aOutBuff,
                             aOutBufLength,
                             (cmiLinkInfoKey)aKey ) != ACI_SUCCESS )
        {
            if ( ( aOutBuff != NULL ) && ( aOutBufLength > 0 ) )
            {
                aOutBuff[0] = '\0';
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        if ( ( aOutBuff != NULL ) && ( aOutBufLength > 0 ) )
        {
            aOutBuff[0] = '\0';
        }
        else
        {
            /* Nothing to do */
        }
    }
}

