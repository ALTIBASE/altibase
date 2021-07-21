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
 * Result code type on ADLP protocol
 */
public class ResultCode
{
    public static final byte None                         = -1;
    public static final byte Success                      = 0;
    public static final byte Fragmented                   = 1;
    public static final byte Failed                       = 2;
    public static final byte RemoteServerError            = 3;
    public static final byte StatementTypeError           = 4;
    public static final byte NotPreparedRemoteStatement   = 5;
    public static final byte NotSupportedDataType         = 6;
    public static final byte NotSupportedDBCharSet        = 7;
    public static final byte SetAutoCommitModeFailed      = 8;
    public static final byte RemoteServerConnectionFail   = 9;
    public static final byte UnknownOperation             = 10;
    public static final byte UnknownSession               = 11;
    public static final byte WrongPacket                  = 12;
    public static final byte OutOfRange                   = 13;
    public static final byte UnknownRemoteStatement       = 14;
    public static final byte AlreadyCreatedSession        = 15;
    public static final byte ExcessRemoteNodeSessionCount = 16;
    public static final byte FailedToSetSavepoint         = 17;
    public static final byte TimedOut                     = 18;
    public static final byte Busy                         = 19;
    public static final byte EndOfFetch                   = 20; 
    public static final byte XAException                  = 21; 
}
