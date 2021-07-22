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
 
package com.altibase.altilinker.worker.task;

public interface TaskType
{
    public static final byte None                                = 0;  
    public static final byte CreateLinkerCtrlSession             = 1;  
    public static final byte CreateLinkerDataSession             = 2;  
    public static final byte DestroyLinkerDataSession            = 3;  
    public static final byte RequestLinkerStatus                 = 4;  
    public static final byte RequestPVInfoDBLinkAltiLinkerStatus = 5;  
    public static final byte PrepareLinkerShutdown               = 6;  
    public static final byte DoLinkerShutdown                    = 7;  
    public static final byte RequestCloseRemoteNodeSession       = 8;  
    public static final byte SetAutoCommitMode                   = 9;  
    public static final byte CheckRemoteNodeSession              = 10;  
    public static final byte Commit                              = 11;  
    public static final byte Rollback                            = 12;
    public static final byte Savepoint                           = 13;  
    public static final byte AbortRemoteStatement                = 14;  
    public static final byte PrepareRemoteStatement              = 15;  
    public static final byte RequestRemoteErrorInfo              = 16;  
    public static final byte ExecuteRemoteStatement              = 17;  
    public static final byte RequestFreeRemoteStatement          = 18;  
    public static final byte FetchResultSet                      = 19;  
    public static final byte RequestResultSetMeta                = 20;  
    public static final byte BindRemoteVariable                  = 21;  
    public static final byte RequestLinkerDump                   = 22;
    public static final byte PrepareRemoteStatementBatch         = 23;
    public static final byte RequestFreeRemoteStatementBatch     = 24;
    public static final byte ExecuteRemoteStatementBatch         = 25;
    public static final byte PrepareAddBatch                     = 26;
    public static final byte AddBatch                            = 27;
    public static final byte CreateLinkerNotifySession           = 28;  
    public static final byte DestroyLinkerNotifySession          = 29;  
    public static final byte XAPrepare                           = 30;
    public static final byte XACommit                            = 31;
    public static final byte XARollback                          = 32;
    public static final byte XAForget                            = 33;
}
