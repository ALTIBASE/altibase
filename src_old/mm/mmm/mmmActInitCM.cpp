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

#include <cm.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmtThreadManager.h>
#include <mmuProperty.h>


static IDE_RC mmmPhaseActionInitCM(mmmPhase        aPhase,
                                   UInt             /*aOptionflag*/,
                                   mmmPhaseAction * /*aAction*/)
{
    SChar sBuffer[MM_IP_VER_STRLEN_ON_BOOT];
    SInt  sImpl;
    UInt  sCmMaxPendingList;

    // BUG-19465 : CM_Buffer의 pending list를 제한
    sCmMaxPendingList = mmuProperty::getCmMaxPendingList();

    switch (aPhase)
    {
        case MMM_STARTUP_PRE_PROCESS:
            IDE_TEST(cmiInitialize(sCmMaxPendingList) != IDE_SUCCESS);

            /* BUG-44488 */
            if (mmuProperty::getSslEnable() == ID_TRUE) 
            {
                /* Load OpenSSL library dynamically */
                IDE_TEST(cmiSslInitialize() != IDE_SUCCESS);
            }

            /* PROJ-2681 */
            if (mmuProperty::getIBEnable() == ID_TRUE)
            {
                IDE_TEST(cmiIBInitialize() != IDE_SUCCESS);
            }

            IDE_TEST(mmtThreadManager::setupProtocolCallback() != IDE_SUCCESS);
            break;

        case MMM_STARTUP_PROCESS:
            break;

        case MMM_STARTUP_CONTROL:
            break;

        case MMM_STARTUP_META:
            break;

        case MMM_STARTUP_SERVICE:
            for (sImpl = CMI_LINK_IMPL_BASE; sImpl < CMI_LINK_IMPL_MAX; sImpl++)
            {
                if (mmtThreadManager::addListener((cmiLinkImpl)sImpl,
                                                  sBuffer,
                                                  ID_SIZEOF(sBuffer)) == IDE_SUCCESS)
                {
                    IDE_CALLBACK_SEND_SYM("  [CM] Listener started : ");
                    IDE_CALLBACK_SEND_MSG(sBuffer);
                }
                else
                {
                    if (ideGetErrorCode() != mmERR_IGNORE_UNABLE_TO_MAKE_LISTENER)
                    {
                        IDE_CALLBACK_SEND_SYM("  [CM] Listener failed  : ");
                        IDE_CALLBACK_SEND_MSG(sBuffer);

                        IDE_RAISE(AddListenerFail);
                    }
                }
            }
            break;

        case MMM_STARTUP_DOWNGRADE: /* PROJ-2689 */
            break;

        case MMM_STARTUP_SHUTDOWN:
        default:
            IDE_CALLBACK_FATAL("invalid startup phase");
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(AddListenerFail);

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitCM =
{
    (SChar *)"Initialize Communication Manager",
    0,
    mmmPhaseActionInitCM
};

