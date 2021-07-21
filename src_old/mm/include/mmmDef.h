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

#ifndef _O_MMM_DEF_H_
#define _O_MMM_DEF_H_ 1

/*
 * MMM [M]ain [M]odule [M]ultiple Phase Startup
 */

#ifndef ALTIBASE_STATUS_BOOTUP
#define ALTIBASE_STATUS_BOOTUP             0
#define ALTIBASE_STATUS_RUNNING            1
#define ALTIBASE_STATUS_SHUTDOWN_NORMAL    2
#define ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE 3
#define ALTIBASE_STATUS_SHUTDOWN_ERROR     4
#define ALTIBASE_STATUS_MAX_MAX_MAX        5
#endif

#define MMM_VERSION_STRLEN              (128)

typedef enum
{
    MMM_STARTUP_INIT        = IDU_STARTUP_INIT,
    MMM_STARTUP_PRE_PROCESS = IDU_STARTUP_PRE_PROCESS,
    MMM_STARTUP_PROCESS     = IDU_STARTUP_PROCESS,
    MMM_STARTUP_CONTROL     = IDU_STARTUP_CONTROL,
    MMM_STARTUP_META        = IDU_STARTUP_META,
    MMM_STARTUP_SERVICE     = IDU_STARTUP_SERVICE,
    MMM_STARTUP_SHUTDOWN    = IDU_STARTUP_SHUTDOWN,
    MMM_STARTUP_DOWNGRADE   = IDU_STARTUP_DOWNGRADE, /* PROJ-2689 */

    MMM_STARTUP_MAX         = IDU_STARTUP_MAX,    /* 8 */
} mmmPhase;

struct mmmPhaseAction;

typedef IDE_RC (*mmmPhaseActionFunc)(mmmPhase, UInt, mmmPhaseAction *);

#define MMM_ACTION_NO_LOG  0x00000001 /* 로깅 조차 할 수 없는 경우 */

typedef struct mmmPhaseAction
{
    SChar              *mMessage;
    UInt                mFlag;
    mmmPhaseActionFunc  mFunction;
} mmmPhaseAction;

typedef struct mmmPhaseDesc
{
    mmmPhase         mPhase;
    SChar           *mMessage;
    mmmPhaseAction **mActions;

} mmmPhaseDesc;

// for fixed table.
typedef struct mmmVersionInfo
{
    const SChar *mProdVersion;
    SChar        mPkgPlatFormInfo[MMM_VERSION_STRLEN + 1];
    SChar        mProductTime[MMM_VERSION_STRLEN + 1];
    const SChar *mSmVersion;
    SChar        mMetaVersion[MMM_VERSION_STRLEN + 1];
    SChar        mProtocolVersion[MMM_VERSION_STRLEN + 1];
    SChar        mReplProtocolVersion[MMM_VERSION_STRLEN + 1];
} mmmVersionInfo;

typedef struct mmmInstance4PerfV
{
    mmmPhase   mStartupPhase;
    SLong      mStartupTimeSec;
    SLong      mWorkingTimeSec;
} mmmInstance4PerfV;

#endif
