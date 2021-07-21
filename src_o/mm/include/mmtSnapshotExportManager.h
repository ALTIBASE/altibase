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

#ifndef _O_MMT_SNAPSHOT_EXPORT_MANAGER_H_
#define _O_MMT_SNAPSHOT_EXPORT_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idtBaseThread.h>
#include <idu.h>
#include <mmcSession.h>

#define MMT_SNAPSHOT_EXPORT_USAGE_BUFFER_SIZE (10)

class mmtSnapshotExportThread : public idtBaseThread
{
private:
    idBool        mRun;
    idBool        mIsBegin;
    smiTrans      mTrans;
    smiStatement  mSmiStmt;
public:

    mmtSnapshotExportThread();

    void initialize();
    void finalize();

    IDE_RC initializeThread();
    void   finalizeThread();

    void run();
    void stop();

    IDE_RC beginSnapshot( ULong * aSCN );
    IDE_RC endSnapshot( void );

    idBool isRun( void );
    idBool isBegin( void );
    idBool checkThreshold( idBool aCheckAll );
    IDE_RC getSCN( smSCN * aSCN );
};

typedef struct mmtSnapshotExportInfo
{
    ULong mSCN;
    SLong mBeginTime;
    SInt mBeginMemUsage;
    SInt mBeginDiskUndoUsage;

    SLong mCurrentTime;
    SInt  mCurrentMemUsage;
    SInt  mCurrentDiskUndoUsage;
} mmtSnapshotExportInfo;

class mmtSnapshotExportManager
{
private:
    static mmtSnapshotExportThread * mThread;
    static iduMutex                  mMutex;
public:
    static mmtSnapshotExportInfo     mInfo;

    static IDE_RC initialize();
    static IDE_RC finalize();
    static IDE_RC beginEndCallback( idBool aIsBegin );
    static IDE_RC getSnapshotSCN( smSCN * aSCN );
    static idBool isBeginSnapshot( void );
    static IDE_RC buildRecordForSnapshot(idvSQL              *,
                                         void                * aHeader,
                                         void                *,
                                         iduFixedTableMemory * aMemory );
    static inline void lock( void );
    static inline void unlock( void );
};

inline void mmtSnapshotExportManager::lock()
{
    IDE_ASSERT( mMutex.lock(NULL) == IDE_SUCCESS );
}

inline void mmtSnapshotExportManager::unlock()
{
    IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
}
#endif /* _O_MMT_SNAPSHOT_EXPORT_MANAGER_H_  */

