/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id: iduMutexEntryPOSIX.cpp 15077 2006-02-09 02:00:43Z sjkim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduMemPool.h>
#include <iduMutexEntry.h>
#include <idp.h>

static IDE_RC iduPosixInitializeStatic()
{
    iduMutexEntry sDummy;

    IDE_ASSERT( ID_SIZEOF(PDL_thread_mutex_t) <= ID_SIZEOF(sDummy.mMutex));

    return IDE_SUCCESS;
}

static IDE_RC iduPosixDestroyStatic()
{
    return IDE_SUCCESS;
}

static IDE_RC iduPosixCreate(void* /*aRsc*/)
{
    return IDE_SUCCESS;
}

static IDE_RC iduPosixInitialize(void *aRsc, UInt /*aBusyValue*/)
{
    PDL_thread_mutex_t *sMutex;

    sMutex = (PDL_thread_mutex_t *)aRsc;

    IDE_TEST_RAISE(idlOS::thread_mutex_init(sMutex, USYNC_THREAD, 0)
                   != 0, mutex_init_error);


    return IDE_SUCCESS;
    IDE_EXCEPTION(mutex_init_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode (idERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC iduPosixFinalize(void *aRsc)
{
    PDL_thread_mutex_t *sMutex;

    sMutex = (PDL_thread_mutex_t *)aRsc;

    IDE_TEST_RAISE(idlOS::thread_mutex_destroy(sMutex) != 0,
                   m_mutexdestroy_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(m_mutexdestroy_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode (idERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC iduPosixDestroy(void* /*aRsc*/)
{
    return IDE_SUCCESS;
}

static void iduPosixTryLock(void         * aRsc, 
                            idBool       * aRet, 
                            iduMutexStat * /*aStat*/)
{
    if (idlOS::thread_mutex_trylock((PDL_thread_mutex_t *)aRsc) == 0)
    {
        *aRet = ID_TRUE;
    }
    else
    {
        IDE_DASSERT(errno == EBUSY);
        *aRet = ID_FALSE;
    }
}

static void iduPosixLock(void         * aRsc,
                         iduMutexStat * /*aStat*/,
                         void         * /*aStatSQL*/,
                         void         * /*aWeArgs*/ )
{
    if (idlOS::thread_mutex_trylock((PDL_thread_mutex_t *)aRsc) != 0)
    {
        IDE_DASSERT(errno == EBUSY);
        
        IDE_ASSERT(idlOS::thread_mutex_lock((PDL_thread_mutex_t *)aRsc) == 0);
    }
}

static void iduPosixUnlock(void *aRsc, iduMutexStat */*aStat*/)
{
    IDE_ASSERT(idlOS::thread_mutex_unlock((PDL_thread_mutex_t *)aRsc) == 0);
}

//fix PROJ-1749
iduMutexOP gPosixMutexClientOps =
{
    iduPosixInitializeStatic,
    iduPosixDestroyStatic,
    iduPosixCreate,
    iduPosixInitialize,
    iduPosixFinalize,
    iduPosixDestroy,
    iduPosixLock,
    iduPosixTryLock,
    iduPosixUnlock
};
