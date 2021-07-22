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

#ifndef _O_MMM_ACT_LIST_H_
#define _O_MMM_ACT_LIST_H_  1

/*
 * MMM Action Function List
 */

extern mmmPhaseAction gMmmActCheckShellEnv;
extern mmmPhaseAction gMmmActCallback;
extern mmmPhaseAction gMmmActInitBootMsgLog;
extern mmmPhaseAction gMmmActLogRLimit;
extern mmmPhaseAction gMmmActCheckIdeTSD;
extern mmmPhaseAction gMmmActProperty;
extern mmmPhaseAction gMmmActInitModuleMsgLog;
extern mmmPhaseAction gMmmActInitMemoryMgr;
extern mmmPhaseAction gMmmActInitMemPoolMgr;
extern mmmPhaseAction gMmmActVersionInfo;
extern mmmPhaseAction gMmmActSystemUpTime;
extern mmmPhaseAction gMmmActDaemon;
extern mmmPhaseAction gMmmActThread;
extern mmmPhaseAction gMmmActPIDInfo;
extern mmmPhaseAction gMmmActPreallocMemory;
extern mmmPhaseAction gMmmActLoadMsb;
extern mmmPhaseAction gMmmActInitThreadManager;
extern mmmPhaseAction gMmmActInitLockFile;
extern mmmPhaseAction gMmmActWaitAdmin;
extern mmmPhaseAction gMmmActCheckLicense;
extern mmmPhaseAction gMmmActInitOS;
extern mmmPhaseAction gMmmActInitNLS;
extern mmmPhaseAction gMmmActSignal;
extern mmmPhaseAction gMmmActInitGPKI;
extern mmmPhaseAction gMmmActInitMT;
extern mmmPhaseAction gMmmActInitSM;
extern mmmPhaseAction gMmmActInitQP;
extern mmmPhaseAction gMmmActInitSecurity; // PROJ-2002
extern mmmPhaseAction gMmmActInitCM;
extern mmmPhaseAction gMmmActInitDK;
extern mmmPhaseAction gMmmActInitRC;
extern mmmPhaseAction gMmmActInitRCExecuter;
extern mmmPhaseAction gMmmActInitService;
extern mmmPhaseAction gMmmActInitXA;
extern mmmPhaseAction gMmmActInitQueryProfile;
extern mmmPhaseAction gMmmActInitSNMP;
extern mmmPhaseAction gMmmActInitREPL;
extern mmmPhaseAction gMmmActTimer;
extern mmmPhaseAction gMmmActInitAudit; /* PROJ-2223 Altibase Auditing */
extern mmmPhaseAction gMmmActInitJobManager; /* PROJ-1438 Job Scheduler */
extern mmmPhaseAction gMmmActInitSnapshotExport; /* PROJ-2626 Snapshot Export */
extern mmmPhaseAction gMmmActInitSD;

// shutdown
extern mmmPhaseAction gMmmActShutdownPrepare;
extern mmmPhaseAction gMmmActShutdownSNMP;
extern mmmPhaseAction gMmmActShutdownService;
extern mmmPhaseAction gMmmActShutdownXA;
extern mmmPhaseAction gMmmActShutdownQueryProfile;
extern mmmPhaseAction gMmmActShutdownREPL;
extern mmmPhaseAction gMmmActShutdownFixedTable;

extern mmmPhaseAction gMmmActShutdownQP;
extern mmmPhaseAction gMmmActShutdownSecurity; // PROJ-2002
extern mmmPhaseAction gMmmActShutdownSM;
extern mmmPhaseAction gMmmActShutdownMT;
extern mmmPhaseAction gMmmActShutdownCM;
extern mmmPhaseAction gMmmActShutdownDK;
extern mmmPhaseAction gMmmActShutdownGPKI;
extern mmmPhaseAction gMmmActShutdownEND;
extern mmmPhaseAction gMmmActShutdownTimer;
extern mmmPhaseAction gMmmActShutdownRC;
extern mmmPhaseAction gMmmActShutdownProperty;
extern mmmPhaseAction gMmmActShutdownSD;

// Descriptor Begin & End Actions
extern mmmPhaseAction gMmmActBeginPreProcess;
extern mmmPhaseAction gMmmActBeginProcess;
extern mmmPhaseAction gMmmActBeginControl;
extern mmmPhaseAction gMmmActBeginMeta;
extern mmmPhaseAction gMmmActBeginService;
extern mmmPhaseAction gMmmActBeginShutdown;
extern mmmPhaseAction gMmmActBeginDowngrade;

extern mmmPhaseAction gMmmActEndPreProcess;
extern mmmPhaseAction gMmmActEndProcess;
extern mmmPhaseAction gMmmActEndControl;
extern mmmPhaseAction gMmmActEndMeta;
extern mmmPhaseAction gMmmActEndService;
extern mmmPhaseAction gMmmActEndShutdown;
extern mmmPhaseAction gMmmActFixedTable;
extern mmmPhaseAction gMmmActEndDowngrade;


extern mmmPhaseAction gMmmActInitRBHash;

#endif
