/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.session;

/**
 * Task result class by JDBC interface
 */
public class TaskResult
{
    public static final int None              = 0;
    public static final int Success           = 1;
    public static final int Failed            = 2;
    public static final int RemoteServerError = 3;

    private int               mResult            = None;
    private RemoteServerError mRemoteServerError = null;
    
    /**
     * Constructor
     * 
     * @param aResult                   Task result value
     * @param aRemoteServerError        Remote server error when JDBC interface cause exception.
     */
    public TaskResult(int aResult, RemoteServerError aRemoteServerError)
    {
        mResult            = aResult;
        mRemoteServerError = aRemoteServerError;
    }

    /**
     * Constructor 
     * 
     * @param aRemoteServerError        Remote server error when JDBC interface cause exception.
     */
    public TaskResult(RemoteServerError aRemoteServerError)
    {
        this(RemoteServerError, aRemoteServerError);
    }
    
    /**
     * Constructor
     * 
     * @param aResult   Task result value 
     */
    public TaskResult(int aResult)
    {
        this(aResult, null);
    }
    
    /**
     * Get task result value
     * 
     * @return  Task result value
     */
    public int getResult()
    {
        return mResult;
    }
    
    /**
     * Get remote server error when JDBC interface cause exception.
     * 
     * @return  RemoteServerError object
     */
    public RemoteServerError getRemoteServerError()
    {
        return mRemoteServerError;
    }
    
    /**
     * Whether remote server error cause or not
     * 
     * @return  true, if JDBC interface cause remote server error. false, if does not.
     */
    public boolean isRemoteServerError()
    {
        return (mRemoteServerError != null) ? true : false;
    }
}
