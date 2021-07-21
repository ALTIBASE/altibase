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

#include <idl.h>
#include <iduMemPoolMgr.h>
#include <mmm.h>

#define MMM_MAKE_MEMPOOL_MGR ( (IDU_USE_MEMORY_POOL == 1)                 &&\
                               (iduMemMgr::isServerMemType() == ID_TRUE) )



/* =======================================================
 * Action Function
 * => Initialize Mempool Manager
 * =====================================================*/

static IDE_RC mmmPhaseActionInitMemPoolMgr(mmmPhase         /*aPhase*/,
                                           UInt             /*aOptionflag*/,
                                           mmmPhaseAction * /*aAction*/)
{
    /*
     * BUG-32751
     * Additional initialization of iduMemPoolMgr
     * Because this manager should be initialized after
     * the initialization of iduMutexMgr
     */
    if( MMM_MAKE_MEMPOOL_MGR == ID_TRUE )
    {
        IDE_ASSERT(iduMemPoolMgr::initialize() == IDE_SUCCESS); 
    }

    return IDE_SUCCESS;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitMemPoolMgr =
{
    (SChar *)"Initialize Altibase Memory Pool Manager",
    0,
    mmmPhaseActionInitMemPoolMgr
};

