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
* $Id: smmPLoadChild.cpp 82075 2018-01-17 06:39:52Z jina.kim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smm.h>
#include <smmManager.h>

// 파일의 내용을 로드하기 위해 필요한 정보를 설정한다.
// 인자에 대한 자세한 설명은 loadDbFile을 참고한다.
void smmPLoadChild::setFileToBeLoad(smmTBSNode *     aTBSNode,
                                    UInt             aFileNumber,
                                    scPageID         aFirstPID,
                                    scPageID         aLastPID,
                                    vULong           aLoadPageCount )
{
    mTBSNode        = aTBSNode;
    mFileNumber     = aFileNumber;
    mFirstPID       = aFirstPID;
    mLastPID        = aLastPID;
    mLoadPageCount  = aLoadPageCount;
}
    
IDE_RC smmPLoadChild::doJob()
{
    
    IDE_TEST( smmManager::loadDbFile(mTBSNode,
                                     mFileNumber,
                                     mFirstPID,
                                     mLastPID,
                                     mLoadPageCount ) != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    {
        
        ideLog::logErrorMsg(IDE_SERVER_0);
        
        IDE_CALLBACK_FATAL("Loading DataFile Failed");
    }
    
    return IDE_FAILURE;
}
