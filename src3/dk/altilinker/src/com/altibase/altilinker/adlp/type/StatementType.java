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
 * Statement type on ADLP protocol
 */
public interface StatementType
{
    public static final int Unknown                  = -1;
    public static final int RemoteTable              = 0;
    public static final int RemoteExecuteImmediate   = 1;
    public static final int ExecuteQueryStatement    = 2;
    public static final int ExecuteNonQueryStatement = 3;
    public static final int ExecuteStatement         = 4;
}
