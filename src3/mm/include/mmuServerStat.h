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

#ifndef _O_MMU_SERVERSTAT_H_
#define _O_MMU_SERVERSTAT_H_ 1

#include <idl.h>

class mmuServerStat
{
    SChar mFileName[512];
    idlOS::pdl_flock_t mLockFile;
    
    IDE_RC tryToHold(idBool *aFlag);
    
public:
    IDE_RC initialize();

    idBool isFileExist();
    //fix BUG-17545
    IDE_RC createLockFile(idBool* aRetry);
    /* BUG 18294 */
    IDE_RC initFileLock();
    IDE_RC destFileLock();
    IDE_RC destroy(); 
    
    IDE_RC hold();
    IDE_RC tryhold();
    IDE_RC release();
    
    IDE_RC checkServerRunning(idBool *aRunningFlag); // try & get flag
    SChar* getFileName() { return mFileName; }
};

#endif
