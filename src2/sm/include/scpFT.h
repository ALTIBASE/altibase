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
 * $Id: scpFT.h 39159 2010-04-27 01:47:47Z anapple $
 *
 * Description :
 *
 * 본 파일은 Common-DataPort layer의 Fixed Table 헤더파일 입니다.
 *
 **********************************************************************/

#ifndef _O_SCP_FT_H_
#define _O_SCP_FT_H_ 1

#include <smDef.h>
#include <scpDef.h>

class scpFT
{
public:
    //------------------------------------------
    // For X$DATAPORT
    //------------------------------------------
    static IDE_RC buildRecord4DataPort( idvSQL              * /*aStatistics*/,
                                        void                *aHeader,
                                        void                *aDumpObj,
                                        iduFixedTableMemory *aMemory );
};

#endif // _O_SCP_FT_H_
