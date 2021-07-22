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
 * $Id: smcTableBackupFile.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMC_TABLE_BACKUP_FILE_H_
#define _O_SMC_TABLE_BACKUP_FILE_H_ 1

#include <idu.h>
#include <smDef.h>

class iduFile;

class smcTableBackupFile
{
    
public:
    
    IDE_RC initialize( SChar *aStrFileName);
    
    IDE_RC destroy();

    IDE_RC open(idBool aCreate);
    
    IDE_RC close();

    IDE_RC read(ULong  aWhere,
                void  *aBuffer,
                SInt   aSize);

    IDE_RC write(ULong  aWhere,
                 void  *aBuffer,
                 SInt   aSize);

    IDE_RC     getFileSize(ULong *aSize) {return mFile.getFileSize(aSize);    };
    SChar*     getFileName(){return mFile.getFileName(); };
    smcTableBackupFile();
    virtual ~smcTableBackupFile();

//For Member
private:
    iduFile   mFile;
    SInt      mBufferSize;
    SInt      mFirstWriteOffset;
    SInt      mLastWriteOffset;
    
    ULong     mBeginPos;
    ULong     mEndPos;
    ULong     mFileSize;
    idBool    mOpen;
    idBool    mCreate;
    SChar*    mBuffer;
};

#endif
