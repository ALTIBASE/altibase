/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: ideMsgLogFT.cpp 91200 2021-07-12 02:29:48Z hykim $
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idp.h>

typedef struct ideTrcStat
{
    SChar *mModName; // 8
    UInt   mLevel;
    SChar *mFlag;    // 8
    ULong  mPowLevel;
    SChar *mDesc;    // 64
    
} ideTrcStat;

/* ------------------------------------------------
 *  FLAGS Description
 *  0 => NAME of Module
 *  1 => fitst bit..
 *    ........
 *  32 => 32nd bit..
 * ----------------------------------------------*/

// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gServerTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"[DEFAULT] TimeOut(Query,Fetch,Idle,UTrans) Trace Log",
    /*  2 bit */ (SChar *)"[DEFAULT] Network Operation Fail Trace Log",
    /*  3 bit */ (SChar *)"[DEFAULT] Memory Operation Warning Trace Log",
    /*  4 bit */ (SChar *)"---",
    /*  5 bit */ (SChar *)"---",
    /*  6 bit */ (SChar *)"---",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"---"
};

// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gQpTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"PSM Error Line Trace Log",
    /*  2 bit */ (SChar *)"DDL Trace Log",
    /*  3 bit */ (SChar *)"Job Scheduler Trace Log",
    /*  4 bit */ (SChar *)"DCL Trace Log",
    /*  5 bit */ (SChar *)"Query in PSM Trace Log",
    /*  6 bit */ (SChar *)"Query Plan in PSM Trace Log",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"Debug"
};

// BUG-49108 SERVER : WARNING!!!! << under 64 characters >>
static SChar *gJobTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"JOB Trace Log",
    /*  2 bit */ (SChar *)"---",
    /*  3 bit */ (SChar *)"---",
    /*  4 bit */ (SChar *)"---",
    /*  5 bit */ (SChar *)"---",
    /*  6 bit */ (SChar *)"---",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"Debug"
};

// BUG-46138 SERVER : WARNING!!!! << under 64 characters >>
static SChar *gSdTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"Shard Meta Error Trace Log",
    /*  2 bit */ (SChar *)"Global Transaction Error Log",
    /*  3 bit */ (SChar *)"---",
    /*  4 bit */ (SChar *)"Cluster Error Log",
    /*  5 bit */ (SChar *)"Shard Failover Error Log",
    /*  6 bit */ (SChar *)"---",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"Shard Meta Change Trace Log",
    /* 18 bit */ (SChar *)"Global Consistent Transaction DistTxInfo Trace Log",
    /* 19 bit */ (SChar *)"Global Consistent Transaction SM module Trace Log",
    /* 20 bit */ (SChar *)"Cluster Trace Log",
    /* 21 bit */ (SChar *)"Shard Failover Trace Log",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"Shard debug log",
    /* 32 bit */ (SChar *)"Shared transaction debug log",
};

// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gSmTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"Startup",
    /*  2 bit */ (SChar *)"Shutdown",
    /*  3 bit */ (SChar *)"Fatal error",
    /*  4 bit */ (SChar *)"Abort error",
    /*  5 bit */ (SChar *)"Warnning",
    /*  6 bit */ (SChar *)"Memory resource manager",
    /*  7 bit */ (SChar *)"Disk resource manager",
    /*  8 bit */ (SChar *)"Buffer manager",
    /*  9 bit */ (SChar *)"Memory recovery manager",
    /* 10 bit */ (SChar *)"Disk recovery manager",
    /* 11 bit */ (SChar *)"Memory page",
    /* 12 bit */ (SChar *)"Disk page",
    /* 13 bit */ (SChar *)"Memory record",
    /* 14 bit */ (SChar *)"Disk record",
    /* 15 bit */ (SChar *)"Memory index",
    /* 16 bit */ (SChar *)"Disk index",
    /* 17 bit */ (SChar *)"Transaction manager",
    /* 18 bit */ (SChar *)"Lock manager",
    /* 19 bit */ (SChar *)"Ager",
    /* 20 bit */ (SChar *)"Thread",
    /* 21 bit */ (SChar *)"Interface",
    /* 22 bit */ (SChar *)"Utility",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"Debug"
};

// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gRpTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"HBT Event Trace Log",
    /*  2 bit */ (SChar *)"Conflict Event Trace Log",
    /*  3 bit */ (SChar *)"Conflict SQL Trace Log",
    /*  4 bit */ (SChar *)"Replication Log Buffer Log",
    /*  5 bit */ (SChar *)"Conflict Primary Key Trace Log",
    /*  6 bit */ (SChar *)"Consistent Replication File Operation Log",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"---"
};


// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gRpConflictTrcDesc[32] = 
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"---",
    /*  2 bit */ (SChar *)"Conflict Event Trace Log",
    /*  3 bit */ (SChar *)"Conflict SQL Trace Log",
    /*  4 bit */ (SChar *)"---",
    /*  5 bit */ (SChar *)"Conflict Primary Key Trace Log",
    /*  6 bit */ (SChar *)"---",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"---"
};

// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gDkTrcDesc[32] =
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"Operational Trace Message",
    /*  2 bit */ (SChar *)"---",
    /*  3 bit */ (SChar *)"---",
    /*  4 bit */ (SChar *)"---",
    /*  5 bit */ (SChar *)"---",
    /*  6 bit */ (SChar *)"---",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"---"
};

/* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
// SERVER : WARNING!!!! << under 64 characters >>
static SChar *gLbTrcDesc[32] =
{
                         //0123456789012345678901234567890123456789012345678901234567890123
    /*  1 bit */ (SChar *)"Service Thread Event Trace Log",
    /*  2 bit */ (SChar *)"Task Event Trace Log",
    /*  3 bit */ (SChar *)"---",
    /*  4 bit */ (SChar *)"---",
    /*  5 bit */ (SChar *)"---",
    /*  6 bit */ (SChar *)"---",
    /*  7 bit */ (SChar *)"---",
    /*  8 bit */ (SChar *)"---",
    /*  9 bit */ (SChar *)"---",
    /* 10 bit */ (SChar *)"---",
    /* 11 bit */ (SChar *)"---",
    /* 12 bit */ (SChar *)"---",
    /* 13 bit */ (SChar *)"---",
    /* 14 bit */ (SChar *)"---",
    /* 15 bit */ (SChar *)"---",
    /* 16 bit */ (SChar *)"---",
    /* 17 bit */ (SChar *)"---",
    /* 18 bit */ (SChar *)"---",
    /* 19 bit */ (SChar *)"---",
    /* 20 bit */ (SChar *)"---",
    /* 21 bit */ (SChar *)"---",
    /* 22 bit */ (SChar *)"---",
    /* 23 bit */ (SChar *)"---",
    /* 24 bit */ (SChar *)"---",
    /* 25 bit */ (SChar *)"---",
    /* 26 bit */ (SChar *)"---",
    /* 27 bit */ (SChar *)"---",
    /* 28 bit */ (SChar *)"---",
    /* 29 bit */ (SChar *)"---",
    /* 30 bit */ (SChar *)"---",
    /* 31 bit */ (SChar *)"---",
    /* 32 bit */ (SChar *)"---"
};

static IDE_RC buildTrcRecord( SChar               *aModName,
                              UInt                 aTotalValue,
                              SChar               *aDesc[],
                              void                *aHeader,
                              iduFixedTableMemory *aMemory)
{
    UInt i;

    ideTrcStat  sStat;
    
    idlOS::memset(&sStat, 0, ID_SIZEOF(ideTrcStat));

    sStat.mModName = aModName;
    
    for (i = 0; i < 32; i++)
    {
        ULong mBit = 1;

        mBit <<= i;
        
        sStat.mLevel = (i + 1);
        sStat.mFlag  = (aTotalValue & mBit) > 0 ? (SChar *)"O" : (SChar *)"X";
        sStat.mPowLevel = mBit;
        sStat.mDesc  = aDesc[i];
        
        /* [2] make one fixed table record */
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            &sStat)
                 != IDE_SUCCESS);
    }
    // final : sum
    
    sStat.mLevel = 99;
    sStat.mFlag  = (SChar *)"SUM";
    sStat.mPowLevel = aTotalValue;
    sStat.mDesc = (SChar *)"Total Sum of Trace Log Values";
    
    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        &sStat)
             != IDE_SUCCESS);
        
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}



IDE_RC buildRecordCallback(idvSQL              */* aStatistics */,
                           void                *aHeader,
                           void                * /* aDumpObj */,
                           iduFixedTableMemory *aMemory)
{
    /* ------------------------------------------------
     * SERVER 
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"SERVER",
                            iduProperty::getServerTrcFlag(),
                            gServerTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);
    
    /* ------------------------------------------------
     * SM 
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"SM",
                            iduProperty::getSmTrcFlag(),
                            gSmTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);
    
    /* ------------------------------------------------
     * QP 
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"QP",
                            iduProperty::getQpTrcFlag(),
                            gQpTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);

    /* ------------------------------------------------
     * JOB   BUG-49108
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"JOB",
                            iduProperty::getJobTrcFlag(),
                            gJobTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);

    /* ------------------------------------------------
     * SD   BUG-46138 
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"SD",
                            iduProperty::getSdTrcFlag(),
                            gSdTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);

    /* ------------------------------------------------
     * RP 
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"RP",
                            iduProperty::getRpTrcFlag(),
                            gRpTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);
    
    IDE_TEST(buildTrcRecord((SChar *)"RP_CONFLICT",
                            iduProperty::getRpConflictTrcFlag(),
                            gRpConflictTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);

    /* ------------------------------------------------
     * DK
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"DK",
                            iduProperty::getDkTrcFlag(),
                            gDkTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);

    /* ------------------------------------------------
     * LB(LoadBalancer)
     * ----------------------------------------------*/
    IDE_TEST(buildTrcRecord((SChar *)"LB",
                            iduProperty::getLbTrcFlag(),
                            gLbTrcDesc,
                            aHeader,
                            aMemory) != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableColDesc gTracelogColDesc[] =
{
    {
        (SChar *)"MODULE_NAME",
        offsetof(ideTrcStat, mModName),
        16,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TRCLEVEL",
        offsetof(ideTrcStat, mLevel),
        IDU_FT_SIZEOF(ideTrcStat, mLevel),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"FLAG",
        offsetof(ideTrcStat, mFlag),
        8,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"POWLEVEL",
        offsetof(ideTrcStat, mPowLevel),
        IDU_FT_SIZEOF(ideTrcStat, mPowLevel),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"DESCRIPTION",
        offsetof(ideTrcStat, mDesc),
        64,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
};


iduFixedTableDesc gTracelogTable =
{
    (SChar *)"X$TRACELOG",
    buildRecordCallback,
    gTracelogColDesc,
    IDU_STARTUP_INIT,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
