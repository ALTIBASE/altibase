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

import java.sql.SQLException;

/**
 * 접속 장애 상황에서 발생하는 Shard jdbc exception <br>
 * 내부적으로 node name을 가지고 있으며 노드커넥션에서 발생한 에러메세지에 대해서는 <br>
 * node name, ip address, port를 추가해서 돌려준다.
 */
public class ShardJdbcException extends SQLException
{
    private String mNodeName;

    public ShardJdbcException(String aErrorMsg, int aErrorCode, String aNodeName, String aIpAddress,
                              int aPortNo)
    {
        super(makeString(aErrorMsg, aNodeName, aIpAddress, aPortNo),
              ErrorDef.getErrorState(aErrorCode), aErrorCode);
        mNodeName = aNodeName;
    }

    private static String makeString(String aErrorMsg, String aNodeName, String aIpAddress,
                                     int aPortNo)
    {
        StringBuilder sSb = new StringBuilder();
        // BUG-46790 meta에서 발생한 failover exception인 경우에는 node 정보를 표시하지 않는다.
        if (aNodeName != null)
        {
            sSb.append("[").append(aNodeName);
            sSb.append(",").append(aIpAddress).append(":").append(aPortNo);
            sSb.append("] ");
        }
        sSb.append(aErrorMsg);

        return sSb.toString();
    }

    public String getNodeName()
    {
        return mNodeName;
    }

}
