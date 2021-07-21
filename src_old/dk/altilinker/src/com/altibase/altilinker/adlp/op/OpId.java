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
 
package com.altibase.altilinker.adlp.op;

public interface OpId
{
    public static final byte None                              = (byte)0x00;

    // control operations
    public static final byte CreateLinkerCtrlSession           = (byte)0x01;  
    public static final byte CreateLinkerCtrlSessionResult     = (byte)0x02;  
    public static final byte CreateLinkerDataSession           = (byte)0x03;  
    public static final byte CreateLinkerDataSessionResult     = (byte)0x04;  
    public static final byte DestroyLinkerDataSession          = (byte)0x05;  
    public static final byte DestroyLinkerDataSessionResult    = (byte)0x06;  
    public static final byte RequestLinkerStatus               = (byte)0x07;  
    public static final byte RequestLinkerStatusResult         = (byte)0x08;  
    public static final byte RequestPVInfoDBLinkAltiLinkerStatus
                                                               = (byte)0x09;  
    public static final byte RequestPVInfoDBLinkAltiLinkerStatusResult
                                                               = (byte)0x0A;  
    public static final byte PrepareLinkerShutdown             = (byte)0x0B;  
    public static final byte PrepareLinkerShutdownResult       = (byte)0x0C;  
    public static final byte DoLinkerShutdown                  = (byte)0x0D;
    public static final byte RequestLinkerDump                 = (byte)0x0E;
    public static final byte RequestLinkerDumpResult           = (byte)0x0F;
  
    public static final byte UknownOperation                   = (byte)0x7F;

    // data operations
    public static final byte PrepareRemoteStatement            = (byte)0x81;  
    public static final byte PrepareRemoteStatementResult      = (byte)0x82;  
    public static final byte RequestRemoteErrorInfo            = (byte)0x83;  
    public static final byte RequestRemoteErrorInfoResult      = (byte)0x84;  
    public static final byte ExecuteRemoteStatement            = (byte)0x85;  
    public static final byte ExecuteRemoteStatementResult      = (byte)0x86;  
    public static final byte RequestFreeRemoteStatement        = (byte)0x87;  
    public static final byte RequestFreeRemoteStatementResult  = (byte)0x88;  
    public static final byte FetchResultSet                    = (byte)0x89;  
    public static final byte FetchResultRow                    = (byte)0x8A;  
    public static final byte RequestResultSetMeta              = (byte)0x8B;  
    public static final byte RequestResultSetMetaResult        = (byte)0x8C;  
    public static final byte BindRemoteVariable                = (byte)0x8D;  
    public static final byte BindRemoteVariableResult          = (byte)0x8E;
    public static final byte RequestCloseRemoteNodeSession     = (byte)0x8F;  
    public static final byte RequestCloseRemoteNodeSessionResult
                                                               = (byte)0x90;  
    public static final byte SetAutoCommitMode                 = (byte)0x91;  
    public static final byte SetAutoCommitModeResult           = (byte)0x92;  
    public static final byte CheckRemoteNodeSession            = (byte)0x93;  
    public static final byte CheckRemoteNodeSessionResult      = (byte)0x94;  
    public static final byte Commit                            = (byte)0x95;  
    public static final byte CommitResult                      = (byte)0x96;  
    public static final byte Rollback                          = (byte)0x97;
    public static final byte RollbackResult                    = (byte)0x98;
    public static final byte Savepoint                         = (byte)0x99;  
    public static final byte SavepointResult                   = (byte)0x9A;
    public static final byte AbortRemoteStatement              = (byte)0x9B;  
    public static final byte AbortRemoteStatementResult        = (byte)0x9C;
    public static final byte PrepareRemoteStatementBatch       = (byte)0x9D;
    public static final byte PrepareRemoteStatementBatchResult = (byte)0x9E;
    public static final byte RequestFreeRemoteStatementBatch   = (byte)0x9F;
    public static final byte RequestFreeRemoteStatementBatchResult
                                                               = (byte)0xA0;
    public static final byte ExecuteRemoteStatementBatch       = (byte)0xA1;
    public static final byte ExecuteRemoteStatementBatchResult = (byte)0xA2;
    public static final byte PrepareAddBatch                   = (byte)0xA3;
    public static final byte PrepareAddBatchResult             = (byte)0xA4;
    public static final byte AddBatch                          = (byte)0xA5;
    public static final byte AddBatchResult                    = (byte)0xA6;
    // PROJ-2569
    public static final byte XAPrepare                         = (byte)0xA7;  
    public static final byte XAPrepareResult                   = (byte)0xA8;  
    public static final byte XACommit                          = (byte)0xA9;  
    public static final byte XACommitResult                    = (byte)0xAA;  
    public static final byte XARollback                        = (byte)0xAB;
    public static final byte XARollbackResult                  = (byte)0xAC;
    public static final byte XAForget                          = (byte)0xAD;
    public static final byte XAForgetResult                    = (byte)0xAE;
}
