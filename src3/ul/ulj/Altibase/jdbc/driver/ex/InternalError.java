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

package Altibase.jdbc.driver.ex;

/**
 * 이 에러는 JDBC나 CM 프로토콜에 버그가 있을 때 Error를 던지는데 사용한다.
 * 즉, 이 에러가 떴다면 코딩 실수나 고려하지 못한 상황으로 인한 버그가 있는 것이다.
 */
class InternalError extends AssertionError
{
    private static final long serialVersionUID = 5847101200927829703L;

    private final int         mErrorCode;

    public InternalError(String aMessage, int aErrorCode)
    {
        super(aMessage);
        mErrorCode = aErrorCode;
    }

    public int getErrorCode()
    {
        return mErrorCode;
    }
}
