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
 * $Id: ulsdnDescribeCol.h 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/

#ifndef _O_ULSDN_DESCRIBECOL_H_
#define _O_ULSDN_DESCRIBECOL_H_ 1

/* PROJ-2638 shard native linker */
SQLRETURN ulsdDescribeCol( ulnStmt      *aStmt,
                           acp_uint16_t  aColumnNumber,
                           acp_char_t   *aColumnName,
                           acp_sint32_t  aBufferLength,
                           acp_uint32_t *aNameLengthPtr,
                           acp_uint32_t *aDataMtTypePtr,
                           acp_sint32_t *aPrecisionPtr,
                           acp_sint16_t *aScalePtr,
                           acp_sint16_t *aNullablePtr );
#endif //_O_ULSDN_DESCRIBECOL_H_
