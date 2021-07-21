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

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * AltibaseFailoverServerInfo의 리스트를 포함하고 있는 클래스.<br/>
 */
public final class AltibaseFailoverServerInfoList
{
    // BUG-46790 직접 ArrayList를 상속하지 않고 composition으로 구성한다.
    private List<AltibaseFailoverServerInfo> mInternalList;
    private Random                           mRandom = new Random();

    public AltibaseFailoverServerInfoList()
    {
        mInternalList = new ArrayList<AltibaseFailoverServerInfo>();
    }

    /**
     * alternate servers string 형태의 문자열로 변환한다.
     */
    @Override
    public String toString()
    {
        StringBuilder sBuf = new StringBuilder("(");
        for (int i = 0; i < mInternalList.size(); i++)
        {
            sBuf.append(mInternalList.get(i).toString());
            if (i > 0)
            {
                sBuf.append(',');
            }
        }
        sBuf.append(')');

        return sBuf.toString();
    }

    AltibaseFailoverServerInfo getRandom()
    {
        return mInternalList.get(mRandom.nextInt(mInternalList.size()));
    }

    public boolean add(String aServer, int aPort, String aDbName)
    {
        return mInternalList.add(new AltibaseFailoverServerInfo(aServer, aPort, aDbName));
    }

    public void add(int aIndex, String aServer, int aPort, String aDbName)
    {
        mInternalList.add(aIndex, new AltibaseFailoverServerInfo(aServer, aPort, aDbName));
    }

    public List<AltibaseFailoverServerInfo> getList()
    {
        return mInternalList;
    }

    public int size()
    {
        return mInternalList.size();
    }

    public void add(int aIndex, AltibaseFailoverServerInfo aServerInfo)
    {
        mInternalList.add(aIndex, aServerInfo);
    }

    public void set(int aIndex, AltibaseFailoverServerInfo aServerInfo)
    {
        mInternalList.set(aIndex, aServerInfo);
    }
}
