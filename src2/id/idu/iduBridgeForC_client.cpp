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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <iduProperty.h>
#include <idErrorCode.h>
#include <iduBridgeTime.h>

IDL_EXTERN_C_BEGIN

ULong idlOS_getThreadID()
{
    return idlOS::getThreadID();
}

SInt idlOS_getProcessorCount()
{
    return idlVA::getProcessorCount();
}

void idlOS_sleep(ULong aSec, ULong aMicroSec)
{
    PDL_Time_Value      sTimeOut;

    sTimeOut.set(aSec, aMicroSec);
    idlOS::sleep(sTimeOut);
}

SInt  idlOS_rand()
{
    return idlOS::rand();
}

void assertForC(SChar *aCond, SChar *aFile, UInt aLine)
{
    ideLog::log(IDE_SERVER_0,ID_TRC_ASSERT_BEGIN,
                aCond, aFile, aLine);

    ideLog::logCallStack(IDE_SERVER_0);
    ideLog::log(IDE_SERVER_0, ID_TRC_ASSERT_END);
    gCallbackForAssert(aFile, aLine);
    
    assert(0);
    idlOS::exit(-1);
}


/* idvTime related functions */
// return ID_SIZEOF(idvTime)
UInt idv_SizeOf_IdvTime()
{
    return 0;
}

// wrapper for IDV_TIME_AVAILABLE() macro
idBool idv_TIME_AVAILABLE()
{
    return ID_TRUE;
}


// wrapper for IDV_TIME_GET()
void idv_TIME_GET( iduBridgeTime * aTime )
{
}

// wrapper for IDV_TIME_DIFF_MICRO()
ULong  idv_TIME_DIFF_MICRO(iduBridgeTime * aBeforeTime,
                           iduBridgeTime * aAfterTime)
{
    return 0;
}


/* ------------------------------------------------
 *  Read Property
 * ----------------------------------------------*/
UInt iduBridge_getMutexSpinCount()
{
    return iduProperty::getMutexSpinCount();
}

// wrapper for iduProperty::getCheckMutexDurationTimeEnable()
UInt iduBridge_getCheckMutexDurationTimeEnable()
{
    return iduProperty::getCheckMutexDurationTimeEnable();
}

// wrapper for iduProperty::getTimedStatistics()
UInt iduBridge_getTimedStatistcis()
{
    return iduProperty::getTimedStatistics();
}

void idv_BEGIN_WAIT_EVENT( void * aStatSQL,
                           UInt   aWaitEventID )
{
}

void idv_END_WAIT_EVENT( void  * aStatSQL,
                         UInt    aWaitEventID )
{
}

IDL_EXTERN_C_END
