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
 * $Id: smmPLoadMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMM_PLOAD_MGR_H_
#define _O_SMM_PLOAD_MGR_H_ 1


#include <idl.h>
#include <idu.h>
#include <idp.h>
#include <smDef.h>
#include <smmDef.h>
#include <smtPJMgr.h>
#include <smmPLoadChild.h>

/* ---------------------------------------------------------------
 *  Parallel Write Manager Impl.
 * -------------------------------------------------------------*/

class smmPLoadMgr : public smtPJMgr
{
public:
    SInt              mThreadCount;
    smmPLoadChild  ** mChildArray;
    smmTBSNode *      mTBSNode;
    SInt              mCurrentDB;
    UInt              mCurrFileNumber;
    UInt              mMaxFileNumber;
    idBool            mSuccess;
    scPageID          mStartReadPageID;
    
    
public:
    smmPLoadMgr() : smtPJMgr() {}
    IDE_RC initializePloadMgr(smmTBSNode *     aTBSNode,
                              SInt             aThreadCount);
    
    IDE_RC assignJob(SInt    aReqChild,
                     idBool *aJobAssigned);
    IDE_RC destroy();
};

#endif /* _O_SMM_PLOAD_MGR_H_ */
