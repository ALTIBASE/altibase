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

/**
 * Database character set on ADLP protocol
 */
public abstract class DBCharSet
{
    public static final int None     = 0;
    public static final int UTF8     = 10000;
    public static final int UTF16    = 11000;
    public static final int ASCII    = 20000;
    public static final int KSC5601  = 30000;
    public static final int MS949    = 31000;
    public static final int SHIFTJIS = 40000;
    public static final int EUCJP    = 41000;
    /* PROJ-2590 support CP932 database character set */
    public static final int MS932    = 42000;
    public static final int GB231280 = 50000;
    public static final int BIG5     = 51000;
    /* PROJ-2414 GBK, CP936 character set */
    public static final int MS936    = 52000;

    /**
     * Get database character set name on JAVA from database character set value of ALTIBASE server
     *
     * @param aDBCharSet Database character set value of ALTIBASE server
     * @return Database character set name on JAVA
     */
    public static String getDBCharSetName(int aDBCharSet)
    {
        String sDBCharSetName = null;

        switch (aDBCharSet)
        {
        case UTF8:     sDBCharSetName = "UTF-8";     break;
        case UTF16:    sDBCharSetName = "UTF-16";    break;
        case ASCII:    sDBCharSetName = "US-ASCII";  break;
        case KSC5601:  sDBCharSetName = "KSC5601";   break;
        case MS949:    sDBCharSetName = "MS949";     break;
        case SHIFTJIS: sDBCharSetName = "Shift_JIS"; break;
        case EUCJP:    sDBCharSetName = "EUC_JP";    break;
        case GB231280: sDBCharSetName = "GB2312";    break;
        case BIG5:     sDBCharSetName = "Big5";      break;
         /* PROJ-2414 GBK, CP936 character set */
        case MS936:    sDBCharSetName = "GBK";       break;
        /* PROJ-2590 support CP932 database character set */
        case MS932:    sDBCharSetName = "MS932";     break;
        default:       sDBCharSetName = "";          break;
        }

        return sDBCharSetName;
    }
    
    public static int getCharSetScale( int aDBCharSet )
    {
        int sScale = 0;
        switch (aDBCharSet)
        {
            case UTF8:     sScale = 3; break;
            case UTF16:    sScale = 2; break;
            case ASCII:    sScale = 1; break;
            case KSC5601:  sScale = 2; break;
            case MS949:    sScale = 2; break;
            case SHIFTJIS: sScale = 2; break;
            case EUCJP:    sScale = 3; break;
            case GB231280: sScale = 2; break;
            case BIG5:     sScale = 2; break;
            case MS936:    sScale = 2; break;
            case MS932:    sScale = 2; break;
            default:       sScale = 1; break;
        }
        return sScale;
    }
}
