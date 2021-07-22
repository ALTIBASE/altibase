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
 
#ifndef _O_SVC_DEF_H_
#define _O_SVC_DEF_H_ 1

#include <smDef.h>


#define SVC_COLUMN_TYPE_FIXED       (1)
#define SVC_COLUMN_TYPE_VARIABLE    (2)
/* PROJ-2174 Supporting Lob in Volatile tablespace */
#define SVC_COLUMN_TYPE_LOB         (3)

#define SVC_COLUMN_MODE_NA          (0)
#define SVC_COLUMN_MODE_IN          (1)
#define SVC_COLUMN_MODE_OUT         (2)

#endif /* _O_SVC_DEF_H_ */
