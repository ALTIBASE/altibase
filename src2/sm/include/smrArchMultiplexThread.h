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
 * $Id: smrArchMultiplexThread.h $
 **********************************************************************/

#ifndef _O_SMR_LOG_FILE_BACKUP_THREAD_H_
#define _O_SMR_LOG_FILE_BACKUP_THREAD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <smDef.h>
#include <smrLogFile.h>

typedef enum smrArchMultiplexThreadState
{
    SMR_LOG_FILE_BACKUP_THREAD_WAKEUP,
    SMR_LOG_FILE_BACKUP_THREAD_WAIT,
    SMR_LOG_FILE_BACKUP_THREAD_ERROR
}smrArchMultiplexThreadState;

class smrArchMultiplexThread : public idtBaseThread
{
//For Operation    
public:

    smrArchMultiplexThread();     
    virtual ~smrArchMultiplexThread();

    static IDE_RC performArchving( 
                            smrArchMultiplexThread * aArchMultiplexThread,
                            smrLogFile             * aSrcLogFile );

    static IDE_RC initialize( smrArchMultiplexThread ** aArchMultiplexThread,
                              const SChar            ** aArchPath,
                              UInt                      aThreadCnt );

    static IDE_RC destroy( smrArchMultiplexThread * aArchMultiplexThread );

    static IDE_RC startThread( smrArchMultiplexThread * aArchMultiplexThread );

    static IDE_RC shutdownThread( 
                            smrArchMultiplexThread * aArchMultiplexThread );

    virtual void run();

private:

    IDE_RC initializeThread( const SChar * aArchPath,
                             UInt          aMultiplexIdx );

    IDE_RC destroyThread();

    IDE_RC backupLogFile();

    IDE_RC wakeUp();

    IDE_RC lock() { return mMutex.lock(NULL); }

    IDE_RC unlock() { return mMutex.unlock(); }

    static IDE_RC wait( smrArchMultiplexThread * aArchMultiplexThread );

//For Member
private:

    /* thread member variable */
    const SChar                         * mArchivePath;
    UInt                                  mMultiplexIdx;
    volatile smrArchMultiplexThreadState  mThreadState;
    iduMutex                              mMutex;
    iduCond                               mCv;

    /* class member variable */
    static smrLogFile                   * mSrcLogFile;
    static UInt                           mMultiplexCnt;

public:
    static idBool                         mFinish;
};

#endif
