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
 * $Id:  $
 **********************************************************************/

#ifndef  _O_IDTYPES_CLIENT_H_
# define _O_IDTYPES_CLIENT_H_ 1

#include <acp.h>
#include <acl.h>

/*
 * XA XID
 */
/* BUG-18981 */
#define ID_MAXXIDDATASIZE  128      /* size in bytes */
#define ID_MAXGTRIDSIZE    64      /* maximum size in bytes of gtrid */
#define ID_MAXBQUALSIZE    64      /* maximum size in bytes of bqual */

#define ID_XID_DATA_MAX_LEN 256
#define ID_GTRIDSIZE    18      
#define ID_BQUALSIZE    4      
#define ID_XIDDATASIZE  (ID_GTRIDSIZE + ID_BQUALSIZE)

/*
 * fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
 * 그들간의 관계를 정확히 유지해야 함.
 */
#define ID_NULL_SESSION_ID  ACP_UINT32_MAX
#define ID_NULL_TRANS_ID    ACP_UINT32_MAX
struct id_xid_t
{
    acp_slong_t formatID;            /* format identifier */
    acp_slong_t gtrid_length;        /* value from 1 through 64 */
    acp_slong_t bqual_length;        /* value from 1 through 64 */
    acp_sint8_t data[ID_MAXXIDDATASIZE];
};

typedef struct id_xid_t ID_XID;


#endif /* _O_IDTYPES_H_ */
