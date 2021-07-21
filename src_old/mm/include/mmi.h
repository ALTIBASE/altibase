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

#ifndef _O_MMI_H_
#define _O_MMI_H_ 1

#include <mmtServiceThread.h>
#include <mmtThreadManager.h>
#include <mmtAdminManager.h>
#include <mmm.h>

extern const SChar *ulcGetClientType();

#define MMI_ERROR_MESSAGE_SIZE  256

#define DEBUG_MASK              (0x10000000)
#define DEBUG_TRUE              (0x10000000)
#define DEBUG_FALSE             (0x00000000)

#define SIGNAL_MASK             (0x20000000)
#define SIGNAL_TRUE             (0x20000000)
#define SIGNAL_FALSE            (0x00000000)

#define STARTUP_PROCESS         (2)
#define STARTUP_SERVICE         (5)

#define MMI_DEBUG_MASK          DEBUG_MASK
#define MMI_DEBUG_TRUE          DEBUG_TRUE
#define MMI_DEBUG_FALSE         DEBUG_FALSE

#define MMI_SIGNAL_MASK         SIGNAL_MASK
#define MMI_SIGNAL_TRUE         SIGNAL_TRUE
#define MMI_SIGNAL_FALSE        SIGNAL_FALSE

#define MMI_DAEMON_MASK         (0x01000000)
#define MMI_DAEMON_TRUE         (0x01000000)
#define MMI_DAEMON_FALSE        (0x00000000)

#define MMI_INNER_MASK          (0x02000000)
#define MMI_INNER_TRUE          (0x02000000)
#define MMI_INNER_FALSE         (0x00000000)

#define MMI_STARTUP_PROCESS     STARTUP_PROCESS 
#define MMI_STARTUP_SERVICE     STARTUP_SERVICE

class mmiServer: public idtBaseThread
{
public:
    mmiServer( mmmPhase aPhase ) 
    { 
        mPhase   = aPhase; 
        mStatus  = 0;
    };

    ~mmiServer() {};

    void               run();

    IDE_RC             initialize();

    IDE_RC             finalize();

    IDE_RC             wait();

    IDE_RC             post();

    SInt               status() { return mStatus; };

private:
    mmmPhase           mPhase;
    idBool             mPost;
    SInt               mStatus;
};

class mmi
{
public:
    static UInt    getServerOption() { return mServerOption; }

    static void    setError( SInt aErrorCode, const SChar * aErrorMessage );

    // server control api
    static SInt    serverStart( SInt aPhase, SInt aOption );

    static SInt    serverStop();

    static SInt    isStarted();

    static SInt    getErrorCode();

    static SChar * getErrorMessage();

private:
    static mmiServer     * mServer;
    static UInt            mServerOption;

    static SChar           mErrorMessage[MMI_ERROR_MESSAGE_SIZE];
    static SInt            mErrorCode;
};

#endif
