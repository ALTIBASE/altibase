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
 * $Id: scpfFT.h 39159 2010-04-27 01:47:47Z anapple $
 *
 * Description :
 *
 * 본 파일은 Common-DataPort-File layer의 Fixed Table 헤더파일 입니다.
 *
 **********************************************************************/

#ifndef _O_SCPF_FT_H_
#define _O_SCPF_FT_H_ 1

#include <smDef.h>
#include <scpfDef.h>

class scpfFT
{
public:
    //------------------------------------------
    // For X$DATAPORT_FILE_HEADER
    //------------------------------------------
    static IDE_RC buildRecord4DataPortFileHeader( 
        idvSQL              * /*aStatistics*/,
        void                *aHeader,
        void                *aDumpObj,
        iduFixedTableMemory *aMemory );

    //------------------------------------------
    // For X$DATAPORT_FILE_CURSOR
    //------------------------------------------
    static IDE_RC buildRecord4DataPortFileCursor(
        idvSQL              * /*aStatistics*/,
        void                *aHeader,
        void                *aDumpObj,
        iduFixedTableMemory *aMemory );
};

#endif // _O_SCPF_FT_H_


