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

#include <mmm.h>
#include <mmmActList.h>


/* =======================================================
 *  Action Descriptor
 * =====================================================*/
static mmmPhaseAction *gPreProcessActions[] =
{
    &gMmmActBeginPreProcess,

    &gMmmActCheckShellEnv,

    &gMmmActInitBootMsgLog,

    &gMmmActLogRLimit,

    &gMmmActProperty,

    &gMmmActCheckLicense,

    &gMmmActSignal,

    /* 
     * BUG-32751
     * Memory Manager에서 더 이상 Mutex를 사용하지 않으며
     * iduMemPool에서 server_malloc을 사용하기 때문에 위치를 변경합니다.
     */
    &gMmmActInitMemoryMgr,

    &gMmmActCheckIdeTSD, // Ide Layer 초기화 검사, mutex, latch 초기화

    &gMmmActInitMemPoolMgr,

    &gMmmActLoadMsb,

    &gMmmActInitModuleMsgLog,


    &gMmmActCallback,

    &gMmmActVersionInfo,
#if !defined(ANDROID) && !defined(SYMBIAN)
    &gMmmActSystemUpTime,
#endif
    &gMmmActDaemon,
    &gMmmActThread,
    &gMmmActPIDInfo,
    &gMmmActInitLockFile,    // Daemonize 이후에 사용하도록 함.

    /* TASK-6780 */
    &gMmmActInitRBHash,

//     &gMmmActInitGPKI,
    &gMmmActInitSM, // todo
    &gMmmActInitQP, // todo
    &gMmmActInitMT,
    &gMmmActFixedTable,
    &gMmmActTimer,
    &gMmmActInitCM,       // Initialize CM
    &gMmmActInitDK,
    &gMmmActInitThreadManager,
    &gMmmActInitOS,
    // &gMmmActInitNLS,
    &gMmmActInitQueryProfile,
    &gMmmActInitSD,
    &gMmmActInitREPL,
    &gMmmActInitService,

    &gMmmActEndPreProcess,
    NULL /* should exist!! */
};

/* =======================================================
 *  Process Phase Descriptor
 * =====================================================*/
mmmPhaseDesc gMmmGoToPreProcess =
{
    MMM_STARTUP_PRE_PROCESS,
    (SChar *)"PRE-PROCESSING",
    gPreProcessActions
};
