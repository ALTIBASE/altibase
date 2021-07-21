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

package Altibase.jdbc.driver.cm;

import Altibase.jdbc.driver.util.AltibaseProperties;
import Altibase.jdbc.driver.util.StringUtils;

/**
 * PROJ-2474 Ssl Certificate 정보를 담고있는 java bean 클래스
 * @author yjpark
 *
 */
public class SSLProperties
{
    public static final String  KEYSTORE_URL                = "keystore_url";
    public static final String  KEYSTORE_TYPE               = "keystore_type";
    public static final String  KEYSTORE_PASSWORD           = "keystore_password";
    public static final String  TRUSTSTORE_URL              = "truststore_url";
    public static final String  TRUSTSTORE_TYPE             = "truststore_type";
    public static final String  TRUSTSTORE_PASSWORD         = "truststore_password";
    public static final String  CIPHERSUITE_LIST            = "ciphersuite_list";
    public static final String  VERIFY_SERVER_CERTIFICATE   = "verify_server_certificate";
    public static final String  SSL_ENABLE                  = "ssl_enable";
    private static final String SYSTEM_KEY_STORE            = "javax.net.ssl.keyStore";
    private static final String SYSTEM_KEY_STORE_PASSWORD   = "javax.net.ssl.keyStorePassword";
    private static final String SYSTEM_TRUST_STORE          = "javax.net.ssl.trustStore";
    private static final String SYSTEM_TRUST_STORE_PASSWORD = "javax.net.ssl.trustStorePassword";
    private static final String SPRIT_CHAR                  = ":";
    private String              mKeyStoreUrl;
    private String              mKeyStoreType;
    private String              mKeyStorePassword;
    private String              mTrustStoreUrl;
    private String              mTrustStoreType;
    private String              mTrustStorePassword;
    private String              mCipherSuiteList;
    private boolean             mVerifyServerCertificate;
    
    public SSLProperties(AltibaseProperties aProps)
    {
        this.mKeyStoreUrl = aProps.getProperty(KEYSTORE_URL);
        this.mKeyStoreType = aProps.getProperty(KEYSTORE_TYPE);
        this.mKeyStorePassword = aProps.getProperty(KEYSTORE_PASSWORD);
        this.mTrustStoreUrl = aProps.getProperty(TRUSTSTORE_URL);
        this.mTrustStoreType = aProps.getProperty(TRUSTSTORE_TYPE);
        this.mTrustStorePassword = aProps.getProperty(TRUSTSTORE_PASSWORD);
        this.mCipherSuiteList = aProps.getProperty(CIPHERSUITE_LIST);
        this.mVerifyServerCertificate = aProps.getBooleanProperty(VERIFY_SERVER_CERTIFICATE, true);
    }
    
    /**
     * Ssl 프로퍼티가 null인 경우 System 프로퍼티를 확인해 가져온다.
     * @param aSslPropValue jdbc property value
     * @param aSysPropName system property name
     * @return
     */
    private String checkAndGetValueFromSystemProp(String aSslPropValue, String aSysPropName)
    {
        String sResult = aSslPropValue;
        
        if (StringUtils.isEmpty(aSslPropValue) && !StringUtils.isEmpty(System.getProperty(aSysPropName)))
        {
            sResult = System.getProperty(aSysPropName);
        }
        
        return sResult;
    }
    
    public String getKeyStoreUrl()
    {
        return checkAndGetValueFromSystemProp(mKeyStoreUrl, SYSTEM_KEY_STORE);
    }
    
    public void setKeyStoreUrl(String aKeyStoreUrl)
    {
        this.mKeyStoreUrl = aKeyStoreUrl;
    }
    
    public String getKeyStoreType()
    {
        return mKeyStoreType;
    }
    
    public void setKeyStoreType(String aKeyStoreType)
    {
        this.mKeyStoreType = aKeyStoreType;
    }
    
    public String getKeyStorePassword()
    {
        return checkAndGetValueFromSystemProp(mKeyStorePassword, SYSTEM_KEY_STORE_PASSWORD);
    }
    
    public void setKeyStorePassword(String aKeyStorePassword)
    {
        this.mKeyStorePassword = aKeyStorePassword;
    }
    
    public String getTrustStoreUrl()
    {
        return checkAndGetValueFromSystemProp(mTrustStoreUrl, SYSTEM_TRUST_STORE);
    }
    
    public void setTrustStoreUrl(String aTrustStoreUrl)
    {
        this.mTrustStoreUrl = aTrustStoreUrl;
    }
    
    public String getTrustStoreType()
    {
        return mTrustStoreType;
    }
    
    public void setTrustStoreType(String aTrustStoreType)
    {
        this.mTrustStoreType = aTrustStoreType;
    }
    public String getTrustStorePassword()
    {
        return checkAndGetValueFromSystemProp(mTrustStorePassword, SYSTEM_TRUST_STORE_PASSWORD);
    }
    
    public void setTrustStorePassword(String aTrustStorePassword)
    {
        this.mTrustStorePassword = aTrustStorePassword;
    }
    
    public boolean verifyServerCertificate()
    {
        return mVerifyServerCertificate;
    }
    
    public void setVerifyServerCertificate(boolean aVerifyServerCertificate)
    {
        this.mVerifyServerCertificate = aVerifyServerCertificate;
    }

    public String[] getCipherSuiteList()
    {
        String[] sCipherSuiteList = null;
        if (!StringUtils.isEmpty(mCipherSuiteList))
        {
            // PROJ-2474 문자열에서 먼저 공백을 제거하고 콜론으로 구분한 문자열 배열을 만든다. 
            mCipherSuiteList = mCipherSuiteList.replaceAll(" ", "");
            sCipherSuiteList = mCipherSuiteList.split(SPRIT_CHAR);
        }
        return sCipherSuiteList;
    }

    public String toString()
    {
        return "SslCertificateInfo [mKeyStoreUrl=" + mKeyStoreUrl + ", mKeyStoreType=" + mKeyStoreType + ", "
                + "mKeyStorePassword=" + mKeyStorePassword + ", mTrustStoreUrl=" + mTrustStoreUrl + ", "
                        + "mTrustStoreType=" + mTrustStoreType + ", mTrustStorePassword=" + mTrustStorePassword + 
                        ", mVerifyServerCertificate=" + mVerifyServerCertificate + "]";
    }
}
