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
 * $Id:$
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smr.h>

/* BUG-35392 */
smrUCSNChkThread::smrUCSNChkThread() : idtBaseThread()
{

}

smrUCSNChkThread::~smrUCSNChkThread()
{

}

IDE_RC smrUCSNChkThread::initialize()
{
    mIntervalUSec = smuProperty::getUCLSNChkThrInterval();
    mFinish   = ID_FALSE;

    IDE_TEST( start() != IDE_SUCCESS );
    IDE_TEST( waitToStart(0) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrUCSNChkThread::destroy( )
{
    IDE_TEST( shutdown( ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smrUCSNChkThread::run()
{
    PDL_Time_Value    sIntervalTime;

    sIntervalTime.set( 0, mIntervalUSec );

    while( mFinish == ID_FALSE )
    {
        smrLogMgr::rebuildMinUCSN();

        idlOS::sleep( sIntervalTime );
    }

    return ;
}

IDE_RC smrUCSNChkThread::shutdown( )
{
    mFinish = ID_TRUE;

    IDE_TEST_RAISE( join() != IDE_SUCCESS, err_thr_join );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_thr_join);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

