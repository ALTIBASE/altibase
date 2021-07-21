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
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKA_LINKER_PROCESS_MGR_H_
#define _O_DKA_LINKER_PROCESS_MGR_H_ 1

#include <dka.h>
#include <dkuProperty.h>

class dkaLinkerProcessMgr
{
private:
    static idBool              mIsPropertiesLoaded;
    static SChar               mServerInfoStr[DK_MSG_LEN];
    static UInt                mDBCharSet;
    static dkaLinkerProcInfo   mLinkerProcessInfo;
    static iduMutex            mDkaMutex;

    /* AltiLinker properties */
    static iduMutex            mConfMutex;    
    static dkcDblinkConf      *mConf;
    static UInt                mAltilinkerRestartNumber;

public:
    static IDE_RC       initializeStatic();
    static IDE_RC       finalizeStatic();

    /* Startup or shutdown AltiLinker process */
    static IDE_RC       startLinkerProcess();
    static IDE_RC       shutdownLinkerProcess( idBool   aFlag );
    static IDE_RC       dumpStackTrace();

    /* Load AltiLinker process properties */
    static IDE_RC       loadLinkerProperties();

    /* Get linker process information */
    static IDE_RC       getLinkerProcessInfo( dkaLinkerProcInfo **aInfo );

    static IDE_RC       getTargetInfo( SChar            *aTargetName,
                                       dkaTargetInfo    *aInfo );

    static UInt         getTargetsLength( dkcDblinkConfTarget *aTargetItem );
    static UInt         getTargetItemLength( dkcDblinkConfTarget *aTargetItem );
    static UInt         getTargetItemCount( dkcDblinkConfTarget *aTargetItem );

    /* Validata target info */
    static idBool       validateTargetInfo( SChar  *aTargetName );

    /* Get AltiLinker information */
    static inline dkaLinkerStatus  getLinkerStatus();
    static inline void  getLinkerSessionCount();
    static IDE_RC       getRemoteNodeSessionCountFromProtocol( UInt *aCount );
    static IDE_RC       getJvmMemUsageFromProtocol( UInt    *aJvmUsage );
    static IDE_RC       getJvmMemMaxSize( UInt  *aJvmMaxSize );
    static IDE_RC       getLinkerStartTime( SChar *aStartTime );

    /* Set linker process information */
    static inline void  setIsPropertiesLoaded( idBool   aIsLoaded );
    static inline void  initLinkerProcessInfo();
    static inline void  setLinkerStatus( dkaLinkerStatus aStatus );
    static inline void  setJvmMemMaxSize( UInt  aSize );
    static inline void  setLinkerStartTime();

    static inline void   resetDBCharSet();
    static inline UInt   getDBCharSet();
    static inline SChar* getServerInfoStr();

    /* Get Altilinker property values */
    static inline idBool isPropertiesLoaded();
    static inline idBool checkAltiLinkerEnabledFromConf();
    static IDE_RC getAltiLinkerPortNoFromConf( SInt *aPortNo );
    static IDE_RC getAltiLinkerReceiveTimeoutFromConf( SInt *aReceiveTimeout );
    static IDE_RC getAltiLinkerRemoteNodeReceiveTimeoutFromConf( SInt *RemoteNodeReceiveTimeout );
    static IDE_RC getAltiLinkerQueryTimeoutFromConf( SInt *aQueryTimeout );
    static IDE_RC getAltiLinkerNonQueryTimeoutFromConf( SInt *aNonQueryTimeout );
    static IDE_RC getAltiLinkerThreadCountFromConf( SInt *aThreadCount );
    static IDE_RC getAltiLinkerThreadSleepTimeFromConf( SInt *aThreadSleepTime );
    static IDE_RC getAltiLinkerRemoteNodeSessionCountFromConf( SInt *aRemoteNodeSessionCount );
    static IDE_RC getAltiLinkerTraceLogFileSizeFromConf( SInt *aTraceLogFileSize );
    static IDE_RC getAltiLinkerTraceLogFileCountFromConf( SInt *aTraceLogFileCount );
    static IDE_RC getAltiLinkerTraceLoggingLevelFromConf( SInt *aTraceLoggingLevel );
    static IDE_RC lockAndGetTargetListFromConf( dkcDblinkConfTarget **aTargetList );
    static void unlockTargetListFromConf( void );    

private:
    static IDE_RC checkAltilinkerRestartNumberAndReloadLinkerProperties( void );
    static IDE_RC startLinkerProcessInternal( SInt aMinus1onlyRetry,
                                              SInt *aPid );
    static IDE_RC shutdownLinkerProcessInternal( idBool  aFlag );
    static IDE_RC dumpStackTraceInternal( void );
    
    
};

inline void dkaLinkerProcessMgr::setIsPropertiesLoaded( idBool  aIsLoaded )
{
    mIsPropertiesLoaded = aIsLoaded;
}

inline void  dkaLinkerProcessMgr::initLinkerProcessInfo()
{
    mLinkerProcessInfo.mStatus               = DKA_LINKER_STATUS_NON;
    mLinkerProcessInfo.mLinkerSessionCnt     = 0;
    mLinkerProcessInfo.mRemoteNodeSessionCnt = 0;
    mLinkerProcessInfo.mJvmMemMaxSize        = 0;
    mLinkerProcessInfo.mJvmMemUsage          = 0;

    idlOS::memset( &mLinkerProcessInfo.mStartTime,
                   0,
                   ID_SIZEOF( mLinkerProcessInfo.mStartTime ) );
}

inline idBool dkaLinkerProcessMgr::isPropertiesLoaded()
{
    return mIsPropertiesLoaded;
}

inline void dkaLinkerProcessMgr::resetDBCharSet()
{
    mDBCharSet = mtl::mDBCharSet->id;
}

inline UInt dkaLinkerProcessMgr::getDBCharSet()
{
    return mDBCharSet;
}

inline SChar * dkaLinkerProcessMgr::getServerInfoStr()
{
    return mServerInfoStr;
}

inline dkaLinkerStatus dkaLinkerProcessMgr::getLinkerStatus()
{
    return mLinkerProcessInfo.mStatus;
}

inline void dkaLinkerProcessMgr::getLinkerSessionCount()
{
    mLinkerProcessInfo.mLinkerSessionCnt = dksSessionMgr::getLinkerSessionCount();
}

inline void dkaLinkerProcessMgr::setLinkerStatus( dkaLinkerStatus aStatus )
{
    mLinkerProcessInfo.mStatus = aStatus;
}

inline void dkaLinkerProcessMgr::setJvmMemMaxSize( UInt aSize )
{
    mLinkerProcessInfo.mJvmMemMaxSize = aSize;
}

inline void dkaLinkerProcessMgr::setLinkerStartTime()
{
    PDL_Time_Value  sTimeValue;
    struct tm       sLocalTime;
    time_t          sTime;

    sTimeValue = idlOS::gettimeofday();
    sTime      = (time_t)sTimeValue.sec();
    idlOS::localtime_r( &sTime, &sLocalTime );

    idlOS::snprintf( mLinkerProcessInfo.mStartTime,
                     ID_SIZEOF( mLinkerProcessInfo.mStartTime ),
                     "%4"ID_UINT32_FMT  "/"
                     "%02"ID_UINT32_FMT "/"
                     "%02"ID_UINT32_FMT ","
                     "%02"ID_UINT32_FMT ":"
                     "%02"ID_UINT32_FMT ":"
                     "%02"ID_UINT32_FMT " ",
                     sLocalTime.tm_year + 1900,
                     sLocalTime.tm_mon + 1,
                     sLocalTime.tm_mday,
                     sLocalTime.tm_hour,
                     sLocalTime.tm_min,
                     sLocalTime.tm_sec );
}

inline idBool dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf()
{
    idBool sResult = ID_FALSE;

    IDE_ASSERT( mConfMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    if ( mConf->mAltilinkerEnable == 0 )
    {
        sResult = ID_FALSE;
    }
    else
    {
        sResult = ID_TRUE;
    }

    IDE_ASSERT( mConfMutex.unlock() == IDE_SUCCESS );
    
    return sResult; 
}

#endif /* _O_DKA_LINKER_PROCESS_MGR_H_ */
