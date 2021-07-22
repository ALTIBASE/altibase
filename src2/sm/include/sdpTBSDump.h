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
 * $Id:$
 ***********************************************************************/

# ifndef _O_SDP_TBS_DUMP_H_
# define _O_SDP_TBS_DUMP_H_ 1

# include <sdr.h>

class sdpTBSDump
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC buildRecord4ExtFreeList(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * aDumpObj,
        iduFixedTableMemory * aMemory );

};

#endif // _O_SDP_TBS_DUMP_H_

