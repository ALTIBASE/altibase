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

#include <mmsSNMP.h>
#include <mmErrorCode.h>
#include <idm.h>
#include <idmSNMP.h>

#define PACKET_BUF_SIZE    (16384 * ID_SIZEOF(*mPacketBuf))
#define OID_MAX_LEN        (128)

/*
 * PROJ-2473 SNMP 지원
 *
 * ALTIBASE와 SNMP SUB-AGENT for ALTIBASE간의 통신을 위한 프로토콜이며
 * 같은 장비에서 수행되기 때문에 ENDIAN 변환은 필요치 않다.
 *
 * MMS Protocol - REQUEST
 *
 *  Protocol ID   (GET, SET, GETNEXT)  - UINT           -----+
 *  OID Length                         - UINT                |
 *  OID[0]                             - oid(vULong)         |--- mmsProtocol
 *  OID[1]                             - oid(vULong)         |
 *  OID[x]                             - oid(vULong)    -----+
 *  --------------------------------------------------
 *  DATA Type                          - UINT           -----+
 *  DATA Length                        - UINT                |--- mmsData
 *  DATA                               - UCHAR          -----+
 *
 *  OID는 가변이며 최대개수는 128개. 128개를 고정으로 잡는게 나을지두...
 *  DATA 역시 가변이다.
 *
 *
 * MMS Protocol - RESPONSE
 *
 *  Protocol ID   (SUCCESS, FAILURE)   - UINT           -----+
 *  OID Length                         - UINT                |
 *  OID[0]                             - oid(vULong)         |--- mmsProtocol
 *  OID[1]                             - oid(vULong)         |
 *  OID[x]                             - oid(vULong)    -----+
 *  --------------------------------------------------
 *  DATA Type                          - UINT           -----+
 *  DATA Length                        - UINT                |--- mmsData
 *  DATA                               - UCHAR          -----+
 */

enum
{
    MMS_SNMP_PROTOCOL_GET     = 1,
    MMS_SNMP_PROTOCOL_GETNEXT = 2,
    MMS_SNMP_PROTOCOL_SET     = 3,

    MMS_SNMP_PROTOCOL_SUCCESS = 101,
    MMS_SNMP_PROTOCOL_FAILURE = 102
};

/* from idmDef.h
struct idmId {
    UInt length;
    oid  id[1];
};
*/

typedef struct
{
    UInt  mProtocolID;
    idmId mOID;
} mmsProtocol;

typedef struct
{
    UInt  mValueType;
    UInt  mValueLen;
    UChar mValue[4];  /* Padding 때문에 4의 배수로... */
} mmsData;

/* PROJ-2473 SNMP 지원 */
mmsSNMP gMmsSNMP;

/* 
 * LOG LEVEL
 *
 * IDE_SNMP_0 - 초기 정보 및 심각한 에러 (시스템콜)
 * IDE_SNMP_1 - 동작에 영향을 주지 않는 에러 (INVALID Protocol)
 * IDE_SNMP_2 - 패킷 정보 (DUMP Protocol)
 * IDE_SNMP_3 - Not used yet.
 * IDE_SNMP_4 - Not used yet.
 */

/**
 *  mmsSNMP::processSNMP
 *
 *  SNMP Sub Agent의 요청을 처리한다.
 */
SInt mmsSNMP::processSNMP(UInt aRecvPacketLen)
{
    mmsProtocol    *sProtocol    = NULL;
    UInt            sProtocolLen = 0;
    mmsData        *sData        = NULL;
    UInt            sDataLen     = 0;

    /* idm에서 얻어오는 값을 저장 */
    UInt            sValueType;
    UInt            sValueLen;
    UChar          *sValue     = mValueBuf;
    SInt            sValueSize = mValueBufSize;

    oid             sOIDBuf[OID_MAX_LEN + 1];
    idmId          *sNextOID    = (idmId *)sOIDBuf;
    UInt            sNextOIDLen = 0;

    UInt            sSendPacketLen = 0;

    /* static buf가 리턴된다 */
    mClientAddrStr = idlOS::inet_ntoa(((struct sockaddr_in *)&mClientAddr)->sin_addr);

    /* UDP는 데이터가 분할되는 경우는 없다. */
    IDE_TEST_RAISE(aRecvPacketLen < ID_SIZEOF(*sProtocol), ERR_SNMP_INVALID_PROTOCOL);

    sProtocol    = (mmsProtocol *)mPacketBuf;
    sProtocolLen = ID_SIZEOF(*sProtocol) -
                   ID_SIZEOF(sProtocol->mOID.id) +
                   sProtocol->mOID.length * ID_SIZEOF(sProtocol->mOID.id);

    /* UDP는 데이터가 분할되는 경우는 없다. */
    IDE_TEST_RAISE(aRecvPacketLen < sProtocolLen, ERR_SNMP_INVALID_PROTOCOL);

    /* OID가 가변이기 때문에 mmsData의 위치를 계산해 주어야 한다 */
    sData = (mmsData *)(mPacketBuf + sProtocolLen);

    /* 프로토콜 로그 출력 */
    dumpSNMPProtocol(aRecvPacketLen);

    switch (sProtocol->mProtocolID)
    {
        case MMS_SNMP_PROTOCOL_GET:
            IDE_TEST_RAISE(idm::get(&sProtocol->mOID,
                                    &sValueType,
                                    sValue,
                                    &sValueLen,
                                    sValueSize) != IDE_SUCCESS,
                           ERR_IDM_FAILURE);

            sProtocol->mProtocolID = MMS_SNMP_PROTOCOL_SUCCESS;

            sData->mValueType = sValueType;
            sData->mValueLen  = sValueLen;
            idlOS::memcpy(sData->mValue, sValue, sValueLen);

            sDataLen = ID_SIZEOF(*sData) - ID_SIZEOF(sData->mValue) + sData->mValueLen;
            sSendPacketLen = sProtocolLen + sDataLen;
            break;

        case MMS_SNMP_PROTOCOL_GETNEXT:
            IDE_TEST_RAISE(idm::getNext(&sProtocol->mOID,
                                        sNextOID,
                                        OID_MAX_LEN,
                                        &sValueType,
                                        sValue,
                                        &sValueLen,
                                        sValueSize) != IDE_SUCCESS,
                           ERR_IDM_FAILURE);

            sProtocol->mProtocolID = MMS_SNMP_PROTOCOL_SUCCESS;

            /* NEXT OID를 보내기 위해 Packet Buffer에 복사한다. */
            sProtocol->mOID.length = sNextOID->length;
            sNextOIDLen = sNextOID->length * ID_SIZEOF(sNextOID->id);
            idlOS::memcpy(&sProtocol->mOID.id, sNextOID->id, sNextOIDLen);

            /* OID가 변경되었기에 다시 계산해 주어야 한다. */
            sProtocolLen = ID_SIZEOF(*sProtocol) -
                            ID_SIZEOF(sProtocol->mOID.id) +
                            sProtocol->mOID.length * ID_SIZEOF(sProtocol->mOID.id);
            sData        = (mmsData *)(mPacketBuf + sProtocolLen);

            sData->mValueType = sValueType;
            sData->mValueLen  = sValueLen;
            idlOS::memcpy(sData->mValue, sValue, sValueLen);

            sDataLen = ID_SIZEOF(*sData) - ID_SIZEOF(sData->mValue) + sData->mValueLen;
            sSendPacketLen = sProtocolLen + sDataLen;
            break;

        case MMS_SNMP_PROTOCOL_SET:
            /* mmsData의 유효성 체크 */
            sDataLen = ID_SIZEOF(*sData) - ID_SIZEOF(sData->mValue) + 1;
            IDE_TEST_RAISE(aRecvPacketLen < sProtocolLen + sDataLen,
                           ERR_SNMP_INVALID_PROTOCOL);

            /* 계산된 사이즈가 aRecvPacketLen보다 클 수 없다 */
            sDataLen = ID_SIZEOF(*sData) - ID_SIZEOF(sData->mValue) + sData->mValueLen;
            IDE_TEST_RAISE(aRecvPacketLen < sProtocolLen + sDataLen,
                           ERR_SNMP_INVALID_PROTOCOL);

            IDE_TEST_RAISE(idm::set(&sProtocol->mOID,
                                    sData->mValueType,
                                    sData->mValue,
                                    sData->mValueLen) != IDE_SUCCESS,
                           ERR_IDM_FAILURE);

            sProtocol->mProtocolID = MMS_SNMP_PROTOCOL_SUCCESS;
            sSendPacketLen = sProtocolLen;
            break;

        default:
            IDE_RAISE(ERR_SNMP_INVALID_PROTOCOL);
            break;
    }

    return sSendPacketLen;

    IDE_EXCEPTION(ERR_SNMP_INVALID_PROTOCOL)
    {
        hexDumpSNMPInvalidProtocol(aRecvPacketLen);
        sSendPacketLen = 0;
    }
    IDE_EXCEPTION(ERR_IDM_FAILURE)
    {
        ideLog::log(IDE_SNMP_0, MM_TRC_SNMP_ERR, ideGetErrorMsg());

        sProtocol->mProtocolID = MMS_SNMP_PROTOCOL_FAILURE;
        sSendPacketLen = sProtocolLen;
    }
    IDE_EXCEPTION_END;

    return sSendPacketLen;
}

/**
 * dumpSNMPProtocol
 */
void mmsSNMP::dumpSNMPProtocol(UInt aRecvPacketLen)
{
    mmsProtocol *sProtocol;
    SChar       *sProtocolIDStr;
    SChar        sOIDStr[1024];
    UInt         i;
    SInt         sIndex = 0;

    sProtocol = (mmsProtocol *)mPacketBuf;

    switch (sProtocol->mProtocolID)
    {
        case MMS_SNMP_PROTOCOL_GET:
            sProtocolIDStr = (SChar *)"GET";
            break;

        case MMS_SNMP_PROTOCOL_GETNEXT:
            sProtocolIDStr = (SChar *)"GETNEXT";
            break;

        case MMS_SNMP_PROTOCOL_SET:
            sProtocolIDStr = (SChar *)"SET";
            break;

        default:
            sProtocolIDStr = (SChar *)"INVAILD";
            break;
    }

    for (i = 0; i < sProtocol->mOID.length; i++)
    {
        sIndex += idlOS::snprintf(&sOIDStr[sIndex], sizeof(sOIDStr) - sIndex,
                                  "%"ID_vULONG_FMT".",
                                  sProtocol->mOID.id[i]);
    }
    /* 마지막 dot는 제거 */
    if (sIndex > 0)
    {
        sOIDStr[sIndex - 1] = '\0';
    }
    else
    {
        /* Nothing */
    }

    ideLog::log(IDE_SNMP_2, MM_TRC_SNMP_DUMP_PROTOCOL,
                sProtocolIDStr,
                sOIDStr,
                aRecvPacketLen,
                mClientAddrStr);

    return;
}

/**
 * hexDumpSNMPInvalidProtocol
 */
void mmsSNMP::hexDumpSNMPInvalidProtocol(SInt aLen)
{
    static UChar sHex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    UChar sHexDump[256];
    SInt  i;
    SInt  j;

    IDE_TEST(aLen <= 0);

    for (i = 0, j = 0; i < aLen && j < 253; i++)
    {
        sHexDump[j++] = sHex[mPacketBuf[i] & 0xF0 >> 4];
        sHexDump[j++] = sHex[mPacketBuf[i] & 0x0F >> 0];
    }
    sHexDump[j] = '\0';

    ideLog::log(IDE_SNMP_1, MM_TRC_SNMP_INVALID_PROTOCOL,
                aLen,
                sHexDump,
                mClientAddrStr);

    return;

    IDE_EXCEPTION_END;

    return;
}


/**
 * mmsSNMP::run
 */
void mmsSNMP::run()
{
    fd_set          sReadFdSet;
    fd_set          sWriteFdSet;
    PDL_Time_Value  sTimeout;
    UInt            sSelectRet = SNMP_SELECT_ERR;

    UInt            sRet = 0;
    UInt            sRecvPacketLen = 0;
    UInt            sSendPacketLen = 0;

    FD_ZERO(&sReadFdSet);
    FD_ZERO(&sWriteFdSet);

    while (mRun == ID_TRUE)
    {
        mClientAddrLen = ID_SIZEOF(mClientAddr);
        sTimeout.msec(mSNMPRecvTimeout);
        FD_SET(mSock, &sReadFdSet);

        sSelectRet = idmSNMP::selectSNMP(mSock,
                                         &sReadFdSet, NULL,
                                         sTimeout, (SChar *)"SNMP_RECV");

        if ((sSelectRet & SNMP_SELECT_READ) == SNMP_SELECT_READ)
        {
            sRecvPacketLen = idlOS::recvfrom(mSock,
                                             (SChar *)mPacketBuf,
                                             mPacketBufSize,
                                             0,
                                             &mClientAddr,
                                             &mClientAddrLen);

            if (sRecvPacketLen > 0)
            {
                /* 패킷을 처리한다. */
                sSendPacketLen = processSNMP(sRecvPacketLen);
            }
            else
            {
                /* Non-rechable */
                ideLog::log(IDE_SNMP_0,
                            "SNMP: RECV RET: %"ID_UINT32_FMT" (%"ID_UINT32_FMT")",
                            sRecvPacketLen,
                            errno);

                continue;
            }
        }
        else
        {
            /* Timeout or Select err */
            continue;
        }

        /* processSNMP()에서 요청에 대한 응답을 보내는 경우 */
        if (sSendPacketLen > 0)
        {
            sTimeout.msec(mSNMPSendTimeout);
            FD_SET(mSock, &sWriteFdSet);

            /* sendto()에서 select는 큰 의미가 없다 */
            sSelectRet = idmSNMP::selectSNMP(mSock,
                                             NULL, &sWriteFdSet,
                                             sTimeout, (SChar *)"SNMP_SEND");

            if ((sSelectRet & SNMP_SELECT_WRITE) == SNMP_SELECT_WRITE)
            {
                sRet = idlOS::sendto(mSock,
                                     (SChar *)mPacketBuf,
                                     sSendPacketLen,
                                     0,
                                     &mClientAddr,
                                     ID_SIZEOF(mClientAddr));

                if (sRet != sSendPacketLen)
                {
                    /* Non-rechable */
                    ideLog::log(IDE_SNMP_0,
                                "SNMP: SEND RET: %"ID_UINT32_FMT" (%"ID_UINT32_FMT")",
                                sRet,
                                errno);
                }
                else
                {
                    /* Nothing */
                }
            }
            else
            {
                /* Timeout or Select err - maybe non-reachable */
            }
        }
        else
        {
            /* Nothing */
        }
    }

    idlOS::closesocket(mSock);
    mSock = PDL_INVALID_SOCKET;

    return;
}

/**
 * mmsSNMP::startSNMPThread
 */
IDE_RC mmsSNMP::startSNMPThread()
{
    struct sockaddr_in sListenAddr;

    idlOS::memset(&sListenAddr, 0, ID_SIZEOF(sListenAddr));

    sListenAddr.sin_family      = AF_INET;
    sListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sListenAddr.sin_port        = htons(mSNMPPortNo);

    /* SNMP Sub Agent와 UDP 통신을 위한 소켓생성 및 바인드 */
    mSock = idlOS::socket(AF_INET, SOCK_DGRAM, 0);
    IDE_TEST_RAISE(mSock == PDL_INVALID_SOCKET, ERR_SOCKET_CREATE_FAILED);

    IDE_TEST_RAISE(idlOS::bind(mSock,
                               (struct sockaddr *)&sListenAddr,
                               ID_SIZEOF(sListenAddr)) < 0,
                   ERR_SOCKET_BIND_FAILED);

    /* Thread 구동 */
    mRun = ID_TRUE;
    IDE_TEST_RAISE(gMmsSNMP.start() != IDE_SUCCESS, ERR_THREAD_CREATE_FAILED);
    IDE_TEST_RAISE(gMmsSNMP.waitToStart() != IDE_SUCCESS, ERR_THREAD_CREATE_FAILED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SOCKET_CREATE_FAILED)
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_SOCKET_CREATE_FAILED));
    }
    IDE_EXCEPTION(ERR_SOCKET_BIND_FAILED)
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_SOCKET_BIND_FAILED));
    }
    IDE_EXCEPTION(ERR_THREAD_CREATE_FAILED)
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CREATE_FAILED));
    }
    IDE_EXCEPTION_END;

    if (mSock != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(mSock);
        mSock = PDL_INVALID_SOCKET;
    }
    else
    {
        /* Nothing */
    }

    return IDE_FAILURE;
}


/**
 * mmsSNMP::initialize()
 */
IDE_RC mmsSNMP::initialize()
{
    SChar  sInitMessage[64];
    IDE_RC sRC = IDE_FAILURE;

    IDE_TEST_RAISE(iduProperty::getSNMPEnable() == 0, SNMP_DISABLED);

    mSock            = PDL_INVALID_SOCKET;
    mSNMPPortNo      = iduProperty::getSNMPPortNo();
    mSNMPTrapPortNo  = iduProperty::getSNMPTrapPortNo();
    mSNMPRecvTimeout = iduProperty::getSNMPRecvTimeout();
    mSNMPSendTimeout = iduProperty::getSNMPSendTimeout();

    ideLog::log(IDE_SNMP_0, MM_TRC_SNMP_PROPERTIES,
                mSNMPPortNo,
                mSNMPTrapPortNo,
                mSNMPRecvTimeout,
                mSNMPSendTimeout,
                iduProperty::getSNMPTrcFlag());

    IDE_TEST(startSNMPThread() != IDE_SUCCESS);

    idlOS::snprintf(sInitMessage,
                    ID_SIZEOF(sInitMessage),
                    MM_TRC_SNMP_LISTENER_START,
                    mSNMPPortNo);
    IDE_CALLBACK_SEND_MSG_NOLOG(sInitMessage);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SNMP_DISABLED)
    {
        ideLog::log(IDE_SNMP_0, MM_TRC_SNMP_DISABLED);
        sRC = IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return sRC;
}

/**
 * mmsSNMP::finalize()
 */
IDE_RC mmsSNMP::finalize()
{
    IDE_RC sRC = IDE_FAILURE;

    IDE_TEST_RAISE(iduProperty::getSNMPEnable() == 0, SNMP_DISABLED);

    mRun = ID_FALSE;
    IDE_TEST(gMmsSNMP.join() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SNMP_DISABLED)
    {
        sRC = IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return sRC;
}

IDE_RC mmsSNMP::initializeThread()
{
    IDU_FIT_POINT_RAISE("mmsSNMP::initializeThread::malloc::PacketBuf", INSUFFICIENT_MEMORY );
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT,
                                     PACKET_BUF_SIZE,
                                     (void **)&mPacketBuf) != IDE_SUCCESS,
                   INSUFFICIENT_MEMORY);
    idlOS::memset((void *)mPacketBuf, 0x0, PACKET_BUF_SIZE);
    mPacketBufSize = PACKET_BUF_SIZE;
    mValueBufSize = PACKET_BUF_SIZE / 2;

    IDU_FIT_POINT_RAISE("mmsSNMP::initializeThread::malloc::ValueBufSize", INSUFFICIENT_MEMORY );
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT,
                                     mValueBufSize,
                                     (void **)&mValueBuf) != IDE_SUCCESS,
                   INSUFFICIENT_MEMORY);
    idlOS::memset((void *)mValueBuf, 0x0, mValueBufSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION(INSUFFICIENT_MEMORY)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void   mmsSNMP::finalizeThread()
{
    IDE_DASSERT(iduMemMgr::free(mPacketBuf) == IDE_SUCCESS);
    IDE_DASSERT(iduMemMgr::free(mValueBuf)  == IDE_SUCCESS); 
}
