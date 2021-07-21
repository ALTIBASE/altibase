/***********************************************************************
 * Copyright 1999-2014, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idmSNMP.h 67796 2014-12-03 08:39:33Z donlet $
 **********************************************************************/

#ifndef _O_IDM_SNMP_H_
#define _O_IDM_SNMP_H_ 1

#include <ide.h>
#include <idu.h>
#include <idm.h>

/* 
 * PROJ-2473 SNMP Áö¿ø
 */

typedef struct idmSNMPTrap idmSNMPTrap;

struct idmSNMPTrap
{
    UChar mAddress[128];
    UInt  mLevel;
    UInt  mCode;
    UChar mMessage[512];
    UChar mMoreInfo[512];
};

/* Trap Codes */
enum
{
    SNMP_ALARM_ALTIBASE_RUNNING      = 10000001,
    SNMP_ALARM_ALTIBASE_NOT_RUNNING  = 10000002,

    SNMP_ALARM_QUERY_TIMOUT          = 10000101,
    SNMP_ALARM_FETCH_TIMOUT          = 10000102,
    SNMP_ALARM_UTRANS_TIMOUT         = 10000103,

    SNMP_ALARM_SESSION_FAILURE_COUNT = 10000201
};

enum
{
    SNMP_SELECT_TIMEOUT    =  0,
    SNMP_SELECT_ERR        =  1,
    SNMP_SELECT_READ       =  2,
    SNMP_SELECT_WRITE      =  4,
    SNMP_SELECT_READ_WRITE =  SNMP_SELECT_READ | SNMP_SELECT_WRITE
};

class idmSNMP
{
private:
    static PDL_SOCKET          mSock;
    static struct sockaddr_in  mSubagentAddr;

    static UInt                mSNMPRecvTimeout;
    static UInt                mSNMPSendTimeout;

public:
    static void   trap(const idmSNMPTrap *aTrap);

    static UInt   selectSNMP(const PDL_SOCKET&  aSock,
                             fd_set            *aReadFdSet,
                             fd_set            *aWriteFdSet,
                             PDL_Time_Value&    aTimeout,
                             SChar             *aMessage);
 
    static IDE_RC initialize();
    static IDE_RC finalize();
};

#endif /* _O_IDM_SNMP_H_ */
