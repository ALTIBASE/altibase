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
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKD_DEF_H_
#define _O_DKD_DEF_H_ 1


#include <idTypes.h>
#include <dkDef.h>

#define DKD_BUFFER_BLOCK_SIZE_UNIT_MB  ((1024)*(1024)) /* MB: Mega byte */
#define DKD_BUFFER_BLOCK_SIZE_UNIT_KB  (1024)          /* KB: Kilo byte */

/* Define record cartridge */
typedef struct dkdRecord
{
    void          * mData;  /* Record 의 실제 데이터 */
    iduListNode     mNode;
} dkdRecord;

#endif /* _O_DKD_DEF_H_ */

