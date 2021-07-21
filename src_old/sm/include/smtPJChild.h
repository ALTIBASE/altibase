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
 * $Id: smtPJChild.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMT_PJ_CHILD_H_    // Parallel Job Child
#define _O_SMT_PJ_CHILD_H_ 1

#include <idl.h>
#include <ideErrorMgr.h>
#include <idtBaseThread.h>

// signal section 
#define SMT_PJ_SIGNAL_NONE     (0x00000001) // not Running : Just destroyed
#define SMT_PJ_SIGNAL_EXIT     (0x00000002) // Normal Á¾·á : to be join..
#define SMT_PJ_SIGNAL_ERROR    (0x00000004) // Error 
#define SMT_PJ_SIGNAL_RUNNING  (0x00000008) // Running : Now Running
#define SMT_PJ_SIGNAL_SLEEP    (0x00000010) // sleep

class smtPJMgr;

class smtPJChild : public idtBaseThread
{
    friend class smtPJMgr;
protected:
    UInt        mNumber;
    UInt        mStatus;
    smtPJMgr   *mMgr;
    UInt        mErrorNo;

public:
    smtPJChild();
    IDE_RC initialize( smtPJMgr   *aMgr,
                       UInt        aNumber);
    IDE_RC destroy();
    IDE_RC getJob(idBool *aJobAssigned);
    UInt   getErrorNo() { return mErrorNo; }
    UInt   getStatus() { return mStatus; }
    void   setStatus(UInt aStatus) { mStatus = aStatus; }
    virtual void   run();
    virtual IDE_RC   doJob() = 0;// called when job is done!
};


#endif
