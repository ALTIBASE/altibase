/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/*****************************************************************************
 * $Id: csType.h 89129 2020-11-03 04:12:46Z jykim $
 ****************************************************************************/

#ifndef _O_CS_TYPE_H_
#define _O_CS_TYPE_H_

typedef enum idBool
{
    ID_TRUE = 0,
    ID_FALSE
} idBool;

typedef unsigned long ULong;
typedef unsigned int  UInt;
typedef long          SLong;
typedef int           SInt;

typedef ULong         Address;

#endif
