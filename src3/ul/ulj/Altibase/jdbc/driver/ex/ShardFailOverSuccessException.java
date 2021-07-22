/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.ex;

/**
 * Shard 상황하에서 STF가 성공했을 때 발생함
 */
public class ShardFailOverSuccessException extends ShardJdbcException
{
    public ShardFailOverSuccessException(String aErrorMsg, int aErrorCode, String aNodeName,
                                         String aIpAddress, int aPort)
    {
        super("Failover success. " + aErrorMsg, aErrorCode, aNodeName, aIpAddress, aPort);
    }

    public ShardFailOverSuccessException(String aErrorMsg, int aErrorCode)
    {
        super("Failover success. " + aErrorMsg, aErrorCode, null, null, 0);
    }
}
