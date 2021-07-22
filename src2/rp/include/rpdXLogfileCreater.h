/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 * $Id: rpdXLogfileCreater.h
 ******************************************************************************/

#ifndef _O_RPD_XLOGFILECREATER_H_
#define _O_RPD_XLOGFILECREATER_H_ 1

#include <idtBaseThread.h>
#include <rpDef.h>
#include <qci.h>

/* xlogfile을 생성 및 삭제를 담당하는 thread */
class rpdXLogfileCreater : public idtBaseThread
{
    public:
        /* thread 기본 */
        rpdXLogfileCreater() : idtBaseThread() {};
        virtual ~rpdXLogfileCreater() {};
        IDE_RC  initializeThread() { return IDE_SUCCESS; };
        void    finalizeThread() {};
        void    run();

        /* xlogfileMgr */
        class rpdXLogfileMgr *    mXLogfileMgr;

        /* 초기화 및 정리 */
        IDE_RC  initialize( SChar *             aReplName,
                            rpXLogLSN           aXLogLSN,
                            rpdXLogfileMgr *    aXLogfileMgr,
                            void *              aReceiverPtr );
        void    finalize();
        void    destroy();

        /* create 작업 관련 */
        IDE_RC  createXLogfiles();
        IDE_RC  createXLogfile( UInt    aFileNo );

        /* 파일 삭제 관련 */
        IDE_RC  checkAndRemoveOldXLogfiles();
        IDE_RC  removeOldXLogfiles( UInt    aUsingXlogfileNo );
        IDE_RC  removeXLogfile( SChar *     aFilePath );
    private:
        IDE_RC  getOldestXLogfileNo( UInt   aFileNo );
    public:
        UInt    mOldestExistXLogfileNo;
        UInt    mFileCreateCount;

        /* 파일 삭제를 위해 receiver에 접근 */
        void *   mReceiverPtr;

        /* 흐름 제어 관련 */
        void    wakeupCreater();
        void    sleepCreater();
        idBool  isCreaterAlive();

        idBool      mExitFlag;
        idBool      mEndFlag;
        idBool      mIsThreadSleep;
        iduMutex    mThreadWaitMutex;
        iduCond     mThreadWaitCV;

        /* prePareFileCnt 관련 */
        void    getPrepareFileCnt( UInt *   aResult );
        void    incPrepareFileCnt();
        void    decPrepareFileCnt();

        UInt        mPrepareXLogfileCnt;
        iduMutex    mFileCntMutex;

        /* 새 xlogfile의 생성에 필요한 정보 */
        SChar *     mXLogDirPath;
        SChar       mRepName[QCI_MAX_NAME_LEN + 1];
        UInt        mLastCreatedFileNo;

        /* 기타 */
        static idBool   isFileExist( SChar * aFilePath );
};

inline idBool rpdXLogfileCreater::isCreaterAlive()
{
    if( mEndFlag == ID_TRUE )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

#endif // _O_RPD_XLOGFILECREATER_H_
