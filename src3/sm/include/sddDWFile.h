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
 * Description :
 **********************************************************************/

#ifndef _O_SDD_DWFILE_H_
#define _O_SDD_DWFILE_H_ 1

#include <idu.h>
#include <idv.h>
#include <sdbDef.h>

class sddDWFile
{
public:
    IDE_RC create(UInt aFileID, UInt aPageSize, UInt aPageCount, sdLayerState aType);

    IDE_RC load(SChar *aFileName, idBool *aRightDWFile);

    IDE_RC destroy();

    IDE_RC write(idvSQL *aStatistics,
                 UChar  *aIOB,
                 UInt    aPageCount);

    IDE_RC read(idvSQL *aStatistics,
                SChar  *aIOB,
                UInt    aPageIndex);

    UInt getPageSize();

    UInt getPageCount();

    /* BUG-27776 the server startup can be fail since the dw file is 
     * removed after DW recovery. 
     * DWFile을 지우는 대신 Reset합니다.*/
    IDE_RC reset()
    {
        /* writeHeader를 하면 Header를 기록하면서 File을 reset합니다.*/
        return writeHeader();
    }

private:
    IDE_RC writeHeader();

    void readHeader(idBool *aRightDWFile);

private:
    iduFile  mFile;
    UInt     mFileID;
    UInt     mPageSize;
    UInt     mPageCount;
    idBool   mFileInitialized;
    idBool   mFileOpened;
};  

inline UInt sddDWFile::getPageSize()
{
    return mPageSize;
}

inline UInt sddDWFile::getPageCount()
{
    return mPageCount;
}

#endif // _O_SDD_DWFILE_H_

