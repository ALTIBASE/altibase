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

#ifndef _O_MMM_H_
#define  _O_MMM_H_  1

#include <idl.h>
#include <idu.h>
#include <ide.h>
#include <smiDef.h>
#include <mmuServerStat.h>
#include <mmmDef.h>


class mmm
{
    static mmmPhase              mPhase;
    static mmmPhaseDesc        **mPhaseDesc;
    static UChar                 mTransitionMatrix[MMM_STARTUP_MAX][MMM_STARTUP_MAX];
    static SInt                  mServerStatus;
    static UInt                  mEmergencyFlag;

    static idBool   canTransit(mmmPhase aNextPhase);
    static IDE_RC   executeInternal(mmmPhaseDesc *aDesc, UInt aOptionflag);
public:

    static smiGlobalCallBackList mSmiGlobalCallBackList;
    static PDL_Time_Value        mStartupTime;
    static mmuServerStat         mCheckStat;

    static void     setEmergencyFlag(UInt aFlag) { mEmergencyFlag |= aFlag; }
    static void     clrEmergencyFlag(UInt aFlag) { mEmergencyFlag &= aFlag; }
    static UInt     getEmergencyFlag()           { return  mEmergencyFlag; }
    static void     setServerStatus(SInt aFlag)  { mServerStatus = aFlag; }
    static SInt     getServerStatus()            { return  mServerStatus; }

    static IDE_RC   execute(mmmPhase, UInt aOptionflag);

    static mmmPhase getCurrentPhase() { return mPhase; }

    // Shutdown Preparing
    static void prepareShutdown(SInt aMode, idBool aNewFlag = ID_FALSE);

    // Write Message to system logging.
    static void     logToSystem(mmmPhase, mmmPhaseAction *);

    // for fixed table.
    static IDE_RC buildRecordForInstance(idvSQL              * /*aStatistics*/,
                                         void        *aHeader,
                                         void        *aDumpObj,
                                         iduFixedTableMemory *aMemory);
};

#endif  /* _O_MMM_SESSION_H_ */
