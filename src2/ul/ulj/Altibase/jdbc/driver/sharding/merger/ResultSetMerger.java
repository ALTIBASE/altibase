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

package Altibase.jdbc.driver.sharding.merger;

import java.sql.ResultSet;
import java.sql.SQLException;

/**
 * ResultSet 병합 인터페이스
 */
public interface ResultSetMerger
{
    /**
     * 다음 데이터를 반복한다.
     *
     * @return 다음 데이터
     * @throws SQLException SQL Exception
     */
    boolean next() throws SQLException;
    ResultSet getCurrentResultSet() throws SQLException;
}
