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
 * $Id: stnIndexTypes.cpp 18910 2006-11-13 01:56:34Z leekmo $
 **********************************************************************/

#include <idl.h>
#include <stix.h>
#include <stnmrModule.h>
#include <stndrModule.h>

smnIndexType gStmnRtreeIndex =
{
    SMI_ADDITIONAL_RTREE_INDEXTYPE_ID,
    SMN_EXTERNAL_INDEX,
    SMN_INDEX_UNIQUE_DISABLE          |
    SMN_INDEX_COMPOSITE_DISABLE       |
    SMN_INDEX_NUMERIC_COMPARE_DISABLE |
    SMN_INDEX_AGING_ENABLE,
    (SChar*)"RTREE",
    {NULL, NULL, &stnmrModule, &stndrModule, NULL, NULL, NULL}
};
