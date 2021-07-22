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
 
package com.altibase.altilinker.adlp.type;

/**
 * Property type types on ADLP protocol
 */
public interface PropertyType
{
    public static final int RemoteNodeReceiveTimeout = 0;
    public static final int QueryTimeout             = 1;
    public static final int NonQueryTimeout          = 2;
    public static final int ThreadCount              = 3;
    public static final int ThreadSleepTimeout       = 4;
    public static final int RemoteNodeSessionCount   = 5;
    public static final int TraceLoggingLevel        = 6;
    public static final int TraceLogFileSize         = 7;
    public static final int TraceLogFileCount        = 8;
    public static final int TargetList               = 9;
    public static final int TargetItemBegin          = 100;
    public static final int TargetItemName           = TargetItemBegin + 0;
    public static final int TargetItemJdbcDriver     = TargetItemBegin + 1;
    public static final int TargetItemJdbcDriverClassName = TargetItemBegin + 2;
    public static final int TargetItemConnectionUrl  = TargetItemBegin + 3;
    public static final int TargetItemUser           = TargetItemBegin + 4;
    public static final int TargetItemPassword       = TargetItemBegin + 5;
    public static final int TargetItemXADataSourceClassName = TargetItemBegin + 6;
    public static final int TargetItemXADataSourceUrlSetterName = TargetItemBegin + 7;

    public static final int TargetItemEnd            = TargetItemXADataSourceUrlSetterName;
}
