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
#include <mmErrorCode.h>
#include <mmuProperty.h>


static const SChar *mmiMsbList[] =
{
    "ID",
    "SM",
    "MT",
    "QP",
    "SD",
    "MM",
    "RP",
    "CM",
    "ST",
    "DK",
    NULL
};

static IDE_RC  mainLoadErrorMsb(SChar *aRootDir, SChar *aIDN)
{

    SChar filename[512];
    UInt  i;

    for (i = 0; mmiMsbList[i] != NULL; i++)
    {
        idlOS::memset(filename, 0, ID_SIZEOF(filename));
        idlOS::snprintf(filename, ID_SIZEOF(filename), "%s%cmsg%cE_%s_%s.msb",
                        aRootDir,
                        IDL_FILE_SEPARATOR,
                        IDL_FILE_SEPARATOR,
                        mmiMsbList[i],
                        aIDN);
        IDE_TEST_RAISE(ideRegistErrorMsb(filename) != IDE_SUCCESS,
                   load_msb_error);
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(load_msb_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_LOAD_MSB_ERROR, filename));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmPhaseActionLoadMsb(mmmPhase         /*aPhase*/,
                                    UInt             /*aOptionflag*/,
                                    mmmPhaseAction * /*aAction*/)
{
    IDE_TEST (mainLoadErrorMsb(idp::getHomeDir(),
                               (SChar *)"US7ASCII") != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActLoadMsb =
{
    (SChar *)"Loading Error Message Binary File",
    0,
    mmmPhaseActionLoadMsb
};

