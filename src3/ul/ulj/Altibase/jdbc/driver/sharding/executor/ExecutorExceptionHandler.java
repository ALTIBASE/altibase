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

package Altibase.jdbc.driver.sharding.executor;

import java.sql.SQLException;

public class ExecutorExceptionHandler
{
    /**
     * failover 관련된 Exception은 자체적으로 node name이 있기 때문에 그냥 돌려주고 나머지는 node name을
     * 추가해서 돌려준다.
     *
     * @param aException 처리할 Exception
     * @param aNodeName 노드네임
     * @throws SQLException SQLException
     */
    public static void handleException(Exception aException, String aNodeName) throws SQLException
    {
        // BUG-46513 SQLException에는 sql message에 node name을 추가해서 던지고 나머지는 RuntimeException으로 처리한다.
        if (aException instanceof SQLException)
        {
            throwShardSQLException(aException, aNodeName);
        }

        throw new RuntimeException(aException);
    }

    static void handleException(Exception aException) throws SQLException
    {
        handleException(aException, null);
    }

    private static void throwShardSQLException(Exception aException,
                                               String aNodeName) throws SQLException
    {
        StringBuilder sSb = new StringBuilder();
        if (aNodeName == null)
        {
            sSb.append(aException.getMessage());
        }
        else
        {
            sSb.append("[").append(aNodeName).append("] ").append(aException.getMessage());
        }

        String sSqlState = ((SQLException)aException).getSQLState();
        int sErrorCode = ((SQLException)aException).getErrorCode();
        throw new SQLException(sSb.toString(), sSqlState, sErrorCode);
    }
}
