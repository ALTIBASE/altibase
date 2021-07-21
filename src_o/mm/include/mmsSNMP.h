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

#ifndef _O_MMS_SNMP_H_
#define _O_MMS_SNMP_H_ 1

#include <ide.h>
#include <idu.h>
#include <idtBaseThread.h>

/* 
 * PROJ-2473 SNMP 지원
 */

class  mmsSNMP;
extern mmsSNMP gMmsSNMP;

class mmsSNMP : public idtBaseThread
{
private:
    PDL_SOCKET       mSock;
    struct sockaddr  mClientAddr;
    SInt             mClientAddrLen;
    SChar           *mClientAddrStr;

    UInt   mSNMPPortNo;
    UInt   mSNMPTrapPortNo;
    UInt   mSNMPRecvTimeout;
    UInt   mSNMPSendTimeout;

    /* mPacketBuf는 데이터를 주고 받을 때 사용한다 */
    UChar *mPacketBuf;
    SInt   mPacketBufSize;

    /* idm에서 얻어오는 값을 저장 */
    UChar *mValueBuf;
    SInt   mValueBufSize;

    /* Process protocols */
    SInt   processSNMP(UInt aRecvPacketLen);

    /* utils */
    void   dumpSNMPProtocol(UInt aRecvPacketLen);
    void   hexDumpSNMPInvalidProtocol(SInt aLen);

    /* Start thread */
    idBool mRun;
    void   run();
    IDE_RC startSNMPThread();

public:
    mmsSNMP()  {};
    ~mmsSNMP() {};

    IDE_RC initialize();
    IDE_RC finalize();

    /* BUG-38641 Apply PROJ-2379 Thread Renewal on MM */
    IDE_RC initializeThread();
    void   finalizeThread();
};

#endif /* _O_MMS_SNMP_H_ */
