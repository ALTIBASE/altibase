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
 * $Id: smnFT.h 37720 2010-01-11 08:53:19Z cgkim $
 **********************************************************************/

#ifndef _O_SMN_FT_H_
# define _O_SMN_FT_H_ 1

# include <smc.h>
# include <smnDef.h>
# include <smnbDef.h>

/* X$INDEX 출력용 자료구조 */
typedef struct  smnIndexInfo4PerfV
{
    smOID        mTableOID;
    scPageID     mIndexSegPID;
    UInt         mIndexID;
    UInt         mIndexType;
}smnIndexInfo4PerfV;

class smnFT
{
public:
    static IDE_RC buildRecordForIndexInfo(idvSQL              * /*aStatistics*/,
                                          void        *aHeader,
                                          void        *aDumpObj,
                                          iduFixedTableMemory *aMemory);
};


#endif /* _O_SMN_FT_H_ */
