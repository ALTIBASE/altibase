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

package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.util.DynamicArray;

/**
 * ResultSet record 데이터에 접근하는 인터페이스를 정의한다.
 */
public interface RowHandle extends BatchRowHandle
{
    void increaseStoreCursor();

    void beforeFirst();

    void setPrepared(boolean aIsPrepared);

    DynamicArray getDynamicArray(int aIndex);

    boolean next();

    boolean isAfterLast();

    int getPosition();

    boolean isBeforeFirst();

    boolean isFirst();

    boolean isLast();

    boolean setPosition(int aIndex);

    void afterLast();

    boolean previous();

    void delete();

    void reload();

    void update();
}
