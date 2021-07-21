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
 
package com.altibase.altilinker.adlp.type;

import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.jdbc.JdbcDriver;

/**
 * Target class of dblink.conf properties
 */
public class Target implements Cloneable
{
    public String     mName          = null;
    public JdbcDriver mDriver        = null;
    public String     mConnectionUrl = null;
    public String     mUser          = null;
    public String     mPassword      = null;
    public String     mXADataSourceClassName     = null;
    public String     mXADataSourceUrlSetterName = null;

    public Target()
    {
        mDriver = new JdbcDriver();
    }
    /**
     * Get hash code value
     * 
     * @return hash code value
     */
    public int hashCode()
    {
        // reference: API document for String.hashCode method
        final int sDefaultHashCode = 31;
        
        int sHashCode = sDefaultHashCode;
        
        if (mName          != null) sHashCode += mName         .hashCode();
        if ( mDriver.mDriverPath      != null ) sHashCode += mDriver.mDriverPath     .hashCode();
        if ( mDriver.mDriverClassName != null ) sHashCode += mDriver.mDriverClassName.hashCode();
        if (mConnectionUrl != null) sHashCode += mConnectionUrl.hashCode();
        if (mUser          != null) sHashCode += mUser         .hashCode();
        if (mPassword      != null) sHashCode += mPassword     .hashCode();
        if (mXADataSourceClassName != null) sHashCode += mXADataSourceClassName.hashCode();
        if (mXADataSourceUrlSetterName != null) sHashCode += mXADataSourceUrlSetterName.hashCode();
        
        return sHashCode;
    }
    
    /**
     * Whether given Target object equals or not
     * 
     * @return true, if given Target object equals. false, if not.
     */
    public boolean equals(Object aObject)
    {
        if (aObject == null)
            return false;
        
        if (getClass() != aObject.getClass())
            return false;

        Target sTarget = (Target)aObject;
        
        if (isEqual(mName,          sTarget.mName         ) == false) return false;
        if (isEqual(mDriver.mDriverPath,      sTarget.mDriver.mDriverPath     ) == false) return false;
        if (isEqual(mDriver.mDriverClassName, sTarget.mDriver.mDriverClassName) == false) return false;
        if (isEqual(mConnectionUrl, sTarget.mConnectionUrl) == false) return false;
        if (isEqual(mUser,          sTarget.mUser         ) == false) return false;
        if (isEqual(mPassword,      sTarget.mPassword     ) == false) return false;
        if (isEqual(mXADataSourceClassName, sTarget.mXADataSourceClassName ) == false) return false;
        if (isEqual(mXADataSourceUrlSetterName, sTarget.mXADataSourceUrlSetterName ) == false) return false;
        
        return true;
    }
    
    /**
     * String compare
     * 
     * @param sString1 String 1 to compare
     * @param sString2 String 2 to compare
     * @return true, if equals. false, if not.
     */
    private boolean isEqual(String sString1, String sString2)
    {
        if (sString1 == null && sString2 == null)
        {
            return true;
        }

        if (sString1 == null || sString2 == null)
        {
            return false;
        }
        
        if (sString1.equals(sString2) == false)
        {
            return false;
        }
        
        return true;
    }

    /**
     * Validate target
     * 
     * @return true, if valid. false, if invalid
     */
    public boolean validate()
    {
        if (mName == null || mName.length() == 0)
        {
            return false;
        }
        
        if ( mDriver.mDriverPath == null || mDriver.mDriverPath.length() == 0 )
        {
            return false;
        }
        
        if (mConnectionUrl == null || mConnectionUrl.length() == 0)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Validate target with user info
     * 
     * @return true, if valid. false, if invalid
     */
    public boolean validateWithUserInfo()
    {
        boolean sValidated = validate();
        
        if (sValidated == false)
        {
            return false;
        }
        
        if (mUser == null || mUser.length() == 0)
        {
            return false;
        }
        
        if (mPassword == null || mPassword.length() == 0)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Clone this object
     * 
     * @return Cloned object
     */
    public Object clone()
    {
        Target sNewTarget = null;
        
        try
        {
            sNewTarget = (Target)super.clone();
        }
        catch (CloneNotSupportedException e)
        {
            TraceLogger.getInstance().log(e);
        }
        
        sNewTarget.mName          = mName;
        sNewTarget.mDriver.mDriverPath      = mDriver.mDriverPath;
        sNewTarget.mDriver.mDriverClassName = mDriver.mDriverClassName;
        sNewTarget.mConnectionUrl = mConnectionUrl;
        sNewTarget.mUser          = mUser;
        sNewTarget.mPassword      = mPassword;
        sNewTarget.mXADataSourceClassName     = mXADataSourceClassName;
        sNewTarget.mXADataSourceUrlSetterName = mXADataSourceUrlSetterName;
        
        return sNewTarget;
    }
    
    /**
     * Get string representation for this object
     * 
     * @return String representation for this object
     */
    public String toString()
    {
        String sString = "";
        
        sString += "NAME = "           + mName          + ",";
        sString += "JDBC_DRIVER = "            + mDriver.mDriverPath      + ",";
        sString += "JDBC_DRIVER_CLASS_NAME = " + mDriver.mDriverClassName + ",";
        sString += "CONNECTION_URL = " + mConnectionUrl + ",";
        sString += "USER = "           + mUser          + ",";
        sString += "PASSWORD = "       + mPassword      + ",";
        sString += "XADATASOURCE_CLASS_NAME = " + mXADataSourceClassName + ",";
        sString += "XADATASOURCE_URL_SETTER_NAME = " + mXADataSourceUrlSetterName;

        return sString;
    }
    
    /**
     * Set user of target
     * 
     * @param aUser User to set
     */
    public void setUser(String aUser)
    {
        if (aUser != null)
            mUser = aUser;
    }
    
    /**
     * Set password of target
     * 
     * @param aPassword Password to set
     */
    public void setPassword(String aPassword)
    {
        if (aPassword != null)
            mPassword = aPassword;
    }
}
