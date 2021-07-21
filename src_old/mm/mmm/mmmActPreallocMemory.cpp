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

#include <idp.h>
#include <mmm.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionPreallocMemory(mmmPhase         /*aPhase*/,
                                           UInt             /*aOptionflag*/,
                                           mmmPhaseAction * /*aAction*/)
{
    ULong  sSize = 0;
    void  *sPtr;

    IDE_ASSERT(idp::read("PREALLOC_MEMORY", &sSize) == IDE_SUCCESS);

    if (sSize > 0)
    {
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_OTHER, sSize, &sPtr) != IDE_SUCCESS);

        idlOS::memset(sPtr, 0, sSize);

        IDE_TEST(iduMemMgr::free(sPtr) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActPreallocMemory =
{
    (SChar *)"Preallocate Altibase Memory",
    0,
    mmmPhaseActionPreallocMemory
};
