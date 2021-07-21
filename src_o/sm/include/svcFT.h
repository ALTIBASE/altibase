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
 * $Id: svcFT.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SVC_FT_H_
#define _O_SVC_FT_H_  (1)

# include <idu.h>
# include <smDef.h>
# include <sdcDef.h>

class svcFT
{
public:

    //------------------------------------------
    // For D$VOL_TABLE_RECORD
    //------------------------------------------
    static IDE_RC buildRecordVolTableRecord( idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * aDumpObj,
                                             iduFixedTableMemory * aMemory );

 };

#endif /* _O_SVC_FT_H_ */
