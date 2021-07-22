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

/**
 * PROJ-2190 ClientSide Autocommit 모드가 추가되었기 때문에 autoCommit 값을 boolean이 아닌 enum type으로 표현한다.
 * 
 * @author yjpark
 */
public enum AutoCommitMode
{
    SERVER_SIDE_AUTOCOMMIT_ON, SERVER_SIDE_AUTOCOMMIT_OFF, CLIENT_SIDE_AUTOCOMMIT_ON;
}
