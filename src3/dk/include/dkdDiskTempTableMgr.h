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

#ifndef _O_DKD_DISK_TEMP_TABLE_MGR_H_
#define _O_DKD_DISK_TEMP_TABLE_MGR_H_ 1


#include <dkd.h>
#include <dkdDiskTempTable.h>


IDE_RC  fetchRowFromDiskTempTable( void  *aMgrHandle,  void    **aRow );
IDE_RC  insertRowIntoDiskTempTable( void  *aMgrHandle, dkdRecord *aRecord );
IDE_RC  restartDiskTempTable( void  *aMgrHandle );

class dkdDiskTempTableMgr
{
private:
    idBool                      mIsEndOfFetch;   /* check fetch row end */
    idBool                      mIsCursorOpened; /* check cursor opened */
    dkdDiskTempTableHandle    * mDiskTempTableH; /* disk temp table handle */

public:
    /* Initialize / Finalize */
    IDE_RC       initialize( void       *aQcStatement,
                             mtcColumn  *aColMetaArr,
                             UInt        aColCnt );
    IDE_RC       finalize();

    /* Operations */
    IDE_RC       fetchRow( void **aRow );
    IDE_RC       insertRow( dkdRecord   *aRecord );
    IDE_RC       restart();
};

#endif /* _O_DKD_DISK_TEMP_TABLE_MGR_H_ */
