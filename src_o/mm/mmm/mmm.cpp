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
#include <mtc.h>
#include <qci.h>
#include <mmm.h>
#include <mmcSession.h>
#include <mmErrorCode.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>
#include <mmtAuditManager.h>

extern mmmPhaseDesc gMmmGoToInit;
extern mmmPhaseDesc gMmmGoToPreProcess;
extern mmmPhaseDesc gMmmGoToProcess;
extern mmmPhaseDesc gMmmGoToControl;
extern mmmPhaseDesc gMmmGoToMeta;
extern mmmPhaseDesc gMmmGoToService;
extern mmmPhaseDesc gMmmGoToShutdown;
extern mmmPhaseDesc gMmmGoToDowngrade;


static mmmPhaseDesc *mDescList[] =
{
    &gMmmGoToInit,
    &gMmmGoToPreProcess,
    &gMmmGoToProcess,
    &gMmmGoToControl,
    &gMmmGoToMeta,
    &gMmmGoToService,
    &gMmmGoToShutdown,
    &gMmmGoToDowngrade,


    NULL
};

mmmPhase              mmm::mPhase     = MMM_STARTUP_INIT;
mmmPhaseDesc        **mmm::mPhaseDesc = mDescList;
SInt                  mmm::mServerStatus;
UInt                  mmm::mEmergencyFlag;
PDL_Time_Value        mmm::mStartupTime;
mmuServerStat         mmm::mCheckStat;

static void mmiSetEmergencyFlag(UInt aSetFlag)
{
    mmm::setEmergencyFlag(aSetFlag); }

static void mmiClrEmergencyFlag(UInt aClrFlag)
{
    mmm::clrEmergencyFlag(aClrFlag);
}

smiGlobalCallBackList mmm::mSmiGlobalCallBackList =
{
    mtc::findCompare,
    mtc::findKey2String,
    mtc::findNull,
    mtc::findStoredValue2MtdValue,
    mtc::findStoredValue2MtdValue4DataType,
    mtc::findActualSize,
    mtc::findHashKeyFunc,
    mtc::findIsNull,
    mtc::getAlignValue,
    mtc::getValueLengthFromFetchBuffer,
    mmtThreadManager::threadSleepCallback,
    mmtThreadManager::threadWakeupCallback,
    mmiSetEmergencyFlag,
    mmiClrEmergencyFlag,
    mmtSessionManager::getBaseTime,
    NULL,/*qdc::setExecDDLdisableProperty BUGBUG : What? 사리짐..*/
    qciMisc::makeNullRow,
    qciMisc::smiCallbackCheckNeedUndoRecord, // BUG-21895
    mmcSession::getSessionUpdateMaxLogSizeCallback,
    mmcSession::getSessionSqlText,
    // TASK-3171 B-Tree for spatial
    mtc::getNonStoringSize,
    // Proj-2059 DB Upgrade 기능
    NULL, /*qciMisc::getColumnHeaderDesc*/
    NULL, /*qciMisc::getTableHeaderDesc*/
    NULL, /*qciMisc::getPartitionHeaderDesc*/
    NULL, /*qciMisc::getColumnMapFromColumnHeader*/
    // BUG-37484
    mtc::needMinMaxStatistics,
    mtc::getColumnStoreLen,
    mtd::isUsablePartialDirectKeyIndex,
    mmcSession::getDDLLockTimeout
};

/*
 * 각 단계에서 다음 단계로 갈 때 허용 여부
 * 0일 경우 허용 안됨. 1일 경우 허용됨.
 */
UChar mmm::mTransitionMatrix[MMM_STARTUP_MAX][MMM_STARTUP_MAX] =
{
    /* INIT */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  1,  1,   1,   1,   1,   0,   1
    },
    /* PRE-PROCESS */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  1,   1,   1,   1,   0,   1
    },
    /* PROCESS */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  0,   1,   1,   1,   0,   1
    },
    /* CONTROL */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  0,   0,   1,   1,   0,   1
    },
    /* OPEN_META */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  0,   0,   0,   1,   0,   1
    },
    /* SERVICE */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  0,   0,   0,   0,   1,   0
    },
    /* SHUTDOWN */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  0,   0,   0,   0,   0,   0
    },
    /* DOWNGRADE */
    { /* INIT PRE PROC CONT META SERV SHUT DOWN */
           0,  0,  0,   0,   0,   0,   0,   0
    }
};

idBool   mmm::canTransit(mmmPhase aNextPhase)
{
    UChar aFlag = mTransitionMatrix[getCurrentPhase()][aNextPhase];

    return (aFlag == 0) ? ID_FALSE : ID_TRUE;
}

IDE_RC mmm::executeInternal(mmmPhaseDesc *aDesc, UInt aOptionflag)
{
    mmmPhaseAction  **sCurAction;
    SChar             sBuffer[1024];

    IDE_CALLBACK_SEND_MSG("");
    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer), "\nTRANSITION TO PHASE : %s", aDesc->mMessage);
    IDE_CALLBACK_SEND_MSG(sBuffer);

    for (sCurAction = aDesc->mActions;
         (*sCurAction) != NULL;
         sCurAction++)
    {
        /* bug-36515 mismatched err-msg when startup failed
           이전 단계에서 세팅됐을 수도 있는 필요없는 에러메시지를 clear */
        ideClearError();

        if ( ((*sCurAction)->mFlag & MMM_ACTION_NO_LOG) == 0)
        {
            ideLog::log(IDE_SERVER_0, "  ==> %s  ", (*sCurAction)->mMessage);
        }

        IDE_TEST( (*(*sCurAction)->mFunction)(aDesc->mPhase, aOptionflag, *sCurAction)
                  != IDE_SUCCESS);

        if ( ((*sCurAction)->mFlag & MMM_ACTION_NO_LOG) == 0)
        {
            ideLog::log(IDE_SERVER_0,"  ... [SUCCESS] ");
        }
    }

    // fix BUG-22157
    if (aDesc->mPhase != MMM_STARTUP_SHUTDOWN)
    {
        IDE_CALLBACK_SEND_NCHAR();
    }

//      idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer), "PHASE : %s : [SUCCESS]", aDesc->mMessage);
//      IDE_CALLBACK_SEND_MSG(sBuffer);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    {
        ideLog::logErrorMsg(IDE_SERVER_0);

        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer), "[FAILURE] %s", ideGetErrorMsg(ideGetErrorCode()));
        IDE_CALLBACK_SEND_MSG(sBuffer);
        return IDE_FAILURE;
    }

}


IDE_RC mmm::execute(mmmPhase aPhase, UInt aOptionflag)
{
    mmmPhaseDesc **sCurDesc;

    IDE_TEST_RAISE(canTransit(aPhase) != ID_TRUE, transition_error);

    if (aPhase == MMM_STARTUP_SHUTDOWN)
    {
        sCurDesc = &mDescList[MMM_STARTUP_SHUTDOWN]; /* shutdown phase */
    }
    else // Normal Startup
    {
        sCurDesc = &mDescList[getCurrentPhase() + 1]; /* next phase */
    }

    while(*sCurDesc != NULL)
    {
        IDE_TEST(executeInternal(*sCurDesc, aOptionflag) != IDE_SUCCESS);
        if ( (*sCurDesc)->mPhase == aPhase)
        {
            break; /* end of transition */
        }

        /* PROJ-2689 Downgrade meta */
        if ( ( aPhase == MMM_STARTUP_DOWNGRADE ) && 
             ( (*sCurDesc)->mPhase == MMM_STARTUP_META ) )
        {
            sCurDesc = &mDescList[MMM_STARTUP_DOWNGRADE]; 
        }
        else
        {
            sCurDesc++;
        }
    }
    mPhase = aPhase;
    return IDE_SUCCESS;
    IDE_EXCEPTION(transition_error);
    {
        
        ideLog::logErrorMsg(IDE_SERVER_0);
        IDE_SET(ideSetErrorCode(mmERR_ABORT_STARTUP_PHASE_ERROR));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/*
 * startup 되면서, 메시지 로깅 조차 초기화 되기 이전에
 * 에러가 발생할 경우 여기로 호출됨.
 * Howto?
 * 가장 좋은 방법은 client에게 메시지를 보내는 것이나, 불가능.
 * 시스템 로그로 메시지를 로깅하도록 계획을 세움. not yet. BUGBUG
 *
 */
void mmm::logToSystem(mmmPhase /*aPhase*/, mmmPhaseAction * /*aAction*/)
{
    // syslog("altibase was failed to startup...., aPhase, aAction Info");
    idlOS::exit(-1);
}


IDE_RC mmm::buildRecordForInstance(idvSQL              * /*aStatistics*/,
                                   void        *aHeader,
                                   void        * /* aDumpObj */,
                                   iduFixedTableMemory *aMemory)
{
    mmmInstance4PerfV sInstanceInfo;
    PDL_Time_Value    sCurTime;
    PDL_Time_Value    sWorkingTime;


    sInstanceInfo.mStartupPhase = mmm::mPhase;
    sInstanceInfo.mStartupTimeSec = mmm::mStartupTime.sec();
    sCurTime = idlOS::gettimeofday();
    sWorkingTime = sCurTime - mmm::mStartupTime;

    sInstanceInfo.mWorkingTimeSec = sWorkingTime.sec();

    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *)&sInstanceInfo)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gInstanceColDesc[]=
{

    {
        (SChar*)"STARTUP_PHASE",
        offsetof(mmmInstance4PerfV,mStartupPhase),
        IDU_FT_SIZEOF(mmmInstance4PerfV,mStartupPhase),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"STARTUP_TIME_SEC",
        offsetof(mmmInstance4PerfV, mStartupTimeSec),
        IDU_FT_SIZEOF(mmmInstance4PerfV,mStartupTimeSec),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"WORKING_TIME_SEC",
        offsetof(mmmInstance4PerfV,mWorkingTimeSec ),
        IDU_FT_SIZEOF(mmmInstance4PerfV,mWorkingTimeSec),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0,NULL // for internal use
    },


    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};
// X$INSTANCE
iduFixedTableDesc  gInstanceDesc =
{
    (SChar *)"X$INSTANCE",
    mmm::buildRecordForInstance,
    gInstanceColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



