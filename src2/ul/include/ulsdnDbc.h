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
 * $Id: ulsdnDbc.h 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/
#ifndef _O_ULSDN_DBC_H_
#define _O_ULSDN_DBC_H_ 1

/* PROJ-2638 shard native linker */
acp_char_t * ulsdDbcGetShardTargetDataNodeName(ulnDbc *aDbc);
void ulsdDbcSetShardTargetDataNodeName(ulnDbc *aDbc, acp_uint8_t * aNodeName);
void ulsdDbcGetLinkInfo(ulnDbc      *aDbc,
                        acp_char_t  *aOutBuff,
                        acp_uint32_t aOutBufLength,
                        acp_sint32_t aKey);

#endif // _O_ULSDN_DBC_H_ 1
