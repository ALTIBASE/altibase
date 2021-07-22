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

interface AltibaseFailoverServerCheckCallback
{
    /**
     * 서버 정보를 확인하고, 결과를 반환한다.
     *
     * @param aServerInfo 확인할 서버 정보
     * @param aJustCheck 단순히 확인만 해볼것인지 여부
     * @return 서버가 사용 가능하면 true, 아니면 false
     */
    boolean checkUsable(AltibaseFailoverServerInfo aServerInfo, boolean aJustCheck);
}

abstract class AbstractAltibaseFailoverServerCheckCallback implements AltibaseFailoverServerCheckCallback
{
    protected AltibaseFailoverContext mContext;

    public AbstractAltibaseFailoverServerCheckCallback(AltibaseFailoverContext aContext)
    {
        mContext = aContext;
    }
}
