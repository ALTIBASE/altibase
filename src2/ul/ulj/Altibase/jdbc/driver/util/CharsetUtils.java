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

package Altibase.jdbc.driver.util;

import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;
import java.util.HashMap;

public final class CharsetUtils
{
    private static final Charset        ASCII_CHARSET    = Charset.forName("US-ASCII");
    private static final Charset        UTF16BE_CHARSET  = Charset.forName("UTF-16BE");
    private static final HashMap        mNLSToCharsetMap = new HashMap();

    static
    {
        mNLSToCharsetMap.put("ASCII", "US-ASCII");
        mNLSToCharsetMap.put("US-ASCII", "US-ASCII");
        mNLSToCharsetMap.put("US7ASCII", "US-ASCII");
        mNLSToCharsetMap.put("CL8KOI8R", "US-ASCII");

        mNLSToCharsetMap.put("EUC-KR", "EUC_KR");
        mNLSToCharsetMap.put("EUC_KR", "EUC_KR");
        mNLSToCharsetMap.put("EUCKR", "EUC_KR");

        mNLSToCharsetMap.put("KO16KSC5601", "KS_C_5601-1987");
        mNLSToCharsetMap.put("KOREAN", "KS_C_5601-1987");
        mNLSToCharsetMap.put("KSC5601", "KS_C_5601-1987");

        mNLSToCharsetMap.put("CP949", "MS949");
        mNLSToCharsetMap.put("WINDOWS949", "MS949");
        mNLSToCharsetMap.put("MS949", "MS949");

        mNLSToCharsetMap.put("UTF16", "UTF-16BE");
        mNLSToCharsetMap.put("UTF-16", "UTF-16BE");
        mNLSToCharsetMap.put("UTF-16BE", "UTF-16BE");
        mNLSToCharsetMap.put("UTF-16LE", "UTF-16LE");
        mNLSToCharsetMap.put("UTF-8", "UTF-8");
        mNLSToCharsetMap.put("UTF8", "UTF-8");
        mNLSToCharsetMap.put("UTF_8", "UTF-8");

        mNLSToCharsetMap.put("BIG5", "Big5");
        mNLSToCharsetMap.put("ZHT16BIG5", "Big5");
        
        mNLSToCharsetMap.put("CHINESE", "GB2312");
        mNLSToCharsetMap.put("GB2312", "GB2312");
        mNLSToCharsetMap.put("GB231280", "GB2312");
        mNLSToCharsetMap.put("ZHS16CGB231280", "GB2312");
        mNLSToCharsetMap.put("ZHS16CGB231280FIXED", "GB2312");

        /* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
        mNLSToCharsetMap.put("GBK", "GBK");
        mNLSToCharsetMap.put("CP936", "GBK");
        mNLSToCharsetMap.put("MS936", "GBK");
        mNLSToCharsetMap.put("WINDOWS949", "GBK");
        mNLSToCharsetMap.put("ZHS16GBK", "GBK");
        mNLSToCharsetMap.put("ZHS16GBKFIXED", "GBK");

        mNLSToCharsetMap.put("EUC-JP", "EUC-JP");
        mNLSToCharsetMap.put("EUCJP", "EUC-JP");
        mNLSToCharsetMap.put("EUC_JP", "EUC-JP");
        
        mNLSToCharsetMap.put("JAPANESE", "Shift_JIS");
        mNLSToCharsetMap.put("SHIFTJIS", "Shift_JIS");

        /* PROJ-2590 [기능성] CP932 database character set 지원 */
        mNLSToCharsetMap.put("MS932", "MS932");
        mNLSToCharsetMap.put("CP932", "MS932");
        mNLSToCharsetMap.put("WINDOWS932", "MS932");
    }

    private CharsetUtils()
    {
    }

    public static Charset getCharset(String aNLSName)
    {
        return Charset.forName((String)mNLSToCharsetMap.get(aNLSName.toUpperCase()));
    }
    
    public static Charset getAsciiCharset()
    {
        return ASCII_CHARSET;
    }
    
    public static CharsetEncoder newAsciiEncoder()
    {
        return ASCII_CHARSET.newEncoder();
    }
    
    public static CharsetDecoder newAsciiDecoder()
    {
        return ASCII_CHARSET.newDecoder();
    }

    public static CharsetEncoder newUTF16Encoder()
    {
        return UTF16BE_CHARSET.newEncoder();
    }
}
