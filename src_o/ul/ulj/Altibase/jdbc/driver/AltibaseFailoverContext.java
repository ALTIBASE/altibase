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

import Altibase.jdbc.driver.util.AltibaseProperties;

public class AltibaseFailoverContext
{
    private AltibaseConnection             mConnection;
    private AltibaseProperties             mConnectionProp;
    private AltibaseFailoverCallback       mCallback      = new AltibaseFailoverCallbackDummy();
    private AltibaseFailover.CallbackState mCallbackState = AltibaseFailover.CallbackState.STOPPED;
    private Object                         mAppContext;
    private AltibaseFailoverServerInfo     mCurrentServer;
    private AltibaseFailoverServerInfoList mFailoverServerList;
    private AltibaseXAResource             mRelatedXAResource;

    public AltibaseFailoverContext(AltibaseConnection aConn, AltibaseProperties aConnProp, AltibaseFailoverServerInfoList aFailoverServerList)
    {
        mConnection = aConn;
        mConnectionProp = (AltibaseProperties) aConnProp.clone();
        mFailoverServerList = aFailoverServerList;
    }

    public AltibaseConnection getConnection()
    {
        return mConnection;
    }

    public void setConnection(AltibaseConnection aConnection)
    {
        this.mConnection = aConnection;
    }

    public AltibaseProperties connectionProperties()
    {
        return mConnectionProp;
    }

    public AltibaseFailoverCallback getCallback()
    {
        return mCallback;
    }

    public void setCallback(AltibaseFailoverCallback aCallback)
    {
        this.mCallback = aCallback;
    }

    public AltibaseFailover.CallbackState getCallbackState()
    {
        return mCallbackState;
    }

    public void setCallbackState(AltibaseFailover.CallbackState aCallbackState)
    {
        this.mCallbackState = aCallbackState;
    }

    public Object getAppContext()
    {
        return mAppContext;
    }

    public void setAppContext(Object aAppContext)
    {
        this.mAppContext = aAppContext;
    }

    public AltibaseFailoverServerInfo getCurrentServer()
    {
        return mCurrentServer;
    }

    public void setCurrentServer(AltibaseFailoverServerInfo aCurrentServer)
    {
        this.mCurrentServer = aCurrentServer;
    }

    public AltibaseFailoverServerInfoList getFailoverServerList()
    {
        return mFailoverServerList;
    }

    public AltibaseXAResource getRelatedXAResource()
    {
        return mRelatedXAResource;
    }

    public void setRelatedXAResource(AltibaseXAResource aXAResource)
    {
        mRelatedXAResource = aXAResource;
    }

    /**
     * STF를 사용하도록 설정했는지 얻는다.
     *
     * @return STF를 사용할지 여부
     */
    public boolean useSessionFailover()
    {
        return mConnectionProp.useSessionFailover();
    }

    /**
     * Failover를 수행할 때, 서버에 전해줄 Failover source를 설정한다.
     * <p>
     * 이 정보는 V$SESSION.FAILOVER_SOURCE로 출력된다. 
     *
     * @param aFailoverSource 설정할 Failover source 문자열
     */
    public void setFailoverSource(String aFailoverSource)
    {
        mConnectionProp.setFailoverSource(aFailoverSource);
    }
}
