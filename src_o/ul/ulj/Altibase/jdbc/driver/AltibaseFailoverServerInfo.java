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

import Altibase.jdbc.driver.util.StringUtils;

public class AltibaseFailoverServerInfo
{
    private final String mServer;
    private final String mDbName; // BUG-43219 add dbname property to failover
    private final int    mPortNo;
    private long         mFailStartTime;

    public AltibaseFailoverServerInfo(String aServer, int aPortNo, String aDbName)
    {
        mServer = aServer;
        mPortNo = aPortNo;
        mDbName = aDbName;
    }

    /**
     * 접속할 서버의 host 값을 얻는다.
     *
     * @return host
     */
    public String getServer()
    {
        return mServer;
    }

    /**
     * 접속할 서버의 port number를 얻는다.
     *
     * @return port number
     */
    public int getPort()
    {
        return mPortNo;
    }

    // #region 서버 사용 시작, 접속 실패 시간 정보

    /**
     * 서버 접속에 실패한 시간을 얻는다.
     *
     * @return 서버 접속에 실패한 시간
     */
    public long getFailStartTime()
    {
        return mFailStartTime;

    }

    /**
     * 접속할 서버의 DB Name을 얻는다.
     *
     * @return DB Name
     */

    public String getDatabase()
    {
        return mDbName;
    }

    public void setFailStartTime()
    {
        mFailStartTime = (System.currentTimeMillis() / 1000 + 1);
    }

    // #endregion

    // #region Object 오버라이드

    /**
     * alternate servers string 포맷에 맞는 문자열 값을 얻는다.
     */
    public String toString()
    {
        /* alternateservers 포맷에 맞춤 */
        String sDbName = (StringUtils.isEmpty(mDbName)) ? "" : "/" + mDbName;
        return mServer + ":" + mPortNo + sDbName;
    }

    public int hashCode()
    {
        return toString().hashCode();
    }

    /**
     * Server와 Port, Database Name이 같은지 확인한다.
     */
    public boolean equals(Object aObj)
    {
        if (this == aObj)
        {
            return true;
        }
        if (aObj == null)
        {
            return false;
        }
        if (this.getClass() != aObj.getClass())
        {
            return false;
        }

        AltibaseFailoverServerInfo sServer = (AltibaseFailoverServerInfo) aObj;

        if (mServer == null)
        {
            if (sServer.getServer() != null)
            {
                return false;
            }
        }
        else /* mServer != null */
        {
            if (mServer.equals(sServer.getServer()) == false)
            {
                return false;
            }
        }

        if (mDbName != sServer.getDatabase())
        {
            return false;
        }

        if (mPortNo != sServer.getPort())
        {
            return false;
        }

        return true;
    }

    // #endregion
}
