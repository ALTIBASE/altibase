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

#ifndef _O_MMT_ADMIN_MANAGER_H_
#define _O_MMT_ADMIN_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <mmcSession.h>
#include <mmcTask.h>


class mmtAdminManager
{
private:
    static mmcTask   *mTask;
    static iduMutex   mMutex;
    // bug-24366: sendMsgService mutex invalid
    // mutex가 destroy 되었는지 확인할 수 있는 flag 추가.
    static idBool     mMutexEnable;

public:
    static IDE_RC     initialize();
    static IDE_RC     finalize();

    static IDE_RC     refreshUserInfo();

    static IDE_RC     sendMsgPreProcess(const SChar *aMessage, SInt aCRFlag, idBool aLogMsg);
    static IDE_RC     sendMsgService(const SChar *aMessage, SInt aCRFlag, idBool aLogMsg);
    static IDE_RC     sendMsgConsole(const SChar *aMessage, SInt aCRFlag, idBool aLogMsg);

    static IDE_RC     sendNChar();

    static void       waitForAdminTaskEnd();

    static IDE_RC     setTask(mmcTask *aTask);
    static IDE_RC     unsetTask(mmcTask *aTask);

    static mmcTask   *getTask();

    static idBool     isConnected();

    static idBool     isConnectedViaIPC();

    static mmcSessID  getSessionID();
};


inline mmcTask *mmtAdminManager::getTask()
{
    return mTask;
}

inline idBool mmtAdminManager::isConnected()
{
    return (mTask != NULL) ? ((mTask->getSession() != NULL) ? ID_TRUE : ID_FALSE) : ID_FALSE;
}

/* 
 * BUG-43654 IPC Normal users must not be connected to the server
 *           in case of IPC_CHANNEL_COUNT is 0.
 */
inline idBool mmtAdminManager::isConnectedViaIPC()
{
    idBool sResult = ID_FALSE;

    if (isConnected() == ID_TRUE)
    {
        if (mTask->getLink()->mImpl == CMN_LINK_IMPL_IPC)
        {
            sResult = ID_TRUE;
        }
    }

    return sResult;
}

inline mmcSessID mmtAdminManager::getSessionID()
{
    return (mTask != NULL) ? mTask->getSession()->getSessionID() : 0;
}


#endif
