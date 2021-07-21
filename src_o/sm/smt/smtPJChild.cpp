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
 * $Id: smtPJChild.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idtBaseThread.h>
#include <smErrorCode.h>
#include <smtPJMgr.h>
#include <smtPJChild.h>


smtPJChild::smtPJChild() : idtBaseThread(IDT_DETACHED)
{

}

IDE_RC smtPJChild::initialize(smtPJMgr   *aMgr,
                              UInt        aNumber)
{
    mNumber    = aNumber;
    mMgr       = aMgr;
    mStatus    = SMT_PJ_SIGNAL_NONE;
    mErrorNo   = 0;
    
    return IDE_SUCCESS;
}

IDE_RC smtPJChild::destroy()
{

    return IDE_SUCCESS;

}

IDE_RC smtPJChild::getJob(idBool *aJobAssigned)
{
    UInt sState = 0;

    IDE_DASSERT(aJobAssigned != NULL);

    IDE_TEST(mMgr->lock() != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(mMgr->assignJob(mNumber, aJobAssigned));

    sState = 0;
    IDE_TEST(mMgr->unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState == 1)
    {
        IDE_ASSERT(mMgr->unlock() == IDE_SUCCESS);
    }

    mErrorNo = ideGetErrorCode();
    
    return IDE_FAILURE;
}

void   smtPJChild::run()
{
    idBool sJobAssigned = ID_FALSE;

    setStatus(SMT_PJ_SIGNAL_RUNNING);

    while(ID_TRUE)
    {
        IDE_TEST_RAISE(getJob(&sJobAssigned) != IDE_SUCCESS,
                       get_job_error);
        if (sJobAssigned == ID_FALSE)
        {
            break;
        }
        IDE_TEST_RAISE(doJob() != IDE_SUCCESS, job_error);
    }

    setStatus(SMT_PJ_SIGNAL_EXIT);

    return;
    
    IDE_EXCEPTION(job_error);
    {
//        IDE_CALLBACK_FATAL("job_error");
    }
    IDE_EXCEPTION(get_job_error);
    {
//        IDE_CALLBACK_FATAL("get_job_error");
    }
    IDE_EXCEPTION_END;
    {
        setStatus(SMT_PJ_SIGNAL_ERROR | SMT_PJ_SIGNAL_EXIT);
    }

    mErrorNo = ideGetErrorCode();

}

