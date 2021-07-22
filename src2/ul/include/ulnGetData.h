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

#ifndef _O_ULN_GET_DATA_H_
#define _O_ULN_GET_DATA_H_ 1

ACI_RC ulnSFID_29(ulnFnContext *aContext);

#define ULN_GD_ST_INITIAL    1
#define ULN_GD_ST_RETRIEVING 2

ACI_RC ulnGDInitColumn(ulnFnContext *aFnContext,
                       acp_uint16_t  aColumnNumber);

#endif /*_O_ULN_GET_DATA_H_*/
