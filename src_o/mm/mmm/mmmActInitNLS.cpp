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
#include <idn.h>
#include <mmm.h>
#include <mtd.h>


static IDE_RC mmmPhaseActionInitNLS(mmmPhase         /*aPhase*/,
                                    UInt             /*aOptionflag*/,
                                    mmmPhaseAction * /*aAction*/)
{
    // PROJ-1361
    //IDE_TEST(idnSetDefaultFactory(mmuProperty::getNLS()) != IDE_SUCCESS);

    // PROJ-1579 NCHAR
    // mtdModule.column->language 정보를 다시 세팅해준다.
    IDE_TEST( mtd::modifyNls4MtdModule() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitNLS =
{
    (SChar *)"Initialize NLS System",
    0,
    mmmPhaseActionInitNLS
};

 
