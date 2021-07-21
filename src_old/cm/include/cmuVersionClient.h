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

#ifndef _O_CMU_VERSION_CLIENT_H_
#define _O_CMU_VERSION_CLIENT_H_ 1

// proj_2160 cm_type removal: cm version up: 5.6.2 -> 7.1.1
/* 7.1.2 Added protocols connectEx, fetchV2 */
/* 7.1.3 Added protocols propertySetV2 */
/* 7.1.4 Added protocols ParamDataInListV2, ParamDataInListV2Result, ExecuteV2, ExecuteV2Result */
/* 7.1.5 Added shard protocols */
/* 7.1.6 Added shard handshake protocol */
/* 7.1.7 Added/Modify shard protocol */
/* 7.1.8 Added V3 Protocols */
#define CM_MAJOR_VERSION 7
#define CM_MINOR_VERSION 1
#define CM_PATCH_VERSION 8

extern const acp_uint64_t  cmProtocolVersion;

#define CM_GET_MAJOR_VERSION(a) (acp_uint32_t)(((acp_uint64_t)(a) >> 48) & 0xffff)
#define CM_GET_MINOR_VERSION(a) (acp_uint32_t)(((acp_uint64_t)(a) >> 32) & 0xffff)
#define CM_GET_PATCH_VERSION(a) (acp_uint32_t)((acp_uint64_t)(a) & ACP_UINT64_LITERAL(0xffffffff))

#endif
