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
 * DB Message 프로토콜을 처리하는 기본 구현체.<br> 단순히 서버로부터 넘어온 메시지를 콘솔에 출력한다.
 */
public class AltibaseConsoleMessageCallback implements AltibaseMessageCallback
{
    public void print(String aMsg)
    {
        System.out.println(aMsg);
    }
}
