/*
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

package Altibase.jdbc.driver;

import java.sql.SQLException;

public abstract class AltibaseScrollableResultSet extends AltibaseReadableResultSet
{
    /**
     * 현재 커서 위치의 Row를 Cache에서 지우고, 커서를 하나 앞으로 이동한다.
     * <p>
     * 지운 Row는 refresh를 하더라도 다시 가져오지 않는다.
     *
     * @throws SQLException ResultSet이 이미 닫혔거나 커서 위치가 올바르지 않을 경우
     */
    abstract void deleteRowInCache() throws SQLException;
}
