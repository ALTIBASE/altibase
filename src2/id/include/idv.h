/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idv.h 87960 2020-07-06 09:53:27Z kclee $
 **********************************************************************/

#ifndef _O_IDV_H_
#define _O_IDV_H_ 1

#include <idl.h>
#include <idvType.h>
#include <idvTime.h>

/* ------------------------------------------------
 *  하나의 Stmt에 대한 통계 정보
 * ----------------------------------------------*/

class  iduFixedTableMemory;

#define IDV_SQL_OPTIME_BEGIN( __idvBasePtr__, __Idx__ ) \
    if (__idvBasePtr__ != NULL)                                         \
        idvManager::beginOpTime( __idvBasePtr__, __Idx__ );

#define IDV_SQL_OPTIME_END( __idvBasePtr__, __Idx__ ) \
    if (__idvBasePtr__ != NULL)                                         \
        idvManager::endOpTime( __idvBasePtr__, __Idx__ );

#define IDV_SQL_WAIT_TIME_DIRECT( __idvBasePtr__ )  \
    &((__idvBasePtr__)->mTimedWait)

#define IDV_SQL_WAIT_PARAM_DIRECT( __idvBasePtr__, __Idx__ )    \
    &((__idvBasePtr__)->mWaitEventParam[(UInt)__Idx__])

#define IDV_SQL_OPTIME_DIRECT( __idvBasePtr__, __Idx__)  \
    &((__idvBasePtr__)->mOpTime[(UInt)__Idx__])

#define IDV_SESSION_WAIT_EVENT( __idvBasePtr__, __Idx__ )  \
    &((__idvBasePtr__)->mWaitEvent[(UInt)__Idx__])

#define IDV_SYSTEM_WAIT_EVENT( __Idx__ )  \
    &(gSystemInfo.mWaitEvent[(UInt)__Idx__])

/* ------------------------------------------------
 * idvSQL Member Get/Set Macro for Thread, Process
 * Info
 * ----------------------------------------------*/
#define IDV_WP_GET_PROC_INFO(__idvBasePtr__) (((__idvBasePtr__) == NULL ) ? NULL : (__idvBasePtr__)->mProcInfo)
#define IDV_WP_SET_MAIN_PROC_INFO(__idvBasePtr__) \
        (__idvBasePtr__)->mProcInfo = idwPMMgr::getMainShmProcInfo();

#define IDV_WP_SET_PROC_INFO(__idvBasePtr__, __Value__) \
    (__idvBasePtr__)->mProcInfo = (__Value__);

#define IDV_WP_GET_THR_INFO(__idvBasePtr__) (((__idvBasePtr__) == NULL ) ? NULL : (iduShmTxInfo*)(__idvBasePtr__)->mThrInfo)
#define IDV_WP_SET_THR_INFO(__idvBasePtr__, __Value__)    \
    (__idvBasePtr__)->mThrInfo = (__Value__);

#define IDV_SQL_ADD(__idvBasePtr__, __Name__, __Value__)                \
    if (__idvBasePtr__ != NULL)                                         \
        IDV_SQL_ADD_DIRECT(__idvBasePtr__, __Name__, __Value__);

#define IDV_SQL_SET(__idvBasePtr__, __Name__, __Value__)                \
    if (__idvBasePtr__ != NULL)                                         \
        IDV_SQL_SET_DIRECT(__idvBasePtr__, __Name__, __Value__);

#define IDV_WEARGS_INIT( __idvBasePtr__ )                      \
    if (__idvBasePtr__ != NULL)                                \
         { idlOS::memset( (__idvBasePtr__)->mWaitParam,        \
                        0x00, IDV_WAIT_PARAM_COUNT );          \
          (__idvBasePtr__)->mWaitEventID = IDV_WAIT_INDEX_NULL; }

#define IDV_WEARGS_SET( __idvBasePtr__, __Idx__, __Param1__, __Param2__, __Param3__ ) \
    (__idvBasePtr__)->mWaitEventID  = __Idx__;     \
    (__idvBasePtr__)->mWaitParam[0] = __Param1__;  \
    (__idvBasePtr__)->mWaitParam[1] = __Param2__;  \
    (__idvBasePtr__)->mWaitParam[2] = __Param3__;

/* ------------------------------------------------
 *  idvTimeBox Add/Set Macro
 * ----------------------------------------------*/
#define IDV_TIMEBOX_INIT( __idvBasePtr__ )                       \
    if (__idvBasePtr__ != NULL)                                 \
        IDV_TIMEBOX_INIT_DIRECT( __idvBasePtr__ );

#define IDV_TIMEBOX_BEGIN( __idvBasePtr__ )                      \
    if (__idvBasePtr__ != NULL)                                 \
        IDV_TIMEBOX_BEGIN_DIRECT( __idvBasePtr__ );

#define IDV_TIMEBOX_END(__idvBasePtr__)                         \
    if (__idvBasePtr__ != NULL)                                 \
        IDV_TIMEBOX_END_DIRECT( __idvBasePtr__ );
#define IDV_TIMEBOX_SET( __idvBasePtr__, __TimeValue__,__Begin__, __End__ )          \
    if (__idvBasePtr__ != NULL)                                                      \
        IDV_TIMEBOX_SET_DIRECT(__idvBasePtr__, __TimeValue__, __Begin__, __End__ );

#define IDV_TIMEBOX_ACCUMULATED( __idvBasePtr__ )                           \
    if (__idvBasePtr__ != NULL)                                            \
        IDV_TIMEBOX_ACCUMULATED_DIRECT( __idvBasePtr__ );


/* ------------------------------------------------
 *  idvSQL Member Operation Elasped Time Get/Set Macro
 * ----------------------------------------------*/

#define IDV_TIMEBOX_GET_ELA_TIME( __idvBasePtr__ ) \
        (( __idvBasePtr__ )->mATD.mElaTime)

#define IDV_TIMEBOX_GET_ACCUM_TIME( __idvBasePtr__ ) \
        (( __idvBasePtr__ )->mATD.mAccumTime)

// BUG-21204 : V$SESSTAT의 누적될 시간 초기화
#define IDV_TIMEBOX_INIT_ACCUM_TIME( __idvBasePtr__ ) \
        (( __idvBasePtr__ )->mATD.mAccumTime = 0)

#define IDV_TIMEBOX_GET_BEGIN_TIME(  __idvBasePtr__ ) \
        ((  __idvBasePtr__ )->mBegin )

#define IDV_TIMEBOX_GET_END_TIME(  __idvBasePtr__ ) \
        (( __idvBasePtr__ )->mEnd)

#define IDV_TIMEBOX_GET_TIME_SWITCH( __idvBasePtr__ ) \
        (( __idvBasePtr__ )->mATD.mTimeSwitch)



#define IDV_SQL_OPTIME_OFFSET( __Idx__ ) \
    offsetof( idvSQL, mOpTime[ (UInt)__Idx__ ] )

#define IDV_SQL_WAIT_PARAM_OFFSET( __Idx__ ) \
    offsetof( idvWeArgs, mWaitParam[ (UInt)__Idx__ ] )

/* ------------------------------------------------
 *  idvSession Member Add/Set Macro
 * ----------------------------------------------*/

#define IDV_SESS_ADD(__idvBasePtr__, __Idx__, __Value__) \
       if (__idvBasePtr__ != NULL)  IDV_SESS_ADD_DIRECT(__idvBasePtr__, __Idx__, __Value__);

#define IDV_SESS_SET(__idvBasePtr__, __Idx__, __Value__) \
       if (__idvBasePtr__ != NULL)  IDV_SESS_SET_DIRECT(__idvBasePtr__, __Idx__, __Value__);

#define IDV_BEGIN_WAIT_EVENT(__idvBasePtr__, __WeArgs__) \
    if (__idvBasePtr__ != NULL) \
        IDV_BEGIN_WAIT_EVENT_DIRECT( __idvBasePtr__, __WeArgs__ );

#define IDV_END_WAIT_EVENT(__idvBasePtr__, __WeArgs__) \
    if (__idvBasePtr__ != NULL) \
        IDV_END_WAIT_EVENT_DIRECT( __idvBasePtr__, __WeArgs__ );

/* ------------------------------------------------
 *  idvSystem Member Add/Set Macro
 * ----------------------------------------------*/

#define IDV_SYS_ADD( __Idx__, __Value__) \
    gSystemInfo.mStatEvent[(UInt)__Idx__ ].mValue += __Value__

#define IDV_SYS_SET(__Idx__, __Value__) \
    gSystemInfo.mStatEvent[(UInt)__Idx__ ].mValue = __Value__

#define IDV_SYS_LATCH_STAT_ADD( __Idx__, __Value1__, __Value2__) \
    {gSystemInfo.mMissCnt[(UInt)__Idx__ ] += __Value1__;          \
     gSystemInfo.mMissTime[(UInt)__Idx__ ] += __Value2__;}



/* ------------------------------------------------
 *  direct series Member Add/Set Macro
 * ----------------------------------------------*/

// DIRECT : pointer should not be NULL!!!
#define IDV_SQL_ADD_DIRECT(__idvBasePtr__, __Name__, __Value__) \
        {(__idvBasePtr__)->__Name__ += __Value__;}

#define IDV_SQL_SET_DIRECT(__idvBasePtr__, __Name__, __Value__) \
        {(__idvBasePtr__)->__Name__ = __Value__;}

#define IDV_SESS_ADD_DIRECT(__idvBasePtr__, __Idx__, __Value__) \
        {(__idvBasePtr__)->mStatEvent[(UInt)__Idx__].mValue += __Value__;}

#define IDV_SESS_SET_DIRECT(__idvBasePtr__, __Idx__, __Value__) \
        {(__idvBasePtr__)->mStatEvent[(UInt)__Idx__].mValue[(UInt)__Idx__] = __Value__;}

#define IDV_TIMEBOX_INIT_DIRECT( __idvBasePtr__ )       \
    {IDV_TIME_INIT(&(( __idvBasePtr__ )->mEnd));}

#define IDV_TIMEBOX_BEGIN_DIRECT( __idvBasePtr__ )                \
    { IDV_TIME_GET(&(( __idvBasePtr__ )->mBegin));                \
    ( __idvBasePtr__ )->mATD.mTimeSwitch = IDV_TIME_SWITCH_ON; }

/* BUG-44419  경과시간 통계값 에서 잘못된 값이 사용되어지는 경우가 있음. */
#define IDV_TIMEBOX_END_DIRECT( __idvBasePtr__ )                                       \
    { if ( ( __idvBasePtr__ )->mATD.mTimeSwitch == IDV_TIME_SWITCH_ON )                \
      { IDV_TIME_GET(&(( __idvBasePtr__ )->mEnd));                                     \
            ( __idvBasePtr__ )->mATD.mElaTime =                                        \
                IDV_TIME_DIFF_MICRO_SAFE(&(( __idvBasePtr__ )->mBegin),                \
                                         &(( __idvBasePtr__ )->mEnd));                 \
            ID_SERIAL_BEGIN( idCore::acpAtomicAdd64 (                                  \
                                    &(( __idvBasePtr__ )->mATD.mAccumTime),            \
                                    ( __idvBasePtr__ )->mATD.mElaTime ) );/*BUG-47939*/\
            ID_SERIAL_END(( __idvBasePtr__ )->mATD.mTimeSwitch = IDV_TIME_SWITCH_OFF); }}

#define IDV_TIMEBOX_SET_DIRECT(__idvBasePtr__, __TimeValue__, __Begin__, __End__ )  \
    { ( __idvBasePtr__ )->mATD.mAccumTime = __TimeValue__;                          \
        ( __idvBasePtr__ )->mBegin        = __Begin__;                              \
        ( __idvBasePtr__ )->mEnd          = __End__; }

#define IDV_TIMEBOX_ACCUMULATED_DIRECT( __idvBasePtr__ )                \
    {if (( __idvBasePtr__ )->mATD.mTimeSwitch == IDV_TIME_SWITCH_OFF )  \
         ( __idvBasePtr__ )->mATD.mTimeSwitch = IDV_TIME_SWITCH_ACCUM;}


#define IDV_BEGIN_WAIT_EVENT_DIRECT(__idvBasePtr__, __WeArgs__)    \
    idvManager::beginWaitEvent( __idvBasePtr__, __WeArgs__);

#define IDV_END_WAIT_EVENT_DIRECT(__idvBasePtr__, __WeArgs__)    \
    idvManager::endWaitEvent( __idvBasePtr__, __WeArgs__);


#define IDV_SESSION_EVENT(__idvBasePtr__)                       \
    (__idvBasePtr__ == NULL ? NULL : __idvBasePtr__->mSessionEvent)

#define IDV_CURRENT_STATEMENT_ID(__idvBasePtr__)                \
    (__idvBasePtr__ == NULL ? NULL : __idvBasePtr__->mCurrStmtID)

/* ------------------------------------------------
 *  Class for idv Module Control
 * ----------------------------------------------*/

struct idvResource;

typedef struct idvHandler
{
    IDE_RC (*mInitializeStatic)(idvResource **, ULong *aClockArea, ULong *aSecond);
    IDE_RC (*mStartup)(idvResource *);
    IDE_RC (*mShutdown)(idvResource *);

    IDE_RC (*mDestroyStatic)(idvResource *);

}idvHandler;


typedef struct idvTimeFunc
{
    void   (*mInit)(idvTime *);
    idBool (*mIsInit)(idvTime *);
    void   (*mGet)(idvTime *);
    ULong  (*mDiff)(idvTime *aBefore, idvTime *aAfter); // to Micro
    ULong  (*mMicro)(idvTime *);                    // to Micro
    idBool (*mIsOlder)(idvTime *aBefore, idvTime *aAfter);
}idvTimeFunc;

typedef IDE_RC (*idvLinkCheckCallback)(void *aLink);
typedef UInt   (*idvBaseTimeCallback)();

class idvManager
{
    static UInt          mType; // 0 : none 1 : timer thread 2:libraray 3:tick
    static UInt          mLinkCheckTime;
    static ULong         mClock;
    static ULong         mTimeSec;
    static idvResource  *mResource;
    static idvHandler   *mHandler;
    static idvTimeFunc  *mOPArray[]; // mType Range
public:
    static idvStatName        mStatName[IDV_STAT_INDEX_MAX];

    static idvWaitEventName   mWaitEventName[IDV_WAIT_INDEX_MAX + 1];
    static idvWaitClassName   mWaitClassName[IDV_WCLASS_INDEX_MAX];
    static idvTimeFunc        *mOP;

    static idvLinkCheckCallback mLinkCheckCallback;
    static idvBaseTimeCallback  mBaseTimeCallback;
    static idBool               mIsServiceAvail;

    /* ------------------------------------------------
     *  Functions for idvSQL
     * ----------------------------------------------*/

    static void initSQL(idvSQL     *,
                        idvSession *,
                        ULong      *aSessionEvent  = NULL,
                        UInt       *aCurrStmtID    = NULL,
                        void       *aLink          = NULL,
                        UInt       *aLinkCheckTime = NULL,
                        idvOwner    aOwner         = IDV_OWNER_UNKNOWN);

    static void resetSQL(idvSQL *aSQL);

    static void initStat4Session( idvSession* aSession);

    static void initSession( idvSession *,
                             UInt aSessionID,
                             void* aMMSessionPtr );

    // BUG-21204
    static void initPrepareAccumTime( idvSQL * aStatSQL );
    static void initBeginStmtAccumTime( idvSQL * aStatSQL );

    /* BUG-31545 replication statistics */
    static void initRPSenderAccumTime( idvSQL * aStatSQL );
    static void initRPSenderApplyAccumTime( idvSQL * aStatSQL );
    static void initRPReceiverAccumTime( idvSQL * aStatSQL );

    /* ------------------------------------------------
     *  Functions for Manager
     * ----------------------------------------------*/
    static IDE_RC initializeStatic();
    static IDE_RC startupService();
    /* time 서비스 사용이 가능한지? */
    inline static idBool isServiceAvail();
    static IDE_RC shutdownService();
    static IDE_RC destroyStatic();
    static UInt   getTimeType() { return mType; }

    /* Session end시와 주기적으로 에 통계정보를 자신의
       System 통계정보에 누적시킨다. */
    static void   applyStatisticsToSystem(idvSession  *aCurr,
                                          idvSession  *aOld);

    static void   applyOpTimeToSession( idvSession  *aCurrSess,
                                        idvSQL      *aCurrSQL );

    /* 대기이벤트에 대한 통계정보를 측정한다. */
    static void   beginWaitEvent( void   * aStatSQL,
                                  void   * aWeArgs );

    static void   endWaitEvent( void  * aStatSQL,
                                void   * aWeArgs );

    /* 연산에 대한 경과시간 통계정보를 측정한다. */
    static void   beginOpTime( idvSQL               * aStatSQL,
                               idvOperTimeIndex    aOpIdx );

    static void   endOpTime( idvSQL        * aStatSQL,
                             idvOperTimeIndex  aOpIdx );

    static UInt   getLinkCheckTime()           { return mLinkCheckTime;  }
    static void   setLinkCheckTime(UInt aTime) { mLinkCheckTime = aTime; }

    static void   setLinkCheckCallback(idvLinkCheckCallback aCallback)
    {
        mLinkCheckCallback = aCallback;
    }

    static void   setBaseTimeCallback(idvBaseTimeCallback aCallback)
    {
        mBaseTimeCallback = aCallback;
    }

    /* BUG-29005 - Fullscan 성능 문제
     * idvSQL에서 Session ID를 가져오는 함수 추가함.
     * SM에서 callback으로 사용 */
    static UInt   getSessionID( idvSQL  *aStatSQL );

    static ULong  getClock()  { return mClock; }
    static ULong  getSecond() { return (ULong)mBaseTimeCallback(); }

    static IDE_RC buildRecordForStatName( idvSQL              *aStatistics,
                                          void                *aHeader,
                                          void                *aDumpObj,
                                          iduFixedTableMemory *aMemory);

    static IDE_RC buildRecordForWaitEventName(
        idvSQL              *aStatistics,
        void                *aHeader,
        void                *aDumpObj,
        iduFixedTableMemory *aMemory);

    static IDE_RC buildRecordForWaitClassName(
        idvSQL              *aStatistics,
        void                *aHeader,
        void                *aDumpObj,
        iduFixedTableMemory *aMemory);

    static ULong fixTimeDiff( idvTime *aBefore )
    {
        idvTime           sCurTime;

        IDV_TIME_GET(&sCurTime);

        return mOP->mDiff((aBefore), (&sCurTime));
    }

private:

    static void   addOpTimeToSession( idvStatIndex  aStatIdx,
                                      idvSession   *aCurrSess,
                                      idvSQL       *aCurrSQL );

    static void   applyStatEventToSystem(idvStatIndex  aStatIdx,
                                         idvStatEvent *aCurr,
                                         idvStatEvent *aOld);

    static void   applyWaitEventToSystem(idvWaitIndex  aWaitIdx,
                                         idvWaitEvent *aCurr,
                                         idvWaitEvent *aOld );

    static void   applyLatchStatToSystem( UInt          aPageType,
                                          ULong        *aCurrMissCnt,
                                          ULong        *aOldMissCnt,
                                          ULong        *aCurrMissTime,
                                          ULong        *aOldMissTime );
};

extern idvSystem gSystemInfo;

/* ------------------------------------------------
 *  Time Service의 사용가능 여부 체크
 * ----------------------------------------------*/
idBool idvManager::isServiceAvail()
{
    return mIsServiceAvail;
}

#endif
