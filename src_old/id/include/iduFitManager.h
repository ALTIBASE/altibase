/***********************************************************************
 * Copyright 1999-2013, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFitManager.h 80739 2017-08-08 07:58:45Z andrew.shin $
 **********************************************************************/

#ifndef _O_IDU_FIT_MANAGER_H_
#define _O_IDU_FIT_MANAGER_H_ 1

#ifdef ALTIBASE_FIT_CHECK

#define IDU_FIT_POINT( aUID, ... )                                        \
{                                                                         \
    IDU_FIT_POINT_RAISE( aUID, IDE_EXCEPTION_END_LABEL, ##__VA_ARGS__ );  \
}

#define IDU_FIT_POINT_RAISE( aUID, aLabel, ... )                                        \
{                                                                                       \
    iduFitAction sFitAction;                                                            \
    if ( ( iduFitManager::mIsFitEnabled != IDU_FIT_FALSE ) &&                           \
         ( iduFitManager::getFitEnable() == IDU_FIT_TRUE ) )                            \
    {                                                                                   \
        if ( iduFitManager::requestFitAction( aUID, &sFitAction ) == IDE_SUCCESS )      \
        {                                                                               \
            iduFitManager::setLogInfo((SChar*)__FILE__, __LINE__, &sFitAction);         \
            if ( #__VA_ARGS__[0] != IDE_FIT_STRING_TERM )                               \
            {                                                                           \
                iduFitManager::setErrorCode("%s", ##__VA_ARGS__);                       \
            }                                                                           \
                                                                                        \
            (void)iduFitManager::validateFitAction( &sFitAction );                      \
                                                                                        \
            if ( IDU_FIT_ACTION_JUMP == sFitAction.mType )                              \
            {                                                                           \
                goto aLabel;                                                            \
            }                                                                           \
            else if ( IDU_FIT_ACTION_SIGSEGV == sFitAction.mType )                      \
            {                                                                           \
                IDU_FIT_DO_SIGSEGV();                                                   \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
}

/* PROJ-2617 */
#define IDU_FIT_POINT_FATAL( aUID )                                                     \
{                                                                                       \
    iduFitAction sFitAction;                                                            \
    if ( ( iduFitManager::mIsFitEnabled != IDU_FIT_FALSE ) &&                           \
         ( iduFitManager::getFitEnable() == IDU_FIT_TRUE ) )                            \
    {                                                                                   \
        if ( iduFitManager::requestFitAction( aUID, &sFitAction ) == IDE_SUCCESS )      \
        {                                                                               \
            iduFitManager::setLogInfo((SChar*)__FILE__, __LINE__, &sFitAction);         \
                                                                                        \
            (void)iduFitManager::validateFitAction( &sFitAction );                      \
                                                                                        \
            if ( IDU_FIT_ACTION_SIGSEGV == sFitAction.mType )                           \
            {                                                                           \
                IDU_FIT_DO_SIGSEGV();                                                   \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
}                                                                                       \

#define IDU_FIT_DO_SIGSEGV()            \
{                                       \
    volatile SInt *sSigsegvA = NULL;    \
    volatile SInt sSigsegvB;            \
    sSigsegvB = *sSigsegvA;             \
    sSigsegvB++;                        \
}                                       \

#define IDU_FIT_LOG( aMessage )                     \
{                                                   \
    ideLogEntry sLog(IDE_FIT_0);                    \
    sLog.setTailless(ACP_TRUE);                     \
    sLog.appendFormat( "%s\n", aMessage );          \
    sLog.write();                                   \
} 

#define IDU_FIT_DEBUG_LOG( aMessage )                                   \
{                                                                       \
    ideLogEntry sLog(IDE_FIT_0);                                        \
    sLog.setTailless(ACP_TRUE);                                         \
    sLog.appendFormat( "%s::%d %s\n", __FILE__, __LINE__, aMessage );   \
    sLog.write();                                                       \
} 


#define IDU_FIT_EXIT() { if ( iduFitManager::mIsFitExit == ID_TRUE ) return; }

#define IDU_FIT_PLUGIN_NAME          ( (SChar *)"libfitplugin" )
#define IDU_FIT_ATAF_HOME_ENV        ( (SChar *)"ATAF_HOME"  )
#define IDU_FIT_ENABLE_ENV           ( (SChar *)"FIT_ENABLE"  )
#define IDU_FIT_PLUGIN_TRANSMISSION  ( (SChar *)"cmTransmission" )


#define IDU_FIT_SID_MAX_LEN    (32)
#define IDU_FIT_TID_MAX_LEN    (128)
#define IDU_FIT_DATA_MAX_LEN   (256)
#define IDU_FIT_PATH_MAX_LEN   (1024)
#define IDU_FIT_NAME_MAX_LEN   (256)

#define IDE_FIT_LEFT_BRACE     "{"
#define IDE_FIT_RIGHT_BRACE    "}"
#define IDE_FIT_STRING_TERM    '\0'
#define IDU_FIT_VERTICAL_BAR   "|"

#define IDU_FIT_INIT_RETRY_MAX (100)

#define IDU_FIT_POINT_EXIST (0)
#define IDU_FIT_POINT_NOT_EXIST (1)

#define IDU_FIT_SHM_KEY             ( (SChar *)"FIT_SHM_KEY" )
#define IDU_FIT_SHM_KEY_DEFAULT     ( (SChar *)"FIT_SHM_KEY_DEFAULT" )
#define IDU_FIT_ALTIBASE_PORT_NO    ( (SChar *)"ALTIBASE_PORT_NO" )
#define IDU_FIT_SHM_KEY_WEIGHT_NUMBER (100)

typedef SInt (*fitTransmission)(SChar *, UInt, SChar *, UInt);

typedef enum iduFitProtocol
{
    IDU_FIT_DO_ACTION = 0,
    IDU_FIT_NOT_READY_YET = 1,
    IDU_FIT_NOT_AVAILABLE = 2,
    IDU_FIT_NOT_EXIST = 3,
    IDU_FIT_SEARCH_ERROR = 4,
    IDU_FIT_PROTOCOL_MAX = 5
} iduFitProtocol;

typedef enum iduFitActionType
{
    IDU_FIT_ACTION_NONE = 0,
    IDU_FIT_ACTION_HIT = 1,
    IDU_FIT_ACTION_JUMP = 2,
    IDU_FIT_ACTION_KILL = 3,
    IDU_FIT_ACTION_SLEEP = 4,
    IDU_FIT_ACTION_TIME_SLEEP = 5,
    IDU_FIT_ACTION_WAKEUP = 6,
    IDU_FIT_ACTION_WAKEUP_ALL = 7,
    IDU_FIT_ACTION_SIGSEGV = 8,
    IDU_FIT_ACTION_ERROR = 9,
    IDU_FIT_ACTION_MAX = 10
} iduFitActionType;

typedef enum iduFitStatus
{
    IDU_FIT_NONE = 0,
    IDU_FIT_TRUE = 1,
    IDU_FIT_FALSE= 2,
    IDU_FIT_ERROR = 3,
    IDU_FIT_MAX = 4
} iduFitStatus;

typedef struct iduFitAction
{
    SChar mTID[IDU_FIT_TID_MAX_LEN + 1];

    iduFitProtocol mProtocol;
    iduFitActionType mType;

    UInt mTimeout;

    /* PROJ-2581 */
    SChar mFileName[IDU_FIT_NAME_MAX_LEN + 1];
    UInt mCodeLine;
} iduFitAction;

typedef struct iduSleepNode
{
    iduFitAction mAction;

    PDL_Time_Value mTV;

    iduSleepNode *mNext;
    iduSleepNode *mPrev;

    iduMutex mMutex;
    iduCond  mCond;
} iduSleepNode;

typedef struct iduSleepList
{
    iduSleepNode mHead;
    iduSleepNode mTail;

    UShort mCount;
    iduMutex mMutex;
} iduSleepList;


class iduFitManager
{
public:

    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC requestFitAction( const SChar *aUID, iduFitAction *aFitAction );
    static IDE_RC validateFitAction( iduFitAction *aFitAction );

    /* ErrorCode */
    static void setErrorCode( const SChar *aFormat, ... );
    static void setLogInfo( const SChar *aFileName, 
                            UInt aCodeLine, 
                            iduFitAction *aFitAction );

    static idBool mIsFitExit;
    static iduShmProcType mFitProcType;

    static iduFitStatus mIsFitEnabled;
    static inline iduFitStatus getFitEnable();

    static UInt mInitFailureCount;

    static void *mShmPtr;
    static volatile SInt *mShmBuffer;

private:

    iduFitManager(){};
    ~iduFitManager(){};

    /* fitPlugin 로딩 관련된 함수들 */
    static IDE_RC loadFitPlugin();
    static IDE_RC unloadFitPlugin();
    static IDE_RC loadFitFunctions();

    /* 파싱 관련된 함수들 */
    static IDE_RC parseFitAction( SChar *aRecvData, iduFitAction *aFitAction );
    static IDE_RC parseSID( void );

    /* 동적으로 로딩될 함수 */
    static fitTransmission mTransmission;

    /* SleepList 관련된 함수들 */
    static IDE_RC initSleepList( iduSleepList *aSleepList );
    static IDE_RC finalSleepList( iduSleepList *aSleepList );
    static IDE_RC resetSleepList( iduSleepList *aSleepList );

    static IDE_RC createSleepNode( iduSleepNode **aSleepNode, iduFitAction *aFitAction );
    static IDE_RC freeSleepNode( iduSleepNode *aSleepNode );

    static IDE_RC addSleepNode( iduSleepList *aSleepList, iduSleepNode *aSleepNode );
    static IDE_RC deleteSleepNode( iduSleepList *aSleepList, iduSleepNode *aSleepNode );

    static IDE_RC getSleepNode( const SChar *aUID, iduSleepNode **aSleepNode );

    /* FIT 테스트 기능 함수들 */
    static IDE_RC hitServer( iduFitAction *aFitAction );
    static IDE_RC abortServer( iduFitAction *aFitAction );
    static IDE_RC killServer( iduFitAction *aFitAction );
    static IDE_RC sleepServer( iduFitAction *aFitAction );
    static IDE_RC timeSleepServer( iduFitAction *aFitAction );
    static IDE_RC wakeupServerBySignal( iduFitAction *aFitAction );
    static IDE_RC wakeupServerByBroadcast( iduFitAction *aFitAction );
    static IDE_RC sigsegvServer( iduFitAction *aFitAction );

    static IDE_RC attachSharedMemory();
    static IDE_RC getSharedMemoryKey( key_t *aKey );

    static PDL_SHLIB_HANDLE mFitHandle;

    static idBool mIsInitialized;
    static idBool mIsPluginLoaded;

    static SChar mSID[IDU_FIT_SID_MAX_LEN + 1];

    static iduSleepList mSleepList;
	static iduMutex mMutex;

};

inline iduFitStatus iduFitManager::getFitEnable()
{

    static SChar *sFitEnableEnv = NULL;
    static SLong sFitEnable = 0L;
    static SInt sFitEnableEnvLen = 0;

    /* FIT_ENABLE 환경변수를 이미 읽은 경우에는 다시 읽지 않는다. */
    IDE_TEST( mIsFitEnabled != IDU_FIT_NONE );

    sFitEnableEnv = idlOS::getenv( IDU_FIT_ENABLE_ENV );

    if ( sFitEnableEnv != NULL )
    {
        sFitEnableEnvLen = idlOS::strlen( sFitEnableEnv );
        
        IDE_TEST_RAISE( idlVA::strnToSLong( sFitEnableEnv, sFitEnableEnvLen, &sFitEnable, NULL ) != 0, 
                                                                                ERR_LOAD_FIT_ENABLE );
        if ( sFitEnable == 1 )
        {
            mIsFitEnabled = IDU_FIT_TRUE;
        }
        else 
        {
            mIsFitEnabled = IDU_FIT_FALSE;
        }

    }
    else 
    {
        mIsFitEnabled = IDU_FIT_FALSE;
    }

    return mIsFitEnabled;

    IDE_EXCEPTION( ERR_LOAD_FIT_ENABLE ) 
    {
        IDU_FIT_DEBUG_LOG( "FIT_ENABLE Load Failure" );
    }

    IDE_EXCEPTION_END;

    return mIsFitEnabled;
}

#else

#define IDU_FIT_POINT( aUID, ... )                                        
#define IDU_FIT_POINT_RAISE( aUID, aLabel, ... )                          

/* PROJ-2617 */
#define IDU_FIT_POINT_FATAL( aUID )

#endif /* ALTIBASE_FIT_CHECK */

#endif /* _O_IDU_FIT_MANAGER_H_ */
