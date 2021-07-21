/***********************************************************************
 * Copyright 1999-2014, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idmSNMP.cpp 83346 2018-06-26 03:32:55Z kclee $
 **********************************************************************/

#include <idmSNMP.h>

/* 
 * PROJ-2473 SNMP 지원
 */

PDL_SOCKET         idmSNMP::mSock = PDL_INVALID_SOCKET;
struct sockaddr_in idmSNMP::mSubagentAddr;

/* idmSNMPTrap 구조 참고 
struct idmSNMPTrap
{
    UChar mAddress[128];
    UInt  mLevel;
    UInt  mCode;
    UChar mMessage[512];
    UChar mMoreInfo[512];
};
*/

/**
 * idmSNMP::trap
 */
void idmSNMP::trap(const idmSNMPTrap *aTrap)
{
    fd_set             sWriteFdSet;
    PDL_Time_Value     sTimeout;
    UInt               sSelectRet = SNMP_SELECT_ERR;

    UInt               sRet = 0;

    IDE_TEST_RAISE(mSock == PDL_INVALID_SOCKET, SNMP_DISABLED);

    ideLog::log(IDE_SNMP_2, ID_TRC_SNMP_TRAP_MSG,
                aTrap->mAddress,
                aTrap->mLevel,
                aTrap->mCode,
                aTrap->mMessage,
                aTrap->mMoreInfo);

    sTimeout.msec(iduProperty::getSNMPSendTimeout());
    FD_ZERO(&sWriteFdSet);
    FD_SET(mSock, &sWriteFdSet);

    /* sendto()에서 select는 큰 의미가 없다 */
    sSelectRet = idmSNMP::selectSNMP(mSock,
                                     NULL, &sWriteFdSet,
                                     sTimeout, (SChar *)"TRAP_SEND");

    if ((sSelectRet & SNMP_SELECT_WRITE) == SNMP_SELECT_WRITE)
    {
        sRet = idlOS::sendto(mSock,
                             (SChar *)aTrap,
                             sizeof(*aTrap),
                             0,
                             (struct sockaddr *)&mSubagentAddr,
                             ID_SIZEOF(mSubagentAddr));

        if (sRet != sizeof(*aTrap))
        {
            /* Non-rechable */
            ideLog::log(IDE_SNMP_0,
                        "SNMPTrap: SEND RET: %"ID_UINT32_FMT" (%"ID_UINT32_FMT")",
                        sRet,
                        errno);
        }
        else
        {
            ideLog::log(IDE_SNMP_2, "SNMPTrap: SEND SUCCESS");
        }
    }
    else
    {
        /* Timeout or Select err - maybe non-reachable */
    }

    IDE_EXCEPTION_CONT(SNMP_DISABLED);

    return;
}

/**
 * idmSNMP::selectSNMP
 *
 * @aSock : 호출시 Sock + 1로 주지 않아야 한다.
 */
UInt idmSNMP::selectSNMP(const PDL_SOCKET&  aSock,
                         fd_set            *aReadFdSet,
                         fd_set            *aWriteFdSet,
                         PDL_Time_Value&    aTimeout,
                         SChar             *aMessage)
{
    SInt sRet = 0;
    UInt sOut = 0;

    sRet = idlOS::select(aSock + 1, aReadFdSet, aWriteFdSet, NULL, aTimeout);

    switch (sRet)
    {
        case -1:
            ideLog::log(IDE_SNMP_0, "SNMP: SELECT_ERR %s", aMessage);
            sOut = SNMP_SELECT_ERR;
            break;

        case 0:
            sOut = SNMP_SELECT_TIMEOUT;
            break;

        default:
            sOut = 0;

            if (aReadFdSet != NULL)
            {
                if (FD_ISSET(aSock, aReadFdSet) != 0)
                {
                    sOut |= SNMP_SELECT_READ;
                }
                else
                {
                    /* Non-rechable */
                }
            }
            else
            {
                /* Nothing */
            }

            if (aWriteFdSet != NULL)
            {
                if (FD_ISSET(aSock, aWriteFdSet) != 0)
                {
                    sOut |= SNMP_SELECT_WRITE;
                }
                else
                {
                    /* Non-rechable */
                }
            }
            else
            {
                /* Nothing */
            }

            break;
    }

    return sOut;
}

/**
 * idmSNMP::initialize()
 */
IDE_RC idmSNMP::initialize()
{
    IDE_RC sRC = IDE_FAILURE;

    IDE_TEST_RAISE(iduProperty::getSNMPEnable() == 0, SNMP_DISABLED);

    mSock = idlOS::socket(AF_INET, SOCK_DGRAM, 0);
    IDE_TEST_RAISE(mSock == PDL_INVALID_SOCKET, ERR_SOCKET_CREATE_FAILED);

    idlOS::memset(&mSubagentAddr, 0, sizeof(mSubagentAddr));

    mSubagentAddr.sin_family      = AF_INET;
    mSubagentAddr.sin_addr.s_addr = idlOS::inet_addr("127.0.0.1");
    mSubagentAddr.sin_port        = htons(iduProperty::getSNMPTrapPortNo());

    return IDE_SUCCESS;

    IDE_EXCEPTION(SNMP_DISABLED)
    {
        /* SNMP가 Disable 되어 있으면 Alarm을 끄자 */
        iduProperty::setSNMPAlarmQueryTimeout(0);
        iduProperty::setSNMPAlarmUtransTimeout(0);
        iduProperty::setSNMPAlarmFetchTimeout(0);
        iduProperty::setSNMPAlarmSessionFailureCount(0);

        sRC = IDE_SUCCESS;
    }
    IDE_EXCEPTION(ERR_SOCKET_CREATE_FAILED)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idc_INET_SOCKET_CREATE_FAILED));
    }
    IDE_EXCEPTION_END;

    return sRC;
}

/**
 * mmsSNMP::finalize()
 */
IDE_RC idmSNMP::finalize()
{
    IDE_RC sRC = IDE_FAILURE;

    IDE_TEST_RAISE(iduProperty::getSNMPEnable() == 0, SNMP_DISABLED);

    idlOS::closesocket(mSock);
    mSock = PDL_INVALID_SOCKET;

    return IDE_SUCCESS;

    IDE_EXCEPTION(SNMP_DISABLED)
    {
        sRC = IDE_SUCCESS;
    }
    IDE_EXCEPTION_END;

    return sRC;
}
