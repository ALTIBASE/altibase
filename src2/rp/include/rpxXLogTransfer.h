/*
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

#ifndef _O_RPX_XLOGTRANSFER_H_
#define _O_RPX_XLOGTRANSFER_H_ 1

#include <idtBaseThread.h>

#include <cm.h>

#include <rp.h>
#include <rpxReceiver.h>
#include <rpdXLogfileMgr.h>

class rpxXLogTransfer : public idtBaseThread
{
public:
    rpxXLogTransfer();
    virtual ~rpxXLogTransfer() {};

    IDE_RC initialize( rpxReceiver * aReceiver, rpdXLogfileMgr * aXLogfileManager );
    IDE_RC finalize();

    void destroy();

    IDE_RC initializeThread();
    void   finalizeThread();

    void run();

    inline void setExit( idBool aIsExitFlag ) { *mExitFlag = aIsExitFlag; }
    inline idBool isExit() { return  * mExitFlag; }

    //move semantic network resources, called by receiver, Ack를 전송하는 send Socket에 대해서 공유가 필요하므로, sendAck를 함수 진행 중이 아닌 경우, 넘겨준다.
    cmiProtocolContext * takeAwayNetworkResources(); //Transfer -> Receiver, Sync가 시작하기 전 호출, DDL replication 이나 기타 receiver에서 작업이 필요한 경우 호출
    IDE_RC handOverNetworkResources( cmiProtocolContext * aProtocolContext ); //Receiver -> Transfer, Sync가 끝난 후 호출, DDL replication 이나 기타 receiver에서 필요한 작업이 끝난 후 호출

    IDE_RC waitForReceiverProcessDone();
    idBool isWaitFromReceiverProcessDone();
    IDE_RC signalToWaitedTransfer();

    //receiver must call below function periodically
    void setLastProcessedSN( smSN aLastProcessedSN ) { mLastProcessedSN = aLastProcessedSN; }
    void setRestartSN( smSN aRestartSN ) { mRestartSN = aRestartSN; }

private:
    IDE_RC lockForWaitReceiverProcessDone();
    IDE_RC unlockForWaitReceiverProcessDone();

    IDE_RC getStatusFile();

    IDE_RC recvXLog();

    void setCurrentPositionFromReadBlockInCMContext( cmiProtocolContext *aProtocolContext, UChar * aPosition );
    IDE_RC writeXLog( cmiProtocolContext * aProtocolContext, UShort aWriteSize, smSN aXSN );
    IDE_RC writeValuesXLog( cmiProtocolContext * aProtocolContext );
    IDE_RC writeSPSetLog( cmiProtocolContext * aProtocolContext, smSN aXSN );
    IDE_RC writeSPAbortLog( cmiProtocolContext * aProtocolContext, smSN aXSN );
    IDE_RC writeLobPartialWriteXLog( cmiProtocolContext * aProtocolContext );

    IDE_RC receiveCMBufferAndWriteXLog( cmiProtocolContext * aProtocolContext,
                                        rpXLogType         * aXLogType );

    IDE_RC processXLogInXLogTransfer( rpXLogType aXLogType );

    void increaseProcessedXLogCount() { mProcessedXLogCount++;}
    void resetProcessedXLogCount() { mProcessedXLogCount = 0;}
    UInt getProcessedXLogCount() { return mProcessedXLogCount; }

    IDE_RC buildAndSendAck( rpXLogType aType );

    void buildAck( rpXLogAck * aAckXLog, rpXLogType aType );
    IDE_RC sendAck( rpXLogAck * aAckXLog );

//variable
private:
    rpxReceiver * mReceiver;

    rpdXLogfileMgr * mXLogfileManager;

    idBool * mExitFlag;

    SChar mRepName[QCI_MAX_OBJECT_NAME_LEN + 1];

    cmiProtocolContext * mProtocolContext;
    cmiLink * mLink;

    idBool *mNetworkError;

    smSN mLastCommitSN;
    smTID mLastCommitTID;
    smSN mLastWrittenSN;

    smSN mLastProcessedSN;
    smSN mRestartSN;
    smSN mFlushSN;

    UInt  mProcessedXLogCount;

public:
    iduCond mWaitForReceiverProcessDoneCV;
    iduMutex mWaitForReceiverProcessDoneMutex;
    idBool mIsSendAckThroughReceiver;

    SChar * getRepName() { return mRepName; }
    smTID getLastCommitTID() { return mLastCommitTID; }
    smSN getLastCommitSN() { return mLastCommitSN; }
    smSN getLastWrittenSN() { return mLastWrittenSN; }
    smSN getRestartSN() { return mRestartSN; }
    UInt getLastWrittenFileNo();
    UInt getLastWrittenFileOffset();
    UInt getNetworkResourceStatus() { return mProtocolContext != NULL ? 1:0; }

};

#endif /* _O_RPX_XLOGTRANSFER_H_ */
