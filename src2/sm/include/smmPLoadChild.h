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
 * $Id: smmPLoadChild.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMM_PLOAD_CHILD_H_
#define _O_SMM_PLOAD_CHILD_H_ 1

#include <idu.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>
#include <smtPJChild.h>

/* ---------------------------------------------------------------
 *  Parallel Load CHILD Impl.
 * -------------------------------------------------------------*/

class smmPLoadChild : public smtPJChild
{
    smmTBSNode *     mTBSNode;
    UInt             mFileNumber;
    scPageID         mFirstPID;
    scPageID         mLastPID;
    ULong            mLoadPageCount;
    smmRestoreOption mRestoreOption;

    
public:
    smmPLoadChild() {}
    
    // 파일의 내용을 로드하기 위해 필요한 정보를 설정한다.
    // 인자에 대한 자세한 설명은 loadDbFile을 참고한다.
    void setFileToBeLoad(smmTBSNode *     aTBSNode,
                         UInt             aFileNumber,
                         scPageID         aFirstPID,
                         scPageID         aLastPID,
                         vULong           aLoadPageCount );
    
    IDE_RC doJob();
};



#endif /* _O_SMM_PLOAD_CHILD_H_ */
