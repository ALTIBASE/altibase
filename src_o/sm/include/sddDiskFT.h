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
 * $Id
 *
 * Description :
 *
 * 디스크 테이블스페이스 관련 Fixed Table 정의
 * 
 **********************************************************************/

#ifndef _O_SDD_DISK_FT_H_
#define _O_SDD_DISK_FT_H_ 1

#include <idu.h>
#include <smu.h>
#include <smDef.h>
#include <sddDiskMgr.h>

typedef struct sddFileStatFT
{
    scSpaceID       mSpaceID;
    sdFileID        mFileID;

    /* File I/O 통계정보 */
    iduFIOStat      mFileIOStat;

    /* I/O 평균 Time */
    ULong           mAvgIOTime;
    
} sddFileStatFT;

class sddDiskFT
{
    
public:
    // X$FILESTAT 
    static IDE_RC buildFT4FILESTAT(
			 idvSQL		     * /*aStatistics*/,
                         void                * aHeader,
                         void                * aDumpObj,
                         iduFixedTableMemory * aMemory );

    // X$DATAFILES
    static IDE_RC buildFT4DATAFILES(
			 idvSQL		     * /*aStatistics*/,
                         void                * aHeader,
                         void                * aDumpObj,
                         iduFixedTableMemory * aMemory );
    
    // XPAGE_SIZE
    static IDE_RC buildFT4PAGESIZE(
			 idvSQL		     * /*aStatistics*/,
                         void                 * aHeader,
                         void                 * aDumpObj,
                         iduFixedTableMemory  * aMemory );
    
};

#endif // _O_SDD_DISK_FT_H_
