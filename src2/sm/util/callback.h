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
 
#include <idl.h>
#include <smi.h>

void defaultEmergencyCallback(UInt /*aTmp*/)
{
}

UInt defaultGetTimeCallback( )
{
    return 0;
}

smiGlobalCallBackList gCallBack =
{
    NULL,   // findCompare; 
    NULL,   // findKey2String
    NULL,   // findNull
    NULL,   // findCopyDiskColumnValue
    NULL,   // findCopyDiskColumnValue4DataType
    NULL,   // findActualSize
    NULL,   // findHash
    NULL,   // findIsNull
    NULL,   // getAlignValue
    NULL,   // getValueLengthFromFetchBuffer
    NULL,   // waitLockFunc
    NULL,   // wakeupLockFunc
    defaultEmergencyCallback,   // setEmergencyFunc
    defaultEmergencyCallback,   // clrEmergencyFunc
    defaultGetTimeCallback,     // getCurrTimeFunc
    NULL,   // switchDDLFunc
    NULL,   // makeNullRow
    NULL,   // checkNeedUndoRecord
    NULL,   // getUpdateMaxLogSize
    NULL,   // getSQLText
    NULL,   // getNonStoringSize
    NULL,   // getColumnHeaderDescFunc
    NULL,   // getTableHeaderDescFunc
    NULL,   // getPartitionHeaderDescFunc
    NULL,   // getColumnMapFromColumnHeaderFunc
    NULL,   // needMinMaxStatistics
    NULL,   // getColumnStoreLen
    NULL,   // isUsablePartialDirectKeyIndex
    NULL
};

