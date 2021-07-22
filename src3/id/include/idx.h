/***********************************************************************
 * Copyright 1999-2013, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idx.h 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#ifndef _O_IDX_H_
#define  _O_IDX_H_  1

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <idl.h>
#include <idTypes.h>
#include <idxLocalSock.h>
#include <iduProperty.h>
#include <iduMemory.h>

#define IDX_AGENT_PATH_MAXLEN       1024
#define IDX_LIB_PATH_MAXLEN         1024    // TODO < full-path of altibase
#define IDX_SOCK_PATH_MAXLEN        1024    // TODO < full-path of altibase
#define IDX_LIB_NAME_MAXLEN         128     // = QC_MAX_OBJECT_NAME_LEN
#define IDX_SOCK_NAME_MAXLEN        64      // socket_number (session id)
#define IDX_FUNC_NAME_MAXLEN        128
#define IDX_PROPERTY_NAME_MAXLEN    10      // longest name is 'INDICATOR'


/* Socket connection timeout */
#define IDX_WAITTIME_PER_CONN       20     // 20 ms

#define IDX_CONNECTION_MAX_COUNT  \
    (((IDU_EXTPROC_AGENT_CONNECT_TIMEOUT) * 1000) / IDX_WAITTIME_PER_CONN)

#define IDX_AGENT_WAITING_COUNT   \
    (((IDU_EXTPROC_AGENT_IDLE_TIMEOUT) * 1000) / IDX_WAITTIME_PER_CONN)

#define IDX_RETURN_ORDER        -1

/* agent, library, and socket path with separators TODO */
#define IDX_AGENT_DEFAULT_DIR   "/bin/"
#define IDX_LIB_DEFAULT_DIR     "/lib/"

#define IDX_SOCK_NAME_PREFIX    "socket"

#if defined(ALTI_CFG_OS_WINDOWS)
#define IDX_AGENT_PROC_NAME         PRODUCT_PREFIX"extprocAgent.exe"
#define IDX_CONV_SOCK_NAME(name)    (ASYS_TCHAR *)PDL_TEXT(name)
#define IDX_PIPEPATH                "\\\\.\\Pipe\\"
#else
#define IDX_AGENT_PROC_NAME         PRODUCT_PREFIX"extprocAgent"
#define IDX_CONV_SOCK_NAME(name)    name
#define IDX_PIPEPATH                "."
#endif /* ALTI_CFG_OS_WINDOWS */

/* BUG 36962 TRUE/FALSE reverse */
#define IDX_BOOLEAN_TRUE            (UChar)0x01
#define IDX_BOOLEAN_FALSE           (UChar)0x00

// for referential action
typedef enum idxParamType {
    IDX_TYPE_NONE = 0,
    IDX_TYPE_CHAR,
    IDX_TYPE_BOOL,
    IDX_TYPE_SHORT,
    IDX_TYPE_INT,
    IDX_TYPE_INT64,
    IDX_TYPE_FLOAT,
    IDX_TYPE_DOUBLE,
    IDX_TYPE_NUMERIC,
    IDX_TYPE_TIMESTAMP,
    IDX_TYPE_LOB,            // BUG-39814
    IDX_TYPE_BYTE            // PROJ-2717 Internal procedure
} idxParamType;

typedef enum idxParamPropType {
    IDX_TYPE_PROP_NONE = 0,
    IDX_TYPE_PROP_IND,
    IDX_TYPE_PROP_LEN,
    IDX_TYPE_PROP_MAX
} idxParamPropType;

typedef enum idxInOutType {
    IDX_MODE_NONE = 0,
    IDX_MODE_IN,
    IDX_MODE_OUT,
    IDX_MODE_INOUT
} idxInOutType;

typedef enum idxProcState {
    IDX_PROC_INITED = 0,    /* initialized */
    IDX_PROC_RUNNING,       /* processing with a msg */
    IDX_PROC_STOPPED,       /* stopped and wait new msg */
    IDX_PROC_FAILED,        /* got an exception */
    IDX_PROC_ALLOC          /* alloc'd but not initialized yet */
} idxProcState;

/* same with SQL_TIMESTAMP_STRUCT */
typedef struct idxTimestamp
{
    SShort      mYear;
    UShort      mMonth;
    UShort      mDay;
    UShort      mHour;
    UShort      mMinute;
    UShort      mSecond;
    UInt        mFraction;
} idxTimestamp;

#define IDX_INIT_TIMESTAMP( _idxTimestamp_ ) \
{                                            \
    _idxTimestamp_.mYear     = 0;            \
    _idxTimestamp_.mMonth    = 0;            \
    _idxTimestamp_.mDay      = 0;            \
    _idxTimestamp_.mHour     = 0;            \
    _idxTimestamp_.mMinute   = 0;            \
    _idxTimestamp_.mSecond   = 0;            \
    _idxTimestamp_.mFraction = 0;            \
}

typedef struct idxParamInfo {
    UInt                mSize;
    SShort              mColumn;
    SShort              mTable;
    SInt                mOrder;         /* -1 : RETURN */
    idxInOutType        mMode;
    idxParamType        mType;
    idxParamPropType    mPropType;
    idBool              mIsPtr;

    SShort              mIndicator;     /* ID_TRUE : NULL */
    SLong               mLength;
    SLong               mMaxLength;

    union {
        UChar               mBool;
        SShort              mShort;
        SInt                mInt;
        SLong               mLong;
        SFloat              mFloat;
        SDouble             mDouble;
        idxTimestamp        mTimestamp;   // date, interval
        void              * mPointer;     // char, pointer
    } mD;
} idxParamInfo;

typedef struct idxIntProcMsg {
    UInt                mParamCount;                        /* all parameter count for calling C function */
    SChar             * mLibName;                           /* library name */
    SChar             * mFuncName;                          /* function name */
    PDL_SHLIB_HANDLE    mHandle;
    void              * mFuncPtr;
    idxParamInfo      * mParamInfos;                        /* parameter info(s) for calling C function */
    idxParamInfo        mReturnInfo;                        /* return info(s) for calling C function */
} idxIntProcMsg;

typedef struct idxExtProcMsg {
    UInt                mErrorCode;                         /* error code */
    SChar               mLibName[IDX_LIB_NAME_MAXLEN + 1];  /* library name */
    SChar               mFuncName[IDX_FUNC_NAME_MAXLEN + 1];/* function name */
    UInt                mParamCount;                        /* all parameter count for calling C function */
    idxParamInfo        mReturnInfo;                        /* return info(s) for calling C function */
    idxParamInfo      * mParamInfos;                        /* parameter info(s) for calling C function */
} idxExtProcMsg;

typedef struct idxAgentProc idxAgentProc;

struct idxAgentProc
{
    UInt            mSID;                               /* Session ID */
    UInt            mPID;                               /* Process ID (pid_t or DWORD) */
    UInt            mCreated;                           /* created timestamp */
    UInt            mLastReceived;                      /* timestamp when the latest msg is received */
    UInt            mLastSent;                          /* timestamp when the latest msg is sent */
    idxProcState    mState;                             /* state (it depends on idx module) */
    IDX_LOCALSOCK   mSocket;                            /* connected socket */
    SChar           mSockFile[IDX_SOCK_NAME_MAXLEN];    /* path of session file */
    SChar         * mTempBuffer;                        /* temp buffer for ping */
};

class idxProc
{
    public:
        static idxAgentProc * mAgentProcList;
        static SChar          mAgentPath[IDX_AGENT_PATH_MAXLEN];
        static SChar        * mTempBuffer;

        // initialize class
        static IDE_RC initializeStatic();

        // finalize class
        static void   destroyStatic();

        /* Set pointers of each parameters (by its type) */
        static inline IDE_RC setParamPtr( idxParamInfo *aParam, void ** aOutPtr )
        {
            switch( aParam->mType )
            {
                case IDX_TYPE_BOOL:
                    *aOutPtr = &aParam->mD.mBool;
                    break;
                case IDX_TYPE_SHORT:
                    *aOutPtr = &aParam->mD.mShort;
                    break;
                case IDX_TYPE_INT:
                    *aOutPtr = &aParam->mD.mInt;
                    break;
                case IDX_TYPE_INT64:
                    *aOutPtr = &aParam->mD.mLong;
                    break;
                case IDX_TYPE_FLOAT:
                    *aOutPtr = &aParam->mD.mFloat;
                    break;
                case IDX_TYPE_DOUBLE:
                case IDX_TYPE_NUMERIC:
                    *aOutPtr = &aParam->mD.mDouble;
                    break;
                case IDX_TYPE_TIMESTAMP:
                    *aOutPtr = &aParam->mD.mTimestamp;
                    break;
                case IDX_TYPE_CHAR:
                case IDX_TYPE_LOB:   // BUG-39814 IN mode LOB Parameter in Extproc
                case IDX_TYPE_BYTE:  // PROJ-2717 Internal procedure
                    *aOutPtr =  aParam->mD.mPointer;
                    break;
                default:
                    break;
            }

            if( *aOutPtr == NULL )
            {
                return IDE_FAILURE;
            }
            else
            {
                return IDE_SUCCESS;
            }
        }

        /* Backup original property values of each parameters */ 
        static inline void backupParamProperty( idxParamInfo *aDestParams,
                                                idxParamInfo *aSrcParams,
                                                UInt          aParamCount )
        {
            UInt i = 0;

            for( i = 0; i < aParamCount; i++ )
            {
                idlOS::memcpy( &aDestParams[i],
                               &aSrcParams[i],
                               ID_SIZEOF(idxParamInfo) );
            }
        }

        /* ---------------------------------------------------------------------
         * Interfaces for an agent process
         * --------------------------------------------------------------------*/
        static IDE_RC createAgentProcess( UInt            aSessionID,
                                          idxAgentProc ** aAgentProc, 
                                          iduMemory     * aExeMem );

        static void   destroyAgentProcess( UInt aSessionID );

        static IDE_RC getAgentProcess( UInt            aSessionID,
                                       idxAgentProc ** aAgentProc,
                                       idBool        * aIsFirstTime );

        static IDE_RC terminateProcess( idxAgentProc * aAgentProc );

        /* ---------------------------------------------------------------------
         * Interfaces for an array of parameter information
         * --------------------------------------------------------------------*/
        static void   copyParamInfo( idxParamInfo * aSrcParam,
                                     idxParamInfo * aDestParam );

        /* ---------------------------------------------------------------------
         * Interfaces for a message
         * --------------------------------------------------------------------*/

        static IDE_RC packExtProcMsg( iduMemory      * aExeMem,
                                      idxExtProcMsg  * aMsg,
                                      SChar         ** aOutBuffer );

        static IDE_RC packExtProcErrorMsg( iduMemory      * aExeMem,
                                           UInt             aErrorCode,
                                           SChar         ** aOutBuffer );

        static IDE_RC unpackExtProcMsg( iduMemory      * aExeMem,
                                        SChar          * aInBuffer,
                                        idxExtProcMsg ** aMsg );

        static IDE_RC setParamType( UInt   aParamType, // qsInOutType
                                    UInt * aReturnType );

        /* ---------------------------------------------------------------------
         * Interfaces for calling
         * --------------------------------------------------------------------*/
        static IDE_RC callExtProc( iduMemory      * aExeMem,
                                   UInt             aSessionID,
                                   idxExtProcMsg  * aMsg );

};

#endif /* _O_IDX_H_ */
